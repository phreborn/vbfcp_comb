#include <iostream>

void readTauWS(){
  TFile *f = new TFile("/scratchfs/atlas/huirun/atlaswork/VBF_CP/TauWS/130719_MVA_comb_data/m001/combined/125.root", "read");
  RooWorkspace *ws = (RooWorkspace*) f->Get("combined");
  RooStats::ModelConfig *mc = dynamic_cast<RooStats::ModelConfig *>(ws->obj("ModelConfig"));

  ofstream nplist("tautau_npList.txt");
  if(!nplist){
    nplist.close();
    cout<<"error can't open file for record"<<endl;
  }

  ofstream globlist("tautau_globList.txt");
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
}
