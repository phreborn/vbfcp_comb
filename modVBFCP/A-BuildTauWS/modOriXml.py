#!/usr/bin/python
# -*- coding: UTF-8 -*-

# use copyWS.sh to refresh oriXmls first

import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-d', '--dtilde', type=str, default='SM')

result = parser.parse_args()

tmp='.tmp'
tmp=''

dvals=[]
dfile="../dtilde.list"
with open(dfile, 'r') as f:
  for line in f.readlines():
    dvals.append(line.strip())

#print dvals

dvals=[result.dtilde]

higgs=['VBFH125',
'VBFH125dtildeX',
'VBFH125WW',
'VBFH125WWdtildeX',
'otherH', # other Higgs include decaying to WW or tautau-only? should be tautau-only (ATLAS_BR_tautau)
]

bkgs=['Fake',
'Othersrew_hh',
'Othersrew_lh',
'Othersrew_ll',
'Top',
'VV',
'Zllrew',
'Zttrew',
]

for dval in dvals:
  oriXml="workspaces/VBFCP_comb_data_%s/xml/VBFCPCOMBVBFMVA_FSPLIT.xml"%(dval)
  print oriXml
  subXmls=[]
  with open(oriXml, 'r') as f:
    for line in f.readlines():
      if 'Input' not in line: continue
      spath=line.replace('<Input>', '').replace('</Input>', '').replace(' ', '').replace('\n', '')
      subXmls.append(spath)
  
  with open(oriXml, 'r') as f:
    xml=f.read()
    xml=xml.replace('<POI>ATLAS_epsilon ATLAS_epsilon_rejected</POI>', '<POI>ATLAS_epsilon ATLAS_epsilon_rejected mu mu_VBF mu_otherH mu_BR_tautau mu_BR_WW</POI>')
    #print xml
  
  with open(oriXml+tmp, 'w') as f:
    f.write(xml)
  
  pattern = re.compile(r'Name=".*?"')
  for xml in subXmls:
    new_lines=[]
    with open(xml, 'r') as f:
      old_lines=f.readlines()
      for line in old_lines:
        new_lines.append(line)
        if '<Sample' not in line: continue
        #print line
        #result=pattern.match(line) # must match from the beginning of the string
        result=pattern.findall(line)
        proc=(re.split(r'"', result[0]))[1]
        #print proc
        if proc in higgs:
          new_lines.append('      <NormFactor Name="mu"  Val="1"  High="5"  Low="-5"  Const="True"   />\n')
          if 'VBF' in proc:
            new_lines.append('      <NormFactor Name="mu_VBF"  Val="1"  High="5"  Low="-5"  Const="True"   />\n')
          if 'otherH' in proc:
            new_lines.append('      <NormFactor Name="mu_otherH"  Val="1"  High="5"  Low="-5"  Const="True"   />\n')
          if 'VBFH125WW' not in proc:
            new_lines.append('      <NormFactor Name="mu_BR_tautau"  Val="1"  High="5"  Low="-5"  Const="True"   />\n')
          if 'VBFH125WW' in proc:
            new_lines.append('      <NormFactor Name="mu_BR_WW"  Val="1"  High="5"  Low="-5"  Const="True"   />\n')
  
    with open(xml+tmp, 'w') as f:
      for line in new_lines:
        f.write(line)
