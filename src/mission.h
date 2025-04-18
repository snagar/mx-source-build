#ifndef MISSION_H_
#define MISSION_H_

#pragma once

#include "core/base_c_includes.h"
#include "core/base_xp_include.h"
#include "core/xx_mission_constants.hpp"

// Gather Stats v3.0.255.1
//#ifdef GATHER_STATS
#include "core/thread/gather_stats.h"
//#endif

#include "core/Timer.hpp"
#include "core/Utils.h"
//#include "core/data_manager.h"
#include "core/embeded_script/ext_script.h" // includes QMM and data_manager
// load mission
#include "io/read_mission_file.h"
#include "io/system_actions.h"

// UI
#include "ui/core/BitmapReader.h"


// IMGUI test widget
#include "ui/gl/WinImguiBriefer.h" // v3.0.251.1
#include "ui/gl/WinImguiMxpad.h"   // v3.0.251.1
#include "ui/gl/WinImguiOptions.h" // v3.0.251.1


// RANDOM
#include "io/OptimizeAptDat.h"
#include "random/RandomEngine.h"

// Bind Commands
#include "logic/BindCommand.h"

using namespace missionx;

namespace missionx
{

typedef std::shared_ptr<WinImguiOptions> ImgOptionsSPtrTy;
typedef std::shared_ptr<WinImguiMxpad>   ImgMxpadSPtrTy;
typedef std::shared_ptr<WinImguiBriefer> ImguiBrieferSPtrTy;

const static auto PLUGIN_VERSION_S = fmt::format("{} {} {}", missionx::PLUGIN_VER_MAJOR, missionx::PLUGIN_VER_MINOR, missionx::PLUGIN_REVISION_S);

class Mission
{
private:

  const missionx::dataref_const drefConst;
  missionx::OptimizeAptDat      optAptDat;

  //missionx::GatherStats gather_stats; // v3.0.255.1 // v3.303.14 Moved to data_manager
  
  // v3.303.14 used in Draw loop back to determined landing/takeoff state and then use the bursting. But we call gather_stats.xxx to store the stats
  // We should not use the "gather_stats_drawLoopBack" to store into the stats DB.
  //missionx::GatherStats gather_stats_drawLoopBack; // USLESS
  //std::vector<missionx::mx_stats_data> get_landing_stats_as_vec(); // v3.303.8.3

  void flc_threads();

  missionx::RandomEngine engine;
  void                   setMissionTime();
  bool                   flag_found{false}; // used with XML node reading

  int  flight_leg_progress_counter_i{ 0 }; // starts with 0, will be used with global GPS to recognize which point in the <gps> global element we should add to the FMS/GPS unit once Leg transitioned.
  void add_GPS_data(int optionalPointIndex = -1);

  // stats information // v3.0.255.1
  typedef struct _mx_plane_stats
  {

    Timer burstTimer;
    float prev_faxil{ 0.0f };    // v3.0.255.1
    float current_faxil{ 0.0f }; // v3.0.255.1

    enum class mx_plane_status
    {
      no_status = 0,
      decide_state,
      start_on_ground,
      start_airborn,
      plane_is_landing,
      plane_is_taking_off,
      transition, // after taking off or touchdown (plane_is_landing)
      enroute,    // after transition
      landed      // after transition
    };

    mx_plane_status current_plane_state{ mx_plane_status::no_status };
    mx_plane_status prev_plane_state{ mx_plane_status::no_status };
    mx_plane_status before_transition_state{ mx_plane_status::no_status };

    void reset()
    {
      burstTimer.reset();
      prev_faxil          = 0.0f; //
      current_faxil       = 0.0f; //
      current_plane_state = prev_plane_state = before_transition_state = mx_plane_status::no_status;
    }

    void set_plane_state(mx_plane_status inVal)
    {
      prev_plane_state    = current_plane_state;
      current_plane_state = inVal;
    }

  } mx_plane_stats;
  mx_plane_stats plane_stats; // v3.0.255.1

  int delay_camera_position_change{ 0 }; // when positioning camera, we want to automaticaly move to free view. We need few cycles before doing so

public:
  Mission();
  virtual ~Mission();

  // core attributes

  static bool                     isMissionValid;
  static std::string              currentObjectiveName; // useful for extSetTask()
  static std::vector<std::string> vecGoalsExecOrder;

  // X-Plane Menu Methods & Members
  // Menu Commands/sub menus
  typedef enum class _menuIdRefs
    : uint8_t
  {
    MISSIONX_MENU_ENTRY, // Menu seed under plug-in menu
    MENU_GAUGE,
    MENU_TOGGLE_MISSIONX_BRIEFER,
    MENU_RESET_BRIEFER_POSITION, // v3.305.1 will center the briefer window once it will be visible.
    MENU_TOGGLE_MXPAD,
    MENU_TOGGLE_MAP,
    MENU_DEBUG,
    MENU_OPTIONS,
    MENU_TOOLS, // v3.0.221.15rc3.4
    MENU_APT_DAT_OPTIMIZATION,  // V3.0.219.12
    MENU_TOGGLE_CHOICE_WINDOW,  // V3.0.231.1
    MENU_OPEN_LIST_OF_MISSIONS, // until we will create briefer and load screen
    MENU_LOAD_DUMMY_MISSION,    // until we will create briefer and load screen
    MENU_CREATE_SAVEPOINT,      // until we will create briefer and save checkpoint screen
    MENU_LOAD_SAVEPOINT,        // until we will create briefer and load checkpoint screen
    MENU_START_MISSION,         // until we will create briefer and load screen
    MENU_STOP_MISSION,          // until we will create briefer and load screen
    MENU_OPTIONS_MENU,
    MENU_OPTIONS_TOGGLE_OPTIONS,
    //MENU_RELOAD_PLUGINS, // v3.305.2
    MENU_DUMMY_OPTIONS_PLACE_HOLDER,
    MENU_DUMMY_TOOLS_PLACE_HOLDER,                                
    MENU_TOOLS_UPDATE_POINT_IN_FILE_WITH_TEMPLATE_BASED_ON_PROBE, 
    MENU_TOOLS_CREATE_EXTERNAL_FPLN_BASED_ON_GPS, // v3.0.255.4.4
    MENU_TOOLS_WRITE_PLANE_POSITION_TO_LOG, // v3.303.14
    MENU_TOOLS_WRITE_CAMERA_POSITION_TO_LOG, // v3.303.14
    MENU_TOOLS_WRITE_WEATHER_STATE_TO_LOG, // v3.303.13
    MENU_OPTION_DISPLAY_MISSIONX_IN_VR,       // v3.0.221.7
    MENU_OPTION_AUTO_PAUSE_IN_VR,             // v3.0.221.6
    MENU_OPTION_DISABLE_PLUGIN_COLD_AND_DARK, // v3.0.221.10
    MENU_OPTION_WRITE_FPLN_TO_XPLANE_FOLDER,  // v3.0.241.2 write flight plans to xplane FMS folder or use the fpln_folders.ini file
    MENU_OPTION_TOGGLE_DESIGNER_MODE,
    MENU_OPTION_TOGGLE_CUE_INFO,
    MENU_OPTION_TOGGLE_MXPAD
  } mx_menuIdRefs;
  // mx_menuIdRefs menuIdRefs;

  // Main Mission Menu
  XPLMMenuID missionMenuEntry{0};
  XPLMMenuID missionMenuOptionsEntry{0};
  XPLMMenuID missionMenuToolsEntry{0};
  void prepareMissionBrieferInfo(); // v3.0.241.10 b3 The function will reinitialize the list of missions. This will allow a "template mission folder" to be found once we hit the "start" mission in the "Generate Random" mission screens
  void MissionMenuHandler(void* inMenuRef, void* inItemRef);

  // Menus for Mission
  struct _missionx_menu
  {
    int newBrieferMenu;
    int mxpadMenu;
    int optimizeAptDatMenu;
    int toggleChoiceMenu; // v3.0.231.1
    int optionsMenu;
    int reloadPluginsMenu; // v3.305.2
    int toolsMenu;
    int toolsCreateExternalFPLN_fromGPS; // v3.0.255.4.4
    int toolsModifyPointFileBaseOnTerrainProbe;
    int toolsWritePlanePosToLog; // v3.303.14
    int toolsWriteCameraPosToLog; // v3.303.14
    int toolsWriteWeatherToLog; // v3.303.13
    int optionToggleDesignerMode;
    int optionToggleCueInfo;
    int optionAutoHideMxPad;
    int optionIgnoreAptdatCache;
    int optionDisablePluginColdAndDark; // v3.0.221.10
    int ui_compatibility_menu;
  } mx_menu{};

  void syncOptionsWithMenu() const;

  // -----------------------------------------

  // core members
  static void init();
  void initImguiParametersAtPluginsStart(); // v3.0.253.7.rc2 initialize some of the strct_xxx after plugin is enabled and breifer has been initialized

  // "prepareMission" will check validity of loaded mission
  // will call "mission.preStartMission" external function.
  bool prepareMission(std::string& outError);

  // "START_MISSION" is valid if "prepareMission()" returns true.
  // will call "mission.postStartMission" external function.
  void        START_MISSION();
  static void readCurrentMissionTextures(); // v3.0.156

  void flc(); // flight callback

  void exec_apt_dat_optimization();

  //// https://www.learncpp.com/cpp-tutorial/function-pointers1
  //void prepare_func_pointers();
  //void (*fcnDrawPtr)(const XPLMDrawingPhase&, const int, void*) = { nullptr };

private:
  std::string checkGLError (const std::string &label); // v24.06.1 try to figure out the crash when loading inventory images.

  void flc_legs (); // flight callback
  void flc_fail_timers ();

  bool handle_choice_option ();

  // Evaluate objectives tasks.
  // At the end of ALL evaluations, you must check mandatory tasks to decide if we can flag Objective as complete (though it might need to continue evaluation).
  void flc_objective (std::string &inObjName, mxProperties &inSmPropSeedValues);
  // Loop over task and its dependency.
  // return true if completed (include dependent tasks)
  bool flc_task (const std::string &inTaskName, Objective &obj, mxProperties &inSmPropSeedValues);
  bool flc_trigger (Trigger &trig, mxProperties &inSmPropSeedValues, const bool &isTaskMandatory = false);

  void flc_3d_objects (mxProperties &inSmPropSeedValues); // which 3d instance object should be displayed
  void flc_check_plane_in_external_inventory_area ();


  void initFlightLegDisplayObjects (); // v3.0.200 // we will use currentGoal // if goal has <display_object> we should add it to the instance map


  // CLEARING
  void stopDataManagerAndClearScriptManager ();

  // PRE flight loop back
  void flcPRE (); // Commands that Mission class should take before issuing all other flc() calls

  // Metar
  void        injectMetarFile (); // tell X-Plane to use new METAR file
  void        AddFlightLegFailTimers (); // Add the fail definition when flight leg starts
  static bool usingCustomMetarFile;

  void             start_cold_and_dark ();
  int              frameCounter_v2{ 0 };
  XPLMDrawingPhase drawPhase{ xplm_Phase_Modern3D };

  // v3.305.2
  void  flc_check_success ();
  Timer timerOptLegTriggersTimer;

public:
  void             setDrawingPhase (const XPLMDrawingPhase &inDrawPhase) { drawPhase = inDrawPhase; }
  XPLMDrawingPhase getDrawingPhase () const { return drawPhase; }
  static int       displayCueInfo; // v3.0.203.5 // evaluate every FlightLoopBack, reading from missionx::system_actions::pluginSetupOptions
  void             drawCallback (const XPLMDrawingPhase &inPhase, const int inIsBefore, void *inRefcon); // draw callback
  void             drawMarkings (XPLMMapLayerID layer, const float *inMapBoundsLeftTopRightBottom, float zoomRatio, float mapUnitsPerUserInterfaceUnit, XPLMMapStyle mapStyle, XPLMMapProjectionID projection, void *inRefcon); // draw callback
  void gatherStats_identify_takeoff_and_landings (); // part of draw callback. Will check if to store information regarding takeoff and landing

  // checkpoint
  void storeCoreAttribAsProperties ();
  void applyPropertiesToLocal ();
  bool saveCheckpoint ();
  bool loadCheckpoint ();

  // flc action members
  void stopMission ();
  void loadMission ();

  /////// UI ///////////
  void setUiEndMissionTexture (); // v3.0.156

  static ImgOptionsSPtrTy   uiImGuiOptions; // ImGUI: Options 2D window
  static ImgMxpadSPtrTy     uiImGuiMxpad; // ImGUI: mxpad 2D window
  static ImguiBrieferSPtrTy uiImGuiBriefer; // ImGUI: mxpad 2D window

  // void initFonts();
  void prepareUiMissionList ();

  //// plugin related members - for init and cleanup
  void stop_plugin (); // v3.0.149

  // v3.0.221.15rc3.4 tool to modify <points> in a file and set its template attribute based on terrain probe (hover/land)
  bool read_and_update_xml_file_point_elements_base_on_terrain_probing (std::string inFileName);
  int  parseAndModifyChildPoints (IXMLNode &inParent, int inLevel); // return how many elements processed
};

} // namespace
#endif
