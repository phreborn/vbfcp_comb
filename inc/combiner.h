/*
 * =====================================================================================
 *
 *       Filename:  combiner.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  05/17/2012 02:49:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  haoshuang.ji (), haoshuang.ji@cern.ch
 *   Organization:
 *
 * =====================================================================================
 */

#include "CommonHead.h"
#include "RooFitHead.h"
#include "RooStatsHead.h"
#include "rooCommon.h"

#include "asimovUtil.hh"

struct POI
{
  TString name;
  double min;
  double max;
  double value;

  bool operator==(const char *poiName)
  {
    return name == poiName; /* Channel name should never be duplicated */
  }
};

struct Channel
{
  TString name_; /* name of itself */
  TString fileName_;
  TString wsName_;
  TString mcName_;

  TString dataName_;
  TString categoryName_;
  std::map<TString, TString> renameMap_;
  std::map<TString, TString> pdfMap_;
  std::map<TString, TString> poiMap_;
  std::map<TString, TString> varMap_;

  void operator=(const Channel &ch)
  {
    name_ = ch.name_;
    fileName_ = ch.fileName_;
    wsName_ = ch.wsName_;
    mcName_ = ch.mcName_;
    dataName_ = ch.dataName_;
    renameMap_ = ch.renameMap_;
    pdfMap_ = ch.pdfMap_;
    poiMap_ = ch.poiMap_;
    varMap_ = ch.varMap_;
  }

  bool operator==(const Channel &ch)
  {
    return name_ == ch.name_; /* Channel name should never be duplicated */
  }

  void Print()
  {
    auxUtil::printTitle("Channel Name: " + name_, "+");
    spdlog::info("\tInput File Name: {}", fileName_.Data());
    spdlog::info("\tWorkspace Name: {}", wsName_.Data());
    spdlog::info("\tModelConfig Name: {}", mcName_.Data());
    spdlog::info("\tData Name: {}", dataName_.Data());

    for (std::map<TString, TString>::iterator iterator = poiMap_.begin(); iterator != poiMap_.end(); iterator++)
      spdlog::info("\t\tOld POI name: {} ---> New POI name: {}", iterator->first.Data(), iterator->second.Data());
    for (std::map<TString, TString>::iterator iterator = varMap_.begin(); iterator != varMap_.end(); iterator++)
      spdlog::info("\t\tOld variable name: {} ---> New variable name: {}", iterator->first.Data(), iterator->second.Data());
  }
};

class combiner
{
public:
  combiner();
  ~combiner() {}

  void combineWorkspace();
  void makeSnapshots();

  void readConfigXml(TString configFileName);
  void readChannel(TXMLNode *rootNode);

  RooArgSet *findArgSetIn(RooWorkspace *w, RooArgSet *set);
  void printSummary()
  {
    int num = (int)m_summary.size();
    auxUtil::printTitle(Form("Input summary (%d channels)", num));
    for (int i = 0; i < num; i++)
    {
      m_summary[i].Print();
    }
  }

  static TString DUMMY;
  static TString WGTNAME;

private:
  std::vector<Channel> m_summary;
  std::vector<POI> m_pois;
  Channel m_outSummary;

  TString m_wsName;
  TString m_mcName;
  TString m_dataName;
  TString m_pdfName;
  TString m_catName;
  TString m_outputFileName;

  std::unique_ptr<RooWorkspace> m_comb;
  std::unique_ptr<RooStats::ModelConfig> m_mc;

  unique_ptr<asimovUtil> m_asimovHandler;
};
