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
gStyle.SetLegendFont(42)

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

################### for Dtilde ###################

limL = -0.169
limR = 0.169

dirname = "./"
dirname_exp = "nllcurveinputs/Expected/"
dirname_obs = "nllcurveinputs/Observed/"

inNLL = dirname_exp + "dNLL_Comb_allSys.log"
inNLL = "Expected/run2_dNLL_Gam_allSys.log"
gr1 = getNLLcurve(inNLL)

inNLL = dirname_exp + "dNLL_Gam_allSys.log"
inNLL = "Expected/run3_dNLL_Gam_allSys.log"
gr2 = getNLLcurve(inNLL)

#inNLL = dirname_exp + "dNLL_Tau_allSys.log"
#gr3 = getNLLcurve(inNLL)
#
#inNLL = dirname_obs + "dNLL_Comb_allSys.log"
#gr4 = getNLLcurve(inNLL)
#
#inNLL = dirname_obs + "dNLL_Gam_allSys.log"
#gr5 = getNLLcurve(inNLL)
#
#inNLL = dirname_obs + "dNLL_Tau_allSys.log"
#gr6 = getNLLcurve(inNLL)


c1 = TCanvas("c1", "canvas", 1000, 800)
c2 = TCanvas("c2", "canvas", 1000, 800)

c1.cd()
gr1.GetXaxis().SetLimits(limL, limR)
gr1.GetYaxis().SetRangeUser(-0.01, 27.5)
gr1.GetYaxis().SetTitle("2 #times #DeltaNLL")
gr1.GetXaxis().SetTitle("#it{#tilde{d}}")
gr1.SetTitle("Negative Log Likelihood")
gr1.SetMaximum(27.5)

#gr1.SetLineStyle(4)
#gr1.SetMarkerSize(0.9)
gr1.SetMarkerSize(0)
gr1.SetLineWidth(3)
gr1.Draw("AC")

gr2.SetLineColor(kBlue);
#gr2.SetLineStyle(4)
gr2.SetMarkerSize(0.);
gr2.SetLineWidth(3);
gr2.Draw("C same");

#gr3.SetLineColor(kRed);
#gr3.SetLineStyle(4)
#gr3.SetMarkerSize(0.);
#gr3.SetLineWidth(3);
#gr3.Draw("C same");
#
##gr4.SetMarkerSize(0.9)
#gr4.SetMarkerSize(0)
#gr4.SetLineWidth(3)
#gr4.Draw("C same")
#
#gr5.SetLineColor(kBlue);
#gr5.SetMarkerSize(0.);
#gr5.SetLineWidth(3);
#gr5.Draw("C same");
#
#gr6.SetLineColor(kRed);
#gr6.SetMarkerSize(0.);
#gr6.SetLineWidth(3);
#gr6.Draw("C same");


lg1=TLegend(0.55, 0.68, 0.93, 0.93)
#lg1.SetNColumns(2)
lg1.AddEntry(gr1, "Exp. Run2", "lp")
lg1.AddEntry(gr2, "Exp. 300/fb Prospect", "l")
#lg1.AddEntry(gr4, "Obs. Comb", "lp")
#lg1.AddEntry(gr5, "Obs. H #rightarrow #gamma#gamma", "l")
#lg1.AddEntry(gr3, "Exp. H #rightarrow #tau#tau", "l")
#lg1.AddEntry(gr6, "Obs. H #rightarrow #tau#tau", "l")
lg1.SetFillStyle(0);
lg1.SetBorderSize(0);
lg1.SetTextFont(42);
lg1.Draw("same");

h68_1=TH1F("68CL", "", 1, limL, limR+0.1);
h68_1.SetBinContent(1, 0.5*2);
h68_1.SetLineStyle(kDashed);
h68_1.SetLineColor(9);
h68_1.Draw("same hist");
h95_1=TH1F("95CL", "", 1, limL, limR+0.1);
h95_1.SetBinContent(1, 1.92*2);
h95_1.SetLineStyle(kDashed);
h95_1.SetLineColor(9);
h95_1.Draw("same hist");

lt1 = TLatex()
lt1.SetTextSize(0.035)
lt1.SetTextAlign(10)
lt1.SetTextFont(42);
lt1.DrawLatex(-0.22, 0.5*2, '#color[9]{68% CL}')
lt1.DrawLatex(-0.22, 1.92*2, '#color[9]{95% CL}')


yoffset = 0.02
xoffset = 0.0
textsize = 0.042
ltATLAS = TLatex();
ltATLAS.SetNDC();
ltATLAS.SetTextSize(textsize);
ltATLAS.SetTextFont(72);
ltATLAS.DrawLatex(0.20+xoffset, 0.85+yoffset, "ATLAS");
ltATLAS.SetTextFont(42);
ltATLAS.DrawLatex(0.20+0.13+xoffset, 0.85+yoffset, "Internal");
#ATLASLabel(0.2+xoffset,0.85+yoffset, "Internal");

lt = TLatex();
lt.SetNDC();
lt.SetTextFont(42);
lt.SetTextSize(textsize);
lt.DrawLatex(0.2+xoffset, 0.78+yoffset, "#sqrt{s} = 13 TeV, 36 - 139 fb^{-1}");
#myText(0.2+xoffset, 0.78+yoffset, 1, "#sqrt{s} = 13 TeV, 36 - 139 fb^{-1}");
#myText(0.2+xoffset, 0.71+yoffset, 1, "VBF H #rightarrow #gamma#gamma, #tau#tau");


c1.Draw();
c1.SaveAs("Expected/nllDtilde_scaleToRun3.png")
c1.SaveAs("Expected/nllDtilde_scaleToRun3.pdf")

#################### for cHWt ###################
#
#c2.cd()
#
#limL = -1.29
#limR = 1.29
#
#dirname = "./"
#dirname_exp = "nllcurveinputs/cHWt/Expected/"
#dirname_obs = "nllcurveinputs/cHWt/Observed/"
#
#inNLL = dirname_exp + "dNLL_AllCats_SMEFT_allSys.log"
#gr7 = getNLLcurve(inNLL)
#
#inNLL = dirname_obs + "dNLL_AllCats_SMEFT_allSys.log"
#gr8 = getNLLcurve(inNLL)
#
#gr7.GetXaxis().SetLimits(limL, limR)
#gr7.GetYaxis().SetRangeUser(-0.01, 9.5)
#gr7.GetYaxis().SetTitle("2 #times #DeltaNLL")
#gr7.GetXaxis().SetTitle("#it{c_{H#tilde{W}}}")
#gr7.SetTitle("Negative Log Likelihood")
#
#gr7.SetLineStyle(4)
##gr7.SetMarkerSize(0.9)
#gr7.SetMarkerSize(0)
#gr7.SetLineWidth(3)
#gr7.Draw("AC")
#
##gr8.SetMarkerSize(0.9)
#gr8.SetMarkerSize(0)
#gr8.SetLineWidth(3)
#gr8.Draw("C same")
#
#lg2=TLegend(0.40, 0.55, 0.85, 0.67)
#lg2.AddEntry(gr7, "Exp. stat. + syst.", "lp")
#lg2.AddEntry(gr8, "Obs. stat. + syst.", "lp")
#lg2.SetBorderSize(0);
#lg2.SetTextFont(42);
#lg2.Draw("same");
#
#h68_2=TH1F("68CL", "", 1, limL, limR+0.1);
#h68_2.SetBinContent(1, 0.5*2);
#h68_2.SetLineStyle(kDashed);
#h68_2.SetLineColor(9);
#h68_2.Draw("same hist");
#h95_2=TH1F("95CL", "", 1, limL, limR+0.1);
#h95_2.SetBinContent(1, 1.92*2);
#h95_2.SetLineStyle(kDashed);
#h95_2.SetLineColor(9);
#h95_2.Draw("same hist");
#
#lt2 = TLatex()
#lt2.SetTextSize(0.035)
#lt2.SetTextAlign(10)
#lt2.SetTextFont(42);
#lt2.DrawLatex(-1.7, 0.5*2, '#color[9]{68% CL}')
#lt2.DrawLatex(-1.7, 1.92*2, '#color[9]{95% CL}')
#
#xoffset = 0.20
#ltATLAS = TLatex();
#ltATLAS.SetNDC();
#ltATLAS.SetTextFont(72);
#ltATLAS.DrawLatex(0.20+xoffset, 0.85+yoffset, "ATLAS");
#ltATLAS.SetTextFont(42);
#ltATLAS.DrawLatex(0.20+0.15+xoffset, 0.85+yoffset, "Internal");
##ATLASLabel(0.2+xoffset,0.85+yoffset, "Internal");
#myText(0.2+xoffset, 0.78+yoffset, 1, "#sqrt{s} = 13 TeV, 139 fb^{-1}");
#myText(0.2+xoffset, 0.71+yoffset, 1, "VBF H #rightarrow #gamma#gamma");
#
#c2.Draw();
#c2.SaveAs(dirname+"/python_nllcHWt_V3.png")
#c2.SaveAs(dirname+"/python_nllcHWt_V3.pdf")
#
#
