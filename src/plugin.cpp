#include "plugin.h"
#ifndef LIN
#include "lr_crash.h" // v3.0.255.4.4
#endif
#include "core/base_xp_include.h"
#include "core/QueueMessageManager.h"
#include "mission.h" // include data_manager

#ifdef LIN
// __asm__(".symver realpath,realpath@GLIBC_2.35");
#endif



namespace missionx
{
//// https://www.learncpp.com/cpp-tutorial/function-pointers1

#ifdef PLUGIN_EVALUATE_PERFORMANCE
double callback_duration     = 0;
double drawcallback_duration = 0;
#endif
mxconst mx_const; // first initialization before other classes
Mission mission;
void    MissionMenuHandler(void* inMenuRef, void* inItemRef);
void    OptionsMenuHandler(void* inMenuRef, void* inItemRef); // v3.0.219.12+
void    toolsMenuHandler(void* inMenuRef, void* inItemRef);   // v3.0.219.15rc3.4

static float pluginCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon); // missionx FLC
int          drawCallback_missionx(XPLMDrawingPhase inPhase, int inIsBefore, void* inRefcon);                                     // draw dlc = draw loop call

/* ******** Commands ************************** */

// XPLMCommandRef dummyRef = NULL; // v3.0.213.5
std::map<std::string, XPLMCommandRef> mapMxCommands;
std::map<XPLMCommandRef, int> mapSelCmdRef; // v3.0.213.5 replaces pitmib implementation in v3.0.205.1

// command members
int BriefCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);
int mxpadShowHideCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);
int toggleMapCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);
int toggleCueCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);
int toggleDesignerModeCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);
int autoHideMxpadCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);
int toggleCoiceWindow_CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);
int toggleTargetMarkerSetting_CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon); // v3.0.253.9
int hideTargetMarkerSetting_CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);   // v3.0.255.4.1
int showTargetMarkerSetting_CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);   // v3.0.255.4.1
int reloadPlugins_CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);             // v3.305.2

int writePlaneCoordinationToLogCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);       // v3.0.213.7
int writeCameraCoordinationToLogCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon); // v3.0.213.7
int writeWeatherToLogCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon); // v3.303.13

int toggleAutoSkipInStoryModeCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon); // v3.305.3

static void prep_cache(XPLMMapLayerID layer, const float* inTotalMapBoundsLeftTopRightBottom, XPLMMapProjectionID projection, void* inRefcon);
static void draw_markings(XPLMMapLayerID layer, const float* inMapBoundsLeftTopRightBottom, float zoomRatio, float mapUnitsPerUserInterfaceUnit, XPLMMapStyle mapStyle, XPLMMapProjectionID projection, void* inRefcon);
static void will_be_deleted(XPLMMapLayerID layer, void* inRefcon);
void createMapLayer(const char* mapIdentifier, void* refcon); // v3.305.4

// v3.0.213.7 Datarefs to hold current plane location // TODO: replace with plane location already implemented.
XPLMDataRef gHeadingPsiRef = nullptr; // heading - true north

// v3.0.221.7 shared param dummy function for now. This callback is called whenever our shared data is changed. */
static void MyDataChangedCallback(void* inRefcon);
static bool setSharedDataRef(std::string inDataName, XPLMDataTypeID inDataType, XPLMDataChanged_f inNotificationFunc, double inValue = 0.0);

// v3.303.8 deprecated function
//static bool setSharedDataRef_array(std::string inDataName, XPLMDataTypeID inDataType, XPLMDataChanged_f inNotificationFunc, void *inArray, size_t inArraySize);

static int MyKeySniffer(char inChar, XPLMKeyFlags inFlags, char inVirtualKey, void* inRefcon);
}

// -----------------------------------
// Mandatory function. START
PLUGIN_API int
XPluginStart(char* outName, char* outSig, char* outDesc)
{

  XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);
  XPLMEnableFeature("XPLM_USE_NATIVE_WIDGET_WINDOWS", 1);

  curl_global_init(CURL_GLOBAL_DEFAULT);

  // v3.0.219.7
  missionx::data_manager::g_vr_dref = XPLMFindDataRef("sim/graphics/VR/enabled");
  missionx::data_manager::flc_vr_state();

  missionx::data_manager::clear();

  missionx::Mission::init();

  // data_manager::device_init(data_manager::mx_device_nk); // moved from read fonts nk. seem more suitable

  data_manager::readPluginTextures(); // v3.0.118
  // data_manager::queFlcActions.push (missionx::mx_flc_pre_command::load_briefer_textures); // v25.04.2

  std::memset(missionx::LOG_BUFF, '\0', missionx::LOG_BUFF_SIZE); // First time initialization of LOG_BUFF so it will have a concrete set of memory to work on.
  missionx::Timer t1;
  Timer::start(t1, 0, "XPluginStart Timer");


  // folder data
  Log::logMsgNone(Utils::xml_get_node_content_as_text(data_manager::mx_folders_properties.node));


  // Register Plugin
#ifndef RELEASE
#define VERSION_DATE __DATE__ " " __TIME__
#else
#define VERSION_DATE ""
#endif

  const std::string plugin_name = "Mission-X v" + std::string(PLUGIN_VER_MAJOR) + "." + std::string(PLUGIN_VER_MINOR) + "." + std::string(PLUGIN_REVISION) + " " + VERSION_DATE;
  /*const std::string plugin_sig = "missionx_snagar.dev";*/
  const std::string plugin_desc = "XML based mission with embedded scripting support (snagar.net)";

#ifdef IBM
  strncpy_s(outName, 255, plugin_name.c_str(), plugin_name.length());
  strncpy_s(outSig, 255, data_manager::plugin_sig.c_str(), data_manager::plugin_sig.length());
  strncpy_s(outDesc, 255, plugin_desc.c_str(), plugin_desc.length());
#else
  std::strncpy(outName, plugin_name.c_str(), 255);
  std::strncpy(outSig, data_manager::plugin_sig.c_str(), 255);
  std::strncpy(outDesc, plugin_desc.c_str(), 255);
#endif
  // END Register Plugin


  // v3.0.255.4.4 add crash handling based on https://developer.x-plane.com/code-sample/crash-handling/
#ifndef LIN
    register_crash_handler();
#endif

#if SUPPORT_BACKGROUND_THREADS
  s_background_thread = std::thread(&background_thread_func);
#endif

  // register callbacks
  XPLMRegisterFlightLoopCallback(pluginCallback, -1, nullptr);
  const dataref_const dc;
  missionx::data_manager::xplane_using_modern_driver_b = XPLMGetDatai(dc.dref_xplane_using_modern_driver_b);

  if (missionx::data_manager::xplane_using_modern_driver_b)
  {

    if (data_manager::xplane_ver_i <= missionx::XP12_VERSION_NO)
    {
#ifdef MAC
      XPLMRegisterDrawCallback(missionx::drawCallback_missionx, xplm_Phase_Modern3D, 0, NULL); // In this phase, we can't draw to the XPlane 3D World only 2D but maybe we could do some movement calculation
      mission.setDrawingPhase(xplm_Phase_Window); // to be on the safe side
#else
      mission.setDrawingPhase(xplm_Phase_Modern3D);
#endif
      XPLMRegisterDrawCallback(missionx::drawCallback_missionx, mission.getDrawingPhase(), 0, nullptr); // VERY EXPENSIVE PHASE (according to LR) Notes: https://developer.x-plane.com/2020/04/updated-plugin-sdk-docs-and-sample-code-for-vulkan-metal/
      //XPLMRegisterDrawCallback(missionx::drawCallback_missionx, xplm_Phase_Objects, 0, NULL); // VERY EXPENSIVE PHASE (according to LR) Notes: https://developer.x-plane.com/2020/04/updated-plugin-sdk-docs-and-sample-code-for-vulkan-metal/
      
    }
    else
    { // in XP12 xplm_Phase_Window seem the only working phase but not the other phases
      mission.setDrawingPhase(xplm_Phase_Window);
      XPLMRegisterDrawCallback(missionx::drawCallback_missionx, mission.getDrawingPhase(), 0, nullptr); // VERY EXPENSIVE PHASE (according to LR) Notes: https://developer.x-plane.com/2020/04/updated-plugin-sdk-docs-and-sample-code-for-vulkan-metal/

    }
                                                                                     // and https://developer.x-plane.com/article/plugin-compatibility-guide-for-x-plane-11-50/
  }
  else
  {
    mission.setDrawingPhase(xplm_Phase_Objects);
    XPLMRegisterDrawCallback(missionx::drawCallback_missionx, mission.getDrawingPhase(), 0, nullptr);
  }


  //// Create UI Windows
  // first set compatibility UI

  // v3.0.251.1
  // uiImGuiOptions creation: mission::flcQue() "display_choice_window"
  // uiImGuiMxpad: created at plugin start
  if (missionx::Mission::uiImGuiMxpad == nullptr)
  {
    int left, top, right, bottom;
    Utils::getWinCoords(left, top, right, bottom); // 410, 170
                                                   // Position widget relative to windows borders
    left   = right - missionx::WinImguiMxpad::WINDOWS_WIDTH - 20;
    right  = left + missionx::WinImguiMxpad::WINDOWS_WIDTH;
    top    = top - 60;
    bottom = top - missionx::WinImguiMxpad::WINDOWS_MAX_HEIGHT;

    Mission::uiImGuiMxpad = std::make_shared<WinImguiMxpad>(left, top, right, bottom, xplm_WindowDecorationSelfDecoratedResizable, xplm_WindowLayerFloatingWindows); // special decorations

  }

  if (missionx::Mission::uiImGuiBriefer == nullptr)
  {
    int left, top, right, bottom;
    Utils::getWinCoords(left, top, right, bottom); // 1200, 450
    // Position widget at the center of screen
    left   = (int)(((right - left) / 2) - (missionx::WinImguiBriefer::WINDOW_MAX_WIDTH / 2));
    right  = left + missionx::WinImguiBriefer::WINDOW_MAX_WIDTH;
    top    = (int)(((top - bottom) / 2) + (missionx::WinImguiBriefer::WINDOWS_MAX_HEIGHT / 2));
    bottom = top - missionx::WinImguiBriefer::WINDOWS_MAX_HEIGHT;

    Mission::uiImGuiBriefer = std::make_shared<WinImguiBriefer>(left, top, right, bottom); // decoration and layer will use default values
    Mission::uiImGuiBriefer->SetWindowPositioningMode(xplm_WindowCenterOnMonitor);

    missionx::mission.initImguiParametersAtPluginsStart(); // v3.0.253.7 rc2 initialize setup according to setup file
  }


  // v3.0.241.10 b2 deprecated code, since we have the choice window
  //// create mxSelecRef int dataref // pitmib v3.0.205.1

  // COMMANDS
  // Create and Register our custom command
  // 1. in command string name - ad hock 2. function handler.   3. Receive input before plugin windows., 4. inRefcon <- I think when using C++ function we should do something like the following: "MXCKeySniffer * me =
  // reinterpret_cast<MXCKeySniffer *>(inRefCon);"
  mapMxCommands["BriefCommand"] = XPLMCreateCommand("missionx/general/missionx-toggle_briefer_window", "Toggle Mission-X Window.");
  XPLMRegisterCommandHandler(mapMxCommands["BriefCommand"], BriefCommandHandler, 1, (void*)nullptr);
  mapMxCommands["mxpadShowHideCommand"] = XPLMCreateCommand("missionx/general/missionx-toggle_mxpad_window", "Toggle Mission-X MXPAD Window.");
  XPLMRegisterCommandHandler(mapMxCommands["mxpadShowHideCommand"], mxpadShowHideCommandHandler, 1, (void*)nullptr);
  mapMxCommands["mapCommand"] = XPLMCreateCommand("missionx/general/missionx-toggle_map", "Toggle Mission-X mission Map");
  XPLMRegisterCommandHandler(mapMxCommands["mapCommand"], toggleMapCommandHandler, 1, (void*)nullptr);
  mapMxCommands["autoHideMxpadCommand"] = XPLMCreateCommand("missionx/general/missionx_auto_hide_show_mxpad", "Auto Hide/Show MX-Pad"); // v3.0.215.1
  XPLMRegisterCommandHandler(mapMxCommands["autoHideMxpadCommand"], autoHideMxpadCommandHandler, 1, (void*)nullptr);                          // v3.0.215.1


  mapMxCommands["toggleCoiceWindow_CommandHandler"] = XPLMCreateCommand("missionx/general/missionx-toggle_choice", "Toggle choice window (if available)"); // v3.0.231.1
  XPLMRegisterCommandHandler(mapMxCommands["toggleCoiceWindow_CommandHandler"], toggleCoiceWindow_CommandHandler, 1, (void*)nullptr);                            // v3.0.231.1

  // Designer helper commands
  mapMxCommands["toggleDesignerModeCommand"] = XPLMCreateCommand("missionx/designer/missionx-toggle_Designer_Mode", "Toggle Mission-X Designer Mode"); // v3.0.215.1 only changed the command location in tree node to "designer" from "general"
  XPLMRegisterCommandHandler(mapMxCommands["toggleDesignerModeCommand"], toggleDesignerModeCommandHandler, 1, (void*)nullptr);
  mapMxCommands["toggleCueCommand"] = XPLMCreateCommand("missionx/designer/missionx-toggle_Cue_Info", "Toggle Mission-X Visual Cue Info"); // v3.0.215.1 only changed the command location in tree node to "designer" from "general"
  XPLMRegisterCommandHandler(mapMxCommands["toggleCueCommand"], toggleCueCommandHandler, 1, (void*)nullptr);
  mapMxCommands["write_plane_coordiante"] = XPLMCreateCommand("missionx/designer/write-plane-cooridinate-to-log", "Write Plane Coordinates to Log file");     // v3.0.213.7
  XPLMRegisterCommandHandler(mapMxCommands["write_plane_coordiante"], writePlaneCoordinationToLogCommandHandler, 1, (void*)nullptr);                                                 // write to Log command
  mapMxCommands["write_camera_coordiante"] = XPLMCreateCommand("missionx/designer/write-cammera-cooridinate-to-log", "Write Camera Coordinates to Log file"); // v3.0.219.3
  XPLMRegisterCommandHandler(mapMxCommands["write_camera_coordiante"], writeCameraCoordinationToLogCommandHandler, 1, (void*)nullptr);                                          // write Camera data to Log command
  // v3.303.13 weather helper command
  mapMxCommands["write_weather_data"] = XPLMCreateCommand("missionx/designer/write-wether-data-to-log", "Write Weather data to Log file"); // v3.303.13
  XPLMRegisterCommandHandler(mapMxCommands["write_weather_data"], writeWeatherToLogCommandHandler, 1, (void*)nullptr); // v3.303.13 // write Weather data to Log command
  // v3.305.3
  mapMxCommands["toggleAutoSkipInStoryMode_CommandHandler"] = XPLMCreateCommand("missionx/designer/missionx-auto_skip", "Toggle Auto Skip In Story Mode");
  XPLMRegisterCommandHandler(mapMxCommands["toggleAutoSkipInStoryMode_CommandHandler"], toggleAutoSkipInStoryModeCommandHandler, 1, (void*)nullptr);



  // v3.0.253.9
  mapMxCommands["toggleTargetMarker_CommandHandler"] = XPLMCreateCommand("missionx/setup/missionx-toggle_target-marker", "Toggle target markers for generated mission"); // v3.0.253.9
  XPLMRegisterCommandHandler(mapMxCommands["toggleTargetMarker_CommandHandler"], toggleTargetMarkerSetting_CommandHandler, 1, (void*)nullptr);                                 // v3.0.253.9

  // v3.0.255.4.1
  mapMxCommands["hideTargetMarkerSetting_CommandHandler"] = XPLMCreateCommand("missionx/setup/missionx-hide_target-marker", "Hide target markers for generated mission"); // v3.0.255.4.1
  XPLMRegisterCommandHandler(mapMxCommands["hideTargetMarkerSetting_CommandHandler"], hideTargetMarkerSetting_CommandHandler, 1, (void*)nullptr);                               // v3.0.255.4.1
  mapMxCommands["showTargetMarkerSetting_CommandHandler"] = XPLMCreateCommand("missionx/setup/missionx-show_target-marker", "Show target markers for generated mission"); // v3.0.255.4.1
  XPLMRegisterCommandHandler(mapMxCommands["showTargetMarkerSetting_CommandHandler"], showTargetMarkerSetting_CommandHandler, 1, (void*)nullptr);                               // v3.0.255.4.1


  // v3.305.2
  mapMxCommands["reloadPlugins_CommandHandler"] = XPLMCreateCommand("missionx/internal/missionx-reload_plugins", "Reload Plugins");
  XPLMRegisterCommandHandler(mapMxCommands["reloadPlugins_CommandHandler"], reloadPlugins_CommandHandler, 1, (void*)nullptr);




  ///// Create Menu //////
  int missionSubMenuItem   = XPLMAppendMenuItem(XPLMFindPluginsMenu(), outName, (void*)Mission::mx_menuIdRefs::MISSIONX_MENU_ENTRY, 1);
  mission.missionMenuEntry = XPLMCreateMenu(outName, XPLMFindPluginsMenu(), missionSubMenuItem, &missionx::MissionMenuHandler, nullptr);

  //// Generic menu
  mission.mx_menu.newBrieferMenu = XPLMAppendMenuItem(mission.missionMenuEntry, "Toggle Mission-X Window", (void*)Mission::mx_menuIdRefs::MENU_TOGGLE_MISSIONX_BRIEFER, 1); // 
  mission.mx_menu.newBrieferMenu = XPLMAppendMenuItem(mission.missionMenuEntry, "Reset Mission-X Window Position", (void*)Mission::mx_menuIdRefs::MENU_RESET_BRIEFER_POSITION, 1); // v3.305.1
  XPLMAppendMenuSeparator(mission.missionMenuEntry);                                                                                                                      // v3.0.0
  mission.mx_menu.mxpadMenu = XPLMAppendMenuItemWithCommand(mission.missionMenuEntry, "Toggle MX-Pad", XPLMFindCommand("missionx/general/missionx-toggle_mxpad_window")); // v3.0.215.1 replaced sub menu with command menu
  XPLMAppendMenuSeparator(mission.missionMenuEntry);


  ///// Option Menu and its sub menus v3.0.215.15rc3.4 /////
  mission.mx_menu.toolsMenu     = XPLMAppendMenuItem(mission.missionMenuEntry, "Tools Menu", (void*)Mission::mx_menuIdRefs::MENU_DUMMY_TOOLS_PLACE_HOLDER, 1);         // v3.0.215.15rc3.4
  mission.missionMenuToolsEntry = XPLMCreateMenu("Tools", mission.missionMenuEntry, mission.mx_menu.toolsMenu, &missionx::toolsMenuHandler, (void*)Mission::mx_menuIdRefs::MENU_TOOLS); // v3.0.215.15rc3.4
  // tools sub menus
  mission.mx_menu.toolsCreateExternalFPLN_fromGPS        = XPLMAppendMenuItem(mission.missionMenuToolsEntry, "Create External FPLN based on GPS (fpln_folders.ini)", (void*)Mission::mx_menuIdRefs::MENU_TOOLS_CREATE_EXTERNAL_FPLN_BASED_ON_GPS, 1);          // v3.0.255.4.4
  mission.mx_menu.toolsModifyPointFileBaseOnTerrainProbe = XPLMAppendMenuItem(mission.missionMenuToolsEntry, "Modify points.xml file (check designer guide)", (void*)Mission::mx_menuIdRefs::MENU_TOOLS_UPDATE_POINT_IN_FILE_WITH_TEMPLATE_BASED_ON_PROBE, 1); // v3.0.221.15rc3.4

  // Reload pluigins / internal
  XPLMAppendMenuSeparator(mission.missionMenuToolsEntry);
  mission.mx_menu.reloadPluginsMenu = XPLMAppendMenuItemWithCommand(mission.missionMenuToolsEntry, "Reload Plugins", XPLMFindCommand("missionx/internal/missionx-reload_plugins")); // v3.305.2

  XPLMAppendMenuSeparator(mission.missionMenuToolsEntry);
  mission.mx_menu.toolsWritePlanePosToLog  = XPLMAppendMenuItem(mission.missionMenuToolsEntry, "Write Plane position to Log.txt file", (void*)Mission::mx_menuIdRefs::MENU_TOOLS_WRITE_PLANE_POSITION_TO_LOG, 1);   // v3.303.14
  mission.mx_menu.toolsWriteCameraPosToLog = XPLMAppendMenuItem(mission.missionMenuToolsEntry, "Write Camera position to Log.txt file", (void*)Mission::mx_menuIdRefs::MENU_TOOLS_WRITE_CAMERA_POSITION_TO_LOG, 1); // v3.303.14
  XPLMAppendMenuSeparator(mission.missionMenuToolsEntry);
  mission.mx_menu.toolsWriteWeatherToLog = XPLMAppendMenuItem(mission.missionMenuToolsEntry, "Write weather state to Log.txt file", (void*)Mission::mx_menuIdRefs::MENU_TOOLS_WRITE_WEATHER_STATE_TO_LOG, 1); // v3.303.13

  XPLMAppendMenuSeparator(mission.missionMenuOptionsEntry);


  ///// Option Menu and its sub menus v3.0.215.1 /////
  mission.mx_menu.optionsMenu     = XPLMAppendMenuItem(mission.missionMenuEntry, "Options Menu", (void*)Mission::mx_menuIdRefs::MENU_DUMMY_OPTIONS_PLACE_HOLDER, 1);             // v3.0.215.1
  mission.missionMenuOptionsEntry = XPLMCreateMenu("Options", mission.missionMenuEntry, mission.mx_menu.optionsMenu, &missionx::OptionsMenuHandler, (void*)Mission::mx_menuIdRefs::MENU_OPTIONS); // v3.0.215.1
  // options sub menus
  mission.mx_menu.optionAutoHideMxPad = XPLMAppendMenuItemWithCommand(mission.missionMenuOptionsEntry, "Auto Hide/Show MX-Pad", XPLMFindCommand("missionx/general/missionx_auto_hide_show_mxpad")); // v3.0.215.1


  XPLMAppendMenuSeparator(mission.missionMenuOptionsEntry);
  mission.mx_menu.optionDisablePluginColdAndDark = XPLMAppendMenuItem(mission.missionMenuOptionsEntry, "Disable Plugin Cold and Dark at mission start", (void*)Mission::mx_menuIdRefs::MENU_OPTION_DISABLE_PLUGIN_COLD_AND_DARK, 1); // v3.0.221.6 Allow auto pause when in VR mode

  //if (missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO)
  {
    XPLMAppendMenuSeparator(mission.missionMenuOptionsEntry);
    mission.mx_menu.optionToggleDesignerMode = XPLMAppendMenuItemWithCommand(mission.missionMenuOptionsEntry, "Toggle Designer Mode", XPLMFindCommand("missionx/designer/missionx-toggle_Designer_Mode")); // v3.0.211.2
    mission.mx_menu.optionToggleCueInfo      = XPLMAppendMenuItemWithCommand(mission.missionMenuOptionsEntry, "Toggle CueInfo", XPLMFindCommand("missionx/designer/missionx-toggle_Cue_Info"));            // v3.0.211.2
  }
  XPLMAppendMenuSeparator(mission.missionMenuEntry); // v3.0.219.12

#ifdef IBM
  mission.mx_menu.optimizeAptDatMenu = XPLMAppendMenuItem(mission.missionMenuEntry, "APT.DAT optimization (run in background: 1-2min)", (void*)Mission::mx_menuIdRefs::MENU_APT_DAT_OPTIMIZATION, 1); // v3.0.219.2 Apt.dat optimization
#else
  mission.mx_menu.optimizeAptDatMenu = XPLMAppendMenuItem(mission.missionMenuEntry, "APT.DAT optimization (run in background: 15-60 sec)", (void*)Mission::mx_menuIdRefs::MENU_APT_DAT_OPTIMIZATION, 1); // v3.0.231.1 Linux and MAC  are much faster
#endif
  XPLMAppendMenuSeparator(mission.missionMenuEntry); // v3.0.217.1



  // v3.0.221.7 register shared params

  std::string dref_name;

  dref_name = "xpshared/target/lat"; // holds target latitude
  setSharedDataRef(dref_name, xplmType_Float, MyDataChangedCallback, 0.0);

  dref_name = "xpshared/target/lon"; // holds target longitude
  setSharedDataRef(dref_name, xplmType_Float, MyDataChangedCallback, 0.0);

  dref_name = "xpshared/target/type"; // holds expected action type: 0: not set, 1: patient, 2: delivery
  setSharedDataRef(dref_name, xplmType_Int, MyDataChangedCallback, 0.0);

  dref_name = "xpshared/target/need_to_hover"; // Do we need to hover ? 0=land 1=hover or 0=false 1=true
  setSharedDataRef(dref_name, xplmType_Int, MyDataChangedCallback, 0.0);

  dref_name = "xpshared/target/status"; // holds result from other plugin.  -1: failed, 0: not set, waiting, 1: success
  setSharedDataRef(dref_name, xplmType_Int, MyDataChangedCallback, 0.0);

  dref_name = "xpshared/target/listen_plugin_available"; // Does a plugin listen or going to use theee datarefs and update the status ?  0=no one listen  1=someone is listening
  setSharedDataRef(dref_name, xplmType_Int, MyDataChangedCallback, 0.0);

  //dref_name = "xpshared/target/target_lat_arr"; // 
  //setSharedDataRef_array(dref_name, xplmType_FloatArray, MyDataChangedCallback, missionx::data_manager::mx_dref_target_lat_arr, 16 );

  dref_name = "missionx/obj3d/rotation_per_frame_1_min"; // holds keyframe information for 1 min per frame
  setSharedDataRef(dref_name, xplmType_Float, MyDataChangedCallback, 0.0);


  // setup check marks
  missionx::mission.syncOptionsWithMenu(); // v3.0.215.1

  /* Register key sniffer as per https://developer.x-plane.com/code-sample/keysniffer/ */
  XPLMRegisterKeySniffer(missionx::MyKeySniffer, /* handling function callback. */
                         1,                      /* Receive input before plugin windows. */
                         nullptr);                     /* Refcon - not used. */


  // quick reference to write plane coordinate to Log.txt
  //gPlanePsiRef = XPLMFindDataRef("sim/flightmodel2/position/true_psi"); // v3.0.301 no need we get it from dataref_manager
  gHeadingPsiRef = XPLMFindDataRef("sim/graphics/view/view_heading"); // v3.0.253.5 or sim/graphics/view/pilots_head_psi


  if (missionx::data_manager::init_xp_airport_db()) // v3.0.255.3 The airport database is a key component in the Random Mission builder. We need to establish validity when plugin starts
  {
    missionx::data_manager::sqlite_test_db_validity(missionx::data_manager::db_xp_airports); // v3.0.255.3
  }

  // 24.12.2 Seed the std::rnd with current time
  auto now = std::chrono::system_clock::now();
  std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
  std::srand(static_cast<unsigned>(std::time(&currentTime)));


  Timer::wasEnded(t1);
  //  Log::logMsgNone("\n");
  Log::logMsgNone(Timer::to_string(t1));

  Log::logXPLMDebugString(">>>>>>>>>>>> END Loading Mission-X <<<<<<<<<<<<\n");

  return 1;
}

// -----------------------------------
// Mandatory Function STOP
PLUGIN_API void
XPluginStop(void)
{
  // Clean up our map layer: if we created it, we should be good citizens and destroy it before the plugin is unloaded
  if (missionx::data_manager::g_layer)
  {
    // Triggers the will-be-deleted callback of the layer, causing g_layer to get set back to NULL
    XPLMDestroyMapLayer(missionx::data_manager::g_layer);
  }

  try
  {

    missionx::QueueMessageManager::sound.release();
    missionx::mission.stop_plugin(); // v3.0.149


    if (missionx::Mission::uiImGuiMxpad)
      XPLMDestroyWindow(missionx::Mission::uiImGuiMxpad->mWindow); // v3.0.190
    if (missionx::Mission::uiImGuiOptions)
      XPLMDestroyWindow(missionx::Mission::uiImGuiOptions->mWindow); // v3.0.190



    // v3.0.251.0
    // Cleanup the general stuff
    if (missionx::Mission::uiImGuiMxpad)
    {
      missionx::WinImguiMxpad::sFont1.reset();
      missionx::Mission::uiImGuiMxpad = nullptr;
    }

    // v3.0.221.7 release shared data
    for (auto& [drefName, drefObject] : missionx::data_manager::mapSharedParams)
    {
      if (XPLMUnshareData(drefName.c_str(), drefObject.getDataRefType(), MyDataChangedCallback, nullptr))
      {
#ifndef RELEASE
        Log::logMsgNone("DataRef: " + drefName + " has been unshared.");
#endif
      }
    }

    missionx::data_manager::pluginStop(); // clear all mission vecTextures in data_manager + font atlas + GL device

    // destroy instances
    for (auto& [objName, obj3dInstance] : missionx::data_manager::map3dInstances)
    {
      if (obj3dInstance.g_instance_ref)
      {
        XPLMDestroyInstance(obj3dInstance.g_instance_ref);
        XPLMDebugString((std::string("missionx: Destroyed Instance: ") + objName + mxconst::get_UNIX_EOL()).c_str()); // debug
      }
    }
    // unload 3d objects from map3dInstance
    for (auto& obj3dInstance : missionx::data_manager::map3dInstances | std::views::values) // v24.12.2 CLion, convert to view:values since only values are being used from the <pair>
    {
      if (obj3dInstance.g_object_ref)
      {
        XPLMUnloadObject(obj3dInstance.g_object_ref);
        XPLMDebugString((std::string("missionx: Unload 3D file: ") + obj3dInstance.getName() + mxconst::get_UNIX_EOL()).c_str()); // debug
      }
    }

    XPLMDebugString("\nClearing Logs"); // debug
    // abort Log writeMessage
    missionx::Log::stop_mission(); // v3.0.217.8

    XPLMDebugString("\nmissionx: Release static"); // debug
    // release curl from data_manager
    missionx::data_manager::release_static(); // v3.0.253.1

    XPLMDebugString("\nmissionx: Release RandomEngine::threadState.flagIsActive"); // debug
    if (RandomEngine::threadState.flagIsActive)
    {
      XPLMDebugString("\nmissionx: sleep 1 sec - wait for Random");
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (missionx::data_manager::threadStateMetar.flagIsActive)
    {
      missionx::data_manager::threadStateMetar.flagAbortThread = true;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  

    XPLMDebugString("\nmissionx: Close all databases"); // debug
    missionx::data_manager::db_close_all_databases(); // v3.0.241.10

    curl_global_cleanup();

    // v3.0.255.4.4 LR crash handling
#if SUPPORT_BACKGROUND_THREADS
    // Tell the background thread to shut down and then wait for it
    s_background_thread_shutdown.store(true, std::memory_order_release);
    s_background_thread.join();
#endif

    XPLMDebugString("\nmissionx: Plug-in stopped");

#ifndef LIN
    unregister_crash_handler();
#endif
  }

  catch (const std::exception& e)
  {
    const std::string s = std::string("[Mission-X plugin stopped]") + e.what();
    XPLMDebugString(s.c_str());
  }
}

// -----------------------------------
// Mandatory Function ENABLE
PLUGIN_API int
XPluginEnable(void)
{
  Log::logMsg("Plug-in Enabling\n");
  // register callbacks
  XPLMRegisterFlightLoopCallback(pluginCallback, -1, nullptr);
  if (missionx::data_manager::xplane_using_modern_driver_b)
  {
    XPLMRegisterDrawCallback(missionx::drawCallback_missionx, mission.getDrawingPhase(), 0, nullptr); // for XP12
  }
  else
  {
    XPLMRegisterDrawCallback(missionx::drawCallback_missionx, xplm_Phase_Objects, 0, nullptr); // xplane v1141xxx and lower or v1151 + opengl
  }

// We want to create our layer in the standard map used in the UI (not other maps like the IOS).
  // If the map already exists in X-Plane (i.e., if the user has opened it), we can create our layer immediately.
  // Otherwise, though, we need to wait for the map to be created, and only *then* can we create our layers.
  if (XPLMMapExists(XPLM_MAP_USER_INTERFACE))
  {
    createMapLayer(XPLM_MAP_USER_INTERFACE, nullptr);
  }
  // Listen for any new map objects that get created
  XPLMRegisterMapCreationHook(&createMapLayer, nullptr);

  return 1; // important so callback will continue
}

// -----------------------------------
// Mandatory Function DISABLE
PLUGIN_API void
XPluginDisable(void)
{
  // debug
  Log::logMsg("[Mission-X] Plug-in Disabling");
  // unregister callbacks
  XPLMUnregisterFlightLoopCallback(pluginCallback, nullptr);
  XPLMUnregisterDrawCallback(missionx::drawCallback_missionx, xplm_Phase_Objects, 0, nullptr);
  XPLMUnregisterDrawCallback(missionx::drawCallback_missionx, xplm_Phase_Window, 0, nullptr);
  XPLMUnregisterDrawCallback(missionx::drawCallback_missionx, xplm_Phase_Modern3D, 0, nullptr); // v3.0.241.5 for xplane v1151xx + vulkan
  XPLMUnregisterDrawCallback(missionx::drawCallback_missionx, mission.getDrawingPhase(), 0, nullptr); // v3.303.8.3

  // abort Log writeMessage
  missionx::Log::stop_mission(); // v3.0.217.8


  // debug
  Log::logMsg("[Mission-X] Plug-in Disabled");
}

// -----------------------------------
PLUGIN_API void
XPluginReceiveMessage(XPLMPluginID inFrom, const intptr_t inMsg, void* inParam)
{
#ifdef DEBUG

  // Log::logMsg ("[PluginMsg]Message Sent: " + Utils:: );
#endif

  switch (inMsg)
  {
    case XPLM_MSG_AIRPORT_LOADED:
    case XPLM_MSG_SCENERY_LOADED:
    {
      missionx::data_manager::strct_sceneryOrPlaneLoadState.set_flagSceneryOrAirportLoaded(true); // v3.305.1c
      missionx::data_manager::refresh_3d_objects_and_cues_after_location_transition();
      missionx::Mission::uiImGuiBriefer->strct_ext_layer.from_icao.clear();
    }
    break;
    case XPLM_MSG_PLANE_CRASHED: // abort only is mission is active and if auto abort on crash flag is true
    {

      if (data_manager::missionState == mx_mission_state_enum::mission_is_running)
      {
        const std::string msg = "Plane crashed, aborting mission.";
        data_manager::setAbortMission(msg);
        missionx::Log::logMsg("[XPluginReceiveMessage] " + msg); // debug
      }
    }
    break;
    case XPLM_MSG_ENTERED_VR:
    {
      missionx::mxvr::flag_in_vr = true;
      // v3.0.219.7
      missionx::mxvr::vr_display_missionx_in_vr_mode = true; // XPLMGetDatai(g_vr_dref);

      const auto val = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISPLAY_MISSIONX_IN_VR(), mxconst::DEFAULT_DISPLAY_MISSIONX_IN_VR); // override VR state. enable/disable missionx in VR
      if (!val)
        missionx::mxvr::vr_display_missionx_in_vr_mode = false;

    }
    break;
    case XPLM_MSG_EXITING_VR:
    {
      missionx::mxvr::flag_in_vr    = false;
      missionx::mxvr::vr_display_missionx_in_vr_mode = false;
    }
    break;
    case XPLM_MSG_PLANE_LOADED:
    {
// #ifndef RELEASE
//       missionx::data_manager::gather_acf_cargo_data(data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()]);
// #endif

      // // v24.12.2
      // if (data_manager::missionState == mx_mission_state_enum::mission_is_running)
      // {
      //   // TODO: decide if we need to re-initialize
      //   // missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i = missionx::data_manager::get_inv_layout_based_on_mission_ver_and_compatibility_node(missionx::data_manager::mission_file_supported_versions, missionx::data_manager::mx_global_settings.xCompatibility_ptr, data_manager::flag_setupUseXP11InventoryUI); // v25.03.1
      //   missionx::Inventory::gather_acf_cargo_data(data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()], true);
      //   data_manager::dref_acf_station_max_kgs_f_arr.setAndInitializeKey("sim/aircraft/weight/acf_m_station_max");
      //   missionx::dataref_param::set_dataref_values_into_xplane(data_manager::dref_m_stations_kgs_f_arr ); // force original weight on the new plane
      // }

      // missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::gather_acf_custom_datarefs); // v3.303.13 make sure we will have the plane dataref information
    }
    break;
    default:
      break;
  } // switch
}

// -----------------------------------

namespace missionx
{
void
MissionMenuHandler(void* inMenuRef, void* inItemRef)
{
  // Main Menu Dispatcher that calls the function that creates each Widget

  switch (static_cast<Mission::mx_menuIdRefs>(reinterpret_cast<intptr_t>(inItemRef)))
  {
    case Mission::mx_menuIdRefs::MENU_TOGGLE_MISSIONX_BRIEFER:
    {
#ifndef RELEASE
      missionx::Log::logMsg("[Missionx] Menu toggle briefer.");
#endif
      BriefCommandHandler(nullptr, xplm_CommandBegin, nullptr);
    }
    break;
    case Mission::mx_menuIdRefs::MENU_RESET_BRIEFER_POSITION:
    {
#ifndef RELEASE
      missionx::Log::logMsg("[Missionx] Reset Briefer Position to Center.");
#endif
      if (missionx::Mission::uiImGuiBriefer)
        missionx::Mission::uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_RESET_BRIEFER_POSITION);
    }
    break;
    case Mission::mx_menuIdRefs::MENU_TOGGLE_MXPAD:
    {
#ifndef RELEASE
      missionx::Log::logMsg("[Missionx] Menu toggle mxpad.");
#endif
      mxpadShowHideCommandHandler(nullptr, xplm_CommandBegin, nullptr);
    }
    break;
    case Mission::mx_menuIdRefs::MENU_TOGGLE_MAP:
    {
#ifndef RELEASE
      missionx::Log::logMsg("[Missionx] Menu toggle mission map.");
#endif
      toggleMapCommandHandler(nullptr, xplm_CommandBegin, nullptr);
    }
    break;
    case Mission::mx_menuIdRefs::MENU_APT_DAT_OPTIMIZATION:
    {
#ifndef RELEASE
      missionx::Log::logMsg("[Missionx] Menu Optimize \"apt.dat\" files (should take 3-8min, depends on machine, runs in the background).");
#endif
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::exec_apt_dat_optimization); // v3.0.253.6
    }
    break;
    case Mission::mx_menuIdRefs::MENU_TOGGLE_CHOICE_WINDOW:
    {
#ifndef RELEASE
      missionx::Log::logMsg("[Missionx] Toggle Choice window.");
#endif
      toggleCoiceWindow_CommandHandler(nullptr, xplm_CommandBegin, nullptr);
    }
    break;

    default:
      break;

  }
}

// -----------------------------------

void
OptionsMenuHandler(void* inMenuRef, void* inItemRef)
{
  // Main Menu Dispatcher that calls the function that creates each Widget

  switch (static_cast<Mission::mx_menuIdRefs>((intptr_t)inItemRef))
  {
    case Mission::mx_menuIdRefs::MENU_OPTION_DISPLAY_MISSIONX_IN_VR:
    {
#ifndef RELEASE
      missionx::Log::logMsg("[Missionx] Menu Toggle if Mission-X will be displayed in VR mode");
#endif
      /// toggle state of MENU_OPTION_DISPLAY_MISSIONX_IN_VR

      const auto val = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node,
                                                         mxconst::get_OPT_DISPLAY_MISSIONX_IN_VR(),
                                                         mxconst::DEFAULT_DISPLAY_MISSIONX_IN_VR); //  missionx::system_actions::pluginSetupOptions.getPropertyValue<int>(mxconst::get_OPT_DISPLAY_MISSIONX_IN_VR(), data_manager::errStr);

      missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_DISPLAY_MISSIONX_IN_VR(), !val);
      mission.syncOptionsWithMenu();
      missionx::system_actions::store_plugin_options();
    }
    break;
    case Mission::mx_menuIdRefs::MENU_OPTION_DISABLE_PLUGIN_COLD_AND_DARK:
    {
#ifndef RELEASE
      missionx::Log::logMsg("[Missionx] Menu Toggle disable plugin cold and dark at mission start");
#endif
      /// toggle state of MENU_OPTION_DISABLE_PLUGIN_COLD_AND_DARK

      const auto val = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISABLE_PLUGIN_COLD_AND_DARK_WORKAROUND(), mxconst::DEFAULT_DISABLE_PLUGIN_COLD_AND_DARK);

      missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_DISABLE_PLUGIN_COLD_AND_DARK_WORKAROUND(), !val);
      mission.syncOptionsWithMenu();
      missionx::system_actions::store_plugin_options();
    }
    break;
    default:
      break;
  }
}

// -----------------------------------

void
toolsMenuHandler(void* inMenuRef, void* inItemRef)
{
  switch ((Mission::mx_menuIdRefs)((intptr_t)inItemRef))
  {
    case Mission::mx_menuIdRefs::MENU_TOOLS_CREATE_EXTERNAL_FPLN_BASED_ON_GPS:
    {
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::write_fpln_to_external_folder); // v3.0.255.4.4
    }
    break;
    case Mission::mx_menuIdRefs::MENU_TOOLS_UPDATE_POINT_IN_FILE_WITH_TEMPLATE_BASED_ON_PROBE:
    {
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::update_point_in_file_with_template_based_on_probe); // v3.0.255.4.4
    }
    break;
    case Mission::mx_menuIdRefs::MENU_TOOLS_WRITE_PLANE_POSITION_TO_LOG: // v3.303.14
    {
      missionx::data_manager::write_plane_position_to_log_file(); 
    }
    break;
    case Mission::mx_menuIdRefs::MENU_TOOLS_WRITE_CAMERA_POSITION_TO_LOG: // v3.303.14
    {
      missionx::data_manager::write_camera_position_to_log_file(); 
    }
    break;
    case Mission::mx_menuIdRefs::MENU_TOOLS_WRITE_WEATHER_STATE_TO_LOG: // v3.303.13
    {
      missionx::data_manager::write_weather_state_to_log_file(); 
    }
    break;
    //case Mission::mx_menuIdRefs::MENU_RELOAD_PLUGINS: // v3.305.2
    //{
    //  XPLMReloadPlugins();
    //}
    //break;
    default:
      break;

  } // end case
}

// -----------------------------------

float
pluginCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
//#ifdef TIMER_FUNC
//  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
//#endif // TIMER_FUNC

#ifdef PLUGIN_EVALUATE_PERFORMANCE
  // Chrono
  auto start = chrono::steady_clock::now();
#endif

  missionx::mission.flc();

#ifdef PLUGIN_EVALUATE_PERFORMANCE
  auto end          = chrono::steady_clock::now();
  auto diff         = end - start;
  callback_duration = chrono::duration<double, milli>(diff).count();
  Log::logAttention("pluginCallback Duration: " + Utils::formatNumber<double>(callback_duration, 3) + "ms (" + Utils::formatNumber<double>((callback_duration / 1000), 3) + "sec)  ");
  Log::logAttention("drawCallback_missionx Duration: ~" + Utils::formatNumber<double>(drawcallback_duration, 3) + "ms (~" + Utils::formatNumber<double>((drawcallback_duration / 1000), 3) + "sec)  ");
#endif

#ifdef TIMER_FUNC
  Log::logMsg("--");
#endif

  return 1.0f;
}

// -----------------------------------

int
drawCallback_missionx(XPLMDrawingPhase inPhase, int inIsBefore, void* inRefcon)
{
//#ifdef TIMER_FUNC
//  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
//#endif // TIMER_FUNC

#ifdef PLUGIN_EVALUATE_PERFORMANCE
  // Chrono
  auto start = chrono::steady_clock::now();
#endif
  // for each draw callback phase we should call different draw function
  missionx::mission.drawCallback(inPhase, inIsBefore, inRefcon);
#ifdef PLUGIN_EVALUATE_PERFORMANCE
  auto end              = chrono::steady_clock::now();
  auto diff             = end - start;
  drawcallback_duration = chrono::duration<double, milli>(diff).count();
  // Log::logAttention("drawCallback_missionx Duration: " + Utils::formatNumber<double>(drawcallback_duration, 3) + "ms (" + Utils::formatNumber<double>((drawcallback_duration / 1000), 3) + "sec)  ");
#endif
  return 1;
}

// -----------------------------------

int
BriefCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{

  if (inPhase == xplm_CommandBegin)
  {
    // reload mission list if mission is in undefined state and briefer window is closed
    if (data_manager::missionState == missionx::mx_mission_state_enum::mission_undefined && (missionx::Mission::uiImGuiBriefer->GetVisible() == false))
    {
      missionx::mission.prepareUiMissionList();
    }

    // toggle briefer windows state
    missionx::Mission::uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_TOGGLE_BRIEFER);
  }

  // Log::logMsg("[Rares debug] plugin: after BriefCommandHandler"); // debug - rares

  return 0;
}



int
mxpadShowHideCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
  if (inPhase == xplm_CommandBegin)
  {
    if (data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
    {
      // v3.0.251.1
      if (missionx::Mission::uiImGuiMxpad)
      {
        missionx::Mission::uiImGuiMxpad->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
        // We will reset auto hide option if and only if it was active at least once and the "auto hide" option is set
        if (missionx::Mission::uiImGuiMxpad->getWasHiddenByAutoHideOption() &&
            Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node,
                                              mxconst::get_OPT_AUTO_HIDE_SHOW_MXPAD(),
                                              false))    // meaning Auto Hide Option is set && if user setup was also set to true (auto hide mxpad)          
        {
          missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_AUTO_HIDE_SHOW_MXPAD(), false); // cancel auto hide, since user asked to "force" toggle manually
                                                                                                          // //missionx::system_actions::pluginSetupOptions.setIntProperty(mxconst::get_OPT_AUTO_HIDE_SHOW_MXPAD(), (int)0); // no
          missionx::Mission::uiImGuiMxpad->setWasHiddenByAutoHideOption(false);                           // reset its state since simmer changed the status manually
          missionx::mission.syncOptionsWithMenu();                                                        // flag correctly menu options
        }
      }
    }
  }

  return 0;
}


int
toggleMapCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
  if (inPhase == xplm_CommandBegin)
  {

    if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
    {
      missionx::Mission::uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_TOGGLE_MAP);
    }
  }

  return 0;
}


int
toggleCueCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
  if (inPhase == xplm_CommandBegin)
  {

    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::toggle_cue_info_mode);

  }

  return 0;
}


int
toggleDesignerModeCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{

  if (inPhase == xplm_CommandBegin)
  {

    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::toggle_designer_mode);

  }

  return 0;
}


int
autoHideMxpadCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
  if (inPhase == xplm_CommandBegin)
  {

    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::toggle_auto_hide_show_mxpad_option);

  }

  return 0;
}


int
toggleCoiceWindow_CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
  if (inPhase == xplm_CommandEnd)
  {
    if (!missionx::mxvr::vr_display_missionx_in_vr_mode && data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
    {
      if (missionx::data_manager::mxChoice.is_choice_set() && Mission::uiImGuiOptions) // display window if all is set
      {
        missionx::Mission::uiImGuiOptions->execAction(missionx::mx_window_actions::ACTION_TOGGLE_CHOICE_WINDOW); // v3.0.251.1 use imgui window
      }
      else if (!missionx::data_manager::mxChoice.last_choice_name_s.empty() && Mission::uiImGuiOptions) // if mxChoice is not set but we have its last choice saved, then try to prepare Choice window out of it
      {
        if (missionx::data_manager::prepare_choice_options(missionx::data_manager::mxChoice.last_choice_name_s))
        {
          missionx::Mission::uiImGuiOptions->execAction(missionx::mx_window_actions::ACTION_TOGGLE_CHOICE_WINDOW); // v3.0.251.1 use imgui window
        }
        else
          missionx::data_manager::mxChoice.last_choice_name_s.clear(); // if we failed creating the last saved choice
      }
    }
  }
  return 0;
}

// -----------------------------------

int
toggleTargetMarkerSetting_CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
  if (inPhase == xplm_CommandEnd)
  {
    // we do not save the changes to the options file when toggling using the command
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::toggle_target_marker_option);
  }
  return 0;
}



// -----------------------------------
int
hideTargetMarkerSetting_CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
  if (inPhase == xplm_CommandBegin)
  {
    // we do not save the changes to the options file when toggling using the command
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::hide_target_marker_option);
  }
  return 0;
}



// -----------------------------------
int
showTargetMarkerSetting_CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
  if (inPhase == xplm_CommandBegin)
  {
    // we do not save the changes to the options file when toggling using the command
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::show_target_marker_option);
  }
  return 0;
}

// -----------------------------------

int
reloadPlugins_CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
  if (inPhase == xplm_CommandBegin)
  {
    XPLMReloadPlugins();
  }

  return 0;
}



// -----------------------------------


int
writePlaneCoordinationToLogCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{

  if (inPhase == xplm_CommandBegin)
  {

    missionx::data_manager::write_plane_position_to_log_file();

    return 1;
  }


  return 0;
}

int
writeCameraCoordinationToLogCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{

  if (inPhase == xplm_CommandBegin)
  {

    missionx::data_manager::write_camera_position_to_log_file();

    return 1;
  }

  return 0;
}


int
writeWeatherToLogCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{

  if (inPhase == xplm_CommandBegin)
  {
    missionx::data_manager::write_weather_state_to_log_file();
    return 1;
  }
  return 0;
}


int
toggleAutoSkipInStoryModeCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{

  if (inPhase == xplm_CommandBegin)
  {
    missionx::data_manager::flag_setupAutoSkipStoryMessage ^= 1;
    return 1;
  }
  return 0;
}


void
prep_cache(XPLMMapLayerID layer, const float* inTotalMapBoundsLeftTopRightBottom, XPLMMapProjectionID projection, void* inRefcon)
{
  // We're simply going to cache the locations, in *map* coordinates, of all the places we want to draw.
  data_manager::s_num_cached_coords = 0;
  for (int lon = -180; lon < 180; ++lon)
  {
    for (int lat = -90; lat < 90; ++lat)
    {
      float       x, y;
      const float offset = 0.0f;  //0.25; // to avoid drawing on grid lines
      XPLMMapProject(projection, lat, lon, &x, &y);
      if (mxUtils::coord_in_rect(x, y, inTotalMapBoundsLeftTopRightBottom))
      {
        data_manager::s_cached_x_coords[data_manager::s_num_cached_coords] = x;
        data_manager::s_cached_y_coords[data_manager::s_num_cached_coords] = y;
        data_manager::s_cached_lon_coords[data_manager::s_num_cached_coords] = lon + offset;
        data_manager::s_cached_lat_coords[data_manager::s_num_cached_coords] = lat + offset;
        ++data_manager::s_num_cached_coords;
      }
    }
  }

  // Because the map uses true cartographical projections, the size of 1 meter in map units can change
  // depending on where you are asking about. We'll ask about the midpoint of the available bounds
  // and assume the answer won't change too terribly much over the size of the maps shown in the UI.
  const float midpoint_x = (inTotalMapBoundsLeftTopRightBottom[0] + inTotalMapBoundsLeftTopRightBottom[2]) / 2;
  const float midpoint_y = (inTotalMapBoundsLeftTopRightBottom[1] + inTotalMapBoundsLeftTopRightBottom[3]) / 2;
  // We'll draw our icons to be 5000 meters wide in the map
  data_manager::s_icon_width = XPLMMapScaleMeter(projection, midpoint_x, midpoint_y) * 5000;
}

void
draw_markings(XPLMMapLayerID layer, const float* inMapBoundsLeftTopRightBottom, float zoomRatio, float mapUnitsPerUserInterfaceUnit, XPLMMapStyle mapStyle, XPLMMapProjectionID projection, void* inRefcon)
{
  mission.drawMarkings(layer, inMapBoundsLeftTopRightBottom, zoomRatio, mapUnitsPerUserInterfaceUnit, mapStyle, projection, inRefcon);
}

void
will_be_deleted(XPLMMapLayerID layer, void* inRefcon)
{
  if (layer == missionx::data_manager::g_layer)
    missionx::data_manager::g_layer = nullptr;
}

void
createMapLayer(const char* mapIdentifier, void* refcon)
{
  if (!missionx::data_manager::g_layer &&    // Confirm we haven't created our markings layer yet (e.g., as a result of a previous callback), or if we did, it's been destroyed
      !strcmp(mapIdentifier, XPLM_MAP_USER_INTERFACE)) // we only want to create a layer in the normal user interface map (not the IOS)
  {
    XPLMCreateMapLayer_t params;
    params.structSize            = sizeof(XPLMCreateMapLayer_t);
    params.mapToCreateLayerIn    = XPLM_MAP_USER_INTERFACE;
    params.willBeDeletedCallback = &will_be_deleted;
    params.prepCacheCallback     = &prep_cache;
    params.showUiToggle          = 1;
    params.refcon                = nullptr;
    params.layerType             = xplm_MapLayer_Markings;
    params.drawCallback          = &draw_markings;
    params.iconCallback          = nullptr, // &draw_marking_icons;
    params.labelCallback         = nullptr;
    params.layerName             = "Mission-X Markings";
    // Note: this could fail (return NULL) if we hadn't already confirmed that params.mapToCreateLayerIn exists in X-Plane already
    missionx::data_manager::g_layer = XPLMCreateMapLayer(&params);
  }
}

/*
 * This is the callback for our shared data.  Right now we do not react
 * to our shared data being changed.
 *
 */

void
MyDataChangedCallback(void* inRefcon)
{
}


bool
setSharedDataRef(std::string inDataName, XPLMDataTypeID inDataType, XPLMDataChanged_f inNotificationFunc, double inValue)
{
  missionx::dataref_param dref;
  XPLMDataChanged_f       notificationFunc = MyDataChangedCallback;
  if (!(inNotificationFunc == nullptr))
    notificationFunc = inNotificationFunc;

  if (inDataName.empty())
    return false;

  int status = XPLMShareData(inDataName.c_str(), inDataType, notificationFunc, nullptr);
  if (status)
  {
    dref.setName(inDataName);
    dref.setAndInitializeKey(inDataName); // will initialize the dataref param with reference to memory
    if (dref.flag_paramReadyToBeUsed)
    {
      // initialize parameter
      dref.setValue(inValue);
      missionx::dataref_param::set_dataref_values_into_xplane(dref);

      // add to shared params in data_manager
      Utils::addElementToMap(missionx::data_manager::mapSharedParams, inDataName, dref);
      XPLMDebugString(std::string("missionx: Added shared dataref: " + inDataName + "\t " + dref.to_string()).c_str());
    }
    else
    {
      XPLMDebugString(std::string("missionx: Failed adding shared dataref: " + inDataName + "\t [" + dref.errReason + "]\n").c_str());
      return false;
    }
  }
  else
  {
    XPLMDebugString(std::string("missionx: Failed adding shared dataref: " + inDataName + ". Dataref with same name or different datatype might already exists.\n").c_str());

    return false;
  }

  return true;
}



/*
 * MyKeySniffer
 *
 * This routnine receives keystrokes from the simulator as they are pressed.
 * A separate message is received for each key press and release as well as
 * keys being held down.
 *
 */
int
MyKeySniffer(char inChar, XPLMKeyFlags inFlags, char inVirtualKey, void* inRefcon)
{
  /* First record the key data. */
  /* Take the last key stroke and form a descriptive string.
   * Note that ASCII values may be printed directly.  Virtual key
   * codes are not ASCII and cannot be, but the utility function
   * XPLMGetVirtualKeyDescription provides a human-readable string
   * for each key.  These strings may be multicharacter, e.g. 'ENTER'
   * or 'NUMPAD-0'. */
  // sprintf(str, "%d '%c' | %d '%s' (%c %c %c %c %c)",
  //  gChar,
  //  (gChar) ? gChar : '0',
  //  (int)(unsigned char)gVirtualKey,
  //  XPLMGetVirtualKeyDescription(gVirtualKey),
  //  (gFlags & xplm_ShiftFlag) ? 'S' : ' ',
  //  (gFlags & xplm_OptionAltFlag) ? 'A' : ' ',
  //  (gFlags & xplm_ControlFlag) ? 'C' : ' ',
  //  (gFlags & xplm_DownFlag) ? 'D' : ' ',
  //  (gFlags & xplm_UpFlag) ? 'U' : ' ');

  data_manager::mx_global_key_sniffer.keySnifferVirtualKey = inVirtualKey;
  data_manager::mx_global_key_sniffer.keySnifferFlags      = inFlags;
  data_manager::mx_global_key_sniffer.keySnifferChar       = inChar;


  switch (inVirtualKey)
  {
    case XPLM_VK_ESCAPE:
    {

      if (missionx::mission.uiImGuiBriefer && missionx::mission.uiImGuiBriefer->GetVisible() && !(missionx::mission.uiImGuiBriefer->IsInVR()))                                           // if briefer is visible and not in VR
      {
        switch (missionx::mission.uiImGuiBriefer->getCurrentLayer())
        {
          // we deliberately and explicitly list each briefer layer for fine grained control
          case missionx::uiLayer_enum::imgui_home_layer: // v3.0.253.1
          case missionx::uiLayer_enum::option_user_generates_a_mission_layer:
          case missionx::uiLayer_enum::option_generate_mission_from_a_template_layer:
          case missionx::uiLayer_enum::option_mission_list:
          case missionx::uiLayer_enum::option_setup_layer:
          case missionx::uiLayer_enum::option_external_fpln_layer:
          case missionx::uiLayer_enum::flight_leg_info:
          case missionx::uiLayer_enum::flight_leg_info_end_summary:
          case missionx::uiLayer_enum::flight_leg_info_inventory:
          case missionx::uiLayer_enum::flight_leg_info_map2d:
          {
            missionx::mission.uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
            return 0; // consume the action
          }
          break;
          default:
            break; // do nothing

        } // end layer handling
      } // end if window is visible and not in VR

    }
    break;
    default:
      break;
  } // end key handling

  /* Return 1 to pass the keystroke to plugin windows and X-Plane.
   * Returning 0 would consume the keystroke. */
  return 1;
}

// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------

}
