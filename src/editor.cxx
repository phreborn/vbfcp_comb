/*
 * =====================================================================================
 * *       Filename:  editor.cxx
 *
 *    Description:  Edit workspace
 *
 *        Version:  1.1
 *        Created:  07/19/12 17:33:20
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
#include "editor.h"
#include "textTable.h"

const TString editor::CONSTRAINT = "constraint";
const TString editor::NORMAL = "normal";
const TString editor::EXTERNAL = "external";

editor::editor(TString configFileName)
{
  m_numThreads = 1;
  _asimovHandler.reset(new asimovUtil());
  readConfigXml(configFileName);
}

bool editor::run()
{
  unique_ptr<TFile> f(TFile::Open(m_inFile));
  RooWorkspace *w = auxUtil::getWorkspaceFromFile(f.get(), m_wsName);
  RooStats::ModelConfig *mc = auxUtil::getModelConfigFromWorkspace(w, m_mcName);
  m_pdf = dynamic_cast<RooSimultaneous *>(mc->GetPdf());

  // Implement objects
  int nItems = (int)m_actionItems.size();
  for (int i = 0; i < nItems; i++)
  {
    const TString actionStr = m_actionItems[i];
    const TString actionType = m_actionTypes[i];

    spdlog::info("Item {} ({}), {}", i, actionType.Data(), actionStr.Data());

    TString objName = auxUtil::getObjName(actionStr);
    if (actionStr.Contains("FlexibleInterpVar"))
      implementFlexibleInterpVar(m_nW.get(), actionStr);
    else if (actionStr.Contains("RooMultiVarGaussian"))
      implementMultiVarGaussian(m_nW.get(), actionStr);
    else
      auxUtil::implementObj(m_nW.get(), actionStr);
  }

  // If there are additional constraint pdf to be attached
  if (m_constraintPdf.size() > 0)
  {
    spdlog::info("Attaching additional constraint terms");
    m_cat = (RooCategory *)(&m_pdf->indexCat());
    m_total = m_cat->numBins(0);
    m_idx = 0; /* Reset counter */
    m_thread_ptrs.clear();
    for (unsigned i = 0; i < std::min(m_numThreads, m_total); i++)
    {
        m_thread_ptrs.emplace_back(new std::thread(&editor::remakeCategories, this));
        spdlog::info("  -> Processor thread #{} started!", i);
    }
    join();

    RooSimultaneous *newPdf = new RooSimultaneous(
        (TString(m_pdf->GetName()) + "__addConstr"),
        (TString(m_pdf->GetName()) + "__addConstr"),
        m_pdfMap,
        *m_cat);

    mc->SetPdf(*newPdf);
  }

  // Read rename map
  map<TString, TString> renameMap;
  // "EDIT::NEWPDF(OLDPDF,mu=invBRsum)"
  // cout << "Rename action: " << m_mapStr << endl;
  Ssiz_t firstC = m_mapStr.First(",") + 1;
  Ssiz_t firstR = m_mapStr.First(")");
  TString renameStr = m_mapStr(firstC, firstR - firstC);
  renameStr = renameStr.Strip(TString::kTrailing, ',');
  // cout << "Rename action: " << renameStr << endl;

  vector<TString> renameList = auxUtil::splitString(renameStr, ',');
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
    if (not w->arg(oldObjName))
    {
      if (m_isStrict)
        auxUtil::alertAndAbort("Object " + oldObjName + " (-> " + newObjName + ") does not exist in the old workspace"); // If it does not exist in the old workspace, that's probably okay
      else
      {
        spdlog::warn("Object {} does not exist in the old workspace, skipping...", oldObjName.Data()); // If it does not exist in the old workspace, that's probably okay
        continue;
      }
    }
    if (!m_nW->arg(newObjName))
      auxUtil::alertAndAbort("Object " + newObjName + " (<- " + oldObjName + ") does not exist in the new workspace"); // If it is missing in the new workspace, it is not acceptable
    if (renameMap.find(oldObjName.Data()) != renameMap.end())
      auxUtil::alertAndAbort("Object " + oldObjName + " is renamed more than once"); // Same variable renamed multiple times
    renameMap[oldObjName.Data()] = newObjName.Data();
    t.add(oldObjName.Data());
    t.add(newObjName.Data());
    t.endOfRow();
  }
  spdlog::info("Replace table:");
  cout << t;

  TString oldStr = "";
  TString newStr = "";
  auxUtil::linkMap(renameMap, oldStr, newStr, ",");

  /* import pdf */
  m_nW->import(*(mc->GetPdf()),
             RooFit::RenameVariable(oldStr, newStr),
             RooFit::RecycleConflictNodes(), RooFit::Silence());
  RooAbsPdf *nPdf = m_nW->pdf((mc->GetPdf())->GetName());
  m_nMc->SetPdf(*nPdf);

  /* import data */
  list<RooAbsData *> dataList = w->allData();
  for (list<RooAbsData *>::iterator it = dataList.begin(); it != dataList.end(); ++it)
    m_nW->import(**it);

  /* poi */
  RooArgSet newPOI;
  spdlog::debug("List of POIs in the new parameterization:");
  for (int i = 0; i < (int)m_poiNames.size(); i++)
  {
    RooRealVar *var = m_nW->var(m_poiNames[i]);
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
  if (mc->GetNuisanceParameters())
  {
    unique_ptr<TIterator> iterN(mc->GetNuisanceParameters()->createIterator());
    for (RooRealVar *v = (RooRealVar *)iterN->Next(); v != 0; v = (RooRealVar *)iterN->Next())
    {
      RooRealVar *var = m_nW->var(v->GetName());
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

  if (mc->GetGlobalObservables())
  {
    unique_ptr<TIterator> iterG(mc->GetGlobalObservables()->createIterator());
    for (RooRealVar *v = (RooRealVar *)iterG->Next(); v != 0; v = (RooRealVar *)iterG->Next())
    {
      RooRealVar *var = m_nW->var(v->GetName());
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

  if (mc->GetObservables())
  {
    unique_ptr<TIterator> iterM(mc->GetObservables()->createIterator());
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
  for (map<TString, pair<TString, TString>>::iterator it = m_constraintPdf.begin(); it != m_constraintPdf.end(); ++it)
  {
    pair<TString, TString> NPGO = it->second;
    vector<TString> NPList = auxUtil::splitString(NPGO.first, ',');
    for (auto NPName : NPList)
    {
      RooRealVar *np = m_nW->var(NPName);
      if (not np)
        auxUtil::alertAndAbort("Nuisance parameter " + NPName + " does not exist in the new workspace");
      nuis.add(*np);
    }

    vector<TString> GOList = auxUtil::splitString(NPGO.second, ',');
    for (auto GOName : GOList)
    {
      RooRealVar *go = m_nW->var(GOName);
      if (not go)
        auxUtil::alertAndAbort("Global observable " + GOName + " does not exist in the new workspace");
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
  auxUtil::collectEverything(mc, &everything_old);

  // Nuisance parameter snapshot
  for (auto snapshot : m_snapshotNP)
  {
    if (not w->loadSnapshot(snapshot))
      spdlog::warn("nuisance parameter snapshot {} does not exist in the old workspace", snapshot.Data());
    nuis = *mc->GetNuisanceParameters();
    m_nW->saveSnapshot(snapshot, nuis);
  }

  // Global observable snapshot
  for (auto snapshot : m_snapshotGO)
  {
    if (not w->loadSnapshot(snapshot))
      spdlog::warn("global observable snapshot {} does not exist in the old workspace", snapshot.Data());
    gobs = *mc->GetGlobalObservables();
    m_nW->saveSnapshot(snapshot, gobs);
  }

  // POI snapshot
  for (auto snapshot : m_snapshotPOI)
  {
    if (not w->loadSnapshot(snapshot))
      spdlog::warn("POI snapshot {} does not exist in the old workspace", snapshot.Data());
    newPOI = *mc->GetParametersOfInterest();
    m_nW->saveSnapshot(snapshot, newPOI);
  }

  // Everything snapshot
  for (auto snapshot : m_snapshotAll)
  {
    if (not w->loadSnapshot(snapshot))
      spdlog::warn("Everything snapshot {} does not exist in the old workspace", snapshot.Data());

    everything_new = everything_old;
    m_nW->saveSnapshot(snapshot, everything_new);
  }

  everything_new = *everything_new_snapshot; // Recover values

  // Performing fits
  if (_asimovHandler->genAsimov())
    _asimovHandler->generateAsimov(m_nMc.get(), m_dsName);

  unique_ptr<TFile> fout(TFile::Open(m_outFile, "recreate"));
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

  // make new workspace at the beginning
  m_nW.reset(new RooWorkspace(m_wsName));
  m_nMc.reset(new ModelConfig(m_mcName, m_nW.get()));
  
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
          unique_ptr<TFile> fTemp(TFile::Open(fileName));
          // Workspace name is mandatory
          TString wsTempName = auxUtil::getAttributeValue(node, "WorkspaceName");
          RooWorkspace *wsTemp = auxUtil::getWorkspaceFromFile(fTemp.get(), wsTempName);
          RooAbsPdf *pdfTemp = auxUtil::getPdfFromWorkspace(wsTemp, constrPdfName);
          m_nW->import(*pdfTemp);
          fTemp->Close();
        }
        m_constraintPdf[constrPdfName] = make_pair(NPName, GOName);
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

  vector<double> sigma_var_low, sigma_var_high;
  vector<int> code;

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
  vector<TString> obsListVec = auxUtil::splitString(obsListStr, ',');

  TString meanListStr = ((TObjString *)iArray->At(1))->GetString();
  meanListStr = meanListStr(meanListStr.First('{') + 1, meanListStr.First('}') - 1);
  vector<TString> meanListVec = auxUtil::splitString(meanListStr, ',');

  TString uncertListStr = ((TObjString *)iArray->At(2))->GetString();
  uncertListStr = uncertListStr(uncertListStr.First('{') + 1, uncertListStr.First('}') - 1);
  vector<TString> uncertListVec = auxUtil::splitString(uncertListStr, ',');

  TString correlationListStr = ((TObjString *)iArray->At(3))->GetString();
  correlationListStr = correlationListStr(correlationListStr.First('{') + 1, correlationListStr.First('}') - 1);
  vector<TString> correlationListVec = auxUtil::splitString(correlationListStr, ',');

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
          cerr << "Number of index larger than array size!" << endl;
          abort();
        }
        V(i, j) = correlationListVec[corIdx].Atof() * uncertListVec[i].Atof() * uncertListVec[j].Atof();
        V(j, i) = V(i, j);
        corIdx++;
      }
    }
  }

  V.Print();
  RooMultiVarGaussian *model = new RooMultiVarGaussian(functionName, functionName, obsList, meanList, V);
  w->import(*model);
}

void editor::remakeCategories()
{
  std::unique_lock<std::mutex> lk(m_mutex, std::defer_lock);
  while(m_idx < m_total)
  {
    int icat = m_idx++;
    if (m_idx > m_total)
      break;
    lk.lock();
    m_cat->setBin(icat);
    TString catName = m_cat->getLabel();
    lk.unlock();

    spdlog::info("Creating new PDF for category {}", catName.Data());
    RooAbsPdf *pdfi = dynamic_cast<RooAbsPdf *>(m_pdf->getPdf(catName));
    RooArgSet baseComponents;
    if (typeid(*pdfi) == typeid(RooProdPdf))
    {
      auxUtil::getBasePdf((RooProdPdf *)pdfi, baseComponents);
      for (map<TString, pair<TString, TString>>::iterator it = m_constraintPdf.begin(); it != m_constraintPdf.end(); ++it)
      {
        RooAbsPdf *pdf_tmp = m_nW->pdf((it->first));
        if (!pdf_tmp)
          auxUtil::alertAndAbort("PDF " + it->first + " does not exist in the new workspace");
        baseComponents.add(*pdf_tmp);
      }
    }
    else
    {
      baseComponents.add(*pdfi);
    }
    TString newPdfName = TString(pdfi->GetName()) + "__addConstr";

    lk.lock();
    pdfi = new RooProdPdf(newPdfName, newPdfName, baseComponents);    
    m_pdfMap[catName.Data()] = pdfi;
    m_keep.Add(pdfi);
    lk.unlock();
  }
}