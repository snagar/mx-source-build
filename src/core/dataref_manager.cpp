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
float          missionx::dataref_manager::fps = 0.0f;
missionx::structs::def_strct_plane_base_info missionx::dataref_manager::strct_plane_base_info;


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

  float dataref_manager::get_fps_f()
  {
    return 1.0f / dataref_manager::fps;
  }

  float dataref_manager::get_raw_fps_f()
  {
    return dataref_manager::fps;
  }

  float dataref_manager::init_raw_fps_f(const bool b_store)
  {
    auto local_fps = XPLMGetDataf(drefConst.fps_f_dref);
    if (local_fps == 0.0f)
      local_fps = 1.0f;

    if (b_store)
      dataref_manager::fps = local_fps;

    return local_fps;
  }

  void
  dataref_manager::setPlaneInLocalCoordiantes (const double x, const double y, const double z)
  {
    const missionx::dataref_const dm;
    XPLMSetDatad(dm.dref_local_x_d, x);
    XPLMSetDatad(dm.dref_local_y_d, y);
    XPLMSetDatad(dm.dref_local_z_d, z);
  }


  std::string dataref_manager::get_plane_icao() 
  { 

    XPLMDataRef drICAO = XPLMFindDataRef("sim/aircraft/view/acf_ICAO");
    if (drICAO != nullptr)
    {
      char buffer[64];
      // XPLMGetDatab is used for string (byte array) datarefs
      const int length = XPLMGetDatab(drICAO, buffer, 0, sizeof(buffer));
      if (length > 0)
        return {buffer};
        // return std::string(buffer, length);
    }

    return {};
  }

  void dataref_manager::gather_active_acf_base_info_for_llm() 
  { 
    dataref_manager::strct_plane_base_info.plane_max_gross_weight_f_and_p_kg = dataref_manager::get_acf_m_max_currentMaxPlaneAllowedWeightK(); // max total payload allowed
    dataref_manager::strct_plane_base_info.plane_empty_weight_kg             = XPLMGetDataf(missionx::drefConst.dref_acf_m_empty_weight); // current plane empty weight
    dataref_manager::strct_plane_base_info.plane_max_fuel_weight_kg          = XPLMGetDataf(missionx::drefConst.dref_acf_m_fuel_tot_lbs) * lbs2kg; // max allowed fuel. converted from lbs to kg
    dataref_manager::strct_plane_base_info.plane_max_speed_vno               = XPLMGetDataf(missionx::drefConst.dref_acf_vno_f); // max allowed cruise speed

    dataref_manager::strct_plane_base_info.plane_current_total_weight_kg = XPLMGetDataf(missionx::drefConst.dref_m_total_f); // current total weight: fuel+payload
    dataref_manager::strct_plane_base_info.plane_max_payload_kg          = dataref_manager::strct_plane_base_info.plane_max_gross_weight_f_and_p_kg - dataref_manager::strct_plane_base_info.plane_max_fuel_weight_kg - dataref_manager::strct_plane_base_info.plane_empty_weight_kg;

    dataref_manager::strct_plane_base_info.plane_estimated_fuel_endurance_hours = CalculateEstimatedFuelTimeHours();
  }

float dataref_manager::CalculateEstimatedFuelTimeHours()
{
    constexpr float POWER_ESTIMATE = 0.70f;
    constexpr int MAX_NUM_OF_ENGINES = 16;
    XPLMDataRef numEnginesRef = XPLMFindDataRef("sim/aircraft/engine/acf_num_engines");
    XPLMDataRef pMaxRef = XPLMFindDataRef("sim/aircraft/engine/acf_pmax_per_engine");
    // XPLMDataRef tMaxRef = XPLMFindDataRef("sim/aircraft/engine/acf_tmax");
    XPLMDataRef tMaxRef = XPLMFindDataRef("sim/aircraft/engine/acf_tmax_per_engine");
    XPLMDataRef clutchRef = XPLMFindDataRef("sim/aircraft/artstability/acf_has_clutch");
    XPLMDataRef fuelWeightRef = XPLMFindDataRef("sim/flightmodel/weight/m_fuel");

    const auto totalFuelKg = XPLMGetDataf(missionx::drefConst.dref_acf_m_fuel_tot_lbs) * lbs2kg;

    Log::logMsgThread(fmt::format("[{}] FFFF plane max fuel: {:.2f} FFFF\n", __func__, totalFuelKg));

    if (!numEnginesRef || !fuelWeightRef) return 0.0f;

    const int numEngines = XPLMGetDatai(numEnginesRef);
    if (numEngines <= 0) return 0.0f;

    if (totalFuelKg <= 0.0f) return 0.0f;

    const bool isHelicopter = clutchRef ? (XPLMGetDatai(clutchRef) != 0) : false;
    float fuelFlowKgPerSec = 0.0f;

    // 1. Check for Power-based engines (Pistons, Turboprops, Helicopters)
    if (pMaxRef) {
        float pMaxPerEngine[MAX_NUM_OF_ENGINES] = {0.0f};
        XPLMGetDatavf(pMaxRef, pMaxPerEngine, 0, MAX_NUM_OF_ENGINES);

        float totalPowerWatts = 0.0f;
        for (int i = 0; i < numEngines && i < MAX_NUM_OF_ENGINES; ++i) {
            totalPowerWatts += pMaxPerEngine[i];
        }

        if (totalPowerWatts > 0.0f) {
            if (isHelicopter) {
                // Helicopters demand higher average power for rotor management (~75%)
                const float cruisePowerWatts = totalPowerWatts * 0.75f;
                constexpr float turboshaftSfc = 0.00000010f;
                fuelFlowKgPerSec = cruisePowerWatts * turboshaftSfc;
            } else {
                // Standard Props / Turboprops (~65-70% cruise power)
                const float cruisePowerWatts = totalPowerWatts * 0.68f;
                constexpr float propSfc = 0.00000008f;
                fuelFlowKgPerSec = cruisePowerWatts * propSfc;
            }
        }
    }

    // 2. Fall back to Thrust-based engines (Jets / Turbofans) if power is zero
    if (fuelFlowKgPerSec <= 0.0f && tMaxRef) {
      float tMaxPerEngine[16] = {0.0f};
      XPLMGetDatavf(tMaxRef, tMaxPerEngine, 0, 16);

      float totalThrustNewtons = 0.0f;
      for (int i = 0; i < numEngines && i < 16; ++i) {
        totalThrustNewtons += tMaxPerEngine[i];
      }

      if (totalThrustNewtons > 0.0f) {
        // Update the jet fuel flow calculation using this calibrated coefficient
        // to match X-Plane's native 3.15-hour baseline estimation:
        constexpr float correctedJetSfc = 0.00000325f;
        fuelFlowKgPerSec = totalThrustNewtons * correctedJetSfc;
      }
    }

    // Ultimate safety fallback
    if (fuelFlowKgPerSec <= 0.0f) {
        fuelFlowKgPerSec = 0.04f;
    }

    #ifndef RELEASE
    auto debug_calc_result = (totalFuelKg / fuelFlowKgPerSec) / 3600.0f;
    #endif

    return (totalFuelKg / fuelFlowKgPerSec) / 3600.0f;
    // const auto totalFuelKg = max_fuel_f;
    // const float totalSeconds = totalFuelKg / fuelFlowKgPerSec;
    //
    // return totalSeconds / 3600.0f; // Returns estimated endurance in hours
}


} // namespace missionx


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
  dataref_manager::init_raw_fps_f(true); // v26.03.1
}


void
missionx::dataref_manager::set_xplane_dataref_value (const std::string full_name, const double inValue)
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
        XPLMSetDatai(dataRefId, static_cast<int> (inValue));
      }
      break;
      case xplmType_Float:
      {
        XPLMSetDataf(dataRefId, static_cast<float> (inValue));
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
missionx::dataref_manager::getDataRef (const std::string& inDrefName) // v2.1.0
{
  return XPLMFindDataRef(inDrefName.c_str());
}



/////////////////////////////////////////////////////////////////////


bool
missionx::dataref_manager::isSimPause()
{
  if (missionx::drefConst.dref_pause)
    return static_cast<bool> (XPLMGetDatai (missionx::drefConst.dref_pause)); // this should always work


  return static_cast<bool> (getDataRefValue<int> (std::string ("sim/time/paused"))); // just in case
}

bool
missionx::dataref_manager::isSimRunning()
{
  #ifndef RELEASE
  const bool bVal = !static_cast<bool> (XPLMGetDatai (missionx::drefConst.dref_pause)); // return the opposite of what is returned. If return true then we convert to "false" = "sim is not running"
  #endif

  return !static_cast<bool> (XPLMGetDatai (missionx::drefConst.dref_pause));
}


bool
missionx::dataref_manager::isSimInReplayMode()
{
  if (missionx::drefConst.dref_is_in_replay)
    return static_cast<bool> (XPLMGetDatai (missionx::drefConst.dref_is_in_replay));


  return static_cast<bool> (getDataRefValue<int> (std::string ("sim/time/is_in_replay")));
}


bool
missionx::dataref_manager::isPlaneOnGround()
{
  return (XPLMGetDataf (missionx::drefConst.dref_faxil_gear_f) != 0.0f); // in air value == 0
}

bool
dataref_manager::isPlaneAirborne ()
{
  return (!isPlaneOnGround());
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
missionx::dataref_manager::getLocalMinutes ()
{
  const auto local_time_sec_f = static_cast<int> (XPLMGetDataf (missionx::drefConst.dref_local_time_sec_f));
  #ifndef RELEASE
  const auto minutesInCurrentHour = (local_time_sec_f % 3600) / 60;
  #endif

  return (local_time_sec_f % 3600) / 60;
}

int
missionx::dataref_manager::getLocalHour()
{
  return XPLMGetDatai(missionx::drefConst.dref_local_time_hours_i);
}

// ---------------------------------------

void
missionx::dataref_manager::setQuaternion (const float w, const float x, const float y, const float z)
{
  float floatVals[4] = { w, x, y, z };

  if (missionx::drefConst.dref_q != nullptr)
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
missionx::dataref_manager::getCurrentPlanePointLocation (const bool inStoreLocation)
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
