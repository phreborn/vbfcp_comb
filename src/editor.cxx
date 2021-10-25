/*
 * =====================================================================================
 * *       Filename:  editor.cxx
 *
 *    Description:  Edit workspace
 *
 *        Version:  1.1
 *        Created:  07/19/12 17:33:20
 *       Revision:  10/25/21 during pandemic
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji, haoshuang.ji@cern.ch 
 *                  Hongtao Yang, Hongtao.Yang@cern.ch
 *   Organization:  University of Wisconsin
 *                  Lawrence Berkeley National Lab
 * =====================================================================================
 */
#include "editor.h"
#include "textTable.h"

const TString editor::CONSTRAINT = "constraint";
const TString editor::NORMAL = "normal";
const TString editor::EXTERNAL = "external";

editor::editor(TString configFileName)
{
  _asimovHandler.reset(new asimovUtil());
  readConfigXml(configFileName);
}

bool editor::run()
{
  std::unique_ptr<TFile> f(TFile::Open(m_inFile));
  m_oW = auxUtil::getWorkspaceFromFile(f.get(), m_wsName);
  m_oMc = auxUtil::getModelConfigFromWorkspace(m_oW, m_mcName);

  // Implement objects
  auxUtil::printTitle("Step 1: create new objects");
  // Everything first goes into a temporary workspace
  std::unique_ptr<RooWorkspace> wTemp(new RooWorkspace(m_wsName));
  int nItems = (int)m_actionItems.size();
  for (int i = 0; i < nItems; i++)
  {
    const TString actionStr = m_actionItems[i];
    const TString actionType = m_actionTypes[i];

    spdlog::info("Item {} ({}), {}", i, actionType.Data(), actionStr.Data());

    TString objName = auxUtil::getObjName(actionStr);
    if (actionStr.Contains("FlexibleInterpVar"))
      implementFlexibleInterpVar(wTemp.get(), actionStr);
    else if (actionStr.Contains("RooMultiVarGaussian"))
      implementMultiVarGaussian(wTemp.get(), actionStr);
    else
      auxUtil::implementObj(wTemp.get(), actionStr);
  }

  // Read rename map
  std::map<TString, TString> renameMap;
  // "EDIT::NEWPDF(OLDPDF,mu=invBRsum)"
  // cout << "Rename action: " << m_mapStr << endl;
  Ssiz_t firstC = m_mapStr.First(",") + 1;
  Ssiz_t firstR = m_mapStr.First(")");
  TString renameStr = m_mapStr(firstC, firstR - firstC);
  renameStr = renameStr.Strip(TString::kTrailing, ',');
  // cout << "Rename action: " << renameStr << endl;

  std::vector<TString> renameList = auxUtil::splitString(renameStr, ',');
  TextTable t('-', '|', '+');
  t.add("Old");
  t.add("New");
  t.endOfRow();
  for (auto iStr : renameList)
  {
    if (!iStr.Contains("="))
      auxUtil::alertAndAbort("Renaming syntax " + iStr + " not valid");

    Ssiz_t firstE = iStr.First("=");
    Ssiz_t len = iStr.Length();
    TString oldObjName = iStr(0, firstE);
    TString newObjName = iStr(firstE + 1, len);
    if (!m_oW->arg(oldObjName))
    {
      if (m_isStrict)
        auxUtil::alertAndAbort("Object " + oldObjName + " (-> " + newObjName + ") does not exist in the old workspace"); // If it does not exist in the old workspace, that's probably okay
      else
      {
        spdlog::warn("Object {} does not exist in the old workspace, skipping...", oldObjName.Data()); // If it does not exist in the old workspace, that's probably okay
        continue;
      }
    }
    if (!wTemp->arg(newObjName))
      auxUtil::alertAndAbort("Object " + newObjName + " (<- " + oldObjName + ") does not exist in the temporary workspace"); // If it is missing in the new workspace, it is not acceptable
    if (renameMap.find(oldObjName.Data()) != renameMap.end())
      auxUtil::alertAndAbort("Object " + oldObjName + " is renamed more than once"); // Same variable renamed multiple times
    renameMap[oldObjName.Data()] = newObjName.Data();
    t.add(oldObjName.Data());
    t.add(newObjName.Data());
    t.endOfRow();
  }
  spdlog::info("Replace table:");
  std::cout << t;

  TString oldStr = "";
  TString newStr = "";
  auxUtil::linkMap(renameMap, oldStr, newStr, ",");

  // Rename objects through import
  auxUtil::printTitle("Step 2: renaming objects");
  wTemp->import(*m_oMc->GetPdf(),
                RooFit::RenameVariable(oldStr, newStr),
                RooFit::RecycleConflictNodes());

  // Save output workspace. If needed remake the categories
  auxUtil::printTitle("Step 3: Making output workspace");

  // If there are additional constraint pdf to be attached
  if (m_constraintPdf.size() > 0)
  {
    // make new workspace at the beginning
    m_nW.reset(new RooWorkspace(m_wsName));
    spdlog::info("Attaching additional constraint terms");
    remakeCategories(wTemp.get());
  }
  else
  {
    m_nW = std::move(wTemp);
  }

  // Start preparing ModelConfig
  m_nMc.reset(new ModelConfig(m_mcName, m_nW.get()));

  // The RooSimultaneous pdf name is not supposed to be changed
  RooAbsPdf *nPdf = m_nW->pdf(m_oMc->GetPdf()->GetName());
  m_nMc->SetPdf(*nPdf);
  /* import data */
  std::list<RooAbsData *> dataList = m_oW->allData();
  for (std::list<RooAbsData *>::iterator it = dataList.begin(); it != dataList.end(); ++it)
    m_nW->import(**it);

  /* poi */
  RooArgSet newPOI;
  spdlog::debug("List of POIs in the new parameterization:");
  for (int i = 0; i < (int)m_poiNames.size(); i++)
  {
    RooRealVar *var = auxUtil::checkVarExist(m_nW.get(), m_poiNames[i], nPdf);
    if (!var)
    {
      if (m_isStrict)
        auxUtil::alertAndAbort("POI " + m_poiNames[i] + " does not exist");
      else
      {
        spdlog::warn("POI {} does not exist, skipping...", m_poiNames[i].Data());
        continue;
      }
    }
    spdlog::debug(m_poiNames[i].Data());
    /* float by default */
    var->setConstant(false);
    newPOI.add(*var);
  }
  m_nMc->SetParametersOfInterest(newPOI);

  // Set nuisance parameters, global observables, and observables
  RooArgSet nuis;
  RooArgSet gobs;
  RooArgSet mobs;
  if (m_oMc->GetNuisanceParameters())
  {
    std::unique_ptr<TIterator> iterN(m_oMc->GetNuisanceParameters()->createIterator());
    for (RooRealVar *v = (RooRealVar *)iterN->Next(); v != 0; v = (RooRealVar *)iterN->Next())
    {
      RooRealVar *var = auxUtil::checkVarExist(m_nW.get(), v->GetName(), nPdf);
      if (!var)
      {
        spdlog::warn("Old nuisance parameter {} no longer exists in the new workspace. Remove from ModelConfig...", v->GetName());
        continue;
      }
      if (var->isConstant())
      {
        spdlog::warn("Nuisance parameter {} is set to constant. Floating the parameter...", v->GetName());
        var->setConstant(false);
      }
      nuis.add(*var);
    }
  }

  if (m_oMc->GetGlobalObservables())
  {
    std::unique_ptr<TIterator> iterG(m_oMc->GetGlobalObservables()->createIterator());
    for (RooRealVar *v = (RooRealVar *)iterG->Next(); v != 0; v = (RooRealVar *)iterG->Next())
    {
      RooRealVar *var = auxUtil::checkVarExist(m_nW.get(), v->GetName(), nPdf);
      if (!var)
      {
        spdlog::warn("Old global observable {} no longer exists in the new workspace. Remove from ModelConfig...", v->GetName());
        continue;
      }
      if (!var->isConstant())
      {
        spdlog::warn("Global observable {} is set to float. Fixing the parameter...", v->GetName());
        var->setConstant(true);
      }
      gobs.add(*var);
    }
  }

  // For observable it is special, as it may also contain RooCategory
  if (m_oMc->GetObservables())
  {
    std::unique_ptr<TIterator> iterM(m_oMc->GetObservables()->createIterator());
    for (RooAbsArg *v = (RooAbsArg *)iterM->Next(); v != 0; v = (RooAbsArg *)iterM->Next())
    {
      RooAbsArg *var = dynamic_cast<RooAbsArg *>(m_nW->obj(v->GetName()));
      if (!var)
      {
        auxUtil::alertAndAbort(Form("Old observable %s no longer exists in the new workspace", v->GetName())); // Renaming observable is currently not supported
      }
      mobs.add(*var);
    }
  }

  // Adding additional nuisance parameters and global observables
  for (std::map<TString, std::pair<TString, TString>>::iterator it = m_constraintPdf.begin(); it != m_constraintPdf.end(); ++it)
  {
    std::pair<TString, TString> NPGO = it->second;
    std::vector<TString> NPList = auxUtil::splitString(NPGO.first, ',');
    for (auto NPName : NPList)
    {
      RooRealVar *np = auxUtil::checkVarExist(m_nW.get(), NPName, nPdf);
      if (!np)
      {
        if (m_isStrict)
          auxUtil::alertAndAbort("Nuisance parameter " + NPName + " does not exist in the new workspace");
        else
        {
          spdlog::warn("Nuisance parameter {} does not exist in the new workspace. Skipping...", NPName.Data());
          continue;
        }
      }
        
      nuis.add(*np);
    }

    std::vector<TString> GOList = auxUtil::splitString(NPGO.second, ',');
    for (auto GOName : GOList)
    {
      RooRealVar *go = auxUtil::checkVarExist(m_nW.get(), GOName, nPdf);
      if (!go)
      {
        if (m_isStrict)
          auxUtil::alertAndAbort("Global observable " + GOName + " does not exist in the new workspace");
        else
        {
          spdlog::warn("Global observable {} does not exist in the new workspace. Skipping...", GOName.Data());
          continue;
        }
      }
      gobs.add(*go);
    }
  }

  m_nMc->SetNuisanceParameters(nuis);
  m_nMc->SetGlobalObservables(gobs);
  m_nMc->SetObservables(mobs);

  m_nW->import(*m_nMc);
  m_nW->importClassCode();

  // Copy snapshots
  RooArgSet everything_new, everything_old;
  auxUtil::collectEverything(m_nMc.get(), &everything_new);
  RooArgSet *everything_new_snapshot = dynamic_cast<RooArgSet *>(everything_new.snapshot());
  auxUtil::collectEverything(m_oMc, &everything_old);

  // Nuisance parameter snapshot
  for (auto snapshot : m_snapshotNP)
  {
    if (not m_oW->loadSnapshot(snapshot))
      spdlog::warn("nuisance parameter snapshot {} does not exist in the old workspace", snapshot.Data());
    nuis = *m_oMc->GetNuisanceParameters();
    m_nW->saveSnapshot(snapshot, nuis);
  }

  // Global observable snapshot
  for (auto snapshot : m_snapshotGO)
  {
    if (not m_oW->loadSnapshot(snapshot))
      spdlog::warn("global observable snapshot {} does not exist in the old workspace", snapshot.Data());
    gobs = *m_oMc->GetGlobalObservables();
    m_nW->saveSnapshot(snapshot, gobs);
  }

  // POI snapshot
  for (auto snapshot : m_snapshotPOI)
  {
    if (not m_oW->loadSnapshot(snapshot))
      spdlog::warn("POI snapshot {} does not exist in the old workspace", snapshot.Data());
    newPOI = *m_oMc->GetParametersOfInterest();
    m_nW->saveSnapshot(snapshot, newPOI);
  }

  // Everything snapshot
  for (auto snapshot : m_snapshotAll)
  {
    if (not m_oW->loadSnapshot(snapshot))
      spdlog::warn("Everything snapshot {} does not exist in the old workspace", snapshot.Data());

    everything_new = everything_old;
    m_nW->saveSnapshot(snapshot, everything_new);
  }

  everything_new = *everything_new_snapshot; // Recover values

  // Performing fits
  if (_asimovHandler->genAsimov())
    _asimovHandler->generateAsimov(m_nMc.get(), m_dsName);

  std::unique_ptr<TFile> fout(TFile::Open(m_outFile, "recreate"));
  m_nW->Write();
  fout->Close();
  f->Close();
  spdlog::info("Written to file: {}", m_outFile.Data());

  return true;
}

void editor::readConfigXml(TString filen)
{
  // cout<<__PRETTY_FUNCTION__<<endl;
  TDOMParser xmlparser;
  auxUtil::parseXMLFile(&xmlparser, filen);
  // cout<<"XML file parsed"<<endl;

  TXMLDocument *xmldoc = xmlparser.GetXMLDocument();
  TXMLNode *rootNode = xmldoc->GetRootNode();
  TListIter rootAttIt = rootNode->GetAttributes();
  TXMLNode *node = rootNode->GetChildren();

  m_inFile = auxUtil::getAttributeValue(rootNode, "InFile");
  m_outFile = auxUtil::getAttributeValue(rootNode, "OutFile");
  m_modelName = auxUtil::getAttributeValue(rootNode, "ModelName");
  m_wsName = auxUtil::getAttributeValue(rootNode, "WorkspaceName", true, "combWS");
  m_mcName = auxUtil::getAttributeValue(rootNode, "ModelConfigName", true, "ModelConfig");
  m_dsName = auxUtil::getAttributeValue(rootNode, "DataName", true, "combData");
  m_poiNames = auxUtil::splitString(auxUtil::getAttributeValue(rootNode, "POINames"), ',');
  m_snapshotNP = auxUtil::splitString(auxUtil::getAttributeValue(rootNode, "SnapshotNP", true, ""), ',');   // NP snapshots to be copied
  m_snapshotGO = auxUtil::splitString(auxUtil::getAttributeValue(rootNode, "SnapshotGO", true, ""), ',');   // GO snapshots to be copied
  m_snapshotPOI = auxUtil::splitString(auxUtil::getAttributeValue(rootNode, "SnapshotPOI", true, ""), ','); // POI snapshots to be copied
  m_snapshotAll = auxUtil::splitString(auxUtil::getAttributeValue(rootNode, "SnapshotAll", true, ""), ','); // Everything snapshots to be copied
  m_isStrict = auxUtil::to_bool(auxUtil::getAttributeValue(rootNode, "Strict", true, "true"));              // Strict mode
  // cout<<"Root node read"<<endl;

  /* root node children */
  while (node != 0)
  {
    const TString nodeName = node->GetNodeName();
    if (nodeName == "Item")
    {
      m_actionItems.push_back(auxUtil::getAttributeValue(node, "Name"));
      m_actionTypes.push_back(auxUtil::getAttributeValue(node, "Type", true, NORMAL)); // By default it is normal type
      if (m_actionTypes.back() == CONSTRAINT)
      {
        TString constrPdfName = auxUtil::getObjName(m_actionItems.back());
        TString NPName = auxUtil::getAttributeValue(node, "NP");
        TString GOName = auxUtil::getAttributeValue(node, "GO");
        TString fileName = auxUtil::getAttributeValue(node, "FileName", true, "");
        // If the pdf is saved in another file, then import it to current workspace
        if (fileName != "")
        {
          std::unique_ptr<TFile> fTemp(TFile::Open(fileName));
          // Workspace name is mandatory
          TString wsTempName = auxUtil::getAttributeValue(node, "WorkspaceName");
          RooWorkspace *wsTemp = auxUtil::getWorkspaceFromFile(fTemp.get(), wsTempName);
          RooAbsPdf *pdfTemp = auxUtil::getPdfFromWorkspace(wsTemp, constrPdfName);
          m_nW->import(*pdfTemp);
          fTemp->Close();
        }
        m_constraintPdf[constrPdfName] = std::make_pair(NPName, GOName);
      }
    }
    else if (nodeName == "Map")
    {
      m_mapStr = auxUtil::getAttributeValue(node, "Name");
    }
    else if (nodeName == "Asimov")
    {
      // cout<<"Filling Asimov handler"<<endl;
      _asimovHandler->addEntry(node);
    }
    node = node->GetNextNode();
  }
}

void editor::implementFlexibleInterpVar(RooWorkspace *w, TString actionStr)
{
  actionStr = actionStr.ReplaceAll(" ", "");
  actionStr = actionStr.ReplaceAll("FlexibleInterpVar::", "");

  TString responseName = actionStr;
  responseName = responseName(0, actionStr.First("("));

  Size_t begin = actionStr.First("(") + 1;
  Size_t end = actionStr.First(")");
  actionStr = actionStr(begin, end - begin);
  TObjArray *iArray = actionStr.Tokenize(",");
  int iNum = iArray->GetEntries();
  if (iNum != 5)
    auxUtil::alertAndAbort("the format of FlexibleInterpVar (" + actionStr + ") is not correct");

  TString NPName = ((TObjString *)iArray->At(0))->GetString();
  // if (!w->var(NPName))
  // {
    // cerr<<"ERROR: nuisance parameter "<<NPName<<" is missing."<<endl;
    // abort();
  // }

  RooArgList nuiList(*w->arg(NPName));

  double nominal = atof(((TObjString *)iArray->At(1))->GetString());
  double errHi = atof(((TObjString *)iArray->At(2))->GetString());
  double errLo = atof(((TObjString *)iArray->At(3))->GetString());
  int interpolation = atoi(((TObjString *)iArray->At(4))->GetString());

  std::vector<double> sigma_var_low, sigma_var_high;
  std::vector<int> code;

  sigma_var_low.push_back(nominal + errLo);
  sigma_var_high.push_back(nominal + errHi);
  code.push_back(interpolation);

  RooStats::HistFactory::FlexibleInterpVar expected_var(responseName, responseName, nuiList, nominal, sigma_var_low, sigma_var_high, code);

  w->import(expected_var);
}

void editor::implementMultiVarGaussian(RooWorkspace *w, TString actionStr)
{
  actionStr = actionStr.ReplaceAll(" ", "");
  actionStr = actionStr.ReplaceAll("RooMultiVarGaussian::", "");

  TString functionName = actionStr;
  functionName = functionName(0, actionStr.First("("));

  Size_t begin = actionStr.First("(") + 1;
  Size_t end = actionStr.First(")");
  actionStr = actionStr(begin, end - begin);
  TObjArray *iArray = actionStr.Tokenize(":");
  int iNum = iArray->GetEntries();
  if (iNum != 4)
    auxUtil::alertAndAbort("the format of RooMultiVarGaussian (" + actionStr + ") is not correct");

  RooArgList obsList, meanList;
  // Get inputs
  TString obsListStr = ((TObjString *)iArray->At(0))->GetString();
  obsListStr = obsListStr(obsListStr.First('{') + 1, obsListStr.First('}') - 1);
  std::vector<TString> obsListVec = auxUtil::splitString(obsListStr, ',');

  TString meanListStr = ((TObjString *)iArray->At(1))->GetString();
  meanListStr = meanListStr(meanListStr.First('{') + 1, meanListStr.First('}') - 1);
  std::vector<TString> meanListVec = auxUtil::splitString(meanListStr, ',');

  TString uncertListStr = ((TObjString *)iArray->At(2))->GetString();
  uncertListStr = uncertListStr(uncertListStr.First('{') + 1, uncertListStr.First('}') - 1);
  std::vector<TString> uncertListVec = auxUtil::splitString(uncertListStr, ',');

  TString correlationListStr = ((TObjString *)iArray->At(3))->GetString();
  correlationListStr = correlationListStr(correlationListStr.First('{') + 1, correlationListStr.First('}') - 1);
  std::vector<TString> correlationListVec = auxUtil::splitString(correlationListStr, ',');

  const int nPOI = obsListVec.size();
  for (int i = 0; i < nPOI; i++)
  {
    TString obsName = obsListVec[i];
    TString meanName = meanListVec[i];
    if (!w->arg(obsName))
      auxUtil::alertAndAbort("variable " + obsName + " does not exist in workspace");

    obsList.add(*w->arg(obsName));

    if (!w->arg(meanName))
      auxUtil::alertAndAbort("variable " + meanName + " does not exist in workspace");

    meanList.add(*w->arg(meanName));
  }

  TMatrixDSym V(nPOI);
  int corIdx = 0;
  for (int i = 0; i < nPOI; i++)
  {
    for (int j = 0; j < nPOI; j++)
    {
      if (i == j)
      {
        V(i, j) = uncertListVec[i].Atof() * uncertListVec[j].Atof();
      }
      else if (i < j)
      {
        // Problematic: ad-hoc implementaiton for the time-being
        if (corIdx >= nPOI)
        {
          auxUtil::alertAndAbort(Form("Number of index %d larger than array size %d!", corIdx, nPOI));
        }
        V(i, j) = correlationListVec[corIdx].Atof() * uncertListVec[i].Atof() * uncertListVec[j].Atof();
        V(j, i) = V(i, j);
        corIdx++;
      }
    }
  }

  V.Print();
  RooMultiVarGaussian model(functionName, functionName, obsList, meanList, V);
  w->import(model);
}

void editor::remakeCategories(RooWorkspace *w)
{
  // Get RooSimultaneous pdf
  TString pdfName = m_oMc->GetPdf()->GetName();
  RooSimultaneous *pdf = dynamic_cast<RooSimultaneous*>(w->pdf(pdfName));
  if (!pdf)
    auxUtil::alertAndAbort(Form("Pdf %s does not exist in workspace %s", pdfName.Data(), w->GetName()));
  RooCategory *cat = (RooCategory *)(&pdf->indexCat());
  const int ncat = cat->numBins(0);  

  // Get observables. All observables must exist
  std::unique_ptr<RooArgSet> observables(auxUtil::findArgSetIn(w, const_cast<RooArgSet *>(m_oMc->GetObservables()), true));
  // Get nuisance parameters. Some of them can be missing
  std::unique_ptr<RooArgSet> nuis(auxUtil::findArgSetIn(w, const_cast<RooArgSet *>(m_oMc->GetNuisanceParameters()), false));

  // Create new RooSimultaneous pdf
  std::map<std::string, RooAbsPdf *> pdfMap;
  std::shared_ptr<RooAbsPdf> pdfArr[ncat];

  for (int icat = 0; icat < ncat; icat++)
  {
    cat->setBin(icat);
    TString catName = cat->getLabel();

    spdlog::info("Creating new PDF for category {}", catName.Data());
    RooAbsPdf *pdfi = dynamic_cast<RooAbsPdf *>(pdf->getPdf(catName));

    RooArgSet baseComponents;

    RooArgList obsTerms, disConstraints;
    RooStats::FactorizePdf(*observables, *pdfi, obsTerms, disConstraints);
    baseComponents.add(obsTerms);

    // Remove constraint pdfs that are no longer needed
    if (disConstraints.getSize() > 0)
    {
      std::unique_ptr<RooArgSet> constraints(pdfi->getAllConstraints(*observables, *nuis, true));
      baseComponents.add(*constraints);
      // disConstraints.remove(*constraints);
      // spdlog::warn("The following redundant constraint pdfs will be removed");
      // disConstraints.Print();
    }

    // Adding additional constraint pdf
    for (std::map<TString, std::pair<TString, TString>>::iterator it = m_constraintPdf.begin(); it != m_constraintPdf.end(); ++it)
    {
      RooAbsPdf *pdf_tmp = w->pdf((it->first));
      if (!pdf_tmp)
        auxUtil::alertAndAbort("PDF " + it->first + " does not exist in the new workspace");
      // Check whether the current category depends on the constraint pdf
      std::vector<TString> NPList = auxUtil::splitString(it->second.first, ',');
      for (auto NPName : NPList)
      {
        if (auxUtil::checkVarExist(w, NPName, pdfi))
        {
          baseComponents.add(*pdf_tmp);
          break;
        }
      }
    }
    TString newPdfName = TString(pdfi->GetName()) + "__addConstr";

    pdfArr[icat].reset(new RooProdPdf(newPdfName, newPdfName, baseComponents));
    pdfMap[catName.Data()] = pdfArr[icat].get();
  }
  RooSimultaneous newPdf(pdfName, pdfName + "__addConstr", pdfMap, *cat);
  m_nW->import(newPdf);
}