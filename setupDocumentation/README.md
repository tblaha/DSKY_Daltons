# create main project

*IMPORTANT: Do not do any of this! Till has already done it. Just for illustration, reference and documentation of the pain...* 

Note: `<myPath>` stands for some (invariant)  path on my local machine.

```
cd <myPath>
dueca-project new
# name DSKY_Daltons_Project
# use python
```

### Add the custom `.dco` files

```
cd DSKY_Daltons_Project/DSKY_Daltons_Project/comm-objects
```

Now, create the custom `.dco` files here (not the to be borrowed ones). Make sure that `tree` gives this output:

```
.
├── comm-objects.lst
├── CVS
│   ├── Entries
│   ├── Repository
│   └── Root
├── flightControlModes.dco
├── initState.dco
├── Makefile
├── setpoints.dco
├── terrainDataAtCurrentLocation.dco
├── thrusterForces.dco
└── vehicleState.dco
```

Add those `.dco`s to the `.lst`:


Add sources with `dueca-project add-source *.dco` (all of the `.dco`'s).

Commit
```
cd <myPath>/DSKY_Daltons_Project/DSKY_Daltons_Project/
dueca-project commit
```


### Add borrowed `.dco` files


### Borrow/copy existing modules

Move to correct directory  `cd DSKY_Daltons_Project/DSKY_Daltons_Project`

*Borrow Stick related modules*
```
dueca-project borrow-module
# name: SimCourse2021/multi-stick

dueca-project borrow-module
# name: SimCourse2021/gui-stick
```

*Copy HUD (as we'll have to change it)*
```
dueca-project copy-module
# name: SimCourse2021/F16HUD
```

### Verify what we got so far

```
<myPath>/DSKy_Daltons_Project $ tree -L 2
.
├── DSKY_Daltons_Project
│   ├── comm-objects
│   ├── CVS
│   ├── F16HUD
│   ├── Makefile -> Makefile.common
│   ├── Makefile.common
│   ├── modules.solo
│   └── run
└── SimCourse2021
    ├── comm-objects
    └── multi-stick
```

### commit

```
cd DSKY_Daltons_Project/DSKY_Daltons_Project
dueca-project update
dueca-project commit
```



# Create secondary projects for our own modules


### Controller

Make project
```
cd <myPath>
dueca-project new
# Name DSKY_Daltons_Controller
# use python
```

Make new module for the controller
```
cd DSKY_Daltons_Controller/DSKY_Daltons_Controller
dueca-project new-module controller

cd controller
new-module dusime controller
# bytes: 0
# descr: "takes stick inputs and current rates. Outputs thruster forces and engine thrust command"

dueca-project add-source controller.*xx
```

Add `.dco`s to `./controller/comm-objects.lst` and update them from the main repo:
```
# this file contains a list of all communicated objects needed by this
# specific module
# specify these with the .dco extension, examples:
# DSKY_Daltons_Controller/comm-objects/MyDataObject.dco
# Generic/comm-objects/PrimaryControls.dco
DSKY_Daltons_Project/comm-objects/vehicleState.dco
DSKY_Daltons_Project/comm-objects/flightControlModes.dco
DSKY_Daltons_Project/comm-objects/setpoints.dco
DSKY_Daltons_Project/comm-objects/thrusterForces.dco

SimCourse2021/comm-objects/PrimaryControls.dco
SimCourse2021/comm-objects/SecondaryControls.dco
SimCourse2021/comm-objects/PrimarySwitches.dco
SimCourse2021/comm-objects/SecondarySwitches.dco
```

Update the module:
```
cd <myPath>/DSKY_Daltons_Controller/DSKY_Daltons_Controller
dueca-project update
```

Commit, voor de zekerheid:
```
dueca-project commit
```


### Dynamics

Make project
```
cd <myPath>
dueca-project new
# Name DSKY_Daltons_Dynamics
# use python
```

Make new module for the dynamics
```
cd DSKY_Daltons_Dynamics/DSKY_Daltons_Dynamics
dueca-project new-module dynamics

cd dynamics
new-module dusime dynamics
# bytes: 0
# descr: "Integrates all forces. Also provides landing gear sim"

dueca-project add-source dynamics.*xx
```

Add `.dco`s to `./dynamics/comm-objects.lst` and update them from the main repo:
```
# this file contains a list of all communicated objects needed by this
# specific module
# specify these with the .dco extension, examples:
# DSKY_Daltons_Dynamics/comm-objects/MyDataObject.dco
# Generic/comm-objects/PrimaryControls.dco

DSKY_Daltons_Project/comm-objects/vehicleState.dco
DSKY_Daltons_Project/comm-objects/thrusterForces.dco
DSKY_Daltons_Project/comm-objects/terrainDataAtCurrentLocation.dco
DSKY_Daltons_Project/comm-objects/initState.dco
```

Update the module:
```
cd <myPath>/DSKY_Daltons_Dynamics/DSKY_Daltons_Dynamics
dueca-project update
```

Commit, voor de zekerheid:
```
dueca-project commit
```



### HUD

Make project
```
cd <myPath>
dueca-project new
# Name DSKY_Daltons_HUD
# use python
```

Make new module for the hud
```
cd DSKY_Daltons_HUD/DSKY_Daltons_HUD
dueca-project copy-module
# Name: SimCourse2021/F16HUD
# version: hit enter for latest

cd F16HUD
```

Add `.dco`s to `./F16HUD/comm-objects.lst` and update them from the main repo:
```
# this file contains a list of all communicated objects needed by this 
# specific module
# specify these with the .dco extension, examples:
# SimCourse2021/comm-objects/MyDataObject.dco
DSKY_Daltons_Project/comm-objects/vehicleState.dco
DSKY_Daltons_Project/comm-objects/flightControlModes.dco
DSKY_Daltons_Project/comm-objects/setpoints.dco
```

Update the module:
```
cd <myPath>/DSKY_Daltons_HUD/DSKY_Daltons_HUD
dueca-project update
```

Commit, voor de zekerheid:
```
dueca-project commit
```




