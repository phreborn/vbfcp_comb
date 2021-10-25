/*
 * =====================================================================================
 *
 *       Filename:  editor.h
 *
 *    Description:  Edit workspace
 *
 *        Version:  1.1
 *        Created:  07/19/12 17:32:58
 *       Revision:  10/25/21 during pandemic
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji, haoshuang.ji@cern.ch
 *                  Hongtao Yang (杨洪洮), Hongtao.Yang@cern.ch
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

#include "auxUtil.hh"
#include "asimovUtil.hh"

class editor {
public:
  editor(TString configFileName) ;
  ~editor() {}
  bool run();
  void readConfigXml( TString configFileName );

private:
  void implementFlexibleInterpVar(RooWorkspace *w, TString actionStr);
  void implementMultiVarGaussian(RooWorkspace *w, TString actionStr);
  void remakeCategories(RooWorkspace *w);

  std::unique_ptr<asimovUtil> _asimovHandler;
  std::vector<TString> m_actionItems;
  std::vector<TString> m_actionTypes;
  TString m_modelName; 
  std::vector<TString> m_poiNames;
  std::vector<TString> m_snapshotNP, m_snapshotGO, m_snapshotPOI, m_snapshotAll;
  TString m_mapStr;
  std::map<TString, std::pair<TString, TString> > m_constraintPdf;
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

  RooWorkspace *m_oW;                 // Old workspace
  ModelConfig *m_oMc;                 // ModelConfig in old workspace
  std::unique_ptr<RooWorkspace> m_nW; // New workspace
  std::unique_ptr<ModelConfig> m_nMc; // New ModelConfig
};

#endif
