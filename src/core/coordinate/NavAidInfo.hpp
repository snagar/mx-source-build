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
  bool flag_is_same_as_start_location; // v29.09.2 used when "location type" or "template" are equal "start".
  bool flag_fetched_from_db; // v24.03.1
  bool flag_fetched_from_webosm{ false };
  bool flag_navDataFetchedFromXPLMGetNavAidInfo; // v24.03.1
  bool flag_picked_random_lat_long; // v3.0.219.14 used when we want to flag special case of random coordinate. Can help with deciding if a template is medevac or not, and if you need heli or not.

  XPLMNavRef  navRef;                      // XPSDK type
  XPLMNavType navType{ xplm_Nav_Unknown }; // XPSDK type
  int         freq;
  char        ID[64];
  char        name[256];
  // char        inRegion = 0;

  float degRelativeToSearchPoint; // degrees // bearingRelativeToSearchPoint;
  float lat, lon, height_mt, heading;

  missionx::Point p;                  // will hold lat/long/elev/heading_psi
  missionx::mxVec2d skewed_location;

  IXMLNode        xml_skewdPointNode; // will hold the skewed lat/long/elev/heading_psi, to calculate during "inject Message function"
  IXMLNode        xml_osm_around;     // v3.0.253.4 holds street around locations
  std::string     ways_around; // v3.0.253.4

  int         icao_id{ 0 }; // v3.303.8.3 holds icao_id from xp_airports sqlite DB
  std::string loc_desc, template_type, err;
  std::string flightLegName;
  std::string radius_mt_suggested_s;                                             // used in RandomEngine when we pick data from apt.dat or not. If from apt.dat then we should use ~40mt if not then empty and it wil default to 500mt
  IXMLNode    xLegFromTemplate;                                                  // v3.0.221.2 holds template flight <leg> node.
  bool        flag_force_picked_same_point_template_as_flight_leg_template_type; // v3.0.221.15rc4
  bool flag_is_custom_scenery{ false }; // v3.0.253.6 used with apt.dat information when we want to pick airports around the plane, we can use the cached data to flag navaid as custom scenery based and therefore maybe to prefer it over generic one

  struct navAidRamp // v3.0.253.1 added ramp specific info. lat.lon/heading will be kept in the main NavAid class.
  {
    float lat {0.0f};
    float lon {0.0f};
    float heading {0.0f};
    std::string gate;
    std::string ramp_for_planes;    // who can park here
    std::string uq_name; // unique name for this ramp
    std::string operation_type; // type of operation
    std::string ramp_width_code; // icao_width_code
  };
  navAidRamp ramp_info;

  // Random Engine Generator Helper. Not really part of the NavAidInfo class that needs to be stored in the Point or XML node.
  float bearing_next{ 0.0f };
  float bearing_to_current_target{ 0.0f };   // holds the bearing to reach this target from previous target
  float bearing_back_to_prev_target{ 0.0f }; // holds the bearing to the previous target. If bearing_relative_from_prev_target=10 degrease then bearing_back_to_prev_target=10+180

  // v25.06.1 ////////////////
  bool   fpln_is_last_flight_leg{ false };
  int    fpln_seq{ -1 }; // v25.06.1 can hold a sequence number to be used with RandomEngin and Surprise Me flow.
  bool   fpln_is_wet{ false };
  bool   fpln_navaid_was_already_prepared{ false }; // v25.09.1 Will be used with the new Oilrig function. The briefer navaid is setup by the gen_oilrig_targets() function.
  double fpln_slope{ 0.0 }; // holds the expected slope at the target area

  // v25.09.1
  missionx::enums::mx_rnd_task_type            fpln_task_type{ missionx::enums::mx_rnd_task_type::undefined }; // high level type of mission: medevac or cargo
  missionx::enums::mx_user_picked_mission_type fpln_mission_type{ missionx::enums::mx_user_picked_mission_type::undefined }; // more detail mission type, medevac/oilrig_medevac/oilrig_cargo
  missionx::enums::mx_rnd_mission_phase        fpln_mission_phase{ missionx::enums::mx_rnd_mission_phase::undefined }; // start/land_target/land_extraction

  bool fpln_is_oilrig{ false }; // v25.09.1

  // Valid values: "land_hover", "land". Used with the osm_gen.xml nodes. Each <q> node should have "wp_type" attribute that hints the plugin how to deal with the wp.
  // "land_hover" will have two triggers-based tasks that will allow you to either land or hover. "land" type will have one task based trigger, with smaller radius to land in.
  // As a rule of thumb, Odd sequence = "land_hover", while Even = "land". Example "seq=1" -> "land_hover", "seq=2" -> "land".

  double fpln_distance_between_prev_and_current_navaid{ 0.0f };
  double fpln_distance_to_next_navaid{ 0.0f };
  std::string fpln_wp_template_type;
  std::string fpln_leg_name;
  IXMLNode fpln_xml_target_leg_node; // holds a pointer to the XML node that represents this navaid. Example: trigger node.
  IXMLNode fpln_xml_inv_node; // holds the target inventory information during random engine mission generation
  IXMLNode fpln_xml_osm_q_or_raw_tmpl_node; // holds original osm_query <q> node or from <template> file.
  IXMLNode fpln_xml_way_node; // holds <way> node result
  IXMLNode fpln_xml_q_tag_header_node; // holds <{topic}> node without the childs, at first
  ///// End v25.06.1

  // v25.09.1
  std::string sMetar;
  // v25.09.2
  std::string fpln_msg_text; // will be used to store flight leg message text, like the briefer's starting description that will be shown in the first leg.

  missionx::structs::strct_expected_location_data fpln_expected_location_data;
  bool fpln_copy_as_is_b {false}; // NOT IMPLEMENTED YET set of flight legs that will be stored in fpln_xml_target_leg_node and we will have to add them to the final mission leg list

  // used during <leg> creation
  std::vector<IXMLNode> fpln_leg_vec_trigger_nodes;
  std::vector<IXMLNode> fpln_leg_vec_task_nodes;
  IXMLNode              fpln_leg_objective_node;

  // v25.12.1 store guess/estimate vector data between two nodes in <way> to assist with 3D positioning.
  IXMLNode fpln_xml_next_node_to_find_vector; // keep history of the original node. We already calculated the vector.
  double   fpln_target_node_estimate_vector{0.0}; // holds the estimate vector.


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
     // this->setRegion (in_na.inRegion);

     this->setBaseNodeName( in_na.getBaseNodeName () );


     flag_is_skewed                           = in_na.flag_is_skewed;
     flag_is_brieferOrStartLocation           = in_na.flag_is_brieferOrStartLocation;
     flag_is_same_as_start_location           = in_na.flag_is_same_as_start_location;
     flag_fetched_from_db                     = in_na.flag_fetched_from_db;
     flag_fetched_from_webosm                 = in_na.flag_fetched_from_webosm;
     flag_navDataFetchedFromXPLMGetNavAidInfo = in_na.flag_navDataFetchedFromXPLMGetNavAidInfo;
     flag_picked_random_lat_long              = in_na.flag_picked_random_lat_long;

     navRef  = in_na.navRef;
     navType = in_na.navType;
     freq    = in_na.freq;


     degRelativeToSearchPoint = in_na.degRelativeToSearchPoint;
     lat                      = in_na.lat;
     lon                      = in_na.lon;
     height_mt                = in_na.height_mt;
     heading                  = in_na.heading;

     p                    = in_na.p;
     skewed_location      = in_na.skewed_location; // v25.09.2
     xml_skewdPointNode   = in_na.xml_skewdPointNode.deepCopy ();
     xml_osm_around       = in_na.xml_osm_around.deepCopy ();
     ways_around          = in_na.ways_around;
     icao_id              = in_na.icao_id;

     loc_desc      = in_na.loc_desc;
     template_type = in_na.template_type;
     err.clear ();

     flightLegName         = in_na.flightLegName;
     radius_mt_suggested_s = in_na.radius_mt_suggested_s;


     xLegFromTemplate                                                  = in_na.xLegFromTemplate;
     flag_force_picked_same_point_template_as_flight_leg_template_type = in_na.flag_force_picked_same_point_template_as_flight_leg_template_type;

     ramp_info.gate    = in_na.ramp_info.gate;
     ramp_info.ramp_for_planes    = in_na.ramp_info.ramp_for_planes;
     ramp_info.uq_name = in_na.ramp_info.uq_name;

     bearing_next                = in_na.bearing_next;
     bearing_to_current_target   = in_na.bearing_to_current_target;
     bearing_back_to_prev_target = in_na.bearing_back_to_prev_target;

     fpln_is_last_flight_leg         = in_na.fpln_is_last_flight_leg; // v25.06.1
     fpln_is_wet                     = in_na.fpln_is_wet; // v25.06.1
     fpln_seq                        = in_na.fpln_seq; // v25.06.1
     fpln_wp_template_type                    = in_na.fpln_wp_template_type; // v25.06.1
     fpln_leg_name                   = in_na.fpln_leg_name; // v25.06.1
     fpln_xml_target_leg_node        = in_na.fpln_xml_target_leg_node.deepCopy (); // v25.06.1
     fpln_xml_osm_q_or_raw_tmpl_node = in_na.fpln_xml_osm_q_or_raw_tmpl_node.deepCopy ();
     fpln_xml_inv_node               = in_na.fpln_xml_inv_node.deepCopy ();
     fpln_xml_way_node               = in_na.fpln_xml_way_node.deepCopy ();
     fpln_xml_q_tag_header_node      = in_na.fpln_xml_q_tag_header_node.deepCopy ();
     // v25.09.1
     fpln_is_oilrig                   = in_na.fpln_is_oilrig; // v25.09.1
     fpln_task_type                   = in_na.fpln_task_type; // v25.09.1
     fpln_mission_type                = in_na.fpln_mission_type; // v25.09.1
     fpln_mission_phase               = in_na.fpln_mission_phase; // v25.09.1
     fpln_navaid_was_already_prepared = in_na.fpln_navaid_was_already_prepared; // v25.06.1

     // used with Navaid info UI screen
     sMetar = in_na.sMetar; // v25.09.1

     // v25.09.2
     fpln_msg_text = in_na.fpln_msg_text;

     fpln_expected_location_data = in_na.fpln_expected_location_data;
     fpln_copy_as_is_b = in_na.fpln_copy_as_is_b;

     fpln_leg_vec_trigger_nodes = Utils::clone_xml_vector (in_na.fpln_leg_vec_trigger_nodes);
     fpln_leg_vec_task_nodes    = Utils::clone_xml_vector (in_na.fpln_leg_vec_task_nodes);
     fpln_leg_objective_node    = in_na.fpln_leg_objective_node.deepCopy ();

      // v25.12.1
     fpln_target_node_estimate_vector = in_na.fpln_target_node_estimate_vector;
     fpln_xml_next_node_to_find_vector = in_na.fpln_xml_next_node_to_find_vector.deepCopy ();

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
    // inRegion                        = 0;

    node.updateName(mxconst::get_ELEMENT_POINT().c_str()); // v3.303.11

    this->loc_desc.clear(); // = "lat/lon";
    this->template_type.clear();
    flightLegName.clear();

    radius_mt_suggested_s.clear();
    xLegFromTemplate                                                  = IXMLNode::emptyIXMLNode; // v3.0.221.2
    flag_force_picked_same_point_template_as_flight_leg_template_type = false;                    // v3.0.221.15rc4

    skewed_location    = { 0.0, 0.0 };
    xml_skewdPointNode = IXMLNode::emptyIXMLNode; // v3.0.241.8
    xml_osm_around     = IXMLNode::emptyIXMLNode; // v3.0.253.4
    flag_is_skewed     = false;

    bearing_next              = 0.0f;
    bearing_to_current_target = 0.0f;

    flag_fetched_from_webosm                 = false;
    flag_is_custom_scenery                   = false;
    flag_is_brieferOrStartLocation           = false;
    flag_is_same_as_start_location           = false;
    flag_fetched_from_db                     = false; // v24.03.1
    flag_navDataFetchedFromXPLMGetNavAidInfo = false; // v24.03.1
    flag_picked_random_lat_long              = false; // v3.0.219.14

    fpln_is_last_flight_leg = false; // v25.06.1
    fpln_is_wet             = false; // v25.06.1
    fpln_is_oilrig          = false; // v25.09.1
    fpln_seq                = -1; // v25.06.1
    fpln_wp_template_type.clear (); // v25.06.1
    fpln_leg_name.clear (); // v25.06.1
    fpln_xml_target_leg_node   = IXMLNode::emptyIXMLNode; // v25.06.1
    fpln_xml_osm_q_or_raw_tmpl_node        = IXMLNode::emptyIXMLNode; // v25.06.1
    fpln_xml_inv_node          = IXMLNode::emptyIXMLNode; // v25.06.1
    fpln_xml_way_node          = IXMLNode::emptyIXMLNode; // v25.06.1
    fpln_xml_q_tag_header_node = IXMLNode::emptyIXMLNode; // v25.06.1

     fpln_navaid_was_already_prepared = false; // v25.09.1
     fpln_task_type                   = missionx::enums::mx_rnd_task_type::undefined; // v25.09.1
     fpln_mission_type                = missionx::enums::mx_user_picked_mission_type::undefined; // v25.09.1
     fpln_mission_phase               = missionx::enums::mx_rnd_mission_phase::undefined; // v25.09.1
     fpln_navaid_was_already_prepared = false;

     sMetar.clear(); // v25.09.1

     // v25.09.2
     fpln_msg_text.clear();
     fpln_copy_as_is_b = false;
     fpln_expected_location_data.reset();
     fpln_leg_vec_trigger_nodes.clear ();
     fpln_leg_vec_task_nodes.clear ();
     fpln_leg_objective_node = IXMLNode::emptyIXMLNode;

     // v25.12.1
     fpln_target_node_estimate_vector = 0.0;
     fpln_xml_next_node_to_find_vector = IXMLNode::emptyIXMLNode;


  }

  // -----------------------------------
  
  bool is_lat_lon_valid()
  {
    return ( this->lat * this->lon != 0.0f );
  }

  // -----------------------------------

  bool is_navaid_valid (std::string &outErr) // v25.09.2
   {
     outErr.clear();
     if (! is_lat_lon_valid () )
       outErr.append ( fmt::format ("Navaid: {} ({}), has invalid coordinates.\nlat: {}, lon: {}\n", this->fpln_seq, this->get_loc_desc (), this->lat, this->lon) );

      return outErr.empty();
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
          // const std::string key    = Utils::xml_get_attribute_value(nTag, mxconst::get_ATTRIB_OSM_KEY(), bFound);
          // const std::string val    = Utils::xml_get_attribute_value(nTag, mxconst::get_ATTRIB_OSM_VALUE(), bFound);
          const std::string key    = Utils::readAttrib(nTag, mxconst::get_ATTRIB_OSM_KEY(), "");
          const std::string val    = Utils::readAttrib(nTag, mxconst::get_ATTRIB_OSM_VALUE(), "");

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

  std::string get_latLon() { return fmt::format("{:.8f},{:.8f}", lat, lon); }

  std::string get_latLon_short() { return fmt::format("{:.5f},{:.5f}", lat, lon); }

  std::string get_latLon_shortest() { return fmt::format("{:.3f},{:.3f}", lat, lon); } // v25.09.2

  std::string get_skewed_desc() { return fmt::format("{:.3f},{:.3f}", skewed_location.lat, skewed_location.lon); } // v25.09.2

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
     return fmt::format("Gate Type: {}, For planes: {}, Ramp name: [{}]", this->ramp_info.gate, this->ramp_info.ramp_for_planes, this->ramp_info.uq_name);
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


  // std::string init_locDesc()
  void init_locDesc()
  {
     const bool  flag_navaid_has_unique_name = nav_aid_has_unique_name (); // v25.06.1 check if the name does not have "coordinate" or "leg" text in it.
     if ( (getID().empty() && getName().empty() ) || !flag_navaid_has_unique_name)
     {
       if (this->is_lat_lon_valid ())
         this->loc_desc = fmt::format("{}: [{:.4f}, {:.4f}]", ((this->flag_fetched_from_webosm) ? "osmweb": "coordinates"), this->lat, this->lon);
       else
         this->loc_desc = "Navaid might be invalid.";
         // this->loc_desc = "Check the GPS for navigation guidance.";
     }
     else if (getID ().empty ())
       this->loc_desc = getName ();
     else
       this->loc_desc = fmt::format ("{}({})", getName (), getID ());

     if (this->height_mt != 0.0f)
     {
       const float height_ft = height_mt * missionx::meter2feet;
       this->loc_desc.append ( fmt::format (" (elevation: ~{}ft)", Utils::formatNumber<float> (height_ft, 0) ) );
     }
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
        loc_desc_short = fmt::format("{}: [{:.4f}, {:.4f}]", ((this->flag_fetched_from_webosm) ? "osmweb": "coordinates"), this->lat, this->lon);
        // loc_desc_short = ((this->flag_nav_from_webosm) ? "osmweb: (" : "XY: (") + Utils::formatNumber<float>(this->lat, 4) + ", " + Utils::formatNumber<float>(this->lon, 4) + ")"; // v3.0.253.6 added flag_nav_from_webosm check to better display origin of data
      else
        loc_desc_short = fmt::format("{} ({:.4f}, {:.4f})", this->loc_desc, this->lat, this->lon);
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

  // void setRegion(const char inVal)
  // {
  //   inRegion = inVal;
  // }

  std::string getNavAsAptRampCode_1300() const { return mxconst::get_APT_1300_RAMP_CODE_v11_SPACE() + mxUtils::formatNumber<double>(this->lat, 8) + mxconst::get_SPACE() + mxUtils::formatNumber<double>(this->lon, 8); }

  void synchToPoint(const bool &force_init_desc = false)
  {
    if (this->loc_desc.empty() || force_init_desc) // v3.0.221.10
    init_locDesc(); // Init the loc_desc attribute

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
    this->init_locDesc ();

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

    // this->init_locDesc ();

    // bool flag_found = false;
    this->lat       = static_cast<float> (Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_LAT (), 0.0));
    this->lon       = static_cast<float> (Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_LONG (), 0.0));
    const float elev_ft   = static_cast<float> (Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_ELEV_FT (), 0.0));
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

    this->init_locDesc ();
  }

static missionx::structs::strct_expected_location_data
parse_expected_location (const IXMLNode &in_xml_leg_from_template, const std::string &custom_error_message, const bool is_last_leg)
{
  missionx::structs::strct_expected_location_data data;

  //// PARSE EXPECTED LOCATION  ////
  IXMLNode xExpectedLocation = in_xml_leg_from_template.getChildNode (mxconst::get_ELEMENT_EXPECTED_LOCATION ().c_str ()).deepCopy (); // xLegFromTemplate.getChildNode (mxconst::get_ELEMENT_EXPECTED_LOCATION ().c_str ());
  if (xExpectedLocation.isEmpty ())
  {
    data.error = fmt::format("[{}] Failed to find: {}, while parsing {}. Please fix template.", __func__, mxconst::get_ELEMENT_EXPECTED_LOCATION (), custom_error_message);
    return data;
  }

  data.flag_force_template_distances_b = Utils::readBoolAttrib (xExpectedLocation, mxconst::get_ATTRIB_FORCE_TEMPLATE_DISTANCES_B (), false); // will be used in get_target() function to disable the "expected distance setup option".

  // if (location_type_s.empty())
  if (Utils::readAttrib (xExpectedLocation, mxconst::get_ATTRIB_LOCATION_TYPE (), "").empty ())
  {
    data.error = fmt::format ("[{}] {} node is empty.", __func__, mxconst::get_ELEMENT_EXPECTED_LOCATION ());
    return data;
  }

  data.location_type       = Utils::readAttrib (xExpectedLocation, mxconst::get_ATTRIB_LOCATION_TYPE (), "");
  data.location_properties_s = Utils::readAttrib (xExpectedLocation, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), mxconst::get_ATTRIB_LOCATION_VALUE (),  "", true);

  // location properties format can be: "{number}|{ramp type}|{min-max},..."
  // mapLocationValueOptions: {name},{value}.
  std::map<int, std::string> mapLocationValueOptions; // v3.0.221.7 will hold the complex options used by "|".
  std::string                location_value_min_max_distance_s, location_value_tag_name_s, location_value_poi_s; // v3.0.221.7 @Daikan used in Random airport pick. "location_value_tag_name_s" will be used to hold element name to search in template.

  ///////////// CHECK if LOCATION TYPE needs to be Randomized ////////////////
  // support for multi-location vecTypeValues type to pick
  if (data.location_type.find (mxconst::get_COMMA_DELIMITER ()) != std::string::npos)
  {
    std::vector<std::string> vecTypes      = mxUtils::split_v2 (data.location_type, mxconst::get_COMMA_DELIMITER ());
    std::vector<std::string> vecTypesProperties = mxUtils::split_v2 (data.location_properties_s, mxconst::get_COMMA_DELIMITER ());
    int                      picked        = 0;

    int       nTypes       = static_cast<int> (vecTypes.size ());
    const int nTypesProperties = static_cast<int> (vecTypesProperties.size ());

    Log::logDebugBO ("[DEBUG pick <leg> type & value] vecTypes: " + Utils::formatNumber<size_t> (vecTypes.size ()) + ", values:" + Utils::formatNumber<size_t> (vecTypesProperties.size ()), true);

    if (nTypes == 0)
    {
      data.error = "Found a location type with wrong definition. Check type properties. aborting !!!";
      return data;
    }

    if (nTypes == 1)
    {
      data.location_type = vecTypes.at (0);
      picked        = 0; // meaning first choice
    }
    else // random pick type
    {
      picked = Utils::getRandomIntNumber (0, nTypes - 1);
      if (picked > nTypes) // just in case
        picked = nTypes - 1;

      data.location_type = vecTypes.at (picked);
    }

    //// pick the property with the same pick index. picked can't be bigger than the number of types
    if (nTypesProperties >= nTypes || (picked < (nTypesProperties - 1)))
      data.location_properties_s = vecTypesProperties.at (picked);
    else if (nTypes >= 1 && nTypesProperties == 1) // if we have few Types but only 1 location_value_nm_s, then it is shared between all of them
      data.location_properties_s = vecTypesProperties.front ();
    else
      data.location_properties_s.clear ();

    if (data.location_properties_s == "_") // if special character that represents empty
      data.location_properties_s.clear ();
  }

  // The Random Engine needs the "location_properties_s" and "type" to find suitable target.
  // we will prepare all data that is necessary : string or container, and the code will have to pick the correct option from the "data" struct.
  data.flight_leg_type_hover_land_or_start = mxUtils::stringToLower (Utils::readAttrib (in_xml_leg_from_template, mxconst::get_ATTRIB_TEMPLATE (), EMPTY_STRING));
  // std::string       flight_leg_type_hover_land_or_start = data.flight_leg_type_hover_land_or_start;
  ///////////// CHECK if Flight Leg TYPE needs to be Randomized ////////////////
  if (data.flight_leg_type_hover_land_or_start.empty ())
  {
    data.reset ();
    data.error = "Found a <leg> template without type definition. skipping.";
    return data;
  }

  // support for "multi Leg type" to pick from
  if (data.flight_leg_type_hover_land_or_start.find (mxconst::get_COMMA_DELIMITER ()) != std::string::npos) // mxconst::get_COMMA_DELIMITER() = ","
  {
    const std::vector<std::string> vecTypes = mxUtils::split_v2 (data.flight_leg_type_hover_land_or_start, mxconst::get_COMMA_DELIMITER ()); // mxconst::get_COMMA_DELIMITER() = ","
    if (const int nTypes = static_cast<int> (vecTypes.size ())
      ; nTypes == 0)
    {
      data.reset ();
      data.error = "Found a <leg> template without type definition. skipping.";
      return data;
    }
    else if (nTypes == 1)
    {
      data.flight_leg_type_hover_land_or_start = vecTypes.at (0);
    }
    else // random pick type
    {
      int picked = Utils::getRandomIntNumber (0, nTypes - 1);
      if (picked > nTypes)
        picked = nTypes - 1;

      data.flight_leg_type_hover_land_or_start = vecTypes.at (picked);
    }
  }

  if ((mxconst::get_FL_TEMPLATE_VAL_START () == data.flight_leg_type_hover_land_or_start) || (data.location_type == mxconst::get_FL_TEMPLATE_VAL_START ())) // v3.0.221.15 consolidate if logic to one  // v3.0.221.7
  {
    data.flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_START ();
    data.location_type                       = mxconst::get_FL_TEMPLATE_VAL_START ();
    data.location_properties_s               = mxconst::get_FL_TEMPLATE_VAL_START ();
    data.mapLocationSplitPropertiesValues.clear ();
    data.vecLocationPropertiesSplit_vec.clear ();
  }
  ////////// Check if has special instructions like: "nm=20|ramp=H|nm_between=10-20|tag={some name}"
  else if (!data.location_properties_s.empty ())
  {
    //// v3.0.221.7 replace old logic with new more readable one
    // split between numbers and characters
    data.vecLocationPropertiesSplit_vec = mxUtils::split_v2 (data.location_properties_s, mxconst::get_PIPE_DELIMITER ()); // "|"

    for (const auto &v : data.vecLocationPropertiesSplit_vec)
    {
      std::vector<std::string> vecSplit = mxUtils::split_v2 (v, "=");
      if (auto size_i = vecSplit.size ()
        ; size_i == 1) // NO backwards compatibility.
      {
        std::string attribName = Utils::stringToLower (vecSplit.at (0));
        Log::logMsgErr (fmt::format ("[{}] Found location Property without explicit formating: {}={}. Skipping this directive.", __func__, attribName, "{missing value}" ) , true);
      }
      else if (size_i > 1)
      {
        std::string        attribName  = Utils::stringToLower (vecSplit.at (0));
        const std::string &attribValue = vecSplit.at (1);
        Utils::addElementToMap (data.mapLocationSplitPropertiesValues, attribName, attribValue);
      }
      else
        data.location_properties_s.clear ();
    } // end loop over split location_properties

    data.location_properties_s.clear ();

    // prepare local variables according to the split information
    const std::string local_location_value_min_max_distance_s = mxUtils::getValueFromElement (data.mapLocationSplitPropertiesValues, std::string ("nm_between"), std::string (""));
    if (!local_location_value_min_max_distance_s.empty ()) // min-max
    {
      const std::vector<double> vecMinMax = Utils::splitStringToNumbers<double> (local_location_value_min_max_distance_s, "-, ");
      for (size_t i1 = 0; i1 < vecMinMax.size (); ++i1)
      {
        switch (i1)
        {
          case 0:
            data.nm_between_min = static_cast<float>(vecMinMax.at(i1));
            data.mapLocationSplitPropertiesValues["min_distance_nm"] = fmt::format("{:.2f}", vecMinMax.at(i1) );
            break;
          case 1:
            data.nm_between_max = static_cast<float>(vecMinMax.at(i1));
            data.mapLocationSplitPropertiesValues["max_distance_nm"] = fmt::format("{:.2f}", vecMinMax.at(i1) );
            break;
          default:
            break;
        } // end switch
      }

      // Validate and fix if min > max
      if (data.nm_between_min >=0 && data.nm_between_max >=0 )
      {
        if (data.nm_between_min > data.nm_between_max)
          std::swap(data.nm_between_min, data.nm_between_max);
      }
    } // end "nm_between"

    // prepare local variables according to the split information
    if (Utils::isElementExists (data.mapLocationSplitPropertiesValues, "nm")) // represent distance in nm
      data.location_properties_s = data.mapLocationSplitPropertiesValues["nm"];

    // replace "_" with empty string
    if (data.location_properties_s == "_") // if special character that represent empty
      data.location_properties_s.clear ();

  }

  Log::logDebugBO ("[DEBUG pick template <leg> type] type picked: " + data.location_type, true);
  Log::logDebugBO ("[DEBUG random location info] location_value_nm_s=" + data.location_properties_s, true);

  return data;
}

void copy_target_nav_data_only (missionx::NavAidInfo &in_navaid)
{
  this->lat       = in_navaid.lat;
  this->lon       = in_navaid.lon;
  this->navRef    = in_navaid.navRef;
  this->heading   = in_navaid.heading;
  this->height_mt = in_navaid.height_mt;

  this->flag_fetched_from_db                     = in_navaid.flag_fetched_from_db;
  this->flag_fetched_from_webosm                 = in_navaid.flag_fetched_from_webosm;
  this->flag_navDataFetchedFromXPLMGetNavAidInfo = in_navaid.flag_navDataFetchedFromXPLMGetNavAidInfo;
  this->flag_picked_random_lat_long              = in_navaid.flag_picked_random_lat_long;

  // v26.03.1 copy ramp
  this->ramp_info = in_navaid.ramp_info;

  if (!in_navaid.getID ().empty () && this->getNavAidName ().empty ())
    this->setID (in_navaid.getID ());
  if (!in_navaid.getName ().empty () && this->getNavAidName ().empty ())
    this->setName (in_navaid.getName ());

  this->synchToPoint (true);
}

};

}
#endif
