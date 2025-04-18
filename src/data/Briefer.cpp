#include "Briefer.hpp"
#include "../core/dataref_manager.h" // v3.303.14 moved from header

namespace missionx
{
bool Briefer::need_to_pause_xplane;

}


missionx::Briefer::Briefer()
{
  this->node.updateName (mxconst::get_ELEMENT_BRIEFER ().c_str ()); // v3.303.11
  init ();
}

void Briefer::init()
{
  has_valid_location_adjust = false;
  need_to_pause_xplane      = false; // relevant after position plane function was called

  if (!this->node.isEmpty())
  {
    setStringProperty(mxconst::get_ATTRIB_STARTING_ICAO(), "");
    setStringProperty(mxconst::get_ATTRIB_STARTING_LEG(), "");

    // // location adjust properties
    setNumberProperty<double>(mxconst::get_ATTRIB_LAT(), 0.0);
    setNumberProperty<double>(mxconst::get_ATTRIB_LONG(), 0.0);
    setNumberProperty<double>(mxconst::get_ATTRIB_ELEV_FT(), 0.0);      // if elevation_ft is set to 0, we will place the plane on ground.
    setNumberProperty<float>(mxconst::get_ATTRIB_HEADING_PSI(), 0.0f); // for the QUATERNION

    #ifdef LIN
    setStringProperty("pause_after_location_adjust", "false"); // v24.06.1 resolve Linux crash when compiling in release mode. Fail to read mxconst::get_ATTRIB_PAUSE_AFTER_LOCATION_ADJUST(), so I hardcoded the attribute name.
    #else
    setNumberProperty<bool>(mxconst::get_ATTRIB_PAUSE_AFTER_LOCATION_ADJUST(), false);
    #endif

    setStringProperty(mxconst::get_ELEMENT_DESC(), "Please add Briefer description!!!");
  }

}

bool
missionx::Briefer::parse_node()
{
  assert(!this->node.isEmpty());

  this->xLocationAdjust = this->node.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str());

  // read and set main briefer properties
  const std::string startICAO      = Utils::readAttrib(this->node, mxconst::get_ATTRIB_STARTING_ICAO(), "");
  const std::string startFlightLeg = Utils::readAttrib(this->node, mxconst::get_ATTRIB_STARTING_LEG(), mxconst::get_ATTRIB_STARTING_GOAL(), "", true); // v3.0.221.15rc5 compatible with new Leg and briefer attribute start_goal
  const std::string brieferDesc    = Utils::xml_read_cdata_node (this->node, "Please add Briefer description!!!");

  this->setStringProperty(mxconst::get_ATTRIB_STARTING_ICAO(), startICAO);
  this->setStringProperty(mxconst::get_ELEMENT_DESC(), brieferDesc, false);
  this->setStringProperty(mxconst::get_ATTRIB_STARTING_LEG(), startFlightLeg);

  // check values entered are enough for location adjustment
  if (!this->xLocationAdjust.isEmpty())
  {
    if ((Utils::readNodeNumericAttrib<double>(this->xLocationAdjust, mxconst::get_ATTRIB_LAT(), 0.0) != 0.0 && Utils::readNodeNumericAttrib<double>(this->xLocationAdjust, mxconst::get_ATTRIB_LONG(), 0.0) != 0.0) || Utils::readNodeNumericAttrib<double>(this->xLocationAdjust, mxconst::get_ATTRIB_HEADING_PSI(), -1.0) >= 0.0 ||
        Utils::readNodeNumericAttrib<double>(this->xLocationAdjust, mxconst::get_ATTRIB_STARTING_SPEED_MT_SEC(), -1.0) >= 0.0 // v3.0.241.3 fixed speed attribute
    )
      this->has_valid_location_adjust = true; //
  }


  // Print debug info
  Log::logMsg(this->to_string(), format_type::none, false);

  // validate Briefer
  const auto lmbda_validate_briefer = [&]() {
    // sanity validations
    if (startFlightLeg.empty())
    {

      Log::add_missionLoadError("[Critical] No starting Flight Leg was defined. Mission is not valid. Please fix the mission settings.");
      return false;
    }

    // validate briefer has correct starting point
    if (startICAO.empty() && !this->has_valid_location_adjust)
    {
      Log::add_missionLoadError("Briefer does not have enough information regarding starting position. Please check and fix <briefer> settings.");
      return  false;
    }

    return true;
  };

  return lmbda_validate_briefer();
}

void
missionx::Briefer::positionPlane(const bool inflag_setupForcePlanePositioning, const bool inflag_setupChangeHeadingEvenIfPlaneIn_20meter_radius, const int inXplaneVersion_i)
{
  std::string err;
  err.clear();

  const std::string ICAO = Utils::readAttrib(this->node, mxconst::get_ATTRIB_STARTING_ICAO(), ""); // v3.0.241.1  
  const int position_pref_i = Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_ATTRIB_POSITION_PREF(), 11); // v3.0.301 B4 if empty than default will be xp11

  // reading mission briefer location information
  const double lat1          = Utils::readNodeNumericAttrib<double>(this->node.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str()), mxconst::get_ATTRIB_LAT(), 0.0);                   // v3.0.241.1
  const double lon1          = Utils::readNodeNumericAttrib<double>(this->node.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str()), mxconst::get_ATTRIB_LONG(), 0.0);                  // v3.0.241.1
  const double elev_ft       = Utils::readNodeNumericAttrib<double>(this->node.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str()), mxconst::get_ATTRIB_ELEV_FT(), 0.0);               // v3.0.241.1
  const float  heading_psi_f = Utils::readNodeNumericAttrib<float>(this->node.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str()), mxconst::get_ATTRIB_HEADING_PSI(), 0.0f);                // v3.0.241.1
  const double startingSpeed = Utils::readNodeNumericAttrib<double>(this->node.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str()), mxconst::get_ATTRIB_STARTING_SPEED_MT_SEC(), 0.0); // v3.0.241.1
#ifdef IBM
  const bool   force_heading_b = Utils::readBoolAttrib(this->node.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str()), mxconst::get_ATTRIB_FORCE_HEADING_B(), false); // v3.0.301 B4
#else
  auto node_location_adjust = this->node.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str());
  const bool   force_heading_b = Utils::readBoolAttrib(node_location_adjust, mxconst::get_ATTRIB_FORCE_HEADING_B(), false); // v3.0.301 B4
#endif


  if (!ICAO.empty() || (lat1 != 0.0 && lon1 != 0.0)) // recognize the need to adjust location
  {
    missionx::Point p1;
    // Prepare probing object
     p1.resetData();
     p1.setLat(lat1);
     p1.setLon(lon1);
     p1.setElevationFt(elev_ft); // in feet will be translated automatically to meters too
     p1.setHeading(XPLMGetDataf(drefConst.dref_heading_psi_f)); // store current plane heading. this is not mission start heading
     p1.calcSimLocalData();

     const bool flag_is_plane_is_in_20Meter_from_starting_position = (missionx::dataref_manager::getCurrentPlanePointLocation().calcDistanceBetween2Points(p1, missionx::mx_units_of_measure::meter) > mxconst::TWNENTY_METERS_D);
     const bool shouldWeForceHeading_b                             = inflag_setupChangeHeadingEvenIfPlaneIn_20meter_radius + force_heading_b + flag_is_plane_is_in_20Meter_from_starting_position;


    // XPSDK300
    if (position_pref_i == 11 ) //&& inXplaneVersion_i < missionx::XP12_VERSION_NO)
    {
      missionx::Point plane = missionx::dataref_manager::getCurrentPlanePointLocation();
    
      if (!ICAO.empty())
        Utils::position_plane_in_ICAO(ICAO, (float)p1.getLat(), (float)p1.getLon(), (float)plane.getLat(), (float)plane.getLon());
    
      if ( lat1 * lon1 * (inflag_setupForcePlanePositioning + flag_is_plane_is_in_20Meter_from_starting_position) != 0) // if Zero then skip <location adjust>
      {       
        if (p1.getElevationInMeter() == 0.0)
        {
          {
            XPLMProbeResult       probeResult;
            [[maybe_unused]] auto probedElevInWorldCoordinates_mt = missionx::Point::getTerrainElevInMeter_FromPoint(p1, probeResult); // the function "getTerrainElevInMeter_FromPoint()" already set p1 elevation value, so we don't need to reassign it

          }

        }



        XPLMPlaceUserAtLocation(p1.getLat(), p1.getLon(), (float)(p1.getElevationInMeter() ), heading_psi_f, (float)startingSpeed);      
      }
    }
    else 
      positionPlane_v10(p1, shouldWeForceHeading_b);

    // reorienting plane if forced by the heading flag
    if (shouldWeForceHeading_b)
    { 
      setPlaneHeading(heading_psi_f);     
    }

  }


  // check if already in pause mode
  // if not, then trigger pause command
  if (!dataref_manager::isSimPause())
  {
    Briefer::need_to_pause_xplane = true;
  }


} // positionPlane

void
missionx::Briefer::setPlaneHeading(const float& inHeading)
{
  
    missionx::Utils::QUATERNION q;
    missionx::Utils::HPR        hpr;
    float                       q_orig[4];
    dataref_const               dm;

    XPLMGetDatavf(dm.dref_q, q_orig, 0, 4); // get original quaternion data
    Utils::convert_qArr_to_Quaternion(q_orig, q);

    hpr.Pitch   = dataref_manager::getDataRefValue<float>("sim/flightmodel/position/theta");
    hpr.Roll    = dataref_manager::getDataRefValue<float>("sim/flightmodel/position/phi");
    hpr.Heading = inHeading;

    Utils::HPRToQuaternion(hpr, &q);
    missionx::dataref_manager::setQuaternion((float)q.w, (float)q.x, (float)q.y, (float)q.z);

}



void
missionx::Briefer::positionPlane_v10(missionx::Point& inNewPosition, const bool inForceHeading_b)
{
  int                         heading_psi_i = Utils::readNodeNumericAttrib<int>(this->node.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str()), mxconst::get_ATTRIB_HEADING_PSI(), (int)XPLMGetDataf(drefConst.dref_heading_psi_f));
  const double                elev_ft     = Utils::readNodeNumericAttrib<double>(this->node.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str()), mxconst::get_ATTRIB_ELEV_FT(), 0.0); // v3.0.241.1
  XPLMProbeResult             probeResult;
  float                       q_orig[4]; // v2.1.26a8
  std::string                 STARTING_ICAO = Utils::readAttrib(this->node, mxconst::get_ATTRIB_STARTING_ICAO(), "");
  std::string                 ICAO = Utils::readAttrib(this->node, mxconst::get_ELEMENT_ICAO(), "");
  missionx::Utils::QUATERNION q;
  missionx::Utils::HPR        hpr;  
  dataref_const dm;
  missionx::Point             plane                           = missionx::dataref_manager::getCurrentPlanePointLocation();
  auto probedElevInWorldCoordinates_mt = missionx::Point::getTerrainElevInMeter_FromPoint(inNewPosition, probeResult);

  // decide which heading to use. If we do not force heading, then use current planes heading, if we force, then we use the mission heading
  hpr.Heading = (float)((inForceHeading_b * Utils::calc_heading_base_on_plane_move(plane.lat, plane.lon, inNewPosition.lat, inNewPosition.lon, heading_psi_i)) + ((float)(!inForceHeading_b) * XPLMGetDataf(drefConst.dref_heading_psi_f)) );

  double plane_dref_acf_h_eqlbm = (double)XPLMGetDataf(drefConst.dref_acf_h_eqlbm); // height plane from ground in meters

  snprintf(LOG_BUFF, sizeof(LOG_BUFF) - 1, "[positionPlane 10]  Before: probElevInWorldCoord meters: %f, New Pos elevation mt: %f, New Pos elevation ft: %f", 
           (float)probedElevInWorldCoordinates_mt
         , (float)inNewPosition.getElevationInMeter()
         , (float)inNewPosition.getElevationInFeet());
  
  Log::logMsg(LOG_BUFF);

  // Override Plane Path = disable physics 
  int override_planepath = { 1 };  // override user's plane path
  XPLMSetDatavi(dm.dref_override_planepath_i_arr, &override_planepath, 0, 1); // we only change the user's plane path so we just need one integer parameter

  bool         flag_used_elev_workaround = false;
  bool         flag_positioned_plane_in_ICAO = false;
  const double store_elev_mt_d = inNewPosition.getElevationInMeter(); // store elevation before moving plane. used only if we don't have ICAO or elevation is 0

  Utils::position_plane_in_ICAO(ICAO, (float)inNewPosition.getLat(), (float)inNewPosition.getLon(), (float)plane.getLat(), (float)plane.getLon(), true);

  
  if (ICAO.empty() || (elev_ft <= 0 && flag_positioned_plane_in_ICAO == false)) // this code might never be executed
  {
    inNewPosition.setElevationMt(10000.0);
    flag_used_elev_workaround = true;
  }

  inNewPosition.calcSimLocalData();

  // position plane in OpenGL world
  missionx::dataref_manager::setPlaneInLocalCoordiantes(inNewPosition.local_x, inNewPosition.local_y, inNewPosition.local_z);
  plane = missionx::dataref_manager::getCurrentPlanePointLocation(); // position after set plane

  if (flag_used_elev_workaround)
  {
    inNewPosition.setElevationMt(store_elev_mt_d);
  }

  // decide about the final plane position based on target terrain elevation + user request elemvation + plane height from ground
  probedElevInWorldCoordinates_mt = missionx::Point::getTerrainElevInMeter_FromPoint(plane, probeResult);
  if (probeResult == xplm_ProbeHitTerrain &&
      (flag_used_elev_workaround || inNewPosition.getElevationInMeter() < probedElevInWorldCoordinates_mt || (inNewPosition.getElevationInMeter() - (plane_dref_acf_h_eqlbm + probedElevInWorldCoordinates_mt) < 0.0)))
  {
    inNewPosition.setElevationMt(probedElevInWorldCoordinates_mt + plane_dref_acf_h_eqlbm);  
    inNewPosition.calcSimLocalData();
    // Position the plane
    missionx::dataref_manager::setPlaneInLocalCoordiantes(inNewPosition.local_x, inNewPosition.local_y, inNewPosition.local_z);

  }

  snprintf(LOG_BUFF, sizeof(LOG_BUFF)-1, "[positionPlane 10]  Before: probElevInWorldCoord meters: %f, New Pos elevation mt: %f, New Pos elevation ft: %f",
            (float)probedElevInWorldCoordinates_mt,
            (float)inNewPosition.getElevationInMeter(),
            (float)inNewPosition.getElevationInFeet());  

  Log::logMsg(LOG_BUFF);

  //// reorient the plane again since if we positioned first using ICAO code, then the plane heading will also be affected. So now we reorient the plane to its intended plugin heading.
  XPLMGetDatavf(dm.dref_q, q_orig, 0, 4); // get original quaternion data
  Utils::convert_qArr_to_Quaternion(q_orig, q);
  hpr.Pitch   = dataref_manager::getDataRefValue<float>("sim/flightmodel/position/theta");
  hpr.Roll    = dataref_manager::getDataRefValue<float>("sim/flightmodel/position/phi");
  //hpr.Heading = (float)heading_psi_i; // deprecated, we decide what will be the heading at the begining of this function
  Utils::HPRToQuaternion(hpr, &q);
  missionx::dataref_manager::setQuaternion((float)q.w, (float)q.x, (float)q.y, (float)q.z);

  override_planepath = 0;
  XPLMSetDatavi(dm.dref_override_planepath_i_arr, &override_planepath, 0, 1);
}
