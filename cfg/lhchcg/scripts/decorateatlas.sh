make


# # ++++++++++++++++++++++++++++ ATLAS H->gamgam workspaces +++++++++++++++++++++++++++++++++++++
inputAsimov_gg=workspace/lhchcg/atlas/mass/atlas_hgamgam.root
inputToy_gg=workspace/lhchcg/atlas/mass/atlas_hgamgam.root
inputObs_gg=workspace/lhchcg/atlas/mass/atlas_hgamgam.root
inputPostfit_gg=workspace/lhchcg/atlas/mass/atlas_hgamgam.root

dnameAsimov="asimovData"
dnameToy="toyData"
dnameObs="obsData"
dnamePostfit="asimovData_postfit"

mkdir -vp workspace/lhchcg/myrep/atlas/decorated/
decoratedAsimov_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_asimov_decorated.root
decoratedToy_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_toy_decorated.root
decoratedObs_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_obs_decorated.root
decoratedPostfit_gg=workspace/lhchcg/myrep/atlas/decorated/atlas_hgamgam_postfit_decorated.root

# ################################# Decorating workspace ################################
# manager -w decorate -f $inputAsimov_gg -p $decoratedAsimov_gg --dataName $dnameAsimov
# manager -w decorate -f $inputToy_gg    -p $decoratedToy_gg    --dataName $dnameToy
# manager -w decorate -f $inputObs_gg    -p $decoratedObs_gg    --dataName $dnameObs
manager -w decorate -f $inputPostfit_gg    -p $decoratedPostfit_gg    --dataName $dnamePostfit

# # ++++++++++++++++++++++++++++ ATLAS H->ZZ workspaces +++++++++++++++++++++++++++++++++++++
inputAsimov_zz=workspace/lhchcg/atlas/mass/atlas_h4l.root
inputToy_zz=workspace/lhchcg/atlas/mass/atlas_h4l.root
inputObs_zz=workspace/lhchcg/atlas/mass/atlas_h4l.root
inputPostfit_zz=workspace/lhchcg/atlas/mass/atlas_h4l.root

dnameAsimov="asimovData"
dnameToy="toyData"
dnameObs="obsData"
dnamePostfit="asimovData_postfit"

mkdir -vp workspace/lhchcg/myrep/atlas/decorated/
decoratedAsimov_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_asimov_decorated.root
decoratedToy_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_toy_decorated.root
decoratedObs_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_obs_decorated.root
decoratedPostfit_zz=workspace/lhchcg/myrep/atlas/decorated/atlas_h4l_postfit_decorated.root

# # # ################################# Decorating workspace ################################
# manager -w decorate -f $inputAsimov_zz -p $decoratedAsimov_zz --dataName $dnameAsimov
# manager -w decorate -f $inputToy_zz    -p $decoratedToy_zz    --dataName $dnameToy   
manager -w decorate -f $inputObs_zz    -p $decoratedObs_zz    --dataName $dnameObs   
manager -w decorate -f $inputPostfit_zz    -p $decoratedPostfit_zz    --dataName $dnamePostfit   

# # # ################################ Implementing handlers in the workspace #######################################
# cat cfg/lhchcg/ATLAS_measurement_parametrization_zz_template.xml | sed "s|#INPUT#|$decoratedAsimov_zz|g" | sed "s|#OUTPUT#|$handlerAsimov_zz|g" > cfg/lhchcg/ATLAS_measurement_parametrization_zz_asimov.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_measurement_parametrization_zz_asimov.xml

# cat cfg/lhchcg/ATLAS_measurement_parametrization_zz_template.xml | sed "s|#INPUT#|$decoratedToy_zz|g" | sed "s|#OUTPUT#|$handlerToy_zz|g" > cfg/lhchcg/ATLAS_measurement_parametrization_zz_toy.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_measurement_parametrization_zz_toy.xml

# cat cfg/lhchcg/ATLAS_measurement_parametrization_zz_template.xml | sed "s|#INPUT#|$decoratedObs_zz|g" | sed "s|#OUTPUT#|$handlerObs_zz|g" > cfg/lhchcg/ATLAS_measurement_parametrization_zz_obs.xml
# manager -w orgnize -x cfg/lhchcg/ATLAS_measurement_parametrization_zz_obs.xml

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

cp ${PWD}/scripts/decorateatlas.sh workspace/lhchcg/myrep/atlas/decorated/
