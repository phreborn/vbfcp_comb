#!/bin/usr/python
# -*- coding: UTF-8 -*-

from config import *

cats=[]
with open(path_cfgCats,'r') as f:
  for line in f.readlines():
    if '#' in line or ':' not in line: continue
    cat=line.split(':')[0]
#    print cat
    cats.append(cat)

sysList=[]
with open(path_sysList,'r') as f:
  for line in f.readlines():
    if 'Systematic' not in line: continue
    sysnametmp=line.split('\"')[1]
    if '_Hgg_BIAS_' in sysnametmp: continue
    sysname=sysnametmp.replace('ATLAS_','')
#    print sysname
    sysList.append(sysname)

sysList=list(set(sysList))
for cat in cats:
  sysList.append('Hgg_BIAS_OO_%s'%cat)

sysList.sort()

with open(path_yySysList,'w') as f:
  #f.writelines(sysList)
  for sys in sysList:
#    print sys
    if sys != sysList[0]: f.write("\n")
    f.write(sys)

print "\nsystematic list length:",len(sysList),"\n"
