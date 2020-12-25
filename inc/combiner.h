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

  bool operator==(const char *chName)
  {
    return name_ == chName; /* Channel name should never be duplicated */
  }

  void Print()
  {
    auxUtil::printTitle("Channel Name: " + name_, "+");
    spdlog::info("\tInput File Name: {}", fileName_.Data());
    spdlog::info("\tWorkspace Name: {}", wsName_.Data());
    spdlog::info("\tModelConfig Name: {}", mcName_.Data());
    spdlog::info("\tData Name: {}", dataName_.Data());
  }
};

class combiner
{
public:
  combiner();
  ~combiner() {}

  void rename(bool saveTmpWs = true);
  void combine(bool readTmpWs = false, bool saveRawWs = true);
  void makeSnapshots(bool readRawWs = false);

  void readConfigXml(TString configFileName);

  void printSummary();

  /* Multi-threading */
  void setNumThreads(unsigned num) { m_numThreads = num; }
  void join()
  {
    for (auto &thread : thread_ptrs)
    {
      if (thread->joinable())
        thread->join();
    }
  }

  static TString DUMMY;
  static TString WGTNAME;
  static TString PDFPREFIX;
  static TString DATAPREFIX;
  static TString CATPREFIX;
  static TString WSPOSTFIX;
  static TString TMPPOSTFIX;
  static TString RAWPOSTFIX;

private:
  void makeModelConfig();
  void readChannel(TXMLNode *rootNode);

  std::vector<Channel> m_summary;
  std::vector<POI> m_pois;

  TString m_wsName;
  TString m_mcName;
  TString m_dataName;
  TString m_pdfName;
  TString m_catName;
  TString m_nuisName;
  TString m_globName;
  TString m_outputFileName;

  std::unique_ptr<RooWorkspace> m_comb;
  std::unique_ptr<RooStats::ModelConfig> m_mc;
  std::unique_ptr<RooArgSet> m_nuis;
  std::unique_ptr<RooArgSet> m_glob;
  std::unique_ptr<RooArgSet> m_obs;

  /* Temporary workspace */
  std::unique_ptr<RooWorkspace> m_tmpWs;
  std::unique_ptr<TFile> m_inputFile;

  /* Asimov handler */
  unique_ptr<asimovUtil> m_asimovHandler;

  bool m_strictMode;

  /* Multi-threading */
  unsigned m_numThreads;
  std::vector<std::unique_ptr<std::thread>> thread_ptrs;
  std::mutex mtx;
};
