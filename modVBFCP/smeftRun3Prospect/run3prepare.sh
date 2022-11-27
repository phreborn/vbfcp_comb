#!/bin/bash

source rename.sh

dtilde=$(cat cHW_fine)
for d in ${dtilde}
do
  #dgam=$(dTran ${d})
  #echo "${d} - ${dgam}"
  #taudir="inWS/tautau/${dgam}"
  #if [ ! -d ${taudir} ];then mkdir -p ${taudir};fi

  dgam=${d}

  # observed
  cp /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/xmlAnaWSBuilder/run/WSSMEFT_run3/vbf_cp_${dgam}/vbf_cp_${dgam}.root inWS/gamgam/Observed/vbf_cp_${dgam}.root
  ##cp splitLumiTau/lumi_split/${d}/125_lumi_split.root ${taudir}
  #cp splitLumiTau/lumi_split/${d}/VBFCPCOMBVBFMVA_FSPLIT_combined_VBFCPCOMBVBFMVA_FSPLIT_model_lumi_split.root ${taudir}/125_lumi_split.root

  # expected
done
