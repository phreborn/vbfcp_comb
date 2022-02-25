#!/bin/bash

cat=Tau
cat=Gam
cat=Comb
for suffix in statOnly allSys;do
  outnll=autonll/dNLL_${cat}_${suffix}.log
  > ${outnll}
  tmpall=tmpall
  root -b -q getNLL.cxx\(\"out${cat}_${suffix}\"\) | grep nll | cut -d : -f 2 > ${tmpall}
  cat ${tmpall} | grep m | sort -r | sed 's/m/-0\./g' >> ${outnll}
  cat ${tmpall} | grep p | sed 's/p/0\./g' >> ${outnll}
  rm ${tmpall}
done
