#!/usr/bin/python
# -*- coding: UTF-8 -*-

import sys

from ROOT import *
from math import sin
from array import array

def getNLLcurve(fnll):
  pnll = array('f') # precision lost in transformation to array
  vnll = array('f')
  with open(fnll, 'r') as f:
    for line in f.readlines():
      nll = line.split(' ')
      pnll.append(float(nll[0]))
      vnll.append(2*float(nll[1]))
  return vnll

outName = "comb"
sysSet='statOnly'
sysSet='allSys'

dirname = "autonll/"
dirname = "Expected/"

inNLL = dirname + "dNLL_Comb_Asi_"+sysSet+".log"
nll1 = getNLLcurve(inNLL)

inNLL = dirname + "tauMu1_matchglob/dNLL_Tau_"+sysSet+".log"
nll2 = getNLLcurve(inNLL)

inNLL = dirname + "gamMu1_matchglob/dNLL_Gam_"+sysSet+".log"
nll3 = getNLLcurve(inNLL)

#dirname = "Observed/"
#
#inNLL = dirname + "dNLL_Comb_"+sysSet+".log"
#nll1 = getNLLcurve(inNLL)
#
#inNLL = dirname + "dNLL_Tau_"+sysSet+".log"
#nll2 = getNLLcurve(inNLL)
#
#inNLL = dirname + "dNLL_Gam_"+sysSet+".log"
#nll3 = getNLLcurve(inNLL)

for i in range(len(nll1)):
  print i, nll1[i] - (nll2[i] + nll3[i])
