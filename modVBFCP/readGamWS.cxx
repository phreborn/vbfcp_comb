#include <iostream>

void readGamWS(){
  TFile *f = new TFile("inWS/gamgam/Observed/vbf_cp_m00_mod.root", "read");
  RooWorkspace *ws = (RooWorkspace*) f->Get("combWS");
  RooStats::ModelConfig *mc = dynamic_cast<RooStats::ModelConfig *>(ws->obj("ModelConfig"));

  ofstream nplist("yy_nuisList.txt");
  if(!nplist){
    nplist.close();
    cout<<"error can't open file for record"<<endl;
  }

  ofstream globlist("globs/yy_globList.txt");
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
