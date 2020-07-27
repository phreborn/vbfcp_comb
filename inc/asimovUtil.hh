#ifndef ASIMOVUTIL_HEADER
#define ASIMOVUTIL_HEADER

#include "CommonHead.h"
#include "RooFitHead.h"
#include "RooStatsHead.h"

#include "auxUtil.hh"
#include "fitUtil.hh"

using namespace std;
using namespace RooFit;
using namespace RooStats;

class asimovUtil{
private:
  vector<TString> _asimovNames, _asimovSetups, _asimovProfiles;
  vector<TString> _SnapshotsAll, _SnapshotsNuis, _SnapshotsGlob, _SnapshotsPOI, _Snapshots;
  vector<TString> _dataToFit;
  TString _rangeName;
  // action items
  static const TString RAW;
  static const TString FIT;
  static const TString RESET;
  static const TString GENASIMOV;
  static const TString FLOAT;
  static const TString FIXSYST;
  static const TString FIXALL;
  static const TString MATCHGLOB;
  static const TString SAVESNAPSHOT;
public:
  asimovUtil(){_rangeName="";}
  void addEntry(TXMLNode *node);
  void generateAsimov(ModelConfig *mc, TString dataName);
  void printSummary();
  bool genAsimov(){return _asimovNames.size()>0;}
  void setRange(TString rangeName){_rangeName=rangeName;}
  void matchGlob(ModelConfig *mc);
};

#endif
