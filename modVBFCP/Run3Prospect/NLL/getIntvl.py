#!/usr/bin/python
# -*- coding: UTF-8 -*-

import sys

from ROOT import *
from math import sin
from array import array

gROOT.SetBatch()

dataType = "Observed"
bdtcat = "AllCats"

#dataType = sys.argv[1]
#bdtcat = sys.argv[2]

#dirname = "/scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/xmlAnaWSBuilder/run/autonll/"
dirname = dataType + "/updatedBDT/"
inNLL = dirname + "dNLL_" + bdtcat + "_statOnly.log"
dirname = dataType + "/updatedBDT_SMEFT/"
inNLL = dirname + "dNLL_" + bdtcat + "_SMEFT_statOnly.log"
#inNLL = dirname + "dNLL_SMEFT_quad_statOnly.log"
inNLL = "Expected/run3_dNLL_Gam_allSys.log"

### reading NLL points and ordering ###
nlls = {}
with open(inNLL, 'r') as f:
  for line in f.readlines():
    nll = line.split(' ')
    nlls[float(nll[0])] = float(nll[1])

if len(nlls) < 2:
  print 'NLL points less than 2, exiting.'
  sys.exit(1)

ordnll = sorted(nlls.keys())

#pnll = array('f')
#vnll = array('f')
pnll = []
vnll = []
for nll in ordnll:
    print nll, nlls[nll]
    pnll.append(nll)
    vnll.append(nlls[nll])

##precision lost in transformation to array
#for i in range(len(pnll)): print pnll[i], vnll[i]

### truncating NLL curve ###
truncs = {}
truncount = 0
tmplist = []
slope = 0
for i in range(len(nlls)):
  phere = vnll[i]
  pnext = 0
  plast = 0
  if i == 0: plast = phere-(vnll[i+1]-phere)
  else: plast = vnll[i-1]
  if i == len(nlls)-1: pnext = phere
  else: pnext = vnll[i+1]

  diff = phere - plast
  if diff != 0: slope = 1 if diff > 0 else -1

  tmplist.append(pnll[i])
  if ((plast-phere)*(pnext-phere) >= 0 and (pnext-phere) != 0) or i == len(nlls)-1:
    truncs[truncount] = [slope, tmplist]
    truncount += 1
    tmplist = []
    slope = 0

### get best fit ###
ary_pnll = array('d')
ary_vnll = array('d')
inll0 = 0
for i in range(len(nlls)):
  if vnll[i] == 0:
    inll0 = i
count = 0
for i in range(len(nlls)):
  if abs(i - inll0) > 3: continue
  ary_pnll.append(pnll[i])
  ary_vnll.append(vnll[i])
  count+=1
gr_bf = TGraph(count, ary_pnll, ary_vnll)
fpol2 = TF1('poly2', 'pol2', -0.03, 0.03)
gr_bf.Fit(fpol2, 'W')
p1 = fpol2.GetParameter(1)
p2 = fpol2.GetParameter(2)
#print 'best fit(not precise, too many CPV points):',-p1/(2*p2)
#print 'best fit:%0.3f'%(-p1/(2*p2))
print 'best fit:', -p1/(2*p2)
gr_bf.Draw()

### evaluating intervals ###
l1sigma=0
r1sigma=0
l2sigma=0
r2sigma=0
for Ord in truncs.keys():
  nllp, nllv = array('d'), array('d')
  for dval in truncs[Ord][1]:
    nllp.append(dval)
    nllv.append(nlls[dval])

  gr = TGraph(len(nllv), nllv, nllp)
  if truncs[Ord][0] == -1:
    #print 'best fit:', abs(gr.Eval(0))
    l1sigma = gr.Eval(0.5)
    l2sigma = gr.Eval(1.92)
    print '68%CL interval left:', gr.Eval(0.5)
    print '95%CL interval left:', gr.Eval(1.92)
  if truncs[Ord][0] == 1:
    r1sigma = gr.Eval(0.5)
    r2sigma = gr.Eval(1.92)
    print '68%CL interval right:', gr.Eval(0.5)
    print '95%CL interval right:', gr.Eval(1.92)

#print '68%%CL interval: [%0.3f, %0.3f]'%(l1sigma, r1sigma)
#print '95%%CL interval: [%0.3f, %0.3f]'%(l2sigma, r2sigma)
