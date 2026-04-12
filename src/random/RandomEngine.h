/*
 * RandomEngine.h
 *
 *  Created on: Dec 13, 2018
 *      Author: snagar

Random Engine does not run on the main thread.
To make it compatible with the Laminar XPSDK, we must ensure that all calls are executed on the main thread and that the results will pass back and be handled by the Random class.
In this case, I had to implement a workaround: the child thread waits for N milliseconds and then processes the outcome.

Why is it a workaround ?
Because we are using shared parameters as the communication mechanism between threads.

We use:
RandomEngine::shared_navaid_info to hold target information as a navaid class.
RandomEngine::random_thread_state to hold navigation data shared with the main thread via the data_manager::waitForPluginCallbackJob() function call.

The end result is as follows:
We use data_manager::waitForPluginCallbackJob() to invoke functions that interact with the XPSDK APIs, while shared_navaid_info is used to pass data to and from the main thread.
For specific action,  We send missionx::mx_flc_pre_command:xxxx, which is handled by the mission::flcPRE() function.

!!! The primary issue with this flow is that it assumes no other code will modify the shared structure. !!!

That said, it works and remains within Laminar Research standards.

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
  bool                       flag_found;

  std::string pathToRandomRootFolder;
  std::string pathToRandomBrieferFolder;

  // key=types value: string translation in lower case
  static mx_plane_types_enum template_plane_type_enum; // holds the enum type. we can then use mapPlaneEnumToStringTypes to translate to string value
  static std::string                   translatePlaneTypeToString (mx_plane_types_enum in_plane_type);
  static missionx::mx_plane_types_enum translatePlaneTypeToEnum (const std::string &in_plane_type);

  std::string                randomPlaneType; // v3.0.221.11
  mx_plane_types_enum        setPlaneType (std::string inPlaneType);
  void                       setPlaneType (mx_plane_types_enum inPlaneType);
  // v26.04.2 Only returns plane type as "enum" value
  static mx_plane_types_enum get_plane_type_enum (std::string inPlaneType);
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

  //// hidden function members
  static void setError(const std::string& inMsg);

  static IXMLNode gen_get_skewed_target_position (const IXMLNode &inRealTargetPositionPoint); // will return a skewed position in the ~0.5-3.0nm away from target.
  static bool     parse_display_object_element (const missionx::NavAidInfo *in_target_nav_ptr, const IXMLNode &inFlightLegNode, IXMLNode &inDisplayNode, const IXMLNode &in_xRootTemplate, IXMLNode &x3DObjTemplate, const double &expected_slope_at_target_location_d, std::string &inout_err); // check xml tag <display_object> for specific random attributes.


  bool gen_read_mission_info_element();
  bool flag_isLastFlightLeg;                  // v3.0.219.11 specifically for slope test in location_type="xy" and template_type="medevac". In last goal we skip this test, since we expect to land

  static IXMLNode get_content_story (const IXMLNode &xTemplateNode /*, std::string inTemplateType*/); // v3.0.219.14 Will try to parse and pick a background content story


  typedef enum _inv_source
  {
    point   = 1,
    trigger = 2
  } mxInvSource;

  bool writeTargetFile();

  // do a 360 swipe and pick Airports every 10deg and in distances of 10/20/30..Nth nautical miles. Store airport data in a map of <int, NavAidInfo>
  double expected_slope_at_target_location_d;

  static bool gen_get_is_navaid_in_a_valid_distance (const double &currentDistanceToTarget, const double &in_location_value_d, const double &in_location_minDistance_d, const double &in_location_maxDistance_d);
  void init();

  void        gen_inject_countdown_timer ( const int &current_nav_index, std::map<int, missionx::NavAidInfo> &in_navaid_targets); // v25.10.1
  static bool get_user_wants_to_start_from_plane_position (); // v3.0.253.11 a function that checks property setup + layer came from so we will ignore this property if not called from the correct layer

  static bool      flag_force_template_distances_b;
  const int RADIUS_MT_MINIMUM_LENGTH = 50;

  bool        prepare_blank_template_with_flight_legs_based_on_ui(IXMLNode& pNode,IXMLNode& outMetaNode, std::string& outErr); // v3.0.241.9
  std::string briefer_skeleton_message_to_use_in_injectTypeMissionFeature; // v3.0.241.9
  std::string briefer_starting_location_desc; // v25.05.1

  static std::map<int, NavAidInfo> gen_get_databaseflightplan_site_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr); // v25.10.1
  missionx::mx_return gen_prepare_mission_based_on_databaseflightplan_site(IXMLNode& in_xTemplateNode, IXMLNode & inout_meta_node); // v25.10.1

  static std::map<int, NavAidInfo> gen_get_ils_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr); // v25.10.1
  missionx::mx_return gen_prepare_mission_based_on_ils_search (IXMLNode &pNode, IXMLNode & inout_meta_node); // v25.10.1

  void add_waypoints_for_fpln_or_simbrief(IXMLNode& pNode); // v25.04.2 // NEED TO CONVERT
  static std::map<int, NavAidInfo> gen_get_user_fpln_or_simbrief_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr); // v25.10.1
  missionx::mx_return gen_prepare_mission_based_on_user_fpln_or_simbrief (IXMLNode &in_xTemplateNode, IXMLNode & inout_meta_node); // v25.10.1
  // end v25.06.1
  mx_return gen_prepare_mission_based_on_oilrig (IXMLNode &inRootTemplate, IXMLNode & inout_meta_node); // v25.09.2

  static missionx::mx_plane_types_enum gen_parse_plane_type (missionx::mx_base_node &in_user_property_ui_node, IXMLNode & inout_parent_node, IXMLNode & inout_meta_node );

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
  // members exposing private parameters
  static bool                is_plane_type_valid (const std::string &in_plane_type); // v25.09.2
  static uint8_t             getPlaneType ();
  static mx_plane_types_enum getPlaneType_enum ();

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

  static std::map<XPLMNavRef, missionx::NavAidInfo> mapNavAidsFromMainThread; // v3.0.221.4 holds nav aid data from main plugin thread so thread will process it later in the background

  bool        exec_generate_mission_thread (const std::string &inKey);
  static void stop_plugin ();

  int get_num_of_flight_legs(); // v3.0.219.14 return how many goals in xFlightLegs node

  static float calc_slope_at_point_mainThread(NavAidInfo& inNavAid);

  // v3.0.255.3 implementing db queries instead of using cached data
  static std::map<std::string, std::string>                          row_gather_db_data;
  static std::unordered_map<int, std::map<std::string, std::string>> resultTable_gather_random_airports;
  static std::unordered_map<int, std::map<std::string, std::string>> resultTable_gather_ramp_data;

  static int callback_gather_random_airports_db(void* data, int argc, char** argv, char** azColName);    // this function will be called for each fetched row
  static int callback_pick_random_ramp_location_db(void* data, int argc, char** argv, char** azColName); // this function will be called for each fetched row

  static NavAidInfo get_random_airport_from_db(missionx::Point& inPoint, float inMinDistance_nm, float inMaxDistance_nm, int inExcludeAngle, missionx::mx_base_node &inProperties, const uint8_t & in_plane_type);

  //// v25.10.2 Find Filter
  static missionx::mx_return gen_get_ramp_based_on_plane_type (missionx::NavAidInfo &     inout_target_navaid
                                                               , const mx_plane_types_enum &in_plane_type_enum_to_search
                                                               , const missionx::mxFilterRampType &inRampFilterType
                                                               , const bool &if_start_ramp_then_force_plane_position = false // v26.03.1
                                                               );

  // ///// weather v3.303.13
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


  // v25.09.2 re-implement: "get_target_or_lastFlightLeg_base_on_XY_or_OSM()", search for airports based on XY information for all planes and for helos it can also be based on OSM data (depends on the location_type value - inLocationType)
  static bool gen_target_or_last_flight_leg_base_on_xy_or_osm(NavAidInfo&       outNewNavInfo,
                                            mx_plane_types_enum                      in_plane_type_enum,
                                            std::map<std::string, std::string>& inMapLocationSplitValues,
                                            missionx::mx_base_node&             inProperties,
                                            NavAidInfo *prev_na_ptr);

  // v25.09.2 Search and pick a pre-defined location based on an XML tag name
  static bool gen_get_target_base_on_tag_name (NavAidInfo &                  outNewNavInfo,
                                                  std::map<std::string, std::string>& inMapLocationSplitValues,
                                                  missionx::mx_base_node &inProperties,
                                                  NavAidInfo *prev_na_ptr);


  static double get_slope_at_point (const missionx::NavAidInfo &outNavAid);
  static bool   get_is_wet_at_point (const missionx::NavAidInfo &inNavAid);
  static void   get_skew_target_data (missionx::NavAidInfo &in_target_navaid); // v25.09.2
  static float  get_terrain_elevation_at_point_in_mt (const missionx::NavAidInfo &inNavAid);


  static void calculate_bbox_coordinates(missionx::Point& outN0, missionx::Point& outS180, missionx::Point& outE90, missionx::Point& outW270, float inRefLat, float inRefLon, double inMaxRadius_d); // v3.0.255.3
  static void gather_all_osm_db_files_names_and_path(std::list<std::string>& outListOfFiles);

  static bool osm_get_navaid_from_osm(NavAidInfo&                         outNavAid,
                               std::map<std::string, std::string>& inMapLocationSplitValues,
                               missionx::mx_base_node&             inProperties, // v3.305.1
                               NavAidInfo*                         prev_navaid_ptr, // v25.10.1
                               double                              min_lat,
                               double                              max_lat,
                               double                              min_lon,
                               double                              max_lon,
                               double                              maxDistance_d = mxconst::SLIDER_MAX_RND_DIST,
                               double minDistance_d = (double)mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT );

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
                                    double                              minDistance_d = (double)mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT);

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
                                        double                              minDistance_d = (double)mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT);
  
  // initialize the sqlite queries we would use.
  static void initQueries();

  // overpass mission_info custom urls
  static std::vector<std::string> vecMissionInfoOverpassUrls;
  static int                      current_url_indx_used_i; // = mxconst::INT_UNDEFINED;

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

  mx_return gen_prepare_medevac_surprise_me(IXMLNode& inRootTemplate, const IXMLNode& inoutMetaNode, const missionx::Point& in_plane_location); // v25.05.1

  static std::vector<missionx::structs::strct_osm_query> gen_osm_analyse (mx_return &out_mx_return, const std::string &xmlFilename, const std::string &in_cache_folder, double centre_lat, double centre_lon, IXMLNode &outRootNode = IXMLNode::emptyIXMLNode);
  // The function returns a "shuffled index vector" as a value, and initializes the "out_main_subject_node" and "analyzed_query" from inside the function to use later from the calling routine.
  static std::vector<int>                    gen_shuffled_q_from_osm_subject_node (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_root_node, const std::vector<missionx::structs::strct_osm_query> &vec_osm_queries, IXMLNode &out_main_subject_node, missionx::structs::strct_osm_query &analyzed_query);
  static std::map<int, missionx::NavAidInfo> gen_get_targets_using_osm_queries_from_a_thread (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_root_node, missionx::structs::strct_osm_query &inout_osm_query, strct_shared_random_airport_info &inout_shared_navaid);
  // find metadata of current target NavAid relative to previous and next NavAids
  static std::string          gen_leg_name (int *seq, const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo &inTargetNavAid);
  static void                 gen_gather_navaid_metadata_relative_to_target (const IXMLNode &inoutMetaNode, missionx::NavAidInfo &inout_target_navaid, missionx::NavAidInfo &inout_from_navaid, missionx::NavAidInfo *inout_next_navaid_ptr);
  static IXMLNode             gen_trigger_node (int &seq, const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo &inTargetNavAid, const std::list<missionx::structs::strct_node_attribute_key_value> &in_attrib_list, IXMLNode *parentNode = nullptr);
  static IXMLNode             gen_task_node (int &seq, const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo &inTargetNavAid, const std::list<missionx::structs::strct_node_attribute_key_value> &in_attrib_list, IXMLNode *parentNode = nullptr);
  static IXMLNode             gen_objective_node (int &seq, const std::string &prefix_name, const std::string &postfix_name, IXMLNode *parentNode = nullptr);
  static IXMLNode             gen_leg_node (const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo *inTargetNavAid, const std::list<missionx::structs::strct_node_attribute_key_value> *in_attrib_list, IXMLNode *parentNode = nullptr);
  static missionx::NavAidInfo gen_briefer_phase_01_parse_briefer_and_start_location (const IXMLNode &in_xTemplate, IXMLNode &x_briefer_and_start_location); // parse <briefer_and_start_location>
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
  static void     gen_messages_when_reaching_target_leg (int &seq_trig, int &seq_msg, NavAidInfo &inout_target_na, IXMLNode &in_metadata_node, IXMLNode &inout_xml_messages, IXMLNode &inout_xml_triggers, const IXMLNode &in_xml_land_trigger, const IXMLNode &in_xml_hover_trigger); // add "you reached the target area" message. Add as trigger
  static void     gen_2nm_to_N_nm_message (int &seq_trig, int &seq_msg, NavAidInfo &inout_target_na, IXMLNode &inout_xml_messages, IXMLNode &inout_xml_triggers, const IXMLNode &in_xml_land_trigger);
  static void     gen_parse_and_add_all_display_objects_in_node (const std::string &in_which_func_called, missionx::NavAidInfo &in_target_navaid, const IXMLNode &in_source_node, IXMLNode &inout_target_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d);
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
  missionx::mx_return                        gen_content_option_01_random_mission_from_content (IXMLNode &xTemplateNode, IXMLNode & xContent); // v25.09.2
  missionx::mx_return                        gen_content_option_02_copy_as_is (IXMLNode &xTemplateNode, IXMLNode & xContent); // v25.09.2
  missionx::mx_return                        gen_prepare_random_mission_based_on_leg_nodes_in_template (IXMLNode &in_xTemplateNode, IXMLNode & inout_meta_node); // v25.09.2 - this represents the original way we constructed a random mission
  void                                       gen_create_all_leg_nodes_based_on_navaid_targets (std::map<int, NavAidInfo> &navaid_targets, bool in_only_2_legs = false);

  static std::string gen_get_cumulative_fpln_desc (std::map<int, NavAidInfo> &navaid_targets);
  // conduct basic validations to figure out if all navaids are valid and how many are valid
  static missionx::mx_return gen_validate_navaids (std::map<int, NavAidInfo> &navaid_targets, int &inout_valid_navaids);
  static IXMLNode gen_set_and_get_start_cold_and_dark (IXMLNode &xTemplateNode, NavAidInfo &navaid);

  // v25.10.2
  // Will return true if the function finds an airport data. out_rw_count and out_longest_rw should return a value greater than zero (0).
  static bool gen_get_rw_metadata (const std::string &in_icao, int &out_rw_count, float &out_longest_rw);

  #ifndef RELEASE_DEBUG
  static void write_targets_to_file (const std::map<int, NavAidInfo>& navaid_targets);
  #endif

public:

  inline static std::map<std::string, mx_plane_types_enum> mapPlaneStringTypesToEnum = {
    {"", missionx::mx_plane_types_enum::plane_type_any},
    {"helos", missionx::mx_plane_types_enum::plane_type_helos},
    {"prop", missionx::mx_plane_types_enum::plane_type_props},
    {"prop_floats", missionx::mx_plane_types_enum::plane_type_prop_floats},
    {"ga", missionx::mx_plane_types_enum::plane_type_ga},
    {"ga_floats", missionx::mx_plane_types_enum::plane_type_ga_floats},
    {"turboprops", missionx::mx_plane_types_enum::plane_type_turboprops},
    {"jet", missionx::mx_plane_types_enum::plane_type_jets},
    {"airline", missionx::mx_plane_types_enum::plane_type_airline},
    {"cargo", missionx::mx_plane_types_enum::plane_type_cargo},
    {"heavy_airline", missionx::mx_plane_types_enum::plane_type_heavy_airline},
    {"heavy_cargo", missionx::mx_plane_types_enum::plane_type_heavy_cargo},
    {"fighter", missionx::mx_plane_types_enum::plane_type_fighter}
  };

  inline static std::map<mx_plane_types_enum, std::string> mapPlaneEnumToStringTypes;
  // Stores the tag names need to fetch from the <MAPPING> part of the template file to use as navaid.fpln_xml_leg
  inline static std::map<int, std::string> map_flight_legs_translation_from_template = {}; // v25.09.1 used with the new oil-rig function since gen_osm template is different.

};

/* namespace missionx */
}

/* RANDOMENGINE_H_ */
#endif
