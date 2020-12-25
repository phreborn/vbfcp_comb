/*
 * =====================================================================================
 *
 *       Filename:  combiner.cxx
 *
 *    Description:  Workspace Combinner
 *
 *        Version:  1.1
 *        Created:  05/17/2012 02:49:27 PM
 *       Revision:  13/12/2020
 *       Compiler:  gcc
 *
 *         Author:  haoshuang.ji (已经去工业界了，别联系), haoshuang.ji@cern.ch
 *                  Hongtao Yang (还没跑，可以联系), Hongtao.Yang@cern.ch
 *   Organization:
 *
 * =====================================================================================
 */

#include "combiner.h"
#include "textTable.h"

using namespace RooStats;
using namespace RooFit;
using namespace std;

TString combiner::DUMMY = "dummy";
TString combiner::WGTNAME = "_weight_";
TString combiner::PDFPREFIX = "__PDF_TMPWS__";
TString combiner::DATAPREFIX = "__DATA_TMPWS__";
TString combiner::CATPREFIX = "__CAT_TMPWS__";
TString combiner::WSPOSTFIX = "_tmp";
TString combiner::TMPPOSTFIX = "_tmp.root";
TString combiner::RAWPOSTFIX = "_raw.root";

struct TOwnedList : public TList
{
    // A collection class for keeping TObjects for deletion.
    // TOwnedList is like TList with SetOwner(), but really deletes all objects, whether or not on heap.
    // This is a horrible hack to work round the fact that RooArgSet and RooDataSet objects have have IsOnHeap() false.
    TOwnedList() : TList() { SetOwner(); }
    virtual ~TOwnedList() { Clear(); }
    virtual void Clear(Option_t *option = "")
    {
        if (!option || strcmp(option, "nodelete") != 0)
            for (TIter it(this); TObject *obj = it();)
                SafeDelete(obj);
        TList::Clear("nodelete");
    }
};

combiner::combiner() : m_wsName("combWS"),
                       m_mcName("ModelConfig"),
                       m_dataName("combData"),
                       m_pdfName("combPdf"),
                       m_catName("combCat"),
                       m_nuisName("nuisanceParameters"),
                       m_globName("globalObservables"),
                       m_outputFileName("combined.root"),
                       m_strictMode(false)
{
    m_asimovHandler.reset(new asimovUtil());
}

void combiner::readConfigXml(TString filen)
{
    TDOMParser xmlparser;
    // reading in the file and parse by DOM
    auxUtil::parseXMLFile(&xmlparser, filen);

    TXMLDocument *xmldoc = xmlparser.GetXMLDocument();
    TXMLNode *rootNode = xmldoc->GetRootNode();

    m_wsName = auxUtil::getAttributeValue(rootNode, "WorkspaeName", true, m_wsName);
    m_mcName = auxUtil::getAttributeValue(rootNode, "ModelConfigName", true, m_mcName);
    m_dataName = auxUtil::getAttributeValue(rootNode, "DataName", true, m_dataName);
    m_outputFileName = auxUtil::getAttributeValue(rootNode, "OutputFile", true, m_outputFileName);
    m_strictMode = auxUtil::to_bool(auxUtil::getAttributeValue(rootNode, "StrictMode", true, m_strictMode));

    /* Create output dir, if needed */
    if (m_outputFileName.Contains('/'))
    {
        TString dirName = m_outputFileName(0, m_outputFileName.Last('/'));
        system("mkdir -vp " + dirName);
    }
    for (TXMLNode *node = rootNode->GetChildren(); node != 0; node = node->GetNextNode())
    {
        TString nodeName = node->GetNodeName();
        if (nodeName == "Asimov")
            m_asimovHandler->addEntry(node);

        else if (nodeName == "Channel")
            readChannel(node);
        else if (nodeName == "POIList")
        {
            TString poiStr = auxUtil::getAttributeValue(node, "Combined");
            vector<TString> poiList = auxUtil::splitString(poiStr, ',');
            for (auto poiStr : poiList)
            {
                if (poiStr == "")
                    continue;
                vector<TString> poiInfo = decomposeStr(poiStr, '~', auxUtil::SQUARE);
                POI poi;
                poi.name = poiInfo.back();
                if (find(m_pois.begin(), m_pois.end(), poi.name) != m_pois.end())
                    auxUtil::alertAndAbort(Form("Combined POI %s is duplicated. Please double check the XML", poi.name.Data()), "Duplication");
                switch (poiInfo.size())
                {
                case 2: /* Only central value provided: fix */
                    poi.value = poiInfo[0].Atof();
                    poi.min = 1;
                    poi.max = 0;
                    break;
                case 3: /* Only range provided */
                    poi.min = poiInfo[0].Atof();
                    poi.max = poiInfo[1].Atof();
                    if (fabs(poi.min - poi.max) < auxUtil::epsilon)
                    {
                        poi.value = poiInfo[0].Atof();
                        poi.min = 1;
                        poi.max = 0;
                    }
                    else
                        poi.value = poi.min - 1;
                    break;
                case 4: /* Central value and range provided: float */
                    poi.value = poiInfo[0].Atof();
                    poi.min = poiInfo[1].Atof();
                    poi.max = poiInfo[2].Atof();
                }
                m_pois.push_back(poi);
            }
        }
    }
    spdlog::info("XML file processing finished");
}

void combiner::readChannel(TXMLNode *rootNode)
{
    Channel channel;
    /* walk through the key node */

    channel.name_ = auxUtil::getAttributeValue(rootNode, "Name");
    channel.fileName_ = auxUtil::getAttributeValue(rootNode, "InputFile");
    channel.wsName_ = auxUtil::getAttributeValue(rootNode, "WorkspaceName");
    channel.mcName_ = auxUtil::getAttributeValue(rootNode, "ModelConfigName");
    channel.dataName_ = auxUtil::getAttributeValue(rootNode, "DataName");

    if (find(m_summary.begin(), m_summary.end(), channel.name_) != m_summary.end())
        auxUtil::alertAndAbort(Form("Channel %s has duplicated name of other channels", channel.name_.Data()), "XML error");
    /* Check whether the file exists and whether the content is correct */
    /* Do not wait until a few hours later to find out that you made a typo in config file! */
    m_inputFile.reset(TFile::Open(channel.fileName_));
    if (!m_inputFile.get())
        auxUtil::alertAndAbort(Form("Input file %s for channel %s does not exist", channel.fileName_.Data(), channel.name_.Data()));

    RooWorkspace *w = dynamic_cast<RooWorkspace *>(m_inputFile->Get(channel.wsName_));
    if (!w)
        auxUtil::alertAndAbort(Form("Input file %s for channel %s does not contain workspace %s", channel.fileName_.Data(), channel.name_.Data(), channel.wsName_.Data()));

    RooStats::ModelConfig *mc = dynamic_cast<RooStats::ModelConfig *>(w->obj(channel.mcName_));
    if (!mc)
        auxUtil::alertAndAbort(Form("Input file %s for channel %s does not contain ModelConfig %s", channel.fileName_.Data(), channel.name_.Data(), channel.mcName_.Data()));

    RooDataSet *data = dynamic_cast<RooDataSet *>(w->data(channel.dataName_));
    if (!data)
        auxUtil::alertAndAbort(Form("Input file %s for channel %s does not contain RooDataSet %s", channel.fileName_.Data(), channel.name_.Data(), channel.dataName_.Data()));

    /* walk through children list */

    for (TXMLNode *node = rootNode->GetChildren(); node != 0; node = node->GetNextNode())
    {
        if (node->GetNodeName() == TString("RenameMap"))
        {
            vector<TString> NPSanity;

            for (TXMLNode *subNode = node->GetChildren(); subNode != 0; subNode = subNode->GetNextNode())
            {
                TString oldName = "";
                TString newName = "";

                if (subNode->GetNodeName() == TString("Syst"))
                {
                    oldName = auxUtil::getAttributeValue(subNode, "OldName");
                    newName = auxUtil::getAttributeValue(subNode, "NewName");

                    if (oldName == "")
                    {
                        spdlog::warn("Old object name is empty. Please check your config file!!!");
                        if (m_strictMode)
                            throw std::runtime_error("Flawed XML");
                        continue;
                    }
                    if (newName == "")
                    {
                        spdlog::warn("New object name is empty. Please check your config file!!!");
                        if (m_strictMode)
                            throw std::runtime_error("Flawed XML");
                        continue;
                    }

                    /* add them to map */
                    if (auxUtil::getItemType(oldName) == auxUtil::CONSTR)
                    {
                        vector<TString> itemList = auxUtil::decomposeStr(oldName, ',', auxUtil::ROUND);
                        if (itemList.size() != 3)
                            auxUtil::alertAndAbort(Form("Wrong format for constraint pdf %s in channel %s", oldName.Data(), channel.name_.Data()), "XML error");
                        /* First check wether the old PDF exists. If not, simply ignore this line */
                        if (!w->pdf(itemList.back()))
                        {
                            spdlog::warn("Constraint PDF {} does not exist in workspace {}. Skip it", oldName.Data(), channel.fileName_.Data());
                            if (m_strictMode)
                                if (m_strictMode)
                                    throw std::runtime_error("Flawed XML");
                            continue;
                        }
                        /* Then check wether the old PDF is Gaussian. If not, simply ignore this line */
                        TString className = w->pdf(itemList.back())->ClassName();
                        if (className != "RooGaussian")
                        {
                            spdlog::warn("Constraint PDF {} is not RooGaussian. Only Gaussian is supported now. Skip it", oldName.Data());
                            if (m_strictMode)
                                throw std::runtime_error("Flawed input");
                            continue;
                        }
                        if (!w->var(itemList[0]))
                            auxUtil::alertAndAbort(Form("No nuisance parameter %s in workspace %s", itemList[0].Data(), channel.fileName_.Data()), "Input error");
                        if (!w->var(itemList[1]))
                            auxUtil::alertAndAbort(Form("No global observable %s in workspace %s", itemList[1].Data(), channel.fileName_.Data()), "Input error");
                        /* Check whether sigma of the Gaussian is unity */
                        unique_ptr<TIterator> iter(w->pdf(itemList.back())->serverIterator());
                        bool sigmaCheck = false, NPCheck = false, GOCheck = false;
                        for (RooAbsReal *arg = (RooAbsReal *)iter->Next(); arg != 0; arg = (RooAbsReal *)iter->Next())
                        {
                            if (itemList[0] == arg->GetName())
                                NPCheck = true;
                            else if (itemList[1] == arg->GetName())
                                GOCheck = true;
                            else if (fabs(arg->getVal() - 1) < auxUtil::epsilon)
                                sigmaCheck = true;
                        }
                        if (!sigmaCheck)
                            auxUtil::alertAndAbort(Form("Sigma of constraint PDF %s in workspace %s is not unity", itemList.back().Data(), channel.fileName_.Data()), "Input error");
                        if (!NPCheck)
                            auxUtil::alertAndAbort(Form("Nuisance parameter %s has no dependence on constraint PDF %s in workspace %s", itemList[0].Data(), itemList.back().Data(), channel.fileName_.Data()), "Input error");
                        if (!GOCheck)
                            auxUtil::alertAndAbort(Form("Global observable %s has no dependence on constraint PDF %s in workspace %s", itemList[1].Data(), itemList.back().Data(), channel.fileName_.Data()), "Input error");
                        /* only require OldName has all these components, NewName can be only a nuis parameter name, since they should follow the same format */
                        if (auxUtil::getItemType(newName) == auxUtil::CONSTR)
                            auxUtil::alertAndAbort(Form("Error processing %s: Users are only supposed to provide a nuisance parameter name", newName.Data()), "XML error");
                        if (find(NPSanity.begin(), NPSanity.end(), newName) != NPSanity.end())
                            auxUtil::alertAndAbort(Form("New NP %s is duplicated in channel %s. Please double check the XML file", newName.Data(), channel.name_.Data()), "XML error");
                        newName = Form("%s_Pdf(%s, %s_In)", newName.Data(), newName.Data(), newName.Data());
                        if (channel.pdfMap_.find(oldName) != channel.pdfMap_.end())
                            auxUtil::alertAndAbort(Form("Old constraint PDF %s is duplicated in channel %s. Please double check the XML file", oldName.Data(), channel.name_.Data()), "XML error");
                        channel.pdfMap_[oldName] = newName;
                        NPSanity.push_back(newName);
                    }
                    else
                    {
                        /* First check wether the old variable exists. If not, simply ignore this line */
                        if (!w->var(oldName))
                        {
                            spdlog::warn("Variable {} does not exist in workspace {}", oldName.Data(), channel.fileName_.Data());
                            if (m_strictMode)
                                if (m_strictMode)
                                    throw std::runtime_error("Flawed XML");
                            continue;
                        }
                        if (channel.varMap_.find(oldName) != channel.varMap_.end())
                            auxUtil::alertAndAbort(Form("Old NP %s is duplicated in channel %s. Please double check the XML file", oldName.Data(), channel.name_.Data()));
                        if (find(NPSanity.begin(), NPSanity.end(), newName) != NPSanity.end())
                            auxUtil::alertAndAbort(Form("New NP %s is duplicated in channel %s. Please double check the XML file", newName.Data(), channel.name_.Data()));
                        channel.varMap_[oldName] = newName;
                        NPSanity.push_back(newName);
                    }
                }
            }
        }

        if (node->GetNodeName() == TString("POIList"))
        {
            vector<TString> POISanity;
            TString poiStr = auxUtil::getAttributeValue(node, "Input");
            std::vector<TString> poiList = auxUtil::splitString(poiStr, ',');
            const int nPOI = poiList.size();
            if (nPOI > m_pois.size())
            {
                auxUtil::alertAndAbort(Form("Channel %s has more POIs than combined model", channel.name_.Data()));
            }
            for (int ipoi = 0; ipoi < nPOI; ipoi++)
            {
                if (poiList[ipoi] != DUMMY)
                {
                    if (!w->var(poiList[ipoi]))
                        auxUtil::alertAndAbort(Form("POI %s does not exist in channel %s. Please double check the XML file", poiList[ipoi].Data(), channel.name_.Data()));
                    if (find(POISanity.begin(), POISanity.end(), poiList[ipoi]) != POISanity.end())
                        auxUtil::alertAndAbort(Form("POI %s is duplicated in channel %s. Please double check the XML file", poiList[ipoi].Data(), channel.name_.Data()));
                    POISanity.push_back(poiList[ipoi]);
                }
                channel.poiMap_[m_pois[ipoi].name] = poiList[ipoi];
            }
            for (unsigned ipoi = nPOI; ipoi < m_pois.size(); ipoi++)
                channel.poiMap_[m_pois[ipoi].name] = DUMMY;
        }
    }
    m_inputFile->Close();

    m_summary.push_back(channel);
}

void combiner::rename(bool saveTmpWs)
{
    auxUtil::printTitle("Renaming variables, PDFs, and functions", "-");
    m_tmpWs.reset(new RooWorkspace(m_wsName + WSPOSTFIX, m_wsName + WSPOSTFIX));
    m_nuis.reset(new RooArgSet());
    m_glob.reset(new RooArgSet());

    const int m_numChannel = m_summary.size();
    TOwnedList tmpKeep;
    for (int ich = 0; ich < m_numChannel; ich++)
    {
        TString channelName = m_summary[ich].name_;

        m_inputFile.reset(TFile::Open(m_summary[ich].fileName_));
        unique_ptr<RooWorkspace> w(dynamic_cast<RooWorkspace *>(m_inputFile->Get(m_summary[ich].wsName_)));
        RooStats::ModelConfig *mc = dynamic_cast<RooStats::ModelConfig *>(w->obj(m_summary[ich].mcName_));
        RooDataSet *data = dynamic_cast<RooDataSet *>(w->data(m_summary[ich].dataName_));

        /* Bookkeeping */
        RooArgSet excludedVars, excludedPdfs;

        RooSimultaneous *pdf = dynamic_cast<RooSimultaneous *>(mc->GetPdf());
        /* what if it's not RooSimultaneous PDF??? */
        if (!pdf)
        {
            RooAbsPdf *pdf_s = mc->GetPdf();
            std::unique_ptr<RooCategory> cat(new RooCategory("adhocCat", "adhocCat"));
            cat->defineType(channelName);

            map<string, RooAbsPdf *> pdfMap_tmp;
            map<string, RooDataSet *> dataMap_tmp;

            /* make pdf */
            pdfMap_tmp[channelName.Data()] = pdf;
            /* make data */
            dataMap_tmp[channelName.Data()] = data;
            TString pdfName = pdf->GetName();
            TString dataName = data->GetName();

            RooSimultaneous *pdf_new = new RooSimultaneous(pdfName, pdfName, pdfMap_tmp, *cat);
            tmpKeep.Add(pdf_new);
            RooArgSet obsAndWgt = *data->get();
            RooRealVar weight(WGTNAME, "", 1.);
            obsAndWgt.add(weight);
            RooDataSet *data_new = new RooDataSet(
                dataName,
                dataName,
                obsAndWgt,
                RooFit::Index(*cat),
                RooFit::Import(dataMap_tmp),
                RooFit::WeightVar(WGTNAME) /* actually just pass a name */
            );
            tmpKeep.Add(data_new);
            pdf = pdf_new;
            data = data_new;
        }

        /* let global observables fixed, and nuisances parameters float */
        RooStats::SetAllConstant(*mc->GetNuisanceParameters(), false);
        RooStats::SetAllConstant(*mc->GetGlobalObservables(), true);

        /* TODO: check whether the support for HistFactory style lumi uncertainty is still needed */
        /* Removed for now */
        for (std::map<TString, TString>::iterator iterator = m_summary[ich].pdfMap_.begin(); iterator != m_summary[ich].pdfMap_.end(); iterator++)
        {
            vector<TString> pdfItemList = auxUtil::decomposeStr(iterator->second, ',', auxUtil::ROUND);
            TString pdfName = pdfItemList[2], obsName = pdfItemList[0], meanName = pdfItemList[1];

            vector<TString> oldPdfItemList = auxUtil::decomposeStr(iterator->first, ',', auxUtil::ROUND);
            TString oldPdfName = oldPdfItemList[2], oldObsName = oldPdfItemList[0], oldMeanName = oldPdfItemList[1];

            auxUtil::renameAndAdd(w->pdf(oldPdfName), pdfName, excludedPdfs);
            auxUtil::renameAndAdd(w->var(oldObsName), obsName, excludedVars);
            auxUtil::renameAndAdd(w->var(oldMeanName), meanName, excludedVars);
        }

        /* Rename free floating NPs already now to keep track of them */
        for (std::map<TString, TString>::iterator iterator = m_summary[ich].varMap_.begin(); iterator != m_summary[ich].varMap_.end(); iterator++)
            auxUtil::renameAndAdd(w->var(iterator->first), iterator->second, excludedVars);

        /* Rename POIs */
        for (std::map<TString, TString>::iterator iterator = m_summary[ich].poiMap_.begin(); iterator != m_summary[ich].poiMap_.end(); iterator++)
        {
            if (iterator->second == DUMMY)
                continue;
            auxUtil::renameAndAdd(w->var(iterator->second), iterator->first, excludedVars);
        }

        /* Exclude observables */
        std::unique_ptr<RooArgSet> tmpObs(pdf->getObservables(*data));
        excludedVars.add(*tmpObs);

        /* rename functions & pdfs, to avoid naming conflicts */
        RooArgSet everything = w->allFunctions();
        RooArgSet allPdfs = w->allPdfs();
        RooArgSet allVars = w->allVars();

        allPdfs.remove(excludedPdfs);
        allVars.remove(excludedVars);

        everything.add(allPdfs);
        everything.add(allVars);

        /* Rename everything */
        std::unique_ptr<TIterator> fiter(everything.createIterator());
        for (RooAbsReal *v = (RooAbsReal *)fiter->Next(); v != 0; v = (RooAbsReal *)fiter->Next())
            v->SetName((TString(v->GetName()) + "_" + channelName));

        pdf->SetName(PDFPREFIX + channelName);
        data->SetName(DATAPREFIX + channelName);
        RooCategory *cat = dynamic_cast<RooCategory *>(w->obj(pdf->indexCat().GetName()));
        TString oldCatName = cat->GetName();
        TString newCatName = CATPREFIX + channelName;
        cat->SetName(newCatName);

        m_tmpWs->import(*pdf, RecycleConflictNodes(), Silence());
        m_tmpWs->import(*data, RooFit::RenameVariable(oldCatName, newCatName));
        m_nuis->add(*mc->GetNuisanceParameters()->snapshot(), true);
        m_glob->add(*mc->GetGlobalObservables()->snapshot(), true);
        auxUtil::defineSet(m_tmpWs.get(), m_nuis.get(), m_nuisName);
        auxUtil::defineSet(m_tmpWs.get(), m_glob.get(), m_globName);
        m_inputFile->Close();
    }
    if (saveTmpWs)
    {
        unique_ptr<TFile> outputFile(TFile::Open(m_outputFileName + TMPPOSTFIX, "recreate"));
        outputFile->cd();
        m_tmpWs->Write();
        outputFile->Close();
    }
}

void combiner::combine(bool readTmpWs, bool saveRawWs)
{
    auxUtil::printTitle("Combining likelihood and dataset", "-");
    if (readTmpWs)
    {
        m_inputFile.reset(TFile::Open(m_outputFileName + TMPPOSTFIX));
        m_tmpWs.reset(dynamic_cast<RooWorkspace *>(m_inputFile->Get(m_wsName + WSPOSTFIX)));
        m_nuis.reset(const_cast<RooArgSet *>(m_tmpWs->set(m_nuisName)));
        m_glob.reset(const_cast<RooArgSet *>(m_tmpWs->set(m_globName)));
    }

    m_comb.reset(new RooWorkspace(m_wsName, m_wsName));

    RooCategory combCat(m_catName, m_catName);
    RooSimultaneous combPdf(m_pdfName, m_pdfName, combCat);

    m_obs.reset(new RooArgSet());
    RooRealVar weight(WGTNAME, "", 1.);
    map<string, RooDataSet *> dataMap;

    TOwnedList tmpKeep;
    const int m_numChannel = m_summary.size();
    for (int ich = 0; ich < m_numChannel; ich++)
    {
        TString channelName = m_summary[ich].name_;

        RooSimultaneous *pdf = dynamic_cast<RooSimultaneous *>(m_tmpWs->pdf(PDFPREFIX + channelName));
        RooDataSet *data = dynamic_cast<RooDataSet *>(m_tmpWs->data(DATAPREFIX + channelName));

        /* Split pdf */
        RooCategory *indivCat = (RooCategory *)&pdf->indexCat();

        /* split the original dataSet */
        TList *indivDataList = data->split(*indivCat, true);
        const int ncat = indivCat->numBins(0);

        auxUtil::printTitle("Channel " + channelName, "+");
        for (int icat = 0; icat < ncat; ++icat)
        {
            indivCat->setBin(icat);
            spdlog::info("Category --> {} {}", icat, indivCat->getLabel());

            RooAbsPdf *pdfi = pdf->getPdf(indivCat->getLabel());
            RooDataSet *datai = (RooDataSet *)(indivDataList->FindObject(indivCat->getLabel()));

            if (TString(pdfi->ClassName()) == "RooProdPdf")
            {
                /* strip those disconnected pdfs from minimization */
                std::unique_ptr<RooArgSet> cPars(pdfi->getParameters(datai));
                RooArgSet dPars = *cPars;
                RooArgSet *constraints = pdfi->getAllConstraints(*datai->get(), *cPars, true);

                std::unique_ptr<RooArgSet> disConstraints(pdfi->getAllConstraints(*datai->get(), dPars, false));
                disConstraints->remove(*constraints);

                RooProdPdf *prodPdf = dynamic_cast<RooProdPdf *>(pdfi);
                assert(prodPdf);
                RooArgSet baseComponents;
                auxUtil::getBasePdf(prodPdf, baseComponents);
                /* remove disconnected pdfs */
                baseComponents.remove(*disConstraints);

                TString newPdfName = TString(pdfi->GetName()) + "_deComposed";

                pdfi = new RooProdPdf(newPdfName, newPdfName, baseComponents);
                tmpKeep.Add(pdfi);
            }

            /* make category */
            TString type = Form("%s_%s_%s", m_catName.Data(), indivCat->getLabel(), channelName.Data());

            spdlog::info("\tNew category name --> {}", type.Data());

            combCat.defineType(type);
            combPdf.addPdf(*pdfi, type);

            /* make observables */
            unique_ptr<RooArgSet> indivObs(pdfi->getObservables(*datai));
            m_obs->add(*indivObs->snapshot(), true);

            /* Make data */
            RooArgSet obsAndWgt(*indivObs, weight);
            RooDataSet *dataNew_i = new RooDataSet(type + "_data", type + "_data", obsAndWgt, WeightVar(WGTNAME));

            for (int j = 0, nEntries = datai->numEntries(); j < nEntries; ++j)
            {
                *indivObs = *datai->get(j);
                double dataWgt = datai->weight();
                dataNew_i->add(obsAndWgt, dataWgt);
            }
            dataMap[type.Data()] = dataNew_i;
            tmpKeep.Add(dataNew_i);
        }
    }
    m_comb->import(combPdf);
    spdlog::info("Combined pdf created");
    m_obs->add(combCat);
    RooArgSet obsAndWgt(*m_obs, weight);

    RooDataSet combData(m_dataName, m_dataName, obsAndWgt, Index(combCat), Import(dataMap), WeightVar(WGTNAME));

    m_comb->import(combData);
    spdlog::info("Combined dataset created");

    /* Redefine the variable sets */
    m_nuis.reset(auxUtil::findArgSetIn(m_comb.get(), m_nuis.get(), m_strictMode));
    m_glob.reset(auxUtil::findArgSetIn(m_comb.get(), m_glob.get(), m_strictMode));
    m_obs.reset(auxUtil::findArgSetIn(m_comb.get(), m_obs.get(), m_strictMode));

    if (m_inputFile.get())
        m_inputFile->Close();

    makeModelConfig();

    if (saveRawWs)
    {
        spdlog::info("Saving pre-fit combined workspace to file {}", (m_outputFileName + RAWPOSTFIX).Data());
        unique_ptr<TFile> outputFile(TFile::Open(m_outputFileName + RAWPOSTFIX, "recreate"));
        outputFile->cd();
        m_comb->Write();
        outputFile->Close();
    }
}

void combiner::makeModelConfig()
{
    auxUtil::printTitle("Creating ModelConfig", "-");

    /* Organizing parameters */
    RooArgSet poi;
    TString varName = "";

    for (auto item : m_pois)
    {
        RooRealVar *poiVar = m_comb->var(item.name);
        if (poiVar)
        {
            if (item.min > item.max)
                poiVar->setConstant(true);
            else
            {
                poiVar->setRange(item.min, item.max);
                if (item.value > item.min && item.value < item.max)
                    poiVar->setVal(item.value);
            }
            poi.add(*poiVar);
        }
        else
            auxUtil::alertAndAbort(Form("POI %s cannot be found in combined model", item.name.Data()));
    }

    m_mc.reset(new ModelConfig(m_mcName, m_comb.get()));
    m_mc->SetWorkspace(*m_comb);

    m_mc->SetPdf(*m_comb->pdf(m_pdfName));
    m_mc->SetProtoData(*m_comb->data(m_dataName));
    m_mc->SetParametersOfInterest(poi);

    m_mc->SetNuisanceParameters(*m_nuis);
    m_mc->SetGlobalObservables(*m_glob);
    m_mc->SetObservables(*m_obs);

    m_comb->import(*m_mc);
    m_comb->importClassCode(); // Jared

    auxUtil::printTitle("Simple Summary", "~");
    spdlog::info("There are {} nuisances parameters", m_mc->GetNuisanceParameters()->getSize());
    // m_mc->GetNuisanceParameters()->Print();
    spdlog::info("There are {} global observables", m_mc->GetGlobalObservables()->getSize());
    // m_mc->GetGlobalObservables()->Print();
    spdlog::info("There are {} pois\n", poi.getSize());
    // m_mc->GetParametersOfInterest()->Print();
}

void combiner::makeSnapshots(bool readRawWs)
{
    auxUtil::printTitle("Creating snapshot/Asimov", "-");

    if (readRawWs)
    {
        m_inputFile.reset(TFile::Open(m_outputFileName + RAWPOSTFIX));
        m_comb.reset(dynamic_cast<RooWorkspace*>(m_inputFile->Get(m_wsName)));
        m_mc.reset(dynamic_cast<ModelConfig*>(m_comb->obj(m_mcName)));
    }

    if (m_asimovHandler->genAsimov())
        m_asimovHandler->generateAsimov(m_mc.get(), m_dataName);

    if (m_inputFile.get())
        m_inputFile->Close();
    unique_ptr<TFile> outputFile(TFile::Open(m_outputFileName, "recreate"));
    outputFile->cd();
    m_comb->Write();
    outputFile->Close();
    spdlog::info("Saving final combined workspace to file {}", m_outputFileName.Data());
}

void combiner::printSummary()
{
    int num = (int)m_summary.size();
    auxUtil::printTitle(Form("Input summary (%d channels)", num));
    for (int i = 0; i < num; i++)
        m_summary[i].Print();

    /* Print matrix of POI */
    auxUtil::printTitle("POI matrix");
    ofstream fout(m_outputFileName + "_POIMatrix.txt");
    TextTable t('-', '|', '+');
    t.add("Combined");
    for (int i = 0; i < num; i++)
        t.add(m_summary[i].name_.Data());
    t.endOfRow();

    for (auto poi : m_pois)
    {
        t.add(poi.name.Data());
        for (int i = 0; i < num; i++)
            t.add(m_summary[i].poiMap_[poi.name].Data());
        t.endOfRow();
    }
    fout << t;
    cout << t;
    fout.close();
}