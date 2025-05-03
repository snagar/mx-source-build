#ifndef XX_MX_CONSTANTS_HPP
#define XX_MX_CONSTANTS_HPP
#pragma once

#include <cfloat>
#include <iostream>
#include <sstream>
#include <string>
#include <deque>
#include <list>
#include <map>
#include <vector>
#include <chrono>

#ifndef IBM
#include <cfloat>
#include <limits>
#endif

#include "mxconst.h" // v25.04.2

#include <fmt/format.h>


namespace missionx
{

//#define TIMER_FUNC // v3.305.2 add timing for functions to understand were it consumes more time.
//#define USE_TRIGGER_OPTIMIZATION // v3.305.2

#define ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL 0

constexpr static const int MX_FEATURES_VERSION = 20241212; //20230917; // 330491; //302564;  // added SETUP_LOCK_OVERPASS_URL_TO_USER_PICK


inline constexpr static auto PLUGIN_VER_MAJOR  = "25"; // year
inline constexpr static auto PLUGIN_VER_MINOR  = "04"; // month
inline constexpr static auto PLUGIN_REVISION_S =  "2";
inline constexpr static auto PLUGIN_REVISION = PLUGIN_REVISION_S;

#ifdef RELEASE
constexpr static auto PLUGIN_DEV_BUILD = "";
#else
constexpr static auto PLUGIN_DEV_BUILD = "D"; // The char "D", won't interfere with plugin version number when we write it to the preference file.
#endif

constexpr static int XP12_VERSION_NO = 120000;
constexpr static int XP11_COMPATIBILITY = 11;
constexpr static int XP12_COMPATIBILITY = 12;

// const static auto PLUGIN_VERSION_S = fmt::format("{} {} {}", PLUGIN_VER_MAJOR, PLUGIN_VER_MINOR, PLUGIN_REVISION_S).c_str ();

#ifndef RELEASE
  const static auto FULL_VERSION = std::string(PLUGIN_VER_MAJOR).append(".").append(PLUGIN_VER_MINOR).append(".").append(PLUGIN_REVISION_S).append( PLUGIN_DEV_BUILD).append(", ").append( __DATE__ ).append(" ").append(__TIME__);
  // static auto FULL_VERSION = std::string("{}.{}.{} {} {} {}", PLUGIN_VER_MAJOR, PLUGIN_VER_MINOR, PLUGIN_REVISION_S, PLUGIN_DEV_BUILD, __DATE__, __TIME__).c_str ();
#else
  const static auto FULL_VERSION = std::string(PLUGIN_VER_MAJOR).append(".").append(PLUGIN_VER_MINOR).append(".").append(PLUGIN_REVISION_S);
  // const static auto FULL_VERSION = fmt::format ("{}.{}.{} {}", PLUGIN_VER_MAJOR, PLUGIN_VER_MINOR, PLUGIN_DEV_BUILD, PLUGIN_REVISION_S).c_str ();
#endif


inline constexpr static auto APP_NAME = "Mission-X Plugin";

inline constexpr static auto PLUGIN_DIR_NAME     = "missionx";
inline constexpr static auto PLUGIN_FILE_VER     = "312"; // v24.12.2
inline constexpr static auto PLUGIN_FILE_VER_XP11= "301";
inline constexpr static auto RANDOM_TEMPLATE_VER = "301";

const static std::list<int> lsSupportedMissionFileVersions = { 301,312 }; // v24.12.2 used when reading missions files.

inline constexpr static auto TEXTURE_DIR_NAME            = "bitmap";
inline constexpr static auto XPLANE_SCENERY_INI_FILENAME = "scenery_packs.ini";
inline constexpr static auto SCENERY_PACK_               = "SCENERY_PACK ";
inline constexpr static auto CARGO_DATA_FILE             = "cargo_data.xml";

inline constexpr static int MAX_COORDS (360 * 180);

// GLOBAL CONSTANTS
inline static constexpr double PI  = 3.1415926535897932384626433832795; // from windows calculator
inline static constexpr double PI2 = 6.283185307179586476925286766559;  // FOR DROPPING calculation

// static double Gravity=-9.81;

inline static constexpr float EARTH_RADIUS_M                  = 6378145.0f;
inline static constexpr double LOWEST_GROUND_ELEV_FOR_TRIGGERS = -3000.0; // v3.0.253.5 used in cases we need to configure triggers with "--" or "---" signs


inline static constexpr float RadToDeg             = (float)(180.0f / PI); // 57.295779513082320876798154814105
inline static constexpr float DegToRad             = 1.0f / RadToDeg;      // 0.01745329251994329576923690768489
inline static constexpr float feet2meter           = 0.3048f;              // 1.0f / 3.28083f;
inline static constexpr float meter2feet           = 3.28083f;
inline static constexpr float meter2nm             = 0.000539957f;
inline static constexpr float nm2meter             = 1852.0f;
inline static constexpr float nm2km                = 1.852f;
inline static constexpr float km2nm                = 0.539957f;
inline static constexpr float OneNmh2meterInSecond = 0.5144444f; //

inline static constexpr float seconds2minutes_f = 0.0166f; // v3.305.3

inline static constexpr float fpm2ms  = 0.005f; // fpm * feetPerMin2MeterSec = meters per seconds

inline static constexpr float kmh2fts = 0.911344f; // v3.0.202
inline static constexpr float fts2kmh = 1.09728f;  // v3.0.202



inline static constexpr int OUT_OF_BOUNDING_ALERT_TIMER_SEC = 30; // 30 sec. alert will broadcast every 30 seconds
inline static constexpr float MISSIONX_DOUBLE_CLICK           = 0.9f;

inline const static std::string EMPTY_STRING = "";
inline const static std::string PREF_FILE_NAME = "missionx_pref.xml";

inline static constexpr int BRIEFER_DESC_CHAR_WIDTH = 70;

// Global Char Buffer for Login
inline static constexpr int LOG_BUFF_SIZE = 2048;
[[maybe_unused]]
inline static char      LOG_BUFF[LOG_BUFF_SIZE];


// 21638.7 Nautical Miles; 40075.0 km  24901.5 miles
inline static constexpr float EQUATER_LEN_NM      = 21638.7f;
inline static constexpr float EARTH_AVG_RADIUS_NM = 3440.07019148103f;


inline static constexpr float    DEGREESE_IN_CIRCLE    = 360.0f;
inline static constexpr intptr_t DAYS_IN_YEAR_365      = 365;
inline static constexpr intptr_t SECONDS_IN_1HOUR_3600 = 3600;
inline static constexpr intptr_t SECONDS_IN_1MINUTE    = 60;
inline static constexpr float    SECONDS_IN_1MINUTE_F  = 60.0f;
inline static constexpr intptr_t SECONDS_IN_1DAY       = 86400;
inline static constexpr intptr_t HOURS_IN_A_DAY_24     = 24;

inline static constexpr double NEARLY_ZERO = 0.0000000001; // dataref_manager

#ifdef DEBUG_WRONG_TERRAIN_ELEV
inline const static unsigned int NUM_CIRCLE_POINTS = 25; // v3.0.253.1
#else
inline static constexpr unsigned int NUM_CIRCLE_POINTS        = 25; // v3.0.253.7 decreased from 359; // v3.0.202a
inline static constexpr unsigned int NUM_CIRCLE_POINTS_3D_OBJ = 8;  // v3.0.253.7 decreased from 359; // v3.0.202a

inline static constexpr size_t MAX_MX_PAD_MESSAGES = 20; // v3.0.110

#endif
////// ENUMS & STRUCTS //////
namespace enums
{
// // v24.12.2
// typedef enum class _from_where_mission_was_loaded
//   : uint8_t
// {
//   loaded_from_mission_file = 0,
//   loaded_from_savepoint = 1,
//   loaded_from_random = 2
// } mx_from_where_mission_was_loaded;


typedef enum class _mx_timer_type
  : uint8_t
{
  xp = 0,
  os = 1
} mx_timer_type;

// v24.03.1
typedef enum class _mx_treeNodeState
  : uint8_t
{
  closed = 0,
  opened = 1,
  was_opened = 2 // distinguish when we at least dealt with the "open" state. Used for one time scroll logic.
} mx_treeNodeState;

// v24.03.1
typedef enum class _mxNoteField
  : uint8_t
{
  fromICAO = 0,
  fromParkLoc,
  fromRunway,
  fromSID,
  fromTrans,
  toTrans,
  toSTAR,
  toRunway,
  toParkLoc,
  toICAO,
  _COUNT, // ILLEGAL
  begin = 0,
  end = _COUNT
} mx_note_shortField_enum;

typedef enum class _mxNoteLargeFields
  : uint8_t
{
  waypoints = 0,
  taxi,
  takeoff_notes,
  cruise_notes,
  descent_notes,
  _COUNT, // ILLEGAL
  begin  = 0,
  end    = _COUNT
} mx_note_longField_enum;

typedef enum class _mx_action_from_trigger_enum
  : uint8_t
{
  set_success = 0,
  reset = 1
} mx_action_from_trigger_enum;

typedef enum class _mx_acf_line_type_enum
  : uint8_t
{
  max_weight_line   = 0,
  station_name_line = 1,
} mx_acf_line_type_enum;


} // namespace enums

namespace enums_translation
{
static const std::map <missionx::enums::mx_note_shortField_enum, const char*> trnsEnumNoteShort =
  {
    {missionx::enums::mx_note_shortField_enum::fromICAO, "fromICAO"},
    {missionx::enums::mx_note_shortField_enum::fromParkLoc, "fromParkLoc"},
    {missionx::enums::mx_note_shortField_enum::fromRunway, "fromRunway"},
    {missionx::enums::mx_note_shortField_enum::fromSID, "fromSID"},
    {missionx::enums::mx_note_shortField_enum::fromTrans, "fromTrans"},
    {missionx::enums::mx_note_shortField_enum::toTrans, "toTrans"},
    {missionx::enums::mx_note_shortField_enum::toSTAR, "toSTAR"},
    {missionx::enums::mx_note_shortField_enum::toRunway, "toRunway"},
    {missionx::enums::mx_note_shortField_enum::toParkLoc, "toParkLoc"},
    {missionx::enums::mx_note_shortField_enum::toICAO, "toICAO"}
  };
static const std::map <missionx::enums::mx_note_longField_enum, const char*> trnsEnumNoteLong =
  {
    {missionx::enums::mx_note_longField_enum::waypoints, "waypoints"},
    {missionx::enums::mx_note_longField_enum::taxi, "taxi"},
    {missionx::enums::mx_note_longField_enum::takeoff_notes, "takeoff_notes"},
    {missionx::enums::mx_note_longField_enum::cruise_notes, "cruise_notes"},
    {missionx::enums::mx_note_longField_enum::descent_notes, "descent_notes"}
  };
}

typedef struct _mxVec3
{
  float x, y, z;
  _mxVec3() { x = y = z = 0.0f; };
  _mxVec3(float _x, float _y, float _z)
  {
    x = _x;
    y = _y;
    z = _z;
  }
} mxVec3;


typedef struct _mxVec4
{
  float x, y, z, w;
  _mxVec4() { x = y = z = w = 0.0f; };
  _mxVec4(float _x, float _y, float _z, float _w)
  {
    x = _x;
    y = _y;
    z = _z;
    w = _w;
  }
} mxVec4;


typedef struct _mxRGB // v3.305.1
{
  float r {255.0f};
  float g {255.0f};
  float b {255.0f};

  _mxRGB()
  {
    r = g = b =255.0f;
  }

  void setRGB(float inR, float inG, float inB)
  {
    r = inR;
    g = inG;
    b = inB;
  }

} mxRGB;


typedef struct _wp_guess_result
{
  int nav_ref{ -1 };
  int nav_type{ 0 };
#ifdef IBM
  double distance_d{ DBL_MAX };
#else
  double distance_d{ std::numeric_limits<double>::max() };
#endif
  std::string name{ "" };
} mx_wp_guess_result;

#ifdef LIN
	typedef enum class _mx_linux_distro
	  : uint8_t
	{
	  debian_ubuntu_distro          = 0,
	  arch_manjaro_garudo_endeavour = 1,
	  others_unknown                = 2
	} mx_linux_distro;
#endif

typedef struct _mx_score_strct
{
  float min{ 0.0f }, max{ 0.0f };
  float score{ 0.5f };
} mx_score_strct;


typedef enum class _mx_btn_colors
  : uint8_t
{
  white,
  yellow,
  red,
  green,
  blue,
  purple,
  orange,
  black
} mx_btn_colors;

typedef enum class _random_thread_wait_state
  : uint8_t
{
  not_waiting = 0, // we need this if designer used wrong string for channel
  waiting_for_plugin_callback_job,
  finished_plugin_callback_job
} mx_random_thread_wait_state_enum;


typedef enum class _cue_actions
  : uint8_t
{
  cue_action_none,
  cue_action_first_time,
  cue_action_refresh
} cue_actions_enum;

typedef enum class _mx_msg_mode : int8_t
{
  mode_default = 0,
  mode_story   = 1
} mx_msg_mode;

typedef enum class _mx_msg_line_type : int8_t
{
  line_type_text = 0,
  line_type_action = 1
} mx_msg_line_type;

typedef struct _mx_fetch_info
{
  size_t      lastPos;
  std::string token;

  _mx_fetch_info()
  {
    lastPos = 0;
    token.clear();
  }
} mx_fetch_info;


typedef enum class _message_channel_type
  : uint8_t
{
  comm       = 0, // comm is also mxpad channel
  background = 1,
  no_type    = 3 // we need this if designer used wrong string for channel
  // pad = 3
} mx_message_channel_type_enum;


// describe message state and progress.
// Some of the stages are internal and very short.
typedef enum class _missionx_message_state
  : uint8_t
{
  msg_undefined,
  msg_abort,           // v3.0.223.7 allow the QMM to skip post message actions
  msg_not_broadcasted, // loaded but not dispatched into message pool yet
  msg_in_pool,
  msg_text_is_ready,
  msg_prepare_channels,
  msg_channels_need_to_finish_loading,
  msg_is_ready_to_be_played,
  msg_broadcast_once,
  msg_broadcast_few_times,
  msg_is_ready_for_post_message_actions, // v3.305.1
} mx_message_state_enum;


typedef enum class _mxWindowActions
  : uint8_t
{
  ACTION_NONE,                                                // initialize action
  ACTION_TOGGLE_WINDOW,                                       // only change window state (show / hide ) should not modify layer.
  ACTION_TOGGLE_BRIEFER,                                      // v3.0.201 when a command is called then it means we want to see LEG info and such. Layer should be flight_leg_info for running. We also ad some logic to this action
  ACTION_HIDE_WINDOW,                                         // v3.0.160
  ACTION_SHOW_WINDOW,                                         // v3.0.201
  ACTION_CANCEL,                                              // v3.0.141
  ACTION_CLOSE_WINDOW,                                        // v3.0.145
  ACTION_OPEN_NAV_LAYER,                                      // v24025
  ACTION_QUIT_MISSION,                                        // v3.0.145
  ACTION_QUIT_MISSION_AND_SAVE,                               // v3.0.251.1 b2
  ACTION_LOAD_MISSION,                                        // v3.0.141
  ACTION_CREATE_SAVEPOINT,                                    // v3.0.151
  ACTION_LOAD_SAVEPOINT,                                      // v3.0.151
  ACTION_START_MISSION,                                       // v3.0.141
  ACTION_START_RANDOM_MISSION,                                // v3.0.219.1
  ACTION_TOGGLE_MAP,                                          // v3.0.201
  ACTION_TOGGLE_INVENTORY,                                    // v3.0.213.2
  ACTION_COMMIT_TRANSFER,                                     // v3.0.213.2
  ACTION_TOGGLE_MISSION_GENERATOR,                            // v3.0.217.1
  ACTION_TOGGLE_CHOICE_WINDOW,                                // v3.0.231.1
  ACTION_GUESS_WAYPOINTS,                                     // v3.0.355.2
  ACTION_GENERATE_RANDOM_MISSION,                             // v3.0.217.2
  ACTION_SET_LEG_INFO,                                        // v3.0.221.15rc5 support leg // v3.0.221.6 in VR mode we won't close Mission-X when closing inventory or map, instead we will set goal info layer
  ACTION_SET_MISSION_LIST,                                    // v3.0.221.6 in VR mode we won't close Mission-X when closing Mission Generator.
  ACTION_TOGGLE_BRIEFER_MXPAD,                                // v3.0.221.7 in VR mode
  ACTION_TOGGLE_PLUGIN_SETUP,                                 // v3.0.241.7 plugin setup screen
  ACTION_TOGGLE_UI_USER_MISSION_GENERATOR,                    // v3.0.241.9 display user mission creation from UI and not template
  ACTION_POST_TEMPLATE_LOAD_DISPLAY_IMGUI_GENERATE_TEMPLATES_IMAGES, // v3.0.251.1 display the list of custom template images
  ACTION_SHOW_END_SUMMARY_LAYER,                              // v3.0.251.1 display the IMGUI summary end window
  ACTION_FETCH_FPLN_FROM_EXTERNAL_SITE,                       // v3.0.253.1 start async fetch process
  ACTION_FETCH_FPLN_FROM_SIMBRIEF_SITE,                       // v25.03.3 start async fetch process from simbrief
  ACTION_FETCH_ILS_AIRPORTS,                                  // v3.0.253.6
  ACTION_FETCH_NAV_INFORMATION,                               // v24025
  ACTION_FETCH_MISSION_STATS,                                 // v3.0.255.1
  ACTION_ABORT_RANDOM_ENGINE_RUN,                             // v3.0.253.6
  ACTION_SAVE_USER_SETUP_OPTIONS,                             // v3.0.255.4.2
  ACTION_GENERATE_MISSION_FROM_LNM_FPLN,                      // v3.0.301
  ACTION_GENERATE_RANDOM_DATE_TIME,                           // v3.303.10
  ACTION_OPEN_STORY_LAYOUT,                                   // v3.305.1 open story layout
  ACTION_RESET_BRIEFER_POSITION                               // v3.305.1 reset Briefer window position to center of window
} mx_window_actions;                                          // for all windows. some actions are specific and some shared

// types of cue triggers, this will help with GL color selecting. I hope to also have combinations of colors
typedef enum class _cue_types
  : uint8_t
{
  cue_none           = 0,
  cue_trigger        = 1,  // yellow
  cue_task           = 5,  // green
  cue_mandatory_task = 7,  // magenta
  cue_sling_task     = 8,  //
  cue_message        = 10, // black
  cue_inventory      = 12, // purple
  cue_script         = 15, // orange
  cue_obj            = 20  // orange ? v3.0.303.6
} mx_cue_types;

typedef enum class _mx_property_type
  : uint8_t
{
  MX_UNKNOWN = 0,
  MX_BOOL    = 1,
  MX_CHAR    = 2,
  MX_INT     = 3,
  MX_FLOAT   = 4,
  MX_DOUBLE  = 5,
  MX_STRING  = 6
} mx_property_type; // v3.0.217.2 moved from mxProperty

// v3.305.1 TODO: should we deprecate this type ?
typedef struct _mx_property_type_as_string_code
{
  const std::string BOOL{ "1" };
  const std::string CHAR{ "2" };
  const std::string INT{ "3" };
  const std::string FLOAT{ "4" };
  const std::string DOUBLE{ "5" };
  const std::string STRING{ "6" };
} mx_property_type_as_string_code;


typedef enum class _enum_layer_state
  : uint8_t // v3.0.253.6
{
  not_initialized                              = 0,
  validating_data                              = 5,
  failed_data_is_not_present                   = 10,
  fatal_database_is_not_initializing_correctly = 15,
  success_can_draw                             = 20
} mx_layer_state_enum;

typedef enum class _enum_ils_types
  : uint8_t // v3.0.253.6
{
  LOC = 0,
  ILS_cat_I,
  ILS_cat_II,
  ILS_cat_III,
  IGS,
  LDA,
  SDF,
  GLS,
  LP,
  LPV
} mx_ils_type_enum;

// The order of _enum_trig_type is crucial since we use it in the conversion UI screen, so camera must be 3 and rad 0
typedef enum class _enum_trig_type
  : uint8_t // v3.0.301
{
  rad    = 0,
  script = 1,
  poly   = 2,
  camera = 3, // v3.0.303.7
  slope = 4
} mx_trig_type_enum;




// v3.0.253.6
static std::map<mx_ils_type_enum, std::string> mapILS_types = { { mx_ils_type_enum::LOC, "LOC" },
                                                                { mx_ils_type_enum::ILS_cat_I, "ILS-cat-I" },
                                                                { mx_ils_type_enum::ILS_cat_II, "ILS-cat-II" },
                                                                { mx_ils_type_enum::ILS_cat_III, "ILS-cat-III" },
                                                                { mx_ils_type_enum::IGS, "IGS" },
                                                                { mx_ils_type_enum::LDA, "LDA" },
                                                                { mx_ils_type_enum::SDF, "SDF" },
                                                                { mx_ils_type_enum::GLS, "GLS" }, // v3.305.4
                                                                { mx_ils_type_enum::LP, "LP" }, // v3.305.4
                                                                { mx_ils_type_enum::LPV, "LPV" } // v3.305.4
                                                              };


typedef struct _messageLine_struct
{
  missionx::mx_msg_line_type type; // if mode is story_mode then "text_line" could be an action and not just a text message
  std::string label;
  std::string label_position;
  std::string label_color;

  std::string message_text; // original message without splitting
                            // std::vector<std::string> textLines; // split message
  std::deque<std::string> textLines; // if line type is "action", then only the first value in the "deque" is relevant.

  missionx::mxVec4 imvec4Color;
  //ImVec4           imvec4Color;

  _messageLine_struct() { init(); }

  _messageLine_struct(const std::string& inLabel, const std::string& inMessageText, const mxVec4& inColor)
    : _messageLine_struct() // v3.305.3
  {
    this->label        = inLabel;
    this->imvec4Color  = inColor;
    this->message_text = inMessageText;
  }

  void init()
  {
    type = missionx::mx_msg_line_type::line_type_text;
    label.clear();
    label_position.clear();
    label_color.clear();
    message_text.clear();
    textLines.clear();
    imvec4Color.x = imvec4Color.y = imvec4Color.z = imvec4Color.w = 1.0f; // white
  }

} messageLine_strct;
// end struct


typedef struct _mx_location_3d_objects // v3.0.213.7 moved from read_mission_file class, to use in Point class too.
{
  std::string distance_to_display_nm, keep_until_leg, cond_script;
  std::string lat, lon, elev, elev_above_ground;
  std::string heading, pitch, roll, speed; // v3.0.202
  std::string adjust_heading;              // v3.0.207.5

  _mx_location_3d_objects()
  {
    lat = lon = elev = elev_above_ground = "";
    distance_to_display_nm = keep_until_leg = cond_script = "";
    heading = pitch = roll = speed = "";
    adjust_heading.clear(); // v3.0.207.5
  }
} mx_location_3d_objects;


// v24.12.2 - requested by @RandomUser in x-plane.org
typedef struct _mx_option_info
{
  int         longest_text_length_i{ 0 };
  int         user_pick_from_replaceOptions_combo_i{ 0 }; // ui
  float       combo_width_f; // ui
  std::string combo_label_s; // ui

  std::string              longestTextInVector_s;  // Longest text from the option names
  std::string              name;                   // holds the <option name="" > attribute value. Will be displayed above the option list.
  std::vector<std::string> vecReplaceOptions_s;    // holds all the <opt> name attribute values.
  std::vector<const char*> vecReplaceOptions_char; // ui, holds all the <opt> name attribute values from the string vector.


  std::vector<const char*> refresh_vecReplaceOptions_char()
  {
    vecReplaceOptions_char.clear();
    for (const auto& opt_name_ptr : vecReplaceOptions_s)
      vecReplaceOptions_char.push_back(opt_name_ptr.c_str());

    return vecReplaceOptions_char;
  }
} mx_option_info;


// v25.02.1 - success task timer information
typedef struct _mx_success_timer_info
{

  float       prevRemainingTime_f{ 9999999.0f };
  std::string triggerNameWithShortestSuccessTimer;
  std::vector<std::string> vecTriggersWithActiveTimers;

  void reset()
  {
    prevRemainingTime_f = 9999999.0f;
    triggerNameWithShortestSuccessTimer.clear();
    vecTriggersWithActiveTimers.clear();
  }

} mx_success_timer_info;



//// a struct to hold UI <option> information
//typedef struct _mx_xy_f
//{
//  float x{ 0.0f };
//  float y{ 0.0f };
//
//  _mx_xy_f() = default;
//  _mx_xy_f(const float& inX, const float& inY)
//  {
//    x = inX;
//    y = inY;
//  }
//
//} mx_xy_f;

// v24.12.2



// v3.0.213.5
constexpr int BRIEFER_WINDOWS_WIDTH  = 1200;
constexpr int BRIEFER_WINDOWS_HEIGHT = 600;

// v3.0.223.1 VR related constants
constexpr int MAX_CHARS_IN_RANDOM_LINE_2D = 95;
constexpr int MAX_CHARS_IN_RANDOM_LINE_3D = 80;
constexpr int MAX_LINES_IN_RANDOM_DESC_2D = 24;
constexpr int MAX_LINES_IN_RANDOM_DESC_3D = (int)(MAX_LINES_IN_RANDOM_DESC_2D );

constexpr int MAX_CHARS_IN_BRIEFER_LINE_2D = 160;
constexpr int MAX_CHARS_IN_BRIEFER_LINE_3D = 145;
constexpr int MAX_LINES_IN_BRIEFER_DESC_2D = 15;
constexpr int MAX_LINES_IN_BRIEFER_DESC_3D = (int)(MAX_LINES_IN_BRIEFER_DESC_2D);



class MxKeyValuePropertiesTypes // Used in ext_script.cpp to hold updatable element properties to modify or to fetch.
{
public:
  MxKeyValuePropertiesTypes()
  {
    key_s.clear();
    key_type = missionx::mx_property_type::MX_UNKNOWN;
  }

  MxKeyValuePropertiesTypes(std::string inKey, missionx::mx_property_type inValType)
  {
    key_s    = inKey;
    key_type = inValType;
  }

  ~MxKeyValuePropertiesTypes() {}

  std::string                key_s;
  missionx::mx_property_type key_type; // represent property type: MX_STRING, MX_DOUBLE, MX_FLOAT, MX_INT, MX_BOOL
};


typedef enum class _mx_plane_type
  : uint8_t
{
  plane_type_any = 0,
  plane_type_helos,
  plane_type_props,
  plane_type_prop_floats, // custom
  plane_type_ga_floats,   // custom
  plane_type_ga,          // custom
  plane_type_turboprops,
  plane_type_jets,
  plane_type_heavy,
  plane_type_fighter
} mx_plane_types;


// v3.0.241.10 sqlite data types
typedef enum class _mx_datatypes
  : int8_t
{
  skip_field = 0,
  int_typ    = 1,
  real_typ   = 2, // real stands for float + double
  text_typ   = 3,
  date_typ   = 4,
  null_typ   = 5,
  float_typ  = 6, // just in case
  double_typ = 7,  // just in case
  zero_type  =10
} db_types;


typedef enum class _uiLayer
  : uint8_t
{
  imgui_home_layer = 0, // contains all options layers
  option_user_generates_a_mission_layer,
  option_setup_layer, // v3.0.241.7
  option_mission_list,
  option_generate_mission_from_a_template_layer, // v3.0.217.1
  option_external_fpln_layer,                    // v3.0.253.1
  option_ils_layer,                              // v3.0.253.6 ILS search layer
  option_conv_fpln_to_mission,                   // v3.0.301 convert FPLN to mission
  flight_leg_info,
  flight_leg_info_map2d, // v3.0.200a2
  flight_leg_info_end_summary,
  flight_leg_info_inventory, // v3.0.213.1
  flight_leg_info_story_mode, // v3.305.1
  about_layer,               // v3.0.253.2
  uiLayerUnset,              // v3.0.160
} uiLayer_enum;


typedef enum class _unitsOfMeasure
  : uint8_t
{
  unit_unsupports,
  ft, // feet/foot
  meter,
  km, // kilometers
  nm  // nautical miles
} mx_units_of_measure;

typedef enum class _mission_type
  : int8_t
{
  not_defined = -1,
  medevac = 0,
  cargo   = 1,
  oil_rig = 2
} mx_mission_type;

// v3.303.14
typedef enum class _mission_subcategory_type
  : int8_t
{
  not_defined      = -1,
  med_any_location = 0,
  med_accident     = 1,
  med_outdoors     = 2,
  oilrig_cargo     = 10,
  oilrig_med       = 11,
  oilrig_personnel = 12,
  cargo_ga         = 20,
  cargo_farming    = 21,
  cargo_isolated   = 22,
  cargo_heavy
} mx_mission_subcategory_type;

typedef enum class _ui_random_date_time_type // v3.303.10
  : uint8_t
{
  xplane_day_and_time = 0, // use current time
  os_day_and_time = 1, // use OS day of year and time (hour)
  any_day_time   = 2, // any day and time, fully random
  exact_day_and_time = 3,
  pick_months_and_part_of_preferred_day = 4
} mx_ui_random_date_time_type;

typedef enum class _ui_random_weather_options // v3.303.12
  : uint8_t
{
  use_xplane_weather = 0, // use x-plane settings = do nothing from plugin side
  pick_pre_defined   = 1,  // Pick one of the 9 pre-defined sets
  use_xplane_weather_and_store = 2 // v3.303.14 deprecated, convert to checkbox   // Let the user define bottom/top layers, wind speed, precipitation and more
} mx_ui_random_weather_options;

typedef enum class _mxFetchState
  : uint8_t
{
  fetch_not_started    = 0,
  fetch_in_process     = 5,
  fetch_ended          = 10,
  fetch_guess_wp       = 15, // guess the waypoint in main plugin call back
  fetch_guess_wp_ended = 16, // guess the waypoint in main plugin call back
  fetch_error          = 20
} mxFetchState_enum;

typedef enum class _mx_filter_ramp_type
  : uint8_t
{
  any_ramp_location     = 0,
  start_ramp            = 1, // example: briefer starting location
  exact_plane_ramp_type = 2,
  airport_ramp          = 5,
  end_ramp              = 10
} mxFilterRampType;

typedef struct _sound_directive_struct
{
  bool bCommandIsActive{ false };         // will be true on first time when it is being handled
  bool bCommandWasEnded{ false };         // will be true when SoundFragment::flc() finish the channel volume change
  bool bCalledRepeatAtLeastOnce{ false }; // store progress of repeat calls. If it is true than we called the repeat code at least once so we should not call it again

  char command{ '\0' };

  int   new_volume{ -1 };
  float startingVolume_f{ 0.0f }; // holds the volume at first time

  float seconds_to_start_f{ -1.0f };
  float transition_time_f{ 0.0f }; // >=0 => immediate
  int   step_i         = { 0 };    // if change volume through time then which step are we from the whole ?
  float increment_by_f = { 0.0f };

  int loopFor_i = { 0 }; // How many times to loop
  int currentLoop_i = { 0 }; // How many loops we had

} mx_track_instructions_strct;

typedef struct _latLon
{
  float lat{ 0.0f };
  float lon{ 0.0f };

  _latLon() {
    lat = 0.0f;
    lon = 0.0f;
  }
  _latLon(float inLat, float inLon)
  {
    lat = inLat;
    lon = inLon;
  }
} mxVec2f;

typedef struct _latLonDouble
{
  _latLonDouble(){};
  _latLonDouble(const double inLat, const double inLon)
  {
    lat = inLat;
    lon = inLon;
  }
  double lat{ 0.0f };
  double lon{ 0.0f };
} mxVec2d;



  typedef struct _stats_tmp_data
{
  bool        bInitialized{ false };
  int         line_id{ -1 };
  int         icao_id{ -1 };
  double      time_passed{ 0.0 };
  double      distance_from_center_of_rw_d{ 0.0 };
  double      score_centerLine{ 0.0 }; // how good between 0.0 and 1.0

  mxVec2d     plane;               // position of plane

  std::string activity_s{ "" }; // takeoff landing
  std::string icao{ "N/A" };
  std::string runway_s{ "N/A" };

} mx_stats_data;


typedef struct _ext_internet_fpln_strct
{
  int         internal_id{ -1 };
  int         fpln_unique_id{ -1 }; // holds plan unique id from site
  std::string fromICAO_s;
  std::string toICAO_s;
  std::string fromName_s;
  std::string toName_s;
  std::string flightNumber_s;
  double      distnace_d{ 0.0 };

  int         maxAltitude_i{ 0 };
  std::string notes_s;

  std::string popularity_s; // v3.0.253.3
  int         popularity_i{ 0 };  // v3.0.253.3

  int                                     waypoints_i{ 0 };
  std::string                             encode_polyline_s;
  std::list<missionx::mxVec2f>            listNavPoints;
  std::list<missionx::mx_wp_guess_result> listNavPointsGuessedName; // v3.0.255.2

  std::string formated_nav_points_s;                    // holds the list of nav points but in formated string
  std::string formated_nav_points_with_guessed_names_s; // v3.0.255.2 holds the list of nav points but in formated string and guessed wp names, not just coordinates

  std::string simbrief_route; // v25.03.3
  std::string simbrief_route_ifps; // v25.03.3
  std::string simbrief_route_navigraph; // v25.03.3

  std::string simbrief_sid; // v25.03.3
  std::string simbrief_star; // v25.03.3

  std::string simbrief_from_rw; // v25.03.3
  std::string simbrief_to_rw; // v25.03.3

  std::string simbrief_from_trans_alt; // v25.03.3
  std::string simbrief_to_trans_alt; // v25.03.3

  void reset()
  {
    internal_id    = -1;
    fpln_unique_id = -1;
    fromICAO_s.clear ();
    toICAO_s.clear ();
    fromName_s.clear ();
    toName_s.clear ();
    flightNumber_s.clear ();
    distnace_d = 0.0;

    maxAltitude_i = 0;
    notes_s.clear();

    popularity_s.clear();
    popularity_i=0;

    waypoints_i = 0;
    encode_polyline_s.clear();
    listNavPoints.clear();
    listNavPointsGuessedName.clear();

    formated_nav_points_s.clear();
    formated_nav_points_with_guessed_names_s.clear();

    formated_nav_points_s.clear();
    formated_nav_points_with_guessed_names_s.clear();
    simbrief_route.clear();
    simbrief_route_ifps.clear();
    simbrief_route_navigraph.clear();
    simbrief_sid.clear();
    simbrief_star.clear();
    simbrief_from_rw.clear();
    simbrief_to_rw.clear();
    simbrief_from_trans_alt.clear();
    simbrief_to_trans_alt.clear();
  }

} mx_ext_internet_fpln_strct;



const static struct _fplndb_json_keys_struct
{
  const std::string Z_KEY_id{ "id" };
  const std::string Z_KEY_popularity{ "popularity" }; // v3.0.253.3
  const std::string Z_KEY_distance{ "distance" };
  const std::string Z_KEY_encodedPolyline{ "encodedPolyline" };
  const std::string Z_KEY_flightNumber{ "flightNumber" };
  const std::string Z_KEY_fromICAO{ "fromICAO" };
  const std::string Z_KEY_fromName{ "fromName" };
  const std::string Z_KEY_maxAltitude{ "maxAltitude" };
  const std::string Z_KEY_notes{ "notes" };
  const std::string Z_KEY_toICAO{ "toICAO" };
  const std::string Z_KEY_toName{ "toName" };
  const std::string Z_KEY_waypoints{ "waypoints" };
} mx_fplndb_json_keys;

// v3.0.253.6 holds the query result from ILS query
typedef struct _ils_airport_row_strct
{
  int    seq{ -1 }; // used by the plugin to distinguish between the rows
  int    ap_elev_ft_i{ 0 };
  int    loc_bearing_i{ 0 };
  int    rw_length_mt_i{ 0 };
  int    loc_frq_mhz{ 0 };
  double distnace_d{ 0.0 };
  double rw_width_d{ 0.0 };
  double bearing_from_to_icao_d{ 0.0 }; // bearing relative to start icao v3.0.253.13


  std::string toICAO_s{ "" };   // target airport name
  std::string toName_s{ "" };   // target airport name
  std::string loc_rw_s{ "" };   // localaizer runway
  std::string locType_s{ "" };  // localaizer type
  std::string surfType_s{ "" }; // Surface type v3.0.253.13

} mx_ils_airport_row_strct;


typedef struct _leg_enrout_stats_strct
{
  std::string sLegName{ "" };
  float       fDistanceFlew{ 0.0f };
  float       fCumulativeDistanceFlew_beforeCurrentLeg{ 0.0f }; // holds the distance flew before the start of the current leg.
  float       fStartingPayload{ 0.0f };
  float       fTouchDownWeight{ 0.0f }; // payload + fuel
  float       fTouchDown_acf_m_max{ 0.0f }; // Max weight allowed for plane at the touch-down point. This to make sure user won't change plane after touch down and we will have wrong info.
  float       fEndPayload{ 0.0f };
  float       fStartingFuel{ 0.0f };
  float       fEndFuel{ 0.0f };

  float fMaxG{ 0.0f };
  float fMinG{ 0.0f };
  float fMaxPitch{ 0.0f };
  float fMinPitch{ 0.0f };
  float fMaxRoll{ 0.0f };
  float fMinRoll{ 0.0f };

  float fTouchDown_landing_rate_vh_ind_fpm{ 0.0f };
  float fTouchDown_gforce_normal{ 1.0f };
  float fLanding_vh_ind_fpm_avg_15_meters{ 0.0f };
  float fLanding_gforce_normal_avg_15_meters{ 0.0f };
  std::vector<float> vecFpm15Meters{};
  std::vector<float> vecgForce15Meters{};

  void init() { sLegName.clear();
    fDistanceFlew = fCumulativeDistanceFlew_beforeCurrentLeg = 0.0f;
    fStartingPayload = fEndPayload = 0.0f;
    fStartingFuel = fEndFuel = 0.0f;
    fTouchDownWeight = 0.0f;
    fTouchDown_acf_m_max     = 0.0f;
    fMaxG = fMinG = 0.0f;
    fMaxPitch = fMinPitch = 0.0f;
    fMaxRoll = fMinRoll = 0.0f;
    fTouchDown_landing_rate_vh_ind_fpm = fLanding_vh_ind_fpm_avg_15_meters = 0.0f;
    fTouchDown_gforce_normal = fLanding_gforce_normal_avg_15_meters = 0.0f;


    vecFpm15Meters.clear();
    vecgForce15Meters.clear();
  }

  void reset_fpm_and_gForce_data()
  {
    fLanding_vh_ind_fpm_avg_15_meters = 0.0f;
    vecFpm15Meters.clear();

    fLanding_gforce_normal_avg_15_meters = 0.0f;
    vecgForce15Meters.clear();
  }

} mx_enrout_stats_strct;


typedef struct _mission_stats_strct // v3.0.255.1
{
  std::vector<double>      vecSeq_d; // line_id used by the plugin to distinguish between the rows
  std::vector<double>      vecElev_mt_d;
  std::vector<double>      vecElev_ft_d;
  std::vector<double>      vecFlaps_d;
  std::vector<double>      vecDayInYear_d;     // day in year
  std::vector<double>      vecLocalTime_sec_d; // local time from midnight
  std::vector<double>      vecAirSpeed_d;      //
  std::vector<double>      vecGroundSpeed_d;   //
  std::vector<double>      vecFaxil_d;         //
  std::vector<double>      vecRoll_d;          //
  std::vector<std::string> vecActivity_s;      // takeoff and landing, in most cases the value is empty

  double      min_elev_ft{ 0.0 }, max_elev_ft{ 0.0 }, min_airspeed{ 0.0 }, max_airspeed{ 0.0 };
  double      time_flew_sec_d{ 0.0 };

  // v3.303.8.3 MIN/MAX stats_vu
  double minPitch, maxPitch;
  double minRoll, maxRoll;
  double minGforce, maxGforce;
  double minFaxilG, maxFaxilG;
  double minAirspeed, maxAirspeed;
  double minGroundSpeed, maxGroundspeed;

  //std::string landing_stats_s{ "" }; // v3.303.8.3

  std::string time_flew_format_s{ "" };

  std::vector<missionx::mx_stats_data> vecLandingStatsResults;  // v3.303.8.3
  float                                pitchScore{ 0.0f }; // v3.303.8.3
  float                                rollScore{ 0.0f };  // v3.303.8.3
  float                                gForceScore{ 0.0f }; // v3.303.8.3

  _mission_stats_strct() { reset(); }

  void reset()
  {
    vecSeq_d.clear();
    vecElev_mt_d.clear();
    vecElev_ft_d.clear();
    vecFlaps_d.clear();
    vecDayInYear_d.clear();
    vecLocalTime_sec_d.clear();
    vecLocalTime_sec_d.clear();
    vecAirSpeed_d.clear();
    vecFaxil_d.clear();
    vecRoll_d.clear();
    vecActivity_s.clear();

    min_elev_ft = max_elev_ft = -4000.0;
    min_airspeed = max_airspeed = 0.0;

    time_flew_sec_d = 0.0;
    time_flew_format_s.clear();

    vecLandingStatsResults.clear();

    pitchScore = rollScore = gForceScore = 0.0f;
  }

} mx_mission_stats_strct;

typedef struct _mx_clock_time
{
  static constexpr int CHRONO_START_YEAR = 1900;

  int year{ CHRONO_START_YEAR };
  int month{ 1 };
  int dayInMonth{ 0 };
  int dayInYear{ 0 }; // starts in 0..355
  int hour{ 0 };
  int minutes{ 0 };
  int seconds_in_day{ 0 };

} mx_clock_time_strct;

typedef struct _mx_aptdat_cached_info
{
  bool                   isCustom{ false };
  std::string            boundary; // v3.303.8.3 each node(x,y) will be terminated with "|"
  std::list<std::string> listNavInfo;

  void clear()
  {
    isCustom = false;
    boundary.clear();
    listNavInfo.clear();
  }

} mx_aptdat_cached_info;


//// https://github.com/jpreiss/array2d


  namespace dref
  {
    const static std::string DREF_SHARED_DRAW_ROTATION = "missionx/obj3d/rotation_per_frame_1_min";

  }

namespace conv
  {
  constexpr int DEFAULT_RADIUS_LENGTH_FOR_GROUND_WP = 60;
  constexpr int DEFAULT_RADIUS_LENGTH_FOR_AIRBORNE_WP = 2000;

  };


} // end missionx namespace


#endif // XX_MX_CONSTANTS_HPP
