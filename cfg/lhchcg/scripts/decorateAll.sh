make
# ++++++++++++++++++++++++++++ ATLAS workspaces +++++++++++++++++++++++++++++++++++
# manager -w split -i all -f workspace/lhchcg/atlas/tests/atlas_hgamgam_RFRV.root -p workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_toy_decorated.root --dataName toyData --editRFV true --ReBin -1
# manager -w split -i all -f workspace/lhchcg/atlas/tests/atlas_hgamgam_RFRV.root -p workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_asimov_decorated.root --dataName asimovData --editRFV true
manager -w split -i all -f workspace/lhchcg/atlas/tests/atlas_hgamgam_RFRV.root -p workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_obs_decorated.root --dataName combData --editRFV true --ReBin -1

# manager -w decorate -f workspace/lhchcg/atlas/tests/atlas_h4l.root -p workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_toy_decorated.root --dataName toyData --ReBin -1
# manager -w decorate -f workspace/lhchcg/atlas/tests/atlas_h4l.root -p workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_asimov_decorated.root --dataName asimovData --ReBin -1
# manager -w decorate -f workspace/lhchcg/atlas/tests/atlas_h4l.root -p workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_obs_decorated.root --dataName obsData --ReBin -1

# ++++++++++++++++++++++++++++ CMS workspaces +++++++++++++++++++++++++++++++++++++
# manager -w decorate -f workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.prefit.asimov.root -p workspace/lhchcg/myrep/cms/decorated/hgg.mH.prefit.asimov_decorated.root --dataName toy_asimov --setVar MH=125_120_130,RF=1_0_10,RV=1_-10_10
# manager -w decorate -f workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.toy2.root -p workspace/lhchcg/myrep/cms/decorated/hgg.mH.toy2_decorated.root --dataName toy_1 --setVar MH=125_120_130,RF=1_0_10,RV=1_-10_10
# manager -w decorate -f workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.toy.root -p workspace/lhchcg/myrep/cms/decorated/hgg.mH.obs_decorated_roodataset.root --dataName data_obs --setVar MH=125_120_130,RF=1_0_10,RV=1_-10_10
#manager -w decorate -f workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.toy.root -p workspace/lhchcg/myrep/cms/decorated/hgg.mH.obs_decorated.root --dataName data_obs --histToData false
# manager -w decorate -f workspace/lhchcg/cms/tests/mass/hzz4l/lhchcg_cms_hzz_v0.root -p workspace/lhchcg/myrep/cms/decorated/hzz.mH.prefit.asimov_decorated.root --dataName asimovData_prefit --setVar MH=125_120_130,r=1_0_10
# manager -w decorate -f workspace/lhchcg/cms/tests/mass/hzz4l/lhchcg_cms_hzz_v0.root -p workspace/lhchcg/myrep/cms/decorated/hzz.mH.toy_decorated.root --dataName toyData --setVar MH=125_120_130,r=1_0_10
# manager -w decorate -f workspace/lhchcg/cms/tests/mass/hzz4l/lhchcg_cms_hzz_v0.root -p workspace/lhchcg/myrep/cms/decorated/hzz.mH.obs_decorated.root --dataName data_obs --setVar MH=125_120_130,r=1_0_10

