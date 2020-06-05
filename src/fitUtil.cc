#include "fitUtil.hh"

// Default values for the minimizer
int fitUtil::_minimizerStrategy=1;
string fitUtil::_minimizerAlgo="Minuit2";
double fitUtil::_minimizerTolerance=1e-3;
bool fitUtil::_nllOffset=true;
int fitUtil::_printLevel=2;
bool fitUtil::_constOpt=true;
bool fitUtil::_improveFit=false;

int fitUtil::profileToData(ModelConfig *mc, RooAbsData *data, TString rangeName){
  RooAbsPdf *pdf=mc->GetPdf();

  RooWorkspace *w=mc->GetWS();
  RooArgSet funcs = w->allPdfs();
  unique_ptr<TIterator> iter(funcs.createIterator());
  for ( RooAbsPdf* v = (RooAbsPdf*)iter->Next(); v!=0; v = (RooAbsPdf*)iter->Next() ) {
    string name = v->GetName();
    if (v->IsA() == RooRealSumPdf::Class() && name.find("binned")!=std::string::npos) { // The binned attribute will only be set if there is "binned" in the PDF name
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

  RooMinimizer minim(*nll);
  minim.setStrategy(_minimizerStrategy);
  minim.setPrintLevel(_printLevel-1);
  minim.setProfile(); /* print out time */
  minim.setEps(_minimizerTolerance/0.001);
  minim.setOffsetting(_nllOffset);
  if(_constOpt) minim.optimizeConst(2);
  
  minim.setMinimizerType(_minimizerAlgo.c_str()); // Suggested by Nicolas M.
  minim.setMaxFunctionCalls(5000*mc->GetPdf()->getVariables()->getSize());//suggest by Stefan G.
  int status=minim.minimize(_minimizerAlgo.c_str());

  if(_improveFit) minim.improve();

  // Important: remove binned likleihood attribute after the fit, otherwise it will affect Asimov dataset generation
  iter.reset(funcs.createIterator());
  for ( RooAbsPdf* v = (RooAbsPdf*)iter->Next(); v!=0; v = (RooAbsPdf*)iter->Next() ) {
    std::string name = v->GetName();
    if (v->IsA() == RooRealSumPdf::Class() && name.find("binned")!=std::string::npos) {
      std::cout << "\t\tremove binned likelihood for: " << v->GetName() << std::endl;
      v->setAttribute("BinnedLikelihood", false);
    }
  }

  return status;
}
