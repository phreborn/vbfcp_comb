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
 *         Author:  Haoshuang Ji (hji), haoshuang.ji@cern.ch
 *   Organization:
 *
 * =====================================================================================
 */
#include "Organizer.h"
#include "combiner.h"

Organizer::Organizer( std::string combinedFile, std::string outFile ): AlgoBase("Organizer specific options") {
    options_.add_options();
    m_inFile = combinedFile;
    m_outFile = outFile;
}

void Organizer::applyOptions(const boost::program_options::variables_map &vm) {
}

void Organizer::applyDefaultOptions() { validateOptions(); }

void Organizer::validateOptions() {
}

bool Organizer::run(bool makeSnapshot, std::string dataName, std::string wsName, std::string mcName)
{
  TFile* f = TFile::Open(m_inFile.c_str());
  RooWorkspace* w = (RooWorkspace*)(f->Get(wsName.c_str()));
  RooStats::ModelConfig* mc = (RooStats::ModelConfig*)(w->obj(mcName.c_str()));
  RooSimultaneous* pdf = dynamic_cast<RooSimultaneous*>(mc->GetPdf());


  /* make new workspace to hold them */
  w->SetName("oldW");
  RooWorkspace* nW = new RooWorkspace("combWS");
  ModelConfig* nMc = new ModelConfig( "ModelConfig", nW );
  nMc->SetWorkspace(*nW);


  std::map<std::string, std::string> renameMap;

  std::string oldPdfName = pdf->GetName();
  std::string newPdfName = oldPdfName+"_"+m_modelName;
  bool additionalTerm(false);
  std::vector<std::string> additionalNames;
  std::vector<std::string> additionalVarNames, additionalGlobs;

  bool action(true);
  TString actionStr = "";
  int nItems = (int)m_actionItems.size();
  for ( int i= 0; i < nItems; i++ ) {
    actionStr = m_actionItems[i].c_str();
    /* get rid of SPACE */
    actionStr = actionStr.ReplaceAll(" ", "");

    if ( actionStr.Contains("NEWPDF") ) {
      actionStr.ReplaceAll("NEWPDF", newPdfName.c_str());
      // "EDIT::NEWPDF(OLDPDF,mu=invBRsum)"
      Ssiz_t firstC = actionStr.First(",") + 1;
      Ssiz_t firstR = actionStr.First(")");
      TString renameStr = actionStr(firstC, firstR);

      TObjArray* iArray = TString(renameStr.Data()).Tokenize(",");
      int iNum = iArray->GetEntries();
      TString iStr;
      for ( int i= 0; i < iNum; i++ ) {
        iStr = ((TObjString*)iArray->At(i))->GetString();
        iStr = iStr.ReplaceAll(")", "");
        // "mu=invBRsum"
        Ssiz_t firstE = iStr.First("=");
        Ssiz_t len = iStr.Length();
        TString oldVarName = iStr(0, firstE);
        TString newVarName = iStr(firstE+1, len);
        /* if the old str does not exsit, do remove this item */
        if ( !w->var(oldVarName.Data()) ) {
          actionStr.ReplaceAll(iStr+",", "");
        }
        renameMap[oldVarName.Data()] = newVarName.Data();
      }
      if ( actionStr.Contains("OLDPDF") ) {
        actionStr.ReplaceAll("OLDPDF", oldPdfName.c_str());
      }

      /* shouldn't commit this action */
      continue;
    }
    std::cout << "\tItem " << i << ", " << actionStr << std::endl;

    if(actionStr.Contains("FlexibleInterpVar")) implementFlexibleInterpVar(nW, actionStr);
    else nW->factory(actionStr);

    if (actionStr.Contains("ConstraintPdf")) {
      actionStr.ReplaceAll(" ","");
      actionStr=actionStr.ReplaceAll("RooGaussian::",  "");
      actionStr=actionStr.ReplaceAll("EXPR::",  "");

      TString pdfName  =  actionStr;

      pdfName  =  pdfName(0,  pdfName.First("("));
      additionalNames.push_back(pdfName.Data());

      std::cout << "pdf: " << pdfName << std::endl;
      
      Size_t begin = actionStr.First("(")+1;
      Size_t end = actionStr.First(",");
      TString varName  =  actionStr(begin, end-begin);
      additionalVarNames.push_back(varName.Data());
      Size_t begin_glob = actionStr.First(",")+1;
      Size_t end_glob = actionStr.Last(',');
      TString varName_glob=actionStr(begin_glob, end_glob-begin_glob);
      additionalGlobs.push_back(varName_glob.Data());
      additionalTerm  =  true;
    }
  }

  /* add additionalTerm */
  if (additionalTerm) {
    std::cout<<"Adding additional constraint terms"<<std::endl;
    RooAbsCategoryLValue*  m_cat = (RooAbsCategoryLValue*)&pdf->indexCat();
    int numChannels = m_cat->numBins(0);

    std::map<std::string, RooAbsPdf*> pdfMap;

    for ( int i= 0; i < numChannels; i++ ) {

      m_cat->setBin(i);
      std::cout<<"Creating new PDFs for category "<<m_cat->getLabel()<<std::endl;
      RooAbsPdf* pdfi = dynamic_cast<RooAbsPdf*>(pdf->getPdf(m_cat->getLabel()));
      RooArgSet baseComponents;
      if(typeid( *pdfi) == typeid(RooProdPdf)){

	combiner::getBasePdf((RooProdPdf*)pdfi, baseComponents);
	//std::cout<<"PDF decomposed."<<std::endl;
	/* insert additional constraints */
	for (std::vector<std::string>::iterator it  =
	       additionalNames.begin(); it !=  additionalNames.end(); ++it)
	  {
	    RooAbsPdf* pdf_tmp  =  nW->pdf((*it).c_str());
	    assert(pdf_tmp);
	    baseComponents.add(*pdf_tmp);
	  }

      }
      else{
	baseComponents.add(*pdfi);
      }
      std::string newPdfName = std::string(pdfi->GetName())+"_coupling";
      pdfi = new RooProdPdf(newPdfName.c_str(), newPdfName.c_str(), baseComponents);
      
      pdfMap[m_cat->getLabel()] = pdfi;
    }

    RooSimultaneous* newPdf = new RooSimultaneous(
        (std::string(pdf->GetName())+"_coupling").c_str(),
        (std::string(pdf->GetName())+"_coupling").c_str(),
        pdfMap,
        *m_cat
        );

    mc->SetPdf(*newPdf);
  }

  if(m_modelName.find("NNuis")!=std::string::npos){
    std::unique_ptr<TIterator> iter(mc->GetNuisanceParameters()->createIterator());
    while ( RooRealVar* v = (RooRealVar*)iter->Next() ) {
      // RooRealVar* var = w->var("ATLAS_SF_HWW_WW0j_WW_2012");
      bool isGauPoi = false;
      std::string name = v->GetName();
      if ( name.find("gamma_stat_")!=std::string::npos ) {
        isGauPoi = true;
      }
      else
      {
        std::unique_ptr<TIterator> iter1(v->clientIterator());
        while ( RooAbsArg* v1 = (RooAbsArg*)iter1->Next() ) {
          std::string className = v1->ClassName();
          if ( className=="RooGaussian" ) {
            isGauPoi = true;
            break;
          }
        }
      }

      if ( isGauPoi ) {
        nW->factory(TString::Format("%s[%.2f]",v->GetName(),v->getVal()).Data());
        std::cout << "\tmade: " << TString::Format("%s[%.2f]",v->GetName(),v->getVal())<< std::endl;
      }
    }
  }

  std::string oldStr = "";
  std::string newStr = "";
  combiner::linkMap( renameMap, oldStr, newStr, "," );
  std::cout << "\told: " << oldStr << std::endl;
  std::cout << "\tnew: " << newStr << std::endl;

  // check the naming
  for( TString oldVar : SplitString(oldStr, ',') ) {
    if (not w->arg(oldVar))  {
      cout << "\033[91m WARNING: Object " << oldVar << " not in workspace. \033[0m" << endl;
      //return false;
    }
  }

  for( TString newVar : SplitString(newStr, ',') ) {
    if (not nW->arg(newVar))  {
      cout << "\033[91m WARNING: Object " << newVar << " not in workspace. \033[0m" << endl;
      //return false;
    }
  }

  /* import pdf */
  nW->import(*(mc->GetPdf()),
             RooFit::RenameVariable( oldStr.c_str(), newStr.c_str() ),
             RooFit::RecycleConflictNodes(), RooFit::Silence()
            );
  RooAbsPdf* nPdf = nW->pdf((mc->GetPdf())->GetName());
  nMc->SetPdf(*nPdf);

  /* import data */
  const RooArgSet* nuis1 = mc->GetNuisanceParameters();
  const RooArgSet* gobs1 = mc->GetGlobalObservables();
  std::list<RooAbsData*> dataList = w->allData();
  for (std::list<RooAbsData*>::iterator it  =
       dataList.begin(); it !=  dataList.end(); ++it)
  {
    nW->import(**it,
               RooFit::RenameVariable( oldStr.c_str(), newStr.c_str() )
              );
    std::string dataName = (*it)->GetName();
    if ( dataName=="asimovData_0" ) {
      if ( !( w->loadSnapshot("conditionalGlobs_0") && w->loadSnapshot("conditionalNuis_0") ) ) {
        continue;
      }
      nW->saveSnapshot("conditionalNuis_0", *nuis1, true);
      nW->saveSnapshot("conditionalGlobs_0", *gobs1, true);
    }
    else if ( dataName=="asimovData_1" ) {
      continue;
      assert ( w->loadSnapshot("nominalNuis") );
      assert ( w->loadSnapshot("nominalGlobs") );

      nW->saveSnapshot("nominalGlobs", *gobs1, true);
      nW->saveSnapshot("nominalNuis", *nuis1, true);

      assert ( w->loadSnapshot("conditionalGlobs_1") );
      assert ( w->loadSnapshot("conditionalNuis_1") );
      nW->saveSnapshot("conditionalNuis_1", *nuis1, true);
      nW->saveSnapshot("conditionalGlobs_1", *gobs1, true);
    }
    else if ( dataName=="asimovData_muhat" ) {
      assert ( w->loadSnapshot("conditionalGlobs_muhat") );
      assert ( w->loadSnapshot("conditionalNuis_muhat") );
      nW->saveSnapshot("conditionalNuis_muhat", *nuis1, true);
      nW->saveSnapshot("conditionalGlobs_muhat", *gobs1, true);

    }
  }

  /* poi */
  RooArgSet newPOI;
  std::cout<<"List of POIs in the new parameterization:"<<std::endl;
  for ( int i= 0; i < (int)m_poiNames.size(); i++ ) {
    RooRealVar* var = nW->var(m_poiNames[i].c_str());
    if(!var){
      cout << "\033[91m ERROR: POI " << m_poiNames[i] << " is missing. Please intervene...\033[0m" << endl;
      abort();
    }
    std::cout<<m_poiNames[i].c_str()<<std::endl;
    /* float by default */
    var->setConstant(false);
    newPOI.add(*var);
  }
  nMc->SetParametersOfInterest(newPOI);

  RooArgSet nuis;
  RooArgSet gobs;
  RooArgSet mobs;
  std::unique_ptr<TIterator> iterN(mc->GetNuisanceParameters()->createIterator());
  for ( RooRealVar* v = (RooRealVar*)iterN->Next(); v!=0; v = (RooRealVar*)iterN->Next() ) {
    TObject* arg = nW->obj(v->GetName());
    if(!(bool)arg){
      std::cerr<<"\tWARNING: variable "<<v->GetName()<<"  not found as an object in the workspace..."<<std::endl;
      continue;
    }
    std::string className = arg->ClassName();
    // std::cout << "???class: " << className << std::endl;
    if ( className!="RooRealVar" ) {
      std::cout << "\tWARNING: no " << v->GetName() << std::endl;
      continue;
    }
    RooRealVar* var = nW->var(v->GetName());
    if(!(bool)var){
      std::cerr<<"\tWARNING: variable "<<v->GetName()<<"  not found as a RooRealVar in the workspace..."<<std::endl;
      continue;
    }
    if ( !var->isConstant() ) { nuis.add(*var); }
  }

  std::unique_ptr<TIterator> iterG(mc->GetGlobalObservables()->createIterator());
  for ( RooRealVar* v = (RooRealVar*)iterG->Next(); v!=0; v = (RooRealVar*)iterG->Next() ) {
    // std::cout << "name: " << v->GetName() << std::endl;
    std::string vName = v->GetName();
    // if ( vName=="ATLAS_EM_ES_Z_In" || vName=="ATLAS_EM_MAT1_In" || vName=="ATLAS_EM_PS1_In" ) {
    //   continue;
    // }
    RooRealVar* var = nW->var(v->GetName());
    if(!(bool)var){
      std::cerr<<"\tWARNING: variable "<<v->GetName()<<"  not found in the global observables..."<<std::endl;
      continue;
    }
    gobs.add(*var);
  }

  std::unique_ptr<TIterator> iterM(mc->GetObservables()->createIterator());
  for ( RooAbsArg* v = (RooAbsArg*)iterM->Next(); v!=0; v = (RooAbsArg*)iterM->Next() ) {
    // std::cout << "name: " << v->GetName() << std::endl;
    RooAbsArg* var = dynamic_cast<RooAbsArg*>(nW->obj(v->GetName()));
//     assert ( var );
    if(!var){
      std::cerr<<"Variable "<<v->GetName()<<" does not exist."<<std::endl;
      continue;
    }
    mobs.add(*var);
  }

  // nuis.Print();
  for (std::vector<std::string>::iterator it  =
       additionalVarNames.begin(); it !=  additionalVarNames.end(); ++it)
  {
    RooAbsArg* arg = dynamic_cast<RooRealVar*>(nW->obj((*it).c_str()));
    if(arg){
      std::cout << "\tadding " << *it << " to Nuisance Parameters... " << std::endl;
      nuis.add(*arg);
    }
  }

  for (std::vector<std::string>::iterator it  =
       additionalGlobs.begin(); it !=  additionalGlobs.end(); ++it)
  {
    RooRealVar* arg = dynamic_cast<RooRealVar*>(nW->obj((*it).c_str()));
    if(arg){
      std::cout << "\tadding " << *it << " to Global Observables... " << std::endl;
      arg->setConstant(true);
      gobs.add(*arg);
    }
  }

  nMc->SetNuisanceParameters( nuis );

  nMc->SetGlobalObservables( gobs );
  nMc->SetObservables( mobs );

  if(makeSnapshot)
  {
    RooDataSet* data = dynamic_cast<RooDataSet*>(nW->data(dataName.c_str()));
    assert ( data );
    asimovUtils::makeAsimovDataForMultiPoi(nW, nMc, data, m_poiNames);
  }


  nW->import(*nMc);
  nW->importClassCode(); 

  std::cout << "Writing to file: " << m_outFile << std::endl;
  nW->writeToFile(m_outFile.c_str(), true);
  std::cout << "Written to file: " << m_outFile << std::endl;

  return action;
}

void Organizer::readConfigXml( std::string filen )
{
  std::cout << "Parsing file: " << filen << std::endl;
  TDOMParser xmlparser;
  // reading in the file and parse by DOM
  Int_t parseError = xmlparser.ParseFile( filen.c_str() );

  if ( parseError )
  {
    std::cout << "Loading of xml document \"" << filen
        << "\" failed" << std::endl;
  }

  TXMLDocument* xmldoc = xmlparser.GetXMLDocument();
  TXMLNode* rootNode = xmldoc->GetRootNode();
  TListIter rootAttIt = rootNode->GetAttributes();
  TXMLNode* node = rootNode->GetChildren();
  TXMLNode* thisNode = NULL;
  TXMLAttr* curAttr = NULL;

  /* root node attributes */
  while (( curAttr = dynamic_cast< TXMLAttr* >( rootAttIt() ) ) != 0 )
  {
    if ( curAttr->GetName() == TString( "InFile" ) )
    {
      if (m_inFile == "") m_inFile = curAttr->GetValue();
    }
    else if ( curAttr->GetName() == TString( "OutFile" ) )
    {
      if (m_outFile == "") m_outFile = curAttr->GetValue();
    }
    else if ( curAttr->GetName() == TString( "ModelName" ) )
    {
      m_modelName = curAttr->GetValue();
    }
    else if ( curAttr->GetName() == TString( "POINames" ) )
    {
      std::string poiName = curAttr->GetValue();
      TObjArray* iArray = TString(poiName.c_str()).Tokenize(",");
      int iNum = iArray->GetEntries();
      TString iStr;
      for ( int i= 0; i < iNum; i++ ) {
        iStr = ((TObjString*)iArray->At(i))->GetString();
        iStr.ReplaceAll(" ", "");
        // std::cout << "poi: " << iStr << std::endl;
        m_poiNames.push_back(iStr.Data());
      }
    }
  }

  /* root node children */
  while ( node != 0 )
  {
    if ( node->GetNodeName() == TString( "Item" ) )
    {
      TListIter attribIt = node->GetAttributes();

      while (( curAttr = dynamic_cast< TXMLAttr* >( attribIt() ) ) != 0 )
      {
        if ( curAttr->GetName() == TString( "Name" ) )
        {
          m_actionItems.push_back(curAttr->GetValue());
        }
      }
    }
    node = node->GetNextNode();
  }
}


std::vector<TString> Organizer::SplitString(const TString& theOpt, const char separator )
{
   // splits the option string at 'separator' and fills the list
   // 'splitV' with the primitive strings
  std::vector<TString> splitV;
  TString splitOpt(theOpt);
  splitOpt.ReplaceAll("\n"," ");
  splitOpt = splitOpt.Strip(TString::kBoth,separator);
  while (splitOpt.Length()>0) {
    if ( !splitOpt.Contains(separator) ) {
      splitV.push_back(splitOpt);
      break;
    }
    else {
      TString toSave = splitOpt(0,splitOpt.First(separator));
      splitV.push_back(toSave);
      splitOpt = splitOpt(splitOpt.First(separator),splitOpt.Length());
    }
    splitOpt = splitOpt.Strip(TString::kLeading,separator);
  }
  return splitV;
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
  if(iNum!=5){
    std::cerr<<"ERROR: the format of FlexibleInterpVar is not correct."<<std::endl;
    abort();
  }

  TString NPName=((TObjString*)iArray->At(0))->GetString();
  if(!w->var(NPName)){
    std::cerr<<"ERROR: nuisance parameter "<<NPName<<" is missing."<<std::endl;
    abort();
  }
  
  RooArgList nuiList(*w->var(NPName));

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

  expected_var.Print();
  w->import(expected_var);
}

