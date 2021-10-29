#!/bin/bash

source rename.sh

for d in ${dtilde}
do
quickFit -f workspace/combined_$(dTran ${d}).root -w combWS -d combData -p mu=1,mu_VBF_RW=1_0_5,mu_VBF_SM=0,mu_ggH=1,mu_ggH_SM=0,ATLAS_epsilon_rejected=1_0_5,ATLAS_epsilon=0 -o out/out_$(dTran ${d}).root
done
