#!/bin/bash

source rename.sh

for d in ${dtilde}
do
  python makeXml.py -d $(dTran ${d})
done
