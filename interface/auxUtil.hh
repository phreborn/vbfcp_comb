#ifndef AUXUTIL_HEADER
#define AUXUTIL_HEADER

#include "CommonHead.h"
#include "RooFitHead.h"
#include "RooStatsHead.h"

using namespace std;
using namespace RooFit;
using namespace RooStats;

namespace auxUtil{
  const TString WARNING="\033[91m";
  const TString ENDC="\033[0m";

  enum _itemType{FUNCTION, VARIABLE, EXIST};

  const double epsilon=1e-6;;
  
  void printTime();
  void printTitle(TString titleText, TString separator="-", int width=10);
  vector<TString> splitString(const TString& theOpt, const char separator );
  void removeDuplicatedString(vector<TString>& strArr);
  void removeString(vector<TString>& strArr, TString target);
  void removeWhiteSpace(TString &item);
  vector<TString> diffSet(vector<TString> A, vector<TString> B);
  int parseXMLFile(TDOMParser *xmlparser, TString inputFile);
  TString getAttributeValue( TXMLNode* rootNode, TString attributeKey, bool allowEmpty=false, TString defaultStr="");

  TString generateExpr(TString head, RooArgSet *set, bool closeExpr=true);
  void closeFuncExpr(TString &expr);
  TString getObjName(TString inputName);
  
  void defineSet(RooWorkspace *w, RooArgSet set, TString setName);
  void defineSet(RooWorkspace *w, vector<TString> set, TString setName);
  TString implementObj(RooWorkspace *w, TString expr, bool checkExistBeforeImp=false);
  void collectEverything(ModelConfig *mc, RooArgSet *set);

  void Reset(RooArgSet* original, RooArgSet* snapshot);

  int getItemType(TString item);
  TString translateItemType(int type);
  TString translateItemType(TString item);
  TString translateUncertType(int type);
  
  void alert(TString msg);
  void alertAndAbort(TString msg);
  TString combineName(TString name, TString tag);
  void setValAndFix(RooRealVar *var, double value);
  int stripSign(TString &expr);

  vector<TString> decomposeFuncStr(TString function);
  bool to_bool(TString str);
  TXMLNode* findNode(TXMLNode* rootNode, TString nodeName);
  TXMLAttr* findAttribute(TXMLNode* rootNode, TString attributeKey);
   
  bool checkExist(TString name);
  TString readNumFromOption(TString opt, TString key);
  int findBin(TH1 *h, double lowedge);
  TStyle* ATLASStyle();
  void setATLASStyle();
  int getNDOF(RooAbsPdf* pdf, RooRealVar* x, bool exclSyst=true);
  pair<double, int> calcChi2(TH1* hdata, TH1* hpdf, double blindMin = std::numeric_limits<double>::min(), double blindMax = std::numeric_limits<double>::max(), double threshold = 3);

};

#endif
