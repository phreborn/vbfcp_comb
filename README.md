The tool was created by Haoshuang Ji (University of Wisconsin) and rewritten by Hongtao Yang.

# Installation
To install the software, first clone the project

> git clone ssh://git@gitlab.cern.ch:7999/atlas_higgs_combination/software/workspaceCombiner.git

Then setup ROOT and cmake. On lxplus one can directly use the script

> source setup_lxplus.sh

The package depends on [RooFitExtensions](https://gitlab.cern.ch/atlas_higgs_combination/software/RooFitExtensions). If RooFitExtensions is not installed yet, first run

> sh scripts/install_roofitext.sh

Now compile the code:

> mkdir build
> cd build
> cmake .. # The default installation folder is the source code dir. If you need a different location, please specify with -DCMAKE_INSTALL_PREFIX=<your path>
> make -j8 
> make install # Moving the executables to workspaceCombiner/bin
> cd ..

Now the software should be ready for use. Next time before using it, one only need to setup ROOT.

# Usage

Usage of the tool is provided at

https://twiki.cern.ch/twiki/bin/view/AtlasProtected/WorkspaceCombiner (obsolete, to be updated)

Below are some instructions based on ICHEP 2020 combination
## Workspace combination
An example for the mu workspace combination is provided at

> /afs/cern.ch/user/y/yanght/public/combination_mu_new_wscombiner.xml

Please copy the file to overwrite your combination_mu.xml, and also copy the latest [dtd/Combination.dtd](https://gitlab.cern.ch/atlas_higgs_combination/software/workspaceCombiner/-/blob/development/dtd/Combination.dtd) to the same folder. Then run

> manager -w combine -x <your XML file path> -s 0

## Workspace editing

The workspace editing syntax is unchanged wrt ICHEP 2020, except that now the method is invoked by keyword "edit" instead of "organize", i.e.

> manager -w edit -x <your XML file path>

## Workspace printing/splitting/regulating

For splitting, the sytax is basically unchanged, except that now you need to specify the workspace name and ModelConfig name (it is not good to let the code dig it out)

> manager -w split -f <your root file> --wsName <your workspace name> --mcName <your ModelConfig name> --dataName <your data name> ...

For workspace printing, it is simplying printing out the workspace content without further operations.

For workspace regulating, it is equivalent to splitting including all indices ("-i all") with rebuild PDF option (remove constraint PDFs that are not relevant) switched on.

For any questions please contact workspaceCombiner-user@cern.ch
