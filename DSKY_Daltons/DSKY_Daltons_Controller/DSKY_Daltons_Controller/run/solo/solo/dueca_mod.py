## -*-python-*-
## this is an example dueca_mod.py file, for you to start out with and adapt
## according to your needs. Note that you need a dueca_mod.py file only for the
## node with number 0

## in general, it is a good idea to clearly document your set up
## this is an excellent place.

## node set-up
ecs_node = 0   # dutmms1, send order 3
#aux_node = 1   # dutmms3, send order 1
#pfd_node = 2   # dutmms5, send order 2
#cl_node = 3    # dutmms4, send order 0

## priority set-up
# normal nodes: 0 administration
#               1 hdf5 logging
#               2 simulation, unpackers
#               3 communication
#               4 ticker

# administration priority. Run the interface and logging here
admin_priority = dueca.PrioritySpec(0, 0)

# logging prio. Keep this free from time-critical other processes
log_priority = dueca.PrioritySpec(1, 0)

# priority of simulation, just above log
sim_priority = dueca.PrioritySpec(2, 0)

# nodes with a different priority scheme
# control loading node has 0, 1, 2 and 3 as above and furthermore
#               4 stick priority
#               5 ticker priority
# priority of the stick. Higher than prio of communication
# stick_priority = dueca.PrioritySpec(4, 0)

# timing set-up
# timing of the stick calculations. Assuming 100 usec ticks, this gives 2500 Hz
# stick_timing = dueca.TimeSpec(0, 4)

# this is normally 100, giving 100 Hz timing
sim_timing = dueca.TimeSpec(0, 100)

## for now, display on 50 Hz
display_timing = dueca.TimeSpec(0, 200)

## log a bit more economical, 25 Hz
log_timing = dueca.TimeSpec(0, 400)

## ---------------------------------------------------------------------
### the modules needed for dueca itself
if this_node_id == ecs_node:

    # create a list of modules:
    DUECA_mods = []
    DUECA_mods.append(dueca.Module("dusime", "", admin_priority))
    DUECA_mods.append(dueca.Module("dueca-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("activity-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("timing-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("log-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("channel-view", "", admin_priority))
    # uncomment for web-based graph, see DUECA documentation
    # DUECA_mods.append(dueca.Module("config-storage", "", admin_priority))

    # create the entity with that list
    DUECA_entity = dueca.Entity("dueca", DUECA_mods)

## ---------------------------------------------------------------------
# modules for your project (example)
mymods = []

use_gui_stick = True;
use_WorldView = False;

gamma = 0.25;

if this_node_id == ecs_node:
    #mymods.append(dueca.Module(
    #    "unit_tester", "", sim_priority).param(
    #        set_timing = sim_timing,
    #        check_timing = (10000, 20000)))

    mymods.append(dueca.Module(
        "controller", "", sim_priority).param(
            set_timing = sim_timing,
            check_timing = (10000, 20000),
            Prop_roll = 20800.0,
            Prop_pitch = 17400.0,
            Prop_yaw = 16500.0,
            Der_roll = 0.0,
            Der_pitch = 0.0,
            Der_yaw = 0.0,
            ))

    mymods.append(dueca.Module('multi-stick', "", sim_priority).param(
        set_timing = sim_timing,
        # Supply a time specification to define the update rate of the main activity
        check_timing = (10000, 20000),
        # Supply three integer parameters to specify a check on the timing of
        # the main activity: warning limit (in us), critical limit (in us), and
        # the number of loops to test before sending a report (optional, dflt=2000)
        use_primary = True,
        # Output data on the "PrimaryControls" channel
        use_secondary = True,
        # Output data on the "SecondaryControls" channel
        use_primary_switches = True,
        # Output data on the "PrimarySwitches" channel
        use_secondary_switches = True,
        # Output data on the "SecondarySwitches" channel
        set_device = "/dev/input/js1",
        # Supply the path to a joystick device. Multiple devices may be added
        # the result will be one logical device with a number of axes and
        # buttons
        add_link = dueca.MultiStickValue().param(
            name = "ux",
            read_axis = 3,
            # a0 + a1 x + a2 x^2 etc.
            # nice exponential scaling, see https://www.desmos.com/calculator/ekbtvpdhfy
            calibration_polynomial = (0.0, gamma, 0.0, 0.0, 0.0, 1-gamma),
        ),
        ).param( # repeated arguments
        add_link = dueca.MultiStickValue().param(
            name = "uy",
            read_axis = 2,
            calibration_polynomial = (0.0, gamma, 0.0, 0.0, 0.0, 1-gamma),
        ),
        ).param( # repeated arguments
        add_link = dueca.MultiStickValue().param(
            name = "uz",
            read_axis = 0,
            calibration_polynomial = (0.0, gamma, 0.0, 0.0, 0.0, 1-gamma),
        ),
        ).param( # repeated arguments
        add_link = dueca.MultiStickValue().param(
            name = "uc",
            read_axis = 1,
            # offset by 0.5 (such that stick down in 0 collective) and invert (for some reason that is needed)
            # NOTE: the calibration polynomial means we have fine control over 0.3-0.7 and only very rudimentary
            #       control over the 0-0.3 and 0.7-1 range! May need to reduce gamma for this!
            calibration_polynomial = (0.5, -0.5*gamma, 0.0, 0.0, 0.0, -0.5+0.5*gamma),
        ),
        ).param( # repeated arguments
        add_link = dueca.MultiStickValue().param(
            name = "AT_disconnect",
            # identifying name for the stick value reader
            read_button = 3,
            # read off a button from the joystick, and use it as a truth value
            # Give the button number as argument
        ),
        ).param( # repeated arguments
        add_link = dueca.MultiStickValue().param(
            name = "AP_disconnect",
            # identifying name for the stick value reader
            create_counter = (1,0),
            set_counter_up = dueca.MultiStickValue().param(
                read_button = 6),
            set_counter_down = dueca.MultiStickValue().param(
                read_button = 4),
            calibration_steps = (0,0, 1,1),
        ),
        ).param( # repeated arguments
        add_link = dueca.MultiStickValue().param(
            name = "flap_setting",
            # identifying name for the stick value reader
            create_counter = (4,0),
            # create a counting device. Supply the following integer values:
            #  * a (positive) integer number for the maximum count
            #  * a (nonnegative) integer number for center (init) value (optional)
            #  * a time value (in DUECA increments) for initial repeat action (opt)
            #  * a time value for subsequent repeat actions (opt)
            # You must also indicate how the counter is supposed to be driven,
            # by creating stick-value objects that are set up with
            # either read-button or read-criterion objects, attach them with
            # 'set-counter-up, 'set-counter-down and optionally 'center-counter.
            set_counter_up = dueca.MultiStickValue().param(
                read_button = 7),
            # provide a logical stick value that drives up counting
            set_counter_down = dueca.MultiStickValue().param(
                read_button = 5),
            # provide a logical stick value that drives down counting
            calibration_steps = (0,1, 1,2, 2,3, 3,4, 4,5),
            # provide 'choice' steps for calibration, give input, output pairs
            # u0, y0, u1, y1 etc. Finds u closest to input value, returns
            # corresponding output value
        ),
        
        # Supply a stick-value object that will convert one or more of the
        # stick's inputs to the control inputs. Remember to supply a name for
        # this stick value that matches one of the input names
        set_usb_priority = admin_priority,
        # Supply a (preferably dedicated) priority for the USB reading.
    ))

    # Uncomment and adapt for web-based graph, see DUECA documentation.
    # This also serves the static files for the default plotting application
    # over http.
    # adjust the priority if you need this for other, time-critical, data
    #
    # mymods.append(
    #     dueca.Module(
    #         "web-sockets-server", "", admin_priority).param(
    #             ('set-timing', sim_timing),
    #             ('check-timing', (5000, 9000)),
    #             ('port', 8001),
    #             ('info', ("endpoint", "MyData://PHLAB")),
    #             ('write-and-read', ("plotconfig",
    #                                 "ConfigFileRequest://dueca",
    #                                 "ConfigFileData://dueca")),
    #             ('http-port', 8000),
    #             ('document-root', '/usr/share/dplotter/dist')))

    # uncomment and adapt for HDF5 logging, see DUECA documentation
    # mymods.append(
    #     dueca.Module(
    #         "hdf5-logger", "", log_priority).param(
    #             ('set_timing', log_timing),
    #             ('chunksize', 3000),
    #             ('log_entry', ("MyData://PHLAB",
    #                            "MyData", "/data/mydata"))))
    #     )

# etc, each node can have modules in its mymods list

# then combine in an entity (one "copy" per node)
if mymods:
    myentity = dueca.Entity("PHLAB", mymods)

