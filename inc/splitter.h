/*
 * =====================================================================================
 *
 *       Filename:  splitter.h
 *
 *    Description:  Workspace splitter
 *
 *        Version:  1.0
 *        Created:  05/19/2012 10:10:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji (haoshuang.ji@cern.ch), Hongtao Yang (Hongtao.Yang@cern.ch)
 *   Organization:
 *
 * =====================================================================================
 */

#include "rooCommon.h"
#include "TFile.h"
#include "TKey.h"
#include "TIterator.h"
#include "asimovUtil.hh"

class splitter
{
public:
    splitter(
        std::string combinedFile,
        std::string splittedFile,
        std::string dataName = "combData");
    ~splitter();
    void grabAsimov(std::string combinedFile);
    void printSummary(bool verbose = false);
    void fillIndice(std::string indice);
    void fillFixed(std::string toBeFixed);
    void makeWorkspace(double rMax_ = -999, int reBin = -1, double mass = -1, bool editRFV = true);
    void findArgSetIn(RooWorkspace *w, RooArgSet *set);

    void getParaAndVals(
        std::string parseStr,
        std::vector<std::string> &paras,
        std::vector<double> &vals);
    void setReplaceStr(std::string res)
    {
        replaceStr_ = res;
    }

    void linkMap(
        std::map<std::string, std::string> &attMap,
        std::string &keyStr,
        std::string &valueStr,
        std::string linker = ",")
    {
        int mapSize = (int)attMap.size();
        int index = 0;
        keyStr = "";
        valueStr = "";
        typedef std::map<std::string, std::string>::iterator it_type;
        for (it_type iterator = attMap.begin(); iterator != attMap.end(); iterator++)
        {
            keyStr += iterator->first;
            valueStr += iterator->second;
            if (index < (mapSize - 1))
            {
                keyStr += linker;
                valueStr += linker;
            }
            index += 1;
        }
    }

private:
    RooStats::ModelConfig *m_mc;
    RooWorkspace *m_comb;
    // RooAbsCategoryLValue* m_cat;
    RooCategory *m_cat;
    RooSimultaneous *m_pdf;
    RooDataSet *m_data;
    // RooRealVar* m_poi;
    RooArgSet m_poi;
    const RooArgSet *m_nuis;
    const RooArgSet *m_gobs;
    TList *m_dataList;
    int numChannels;

    std::vector<std::string> m_fixNuis;

    /* take these from the combined workspace */
    std::vector<int> m_useIndice;
    int useNumChannels;

    std::string splittedFile_;
    std::string replaceStr_;

    RooStats::ModelConfig *m_subMc;
    RooWorkspace *m_subComb;
    RooCategory *m_subCat;
    RooSimultaneous *m_subPdf;
    RooDataSet *m_subData;
    RooArgSet m_subNuis;
    RooArgSet m_subGobs;
    RooArgSet m_subObs;
    RooArgSet m_subPoi;
    RooArgSet m_subObsAndWgt;
    std::map<std::string, RooAbsPdf *> m_subPdfMap;
    std::map<std::string, RooDataSet *> m_subDataMap;
};
