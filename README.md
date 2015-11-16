The tool is created by Haoshuang Ji.

To compile, Run

> source setup.sh

> make

To implement kappa parametrization, first go to file "cfg/tth/model_plus.xml" and modify the input/output workspace file names. Also double check if the mu handlers needed have been implemented in your workspace and have correct names

Then you can produce the kappa parametrization workspace by

> manager -w organize -x cfg/tth/model_plus.xml

