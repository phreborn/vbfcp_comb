#!/bin/bash

expobs=Expected
expobs=Observed

cat=Tau
cat=Gam
cat=Comb
cat=Comb_Asi
for suffix in statOnly allSys;do
  outnll=${expobs}/dNLL_${cat}_${suffix}.log
  > ${outnll}
  tmpall=tmpall
  root -b -q getNLL.cxx\(\"fitOutputs/${cat}_${suffix}\"\) | grep nll | cut -d : -f 2 > ${tmpall}
  cat ${tmpall} | grep m | sort -r | sed 's/m/-0\./g' >> ${outnll}
  cat ${tmpall} | grep p | sed 's/p/0\./g' >> ${outnll}
  rm ${tmpall}
done
