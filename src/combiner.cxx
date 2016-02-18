/*
 * =====================================================================================
 *
 *       Filename:  combiner.h
 *
 *    Description:  Workspace combinner
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

#include "combiner.h"
#include "asimovUtils.h"
#include "TTree.h"
#include "RooHistFunc.h"
#include "RooDataHist.h"
#include "RooStats/HistFactory/ParamHistFunc.h"

using namespace RooStats;
using namespace RooFit;
using namespace std;

struct TOwnedList : public TList {
  // A collection class for keeping TObjects for deletion.
  // TOwnedList is like TList with SetOwner(), but really deletes all objects, whether or not on heap.
  // This is a horrible hack to work round the fact that RooArgSet and RooDataSet objects have have IsOnHeap() false.
  TOwnedList() : TList() { SetOwner(); }
  virtual ~TOwnedList()  { Clear(); }
  virtual void Clear (Option_t* option="") {
    if (!option || strcmp(option,"nodelete")!=0)
      for (TIter it(this); TObject* obj= it();) delete obj;
    TList::Clear("nodelete");
  }
};

combiner::combiner():
  m_wsName( "combWS" ),
  m_dataName( "combData" ),
  m_pdfName( "combPdf" ),
  // m_poiName( "mu" ),
  m_obsName( "combObs" ),
  m_nuisName( "combNuis" ),
  m_gObsName( "combGobs" ),
  m_catName( "combCat" ),
  m_outputFileName( "combination.root" ),
  m_editBR( false ),
  m_editPDF( false ),
  m_editRFV( false ),
  m_mkBonly( true ),
  m_comb( new RooWorkspace( m_wsName.c_str(), m_wsName.c_str() ) ),
  m_nuis( new RooArgSet( "nuis" ) ),
  m_gObs( new RooArgSet( "gObs" ) ),
  m_mc(0),
  m_tmpComb( new RooWorkspace( "tmpWs", "tmpWs" ) ),
  m_tmpNuis( new RooArgSet( "tmpNuis" ) ),
  m_tmpGobs( new RooArgSet( "tmpGobs" ) ),
  m_cat( new RooCategory( m_catName.c_str(), m_catName.c_str() ) )
{
  m_keep.SetOwner();
}

void combiner::write(std::string outputFile)
{
  if ( outputFile!="" ) { m_outputFileName = outputFile; }
  // m_comb->loadSnapshot("ucmles");

  {
    delete m_nuis; m_nuis = NULL;
    delete m_gObs; m_gObs = NULL;
    delete m_tmpNuis; m_tmpNuis = NULL;
    delete m_tmpGobs; m_tmpGobs = NULL;
  }

  m_comb->writeToFile( m_outputFileName.c_str() );

  std::string plotName = m_outputFileName;
  plotName = plotName.replace(plotName.find(".root"), 5, "")+"_obsData.pdf";
  asimovUtils::makePlots( *m_pdf, *m_data,  plotName);
}

void combiner::editBR(RooAbsPdf* pdf, RooWorkspace* w, std::map<std::string, std::string>& renameMap)
{

  /* edit the ATLAS_BR_** uncertainties */
  /* mu_BR_ZZ --> mu_BR_ZZ / (mu_BR_ZZ * BR_ZZ * (1+\sigma_ZZ)^{ATLAS_BR_ZZ}) */
  /* mu_BR_ZZ --> mu_BR_ZZ / (mu_BR_ZZ * (0) * (1+(1))^{ATLAS_BR_ZZ}) */
  // MH	W_BB 		W_TAUTAU	W_MUMU		W_SS		W_CC		W_TT		W_GG		W_GAMGAM	W_ZGAM		W_WW		W_ZZ		WIDTH
  std::string decayModes[]  =  {"bb", "tautau", "mumu", "ss", "cc", "to", "gl", "gamgam", "zgam", "WW", "ZZ"};
  /* patial widths, will read from txt file */
  double patialWidths[]  =  {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};

  /* pw uncertainties */
  /* all correlated */
  std::string decayPUItems[]  =  {"PU", "PU", "PU", "PU", "PU", "PU", "PU", "PU", "PU", "PU", "PU"};
  double decayPUDeltas[]  =  {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};

  /* some correlated */
  std::string decayTUItems[]  =  {"TU_ff", "TU_ff", "TU_ff", "TU_ff", "TU_ff", "TU_tt", "TU_gl", "TU_gg", "TU_zg", "TU_vv", "TU_vv"};
  double decayTUDeltas[]  =  {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};

  std::map<double, std::vector<double> > nominal;
  std::map<double, std::vector<double> > puUncert;
  std::map<double, std::vector<double> > tuUncert;
  std::vector<std::string> nominalTitles;
  std::vector<std::string> puUncertTitles;
  std::vector<std::string> tuUncertTitles;
  std::string nominalTxt = "/users/hji/work/trunk/wsCombiner_new/cfg/data/partialWidth/W_central.dat";
  std::string puUncertTxt = "/users/hji/work/trunk/wsCombiner_new/cfg/data/partialWidth/dW_par.dat";
  std::string tuUncertTxt = "/users/hji/work/trunk/wsCombiner_new/cfg/data/partialWidth/dW_TH.dat";

  readTxt(nominalTxt, nominalTitles, nominal);
  readTxt(puUncertTxt, puUncertTitles, puUncert);
  readTxt(tuUncertTxt, tuUncertTitles, tuUncert);

  /* the input file should have same numbers of entries */
  assert ( nominal.size()==puUncert.size() );
  assert ( nominal.size()==tuUncert.size() );
  int nItems = (int)nominalTitles.size();
  std::cout << "nItems: " << nItems << std::endl;
  assert ( nItems==13 );

  /* only has mass every 5GeV */
  double mass = double(int(m_mass)/5*5);

  /* they should have this mass!!! */
  assert ( puUncert.find(mass)!=puUncert.end() );
  assert ( tuUncert.find(mass)!=tuUncert.end() );
  for ( int  i= 0; i < 11; i++ ) {
    std::cout << "\tnominal: " <<nominal[mass][i+1] << ", pu: " << puUncert[mass][i+1] << ", tu: " << tuUncert[mass][i+1] << std::endl;
    patialWidths[i] =  nominal[mass][i+1]/nominal[mass][12];
    if ( patialWidths[i] == 0 ) {
      decayPUDeltas[i] = 0.;
      decayTUDeltas[i] = 0.;
    }
    else{
      decayPUDeltas[i] = puUncert[mass][i+1]/nominal[mass][i+1];
      decayTUDeltas[i] = tuUncert[mass][i+1]/nominal[mass][i+1];
    }
  }

  TString sumStr  =  "";
  TString normStr  =  "";

  /* all vars used in RooFormulaVar */
  std::vector<std::string> vars;
  RooArgSet* allVars = pdf->getVariables();

  RooArgSet newNuis;
  RooArgSet newGObs;

  std::string pwProduct  = "";
  /* loop over all decay modes */
  for ( int  i =  0; i < (int)(sizeof(decayModes)/sizeof(std::string)); i++ ) {
    std::string muVar = "mu_BR_";
    std::string decay = decayModes[i];
    muVar += decay;
    RooRealVar* var = dynamic_cast<RooRealVar*>(allVars->find(muVar.c_str()));
    TString editStr = "";
    TString editNormStr = "";

    /* this decay not in, like H->gluongluon */
    if (!var) {
      std::cout << "\t\tNot found: " << muVar << std::endl;
      muVar  =  "1";
    }
    else{
      std::cout << "\tFound: " << muVar << std::endl;
    }

    decay = (decay=="WW" || decay=="ZZ") ? "VV" : decay;
    std::string npName  =  std::string("ATLAS_BR_")+decay;
    RooRealVar* np = dynamic_cast<RooRealVar*>(allVars->find((npName).c_str()));

    /* so replace NP ATLAS_BR_ZZ with ATLAS_BR_ZZ[1] later if there is np */
    if ( np ) {
      renameMap[std::string("ATLAS_BR_")+decay+"_Pdf"]  =  "pwProduct"; /* pwProduct will be defined later */
      // renameMap[std::string("ATLAS_BR_")+decay]  =  "0";
      // renameMap[std::string("ATLAS_BR_")+decay+"_In"]  =  "0";
      np->setVal(0);
      np->setConstant(true);
      RooRealVar* go = dynamic_cast<RooRealVar*>(allVars->find((npName+"_In").c_str()));
      m_gObs->remove(*go, false, true);
      m_nuis->remove(*np, false, true);
    }

    /* has mu but no BR uncertainty */
    if ( var && !np ) {
      std::cout << "\tWARNING: ATLAS_BR_" << decay << " not implemented!!!" << std::endl;
      // editStr = TString::Format("mu_BR_%s * %.3f", decay.c_str(), patialWidths[i]);
      // vars.push_back(TString::Format("mu_BR_%s", decay.c_str()).Data());
    } else {
      /* make NPs of ATLAS_PW_*, ATLAS_PW_* */
      std::string puVar  = std::string("ATLAS_PW_")+decayPUItems[i];
      std::string tuVar  = std::string("ATLAS_PW_")+decayTUItems[i];
      if ( !(w->var(puVar.c_str())) ) {
	w->factory((std::string("RooGaussian::")+puVar+"_Pdf("+puVar+"[-5, 5], "+puVar+"_In[-5, 5], 1)").c_str());
	pwProduct  =  (pwProduct == "") ? puVar+"_Pdf" : pwProduct+","+puVar+"_Pdf";
	newGObs.add(*w->var((puVar+"_In").c_str()));
	newNuis.add(*w->var(puVar.c_str()));
      }
      if ( !(w->var(tuVar.c_str())) ) {
	w->factory((std::string("RooGaussian::")+tuVar+"_Pdf("+tuVar+"[-5, 5], "+tuVar+"_In[-5, 5], 1)").c_str());
	pwProduct  =  (pwProduct == "") ? tuVar+"_Pdf" : pwProduct+","+tuVar+"_Pdf";
	newGObs.add(*w->var((tuVar+"_In").c_str()));
	newNuis.add(*w->var(tuVar.c_str()));
      }

      editStr = TString::Format("%s * %.4f * (1 + %.4f)^(%s) * (1 + %.4f)^(%s)",
				muVar.c_str(), patialWidths[i], decayPUDeltas[i], puVar.c_str(), decayTUDeltas[i], tuVar.c_str());
      editNormStr = TString::Format("%s * %.4f",
				    muVar.c_str(), patialWidths[i]);
      vars.push_back(muVar);
      vars.push_back(puVar);
      vars.push_back(tuVar);
      sumStr  = (sumStr  == "") ? editStr : sumStr+" + "+editStr;
      normStr  = (normStr  == "") ? editNormStr : normStr+" + "+editNormStr;
    }
  }

  m_gObs->add(newGObs);
  m_nuis->add(newNuis);

  TString finalStr = TString::Format("expr::sumBR('(%s)/(%s)', ", sumStr.Data(), normStr.Data());
  int nVars  = (int)(vars.size());
  /* import first those components */
  int firstIndex  = 0;
  for ( int  i= 0; i < nVars; i++ ) {
    /* add firstIndex to deal with the case that the first element is 1 */
    if ( vars[i] == "1" ) {
      firstIndex++;
      continue;
    }
    else{
      firstIndex  ==  0;
    }
    /* already has this var */
    if ( finalStr.Contains(TString(vars[i].c_str())+",") ) { continue; }

    finalStr  = (i == firstIndex) ? finalStr+vars[i].c_str() : finalStr+","+vars[i].c_str() ;

    RooRealVar* var = dynamic_cast<RooRealVar*>(allVars->find(vars[i].c_str()));
    if ( var ) { w->import(*var); }
  }
  finalStr +=  ")";
  std::cout << "sumStr: " << finalStr << std::endl;

  finalStr = editRFVString(finalStr);

  w->factory(finalStr.Data());

  std::cout << "product: " << pwProduct << std::endl;
  w->factory(TString::Format("PROD::pwProduct(%s)", pwProduct.c_str()).Data());

  RooFormulaVar* sumBR  = dynamic_cast<RooFormulaVar*>(w->obj("sumBR"));


  /* replace mu_BR_ZZ with mu_BR_ZZ * PU * TU / sum */
  for ( int  i =  0; i < (int)(sizeof(decayModes)/sizeof(std::string)); i++ ){
    RooRealVar* var = dynamic_cast<RooRealVar*>(allVars->find((std::string("mu_BR_")+decayModes[i]).c_str()));
    // allVars->Print();
    if ( var ) {
      std::string puVar  = std::string("ATLAS_PW_")+decayPUItems[i];
      std::string tuVar  = std::string("ATLAS_PW_")+decayTUItems[i];
      w->import(*var);
      // TString reducedMu  = TString::Format("expr::%s_reduced('%s*(1+%.4f)^(%s)*(1+%.4f)^(%s)/sumBR', %s, %s, %s, sumBR)",
      //                                      var->GetName(), var->GetName(),
      //                                      decayPUDeltas[i], puVar.c_str(),
      //                                      decayTUDeltas[i], tuVar.c_str(),
      //                                      var->GetName(), puVar.c_str(), tuVar.c_str());
      TString reducedMu  = TString::Format("expr::%s_reduced('@0*(1+%.4f)^(@1)*(1+%.4f)^(@2)/@3', %s, %s, %s, sumBR)",
					   var->GetName(), decayPUDeltas[i], decayTUDeltas[i],
					   var->GetName(), puVar.c_str(), tuVar.c_str());

      w->factory(reducedMu.Data());
      renameMap[var->GetName()]  =  std::string(var->GetName())+"_reduced";
    }
  }
}

void combiner::editPDF(RooAbsPdf* pdf, RooWorkspace* w, std::map<std::string, std::string>& renameMap)
{

  TString sumStr  =  "";

  /* all vars used in RooFormulaVar */
  std::vector<std::string> vars;
  RooArgSet* allVars = pdf->getVariables();

  RooArgSet newNuis;
  RooArgSet newGObs;

  std::string prodModes[]  =  {
    "7_ggF", "7_VBF", "7_WH", "7_ZH", "7_ttH",
    "8_ggF", "8_VBF", "8_WH", "8_ZH", "8_ttH"
  };
  double prodModesPdfUncert[]  =  {
    (0.076+0.071)/2.,  (0.025+0.021)/2., (0.035+0.035)/2., (0.035+0.035)/2., (0.085+0.085)/2.,
    (0.075+0.069)/2.,  (0.026+0.028)/2., (0.035+0.035)/2., (0.035+0.035)/2., (0.078+0.078)/2.
  };
  std::string prodModesPdfUncertStr[]  =  {
    "0.34*pdf_Higgs_3 - 0.74*pdf_Higgs_2 - 0.55*pdf_Higgs_1 - 0.15*pdf_Higgs_4 + 0.057*pdf_Higgs_5",
    "0.92*pdf_Higgs_1 - 0.011*pdf_Higgs_2 - 0.32*pdf_Higgs_3 - 0.22*pdf_Higgs_4 + 0.062*pdf_Higgs_5",
    "0.9*pdf_Higgs_1 - 0.33*pdf_Higgs_2 + 0.17*pdf_Higgs_3 + 0.24*pdf_Higgs_4 + 0.068*pdf_Higgs_5",
    "0.89*pdf_Higgs_1 - 0.35*pdf_Higgs_2 + 0.24*pdf_Higgs_3 - 0.083*pdf_Higgs_4 - 0.11*pdf_Higgs_5",
    "0.24*pdf_Higgs_1 + 0.85*pdf_Higgs_2 + 0.46*pdf_Higgs_3 - 0.081*pdf_Higgs_4 + 0.033*pdf_Higgs_5",

    "0.34*pdf_Higgs_3 - 0.74*pdf_Higgs_2 - 0.55*pdf_Higgs_1 - 0.15*pdf_Higgs_4 + 0.057*pdf_Higgs_5",
    "0.92*pdf_Higgs_1 - 0.011*pdf_Higgs_2 - 0.32*pdf_Higgs_3 - 0.22*pdf_Higgs_4 + 0.062*pdf_Higgs_5",
    "0.9*pdf_Higgs_1 - 0.33*pdf_Higgs_2 + 0.17*pdf_Higgs_3 + 0.24*pdf_Higgs_4 + 0.068*pdf_Higgs_5",
    "0.89*pdf_Higgs_1 - 0.35*pdf_Higgs_2 + 0.24*pdf_Higgs_3 - 0.083*pdf_Higgs_4 - 0.11*pdf_Higgs_5",
    "0.24*pdf_Higgs_1 + 0.85*pdf_Higgs_2 + 0.46*pdf_Higgs_3 - 0.081*pdf_Higgs_4 + 0.033*pdf_Higgs_5"
  };

  std::string pwProduct  = "";

  /* create the new vars */
  for ( int j= 0; j < 5; j++ ) {
    std::string tuVar  = TString::Format("pdf_Higgs_%d", j+1).Data();
    w->factory((std::string("RooGaussian::")+tuVar+"_Pdf("+tuVar+"[-5, 5], "+tuVar+"_In[-5, 5], 1)").c_str());
    pwProduct  =  (pwProduct == "") ? tuVar+"_Pdf" : pwProduct+","+tuVar+"_Pdf";
    newGObs.add(*w->var((tuVar+"_In").c_str()));
    newNuis.add(*w->var(tuVar.c_str()));
  }
  m_gObs->add(newGObs);
  m_nuis->add(newNuis);

  std::cout << "product: " << pwProduct << std::endl;
  w->factory(TString::Format("PROD::pdfProduct(%s)", pwProduct.c_str()).Data());


  /* loop over all decay modes */
  for ( int  i =  0; i < (int)(sizeof(prodModes)/sizeof(std::string)); i++ ) {
    std::string muVar = "mu_XS";
    std::string prod = prodModes[i];
    muVar += prod;
    RooRealVar* var = dynamic_cast<RooRealVar*>(allVars->find(muVar.c_str()));
    TString editStr = "";

    /* this production mode is not in */
    if (!var) {
      std::cout << "\t\tNot found: " << muVar << std::endl;
      muVar  =  "1";
    }
    else{
      std::cout << "\tFound: " << muVar << std::endl;
    }

    prod = (prod=="7_ggF" || prod=="8_ggF") ? "ggH" : prod;
    prod = (prod=="7_VBF" || prod=="8_VBF") ? "qqH" : prod;
    prod = (prod=="7_WH" || prod=="8_WH") ? "VH" : prod;
    prod = (prod=="7_ZH" || prod=="8_ZH") ? "VH" : prod;
    prod = (prod=="7_ttH" || prod=="8_ttH") ? "ttH" : prod;
    std::string npName  =  std::string("pdf_Higgs_")+prod;
    RooRealVar* np = dynamic_cast<RooRealVar*>(allVars->find((npName).c_str()));

    /* so replace NP ATLAS_BR_ZZ with ATLAS_BR_ZZ[1] later if there is np */
    if ( np ) {
      renameMap[std::string("pdf_Higgs_")+prod+"_Pdf"]  =  "pdfProduct"; /* pwProduct will be defined later */
      // renameMap[npName]  =  "0"; /* can't do so, since RooProduct is np*something, if replace np with 0, the RooProduct will get crazy values */
      // renameMap[std::string("pdf_Higgs_")+prod+"_In"]  =  "0";
      np->setVal(0);
      np->setConstant(true);
      RooRealVar* go = dynamic_cast<RooRealVar*>(allVars->find((npName+"_In").c_str()));
      m_gObs->remove(*go, false, true);
      m_nuis->remove(*np, false, true);
    }

    /* has mu but no pdf uncertainty */
    if ( var && !np ) {
      std::cout << "\tWARNING: pdf_Higgs_" << prod << " not implemented!!!" << std::endl;
    }

    /* replace mu_BR_ZZ with mu_BR_ZZ * PU * TU */
    for ( int  j =  0; j < (int)(sizeof(prodModes)/sizeof(std::string)); j++ ){
      RooRealVar* var = dynamic_cast<RooRealVar*>(allVars->find((std::string("mu_XS")+prodModes[j]).c_str()));

      double uncert = prodModesPdfUncert[j];
      std::string uncertStr = prodModesPdfUncertStr[j];

      if ( var ) {
        w->import(*var);
        TString reducedMu  = TString::Format("expr::%s_modified('%s*(1+%.4f)^(%s)', %s, pdf_Higgs_1, pdf_Higgs_2, pdf_Higgs_3, pdf_Higgs_4, pdf_Higgs_5)",
                                             var->GetName(), var->GetName(), uncert, uncertStr.c_str(), var->GetName() );
        w->factory(reducedMu.Data());
        renameMap[var->GetName()]  =  std::string(var->GetName())+"_modified";
      }
    }
  } /* end loop for different production modes */
}



combiner::~combiner()
{
  delete m_comb;
  delete m_nuis;
  delete m_gObs;
  delete m_tmpComb;
  delete m_tmpNuis;
  delete m_tmpGobs;
  delete m_cat;
  delete m_mc;

}

void combiner::initWorkspace(std::string tmpFileName, keepTmp what)
{
  /* setup the output */
  {
    m_outputFileName = m_outSummary.fileName_;
    m_wsName = m_outSummary.wsName_;
    // m_mcName = m_outSummary.mcName_;
    /* pois */
    getPOIs(m_outSummary.poiName_, m_pois);

    int nPoi = m_pois.size();
    for ( int p= 0; p < nPoi; p++ ) {
      /* I need to split one mu into mu1*mu2 */
      if ( m_pois[p].name.find("*")!=std::string::npos ) {
        TString poiName = m_pois[p].name.c_str();
        poiName = poiName.ReplaceAll(" ", "");
        poiName = poiName.ReplaceAll("(", "[");
        poiName = poiName.ReplaceAll(")", "]");
        poiName = poiName.ReplaceAll("~", ","); /* range */
        TObjArray* strArray = poiName.Tokenize("*");
        int nPars = strArray->GetEntries();

        // editStr = "expr::mu('@0*@0',CV)";
        TString dummys = "";
        TString names = "";
        TString newPoiName = "";
        std::string tmpStr = "";
        for ( int i= 0; i < nPars; i++ ) {
          if (i==0) {
            names = ((TObjString*)strArray->At(i))->GetString();
            dummys = TString::Format("@%d", i);
            // newPoiName = ((TObjString*)strArray->At(i))->GetString();
            tmpStr = (((TObjString*)strArray->At(i))->GetString()).Data();
            std::string::size_type idx = 0;
            idx = tmpStr.find("[", 0);
            tmpStr = tmpStr.substr(0, idx);
            newPoiName = tmpStr.c_str();
          }
          else
	    {
	      names += ",";
	      names += ((TObjString*)strArray->At(i))->GetString();
	      dummys += TString::Format("*@%d", i);
	      // newPoiName += TString::Format("-%s", (((TObjString*)strArray->At(i))->GetString()).Data());
	      tmpStr = (((TObjString*)strArray->At(i))->GetString()).Data();
	      std::string::size_type idx = 0;
	      idx = tmpStr.find("[", 0);
	      tmpStr = tmpStr.substr(0, idx);
	      newPoiName += TString::Format("-X-%s", tmpStr.c_str());
	    }
        }

        newPoiName += "-Composite";
        m_pois[p].name = newPoiName;

        TString editStr = TString::Format("expr::%s('%s',%s)", m_pois[p].name.c_str(), dummys.Data(), names.Data());
        m_comb->factory(editStr.Data());

      }
    }
    m_dataName = m_outSummary.dataName_;
    m_mass = m_outSummary.mass_;
  }
  /* setup the input */
  m_numChannel = ( int )m_summary.size();

  if(what!=ReadTmp)
    {
      for ( int i = 0; i < m_numChannel; i++ )
	{
	  std::string channelName = m_summary[i].name_;
	  std::vector<POI> subPois;
	  getPOIs(m_summary[i].poiName_, subPois);


	  TFile* f = new TFile( m_summary[i].fileName_.c_str() );
	  m_keep.Add(f);

	  RooWorkspace* w = ( RooWorkspace* )( f->Get( m_summary[i].wsName_.c_str() ) );
	  m_keep.Add(w);
	  RooStats::ModelConfig* mc = ( RooStats::ModelConfig* )( w->obj( m_summary[i].mcName_.c_str() ) );
	  RooDataSet* data =  (RooDataSet*)( w->data( m_summary[i].dataName_.c_str() ) );
	  RooSimultaneous* pdf = (RooSimultaneous*)mc->GetPdf();
	  TOwnedList tmpKeep;

	  /* for llll */
	  {
	    RooRealVar* var = NULL;
	    var = w->var("mu_VBF"); if ( var ) { var->setVal(1); var->setConstant(); }
	    var = w->var("mu_VH"); if ( var ) { var->setVal(1); var->setConstant(); }
	    var = w->var("mu_ggH"); if ( var ) { var->setVal(1); var->setConstant(); }
	  }

	  /* what if it's not RooSimultaneous PDF??? */
	  if ( std::string(pdf->ClassName())!="RooSimultaneous" ) {
	    RooAbsPdf* pdf_s = mc->GetPdf();
	    RooCategory* cat = new RooCategory("adhocCat", "adhocCat");
	    cat->defineType( channelName.c_str() );

	    std::map<std::string, RooAbsPdf*> pdfMap;
	    std::map<std::string, RooDataSet*> dataMap;
	    /* make pdf */
	    pdfMap[channelName] = pdf;
	    /* make data */
	    dataMap[channelName] = data;
	    std::string pdfName = pdf->GetName();
	    std::string dataName = data->GetName();

	    RooSimultaneous* pdf_new = new RooSimultaneous( pdfName.c_str(), pdfName.c_str(), pdfMap, *cat);
	    tmpKeep.Add(pdf_new);

	    RooArgSet obsAndWgt = *data->get();
	    // RooRealVar* weightVar = new RooRealVar( "weightVar", "", 1., -1e10, 1e10 );
	    RooRealVar* weightVar = w->var("weightVar");
	    assert ( weightVar );
	    obsAndWgt.add( *weightVar );
	    RooDataSet* data_new = new RooDataSet(
						  dataName.c_str(),
						  dataName.c_str(),
						  obsAndWgt,
						  RooFit::Index( *cat ),
						  RooFit::Import( dataMap ),
						  RooFit::WeightVar( *weightVar ) /* actually just pass a name */
						  );
	    tmpKeep.Add(data_new);

	    pdf = pdf_new;
	    data = data_new;
	    delete cat;
	  }

	  RooSimultaneous* newPdf = asimovUtils::rebuildSimPdf(*data->get(), pdf);
	  tmpKeep.Add(newPdf);

	  /* let global observables fixed, and nuisances parameters float */
	  RooStats::SetAllConstant( *mc->GetNuisanceParameters(), false );
	  RooStats::SetAllConstant( *mc->GetGlobalObservables(), true );

	  /* used to change the name of global observables */
	  std::map<std::string, std::string> renamedMap = m_summary[i].renameMap_;

	  /* adhoc fix for llll workspace */
	  if ( channelName.find("llll_old")!=std::string::npos )
	    {
	      std::string duplicates[] =
		{ "lumiConstraint", "Lumi"};

	      std::string oldTopPdfName = pdf->GetName();
	      RooCustomizer cust(*pdf, channelName.c_str());

	      m_tmpComb->factory("_UNIT_[1]");
	      RooAbsArg* unit = (RooAbsArg*)m_tmpComb->obj("_UNIT_");
	      for ( int i= 0; i < (int)(sizeof(duplicates)/sizeof(std::string)); i++ ) {
		RooAbsArg* pdfi = (RooAbsArg*)w->obj(duplicates[i].c_str());
		if ( pdfi ) {
		  cust.replaceArg(*pdfi, *unit);
		}
	      }

	      /* new top pdf */
	      pdf = dynamic_cast<RooSimultaneous *>(cust.build());
	      pdf->SetName(oldTopPdfName.c_str());
	      tmpKeep.Add(pdf);

	      /* hard coded for llll */
	      {
		RooArgSet funcs = w->allPdfs();
		std::unique_ptr<TIterator> iter(funcs.createIterator());
		for ( RooAbsPdf* v = (RooAbsPdf*)iter->Next(); v!=0; v = (RooAbsPdf*)iter->Next() ) {
		  std::string name = v->GetName();
		  if (v->IsA()==RooRealSumPdf::Class()
		      &&(
			 name.find("H_2e2mu")!=std::string::npos
			 || name.find("H_2mu2e")!=std::string::npos
			 || name.find("H_4mu")!=std::string::npos
			 || name.find("H_4e")!=std::string::npos
			 )
		      )
		    {
		      v->forceNumInt(false);
		      std::cout << "\tTurn off forceNumIntegral For " << v->GetName() << std::endl;
		    }

		}
	      }
	    }

	  /* replace the whole pdf */
	  std::map<std::string, std::string> pdfRemap = m_summary[i].pdfReMap_;
	  typedef std::map<std::string, std::string>::iterator it_type;
	  for(it_type iterator = pdfRemap.begin(); iterator != pdfRemap.end(); iterator++) {
	    std::string pdfStr = iterator->second;
	    std::string pdfName, obsName, meanName;
	    double sigma;
	    deCompose(pdfStr, pdfName, obsName, meanName, sigma);

	    std::string oldPdfStr = iterator->first;
	    std::string oldPdfName, oldObsName, oldMeanName;
	    double oldSigma;
	    deCompose(oldPdfStr, oldPdfName, oldObsName, oldMeanName, oldSigma);
	    // Hack from Hongtao to understand better what is going on with vvqq
	    // std::cout<<pdfName<<" "<<obsName<<" "<<meanName<<std::endl;
	    // std::cout<<oldPdfName<<" "<<oldObsName<<" "<<oldMeanName<<std::endl;
	    // getchar();
	    /* if the old gaus pdf is not normal */
	    if ( fabs(oldSigma-1)>10e-4 ) {

	      /* let it be imported first to hold the place */
	      std::string edit = TString::Format("Gaussian::%s(%s[-5,5],%s[0,-5,5],1)", pdfName.c_str(), obsName.c_str(), meanName.c_str()).Data();
	      std::cout << "EDIT: " << edit << std::endl;
	      m_tmpComb->factory(edit.c_str());
	      m_tmpComb->var(meanName.c_str())->setConstant(); /* global observables */

	      /* special case for lumi in histfactory, transfer to lognormal */
	      if ( oldPdfName.find("lumiConstraint")!=std::string::npos ) {
		RooRealVar* lumi = m_tmpComb->var(obsName.c_str());
		assert ( lumi );
		vector<double> vals_up, vals_down;
		vals_up.push_back(1+oldSigma);
		vals_down.push_back(1/(1+oldSigma));
		RooArgList lumiList(*lumi);
		TString tag = TString::Format("lumi_fiv_%f", oldSigma);
		tag.ReplaceAll("0", "").ReplaceAll(".", "");
		RooStats::HistFactory::FlexibleInterpVar* lumi_fiv = new RooStats::HistFactory::FlexibleInterpVar(tag.Data(), tag.Data(), lumiList, 1., vals_down, vals_up);

		std::string oldTopPdfName = pdf->GetName();
		pdf->SetName((oldTopPdfName+"_old").c_str());

		RooCustomizer cust(*pdf, channelName.c_str());
		/* after rebuilding the pdf, the oldPdfName will have a tag "_llll" */
		oldPdfName = oldPdfName+"_"+channelName;

		RooRealVar* oldLumi = w->var(oldObsName.c_str());
		assert ( oldLumi );
		cust.replaceArg(*oldLumi, *lumi_fiv);
		/* new top pdf */
		pdf = dynamic_cast<RooSimultaneous *>(cust.build());
		pdf->SetName(oldTopPdfName.c_str());
		tmpKeep.Add(pdf);
	      }
	      m_summary[i].renameMap_[oldPdfName] = pdfName;
	      m_summary[i].renameMap_[oldObsName] = obsName;
	      m_summary[i].renameMap_[oldMeanName] = meanName;
	      renamedMap[oldPdfName] = pdfName;
	      renamedMap[oldObsName] = obsName;
	      renamedMap[oldMeanName] = meanName;

	    }else{
	      /* change the nuis/gobs/pdf name here, do not wait rename later */
	      RooAbsPdf* t_pdf = dynamic_cast<RooAbsPdf*>(w->pdf(oldPdfName.c_str()));
	      if ( !t_pdf ) {
		/* some channel does not have the same NPs for low and high masses, adding all of them could be easier */
		std::cout << "No Pdf: " << oldPdfName << std::endl;
	      }
	      else
		{
		  assert(t_pdf);
		  t_pdf->SetName(pdfName.c_str());

		  std::string className = t_pdf->ClassName();
		  if ( className!="RooGaussian" ) {
		    std::cout << "\tpdf " << pdfName << " is not RooGaussian, skip it!!!" << std::endl;
		    continue;
		  }

		  RooRealVar* t_var = dynamic_cast<RooRealVar*>(w->var(oldObsName.c_str()));
		  if ( !t_var ) {
		    std::cout << "\tNo obs: " << oldObsName << " in the workspace" << std::endl;
		    /* added later... */
		    continue;
		  }
		  assert(t_var);
		  t_var->SetName(obsName.c_str());

		  t_var = dynamic_cast<RooRealVar*>(w->obj(oldMeanName.c_str()));
		  if ( !t_var ) {
		    std::cout << "\tNo mean: " << oldMeanName << " in the workspace" << std::endl;
		  }
		  assert(t_var);
		  t_var->SetName(meanName.c_str());
		  renamedMap[pdfName] = pdfName;
		  renamedMap[obsName] = obsName;
		  renamedMap[meanName] = meanName;

		  // Hack from Hongtao: to understand better what is going on with the vvqq analysis
		  // std::cout<<pdfName<<" "<<obsName<<" "<<meanName<<std::endl;
		  // std::cout<<"Successfully implemented"<<std::endl;
		}
	    }
	  }

	  /* have to rename, otherwise can't find the right global observables */
	  /* if the variable name is to be changed, should also change it in individual global observables */
	  RooArgSet tmpNuis(*mc->GetNuisanceParameters());
	  tmpNuis.add(*mc->GetGlobalObservables());
	  std::unique_ptr<TIterator> iter(tmpNuis.createIterator());
	  for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() ) {
	    if ( renamedMap.find(v->GetName()) != renamedMap.end() ) {
	      std::string newName = renamedMap[v->GetName()];
	      std::cout << "Setting name: " << std::string(v->GetName()) << " -> " << newName << std::endl;
	      v->SetName(newName.c_str());
	    }else{
	      TString newName = v->GetName();
	      if ( !newName.EndsWith((std::string("_")+channelName).c_str()) ) {
		std::cout << "Setting name: " << std::string(v->GetName()) << " -> " << std::string(v->GetName())+"_"+channelName << std::endl;
		v->SetName((std::string(v->GetName())+"_"+channelName).c_str());
	      }
	    }
	  }

	  /* Need these to keep track of global/nuisances */
	  m_tmpGobs->add( *mc->GetGlobalObservables() );
	  m_tmpNuis->add( *mc->GetNuisanceParameters() );

	  /* Change also POI! Add Entry */
	  /* allow missing some in individual channels... */
	  if ( m_pois.size()<subPois.size() ) {
	    std::cout << "\tProblem with: " << m_summary[i].poiName_ << std::endl;
	    assert ( m_pois.size() >= subPois.size() );
	  }

	  int all = m_pois.size();
	  int sub = subPois.size();
	  for ( int t= 0; t < all; t++ ) {
	    if ( t>=sub ) {
	      POI tmp;
	      tmp.name = "dummy";
	      tmp.max = tmp.min = -999;
	      subPois.push_back(tmp);
	    }
	    m_summary[i].renameMap_[subPois[t].name+"_"+channelName] = m_pois[t].name;
	  }


	  /* rename functions & pdfs, in case two identical channels!!!! */
	  RooArgSet funcAndPdfs = w->allFunctions();

	  // /* bug in roostats */
	  // std::unique_ptr<TIterator> it(funcAndPdfs.createIterator());
	  // while ( RooAbsArg* v = (RooAbsArg*)it->Next() )
	  // {

	  //   int flexVarInterp = 4;
	  //   int piewVarInterp = 4;

	  //   FlexibleInterpVar* far = dynamic_cast<FlexibleInterpVar*>(v);
	  //   if ( far ) {
	  //     std::cout << "\t\tSet Flex::" << v->GetName() << " interpolation code to be : " << flexVarInterp << std::endl;
	  //     far->setAllInterpCodes(flexVarInterp);
	  //     continue;
	  //   }

	  //   PiecewiseInterpolation* var = dynamic_cast<PiecewiseInterpolation*>(v);
	  //   if ( var ) {
	  //     std::cout << "\t\tSet Piew::" << v->GetName() << " interpolation code to be : " << piewVarInterp << std::endl;
	  //     var->setAllInterpCodes(piewVarInterp);
	  //     continue;
	  //   }
	  // }


	  // funcAndPdfs.add(w->allPdfs());
	  RooArgSet renamePdfs = w->allPdfs();
	  typedef std::map<std::string, std::string>::iterator it_type;
	  for(it_type iterator = pdfRemap.begin(); iterator != pdfRemap.end(); iterator++) {
	    std::string pdfStr = iterator->second;
	    std::string pdfName, obsName, meanName;
	    double sigma;
	    deCompose(pdfStr, pdfName, obsName, meanName, sigma);
	    if ( RooAbsArg* pdf = (RooAbsArg*)renamePdfs.find(pdfName.c_str()) ) {
	      renamePdfs.remove(*pdf);
	    }
	  }
	  funcAndPdfs.add(renamePdfs);


	  RooArgSet otherVars = w->allVars();

	  /* this code still can't be used!!!!! */
	  if(m_editRFV){
	    /* take this opportunity to edit RooFormulaVar, replace the hard-coded function */
	    std::unique_ptr<TIterator> fVarIter(w->componentIterator());
	    for ( RooRealVar* v = (RooRealVar*)fVarIter->Next(); v!=0; v = (RooRealVar*)fVarIter->Next() ) {
	      if ( std::string(v->ClassName())=="RooFormulaVar" ) {
		std::cout << "\tRooFormulaVar: " << v->GetName() << std::endl;
		RooFormulaVar* oldVar = dynamic_cast<RooFormulaVar*>(w->obj(v->GetName()));
		assert ( oldVar );
		std::string varName = v->GetName();
		/* be careful if it's renamed, we put the renamed one in */
		/* we have map A->B, now changing to A', so add A'->B */
		for(it_type iterator = m_summary[i].renameMap_.begin(); iterator != m_summary[i].renameMap_.end(); iterator++) {
		  if ( iterator->first==(varName+"_"+channelName) ) {
		    varName = iterator->second;
		    break;
		  }
		}

		RooFormulaVarExt oldVarExt(*oldVar, "oldVarExtName");

		/* create a new one to hold the place */
		RooFormulaVar* newVar = NULL;
		TString strFormula = oldVarExt.RebuildStr(newVar, varName, false);
		RooArgList dependVars = oldVarExt.dependVars();
		std::unique_ptr<TIterator> dIter(dependVars.createIterator());
		while ( RooRealVar* d = (RooRealVar*)dIter->Next() ) {
		  std::string dName = d->GetName();
		  /* be careful if it's renamed, we put the renamed one in */
		  for(it_type iterator = m_summary[i].renameMap_.begin(); iterator != m_summary[i].renameMap_.end(); iterator++) {
		    if ( iterator->first==(dName+"_"+channelName) ) {
		      d->SetName((iterator->second).c_str());
		      /* below will add a tag->"_channelName" to poi, so we have to add a remap */
		      m_summary[i].renameMap_[iterator->second+"_"+channelName] = iterator->second;
		    }
		  }
		}

		/* don't need to change */
		if ( strFormula=="" ) {
		  std::cout << "\tNo need to change" << newVar->GetName() << std::endl;
		  continue;
		}

		/* if already in ws */
		newVar = dynamic_cast<RooFormulaVar*>(m_tmpComb->obj(varName.c_str()));
		if ( newVar ) {
		  std::cout << "\tWS already have " << newVar->GetName() << std::endl;
		  continue;
		}

		newVar = new RooFormulaVar(varName.c_str(), strFormula.Data(), dependVars);
		if ( newVar ) {
		  // newVar->Print();
		  m_tmpComb->import(*newVar);
		  // newVar->Print();
		  std::cout << "\tImported " << newVar->GetName() << " = " << strFormula << std::endl;
		}
	      }
	    }

	  }

	  /* global observables */
	  RooArgSet excludedVars = *mc->GetGlobalObservables();
	  /* nuisance parameters */
	  excludedVars.add(tmpNuis);
	  // excludedVars.add(*mc->GetParametersOfInterest());
	  /* observables */
	  // excludedVars.add(*pdf->getObservables(*data));
	  RooArgSet* tmpObs= pdf->getObservables(*data);
	  excludedVars.add(*tmpObs);
	  std::unique_ptr<TIterator> exIter(excludedVars.createIterator());
	  for ( RooRealVar* v = (RooRealVar*)exIter->Next(); v!=0; v = (RooRealVar*)exIter->Next() ) {
	    otherVars.remove(*v);
	  }
	  delete tmpObs;

	  otherVars.Print();

	  funcAndPdfs.add(otherVars);
	  std::unique_ptr<TIterator> fiter(funcAndPdfs.createIterator());
	  for ( RooAbsReal* v = (RooAbsReal*)fiter->Next(); v!=0; v = (RooAbsReal*)fiter->Next() ) {
	    v->SetName((std::string(v->GetName())+"_"+channelName).c_str());
	  }


	  std::string oldStr = "";
	  std::string newStr = "";
	  linkMap( m_summary[i].renameMap_, oldStr, newStr, "," );
	  std::cout << "oldStr: " << oldStr << std::endl;
	  std::cout << "newStr: " << newStr << std::endl;
	  // assert ( false );

	  pdf->SetName(( m_summary[i].name_ + "_pdf" ).c_str() );
	  data->SetName(( m_summary[i].name_ + "_data" ).c_str() );


	  m_tmpComb->import(
			    *pdf,
			    RooFit::RenameVariable( oldStr.c_str(), newStr.c_str() ),
			    RooFit::RecycleConflictNodes(),
			    RooFit::Silence()
			    );

	  m_tmpComb->import(
			    *data,
			    RooFit::RenameVariable( oldStr.c_str(), newStr.c_str() )
			    );


	  // f->Close();

	}

      m_tmpComb->defineSet(m_tmpNuis->GetName(), *m_tmpNuis, true);
      m_tmpComb->defineSet(m_tmpGobs->GetName(), *m_tmpGobs, true);
      if ( what==WriteTmp ) {
	m_tmpComb->writeToFile(tmpFileName.c_str());
      }
      // delete m_tmpComb; m_tmpComb = NULL;

    }
  /* only read the temporary ws */
  else
    {
      TFile* tmpFile = new TFile(tmpFileName.c_str());
      assert ( tmpFile );
      m_keep.Add(tmpFile);
      m_tmpComb = dynamic_cast<RooWorkspace*>(tmpFile->Get(m_tmpComb->GetName()));
      m_tmpNuis = new RooArgSet(*m_tmpComb->set(m_tmpNuis->GetName()));
      m_tmpGobs = new RooArgSet(*m_tmpComb->set(m_tmpGobs->GetName()));
    }
}

void combiner::regularizeWorkspace()
{
  RooArgSet poi;
  TString varName = "";
  for ( int ipoi= 0; ipoi < (int)m_pois.size(); ipoi++ ) {
    varName = m_pois[ipoi].name.c_str();
    if ( varName.Contains("-Composite") ) {
      varName = varName.ReplaceAll("-Composite", "");
      TObjArray* strArray = varName.Tokenize("-X-");
      int nPars = strArray->GetEntries();
      for ( int ipar= 0; ipar < nPars; ipar++ )
	{
	  std::string name = (((TObjString*)strArray->At(ipar))->GetString()).Data();
	  if ( RooRealVar* var = m_comb->var(name.c_str()) ) {
	    poi.add(*var);
	  }
	}
    }
    else if ( RooRealVar* var = m_comb->var(varName.Data()) ) {
      poi.add(*var);
      if(m_pois[ipoi].max == m_pois[ipoi].min){
	var->setVal(m_pois[ipoi].max);
	var->setConstant(true);
      }
    }
  }
  m_comb->defineSet( "poi", poi );


  m_mc = new ModelConfig( "ModelConfig", m_comb );
  m_mc->SetWorkspace(*m_comb);
  // m_mc->SetPdf( *m_comb->pdf( m_pdfName.c_str() ) );
  // m_mc->SetProtoData( *m_comb->data( m_dataName.c_str() ) );
  // m_mc->SetParametersOfInterest( *m_comb->set( "poi" ) );
  // m_mc->SetNuisanceParameters( *m_comb->set( m_nuisName.c_str() ) );
  // m_mc->SetGlobalObservables( *m_comb->set( m_gObsName.c_str() ) );
  // m_mc->SetObservables( *m_comb->set( m_obsName.c_str() ) );

  m_mc->SetPdf( *m_pdf );
  m_mc->SetProtoData( *m_data );
  m_mc->SetParametersOfInterest( poi );
  m_mc->SetNuisanceParameters( *m_nuis );
  m_mc->SetGlobalObservables( *m_gObs );
  m_mc->SetObservables( m_obs );

  m_comb->import( *m_mc );


  if ( m_mkBonly ) {
    if ( m_pdf->dependsOn(*m_mc->GetParametersOfInterest()->first()) )
      {
	m_comb->factory("_zero_[0]");
	RooCustomizer make_model_s(*m_mc->GetPdf(),"_model_bonly_");
	make_model_s.replaceArg(*m_mc->GetParametersOfInterest()->first(), *m_comb->var("_zero_"));


	RooAbsPdf *model_b = dynamic_cast<RooAbsPdf *>(make_model_s.build(true));
	model_b->SetName("_model_bonly_");
	m_comb->import(*model_b, RooFit::Silence());
	RooStats::ModelConfig* mc_bonly = new RooStats::ModelConfig(*m_mc);
	mc_bonly->SetPdf(*model_b);
	mc_bonly->SetName("ModelConfig_bonly");
	m_comb->import(*mc_bonly);
	delete mc_bonly;
	delete model_b;
      }
    else{
      assert ( false );
      // /* I don't understand how this can happen!!! */
      // RooSimultaneous *model_b = new RooSimultaneous(*m_pdf, "_model_bonly_");
      // m_comb->import(*model_b, RooFit::Silence());
      // RooStats::ModelConfig* mc_bonly = new RooStats::ModelConfig(*m_mc);
      // mc_bonly->SetPdf(*model_b);
      // mc_bonly->SetName("ModelConfig_bonly");
      // m_comb->import(*mc_bonly);
    }
  }

  /* H->ZZ */
  RooRealVar* mHiggs = m_comb->var("mHiggs");
  if ( mHiggs ) {
    mHiggs->setVal(m_mass);
    mHiggs->setConstant(1);
    std::cout << "\tFixing higgs mass to " << m_mass << std::endl;
  }

  RooArgSet* nuis = const_cast<RooArgSet*>(m_mc->GetNuisanceParameters()); nuis->sort();
  RooArgSet* gobs = const_cast<RooArgSet*>(m_mc->GetGlobalObservables()); gobs->sort();

  std::cout << "\t~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  std::cout << "\t~~~~~Simple Summary~~~~~" << std::endl;
  std::cout << "\t~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  printf("There are %d nuisances parameters:\n", m_mc->GetNuisanceParameters()->getSize());
  nuis->Print();
  std::cout << "\t~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  printf("There are %d global observables:\n", m_mc->GetGlobalObservables()->getSize());
  gobs->Print();
  std::cout << "\t~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  printf("There are %d pois:\n", poi.getSize());
  poi.Print("v");
  std::cout << "\t~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  printf("Pdf nominal value: %f\n", m_pdf->getVal());

}
void combiner::makeSimCategory()
{
  TOwnedList tmpKeep;
  /* loop over channel pdfs */
  for ( int index = 0; index < m_numChannel; index++ )
    {
      std::string channelName = m_summary[index].name_;
      RooSimultaneous* indivPdf = ( RooSimultaneous* ) m_tmpComb->pdf(( channelName + "_pdf" ).c_str() );
      RooDataSet* indivData = ( RooDataSet* ) m_tmpComb->data(( channelName + "_data" ).c_str() );
      assert( indivPdf );
      assert( indivData );
      /* loop over each channel's pdfs */
      RooAbsCategoryLValue* indivCat = ( RooAbsCategoryLValue* ) &indivPdf->indexCat();
      std::cout << "catName " << indivCat->GetName() << std::endl;
      /* split the original dataSet */
      TList* indivDataList = indivData->split( *indivCat, true );
      int n = indivCat->numBins( 0 );

      for ( int i = 0; i < n; ++i )
	{
	  std::cout << "\tsub-index --> " << i << std::endl;
	  indivCat->setBin( i );
	  RooDataSet* datai = ( RooDataSet* )( indivDataList->At( i ) );
	  RooAbsPdf* pdfi = indivPdf->getPdf( indivCat->getLabel() );

	  /* in case combining the same channel, it's good to differentiate them */
	  if ( !TString(datai->GetName()).EndsWith(channelName.c_str()) ) {
	    datai->SetName((std::string(datai->GetName())+"_"+channelName).c_str());
	  }
	  if ( !TString(pdfi->GetName()).EndsWith(channelName.c_str()) ) {
	    pdfi->SetName((std::string(pdfi->GetName())+"_"+channelName).c_str());
	  }

	  if ( std::string(pdfi->ClassName())=="RooProdPdf" ) {
	    /* strip those disconnected pdfs from minimization */
	    // RooArgSet cPars = *pdfi->getParameters(datai);
	    // RooArgSet dPars = cPars;
	    // RooArgSet* constraints = pdfi->getAllConstraints(*datai->get(), cPars, true);
	    RooArgSet* cPars = pdfi->getParameters(datai);
	    RooArgSet dPars = *cPars;
	    RooArgSet* constraints = pdfi->getAllConstraints(*datai->get(), *cPars, true);
	    delete cPars;
	    RooArgSet* disConstraints = pdfi->getAllConstraints(*datai->get(), dPars, false);
	    disConstraints->remove(*constraints);

	    RooProdPdf* prodPdf = dynamic_cast<RooProdPdf*>(pdfi);
	    assert ( prodPdf );
	    RooArgSet baseComponents;
	    getBasePdf(prodPdf, baseComponents);
	    /* remove disconnected pdfs */
	    baseComponents.remove(*disConstraints);
	    delete disConstraints;

	    std::string newPdfName = std::string(pdfi->GetName())+"_deComposed";
	    pdfi = new RooProdPdf(newPdfName.c_str(), newPdfName.c_str(), baseComponents);
	    tmpKeep.Add(pdfi);
	  }

	  /* make category */
	  TString type = Form( "%s_%s_%s", m_catName.c_str(), indivCat->getLabel(), m_summary[index].name_.c_str() );

	  std::cout << "\t\ttype --> " << type << std::endl;
	  m_cat->defineType( type );
	  /* make observables */
	  RooArgSet* indivObs = pdfi->getObservables( *indivData );

	  m_obs.add( *indivObs );

	  /* make nuisances */
	  RooArgSet* indivNuis = pdfi->getParameters( *indivObs );
	  delete indivObs;
	  std::unique_ptr<TIterator> iter( indivNuis->createIterator() );
	  for ( RooRealVar* v = ( RooRealVar* )iter->Next(); v != 0; v = ( RooRealVar* )iter->Next() )
	    {
	      /* set poi */
	      bool isPoi = false;
	      for ( int i= 0; i < int(m_pois.size()); i++ ) {
		if ( std::string(v->GetName())==m_pois[i].name )
		  {
		    if ( m_pois[i].max > 0 ) {
		      if ( m_pois[i].max == m_pois[i].min ) {
			v->setVal(m_pois[i].max);
			v->setConstant(true);
		      }
		      else{
			v->setMin(m_pois[i].min);
			v->setMax(m_pois[i].max);
		      }
		    }
		    isPoi = true;
		    break;
		  }
	      }
	      if ( isPoi ) { continue; }


	      /* in original global observables */
	      RooRealVar* obs = ( RooRealVar* )m_tmpGobs->find( v->GetName() );
	      if ( obs )
		{
		  m_gObs->add( *obs );
		  continue; // this one added as gobs
		}

	      /* float any way */
	      RooRealVar* nui = ( RooRealVar* )m_tmpNuis->find( v->GetName() );
	      if ( nui && !( nui->isConstant() ) )
		{
		  m_nuis->add( *v );
		}
	    }
	  delete indivNuis;

	  /* make pdf */
	  m_pdfMap[type.Data()] = pdfi;
	  /* make data */
	  m_dataMap[type.Data()] = datai;
	}
    }

  m_comb->import( *m_cat );
  // m_cat = (RooCategory*) m_comb->obj(m_cat->GetName());


  m_obs.add( *m_cat );
  /* make pdf */
  m_pdf = new RooSimultaneous(
			      m_pdfName.c_str(),
			      m_pdfName.c_str(),
			      m_pdfMap,
			      *m_cat
			      );

  std::map<std::string, std::string> brRenamedMap;
  if ( m_editBR ) {
    editBR(m_pdf, m_comb, brRenamedMap);
  }
  else{
  }


  if ( m_editPDF ) {
    editPDF(m_pdf, m_comb, brRenamedMap);
  }

  std::string oldStr = "";
  std::string newStr = "";
  linkMap( brRenamedMap, oldStr, newStr, "," );
  // std::cout << "oldStr: " << oldStr << std::endl;
  // std::cout << "newStr: " << newStr << std::endl;
  // assert ( false );

  m_comb->import( *m_pdf,
		  RooFit::RenameVariable( oldStr.c_str(), newStr.c_str() ),
		  RooFit::RecycleConflictNodes(),
		  RooFit::Silence() );

  // // use the version imported
  // m_pdf = ( RooSimultaneous* )m_comb->obj( m_pdf->GetName() );

  TString tmpName= m_pdf->GetName();
  delete m_pdf;

  // use the version imported
  m_pdf = ( RooSimultaneous* )m_comb->obj( tmpName );

  /* make data */
  m_obsAndWgt.add( m_obs );
  // RooAbsData::defaultStorageType = RooAbsData::Tree; // change to Tree then method 2 does not work
  int method = 0;

  if ( method == 0 )
    {
      /* method 0 */
      RooRealVar weight( "_weight_", "", 1. );
      m_obsAndWgt.add( weight );
      m_data = new RooDataSet( m_dataName.c_str(), m_dataName.c_str(), m_obsAndWgt, "_weight_" );

      /* loop over channel pdfs */
      for ( int index = 0; index < m_numChannel; index++ )
	{
	  std::string channelName = m_summary[index].name_;
	  RooSimultaneous* indivPdf = ( RooSimultaneous* ) m_tmpComb->pdf(( channelName + "_pdf" ).c_str() );
	  RooDataSet* indivData = ( RooDataSet* ) m_tmpComb->data(( channelName + "_data" ).c_str() );
	  assert( indivData );
	  RooAbsCategoryLValue* indivCat = ( RooAbsCategoryLValue* ) &indivPdf->indexCat();
	  /* split the original dataSet */
	  TList* indivDataList = indivData->split( *indivCat, true );
	  int n = indivCat->numBins( 0 );

	  /* loop over category */
	  for ( int i = 0; i < n; ++i )
	    {
	      indivCat->setBin( i );
	      RooAbsPdf* pdfi = indivPdf->getPdf( indivCat->getLabel() );
	      RooDataSet* datai = ( RooDataSet* )( indivDataList->At( i ) );

	      TString type = Form( "%s_%s_%s", m_catName.c_str(), indivCat->getLabel(), channelName.c_str() );
	      m_cat->setLabel( type, true ); // print error

	      RooArgSet *thisObsSet = pdfi->getObservables(*datai);

	      for ( int j = 0, nEntries = datai->numEntries(); j < nEntries; ++j )
		{
		  m_obs = *datai->get(j);

		  double dataWgt = datai->weight();

		  bool addGhost = true;
		  bool warn = false;
		  if ( addGhost && dataWgt==0 &&
		       (
			type.Contains("ATLAS_H_4mu") ||
			type.Contains("ATLAS_H_4e") ||
			type.Contains("ATLAS_H_2mu2e") ||
			type.Contains("ATLAS_H_2e2mu")
			)
		       ) {
		    RooRealVar* thisObs = dynamic_cast<RooRealVar*>(thisObsSet->first());
		    double mPoint = thisObs->getVal() ;
		    if ( fabs(mPoint-m_mass)<10 )
		      {
			dataWgt = pow(10., -9.);
			if(warn)std::cout << "\tWARNING --> obs: " << mPoint << ", weight = " << dataWgt << std::endl;
		      }
		  }

		  m_data->add( m_obs, dataWgt );
		}
	      delete thisObsSet;
	    }
	}
    }
  else if ( method == 1 )
    {
      RooRealVar weight( "_weight_", "", 1. );
      m_obsAndWgt.add( weight );
      RooArgSet obsPlusCats( m_obs );
      m_data = new RooDataSet( m_dataName.c_str(), m_dataName.c_str(), m_obsAndWgt, "_weight_" );

      /* loop over channel pdfs */
      for ( int index = 0; index < m_numChannel; index++ )
	{
	  RooSimultaneous* indivPdf = ( RooSimultaneous* ) m_tmpComb->pdf(( m_summary[index].name_ + "_pdf" ).c_str() );
	  RooDataSet* indivData = ( RooDataSet* ) m_tmpComb->data(( m_summary[index].name_ + "_data" ).c_str() );
	  assert( indivData );
	  RooAbsCategoryLValue* indivCat = ( RooAbsCategoryLValue* ) &indivPdf->indexCat();
	  obsPlusCats.add( *indivCat );

	  for ( int j = 0, nEntries = indivData->numEntries(); j < nEntries; ++j )
	    {
	      obsPlusCats = *indivData->get( j );
	      TString type = Form( "%s_%s_%s", m_catName.c_str(), obsPlusCats.getCatLabel( indivCat->GetName() ), m_summary[index].name_.c_str());
	      m_cat->setLabel( type, true ); // print error
	      m_data->add( m_obs, indivData->weight() );
	    }
	}
    }
  else
    {
      /* method 2, works only if the weight name in given data is weightVar */
      RooRealVar* weightVar = new RooRealVar( "weightVar", "", 1., -1e10, 1e10 );
      m_obsAndWgt.add( *weightVar );
      m_data = new RooDataSet(
			      m_dataName.c_str(),
			      m_dataName.c_str(),
			      m_obsAndWgt,
			      RooFit::Index( *m_cat ),
			      RooFit::Import( m_dataMap ),
			      RooFit::WeightVar( *weightVar ) /* actually just pass a name */
			      );
    }

  m_comb->import( *m_data );
  tmpName= m_data->GetName();
  delete m_data;
  m_data = ( RooDataSet* )m_comb->data( tmpName );

  // std::string plotName = m_outputFileName;
  // plotName = plotName.replace(plotName.find(".root"), 5, "")+"_obsData.pdf";
  // makePlots( *m_pdf, *m_data,  plotName);

  /* should use those already in combined workspace */
  findArgSetIn( m_comb, &m_obs );
  findArgSetIn( m_comb, m_gObs );
  findArgSetIn( m_comb, m_nuis );



  m_comb->defineSet( m_obsName.c_str(), m_obs );
  m_comb->defineSet( m_gObsName.c_str(), *m_gObs );
  RooStats::RemoveConstantParameters( m_nuis );
  RooStats::SetAllConstant( *m_gObs );
  m_comb->defineSet( m_nuisName.c_str(), *m_nuis );
  std::cout << "Leaving makeSimCategory()" << std::endl;
}
void combiner::findArgSetIn( RooWorkspace* w, RooArgSet* set )
{
  std::string setName = set->GetName();
  set->setName( "old" );
  RooArgSet* inSet = ( RooArgSet* )set->clone( setName.c_str() );
  std::unique_ptr<TIterator> iter( set->createIterator() );

  for ( RooAbsArg* v = ( RooAbsArg* )iter->Next(); v != 0; v = ( RooAbsArg* )iter->Next() )
    {
      RooAbsArg* inV = ( RooAbsArg* )w->obj( v->GetName() );
      if ( !inV ) {
	std::cout << "\tNo var " << v->GetName() << " in the workspace" << std::endl;
	// w->Print();
      }
      assert( inV );
      inSet->add( *inV, kTRUE );
    }

  set = inSet;
}
void combiner::readConfigXml( std::string filen )
{
  std::cout << "Parsing file: " << filen << endl;
  TDOMParser xmlparser;
  // reading in the file and parse by DOM
  Int_t parseError = xmlparser.ParseFile( filen.c_str() );

  if ( parseError )
    {
      std::cout << "Loading of xml document \"" << filen
		<< "\" failed" << std::endl;
    }

  TXMLDocument* xmldoc = xmlparser.GetXMLDocument();
  TXMLNode* rootNode = xmldoc->GetRootNode();
  TXMLNode* node = rootNode->GetChildren();
  TXMLNode* thisNode = NULL;

  while ( node != 0 )
    {
      if ( node->GetNodeName() == TString( "Channel" ) )
	{
	  TListIter attribIt = node->GetAttributes();
	  TXMLAttr* curAttr = 0;

	  while (( curAttr = dynamic_cast< TXMLAttr* >( attribIt() ) ) != 0 )
	    {
	      thisNode = node;
	      readChannel( thisNode );
	    }
	}

      node = node->GetNextNode();
    }
}

void combiner::readChannel( TXMLNode* rootNode )
{
  /* all the attributes of a channel */
  std::string name;   /* name of itself */
  bool isCombined = false;
  double mass = -1;
  std::string fileName;
  std::string wsName;
  std::string mcName;
  std::string poiName;
  // std::string poiMin;
  // std::string poiMax;
  std::string dataName;
  std::map<std::string, std::string> renameMap;
  std::map<std::string, std::string> pdfReMap;
  /* walk through the key node */
  TListIter attribIt = rootNode->GetAttributes();
  TXMLAttr* curAttr = 0;

  while (( curAttr = dynamic_cast< TXMLAttr* >( attribIt() ) ) != 0 )
    {
      if ( curAttr->GetName() == TString( "Name" ) )
	{
	  // name of the channel
	  name = curAttr->GetValue() ;
	}

      if ( curAttr->GetName() == TString( "IsCombined" ) )
	{
	  std::string value = curAttr->GetValue();

	  if ( value.find( "rue" ) != std::string::npos )
	    {
	      isCombined = true;
	    }
	}

      if ( curAttr->GetName() == TString( "Mass" ) )
	{
	  std::string value = curAttr->GetValue();
	  mass = TString(value.c_str()).Atof();
	}
    }

  /* walk through children list */
  TXMLNode* node = rootNode->GetChildren();

  while ( node != 0 )
    {
      if ( node->GetNodeName() == TString( "RenameMap" ) )
	{
	  TXMLNode* subNode = node->GetChildren();

	  while ( subNode != 0 )
	    {
	      std::string oldName = "";
	      std::string newName = "";

	      if ( subNode->GetNodeName() == TString( "Syst" ) )
		{
		  oldName = getAttributeValue( subNode, "OldName" );
		  newName = getAttributeValue( subNode, "NewName" );
		}

	      /* add them to map */
	      if ( oldName != "" && newName != "" )
		{
		  if (
		      oldName.find("(")!=std::string::npos &&
		      oldName.find(")")!=std::string::npos
		      ) {
		    /* only require OldName has all these components, NewName can be only a nuis parameter name, since they should follow the same format */
		    if ( newName.find("(")==std::string::npos ) {
		      assert ( newName.find(")")==std::string::npos );
		      newName = TString::Format("%s_Pdf(%s, %s_In)", newName.c_str(), newName.c_str(), newName.c_str()).Data();
		    }

		    pdfReMap.insert( make_pair( oldName, newName ) );
		  } else if(TString(oldName.c_str()).BeginsWith("alpha_")){
		    /* histfactory style for normal nuisance parameters(except lumi) */
		    oldName = TString::Format("%sConstraint(%s, nom_%s)", oldName.c_str(), oldName.c_str(), oldName.c_str()).Data();
		    newName = TString::Format("%s_Pdf(%s, %s_In)", newName.c_str(), newName.c_str(), newName.c_str()).Data();
		    pdfReMap.insert( make_pair( oldName, newName ) );
		  } else {
		    renameMap.insert( make_pair( oldName, newName ) );
		  }
		}
	      else
		{
		  assert( oldName == "" && newName == "" );
		}

	      subNode = subNode->GetNextNode();
	    }
	}

      if ( node->GetNodeName() == TString( "File" ) )
	{
	  fileName = getAttributeValue( node, "Name" );
	}

      if ( node->GetNodeName() == TString( "Workspace" ) )
	{
	  wsName = getAttributeValue( node, "Name" );
	}

      if ( node->GetNodeName() == TString( "ModelConfig" ) )
	{
	  mcName = getAttributeValue( node, "Name" );
	}

      if ( node->GetNodeName() == TString( "ModelPOI" ) )
	{
	  poiName = getAttributeValue( node, "Name" );
	}

      if ( node->GetNodeName() == TString( "ModelData" ) )
	{
	  dataName = getAttributeValue( node, "Name" );
	}

      node = node->GetNextNode();
    }

  Channel channel;
  channel.name_                                 = name;
  channel.fileName_                             = fileName;
  channel.wsName_                               = wsName;
  channel.mcName_                               = mcName;
  channel.poiName_                              = poiName;
  channel.dataName_                             = dataName;
  channel.renameMap_                            = renameMap;
  channel.pdfReMap_                             = pdfReMap;

  if ( isCombined )
    {
      channel.mass_ = mass;
      m_outSummary = channel;
    }
  else
    {
      m_summary.push_back( channel );
    }
}

std::string combiner::getAttributeValue( TXMLNode* rootNode, std::string attributeKey )
{
  TListIter attribIt = rootNode->GetAttributes();
  TXMLAttr* curAttr = 0;
  std::string attributeValue = "";

  while (( curAttr = dynamic_cast< TXMLAttr* >( attribIt() ) ) != 0 )
    {
      if ( curAttr->GetName() == TString( attributeKey.c_str() ) )
	{
	  attributeValue = curAttr->GetValue() ;
	  break;
	}
    }

  return attributeValue;
}

void combiner::doFit( double mu )
{
  RooArgSet  poi( *m_mc->GetParametersOfInterest() );
  RooRealVar* r = dynamic_cast<RooRealVar*>( poi.first() );

  if ( mu < -0.0001 )
    {
      r->setConstant( false );
    }
  else
    {
      r->setConstant( true );
      r->setVal( mu );
    }

  if ( m_mc->GetNuisanceParameters() )
    {
      m_mc->GetPdf()->fitTo( *m_data, RooFit::Minimizer( "Minuit2", "minimize" ), RooFit::Strategy( 0 ), RooFit::Constrain( *m_mc->GetNuisanceParameters() ) );
      // m_mc->GetPdf()->fitTo(*m_data, RooFit::Minimizer( "Minuit", "MIGRAD" ), RooFit::Hesse(false),RooFit::Minos(false),RooFit::PrintLevel(0),RooFit::Extended(), RooFit::Constrain(*m_mc->GetNuisanceParameters()));
    }
}

void combiner::makeSnapshots0(
			      std::string minimizerType,
			      std::string combinedFile,
			      std::string snapshotHintFile,
			      double tolerance,
			      bool simple,
			      int fitFlag
			      )
{

  asimovUtils::makeSnapshots(m_comb, m_mc, m_data, m_pdf, combinedFile, minimizerType, tolerance, simple, snapshotHintFile, fitFlag);

}


void combiner::makeSnapshots(
			     std::string minimizerType,
			     std::string combinedFile,
			     std::string snapshotHintFile,
			     double tolerance,
			     bool simple,
			     int fitFlag
			     )
{
  TFile* f = new TFile((combinedFile+"_raw.root").c_str());
  RooWorkspace* w = dynamic_cast<RooWorkspace*>(f->Get("combWS"));
  ModelConfig* mc = dynamic_cast<ModelConfig*>(w->obj("ModelConfig"));
  RooArgSet nui = *mc->GetNuisanceParameters();
  RooDataSet* data = dynamic_cast<RooDataSet*>(w->data("combData"));
  RooAbsPdf* pdf = mc->GetPdf();

  RooRealVar* poi  =  (RooRealVar*)(w->set("poi")->first());

  bool isSpin  = (std::string(poi->GetName()) == "epsilon");
  if (isSpin) {
    std::unique_ptr<TIterator> iter(nui.createIterator());
    for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() ) {
      if (TString(v->GetName()).BeginsWith("mu_")) {
        v->setVal(1);
      }
    }

    RooAbsData* data_1  =  asimovUtils::asimovDatasetNominal(mc, 1);
    data_1->SetName("asimov_1_spin");
    w->import(*data_1);
    delete data_1;

    RooAbsData* data_0  =  asimovUtils::asimovDatasetNominal(mc, 0);
    data_0->SetName("asimov_0_spin");
    w->import(*data_0);
    delete data_0;
  }
  else{
    TString outputPlotName = combinedFile.c_str();
    outputPlotName = outputPlotName.ReplaceAll("_raw", "");
    std::cout << "\tOutput plot: " << outputPlotName << std::endl;
    asimovUtils::makeSnapshots(w, mc, data, pdf, outputPlotName.Data(), minimizerType, tolerance, simple, snapshotHintFile, fitFlag);
  }
  w->importClassCode();
  w->writeToFile(combinedFile.c_str());
  delete w;
  delete f;
}


void combiner::getBasePdf(RooProdPdf* pdf, RooArgSet& set)
{
  RooArgList pdfList = pdf->pdfList();
  int pdfSize = pdfList.getSize();
  if (pdfSize==1)
    {
      set.add(pdfList);
    }
  else
    {
      TIterator* iter = pdfList.createIterator();
      RooAbsArg* arg;
      while ((arg = (RooAbsArg*)iter->Next()))
	{
	  std::string className = arg->ClassName();
	  if (className != "RooProdPdf")
	    {
	      set.add(*arg);
	    }
	  else
	    {
	      RooProdPdf* thisPdf = dynamic_cast<RooProdPdf*>(arg);
	      assert ( thisPdf );
	      getBasePdf(thisPdf, set);
	    }
	}
      delete iter; iter = NULL;
    }
}


