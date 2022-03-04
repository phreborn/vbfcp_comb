#!/bin/bash

rundir=$(pwd)

source rename.sh

xmldir=asimovGamXml

mu=onfly
mu=mu1d00
mu=mu1d29

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/Asimov_${dgam}.xml
  if [ "${mu}" == "onfly" ];then
    cat ${xmldir}/exampleOnfly.xml > ${xmlfile}
  else
    cat ${xmldir}/example.xml > ${xmlfile}
  fi
  sed -i "s/m00/${dgam}/g" ${xmlfile}
  sed -i "s/:MU:/${mu}/g" ${xmlfile}
  if [ "${mu}" == "onfly" ];then
    echo "do nothing"
  else
    muval=$(echo ${mu} | awk '{ sub(/mu/,""); sub(/d/,"."); print $0 }')
    sed -i "s/:MUVAL:/${muval}/g" ${xmlfile}
  fi
done

cd ../../quickFit/
source setup_lxplus.sh
cd ${rundir}

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/Asimov_${dgam}.xml
  ../../quickFit/bin/quickAsimov -x ${xmlfile} -w combWS -m ModelConfig -d combData
done
