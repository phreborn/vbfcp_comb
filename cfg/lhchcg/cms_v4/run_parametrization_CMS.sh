#!/bin/bash

if test $# -lt 1; then
    echo "USAGE: [exe] <option (1-4)>" 
    exit
fi

if [ ! $_DIR ]; then
    source ./setup.sh
fi

which root
which gcc
make

inputAsimov=workspace/lhchcg/myrep/CMS_combined_asimov.root
inputToy=workspace/lhchcg/myrep/CMS_combined_toy.root
inputObs=workspace/lhchcg/myrep/CMS_combined_obs.root

outputAsimov=workspace/lhchcg/myrep/CMS_combined_asimov_deltaM.root
outputToy=workspace/lhchcg/myrep/CMS_combined_toy_deltaM.root
outputObs=workspace/lhchcg/myrep/CMS_combined_obs_deltaM.root

folder="lhchcg"
template=cfg/${folder}/tension_template.xml

option=$1

if test $option -eq 0; then
# ======================== Asimov ============================

cat ${template} | sed "s|#INPUT#|${inputAsimov}|g" | sed "s|#OUTPUT#|${outputAsimov}|g" > cfg/${folder}/CMS_tension_asimov.xml

manager -w orgnize -x cfg/${folder}/CMS_tension_asimov.xml

elif test $option -eq 1; then	# test CMS mass workspaces
# ======================== Toy ============================

cat ${template} | sed "s|#INPUT#|${inputToy}|g" | sed "s|#OUTPUT#|${outputToy}|g" > cfg/${folder}/CMS_tension_toy.xml

manager -w orgnize -x cfg/${folder}/CMS_tension_toy.xml

# ======================== Data ============================
elif test $option -eq 2; then	# test CMS mass workspaces

cat ${template} | sed "s|#INPUT#|${inputObs}|g" | sed "s|#OUTPUT#|${outputObs}|g" > cfg/${folder}/CMS_tension_obs.xml

manager -w orgnize -x cfg/${folder}/CMS_tension_obs.xml

fi
