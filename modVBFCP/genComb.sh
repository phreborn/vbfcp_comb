#!/bin/bash

rundir=$(pwd)

dtilde=SM
source rename.sh

xmldir=combXml

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/comb_${dgam}.xml
  cat ${xmldir}/example.xml > ${xmlfile}
  sed -i "s/m00/${dgam}/g" ${xmlfile}
done

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/comb_${dgam}.xml
  manager -w combine -x ${xmlfile}
done
