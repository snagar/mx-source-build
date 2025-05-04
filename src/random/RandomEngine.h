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

private:
  // key=types value: string translation in lower case  
  std::map<mx_plane_types, std::string> mapPlaneEnumToStringTypes;
  std::map<std::string, mx_plane_types> mapPlaneStringTypesToEnum;
  mx_plane_types                        template_plane_type_enum; // holds the enum type. we can then use mapPlaneEnumToStringTypes to translate to string value

  std::string              translatePlaneTypeToString(mx_plane_types in_plane_type);
  missionx::mx_plane_types translatePlaneTypeToEnum(const std::string& in_plane_type);


  std::string pathToRandomRootFolder;
  std::string pathToRandomBrieferFolder;
  // std::string templateFile; // v25.02.1 deprecated, use local inKey instead // v3.0.217.6 holds the picked template file name. We can use it in other class functions
  bool        flag_found;
  std::string randomPlaneType; // v3.0.221.11
  void        setPlaneType(std::string inPlaneType);
  void        setPlaneType(mx_plane_types inPlaneType);
  uint8_t     getPlaneType();

  std::string errMsg;

  ///// template stream and xml nodes
  //// Read XML mission file
  IXMLDomParser iDomTemplate;

  IXMLNode xRootTemplate;

  //// Mission File stream and xml nodes
  IXMLNode xTargetMainNode; //=XMLNode::openFileHelper( std::string("save1.xml").c_str() );
  IXMLNode xDummyTopNode;   // v3.0.253.12 renamed from xTargetTopNode to xDummyTopNode. The dummy node holds the <MISSION> element, but when we write to the file we will create the true Target node. This is done for filterring out cases,
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
  //IXMLNode xMessageTemplates; // holds custom messages designer added to the template for <leg_message> usage for example
  IXMLNode xEnd;              // holds end element information
  IXMLNode xGPS;              // holds GPS coordinates
  IXMLNode xChoices;          // holds <choice> sub elements
  IXMLNode x3DObjTemplate;    // holds 3D Object Templates
  IXMLNode xpData;            // v3.0.221.10 holds Dataref list
  IXMLNode xEmbedScripts;     // v3.0.221.10 holds external or scriptlet directives

  typedef struct _mission_xml_data
  {
    std::string currentLegName;

    _mission_xml_data() { init(); }

    void init() { currentLegName.clear(); }
  } mission_xml_data_struct;

  mission_xml_data_struct mission_xml_data;

  ///// end mission XML elements ////

  //missionx::mxProperties elementBrieferInfoProperties;
  missionx::mx_base_node elementBrieferInfoProperties;

  ///// Useful Parameters ////
  missionx::Point planeLocation;

  std::list<missionx::NavAidInfo> listNavInfo; // v3.0.219.6

  std::map<std::string, int> mapFlightPlanOrder_si; // assist in constructing next goal. We add goal name and sequence numbering every time we add to "xFlightLegs"
  std::map<int, std::string> mapFLightPlanOrder_is; // assist in constructing next goal. We add sequence numbering and then the goal name.

  //// hidden function members
  void setError(const std::string& inMsg);

  // XML related
  static void add_xml_space(IXMLNode& node)
  {
    if (!node.isEmpty())
    {
      node.addClear(" ", nullptr, nullptr);
    }
  }

  static IXMLNode get_skewed_target_position (const IXMLNode & inRealTargetPositionPoint);             // will return a skewed position in the ~0.5-3.0nm away from target.
  bool     parse_display_object_element(IXMLNode& inFlightLegNode, IXMLNode& inDisplayNode); // check xml tag <display_object> for specific random attributes.
  void     parse_3D_object_template_element();                                          // check xml tag <object_template> for specific random attributes.


  bool readMissionInfoElement();
  bool readBrieferAndStartLocation();         // assist in constructing briefer element. Mission description will be fetched from "elementBrieferInfoProperties" which was initialized in "readMissionInfoElement()"
  bool readFlightLegs_directlyFromTemplate(); // assist in constructing Goals element.
  bool flag_isLastFlightLeg;                  // v3.0.219.11 specifically for slope test in location_type="xy" and template_type="medevac". In last goal we skip this test, since we expect to land

  IXMLNode get_content_story(const IXMLNode& xTemplateNode /*, std::string inTemplateType*/); // v3.0.219.14 Will try to parse and pick a background content story
  bool     extract_flight_leg_set(IXMLNode& inNodeTemplate, const IXMLNode& inSetNode, int& inCounter); // v3.0.219.14 Will try to parse and pick a background content story
  bool     build_and_add_flight_leg_from_node(const IXMLNode& inNode, int& inCounter);        // v3.0.219.14 Will try to parse and pick a background content story
  bool     generateRandomMissionBasedOnContent(IXMLNode& xTemplateNode);                // v3.0.219.13
  static bool     get_isNavAidInValidDistance(const double& currentDistanceToTarget, const double& in_location_value_d, const double& in_location_minDistance_d, const double& in_location_maxDistance_d);


  IXMLNode buildFlightLeg(int inFlightLegCounter, const IXMLNode& in_legNodeFromTemplate);

  void fill_up_next_leg_attrib_after_flight_plan_was_generated();


  void readOptimizedAptDatIntoCache();

  bool setInstanceProperties(IXMLNode& pNode, missionx::NavAidInfo& inTargetNavInfo, bool flag_place_target_markers_near_target);
  void injectMissionTypeFeatures();
  void injectMessagesWhileFlyingToDestination();
  typedef enum _inv_source
  {
    point   = 1,
    trigger = 2
  } mxInvSource;
  void                  addInventory(const std::string& inGoalName, const IXMLNode & inSourceNode, mxInvSource inSource = mxInvSource::trigger);
  std::set<std::string> setInventories;

  // void check_validity_of_display_object_elements(const std::string &inSavePath, const IXMLNode parent, IXMLNode& nodeBriefer); // deprecated since we do not use. It was moved the data_manager class.
  bool writeTargetFile();

  // do a 360 swipe and pick Airports every 10deg and in distances of 10/20/30..Nth nautical miles. Store airport data in a map of <int, NavAidInfo>
  //void getRandomAirport_localThread(missionx::NavAidInfo& outNavAid, std::string inLocationType = EMPTY_STRING, std::string inRestrictRampType = missionx::EMPTY_STRING); // v3.0.221.15 rc3.5

  double expected_slope_at_target_location_d;

  void init();

  void       addTriggersBasedOnTargetLocation(NavAidInfo& inNav, IXMLNode& inSpecialLegSubNode);
  void       injectCountdownTimers();
  static bool get_user_wants_to_start_from_plane_position(); // v3.0.253.11 a function that checks property setup + layer came from so we will ignore this property if not called from the correct layer

  std::map<std::string, std::string> mapReplaceMessageKeywords; // v3.0.221.11 keyword, value from Navaid

  void        update_start_cold_and_dark_with_special_keywords(IXMLNode& inDatarefStartColdAndDarkNode);
  std::string prepare_message_with_special_keywords(missionx::NavAidInfo& inNavAid, std::string inMessage);

  bool      flag_force_template_distances_b;
  const int RADIUS_MT_MINIMUM_LENGTH = 50;

  bool        prepare_blank_template_with_flight_legs_based_on_ui(IXMLNode& pNode,IXMLNode& outMetaNode, std::string& outErr); // v3.0.241.9
  std::string briefer_skeleton_message_to_use_in_injectTypeMissionFeature;           // v3.0.241.9

  bool prepare_mission_based_on_external_fpln(IXMLNode& pNode); // v3.0.253.1
  bool prepare_mission_based_on_ils_search(IXMLNode& pNode);    // v3.0.253.6
  void add_waypoints_for_fpln_or_simbrief(IXMLNode& pNode); // v25.04.2
  bool prepare_mission_based_on_user_fpln_or_simbrief(IXMLNode& pNode); // v25.03.3
  bool prepare_mission_based_on_oilrig ( const IXMLNode & pNode, std::string& outErr); // v3.303.14


  //// OSM related queries
  bool flag_picked_from_osm_database;

public:
  RandomEngine();
  ~RandomEngine() override;

  void abortThread();

  missionx::TemplateFileInfo* working_tempFile_ptr;          // v3.0.241.9
  bool                        flag_rules_defined_by_user_ui; // v3.0.241.9
  std::string                 cumulative_location_desc_s;
  std::string                 first_location_desc_s; // v25.04.2, used in conjunction with "Expose all GPS legs at mission start" = false

  // members exposing private parameters
  std::string getErrorMsg() { return this->errMsg; }

  IXMLNode getBrieferNode() const
  {
    if (this->xBriefer.isEmpty())
      return IXMLNode::emptyIXMLNode;

    return this->xBriefer.deepCopy();
  }

  // members
  ///// Thread members
  static std::thread  thread_ref;
  static thread_state threadState;

  std::map<XPLMNavRef, missionx::NavAidInfo> mapNavAidsFromMainThread;                           // v3.0.221.4 holds nav aid data from main plugin thread so thread will process it later in the background
  std::map<std::string, XPLMNavRef>          map_customScenery_XPLMNavRef_NavAidsFromMainThread; // v3.0.253.6 holds navaids name and reference to Navaids that are also custom based (from Custom Scenery)

  bool exec_generate_mission_thread(const std::string& inKey);
  static void       stop_plugin();
  // void       resetCache();


  int get_num_of_flight_legs(); // v3.0.219.14 return how many goals in xFlightLegs node

  float calc_slope_at_point_mainThread(NavAidInfo& inNavAid);

  missionx::NavAidInfo lastFlightLegNavInfo; // v3.0.219.9 Holds the last point location of the last built goal // v3.0.221.4 moved to public

  typedef struct _random_airport_info_struct
  {
    // convert icao to xml point
    IXMLNode parentNode_ptr; // pointer to XML element that might hold icao elements that needs to be converted to points

    Point p;
    bool  isWet{}; // will hold wet status

    float       inMinDistance_nm{}; // v3.0.221.15
    float       inMaxDistance_nm{};
    int         inExcludeAngle{};
    float       inStartFromDistance_nm{};
    std::string inRestrictRampType;
    // std::string inRestrictMinMaxNavaidDistance; // v3.0.221.7 asked Daikan, format min-max

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
  } random_airport_info_struct;
  random_airport_info_struct shared_navaid_info;

  // v3.0.255.3 implementing db queries instead of using cached data

  static std::map<std::string, std::string>                          row_gather_db_data;
  static std::unordered_map<int, std::map<std::string, std::string>> resultTable_gather_random_airports;
  static std::unordered_map<int, std::map<std::string, std::string>> resultTable_gather_ramp_data;

  static int callback_gather_random_airports_db(void* data, int argc, char** argv, char** azColName);    // this function will be called for each fetched row
  static int callback_pick_random_ramp_location_db(void* data, int argc, char** argv, char** azColName); // this function will be called for each fetched row
  /// <summary>
  /// Main function to gather airport information from an optimized database.
  /// We fetch: airports relative to distance, bearing, runway type and plane type = ramp type.
  /// </summary>
  /// <param name="inPoint">A coordinate location which represent one of the targets the plane should reach</param>
  /// <param name="inMinDistance_nm">Minimum distance to search airports from.</param>
  /// <param name="inMaxDistance_nm">Maximum distance to search for airports.</param>
  /// <param name="inExcludeAngle">In many cases we want to pick new airports and not pick the one we just came from. The idea is to limit the angle we will fly next so the flight plan will be seen plausiable.</param>
  /// <returns></returns>
  NavAidInfo get_random_airport_from_db(missionx::Point& inPoint, float inMinDistance_nm, float inMaxDistance_nm, int inExcludeAngle);

  // gather NavAid information from main plugin thread
  void gatherRandomAirport_mainThread(const Point& inPoint, float inMaxDistance_nm, int inExcludeAngle = -1, float inStartFromDistance_nm = 0.0f);

  //// v3.0.253.7 made public
  bool filterAndPickRampBasedOnPlaneType(missionx::NavAidInfo& navAid,
                                         std::string&          outErrorMsg,
                                         const missionx::mxFilterRampType & inRampFilterType
                                         //const bool & inIgnoreCenterOfRunwayAsRamp = false
                                         ); // v3.0.253.1 deprecated since not in use , std::string inRestrictRampType = ""); // v3.0.221.7 currently being used when reading briefer and we want to place in plausiable
  ///// weather v3.303.13
  static std::string current_weather_datarefs_s;


private:
  const int MAX_OSM_NODES_TO_SEARCH{ 10 };

  typedef enum class _which_type_to_force
    : uint8_t
  {
    no_force_is_needed,
    force_hover,
    force_flat_terrain_to_land
  } mx_which_type_to_force;


  // main function
  // bool generateRandomMission(std::string *inKey_ptr, thread_state *inThreadState);
  std::string inject_files_into_xml(missionx::TemplateFileInfo* tempFile_ptr);  // v24.12.2 implement the new multi options code.
  std::string inject_files_into_xml_1(missionx::TemplateFileInfo* tempFile_ptr); // keep the original code before using the "multi option" code.
  bool        generateRandomMission();

  // A simple function to manage thread wait for main thread actions that needs to take place before it can continue. Default wait time is 500 milliseconds for 10 iteration (5 seconds)
  // For every function call we need to handle failure cases (false returned).
  // For every function call we need to use threadState.pipeProperties to set the attributes we want the main thread to handle.
  static bool waitForPluginCallbackJob(missionx::mx_flc_pre_command inQueuedCommand, std::chrono::milliseconds inWaitTimeMilliseconds = std::chrono::milliseconds(500), int inLimitWaitCounter = 10);

  // Main function to search destination airports
  bool get_target(NavAidInfo& outNewNavInfo, const IXMLNode &inLegFromTemplate, mx_plane_types in_plane_type_enum, std::map<std::string, std::string>& inMapLocationSplitValues, missionx::mx_base_node& inProperties); // v3.305.1

  // get Helos target based on OSM or fallback to XY location if none was found
  bool get_targetForHelos_based_XY_OSM_OSMWEB(NavAidInfo&                         outNewNavInfo,
                                              mx_plane_types                      in_plane_type_enum,
                                              std::map<std::string, std::string>& inMapLocationSplitValues,
                                              //missionx::mxProperties&             inProperties,
                                              missionx::mx_base_node&             inProperties, // v3.305.1
                                              double                              location_value_d,
                                              double                              location_minDistance_d,
                                              double                              location_maxDistance_d);
  // search for airports based on XY information for all planes and for helos it can also be based on OSM data (depends on the location_type value - inLocationType)
  bool get_target_or_lastFlightLeg_based_on_XY_or_OSM (NavAidInfo                         &outNewNavInfo,
                                                       std::map<std::string, std::string> &inMapLocationSplitValues,
                                                       missionx::mx_base_node             &inProperties,
                                                       // v3.305.1
                                                       double location_value_d,
                                                       double location_minDistance_d,
                                                       double location_maxDistance_d);
  // Search and pick pre-defined location based on an XML tag name
  bool get_targetBasedOnTagName(NavAidInfo&             outNewNavInfo,
                                mx_plane_types          in_plane_type_enum,
                                  const missionx::mx_base_node& inProperties, // v3.305.1
                                const std::string&             location_value_tag_name_s,
                                double                  location_value_d,
                                double                  location_minDistance_d,
                                double                  location_maxDistance_d);

  double get_slope_at_point(missionx::NavAidInfo& outNavAid);
  bool   get_is_wet_at_point ( const missionx::NavAidInfo& inNavAid);


  void calculate_bbox_coordinates(missionx::Point& outN0, missionx::Point& outS180, missionx::Point& outE90, missionx::Point& outW270, float inRefLat, float inRefLon, double inMaxRadius_d); // v3.0.255.3
  void gather_all_osm_db_files_names_and_path(std::list<std::string>& outListOfFiles);
  bool osm_get_navaid_from_osm(NavAidInfo&                         outNavAid,
                               std::map<std::string, std::string>& inMapLocationSplitValues,
                               //missionx::mxProperties&             inProperties,
                               missionx::mx_base_node&             inProperties, // v3.305.1
                               double                              sourceLat_d,
                               double                              sourceLon_d,
                               double                              min_lat,
                               double                              max_lat,
                               double                              min_lon,
                               double                              max_lon,
                               double                              maxDistance_d = mxconst::SLIDER_MAX_RND_DIST,
                               double minDistance_d = (double)mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT /*, std::string inExpectedLocationType = ""*/); // if return empty string then no file was found valid for the search

  bool osm_get_navaid_from_overpass(NavAidInfo&                         outNavAid,
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

  bool osm_get_navaid_from_osm_database(NavAidInfo&                         outNavAid,
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
  std::vector<std::string> vecMissionInfoOverpassUrls;
  int                      current_url_indx_used_i = mxconst::INT_UNDEFINED;

  // // Test both NavAid ID and Name. Sometimes the ID can be empty but not the name.
  static bool check_if_new_target_is_same_as_prev(missionx::NavAidInfo &inCurrentTargetNav, missionx::NavAidInfo &inPrevNav);
  bool check_last_2_legs_if_they_have_same_icao();

  static std::string get_short_flight_description_from_to(const std::string& inFromName,const std::string& inFromICAO,const std::string& inToName,const std::string& inToICAO );

  // v25.02.1
  static std::vector<IXMLNode> calc_land_hover_display_objects (const double &inLat, const double &inLon, const int &inRadiusMT, const int &inHowManyObjects, int &inout_seq, const std::string &inFileName = "land_hover01.obj");

};

/* namespace missionx */
}

/* RANDOMENGINE_H_ */
#endif
