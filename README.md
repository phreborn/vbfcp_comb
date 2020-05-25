The tool was created by Haoshuang Ji.

Do not forget to clone the RooFitExtensions submodule as well.

> git clone --recursive ssh://git@gitlab.cern.ch:7999/atlas_higgs_combination/software/workspaceCombiner.git

To compile on lxplus, run

> source setup.sh

> mkdir build && cd build && cmake .. && make -j7 && cd ..

Then each time before using the tool, just do 

> source setup.sh

Usage of the tool is provided at

https://twiki.cern.ch/twiki/bin/view/AtlasProtected/WorkspaceCombiner

For any questions please contact Hongtao.Yang@cern.ch
