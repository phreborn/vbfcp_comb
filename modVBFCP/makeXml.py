#!/usr/bin/python
# -*- coding: UTF-8 -*-

import os, sys, commands
import argparse
from config import *

parser = argparse.ArgumentParser()
parser.add_argument('-d', '--dtilde', type=str, default='SM')

result = parser.parse_args()

dirpath=sys.path[0]+'/'
combOutRootFile=dirpath+combOutRootFile.replace('{dtilde}', result.dtilde)
yyInRootFile=dirpath+yyInRootFile.replace('{dtilde}', result.dtilde)
ttInRootFile=dirpath+ttInRootFile.replace('{dtilde}', result.dtilde)
outXml=dirpath+outXml.replace('{dtilde}', result.dtilde)

class Syst:
  'a systematic uncertainty'
  sysCount=0

  def __init__(self, name):
    self.name = name
    self.up = 0
    self.dn = 0
    Syst.sysCount += 1

  def setUpDn(self,up,dn):
    self.up = up
    self.dn = dn
    self.updn_is_set=True

  def displaySyst(self):
    if self.updn_is_set: print '%s:%d,%d'%(self.name,self.up,self.dn)
    else : print '%s: unset'%self.name

#sys = Syst('jer', 0.1, -0.2)
#sys.displaySyst()

class Combination:
  'combination head'
  combCount = 0

  def __init__(self, pois, ws='combWS', mc='ModelConfig', data='combData', outfile='./workspace/combined.root'):
    self.ws = ws
    self.mc = mc
    self.data = data
    self.outfile = outfile
    self.pois = pois
    Combination.combCount += 1

  def getHeadXmlLines(self):
    tmpXmlLines = []
    tmpXmlLines.append('<Combination WorkspaceName = "%s" ModelConfigName = "%s" DataName = "%s" OutputFile="%s">'%(self.ws, self.mc, self.data, self.outfile))
    tmpXmlLines.append('  <POIList Combined = "%s[1~1]"/>'%('[1~1],'.join(self.pois)))

    self.xmlLines = tmpXmlLines
    return self.xmlLines

  def getEndXmlLine(self):
    endline = '</Combination>'
    return endline

class Asimov:
  'configure actions to generate Asimov data'
  asimovCount = 0

  def __init__(self, name, setup, act, snapshot):
    self.name = name
    self.setup = setup
    self.act = act
    self.snapshot = snapshot
    Asimov.asimovCount += 1

  def getXmlLines(self):
    tmpXmlLines = []
    tmpXmlLines.append('  <Asimov Name="%s" Setup="%s" Action="%s" SnapshotAll="%s"/>'%(self.name, self.setup, self.act, self.snapshot))

    self.xmlLines = tmpXmlLines
    return self.xmlLines

class Channel:
  'individual channel class to be combined'
  channelCount = 0

  def __init__(self, name, pois, inFile, chlist='channelList', ws='combWS', mc='ModelConfig', data='combData'):
    self.name = name
    self.pois = pois
    self.inFile = inFile
    self.chlist = chlist
    self.ws = ws
    self.mc = mc
    self.data = data
    self.xmlLines = []
    Channel.channelCount += 1

  def getXmlLines(self, *renameNPs):
    tmpXmlLines = []
    tmpXmlLines.append('  <Channel Name="%s" InputFile = "%s" WorkspaceName = "%s" ModelConfigName = "%s" DataName = "%s">'%(self.name, self.inFile, self.ws, self.mc, self.data))
    tmpXmlLines.append('    <POIList Input="%s"/>'%(', '.join(self.pois)))
    tmpXmlLines.append('    <RenameMap>')
    for np in renameNPs: tmpXmlLines.append('      '+np)
    #tmpXmlLines.append('')
    #tmpXmlLines.append('      <Syst OldName= "%s" NewName= "Cat_%s"/>'%(self.chlist, chlistName[self.name]))
    tmpXmlLines.append('    </RenameMap>')
    tmpXmlLines.append('  </Channel>')

    self.xmlLines = tmpXmlLines
    return self.xmlLines

CONSTRPREFIX="constr__"
GLOBPREFIX="RNDM__"
def strReplaceNPName_yy(nui):
  for prefix in elseConstr:
    if prefix in nui:
      nuinew = yyMaps[nui]
      return '<Syst OldName = "%s(%s, %s)" NewName = "%s" />'%(nui+'_ConstraintPdf',nui,nui+'_In',nuinew)
      #return '<Syst OldName = "%s(%s, %s)" NewName = "%s" />'%(nui+'_ConstraintPdf',nui,nui+'_In',nui+'_HGam')
  if nui in yyMaps.keys():
    nuinew = yyMaps[nui]
    return '<Syst OldName = "%s%s(%s, %s%s)" NewName = "%s" />'%(CONSTRPREFIX,nui,nui,GLOBPREFIX,nui,nuinew)
    #return '<Syst OldName = "%s%s(%s, %s%s)" NewName = "%s" />'%(CONSTRPREFIX,nui,nui,GLOBPREFIX,nui,nui+'_HGam')
  else:
    return '<Syst OldName = "%s%s(%s, %s%s)" NewName = "%s" />'%(CONSTRPREFIX,nui,nui,GLOBPREFIX,nui,nui+'_HGam')

def strReplaceNPName_tt(nui):
  for prefix in elseConstr:
    if prefix in nui:
      nuinew = ttMaps[nui]
      return '<Syst OldName = "%s(%s, %s)" NewName = "%s" />'%(nui+'_Pdf',nui,nui+'_In',nuinew)
      #return '<Syst OldName = "%s(%s, %s)" NewName = "%s" />'%(nui+'_Pdf',nui,nui+'_In',nui+'_HTau')
  if nui in ttMaps.keys():
    nuinew = ttMaps[nui]
    return '<Syst OldName = "%s(%s, %s)" NewName = "%s" />'%(''+nui+'Constraint',nui,'nom_'+nui,nuinew)
    #return '<Syst OldName = "%s(%s, %s)" NewName = "%s" />'%('alpha_'+nui+'Constraint',nui,'nom_alpha_'+nui,nui+'_HTau')
  else:
    if 'gamma_stat_' in nui: return '<Syst OldName = "%s" NewName = "%s" />'%(nui,nui+'_HTau')
    return '<Syst OldName = "%s(%s, %s)" NewName = "%s" />'%(''+nui+'Constraint',nui,'nom_'+nui,nui+'_HTau')


##### reading systematic list #####

sysList=[]
with open(path_yySysList,'r') as f:
  for line in f.readlines():
    sysList.append(line.replace('\n',''))
sysList=list(set(sysList))
sysList.sort()
print "systematic list length:",len(sysList),"\n"

ttsysList=[]
with open(path_ttSysList,'r') as f:
  for line in f.readlines():
    if 'norm_' in line: continue
    if 'gamma_' in line: continue
    ttsysList.append(line.replace('\n',''))
ttsysList=list(set(ttsysList))
ttsysList.sort()
print "systematic list length:",len(ttsysList),"\n"

##### creat xml lines #####
combination = Combination(combPois, outfile = combOutRootFile)

yyRenameList=[]
atlasPrefix="ATLAS_"
for sys in sysList:
  yyRenameList.append(strReplaceNPName_yy(sys))

ttRenameList=[]
for sys in ttsysList:
  ttRenameList.append(strReplaceNPName_tt(sys))

gamChannel = Channel('HGam', yyPois, yyInRootFile, data=yydata)

tauChannel = Channel('HTau', ttPois, ttInRootFile, 'channelCat', ws='combined', data=ttdata)

with open(outXml, 'w') as f:
  f.write('<!DOCTYPE Combination  SYSTEM \'Combination.dtd\'>')
  for line in combination.getHeadXmlLines():
    f.write('\n'+line)
    print line
  f.write('\n')
#  for line in gamChannel.getXmlLines():
  for line in gamChannel.getXmlLines(*yyRenameList):
    f.write('\n'+line)
    print line
  f.write('\n')
#  for line in tauChannel.getXmlLines():
  for line in tauChannel.getXmlLines(*ttRenameList):
    print line
    f.write('\n'+line)
  f.write('\n'+combination.getEndXmlLine())
