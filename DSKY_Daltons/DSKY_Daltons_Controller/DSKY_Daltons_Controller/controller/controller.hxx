/* ------------------------------------------------------------------   */
/*      item            : controller.hxx
        made by         : tblaha
        from template   : DusimeModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Thu May 13 15:41:29 2021
        category        : header file
        description     :
        changes         : Thu May 13 15:41:29 2021 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
*/

#ifndef controller_hxx
#define controller_hxx

// include the dusime header
#include <dusime.h>
USING_DUECA_NS;

#include <Eigen/Dense>

// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"

// include headers for functions/classes you need in the module


/** A module.

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude controller.scm
*/
class controller: public SimulationModule
{
  /** self-define the module type, to ease writing the parameter table */
  typedef controller _ThisModule_;

private: // simulation data
  // declare the data you need in your simulation
  // Input rates
  Eigen::Array3f myRates{};
  float myThrottle;
  // Maximum angular rates
  float rref;
  // Maximum/minium thurster moments
  float maxThrusterMoment;
  // Vehicle angular rates
  Eigen::Array3f mypqr{};
  // Error
  Eigen::Array3f epqr{};
  // Proportional control
  Eigen::Array3f Prop_pqr{};
  float Kp_p {1.0f};
  float Kp_q {1.0f};
  float Kp_r {1.0f};
  Eigen::Array3f Prop_gain{};
  // Derivative control
  Eigen::Array3f Der_pqr{};
  float Kd_p {0.0f};
  float Kd_q {0.0f};
  float Kd_r {0.0f};
  Eigen::Array3f Der_gain{};
  // Output Forces and Moments
  float myMx;
  float myMy;
  float myMz;
  float myFx;
  float myFy;
  float myFz;
  bool sat_neg{};
  bool sat_pos{};
  float vertical_rate_sp{};
  float current_z{};
  float terminal_z{};
  float map_range_setting{};

  Eigen::Array3f epqr_prev{0,0,0};

  /**
   * @brief Thrust controller internal variables
   * 
   */
  Eigen::Quaternionf        myquat{};
  Eigen::Vector3f           myuvw{};
  Eigen::Vector3f           myInertialVel{};
  const Eigen::Vector3f  nB{0,0,1}; /**< Quaternized normal vector in body coordinates */
  Eigen::Vector3f        nI{}; /**< Quaternized normal vector in inertial coordinates */
  const float T_max = 27000.0f; /**< Newtons */
  const float t_limits[2] {0.01, 0.99};  /**< usable thrust region of the engine */
  const float gM = 1.62f; /**< moon gravity in m/s/s */
  float mass; /**< current mass as given by the dynamics module */
  float T_ref {}; /**< to be calculated by the controller */
  float max_zdot {};
  float z_ref_setting {1.0f};
  float z_ref_mult {1.0f};
  float Tp {2e-1};
  float mu {1.0f};

  void update_max_zdot() { max_zdot = z_ref_mult * z_ref_setting * z_ref_setting; }; /**< max zdot, depending on mode */

  float gen_z_dot_ref(const float& stick) {return -max_zdot * 2.0f * (stick - 0.5f); }; /**< controller reference for decend rate (positive down) */

  template<class T>
  T clamp(const T& v, const T& lo, const T& hi) {return (v<lo) ? lo : (v>hi) ? hi : v;};
  

private: // trim calculation data
  // declare the trim calculation data needed for your simulation

private: // snapshot data
  // declare, if you need, the room for placing snapshot data

private: // channel access
  // declare access tokens for all the channels you read and write
  // examples:
  // ChannelReadToken    r_mytoken;
  // ChannelWriteToken   w_mytoken;

  // Tokens for the primary and secondary channels for control and switches
  StreamChannelReadToken<PrimaryControls> myControlPrimaryStreamReadToken;
  StreamChannelReadToken<SecondaryControls> myControlSecondaryStreamReadToken;

  StreamChannelReadToken<PrimarySwitches> mySwitchPrimaryStreamReadToken;
  StreamChannelReadToken<SecondarySwitches> mySwitchSecondaryStreamReadToken;

  // Tokens for the vehicle state
  StreamChannelReadToken<vehicleState> myVehicleStateStreamReadToken;

  //Token to write to thruster force
  StreamChannelWriteToken<thrusterForces> myThrusterForcesStreamWriteToken;
  StreamChannelWriteToken<flightControlModes> myflightControlModesToken;


private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  PeriodicAlarm        myclock;

  /** Callback object for simulation calculation. */
  Callback<controller>  cb1;

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
  controller(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~controller();

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
