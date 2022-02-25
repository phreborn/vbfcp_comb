#!/bin/bash

tagCfg=Tau

fix_stat=ATLAS_*,gamma_stat_*,Lumi*,Theo*,Z*,*_fake_*
set_poi="ATLAS_epsilon_rejected=1_-5_5,ATLAS_epsilon=0,ATLAS_norm_HH_vbf_Fake=1_0_100,ATLAS_norm_LL_vbf_Top=1_0_100,ATLAS_norm_LL_vbf_Zll=1_0_100,ATLAS_norm_vbf_Ztt=1_0_100"

ws=combined

dataset=obsData
wsfname=125
dataset=asimovData
wsfname=125_plus190722
wsfname=125

if [ ! -d out${tagCfg}_statOnly ];then mkdir out${tagCfg}_statOnly;fi
if [ ! -d out${tagCfg}_allSys ];then mkdir out${tagCfg}_allSys;fi

source rename.sh

dir=condor

allJobs=jobsSub.sh
> ${allJobs}

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  jobName=taufit_${dgam}; echo ${jobName}
  if [ ! -d ${dir}/hepsub_${jobName} ]; then mkdir ${dir}/hepsub_${jobName}; fi
  executable=${dir}/exe_${jobName}.sh
  > ${executable}

  echo "#!/bin/bash" >> ${executable}
  echo "" >> ${executable}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/quickFit" >> ${executable}
  echo "source setup_lxplus.sh" >> ${executable}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/workspaceCombiner/vbfcpComb" >> ${executable}
  echo "" >> ${executable}
  echo "quickFit -f inWS/tautau/${dgam}/${wsfname}.root -w ${ws} -d ${dataset} -p ${set_poi} -n ${fix_stat} -o out${tagCfg}_statOnly/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}
  echo "quickFit -f inWS/tautau/${dgam}/${wsfname}.root -w ${ws} -d ${dataset} -p ${set_poi} -o out${tagCfg}_allSys/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}

  chmod +x ${executable}

  echo "hep_sub ${executable} -g atlas -os CentOS7 -wt mid -mem 2048 -o ${dir}/hepsub_${jobName}/log-0.out -e ${dir}/hepsub_${jobName}/log-0.err" >> ${allJobs}

  if [ "$(ls ${dir}/hepsub_${jobName}/)" != "" ];then rm ${dir}/hepsub_${jobName}/*;fi
done
