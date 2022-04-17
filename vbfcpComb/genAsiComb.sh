#!/bin/bash

rundir=$(pwd)

dtilde=SM
source rename.sh

xmldir=asimovCombXml

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/Asimov_${dgam}.xml
  cat ${xmldir}/example.xml > ${xmlfile}
  sed -i "s/m00/${dgam}/g" ${xmlfile}
done

tagCfg=Comb_Asi

fix_stat=ATLAS_*_gam,ATLAS_*_tau,gamma_stat_*,Lumi*_tau,Theo*tau,Z*_tau,*_fake_*,ATLAS_SM_*
set_poi="mu=1,mu_VBF_RW=1_0_5,mu_VBF_SM=0,mu_ggH=1,mu_ggH_SM=0,mu_spur=1,mu_spur_SM=0,ATLAS_epsilon_rejected=1_-5_5,ATLAS_epsilon=0"

ws=combWS
dataset=asimovData_SM_Mu1

if [ ! -d fitOutputs/${tagCfg}_statOnly ];then mkdir fitOutputs/${tagCfg}_statOnly;fi
if [ ! -d fitOutputs/${tagCfg}_allSys ];then mkdir fitOutputs/${tagCfg}_allSys;fi

dir=condor

allJobs=jobsSub.sh
> ${allJobs}

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  jobName=combAsi_${dgam}; echo ${jobName}
  if [ ! -d ${dir}/hepsub_${jobName} ]; then mkdir ${dir}/hepsub_${jobName}; fi
  executable=${dir}/exe_${jobName}.sh
  > ${executable}

  echo "#!/bin/bash" >> ${executable}
  echo "" >> ${executable}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/quickFit" >> ${executable}
  echo "source setup_lxplus.sh" >> ${executable}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/workspaceCombiner/vbfcpComb" >> ${executable}
  echo "" >> ${executable}

  dgam=$(dTran ${d})
  xmlfile=${xmldir}/Asimov_${dgam}.xml
  echo "../../quickFit/bin/quickAsimov -x ${xmlfile} -w combWS -m ModelConfig -d combData" >>  ${executable}

  echo "quickFit -f workspace/combined_${dgam}_Asi.root -w ${ws} -d ${dataset} -s Glob_SM_Mu1 -p ${set_poi} -n ${fix_stat} -o fitOutputs/${tagCfg}_statOnly/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}
  echo "quickFit -f workspace/combined_${dgam}_Asi.root -w ${ws} -d ${dataset} -s Glob_SM_Mu1 -p ${set_poi} -o fitOutputs/${tagCfg}_allSys/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}

  chmod +x ${executable}

  echo "hep_sub ${executable} -g atlas -os CentOS7 -wt mid -mem 2048 -o ${dir}/hepsub_${jobName}/log-0.out -e ${dir}/hepsub_${jobName}/log-0.err" >> ${allJobs}

  if [ "$(ls ${dir}/hepsub_${jobName}/)" != "" ];then rm ${dir}/hepsub_${jobName}/*;fi
done
