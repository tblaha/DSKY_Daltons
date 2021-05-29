/* ------------------------------------------------------------------   */
/*      item            : MultiStick.cxx
        made by         : repa
	from template   : DusimeModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Fri Dec  5 12:52:42 2003
	category        : body file 
        description     : 
	changes         : Fri Dec  5 12:52:42 2003 first version
	                  040319 RvP, corrected value objects that
	                  were not initialized as NULL pointers
	template changes: 030401 RvP Added template creation comment
        language        : C++
*/

#define MultiStick_cxx
// include the definition of the module class
#include "MultiStick.hxx"

// include the debug writing header, by default, write warning and 
// error messages
#define E_CNF
#define W_CNF
#define W_MOD
#define E_MOD
#include <debug.h>

// include additional files needed for your calculation here
#include <Ticker.hxx>
#ifndef USE_SDL
#include <linux/joystick.h>
#endif
#include <fcntl.h>
#include <Entity.hxx>
#include <errno.h>
// some versions of boost react to this nice define for real-time workshop
// thanks -> (c) The MathWorks
#undef RT
#include <boost/lexical_cast.hpp>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dusime.h>
#include <MemberCall2Way.hxx>
#include <AsyncList.hxx>

// class/module name
const char* const MultiStick::classname = "multi-stick";

// initial condition/trim table
const IncoTable* MultiStick::getMyIncoTable()
{
  static IncoTable inco_table[] = {
    // enter pairs of IncoVariable and VarProbe pointers (i.e. 
    // objects made with new), in this table.
    // For example
//    {(new IncoVariable("example", 0.0, 1.0, 0.01))
//     ->forMode(FlightPath, Constraint)
//     ->forMode(Speed, Control),
//     new VarProbe<MultiStick,double>
//       (REF_MEMBER(&MultiStick::i_example))}
    
    // always close off with:
    { NULL, NULL} };

  return inco_table;
}

// parameters to be inserted
const ParameterTable* MultiStick::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing", 
      new MemberCall<MultiStick,TimeSpec>
        (&MultiStick::setTimeSpec), set_timing_description },

    { "check-timing", 
      new MemberCall<MultiStick,vector<int> >
      (&MultiStick::checkTiming), check_timing_description },

    /* Selecton of interfaces to present */
    { "use-primary", new MemberCall<MultiStick,bool>
      (&MultiStick::usePrimary), 
      "Output data on the \"PrimaryControls\" channel" },
    { "use-secondary", new MemberCall<MultiStick,bool>
      (&MultiStick::useSecondary), 
      "Output data on the \"SecondaryControls\" channel"},
    { "use-primary-switches", new MemberCall<MultiStick,bool>
      (&MultiStick::usePrimarySwitches), 
      "Output data on the \"PrimarySwitches\" channel"},
    { "use-secondary-switches", new MemberCall<MultiStick,bool>
      (&MultiStick::useSecondarySwitches), 
      "Output data on the \"SecondarySwitches\" channel"},

    /* Add a new joystick device to be read. */
    { "set-device", new MemberCall<MultiStick,vstring>
      (&MultiStick::setDevice), 
      "Supply the path to a joystick device. Multiple devices may be added\n"
      "the result will be one logical device with a number of axes and\n"
      "buttons" },

    /* Add a new conversion/reader for the joystick. */
    { "add-link", new MemberCall2Way<MultiStick,ScriptCreatable>
      (&MultiStick::addLink),
      "Supply a stick-value object that will convert one or more of the\n"
      "stick's inputs to the control inputs. Remember to supply a name for\n"
      "this stick value that matches one of the input names" },

    /* Set a priority for the (possibly blocking) USB activity. */
    { "set-usb-priority", new MemberCall<MultiStick, PrioritySpec>
      (&MultiStick::setUSBPrio),
      "Supply a (preferably dedicated) priority for the USB reading."},

    // always close off with:
    { NULL, NULL, 
      "A joystick reading class, can provide the default aircraft inputs,\n"
      "PrimaryControls, SecondaryControls, PrimarySwitches and\n"
      "SecondarySwitches"} 
  };

  return parameter_table;
}

// constructor
MultiStick::MultiStick(Entity* e, const char* part, const
		       PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class. 
     You always pass the pointer to the entity, give the classname and the 
     part arguments.
     If you give a NULL pointer instead of the inco table, you will not be 
     called for trim condition calculations, which is normal if you for 
     example implement logging or a display.
     If you give 0 for the snapshot state, you will not be called to 
     fill a snapshot, or to restore your state from a snapshot. Only 
     applicable if you have no state. */
  SimulationModule(e, classname, part, getMyIncoTable(), 0),

  // initialize the data you need in your simulation
#ifdef USE_SDL
  need_sdl_init(false),
#endif
  all_devices_open(true),
  ux(NULL),
  uy(NULL),
  uz(NULL),
  uc(NULL),
  fbrake_left(NULL),
  fbrake_right(NULL),
  throttle_left(NULL),
  throttle_right(NULL),
  speedbrake(NULL),
  flap_setting(NULL),
  
  xmit(NULL),
  trim_up(NULL),
  trim_down(NULL),
  AP_disconnect(NULL),
  AT_disconnect(NULL),
  ignition_left(NULL),
  ignition_right(NULL),
  gear_up(NULL),
  gear_neutral(NULL),
  gear_down(NULL),
  test_button(NULL),

  pc_list(20, "PrimaryControls list"),
  sc_list(20, "SecondaryControls list"),
  ps_list(20, "PrimarySwitches list"),
  ss_list(20, "SecondarySwitches list"),

  myPC(), mySC(), myPS(), mySS(),

  // initialize the data you need for the trim calculation
  // no state, no trim calculation

  // initialize the channel access tokens
  controls(NULL),
  secondary(NULL),
  primary_switches(NULL),
  secondary_switches(NULL),

  // activity initialization
  cb1(this, &MultiStick::doCalculation),
  cb2(this, &MultiStick::doCommunication),
  do_calc(getId(), "joystick reading", &cb1, ps),
  do_comm(getId(), "sending joystick output", &cb2, ps)
{
  // do the actions you need for the simulation

  // connect the triggers for simulation
  do_calc.setTrigger(*Ticker::single());
  do_comm.setTrigger(*Ticker::single());

#ifdef USE_SDL
  JoystickDevice jsnew = {NULL, 0, 0, 0};
#else
  JoystickDevice jsnew = {-1, 0, 0};
#endif
  device.push_back(jsnew);
}

bool MultiStick::complete()
{
  /* Reserve the room for the joystick buttons and axes. */
  button.resize(device.back().offset_button, false);
  axis.resize(device.back().offset_axis, 0.0);
  return true;
}

// destructor
MultiStick::~MultiStick()
{
  delete ux;
  delete uy;
  delete fbrake_left;
  delete fbrake_right;
  delete throttle_left;
  delete throttle_right;
  delete speedbrake;
  delete flap_setting;

  delete xmit;
  delete trim_up;
  delete trim_down;
  delete AP_disconnect;
  delete AT_disconnect;
  delete ignition_left;
  delete ignition_right;
  delete gear_up;
  delete gear_neutral;
  delete gear_down;

  delete controls;
  delete secondary;
  delete primary_switches;
  delete secondary_switches;
}

// as an example, the setTimeSpec function
bool MultiStick::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);
  do_comm.setTimeSpec(ts);

  // return true if everything is acceptable
  return true;
}

// and the checkTiming function
bool MultiStick::checkTiming(const vector<int>& i)
{
  if (i.size() == 3) {
    new TimingCheck(do_calc, i[0], i[1], i[2]);
    new TimingCheck(do_comm, i[0], i[1], i[2]);
 }
  else if (i.size() == 2) {
    new TimingCheck(do_calc, i[0], i[1]);
    new TimingCheck(do_comm, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

bool MultiStick::usePrimary(const bool& b)
{
  if(b && controls == NULL) {
    controls = new StreamChannelWriteToken<PrimaryControls>
      (getId(), NameSet(getEntity(), "PrimaryControls", getPart()), 101);
  }
  else if(!b && controls != NULL) {
    delete controls; controls = NULL;
  }

  return true;
}

bool MultiStick::useSecondary(const bool& b)
{
  if(b && secondary == NULL) {
    secondary = new StreamChannelWriteToken<SecondaryControls>
      (getId(), NameSet(getEntity(), "SecondaryControls", getPart()), 101);
  }
  else if (!b && secondary != NULL) {
    delete secondary; secondary = NULL;
  }
  return true;
}

bool MultiStick::usePrimarySwitches(const bool& b)
{
  if(b && primary_switches == NULL) {
    primary_switches = new StreamChannelWriteToken<PrimarySwitches>
      (getId(), NameSet(getEntity(), "PrimarySwitches", getPart()), 101);
  }
  else if(!b && primary_switches != NULL) {
    delete primary_switches; primary_switches = NULL;
  }

  return true;
}

bool MultiStick::useSecondarySwitches(const bool& b)
{
  if(b && secondary_switches == NULL) {
    secondary_switches = new StreamChannelWriteToken<SecondarySwitches>
      (getId(), NameSet(getEntity(), "SecondarySwitches", getPart()), 101);
  }
  else if (!b && secondary_switches != NULL) {
    delete secondary_switches; secondary_switches = NULL;
  }
  return true;
}


#ifdef USE_SDL
bool MultiStick::setDevice(const vstring& t)
{
  if (need_sdl_init) {
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    need_sdl_init = false;
  }

  // number of joysticks
  unsigned njoy = SDL_NumJoysticks();
  unsigned reqjoy;
  
  // the argument is now a joystick number, rather than a path
  try {
    reqjoy = boost::lexical_cast<unsigned>(t.c_str());
  }
  catch (const boost::bad_lexical_cast& ) {
    E_MOD("cannot extract a joystick device number from \"" << t << '"');
    return false;
  }
  if (reqjoy >= njoy) {
    E_MOD("Have " << njoy << " joysticks, cannot open no " << reqjoy);
    return false;
  }
  SDL_Joystick* joystick = SDL_JoystickOpen(reqjoy);
  SDL_JoystickEventState(SDL_ENABLE);
  which_joystick[SDL_JoystickInstanceID(joystick)] = device.size() - 1;
  
  int numaxes = device.back().offset_hat = SDL_JoystickNumAxes(joystick);
  numaxes += SDL_JoystickNumHats(joystick);
  int numbuttons =  SDL_JoystickNumButtons(joystick);
  std::string name = SDL_JoystickName(joystick);

  cout << "Opened joystick " << name << " as number " << t.c_str()
       << " ax=" << numaxes << " bt=" << numbuttons << endl;

  // save the device
  device.back().joystick = joystick;

  // prepare place for a new joystick device
  JoystickDevice newjs = 
    { NULL, 0, device.back().offset_axis + numaxes, 
      device.back().offset_button + numbuttons};
  device.push_back(newjs);
  
  return true;
}
  
#else
bool MultiStick::setDevice(const vstring& t)
{
  int fd;

  // try to open the device
  if ((fd = open(t.c_str(), O_RDONLY | O_NONBLOCK)) < 0) {
    perror("Opening joystick device");
    all_devices_open = false; return false;
  }
  
  // read the construction data. 
  unsigned char axes = 0;
  unsigned char buttons = 0;
  const int NAME_LENGTH = 128;
  unsigned char name[NAME_LENGTH] = "unknown";

  if (ioctl(fd, JSIOCGAXES, &axes) != 0) {
    perror("Cannot find no of axes");
    all_devices_open = false; return false;
  }

  // check no of buttons
  if (ioctl(fd, JSIOCGBUTTONS, &buttons) != 0) {
    perror("Cannot find no of buttons");
    all_devices_open = false; return false;
  }
  
  // check name
  if (ioctl(fd, JSIOCGNAME(NAME_LENGTH), &name) == 0) {
    perror("Cannot find name");
    all_devices_open = false; return false;
  }
  cout << "Opened joystick " << name << " on device " << t.c_str()
       << " ax=" << int(axes) << " bt=" << int(buttons) << endl;

  // put data into the array. There is already a place reserved, with
  // the proper offsets. 
  device.back().fd = fd;
  
  // prepare a new joystick device
  JoystickDevice newjs = 
    { -1, device.back().offset_axis + axes, 
      device.back().offset_button + buttons};
  
  // and put it at the end of the list
  device.push_back(newjs);

  // means success.
  return true;
}
#endif

// linkmap
struct LinkMap 
{
  const char* name;
  MultiStickValue* &mv;
};

bool MultiStick::addLink(ScriptCreatable& sv, bool in)
{
  // check direction
  if (!in) return false;

  // try a dynamic cast
  MultiStickValue* value = dynamic_cast<MultiStickValue*> (&sv);
  if (value == NULL) {
    E_CNF("must supply a stick-value");
    return false;
  }

  // now make sure Scheme does not clean this helper from right
  // under our noses:
  getMyEntity()->scheme_id.addReferred(value->scheme_id.getSCM());

  // Seems OK. In any case, add it to our known_values list, so it
  // gets updated
  known_values.push_back(value);

  // link map, describes how values are linked to specific control
  // outputs
  static LinkMap link_map[] = {
    {"ux", ux},
    {"uy", uy},
    {"uz", uz}, 
    {"uc", uc}, 
    {"fbrake_left", fbrake_left},
    {"fbrake_right", fbrake_right},
    {"throttle_left", throttle_left},
    {"throttle_right", throttle_right},
    {"speedbrake", speedbrake},
    {"flap_setting", flap_setting},
    {"xmit", xmit},
    {"trim_up", trim_up},
    {"trim_down", trim_down},
    {"AP_disconnect", AP_disconnect},
    {"AT_disconnect", AT_disconnect},
    {"ignition_left", ignition_left},
    {"ignition_right", ignition_right},
    {"gear_up", gear_up},
    {"gear_neutral", gear_neutral},
    {"gear_down", gear_down},
    {"test_button", test_button}
  };


  for (int ii = sizeof(link_map)/sizeof(LinkMap); ii--; ) {
    if (!strcmp(link_map[ii].name, value->getName().c_str())) {
      if (link_map[ii].mv) {
	E_CNF(getId() << "output " << link_map[ii].name <<
	      " already connected");
	return false;
      }
      link_map[ii].mv = value;
      return true;
    }
  }
  
  W_CNF(getId() << "no such output " << value->getName());
  return true;
}

bool MultiStick::setUSBPrio(const PrioritySpec& ps)
{
  if(ps.getPriority() >= 0) {
    do_calc.changePriority(ps);
    return true;
  } else {
    E_MOD(classname << " cannot set USB priority " << ps.getPriority());
    return false;
  }
}

// tell DUECA you are prepared
bool MultiStick::isPrepared()
{
  // do whatever additional calculations you need to prepare the model. 

  // Check that all conditions for running are good. 
  // It helps to indicate what the problems are
  bool res = 
    (controls == NULL || controls->isValid()) &&
    (secondary == NULL || secondary->isValid()) &&
    (primary_switches == NULL || primary_switches->isValid()) &&
    (secondary_switches == NULL || secondary_switches->isValid()) &&
    all_devices_open;

  if (!res) {
    W_MOD(getId() << '/' << classname << " not prepared, flags=" <<
	  (controls == NULL || controls->isValid()) <<
	  (secondary == NULL || secondary->isValid()) <<
	  (primary_switches == NULL || primary_switches->isValid()) <<
	  (secondary_switches == NULL || secondary_switches->isValid()) <<
	  all_devices_open);
  }

  // return result of check
  return res;
}

// start the module
void MultiStick::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
  do_comm.switchOn(time);
}

// stop the module
void MultiStick::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
  do_comm.switchOff(time);
}


// this routine contains the main simulation process of your module. You 
// should read the input channels here, and calculate and write the 
// appropriate output
void MultiStick::doCalculation(const TimeSpec& ts)
{
#ifdef CHECK_FIRSTTIME
  {
    static bool first = true;
    if (first) {
      W_MOD(getId() << '/' << getClass() << " enter " << ts);
      first = false;
    }
  }
#endif
  
#ifdef USE_SDL
  SDL_Event event;
  bool keepup = true;
  
  while (SDL_PollEvent(&event) && keepup) {
    switch (event.type) {
    case SDL_QUIT:
      W_MOD("got SDL quit event");
      keepup = false;
      break;
    case SDL_JOYAXISMOTION: {
      unsigned idx = which_joystick[event.jaxis.which];
      axis[device[idx].offset_axis+event.jaxis.axis] =
	event.jaxis.value / 32767.0f;
      break;
    }
    case SDL_JOYHATMOTION: {
      unsigned idx = which_joystick[event.jhat.which];
      int ax = 0, ay = 0;
      switch (event.jhat.value) {
      case SDL_HAT_LEFTUP:
	ax = 1; ay=1;
	break;
      case SDL_HAT_UP:
	ay = 1;
	break;
      case SDL_HAT_RIGHTUP:
	ax = -1; ay=1;
	break;
      case SDL_HAT_LEFT:
	ax = 1;
	break;
      case SDL_HAT_CENTERED:
	break;
      case SDL_HAT_RIGHT:
	ay = 1;
	break;
      case SDL_HAT_LEFTDOWN:
	ay = -1; ax = 1;
	break;
      case SDL_HAT_DOWN:
	ay = -1;
	break;
      case SDL_HAT_RIGHTDOWN:
	ax = 1; ay = -1;
	break;
      }
      axis[device[idx].offset_axis + device[idx].offset_hat + event.jhat.hat] =
	float(ax);
      axis[device[idx].offset_axis + device[idx].offset_hat + event.jhat.hat + 1] =
	float(ay);
    }
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP: {
      unsigned idx = which_joystick[event.jbutton.which];
      button[device[idx].offset_button + event.jbutton.button] =
	event.jbutton.state == SDL_PRESSED;
      break;
    }
    }
  }
#else
  // variable for reading joystick input
  struct js_event js;
  const double JS_RANGE=double(0x8000);

  // always read and update the device array
  for (unsigned int ii = 0; ii < device.size()-1; ii++) {
    
    while (read(device[ii].fd, &js, sizeof(struct js_event)) > 0) {
 
      // read out joystick event
      switch(js.type & ~JS_EVENT_INIT) {
      case JS_EVENT_BUTTON:
	button[js.number + device[ii].offset_button] = (js.value != 0);
	break;
      
      case JS_EVENT_AXIS:
	axis[js.number + device[ii].offset_axis] = 
	  double(js.value)/double(JS_RANGE);
	break;
      }
    }
    if (errno != EAGAIN) {
      perror("reading joystick");
    }
  } 
#endif
  
  // check the state we are supposed to be in
  switch (getCurrentState()) {
  case SimulationState::HoldCurrent: {

    // same type of calculations
    int elapsed = ts.getValidityEnd() - ts.getValidityStart();
    for (std::list<MultiStickValue*>::iterator ii = known_values.begin(); 
	 ii != known_values.end(); ii++) {
      (*ii)->update(axis, button, elapsed);
    }

    break;
    }

  case SimulationState::Replay:
  case SimulationState::Advance: {
    int elapsed = ts.getValidityEnd() - ts.getValidityStart();

    // do the simulation calculations, one step
    for (std::list<MultiStickValue*>::iterator ii = known_values.begin(); 
	 ii != known_values.end(); ii++) {
      (*ii)->update(axis, button, elapsed);
    }

    break;
    }
  default:
    // other states should never be entered for a SimulationModule, 
    // HardwareModules on the other hand have more states. Throw an
    // exception if we get here, 
    throw CannotHandleState(getId(),GlobalId(), "state unhandled");
  }

  // put the data in AsyncLists for the comm thread to send out
#define READDOUBLE(A) c. A = (A == NULL) ? 0.0 : A ->getDouble()
#define READBOOL(A) c. A = (A == NULL) ? false : A ->getBool()

  if (controls != NULL) {
    PrimaryControls c;
    READDOUBLE(ux);
    READDOUBLE(uy);
    READDOUBLE(uz);
    READDOUBLE(uc);
    c.test = (test_button == NULL) ? 0x0 :
      (test_button->getBool() ? 0x03 : 0x01);
    READDOUBLE(fbrake_left);
    READDOUBLE(fbrake_right);
    
    // provide sensible numbers for data we can't measure
    c.Sx =0.0; c.Sy =0.0; c.Sz =0.0;
    c.dSx =0.0; c.dSy =0.0; c.dSz =0.0;
    c.Mx =0.0; c.My =0.0; c.Mz =0.0;

    pc_list.push_back(c);
  }

  if (secondary != NULL) {
#ifdef CHECK_FIRSTTIME
    static bool first = true;
    if (first) {
      W_MOD(getId() << '/' << getClass() << " write " << ts);
      first = false;
    }
#endif
    SecondaryControls c;
    READDOUBLE(throttle_left);
    READDOUBLE(throttle_right);
    READDOUBLE(flap_setting);
    READDOUBLE(speedbrake);

    sc_list.push_back(c);
  }
  
  if (primary_switches != NULL) {
    PrimarySwitches c;
    READBOOL(xmit);
    READBOOL(trim_up);
    READBOOL(trim_down);
    READBOOL(AP_disconnect);

    ps_list.push_back(c);
  }
  
  if (secondary_switches != NULL) {
    SecondarySwitches c;
    READBOOL(AT_disconnect);
    READBOOL(ignition_left);
    READBOOL(ignition_right);
    READBOOL(gear_up);
    READBOOL(gear_neutral);
    READBOOL(gear_down);

    ss_list.push_back(c);
  }
} 

// The activity that communicates with DUECA.
// Note on timing: since acquisition and transmission now run 
//   asynchronously (if in separate threads/prios), the data can
//   sometimes be 1 time step old. This is the price to pay for 
//   robustness. Putting them in the same prio causes acquisition
//   to run first (not guaranteed?) and the tightest timing. Using 
//   USB and expecting tight, reliable timing is a bit foolish anyway ;-)
void MultiStick::doCommunication(const TimeSpec& ts)
{
  // check the state we are supposed to be in
  getAndCheckState(ts);

  unsigned int list_size = 0;

  // get latest data from device
  while(pc_list.notEmpty()) {
    myPC = pc_list.front();
    pc_list.pop();
    list_size++;
  }

  // check if we're still running normally: either tightly (1) or more
  // loosely (sometimes alternating 0 and 2)
  if(list_size > 2) 
    W_MOD(classname << " encountered severe desynchonisation @ " << ts <<
	  ", expecting no more than 2, got " << list_size);

  while(sc_list.notEmpty()) {
    mySC = sc_list.front();
    sc_list.pop();
  }
  while(ps_list.notEmpty()) {
    myPS = ps_list.front();
    ps_list.pop();
  }
  while(ss_list.notEmpty()) {
    mySS = ss_list.front();
    ss_list.pop();
  }
  
  // write to channels
  if (controls != NULL) {
    StreamWriter<PrimaryControls> pc(*controls, ts);
    pc.data() = myPC;
  }

  if (secondary != NULL) {
    StreamWriter<SecondaryControls> sc(*secondary, ts);
    sc.data() = mySC;
  }
  
  if (primary_switches != NULL) {
    StreamWriter<PrimarySwitches> ps(*primary_switches, ts);
    ps.data() = myPS;
  }
  
  if (secondary_switches != NULL) {
    StreamWriter<SecondarySwitches> ss(*secondary_switches, ts);
    ss.data() = mySS;
  }
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<MultiStick> a(MultiStick::getMyParameterTable());

