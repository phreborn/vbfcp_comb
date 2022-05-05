#include <iostream>

void readTauWS(){
  TFile *f = new TFile("inWS/tautau/m00/125_lumi_split_mod.root", "read");
  RooWorkspace *ws = (RooWorkspace*) f->Get("combined");
  RooStats::ModelConfig *mc = dynamic_cast<RooStats::ModelConfig *>(ws->obj("ModelConfig"));

  ofstream nplist("tautau_nuisList.txt");
  if(!nplist){
    nplist.close();
    cout<<"error can't open file for record"<<endl;
  }

  ofstream globlist("globs/tautau_globList.txt");
  if(!globlist){
    globlist.close();
    cout<<"error can't open file for record"<<endl;
  }

  unique_ptr<TIterator> it(mc->GetNuisanceParameters()->createIterator());
  for (RooRealVar *arg = dynamic_cast<RooRealVar *>(it->Next()); arg != 0; arg = dynamic_cast<RooRealVar *>(it->Next()))
  {
    TString nuisName = arg->GetName();
    nplist<<nuisName<<endl;
  }

  it.reset(mc->GetGlobalObservables()->createIterator());
  for (RooRealVar *arg = dynamic_cast<RooRealVar *>(it->Next()); arg != 0; arg = dynamic_cast<RooRealVar *>(it->Next()))
  {
    TString nuisName = arg->GetName();
    globlist<<nuisName<<endl;
  }

  list<RooAbsData*> dataList(ws->allData());
  for(auto data : dataList){
     cout<<data->GetName()<<endl;
  }
}
