#!/bin/bash

folder="lhchcg"
logname="ATLAS_hzz"

source ./setup.sh
make

echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${logname}.setup
which root
which gcc

manager -w combine -x cfg/${folder}/atlas_hzz_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/ATLAS_hzz_asimov.root --procedure 4  --ReBin -1 >& workspace/${folder}/myrep/${logname}_asimov.log &
manager -w combine -x cfg/${folder}/atlas_hzz_toy.xml -s true -t 2 -f workspace/${folder}/myrep/ATLAS_hzz_toy.root --procedure 4 --ReBin -1 >& workspace/${folder}/myrep/${logname}_toy.log &
