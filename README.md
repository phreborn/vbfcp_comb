The tool is created by Haoshuang Ji.

To compile, Run

> source setup.sh

> make

To implement kappa parametrization, first go to file "cfg/tth/model_plus_NLO.xml" and modify the input/output workspace file names. Also double check if the mu handlers needed have been implemented in your workspace and have correct names

Then you can produce the kappa parametrization workspace by

> manager -w orgnize -x cfg/tth/model_plus_NLO.xml

The argument "orgnize" is apparently typo of "organize" due to historical reason. To respect the work of the author I decide to leave it, but feel free to change the code if you feel uncomfortable.