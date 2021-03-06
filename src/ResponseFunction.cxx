// @(#)root/roostats:$Id:  cranmer $
// Author: Kyle Cranmer, Akira Shibata
// Author: Giovanni Petrucciani (UCSD) (log-interpolation)
// Modified by Hongtao Yang for xmlAnaWSBuilder
/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//_________________________________________________
/*
BEGIN_HTML
<p>
</p>
END_HTML
*/
//

#include "RooFit.h"

#include "Riostream.h"
#include <math.h>
#include "TMath.h"

#include "RooAbsReal.h"
#include "RooRealVar.h"
#include "RooArgList.h"
#include "RooMsgService.h"
#include "RooTrace.h"

#include "TMath.h"

#include "ResponseFunction.h"

using namespace std;

ClassImp(ResponseFunction)

//_____________________________________________________________________________
ResponseFunction::ResponseFunction()
{
  // Default constructor
  _paramIter = _paramList.createIterator();
  _lowIter = _lowList.createIterator();
  _highIter = _highList.createIterator();
  _interpBoundary = 1.;
  _logInit = kFALSE;
  TRACE_CREATE
}

//_____________________________________________________________________________
ResponseFunction::ResponseFunction(const char *name, const char *title,
                                   const RooArgList &paramList,
                                   std::vector<double> nominal, const RooArgList &lowList, const RooArgList &highList, std::vector<int> code)
    : RooAbsReal(name, title),
      _paramList("paramList", "List of paramficients", this),
      _nominal(nominal),
      _lowList("lowList", "List of low variation parameters", this),
      _highList("highList", "List of high variation parameters", this),
      _interpCode(code),
      _interpBoundary(1.)
{

  _logInit = kFALSE;

  _paramIter = _paramList.createIterator();

  unique_ptr<TIterator> paramIter(paramList.createIterator());
  RooAbsArg *param;
  while ((param = (RooAbsArg *)paramIter->Next()))
  {
    if (!dynamic_cast<RooAbsReal *>(param))
    {
      coutE(InputArguments) << "ResponseFunction::ctor(" << GetName() << ") ERROR: paramficient " << param->GetName()
                            << " is not of type RooAbsReal" << endl;
      assert(0);
    }
    _paramList.add(*param);
  }

  _lowIter = _lowList.createIterator();

  unique_ptr<TIterator> lowIter(lowList.createIterator());
  RooAbsArg *low;
  while ((low = (RooAbsArg *)lowIter->Next()))
  {
    if (!dynamic_cast<RooAbsReal *>(low))
    {
      coutE(InputArguments) << "ResponseFunction::ctor(" << GetName() << ") ERROR: paramficient " << low->GetName()
                            << " is not of type RooAbsReal" << endl;
      assert(0);
    }
    _lowList.add(*low);
  }

  _highIter = _highList.createIterator();

  unique_ptr<TIterator> highIter(highList.createIterator());
  RooAbsArg *high;
  while ((high = (RooAbsArg *)highIter->Next()))
  {
    if (!dynamic_cast<RooAbsReal *>(high))
    {
      coutE(InputArguments) << "ResponseFunction::ctor(" << GetName() << ") ERROR: paramficient " << high->GetName()
                            << " is not of type RooAbsReal" << endl;
      assert(0);
    }
    _highList.add(*high);
  }

  TRACE_CREATE
}

//_____________________________________________________________________________
ResponseFunction::ResponseFunction(const ResponseFunction &other, const char *name)
    : RooAbsReal(other, name),
      _paramList("paramList", this, other._paramList),
      _nominal(other._nominal),
      _lowList("lowList", this, other._lowList),
      _highList("highList", this, other._highList),
      _interpCode(other._interpCode), _interpBoundary(other._interpBoundary)

{
  // Copy constructor
  _logInit = kFALSE;
  _paramIter = _paramList.createIterator();
  _lowIter = _lowList.createIterator();
  _highIter = _highList.createIterator();
  TRACE_CREATE
}

//_____________________________________________________________________________
ResponseFunction::~ResponseFunction()
{
  // Destructor
  delete _paramIter;
  delete _lowIter;
  delete _highIter;

  TRACE_DESTROY
}

//_____________________________________________________________________________
void ResponseFunction::setInterpCode(RooAbsReal &param, int code)
{

  int index = _paramList.index(&param);
  if (index < 0)
  {
    coutE(InputArguments) << "ResponseFunction::setInterpCode ERROR:  " << param.GetName()
                          << " is not in list" << endl;
  }
  else
  {
    coutW(InputArguments) << "ResponseFunction::setInterpCode :  " << param.GetName()
                          << " is now " << code << endl;
    _interpCode.at(index) = code;
  }
  // GHL: Adding suggestion by Swagato:
  _logInit = kFALSE;
  setValueDirty();
}

//_____________________________________________________________________________
void ResponseFunction::setAllInterpCodes(int code)
{

  for (unsigned int i = 0; i < _interpCode.size(); ++i)
  {
    _interpCode.at(i) = code;
  }
  // GHL: Adding suggestion by Swagato:
  _logInit = kFALSE;
  setValueDirty();
}

//_____________________________________________________________________________
double ResponseFunction::PolyInterpValue(int i, double x) const
{

  // code for polynomial interpolation used when interpCode=4

  double boundary = _interpBoundary;

  double x0 = boundary;

  // cache the polynomial coefficient values
  // which do not dpened on x but on the boundaries values
  if (!_logInit)
  {

    _logInit = kTRUE;

    unsigned int n = _lowList.getSize();
    assert(n == (unsigned int)_highList.getSize());

    _polCoeff.resize(n * 6);

    for (unsigned int j = 0; j < n; j++)
    {

      // location of the 6 coefficient for the j-th variable
      double *coeff = &_polCoeff[j * 6];
      double highVar = _nominal.at(j) + ((RooAbsReal *)(_highList.at(j)))->getVal();
      double lowVar = _nominal.at(j) + ((RooAbsReal *)(_lowList.at(j)))->getVal();

      // GHL: Swagato's suggestions
      double pow_up = std::pow(highVar / _nominal.at(j), x0);
      double pow_down = std::pow(lowVar / _nominal.at(j), x0);
      double logHi = std::log(highVar);
      double logLo = std::log(lowVar);
      double pow_up_log = highVar <= 0.0 ? 0.0 : pow_up * logHi;
      double pow_down_log = lowVar <= 0.0 ? 0.0 : -pow_down * logLo;
      double pow_up_log2 = highVar <= 0.0 ? 0.0 : pow_up_log * logHi;
      double pow_down_log2 = lowVar <= 0.0 ? 0.0 : -pow_down_log * logLo;

      double S0 = (pow_up + pow_down) / 2;
      double A0 = (pow_up - pow_down) / 2;
      double S1 = (pow_up_log + pow_down_log) / 2;
      double A1 = (pow_up_log - pow_down_log) / 2;
      double S2 = (pow_up_log2 + pow_down_log2) / 2;
      double A2 = (pow_up_log2 - pow_down_log2) / 2;

      //fcns+der+2nd_der are eq at bd

      // cache  coefficient of the polynomial
      coeff[0] = 1. / (8 * x0) * (15 * A0 - 7 * x0 * S1 + x0 * x0 * A2);
      coeff[1] = 1. / (8 * x0 * x0) * (-24 + 24 * S0 - 9 * x0 * A1 + x0 * x0 * S2);
      coeff[2] = 1. / (4 * pow(x0, 3)) * (-5 * A0 + 5 * x0 * S1 - x0 * x0 * A2);
      coeff[3] = 1. / (4 * pow(x0, 4)) * (12 - 12 * S0 + 7 * x0 * A1 - x0 * x0 * S2);
      coeff[4] = 1. / (8 * pow(x0, 5)) * (+3 * A0 - 3 * x0 * S1 + x0 * x0 * A2);
      coeff[5] = 1. / (8 * pow(x0, 6)) * (-8 + 8 * S0 - 5 * x0 * A1 + x0 * x0 * S2);
    }
  }

  // GHL: Swagato's suggestions
  // if( _low[i] == 0 ) _low[i] = 0.0001;
  // if( _high[i] == 0 ) _high[i] = 0.0001;

  // get pointer to location of coefficients in the vector
  const double *coeff = &_polCoeff.front() + 6 * i;

  double a = coeff[0];
  double b = coeff[1];
  double c = coeff[2];
  double d = coeff[3];
  double e = coeff[4];
  double f = coeff[5];

  // evaluate the 6-th degree polynomial using Horner's method
  double value = 1. + x * (a + x * (b + x * (c + x * (d + x * (e + x * f)))));
  return value;
}

//_____________________________________________________________________________
Double_t ResponseFunction::evaluate() const
{
  // Calculate and return value of polynomial

  Double_t total = 1;
  _paramIter->Reset();

  RooAbsReal *param;
  //const RooArgSet* nset = _paramList.nset() ;
  int i = 0;

  // TString name = GetName();
  // if (name == TString("ZHWW_ll12_vzll_epsilon") )
  //    //    std::cout << "evaluate flexible interp var - init flag is " << _logInit << std::endl;

  while ((param = (RooAbsReal *)_paramIter->Next()))
  {
    //    param->Print("v");

    Int_t icode = _interpCode[i];

    switch (icode)
    {

    case 0:
    {
      // piece-wise linear: only consider upper uncertainty as it should always be symmetric in XML workspace builder
      total *= _nominal.at(i) + param->getVal() * ((RooAbsReal *)(_highList.at(i)))->getVal();
      break;
    }
    case 1:
    {
      // pice-wise log: only consider upper uncertainty as it should always be symmetric in XML workspace builder
      total *= pow(1 + ((RooAbsReal *)(_highList.at(i)))->getVal() / _nominal.at(i), param->getVal());
      break;
    }
    case 4:
    {
      double boundary = _interpBoundary;
      double x = param->getVal();
      //std::cout << icode << " param " << param->GetName() << "  " << param->getVal() << " boundary " << boundary << std::endl;

      if (x >= boundary)
      {
        total *= std::pow(1 + ((RooAbsReal *)(_highList.at(i)))->getVal() / _nominal.at(i), param->getVal());
      }
      else if (x <= -boundary)
      {
        total *= std::pow(1 + ((RooAbsReal *)(_lowList.at(i)))->getVal() / _nominal.at(i), -param->getVal());
      }
      else if (x != 0)
      {
        total *= PolyInterpValue(i, x);
      }
      break;
    }
    default:
    {
      coutE(InputArguments) << "ResponseFunction::evaluate ERROR:  " << param->GetName()
                            << " with unknown interpolation code" << endl;
    }
    }
    ++i;
  }

  // if(total<=0) {
  //    total= TMath::Limits<double>::Min();
  // }

  return total;
}

void ResponseFunction::printMultiline(ostream &os, Int_t contents,
                                      Bool_t verbose, TString indent) const
{
  RooAbsReal::printMultiline(os, contents, verbose, indent);
  os << indent << "--- ResponseFunction ---" << endl;
  printResponseFunctions(os);
}

void ResponseFunction::printResponseFunctions(ostream &os) const
{
  _paramIter->Reset();
  for (int i = 0; i < (int)_paramList.getSize(); i++)
  {
    RooAbsReal *param = (RooAbsReal *)_paramIter->Next();
    os << setw(36) << param->GetName() << ": "
       << setw(7) << _nominal.at(i) << "  "
       << setw(7) << ((RooAbsReal *)(_lowList.at(i)))->getVal() << "  "
       << setw(7) << ((RooAbsReal *)(_highList.at(i)))->getVal()
       << setw(7) << _interpCode.at(i)
       << endl;
  }
}
