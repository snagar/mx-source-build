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
  std::string     ways_around; // v3.0.253.4

  int         icao_id{ 0 }; // v3.303.8.3 holds icao_id from xp_airports sqlite DB
  std::string loc_desc, template_type, err;
  std::string flightLegName;
  std::string radius_mt_suggested_s;                                             // used in RandomEngine when we pick data from apt.dat or not. If from apt.dat then we should use ~40mt if not then empty and it wil default to 500mt
  bool        flag_picked_random_lat_long;                                       // v3.0.219.14 used when we want to flag special case of random coordinate. Can help with deciding if a template is medevac or not, and if you need heli or not.
  IXMLNode    xLegFromTemplate;                                                  // v3.0.221.2 holds template flight <leg> node.
  bool        flag_force_picked_same_point_template_as_flight_leg_template_type; // v3.0.221.15rc4
  bool flag_is_custom_scenery{ false }; // v3.0.253.6 used with apt.dat information when we want to pick airports around the plane, we can use the cached data to flag navaid as custom scenery based and therefore maybe to prefer it over generic one

  typedef struct _ramp_data // v3.0.253.1 added ramp specific info. lat.lon/heading will be kept in the main NavAid class.
  {
    std::string gate{ "" };
    std::string jets{ "" };    // who can park here
    std::string uq_name{ "" }; // unique name for this ramp
  } navAidRamp;
  navAidRamp ramp_info;

  // Random Engine Generator Helper. Not really part of the NavAidInfo class that needs to be stored in the Point or XML node.
  float bearing_next{ 0.0f };
  float bearing_to_current_target{ 0.0f };   // holds the bearing to reach this target from previous target
  float bearing_back_to_prev_target{ 0.0f }; // holds the bearing to the previous target. If bearing_relative_from_prev_target=10 degrease then bearing_back_to_prev_target=10+180

  // v25.06.1 ////////////////
  int fpln_seq{ -1 }; // v25.06.1 can hold a sequence number to be used with RandomEngin and Surprise Me flow.
  bool fpln_is_wet{ false };

  // Valid values: "land_hover", "land". Used with the osm_gen.xml nodes. Each <q> node should have "wp_type" attribute that hints the plugin how to deal with the wp.
  // "land_hover" will have two triggers based tasks that will allow you to either land or hover. "land" type will have one task based trigger, with smaller radius to land in.
  // As a rule of thumb, Odd sequence = "land_hover", while Even = "land". Example "seq=1" -> "land_hover", "seq=2" -> "land".
  std::string fpln_osm_wp_type;
  //NavAidInfo *prev_navaid{ nullptr };
  double fpln_distance_between_prev_and_current_navaid{ 0.0f };
  double fpln_distance_to_next_navaid{ 0.0f };
  IXMLNode fpln_xml_mission_leg_node; // holds a pointer to the XML node that represent this navaid. Example: trigger node.
  IXMLNode fpln_xml_inv_node; // holds the target inventory information during random engine mission generation
  IXMLNode fpln_xml_osm_q_node; // holds original osm_query <q> node

  ///// End v25.06.1

   missionx::NavAidInfo& operator= (const NavAidInfo &in_na)
   {
     if (this == &in_na)
       return *this;

     this->clone(in_na);

     return *this;
   }

   void clone (const missionx::NavAidInfo &in_na)
   {
     this->node = in_na.node.deepCopy();
     this->setID(in_na.ID);
     this->setName(in_na.name);
     this->setRegion (in_na.inRegion);

     this->setBaseNodeName( in_na.getBaseNodeName () );


     flag_is_skewed                           = in_na.flag_is_skewed;
     flag_is_brieferOrStartLocation           = in_na.flag_is_brieferOrStartLocation;
     flag_navDataFetchedFromDB                = in_na.flag_navDataFetchedFromDB;
     flag_navDataFetchedFromXPLMGetNavAidInfo = in_na.flag_navDataFetchedFromXPLMGetNavAidInfo;

     navRef  = in_na.navRef;
     navType = in_na.navType;
     freq    = in_na.freq;


     degRelativeToSearchPoint = in_na.degRelativeToSearchPoint;
     lat                      = in_na.lat;
     lon                      = in_na.lon;
     height_mt                = in_na.height_mt;
     heading                  = in_na.heading;

     p                    = in_na.p;
     xml_skewdPointNode   = in_na.xml_skewdPointNode.deepCopy ();
     xml_osm_around       = in_na.xml_osm_around.deepCopy ();
     flag_nav_from_webosm = in_na.flag_nav_from_webosm;
     ways_around          = in_na.ways_around;
     icao_id              = in_na.icao_id;

     loc_desc      = in_na.loc_desc;
     template_type = in_na.template_type;
     err.clear ();

     flightLegName         = in_na.flightLegName;
     radius_mt_suggested_s = in_na.radius_mt_suggested_s;


     flag_picked_random_lat_long                                       = in_na.flag_picked_random_lat_long;
     xLegFromTemplate                                                  = in_na.xLegFromTemplate;
     flag_force_picked_same_point_template_as_flight_leg_template_type = in_na.flag_force_picked_same_point_template_as_flight_leg_template_type;

     ramp_info.gate    = in_na.ramp_info.gate;
     ramp_info.jets    = in_na.ramp_info.jets;
     ramp_info.uq_name = in_na.ramp_info.uq_name;

     bearing_next                = in_na.bearing_next;
     bearing_to_current_target   = in_na.bearing_to_current_target;
     bearing_back_to_prev_target = in_na.bearing_back_to_prev_target;

     fpln_is_wet = in_na.fpln_is_wet; // v25.06.1
     fpln_seq = in_na.fpln_seq; // v25.06.1
     fpln_osm_wp_type = in_na.fpln_osm_wp_type; // v25.06.1
     //prev_navaid = in_na.prev_navaid; // v25.06.1
     fpln_xml_mission_leg_node = in_na.fpln_xml_mission_leg_node; // v25.06.1
     fpln_xml_osm_q_node = in_na.fpln_xml_osm_q_node.deepCopy ();

     this->synchToPoint ();
   }

  ~NavAidInfo() {}

  NavAidInfo() { init(); }
  NavAidInfo(const float &inLat, const float &inLon, const float inElevMt)
  {
    this->lat  = inLat;
    this->lon       = inLon;
    this->height_mt = inElevMt;
  }
  NavAidInfo(const missionx::NavAidInfo &in_na) : mx_base_node(in_na)
   {
     this->clone(in_na);
     this->synchToPoint ();
   }

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
    xLegFromTemplate                                                  = IXMLNode::emptyIXMLNode; // v3.0.221.2
    flag_force_picked_same_point_template_as_flight_leg_template_type = false;                    // v3.0.221.15rc4

    xml_skewdPointNode = IXMLNode::emptyIXMLNode; // v3.0.241.8
    xml_osm_around     = IXMLNode::emptyIXMLNode; // v3.0.253.4
    flag_is_skewed     = false;

    bearing_next              = 0.0f;
    bearing_to_current_target = 0.0f;

    flag_nav_from_webosm   = false;
    flag_is_custom_scenery = false;

    flag_is_brieferOrStartLocation = false;

    flag_navDataFetchedFromDB = false; // v24.03.1
    flag_navDataFetchedFromXPLMGetNavAidInfo = false; // v24.03.1

    fpln_seq = -1; // v25.06.1
    fpln_osm_wp_type.clear (); // v25.06.1
    //prev_navaid = nullptr; // v25.06.1
    fpln_xml_mission_leg_node = IXMLNode::emptyIXMLNode; // v25.06.1
    fpln_xml_osm_q_node = IXMLNode::emptyIXMLNode; // v25.06.1

  }

  // -----------------------------------

  std::string parse_ways_around(const IXMLNode& inOSM = IXMLNode::emptyIXMLNode)
  {
    if (inOSM.isEmpty() && this->xml_osm_around.isEmpty())
      return "";

    if (!inOSM.isEmpty())
      this->xml_osm_around = inOSM.deepCopy();

    if (!this->xml_osm_around.isEmpty())
    {
      std::map<std::string, std::string> mapStreets;
      const int                          nWays = this->xml_osm_around.nChildNode ("way");
      for (int i1 = 0; i1 < nWays; ++i1)
      {
        auto w = this->xml_osm_around.getChildNode("way", i1);
        for (int i2 = 0; i2 < w.nChildNode("tag"); ++i2)
        {
          bool              bFound = false;
          auto              nTag   = w.getChildNode("tag", i2);
          const std::string key    = Utils::xml_get_attribute_value(nTag, mxconst::get_ATTRIB_OSM_KEY(), bFound);
          const std::string val    = Utils::xml_get_attribute_value(nTag, mxconst::get_ATTRIB_OSM_VALUE(), bFound);

          if (mxconst::get_ATTRIB_NAME() == key && !val.empty())
            Utils::addElementToMap(mapStreets, val, val);

        } // end loop over all <tag> sub elements

        // construct the streets string
        bool bFirstTime = true;
        for (const auto &key_name : mapStreets | std::views::keys)
        {
          if (bFirstTime)
            this->ways_around = key_name;
          else
            this->ways_around += ", " + key_name;

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

  std::string get_name_or_icao_id()
  {
    if (std::string(this->name).empty())
      return getID();

    return {this->name};
  }

  std::string getRampInfo()
   {
     return std::format("Gate Type: {}, For planes: {}, ramp name: {}", this->ramp_info.gate, this->ramp_info.jets, this->ramp_info.uq_name);
   }

  std::string get_loc_desc() const
   {
     return this->loc_desc;
   }

  bool nav_aid_has_unique_name ()
   {
     const bool has_coordinate_in_name = mxUtils::find_text (getNavAidName (), "coordinate", false) != std::string::npos;
     const bool has_leg_string_in_name = mxUtils::find_text (getNavAidName (), mxconst::get_ELEMENT_LEG (), false) != std::string::npos;

     return !(this->getNavAidName ().empty () + has_coordinate_in_name + has_leg_string_in_name ); // Logical OR. Unique name means we do not have "coordinates" not "leg" in the navaid name.
   };


  std::string init_locDesc()
  {
     std::string loc_desc;
     const bool  flag_navaid_has_unique_name = nav_aid_has_unique_name (); // v25.06.1 check if the name does not have "coordinate" or "leg" text in it.
     if ((getID().empty () && getName().empty () || !flag_navaid_has_unique_name))
       this->loc_desc = std::format("{}: [{:.4f}, {:.4f}]", ((this->flag_nav_from_webosm) ? "osmweb": "coordinates"), this->lat, this->lon);
     else if (getID ().empty ())
       this->loc_desc = getName ();
     else
       this->loc_desc = std::format ("{}({})", getName (), getID ());

     if (this->height_mt != 0.0f)
     {
       const float height_ft = height_mt * missionx::meter2feet;
       this->loc_desc.append ( std::format (" (elevation: ~{}ft)", Utils::formatNumber<float> (height_ft, 0) ) );
     }

     return this->loc_desc;
  }


  // The function won't modify the "loc_desc" in NavAidInfo class. Will only return the formated string
  std::string gen_locDesc_short() // v3.0.241.9 will be used in briefer
  {
    std::string loc_desc_short;
    const bool flag_navaid_has_unique_name = nav_aid_has_unique_name(); // v25.06.1
    // v25.06.1 added unique name check
    if (this->getID ().empty() && (this->getName ().empty() || mxconst::get_COORDINATES_IN_THE_GPS_S() == name || !flag_navaid_has_unique_name))
    {
      if (this->loc_desc.empty() || !flag_navaid_has_unique_name) // v3.0.241.10 b3 extended to have better description
        loc_desc_short = std::format("{}: [{:.4f}, {:.4f}]", ((this->flag_nav_from_webosm) ? "osmweb": "coordinates"), this->lat, this->lon);
        // loc_desc_short = ((this->flag_nav_from_webosm) ? "osmweb: (" : "XY: (") + Utils::formatNumber<float>(this->lat, 4) + ", " + Utils::formatNumber<float>(this->lon, 4) + ")"; // v3.0.253.6 added flag_nav_from_webosm check to better display origin of data
      else
        loc_desc_short = std::format("{} ({:.4f}, {:.4f})", this->loc_desc, this->lat, this->lon);
        // loc_desc_short += " (" + Utils::formatNumber<float>(this->lat, 4) + ", " + Utils::formatNumber<float>(this->lon, 4) + ")";


    }
    else if (this->getID ().empty())
      loc_desc_short = this->getName ();
    else
      loc_desc_short = this->getName () + "(" + this->getID () + ")";

    return loc_desc_short;
  }


  void setID(const std::string& inVal)
  {

    #ifdef IBM
    strncpy_s(this->ID, 63, inVal.c_str(), 63);
    #else
    std::strncpy(this->ID, inVal.c_str(), 63);
    #endif
  }


  void setName(const std::string& inVal)
  {
    #ifdef IBM
    strncpy_s(this->name, 250, inVal.c_str(), 250);
    #else
    std::strncpy(this->name, inVal.c_str(), 250);
    #endif
  }

  void setRegion(const std::string& inVal)
  {
    #ifdef IBM
    strncpy_s(this->inRegion, 1, inVal.c_str(), 1);
    #else
    std::strncpy(this->inRegion, inVal.c_str(), 1);
    #endif
  }

  std::string getNavAsAptRampCode_1300() const { return mxconst::get_APT_1300_RAMP_CODE_v11_SPACE() + mxUtils::formatNumber<double>(this->lat, 8) + mxconst::get_SPACE() + mxUtils::formatNumber<double>(this->lon, 8); }

  void synchToPoint()
  {
    if (this->loc_desc.empty()) // v3.0.221.10
      init_locDesc();            // this init the loc_desc attribute

    p = missionx::Point(lat, lon);
    p.setElevationMt(height_mt);
    p.setHeading(heading);
    p.setName((flightLegName.empty() ? this->get_name_or_icao_id() : flightLegName)); // v3.0.253.7 try to make sure we have name // v3.0.241.8 added set Name function to Point class
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
    p.setNodeStringProperty( mxconst::get_PROP_IS_WET(), (fpln_is_wet)?"yes" : "");  // v25.06.1


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
      node.updateAttribute((flightLegName.empty() ? this->get_name_or_icao_id() : flightLegName).c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str()); // v3.0.219.11 goal name

      node.updateAttribute(this->radius_mt_suggested_s.c_str(), mxconst::get_ATTRIB_RADIUS_MT().c_str(), mxconst::get_ATTRIB_RADIUS_MT().c_str());                                                            // v3.0.219.12+ add suggested radius_mt
      node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_picked_random_lat_long).c_str(), mxconst::get_ATTRIB_IS_RANDOM_COORDINATES().c_str(), mxconst::get_ATTRIB_IS_RANDOM_COORDINATES().c_str()); // v3.0.219.14
      node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_is_skewed).c_str(), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B().c_str(), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B().c_str());                // v3.0.241.8 Adds skewed data to point

      node.updateAttribute(mxUtils::formatNumber<int>(this->icao_id).c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str()); // v3.303.8.3

    // v3.303.10
      if (this->flag_is_brieferOrStartLocation)
        node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_is_brieferOrStartLocation).c_str(), mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B().c_str(), mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B().c_str()); // v3.0.303.10

      node.updateAttribute( (fpln_is_wet)?"yes" : "", mxconst::get_PROP_IS_WET().c_str (), mxconst::get_PROP_IS_WET().c_str ());  // v25.06.1

    }
  }

  void syncPointToNav()
  {
    // this->NavAidInfo::NavAidInfo();
    lat           = static_cast<float> (p.getLat ());
    lon           = static_cast<float> (p.getLon ());
    heading       = static_cast<float> (p.getHeading ());
    height_mt     = static_cast<float> (p.getElevationInMeters ());
    template_type = Utils::readAttrib(p.node, mxconst::get_ATTRIB_TEMPLATE(), "");                                   // v3.0.241.8
    flightLegName = p.getName();                                                                               // v3.0.241.8 added getName() function to Point class
    this->setName(flightLegName);                                                                              // v3.0.253.7
    radius_mt_suggested_s       = Utils::readAttrib(p.node, mxconst::get_ATTRIB_RADIUS_MT(), "");                    // v3.0.241.8 
    flag_picked_random_lat_long = Utils::readBoolAttrib(p.node, mxconst::get_ATTRIB_IS_RANDOM_COORDINATES(), false); // v3.0.241.8  // v3.0.219.14
    flag_is_skewed              = Utils::readBoolAttrib(p.node, mxconst::get_ATTRIB_IS_SKEWED_POSITION_B(), false);  // v3.0.241.8 Adds skewed data to Nav from Point
    icao_id                     = Utils::readNodeNumericAttrib<int>(p.node, mxconst::get_ATTRIB_ICAO_ID(), 0); // v3.303.8.3
    flag_is_brieferOrStartLocation = Utils::readBoolAttrib(p.node, mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B(), false); // v3.303.10

    fpln_is_wet = Utils::readBoolAttrib(p.node,mxconst::get_PROP_IS_WET (), false); // v25.06.1

    this->setID(Utils::readAttrib(p.node, mxconst::get_ELEMENT_ICAO(), "")); // store icao
    if (p.node.isAttributeSet (mxconst::get_ATTRIB_LOC_DESC().c_str()))
    {
      std::string p_loc_desc = Utils::readAttrib(p.node, mxconst::get_ATTRIB_LOC_DESC(), ""); // v3.0.241.8
      // if (err.empty() && !p_loc_desc.empty())
      if (!p_loc_desc.empty())
        this->loc_desc = p_loc_desc;
    }
    // v3.0.255.4
    navRef  = static_cast<int> (Utils::readNumericAttrib (p.node, mxconst::get_ATTRIB_NAVREF (), XPLM_NAV_NOT_FOUND));
    navType = static_cast<int> (Utils::readNumericAttrib (p.node, mxconst::get_ATTRIB_NAV_TYPE (), xplm_Nav_Unknown));

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
      node.updateAttribute((this->flightLegName.empty() ? this->get_name_or_icao_id() : flightLegName).c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str()); // v3.0.219.11 goal name

      node.updateAttribute(this->radius_mt_suggested_s.c_str(), mxconst::get_ATTRIB_RADIUS_MT().c_str(), mxconst::get_ATTRIB_RADIUS_MT().c_str());                                                            // v3.0.219.12+ add suggested radius_mt
      node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_picked_random_lat_long).c_str(), mxconst::get_ATTRIB_IS_RANDOM_COORDINATES().c_str(), mxconst::get_ATTRIB_IS_RANDOM_COORDINATES().c_str()); // v3.0.219.14

      node.updateAttribute(this->ID, mxconst::get_ELEMENT_ICAO().c_str(), mxconst::get_ELEMENT_ICAO().c_str()); // v3.0.221.7 add the icao to the point. We will add this to the GPS too before the point lat/lon
      node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_is_skewed).c_str(), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B().c_str(), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B().c_str()); // v3.0.241.8 Adds skewed data to point

      node.updateAttribute(mxUtils::formatNumber<int>(this->icao_id).c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str()); // v3.303.8.3
                                                                                                                                                 // v3.303.10
      if (this->flag_is_brieferOrStartLocation)
        node.updateAttribute(mxUtils::formatNumber<bool>(this->flag_is_brieferOrStartLocation).c_str(), mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B().c_str(), mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B().c_str()); // v3.0.303.10

      node.updateAttribute( (fpln_is_wet)?"yes" : "", mxconst::get_PROP_IS_WET().c_str (), mxconst::get_PROP_IS_WET().c_str ());  // v25.06.1
    }
  }

  void syncXmlPointToNav()
  {
    if (this->node.isEmpty())
      return;

    // this->NavAidInfo::NavAidInfo();

    // bool flag_found = false;
    this->lat       = static_cast<float> (Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_LAT (), 0.0));
    this->lon       = static_cast<float> (Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_LONG (), 0.0));
    float elev_ft   = static_cast<float> (Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_ELEV_FT (), 0.0));
    this->height_mt = (float)(elev_ft * missionx::feet2meter);

    this->heading       = static_cast<float> (Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_HEADING_PSI (), 0.0));
    this->template_type = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TEMPLATE(), "");

    this->flightLegName = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");
    this->setName(flightLegName); // v3.0.253.7

    this->radius_mt_suggested_s = Utils::readAttrib(this->node, mxconst::get_ATTRIB_RADIUS_MT(), "");

    this->flag_picked_random_lat_long = static_cast<bool> (Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_IS_RANDOM_COORDINATES (), 0.0)); // v3.0.219.14
    this->setID(Utils::readAttrib(this->node, mxconst::get_ELEMENT_ICAO(), ""));                                            // // v3.0.221.7 store icao
    this->loc_desc       = Utils::readAttrib(this->node, mxconst::get_ATTRIB_LOC_DESC(), this->loc_desc);                             // v3.0.221.10
    this->flag_is_skewed = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_IS_SKEWED_POSITION_B(), false);                      // v3.0.241.8 Adds skewed data to Nav from Point
    // v3.0.255.4
    navRef  = static_cast<int> (Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_NAVREF (), XPLM_NAV_NOT_FOUND));
    navType = static_cast<int> (Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_NAV_TYPE (), xplm_Nav_Unknown));
    // v3.303.8.3
    icao_id = Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_ATTRIB_ICAO_ID(), 0); // v3.303.8.3
    // v3.303.10
    flag_is_brieferOrStartLocation = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_IS_BRIEFER_OR_START_LOCATION_B(), false); // v3.303.10

    this->fpln_is_wet = Utils::readBoolAttrib(this->node, mxconst::get_PROP_IS_WET(), false);  // v25.06.1


    p = Point(lat, lon);
    p.setElevationMt(height_mt);
    p.setHeading(heading);
    // p.setName(flightLegName);
    p.setName((flightLegName.empty() ? this->get_name_or_icao_id() : flightLegName)); // v3.0.253.7 try to make sure we have name // v3.0.241.8 added set Name function to Point class

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

    p.setNodeStringProperty( mxconst::get_PROP_IS_WET(), (this->fpln_is_wet)?"yes" : "");  // v25.06.1
  }
};

}
#endif
