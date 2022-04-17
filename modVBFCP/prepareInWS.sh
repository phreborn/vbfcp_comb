#!/bin/bash

source rename.sh

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  echo "${d} - ${dgam}"
  taudir="inWS/tautau/${dgam}"
  if [ ! -d ${taudir} ];then mkdir -p ${taudir};fi

  # observed
  cp /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/xmlAnaWSBuilder/run/WSAllCats/vbf_cp_${dgam}/vbf_cp_${dgam}.root inWS/gamgam/Observed/
  cp splitLumiTau/lumi_split/${d}/125_lumi_split.root ${taudir}

  # expected
done
