/* ------------------------------------------------------------------   */
/*      item            : F16HUD.cxx
        made by         : DASTUDostroosma
	    from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Wed May  4 13:30:35 2011
	    category        : body file
        description     : 
	    changes         : Wed May  4 13:30:35 2011 first version
	    template changes: 030401 RvP Added template creation comment
	                      060512 RvP Modified token checking code
        language        : C++
*/

static const char c_id[] =
"$Id: F16HUD.cxx,v 1.18 2021/06/12 20:05:08 tblaha Exp $";

#define F16HUD_cxx
// include the definition of the module class
#include "F16HUD.hxx"

// include the debug writing header, by default, write warning and 
// error messages
#define W_MOD
#define E_MOD
// #define D_MOD
#include <debug.h>
#include <Eigen/Dense>
#include <vector>

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dueca.h>
#include <Entity.hxx>
#include <MemberCall2Way.hxx>

// class/module name
const char* const F16HUD::classname = "f16-hud";

// Parameters to be inserted
const ParameterTable* F16HUD::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing", 
      new MemberCall<F16HUD,TimeSpec>
        (&F16HUD::setTimeSpec), set_timing_description },

    { "check-timing", 
      new MemberCall<F16HUD,vector<int> >
      (&F16HUD::checkTiming), check_timing_description },

#ifdef HAVE_OSG_WORLDVIEW 
    { "set-callback", 
      new MemberCall2Way<F16HUD, ScriptCreatable>
      (&F16HUD::setCallback), 
      "specify a WorldView callback to control" },
#endif

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string). 

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "stand-alone F16 HUD"} };

  return parameter_table;
}

// constructor
F16HUD::F16HUD(Entity* e, const char* part, const
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
  Module(e, classname, part),

  // initialize the data you need in your simulation
  window(), 
#ifdef HAVE_OSG_WORLDVIEW 
  osg_callback(NULL),
#endif
  
  // initialize the data you need for the trim calculation

  // initialize the channel access tokens
  // example
  // my_token(getId(), NameSet(getEntity(), "MyData", part)),
  myHUDbundleReadToken(getId(), NameSet(getEntity(), "HUDbundle", part)),

  // activity initialization
  myclock(), //TRIGGERING ON CLOCK (for now)
  cb1(this, &F16HUD::doCalculation),
  do_calc(getId(), "drawing F16 HUD", &cb1, PrioritySpec(0,0))
{
  // do the actions you need for the simulation
  if(ps.getPriority() != 0) {
    W_MOD(classname << ": GUI modules should run in admin-priority, corrected");
  }

  // connect the triggers for simulation
  //do_calc.setTrigger(myclock); //CLOCK
  do_calc.setTrigger(myHUDbundleReadToken); //CLOCK

}

bool F16HUD::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */

  // Open the GL window
  window.openWindow();

  return true;
}

// destructor
F16HUD::~F16HUD()
{
  //
}

// as an example, the setTimeSpec function
bool F16HUD::setTimeSpec(const TimeSpec& ts)
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
bool F16HUD::checkTiming(const vector<int>& i)
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

#ifdef HAVE_OSG_WORLDVIEW
bool F16HUD::setCallback(ScriptCreatable &obj, bool in)
{
  // check direction
  if (!in) return false;

  if (osg_callback) {
    E_CNF(classname << ": can only specify one callback");
    return false;
  }

  // try a dynamic cast
  osg_callback = dynamic_cast<F16HUDOSGCallback*> (&obj);
  if (osg_callback == NULL) {
    E_CNF(classname << ": must supply a F16HUDOSGCallback");
    return false;
  }

  return true;
}
#endif

// tell DUECA you are prepared
bool F16HUD::isPrepared()
{
  bool res = true;

  // Example checking a token:
  CHECK_TOKEN(myHUDbundleReadToken);

  // D_MOD("HUD TOKEN GOOD")

  // Example checking anything
  // CHECK_CONDITION(myfile.good());

  // return result of checks
  return res;
}

// start the module
void F16HUD::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void F16HUD::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You 
// should read the input channels here, and calculate and write the 
// appropriate output
void F16HUD::doCalculation(const TimeSpec& ts)
{
  // overload protection: return immediately if we're lagging
  if(do_calc.noScheduledBehind()) {
    return;
  }

  // access the input 
  // example:
  // try {
  //   StreamReader<MyInput> u(input_token, ts);
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

  try {
    StreamReader<HUDbundle> HUDbundleReader(myHUDbundleReadToken, ts);
/*     your_data.ias = myvehicleStateReader.data().u; //full steam ahead
    // myvehicleStateReader.data().u; //
    your_data.alt = -myvehicleStateReader.data().z; //negative z is upwards
    your_data.roll = myvehicleStateReader.data().phi;
    your_data.pitch = myvehicleStateReader.data().theta;
    your_data.heading = myvehicleStateReader.data().psi;
    your_data.loadfactor = myvehicleStateReader.data().thrust;
 */
    // set data on windowed HUD
    fillData(window.getHUD(), HUDbundleReader.data());
    
    window.redraw();

  #ifdef HAVE_OSG_WORLDVIEW
      // set data on OSG callback HUD
      if(osg_callback) {
        fillData(osg_callback->getHUD(), HUDbundleReader.data());
      }
  #endif
      
    } catch (Exception& e) {
      W_MOD(classname << " caught " << e << " @ " << ts);
    }
    
    // do the simulation or other calculations, one step

    // DUECA applications are data-driven. From the time a module is switched
    // on, it should produce data, so that modules "downstreams" are 
    // activated
    // access your output channel(s)
    // example
    // StreamWriter<MyOutput> y(output_token, ts);

    // write the output into the output channel, using the stream writer
    // y.data().var1 = something; ...
}

//void F16HUD::fillData(HUD& hud, const YourData& your_data)
void F16HUD::fillData(HUD& hud, const HUDbundle& myVS)
{
    Eigen::Quaternionf q {myVS.e0, myVS.ex, myVS.ey, myVS.ez};
    Eigen::Vector3f VB {myVS.u, myVS.v, myVS.w};

    Eigen::Vector3f VI = q * VB; // hamiltonion product #operatoroverloading
    Eigen::Vector2f VI_proj {VI(0), VI(1)};

    Eigen::Vector3f PosI_proj {myVS.x, myVS.y, 0}; // inertial position. projected on the ground 

    // horizontal speed and lateral speed
    hud.SetSpeedTapeSpeedIAS(VI_proj.norm());
    hud.SetSpeedTapeLateralSpeed(myVS.v);

    hud.SetAltTapeAltitude(-myVS.z);
    hud.SetAltTapeFloor(myVS.terrain_h);
    hud.SetAltTapeTermAlt(-myVS.terminal_z);
    hud.SetAltTapeRates(-VI(2), myVS.max_rate_sp);


    // attitude
    hud.SetPitchLadderPitchAngle(myVS.theta);
    hud.SetPitchLadderRollAngle(myVS.phi);
    hud.SetBankIndicatorRollAngle(myVS.phi);
    hud.SetHeadingTapeHeading(myVS.psi);

    // goodies
    hud.SetAircraftReferenceNz(myVS.thrust, myVS.sat_pos || myVS.sat_neg);
    hud.SetAircraftReferenceFuel(myVS.mass - 4500.0f);
    hud.SetAircraftReferenceSpeedVector(VI_proj);

    std::vector<Eigen::Vector3f> landing_sites{};
    std::vector<Eigen::Vector3f> other_groups{};
    landing_sites.push_back(Eigen::Vector3f(myVS.L1_x, myVS.L1_y, myVS.L1_z) - PosI_proj);
    landing_sites.push_back(Eigen::Vector3f(myVS.L2_x, myVS.L2_y, myVS.L2_z) - PosI_proj);
    landing_sites.push_back(Eigen::Vector3f(myVS.L3_x, myVS.L3_y, myVS.L3_z) - PosI_proj);
    landing_sites.push_back(Eigen::Vector3f(myVS.L4_x, myVS.L4_y, myVS.L4_z) - PosI_proj);
    landing_sites.push_back(Eigen::Vector3f(myVS.L5_x, myVS.L5_y, myVS.L5_z) - PosI_proj);
    landing_sites.push_back(Eigen::Vector3f(myVS.L6_x, myVS.L6_y, myVS.L6_z) - PosI_proj);
    landing_sites.push_back(Eigen::Vector3f(myVS.Lf_x, myVS.Lf_y, myVS.Lf_z) - PosI_proj);
    
    other_groups.push_back(Eigen::Vector3f(myVS.G1_x, myVS.G1_y, myVS.G1_z) - PosI_proj);
    other_groups.push_back(Eigen::Vector3f(myVS.G3_x, myVS.G3_y, myVS.G3_z) - PosI_proj);

    hud.SetAircraftReferenceYaw(myVS.psi);
    hud.SetAircraftReferenceLandingSites(landing_sites);
    hud.SetAircraftReferenceOtherGroups(other_groups);
    hud.SetAircraftReferenceRangeScale(myVS.hud_range);

    hud.SetAircraftReferenceMessage(myVS.message);
    hud.SetAircraftReferenceLeaderboard(myVS.Team1Score, myVS.Team2Score, myVS.Team3Score);
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<F16HUD> a(F16HUD::getMyParameterTable());

