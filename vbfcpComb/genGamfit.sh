#!/bin/bash

tagCfg=Gam

fix_stat=ATLAS_*
set_poi="mu=1,mu_ggH_SM=0,mu_VBF_SM=0,mu_ggH=1,mu_VBF_RW=1_0_5"

expobs=Expected

ws=combWS

dataset=asimovData_SM
wsfname=_onfly

outfrlt=fitOutputs

if [ ! -d ${outfrlt}/${tagCfg}_statOnly ];then mkdir ${outfrlt}/${tagCfg}_statOnly;fi
if [ ! -d ${outfrlt}/${tagCfg}_allSys ];then mkdir ${outfrlt}/${tagCfg}_allSys;fi

source rename.sh

dir=condor

allJobs=jobsSub.sh
> ${allJobs}

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  jobName=gamfit_${dgam}; echo ${jobName}
  if [ ! -d ${dir}/hepsub_${jobName} ]; then mkdir ${dir}/hepsub_${jobName}; fi
  executable=${dir}/exe_${jobName}.sh
  > ${executable}

  echo "#!/bin/bash" >> ${executable}
  echo "" >> ${executable}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/quickFit" >> ${executable}
  echo "source setup_lxplus.sh" >> ${executable}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/workspaceCombiner/vbfcpComb" >> ${executable}
  echo "" >> ${executable}
  echo "quickFit -f inWS/gamgam/${expobs}/vbf_cp_${dgam}${wsfname}.root -w ${ws} -d ${dataset} -p ${set_poi} -n ${fix_stat} -o ${outfrlt}/${tagCfg}_statOnly/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}
  echo "quickFit -f inWS/gamgam/${expobs}/vbf_cp_${dgam}${wsfname}.root -w ${ws} -d ${dataset} -p ${set_poi} -o ${outfrlt}/${tagCfg}_allSys/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}

  chmod +x ${executable}

  echo "hep_sub ${executable} -g atlas -os CentOS7 -wt mid -mem 2048 -o ${dir}/hepsub_${jobName}/log-0.out -e ${dir}/hepsub_${jobName}/log-0.err" >> ${allJobs}

  if [ "$(ls ${dir}/hepsub_${jobName}/)" != "" ];then rm ${dir}/hepsub_${jobName}/*;fi
done
