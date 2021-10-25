#ifndef FITUTIL_HEADER
#define FITUTIL_HEADER

#include "CommonHead.h"
#include "RooFitHead.h"
#include "RooStatsHead.h"

#include "auxUtil.hh"

using namespace RooFit;
using namespace RooStats;

class fitUtil{
public:
  // Minimizer setup
  static int _minimizerStrategy;
  static std::string _minimizerAlgo;
  static double _minimizerTolerance;
  static bool _nllOffset;
  static int _printLevel;
  static bool _constOpt;
  static bool _improveFit;
  
  static int profileToData(ModelConfig *mc, RooAbsData *data, TString rangeName="");
};

#endif
