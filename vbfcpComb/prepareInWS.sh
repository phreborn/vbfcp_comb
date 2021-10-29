#!/bin/bash

source rename.sh

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  echo "${d} - ${dgam}"
  taudir="inWS/tautau/${dgam}"
  if [ ! -d ${taudir} ];then mkdir -p ${taudir};fi
  cp ../../xmlAnaWSBuilder/run/WSStatOnly/vbf_cp_${dgam}/*root inWS/gamgam/
  cp ../../histFactory/vbfcpTau/workspaces/VBFCP_comb_${d}/xml/VBFCPCOMBVBFMVA_FSPLIT_combined_VBFCPCOMBVBFMVA_FSPLIT_model.root ${taudir}
done
