/* ------------------------------------------------------------------   */
/*      item            : TestGUI.cxx
        made by         : ostroosma
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Fri May 21 12:50:28 2021
        category        : body file
        description     :
        changes         : Fri May 21 12:50:28 2021 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
*/


#define TestGUI_cxx

// include the definition of the module class
#include "TestGUI.hxx"

// include the debug writing header, by default, write warning and
// error messages
#define D_MOD
#define I_MOD
#define W_MOD
#define E_MOD
#include <debug.h>

// include additional files needed for your calculation here
#include <cmath>

// these macros are needed to parse the module's directory when
// looking for the .glade file (also see Makefile)
// eliminates the need for links.script entry
#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dueca.h>

// class/module name
const char* const TestGUI::classname = "test-gui";

// GTK callbacks
static GladeCallbackTable cbtable[] = 
{
  // links the button, with signal pressed, to callback function 
  { "ae_send", "clicked", 
    gtk_callback(&TestGUI::onSendAEClicked),gpointer(0)},
  { "fr_send", "clicked", 
    gtk_callback(&TestGUI::onSendFRClicked),gpointer(0)},
  { "state_send", "clicked", 
    gtk_callback(&TestGUI::onSendStateClicked),gpointer(0)},


  // close off in familiar style
  { NULL, NULL, NULL, NULL }
};


// Parameters to be inserted
const ParameterTable* TestGUI::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_,TimeSpec>
        (&_ThisModule_::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_,std::vector<int> >
      (&_ThisModule_::checkTiming), check_timing_description },

    { "read-streams",
      new MemberCall<_ThisModule_,bool >
      (&_ThisModule_::readStreams), "also read the referee streams" },
    
    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "module for testing referee events"} };

  return parameter_table;
}

// constructor
TestGUI::TestGUI(Entity* e, const char* part, const
                   PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  winglade(),

  // widgets
  ae_message(NULL),
  fr_group(NULL), fr_fuel(NULL),
  ic_radio(NULL),
  state_x(NULL), state_y(NULL), state_z(NULL),
  state_u(NULL), state_v(NULL), state_w(NULL),
  state_phi(NULL), state_theta(NULL), state_psi(NULL),
  state_p(NULL), state_q(NULL), state_r(NULL),
  state_fuel(NULL), state_group(NULL),
  lbt1(NULL), lbt2(NULL), lbt3(NULL),
  
  // initialize the channel access tokens, check the documentation for the
  // various parameters. Some examples:
  // r_mytoken(getId(), NameSet(getEntity(), MyData::classname, part),
  //           MyData::classname, 0, Channel::Events),
  // w_mytoken(getId(), NameSet(getEntity(), MyData2::classname, part),
  //           MyData2::classname, "label", Channel::Continuous),
  ae_token(getId(), NameSet("world", AnnouncementEvent::classname, part),
	   AnnouncementEvent::classname, "fromTestGUI", Channel::Events,
	   Channel::OneOrMoreEntries),
  fr_token(getId(), NameSet("world", FuelRewardEvent::classname, part),
	   FuelRewardEvent::classname, "fromTestGUI", Channel::Events,
	   Channel::OneOrMoreEntries),
  ic_token(getId(),
	   NameSet("world", InitialConditionsEvent::classname, part),
	   InitialConditionsEvent::classname, "fromTestGUI", Channel::Events,
	   Channel::OneOrMoreEntries),
  re_token(getId(), NameSet("world", RespawnEvent::classname, part),
	   RespawnEvent::classname, "fromTestGUI", Channel::Events,
	   Channel::OneOrMoreEntries),
  lb_token(getId(), NameSet("world", LeaderboardStream::classname, part),
	   LeaderboardStream::classname, "fromTestGUI", Channel::Continuous),

  lbs_token(NULL), vss_token(NULL),

  // create a clock, if you need time based triggering
  // instead of triggering on the incoming channels
  myclock(),

  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisModule_::doCalculation),
  // the module's main activity
  do_calc(getId(), "GUI for testing", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(myclock);
  
}

bool TestGUI::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  bool res = false;

  // find .glade file
  string gladefile = string(STRINGIZE_VALUE_OF(GUIDIR)) + "ref_test.glade";

  res = winglade.readGladeFile(gladefile.c_str(), "window1", 
			       reinterpret_cast<gpointer>(this), 
			       cbtable);
  // find widgets
  ae_message = winglade["ae_message"];
  fr_group = winglade["fr_groupnr"];
  fr_fuel = winglade["fr_fuel"];
  ic_radio = winglade["ic_radio"];
  state_x = winglade["state_x"];
  state_y = winglade["state_y"];
  state_z = winglade["state_z"];
  state_u = winglade["state_u"];
  state_v = winglade["state_v"];
  state_w = winglade["state_w"];
  state_phi = winglade["state_phi"];
  state_theta = winglade["state_theta"];
  state_psi = winglade["state_psi"];
  state_p = winglade["state_p"];
  state_q = winglade["state_q"];
  state_r = winglade["state_r"];
  state_fuel = winglade["state_fuel"];
  state_group = winglade["state_groupnr"];
  lbt1 = winglade["lb_team1"];
  lbt2 = winglade["lb_team2"];
  lbt3 = winglade["lb_team3"];
 
  // show the GUI
  winglade.show();
  
  return res;
}

// destructor
TestGUI::~TestGUI()
{
  //
}

// as an example, the setTimeSpec function
bool TestGUI::setTimeSpec(const TimeSpec& ts)
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

// the checkTiming function installs a check on the activity/activities
// of the module
bool TestGUI::checkTiming(const std::vector<int>& i)
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

// Also read the streams, if available
bool TestGUI::readStreams(const bool &b)
{
  if(b) {
    lbs_token = new ChannelReadToken
      (getId(), NameSet(getEntity(), LeaderboardStream::classname, getPart()),
       LeaderboardStream::classname, 0, Channel::Continuous,
       Channel::ZeroOrMoreEntries);
    vss_token = new ChannelReadToken
      (getId(), NameSet(getEntity(), VehicleStateStream::classname, getPart()),
       VehicleStateStream::classname, 0, Channel::Continuous,
       Channel::ZeroOrMoreEntries);
  } else {
    delete lbs_token;
    delete vss_token;
    lbs_token = NULL; vss_token = NULL;
  }

  return true;
}

// tell DUECA you are prepared
bool TestGUI::isPrepared()
{
  bool res = true;

  CHECK_TOKEN(ae_token);
  CHECK_TOKEN(fr_token);
  CHECK_TOKEN(ic_token);
  CHECK_TOKEN(re_token);
  CHECK_TOKEN(lb_token);
  if(lbs_token) CHECK_TOKEN(*lbs_token);
  if(vss_token) CHECK_TOKEN(*vss_token);
 
  CHECK_CONDITION(ae_message);
  CHECK_CONDITION(fr_group);
  CHECK_CONDITION(fr_fuel); 
  CHECK_CONDITION(ic_radio);
  CHECK_CONDITION(state_x);
  CHECK_CONDITION(state_y);
  CHECK_CONDITION(state_z);
  CHECK_CONDITION(state_u);
  CHECK_CONDITION(state_v);
  CHECK_CONDITION(state_w);
  CHECK_CONDITION(state_phi);
  CHECK_CONDITION(state_theta);
  CHECK_CONDITION(state_psi);
  CHECK_CONDITION(state_p);
  CHECK_CONDITION(state_q);
  CHECK_CONDITION(state_r);
  CHECK_CONDITION(state_fuel);
  CHECK_CONDITION(state_group);
  CHECK_CONDITION(lbt1);
  CHECK_CONDITION(lbt2);
  CHECK_CONDITION(lbt3);

  // return result of checks
  return res;
}

// start the module
void TestGUI::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void TestGUI::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void TestGUI::doCalculation(const TimeSpec& ts)
{
  // access the input
  // example:
  // try {
  //   DataReader<MyData> u(r_mytoken, ts);
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

  // do the simulation or other calculations, one step

  // DUECA applications are data-driven. From the time a module is switched
  // on, it should produce data, so that modules "downstream" are
  // activated
  // access your output channel(s)
  // example
  // DataWriter<MyData2> y(w_mytoken, ts);
  DataWriter<LeaderboardStream> lb(lb_token, ts);
  
  // write the output into the output channel, using the data writer
  // y.data().var1 = something; ...
  lb.data().Team1Score = int(gtk_spin_button_get_value(GTK_SPIN_BUTTON(lbt1)));
  lb.data().Team2Score = int(gtk_spin_button_get_value(GTK_SPIN_BUTTON(lbt2)));
  lb.data().Team3Score = int(gtk_spin_button_get_value(GTK_SPIN_BUTTON(lbt3)));
}

void TestGUI::onSendAEClicked(GtkButton *button, gpointer user_data)
{
  DataWriter<AnnouncementEvent> ae(ae_token);

  ae.data().message = gtk_entry_get_text(GTK_ENTRY(ae_message));

  D_MOD(classname << " sending " << ae.data());
}

void TestGUI::onSendFRClicked(GtkButton *button, gpointer user_data)
{
  DataWriter<FuelRewardEvent> fr(fr_token);
  fr.data().GroupNumber = gtk_spin_button_get_value_as_int
    (GTK_SPIN_BUTTON(fr_group));
  fr.data().fuel_reward = gtk_spin_button_get_value
    (GTK_SPIN_BUTTON(fr_fuel));

  D_MOD(classname << " sending " << fr.data());
}

void TestGUI::onSendStateClicked(GtkButton *button, gpointer user_data)
{
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ic_radio))) {
    DataWriter<InitialConditionsEvent> ic(ic_token);

    ic.data().GroupNumber = gtk_spin_button_get_value_as_int
    (GTK_SPIN_BUTTON(state_group));

    ic.data().x = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_x));
    ic.data().y = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_y));
    ic.data().z = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_z));
    ic.data().u = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_u));
    ic.data().v = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_v));
    ic.data().w = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_w));
    ic.data().p = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_p));
    ic.data().q = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_q));
    ic.data().r = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_r));
    ic.data().phi = M_PI/180.0f*
      gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_phi));
    ic.data().theta = M_PI/180.0f*
      gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_theta));
    ic.data().psi = M_PI/180.0f*
      gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_psi));

    ic.data().fuel_mass =
      gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_fuel));
  
    D_MOD(classname << " sending " << ic.data());
  } else {
    DataWriter<RespawnEvent> re(re_token);

    re.data().GroupNumber = gtk_spin_button_get_value_as_int
    (GTK_SPIN_BUTTON(state_group));

    re.data().x = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_x));
    re.data().y = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_y));
    re.data().z = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_z));
    re.data().u = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_u));
    re.data().v = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_v));
    re.data().w = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_w));
    re.data().p = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_p));
    re.data().q = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_q));
    re.data().r = gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_r));
    re.data().phi = M_PI/180.0f*
      gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_phi));
    re.data().theta = M_PI/180.0f*
      gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_theta));
    re.data().psi = M_PI/180.0f*
      gtk_spin_button_get_value (GTK_SPIN_BUTTON(state_psi));

    D_MOD(classname << " sending " << re.data());
  }
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
static TypeCreator<TestGUI> a(TestGUI::getMyParameterTable());

