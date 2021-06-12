/* ------------------------------------------------------------------   */
/*      item            : TestGUI.hxx
        made by         : ostroosma
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Fri May 21 12:50:28 2021
        category        : header file
        description     :
        changes         : Fri May 21 12:50:28 2021 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
*/

#ifndef TestGUI_hxx
#define TestGUI_hxx

// include the dusime header
#include <dueca.h>
USING_DUECA_NS;

// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"

// include headers for functions/classes you need in the module
#include <GtkGladeWindow.hxx>


/** A simulation module.

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude test-gui.scm
 */
class TestGUI: public Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef TestGUI _ThisModule_;

private: // simulation data
  // declare the data you need in your simulation

  /** Glade object. */
  GtkGladeWindow winglade;

  /** GTK widgets. */
  GtkWidget *ae_message;
  GtkWidget *fr_group, *fr_fuel;
  GtkWidget *ic_radio;
  GtkWidget *state_x, *state_y, *state_z, *state_u, *state_v, *state_w;
  GtkWidget *state_phi, *state_theta, *state_psi, *state_p, *state_q, *state_r;
  GtkWidget *state_fuel, *state_group;
  GtkWidget *lbt1, *lbt2, *lbt3;
  
  
public: // GTK callbacks
  // Process a Command
  void onSendAEClicked(GtkButton *button,
		       gpointer user_data);
  void onSendFRClicked(GtkButton *button,
		       gpointer user_data);
  void onSendStateClicked(GtkButton *button,
		       gpointer user_data);

private: // channel access
  // declare access tokens for all the channels you read and write
  // examples:
  // ChannelReadToken    r_mytoken;
  // ChannelWriteToken   w_mytoken;
  ChannelWriteToken ae_token;
  ChannelWriteToken fr_token;
  ChannelWriteToken ic_token;
  ChannelWriteToken re_token;
  ChannelWriteToken lb_token;
  
  ChannelReadToken *lbs_token;
  ChannelReadToken *vss_token;
  
private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  PeriodicAlarm        myclock;

  /** Callback object for simulation calculation. */
  Callback<TestGUI>  cb1;

  /** Activity for simulation calculation. */
  ActivityCallback      do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the parameter table. */
  static const ParameterTable*       getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  TestGUI(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~TestGUI();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /** Request check on the timing. */
  bool checkTiming(const std::vector<int>& i);

  /** Read the stream channels. */
  bool readStreams(const bool& b);
  
public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec& ts);

private:
  // utility functions
  void fillGUI(const LeaderboardStream&);
  void fillGUI(const VehicleStateStream&);
};

#endif
