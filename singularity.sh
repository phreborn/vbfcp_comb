#singularity run /cvmfs/unpacked.cern.ch/gitlab-registry.cern.ch/atlas_higgs_combination/software/hcomb-docker/analyzer:paper2021-3
#singularity run /cvmfs/unpacked.cern.ch/gitlab-registry.cern.ch/atlas_higgs_combination/software/hcomb-docker/analyzer:paper2021-8
#Singularity> ulimit -S -s unlimited -u unlimited
#
#singularity shell -B vbfcpComb/:/mnt /cvmfs/unpacked.cern.ch/gitlab-registry.cern.ch/atlas_higgs_combination/software/hcomb-docker/analyzer:paper2021-8
#
#singularity shell -B vbfcpComb/:/mnt /cvmfs/unpacked.cern.ch/gitlab-registry.cern.ch/atlas_higgs_combination/software/hcomb-docker/analyzer:2-2
ipath=$1
singularity shell -B ${ipath}:/mnt /cvmfs/unpacked.cern.ch/gitlab-registry.cern.ch/atlas_higgs_combination/software/hcomb-docker/analyzer:2-2
