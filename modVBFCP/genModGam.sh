#!/bin/bash

rundir=$(pwd)

dtilde=SM
source rename.sh

xmldir=modGamXml

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/mod_${dgam}.xml
  cat ${xmldir}/example.xml > ${xmlfile}
  sed -i "s/m00/${dgam}/g" ${xmlfile}
done

allJobs=jobsSub.sh
> ${allJobs}

condor=condor
for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/mod_${dgam}.xml

  jobName=modgam_${dgam}; echo ${jobName}
  hepout=${condor}/hep_sub_${jobName}
  if [ ! -d ${hepout} ]; then mkdir ${hepout}; fi
  singexec=${condor}/singexe_${jobName}.sh
  > ${singexec}
  executable=${condor}/exe_${jobName}.sh
  > ${executable}

  echo "#!/bin/bash" >> ${singexec}
  echo "" >> ${singexec}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/workspaceCombiner/modVBFCP" >> ${singexec}
  echo "singularity exec --env=\"LD_LIBRARY_PATH=/usr/local/venv/lib\" /cvmfs/unpacked.cern.ch/gitlab-registry.cern.ch/atlas_higgs_combination/software/hcomb-docker/analyzer:2-2 ${executable}" >> ${singexec}

  echo "#!/bin/bash" >> ${executable}
  echo "" >> ${executable}
  echo "manager -w edit -x ${xmlfile}" >> ${executable}

  chmod +x ${executable}
  chmod +x ${singexec}

  echo "hep_sub ${singexec} -g atlas -os CentOS7 -wt mid -mem 4096 -o ${hepout}/log-0.out -e ${hepout}/log-0.err" >> ${allJobs}

  if [ "$(ls ${hepout}/)" != "" ];then rm ${hepout}/*;fi
done
