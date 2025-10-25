/*
 * RandomEngine.h
 *
 *  Created on: Dec 13, 2018
 *      Author: snagar

    Random Engine does not run on main thread.
    To make it compatible with Laminar XPSDK, we must make sure that all calls will be execute from Main thread and the result will be handled here.
    In this class case, I had to do an ugly workaround that will make the child thread wait for N milliseconds, and then handle the outcome.
    Why is it ugly ?
    Because we we are using shared parameters as the means to communicate between the threads.
    We use: base_thread.status_thread.pipeParameters to store lat/long or any string/number/bool but we also use
            _random_airport_info_struct: to store information when we need to call to get_random_airport(p1,p2,p3,p4,p5) function from main thread. But we also added to the struct -
            "NavAidInfo navAid" to hold information on the Navaid information returned from "get_random_airport()".
    The end result was:
    We use: gatherRandomAirport_mainThread(Point inPoint, float inMaxDistance_nm, std::string inRestrictRampType, int inExcludeAngle, float inStartFromDistance_nm) from Main thread to gather ALL navaids we will process in the thread. The
 parameters are passed from "_random_airport_info_struct" struct. We then call "getRandomAirport_localThread()" to process the information gathered in: "gatherRandomAirport_mainThread" and was stored in "mapNavAidsFromMainThread". We use:
 calc_slope_at_point_mainThread() to calculate slope of target point. we use threadState.pipeProperties to store lat/long of current target point coordinates. we retrieve from: "threadState.pipeProperties" ATTRIB_TERRAIN_SLOPE as result
 from main thread.


    The main function that "instructs" main thread is: "waitForPluginCallbackJob()". We send "missionx::mx_flc_pre_command:xxxx" that is handled by: "mission::flcPRE()" function.
    !!! The main problem with this flow of code is: "we take for granted that no one is going to touch our shared struct or "pipe". !!!

    but it works, and it confined to LR standard.

 */

#ifndef RANDOMENGINE_H_
#define RANDOMENGINE_H_

#include <map> // v3.0.219.2
#include <random>
#include <vector> // v3.0.219.1

#include "../core/coordinate/NavAidInfo.hpp"
#include "../core/data_manager.h"
#include "../core/thread/base_thread.hpp"
//#include "../io/system_actions.h"
// #include "../io/OptimizeAptDat.h"



namespace missionx
{

//////////////////////////////////////////////////////
class RandomEngine final : public base_thread
{
public:
  typedef struct _random_airport_info_struct
  {
    // convert icao to xml point
    IXMLNode parentNode_ptr; // pointer to XML element that might hold icao elements that needs to be converted to points

    Point p;
    bool  isWet{false}; // will hold wet status

    float       inMinDistance_nm{}; // v3.0.221.15
    float       inMaxDistance_nm{};
    int         inExcludeAngle{};
    float       inStartFromDistance_nm{};
    std::string inRestrictRampType;

    NavAidInfo navAid;

    _random_airport_info_struct() { init(); }

    void init()
    {
      p.init();
      isWet          = false;
      inExcludeAngle = -1;
      inRestrictRampType.clear();
      inMinDistance_nm       = 0.0f;
      inMaxDistance_nm       = static_cast<float>(mxconst::MAX_DISTANCE_TO_SEARCH_AIRPORT);
      inStartFromDistance_nm = mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT;
      navAid.init();
      parentNode_ptr = IXMLNode::emptyIXMLNode;
    }
  } strct_shared_random_airport_info;
  static strct_shared_random_airport_info shared_navaid_info;

private:
  // key=types value: string translation in lower case    
  static mx_plane_types_enum template_plane_type_enum; // holds the enum type. we can then use mapPlaneEnumToStringTypes to translate to string value

  static std::string                   translatePlaneTypeToString (mx_plane_types_enum in_plane_type);
  static missionx::mx_plane_types_enum translatePlaneTypeToEnum (const std::string &in_plane_type);


  std::string pathToRandomRootFolder;
  std::string pathToRandomBrieferFolder;

  bool                       flag_found;
  std::string                randomPlaneType; // v3.0.221.11
  static bool                is_plane_type_valid (const std::string &in_plane_type); // v25.09.2
  mx_plane_types_enum        setPlaneType (std::string inPlaneType);
  void                       setPlaneType (mx_plane_types_enum inPlaneType);
  static uint8_t             getPlaneType ();
  static mx_plane_types_enum getPlaneType_enum ();

  static std::string errMsg;

  ///// template stream and xml nodes
  //// Read XML mission file
  IXMLDomParser iDomTemplate;

  static IXMLNode xRootTemplate;

  //// Mission File stream and xml nodes
  IXMLNode xTargetMainNode; //=XMLNode::openFileHelper( std::string("save1.xml").c_str() );
  IXMLNode xDummyTopNode; // v3.0.253.12 renamed from xTargetTopNode to xDummyTopNode. The dummy node holds the <MISSION> element, but when we write to the file we will create the true Target node. This is done for filterring out cases,
  // like not writing the GPS element

  IXMLNode xMetadata;         // holds metadata information // v24.12.1
  IXMLNode xFlightLegs;       // holds Flight Legs
  IXMLNode xGlobalSettings;    // holds global settings information
  IXMLNode xScoring;          // holds the <scoring> sub element in global settings // v3.303.9
  IXMLNode xCompatibility;    // holds the <compatibility> sub element in global settings // v24.12.2
  IXMLNode xBrieferInfo;      // holds briefer element information
  IXMLNode xBriefer;          // holds briefer element information
  IXMLNode xObjectives;       // holds Objectives element information
  IXMLNode xTriggers;         // holds all triggers element information
  IXMLNode xInventoris;       // holds all inventories element information
  IXMLNode xMessages;         // holds all messages information
  IXMLNode xEnd;              // holds end element information
  IXMLNode xGPS;              // holds GPS coordinates
  IXMLNode xChoices;          // holds <choice> sub elements
  IXMLNode x3DObjTemplate;    // holds 3D Object Templates
  IXMLNode xpData;            // v3.0.221.10 holds Dataref list
  IXMLNode xEmbedScripts;     // v3.0.221.10 holds external or scriptlet directives
  static IXMLNode xDrefStartColdAndDark; // v25.10.1

  typedef struct _mission_xml_data
  {
    std::string currentLegName;
    _mission_xml_data() { init(); }
    void init() { currentLegName.clear(); }
  } mission_xml_data_struct;

  mission_xml_data_struct mission_xml_data; // del ?

  ///// end mission XML elements ////

  ///// Useful Parameters ////
  static missionx::Point planeLocation;

  static std::list<missionx::NavAidInfo> listNavInfo; // v3.0.219.6 // del ?

  // std::map<std::string, int> mapFlightPlanOrder_si; // del? // assist in constructing next goal. We add goal name and sequence numbering every time we add to "xFlightLegs"
  // std::map<int, std::string> mapFLightPlanOrder_is; // del? // assist in constructing next goal. We add sequence numbering and then the goal name.

  //// hidden function members
  static void setError(const std::string& inMsg);

  // static missionx::mxVec2d del_get_skewed_target_position (const missionx::Point &p); // del// v25.09.2 will return a skewed position in the ~0.5-3.0nm away from target.
  static IXMLNode gen_get_skewed_target_position (const IXMLNode &inRealTargetPositionPoint); // will return a skewed position in the ~0.5-3.0nm away from target.
  // static bool     parse_display_object_element (IXMLNode &inFlightLegNode, IXMLNode &inDisplayNode, IXMLNode &in_xRootTemplate, IXMLNode &x3DObjTemplate, double &expected_slope_at_target_location_d, std::string &inout_err); // check xml tag <display_object> for specific random attributes.
  static bool     parse_display_object_element (missionx::NavAidInfo *in_target_nav_ptr, IXMLNode &inFlightLegNode, IXMLNode &inDisplayNode, IXMLNode &in_xRootTemplate, IXMLNode &x3DObjTemplate, double &expected_slope_at_target_location_d, std::string &inout_err); // check xml tag <display_object> for specific random attributes.
  // del ?
  // static void     parse_3D_object_template_element (const IXMLNode &in_root_template_node, const IXMLNode &in_3d_obj_template_node, std::string &inout_err); // check xml tag <object_template> for specific random attributes.


  bool gen_read_mission_info_element();
  // bool prepareBrieferAndStartLocation();   // del      // assist in constructing a briefer element. Mission description will be fetched from "elementBrieferInfoProperties" which was initialized in "readMissionInfoElement()"
  // bool readFlightLegs_directlyFromTemplate(); // del // assist in constructing <leg> elements.

  bool flag_isLastFlightLeg;                  // v3.0.219.11 specifically for slope test in location_type="xy" and template_type="medevac". In last goal we skip this test, since we expect to land

  static IXMLNode get_content_story (const IXMLNode &xTemplateNode /*, std::string inTemplateType*/); // v3.0.219.14 Will try to parse and pick a background content story

  // del both functions
  // bool            extract_flight_leg_set (IXMLNode &inNodeTemplate, const IXMLNode &inSetNode, int &inCounter); // v3.0.219.14 Will try to parse and pick a background content story
  // bool            build_and_add_flight_leg_from_node (const IXMLNode &inNode, int &inCounter); // v3.0.219.14 Will try to parse and pick a background content story

  // bool        generateRandomMissionBasedOnContent (IXMLNode &xTemplateNode); // del // v3.0.219.13


  // IXMLNode buildFlightLeg(int inFlightLegCounter, const IXMLNode& in_legNodeFromTemplate); // del

  // void fill_up_next_leg_attrib_after_flight_plan_was_generated(); // del


  // static bool setInstanceProperties(IXMLNode& pNode, missionx::NavAidInfo& inTargetNavInfo, IXMLNode &inDummyTopNode, const bool &flag_isLastFlightLeg); // del
  // void injectMissionTypeFeatures(); // del
  // void injectMessagesWhileFlyingToDestination(); // del

  typedef enum _inv_source
  {
    point   = 1,
    trigger = 2
  } mxInvSource;

  // void addInventory (const std::string &inFlightLegName, const IXMLNode &inSourceNode, mxInvSource inSource = mxInvSource::trigger);

  // std::set<std::string> setInventories;

  // void check_validity_of_display_object_elements(const std::string &inSavePath, const IXMLNode parent, IXMLNode& nodeBriefer); // deprecated since we do not use. It was moved the data_manager class.
  bool writeTargetFile();

  // do a 360 swipe and pick Airports every 10deg and in distances of 10/20/30..Nth nautical miles. Store airport data in a map of <int, NavAidInfo>
  //void getRandomAirport_localThread(missionx::NavAidInfo& outNavAid, std::string inLocationType = EMPTY_STRING, std::string inRestrictRampType = missionx::EMPTY_STRING); // v3.0.221.15 rc3.5

  double expected_slope_at_target_location_d;

  static bool gen_get_is_navaid_in_a_valid_distance (const double &currentDistanceToTarget, const double &in_location_value_d, const double &in_location_minDistance_d, const double &in_location_maxDistance_d);
  void init();

  // void        addTriggersBasedOnTargetLocation (NavAidInfo &inNav, IXMLNode &inSpecialLegSubNode); // del
  void        gen_inject_countdown_timer ( const int &current_nav_index, std::map<int, missionx::NavAidInfo> &in_navaid_targets); // v25.10.1
  // void        gen_inject_countdown_timers (std::map<int, missionx::NavAidInfo> &in_navaid_targets, IXMLNode &in_xGlobalSettings, IXMLNode &in_xFlightLegs); // v25.10.1
  // void        injectCountdownTimers ();
  static bool get_user_wants_to_start_from_plane_position (); // v3.0.253.11 a function that checks property setup + layer came from so we will ignore this property if not called from the correct layer

  // std::string prepare_message_with_special_keywords(missionx::NavAidInfo& inNavAid, const std::string &inMessage); // del

  static bool      flag_force_template_distances_b;
  const int RADIUS_MT_MINIMUM_LENGTH = 50;

  bool        prepare_blank_template_with_flight_legs_based_on_ui(IXMLNode& pNode,IXMLNode& outMetaNode, std::string& outErr); // v3.0.241.9
  std::string briefer_skeleton_message_to_use_in_injectTypeMissionFeature; // v3.0.241.9
  std::string briefer_starting_location_desc; // v25.05.1

  static std::map<int, NavAidInfo> gen_get_databaseflightplan_site_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr); // v25.10.1
  missionx::mx_return gen_prepare_mission_based_on_databaseflightplan_site(IXMLNode& in_xTemplateNode); // v25.10.1
  // bool prepare_mission_based_on_external_fpln(IXMLNode& pNode); // v3.0.253.1 // NEED TO CONVERT // DEPRECATED

  static std::map<int, NavAidInfo> gen_get_ils_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr); // v25.10.1
  missionx::mx_return gen_prepare_mission_based_on_ils_search (IXMLNode &pNode); // v25.10.1
  // bool prepare_mission_based_on_ils_search(IXMLNode& pNode);    // v3.0.253.6 // NEED TO CONVERT // DEPRECATED

  void add_waypoints_for_fpln_or_simbrief(IXMLNode& pNode); // v25.04.2 // NEED TO CONVERT
  static std::map<int, NavAidInfo> gen_get_user_fpln_or_simbrief_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr); // v25.10.1
  missionx::mx_return gen_prepare_mission_based_on_user_fpln_or_simbrief (IXMLNode &in_xTemplateNode); // v25.10.1
  // bool prepare_mission_based_on_user_fpln_or_simbrief (IXMLNode &pNode); // v25.03.3 // v25.10.1 DEPRECATED
  // end v25.06.1
  mx_return gen_prepare_mission_based_on_oilrig (IXMLNode &inRootTemplate); // v25.09.2
  // mx_return gen_prepare_mission_based_on_oilrig2 (IXMLNode &pNode, std::string &outErr); // v25.09.1


public:
  RandomEngine();
  ~RandomEngine() override;

  void abortThread();
  void reset_sequence_numbers(); // v25.06.1

  static missionx::TemplateFileInfo *working_tempFile_ptr; // v3.0.241.9
  bool                               flag_rules_defined_by_user_ui; // v3.0.241.9
  std::string                        cumulative_location_desc_s;
  std::string                        first_location_desc_s; // v25.04.2, used in conjunction with "Expose all GPS legs at mission start" = false

  //// OSM related queries
  // static bool flag_picked_from_osm_database; // del // v25.09.2 deprecated since we never use it

  // members exposing private parameters
  static std::string getErrorMsg() { return missionx::RandomEngine::errMsg; }

  IXMLNode getBrieferNode() const
  {
    if (this->xBriefer.isEmpty())
      return IXMLNode::emptyIXMLNode;

    return this->xBriefer.deepCopy();
  }

  // members
  ///// Thread members
  static std::thread  thread_ref;
  static strct_thread_state random_thread_state;

  static std::map<XPLMNavRef, missionx::NavAidInfo> mapNavAidsFromMainThread;                           // v3.0.221.4 holds nav aid data from main plugin thread so thread will process it later in the background
  // std::map<std::string, XPLMNavRef>          map_customScenery_XPLMNavRef_NavAidsFromMainThread; // v3.0.253.6 holds navaids name and reference to Navaids that are also custom based (from Custom Scenery)

  bool        exec_generate_mission_thread (const std::string &inKey);
  static void stop_plugin ();

  int get_num_of_flight_legs(); // v3.0.219.14 return how many goals in xFlightLegs node

  static float calc_slope_at_point_mainThread(NavAidInfo& inNavAid);

  // static missionx::NavAidInfo lastFlightLegNavInfo; // v3.0.219.9 Holds the last point location of the last built goal // v3.0.221.4 moved to public

  // v3.0.255.3 implementing db queries instead of using cached data
  static std::map<std::string, std::string>                          row_gather_db_data;
  static std::unordered_map<int, std::map<std::string, std::string>> resultTable_gather_random_airports;
  static std::unordered_map<int, std::map<std::string, std::string>> resultTable_gather_ramp_data;

  static int callback_gather_random_airports_db(void* data, int argc, char** argv, char** azColName);    // this function will be called for each fetched row
  static int callback_pick_random_ramp_location_db(void* data, int argc, char** argv, char** azColName); // this function will be called for each fetched row

  // NavAidInfo get_random_airport_from_db(missionx::Point& inPoint, float inMinDistance_nm, float inMaxDistance_nm, int inExcludeAngle, missionx::mx_base_node &inProperties);
  static NavAidInfo get_random_airport_from_db(missionx::Point& inPoint, float inMinDistance_nm, float inMaxDistance_nm, int inExcludeAngle, missionx::mx_base_node &inProperties, const uint8_t & in_plane_type);

  // gather NavAid information from main plugin thread
  // void gatherRandomAirport_mainThread(const Point& inPoint, float inMaxDistance_nm, int inExcludeAngle = -1, float inStartFromDistance_nm = 0.0f);

  //// v3.0.253.7 made public
  static bool filterAndPickRampBasedOnPlaneType (missionx::NavAidInfo &           navAid,
                                                std::string &                     outErrorMsg,
                                                const missionx::mxFilterRampType &inRampFilterType); // v3.0.221.7 currently being used when reading briefer, and we want to place in plausible
  ///// weather v3.303.13
  static std::string current_weather_datarefs_s;

  static constexpr int MAX_OSM_NODES_TO_SEARCH = 10;

private:

  typedef enum class _which_type_to_force
    : uint8_t
  {
    no_force_is_needed,
    force_hover,
    force_flat_terrain_to_land
  } mx_which_type_to_force;


  // main function
  // Builds a temporary XML template file from user options, after doing find/replace.
  std::string inject_files_into_xml(missionx::TemplateFileInfo* tempFile_ptr);  // v24.12.2 implement the new multi options code.
  bool        generateRandomMission();

  // A simple function to manage thread wait for main thread actions that needs to take place before it can continue. Default wait time is 500 milliseconds for 10 iteration (5 seconds)
  // For every function call we need to handle failure cases (false returned).
  // For every function call we need to use threadState.pipeProperties to set the attributes we want the main thread to handle.

  // v25.09.1 deprecated moved to data_manager to share with other parts of the code
  // static bool waitForPluginCallbackJob(missionx::mx_flc_pre_command inQueuedCommand, std::chrono::milliseconds inWaitTimeMilliseconds = std::chrono::milliseconds(500), int inLimitWaitCounter = 10);

  // Main function to search destination airports
  // del
  // bool get_target(NavAidInfo& outNewNavInfo, const IXMLNode &inLegFromTemplate, mx_plane_types in_plane_type_enum, std::map<std::string, std::string>& inMapLocationSplitValues, missionx::mx_base_node& inProperties); // v3.305.1

  // v25.09.2
  static bool gen_target_base_on_icao_or_near_types(NavAidInfo&                         outNewNavInfo,
                                              mx_plane_types_enum                      in_plane_type_enum,
                                              std::map<std::string, std::string>& inMapLocationSplitValues,
                                              missionx::mx_base_node&             inProperties,
                                              NavAidInfo *prev_na_ptr);


  // v25.09.2 re-implement:"get_targetForHelos_base_XY_OSM_OSMWEB()", get Helos target based on OSM or fallback to XY location if none was found
  static bool gen_target_base_on_xy_osm_or_osmweb_types(NavAidInfo&                         outNewNavInfo,
                                              mx_plane_types_enum                      in_plane_type_enum,
                                              std::map<std::string, std::string>& inMapLocationSplitValues,
                                              missionx::mx_base_node&             inProperties,
                                              NavAidInfo *prev_na_ptr); // v25.09.2


  // get Helos target based on OSM or fallback to XY location if none was found
  // static bool get_targetForHelos_base_XY_OSM_OSMWEB(NavAidInfo&                         outNewNavInfo,
  //                                             mx_plane_types_enum                      in_plane_type_enum,
  //                                             std::map<std::string, std::string>& inMapLocationSplitValues,
  //                                             missionx::mx_base_node&             inProperties); // v3.305.1
  //                                             // double                              location_value_d,
  //                                             // double                              location_min_distance_d,
  //                                             // double                              location_max_distance_d);

  // v25.09.2 re-implement: "get_target_or_lastFlightLeg_base_on_XY_or_OSM()", search for airports based on XY information for all planes and for helos it can also be based on OSM data (depends on the location_type value - inLocationType)
  static bool gen_target_or_last_flight_leg_base_on_xy_or_osm(NavAidInfo&       outNewNavInfo,
                                            mx_plane_types_enum                      in_plane_type_enum,
                                            std::map<std::string, std::string>& inMapLocationSplitValues,
                                            missionx::mx_base_node&             inProperties,
                                            NavAidInfo *prev_na_ptr);

  // search for airports based on XY information for all planes and for helos it can also be based on OSM data (depends on the location_type value - inLocationType)
  // static bool get_target_or_lastFlightLeg_base_on_XY_or_OSM (NavAidInfo                         &outNewNavInfo,
  //                                                      std::map<std::string, std::string> &inMapLocationSplitValues,
  //                                                      missionx::mx_base_node             &inProperties);
  //                                                      // v3.305.1
  //                                                      // double location_value_d,
  //                                                      // double location_minDistance_d,
  //                                                      // double location_maxDistance_d);

  // v25.09.2 Search and pick a pre-defined location based on an XML tag name
  static bool gen_get_target_base_on_tag_name (NavAidInfo &                  outNewNavInfo,
                                                  std::map<std::string, std::string>& inMapLocationSplitValues,
                                                  missionx::mx_base_node &inProperties,
                                                  // const std::string &           location_value_tag_name_s,
                                                  // double                        location_value_d,
                                                  // double                        location_minDistance_d,
                                                  // double                        location_maxDistance_d,
                                                  NavAidInfo *prev_na_ptr);

  // bool get_target_base_on_tag_name(NavAidInfo&             outNewNavInfo,
  //                               mx_plane_types_enum          in_plane_type_enum,
  //                                 const missionx::mx_base_node& inProperties, // v3.305.1
  //                               const std::string&             location_value_tag_name_s,
  //                               double                  location_value_d,
  //                               double                  location_minDistance_d,
  //                               double                  location_maxDistance_d);

  static double get_slope_at_point (const missionx::NavAidInfo &outNavAid);
  static bool   get_is_wet_at_point (const missionx::NavAidInfo &inNavAid);
  static void   get_skew_target_data (missionx::NavAidInfo &in_target_navaid); // v25.09.2
  static float  get_terrain_elevation_at_point_in_mt (const missionx::NavAidInfo &inNavAid);


  static void calculate_bbox_coordinates(missionx::Point& outN0, missionx::Point& outS180, missionx::Point& outE90, missionx::Point& outW270, float inRefLat, float inRefLon, double inMaxRadius_d); // v3.0.255.3
  static void gather_all_osm_db_files_names_and_path(std::list<std::string>& outListOfFiles);
  // static bool osm_get_navaid_from_osm(NavAidInfo&                         outNavAid,
  //                              std::map<std::string, std::string>& inMapLocationSplitValues,
  //                              missionx::mx_base_node&             inProperties, // v3.305.1
  //                              double                              sourceLat_d,
  //                              double                              sourceLon_d,
  //                              double                              min_lat,
  //                              double                              max_lat,
  //                              double                              min_lon,
  //                              double                              max_lon,
  //                              double                              maxDistance_d = mxconst::SLIDER_MAX_RND_DIST,
  //                              double minDistance_d = (double)mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT /*, std::string inExpectedLocationType = ""*/); // if return empty string then no file was found valid for the search

  static bool osm_get_navaid_from_osm(NavAidInfo&                         outNavAid,
                               std::map<std::string, std::string>& inMapLocationSplitValues,
                               missionx::mx_base_node&             inProperties, // v3.305.1
                               NavAidInfo*                         prev_navaid_ptr, // v25.10.1
                               double                              min_lat,
                               double                              max_lat,
                               double                              min_lon,
                               double                              max_lon,
                               double                              maxDistance_d = mxconst::SLIDER_MAX_RND_DIST,
                               double minDistance_d = (double)mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT /*, std::string inExpectedLocationType = ""*/); // if return empty string then no file was found valid for the search

  static bool osm_get_navaid_from_overpass(NavAidInfo&                         outNavAid,
                                    std::map<std::string, std::string>& inMapLocationSplitValues,
                                    missionx::mx_base_node&             inProperties, // v3.305.1
                                    double                              sourceLat_d,
                                    double                              sourceLon_d,
                                    double                              min_lat,
                                    double                              max_lat,
                                    double                              min_lon,
                                    double                              max_lon,
                                    double                              maxDistance_d = mxconst::SLIDER_MAX_RND_DIST,
                                    double                              minDistance_d = (double)mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT); // if return empty string then no file was found valid for the search

  static bool osm_get_navaid_from_osm_database(NavAidInfo&                         outNavAid,
                                        std::map<std::string, std::string>& inMapLocationSplitValues,
                                        missionx::mx_base_node&             inProperties, // v3.305.1
                                        double                              sourceLat_d,
                                        double                              sourceLon_d,
                                        double                              min_lat,
                                        double                              max_lat,
                                        double                              min_lon,
                                        double                              max_lon,
                                        double                              maxDistance_d = mxconst::SLIDER_MAX_RND_DIST,
                                        double                              minDistance_d = (double)mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT); // if return empty string then no file was found valid for the search
  static void initQueries();

  // overpass mission_info custom urls
  static std::vector<std::string> vecMissionInfoOverpassUrls;
  static int                      current_url_indx_used_i; // = mxconst::INT_UNDEFINED;

  // // Test both NavAid ID and Name. Sometimes the ID can be empty but not the name.
  // del
  // static bool check_if_new_target_is_same_as_prev (missionx::NavAidInfo &inCurrentTargetNav, missionx::NavAidInfo &inPrevNav);
  // bool        check_last_2_legs_if_they_have_same_icao (); // del

  static std::string get_short_flight_description_from_to (const std::string &inFromName, const std::string &inFromICAO, const std::string &inToName, const std::string &inToICAO);

  // v25.02.1
  static std::vector<IXMLNode>                                             gen_land_hover_display_objects (const double &inLat, const double &inLon, const int &inRadiusMT, const int &inHowManyObjects, int &inout_seq, const std::string &inFileName = "land_hover01.obj");
  static std::map<missionx::enums::mx_osm_region, missionx::structs::BBox> gen_quadrant_bboxes (double centerLat, double centerLon);

  int seq_triggers   = 0;
  int seq_tasks      = 0;
  int seq_objectives = 0;
  int seq_waypoints  = 0; // flight leg
  int seq_messages   = 0; // messages
  typedef struct _inventory_track_struct
  {
    bool flag_created_move_and_remove_item_from_plane_script = false;
    bool flag_called_remove_items_from_plane = false;

    int fpln_seq = -1;

    std::string inventory_name;
    std::string move_to_plane_script_name;
    std::string remove_from_plane_script_name;

    std::string scriptlet_move_to_plane_text;
    std::string scriptlet_remove_from_plane_text;

    IXMLNode xml_move_to_plane_script_node;
    IXMLNode xml_remove_from_plane_script_node;

  } mx_inventory_track_strct;
  std::unordered_map<int, RandomEngine::mx_inventory_track_strct> map_osm_inventory_track;

  mx_return                                              gen_prepare_medevac_surprise_me (IXMLNode &inRootTemplate, const IXMLNode &inoutMetaNode, const missionx::Point &in_plane_location); // v25.05.1
  // mx_return                                              gen_prepare_medevac_surprise_me2 (IXMLNode &inRootTemplate, const IXMLNode &inoutMetaNode, const missionx::Point &in_plane_location); // v25.05.1
  static std::vector<missionx::structs::strct_osm_query> gen_osm_analyse (mx_return &out_mx_return, const std::string &xmlFilename, const std::string &in_cache_folder, double centre_lat, double centre_lon, IXMLNode &outRootNode = IXMLNode::emptyIXMLNode);
  // The function returns "shuffled index vector" as a value, and initializes the "out_main_subject_node" and "analyzed_query" from inside the function to use later from the calling routine.
  static std::vector<int>                    gen_shuffled_q_from_osm_subject_node (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_root_node, const std::vector<missionx::structs::strct_osm_query> &vec_osm_queries, IXMLNode &out_main_subject_node, missionx::structs::strct_osm_query &analyzed_query);
  static std::map<int, missionx::NavAidInfo> gen_get_targets_using_osm_queries_from_a_thread (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_root_node, missionx::structs::strct_osm_query &inout_osm_query, strct_shared_random_airport_info &inout_shared_navaid);
  // find metadata of current target NavAid relative to previous and next NavAids
  static std::string          gen_leg_name (int *seq, const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo &inTargetNavAid);
  static void                 gen_gather_navaid_metadata_relative_to_target (const IXMLNode &inoutMetaNode, missionx::NavAidInfo &inout_target_navaid, missionx::NavAidInfo &inout_from_navaid, missionx::NavAidInfo *inout_next_navaid_ptr);
  static IXMLNode             gen_trigger_node (int &seq, const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo &inTargetNavAid, const std::list<missionx::structs::strct_node_attribute_key_value> &in_attrib_list, IXMLNode *parentNode = nullptr);
  static IXMLNode             gen_task_node (int &seq, const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo &inTargetNavAid, const std::list<missionx::structs::strct_node_attribute_key_value> &in_attrib_list, IXMLNode *parentNode = nullptr);
  static IXMLNode             gen_objective_node (int &seq, const std::string &prefix_name, const std::string &postfix_name, IXMLNode *parentNode = nullptr);
  static IXMLNode             gen_leg_node (const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo *inTargetNavAid, const std::list<missionx::structs::strct_node_attribute_key_value> *in_attrib_list, IXMLNode *parentNode = nullptr);
  static missionx::NavAidInfo gen_briefer_phase_01_parse_briefer_and_start_location (const IXMLNode &in_xTemplate, IXMLNode &in_node); // parse <briefer_and_start_location>
  static NavAidInfo           gen_briefer_phase_02_base_node_from_navaid (missionx::NavAidInfo &inout_start_navaid, strct_shared_random_airport_info &inout_strct_shared_navaid_info, bool in_flag_we_have_target_above_water);
  static void                 gen_briefer_phase_03_add_desc (std::map<int, NavAidInfo> &inout_targets, bool flag_has_wet_target);
  IXMLNode                    gen_mission_info_node (const IXMLNode &xRootTemplate, const std::string &in_template_name, const std::string &in_template_image_file_name, const std::string &in_mission_folder_name);
  static IXMLNode             gen_add_inventory_phase01_node (const int &in_seq, missionx::NavAidInfo &inout_navaid, std::unordered_map<int, mx_inventory_track_strct> &inout_map_osm_inventory_track, const float &in_radius = 0.0, const std::list<missionx::structs::strct_node_attribute_key_value> *in_override_attrib_list = nullptr);
  static void                 gen_add_inventory_phase02_add_items (missionx::NavAidInfo &inOutNavAidInfo);
  static void                 gen_target_inventory_scripts (missionx::NavAidInfo &in_target_navaid, std::unordered_map<int, mx_inventory_track_strct> &inout_map_osm_inventory_track);
  static std::string          gen_message_with_special_keywords_static (std::string inMessage, missionx::NavAidInfo &in_target_navaid);
  // Add a description and return the description node pointer
  static IXMLNode gen_leg_description (IXMLNode &in_xml_leg_node, missionx::NavAidInfo &inout_navaid_target, missionx::NavAidInfo *in_next_leg_as_navaid_ptr = nullptr); //, random_airport_info_struct &inout_random_airport_info_struct);
  static void     gen_add_3d_marker_to_current_target (IXMLNode &inout_leg_node, missionx::NavAidInfo &in_target_navaid); // adds a marker - <display_object>, above the target.
  static void     gen_leg_start_messages (int &seq, NavAidInfo &inout_target_na, std::map<int, NavAidInfo> &navaid_targets, IXMLNode &inout_xml_messages, const bool &flag_one_of_targets_is_above_water_body); // adds simple messages between flight legs.
  static void     gen_messages_when_reaching_target_leg (int &seq_trig, int &seq_msg, NavAidInfo &inout_target_na, IXMLNode &inout_xml_messages, IXMLNode &inout_xml_triggers, const IXMLNode &in_xml_land_trigger, const IXMLNode &in_xml_hover_trigger); // add "you reached the target area" message. Add as trigger
  static void     gen_2nm_message (int &seq_trig, int &seq_msg, NavAidInfo &inout_target_na, IXMLNode &inout_xml_messages, IXMLNode &inout_xml_triggers, const IXMLNode &in_xml_land_trigger);
  // static void     gen_parse_and_add_all_display_objects_in_node (const std::string &in_which_func_called, const IXMLNode &in_source_node, IXMLNode &inout_target_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d);
  static void     gen_parse_and_add_all_display_objects_in_node (const std::string &in_which_func_called, missionx::NavAidInfo &in_target_navaid, IXMLNode &in_source_node, IXMLNode &inout_target_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d);
  static void     gen_3d_hint_objects_for_land_and_hover (const NavAidInfo &inout_target_na, IXMLNode &inout_leg_node, const NavAidInfo *next_navaid_ptr); // Add 3D hint objects
  static void     gen_add_3d_objects_for_surprise_me_base_on_predefined_attributes (NavAidInfo &inout_target_na, IXMLNode &inout_leg_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d); // Add 3D clutter Objects around the target

  // v25.09.2 used with template <content> type missions. Only adds the <display_objects> to the <leg>.
  // Will be correctly set once gen_3d_instance_properties() function will be called.
  static bool gen_3d_add_display_object_sets_instances_to_leg (NavAidInfo &inout_target_na, IXMLNode &inout_leg_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d); // Add 3D clutter Objects around the target
  static bool gen_3d_parse_instances_in_leg (IXMLNode &pNode, missionx::NavAidInfo &in_target_navaid);
  // end v25.06.1

  // v25.09.1
  static std::map<int, missionx::NavAidInfo> gen_oilrig_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_mapping_root_node, IXMLNode &inout_meta_data_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr);

  // returns error message. Empty message means code is good.
  static missionx::structs::strct_expected_location_data parse_expected_location (const IXMLNode &in_xml_leg_from_template, const std::string &custom_error_message, bool is_last_leg = false);

  // v25.09.2
  // Will only parse the template "leg" from a template file.
  // Internally will also parse the <expected_location>.
  static missionx::NavAidInfo                gen_parse_template_leg (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &xTemplateNode, const IXMLNode &xml_leg_node_from_template, strct_shared_random_airport_info &inout_shared_navaid, std::map<int, missionx::NavAidInfo> &in_mission_targets, const int &in_leg_counter, bool is_last_flight_leg, std::string &outErr);
  static std::map<int, missionx::NavAidInfo> gen_get_content_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, const IXMLNode &in_content_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr);
  static std::map<int, missionx::NavAidInfo> gen_get_generic_template_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr);
  missionx::mx_return                        gen_prepare_random_mission_based_on_content (IXMLNode &xTemplateNode); // v25.09.2
  // missionx::mx_return                        gen_content_option01_random_mission_from_content2 (IXMLNode &xTemplateNode, IXMLNode & xContent); //  del
  missionx::mx_return                        gen_content_option_01_random_mission_from_content (IXMLNode &xTemplateNode, IXMLNode & xContent); // v25.09.2
  missionx::mx_return                        gen_content_option_02_copy_as_is (IXMLNode &xTemplateNode, IXMLNode & xContent); // v25.09.2
  missionx::mx_return                        gen_prepare_random_mission_based_on_leg_nodes_in_template (IXMLNode &in_xTemplateNode); // v25.09.2 - this represents the original way we constructed a random mission
  void                                       gen_create_all_leg_nodes_based_on_navaid_targets (std::map<int, NavAidInfo> &navaid_targets);

  static std::string gen_get_cumulative_fpln_desc (std::map<int, NavAidInfo> &navaid_targets);
  // conduct basic validations to figure out if all navaids are valid and how many are valid
  static missionx::mx_return gen_validate_navaids (std::map<int, NavAidInfo> &navaid_targets, int &inout_valid_navaids);
  static IXMLNode gen_set_and_get_start_cold_and_dark (IXMLNode &xTemplateNode, NavAidInfo &navaid);

public:

  inline static std::map<std::string, mx_plane_types_enum> mapPlaneStringTypesToEnum = {
    { "", missionx::mx_plane_types_enum::plane_type_any },
    { "helos", missionx::mx_plane_types_enum::plane_type_helos },
    { "prop", missionx::mx_plane_types_enum::plane_type_props },
    { "prop_floats", missionx::mx_plane_types_enum::plane_type_prop_floats },
    { "heavy", missionx::mx_plane_types_enum::plane_type_heavy },
    { "turboprops", missionx::mx_plane_types_enum::plane_type_turboprops },
    { "jet", missionx::mx_plane_types_enum::plane_type_jets },
    { "ga", missionx::mx_plane_types_enum::plane_type_ga },
    { "ga_floats", missionx::mx_plane_types_enum::plane_type_ga_floats }
  };

  inline static std::map<mx_plane_types_enum, std::string> mapPlaneEnumToStringTypes;
  // Stores the tag names need to fetch from the <MAPPING> part of the template file to use as navaid.fpln_xml_leg
  inline static std::map<int, std::string> map_flight_legs_translation_from_template = {}; // v25.09.1 used with the new oil-rig function since gen_osm template is different.

};

/* namespace missionx */
}

/* RANDOMENGINE_H_ */
#endif
