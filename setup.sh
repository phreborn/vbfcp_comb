#!/bin/bash
# $Id: setup.sh 80 2012-01-31 19:08:31Z hji $

# The Makefiles depend only on the root-config script to use ROOT,
# so make sure that is available
# if [[ `which root-config` == "" ]]; then
#     echo "Info: Setting up environment for root first!"
#     # source ~/setROOT_patch.sh
#     source ~/setROOT32.sh
#     if [[ `which root-config` == "" ]]; then
#         echo "Error: Setting up root failed!"
#     fi
# fi

# gcc
#source /afs/cern.ch/project/wiscatlas/HtoGG/yanght/xdata2/root-v5-34_x86-64/setup.sh
#export LD_LIBRARY_PATH=${ROOTSYS}/lib:${LD_LIBRARY_PATH}

source /afs/cern.ch/user/y/yanght/bin/setupROOT.sh

which gcc
which root

#dynamic libraries
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/lib:/afs/cern.ch/cms/slc6_amd64_gcc472/external/boost/1.51.0-cms/lib/
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/lib:/afs/cern.ch/cms/slc5_amd64_gcc434/external/boost/1.44.0-cms/lib/

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
