#!/usr/bin/python
# -*- coding: UTF-8 -*-

import sys

from ROOT import *
from math import sin
from array import array

gROOT.SetBatch()

gROOT.LoadMacro("AtlasStyle.C")
gROOT.LoadMacro("AtlasUtils.C")
gROOT.LoadMacro("AtlasLabels.C")

SetAtlasStyle()

gStyle.SetErrorX(0.5); # !!! must after SetAtlasStyle()
gStyle.SetOptStat(0)

def getNLLcurve(fnll):
  pnll = array('f') # precision lost in transformation to array
  vnll = array('f')
  with open(fnll, 'r') as f:
    for line in f.readlines():
      nll = line.split(' ')
      pnll.append(float(nll[0]))
      vnll.append(2*float(nll[1]))
  curve = TGraph(len(pnll), pnll, vnll)
  return curve

limL = -0.2
limR = 0.2

outName = "comb"
sysSet='allSys'

outdir = "./"

dirname = "Expected/"
inNLL = dirname + "dNLL_Comb_"+sysSet+".log"
gr1 = getNLLcurve(inNLL)

dirname = "Observed/"
inNLL = dirname + "dNLL_Comb_"+sysSet+".log"
#gr1 = TGraph(inNLL, '%lg %lg')
gr2 = getNLLcurve(inNLL)

dirname = "Expected/"
inNLL = dirname + "dNLL_Gam_"+sysSet+".log"
gr3 = getNLLcurve(inNLL)

dirname = "Observed/"
inNLL = dirname + "dNLL_Gam_"+sysSet+".log"
gr4 = getNLLcurve(inNLL)

dirname = "Expected/"
inNLL = dirname + "dNLL_Tau_"+sysSet+".log"
gr5 = getNLLcurve(inNLL)

dirname = "Observed/"
inNLL = dirname + "dNLL_Tau_"+sysSet+".log"
gr6 = getNLLcurve(inNLL)

c = TCanvas('c', '', 200, 10, 700, 560)

gr1.GetXaxis().SetLimits(limL, limR)
gr1.GetYaxis().SetRangeUser(-0.01, 10*2)
gr1.GetYaxis().SetTitle("2#times#DeltaNLL")
gr1.GetXaxis().SetTitle("#tilde{d}")
gr1.SetTitle("Negative Log Likelihood")

gr1.SetLineStyle(kDashed);
gr1.SetLineColorAlpha(kBlack, 0.7);
gr1.SetMarkerSize(0)
gr1.SetLineWidth(2)
gr1.Draw("")

gr2.SetMarkerSize(0.9);
gr2.SetLineWidth(2);
gr2.Draw("C same");

gr3.SetLineStyle(kDashed);
gr3.SetLineColorAlpha(kGreen, 0.7);
gr3.SetMarkerSize(0.9);
gr3.SetLineWidth(2);
gr3.Draw("C same");

gr4.SetLineColor(kGreen);
gr4.SetMarkerSize(0.9);
gr4.SetLineWidth(2);
gr4.Draw("C same");

gr5.SetLineStyle(kDashed);
gr5.SetLineColorAlpha(kBlue, 0.7);
gr5.SetMarkerSize(0.9);
gr5.SetLineWidth(2);
gr5.Draw("C same");

gr6.SetLineColor(kBlue);
gr6.SetMarkerSize(0.9);
gr6.SetLineWidth(2);
gr6.Draw("C same");

#gr4.SetLineColor(kViolet);
#gr4.SetMarkerSize(0.9);
#gr4.SetLineWidth(1);
#gr4.Draw("C same");

lg=TLegend(0.4, 0.62, 0.75, 0.92)
lg.AddEntry(gr1, "combined exp", "lp")
lg.AddEntry(gr2, "combined obs", "lp")
lg.AddEntry(gr3, "#gamma#gamma exp", "l");
lg.AddEntry(gr4, "#gamma#gamma obs", "l");
lg.AddEntry(gr5, "#tau#tau exp", "l");
lg.AddEntry(gr6, "#tau#tau obs", "l");

lg.SetFillStyle(0);
lg.SetBorderSize(0);
lg.Draw("same");

h68=TH1F("68CL", "", 1, limL, limR+0.1);
h68.SetBinContent(1, 0.5*2);
h68.SetLineStyle(kDashed);
h68.SetLineColor(kBlue-2);
h68.Draw("same hist");
h95=TH1F("95CL", "", 1, limL, limR+0.1);
h95.SetBinContent(1, 1.92*2);
h95.SetLineStyle(kDashed);
h95.SetLineColor(kBlue-2);
h95.Draw("same hist");

c.SaveAs(outdir+"/python_nll_"+outName+"_"+sysSet+".png")
c.SaveAs(outdir+"/python_nll_"+outName+"_"+sysSet+".pdf")
