/*
 * =====================================================================================
 *
 *       Filename:  combiner.cxx
 *
 *    Description:  Workspace Combinner
 *
 *        Version:  1.1
 *        Created:  05/17/2012 02:49:27 PM
 *       Revision:  12/27/20 during pandemic
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji (已经去工业界了，别联系), haoshuang.ji@cern.ch
 *                  Hongtao Yang (还没跑，可以联系), Hongtao.Yang@cern.ch
 *   Organization:  University of Wisconsin
 *                  Lawrence Berkeley National Lab
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
TString combiner::WSPOSTFIX = "_tmp";
TString combiner::TMPPOSTFIX = "_tmp.root";
TString combiner::RAWPOSTFIX = "_raw.root";
TString combiner::PDFNAME = "combPdf";
TString combiner::CATNAME = "combCat";
TString combiner::NUISNAME = "nuisanceParameters";
TString combiner::GLOBNAME = "globalObservables";
TString combiner::OBSNAME = "observables";
TString combiner::POINAME = "POI";
TString combiner::CONSTRPOSTFIX = "_Pdf";
TString combiner::GOPOSTFIX = "_In";

combiner::combiner(TString inputXMLFileName)
{
    m_asimovHandler.reset(new asimovUtil());
    m_wsName = "combWS";
    m_mcName = "ModelConfig";
    m_dataName = "combData";
    m_outputFileName = "combined.root";
    m_strictMode = false;
    m_numThreads = 1;
    readConfigXml(inputXMLFileName);  
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
    channel.simplifiedImport_ = auxUtil::to_bool(auxUtil::getAttributeValue(rootNode, "SimplifiedImport", true, "0"));

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

    RooSimultaneous *pdf = dynamic_cast<RooSimultaneous *>(mc->GetPdf());
    if (!pdf)
        auxUtil::alertAndAbort(Form("Input file %s for channel %s ModelConfig %s does not contain a valid RooSimultaneous PDF", channel.fileName_.Data(), channel.name_.Data(), channel.mcName_.Data()));

    RooDataSet *data = dynamic_cast<RooDataSet *>(w->data(channel.dataName_));
    if (!data)
        auxUtil::alertAndAbort(Form("Input file %s for channel %s does not contain RooDataSet %s", channel.fileName_.Data(), channel.name_.Data(), channel.dataName_.Data()));

    /* walk through children list */

    for (TXMLNode *node = rootNode->GetChildren(); node != 0; node = node->GetNextNode())
    {
        if (node->GetNodeName() == TString("RenameMap"))
        {
            TString inputFileName = auxUtil::getAttributeValue(node, "InputFile", true, "");
            if (inputFileName != "")
            {
                TDOMParser xmlparser;
                // reading in the file and parse by DOM
                auxUtil::parseXMLFile(&xmlparser, inputFileName);

                TXMLDocument *xmldoc = xmlparser.GetXMLDocument();
                TXMLNode *renameNode = xmldoc->GetRootNode();
                readRenameMap(channel, renameNode, w);
            }
            else
                readRenameMap(channel, node, w);
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

void combiner::rename_core()
{
    std::unique_lock<std::mutex> lk(m_mutex, std::defer_lock);
    while (m_idx < m_summary.size())
    {
        int ich = m_idx++;

        if (m_idx > m_summary.size())
        {
            break;
        }

        RooWorkspace *w = m_wArr[ich];
        Channel channel = m_summary[ich];
        TString channelName = channel.name_;
        spdlog::info("Renaming channel {} {}", ich, channelName.Data());

        RooStats::ModelConfig *mc = dynamic_cast<RooStats::ModelConfig *>(w->obj(channel.mcName_));
        RooDataSet *data = dynamic_cast<RooDataSet *>(w->data(channel.dataName_));
        RooSimultaneous *pdf = dynamic_cast<RooSimultaneous *>(mc->GetPdf());

        /* Bookkeeping */
        RooArgSet allVars = w->allVars();
        RooArgSet allPdfs = w->allPdfs();
        RooArgSet allFunc = w->allFunctions();

        unique_ptr<RooArgSet> tmpNuis(mc->GetNuisanceParameters()->snapshot());
        unique_ptr<RooArgSet> tmpGlob(mc->GetGlobalObservables()->snapshot());

        /* let global observables fixed, and nuisances parameters float */
        RooStats::SetAllConstant(*mc->GetNuisanceParameters(), false);
        RooStats::SetAllConstant(*mc->GetGlobalObservables(), true);

        /* TODO: check whether the support for HistFactory style lumi uncertainty is still needed */
        /* Removed for now */
        for (std::map<TString, TString>::iterator it = channel.pdfMap_.begin(); it != channel.pdfMap_.end(); it++)
        {
            if (!channel.simplifiedImport_)
                allPdfs.remove(*w->pdf(it->first));

            w->pdf(it->first)->SetName(it->second);
        }

        /* Rename free floating NPs already now to keep track of them */
        for (std::map<TString, TString>::iterator it = channel.varMap_.begin(); it != channel.varMap_.end(); it++)
        {
            if (!channel.simplifiedImport_)
                allVars.remove(*w->var(it->first));
            else
            {
                tmpNuis->remove(*w->var(it->first));
                tmpGlob->remove(*w->var(it->first));
            }
            w->var(it->first)->SetName(it->second);
        }

        /* Rename POIs */
        for (std::map<TString, TString>::iterator it = channel.poiMap_.begin(); it != channel.poiMap_.end(); it++)
        {
            if (it->second == DUMMY)
                continue;
            if (!channel.simplifiedImport_)
                allVars.remove(*w->var(it->second));

            w->var(it->second)->SetName(it->first);
        }

        if (channel.simplifiedImport_)
        {
            spdlog::info("Simplified import requested for channel {}. Will only rename constraint PDFs, nuisance parameters, and global observables.", channelName.Data());

            unique_ptr<TIterator> it(tmpGlob->createIterator());
            for (RooRealVar *arg = dynamic_cast<RooRealVar *>(it->Next()); arg != 0; arg = dynamic_cast<RooRealVar *>(it->Next()))
            {
                TString globName = arg->GetName();
                RooRealVar *var = w->var(globName);
                unique_ptr<TIterator> constrIt(var->clientIterator());
                var->SetName(globName + "_" + channelName);
                RooAbsPdf *constr = dynamic_cast<RooAbsPdf *>(constrIt->Next());
                if (!constr)
                    spdlog::warn("Global observable {} in channel {} does not have constraint PDF", globName.Data(), channelName.Data());
                TString constrName = constr->GetName();
                constr->SetName(constrName + "_" + channelName);
                if(constrIt->Next())
                    auxUtil::alertAndAbort(Form("Global observable %s in channel %s has more than one constraint PDF", globName.Data(), channelName.Data()));
            }

            it.reset(tmpNuis->createIterator());
            for (RooRealVar *arg = dynamic_cast<RooRealVar *>(it->Next()); arg != 0; arg = dynamic_cast<RooRealVar *>(it->Next()))
            {
                TString nuisName = arg->GetName();
                RooRealVar *var = w->var(nuisName);
                var->SetName(nuisName + "_" + channelName);
            }
        }
        else{
            /* Exclude observables */
            unique_ptr<RooArgSet> tmpObs(pdf->getObservables(*data));
            allVars.remove(*tmpObs);

            /* Rename everything */
            unique_ptr<TIterator> it(allFunc.createIterator());
            for (RooAbsArg *arg = dynamic_cast<RooAbsArg *>(it->Next()); arg != 0; arg = dynamic_cast<RooAbsArg *>(it->Next()))
            {
                TString curName = arg->GetName();
                arg->SetName(curName + "_" + channelName);
            }
            spdlog::debug("All functions in channel {} {} renamed", ich, channelName.Data());
            it.reset(allPdfs.createIterator());
            for (RooAbsArg *arg = dynamic_cast<RooAbsArg *>(it->Next()); arg != 0; arg = dynamic_cast<RooAbsArg *>(it->Next()))
            {
                TString curName = arg->GetName();
                arg->SetName(curName + "_" + channelName);
            }
            spdlog::debug("All PDFs in channel {} {} renamed", ich, channelName.Data());
            it.reset(allVars.createIterator());
            for (RooAbsArg *arg = dynamic_cast<RooAbsArg *>(it->Next()); arg != 0; arg = dynamic_cast<RooAbsArg *>(it->Next()))
            {
                TString curName = arg->GetName();
                arg->SetName(curName + "_" + channelName);
            }
            spdlog::debug("All variables in channel {} {} renamed", ich, channelName.Data());            
        }

        /* Rename key objects */
        TString oldCatName = pdf->indexCat().GetName();
        TString newCatName = CATNAME + "_" + channelName;

        pdf->SetName(PDFNAME + "_" + channelName);
        data->SetName(m_dataName + "_" + channelName);

        lk.lock();
        m_tmpWs->import(*pdf, RenameVariable(oldCatName, newCatName), RecycleConflictNodes(), Silence());
        m_tmpWs->import(*data, RenameVariable(oldCatName, newCatName));
        m_nuis->add(*mc->GetNuisanceParameters()->snapshot(), true);
        m_glob->add(*mc->GetGlobalObservables()->snapshot(), true);        
        spdlog::info("Channel {} {} finished", ich, channelName.Data());
        lk.unlock();
    }
}

void combiner::rename(bool saveTmpWs)
{
    auxUtil::printTitle("Renaming variables, PDFs, and functions", "-");
    m_tmpWs.reset(new RooWorkspace(m_wsName + WSPOSTFIX, m_wsName + WSPOSTFIX));
    m_nuis.reset(new RooArgSet());
    m_glob.reset(new RooArgSet());

    const int numChannels = m_summary.size();
    shared_ptr<TFile> fileArr[numChannels];

    for (int ich = 0; ich < numChannels; ich++)
    {
        fileArr[ich].reset(TFile::Open(m_summary[ich].fileName_));
        RooWorkspace *w = dynamic_cast<RooWorkspace *>(fileArr[ich]->Get(m_summary[ich].wsName_));
        m_wArr.push_back(w);
    }

    m_idx = 0; /* Reset counter */
    m_thread_ptrs.clear();
    
    for (unsigned i = 0; i < std::min(m_numThreads, numChannels); i++)
    {
        m_thread_ptrs.emplace_back(new std::thread(&combiner::rename_core, this));
        spdlog::info("  -> Processor thread #{} started!", i);
    }
    join();

    for (int ich = 0; ich < numChannels; ich++)
    {
        // TString channelName = m_summary[ich].name_;
        // RooStats::ModelConfig *mc = dynamic_cast<RooStats::ModelConfig *>(m_wArr[ich]->obj(m_summary[ich].mcName_));
        // RooAbsData *data = m_wArr[ich]->data(DATAPREFIX + channelName);
        // RooSimultaneous *pdf = dynamic_cast<RooSimultaneous *>(m_wArr[ich]->pdf(PDFPREFIX + channelName));
        fileArr[ich]->Close();
    }

    if (saveTmpWs)
    {
        /* Noticed that when running in concurrency mode, the hash table will sometimes be missing */
        /* Therefore force importing all variables in the set, whether it can be found or not */
        /* Hope this bug will be fixed in RooFit soon (enforce rehash?) */
        m_tmpWs->defineSet(NUISNAME, *m_nuis, true);
        m_tmpWs->defineSet(GLOBNAME, *m_glob, true);
        unique_ptr<TFile> outputFile(TFile::Open(m_outputFileName + TMPPOSTFIX, "recreate"));
        outputFile->cd();
        m_tmpWs->Write();
        outputFile->Close();
        spdlog::info("Saving temporary workspace to file {}", (m_outputFileName + TMPPOSTFIX).Data());
    }
}

void combiner::combine(bool readTmpWs, bool saveRawWs)
{
    auxUtil::printTitle("Combining likelihood and dataset", "-");
    if (readTmpWs)
    {
        spdlog::info("Reading file {}", (m_outputFileName + TMPPOSTFIX).Data());
        m_inputFile.reset(TFile::Open(m_outputFileName + TMPPOSTFIX));
        m_tmpWs.reset(dynamic_cast<RooWorkspace *>(m_inputFile->Get(m_wsName + WSPOSTFIX)));
        m_nuis.reset(m_tmpWs->set(NUISNAME)->snapshot());
        m_glob.reset(m_tmpWs->set(GLOBNAME)->snapshot());
    }

    m_comb.reset(new RooWorkspace(m_wsName, m_wsName));

    m_combCat.reset(new RooCategory(CATNAME, CATNAME));
    m_combPdf.reset(new RooSimultaneous(PDFNAME, PDFNAME, *m_combCat));

    m_obs.reset(new RooArgSet());

    for (int ich = 0; ich < m_summary.size(); ich++)
    {
        TString channelName = m_summary[ich].name_;

        m_curPdf = dynamic_cast<RooSimultaneous *>(m_tmpWs->pdf(PDFNAME + "_" + channelName));
        if (!m_curPdf)
            auxUtil::alertAndAbort(Form("PDF %s in channel %s is missing", (PDFNAME + "_" + channelName).Data(), channelName.Data()), "Combination error");
        m_curCat = (RooCategory *)&m_curPdf->indexCat();

        RooDataSet *data = dynamic_cast<RooDataSet *>(m_tmpWs->data(m_dataName + "_" + channelName));
        if (!data)
            auxUtil::alertAndAbort(Form("Dataset %s in channel %s is missing", (m_dataName + "_" + channelName).Data(), channelName.Data()), "Combination error");        
        /* split the original dataSet */
        m_curDataList = data->split(*m_curCat, true);

        auxUtil::printTitle("Channel " + channelName, "+");
        m_total = m_curCat->numBins(0);
        m_catNamePrefix = CATNAME + "_" + channelName;
        m_idx = 0;
        m_thread_ptrs.clear();

        for (unsigned i = 0; i < std::min(m_numThreads, m_total); i++)
        {
            m_thread_ptrs.emplace_back(new std::thread(&combiner::combine_core, this));
            spdlog::info("  -> Processor thread #{} started!", i);
        }
        join();
    }
    m_comb->import(*m_combPdf);
    spdlog::info("Combined pdf created");
    m_obs->add(*m_combCat);
    RooRealVar weight(WGTNAME, "", 1.);
    RooArgSet obsAndWgt(*m_obs, weight);

    RooDataSet combData(m_dataName, m_dataName, obsAndWgt, Index(*m_combCat), Import(m_dataMap), WeightVar(WGTNAME));

    m_comb->import(combData);
    spdlog::info("Combined dataset created");

    m_comb->defineSet(OBSNAME, *m_obs, true);
    m_comb->defineSet(GLOBNAME, *m_glob, true);
    m_comb->defineSet(NUISNAME, *m_nuis, true);

    if (m_inputFile.get())
        m_inputFile->Close();

    if (saveRawWs)
    {
        spdlog::info("Saving pre-fit combined workspace to file {}", (m_outputFileName + RAWPOSTFIX).Data());
        unique_ptr<TFile> outputFile(TFile::Open(m_outputFileName + RAWPOSTFIX, "recreate"));
        outputFile->cd();
        m_comb->Write();
        outputFile->Close();
    }
}

void combiner::combine_core()
{
    std::unique_lock<std::mutex> lk(m_mutex, std::defer_lock);

    while (m_idx < m_total)
    {
        int icat = m_idx++;
        if (m_idx > m_total)
        {
            break;
        }
        
        lk.lock();
        m_curCat->setBin(icat);
        TString catName = m_curCat->getLabel();
        TString type = m_catNamePrefix + "_" + catName;
        spdlog::info("Category --> {} {}", icat, catName.Data());
        spdlog::info("\tNew category name --> {}", type.Data());
        RooAbsPdf *pdfi = m_curPdf->getPdf(catName);
        RooDataSet *datai = (RooDataSet *)(m_curDataList->FindObject(catName));
        lk.unlock();
        
        /* Make data */
        unique_ptr<RooArgSet> indivObs(pdfi->getObservables(*datai));
        RooRealVar weight(WGTNAME, "", 1.);
        RooArgSet obsAndWgt(*indivObs, weight);

        lk.lock();
        RooDataSet *dataNew_i = new RooDataSet(type + "_data", type + "_data", obsAndWgt, WeightVar(WGTNAME));
        lk.unlock();
        
        for (int j = 0, nEntries = datai->numEntries(); j < nEntries; ++j)
        {
            *indivObs = *datai->get(j);
            double dataWgt = datai->weight();
            dataNew_i->add(obsAndWgt, dataWgt);
        }

        lk.lock();
        m_keep.Add(dataNew_i);
        
        m_combCat->defineType(type);
        m_combPdf->addPdf(*pdfi, type);
        m_dataMap[type.Data()] = dataNew_i;
        m_obs->add(*indivObs->snapshot(), true);
        lk.unlock();
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
            spdlog::warn("POI {} cannot be found in combined model", item.name.Data());
    }

    m_mc.reset(new ModelConfig(m_mcName, m_comb.get()));
    m_mc->SetWorkspace(*m_comb);

    m_mc->SetPdf(*m_comb->pdf(PDFNAME));
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
    spdlog::info("There are {} pois", poi.getSize());
    // m_mc->GetParametersOfInterest()->Print();
}

void combiner::finalize(bool readRawWs)
{
    auxUtil::printTitle("Creating ModelConfig & snapshot/Asimov", "-");
    if (readRawWs)
    {
        spdlog::info("Reading file {}", (m_outputFileName + RAWPOSTFIX).Data());
        m_inputFile.reset(TFile::Open(m_outputFileName + RAWPOSTFIX));
        m_comb.reset(dynamic_cast<RooWorkspace*>(m_inputFile->Get(m_wsName)));
        m_obs.reset(m_comb->set(OBSNAME)->snapshot());
        m_nuis.reset(m_comb->set(NUISNAME)->snapshot());
        m_glob.reset(m_comb->set(GLOBNAME)->snapshot());
    }

    makeModelConfig();
    /* Make snapshots */
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

void combiner::join()
{
    for (auto &thread : m_thread_ptrs)
    {
      if (thread->joinable())
        thread->join();
    }
}

void combiner::readRenameMap(Channel &channel, TXMLNode *node, RooWorkspace *w)
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

                TString NPName = itemList[0], GOName = itemList[1], pdfName = itemList[2];
                /* First check wether the old PDF exists. If not, simply ignore this line */
                if (!w->pdf(pdfName))
                {
                    spdlog::warn("Constraint PDF {} does not exist in workspace {}. Skip it", oldName.Data(), channel.fileName_.Data());
                    if (m_strictMode)
                        if (m_strictMode)
                            throw std::runtime_error("Flawed XML");
                    continue;
                }
                /* Then check wether the old PDF is Gaussian. If not, simply ignore this line */
                TString className = w->pdf(pdfName)->ClassName();
                if (className != "RooGaussian")
                {
                    spdlog::warn("Constraint PDF {} is not RooGaussian. Only Gaussian is supported now. Skip it", oldName.Data());
                    if (m_strictMode)
                        throw std::runtime_error("Flawed input");
                    continue;
                }
                if (!w->var(NPName))
                    auxUtil::alertAndAbort(Form("No nuisance parameter %s in workspace %s", NPName.Data(), channel.fileName_.Data()), "Input error");
                if (!w->var(GOName))
                    auxUtil::alertAndAbort(Form("No global observable %s in workspace %s", GOName.Data(), channel.fileName_.Data()), "Input error");
                /* Check whether sigma of the Gaussian is unity */
                unique_ptr<TIterator> iter(w->pdf(pdfName)->serverIterator());

                bool sigmaCheck = false, NPCheck = false, GOCheck = false;
                for (RooAbsReal *arg = (RooAbsReal *)iter->Next(); arg != 0; arg = (RooAbsReal *)iter->Next())
                {
                    if (NPName == arg->GetName())
                        NPCheck = true;
                    else if (GOName == arg->GetName())
                        GOCheck = true;
                    else if (fabs(arg->getVal() - 1) < auxUtil::epsilon)
                        sigmaCheck = true;
                }
                if (!sigmaCheck)
                    auxUtil::alertAndAbort(Form("Sigma of constraint PDF %s in workspace %s is not unity", pdfName.Data(), channel.fileName_.Data()), "Input error");
                if (!NPCheck)
                    auxUtil::alertAndAbort(Form("Nuisance parameter %s has no dependence on constraint PDF %s in workspace %s", NPName.Data(), pdfName.Data(), channel.fileName_.Data()), "Input error");
                if (!GOCheck)
                    auxUtil::alertAndAbort(Form("Global observable %s has no dependence on constraint PDF %s in workspace %s", GOName.Data(), pdfName.Data(), channel.fileName_.Data()), "Input error");
                /* only require OldName has all these components, NewName can be only a nuis parameter name, since they should follow the same format */
                if (auxUtil::getItemType(newName) == auxUtil::CONSTR)
                    auxUtil::alertAndAbort(Form("Error processing %s: Users are only supposed to provide a nuisance parameter name", newName.Data()), "XML error");
                if (find(NPSanity.begin(), NPSanity.end(), newName) != NPSanity.end())
                    auxUtil::alertAndAbort(Form("New NP %s is duplicated in channel %s. Please double check the XML file", newName.Data(), channel.name_.Data()), "XML error");
                NPSanity.push_back(newName);

                if (channel.pdfMap_.find(pdfName) != channel.pdfMap_.end())
                    auxUtil::alertAndAbort(Form("Old constraint PDF %s is duplicated in channel %s. Please double check the XML file", pdfName.Data(), channel.name_.Data()), "XML error");
                channel.pdfMap_[pdfName] = newName + CONSTRPOSTFIX;

                if (channel.varMap_.find(NPName) != channel.varMap_.end())
                    auxUtil::alertAndAbort(Form("Old nuisance parameter %s is duplicated in channel %s. Please double check the XML file", NPName.Data(), channel.name_.Data()), "XML error");
                channel.varMap_[NPName] = newName;

                if (channel.varMap_.find(GOName) != channel.varMap_.end())
                    auxUtil::alertAndAbort(Form("Old global observable %s is duplicated in channel %s. Please double check the XML file", GOName.Data(), channel.name_.Data()), "XML error");
                channel.varMap_[GOName] = newName + GOPOSTFIX;
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
