#!/bin/bash

#"    <NormFactor Name=\"mu_bkg[1]\"/>"

cfgdir=/scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/xmlAnaWSBuilder/run/configSMEFT_run3/

chws="m0d05"
cats="TL"
bins="b1"
chws=$(cat cHW_fine)
cats="TT TL LT"
bins="b1 b2 b3 b4 b5 b6"

for chw in ${chws}
do
for cat in ${cats}
do
for bin in ${bins}
do
  echo ${chw} ${cat} ${bin}
  catbin=${cat}_${bin}
  cfg=${cfgdir}/vbf_cp_${chw}/channel/category_OO_${catbin}.xml
  pos=$(sed -n '/nbkg_:category/=' ${cfg})
  Nexist=$(grep "mu_bkg" ${cfg} | wc -l)
  if [ ${Nexist} -eq 0 ];then
    sed -i "${pos}a\ \ \ \ <NormFactor Name=\"mu_bkg[1]\"\/>" ${cfg}
  fi
done
done
done
