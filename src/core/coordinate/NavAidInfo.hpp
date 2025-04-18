/*
 * NavAidInfo.h
 *
 *  Created on: Jan 29, 2019
 *      Author: snagar
 *
 */
#ifndef NAVAIDINFO_H_
#define NAVAIDINFO_H_


#include "../MxUtils.h"
#include "../xx_mission_constants.hpp"
#include "Point.hpp"
#include "../mx_base_node.h" // v3.303.11

namespace missionx
{
class NavAidInfo : public missionx::mx_base_node
{
private:

public:
  bool flag_is_skewed; // v3.0.241.8 we use this flag when we have skewdPointNode that is different than our "node" or "Point p"
  bool flag_is_brieferOrStartLocation; // v3.303.10
  bool flag_navDataFetchedFromDB; // v24.03.1
  bool flag_navDataFetchedFromXPLMGetNavAidInfo; // v24.03.1

  XPLMNavRef  navRef;                      // XPSDK type
  XPLMNavType navType{ xplm_Nav_Unknown }; // XPSDK type
  int         freq;
  char        ID[64];
  char        name[256];
  char        inRegion[1];

  float degRelativeToSearchPoint; // degrees // bearingRelativeToSearchPoint;
  float lat, lon, height_mt, heading;

  missionx::Point p;                  // will hold lat/long/elev/heading_psi
  IXMLNode        xml_skewdPointNode; // will hold the skewed lat/long/elev/heading_psi, to calculate during "inject Message function"
  IXMLNode        xml_osm_around;     // v3.0.253.4 holds street around locations
  bool            flag_nav_from_webosm{ false };
  std::string     ways_around{ "" }; // v3.0.253.4

  int         icao_id{ 0 }; // v3.303.8.3 holds icao_id from xp_airports sqlite DB
  std::string loc_desc, template_type, err;
  std::string flightLegName;
  std::string radius_mt_suggested_s;                                             // used in RandomEngine when we pick data from apt.dat or not. If from apt.dat then we should use ~40mt if not then empty and it wil default to 500mt
  bool        flag_picked_random_lat_long;                                       // v3.0.219.14 used when we want to flag special case of random coordinate. Can help with deciding if template is medevac or not and if need heli or not.
  IXMLNode    xLegFromTemplate;                                                  // v3.0.221.2 holds template flight <leg> node.
  bool        flag_force_picked_same_point_template_as_flight_leg_tempalte_type; // v3.0.221.15rc4
  bool flag_is_custom_scenery{ false }; // v3.0.253.6 used with apt.dat information when we want to pick airports around us we can use the cached data to flag navaid as custom scenery based and therefore maybe to prefare it over generic one

  typedef struct _ramp_data // v3.0.253.1 added ramp specific info. lat.lon/heading will be kept in the main NavAid class.
  {
    std::string gate{ "" };
    std::string jets{ "" };    // who can park here
    std::string uq_name{ "" }; // unique name for this ramp
  } navAidRamp;
  navAidRamp ramp_info;

  float bearing_next{ 0.0f };
  float bearing_to_current_target{ 0.0f };   // holds the bearing to reach this target from previous target
  float bearing_back_to_prev_target{ 0.0f }; // holds the bearing to the previous target. If bearing_relative_from_prev_target=10 degrease then bearing_back_to_prev_target=10+180



  ~NavAidInfo() {}

  NavAidInfo() { init(); }

  void init()
  {
    degRelativeToSearchPoint = -1.0f; // not set

    icao_id = 0;
    navRef  = XPLM_NAV_NOT_FOUND;
    navType = xplm_Nav_Unknown;
    lat = lon = height_mt = heading = 0.0f;
    freq                            = 0; // not set
    ID[0]                           = '\0';
    name[0]                         = '\0';
    inRegion[0]                     = '\0';

    node.updateName(mxconst::get_ELEMENT_POINT().c_str()); // v3.303.11


    this->loc_desc.clear(); // = "lat/lon";
    this->template_type.clear();
    flightLegName.clear();

    radius_mt_suggested_s.clear();
    flag_picked_random_lat_long                                       = false;                    // v3.0.219.14
    xLegFromTemplate                                                  = IXMLNode().emptyIXMLNode; // v3.0.221.2
    flag_force_picked_same_point_template_as_flight_leg_tempalte_type = false;                    // v3.0.221.15rc4

    xml_skewdPointNode = IXMLNode().emptyIXMLNode; // v3.0.241.8
    xml_osm_around     = IXMLNode().emptyIXMLNode; // v3.0.253.4
    flag_is_skewed     = false;

    bearing_next              = 0.0f;
    bearing_to_current_target = 0.0f;

    flag_nav_from_webosm   = false;
    flag_is_custom_scenery = false;

    flag_is_brieferOrStartLocation = false;

    flag_navDataFetchedFromDB = false; // v24.03.1
    flag_navDataFetchedFromXPLMGetNavAidInfo = false; // v24.03.1
  }

  // -----------------------------------

  std::string parse_ways_around(IXMLNode inOSM = IXMLNode().emptyIXMLNode)
  {
    if (inOSM.isEmpty() && this->xml_osm_around.isEmpty())
      return "";

    if (!inOSM.isEmpty())
      this->xml_osm_around = inOSM.deepCopy();

    if (!this->xml_osm_around.isEmpty())
    {
      std::map<std::string, std::string> mapStreets;
      int                                nWays = this->xml_osm_around.nChildNode("way");
      for (int i1 = 0; i1 < nWays; ++i1)
      {
        auto w = this->xml_osm_around.getChildNode("way", i1);
        for (int i2 = 0; i2 < w.nChildNode("tag"); ++i2)
        {
          bool              bFound = false;
          auto              nTag   = w.getChildNode("tag", i2);
          const std::string key    = Utils::xml_get_attribute_value(nTag, mxconst::get_ATTRIB_OSM_KEY(), bFound);
          const std::string val    = Utils::xml_get_attribute_value(nTag, mxconst::get_ATTRIB_OSM_VALUE(), bFound);

          if (mxconst::get_ATTRIB_NAME().compare(key) == 0 && !val.empty())
            Utils::addElementToMap(mapStreets, val, val);

        } // end loop over all <tag> sub elements

        // construct the streets string
        bool bFirstTime = true;
        for (auto& name : mapStreets)
        {
          if (bFirstTime)
            this->ways_around = name.first;
          else
            this->ways_around += ", " + name.first;

          bFirstTime = false;
        }
      }
    }

    return this->ways_around;
  } // parse_ways_around

  // -----------------------------------

  std::string getLat() { return Utils::formatNumber<float>(this->lat, 9); }

  std::string getLon() { return Utils::formatNumber<float>(this->lon, 9); }

  std::string get_latLon() { return this->getLat() + "," + this->getLon(); }

  std::string get_latLon_name() { return this->getLat() + ", " + this->getLon() + " ( " + this->getName() + " )"; }

  std::string getHeading_s() { return Utils::formatNumber<float>(this->heading, 2); }

  std::string getID() { return {this->ID}; }

  std::string getNavAidName() { return {this->name}; }

  std::string getName() { return {this->name}; }

  std::string getSomeName()
  {
    if (std::string(this->name).empty())
      return getID();

    return {this->name};
  }

  std::string getRampInfo() { return std::string("Gate Type: ") + this->ramp_info.gate + ", For planes: " + this->ramp_info.jets + ", ramp name: " + this->ramp_info.uq_name; }

  std::string get_locDesc()
  {
    if (std::string(ID).empty() && std::string(name).empty())
      this->loc_desc = "coordinates: [" + Utils::formatNumber<float>(this->lat, 6) + "," + Utils::formatNumber<float>(this->lon, 6) + "]";
    else if (std::string(ID).empty())
      this->loc_desc = std::string(name);
    else
      this->loc_desc = std::string(name) + "(" + std::string(ID) + ")";

    if (this->height_mt != 0.0f)
    {
      float height_ft = height_mt * missionx::meter2feet;
      loc_desc += " (elevation: ~" + Utils::formatNumber<float>(height_ft, 0) + "ft)";
    }

    return loc_desc;
  }

  std::string get_locDesc_short() // v3.0.241.9 will be used in briefer
  {
    if (std::string(ID).empty() && (std::string(name).empty() || mxconst::get_COORDINATES_IN_THE_GPS_S().compare(name) == 0))
    {
      if (loc_desc.empty() || Utils::stringToLower(loc_desc).find("coordinates") != std::string::npos) // v3.0.241.10 b3 extended to have better description
        this->loc_desc = ((this->flag_nav_from_webosm) ? "osmweb: (" : "XY: (") + Utils::formatNumber<float>(this->lat, 4) + ", " + Utils::formatNumber<float>(this->lon, 4) +
                         ")"; // v3.0.253.6 added flag_nav_from_webosm check to better display origin of data
      else
        this->loc_desc += " (" + Utils::formatNumber<float>(this->lat, 4) + ", " + Utils::formatNumber<float>(this->lon, 4) + ")";
    }
    else if (std::string(ID).empty())
      this->loc_desc = std::string(name);
    else
      this->loc_desc = std::string(name) + "(" + std::string(ID) + ")";

    return loc_desc;
  }


  void setID(std::string inVal)
  {

#ifdef IBM
    strncpy_s(this->ID, 63, inVal.c_str(), 63);
#else
    std::strncpy(this->ID, inVal.c_str(), 63);
#endif
  }


  void setName(std::string inVal)
  {
#ifdef IBM
    strncpy_s(this->name, 250, inVal.c_str(), 250);
#else
    std::strncpy(this->name, inVal.c_str(), 250);
#endif
  }

  std::string getNavAsAptRampCode_1300() { return mxconst::get_APT_1300_RAMP_CODE_v11_SPACE() + mxUtils::formatNumber<double>(this->lat, 8) + mxconst::get_SPACE() + mxUtils::formatNumber<double>(this->lon, 8); }

  void synchToPoint()
  {
    if (this->loc_desc.empty()) // v3.0.221.10
      get_locDesc();            // this init the loc_desc attribute

    p = missionx::Point(lat, lon);
    p.setElevationMt(height_mt);
    p.setHeading(heading);
    p.setName((flightLegName.empty() ? this->getSomeName() : flightLegName)); // v3.0.253.7 try to make sure we have name // v3.0.241.8 added set Name function to Point class
    p.setNodeStringProperty(mxconst::get_ATTRIB_ID(), this->ID); 
    p.setNodeProperty<int>(mxconst::get_ATTRIB_NAVREF(), this->navRef);               // v3.0.255.4
    p.setNodeProperty<int>(mxconst::get_ATTRIB_NAV_TYPE(), this->navType);            // v3.0.255.4
    p.setNodeStringProperty(mxconst::get_ATTRIB_RADIUS_MT(), radius_mt_suggested_s);  // v3.0.219.12+
    p.setNodeStringProperty(mxconst::get_ELEMENT_ICAO(), std::string(this->ID)); 
    p.setNodeStringProperty(mxconst::get_ATTRIB_LOC_DESC(), this->loc_desc);  // v3.0.221.10
    p.setNodeProperty<bool>(mxconst::get_ATTRIB_IS_RANDOM_COORDINATES(), flag_picked_random_lat_long);  // v3.0.241.8 //    
    p.setNodeProperty<bool>(mxconst::get_ATTRIB_IS_SKEWED_POSITION_B(), this->flag_is_skewed);  // v3.0.241.8 Adds skewed data to point
    // v3.303.8.3
    p.setNodeProperty<int>(mxconst::get_ATTRIB_ICAO_ID(), this->navType);                       
    // v3.303.10
    if (this->flag_is_brieferOrStartLocation)
      p.setNodeProperty<bool>(mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B(), this->flag_is_brieferOrStartLocation); 


    if (!node.isEmpty())
    {
      node.updateAttribute(mxUtils::formatNumber<double>(lat, 8).c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
      node.updateAttribute(mxUtils::formatNumber<double>(lon, 8).c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());
      node.updateAttribute(mxUtils::formatNumber<double>(p.getElevationInFeet()).c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str());

      node.updateAttribute(this->ID, mxconst::get_ELEMENT_ICAO().c_str(), mxconst::get_ELEMENT_ICAO().c_str()); // v3.0.221.7 ATTRIB_ID: add the icao to the point. We will add this to the GPS too before the point lat/lon

      // for FMS data
      node.updateAttribute(Utils::formatNumber<int>(this->navRef).c_str(), mxconst::get_ATTRIB_NAVREF().c_str(), mxconst::get_ATTRIB_NAVREF().c_str());      // v3.0.231.1
      node.updateAttribute(Utils::formatNumber<int>(this->navType).c_str(), mxconst::get_ATTRIB_NAV_TYPE().c_str(), mxconst::get_ATTRIB_NAV_TYPE().c_str()); // v3.0.231.1

      node.updateAttribute(loc_desc.c_str(), mxconst::get_ATTRIB_LOC_DESC().c_str(), mxconst::get_ATTRIB_LOC_DESC().c_str());
      node.updateAttribute(this->template_type.c_str(), mxconst::get_ATTRIB_TEMPLATE().c_str(), mxconst::get_ATTRIB_TEMPLATE().c_str());
      node.updateAttribute((flightLegName.empty() ? this->getSomeName() : flightLegName).c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str()); // v3.0.219.11 goal name

      node.updateAttribute(this->radius_mt_suggested_s.c_str(), mxconst::get_ATTRIB_RADIUS_MT().c_str(), mxconst::get_ATTRIB_RADIUS_MT().c_str());                                                            // v3.0.219.12+ add suggested radius_mt
      node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_picked_random_lat_long).c_str(), mxconst::get_ATTRIB_IS_RANDOM_COORDINATES().c_str(), mxconst::get_ATTRIB_IS_RANDOM_COORDINATES().c_str()); // v3.0.219.14
      node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_is_skewed).c_str(), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B().c_str(), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B().c_str());                // v3.0.241.8 Adds skewed data to point

      node.updateAttribute(mxUtils::formatNumber<int>(this->icao_id).c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str()); // v3.303.8.3

    // v3.303.10
      if (this->flag_is_brieferOrStartLocation)
        node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_is_brieferOrStartLocation).c_str(), mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B().c_str(), mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B().c_str()); // v3.0.303.10

    }
  }

  void syncPointToNav()
  {
    // this->NavAidInfo::NavAidInfo();
    lat           = (float)p.getLat();
    lon           = (float)p.getLon();
    heading       = (float)p.getHeading();
    height_mt     = (float)p.getElevationInMeter();
    template_type = Utils::readAttrib(p.node, mxconst::get_ATTRIB_TEMPLATE(), "");                                   // v3.0.241.8
    flightLegName = p.getName();                                                                               // v3.0.241.8 added getName() function to Point class
    this->setName(flightLegName);                                                                              // v3.0.253.7
    radius_mt_suggested_s       = Utils::readAttrib(p.node, mxconst::get_ATTRIB_RADIUS_MT(), "");                    // v3.0.241.8 
    flag_picked_random_lat_long = Utils::readBoolAttrib(p.node, mxconst::get_ATTRIB_IS_RANDOM_COORDINATES(), false); // v3.0.241.8  // v3.0.219.14
    flag_is_skewed              = Utils::readBoolAttrib(p.node, mxconst::get_ATTRIB_IS_SKEWED_POSITION_B(), false);  // v3.0.241.8 Adds skewed data to Nav from Point
    icao_id                     = Utils::readNodeNumericAttrib<int>(p.node, mxconst::get_ATTRIB_ICAO_ID(), 0); // v3.303.8.3
    flag_is_brieferOrStartLocation = Utils::readBoolAttrib(p.node, mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B(), false); // v3.303.10

    this->setID(Utils::readAttrib(p.node, mxconst::get_ELEMENT_ICAO(), "")); // store icao
    if (p.node.isAttributeSet (mxconst::get_ATTRIB_LOC_DESC().c_str()))
    {
      std::string p_loc_desc = Utils::readAttrib(p.node, mxconst::get_ATTRIB_LOC_DESC(), ""); // v3.0.241.8
      // if (err.empty() && !p_loc_desc.empty())
      if (!p_loc_desc.empty())
        this->loc_desc = p_loc_desc;
    }
    // v3.0.255.4
    navRef  = (int)Utils::readNumericAttrib(p.node, mxconst::get_ATTRIB_NAVREF(), XPLM_NAV_NOT_FOUND);
    navType = (int)Utils::readNumericAttrib(p.node, mxconst::get_ATTRIB_NAV_TYPE(), xplm_Nav_Unknown);

    if (!node.isEmpty())
    {
      node.updateAttribute(mxUtils::formatNumber<double>(lat, 8).c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
      node.updateAttribute(mxUtils::formatNumber<double>(lon, 8).c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());
      node.updateAttribute(mxUtils::formatNumber<double>(p.getElevationInFeet()).c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str());

      // for FMS data
      node.updateAttribute(Utils::formatNumber<int>(this->navRef).c_str(), mxconst::get_ATTRIB_NAVREF().c_str(), mxconst::get_ATTRIB_NAVREF().c_str());      // v3.0.231.1
      node.updateAttribute(Utils::formatNumber<int>(this->navType).c_str(), mxconst::get_ATTRIB_NAV_TYPE().c_str(), mxconst::get_ATTRIB_NAV_TYPE().c_str()); // v3.0.255.4

      node.updateAttribute(loc_desc.c_str(), mxconst::get_ATTRIB_LOC_DESC().c_str(), mxconst::get_ATTRIB_LOC_DESC().c_str());
      node.updateAttribute(this->template_type.c_str(), mxconst::get_ATTRIB_TEMPLATE().c_str(), mxconst::get_ATTRIB_TEMPLATE().c_str());
      node.updateAttribute((this->flightLegName.empty() ? this->getSomeName() : flightLegName).c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str()); // v3.0.219.11 goal name

      node.updateAttribute(this->radius_mt_suggested_s.c_str(), mxconst::get_ATTRIB_RADIUS_MT().c_str(), mxconst::get_ATTRIB_RADIUS_MT().c_str());                                                            // v3.0.219.12+ add suggested radius_mt
      node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_picked_random_lat_long).c_str(), mxconst::get_ATTRIB_IS_RANDOM_COORDINATES().c_str(), mxconst::get_ATTRIB_IS_RANDOM_COORDINATES().c_str()); // v3.0.219.14

      node.updateAttribute(this->ID, mxconst::get_ELEMENT_ICAO().c_str(), mxconst::get_ELEMENT_ICAO().c_str()); // v3.0.221.7 add the icao to the point. We will add this to the GPS too before the point lat/lon
      node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_is_skewed).c_str(), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B().c_str(), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B().c_str()); // v3.0.241.8 Adds skewed data to point

      node.updateAttribute(mxUtils::formatNumber<int>(this->icao_id).c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str()); // v3.303.8.3
                                                                                                                                                 // v3.303.10
      if (this->flag_is_brieferOrStartLocation)
        node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_is_brieferOrStartLocation).c_str(), mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B().c_str(), mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B().c_str()); // v3.0.303.10

    }
  }

  void syncXmlPointToNav()
  {
    if (this->node.isEmpty())
      return;

    // this->NavAidInfo::NavAidInfo();

    // bool flag_found = false;
    this->lat       = (float)Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_LAT(), 0.0);
    this->lon       = (float)Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_LONG(), 0.0);
    float elev_ft   = (float)Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ELEV_FT(), 0.0);
    this->height_mt = (float)(elev_ft * missionx::feet2meter);

    this->heading       = (float)Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_HEADING_PSI(), 0.0);
    this->template_type = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TEMPLATE(), EMPTY_STRING);

    this->flightLegName = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), EMPTY_STRING);
    this->setName(flightLegName); // v3.0.253.7

    this->radius_mt_suggested_s = Utils::readAttrib(this->node, mxconst::get_ATTRIB_RADIUS_MT(), EMPTY_STRING);

    this->flag_picked_random_lat_long = (bool)Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_IS_RANDOM_COORDINATES(), 0.0); // v3.0.219.14
    this->setID(Utils::readAttrib(this->node, mxconst::get_ELEMENT_ICAO(), EMPTY_STRING));                                            // // v3.0.221.7 store icao
    this->loc_desc       = Utils::readAttrib(this->node, mxconst::get_ATTRIB_LOC_DESC(), this->loc_desc);                             // v3.0.221.10
    this->flag_is_skewed = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_IS_SKEWED_POSITION_B(), false);                      // v3.0.241.8 Adds skewed data to Nav from Point
    // v3.0.255.4
    navRef  = (int)Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_NAVREF(), XPLM_NAV_NOT_FOUND);
    navType = (int)Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_NAV_TYPE(), xplm_Nav_Unknown);
    // v3.303.8.3
    icao_id = Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_ATTRIB_ICAO_ID(), 0); // v3.303.8.3
    // v3.303.10
    flag_is_brieferOrStartLocation = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B(), false); // v3.303.10



    p = Point(lat, lon);
    p.setElevationMt(height_mt);
    p.setHeading(heading);
    // p.setName(flightLegName);
    p.setName((flightLegName.empty() ? this->getSomeName() : flightLegName)); // v3.0.253.7 try to make sure we have name // v3.0.241.8 added set Name function to Point class

    // v3.0.241.8 convert to Node settings
    p.setNodeStringProperty(mxconst::get_ATTRIB_ID(), this->ID); 
    p.setNodeProperty<int>(mxconst::get_ATTRIB_NAVREF(), this->navRef);               // v3.0.255.4
    p.setNodeProperty<int>(mxconst::get_ATTRIB_NAV_TYPE(), this->navType);            // v3.0.255.4
    p.setNodeStringProperty(mxconst::get_ATTRIB_TEMPLATE(), this->template_type);     // v3.0.219.12+
    p.setNodeStringProperty(mxconst::get_ATTRIB_RADIUS_MT(), radius_mt_suggested_s);  // v3.0.219.12+
    p.setNodeStringProperty(mxconst::get_ELEMENT_ICAO(), std::string(this->ID)); 
    p.setNodeStringProperty(mxconst::get_ATTRIB_LOC_DESC(), this->loc_desc);  // v3.0.221.10
    p.setNodeProperty<bool>(mxconst::get_ATTRIB_IS_RANDOM_COORDINATES(), flag_picked_random_lat_long);  // v3.0.241.8
    p.setNodeProperty<bool>(mxconst::get_ATTRIB_IS_SKEWED_POSITION_B(), this->flag_is_skewed);  // v3.0.241.8 Adds skewed data to point

    p.setNodeProperty<int>(mxconst::get_ATTRIB_ICAO_ID(), this->navType);  // v3.303.8.3
    // v3.303.10
    if (this->flag_is_brieferOrStartLocation)
      p.setNodeProperty<bool>(mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B(), this->flag_is_brieferOrStartLocation); 
  }
};

}
#endif
