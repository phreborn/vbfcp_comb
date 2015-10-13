/*
 * =====================================================================================
 *
 *       Filename:  decorator.h
 *
 *    Description:  Workspace decorator
 *
 *        Version:  1.0
 *        Created:  05/19/2012 10:10:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  haoshuang.ji (), haoshuang.ji@cern.ch
 *   Organization:
 *
 * =====================================================================================
 */

#include "rooCommon.h"
#include "TFile.h"
#include "TKey.h"
#include "TIterator.h"
#include "asimovUtils.h"

class decorator {
 public:
  decorator(std::string combinedFile, std::string splittedFile, std::string dataName);
  ~decorator();
  void decorate();
  void printSummary(bool verbose);
  void makeSnapshots(std::string minimizerType, double tolerance = 0.001, int fitFlag=0);
  void write();
  void hist2dataset();
  void Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters);
  void setVar(std::string input){ setVar_=input;}
  void setHistToData(bool input){ histToData_=input;}
 private:
  TFile *fin_;
  RooStats::ModelConfig* m_mc;
  RooWorkspace* m_comb;
  // RooAbsCategoryLValue* m_cat;
  RooCategory* m_cat;
  RooSimultaneous* m_pdf;
  RooAbsData* m_data;
  // RooRealVar* m_poi;
  RooArgSet m_poi;
  const RooArgSet* m_nuis;
  const RooArgSet* m_gobs;
  TList* m_dataList;
  int numChannels;
  std::string splittedFile_;
  bool histToData_=true;
  std::string setVar_="";
};

