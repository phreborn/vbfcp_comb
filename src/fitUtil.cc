#include "fitUtil.hh"

// Default values for the minimizer
int fitUtil::_minimizerStrategy=1;
string fitUtil::_minimizerAlgo="Minuit2";
double fitUtil::_minimizerTolerance=1e-3;
bool fitUtil::_nllOffset=true;
int fitUtil::_printLevel=2;

int fitUtil::profileToData(ModelConfig *mc, RooAbsData *data, TString rangeName){
  RooAbsPdf *pdf=mc->GetPdf();

  RooWorkspace *w=mc->GetWS();
  RooArgSet funcs = w->allPdfs();
  unique_ptr<TIterator> iter(funcs.createIterator());
  for ( RooAbsPdf* v = (RooAbsPdf*)iter->Next(); v!=0; v = (RooAbsPdf*)iter->Next() ) {
    string name = v->GetName();
    if (v->IsA() == RooRealSumPdf::Class()) {
      cout << "\tset binned likelihood for: " << v->GetName() << endl;
      v->setAttribute("BinnedLikelihood", true);
    }
  }
  unique_ptr<RooAbsReal> nll;
  if(rangeName!=""){
    cout<<endl<<auxUtil::WARNING<<" \tREGTEST: Performing binned fit in range "<<rangeName<<auxUtil::ENDC<<endl<<endl;
    nll.reset(pdf->createNLL(*data, Constrain(*mc->GetNuisanceParameters()), GlobalObservables(*mc->GetGlobalObservables()), Range(rangeName), SplitRange()));
  }
  else nll.reset(pdf->createNLL(*data, Constrain(*mc->GetNuisanceParameters()), GlobalObservables(*mc->GetGlobalObservables())));
  nll->enableOffsetting(_nllOffset);
  RooMinimizer minim(*nll);
  minim.setStrategy(_minimizerStrategy);
  minim.setPrintLevel(_printLevel-1);
  minim.setProfile(); /* print out time */
  minim.setEps(_minimizerTolerance/0.001);
  minim.optimizeConst(2);
  int status=minim.minimize(_minimizerAlgo.c_str());
  return status;
}
