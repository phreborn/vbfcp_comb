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

inputAsimov_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_asimov_decorated.root
inputToy_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_toy_decorated.root
inputObs_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_obs_decorated.root

inputAsimov_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_asimov_decorated.root
inputToy_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_toy_decorated.root
inputObs_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_obs_decorated.root


option=$1

if test $option -eq 0; then
# ======================== H->gamgam ============================
    folder="lhchcg"
    logname="ATLAS_hgg"
    template=cfg/${folder}/${logname}_template.xml
    echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${logname}.setup

# Asimov
    cat ${template} | sed "s|#INPUT1#|${inputAsimov_gg}|g" | sed "s|#INPUT2#|${inputAsimov_zz}|g" > cfg/${folder}/${logname}_asimov.xml
    manager -w combine -x cfg/${folder}/${logname}_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_asimov.root >& workspace/${folder}/myrep/${logname}_asimov.log &

# Toy
    cat ${template} | sed "s|#INPUT1#|${inputToy_gg}|g" | sed "s|#INPUT2#|${inputToy_zz}|g" > cfg/${folder}/${logname}_toy.xml
    manager -w combine -x cfg/${folder}/${logname}_toy.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_toy.root >& workspace/${folder}/myrep/${logname}_toy.log &

# Data
    cat ${template} | sed "s|#INPUT1#|${inputObs_gg}|g" | sed "s|#INPUT2#|${inputObs_zz}|g" > cfg/${folder}/${logname}_obs.xml
    manager -w combine -x cfg/${folder}/${logname}_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_obs.root >& workspace/${folder}/myrep/${logname}_obs.log &

elif test $option -eq 1; then	# test ATLAS mass workspaces
# ======================== H->ZZ->4l ============================
    folder="lhchcg"
    logname="ATLAS_hzz"
    template=cfg/${folder}/${logname}_template.xml
    echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${logname}.setup

# Asimov
    cat ${template} | sed "s|#INPUT1#|${inputAsimov_gg}|g" | sed "s|#INPUT2#|${inputAsimov_zz}|g" > cfg/${folder}/${logname}_asimov.xml
    manager -w combine -x cfg/${folder}/${logname}_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_asimov.root >& workspace/${folder}/myrep/${logname}_asimov.log &

# # Toy
#     cat ${template} | sed "s|#INPUT1#|${inputToy_gg}|g" | sed "s|#INPUT2#|${inputToy_zz}|g" > cfg/${folder}/${logname}_toy.xml
#     manager -w combine -x cfg/${folder}/${logname}_toy.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_toy.root >& workspace/${folder}/myrep/${logname}_toy.log &

# # Data
#     cat ${template} | sed "s|#INPUT1#|${inputObs_gg}|g" | sed "s|#INPUT2#|${inputObs_zz}|g" > cfg/${folder}/${logname}_obs.xml
#     manager -w combine -x cfg/${folder}/${logname}_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_obs.root >& workspace/${folder}/myrep/${logname}_obs.log &

elif test $option -eq 2; then	# test ATLAS mass workspaces
# ======================== Mass combination ============================
    folder="lhchcg"
    logname="ATLAS_combined"
    template=cfg/${folder}/${logname}_template.xml
#     echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${logname}.setup

# # Asimov
#     cat ${template} | sed "s|#INPUT1#|${inputAsimov_gg}|g" | sed "s|#INPUT2#|${inputAsimov_zz}|g" > cfg/${folder}/${logname}_asimov.xml
#     manager -w combine -x cfg/${folder}/${logname}_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_asimov.root >& workspace/${folder}/myrep/${logname}_asimov.log &

# # Toy
#     cat ${template} | sed "s|#INPUT1#|${inputToy_gg}|g" | sed "s|#INPUT2#|${inputToy_zz}|g" > cfg/${folder}/${logname}_toy.xml
#     manager -w combine -x cfg/${folder}/${logname}_toy.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_toy.root >& workspace/${folder}/myrep/${logname}_toy.log &

# # Data
#     cat ${template} | sed "s|#INPUT1#|${inputObs_gg}|g" | sed "s|#INPUT2#|${inputObs_zz}|g" > cfg/${folder}/${logname}_obs.xml
#     manager -w combine -x cfg/${folder}/${logname}_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_obs.root >& workspace/${folder}/myrep/${logname}_obs.log &


    outputname=${logname}_parambr
    echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${outputname}.setup
    inputAsimov_gg=workspace/lhchcg/myrep/atlas/parambr/atlas_hgamgam_asimov_parambr.root
    inputToy_gg=workspace/lhchcg/myrep/atlas/parambr/atlas_hgamgam_toy_parambr.root
    inputObs_gg=workspace/lhchcg/myrep/atlas/parambr/atlas_hgamgam_obs_parambr.root

    inputAsimov_zz=workspace/lhchcg/myrep/atlas/parambr/atlas_h4l_asimov_parambr.root
    inputToy_zz=workspace/lhchcg/myrep/atlas/parambr/atlas_h4l_toy_parambr.root
    inputObs_zz=workspace/lhchcg/myrep/atlas/parambr/atlas_h4l_obs_parambr.root
# Asimov
    cat ${template} | sed "s|#INPUT1#|${inputAsimov_gg}|g" | sed "s|#INPUT2#|${inputAsimov_zz}|g" > cfg/${folder}/${outputname}_asimov.xml
    manager -w combine -x cfg/${folder}/${outputname}_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_asimov.root >& workspace/${folder}/myrep/${outputname}_asimov.log &

# Toy
    cat ${template} | sed "s|#INPUT1#|${inputToy_gg}|g" | sed "s|#INPUT2#|${inputToy_zz}|g" > cfg/${folder}/${outputname}_toy.xml
    manager -w combine -x cfg/${folder}/${outputname}_toy.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_toy.root >& workspace/${folder}/myrep/${outputname}_toy.log &

# Data
    cat ${template} | sed "s|#INPUT1#|${inputObs_gg}|g" | sed "s|#INPUT2#|${inputObs_zz}|g" > cfg/${folder}/${outputname}_obs.xml
    manager -w combine -x cfg/${folder}/${outputname}_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_obs.root >& workspace/${folder}/myrep/${outputname}_obs.log &

    outputname=${logname}_parambrlumi
    echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${outputname}.setup
 
    inputAsimov_gg=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_hgamgam_asimov_parambrlumi.root
    inputToy_gg=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_hgamgam_toy_parambrlumi.root
    inputObs_gg=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_hgamgam_obs_parambrlumi.root

    inputAsimov_zz=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_h4l_asimov_parambrlumi.root
    inputToy_zz=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_h4l_toy_parambrlumi.root
    inputObs_zz=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_h4l_obs_parambrlumi.root
# Asimov
    cat ${template} | sed "s|#INPUT1#|${inputAsimov_gg}|g" | sed "s|#INPUT2#|${inputAsimov_zz}|g" > cfg/${folder}/${outputname}_asimov.xml
    manager -w combine -x cfg/${folder}/${outputname}_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_asimov.root >& workspace/${folder}/myrep/${outputname}_asimov.log &

# Toy
    cat ${template} | sed "s|#INPUT1#|${inputToy_gg}|g" | sed "s|#INPUT2#|${inputToy_zz}|g" > cfg/${folder}/${outputname}_toy.xml
    manager -w combine -x cfg/${folder}/${outputname}_toy.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_toy.root >& workspace/${folder}/myrep/${outputname}_toy.log &

# Data
    cat ${template} | sed "s|#INPUT1#|${inputObs_gg}|g" | sed "s|#INPUT2#|${inputObs_zz}|g" > cfg/${folder}/${outputname}_obs.xml
    manager -w combine -x cfg/${folder}/${outputname}_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_obs.root >& workspace/${folder}/myrep/${outputname}_obs.log &

fi
