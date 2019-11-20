#include <stdlib.h>

#include "asimovUtils.h"

using namespace RooStats;
using namespace RooFit;
using namespace HistFactory;

asimovUtils::SinglePdfGenInfo::SinglePdfGenInfo( RooAbsPdf& pdf, const RooArgSet& observables, bool preferBinned, const RooDataSet* protoData, int forceEvents ) :
    mode_( pdf.canBeExtended() ? ( preferBinned ? Binned : Unbinned ) : Counting ),
    pdf_( &pdf ),
    spec_( 0 )
{
    if ( pdf.canBeExtended() )
    {
        if ( pdf.getAttribute( "forceGenBinned" ) ) mode_ = Binned;
        else if ( pdf.getAttribute( "forceGenUnbinned" ) ) mode_ = Unbinned;

        //else std::cout << "Pdf " << pdf.GetName() << " has no preference" << std::endl;
    }

    RooArgSet* obs = pdf.getObservables( observables );
    observables_.add( *obs );
    delete obs;
    //if (mode_ == Unbinned) spec_ = protoData ? pdf.prepareMultiGen(observables_, RooFit::Extended(), RooFit::ProtoData(*protoData, true, true))
    //                                         : pdf.prepareMultiGen(observables_, RooFit::Extended());
}

asimovUtils::SinglePdfGenInfo::~SinglePdfGenInfo()
{
    delete spec_;
}

    RooDataSet *
asimovUtils::SinglePdfGenInfo::generateAsimov( RooRealVar *&weightVar )
{
    if ( mode_ == Counting ) return generateCountingAsimov();

    if ( observables_.getSize() > 3 ) throw std::invalid_argument( std::string( "ERROR in SinglePdfGenInfo::generateAsimov for " ) + pdf_->GetName() + ", more than 3 observable" );

    RooArgList obs( observables_ );
    RooRealVar* x = ( RooRealVar* )obs.at( 0 );
    RooRealVar* y = obs.getSize() > 1 ? ( RooRealVar* )obs.at( 1 ) : 0;
    RooRealVar* z = obs.getSize() > 2 ? ( RooRealVar* )obs.at( 2 ) : 0;

    if ( weightVar == 0 ) weightVar = new RooRealVar( "_weight_", "", 1.0 );

    RooCmdArg ay = ( y ? RooFit::YVar( *y ) : RooCmdArg::none() );
    RooCmdArg az = ( z ? RooFit::YVar( *z ) : RooCmdArg::none() );
    std::unique_ptr<TH1> hist( pdf_->createHistogram( "htemp", *x, ay, az ) );
    double expectedEvents = pdf_->expectedEvents( observables_ );
    hist->Scale( expectedEvents / hist->Integral() );
    RooArgSet obsPlusW( obs );
    obsPlusW.add( *weightVar );
    RooDataSet* data = new RooDataSet( TString::Format( "%sData", pdf_->GetName() ), "", obsPlusW, weightVar->GetName() );

    switch ( obs.getSize() )
    {
        case 1:

            for ( int i = 1, n = hist->GetNbinsX(); i <= n; ++i )
            {
                x->setVal( hist->GetXaxis()->GetBinCenter( i ) );
                data->add( observables_, hist->GetBinContent( i ) );
            }

            break;
        case 2:
            {
                TH2& h2 = dynamic_cast<TH2&>( *hist );

                for ( int ix = 1, nx = h2.GetNbinsX(); ix <= nx; ++ix )
                {
                    for ( int iy = 1, ny = h2.GetNbinsY(); iy <= ny; ++iy )
                    {
                        x->setVal( h2.GetXaxis()->GetBinCenter( ix ) );
                        y->setVal( h2.GetYaxis()->GetBinCenter( iy ) );
                        data->add( observables_, h2.GetBinContent( ix, iy ) );
                    }
                }
            }
            break;
        case 3:
            {
                TH3& h3 = dynamic_cast<TH3&>( *hist );

                for ( int ix = 1, nx = h3.GetNbinsX(); ix <= nx; ++ix )
                {
                    for ( int iy = 1, ny = h3.GetNbinsY(); iy <= ny; ++iy )
                    {
                        for ( int iz = 1, nz = h3.GetNbinsZ(); iz <= nz; ++iz )
                        {
                            x->setVal( h3.GetXaxis()->GetBinCenter( ix ) );
                            y->setVal( h3.GetYaxis()->GetBinCenter( iy ) );
                            z->setVal( h3.GetYaxis()->GetBinCenter( iz ) );
                            data->add( observables_, h3.GetBinContent( ix, iy, iz ) );
                        }
                    }
                }
            }
    }

    //std::cout << "Asimov dataset generated from " << pdf_->GetName() << " (sumw? " << data->sumEntries() << ", expected events " << expectedEvents << ")" << std::endl;
    //utils::printRDH(data);
    return data;
}

    RooDataSet *
asimovUtils::SinglePdfGenInfo::generateCountingAsimov()
{
    RooArgSet obs( observables_ );
    RooProdPdf* prod = dynamic_cast<RooProdPdf*>( pdf_ );
    RooPoisson* pois = 0;

    if ( prod != 0 )
    {
        setToExpected( *prod, observables_ );
    }
    else if (( pois = dynamic_cast<RooPoisson*>( pdf_ ) ) != 0 )
    {
        setToExpected( *pois, observables_ );
    }
    else throw std::logic_error( "A counting model pdf must be either a RooProdPdf or a RooPoisson" );

    RooDataSet* ret = new RooDataSet( TString::Format( "%sData", pdf_->GetName() ), "", obs );
    ret->add( obs );
    return ret;
}

    void
asimovUtils::SinglePdfGenInfo::setToExpected( RooProdPdf& prod, RooArgSet& obs )
{
    std::unique_ptr<TIterator> iter( prod.pdfList().createIterator() );

    for ( RooAbsArg* a = ( RooAbsArg* ) iter->Next(); a != 0; a = ( RooAbsArg* ) iter->Next() )
    {
        if ( !a->dependsOn( obs ) ) continue;

        RooPoisson* pois = 0;

        if (( pois = dynamic_cast<RooPoisson*>( a ) ) != 0 )
        {
            setToExpected( *pois, obs );
        }
        else
        {
            RooProdPdf* subprod = dynamic_cast<RooProdPdf*>( a );

            if ( subprod ) setToExpected( *subprod, obs );
            else throw std::logic_error( "Illegal term in counting model: depends on observables, but not Poisson or Product" );
        }
    }
}

    void
asimovUtils::SinglePdfGenInfo::setToExpected( RooPoisson& pois, RooArgSet& obs )
{
    RooRealVar* myobs = 0;
    RooAbsReal* myexp = 0;
    std::unique_ptr<TIterator> iter( pois.serverIterator() );

    for ( RooAbsArg* a = ( RooAbsArg* ) iter->Next(); a != 0; a = ( RooAbsArg* ) iter->Next() )
    {
        if ( obs.contains( *a ) )
        {
            assert( myobs == 0 && "SinglePdfGenInfo::setToExpected(RooPoisson): Two observables??" );
            myobs = dynamic_cast<RooRealVar*>( a );
            assert( myobs != 0 && "SinglePdfGenInfo::setToExpected(RooPoisson): Observables is not a RooRealVar??" );
        }
        else
        {
            assert( myexp == 0 && "SinglePdfGenInfo::setToExpected(RooPoisson): Two expecteds??" );
            myexp = dynamic_cast<RooAbsReal*>( a );
            assert( myexp != 0 && "SinglePdfGenInfo::setToExpected(RooPoisson): Expectedis not a RooAbsReal??" );
        }
    }

    assert( myobs != 0 && "SinglePdfGenInfo::setToExpected(RooPoisson): No observable?" );
    assert( myexp != 0 && "SinglePdfGenInfo::setToExpected(RooPoisson): No expected?" );
    myobs->setVal( myexp->getVal() );
}

asimovUtils::SimPdfGenInfo::SimPdfGenInfo( RooAbsPdf& pdf, const RooArgSet& observables, bool preferBinned, const RooDataSet* protoData, int forceEvents ) :
    pdf_( &pdf ),
    cat_( 0 ),
    observables_( observables ),
    copyData_( true )
{
    assert( forceEvents == 0 && "SimPdfGenInfo: forceEvents must be zero at least for now" );
    RooSimultaneous* simPdf = dynamic_cast<RooSimultaneous*>( &pdf );

    if ( simPdf )
    {
        cat_ = const_cast<RooAbsCategoryLValue*>( &simPdf->indexCat() );
        int nbins = cat_->numBins(( const char* )0 );
        pdfs_.resize( nbins, 0 );
        RooArgList dummy;

        for ( int ic = 0; ic < nbins; ++ic )
        {
            cat_->setBin( ic );
            RooAbsPdf* pdfi = simPdf->getPdf( cat_->getLabel() );
            RooAbsPdf* newpdf = asimovUtils::factorizePdf( observables, *pdfi, dummy );
            pdfs_[ic] = new SinglePdfGenInfo( *newpdf, observables, preferBinned );

            if ( newpdf != 0 && newpdf != pdfi )
            {
                ownedCrap_.addOwned( *newpdf );
            }
        }
    }
    else
    {
        pdfs_.push_back( new SinglePdfGenInfo( pdf, observables, preferBinned, protoData, forceEvents ) );
    }
}

asimovUtils::SimPdfGenInfo::~SimPdfGenInfo()
{
    for ( std::vector<SinglePdfGenInfo*>::iterator it = pdfs_.begin(), ed = pdfs_.end(); it != ed; ++it )
    {
        delete *it;
    }

    pdfs_.clear();

    //for (std::map<std::string,RooDataSet*>::iterator it = datasetPieces_.begin(), ed = datasetPieces_.end(); it != ed; ++it) {
    for ( std::map<std::string, RooAbsData*>::iterator it = datasetPieces_.begin(), ed = datasetPieces_.end(); it != ed; ++it )
    {
        delete it->second;
    }

    datasetPieces_.clear();
}

    RooAbsData *
asimovUtils::SimPdfGenInfo::generateAsimov( RooRealVar *&weightVar )
{
  std::cout<<std::endl<<"Entering the asimovUtils::SimPdfGenInfo::generateAsimov function."<<std::endl;
    RooAbsData* ret = 0;
    TString retName =  TString::Format( "%sData", pdf_->GetName() );
    std::cout<<retName<<std::endl;
    if ( cat_ != 0 )
    {
        //bool needsWeights = false;
        for ( int i = 0, n = cat_->numBins(( const char* )0 ); i < n; ++i )
        {
	  std::cout<<"Entering the loop section."<<std::endl;
            if ( pdfs_[i] == 0 ) continue;

            cat_->setBin( i );
	    std::cout<<cat_->getLabel()<<std::endl;
            //RooAbsData *&data =  datasetPieces_[cat_->getLabel()];
            // delete data;
            // data = pdfs_[i]->generateAsimov( weightVar );
	    pdfs_[i]->generateAsimov( weightVar );
	    std::cout<<"Everything okay!"<<std::endl;
	    datasetPieces_[cat_->getLabel()]=pdfs_[i]->generateAsimov( weightVar );
	    datasetPieces_[cat_->getLabel()]->Print();
        }

        if ( copyData_ )
        {
	  std::cout<<"Entering the copyData_ section."<<std::endl;
            std::map<std::string, RooDataSet*> otherMap;

            for ( std::map<std::string, RooAbsData*>::iterator it = datasetPieces_.begin(), ed = datasetPieces_.end(); it != ed; ++it )
            {
	      
	      ((RooAbsData*)it->second)->Print();
	      RooDataSet* rds = dynamic_cast<RooDataSet*>( it->second );
	      
	      if ( rds == 0 ) throw std::logic_error( "Error, it should have been a RooDataSet" );
	      
	      otherMap[it->first] = rds;
            }

            RooArgSet varsPlusWeight( observables_ );
            varsPlusWeight.add( *weightVar );
            ret = new RooDataSet(
                    retName,
                    "",
                    varsPlusWeight,
                    RooFit::Index(( RooCategory& )*cat_ ),
                    RooFit::Import( otherMap ),
                    RooFit::WeightVar( *weightVar )
                    );
        }
        else
        {
            ret = new RooDataSet(
                    retName,
                    "",
                    observables_,
                    RooFit::Index(( RooCategory& )*cat_ ),
                    RooFit::Link( datasetPieces_ ) /*,
                                                     RooFit::OwnLinked()*/
                    );
        }
    }
    else ret = pdfs_[0]->generateAsimov( weightVar );

    //std::cout << "Asimov dataset generated from sim pdf " << pdf_->GetName() << " (sumw? " << ret->sumEntries() << ")" << std::endl;
    //utils::printRAD(ret);
    return ret;
}

RooAbsData *asimovUtils::asimovDatasetNominal(
        RooStats::ModelConfig *mc,
        double poiValue,
        int verbose
        ) {
    RooArgSet  poi(*mc->GetParametersOfInterest());
    RooRealVar *r = dynamic_cast<RooRealVar *>(poi.first());
    r->setConstant(true); r->setVal(poiValue);
    asimovUtils::SimPdfGenInfo newToyMC(*mc->GetPdf(), *mc->GetObservables(), false);
    RooRealVar *weightVar = 0;
    RooAbsData *asimov = newToyMC.generateAsimov(weightVar);
    delete weightVar;
    return asimov;
}


RooAbsData* asimovUtils::asimovDatasetWithFit(
        RooStats::ModelConfig* mc,
        RooAbsData& realdata,
        RooAbsCollection& snapshot,
        double& nll,
        double poiValue,
        double tolerance,
        double forceMuAfterFit,
        int verbose
        )
{
    RooArgSet  poi( *mc->GetParametersOfInterest() );
    RooRealVar* r = dynamic_cast<RooRealVar*>( poi.first() );

    if ( poiValue<-99 ) {
        r->setConstant(false);
    } else {
        r->setConstant( true );
        r->setVal( poiValue );
    }

    {

        if ( mc->GetNuisanceParameters() )
        {
	  // Speed-up fit to binned PDFs
          RooWorkspace* w = mc->GetWS();
          RooArgSet funcs = w->allPdfs();
          {
            std::unique_ptr<TIterator> iter(funcs.createIterator());
            for ( RooAbsPdf* v = (RooAbsPdf*)iter->Next(); v!=0; v = (RooAbsPdf*)iter->Next() ) {
              std::string name = v->GetName();
              if (v->IsA() == RooRealSumPdf::Class() && name.find("binned")!=std::string::npos) {
                std::cout << "\tset binned likelihood for: " << v->GetName() << std::endl;
                v->setAttribute("BinnedLikelihood", true);
              }
            }
          }

	  // // Deactivate level 2 constant term optimization for CMS H->gamgam workspace
	  // {
	  //   RooFIter iter=w->components().fwdIterator();
	  //   RooAbsArg *arg;
	  //   while ((arg=iter.next())){
	  //     if(arg->IsA()==RooMultiPdf::Class()){
	  // 	arg->setAttribute("NoCacheAndTrack");
	  // 	std::cout<<"De-activating level 2 constant term optimization for "<<arg->GetName()<<std::endl;
	  //     }
	  //   }
	  // }

          RooAbsReal* nll_ = mc->GetPdf()->createNLL(realdata,
                                                     RooFit::Constrain( *mc->GetNuisanceParameters() ),
                                                     RooFit::GlobalObservables(*mc->GetGlobalObservables())
                                                    );

          /* from wouter */
          // minim.setOffsetting(kTRUE);
          nll_->enableOffsetting(nllOffset_);

	  if(robustFit_){
	    RooWorkspace *w_temp=mc->GetWS();
	    w_temp->saveSnapshot("nominalPOIs", *mc->GetParametersOfInterest());
	  }
	  RooMinimizer minim(*nll_);
	  minim.setStrategy(minimizerStrategy_);
	  minim.setPrintLevel(1);
	  minim.setProfile(); /* print out time */
	  minim.setEps(tolerance/0.001);
	  if(constOpt_){
	    cout<<"REGTEST: using level 2 constant optimization"<<endl;
	    minim.optimizeConst(2);
	  }
	  minim.setMaxFunctionCalls(5000*mc->GetPdf()->getVariables()->getSize());//suggest by Stefan
	  minim.minimize("Minuit2");
	  if(improveFit_) minim.improve(); 	// Improve the fit

	  if(robustFit_){
	    RooWorkspace *w_temp=mc->GetWS();
	    // Saving results from last iteration
	    RooArgSet *NuisAndPoi=new RooArgSet();
	    NuisAndPoi->add(*mc->GetNuisanceParameters());
	    NuisAndPoi->add(*mc->GetParametersOfInterest());

	    w_temp->saveSnapshot("fitResults_step1",*NuisAndPoi);
	    double nll_step1=nll_->getVal();
	    // Now kick the POI back to the nominal values and refit.
	    w_temp->loadSnapshot("nominalPOIs");

	    int status = minim.minimize(ROOT::Math::MinimizerOptions::DefaultMinimizerType().c_str(), ROOT::Math::MinimizerOptions::DefaultMinimizerAlgo().c_str());
	    std::cout<<"Refit done with status "<<status<<std::endl;
	    double nll_step2=nll_->getVal();
	    if(nll_step2>nll_step1){
	      std::cout<<"Warning: NLL from refit "<<nll_step2<<" is larger than the one from the first fit "<<nll_step1<<". Reverting to the results from first fit..."<<std::endl;
	      w_temp->loadSnapshot("fitResults_step1");
	    }
	  }

          nll = nll_->getVal();
	  std::cout<<"REGTEST: NLL value: "<<nll<<std::endl;
	  
	  // double init_val=-1;

	  RooWorkspace *m_comb=mc->GetWS();
	  // m_comb->writeToFile("test.root");
	  // m_comb->var("CMS_hzz_bkgMELA")->Print("v");
	  // mc->GetGlobalObservables()->Print("v");
	  // m_comb->function("CMS_hzz_bkgMELA_Pdf")->Print("v");
	  // //std::cout<<glob_temp<<std::endl;
	  // for(int step=0;step<10;step++){
	  //   m_comb->var("CMS_hzz_bkgMELA_In")->setVal(init_val+0.2*step);
	  //   std::cout<<m_comb->var("CMS_hzz_bkgMELA_In")->getVal()<<" "<<nll_->getVal()<<" "<<m_comb->function("CMS_hzz_bkgMELA_Pdf")->getVal()<<std::endl;
	  // }
          delete nll_; nll_ = NULL;

	  // RooArgSet nuisSnapshot;
	  // nuisSnapshot.add( *mc->GetNuisanceParameters() );
	  // m_comb->saveSnapshot( "conditionalNuis_muhat", nuisSnapshot, true );
	  // nuisSnapshot.add( *mc->GetParametersOfInterest() );
	  // m_comb->saveSnapshot( "ucmles", nuisSnapshot, true );
	  // m_comb->saveSnapshot( "conditionalGlobs_muhat", *mc->GetGlobalObservables(), true );

          // Important: binned likelihood attribute to speed up bb and other fits
          {
            // RooWorkspace* w = mc->GetWS();
            // RooArgSet funcs = w->allPdfs();
            std::unique_ptr<TIterator> iter(funcs.createIterator());
            for ( RooAbsPdf* v = (RooAbsPdf*)iter->Next(); v!=0; v = (RooAbsPdf*)iter->Next() ) {
              std::string name = v->GetName();
              if (v->IsA() == RooRealSumPdf::Class() && name.find("binned")!=std::string::npos) {
                std::cout << "\t\tremove binned likelihood for: " << v->GetName() << std::endl;
                v->setAttribute("BinnedLikelihood", false);
              }
            }
          }

	  std::cout << "REGTEST: Fit finished" << std::endl;

        }
        else
        {
          // Do we have free parameters anyway that need fitting?
          bool hasFloatParams = false;
          std::unique_ptr<RooArgSet> params( mc->GetPdf()->getParameters( realdata ) );
          std::unique_ptr<TIterator> iter( params->createIterator() );

          for ( RooAbsArg* a = ( RooAbsArg* ) iter->Next(); a != 0; a = ( RooAbsArg* ) iter->Next() )
          {
            RooRealVar* rrv = dynamic_cast<RooRealVar*>( a );

            if ( rrv != 0 && rrv->isConstant() == false )
            {
              hasFloatParams = true;
              break;
            }
          }

          if ( hasFloatParams )
            mc->GetPdf()->fitTo( realdata,
                                RooFit::Minimizer( "Minuit2", "minimize" ),
                                RooFit::Strategy( minimizerStrategy_ ),
                                RooFit::GlobalObservables(*mc->GetGlobalObservables())
                               );
        }
    }

    if ( mc->GetNuisanceParameters() && verbose )
    {
      std::cout << "Nuisance parameters after fit for asimov dataset: " << std::endl;
      mc->GetNuisanceParameters()->Print( "V" );
    }

    // ***************Hongtao: Save a snapshot which saves fitted mu****************
    RooArgSet* poi_ucmles=(RooArgSet*)mc->GetParametersOfInterest()->snapshot();
    // *****************************************************************************
    /* force mu value before making asimov dataset */
    if ( forceMuAfterFit>-99 ) {
      r->setVal(forceMuAfterFit);
    }
    std::cout<<"REGTEST: Start generating Asimov data...";
    
    // asimovUtils::SimPdfGenInfo newToyMC( *mc->GetPdf(), *mc->GetObservables(), false );
    // RooRealVar* weightVar = 0;

    // RooAbsData* asimov = newToyMC.generateAsimov( weightVar );
    // delete weightVar;

    RooAbsData* asimov=NULL;
    if(generateAsimov_){
      if(preFit_){
	mc->GetWS()->saveSnapshot("conditionalNuis_muhat", *mc->GetNuisanceParameters());
	((RooRealVar*)mc->GetParametersOfInterest()->first())->setVal(1);
	mc->GetWS()->loadSnapshot("nominalNuis");
	mc->GetWS()->loadSnapshot("nominalGlobs");
      }
      asimov=AsymptoticCalculator::GenerateAsimovData( *mc->GetPdf(), *mc->GetObservables() );
    }
    std::cout<<"Done."<<std::endl;
    // NOW SNAPSHOT THE GLOBAL OBSERVABLES
    // GOBS find NUIS
    if ( mc->GetGlobalObservables() && mc->GetGlobalObservables()->getSize() > 0 )
    {
      RooArgSet gobs( *mc->GetGlobalObservables() );
      // snapshot data global observables
      RooArgSet snapGlobalObsData;
      // utils::setAllConstant( gobs, true );
      RooStats::SetAllConstant( gobs, true );
      gobs.snapshot( snapGlobalObsData );
      RooArgSet nuis( *mc->GetNuisanceParameters() );
      std::unique_ptr<RooAbsPdf> nuispdf( asimovUtils::makeNuisancePdf( *mc ) );
      RooProdPdf* prod = dynamic_cast<RooProdPdf*>( nuispdf.get() );
      std::cout<<"REGTEST: Factorizing PDF done."<<std::endl;
      if ( prod == 0 ) throw std::runtime_error( "AsimovUtils: the nuisance pdf is not a RooProdPdf!" );

      std::unique_ptr<TIterator> iter( prod->pdfList().createIterator() );

      for ( RooAbsArg* a = ( RooAbsArg* ) iter->Next(); a != 0; a = ( RooAbsArg* ) iter->Next() )
      {
        RooAbsPdf* cterm = dynamic_cast<RooAbsPdf*>( a );

        if ( !cterm ) throw std::logic_error( "AsimovUtils: a factor of the nuisance pdf is not a Pdf!" );

        if ( !cterm->dependsOn( nuis ) ) continue; // dummy constraints

        if ( typeid( *cterm ) == typeid( RooUniform ) ) continue;
        if ( typeid( *cterm ) == typeid( RooGenericPdf ) ) continue;

        std::unique_ptr<RooArgSet> cpars( cterm->getParameters( &gobs ) );
        std::unique_ptr<RooArgSet> cgobs( cterm->getObservables( &gobs ) );

        if ( cgobs->getSize() != 1 )
        {
          throw std::runtime_error( Form( "AsimovUtils: constraint term %s has multiple global observables", cterm->GetName() ) );
        }

        RooRealVar& rrv = dynamic_cast<RooRealVar&>( *cgobs->first() );
        RooAbsReal* match = 0;

        if ( cpars->getSize() == 1 )
        {
          match = dynamic_cast<RooAbsReal*>( cpars->first() );
        }
        else
        {
          std::unique_ptr<TIterator> iter2( cpars->createIterator() );

          for ( RooAbsArg* a2 = ( RooAbsArg* ) iter2->Next(); a2 != 0; a2 = ( RooAbsArg* ) iter2->Next() )
          {
            RooRealVar* rrv2 = dynamic_cast<RooRealVar*>( a2 );

            if ( rrv2 != 0 && !rrv2->isConstant() )
            {
              if ( match != 0 ) throw std::runtime_error( Form( "AsimovUtils: constraint term %s has multiple floating params", cterm->GetName() ) );

              match = rrv2;
            }
          }
        }

        if ( match == 0 )
        {
          std::cerr << "ERROR: AsimovUtils: can't find nuisance for constraint term " << cterm->GetName() << std::endl;
          std::cerr << "Parameters: " << std::endl;
          cpars->Print( "V" );
          std::cerr << "Observables: " << std::endl;
          cgobs->Print( "V" );
          throw std::runtime_error( Form( "AsimovUtils: can't find nuisance for constraint term %s", cterm->GetName() ) );
        }

        std::string pdfName( cterm->ClassName() );

        if ( pdfName == "RooGaussian"
            || pdfName == "RooBifurGauss"
            || pdfName == "RooPoisson" )
        {
          // this is easy
          if ( pdfName == "RooGaussian" || pdfName == "RooBifurGauss" )
          {
            rrv.setVal( match->getVal() );
          }
          else
          {
            rrv.setVal( match->getVal()*rrv.getVal() );
            // cout << "setting " << rrv.GetName() << "'s value to " << match->GetName() << "'s value, which is: " << rrv.getVal() << endl;
          }
        }
        else if ( pdfName == "RooGamma" )
        {
          // notation as in http://en.wikipedia.org/wiki/Gamma_distribution
          //     nuisance = x
          //     global obs = kappa ( = observed sideband events + 1)
          //     scaling    = theta ( = extrapolation from sideband to signal)
          // we want to set the global obs to a value for which the current value
          // of the nuisance is the best fit one.
          // best fit x = (k-1)*theta    ---->  k = x/theta + 1
          RooArgList leaves;
          cterm->leafNodeServerList( &leaves );
          std::unique_ptr<TIterator> iter2( leaves.createIterator() );
          RooAbsReal* match2 = 0;

          for ( RooAbsArg* a2 = ( RooAbsArg* ) iter2->Next(); a2 != 0; a2 = ( RooAbsArg* ) iter2->Next() )
          {
            RooAbsReal* rar = dynamic_cast<RooAbsReal*>( a2 );

            if ( rar == 0 || rar == match || rar == &rrv ) continue;

            if ( !rar->isConstant() ) throw std::runtime_error( Form( "AsimovUtils: extra floating parameter %s of RooGamma %s.", rar->GetName(), cterm->GetName() ) );

            if ( rar->getVal() == 0 ) continue; // this could be mu

            if ( match2 != 0 ) throw std::runtime_error( Form( "AsimovUtils: extra constant non-zero parameter %s of RooGamma %s.", rar->GetName(), cterm->GetName() ) );

            match2 = rar;
          }

          if ( match2 == 0 ) throw std::runtime_error( Form( "AsimovUtils: could not find the scaling term for  RooGamma %s.", cterm->GetName() ) );

          rrv.setVal( match->getVal() / match2->getVal() + 1. );
        }
        else
        {
          throw std::runtime_error( Form( "AsimovUtils: can't handle constraint term %s of type %s", cterm->GetName(), cterm->ClassName() ) );
        }
      }

      // snapshot
      snapshot.removeAll();
      // utils::setAllConstant( gobs, true );
      RooStats::SetAllConstant( gobs, true );
      gobs.snapshot( snapshot );
      // revert things to normal
      gobs = snapGlobalObsData;

      if ( verbose > 1 )
      {
        std::cout << "Global observables for data: " << std::endl;
        snapGlobalObsData.Print( "V" );
        std::cout << "Global observables for asimov: " << std::endl;
        snapshot.Print( "V" );
        // assert ( false );
      }
    }

    // ******************* Hongtao: Recover the fitted mu **************************
    RooArgSet *poi_tmp=(RooArgSet*)mc->GetParametersOfInterest();
    *poi_tmp=*poi_ucmles;
    // *****************************************************************************

    return asimov;
}

RooAbsPdf* asimovUtils::makeNuisancePdf( RooStats::ModelConfig& model, const char* name )
{
  return asimovUtils::makeNuisancePdf( *model.GetPdf(), *model.GetObservables(), name );
}

RooAbsPdf* asimovUtils::makeNuisancePdf( RooAbsPdf& pdf, const RooArgSet& observables, const char* name )
{
  RooArgList obsTerms, constraints;
  factorizePdf( observables, pdf, obsTerms, constraints );
  return new RooProdPdf( name, "", constraints );
}

RooAbsPdf* asimovUtils::factorizePdf( const RooArgSet& observables, RooAbsPdf& pdf, RooArgList& constraints )
{
  const std::type_info& id = typeid( pdf );

  if ( id == typeid( RooProdPdf ) )
  {
    //std::cout << " pdf is product pdf " << pdf.GetName() << std::endl;
    RooProdPdf* prod = dynamic_cast<RooProdPdf*>( &pdf );
    RooArgList newFactors;
    RooArgSet newOwned;
    RooArgList list( prod->pdfList() );
    bool needNew = false;

    for ( int i = 0, n = list.getSize(); i < n; ++i )
    {
      RooAbsPdf* pdfi = ( RooAbsPdf* ) list.at( i );
      RooAbsPdf* newpdf = factorizePdf( observables, *pdfi, constraints );

      //std::cout << "    for " << pdfi->GetName() << "   newpdf  " << (newpdf == 0 ? "null" : (newpdf == pdfi ? "old" : "new"))  << std::endl;
      if ( newpdf == 0 )
      {
        needNew = true;
        continue;
      }

      if ( newpdf != pdfi )
      {
        needNew = true;
        newOwned.add( *newpdf );
      }

      newFactors.add( *newpdf );
    }

    if ( !needNew )
    {
      copyAttributes( pdf, *prod );
      return prod;
    }
    else if ( newFactors.getSize() == 0 ) return 0;
    else if ( newFactors.getSize() == 1 )
    {
      RooAbsPdf* ret = ( RooAbsPdf* ) newFactors.first()->Clone( TString::Format( "%s_obsOnly", pdf.GetName() ) );
      copyAttributes( pdf, *ret );
      return ret;
    }

    RooProdPdf* ret = new RooProdPdf( TString::Format( "%s_obsOnly", pdf.GetName() ), "", newFactors );
    // newFactors.Print();
    ret->addOwnedComponents( newOwned );
    copyAttributes( pdf, *ret );
    return ret;
  }
  else if ( id == typeid( RooSimultaneous ) )
  {
    RooSimultaneous* sim  = dynamic_cast<RooSimultaneous*>( &pdf );
    RooAbsCategoryLValue* cat = ( RooAbsCategoryLValue* ) sim->indexCat().Clone();
    int nbins = cat->numBins(( const char* )0 );
    TObjArray factorizedPdfs( nbins );
    RooArgSet newOwned;
    bool needNew = false;

    for ( int ic = 0, nc = nbins; ic < nc; ++ic )
    {
      cat->setBin( ic );
      RooAbsPdf* pdfi = sim->getPdf( cat->getLabel() );
      RooAbsPdf* newpdf = factorizePdf( observables, *pdfi, constraints );
      factorizedPdfs[ic] = newpdf;

      if ( newpdf == 0 )
      {
        throw std::runtime_error( std::string( "ERROR: channel " ) + cat->getLabel() + " factorized to zero." );
      }

      if ( newpdf != pdfi )
      {
        needNew = true;
        newOwned.add( *newpdf );
      }
    }

    RooSimultaneous* ret = sim;

    if ( needNew )
    {
      ret = new RooSimultaneous( TString::Format( "%s_obsOnly", pdf.GetName() ), "", ( RooAbsCategoryLValue& ) sim->indexCat() );

      for ( int ic = 0, nc = nbins; ic < nc; ++ic )
      {
        cat->setBin( ic );
        RooAbsPdf* newpdf = ( RooAbsPdf* ) factorizedPdfs[ic];

        if ( newpdf ) ret->addPdf( *newpdf, cat->getLabel() );
      }

      ret->addOwnedComponents( newOwned );
    }

    delete cat;
    copyAttributes( pdf, *ret );
    return ret;
  }
  else if ( pdf.dependsOn( observables ) )
  {
    return &pdf;
  }
  else
  {
    if ( !constraints.contains( pdf ) ) constraints.add( pdf );

    return 0;
  }
}

void asimovUtils::factorizePdf( RooStats::ModelConfig& model, RooAbsPdf& pdf, RooArgList& obsTerms, RooArgList& constraints, bool debug )
{
  return factorizePdf( *model.GetObservables(), pdf, obsTerms, constraints, debug );
}
void asimovUtils::factorizePdf( const RooArgSet& observables, RooAbsPdf& pdf, RooArgList& obsTerms, RooArgList& constraints, bool debug )
{
  const std::type_info& id = typeid( pdf );

  if ( id == typeid( RooProdPdf ) )
  {
    RooProdPdf* prod = dynamic_cast<RooProdPdf*>( &pdf );
    RooArgList list( prod->pdfList() );

    for ( int i = 0, n = list.getSize(); i < n; ++i )
    {
      RooAbsPdf* pdfi = ( RooAbsPdf* ) list.at( i );
      factorizePdf( observables, *pdfi, obsTerms, constraints );
    }
  }
  else if ( id == typeid( RooSimultaneous ) )
  {
    RooSimultaneous* sim  = dynamic_cast<RooSimultaneous*>( &pdf );
    RooAbsCategoryLValue* cat = ( RooAbsCategoryLValue* ) sim->indexCat().Clone();

    for ( int ic = 0, nc = cat->numBins(( const char* )0 ); ic < nc; ++ic )
    {
      cat->setBin( ic );
      factorizePdf( observables, *sim->getPdf( cat->getLabel() ), obsTerms, constraints );
    }

    delete cat;
  }
  else if ( pdf.dependsOn( observables ) )
  {
    if ( !obsTerms.contains( pdf ) ) obsTerms.add( pdf );
  }
  else
  {
    if ( !constraints.contains( pdf ) ) constraints.add( pdf );
  }
}

void asimovUtils::copyAttributes( const RooAbsArg& from, RooAbsArg& to )
{
  if ( &from == &to ) return;

  const std::set<std::string> attribs = from.attributes();

  if ( !attribs.empty() )
  {
    for ( std::set<std::string>::const_iterator it = attribs.begin(), ed = attribs.end(); it != ed; ++it ) to.setAttribute( it->c_str() );
  }

  const std::map<std::string, std::string> strattribs = from.stringAttributes();

  if ( !strattribs.empty() )
  {
    for ( std::map<std::string, std::string>::const_iterator it = strattribs.begin(), ed = strattribs.end(); it != ed; ++it ) to.setStringAttribute( it->first.c_str(), it->second.c_str() );
  }
}

RooSimultaneous * asimovUtils::rebuildSimPdf(const RooArgSet &observables, RooSimultaneous *sim) {
  RooArgList constraints;
  RooAbsCategoryLValue *cat = (RooAbsCategoryLValue *) sim->indexCat().Clone();
  int nbins = cat->numBins((const char *)0);
  TObjArray factorizedPdfs(nbins);
  RooArgSet newOwned;
  for (int ic = 0, nc = nbins; ic < nc; ++ic) {
    cat->setBin(ic);
    RooAbsPdf *pdfi = sim->getPdf(cat->getLabel());
    if (pdfi == 0) { factorizedPdfs[ic] = 0; continue; }
    RooAbsPdf *newpdf = factorizePdf(observables, *pdfi, constraints);
    factorizedPdfs[ic] = newpdf;
    if (newpdf == 0) { continue; }
    if (newpdf != pdfi) { newOwned.add(*newpdf);  }
  }

  RooSimultaneous *ret = new RooSimultaneous(TString::Format("%s_reloaded", sim->GetName()), "", (RooAbsCategoryLValue&) sim->indexCat());
  for (int ic = 0, nc = nbins; ic < nc; ++ic) {
    cat->setBin(ic);
    RooAbsPdf *newpdf = (RooAbsPdf *) factorizedPdfs[ic];
    if (newpdf) {
      if (constraints.getSize() > 0) {
        RooArgList allfactors(constraints); allfactors.add(*newpdf);
        RooProdPdf *newerpdf = new RooProdPdf(TString::Format("%s_plus_constr", newpdf->GetName()), "", allfactors);
        ret->addPdf(*newerpdf, cat->getLabel());
        copyAttributes(*newpdf, *newerpdf);
        newOwned.add(*newerpdf);

        // std::cout << "newpdf " << std::endl;
        // newpdf->Print("v");
        // std::cout << "newerpdf " << std::endl;
        // newerpdf->Print("v");
        // assert ( false );
      } else {
        ret->addPdf(*newpdf, cat->getLabel());
      }
    }
  }
  ret->addOwnedComponents(newOwned);
  copyAttributes(*sim, *ret);
  delete cat;
  return ret;
}

void asimovUtils::makePlots( const RooAbsPdf& pdf, const RooAbsData& data, std::string outPutPlotName_)
{
  if(!makePlots_) return;

  std::vector<RooPlot*> plots;
  const RooSimultaneous* sim  = dynamic_cast<const RooSimultaneous*>( &pdf );
  const RooAbsCategoryLValue& cat = ( RooAbsCategoryLValue& ) sim->indexCat();
  TList* datasets = data.split( cat, true );
  TIter next( datasets );

  int index = 0;
  for ( RooAbsData* ds = ( RooAbsData* ) next(); ds != 0; ds = ( RooAbsData* ) next() )
  {
    RooAbsPdf* pdfi  = sim->getPdf( ds->GetName() );
    std::unique_ptr<RooArgSet> obs( pdfi->getObservables( ds ) );

    if ( obs->getSize() == 0 ) break;

    RooRealVar* x = dynamic_cast<RooRealVar*>( obs->first() );

    if ( x == 0 ) continue;

    int nbins = x->numBins();

    if ( nbins == 0 ) nbins = 100;

    x->SetTitle( ds->GetName() );
    plots.push_back( x->frame( RooFit::Title( ds->GetName() ), RooFit::Bins( nbins ) ) );
    float integral = pdfi->expectedEvents(*x);
    // pdfi->plotOn( plots.back(), Normalization(integral,RooAbsReal::NumEvent) );

    bool drawComponent(true);
    if ( drawComponent && std::string(pdfi->ClassName())=="RooProdPdf" ) {
      RooArgList pdfList = (dynamic_cast<RooProdPdf*>(pdfi))->pdfList();
      /* loop 1 */
      std::unique_ptr<TIterator> iter(pdfList.createIterator());
      for ( RooRealSumPdf* v = (RooRealSumPdf*)iter->Next(); v!=0; v = (RooRealSumPdf*)iter->Next() ) {
        if ( std::string(v->ClassName())=="RooRealSumPdf" ) {
          /* loop 2 */
          RooArgList funcList = (dynamic_cast<RooRealSumPdf*>(v))->funcList();
          RooArgList coefList = (dynamic_cast<RooRealSumPdf*>(v))->coefList();

          int size = funcList.getSize();
          RooProduct* func = NULL;
          RooRealVar* binWidth = NULL;
          // RooAbsArg coef;
          double sum = 0.;

          THStack* stack = new THStack("test", "test");
          TLegend* legend = new TLegend(0.63, 0.63, 0.85, 0.87);
          legend->SetFillStyle( 4000 );
          legend->SetBorderSize( 0 );
          legend->SetShadowColor( 0 );
          legend->SetTextFont( 42 );
          legend->SetTextSize( 0.03 );
          // for ( int i= 0; i < size; i++ ) {
          for ( int i= size-1; i >= 0; i-- ) {
            func = dynamic_cast<RooProduct*>(funcList.at(i));
            binWidth = dynamic_cast<RooRealVar*>(coefList.at(i));

            TString compName(func->GetName());
            TH1* h = func->createHistogram(compName.Data(),*x);
            h->Scale(binWidth->getVal());

            compName.ReplaceAll("L_x_","");
            compName.ReplaceAll("ATLAS_","");
            // compName.ReplaceAll(chanName,"");
            compName.ReplaceAll("_overallSyst_x_StatUncert","");
            compName.ReplaceAll("_overallSyst_x_HistSyst","");
            compName.ReplaceAll("_overallSyst_x_Exp","");
            Ssiz_t index = compName.Index("_");
            compName = compName(0, index);

            h->SetLineColor(i+1);
            h->SetLineWidth(2);
            stack->Add(h);
            legend->AddEntry(h, compName.Data(), "L");
          }
          plots.back()->addObject(stack, "same");
          plots.back()->addObject(legend, "same");

        }
        }
      }
      ds->plotOn( plots.back(), RooFit::DataError(RooAbsData::None), RooFit::MarkerSize(1) );
      pdfi->plotOn( plots.back(), Normalization(integral,RooAbsReal::NumEvent) );

      delete ds; ds = NULL;
      index += 1;
    }
    delete datasets; datasets = NULL;

    gErrorIgnoreLevel = 2001;

    TCanvas* c1 = new TCanvas( "c1", "c1" );
    c1->Print(( outPutPlotName_ + "[" ).c_str() );

    for ( std::vector<RooPlot*>::iterator it = plots.begin(), ed = plots.end(); it != ed; ++it )
    {
      c1->cd();
      ( *it )->Draw();
      c1->SetName(( *it )->GetName() );
      c1->Print(( outPutPlotName_ ).c_str() );
    }

    c1->Print(( outPutPlotName_ + "]" ).c_str() );

    /* should delete, otherwise crashes */
    int num = (int)plots.size();
    for ( int i= 0; i < num; i++ ) {
      delete plots[i];
      plots[i] = NULL;
    }
    plots.clear();
  }


  void asimovUtils::makeAsimovDataForMultiPoi(RooWorkspace* w, RooStats::ModelConfig* mc, RooDataSet* data, std::vector<std::string> poiNames )
  {
    std::cout << "\tGoing to make snapshot for multi-poi: " << std::endl;
    std::string minimizerType = "Minuit2";
    ROOT::Math::MinimizerOptions::SetDefaultMinimizer(minimizerType.c_str());
    RooArgSet snapshot;
    double nll_float, nll_1;

    /* all float */
    RooArgSet ucmles;
    RooAbsData* asimovData_muhat = asimovUtils::asimovDatasetWithFit( mc, *data, snapshot, nll_float, -999);
    ucmles.add(*mc->GetNuisanceParameters());
    ucmles.add(*mc->GetParametersOfInterest());
    w->saveSnapshot( "ucmles", ucmles, true );
    if(generateAsimov_){
      w->saveSnapshot( "conditionalGlobs_muhat_mpoi", *mc->GetGlobalObservables() );
      asimovData_muhat->SetName("asimovData_muhat_mpoi");
      w->import(*asimovData_muhat);
    }

    // /* all set at 1 */
    // for ( int i= 0; i < (int)poiNames.size(); i++ ) {
    //   RooRealVar* var = w->var(poiNames[i].c_str());
    //   var->setConstant(true);
    //   var->setVal(1.);
    //   /* We don't expect to see BRinv */
    //   if ( std::string(var->GetName())=="BRinv" ) {
    //     var->setVal(0.);
    //   }
    // }
    // RooArgSet cond1;
    // RooAbsData* asimovData_1 = asimovUtils::asimovDatasetWithFit( mc, *data, snapshot, nll_1, 1.);
    // cond1.add(*mc->GetNuisanceParameters());
    // // cond1.add(*mc->GetParametersOfInterest());
    // w->saveSnapshot( "conditionalNuis_1", cond1, true );
    // w->saveSnapshot( "conditionalGlobs_1", *mc->GetGlobalObservables() );
    // asimovData_1->SetName("asimovData_1");
    // w->import(*asimovData_1);


    // RooRealVar* var1 = new RooRealVar("nll_1_mpoi", "nll_1_mpoi", nll_1);
    // RooRealVar* var = new RooRealVar("nll_muhat_mpoi", "nll_muhat_mpoi", nll_float);
    // w->import(*var);
    // w->import(*var1);

    // w->loadSnapshot("ucmles_c");
  }

  /* makeSnapshots */
  int asimovUtils::makeSnapshots(
      RooWorkspace* m_comb,
      RooStats::ModelConfig* m_mc,
      RooDataSet* m_data,
      RooAbsPdf* m_pdf,
      std::string m_outputFileName,
      std::string minimizerType,
      double tolerance,
      bool simple,
      std::string hintFileName,
      int runFlag
      )
  {
    ROOT::Math::MinimizerOptions::SetDefaultMinimizer(minimizerType.c_str());

    std::cout << "default minimizerType: " << minimizerType << std::endl;

    RooArgSet hintNuisSet, hintPoiSet;
    RooArgSet nuisSet = *m_mc->GetNuisanceParameters();
    RooArgSet poiSet  = *m_mc->GetParametersOfInterest();

    bool doZero = false;
    bool doFree = false;
    bool doOne = false;
    if ( runFlag==0 ) {
      doZero = true;
      doFree = true;
      doOne = true;
    }
    else if ( runFlag==1 ) {
      doZero = true;
    }
    else if ( runFlag==2 ) {
      doFree = true;
    }
    else if ( runFlag==3 ) {
      doOne = true;
    }

    // {
    //   std::unique_ptr<TIterator> embIter(nuisSet.createIterator());
    //   std::string varName = "";
    //   for ( RooRealVar* v = (RooRealVar*)embIter->Next(); v!=0; v = (RooRealVar*)embIter->Next() ) {
    //     varName = v->GetName();
    //     if ( varName.find("_EMB_")!=std::string::npos &&
    //         varName.find("_EMB_ISO")==std::string::npos
    //        ) {
    //       v->setVal(-0.2);
    //     }
    //   }
    // }


    m_comb->saveSnapshot( "nominalNuis", *m_mc->GetNuisanceParameters() );
    m_comb->saveSnapshot( "nominalGlobs", *m_mc->GetGlobalObservables() );
    RooArgSet snapshot;
    RooArgSet nuisSnapshot;
    double mu = 0;
    double nll_float, nll_1, nll_0;
    RooAbsData* asimovData = NULL;
    std::string plotName = "";
    bool clearMemory = true;

    bool status = false;
    if ( doZero ) {
      /* mu=0 */
      mu = 0;
      status = getHintSet(hintFileName, "conditionalNuis_0", hintNuisSet, hintPoiSet);
      if(status)
      {
        nuisSet = hintNuisSet;
        poiSet = hintPoiSet;
      }

      std::cout << "fit for mu = 0" << std::endl;

      asimovData = asimovUtils::asimovDatasetWithFit( m_mc, *m_data, snapshot, nll_0, mu, tolerance );
      
      m_comb->saveSnapshot( "conditionalNuis_0", *m_mc->GetNuisanceParameters(), true );
      m_comb->saveSnapshot( "conditionalGlobs_0", snapshot, true );
      
      if(generateAsimov_){
	assert( asimovData );
	asimovData->SetName( "asimovData_0" );
	m_comb->import( *asimovData );
      }

      if(makePlots_){
	if(generateAsimov_){
	  plotName = m_outputFileName;
	  plotName = plotName.replace(plotName.find(".root"), 5, "")+"_asimovData_0.pdf";
	  makePlots( *m_pdf, *asimovData,  plotName);
	}
	plotName = m_outputFileName;
	plotName = plotName.replace(plotName.find(".root"), 5, "")+"_obsData_0.pdf";
	makePlots( *m_pdf, *m_data,  plotName);
      }
      
      if(clearMemory){SafeDelete(asimovData); asimovData = NULL; }

      if(simple){
        asimovData = asimovDatasetNominal(m_mc, 1);
        if(generateAsimov_) assert ( asimovData );
        asimovData->SetName( "asimovData_1_paz" );
        m_comb->import( *asimovData );
        m_comb->writeToFile((m_outputFileName+"_simple_asimov.root").c_str());
        if(clearMemory){delete asimovData; asimovData = NULL; }
      }
    }


    if ( doFree ) {
      /* unconditional */
      mu = -100;
      status = getHintSet(hintFileName, "ucmles", hintNuisSet, hintPoiSet);
      std::cout << "\tBegin with free mu fit ~~~ " << std::endl;
      if(status)
      {
        nuisSet = hintNuisSet;
        poiSet = hintPoiSet;
      }

      asimovData = asimovUtils::asimovDatasetWithFit(
          m_mc,
          *m_data,
          snapshot,
          nll_float,
          mu,
          tolerance,
          1 /* force mu = 1 after fit to make asimov data */
          );

      nuisSnapshot.add( *m_mc->GetNuisanceParameters() );
      if(preFit_) m_comb->loadSnapshot("conditionalNuis_muhat");
      else m_comb->saveSnapshot( "conditionalNuis_muhat", nuisSnapshot, true );
      nuisSnapshot.add( *m_mc->GetParametersOfInterest() );
      m_comb->saveSnapshot( "ucmles", nuisSnapshot, true );
      if(!preFit_) m_comb->saveSnapshot( "conditionalGlobs_muhat", snapshot, true );

      if(generateAsimov_){
	assert( asimovData );
	if(preFit_) asimovData->SetName( "asimovData_1_prefit" );
	else asimovData->SetName( "asimovData_muhat" );
	m_comb->import( *asimovData );

      }

      if(makePlots_){
	if(generateAsimov_){
	  plotName = m_outputFileName;
	  plotName = plotName.replace(plotName.find(".root"), 5, "")+"_asimovData_muhat.pdf";
	  makePlots( *m_pdf, *asimovData,  plotName);
	}
	plotName = m_outputFileName;
	plotName = plotName.replace(plotName.find(".root"), 5, "")+"_obsData_muhat.pdf";
	makePlots( *m_pdf, *m_data,  plotName);
      }
      if(writemuhatWS_) m_comb->writeToFile((m_outputFileName+"_muhat.root").c_str());

      if(clearMemory){SafeDelete(asimovData); asimovData = NULL; }
    }

    if ( doOne ) {
      /* mu=1 */
      mu = 1;
      std::cout << "fit for mu = 1" << std::endl;

      asimovData = asimovUtils::asimovDatasetWithFit( m_mc, *m_data, snapshot, nll_1, mu, tolerance );

      m_comb->saveSnapshot( "conditionalNuis_1", *m_mc->GetNuisanceParameters(), true );
      m_comb->saveSnapshot( "conditionalGlobs_1", snapshot, true );
      if(generateAsimov_){
	assert( asimovData );
	if(preFit_) asimovData->SetName( "asimovData_1_prefit" );
	else asimovData->SetName( "asimovData_1" );
	m_comb->import( *asimovData );
      }

      if(makePlots_){
	if(generateAsimov_){
	  plotName = m_outputFileName;
	  plotName = plotName.replace(plotName.find(".root"), 5, "")+"_asimovData_1.pdf";
	  makePlots( *m_pdf, *asimovData,  plotName);
	}
	plotName = m_outputFileName;
	plotName = plotName.replace(plotName.find(".root"), 5, "")+"_obsData_1.pdf";
	makePlots( *m_pdf, *m_data,  plotName);
      }

      if(clearMemory){SafeDelete(asimovData); asimovData = NULL; }
    }

    // if ( nll_float>nll_1 ) {
    //   /* unconditional again */
    //   mu = -100;
    //   RooAbsData* asimovData = asimovUtils::asimovDatasetWithFit( m_mc, *m_data, snapshot, nll_float, mu, tolerance );
    //   assert( asimovData );
    //   nuisSnapshot.add( *m_mc->GetNuisanceParameters() );
    //   m_comb->saveSnapshot( "conditionalNuis_muhat", nuisSnapshot, true );
    //   nuisSnapshot.add( *m_mc->GetParametersOfInterest() );
    //   m_comb->saveSnapshot( "ucmles", nuisSnapshot, true );
    //   m_comb->saveSnapshot( "conditionalGlobs_muhat", snapshot, true );
    //   asimovData->SetName( "asimovData_muhat" );
    //   m_comb->import( *asimovData, true ); /* replace the old */
    //   plotName = m_outputFileName;
    //   plotName = plotName.replace(plotName.find(".root"), 5, "")+"_asimovData_muhat.pdf";
    //   makePlots( *m_pdf, *asimovData,  plotName);

    //   plotName = m_outputFileName;
    //   plotName = plotName.replace(plotName.find(".root"), 5, "")+"_obsData_muhat.pdf";
    //   makePlots( *m_pdf, *m_data,  plotName);


    //   if(clearMemory){delete asimovData; asimovData = NULL; }

    //   /* mu=0 again */
    //   mu = 0;
    //   asimovData = asimovUtils::asimovDatasetWithFit( m_mc, *m_data, snapshot, nll_0, mu, tolerance );
    //   assert( asimovData );
    //   m_comb->saveSnapshot( "conditionalNuis_0", *m_mc->GetNuisanceParameters(), true );
    //   m_comb->saveSnapshot( "conditionalGlobs_0", snapshot, true );
    //   asimovData->SetName( "asimovData_0" );
    //   m_comb->import( *asimovData, true );
    //   plotName = m_outputFileName;
    //   plotName = plotName.replace(plotName.find(".root"), 5, "")+"_asimovData_0.pdf";
    //   makePlots( *m_pdf, *asimovData,  plotName);
    //   plotName = m_outputFileName;
    //   plotName = plotName.replace(plotName.find(".root"), 5, "")+"_obsData_0.pdf";
    //   makePlots( *m_pdf, *m_data,  plotName);


    //   if(clearMemory){delete asimovData; asimovData = NULL; }
    // }

    if ( runFlag!=0 ) {
      return 1;
    }

    int nll_base = int(nll_float);
    std::cout << "nll_flint: " << nll_base << std::endl;
    std::cout << "nll_float: " << nll_float-nll_base << std::endl;
    std::cout << "nll_1    : " << nll_1-nll_base << std::endl;
    std::cout << "nll_0    : " << nll_0-nll_base << std::endl;
    double signif = sqrt(2 * (nll_0-nll_float));
    signif = signif > 0 ? signif : 0;
    std::cout << "signif   : " << signif << std::endl;
    std::cout << "p0       : " << RooStats::SignificanceToPValue(signif) << std::endl;
    RooRealVar* var1 = new RooRealVar("nll_1", "nll_1", nll_1);
    RooRealVar* var0 = new RooRealVar("nll_0", "nll_0", nll_0);
    RooRealVar* var = new RooRealVar("nll_muhat", "nll_muhat0", nll_float);
    m_comb->import(*var);
    m_comb->import(*var0);
    m_comb->import(*var1);

    return 0;
  }


  void asimovUtils::makeAsimov(
      RooWorkspace* m_comb,
      RooStats::ModelConfig* m_mc,
      RooDataSet *m_data,
      RooAbsPdf *m_pdf,
      RooArgSet& snapshot,
      double& nll_float,
      double mu,
      double tolerance,
      std::string asimovTag,
      std::string m_outputFileName
      )
  {
    RooArgSet nuisSnapshot;
    RooDataSet* asimovData = (RooDataSet*)asimovUtils::asimovDatasetWithFit( m_mc, *m_data, snapshot, nll_float, mu, tolerance );
    assert( asimovData );
    nuisSnapshot.add( *m_mc->GetNuisanceParameters() );
    m_comb->saveSnapshot( (std::string("conditionalNuis_")+asimovTag).c_str(), nuisSnapshot, true );
    nuisSnapshot.add( *m_mc->GetParametersOfInterest() );
    if ( mu<-99 ) {
      m_comb->saveSnapshot( "ucmles", nuisSnapshot, true );
    }
    m_comb->saveSnapshot( (std::string("conditionalGlobs_"+asimovTag)).c_str(), snapshot, true );
    asimovData->SetName( (std::string("asimovData_")+asimovTag).c_str() );
    m_comb->import( *asimovData );
    std::string plotName = m_outputFileName;
    plotName = plotName.replace(plotName.find(".root"), 5, "")+"_"+asimovData->GetName()+".pdf";

    /* make plots of asimovData */
    makePlots( *m_pdf, *asimovData,  plotName);
    delete asimovData; asimovData = NULL;
  }

  namespace asimovUtils {
    void ModifyInterpolation(){
      std::cout <<"Choose from the following"<<std::endl;
      std::cout <<"void ModifyInterpolationForAll(RooWorkspace* ws, int code=1);"<<std::endl;
      std::cout <<"void ModifyInterpolationForSet(RooArgSet* modifySet, int code = 1);"<<std::endl;

      std::cout <<"void ModifyShapeInterpolationForAll(RooWorkspace* ws, int code=1);"<<std::endl;
      std::cout <<"void ModifyShapeInterpolationForSet(RooArgSet* modifySet, int code = 1);"<<std::endl;
      std::cout <<"void CheckInterpolation(RooWorkspace* ws);"<<std::endl;
    }

    void ModifyInterpolationForAll(RooWorkspace* ws, int code){
      RooArgSet funcs = ws->allFunctions();
      TIterator* it = funcs.createIterator();
      TObject* tempObj=0;
      while((tempObj=it->Next())){
        FlexibleInterpVar* flex = dynamic_cast<FlexibleInterpVar*>(tempObj);
        if(flex){
          flex->setAllInterpCodes(code);
          // flex->protectAgainstZero();
        }
      }
    }

    void ModifyInterpolationForSet(RooArgSet* modifySet, int code){

      TIterator* it = modifySet->createIterator();
      RooRealVar* alpha=0;
      while((alpha=(RooRealVar*)it->Next())){
        TIterator* serverIt = alpha->clientIterator();
        TObject* tempObj=0;
        while((tempObj=serverIt->Next())){
          FlexibleInterpVar* flex = dynamic_cast<FlexibleInterpVar*>(tempObj);
          if(flex){
            flex->printAllInterpCodes();
            flex->setInterpCode(*alpha,code);
            flex->printAllInterpCodes();
          }
        }
      }

    }

    void ModifyShapeInterpolationForAll(RooWorkspace* ws, int code){
      RooArgSet funcs = ws->allFunctions();
      TIterator* it = funcs.createIterator();
      TObject* tempObj=0;
      while((tempObj=it->Next())){
        PiecewiseInterpolation* piece = dynamic_cast<PiecewiseInterpolation*>(tempObj);
        if(piece){
          piece->setAllInterpCodes(code);
        }
      }
    }

    void ModifyShapeInterpolationForSet(RooArgSet* modifySet, int code){

      TIterator* it = modifySet->createIterator();
      RooRealVar* alpha=0;
      while((alpha=(RooRealVar*)it->Next())){
        TIterator* serverIt = alpha->clientIterator();
        TObject* tempObj=0;
        while((tempObj=serverIt->Next())){

          PiecewiseInterpolation* piece = dynamic_cast<PiecewiseInterpolation*>(tempObj);
          if(piece){
            piece->printAllInterpCodes();
            piece->setInterpCode(*alpha,code);
            piece->printAllInterpCodes();
          }
        }
      }

    }

    void CheckInterpolation(RooWorkspace* ws){
      RooArgSet funcs = ws->allFunctions();
      TIterator* it = funcs.createIterator();
      TObject* tempObj=0;
      while((tempObj=it->Next())){
        FlexibleInterpVar* flex = dynamic_cast<FlexibleInterpVar*>(tempObj);
        if(flex){
          flex->printAllInterpCodes();
        }
      }
    }

    void CheckShapeInterpolation(RooWorkspace* ws){
      RooArgSet funcs = ws->allFunctions();
      TIterator* it = funcs.createIterator();
      TObject* tempObj=0;
      while((tempObj=it->Next())){
        PiecewiseInterpolation* piece = dynamic_cast<PiecewiseInterpolation*>(tempObj);
        if(piece){
          piece->printAllInterpCodes();
        }
      }
    }

    bool robustMinimize( RooAbsReal &nll, RooMinimizer &minim )
    {

      double initialNll = nll.getVal();
      double nowNll = initialNll;
      std::unique_ptr<RooArgSet> pars;
      bool ret = false;

      /* check the status of minimization, takes time & cpu */
      bool checkStatus = false;

      for (int tries = 0, maxtries = 4; tries <= maxtries; ++tries) {

        int status = minim.minimize(ROOT::Math::MinimizerOptions::DefaultMinimizerType().c_str(), ROOT::Math::MinimizerOptions::DefaultMinimizerAlgo().c_str());
        nowNll = nll.getVal();

        printf("\n -------> initialNll: %10.7f, nll.getVal(): %10.7f", initialNll, nowNll);

        if (status == 0 && nowNll > initialNll + 0.02) {
          if ( checkStatus ) {
            std::unique_ptr<RooFitResult> res(minim.save());
            printf("\n  --> false minimum, status %d, cov. quality %d, edm %10.7f, nll initial % 10.4f, nll final % 10.4f, change %10.5f\n", status, res->covQual(), res->edm(), initialNll, nowNll, initialNll - nowNll);
            if (pars.get() == 0) pars.reset(nll.getParameters((const RooArgSet*)0));
            *pars = res->floatParsInit();
          } else {
            printf("\n  --> false minimum\n");
          }

          if (tries == 0) {
            printf("    ----> Doing a re-scan and re-trying\n");
            minim.minimize("Minuit2","Scan");
          } else if (tries == 1) {
            printf("    ----> Re-trying with strategy = 1\n");
            minim.setStrategy(1);
          } else if (tries == 2) {
            printf("    ----> Re-trying with strategy = 2\n");
            minim.setStrategy(2);
          } else  {
            printf("    ----> Last attempt: simplex method \n");
            status = minim.minimize("Minuit2","Simplex");
            if (nowNll < initialNll + 0.02) {
              printf("\n  --> success: status %d, nll initial % 10.4f, nll final % 10.4f, change %10.5f\n", status, initialNll, nowNll, initialNll - nowNll);
              ret = true;
              break;
            } else {
              printf("\n  --> final fail: status %d, nll initial % 10.4f, nll final % 10.4f, change %10.5f\n", status, initialNll, nowNll, initialNll - nowNll);
              ret = false;
              break;
            }
          }
        } else if (status == 0) {
          printf("\n  --> success: status %d, nll initial % 10.4f, nll final % 10.4f, change %10.5f\n", status, initialNll, nowNll, initialNll - nowNll);
          ret = true;
          break;
        } else if (tries != maxtries) {
          if (checkStatus)
          {
            std::unique_ptr<RooFitResult> res(minim.save());
            // if (tries > 0 && res->edm() < 0.05*ROOT::Math::MinimizerOptions::DefaultTolerance())
            if (tries >= 0 && res->edm() < 0.1*ROOT::Math::MinimizerOptions::DefaultTolerance()) {
              printf("\n  --> acceptable: status %d, edm %10.7f, nll initial % 10.4f, nll final % 10.4f, change %10.5f\n", status, res->edm(), initialNll, nowNll, initialNll - nowNll);
              ret = true;
              break;
            }
            printf("\n  tries: %d, --> partial fail: status %d, cov. quality %d, edm %10.7f, nll initial % 10.4f, nll final % 10.4f, change %10.5f\n", tries, status, res->covQual(), res->edm(), initialNll, nowNll, initialNll - nowNll);
          }
          else
          {
            printf("\n  tries: %d, --> partial fail\n", tries);
          }

          if (tries == 1) {
            printf("    ----> Doing a re-scan first, and switching to strategy 1\n");
            minim.minimize("Minuit2","Scan");
            minim.setStrategy(1);
          }
          else if(tries>1){
            ret = false;
            break;
          }
          /* commented because using strategy 2 is very time-consuming but not helpful */
          // else if (tries == 2) {
          //     printf("    ----> trying with strategy = 2\n");
          //     minim.minimize("Minuit2","Scan");
          //     minim.setStrategy(2);
          // }
          // else if (tries == 3) {
          //     printf("    ----> trying with strategy = 2\n");
          //     minim.minimize("Minuit2","Scan");
          //     minim.setStrategy(2);
          // }
        }
        else {
          if ( checkStatus ) {
            std::unique_ptr<RooFitResult> res(minim.save());
            printf("\n  --> final fail: status %d, cov. quality %d, edm %10.7f, nll initial % 10.4f, nll final % 10.4f, change %10.5f\n", status, res->covQual(), res->edm(), initialNll, nowNll, initialNll - nowNll);
          } else {
            printf("\n  --> final fail\n");
          }
        }
      }
      return ret;
    }

    bool getHintSet(
        std::string snapshotHintFile,
        std::string snapshotHintSnap,
        RooArgSet& nuisSet,
        RooArgSet& poiSet
        )
    {
      bool success = false;
      if(snapshotHintFile=="") return success;
      TFile* hf = new TFile(snapshotHintFile.c_str());
      if(!hf) return success;

      RooWorkspace* hw = (RooWorkspace*)(hf->Get("combWS"));
      if(hw)
      {
        RooStats::ModelConfig* mc = (RooStats::ModelConfig*)(hw->obj("ModelConfig"));
        bool hasSnap = false;
        hasSnap = hw->loadSnapshot(snapshotHintSnap.c_str());
        hasSnap = (hasSnap && (mc!=NULL));
        if(hasSnap)
        {
          if(nuisSet.getSize()==0)
            nuisSet.add(*mc->GetNuisanceParameters());
          else
            nuisSet = *mc->GetNuisanceParameters();

          if(poiSet.getSize()==0)
            poiSet.add(*mc->GetParametersOfInterest());
          else
            poiSet = *mc->GetParametersOfInterest();

          success = true;
        }
      }
      hf->Close();
      if ( success ) {
        std::cout << "\tTaken the given snapshot/hint using file: " << snapshotHintFile << std::endl;
      }

      return success;
    }


    // bool trimGammaParam( RooWorkspace* w, ModelConfig* mc) {
    //     RooArgSet* npSet = mc->GetNuisanceParameters();
    //     std::unique_ptr<TIterator> iter(npSet->createIterator());

    //     for ( RooRealVar* v = (RooRealVar*)iter->Next(); v!=0; v = (RooRealVar*)iter->Next() ) {
    //         std::string ConstraintType = constraint->IsA()->GetName();

    //         if( ConstraintType == "RooGaussian" ){
    //             RooAbsReal* sigmaVar = (RooAbsReal*) w->obj( (name+"_sigma").c_str() );
    //             if( !sigmaVar ) return false;
    //             double sigma = sigmaVar->getVal();

    //             arg->setMax(arg->getVal()+stdDevRange*sigma);
    //             arg->setMin(arg->getVal()-stdDevRange*sigma);
    //             cout << "Adjusted Gamma parameter " << arg->GetName() << " ["<<arg->getMin()<<","<<arg->getMax()<<"]" << endl;
    //         }else if( ConstraintType == "RooPoisson" ){
    //             RooAbsReal* nom_gamma = (RooAbsReal*) w->obj( ("nom_" + name).c_str() );
    //             if( !nom_gamma ) return false;

    //             double nom_gamma_val = nom_gamma->getVal();
    //             cout << "nom_gamma_val = " << nom_gamma_val << endl;

    //             double sigma = 1/TMath::Sqrt( nom_gamma_val );
    //             arg->setMax(arg->getVal()+stdDevRange*sigma);
    //             arg->setMin(arg->getVal()-stdDevRange*sigma);
    //             if( arg->getMin() < 0.0 ) arg->setMin( 0.0 );
    //             cout << "Adjusted Gamma parameter " << arg->GetName() << " ["<<arg->getMin()<<","<<arg->getMax()<<"]" << endl;

    //             if( (arg->getMax() - arg->getMin())/arg->getVal() < 0.001 ) {
    //                 cout << "  ^^^^ candidate to make const: diff/val = " << (arg->getMax() - arg->getMin())/arg->getVal() << endl;
    //             }
    //         }else{
    //             std::cout << "Error: Unknown Stat Uncertainty constraint found: " << ConstraintType << std::endl;
    //         }
    //     }

    //     return true;
    // }

  }
