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
#define TAU_ENGINE 0.5
// include the definition of the module class
#include "dynamics.hxx"

// include the debug writing header. Warning and error messages
// are on by default, debug and info can be selected by
// uncommenting the respective defines
#define D_MOD
#define W_MOD
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
  body(4500, 20800, 17400, 16500, 0, 0, 0, 1),
  workspace(14),
  gravity(0, 0, 1.62),
  thursterLocation(0, 0, 0),

  // initialize the data you need for the trim calculation

  // initialize the channel access tokens, check the documentation for the
  // various parameters. Some examples:
  // r_mytoken(getId(), NameSet(getEntity(), MyData::classname, part),
  //           MyData::classname, 0, Channel::Events, Channel::ReadAllData),
  // w_mytoken(getId(), NameSet(getEntity(), MyData::classname, part),
  //           MyData::classname, "label", Channel::Continuous),
  thrusterForcesReadToken(getId(), NameSet(getEntity(), "thrusterForces", part)),
  vehicleStateWriteToken(getId(), NameSet(getEntity(), "vehicleState", part)),
  ObjectMotionWriteToken(getId(), NameSet(getEntity(), "ObjectMotion", part)),

  // Referee channels
  refVehicleStateWriteToken(getId(), NameSet("VehicleStateStream://world"), "VehicleStateStream", "Team2", Channel::Continuous, Channel::OneOrMoreEntries),
  initialConditionsEventReadToken(getId(), NameSet("world", InitialConditionsEvent::classname, part)),
  respawnEventReadToken(getId(), NameSet("world", RespawnEvent::classname, part)),
  fuelRewardEventReadToken(getId(), NameSet("world", FuelRewardEvent::classname, part)),
  terrainHeightStreamReadToken(getId(), NameSet(getEntity(), "TerrainHeightStream", part)),

  // activity initialization
  // myclock(),
  cb1(this, &_ThisModule_::doCalculation),
  do_calc(getId(), "Integrates all forces. Also provides landing gear sim", &cb1, ps)
{
  // do the actions you need for the simulation
  this->thrusterForcesData.F << 0.0, 0.0, 0.0;
  this->thrusterForcesData.M << 0.0, 0.0, 0.0;

  this->vehicleStateData.xyz << 0.0, 0.0, 0.0;
  this->vehicleStateData.uvw << 0.0, 0.0, 0.0;
  this->vehicleStateData.pqr << 0.0, 0.0, 0.0;

  this->vehicleStateData.quat.w() = 1.0;
  this->vehicleStateData.quat.x() = 0.0;
  this->vehicleStateData.quat.y() = 0.0;
  this->vehicleStateData.quat.z() = 0.0;

  this->vehicleStateData.phi = 0.0;
  this->vehicleStateData.theta = 0.0;
  this->vehicleStateData.psi = 0.0;

  this->vehicleStateData.thrust = 0.0;
  this->vehicleStateData.dryMass = 4500.0;
  this->vehicleStateData.fuelMass = 0.0;
  this->vehicleStateData.groundHeight = 0.0;

  this->vehicleStateData.landed = true;
  //this->vehicleStateData.lgDelta = { }; see .hxx

  bodySetStateToCurrentData();

  // connect the triggers for simulation
  do_calc.setTrigger(thrusterForcesReadToken);

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

  CHECK_TOKEN(vehicleStateWriteToken);
  CHECK_TOKEN(refVehicleStateWriteToken);
  CHECK_TOKEN(respawnEventReadToken);
  CHECK_TOKEN(initialConditionsEventReadToken);
  CHECK_TOKEN(fuelRewardEventReadToken);
  //CHECK_TOKEN(terrainHeightStreamReadToken);
  
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
      readTerrainHeightStream(ts);

      if(this->vehicleStateData.landed != true){
        bodyStep(ts);
      }

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
  writeObjectMotionStream(ts);

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
          D_MOD(classname << ": Initial conditions event!" << ts);

          // Stream initializations
          this->vehicleStateData.xyz << initialConditionsEventReader.data().x, initialConditionsEventReader.data().y, initialConditionsEventReader.data().z;
          this->vehicleStateData.uvw << initialConditionsEventReader.data().u, initialConditionsEventReader.data().v, initialConditionsEventReader.data().w;
          this->vehicleStateData.pqr << initialConditionsEventReader.data().p, initialConditionsEventReader.data().q, initialConditionsEventReader.data().r;
          
          this->vehicleStateData.phi = initialConditionsEventReader.data().phi;
          this->vehicleStateData.theta = initialConditionsEventReader.data().theta;
          this->vehicleStateData.psi = initialConditionsEventReader.data().psi;

          this->vehicleStateData.fuelMass = initialConditionsEventReader.data().fuel_mass;

          // Self initializations
          bodySetStateToCurrentData();
          this->vehicleStateData.thrust = 0;
          this->body.setMass(this->vehicleStateData.dryMass + this->vehicleStateData.fuelMass);

          this->vehicleStateData.landed = false;
        }
    }
    catch (Exception& e) {
      W_MOD(classname << ": Error while reading InitialConditionsEvent!" << e);
    }
  }

  return;
}

void dynamics::readRespawnEvent(const TimeSpec& ts) {
  if(respawnEventReadToken.getNumWaitingEvents(ts)) {
    try {
      EventReader<RespawnEvent> respawnEventReader(respawnEventReadToken, ts);
      if(respawnEventReader.data().GroupNumber == GROUP_NUMBER) {
        D_MOD(classname << ": Respawn event!" << ts);

        // Stream initializations
        this->vehicleStateData.xyz << respawnEventReader.data().x, respawnEventReader.data().y, respawnEventReader.data().z;
        this->vehicleStateData.uvw << respawnEventReader.data().u, respawnEventReader.data().v, respawnEventReader.data().w;
        this->vehicleStateData.pqr << respawnEventReader.data().p, respawnEventReader.data().q, respawnEventReader.data().r;

        this->vehicleStateData.phi = respawnEventReader.data().phi;
        this->vehicleStateData.theta = respawnEventReader.data().theta;
        this->vehicleStateData.psi = respawnEventReader.data().psi;

        // Self initializations
        bodySetStateToCurrentData();
        this->vehicleStateData.thrust = 0;
        this->body.setMass(this->vehicleStateData.dryMass + this->vehicleStateData.fuelMass);

        this->vehicleStateData.landed = false;
      }
    }
    catch (Exception& e) {
      W_MOD(classname << ": Error while reading RespawnEvent!" << e);
    }
  }
  
  return;
}

void dynamics::readFuelRewardEvent(const TimeSpec& ts) {
  if(fuelRewardEventReadToken.getNumWaitingEvents(ts)) {
    try {
      EventReader<FuelRewardEvent> fuelRewardEventReader(fuelRewardEventReadToken, ts);
      if(fuelRewardEventReader.data().GroupNumber == GROUP_NUMBER) {
        D_MOD(classname << ": Fuel reward event!" << ts);
        this->vehicleStateData.fuelMass = this->vehicleStateData.fuelMass + fuelRewardEventReader.data().fuel_reward;
        this->body.setMass(this->vehicleStateData.dryMass + this->vehicleStateData.fuelMass);
      }
    }
    catch (Exception& e) {
      W_MOD(classname << ": Error while reading RespawnEvent!" << e);
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
    W_MOD(classname << ": Error while reading thrusterForces channel!" << e);
    this->thrusterForcesData.F << 0, 0, 0;
    this->thrusterForcesData.M << 0, 0, 0;
  }

  return;
}

void dynamics::readTerrainHeightStream(const TimeSpec& ts) {
  double terrainHeight = 0;

  // try {
  //   StreamReader<TerrainHeightStream> terrainHeightReader(terrainHeightStreamReadToken, ts);
  //   terrainHeight = terrainHeightReader.data().TerrainHeightGroup2;
  // } 
  // catch (Exception& e) {
  //   W_MOD(classname << ": Error while reading thrusterForces channel!" << e);
  // }

  this->vehicleStateData.groundHeight = this->vehicleStateData.xyz(2) - terrainHeight;

  

  Vector3d groundVec(0, 0, this->vehicleStateData.groundHeight);
  Vector3d landingLegs[4] = {Vector3d(0.577,   0.577, 0.577),
                             Vector3d(-0.577,  0.577, 0.577),
                             Vector3d(0.577,  -0.577, 0.577),
                             Vector3d(-0.577, -0.577, 0.577)};

  for(int i = 0; i < 4; i++) {
    landingLegs[i] = this->vehicleStateData.quat * landingLegs[i];
    double projected = (landingLegs[i].dot(groundVec) / groundVec.dot(groundVec) * groundVec)[2];

    if(this->vehicleStateData.groundHeight - projected > 0) {
      D_MOD(classname << ": Contact with leg: " << i);

      Eigen::Quaterniond state_q = Eigen::AngleAxisd(this->vehicleStateData.phi, Eigen::Vector3d::UnitX())
      * Eigen::AngleAxisd(this->vehicleStateData.theta, Eigen::Vector3d::UnitY())
      * Eigen::AngleAxisd(this->vehicleStateData.psi, Eigen::Vector3d::UnitZ());

      double state[] = {0, 0, 0,
                        this->vehicleStateData.xyz[0], this->vehicleStateData.xyz[1], terrainHeight,
                        0, 0, 0,
                        state_q.x(), state_q.y(), state_q.z(), state_q.w(), this->vehicleStateData.thrust};

      Map<VectorXd> stateMap(state, 14);
      setState(stateMap);

      this->vehicleStateData.landed = true;

      break;
    }
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

  vehicleStateWriter.data().e0 = this->vehicleStateData.quat.w();
  vehicleStateWriter.data().ex = this->vehicleStateData.quat.x();
  vehicleStateWriter.data().ey = this->vehicleStateData.quat.y();
  vehicleStateWriter.data().ez = this->vehicleStateData.quat.z();

  vehicleStateWriter.data().phi = this->vehicleStateData.phi;
  vehicleStateWriter.data().theta = this->vehicleStateData.theta;
  vehicleStateWriter.data().psi = this->vehicleStateData.psi;

  vehicleStateWriter.data().p = this->vehicleStateData.pqr(0);
  vehicleStateWriter.data().q = this->vehicleStateData.pqr(1);
  vehicleStateWriter.data().r = this->vehicleStateData.pqr(2);

  vehicleStateWriter.data().mass = this->vehicleStateData.dryMass + this->vehicleStateData.fuelMass;
  vehicleStateWriter.data().thrust = this->vehicleStateData.thrust;

  // see .hxx
  // vehicleStateWriter.data().lgDelta1 = this->.lgDelta(0);
  // vehicleStateWriter.data().lgDelta2 = this->vehicleStateData.lgDelta(1);
  // vehicleStateWriter.data().lgDelta3 = this->vehicleStateData.lgDelta(2);
  // vehicleStateWriter.data().lgDelta4 = this->vehicleStateData.lgDelta(2);
  
  return;
}

void dynamics::writeObjectMotionStream(const TimeSpec& ts) {
  StreamWriter<ObjectMotion> ObjectMotionWriter(ObjectMotionWriteToken, ts);

  ObjectMotionWriter.data().attitude_q[0] = this->vehicleStateData.quat.w();
  ObjectMotionWriter.data().attitude_q[1] = this->vehicleStateData.quat.x();
  ObjectMotionWriter.data().attitude_q[2] = this->vehicleStateData.quat.y();
  ObjectMotionWriter.data().attitude_q[3] = this->vehicleStateData.quat.z();

  ObjectMotionWriter.data().xyz[0] = this->vehicleStateData.xyz(0);
  ObjectMotionWriter.data().xyz[1] = this->vehicleStateData.xyz(1);
  ObjectMotionWriter.data().xyz[2] = this->vehicleStateData.xyz(2);

  ObjectMotionWriter.data().omega[0] = this->vehicleStateData.pqr(0);
  ObjectMotionWriter.data().omega[1] = this->vehicleStateData.pqr(1);
  ObjectMotionWriter.data().omega[2] = this->vehicleStateData.pqr(2);

  ObjectMotionWriter.data().uvw[0] = this->vehicleStateData.uvw(0);
  ObjectMotionWriter.data().uvw[1] = this->vehicleStateData.uvw(1);
  ObjectMotionWriter.data().uvw[2] = this->vehicleStateData.uvw(2);

  return;
}

void dynamics::writeRefVehicleStateStream(const TimeSpec& ts) {
  DataWriter<VehicleStateStream> refVehicleStateWriter(refVehicleStateWriteToken, ts);
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

  refVehicleStateWriter.data().phi = this->vehicleStateData.phi;
  refVehicleStateWriter.data().theta = this->vehicleStateData.theta;
  refVehicleStateWriter.data().psi = this->vehicleStateData.psi;
  
  refVehicleStateWriter.data().q1 = this->vehicleStateData.quat.x();
  refVehicleStateWriter.data().q2 = this->vehicleStateData.quat.y();
  refVehicleStateWriter.data().q3 = this->vehicleStateData.quat.z();
  refVehicleStateWriter.data().q4 = this->vehicleStateData.quat.w();

  refVehicleStateWriter.data().fuel_mass = this->vehicleStateData.fuelMass;
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

void dynamics::derivative(VectorE& xd, double dt) {
  body.zeroForces();
  body.addInertialGravity(gravity);

  // Main thruster
  if(this->vehicleStateData.fuelMass > 0) {
    xd[13] = (this->thrusterForcesData.F[2] - body.X()[13]) / TAU_ENGINE;
    //this->vehicleStateData.fuelMass = this->vehicleStateData.fuelMass - 1; //replace 1 with fuel use
  } else {
    xd[13] = (0.0 - body.X()[13]) / TAU_ENGINE;
  }

  Vector3d force(this->thrusterForcesData.F[0], this->thrusterForcesData.F[1], body.X()[13]);
  body.applyBodyForce(force, this->thursterLocation);
  body.applyBodyMoment(this->thrusterForcesData.M);

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

void dynamics::bodyStep(const TimeSpec& ts) {
  double dt = ts.getDtInSeconds();

  integrate_rungekutta(*this, workspace, dt);

  this->vehicleStateData.uvw << this->body.X()[0], this->body.X()[1], this->body.X()[2];
  this->vehicleStateData.xyz << this->body.X()[3], this->body.X()[4], this->body.X()[5];
  this->vehicleStateData.pqr << this->body.X()[6], this->body.X()[7], this->body.X()[8];

  this->vehicleStateData.quat.w() = this->body.X()[12];
  this->vehicleStateData.quat.x() = this->body.X()[9];
  this->vehicleStateData.quat.y() = this->body.X()[10];
  this->vehicleStateData.quat.z() = this->body.X()[11];

  this->vehicleStateData.thrust = this->body.X()[13];

  this->body.output();

  this->vehicleStateData.phi = this->body.phi();
  this->vehicleStateData.theta = this->body.theta();
  this->vehicleStateData.psi = this->body.psi();

  return;
}

void dynamics::bodySetStateToCurrentData() {
  Eigen::Quaterniond state_q = Eigen::AngleAxisd(this->vehicleStateData.phi, Eigen::Vector3d::UnitX())
    * Eigen::AngleAxisd(this->vehicleStateData.theta, Eigen::Vector3d::UnitY())
    * Eigen::AngleAxisd(this->vehicleStateData.psi, Eigen::Vector3d::UnitZ());

  double state[] = {this->vehicleStateData.uvw[0], this->vehicleStateData.uvw[1], this->vehicleStateData.uvw[2],
                    this->vehicleStateData.xyz[0], this->vehicleStateData.xyz[1], this->vehicleStateData.xyz[2],
                    this->vehicleStateData.pqr[0], this->vehicleStateData.pqr[1], this->vehicleStateData.pqr[2],
                    state_q.x(), state_q.y(), state_q.z(), state_q.w(), this->vehicleStateData.thrust};

  Map<VectorXd> stateMap(state, 14);
  setState(stateMap);

  return;
}

static TypeCreator<dynamics> a(dynamics::getMyParameterTable());
