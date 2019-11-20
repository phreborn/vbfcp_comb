#!/bin/bash
# $Id: setup.sh 80 2012-01-31 19:08:31Z hji $
# Updated by Hongtao Yang on Jan 23, 2017
#

export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase # use your path
source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh
lsetup "root 6.14.04-x86_64-slc6-gcc62-opt" 
lsetup cmake
lsetup "boost boost-1.66.0-python2.7-x86_64-slc6-gcc62"

ulimit -S -s unlimited

which gcc
which root

# dynamic libraries

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/lib:/cvmfs/sft.cern.ch/lcg/releases/LCG_94/Boost/1.66.0/x86_64-slc6-gcc62-opt/lib/

# Greet the user
echo "Setting up environment for compiling/running SFrame"

if [ $_DIR ]; then
    echo _DIR is already defined, use a clean shell
    return 1
fi

# speficy the SFRAME base directory, i.e. the directory in which this file lives
export _DIR=${PWD}

# Modify to describe your directory structure. Default is to use the a structure where
# all directories are below the SFrame base directory specified above
export _BIN_PATH=${_DIR}/bin
export _LIB_PATH=${_DIR}/lib

# Check if bin/lib/include directories exist, if not create them
if [ ! -d ${_BIN_PATH} ]; then
    echo Directory ${_BIN_PATH} does not exist ... creating it
    mkdir ${_BIN_PATH}
fi
if [ ! -d ${_LIB_PATH} ]; then
    echo Directory ${_LIB_PATH} does not exist ... creating it
    mkdir ${_LIB_PATH}
fi

# The Makefiles depend only on the root-config script to use ROOT,
# so make sure that is available
if [[ `which root-config` == "" ]]; then
    echo "Error: ROOT environment doesn't seem to be configured!"
fi

if [[ `root-config --platform` == "macosx" ]]; then

    # With Fink ROOT installations, DYLD_LIBRARY_PATH doesn't have
    # to be defined for ROOT to work. So let's leave the test for it...
    export DYLD_LIBRARY_PATH=${_LIB_PATH}:${DYLD_LIBRARY_PATH}

else

    if [ ! $LD_LIBRARY_PATH ]; then
        echo "Warning: so far you haven't setup your ROOT enviroment properly (no LD_LIBRARY_PATH): SFrame will not work"
    fi

    export LD_LIBRARY_PATH=${_LIB_PATH}:${LD_LIBRARY_PATH}

fi

export PATH=${_BIN_PATH}:${PATH}
export PYTHONPATH=${_DIR}/python:${PYTHONPATH}

export PAR_PATH=./:${_LIB_PATH}
