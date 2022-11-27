#!/bin/bash

rundir=$(pwd)
singws="\/mnt"

dtilde=SM
source rename.sh

xmldir=run3ProspGam

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/mod_${dgam}.xml
  cat ${xmldir}/example.xml > ${xmlfile}
  sed -i "s/m00/${dgam}/g" ${xmlfile}
  sed -i "s/InFile=\"/InFile=\"${singws}\//g" ${xmlfile}
  sed -i "s/OutFile=\"/OutFile=\"${singws}\//g" ${xmlfile}
done
singws=$(echo ${singws} | cut -d '\' -f 2) # ugly way

allJobs=run3jobsSub.sh
> ${allJobs}

condor=condor
for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/mod_${dgam}.xml

  jobName=run3gam_${dgam}; echo ${jobName}
  hepout=${condor}/hepsub_${jobName}
  if [ ! -d ${hepout} ]; then mkdir ${hepout}; fi
  singexec=${condor}/singexe_${jobName}.sh
  > ${singexec}
  executable=${condor}/exe_${jobName}.sh
  > ${executable}

  echo "#!/bin/bash" >> ${singexec}
  echo "" >> ${singexec}
  echo "cd ${rundir}" >> ${singexec}
  echo "singularity exec --env=\"LD_LIBRARY_PATH=/usr/local/venv/lib\" -B ./:${singws} /cvmfs/unpacked.cern.ch/gitlab-registry.cern.ch/atlas_higgs_combination/software/hcomb-docker/analyzer:2-2 ${singws}/${executable}" >> ${singexec}

  echo "#!/bin/bash" >> ${executable}
  echo "" >> ${executable}
  echo "manager -w edit -x ${singws}/${xmlfile}" >> ${executable}

  chmod +x ${executable}
  chmod +x ${singexec}

  echo "hep_sub ${singexec} -g atlas -os CentOS7 -wt mid -mem 4096 -o ${hepout}/log-0.out -e ${hepout}/log-0.err" >> ${allJobs}

  if [ "$(ls ${hepout}/)" != "" ];then rm ${hepout}/*;fi
done
