#include <assert.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <typeinfo>

#include "combiner.h"
#include "splitter.h"
#include "Organizer.h"
#include "decorator.h"
#include <boost/program_options.hpp>

// ATLAS-CMS combination
#include "CommonOptions.h"
#include "asimovUtilsOptions.h"
// bool combine_ = true;

bool histToData_=true;
std::string setVar_="";

int main( int argc, char** argv )
{
  using namespace boost;
  namespace po = boost::program_options;
  po::options_description desc( "Main options" );
  desc.add_options()
    ("help,h", "Produce help message")
    // ( "Combine,c",               po::value<bool>( &combine_ )->default_value( combine_ ), "do the combination" )
    ( "what,w",               po::value<std::string>( &what_ )->default_value( what_ ), "what to do? combine/split/orgnize/decorate" )
    ( "ConfigXml,x",               po::value<std::string>( &configFile_ )->default_value( configFile_ ), "The configure xml file" )
    ( "CombinedFile,f",               po::value<std::string>( &combinedFile_ )->default_value( combinedFile_ ), "force the output combined file name, even though it is given in the xml file" )
    ( "SplittedFile,p",               po::value<std::string>( &splittedFile_ )->default_value( splittedFile_ ), "force the output splitted file name" )
    ( "Snapshot,s",               po::value<bool>( &snapShot_ )->default_value( snapShot_ ), "" )
    ( "SnapshotHintFile",               po::value<std::string>( &snapshotHintFile_ )->default_value( snapshotHintFile_ ), "" )
    ( "MinimizerType,m",               po::value<std::string>( &minimizerType_ )->default_value( minimizerType_ ), "" )
    ( "Indice,i",               po::value<std::string>( &indice_ )->default_value( indice_ ), "Select the sub-categories indice" )
    ( "Fix,F",               po::value<std::string>( &toBeFixed_ )->default_value( toBeFixed_ ), "Fix some nuisance parameters" )
    ( "UsePseudoData",               po::value<std::string>( &usePseudoData_ )->default_value( usePseudoData_ ), "Use Pseudo data as observed data" )
    ( "mkInjectedWS",               po::value<bool>( &mkInjectedWS_ )->default_value( mkInjectedWS_ ), "Make signal injected workspace" )
    ( "lumiScale",               po::value<bool>( &simpleScale_ )->default_value( simpleScale_ ), "" )
    ( "injectFile",               po::value<std::string>( &injectFile_ )->default_value( injectFile_ ), "" )
    ( "injectMass",               po::value<std::string>( &injectMass_ )->default_value( injectMass_ ), "" )
    ( "injectString",               po::value<std::string>( &injectStr_ )->default_value( injectStr_ ), "" )
    ( "injectData",               po::value<std::string>( &injectDataSet_ )->default_value( injectDataSet_ ), "" )
    ( "replaceString",               po::value<std::string>( &replaceStr_ )->default_value( replaceStr_ ), "" )
    ( "rMax,r",               po::value<double>( &rMax_ )->default_value( rMax_ ), "Set the max value for the global poi" )
    ( "tolerance",               po::value<double>( &tolerance_ )->default_value( tolerance_ ), "" )
    ( "mHiggs",               po::value<double>( &mHiggs_ )->default_value( mHiggs_ ), "Set the mass of the higgs" )
    ( "ReBin",               po::value<int>( &reBin_ )->default_value( reBin_ ), "Rebin the gammagamma dataset" )
    ( "fitFlag,t",               po::value<int>( &fitFlag_ )->default_value( fitFlag_ ), "Choose which one to fit" )
    ( "editBR",               po::value<bool>( &editBR_ )->default_value( editBR_ ), "Edit BR uncertainties" )
    ( "editPDF",               po::value<bool>( &editPDF_ )->default_value( editPDF_ ), "Edit PDF uncertainties" )
    ( "editRFV",               po::value<bool>( &editRFV_ )->default_value( editRFV_ ), "Edit RooFormulaVar" )
    ( "procedure",               po::value<int>( &procedure_ )->default_value( procedure_ ), "" )
    ( "makeBOnly",               po::value<bool>( &makeBOnly_ )->default_value( makeBOnly_ ), "do we make b-only" )
    ( "verbose",               po::value<bool>( &verbose_ )->default_value( verbose_ ), "verbose..." )
    ( "dataName,d",               po::value<std::string>( &dataName_ )->default_value( dataName_ ), "Name of the dataset" )
    ("minimizerTolerance", po::value<float>(&minimizerTolerance_)->default_value(minimizerTolerance_),  "Tolerance for minimizer used for profiling")
    ("minimizerStrategy", po::value<int>(&minimizerStrategy_)->default_value(minimizerStrategy_),  "Strategy for minimizer used for profiling")
    ("nllOffset", po::value<bool>(&nllOffset_)->default_value(nllOffset_),  "Enable NLL offsetting")
    ("improveFit", po::value<bool>(&improveFit_)->default_value(improveFit_), "Whether to call improve() after fit converges")
    ("robustFit", po::value<bool>(&robustFit_)->default_value(robustFit_), "Whether to fit again after fit converges")
    ("setVar", po::value<std::string>(&setVar_)->default_value(setVar_), "Manipulating variables in the workspace")
    ("generateAsimov", po::value<bool>(&generateAsimov_)->default_value(generateAsimov_), "Generate Asimov data or not")
    ("histToData", po::value<bool>(&histToData_)->default_value(histToData_), "Convert RooDataHist to RooDataSet")
    ;
  po::variables_map vm0;
  
  try
  {
    po::store( po::command_line_parser( argc, argv ).options( desc ).run(), vm0 );
    po::notify( vm0 );
  }
  catch ( std::exception& ex )
  {
    std::cerr << "Invalid options: " << ex.what() << std::endl;
    std::cout << "Invalid options: " << ex.what() << std::endl;
    std::cout << "Use manager --help to get a list of all the allowed options"  << std::endl;
    return 999;
  }
  catch ( ... )
  {
    std::cerr << "Unidentified error parsing options." << std::endl;
    return 1000;
  }

  // if help, print help
  if ( vm0.count( "help" ) )
  {
    std::cout << "Usage: manager [options]\n";
    std::cout << desc;
    return 0;
  }

  RooMsgService::instance().getStream(1).removeTopic(RooFit::NumIntegration) ;
  RooMsgService::instance().getStream(1).removeTopic(RooFit::Fitting) ;
  RooMsgService::instance().getStream(1).removeTopic(RooFit::Minimization) ;
  RooMsgService::instance().getStream(1).removeTopic(RooFit::InputArguments) ;
  RooMsgService::instance().getStream(1).removeTopic(RooFit::Eval) ;

  RooMsgService::instance().setGlobalKillBelow(RooFit::ERROR);

  TStopwatch timer;

  if ( what_=="combine" )
  {
    bool doStep1 = false;
    bool doStep2 = false;
    bool doStep3 = false;
    bool doStep0 = false;
    if ( procedure_==0 ) {
      doStep1 = true;
      doStep2 = true;
      doStep3 = true;
    } else if ( procedure_==1 ) {
      doStep1 = true;
    } else if ( procedure_==2 ) {
      doStep2 = true;
    } else if ( procedure_==3 ) {
      doStep3 = true;
    } else if ( procedure_==4 ) {
      doStep0 = true;
      writemuhatWS_=false;
    }

    if(doStep0) {
      combiner* comb = new combiner();
      comb->readConfigXml(configFile_);
      comb->printSummary();
      comb->editBR(editBR_); /* set the flag */
      comb->editPDF(editPDF_); /* set the flag */
      comb->editRFV(editRFV_);
      comb->initWorkspace(combinedFile_+"_tmp.root", combiner::nRnWTmp); /* make the temporary ws */
      comb->makeBOnly(makeBOnly_);
      comb->makeSimCategory();
      comb->regularizeWorkspace();
      comb->makeSnapshots0( minimizerType_, combinedFile_, snapshotHintFile_, minimizerTolerance_, false, fitFlag_);
      comb->write(combinedFile_);
    }


    if(doStep1) {
      combiner* comb = new combiner();
      comb->readConfigXml(configFile_);
      comb->printSummary();
      comb->editBR(editBR_); /* set the flag */
      comb->editPDF(editPDF_); /* set the flag */
      comb->editRFV(editRFV_);
      // comb->initWorkspace(combinedFile_+"_tmp.root", true); /* make the temporary ws */
      comb->initWorkspace(combinedFile_+"_tmp.root", combiner::WriteTmp); /* make the temporary ws */
      delete comb; comb = NULL;
      std::cout << "\tMade simple-merge workspace ~~~~ " << std::endl;

    }
    if(doStep2) {
      combiner* comb1 = new combiner();
      comb1->readConfigXml(configFile_);
      comb1->editBR(editBR_); /* set the flag */
      comb1->editPDF(editPDF_); /* set the flag */
      // comb1->initWorkspace(combinedFile_+"_tmp.root", false); /* read the temporary ws */
      comb1->initWorkspace(combinedFile_+"_tmp.root", combiner::ReadTmp); /* read the temporary ws */
      comb1->makeBOnly(makeBOnly_);
      comb1->makeSimCategory();
      comb1->regularizeWorkspace();
      // comb1->write(combinedFile_+"_raw0.root");
      comb1->write(combinedFile_+"_raw.root");
      delete comb1; comb1 = NULL;
      std::cout << "\tMade raw workspace ~~~~ " << std::endl;
    }

    if(doStep3) {
      combiner::makeSnapshots( minimizerType_, combinedFile_, snapshotHintFile_, minimizerTolerance_, true, fitFlag_);
      std::cout << "\tMade final workspace ~~~~ " << std::endl;
    }
  }
  else if ( what_=="split" )
  {
    writemuhatWS_=false;
    std::string dataName = dataName_;
    splitter* split = new splitter(combinedFile_, splittedFile_, usePseudoData_== "" ? dataName : usePseudoData_);
    split->printSummary(verbose_);
    split->fillIndice(indice_);

    split->makeBOnly(makeBOnly_);

    split->setReplaceStr(replaceStr_);
    /* may have to fix some nuisance parameters */
    if ( toBeFixed_!="" ) { split->fillFixed(toBeFixed_); }

    split->makeWorkspace(rMax_, reBin_, mHiggs_, editRFV_);

    if ( mkInjectedWS_ ) {
      assert ( indice_ == "all" );
      split->makeInjection(
          injectFile_,
          injectMass_,
          injectStr_,
          simpleScale_,
          true,
          injectDataSet_
          );
    }

    if ( snapShot_ )
    {
      split->makeSnapshots(minimizerType_, tolerance_, singlePoi_, fitFlag_);
    } else if ( indice_ == "all" ) {
      /* take the asimov */
      split->grabAsimov(combinedFile_);
    }

    delete split; split = NULL;
  }
  else if ( what_=="orgnize" )
  {
    Organizer* org = new Organizer();
    org->readConfigXml(configFile_);
    // org->printSummary();
    org->run(snapShot_);
  }
  else if ( what_=="decorate" )
  {

    decorator* decorate=new decorator(combinedFile_,splittedFile_,dataName_);
    decorate->setVar(setVar_);
    decorate->setHistToData(histToData_);
    decorate->decorate();
    
    if ( snapShot_ ){
      decorate->makeSnapshots(minimizerType_, tolerance_, fitFlag_);
    }
    decorate->printSummary(verbose_);
    decorate->write();
  }
  else
  {
    std::cout << "Should at least point out what to do... " << std::endl;
    EXIT_FAILURE;
  }

  timer.Stop();
  double t_cpu_ = timer.CpuTime()/60.;
  double t_real_ = timer.RealTime()/60.;
  printf("Done in %.2f min (cpu), %.2f min (real)\n", t_cpu_, t_real_);

  return 1;
}
