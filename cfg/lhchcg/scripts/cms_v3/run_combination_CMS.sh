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

inputAsimov_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.prefit.asimov_decorated.root
inputToy_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.toy2_decorated.root
inputObs_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.obs_decorated_roodataset.root

inputAsimov_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.prefit.asimov_decorated.root
inputToy_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.toy_decorated.root
inputObs_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.obs_decorated.root


option=$1

if test $option -eq 0; then
# ======================== H->gamgam ============================
    folder="lhchcg"
    logname="CMS_hgg"
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

elif test $option -eq 1; then	# test CMS mass workspaces
# ======================== H->ZZ->4l ============================
    folder="lhchcg"
    logname="CMS_hzz"
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

elif test $option -eq 2; then	# test CMS mass workspaces
# ======================== Mass combination ============================
    folder="lhchcg"
    logname="CMS_combined"
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

# # Data
#     template=cfg/${folder}/${logname}_simple_template.xml
#     cat ${template} | sed "s|#INPUT1#|${inputObs_gg}|g" | sed "s|#INPUT2#|${inputObs_zz}|g" > cfg/${folder}/${logname}_simple_obs.xml
#     manager -w combine -x cfg/${folder}/${logname}_simple_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_simple_obs.root >& workspace/${folder}/myrep/${logname}_simple_obs.log &

# Data
    # inputObs_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.obs_decorated.root
    # cat ${template} | sed "s|#INPUT1#|${inputObs_gg}|g" | sed "s|#INPUT2#|${inputObs_zz}|g" > cfg/${folder}/${logname}_hist_obs.xml
    # manager -w combine -x cfg/${folder}/${logname}_hist_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_hist_obs.root >& workspace/${folder}/myrep/${logname}_hist_obs.log &
fi
