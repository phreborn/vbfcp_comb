which root
which gcc
make

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

# inputAsimov_atlas_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_asimov_decorated.root
# inputToy_atlas_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_toy_decorated.root
# inputObs_atlas_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_obs_decorated.root

# inputAsimov_atlas_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_asimov_decorated.root
# inputToy_atlas_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_toy_decorated.root
# inputObs_atlas_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_obs_decorated.root

# inputAsimov_cms_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.prefit.asimov_decorated.root
# inputToy_cms_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.toy_decorated.root
# inputObs_cms_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.obs_decorated.root

# inputAsimov_cms_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.prefit.asimov_decorated.root
# inputToy_cms_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.toy_decorated.root
# inputObs_cms_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.obs_decorated.root

inputAsimov_atlas_gg=workspace/lhchcg/myrep/mH_theory/decorated/atlas_hgamgam_asimov_mH_theory_decorated.root
inputObs_atlas_gg=workspace/lhchcg/myrep/mH_theory/decorated/atlas_hgamgam_obs_mH_theory_decorated.root

inputAsimov_atlas_zz=workspace/lhchcg/myrep/mH_theory/decorated/atlas_h4l_asimov_mH_theory_decorated.root
inputObs_atlas_zz=workspace/lhchcg/myrep/mH_theory/decorated/atlas_h4l_obs_mH_theory_decorated.root

inputAsimov_cms_gg=workspace/lhchcg/myrep/mH_theory/decorated/cms_hgamgam_asimov_mH_theory_decorated.root
inputObs_cms_gg=workspace/lhchcg/myrep/mH_theory/decorated/cms_hgamgam_obs_mH_theory_decorated.root

inputAsimov_cms_zz=workspace/lhchcg/myrep/mH_theory/decorated/cms_h4l_asimov_mH_theory_decorated.root
inputObs_cms_zz=workspace/lhchcg/myrep/mH_theory/decorated/cms_h4l_obs_mH_theory_decorated.root


logname="LHC_combined_normmu"

if test $option -eq 0; then
    # Nominal LHC combination
    logname="LHC_combined_normmu"


elif test $option -eq 1; then
    # Just combine H->yy
    logname="LHC_hgg_normmu"

elif test $option -eq 2; then
    # Just combine H->ZZ
    logname="LHC_hzz_normmu"
fi


#manager -w combine -x cfg/${folder}/${logname}_obs_indepmass.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_obs_indepmass.root >& workspace/${folder}/myrep/${logname}_obs_indepmass.log &

#manager -w combine -x cfg/${folder}/${logname}_obs_normmu.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_obs_normmu.root >& workspace/${folder}/myrep/${logname}_obs_normmu.log &

template=cfg/${folder}/${logname}_template.xml

# # ======================== Mass combination ============================
echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${logname}.setup

# Asimov
cat ${template} | sed "s|#INPUT1_ATLAS#|${inputAsimov_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputAsimov_atlas_zz}|g" | sed "s|#INPUT1_CMS#|${inputAsimov_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputAsimov_cms_zz}|g" > cfg/${folder}/${logname}_asimov.xml
python python/createXML.py cfg/${folder}/${logname}_asimov.xml
manager -w combine -x cfg/${folder}/${logname}_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_asimov.root >& workspace/${folder}/myrep/${logname}_asimov.log &

# Data
cat ${template} | sed "s|#INPUT1_ATLAS#|${inputObs_atlas_gg}|g" | sed "s|#INPUT2_ATLAS#|${inputObs_atlas_zz}|g"  | sed "s|#INPUT1_CMS#|${inputObs_cms_gg}|g" | sed "s|#INPUT2_CMS#|${inputObs_cms_zz}|g" > cfg/${folder}/${logname}_obs.xml
python python/createXML.py cfg/${folder}/${logname}_obs.xml
manager -w combine -x cfg/${folder}/${logname}_obs.xml -s true -t 2 -f workspace/${folder}/myrep/${logname}_obs.root >& workspace/${folder}/myrep/${logname}_obs.log &
