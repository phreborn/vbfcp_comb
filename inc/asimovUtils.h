#ifndef ASIMOVUTILS
#define ASIMOVUTILS

#include "rooCommon.h"

#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "THStack.h"
#include "TLegend.h"
#include "TCanvas.h"
#include "TError.h"
#include "TFile.h"

#include <stdexcept>
#include <memory>

namespace asimovUtils {
        class SinglePdfGenInfo {
                public:
                        enum Mode { Binned, Unbinned, Counting };
                        SinglePdfGenInfo( RooAbsPdf& pdf, const RooArgSet& observables, bool preferBinned, const RooDataSet* protoData = NULL, int forceEvents = 0 ) ;
                        ~SinglePdfGenInfo() ;
                        RooDataSet* generateAsimov( RooRealVar *&weightVar ) ;
                        const RooAbsPdf* pdf() const
                        {
                                return pdf_;
                        }
                private:
                        Mode mode_;
                        RooAbsPdf* pdf_;
                        RooArgSet observables_;
                        RooAbsPdf::GenSpec* spec_;
                        RooDataSet* generateCountingAsimov() ;
                        void setToExpected( RooProdPdf& prod, RooArgSet& obs ) ;
                        void setToExpected( RooPoisson& pois, RooArgSet& obs ) ;
        };

        class SimPdfGenInfo
        {
                public:
                        SimPdfGenInfo( RooAbsPdf& pdf, const RooArgSet& observables, bool preferBinned, const RooDataSet* protoData = NULL, int forceEvents = 0 ) ;
                        ~SimPdfGenInfo() ;
                        RooAbsData* generateAsimov( RooRealVar *&weightVar ) ;
                        void setCopyData( bool copyData )
                        {
                                copyData_ = copyData;
                        }
                private:
                        RooAbsPdf*                       pdf_;
                        RooAbsCategoryLValue*            cat_;
                        RooArgSet                        observables_;
                        std::vector<SinglePdfGenInfo*>  pdfs_;
                        RooArgSet                        ownedCrap_;
                        std::map<std::string, RooAbsData*> datasetPieces_;
                        bool                              copyData_;
                        //std::map<std::string,RooDataSet*> datasetPieces_;
        };

        RooAbsData *asimovDatasetNominal( RooStats::ModelConfig *mc, double poiValue, int verbose = 0);

        RooAbsData* asimovDatasetWithFit(
                        RooStats::ModelConfig* mc,
                        RooAbsData& realdata,
                        RooAbsCollection& snapshot,
                        double& nll,
                        double poiValue = 0.0,
                        double tolerance = 0.000001,
                        double forcePoiAfterFit = -100.,
                        int verbose = 0
                        ) ;

        RooAbsPdf* factorizePdf( const RooArgSet& observables, RooAbsPdf& pdf, RooArgList& constraints );

        /// collect factors depending on observables in obsTerms, and all others in constraints
        void factorizePdf( RooStats::ModelConfig& model, RooAbsPdf& pdf, RooArgList& obsTerms, RooArgList& constraints, bool debug = false );
        void factorizePdf( const RooArgSet& observables, RooAbsPdf& pdf, RooArgList& obsTerms, RooArgList& constraints, bool debug = false );
        RooAbsPdf* makeNuisancePdf( RooStats::ModelConfig& model, const char* name = "nuisancePdf" ) ;
        RooAbsPdf* makeNuisancePdf( RooAbsPdf& pdf, const RooArgSet& observables, const char* name = "nuisancePdf" ) ;

        void copyAttributes( const RooAbsArg& from, RooAbsArg& to ) ;

        RooSimultaneous * rebuildSimPdf(const RooArgSet &observables, RooSimultaneous *sim);

        void ModifyInterpolationForAll(RooWorkspace* ws, int code=1);
        void ModifyInterpolationForSet(RooArgSet* modifySet, int code = 1);
        void CheckInterpolation(RooWorkspace* ws);
        void ModifyShapeInterpolationForAll(RooWorkspace* ws, int code=1);
        void ModifyShapeInterpolationForSet(RooArgSet* modifySet, int code = 1);
        void CheckShapeInterpolation(RooWorkspace* ws);

        bool robustMinimize( RooAbsReal &nll, RooMinimizer &minim );
        void makePlots( const RooAbsPdf& pdf, const RooAbsData& data, std::string outPutPlotName_);
        int makeSnapshots(
                        RooWorkspace* m_comb,
                        RooStats::ModelConfig* m_mc,
                        RooDataSet* m_data,
                        RooAbsPdf* m_pdf,
                        std::string m_outputFileName,
                        std::string minimizerType,
                        double tolerance = 0.001,
                        bool simple = false,
                        std::string hintFileName = "",
                        int runFlag=0
                        );

        void makeAsimovDataForMultiPoi(RooWorkspace* w, RooStats::ModelConfig* mc, RooDataSet* data, std::vector<std::string> poiNames );
        void makeAsimov(
                        RooWorkspace* m_comb,
                        RooStats::ModelConfig* m_mc,
                        RooDataSet *m_data,
                        RooAbsPdf* m_pdf,
                        RooArgSet& snapshot,
                        double& nll_float,
                        double mu,
                        double tolerance,
                        std::string asimovTag,
                        std::string outputName
                       );

        bool getHintSet(
                        std::string snapshotHintFile,
                        std::string snapshotHintSnap,
                        RooArgSet& nuisSet,
                        RooArgSet& poiSet
                       );
        //
        //     {
        //         bool success = false; //         TFile* hf = new TFile(snapshotHintFile.c_str());
        //         RooWorkspace* hw = (RooWorkspace*)(hf->Get("combWS"));
        //         if(hw)
        //         {
        //             RooStats::ModelConfig* mc = (RooStats::ModelConfig*)(hw->obj("ModelConfig"));
        //             bool hasSnap = false;
        //             hasSnap = hw->loadSnapshot(snapshotHintSnap.c_str());
        //             hasSnap = (hasSnap && (mc!=NULL));
        //             if(hasSnap)
        //             {
        //                 nuisSet = *mc->GetNuisanceParameters();
        //                 poiSet = *mc->GetParametersOfInterest();
        //                 success = true;
        //             }
        //         }
        //         hf->Close();
        //         return success;
        //     }
  extern bool generateAsimov_;
  extern bool writemuhatWS_;
  extern bool makePlots_;
  extern bool preFit_;

}


#endif
