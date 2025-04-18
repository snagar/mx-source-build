#ifndef BASE_DEFINE_CONST_H_
#define BASE_DEFINE_CONST_H_

/**************


**************/

#include "base_c_includes.h"

namespace missionx
{

#ifdef __cplusplus
extern "C"
{
#endif
  // PLUGIN CONSTANTS

  // const static int NULL = 0;

#define PLUGIN_VER_MAJOR "v2.1.29.91"
#define PLUGIN_REVISION  ""
  const static std::string PLUGIN_VER = "200";

  const static std::string PLUGIN_DIR_NAME  = "missionx";
  const static std::string TEXTURE_DIR_NAME = "gui";
  const static std::string DB_FILE_NAME     = (PLUGIN_DIR_NAME + ".sqlite"); // the name of SQLITE database file

  // const static std::string DESIGNER_DESC = ""; // release
  const static std::string DESIGNER_DESC = "* Random Weather;* Embeded Script 40%.";
  // const static std::string DESIGNER_DESC = "* Support windows DLL reading.;* Implemented math_parser.;* re-write eval function to check the logic for param & expressions.;* Fixed multi condition in checkLogicOfObjSuccess().;* Metar
  // Inject.;* Cue messages;* freeType alpha 1";


#ifdef ENABLE_SOUND
  const static std::string PLUGIN_SOUND = ";(With FMOD sound support);" + DESIGNER_DESC;
  // static unsigned int i_sound_dll_loaded = 0;
#else
const static std::string PLUGIN_SOUND = ";(NO FMOD sound support)" + DESIGNER_DESC;
#endif


  // GLOBAL ENUMS

  enum Progress
  {
    enumNotStarted,
    enumQuit,
    enumStarted,
    enumEnded,
    enumPaused,
    enumInProgress
  };



  // GLOBAL CONSTANTS
  const static double PI  = 3.1415926535897932384626433832795; // from windows calc
  const static double PI2 = 6.283185307179586476925286766559;  // FOR DROPPING calculation


// static double Gravity=-9.81;
#define POWER2(a)  ((a) * (a))
#define DEG2RAD(a) ((a)*0.01745329251994329576923690768489)
#define RAD2DEG(a) ((a)*57.295779513082320876798154814105)

  const static float EARTH_RADIUS_M = 6378137.0f;


  const float RadToDeg   = (float)(180.0f / PI);
  const float DegToRad   = 1.0f / RadToDeg;
  const float feet2meter = 1.0f / 3.28083f;
  const float meter2feet = 3.28083f;
  // const float meter2nm = 2.0f; // ORIGINAL v2.1x but seem wrong.// according to Sagi, meter/sec to nm/hr (basically speed)
  const float meter2nm             = 1.0f / 1852.0f; // copied from v2.0x
  const float nm2meter             = 1.0f * 1852.0f; // v2.1.23
  const float secInHour            = 3600.0f;        // v2.1.26.a7
  const float OneNmh2meterInSecond = 0.5144444f;     // v2.1.26.a7


  const int   OUT_OF_BOUNDING_ALERT_TIMER_SEC = 30; // 30 sec. alert will broadcast every 30 seconds
  const float MISSIONX_DOUBLE_CLICK           = 0.9f;

  const static std::string EMPTY_STRING   = "";
  const static std::string condStr[6]     = { "lt;", "lte;", "=", "!=", "gt;", "gte;" };
  const static std::string MSG_SEP        = ";----------------;;";
  const static std::string PREF_FILE_NAME = "missionx_pref.xml";

  const static std::string CALC_RADIUS_BASE_ON_TARGET = "target";
  const static std::string CALC_RADIUS_BASE_ON_SOURCE = "source";

  // Inventory Item consts
  const static short ADD    = 1;
  const static short REMOVE = -1;

  const static int BRIEFER_DESC_CHAR_WIDTH = 70;

  // Global Char Buffer for Loggin
  const int   LOG_BUFF_SIZE = 2048;
  static char LOG_BUFF[LOG_BUFF_SIZE];


  // 21638.7 Naotical Miles; 40075.0 km  24901.5 miles
  const static float EQUATER_LEN_NM = 21638.7f;
  // const static float EQUATER_LEN_KM = 40075.0;
  // const static float EQUATER_LEN_MI = 24901.5;
  // const static float EARTH_AVG_RADIUS_KM = 6371.01;
  const static float EARTH_AVG_RADIUS_NM = 3440.07019148103f;

  const static float FEET_TO_NM    = 6076.11549f;
  const static float KM_TO_NM      = 1.852f;
  const static float METER_TO_FEET = 3.2808399f;
  const static float FEET_TO_METER = 0.3048f;
  const static float METER_TO_NM   = 0.000539956803f;

  const static int BUFF_SIZE = 20;

  const static short STATE_LAND = 1;
  const static short STATE_AIR  = 2;

  const static short SCENERY_EARTH = 0;
  const static short SCENERY_MARS  = 1;

  const static int WINDOW_WIDTH  = 130;
  const static int WINDOW_HEIGHT = 65;

  const static int MAX_TOKEN_BUF      = 100;
  const static int CAPTION_CORRECTION = 3;

  const static char  WIDGET_NEWLINE_CHAR        = ';'; //
  const static short IGNORE_DISTANCE_TOLLERENCE = -1;

  // Widget New Line
  const static std::string WNL   = ";";  // Widget New line, used with List Widgets
  const static std::string W_TAB = "  "; // two space


  const static std::string TIMER_TYPE_AREA     = "area";     // used in Time class
  const static std::string TIMER_TYPE_ZULU     = "zulu";     // used in Time class
  const static std::string TIMER_TYPE_AREAZULU = "areaZulu"; // used in Time class
  const static std::string TIMER_CLOCK_STOPPER = "stopper";  // used in Time class
  const static std::string TIMER_CLOCK_EXACT   = "exact";    // used in Time class

  const static std::string AREA_TYPE_RADIUS = "rad";
  const static std::string AREA_TYPE_POLY   = "poly";
  const static std::string AREA_TYPE_SLOPE  = "slope"; // slope / angle
  const static std::string AREA_TYPE_LOGIC  = "logic"; // logic is trigger event only type. Does not need an Area

  const static std::string AREA_RULE_ENTER      = "enter";        // default: Simple no time restriction rule
  const static std::string AREA_RULE_TIMER      = "timer";        // how much time to reach area ( min )
  const static std::string AREA_RULE_TIMER_STAY = "timerStay";    // how much time can stay in the area ( min )
  const static std::string AREA_RULE_RESTRICT   = "restrictTime"; // Same as TIMER_STAY
  const static std::string AREA_RULE_BOUNDS     = "bound";        // Bound of area
  const static std::string AREA_RULE_HIT        = "hit";          // Hit in area
  const static std::string AREA_RULE_LOGIC      = "logic";        // Logic Rule. Only rule for Area Type Logic

  // Event constants
  const static std::string EVENT_ONENTER       = "onEnter";
  const static std::string EVENT_ONLEAVE       = "onLeave";
  const static std::string EVENT_ONFAIL        = "onFail";
  const static std::string EVENT_ONSUCCESS     = "onSuccess";
  const static std::string EVENT_ONTIMEELAPSED = "onTimeElapsed";
  const static std::string EVENT_ONSELF        = "onSelf";      // on CHECK logic, means to apply all THEN events. no need for LogicApply tag in other event
  const static std::string EVENT_ONHIT         = "onHit";       //
  const static std::string EVENT_ONMISSED      = "onHitFailed"; //
  const static std::string EVENT_ONDROP        = "onDrop";      // v2.07rc3

#define POSITION_LAST    1
#define POSITION_CURRENT 2

  const static std::string AUTO_MSG_NONE    = "none";
  const static std::string AUTO_MSG_PLUGIN  = "plugin";
  const static std::string AUTO_MSG_MISSION = "mission";
  const static std::string AUTO_MSG_ALL     = "all";


  const static std::string TARGET_TYPE_PICK = "pick"; // used in Objective class nad part os xml Target definition

  const static int MAP_PANEL_WIDTH        = 1024;
  const static int MAP_PANEL_HEIGHT       = 512;
  const static int MAP_PANEL_IMAGE_HEIGHT = 16; // 512.0 // should be a stripe



  const static unsigned int INVALID_STEP_SEQ_ID = 0;       // v2.1.27.1
  const static unsigned int OBJ_START_ID        = 1;       // modified from 0 to 1 in v2.1.27.1
  const static int          OBJ_END_ID          = 9999;    // max FMS
  const static std::string  END_STEP_NAME       = "end";   //
  const static std::string  FIRST_STEP_NAME     = "first"; //
  const static std::string  START_STEP_NAME     = "start"; //

  const static unsigned int MAX_FUEL_TANKS_IN_PLANE = 9; // v2.1.26a9

  // const static double FEET_TO_METER = 3.28083;


  const static std::string STORE_DEFAULT_NAME       = "world";
  const static std::string STORE_PLANE_DEFAULT_NAME = "plane";


  // BRIEFER Constants
  const static int BRIEFER_X_WIDTH  = 600;
  const static int BRIEFER_Y_HEIGHT = 390;
  const static int MIN_Y_BORDER     = 20;
  const static int MIN_X_BORDER     = 20;

  const static int    LIMIT_3D_OBJECTS         = 150;
  const static double LIMIT_3D_POINTS_DISTANCE = 150.0;


  const static int    IGNORE_VALUE     = -999; // under Weather/Force settings
  const static double INVALID_DISTANCE = -1.0; // v2.1.27.1 // used when plane returns invalid PointXY* address. Used in base_target.getDistanceToTarget()


#define ITEM_DEFULT_TYPE  "s" // static
#define ITEM_DROP_TYPE    "d" // drop-able
#define ITEM_CONSUME_TYPE "c" // consumable
#define ITEM_MISSION_TYPE "m" // mission

#define INV_LINES 5


  // SOUND
  const static std::string DEFULT_SOUND_TYPE = "hw";

  enum enumMissionXTargetTypes
  {
    NONE_TARGET,
    BRIEFER,
    STATIC_TARGET,
    MOVING_OBJECT_3D, // any moving 3D object
    MOVING_TARGET,    // only if is linked to a target
    AREA_TARGET,
    ATTACHED_TRIGGER, // v2.00
    LOGIC_TARGET,     // v1.30
    FEEDBACK,         // v1.30
    STORE,            // v1.30
    TRIGGER_ZONE,     // v2.00  // not in use yet, since AREA_TARGET is the default for all Area classes
    TRIGGER_LOGIC,
    ATTACHED_EVENT, // v1.30
    STEP,           // v2.00
    DROP_OBJECT,    // v2.00
    REFUEL_LOCATION // v2.1.0 a23
  };


  enum
  {
    LESSER           = 10,
    LESSER_OR_EQUAL  = 20,
    EQUAL            = 30,
    NOT_EQUAL        = 40,
    GREATER          = 50,
    GREATER_OR_EQUAL = 60
  };


  // LOGIC

  const static int MAX_DRILL_LEVEL = 5;


  // OPENGL / Cue


  const static unsigned int NUM_CIRCLE_POINTS = 359;

  const static std::string XPLANE_METAR_FILENAME = "METAR.rwx"; //

  const float SECONDS_IN_DAY     = 86400.0f;
  const float SECONDS_IN_MINUTES = 60.0f; // v2.1.29.7

  const double NEARLY_ZERO = 0.0000000001;

#ifdef __cplusplus
}
#endif


} // end namespace

#endif // BASE_DEFINE_CONST_H_
