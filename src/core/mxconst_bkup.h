//
// Created by xplane on 4/16/25.
//

#ifndef MXCONST_H
#define MXCONST_H
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
// #include  "xx_mission_constants.hpp"

namespace missionx {

class mxconst {


public:
  mxconst ();
  mxconst (missionx::mxconst &other);

mxconst &operator= (const mxconst &other)
{
  if (this == &other)
    return *this;

  return *this;
}

  mxconst &operator= (mxconst &&other) noexcept
{
  if (this == &other)
    return *this;

  return *this;
}

typedef struct _mx_test01
{
  std::string value = "Test 1234";
  _mx_test01(){};
  [[nodiscard]] std::string getValue() const
  {
    return value;
  }
} mx_test01_strct;

  static std::string get_test01()
  {
    static mx_test01_strct test01;
    return test01.value;
  }


const std::string  ELEMENT_MESSAGE = "message"; // const std::string  mxconst::get_ELEMENT_MESSAGE() =  "message";

static const std::vector<const char*> vecMarkerTypeOptions; //
static const std::vector<std::string> vecMarkerTypeOptions_markers; //

const static std::string  DEFAULT_CYCLE; //
const static std::string  DEFAULT_FONT_LOCATION_0; //
const static std::string  DEFAULT_FONT_LOCATION_1; //
const static std::string  DEFAULT_FONT_LOCATION_2; //
const static std::string  DEFAULT_FONT_LOCATION_3; //


const static  std::string  TEXT_TYPE_DEFAULT; //
const static  std::string  TEXT_TYPE_DEFAULT_PLUS_1; //
const static  std::string  TEXT_TYPE_TITLE_SMALL; //  // title small 14px
const static  std::string  TEXT_TYPE_TITLE_SMALLEST; //     // title smallest ^ 13px
const static  std::string  TEXT_TYPE_TITLE_REG; //  title regular
const static  std::string  TEXT_TYPE_TITLE_MED; //  title medium
const static  std::string  TEXT_TYPE_TITLE_BIG; //  title big
const static  std::string  TEXT_TYPE_TEXT_REG; //  text_reg
const static  std::string  TEXT_TYPE_TEXT_MED; //  text medium
const static  std::string  TEXT_TYPE_TEXT_SMALL; //  text small
const static  std::string  TEXT_TYPE_MSG_BOTTOM; //  msg_bottom
const static  std::string  TEXT_TYPE_MSG_POPUP; //  msg popup
const static  std::string  TEXT_TYPE_TITLE_TOOLBAR; //  title toolbar

static constexpr  float  FONT_PIXEL_13 =  13.0f; ; // // v3.0.251.1 holds preferred font pixel size
static constexpr  float  FONT_PIXEL_14 =  14.0f; ; // // v3.303.14 holds preferred font pixel size for bigger characters
static constexpr  float  FONT_PIXEL_15 =  15.0f; ; // // v3.303.14 holds preferred font pixel size for bigger characters
static constexpr  float  FONT_PIXEL_16 =  16.0f; ; // // v3.303.14 holds preferred font pixel size for bigger characters
static constexpr  float  FONT_PIXEL_18 =  18.0f; ; // // v3.303.14 holds preferred font pixel size for bigger characters
static constexpr  float  FONT_PIXEL_20 =  20.0f; ; // // v3.303.14 holds preferred font pixel size for bigger characters
static constexpr  float  FONT_PIXEL_22 =  22.0f; ; // // v3.303.14 holds preferred font pixel size for bigger characters
static constexpr  float  FONT_PIXEL_24 =  24.0f; ; // // v3.303.14 holds preferred font pixel size for bigger characters
static constexpr  float  FONT_PIXEL_26 =  26.0f; ; // // v3.303.14 holds preferred font pixel size for bigger characters
static constexpr  float  FONT_PIXEL_32 =  32.0f; ; // // v3.303.14 holds preferred font pixel size for bigger characters

constexpr  static float  DEFAULT_MIN_FONT_PIXEL_SIZE =  12.0f; //
constexpr  static float  DEFAULT_MAX_FONT_PIXEL_SIZE =  16.0f; //

constexpr  static float  DEFAULT_MIN_FONT_SCALE =  0.8f; //
constexpr  static float  DEFAULT_MAX_FONT_SCALE =  1.4f; //
constexpr  static float  DEFAULT_BASE_FONT_SCALE =  1.0f; //


// FILE NAMES
static const  std::string  CUSTOM_APT_DAT_FILE; // // v3.303.12_r2 custom apt dat file that we want to deprecate in near future

// General
const static  std::string  UNIX_EOL; //
const static  std::string  WIN_EOL; //
const static  std::string  QM; //
const static  std::string  SPACE; //
const static  std::string  FOLDER_SEPARATOR; //
const static  std::string  BRIEFER_FOLDER; //
const static  std::string  TEMPLATE_FILE_NAME; // // v3.0.241.10 b2 define a template file in a custom mission folder
const static  std::string  FOLDER_RANDOM_MISSION_NAME; //
const static  std::string  RANDOM_MISSION_DATA_FILE_NAME; //
const static  std::string  COLON; //
const static  std::string  COMMA_DELIMITER; //
const static  std::string  PIPE_DELIMITER; //


constexpr  static double  TWNENTY_METERS_D =  20.0; ; // // v3.0.241.10 b3 - used in position plane. This is the threshold distance in meters that will be converted to NM
constexpr  static double  MIN_ACCEPTABLE_DISTANCE_TO_GUESS_WP_TYPE_NM_D =  1.0;  ; // // v3.0.255.2 - used in guessing waypoint type.


const static  std::string  CONVERTER_FILE; //


// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 ///// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 ///
// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 ///// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 ///
// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 ///// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 /// V3.0.0 ///

// custom dataref
const static  std::string  FILE_CUSTOM_ACF_CACHED_DATAREFS_NAME; // // v3.303.9.1


// Common Attributes
const static  std::string  ATTRIB_MXFEATURE; // / v3.0.255.3
const static  std::string  ATTRIB_XP_VERSION; // / v3.303.12
const static  std::string  ATTRIB_PLUGIN_VERSION; // / v24.02.5
const static  std::string  ELEMENT_SETUP; // / v3.0.255.3 // v3.0.255.4.2 to lower case
const static  std::string  ELEMENT_NODE; // / v3.303.11 part of data_manager::mx_base_node class and should use instead of mxProprties class when only dealing with XML
const static  std::string  ELEMENT_OPTIONS_CAPITAL_LETTERS; // / v3.0.255.4.2 - will replace <options>
const static  std::string  ELEMENT_TEMPLATE_REPLACE_OPTIONS; // // v3.0.255.4.1 used in template injection XML
const static  std::string  ELEMENT_OPT; // // v3.0.255.4
const static  std::string  ELEMENT_FIND_REPLACE; // // v3.0.255.4.1 used in Random::inject_file_info()
const static  std::string  ELEMENT_INFO; // // v3.0.255.4.1 <info> is a sub element of <opt> element. All its attributes in <info> will be copied over mission_info
const static  std::string  ELEMENT_ERROR; // // v3.305.3 use with log formatting
const static  std::string  ELEMENT_WARNING; // // v3.305.3 use with log formatting
const static  std::string  ATTRIB_ID; //
const static  std::string  ATTRIB_TITLE; //
const static  std::string  ATTRIB_OPTIONAL; //

const static  std::string  ATTRIB_STARTING_ICAO; //



static constexpr  int    MAX_ITEM_IN_INVENTORY =  15; ; // // v3.0.213.4 // used in em_reset_inventory_item_table() function.
static constexpr  float  AGL_TO_GATHER_FPM_DATA =  15.0f; ; // // v3.0.213.4 // used in em_reset_inventory_item_table() function.

static constexpr  int  INT_UNDEFINED =  -1; //
static constexpr  int  INT_FIRST_0 =  0; //

const static  std::string  ZERO; //
const static  std::string  ONE; //
const static  std::string  TWO; //
const static  std::string  THREE; //
const static  std::string  FOUR; //
const static  std::string  FIVE; //
const static  std::string  SIX; //
const static  std::string  SEVEN; //
const static  std::string  EIGHT; //
const static  std::string  NINE; //
const static  std::string  TEN; //
const static  std::string  TWELVE; //
const static  std::string  FORTEEN; //
const static  std::string  FIFTEEN; //
const static  std::string  SIXTEEN; //

// Mission Data - XML

// missionx configuration file
const static  std::string  MISSIONX_CONF_FILE; // // v3.0.255.4.1
const static  std::string  MISSIONX_ROOT_DOC; // // v3.0.255.4.1
const static  std::string  ELEMENT_OVERPASS; // // v3.0.255.4.1
const static  std::string  ELEMENT_URL; // // v3.0.255.4.1

// const static std::string_view OVERPASS_XML_URLS ^ R"(<OVERPASS>
//  <url>https://lz4.overpass-api.de/api/interpreter</url>
//  <url>https://z.overpass-api.de/api/interpreter</url>
//  <url>https://overpass.openstreetmap.ru/api/interpreter</url>
//  <url>https://overpass.openstreetmap.fr/api/interpreter</url>
//  <url>https://overpass.kumi.systems/api/interpreter</url>
//</OVERPASS>)";

const static  std::string  OVERPASS_XML_URLS; //


const static  std::string  MISSION_ELEMENT; //
const static  std::string  ELEMENT_METADATA; // // v24.12.1 holds user generated type mission and subcategory.
const static  std::string  ATTRIB_CATEGORY; // // v24.12.1 holds user generated type mission and subcategory.
const static  std::string  ATTRIB_UI_LAYER; // // v24.12.1 from which layer the mission was generated ?
const static  std::string  CONVERSION_ROOT_DOC; //
const static  std::string  DUMMY_ROOT_DOC; //
const static  std::string  TEMPLATE_ROOT_DOC; //
const static  std::string  MAPPING_ROOT_DOC; //
const static  std::string  MISSION_ROOT_SAVE_DOC; //
const static  std::string  ATTRIB_VERSION; // // MISSION
const static  std::string  ATTRIB_MISSION_FILE_FORMAT; // // v25.03.1 used with template. Will hold the target mission file format (301 or 312 as an example)
const static  std::string  ATTRIB_MISSION_DESIGNER_MODE; // // MISSION
const static  std::string  MX_FILE_SAVE_EXTENTION; // // MISSION //
const static  std::string  MX_FILE_SAVE_DREF_EXTENTION; // // MISSION //
const static  std::string  ATTRIB_TYPE; // // trigger // refuel_zone
const static  std::string  ATTRIB_ELEV_MIN_FT; // // v3.0.303.4 trigger elevation
const static  std::string  ATTRIB_ELEV_MAX_FT; // // trigger elevation
const static  std::string  ATTRIB_RE_ARM; // // v3.0.219.1 used in trigger. Will be set when leaving trigger
const static  std::string  XML_EXTENTION; // // v3.0.241.10 b3 refine random osm
const static  std::string  ELEMENT_DESIGNER; //
const static  std::string  ATTRIB_FORCE_LEG_NAME; // // v3.0.221.15rc5 add LEG support. global settings - designer
const static  std::string  ELEMENT_DATAREF; // //
const static  std::string  GLOBAL_SETTINGS; //
const static  std::string  ATTRIB_METAR_FILE_NAME; // //
const static  std::string  ELEMENT_COMPATIBILITY; // // v24.12.2 a global_setting sub element, used to set compatibility flags for older missions.
const static  std::string  ATTRIB_INVENTORY_LAYOUT; // // v24.12.2 force mission inventory layout. Values "[11|12]". USed with <compatibility> element.
const static  std::string  ATTRIB_TIME_HOURS; // // Time     // global settings
const static  std::string  ATTRIB_TIME_MIN; // // global settings
const static  std::string  ELEMENT_START_TIME; //
const static  std::string  ATTRIB_TIME_DAY_IN_YEAR; // // global settings
const static  std::string  ATTRIB_LOCAL_TIME_SEC; // // global settings // v3.303.8.3
const static  std::string  ELEMENT_MISSION_INFO; // // Mission Description // Holds information for mission briefing
const static  std::string  ATTRIB_MISSION_IMAGE_FILE_NAME; // // mission briefing
const static  std::string  ATTRIB_TEMPLATE_IMAGE_FILE_NAME; // // template image
const static  std::string  ATTRIB_PLANE_DESC; // // mission briefing
const static  std::string  ATTRIB_ESTIMATE_TIME; // // mission briefing
const static  std::string  ATTRIB_DIFFICULTY; // // mission briefing
const static  std::string  ATTRIB_WEATHER_SETTINGS; // // weather briefing
const static  std::string  ATTRIB_SCENERY_SETTINGS; // // scenery briefing
const static  std::string  ATTRIB_WRITTEN_BY; // // scenery briefing
const static  std::string  ATTRIB_OTHER_SETTINGS; // // weather briefing
const static  std::string  ATTRIB_SHORT_DESC; // // short description
const static  std::string  ELEMENT_PROPERTIES; // // Save checkpoint // properties, for checkpoint
const static  std::string  ELEMENT_SAVE; // // for checkpoint
const static  std::string  PROP_MISSION_PROPERTIES; // // for checkpoint
const static  std::string  PROP_MISSION_FOLDER_PROPERTIES; // // for checkpoint

const static  std::string  ELEMENT_FOLDERS; // // FOLDER PROPERTIES
const static  std::string  ATTRIB_MISSION_PACKAGE_FOLDER_PATH; // // v3.0.213.7 renamed  from "missions_root_folder_name" to "MISSION_PACKAGE_FOLDER_PATH" for better readability
const static  std::string  ATTRIB_SOUND_FOLDER_NAME; //
const static  std::string  ATTRIB_OBJ3D_FOLDER_NAME; //
const static  std::string  ATTRIB_METAR_FOLDER_NAME; //
const static  std::string  ATTRIB_SCRIPT_FOLDER_NAME; //
const static  std::string  PROP_XPLANE_VERSION; // // v3.303.8
const static  std::string  PROP_XPLANE_INSTALL_PATH; //
const static  std::string  PROP_XPLANE_PLUGINS_PATH; //
const static  std::string  PROP_MISSIONX_PATH; //
const static  std::string  PROP_MISSIONX_BITMAP_PATH; // // v3.0.118
const static  std::string  FLD_MISSIONX_SAVE_PATH; //
const static  std::string  FLD_MISSIONX_LOG_PATH; //
const static  std::string  FLD_MISSIONX_SAVEPOINT_PATH; //
const static  std::string  FLD_MISSIONX_SAVEPOINT_DREF; //
const static  std::string  FLD_MISSIONS_ROOT_PATH; // // currently "custom scenery/missionx"
const static  std::string  FLD_RANDOM_TEMPLATES_PATH; // // v3.0.217.1 currently "Resources/plugin/missionx/templates"
const static  std::string  FLD_RANDOM_MISSION_PATH; // // v3.0.217.1 currently "custom scenery/missionx/random"
const static  std::string  FLD_CUSTOM_SCENERY_FOLDER_PATH; // // v3.0.219.9 currently "custom scenery"
const static  std::string  FLD_DEFAULT_APTDATA_PATH; // // v3.0.219.9 currently "xp11/Resources/default scenery/default apt dat/"
const static  std::string  ATTRIB_NAME; //
const static  std::string  ATTRIB_UNIQUE_ELEMENT_NAME; // // v3.0.217.4
const static  std::string  ELEMENT_DESC; //
const static  std::string  ATTRIB_IS_TARGET_POI; // // v3.0.303 boolean attribute to flag a task as the target position. Used to write the position to xpshared/target_lat|target_lon
const static  std::string  ATTRIB_TARGET_LAT; // // v3.0.303 Store task lat in <leg >
const static  std::string  ATTRIB_TARGET_LON; // // v3.0.303 Store task lon in <leg >
const static  std::string  ATTRIB_IS_DUMMY; // // v3.0.303_12 used with <leg> element. It will ignore all objective/tasks and triggers tests and will only execute: "pre/post scripts", "pre/post commands" and "<display_object{_xxx}>" elements consider to also evaluate linked triggers at <leg> level. Will always flagged as success after one cycle.
const static  std::string  ATTRIB_DUMMY_LEG_ITERATIONS; // // v3.0.303_12 holds a counter, how many times the dummy leg was iterated. If larger than 2 then we can flag it is successfull and continue to next leg


const static  std::string  ELEMENT_METAR; // // METAR
const static  std::string  ATTRIB_INJECT_METAR_FILE; // // v3.0.224.1 used in message element and will be called once text message has been broadcasted.
const static  std::string  XPLANE_METAR_FILENAME; // //
const static  std::string  ATTRIB_FORCE_CUSTOM_METAR_FILE; // // bool value "0 ^ no/false; 1 ^ yes/true
const static  std::string  ELEMENT_MAP; // // Map 2D
const static  std::string  ATTRIB_MANDATORY; //
const static  std::string  PROP_TASK_STATE; //
const static  std::string  ATTRIB_MAP_WIDTH; //
const static  std::string  ATTRIB_MAP_HEIGHT; //
const static  std::string  ATTRIB_MAP_FILE_NAME; //
const static  std::string  ELEMENT_EMBEDDED_SCRIPTS; //
const static  std::string  ELEMENT_FILE; //
const static  std::string  ELEMENT_INCLUDE_FILE; //
const static  std::string  ATTRIB_INCLUDE_FILE; // // for the sake of readability
const static  std::string  ELEMENT_SCRIPTLET; //
const static  std::string  ELEMENT_SHARED_VARIABLES; // // v3.0.202a // root of all shared variables
const static  std::string  ELEMENT_VAR; // // v3.0.202a // represent a shared variable element
const static  std::string  ATTRIB_INIT_VAL; // // v3.0.202a // represent a shared variable element
const static  std::string  ELEMENT_SCRIPT_GLOBAL_STRING_PARAMS; //
const static  std::string  ELEMENT_SCRIPT_GLOBAL_NUMBER_PARAMS; //
const static  std::string  ELEMENT_SCRIPT_GLOBAL_BOOL_PARAMS; //
const static  std::string  ELEMENT_LOGIC; //
const static  std::string  ELEMENT_INTERPOLATION; // // v3.305.3 used with save/load checkpoint
const static  std::string  ELEMENT_XPDATA; //
const static  std::string  ELEMENT_TASK; //
const static  std::string  ATTRIB_BASE_ON_TRIGGER; // // trigger name that will evaluate the task from physical perspectives (can have logical aspects too).
const static  std::string  ATTRIB_BASE_ON_SCRIPT; // // script will evaluate validity of task or other action. Not meant for physical location.
const static  std::string  ATTRIB_BASE_ON_COMMAND; // // v3.0.221.10 Holds command path
const static  std::string  ATTRIB_BASE_ON_SLING_LOAD; // // v3.0.303 sling load
const static  std::string  ATTRIB_IS_PLACEHOLDER; // // v3.0.303.7 placeholder task. Meaning its state will be changed through indirect code, like a script from other trigger or as an outcome of a message or choice
const static  std::string  ATTRIB_EVAL_SUCCESS_FOR_N_SEC; // // Flag task as success only if its conditions are met for at least N seconds
const static  std::string  ATTRIB_FORCE_EVALUATION; // // renamed in v3.0.205.3 from continues_evaluation // "always_evaluate"; ^// Precede "eval_success_for_n_sec". Even it task was flaged as success, it will continue to // evaluate its conditions until Objective will be flagged as success.
const static  std::string  ATTRIB_CUMULATIVE_TIMER_FLAG; // // v3.0.221.11 boolean - flag if timer should be cumulative or not. Example: when evaluating seconds for success, you are not allowed to leave the area of // effect. It will reset the timer. Cumulative will "just" pause and then un-pause the timer.
const static  std::string  ELEMENT_OBJECTIVES; //
const static  std::string  ELEMENT_OBJECTIVE; //
const static  std::string  ATTRIB_TASK_NAME; //
const static  std::string  ATTRIB_DEPENDS_ON_TASK; // // depends on a task in same objective

const static  std::string  ELEMENT_MX_CHOICES; // // v3.0.231.1 plugin root element for all loaded choices. Used internally by the plugin and not the user. also used in save checkpoint.
const static  std::string  ELEMENT_CHOICES; // // v3.0.231.1 The container for choices
const static  std::string  ELEMENT_CHOICE; // // v3.0.231.1
const static  std::string  ELEMENT_OPTION; // // v3.0.231.1
const static  std::string  ELEMENT_OPTION_GROUP; // // v24.12.2
const static  std::string  ATTRIB_TEXT; // // v3.0.231.1
const static  std::string  ATTRIB_ONETIME_OPTION_B; // // v3.0.231.1
const static  std::string  ATTRIB_NEXT_CHOICE; // // v3.0.231.1 - follow up choice to display
const static  std::string  ATTRIB_ACTIVE_CHOICE; // // v3.0.231.1
const static  std::string  ELEMENT_FMS; // // v3.0.231.1
const static  std::string  ELEMENT_GPS; // // v3.0.217.4 holds GPS points for generated mission and maybe to regular missions too
const static  std::string  ATTRIB_GPS_DISPLAY_FULL_ROUTE_B; // // v3.0.253.7
const static  std::string  ATTRIB_DESTINATION_ENTRY; // // v3.0.219.7 add GPS/FMS current destination flying to.
const static  std::string  ELEMENT_FMS_ENTRY; // // v3.0.219.7 add GPS/FMS current destination flying to.
const static  std::string  ELEMENT_GOALS; //
const static  std::string  ELEMENT_GOAL; //
const static  std::string  ELEMENT_FLIGHT_PLAN; // // v3.0.221.15rc4 will replace <goals>
const static  std::string  ELEMENT_LEG; // // v3.0.221.15rc4 will replace <goal>. represent a leg in a flight plan
const static  std::string  ELEMENT_WEATHER; // // v3.0.303.12 sub element <weather> to use in <leg> parent element
const static  std::string  ELEMENT_SAVED_WEATHER; // // v3.0.303.13 sub element <saved_weather> saved <global_settings" to apply the weather during mission start and only if was loaded from save point.
const static  std::string  ELEMENT_SPECIAL_LEG_DIRECTIVES; // // v3.0.221.15rc5 replaces ELEMENT_SPECIAL_GOAL_DIRECTIVES
const static  std::string  ELEMENT_SPECIAL_GOAL_DIRECTIVES; // // v3.0.221.8
const static  std::string  ELEMENT_LINK_TO_OBJECTIVE; //
const static  std::string  ELEMENT_LINK_TO_TRIGGER; //
const static  std::string  ELEMENT_START_LEG_MESSAGE; //
const static  std::string  ELEMENT_START_GOAL_MESSAGE; //
const static  std::string  PROP_START_LEG_MESSAGE_FIRED; // // v3.0.221.15rc5 replaced PROP_START_GOAL_MESSAGE_FIRED // v3.0.213.7 add to goal after message was broadcasted, so won't fire after load checkpoint
const static  std::string  ELEMENT_END_LEG_MESSAGE; // // v3.0.221.15rc5 replaced  ELEMENT_END_GOAL_MESSAGE
const static  std::string  ELEMENT_END_GOAL_MESSAGE; // // compatibility
const static  std::string  ELEMENT_PRE_LEG_SCRIPT; //
const static  std::string  PROP_PRE_LEG_SCRIPT_FIRED; // // v3.0.221.15rc5 replaced PROP_PRE_GOAL_SCRIPT_FIRED // v3.0.213.7 add to goal after script was fired, so won't fire after load checkpoint
const static  std::string  ELEMENT_POST_LEG_SCRIPT; //
const static  std::string  ATTRIB_DISPLAY_AT_POST_LEG_B; // // v3.0.303.11 Used to display 3D Objects only when a flight leg was ended and after "post_leg_script"

const static  std::string  ATTRIB_NEXT_LEG; //
const static  std::string  ATTRIB_NEXT_GOAL; //
const static  std::string  ELEMENT_DRAW_SCRIPT; // // v3.0.224.2 <draw_script name^"" /> allow embeded scripts to be executed in the draw callback.
const static  std::string  DYNAMIC_MESSAGE; // // Leg Messages // v3.0.224.4
const static  std::string  PREFIX_DYN_MESSAGE_NAME; // // v3.0.224.4
const static  std::string  PREFIX_TRIG_DYN_MESSAGE_NAME; // // v3.0.224.4
const static  std::string  ATTRIB_MESSAGE_NAME_TO_CALL; // // v3.0.224.3
const static  std::string  ATTRIB_RELATIVE_TO_TASK; // // v3.0.224.3
const static  std::string  ATTRIB_RELATIVE_TO_TRIGGER; // // v3.0.224.3
const static  std::string  ATTRIB_OVERRIDE_TASK_NAME; // // v3.0.224.3 used only in random templates. Will replace the generated task name with the overrided one. in <expected_location location_type^"xy" // location_value^"tag^loc_acc|nm_between^2-30" override_task_name^"task_name1" override_trigger_name^"trig_name1" />
const static  std::string  ATTRIB_OVERRIDE_TRIGGER_NAME; // // v3.0.224.3 only in random templates. Will replace the generated trigger name with the overrided one
const static  std::string  ATTRIB_DISABLE_AUTO_MESSAGE_B; // // v3.0.223.4 will be used in random mission template in <leg>. This should disable all distance messages that being injected in RandomEngine::injectMessagesWhileFlyingToDestination().
const static  std::string  ATTRIB_SUPPRESS_DISTANCE_MESSAGES_B; // // v25.02.1 // End Leg Messages
const static  std::string  ELEMENT_BRIEFER; //
const static  std::string  ATTRIB_STARTING_LEG; // // v3.0.221.15rc5 replaced ATTRIB_STARTING_GOAL
const static  std::string  ATTRIB_STARTING_GOAL; // // compatibility
const static  std::string  ATTRIB_POSITION_PREF; // // v3.0.301 B4. Valid options: xp10/xp11
const static  std::string  ELEMENT_LOCATION_ADJUST; //
const static  std::string  ATTRIB_LAT; //
const static  std::string  ATTRIB_LONG; //
const static  std::string  ATTRIB_LAT_OSM; // // v3.0.253.4 osm latitude
const static  std::string  ATTRIB_LONG_OSM; // // v3.0.253.4 osm longitude
const static  std::string  ATTRIB_ELEV_FT; // // under trigger
const static  std::string  ATTRIB_HEADING_PSI; // // psi - The true heading of the aircraft in degrees from the Z axis - OpenGL coordinates
const static  std::string  ATTRIB_FORCE_HEADING_B; // // v3.0.301 B4, force heading of plane even if plane is in 20m radius from starting location. Mainly for designers and not the simmers
const static  std::string  ATTRIB_FORCE_POSITION_B; // // v3.303.12_r2
const static  std::string  ATTRIB_ADJUST_HEADING; // // v3.0.207.5 +/- values to adjust heading of moving object
const static  std::string  ATTRIB_PITCH; // //
const static  std::string  ATTRIB_ROLL; // //
const static  std::string  ATTRIB_STARTING_SPEED_MT_SEC; // // starting speed in meters per seconds
const static  std::string  ATTRIB_PAUSE_AFTER_LOCATION_ADJUST; //
const static  std::string  PROP_CURRENT_LOCATION; //
const static  std::string  PROP_POINT_DATA; //
const static  std::string  ATTRIB_RAMP_INFO; // // v3.0.253.1 ramp info is used in RandomEngine::prepare_mission_based_on_external_fpln() for briefer debug info
const static  std::string  ATTRIB_START_LAT; // // Sling Load - v3.0.303
const static  std::string  ATTRIB_START_LON; //
const static  std::string  ATTRIB_END_LAT; //
const static  std::string  ATTRIB_END_LON; //

const static  std::string  ATTRIB_INIT_SCRIPT; //
const static  std::string  PROP_OBJECTS_ROOT; // // 3D Objects // v3.0.200 // savepoint
const static  std::string  PROP_OBJECTS_INSTANCES; // // savepoint
const static  std::string  ELEMENT_OBJECT_TEMPLATES; //
const static  std::string  ELEMENT_OBJ3D; //
const static  std::string  ELEMENT_PATH; // // v3.0.207.1 renamed to "path". Holds points of movement path
const static  std::string  ATTRIB_CYCLE; // // v3.0.207.1 cycle path ?
const static  std::string  PROP_POINT_FROM; // // v3.0.213.7  obj3d.pointFrom
const static  std::string  PROP_POINT_TO; // // v3.0.213.7  obj3d.pointTo
const static  std::string  PROP_CURRENT_POINT_NO; // // v3.0.213.7 obj3d.currentPointNo
const static  std::string  PROP_INSTANCE_DATA_ELEMENT; // // v3.0.213.7 obj3d instance element for checkpoint
const static  std::string  PROP_INSTANCE_SPECIAL_PROPERTIES; // // v3.0.213.7 obj3d instance element for checkpoint
const static  std::string  PROP_GOAL_INSTANCES_SPECIAL_DATA_ELEMENT; // // v3.0.213.7 Will use after load checkpoint to apply on loaded instances (if any)
const static  std::string  PROP_LOADED_FROM_CHECKPOINT; // // v3.0.213.7 we will add this to Goal elements, so we will know that instance objects should be read from the instance data in the checkpoint file.
const static  std::string  PROP_GOAL_MAP2D; // // v3.0.213.7 we will add this to Goal elements, so we will know that instance objects should be read from the instance data in the checkpoint file.
const static  std::string  ELEMENT_LOCATION; // // lat long elev_ft
const static  std::string  ELEMENT_TILT; // // heading pitch roll
const static  std::string  ELEMENT_DISPLAY_OBJECT; // // "flight_leg" element. Inform plugin to add new instance for specific obj3d template when goal became active
const static  std::string  ELEMENT_DISPLAY_OBJECT_SET; // // v3.0.219.12+ points to an element name and its sub element tag name <display_object_set name^"<main tag name>" template^"<sub tag name>". Holds 3D Objects to display in target location
const static  std::string  ELEMENT_DISPLAY_OBJECT_NEAR_PLANE; // // v3.303.12 "flight_leg" element. Inform plugin to add new instance relative to plane position
const static  std::string  ATTRIB_REPLACE_LAT; // // v3.0.217.6
const static  std::string  ATTRIB_REPLACE_LONG; // // v3.0.217.6
const static  std::string  ATTRIB_REPLACE_ELEV_FT; // //  v3.0.217.6
const static  std::string  ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT; // //  v3.0.217.6
const static  std::string  ATTRIB_REPLACE_DISTANCE_TO_DISPLAY_NM; // //  v3.0.303.3
const static  std::string  ATTRIB_INSTANCE_NAME; // // unique name for 3D object template
const static  std::string  ATTRIB_TARGET_MARKER_B; // // v3.0.241.7 mark an instance as a target marker. Can be helful to show hide these specific markers in random MED missions
const static  std::string  ATTRIB_OBJ3D_TYPE; //
const static  std::string  ATTRIB_IS_VIRTUAL_B; // // v3.0.255.4 - if yes then treat the file name as a virtual library object to load and do not use the mission folder "obj" folder.
const static  std::string  ATTRIB_ALTERNATE_OBJ_FILE; //
const static  std::string  ATTRIB_DISTANCE_TO_DISPLAY_NM; // // v3.0.200
const static  std::string  ATTRIB_KEEP_UNTIL_GOAL; // // v3.0.200 ***** converted to ATTRIB_KEEP_UNTIL_LEG *****
const static  std::string  ATTRIB_KEEP_UNTIL_LEG; // // v3.0.221.15rc5 add LEG support
const static  std::string  ATTRIB_HIDE; // // v3.0.207.4 // boolean value that will flag a 3D Instance to hide
const static  std::string  PROP_DISPLAY_DEFAULT_OBJECT_FILE_OVER_ALTERNATE; // // v3.0.200 // boolean. Which 3D Object to display the default or: "alternate_obj_file" attribute one.
const static  std::string  PROP_CAN_BE_DISPLAYED; // // v3.0.200 // boolean. can an instance be drawn in X-Plane world ?
const static  std::string  ATTRIB_ELEV_ABOVE_GROUND_FT; // // v3.0.200 // boolean. can an instance be drawn in X-Plane world ?
const static  std::string  PREFIX_REPLACE_; // // v3.0.200 // all attributes we want to replace, should start with replace_xxx, example: replace_lat, replace_lon. The work after "replace_" must be the same as the original attribute, // so we will be able to find it in the property.
const static  std::string  ATTRIB_LINK_TASK; // // v3.0.200 // used with "display_object" element when we want to add task state to the "show/hide" conditions with "cond_script, keep_until.., display_". Value format: "objective.task"

const static  std::string  PROP_LINK_OBJECTIVE_NAME; // // v3.0.200 // attrib_task is split to task and objects then this property holds the object name task is in.
const static  std::string  ATTRIB_SPEED_KMH; // // km per hour
const static  std::string  ATTRIB_WAIT_SEC; //
const static  std::string  ELEMENT_TRIGGERS; // // Triggers
const static  std::string  ELEMENT_TRIGGER; //
const static  std::string  ELEMENT_SCRIPTS; //
const static  std::string  ELEMENT_SET_DATAREFS; // // v3.303.12 used in "on_enter" trigger
const static  std::string  ELEMENT_SET_DATAREFS_ON_EXIT; // // v3.303.12 "on_exit" trigger
const static  std::string  ELEMENT_CONDITIONS; //
const static  std::string  ATTRIB_PLANE_ON_GROUND; // // bool. does trigger fires when plane is on ground ?
const static  std::string  ATTRIB_COND_SCRIPT; // // script to evaluate trigger as part of the trigger firing conditions
const static  std::string  ATTRIB_POST_SCRIPT; //
const static  std::string  ATTRIB_SCRIPT_NAME_WHEN_FIRED; // // v3.0.213.4 script_name_when_enter was renamed to script_name_when_fired
const static  std::string  ATTRIB_SCRIPT_NAME_WHEN_LEFT; // // when plane leaves trigger area
const static  std::string  ELEMENT_LOC_AND_ELEV_DATA; // //
const static  std::string  ELEMENT_POINT; // //
const static  std::string  ELEMENT_REFERENCE_POINT; // // center of polygonal area or a reference point for triggers based script.
const static  std::string  ELEMENT_RADIUS; // //
const static  std::string  ELEMENT_RECTANGLE; // // v3.0.253.5 used in poly triggers and auto rectangular trigger creation, just like the radius and radius length.
const static  std::string  ATTRIB_DIMENSIONS; // // v3.0.253.5 used in poly triggers and auto rectangular trigger creation. This will determined the bearing to calculate the rectangular.
const static  std::string  DEFAULT_RECT_DIMENTIONS; // // v3.0.253.5 used in poly triggers and auto rectangular trigger creation. This will determined the bearing to calculate the rectangular.
const static  std::string  ATTRIB_FIRST_POINT_IS_CENTER_B; // // v3.0.301 B4 used with flight plan conversion screen, when creating centered box triggers
const static  std::string  ATTRIB_VECTOR_BT_LENGTH_MT; // // v3.0.301 B4 bottom to top length in meters (height)
const static  std::string  ATTRIB_VECTOR_LR_LENGTH_MT; // // v3.0.301 B4 left to right length in meters (width)
const static  std::string  ELEMENT_ELEVATION_VOLUME; // //
const static  std::string  ATTRIB_ELEV_MIN_MAX_FT; // // volume of trigger [ --N|++N] or [min|max]
const static  std::string  ATTRIB_ELEV_LOWER_UPPER_FT; // // volume of trigger [ --N|++N] or [lower|upper] in feet // v3.0.205.3
const static  std::string  ATTRIB_LENGTH_MT; // // radius length in meter
const static  std::string  ATTRIB_ENABLED; // // trigger
const static  std::string  TRIG_TYPE_RAD; // // Trigger types // radius
const static  std::string  TRIG_TYPE_POLY; // // polygon zone
const static  std::string  TRIG_TYPE_SLOPE; // // slope
const static  std::string  TRIG_TYPE_SCRIPT; // //
const static  std::string  TRIG_TYPE_CAMERA; // // v3.0.223.7 we can add camera_rad or camera_poly if designers will really really want this. Default is "rad"

const static  std::string  ATTRIB_ANGLE; // // slope related
const static  std::string  ELEMENT_CALC_SLOPE; // // slope related // calculate 3 point slope from user data and one origin point
const static  std::string  ATTRIB_BEARING_2D; // // slope related // calculate 3 point slope from user data and one origin point
const static  std::string  ATTRIB_LENGTH_WITH_UNITS; // // slope related // calculate 3 point slope from user data and one origin point
const static  std::string  ATTRIB_SLOPE_ANGLE_3D; // // slope related // calculate 3 point slope from user data and one origin point
const static  std::string  PROP_HAS_CALC_SLOPE; // // slope related // special boolean property to flag that we read <calc_slope> and it is valid
const static  std::string  ELEMENT_CARGO_CATEGORIES; // // v24.05.1 /// INVENTORIES // v3.0.213.1
const static  std::string  ELEMENT_INVENTORIES; //
const static  std::string  ELEMENT_INVENTORY; //
const static  std::string  ELEMENT_PLANE; //
const static  std::string  ELEMENT_ITEM_BLUEPRINTS; //
const static  std::string  ELEMENT_ITEM; // // inventory item
const static  std::string  ATTRIB_WEIGHT_KG; //
const static  std::string  ATTRIB_QUANTITY; //
const static  std::string  ATTRIB_BARCODE; //




const static  std::string  ELEMENT_MESSAGE_TEMPLATES; // // MESSAGES
const static  std::string  ELEMENT_MESSAGES; //
const static  std::string  ATTRIB_MESSAGE_NAME_WHEN_FIRED; // // trigger message //
const static  std::string  ATTRIB_MESSAGE_NAME_WHEN_LEFT; // // trigger message
const static  std::string  ATTRIB_ORIGINATE_MESSAGE_NAME; // // v3.0.223.7 will be set in SF once it is created. Will be used with background SF only (for now)
const static  std::string  ATTRIB_MESSAGE_NAME_WHEN_ENTERING_PHYSICAL_AREA; // // v3.305.1b useful in triggers when we want to send a message even before "all conditions are met".
const static  std::string  ELEMENT_OUTCOME; // // v3.0.221.11+ trigger commands // trigger other outcomes
const static  std::string  ATTRIB_COMMANDS_TO_EXEC_WHEN_FIRED; // // v3.0.221.11+ trigger commands
const static  std::string  ATTRIB_COMMANDS_TO_EXEC_WHEN_LEFT; // // v3.0.221.11+ trigger commands
const static  std::string  ATTRIB_DATAREF_TO_EXEC_WHEN_FIRED; // // v3.0.221.11+ trigger commands
const static  std::string  ATTRIB_DATAREF_TO_EXEC_WHEN_LEFT; // // v3.0.221.11+ trigger commands
const static  std::string  ATTRIB_SET_OTHER_TASKS_AS_SUCCESS; // // v25.02.1
const static  std::string  ATTRIB_RESET_OTHER_TASKS_STATE; // // v25.02.1
const static  std::string  ATTRIB_SET_OTHER_TRIGGERS_AS_SUCCESS; // // v25.02.1
const static  std::string  ELEMENT_MIX; // // hold message channels information
const static  std::string  ATTRIB_MESSAGE; //
const static  std::string  ATTRIB_MESSAGE_TYPE; // // message and queues // comm, back(ground), pad message.
const static  std::string  PROP_IS_MXPAD_MESSAGE; // // Internal flag. Define message as PAD message or not.
const static  std::string  PROP_MESSAGE_HAS_TEXT_TRACK; // // created during class init() and needs to be removed once we read "text" channel
const static  std::string  ATTRIB_MESSAGE_MIX_TRACK_TYPE; // // message and queues // comm, back(ground), pad message.
const static  std::string  ATTRIB_PARENT_MESSAGE; // // SoundFragment holds the message name it was defined in so we can stop specific communication channel or background sound.
const static  std::string  ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_PLAY; // // Message/seconds to play/display message. Will override the queMessage display timer rules. Can be used for sound file too but need implementation.
const static  std::string  ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT; // // Message/seconds to play/display message. Will override the queMessage display timer rules. Can be used for sound file too but need implementation.
const static  std::string  ATTRIB_MESSAGE_OVERRIDE_SECONDS_CALC_PER_LINE; // // used when plugin calculate per line /seconds to play/display message. Will override the queMessage display timer rules. Can be used for sound file too but need implementation.
const static  std::string  ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR; // // if mute then we won't use "speak string"
const static  std::string  ATTRIB_MESSAGE_HIDE_TEXT; // // Message/Desc used under
const static  std::string  ATTRIB_SOUND_FILE; // // Message/Desc used under
const static  std::string  ATTRIB_SOUND_VOL; // // Message/Desc used under
const static  std::string  ATTRIB_ORIGINAL_SOUND_VOL; // // v3.306.1b used with <mix background> when we have repeat command we should consider using this attribute
const static  std::string  ATTRIB_TRACK_INSTRUCTIONS; // // v3.0.303.6 sound directive, special string command to tell the mixture how to configure the sound volume during playtime
const static  std::string  ATTRIB_TIMER_TYPE; // // v3.306.1 timr type, used with background mixture
const static  std::string  PROP_TIMER_TYPE_XP; // // v3.306.1 timer type "xp" default
const static  std::string  PROP_TIMER_TYPE_OS; // // v3.306.1 timer type os.

const static  std::string  ATTRIB_CHARACTERS; // // v3.305.1 used in <mix>, format "code|label|hex color", example "j|Jeff|#B17DCB"
const static  std::string  PROP_IMAGE_FILE_NAME; // // mxpad image file name
const static  std::string  PROP_TEXT_RGB_COLOR; // // mxpad text color. there should be translation
const static  std::string  ATTRIB_LABEL_PLACEMENT; // // v3.0.223.4 L or R. Also replaces "mxpad_label_placement"
const static  std::string  ATTRIB_LABEL; // // text label // v3.0.223.4
const static  std::string  ATTRIB_LABEL_COLOR; // // label color // v3.0.223.4
const static  std::string  ELEMENT_MXPAD_ACTIVE_MESSAGES; // // for savepoint, represent the current messages in MxPad window
const static  std::string  ELEMENT_MXPAD_ACTION_REQUEST; // // for savepoint, represent the current messages in MxPad window
const static  std::string  ELEMENT_MXPAD_DATA; // // for savepoint, represent the root MXPAD related elements
const static  std::string  PROP_CURRENT_MX_PAD_RUNNING; // // savepoint. stores current running mxpad script name. We need to init after load
const static  std::string  PROP_NEXT_MSG; // // v3.0.223.1 automatic find and send next message
const static  std::string  ATTRIB_MODE; // // v3.305.1 Used with message, empty or story message mode.
const static  std::string  ATTRIB_FADE_BG_CHANNEL; // // v3.305.3 seconds to auto mute and stop the background mix channel. Format: "message name where bg started,seconds to fade"
const static  std::string  ATTRIB_ADD_MINUTES; // // v3.0.223.1 add minutes as timelapse. We should configure
const static  std::string  ATTRIB_TIMELAPSE_TO_LOCAL_HOURS; // // v3.0.223.1 timelapse to next local hour in format "24H:MI"
const static  std::string  ATTRIB_SET_DAY_HOURS; // // v3.0.223.1 Immediate set time and day in format "day:24H:MI"
const static  std::string  MESSAGE_MODE_STORY; // // v3.305.3
const static  std::string  CHANNEL_TYPE_TEXT; // // comm / back / text
const static  std::string  CHANNEL_TYPE_COMM; // // comm / back / text
const static  std::string  CHANNEL_TYPE_BACKGROUND; // // comm / back / text
const static  std::string  ELEMENT_END_MISSION; // // MESSAGES
const static  std::string  ELEMENT_END_MISSION_SUCCESS; //
const static  std::string  ELEMENT_END_MISSION_FAIL; //
const static  std::string  ELEMENT_END_SUCCESS_IMAGE; //
const static  std::string  ELEMENT_END_SUCCESS_MSG; //
const static  std::string  ELEMENT_END_SUCCESS_SOUND; //
const static  std::string  ELEMENT_END_FAIL_IMAGE; //
const static  std::string  ELEMENT_END_FAIL_MSG; //
const static  std::string  ELEMENT_END_FAIL_SOUND; //
const static  std::string  ATTRIB_OPEN_CHOICE; // // v3.0.303.7
const static  std::string  ATTRIB_FILE_NAME; // // MAP
const static  std::string  ATTRIB_FILE_PATH; //
const static  std::string  ATTRIB_FULL_FILE_PATH; // // v3.0.217.2
const static  std::string  ATTRIB_REAL_W; // // real width
const static  std::string  ATTRIB_REAL_H; // // real height
const static  std::string  ELEMENT_MXPAD; // // mxpad // MXPAD
const static  std::string  ATTRIB_MANAGE_SCRIPT; // // script that manages the mxpad progress

const static  std::string  ATTRIB_STARTING_FUNCTION; // // When calling manage_script for first time, which function should be first to be evaluated.
const static  std::string  PROP_NEXT_RUNNING_FUNCTION; // // script that manages the mxpad progress
const static  std::string  PROP_CURRENT_RUNNING_FUNCTION; // // script that manages the mxpad progress
const static  std::string  PROP_PREV_RUNNING_FUNCTION; // // script that manages the mxpad progress
const static  std::string  PROP_IS_LINKED; // // PROPERTIES representing core attributes          // boolean property to flag a trigger/task as linked. Add it during Objective validation and use it to store "checkpoint" and load/set mission.
const static  std::string  PROP_LINKED_TO; // // boolean property to flag a trigger as linked to a task. checkpoint save/load
const static  std::string  PROP_All_COND_MET_B; // // boolean property to flag a trigger as success
const static  std::string  PROP_PLANE_IN_PHYSICAL_AREA; // // boolean property to flag plane in trigger area // v3.303.14
const static  std::string  PROP_PLANE_IN_ELEV_VOLUME; // // boolean property to flag plane in trigger volume elevation // v3.303.14
const static  std::string  PROP_PLANE_ON_GROUND; // // boolean property to flag plane on ground v3.305.1c
const static  std::string  PROP_SCRIPT_COND_MET_B; // // boolean property to flag a condition script as success
const static  std::string  PROP_STATE_ENUM; // // enum property describes state of Trigger or Flight Leg
const static  std::string  PROP_TRIG_ELEV_TYPE; // // enum property to describe trigger elevation volume
const static  std::string  PROP_IS_VALID; // // boolean property to flag a goal/objective state
const static  std::string  PROP_HAS_MANDATORY; // // boolean property to flag a goal/objective with Mandatory Objective
const static  std::string  PROP_IS_COMPLETE; // // boolean property to flag a task is completed or not (achieved or not)
const static  std::string  PROP_IS_FIRST_TIME; // // v3.0.241.1 boolean property to flag a flight leg as first time or not. Relevant after loading checkpoint, to not execute all first time messages and settings.
const static  std::string  PROP_COMPLETE_DESC; // // boolean property to flag a task is completed successfully or not.
const static  std::string  PROP_ERROR_REASON; // // holds task error explanation
const static  std::string  PROP_COUNTER; // // message counter
const static  std::string  PROP_MESSAGE_BROADCAST_FOR; // // message, to whom was broadcast
const static  std::string  PROP_MESSAGE_TRACK_NAME; // // message track name for Queue Message Manager
const static  std::string  PROP_POINTS; // //
const static  std::string  PROP_HAS_ALWAYS_TASK; // // objective attribute
const static  std::string  PROP_MISSION_FILE_LOCATION; // // v3.0.255.3
const static  std::string  ATTRIB_DREF_KEY; // // Datarefs
const static  std::string  PROP_MISSION_CURRENT_LEG; // // savepoint // // Mission properties
const static  std::string  PROP_MISSION_STATE; // // MISSION // savepoint
const static  std::string  PROP_MISSION_ABORT_REASON; // // MISSION // savepoint
const static  std::string  SAVEPOINT_PLANE_LATITUDE; // // MISSION // savepoint // Mission Savepoint plane location
const static  std::string  SAVEPOINT_PLANE_LONGITUDE; // // MISSION // savepoint
const static  std::string  SAVEPOINT_PLANE_ELEVATION; // // MISSION // savepoint
const static  std::string  SAVEPOINT_PLANE_SPEED; // // MISSION // savepoint // groundspeed
const static  std::string  SAVEPOINT_PLANE_HEADING; // // MISSION // savepoint // psi

const static  std::string  MX_; // // embedded script - seeded attributes
const static  std::string  EXT_MX_FUNC_CALL; //
const static  std::string  EXT_MX_CURRENT_LEG; // // v3.0.221.15rc5 replaces mxCurrentGoal
const static  std::string  EXT_MX_CURRENT_OBJ; //
const static  std::string  EXT_MX_CURRENT_TASK; //
const static  std::string  EXT_MX_CURRENT_TRIGGER; //
const static  std::string  EXT_MX_CURRENT_3DOBJECT; // // v3.0.200
const static  std::string  EXT_MX_CURRENT_3DINSTANCE; // // v3.0.200
const static  std::string  EXT_MX_QM_MESSAGE; // // v3.0.223.1 holds the message name
const static  std::string  EXT_mxState; // // Task get info // string "success/was_success/need_evaluation"
const static  std::string  EXT_mxType; // // "mxType"; ^// string trigger/script/undefined
const static  std::string  EXT_mxTaskActionName; // // string // action_code_name
const static  std::string  EXT_mxTaskHasBeenEvaluated; // // bool


const static  std::string EXT_mxNavType; // // Navigation seed attributes
const static  std::string EXT_mxNavLat; //
const static  std::string EXT_mxNavLon; //
const static  std::string EXT_mxNavHeight; //
const static  std::string EXT_mxNavFreq; //
const static  std::string EXT_mxNavHead; //
const static  std::string EXT_mxNavID; //
const static  std::string EXT_mxNavName; //
const static  std::string EXT_mxNavRegion; //
const static  std::string EXT_mxCargoPosLat; // // Cargo Information for SlingLoad scripts // v3.0.303 Sling Load
const static  std::string EXT_mxCargoPosLon; //
const static  std::string QMM_DUMMY_MSG_NAME_PREFIX; // // Queue Message Manager - QMM // used to construct new messages from code
const static  std::string ELEMENT_BASE_WEIGHTS_KG; // // Base weights by designer // options // mute any sound, include narrator and fmod sounds
const static  std::string ATTRIB_PILOT; // // v3.0.213.4 pilot weight in kg
const static  std::string ATTRIB_PASSENGERS; // // v3.0.213.4 passengers weight in kg
const static  std::string ATTRIB_STORAGE; // // v3.0.213.4 storage weight in kg
const static  std::string ELEMENT_POSITION; // // v3.0.223.1 Hidden parameter for now. Will skip plane repositioning at random start.
const static  std::string ATTRIB_AUTO_POSITION_PLANE; // // v3.0.223.1 Hidden attribute. Boolean type. true^position plane (default), false^skip position plane.
const static  std::string OPT_MUTE_MX_SOUNDS; // // OPTIONS // options // mute any sound, include narrator and fmod sounds
const static  std::string OPT_ABORT_MISSION_ON_CRASH; // // options // bool // abort mission on crash
const static  std::string OPT_ENABLE_DESIGNER_MODE; // // options // bool //
const static  std::string OPT_DISPLAY_VISUAL_CUES; // // options // bool //
const static  std::string OPT_AUTO_HIDE_SHOW_MXPAD; // // v3.0.215.1 auto hide mxpad options
const static  std::string OPT_AUTO_PAUSE_IN_2D; // // v3.0.253.9.1
const static  std::string OPT_AUTO_PAUSE_IN_VR; // // v3.0.221.6
const static  std::string OPT_CYCLE_LOG_FILES; // // v25.03.1
const static  std::string OPT_DISPLAY_MISSIONX_IN_VR; // // v3.0.221.7
const static  std::string OPT_WRITE_CONVERTED_FPLN_TO_XPLANE_FOLDER; // // v3.0.241.2
const static  std::string OPT_DISABLE_PLUGIN_COLD_AND_DARK_WORKAROUND; // // v3.0.221.10
const static  std::string OPT_OVERPASS_FILTER; // // v3.0.253.4
const static  std::string OPT_OVERPASS_URL; // // v3.0.253.6 store overpass url
const static  std::string OPT_GPS_IMMEDIATE_EXPOSURE; // // v3.0.253.7 Display all points of global GPS in FMS or one by one
const static  std::string OPT_FLIGHT_LEG_PROGRESS_COUNTER; // // v3.0.253.7 flight_leg_progress_counter_i


static constexpr  float    MINIMUM_EXPECTED_PILOT_WEIGHT_IN_STATION =  40.0f; ; // // v24.12.2 used when evaluating if to add pilot weight to the plane.
static constexpr  int      MAX_LAPS_I =  24; ; // // v3.0.223.1
static constexpr  int      MXPAD_LINE_WIDTH =  50; //
static constexpr  bool     DEFAULT_AUTO_PAUSE_IN_VR =  false; ; // // v3.0.255.4.2
static constexpr  bool     DEFAULT_AUTO_PAUSE_IN_2D =  true; ; // // v3.0.255.4.2
static constexpr  bool     DEFAULT_DISPLAY_MISSIONX_IN_VR =  true; ; // // v3.0.255.4.2
static constexpr  bool     DEFAULT_WRITE_CONVERTED_FMS_TO_XPLANE_FOLDER =  false; ; // // v3.0.255.4.2
static constexpr  bool     DEFAULT_DISABLE_PLUGIN_COLD_AND_DARK =  false; ; // // v3.0.255.4.2
static constexpr  float    DEFAULT_PILOT_WEIGHT =  85.0f; ; // // v3.0.213.3  85kg // calculate weight related
static constexpr  float    DEFAULT_STORED_WEIGHT =  5.0f; ; // // v3.0.213.3  5kg
static constexpr  float    DEFUALT_3D_OBJECT_SPEED_KMH =  10.0f; ; // // MOVING OBJECT Constants
static constexpr  float    MANDATORY_R =  0.99f; //
static constexpr  float    MANDATORY_G =  0.001f; //
static constexpr  float    MANDATORY_B =  0.001f; //
static constexpr  float    DEFAULT_R =  0.9f; //
static constexpr  float    DEFAULT_G =  0.65f; //
static constexpr  float    DEFAULT_B =  0.0f; //
static constexpr  int      FONT_HEIGHT =  8; ; // // 14; ^// x-plane default font height
static constexpr  int      MAX_MXPAD_SELECTIONS =  6; ; // // ####### MXPAD SELECTION ####
static constexpr  double   MAX_AWAY_SKEWED_DISTANCE_NM =  0.5; ; // // v3.0.253.12 added for general use in all classes and not just RandomMission:get_skewed_target_position() function
static constexpr  float    DEFAULT_RANDOM_POINT_JUMP_NM =  0.5f; ; // // v3.0.219.5 used in RandomEngine::getRandomAirport(). Represent how much nautical miles to jump each airport search
static constexpr  int      MAX_DISTANCE_TO_SEARCH_AIRPORT =  2500; ; // // v3.0.219.5 used in RandomEngine::getRandomAirport(). Represent the maximum distance we allow to search for next airport in nautical miles.
static constexpr  float    MIN_DISTANCE_TO_SEARCH_AIRPORT =  1.5f; ; // // v3.0.219.12 used in RandomEngine::getRandomAirport(). Represent the minimum distance to filter out airports.
static constexpr  int      DEFAULT_RANDOM_DEGREES_TO_EXCLUDE =  20; ; // // v3.0.219.5 used in RandomEngine::getRandomAirport(). Represent the degrees to include in the "exclude" configuration
static constexpr  float    DEFAULT_MAX_SLOPE_TO_LAND_ON =  6.0f; ; // // v3.0.219.11
static constexpr  double   SLIDER_SHORTEST_MIN_ILS_SEARCH_RADIUS =  50; ; // // ILS LAYER // v3.0.253.6 holds the minimum search ILS airport
static constexpr  float    SLIDER_SHORTEST_MAX_ILS_SEARCH_RADIUS =  250.0; ; // // v3.0.253.6 holds the lowest MAX radius we will search for ILS airports.
static constexpr  float    SLIDER_ILS_MAX_SEARCH_RADIUS =  9000.0; ; // // v3.0.253.6 holds the MAX radius we will search for ILS airports.
static constexpr  int      SLIDER_ILS_SHORTEST_RW_LENGTH_MT =  740; ; // // v3.0.253.6 holds the shortest slider value allowed for runway length (ILS layer).
static constexpr  int      SLIDER_ILS_LOGEST_RW_LENGTH_MT =  3740; ; // // v3.0.253.6 holds the longest slider value for runway length (ILS layer)
static constexpr  int      SLIDER_ILS_STARTING_RW_LENGTH_VALUE =  1000; ; // // v3.0.253.6 holds the longest slider value for runway length (ILS layer)
static constexpr  int      SLIDER_ILS_SHORTEST_RW_WIDTH_MT =  15; ; // // v3.0.253.6 holds the shortest slider value allowed for runway width (ILS layer).
static constexpr  int      SLIDER_ILS_WIDEST_RW_WIDTH_MT =  70; ; // // v3.0.253.6 holds the longest slider value for runway width (ILS layer)
static constexpr  int      SLIDER_ILS_STARTING_RW_WIDTH_VALUE =  45; ; // // v3.0.253.6 holds the longest slider value for runway width (ILS layer)
static constexpr  int      SLIDER_ILS_LOWEST_AIRPORT_ELEV_FT =  -80; ; // // v3.0.253.6 holds the lowest slider value allowed for airport elevation (ILS layer).
static constexpr  int      SLIDER_ILS_HIGHEST_AIRPORT_ELEV_FT =  7000; ; // // v3.0.253.6 holds the highest slider value for airport elevation (ILS layer)
static constexpr  int      SLIDER_ILS_STARTING_AIRPORT_ELEV_VALUE_FT =  0; ; // // v3.0.253.6 holds the default slider value for airport elevation (ILS layer)
static constexpr  double   SLIDER_MIN_RND_DIST =  5.0; ; // // SETUP Screen
static constexpr  double   SLIDER_MAX_RND_DIST =  50.0; //
static constexpr  float    MAX_RAD_4_OSM_MAX_DIST =  20.0; ; // // v25.12.2
static constexpr  int      DEFAULT_SETUP_MISSION_VOLUME_I =  30; ; // // v3.0.303.6 holds default volume value
static constexpr  float    NEAR_ENOUGH_DISTANCE_FOR_CUSTOM_NAVAID_SCENERY =  20.0f; ; // // v3.0.253.6 used when picking near navaid and we want to also consider custom scenery, so we first pick the closest scenery and then loop over the navaids that have custom scenery and check their distance relative to the one we know is nearest. In ^20nm distance from nearest we should pick the custom scenery navaid over the default one.
static constexpr  int      PERCENT_TO_PICK_CUSTOM_SCENERY_OVER_GENERIC =  40; ; // // v3.0.253.6 used in RandomEngine::getRandomAirport_localThread()
static constexpr  double   SLING_LOAD_SUCCESS_RADIUS_MT =  10.0; ; // // sling load success radius in meters
static constexpr  float    MIN_SOUND_VOLUME_F =  0.0f; ; // // SOUND related
static constexpr  float    MAX_SOUND_VOLUME_F =  100.0f; //
static constexpr  float    GATHER_STATS_INTERVAL_SEC =  1.0f; ; // // gather stats


constexpr const static  char  *SETUP_PILOT_NAME =  "setup_pilot_name"; ; // // v3.305.1 pilot nickname used in story mode messages
constexpr const static  char  *DEFAULT_SETUP_PILOT_NAME =  "Pilot"; ; // // v3.305.1 pilot nickname used in story mode messages
constexpr const static  char  *SETUP_USE_XP11_INV_LAYOUT =  "setup_use_xp11_inv_layout"; ; // // v24.12.2 Toggle option to use the Original inventory ui layout, instead of "station" layout.
constexpr const static  char  *SETUP_WRITE_CACHE_TO_DB =  "setup_write_cache_to_db"; ; // // v3.0.253.3 Write information to SQLITE DB too. // DEPRECATED ELEMENTS turned to constexpr
constexpr const static  char  *SETUP_FONT_PIXEL_SIZE =  "setup_font_pixel_size"; ; // // v3.0.251.1 holds preferred font pixel size
constexpr const static  char  *SETUP_FONT_LOCATION =  "setup_font_location"; ; // // v3.0.251.1 holds preferred font pixel size




const static  std::string OPT_PILOT_BASE_WEIGHT; // // the following optional attributes are read from General_Properties of the mission, they are not stored in option file. // v3.0.213.3 assist in calculating weight of plane ^ empty plane + pilot + passengers + lauggage
const static  std::string OPT_STORAGE_BASE_WEIGHT; // // v3.0.213.3 other payload
const static  std::string OPT_PASSENGERS_BASE_WEIGHT; // // v3.0.213.3 passengers
const static  std::string OPT_PILOT; // // v24.03.2 Adding deprecated attribute
const static  std::string OPT_STORAGE; // // v24.03.2 Adding deprecated attribute
const static  std::string OPT_PASSENGERS; // // v24.03.2 Adding deprecated attribute
const static  std::string MESSAGE_NEED_TO_RESTART_XPLANE; // // PLUGIN MESSAGES
const static  std::string ELEMENT_XP_COMMANDS; // //// COMMANDS related elements // v3.0.221.9
const static  std::string ATTRIB_COMMANDS; // // use as an attribute at goal level. example: <fire_commands_at_goal_begin commands^"bell/412/push,bell/412/" /> we should execute each command at its own flb
const static  std::string ELEMENT_FIRE_COMMANDS_AT_LEG_START; //
const static  std::string ELEMENT_FIRE_COMMANDS_AT_GOAL_START; //
const static  std::string ELEMENT_FIRE_COMMANDS_AT_LEG_END; //
const static  std::string ELEMENT_FIRE_COMMANDS_AT_GOAL_END; //
const static  std::string ATTRIB_FIRE_COMMANDS_ON_SUCCESS; // // can be used with triggers /// TBD not yet implemented
const static  std::string ATTRIB_FIRE_COMMANDS_ON_FAILURE; // // can be used with triggers
const static  std::string ATTRIB_FIRE_COMMANDS_AT_TASK_END; // // TODO: not sure if we want this /// END TBD not yet implemented
const static  std::string ATTRIB_START_COLD_AND_DARK; // // v3.0.221.10 briefers additional_location attribute
const static  std::string ELEMENT_DATAREFS_START_COLD_AND_DARK; // //  v3.0.221.10 will be stored at briefer level
const static  std::string MX_TRUE; // // used when reading XML attributes
const static  std::string MX_FALSE; // // used when reading XML attributes
const static  std::string MX_YES; // // used when reading XML attributes
const static  std::string MX_NO; // // used when reading XML attributes

const static  std::string BITMAP_INVENTORY_MXPAD; // // original textures //////////////////////////////  BITMAP BITMAP BITMAP  //////////////////////////////
const static  std::string BITMAP_MAP_MXPAD; //
const static  std::string BITMAP_AUTO_HIDE_EYE_FOCUS; // // used in MX-Pad to show if auto hide is active.
const static  std::string BITMAP_AUTO_HIDE_EYE_FOCUS_DISABLED; // // used in MX-Pad to show if auto hide is active.
const static  std::string BITMAP_LOAD_MISSION; // // new textures // used in welcome layer. // v24025 renamed file
const static  std::string BITMAP_HOME; // // IMGUI Related // v3.0.251.1 home like icon in blue
const static  std::string BITMAP_TARGET_MARKER_ICON; // // v3.0.253.9.1 icon for target marker show/hide
const static  std::string BITMAP_BTN_LAB_24X18; // // v3.0.251.1
const static  std::string BITMAP_BTN_WORLD_PATH_24X18; // // v3.0.251.1
const static  std::string BITMAP_BTN_PREPARE_MISSION_24X18; // // v3.0.251.1
const static  std::string BITMAP_BTN_FLY_MISSION_24X18; // // v3.0.251.1
const static  std::string BITMAP_BTN_SETUP_24X18; // // v3.0.251.1
const static  std::string BITMAP_BTN_CONVERT_FPLN_TO_MISSION_24X18; // // v3.0.301
const static  std::string BITMAP_BTN_TOOLBAR_SETUP_64x64; // // v3.0.251.1
const static  std::string BITMAP_BTN_QUIT_48x48; // // v3.0.251.1
const static  std::string BITMAP_BTN_SAVE_48x48; // // v3.0.251.1
const static  std::string BITMAP_BTN_ABOUT_64x64; // // v3.0.253.2
const static  std::string BITMAP_BTN_NAV_24x18; // // v3.0.253.6 // v24025 renamed
const static  std::string BITMAP_FMOD_LOGO; // // v3.0.253.6 fmod logo
const static  std::string BITMAP_BTN_WARN_SMALL_32x28; // // v3.0.255.3 warning sign
const static  std::string BITMAP_BTN_NAVINFO; // // v24025
const static  std::string BITMAP_BTN_SIMBRIEF_ICO; // // v25.03.3
const static  std::string BITMAP_BTN_SIMBRIEF_BIG; // // v25.03.3
const static  std::string BITMAP_BTN_FLIGHTPLANDB; // // v25.03.3

const static  std::string WHITE; //
const static  std::string RED; //
const static  std::string YELLOW; //
const static  std::string GREEN; //
const static  std::string ORANGE; //
const static  std::string PURPLE; //
const static  std::string BLUE; //
const static  std::string BLACK; //
const static  std::string MXPAD_SELECTION_DELIMITER; //

const static  std::string ATTRIB_USE_TRIGGER_NAME_FROM_TEMPLATE; // // RANDOM ENGINE: Generates Mission elements and attributes // v3.0.221.10 will search for element <trigger name^"" > in the template and will use it instead of the default map trigger template. This will allow for special triggers that are not just based on radius but
const static  std::string ATTRIB_ADD_TASKS_FROM_TEMPLATE; // // v3.0.221.10 will search for element <tasks name^"" > and will append all tasks in it to the current objective.
const static  std::string ATTRIB_ADD_MESSAGES_FROM_TEMPLATE; // // v3.0.221.10 will search for element with tag <{add_messages_from_template}> and will append all messages in it to xMessages
const static  std::string ATTRIB_ADD_TRIGGERS_FROM_TEMPLATE; // // v3.0.221.10 will search for element with tag <{add_triggers_from_template}> and will append all triggers elements to xTriggers
const static  std::string ATTRIB_ADD_SCRIPTS_FROM_TEMPLATE; // // v3.0.221.10 will search for element with tag <{add_scripts_from_template}> and will append all tasks in it to xScripts.
const static  std::string ATTRIB_LOCATION_TYPE; //
const static  std::string ATTRIB_LOCATION_VALUE; //
const static  std::string ATTRIB_EXPECTED_LAND_ON; //
const static  std::string ELEMENT_TEMPLATE; //
const static  std::string ATTRIB_TEMPLATE; //
const static  std::string ELEMENT_BRIEFER_AND_START_LOCATION; //
const static  std::string ELEMENT_EXPECTED_LOCATION; //
const static  std::string ATTRIB_PLANE_TYPE; // // v3.0219.6 used in template so we know which type of plane: [heli|ga|big] [helos|jets|turboprops|props|heavy]
const static  std::string PLANE_TYPE_HELOS; // // v3.0219.9 Helicopter
const static  std::string PLANE_TYPE_JETS; // // v3.0219.9 Jet planes
const static  std::string PLANE_TYPE_PROPS; // // v3.0219.9 Propellor
const static  std::string PLANE_TYPE_TURBOPROPS; // // v3.0219.9 Turbo Propeller, general heavier and larger types
const static  std::string PLANE_TYPE_HEAVY; // // v3.0219.9 The big daddies :-)
const static  std::string ELEMENT_CONTENT; // // v3.0.219.14 Used in random templates only
const static  std::string ATTRIB_LIST; // // v3.0.219.14 Used in random templates only
const static  std::string LOCATION_TYPE_SAME_AS; // // v3.0.219.1 "generate template" used in "goal template type" (implemented)
const static  std::string FL_TEMPLATE_VAL_START; // // v3.0.219.3 "generate template" used in "goal template type" (implemented)
const static  std::string FL_TEMPLATE_VAL_LAND; // // v3.0.219.1 "generate template" used in "goal template type" (implemented)
const static  std::string FL_TEMPLATE_VAL_HOVER; // // v3.0.219.3 "generate template" used in "goal template type" ()
const static  std::string FL_TEMPLATE_VAL_LAND_HOVER; // // v25.02.1 "generate template" - should be the default for "medevac" and "helos"
const static  std::string FL_TEMPLATE_VAL_DELIVER; // // v3.0.219.6 "generate template" used in "goal template type" ()
const static  std::string EXPECTED_LOCATION_TYPE_ICAO; // // v3.0.221.4
const static  std::string EXPECTED_LOCATION_TYPE_XY; // // xy, icao,goal,start  (xy can have string based random or number random that represents random lat/long
const static  std::string EXPECTED_LOCATION_TYPE_OILRIG; // // v3.303.14 oilrig ^ lat/long should be embedded in the <leg> element, no search is needed.
const static  std::string EXPECTED_LOCATION_TYPE_OSM; // // v3.0.241.10 b2 osm
const static  std::string EXPECTED_LOCATION_TYPE_WEBOSM; // // v3.0.253.4 webosm used with custom templates where a designer asks from plugin to fetch osm data from web (overpass in this implementation).
const static  std::string EXPECTED_LOCATION_TYPE_NEAR; // // "near" usually with land type, searches for nearest ICAO relative to last goal position
const static  std::string ELEMENT_ICAO; // // v3.0.221.5 used in RandomEngine when we try convert any icao tag to point
const static  std::string ATTRIB_ICAO_ID; // // v3.303.8.3
const static  std::string ATTRIB_AP_NAME; // // v3.303.14 airport name, used when creating OIL RIG mission
const static  std::string ATTRIB_NAVREF; // // v3.0.231.1 FMS data
const static  std::string ATTRIB_NAV_TYPE; // // v3.0.255.4
const static  std::string ATTRIB_DISP_FMS_ENTRY; // // v3.0.231.1 FMS data

const static  std::string GENERATE_TYPE_MEDEVAC; //
const static  std::string GENERATE_TYPE_DELIVERY; // // v3.0.219.6
const static  std::string GENERATE_TYPE_CARGO; // // v3.303.14
const static  std::string GENERATE_TYPE_OILRIG_MED; // // v3.303.14
const static  std::string GENERATE_TYPE_OILRIG_CARGO; // // v3.303.14
const static  std::string ELEMENT_CARGO; // // v24.05.1 used when reading external cargo xml data file
const static  std::string DEFAULT_RANDOM_IMAGE_FILE; // // v3.0.217.6
const static  std::string TERRAIN_TYPE_MEDEVAC_SLOPE; // // v3.0.219.12+ will be used when using <display_object_set> if the template name is "medevac" and slope is larger than plugin define max terrain slope then replace with "medevac_slope" object sets.
const static  std::string ATTRIB_RADIUS_MT; // // v3.0.217.6 used in point template
const static  std::string ATTRIB_LOC_DESC; // // v3.0.217.6 used in point template
const static  std::string ATTRIB_RANDOM_TAG; // // v3.0.219.1 the name of the element tag to search for random data. Can be random 3D Object or <point> etc.
const static  std::string ATTRIB_SET_NAME; // // v3.0.219.14 the name of a sub element that we need to random pick from.
const static  std::string ATTRIB_RANDOM_WATER_TAG; // // v3.0.219.10 the name of the element tag to search for random water 3D Objects
const static  std::string ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT; // // v3.0.219.1 position 3D Object providing: "bearing|distance"
const static  std::string ATTRIB_DEBUG_RELATIVE_POS; // // v3.303.14 used with RandomEngine. After calculating ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT we need to reset its value so it won't interfer with positioning of <display_object>. The "ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT" has two uses, calculate at the start of the mission or calculate relative to target/plane during Random mission creation.
const static  std::string ATTRIB_EXCLUDE_OBJ; // // v3.0.219.5 used in <point> hint Engine which objects to remove from generated mission if exists.
const static  std::string ATTRIB_INCLUDE_OBJ; // // v3.0.219.5 used in <point> hint Engine which objects to include in generated mission. Include can only pick one: "hiker,man,woman" will pick one and add it. overlaping location restriction.
const static  std::string PROP_IS_WET; // // v3.0.219.10 holds point data if water or terrain (land)
const static  std::string PROP_FOUND_ICAO; // // v3.0.219.12+ flags NavAidInfo as found in optimized aptData information
const static  std::string ATTRIB_LIMIT_TO_TERRAIN_SLOPE; // // v3.0.219.12+ used in <display_object> element where attribute "limit_to_terrain_slope" will assist if to display the object on certain terrain
const static  std::string ATTRIB_IS_SET_OF_FLIGHT_LEGS; // // v3.0.221.7 renamed to is_set_of_goals //v3.0.219.12+ used in <display_object> element where attribute "limit_to_terrain_slope" will assist if to display the object on certain terrain
const static  std::string ATTRIB_COPY_LEG_AS_IS_B; // // v3.0.303 used in combination of <content> element. when picking a set of legs we can flag them as final without the need to generate them from scratch, meaning we have all we need
const static  std::string ATTRIB_IS_RANDOM_COORDINATES; // // v3.0.219.12+ used in <display_object> element where attribute "limit_to_terrain_slope" will assist if to display the object on certain terrain
const static  std::string ATTRIB_POI_TAG; // // v3.0.219.14+ point of interest tag a <point> attribute. We will add to the GPS not as a target but as a point you can navigate too.
const static  std::string ATTRIB_BASE_ON_EXTERNAL_PLUGIN; // // v3.0.221.9 values: "true" or "false", flag that tells plugin to use external dataref to flag success of task. You still need to define a trigger since // we are not sure that there is any plugin listening and working with these datarefs
const static  std::string ATTRIB_HOVER_TIME_SEC_RANDOM; // // v3.0.241.8 former: ATTRIB_CUSTOM_HOVER_TIME // v3.0.221.11 random hover time in seconds. Can be in format "10" or "10-40"
const static  std::string ATTRIB_DEFAULT_RANDOM_HOVER_TIME; // // v3.0.241.8 default hover time
const static  std::string ATTRIB_CUSTOM_FLIGHT_LEG_DESC_FLAG; // // v3.0.221.15rc5 // v3.0.221.11 does goal has custom description
const static  std::string ATTRIB_FORCE_TEMPLATE_DISTANCES_B; // // v3.0.241.8 If the user picks expected distance in the setup screen, the designer can still decide if to allow this or not per <expected_location> element, not globally.
const static  std::string ATTRIB_IS_SKEWED_POSITION_B; // // v3.0.241.8 flag a point element as skewed position. Can be used in Random message handling when picking a point and it has this attribute.
const static  std::string ATTRIB_IS_BRIEFER_OR_START_LOCATION_B; // // v3.0.303.10
const static  std::string ATTRIB_SKEWED_NAME; // // v3.0.241.8 For templates markers. Replace <display_object> name with the "skewed_name"
const static  std::string ATTRIB_REAL_POSITION; // // v3.0.241.8
const static  std::string ATTRIB_IS_TARGET_POINT_B; // // v3.0.241.8 a flag on the <point> that flag it as a target point. It can still be skewed value.


const static  std::string ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B; // // v3.0.221.15rc5 renamed from pick_location_based_on_same_goal_template_b //  v3.0.221.15 rc3 an attribute that holds flag if random picked <point> should be same as the flight leg template type.
const static  std::string ATTRIB_MIN_VALID_FLIGHT_LEGS; // // v3.0.221.15 rc3.2 minimum number of goals so mission would be valid.
const static  std::string ATTRIB_SKIP_AUTO_TASK_CREATION_B; // // v3.0.221.15 rc4 skip auto task creation when building a goal. Designer probably will inject his/her tasks in goal_special_directives
const static  std::string ATTRIB_FORCE_SLOPED_TERRAIN; // // v3.0.253.6 // <expected_location> Only if leg template^hover, force slope so if getTarget() will pick location with shalower slope then we will fail it and ask for another target. // ATTRIB_FORCE_SLOPED_TERRAIN only for none slopped terrain. Provide values 0..10
const static  std::string ATTRIB_FORCE_LEVELED_TERRAIN; // // v3.0.253.9.1 // <expected_location> henry @Daikan asked for this. plugin will pick only "land" area and fail if won't find. Same as
const static  std::string ATTRIB_FORCE_TYPE_OF_TEMPLATE; // // v3.0.253.9.1 // <expected_location> henry @Daikan asked for this. plugin will pick only "land" area and fail if won't find. Same as
const static  std::string ATTRIB_DESIGNER_MAX_SLOPE_TO_LAND; // // v3.0.253.9.1 // <expected_location> henry @Daikan asked for this. Designer prefered max_slope_to_land value. default is mxconst::MAX_SLOPE_TO_LAND and should be stored in data_manager::MAX_SLOPE_TO_LAND.
const static  std::string PROP_NUMBER_OF_LOOPS_TO_FORCE_TYPE_TEMPLATE; // // v3.0.253.6 // internal, used in
const static  std::string PROP_IS_LAST_FLIGHT_LEG; // // v3.0.223.1 used during random mission generation
const static  std::string APT_1_LAND_AIRPORT_HEADER_CODE_v11_SPACE; // // v3.0.219.12+ apt dat codes with space. add trim if you don't want space
const static  std::string APT_16_SEAPLANE_BASE_HEADER_CODE_v11_SPACE; // // v3.0.219.12+ apt dat codes with space. add trim if you don't want space
const static  std::string APT_17_HELIPORT_HEADER_CODE_v11_SPACE; // // v3.0.219.12+ apt dat codes with space. add trim if you don't want space
const static  std::string APT_1300_RAMP_CODE_v11_SPACE; // // v3.0.219.12+ apt dat codes with space. add trim if you don't want space
const static  std::string APT_100_LAND_RW_CODE_v11_SPACE; // // v3.0.219.12+ apt dat codes with space. add trim if you don't want space
const static  std::string APT_101_WATER_RW_CODE_v11_SPACE; // // v3.0.219.12+ apt dat codes with space. add trim if you don't want space
const static  std::string APT_102_HELIPAD_RW_CODE_v11_SPACE; // // v3.0.219.12+ apt dat codes with space. add trim if you don't want space
const static  std::string APT_1_LAND_AIRPORT_HEADER_CODE_v11; // // v3.0.253.7 apt dat codes with space.
const static  std::string APT_16_SEAPLANE_BASE_HEADER_CODE_v11; // // v3.0.253.7 apt dat codes with space.
const static  std::string APT_17_HELIPORT_HEADER_CODE_v11; // // v3.0.253.7 apt dat codes with space.
const static  std::string APT_1300_RAMP_CODE_v11; // // v3.0.253.7 apt dat codes with space.
const static  std::string APT_100_LAND_RW_CODE_v11; // // v3.0.253.7 apt dat codes with space.
const static  std::string APT_101_WATER_RW_CODE_v11; // // v3.0.253.7 apt dat codes with space.
const static  std::string APT_102_HELIPAD_RW_CODE_v11; // // v3.0.253.7 apt dat codes with space.
const static  std::string ATTRIB_DISTANCE_NM; // // v3.0.219.11 hold distance from last goal to current
const static  std::string ATTRIB_SHARED_TEMPLATE_TYPE; // // v3.0.221.7 used with shared type datarefs
const static  std::string ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE; // // v3.0.221.15rc5 support leg/ Replaces the name shared_goal_template  // v3.0.221.7 used with shared type datarefs
const static  std::string ATTRIB_SHARED_GOAL_TEMPLATE; // // v3.0.221.7 used with shared type datarefs
const static  std::string ATTRIB_TARGET_POS; // // helpers attribute for random creation  // v3.0.221.9 target position - format: "{lat}|{lon}|{elev}. Internal use. holds leg main target location. Useful to share lat/lon with shared datarefs. Also we should use it when we determind not random mission, it should be the first task in a goal that is mandatory depends on trigger with location values.
const static  std::string ATTRIB_TASK_TRIGGER_NAME; // // v3.0.219.2 store the name of the trigger the goals task is dependent upon.
const static  std::string ATTRIB_HOVER_TIME; // // v3.0.219.3 store hover time, which is also task success evaluation time
const static  std::string ATTRIB_DEPENDS_ON; // // v3.0.219.11 depends on other goal. Used in random mission generator template.
const static  std::string ATTRIB_TERRAIN_SLOPE; // // v3.0.221.3 holds terrain slope from calculate_slope_for_build_flight_leg_thread ( mission.flcPRE() )
const static  int         DEFAULT_HOVER_HEIGHT_FT; //

const static  std::string DEFAULT_TEMPLATE_EXPECTED_LOCATION_LAND_ON; //
const static  std::string DEFAULT_TEMPLATE_DELIVERY_EXPECTED_LOCATION_TYPE_VALUE; // //"icao"; ^// v3.0.219.6 for delivery type template
const static  std::string EXPECTED_LOCATION_VALUE_DEFAULT_DISTANCE; // // 30nm for XY based option
const static  std::string DEFAULT_INVENTORY_RADIUS_MT; // // v3.0.219.7 used in RandomEngine.injectInventory() function
const static  std::string RXP_FPEI_FLIGHT_PLAN_SEGMENT; // // Reality XP related // v3.0.242.2 RealityXP Flight Plan Segment. Flight Plan Element Identifier (FPEI)
const static  std::string EXTERNAL_FPLN_FOLDERS_FILE_NAME; // // v3.0.242.2 Holds the file name in "missionx" plugin that points to all the external utilities FPLN/data folders. Example: RXP
const static  std::string EXTERNAL_FPLN_TARGET_FILE_NAME; // // v3.0.242.2 Holds the file name missionx will create. Example: RXP

const static  std::map<std::string, int> MAP_IGNORE_PROPS_DURING_IO; //

const static  std::string SETUP_DISPLAY_TARGET_MARKERS; // // v3.0.241.7 will assist in show/hide instanced objects that are flaged with attribute: "target_marker_b" attribute
const static  std::string SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET; // // v3.0.241.7 Random medevac missions will display the target marker ^3nm from target.
const static  std::string OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE; // // v3.0.241.7 Random medevac missions will override target distance. This is a hint and not for all targets type. "allow_override_b" atttribute should be present.
const static  std::string SETUP_SLIDER_RANDOM_TARGET_MIN_DISTANCE; // // v3.0.241.7 holds slider value of minimal expected distance
const static  std::string SETUP_SLIDER_FONT_SCALE_SIZE; // // v3.0.251.1 holds preferred font size by user
const static  std::string SETUP_LOCK_OVERPASS_URL_TO_USER_PICK; // // v3.0.253.4.2 used with overpass combo
const static  std::string NO_SETUP_TEXT; // // v3.0.253.4.2 used with overpass combo
const static  std::string SETUP_NORMALIZED_VOLUME; // // v3.0.303.6 Used with volume settings. Values between 1..100
const static  std::string SETUP_SIMBRIEF_PILOT_ID; // // Simbrief // v25.03.3 Stores Simbrief Pilot ID
const static  std::string SETUP_NORMALIZE_VOLUME_B; // // v3.0.303.6 If the failure is true: then use the "setup_mission_volume" to override higher mixture values. Example, if the <mix> tag defines a volume of 0.5 and the setup volume is set to 40 (^0.4) then it will force the lower value
const static  std::string SETUP_AUTHORIZATIOJN_KEY; // // v3.303.8.3 authorization key from flightplandatabase.com




#ifdef LIN
const static  std::string SETUP_LINUX_FLAVOR_CODE_I; // // v3.303.8.1 FMOD seems to crash on System::release() in arch based Linux distro. The workaround is to not release it at all. It is not the best approach but it is a tested workaround none the less.
#endif

const static  std::string PROP_FPLN_ID_PICKED; // // properties name to user UI // v3.0.253.1 Used with "fetch external flight plan" layer, and holds the FPLN ID picked for Random Engine + missionx::data_manager::tableExternalFPLN_vec
const static  std::string PROP_MED_CARGO_OR_OILRIG; // // v3.0.241.9 UI info from user defined mission screen
const static  std::string PROP_MISSION_SUBCATEGORY; // // v3.303.14 sub category property. Used with user creation screen, stored which subcategory the user picked. Example: OilRig, medevac
const static  std::string PROP_MISSION_SUBCATEGORY_LBL; // // v24.05.1
const static  std::string PROP_STARTING_DAY; // // v3.0.303.8 UI info from user defined mission screen
const static  std::string PROP_STARTING_HOUR; // // v3.0.303.8 UI info from user defined mission screen
const static  std::string PROP_STARTING_MINUTE; // // v3.0.303.8 UI info from user defined mission screen
const static  std::string PROP_PLANE_TYPE_I; // // v3.0.241.9 UI info from user defined mission screen
const static  std::string PROP_PLANE_TYPE_S; // // v3.0.241.9 UI info from user defined mission screen
const static  std::string PROP_NO_OF_LEGS; // // v3.0.241.9 UI info from user defined mission screen
const static  std::string PROP_MIN_DISTANCE_SLIDER; // // v3.0.241.9 UI info from user defined mission screen
const static  std::string PROP_MAX_DISTANCE_SLIDER; // // v3.0.241.9 UI info from user defined mission screen
const static  std::string PROP_USE_OSM_CHECKBOX; // // Use OSM databases checkbox.
const static  std::string PROP_USE_WEB_OSM_CHECKBOX; // // Use WEB OSM databases checkbox.
const static  std::string PROP_NARROW_HELOS_RAMP_SEARCH; // // v3.0.253.7
const static  std::string PROP_ADD_COUNTDOWN; // // v3.0.253.9.1 add countdown to most flight legs except the last one
const static  std::string PROP_START_FROM_PLANE_POSITION; // // v3.0.253.11 User prefers not to move the plane when it searches for a starting location. Good for cold and dark cases since after moving the plane using the API, the plane is out of cold and dark state
const static  std::string PROP_GENERATE_GPS_WAYPOINTS; // // v3.0.253.12 Should Random Engine generate GPS waypoint or not. Will be used in "User creation", "ILS" and "external FPLN" screens.
const static  std::string PROP_FILTER_AIRPORTS_BY_RUNWAY_TYPE; // // v3.0.255.2 Defined in "create user screen" and used in RandomEngine when needs to filter our airports that has only excluded runways types
const static  std::string PROP_REMOVE_DUPLICATE_ICAO_ROWS; // // v3.303.8.3 During data_manager::fetch_fpln_from_external_site() remove duplicate ICAO results. Used in FPLN from External site screen.
const static  std::string PROP_GROUP_DUPLICATES_BY_WAYPOINTS; // // v3.303.8.3 During data_manager::fetch_fpln_from_external_site() remove duplicate ICAO & Waypoint numbers. Used in FPLN from External site screen.
const static  std::string PROP_WEATHER_USER_PICKED; // // v3.303.12
const static  std::string PROP_WEATHER_CHANGE_MODE_USER_PICKED; // // v3.303.13  // TODO CONSIDER REPRECATING
const static  std::string VALUE_STORE_CURRENT_WEATHER_DATAREFS; // // v3.303.13
const static  std::string PROP_ADD_DEFAULT_WEIGHTS_TO_PLANE; // // v3.303.14
const static  std::string PROP_FROM_ICAO; // // used with external fpln
const static  std::string PROP_TO_ICAO; // // v3.0.253.3 used with external fpln // add the option to define destination location
const static  std::string PROP_LIMIT; // // used with external fpln
const static  std::string PROP_SORT_FPLN_BY; // // used with external fpln
const static  std::string RANDOM_TEMPLATE_BLANK_4_UI; // // template_blank_4_ui is the only template that is driven by UI so we should not load them.
const static  std::string COORDINATES_IN_THE_GPS_S; //

const static  std::string ELEMENT_UI_PROPERTIES; // //// SQLite DB
const static  std::string DB_AIRPORTS_XP; //
const static  std::string DB_AIRPORTS_XP2; //
const static  std::string DB_STATS_XP; // // v3.0.255.1 holds running mission stats
const static  std::string DB_FOLDER_NAME; // // sub folder in "missionx" plugin folder.
const static  std::string DB_FILE_EXTENSION; // // sub folder in "missionx" plugin folder.
const static  std::string STATS_LANDING; // // "L"; ^// "Landing" // v3.0.255.1
const static  std::string STATS_TAKEOFF; // //"T"; ^// "takeoff"  // v3.0.255.1
const static  std::string STATS_TRANSITION; // //"T"; ^// "takeoff"  // v3.0.255.1
const static  std::string DEFAULT_OVERPASS_URL; // // v3.0.253.6 overpass default overpass URL
const static  std::string ATTRIB_OSM_KEY; // // v3.0.253.4 <way k^"" v^"">
const static  std::string ATTRIB_OSM_VALUE; // // v3.0.253.4 <way k^"" v^"">
const static  std::string ELEMENT_NODE_OSM; // // v3.0.253.4 <node id^"" lat^""...">
const static  std::string ELEMENT_CENTER; // // v3.0.253.9 <center ><way> sub-element. Will be present if designer added: ";out center;" to the filter text
const static  std::string ELEMENT_WAY_OSM; // // v3.0.253.4 <way id^"">
const static  std::string ELEMENT_ND_OSM; // // v3.0.253.4 <way id^"">
const static  std::string ELEMENT_REL_OSM; // // v3.0.253.9 <rel >
const static  std::string ATTRIB_REF_OSM; // // v3.0.253.4 <way id^"">


static const  std::string ELEMENT_TIMER; // // v3.0.253.7 used in flight leg and global settings. We should only have 1 in each root element.
static const  std::string ATTRIB_FAIL_MSG; // // v3.0.253.7 used in flight leg and global settings. <timer... fail_msg>
static const  std::string ATTRIB_SUCCESS_MSG; // // v3.0.253.7 used in flight leg and global settings. <timer... success_msg>
static const  std::string ATTRIB_RUN_UNTIL_LEG; // // v3.0.253.7 used in <timer... success_msg>
static const  std::string ATTRIB_FAIL_ON_TIMEOUT_B; // // v3.305.3 used in <timer>, we also support "post_script"
static const  std::string ATTRIB_STOP_ON_LEG_END_B; // // v3.305.3 used in <timer>
static const  std::string ELEMENT_SCORING; // // v3.303.9.1 renamed "rank" element to "scoring"
static const  std::string ELEMENT_PITCH; // // v3.303.8.3 scoring sub element.
static const  std::string ELEMENT_ROLL; // // v3.303.8.3 scoring sub element.
static const  std::string ELEMENT_GFORCE; // // v3.303.8.3 scoring sub element.
static const  std::string ELEMENT_CENTER_LINE; // // v3.303.8.3 scoring sub element.
static const  std::string ATTRIB_ZULU_TIME_SEC; // // attribs from fragment time // v3.0.253.7 zuluTime_sec used in <timer... > save checkpoint
static const  std::string ATTRIB_DAY_IN_YEAR; // // v3.0.253.7 day_in_year used in <timer... > save checkpoint
static const  std::string ATTRIB_TOTAL_RUNNING_TIME_SEC_SINCE_SIM_START; // // v3.0.253.7 total_running_time_sec_since_sim_start used in <timer... > save checkpoint
static const  std::string ATTRIB_RUN_CONTINUOUSLY; // // attribs from timer // v3.0.253.7 used in <timer... > save checkpoint
static const  std::string ATTRIB_SECONDS_PASSED; // // v3.0.253.7 used in <timer... > save checkpoint
static const  std::string ATTRIB_BEGIN_TIME; // // v3.0.253.7 used in <timer... > save checkpoint
static const  std::string ATTRIB_TIMER_STATE; // // v3.0.253.7 used in <timer... > save checkpoint
static const  std::string ATTRIB_FIND; // // XML replace options // v3.0.255.4 // v3.0.255.4
static const  std::string ATTRIB_REPLACE_WITH; // // v3.0.255.4
static const  std::string TEMPLATE_INJECTED_FILE_NAME; // // v3.0.255.4 the name of the final template that holds all the injected data into it

const static  std::string ELEMENT_LNM_LittleNavmap; // // v3.0.301 <LittleNavmap> ROOT
const static  std::string ELEMENT_LNM_Flightplan; // // v3.0.301 <Flightplan>
const static  std::string ELEMENT_LNM_Waypoints; // // v3.0.301 <Waypoints>
const static  std::string ELEMENT_LNM_Waypoint; // // v3.0.301 <Waypoint>
const static  std::string ELEMENT_LNM_Departure; // // v3.0.301 <Departure>
const static  std::string ELEMENT_LNM_Pos; // // v3.0.301 <Pos>
const static  std::string ELEMENT_LNM_Heading; // // v3.0.301 <Heading>
const static  std::string ELEMENT_LNM_Name; // // v3.0.301 <Name>
const static  std::string ELEMENT_LNM_Ident; // // v3.0.301 <Ident>
const static  std::string ELEMENT_LNM_Region; // // v3.0.301 <Region>
const static  std::string ELEMENT_LNM_Type; // // v3.0.301 <Type>
const static  std::string ATTRIB_LNM_Lat; // // v3.0.301
const static  std::string ATTRIB_LNM_Lon; // // v3.0.301
const static  std::string ATTRIB_LNM_Alt; // // v3.0.301
static const  std::string DREF_TARGET_POS_LAT_D; // // Sling Load // The new datarefs to monitor cargo position
static const  std::string DREF_TARGET_POS_LON_D; //
static const  std::string DREF_TARGET_POS_ELEV_M_D; //
static const  std::string DREF_EXTERNAL_HSL_CARGO_SET_LATITUDE; // // These datarefs are dependent on the "Sling Load" plugin. If one of them is changed, then we must do the same to the string values and then recompile Mission-X plugin. No need to change the const string parameter name
static const  std::string DREF_EXTERNAL_HSL_CARGO_SET_LONGITUDE; //
static const  std::string DREF_EXTERNAL_HSL_CARGO_MASS; //
static const  std::string DREF_EXTERNAL_HSL_CARGO_FOLLOW_ONLY; // // integer 1 if the cargo is neither connected nor placed on terrain, 0 otherwise.
static const  std::string DREF_EXTERNAL_HSL_CARGO_CONNECTED; // // 1 if the cargo is connected to the rope, 0 otherwise.
static const  std::string CMD_EXTERNAL_HSL_ENABLE_SLING_LOAD; // // Enable Sling Load
static const  std::string CMD_EXTERNAL_HSL_DISABLE_SLING_LOAD; // // Disable Sling Load
static const  std::string CMD_EXTERNAL_HSL_CARGO_LOAD_ON_COORDINATES; // // Place the load at given coordinates
static const  std::string CMD_EXTERNAL_HSL_UPDATE_OBJECTS_SLING_LOAD; // // Update HSL 3D Objects
static const  std::string ELEMENT_FPLN; // // v3.0.303.4 // Storing conversion data
static const  std::string ELEMENT_BUFFERS; // // v3.0.303.4
static const  std::string ELEMENT_BUFF; // // v3.0.303.4
const static  std::string ATTRIB_INDEX; // // v3.0.303.4
const static  std::string CONV_ATTRIB_isLeg; // // v3.0.303.4
const static  std::string CONV_ATTRIB_isLast; // // v3.0.303.4
const static  std::string CONV_ATTRIB_ignore_leg; // // v3.0.303.4
const static  std::string CONV_ATTRIB_convertToBriefer; // // v3.0.303.4
const static  std::string CONV_ATTRIB_add_marker; // // v3.0.303.4
const static  std::string CONV_ATTRIB_markerType; // // v3.0.303.4
const static  std::string CONV_ATTRIB_iCurrentBuf; // // v3.0.303.4
const static  std::string CONV_ATTRIB_radius_to_display_marker; // // v3.0.303.4
const static  std::string CONV_ATTRIB_on_ground; // // v3.0.303.4
const static  std::string CONV_ATTRIB_elev_combo_picked_i; // // v3.0.303.4
const static  std::string CONV_ATTRIB_slider_elev_value_i; // // v3.0.303.4 inLegData.target_trig_strct.slider_elev_value_i
const static  std::string CONV_ATTRIB_elev_rule_s; // // v3.0.303.4
const static  std::string CONV_ATTRIB_trig_ui_type_combo_indx; // // Triggers related // v3.0.303.4 radius, script, box
const static  std::string CONV_ATTRIB_trig_ui_plane_pos_combo_indx; // // v3.0.303.4 on ground, ignore, airborne
const static  std::string CONV_ATTRIB_trig_ui_elev_type_combo_indx; // // v3.0.303.4 above than, min/max etc...

const static  std::string ELEMENT_QUERY; // // SQLite sql.xml elements name
const static  std::string SQLITE_OILRIG_SQLS; //
const static  std::string SQLITE_OSM_SQLS; //
const static  std::string SQLITE_NAVDATA_SQLS; //
const static  std::string SQLITE_ILS_SQLS; //
const static  std::string STORY_IMAGE_POS; // // v3.305.1 story mode constants// image position. ATTRIB_NAME
const static  std::string STORY_CHARACTER_CODE; // // Story Character Code to find in Message->mapCharacters
const static  std::string STORY_TEXT; // // Story Text
const static  std::string STORY_PAUSE_TIME; // // How much time to Pause before skipping
const static  std::string ATTRIB_IGNORE_PUNCTUATIONS_B; // // v3.305.1 Used in story mode. Do you want the plugin to use same timer for all characters
const static  std::string STORY_DEFAULT_TITLE_CODE; // // If no character found then use N/A
const static  std::string STORY_DEFAULT_TITLE_NA; // // If no character found then use N/A

static const  std::vector<const char *>  vecStoryActions; // // s^speak title, p^pause, i^image, c^choice, m^text line + display in mxpad, t^text line
static const  std::vector<const char *>  vecStoryPunctuation; // // > ignore punctuation, < force punctuation


static constexpr const  char   STORY_ACTION_TEXT =  't'; ; // // action t - display text
static constexpr const  char   STORY_ACTION_IMG =  'i'; ; // // action i - load image
static constexpr const  char   STORY_ACTION_PAUSE =  'p'; ; // // action p - pause
static constexpr const  char   STORY_ACTION_HIDE =  'h'; ; // // action h - Hides main window, only as last line
static constexpr const  char   STORY_ACTION_MSGPAD =  'm'; ; // // action m - show text in message pad and as a text ?
static constexpr const  float  STORY_DEFAULT_TIME_BETWEEN_CHARS_SEC_F =  0.05f; //
static constexpr const  float  STORY_DEFAULT_TIME_AFTER_PERIOD_SEC_F =  1.25f; //
static constexpr const  float  STORY_DEFAULT_TIME_AFTER_COMMA_SEC_F =  0.60f; //
static constexpr const  float  STORY_DEFAULT_PUNCTUATION_EXPONENT_F =  0.5f; //
static constexpr const  float  DEFAULT_SKIP_MESSAGE_TIMER_IN_SEC_F =  3.0f; ; // // v3.305.3 moved from uiImGuiBriefer->strct_flight_leg_info.strct_story_mode
static constexpr const  float  DEFAULT_SF_FADE_SECONDS_F =  6.0; ; // // v3.306.1b
static constexpr const  char   *PROP_SECONDS_TO_RUN_I =  "seconds_to_run_i"; ; // // v3.305.3 interpolation properties
static constexpr const  char   *PROP_FOR_HOW_MANY_CYCLES_I =  "for_how_many_cycles_i"; //
static constexpr const  char   *PROP_CURRENTCYCLECOUNTER_I =  "currentCycleCounter_i"; //
static constexpr const  char   *PROP_TARGET_VALUE_D =  "target_value_d"; //
static constexpr const  char   *PROP_DELTA_SCALAR =  "delta_scalar"; //
static constexpr const  char   *PROP_DELTA_ARRAY_F =  "delta_array_f"; //
static constexpr const  char   *PROP_LAST_VALUE_ARRAY_D =  "last_value_array_d"; //
static constexpr const  char   *PROP_STARTING_SCALAR_VALUE_D =  "starting_scalar_value_d"; //
static constexpr const  char   *PROP_LAST_VALUE_D =  "last_value_d"; //
static constexpr const  char   *PROP_SECONDS_PASSED_F =  "seconds_passed_f"; //
static constexpr const  char   *PROP_TARGET_VALUES_S =  "target_values_s"; //
static constexpr const  char   *PROP_START_VALUES_S =  "start_values_s"; //
static constexpr const  char   *PROP_DELTA_VALUES_S =  "delta_values_s"; //
static constexpr const  char   *REPLACE_KEYWORD_PILOT_NAME =  "%pilot%"; ; // // v3.305.1 replace string used when reading a story mode message line and we want to replace it
static constexpr const  char   *REPLACE_KEYWORD_SELF =  "%self%"; ; // // v3.305.3 can be used with scripts as a parameter sending of a dynamic trigger that we don't know its name, like: "trigger name" from dynamic message.

static const  std::string ELEMENT_NOTES; // // v24.03.1
static const  std::string ELEMENT_STATION; // // v24.12.2 Used with acf cargo information. sim/flightmodel/weight/m_stations []
static const  std::string ATTRIB_TARGET_INVENTORY; // // v24.12.2 target inventory
static const  std::string ATTRIB_FALLTHROUGH_B; // // v3.305.1b used in Messages. If message is last message and we want quick transition to next leg, we should set this attribute to true.

// Simbrief
static const  std::string ELEMENT_GENERAL; // // v25.03.3 Simbrief <general>
static const  std::string ELEMENT_ROUTE; // // v25.03.3 Simbrief
static const  std::string ELEMENT_ROUTE_IFPS; // // v25.03.3 Simbrief
static const  std::string ELEMENT_ROUTE_NAVIGRAPH; // // v25.03.3 Simbrief
static const  std::string ELEMENT_FLIGHT_NUMBER; // // v25.03.3 Simbrief
static const  std::string ELEMENT_ORIGIN; // // v25.03.3 Simbrief
static const  std::string ELEMENT_ICAO_CODE; // // v25.03.3 Simbrief
static const  std::string ELEMENT_DESTINATION; // // v25.03.3 Simbrief
static const  std::string ELEMENT_PLAN_RWY; // // v25.03.3 Simbrief
static const  std::string ELEMENT_TRANS_ALT; // // v25.03.3 Simbrief
static const  std::string ATTRIB_FORMATED_NAV_POINTS; // // v25.03.3 mission creation using FPLN

// OSM
const static  std::string ATTRIB_WEBOSM_OPTIMIZE; // // v3.0.253.4     <expected_location location_type~"webosm,near,xy" // location_value~"nm~25|tag~webosm_helipads|webosm_optimize~n|ramp~H|dbfile~osm-liechtenstein.db,nm~20,tag~start_location" force_template_distances_b~"yes" />
const static  std::string DEFAULT_OVERPASS_WAYS_FILTER; // // v3.0.253.9 added way and ";" since we changed the way designer provide filters. We do not fix their code. // v3.0.253.4 overpass ways filter

private:
  static constexpr  float  meter2feet =  3.28083f; //

	};// end class
} // missionx namespace

#endif //MXCONST_H
