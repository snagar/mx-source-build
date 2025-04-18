/*
 * dataref_manager.cpp
 *
 *  Created on: Aug 25, 2017
 *      Author: snagar
 */

// *************


#include "dataref_manager.h"

namespace missionx
{
Point          missionx::dataref_manager::planePoint;
Point          missionx::dataref_manager::cameraPoint;
XPLMDataTypeID missionx::dataref_manager::dataRefType;

float
dataref_manager::getAirspeed()
{
  return XPLMGetDataf(missionx::drefConst.indicated_airspeed_f);
}

float
dataref_manager::getGroundSpeed()
{
  return XPLMGetDataf(missionx::drefConst.dref_groundspeed_f);
}

float
dataref_manager::get_vh_ind()
{
  return XPLMGetDataf(missionx::drefConst.dref_vh_ind_f);
}

float
dataref_manager::get_gforce_normal()
{
  return XPLMGetDataf(missionx::drefConst.dref_gforce_normal_f);
}

float
dataref_manager::get_gforce_axil()
{
  return XPLMGetDataf(missionx::drefConst.dref_gforce_axil_f);
}

float
dataref_manager::get_g_normal()
{
  return XPLMGetDataf(missionx::drefConst.dref_g_nrml_f);
}

float
dataref_manager::get_fnrml_total()
{
  return XPLMGetDataf(missionx::drefConst.dref_fnrml_total_f);
}

float
dataref_manager::getAoA()
{
  return XPLMGetDataf(missionx::drefConst.AoA_f);
}

float
dataref_manager::getPitch()
{
  return XPLMGetDataf(missionx::drefConst.dref_pitch_f);
}

float
dataref_manager::getRoll()
{
  return XPLMGetDataf(missionx::drefConst.dref_roll_f);
}

float
dataref_manager::getFaxilGear()
{
  return XPLMGetDataf(missionx::drefConst.dref_faxil_gear_f);
}

float
dataref_manager::getBrakeLeftAdd()
{
  return XPLMGetDataf(missionx::drefConst.dref_brake_Left_add_f);
}

float
dataref_manager::getBrakeRightAdd()
{
  return XPLMGetDataf(missionx::drefConst.dref_brake_Right_add_f);
}

void
dataref_manager::setPlaneInLocalCoordiantes(double x, double y, double z)
{
  missionx::dataref_const dm;
  XPLMSetDatad(dm.dref_local_x_d, x);
  XPLMSetDatad(dm.dref_local_y_d, y);
  XPLMSetDatad(dm.dref_local_z_d, z);
}

}



missionx::dataref_manager::dataref_manager()
{
  // TODO Auto-generated constructor stub
  init();
}

void
missionx::dataref_manager::init()
{
}

missionx::dataref_manager::~dataref_manager()
{
  // TODO Auto-generated destructor stub
}

void
missionx::dataref_manager::flc()
{
#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
#endif // TIMER_FUNC
  dataref_manager::storePlanePoint();
  dataref_manager::storeCameraPoint(); // v3.0.223.7
}


void
missionx::dataref_manager::set_xplane_dataref_value(std::string full_name, double inValue)
{
  static XPLMDataRef dataRefId; 
  dataRefId = XPLMFindDataRef(full_name.c_str());

  if (dataRefId) // if exists
  {
    dataRefType = XPLMGetDataRefTypes(dataRefId);
    switch (dataRefType)
    {
      case xplmType_Int:
      {
        XPLMSetDatai(dataRefId, (int)inValue);
      }
      break;
      case xplmType_Float:
      {
        XPLMSetDataf(dataRefId, (float)inValue);
      }
      break;
      case (xplmType_Double): // v3.0.255.4.3 added to solve user dataref creation cases.
      case (xplmType_Float | xplmType_Double):
      {
        XPLMSetDatad(dataRefId, inValue);
      }
      break;
      case (xplmType_IntArray): //  can only return the value of specific array and not the whole array. arrayElementPicked must be defined
      {
        Log::logMsg("Setting value into INT array is not supported yet.");
      }
      break;
      case (xplmType_FloatArray): // can only return the value of specific array and not the whole array. arrayElementPicked must be defined
      {
        Log::logMsg("Setting value into FLOAT array is not supported yet.");
      }
      break;
      default:
      {
        Log::logMsg("[dref_manager] Can't handle this Dataref Datatype!!! ");
      }
      break;
    } // end switch
  }   // end if
} // end set_xplane_dataref_value

XPLMDataRef
missionx::dataref_manager::getDataRef(std::string inDrefName) // v2.1.0
{
  return XPLMFindDataRef(inDrefName.c_str());
}



/////////////////////////////////////////////////////////////////////


bool
missionx::dataref_manager::isSimPause()
{
  if (missionx::drefConst.dref_pause)
    return (bool)XPLMGetDatai(missionx::drefConst.dref_pause); // this should always work


  return (bool)getDataRefValue<int>(std::string("sim/time/paused")); // just in case
}

bool
missionx::dataref_manager::isSimRunning()
{
  bool bVal = !((bool)XPLMGetDatai(missionx::drefConst.dref_pause)); // return the oposite of what is returned. If return true then we convert to "false" = "sim is not running"
  
  return bVal;
  
}


bool
missionx::dataref_manager::isSimInReplayMode()
{
  if (missionx::drefConst.dref_is_in_replay)
    return (bool)XPLMGetDatai(missionx::drefConst.dref_is_in_replay); 


  return (bool)getDataRefValue<int>(std::string("sim/time/is_in_replay")); 
}


bool
missionx::dataref_manager::isPlaneOnGround()
{
  return (XPLMGetDataf(missionx::drefConst.dref_faxil_gear_f) == 0.0f ? false : true); // in air value == 0
}

double
missionx::dataref_manager::getLat()
{
  return XPLMGetDatad(missionx::drefConst.dref_lat_d);
}

double
missionx::dataref_manager::getLong()
{
  return XPLMGetDatad(missionx::drefConst.dref_lon_d);
}

double
missionx::dataref_manager::getElevation()
{
  return XPLMGetDatad(missionx::drefConst.dref_elev_d);
}

float
missionx::dataref_manager::getAGL()
{
  return XPLMGetDataf(missionx::drefConst.dref_y_agl_f);
}

float
missionx::dataref_manager::getHeadingPsi()
{
  return XPLMGetDataf(missionx::drefConst.dref_heading_true_psi_f);
}

float
missionx::dataref_manager::getTotalRunningTimeSec()
{
  return XPLMGetDataf(missionx::drefConst.dref_total_running_time_sec_f);
}

float
missionx::dataref_manager::get_mTotal_currentTotalWeightK()
{
  return XPLMGetDataf(missionx::drefConst.dref_m_total_f);
}

float
missionx::dataref_manager::get_acf_m_max_currentMaxPlaneAllowedWeightK()
{
  return XPLMGetDataf(missionx::drefConst.dref_acf_m_max_weight);
}

int
missionx::dataref_manager::getLocalDateDays()
{
  return XPLMGetDatai(missionx::drefConst.dref_local_date_days_i);
}

float
missionx::dataref_manager::getLocalTimeSec()
{
  return XPLMGetDataf(missionx::drefConst.dref_local_time_sec_f);
}

int
missionx::dataref_manager::getLocalHour()
{

  return XPLMGetDatai(missionx::drefConst.dref_local_time_hours_i);
}





// ---------------------------------------

void
missionx::dataref_manager::setQuaternion(float w, float x, float y, float z)
{
  float floatVals[4] = { w, x, y, z };

  if (missionx::drefConst.dref_q != NULL)
    XPLMSetDatavf(missionx::drefConst.dref_q, floatVals, 0, 4);
  else
    Log::logMsgErr("Failed to find and set Quaternion values !!!");
}

missionx::Point
missionx::dataref_manager::getPlanePointLocationThreadSafe()
{
  return missionx::dataref_manager::planePoint;
}

missionx::Point
missionx::dataref_manager::getCameraPointLocation()
{
  return missionx::dataref_manager::cameraPoint;
}

missionx::Point
missionx::dataref_manager::getCurrentPlanePointLocation(bool inStoreLocation)
{
  missionx::Point p(dataref_manager::getLat(), dataref_manager::getLong());
  p.setElevationMt(dataref_manager::getElevation());
  p.setHeading((double)dataref_manager::getHeadingPsi()); // v3.0.217.3

  if (inStoreLocation)
    dataref_manager::planePoint = p;


  return p;
}

// ---------------------------------------
void
missionx::dataref_manager::storePlanePoint()
{
  dataref_manager::planePoint = getCurrentPlanePointLocation(true);
}

void
missionx::dataref_manager::storeCameraPoint()
{
  double outX, outY, outZ;
  outX = outY = outZ = 0.0;

  const double x = XPLMGetDataf(drefConst.dref_camera_view_x_f);
  const double y = XPLMGetDataf(drefConst.dref_camera_view_y_f);
  const double z = XPLMGetDataf(drefConst.dref_camera_view_z_f);

  XPLMLocalToWorld(x, y, z, &outX, &outY, &outZ);

  outZ *= missionx::meter2feet;  // v24.03.2 fix elevation to be feet and not meter
  dataref_manager::cameraPoint = Point(outX, outY, outZ);
}

// ---------------------------------------



// ---------------------------------------
// ---------------------------------------
