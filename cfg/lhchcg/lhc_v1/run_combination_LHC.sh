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


# option=$1
folder="lhchcg"
logname="LHC_combined"
template=cfg/${folder}/${logname}_template.xml

# # ======================== Mass combination ============================

# inputAsimov_atlas_gg=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_hgamgam_asimov_parambrlumi.root
# inputToy_atlas_gg=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_hgamgam_toy_parambrlumi.root
# inputObs_atlas_gg=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_hgamgam_obs_parambrlumi.root

# inputAsimov_atlas_zz=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_h4l_asimov_parambrlumi.root
# inputToy_atlas_zz=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_h4l_toy_parambrlumi.root
# inputObs_atlas_zz=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_h4l_obs_parambrlumi.root

# inputAsimov_cms_gg=workspace/lhchcg/myrep/cms/parambrlumi/hgg.mH.prefit.asimov_parambrlumi.root
# inputToy_cms_gg=workspace/lhchcg/myrep/cms/parambrlumi/hgg.mH.toy_parambrlumi.root
# inputObs_cms_gg=workspace/lhchcg/myrep/cms/parambrlumi/hgg.mH.obs_parambrlumi.root

# inputAsimov_cms_zz=workspace/lhchcg/myrep/cms/parambrlumi/hzz.mH.prefit.asimov_parambrlumi.root
# inputToy_cms_zz=workspace/lhchcg/myrep/cms/parambrlumi/hzz.mH.toy_parambrlumi.root
# inputObs_cms_zz=workspace/lhchcg/myrep/cms/parambrlumi/hzz.mH.obs_parambrlumi.root

# outputname=${logname}
# echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${outputname}.setup

# # Asimov
# cat ${template} | sed "s|#INPUT1_ATLAS#|${inputAsimov_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputAsimov_atlas_zz}|g" | sed "s|#INPUT1_CMS#|${inputAsimov_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputAsimov_cms_zz}|g" > cfg/${folder}/${outputname}_asimov.xml
# manager -w combine -x cfg/${folder}/${outputname}_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_asimov.root >& workspace/${folder}/myrep/${outputname}_asimov.log &

# # Toy
# cat ${template} | sed "s|#INPUT1_ATLAS#|${inputToy_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputToy_atlas_zz}|g" | sed "s|#INPUT1_CMS#|${inputToy_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputToy_cms_zz}|g" > cfg/${folder}/${outputname}_toy.xml
# manager -w combine -x cfg/${folder}/${outputname}_toy.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_toy.root >& workspace/${folder}/myrep/${outputname}_toy.log &

# # Data
# cat ${template} | sed "s|#INPUT1_ATLAS#|${inputObs_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputObs_atlas_zz}|g"  | sed "s|#INPUT1_CMS#|${inputObs_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputObs_cms_zz}|g" > cfg/${folder}/${outputname}_obs.xml
# manager -w combine -x cfg/${folder}/${outputname}_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_obs.root >& workspace/${folder}/myrep/${outputname}_obs.log &

inputAsimov_atlas_gg=workspace/lhchcg/myrep/atlas/parambr/atlas_hgamgam_asimov_parambr.root
inputToy_atlas_gg=workspace/lhchcg/myrep/atlas/parambr/atlas_hgamgam_toy_parambr.root
inputObs_atlas_gg=workspace/lhchcg/myrep/atlas/parambr/atlas_hgamgam_obs_parambr.root

inputAsimov_atlas_zz=workspace/lhchcg/myrep/atlas/parambr/atlas_h4l_asimov_parambr.root
inputToy_atlas_zz=workspace/lhchcg/myrep/atlas/parambr/atlas_h4l_toy_parambr.root
inputObs_atlas_zz=workspace/lhchcg/myrep/atlas/parambr/atlas_h4l_obs_parambr.root

inputAsimov_cms_gg=workspace/lhchcg/myrep/cms/parambr/hgg.mH.prefit.asimov_parambr.root
inputToy_cms_gg=workspace/lhchcg/myrep/cms/parambr/hgg.mH.toy_parambr.root
inputObs_cms_gg=workspace/lhchcg/myrep/cms/parambr/hgg.mH.obs_parambr.root

inputAsimov_cms_zz=workspace/lhchcg/myrep/cms/parambr/hzz.mH.prefit.asimov_parambr.root
inputToy_cms_zz=workspace/lhchcg/myrep/cms/parambr/hzz.mH.toy_parambr.root
inputObs_cms_zz=workspace/lhchcg/myrep/cms/parambr/hzz.mH.obs_parambr.root

outputname=${logname}_parambr

echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${outputname}.setup

# Asimov
cat ${template} | sed "s|#INPUT1_ATLAS#|${inputAsimov_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputAsimov_atlas_zz}|g" | sed "s|#INPUT1_CMS#|${inputAsimov_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputAsimov_cms_zz}|g" > cfg/${folder}/${outputname}_asimov.xml
manager -w combine -x cfg/${folder}/${outputname}_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_asimov.root >& workspace/${folder}/myrep/${outputname}_asimov.log &

# Toy
cat ${template} | sed "s|#INPUT1_ATLAS#|${inputToy_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputToy_atlas_zz}|g" | sed "s|#INPUT1_CMS#|${inputToy_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputToy_cms_zz}|g" > cfg/${folder}/${outputname}_toy.xml
manager -w combine -x cfg/${folder}/${outputname}_toy.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_toy.root >& workspace/${folder}/myrep/${outputname}_toy.log &

# Data
cat ${template} | sed "s|#INPUT1_ATLAS#|${inputObs_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputObs_atlas_zz}|g"  | sed "s|#INPUT1_CMS#|${inputObs_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputObs_cms_zz}|g" > cfg/${folder}/${outputname}_obs.xml
manager -w combine -x cfg/${folder}/${outputname}_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${outputname}_obs.root >& workspace/${folder}/myrep/${outputname}_obs.log &
