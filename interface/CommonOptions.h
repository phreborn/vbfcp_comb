#ifndef _CommonOption_h_
#define _CommonOption_h_


  std::string what_ = "";
  std::string configFile_ = "";
  std::string combinedFile_ = "combined.root";
  std::string splittedFile_ = "splitted.root";
  bool snapShot_ = false;
  std::string snapshotHintFile_ = "";
  std::string minimizerType_ = "Minuit2";
  std::string indice_ = "";
  std::string toBeFixed_ = "";
  bool mkInjectedWS_ = false;
  bool simpleScale_ = false;
  std::string injectFile_ = "";
  std::string injectMass_ = "";
  std::string injectStr_ = "mu~1";
  std::string injectDataSet_ = "asimovData_1";
  std::string replaceStr_ = "";
  std::string usePseudoData_ = "";
  std::string dataName_ = "obsData";
  float minimizerTolerance_ = 0.001;

  double rMax_ = -999;
  double tolerance_ = 0.001; /* root default is 0.001 */
  double mHiggs_ = -1;
  bool singlePoi_ = true;
  int fitFlag_ = 0;
  bool editBR_ = false;
  bool editPDF_ = false;
  bool editRFV_ = false;
  int procedure_ = 0;
  bool verbose_ = false;

  bool makeBOnly_ = false;
  // int reBin_ = -1;
  int reBin_ = 500;

#endif
