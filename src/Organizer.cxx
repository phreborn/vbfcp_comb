/*
 * =====================================================================================
 * *       Filename:  Organizer.cxx
 *
 *    Description:  Orgnize the workspace
 *
 *        Version:  1.0
 *        Created:  07/19/12 17:33:20
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji (那你是真的牛逼), haoshuang.ji@cern.ch 
 *   Organization:  University of Wisconsin
 *
 * Rewritten by Hongtao Yang (Hongtao.Yang@cern.ch) in 2019.
 *
 * =====================================================================================
 */
#include "Organizer.h"
#include "combiner.h"

const TString Organizer::CONSTRAINT = "constraint";
const TString Organizer::NORMAL = "normal";
const TString Organizer::EXTERNAL = "external";

Organizer::Organizer(TString configFileName){
  // cout<<__PRETTY_FUNCTION__<<endl;
  _asimovHandler.reset(new asimovUtil());
  readConfigXml(configFileName);
}

bool Organizer::run(){
  // cout<<__PRETTY_FUNCTION__<<endl;
  TFile* f = TFile::Open(m_inFile);
  RooWorkspace* w = (RooWorkspace*)(f->Get(m_wsName));
  RooStats::ModelConfig* mc = (RooStats::ModelConfig*)(w->obj(m_mcName));
  RooSimultaneous* pdf = dynamic_cast<RooSimultaneous*>(mc->GetPdf());

  /* make new workspace to hold them */
  RooWorkspace* nW = new RooWorkspace( m_wsName );
  ModelConfig* nMc = new ModelConfig( m_mcName, nW );

  // Implement objects
  int nItems = (int)m_actionItems.size();
  for ( int i= 0; i < nItems; i++ ) {
    const TString actionStr = m_actionItems[i];
    const TString actionType = m_actionTypes[i];
    
    cout << "\tItem " << i << " (" << actionType << "), " << actionStr << endl;

    TString objName = auxUtil::getObjName( actionStr );
    if(actionStr.Contains("FlexibleInterpVar")) implementFlexibleInterpVar(nW, actionStr);
    else if(actionStr.Contains("RooMultiVarGaussian")) implementMultiVarGaussian(nW, actionStr);
    else auxUtil::implementObj(nW, actionStr);
  }

  // If there are additional constraint pdf to be attached
  if (m_constraintPdf.size() > 0) {
    cout<<"Attaching additional constraint terms"<<endl;
    RooAbsCategoryLValue*  m_cat = (RooAbsCategoryLValue*)&pdf->indexCat();
    int numChannels = m_cat->numBins(0);

    map<string, RooAbsPdf*> pdfMap;

    for ( int i= 0; i < numChannels; i++ ) {
      m_cat->setBin(i);
      cout<<"Creating new PDFs for category "<<m_cat->getLabel()<<endl;
      RooAbsPdf* pdfi = dynamic_cast<RooAbsPdf*>(pdf->getPdf(m_cat->getLabel()));
      RooArgSet baseComponents;
      if(typeid( *pdfi) == typeid(RooProdPdf)){
	combiner::getBasePdf((RooProdPdf*)pdfi, baseComponents);
	for (map<TString, pair<TString, TString> >::iterator it = m_constraintPdf.begin(); it != m_constraintPdf.end(); ++it){
	  RooAbsPdf* pdf_tmp = nW->pdf((it->first));
	  if(not pdf_tmp) auxUtil::alertAndAbort("PDF "+it->first+" does not exist in the new workspace");
	  baseComponents.add(*pdf_tmp);
	}
      }
      else{
	baseComponents.add(*pdfi);
      }
      TString newPdfName = TString(pdfi->GetName())+"__addConstr";
      pdfi = new RooProdPdf(newPdfName, newPdfName, baseComponents);
      
      pdfMap[m_cat->getLabel()] = pdfi;
    }

    RooSimultaneous* newPdf = new RooSimultaneous(
						  (TString(pdf->GetName())+"__addConstr"),
						  (TString(pdf->GetName())+"__addConstr"),
						  pdfMap,
						  *m_cat
						  );

    mc->SetPdf(*newPdf);
  }

  // Read rename map
  map<string, string> renameMap;
  // "EDIT::NEWPDF(OLDPDF,mu=invBRsum)"
  // cout << "Rename action: " << m_mapStr << endl;
  Ssiz_t firstC = m_mapStr.First(",") + 1;
  Ssiz_t firstR = m_mapStr.First(")");
  TString renameStr = m_mapStr(firstC, firstR-firstC);
  renameStr = renameStr.Strip(TString::kTrailing, ',');
  // cout << "Rename action: " << renameStr << endl;
  
  vector<TString> renameList = auxUtil::splitString(renameStr, ',');
  for ( auto iStr : renameList ) {
    if(!iStr.Contains("=")) auxUtil::alertAndAbort("Renaming syntax "+iStr+" not valid");

    Ssiz_t firstE = iStr.First("=");
    Ssiz_t len = iStr.Length();
    TString oldObjName = iStr(0, firstE);
    TString newObjName = iStr(firstE+1, len);
    if( not w->arg(oldObjName) ){
      if(m_isStrict) auxUtil::alertAndAbort("Object "+oldObjName+" (-> "+newObjName+") does not exist in the old workspace"); // If it does not exist in the old workspace, that's probably okay
      else{
	auxUtil::alert("Object "+oldObjName+" does not exist in the old workspace, skipping..."); // If it does not exist in the old workspace, that's probably okay
	continue;
      }
    }
    if( not nW->arg(newObjName) ) auxUtil::alertAndAbort("Object "+newObjName+" (<- "+oldObjName+") does not exist in the new workspace"); // If it is missing in the new workspace, it is not acceptable
    if(renameMap.find(oldObjName.Data()) != renameMap.end()) auxUtil::alertAndAbort("Object "+oldObjName+" is renamed more than once"); // Same variable renamed multiple times
    renameMap[oldObjName.Data()] = newObjName.Data();
  }

  string oldStr = "";
  string newStr = "";
  combiner::linkMap( renameMap, oldStr, newStr, "," );
  cout << "\told: " << oldStr << endl;
  cout << "\tnew: " << newStr << endl;

  /* import pdf */
  nW->import(*(mc->GetPdf()),
             RooFit::RenameVariable( oldStr.c_str(), newStr.c_str() ),
             RooFit::RecycleConflictNodes(), RooFit::Silence()
            );
  RooAbsPdf* nPdf = nW->pdf((mc->GetPdf())->GetName());
  nMc->SetPdf(*nPdf);

  /* import data */
  list<RooAbsData*> dataList = w->allData();
  for (list<RooAbsData*>::iterator it = dataList.begin(); it !=  dataList.end(); ++it) nW->import(**it, RooFit::RenameVariable( oldStr.c_str(), newStr.c_str() ));

  /* poi */
  RooArgSet newPOI;
  cout<<endl<<"List of POIs in the new parameterization: ";
  for ( int i= 0; i < (int)m_poiNames.size(); i++ ) {
    RooRealVar* var = nW->var(m_poiNames[i]);
    if(!var){
      if(m_isStrict) auxUtil::alertAndAbort("POI "+m_poiNames[i]+" does not exist");
      else{
	auxUtil::alert("POI "+m_poiNames[i]+" does not exist, skipping...");
	continue;
      }
    }
    cout<<m_poiNames[i]<<", ";
    /* float by default */
    var->setConstant(false);
    newPOI.add(*var);
  }
  cout<<endl<<endl;
  nMc->SetParametersOfInterest(newPOI);

  // Set nuisance parameters, global observables, and observables
  RooArgSet nuis;
  RooArgSet gobs;
  RooArgSet mobs;
  if(mc->GetNuisanceParameters()){
    unique_ptr<TIterator> iterN(mc->GetNuisanceParameters()->createIterator());
    for ( RooRealVar* v = (RooRealVar*)iterN->Next(); v!=0; v = (RooRealVar*)iterN->Next() ) {
      RooRealVar* var = nW->var(v->GetName());
      if(!(bool)var){
	auxUtil::alert(Form("old nuisance parameter %s no longer exists in the new workspace...", v->GetName()));
	continue;
      }
      if( var->isConstant() ){
	auxUtil::alert(Form("nuisance parameter %s is set to constant. Floating the parameter...", v->GetName()));
	var->setConstant(false);
      }
      nuis.add(*var);
    }
  }

  if(mc->GetGlobalObservables()){
    unique_ptr<TIterator> iterG(mc->GetGlobalObservables()->createIterator());
    for ( RooRealVar* v = (RooRealVar*)iterG->Next(); v!=0; v = (RooRealVar*)iterG->Next() ) {
      RooRealVar* var = nW->var(v->GetName());
      if(!(bool)var){
	auxUtil::alert(Form("old global observable %s no longer exists in the new workspace...", v->GetName()));
	continue;
      }
      if( !var->isConstant() ){
	auxUtil::alert(Form("global observable %s is not set to constant. Fixing the parameter...", v->GetName()));
	var->setConstant(true);
      }
      gobs.add(*var);
    }
  }

  if(mc->GetObservables()){
    unique_ptr<TIterator> iterM(mc->GetObservables()->createIterator());
    for ( RooAbsArg* v = (RooAbsArg*)iterM->Next(); v!=0; v = (RooAbsArg*)iterM->Next() ) {
      RooAbsArg* var = dynamic_cast<RooAbsArg*>(nW->obj(v->GetName()));
      if(!var){
	auxUtil::alertAndAbort(Form("old observable %s no longer exists in the new workspace", v->GetName())); // Renaming observable is currently not supported
      }
      mobs.add(*var);
    }
  }
  
  // Adding additional nuisance parameters and global observables
  for (map<TString, pair<TString, TString> >::iterator it = m_constraintPdf.begin(); it != m_constraintPdf.end(); ++it){
    pair<TString, TString> NPGO = it->second;
    vector<TString> NPList = auxUtil::splitString(NPGO.first, ',');
    for(auto NPName : NPList){
      RooRealVar *np = nW->var(NPName);
      if( not np )  auxUtil::alertAndAbort("nuisance parameter " + NPName + " does not exist in the new workspace");
      nuis.add(*np);
    }

    vector<TString> GOList = auxUtil::splitString(NPGO.second, ',');
    for(auto GOName : GOList){
      RooRealVar *go = nW->var(GOName);
      if( not go )  auxUtil::alertAndAbort("global observable " + GOName + " does not exist in the new workspace");
      gobs.add(*go);
    }
  }

  nMc->SetNuisanceParameters( nuis );
  nMc->SetGlobalObservables( gobs );
  nMc->SetObservables( mobs );

  nW->import(*nMc);
  nW->importClassCode(); 

  // Copy snapshots
  RooArgSet everything_new, everything_old;
  auxUtil::collectEverything(nMc, &everything_new);
  RooArgSet *everything_new_snapshot = dynamic_cast<RooArgSet*>(everything_new.snapshot());
  auxUtil::collectEverything(mc, &everything_old);
  
  // Nuisance parameter snapshot
  for(auto snapshot : m_snapshotNP){
    if( not w->loadSnapshot(snapshot) ) auxUtil::alert("nuisance parameter snapshot "+snapshot+" does not exist in the old workspace");
    nuis=*mc->GetNuisanceParameters();
    nW->saveSnapshot(snapshot, nuis);
  }

  // Global observable snapshot
  for(auto snapshot : m_snapshotGO){
    if( not w->loadSnapshot(snapshot) ) auxUtil::alert("global observable snapshot "+snapshot+" does not exist in the old workspace");
    gobs=*mc->GetGlobalObservables();
    nW->saveSnapshot(snapshot, gobs);
  }

  // POI snapshot
  for(auto snapshot : m_snapshotPOI){
    if( not w->loadSnapshot(snapshot) ) auxUtil::alert("POI snapshot "+snapshot+" does not exist in the old workspace");
    newPOI=*mc->GetParametersOfInterest();
    nW->saveSnapshot(snapshot, newPOI);
  }

  // Everything snapshot
  for(auto snapshot : m_snapshotAll){
    if( not w->loadSnapshot(snapshot) ) auxUtil::alert("Everything snapshot "+snapshot+" does not exist in the old workspace");
    
    everything_new=everything_old;
    nW->saveSnapshot(snapshot, everything_new);
  }

  everything_new = *everything_new_snapshot; // Recover values
  
  // Performing fits
  if(_asimovHandler->genAsimov()) _asimovHandler->generateAsimov( nMc, m_dsName );
  
  unique_ptr<TFile> fout(TFile::Open(m_outFile, "recreate"));
  nW->Write();
  fout->Close();
  cout << "Written to file: " << m_outFile << endl;

  return true;
}

void Organizer::readConfigXml( TString filen )
{
  // cout<<__PRETTY_FUNCTION__<<endl;
  TDOMParser xmlparser;
  auxUtil::parseXMLFile(&xmlparser, filen);
  // cout<<"XML file parsed"<<endl;
  
  TXMLDocument* xmldoc = xmlparser.GetXMLDocument();
  TXMLNode* rootNode = xmldoc->GetRootNode();
  TListIter rootAttIt = rootNode->GetAttributes();
  TXMLNode* node = rootNode->GetChildren();

  m_inFile = auxUtil::getAttributeValue(rootNode, "InFile");
  m_outFile = auxUtil::getAttributeValue(rootNode, "OutFile");
  m_modelName = auxUtil::getAttributeValue(rootNode, "ModelName");
  m_wsName = auxUtil::getAttributeValue(rootNode, "WorkspaceName", true, "combWS");
  m_mcName = auxUtil::getAttributeValue(rootNode, "ModelConfigName", true, "ModelConfig");
  m_dsName = auxUtil::getAttributeValue(rootNode, "DataName", true, "combData");
  m_poiNames = auxUtil::splitString(auxUtil::getAttributeValue(rootNode, "POINames"), ',');
  m_snapshotNP = auxUtil::splitString(auxUtil::getAttributeValue(rootNode, "SnapshotNP", true, ""), ','); // NP snapshots to be copied
  m_snapshotGO = auxUtil::splitString(auxUtil::getAttributeValue(rootNode, "SnapshotGO", true, ""), ','); // GO snapshots to be copied
  m_snapshotPOI = auxUtil::splitString(auxUtil::getAttributeValue(rootNode, "SnapshotPOI", true, ""), ','); // POI snapshots to be copied
  m_snapshotAll = auxUtil::splitString(auxUtil::getAttributeValue(rootNode, "SnapshotAll", true, ""), ','); // Everything snapshots to be copied
  m_isStrict = auxUtil::to_bool(auxUtil::getAttributeValue(rootNode, "Strict", true, "true")); // Strict mode
  // cout<<"Root node read"<<endl;
  
  /* root node children */
  while ( node != 0 ){
    const TString nodeName=node->GetNodeName();
    if ( nodeName == "Item" ){
      m_actionItems.push_back(auxUtil::getAttributeValue(node, "Name"));
      m_actionTypes.push_back(auxUtil::getAttributeValue(node, "Type", true, NORMAL)); // By default it is normal type
      if( m_actionTypes.back() == CONSTRAINT ){
	TString constrPdfName = auxUtil::getObjName(m_actionItems.back());
	TString NPName = auxUtil::getAttributeValue(node, "NP");
	TString GOName = auxUtil::getAttributeValue(node, "GO");
	m_constraintPdf[constrPdfName] = make_pair(NPName, GOName);
      }
    }
    else if ( nodeName == "Map" ){
      m_mapStr = auxUtil::getAttributeValue(node, "Name");
    }
    else if ( nodeName == "Asimov" ){
      // cout<<"Filling Asimov handler"<<endl;
      _asimovHandler->addEntry(node);
    }
    node = node->GetNextNode();
  }
}

void Organizer::implementFlexibleInterpVar(RooWorkspace *w, TString actionStr){
  actionStr=actionStr.ReplaceAll(" ","");
  actionStr=actionStr.ReplaceAll("FlexibleInterpVar::",  "");

  TString responseName=actionStr;
  responseName=responseName(0, actionStr.First("("));

  Size_t begin=actionStr.First("(")+1;
  Size_t end=actionStr.First(")");
  actionStr=actionStr(begin, end-begin);
  TObjArray* iArray=actionStr.Tokenize(",");
  int iNum=iArray->GetEntries();
  if(iNum!=5) auxUtil::alertAndAbort("the format of FlexibleInterpVar ("+actionStr+") is not correct");

  TString NPName=((TObjString*)iArray->At(0))->GetString();
  if(!w->var(NPName)){
    // cerr<<"ERROR: nuisance parameter "<<NPName<<" is missing."<<endl;
    // abort();
  }
  
  RooArgList nuiList(*w->arg(NPName));

  double nominal=atof(((TObjString*)iArray->At(1))->GetString());
  double errHi=atof(((TObjString*)iArray->At(2))->GetString());
  double errLo=atof(((TObjString*)iArray->At(3))->GetString());
  int interpolation=atoi(((TObjString*)iArray->At(4))->GetString());

  vector<double> sigma_var_low, sigma_var_high;
  vector<int> code;

  sigma_var_low.push_back(nominal+errLo);
  sigma_var_high.push_back(nominal+errHi);
  code.push_back(interpolation);
  
  RooStats::HistFactory::FlexibleInterpVar expected_var(responseName,responseName,nuiList,nominal,sigma_var_low,sigma_var_high,code);

  // expected_var.Print();
  w->import(expected_var);
}

void Organizer::implementMultiVarGaussian(RooWorkspace *w, TString actionStr){
  actionStr=actionStr.ReplaceAll(" ","");
  actionStr=actionStr.ReplaceAll("RooMultiVarGaussian::",  "");

  TString functionName=actionStr;
  functionName=functionName(0, actionStr.First("("));

  Size_t begin=actionStr.First("(")+1;
  Size_t end=actionStr.First(")");
  actionStr=actionStr(begin, end-begin);
  TObjArray* iArray=actionStr.Tokenize(":");
  int iNum=iArray->GetEntries();
  if(iNum!=4) auxUtil::alertAndAbort("the format of RooMultiVarGaussian ("+actionStr+") is not correct");

  RooArgList obsList, meanList;
  // Get inputs
  TString obsListStr=((TObjString*)iArray->At(0))->GetString();
  obsListStr=obsListStr(obsListStr.First('{')+1,obsListStr.First('}')-1);
  vector<TString> obsListVec=auxUtil::splitString(obsListStr,',');
  
  TString meanListStr=((TObjString*)iArray->At(1))->GetString();
  meanListStr=meanListStr(meanListStr.First('{')+1,meanListStr.First('}')-1);
  vector<TString> meanListVec=auxUtil::splitString(meanListStr,',');

  TString uncertListStr=((TObjString*)iArray->At(2))->GetString();
  uncertListStr=uncertListStr(uncertListStr.First('{')+1,uncertListStr.First('}')-1);
  vector<TString> uncertListVec=auxUtil::splitString(uncertListStr,',');

  TString correlationListStr=((TObjString*)iArray->At(3))->GetString();
  correlationListStr=correlationListStr(correlationListStr.First('{')+1,correlationListStr.First('}')-1);
  vector<TString> correlationListVec=auxUtil::splitString(correlationListStr,',');

  const int nPOI=obsListVec.size();
  for(int i=0; i<nPOI; i++){
    TString obsName=obsListVec[i];
    TString meanName=meanListVec[i];
    if(!w->arg(obsName)) auxUtil::alertAndAbort("variable "+obsName+" does not exist in workspace");

    obsList.add(*w->arg(obsName));

    if(!w->arg(meanName)) auxUtil::alertAndAbort("variable "+meanName+" does not exist in workspace");

    meanList.add(*w->arg(meanName));
  }

  TMatrixDSym V(nPOI);
  int corIdx=0;
  for (int i=0 ; i<nPOI ; i++) {
    for (int j=0 ; j<nPOI ; j++) {
      if(i==j){
	V(i,j) = uncertListVec[i].Atof()*uncertListVec[j].Atof();
      }
      else if (i<j){
	// Problematic: ad-hoc implementaiton for the time-being
	if(corIdx>=nPOI){
	  cerr<<"Number of index larger than array size!"<<endl;
	  abort();
	}
	V(i,j) = correlationListVec[corIdx].Atof()*uncertListVec[i].Atof()*uncertListVec[j].Atof();
	V(j,i) = V(i,j);
	corIdx++;
      }
    }
  }

  V.Print();
  RooMultiVarGaussian *model=new RooMultiVarGaussian(functionName,functionName, obsList, meanList, V) ;  
  w->import(*model);
}
