## -*-python-*-

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
        set_device = "/dev/input/js2",
        # Supply the path to a joystick device. Multiple devices may be added
        # the result will be one logical device with a number of axes and
        # buttons
        add_link = dueca.MultiStickValue().param(
            name = "uz",
            # identifying name for the stick value reader
            read_axis = 2,
            # read off a value from one of the joystick axes, and use it as a
            # continuous input. Give the axis number as argument
            calibration_polynomial = (0.0, 0.3),
            # provide a polynomial for calibration, coefficients in increasing
            # of power; a0 + a1 x + a2 x^2 etc.
        ),
        ).param( # repeated arguments
        add_link = dueca.MultiStickValue().param(
            name = "xmit",
            # identifying name for the stick value reader
            read_button = 1,
            # read off a button from the joystick, and use it as a truth value
            # Give the button number as argument
        ),
        ).param( # repeated arguments
        add_link = dueca.MultiStickValue().param(
            name = "flap_setting",
            # identifying name for the stick value reader
            create_counter = (5,0),
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
                read_button = 3),
            # provide a logical stick value that drives up counting
            set_counter_down = dueca.MultiStickValue().param(
                read_button = 5),
            # provide a logical stick value that drives down counting
            calibration_steps = (0,0, 1,1, 2,5, 3,10, 4,25, 5,30),
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
 
