#!/bin/bash

#cat=AllCats
#cat=Inject
#cat=LL
cat=TEST
#for suffix in statOnly allSys ssSys jetSys photonSys theorySys;do
for suffix in statOnly;do
  outnll=autonll/dNLL_${cat}_${suffix}.log
  > ${outnll}
  tmpall=tmpall
  root -b -q getNLL.cxx\(\"out${cat}_${suffix}\"\) | grep nll | cut -d : -f 2 > ${tmpall}
  cat ${tmpall} | grep m | sort -r | sed 's/m/-0\./g' >> ${outnll}
  cat ${tmpall} | grep p | sed 's/p/0\./g' >> ${outnll}
  rm ${tmpall}
done
