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

# inputAsimov=workspace/lhchcg/myrep/LHC_combined_parambr_asimov.root
# inputToy=workspace/lhchcg/myrep/LHC_combined_parambr_toy.root
# inputObs=workspace/lhchcg/myrep/LHC_combined_parambr_obs.root

# outputAsimov=workspace/lhchcg/myrep/LHC_combined_parambr_asimov_deltaM_zgam.root
# outputToy=workspace/lhchcg/myrep/LHC_combined_parambr_toy_deltaM_zgam.root
# outputObs=workspace/lhchcg/myrep/LHC_combined_parambr_obs_deltaM_zgam.root

# folder="lhchcg"
# template=cfg/${folder}/LHC_tension_zgam_template.xml

# option=$1

# if test $option -eq 0; then
# # ======================== Asimov ============================

# cat ${template} | sed "s|#INPUT#|${inputAsimov}|g" | sed "s|#OUTPUT#|${outputAsimov}|g" > cfg/${folder}/LHC_tension_asimov.xml

# manager -w orgnize -x cfg/${folder}/LHC_tension_zgam_asimov.xml

# elif test $option -eq 1; then	# test LHC mass workspaces
# # ======================== Toy ============================

# cat ${template} | sed "s|#INPUT#|${inputToy}|g" | sed "s|#OUTPUT#|${outputToy}|g" > cfg/${folder}/LHC_tension_toy.xml

# manager -w orgnize -x cfg/${folder}/LHC_tension_zgam_toy.xml

# # ======================== Data ============================
# elif test $option -eq 2; then	# test LHC mass workspaces

# cat ${template} | sed "s|#INPUT#|${inputObs}|g" | sed "s|#OUTPUT#|${outputObs}|g" > cfg/${folder}/LHC_tension_obs.xml

# manager -w orgnize -x cfg/${folder}/LHC_tension_zgam_obs.xml

# fi

inputAsimov=workspace/lhchcg/myrep/LHC_combined_parambr_asimov.root
inputToy=workspace/lhchcg/myrep/LHC_combined_parambr_toy.root
inputObs=workspace/lhchcg/myrep/LHC_combined_parambr_obs.root

outputAsimov=workspace/lhchcg/myrep/LHC_combined_parambr_asimov_deltaM_exp.root
outputToy=workspace/lhchcg/myrep/LHC_combined_parambr_toy_deltaM_exp.root
outputObs=workspace/lhchcg/myrep/LHC_combined_parambr_obs_deltaM_exp.root

folder="lhchcg"
template=cfg/${folder}/LHC_tension_exp_template.xml

option=$1

if test $option -eq 0; then
# ======================== Asimov ============================

cat ${template} | sed "s|#INPUT#|${inputAsimov}|g" | sed "s|#OUTPUT#|${outputAsimov}|g" > cfg/${folder}/LHC_tension_exp_asimov.xml

manager -w orgnize -x cfg/${folder}/LHC_tension_exp_asimov.xml

elif test $option -eq 1; then	# test LHC mass workspaces
# ======================== Toy ============================

cat ${template} | sed "s|#INPUT#|${inputToy}|g" | sed "s|#OUTPUT#|${outputToy}|g" > cfg/${folder}/LHC_tension_exp_toy.xml

manager -w orgnize -x cfg/${folder}/LHC_tension_exp_toy.xml

# ======================== Data ============================
elif test $option -eq 2; then	# test LHC mass workspaces

cat ${template} | sed "s|#INPUT#|${inputObs}|g" | sed "s|#OUTPUT#|${outputObs}|g" > cfg/${folder}/LHC_tension_exp_obs.xml

manager -w orgnize -x cfg/${folder}/LHC_tension_exp_obs.xml

fi
