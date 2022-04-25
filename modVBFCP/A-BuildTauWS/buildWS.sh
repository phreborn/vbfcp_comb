#!/bin/bash

# refer to https://twiki.cern.ch/twiki/pub/RooStats/WebHome/HistFactoryLikelihood.pdf
# data/ results/ config/, these 3 directories are created by command 'prepareHistFactory'
# config/HistFactorySchema.dtd should be copied to workspaces/VBFCP_comb_data_SM/xml/

rundir=$(pwd)

dtilde=SM
cd ../
source rename.sh
cd -

allJobs=jobsSub.sh
> ${allJobs}

condor=condor
for d in ${dtilde}
do
  dgam=$(dTran ${d})

  jobName=build_${dgam}; echo ${jobName}
  hepout=${condor}/hepsub_${jobName}
  if [ ! -d ${hepout} ]; then mkdir ${hepout}; fi
  executable=${condor}/exe_${jobName}.sh
  > ${executable}

  wsdir=workspaces/VBFCP_comb_data_${d}

  echo "#!/bin/bash" >> ${executable}
  echo "" >> ${executable}
  echo "export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase" >> ${executable}
  echo "source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh" >> ${executable}
  echo "lsetup "views LCG_97a x86_64-centos7-gcc8-opt"" >> ${executable}
  echo "lsetup root" >> ${executable}
  echo "" >> ${executable}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/workspaceCombiner/modVBFCP/A-BuildTauWS" >> ${executable}
  echo "if [ ! -d ${wsdir} ];then" >> ${executable}
  echo "  mkdir ${wsdir}" >> ${executable}
  echo "fi" >> ${executable}
  echo "echo ${wsdir}" >> ${executable}
  echo "cp -r 130719_MVA_comb_data/${d}/xml workspaces/VBFCP_comb_data_${d}/" >> ${executable}
  echo "python modOriXml.py -d ${d}" >> ${executable}
  echo "cp config/HistFactorySchema.dtd workspaces/VBFCP_comb_data_${d}/xml/" >> ${executable}
  echo "hist2workspace workspaces/VBFCP_comb_data_${d}/xml/VBFCPCOMBVBFMVA_FSPLIT.xml" >> ${executable}

  chmod +x ${executable}

  echo "hep_sub ${executable} -g atlas -os CentOS7 -wt mid -mem 4096 -o ${hepout}/log-0.out -e ${hepout}/log-0.err" >> ${allJobs}

  if [ "$(ls ${hepout}/)" != "" ];then rm ${hepout}/*;fi
done
