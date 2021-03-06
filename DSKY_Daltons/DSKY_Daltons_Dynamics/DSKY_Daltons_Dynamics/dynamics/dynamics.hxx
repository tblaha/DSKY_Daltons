/* ------------------------------------------------------------------   */
/*      item            : dynamics.hxx
        made by         : tblaha
        from template   : DusimeModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Thu May 13 16:10:42 2021
        category        : header file
        description     :
        changes         : Thu May 13 16:10:42 2021 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
*/

#ifndef dynamics_hxx
#define dynamics_hxx

// include the dusime header
#include <dusime.h>
USING_DUECA_NS;

// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"

// include headers for functions/classes you need in the module
#include "RigidBody.hxx"
#include "integrate_rungekutta.hxx"
#include <Eigen/Dense>

using Eigen::Vector3d;
using Eigen::VectorXd;
using Eigen::Map;
using Eigen::Quaterniond;

struct ThrusterForcesData {
  Vector3d F;
  Vector3d M;
};

struct VehicleStateData {
  Vector3d xyz;
  Vector3d uvw;
  Quaterniond quat;
  float phi;
  float theta;
  float psi;
  Vector3d pqr;
  float thrust;
  float dryMass;
  float fuelMass;
  float groundHeight;
  bool landed;
  //float lgDelta[4]; need to think about the best way to do this, arrays suck
};

/** A module.

    The instructions to create an module of this class from the Scheme
    script are:

  \verbinclude dynamics.scm
*/
class dynamics: public SimulationModule
{
  /** self-define the module type, to ease writing the parameter table */
  typedef dynamics _ThisModule_;

  private: // simulation data
    // declare the data you need in your simulation
    RigidBody body;
    RungeKuttaWorkspace workspace;
    Vector3d gravity;
    Vector3d thursterLocation;

    // Channel structs
    ThrusterForcesData thrusterForcesData;
    VehicleStateData vehicleStateData;

  private: // trim calculation data
    // declare the trim calculation data needed for your simulation

  private: // snapshot data
    // declare, if you need, the room for placing snapshot data

  private: // channel access
    // declare access tokens for all the channels you read and write
    // examples:
    // ChannelReadToken    r_mytoken;
    // ChannelWriteToken   w_mytoken;
    StreamChannelReadToken<thrusterForces> thrusterForcesReadToken;
    StreamChannelWriteToken<vehicleState> vehicleStateWriteToken;
    StreamChannelWriteToken<ObjectMotion> ObjectMotionWriteToken;

    // Referee channels
    ChannelWriteToken refVehicleStateWriteToken;
    EventChannelReadToken<InitialConditionsEvent> initialConditionsEventReadToken;
    EventChannelReadToken<RespawnEvent> respawnEventReadToken;
    EventChannelReadToken<FuelRewardEvent> fuelRewardEventReadToken;
    StreamChannelReadToken<TerrainHeightStream> terrainHeightStreamReadToken;

  private: // activity allocation
    /** You might also need a clock. Don't mis-use this, because it is
        generally better to trigger on the incoming channels */
    //PeriodicAlarm        myclock;

    /** Callback object for simulation calculation. */
    Callback<dynamics> cb1;

    /** Activity for simulation calculation. */
    ActivityCallback do_calc;

  public: // class name and trim/parameter tables
    /** Name of the module. */
    static const char* const classname;

    /** Return the initial condition table. */
    static const IncoTable* getMyIncoTable();

    /** Return the parameter table. */
    static const ParameterTable* getMyParameterTable();

  public: // construction and further specification
    /** Constructor. Is normally called from scheme/the creation script. */
    dynamics(Entity* e, const char* part, const PrioritySpec& ts);

    /** Continued construction. This is called after all script
        parameters have been read and filled in, according to the
        parameter table. Your running environment, e.g. for OpenGL
        drawing, is also prepared. Any lengty initialisations (like
        reading the 4 GB of wind tables) should be done here.
        Return false if something in the parameters is wrong (by
        the way, it would help if you printed what!) May be deleted. */
    bool complete();

    /** Destructor. */
    ~dynamics();

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
    void fillSnapshot(const TimeSpec& ts, Snapshot& snap, bool from_trim);

    /** Restoring the state of the simulation from a snapshot. */
    void loadSnapshot(const TimeSpec& t, const Snapshot& snap);

    /** Perform a trim calculation. Should NOT use current state
        uses event channels parallel to the stream data channels,
        calculates, based on the event channel input, the steady state
        output. */
    void trimCalculation(const TimeSpec& ts, const TrimMode& mode);

  private:
    void readInitialConditionsEvent(const TimeSpec& ts);
    void readRespawnEvent(const TimeSpec& ts);
    void readFuelRewardEvent(const TimeSpec& ts);
    void readThrusterForcesStream(const TimeSpec& ts);
    void readTerrainHeightStream(const TimeSpec& ts);
    void writeVehicleStateStream(const TimeSpec& ts);
    void writeRefVehicleStateStream(const TimeSpec& ts);
    void writeObjectMotionStream(const TimeSpec& ts);
  public:
    void derivative(VectorE& xd, double dt);
    const Vector& X() const;
    void setState(const VectorE& newx);
    void bodyStep(const TimeSpec& ts);
    void bodySetStateToCurrentData();
};

#endif
