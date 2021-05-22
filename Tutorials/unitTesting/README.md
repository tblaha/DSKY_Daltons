# How to develop unit tests for DSKY Daltons modules

Definitions: 

- `<myPath>` stands for a path on your local (virtual) machine that will hold _all_ of our project. It's best if that is an empty directory as of now.
- `<myModule>` is the name of the module that you will be working on (Capital first letter!!!)


Add module `controller_unit_tests` using 
```
alias dp=dueca-project
cd <mypath>
dp new-module
# Name: controller_unit_tests
new-module dusime controller_unit_tests
```

Duplicate the `run/solo/solo/` directory and name it it `run/solo/tester` using:
```
cd run/solo
cp -R solo tester
cd ../..
```

Now, write your unit tests in `<myPath>/controller_unit_tests/` by providing known time-series inputs on the required comm channels (`vehicleState` and `PrimaryControls` for instance) and evaluating the response of the controller (on the `thrusterForces` channel, for instance). 

ToDo:
create a logfile with time stap, CVS commit number, yadiyada and a list of the tests that were run and whether they passed or not.

Then, add the `controller_unit_tests` module to the `run/solo/tester/dueca_mod.py` and run dueca with:
```
alias dt=(cd run/solo/tester/ && ../../../dueca_run.x)   # note the tester subdir, not solo anymore
dt
```


### template

Till will, at some point, make a template that can maybe be `copy-module`ed and contains the logfile structure and examples for the asserts.
