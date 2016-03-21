/*
 * =====================================================================================
 *
 *       Filename:  decorator.cxx
 *
 *    Description:  Workspace decorator
 *
 *        Version:  1.0
 *        Created:  05/19/2012 10:09:55 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  haoshuang.ji (), haoshuang.ji@cern.ch
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <list>
#include "decorator.h"

decorator::decorator(std::string combinedFile, std::string splittedFile, std::string dataName, std::string wsName, std::string mcName)
{
  splittedFile_=splittedFile;
  TFile *fin_=TFile::Open(combinedFile.c_str());
  assert(fin_);

  if(wsName!=""){
    m_comb=dynamic_cast<RooWorkspace*>(fin_->Get(wsName.c_str()));
  }
  else{
    TList* keys = fin_->GetListOfKeys();
    TIter next(keys);
    TKey* obj;
    std::string className = "";
    
    while ((obj = (TKey*)next())) {
      className = obj->GetClassName();
      if ( className.find("RooWorkspace")!=std::string::npos ) {
	m_comb = (RooWorkspace*)obj->ReadObj();
      }
    }
  }

  assert(m_comb);
  
  if(mcName!=""){
    m_mc=dynamic_cast<ModelConfig*>(m_comb->obj(mcName.c_str()));
  }
  else{
    std::list<TObject*> allObjs = m_comb->allGenericObjects();
    for (std::list<TObject*>::iterator it = allObjs.begin(); it != allObjs.end(); it++) {
      if ( (m_mc = dynamic_cast<RooStats::ModelConfig*>(*it))) {
	  break;
      }
    }
  }
  assert ( m_mc );
  m_pdf = dynamic_cast<RooSimultaneous*>(m_mc->GetPdf()); assert (m_pdf);
  m_cat = (RooCategory*)&m_pdf->indexCat();
  m_gobs = dynamic_cast<const RooArgSet*>(m_mc->GetGlobalObservables()); assert(m_gobs);
  m_nuis = const_cast<RooArgSet*>(m_mc->GetNuisanceParameters()); assert(m_nuis);
  numChannels = m_cat->numBins(0);

  if(!m_comb){
    std::cerr<<"Workspace cannot be found in the input file. Aborting..."<<std::endl;
    abort();
  }

  if(!m_mc){
    std::cerr<<"ModelConfig cannot be found in the input file. Aborting..."<<std::endl;
    abort();
  }

  m_data=(RooDataSet*)(m_comb->data(dataName.c_str()));
  if(!m_data){
    std::cerr<<"Warning! Dataset "<<dataName.c_str()<<" cannot be found in the workspace."<<std::endl
	     <<"Will try other dataset names. Press any key to continue..."<<std::endl;
    getchar();
    m_data = (RooDataSet*)(m_comb->allData().front());
  }

  m_dataList = m_data->split( *m_cat, true );
}

decorator::~decorator()
{
  if ( m_comb ) {
    fin_->Close(); SafeDelete(fin_);
  }
}

void decorator::Tokenize(const std::string& str,
                              std::vector<std::string>& tokens,
                              const std::string& delimiters)
{
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos)
  {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);

  }
}

void decorator::decorate()
{
  std::cout<<"******************* Decorating *******************"<<std::endl;
  std::cout<<"Renaming workspace..."<<std::endl;
  std::cout<<"Workspace: old name "<<m_comb->GetName()<<", new name combWS"<<std::endl;
  m_comb->SetName("combWS");
  std::cout<<"ModelConfig: old name "<<m_mc->GetName()<<", new name ModelConfig"<<std::endl;
  m_mc->SetName("ModelConfig");

  if(typeid( *m_data ) == typeid( RooDataHist )){
    if(histToData_){
      std::cout<<"REGTEST: Converting dataset from RooDataHist to RooDataSet..."<<std::endl;
      hist2dataset();
    }
  }
  else{
    std::cout<<"Dataset: old name "<<m_data->GetName()<<", new name combData"<<std::endl;
    m_data->SetName("combData");
  }

  // Save nominal global observable values in a snapshot
  if(!m_comb->loadSnapshot("nominalGlobs")){
    TIterator *iter = m_gobs -> createIterator();
    RooRealVar* parg = NULL;
    while((parg=(RooRealVar*)iter->Next()) ){
      TString globName=parg->GetName();
      double globValue=parg->getVal();
      if(globValue!=0&&!globName.BeginsWith("nom_gamma_")){
	std::cerr<<"\t\t!!!!!!WARNING: global observable "<<globName
		 <<" has non-zero value "<<globValue
		 <<std::endl;
      }
    }
    SafeDelete(iter);
    m_comb->saveSnapshot( "nominalGlobs", *m_mc->GetGlobalObservables() );
  }
  // Save nominal global observable values in a snapshot
  if(!m_comb->loadSnapshot("nominalNuis")){
    TIterator *iter = m_nuis -> createIterator();
    RooRealVar* parg = NULL;
    while((parg=(RooRealVar*)iter->Next()) ){
      TString nuisName=parg->GetName();
      if(parg->isConstant()||parg->getMin()>=parg->getMax()){
	std::cout<<"\t\t!!!!!!WARNING: nuisance parameter "<<nuisName
		 <<" is a constant. It will be removed from nuisance parameter set."
		 <<std::endl;
	parg->setConstant(true);
	m_nuis->remove(*parg);
      }
    }
    SafeDelete(iter);
    m_comb->saveSnapshot( "nominalNuis", *m_mc->GetNuisanceParameters() );
  }

  // std::cout<<"~~~~~~~~~~~~~ Nominal global observable values ~~~~~~~~~~~~~"<<std::endl;
  // m_gobs->Print("v");

  if ( setVar_!="" ) {

    std::vector<std::string> subStrs;
    Tokenize(setVar_, subStrs, ",");
    int size = subStrs.size();
    for ( int i= 0; i < size; i++ ) {
      std::string varInput=subStrs[i];
      if(varInput.find("=")==std::string::npos){
	std::cerr<<"Input problem: variables should have the format X=Y."<<std::endl;
	continue;
      }
      std::vector<std::string> varInput_token;
      Tokenize(varInput,varInput_token, "=");
      RooRealVar *var_temp=dynamic_cast<RooRealVar*>(m_comb->var(varInput_token[0].c_str()));
      if(!var_temp){
	std::cerr<<"Variable "<<varInput_token[0].c_str()<<" cannot be found!"<<std::endl;
	continue;
      }	
      std::string varValue=varInput_token[1];
      if(varValue.find("_")==std::string::npos){
	double value=atof(varValue.c_str());
	std::cout<<"Setting variable "<<varInput_token[0].c_str()<<" value to "<<value<<" and fixing it to constant"<<std::endl;
	var_temp->setVal(value);
	var_temp->setConstant(true);
      }
      else{
	std::vector<std::string> varValue_token;
	Tokenize(varValue,varValue_token, "_");
	if(varValue_token.size()==2){
	  double xmin=atof(varValue_token[0].c_str());
	  double xmax=atof(varValue_token[1].c_str());
	  std::cout<<"Setting variable "<<varInput_token[0].c_str()<<" range to ["<<xmin<<", "<<xmax<<"]"<<std::endl;
	  var_temp->setRange(xmin,xmax);
	  var_temp->setConstant(xmax<xmin);
	}
	else{
	  double value=atof(varValue_token[0].c_str());
	  double xmin=atof(varValue_token[1].c_str());
	  double xmax=atof(varValue_token[2].c_str());

	  std::cout<<"Setting variable "<<varInput_token[0].c_str()<<" value to "<<value
		   <<", range to ["<<xmin<<", "<<xmax<<"]"<<std::endl;
	  var_temp->setVal(value);
	  var_temp->setRange(xmin,xmax);
	  var_temp->setConstant(xmax<xmin);
	}
	varValue_token.clear();
      }
      varInput_token.clear();
    }
    subStrs.clear();
  }
  std::cout<<"**************************************************"<<std::endl;
}

void decorator::hist2dataset()
{
  RooRealVar* x[500], *w[500];
  RooDataSet* data[500];
  std::map<std::string,RooDataSet*> datasetMap;

  RooArgSet *Observables=new RooArgSet();

  for ( int ich= 0; ich < numChannels; ich++ ) {
    m_cat->setBin(ich);
    TString channelname=m_cat->getLabel();
    RooAbsPdf* pdfi = m_pdf->getPdf(m_cat->getLabel());
    RooAbsData* datai = ( RooAbsData* )( m_dataList->At( ich ) );
    RooRealVar *obsi=(RooRealVar*)pdfi->getObservables(datai)->first();
    x[ich]=m_comb->var(obsi->GetName());
    w[ich]=new RooRealVar(Form("wt_%d",ich),Form("wt_%d",ich),1);

    RooArgSet* args=new RooArgSet();
    args->add(RooArgSet(*x[ich],*w[ich]));
    data[ich]=new RooDataSet("combData","combData",*args,WeightVar(*w[ich]));

    RooArgSet* obs_tmp = (RooArgSet*)datai->get();
    RooRealVar* xdata_tmp = (RooRealVar*)obs_tmp->find(obsi->GetName());

    for (int ievt=0 ; ievt<datai->numEntries() ; ievt++) {
      datai->get(ievt) ;
      double weight=datai->weight();
      x[ich]->setVal(xdata_tmp->getVal());
      w[ich]->setVal(weight);
      data[ich]->add(RooArgSet(*x[ich], *w[ich]), weight);
    }
    Observables->add(*x[ich]);
    datasetMap[channelname.Data()]=data[ich];
  }

  RooRealVar wt("wt","wt",1);//,0,10000);
  RooArgSet *args = new RooArgSet();
  args->add(*Observables);
  args->add(wt);
  RooDataSet* combData = new RooDataSet("combData","combData", *args, Index(*m_cat), Import(datasetMap) ,WeightVar(wt)); 
  m_comb->import(*combData);

  m_data=m_comb->data(combData->GetName());
  delete combData;

  
}

void decorator::printSummary(bool verbose)
{


  std::cout << "\t~~~~~~~~Begin Summary~~~~~~~~" << std::endl;
  std::cout << "\tThere are " << numChannels << " sub channels:" << std::endl;
  for ( int i= 0; i < numChannels; i++ ) {
    m_cat->setBin(i);
    RooAbsPdf* pdfi = m_pdf->getPdf(m_cat->getLabel());
    RooDataSet* datai = ( RooDataSet* )( m_dataList->At( i ) );
    std::cout << "\t\tIndex: " << i << ", Pdf: " << pdfi->GetName() << ", Data: " << datai->GetName()  << ", SumEntries: " << datai->sumEntries() << std::endl;
    // std::cout << "\t\tIndex: " << i << ", Pdf: " << pdfi->GetName() << ", Data: " << pdfi->ClassName()  << ", SumEntries: " << datai->sumEntries() << std::endl;
  }

  if(verbose){
    std::unique_ptr<TIterator> fVarIter(m_comb->componentIterator());
    for ( RooRealVar* v = (RooRealVar*)fVarIter->Next(); v!=0; v = (RooRealVar*)fVarIter->Next() ) {
      if ( std::string(v->ClassName())=="RooStats::HistFactory::FlexibleInterpVar" ) {
        FlexibleInterpVar* oldVar = dynamic_cast<FlexibleInterpVar*>(m_comb->obj(v->GetName()));
        assert ( oldVar );
        std::string varName = v->GetName();

        std::cout << "\nSample name: " << varName << std::endl;
        FlexibleInterpVarExt oldVarExt(*oldVar, "oldVarExtName");
        oldVarExt.printUncerts();

      }
    }
  }
  std::cout<<"\t ########### POI ########### \t"<<std::endl;
  m_mc->GetParametersOfInterest()->Print("v");

  std::cout<<"\t ########### Dataset ########### \t"<<std::endl;
  std::list<RooAbsData*> allData=m_comb->allData();
  for (std::list<RooAbsData*>::iterator it = allData.begin(); it != allData.end(); it++) {
    (*it)->Print();
  }

  std::cout << "\t~~~~~~~~~End Summary~~~~~~~~~" << std::endl;
}

void decorator::write()
{
  m_comb->loadSnapshot("ucmles");
  m_comb->writeToFile(splittedFile_.c_str());
  std::cout<<"File "<<splittedFile_.c_str()<<" has been saved."<<std::endl;
}

void decorator::makeSnapshots(std::string minimizerType, double tolerance, int fitFlag)
{
  asimovUtils::makeSnapshots(m_comb, m_mc, (RooDataSet*)m_data, m_pdf, splittedFile_, minimizerType, tolerance, false, "", fitFlag);
}

