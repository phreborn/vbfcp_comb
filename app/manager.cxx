#include "CommonHead.h"

#include "combiner.h"
#include "editor.h"
#include "splitter.h"

#include "spdlog/spdlog.h"

TString what_ = "";
TString configFile_ = "";
bool verbose_ = false;
int numThreads_ = 1;

/* Following are only used in workspace split/regularization */
TString inputFile_ = "";
TString outputFile_ = "";
TString dataName_ = "obsData";
TString wsName_ = "combWS";
TString mcName_ = "ModelConfig";
TString indices_ = "";
TString snapshots_ = "";
bool editRFV_ = false;
int reBin_ = -1;

struct option longopts[] = {
    {"what", required_argument, NULL, 'w'},
    {"ConfigXml", required_argument, NULL, 'x'},
    {"CombinedFile", required_argument, NULL, 'f'},
    {"OutputFile", required_argument, NULL, 'p'},
    {"minimizerAlgo", required_argument, NULL, 'm'},
    {"minimizerStrategy", required_argument, NULL, 's'},
    {"minimizerTolerance", required_argument, NULL, 't'},
    {"indices", required_argument, NULL, 'i'},
    {"nllOffset", required_argument, NULL, 'n'},
    {"constOpt", required_argument, NULL, 'c'},
    {"verbose", required_argument, NULL, 'v'},
    {"wsName", required_argument, NULL, 301},    
    {"mcName", required_argument, NULL, 302},    
    {"dataName", required_argument, NULL, 303},    
    {"editRFV", required_argument, NULL, 304},    
    {"rebin", required_argument, NULL, 305},
    {"snapshots", required_argument, NULL, 306},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}};

void printHelp(TString exe)
{
  cout << "Usage: " << exe << " [options]" << endl;
  cout << "Allowed options:" << endl;
  cout << " -w [ --what ] arg                  What to do: combine/edit/split/regulate (required)" << endl; // Simplify to combine, edit (the old organize), and split (which should contain features in decorate)
  cout << " -x [ --ConfigXml ] arg             Input XML configure file" << endl;
  cout << " -f [ --CombinedFile ] arg          Input workspace file" << endl;
  cout << " -p [ --SplittedFile ] arg          Output workspace file" << endl;
  cout << " -v [ --verbose ] arg               Printing out debug info or not (default no)" << endl;
  cout << " -i [ --indices ] arg               Category indices to be included in the split file" << endl;
  cout << " -m [ --minimizerAlgo ] arg         Minimizer algorithm (default Minuit2)" << endl;
  cout << " -s [ --minimizerStrategy ] arg     Minimizer strategy (default 1)" << endl;
  cout << " -t [ --minimizerTolerance ] arg    Minimizer tolerance (default 1e-3)" << endl;
  cout << " -o [ --nllOffset ] arg             Enable nllOffset (default on)" << endl;
  cout << " -c [ --constOpt ] arg              Enable constant optimization (default on)" << endl;
  cout << " -r [ --editRFV ] arg               Fixing RooFormulaVar using hard-coded variable name (default off)" << endl;
  cout << " -h [ --help ]                      Produce help message" << endl;
}

int main(int argc, char **argv)
{
  RooMsgService::instance().getStream(1).removeTopic(RooFit::NumIntegration);
  RooMsgService::instance().getStream(1).removeTopic(RooFit::Fitting);
  RooMsgService::instance().getStream(1).removeTopic(RooFit::Minimization);
  RooMsgService::instance().getStream(1).removeTopic(RooFit::InputArguments);
  RooMsgService::instance().getStream(1).removeTopic(RooFit::Eval);

  RooMsgService::instance().setGlobalKillBelow(RooFit::ERROR);

  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [thread %t] [%^%l%$] \t %v");

  int oc;
  while ((oc = getopt_long(argc, argv, ":w:x:f:o:i:v:m:s:t:p:c:r:n:h", longopts, NULL)) != -1)
  {
    switch (oc)
    {
    case 'w':
      what_ = optarg;
      break;
    case 'x':
      configFile_ = optarg;
      break;
    case 'f':
      inputFile_ = optarg;
      break;
    case 'p':
      outputFile_ = optarg;
      break;
    case 'i':
      indices_ = optarg;
      break;
    case 'v':
      verbose_ = auxUtil::to_bool(optarg);
      spdlog::info("Set verbose mode to {}", verbose_);
      break;
    case 'm':
      fitUtil::_minimizerAlgo = optarg;
      spdlog::info("Set minimizer algorithm to {}", fitUtil::_minimizerAlgo.c_str());
      break;
    case 's':
      fitUtil::_minimizerStrategy = atoi(optarg);
      spdlog::info("Set minimizer strategy to {}", fitUtil::_minimizerStrategy);
      break;
    case 't':
      fitUtil::_minimizerTolerance = atof(optarg);
      spdlog::info("Set minimizer tolerance to {}", fitUtil::_minimizerTolerance);
      break;
    case 'o':
      fitUtil::_nllOffset = auxUtil::to_bool(optarg);
      spdlog::info("Set NLL offset to {}", fitUtil::_nllOffset);
      break;
    case 'c':
      fitUtil::_constOpt = auxUtil::to_bool(optarg);
      spdlog::info("Set constant optimization to {}", fitUtil::_constOpt);
      break;
    case 'r':
      editRFV_ = auxUtil::to_bool(optarg);
      spdlog::info("Set editing RooFormulaVar to {}", editRFV_);
      break;
    case 'n':
      numThreads_ = atoi(optarg);
      spdlog::info("Set number of threads to {}", numThreads_);
      break;
    case 301:
      wsName_ = optarg;
      spdlog::info("Set workspace name to {}", wsName_.Data());
      break;
    case 302:
      wsName_ = optarg;
      spdlog::info("Set ModelConfig name to {}", mcName_.Data());
      break;
    case 303:
      dataName_ = optarg;
      spdlog::info("Set dataset name to {}", mcName_.Data());
      break;            
    case 304:
      editRFV_ = atoi(optarg);
      spdlog::info("Set editting RooFormularVar to {}", editRFV_);
      break;
    case 305:
      reBin_ = atoi(optarg);
      spdlog::info("Set number of bins to {}", reBin_);
      break;
    case 306:
      snapshots_ = optarg;
      spdlog::info("Set snapshots to {}", snapshots_.Data());
      break;                  
    case 'h':
      printHelp(argv[0]);
      return 0;
    case ':': /* missing option argument */
      spdlog::error("{}: option `-{}' requires an argument", argv[0], optopt);
      printHelp(argv[0]);
      return 0;
    case '?':
    default:
      spdlog::error("{}: option `-{}' is invalid: ignored\n", argv[0], optopt);
      printHelp(argv[0]);
      return 0;
    }
  }

  TStopwatch timer;

  if (what_ == "combine")
  {
    spdlog::info("Performing workspace combination");
    std::unique_ptr<combiner> comb(new combiner(configFile_));
    comb->printSummary();
    comb->setNumThreads(numThreads_);
    comb->rename(true);
    comb->combine(true, true);
    comb->finalize(true);
  }
  else if (what_ == "edit")
  {
    unique_ptr<editor> edit(new editor(configFile_));
    edit->setNumThreads(numThreads_);
    edit->run();
  }
  else if (what_ == "split" || what_ == "regulate")
  {
    if (outputFile_ == "")
      outputFile_ = "splitted.root";
    if (inputFile_ == "")
      auxUtil::alertAndAbort("Please specify input file to split");
    if (what_ == "regulate")
      indices_ = "all";

    unique_ptr<splitter> split(new splitter(inputFile_, outputFile_, wsName_, mcName_, dataName_));
    split->setEditRFV(editRFV_);
    split->setRebin(reBin_);
    if (snapshots_ != "")
      split->setSnapshots(snapshots_);
    split->printSummary();
    if (indices_ != "")
    {
      split->fillIndices(indices_);
      split->makeWorkspace();
    }
  }  
  else
  {
    auxUtil::alertAndAbort("Unknown command " + what_);
  }

  timer.Stop();
  double t_cpu_ = timer.CpuTime() / 60.;
  double t_real_ = timer.RealTime() / 60.;
  spdlog::info("Done in {} min (cpu), {} min (real)", t_cpu_, t_real_);

  return 1;
}
