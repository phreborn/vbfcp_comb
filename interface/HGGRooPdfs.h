/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitModels                                                     *
 *    File: $Id: HGGRooPdfs.h 157158 2015-05-19 05:40:16Z yanght $
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2005, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/
#ifndef HiggsAnalysis_CombinedLimit_HGGRooPdfs_h
#define HiggsAnalysis_CombinedLimit_HGGRooPdfs_h

#include "RooAbsPdf.h"
#include "RooRealProxy.h"

class RooRealVar;
class RooAbsReal;

class RooPower : public RooAbsPdf {
public:
  RooPower() {} ;
  RooPower(const char *name, const char *title,
		 RooAbsReal& _x, RooAbsReal& _c);
  RooPower(const RooPower& other, const char* name=0);
  virtual TObject* clone(const char* newname) const { return new RooPower(*this,newname); }
  inline virtual ~RooPower() { }

  Int_t getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars, const char* rangeName=0) const ;
  Double_t analyticalIntegral(Int_t code, const char* rangeName=0) const ;
  const RooAbsReal & base() const { return x.arg(); }
  const RooAbsReal & exponent() const { return c.arg(); }

protected:
  RooRealProxy x;
  RooRealProxy c;

  Double_t evaluate() const;

private:
  ClassDef(RooPower,1) // Exponential PDF
};

#endif
