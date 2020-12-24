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

using namespace RooStats;
using namespace RooFit;
using namespace std;

TString combiner::DUMMY = "dummy";
TString combiner::WGTNAME = "_weight_";

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
                delete obj;
        TList::Clear("nodelete");
    }
};

combiner::combiner() : m_wsName("combWS"),
                       m_mcName("ModelConfig"),
                       m_dataName("combData"),
                       m_pdfName("combPdf"),
                       m_catName("combCat"),
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

    TXMLNode *node = rootNode->GetChildren();

    while (node != 0)
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
                    auxUtil::alertAndAbort(Form("Combined POI %s is duplicated. Please double check the XML", poi.name.Data()));
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
        node = node->GetNextNode();
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

    /* walk through children list */
    TXMLNode *node = rootNode->GetChildren();

    while (node != 0)
    {
        if (node->GetNodeName() == TString("RenameMap"))
        {
            TXMLNode *subNode = node->GetChildren();
            vector<TString> NPSanity;

            while (subNode != 0)
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
                        if(m_strictMode) throw std::runtime_error("Flawed XML");
                    }
                    if (newName == "")
                    {
                        spdlog::warn("New object name is empty. Please check your config file!!!");
                        if(m_strictMode) throw std::runtime_error("Flawed XML");
                    }
                }

                /* add them to map */
                if (oldName != "" && newName != "")
                {
                    if (auxUtil::getItemType(oldName) == auxUtil::PDF)
                    {
                        /* only require OldName has all these components, NewName can be only a nuis parameter name, since they should follow the same format */
                        if (auxUtil::getItemType(newName) == auxUtil::PDF)
                            auxUtil::alertAndAbort(Form("Error processing %s: Users are only supposed to provide a nuisance parameter name", newName.Data()));
                        if(find(NPSanity.begin(), NPSanity.end(), newName) != NPSanity.end())
                            auxUtil::alertAndAbort(Form("New NP %s is duplicated in channel %s. Please double check the XML file", newName.Data(), channel.name_.Data()));                            
                        newName = Form("%s_Pdf(%s, %s_In)", newName.Data(), newName.Data(), newName.Data());
                        if(channel.pdfMap_.find(oldName) != channel.pdfMap_.end())
                            auxUtil::alertAndAbort(Form("Old PDF %s is duplicated in channel %s. Please double check the XML file", oldName.Data(), channel.name_.Data()));
                        channel.pdfMap_[oldName] = newName;
                        NPSanity.push_back(newName);
                    }
                    else if (TString(oldName).BeginsWith("alpha_"))
                    {
                        /* histfactory style for normal nuisance parameters(except lumi) */
                        oldName = Form("%sConstraint(%s, nom_%s)", oldName.Data(), oldName.Data(), oldName.Data());
                        if (auxUtil::getItemType(newName) == auxUtil::PDF)
                            auxUtil::alertAndAbort(Form("Error processing %s: Users are only supposed to provide a nuisance parameter name", newName.Data()));
                        if(find(NPSanity.begin(), NPSanity.end(), newName) != NPSanity.end())
                            auxUtil::alertAndAbort(Form("New NP %s is duplicated in channel %s. Please double check the XML file", newName.Data(), channel.name_.Data()));                            
                        newName = Form("%s_Pdf(%s, %s_In)", newName.Data(), newName.Data(), newName.Data());
                        if(channel.pdfMap_.find(oldName) != channel.pdfMap_.end())
                            auxUtil::alertAndAbort(Form("Old PDF %s is duplicated in channel %s. Please double check the XML file", oldName.Data(), channel.name_.Data()));                        
                        channel.pdfMap_[oldName] = newName;
                        NPSanity.push_back(newName);
                    }
                    else
                    {
                        if(channel.varMap_.find(oldName) != channel.varMap_.end())
                            auxUtil::alertAndAbort(Form("Old NP %s is duplicated in channel %s. Please double check the XML file", oldName.Data(), channel.name_.Data()));
                        if(find(NPSanity.begin(), NPSanity.end(), newName) != NPSanity.end())
                            auxUtil::alertAndAbort(Form("New NP %s is duplicated in channel %s. Please double check the XML file", newName.Data(), channel.name_.Data()));
                        channel.varMap_[oldName] = newName;
                        NPSanity.push_back(newName);
                    }
                }
                subNode = subNode->GetNextNode();
            }
        }

        if (node->GetNodeName() == TString("POIList"))
        {
            TString poiStr = auxUtil::getAttributeValue(node, "Input");
            std::vector<TString> poiList = auxUtil::splitString(poiStr, ',');
            const int nPOI = poiList.size();
            if (nPOI > m_pois.size())
            {
                auxUtil::alertAndAbort(Form("Channel %s has more POIs than combined model", channel.name_.Data()));
            }
            for (int ipoi = 0; ipoi < nPOI; ipoi++)
            {
                if(poiList[ipoi] != DUMMY && channel.poiMap_.find(poiList[ipoi]) != channel.poiMap_.end())
                    auxUtil::alertAndAbort(Form("POI %s is duplicated in channel %s. Please double check the XML file", poiList[ipoi].Data(), channel.name_.Data()));
                channel.poiMap_[poiList[ipoi]] = m_pois[ipoi].name;
            }
        }

        node = node->GetNextNode();
    }

    /* Check whether the file exists and whether the content is correct */
    /* Do not wait until a few hours later to find out that you made a typo in config file! */
    unique_ptr<TFile> inputFile(TFile::Open(channel.fileName_));
    if (!inputFile.get())
        auxUtil::alertAndAbort(Form("Input file %s for channel %s does not exist", channel.fileName_.Data(), channel.name_.Data()));

    RooWorkspace *w = dynamic_cast<RooWorkspace *>(inputFile->Get(channel.wsName_));
    if (!w)
        auxUtil::alertAndAbort(Form("Input file %s for channel %s does not contain workspace %s", channel.fileName_.Data(), channel.name_.Data(), channel.wsName_.Data()));

    RooStats::ModelConfig *mc = dynamic_cast<RooStats::ModelConfig *>(w->obj(channel.mcName_));
    if (!mc)
        auxUtil::alertAndAbort(Form("Input file %s for channel %s does not contain ModelConfig %s", channel.fileName_.Data(), channel.name_.Data(), channel.mcName_.Data()));

    RooDataSet *data = dynamic_cast<RooDataSet *>(w->data(channel.dataName_));
    if (!data)
        auxUtil::alertAndAbort(Form("Input file %s for channel %s does not contain RooDataSet %s", channel.fileName_.Data(), channel.name_.Data(), channel.dataName_.Data())); 
    inputFile->Close();

    m_summary.push_back(channel);
}

void combiner::combineWorkspace()
{
    auxUtil::printTitle("Combination started", "-");
    m_comb.reset(new RooWorkspace(m_wsName, m_wsName));

    RooRealVar weight(WGTNAME, "", 1.);

    RooCategory combCat(m_catName, m_catName);
    TString combPdfStr = "";

    unique_ptr<RooArgSet> combObs(new RooArgSet());
    unique_ptr<RooArgSet> combGlob(new RooArgSet());
    unique_ptr<RooArgSet> combNuis(new RooArgSet());

    map<string, RooAbsData *> dataMap;

    const int m_numChannel = (int)m_summary.size();
    for (int ich = 0; ich < m_numChannel; ich++)
    {
        TString channelName = m_summary[ich].name_;

        unique_ptr<TFile> inputFile(TFile::Open(m_summary[ich].fileName_));
        RooWorkspace *w = dynamic_cast<RooWorkspace *>(inputFile->Get(m_summary[ich].wsName_));
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

            RooArgSet obsAndWgt = *data->get();
            obsAndWgt.add(weight);
            RooDataSet *data_new = new RooDataSet(
                dataName,
                dataName,
                obsAndWgt,
                RooFit::Index(*cat),
                RooFit::Import(dataMap_tmp),
                RooFit::WeightVar(WGTNAME) /* actually just pass a name */
            );

            pdf = pdf_new;
            data = data_new;
        }

        /* let global observables fixed, and nuisances parameters float */
        RooStats::SetAllConstant(*mc->GetNuisanceParameters(), false);
        RooStats::SetAllConstant(*mc->GetGlobalObservables(), true);

        for (std::map<TString, TString>::iterator iterator = m_summary[ich].pdfMap_.begin(); iterator != m_summary[ich].pdfMap_.end(); iterator++)
        {
            TString pdfStr = iterator->second;
            TString pdfName, obsName, meanName;
            double sigma;
            auxUtil::deComposeGaus(pdfStr, pdfName, obsName, meanName, sigma);

            TString oldPdfStr = iterator->first;
            TString oldPdfName, oldObsName, oldMeanName;
            double oldSigma;
            auxUtil::deComposeGaus(oldPdfStr, oldPdfName, oldObsName, oldMeanName, oldSigma);

            /* if the old gaus pdf is not normal */
            if (fabs(oldSigma - 1) > auxUtil::epsilon)
                auxUtil::alertAndAbort(Form("Constraint pdf %s in workspace %s is not listed as a normal Gaussian. It hence cannot be correlated with other channels. Please double check your config & workspace to see whether this is intended", oldPdfStr.Data(), m_summary[ich].fileName_.Data()));

            /* change the nuis/gobs/pdf name here, do not wait rename later */
            RooAbsPdf *t_pdf = w->pdf(oldPdfName);
            if (!t_pdf)
            {
                spdlog::warn("No pdf {} in workspace {}. Skip it", oldPdfName.Data(), m_summary[ich].fileName_.Data());
                if(m_strictMode) throw std::runtime_error("Flawed input");
            }
            else
            {
                TString className = t_pdf->ClassName();
                if (className != "RooGaussian")
                {
                    spdlog::warn("Pdf {} is not RooGaussian. Only Gaussian is supported now. Skip it", pdfName.Data());
                    if(m_strictMode) throw std::runtime_error("Flawed input");
                    continue;
                }
                RooRealVar *t_nuis = w->var(oldObsName);
                if (!t_nuis)
                {
                    spdlog::warn("No nuisance parameter {} in workspace {}. Skip it", oldObsName.Data(), m_summary[ich].fileName_.Data());
                    if(m_strictMode) throw std::runtime_error("Flawed input");
                    continue;
                }
                RooRealVar *t_glob = w->var(oldMeanName);
                if (!t_glob)
                {
                    spdlog::warn("No global observable {} in workspace {}. Skip it", oldMeanName.Data(), m_summary[ich].fileName_.Data());
                    if(m_strictMode) throw std::runtime_error("Flawed input");
                    continue;
                }
                /* TODO: Also should check whether the sigma of the Gaussian is unity */
                auxUtil::renameAndAdd(t_pdf, pdfName, excludedPdfs);
                auxUtil::renameAndAdd(t_nuis, obsName, excludedVars);
                auxUtil::renameAndAdd(t_glob, meanName, excludedVars);
            }
        }

        /* Rename free floating NPs already now to keep track of them */
        for (std::map<TString, TString>::iterator iterator = m_summary[ich].varMap_.begin(); iterator != m_summary[ich].varMap_.end(); iterator++)
        {
            RooRealVar *arg = w->var(iterator->first);
            if (arg)
                auxUtil::renameAndAdd(arg, iterator->second, excludedVars);
            else
            {
                spdlog::warn("No variable {} in workspace {}. Skip it", iterator->first.Data(), m_summary[ich].fileName_.Data());
                if(m_strictMode) throw std::runtime_error("Flawed input");
            }
        }

        /* Rename POIs */
        for (std::map<TString, TString>::iterator iterator = m_summary[ich].poiMap_.begin(); iterator != m_summary[ich].poiMap_.end(); iterator++)
        {
            if (iterator->first == DUMMY)
                continue;
            RooRealVar *arg = w->var(iterator->first);
            if (arg)
                auxUtil::renameAndAdd(arg, iterator->second, excludedVars);
            else
                auxUtil::alertAndAbort(Form("No POI %s in workspace %s", iterator->first.Data(), m_summary[ich].fileName_.Data()));
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

        /* Split pdf */
        RooCategory *indivCat = (RooCategory *)&pdf->indexCat();

        /* split the original dataSet */
        TList *indivDataList = data->split(*indivCat, true);
        const int ncat = indivCat->numBins(0);

        combNuis->add(*mc->GetNuisanceParameters(), true);
        combGlob->add(*mc->GetGlobalObservables(), true);

        for (int icat = 0; icat < ncat; ++icat)
        {
            indivCat->setBin(icat);
            spdlog::info("sub-index --> {} {}", icat, indivCat->getLabel());

            RooAbsPdf *pdfi = pdf->getPdf(indivCat->getLabel());
            RooDataSet *datai = (RooDataSet *)(indivDataList->FindObject(indivCat->getLabel()));

            /* in case combining the same channel, it's good to differentiate them */
            if (!TString(pdfi->GetName()).EndsWith(channelName))
                pdfi->SetName((TString(pdfi->GetName()) + "_" + channelName));

            if (TString(pdfi->ClassName()) == "RooProdPdf")
            {
                /* strip those disconnected pdfs from minimization */
                // RooArgSet cPars = *pdfi->getParameters(datai);
                // RooArgSet dPars = cPars;
                // RooArgSet* constraints = pdfi->getAllConstraints(*datai->get(), cPars, true);
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
            }

            /* make category */
            TString type = Form("%s_%s_%s", m_catName.Data(), indivCat->getLabel(), channelName.Data());

            spdlog::info("\ttype --> {}", type.Data());

            combCat.defineType(type);

            /* Make pdf */
            m_comb->import(*pdfi, RecycleConflictNodes(), Silence());
            combPdfStr += (type + "=" + pdfi->GetName() + ",");

            /* make observables */
            unique_ptr<RooArgSet> indivObs(pdfi->getObservables(*datai));
            combObs->add(*indivObs);

            /* Make data */
            RooArgSet obsAndWgt(*indivObs, weight);
            RooDataSet dataNew_i(type + "_data", type + "_data", obsAndWgt, WeightVar(WGTNAME));

            for (int j = 0, nEntries = datai->numEntries(); j < nEntries; ++j)
            {
                *indivObs = *datai->get(j);
                double dataWgt = datai->weight();
                dataNew_i.add(obsAndWgt, dataWgt);
            }
            m_comb->import(dataNew_i);
            dataMap[type.Data()] = (RooDataSet *)m_comb->data(dataNew_i.GetName());
        }
        inputFile->Close();
    }

    /* Constructing combined pdf */
    m_comb->import(combCat);
    auxUtil::implementObj(m_comb.get(), Form("SIMUL::%s(%s, %s)", m_pdfName.Data(), m_catName.Data(), combPdfStr.Strip(TString::kTrailing, ',').Data()));

    combObs->add(combCat);
    RooArgSet obsAndWgt(*combObs, weight);

    RooDataSet combData(m_dataName, m_dataName, obsAndWgt, Index(combCat), Link(dataMap), WeightVar(WGTNAME));

    m_comb->import(combData);

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
        {
            spdlog::warn("POI {} cannot be found in combined model", item.name.Data());
            if(m_strictMode) throw std::runtime_error("Flawed XML");
        }
    }

    m_mc.reset(new ModelConfig(m_mcName, m_comb.get()));
    m_mc->SetWorkspace(*m_comb);

    m_mc->SetPdf(*m_comb->pdf(m_pdfName));
    m_mc->SetProtoData(*m_comb->data(m_dataName));
    m_mc->SetParametersOfInterest(poi);

    combNuis.reset(findArgSetIn(m_comb.get(), combNuis.get()));
    combGlob.reset(findArgSetIn(m_comb.get(), combGlob.get()));
    combObs.reset(findArgSetIn(m_comb.get(), combObs.get()));

    m_mc->SetNuisanceParameters(*combNuis);
    m_mc->SetGlobalObservables(*combGlob);
    m_mc->SetObservables(*combObs);

    m_comb->import(*m_mc);
    m_comb->importClassCode(); // Jared

    auxUtil::printTitle("Simple Summary", "~");
    spdlog::info("There are {} nuisances parameters:", m_mc->GetNuisanceParameters()->getSize());
    m_mc->GetNuisanceParameters()->Print();
    spdlog::info("There are {} global observables:", m_mc->GetGlobalObservables()->getSize());
    m_mc->GetGlobalObservables()->Print();
    spdlog::info("There are {} pois:\n", poi.getSize());
    m_mc->GetParametersOfInterest()->Print("v");
    auxUtil::printTitle("Combination finished", "-");
}

RooArgSet *combiner::findArgSetIn(RooWorkspace *w, RooArgSet *set)
{
    TString setName = set->GetName();
    set->setName("old");
    RooArgSet *inSet = (RooArgSet *)set->clone(setName);
    std::unique_ptr<TIterator> iter(set->createIterator());

    for (RooAbsArg *v = (RooAbsArg *)iter->Next(); v != 0; v = (RooAbsArg *)iter->Next())
    {
        RooAbsArg *inV = (RooAbsArg *)w->obj(v->GetName());
        if (!inV)
        {
            spdlog::warn("No variable {} in the workspace", v->GetName());
            if(m_strictMode) throw std::runtime_error("Flawed combination");
            continue;
        }
        inSet->add(*inV, kTRUE);
    }

    return inSet;
}

void combiner::makeSnapshots()
{
    spdlog::info("Saving pre-fit combined workspace to file {}_prefit.root", m_outputFileName.Data());
    unique_ptr<TFile> outputFile(TFile::Open(m_outputFileName + "_prefit.root", "recreate"));
    outputFile->cd();
    m_comb->Write();
    outputFile->Close();
    auxUtil::printTitle("Creating snapshot/Asimov", "-");

    if (m_asimovHandler->genAsimov())
        m_asimovHandler->generateAsimov(m_mc.get(), m_dataName);

    outputFile.reset(TFile::Open(m_outputFileName, "recreate"));
    outputFile->cd();
    m_comb->Write();
    outputFile->Close();
    spdlog::info("Saving final combined workspace to file {}", m_outputFileName.Data());
}
