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

if this_node_id == ecs_node:

    # mymods.append(dueca.Module(
    #     "some-module-i-created", "", sim_priority).param(
    #         set_timing = sim_timing,
    #         check_timing = (10000, 20000)))
    #
    mymods.append(dueca.Module(
        "world-view","",admin_priority.param(
            #module settings
            set_timing = display_timing,
            check_timing = (10000, 20000),
            claim_tread = False,
            restore_context = False,
            predict_dt = 0.0,
            predict_dt_max = 0.0,
            initial_camera = (0.0,0.0,-10.0,0.0,0.0,0.0),
            add_world_information_channel =
            ("ObjectMotion://world","HUDData://PHLAB"),
            #viewer backend, with its settings
                set_viewer     = dueca.OSGViewer().param(
                set_resourcepath   = "models",
                keep_cursor        = True,

                #first main windows
                add_windows         = "main window",
                window_size_pos     = (400, 300, 0, 0),
                window_x_screen     = "OpenSceneGraph Viewer",
                add_viewport        = "main viewport",
                viewport_pos_size   = (0, 0, 400, 300),
                viewport_window     = "main window",
                eye_offset          = (0, 0, 0, 0, 0, 0)).param(

                #second window for testing
                add_window          = "secondary window",
                window_size_pos     = (200, 150, 0, 300),
                add_viewport        = "secondary viewport",
                viewport_pos_size   = (0, 0, 200, 150),
                viewport_window     = "secondary window").param(

                #objects in the scene
                # 1) terrain
                add_object_class_data = ("static:world", "world",
                 "static", "lz.osg"),
                add_object_class_coordinates = (0.0, 0.0, 0.0),
                static_object         = ("static:world","world")).param(

                # 2) sun
                add_object_class_data = ("static:sunlight", "sunlight",
                "static-light"),
                add_object_class_coordinates = (0.08, 0.08, 0.08, 1,
                 0.08, 0.08, 0.08, 1, 0.0, 0.0, 0.0, 1, 0.4, 0.0, 1.0, 0, 0, 0, 0, 0.2, 0, 0),
                static_object = ("static:sunlight","sunlight")

                )


        )
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

