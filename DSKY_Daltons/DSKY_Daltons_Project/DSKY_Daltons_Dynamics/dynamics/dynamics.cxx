/* ------------------------------------------------------------------   */
/*      item            : dynamics.cxx
        made by         : tblaha
        from template   : DusimeModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Thu May 13 16:10:42 2021
        category        : body file
        description     :
        changes         : Thu May 13 16:10:42 2021 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          131224 RvP convert snap.data_size to
                                 snap.getDataSize()
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
*/

#define dynamics_cxx
#define GROUP_NUMBER 2
// include the definition of the module class
#include "dynamics.hxx"

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
const char* const dynamics::classname = "dynamics";

// initial condition/trim table
const IncoTable* dynamics::getMyIncoTable()
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
const ParameterTable* dynamics::getMyParameterTable()
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
dynamics::dynamics(Entity* e, const char* part, const
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
  body(1, 1, 1, 1, 1, 1, 1, 0),
  workspace(13),
  gravity(0, 0, 1.62),

  // initialize the data you need for the trim calculation

  // initialize the channel access tokens, check the documentation for the
  // various parameters. Some examples:
  // r_mytoken(getId(), NameSet(getEntity(), MyData::classname, part),
  //           MyData::classname, 0, Channel::Events, Channel::ReadAllData),
  // w_mytoken(getId(), NameSet(getEntity(), MyData::classname, part),
  //           MyData::classname, "label", Channel::Continuous),
  thrusterForcesReadToken(getId(), NameSet(getEntity(), "thrusterForces", part)),
  vehicleStateWriteToken(getId(), NameSet(getEntity(), "vehicleState", part)),

  // Referee channels
  //refVehicleStateWriteToken(getId(), NameSet("VehicleStateStream://world"), "VehicleStateStream", "Team2", Channel::Continuous, Channel::OneOrMoreEntries),
  refVehicleStateWriteToken(getId(), NameSet(getEntity(), "VehicleStateStream", part)),
  initialConditionsEventReadToken(getId(), NameSet(getEntity(), "InitialConditionsEvent", part)),
  respawnEventReadToken(getId(), NameSet(getEntity(), "RespawnEvent", part)),
  fuelRewardEventReadToken(getId(), NameSet(getEntity(), "FuelRewardEvent", part)),

  // activity initialization
  // myclock(),
  cb1(this, &_ThisModule_::doCalculation),
  do_calc(getId(), "Integrates all forces. Also provides landing gear sim", &cb1, ps)
{
  // do the actions you need for the simulation
  this->thrusterForcesData.F << 0, 0, 0;
  this->thrusterForcesData.M << 0, 0, 0;

  this->vehicleStateData.xyz << 0, 0, 0;
  this->vehicleStateData.uvw << 0, 0, 0;
  this->vehicleStateData.pqr << 0, 0, 0;

  this->vehicleStateData.quat.w() = 1.0;
  this->vehicleStateData.quat.x() = 0;
  this->vehicleStateData.quat.y() = 0;
  this->vehicleStateData.quat.z() = 0;

  this->vehicleStateData.thrust = 0;
  this->vehicleStateData.mass = 100;
  //this->vehicleStateData.lgDelta = { }; see .hxx

  // connect the triggers for simulation
  do_calc.setTrigger(thrusterForcesReadToken || initialConditionsEventReadToken || respawnEventReadToken || fuelRewardEventReadToken);

  // connect the triggers for trim calculation. Leave this out if you
  // don not need input for trim calculation
  // trimCalculationCondition(/* fill in your trim triggering channels */);
}

bool dynamics::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
dynamics::~dynamics()
{
  //
}

// as an example, the setTimeSpec function
bool dynamics::setTimeSpec(const TimeSpec& ts)
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
bool dynamics::checkTiming(const std::vector<int>& i)
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
bool dynamics::isPrepared()
{
  bool res = true;

  // Example checking a token:
  // CHECK_TOKEN(w_somedata);
  CHECK_TOKEN(vehicleStateWriteToken);
  CHECK_TOKEN(refVehicleStateWriteToken);
  
  // Example checking anything
  // CHECK_CONDITION(myfile.good());
  // CHECK_CONDITION2(sometest, "some test failed");

  // return result of checks
  return res;
}

// start the module
void dynamics::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void dynamics::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// fill a snapshot with state data. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void dynamics::fillSnapshot(const TimeSpec& ts,
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
void dynamics::loadSnapshot(const TimeSpec& t, const Snapshot& snap)
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
void dynamics::doCalculation(const TimeSpec& ts)
{
  // check the state we are supposed to be in
  switch (getAndCheckState(ts)) {
    case SimulationState::HoldCurrent: {
      break;
    }
    case SimulationState::Replay:
    case SimulationState::Advance: {
      readInitialConditionsEvent(ts);
      readRespawnEvent(ts);
      readFuelRewardEvent(ts);
      readThrusterForcesStream(ts);

      bodyStep(ts);

      break;
    }
    default:
      // other states should never be entered for a SimulationModule,
      // HardwareModules on the other hand have more states. Throw an
      // exception if we get here,
      throw CannotHandleState(getId(),GlobalId(), "state unhandled");
  }

  writeVehicleStateStream(ts);
  writeRefVehicleStateStream(ts);

  if (snapshotNow()) {
  }
}

void dynamics::trimCalculation(const TimeSpec& ts, const TrimMode& mode)
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

void dynamics::readInitialConditionsEvent(const TimeSpec& ts) {
  if(initialConditionsEventReadToken.getNumWaitingEvents(ts)) {
    try {
      EventReader<InitialConditionsEvent> initialConditionsEventReader(initialConditionsEventReadToken, ts);
        if(initialConditionsEventReader.data().GroupNumber == GROUP_NUMBER) {
          // Stream initializations
          this->vehicleStateData.xyz << initialConditionsEventReader.data().x, initialConditionsEventReader.data().y, initialConditionsEventReader.data().z;
          this->vehicleStateData.uvw << initialConditionsEventReader.data().u, initialConditionsEventReader.data().v, initialConditionsEventReader.data().w;
          this->vehicleStateData.pqr << initialConditionsEventReader.data().p, initialConditionsEventReader.data().q, initialConditionsEventReader.data().r;
          //this->vehicleStateData.??? << initialConditionsEventReader.data().phi, initialConditionsEventReader.data().theta, initialConditionsEventReader.data().psi;
          this->vehicleStateData.mass = initialConditionsEventReader.data().fuel_mass;

          // Self initializations
          this->vehicleStateData.thrust = 0;
        }
    }
    catch (Exception& e) {
      W_MOD(classname << ": Error while reading InitialConditionsEvent!");
    }
  }

  return;
}

void dynamics::readRespawnEvent(const TimeSpec& ts) {
  if(respawnEventReadToken.getNumWaitingEvents(ts)) {
    try {
      EventReader<RespawnEvent> respawnEventReader(respawnEventReadToken, ts);
      if(respawnEventReader.data().GroupNumber == GROUP_NUMBER) {
        // Stream initializations
        this->vehicleStateData.xyz << respawnEventReader.data().x, respawnEventReader.data().y, respawnEventReader.data().z;
        this->vehicleStateData.uvw << respawnEventReader.data().u, respawnEventReader.data().v, respawnEventReader.data().w;
        this->vehicleStateData.pqr << respawnEventReader.data().p, respawnEventReader.data().q, respawnEventReader.data().r;
        //this->vehicleStateData.??? << respawnEventReader.data().phi, respawnEventReader.data().theta, respawnEventReader.data().psi;

        // Self initializations
        this->vehicleStateData.thrust = 0;
      }
    }
    catch (Exception& e) {
      W_MOD(classname << ": Error while reading RespawnEvent!");
    }
  }
  
  return;
}

void dynamics::readFuelRewardEvent(const TimeSpec& ts) {
  if(fuelRewardEventReadToken.getNumWaitingEvents(ts)) {
    try {
      EventReader<FuelRewardEvent> fuelRewardEventReader(fuelRewardEventReadToken, ts);
      if(fuelRewardEventReader.data().GroupNumber == GROUP_NUMBER) {
        this->vehicleStateData.mass = this->vehicleStateData.mass + fuelRewardEventReader.data().fuel_reward;
      }
    }
    catch (Exception& e) {
      W_MOD(classname << ": Error while reading RespawnEvent!");
    }
  }

  return;
}

void dynamics::readThrusterForcesStream(const TimeSpec& ts) {
  try {
    StreamReader<thrusterForces> thrusterForcesReader(thrusterForcesReadToken, ts);
    this->thrusterForcesData.F << thrusterForcesReader.data().Fx, thrusterForcesReader.data().Fy, thrusterForcesReader.data().Fz;
    this->thrusterForcesData.M << thrusterForcesReader.data().Mx, thrusterForcesReader.data().My, thrusterForcesReader.data().Mz;
  } 
  catch (Exception& e) {
    W_MOD(classname << ": Error while reading thrusterForces channel @ " << ts);
    this->thrusterForcesData.F << 0, 0, 0;
    this->thrusterForcesData.M << 0, 0, 0;
  }

  return;
}

void dynamics::writeVehicleStateStream(const TimeSpec& ts) {
  StreamWriter<vehicleState> vehicleStateWriter(vehicleStateWriteToken, ts);
  vehicleStateWriter.data().x = this->vehicleStateData.xyz(0);
  vehicleStateWriter.data().y = this->vehicleStateData.xyz(1);
  vehicleStateWriter.data().z = this->vehicleStateData.xyz(2);

  vehicleStateWriter.data().u = this->vehicleStateData.uvw(0);
  vehicleStateWriter.data().v = this->vehicleStateData.uvw(1);
  vehicleStateWriter.data().w = this->vehicleStateData.uvw(2);

  // see .hxx
  // vehicleStateWriter.data().e0 = this->vehicleStateData.quat(0);
  // vehicleStateWriter.data().ey = this->vehicleStateData.quat(1);
  // vehicleStateWriter.data().ez = this->vehicleStateData.quat(2);
  // vehicleStateWriter.data().ez = this->vehicleStateData.quat(2);

  vehicleStateWriter.data().p = this->vehicleStateData.pqr(0);
  vehicleStateWriter.data().q = this->vehicleStateData.pqr(1);
  vehicleStateWriter.data().r = this->vehicleStateData.pqr(2);

  vehicleStateWriter.data().mass = this->vehicleStateData.mass;
  vehicleStateWriter.data().thrust = this->vehicleStateData.thrust;

  // see .hxx
  // vehicleStateWriter.data().lgDelta1 = this->vehicleStateData.lgDelta(0);
  // vehicleStateWriter.data().lgDelta2 = this->vehicleStateData.lgDelta(1);
  // vehicleStateWriter.data().lgDelta3 = this->vehicleStateData.lgDelta(2);
  // vehicleStateWriter.data().lgDelta4 = this->vehicleStateData.lgDelta(2);
  
  return;
}

void dynamics::writeRefVehicleStateStream(const TimeSpec& ts) {
  StreamWriter<VehicleStateStream> refVehicleStateWriter(refVehicleStateWriteToken, ts);
  refVehicleStateWriter.data().GroupNumber = GROUP_NUMBER;

  refVehicleStateWriter.data().x = this->vehicleStateData.xyz(0);
  refVehicleStateWriter.data().y = this->vehicleStateData.xyz(1);
  refVehicleStateWriter.data().z = this->vehicleStateData.xyz(2);

  refVehicleStateWriter.data().u = this->vehicleStateData.uvw(0);
  refVehicleStateWriter.data().v = this->vehicleStateData.uvw(1);
  refVehicleStateWriter.data().w = this->vehicleStateData.uvw(2);

  refVehicleStateWriter.data().p = this->vehicleStateData.pqr(0);
  refVehicleStateWriter.data().q = this->vehicleStateData.pqr(1);
  refVehicleStateWriter.data().r = this->vehicleStateData.pqr(2);

  // not implemented
  // refVehicleStateWriter.data().phi = 
  // refVehicleStateWriter.data().theta = 
  // refVehicleStateWriter.data().psi = 

  // see .hxx
  // refVehicleStateWriter.data().q1 = this->vehicleStateData.quat(0);
  // refVehicleStateWriter.data().q2 = this->vehicleStateData.quat(1);
  // refVehicleStateWriter.data().q3 = this->vehicleStateData.quat(2);
  // refVehicleStateWriter.data().q4 = this->vehicleStateData.quat(2);

  refVehicleStateWriter.data().fuel_mass = this->vehicleStateData.mass;
  refVehicleStateWriter.data().F_thruster = this->vehicleStateData.thrust;

  // not implemented
  // refVehicleStateWriter.data().T_x = 
  // refVehicleStateWriter.data().T_y = 
  // refVehicleStateWriter.data().T_z = 

  // refVehicleStateWriter.data().x_LG = 
  // refVehicleStateWriter.data().y_LG = 
  // refVehicleStateWriter.data().z_LG = 
  // refVehicleStateWriter.data().F_LG = 
  // refVehivleStateWriter.data().Landing_gear_bool =
   
  return;
}

void dynamics::derivative(VectorE& xd, double dt)
{
    body.zeroForces();
    body.addInertialGravity(gravity);

    body.derivative(xd);

    return;
}

const Vector& dynamics::X() const {
  return this->body.X();
}

void dynamics::setState(const VectorE& newx) {
  this->body.setState(newx);

  return;
}

void dynamics::bodyStep(const TimeSpec& ts)
{
    double dt = ts.getDtInSeconds();

    integrate_rungekutta(*this, workspace, dt);

    this->vehicleStateData.uvw << this->body.X()[0], this->body.X()[1], this->body.X()[2];
    this->vehicleStateData.xyz << this->body.X()[3], this->body.X()[4], this->body.X()[5];
    this->vehicleStateData.pqr << this->body.X()[6], this->body.X()[7], this->body.X()[8];

    this->vehicleStateData.quat.w() = this->body.X()[12];
    this->vehicleStateData.quat.x() = this->body.X()[9];
    this->vehicleStateData.quat.y() = this->body.X()[10];
    this->vehicleStateData.quat.z() = this->body.X()[11];

    return;
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<dynamics> a(dynamics::getMyParameterTable());
