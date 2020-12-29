/*
 * =====================================================================================
 *
 *       Filename:  editor.h
 *
 *    Description:  Edit workspace
 *
 *        Version:  1.1
 *        Created:  07/19/12 17:32:58
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

#ifndef Manager_editor
#define Manager_editor

#include "CommonHead.h"
#include "RooFitHead.h"
#include "RooStatsHead.h"
#include "rooCommon.h"

#include "auxUtil.hh"
#include "asimovUtil.hh"

class editor {
public:
  editor(TString configFileName) ;
  ~editor() {}
  bool run();
  void readConfigXml( TString configFileName );
  /* Multi-threading */
  void setNumThreads(unsigned num) { 
    m_numThreads = num;
    if (m_numThreads > std::thread::hardware_concurrency())
    {
      m_numThreads = std::thread::hardware_concurrency();
      spdlog::warn("Reducing number of threads to {} to match hardware concurrency", m_numThreads);
    }
  }

private:
  void implementFlexibleInterpVar(RooWorkspace *w, TString actionStr);
  void implementMultiVarGaussian(RooWorkspace *w, TString actionStr);
  void remakeCategories();
  void join()
  {
    for (auto &thread : m_thread_ptrs)
    {
      if (thread->joinable())
        thread->join();
    }
  }

  unique_ptr<asimovUtil> _asimovHandler;
  vector<TString> m_actionItems;
  vector<TString> m_actionTypes;
  TString m_modelName; 
  vector<TString> m_poiNames;
  vector<TString> m_snapshotNP, m_snapshotGO, m_snapshotPOI, m_snapshotAll;
  TString m_mapStr;
  map<TString, pair<TString, TString> > m_constraintPdf;
  bool m_isStrict;

  // Item type names
  static const TString CONSTRAINT;
  static const TString NORMAL;
  static const TString EXTERNAL;

  // Variables which can be controlled from outside
  TString m_wsName;		// Workspace name
  TString m_mcName;		// ModelConfig name
  TString m_dsName;		// Dataset name
  TString m_inFile;		// Input file name
  TString m_outFile;	// Output file name

  /* Multi-threading */
  int m_numThreads;
  std::vector<std::unique_ptr<std::thread>> m_thread_ptrs;
  std::mutex m_mutex;
  std::atomic<int> m_idx;
  std::map<std::string, RooAbsPdf*> m_pdfMap;
  RooCategory *m_cat;
  RooSimultaneous *m_pdf;
  std::unique_ptr<RooWorkspace> m_nW;
  int m_total;
  auxUtil::TOwnedList m_keep;
};

#endif
