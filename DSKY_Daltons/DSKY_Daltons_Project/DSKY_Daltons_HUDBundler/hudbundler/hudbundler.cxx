/* ------------------------------------------------------------------   */
/*      item            : hudbundler.cxx
        made by         : tblaha
        from template   : DusimeModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Sun Jun  6 00:25:14 2021
        category        : body file
        description     :
        changes         : Sun Jun  6 00:25:14 2021 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          131224 RvP convert snap.data_size to
                                 snap.getDataSize()
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
*/


#define hudbundler_cxx
#define GROUP_NUMBER 2
// include the definition of the module class
#include "hudbundler.hxx"

// include the debug writing header. Warning and error messages
// are on by default, debug and info can be selected by
// uncommenting the respective defines
//#define D_MOD
//#define I_MOD
#include <debug.h>

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dusime.h>

// class/module name
const char* const hudbundler::classname = "hudbundler";

// initial condition/trim table
const IncoTable* hudbundler::getMyIncoTable()
{
  static IncoTable inco_table[] = {
    // enter pairs of IncoVariable and VarProbe pointers (i.e.
    // objects made with new), in this table.
    // For example
//    {(new IncoVariable("example", 0.0, 1.0, 0.01))
//     ->forMode(FlightPath, Constraint)
//     ->forMode(Speed, Control),
//     new VarProbe<_ThisModule_,double>
//       (REF_MEMBER(&_ThisModule_::i_example))}

    // always close off with:
    { NULL, NULL} };

  return inco_table;
}

// parameters to be inserted
const ParameterTable* hudbundler::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_,TimeSpec>
        (&_ThisModule_::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_,std::vector<int> >
      (&_ThisModule_::checkTiming), check_timing_description },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "please give a description of this module"} };

  return parameter_table;
}

// constructor
hudbundler::hudbundler(Entity* e, const char* part, const
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

  // initialize the data you need for the trim calculation

  // initialize the channel access tokens, check the documentation for the
  // various parameters. Some examples:
  // r_mytoken(getId(), NameSet(getEntity(), MyData::classname, part),
  //           MyData::classname, 0, Channel::Events, Channel::ReadAllData),
  // w_mytoken(getId(), NameSet(getEntity(), MyData::classname, part),
  //           MyData::classname, "label", Channel::Continuous),
  vehicleStateToken(getId(), NameSet(getEntity(), "vehicleState", part)),
  flightControlModesToken(getId(), NameSet(getEntity(), "flightControlModes", part)),
  LeaderboardStreamToken(getId(), NameSet(getEntity(), "LeaderboardStream", part)),

  fuelRewardEventReadToken(getId(), NameSet(getEntity(), "fuelRewardEvent", part)),
  AnnouncementEventToken(getId(), NameSet(getEntity(), "AnnouncementEvent", part)),

  HUDbundleToken(getId(), NameSet(getEntity(), "HUDbundle", part)),

  // activity initialization
  // myclock(),
  cb1(this, &_ThisModule_::doCalculation),
  do_calc(getId(), "Take info and bundle for the HUD, since HUD can only take one channel", &cb1, ps)
{
  // do the actions you need for the simulation

  // connect the triggers for simulation
  do_calc.setTrigger( vehicleStateToken );

  // connect the triggers for trim calculation. Leave this out if you
  // don not need input for trim calculation
  //trimCalculationCondition(/* fill in your trim triggering channels */);
}

bool hudbundler::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
hudbundler::~hudbundler()
{
  //
}

// as an example, the setTimeSpec function
bool hudbundler::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);
  // or do this with the clock if you have it (don't do both!)
  // myclock.changePeriodAndOffset(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// and the checkTiming function
bool hudbundler::checkTiming(const std::vector<int>& i)
{
  if (i.size() == 3) {
    new TimingCheck(do_calc, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_calc, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

// tell DUECA you are prepared
bool hudbundler::isPrepared()
{
  bool res = true;

  // Example checking a token:
  CHECK_TOKEN(vehicleStateToken);
  CHECK_TOKEN(flightControlModesToken);
  //CHECK_TOKEN(LeaderboardStreamToken);
  //CHECK_TOKEN(fuelRewardEventReadToken);
  //CHECK_TOKEN(AnnouncementEventToken);
  CHECK_TOKEN(HUDbundleToken);

  // Example checking anything
  // CHECK_CONDITION(myfile.good());
  // CHECK_CONDITION2(sometest, "some test failed");

  // return result of checks
  return res;
}

// start the module
void hudbundler::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void hudbundler::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// fill a snapshot with state data. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void hudbundler::fillSnapshot(const TimeSpec& ts,
                              Snapshot& snap, bool from_trim)
{
  // The most efficient way of filling a snapshot is with an AmorphStore
  // object.
  AmorphStore s(snap.accessData(), snap.getDataSize());

  if (from_trim) {
    // use packData(s, trim_state_variable1); ... to pack your state into
    // the snapshot
  }
  else {
    // this is a snapshot from the running simulation. Dusime takes care
    // that no other snapshot is taken at the same time, so you can safely
    // pack the data you copied into (or left into) the snapshot state
    // variables in here
    // use packData(s, snapshot_state_variable1); ...
  }
}

// reload from a snapshot. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void hudbundler::loadSnapshot(const TimeSpec& t, const Snapshot& snap)
{
  // access the data in the snapshot with an AmorphReStore object
  AmorphReStore s(snap.data, snap.getDataSize());

  // use unPackData(s, real_state_variable1 ); ... to unpack the data
  // from the snapshot.
  // You can safely do this, while snapshot loading is going on the
  // simulation is in HoldCurrent or the activity is stopped.
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void hudbundler::doCalculation(const TimeSpec& ts)
{
  StreamWriter<HUDbundle> HUDWriter(HUDbundleToken, ts);

  // check the state we are supposed to be in
  switch (getAndCheckState(ts)) {
  case SimulationState::HoldCurrent: {
    // only repeat the output, do not change the model state

    break;
    }

  case SimulationState::Replay:
  case SimulationState::Advance: {
    // access the input
    // example:
    // try {
    //   DataReader<MyInput> u(input_token, ts);
    //   throttle = u.data().throttle;
    //   de = u.data().de; ....
    // }
    // catch(Exception& e) {
    //   // strange, there is no input. Should I try to continue or not?
    // }
    /* The above piece of code shows a block in which you try to catch
       error conditions (exceptions) to handle the case in which the input
       data is lost. This is not always necessary, if you normally do not
       foresee such a condition, and you don t mind being stopped when
       it happens, forget about the try/catch blocks. */

    // do the simulation calculations, one step
    // controller state
    D_MOD("test1");
    StreamReaderLatest<flightControlModes> FCReader(flightControlModesToken);
    HUDWriter.data().max_rate_sp        = FCReader.data().max_rate_sp;
    HUDWriter.data().vertical_rate_sp   = FCReader.data().vertical_rate_sp;
    HUDWriter.data().phi_d              = FCReader.data().phi_d;
    HUDWriter.data().theta_d            = FCReader.data().theta_d;
    HUDWriter.data().psi_d              = FCReader.data().psi_d;
    HUDWriter.data().sat_pos            = FCReader.data().sat_pos;
    HUDWriter.data().sat_neg            = FCReader.data().sat_neg;

    // vehicleState
    D_MOD("test2");
    StreamReaderLatest<vehicleState> StateReader(vehicleStateToken);
    HUDWriter.data().x        = StateReader.data().x;
    HUDWriter.data().y        = StateReader.data().y;
    HUDWriter.data().z        = StateReader.data().z;
    HUDWriter.data().u        = StateReader.data().u;
    HUDWriter.data().v        = StateReader.data().v;
    HUDWriter.data().w        = StateReader.data().w;
    HUDWriter.data().e0       = StateReader.data().e0;
    HUDWriter.data().ex       = StateReader.data().ex;
    HUDWriter.data().ey       = StateReader.data().ey;
    HUDWriter.data().ez       = StateReader.data().ez;
    HUDWriter.data().phi      = StateReader.data().phi;
    HUDWriter.data().theta    = StateReader.data().theta;
    HUDWriter.data().psi      = StateReader.data().psi;
    HUDWriter.data().p        = StateReader.data().p;
    HUDWriter.data().q        = StateReader.data().q;
    HUDWriter.data().r        = StateReader.data().r;
    HUDWriter.data().mass     = StateReader.data().mass;
    HUDWriter.data().thrust   = StateReader.data().thrust;
    HUDWriter.data().lgDelta1 = StateReader.data().lgDelta1;
    HUDWriter.data().lgDelta2 = StateReader.data().lgDelta2;
    HUDWriter.data().lgDelta3 = StateReader.data().lgDelta3;
    HUDWriter.data().lgDelta4 = StateReader.data().lgDelta4;

    // Leaderboard
    //StreamReaderLatest<LeaderboardStream> LeaderboardReader(LeaderboardStreamToken);
    //HUDWriter.data().Team1Score        = LeaderboardReader.data().Team1Score;
    //HUDWriter.data().Team2Score        = LeaderboardReader.data().Team2Score;
    //HUDWriter.data().Team3Score        = LeaderboardReader.data().Team3Score;

    break;
    }
  default:
    // other states should never be entered for a SimulationModule,
    // HardwareModules on the other hand have more states. Throw an
    // exception if we get here,
    throw CannotHandleState(getId(),GlobalId(), "state unhandled");
  }



  // DUECA applications are data-driven. From the time a module is switched
  // on, it should produce data, so that modules "downstreams" are
  // activated
  // access your output channel(s)
  // example
  // DataWriter<MyOutput> y(output_token, ts);


  // fuel reward
  D_MOD("test5");
  if(fuelRewardEventReadToken.getNumWaitingEvents(ts)) {
    try {
      EventReader<FuelRewardEvent> fuelRewardEventReader(fuelRewardEventReadToken, ts);
      if(fuelRewardEventReader.data().GroupNumber == GROUP_NUMBER) {
        _fuel_reward = fuelRewardEventReader.data().fuel_reward;
      }
    }
    catch (Exception& e) {
      W_MOD(classname << ": Error while reading Fuel Reward Event!");
    }
  }
  D_MOD("test6");
  HUDWriter.data().fuel_reward = _fuel_reward;

  D_MOD("test7");

  // Announcement
  if(AnnouncementEventToken.getNumWaitingEvents(ts)) {
    try {
      EventReader<AnnouncementEvent> AnnouncementReader(AnnouncementEventToken, ts);
      _message = AnnouncementReader.data().message;
    }
    catch (Exception& e) {
      W_MOD(classname << ": Error while reading Announcement Events!");
    }
  }
  D_MOD("test8");
  HUDWriter.data().message = _message;
  D_MOD("test9");


  // write the output into the output channel, using the stream writer
  // y.data().var1 = something; ... w

  if (snapshotNow()) {
    // keep a copy of the model state. Snapshot sending is done in the
    // sendSnapshot routine, later, and possibly at lower priority
    // e.g.
    // snapshot_state_variable1 = state_variable1; ...
    // (or maybe if your state is very large, there is a cleverer way ...)
  }
}

void hudbundler::trimCalculation(const TimeSpec& ts, const TrimMode& mode)
{
  // read the event equivalent of the input data
  // example
  // DataReader<MyData> u(i_input_token, ts);

  // using the input, and the data put into your trim variables,
  // calculate the derivative of the state. DO NOT use the state
  // vector of the normal simulation here, because it might be that
  // this is done while the simulation runs!
  // Some elements in this state derivative are needed as target, copy
  // these out again into trim variables (see you TrimTable

  // trim calculation
  switch(mode) {
  case FlightPath: {
    // one type of trim calculation, find a power setting and attitude
    // belonging to a flight path angle and speed
  }
  break;

  case Speed: {
    // find a flightpath belonging to a speed and power setting (also
    // nice for gliders)
  }
  break;

  case Ground: {
    // find an altitude/attitude belonging to standing still on the
    // ground, power/speed 0
  }
  break;

  default:
    W_MOD(getId() << " cannot calculate inco mode " << mode);
  break;
  }

  // This works just like a normal calculation, only you provide the
  // steady state value (if your system is stable anyhow). So, if you
  // have other modules normally depending on your output, you should
  // also produce the equivalent output here.
  // DataWriter<MyOutput> y(output_token, ts);

  // write the output into the output channel, using the DataWriter

  // now return. The real results from the trim calculation, as you
  // specified them in the TrimTable, will now be collected and sent
  // off for processing.
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<hudbundler> a(hudbundler::getMyParameterTable());

