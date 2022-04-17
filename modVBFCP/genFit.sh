#!/bin/bash

tagCfg=Comb

pois="mu=1,mu_BR_yy=1,mu_spur_SM=0,mu_ggH_SM=0,mu_VBF_SM=0,mu_spur=1,mu_ggH=1,mu_VBF_RW=1_0_5,ATLAS_epsilon=0,ATLAS_epsilon_rejected=1_-5_5"
fix_stat=ATLAS_B*,ATLAS_E*,ATLAS_F*,ATLAS_J*,ATLAS_M*,ATLAS_P*,ATLAS_T*,gamma_stat_*,Lumi*,Theo*,Z*,*_fake_*

ws=combWS
dataset=combData

outstat=fitOutputs/${tagCfg}_statOnly
outsyst=fitOutputs/${tagCfg}_allSys
if [ ! -d ${outstat} ];then mkdir ${outstat};fi
if [ ! -d ${outsyst} ];then mkdir ${outsyst};fi

rundir=$(pwd)

dtilde=SM
source rename.sh

allJobs=jobsSub.sh
> ${allJobs}

condor=condor
for d in ${dtilde}
do
  dgam=$(dTran ${d})

  jobName=fitcomb_${dgam}; echo ${jobName}
  hepout=${condor}/hep_sub_${jobName}
  if [ ! -d ${hepout} ]; then mkdir ${hepout}; fi
  singexec=${condor}/singexe_${jobName}.sh
  > ${singexec}
  executable=${condor}/exe_${jobName}.sh
  > ${executable}

  echo "#!/bin/bash" >> ${singexec}
  echo "" >> ${singexec}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/workspaceCombiner/modVBFCP" >> ${singexec}
  echo "pwd" >> ${singexec}
  echo "singularity exec --env=\"LD_LIBRARY_PATH=/usr/local/venv/lib\" /cvmfs/unpacked.cern.ch/gitlab-registry.cern.ch/atlas_higgs_combination/software/hcomb-docker/analyzer:2-0 ${executable}" >> ${singexec}

  echo "#!/bin/bash" >> ${executable}
  echo "" >> ${executable}
  echo "quickFit -f workspaces/combined_${dgam}.root -w ${ws} -d ${dataset} -p ${pois} -o ${outsyst}/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}

  chmod +x ${executable}
  chmod +x ${singexec}

  echo "hep_sub ${singexec} -g atlas -os CentOS7 -wt mid -mem 4096 -o ${hepout}/log-0.out -e ${hepout}/log-0.err" >> ${allJobs}

  if [ "$(ls ${hepout}/)" != "" ];then rm ${hepout}/*;fi
done
