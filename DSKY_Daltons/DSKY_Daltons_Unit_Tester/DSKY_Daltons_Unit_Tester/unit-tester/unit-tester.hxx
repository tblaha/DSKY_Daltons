/* ------------------------------------------------------------------   */
/*      item            : unit_tester.hxx
        made by         : tblaha
        from template   : DusimeModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Sat May 22 20:22:15 2021
        category        : header file
        description     :
        changes         : Sat May 22 20:22:15 2021 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
*/

#ifndef unit_tester_hxx
#define unit_tester_hxx

// include the dusime header
#include <dusime.h>
USING_DUECA_NS;

// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"
//#include "tester.hxx"

// include headers for functions/classes you need in the module
#include <iostream>
#include <sstream>
#include <fstream>
#include "tests.hxx"

using namespace std;

/** A module.

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude unit_tester.scm
*/
class unit_tester: public SimulationModule, public tests
{
  /** self-define the module type, to ease writing the parameter table */
  typedef unit_tester _ThisModule_;

private: // simulation data
  // declare the data you need in your simulation

private: // tester stuff
  string filename {"Tests.log"};

  ofstream _logfile {};
  stringstream _logtext {};

  void document_test(const string desc, const bool& pass, const stringstream& debug_info, const TimeSpec& ts) {
      //_logtext << ts.getUsecsElapsed();
      _logtext << "Test #" << _test_id << " | " << (pass ? "OK" : "FAIL") << " | " << desc << endl;
      _logtext << debug_info.str() << endl;
  };


  void flush_to_file() {
    _logfile << _logtext.str();
  };

  void perform_tests(const TimeSpec& ts, const bool& hold) {
    if ((_test_id < _num_tests) && !hold && !_test_published) {
      // if we are in hold: act like we didn't publish yet, so don't transition
      // to reading any reponses (there won't be any)
      _test_published = true;
      _response_ready = true; // for now, maybe wait a defined amount if iterations
      
      write_test_data(ts, _test_id);
    } else {
      if (_response_ready) {
        string desc;
        bool test_passed = evaluate_response(desc);

        stringstream debug_info {};
        generate_debug_info(debug_info);
        document_test(desc, test_passed, debug_info, ts);

        if (test_passed) {
          D_MOD("OK");
        } else {
          D_MOD("FAIL");
        }

        _test_published = false;
        _response_ready = false;

        _test_id++;
      }
      // write default data if not currently performing a test 
      write_test_data(ts, -1);
    }

  };


private: // trim calculation data
  // declare the trim calculation data needed for your simulation

private: // snapshot data
  // declare, if you need, the room for placing snapshot data

private: // channel access
  // declare access tokens for all the channels you read and write
  // examples:
  // ChannelReadToken    r_mytoken;
  // ChannelWriteToken   w_mytoken;
  

private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  PeriodicAlarm        myclock;

  /** Callback object for simulation calculation. */
  Callback<unit_tester>  cb1;

  /** Activity for simulation calculation. */
  ActivityCallback      do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the initial condition table. */
  static const IncoTable*            getMyIncoTable();

  /** Return the parameter table. */
  static const ParameterTable*       getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  unit_tester(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~unit_tester();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /** Request check on the timing. */
  bool checkTiming(const std::vector<int>& i);

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

public: // member functions for cooperation with DUSIME
  /** For the Snapshot capability, fill the snapshot "snap" with the
      data saved at a point in your simulation (if from_trim is false)
      or with the state data calculated in the trim calculation (if
      from_trim is true). */
  void fillSnapshot(const TimeSpec& ts,
                    Snapshot& snap, bool from_trim);

  /** Restoring the state of the simulation from a snapshot. */
  void loadSnapshot(const TimeSpec& t, const Snapshot& snap);

  /** Perform a trim calculation. Should NOT use current state
      uses event channels parallel to the stream data channels,
      calculates, based on the event channel input, the steady state
      output. */
  void trimCalculation(const TimeSpec& ts, const TrimMode& mode);
};

#endif
