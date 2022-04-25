#!/bin/bash

rundir=$(pwd)

dtilde=SM
cd ../
source rename.sh
cd -

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  echo workspaces/VBFCP_comb_data_${d}
  if [ ! -d workspaces/VBFCP_comb_data_${d} ];then
    mkdir workspaces/VBFCP_comb_data_${d}
  fi
  cp -r 130719_MVA_comb_data/${d}/xml workspaces/VBFCP_comb_data_${d}/

#  mkdir -p ~/public/forKunlin/VBFCP_Htautau/${d}/combined/
#  cp -r workspaces/VBFCP_comb_data_${d}/xml/VBFCPCOMBVBFMVA_FSPLIT_combined_VBFCPCOMBVBFMVA_FSPLIT_model.root ~/public/forKunlin/VBFCP_Htautau/${d}/combined/
done
