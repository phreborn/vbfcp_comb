make


# # ++++++++++++++++++++++++++++ CMS H->gamgam workspaces +++++++++++++++++++++++++++++++++++++
inputAsimov_gg=workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.prefit.asimov.root_unconst.root
inputToy_gg=workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.toy2.root_unconst.root
inputObs_gg=workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.asimov.root_unconst.root
inputPostfit_gg=workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.asimov.root_unconst.root

dnameAsimov="toy_asimov"
dnameToy="toy_1"
dnameObs="data_obs"
dnamePostfit="toy_asimov"

mkdir -vp workspace/lhchcg/myrep/cms/decorated/
decoratedAsimov_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.prefit.asimov_decorated.root
decoratedToy_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.toy_decorated.root
decoratedObs_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.obs_decorated.root
decoratedPostfit_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.postfit.asimov_decorated.root
# ################################# Decorating workspace ################################
# manager -w decorate -f $inputAsimov_gg -p $decoratedAsimov_gg --dataName $dnameAsimov
# manager -w decorate -f $inputToy_gg    -p $decoratedToy_gg    --dataName $dnameToy
# manager -w decorate -f $inputObs_gg    -p $decoratedObs_gg    --dataName $dnameObs
# manager -w decorate -f $inputPostfit_gg    -p $decoratedPostfit_gg    --dataName $dnamePostfit

# ++++++++++++++++++++++++++++ CMS H->ZZ workspaces +++++++++++++++++++++++++++++++++++++
inputAsimov_zz=workspace/lhchcg/cms/tests/mass/hzz4l/lhchcg_cms_hzz_v1.root
inputToy_zz=workspace/lhchcg/cms/tests/mass/hzz4l/lhchcg_cms_hzz_v1.root
inputObs_zz=workspace/lhchcg/cms/tests/mass/hzz4l/lhchcg_cms_hzz_v1.root
inputPostfit_zz=workspace/lhchcg/cms/tests/mass/hzz4l/lhchcg_cms_hzz_v1.root

dnameAsimov="asimovData_prefit"
dnameToy="toyData"
dnameObs="data_obs"
dnamePostfit="asimovData_postfit"

mkdir -vp workspace/lhchcg/myrep/cms/decorated/
decoratedAsimov_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.prefit.asimov_decorated.root
decoratedToy_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.toy_decorated.root
decoratedObs_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.obs_decorated.root
decoratedPostfit_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.postfit.asimov_decorated.root

# ################################# Decorating workspace ################################
# manager -w decorate -f $inputAsimov_zz -p $decoratedAsimov_zz --dataName $dnameAsimov
# sleep 10
# manager -w decorate -f $inputToy_zz    -p $decoratedToy_zz    --dataName $dnameToy
# sleep 10
# manager -w decorate -f $inputObs_zz    -p $decoratedObs_zz    --dataName $dnameObs
manager -w decorate -f $inputPostfit_zz    -p $decoratedPostfit_zz    --dataName $dnamePostfit

