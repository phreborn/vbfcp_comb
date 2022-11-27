#!/bin/bash

chws=$(cat ../cHW_fine)

for chw in ${chws}
do
  if [[ "${chw}" =~ "m" ]];then
    vchw=$(echo ${chw} | sed 's/m/-/g')
  elif [[ "${chw}" =~ "p" ]];then
    vchw=$(echo ${chw} | sed 's/p/+/g')
  fi
  vchw=$(echo ${vchw} | sed 's/d/./g')
  echo "  cHWs[\"${chw}\"] = \"${vchw}\";"
done
