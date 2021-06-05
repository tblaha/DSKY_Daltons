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
#include <vector>
#include <map>
#include <algorithm>
#include <boost/algorithm/string.hpp>
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
  string _filename {"Tests.log"};

  ofstream _logfile {};
  stringstream _logtext {};
  float _time_passed_since_published {0};
  float _time_passed_since_last_log {0};
  stringstream debug_info {};

  void document_test(const string desc, const bool& pass, const stringstream& debug_info, const TimeSpec& ts) {
      //_logtext << ts.getUsecsElapsed();
      _logtext << "Test #" << _test_id << " | " << (pass ? "OK" : "FAIL") << " | " << desc << endl;
      _logtext << debug_info.str() << endl;
  };


  void flush_to_file() {
    _logfile << _logtext.str();
  };

  map<float, vector<float>> read_csv(std::istream& str)
  {
    map<float, vector<float>> result;

    std::string line;
    int i = 0;
    while (std::getline(str, line))
    {
      if (i++)
      {
        std::vector<std::string> vec;
        boost::algorithm::split(vec, line, boost::is_any_of(","));

        float time = stof( vec[0] );
        vector<std::string> data = vector<std::string>(vec.begin() + 1, vec.end());

        // convert to floats
        // https://stackoverflow.com/questions/20257582/convert-vectorstdstring-to-vectordouble
        vector<float> data_f(data.size());
        std::transform(data.begin(), data.end(), data_f.begin(), [](const std::string& val)
        {
          return stof( val );
        });

        result.insert( pair<float, vector<float>> (time, data_f) );
      }
    }
    return result;
  }

  vector<float> interpolate_map(const map<float, vector<float>>& map2d, const float& value) {
    // https://stackoverflow.com/questions/26597983/stdmap-find-key-pair-and-interpolate-value
//    if ( map.find(value) != map.end() )   return map.at(value);
    if ( value < map2d.begin()->first )     return map2d.begin()->second;
    if ( value > map2d.rbegin()->first )    return map2d.rbegin()->second;

    // little funny, lower_bound actually returns a pointer to the first element that is larget than value 
    auto lower = map2d.lower_bound(value) == map2d.begin() ? map2d.begin() : --( map2d.lower_bound(value));
    auto upper = map2d.upper_bound(value);

    // there is probably no need to call fabs, because maps are guaranteed to be ordered
    float alpha = (value - lower->first)/fabs(upper->first - lower->first);

    // perform computation on all elements of the vector... this is what you get for not reading
    // the manual... valarrays are much better suited, because they already have operator* and operator+ defined...
    vector<float> outarray = lower->second;
    transform(upper->second.begin(), upper->second.end(), outarray.begin(), outarray.begin(), std::minus<float>());
    transform(outarray.begin(), outarray.end(), outarray.begin(), [&alpha](const float& val){return alpha*val; });
    transform(lower->second.begin(), lower->second.end(), outarray.begin(), outarray.begin(), std::plus<float>());

    return outarray;
  }

  bool _generated_map {false};
  float _time_elapsed {0.0f};
  int _log_count {0};
  map<float, vector<float>> _map2d;
  stringstream _csv_output;
  string _csv_output_file;


  void perform_csv_tests(const TimeSpec& ts, const bool& hold) {
    if (hold || _csv_test_id >= _num_csv_tests) {
      write_test_data(ts, -1);
      return;
    }

    if (!_generated_map) {
      _csv_output = stringstream();
      string csv_input_file_name;
      add_csv_header(_csv_output, csv_input_file_name, _csv_test_id);

      stringstream csv_input_file;
      csv_input_file << "../../../unit-tester/" << csv_input_file_name;

      std::ifstream file(csv_input_file.str());
      _map2d = read_csv(file);

      stringstream csv_output_file_s;
      csv_output_file_s << "log_" << csv_input_file_name;
      _csv_output_file = csv_output_file_s.str();

      D_MOD("Imported csv for csv_test #" << _csv_test_id);

      _generated_map = true;
      _time_elapsed = 0.0f;
      _log_count = 0;
    }

    // interpolate and write csv test data to channels and obtain suitable _log_every_seconds
    vector<float> data_f = interpolate_map(_map2d, _time_elapsed);
    write_csv_test_data(ts, data_f, _csv_test_id);

    // log latest return data to stream
    if (_time_elapsed > _log_count * _log_every_seconds) {
      add_csv_data_line(_csv_output, _time_elapsed, _csv_test_id);
      _log_count++;
    }

    if (_time_elapsed >= _map2d.crbegin()->first) {  // reverse iterator gives highest time specified in csv
      // test finished, log to file
      D_MOD("Finished csv_test #" << _csv_test_id);
      ofstream logfile {};
      logfile.open(_csv_output_file);
      logfile << _csv_output.str();
      logfile.close();


      // reset counters
      _csv_test_id++;
      _generated_map = false;
    }

    // keep track of time
    _time_elapsed += ts.getDtInSeconds();

  };

  bool _test_published {false};
  bool _response_ready {false};

  void perform_tests(const TimeSpec& ts, const bool& hold) {
    if ((_test_id < _num_tests) && !hold && !_test_published) {
      // if we are in hold: act like we didn't publish yet, so don't transition
      // to reading any reponses (there won't be any)
      _test_published = true;
      _response_ready = false;
      _time_passed_since_published = 0;
      _time_passed_since_last_log = 0;

      debug_info = stringstream();
      
      write_test_data(ts, _test_id);
      D_MOD("Publishing test #" << _test_id);

    } else {
      if (_test_published && !_response_ready) {

        float dt = ts.getDtInSeconds();
        _time_passed_since_published += dt;
        _time_passed_since_last_log += dt;

        // check if response should already be evaluated. if not, publish test again
        if (_time_passed_since_published >= _test_wait) {
          _response_ready = true;
          write_test_data(ts, -1);
        } else {
          write_test_data(ts, _test_id);
          D_MOD("Republishing test #" << _test_id);
        }

        // check if we need to do a log of the current outputs to the debug info stream
        if (_time_passed_since_last_log >= _log_every_seconds) {
          // write debug data to log file
          stringstream intermediate_debug_info {};
          generate_debug_info(intermediate_debug_info);

          debug_info << "Delta t: " << _time_passed_since_published << "sec" << endl;
          debug_info << intermediate_debug_info.str();

          _time_passed_since_last_log = 0;
        }
      } else { 
        // write default data if not currently performing a test 
        write_test_data(ts, -1);
      }
      if (_response_ready) {
        // we should now evaluate if the reponse is what we expect
        string desc;
        bool test_passed = evaluate_response(desc); // this assigns the test description as well

        document_test(desc, test_passed, debug_info, ts);

        if (test_passed) {
          D_MOD("OK");
        } else {
          D_MOD("FAIL");
        }

        // test done! reset states and increate test count
        _test_published = false;
        _response_ready = false;
        _test_id++;
      }
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
