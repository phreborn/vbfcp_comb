#!/bin/bash

rundir=$(pwd)

dtilde=SM
source rename.sh

xmldir=asimovTauXml

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/Asimov_${dgam}.xml
  cat ${xmldir}/example.xml > ${xmlfile}
  sed -i "s/m00/${dgam}/g" ${xmlfile}
done

cd ../../quickFit/
source setup_lxplus.sh
cd ${rundir}

for d in ${dtilde}
do
  dgam=$(dTran ${d})
  xmlfile=${xmldir}/Asimov_${dgam}.xml
  ../../quickFit/bin/quickAsimov -x ${xmlfile} -w combined -m ModelConfig -d obsData
done
