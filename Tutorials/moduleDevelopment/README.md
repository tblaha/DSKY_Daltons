# How to develop modules for DSKY Daltons

Definitions: 

- `<myPath>` stands for a path on your local (virtual) machine that will hold _all_ of our project. It's best if that is an empty directory as of now.
- `<myModule>` is the name of the module that you will be working on (Capital first letter!!!)


### Overview


##### Main Project

We will have one project called `DSKY_Daltons_Project` that ...

- will not contain any modules of its own (only borrows).
- will contain all of the custom `.dco`s (however, not the referee `.dco` or the stick `.dco`s)

Thus, all modules that the main `_Project` will be developed in other "projects" and only borrowed. However, these modules will reference the `.dco` from the `_Project` and *NEVER* create/contain *any* `.dco`s on their own. This sounds like a circular reference, but somehow works.


##### Modules

We will have the modules (for instance `controller`) in separate projects (for instance `DSKY_Daltons_Controller`). In these projects, other modules can be developed, for instance to test the controller, but at least the primary one (in this case `controller`) must be present and write to the channels as it will eventually be borrowed by the main `DSKY_Daltons_Project`.

Modules that we need to implement from scratch, identified up to now:

- `controller` housed in `DSKY_Daltons_Controller`
- `dynamics` housed in `DSKY_Daltons_Dynamics` (also should do landing gear sim now)
- `F16HUD` (copied from `SimCourse2021`), housed in `DSKY_Daltons_HUD`




### Setup your environment like this:

```
cd <myPath>
dueca-project checkout
# name: DSKY_Daltons_<myModule>   #  (for instance DSKY_Daltons_Controller)
# just hit enter for the next questions
```

Verify that `tree -L 4` gives the following output (of course,`controller` should be `<myModule>` everywhere, the imported `comm-objects` may be different and the contents of `<myModule>` are different):
```
.
└── DSKY_Daltons_Controller
    ├── DSKY_Daltons_Controller
    │   ├── comm-objects
    │   │   ├── comm-objects.lst
    │   │   ├── CVS
    │   │   ├── dummy.o
    │   │   └── Makefile
    │   ├── controller
    │   │   ├── comm-objects.lst
    │   │   ├── controller.cxx
    │   │   ├── controller.hxx
    │   │   ├── CVS
    │   │   ├── dummy.o
    │   │   └── Makefile
    │   ├── CVS
    │   │   ├── Entries
    │   │   ├── Entries.Static
    │   │   ├── Repository
    │   │   └── Root
    │   ├── Makefile -> Makefile.common
    │   ├── Makefile.common
    │   ├── modules.solo
    │   └── run
    │       ├── CVS
    │       ├── run-data
    │       └── solo
    ├── DSKY_Daltons_Project
    │   └── comm-objects
    │       ├── comm-objects.lst
    │       ├── flightControlModes.dco
    │       ├── initState.dco
    │       ├── Makefile
    │       ├── setpoints.dco
    │       ├── terrainDataAtCurrentLocation.dco
    │       ├── thrusterForces.dco
    │       └── vehicleState.dco
    └── SimCourse2021
        └── comm-objects
            ├── comm-objects.lst
            ├── Makefile
            ├── PrimaryControls.dco
            ├── PrimarySwitches.dco
            ├── SecondaryControls.dco
            └── SecondarySwitches.dco

15 directories, 29 files
```


### Guidelines

1. When you start working, make sure you have a backup (see last point), then go to `cd <myPath> DSKY_Daltons_<myModule>/DSKY_Daltons_<myModule>/` and do a `dueca-project update`

2. *never* change or add any `.dco` files anywhere inside `DSKY_Daltons_<myModule>/DSKY_Daltons_<myModule>/`
    - instead, we want to keep them centralized in the `DSKY_Daltons_Project`. Ask Till to fix shit for you, if broken.

3. *never* modify the `DSKY_Daltons_<myModule>/DSKY_Daltons_<myModule>/comm-objects/comm-objects.lst`!
    - modify the `DSKY_Daltons_<myModule>/DSKY_Daltons_<myModule>/<myModule>/comm-objects/comm-objects.lst` instead.
    - this is a _tiny_ difference, but important when borrowing the modules later.

4. *never* modify anything in `DSKY_Daltons_<myModule>/DSKY_Daltons_Project/` or `DSKY_Daltons_<myModule>/SimCourse2021/` or `DSKY_Daltons_<myModule>/Referees/`. You won't have permissions to commit those changes anyway. If there is a problem with these modules, let Till know.

5. To include `.dco`s, add a line to the `DSKY_Daltons_<myModule>/DSKY_Daltons_<myModule>/<myModule>/comm-objects/comm-objects.lst` of the format:
    - `<project_name>/comm-objects/<channel>.dco`
    - Example: `SimCourse2021/comm-objects/PrimaryControls.dco` or `DSKY_Daltons_Project/comm-objects/vehicleState.dco` and hopefully soon `Referees/comm-objects/something.dco`
    - then run `make depend` from the directory `<myPath>/DSKY_Daltons_<myModule>/DSKY_Daltons_<myModule>/` to fix the rest
    - *Note*: the `.dco`s that we discuseed about should already be added

6. When you manually create source files, or use the `new-module` script you need to add the sources before committing:
    - `dueca-project add-source something.cxx somethingelse.hxx ...`
    - only then do `dueca-commit`

7. Always run `dueca-commit` when you stop working; only then can the others update your changes

8. *Important:* Always keep a local backup of the `<myPath>` directory after you commit (ergo before you update), because shit will probably get f'ed at some point:
    - `cp -R <myPath> <some other directory outside of myPath>`




# How to run the full simulation

TBD. Probably just a `dueca-project checkout` of the `DSKY_Daltons_Project` to the `<myPath>` and you're good to go.
