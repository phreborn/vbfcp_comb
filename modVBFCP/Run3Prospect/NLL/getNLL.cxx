void getNLL(TString fitresult = "outAllCats_statOnly"){
//void getNLL(TString fitresult = "outAllCats_allSys"){
//void getNLL(TString fitresult = "outAllCats_ssSys"){
//void getNLL(TString fitresult = "outAllCats_jetSys"){
//void getNLL(TString fitresult = "outAllCats_photonSys"){
//void getNLL(TString fitresult = "outTT_statOnly"){
//void getNLL(TString fitresult = "outTT_allSys"){
//void getNLL(TString fitresult = "outTL_statOnly"){
//void getNLL(TString fitresult = "outTL_allSys"){
//void getNLL(TString fitresult = "outLT_statOnly"){
//void getNLL(TString fitresult = "outLT_allSys"){
//void getNLL(TString fitresult = "outLL_statOnly"){
//void getNLL(TString fitresult = "outLL_allSys"){
  map<TString, TString> cHWs;
  cHWs["m0d05"] = "-0.05";
  cHWs["p0d05"] = "0.05";
  cHWs["m0d15"] = "-0.15";
  cHWs["p0d15"] = "0.15";
  cHWs["m0d10"] = "-0.1";
  cHWs["p0d10"] = "0.1";
  cHWs["m0d25"] = "-0.25";
  cHWs["p0d25"] = "0.25";
  cHWs["m0d20"] = "-0.2";
  cHWs["p0d20"] = "0.2";
  cHWs["m0d35"] = "-0.35";
  cHWs["p0d35"] = "0.35";
  cHWs["m0d30"] = "-0.3";
  cHWs["p0d30"] = "0.3";
  cHWs["m0d45"] = "-0.45";
  cHWs["p0d45"] = "0.45";
  cHWs["m0d40"] = "-0.4";
  cHWs["p0d40"] = "0.4";
  cHWs["m0d55"] = "-0.55";
  cHWs["p0d55"] = "0.55";
  cHWs["m0d50"] = "-0.5";
  cHWs["p0d50"] = "0.5";
  cHWs["m0d65"] = "-0.65";
  cHWs["p0d65"] = "0.65";
  cHWs["m0d60"] = "-0.6";
  cHWs["p0d60"] = "0.6";
  cHWs["m0d75"] = "-0.75";
  cHWs["p0d75"] = "0.75";
  cHWs["m0d70"] = "-0.7";
  cHWs["p0d70"] = "0.7";
  cHWs["m0d85"] = "-0.85";
  cHWs["p0d85"] = "0.85";
  cHWs["m0d80"] = "-0.8";
  cHWs["p0d80"] = "0.8";
  cHWs["m0d95"] = "-0.95";
  cHWs["p0d95"] = "0.95";
  cHWs["m0d90"] = "-0.9";
  cHWs["p0d90"] = "0.9";
  cHWs["m0d00"] = "-1.56e-07";
  cHWs["p1d00"] = "1";
  map<TString, double> d_map;
  d_map["m00"] = 0.;
  d_map["m01"] = -0.01;
  d_map["m02"] = -0.02;
  d_map["m03"] = -0.03;
  d_map["m04"] = -0.04;
  d_map["m05"] = -0.05;
  d_map["m06"] = -0.06;
  d_map["m07"] = -0.07;
  d_map["m08"] = -0.08;
  d_map["m10"] = -0.10;
  d_map["m12"] = -0.12;
  d_map["m14"] = -0.14;
  d_map["m16"] = -0.16;
  d_map["m18"] = -0.18;
  d_map["m20"] = -0.20;
  d_map["p01"] = 0.01;
  d_map["p02"] = 0.02;
  d_map["p03"] = 0.03;
  d_map["p04"] = 0.04;
  d_map["p05"] = 0.05;
  d_map["p06"] = 0.06;
  d_map["p07"] = 0.07;
  d_map["p08"] = 0.08;
  d_map["p10"] = 0.10;
  d_map["p12"] = 0.12;
  d_map["p14"] = 0.14;
  d_map["p16"] = 0.16;
  d_map["p18"] = 0.18;
  d_map["p20"] = 0.20;

  TFile *f_SM = new TFile(fitresult+"/out_m00.root", "read");
  //TFile *f_SM = new TFile(fitresult+"/out_m0d0.root", "read");
  TTree *t_SM = (TTree*)f_SM->Get("nllscan");

  double nll_SM;
  t_SM->SetBranchAddress("nll", &nll_SM);
  t_SM->GetEntry(0);

  double minNLL = nll_SM;
  for(auto d = d_map.begin(); d != d_map.end(); d++){
  //for(auto d = cHWs.begin(); d != cHWs.end(); d++){
    TFile *f = new TFile(fitresult+"/out_"+d->first+".root", "read");
    TTree *t = (TTree*)f->Get("nllscan");

    double nll;
    t->SetBranchAddress("nll", &nll);
    t->GetEntry(0);

    if(nll < minNLL) minNLL = nll;
    delete f;
  }

  for(auto d = d_map.begin(); d != d_map.end(); d++){
  //for(auto d = cHWs.begin(); d != cHWs.end(); d++){
    TFile *f = new TFile(fitresult+"/out_"+d->first+".root", "read");
    TTree *t = (TTree*)f->Get("nllscan");

    double nll;
    t->SetBranchAddress("nll", &nll);
    t->GetEntry(0);

    cout<<"nll:"<<d->first<<" "<<nll - minNLL<<endl;
    delete f;
  }

  delete f_SM;
}
