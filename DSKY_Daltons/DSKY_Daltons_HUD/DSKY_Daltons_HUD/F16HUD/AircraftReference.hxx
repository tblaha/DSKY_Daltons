// ------------------------------------------------------------------
//
//      item            : AircraftReference.hxx
//      made by         : Clark Borst / Herman Damveld
//      date            : Tue Dec  14 21:24:00 2004
//      category        : header file
//      description     : Aircraft reference symbol class
//      changes         : Wed Mar 23 10:25:00 2004 first version
//      language        : C++
//
// ------------------------------------------------------------------

#ifndef AIRCRAFTREFERENCE_HXX
#define AIRCRAFTREFERENCE_HXX

#include <Eigen/Dense>
#include <vector>
#include <string>

enum Shapes { SQUARE, DIAMOND, CIRCLE, DOT };

class AircraftReference
{
   public:
      // ctor
      AircraftReference(double height, double vfov, double width);

      // dtor
      ~AircraftReference();

      // utility
      inline void SetScreenHeight(double height){screenheight = height;}  // drawing units
      inline void SetScreenWidth(double width){screenwidth = width;}
      inline void SetVertFOV(double vfov){vert_fov = vfov;} // deg
      inline void SetRangeScale(float range_scaler){float s = 10000.0f * (range_scaler*0.96f + 0.04); _x_range = s; _y_range = s;}

      // utility
      inline void SetScreenPosition(double x, double y){x_screenpos = x; y_screenpos = y;}
      inline void SetLineWidth(double lw){lineWidth = lw;}
      inline void SetLineColor(double r, double g, double b){lineColor[0] = (GLfloat)r; lineColor[1] = (GLfloat)g; lineColor[2] = (GLfloat)b;}

      inline void SetGunCrossPitchAngle(double theta_gun_cross_rad){theta0 = theta_gun_cross_rad;} // rad
      void SetNz(double nz_g, bool sat); // g's
      inline void SetFuel(double fuel) {_fuel = fuel;};
      inline void SetNzMax(double nm) {nz_max = nm;} // g's //[HJD] added 24/04/2006
      inline void ResetNz(void){nz_max = 1.0;}
      inline void SetSteerpointNo(int stpno){steerpointno = stpno;}
      inline void SetSteerpointDist(double stpdist){steerpointdist = stpdist;} // m
      inline void SetEstimateTimeEnroute(double EstTimeEnr){ETE = EstTimeEnr;} // seconds
      inline void SetYaw(double yaw) {_veh_yaw = yaw;};
      inline void SetRelativeSpeedVector(Eigen::Vector2f PI_proj){_PI_proj = PI_proj;}
      inline void SetRelativeLandingSites(std::vector<Eigen::Vector3f> sitesdiff){_sitesdiff = sitesdiff;}
      inline void SetRelativeOtherGroups(std::vector<Eigen::Vector3f> groupsdiff){_groupsdiff = groupsdiff;} // m

      // referee stuff
      inline void SetMessage(std::string message){_message = message;}
      inline void SetLeaderboard(int g1, int g2, int g3){_g1 = g1; _g2 = g2; _g3 = g3;};

      // opengl display
      void DrawDiamond(Eigen::Vector3f xyz, int i);
      void DrawDot(Eigen::Vector3f xyz, int i);

      template <class T>
      T clamp(T in, T min, T max) {return in > max ? max : in < min ? min : in;}
      void DrawGL();


   private:
      double Rad2Deg(double);
      double Deg2Rad(double);
      void strokeString(double, double, char*, void*, double);
      void strokeString(double, double, char*, void*, double, int);

   private:
      const float size_x = 10000.0f; // max x dimension of playing field
      const float size_y = 10000.0f; // max y dimension of playing field
      double vert_fov;
      double screenheight;
      double screenwidth;
      double x_screenpos;
      double y_screenpos;
      float _range_scaler{};

      double lineWidth;
      GLfloat lineColor[3];

      double theta0;          // rad
      std::vector<Eigen::Vector3f> _sitesdiff {};
      std::vector<Eigen::Vector3f> _groupsdiff {};
      double _veh_yaw;
      Eigen::Vector2f _PI_proj {};
      double _x_range;
      double _y_range;
      bool _sat;
      double nz;              // g
      double nz_max;          // g
      int    steerpointno;    //
      double steerpointdist;  // m
      double ETE;             // seconds

      std::string _message{};
      int _g1{};
      int _g2{};
      int _g3{};
      float _fuel{};
};

#endif
