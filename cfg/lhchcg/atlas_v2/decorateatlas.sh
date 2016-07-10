make


# # ++++++++++++++++++++++++++++ ATLAS H->gamgam workspaces +++++++++++++++++++++++++++++++++++++
inputAsimov_gg=workspace/lhchcg/atlas/tests/atlas_hgamgam_RFRV.root
inputToy_gg=workspace/lhchcg/atlas/tests/atlas_hgamgam_RFRV.root
inputObs_gg=workspace/lhchcg/atlas/tests/atlas_hgamgam_RFRV.root

dnameAsimov="asimovData"
dnameToy="toyData"
dnameObs="combData"

mkdir -vp workspace/lhchcg/myrep/atlas/decorated/
decoratedAsimov_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_asimov_decorated.root
decoratedToy_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_toy_decorated.root
decoratedObs_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_obs_decorated.root

mkdir -vp workspace/lhchcg/myrep/atlas/handler/
handlerAsimov_gg=workspace/lhchcg/myrep/atlas/handler/atlas_hgamgam_asimov_handler.root
handlerToy_gg=workspace/lhchcg/myrep/atlas/handler/atlas_hgamgam_toy_handler.root
handlerObs_gg=workspace/lhchcg/myrep/atlas/handler/atlas_hgamgam_obs_handler.root

# ################################# Decorating workspace ################################
# manager -w split -i all -f $inputAsimov_gg -p $decoratedAsimov_gg --dataName $dnameAsimov --editRFV true
# manager -w split -i all -f $inputToy_gg -p $decoratedToy_gg --dataName $dnameToy --editRFV true
# manager -w split -i all -f $inputObs_gg -p $decoratedObs_gg --dataName $dnameObs --editRFV true

################################ Implementing handlers in the workspace #######################################
# cat cfg/lhchcg/ATLAS_measurement_parametrization_gamgam_template.xml | sed "s|#INPUT#|$decoratedAsimov_gg|g" | sed "s|#OUTPUT#|$handlerAsimov_gg|g" > cfg/lhchcg/ATLAS_measurement_parametrization_gamgam_asimov.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_measurement_parametrization_gamgam_asimov.xml

# cat cfg/lhchcg/ATLAS_measurement_parametrization_gamgam_template.xml | sed "s|#INPUT#|$decoratedToy_gg|g" | sed "s|#OUTPUT#|$handlerToy_gg|g" > cfg/lhchcg/ATLAS_measurement_parametrization_gamgam_toy.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_measurement_parametrization_gamgam_toy.xml

# cat cfg/lhchcg/ATLAS_measurement_parametrization_gamgam_template.xml | sed "s|#INPUT#|$decoratedObs_gg|g" | sed "s|#OUTPUT#|$handlerObs_gg|g" > cfg/lhchcg/ATLAS_measurement_parametrization_gamgam_obs.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_measurement_parametrization_gamgam_obs.xml

# # ++++++++++++++++++++++++++++ ATLAS H->ZZ workspaces +++++++++++++++++++++++++++++++++++++
# inputAsimov_zz=workspace/lhchcg/atlas/tests/atlas_h4l.root
# inputToy_zz=workspace/lhchcg/atlas/tests/atlas_h4l.root
# inputObs_zz=workspace/lhchcg/atlas/tests/atlas_h4l.root

# dnameAsimov="asimovData"
# dnameToy="toyData"
# dnameObs="obsData"

mkdir -vp workspace/lhchcg/myrep/atlas/decorated/
decoratedAsimov_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_asimov_decorated.root
decoratedToy_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_toy_decorated.root
decoratedObs_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_obs_decorated.root

mkdir -vp workspace/lhchcg/myrep/atlas/handler/
handlerAsimov_zz=workspace/lhchcg/myrep/atlas/handler/atlas_h4l_asimov_handler.root
handlerToy_zz=workspace/lhchcg/myrep/atlas/handler/atlas_h4l_toy_handler.root
handlerObs_zz=workspace/lhchcg/myrep/atlas/handler/atlas_h4l_obs_handler.root

# mkdir -vp workspace/lhchcg/myrep/atlas/parambr/
# parambrAsimov_zz=workspace/lhchcg/myrep/atlas/parambr/atlas_h4l_asimov_parambr.root
# parambrToy_zz=workspace/lhchcg/myrep/atlas/parambr/atlas_h4l_toy_parambr.root
# parambrObs_zz=workspace/lhchcg/myrep/atlas/parambr/atlas_h4l_obs_parambr.root

# mkdir -vp workspace/lhchcg/myrep/atlas/parambrlumi/
# parambrlumiAsimov_zz=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_h4l_asimov_parambrlumi.root
# parambrlumiToy_zz=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_h4l_toy_parambrlumi.root
# parambrlumiObs_zz=workspace/lhchcg/myrep/atlas/parambrlumi/atlas_h4l_obs_parambrlumi.root

# # # ################################# Decorating workspace ################################
# # manager -w split -i all -f $inputAsimov_zz -p $decoratedAsimov_zz --dataName $dnameAsimov --editRFV true
# # manager -w split -i all -f $inputToy_zz -p $decoratedToy_zz --dataName $dnameToy --editRFV true
# # manager -w split -i all -f $inputObs_zz -p $decoratedObs_zz --dataName $dnameObs --editRFV true

# # # ################################ Implementing handlers in the workspace #######################################
cat cfg/lhchcg/ATLAS_measurement_parametrization_zz_template.xml | sed "s|#INPUT#|$decoratedAsimov_zz|g" | sed "s|#OUTPUT#|$handlerAsimov_zz|g" > cfg/lhchcg/ATLAS_measurement_parametrization_zz_asimov.xml
manager -w orgnize -x cfg/lhchcg/ATLAS_measurement_parametrization_zz_asimov.xml

cat cfg/lhchcg/ATLAS_measurement_parametrization_zz_template.xml | sed "s|#INPUT#|$decoratedToy_zz|g" | sed "s|#OUTPUT#|$handlerToy_zz|g" > cfg/lhchcg/ATLAS_measurement_parametrization_zz_toy.xml
manager -w orgnize -x cfg/lhchcg/ATLAS_measurement_parametrization_zz_toy.xml

cat cfg/lhchcg/ATLAS_measurement_parametrization_zz_template.xml | sed "s|#INPUT#|$decoratedObs_zz|g" | sed "s|#OUTPUT#|$handlerObs_zz|g" > cfg/lhchcg/ATLAS_measurement_parametrization_zz_obs.xml
manager -w orgnize -x cfg/lhchcg/ATLAS_measurement_parametrization_zz_obs.xml

# # # ################################ Parametrize BR in the workspace #######################################
# cat cfg/lhchcg/ATLAS_param_BR_zz_template.xml | sed "s|#INPUT#|$handlerAsimov_zz|g" | sed "s|#OUTPUT#|$parambrAsimov_zz|g" > cfg/lhchcg/ATLAS_param_BR_zz_asimov.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_param_BR_zz_asimov.xml

# cat cfg/lhchcg/ATLAS_param_BR_zz_template.xml | sed "s|#INPUT#|$handlerToy_zz|g" | sed "s|#OUTPUT#|$parambrToy_zz|g" > cfg/lhchcg/ATLAS_param_BR_zz_toy.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_param_BR_zz_toy.xml

# cat cfg/lhchcg/ATLAS_param_BR_zz_template.xml | sed "s|#INPUT#|$handlerObs_zz|g" | sed "s|#OUTPUT#|$parambrObs_zz|g" > cfg/lhchcg/ATLAS_param_BR_zz_obs.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_param_BR_zz_obs.xml

# cat cfg/lhchcg/ATLAS_param_BR_lumi_zz_template.xml | sed "s|#INPUT#|$handlerAsimov_zz|g" | sed "s|#OUTPUT#|$parambrlumiAsimov_zz|g" > cfg/lhchcg/ATLAS_param_BR_lumi_zz_asimov.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_param_BR_lumi_zz_asimov.xml

# cat cfg/lhchcg/ATLAS_param_BR_lumi_zz_template.xml | sed "s|#INPUT#|$handlerToy_zz|g" | sed "s|#OUTPUT#|$parambrlumiToy_zz|g" > cfg/lhchcg/ATLAS_param_BR_lumi_zz_toy.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_param_BR_lumi_zz_toy.xml

# cat cfg/lhchcg/ATLAS_param_BR_lumi_zz_template.xml | sed "s|#INPUT#|$handlerObs_zz|g" | sed "s|#OUTPUT#|$parambrlumiObs_zz|g" > cfg/lhchcg/ATLAS_param_BR_lumi_zz_obs.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_param_BR_lumi_zz_obs.xml
