/*
 * =====================================================================================
 *
 *       Filename:  splitter.h
 *
 *    Description:  Split/regularize workspace
 *
 *        Version:  1.1
 *        Created:  05/19/2012 10:10:20 PM
 *       Revision:  12/27/20 during pandemic
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji (haoshuang.ji@cern.ch)
 *                  Hongtao Yang (Hongtao.Yang@cern.ch)
 *   Organization:  University of Wisconsin
 *                  Lawrence Berkeley National Lab
 *
 * =====================================================================================
 */

#include "CommonHead.h"
#include "RooFitHead.h"
#include "RooStatsHead.h"

#include "auxUtil.hh"

class splitter
{
public:
    splitter(
        TString inputFileName,
        TString outputFileName,
        TString wsName,
        TString mcName,
        TString dataName);
    ~splitter() {}
    void grabAsimov(TString combinedFile);
    void printSummary();
    void fillIndices(TString indices);
    void setRebin(int reBin) { m_reBin = reBin; }
    void setEditRFV(bool flag) { m_editRFV = flag; }
    void setRebuildPdf(bool flag) { m_rebuildPdf = flag; }
    void setSnapshots(TString snapshots) { m_snapshots = auxUtil::splitString(snapshots, ','); }
    void makeWorkspace();
    static TString WGTNAME;
    static TString PDFPOSTFIX;

private:
    void buildSimPdf(RooAbsPdf *pdf, RooAbsData *data);
    void histToDataset(RooDataHist *data);
    RooAbsPdf *rebuildCatPdf(RooAbsPdf *pdf, RooAbsData *data);
    RooDataSet *rebuildCatData(RooAbsData *data, RooArgSet *obs);

    bool m_editRFV;
    int m_reBin;
    bool m_rebuildPdf;

    std::unique_ptr<TFile> m_inputFile;
    RooStats::ModelConfig *m_mc;
    RooWorkspace *m_comb;
    RooCategory *m_cat;
    RooSimultaneous *m_pdf;
    RooDataSet *m_data;
    RooArgSet m_poi;
    const RooArgSet *m_nuis;
    const RooArgSet *m_gobs;
    TList *m_dataList;
    int m_numChannels;

    std::vector<int> m_useIndices;
    std::vector<TString> m_snapshots;
    
    TString m_outputFileName;

    auxUtil::TOwnedList m_keep;
};
