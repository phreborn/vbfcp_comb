#!/bin/bash

dtilde=$(cat /scratchfs/atlas/huirun/atlaswork/VBF_CP/WSBuilder/histFactory/vbfcpTau/dtilde.list)

function dTran(){
  if [[ "$1" =~ "m" ]];then
    echo $1 | sed 's/m0/m/g'
  elif [[ "$1" =~ "p" ]];then
    echo $1 | sed 's/p0/p/g'
  elif [[ "$1" =~ "SM" ]];then
    echo "m00"
  fi
}
