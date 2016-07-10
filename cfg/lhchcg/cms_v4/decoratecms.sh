make


# # ++++++++++++++++++++++++++++ CMS H->gamgam workspaces +++++++++++++++++++++++++++++++++++++
inputAsimov_gg=workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.prefit.asimov.root
inputToy_gg=workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.toy2.root
inputObs_gg=workspace/lhchcg/cms/tests/mass/hgg/hgg.mH.asimov.root

dnameAsimov="toy_asimov"
dnameToy="toy_1"
dnameObs="data_obs"

mkdir -vp workspace/lhchcg/myrep/cms/decorated/
decoratedAsimov_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.prefit.asimov_decorated.root
decoratedToy_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.toy_decorated.root
decoratedObs_gg=workspace/lhchcg/myrep/cms/decorated/hgg.mH.obs_decorated.root

mkdir -vp workspace/lhchcg/myrep/cms/handler/
handlerAsimov_gg=workspace/lhchcg/myrep/cms/handler/hgg.mH.prefit.asimov_handler.root
handlerToy_gg=workspace/lhchcg/myrep/cms/handler/hgg.mH.toy_handler.root
handlerObs_gg=workspace/lhchcg/myrep/cms/handler/hgg.mH.obs_handler.root

mkdir -vp workspace/lhchcg/myrep/cms/parambr/
parambrAsimov_gg=workspace/lhchcg/myrep/cms/parambr/hgg.mH.prefit.asimov_parambr.root
parambrToy_gg=workspace/lhchcg/myrep/cms/parambr/hgg.mH.toy_parambr.root
parambrObs_gg=workspace/lhchcg/myrep/cms/parambr/hgg.mH.obs_parambr.root

mkdir -vp workspace/lhchcg/myrep/cms/parambrlumi/
parambrlumiAsimov_gg=workspace/lhchcg/myrep/cms/parambrlumi/hgg.mH.prefit.asimov_parambrlumi.root
parambrlumiToy_gg=workspace/lhchcg/myrep/cms/parambrlumi/hgg.mH.toy_parambrlumi.root
parambrlumiObs_gg=workspace/lhchcg/myrep/cms/parambrlumi/hgg.mH.obs_parambrlumi.root

# ################################# Decorating workspace ################################
# manager -w decorate -f $inputAsimov_gg -p $decoratedAsimov_gg --dataName $dnameAsimov
# manager -w decorate -f $inputToy_gg -p $decoratedToy_gg --dataName $dnameToy
# manager -w decorate -f $inputObs_gg -p $decoratedObs_gg --dataName $dnameObs

# ################################ Implementing handlers in the workspace #######################################
# cat cfg/lhchcg/CMS_add_BR_handler_gamgam_template.xml | sed "s|#INPUT#|$decoratedAsimov_gg|g" | sed "s|#OUTPUT#|$handlerAsimov_gg|g" > cfg/lhchcg/CMS_add_BR_handler_gamgam_asimov.xml
# manager -w orgnize -x cfg/lhchcg/CMS_add_BR_handler_gamgam_asimov.xml

# cat cfg/lhchcg/CMS_add_BR_handler_gamgam_template.xml | sed "s|#INPUT#|$decoratedToy_gg|g" | sed "s|#OUTPUT#|$handlerToy_gg|g" > cfg/lhchcg/CMS_add_BR_handler_gamgam_toy.xml
# manager -w orgnize -x cfg/lhchcg/CMS_add_BR_handler_gamgam_toy.xml

# cat cfg/lhchcg/CMS_add_BR_handler_gamgam_template.xml | sed "s|#INPUT#|$decoratedObs_gg|g" | sed "s|#OUTPUT#|$handlerObs_gg|g" > cfg/lhchcg/CMS_add_BR_handler_gamgam_obs.xml
# manager -w orgnize -x cfg/lhchcg/CMS_add_BR_handler_gamgam_obs.xml

# ################################ Parametrize BR in the workspace #######################################
cat cfg/lhchcg/CMS_param_BR_gamgam_template.xml | sed "s|#INPUT#|$handlerAsimov_gg|g" | sed "s|#OUTPUT#|$parambrAsimov_gg|g" > cfg/lhchcg/CMS_param_BR_gamgam_asimov.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_gamgam_asimov.xml

cat cfg/lhchcg/CMS_param_BR_gamgam_template.xml | sed "s|#INPUT#|$handlerToy_gg|g" | sed "s|#OUTPUT#|$parambrToy_gg|g" > cfg/lhchcg/CMS_param_BR_gamgam_toy.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_gamgam_toy.xml

cat cfg/lhchcg/CMS_param_BR_gamgam_template.xml | sed "s|#INPUT#|$handlerObs_gg|g" | sed "s|#OUTPUT#|$parambrObs_gg|g" > cfg/lhchcg/CMS_param_BR_gamgam_obs.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_gamgam_obs.xml

cat cfg/lhchcg/CMS_param_BR_lumi_gamgam_template.xml | sed "s|#INPUT#|$handlerAsimov_gg|g" | sed "s|#OUTPUT#|$parambrlumiAsimov_gg|g" > cfg/lhchcg/CMS_param_BR_lumi_gamgam_asimov.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_lumi_gamgam_asimov.xml

cat cfg/lhchcg/CMS_param_BR_lumi_gamgam_template.xml | sed "s|#INPUT#|$handlerToy_gg|g" | sed "s|#OUTPUT#|$parambrlumiToy_gg|g" > cfg/lhchcg/CMS_param_BR_lumi_gamgam_toy.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_lumi_gamgam_toy.xml

cat cfg/lhchcg/CMS_param_BR_lumi_gamgam_template.xml | sed "s|#INPUT#|$handlerObs_gg|g" | sed "s|#OUTPUT#|$parambrlumiObs_gg|g" > cfg/lhchcg/CMS_param_BR_lumi_gamgam_obs.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_lumi_gamgam_obs.xml


# # ++++++++++++++++++++++++++++ CMS H->ZZ workspaces +++++++++++++++++++++++++++++++++++++
inputAsimov_zz=workspace/lhchcg/cms/tests/mass/hzz4l/lhchcg_cms_hzz_v0.root
inputToy_zz=workspace/lhchcg/cms/tests/mass/hzz4l/lhchcg_cms_hzz_v0.root
inputObs_zz=workspace/lhchcg/cms/tests/mass/hzz4l/lhchcg_cms_hzz_v0.root

dnameAsimov="asimovData_prefit"
dnameToy="toyData"
dnameObs="data_obs"

mkdir -vp workspace/lhchcg/myrep/cms/decorated/
decoratedAsimov_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.prefit.asimov_decorated.root
decoratedToy_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.toy_decorated.root
decoratedObs_zz=workspace/lhchcg/myrep/cms/decorated/hzz.mH.obs_decorated.root

mkdir -vp workspace/lhchcg/myrep/cms/handler/
handlerAsimov_zz=workspace/lhchcg/myrep/cms/handler/hzz.mH.prefit.asimov_handler.root
handlerToy_zz=workspace/lhchcg/myrep/cms/handler/hzz.mH.toy_handler.root
handlerObs_zz=workspace/lhchcg/myrep/cms/handler/hzz.mH.obs_handler.root

mkdir -vp workspace/lhchcg/myrep/cms/parambr/
parambrAsimov_zz=workspace/lhchcg/myrep/cms/parambr/hzz.mH.prefit.asimov_parambr.root
parambrToy_zz=workspace/lhchcg/myrep/cms/parambr/hzz.mH.toy_parambr.root
parambrObs_zz=workspace/lhchcg/myrep/cms/parambr/hzz.mH.obs_parambr.root

mkdir -vp workspace/lhchcg/myrep/cms/parambrlumi/
parambrlumiAsimov_zz=workspace/lhchcg/myrep/cms/parambrlumi/hzz.mH.prefit.asimov_parambrlumi.root
parambrlumiToy_zz=workspace/lhchcg/myrep/cms/parambrlumi/hzz.mH.toy_parambrlumi.root
parambrlumiObs_zz=workspace/lhchcg/myrep/cms/parambrlumi/hzz.mH.obs_parambrlumi.root

# ################################# Decorating workspace ################################
# manager -w decorate -f $inputAsimov_zz -p $decoratedAsimov_zz --dataName $dnameAsimov
# manager -w decorate -f $inputToy_zz -p $decoratedToy_zz --dataName $dnameToy
# manager -w decorate -f $inputObs_zz -p $decoratedObs_zz --dataName $dnameObs

# ################################ Implementing handlers in the workspace #######################################
# cat cfg/lhchcg/CMS_add_BR_handler_zz_template.xml | sed "s|#INPUT#|$decoratedAsimov_zz|g" | sed "s|#OUTPUT#|$handlerAsimov_zz|g" > cfg/lhchcg/CMS_add_BR_handler_zz_asimov.xml
# manager -w orgnize -x cfg/lhchcg/CMS_add_BR_handler_zz_asimov.xml

# cat cfg/lhchcg/CMS_add_BR_handler_zz_template.xml | sed "s|#INPUT#|$decoratedToy_zz|g" | sed "s|#OUTPUT#|$handlerToy_zz|g" > cfg/lhchcg/CMS_add_BR_handler_zz_toy.xml
# manager -w orgnize -x cfg/lhchcg/CMS_add_BR_handler_zz_toy.xml

# cat cfg/lhchcg/CMS_add_BR_handler_zz_template.xml | sed "s|#INPUT#|$decoratedObs_zz|g" | sed "s|#OUTPUT#|$handlerObs_zz|g" > cfg/lhchcg/CMS_add_BR_handler_zz_obs.xml
# manager -w orgnize -x cfg/lhchcg/CMS_add_BR_handler_zz_obs.xml

# ################################ Parametrize BR in the workspace #######################################
cat cfg/lhchcg/CMS_param_BR_zz_template.xml | sed "s|#INPUT#|$handlerAsimov_zz|g" | sed "s|#OUTPUT#|$parambrAsimov_zz|g" > cfg/lhchcg/CMS_param_BR_zz_asimov.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_zz_asimov.xml

cat cfg/lhchcg/CMS_param_BR_zz_template.xml | sed "s|#INPUT#|$handlerToy_zz|g" | sed "s|#OUTPUT#|$parambrToy_zz|g" > cfg/lhchcg/CMS_param_BR_zz_toy.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_zz_toy.xml

cat cfg/lhchcg/CMS_param_BR_zz_template.xml | sed "s|#INPUT#|$handlerObs_zz|g" | sed "s|#OUTPUT#|$parambrObs_zz|g" > cfg/lhchcg/CMS_param_BR_zz_obs.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_zz_obs.xml

cat cfg/lhchcg/CMS_param_BR_lumi_zz_template.xml | sed "s|#INPUT#|$handlerAsimov_zz|g" | sed "s|#OUTPUT#|$parambrlumiAsimov_zz|g" > cfg/lhchcg/CMS_param_BR_lumi_zz_asimov.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_lumi_zz_asimov.xml

cat cfg/lhchcg/CMS_param_BR_lumi_zz_template.xml | sed "s|#INPUT#|$handlerToy_zz|g" | sed "s|#OUTPUT#|$parambrlumiToy_zz|g" > cfg/lhchcg/CMS_param_BR_lumi_zz_toy.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_lumi_zz_toy.xml

cat cfg/lhchcg/CMS_param_BR_lumi_zz_template.xml | sed "s|#INPUT#|$handlerObs_zz|g" | sed "s|#OUTPUT#|$parambrlumiObs_zz|g" > cfg/lhchcg/CMS_param_BR_lumi_zz_obs.xml
manager -w orgnize -x cfg/lhchcg/CMS_param_BR_lumi_zz_obs.xml


