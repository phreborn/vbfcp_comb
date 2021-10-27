#!/usr/bin/python
# -*- coding: UTF-8 -*-

import os, sys, commands
import argparse
from config import *

parser = argparse.ArgumentParser()
parser.add_argument('-d', '--dtilde', type=str, default='SM')

result = parser.parse_args()

dirpath=sys.path[0]+'/'
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

class Channel:
  'individual channel class to be combined'
  channelCount = 0

  def __init__(self, name, pois, inFile, chlist='channelList', ws='combWS', mc='ModelConfig', data='combData', iscomb=False):
    self.name = name
    self.pois = pois
    self.inFile = inFile
    self.chlist = chlist
    self.ws = ws
    self.mc = mc
    self.data = data
    self.xmlLines = []
    self.iscomb = iscomb
    Channel.channelCount += 1

  def getXmlLines(self, *renameNPs):
    tmpXmlLines = []
    if self.iscomb: tmpXmlLines.append('  <Channel Name="%s" IsCombined="true" Mass="125.09">'%self.name)
    else: tmpXmlLines.append('  <Channel Name="%s">'%self.name)
    tmpXmlLines.append('    <File Name="%s"/>'%self.inFile)
    tmpXmlLines.append('    <Workspace Name="%s"/>'%self.ws)
    tmpXmlLines.append('    <ModelConfig Name="%s"/>'%self.mc)
    tmpXmlLines.append('    <ModelData Name="%s"/>'%self.data)
    tmpXmlLines.append('    <ModelPOI Name="%s"/>'%(', '.join(self.pois)))
    if not self.iscomb:
      tmpXmlLines.append('    <RenameMap>')
      for np in renameNPs: tmpXmlLines.append('      '+np)
      tmpXmlLines.append('      <Syst OldName= "%s" NewName= "Cat_%s"/>'%(self.chlist, chlistName[self.name]))
      tmpXmlLines.append('    </RenameMap>')
    tmpXmlLines.append('  </Channel>')

    self.xmlLines = tmpXmlLines
    return self.xmlLines

CONSTRPREFIX="constr__"
GLOBPREFIX="RNDM__"
def strReplaceNPName_yy(nui):
  return '<Syst OldName = "%s%s(%s, %s%s)" NewName = "%s" />'%(CONSTRPREFIX,nui,nui,GLOBPREFIX,nui,nui)


##### reading systematic list #####

sysList=[]
with open(path_yySysList,'r') as f:
  for line in f.readlines():
    sysList.append(line.replace('\n',''))
sysList=list(set(sysList))
sysList.sort()
print "systematic list length:",len(sysList),"\n"

##### creat xml lines #####
combChannel = Channel('combined', combPois, combOutRootFile, iscomb=True)

yyRenameList=[]
atlasPrefix="ATLAS_"
#for sys in sysList:
#  yyRenameList.append(strReplaceNPName_yy(atlasPrefix+sys))

gamChannel=Channel('diphoton', yyPois, yyInRootFile, data='asimovData_SB_SM')

tauChannel = Channel('ditau', ttPois, ttInRootFile, 'channelCat', ws='combined', data='obsData')

with open(outXml, 'w') as f:
  f.write('<!DOCTYPE Combination  SYSTEM \'Combination.dtd\'>\n<Combination>')
  for line in combChannel.getXmlLines():
    f.write('\n'+line)
    print line
  f.write('\n')
  for line in gamChannel.getXmlLines(*yyRenameList):
    f.write('\n'+line)
    print line
  f.write('\n')
  for line in tauChannel.getXmlLines():
    print line
    f.write('\n'+line)
  f.write('\n</Combination>')
