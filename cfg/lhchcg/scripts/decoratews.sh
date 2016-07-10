#!/bin/bash

jobname=$1
shift
mass=$1

dname="combData"
mname="ModelConfig"
wname="combWS"

input=${mass}.root

mHhat=125.4
# ***************measure mu********************
#./exe/combine -D $dname --seed 0 --optimizeSimPdf false -d $input -m $mHhat --modelConfigName $mname --nllOffset true -M Asymptotic -w $wname -v 2 --singlePoint mu=1_0_5,m_H=$mHhat --makeBOnly false --scanMass $mHhat --name .mor14 --minimizerStrategy 1  --saveSS true >fitmu.log &
# ***************measure mH********************
pushd workspace/gg/$jobname/
ln -s /afs/cern.ch/user/y/yanght/workarea/combination/workspaceCombiner/bin/manager
cp /afs/cern.ch/user/y/yanght/workarea/combination/workspaceCombiner/lib/* .
ln -s /afs/cern.ch/user/y/yanght/workarea/combination/workspaceCombiner/setup.sh
ln -s /afs/cern.ch/user/y/yanght/workarea/combination/workspaceCombiner/cfg/

source setup.sh


./manager -w decorate -f ${mass}.root -p ${mass}_decorated.root
sh cfg/hsg1/run_poi_moriond13.sh ${mass}_decorated.root ${mass}_poi.root

popd

#sh decoratews.sh workspace_R25m8_rescale_v2 1265
#sh decoratews.sh workspace_R25m7_rescale_v2 1265

