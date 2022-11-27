#!/bin/bash
fitrslt=../run3fitrslt

expobs=Observed
expobs=Expected

chan=Tau
chan=Comb_Asi
chan=Comb
chan=Gam
#for statType in statOnly allSys;do
for statType in allSys;do
  outnll=${expobs}/dNLL_${chan}_${statType}.log
  > ${outnll}
  tmpall=tmpall
  root -b -q getNLL.cxx\(\"${fitrslt}/${chan}_${statType}\"\) | grep nll | cut -d : -f 2 > ${tmpall}

  ##dtilde
  #cat ${tmpall} | grep m | sort -r | sed 's/m/-0\./g' >> ${outnll}
  #cat ${tmpall} | grep p | sed 's/p/0\./g' >> ${outnll}

  #cHW
  cat ${tmpall} | grep m | sort -r | sed 's/m/-/g' | sed 's/d/./g' >> ${outnll}
  cat ${tmpall} | grep p | sed 's/p//g' | sed 's/d/./g' >> ${outnll}
  rm ${tmpall}
done
