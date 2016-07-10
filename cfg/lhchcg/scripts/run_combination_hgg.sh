#!/bin/bash

folder="lhchcg"
logname="combined_hgg"

echo $(which root; which gcc; date) >  workspace/${folder}/myrep/${logname}.setup
which root
which gcc

manager -w combine -x cfg/${folder}/combined_hgamgam_asimov.xml -s true -t 2 -f workspace/${folder}/myrep/combined_hgg_asimov.root --procedure 4 >& tee workspace/${folder}/myrep/${logname}_asimov.log &
manager -w combine -x cfg/${folder}/combined_hgamgam_toy.xml -s true -t 2 -f workspace/${folder}/myrep/combined_hgg_toy.root --procedure 4 >& tee workspace/${folder}/myrep/${logname}_toy.log &

