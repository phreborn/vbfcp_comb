#include "auxUtil.hh"

void auxUtil::printTitle(TString titleText, TString separator, int width){
  TString fullLine="", line="";
  
  int stringLength=titleText.Length();
  int fullLineWidth=2*width+((stringLength>2*width) ? (stringLength) : (2*width))+2;
    
  for(int i=0;i<fullLineWidth;i++) fullLine+=separator;
  for(int i=0;i<width;i++) line+=separator;
  
  cout<<endl<<TString("\t "+fullLine)<<endl;
  if(stringLength>2*width) cout<<"\t "<<line<<" "<<titleText<<" "<<line<<endl;
  else printf("\t "+line+" %*s%*s "+line+"\n",int(width+titleText.Length()/2),titleText.Data(),int(width-titleText.Length()/2),"");
  cout<<TString("\t "+fullLine)<<endl<<endl;
}

void auxUtil::Reset(RooArgSet* original, RooArgSet* snapshot){
  *original=*snapshot;
  // Still need to recover the ranges for variables
  unique_ptr<TIterator> iter(original->createIterator());
  RooRealVar* parg = NULL;  
  while((parg=dynamic_cast<RooRealVar*>(iter->Next()))){
    RooRealVar *snapVar=dynamic_cast<RooRealVar*>(snapshot->find(parg->GetName()));
    parg->setRange(snapVar->getMin(), snapVar->getMax());
  }
}

int auxUtil::parseXMLFile(TDOMParser *xmlparser, TString inputFile){
  Int_t parseError = xmlparser->ParseFile( inputFile );

  if ( parseError ){
    cerr << "\tERROR: Loading of xml document \"" << inputFile
	      << "\" failed" << endl;
  }
  return parseError;
}

vector<TString> auxUtil::splitString(const TString& theOpt, const char separator )
{
   // splits the option string at 'separator' and fills the list
   // 'splitV' with the primitive strings
   vector<TString> splitV;
   if(theOpt == "") return splitV; // If the string is empty, the vector should be empty as well
   
   TString splitOpt(theOpt);
   splitOpt.ReplaceAll("\n"," ");
   splitOpt = splitOpt.Strip(TString::kBoth,separator);
   while (splitOpt.Length()>0) {
      if ( !splitOpt.Contains(separator) ) {
         splitV.push_back(splitOpt);
         break;
      }
      else {
         TString toSave = splitOpt(0,splitOpt.First(separator));
         splitV.push_back(toSave);
         splitOpt = splitOpt(splitOpt.First(separator),splitOpt.Length());
      }
      splitOpt = splitOpt.Strip(TString::kLeading,separator);
   }
   return splitV;
}

TString auxUtil::generateExpr(TString head, RooArgSet *set, bool closeExpr){
  TString exprStr=head;
  unique_ptr<TIterator> iter (set->createIterator());
  RooAbsArg* parg = NULL;
  while((parg=dynamic_cast<RooAbsArg*>(iter->Next()))){
    exprStr+=TString(parg->GetName())+",";
  }
  if(closeExpr) closeFuncExpr(exprStr);
  return exprStr;
}

void auxUtil::defineSet(RooWorkspace *w, RooArgSet set, TString setName){
  unique_ptr<TIterator> iter(set.createIterator());
  RooRealVar* parg = NULL;
  RooArgSet nameSet;
  while((parg=dynamic_cast<RooRealVar*>(iter->Next()))){
    if(w->var(parg->GetName())) nameSet.add(*w->var(parg->GetName()));
  }

  w->defineSet(setName, nameSet);
}

void auxUtil::defineSet(RooWorkspace *w, vector<TString> set, TString setName){
  RooArgSet nameSet;
  for(vector<TString>::iterator it = set.begin(); it != set.end(); ++it){
    if(w->var(*it)) nameSet.add(*w->var(*it));
  }

  w->defineSet(setName, nameSet);
}

int auxUtil::getItemType(TString item){
  if(item.Contains("::")&&item.Contains("(")) return FUNCTION; // Function or pdf
  else if(item.Contains("[")) return VARIABLE;			// Variable
  else return EXIST;					// Existing obj in workspace
}

TString auxUtil::translateItemType(int type){
  if(type==FUNCTION) return "function";
  else if(type==VARIABLE) return "variable";
  else if(type==EXIST) return "existing object";
  else return "unknown";
}

TString auxUtil::translateItemType(TString item){
  return translateItemType(getItemType(item));
}

TString auxUtil::getObjName(TString objName){
  int type=getItemType(objName);
  if(type==EXIST) return objName;
  
  if(type==VARIABLE){
    objName=objName(0,objName.First('['));
    // cout<<"\tREGTEST: Getting variable name "<<objName<<endl;
  }
  
  else{
    objName=objName(objName.First("::")+2,objName.First('(')-objName.First("::")-2);
    // cout<<"\tREGTEST: Getting function name "<<objName<<endl;
  }
  return objName;
}

TString auxUtil::getAttributeValue( TXMLNode* rootNode, TString attributeKey, bool allowEmpty, TString defaultStr){
  TXMLAttr* attr = auxUtil::findAttribute(rootNode, attributeKey);
  TString attributeValue;
  
  if(!attr){
    if(allowEmpty) attributeValue=defaultStr;
    else alertAndAbort("Attribute "+attributeKey+" cannot be found in node "+rootNode->GetNodeName());
  }
  else attributeValue = attr->GetValue();

  removeWhiteSpace(attributeValue);
  return attributeValue;
}

void auxUtil::alertAndAbort(TString msg){
  cerr<<"\n"<<WARNING<<" \tERROR: "<<msg<<". Please intervene..."<<ENDC<<endl<<endl;
  exit(-1);
}

void auxUtil::alert(TString msg){
  cerr<<"\n"<<WARNING<<" \tWARNING: "<<msg<<ENDC<<endl<<endl;
}

void auxUtil::setValAndFix(RooRealVar *var, double value){
  if ( var->getMax() < value ) {
    var->setMax(value + 1);
  } else if ( var->getMin() > value ) {
    var->setMin(value - 1);
  }
  var->setVal(value);
  var->setConstant(true);
}

int auxUtil::stripSign(TString &expr){
  int sign=1;
  if(expr.BeginsWith('-')||expr.BeginsWith('+')){
    if(expr.BeginsWith('-')) sign=-1;
    expr=expr(1,expr.Length());
  }
  return sign;
}

void auxUtil::removeWhiteSpace(TString& item){
  std::string s=item.Data();
  s.erase( std::remove_if( s.begin(), s.end(), std::bind( std::isspace<char>, std::placeholders::_1, std::locale::classic() ) ), s.end() );
  item=s.c_str();
}

vector<TString> auxUtil::decomposeFuncStr(TString function){
  vector<TString> itemList=splitString(function(function.First('(')+1,function.Last(')')-function.First('(')-1),',');
  if(function.Contains("expr::")) itemList.erase(itemList.begin()); // TODO: find other special syntax to be taken care of
  return itemList;
}

bool auxUtil::to_bool(TString str) {
  bool b;
  if(str.IsDigit()) b=str.Atoi();
  else{				// Is a string
    str.ToLower();
    std::istringstream is(str.Data());
    is >> std::boolalpha >> b;
  }
  return b;
}

TXMLNode *auxUtil::findNode(TXMLNode* rootNode, TString nodeName){
  TXMLNode* node=rootNode->GetChildren();
  while ( node != 0 ){
    if(nodeName==TString(node->GetNodeName())) return node;
    node=node->GetNextNode();
  }
  return NULL;
}

TXMLAttr *auxUtil::findAttribute(TXMLNode* rootNode, TString attributeKey){
  TListIter attribIt = rootNode->GetAttributes();
  TXMLAttr* curAttr = 0;

  while (( curAttr = dynamic_cast< TXMLAttr* >( attribIt() ) ) != 0 )
    if ( curAttr->GetName() == attributeKey ) return curAttr;

  return NULL;
}

bool auxUtil::checkExist(TString nameList) {
  vector<TString> fileNames = auxUtil::splitString(nameList, ',');
  for( auto name : fileNames ){
    if(FILE *file = fopen(name, "r")) fclose(file);
    else return false;
  }
  return true;
}

vector<TString> auxUtil::diffSet(vector<TString> A, vector<TString> B){
  sort(A.begin(), A.end());
  sort(B.begin(), B.end());
  vector<TString> results;
  std::set_difference(A.begin(), A.end(),
		      B.begin(), B.end(),
		      std::back_inserter(results));
  return results;
}

TString auxUtil::readNumFromOption(TString opt, TString key){
  if(opt.Contains(key)){
    return opt(opt.First(key)+key.Length(), opt.Length());
  }
  else return "";
}

int auxUtil::findBin(TH1 *h, double lowedge){
  int nbin=h->GetNbinsX();
  if(lowedge<h->GetBinLowEdge(1)&&fabs(lowedge-h->GetBinLowEdge(1)>epsilon)) return 0;
  if(lowedge>h->GetBinLowEdge(nbin)&&fabs(lowedge-h->GetBinLowEdge(nbin)>epsilon)) return nbin+1;
  for(int ibin=1;ibin<=nbin;ibin++){
    double temp=h->GetBinLowEdge(ibin);
    if(fabs(temp-lowedge)<epsilon) return ibin;
  }
  return -1;
}

TStyle* auxUtil::ATLASStyle() 
{
  TStyle *atlasStyle = new TStyle("ATLAS","Atlas style");

  // use plain black on white colors
  Int_t icol=0; // WHITE
  atlasStyle->SetFrameBorderMode(icol);
  atlasStyle->SetFrameFillColor(icol);
  atlasStyle->SetCanvasBorderMode(icol);
  atlasStyle->SetCanvasColor(icol);
  atlasStyle->SetPadBorderMode(icol);
  atlasStyle->SetPadColor(icol);
  atlasStyle->SetStatColor(icol);
  //atlasStyle->SetFillColor(icol); // don't use: white fill color for *all* objects

  // set the paper & margin sizes
  atlasStyle->SetPaperSize(20,26);

  // set margin sizes
  atlasStyle->SetPadTopMargin(0.05);
  atlasStyle->SetPadRightMargin(0.05);
  atlasStyle->SetPadBottomMargin(0.16);
  atlasStyle->SetPadLeftMargin(0.16);

  // set title offsets (for axis label)
  atlasStyle->SetTitleXOffset(1.1);
  atlasStyle->SetTitleYOffset(1.3);

  // use large fonts
  //Int_t font=72; // Helvetica italics
  Int_t font=42; // Helvetica
  Double_t tsize=0.05; // originally 0.05
  atlasStyle->SetTextFont(font);

  atlasStyle->SetTextSize(tsize);
  atlasStyle->SetLabelFont(font,"x");
  atlasStyle->SetTitleFont(font,"x");
  atlasStyle->SetLabelFont(font,"y");
  atlasStyle->SetTitleFont(font,"y");
  atlasStyle->SetLabelFont(font,"z");
  atlasStyle->SetTitleFont(font,"z");
  
  atlasStyle->SetLabelSize(tsize,"x");
  atlasStyle->SetTitleSize(tsize,"x");
  atlasStyle->SetLabelSize(tsize,"y");
  atlasStyle->SetTitleSize(tsize,"y");
  atlasStyle->SetLabelSize(tsize,"z");
  atlasStyle->SetTitleSize(tsize,"z");

  // use bold lines and markers
  atlasStyle->SetMarkerStyle(20);
  atlasStyle->SetMarkerSize(1.2);
  atlasStyle->SetHistLineWidth((Width_t)3.0);
  atlasStyle->SetLineStyleString(2,"[12 12]"); // postscript dashes

  // get rid of X error bars 
  //atlasStyle->SetErrorX(0.001);
  // get rid of error bar caps
  atlasStyle->SetEndErrorSize(0.);

  // do not display any of the standard histogram decorations
  atlasStyle->SetOptTitle(0);
  //atlasStyle->SetOptStat(1111);
  atlasStyle->SetOptStat(0);
  //atlasStyle->SetOptFit(1111);
  atlasStyle->SetOptFit(0);

  // put tick marks on top and RHS of plots
  atlasStyle->SetPadTickX(1);
  atlasStyle->SetPadTickY(1);

  return atlasStyle;

}

void auxUtil::setATLASStyle()
{
  std::cout << "\nApplying ATLAS style settings...\n" << std::endl ;
  ATLASStyle();
  gROOT->SetStyle("ATLAS");
  gROOT->ForceStyle();
}

int auxUtil::getNDOF(RooAbsPdf *pdf, RooRealVar *x, bool exclSyst){
  RooArgSet *params=pdf->getVariables();
  unique_ptr<RooAbsPdf> nuispdf(RooStats::MakeNuisancePdf(*pdf, RooArgSet(*x), "nuisancePdf"));
  unique_ptr<TIterator> iter(params->createIterator());
  RooRealVar *var=NULL;
  int counter=0;
  while((var=(RooRealVar*)iter->Next()))
    if(!var->isConstant()
       && var->GetName() != x->GetName()
       && (exclSyst && !nuispdf->dependsOn(RooArgSet(*var)))) counter++;

  return counter;
}

pair<double, int> auxUtil::calcChi2(TH1* hdata, TH1* hpdf, double blindMin, double blindMax, double threshold){
  if(hdata->GetNbinsX() != hpdf->GetNbinsX()) auxUtil::alertAndAbort("Number of bins do not match between data and pdf histograms used for chi2 calculation");
  double chi2 = 0, content_data_chi2 = 0, content_pdf_chi2 = 0, error2_data_chi2 = 0, last_increment = 0;
  int nbin_chi2 = 0, last_increment_bin = 1;

  bool goBlind = (blindMin < blindMax) && ( (blindMin > hdata->GetXaxis()->GetXmin()) || (blindMax < hdata->GetXaxis()->GetXmax()) );
  for( int ibin = 1 ; ibin <= hdata->GetNbinsX(); ibin ++ ){
    if(goBlind && hdata->GetBinCenter(ibin) > blindMin && hdata->GetBinCenter(ibin) < blindMax ) continue;

    content_data_chi2 += hdata->GetBinContent(ibin);
    content_pdf_chi2 += hpdf->GetBinContent(ibin);
    error2_data_chi2 += pow(hdata->GetBinError(ibin), 2);
      
    if( content_data_chi2/sqrt(error2_data_chi2) < threshold || fabs(content_data_chi2) < auxUtil::epsilon ){ // Less than 3 sigma from 0 (if it is data it is 9 events)
      if(ibin<hdata->GetNbinsX()) continue; // Not the last bin yet, continue aggregating
      else{			// Reached last bin but still did not get 10 events, then merge back to last increment
	chi2 -= last_increment; // Subtract out last increment first
	content_data_chi2 = hdata->IntegralAndError(last_increment_bin, hdata->GetNbinsX(), error2_data_chi2);
	error2_data_chi2 *= error2_data_chi2;
	content_pdf_chi2 = hpdf->Integral(last_increment_bin, hdata->GetNbinsX());
	chi2 += pow( (content_data_chi2-content_pdf_chi2)/sqrt(error2_data_chi2), 2 );
	if(nbin_chi2==0) nbin_chi2++; // Corner case where the total number of data events is less than 10, in which case there should be one bin
      }
    }
    else{
      last_increment = pow( (content_data_chi2-content_pdf_chi2)/sqrt(error2_data_chi2), 2 );
      last_increment_bin = ibin;
      chi2 += last_increment;
      nbin_chi2++;
      content_data_chi2 = 0;
      content_pdf_chi2 = 0;
      error2_data_chi2 = 0;
    }
  }
  return make_pair(chi2, nbin_chi2);
}

void auxUtil::printTime(){
  time_t result=time(nullptr);
  cout<<asctime(localtime(&result));
}

void auxUtil::removeDuplicatedString(vector<TString>& strArr){
  sort(strArr.begin(), strArr.end());
  strArr.erase(unique(strArr.begin(), strArr.end()), strArr.end());
}

void auxUtil::removeString(vector<TString>& strArr, TString target){
  strArr.erase(remove( strArr.begin(), strArr.end(), target), strArr.end());
}

void auxUtil::closeFuncExpr(TString &expr){
  expr+=")";
  removeWhiteSpace(expr);
  expr.ReplaceAll(",)",")");
}

void auxUtil::collectEverything(ModelConfig *mc, RooArgSet *set){
  if(mc->GetNuisanceParameters()) set->add(*mc->GetNuisanceParameters());
  if(mc->GetGlobalObservables()) set->add(*mc->GetGlobalObservables());
  if(mc->GetParametersOfInterest()) set->add(*mc->GetParametersOfInterest());
}

TString auxUtil::combineName(TString name, TString tag){
  return name+"_"+tag;
}

TString auxUtil::implementObj(RooWorkspace *w, TString expr, bool checkExistBeforeImp){
  // If the obj is claimed to exist, but actually not, then abort.
  int type=auxUtil::getItemType(expr);
  if(type==auxUtil::EXIST){
    if(!w->arg(expr)) auxUtil::alertAndAbort("object "+expr+" does not exist");
    else return expr;
  }
  
  // If the variable is believed to have some chance to exist, we should check.
  TString varName=auxUtil::getObjName(expr);
  if(checkExistBeforeImp){
    if(w->arg(varName)){
      return varName;
    }
  }

  // Otherwise we just blindly implement
  if(!w->factory(expr)) auxUtil::alertAndAbort("Creation of expression "+expr+" failed");
  
  return varName;
}
