#!/bin/bash

# This script is for installing RooFitExtensions. If the user has installed the package somewhere, please make sure cmake can find it
if [ $# -gt 1 ]; then
    outputDir=${1}
else
    outputDir=`pwd`
fi

if [ ! -d ${outputDir}/RooFitExtensions ]; then
    mkdir -vp ${outputDir}
    echo "Cloning RooFitExtensions into target directory ${outputDir}..."
    git clone https://gitlab.cern.ch/atlas_higgs_combination/software/RooFitExtensions.git ${outputDir}/RooFitExtensions
    if [ -d ${outputDir}/RooFitExtensions ]; then
        pushd ${outputDir}/RooFitExtensions
        cmake . && make -j 4
        popd
    fi

    export RooFitExtensions_DIR=${outputDir}/RooFitExtensions
fi
