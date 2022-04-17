#!/bin/bash

source rename.sh

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  echo "${d} - ${dgam}"
  taudir="inWS/tautau/${dgam}"
  if [ ! -d ${taudir} ];then mkdir -p ${taudir};fi
#  cp ../../xmlAnaWSBuilder/run/WSStatOnly/vbf_cp_${dgam}/*root inWS/gamgam/
#  cp ../../histFactory/vbfcpTau/workspaces/VBFCP_comb_${d}/xml/VBFCPCOMBVBFMVA_FSPLIT_combined_VBFCPCOMBVBFMVA_FSPLIT_model.root ${taudir}

  # observed
  cp ../../xmlAnaWSBuilder/run/WSAllCats/vbf_cp_${dgam}/*root inWS/gamgam/Observed/
  #cp /scratchfs/atlas/huirun/atlaswork/VBF_CP/TauWS/130719_MVA_comb_data/${d}/combined/125.root ${taudir}
  cp /scratchfs/atlas/huirun/atlaswork/VBF_CP/TauWS/copy220330/130719_MVA_comb_data/${d}/combined/125_plus190722.root ${taudir}

  ## expected
  ##cp ../../xmlAnaWSBuilder/run/WSAllCats_unblindAsi/vbf_cp_${dgam}/*root inWS/gamgam/Expected/
  #cp ../../xmlAnaWSBuilder/run/WSAllCats/vbf_cp_${dgam}/*root inWS/gamgam/Expected/
  #cp /scratchfs/atlas/huirun/atlaswork/VBF_CP/TauWS/130719_MVA_comb_data/${d}/combined/125_plus190722.root ${taudir}
done
