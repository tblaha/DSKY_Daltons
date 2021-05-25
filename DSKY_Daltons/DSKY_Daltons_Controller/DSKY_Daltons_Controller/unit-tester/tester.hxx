/* ------------------------------------------------------------------   */
/*      item            : tester.hxx
        made by         : tblaha
        from template   : DusimeModuleTemplate.hxx
        template made by: Till Blaha
        date            : Sat May 22 20:22:15 2021
        category        : header file
        description     :
        changes         : Sat May 22 20:22:15 2021 first version
        template changes: 
        language        : C++
        copyright       : (c) 2021 
*/


#ifndef tester_hxx
#define tester_hxx

#define D_MOD
#include "debug.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <dusime.h>
USING_DUECA_NS;
#include "comm-objects.h"

#include "tests.hxx"

using namespace std;

class tester: public tests
{
public:
  ofstream _logfile {};
  stringstream _logtext {};

  void document_test(bool& pass, stringstream& debug_info, const TimeSpec& ts) {
      //_logtext << ts.getUsecsElapsed();
      _logtext << "Test #" << _test_id << " | " << (pass ? "OK" : "FAIL") << endl;

      _logtext << debug_info.str() << endl << endl;
  };


public:
    tester(const string filename, const GlobalId& Id, const string Entity, const char *part, const char *classname_in):
        tests(Id, Entity, part, classname_in) // custom inheritance constructors also work in constructor definitions, nice
    {
        _logfile.open(filename);
    };
    ~tester() {
        flush_to_file();
        _logfile.close();
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
                bool test_passed = evaluate_response();

                stringstream debug_info {};
                generate_debug_info(debug_info);
                document_test(test_passed, debug_info, ts);

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



private:
    
};


#endif