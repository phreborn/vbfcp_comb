#!/bin/bash

source rename.sh

dir=condor
allJobs=jobsSub.sh
> ${allJobs}
if [ "$2" != "" ];then dtilde=$2;fi #here dtilde from tau def, e.g. m001
for d in ${dtilde}
do
  dgam=$(dTran ${d})
  jobName=combws_${dgam}; echo ${jobName}
  #if [ ! -d csv/${jobName} ];then mkdir -p csv/${jobName};fi
  if [ ! -d ${dir}/hepsub_${jobName} ]; then mkdir ${dir}/hepsub_${jobName}; fi
  executable=${dir}/exe_${jobName}.sh
  > ${executable}

  echo "#!/bin/bash" >> ${executable}
  echo "" >> ${executable}
  echo "cd /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/workspaceCombiner/" >> ${executable}
  echo "source setup_lxplus.sh" >> ${executable}
  echo "cd vbfcpComb/" >> ${executable}
  echo "" >> ${executable}
  cmd="../bin/manager -w combine -x xml/comb_$(dTran ${d}).xml -s 0"
  echo "${cmd}" >> ${executable}
  chmod +x ${executable}
  echo "hep_sub ${executable} -g atlas -os CentOS7 -wt mid -mem 2048 -o ${dir}/hepsub_${jobName}/log-0.out -e ${dir}/hepsub_${jobName}/log-0.err" >> ${allJobs}
  if [ "$(ls ${dir}/hepsub_${jobName}/)" != "" ];then rm ${dir}/hepsub_${jobName}/*;fi
done
