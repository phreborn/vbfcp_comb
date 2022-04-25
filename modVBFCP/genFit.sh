#!/bin/bash

expected=1

channel=Comb

pois="mu=1,mu_BR_yy=1,mu_spur_SM=0,mu_ggH_SM=0,mu_VBF_SM=0,mu_spur=1,mu_ggH=1,mu_VBF_RW=1_0_5,ATLAS_epsilon=0,ATLAS_epsilon_rejected=1_-5_5"
fix_stat=ATLAS_*_HGam,ATLAS_B*,ATLAS_E*,ATLAS_F*,ATLAS_J*,ATLAS_L*,ATLAS_M*,ATLAS_P*,ATLAS_T*,BR_*,gamma_stat_*,Lumi*,Theo*,Z*,*_fake_*,QCDalphaS*

ws=combWS
dataset=combData

matchglob=
asiWS=
if [ ${expected} -eq 1 ];then
  dataset=asimovData_SM_Mu1
  matchglob="-s Glob_SM_Mu1"
  asiWS="_Asi"
fi

outstat=fitOutputs/${channel}_statOnly
outsyst=fitOutputs/${channel}_allSys
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
  hepout=${condor}/hepsub_${jobName}
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
  echo "quickFit -f workspaces/combined_${dgam}${asiWS}.root -w ${ws} -d ${dataset} ${matchglob} -p ${pois} -n ${fix_stat} -o ${outstat}/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}
  echo "quickFit -f workspaces/combined_${dgam}${asiWS}.root -w ${ws} -d ${dataset} ${matchglob} -p ${pois} -o ${outsyst}/out_${dgam}.root --savefitresult 1 --saveWS 1" >> ${executable}

  chmod +x ${executable}
  chmod +x ${singexec}

  echo "hep_sub ${singexec} -g atlas -os CentOS7 -wt mid -mem 4096 -o ${hepout}/log-0.out -e ${hepout}/log-0.err" >> ${allJobs}

  if [ "$(ls ${hepout}/)" != "" ];then rm ${hepout}/*;fi
done
