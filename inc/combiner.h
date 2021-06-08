/*
 * =====================================================================================
 *
 *       Filename:  combiner.h
 *
 *    Description:  Combine workspace
 *
 *        Version:  1.1
 *        Created:  05/17/2012 02:49:27 PM
 *       Revision:  12/27/20 during pandemic
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji, haoshuang.ji@cern.ch
 *                  Hongtao Yang, Hongtao.Yang@cern.ch
 *   Organization:  University of Wisconsin
 *                  Lawrence Berkeley National Lab
 *
 * =====================================================================================
 */

#include "CommonHead.h"
#include "RooFitHead.h"
#include "RooStatsHead.h"

#include "asimovUtil.hh"
#include "auxUtil.hh"

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

  std::map<TString, TString> renameMap_;
  std::map<TString, TString> pdfMap_;
  std::map<TString, TString> poiMap_;
  std::map<TString, TString> varMap_;

  bool simplifiedImport_;

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
  combiner(TString configFileName);
  ~combiner(){}

  void rename(bool saveTmpWs = true);
  void combine(bool readTmpWs = false, bool saveRawWs = true);
  void finalize(bool readRawWs = false);
  void printSummary();

  /* Multi-threading */
  void setNumThreads(unsigned num) { 
    m_numThreads = num;
    if (m_numThreads > std::thread::hardware_concurrency())
    {
      m_numThreads = std::thread::hardware_concurrency();
      spdlog::warn("Reducing number of threads to {} to match hardware concurrency", m_numThreads);
    }
  }

  static TString DUMMY;
  static TString WGTNAME;
  static TString WSPOSTFIX;
  static TString TMPPOSTFIX;
  static TString RAWPOSTFIX;
  static TString CATNAME;
  static TString NUISNAME;
  static TString GLOBNAME;
  static TString OBSNAME;
  static TString POINAME;
  static TString PDFNAME;
  static TString CONSTRPOSTFIX;
  static TString GOPOSTFIX;

private:
  void readConfigXml(TString configFileName);
  void makeModelConfig();
  void readChannel(TXMLNode *rootNode);
  void readRenameMap(Channel &channel, TXMLNode *node, RooWorkspace *w);
  void rename_core();
  void combine_core();
  void join();

  std::vector<Channel> m_summary;
  std::vector<POI> m_pois;

  TString m_wsName;
  TString m_mcName;
  TString m_dataName;
  TString m_outputFileName;

  std::unique_ptr<RooWorkspace> m_comb;
  std::unique_ptr<RooStats::ModelConfig> m_mc;
  std::unique_ptr<RooArgSet> m_nuis;
  std::unique_ptr<RooArgSet> m_glob;
  std::unique_ptr<RooArgSet> m_obs;
  map<string, RooDataSet *> m_dataMap;
  std::unique_ptr<RooSimultaneous> m_combPdf;
  std::unique_ptr<RooCategory> m_combCat;

  /* Temporary workspace */
  std::unique_ptr<RooWorkspace> m_tmpWs;
  std::unique_ptr<TFile> m_inputFile;

  /* Asimov handler */
  unique_ptr<asimovUtil> m_asimovHandler;

  bool m_strictMode;

  /* Multi-threading */
  int m_numThreads;
  std::vector<std::unique_ptr<std::thread>> m_thread_ptrs;
  std::mutex m_mutex;
  std::atomic<int> m_idx;
  int m_total;
  RooSimultaneous *m_curPdf;
  TList *m_curDataList;
  RooCategory *m_curCat;
  TString m_catNamePrefix;
  auxUtil::TOwnedList m_keep;
  std::vector<RooWorkspace *> m_wArr;
};
