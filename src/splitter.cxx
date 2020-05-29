/*
 * =====================================================================================
 *
 *       Filename:  splitter.cxx
 *
 *    Description:  Workspace splitter
 *
 *        Version:  1.0
 *        Created:  05/19/2012 10:09:55 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  haoshuang.ji (), haoshuang.ji@cern.ch
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <list>
#include "splitter.h"
#include "RooStats/HistFactory/ParamHistFunc.h"
#include "TObjString.h"

splitter::splitter(
                std::string combinedFile,
                std::string splittedFile,
                std::string dataName
        ):
    m_mkBonly( true )
{
    // m_subComb = NULL;

    m_subComb = new RooWorkspace("combWS", "combWS");

    replaceStr_ = "";

    splittedFile_ = splittedFile;
    TFile* f = new TFile(combinedFile.c_str());
    TList* keys = f->GetListOfKeys();
    TIter next(keys);
    TKey* obj;
    std::string className = "";
    while ((obj = (TKey*)next())) {
        className = obj->GetClassName();
        if ( className.find("RooWorkspace")!=std::string::npos ) {
            m_comb = (RooWorkspace*)obj->ReadObj();
            std::list<TObject*> allObjs = m_comb->allGenericObjects();
            for (std::list<TObject*>::iterator it = allObjs.begin(); it != allObjs.end(); it++) {
                if ( (m_mc = dynamic_cast<RooStats::ModelConfig*>(*it)) &&
                    std::string(m_mc->GetName()).find("only") == std::string::npos
                    ) {
                    break;
                }
            }
            assert ( m_mc );
            m_pdf = dynamic_cast<RooSimultaneous*>(m_mc->GetPdf()); assert (m_pdf);
            // m_cat = dynamic_cast<RooAbsCategoryLValue*>(m_pdf->indexCat());
            // m_cat = (RooAbsCategoryLValue*)&m_pdf->indexCat();
            m_cat = (RooCategory*)&m_pdf->indexCat();
            numChannels = m_cat->numBins(0);

            m_nuis = m_mc->GetNuisanceParameters();
            m_gobs = m_mc->GetGlobalObservables();
            // m_poi = dynamic_cast<RooRealVar*>(m_mc->GetParametersOfInterest()->first()); assert (m_poi);
            m_poi.add(*(m_mc->GetParametersOfInterest()));


            if ( m_poi.getSize()==0 ) {
                /* OK, they don't set pois, i have to do it myself */
                RooArgSet allVars = m_comb->allVars();
                std::unique_ptr<TIterator> iter(allVars.createIterator());
                for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() ) {
                    if ( TString(v->GetName()).BeginsWith("mu_") ) {
                        m_poi.add(*v);
                    }
                }
            }

            // m_data = dynamic_cast<RooDataSet*>(m_comb->data("combData"));
            if ( dataName!="combData" ) {
              std::cout << "\tUsing Pseudo Data: " << dataName << std::endl;
              // assert ( false );
            }
            m_data = dynamic_cast<RooDataSet*>(m_comb->data(dataName.c_str()));
            if ( !m_data ) {
                m_data = dynamic_cast<RooDataSet*>(m_comb->data("obsData"));
            }
            assert ( m_data );
            m_dataList = m_data->split( *m_cat, true );
        }
    }

    /* roostats */
    RooArgSet funcAndPdfs = m_comb->allFunctions();
    std::unique_ptr<TIterator> it(funcAndPdfs.createIterator());
    while ( RooAbsArg* v = (RooAbsArg*)it->Next() ) {

      ParamHistFunc* par = dynamic_cast<ParamHistFunc*>(v);
      if ( par ) {
        // std::cout << "\t\tSet Normalized to be false: " << v->GetName() << std::endl;
        // par->setNormalized(false);
          // par->Print();
        std::string parName = par->GetName();
        if ( parName.find("mc_stat")!=std::string::npos ) {
          continue;
        }
        RooArgList varList = par->paramList();
        std::unique_ptr<TIterator> iter(varList.createIterator());
        while ( RooRealVar* v = (RooRealVar*)iter->Next() ) {
          // v->Print();
          v->setMax(2);
        }
      }

    }
      // assert ( false );



    // /* preset nuisance parameters values */
    // if ( nominalSnapshot!="" ) {
    //     std::cout << "\tINITIALIZE::Setting every nuisance parameters to nominal \n" << std::endl;
    //     m_comb->loadSnapshot(nominalSnapshot.c_str());
    // }
    // else{
    std::cout << "\tINITIALIZE::Setting every nuisance parameters to ucmles to make the fit faster...(may not have it, though) \n" << std::endl;
    m_comb->loadSnapshot("ucmles");
    // }
}

void splitter::grabAsimov(std::string combinedFile)
{
  if ( m_subComb ) {
    TFile* f = new TFile(combinedFile.c_str());
    if ( f ) {
      RooWorkspace* w = dynamic_cast<RooWorkspace*>(f->Get("combWS"));
      if ( !w ) { return; }
      ModelConfig* mc = dynamic_cast<ModelConfig*>(w->obj("ModelConfig"));
      /* if the ws name is combWS, this is a sign that the ws is made by myself */
      if ( w && mc) {
        RooDataSet* a0 = dynamic_cast<RooDataSet*>(w->data("asimovData_0"));
        const RooArgSet* nuis = mc->GetGlobalObservables();
        if ( a0 ) {
          if ( !w->loadSnapshot("conditionalGlobs_0") ) { ; }
          else{
            m_subComb->import(*a0);
            m_subComb->saveSnapshot("conditionalGlobs_0", *nuis, true);
          }
        }
        RooDataSet* a1 = dynamic_cast<RooDataSet*>(w->data("asimovData_1"));
        if ( a1 ) {
          if ( !w->loadSnapshot("conditionalGlobs_1") ) { ; }
          else{
            m_subComb->import(*a1);
            m_subComb->saveSnapshot("conditionalGlobs_1", *nuis, true);
          }
        }
      }
    }
  }
}

splitter::~splitter()
{
  if ( m_subComb ) {
    m_subComb->loadSnapshot("ucmles");
    std::cout<<"Writing the output file "<<splittedFile_.c_str()<<std::endl;
    // asimovUtils::makePlots( *m_subPdf, *m_subData,  "/users/hji/test.pdf");
    m_subComb->writeToFile(splittedFile_.c_str());
    delete m_subComb; m_subComb = NULL;
  }
}

void splitter::printSummary(bool verbose)
{


  std::cout << "\t~~~~~~~~Begin Summary~~~~~~~~" << std::endl;
  std::cout << "\tThere are " << numChannels << " sub channels:" << std::endl;
  for ( int i= 0; i < numChannels; i++ ) {
    m_cat->setBin(i);
    RooAbsPdf* pdfi = m_pdf->getPdf(m_cat->getLabel());
    RooDataSet* datai = ( RooDataSet* )( m_dataList->At( i ) );
    std::cout << "\t\tIndex: " << i << ", Pdf: " << pdfi->GetName() << ", Data: " << datai->GetName()  << ", SumEntries: " << datai->sumEntries() << std::endl;
    // std::cout << "\t\tIndex: " << i << ", Pdf: " << pdfi->GetName() << ", Data: " << pdfi->ClassName()  << ", SumEntries: " << datai->sumEntries() << std::endl;
  }

  if(verbose){
    std::unique_ptr<TIterator> fVarIter(m_comb->componentIterator());
    for ( RooRealVar* v = (RooRealVar*)fVarIter->Next(); v!=0; v = (RooRealVar*)fVarIter->Next() ) {
      if ( std::string(v->ClassName())=="RooStats::HistFactory::FlexibleInterpVar" ) {
        FlexibleInterpVar* oldVar = dynamic_cast<FlexibleInterpVar*>(m_comb->obj(v->GetName()));
        assert ( oldVar );
        std::string varName = v->GetName();

        std::cout << "\nSample name: " << varName << std::endl;
        FlexibleInterpVarExt oldVarExt(*oldVar, "oldVarExtName");
        oldVarExt.printUncerts();

      }
    }
  }
  std::cout<<"\t ########### POI ########### \t"<<std::endl;
  m_mc->GetParametersOfInterest()->Print("v");

  std::cout<<"\t ########### Dataset ########### \t"<<std::endl;
  std::list<RooAbsData*> allData=m_comb->allData();
  for (std::list<RooAbsData*>::iterator it = allData.begin(); it != allData.end(); it++) {
    (*it)->Print();
  }

  std::cout << "\t~~~~~~~~~End Summary~~~~~~~~~" << std::endl;
}

void splitter::fillIndice(std::string indice)
{
  if ( indice=="all" || indice=="ALL" || indice=="All" ) {
    int num = m_cat->numBins(0);
    for ( int i= 0; i < num; i++ ) {
      m_useIndice.push_back(i);
    }
    return;
  }

  /* 0-5,7-9 */
  TObjArray* iArray = TString(indice.c_str()).Tokenize(",");
  int iNum = iArray->GetEntries();
  TString iStr, jStr;
  for ( int i= 0; i < iNum; i++ ) {
    iStr = ((TObjString*)iArray->At(i))->GetString();
    iStr.ReplaceAll(" ", "");
    TObjArray* jArray = iStr.Tokenize("-");
    int jNum = jArray->GetEntries();
    if ( jNum==1 ) {
      jStr = ((TObjString*)jArray->At(0))->GetString();
      jStr.ReplaceAll(" ", "");
      std::cout << "Adding index: " << jStr.Atoi() << std::endl;
      m_useIndice.push_back(jStr.Atoi());
    } else if ( jNum==2 ) {
      TString str1, str2;
      str1 = ((TObjString*)jArray->At(0))->GetString();
      str2 = ((TObjString*)jArray->At(1))->GetString();
      int int1 = str1.Atoi();
      int int2 = str2.Atoi();
      assert ( int1<=int2 );
      for ( int t= int1; t <= int2; t++ ) {
        std::cout << "Adding index: " << t << std::endl;
        m_useIndice.push_back(t);
      }
    } else {
      assert ( false );
    }
  }
}

void splitter::fillFixed(std::string toBeFixed)
{
  TObjArray* iArray = TString(toBeFixed.c_str()).Tokenize(",");
  int iNum = iArray->GetEntries();
  TString iStr;
  for ( int i= 0; i < iNum; i++ ) {
    iStr = ((TObjString*)iArray->At(i))->GetString();
    iStr.ReplaceAll(" ", "");
    m_fixNuis.push_back(iStr.Data());
  }
}

void splitter::makeWorkspace(double rMax_, int reBin, double mass, bool editRFV)
{
  useNumChannels = (int)m_useIndice.size();
  if ( useNumChannels<=0 ) {
    std::cout << "No sub-channel selected, Exit... " << std::endl;
    return;
  }

  // m_subComb = new RooWorkspace("combWS", "combWS");
  // m_subCat = new RooCategory("subCat", "subCat");
  m_subCat = new RooCategory("combCat", "combCat");
  std::string m_poiName = "poi";
  std::string m_pdfName = "combPdf";
  std::string m_dataName = "combData";
  std::string m_obsName = "combObs";
  std::string m_gObsName = "combGobs";
  std::string m_nuisName = "combNuis";

  int index = 0;
  for ( int i= 0; i < useNumChannels; i++ ) {
    index = m_useIndice[i];
    std::cout << "\tsub-index --> " << index << std::endl;
    m_cat->setBin( index );
    RooAbsPdf* pdfi = m_pdf->getPdf( m_cat->getLabel() );
    RooDataSet* datai = ( RooDataSet* )( m_dataList->At( i ) );
    /* make category */
    // TString type = Form( "subCat_%s", m_cat->getLabel() );
    TString type = m_cat->getLabel();

    std::cout << "\t\ttype --> " << type << std::endl;
    m_subCat->defineType( type );
    /* make observables */
    RooArgSet* indivObs = pdfi->getObservables( *datai );
    m_subObs.add( *indivObs );
    /* make nuisances */
    RooArgSet* indivNuis = pdfi->getParameters( *indivObs );
    std::unique_ptr<TIterator> iter( indivNuis->createIterator() );

    // indivNuis->Print();

    for ( RooRealVar* v = ( RooRealVar* )iter->Next(); v != 0; v = ( RooRealVar* )iter->Next() )
    {
      bool isPoi = false;
      std::unique_ptr<TIterator> iter2(m_poi.createIterator());
      while ( RooRealVar* poi = (RooRealVar*)iter2->Next() ) {
        if ( std::string(v->GetName())==std::string(poi->GetName()) ) {
          // continue;
          isPoi = true;
          break;
        }
      }
      if ( isPoi ) { continue; }

      /* in original global observables */
      RooRealVar* obs = ( RooRealVar* )m_gobs->find( v->GetName() );
      if ( obs )
      {
        m_subGobs.add( *obs );
        continue; // this one added as gobs
      }

      /* float any way */
      if ( !( v->isConstant() ) )
      {
        m_subNuis.add( *v );
      }
    }

    /* make pdf */
    m_subPdfMap[type.Data()] = pdfi;
    /* make data */
    m_subDataMap[type.Data()] = datai;
  }

  if(editRFV)
  {
    std::cout << "\tEdit RooFormulaVar " << std::endl;
    /* take this opportunity to edit RooFormulaVar */
    std::unique_ptr<TIterator> fVarIter(m_comb->componentIterator());
    for ( RooRealVar* v = (RooRealVar*)fVarIter->Next(); v!=0; v = (RooRealVar*)fVarIter->Next() ) {
      if ( std::string(v->ClassName())=="RooFormulaVar" ) {
        RooFormulaVar* oldVar = dynamic_cast<RooFormulaVar*>(m_comb->obj(v->GetName()));
        assert ( oldVar );
        std::string varName = v->GetName();

        RooFormulaVarExt oldVarExt(*oldVar, "oldVarExtName");

        /* create a new one to hold the place */
        RooFormulaVar* newVar = NULL;
        oldVarExt.Rebuild(newVar, varName, false);
        if ( newVar ) {
          m_subComb->import(*newVar);
          newVar->Print();
        }
      }
    }
  }

  m_subComb->import(*m_subCat, RooFit::Silence());
  // m_subCat = dynamic_cast<RooCategory*>(m_subComb->obj(m_subCat->GetName()));

  m_subObs.add(*m_subCat);



  std::string oldStr = "";
  std::string newStr = "";
  std::map<std::string, std::string> renameMap;

  if ( replaceStr_!="" ) {
    TString repStr = replaceStr_.c_str();
    repStr = repStr.ReplaceAll(" ", "");
    TObjArray* strArray = repStr.Tokenize(",");
    int nPars = strArray->GetEntries();

    TString subStr;
    for ( int i= 0; i < nPars; i++ ) {
      subStr = ((TObjString*)strArray->At(i))->GetString();
      assert ( subStr.Contains("=") );
      Ssiz_t index = subStr.Index("=");
      TString oldStr = subStr(0, index);
      TString newStr = subStr(index+1,subStr.Length());

      std::cout << "\tReplace: " << oldStr << " to: " << newStr << std::endl;
      RooRealVar* newVar = m_comb->var(newStr.Data());
      assert ( newVar && newStr );
      m_subComb->import(*newVar);
      renameMap[oldStr.Data()] = newStr.Data();
    }
  }


  /* remove some nuisance parameters */

  /* THEORY_allFLAT: Gaussian->Flat */
  /* THEORY_allCONST: all theory syst. const */
  /* NPS_allCONST: all np const */
  /* STATS_only: stat. only */
  /* THEORY_signalCONST: all theory syst. on signal const */
  /* lumiScale: scale the luminosity */
  std::string nuisName = "";
  int numFixNuisSize = (int)m_fixNuis.size();
  if ( numFixNuisSize>0 ) {
    if (
        m_fixNuis[0]=="THEORY_allFLAT"
        || m_fixNuis[0]=="THEORY_allCONST"
        || m_fixNuis[0]=="NPS_allCONST"
        || m_fixNuis[0]=="STATS_only"
       )
    {
      /* THEORY_allFLAT */
      RooArgSet newSet;
      RooArgSet oldSet;
      std::unique_ptr<TIterator> iter(m_subNuis.createIterator());
      TString vName = "";
      TString newVName = "";
      TString pdfName = "";
      TString newPdfName = "";
      for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() )
      {
        vName = v->GetName();

        if ( m_fixNuis[0].find("THEORY")==std::string::npos ) {
          if ( m_fixNuis[0]=="NPS_allCONST" ) {
            // v->setVal(0);
            v->setConstant(true);
            oldSet.add(*v);
          }
          else if ( m_fixNuis[0]=="STATS_only" && !vName.BeginsWith("gamma_stat_") ) {
            // v->setVal(0);
            v->setConstant(true);
            oldSet.add(*v);
          }
        } else if (
            m_fixNuis[0].find("THEORY")!=std::string::npos &&
            // (vName == "QCDscale_Higgs_ggH")

            // (vName == "ATLAS_EM_ES_Z" ||  vName == "ATLAS_EM_MAT1" ||  vName == "ATLAS_EM_PS1"
            //  // ||  vName == "ATLAS_Hgg_Mass_Extra"
            //  || vName.Contains("EM_MAT_LOW")
            //  || vName.Contains("EM_PS_BARR")
            //  || vName.Contains("EM_rest")
            // )
            (vName.BeginsWith("QCDscale_Higgs") ||  vName.BeginsWith("pdf_Higgs"))
                )
                {

                  if ( m_fixNuis[0]=="THEORY_allFLAT" ) {
                    newVName = vName + "_DFD";
                    /* assume it's pdf name!!! */
                    pdfName = vName + "_Pdf";
                    newPdfName = vName + "_DFD_Pdf";
                    RooRealVar* newV = new RooRealVar(newVName.Data(), newVName.Data(), 0, -5, 5);
                    // RooRealVar* newV0 = new RooRealVar((newVName+"_gobs").Data(), (newVName+"_gobs").Data(), v->getVal());
                    RooRealVar* newV0 = new RooRealVar((newVName+"_gobs").Data(), (newVName+"_gobs").Data(), 0.);
                    // RooRealVar* newV = new RooRealVar(newVName.Data(), newVName.Data(), 0, -1, 1);
                    bool doDFD = true;
                    if ( !doDFD ) {
                      RooUniform* newPdf = new RooUniform(newPdfName.Data(), newPdfName.Data(), RooArgSet(*newV));
                      m_subComb->import(*newV);
                      m_subComb->import(*newPdf);
                      std::cout << "\tImported: " << newPdfName << std::endl;
                    } else {

                      // double w = 1000.; double e = 0.1;
                      RooRealVar DFD_e("DFD_e", "DFD_e", 1.);
                      RooRealVar DFD_w("DFD_w", "DFD_w", 500);
                      if ( !(m_subComb->var("DFD_e")) ) { m_subComb->import(DFD_e); }
                      if ( !(m_subComb->var("DFD_w")) ) { m_subComb->import(DFD_w); }

                      RooGenericPdf* flatPdf = new RooGenericPdf(newPdfName.Data(),
                                                                 // "1/( ( 1+exp(@2*(@0-@1)) ) * ( 1+exp(-1*@2*(@0+@1)) ) )",
                                                                 // RooArgList(*newV, DFD_e, DFD_w));
                          "1/( ( 1+exp(@2*(@0-@3-@1)) ) * ( 1+exp(-1*@2*(@0-@3+@1)) ) )",
                          RooArgList(*newV, DFD_e, DFD_w, *newV0));
                      m_subComb->import(*newV);
                      m_subComb->import(*newV0);
                      m_subComb->import(*flatPdf);
                    }


                    oldSet.add(*v);
                    newSet.add(*newV);

                    /* replace the var */
                    renameMap[vName.Data()] = newVName.Data();
                    // renameMap[(vName+"_In").Data()] = (newVName+"_gobs").Data();
                    renameMap[pdfName.Data()] = newPdfName.Data();
                  }
                  else if ( m_fixNuis[0]=="THEORY_allCONST" ) {
                    TString varUnitName = vName + "_UNIT_";
                    if ( !m_subComb->var(varUnitName.Data()) ) {
                      m_subComb->factory((varUnitName+"[0]").Data());
                    }
                    oldSet.add(*v);
                    /* replace the var */
                    renameMap[vName.Data()] = varUnitName.Data();
                  }
                }
      }
      m_subNuis.remove(oldSet);
      m_subNuis.add(newSet);

      linkMap( renameMap, oldStr, newStr, "," );

      std::cout << "\tChanging THEORY constraints from Gaussian to Flat/Fixed: " << std::endl;
      std::cout << "\toldStr: " << oldStr << " ---> " << "newStr: " << newStr << std::endl;
    }
    else if (
        m_fixNuis[0]=="THEORY_signalCONST"
        || m_fixNuis[0]=="lumiScale"
        )
    {
      /* THEORY related */
      RooArgSet theoryNuis;
      std::unique_ptr<TIterator> iter(m_subNuis.createIterator());
      for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() ) {
        TString vName = v->GetName();
        if ( vName.BeginsWith("pdf_")
            || vName.BeginsWith("alpha_pdf_")
            || vName.BeginsWith("QCDscale_") ) {
          theoryNuis.add(*v);
        }
      }

      RooRealVar* firstPOI = dynamic_cast<RooRealVar*>(m_poi.first());

      RooArgSet allPdfs = m_comb->allPdfs();
      /* iter0 */
      std::unique_ptr<TIterator> iter0(allPdfs.createIterator());
      while ( RooAbsPdf* v0 = (RooAbsPdf*)iter0->Next() ) {
        if ( std::string(v0->ClassName())=="RooRealSumPdf"
            || std::string(v0->ClassName())=="RooAddPdf" ) {
          RooArgList funcList;
          if ( std::string(v0->ClassName())=="RooRealSumPdf" ) {
            RooRealSumPdf* sumPdf = dynamic_cast<RooRealSumPdf*>(v0);
            // funcList = sumPdf->funcList();
            funcList.add(sumPdf->funcList());
          }
          else
          {
            RooAddPdf* addPdf = dynamic_cast<RooAddPdf*>(v0);
            // addPdf->Print();
            funcList.add(addPdf->coefList());
          }
          /* iter1 */
          std::unique_ptr<TIterator> iter1(funcList.createIterator());
          for ( RooAbsArg* v1 = (RooAbsArg*)iter1->Next(); v1!=0; v1 = (RooAbsArg*)iter1->Next() ) {

            if(m_fixNuis[0]=="THEORY_signalCONST"){
              if ( v1->dependsOn(*firstPOI) ) { /* signal */
                /* iter2 */
                std::map<std::string, std::string> tmpMap;
                std::string tmpOldStr = "";
                std::string tmpNewStr = "";
                std::unique_ptr<TIterator> iter2(theoryNuis.createIterator());
                for ( RooRealVar* v2 = (RooRealVar*)iter2->Next(); v2!=0; v2 = (RooRealVar*)iter2->Next() ) {
                  // std::string unitName = std::string(v2->GetName()) + "_UNIT_";
                  // if ( !m_subComb->var(unitName.c_str()) ) {
                  //     m_subComb->factory((unitName+"[0]").c_str());
                  // }

                  /* it's good to fix them at MLEs */
                  std::string unitName = std::string(v2->GetName()) + "_MLE_";
                  if ( !m_subComb->var(unitName.c_str()) ) {
                    // m_subComb->factory((unitName+"[0]").c_str());
                    m_subComb->factory(TString::Format("%s[%f]", unitName.c_str(), v2->getVal()).Data());
                  }

                  tmpMap[v2->GetName()] = unitName;
                }
                linkMap( tmpMap, tmpOldStr, tmpNewStr, "," );
                std::cout << "\nfunction: " << v1->GetName() << std::endl;
                std::cout << "\told: " << tmpOldStr << ", new: " << tmpNewStr << std::endl;
                m_subComb->import(*v1,
                                  RooFit::RenameVariable( tmpOldStr.c_str(), tmpNewStr.c_str() ),
                                  RooFit::RecycleConflictNodes(),
                                  RooFit::Silence() );
              }
              else{
                std::cout << "\n\tfunction: " << v1->GetName() << " DOES not depend on mu " << std::endl;
              }
            }
            else {
              if (
                  std::string(v1->ClassName())=="RooRealVar"
                  || std::string(v1->ClassName())=="RooAddition"
                  || std::string(v1->ClassName())=="RooProduct"
                  || std::string(v1->ClassName())=="RooStats::HistFactory::RooBSpline"
                 ) {
                std::string prodName = std::string(v1->GetName());
                if ( prodName.find("frac_")!=std::string::npos ) {
                  continue;
                }

                RooAbsArg* v1Clone = (RooAbsArg*)v1->clone((std::string(prodName+"_lumiScale_old")).c_str());
                if ( RooAbsArg* arg = m_subNuis.find(prodName.c_str()) ) {
                  m_subNuis.remove(*arg);
                  m_subNuis.add(*v1Clone);
                }
                RooArgSet prodSet;
                prodSet.add(*v1Clone);
                RooRealVar* scale = new RooRealVar("lumiScale", "lumiScale", 1, 0, 100);
                scale->setConstant(true);
                prodSet.add(*scale);

                RooProduct* newProd = new RooProduct(prodName.c_str(), prodName.c_str(), prodSet);
                /* import it first to hold the place */
                m_subComb->import(*newProd, RooFit::RecycleConflictNodes(), RooFit::Silence() );
                // std::cout << "\tNew Product " << prodName << " imported! " << std::endl;
              } else{
                std::cout << "\tclass: " << v1->ClassName() << ", name: " << v1->GetName() << std::endl;
                assert ( false );
              }
            }
          }
        }
      }
    }
    else if ( std::string(m_fixNuis[0]).find("*") != std::string::npos ) {
      // std::cout << "\tUsing wild-card, should give only 1 parameter " << std::endl;
      // assert ( numFixNuisSize==1 );
      for ( int i= 0; i < numFixNuisSize; i++ ) {

        RooArgSet* oldSet = dynamic_cast<RooArgSet*>(m_subNuis.selectByName(m_fixNuis[i].c_str()));
        std::unique_ptr<TIterator> iter(oldSet->createIterator());
        for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() ) {
          std::cout << "\t\tSetting " << v->GetName() << " to be constant " << std::endl;
          v->setVal(1);
          v->setConstant();
        }
        m_subNuis.remove(*oldSet);

      }

    }
    else{

      for ( int i= 0; i < numFixNuisSize; i++ ) {
        /* temporary */
        if ( m_fixNuis[i].find("@")!=std::string::npos ) {
          TString tmpStr = m_fixNuis[i].c_str();
          TObjArray* strArray = tmpStr.Tokenize("@");
          int nPars = strArray->GetEntries();
          assert ( nPars==2 );


          TString pdfName = ((TObjString*)strArray->At(0))->GetString();
          TString varName = ((TObjString*)strArray->At(1))->GetString();
          RooRealVar* var = dynamic_cast<RooRealVar*>(m_subNuis.find(varName.Data()));
          assert ( var );

          RooAbsPdf* pdf = dynamic_cast<RooAbsPdf*>(m_comb->obj(pdfName.Data()));
          assert ( pdf );

          TString newPdfName = pdfName + "_flat";
          RooUniform* newPdf = new RooUniform(newPdfName.Data(), newPdfName.Data(), RooArgSet(*var));
          m_subComb->import(*newPdf);
          renameMap[pdfName.Data()] = newPdfName.Data();
          std::cout << "\tReplacing pdf: " << pdfName << " with " << newPdfName << std::endl;
          continue;
        }

        RooRealVar* var = dynamic_cast<RooRealVar*>(m_subNuis.find(m_fixNuis[i].c_str()));
        if ( var ) {

          bool replaceWithFlat(false);
          if ( replaceWithFlat ) {
            /* assume it's pdf name!!! */
            std::string vName = var->GetName();
            std::string pdfName = vName + "_Pdf";
            std::string newPdfName = vName + "_flat_Pdf";
            RooUniform* newPdf = new RooUniform(newPdfName.c_str(), newPdfName.c_str(), RooArgSet(*var));
            m_subComb->import(*newPdf);
            renameMap[pdfName] = newPdfName;
            std::cout << "\tReplacing pdf: " << pdfName << " with " << newPdfName << std::endl;
          } else {
            std::string unitName = std::string(var->GetName()) + "_ZERO_";
            if ( !m_subComb->var(unitName.c_str()) ) {
              // m_subComb->factory((unitName+"[0]").c_str());
              m_subComb->factory(TString::Format("%s[%f]", unitName.c_str(), var->getVal()).Data());
            }

            renameMap[var->GetName()] = unitName;
            m_subNuis.remove(*var);

            // std::cout << "\tGoing to replace " << var->GetName() << " with 0 -> " << unitName << std::endl;
            std::cout << "\tGoing to replace " << var->GetName() << " with " << var->getVal() << " -> " << unitName << std::endl;

          }
        }
      }

    }
  }



  linkMap( renameMap, oldStr, newStr, "," );
  std::cout << "\told: " << oldStr << ", new: " << newStr << std::endl;
  // assert ( false );


  /* make pdf */
  m_subPdf = new RooSimultaneous(
      m_pdfName.c_str(),
      m_pdfName.c_str(),
      m_subPdfMap,
      *m_subCat
      );
  m_subComb->import( *m_subPdf,
                    RooFit::RenameVariable( oldStr.c_str(), newStr.c_str() ),
                    RooFit::RecycleConflictNodes(),
                    RooFit::Silence() );
  m_subPdf = dynamic_cast<RooSimultaneous*>(m_subComb->pdf(m_subPdf->GetName()));


  /* make data */
  // int method = 1;

  if ( reBin<0 ) {
    /* method 0 */
    RooRealVar weight( "_weight_", "", 1. );
    m_subObsAndWgt.add( m_subObs );
    m_subObsAndWgt.add( weight );
    m_subData = new RooDataSet( m_dataName.c_str(), m_dataName.c_str(), m_subObsAndWgt, "_weight_" );

    for ( int i = 0; i < useNumChannels; ++i )
    {
      index = m_useIndice[i];
      m_cat->setBin( index );
      RooDataSet* datai = ( RooDataSet* )( m_dataList->At( index ) );
      // TString type = Form( "subCat_%s", m_cat->getLabel() );
      TString type = m_cat->getLabel();
      m_subCat->setLabel( type, true ); // print error
      for ( int j = 0, nEntries = datai->numEntries(); j < nEntries; ++j )
      {
        // std::cout << "\tEntry: " << j << std::endl;
        m_subObs = *datai->get( j );
        // m_subObs.Print("v");

        double dataWgt = datai->weight();

        m_subData->add( m_subObs, dataWgt );
      }
    }
  }
  else
  {
    RooRealVar* weightVar = new RooRealVar( "_weight_", "", 1. );
    std::map<std::string, RooDataSet*> m_dataMap;
    RooArgSet m_obsAndWgt;

    std::unique_ptr<TIterator> iter(m_subObs.createIterator());
    for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() ) {
      if ( std::string(v->ClassName())!=std::string("RooRealVar") ) { continue; }
      m_obsAndWgt.add(*v);
    }

    m_obsAndWgt.add( *weightVar );

    for ( int i= 0; i < useNumChannels; i++ ) {
      index = m_useIndice[i];
      m_cat->setBin(index);
      RooAbsPdf* pdfi = m_pdf->getPdf(m_cat->getLabel());

      RooDataSet* datai = ( RooDataSet* )( m_dataList->At( index ) );

      int numEntries = datai->numEntries();
      int sumEntries = datai->sumEntries();

      // bool isBinned = (numEntries!=sumEntries);
      bool isBinned = (numEntries!=sumEntries);
      isBinned += (numEntries<reBin);
      if(isBinned)
      {
        TString type = m_cat->getLabel();
        m_subCat->setLabel( type, true );
        m_dataMap[type.Data()] = datai;
      }
      else
      {
        std::string dataiName = datai->GetName();
        std::cout << "\tRebin " << dataiName << std::endl;
        datai->SetName((dataiName+"_old").c_str());
        RooArgList obs (*pdfi->getObservables(*datai));

        RooArgSet obsPlusW;
        obsPlusW.add( obs );
        obsPlusW.add( *weightVar );

        int nBins = reBin;
        RooRealVar* obsVar = (RooRealVar*)obs.at(0);
        TH1* hist = datai->createHistogram((dataiName+"_hist").c_str(), *obsVar, RooFit::Binning(nBins, obsVar->getMin(), obsVar->getMax()));
        RooBinning rebin(nBins, obsVar->getMin(), obsVar->getMax());
        obsVar->setBinning(rebin);

        RooDataSet* dataiNew = new RooDataSet( dataiName.c_str(), "", obsPlusW, weightVar->GetName() );

        for ( int i = 1, n = hist->GetNbinsX(); i <= n; ++i )
        {
          // std::cout << "bin content: " << hist->GetBinContent(i) << std::endl;
          obsVar->setVal( hist->GetXaxis()->GetBinCenter( i ) );
          dataiNew->add( obs, hist->GetBinContent( i ) );
        }

        TString type = m_cat->getLabel();
        m_subCat->setLabel( type, true );
        m_dataMap[type.Data()] = dataiNew;
      }
    }


    m_subData = new RooDataSet(
        m_dataName.c_str(),
        m_dataName.c_str(),
        m_obsAndWgt,
        RooFit::Index( *m_subCat ),
        RooFit::Import( m_dataMap ) ,
        RooFit::WeightVar( *weightVar ) /* actually just pass a name */
        );

    std::cout << "numEntries: " << m_subData->numEntries() << std::endl;
    std::cout << "sumEntries: " << m_subData->sumEntries() << std::endl;
  }


  m_subComb->import( *m_subData );
  m_subComb->importClassCode();

  /* should use those already in combined workspace */
  findArgSetIn( m_subComb, &m_subObs );
  findArgSetIn( m_subComb, &m_subGobs );
  findArgSetIn( m_subComb, &m_subNuis );
  // findArgSetIn( m_subComb, &m_subPoi );

  std::unique_ptr<TIterator> iter(m_poi.createIterator());
  while ( RooRealVar* v = (RooRealVar*)iter->Next() ) {
    RooRealVar* var = m_subComb->var(v->GetName());
    if ( var ) {
      m_subPoi.add(*var);
      if ( rMax_>0 ) { v->setMax(rMax_); }
    }
  }

  /* fix the higgs mass */
  if ( mass>0 ) {
    RooRealVar* mHiggs = m_subComb->var("mHiggs");
    if ( mHiggs ) {
      mHiggs->setVal(mass);
      mHiggs->setConstant(1);
      std::cout << "\tFixing higgs mass to " << mass << std::endl;
      // m_subNuis.remove(*mHiggs);
      m_subNuis.remove(*(m_subNuis.find("mHiggs")));
    }
  }

  m_subComb->defineSet( m_obsName.c_str(), m_subObs, true );
  m_subComb->defineSet( m_gObsName.c_str(), m_subGobs, true );
  m_subComb->defineSet( m_nuisName.c_str(), m_subNuis, true );
  m_subComb->defineSet( "poi", m_subPoi, true );

  m_subMc = new RooStats::ModelConfig( "ModelConfig", m_subComb );
  m_subMc->SetWorkspace(*m_subComb);
  m_subMc->SetPdf( *m_subComb->pdf( m_pdfName.c_str() ) );
  m_subMc->SetProtoData( *m_subComb->data( m_dataName.c_str() ) );
  if(m_subComb->set(m_poiName.c_str()))
    m_subMc->SetParametersOfInterest( *m_subComb->set( "poi" ) );
  m_subMc->SetNuisanceParameters( *m_subComb->set( m_nuisName.c_str() ) );
  m_subMc->SetGlobalObservables( *m_subComb->set( m_gObsName.c_str() ) );
  m_subMc->SetObservables( *m_subComb->set( m_obsName.c_str() ) );
  m_subComb->import( *m_subMc );

  {
    m_subComb->saveSnapshot( "nominalGlobs", *m_subMc->GetGlobalObservables() );
  }

  if(m_mkBonly)
  {
    RooCustomizer make_model_s(*m_subMc->GetPdf(),"_model_bonly_");
    const RooArgSet* poi = m_subMc->GetParametersOfInterest();
    std::unique_ptr<TIterator> iter1(poi->createIterator());
    TString zeroName = "";
    for ( RooRealVar* v = (RooRealVar*)iter1->Next(); v!=0; v = (RooRealVar*)iter1->Next() ) {
      zeroName = TString::Format("_zero_%s", v->GetName());
      // RooRealVar* zero = m_subComb->var("_zero_");
      m_subComb->factory((zeroName + "[0]").Data());
      make_model_s.replaceArg(*v, *m_subComb->var(zeroName.Data()));
    }

    RooAbsPdf *model_b = dynamic_cast<RooAbsPdf *>(make_model_s.build());
    model_b->SetName("_model_bonly_");
    m_subComb->import(*model_b, RooFit::Silence());
    RooStats::ModelConfig* mc_bonly = new RooStats::ModelConfig(*m_subMc);
    mc_bonly->SetPdf(*model_b);
    mc_bonly->SetName("ModelConfig_bonly");
    m_subComb->import(*mc_bonly);
  }
}

void splitter::findArgSetIn( RooWorkspace* w, RooArgSet* set )
{
  std::string setName = set->GetName();
  set->setName( "old" );
  RooArgSet* inSet = ( RooArgSet* )set->clone( setName.c_str() );
  std::unique_ptr<TIterator> iter( set->createIterator() );

  for ( RooAbsArg* v = ( RooAbsArg* )iter->Next(); v != 0; v = ( RooAbsArg* )iter->Next() )
  {
    // std::cout << "name: " << v->GetName() << std::endl;
    RooAbsArg* inV = ( RooAbsArg* )w->obj( v->GetName() );
    if ( !inV ) {
      std::cout << "\tvariable: " << v->GetName() << " not found..." << std::endl;
    }
    assert( inV );
    inSet->add( *inV, kTRUE );
  }

  set = inSet;
}

void splitter::makeSnapshots(std::string minimizerType, double tolerance, bool singlePoi, int fitFlag)
{
  if ((m_subPoi.getSize() == 1) || singlePoi) {

    int nFix = int(m_fixNuis.size());
    if ( (nFix==0) || m_fixNuis[0]!="lumiScale" ) {
      asimovUtils::makeSnapshots(m_subComb, m_subMc, m_subData, m_subPdf, splittedFile_, minimizerType, tolerance, false, "", fitFlag);
    }
    else {
      m_comb->loadSnapshot("conditionalGlobs_1");
      m_comb->loadSnapshot("conditionalNuis_1");
      RooArgSet nuis(*m_subMc->GetNuisanceParameters());
      RooArgSet gobs(*m_subMc->GetGlobalObservables());
      std::unique_ptr<TIterator> iter(nuis.createIterator());
      for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() ) {
        TString name = v->GetName();
        name = name.ReplaceAll("_lumiScale_old", "");
        RooRealVar* var = dynamic_cast<RooRealVar*>(m_comb->obj(name.Data()));
        RooAddition* prod = dynamic_cast<RooAddition*>(m_comb->obj(v->GetName()));
        if ( var ) {
          v->setVal(var->getVal());
        } else if ( prod ) {
          v->setVal(prod->getVal());
          std::cout << "\tFound RooAddition: " << var->GetName() << std::endl;
        } else {
          std::cout << "\tWARNING!! " << v->GetName() << std::endl;
        }
      }
      /* global observables */
      iter.reset(gobs.createIterator());
      for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() ) {
        RooRealVar* var = m_comb->var(v->GetName());
        if ( var ) { v->setVal(var->getVal()); }
      }

      RooDataSet* nomDataset =
          dynamic_cast<RooDataSet*>(
              asimovUtils::asimovDatasetNominal( m_subMc, 1 ));
      nomDataset->SetName("asimovData_1");
      m_subComb->saveSnapshot( "conditionalNuis_1", *m_subMc->GetNuisanceParameters() );
      m_subComb->saveSnapshot( "conditionalGlobs_1", *m_subMc->GetGlobalObservables() );
      m_subComb->import(*nomDataset);
    }

  } else {
    std::vector<std::string> poiNames;
    std::unique_ptr<TIterator> iter(m_subPoi.createIterator());
    for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() ) {
      poiNames.push_back(v->GetName());
    }
    asimovUtils::makeAsimovDataForMultiPoi(m_subComb, m_subMc, m_subData, poiNames );
  }
}

void splitter::makeNominalAsimov()
{
  RooAbsData* asimov_b_nominal = asimovUtils::asimovDatasetNominal(m_subMc, 0);
  asimov_b_nominal->SetName("asimov_b_nominal");
  asimov_b_nominal->SetTitle("asimov_b_nominal");
  m_subComb->import(*asimov_b_nominal);

  RooAbsData* asimov_sb_nominal = asimovUtils::asimovDatasetNominal(m_subMc, 1);
  asimov_sb_nominal->SetName("asimov_sb_nominal");
  asimov_sb_nominal->SetTitle("asimov_sb_nominal");
  m_subComb->import(*asimov_sb_nominal);
}

void splitter::getParaAndVals(
    std::string parseStr,
    std::vector<std::string>& paras,
    std::vector<double>& vals)
{
  std::string::size_type eqidx = 0, colidx = 0, colidx2;
  do {
    eqidx   = parseStr.find("=", colidx);
    colidx2 = parseStr.find(",", colidx+1);
    if (eqidx == std::string::npos || (colidx2 != std::string::npos && colidx2 < eqidx)) {
      throw std::invalid_argument("Error: the argument is wrong~");
    }
    std::string poiName = parseStr.substr(colidx, eqidx);
    std::string poiVal  = parseStr.substr(eqidx+1, (colidx2 == std::string::npos ? std::string::npos : colidx2 - eqidx - 1));
    double value = strtod(poiVal.c_str(),NULL);

    paras.push_back(poiName);
    vals.push_back(value);

    parseStr = parseStr.substr(colidx2+1, std::string::npos);

  } while (colidx2 != std::string::npos);
}

void splitter::makeInjection(
    std::string injectFile,
    std::string injectMass,
    std::string injectStr,
    bool simpleScale,
    bool skipMakeDataset,
    std::string givenSBDataName
    )
{
  /* default givenSBDataName is asimovData_1 */
  std::string nuiSnap = "conditionalNuis_1";
  std::string glbSnap = "conditionalGlobs_1";
  if ( givenSBDataName=="asimovData_1_paz" ) {
    nuiSnap = "conditionalNuis_0";
    glbSnap = "conditionalGlobs_0";
    /* alert!!! */
    /* can only do PAZ(profiled-at-zero) for skipMakeDataset!!! */
  }

  TFile* f = new TFile(injectFile.c_str());
  RooWorkspace* w = (RooWorkspace*)(f->Get("combWS")); assert(w);
  RooStats::ModelConfig* mc = (RooStats::ModelConfig*)w->obj("ModelConfig");
  RooSimultaneous* pdf = dynamic_cast<RooSimultaneous*>(mc->GetPdf()); assert (pdf);
  RooDataSet* data = (RooDataSet*)w->data("combData"); assert (data);
  RooAbsCategoryLValue* cat = (RooAbsCategoryLValue*)&pdf->indexCat();

  /* assume there are injection values, alternatives are all 0 (bkg only) */
  std::vector<std::string> paraNames;
  std::vector<double> paraValues;
  getParaAndVals(injectStr, paraNames, paraValues);
  int nPar = (int)paraNames.size();
  bool isMu = (nPar==1 && paraNames[0]=="mu");
  double muScale = 1.;
  if ( isMu ) {
    muScale = paraValues[0];
    paraValues[0] = 1.;
  }

  /* simply scale */
  if ( simpleScale ) {
    std::string tag = "-";
    for ( int i= 0; i < nPar; i++ ) {
      RooRealVar* var = w->var(paraNames[i].c_str());
      if ( !var ) {
        std::cout << "The input parameter " << paraNames[i] << " does not exsit!!!" << std::endl;
        assert ( false );
      }
      var->setConstant(true);
      var->setVal(paraValues[i]);
      tag += (TString::Format("%s~%.2f-", paraNames[i].c_str(), paraValues[i])).Data();
    }

    /* originally, pass the mu value */
    RooArgSet  poi(*mc->GetParametersOfInterest());
    RooRealVar *r = dynamic_cast<RooRealVar *>(poi.first());
    double firstPOIVal = r->getVal();

    RooRealVar* lumiScale = w->var("lumiScale");
    double lVal = lumiScale->getVal();

    /* set initial values for NPs to the workspaces generated */
    {
      RooArgSet gNuis(*mc->GetNuisanceParameters());
      std::unique_ptr<TIterator> iter(gNuis.createIterator());
      while ( RooRealVar* v = (RooRealVar*)iter->Next() ) {
        RooRealVar* newV = m_subComb->var(v->GetName());
        if ( newV ) {
          newV->setVal(v->getVal());
        }
      }
    }

    /* snapshot @ SM */
    w->loadSnapshot("conditionalNuis_1");
    w->loadSnapshot("conditionalGlobs_1");
    // w->loadSnapshot("nominalNuis");
    // w->loadSnapshot("nominalGlobs");
    RooArgSet gObs(*mc->GetGlobalObservables());
    std::unique_ptr<TIterator> iter(gObs.createIterator());
    while ( RooRealVar* v = (RooRealVar*)iter->Next() ) {
      RooRealVar* newV = m_subComb->var(v->GetName());
      if ( newV ) {
        newV->setVal(v->getVal());
      }
    }
    lumiScale->setVal(lVal);

    RooDataSet* scaleDataset =
        dynamic_cast<RooDataSet*>(
            asimovUtils::asimovDatasetNominal( mc, firstPOIVal ));
    scaleDataset->SetName((std::string("asimovData")+tag).c_str());
    std::cout << "\tFirst poi: " << firstPOIVal << "\tSum: " << scaleDataset->sumEntries() << std::endl;

    m_subComb->import(*scaleDataset);

    return;
  }

  /* if there are no such asimov datas... */
  // if ( !w->data("asimovData_1") ) { isMu = false; }
  if ( !w->data(givenSBDataName.c_str()) ) { isMu = false; }

  /* only skip making asimov datas when injecting mu */
  skipMakeDataset *= isMu;

  std::string tag = "_";
  std::string tag0 = "_";
  for ( int i= 0; i < nPar; i++ ) {
    tag += (TString::Format("%s~%.2f_", paraNames[i].c_str(), paraValues[i])).Data();
    tag0 += (TString::Format("%s~0_", paraNames[i].c_str())).Data();
  }

  if ( !skipMakeDataset ) {
    nuiSnap = std::string("conditionalNuis_") + tag;
    glbSnap = std::string("conditionalGlobs_") + tag;
  }

  ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2");


  /* the injector dataset S+B and B, at injection point */
  RooDataSet* injectSB = NULL;
  RooDataSet* injectB = NULL;


  {
    if ( !skipMakeDataset ) {
      for ( int i= 0; i < nPar; i++ ) {
        RooRealVar* var = w->var(paraNames[i].c_str());
        if ( !var ) {
          std::cout << "The input parameter " << paraNames[i] << " does not exsit!!!" << std::endl;
          assert ( false );
        }
        var->setConstant(true);
        var->setVal(paraValues[i]);
      }

      RooArgSet  poi(*mc->GetParametersOfInterest());
      RooRealVar *r = dynamic_cast<RooRealVar *>(poi.first());
      double firstPOIVal = r->getVal();

      RooArgSet snapshot;
      double nll = 0.;

      injectSB = dynamic_cast<RooDataSet*>(asimovUtils::asimovDatasetWithFit( mc, *data, snapshot, nll, firstPOIVal ));
      injectSB->SetName((std::string("asimovData")+tag).c_str());
      w->import(*injectSB);
      std::cout << "\tMade dataset " << injectSB->GetName() << " for injector" << std::endl;

      RooArgSet nuisSnapshot;
      nuisSnapshot.add( *mc->GetNuisanceParameters() );
      w->saveSnapshot( nuiSnap.c_str(), nuisSnapshot, true );
      w->saveSnapshot( glbSnap.c_str(), snapshot, true );
    } else {
      injectSB = (RooDataSet*)w->data(givenSBDataName.c_str());
    }
    assert ( injectSB );
  }

  /* the injector dataset (B only, but we want to profiled at mu=1/0) */
  {
    assert ( w->loadSnapshot(nuiSnap.c_str()) );
    {
      for ( int i= 0; i < nPar; i++ ) {
        RooRealVar* var = w->var(paraNames[i].c_str());
        var->setConstant(true);
        var->setVal(0.);
      }

      /* originally, pass the mu value */
      RooArgSet  poi(*mc->GetParametersOfInterest());
      RooRealVar *r = dynamic_cast<RooRealVar *>(poi.first());
      double firstPOIVal = r->getVal();

      injectB = dynamic_cast<RooDataSet*>(asimovUtils::asimovDatasetNominal( mc, firstPOIVal ));
      injectB->SetName((std::string("asimovData")+tag0).c_str());
      std::cout << "\tMade dataset " << injectB->GetName() << " for injector" << std::endl;
    }
    assert ( injectB );
    w->import(*injectB);
  }

  TList* injectBDataList = injectB->split( *cat, true );
  TList* injectSBDataList = injectSB->split( *cat, true );

  /*  */
  /* Make B */
  /*  */
  /* the injected dataset (B only, but we want to profiled at mu=1/0) */

  if ( !skipMakeDataset ) {
    /* has to get the snapshot...... */
    for ( int i= 0; i < nPar; i++ ) {
      RooRealVar* var = m_subComb->var(paraNames[i].c_str());
      var->setConstant(true);
      var->setVal(paraValues[i]);
    }

    /* originally, pass the mu value */
    RooArgSet  poi(*m_subMc->GetParametersOfInterest());
    RooRealVar *r = dynamic_cast<RooRealVar *>(poi.first());
    double firstPOIVal = r->getVal();

    RooArgSet snapshot;
    double nll = 0.;

    asimovUtils::asimovDatasetWithFit( m_subMc, *m_subData,
                                      snapshot, nll, firstPOIVal );

    RooArgSet nuisSnapshot;
    nuisSnapshot.add( *m_subMc->GetNuisanceParameters() );
    m_subComb->saveSnapshot( nuiSnap.c_str(), nuisSnapshot, true );
    m_subComb->saveSnapshot( glbSnap.c_str(), snapshot, true );
  }
  else
  {
    /* get nuis parametrized at mu=1/0 from mother workspace */
    assert ( m_comb->loadSnapshot(nuiSnap.c_str()) );
    RooArgSet subNuis = *m_subMc->GetNuisanceParameters();
    subNuis = *m_nuis;
  }

  RooDataSet* asimov_b_nominal = NULL;
  {
    for ( int i= 0; i < nPar; i++ ) {
      RooRealVar* var = m_subComb->var(paraNames[i].c_str());
      var->setConstant(true);
      var->setVal(0.);
    }

    /* originally, pass the mu value */
    RooArgSet  poi(*m_subMc->GetParametersOfInterest());
    RooRealVar *r = dynamic_cast<RooRealVar *>(poi.first());
    double firstPOIVal = r->getVal();

    asimov_b_nominal = dynamic_cast<RooDataSet*>(asimovUtils::asimovDatasetNominal( m_subMc, firstPOIVal ));
    asimov_b_nominal->SetName((std::string("asimovData")+tag0).c_str());
    std::cout << "\tMade dataset " << asimov_b_nominal->GetName() << " for injected" << std::endl;
  }
  assert ( asimov_b_nominal );


  /* set global observables */
  if ( !skipMakeDataset ) {
    m_subComb->loadSnapshot( glbSnap.c_str() );
  } else {
    assert ( m_comb->loadSnapshot(glbSnap.c_str()) );
    RooArgSet subGobs = *m_subMc->GetGlobalObservables();
    subGobs = *m_gobs;
  }

  m_subCat = (RooCategory*)m_subComb->obj(m_subCat->GetName());
  TList* injectingDataList = asimov_b_nominal->split( *m_subCat, true );
  int num = m_subCat->numBins(0);

  /* new dataset */
  RooArgSet obsAndWgt;
  // RooArgSet obs = *asimov_b_nominal->get(); // this is wrong!!!!
  // RooArgSet obs = *m_pdf->getObservables(*asimov_b_nominal);
  // BUG!!!
  RooArgSet obs = *m_subPdf->getObservables(*asimov_b_nominal);
  obs.add(*m_subCat);

  obsAndWgt.add(obs);


  RooRealVar weight( "_weight_", "", 1. );
  obsAndWgt.add( weight );
  std::string name = injectMass + "_injected_data";
  RooDataSet* injectedData = new RooDataSet( name.c_str(), name.c_str(), obsAndWgt, "_weight_" );

  for ( int i= 0; i < num; i++ ) {
    std::cout << "\tsub-index --> " << i << std::endl;
    m_subCat->setBin( i );
    RooDataSet* tmpData = ( RooDataSet* )( injectingDataList->At( i ) );
    std::cout << "\t\tsub-data --> " << tmpData->GetName() << std::endl;
    /* may not have them */
    RooDataSet* tmpInjectB = ( RooDataSet* )(injectBDataList->FindObject(tmpData->GetName()));
    RooDataSet* tmpInjectSB = ( RooDataSet* )(injectSBDataList->FindObject(tmpData->GetName()));
    bool hasInjection = tmpInjectB && tmpInjectSB;

    if ( !hasInjection )
    {
      std::cout << "\tWARNING::No injection from " << tmpData->GetName() << std::endl;
      /* some adhoc modification, since the observables for 4l channel has the mass information
       * , like obs_ATLAS_H_4mu_channel_1400_1 */
      /* this is ugly, I assume they are put in the front of the TList */
      /* also tautau */
      std::string dataName = tmpData->GetName();
      if (
          dataName.find("H_4mu") != std::string::npos ||
          dataName.find("H_4e") != std::string::npos ||
          dataName.find("H_2mu2e") != std::string::npos ||
          dataName.find("H_2e2mu") != std::string::npos ||
          dataName.find("_ll_") != std::string::npos ||
          dataName.find("_lh_") != std::string::npos ||
          dataName.find("_hh_") != std::string::npos
         )
      {
        tmpInjectB = (RooDataSet*) injectBDataList->At(i);
        tmpInjectSB = (RooDataSet*) injectSBDataList->At(i);
        hasInjection = true;
        std::cout << "\t\tFound injection from " << tmpInjectB->GetName() << std::endl;
      }
    }

    // std::cout << "ed name: " << tmpData->GetName() << ", ing name: " << tmpInjectB->GetName() << std::endl;
    for ( int j = 0, nEntries = tmpData->numEntries(); j < nEntries; ++j )
    {
      double thisWeight(0.);
      double injectBWeight(0.);
      double injectSBWeight(0.);
      obs = *tmpData->get(j);
      thisWeight = tmpData->weight();

      if(hasInjection)
      {
        tmpInjectB->get(j);
        tmpInjectSB->get(j);
        injectBWeight = tmpInjectB->weight();
        injectSBWeight = tmpInjectSB->weight();
      }

      double weight = thisWeight + muScale * (injectSBWeight-injectBWeight);
      // std::cout << "\t\tthisWeight: " << thisWeight << ", add Weight: " << (injectSBWeight-injectBWeight) << std::endl;
      injectedData->add( obs, weight );
    }
  }

  std::cout << "\tBOnly dataset sumEntries: " << asimov_b_nominal->sumEntries() << ", injected dataset sumEntries: " << injectedData->sumEntries() << std::endl;

  std::string dataName = m_subData->GetName();
  m_subData = (RooDataSet*)m_subComb->data(dataName.c_str());
  m_subData->SetNameTitle("realData", "realData");
  injectedData->SetName(dataName.c_str());
  // injectedData->SetTitle(dataName.c_str());
  m_subComb->import(*injectedData);

  /* use injectedData as m_subData to make the snapshots */
  // delete m_subData; m_subData = NULL; // can't delete...
  m_subData = injectedData;
}
