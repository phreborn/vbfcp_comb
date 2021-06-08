/*
 * =====================================================================================
 *
 *       Filename:  splitter.cxx
 *
 *    Description:  Workspace splitter
 *
 *        Version:  1.0
 *        Created:  05/19/2012 10:09:55 PM
 *       Revision:  12/27/20 during pandemic
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji, haoshuang.ji@cern.ch 
 *                  Hongtao Yang, Hongtao.Yang@cern.ch
 *   Organization:  University of Wisconsin
 *                  Lawrence Berkeley National Lab
 *
 * =====================================================================================
 */

#include "splitter.h"

#include "RooFormulaVarExt.h"

using namespace std;
using namespace RooFit;
using namespace RooStats;

TString splitter::WGTNAME = "_weight_";
TString splitter::PDFPOSTFIX = "_deComposed";

splitter::splitter(
    TString inputFileName,
    TString outputFileName,
    TString wsName,
    TString mcName,
    TString dataName)
{
  m_outputFileName = outputFileName;
  m_inputFile.reset(TFile::Open(inputFileName));
  if (!m_inputFile.get())
    auxUtil::alertAndAbort(Form("Input file %s does not exist", inputFileName.Data()));
  m_comb = dynamic_cast<RooWorkspace *>(m_inputFile->Get(wsName));
  if (!m_comb)
    auxUtil::alertAndAbort(Form("Workspace %s does not exist in file %s", wsName.Data(), inputFileName.Data()));
  m_mc = dynamic_cast<ModelConfig *>(m_comb->obj(mcName));
  if (!m_mc)
    auxUtil::alertAndAbort(Form("ModelConfig %s does not exist in file %s", mcName.Data(), inputFileName.Data()));

  if (m_mc->GetNuisanceParameters()->getSize() == 0)
    spdlog::warn("There is no nuisance parameter defined in ModelConfig {} of file {}", mcName.Data(), inputFileName.Data());
  if (m_mc->GetGlobalObservables()->getSize() == 0)
    spdlog::warn("There is no global observable defined in ModelConfig {} of file {}", mcName.Data(), inputFileName.Data());
  if (m_mc->GetParametersOfInterest()->getSize() == 0)
    spdlog::warn("There is no parameter of interest defined in ModelConfig {} of file {}", mcName.Data(), inputFileName.Data());

  if (!m_mc->GetPdf())
    auxUtil::alertAndAbort(Form("ModelConfig %s does not point to a valid PDF", mcName.Data()));

  if (!m_comb->data(dataName))
    auxUtil::alertAndAbort(Form("Dataset %s does not exist in file %s", dataName.Data(), inputFileName.Data()));

  m_pdf = dynamic_cast<RooSimultaneous *>(m_mc->GetPdf());
  if (!m_pdf)
  {
    spdlog::warn("PDF in workspace {} of file {} is not a RooSimultaneous PDF. Will create one", wsName.Data(), inputFileName.Data());
    buildSimPdf(m_mc->GetPdf(), m_comb->data(dataName));
  }
  m_cat = (RooCategory *)(&m_pdf->indexCat());
  m_numChannels = m_cat->numBins(0);

  m_data = dynamic_cast<RooDataSet *>(m_comb->data(dataName));
  if (!m_data)
  {
    spdlog::warn("Dataset in workspace {} of file {} is RooDataHist. Convert it to RooDataSet...", wsName.Data(), inputFileName.Data());
    histToDataset(dynamic_cast<RooDataHist *>(m_comb->data(dataName)));
  }
  m_dataList = m_data->split(*m_cat, true);

  m_editRFV = false;
  m_reBin = -1;
  m_rebuildPdf = false;
}

void splitter::printSummary()
{
  auxUtil::printTitle("Begin Summary", '~');
  spdlog::info("There are {} categories:", m_numChannels);
  for (int i = 0; i < m_numChannels; i++)
  {
    m_cat->setBin(i);
    TString channelName = m_cat->getLabel();
    RooAbsPdf *pdfi = m_pdf->getPdf(channelName);
    RooDataSet *datai = (RooDataSet *)(m_dataList->FindObject(channelName));
    spdlog::info("\tIndex: {}, Pdf: {}, Data: {}, SumEntries: {}", i, pdfi->GetName(), datai->GetName(), datai->sumEntries());
  }

  auxUtil::printTitle("POI", '#');
  m_mc->GetParametersOfInterest()->Print("v");

  auxUtil::printTitle("Dataset", '#');
  std::list<RooAbsData *> allData = m_comb->allData();
  for (std::list<RooAbsData *>::iterator it = allData.begin(); it != allData.end(); it++)
    (*it)->Print();

  auxUtil::printTitle("End Summary", '~');
}

void splitter::fillIndices(TString indices)
{
  indices.ToLower();
  if (indices == "all")
  {
    int num = m_cat->numBins(0);
    for (int i = 0; i < num; i++)
    {
      m_useIndices.push_back(i);
    }
    return;
  }

  /* 0-5,7-9 */
  auxUtil::removeWhiteSpace(indices);
  TObjArray *iArray = TString(indices).Tokenize(",");
  int iNum = iArray->GetEntries();
  TString iStr, jStr;
  for (int i = 0; i < iNum; i++)
  {
    iStr = ((TObjString *)iArray->At(i))->GetString();
    TObjArray *jArray = iStr.Tokenize("-");
    int jNum = jArray->GetEntries();
    if (jNum == 1)
    {
      jStr = ((TObjString *)jArray->At(0))->GetString();
      jStr.ReplaceAll(" ", "");
      spdlog::info("Adding index: {}", jStr.Atoi());
      m_useIndices.push_back(jStr.Atoi());
    }
    else if (jNum == 2)
    {
      TString str1, str2;
      str1 = ((TObjString *)jArray->At(0))->GetString();
      str2 = ((TObjString *)jArray->At(1))->GetString();
      int int1 = str1.Atoi();
      int int2 = str2.Atoi();
      assert(int1 <= int2);
      for (int t = int1; t <= int2; t++)
      {
        spdlog::info("Adding index: {}", t);
        m_useIndices.push_back(t);
      }
    }
    else
    {
      auxUtil::alertAndAbort("Invalid syntax " + iStr);
    }
  }
}

void splitter::makeWorkspace()
{
  const int subNumChannels = m_useIndices.size();
  if (subNumChannels == 0)
  {
    spdlog::warn("No sub-channel selected, Exit... ");
    return;
  }
  unique_ptr<RooWorkspace> subComb(new RooWorkspace(m_comb->GetName(), m_comb->GetTitle()));
  unique_ptr<RooCategory> subCat(new RooCategory(m_cat->GetName(), m_cat->GetTitle()));

  RooArgSet subNuis, subGobs, subObs, subPoi;
  std::map<std::string, RooAbsPdf *> subPdfMap;
  std::map<std::string, RooDataSet *> subDataMap;

  int index = 0;
  for (int i = 0; i < subNumChannels; i++)
  {
    index = m_useIndices[i];
    spdlog::info("Sub-index --> {}", index);
    m_cat->setBin(index);
    TString channelName = m_cat->getLabel();
    RooAbsPdf *pdfi = m_pdf->getPdf(channelName);
    RooDataSet *datai = dynamic_cast<RooDataSet *>(m_dataList->FindObject(channelName));
    /* make category */
    spdlog::info("\tChannel name --> {}", channelName.Data());
    subCat->defineType(channelName);
    /* make observables */
    RooArgSet *indivObs = pdfi->getObservables(*datai);
    subObs.add(*indivObs);
    /* make nuisances */
    RooArgSet *indivNuis = pdfi->getParameters(*indivObs);
    std::unique_ptr<TIterator> iter(indivNuis->createIterator());

    for (RooRealVar *v = dynamic_cast<RooRealVar *>(iter->Next()); v != 0; v = dynamic_cast<RooRealVar *>(iter->Next()))
    {
      RooRealVar *var = dynamic_cast<RooRealVar*>(m_mc->GetParametersOfInterest()->find(v->GetName()));
      if (var)
      {
        subPoi.add(*var, true);
        continue;
      }

      /* in original global observables */
      var = dynamic_cast<RooRealVar*>(m_mc->GetGlobalObservables()->find(v->GetName()));
      if (var)
      {
        subGobs.add(*var);
        continue;
      }

      /* Any other free parameters should be counted as nuisance parameters */
      if (!(v->isConstant()))
      {
        subNuis.add(*v);
      }
    }

    if (m_rebuildPdf)
      pdfi = rebuildCatPdf(pdfi, datai);
    subPdfMap[channelName.Data()] = pdfi;

    /* Handle dataset */
    if (m_reBin > 0)
    {
      int numEntries = datai->numEntries();
      int sumEntries = datai->sumEntries();

      bool isBinned = (numEntries != sumEntries);
      isBinned += (numEntries < m_reBin);
      if (isBinned)
      {
        subCat->setLabel(channelName, true);
        subDataMap[channelName.Data()] = rebuildCatData(datai, indivObs);
      }
      else
      {
        TString dataiName = datai->GetName();
        spdlog::info("Rebin {}", dataiName.Data());
        datai->SetName((dataiName + "_old"));

        RooRealVar weight(WGTNAME, "", 1.);
        RooArgSet obsPlusW(*indivObs, weight);

        RooRealVar *obsVar = dynamic_cast<RooRealVar *>(indivObs->first());
        TH1 *hist = datai->createHistogram((dataiName + "_hist"), *obsVar, RooFit::Binning(m_reBin, obsVar->getMin(), obsVar->getMax()));
        RooBinning rebin(m_reBin, obsVar->getMin(), obsVar->getMax());
        obsVar->setBinning(rebin);

        RooDataSet *dataiNew = new RooDataSet(dataiName, "", obsPlusW, WGTNAME);

        for (int i = 1, n = hist->GetNbinsX(); i <= n; ++i)
        {
          obsVar->setVal(hist->GetXaxis()->GetBinCenter(i));
          dataiNew->add(*indivObs, hist->GetBinContent(i));
        }

        subCat->setLabel(channelName, true);
        subDataMap[channelName.Data()] = dataiNew;
        m_keep.Add(dataiNew);
      }
    }
    else
      subDataMap[channelName.Data()] = rebuildCatData(datai, indivObs);
  }

  if (m_editRFV)
  {
    spdlog::info("Edit RooFormulaVar");
    /* take this opportunity to edit RooFormulaVar */
    std::unique_ptr<TIterator> fVarIter(m_comb->componentIterator());
    for (RooRealVar *v = (RooRealVar *)fVarIter->Next(); v != 0; v = (RooRealVar *)fVarIter->Next())
    {
      if (TString(v->ClassName()) == "RooFormulaVar")
      {
        RooFormulaVar *oldVar = dynamic_cast<RooFormulaVar *>(m_comb->obj(v->GetName()));
        assert(oldVar);
        TString varName = v->GetName();

        RooFormulaVarExt oldVarExt(*oldVar, "oldVarExtName");

        /* create a new one to hold the place */
        RooFormulaVar *newVar = NULL;
        oldVarExt.Rebuild(newVar, varName.Data(), false);
        if (newVar)
        {
          subComb->import(*newVar);
          newVar->Print();
        }
      }
    }
  }

  subComb->import(*subCat, RooFit::Silence());

  unique_ptr<RooSimultaneous> subPdf(new RooSimultaneous(m_pdf->GetName(), m_pdf->GetTitle(), subPdfMap, *subCat));
  subComb->import(*subPdf,
                    RooFit::RecycleConflictNodes(),
                    RooFit::Silence());

  subObs.add(*subCat);
  RooRealVar weightVar(WGTNAME, "", 1);
  RooArgSet obsAndWgt(subObs, weightVar);
  unique_ptr<RooDataSet> subData(new RooDataSet(m_data->GetName(), m_data->GetTitle(), obsAndWgt, RooFit::Index(*subCat), RooFit::Import(subDataMap), RooFit::WeightVar(WGTNAME)));

  spdlog::debug("numEntries: {}", subData->numEntries());
  spdlog::debug("sumEntries: {}", subData->sumEntries());

  subComb->import(*subData);
  subComb->importClassCode();

  unique_ptr<ModelConfig> subMc(new ModelConfig(m_mc->GetName(), subComb.get()));
  subMc->SetWorkspace(*subComb);
  subMc->SetPdf(*subPdf);
  subMc->SetProtoData(*subData);
  subMc->SetNuisanceParameters(subNuis);
  subMc->SetGlobalObservables(subGobs);
  subMc->SetParametersOfInterest(subPoi);
  subMc->SetObservables(subObs);
  subComb->import(*subMc);

  /* Copy snapshots */
  for (auto snapshotName : m_snapshots)
  {
    if (m_comb->loadSnapshot(snapshotName))
    {
      RooArgSet *snapshot = const_cast<RooArgSet*>(m_comb->getSnapshot(snapshotName));
      subComb->saveSnapshot(snapshotName, *snapshot);
    }
  }
  m_inputFile->Close();

  unique_ptr<TFile> outputFile(TFile::Open(m_outputFileName, "recreate"));
  subComb->Write();
  outputFile->Close();

  spdlog::info("Output file {} saved", m_outputFileName.Data());
}

void splitter::buildSimPdf(RooAbsPdf *pdf, RooAbsData *data)
{
  TString channelName = pdf->GetName();
  RooCategory cat(channelName + "_single", channelName + "_single");
  cat.defineType(channelName);

  std::map<std::string, RooAbsPdf *> pdfMap;
  std::map<std::string, RooAbsData *> dataMap;
  /* make pdf */
  pdfMap[channelName.Data()] = pdf;
  /* make data */
  dataMap[channelName.Data()] = data;

  TString pdfName = pdf->GetName();
  TString dataName = data->GetName();

  RooSimultaneous *pdf_new = new RooSimultaneous(pdfName + "_sim", pdfName + "_sim", pdfMap, cat);
  m_keep.Add(pdf_new);

  RooArgSet obsAndWgt = *data->get();
  RooRealVar weightVar(WGTNAME, "", 1);
  obsAndWgt.add(weightVar);
  RooDataSet *data_new = new RooDataSet(
      dataName + "_sim",
      dataName + "_sim",
      obsAndWgt,
      RooFit::Index(cat),
      RooFit::Link(dataMap),
      RooFit::WeightVar(weightVar) /* actually just pass a name */
  );
  m_keep.Add(data_new);

  m_pdf = pdf_new;
  m_data = data_new;
}

void splitter::histToDataset(RooDataHist *data)
{
  std::map<std::string, RooDataSet *> datasetMap;

  RooArgSet Observables;
  RooRealVar weightVar(WGTNAME, "", 1);

  for (int ich = 0; ich < m_numChannels; ich++)
  {
    m_cat->setBin(ich);
    TString channelName = m_cat->getLabel();
    RooAbsPdf *pdfi = m_pdf->getPdf(channelName);
    RooAbsData *datai = (RooAbsData *)(m_dataList->FindObject(channelName));
    RooArgSet *obsi = pdfi->getObservables(datai);

    RooArgSet obsAndWgt(*obsi, weightVar);

    TString dataName = datai->GetName();
    RooDataSet *data = new RooDataSet(dataName + "_convert", dataName + "_convert", obsAndWgt, WeightVar(weightVar));
    m_keep.Add(data);

    for (int ievt = 0; ievt < datai->numEntries(); ++ievt)
    {
      *obsi = *datai->get(ievt);
      double dataWgt = datai->weight();
      data->add(obsAndWgt, dataWgt);
    }
    Observables.add(*obsi);
    datasetMap[channelName.Data()] = data;
  }

  RooArgSet obsAndWgt(Observables, weightVar);

  RooDataSet *combData = new RooDataSet(data->GetName(), data->GetTitle(), obsAndWgt, Index(*m_cat), Import(datasetMap), WeightVar(weightVar));
  m_keep.Add(combData);

  m_data = combData;
}

RooAbsPdf *splitter::rebuildCatPdf(RooAbsPdf *pdfi, RooAbsData *datai)
{
  if (TString(pdfi->ClassName()) == "RooProdPdf")
  {
    /* strip those disconnected pdfs from minimization */
    unique_ptr<RooArgSet> cPars(pdfi->getParameters(datai));
    RooArgSet dPars = *cPars;
    unique_ptr<RooArgSet> constraints(pdfi->getAllConstraints(*datai->get(), *cPars, true));
    unique_ptr<RooArgSet> disConstraints(pdfi->getAllConstraints(*datai->get(), dPars, false));
    disConstraints->remove(*constraints);

    RooArgSet baseComponents;
    auxUtil::getBasePdf(dynamic_cast<RooProdPdf *>(pdfi), baseComponents);
    /* remove disconnected pdfs */
    baseComponents.remove(*disConstraints);

    TString newPdfName = TString(pdfi->GetName()) + "_deComposed";
    pdfi = new RooProdPdf(newPdfName, newPdfName, baseComponents);
    m_keep.Add(pdfi);
  }
  return pdfi;
}

RooDataSet *splitter::rebuildCatData(RooAbsData *datai, RooArgSet *indivObs)
{
  RooRealVar weight(WGTNAME, "", 1.);
  RooArgSet obsAndWgt(*indivObs, weight);

  RooDataSet *dataNew_i = new RooDataSet(TString(datai->GetName()) + PDFPOSTFIX, "", obsAndWgt, WeightVar(WGTNAME));

  for (int j = 0, nEntries = datai->numEntries(); j < nEntries; ++j)
  {
    *indivObs = *datai->get(j);
    double dataWgt = datai->weight();
    dataNew_i->add(obsAndWgt, dataWgt);
  }

  m_keep.Add(dataNew_i);
  return dataNew_i;
}
