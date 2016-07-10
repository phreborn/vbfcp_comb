#!/bin/bash

folder="lhchcg"
logname="ATLAS_hgg"

source ./setup.sh
make

echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${logname}.setup
which root
which gcc

#manager -w combine -x cfg/${folder}/atlas_hgamgam_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/ATLAS_hgg_asimov.root --procedure 4 >& workspace/${folder}/myrep/${logname}_asimov.log &
#manager -w combine -x cfg/${folder}/atlas_hgamgam_toy.xml -s true -t 2 -f workspace/${folder}/myrep/ATLAS_hgg_toy.root --procedure 4 --ReBin -1 >& workspace/${folder}/myrep/${logname}_toy.log &

