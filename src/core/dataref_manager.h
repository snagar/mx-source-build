#ifndef DATAREF_MANAGER_H_
#define DATAREF_MANAGER_H_

#include "../io/Log.hpp"
//#include "../logic/dataref_param.h" // v3.303.14 removed not needed
//#include "Utils.h" // v3.303.14 removed not needed
#include "coordinate/Point.hpp"
//#include "dataref_const.h" // v3.303.14 removed for XPLMDataAccess
#include "XPLMDataAccess.h"

using namespace missionx;
// using namespace mxconst;

namespace missionx
{

class dataref_manager
{
private:
  static Point planePoint;         // holds plane coordinate location
  static Point cameraPoint;        // v3.0.223.7
  static void  storePlanePoint();  // flc
  static void  storeCameraPoint(); // flc

public:
  // static dataref_const drefConst;

  dataref_manager();
  virtual ~dataref_manager();


  static XPLMDataTypeID dataRefType;
  static XPLMDataRef    getDataRef(std::string inDrefName); // v2.1.0
  static void           set_xplane_dataref_value(std::string full_name, double inValue);

  // get dataref value Template (for int, float, double only values )
  template<class N>
  static N getDataRefValue(std::string full_name)
  {
    XPLMDataRef    data_id;
    XPLMDataTypeID type_id;

    data_id = XPLMFindDataRef(full_name.c_str());
    if (data_id) // if exists
    {
      type_id = XPLMGetDataRefTypes(data_id);
      switch (type_id)
      {
        case xplmType_Int:
        {
          return (N)XPLMGetDatai(data_id);
        }
        break;
        case xplmType_Float:
        {
          return (N)XPLMGetDataf(data_id);
        }
        break;
        case (xplmType_Float | xplmType_Double):
        {
          return (N)XPLMGetDatad(data_id);
        }
        break;
        default:
        {
          Log::logMsg("Can't get DataRef " + full_name);
        }
        break;
      } // end switch
    }   // end if

    return 0;
  } // end template



  // special members
  static bool isSimPause();
  static bool isSimRunning();  // v3.303.14
  static bool isSimInReplayMode(); // v3.303.14
  static bool isPlaneOnGround();

  static double getLat();
  static double getLong();
  static double getElevation();
  static float  getAGL(); // above ground elevation in meters
  static float  getHeadingPsi();          // v3.0.217.3 for <start_location> in random implementation
  static float  getTotalRunningTimeSec(); // time in seconds from the start of the sim. can be used in Timers
  static float  get_mTotal_currentTotalWeightK(); // v3.303.14
  static float  get_acf_m_max_currentMaxPlaneAllowedWeightK(); // v3.303.14

  static int   getLocalDateDays();
  static float getLocalTimeSec();
  static int   getLocalHour(); // v3.303.8

  static void flc(); // every loop back fetch information once to use in all shared.dref->xx code; example: read pause pointState.
  static void init();

  static void setQuaternion(float w, float x, float y, float z);


  // members
  static Point getPlanePointLocationThreadSafe();
  static Point getCameraPointLocation(); // v3.0.223.7
  static Point getCurrentPlanePointLocation(bool inStoreLocation = false);

  static float getAirspeed();
  static float getGroundSpeed();
  static float get_vh_ind();        // VVI Vertical Velocity in meters per seconds
  static float get_gforce_normal(); //
  static float get_gforce_axil();   //
  static float get_g_normal(); //
  static float get_fnrml_total(); //
  static float getAoA();
  static float getPitch();
  static float getRoll();

  static float getFaxilGear();
  static float getBrakeLeftAdd();
  static float getBrakeRightAdd();

  static void setPlaneInLocalCoordiantes(double x, double y, double z);

};

}

#endif
