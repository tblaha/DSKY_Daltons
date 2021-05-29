/* ------------------------------------------------------------------   */
/*      item            : controller.cxx
        made by         : tblaha
        from template   : DusimeModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Thu May 13 15:41:29 2021
        category        : body file
        description     :
        changes         : Thu May 13 15:41:29 2021 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          131224 RvP convert snap.data_size to
                                 snap.getDataSize()
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
*/


#define controller_cxx
// include the definition of the module class
#include "controller.hxx"

// include the debug writing header. Warning and error messages
// are on by default, debug and info can be selected by
// uncommenting the respective defines
//#define D_MOD
#define W_MOD
//#define I_MOD
#include <debug.h>

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dusime.h>

#include <cmath>

// class/module name
const char* const controller::classname = "controller";

// initial condition/trim table
const IncoTable* controller::getMyIncoTable()
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
const ParameterTable* controller::getMyParameterTable()
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
       
    { "Kp",
      new VarProbe<_ThisModule_,float >
      (&_ThisModule_::Kp),
      "controller proportional gain"},
    
    { "Kd",
      new VarProbe<_ThisModule_,float >
      (&_ThisModule_::Kd),
      "controller derivative gain"},
    
    { NULL, NULL, "please give a description of this module"} };

  return parameter_table;
}

// constructor
controller::controller(Entity* e, const char* part, const
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
  // Input rates
  myRollRate(0.0f),
  myPitchRate(0.0f),
  myYawRate(0.0f),
  myThrottle(0.0f),
  // Maximum angular rates are 10deg/2 (taken from http://www.stengel.mycpanel.princeton.edu/LM.pdf)
  maxp(0.175f),
  maxq(0.175f),
  maxr(0.175f),
  // Vehicle angular rates
  myp(0.0f),
  myq(0.0f),
  myr(0.0f),
  // Error
  ep(0.0f),
  eq(0.0f),
  er(0.0f),
  // Proportional control
  Kp(0.0f),
  Pp(0.0f),
  Pq(0.0f),
  Pr(0.0f),
  // Derivative control
  Kd(0.0f),
  Dp(0.0f),
  Dq(0.0f),
  Dr(0.0f),
  // Controller output
  pout(0.0f),
  qout(0.0f),
  rout(0.0f),
  // Angular accelerations
  P(0.0f),
  Q(0.0f),
  R(0.0f),
  // Moments of Inertia
  Ixx(20800.0f),
  Iyy(17400.0f),
  Izz(16500.0f),
  // Output Forces and Moments
  myMx(0.0f),
  myMy(0.0f),
  myMz(0.0f),
  myFx(0.0f),
  myFy(0.0f),
  myFz(0.0f),
  // Data storage
  prev_pout(0.0f),
  prev_qout(0.0f),
  prev_rout(0.0f),

  // initialize the data you need for the trim calculation

  // initialize the channel access tokens, check the documentation for the
  // various parameters. Some examples:
  // r_mytoken(getId(), NameSet(getEntity(), MyData::classname, part),
  //           MyData::classname, 0, Channel::Events, Channel::ReadAllData),
  // w_mytoken(getId(), NameSet(getEntity(), MyData::classname, part),
  //           MyData::classname, "label", Channel::Continuous),
  myControlPrimaryStreamReadToken(getId(), NameSet(getEntity(), "PrimaryControls", part)),
  myControlSecondaryStreamReadToken(getId(), NameSet(getEntity(), "SecondaryControls", part)),

  mySwitchPrimaryStreamReadToken(getId(), NameSet(getEntity(), "PrimarySwitches", part)),
  mySwitchSecondaryStreamReadToken(getId(), NameSet(getEntity(), "SecondarySwitches", part)),
  
  myVehicleStateStreamReadToken(getId(), NameSet(getEntity(), "vehicleState", part)),
  
  myThrusterForcesStreamWriteToken(getId(), NameSet(getEntity(), "thrusterForces", part)),
  

  // activity initialization
  myclock(),
  cb1(this, &_ThisModule_::doCalculation),
  do_calc(getId(), "takes stick inputs and current rates. Outputs thruster forces and engine thrust command", &cb1, ps)
{
  // do the actions you need for the simulation

  // connect the triggers for simulation
  do_calc.setTrigger( myControlPrimaryStreamReadToken );
  //do_calc.setTrigger( myclock );

  // connect the triggers for trim calculation. Leave this out if you
  // don not need input for trim calculation
  // trimCalculationCondition(/* fill in your trim triggering channels */);
}

bool controller::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
controller::~controller()
{
  //
}

// as an example, the setTimeSpec function
bool controller::setTimeSpec(const TimeSpec& ts)
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
bool controller::checkTiming(const std::vector<int>& i)
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
bool controller::isPrepared()
{
  bool res = true;

  // Example checking a token:
  // CHECK_TOKEN(w_somedata);
  CHECK_TOKEN(myControlPrimaryStreamReadToken);
  CHECK_TOKEN(myControlSecondaryStreamReadToken);
  CHECK_TOKEN(mySwitchPrimaryStreamReadToken);
  CHECK_TOKEN(mySwitchSecondaryStreamReadToken);
  CHECK_TOKEN(myVehicleStateStreamReadToken);
  CHECK_TOKEN(myThrusterForcesStreamWriteToken);

  // Example checking anything
  // CHECK_CONDITION(myfile.good());
  // CHECK_CONDITION2(sometest, "some test failed");

  // return result of checks
  return res;
}

// start the module
void controller::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void controller::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// fill a snapshot with state data. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void controller::fillSnapshot(const TimeSpec& ts,
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
void controller::loadSnapshot(const TimeSpec& t, const Snapshot& snap)
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
void controller::doCalculation(const TimeSpec& ts)
{
  // check the state we are supposed to be in
  switch (getAndCheckState(ts)) {
  case SimulationState::HoldCurrent: {

    // only repeat the output, do not change the model state

    break;
    }

  case SimulationState::Replay:
  case SimulationState::Advance: {


      //
      //READING FROM THE CHANNELS
      //

      //Reading from the primary control stream
      try {
          StreamReader<PrimaryControls> myControlPrimaryReader(myControlPrimaryStreamReadToken, ts);
          myRollRate = myControlPrimaryReader.data().ux*maxp;
          myPitchRate = myControlPrimaryReader.data().uy*maxq;
          myYawRate = myControlPrimaryReader.data().uz*maxr;
          myThrottle = myControlPrimaryReader.data().uc;
      }
      catch (Exception& e) {
          W_MOD(classname << "PrimaryControls read had an error @ " << ts << e);
      }

      //Reading from the secondary control stream and updating the zdot reference generator limits
      try {
          StreamReader<SecondaryControls> myControlSecondaryReader(myControlSecondaryStreamReadToken, ts);
          z_ref_setting = myControlSecondaryReader.data().flap_setting;
          update_max_zdot();
      }
      catch (Exception& e) {
          W_MOD(classname<< "Sec Ctrl This channel had an error @ " << ts );
      }

      //Reading from the primary switch event
      try {
          StreamReader<PrimarySwitches>mySwitchPrimaryReader(mySwitchPrimaryStreamReadToken, ts);
      }
      catch (Exception& e) {
          W_MOD(classname<< "This channel had an error");
      }

      //Reading from the secondary switch stream
      try {
          StreamReader<SecondarySwitches> mySwitchSecondaryReader(mySwitchSecondaryStreamReadToken, ts);
      }
      catch (Exception& e) {
          W_MOD(classname<< "This channel had an error");
      }

      //Reading from the vehicle
      try {
          StreamReader<vehicleState> myVehicleStateReader(myVehicleStateStreamReadToken, ts-100);
          myp = myVehicleStateReader.data().p;
          myq = myVehicleStateReader.data().q;
          myr = myVehicleStateReader.data().r;
          // Compute the angular accelerations
          P = (myp-myp_prev)/ts.getDtInSeconds();
          Q = (myq-myq_prev)/ts.getDtInSeconds();
          R = (myr-myr_prev)/ts.getDtInSeconds();
          myp_prev = myp;
          myq_prev = myq;
          myr_prev = myr;

          myuvw = Eigen::Vector3f(
            myVehicleStateReader.data().u, 
            myVehicleStateReader.data().v, 
            myVehicleStateReader.data().w);
          myquat = Eigen::Quaternionf(
            myVehicleStateReader.data().e0,
            myVehicleStateReader.data().ex,
            myVehicleStateReader.data().ey,
            myVehicleStateReader.data().ez);
          mass = myVehicleStateReader.data().mass;
      }
      catch (Exception& e) {
          W_MOD(classname<< "This channel had an error @ " << ts );
      }

      //
      //WRITING TO THE CHANNELS
      //
      
      // Feedback loop
      ep = myp - myRollRate;
      eq = myq - myPitchRate;
      er = myr - myYawRate;
      
      // Proportional part
      Pp = -Kp*ep;
      Pq = -Kp*eq;
      Pr = -Kp*er;
      
      // Derivative part
      Dp = Kd*P;
      Dq = Kd*Q;
      Dr = Kd*R;
      
      // Controller output
      myMx = clamp(Ixx * (Pp+Dp), -4*700.0f, 4*700.0f);
      myMy = clamp(Iyy * (Pq+Dq), -4*700.0f, 4*700.0f);
      myMz = clamp(Izz * (Pr+Dr), -4*750.0f, 4*750.0f);
      
      // Moments
      //myMx = Ixx*P;
      //myMy = Iyy*Q;
      //myMz = Izz*R;
      
      // Forces
      myFx = 0.0f;
      myFy = 0.0f;
      myFz = -myThrottle*1;

      // Store variables
      prev_pout = pout;
      prev_qout = qout;
      prev_rout = rout;

      /**
       * @brief Thrust controller
       * 
       */
      myInertialVel = myquat * myuvw; /**< note: not normal multiplication! Hamiltonion products according to v' = qvq-1 */
      nI = myquat * nB;
      mu = (std::abs(nI[2]) < 0.1) ? std::copysign(0.1f, nI[2]) : nI[2];

      float T_raw = mass / mu * ( zdot_P * (-gen_z_dot_ref(myThrottle) + myInertialVel[2]) + gM);

      myFz = -clamp( T_raw, T_max*t_limits[0], T_max*t_limits[1] );

      D_MOD("Current zdotref " << -gen_z_dot_ref(myThrottle));
      //D_MOD("Current zdot " << myInertialVel[2]);
      D_MOD("Current mu " << mu);
      //D_MOD("Current T_raw " << T_raw);
      D_MOD("Current myFz" << myFz);

      D_MOD("Current myMx" << myMx);
      D_MOD("Current myMy" << myMy);
      D_MOD("Current myMz" << myMz);
      //D_MOD("Current uc" << myThrottle);
      //D_MOD("Current max_zdot" << max_zdot);

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

  // write the output into the output channel, using the stream writer
  // y.data().var1 = something; ...
  StreamWriter<thrusterForces> myThrusterForcesWriter(myThrusterForcesStreamWriteToken,ts);
  //Writing the moments to the thrusterForces channel
  myThrusterForcesWriter.data().Mx = myMx;
  myThrusterForcesWriter.data().My = myMy;
  myThrusterForcesWriter.data().Mz = myMz;

  //Writing the forces to the thrusterForces channel
  myThrusterForcesWriter.data().Fx = myFx;
  myThrusterForcesWriter.data().Fy = myFy;
  myThrusterForcesWriter.data().Fz = myFz;

  if (snapshotNow()) {
    // keep a copy of the model state. Snapshot sending is done in the
    // sendSnapshot routine, later, and possibly at lower priority
    // e.g.
    // snapshot_state_variable1 = state_variable1; ...
    // (or maybe if your state is very large, there is a cleverer way ...)
  }
}

void controller::trimCalculation(const TimeSpec& ts, const TrimMode& mode)
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
static TypeCreator<controller> a(controller::getMyParameterTable());

