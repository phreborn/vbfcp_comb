#!/usr/bin/python
# -*- coding: UTF-8 -*-

import sys

from ROOT import *
from math import sin
from array import array

#gEnv.Print()

gROOT.SetBatch()

#gROOT.LoadMacro("/afs/cern.ch/user/f/faguo/RootUtils/AtlasStyle.C")
#gROOT.LoadMacro("/afs/cern.ch/user/f/faguo/RootUtils/AtlasUtils.C")
#gROOT.LoadMacro("/afs/cern.ch/user/f/faguo/RootUtils/AtlasLabels.C")
gROOT.LoadMacro("AtlasStyle.C")
gROOT.LoadMacro("AtlasUtils.C")
gROOT.LoadMacro("AtlasLabels.C")

SetAtlasStyle()

gStyle.SetErrorX(0.5); # !!! must after SetAtlasStyle()
gStyle.SetOptStat(0)
gStyle.SetLegendFont(62)

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

outName = "syst"
dirname = "nllcurveinputs/"
dirname_exp = "nllcurveinputs/Expected/"
dirname_obs = "nllcurveinputs/Observed/"

inNLL = dirname_exp + "dNLL_Comb_allSys.log"
gr1 = getNLLcurve(inNLL)

inNLL = dirname_exp + "dNLL_Gam_allSys.log"
gr2 = getNLLcurve(inNLL)

inNLL = dirname_exp + "dNLL_Tau_allSys.log"
gr3 = getNLLcurve(inNLL)

inNLL = dirname_obs + "dNLL_Comb_allSys.log"
gr4 = getNLLcurve(inNLL)

inNLL = dirname_obs + "dNLL_Gam_allSys.log"
gr5 = getNLLcurve(inNLL)

inNLL = dirname_obs + "dNLL_Tau_allSys.log"
gr6 = getNLLcurve(inNLL)


c = TCanvas('c', '', 200, 10, 700, 560)

gr1.GetXaxis().SetLimits(limL, limR)
gr1.GetYaxis().SetRangeUser(-0.01, 30)
gr1.GetYaxis().SetTitle("2#times#DeltaNLL")
gr1.GetXaxis().SetTitle("#tilde{d}")
gr1.SetTitle("Negative Log Likelihood")
gr1.SetMaximum(35)

gr1.SetLineStyle(4)
#gr1.SetMarkerSize(0.9)
gr1.SetMarkerSize(0)
gr1.SetLineWidth(3)
gr1.Draw("AC")

gr2.SetLineColor(kBlue);
gr2.SetLineStyle(4)
gr2.SetMarkerSize(0.);
gr2.SetLineWidth(3);
gr2.Draw("C same");

gr3.SetLineColor(kRed);
gr3.SetLineStyle(4)
gr3.SetMarkerSize(0.);
gr3.SetLineWidth(3);
gr3.Draw("C same");

#gr4.SetMarkerSize(0.9)
gr4.SetMarkerSize(0)
gr4.SetLineWidth(3)
gr4.Draw("C same")

gr5.SetLineColor(kBlue);
gr5.SetMarkerSize(0.);
gr5.SetLineWidth(3);
gr5.Draw("C same");

gr6.SetLineColor(kRed);
gr6.SetMarkerSize(0.);
gr6.SetLineWidth(3);
gr6.Draw("C same");


lg=TLegend(0.6, 0.63, 0.85, 0.91)
lg.AddEntry(gr1, "Exp. Comb", "lp")
lg.AddEntry(gr2, "Exp. H#rightarrow#gamma#gamma", "l")
lg.AddEntry(gr3, "Exp. H#rightarrow#tau#tau", "l")
lg.AddEntry(gr4, "Obs. Comb", "lp")
lg.AddEntry(gr5, "Obs. H#rightarrow#gamma#gamma", "l")
lg.AddEntry(gr6, "Obs. H#rightarrow#tau#tau", "l")
#lg.SetFillStyle(0);
lg.SetBorderSize(0);
lg.SetTextFont(42);
lg.Draw("same");

h68=TH1F("68CL", "", 1, limL, limR+0.1);
h68.SetBinContent(1, 0.5*2);
h68.SetLineStyle(kDashed);
h68.SetLineColor(9);
h68.Draw("same hist");
h95=TH1F("95CL", "", 1, limL, limR+0.1);
h95.SetBinContent(1, 1.92*2);
h95.SetLineStyle(kDashed);
h95.SetLineColor(9);
h95.Draw("same hist");

lt = TLatex()
lt.SetTextSize(0.024)
lt.SetTextAlign(10)
lt.SetTextFont(42);
lt.DrawLatex(-0.245, 0.5*2, '#color[9]{68% CL}')
lt.DrawLatex(-0.245, 1.92*2, '#color[9]{95% CL}')

yoffset = 0.02
xoffset = 0.
ATLASLabel(0.2+xoffset,0.85+yoffset, "Internal");
myText(0.2+xoffset, 0.8+yoffset, 1, "#sqrt{s}= 13 TeV, 139 fb^{-1}");
myText(0.2+xoffset, 0.75+yoffset, 1, "VBF H #rightarrow #gamma#gamma");

c.Draw();
c.SaveAs(dirname+"/python_nll_"+outName+".png")
c.SaveAs(dirname+"/python_nll_"+outName+".pdf")
