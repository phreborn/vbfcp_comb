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

option=$1

folder="lhchcg"

inputAsimov_atlas_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_asimov_decorated.root
inputToy_atlas_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_toy_decorated.root
inputObs_atlas_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_obs_decorated.root
inputPostfit_atlas_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_postfit_decorated.root

inputAsimov_atlas_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_asimov_decorated.root
inputToy_atlas_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_toy_decorated.root
inputObs_atlas_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_obs_decorated.root
inputPostfit_atlas_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_postfit_decorated.root

inputAsimov_cms_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.prefit.asimov_decorated.root
inputToy_cms_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.toy_decorated.root
inputObs_cms_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.obs_decorated.root
inputPostfit_cms_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.postfit.asimov_decorated.root

inputAsimov_cms_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.prefit.asimov_decorated.root
inputToy_cms_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.toy_decorated.root
inputObs_cms_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.obs_decorated.root
inputPostfit_cms_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.postfit.asimov_decorated.root

logname=""

if test $option -eq 0; then
    # Nominal LHC combination
    logname="LHC_combined"


elif test $option -eq 1; then
    # Just combine H->yy
    logname="LHC_hgg"

elif test $option -eq 2; then
    # Just combine H->ZZ
    logname="LHC_hzz"
fi

template=cfg/${folder}/${logname}_template.xml

# # ======================== Mass combination ============================
echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${logname}.setup

# # Asimov
# cat ${template} | sed "s|#INPUT1_ATLAS#|${inputAsimov_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputAsimov_atlas_zz}|g" | sed "s|#INPUT1_CMS#|${inputAsimov_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputAsimov_cms_zz}|g" > cfg/${folder}/${logname}_asimov.xml
# python python/createXML.py cfg/${folder}/${logname}_asimov.xml
# manager -w combine -x cfg/${folder}/${logname}_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_asimov.root >& workspace/${folder}/myrep/${logname}_asimov.log &

# # Toy
# cat ${template} | sed "s|#INPUT1_ATLAS#|${inputToy_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputToy_atlas_zz}|g" | sed "s|#INPUT1_CMS#|${inputToy_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputToy_cms_zz}|g" > cfg/${folder}/${logname}_toy.xml
# python python/createXML.py cfg/${folder}/${logname}_toy.xml
# manager -w combine -x cfg/${folder}/${logname}_toy.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_toy.root >& workspace/${folder}/myrep/${logname}_toy.log &

# # Data
# cat ${template} | sed "s|#INPUT1_ATLAS#|${inputObs_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputObs_atlas_zz}|g"  | sed "s|#INPUT1_CMS#|${inputObs_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputObs_cms_zz}|g" > cfg/${folder}/${logname}_obs.xml
# python python/createXML.py cfg/${folder}/${logname}_obs.xml
# manager -w combine -x cfg/${folder}/${logname}_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_obs.root >& workspace/${folder}/myrep/${logname}_obs.log &

# Postfit
cat ${template} | sed "s|#INPUT1_ATLAS#|${inputPostfit_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputPostfit_atlas_zz}|g" | sed "s|#INPUT1_CMS#|${inputPostfit_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputPostfit_cms_zz}|g" > cfg/${folder}/${logname}_postfit.xml
python python/createXML.py cfg/${folder}/${logname}_postfit.xml
manager -w combine -x cfg/${folder}/${logname}_postfit.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_postfit.root >& workspace/${folder}/myrep/${logname}_postfit.log &
