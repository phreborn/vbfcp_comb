/*
 * =====================================================================================
 *
 *       Filename:  Organizer.h
 *
 *    Description:  Orgnize the workspace
 *
 *        Version:  1.0
 *        Created:  07/19/12 17:32:58
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji (那你是真的牛逼), haoshuang.ji@cern.ch
 *   Organization:  University of Wisconsin
 *
 * Rewritten by Hongtao Yang (Hongtao.Yang@cern.ch) in 2019.
 *
 * =====================================================================================
 */

#ifndef Manager_Organizer
#define Manager_Organizer

#include "CommonHead.h"
#include "RooFitHead.h"
#include "RooStatsHead.h"
#include "rooCommon.h"

#include "auxUtil.hh"
#include "asimovUtil.hh"

class Organizer {
public:
  Organizer(TString configFileName) ;

  bool run();

  void readConfigXml( TString configFileName );
  void printSummary()
  {
    int nItems = (int)m_actionItems.size();
    for ( int i= 0; i < nItems; i++ ) {
      cout << "\tItem " << i << ", " << m_actionItems[i] << endl;
    }
  }

private:
  void implementFlexibleInterpVar(RooWorkspace *w, TString actionStr);
  void implementMultiVarGaussian(RooWorkspace *w, TString actionStr);

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
};

#endif
