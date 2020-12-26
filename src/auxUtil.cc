#include "auxUtil.hh"

void auxUtil::printTitle(TString titleText, TString separator, int width)
{
  TString fullLine = "", line = "";

  int stringLength = titleText.Length();
  int fullLineWidth = 2 * width + ((stringLength > 2 * width) ? (stringLength) : (2 * width)) + 2;

  for (int i = 0; i < fullLineWidth; i++)
    fullLine += separator;
  for (int i = 0; i < width; i++)
    line += separator;

  spdlog::info("{}", fullLine.Data());
  if (stringLength > 2 * width)
    spdlog::info("{} {} {}", line.Data(), titleText.Data(), line.Data());
  else
    spdlog::info(Form("" + line + " %*s%*s " + line, int(width + titleText.Length() / 2), titleText.Data(), int(width - titleText.Length() / 2), ""));
  spdlog::info("{}", fullLine.Data());
}

void auxUtil::printLine(TString separator, int width)
{
  TString line = "";

  for (int i = 0; i < width; i++)
    line += separator;

  spdlog::info("{}", line.Data());
}

void auxUtil::Reset(RooArgSet *original, RooArgSet *snapshot)
{
  *original = *snapshot;
  // Still need to recover the ranges for variables
  unique_ptr<TIterator> iter(original->createIterator());
  RooRealVar *parg = NULL;
  while ((parg = dynamic_cast<RooRealVar *>(iter->Next())))
  {
    RooRealVar *snapVar = dynamic_cast<RooRealVar *>(snapshot->find(parg->GetName()));
    parg->setRange(snapVar->getMin(), snapVar->getMax());
  }
}

int auxUtil::parseXMLFile(TDOMParser *xmlparser, TString inputFile)
{
  Int_t parseError = xmlparser->ParseFile(inputFile);

  if (parseError)
  {
    spdlog::error("Loading of xml document {} failed", inputFile.Data());
  }
  return parseError;
}

vector<TString> auxUtil::splitString(const TString &theOpt, const char separator)
{
  // splits the option string at 'separator' and fills the list
  // 'splitV' with the primitive strings
  vector<TString> splitV;
  if (theOpt == "")
    return splitV; // If the string is empty, the vector should be empty as well

  TString splitOpt(theOpt);
  splitOpt.ReplaceAll("\n", " ");
  splitOpt = splitOpt.Strip(TString::kBoth, separator);
  while (splitOpt.Length() > 0)
  {
    if (!splitOpt.Contains(separator))
    {
      splitV.push_back(splitOpt);
      break;
    }
    else
    {
      TString toSave = splitOpt(0, splitOpt.First(separator));
      splitV.push_back(toSave);
      splitOpt = splitOpt(splitOpt.First(separator), splitOpt.Length());
    }
    splitOpt = splitOpt.Strip(TString::kLeading, separator);
  }
  return splitV;
}

TString auxUtil::generateExpr(TString head, RooArgSet *set, bool closeExpr)
{
  TString exprStr = head;
  unique_ptr<TIterator> iter(set->createIterator());
  RooAbsArg *parg = NULL;
  while ((parg = dynamic_cast<RooAbsArg *>(iter->Next())))
  {
    exprStr += TString(parg->GetName()) + ",";
  }
  if (closeExpr)
    closeFuncExpr(exprStr);
  return exprStr;
}

void auxUtil::defineSet(RooWorkspace *w, RooArgSet *set, TString setName)
{
  unique_ptr<TIterator> iter(set->createIterator());
  RooRealVar *parg = NULL;
  RooArgSet nameSet;
  while ((parg = dynamic_cast<RooRealVar *>(iter->Next())))
  {
    if (w->var(parg->GetName()))
      nameSet.add(*w->var(parg->GetName()));
  }

  w->defineSet(setName, nameSet);
}

RooArgSet *auxUtil::findArgSetIn(RooWorkspace *w, RooArgSet *set, bool strict)
{
    TString setName = set->GetName();
    RooArgSet *outSet = new RooArgSet(setName);
    std::unique_ptr<TIterator> iter(set->createIterator());

    for (RooAbsArg *v = dynamic_cast<RooAbsArg *>(iter->Next()); v != 0; v = dynamic_cast<RooAbsArg *>(iter->Next()))
    {
        RooAbsArg *inV = w->arg(v->GetName());
        if (!inV)
        {
          spdlog::warn("No variable {} in workspace {}", v->GetName(), w->GetName());
          if(strict) throw std::runtime_error("Missing object");
          continue;
        }
        outSet->add(*inV, true);
    }

    return outSet;
}

void auxUtil::defineSet(RooWorkspace *w, vector<TString> set, TString setName)
{
  RooArgSet nameSet;
  for (vector<TString>::iterator it = set.begin(); it != set.end(); ++it)
  {
    if (w->var(*it))
      nameSet.add(*w->var(*it));
  }

  w->defineSet(setName, nameSet);
}

int auxUtil::getItemType(TString item)
{
  if (item.Contains("::") && item.Contains("("))
    return FUNCTION; // Function or pdf
  else if (item.Contains('('))
    return CONSTR;
  else if (item.Contains("["))
    return VARIABLE; // Variable
  else
    return EXIST; // Existing obj in workspace
}

TString auxUtil::translateItemType(int type)
{
  if (type == FUNCTION)
    return "function";
  else if (type == VARIABLE)
    return "variable";
  else if (type == EXIST)
    return "existing object";
  else
    return "unknown";
}

TString auxUtil::translateItemType(TString item)
{
  return translateItemType(getItemType(item));
}

TString auxUtil::getObjName(TString objName)
{
  int type = getItemType(objName);
  if (type == EXIST)
    return objName;

  if (type == VARIABLE)
  {
    objName = objName(0, objName.First('['));
    // cout<<"\tREGTEST: Getting variable name "<<objName<<endl;
  }
  else if (type == CONSTR)
  {
    objName = objName(0, objName.First('('));
  }
  else
  {
    objName = objName(objName.First("::") + 2, objName.First('(') - objName.First("::") - 2);
    // cout<<"\tREGTEST: Getting function name "<<objName<<endl;
  }
  return objName;
}

TString auxUtil::getAttributeValue(TXMLNode *rootNode, TString attributeKey, bool allowEmpty, TString defaultStr)
{
  TXMLAttr *attr = auxUtil::findAttribute(rootNode, attributeKey);
  TString attributeValue;

  if (!attr)
  {
    if (allowEmpty)
      attributeValue = defaultStr;
    else
      alertAndAbort("Attribute " + attributeKey + " cannot be found in node " + rootNode->GetNodeName());
  }
  else
    attributeValue = attr->GetValue();

  removeWhiteSpace(attributeValue);
  return attributeValue;
}

void auxUtil::alertAndAbort(TString msg, TString error)
{
  spdlog::error(msg.Data());
  throw std::runtime_error(error);
}

void auxUtil::setValAndFix(RooRealVar *var, double value)
{
  if (var->getMax() < value)
  {
    var->setMax(value + 1);
  }
  else if (var->getMin() > value)
  {
    var->setMin(value - 1);
  }
  var->setVal(value);
  var->setConstant(true);
}

int auxUtil::stripSign(TString &expr)
{
  int sign = 1;
  if (expr.BeginsWith('-') || expr.BeginsWith('+'))
  {
    if (expr.BeginsWith('-'))
      sign = -1;
    expr = expr(1, expr.Length());
  }
  return sign;
}

void auxUtil::removeWhiteSpace(TString &item)
{
  std::string s = item.Data();
  s.erase(std::remove_if(s.begin(), s.end(), std::bind(std::isspace<char>, std::placeholders::_1, std::locale::classic())), s.end());
  item = s.c_str();
}

vector<TString> auxUtil::decomposeStr(TString function, const char separation, const int bracketType)
{
  char first, last;
  if (bracketType == ROUND)
  {
    first = '(';
    last = ')';
  }
  else if (bracketType == SQUARE)
  {
    first = '[';
    last = ']';
  }
  else
  {
    alertAndAbort("Unknown bracket type " + to_string(bracketType));
  }

  if (function.CountChar(first) > 1 || function.CountChar(last) > 1)
    alertAndAbort("Invalid expression " + function + ". Please check your input config");

  vector<TString> itemList = splitString(function(function.First(first) + 1, function.Last(last) - function.First(first) - 1), separation);
  /* Include also the object name */
  itemList.push_back(function(0, function.First(first)));
  return itemList;
}

bool auxUtil::to_bool(TString str)
{
  bool b;
  if (str.IsDigit())
    b = str.Atoi();
  else
  { // Is a string
    str.ToLower();
    std::istringstream is(str.Data());
    is >> std::boolalpha >> b;
  }
  return b;
}

TXMLNode *auxUtil::findNode(TXMLNode *rootNode, TString nodeName)
{
  TXMLNode *node = rootNode->GetChildren();
  while (node != 0)
  {
    if (nodeName == TString(node->GetNodeName()))
      return node;
    node = node->GetNextNode();
  }
  return NULL;
}

TXMLAttr *auxUtil::findAttribute(TXMLNode *rootNode, TString attributeKey)
{
  TListIter attribIt = rootNode->GetAttributes();
  TXMLAttr *curAttr = 0;

  while ((curAttr = dynamic_cast<TXMLAttr *>(attribIt())) != 0)
    if (curAttr->GetName() == attributeKey)
      return curAttr;

  return NULL;
}

bool auxUtil::checkExist(TString nameList)
{
  vector<TString> fileNames = auxUtil::splitString(nameList, ',');
  for (auto name : fileNames)
  {
    if (FILE *file = fopen(name, "r"))
      fclose(file);
    else
      return false;
  }
  return true;
}

vector<TString> auxUtil::diffSet(vector<TString> A, vector<TString> B)
{
  sort(A.begin(), A.end());
  sort(B.begin(), B.end());
  vector<TString> results;
  std::set_difference(A.begin(), A.end(),
                      B.begin(), B.end(),
                      std::back_inserter(results));
  return results;
}

TString auxUtil::readNumFromOption(TString opt, TString key)
{
  if (opt.Contains(key))
  {
    return opt(opt.First(key) + key.Length(), opt.Length());
  }
  else
    return "";
}

int auxUtil::findBin(TH1 *h, double lowedge)
{
  int nbin = h->GetNbinsX();
  if (lowedge < h->GetBinLowEdge(1) && fabs(lowedge - h->GetBinLowEdge(1) > epsilon))
    return 0;
  if (lowedge > h->GetBinLowEdge(nbin) && fabs(lowedge - h->GetBinLowEdge(nbin) > epsilon))
    return nbin + 1;
  for (int ibin = 1; ibin <= nbin; ibin++)
  {
    double temp = h->GetBinLowEdge(ibin);
    if (fabs(temp - lowedge) < epsilon)
      return ibin;
  }
  return -1;
}

TStyle *auxUtil::ATLASStyle()
{
  TStyle *atlasStyle = new TStyle("ATLAS", "Atlas style");

  // use plain black on white colors
  Int_t icol = 0; // WHITE
  atlasStyle->SetFrameBorderMode(icol);
  atlasStyle->SetFrameFillColor(icol);
  atlasStyle->SetCanvasBorderMode(icol);
  atlasStyle->SetCanvasColor(icol);
  atlasStyle->SetPadBorderMode(icol);
  atlasStyle->SetPadColor(icol);
  atlasStyle->SetStatColor(icol);
  //atlasStyle->SetFillColor(icol); // don't use: white fill color for *all* objects

  // set the paper & margin sizes
  atlasStyle->SetPaperSize(20, 26);

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
  Int_t font = 42;       // Helvetica
  Double_t tsize = 0.05; // originally 0.05
  atlasStyle->SetTextFont(font);

  atlasStyle->SetTextSize(tsize);
  atlasStyle->SetLabelFont(font, "x");
  atlasStyle->SetTitleFont(font, "x");
  atlasStyle->SetLabelFont(font, "y");
  atlasStyle->SetTitleFont(font, "y");
  atlasStyle->SetLabelFont(font, "z");
  atlasStyle->SetTitleFont(font, "z");

  atlasStyle->SetLabelSize(tsize, "x");
  atlasStyle->SetTitleSize(tsize, "x");
  atlasStyle->SetLabelSize(tsize, "y");
  atlasStyle->SetTitleSize(tsize, "y");
  atlasStyle->SetLabelSize(tsize, "z");
  atlasStyle->SetTitleSize(tsize, "z");

  // use bold lines and markers
  atlasStyle->SetMarkerStyle(20);
  atlasStyle->SetMarkerSize(1.2);
  atlasStyle->SetHistLineWidth((Width_t)3.0);
  atlasStyle->SetLineStyleString(2, "[12 12]"); // postscript dashes

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
  std::cout << "\nApplying ATLAS style settings...\n"
            << std::endl;
  ATLASStyle();
  gROOT->SetStyle("ATLAS");
  gROOT->ForceStyle();
}

int auxUtil::getNDOF(RooAbsPdf *pdf, RooRealVar *x, bool exclSyst)
{
  RooArgSet *params = pdf->getVariables();
  unique_ptr<RooAbsPdf> nuispdf(RooStats::MakeNuisancePdf(*pdf, RooArgSet(*x), "nuisancePdf"));
  unique_ptr<TIterator> iter(params->createIterator());
  RooRealVar *var = NULL;
  int counter = 0;
  while ((var = (RooRealVar *)iter->Next()))
    if (!var->isConstant() && var->GetName() != x->GetName() && (exclSyst && !nuispdf->dependsOn(RooArgSet(*var))))
      counter++;

  return counter;
}

pair<double, int> auxUtil::calcChi2(TH1 *hdata, TH1 *hpdf, double blindMin, double blindMax, double threshold)
{
  if (hdata->GetNbinsX() != hpdf->GetNbinsX())
    auxUtil::alertAndAbort("Number of bins do not match between data and pdf histograms used for chi2 calculation");
  double chi2 = 0, content_data_chi2 = 0, content_pdf_chi2 = 0, error2_data_chi2 = 0, last_increment = 0;
  int nbin_chi2 = 0, last_increment_bin = 1;

  bool goBlind = (blindMin < blindMax) && ((blindMin > hdata->GetXaxis()->GetXmin()) || (blindMax < hdata->GetXaxis()->GetXmax()));
  for (int ibin = 1; ibin <= hdata->GetNbinsX(); ibin++)
  {
    if (goBlind && hdata->GetBinCenter(ibin) > blindMin && hdata->GetBinCenter(ibin) < blindMax)
      continue;

    content_data_chi2 += hdata->GetBinContent(ibin);
    content_pdf_chi2 += hpdf->GetBinContent(ibin);
    error2_data_chi2 += pow(hdata->GetBinError(ibin), 2);

    if (content_data_chi2 / sqrt(error2_data_chi2) < threshold || fabs(content_data_chi2) < auxUtil::epsilon)
    { // Less than 3 sigma from 0 (if it is data it is 9 events)
      if (ibin < hdata->GetNbinsX())
        continue; // Not the last bin yet, continue aggregating
      else
      {                         // Reached last bin but still did not get 10 events, then merge back to last increment
        chi2 -= last_increment; // Subtract out last increment first
        content_data_chi2 = hdata->IntegralAndError(last_increment_bin, hdata->GetNbinsX(), error2_data_chi2);
        error2_data_chi2 *= error2_data_chi2;
        content_pdf_chi2 = hpdf->Integral(last_increment_bin, hdata->GetNbinsX());
        chi2 += pow((content_data_chi2 - content_pdf_chi2) / sqrt(error2_data_chi2), 2);
        if (nbin_chi2 == 0)
          nbin_chi2++; // Corner case where the total number of data events is less than 10, in which case there should be one bin
      }
    }
    else
    {
      last_increment = pow((content_data_chi2 - content_pdf_chi2) / sqrt(error2_data_chi2), 2);
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

void auxUtil::printTime()
{
  time_t result = time(nullptr);
  cout << asctime(localtime(&result));
}

void auxUtil::removeDuplicatedString(vector<TString> &strArr)
{
  sort(strArr.begin(), strArr.end());
  strArr.erase(unique(strArr.begin(), strArr.end()), strArr.end());
}

void auxUtil::removeString(vector<TString> &strArr, TString target)
{
  strArr.erase(remove(strArr.begin(), strArr.end(), target), strArr.end());
}

void auxUtil::closeFuncExpr(TString &expr)
{
  expr += ")";
  removeWhiteSpace(expr);
  expr.ReplaceAll(",)", ")");
}

void auxUtil::collectEverything(ModelConfig *mc, RooArgSet *set)
{
  if (mc->GetNuisanceParameters())
    set->add(*mc->GetNuisanceParameters());
  if (mc->GetGlobalObservables())
    set->add(*mc->GetGlobalObservables());
  if (mc->GetParametersOfInterest())
    set->add(*mc->GetParametersOfInterest());
}

TString auxUtil::combineName(TString name, TString tag)
{
  return name + "_" + tag;
}

TString auxUtil::implementObj(RooWorkspace *w, TString expr, bool checkExistBeforeImp)
{
  // If the obj is claimed to exist, but actually not, then abort.
  int type = auxUtil::getItemType(expr);
  if (type == auxUtil::EXIST)
  {
    if (!w->arg(expr))
      auxUtil::alertAndAbort("object " + expr + " does not exist");
    else
      return expr;
  }

  // If the variable is believed to have some chance to exist, we should check.
  TString varName = auxUtil::getObjName(expr);
  if (checkExistBeforeImp)
  {
    if (w->arg(varName))
    {
      return varName;
    }
  }

  // Otherwise we just blindly implement
  if (!w->factory(expr))
    auxUtil::alertAndAbort("Creation of expression " + expr + " failed");

  return varName;
}

void auxUtil::copyAttributes(const RooAbsArg &from, RooAbsArg &to)
{
  if (&from == &to)
    return;

  const std::set<std::string> attribs = from.attributes();

  if (!attribs.empty())
  {
    for (std::set<std::string>::const_iterator it = attribs.begin(), ed = attribs.end(); it != ed; ++it)
      to.setAttribute(it->c_str());
  }

  const std::map<std::string, std::string> strattribs = from.stringAttributes();

  if (!strattribs.empty())
  {
    for (std::map<std::string, std::string>::const_iterator it = strattribs.begin(), ed = strattribs.end(); it != ed; ++it)
      to.setStringAttribute(it->first.c_str(), it->second.c_str());
  }
}

RooAbsPdf *auxUtil::factorizePdf(const RooArgSet &observables, RooAbsPdf &pdf, RooArgList &constraints)
{
  const std::type_info &id = typeid(pdf);

  if (id == typeid(RooProdPdf))
  {
    //std::cout << " pdf is product pdf " << pdf.GetName() << std::endl;
    RooProdPdf *prod = dynamic_cast<RooProdPdf *>(&pdf);
    RooArgList newFactors;
    RooArgSet newOwned;
    RooArgList list(prod->pdfList());
    bool needNew = false;

    for (int i = 0, n = list.getSize(); i < n; ++i)
    {
      RooAbsPdf *pdfi = (RooAbsPdf *)list.at(i);
      RooAbsPdf *newpdf = factorizePdf(observables, *pdfi, constraints);

      //std::cout << "    for " << pdfi->GetName() << "   newpdf  " << (newpdf == 0 ? "null" : (newpdf == pdfi ? "old" : "new"))  << std::endl;
      if (newpdf == 0)
      {
        needNew = true;
        continue;
      }

      if (newpdf != pdfi)
      {
        needNew = true;
        newOwned.add(*newpdf);
      }

      newFactors.add(*newpdf);
    }

    if (!needNew)
    {
      copyAttributes(pdf, *prod);
      return prod;
    }
    else if (newFactors.getSize() == 0)
      return 0;
    else if (newFactors.getSize() == 1)
    {
      RooAbsPdf *ret = (RooAbsPdf *)newFactors.first()->Clone(TString::Format("%s_obsOnly", pdf.GetName()));
      copyAttributes(pdf, *ret);
      return ret;
    }

    RooProdPdf *ret = new RooProdPdf(TString::Format("%s_obsOnly", pdf.GetName()), "", newFactors);
    // newFactors.Print();
    ret->addOwnedComponents(newOwned);
    copyAttributes(pdf, *ret);
    return ret;
  }
  else if (id == typeid(RooSimultaneous))
  {
    RooSimultaneous *sim = dynamic_cast<RooSimultaneous *>(&pdf);
    RooAbsCategoryLValue *cat = (RooAbsCategoryLValue *)sim->indexCat().Clone();
    int nbins = cat->numBins((const char *)0);
    TObjArray factorizedPdfs(nbins);
    RooArgSet newOwned;
    bool needNew = false;

    for (int ic = 0, nc = nbins; ic < nc; ++ic)
    {
      cat->setBin(ic);
      RooAbsPdf *pdfi = sim->getPdf(cat->getLabel());
      RooAbsPdf *newpdf = factorizePdf(observables, *pdfi, constraints);
      factorizedPdfs[ic] = newpdf;

      if (newpdf == 0)
      {
        throw std::runtime_error(std::string("ERROR: channel ") + cat->getLabel() + " factorized to zero.");
      }

      if (newpdf != pdfi)
      {
        needNew = true;
        newOwned.add(*newpdf);
      }
    }

    RooSimultaneous *ret = sim;

    if (needNew)
    {
      ret = new RooSimultaneous(TString::Format("%s_obsOnly", pdf.GetName()), "", (RooAbsCategoryLValue &)sim->indexCat());

      for (int ic = 0, nc = nbins; ic < nc; ++ic)
      {
        cat->setBin(ic);
        RooAbsPdf *newpdf = (RooAbsPdf *)factorizedPdfs[ic];

        if (newpdf)
          ret->addPdf(*newpdf, cat->getLabel());
      }

      ret->addOwnedComponents(newOwned);
    }

    delete cat;
    copyAttributes(pdf, *ret);
    return ret;
  }
  else if (pdf.dependsOn(observables))
  {
    return &pdf;
  }
  else
  {
    if (!constraints.contains(pdf))
      constraints.add(pdf);

    return 0;
  }
}

void auxUtil::linkMap(
    std::map<TString, TString> &attMap,
    TString &keyStr,
    TString &valueStr,
    TString linker)
{
  int mapSize = (int)attMap.size();
  int index = 0;
  typedef std::map<TString, TString>::iterator it_type;
  for (it_type iterator = attMap.begin(); iterator != attMap.end(); iterator++)
  {
    keyStr += iterator->first;
    valueStr += iterator->second;
    if (index < (mapSize - 1))
    {
      keyStr += linker;
      valueStr += linker;
    }
    index += 1;
  }
}

void auxUtil::getBasePdf(RooProdPdf *pdf, RooArgSet &set)
{
  RooArgList pdfList = pdf->pdfList();
  int pdfSize = pdfList.getSize();
  if (pdfSize == 1)
  {
    set.add(pdfList);
  }
  else
  {
    std::unique_ptr<TIterator> iter(pdfList.createIterator());
    RooAbsArg *arg;
    while ((arg = (RooAbsArg *)iter->Next()))
    {
      TString className = arg->ClassName();
      if (className != "RooProdPdf")
      {
        set.add(*arg);
      }
      else
      {
        RooProdPdf *thisPdf = dynamic_cast<RooProdPdf *>(arg);
        assert(thisPdf);
        getBasePdf(thisPdf, set);
      }
    }
    iter = NULL;
  }
}
