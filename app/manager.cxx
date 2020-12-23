#include "CommonHead.h"

#include "combiner.h"
#include "splitter.h"
#include "Organizer.h"
#include "decorator.h"

#include "spdlog/spdlog.h"

std::string what_ = "";
std::string configFile_ = "";
std::string combinedFile_ = "";
std::string splittedFile_ = "";
bool snapShot_ = false;             // To be moved into XML file
std::string snapshotHintFile_ = ""; // To be moved into XML file
std::string indice_ = "";
std::string toBeFixed_ = "";                 // To be moved to workspace editing
bool mkInjectedWS_ = false;                  // To be moved to workspace editing
bool simpleScale_ = false;                   // To be moved to workspace editing
std::string injectFile_ = "";                // To be moved to workspace editing
std::string injectMass_ = "";                // To be moved to workspace editing
std::string injectStr_ = "mu~1";             // To be moved to workspace editing
std::string injectDataSet_ = "asimovData_1"; // To be moved to workspace editing
std::string replaceStr_ = "";                // To be moved to workspace editing
std::string usePseudoData_ = "";             // To be moved to workspace editing
std::string dataName_ = "obsData";           // To be moved to XML file
std::string wsName_ = "combWS";              // To be moved to XML file
std::string mcName_ = "ModelConfig";         // To be moved to XML file

double rMax_ = -999;    // To be moved to XML file
double mHiggs_ = -1;    // To be removed
bool singlePoi_ = true; // To be moved to XML file
int fitFlag_ = 0;       // To be moved to XML file
bool editRFV_ = false;
int procedure_ = 0; // To be moved to XML file
bool verbose_ = false;

int reBin_ = 500;

bool histToData_ = true; // To be moved to XML file
std::string setVar_ = "";

struct option longopts[] = {
    {"what", required_argument, NULL, 'w'},
    {"ConfigXml", required_argument, NULL, 'x'},
    {"CombinedFile", required_argument, NULL, 'f'},
    {"SplittedFile", required_argument, NULL, 'p'},
    {"minimizerAlgo", required_argument, NULL, 'm'},
    {"minimizerStrategy", required_argument, NULL, 's'},
    {"minimizerTolerance", required_argument, NULL, 't'},
    {"indices", required_argument, NULL, 'i'},
    {"nllOffset", required_argument, NULL, 'n'},
    {"constOpt", required_argument, NULL, 'c'},
    {"editRFV", required_argument, NULL, 'r'},
    {"verbose", required_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}};

void printHelp(TString exe)
{
  cout << "Usage: " << exe << " [options]" << endl;
  cout << "Allowed options:" << endl;
  cout << " -w [ --what ] arg                  What to do: combine/split/organize/decorate (required)" << endl; // Simplify to combine, edit (the old organize), and split (which should contain features in decorate)
  cout << " -x [ --ConfigXml ] arg             Input XML configure file" << endl;
  cout << " -f [ --CombinedFile ] arg          Input workspace file" << endl;
  cout << " -p [ --SplittedFile ] arg          Output workspace file" << endl;
  cout << " -v [ --verbose ] arg               Printing out debug info or not (default no)" << endl;
  cout << " -i [ --indices ] arg               Category indices to be included in the split file" << endl;
  cout << " -m [ --minimizerAlgo ] arg         Minimizer algorithm (default Minuit2)" << endl;
  cout << " -s [ --minimizerStrategy ] arg     Minimizer strategy (default 1)" << endl;
  cout << " -t [ --minimizerTolerance ] arg    Minimizer tolerance (default 1e-3)" << endl;
  cout << " -n [ --nllOffset ] arg             Enable nllOffset (default on)" << endl;
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

  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] \t %v");

  int oc;
  while ((oc = getopt_long(argc, argv, ":w:x:f:p:i:v:m:s:t:n:c:r:h", longopts, NULL)) != -1)
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
      combinedFile_ = optarg;
      break;
    case 'p':
      splittedFile_ = optarg;
      break;
    case 'i':
      indice_ = optarg;
      break;
    case 'v':
      verbose_ = auxUtil::to_bool(optarg);
      cout << "Set verbose mode to " << verbose_ << endl;
      break;
    case 'm':
      fitUtil::_minimizerAlgo = optarg;
      cout << "Set minimizer algorithm to " << fitUtil::_minimizerAlgo << endl;
      break;
    case 's':
      fitUtil::_minimizerStrategy = atoi(optarg);
      cout << "Set minimizer strategy to " << fitUtil::_minimizerStrategy << endl;
      break;
    case 't':
      fitUtil::_minimizerTolerance = atof(optarg);
      cout << "Set minimizer tolerance to " << fitUtil::_minimizerTolerance << endl;
      break;
    case 'n':
      fitUtil::_nllOffset = auxUtil::to_bool(optarg);
      cout << "Set NLL offset to " << fitUtil::_nllOffset << endl;
      break;
    case 'c':
      fitUtil::_constOpt = auxUtil::to_bool(optarg);
      cout << "Set constant optimization to " << fitUtil::_constOpt << endl;
      break;
    case 'r':
      editRFV_ = auxUtil::to_bool(optarg);
      cout << "Set editing RooFormulaVar to " << editRFV_ << endl;
      break;
    case 'h':
      printHelp(argv[0]);
      return 0;
    case ':': /* missing option argument */
      fprintf(stderr, "%s: option `-%c' requires an argument\n",
              argv[0], optopt);
      printHelp(argv[0]);
      return 0;
    case '?':
    default:
      fprintf(stderr, "%s: option `-%c' is invalid: ignored\n",
              argv[0], optopt);
      printHelp(argv[0]);
      return 0;
    }
  }

  TStopwatch timer;

  if (what_ == "combine")
  {
    spdlog::info("Performing workspace combination");
    std::unique_ptr<combiner> comb;
    comb.reset(new combiner());
    comb->readConfigXml(configFile_);
    comb->printSummary();
    comb->combineWorkspace();
    comb->makeSnapshots();
  }
  else if (what_ == "split")
  {
    std::string dataName = dataName_;
    if (splittedFile_ == "")
      splittedFile_ = "splitted.root";
    if (combinedFile_ == "")
      std::cerr << "Please specify what file to split" << std::endl;
    EXIT_FAILURE;
    unique_ptr<splitter> split(new splitter(combinedFile_, splittedFile_, usePseudoData_ == "" ? dataName : usePseudoData_));
    split->printSummary(verbose_);
    split->fillIndice(indice_);

    split->setReplaceStr(replaceStr_);
    /* may have to fix some nuisance parameters */
    if (toBeFixed_ != "")
    {
      split->fillFixed(toBeFixed_);
    }

    split->makeWorkspace(rMax_, reBin_, mHiggs_, editRFV_);

    if (indice_ == "all")
    {
      /* take the asimov */
      split->grabAsimov(combinedFile_);
    }
  }
  else if (what_ == "organize")
  {
    Organizer *org = new Organizer(configFile_);
    // org->printSummary();
    org->run();
  }
  else if (what_ == "decorate")
  {
    if (splittedFile_ == "")
      splittedFile_ = "decorated.root";
    if (combinedFile_ == "")
      std::cerr << "Please specify what file to split" << std::endl;
    EXIT_FAILURE;
    decorator *decorate = new decorator(combinedFile_, splittedFile_, dataName_, wsName_, mcName_);
    decorate->setVar(setVar_);
    decorate->setHistToData(histToData_);
    decorate->decorate();

    if (snapShot_)
    {
      decorate->makeSnapshots(fitUtil::_minimizerAlgo, fitUtil::_minimizerTolerance, fitFlag_);
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
  double t_cpu_ = timer.CpuTime() / 60.;
  double t_real_ = timer.RealTime() / 60.;
  printf("Done in %.2f min (cpu), %.2f min (real)\n", t_cpu_, t_real_);

  return 1;
}
