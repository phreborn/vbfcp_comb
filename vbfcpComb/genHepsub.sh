#!/bin/bash

tagCfg=Comb

fix_stat=ATLAS_*_gam,ATLAS_*_tau,gamma_stat_*,Lumi*_tau,Theo*tau,Z*_tau,*_fake_*,ATLAS_SM_*
set_poi="mu=1,mu_VBF_RW=1_0_5,mu_VBF_SM=0,mu_ggH=1,mu_ggH_SM=0,mu_spur=1,mu_spur_SM=0,ATLAS_epsilon_rejected=1_-5_5,ATLAS_epsilon=0"

ws=combWS
dataset=combData

if [ ! -d fitOutputs/${tagCfg}_statOnly ];then mkdir fitOutputs/${tagCfg}_statOnly;fi
if [ ! -d fitOutputs/${tagCfg}_allSys ];then mkdir fitOutputs/${tagCfg}_allSys;fi

source rename.sh

#sequence=($(seq 1 1 ${#dList[@]}))
#intvl=0
#for init in ${sequence[@]};do
#  init=$((${init} - 1))
#  fin=$((${init} + ${intvl}))
#  for num in `seq ${init} 1 ${fin}`;do

dir=condor

allJobs=jobsSub.sh
> ${allJobs}

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  jobName=combfit_${dgam}; echo ${jobName}
  #if [ ! -d csv/${jobName} ];then mkdir -p csv/${jobName};fi
  if [ ! -d ${dir}/hepsub_${jobName} ]; then mkdir ${dir}/hepsub_${jobName}; fi
  executable=${dir}/exe_${jobName}.sh
  > ${executable}

  echo "#!/bin/bash" >> ${executable}
  echo "" >> ${executable}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/quickFit" >> ${executable}
  echo "source setup_lxplus.sh" >> ${executable}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/workspaceCombiner/vbfcpComb" >> ${executable}
  echo "" >> ${executable}
  echo "quickFit -f workspace/combined_${dgam}.root -w ${ws} -d ${dataset} -p ${set_poi} -n ${fix_stat} -o fitOutputs/${tagCfg}_statOnly/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}
  echo "quickFit -f workspace/combined_${dgam}.root -w ${ws} -d ${dataset} -p ${set_poi} -o fitOutputs/${tagCfg}_allSys/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}
#    if [ ${decompSys} -eq 1 ];then
#      echo "quickFit -f WS${preCfg}/vbf_cp_${dList[${num}]}/vbf_cp_${dList[${num}]}.root -w combWS -d ${dataset} -p mu=1,mu_VBF_SM=0,mu_ggH=1,mu_ggH_SM=0,mu_VBF_RW=1_0_5 -n ATLAS_PH*,ATLAS_EG*,*PRW*,*pdf*,*aS*,*qcd*,*shower*,*BIAS*,*lumi*,*HIGGS_MASS*,*rest_Higgs*,*mcstat* -o out${preCfg}_jetSys/out_${dList[${num}]}.root --savefitresult 1 --saveWS 1" >> ${executable}
#      echo "quickFit -f WS${preCfg}/vbf_cp_${dList[${num}]}/vbf_cp_${dList[${num}]}.root -w combWS -d ${dataset} -p mu=1,mu_VBF_SM=0,mu_ggH=1,mu_ggH_SM=0,mu_VBF_RW=1_0_5 -n ATLAS_JET*,*PRW*,*pdf*,*aS*,*qcd*,*shower*,*BIAS*,*lumi*,*HIGGS_MASS*,*rest_Higgs*,*mcstat* -o out${preCfg}_photonSys/out_${dList[${num}]}.root --savefitresult 1 --saveWS 1" >> ${executable}
#      echo "quickFit -f WS${preCfg}/vbf_cp_${dList[${num}]}/vbf_cp_${dList[${num}]}.root -w combWS -d ${dataset} -p mu=1,mu_VBF_SM=0,mu_ggH=1,mu_ggH_SM=0,mu_VBF_RW=1_0_5 -n ATLAS_JET*,ATLAS_PH*,ATLAS_EG*,*PRW*,*pdf*,*aS*,*qcd*,*shower*,*lumi*,*HIGGS_MASS*,*rest_Higgs*,*mcstat* -o out${preCfg}_ssSys/out_${dList[${num}]}.root --savefitresult 1 --saveWS 1" >> ${executable}
#      echo "quickFit -f WS${preCfg}/vbf_cp_${dList[${num}]}/vbf_cp_${dList[${num}]}.root -w combWS -d ${dataset} -p mu=1,mu_VBF_SM=0,mu_ggH=1,mu_ggH_SM=0,mu_VBF_RW=1_0_5 -n ATLAS_JET*,ATLAS_PH*,ATLAS_EG*,*PRW*,*BIAS*,*lumi*,*HIGGS_MASS*,*rest_Higgs*,*mcstat* -o out${preCfg}_theorySys/out_${dList[${num}]}.root --savefitresult 1 --saveWS 1" >> ${executable}
#    fi

  chmod +x ${executable}

  echo "hep_sub ${executable} -g atlas -os CentOS7 -wt mid -mem 2048 -o ${dir}/hepsub_${jobName}/log-0.out -e ${dir}/hepsub_${jobName}/log-0.err" >> ${allJobs}

  if [ "$(ls ${dir}/hepsub_${jobName}/)" != "" ];then rm ${dir}/hepsub_${jobName}/*;fi
done
