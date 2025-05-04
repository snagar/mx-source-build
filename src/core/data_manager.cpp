#include "../io/ListDir.h"


#include <filesystem>
namespace fs = std::filesystem;

#include <nlohmann/json.hpp>

#include "data_manager.h"

#include <array>
#include <cstdio>
// #include <forward_list> // v3.305.3
#include <fstream>
#include <iostream>
#include <list>
#include <unordered_set>

#include "../../libs/polyline-cpp/src/SimplePolyline.h" // v3.303.14
#include "../core/dataref_manager.h"                    // v3.305.2 moved from header
#include "../io/OptimizeAptDat.h"                       // v3.303.14
#include "../io/system_actions.h"                       // v3.0.241.1
#include "../ui/core/BitmapReader.h"                    // v3.303.14

#include "coordinate/NavAidInfo.hpp"
// #include "../core/coordinate/UtilsGraph.hpp"



namespace missionx
{
// missionx missionx_conf file node
IXMLNode data_manager::xMissionxPropertiesNode;

const std::string              data_manager::plugin_sig = "missionx_snagar.dev";
std::vector<std::future<void>> mImageLoadFutures; // we use future<void> since out function returns void.

script_manager data_manager::sm;
#ifdef APL
mb_interpreter_t* data_manager::bas;
bool              data_manager::ext_sm_init_success;
bool              data_manager::ext_bas_open;
#endif

int data_manager::xplane_ver_i;                 // v3.0.241.5
int data_manager::xplane_using_modern_driver_b; // v3.0.241.5

int         data_manager::msgSeq;
std::string data_manager::selectedMissionKey; // v3.0.241.1

// initialize map
std::map<std::string, Waypoint>  data_manager::mapFlightLegs;
std::map<std::string, Objective> data_manager::mapObjectives;
std::map<std::string, Trigger>   data_manager::mapTriggers;
std::map<std::string, Inventory> data_manager::mapInventories;     // v3.0.213.1
Inventory                        data_manager::planeInventoryCopy; // v24.12.2
// std::map<std::string, missionx::Item>      data_manager::mapItemBlueprints;  // v3.0.213.1
IXMLNode                                   data_manager::xmlBlueprints;      // v3.0.241.1
// IXMLNode                                   data_manager::externalInventoryCopy;               // v3.0.241.1 external inventory copy
// IXMLNode                                   data_manager::xmlPlaneInventoryCopy;          // v3.0.241.1 plane inventory copy
Inventory data_manager::externalInventoryCopy;       // v3.0.241.1 external inventory copy
//missionx::Inventory data_manager::xmlPlaneInventoryCopy;          // v3.0.241.1 plane inventory copy
std::string         data_manager::active_external_inventory_name; // v3.0.251.1

std::map<std::string, Message> data_manager::mapMessages;

// xml related
IXMLNode     data_manager::xMainNode;
IXMLRenderer data_manager::xmlRender;
IXMLNode     data_manager::xmlMappingNode;
IXMLNode     data_manager::xmlGPS;
IXMLNode     data_manager::xmlLoadedFMS;


// 3d objects
std::map<std::string, obj3d> data_manager::map3dObj;                     // v3.0.200
std::map<std::string, obj3d> data_manager::map3dInstances;               // v3.0.200 //[name, obj3d instance] // Holds static and moving 3D Objects. We keep different list of 3D Objects in listDisplayStatic3dInstances and listDisplayMoving3dInstances
std::set<std::string>                  data_manager::listDisplayStatic3dInstances; // v3.0.200 //name, // Holds static 3D Objects
std::set<std::string>                  data_manager::listDisplayMoving3dInstances; // v3.0.202 //name, // Holds moving 3D Objects

std::map<std::string, dataref_param> data_manager::mapDref;

mx_base_node data_manager::mx_folders_properties;
std::string            data_manager::errStr;
// std::string            data_manager::loadErrors;
std::list<std::string> data_manager::lstLoadErrors; // v3.305.3
mx_base_node data_manager::xLoadInfoNode; // v3.305.3


// std::multimap<std::string, std::multimap<std::string, std::multimap<std::string, std::string>>> data_manager::mmLoadInfo; // v3.305.3


Briefer                            data_manager::briefer;
std::map<std::string, BrieferInfo> data_manager::mapBrieferMissionList;
std::map<int, std::string>                   data_manager::mapBrieferMissionListLocator;

std::map<std::string, std::string>   data_manager::mapCurrentMissionTexturePathLocator; // {unique filename, path to file}   during mission load store all Legs/global_settings/end status images
std::map<std::string, mxTextureFile> data_manager::mapCurrentMissionTextures;           // <unique filename, image data> once pressed start, we need to load all images. Use "mapCurrentMissionTexturePathLocator"

mx_base_node data_manager::endMissionElement;

std::string data_manager::missionSavepointFilePath;
std::string data_manager::missionSavepointDrefFilePath;

mxProperties data_manager::smPropSeedValues;

std::queue<std::string>                  data_manager::queThreadMessage;
std::queue<mx_flc_pre_command> data_manager::queFlcActions;
std::queue<mx_flc_pre_command> data_manager::postFlcActions; // v3.0.146

std::map<std::string, mxTextureFile>    data_manager::mapCachedPluginTextures;                // v3.0.118 // plugin specific vecTextures
std::map<std::string, TemplateFileInfo> data_manager::mapGenerateMissionTemplateFiles;        // Generate mission vecTextures // v3.0.217.2
std::map<int, std::string>                        data_manager::mapGenerateMissionTemplateFilesLocator; // Generate mission vecTextures // v3.0.217.2
TemplateFileInfo                        data_manager::user_driven_template_info;              // v3.0.241.9 store user driven bare bone template to build on
int                                               data_manager::seqElementId{ 0 };

// std::map<std::string, MxFontFile> data_manager::mapFonts; // v3.0.231.1 freetype

// Global Settings
GLobalSettings data_manager::mx_global_settings; // v3.303.8.2


//// Commands
std::map<std::string, BindCommand> data_manager::mapCommands;          // v3.0.221.9
std::map<std::string, BindCommand> data_manager::mapCommandsWithTimer; // v3.0.221.15
std::deque<std::string>                      data_manager::queCommands;          // v3.0.221.9
std::deque<std::string>                      data_manager::queCommandsWithTimer; // v3.0.221.15

//// Key Sniffer
data_manager::mx_keySniffer data_manager::mx_global_key_sniffer;
XPLMKeyFlags                data_manager::_mx_keySniffer::keySnifferFlags;
char                        data_manager::_mx_keySniffer::keySnifferChar;
char                        data_manager::_mx_keySniffer::keySnifferVirtualKey;

mx_mission_state_enum data_manager::missionState;
std::string           data_manager::currentLegName;


// CueInfo
int data_manager::seqCueInfo{ 0 }; // v3.0.202a

// FailureTimers
std::map<std::string, Timer> data_manager::mapFailureTimers;  // v3.0.253.7
std::string                            data_manager::lowestFailTimerName_s; // v3.0.253.7
std::string                            data_manager::formated_fail_timer_as_text; // v3.0.253.7

mx_success_timer_info data_manager::strct_success_timer_info; // v25.02.1

std::map<std::string, mx_aptdat_cached_info> data_manager::cachedNavInfo_map;

bool data_manager::flag_apt_dat_optimization_is_running{ false };
// Generate Mission Engine Thread flag
bool data_manager::flag_generate_engine_is_running;

// VR Related
XPLMDataRef data_manager::g_vr_dref;
int         data_manager::prev_vr_is_enabled_state{ 0 };

// TimeLapse
TimeLapse data_manager::timelapse;

// Shared Params
std::map<std::string, dataref_param> data_manager::mapSharedParams; // v3.0.221.7

float data_manager::current_plane_payload_weight_f{ 0.0f }; // v3.0.221.9

std::string data_manager::start_cold_and_dark_drefs;

// Metar
std::string data_manager::current_metar_file_s;
std::string data_manager::metar_file_to_inject_s;


// TimeLapse class
dataref_const TimeLapse::dc;

// Draw Scripts
std::string data_manager::draw_script;

// Choices
Choice data_manager::mxChoice;
IXMLNode         data_manager::xmlChoices;

// 2d maps textures
std::map<int, mxTextureFile> data_manager::maps2d_to_display;


// RealityXP
XPLMPluginID data_manager::mx_plugin_id;

/// User defined UI
mx_base_node data_manager::prop_userDefinedMission_ui;                  // v3.0.241.9
int                    data_manager::strct_flight_leg_info_totalMapsCounter{ 0 }; // v3.303.13

// SQLite
dbase data_manager::db_xp_airports; // v3.0.241.10
dbase data_manager::db_cache;       // v3.0.255.3
dbase data_manager::db_stats;       // v3.0.255.1

mx_mission_stats_strct data_manager::mission_stats_from_query;

uiLayer_enum data_manager::generate_from_layer;

std::string                                    data_manager::load_error_message;      // v3.0.251.1
int                                            data_manager::iMissionImageCounter;    // v3.0.251.1
std::map<std::string, mxTextureFile> data_manager::xp_mapMissionIconImages; // v3.0.251.1
std::map<std::string, mxTextureFile> data_manager::xp_mapInvImages;         // v3.0.303.5

//// External Flight plans
std::vector<mx_ext_internet_fpln_strct>     data_manager::tableExternalFPLN_simbrief_vec; // v25.03.3
std::vector<mx_ext_internet_fpln_strct>     data_manager::tableExternalFPLN_vec;
std::map<int, mx_ext_internet_fpln_strct *> data_manager::indexPointer_forExternalFPLN_tableVector;
std::vector<std::future<void>>              data_manager::mFetchFutures; //  holds async pointers
IXMLNode                                    data_manager::jsonConvertedNode_xml;

// ILS Query Filtering
std::vector<mx_ils_airport_row_strct>    data_manager::table_ILS_rows_vec;
std::map<int, mx_ils_airport_row_strct*> data_manager::indexPointer_for_ILS_rows_tableVector; // index pointer to the vector table_ILS_rows_vec

mutex data_manager::s_thread_sync_mutex;   // defined here and not in the class header
mutex data_manager::s_ImageLoadMutex;      // v3.303.14 Used in loadInventoryImages(), also solves non-POD when defined in namespace level
mutex data_manager::s_StoryImageLoadMutex; // v3.305.1 Used in loadStoryImages()
mutex data_manager::mt_SortTriggersMutex;  // v3.305.2 Used in sort

int                      data_manager::overpass_counter_i;                                      // v3.0.255.4.1
std::vector<std::string> data_manager::vecOverpassUrls;                                         // v3.0.255.4.1
int                      data_manager::overpass_user_picked_combo_i;                            // v3.0.255.4.1
int                      data_manager::overpass_last_url_indx_used_i{ mxconst::INT_UNDEFINED }; // v3.0.255.4.1
                                                                                                          // #ifdef USE_CURL
CURL*       data_manager::curl = nullptr;
std::string data_manager::curl_result_s{ "" };
std::string data_manager::overpass_fetch_err{ "" }; // v3.0.255.4

float data_manager::Max_Slope_To_Land_On{ mxconst::DEFAULT_MAX_SLOPE_TO_LAND_ON };

bool data_manager::flag_found_missing_3D_object_files{ false }; // v3.0.255.3 used in UI
bool data_manager::flag_rebuild_apt_dat{ false };               // v3.0.255.3 help decide if to rebuild the apt.dat. Basically calling: missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::exec_apt_dat_optimization)

std::map<std::string, std::string>                          data_manager::row_gather_db_data; // v3.0.255.3
std::unordered_map<int, std::map<std::string, std::string>> data_manager::reasultTable;       // v3.0.255.3

// v3.0.301
std::map<int, mx_local_fpln_strct> data_manager::map_tableOfParsedFpln;

// [opt-in] debug options
bool data_manager::flag_setupChangeHeadingEvenIfPlaneIn_20meter_radius{ false };
bool data_manager::flag_setupForcePlanePositioningAtMissionStart{ false };
bool data_manager::flag_setupAutoSkipStoryMessage{ false }; // v3.305.3
bool data_manager::flag_finished_load_inventory_images{ false };
bool data_manager::flag_setupUseXP11InventoryUI{ false }; // v24.12.2
// int  missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i {0}; // v24.12.2
#ifdef RELEASE
bool missionx::data_manager::flag_setupEnableDesignerMode{ false }; // v24.03.2
bool missionx::data_manager::flag_setupShowDebugTabDuringMission{ false };
bool missionx::data_manager::flag_setupSkipMissionAbortIfScriptIsInvalid{ false };

bool missionx::data_manager::flag_setupDisplayAutoSkipInStoryMessage{ false }; // v3.305.3 Hide the checkbox option
bool missionx::data_manager::flag_setupShowDebugMessageTab{ false };           // v3.305.4 Hide Message tab

#else

bool data_manager::flag_setupEnableDesignerMode{ true }; // v24.03.2
bool data_manager::flag_setupShowDebugTabDuringMission{ true };
bool data_manager::flag_setupSkipMissionAbortIfScriptIsInvalid{ true };

bool data_manager::flag_setupDisplayAutoSkipInStoryMessage{ true }; // v3.305.3 Display the checkbox option
bool data_manager::flag_setupShowDebugMessageTab{ true };           // v3.305.4 Show message tab
#endif



XPLMCameraPosition_t data_manager::mxCameraPosition;
int                  data_manager::isLosingControl_i{ 1 };
bool                 data_manager::flag_gather_acf_info_thread_is_running{ false };
bool                 data_manager::flag_abort_gather_acf_info_thread{ false };
int                  data_manager::iGatherAcfTryCounter{ 0 }; // v3.303.13

// v3.303.12
std::map<int, std::unordered_map<std::string, std::string>> data_manager::mapWeatherPreDefinedStrct_xp11 = { { 0, // clear
                                                                                                                         { { "sim/weather/barometer_current_inhg", "29.89" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[0]", "3048" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[1]", "5486" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[2]", "7924" },
                                                                                                                           { "sim/weather/cloud_coverage[0]", "0." },
                                                                                                                           { "sim/weather/cloud_coverage[1]", "0." },
                                                                                                                           { "sim/weather/cloud_coverage[2]", "0." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[0]", "3657." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[1]", "6096." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[2]", "8534." },
                                                                                                                           { "sim/weather/cloud_type[0]", "0" },
                                                                                                                           { "sim/weather/cloud_type[1]", "0" },
                                                                                                                           { "sim/weather/cloud_type[2]", "0" },
                                                                                                                           { "sim/weather/dewpoi_sealevel_c", "-15." },
                                                                                                                           { "sim/weather/download_real_weather", "0" },
                                                                                                                           { "sim/weather/has_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/precipitation_on_aircraft_ratio", "0" },
                                                                                                                           { "sim/weather/rain_percent", "0." },
                                                                                                                           { "sim/weather/relative_humidity_sealevel_percent", "9.801" },
                                                                                                                           { "sim/weather/rho", "1.224" },
                                                                                                                           { "sim/weather/runway_friction", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[0]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[1]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[2]", "0" },
                                                                                                                           { "sim/weather/shear_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[2]", "0." },
                                                                                                                           { "sim/weather/sigma", "0.99" },
                                                                                                                           { "sim/weather/speed_sound_ms", "340.23" },
                                                                                                                           { "sim/weather/temperatures_ambient_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_le_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_sealevel_c", "15." },
                                                                                                                           { "sim/weather/thermal_altitude_msl_m", "5000." },
                                                                                                                           { "sim/weather/thunderstorm_percent", "0." },
                                                                                                                           { "sim/weather/turbulence[0]", "0." },
                                                                                                                           { "sim/weather/turbulence[1]", "0." },
                                                                                                                           { "sim/weather/turbulence[2]", "0." },
                                                                                                                           { "sim/weather/use_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/visibility_reported_m", "40233.6" },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[0]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[1]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[2]", "15240." },
                                                                                                                           { "sim/weather/wind_direction_degt", "0." },
                                                                                                                           { "sim/weather/wind_direction_degt[0]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[1]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[2]", "7.3165" },
                                                                                                                           { "sim/weather/wind_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[2]", "0." } } },
                                                                                                                       { 1, // Cirrus
                                                                                                                         { { "sim/weather/barometer_current_inhg", "29.89" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[0]", "3048." },
                                                                                                                           { "sim/weather/cloud_base_msl_m[1]", "6096." },
                                                                                                                           { "sim/weather/cloud_base_msl_m[2]", "10668." },
                                                                                                                           { "sim/weather/cloud_coverage[0]", "0." },
                                                                                                                           { "sim/weather/cloud_coverage[1]", "1." },
                                                                                                                           { "sim/weather/cloud_coverage[2]", "1." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[0]", "3657." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[1]", "6705." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[2]", "11277." },
                                                                                                                           { "sim/weather/cloud_type[0]", "0" },
                                                                                                                           { "sim/weather/cloud_type[1]", "1" },
                                                                                                                           { "sim/weather/cloud_type[2]", "1" },
                                                                                                                           { "sim/weather/dewpoi_sealevel_c", "-15." },
                                                                                                                           { "sim/weather/download_real_weather", "0" },
                                                                                                                           { "sim/weather/has_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/precipitation_on_aircraft_ratio", "0" },
                                                                                                                           { "sim/weather/rain_percent", "0." },
                                                                                                                           { "sim/weather/relative_humidity_sealevel_percent", "9.801" },
                                                                                                                           { "sim/weather/rho", "1.224" },
                                                                                                                           { "sim/weather/runway_friction", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[0]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[1]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[2]", "0" },
                                                                                                                           { "sim/weather/shear_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[2]", "0." },
                                                                                                                           { "sim/weather/sigma", "0.99" },
                                                                                                                           { "sim/weather/speed_sound_ms", "340.23" },
                                                                                                                           { "sim/weather/temperatures_ambient_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_le_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_sealevel_c", "15." },
                                                                                                                           { "sim/weather/thermal_altitude_msl_m", "5000." },
                                                                                                                           { "sim/weather/thunderstorm_percent", "0." },
                                                                                                                           { "sim/weather/turbulence[0]", "0." },
                                                                                                                           { "sim/weather/turbulence[1]", "0." },
                                                                                                                           { "sim/weather/turbulence[2]", "0." },
                                                                                                                           { "sim/weather/use_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/visibility_reported_m", "40233.6" },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[0]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[1]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[2]", "15240." },
                                                                                                                           { "sim/weather/wind_direction_degt", "0." },
                                                                                                                           { "sim/weather/wind_direction_degt[0]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[1]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[2]", "7.3165" },
                                                                                                                           { "sim/weather/wind_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[2]", "0." } } },
                                                                                                                       { 2, // Scattered
                                                                                                                         { { "sim/weather/barometer_current_inhg", "29.89" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[0]", "1351." },
                                                                                                                           { "sim/weather/cloud_base_msl_m[1]", "5486." },
                                                                                                                           { "sim/weather/cloud_base_msl_m[2]", "7924." },
                                                                                                                           { "sim/weather/cloud_coverage[0]", "3." },
                                                                                                                           { "sim/weather/cloud_coverage[1]", "0." },
                                                                                                                           { "sim/weather/cloud_coverage[2]", "0." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[0]", "1961." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[1]", "6096." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[2]", "8534." },
                                                                                                                           { "sim/weather/cloud_type[0]", "2" },
                                                                                                                           { "sim/weather/cloud_type[1]", "0" },
                                                                                                                           { "sim/weather/cloud_type[2]", "0" },
                                                                                                                           { "sim/weather/dewpoi_sealevel_c", "-15." },
                                                                                                                           { "sim/weather/download_real_weather", "0" },
                                                                                                                           { "sim/weather/has_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/precipitation_on_aircraft_ratio", "0" },
                                                                                                                           { "sim/weather/rain_percent", "0." },
                                                                                                                           { "sim/weather/relative_humidity_sealevel_percent", "9.801" },
                                                                                                                           { "sim/weather/rho", "1.224" },
                                                                                                                           { "sim/weather/runway_friction", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[0]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[1]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[2]", "0" },
                                                                                                                           { "sim/weather/shear_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[2]", "0." },
                                                                                                                           { "sim/weather/sigma", "0.99" },
                                                                                                                           { "sim/weather/speed_sound_ms", "340.23" },
                                                                                                                           { "sim/weather/temperatures_ambient_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_le_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_sealevel_c", "15." },
                                                                                                                           { "sim/weather/thermal_altitude_msl_m", "5000." },
                                                                                                                           { "sim/weather/thunderstorm_percent", "0." },
                                                                                                                           { "sim/weather/turbulence[0]", "0." },
                                                                                                                           { "sim/weather/turbulence[1]", "0." },
                                                                                                                           { "sim/weather/turbulence[2]", "0." },
                                                                                                                           { "sim/weather/use_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/visibility_reported_m", "40233.6" },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[0]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[1]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[2]", "15240." },
                                                                                                                           { "sim/weather/wind_direction_degt", "0." },
                                                                                                                           { "sim/weather/wind_direction_degt[0]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[1]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[2]", "7.3165" },
                                                                                                                           { "sim/weather/wind_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[2]", "0." } } },
                                                                                                                       { 3, // Broken
                                                                                                                         { { "sim/weather/barometer_current_inhg", "29.89" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[0]", "894." },
                                                                                                                           { "sim/weather/cloud_base_msl_m[1]", "5486." },
                                                                                                                           { "sim/weather/cloud_base_msl_m[2]", "7924." },
                                                                                                                           { "sim/weather/cloud_coverage[0]", "4." },
                                                                                                                           { "sim/weather/cloud_coverage[1]", "0." },
                                                                                                                           { "sim/weather/cloud_coverage[2]", "0." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[0]", "1503.5" },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[1]", "6096." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[2]", "8534." },
                                                                                                                           { "sim/weather/cloud_type[0]", "3" },
                                                                                                                           { "sim/weather/cloud_type[1]", "0" },
                                                                                                                           { "sim/weather/cloud_type[2]", "0" },
                                                                                                                           { "sim/weather/dewpoi_sealevel_c", "9.189" },
                                                                                                                           { "sim/weather/download_real_weather", "0" },
                                                                                                                           { "sim/weather/has_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/precipitation_on_aircraft_ratio", "0" },
                                                                                                                           { "sim/weather/rain_percent", "0." },
                                                                                                                           { "sim/weather/relative_humidity_sealevel_percent", "67.915" },
                                                                                                                           { "sim/weather/rho", "1.224" },
                                                                                                                           { "sim/weather/runway_friction", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[0]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[1]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[2]", "0" },
                                                                                                                           { "sim/weather/shear_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[2]", "0." },
                                                                                                                           { "sim/weather/sigma", "0.99" },
                                                                                                                           { "sim/weather/speed_sound_ms", "340.23" },
                                                                                                                           { "sim/weather/temperatures_ambient_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_le_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_sealevel_c", "15." },
                                                                                                                           { "sim/weather/thermal_altitude_msl_m", "5000." },
                                                                                                                           { "sim/weather/thunderstorm_percent", "0." },
                                                                                                                           { "sim/weather/turbulence[0]", "0." },
                                                                                                                           { "sim/weather/turbulence[1]", "0." },
                                                                                                                           { "sim/weather/turbulence[2]", "0." },
                                                                                                                           { "sim/weather/use_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/visibility_reported_m", "32186.87" },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[0]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[1]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[2]", "15240." },
                                                                                                                           { "sim/weather/wind_direction_degt", "0." },
                                                                                                                           { "sim/weather/wind_direction_degt[0]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[1]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[2]", "7.3165" },
                                                                                                                           { "sim/weather/wind_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[2]", "0." } } },
                                                                                                                       { 4, // Overcast
                                                                                                                         { { "sim/weather/barometer_current_inhg", "29.89" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[0]", "742." },
                                                                                                                           { "sim/weather/cloud_base_msl_m[1]", "5486." },
                                                                                                                           { "sim/weather/cloud_base_msl_m[2]", "7924." },
                                                                                                                           { "sim/weather/cloud_coverage[0]", "4." },
                                                                                                                           { "sim/weather/cloud_coverage[1]", "0." },
                                                                                                                           { "sim/weather/cloud_coverage[2]", "0." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[0]", "1351.1" },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[1]", "6096." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[2]", "8534." },
                                                                                                                           { "sim/weather/cloud_type[0]", "4" },
                                                                                                                           { "sim/weather/cloud_type[1]", "0" },
                                                                                                                           { "sim/weather/cloud_type[2]", "0" },
                                                                                                                           { "sim/weather/dewpoi_sealevel_c", "10.18" },
                                                                                                                           { "sim/weather/download_real_weather", "0" },
                                                                                                                           { "sim/weather/has_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/precipitation_on_aircraft_ratio", "0" },
                                                                                                                           { "sim/weather/rain_percent", "0." },
                                                                                                                           { "sim/weather/relative_humidity_sealevel_percent", "72.65" },
                                                                                                                           { "sim/weather/rho", "1.224" },
                                                                                                                           { "sim/weather/runway_friction", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[0]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[1]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[2]", "0" },
                                                                                                                           { "sim/weather/shear_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[2]", "0." },
                                                                                                                           { "sim/weather/sigma", "0.99" },
                                                                                                                           { "sim/weather/speed_sound_ms", "340.23" },
                                                                                                                           { "sim/weather/temperatures_ambient_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_le_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_sealevel_c", "15." },
                                                                                                                           { "sim/weather/thermal_altitude_msl_m", "5000." },
                                                                                                                           { "sim/weather/thunderstorm_percent", "0." },
                                                                                                                           { "sim/weather/turbulence[0]", "0." },
                                                                                                                           { "sim/weather/turbulence[1]", "0." },
                                                                                                                           { "sim/weather/turbulence[2]", "0." },
                                                                                                                           { "sim/weather/use_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/visibility_reported_m", "24140.1" },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[0]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[1]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[2]", "15240." },
                                                                                                                           { "sim/weather/wind_direction_degt", "0." },
                                                                                                                           { "sim/weather/wind_direction_degt[0]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[1]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[2]", "7.3165" },
                                                                                                                           { "sim/weather/wind_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[2]", "0." } } },
                                                                                                                       { 5, // Low Visibility
                                                                                                                         { { "sim/weather/barometer_current_inhg", "29.89" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[0]", "436.77" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[1]", "2875.1" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[2]", "7924." },
                                                                                                                           { "sim/weather/cloud_coverage[0]", "4." },
                                                                                                                           { "sim/weather/cloud_coverage[1]", "0." },
                                                                                                                           { "sim/weather/cloud_coverage[2]", "0." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[0]", "1046.3" },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[1]", "3484.7" },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[2]", "8534." },
                                                                                                                           { "sim/weather/cloud_type[0]", "4" },
                                                                                                                           { "sim/weather/cloud_type[1]", "4" },
                                                                                                                           { "sim/weather/cloud_type[2]", "0" },
                                                                                                                           { "sim/weather/dewpoi_sealevel_c", "12.16" },
                                                                                                                           { "sim/weather/download_real_weather", "0" },
                                                                                                                           { "sim/weather/has_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/precipitation_on_aircraft_ratio", "0.025582" },
                                                                                                                           { "sim/weather/rain_percent", "0.25" },
                                                                                                                           { "sim/weather/relative_humidity_sealevel_percent", "82.988" },
                                                                                                                           { "sim/weather/rho", "1.224" },
                                                                                                                           { "sim/weather/runway_friction", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[0]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[1]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[2]", "0" },
                                                                                                                           { "sim/weather/shear_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[2]", "0." },
                                                                                                                           { "sim/weather/sigma", "0.99" },
                                                                                                                           { "sim/weather/speed_sound_ms", "340.23" },
                                                                                                                           { "sim/weather/temperatures_ambient_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_le_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_sealevel_c", "15." },
                                                                                                                           { "sim/weather/thermal_altitude_msl_m", "5000." },
                                                                                                                           { "sim/weather/thunderstorm_percent", "0." },
                                                                                                                           { "sim/weather/turbulence[0]", "0." },
                                                                                                                           { "sim/weather/turbulence[1]", "0." },
                                                                                                                           { "sim/weather/turbulence[2]", "0." },
                                                                                                                           { "sim/weather/use_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/visibility_reported_m", "8047." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[0]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[1]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[2]", "15240." },
                                                                                                                           { "sim/weather/wind_direction_degt", "0." },
                                                                                                                           { "sim/weather/wind_direction_degt[0]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[1]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[2]", "7.3165" },
                                                                                                                           { "sim/weather/wind_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[2]", "0." } } },
                                                                                                                       { 6, // Foggy
                                                                                                                         { { "sim/weather/barometer_current_inhg", "29.89" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[0]", "284.4" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[1]", "5486.4" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[2]", "7924." },
                                                                                                                           { "sim/weather/cloud_coverage[0]", "6." },
                                                                                                                           { "sim/weather/cloud_coverage[1]", "0." },
                                                                                                                           { "sim/weather/cloud_coverage[2]", "0." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[0]", "1199." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[1]", "6096." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[2]", "8534." },
                                                                                                                           { "sim/weather/cloud_type[0]", "5" },
                                                                                                                           { "sim/weather/cloud_type[1]", "0" },
                                                                                                                           { "sim/weather/cloud_type[2]", "0" },
                                                                                                                           { "sim/weather/dewpoi_sealevel_c", "13.15" },
                                                                                                                           { "sim/weather/download_real_weather", "0" },
                                                                                                                           { "sim/weather/has_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/precipitation_on_aircraft_ratio", "0." },
                                                                                                                           { "sim/weather/rain_percent", "0." },
                                                                                                                           { "sim/weather/relative_humidity_sealevel_percent", "88.614" },
                                                                                                                           { "sim/weather/rho", "1.224" },
                                                                                                                           { "sim/weather/runway_friction", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[0]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[1]", "0" },
                                                                                                                           { "sim/weather/shear_direction_degt[2]", "0" },
                                                                                                                           { "sim/weather/shear_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/shear_speed_kt[2]", "0." },
                                                                                                                           { "sim/weather/sigma", "0.99" },
                                                                                                                           { "sim/weather/speed_sound_ms", "340.23" },
                                                                                                                           { "sim/weather/temperatures_ambient_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_le_c", "14.95" },
                                                                                                                           { "sim/weather/temperatures_sealevel_c", "15." },
                                                                                                                           { "sim/weather/thermal_altitude_msl_m", "5000." },
                                                                                                                           { "sim/weather/thunderstorm_percent", "0." },
                                                                                                                           { "sim/weather/turbulence[0]", "0." },
                                                                                                                           { "sim/weather/turbulence[1]", "0." },
                                                                                                                           { "sim/weather/turbulence[2]", "0." },
                                                                                                                           { "sim/weather/use_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/visibility_reported_m", "1609." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[0]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[1]", "15240." },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[2]", "15240." },
                                                                                                                           { "sim/weather/wind_direction_degt", "0." },
                                                                                                                           { "sim/weather/wind_direction_degt[0]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[1]", "7.3165" },
                                                                                                                           { "sim/weather/wind_direction_degt[2]", "7.3165" },
                                                                                                                           { "sim/weather/wind_speed_kt[0]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[1]", "0." },
                                                                                                                           { "sim/weather/wind_speed_kt[2]", "0." } } },
                                                                                                                       { 7, // stormy
                                                                                                                         { { "sim/weather/barometer_current_inhg", "29.50" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[0]", "1046.37" },
                                                                                                                           { "sim/weather/cloud_base_msl_m[1]", "13716." },
                                                                                                                           { "sim/weather/cloud_base_msl_m[2]", "14325." },
                                                                                                                           { "sim/weather/cloud_coverage[0]", "5." },
                                                                                                                           { "sim/weather/cloud_coverage[1]", "0." },
                                                                                                                           { "sim/weather/cloud_coverage[2]", "0." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[0]", "11714." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[1]", "14325." },
                                                                                                                           { "sim/weather/cloud_tops_msl_m[2]", "14935." },
                                                                                                                           { "sim/weather/cloud_type[0]", "4" },
                                                                                                                           { "sim/weather/cloud_type[1]", "0" },
                                                                                                                           { "sim/weather/cloud_type[2]", "0" },
                                                                                                                           { "sim/weather/dewpoi_sealevel_c", "22.8" },
                                                                                                                           { "sim/weather/download_real_weather", "0" },
                                                                                                                           { "sim/weather/has_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/precipitation_on_aircraft_ratio", "0.29" },
                                                                                                                           { "sim/weather/rain_percent", "0.75" },
                                                                                                                           { "sim/weather/relative_humidity_sealevel_percent", "64.11" },
                                                                                                                           { "sim/weather/rho", "1.15" },
                                                                                                                           { "sim/weather/runway_friction", "1" },
                                                                                                                           { "sim/weather/shear_direction_degt[0]", "14.219" },
                                                                                                                           { "sim/weather/shear_direction_degt[1]", "9.9" },
                                                                                                                           { "sim/weather/shear_direction_degt[2]", "0" },
                                                                                                                           { "sim/weather/shear_speed_kt[0]", "1.32" },
                                                                                                                           { "sim/weather/shear_speed_kt[1]", "2.4" },
                                                                                                                           { "sim/weather/shear_speed_kt[2]", "0.0" },
                                                                                                                           { "sim/weather/sigma", "0.93" },
                                                                                                                           { "sim/weather/speed_sound_ms", "348.68" },
                                                                                                                           { "sim/weather/temperatures_ambient_c", "29.43" },
                                                                                                                           { "sim/weather/temperatures_le_c", "29.43" },
                                                                                                                           { "sim/weather/temperatures_sealevel_c", "30.31" },
                                                                                                                           { "sim/weather/thermal_altitude_msl_m", "5000." },
                                                                                                                           { "sim/weather/thunderstorm_percent", "0.6" },
                                                                                                                           { "sim/weather/turbulence[0]", "2." },
                                                                                                                           { "sim/weather/turbulence[1]", "2." },
                                                                                                                           { "sim/weather/turbulence[2]", "0." },
                                                                                                                           { "sim/weather/use_real_weather_bool", "0" },
                                                                                                                           { "sim/weather/visibility_reported_m", "24140.16" },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[0]", "893.9" },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[1]", "7751.9" },
                                                                                                                           { "sim/weather/wind_altitude_msl_m[2]", "15240." },
                                                                                                                           { "sim/weather/wind_direction_degt[0]", "238.9" },
                                                                                                                           { "sim/weather/wind_direction_degt[1]", "236.9" },
                                                                                                                           { "sim/weather/wind_direction_degt[2]", "236.9" },
                                                                                                                           { "sim/weather/wind_speed_kt[0]", "11.4" },
                                                                                                                           { "sim/weather/wind_speed_kt[1]", "8.11" },
                                                                                                                           { "sim/weather/wind_speed_kt[2]", "0." } } } };

std::map<int, std::unordered_map<std::string, std::string>> data_manager::mapWeatherPreDefinedStrct_xp12 = { { 0,
                                                                                                                         { { "sim/weather/region/change_mode", "3" },
                                                                                                                           { "sim/weather/region/cloud_base_msl_m", "3048.,6096.,9144." },
                                                                                                                           { "sim/weather/region/cloud_coverage_percent", "0.,0.,0." },
                                                                                                                           { "sim/weather/region/cloud_tops_msl_m", "3657.600098,6705.600098,9753.600586" },
                                                                                                                           { "sim/weather/region/cloud_type", "2.,2.,2." },
                                                                                                                           { "sim/weather/region/dewpoint_deg_c", "-9.852451,-11.631691,-13.359358,-15.271917,-17.424232,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/rain_percent", "0" },
                                                                                                                           { "sim/weather/region/runway_friction", "0" },
                                                                                                                           { "sim/weather/region/shear_direction_degt", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/shear_speed_msc", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/temperatures_aloft_deg_c", "15.,8.57597,2.340051,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/turbulence", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/visibility_reported_sm", "40." },
                                                                                                                           { "sim/weather/region/wind_altitude_msl_m", "1246.90918,2493.818359,3740.727295,4987.636719,6234.545898,7481.45459,8728.363281,9975.273438,11222.182617,12469.091797" },
                                                                                                                           { "sim/weather/region/wind_direction_degt", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/wind_speed_msc", "-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444" },
                                                                                                                           { "sim/weather/region/update_immediately", "1" },
                                                                                                                           { "sim/weather/region/variability_pct", "0." },
                                                                                                                           { "sim/weather/region/update_immediately", "1" } } },
                                                                                                                       { 1,
                                                                                                                         { { "sim/weather/region/change_mode", "3" },
                                                                                                                           { "sim/weather/region/cloud_base_msl_m", "3053.486328,6096.,9144." },
                                                                                                                           { "sim/weather/region/cloud_coverage_percent", "0.25,0.,0." },
                                                                                                                           { "sim/weather/region/cloud_tops_msl_m", "4272.686523,6705.600098,9753.600586" },
                                                                                                                           { "sim/weather/region/cloud_type", "2.,2.,2." },
                                                                                                                           { "sim/weather/region/dewpoint_deg_c", "-4.341856,-6.121095,-7.848763,-9.761321,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/rain_percent", "0" },
                                                                                                                           { "sim/weather/region/runway_friction", "1" },
                                                                                                                           { "sim/weather/region/shear_direction_degt", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/shear_speed_msc", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/temperatures_aloft_deg_c", "15.,8.57597,2.340051,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/turbulence", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/variability_pct", "0.5" },
                                                                                                                           { "sim/weather/region/visibility_reported_sm", "30." },
                                                                                                                           { "sim/weather/region/wind_altitude_msl_m", "1246.90918,2493.818359,3740.727295,4987.636719,6234.545898,7481.45459,8728.363281,9975.273438,11222.182617,12469.091797" },
                                                                                                                           { "sim/weather/region/wind_direction_degt", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/wind_speed_msc", "-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444" },
                                                                                                                           { "sim/weather/region/update_immediately", "1" } } },
                                                                                                                       { 2,
                                                                                                                         { { "sim/weather/region/change_mode", "3" },
                                                                                                                           { "sim/weather/region/cloud_base_msl_m", "2139.086426,6096.,9144." },
                                                                                                                           { "sim/weather/region/cloud_coverage_percent", "0.5,0.,0." },
                                                                                                                           { "sim/weather/region/cloud_tops_msl_m", "3663.086426,6705.600098,9753.600586" },
                                                                                                                           { "sim/weather/region/cloud_type", "2.,2.,2." },
                                                                                                                           { "sim/weather/region/dewpoint_deg_c", "-0.049025,-1.828264,-3.555932,-5.468491,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/rain_percent", "0" },
                                                                                                                           { "sim/weather/region/runway_friction", "0" },
                                                                                                                           { "sim/weather/region/shear_direction_degt", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/shear_speed_msc", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/temperatures_aloft_deg_c", "15.,8.57597,2.340051,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/turbulence", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/update_immediately", "1" },
                                                                                                                           { "sim/weather/region/variability_pct", "0.75" },
                                                                                                                           { "sim/weather/region/visibility_reported_sm", "25." },
                                                                                                                           { "sim/weather/region/wind_altitude_msl_m", "1524.,4572.,9144.,4987.636719,6234.545898,7481.45459,8728.363281,9975.273438,11222.182617,12469.091797" },
                                                                                                                           { "sim/weather/region/wind_direction_degt", "180.,225.,270.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/wind_speed_msc", "1.028889,2.057778,4.115556,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444" },
                                                                                                                           { "sim/weather/region/update_immediately", "1" } } },
                                                                                                                       { 3,
                                                                                                                         { { "sim/weather/region/change_mode", "3" },
                                                                                                                           { "sim/weather/region/cloud_base_msl_m", "1224.686401,6096.,9144." },
                                                                                                                           { "sim/weather/region/cloud_coverage_percent", "0.75,0.,0." },
                                                                                                                           { "sim/weather/region/cloud_tops_msl_m", "4272.686523,6705.600098,9753.600586" },
                                                                                                                           { "sim/weather/region/cloud_type", "2.,2.,2." },
                                                                                                                           { "sim/weather/region/dewpoint_deg_c", "4.245509,2.466269,0.738602,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/rain_percent", "0.05" },
                                                                                                                           { "sim/weather/region/runway_friction", "1" },
                                                                                                                           { "sim/weather/region/shear_direction_degt", "3.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/shear_speed_msc", "0.771667,0.257222,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/temperatures_aloft_deg_c", "15.,8.57597,2.340051,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/turbulence", "0.04,0.02,0.01,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/variability_pct", "0.75" },
                                                                                                                           { "sim/weather/region/visibility_reported_sm", "15." },
                                                                                                                           { "sim/weather/region/wind_altitude_msl_m", "1524.,4572.,9144.,4987.636719,6234.545898,7481.45459,8728.363281,9975.273438,11222.182617,12469.091797" },
                                                                                                                           { "sim/weather/region/wind_direction_degt", "180.,225.,270.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/wind_speed_msc", "1.028889,2.057778,4.115556,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444" },
                                                                                                                           { "sim/weather/region/update_immediately", "1" } } },
                                                                                                                       { 4,
                                                                                                                         { { "sim/weather/region/change_mode", "3" },
                                                                                                                           { "sim/weather/region/cloud_base_msl_m", "615.086426,2443.886475,9144." },
                                                                                                                           { "sim/weather/region/cloud_coverage_percent", "1.,0.5,0." },
                                                                                                                           { "sim/weather/region/cloud_tops_msl_m", "2443.886475,4882.286621,9753.600586" },
                                                                                                                           { "sim/weather/region/cloud_type", "2.,2.,2." },
                                                                                                                           { "sim/weather/region/dewpoint_deg_c", "4.881948,3.102708,1.375041,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/rain_percent", "0.1" },
                                                                                                                           { "sim/weather/region/runway_friction", "2" },
                                                                                                                           { "sim/weather/region/shear_direction_degt", "3.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/shear_speed_msc", "0.771667,0.257222,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/temperatures_aloft_deg_c", "15.,8.57597,2.340051,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/turbulence", "0.04,0.02,0.01,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/variability_pct", "0.25" },
                                                                                                                           { "sim/weather/region/visibility_reported_sm", "3." },
                                                                                                                           { "sim/weather/region/wind_altitude_msl_m", "1524.,4572.,9144.,4987.636719,6234.545898,7481.45459,8728.363281,9975.273438,11222.182617,12469.091797" },
                                                                                                                           { "sim/weather/region/wind_direction_degt", "180.,225.,270.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/wind_speed_msc", "2.057778,4.115556,8.231112,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444" },
                                                                                                                           { "sim/weather/region/update_immediately", "1" } } },
                                                                                                                       { 5,
                                                                                                                         { { "sim/weather/region/change_mode", "3" },
                                                                                                                           { "sim/weather/region/cloud_base_msl_m", "127.406403,6096.,9144." },
                                                                                                                           { "sim/weather/region/cloud_coverage_percent", "1.,0.,0." },
                                                                                                                           { "sim/weather/region/cloud_tops_msl_m", "1346.606445,6705.600098,9753.600586" },
                                                                                                                           { "sim/weather/region/cloud_type", "2.,2.,2." },
                                                                                                                           { "sim/weather/region/dewpoint_deg_c", "5.384088,3.604848,1.877181,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/rain_percent", "0.2" },
                                                                                                                           { "sim/weather/region/runway_friction", "4" },
                                                                                                                           { "sim/weather/region/shear_direction_degt", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/shear_speed_msc", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/temperatures_aloft_deg_c", "15.,8.57597,2.340051,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/turbulence", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/variability_pct", "0.25" },
                                                                                                                           { "sim/weather/region/visibility_reported_sm", "2." },
                                                                                                                           { "sim/weather/region/wind_altitude_msl_m", "1246.90918,2493.818359,3740.727295,4987.636719,6234.545898,7481.45459,8728.363281,9975.273438,11222.182617,12469.091797" },
                                                                                                                           { "sim/weather/region/wind_direction_degt", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/wind_speed_msc", "-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444" },
                                                                                                                           { "sim/weather/region/update_immediately", "1" } } },
                                                                                                                       { 6,
                                                                                                                         { { "sim/weather/region/change_mode", "3" },
                                                                                                                           { "sim/weather/region/cloud_base_msl_m", "66.446404,6096.,9144." },
                                                                                                                           { "sim/weather/region/cloud_coverage_percent", "1.,0.,0." },
                                                                                                                           { "sim/weather/region/cloud_tops_msl_m", "1895.24646,6705.600098,9753.600586" },
                                                                                                                           { "sim/weather/region/cloud_type", "2.,2.,2." },
                                                                                                                           { "sim/weather/region/dewpoint_deg_c", "5.384088,3.604848,1.877181,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/rain_percent", "0.25" },
                                                                                                                           { "sim/weather/region/runway_friction", "5" },
                                                                                                                           { "sim/weather/region/shear_direction_degt", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/shear_speed_msc", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/temperatures_aloft_deg_c", "15.,8.57597,2.340051,-4.561029,-12.324462,-21.186596,-31.632307,-44.455326,-52.249184,-56.5" },
                                                                                                                           { "sim/weather/region/turbulence", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/variability_pct", "0.25" },
                                                                                                                           { "sim/weather/region/visibility_reported_sm", "0.454545" },
                                                                                                                           { "sim/weather/region/wind_altitude_msl_m", "1246.90918,2493.818359,3740.727295,4987.636719,6234.545898,7481.45459,8728.363281,9975.273438,11222.182617,12469.091797" },
                                                                                                                           { "sim/weather/region/wind_direction_degt", "0.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/wind_speed_msc", "-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444" },
                                                                                                                           { "sim/weather/region/update_immediately", "1" } } },
                                                                                                                       { 7,
                                                                                                                         { { "sim/weather/region/change_mode", "3" },
                                                                                                                           { "sim/weather/region/cloud_base_msl_m", "615.086426,6096.,9144." },
                                                                                                                           { "sim/weather/region/cloud_coverage_percent", "0.5,0.,0." },
                                                                                                                           { "sim/weather/region/cloud_tops_msl_m", "10668.,6705.600098,9753.600586" },
                                                                                                                           { "sim/weather/region/cloud_type", "3.,2.,2." },
                                                                                                                           { "sim/weather/region/dewpoint_deg_c", "21.278284,19.499044,14.707237,6.370302,-3.008983,-13.716393,-26.338127,-41.833912,-50.783924,-55.503914" },
                                                                                                                           { "sim/weather/region/rain_percent", "1." },
                                                                                                                           { "sim/weather/region/runway_friction", "6" },
                                                                                                                           { "sim/weather/region/shear_direction_degt", "3.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/shear_speed_msc", "0.771667,0.257222,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/temperatures_aloft_deg_c", "30.,22.240204,14.707238,6.370302,-3.008983,-13.716393,-26.338127,-41.833912,-51.253098,-56.5" },
                                                                                                                           { "sim/weather/region/turbulence", "0.04,0.02,0.01,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/variability_pct", "0.25" },
                                                                                                                           { "sim/weather/region/visibility_reported_sm", "25." },
                                                                                                                           { "sim/weather/region/wind_altitude_msl_m", "1524.,4572.,9144.,4987.636719,6234.545898,7481.45459,8728.363281,9975.273438,11222.182617,12469.091797" },
                                                                                                                           { "sim/weather/region/wind_direction_degt", "180.,225.,270.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/wind_speed_msc", "2.057778,4.115556,8.231112,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444" },
                                                                                                                           { "sim/weather/region/update_immediately", "1" } } },
                                                                                                                       { 8,
                                                                                                                         { { "sim/weather/region/change_mode", "3" },
                                                                                                                           { "sim/weather/region/cloud_base_msl_m", "615.086426,6096.,9144." },
                                                                                                                           { "sim/weather/region/cloud_coverage_percent", "0.75,0.,0." },
                                                                                                                           { "sim/weather/region/cloud_tops_msl_m", "13716.,6705.600098,9753.600586" },
                                                                                                                           { "sim/weather/region/cloud_type", "3.,2.,2." },
                                                                                                                           { "sim/weather/region/dewpoint_deg_c", "16.555349,14.77611,10.584842,2.726525,-6.114142,-16.206459,-28.102854,-42.707718,-51.272343,-55.835941" },
                                                                                                                           { "sim/weather/region/rain_percent", "1" },
                                                                                                                           { "sim/weather/region/runway_friction", "6" },
                                                                                                                           { "sim/weather/region/shear_direction_degt", "3.,0.,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/shear_speed_msc", "0.771667,0.257222,0.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/temperatures_aloft_deg_c", "25.,17.685459,10.584842,2.726525,-6.114142,-16.206459,-28.102854,-42.707718,-51.585125,-56.5" },
                                                                                                                           { "sim/weather/region/turbulence", "0.04,0.02,0.01,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/variability_pct", "0.5" },
                                                                                                                           { "sim/weather/region/visibility_reported_sm", "25" },
                                                                                                                           { "sim/weather/region/wind_altitude_msl_m", "1524.,4572.,9144.,4987.636719,6234.545898,7481.45459,8728.363281,9975.273438,11222.182617,12469.091797" },
                                                                                                                           { "sim/weather/region/wind_direction_degt", "180.,225.,270.,0.,0.,0.,0.,0.,0.,0." },
                                                                                                                           { "sim/weather/region/wind_speed_msc", "3.086667,6.173334,12.346667,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444,-0.514444" },
                                                                                                                           { "sim/weather/region/update_immediately", "1" } } }

};



// v3.303.13
ThreeStoppers                        data_manager::mxThreeStoppers;
std::map<std::string, dataref_param> data_manager::mapInterpolDatarefs;

// v3.303.14
const std::unordered_map<int, std::vector<mx_mission_subcategory_type>> data_manager::mapMissionCategoriesCodes = {
  { static_cast<int> ( mx_mission_type::medevac ), { mx_mission_subcategory_type::med_any_location, mx_mission_subcategory_type::med_accident, mx_mission_subcategory_type::med_outdoors } },
  { static_cast<int> ( mx_mission_type::oil_rig ), { mx_mission_subcategory_type::oilrig_cargo, mx_mission_subcategory_type::oilrig_med, mx_mission_subcategory_type::oilrig_personnel } },
  { static_cast<int> ( mx_mission_type::cargo ), { mx_mission_subcategory_type::cargo_ga, mx_mission_subcategory_type::cargo_farming, mx_mission_subcategory_type::cargo_isolated, mx_mission_subcategory_type::cargo_heavy } }
};

GatherStats                        data_manager::gather_stats;
mx_enrout_stats_strct              data_manager::strct_currentLegStats4UIDisplay; // v3.303.14 holds current leg stats
std::vector<mx_enrout_stats_strct> data_manager::vecPreviousLegStats4UIDisplay;   // v3.303.14

//// v3.305.1b
mx_sceneryOrPlaneLoad_state_strct data_manager::strct_sceneryOrPlaneLoadState;


// v3.305.2
std::list<messageLine_strct> data_manager::listOfMessageStoryMessages;
#ifdef USE_TRIGGER_OPTIMIZATION // v3.305.2
data_manager::mx_opt_leg_triggers_strct missionx::data_manager::optimize_leg_triggers_strct;
// missionx::Point                        data_manager::optLegTriggers_thread_planePos;
// base_thread::thread_state              data_manager::optLegTriggers_thread_state;
// std::thread                            data_manager::optLegTriggers_thread_ref;
#endif // USE_TRIGGER_OPTIMIZATION // v3.305.2

// v3.305.4
XPLMMapLayerID data_manager::g_layer             = NULL;
int            data_manager::s_num_cached_coords = 0;
float          data_manager::s_cached_x_coords[MAX_COORDS];
float          data_manager::s_cached_y_coords[MAX_COORDS];
float          data_manager::s_cached_lon_coords[MAX_COORDS];
float          data_manager::s_cached_lat_coords[MAX_COORDS];
float          data_manager::s_icon_width = 0;

// v24.02.5
std::map<std::string, std::string> data_manager::mapQueries;

// v24.03.1
base_thread::thread_state data_manager::threadStateMetar;

// v24.12.2
dataref_param data_manager::dref_acf_station_max_kgs_f_arr;
dataref_param data_manager::dref_m_stations_kgs_f_arr;
std::string data_manager::mission_file_supported_versions; // v24.12.2


// v25.03.1
std::string missionx::data_manager::active_acf;
std::string missionx::data_manager::prev_acf;

// -------------------------------------

static size_t
my_write(void* buffer, const size_t size, size_t nmemb, void* param)
{
  std::string &text      = *static_cast<std::string *> ( param );
  const size_t       totalsize = size * nmemb;
  text.append(static_cast<char*>(buffer), totalsize);
  return totalsize;
}

// -------------------------------------

int
callback_sqlite_data(void* data, int argc, char** argv, char** azColName)
{
  data_manager::row_gather_db_data.clear();
  for (int i = 0; i < argc; i++)
  {

    data_manager::row_gather_db_data[azColName[i]] = argv[i] ? argv[i] : "";
  }

  data_manager::reasultTable[static_cast<int> ( data_manager::reasultTable.size () )] = data_manager::row_gather_db_data;
  data_manager::row_gather_db_data.clear();


  return 0;
};



} // namespace missionx

// -------------------------------------

// missionx::data_manager::data_manager() {}

// -------------------------------------

// missionx::data_manager::~data_manager() {}


// -------------------------------------

void
data_manager::gatherFlightLegStartStats()
{
  strct_currentLegStats4UIDisplay.init();
  strct_currentLegStats4UIDisplay.sLegName                                 = currentLegName;
  strct_currentLegStats4UIDisplay.fStartingFuel                            = dataref_manager::getDataRefValue<float>("sim/flightmodel/weight/m_fuel_total");
  strct_currentLegStats4UIDisplay.fStartingPayload                         = dataref_manager::getDataRefValue<float>("sim/flightmodel/weight/m_fixed");
  strct_currentLegStats4UIDisplay.fCumulativeDistanceFlew_beforeCurrentLeg = gather_stats.get_stats_object().fCumulativeDistance_nm;
}

// -------------------------------------

void
data_manager::gatherFlightLegEndStatsAndStoreInVector()
{
  // store end stats
  strct_currentLegStats4UIDisplay.fEndFuel      = dataref_manager::getDataRefValue<float>("sim/flightmodel/weight/m_fuel_total");
  strct_currentLegStats4UIDisplay.fEndPayload   = dataref_manager::getDataRefValue<float>("sim/flightmodel/weight/m_fixed");
  strct_currentLegStats4UIDisplay.fDistanceFlew = gather_stats.get_stats_object().fCumulativeDistance_nm - strct_currentLegStats4UIDisplay.fCumulativeDistanceFlew_beforeCurrentLeg;

  // insert at the beginning of the vector
  vecPreviousLegStats4UIDisplay.insert(vecPreviousLegStats4UIDisplay.begin(), strct_currentLegStats4UIDisplay);
}

// -------------------------------------

std::string
data_manager::get_fail_timer_title_formated_for_imgui(const std::string& in_default_title, const std::string prefix_to_foramted_string)
{
  if (lowestFailTimerName_s.empty())
    return in_default_title;

  auto remaining_time = mapFailureTimers[lowestFailTimerName_s].getRemainingTime();
  auto days           = static_cast<int> ( remaining_time / SECONDS_IN_1DAY );
  auto hours          = static_cast<int> ( remaining_time / SECONDS_IN_1HOUR_3600 );
  auto minutes        = static_cast<int> ( ( remaining_time - ( hours * SECONDS_IN_1HOUR_3600 ) ) / SECONDS_IN_1MINUTE );
  auto seconds        = static_cast<int> ( remaining_time - ( hours * SECONDS_IN_1HOUR_3600 + minutes * SECONDS_IN_1MINUTE ) );

  if (days > 0)
    return prefix_to_foramted_string + mxUtils::formatNumber<int>(days) + ":" + mxUtils::formatNumber<int>(hours) + ":" + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(d:h:m:s)";

  return prefix_to_foramted_string + mxUtils::formatNumber<int>(hours) + ":" + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(h:m:s)";
}

// -------------------------------------

std::string
data_manager::get_fail_timer_in_formated_text(const std::string prefix_to_foramted_string)
{
  if (lowestFailTimerName_s.empty())
    return "";

  auto remaining_time = mapFailureTimers[lowestFailTimerName_s].getRemainingTime();
  auto days           = static_cast<int> ( remaining_time / SECONDS_IN_1DAY );
  auto hours          = static_cast<int> ( remaining_time / SECONDS_IN_1HOUR_3600 );
  auto minutes        = static_cast<int> ( ( remaining_time - ( hours * SECONDS_IN_1HOUR_3600 ) ) / SECONDS_IN_1MINUTE );
  auto seconds        = static_cast<int> ( remaining_time - ( hours * SECONDS_IN_1HOUR_3600 + minutes * SECONDS_IN_1MINUTE ) );

  if (days > 0)
    return prefix_to_foramted_string + mxUtils::formatNumber<int>(days) + ":" + mxUtils::formatNumber<int>(hours) + ":" + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(d:h:m:s)";

  return prefix_to_foramted_string + mxUtils::formatNumber<int>(hours) + ":" + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(h:m:s)";
}

// -------------------------------------

void
data_manager::flc_vr_state()
{
  //// v3.0.219.7
  mxvr::vr_display_missionx_in_vr_mode = XPLMGetDatai(g_vr_dref);

  auto val = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISPLAY_MISSIONX_IN_VR(), mxconst::DEFAULT_DISPLAY_MISSIONX_IN_VR);
  if (!val)
    mxvr::vr_display_missionx_in_vr_mode = false;

  // if (missionx::mxvr::vr_display_missionx_in_vr_mode)
  //   missionx::mxvr::vr_ratio = missionx::mxvr::VR_RATIO; // 0.75f; // 0.52f;
  // else
  //   missionx::mxvr::vr_ratio = missionx::mxvr::DEFAULT_RATIO; //  1.0f;
}

// -------------------------------------

std::string
data_manager::set_local_time(int inHours, int inMinutes, int inDayOfYear_0_364, bool gForceTimeSet)
{
  static dataref_const dc;
  std::string                    err;
  err.clear();


  // do sanity tests
  if (!gForceTimeSet && timelapse.flag_isActive)
    return ("TimeLapsed is active. Can't interfier with its action. Aborting your request. Suggestion: Time it differently.");

  if (inHours < 0 || inHours > 23)
    return ("Hours value needs to be between 0-23. Value received: " + Utils::formatNumber<int>(inHours));

  if (inMinutes < 0 || inMinutes > 59)
    return ("Minutes value needs to be between 0-59. Value received: " + Utils::formatNumber<int>(inMinutes));

  if (inDayOfYear_0_364 < -1 || inDayOfYear_0_364 > 364)
    return ("Day in year must be between 0 and 364. Value received: " + Utils::formatNumber<int>(inDayOfYear_0_364));

  float zulu_time_sec_f  = XPLMGetDataf(dc.dref_zulu_time_sec_f);
  float local_time_sec_f = XPLMGetDataf(dc.dref_local_time_sec_f);

  if (inDayOfYear_0_364 != -1) // -1 = undefined, same day.
    XPLMSetDatai(dc.dref_local_date_days_i, (inDayOfYear_0_364));

  float new_local_time_in_seconds               = static_cast<float> ( inHours * SECONDS_IN_1HOUR_3600 + inMinutes * SECONDS_IN_1MINUTE );
  float delta_between_currentLocal_and_newLocal = new_local_time_in_seconds - local_time_sec_f; // we want to minus to add later to zulu

  // set the new time
  XPLMSetDataf(dc.dref_zulu_time_sec_f, zulu_time_sec_f + delta_between_currentLocal_and_newLocal); // set zulu time to reflect the local time


  return err;
}

// -------------------------------------

void
data_manager::readPluginTextures()
{
  std::string                 errorMsg;
  const std::string                 bitmapPath      = mx_folders_properties.getAttribStringValue (mxconst::get_PROP_MISSIONX_BITMAP_PATH(), "", errStr);
  const std::array<std::string, 24> textureName_arr = { mxconst::get_BITMAP_LOAD_MISSION(),
                                                  mxconst::get_BITMAP_INVENTORY_MXPAD(),
                                                  mxconst::get_BITMAP_MAP_MXPAD(),
                                                  mxconst::get_BITMAP_AUTO_HIDE_EYE_FOCUS(),
                                                  mxconst::get_BITMAP_AUTO_HIDE_EYE_FOCUS_DISABLED(),
                                                  
                                                  mxconst::get_BITMAP_HOME(),
                                                  mxconst::get_BITMAP_TARGET_MARKER_ICON(),
                                                  mxconst::get_BITMAP_BTN_LAB_24X18(),
                                                  mxconst::get_BITMAP_BTN_WORLD_PATH_24X18(),
                                                  
                                                  mxconst::get_BITMAP_BTN_PREPARE_MISSION_24X18(),
                                                  mxconst::get_BITMAP_BTN_FLY_MISSION_24X18(),
                                                  mxconst::get_BITMAP_BTN_SETUP_24X18(),
                                                  mxconst::get_BITMAP_BTN_TOOLBAR_SETUP_64x64(),
                                                  mxconst::get_BITMAP_BTN_QUIT_48x48(),
                                                  mxconst::get_BITMAP_BTN_SAVE_48x48(),
                                                  mxconst::get_BITMAP_BTN_ABOUT_64x64(),
                                                  mxconst::get_BITMAP_BTN_NAV_24x18(),
                                                  mxconst::get_BITMAP_FMOD_LOGO(),
                                                  mxconst::get_BITMAP_BTN_WARN_SMALL_32x28(),
                                                  mxconst::get_BITMAP_BTN_CONVERT_FPLN_TO_MISSION_24X18(),
                                                  
                                                  mxconst::get_BITMAP_BTN_NAVINFO(),
                                                  mxconst::get_BITMAP_BTN_SIMBRIEF_BIG(),
                                                  mxconst::get_BITMAP_BTN_SIMBRIEF_ICO(),
                                                  mxconst::get_BITMAP_BTN_FLIGHTPLANDB() };

  // ranged for loop is supported
  for (const auto& f : textureName_arr)
  {
    mxTextureFile tFile;
    tFile.fileName = f;
    tFile.filePath = bitmapPath;

    BitmapReader::loadGLTexture(tFile, errorMsg, false); // v3.0.253.8 do not flip image

    if (tFile.gTexture != 0)
    {
      Utils::addElementToMap(mapCachedPluginTextures, tFile.fileName, tFile);
      Log::logXPLMDebugString("Loaded bitmap: " + tFile.getAbsoluteFileLocation() + "\n"); // debug
    }
  }
}

// -------------------------------------

void
data_manager::seedAdHocParams(const std::string& inParams_s, mxProperties& inSmPropSeedValues)
{

  const std::vector<std::string> vecOfParameterExpressions = mxUtils::split_v2(inParams_s, ",");
  for (const auto& paramExpression : vecOfParameterExpressions)
  {
    const std::vector<std::string> vecNameValue = mxUtils::split_v2(paramExpression, "=");
    if (vecNameValue.size() > 1)
    {
      const std::string name = mxUtils::stringToLower(vecNameValue.at(0));
      const std::string valu = vecNameValue.at(1);

      // check if name starts with "in"
      if (name.find("in") != 0)
      {
        Log::logMsg("Ad hock param name: " + name + ", does not start with 'in' string. ignoring it.");
        continue;
      }


      // seed the correct datatype
      double dOutValue = 0.0;
      if (mxUtils::isStringNumber<double>(dOutValue, valu, &std::dec))
      {
        inSmPropSeedValues.setNumberProperty(name, dOutValue);
        continue;
      }

      bool bBoolValue = false;
      if (mxUtils::isStringBool(valu, bBoolValue))
      {
        inSmPropSeedValues.setBoolProperty(name, bBoolValue);
        continue;
      }

      // seed as string
      inSmPropSeedValues.setStringProperty(name, valu);

    } // end if vector in minimal size
  }   // loop over all params expressions

  // seedAdHocParams
}


// -------------------------------------

bool
data_manager::execScript(const std::string& inScriptName, mxProperties& inSmPropSeedValues, std::string_view inFailureMessage)
{

  std::string result_s{};
  std::string mxFuncCall{};
  std::string seed_in_parameters_s{};

  if (inScriptName.empty())
    return "Script Error: Empty script name provided."; // skip

#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), inScriptName, false);
#endif // TIMER_FUNC

  if (missionState == mx_mission_state_enum::mission_is_running) // v3.0.223.5 do not execute scripts if not in running state
  {

    // v3.305.3 implement adhoc parameter settings = name(inParam1=x, inParam2=y)
    // split the string
    std::string scriptNameWithoutTheParams = Utils::extractBaseFromString(inScriptName, "|", &seed_in_parameters_s); // cond_script="callYouHaveMessage|inNextMessage=msgLeg2_start"
    // check validity of the params


    const std::string scriptName = Utils::extractBaseFromString(scriptNameWithoutTheParams, ".", &mxFuncCall);
    inSmPropSeedValues.setStringProperty(mxconst::get_EXT_MX_FUNC_CALL(), (mxFuncCall.empty()) ? scriptName : mxFuncCall);

    #ifdef DEBUG_SCRIPT
    Log::logMsg(">> Run Script: " + mxconst::get_QM() + inScriptName + mxconst::get_QM() + ", Func: " + mxconst::get_QM() + mxFuncCall + mxconst::get_QM()); // debug
    #endif                                                                                           // DEBUG_SCRIPT

    if (!seed_in_parameters_s.empty()) // v3.305.3
      seedAdHocParams(seed_in_parameters_s, inSmPropSeedValues);

    script_manager::mapScripts[scriptName].setWasCalled(true); // v3.305.3
    script_manager::mapScripts[scriptName].sExecutionTime = Timer::get_debugTimestamp();
    {
      result_s = sm.run_script(scriptName, inSmPropSeedValues);
    }
    script_manager::mapScripts[scriptName].sEndExecutionTime = Timer::get_debugTimestamp(); // v3.305.3

    const std::string& externalScriptFuncName = scriptNameWithoutTheParams;
    if (!mxFuncCall.empty() && scriptName != mxFuncCall)
    {
      missionx::script_manager::mapScripts[externalScriptFuncName].setWasCalled(true);
      missionx::script_manager::mapScripts[externalScriptFuncName].sExecutionTime    = missionx::script_manager::mapScripts[scriptName].sExecutionTime;
      missionx::script_manager::mapScripts[externalScriptFuncName].sEndExecutionTime = missionx::script_manager::mapScripts[scriptName].sEndExecutionTime;
      // set some ad hoc metadata
      missionx::script_manager::mapScripts[externalScriptFuncName].file_name       = externalScriptFuncName;
      missionx::script_manager::mapScripts[externalScriptFuncName].script_body     = "Taken From: " + missionx::script_manager::mapScripts[scriptName].file_name;
      missionx::script_manager::mapScripts[externalScriptFuncName].script_is_valid = true;
    }

    if (!result_s.empty())
    {
      // v3.305.3
      missionx::script_manager::mapScripts[scriptName].err_desc = result_s;
      if (!mxFuncCall.empty() && scriptName != mxFuncCall)
      {
        missionx::script_manager::mapScripts[externalScriptFuncName].err_desc        = result_s;
        missionx::script_manager::mapScripts[externalScriptFuncName].script_is_valid = false;
      }


      std::string abort_mission_reason = std::string(inFailureMessage) + "\n" + result_s;
      if (!flag_setupSkipMissionAbortIfScriptIsInvalid)
        setAbortMission(abort_mission_reason);

      // write failure to log
      Log::logMsgErr(result_s);

      return false;
    }
  }

  return true;
}

// -------------------------------------

void
data_manager::addInstanceNameToDisplayList(const std::string& instName) // v3.0.207.1
{
  if (map3dInstances[instName].obj3dType == obj3d::obj3d_type::static_obj)
  {
    if ( !listDisplayStatic3dInstances.contains(instName ) )
      listDisplayStatic3dInstances.emplace(instName);
  }
  else
  {
    if ( !listDisplayMoving3dInstances.contains(instName ) )
      listDisplayMoving3dInstances.emplace(instName);
  }
}

// -------------------------------------

void
data_manager::delInstanceNameFromDisplayList(const std::string& instName) // v3.0.207.1
{
  if (map3dInstances[instName].obj3dType == obj3d::obj3d_type::static_obj)
  {
    if ( auto inst = listDisplayStatic3dInstances.find ( instName )
      ; inst != listDisplayStatic3dInstances.end())
    {
      #ifndef RELEASE
      Log::logMsg("[flc obj] Destroying Static Instance: " + instName);
      #endif
      listDisplayStatic3dInstances.erase(inst);
    }
  }
  else
  {
    if ( auto inst = listDisplayMoving3dInstances.find ( instName )
      ; inst != listDisplayMoving3dInstances.end())
    {
      #ifndef RELEASE
      Log::logMsg("[flc obj] Destroying Moving Instance: " + instName);
      #endif
      listDisplayMoving3dInstances.erase(inst);
    }
  }
}

// -------------------------------------

void
data_manager::clearFMSEntries()
{
  // clear GPS
  int entries = XPLMCountFMSEntries();
  for (int i = (entries - 1); i >= 0; --i) // entries starts from 0..N-1
    XPLMClearFMSEntry(i);
}

// -------------------------------------

void
data_manager::clearMissionLoadedTextures()
{
  // clear maps and images data
  XPLMDebugString("\n\tReleasing mission textures."); // debug
  mapCurrentMissionTexturePathLocator.clear();
  for ( auto &val : mapCurrentMissionTextures | std::views::values )
  {
    glDeleteTextures(1, reinterpret_cast<const GLuint *> ( &val.gTexture ) ); // v3.0.211.2
  }
  mapCurrentMissionTextures.clear();

  XPLMDebugString("\n\tReleasing inventory textures."); // debug
  for ( auto &val : xp_mapInvImages | std::views::values )
  {
    glDeleteTextures(1, reinterpret_cast<const GLuint *> ( &val.gTexture ) );
  }
  xp_mapInvImages.clear();


  XPLMDebugString("\n\tReleasing Message Story textures."); // debug
  releaseMessageStoryCachedTextures(); // v3.305.1

  XPLMDebugString("\n\tReleasing Random Template textures."); // debug
  clearRandomTemplateTextures();
}


// -------------------------------------


void
data_manager::releaseMessageStoryCachedTextures()
{
  // v3.305.1 clear Message story cached images
  Message::vecStoryCurrentImages_p.clear();
  for (size_t loop1 = 0; loop1 < Message::VEC_STORY_IMAGE_SIZE_I; ++loop1) // should have 7
    Message::vecStoryCurrentImages_p.emplace_back(nullptr);

  for ( auto &img : Message::mapStoryCachedImages | std::views::values )
  {
    glDeleteTextures(1, reinterpret_cast<const GLuint *> ( &img.gTexture ) );
  }

  Message::mapStoryCachedImages.clear();
}

// -------------------------------------

void
data_manager::clearRandomTemplateTextures()
{

  mapGenerateMissionTemplateFilesLocator.clear(); // clear locator for layer set
  for (auto& img : mapGenerateMissionTemplateFiles)
  {
    glDeleteTextures(1, (const GLuint*)&img.second.imageFile.gTexture); // v3.0.217.2
  }
  mapGenerateMissionTemplateFiles.clear();
}

// -------------------------------------

std::string
data_manager::translate_enum_mission_state_to_string ( const mx_mission_state_enum mState )
{
  switch ( mState )
  {
    case mx_mission_state_enum::mission_loaded_from_the_original_file:
      return "mission_loaded_from_the_original_file";
      break;
    case mx_mission_state_enum::mission_loaded_from_savepoint:
      return "mission_loaded_from_savepoint";
      break;
    case mx_mission_state_enum::pre_mission_running:
      return "pre_mission_running";
      break;
    case mx_mission_state_enum::mission_is_running:
      return "mission_is_running";
      break;
    case mx_mission_state_enum::mission_stopped:
      return "mission_stopped";
      break;
    case mx_mission_state_enum::mission_aborted:
      return "mission_aborted";
      break;
    case mx_mission_state_enum::mission_completed_success:
      return "mission_completed_success";
      break;
    case mx_mission_state_enum::mission_completed_failure:
      return "mission_completed_failure";
      break;
    default:
      break;
  }

  return "mission_undefined";
}

// -------------------------------------

mx_mission_state_enum
data_manager::translate_mission_state_to_enum ( const std::string &mState )
{
  if (mState == "mission_loaded_from_the_original_file")
    return mx_mission_state_enum::mission_loaded_from_the_original_file;
  else if (mState == "mission_loaded_from_savepoint")
    return mx_mission_state_enum::mission_loaded_from_savepoint;
  else if (mState == "pre_mission_running")
    return mx_mission_state_enum::pre_mission_running;
  else if (mState == "mission_is_running")
    return mx_mission_state_enum::mission_is_running;
  else if (mState == "mission_stopped")
    return mx_mission_state_enum::mission_stopped;
  else if (mState == "mission_aborted")
    return mx_mission_state_enum::mission_aborted;
  else if (mState == "mission_completed_success")
    return mx_mission_state_enum::mission_completed_success;
  else if (mState == "mission_completed_failure")
    return mx_mission_state_enum::mission_completed_failure;


  return mx_mission_state_enum::mission_undefined;

}

// -------------------------------------

void
data_manager::loadInventoryImages()
{
  std::lock_guard<std::mutex> lock(s_ImageLoadMutex);
  std::string                 errorMsg;

  for (auto& [file, buttonTexture] : xp_mapInvImages)
  {
    mxTextureFile btnImage;
    // mxTextureFile textureFile;
    btnImage.fileName = file;
    btnImage.filePath = mapBrieferMissionList[selectedMissionKey].pathToMissionPackFolderInCustomScenery + XPLMGetDirectorySeparator();


    if (BitmapReader::loadGLTexture(btnImage, errorMsg, false, false)) // load image but do not flip it
    {
      // store vecTextures as generic texture button
      buttonTexture = btnImage;

      Log::logMsgThread("Loaded Inventory Texture: " + buttonTexture.fileName); // debug

    } // end if loaded texture
    else
    {
      Log::logMsgThread("Failed to load Inventory texture file: " + file + "\nError: " + errorMsg);
    }
  }

  if (!xp_mapInvImages.empty())
    queFlcActions.push(mx_flc_pre_command::post_async_inv_image_binding);
}


// -------------------------------------

#ifdef USE_TRIGGER_OPTIMIZATION // v3.305.2
void
missionx::data_manager::optimizeLegTriggers_thread(base_thread::thread_state* outThreadState, missionx::Point* inRefPoint, std::map<std::string, missionx::Trigger>* inMapTriggers, std::list<std::string>* inListTriggersByDistance, std::list<std::string>* outListTriggers_ptr)
{
  std::lock_guard<std::mutex> lock(missionx::data_manager::mt_SortTriggersMutex);

  // loop over all triggers in inListTriggersByDistance
  // calculate the distance of inRefPoint relative to their boundaries:
  //  - rad: distance - "rad"
  //  - poly: Shortest distance to one of the points

  // loop over all result container and place the trigger in the sorted location

  outThreadState->flagIsActive     = true;
  outThreadState->thread_done_work = false;

  std::map<double, std::string> sortedTriggersByDistance; // map key=distance value: trigger Name

  // sort triggers
  for (const auto& trgName : (*inListTriggersByDistance))
  {
    if (outThreadState->abort_thread)
      break;

    double dShortestDist_mt = 99999999999.0;
    for (auto& p : missionx::data_manager::mapTriggers[trgName].deqPoints)
    {
      if (outThreadState->abort_thread)
        break;

      // calculate in meters
      auto dist_mt = Utils::calcDistanceBetween2Points_nm_ts(inRefPoint->lat, inRefPoint->lon, p.lat, p.lon, missionx::mx_units_of_measure::meter);

      if (missionx::data_manager::mapTriggers[trgName].getTriggerType().compare(mxconst::get_TRIG_TYPE_RAD()) == 0)
      {
        double dLengthMt = Utils::readNodeNumericAttrib<double>(missionx::data_manager::mapTriggers[trgName].xRadius, mxconst::get_ATTRIB_LENGTH_MT(), -1.0);
        assert(dLengthMt >= 0.0 && "Radius trigger must have length_mt attribute");

        if (dist_mt - dLengthMt <= 0.0)
          missionx::data_manager::mapTriggers[trgName].flag_inPhysicalArea_fromThread = true;

        dShortestDist_mt = (dist_mt - dLengthMt > 0.0) ? dist_mt - dLengthMt : dist_mt; // make sure we return a positive number or the closest to center
        break;
      }

      dShortestDist_mt = (dShortestDist_mt > dist_mt) ? dist_mt : dShortestDist_mt; // poly type trigger

    } // end loop over triggers points

    if (outThreadState->abort_thread)
      break;

    if (missionx::data_manager::mapTriggers[trgName].getTriggerType().compare(mxconst::get_TRIG_TYPE_POLY()) == 0)
      missionx::data_manager::mapTriggers[trgName].flag_inPhysicalArea_fromThread = missionx::data_manager::mapTriggers[trgName].isPointInPolyArea((*inRefPoint));

    // place distance in sorted list
    while (missionx::mxUtils::isElementExists(sortedTriggersByDistance, dShortestDist_mt))
      dShortestDist_mt += 0.1;

    missionx::Utils::addElementToMap(sortedTriggersByDistance, dShortestDist_mt, trgName);

  } // end loop over triggers

  if (!outThreadState->abort_thread)
  {
    outListTriggers_ptr->clear();
    for (auto& [key, val] : sortedTriggersByDistance)
      outListTriggers_ptr->emplace_back(val);
  }

  outThreadState->thread_done_work = true;
  outThreadState->flagIsActive     = false;
}

#endif

// -------------------------------------


void
data_manager::loadStoryImage(Message* msg, const std::string& inImageName_vu)
{
  std::lock_guard<std::mutex> lock(s_StoryImageLoadMutex);

  if (Message::lineAction4ui.state == enum_mx_line_state::loading)
  {
    #ifndef RELEASE
    Log::logMsgThread((!inImageName_vu.empty()) ? "Loading: " + inImageName_vu : "Release image.");
    #endif // !RELEASE

    if (mxUtils::isElementExists(Message::mapStoryCachedImages, inImageName_vu))
    {
      #ifndef RELEASE
      Log::logMsgThread("Found image in cache: " + inImageName_vu + ", Skipping load.");
      #endif // !RELEASE
      Message::lineAction4ui.state = enum_mx_line_state::ready;
    }
    else
    {
      mxTextureFile btnImage;
      btnImage.filePath = mapBrieferMissionList[selectedMissionKey].pathToMissionPackFolderInCustomScenery;
      // try to figure file name
      std::vector<std::string> vecExtensions = { "", ".png", ".jpg" };
      for (const auto& file_ext : vecExtensions)
      {
        if ( fs::path image = fmt::format("{}{}{}{}", btnImage.filePath, XPLMGetDirectorySeparator(), inImageName_vu, file_ext)
          ; is_regular_file (image))
        {
          btnImage.fileName = inImageName_vu + file_ext;
          break; // exit loop
        }
      }

      #ifndef RELEASE
      Log::logMsgThread("Final image filename: " + btnImage.fileName);
      #endif // !RELEASE

      std::string errorMsg; // v24.06.1
      if (!btnImage.fileName.empty() && BitmapReader::loadGLTexture(btnImage, errorMsg, false, false)) // load image but do not flip it
      {
        Utils::addElementToMap(Message::mapStoryCachedImages, inImageName_vu, btnImage);
        Message::lineAction4ui.state = enum_mx_line_state::ready;

        Log::logMsgThread("Loaded Story Texture: " + btnImage.fileName); // debug

      } // end if loaded texture
      else
      {
        if (inImageName_vu.empty())
          Log::logMsgThread("Releasing image in position: " + Message::lineAction4ui.vals[mxconst:: mxconst::get_STORY_IMAGE_POS()]); // debug
        else
          Log::logMsgThread("Failed loading Story Texture: " + inImageName_vu); // debug

      } // end if Texture loaded
    }   // end if Texture file name is not in the message cache

    queFlcActions.push(mx_flc_pre_command::post_async_story_image_binding); // Always call this action so it will mark the [i] action as "action_ended". It also binding unbound textures.

    // Always try to bind or reset the vector cell to nullptr so it won't show in the Briefer screen
    // store vecTextures as generic texture button
    const auto lmbda_get_image_pointer = [](const std::string& inImageFile) -> mxTextureFile*
    {
      if (mxUtils::isElementExists(Message::mapStoryCachedImages, inImageFile))
      {
        return &Message::mapStoryCachedImages[inImageFile]; // return the pointer
      }

      return nullptr; // force the return of nullptr so our vector will hold nullptr value
    };

    // Store the pointer in the correct image position on the screen
    if (Message::lineAction4ui.vals[ mxconst::get_STORY_IMAGE_POS()] == Message::IMG_POS_S_LEFT)
      Message::vecStoryCurrentImages_p.at(Message::IMG_LEFT) = lmbda_get_image_pointer(inImageName_vu);
    else if (Message::lineAction4ui.vals[ mxconst::get_STORY_IMAGE_POS()] == Message::IMG_POS_S_RIGHT)
      Message::vecStoryCurrentImages_p.at(Message::IMG_RIGHT) = lmbda_get_image_pointer(inImageName_vu);
    else if (Message::lineAction4ui.vals[ mxconst::get_STORY_IMAGE_POS()] == Message::IMG_POS_S_CENTER)
      Message::vecStoryCurrentImages_p.at(Message::IMG_CENTER) = lmbda_get_image_pointer(inImageName_vu);
    else if (Message::lineAction4ui.vals[ mxconst::get_STORY_IMAGE_POS()] == Message::IMG_POS_S_BACKROUND) // back ground image
      Message::vecStoryCurrentImages_p.at(Message::IMG_BACKROUND) = lmbda_get_image_pointer(inImageName_vu);
    else if (Message::lineAction4ui.vals[ mxconst::get_STORY_IMAGE_POS()] == Message::IMG_POS_S_LEFT_MED)
      Message::vecStoryCurrentImages_p.at(Message::IMG_LEFT_MED) = lmbda_get_image_pointer(inImageName_vu);
    else if (Message::lineAction4ui.vals[ mxconst::get_STORY_IMAGE_POS()] == Message::IMG_POS_S_CENTER_MED)
      Message::vecStoryCurrentImages_p.at(Message::IMG_CENTER_MED) = lmbda_get_image_pointer(inImageName_vu);
    else if (Message::lineAction4ui.vals[ mxconst::get_STORY_IMAGE_POS()] == Message::IMG_POS_S_RIGHT_MED)
      Message::vecStoryCurrentImages_p.at(Message::IMG_RIGHT_MED) = lmbda_get_image_pointer(inImageName_vu);

  } // end if Message::lineAction4ui.isReady


  // End loadStoryImage
}


// -------------------------------------

void
data_manager::loadAllMissionsImages() // used in list mission screen (for example)
{
  iMissionImageCounter = 0;
  std::string errorMsg; // v24.06.1

  for (auto& [file_s, mInfo] : mapBrieferMissionList)
  {
    if (mxUtils::isElementExists(mInfo.mapImages, file_s) && !mInfo.mapImages[file_s].fileName.empty() && !mInfo.mapImages[file_s].filePath.empty())
    {

      if (BitmapReader::loadGLTexture(mInfo.mapImages[file_s], errorMsg, false)) // load image but do not flip it
      {
        // store vecTextures as generic texture button

        mxTextureFile missionxTextureButton = mInfo.mapImages[file_s];
        Utils::addElementToMap(xp_mapMissionIconImages, file_s, missionxTextureButton); // store the mxTextureFile in our generic image map for later use

        Log::logMsg("Loaded Mission Image: " + mInfo.mapImages[file_s].fileName); // debug

        ++iMissionImageCounter;
      }
    }

  } // end loop over valid missions

  Log::logMsg("After loadAllMissionsImages!! ");
}

// -------------------------------------

bool
data_manager::prepare_choice_options(const std::string& inChoiceName)
{
  // 1. check if there is a choice by same name
  // 2. extract all options and fill mapChoiceOptions std::map container.
  if (!inChoiceName.empty())
  {
    IXMLNode xChoice = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(xmlChoices, mxconst::get_ELEMENT_CHOICE(),  mxconst::get_ATTRIB_NAME(), inChoiceName, false);
    if (!xChoice.isEmpty())
    {
      mxChoice.init();
      mxChoice.parseChoiseNode(xChoice);
    }
    else
    {
      Log::logMsgWarn("Failed to find a choice by the name: " + inChoiceName);
      return false;
    }
  }

  return true;
}

// -------------------------------------

bool
data_manager::is_choice_name_exists(std::string& inChoiceName)
{
  #ifndef RELEASE
  const IXMLNode xNode = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(xmlChoices,  mxconst::get_ELEMENT_CHOICE(),  mxconst::get_ATTRIB_NAME(), inChoiceName, false); // debug
  #endif                                                                                                                                                                                                 // !RELEASE

  // search <choice> element with name: inChoiceName. If found then it is not empty node.
  return !(Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(xmlChoices,  mxconst::get_ELEMENT_CHOICE(),  mxconst::get_ATTRIB_NAME(), inChoiceName, false).isEmpty());
}


// -------------------------------------

std::string
data_manager::write_fpln_to_external_folder()
{
  ///// Supported formats
  const std::string GTNRXP = "GTNRXP"; // G750
  const std::string GNSRXP = "GNSRXP"; // G530
  const std::string LTLNAV = "LTLNAV"; // LittleNavMap
  const std::string XPLN11 = "XPLN11"; // XPLANE 11 FMS format

  bool bWroteToFile = false;

  std::string err;
  char        path[1024];
  XPLMGetSystemPath(path);
  std::string xplane_flight_plan_path = std::string(path) + "Output" + XPLMGetDirectorySeparator() + "FMS plans" + XPLMGetDirectorySeparator();
  std::string plugin_folder           = Utils::getPluginDirectoryWithSep(PLUGIN_DIR_NAME);

  if (!plugin_folder.empty()) // if not NULL
  {
    const std::string fpln_file_path = plugin_folder + XPLMGetDirectorySeparator() + mxconst::get_EXTERNAL_FPLN_FOLDERS_FILE_NAME();
    // read from file

    std::ifstream infs;

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // prepare reading file: "fpln_folders.ini" file
    infs.open(fpln_file_path.c_str(), std::ios::in); // can we read the file
    if (!infs.is_open())
    {
      err = "[write_fpln_to_external_folder] Fail to open file: " + fpln_file_path + "\n";
      Log::logAttention(err, false); // false = not thread message
    }
    else
    {
      // PREPARE data based on the FMS
      if ( std::deque<NavAidInfo> fmsNavAids = extractNavaidInfoFromPlanesFMS ()
        ; !fmsNavAids.empty())
      {
        std::ofstream outFPLN_file;
        // read each line and parse it
        std::string line;
        while (getline(infs, line))
        {
          // skip if has comment string or is empty
          line = mxUtils::trim(line);
          if ((!line.empty() && (line.front() == '#' || line.front() == ';')) || line.empty())
            continue;

          bool                     b_write_to_xp_folder_option = Utils::getNodeText_type_1_5<bool> ( system_actions::pluginSetupOptions.node, mxconst::get_OPT_WRITE_CONVERTED_FPLN_TO_XPLANE_FOLDER(), mxconst::DEFAULT_WRITE_CONVERTED_FMS_TO_XPLANE_FOLDER );
          std::vector<std::string> vecLineData                 = mxUtils::split_v2 ( line, mxconst::get_PIPE_DELIMITER() );

          ///// FORMAT: {type}|{location}
          const int VECTOR_SIZE_I = static_cast<int> ( vecLineData.size () );
          if (!vecLineData.empty())
          {
            std::string fpln_type = vecLineData.front();

            // return folder to use with seperator
            const auto lmbda_get_target_fpln_folder = [&]()
            {
              std::string local_path;

              if (b_write_to_xp_folder_option == false && VECTOR_SIZE_I > 1)
                local_path = vecLineData.at(1);
              else
                local_path = xplane_flight_plan_path;

              // make sure we finish with "/" (folder separator)
              const char* sep = XPLMGetDirectorySeparator();
              if (local_path.back() != sep[0]) // if we have missing seperator folder, add it to the path string
                local_path += XPLMGetDirectorySeparator();

              return local_path;
            };

            const std::string fpln_folder = lmbda_get_target_fpln_folder();

            std::string fpln_custom_missionx_path = fpln_folder + mxconst::get_EXTERNAL_FPLN_TARGET_FILE_NAME();
            const auto  lmbda_get_fpln_extension  = [&]() // get the file extension
            {
              if (fpln_type == GTNRXP)
              {
                return ".gfp";
              }
              else if (fpln_type == GNSRXP)
              {
                return ".fpl";
              }
              else if (fpln_type == LTLNAV)
              {
                return ".lnmpln";
              }
              else if (fpln_type == XPLN11)
              {
                return "_xp11.fms";
              }

              return "";
            };

            const std::string extension_s = lmbda_get_fpln_extension();
            fpln_custom_missionx_path += extension_s;


            if (!fpln_custom_missionx_path.empty() && !extension_s.empty())
            {
              outFPLN_file.open(fpln_custom_missionx_path.c_str()); // can we create/open the file ?
              if (outFPLN_file.fail())
              {
                err = "[write_fpln_to_external_folder] Fail to create file: " + fpln_custom_missionx_path + "\n";
                Log::logAttention(err, false); // false = not thread message
              }
              else
              {
                #ifndef RELEASE
                Log::logMsg("[write_fpln_to_external_folder] folder: " + fpln_custom_missionx_path, false); // false = not thread message
                #endif

                if (fpln_type == GTNRXP)
                {
                  outFPLN_file << prepare_flight_plan_for_GTN_RXP(fmsNavAids) << '\n'; // PREPARE GTN
                  bWroteToFile = true;
                }
                else if (fpln_type == GNSRXP)
                {
                  IXMLNode     xFlightPlan = prepare_flight_plan_for_GNS530_RXP(fmsNavAids); // PREPARE GNS
                  IXMLRenderer xmlWriter;
                  outFPLN_file << xmlWriter.getString(xFlightPlan) << '\n';
                  xmlWriter.clear();
                  bWroteToFile = true;
                }
                else if (fpln_type == LTLNAV)
                {
                  IXMLNode     xFlightPlan = prepare_flight_plan_for_LTLNAV(fmsNavAids); // PREPARE LittleNavMap
                  IXMLRenderer xmlWriter;
                  outFPLN_file << xmlWriter.getString(xFlightPlan) << '\n';
                  xmlWriter.clear();
                  bWroteToFile = true;
                }
                else if (fpln_type == XPLN11)
                {
                  outFPLN_file << prepare_flight_plan_for_XPLN11(fmsNavAids) << '\n'; // PREPARE XPLN11 FMS file
                  bWroteToFile = true;
                }
              }

              // close output stream
              if (outFPLN_file.is_open())
                outFPLN_file.close();
            } // end if fpln_custom_missionx_path
            else
            {
              Log::logMsg("[write fpln] No file extension or target folder found. Please check your [" + fpln_file_path + "] settings.");
            }

          } // end if vector size is valid

        } // end read all lines in fpln_folder.ini
      }   // end infs.open
    }
    if (infs.is_open())
      infs.close();
  }

  if (!bWroteToFile && err.empty())
    err = "Did not write to any external FMS. Check '" + mxconst::get_EXTERNAL_FPLN_FOLDERS_FILE_NAME() + "' settings in Mission-X plugin folder.";

  return err;
}

// -------------------------------------

std::deque<NavAidInfo>
data_manager::extractNavaidInfoFromPlanesFMS()
{
  std::deque<NavAidInfo> deqNavAids;

  const int fms_entry = XPLMCountFMSEntries();

  for (int i = 0; i < fms_entry; ++i)
  {
    NavAidInfo nav;
    XPLMGetFMSEntryInfo(i, &nav.navType, nav.ID, &nav.navRef, nullptr, &nav.lat, &nav.lon); // ignore elevation
    if (nav.navRef != XPLM_NAV_NOT_FOUND && nav.navType != xplm_Nav_LatLon)
    {
      XPLMGetNavAidInfo(nav.navRef, &nav.navType, &nav.lat, &nav.lon, &nav.height_mt, &nav.freq, &nav.heading, nav.ID, nav.name, nullptr);
    }

    nav.synchToPoint();
    deqNavAids.push_back(nav);
  }


  return deqNavAids;
}

// -------------------------------------

std::string
data_manager::prepare_flight_plan_for_GTN_RXP(std::deque<NavAidInfo>& inNavList)
{
  const size_t MAX_GTN_ICAO_STRING_LENGTH = 5;        // according to "GTN Flight Plan and User Waypoint transfer.pdf". Paragraph 2.4 En Route. ":F:<Waypoint identifier, up to five characters>
  std::string  flight_plan                = "FPN/RI"; // line prefix

  for (auto& navInfo : inNavList)
  {

    switch (navInfo.navType)
    {
      case xplm_Nav_Airport:
      case xplm_Nav_NDB:
      case xplm_Nav_Fix:
      case xplm_Nav_VOR:
      case xplm_Nav_DME:
      {
        const std::string icao = Utils::stringToUpper(navInfo.getID());
        if (!icao.empty() && icao.length() <= MAX_GTN_ICAO_STRING_LENGTH) // only add ICAOs that are shorter than 6 characters and not empty
          flight_plan += mxconst::get_RXP_FPEI_FLIGHT_PLAN_SEGMENT() + icao;    // store ICAO name
        #ifndef RELEASE
        else
          Log::logMsgWarn("Found invalid GTN icao: " + icao + ". Will skip it and use lat/lon instead.");
        #endif // !RELEASE
        [[fallthrough]];
      }
        //      [[fallthrough]] // we also want to add latitude/longitude information
      default:
      {
        // Handle xplm_Nav_LatLon but also add the coordinates for any NavAid at hand
        #ifndef RELEASE
        Log::logMsgNoneCR("[lat] angle: " + mxUtils::formatNumber<float>(navInfo.lat, 6));
        #endif
        std::string lat_s = Utils::DegreesMinutesSecondsLat_XP((double)navInfo.lat);
        #ifndef RELEASE
        Log::logMsgNoneCR(" after:" + lat_s + ", angle: " + mxUtils::formatNumber<float>(navInfo.lon, 6) + " ");
        #endif
        std::string lon_s = Utils::DegreesMinutesSecondsLon_XP((double)navInfo.lon);
        #ifndef RELEASE
        Log::logMsg(" after:" + lon_s);
        #endif

        flight_plan += fmt::format("{}{}{}", mxconst::get_RXP_FPEI_FLIGHT_PLAN_SEGMENT(), lat_s, lon_s);
      }
      break;

    } // end switch

  } // end loop over all NavAids

  return flight_plan;
}

// -------------------------------------

std::string
data_manager::prepare_flight_plan_for_XPLN11(std::deque<NavAidInfo>& inNavList)
{
  constexpr double REGECT_WP_DISTANCE_OF_GUESSED_NAME_NM = 0.5; // ~750 meters

  std::string       flight_plan;
  std::string       header      = "I\n1100 Version\nCYCLE " + Utils::get_nav_dat_cycle() + "\n"; // Header
  std::string       waypoints_s; // Waypoints
  const auto        NUMENR{ inNavList.size() };
  const std::string NUMENR_S              = "NUMENR " + mxUtils::formatNumber<size_t>(NUMENR) + "\n";
  const std::string DIRECT                = "DRCT";
  const std::string AIRPORT_DEST          = "ADES";
  const std::string NON_AIRPORT_DEST      = "DES";
  const std::string AIRPORT_DEPARTURE     = "ADEP";
  const std::string NON_AIRPORT_DEPARTURE = "DEP";
  const std::string WP_S                  = "WP";

  size_t counter = 0;
  for (auto& navInfo : inNavList)
  {
    std::string icao = mxUtils::remove_char_from_string(Utils::stringToUpper(navInfo.getID()), ' '); // remove any space from the name of the icao
    counter++;

    // add departure arrival to header
    bool b_isItAnAirport = Utils::is_it_an_airport(icao);

    if (!b_isItAnAirport) // guess if we are near other type of NavAid
    {
      // v24.02.7b replaces the old code "get_nearest_guessed_navaid_based_on_coordinate()"
      mxVec2f navPos(navInfo.lat, navInfo.lon);
      navInfo = getPlaneAirportOrNearestICAO(true, navPos.lat, navPos.lon);
      // Copy back original position
      navInfo.lat = navPos.lat;
      navInfo.lon = navPos.lon;
      if (navInfo.flag_navDataFetchedFromDB && navInfo.navRef != XPLM_NAV_NOT_FOUND) // did we found nav aid in a boundary in a valid ICAO ?
      {
        icao            = navInfo.getID();
        b_isItAnAirport = true;
      }
      else // fallback to original code and check nearest airport but do not flag as airport
      {
        mx_wp_guess_result result = get_nearest_guessed_navaid_based_on_coordinate(navPos); // use XPSDK - default code
        if (result.distance_d <= REGECT_WP_DISTANCE_OF_GUESSED_NAME_NM)
        {
          navInfo.setID(result.name);
          navInfo.setName(result.name);
          navInfo.navRef  = result.nav_ref;
          navInfo.navType = result.nav_type;

          icao = result.name;
        }
      }
    }

    // if (!b_isItAnAirport)  // v3.0.303.2 use WPn(lat/lon) instead of just "lat/lon"
    //   icao = (Utils::isStringIsValidArithmetic(icao)) ? (WP_S + mxUtils::formatNumber<size_t>(counter)) : icao; // if icao holds lat/lon string then use WPn name
    if (!(b_isItAnAirport) * Utils::isStringIsValidArithmetic(icao))  // v3.0.303.2 use WPn(lat/lon) instead of just "lat/lon"
      icao = (WP_S + mxUtils::formatNumber<size_t>(counter)); // if icao holds lat/lon string then use WPn name


    if (counter == 1)
    {
      if (b_isItAnAirport)
        header += fmt::format ("{} {}\n", AIRPORT_DEPARTURE, icao); // ADES - airport destination
      else
        header += fmt::format ("{} {}\n", NON_AIRPORT_DEPARTURE, icao); // header.append(NON_AIRPORT_DEPARTURE + " " + icao + "\n"); // Not an airport should be DES

    }
    else if (counter == NUMENR)
    {
      if (b_isItAnAirport)
        header += fmt::format ("{} {}\n", AIRPORT_DEST, icao); //header.append(AIRPORT_DEST + " " + icao + "\n"); // ADES - airport destination
      else
        header += fmt::format ("{} {}\n", NON_AIRPORT_DEST, icao); //header.append(NON_AIRPORT_DEST + " " + icao + "\n"); // // Not an airport should be DES
    }

    // Handle waypoints
    switch (navInfo.navType)
    {
      case xplm_Nav_Airport:
      {
        waypoints_s += fmt::format("{} {} {} {} {} {} \n", 1, icao, ((counter == 1) ? AIRPORT_DEPARTURE : (counter == NUMENR) ? AIRPORT_DEST : DIRECT), 0, navInfo.getLat(), navInfo.getLon());
      }
      break;
      case xplm_Nav_NDB:
      {
        waypoints_s += fmt::format("{} {} {} {} {} {} \n", 2, icao, ((counter == 1) ? NON_AIRPORT_DEPARTURE : (counter == NUMENR) ? NON_AIRPORT_DEST : DIRECT), 0, navInfo.getLat(), navInfo.getLon()); // Not an airport should be DES code, I also changed first enroute to DEP from ADEP
      }
      break;
      case xplm_Nav_VOR:
      {
        waypoints_s += fmt::format("{} {} {} {} {} {} \n", 3, icao, ((counter == 1) ? NON_AIRPORT_DEPARTURE : (counter == NUMENR) ? NON_AIRPORT_DEST : DIRECT), 0, navInfo.getLat(), navInfo.getLon()); // Not an airport should be DES code, I also changed first enroute to DEP from ADEP
      }
      break;
      case xplm_Nav_Fix:
      {
        waypoints_s += fmt::format("{} {} {} {} {} {} \n", 11, icao, ((counter == 1) ? NON_AIRPORT_DEPARTURE : (counter == NUMENR) ? NON_AIRPORT_DEST : DIRECT), 0, navInfo.getLat(), navInfo.getLon()); // Not an airport should be DES code, I also changed first enroute to DEP from ADEP
      }
      break;
      default: // unnamed lat/lon and other types
      {
        waypoints_s += fmt::format("{} {} {} {} {} {} \n", 28, icao, ((counter == 1) ? NON_AIRPORT_DEPARTURE : (counter == NUMENR) ? NON_AIRPORT_DEST : DIRECT), 0, navInfo.getLat(), navInfo.getLon()); // Not an airport should be DES code, I also changed first enroute to DEP from ADEP
      }
      break;
    } // end switch
  }

  flight_plan = header + NUMENR_S + waypoints_s;

  return flight_plan;
}

// -------------------------------------

IXMLNode
data_manager::prepare_flight_plan_for_LTLNAV (const std::deque<NavAidInfo>& inNavList)
{
  // the little nav map
  // IXMLNode xTarget;
  auto xTarget = IXMLNode::createXMLTopNode("xml", TRUE);
  xTarget.addAttribute(mxconst::get_ATTRIB_VERSION().c_str(), "1.0");
  xTarget.addAttribute("encoding", "UTF-8");

  IXMLNode xRoot = xTarget.addChild("LittleNavmap");
  xRoot.addAttribute("xmlns", std::string("Created by Mission-X - v" + std::string(PLUGIN_REVISION)).c_str());
  IXMLNode xFlightPlan = xRoot.addChild("Flightplan");

  IXMLNode xHeader = xFlightPlan.addChild ("Header");
  xHeader.addChild("FlightplanType").addText("VFR"); // currently all FPLN are VFR
  xHeader.addChild("CruisingAlt").addText("");
  xHeader.addChild("CreationDate").addText("");
  xHeader.addChild("FileVersion").addText("1.0");
  xHeader.addChild("ProgramName").addText(std::string("Mission-X").c_str());
  xHeader.addChild("ProgramVersion").addText(std::string(PLUGIN_REVISION).c_str());
  xHeader.addChild("Documentation").addText("");

  IXMLNode xWayPoints = xFlightPlan.addChild("Waypoints");

  // Loop over NavAids
  int         seq_i = 0;
  std::string icao;
  for (auto navInfo : inNavList)
  {
    IXMLNode wp = xWayPoints.addChild("Waypoint");

    switch (navInfo.navType)
    {
      case xplm_Nav_Airport:
      case xplm_Nav_LatLon:
      case xplm_Nav_NDB:
      case xplm_Nav_Fix:
      default:
      {
        std::string ident_s = "WP" + Utils::formatNumber<int>(seq_i);
        std::string name;
        if (navInfo.getSomeName().empty()
           || (navInfo.getSomeName().front() == '+' || navInfo.getSomeName().front() == '-') )
          name = ident_s;
        else
          name = navInfo.getSomeName();

        if (navInfo.getID().empty()
          || (navInfo.getID().front() == '+' || navInfo.getID().front() == '-')
          )
        {
          // use original ident_s value
        }
        else
          ident_s = navInfo.getID();

        wp.addChild("Name").addText(name.c_str());
        wp.addChild("Ident").addText(ident_s.c_str());
        wp.addChild("Type").addText("USER");

        auto xPos = wp.addChild("Pos");
        xPos.addAttribute("Lon", navInfo.getLon().c_str());
        xPos.addAttribute("Lat", navInfo.getLat().c_str());
        xPos.addAttribute("Alt", "");

      }
      break;
    } // end switch

    seq_i++;
  } // end loop over navAids

  return xTarget.deepCopy();
}


// -------------------------------------

IXMLNode
data_manager::prepare_flight_plan_for_GNS530_RXP (const std::deque<NavAidInfo>& inNavList)
{
  // Since X-Plane do not have country/Region code for all airports and since we do not have internal database for that, we will only use waypoints
  // IXMLNode xTarget;
  auto xTarget = IXMLNode::createXMLTopNode("xml", TRUE);
  xTarget.addAttribute(mxconst::get_ATTRIB_VERSION().c_str(), "1.0");
  xTarget.addAttribute("encoding", "UTF-8");

  // flight-plan
  IXMLNode xFlightPlan = xTarget.addChild("flight-plan");
  xFlightPlan.addAttribute("xmlns", std::string("Created by Mission-X - v" + std::string(PLUGIN_REVISION)).c_str());
  xFlightPlan.addChild("created").addText("");
  IXMLNode xWayPoints = xFlightPlan.addChild("waypoint-table");
  IXMLNode xRoute     = xFlightPlan.addChild("route");
  xRoute.addChild("route-name").addText("missionx generated");
  xRoute.addChild("flight-plan-index").addText("1");

  // Loop over NavAids
  int         seq_i = 0;
  std::string icao;
  for (auto navInfo : inNavList)
  {

    IXMLNode wp = xWayPoints.addChild("waypoint");
    IXMLNode rp = xRoute.addChild("route-point");

    switch (navInfo.navType)
    {
      case xplm_Nav_Airport:
      case xplm_Nav_LatLon:
      {
        std::string ident_s = "WP" + Utils::formatNumber<int>(seq_i);
        wp.addChild("identifier").addText(ident_s.c_str());
        wp.addChild("type").addText("USER WAYPOINT");
        rp.addChild("waypoint-identifier").addText(ident_s.c_str());
        rp.addChild("waypoint-type").addText("USER WAYPOINT");
      }
      break;
      case xplm_Nav_NDB:
      {
        icao = Utils::stringToUpper(navInfo.getID());
        wp.addChild("identifier").addText(icao.c_str());
        wp.addChild("type").addText("NDB");
        rp.addChild("waypoint-identifier").addText(icao.c_str());
        rp.addChild("waypoint-type").addText("NDB");
      }
      break;
      case xplm_Nav_Fix:
      {
        icao = Utils::stringToUpper(navInfo.getID());
        wp.addChild("identifier").addText(icao.c_str());
        wp.addChild("type").addText("INT");
        rp.addChild("waypoint-identifier").addText(icao.c_str());
        rp.addChild("waypoint-type").addText("INT");
      }
      break;
      default:
      {
        std::string ident_s = "WP" + Utils::formatNumber<int>(seq_i);
        wp.addChild("identifier").addText(ident_s.c_str());
        wp.addChild("type").addText("UNKNOWN WAYPOINT");
        rp.addChild("waypoint-identifier").addText(ident_s.c_str());
        rp.addChild("waypoint-type").addText("UNKNOWN WAYPOINT");
      }
      break;
    } // end switch


    // Generic waypoint data to add
    wp.addChild("country-code").addText("");
    wp.addChild("lat").addText(navInfo.getLat().c_str());
    wp.addChild("lon").addText(navInfo.getLon().c_str());
    if (navInfo.navType == xplm_Nav_Airport) // debug airport ICAO
      wp.addChild("comment").addText(std::string("ICAO: " + navInfo.getID()).c_str());
    else
      wp.addChild("comment").addText("");

    // Generic route information
    rp.addChild("waypoint-country-code").addText("");


    seq_i++;
  } // end loop over navAids

  return xTarget.deepCopy();
}

// -------------------------------------

void
data_manager::clear()
{
  stopMission();

  currentLegName.clear(); // v3.0.147

  mapBrieferMissionList.clear();        // v3.0.137
  mapBrieferMissionListLocator.clear(); // v3.0.140

  selectedMissionKey.clear(); // v3.0.241.1

  init();
}

// -------------------------------------

void
data_manager::stopMission()
{
  draw_script.clear(); // v3.0.224.2

  mapFlightLegs.clear();
  mapObjectives.clear();

  for (auto &dref : mapDref | std::views::values)
  {
    if (dref.arraySize > 0)
      dref.resetArrays();
  }

  mapDref.clear();
  mapTriggers.clear();
  mapInventories.clear();
  planeInventoryCopy.init_plane_inventory(); // v25.03.1 reset plane inventory info.  //init(); // v24.12.2
  externalInventoryCopy.reset_inventory_content(); // v24.12.2
  //data_manager::xmlPlaneInventoryCopy.reset_inventory_content();    // v24.12.2

  mapMessages.clear();

  errStr.clear();
  briefer.clear();

  // data_manager::loadErrors.clear();
  lstLoadErrors.clear(); // v3.305.3 replaces load Errors

  missionSavepointFilePath.clear();
  missionSavepointDrefFilePath.clear();
  mx_global_settings.clear(); // v3.0.241.1

  smPropSeedValues.clear();

  map3dInstances.clear();
  map3dObj.clear();
  listDisplayStatic3dInstances.clear();
  listDisplayMoving3dInstances.clear();

  xmlMappingNode.deleteNodeContent();
  xmlMappingNode = IXMLNode(); // .emptyIXMLNode;

  xmlGPS.deleteNodeContent();
  xmlGPS = IXMLNode(); // .emptyIXMLNode;

  xmlLoadedFMS.deleteNodeContent();
  xmlLoadedFMS = IXMLNode(); // .emptyIXMLNode;

  mapCommands.clear();          // v3.0.221.9
  queCommands.clear();          // v3.0.221.9
  queCommandsWithTimer.clear(); // v3.0.221.15

  current_metar_file_s.clear();   // v3.0.223.1
  metar_file_to_inject_s.clear(); // v3.0.223.1

  xmlChoices.deleteNodeContent(); // v3.0.231.1
  xmlChoices = IXMLNode();        // ::emptyIXMLNode; // v3.0.231.1
  mxChoice.init();                // v3.0.231.1

  maps2d_to_display.clear();

  mapFailureTimers.clear();                // v3.0.253.7
  lowestFailTimerName_s.clear(); // v3.0.253.7

  db_close_database(db_stats); // v3.303.8.3

  mapInterpolDatarefs.clear(); // v3.303.13

  gather_stats.reset();                   // v3.303.14
  strct_currentLegStats4UIDisplay.init(); // v3.303.14
  vecPreviousLegStats4UIDisplay.clear();  // v3.303.14

  listOfMessageStoryMessages.clear(); // v3.305.2

  missionx::data_manager::active_acf.clear ();
  missionx::data_manager::prev_acf.clear ();
}

// -------------------------------------

void
data_manager::init()
{
  // set mandatory folders that won't change during plugin state
  init_static(); // v3.303.8

  preparePluginFolders(); // v3.0.241.1

  OptimizeAptDat::setAptDataFolders(Utils::getCustomSceneryRelativePath(), Utils::readAttrib(mx_folders_properties.node, mxconst::get_FLD_DEFAULT_APTDATA_PATH(), ""), mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", errStr)); // v3.0.219.9

  msgSeq = 0;

  set_found_missing_3D_object_files(false); // v3.0.255.3
}

// -------------------------------------

void
data_manager::init_static()
{
  const dataref_const dc;

  // #ifdef USE_CURL
  curl = curl_easy_init();
  if (!curl)
  {
    throw std::runtime_error("[data_manager] Couldn't initialize curl");
  }
  // #endif

  // if (!data_manager::mapFonts.empty())
  //   data_manager::mapFonts.clear();

  xmlMappingNode = IXMLNode::emptyIXMLNode;

  draw_script.clear();

  xplane_ver_i                 = XPLMGetDatai(dc.dref_xplane_version_i);
  xplane_using_modern_driver_b = XPLMGetDatai(dc.dref_xplane_using_modern_driver_b);

  prop_userDefinedMission_ui.node.updateName(mxconst::get_ELEMENT_UI_PROPERTIES().c_str());

  listOfMessageStoryMessages.clear();

  mapQueries.clear();
}

// -------------------------------------

void
data_manager::release_static()
{
  // #ifdef USE_CURL
  if (curl)
  {
    curl_easy_cleanup(curl);
  }
  // #endif
}


// -------------------------------------
void
data_manager::preparePluginFolders()
{
  mx_folders_properties.setStringProperty(mxconst::get_PROP_XPLANE_PLUGINS_PATH(), Utils::getXplaneFullPluginsPath());

  mx_folders_properties.setStringProperty(mxconst::get_PROP_MISSIONX_PATH(), Utils::getPluginDirectoryWithSep(PLUGIN_DIR_NAME));                                                                                                              // x-plane plugin path
  mx_folders_properties.setStringProperty(mxconst::get_PROP_MISSIONX_BITMAP_PATH(), mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", errStr) + "libs" + XPLMGetDirectorySeparator() + "bitmap"); // plugin bitmap folder path

  mx_folders_properties.setStringProperty(mxconst::get_FLD_MISSIONX_SAVE_PATH(), Utils::getRelativePluginsPath() + "/" + PLUGIN_DIR_NAME + "/save");
  mx_folders_properties.setStringProperty(mxconst::get_FLD_MISSIONX_LOG_PATH(), Utils::getPluginDirectoryWithSep(PLUGIN_DIR_NAME) + "missionx.log");

  mx_folders_properties.setStringProperty(mxconst::get_FLD_MISSIONS_ROOT_PATH(), getMissionsRootPath()); // v3.0.129

  // v3.0.217.1 Random folders
  mx_folders_properties.setStringProperty(mxconst::get_FLD_RANDOM_MISSION_PATH(), mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONS_ROOT_PATH(), "", errStr) + XPLMGetDirectorySeparator() + "random"); // v3.0.217.1
  mx_folders_properties.setStringProperty(mxconst::get_FLD_RANDOM_TEMPLATES_PATH(), mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", errStr) + "templates");                              // v3.0.217.1

  // v3.0.219.9 AptDat folders
  mx_folders_properties.setStringProperty(mxconst::get_FLD_CUSTOM_SCENERY_FOLDER_PATH(), Utils::getCustomSceneryRelativePath());    // "Custom Scenery". This is not absolute path
  if (xplane_ver_i < XP12_VERSION_NO)                                                                               // less than 120000 means it is xp11
    mx_folders_properties.setStringProperty(mxconst::get_FLD_DEFAULT_APTDATA_PATH(), "Resources/default scenery/default apt dat/"); // XP11 "apt.dat". relative folder path
  else
  {
    mx_folders_properties.setNodeStringProperty(mxconst::get_FLD_DEFAULT_APTDATA_PATH(), "Global Scenery/Global Airports/Earth nav data/"); // XP12 "apt.dat". relative folder path
  }
}

// -------------------------------------

std::string
data_manager::getMissionsRootPath()
{
  std::string missionsRootPath;
  missionsRootPath.clear();

  #ifdef MX_EXE
  missionsRootPath = std::string(mxconst::CUSTOM_MISSIONX_PATH);
  #else
  char sysPath[2048] = {};

  XPLMGetSystemPath (sysPath);
  missionsRootPath.append(sysPath).append("Custom Scenery").append(XPLMGetDirectorySeparator()).append("missionx");

  #endif

  return missionsRootPath;
}



// -------------------------------------

void
data_manager::read_element_mapping(const std::string& inFile)
{
  std::string errMsg;
  errMsg.clear();

  IXMLDomParser    iDomTemplate;
  const ITCXMLNode xRootTemplate = iDomTemplate.openFileHelper ( inFile.c_str (), mxconst::get_MAPPING_ROOT_DOC().c_str (), &errMsg ); // parse xml into ITCXMLNode
  if (!errMsg.empty())
  {
    Log::logMsgErr(errMsg);
    return;
  }

  // reset node and fill with new data
  xmlMappingNode.deleteNodeContent(); //  = data_manager::xmlMappingNode.emptyNode();
  xmlMappingNode = xRootTemplate.deepCopy();

  #ifndef RELEASE
  Log::logMsg("[read element mapping] End Read Mappings");
  #endif
}

// -------------------------------------

Point
data_manager::getCameraLocationTerrainInfo()
{
  double gHeading, groundElev_mt; // holds plane position
  double x, y, z;
  double outX, outY, outZ;
  x = y = z = 0.0;
  outX = outY = outZ         = 0.0;
  XPLMDataRef gHeadingPsiRef = nullptr; // heading - true north


  x = XPLMGetDataf(drefConst.dref_camera_view_x_f);
  y = XPLMGetDataf(drefConst.dref_camera_view_y_f);
  z = XPLMGetDataf(drefConst.dref_camera_view_z_f);

  XPLMLocalToWorld(x, y, z, &outX, &outY, &outZ);
  outZ *= meter2feet; // v24.03.2 fix elevation to be feet and not meter
  Point           p = Point(outX, outY, outZ);
  XPLMProbeResult outResult;
  groundElev_mt = Point::getTerrainElevInMeter_FromPoint(p, outResult);
  p.setElevationMt(groundElev_mt);

  // read longitude/latitude info
  gHeading = XPLMGetDataf(gHeadingPsiRef);
  p.setHeading ( gHeading );

  const std::string formatedOutput = "CAMERA VIEW: <point lat=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getLat(), 8) + mxconst::get_QM() + " long=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getLon(), 8) + mxconst::get_QM() + " elev_ft=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getElevationInFeet(), 2) + mxconst::get_QM() +
                               " \ttemplate=\"\"  loc_desc=\"\" type=\"\"    heading_true_psi=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getHeading(), 2) + mxconst::get_QM() + " groundElev=" + mxconst::get_QM() + Utils::formatNumber<double>(groundElev_mt * meter2feet) + mxconst::get_QM() + " ft  replace_lat=\"" + Utils::formatNumber<double>(p.getLat(), 8) + "\"" + " replace_long=\"" +
                               Utils::formatNumber<double>(p.getLon(), 8) + "\" />";
  Log::logMsgNone(formatedOutput);


  return p;
}



// -------------------------------------

bool
data_manager::validateObjective(Objective& inObj, std::string& outError, std::string& outMsg)
{
  // go over tasks and find if there is at least 1 task that is mandatory and not dependent. Exception: Objective has invalidated, flagged as not valid, so it can't turn to valid.
  // Check if all tasks with dependent settings will have the corresponding task available in the Objective. If not, warn and flag task as "undefined"
  // a mandatory task that pTaskName on other task, must also be mandatory. We will fix this during validation and warn designer.

  outError.clear();
  outMsg.clear();

  // member attributes and flags
  inObj.isValid = true;

  errStr.clear();
  std::string dependErrors;
  dependErrors.clear();
  const std::string objName = inObj.name;

  auto cLogNode = Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(xLoadInfoNode.node, mxconst::get_ELEMENT_OBJECTIVE(),  mxconst::get_ATTRIB_NAME(), objName);
  assert(cLogNode.isEmpty() == false && fmt::format("[{}] XML Log Node must not be empty.", __func__).c_str());

  // check function dependency state and objective dependency organization
  if (inObj.initTaskDependencyInformation())
  {
    for ( auto &currentTask : inObj.mapTasks | std::views::values ) // v3.0.303.7 rewrite instead of iterators
    {

      const std::string cTaskName   = Utils::readAttrib(currentTask.node,  mxconst::get_ATTRIB_NAME(), "");
      const std::string trigName    = Utils::readAttrib(currentTask.node, mxconst::get_ATTRIB_BASE_ON_TRIGGER(), "");
      const std::string commandPath = Utils::readAttrib(currentTask.node, mxconst::get_ATTRIB_BASE_ON_COMMAND(), "");
      std::string       scriptName  = Utils::readAttrib(currentTask.node, mxconst::get_ATTRIB_BASE_ON_SCRIPT(), "");


      // Check task validity
      if (currentTask.task_type == mx_task_type::undefined_task)
      {
        const auto txt = fmt::format("found UNDEFINED STATE task: '{}'.", cTaskName); // v3.305.3
        Utils::xml_add_error_child(cLogNode, txt);                                    // v3.305.3

        outError += "In Objective: \"" + objName + "\", " + txt;
        continue; // skip this task
      }

      // check if trigger was defined
      if (Utils::readBoolAttrib(currentTask.node, mxconst::get_ATTRIB_IS_PLACEHOLDER(), false)) // v3.0.303.7
      {
        continue; // if placeholder, then task is valid and we should fetch the next task
      }
      else if (!trigName.empty())
      {
        if (!Utils::isElementExists(mapTriggers, trigName))
        {
          //          countUndefinedTasks++;
          inObj.mapTasks[cTaskName].task_type = mx_task_type::undefined_task;

          const std::string txt = fmt::format("Trigger: '{}', does not exists in the trigger list. Please fix. Task: '{}', marked as undefined.", trigName, cTaskName); // v3.305.3
          Utils::xml_add_error_child(cLogNode, txt); // v3.305.3

          inObj.mapTasks[cTaskName].errReason += txt;
          outError += inObj.mapTasks[cTaskName].errReason;
        }
        else
        {
          mapTriggers[trigName].isLinked = true;
          mapTriggers[trigName].setNodeProperty<bool>(mxconst::get_PROP_IS_LINKED(), mapTriggers[trigName].isLinked); // store state for "save/load checkpoint
          const std::string_view linked_name_vu = "[" + cTaskName + "]";
          if (mapTriggers[trigName].linkedTo.find ( linked_name_vu ) == std::string::npos)
          {
            mapTriggers[trigName].linkedTo += linked_name_vu; //"[" + cTaskName + "]";
            mapTriggers[trigName].setNodeStringProperty(mxconst::get_PROP_LINKED_TO(), mapTriggers[trigName].linkedTo);
          }


          // Add mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC() to trigger. Helps in the "test trigger" code, to keep all tests in one function
          if (currentTask.node.isAttributeSet(mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str()))
          {
            int sec_i = currentTask.getAttribNumericValue<int>(mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), 0, errStr);

            if (sec_i > 0)
            {
              mapTriggers[trigName].setNodeProperty<int>(mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), sec_i);
            }
          }

          // v3.0.221.11 set if timer should be cumulative
          bool flag_cumulative = Utils::readBoolAttrib(currentTask.node, mxconst::get_ATTRIB_CUMULATIVE_TIMER_FLAG(), false);
          mapTriggers[trigName].setCumulative_flag(flag_cumulative);
        }
      }
      else if (!scriptName.empty())
      {
        static std::string mxFuncCall;
        mxFuncCall.clear();

        scriptName = Utils::extractBaseFromString(scriptName, ".", &mxFuncCall);

        if (!Utils::isElementExists(script_manager::mapScripts, scriptName))
        {
          inObj.mapTasks[cTaskName].task_type = mx_task_type::undefined_task;

          const std::string txt = fmt::format("Script: '{}', does not exists in the script list. Please fix Task: '{}', it was marked as undefined.", scriptName, cTaskName); // v3.305.3
          Utils::xml_add_error_child(cLogNode, txt); // v3.305.3
          inObj.mapTasks[cTaskName].errReason += txt;

          outError += inObj.mapTasks[cTaskName].errReason;
          continue;
        }
        else
        { // check if based script
          base_script bScript = script_manager::mapScripts[scriptName];

          if (!bScript.script_is_valid)
          {
            inObj.mapTasks[cTaskName].task_type = mx_task_type::undefined_task;
            // inObj.mapTasks[cTaskName].errReason += "Script: " + mxconst::get_QM() + scriptName + mxconst::get_QM() + ", is not valid. Please fix it. In task: " + mxconst::get_QM() + cTaskName + mxconst::get_QM() + ", marked as undefined.";

            const std::string txt = fmt::format("Script: '{}', is not valid. Please fix it. In task: '{}', it was marked as undefined.", scriptName, cTaskName); // v3.305.3
            Utils::xml_add_error_child(cLogNode, txt.data());                                                                                                    // v3.305.3

            inObj.mapTasks[cTaskName].errReason += txt;

            outError += inObj.mapTasks[cTaskName].errReason;
            continue;
          }
        }
      }
      else if (!commandPath.empty()) // v3.0.221.10
      {
        if (!currentTask.command_ref.setAndListenToCommand(commandPath))
        {
          inObj.mapTasks[cTaskName].task_type = mx_task_type::undefined_task;

          // inObj.mapTasks[cTaskName].errReason += "Command: " + mxconst::get_QM() + commandPath + mxconst::get_QM() + ", is not valid. Please fix. Task: " + mxconst::get_QM() + cTaskName + mxconst::get_QM() + ", marked as undefined.";
          const std::string txt = fmt::format("Command: '{}', is not valid. Please fix it. Task: '{}', is marked as undefined.", commandPath, cTaskName); // v3.305.3
          Utils::xml_add_error_child(cLogNode, txt); // v3.305.3

          inObj.mapTasks[cTaskName].errReason += txt;

          outError += inObj.mapTasks[cTaskName].errReason;
          continue;
        }
      }



    } // end loop over all tasks
  }
  else
  {
    inObj.isValid = false;
    outError += inObj.errReason;
  }

  return inObj.isValid;
}

// -------------------------------------

bool
data_manager::validateFlightLegs(std::string& outError, std::string& outMsg)
{
  // 1. Loop over all FlightLegs and validate if next_leg exists
  // 1.1 validate if description exists
  // 2. loop over all Legs objectives and check if one of them is mandatory and if has TARGET_POI

  bool                   flag_found          = false;
  bool                   flagAllLegsAreValid = true;
  std::list<std::string> listEraseFromLeg; // holds trigger names to erase from global leg list

  outError.clear();
  outMsg.clear();

  if (mapFlightLegs.empty()) // if no Goals where defined
    return false;

  Log::logMsg("\n>>> Start Validating Flight Plan <<<<\n");

  for (auto& [legName, leg] : mapFlightLegs)
  {
    auto cLogNode = xLoadInfoNode.addChild(mxconst::get_ELEMENT_LEG(),  mxconst::get_ATTRIB_NAME(), legName, ""); // v3.305.3

    IXMLNode xDesc = leg.node.getChildNode(mxconst::get_ELEMENT_DESC().c_str()); // v3.303.14

    std::string flightLegName = leg.getName();
    std::string desc          = Utils::xml_read_cdata_node(xDesc, Utils::xml_read_cdata_node(leg.node, "")); // v3.305.3 fixing wrong info that there is no description, we take into consideration dorect description // v3.303.14 fixed desc warning //   leg.getAttribStringValue(mxconst::get_ELEMENT_DESC(), "" , data_manager::errStr);
    std::string nextLeg       = leg.getAttribStringValue(mxconst::get_ATTRIB_NEXT_LEG(), "", errStr);
    std::string target_pos;
    target_pos.clear();

    if (!leg.xmlSpecialDirectives_ptr.isEmpty())
    {
      flag_found = false;
      target_pos = Utils::xml_get_attribute_value(leg.xmlSpecialDirectives_ptr, mxconst::get_ATTRIB_TARGET_POS(), flag_found);
    }


    if (desc.empty())
    {
      // v3.305.3
      const std::string txt = fmt::format("Flight Leg: \"{}\", has no description.", legName);
      Utils::xml_add_info_child(cLogNode, txt);

      outMsg += txt;
    }
    // invalidate if next leg does not exists in the Flight Leg list
    if (!nextLeg.empty() && !Utils::isElementExists(mapFlightLegs, nextLeg))
    {
      flagAllLegsAreValid = false; // invalidate Flight Legs

      // v3.305.3
      const std::string txt = fmt::format("Flight Leg: \"{}\", has attribute '{}' with the value: \"{}\" which does not exists. Please fix leg settings.", legName, mxconst::get_ATTRIB_NEXT_LEG(), nextLeg);
      Utils::xml_add_warning_child(cLogNode, txt);

      outMsg += txt;
    }

    bool foundMandatoryObjective = false;
    if (!leg.getIsDummyLeg())
    {
      // v3.0.241.1 loop over all objectives list in Leg and remove those that are not in mapObjectives (meaning: not valid)
      #ifdef IBM
      for (auto it = leg.listObjectivesInFlightLeg.begin(); it != leg.listObjectivesInFlightLeg.end();)
      #else
      auto it     = leg.listObjectivesInFlightLeg.begin();
      auto it_end = leg.listObjectivesInFlightLeg.end();
      while (it != it_end)
      #endif
      {
        if (!Utils::isElementExists(mapObjectives, (*it)))
        {
          flagAllLegsAreValid = false;

          // v3.305.3
          const std::string txt = fmt::format("Flight Leg: \"{}\",has an objective name: '{}' that is not listed inside the <objectives> list. Please fix your mission.", legName, (*it));
          Utils::xml_add_error_child(cLogNode, txt);

          Log::add_missionLoadError(txt);
          it = leg.listObjectivesInFlightLeg.erase(it); // erase returns the next iterator address
        }
        else
          ++it;
      }


      // Loop over all objectives and check if one of them is mandatory
      for (auto& iterObj : leg.listObjectivesInFlightLeg)
      {
        const std::string objName = iterObj; // string - for readability

        // Objective obj;
        if (Utils::isElementExists(mapObjectives, objName))
        {
          if (mapObjectives[objName].isMandatory && mapObjectives[objName].isValid)
          {
            foundMandatoryObjective = true;
            leg.hasMandatory        = true;
            break; // v3.0.241.1 deprecated this, since we want to also validate that all objectives are present
          }
        }
        else
        {
          // v3.305.3
          const std::string txt = fmt::format("Flight Leg: \"{}\",has an objective name: '{}' that is not listed inside the <objectives> list. Please fix your mission.", legName, objName);
          Utils::xml_add_error_child(cLogNode, txt);

          Log::add_missionLoadError(txt);
        }
      }
    } // end if Leg is not Dummy

    // v3.303.14
    if (flagAllLegsAreValid == false)
    {

      const std::string txt = fmt::format("Not all flight legs are valid.");
      Utils::xml_add_error_child(xLoadInfoNode.node, txt);

      Log::add_missionLoadError(txt);
      return flagAllLegsAreValid;
    }

    // v3.0.223.4 create leg_messages
    parse_leg_dynamic_messages(leg);

    // v3.0.241.1
    parse_leg_DisplayObjects(leg);


    // loop over triggers
    listEraseFromLeg.clear();

    // remove triggers that do not exists or are linked to tasks and should not be global
    for (const auto& trigName : leg.listTriggers)
    {
      // auto trigName = iterTrig;
      if (!Utils::isElementExists(mapTriggers, trigName))
        listEraseFromLeg.push_back(trigName);
      else if (mapTriggers[trigName].isLinked)
        listEraseFromLeg.push_back(trigName);
    }
    // erase non existing triggers
    for (const auto& s : listEraseFromLeg)
    {
      leg.listTriggers.remove(s);
    }

    // v3.305.2 store all "rad" and "poly" triggers in listTriggersByDistance      this->listTriggersByDistance.push_back(trigName); // v3.305.2
    for (auto& trigName : leg.listTriggers)
    {
      if ((mxconst::get_TRIG_TYPE_RAD() == mapTriggers[trigName].getTriggerType()) + (mxconst::get_TRIG_TYPE_POLY() == mapTriggers[trigName].getTriggerType()))
      {
        leg.listTriggersByDistance.push_back(trigName);
        continue;
      }

      leg.listTriggersOthers.push_back(trigName);
    }


    if (!leg.getIsDummyLeg()) // skip objective validation for "dummy" <leg>s
    {
      if (!foundMandatoryObjective)
      {
        // v3.305.3
        const std::string txt = fmt::format("Flight Leg: \"{}\", has no mandatory Objective.", legName);
        Utils::xml_add_error_child(cLogNode, txt);

        outMsg += txt;
        leg.hasMandatory    = false;
        flagAllLegsAreValid = false; // v3.303.14
      }

      // try to figure target position
      bool flag_mandatory_task_found = false;
      if (foundMandatoryObjective && target_pos.empty())
      {
        // loop over all objectives and their tasks. Find the first objective that has a mandatory task that is based on trigger and the trigger pCenter is defined (pCenter.pointState != missionx::mx_point_state::point_undefined)
        for (const auto& objectiveName : leg.listObjectivesInFlightLeg)
        {
          // auto objectiveName = iterObj; // string

          if (flag_mandatory_task_found)
            break;

          // if objective exists and mandatory and objective is valid ?
          if (Utils::isElementExists(mapObjectives, objectiveName) && mapObjectives[objectiveName].isMandatory && mapObjectives[objectiveName].isValid)
          {
            // loop over all object tasks
            for ( auto &task : mapObjectives[objectiveName].mapTasks | std::views::values )
            {
              if (flag_mandatory_task_found)
                break;

              // check task has the attribute mxconst::get_ATTRIB_BASE_ON_TRIGGER() and bool property "ATTRIB_MANDATORY" is true
              if (task.node.isAttributeSet(mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str()) && task.node.isAttributeSet(mxconst::get_ATTRIB_MANDATORY().c_str()))
              {
                std::string trigger_name      = Utils::readAttrib(task.node, mxconst::get_ATTRIB_BASE_ON_TRIGGER(), "");
                bool        flag_is_mandatory = Utils::readBoolAttrib(task.node, mxconst::get_ATTRIB_MANDATORY(), false);

                // v3.0.221.9 check trigger has pCenter
                if (flag_is_mandatory && !trigger_name.empty() && Utils::isElementExists(mapTriggers, trigger_name))
                {
                  if (mapTriggers[trigger_name].pCenter.pointState != mx_point_state::point_undefined)
                  {
                    std::string lat_s        = mxUtils::formatNumber<double>(mapTriggers[trigger_name].pCenter.getLat());
                    std::string lon_s        = mxUtils::formatNumber<double>(mapTriggers[trigger_name].pCenter.getLon());
                    std::string elev_mt_s    = mxUtils::formatNumber<double>(mapTriggers[trigger_name].pCenter.getElevationInFeet(), 2);
                    std::string target_loc_s = fmt::format("{}|{}|{}", lat_s, lon_s, elev_mt_s);
                    if (!leg.xmlSpecialDirectives_ptr.isEmpty())
                      leg.xmlSpecialDirectives_ptr.updateAttribute(target_loc_s.c_str(), mxconst::get_ATTRIB_TARGET_POS().c_str(), mxconst::get_ATTRIB_TARGET_POS().c_str());

                    flag_mandatory_task_found = true;

                    break;
                  }

                } // end validate trigger has valid pCenter
              }   // end if task has mandatory and is based trigger

            } // end loop over objective tasks

          } // end if objective has mandatory and is valid

        } // end loop over objectives in current leg

      } // end - if foundMandatoryObjective && target_pos.empty()

    } // end if Not Dummy LEG

    // v3.0.303 loop over all objectives and search for target_poi/mandatory or a leg with target_lat/lon attributes
    for (const auto& objName_s : leg.listObjectivesInFlightLeg)
    {
      if (search_for_targetPOI(leg.node, objName_s)) // returning false does not mean we did not find target_lat/lon, it just mean that we did not find "is_target_poi" with "lat/lon"
        break;
    }

  } // end loop over all Legs

  // loop over MxPad
  listEraseFromLeg.clear();


  return flagAllLegsAreValid;
}

// -------------------------------------

bool
data_manager::search_for_targetPOI(IXMLNode& legNode, const std::string& objName)
{
  for ( auto &tsk : mapObjectives[objName].mapTasks | std::views::values )
  {
    const bool  isTargetPoi = Utils::readBoolAttrib ( tsk.node, mxconst::get_ATTRIB_IS_TARGET_POI(), false );
    std::string trig_name   = Utils::readAttrib(tsk.node, mxconst::get_ATTRIB_BASE_ON_TRIGGER(), "");
    auto        target_lat  = Utils::readNodeNumericAttrib<double> ( tsk.node, mxconst::get_ATTRIB_TARGET_LAT(), 0.0 );
    auto        target_lon  = Utils::readNodeNumericAttrib<double>(tsk.node, mxconst::get_ATTRIB_TARGET_LON(), 0.0);
    if (isTargetPoi + tsk.isMandatory)
    {
      if (target_lat * target_lon == 0.0)
      {
        if (trig_name.empty())
          continue; // we do
        else if (Utils::isElementExists(mapTriggers, trig_name))
        {
          auto& trig = mapTriggers[trig_name];
          if ((mxconst::get_TRIG_TYPE_RAD() == trig.getTriggerType()) || (mxconst::get_TRIG_TYPE_POLY() == trig.getTriggerType()))
          {
            target_lat = trig.pCenter.getLat();
            target_lon = trig.pCenter.getLon();

            legNode.updateAttribute(trig.pCenter.getLat_s().c_str(), mxconst::get_ATTRIB_TARGET_LAT().c_str(), mxconst::get_ATTRIB_TARGET_LAT().c_str());
            legNode.updateAttribute(trig.pCenter.getLon_s().c_str(), mxconst::get_ATTRIB_TARGET_LON().c_str(), mxconst::get_ATTRIB_TARGET_LON().c_str());

            if (isTargetPoi) // exit loop on first "is_target_poi" found
              return true;
          }
        }
      } // end if <leg target_lat or target_lon> are 0
      else
      {
        legNode.updateAttribute(mxUtils::formatNumber<double>(target_lat, 8).c_str(), mxconst::get_ATTRIB_TARGET_LAT().c_str(), mxconst::get_ATTRIB_TARGET_LAT().c_str());
        legNode.updateAttribute(mxUtils::formatNumber<double>(target_lon, 8).c_str(), mxconst::get_ATTRIB_TARGET_LON().c_str(), mxconst::get_ATTRIB_TARGET_LON().c_str());

        if (isTargetPoi) // exit loop on first "is_target_poi" found
          return true;
      }
    }
    else if (target_lat * target_lon > 0.0) // if this is a task with target_lat/lon defined then store it too on the <leg > element
    {
      legNode.updateAttribute(mxUtils::formatNumber<double>(target_lat, 8).c_str(), mxconst::get_ATTRIB_TARGET_LAT().c_str(), mxconst::get_ATTRIB_TARGET_LAT().c_str());
      legNode.updateAttribute(mxUtils::formatNumber<double>(target_lon, 8).c_str(), mxconst::get_ATTRIB_TARGET_LON().c_str(), mxconst::get_ATTRIB_TARGET_LON().c_str());
    }

  } // end loop over all tasks in objective

  return false;
}

// -------------------------------------

// Prepare mission folder files like sound,object,scripts,metar
void
data_manager::prepare_new_mission_folders ( const GLobalSettings & in_xmlGlobalSettings)
{
  assert(!missionx::data_manager::selectedMissionKey.empty() && "[prepare_new_mission_folders] selectedMissionKey was not initialized."); // v3.0.251.1 b2 abort program if key value is not available. must be fixed first

  const std::string pathToMissionRootFolderInCustomScenery = mapBrieferMissionList[selectedMissionKey].pathToMissionPackFolderInCustomScenery; // v3.0.156
  const std::string pathToMissionFile                      = mapBrieferMissionList[selectedMissionKey].getFullMissionXmlFilePath();

  // root mission folder we already know
  const std::string fldMissionCustom_withSep = pathToMissionRootFolderInCustomScenery + XPLMGetDirectorySeparator();

  // reset main mission folders to be the root mission package folder. This will cover cases where globalSettings.folder_ptr is empty. not set.
  mx_folders_properties.setStringProperty(mxconst::get_ATTRIB_MISSION_PACKAGE_FOLDER_PATH(), fldMissionCustom_withSep);
  mx_folders_properties.setStringProperty(mxconst::get_ATTRIB_SOUND_FOLDER_NAME(), fldMissionCustom_withSep);
  mx_folders_properties.setStringProperty(mxconst::get_ATTRIB_OBJ3D_FOLDER_NAME(), fldMissionCustom_withSep);
  mx_folders_properties.setStringProperty(mxconst::get_ATTRIB_METAR_FOLDER_NAME(), fldMissionCustom_withSep);
  mx_folders_properties.setStringProperty(mxconst::get_ATTRIB_SCRIPT_FOLDER_NAME(), fldMissionCustom_withSep);

  if (!in_xmlGlobalSettings.xFolders_ptr.isEmpty())
  {

    // sound resource path
    std::string fldSoundFolder = Utils::readAttrib(in_xmlGlobalSettings.xFolders_ptr, mxconst::get_ATTRIB_SOUND_FOLDER_NAME(), EMPTY_STRING);
    fldSoundFolder             = (fldSoundFolder.empty()) ? fldMissionCustom_withSep : fldMissionCustom_withSep + fldSoundFolder + XPLMGetDirectorySeparator();
    mx_folders_properties.setStringProperty(mxconst::get_ATTRIB_SOUND_FOLDER_NAME(), fldSoundFolder);

    // mission_obj3d resource path
    std::string fldObj3dFolderPath = Utils::readAttrib(in_xmlGlobalSettings.xFolders_ptr, mxconst::get_ATTRIB_OBJ3D_FOLDER_NAME(), EMPTY_STRING);
    fldObj3dFolderPath             = (fldObj3dFolderPath.empty()) ? fldMissionCustom_withSep : fldMissionCustom_withSep + fldObj3dFolderPath + XPLMGetDirectorySeparator();
    mx_folders_properties.setStringProperty(mxconst::get_ATTRIB_OBJ3D_FOLDER_NAME(), fldObj3dFolderPath);

    // metar resource path
    std::string fldMetarFolderPath = Utils::readAttrib(in_xmlGlobalSettings.xFolders_ptr, mxconst::get_ATTRIB_METAR_FOLDER_NAME(), EMPTY_STRING);
    fldMetarFolderPath             = (fldMetarFolderPath.empty()) ? fldMissionCustom_withSep : fldMissionCustom_withSep + fldMetarFolderPath + XPLMGetDirectorySeparator();
    mx_folders_properties.setStringProperty(mxconst::get_ATTRIB_METAR_FOLDER_NAME(), fldMetarFolderPath);

    // script resource path
    std::string fldDataFolderPath = Utils::readAttrib(in_xmlGlobalSettings.xFolders_ptr, mxconst::get_ATTRIB_SCRIPT_FOLDER_NAME(), EMPTY_STRING);
    fldDataFolderPath             = (fldDataFolderPath.empty()) ? fldMissionCustom_withSep : fldMissionCustom_withSep + fldDataFolderPath + XPLMGetDirectorySeparator();
    mx_folders_properties.setStringProperty(mxconst::get_ATTRIB_SCRIPT_FOLDER_NAME(), fldDataFolderPath);



  } // end read folders info
}

// -------------------------------------

void
data_manager::addDataref(dataref_param inDref)
{
  mapDref.insert(make_pair(inDref.getName(), inDref));
}

// -------------------------------------

void
data_manager::setAbortMission(const std::string& inReason)
{
  mx_global_settings.setNodeStringProperty(mxconst::get_PROP_MISSION_ABORT_REASON(), inReason); // v3.303.11
  queFlcActions.push(mx_flc_pre_command::abort_mission);
}

// -------------------------------------

void
data_manager::prepareInventoryCopies(const std::string& inInventoryName)
{
  // prepare XML copy
  Log::logDebugBO("[prepareInventoryCopies] start\n");
  assert(mxUtils::isElementExists(data_manager::mapInventories, mxconst::get_ELEMENT_PLANE()) && fmt::format("[{}] Plane inventory was not initialized.", __func__).c_str()); // v24.12.2

  externalInventoryCopy.init();
  if (!inInventoryName.empty() && Utils::isElementExists(mapInventories, inInventoryName))
    externalInventoryCopy = mapInventories[inInventoryName]; // v3.0.241.1
  else
    externalInventoryCopy.init();


  //  v24.12.2 Copy Plane inventory
  planeInventoryCopy = mapInventories[mxconst::get_ELEMENT_PLANE()];

  #ifndef RELEASE
  Log::log_to_missionx_log(fmt::format("==========>\nLocal inventory after Coping Plane Inventory:\n[{}\n].\n====> Source Inventory:\n[{}\n]\n<=========", Utils::xml_get_node_content_as_text(planeInventoryCopy.node), Utils::xml_get_node_content_as_text(mapInventories[mxconst::get_ELEMENT_PLANE()].node)));
  #endif


  //data_manager::xmlPlaneInventoryCopy = planeInventoryCopy; // XP11 compatibility. Keep pointer to the copied inventory XML node and not to the original plane xml node.

  // v3.0.221.9 calculate but do not apply to dataref since we did not commit
  // v24.12.2 added "force inventory layout"
  current_plane_payload_weight_f = calculatePlaneWeight(planeInventoryCopy, false, Inventory::opt_forceInventoryLayoutBasedOnVersion_i);

  // v3.0.303.5 store all inventory image file names
  xp_mapInvImages.clear();
  auto const lmbda_read_image_file_names = [&](const IXMLNode& invNode)
  {
    const auto itemNodes_i = invNode.nChildNode(mxconst::get_ELEMENT_ITEM().c_str());

    for (int i1 = 0; i1 < itemNodes_i; ++i1)
    {
      auto node = invNode.getChildNode (mxconst::get_ELEMENT_ITEM().c_str (), i1 );
      if ( const std::string imageFileName = Utils::readAttrib(node,   mxconst::get_PROP_IMAGE_FILE_NAME(), "")
          ; !imageFileName.empty() )
      {
        if ( !Utils::isElementExists(xp_mapInvImages, imageFileName) )
        {
          const mxTextureFile emptyButtonTexture;
          Utils::addElementToMap(xp_mapInvImages, imageFileName, emptyButtonTexture);
        }
      }
    } // end loop over all <item>s
  };  // end lambda

  // Read external inventory
  lmbda_read_image_file_names(externalInventoryCopy.node);
  if (Inventory::opt_forceInventoryLayoutBasedOnVersion_i != XP11_COMPATIBILITY )
  {
    // loop over stations, and read stations items
    for (const auto& acf_station : planeInventoryCopy.mapStations | std::views::values)
    {
      lmbda_read_image_file_names(acf_station.node);
    }
  }
  else // missionx::XP11_COMPATIBILITY
    lmbda_read_image_file_names(planeInventoryCopy.node);

  if (xp_mapInvImages.size() > static_cast<size_t>(0)) // call execute read async images only if there are any
    queFlcActions.push(mx_flc_pre_command::read_async_inv_image_files);
}


// -------------------------------------


void
data_manager::erase_items_with_zero_quantity_in_all_inventories()
{
  for (auto& inv : mapInventories | std::views::values)
  {
    erase_empty_inventory_item_nodes(inv.node);
  } // end loop over inventories
}


// -------------------------------------


void
data_manager::erase_empty_inventory_item_nodes(const IXMLNode& pNode, const int &inLevel)
{
  int        countDeletedNodes = 0;
  int        nodeCounter_i     = 0;
  const auto nChildNodes       = (pNode.nChildNode() - 1); // v25.02.1

  // We will loop from last to first. Zero is a valid node position.
  for (int i1 = nChildNodes; i1 >= 0 ; --i1)
  {
    // check station and call recursion function to clear station items
    auto cNode = pNode.getChildNode(i1);
    if (const std::string node_name = cNode.getName();
        (node_name == mxconst::get_ELEMENT_STATION()) && (inLevel == 0) )
    {
      erase_empty_inventory_item_nodes(cNode, (inLevel + 1)); // recursion
      nodeCounter_i++; // after evaluating <station> element we continue to next child
    }
    // make sure only <item> elements will be removed
    else if (const int quantity_i = static_cast<int>(Utils::readNumericAttrib(cNode, mxconst::get_ATTRIB_QUANTITY(), 0))
      ; (quantity_i == 0) && (node_name == mxconst::get_ELEMENT_ITEM()) )
    {
      #ifndef RELEASE
      const std::string item_name_s = Utils::readAttrib(cNode, mxconst::get_ATTRIB_NAME(), "");
      const std::string inv_name_s = Utils::readAttrib(pNode, mxconst::get_ATTRIB_NAME(), "");
      Log::logMsg(fmt::format("\tRemoving item: {}, from: {}", item_name_s, inv_name_s) );
      #endif // !RELEASE
      cNode.deleteNodeContent();
      countDeletedNodes++;
    }
    else
      nodeCounter_i++;
  }

  #ifndef RELEASE
  Log::logMsg(fmt::format("Parent Node: [{}], tested: {} item nodes and deleted: {} nodes.", Utils::readAttrib(pNode, mxconst::get_ATTRIB_NAME(), pNode.getName()) , nodeCounter_i, countDeletedNodes)); // debug
  #endif
}


// -------------------------------------


float
data_manager::calculatePlaneWeight(const Inventory& inSourceInventory, const bool& flag_apply_to_dataref, const int& inLayoutVersion)
{
  static std::string err;
  static float       planePayloadWeight = 0.0f;
  float              passengersWeight, storedWeight;
  planePayloadWeight = 0.0f;
  float pilotWeight = passengersWeight = storedWeight = 0.0f;

  // v24.03.2 Added compatibility attribute
  pilotWeight      = static_cast<float> ( Utils::readNumericAttrib ( mx_global_settings.xBaseWeight_ptr, mxconst::get_OPT_PILOT_BASE_WEIGHT(), mxconst::get_OPT_PILOT(), mxconst::DEFAULT_PILOT_WEIGHT ) );
  storedWeight     = static_cast<float> ( Utils::readNumericAttrib ( mx_global_settings.xBaseWeight_ptr, mxconst::get_OPT_STORAGE_BASE_WEIGHT(), mxconst::get_OPT_STORAGE(), 0.0f ) );
  passengersWeight = static_cast<float> ( Utils::readNumericAttrib ( mx_global_settings.xBaseWeight_ptr, mxconst::get_OPT_PASSENGERS_BASE_WEIGHT(), mxconst::get_OPT_PASSENGERS(), 0.0f ) );

  // add inventory
  // v3.0.241.1 added weight configuration from Copied Plane Node
  const auto lmbda_calculate_items_weight = [&](const Inventory& inInventory, const int iLayoutVersion)
  {
    double plane_inventory_weight = 0.0;

    if (inInventory.node.isEmpty())
      return 0.0;
    else
    {
      if (iLayoutVersion == XP11_COMPATIBILITY) // XP11 compatibility calculation
      {
        const auto invNode   = inInventory.node;
        int        nChilds_i = invNode.nChildNode(mxconst::get_ELEMENT_ITEM().c_str());
        for (int i1 = 0; i1 < nChilds_i; ++i1)
        {
          const auto itemWeight   = Utils::readNumericAttrib(invNode.getChildNode(mxconst::get_ELEMENT_ITEM().c_str(), i1), mxconst::get_ATTRIB_WEIGHT_KG(), 0.0);
          const auto itemQuantity = Utils::readNumericAttrib(invNode.getChildNode(mxconst::get_ELEMENT_ITEM().c_str(), i1), mxconst::get_ATTRIB_QUANTITY(), 0.0);

          plane_inventory_weight += itemWeight * itemQuantity;
        }
      } // bXP11Layout
      else
      { // stations
        for (const auto& acf_station : planeInventoryCopy.mapStations | std::views::values)
        {
          plane_inventory_weight += acf_station.total_weight_in_station_f;
        }
      }
    }

    return plane_inventory_weight;
  };

  // v24.12.2 extended lmbda_calculate_items_weight() function.
  // missionx::Inventory dummy_ref_inventory;
  // dummy_ref_inventory.node = inRefNode;

  const auto planeInventoryWeight = static_cast<float>(lmbda_calculate_items_weight((inSourceInventory.node.isEmpty() ? mapInventories[mxconst::get_ELEMENT_PLANE()] : inSourceInventory), Inventory::opt_forceInventoryLayoutBasedOnVersion_i));

  planePayloadWeight = pilotWeight + storedWeight + passengersWeight + planeInventoryWeight;

  // store weight in Dataref
  if ( flag_apply_to_dataref && ( inLayoutVersion == missionx::XP11_COMPATIBILITY ) )
  {
    if ( planePayloadWeight > 0.0f )
      dataref_manager::set_xplane_dataref_value("sim/flightmodel/weight/m_fixed", (double)planePayloadWeight);
  }
  else if (flag_apply_to_dataref)
  {
    // set station weight based on the plane inventory map_stations container.
    if (dref_m_stations_kgs_f_arr.flag_paramReadyToBeUsed && dref_m_stations_kgs_f_arr.arraySize > 0)
    {
      // prepare a vector with the station weights
      std::string       m_stations_s;
      int               m_stations_size_i = 0;

      if (auto vec_m_stations = mapInventories[mxconst::get_ELEMENT_PLANE()].get_inventory_station_weights_as_vector(dref_m_stations_kgs_f_arr.arraySize)
          ; !vec_m_stations.empty())
      {
        std::stringstream ss;
        // Inject pilot weight
        if ( vec_m_stations.at ( 0 ) < mxconst::MINIMUM_EXPECTED_PILOT_WEIGHT_IN_STATION && pilotWeight > 0)
          vec_m_stations.at(0) = pilotWeight;

        m_stations_s.clear();
        m_stations_size_i = 0;
        std::ranges::for_each(vec_m_stations,
                              [&](auto& fValue)
                              {
                                ss << (m_stations_size_i == 0 ? std::string() : std::string(",")) << fValue;
                                m_stations_size_i++;
                              });

        m_stations_s = ss.str();
      }

      int iResult = dref_m_stations_kgs_f_arr.setTargetArray<xplmType_FloatArray, float>(m_stations_size_i, m_stations_s, true, ",");
    }
  }


  return planePayloadWeight;
}


void
data_manager::internally_calculateAndStorePlaneWeight(const Inventory& inSourceInventory, const bool& flag_apply_to_dataref, const int& inLayoutVersion)
{
  current_plane_payload_weight_f = calculatePlaneWeight(inSourceInventory, flag_apply_to_dataref, inLayoutVersion);
}

// -------------------------------------
// saveCheckpoint should store all "map containers" into the XML.
// Most of the containers will be stored under the root element.
void
data_manager::saveCheckpoint(IXMLNode& inParent)
{
  // save mission properties
  IXMLNode xMissionFolderProperties = inParent.addChild(mxconst::get_PROP_MISSION_FOLDER_PROPERTIES().c_str());
  // v3.303.8.3 add local time to global settings
  const auto current_time_sec = dataref_manager::getLocalTimeSec();
  mx_global_settings.node.updateAttribute(mxUtils::formatNumber<float>(current_time_sec, 6).c_str(), mxconst::get_ATTRIB_LOCAL_TIME_SEC().c_str(), mxconst::get_ATTRIB_LOCAL_TIME_SEC().c_str());

  inParent.addChild(mx_global_settings.node.deepCopy()); // v3.0.241.1 add global_settings

  // Store Briefer
  Utils::xml_add_comment(inParent, " ===== Briefer ===== ");

  inParent.addChild(briefer.node.deepCopy()); // v3.0.241.1

  // loop over Flight Legs
  Utils::xml_add_comment(inParent, " ===== Flight Legs ===== ");

  IXMLNode xFlightLegs = inParent.addChild(mxconst::get_ELEMENT_FLIGHT_PLAN().c_str()); // v3.0.221.15rc5 compatible with new Leg and Goals
  for ( auto &leg : mapFlightLegs | std::views::values )
  {
    leg.saveCheckpoint(xFlightLegs);
  }

  // loop over objectives
  Utils::xml_add_comment(inParent, " ===== Objectives ===== ");

  IXMLNode xObjectives = inParent.addChild(mxconst::get_ELEMENT_OBJECTIVES().c_str());
  for ( auto &obj : mapObjectives | std::views::values )
  {
    obj.saveCheckpoint(xObjectives);
  }

  // loop over triggers
  Utils::xml_add_comment(inParent, " ===== Triggers ===== ");

  IXMLNode xTriggers = inParent.addChild(mxconst::get_ELEMENT_TRIGGERS().c_str());
  for ( auto &trig : mapTriggers | std::views::values )
  {
    trig.saveCheckpoint(xTriggers);
  }

  // loop over Messages
  Utils::xml_add_comment(inParent, " ===== Messages ===== ");

  IXMLNode xMessages = inParent.addChild(mxconst::get_ELEMENT_MESSAGE_TEMPLATES ().c_str());
  for ( auto &m : mapMessages | std::views::values )
  {
    // std::string debugMode = Utils::readAttrib(m.node, mxconst::get_ATTRIB_MODE(), ""); // v3.305.1 debug why "mode" attribute is reset
    m.saveCheckpoint(xMessages);
  }

  // loop over DataRefs
  Utils::xml_add_comment(inParent, " ===== Datarefs info ===== ");

  IXMLNode xLogic = inParent.addChild(mxconst::get_ELEMENT_LOGIC().c_str());
  for ( auto &dref : mapDref | std::views::values )
  {
    dref.saveCheckpoint(xLogic);
  }


  // v3.305.3
  Utils::xml_add_comment(inParent, " ===== Interpolation Datarefs info ===== ");

  IXMLNode xInterpolation = inParent.addChild(mxconst::get_ELEMENT_INTERPOLATION().c_str());
  for (auto& [key, dref] : mapInterpolDatarefs)
  {
    auto xChild = xInterpolation.addChild(mxconst::get_ELEMENT_DATAREF().c_str());
    if (!xChild.isEmpty())
    {
      // key and name are the same for now
      xChild.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), key.c_str());
      dref.saveInterpolationCheckpoint(xChild);
      xChild.addAttribute(mxconst::get_ATTRIB_DREF_KEY().c_str(), key.c_str());
    }
  }



  // save 3D Objects
  Utils::xml_add_comment(inParent, " ===== 3D Objects and Instances ===== ");
  {
    IXMLNode x3dRoot      = inParent.addChild (   mxconst::get_PROP_OBJECTS_ROOT().c_str () );
    IXMLNode x3dTemplates = x3dRoot.addChild(mxconst::get_ELEMENT_OBJECT_TEMPLATES().c_str());
    for ( const auto &key : map3dObj | std::views::keys )
      map3dObj[key].saveCheckpoint(x3dTemplates);

    IXMLNode x3dInstances = x3dRoot.addChild(mxconst::get_PROP_OBJECTS_INSTANCES().c_str());
    for ( const auto &key : map3dInstances | std::views::keys )
    {
      map3dInstances[key].saveCheckpoint(x3dInstances);
    }

    for (int i1 = 0; i1 < x3dInstances.nChildNode(); ++i1)
    {
      auto              xSubNode     = x3dInstances.getChildNode(i1);
      const std::string attrib_value = Utils::readAttrib(xSubNode, mxconst::get_ATTRIB_INSTANCE_NAME(), "");
      if (attrib_value.empty())
        continue;
      else
      {
        Utils::xml_delete_all_subnodes_except(xSubNode, mxconst::get_ELEMENT_DISPLAY_OBJECT(), true, mxconst::get_ELEMENT_DISPLAY_OBJECT() + "," + mxconst::get_ATTRIB_INSTANCE_NAME() + "," + attrib_value); // remove any <display_object> except the one that holds the instance information.
      }
    }
  }

  inParent.addChild(endMissionElement.node); // v3.0.241.1

  // Inventories
  // loop over Messages
  Utils::xml_add_comment(inParent, " ===== Inventories ===== ");

  IXMLNode xInventories = inParent.addChild(mxconst::get_ELEMENT_INVENTORIES().c_str());
  for ( auto &inv : mapInventories | std::views::values )
  {
    xInventories.addChild(inv.node.deepCopy());
  }

  //// End saving Inventories /////
  // -----------------------------


  //// Save MX Choices
  Utils::xml_add_comment(inParent, " ===== Choices info ===== ");
  if (!xmlChoices.isEmpty())
  {
    // int n = missionx::data_manager::xmlChoices.nChildNode();
    // Utils::xml_print_node(missionx::data_manager::xmlChoices);

    xmlChoices.updateAttribute(mxChoice.currentChoiceName_beingDisplayed_s.c_str(), mxconst::get_ATTRIB_ACTIVE_CHOICE().c_str(), mxconst::get_ATTRIB_ACTIVE_CHOICE().c_str());
    inParent.addChild(xmlChoices.deepCopy());
    xmlChoices.updateAttribute("", mxconst::get_ATTRIB_ACTIVE_CHOICE().c_str(), mxconst::get_ATTRIB_ACTIVE_CHOICE().c_str()); // remove the setting. Only for checkpoint
  }


  //// Save FMS
  // v3.0.231.1 store the FMS information
  Utils::xml_add_comment(inParent, " ===== FMS info ===== ");
  {

    const int entries_i            = XPLMCountFMSEntries();
    const int displayed_entry_i    = XPLMGetDisplayedFMSEntry();
    const int destination_in_fms_i = XPLMGetDestinationFMSEntry();

    IXMLNode xmlFMS = inParent.addChild(mxconst::get_ELEMENT_FMS().c_str());
    for (int i1 = 0; i1 < entries_i; ++i1)
    {
      NavAidInfo navAid;
      int                  alt_mt;

      // read FMS entries and store them in <fms> element
      XPLMGetFMSEntryInfo(i1, &navAid.navType, navAid.ID, &navAid.navRef, &alt_mt, &navAid.lat, &navAid.lon);
      navAid.height_mt = static_cast<float> ( alt_mt );

      navAid.synchToPoint();
      xmlFMS.addChild(navAid.node.deepCopy());
    }

    xmlFMS.updateAttribute(Utils::formatNumber<int>(displayed_entry_i).c_str(), mxconst::get_ATTRIB_DISP_FMS_ENTRY().c_str(), mxconst::get_ATTRIB_DISP_FMS_ENTRY().c_str());
    xmlFMS.updateAttribute(Utils::formatNumber<int>(destination_in_fms_i).c_str(), mxconst::get_ATTRIB_DESTINATION_ENTRY().c_str(), mxconst::get_ATTRIB_DESTINATION_ENTRY().c_str());

    //// Save GPS if was defined in the mission file.
    Utils::xml_add_comment(inParent, " ===== GPS info ===== ");
    if (!xmlGPS.isEmpty())
    {
      // add current location
      xmlGPS.updateAttribute(Utils::formatNumber<int>(destination_in_fms_i).c_str(), mxconst::get_ATTRIB_DESTINATION_ENTRY().c_str(), mxconst::get_ATTRIB_DESTINATION_ENTRY().c_str());

      // add to SAVEPOINT
      inParent.addChild(xmlGPS.deepCopy());
    }

    // v3.0.253.7 Add fail timers
    Utils::xml_add_comment(inParent, " ===== Fail Timers ===== ");
    for ( auto &tmr : mapFailureTimers | std::views::values )
    {
      tmr.saveCheckpoint(inParent);
    }
  } // end FMS
}

// -------------------------------------

void
data_manager::addLoadErr(const std::string& inErr)
{
  if (!inErr.empty())
  {
    lstLoadErrors.emplace_back(inErr); // v3.305.3
  }
}

// -------------------------------------

void
data_manager::clearAllQueues()
{
  // clear pre flc action queue
  while (!queFlcActions.empty())
    queFlcActions.pop();

  while (!queThreadMessage.empty()) // v3.0.219.12+
    queThreadMessage.pop();
}

// -------------------------------------

void
data_manager::resetCueSettings()
{
  seqCueInfo = 0;

  if (!currentLegName.empty())
    mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg.clear();
}

// -------------------------------------

void
data_manager::flc_cue(cue_actions_enum inAction)
{
  // go over all current triggers in active leg ro inventories and check:
  // 1. Is trigger part of current leg.
  // 2. Define line color based on pertinence to the task or leg
  // 3. prepare the coordinates to display GL lines include Elevation
  //  bool flag_found = true;

  if (currentLegName.empty())
    return;

  // Loop over all triggers
  if (!mapTriggers.empty())
  {
    for (auto& trig : mapTriggers)
    {
      std::string trigName    = mapTriggers[trig.first].getName();                                                  // get trigger name
      std::string trigType    = Utils::readAttrib(mapTriggers[trig.first].node, mxconst::get_ATTRIB_TYPE(), "");          // v3.0.241.1 // get trigger type
      bool        trigEnabled = Utils::readBoolAttrib(mapTriggers[trig.first].node, mxconst::get_ATTRIB_ENABLED(), true); // v3.0.241.1 // is trigger active ?

      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Is trigger part of current leg and which Color to draw
      // Default color: Yellow. Task based trigger: Green. Mandatory Task: Red. Disabled trigger: Gray
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // v3.0.213.7 try to fix crash due to null pointers

      mapTriggers[trigName].prepareCueMetaData(); // v3.0.213.7 also try to fix a crash

      CueInfo cue = mapTriggers[trigName].getCopyOfCueInfo(); // clone from trigger cue. We won't touch the trigger cue

      if (cue.cueSeq <= 0)
      {
        ++seqCueInfo;
        cue.cueSeq = seqCueInfo;
      }

      if (trigEnabled)
        cue.color.setToDefaultCueColor(); // pastel yellow

      // color triggers based on task
      for (const auto& objName : mapFlightLegs[currentLegName].listObjectivesInFlightLeg)
      {
        for ( auto &task : mapObjectives[objName].mapTasks | std::views::values )
        {
          // check if task is based on trigger and if trigger name is same as trigger at hand
          std::string base_on_trigger_value = Utils::readAttrib(task.node, mxconst::get_ATTRIB_BASE_ON_TRIGGER(), "");
          if (base_on_trigger_value.empty())
            continue;

          if (trigName == base_on_trigger_value)
          {
            cue.color.setToGreen();

            // check if task is mandatory
            if (task.isMandatory)
              cue.color.setToBlue();

            cue.canBeRendered = true;
            break;
          }
        }
      } // end loop over tasks


      ///////////////////////////////////////////////////////////////////////////////////////////
      // check if trigger was not found in Objective level but it is present in Flight Leg level
      //////////////////////////////////////////////////////////////////////////////////////////
      if (!cue.canBeRendered)
      {
        for (const auto& legTrigName : mapFlightLegs[currentLegName].listTriggers)
        {
          if (legTrigName == trigName) // if trigger is in <leg> level. This can override the previous trigger coloring.
          {
            cue.color.setToBlack();
            cue.canBeRendered = true;
            break; // v3.0.217.7 no reason to continue looping
          }

        } // end loop trig name

      } // end if cueSeq was defined or not

      /////////// CHECK if CAMERA //////////////
      // v3.0.223.7 camera triggers overrides all colors
      if (cue.canBeRendered && !cue.node_ptr.isEmpty()) // cue.sourceProperties_ptr != nullptr)
      {
        std::string err;
        std::string triggerType = Utils::readAttrib(mapTriggers[trigName].node, mxconst::get_ATTRIB_TYPE(), ""); // v3.0.241.1
        if (triggerType == mxconst::get_TRIG_TYPE_CAMERA())
        {
          cue.color.setToPastel_red();
        }
      }


      ///// If trigger disable then we will reset the color anyway //////////////
      if (!trigEnabled)
        cue.color.setColorToDisable(); // Disabled Color = Grey

      //}
      ///////////////////////////////////////////////////////////////
      // End set color
      ///////////////////////////////////////////////////////////////

      //// End if trigger is part of current leg and which color

      if (!cue.canBeRendered) // skip trigger configuration if it is not part of the current <leg>
        continue;             // skip the rest of calculations since triggers Cue should not be displayed

      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /////// Calculate Elevation and points depending on Trigger Type (radius/Poly) and elevation settings (on ground, elevation volume etc).
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

      bool isOnGround = true;

      // v3.0.241.1 replace with XML calls
      const std::string strIsPlaneOnGround = (mapTriggers[trigName].xConditions.isEmpty()) ? "" : Utils::readAttrib(mapTriggers[trigName].xConditions, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "");
      auto            attrib_elev_ft_lower = mapTriggers[trigName].getAttribNumericValue<double>(mxconst::get_ATTRIB_ELEV_FT(), 0.0, errStr); // stay the same since we store it as "xxx|xxx" in xml attribute
      auto            attrib_elev_ft_upper = mapTriggers[trigName].getAttribNumericValue<double>(mxconst::get_ATTRIB_ELEV_MAX_FT(), 0.0, errStr);

      // v3.0.217.7 parse string value into bool
      if (!strIsPlaneOnGround.empty())
        Utils::isStringBool(strIsPlaneOnGround, isOnGround);

      bool hasMinMaxElevationValues = false;

      // Check if trigger is based on RADIUS
      if ((mxconst::get_TRIG_TYPE_RAD().compare(trigType) == 0 || mxconst::get_TRIG_TYPE_CAMERA().compare(trigType) == 0) && !mapTriggers[trigName].deqPoints.empty())
      {

        Point pCenter = mapTriggers[trigName].deqPoints.front(); // retrieve first point from dequeue

        // decide if to check each point elevation. Basically we always do that, but if Elevation is zero than "always true", but if elevation is not zero, then we need to use min and max
        if (!isOnGround || (mapTriggers[trigName].node.isAttributeSet(mxconst::get_ATTRIB_ELEV_FT().c_str()) && mapTriggers[trigName].node.isAttributeSet(mxconst::get_ATTRIB_ELEV_MAX_FT().c_str())))
        {
          hasMinMaxElevationValues = true;
          pCenter.setElevationFt(attrib_elev_ft_lower); // we should set some elevation value if designer set it for us.
        }

        // v3.0.217.7 overrides hasMinMax, since we do not care about elevation if on_ground was not set.
        if (strIsPlaneOnGround.empty())
          hasMinMaxElevationValues = false;

        cue.calculateCircleFromPoint(pCenter, true, !(hasMinMaxElevationValues));
      }
      else if (mxconst::get_TRIG_TYPE_POLY() == trigType && !mapTriggers[trigName].deqPoints.empty())
      {
        if (!isOnGround || (mapTriggers[trigName].node.isAttributeSet(mxconst::get_ATTRIB_ELEV_FT().c_str()) && mapTriggers[trigName].node.isAttributeSet(mxconst::get_ATTRIB_ELEV_MAX_FT().c_str())))
          hasMinMaxElevationValues = true;

        // v3.0.217.7 overrides hasMinMax, since we do not care about elevation if on_ground was not set.
        if (strIsPlaneOnGround.empty())
          hasMinMaxElevationValues = false;

        // loop over all points in poly
        cue.calculatePolygonalPointsElevation();
      }

      // *****************************************************
      // Add points for elevMax_ft = Upper limit
      // *****************************************************
      if (hasMinMaxElevationValues && (attrib_elev_ft_lower != attrib_elev_ft_upper))
      {
        XPLMProbeResult  result = -1;
        std::list<Point> listUpperPoints;

        CueInfo::prepareProbe();
        for (auto p : cue.vecPoints)
        {
          Point pUpper = p; // clone p
          pUpper.setElevationFt(attrib_elev_ft_upper);
          pUpper.calcSimLocalData();

          result                              = -1;
          const double groundElevationInMeter = CueInfo::getTerrainElevInMeter_FromPoint(pUpper, result);

          if (groundElevationInMeter > pUpper.getElevationInMeter())
          {
            pUpper.setElevationMt(groundElevationInMeter);
            pUpper.elevWasProbed = true;
            pUpper.calcSimLocalData();
          }

          listUpperPoints.push_back(pUpper);
        } // end loop over all lower points

        CueInfo::destroyProbe();

        if (!listUpperPoints.empty())
        {
          for (auto p : listUpperPoints)
            cue.vecPoints.push_back(p);
        }

        listUpperPoints.clear();
      } // end if has min/max

      /////////////////////////////////////////
      mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg.push_back(cue);

    } // end mapTrigger loop
  }


  ////////////////////////
  // Loop over inventories
  ////////////////////////


  for (auto& inv : mapInventories)
  {

    const std::string invName = inv.first;
    const std::string invType = Utils::readAttrib(mapInventories[invName].node, mxconst::get_ATTRIB_TYPE(), "");

    CueInfo cue = mapInventories[invName].getCopyOfCueInfo(); // clone from Inventory cue.
    if (cue.cueSeq <= 0)
    {
      ++seqCueInfo;
      cue.cueSeq = seqCueInfo;
    }

    cue.color.setToPurple();
    cue.canBeRendered = true; // we always render inventory/store areas


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////// Calculate Elevation and points depending on Area Type (radius/Poly)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // bool    isOnGround = true; // always for inventories
    // Check if Inventory is based on RADIUS
    if (mxconst::get_TRIG_TYPE_RAD() == invType && !mapInventories[invName].deqPoints.empty())
    {
      Point pCenter;
      pCenter.node = mapInventories[invName].deqPoints.front().node.deepCopy(); // retrieve first point from dequeue
      pCenter.parse_node();

      cue.calculateCircleFromPoint(pCenter, true, true); // calculate points while always closing and calculating elevation on ground

    } // end if RAD based
    else if (mxconst::get_TRIG_TYPE_POLY() == invType && !mapInventories[invName].deqPoints.empty())
    {
      // loop over all points in poly
      cue.calculatePolygonalPointsElevation();
    }

    mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg.push_back(cue);

  } // end inventory cue

  // v3.0.303 added task type cargo - cue calculation
  for (const auto& objectiveName : mapFlightLegs[currentLegName].listObjectivesInFlightLeg)
  {
    for ( auto &task : mapObjectives[objectiveName].mapTasks | std::views::values )
    {
      if (task.task_type == mx_task_type::base_on_sling_load_plugin)
      {
        Point pCenter = task.pSlingEnd;
        CueInfo         cue     = task.getCopyOfCueInfo();

        cue.canBeRendered = true; // we always render cargo target, just like inventories

        cue.color.setToBrown_chocolate();
        cue.calculateCircleFromPoint(pCenter, true, true); // calculate points while always closing and calculating elevation on ground
        mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg.push_back(cue);
      }
    }
  }

  mapFlightLegs[currentLegName].flag_cue_was_calculated = true;


  // end flc_cue
}

// -------------------------------------

void
data_manager::drawCueInfo()
{

  ///* Reset the graphics state.  This turns off fog, texturing, lighting,
  // * alpha blending or testing and depth reading and writing, which
  // * guarantees that our axes will be seen no matter what. */
  // XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0);

  for (auto& cue_ptr : mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg)
  {
    if (cue_ptr.canBeRendered)
    {
      bool bCalculated = false;
      switch (cue_ptr.cueType)
      {
        case mx_cue_types::cue_trigger:
        {

          if (cue_ptr.color.R == mxconst::MANDATORY_R && cue_ptr.color.G == mxconst::MANDATORY_G && cue_ptr.color.B == mxconst::MANDATORY_B) // If is RED = mandatory task
            glLineWidth(4.0); // mandatory line will be wider
          else
            glLineWidth(2.0);
          glBegin(GL_LINE_LOOP);

          auto itEnd   = cue_ptr.vecPoints.end();
          int  counter = 0;
          for (auto it = cue_ptr.vecPoints.begin(); it != itEnd; ++it, ++counter) // inner loop over points
          {
            if (counter == 0) // v3.0.231.2 added starting directional color to assist designer figure which way he/she positioned the plane - clockwise or counter clock wize.
            {
              const CueInfo::struct_color tmpC = cue_ptr.color;
              cue_ptr.color.setToRed();
              glColor3f(cue_ptr.color.R, cue_ptr.color.G, cue_ptr.color.B); // draw GL as RED
              cue_ptr.color.setRGB(tmpC.R, tmpC.G, tmpC.B);                 // return to default color
            }
            else if (counter == 1)
              glColor3f(cue_ptr.color.R, cue_ptr.color.G, cue_ptr.color.B);

            if (!cue_ptr.wasCalculated)
            {
              it->calcSimLocalData();
              bCalculated = true;
            }
            // draw point, will be enclosed by the GL_LINE_LOOP
            glVertex3f( static_cast<float> ( it->local_x ), static_cast<float> ( it->local_y ), static_cast<float> ( it->local_z ) );
          }        // iterate over Points
          glEnd(); // end draw
        }
        break;
        case mx_cue_types::cue_inventory:
        case mx_cue_types::cue_sling_task:
        case mx_cue_types::cue_obj: // v3.0.303.6
        {
          glColor3f(cue_ptr.color.R, cue_ptr.color.G, cue_ptr.color.B);
          glLineWidth(2.0);

          glBegin(GL_LINE_LOOP);
          for (auto& it : cue_ptr.vecPoints) // inner loop over points. "auto &" is important for performance
          {
            if (!cue_ptr.wasCalculated)
            {
              it.calcSimLocalData();
              bCalculated = true;
            }
            // draw point, will be enclosed by the GL_LINE_LOOP
            glVertex3f( static_cast<float> ( it.local_x ), static_cast<float> ( it.local_y ), static_cast<float> ( it.local_z ) );
          }        // iterate over Points
          glEnd(); // end draw
        }
        break;
        default:
          break;
      } // END Switch

      if (bCalculated)
      {
        cue_ptr.wasCalculated = true;
        Log::logDebugBO(mxconst::get_UNIX_EOL() + "re-calculated: " + cue_ptr.originName); // debug
      }
    } // end loop CueInfo
  }
}

// -------------------------------------

void
data_manager::drawCueInfoOn2DMap(const float* inMapBoundsLeftTopRightBottom, const XPLMMapProjectionID * projection_ptr)
{
  assert(projection_ptr && fmt::format("{} must have projection pointer defined.", __func__).c_str());
  assert(inMapBoundsLeftTopRightBottom && fmt::format("{} must have inMapBoundsLeftTopRightBottom pointer defined.", __func__).c_str());

  if (projection_ptr == nullptr)
    return;

  float x, y;
  for (auto& cue_ptr : mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg) //
  {
    bool calculated = false;
    if (cue_ptr.canBeRendered)
    {
      switch (cue_ptr.cueType)
      {
        case mx_cue_types::cue_trigger:
        {

          if (cue_ptr.color.R == mxconst::MANDATORY_R && cue_ptr.color.G == mxconst::MANDATORY_G && cue_ptr.color.B == mxconst::MANDATORY_B) // If is RED = mandatory task
            glLineWidth(4.0);                                                                                                                // mandatory line will be wider
          else
            glLineWidth(2.0);
          glBegin(GL_LINE_LOOP);

          auto itEnd   = cue_ptr.vecPoints.end();
          int  counter = 0;

          for (auto& point : cue_ptr.vecPoints)
          {
            if (counter == 0) // v3.0.231.2 added starting directional color to assist designer figure which way he/she positioned the plane - clockwise or counter clock wize.
            {
              const CueInfo::struct_color tmpC = cue_ptr.color;
              cue_ptr.color.setToRed();
              glColor3f(cue_ptr.color.R, cue_ptr.color.G, cue_ptr.color.B); // draw GL as RED
              cue_ptr.color.setRGB(tmpC.R, tmpC.G, tmpC.B); // return to default color
            }
            else if (counter == 1)
              glColor3f(cue_ptr.color.R, cue_ptr.color.G, cue_ptr.color.B);

            counter++;

            // draw point, will be enclosed by the GL_LINE_LOOP
            XPLMMapProject((*projection_ptr), point.getLat(), point.getLon(), &x, &y);
            // if (mxUtils::coord_in_rect(x, y, inMapBoundsLeftTopRightBottom)) // cooling
            glVertex2f(x, y);

          } // iterate over Points
          glEnd(); // end draw
        }
        break;
        case mx_cue_types::cue_inventory:
        case mx_cue_types::cue_sling_task:
        case mx_cue_types::cue_obj: // v3.0.303.6
        {
          glColor3f(cue_ptr.color.R, cue_ptr.color.G, cue_ptr.color.B);
          glLineWidth(2.0);

          glBegin(GL_LINE_LOOP);
          for (auto& point : cue_ptr.vecPoints) // inner loop over points. "auto &" is important for performance
          {
            // draw point, will be enclosed by the GL_LINE_LOOP
            XPLMMapProject((*projection_ptr), point.getLat(), point.getLon(), &x, &y);
            // if (mxUtils::coord_in_rect(x, y, inMapBoundsLeftTopRightBottom)) // cooling
            glVertex2f(x, y);

          }        // iterate over Points
          glEnd(); // end draw
        }
        break;
        default:
          break;
      } // END Switch

    } // end loop CueInfo
  }
}

// -------------------------------------

void
data_manager::setGPS()
{
  bool           flag_found = false;
  const bool bUseGPS   = (xmlLoadedFMS.isEmpty()) ? true : false; // v25.04.2 decide if to use GPS or FMS, we will use the bool value to decide if to read the auto_load option or not.
  const IXMLNode xFMS_ptr   = (bUseGPS) ? xmlGPS : xmlLoadedFMS; // pick the FMS element or GPS dependent on availability in the mission/checkpoint file.

  if (xFMS_ptr.isEmpty())
    return;

  // v25.04.2
  if (bUseGPS)
  {
    const bool bAutoLoad = Utils::readBoolAttrib (data_manager::xmlGPS, mxconst::get_PROP_AUTO_LOAD_ROUTE_TO_GPS_OR_FMS_B (), false);
    const bool bGenerateGPS = Utils::readBoolAttrib (data_manager::xmlGPS, mxconst::get_PROP_GENERATE_GPS_WAYPOINTS (), false);
    if ( !(bGenerateGPS * bAutoLoad) )
      return; // exit the code in this function.
  }

  // v3.0.303.2
  auto       nodeLocationAdjust_ptr = xMainNode.getChildNodeByPath((mxconst::get_ELEMENT_BRIEFER() + "/" + mxconst::get_ELEMENT_LOCATION_ADJUST()).c_str());
  const bool b_locationTypeIsPlane  = mxconst::get_ELEMENT_PLANE() == Utils::readAttrib(nodeLocationAdjust_ptr, mxconst::get_ATTRIB_LOCATION_TYPE(), "");
  // clear GPS
  clearFMSEntries(); // v3.0.253.7

  // store the current entry to set later on.
  const int iCurrentGpsActiveEntry = static_cast<int> ( Utils::readNumericAttrib ( xFMS_ptr, mxconst::get_ATTRIB_DESTINATION_ENTRY(), 0.0 ) );

  const int nChilds = xFMS_ptr.nChildNode(mxconst::get_ELEMENT_POINT().c_str());

  int index = 0;
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    NavAidInfo navInfo;

    flag_found         = false;
    IXMLNode cNode_ptr = xFMS_ptr.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), i1); // .deepCopy(); // v3.0.219.7 copy of GPS point

    auto              lat_f              = Utils::readNodeNumericAttrib<float> ( cNode_ptr, mxconst::get_ATTRIB_LAT(), 0.0f );
    auto              lon_f              = Utils::readNodeNumericAttrib<float> ( cNode_ptr, mxconst::get_ATTRIB_LONG(), 0.0f );
    const auto        elev_ft_f          = Utils::readNodeNumericAttrib<float> ( cNode_ptr, mxconst::get_ATTRIB_ELEV_FT(), 0.0f ); // v3.0.241.3
    const std::string icao               = Utils::xml_get_attribute_value ( cNode_ptr, mxconst::get_ELEMENT_ICAO(), flag_found );
    const int         navType_fromNode_i = Utils::readNodeNumericAttrib<int> ( cNode_ptr, mxconst::get_ATTRIB_NAV_TYPE(), xplm_Nav_Unknown ); // v3.0.255.4

    // v3.0.221.7 - add ICAO if available
    bool flag_icao_is_valid = false; // will hold the icao string if it is valid one.
    if (!icao.empty())
    {
      // search XPLMNavRef
      if (index == 0 && b_locationTypeIsPlane)
      {
        // don't do anything
      }
      else
      {
        XPLMNavRef nav_ref = XPLM_NAV_NOT_FOUND;

        if (navType_fromNode_i > xplm_Nav_Unknown)  // v3.0.255.4
          nav_ref = XPLMFindNavAid(nullptr, icao.data(), &lat_f, &lon_f, nullptr, navType_fromNode_i); // find ref based on point nav_type. We can add assert on navRef_fromNode_i
        else
          nav_ref = XPLMFindNavAid(nullptr, icao.data(), &lat_f, &lon_f, nullptr, xplm_Nav_Airport);

        if (nav_ref != XPLM_NAV_NOT_FOUND)
        {
          flag_icao_is_valid = true;
          XPLMGetNavAidInfo(nav_ref, &navInfo.navType, &navInfo.lat, &navInfo.lon, &navInfo.height_mt, &navInfo.freq, &navInfo.heading, navInfo.ID, navInfo.name, navInfo.inRegion);

          XPLMSetFMSEntryInfo(index, nav_ref, static_cast<int> ( navInfo.height_mt ) );

          ++index;
        }
      }
    }


    // v3.0.241.3 decide elevation based on icao info
    const auto lmbda_get_elevation = [&]()
    {
      if (flag_icao_is_valid)
        return static_cast<int> ( navInfo.height_mt );

      return static_cast<int> ( elev_ft_f * feet2meter );
    };

    const auto elev_mt_i = lmbda_get_elevation();


    // v3.0.241.2 adding RealityXP ".gpf" flight plan format
    if (flag_icao_is_valid)
    {

      const double distance = Utils::calcDistanceBetween2Points_nm(navInfo.lat, navInfo.lon, lat_f, lon_f);

      // skip lat/lon in FMS if too close to ICAO default location
      if (distance < 0.2)
      {
        continue;
      }
    }

    XPLMSetFMSEntryLatLon(index, lat_f, lon_f, elev_mt_i); // v3.0.241.3

    ++index;
  } // end loop over all child <point>s

  const std::string mission_state_s = Utils::readAttrib(mx_global_settings.node,   mxconst::get_PROP_MISSION_STATE(), ""); // empty value means new mission and not loaded checkpoint
  const int         entries         = XPLMCountFMSEntries ();

  if (mission_state_s.empty())
    XPLMSetDestinationFMSEntry(iCurrentGpsActiveEntry); // set GPS/FMS active entry
  else
    XPLMSetDestinationFMSEntry(entries - 1); // set last entry for loaded from checkpoint
}

// -------------------------------------

void
data_manager::addLatLonEntryToGPS(Point& inPoint, const int& inEntry)
{

  {
    int entries = XPLMCountFMSEntries ();
    if (inEntry < 0)
      XPLMSetFMSEntryLatLon(entries, static_cast<float> ( inPoint.getLat () ), static_cast<float> ( inPoint.getLon () ), static_cast<int> ( inPoint.getElevationInMeter () ) );
    else
      XPLMSetFMSEntryLatLon(inEntry, static_cast<float> ( inPoint.getLat () ), static_cast<float> ( inPoint.getLon () ), static_cast<int> ( inPoint.getElevationInMeter () ) );
  }
}

// -------------------------------------

std::list<std::string>
data_manager::parseStringToCommandRef(const std::string& inCommandsAsString)
{
  std::list<std::string> listCommands = Utils::splitStringToList(inCommandsAsString, ",");

  for (const auto& val : listCommands)
  {
    std::string command = val;

    // check exists
    if (Utils::isElementExists(mapCommands, command))
    {
      Log::logMsg("[read command] Command: " + command + ", already exists. Skipping...");
      continue;
    }
    BindCommand newCommand;

    std::vector<std::string> vecCmdSplit = mxUtils::split_v2(command, ":");
    switch ( int vecSize = static_cast<int> ( vecCmdSplit.size () ) )
    {
      case 2:
      {
        if (Utils::is_number(vecCmdSplit.at(1)))
          newCommand.secToExecute = Utils::stringToNumber<float>(vecCmdSplit.at(1));
      }
        [[fallthrough]];
      case 1:
      {
        command        = vecCmdSplit.front();
        newCommand.ref = XPLMFindCommand(command.c_str());
        if (newCommand.ref == nullptr)
        {
          Log::logMsg("[read command] Command: " + command + ", not found. Skipping...");
          continue;
        }
      }
      break;
      default:
        Log::logMsg("[read command] Command format: " + command + ", is not supported, skipping command...");
        continue;
        break;
    } // end switch

    Utils::addElementToMap(mapCommands, val, newCommand); // val holds the original format of XP command path example: "command/something:10"
#ifndef RELEASE
    Log::logMsg("[read command] Added Command: " + val);
#endif

  } // end inner loop over all command strings

  return listCommands;
}

// -------------------------------------

double
data_manager::get_distance_of_plane_to_instance_in_meters(const std::string& inInstanceName, std::string& errMsg)
{
  errMsg.clear();

  if (Utils::isElementExists(map3dInstances, inInstanceName))
  {

    return dataref_manager::getPlanePointLocationThreadSafe().calcDistanceBetween2Points(map3dInstances[inInstanceName].getCurrentCoordination(), mx_units_of_measure::meter);
  }

  errMsg = "Fail to find active instance by the name: " + inInstanceName;

  return -1.0;
}

// -------------------------------------

double
data_manager::get_elev_of_plane_relative_to_instance_in_feet(const std::string& inInstanceName, std::string& errMsg)
{
  errMsg.clear();

  if (Utils::isElementExists(map3dInstances, inInstanceName))
  {
    return std::fabs(dataref_manager::getPlanePointLocationThreadSafe().getElevationInFeet() - map3dInstances[inInstanceName].getCurrentCoordination().getElevationInFeet());
  }

  errMsg = "Fail to find active instance by the name: " + inInstanceName;

  return -1.0;
}

// -------------------------------------

double
data_manager::get_bearing_of_plane_to_instance_in_deg(const std::string& inInstanceName, std::string& errMsg)
{
  errMsg.clear();

  if (Utils::isElementExists(map3dInstances, inInstanceName))
  {
    Point plane_p  = dataref_manager::getPlanePointLocationThreadSafe();
    Point target_p = map3dInstances[inInstanceName].getCurrentCoordination();

    return Utils::calcBearingBetween2Points(plane_p.getLat(), plane_p.getLon(), target_p.getLat(), target_p.getLon());
  }

  errMsg = "Fail to find active instance by the name: " + inInstanceName;

  return -1.0;
}

// -------------------------------------

void
data_manager::pluginStop()
{
  clearMissionLoadedTextures();

  seqCueInfo = 0; // v3.0.202a
}

// -------------------------------------

void
data_manager::loadAll3dObjectFiles()
{

  std::string objName;
  objName.clear();

  for ( auto &obj : map3dObj | std::views::values )
  {
    bool isVirtual = false;

    // try to load as virtual file
    if (!obj.g_object_ref)
    {
      obj.file_and_path = Utils::readAttrib(obj.node, mxconst::get_ATTRIB_FILE_NAME(), ""); // try to load as Virtual File
      XPLMLookupObjects(obj.file_and_path.c_str(), 0, 0, Utils::load_cb, &obj.g_object_ref);

      if (obj.g_object_ref)
        isVirtual = true;
      else
      {
        // v3.303.14 re-wrote the logic to handle cases where file is relative ("../file") to mission package, or it represents the "3D library" inside "Custom Scenery", so we need to start the search from there.
        const auto internalObject3D_folder = mx_folders_properties.getStringAttributeValue(mxconst::get_ATTRIB_OBJ3D_FOLDER_NAME(), "");
        const auto file_name               = obj.getStringAttributeValue(mxconst::get_ATTRIB_FILE_NAME(), "");

        if (file_name.find("..") == 0)
          obj.file_and_path = internalObject3D_folder + file_name;
        else if ((file_name.find('/') == std::string::npos) && (file_name.find('\\') == std::string::npos)) // if we provide only 3D Object name and there is no path in it
          obj.file_and_path = internalObject3D_folder + file_name;
        else
          obj.file_and_path = "Custom Scenery/" + file_name;

        obj.g_object_ref = XPLMLoadObject(obj.file_and_path.c_str()); // Load the 3D Object as a File
      }
    }



    if (obj.g_object_ref) // debug
      Log::logMsg("Loaded: " + ((isVirtual) ? "Virtual File: " + obj.file_and_path : "File: " + obj.file_and_path));
    else
      Log::logMsg("Failed Loading: " + obj.file_and_path);
  }

  // v3.303.11 look up and set all instances that were loaded from savepoint
  if (mx_global_settings.flag_loadedFromSavepoint)
  {
    for (auto& [key, instObj] : map3dInstances)
    {
      const auto obj3d_template_name = instObj.getName();
      if (mxUtils::isElementExists(map3dObj, obj3d_template_name))
      {
        instObj.g_object_ref = map3dObj[obj3d_template_name].g_object_ref;
      }
    }
  }
}

// -------------------------------------

void
data_manager::releaseMissionInstancesAnd3DObjects()
{
  // destroy instances
  for ( const auto &key : map3dInstances | std::views::keys )
  {
    if (map3dInstances[key].g_instance_ref)
    {
      XPLMDestroyInstance(map3dInstances[key].g_instance_ref);
      Log::logMsg( fmt::format("Destroyed Instance: {}\n", key) ); // debug
    }
  }
  // unload 3d objects from map3dInstance
  for ( const auto &key : map3dInstances | std::views::keys )
  {
    if (map3dInstances[key].g_object_ref)
    {
      Log::logMsg("Unload 3D file: " + map3dInstances[key].getName() + "\n"); // debug
      XPLMUnloadObject(map3dInstances[key].g_object_ref);
    }
  }

  listDisplayStatic3dInstances.clear();
  listDisplayMoving3dInstances.clear();
}

// -------------------------------------

void
data_manager::refresh3DInstancesAndPointLocation()
{
  for ( auto &instObj : map3dInstances | std::views::values )
  {
    // v3.303.11 changed from "data_manager::map3dInstances[xx]." to "instObj.". Should be the same
    instObj.calculate_real_elevation_to_DisplayCoordination();
    instObj.positionInstancedObject();
  }

  // v3.0.203
  if (!currentLegName.empty())
  {
    auto itEnd = mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg.end();
    for (auto it = mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg.begin(); it != itEnd; ++it)
    {
      it->wasCalculated = false;
      it->refreshPointsAndElevBasedTerrain();
    }
  }
}

// -------------------------------------

void
data_manager::setSharedDatarefData()
{
  // Pick relevant information from FMS (lat/long)
  // Check leg xml stored node for "hover" and "type" of leg.
  int      altitude_mt_i       = 0;
  int      fms_entry_i         = XPLMGetDestinationFMSEntry();
  int      fms_entry_counter_i = XPLMCountFMSEntries();
  // int fms_entry_displayed_i = XPLMGetDisplayedFMSEntry(); // only for debug

  NavAidInfo navAid;

  if (fms_entry_counter_i > 0 && fms_entry_i == 0)
  {
    ++fms_entry_i;
  }


  //// Prepare the information for the shared data
  XPLMGetFMSEntryInfo(fms_entry_i, &navAid.navType, navAid.ID, &navAid.navRef, &altitude_mt_i, &navAid.lat, &navAid.lon);
  navAid.height_mt = static_cast<float> ( altitude_mt_i );

  if (Utils::isElementExists(mapFlightLegs, currentLegName))
  {
    int      task_type_i   = 0;
    int      need_to_hover = 0;
    IXMLNode xml_legSpeciallNode_ptr;

    xml_legSpeciallNode_ptr = mapFlightLegs[currentLegName].xmlSpecialDirectives_ptr;
    if (!xml_legSpeciallNode_ptr.isEmpty())
    {
      // Need to hover
      const std::string template_s = Utils::stringToLower(Utils::readAttrib(xml_legSpeciallNode_ptr, mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE(), "")); // Leg template="hover/land/start"
      if (template_s == mxconst::get_FL_TEMPLATE_VAL_HOVER())
        need_to_hover = 1;
      else if (template_s == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER()) // v25.02.1
        need_to_hover = 2;

      // task_type
      const std::string task_type_s = Utils::stringToLower(Utils::readAttrib(xml_legSpeciallNode_ptr, mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE(), "")); // medevac=1/delivery=2
      if (task_type_s == mxconst::get_GENERATE_TYPE_MEDEVAC())
        task_type_i = 1;
      else if (!task_type_s.empty())
        task_type_i = 2;

      // v3.0.221.9 target pos
      const std::string target_pos = Utils::readAttrib(xml_legSpeciallNode_ptr, mxconst::get_ATTRIB_TARGET_POS(), "");
      if (!target_pos.empty())
      {
        const std::vector<std::string> vecSplit = mxUtils::split_v2(target_pos, "|");
        if (vecSplit.size() > 1)
        {
          const std::string& lat_s = vecSplit.front();
          const std::string& lon_s = vecSplit.at(1);

          if (!lat_s.empty() && !lon_s.empty())
          {
            auto   lat_d         = Utils::stringToNumber<double> ( lat_s, static_cast<int> ( lat_s.length () ) );
            auto   lon_d         = Utils::stringToNumber<double> ( lon_s, static_cast<int> ( lon_s.length () ) );
            double distance_nm_d = Utils::calcDistanceBetween2Points_nm(navAid.lat, navAid.lon, lat_d, lon_d);

            double distance_meters_d = distance_nm_d * nm2meter;
            if (distance_meters_d > 20.0)
            {
              Log::logMsg("\t[setSharedData] override GPS NavAid with special_goal_directives. Distance: " + Utils::formatNumber<double>(distance_meters_d) + "mt. Before: " + navAid.get_latLon() + ", after: " + lat_s + "," + lon_s);
              navAid.lat = Utils::stringToNumber<float>(lat_s, static_cast<int> ( lat_s.length () ) );
              navAid.lon = Utils::stringToNumber<float>(lon_s, static_cast<int> ( lon_s.length () ) );
            }
            else
            {
              Log::logMsg("\t[setSharedData] USING GPS NavAid. Distance: " + Utils::formatNumber<double>(distance_meters_d) + "mt. GPS coordinates: " + navAid.get_latLon() + ", <special>: " + lat_s + "," + lon_s);
            }
          }
        }
      }
    }

#ifndef RELEASE
    Log::logMsg("\nwwwwwwwwww Writing to shared datarefs  wwwwwwwwwwww\n");
#endif


    /// set shared data
    std::string dref_name;

    dref_name = "xpshared/target/lat"; // holds target latitude
    if (Utils::isElementExists(mapSharedParams, dref_name))
      mapSharedParams[dref_name].setValue(navAid.lat);

    dref_name = "xpshared/target/lon"; // holds target longitude
    if (Utils::isElementExists(mapSharedParams, dref_name))
      mapSharedParams[dref_name].setValue(navAid.lon);

    dref_name = "xpshared/target/type"; // holds expected action type: 0: not set, 1: medevac, 2: delivery
    if (Utils::isElementExists(mapSharedParams, dref_name))
      mapSharedParams[dref_name].setValue(task_type_i);

    dref_name = "xpshared/target/need_to_hover"; // Do we need to hover ? 0=land 1=hover or 0=false 1=true
    if (Utils::isElementExists(mapSharedParams, dref_name))
      mapSharedParams[dref_name].setValue(need_to_hover);

    dref_name = "xpshared/target/status"; // holds result from other plugin.  -1: failed, 0: not set, waiting, 1: success
    if (Utils::isElementExists(mapSharedParams, dref_name))
      mapSharedParams[dref_name].setValue(0);

    // apply the values
    for (auto [fst, snd] : mapSharedParams)
    {
      if (fst == "xpshared/target/listen_plugin_available")
        continue;

      dataref_param::set_dataref_values_into_xplane(snd);
    }
  }
}


// -------------------------------------

std::string
data_manager::execute_commands(const std::string& inCommands)
{
  std::string err;
  err.clear();
  int counter = 0;

  std::list<std::string> listCommands = parseStringToCommandRef(inCommands);
  for (const auto& xp_command : listCommands)
  {
    queCommands.push_back(xp_command);
    ++counter;
  } // end for loop

  if (counter > 0)
    queFlcActions.push(mx_flc_pre_command::execute_xp_command);

  return err;
}

// -------------------------------------

void
data_manager::inject_metar_file(const std::string& inFileName)
{
  if (!inFileName.empty())
  {
    metar_file_to_inject_s = inFileName;
    queFlcActions.push(mx_flc_pre_command::inject_metar_file); // v3.0.223.1
  }
}

// -------------------------------------

void
data_manager::parse_leg_dynamic_messages(Waypoint& inLeg)
{
  static int         seq = 1;
  static std::string err;
  err.clear();

  std::string sourceTriggerName;
  bool        flag_base_on_external_plugin = false;

  auto cLogNode = Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode ( xLoadInfoNode.node, mxconst::get_ELEMENT_LEG(),  mxconst::get_ATTRIB_NAME(), inLeg.getName () );
  assert ( cLogNode.isEmpty () == false && fmt::format ( "[{}] XML Log Node must not be empty.", __func__ ).c_str () );


  for (auto& xNode : inLeg.list_raw_dynamic_messages_in_leg) // loop over <IXMLNode> of <dynamic_message>s
  {
    std::string message_to_call     = Utils::readAttrib(xNode, mxconst::get_ATTRIB_MESSAGE_NAME_TO_CALL(), "", true);
    std::string relative_to_task    = Utils::readAttrib(xNode, mxconst::get_ATTRIB_RELATIVE_TO_TASK(), "", true);
    std::string relative_to_trigger = Utils::readAttrib(xNode, mxconst::get_ATTRIB_RELATIVE_TO_TRIGGER(), "", true);
    std::string sound_file_name     = Utils::readAttrib(xNode, mxconst::get_ATTRIB_SOUND_FILE(), "", true); // v3.0.303.7 force sound file name for dynamic_messages, useful with the auto text_to_speach script

    bool        mMute          = Utils::readBoolAttrib(xNode,   mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR(), false);
    std::string mLabel         = Utils::readAttrib(xNode, mxconst::get_ATTRIB_LABEL(), "radio");
    std::string mLabelPosition = Utils::readAttrib(xNode, mxconst::get_ATTRIB_LABEL_PLACEMENT(), "L");
    std::string mLabelColor    = Utils::readAttrib(xNode, mxconst::get_ATTRIB_LABEL_COLOR(), mxconst::get_WHITE());

    double      length_mt   = Utils::readNumericAttrib(xNode, mxconst::get_ATTRIB_LENGTH_MT(), 0.0);
    std::string messageText = Utils::xml_get_text(xNode); //  xNode.getText();

    // v3.305.3 extends
    std::string sScriptNameWhenFired = Utils::readAttrib(xNode, mxconst::get_ATTRIB_SCRIPT_NAME_WHEN_FIRED(), "");
    std::string sScriptNameWhenLeft  = Utils::readAttrib(xNode, mxconst::get_ATTRIB_SCRIPT_NAME_WHEN_LEFT(), "");


    std::string newMsgName;

    // create new <message> based on the <dynamic..> attributes if mxconst::get_ATTRIB_MESSAGE_NAME_TO_CALL() is not set
    if (message_to_call.empty() && !messageText.empty())
    {
      Message m;
      newMsgName = mxconst::get_PREFIX_DYN_MESSAGE_NAME() + inLeg.getName() + "_" + Utils::formatNumber<int>(seq);

      m.node = Utils::xml_get_node_from_XSD_map_as_acopy( mxconst::get_ELEMENT_MESSAGE ()); // debug prepare message node

      m.setName(newMsgName);

      m.setNodeProperty<bool>(mxconst::get_PROP_IS_MXPAD_MESSAGE(), true);
      m.setNodeProperty<bool>(mxconst::get_ATTRIB_ENABLED(), true);

      m.xml_nodeTextTrack_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(m.node, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxconst::get_CHANNEL_TYPE_TEXT(), false);
      if (!m.xml_nodeTextTrack_ptr.isEmpty())
      {
        Utils::xml_add_cdata(m.xml_nodeTextTrack_ptr, messageText);

        m.setNodeProperty<bool>(m.xml_nodeTextTrack_ptr, mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR(), mMute, mxconst::get_ELEMENT_MIX());
        m.setNodeStringProperty(mxconst::get_ATTRIB_NAME(), newMsgName, m.xml_nodeTextTrack_ptr);
        m.setNodeProperty<bool>(m.xml_nodeTextTrack_ptr, mxconst::get_ATTRIB_MESSAGE_HIDE_TEXT(), false, mxconst::get_ELEMENT_MIX());

        m.setNodeStringProperty(mxconst::get_ATTRIB_LABEL(), mLabel, m.xml_nodeTextTrack_ptr);
        m.setNodeStringProperty(mxconst::get_ATTRIB_LABEL_PLACEMENT(), mLabelPosition, m.xml_nodeTextTrack_ptr);
        m.setNodeStringProperty(mxconst::get_ATTRIB_LABEL_COLOR(), mLabelColor, m.xml_nodeTextTrack_ptr);
      }

      // v3.0.303.7 add force "sound_file" based on dynamic_message attribute "sound_file"
      m.xml_nodeTextTrack_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(m.node, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxconst::get_CHANNEL_TYPE_COMM(), false);
      if (!m.xml_nodeTextTrack_ptr.isEmpty())
      {
        m.xml_nodeTextTrack_ptr.updateAttribute(sound_file_name.c_str(), mxconst::get_ATTRIB_SOUND_FILE().c_str(), mxconst::get_ATTRIB_SOUND_FILE().c_str());
      }

      if (!Utils::isElementExists(mapMessages, newMsgName) && m.parse_node()) // v3.0.241.2 added parse node for dynamic messages
      {
        Utils::addElementToMap(mapMessages, newMsgName, m);
        message_to_call = newMsgName; // we need this so code will check the relative attribute part of the function too.
        #ifndef RELEASE
        Utils::xml_print_node(m.node); // debug v3.303.8
        #endif
      }
      else
        newMsgName.clear();
    }

    // check the relative attributes
    if (!message_to_call.empty() && Utils::isElementExists(mapMessages, message_to_call)) // if message to call exists
    {
      if (!relative_to_task.empty()) // loop over all tasks and search for same name task. Will pick first one that it finds
      {
        for (auto& objName : inLeg.listObjectivesInFlightLeg)
        {
          if (Utils::isElementExists(mapObjectives[objName].mapTasks, relative_to_task))
          {
            auto& task                  = mapObjectives[objName].mapTasks[relative_to_task];
            flag_base_on_external_plugin = Utils::readBoolAttrib(task.node, mxconst::get_ATTRIB_BASE_ON_EXTERNAL_PLUGIN(), false);
            sourceTriggerName           = Utils::readAttrib(task.node, mxconst::get_ATTRIB_BASE_ON_TRIGGER(), "");
          }

        } // end loop over objectives

      } // end relative_to_task has value
      else if (!relative_to_trigger.empty())
      {
        if (Utils::isElementExists(mapTriggers, relative_to_trigger))
          sourceTriggerName = relative_to_trigger;
      }
    }

    if (sourceTriggerName.empty() && !newMsgName.empty())
    {
      if (Utils::isElementExists(mapMessages, newMsgName)) // remove the new message we created at the beginning of the function and use the predefined one.
        mapMessages.erase(newMsgName);
    }
    else if (!sourceTriggerName.empty() && !flag_base_on_external_plugin)
    {
      // get trigger position
      if (Utils::isElementExists(mapTriggers, sourceTriggerName))
      {
        Trigger newTrigger;
        newTrigger.node = mapTriggers[sourceTriggerName].node.deepCopy(); // this should be a clone of trigger_name
        // change name to: "trig_leg_message_{leg_name}_on_[task_name | trigger_name]_{seq}
        std::string newCustomTrigName = mxconst::get_PREFIX_TRIG_DYN_MESSAGE_NAME() + inLeg.getName() + "_on_" + ((relative_to_task.empty()) ? "trigger_" + relative_to_trigger : "task_" + relative_to_task) + "_" + Utils::formatNumber<int>(seq);


        newTrigger.setNodeStringProperty(mxconst::get_ATTRIB_NAME(), newCustomTrigName);
        newTrigger.xml_subNodeDiscovery(); // v3.0.241.2
        newTrigger.clear_condition_properties();
        newTrigger.clear_outcome_properties();
        // v3.0.241.2 converted to node centric
        newTrigger.setNodeStringProperty(mxconst::get_ATTRIB_TYPE(), mxconst::get_TRIG_TYPE_RAD());
        newTrigger.setNodeProperty<bool>(mxconst::get_PROP_IS_LINKED(), false);

        if (!newTrigger.xRadius.isEmpty())
          Utils::xml_set_attribute_in_node<double>(newTrigger.xRadius, mxconst::get_ATTRIB_LENGTH_MT(), length_mt, newTrigger.xRadius.getName());

        if (!newTrigger.xConditions.isEmpty())
          Utils::xml_set_attribute_in_node_asString(newTrigger.xConditions, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "", newTrigger.xConditions.getName());

        if (!newTrigger.xOutcome.isEmpty())
        {
          Utils::xml_set_attribute_in_node_asString(newTrigger.xOutcome, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED(), message_to_call, newTrigger.xOutcome.getName()); // string value
          // v3.305.1b we should not use the same physical message as the source trigger. We could copy it from the dynamic message if it was defined.
          // This will solve a bug where the dynamic message will broadcast its source physical message, which is an unwanted behavior.
          // v3.305.3 deprecated, we correctly clear the xOutcome node
          // Utils::xml_set_attribute_in_node_asString(newTrigger.xOutcome, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_ENTERING_PHYSICAL_AREA(), Utils::readAttrib(xNode, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_ENTERING_PHYSICAL_AREA(), ""), newTrigger.xOutcome.getName());

          // v3.305.3 added support for script_name_when_fired/left in dynamic message
          if (!sScriptNameWhenFired.empty())
          {
            sScriptNameWhenFired = mxUtils::replaceAll(sScriptNameWhenFired, mxconst::REPLACE_KEYWORD_SELF, newCustomTrigName);
            Utils::xml_set_attribute_in_node_asString(newTrigger.xOutcome, mxconst::get_ATTRIB_SCRIPT_NAME_WHEN_FIRED(), sScriptNameWhenFired, newTrigger.xOutcome.getName()); // string value
          }

          if (!sScriptNameWhenLeft.empty())
          {
            sScriptNameWhenLeft = mxUtils::replaceAll(sScriptNameWhenLeft, mxconst::REPLACE_KEYWORD_SELF, newCustomTrigName);
            Utils::xml_set_attribute_in_node_asString(newTrigger.xOutcome, mxconst::get_ATTRIB_SCRIPT_NAME_WHEN_LEFT(), sScriptNameWhenLeft, newTrigger.xOutcome.getName()); // string value
          }
        }

        if (newTrigger.parse_node()) // v3.0.241.2
        {
          newTrigger.isLinked = false; // I think we need to define this first since applyPropertyToLocal() will use the bool flag to assign the attribute. Strange.
          newTrigger.setNodeProperty<bool>(mxconst::get_PROP_IS_LINKED(), false);


          if (!newTrigger.deqPoints.empty())
            newTrigger.deqPoints.at(0).setElevationFt(0.0); // set on ground

          #ifndef RELEASE
          std::string txt = fmt::format("Adding New trigger '{}' for message: '{}'.", newCustomTrigName, newMsgName);
          Utils::xml_add_info_child(cLogNode, txt);
          #endif


          // add trigger to the pool of triggers and link to current flight_leg
          Utils::addElementToMap(mapTriggers, newCustomTrigName, newTrigger); // add new trigger to
          mapTriggers[newCustomTrigName].prepareCueMetaData();                // calculate cue

          inLeg.listTriggers.push_back(newCustomTrigName); // link flight leg to new trigger

          ++seq;
        }
      }
    } // end trigger name is not empty


  } // end loop over leg_messages
}

// -------------------------------------


void
data_manager::parse_leg_DisplayObjects(Waypoint& leg)
{
  ////////////////////////////
  // read 3d object instances

  // v3.305.3
  auto cLogNode = Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(xLoadInfoNode.node, mxconst::get_ELEMENT_LEG(), mxconst::get_ATTRIB_NAME(), leg.getName());
  assert(cLogNode.isEmpty() == false && fmt::format("[{}] XML Log Node must not be empty.", __func__).c_str());



  // Properties allowed to manipulate.
  const std::map<std::string, mx_property_type> mapOfAllowedReplaceAttributesInDisplayObject = { { mxconst::get_ATTRIB_COND_SCRIPT(), mx_property_type::MX_STRING },
                                                                                                           { mxconst::get_ATTRIB_LAT(), mx_property_type::MX_DOUBLE },
                                                                                                           { mxconst::get_ATTRIB_LONG(), mx_property_type::MX_DOUBLE },
                                                                                                           { mxconst::get_ATTRIB_ELEV_FT(), mx_property_type::MX_INT },
                                                                                                           { mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), mx_property_type::MX_DOUBLE },
                                                                                                           { mxconst::get_ATTRIB_DISTANCE_TO_DISPLAY_NM(), mx_property_type::MX_DOUBLE },
                                                                                                           { mxconst::get_ATTRIB_PITCH(), mx_property_type::MX_DOUBLE },
                                                                                                           { mxconst::get_ATTRIB_ROLL(), mx_property_type::MX_DOUBLE },
                                                                                                           { mxconst::get_ATTRIB_KEEP_UNTIL_GOAL(), mx_property_type::MX_STRING },
                                                                                                           { mxconst::get_ATTRIB_KEEP_UNTIL_LEG(), mx_property_type::MX_STRING },
                                                                                                           { mxconst::get_ATTRIB_HEADING_PSI(), mx_property_type::MX_DOUBLE } };

  int nChilds2 = leg.node.nChildNode();
  for (int i2 = 0; i2 < nChilds2; ++i2)
  {
    mx_base_node instProp; // will hold specific properties for a 3D instance
    instProp.node = leg.node.getChildNode(i2).deepCopy(); // v3.303.11 fetch all childs
    if (!instProp.node.isEmpty())
    {
      const std::string tag_s = instProp.node.getName();
      // filter childs
      if (tag_s != mxconst::get_ELEMENT_DISPLAY_OBJECT() && tag_s != mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE())
        continue;
      else if (tag_s == mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE()) // rename <display_object_near_plane> to <display_object> and force a fictive lat/long positioning
      {
        instProp.node.updateName(mxconst::get_ELEMENT_DISPLAY_OBJECT().c_str());
        // Force replace_lat / replace_long dummy values to be on the safe side
        instProp.node.updateAttribute("1.0", mxconst::get_ATTRIB_REPLACE_LAT().c_str(), mxconst::get_ATTRIB_REPLACE_LAT().c_str());
        instProp.node.updateAttribute("1.0", mxconst::get_ATTRIB_REPLACE_LONG().c_str(), mxconst::get_ATTRIB_REPLACE_LONG().c_str());
      }


      std::string instance_name = Utils::readAttrib(instProp.node, mxconst::get_ATTRIB_INSTANCE_NAME(), "");
      std::string obj3d_name    = Utils::readAttrib(instProp.node,  mxconst::get_ATTRIB_NAME(), "");
      std::string link_task     = Utils::readAttrib(instProp.node, mxconst::get_ATTRIB_LINK_TASK(), "");

      if (obj3d_name.empty() || !mxUtils::isElementExists(map3dObj, obj3d_name))
      {
        // v3.305.3
        const std::string txt = fmt::format("Found <display_object>: '{}' without 3d template name or template name not exists. Skipping instance...", obj3d_name);
        Utils::xml_add_warning_child(cLogNode, txt);

        addLoadErr(txt);

        // Log::logMsgErr(err_s);
        continue;
      }
      else if (instance_name.empty())
      {
        // v3.305.3
        const std::string txt = fmt::format("Found <display_object> without instance name, check: '{}'. Skipping instance...", obj3d_name);
        Utils::xml_add_warning_child(cLogNode, txt);

        addLoadErr(txt);
        // Log::logMsgErr(err_s);
        continue;
      }

      // Check if object 3D is dependent on a task in flight leg. Will be used later in "Mission::flc_3d_objects" to decide if to hide/show a 3D object.
      if ( const std::vector<std::string> vecLinkTask = mxUtils::split_v2 ( link_task, "." )
        ; !link_task.empty() && (vecLinkTask.size() >= 2))
      {
        instProp.setStringProperty(mxconst::get_PROP_LINK_OBJECTIVE_NAME(), vecLinkTask.at(0)); // save instance information at the Display Object element level
        instProp.setStringProperty(mxconst::get_ATTRIB_LINK_TASK(), vecLinkTask.at(1));
      }


      // read special Instance REPLACE attributes and store them
      int nAttrib = instProp.node.nAttribute(); // read number of attributes inside "display_object" element

      auto iterEnd = mapOfAllowedReplaceAttributesInDisplayObject.cend(); // hold the end of the map

      for (int i1 = 0; i1 < nAttrib; ++i1) // loop over attributes and search for the ones with "replace_" in them
      {

        IXMLAttr          attrib     = instProp.node.getAttribute(i1);
        const std::string val        = attrib.sValue;
        const std::string attribName = mxUtils::stringToLower(attrib.sName);

        if (attribName.find(mxconst::get_PREFIX_REPLACE_()) == static_cast<size_t> ( 0 ) ) // v3.0.217.3 use "find" instead of "find_first_of". Fixed bug when replacing "replace_xxx" attribute. Also fixed in v3.0.215.7
        {
          if (attribName.length() <= mxconst::get_PREFIX_REPLACE_().length())
            continue; // attribute name is probably not valid to hold replace_xxx


          const std::string attribWithoutPrefix = attribName.substr(mxconst::get_PREFIX_REPLACE_().length());
          instProp.node.updateAttribute(attrib.sValue, attribWithoutPrefix.c_str(), attrib.sName); // v3.0.241.1 rename the "replace_" attribute with the real valid name. This way we should be able to use them later on

          // search for the specific value in the map and if it was found then continue processing, if not then skip with log message.
          const auto& propIter = mapOfAllowedReplaceAttributesInDisplayObject.find(attribWithoutPrefix);
          if (propIter == iterEnd)
          {

            // v3.305.3
            const std::string txt = fmt::format("Instance: '{}', Found unrecognized replacement attribute name: '{}'. Fix the attribute name or ask the developer to support it.", instance_name, attribWithoutPrefix);
            Utils::xml_add_warning_child(cLogNode, txt);
            continue;
          }
          else
          {
            switch ((*propIter).second)
            {
              case mx_property_type::MX_STRING:
              {
                instProp.setStringProperty((*propIter).first, val);
              }
              break;
              case mx_property_type::MX_DOUBLE:
              {
                if (!val.empty()) // v3.303.11
                {
                  auto val_n = Utils::stringToNumber<double>(val);
                  instProp.setNodeProperty<double>((*propIter).first, val_n);
                }
              }
              break;
              case mx_property_type::MX_INT:
              {
                if (!val.empty()) // v3.303.11
                {
                  int val_n = Utils::stringToNumber<int>(val);
                  instProp.setNodeProperty<int>((*propIter).first, val_n);
                }
              }
              break;
              default:
              {
                // Log::logMsgWarn("Attribute: " + mxconst::get_QM() + attribWithoutPrefix + mxconst::get_QM() + ", is not a valid replace attribute. skipping...");
              }
              break;
            } // end switch
          }


        } // end if found prefix_
      }   // end loop over attributes



      // v3.0.253.7 Fix bug where missing <display_object> lat or lon were not copied over from their Obj3D if they do not exists
      bool flag_found = false;
      if (Utils::readAttrib(instProp.node, mxconst::get_ATTRIB_LAT(), "").empty())
      {
        const std::string obj_lat = Utils::xml_get_attribute_value_drill(map3dObj[obj3d_name].node, mxconst::get_ATTRIB_LAT(), flag_found, mxconst::get_ELEMENT_LOCATION());
        if (flag_found)
          instProp.setNodeProperty<double>(mxconst::get_ATTRIB_LAT(), mxUtils::stringToNumber<double>(obj_lat));
      }
      if (Utils::readAttrib(instProp.node, mxconst::get_ATTRIB_LONG(), "").empty())
      {
        flag_found                 = false;
        const std::string obj_long = Utils::xml_get_attribute_value_drill(map3dObj[obj3d_name].node, mxconst::get_ATTRIB_LONG(), flag_found, mxconst::get_ELEMENT_LOCATION());
        if (flag_found)
          instProp.setNodeProperty<double>(mxconst::get_ATTRIB_LONG(), mxUtils::stringToNumber<double>(obj_long));
      }

      // add instance to 3D Object as a sub-element for future use (if the instance is not part of it, yet).
      if (map3dObj[obj3d_name].node.getChildNodeWithAttribute(mxconst::get_ELEMENT_DISPLAY_OBJECT().c_str(), mxconst::get_ATTRIB_INSTANCE_NAME().c_str(), instance_name.c_str()).isEmpty())
        map3dObj[obj3d_name].node.addChild(instProp.node.deepCopy()); // add display objects element to the 3D Object as a future instance

      // add instance to Flight Leg set
      Utils::addElementToMap(leg.list_displayInstances, instance_name, obj3d_name); // v3.305.1 converted list to a map
    }

  } // end loop <display_object>
}

// -------------------------------------

void
data_manager::translate_relative_3d_display_object_text(std::string& inOutText)
{
  // ELEMENT_DISPLAY_OBJECT_NEAR_PLANE
  std::map<std::string, dataref_param> mapKeywords{ { "{wing_span}", dataref_param("sim/aircraft/view/acf_size_x") }, { "{acf_psi}", dataref_param("sim/flightmodel/position/psi") } };

  for (auto& [key, dref] : mapKeywords)
  {
    if (dref.flag_paramReadyToBeUsed)
    {
      dref.readDatarefValue_into_missionx_plugin();
      inOutText = Utils::replaceStringWithOtherString(inOutText, key, dref.get_dataref_scalar_value_as_string(), true);
    }
  }

  //// Do the same using mission dtarefs and not just pre-defined ones
  for (auto& [key, dref] : mapDref)
  {
    if (dref.flag_paramReadyToBeUsed && dref.dataRefType < xplmType_FloatArray) // if not an array or data (string)
    {
      dref.readDatarefValue_into_missionx_plugin();
      inOutText = Utils::replaceStringWithOtherString(inOutText, key, dref.get_dataref_scalar_value_as_string(), true);
    }
  }
}


// -------------------------------------


void
data_manager::refresh_3d_objects_and_cues_after_location_transition()
{

  if (missionState >= mx_mission_state_enum::pre_mission_running)
  {
    #ifndef RELEASE
    Timer timer;
    Timer::start(timer);
    Log::logMsgWarn("Mission-X XPLM_MSG_SCENERY_LOADED"); // debug
    #endif

    // prepare Cue points
    resetCueSettings();
    mapFlightLegs[currentLegName].flag_cue_was_calculated = false;
    // re-calculate 3D objects local + re-position + Cue Points
    refresh3DInstancesAndPointLocation();

    #ifndef RELEASE
    Timer::wasEnded(timer);
    Log::logMsg("\n refresh took: " + Utils::formatNumber<float>(timer.getOsSecondsPassed(), 6) + "\n"); // debug
    Log::logMsgWarn("END Mission-X XPLM_MSG_SCENERY_LOADED"); // debug
    #endif
  }
}

// -------------------------------------


bool
data_manager::init_xp_airport_db()
{

  const std::string path_to_airports_db_s = mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", errStr) + "/" + mxconst::get_DB_FOLDER_NAME() + "/" + mxconst::get_DB_AIRPORTS_XP();
  if (db_connect(db_xp_airports, path_to_airports_db_s))
  {
    Log::logMsg("[db] Connected to: " + path_to_airports_db_s);

    return true;
  }

  return false;
}

// -------------------------------------

bool
data_manager::init_xp_airport_db2()
{
  // file::memory:?cache=shared <=== https://sqlite.org/inmemorydb.html
  const std::string path_to_airports_db_s = "file::memory:?cache=shared"; // sqlite in memory database name should be ":memory:"
  if (db_connect(db_cache, path_to_airports_db_s))
  {
    Log::logMsg("[db] Connected to: " + path_to_airports_db_s);
    return true;
  }

  return false;
}

// -------------------------------------

bool
data_manager::delete_db_file(const std::string& inPathAndFile)
{
  fs::path dbFile = inPathAndFile;
  if (exists(dbFile))
  {
    if (is_regular_file(dbFile))
    {
      try
      {
        return fs::remove(dbFile);
      }
      catch (std::filesystem::filesystem_error const& ex)
      {

        Log::logMsg("Delete File Error:  " + std::string(ex.what()) + ", message: " + ex.code().message());
        return false;
      }
    }
    else
      return false;
  }

  return true;
}

// -------------------------------------

bool
data_manager::init_stats_db()
{
  const std::string path_to_stats_db_s = mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", errStr) + mxconst::get_DB_FOLDER_NAME() + "/" + mxconst::get_DB_STATS_XP();
  if (db_connect(db_stats, path_to_stats_db_s))
  {
    Log::logMsg("[db] Connected to: " + path_to_stats_db_s);
    return true;
  }

  return false;
}

// -------------------------------------

bool
data_manager::db_connect(dbase& db, const std::string& inPathToDbFile)
{

  db.set_db_path_and_file(inPathToDbFile);
  if (db.open_database())
    return true;
  else
    Log::logMsgErr("[data_manager] Failed to connect to database file: " + inPathToDbFile);

  return false;
}

// -------------------------------------

void
data_manager::db_close_all_databases()
{
  db_xp_airports.close_database();
  db_stats.close_database(); // v3.0.255.1
  db_cache.close_database(); // v3.0.255.3
}

// -------------------------------------

void
data_manager::db_close_database(dbase& inDB)
{
  if (inDB.db && inDB.db_is_open_and_ready)
    inDB.close_database();
}


// -------------------------------------

void
data_manager::fetch_ils_rw_from_sqlite(NavAidInfo* inFrom_navaid, std::string* inFilterQuery, mxFetchState_enum* outState, std::string* outStatusMessage)
{
  outStatusMessage->clear();
  std::lock_guard<std::mutex> lock(s_thread_sync_mutex);

  // std::string query = (*inQuery);

  // v24.03.1 add support for custom "base query" for the ILS search screen
  std::string sFilterQuery = (*inFilterQuery);
  // v24.06.1 added xx_rank column as an additional sorting
  std::string query = R"(select icao, round(distance_nm) as distance_nm, loc_rw, loc_type, frq_mhz, loc_bearing, rw_length_mt, rw_width, ap_elev, ap_name, surf_type_text, bearing_from_to_icao
from (
select xp_loc.icao
      , mx_calc_distance ({}, {}, xp_loc.lat, xp_loc.lon, 3440) as distance_nm
      , xp_loc.loc_rw, xp_loc.loc_type, xp_loc.frq_mhz, xp_loc.loc_bearing, xp_rw.rw_length_mt, xp_rw.rw_width, xa.ap_elev, xa.ap_name
      , case xp_rw.rw_surf when 1 then 'Asphalt' when 2 then 'Concrete' when 3 then 'Turf or grass' when 4 then 'Dirt' when 5 then 'Gravel' when 12 then 'Dry lakebed' when 13 then 'Water runways' when 14 then 'Snow or ice' when 15 then 'Transparent' else 'other' end as surf_type_text
      , mx_bearing({}, {}, xp_loc.lat, xp_loc.lon) as bearing_from_to_icao
from xp_loc, xp_rw, xp_airports xa
where xp_rw.icao = xp_loc.icao
and (xp_rw.rw_no_1 = xp_loc.loc_rw or xp_rw.rw_no_2 = xp_loc.loc_rw)
and xa.icao = xp_rw.icao
)
where 1 = 1
)";

  if (init_xp_airport_db() && db_xp_airports.db_is_open_and_ready)
  {

    //////////////////////////////////////////
    // read custom SQL first from sql.xml file
    //////////////////////////////////////////
    const std::string stmt_base_ils_search_query_name = "base_ils_search_query"; // v24.03.1
    const std::string stmt_uq_name                    = "ils_search_query";      // v24.03.1 renamed the statement final name.

    // v24.03.1 Reading SQL from sql.xml file
    Utils::read_external_sql_query_file(mapQueries, mxconst::get_SQLITE_ILS_SQLS());
    if (mxUtils::isElementExists(mapQueries, stmt_base_ils_search_query_name))
    {
      query = mapQueries[stmt_base_ils_search_query_name];
      #ifndef RELEASE
      Log::logMsgThread("Picked query from sql.xml file.");
      #endif
    }

    query += " " + sFilterQuery;                                                     // Construct the full query
    if (!mxUtils::isElementExists(mapQueries, stmt_uq_name)) // add the query to mapQueries
      mapQueries[stmt_uq_name] = query;

    // Replace the BINDINGS
    // v24.03.1 the query will use the fmt::format library to modify the query, instead of using bindings
    std::map<int, std::string> mapArgs = { { 1, inFrom_navaid->getLat() }, { 2, inFrom_navaid->getLon() }, { 3, inFrom_navaid->getLat() }, { 4, inFrom_navaid->getLon() } };
    query                              = mxUtils::format(query, mapArgs);

    #ifndef RELEASE
    Log::logMsgThread("[ils layer] Query:\n" + query);                                    // debug
    Log::logMsgThread("[ils layer] Query lat/lon:" + inFrom_navaid->get_latLon() + "\n"); // debug
    #endif

    // prepare query statement
    if (db_xp_airports.prepareNewStatement(stmt_uq_name, query))
    {
      assert(data_manager::db_xp_airports.mapStatements[stmt_uq_name] != nullptr);


      // v24.03.1 DEPRECATED bindings, since we are using "fmt::format" library with "{}"
      // sqlite3_bind_double(data_manager::db_xp_airports.mapStatements[stmt_uq_name], 1, inFrom_navaid->lat); // 1 Bindings start in 1
      // sqlite3_bind_double(data_manager::db_xp_airports.mapStatements[stmt_uq_name], 2, inFrom_navaid->lon); // 2
      // sqlite3_bind_double(data_manager::db_xp_airports.mapStatements[stmt_uq_name], 3, inFrom_navaid->lat); // 3
      // sqlite3_bind_double(data_manager::db_xp_airports.mapStatements[stmt_uq_name], 4, inFrom_navaid->lat); // 4 with the use of mx_calc_distance() in the new query we do not need this binding
      // sqlite3_bind_double(data_manager::db_xp_airports.mapStatements[stmt_uq_name], 5, inFrom_navaid->lon); // 5

      int seq = 0;
      while (sqlite3_step(db_xp_airports.mapStatements[stmt_uq_name]) == SQLITE_ROW)
      {
        mx_ils_airport_row_strct row;

        row.seq = seq;

        int i        = 0;
        row.toICAO_s = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], i)));
        i++; // 0 columns start in 0
        row.distnace_d = sqlite3_column_double(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++; // 1 distance from start location
        row.loc_rw_s = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], i)));
        i++; // 2 on which runway ?
        row.locType_s = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], i)));
        i++; // 3 type
        row.loc_frq_mhz = sqlite3_column_int(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++; // 4 frq mhz
        row.loc_bearing_i = sqlite3_column_int(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++; // 5 localizer bearing
        row.rw_length_mt_i = sqlite3_column_int(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++; // 6 rw length mt
        row.rw_width_d = sqlite3_column_double(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++; // 7 rw width
        row.ap_elev_ft_i = sqlite3_column_int(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++; // 8 rw length mt
        row.toName_s = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], i)));
        i++; // 8 airport name
        row.surfType_s = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], i)));
        i++; // 9 surface type v3.0.253.13
        row.bearing_from_to_icao_d = sqlite3_column_double(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++; // 10 bearing_from_to_icao_d

        table_ILS_rows_vec.emplace_back(row);
        Utils::addElementToMap(indexPointer_for_ILS_rows_tableVector, seq, &table_ILS_rows_vec.back());

        seq++;
      }

      if (seq == 0)
        (*outStatusMessage) = "!!! No Rows Found !!!"; // success message
      else
        (*outStatusMessage) = fmt::format("Fetched: {} rows", seq); // success message // v24.03.1 replaced with fmt
    }
    else
    {
      (*outStatusMessage) = "Failed to prepare filter statement. Notify developer !!!";
    }
  }
  else
  {
    (*outStatusMessage) = "Could not work on database. Consider rebuilding using apt.dat optimization.\n" + db_xp_airports.last_err;
  }

  (*outState) = mxFetchState_enum::fetch_ended; // once we change the state, the UI can show/hide the relevent layers/widgets
}

// -------------------------------------

void
data_manager::fetch_nav_data_from_sqlite(std::unordered_map<int, mx_nav_data_strct>* outMapNavaidData, std::string* inICAO, Point* inPlanePos, mxFetchState_enum* outState, std::string* outStatusMessage)
{

  std::string sICAO = (*inICAO);

  outStatusMessage->clear();
  std::lock_guard<std::mutex> lock(s_thread_sync_mutex);


  if (init_xp_airport_db() && db_xp_airports.db_is_open_and_ready)
  {
    std::string query;

    Utils::read_external_sql_query_file(mapQueries, mxconst::get_SQLITE_NAVDATA_SQLS());

    // construct the Query 1
    query = R"(select t1.icao_id
, FORMAT("%s (%s), coord: %3.4f/%3.4f (%ift) ", t1.ap_name, t1.icao, ap_lat, ap_lon, ap_elev) as data
, mx_calc_distance(t1.ap_lat, t1.ap_lon, {}, {}, 3440) as distance, t1.ap_lat, t1.ap_lon
from xp_airports t1
where 1 = 1
and icao = '{}' order by distance
)";


    // ------------
    //    Step 1
    // ------------

    std::string stmt_uq_name = "fetch_nav_data_step1";

    if (mxUtils::isElementExists(mapQueries, stmt_uq_name))
    {
      query = mapQueries[stmt_uq_name];
      #ifndef RELEASE
      Log::logMsgThread("Using fetch_nav_data_step1 from sql.xml file");
      #endif
    }
    else
      mapQueries[stmt_uq_name] = query;

    std::map<int, std::string> mapArgs = { { 1, inPlanePos->getLat_s() }, { 2, inPlanePos->getLon_s() }, { 3, sICAO } };
    query                              = mxUtils::format(query, mapArgs);

    // query = fmt::format(query.data(), inPlanePos->lat, inPlanePos->lon, sICAO);

    #ifndef RELEASE
    Log::logMsgThread(fmt::format("Nav Data Query 1:\n{}", query));
    #endif // !RELEASE

    // prepare query statement
    if (db_xp_airports.prepareNewStatement(stmt_uq_name, query))
    {
      assert(data_manager::db_xp_airports.mapStatements[stmt_uq_name] != nullptr && fmt::format("[{}] Statement '{}' was not created.", __func__, stmt_uq_name).c_str());

      int seq = 0;
      while (sqlite3_step(db_xp_airports.mapStatements[stmt_uq_name]) == SQLITE_ROW)
      {
        mx_nav_data_strct row;

        row.seq  = seq;
        row.icao = sICAO;

        int i       = 0; // 0 icao_id
        row.icao_id = sqlite3_column_int(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++; // 1 Airport description text to display
        row.sApDesc = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], i)));
        i++; // 2 Distance in nm, as number.
        row.dDistance = sqlite3_column_double(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++; // lat
        row.ap_lat = sqlite3_column_double(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++; // lon
        row.ap_lon = sqlite3_column_double(db_xp_airports.mapStatements[stmt_uq_name], i);
        i++;

        Utils::addElementToMap((*outMapNavaidData), row.icao_id, row);



        seq++;
      }

      const auto ap_rows = seq;


      if (ap_rows)
      {
        // Loop over ALL fetched airports
        for ( auto &data : *outMapNavaidData | std::views::values )
        {

          // ------------
          //    Step 2 - Runway data
          // ------------

          // Fetch runway data

          stmt_uq_name = "fetch_rw_info_step2";
          query        = R"(select t2.rw_no_1 || '/' || t2.rw_no_2 as rw_key
, FORMAT ("%-*s Length: %d meters", 10, t2.rw_no_1 || '/' || t2.rw_no_2, t2.rw_length_mt) as rw_data
, mx_calc_distance(t2.rw_no_1_lat, t2.rw_no_1_lon, {}, {}, 3440) as distance
from xp_rw t2
where 1 = 1
and icao_id = '{}'
)";

          if (mxUtils::isElementExists(mapQueries, stmt_uq_name))
          {
            query = mapQueries[stmt_uq_name];
            #ifndef RELEASE
            Log::logMsgThread("Using fetch_rw_info_step2 from sql.xml file");
            #endif
          }
          else
            mapQueries[stmt_uq_name] = query;


          // query = fmt::format(query.data(), inPlanePos->lat, inPlanePos->lon, data.icao_id); // prepare the query text
          std::map<int, std::string> map_args = { { 1, inPlanePos->getLat_s() }, { 2, inPlanePos->getLon_s() }, { 3, mxUtils::formatNumber(data.icao_id) } };
          query                               = mxUtils::format(query, map_args);


          #ifndef RELEASE
          Log::logMsgThread(fmt::format("Nav Data Query 2:\n{}", query));
          #endif // !RELEASE


          if (db_xp_airports.prepareNewStatement(stmt_uq_name, query))
          {
            assert(data_manager::db_xp_airports.mapStatements[stmt_uq_name] != nullptr && fmt::format("[{}] Statement '{}' was not created.", __func__, stmt_uq_name).c_str());
            while (sqlite3_step(db_xp_airports.mapStatements[stmt_uq_name]) == SQLITE_ROW)
            {
              mx_nav_data_strct::mx_row2_strct row;

              int i      = 0; // 0 runway key
              row.rw_key = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], i)));
              i++; // 1 rw_data
              row.desc = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], i)));
              i++; // 2 Distance in nm, as number.
              row.dDistance = sqlite3_column_double(db_xp_airports.mapStatements[stmt_uq_name], i);
              i++;

              Utils::addElementToMap(data.mapRunwayData, row.rw_key, row);
            }

          } // END Step 2



          // ------------
          //    Step 3 - VOR/NDB/DME
          // ------------
          {
            const std::string stmt_vor = "fetch_vor_ndb_dme_info_step3";

            // The query also deduce "VOR/DME" cases
            query = R"(WITH vor_dme AS (
  SELECT v1.ident, v1.loc_data, distance, 'VOR/DME' AS loc_type, v1.frq_mhz, identNameFrq, prevIdentNameFrq
    FROM (
           SELECT t3.ident,
                  FORMAT("%.2f (%i,%s) %s", CASE WHEN length(t3.frq_mhz) = 5 THEN t3.frq_mhz / 100.0 ELSE t3.frq_mhz END
                                          , mx_bearing({1}, {2}, t3.lat, t3.lon)
                                          , 'VOR/DME'/* constant */
                                          , t3.name) AS loc_data,
                  mx_calc_distance(t3.lat, t3.lon, {3}, {4}, 3440) AS distance,
                  t3.loc_type,
                  t3.frq_mhz,
                  IFNULL( (t3.ident || t3.name || t3.frq_mhz), t3.ident) AS identNameFrq,
                  lag(IFNULL( (t3.ident || t3.name || t3.frq_mhz), t3.ident)) OVER (ORDER BY t3.ident, t3.frq_mhz, t3.loc_type) AS prevIdentNameFrq,
                  lag(t3.loc_type) OVER (ORDER BY ident, frq_mhz, loc_type) AS prev_type
             FROM xp_loc t3
            WHERE distance <= 20
            and t3.loc_type in ('VOR', 'DME')
            ORDER BY distance, ident, frq_mhz, name
         )
         v1
   WHERE 1 = 1
     AND v1.identNameFrq = v1.prevIdentNameFrq
     AND ( (v1.loc_type = 'VOR' AND v1.prev_type = 'DME') OR
           (v1.loc_type = 'DME' AND v1.prev_type = 'VOR') )
)
SELECT v1.ident, v1.loc_data, v1.distance
  FROM vor_dme v1
UNION ALL
SELECT *
FROM (
    SELECT t1.ident
           , FORMAT("%.2f (%i,%s) %s", CASE WHEN length(t1.frq_mhz) = 5 THEN t1.frq_mhz / 100.0 ELSE t1.frq_mhz END, mx_bearing({5}, {6}, t1.lat, t1.lon), t1.loc_type, t1.name) AS loc_data
           , mx_calc_distance(t1.lat, t1.lon, {7}, {8}, 3440) as distance
      FROM xp_loc t1
     WHERE mx_calc_distance (t1.lat, t1.lon, {9}, {10}, 3440) <= 20
       AND t1.loc_type IN ('VOR', 'DME')
       AND IFNULL( (t1.ident || t1.name || t1.frq_mhz), t1.ident) not in (select identNameFrq
                                                                            from vor_dme)
    UNION ALL
    SELECT t1.ident,
          FORMAT("%i (%i,%s) %s", t1.frq_mhz
                                , mx_bearing({11}, {12}, t1.lat, t1.lon)
                                , t1.loc_type
                                , t1.name) AS loc_data
          , mx_calc_distance(t1.lat, t1.lon, {13}, {14}, 3440) as distance
     FROM xp_loc t1
    WHERE mx_calc_distance(t1.lat, t1.lon, {15}, {16}, 3440) <= 20
      AND t1.loc_type = 'NDB'
ORDER BY distance
)
)";


            if (mxUtils::isElementExists(mapQueries, stmt_vor))
            {
              query = mapQueries[stmt_vor];
              #ifndef RELEASE
              Log::logMsgThread("Using fetch_vor_ndb_dme_info_step3 from sql.xml file");
              #endif
            }
            else
              mapQueries[stmt_vor] = query;

            // prepare the query text
            // query = fmt::mx_format (query, data.ap_lat, data.ap_lon, data.ap_lat, data.ap_lon
            //                                        , data.ap_lat, data.ap_lon, data.ap_lat, data.ap_lon
            //                                        , data.ap_lat, data.ap_lon, data.ap_lat, data.ap_lon
            //                                        , data.ap_lat, data.ap_lon, data.ap_lat, data.ap_lon); // 16 {}

            std::map<int, std::string> local_map_args = { { 1, mxUtils::formatNumber(data.ap_lat) },  { 2, mxUtils::formatNumber(data.ap_lon) },  { 3, mxUtils::formatNumber(data.ap_lat) },  { 4, mxUtils::formatNumber(data.ap_lon) }
                                                       ,  { 5, mxUtils::formatNumber(data.ap_lat) },  { 6, mxUtils::formatNumber(data.ap_lon) },  { 7, mxUtils::formatNumber(data.ap_lat) },  { 8, mxUtils::formatNumber(data.ap_lon) }
                                                       ,  { 9, mxUtils::formatNumber(data.ap_lat) },  { 10, mxUtils::formatNumber(data.ap_lon) }, { 11, mxUtils::formatNumber(data.ap_lat) }, { 12, mxUtils::formatNumber(data.ap_lon) }
                                                       ,  { 13, mxUtils::formatNumber(data.ap_lat) }, { 14, mxUtils::formatNumber(data.ap_lon) }, { 15, mxUtils::formatNumber(data.ap_lat) }, { 16, mxUtils::formatNumber(data.ap_lon) }
                                                        };
            query                                     = mxUtils::format(query, local_map_args); // v25.03.3 fixed map name sent to query.


            #ifndef RELEASE
            Log::logMsgThread(fmt::format("Nav Data Query 3:\n{}", query));
            #endif // !RELEASE


            if (db_xp_airports.prepareNewStatement(stmt_vor, query))
            {
              assert(data_manager::db_xp_airports.mapStatements[stmt_vor] != nullptr && fmt::format("[{}] Statement '{}' was not created.", __func__, stmt_vor).c_str());
              while (sqlite3_step(db_xp_airports.mapStatements[stmt_vor]) == SQLITE_ROW)
              {
                mx_nav_data_strct::mx_row3_4_5_strct row;

                int i      = 0; // 0 ident
                row.field1 = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_vor], i)));
                i++; // desc - loc_data
                row.field2 = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_vor], i)));
                i++; // 1 Distance in nm
                row.dDistance = sqlite3_column_double(db_xp_airports.mapStatements[stmt_vor], i);
                i++; // skip rest of fields, we don't care about them

                data.listVor.emplace_back(row);
              }
            }
          } // END Step 3

          // ------------
          //    Step 4 - Runway Localizer
          // ------------
          {
            // NavAid table does not hold icao_id field, so we use icao field instead.
            const std::string stmt_loc = "fetch_loc_info_step4";
            query                      = R"(select loc_data, distance, loc_type_code, loc_rw
from (
select FORMAT ("%-*s , freq: %5.3f, Type: %s, Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz*0.01, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, {1}, {2}, 3440) as distance
, t3.loc_rw
, 1 as loc_type_code
from xp_loc t3
where icao = '{3}'
and ( t3.loc_type like ('ILS%' ) or t3.loc_type like ('LOC%' ) )
) v1
UNION ALL
select loc_data, distance, loc_type_code, loc_rw
from (
select FORMAT ("%-*s , channel: %i/%s, Type: %s, Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz, t3.ident, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, {4}, {5}, 3440) as distance
, t3.loc_rw
, 2 as loc_type_code
from xp_loc t3
where icao = '{6}'
and t3.loc_type not like ('ILS%' )
and t3.loc_type not like ('LOC%' )
and t3.loc_type not in ('DME', 'VOR', 'NDB' )
)
order by loc_type_code, loc_rw
)";

            if (mxUtils::isElementExists(mapQueries, stmt_loc))
            {
              query = mapQueries[stmt_loc];
              #ifndef RELEASE
              Log::logMsgThread("Using fetch_loc_info_step4 from sql.xml file");
              #endif
            }
            else
              mapQueries[stmt_loc] = query;


            // query = fmt::format(query.data(), inPlanePos->lat, inPlanePos->lon, sICAO); // prepare the query text
            std::map<int, std::string> map_args2 = { { 1, inPlanePos->getLat_s() }, { 2, inPlanePos->getLon_s() }, { 3, sICAO }
                                                   , { 4, inPlanePos->getLat_s() }, { 5, inPlanePos->getLon_s() }, { 6, sICAO } };
            query                                = mxUtils::format(query, map_args2);


            #ifndef RELEASE
            Log::logMsgThread(fmt::format("Nav Data Query 4:\n{}", query));
            #endif // !RELEASE


            if (db_xp_airports.prepareNewStatement(stmt_loc, query))
            {
              assert(data_manager::db_xp_airports.mapStatements[stmt_loc] != nullptr && fmt::format("[{}] Statement '{}' was not created.", __func__, stmt_loc).c_str());
              while (sqlite3_step(db_xp_airports.mapStatements[stmt_loc]) == SQLITE_ROW)
              {
                mx_nav_data_strct::mx_row3_4_5_strct row;

                int i      = 0; // 0 desc
                row.field2 = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_loc], i)));
                i++; // 1 Distance in nm
                row.dDistance = sqlite3_column_double(db_xp_airports.mapStatements[stmt_loc], i);
                i++;                                                                                       // 2 Localizer internala type. Either "one" or "two"
                row.iField1 = sqlite3_column_int(db_xp_airports.mapStatements[stmt_loc], i); // v24.06.1
                i++;                                                                                       // skip rest of fields, we don't care about them

                data.listLoc.emplace_back(row);
              }
            }
          } // END Step 4

          // ------------
          //    Step 5 - FREQ
          // ------------
          {

            const std::string stmt_frq = "fetch_ap_frq_step5";

            query = R"(select FORMAT ("%-*.3f %s", 10, xaf.frq/1000.0, xaf.frq_desc ) as frq_data
from xp_ap_frq xaf
where icao_id = {}
)";

            if (mxUtils::isElementExists(mapQueries, stmt_frq))
            {
              query = mapQueries[stmt_frq];
              #ifndef RELEASE
              Log::logMsgThread("Using fetch_ap_frq_step5 from sql.xml file");
              #endif
            }
            else
              mapQueries[stmt_frq] = query;


            // query = fmt::format(query.data(), data.icao_id); // prepare the query text
            std::map<int, std::string> map_args3 = { { 1, mxUtils::formatNumber(data.icao_id) } }; // v24.05.2
            query                              = mxUtils::format(query, map_args3);

            #ifndef RELEASE
            Log::logMsgThread(fmt::format("Nav Data Query 5:\n{}", query));
            #endif // !RELEASE


            if (db_xp_airports.prepareNewStatement(stmt_frq, query))
            {
              assert(data_manager::db_xp_airports.mapStatements[stmt_frq] != nullptr && fmt::format("[{}] Statement '{}' was not created.", __func__, stmt_frq).c_str());
              while (sqlite3_step(db_xp_airports.mapStatements[stmt_frq]) == SQLITE_ROW)
              {
                mx_nav_data_strct::mx_row3_4_5_strct row;

                int i      = 0; // 0 desc
                row.field2 = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_frq], i)));

                data.listFrq.emplace_back(row);
              }
            }
          } // END Step 5


        } // end loop over fetched airports from step 1

      } // end if ap_rows has positive value = we found airports


      if (ap_rows == 0)
        (*outStatusMessage) = "!!! No Rows Found !!!"; // success message
      else
      {
        (*outStatusMessage) = fmt::format("Fetched: {} nav data rows for ICAO: \"{}\". Trying to fetch METAR from flightplandatabase.com site.", ap_rows, sICAO); // success message
        postFlcActions.push(mx_flc_pre_command::fetch_metar_data_after_nav_info);
      }
    }
    else
    {
      (*outStatusMessage) = "Failed to prepare filter statement. Notify developer !!!";
    }
  }
  else
  {
    (*outStatusMessage) = "Could not work on database. Consider rebuilding it using apt.dat optimization.\n" + db_xp_airports.last_err;
  }



  (*outState) = mxFetchState_enum::fetch_ended; // once we change the state, the UI can show/hide the relevant layers/widgets
}

// -------------------------------------


void
data_manager::fetch_last_mission_stats(mxFetchState_enum* outState, std::string* outStatusMessage)
{

  outStatusMessage->clear();
  std::lock_guard<std::mutex> lock(s_thread_sync_mutex);

  const std::string query_stats = "select elev, flap_ratio * 100 as flap_ratio, local_date_days, local_time_sec, case when airspeed < 0.0 then 0.0 else airspeed end as airspeed , groundspeed, faxil_gear, roll, IFNULL(activity,'') from stats order by line_id";

  if (db_stats.db_is_open_and_ready)
  {
    const std::string stmt_uq_name = "fetch_plane_stats";
    // prepare query statement
    if (db_stats.prepareNewStatement(stmt_uq_name, query_stats))
    {
      assert(data_manager::db_stats.mapStatements[stmt_uq_name] != nullptr);

      int seq = 0;
      mission_stats_from_query.reset();

      while (sqlite3_step(db_stats.mapStatements[stmt_uq_name]) == SQLITE_ROW)
      {

        int i = 0;
        mission_stats_from_query.vecSeq_d.emplace_back( static_cast<double> ( seq ) ); // 0 column line_id (seq) we will use internal numbering so we always start from ZERO

        const auto elev_mt = static_cast<double> ( sqlite3_column_int ( db_stats.mapStatements[stmt_uq_name], i ) );
        i++; // column 1 elevation in meters
        const auto elev_ft = elev_mt * meter2feet;
        mission_stats_from_query.vecElev_mt_d.emplace_back(elev_mt); // 1 column Elevation meters
        mission_stats_from_query.vecElev_ft_d.emplace_back(elev_ft); // store elevation as feet
        // Min Max
        mxUtils::eval_min_max<double>(elev_ft, mission_stats_from_query.min_elev_ft, mission_stats_from_query.max_elev_ft);

        if (seq == 0)
          mission_stats_from_query.min_elev_ft = mission_stats_from_query.max_elev_ft = elev_ft;

        mission_stats_from_query.vecFlaps_d.emplace_back( static_cast<double> ( sqlite3_column_int ( db_stats.mapStatements[stmt_uq_name], i ) ) );
        i++; // 2 column flaps
        mission_stats_from_query.vecDayInYear_d.emplace_back( static_cast<double> ( sqlite3_column_int ( db_stats.mapStatements[stmt_uq_name], i ) ) );
        i++; // 3 column day in year
        mission_stats_from_query.vecLocalTime_sec_d.emplace_back(sqlite3_column_double(db_stats.mapStatements[stmt_uq_name], i));
        i++; // 4 column local time in seconds

        const auto air_speed = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name], i);
        i++; // 5 column Air speed
        mission_stats_from_query.vecAirSpeed_d.emplace_back(air_speed);
        // Min Max
        mxUtils::eval_min_max<double>(air_speed, mission_stats_from_query.min_airspeed, mission_stats_from_query.max_airspeed);

        mission_stats_from_query.vecGroundSpeed_d.emplace_back(sqlite3_column_double(db_stats.mapStatements[stmt_uq_name], i));
        i++; // 6 column ground speed
        mission_stats_from_query.vecFaxil_d.emplace_back(sqlite3_column_double(db_stats.mapStatements[stmt_uq_name], i));
        i++; // 7 column Faxil
        mission_stats_from_query.vecRoll_d.emplace_back(sqlite3_column_double(db_stats.mapStatements[stmt_uq_name], i));
        i++; // 8 column roll
        mission_stats_from_query.vecActivity_s.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(db_stats.mapStatements[stmt_uq_name], i)));
        i++; // 9. column Activity: takeoff / landing

        seq++;
      }

      if (seq == 0)
        (*outStatusMessage) = "!!! No Stats Rows Found !!!"; // success message
      else
        (*outStatusMessage) = "Fetched: " + mxUtils::formatNumber<int>(seq) + " of statistics rows."; // success message
    }
    else
    {
      (*outStatusMessage) = "Failed to prepare stats filter statement. Notify developer !!!";
    }

    // query time based on stats
    // Sum the time we flew for each day in a year, in most cases we fly during same day hours but for longer flights it might take during evening of one day and the few hours from the other day.
    const std::string query_time_fly = "select sum ( active_time_in_sec_in_aday ) as total_mission_time_sec from ( select MAX(local_time_sec) - MIN(local_time_sec) as active_time_in_sec_in_aday from stats group by local_date_days) v1";
    if (const std::string stmt_uq_name2 = "query_time_flew"; db_stats.prepareNewStatement(stmt_uq_name2, query_time_fly))
    {
      assert(data_manager::db_stats.mapStatements[stmt_uq_name2] != nullptr);

      double cumulative_time_flew = 0.0;
      while (sqlite3_step(db_stats.mapStatements[stmt_uq_name2]) == SQLITE_ROW)
      {
        const double active_time_in_sec_in_aday_d = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name2], 0);
        cumulative_time_flew += active_time_in_sec_in_aday_d;
      }
      mission_stats_from_query.time_flew_sec_d    = cumulative_time_flew;
      mission_stats_from_query.time_flew_format_s = Utils::format_number_as_hours(cumulative_time_flew);

    } // end if "query_time_fly" is valid

    // Read more statistic queries like: max faxil/max roll/max AoA/maybe max G/
    //       Other stats could be: Center landing, airport ICAO plane landed and on which runway.
    const std::string query_min_max_stats = "SELECT v1.min_pitch, v1.max_pitch, v1.min_roll, v1.max_roll, v1.min_gforce_normal, v1.max_gforce_normal, v1.min_gforce_axil, v1.max_gforce_axil "
                                            ", v1.min_airspeed, v1.max_airspeed, v1.min_groundspeed, v1.max_groundspeed, v1.min_vvi_fpm_pilot, v1.max_vvi_fpm_pilot ,v1.min_vh_ind "
                                            ", v1.max_vh_ind, v1.min_aoa, v1.max_aoa FROM stats_summary v1";

    const std::string stmt_uq_name3 = "query_min_max_stats_vu";

#ifndef RELEASE
    Log::logMsgThread("query min/max stats:" + query_min_max_stats);
#endif // !RELEASE


    if (db_stats.prepareNewStatement(stmt_uq_name3, query_min_max_stats))
    {
      assert(data_manager::db_stats.mapStatements[stmt_uq_name3] != nullptr);

      if (db_stats.step(db_stats.mapStatements[stmt_uq_name3]) == SQLITE_ROW) // fetch 1 row
      {
        int iCol                                        = 0;
        mission_stats_from_query.minPitch = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.maxPitch = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.minRoll = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.maxRoll = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.minGforce = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.maxGforce = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.minFaxilG = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.maxFaxilG = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.minAirspeed = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.maxAirspeed = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.minGroundSpeed = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        mission_stats_from_query.maxGroundspeed = sqlite3_column_double(db_stats.mapStatements[stmt_uq_name3], iCol);
        iCol++;
      }


      // v3.303.9.1 use default scoring if there is none in the mission file
      if (mx_global_settings.xScoring_ptr.isEmpty())
        mx_global_settings.xScoring_ptr = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_ELEMENT_SCORING());

      if (!mx_global_settings.xScoring_ptr.isEmpty())
      {
        // pitch
        const auto vecPitchScoring          = Utils::xml_extract_scoring_subElements ( mx_global_settings.xScoring_ptr, mxconst::get_ELEMENT_PITCH() );
        mission_stats_from_query.pitchScore = Utils::getScoreAfterAnalyzeMinMax ( vecPitchScoring, mission_stats_from_query.minPitch, mission_stats_from_query.maxPitch );
        // roll
        const auto vecRollScoring          = Utils::xml_extract_scoring_subElements ( mx_global_settings.xScoring_ptr, mxconst::get_ELEMENT_ROLL() );
        mission_stats_from_query.rollScore = Utils::getScoreAfterAnalyzeMinMax ( vecRollScoring, mission_stats_from_query.minRoll, mission_stats_from_query.maxRoll );
        // gForce
        const auto vecGForceScoring          = Utils::xml_extract_scoring_subElements ( mx_global_settings.xScoring_ptr, mxconst::get_ELEMENT_GFORCE() );
        mission_stats_from_query.gForceScore = Utils::getScoreAfterAnalyzeMinMax ( vecGForceScoring, mission_stats_from_query.minGforce, mission_stats_from_query.maxGforce );
      }
    } // end if "query_min_max_stats" is valid
  }
  else
  {
    (*outStatusMessage) = "Could not work on the stats database.\n" + db_stats.last_err;
  }


  ///// v3.0.308.4.3
  mission_stats_from_query.vecLandingStatsResults = get_landing_stats_as_vec(); // v3.303.8.3 will get landing information + center line score

  (*outState) = mxFetchState_enum::fetch_ended; // once we change the state, the UI can show/hide the relevant layers/widgets
}


// -------------------------------------


std::vector<mx_stats_data>
data_manager::get_landing_stats_as_vec()
{
  std::vector<mx_stats_data> vecActivity; // store stmt01 data
  // std::string                          icao_id_s;

  // Fetch time passes between activities.First row will always be 0
  // The query will convert days into seconds in order to have correct delta result between "current - previous" fields
  // columns return:  line_id, plane_lat, plane_lon, elev_mt, activity and time_passed (between activities)
  const std::string stmt01 = R"(select line_id, plane_lat, plane_lon, activity
           , case
                  when v1.local_date_days >= v1.prev_date_days then ( v1.local_date_days_in_seconds + v1.local_time_sec ) - ( v1.prev_local_date_days_in_seconds + v1.prev_local_time_sec) -- should pick 99.9% of cases
                  when v1.local_date_days < v1.prev_date_days  then ( (prev_local_date_days_in_seconds + 86400) + v1.local_time_sec ) - (v1.prev_local_date_days_in_seconds + v1.prev_local_time_sec) -- transition a year, very rare
             else 0 end time_passed
from (
select line_id, local_date_days, local_date_days*86400 as local_date_days_in_seconds, local_time_sec, lat as plane_lat, lon as plane_lon, elev as elev_mt, gforce_normal, gforce_axil, vvi_fpm_pilot, activity, lag(local_date_days) over (order by line_id) as prev_date_days, (lag(local_date_days) over (order by line_id) * 86400)  as prev_local_date_days_in_seconds, lag (local_time_sec) over (order by line_id) as prev_local_time_sec
from stats
where (stats.activity != "" and stats.activity is not null)
) v1
)";


#ifndef RELEASE

  Log::logMsg("\nstmt01: " + stmt01); // debug

#endif // !RELEASE

  const std::string unique_name_s01 = "fetch_activity_stats";

  const auto vecCenterLineScores = Utils::xml_extract_scoring_subElements(mx_global_settings.xScoring_ptr, mxconst::get_ELEMENT_CENTER_LINE());


  // Q1: gather activity data from stats db
  if (db_stats.prepareNewStatement(unique_name_s01, stmt01))
  {
    mx_stats_data prev_landing_data;

    // Loop over all activities that are "landing" and gather airport and runway information (drill down)
    while (db_stats.step(db_stats.mapStatements[unique_name_s01]) == SQLITE_ROW) // fetch 1 row))
    {
      mx_stats_data data;
      int           iCol01 = 0;

      data.line_id = sqlite3_column_int(db_stats.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.plane.lat = sqlite3_column_double(db_stats.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.plane.lon = sqlite3_column_double(db_stats.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.activity_s = mxUtils::stringToLower(std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_stats.mapStatements[unique_name_s01], iCol01)))); // takeoff or landing
      iCol01++;
      data.time_passed = sqlite3_column_double(db_stats.mapStatements[unique_name_s01], iCol01);
      iCol01++;

      data.bInitialized = true;

      if (data.time_passed == 0.0) // first activity, probably takeoff, since there is no time passed information between activities
        continue;
      else if (mxconst::get_STATS_LANDING() == data.activity_s) // landing stats ?
      {
        assert(data_manager::db_xp_airports.db && "db_xp_airports was not initialized correctly. Notify developer or try to restart X-Plane.");

        // Q2: fetch the airport (icao_id) the plane landed in
        {
          if (auto navaid = getPlaneAirportOrNearestICAO(false, data.plane.lat, data.plane.lon);
              !navaid.getID().empty())
          {
            data.icao    = navaid.getID();
            data.icao_id = navaid.icao_id;
          }
        }

        // -- Q3 fetch distance from center of rw, if plane landed
        // -- Neads plane position.
        // -- We added special case where there are no boundary, we will only use ICAO.
        // -- This should be ok because even in duplicate ICAO cases, the query won't retrieve runways that the plane is not inside their boundary
        std::string stmt03 = R"(select xp_rw.rw_no_1 || '-' || xp_rw.rw_no_2 as runway
, mx_get_shortest_distance_to_rw_vector ( {}, {}, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon) as distance_from_center
, mx_is_plane_in_rw_area( {}, {}, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width) as is_plane_on_rw
from xp_rw
where is_plane_on_rw = 1
)";

        // v24.03.1 Modified query to use fmt {} instead of bind variables.
        // stmt03 = fmt::format(stmt03.data(), data.plane.lat, data.plane.lon, data.plane.lat, data.plane.lon);
        std::map<int, std::string> mapArgs = { { 1, mxUtils::formatNumber(data.plane.lat, 12) }, { 2, mxUtils::formatNumber(data.plane.lon, 12) }, { 3, mxUtils::formatNumber(data.plane.lat, 12) }, { 4, mxUtils::formatNumber(data.plane.lon, 12) } }; // v24.12.2 added missing lat/lon for second pair of {} // v24.05.2
        stmt03                             = mxUtils::format(stmt03, mapArgs);

        if (data.icao_id <= 0 && (!data.icao.empty()))
        {
          stmt03 += " and xp_rw.icao = '" + data.icao + "'";
        }
        else
        {
          stmt03 += (" and xp_rw.icao_id = " + mxUtils::formatNumber<int>(data.icao_id)); // use icao_id
        }

        #ifndef RELEASE
        Log::logMsg("Fetching landing info in: " + data.icao + ", with icao_id: " + mxUtils::formatNumber<int>(data.icao_id)); // debug
        Log::logMsg("stmt03: " + stmt03);
        #endif // !RELEASE

        // Q3 fetch touchdown relative to center of rw data
        const std::string unique_name_s03 = "fetch_plane_relative_to_center";
        if (db_xp_airports.prepareNewStatement(unique_name_s03, stmt03))
        {
          //// bind query
          // Q3 step
          if (db_xp_airports.step(db_xp_airports.mapStatements[unique_name_s03]) == SQLITE_ROW)
          {
            int iCol03    = 0;
            data.runway_s = std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[unique_name_s03], iCol03))); // runway
            iCol03++;
            data.distance_from_center_of_rw_d = sqlite3_column_double(db_xp_airports.mapStatements[unique_name_s03], iCol03); // distance from center
            iCol03++;

            data.score_centerLine = Utils::getScoreAfterAnalyzeMinMax(vecCenterLineScores, data.distance_from_center_of_rw_d, data.distance_from_center_of_rw_d);

          } // end Q3 step

        } // END Q3

        vecActivity.emplace_back(data);

        prev_landing_data = data; // there is no real use with this parameter
      }                           // end landing activity stats

    } // end Q1: loop over all rows
  }   // End Q1 prepare statement

  return vecActivity; // landing_stats;
}

// -------------------------------------

void
data_manager::reset_runway_search_filter()
{

  // reset the property value so it won't conflict with other searches
  prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_FILTER_AIRPORTS_BY_RUNWAY_TYPE(), "");
}

// -------------------------------------

void
data_manager::deleteCueFromListCueInfoToDisplayInFlightLeg(const std::string& name_s)
{
  // v3.0.303.6 erase 3D instance cue from listCueInfoToDisplayInFlightLeg
  auto itCueEnd = mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg.end();
  for (auto itCue = mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg.begin(); itCue != itCueEnd; ++itCue)
  {
    const std::string cueInstanceName = Utils::readAttrib(itCue->node_ptr, mxconst::get_ATTRIB_INSTANCE_NAME(), itCue->originName);
    if (itCue->cueType == mx_cue_types::cue_obj && name_s == cueInstanceName)
    {
      mapFlightLegs[currentLegName].listCueInfoToDisplayInFlightLeg.erase(itCue);
      break; // exit loop
    }
  }
} // deleteCueFromlistCueInfoToDisplayInFlightLeg


// -------------------------------------
// -------------------------------------


void
data_manager::fetch_METAR(std::unordered_map<int, mx_nav_data_strct>* mapNavaidData, mxFetchState_enum* outState, std::string* outStatusMessage, std::string* outErrorMsg, bool* lockThread)
{
  // We will need to loop over all navaids in the "mapNavaidData" and fetch their METAR.

  constexpr static const char* url_s = "api.flightplandatabase.com";
  (*outState)                        = mxFetchState_enum::fetch_in_process;

  outStatusMessage->clear();


  if (threadStateMetar.flagAbortThread)
  {
    (*outState) = mxFetchState_enum::fetch_ended;
    return;
  }

  threadStateMetar.flagIsActive       = true;
  threadStateMetar.flagThreadDoneWork = false;

  if ((*lockThread)) // when called from other thread, do not try to lock
    std::lock_guard<std::mutex> lock(s_thread_sync_mutex);

  bool flag_http_success = false;
  bool bIsFirstTime      = true;

  const std::string authKey_s = Utils::getNodeText_type_6(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_AUTHORIZATION_KEY(), "");
  std::string       result_s;


  // Loop over all Nav Aids and fetch their metar
  for (auto& nav : *mapNavaidData | std::views::values)
  {
    long              httpStatus = 0;
    std::string       q          = "/weather/" + nav.icao; // we need to add the ICAO
    const std::string full_url_s = mxUtils::trim(fmt::format("https://{}:443{}", url_s, q));
    Log::logMsgThread("url: " + full_url_s); // debug

    // std::string err;
    std::string cert_loc_s = mx_folders_properties.getStringAttributeValue(mxconst::get_PROP_MISSIONX_PATH(), ""); // v24.03.1

    // Prepare cURL
    int iCurlTry      = 0;
    flag_http_success = false;
    while ((iCurlTry < 3) && !(flag_http_success) && !threadStateMetar.flagAbortThread)
    {
      iCurlTry++;

      char errBuff[CURL_ERROR_SIZE]{ '\0' };
      curl_easy_setopt(curl, CURLOPT_URL, full_url_s.c_str());
      // curl_easy_setopt(data_manager::curl, CURLOPT_PORT, 443L);

      // set authorization key if present
      if (!authKey_s.empty())
      {
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curl, CURLOPT_USERPWD, authKey_s.c_str());
      }
      // setup agent
      curl_easy_setopt(curl, CURLOPT_USERAGENT, APP_NAME);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L); // v24.06.1 /Timeout for server connection
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);        // v24.06.1 overall work timeout - 60 seconds
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 0L);        // CURLOPT_NOSIGNAL - skip all signal handling (values 0 or 1)

      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE); // ignore SSL verify
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
      // curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, onProgress); // simple function that manage cancel state

      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errBuff);

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result_s);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

      // https://curl.haxx.se/docs/sslcerts.html
      // curl_easy_setopt(curl, CURLOPT_CAINFO, cacert);

      if (CURLcode res_curl = curl_easy_perform (curl);
          CURLE_OK != res_curl)
      {
        Log::logMsgThread("cURL error code while fetching JSON METAR information: " + Utils::formatNumber<int>(res_curl) + "\n");
      }

      std::string errBuff_s(errBuff);
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
      if (httpStatus != 200 || !errBuff_s.empty())
      {
        outStatusMessage->append(fmt::format("CURL HTTP status: {} Error Buff: {}. {}:{}", std::to_string(httpStatus), errBuff_s, ((iCurlTry < 3) ? "Will try again in 5 sec" : "No Luck"), iCurlTry)); // v24.03.1
        Log::logMsgThread((*outStatusMessage) + "\n");                                                                                                                                                  // debug

        std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // sleep for 5 seconds
        // goto CURL;
      }
      else // parse json
      {
        flag_http_success   = true;
        (*outStatusMessage) = "cURL fetched JSON METAR success.";
      }
    } // end while loop

    /////////////////////////////
    ////// Handle Success //////

    if (flag_http_success)
    {
      int              counter = 0;
      std::set<size_t> setHashIcaoName;

      #ifndef RELEASE
      std::string            last_toICAO_s;
      std::string            last_toName_s;
      std::map<size_t, bool> mapHashIcaoAndName;

      Log::logMsgThread(result_s);
      #endif // !RELEASE

      constexpr static auto KEY_METAR = "METAR";

      try
      {
        nlohmann::json js   = nlohmann::json::parse(result_s, nullptr, true);
        nav.sMetar          = Utils::getJsonValue(js, KEY_METAR, "");
        (*outStatusMessage) = "Finished fetching METAR information for: " + nav.icao;
      }
      catch (nlohmann::json::parse_error& ex)
      {
        Log::logMsgThread(fmt::format("{}.\n== JSON: ==>\n{}\n<== END JSON ==", ex.what(), result_s));
        (*outErrorMsg) = ex.what();
      }
    } // end if http success
  }   // end loop over all Nav Aids

  if (threadStateMetar.flagAbortThread)
    (*outStatusMessage) = "Metar thread aborted.";

  threadStateMetar.flagThreadDoneWork = true;
  threadStateMetar.flagIsActive       = false;
  threadStateMetar.flagAbortThread    = false;
  ( *outState )                       = mxFetchState_enum::fetch_ended;
}

// -------------------------------------

void
data_manager::fetch_fpln_from_simbrief_site (missionx::base_thread::thread_state *inoutThreadState, const std::string &in_pilot_id, missionx::mxFetchState_enum *outState, std::string *outStatusMessage)
{
  outStatusMessage->clear();
  std::lock_guard<std::mutex> lock(s_thread_sync_mutex);

  if (inoutThreadState != nullptr)
  {
    inoutThreadState->flagIsActive       = true;
    inoutThreadState->flagThreadDoneWork = false;
    inoutThreadState->flagAbortThread    = false;

    inoutThreadState->startThreadStopper();
  }

  bool flag_http_success = false;
  bool bIsFirstTime      = true;

  const std::string full_url_s = fmt::format ("https://www.simbrief.com/api/xml.fetcher.php?userid={}", in_pilot_id);

  //// Fetch information
  std::string result_s;
  std::string err;
  std::string cert_loc_s = data_manager::mx_folders_properties.getStringAttributeValue (mxconst::get_PROP_MISSIONX_PATH(), "");
    long httpStatus = 0;
    if (curl)
    {
      char errBuff[CURL_ERROR_SIZE]{ '\0' };
      curl_easy_setopt(curl, CURLOPT_URL, full_url_s.c_str());
      // curl_easy_setopt(data_manager::curl, CURLOPT_PORT, 443L);

      // set authorization key if present
      // if (!authKey_s.empty())
      // {
      //   curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
      //   curl_easy_setopt(curl, CURLOPT_USERPWD, authKey_s.c_str());
      // }
      // setup agent
      curl_easy_setopt(curl, CURLOPT_USERAGENT, APP_NAME);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L); // v24.06.1 /Timeout for server connection
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);        // v24.06.1 overall work timeout - 60 seconds
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 0L);        // CURLOPT_NOSIGNAL - skip all signal handling (values 0 or 1)

      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE); // ignore SSL verify
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
      // curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, onProgress); // simple function that manage cancel state

      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errBuff);

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result_s);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

      // https://curl.haxx.se/docs/sslcerts.html
      // curl_easy_setopt(curl, CURLOPT_CAINFO, cacert);


      if (CURLcode res_curl = curl_easy_perform(curl) // execute the REQUEST
          ; CURLE_OK != res_curl)
      {
        Log::logMsgThread("cURL error code: " + Utils::formatNumber<int>(res_curl) + "\n");
      }
      std::string errBuff_s(errBuff);
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
      if (httpStatus != 200 || !errBuff_s.empty())
      {
        outStatusMessage->append(std::string("cURL HTTP status: ") + std::to_string(httpStatus) + ". Error Buff: " + errBuff_s);

        #ifndef RELEASE
        Log::logMsgThread((*outStatusMessage)); // debug
        #endif
      }
      else // parse json
      {
        flag_http_success   = true;
        (*outStatusMessage) = "cURL fetch success.";
      }

    } // end data_manager::curl - handling cURL

  //// Parse result
  if (flag_http_success)
  {
    IXMLReaderStringSZ iReaderSZ(result_s.c_str());
    IXMLResults parse_result;
    IXMLDomParser idom;
    auto xml_ofp = idom.parseString (result_s.c_str (), "OFP", &parse_result);
    if (parse_result.errorCode == IXMLError_None)
    {
      mx_ext_internet_fpln_strct fpln;
      fpln.internal_id = 0;

      missionx::mx_base_node ofp_node;
      ofp_node.node = xml_ofp.deepCopy ();
      // read the route nodes. So far there are three
      auto node_general     = ofp_node.node.getChildNode (mxconst::get_ELEMENT_GENERAL().c_str ());
      auto node_origin      = ofp_node.node.getChildNode (mxconst::get_ELEMENT_ORIGIN().c_str ());
      auto node_destination = ofp_node.node.getChildNode (mxconst::get_ELEMENT_DESTINATION().c_str ());

      if (!node_general.isEmpty ())
      {
        auto flnum_node           = node_general.getChildNode (mxconst::get_ELEMENT_FLIGHT_NUMBER().c_str ());
        auto route_node           = node_general.getChildNode (mxconst::get_ELEMENT_ROUTE().c_str ());
        auto route_ifps_node      = node_general.getChildNode (mxconst::get_ELEMENT_ROUTE_IFPS().c_str ());
        auto route_navigraph_node = node_general.getChildNode (mxconst::get_ELEMENT_ROUTE_NAVIGRAPH().c_str ());

        fpln.flightNumber_s           = Utils::xml_get_text (flnum_node);
        fpln.simbrief_route           = Utils::xml_get_text (route_node);
        fpln.simbrief_route_ifps      = Utils::xml_get_text (route_ifps_node);
        fpln.simbrief_route_navigraph = Utils::xml_get_text (route_navigraph_node);
      }

      if (!node_origin.isEmpty ())
      {
        auto icao_code_node = node_origin.getChildNode (mxconst::get_ELEMENT_ICAO_CODE().c_str ());
        auto name_node      = node_origin.getChildNode (mxconst::get_ATTRIB_NAME().c_str ());
        auto trans_alt_node      = node_origin.getChildNode (mxconst::get_ELEMENT_TRANS_ALT().c_str ());
        auto plan_rwy_node      = node_origin.getChildNode (mxconst::get_ELEMENT_PLAN_RWY().c_str ());

        fpln.fromICAO_s = Utils::xml_get_text (icao_code_node);
        fpln.fromName_s = Utils::xml_get_text (name_node);

        fpln.simbrief_from_rw = Utils::xml_get_text (plan_rwy_node);
        fpln.simbrief_from_trans_alt = Utils::xml_get_text (trans_alt_node);
      }

      if (!node_destination.isEmpty ())
      {
        auto icao_code_node = node_destination.getChildNode (mxconst::get_ELEMENT_ICAO_CODE().c_str ());
        auto name_node      = node_destination.getChildNode (mxconst::get_ATTRIB_NAME().c_str ());
        auto trans_alt_node = node_destination.getChildNode (mxconst::get_ELEMENT_TRANS_ALT().c_str ());
        auto plan_rwy_node  = node_destination.getChildNode (mxconst::get_ELEMENT_PLAN_RWY().c_str ());

        fpln.toICAO_s = Utils::xml_get_text (icao_code_node);
        fpln.toName_s = Utils::xml_get_text (name_node);

        fpln.simbrief_to_rw = Utils::xml_get_text (plan_rwy_node);
        fpln.simbrief_to_trans_alt = Utils::xml_get_text (trans_alt_node);

      }

      data_manager::tableExternalFPLN_simbrief_vec.emplace_back (fpln);

    }

    #ifndef RELEASE
    Log::logMsgThread("Curl Result: " + result_s);
    #endif

  }

  if (data_manager::tableExternalFPLN_simbrief_vec.empty ())
    (*outStatusMessage) = "Failed to fetch Simbrief Flight Plan.";
  else
    (*outStatusMessage) = mxUtils::formatNumber<size_t> (data_manager::tableExternalFPLN_simbrief_vec.size ()) + " rows fetched"; //

  if (inoutThreadState != nullptr)
  {
    inoutThreadState->flagIsActive       = false;
    inoutThreadState->flagThreadDoneWork = true;
    inoutThreadState->flagAbortThread    = false;
  }

  (*outState) = mxFetchState_enum::fetch_ended; // once we change the state, the UI can show/hide the relevant layers/widgets

} // end function fetch_fpln_from_simbrief_site()


// -------------------------------------


void
data_manager::fetch_fpln_from_external_site(base_thread::thread_state* inoutThreadState, const IXMLNode& inUserPref, mxFetchState_enum* outState, std::string* outStatusMessage)
{
  outStatusMessage->clear();
  std::lock_guard<std::mutex> lock(s_thread_sync_mutex);

  if (inoutThreadState != nullptr)
  {
    inoutThreadState->flagIsActive       = true;
    inoutThreadState->flagThreadDoneWork = false;
    inoutThreadState->flagAbortThread    = false;

    inoutThreadState->startThreadStopper();
  }

  bool flag_http_success = false;
  bool bIsFirstTime      = true;

  const bool bUserAskedToRemoveDuplicateICAO     = Utils::readBoolAttrib(inUserPref,   mxconst::get_PROP_REMOVE_DUPLICATE_ICAO_ROWS(), false);
  const bool bUserAskedToGroupByICAOAndWaypoints = Utils::readBoolAttrib(inUserPref,   mxconst::get_PROP_GROUP_DUPLICATES_BY_WAYPOINTS(), false);

  const std::string authKey_s = Utils::getNodeText_type_6(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_AUTHORIZATION_KEY(), "");
  std::string       result_s;

  std::string q     = "/search/plans?";
  std::string val_s = Utils::readAttrib(inUserPref,   mxconst::get_PROP_FROM_ICAO(), "");

  indexPointer_forExternalFPLN_tableVector.clear();
  tableExternalFPLN_vec.clear();

  const auto lmbda_build_q = [](bool& isFirstTime, std::string q, std::string attrib, std::string val = "")
  {
    if (!val.empty())
    {
      q += (isFirstTime) ? attrib + "=" + val : "&" + attrib + "=" + val;
      isFirstTime = false;
    }

    return q;
  };

  /// Prepare the query string
  q = lmbda_build_q(bIsFirstTime, q,   mxconst::get_PROP_FROM_ICAO(), val_s);

  // v3.0.253.3 added toICAO as a search criteria
  val_s = Utils::readAttrib(inUserPref,   mxconst::get_PROP_TO_ICAO(), "");
  q     = lmbda_build_q(bIsFirstTime, q,   mxconst::get_PROP_TO_ICAO(), val_s);

  val_s = Utils::readAttrib(inUserPref,   mxconst::get_PROP_MAX_DISTANCE_SLIDER(), "");
  q     = lmbda_build_q(bIsFirstTime, q, "distanceMax", val_s);


  val_s = Utils::readAttrib(inUserPref,   mxconst::get_PROP_LIMIT(), "");
  q     = lmbda_build_q(bIsFirstTime, q,   mxconst::get_PROP_LIMIT(), val_s);


  val_s = Utils::readAttrib(inUserPref,   mxconst::get_PROP_SORT_FPLN_BY(), "");
  q     = lmbda_build_q(bIsFirstTime, q,   mxconst::get_PROP_SORT_FPLN_BY(), val_s);

  const std::string url_s      = "api.flightplandatabase.com";
  const std::string full_url_s = mxUtils::trim("https://" + url_s + ":443" + q);
  Log::logMsgThread("url: " + full_url_s); // debug



  //// Fetch information
  std::string err;
  std::string cert_loc_s = mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", err);

  // if (!flag_http_success)
  {

    long httpStatus = 0;
    if (curl)
    {
      char errBuff[CURL_ERROR_SIZE]{ '\0' };
      curl_easy_setopt(curl, CURLOPT_URL, full_url_s.c_str());
      // curl_easy_setopt(data_manager::curl, CURLOPT_PORT, 443L);

      // set authorization key if present
      if (!authKey_s.empty())
      {
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curl, CURLOPT_USERPWD, authKey_s.c_str());
      }
      // setup agent
      curl_easy_setopt(curl, CURLOPT_USERAGENT, APP_NAME);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L); // v24.06.1 /Timeout for server connection
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);        // v24.06.1 overall work timeout - 60 seconds
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 0L);        // CURLOPT_NOSIGNAL - skip all signal handling (values 0 or 1)

      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE); // ignore SSL verify
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
      // curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, onProgress); // simple function that manage cancel state

      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errBuff);

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result_s);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

      // https://curl.haxx.se/docs/sslcerts.html
      // curl_easy_setopt(curl, CURLOPT_CAINFO, cacert);
      CURLcode res_curl = curl_easy_perform(curl); // execute the REQUEST

      if (CURLE_OK != res_curl)
      {
        Log::logMsgThread("cURL error code: " + Utils::formatNumber<int>(res_curl) + "\n");
      }
      std::string errBuff_s(errBuff);
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
      if (httpStatus != 200 || !errBuff_s.empty())
      {
        outStatusMessage->append(std::string("cURL HTTP status: ") + std::to_string(httpStatus) + ". Error Buff: " + errBuff_s);

        #ifndef RELEASE
        Log::logMsgThread((*outStatusMessage)); // debug
        #endif
      }
      else // parse json
      {
        flag_http_success   = true;
        (*outStatusMessage) = "cURL fetch success.";
      }

    } // end data_manager::curl - handling cURL
      // #endif // USE_CURL

  } // end httplib fallback

  //// Parse result
  if (flag_http_success)
  {
    int              counter = 0;
    std::set<size_t> setHashIcaoName;

    #ifndef RELEASE
    std::string            last_toICAO_s;
    std::string            last_toName_s;
    std::map<size_t, bool> mapHashIcaoAndName;
    #endif // !RELEASE

    nlohmann::json jSON = nlohmann::json::parse(result_s);

    for (auto& js : jSON)
    {
      #ifndef RELEASE
      Log::logMsgThread(fmt::format("JSON size: {}\nJSON Line:\n{}\n", js.size(), js.dump()));
      #endif
      mx_ext_internet_fpln_strct fpln;

      fpln.distnace_d        = Utils::getJsonValue<double>(js, mx_fplndb_json_keys.Z_KEY_distance, nlohmann::detail::value_t::number_float, 0.0);
      fpln.encode_polyline_s = Utils::getJsonValue(js, mx_fplndb_json_keys.Z_KEY_encodedPolyline, "");
      fpln.flightNumber_s    = Utils::getJsonValue(js, mx_fplndb_json_keys.Z_KEY_flightNumber, "");
      fpln.fromICAO_s        = Utils::getJsonValue(js, mx_fplndb_json_keys.Z_KEY_fromICAO, "");
      fpln.fromName_s        = Utils::getJsonValue(js, mx_fplndb_json_keys.Z_KEY_fromName, "");
      fpln.maxAltitude_i     = Utils::getJsonValue<int>(js, mx_fplndb_json_keys.Z_KEY_maxAltitude, nlohmann::detail::value_t::number_integer, 0);
      fpln.notes_s           = Utils::getJsonValue(js, mx_fplndb_json_keys.Z_KEY_notes, "");
      fpln.toICAO_s          = Utils::getJsonValue(js, mx_fplndb_json_keys.Z_KEY_toICAO, "");
      fpln.toName_s          = Utils::getJsonValue(js, mx_fplndb_json_keys.Z_KEY_toName, "");
      fpln.waypoints_i       = Utils::getJsonValue<int>(js, mx_fplndb_json_keys.Z_KEY_waypoints, nlohmann::detail::value_t::number_integer, 0);
      fpln.fpln_unique_id    = Utils::getJsonValue<int>(js, mx_fplndb_json_keys.Z_KEY_id, nlohmann::detail::value_t::number_integer, 0);
      fpln.popularity_i      = Utils::getJsonValue<int>(js, mx_fplndb_json_keys.Z_KEY_popularity, nlohmann::detail::value_t::number_integer, 0);


      //// Post Parsing
      // Decode polylines
      if (!fpln.encode_polyline_s.empty())
      {
        SimplePolyline             smp;
        [[maybe_unused]] const int size = smp.decode(fpln);
      }

      size_t hashIcaoAndAirportName;
      if (bUserAskedToRemoveDuplicateICAO && bUserAskedToGroupByICAOAndWaypoints) // include waypoints in filter hash
        hashIcaoAndAirportName = std::hash<std::string>{}(mxUtils::stringToLower(fpln.toICAO_s + fpln.toName_s + mxUtils::formatNumber<int>(fpln.waypoints_i)));
      else // exclude waypoints in filter hash
        hashIcaoAndAirportName = std::hash<std::string>{}(mxUtils::stringToLower(fpln.toICAO_s + fpln.toName_s));

      if (bUserAskedToRemoveDuplicateICAO && (setHashIcaoName.find(hashIcaoAndAirportName) != setHashIcaoName.cend()))
        continue; // skip current fpln

      setHashIcaoName.insert(hashIcaoAndAirportName); // insert hash

      #ifndef RELEASE
      const std::string waypoints_s = mxUtils::formatNumber<int>(fpln.waypoints_i);

      last_toICAO_s = fpln.toICAO_s;
      last_toName_s = fpln.toName_s;
      Utils::addElementToMap(mapHashIcaoAndName, hashIcaoAndAirportName, true);
      #endif // !RELEASE


      fpln.internal_id = counter; // we will use it if we need to show extra details of specific flight plan

      tableExternalFPLN_vec.emplace_back(fpln);
      Utils::addElementToMap(indexPointer_forExternalFPLN_tableVector, counter, &tableExternalFPLN_vec.back()); // adding pointer data to std::map index like container


      counter++;
    } // end loop over all <a> nodes or JSON lines

    if (flag_http_success)
      curl_result_s = result_s; // store json output as a string
    else
      curl_result_s.clear();

    #ifndef RELEASE
    Log::logMsgThread("Curl Result: " + curl_result_s);
    #endif

    // #endif //original #endif, moved after Json to XML parsing.

    if (counter == 0)
      (*outStatusMessage) = "No flight plans were found for this location.";
    else
      (*outStatusMessage) = mxUtils::formatNumber<int>(counter) + " rows fetched"; // v3.303.8.3

  } // if flag_http_success

  if (inoutThreadState != nullptr)
  {
    inoutThreadState->flagIsActive       = false;
    inoutThreadState->flagThreadDoneWork = true;
    inoutThreadState->flagAbortThread    = false;
  }

  (*outState) = mxFetchState_enum::fetch_ended; // once we change the state, the UI can show/hide the relevant layers/widgets
}


// -------------------------------------
// -------------------------------------
// -------------------------------------

std::string
data_manager::fetch_overpass_info(const std::string& in_url_s, std::string& outError)
{
  // bool flag_http_success = false;
  std::string result_s;
  outError.clear();

  const std::string url_site = (in_url_s.find('?') != std::string::npos) ? in_url_s.substr(0, in_url_s.find('?')) : "";
  const std::string q        = (in_url_s.find('?') != std::string::npos) ? in_url_s.substr(in_url_s.find('?') + 1) : "";

  //// Fetch information
  std::string err;
  std::string cert_loc_s = mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", err);

  {
    // #ifdef USE_CURL

    std::lock_guard<std::mutex> lock(s_thread_sync_mutex);
    // curl_global_init(CURL_GLOBAL_DEFAULT); // DO NOT USE
    long httpStatus = 0;
    if (curl)
    {
      overpass_counter_i++; // counter
      Log::logMsgThread("[overpass] Overpass Calls: " + mxUtils::formatNumber<int>(overpass_counter_i));

      char errBuff[CURL_ERROR_SIZE] = "\0"; // v3.305.3

      curl_easy_setopt(curl, CURLOPT_URL, in_url_s.c_str());
      // curl_easy_setopt(data_manager::curl, CURLOPT_PORT, 443L);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, APP_NAME);
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE); // ignore SSL verify
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
      // curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, onProgress); // simple function that manage cancel state

      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errBuff);

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write); // <==== THIS IS WHERE WE HANDLE THE RESPOND DATA
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result_s);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      // https://curl.haxx.se/docs/sslcerts.html
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);

      // https://curl.haxx.se/docs/sslcerts.html
      // curl_easy_setopt(curl, CURLOPT_CAINFO, cacert);
      CURLcode res_curl = curl_easy_perform(curl); // execute the REQUEST

      if (CURLE_OK != res_curl)
      {
        Log::logMsgThread("CURL error code: " + Utils::formatNumber<int>(res_curl) + "\n");
      }
      std::string errBuff_s = errBuff; // v3.305.3
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);

      if (httpStatus != 200 || !errBuff_s.empty())
      {
        outError.append(std::string("CURL HTTP status: ") + std::to_string(httpStatus) + ((errBuff_s.empty()) ? "" : std::string(". Error Buff: ") + errBuff_s)); // v3.0.255.4 added logic to display "Error Buff" only if there is a string value in it

        switch (httpStatus)
        {
          case 504: // "504 Gateway Time-out"
            outError.append(": Overpass Gateway Time-out.");
            break;
          case 429: // "429 Too many requests"
            outError.append(": Too many requests.");
            break;
          default:
            break;
        }


#ifndef RELEASE
        Log::logMsgThread(outError); // debug
#endif
      }
      else
      {
        outError.clear();
      }

    } // end data_manager::curl - handling cURL
#ifndef RELEASE
    // Log::logMsgThread("Curl Result: \n" + result_s); // debug
#endif

    // #endif
  } // end CURL handling if HTTPLIB failed

  overpass_fetch_err = outError; // v3.0.255.4 overpass_fetch_err will get the error. This will clear the last error message overpass_fetch_err had.

  return result_s;
}

// -------------------------------------

NavAidInfo
data_manager::getPlaneAirportOrNearestICAO(const bool& inOnlySearchInDatabase, const double& inLat, const double& inLon, bool inIsThread)
{
  // The following function will first try to figure out if the plane inside one of the airports boundary which is stored in the SQLITE database.
  // If not then it will fall back to the original code, by using the XPSDK.
  bool flagFoundPlaneInAirportArea{ false }; // v3.303.8.3


  NavAidInfo navAid;
  navAid.navRef       = XPLM_NAV_NOT_FOUND;
  Point planePosition = dataref_manager::getPlanePointLocationThreadSafe();
  // v3.303.8.3 add the option to receive plane location relative to pre-defined location and not to current plane position.
  if (inLat * inLon != 0.0)
  {
    planePosition.setLat(inLat);
    planePosition.setLon(inLon);
    if (inIsThread)
      planePosition.setElevationFt(0.0);
    else
      planePosition.get_terrain_elev_mt_from_probe();
  }


  ImVec2 posVec2( static_cast<float> ( planePosition.lat ), static_cast<float> ( planePosition.lon ) );

  // v3.303.8.3 Adding sqlite query
  // Add code to search airport plane is in its boundary from sqlite db

  std::string query = R"(
select icao, ap_name, ap_lat, ap_lon, ap_elev
      , mx_bearing( {1}, {2}, ap_lat, ap_lon ) as heading
      , mx_is_plane_in_airport_boundary( {1}, {2}, boundary ) as is_plane_in_boundary
      , icao_id, boundary
from
(
  select xp_airports.icao_id, xp_airports.icao, xp_airports.ap_name, xp_airports.ap_lat, xp_airports.ap_lon, xp_airports.ap_elev, xp_airports.boundary
  from xp_airports
  where xp_airports.boundary is not null
  and ( trunc(xp_airports.ap_lat) between trunc( {1} - 1.0) and  trunc( {1} + 1.0) )
  and ( trunc(xp_airports.ap_lon) between trunc( {2} - 1.0) and  trunc( {2} + 1.0) )
) v1
where is_plane_in_boundary = 1
)";


  std::map<int, std::string> mapArgs = { { 1, planePosition.getLat_s() }, { 2, planePosition.getLon_s() } }; // v24.05.2
  query                              = mxUtils::format(query, mapArgs);



#ifndef RELEASE
  Log::logMsgThread("Fetch_airport_where_plane_is_in_its_boundary Query:\n" + query);
#endif // !RELEASE


  if (db_xp_airports.db_is_open_and_ready)
  {
    // prepare query statement
    if (const std::string stmt_uq_name = "fetch_airport_where_plane_is_in_its_boundary";
        db_xp_airports.prepareNewStatement(stmt_uq_name, query))
    {
      assert(data_manager::db_xp_airports.mapStatements[stmt_uq_name] != nullptr);

      // fetch only first row if exists
      while (sqlite3_step(db_xp_airports.mapStatements[stmt_uq_name]) == SQLITE_ROW) // if SQLITE_ROW or SQLITE_DONE
      {
        int iCol = 0;

        const unsigned char* icao_ucc = sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], iCol);
        navAid.setID(std::string(reinterpret_cast<const char*>(icao_ucc)));

        ++iCol;
        const unsigned char* ap_name = sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], iCol);
        navAid.setName(std::string(reinterpret_cast<const char*>(ap_name)));

        ++iCol;
        navAid.lat = static_cast<float> ( sqlite3_column_double ( db_xp_airports.mapStatements[stmt_uq_name], iCol ) );
        ++iCol;
        navAid.lon = static_cast<float> ( sqlite3_column_double ( db_xp_airports.mapStatements[stmt_uq_name], iCol ) );
        ++iCol;
        navAid.height_mt = static_cast<float> ( sqlite3_column_double ( db_xp_airports.mapStatements[stmt_uq_name], iCol ) );
        ++iCol;
        navAid.heading = static_cast<float> ( sqlite3_column_double ( db_xp_airports.mapStatements[stmt_uq_name], iCol ) );
        ++iCol;
        flagFoundPlaneInAirportArea = sqlite3_column_int(db_xp_airports.mapStatements[stmt_uq_name], iCol);
        // v3.303.8.3 add icao_id to query result
        ++iCol;
        navAid.icao_id = sqlite3_column_int(db_xp_airports.mapStatements[stmt_uq_name], iCol); // icao_id
        ++iCol;                                                                                              // we do not read boundary field

        navAid.flag_navDataFetchedFromDB = true; // v24.03.1
      }                                          // end if sql fetched at least one row (we will only pick the first row


    } // end if prepare statement succeeded.

  } // end airport search using sqlite boundary query

  if (inOnlySearchInDatabase + inIsThread)
  {
    if (flagFoundPlaneInAirportArea)
      navAid.navRef = XPLMFindNavAid(nullptr, navAid.ID, &navAid.lat, &navAid.lon, nullptr, xplm_Nav_Airport); // we are still using XPlane library to re-fetch information but this time we set the ICAO and lat/lon beforehand

    return navAid;
  }
  else
  {
    if (flagFoundPlaneInAirportArea)
      navAid.navRef = XPLMFindNavAid(nullptr, navAid.ID, &navAid.lat, &navAid.lon, nullptr, xplm_Nav_Airport); // we are still using XPlane library to re-fetch information but this time we set the ICAO and lat/lon beforehand
    else
    {
      navAid.flag_navDataFetchedFromDB                = false; // v24.03.1
      navAid.flag_navDataFetchedFromXPLMGetNavAidInfo = true;  // v24.03.1
      navAid.navRef                                   = XPLMFindNavAid(nullptr, nullptr, &posVec2.x, &posVec2.y, nullptr, xplm_Nav_Airport);
    }

    if (navAid.navRef != XPLM_NAV_NOT_FOUND) // fetch more information
      XPLMGetNavAidInfo(navAid.navRef, &navAid.navType, &navAid.lat, &navAid.lon, &navAid.height_mt, &navAid.freq, &navAid.heading, navAid.ID, navAid.name, nullptr);
    else
      navAid.init();
  }

  return navAid;
}

// -------------------------------------

NavAidInfo
data_manager::getICAO_info (const std::string &inICAO)
{
  NavAidInfo  nNavAid;
  const Point planePosition = dataref_manager::getPlanePointLocationThreadSafe ();
  ImVec2      posVec2 (static_cast<float> (planePosition.lat), static_cast<float> (planePosition.lon));

  nNavAid.navRef = XPLMFindNavAid (nullptr, inICAO.c_str (), &posVec2.x, &posVec2.y, nullptr, xplm_Nav_Airport); // find airport with the id: inICAO closest to plane
  if (nNavAid.navRef == XPLM_NAV_NOT_FOUND)
    nNavAid.init ();
  else
    XPLMGetNavAidInfo (nNavAid.navRef, &nNavAid.navType, &nNavAid.lat, &nNavAid.lon, &nNavAid.height_mt, &nNavAid.freq, &nNavAid.heading, nNavAid.ID, nNavAid.name, NULL);

  return nNavAid;
}

// -----------------------------------


missionx::NavAidInfo
data_manager::get_and_guess_nav_info (const std::string &in_id_nav_name, const missionx::Point &prevPoint)
{
  NavAidInfo  nNavAid;
  auto lat = static_cast<float>(prevPoint.lat);
  auto lon = static_cast<float>(prevPoint.lon);
  nNavAid.navRef = XPLMFindNavAid (nullptr, in_id_nav_name.c_str (), &lat, &lon, nullptr, xplm_Nav_Airport|xplm_Nav_NDB|xplm_Nav_VOR| xplm_Nav_Fix|xplm_Nav_DME); // find navaid

  if (nNavAid.navRef == XPLM_NAV_NOT_FOUND)
    nNavAid.init ();
  else
    XPLMGetNavAidInfo (nNavAid.navRef, &nNavAid.navType, &nNavAid.lat, &nNavAid.lon, &nNavAid.height_mt, &nNavAid.freq, &nNavAid.heading, nNavAid.ID, nNavAid.name, NULL);

  return nNavAid;
}

// -----------------------------------

mx_wp_guess_result
data_manager::get_nearest_guessed_navaid_based_on_coordinate(mxVec2f& inPos) noexcept
{
  const static std::vector<XPLMNavType> vecNavTypes{ xplm_Nav_Airport, xplm_Nav_NDB, xplm_Nav_VOR, xplm_Nav_Fix, xplm_Nav_DME };
  NavAidInfo                            navAid;
  mx_wp_guess_result          result;

  for (auto& navType : vecNavTypes)
  {
    navAid.navRef = XPLMFindNavAid(nullptr, nullptr, &inPos.lat, &inPos.lon, nullptr, navType);
    if (navAid.navRef == XPLM_NAV_NOT_FOUND)
      continue;

    XPLMGetNavAidInfo(navAid.navRef, &navAid.navType, &navAid.lat, &navAid.lon, &navAid.height_mt, &navAid.freq, &navAid.heading, navAid.ID, navAid.name, nullptr);
    navAid.synchToPoint();
    const auto distance_d = Point((double)inPos.lat, (double)inPos.lon).calcDistanceBetween2Points(navAid.p); // find distance between waypoint and the closest navaid location
    if (result.distance_d > distance_d)
    {
      result.nav_ref    = (int)navAid.navRef;
      result.nav_type   = (int)navAid.navType;
      result.distance_d = distance_d;
      result.name       = (navAid.getID());
    }

  } // end loop over NavTypes

  return result;
}


// -----------------------------------


void
data_manager::validate_display_object_file_existence(const std::string& inMissionFile, const IXMLNode& parentOfLegNodes_ptr, const IXMLNode &inGlobalSettingsNode, const IXMLNode &inObjectTemplatesNode, std::string& outErr)
{
  // The function must receive the parent node of the <display_object> elements like: "<flight_plan>"
  // loop over all display objects
  // Store 3D "file_name" strings.
  // Ignore any <display_object> without the "file_name" attribute
  // Concatenate the "file_name" to the "inFilePath" string and try to figure if this is a file
  assert((!inGlobalSettingsNode.isEmpty()) && "[validate_display_object_file_existence]<global_settings> node is empty !!!");
  assert((!parentOfLegNodes_ptr.isEmpty()) && "[validate_display_object_file_existence]parent flight leg node is empty !!!");

  std::lock_guard<std::mutex> lock(s_thread_sync_mutex); // make sure function is thread safe (I think)


  int                                err_counter_i = 0;
  std::map<std::string, std::string> mapFilesAndErrors; // map holds the file_name as key and if there is any issue with the existence of the file it will hold the error
  outErr.clear();

  fs::path          filePath             = inMissionFile;
  const std::string missionFilePath      = filePath.remove_filename().string();
  const std::string missionPackageFolder = missionFilePath + "../"; // example from: "{xp11}/Custom Scenery/missionx/random/briefer" to "{xp11}/Custom Scenery/missionx/random". This is the default file locations for sound and 3D files if global settings does not hold anything.

  int legs_i = parentOfLegNodes_ptr.nChildNode(mxconst::get_ELEMENT_LEG().c_str());
  for (int i2 = 0; i2 < legs_i; ++i2)
  {

    auto leg = parentOfLegNodes_ptr.getChildNode(mxconst::get_ELEMENT_LEG().c_str(), i2);
    if (!leg.isEmpty())
    {
      // loop over all display objects
      int display_object_i = leg.nChildNode();
      for (int i3 = 0; i3 < display_object_i; ++i3)
      {
        auto do_node = leg.getChildNode(i3);
        if (!do_node.isEmpty())
        {
          std::string tag_s = do_node.getName();
          if (tag_s != mxconst::get_ELEMENT_DISPLAY_OBJECT() && tag_s != mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE() )
            continue; // skip elements that are not <display_object> or <display_object_near_plane>

          std::string file_name       = Utils::readAttrib(do_node, mxconst::get_ATTRIB_FILE_NAME(), "");
          {
            if ( const auto name_of_3d_template_file = Utils::readAttrib ( do_node,  mxconst::get_ATTRIB_NAME(), "" )
              ; name_of_3d_template_file.empty())
            {
              continue;
            }
            else
            {
              // search for the file name in <object_templates>
              auto node_clone = inObjectTemplatesNode.deepCopy();
              auto obj3d_node = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(node_clone, mxconst::get_ELEMENT_OBJ3D(),  mxconst::get_ATTRIB_NAME(), name_of_3d_template_file, false);
              file_name       = Utils::readAttrib(obj3d_node, mxconst::get_ATTRIB_FILE_NAME(), "");
            }
          } // end extract file_name information from <display_object> or <object_template>


          if (file_name.empty())
            continue;
          else if (!Utils::isElementExists(mapFilesAndErrors, file_name)) // if not already exists
          {
            bool is_virtual_file = false;
            // check if file exists
            std::string err;


            // Lambda function to search the file //
            const auto lmbda_get_3d_file_location = [file_name = file_name, missionPackageFolder = missionPackageFolder, inGlobalSettingsNode = inGlobalSettingsNode, missionFilePath = missionFilePath](bool& outIsVirtual)
            {
              // 1. search virtual
              // 2. Search as physical file
              XPLMObjectRef g_object_ref;

              int i = XPLMLookupObjects(file_name.c_str(), 0, 0, Utils::load_cb_dummy, &g_object_ref); // The dummy function is important so we won't crash
              if (i)
              {
                outIsVirtual = true;
                return file_name + " (" + mxUtils::formatNumber<int>(i) + ")";
              }
              else
              { // search as physical

                fs::path display_file = file_name;
                #ifndef RELEASE
                const std::string filename     = display_file.filename().string();      // debug
                const std::string relativePath = display_file.relative_path().string(); // debug
                #endif

                if (display_file.filename() == display_file.relative_path()) // this is probably locally defined 3D object and not an external library 3D object
                {
                  // get 3D Object location from global_settings
                  // Concatenate the file_name with the mission root folder (missionPackageFolder).
                  bool              flag_found               = false;
                  IXMLNode          globalSettings_cloneNode = inGlobalSettingsNode.deepCopy();
                  const std::string obj3d_custom_folder_name = Utils::xml_get_attribute_value_drill(globalSettings_cloneNode, mxconst::get_ATTRIB_OBJ3D_FOLDER_NAME(), flag_found, mxconst::get_ELEMENT_FOLDERS());

                  #ifndef RELEASE
                  // debug purposes
                  const std::string debug_filePath = (obj3d_custom_folder_name.empty()) ? missionPackageFolder + file_name : missionPackageFolder + obj3d_custom_folder_name + "/" + file_name;
                  #endif // !RELEASE

                  return (obj3d_custom_folder_name.empty()) ? missionPackageFolder + file_name : missionPackageFolder + obj3d_custom_folder_name + "/" + file_name;
                }
                else
                { // v3.303.14 check if file string starts with ".." which means relative to Mission Pack location
                  if (file_name.find("..") == 0 || ((file_name.find('/') == std::string::npos) && (file_name.find('\\') == std::string::npos))) // if we provide only 3D Object name and there is no path in it
                    return missionFilePath + file_name; // construct file location relative to mission briefer folder location, example: "{xp11}/Custom Scenery/missionx/random/briefer" + file_name
                  else
                    return "Custom Scenery/" + file_name; // v3.303.14 Add "Head of Custom Scenery" folder to the file name, which means "relative to X-Plane root folder".
                }
              }
              return EMPTY_STRING;
            };



            std::string objectFile3D = lmbda_get_3d_file_location(is_virtual_file); // The function returns file location and if it is from a virtual object. It tests for Virtual and then constructs the Physical Path as string.

            if (is_virtual_file)
            {

              if (objectFile3D.empty()) // means we did not found virtual file
              {
                ++err_counter_i;
                err = fmt::format("[Validate 3D file] Not valid File: \"{}\", is not a valid virtual file. Copy the correct value from the library: EXPORT VIRTUAL PHYSICAL. See: https://developer.x-plane.com/article/library-library-txt-file-format-specification/", file_name);
                Log::logMsgThread(err);
                // Log::logMsgThread("[dm Validate 3D Objects]: " + err);
              }
            }
            else // if (!is_virtual_file)
            {
              if ( fs::path file = objectFile3D
                  ; !exists(file) || !fs::directory_entry(file).is_regular_file())
              {
                ++err_counter_i;
                if (!exists(file))
                  err = "Not Found File: " + file.string();
                else if (!fs::directory_entry(file).is_regular_file())
                  err = "Not a File: " + file.string() + " is a folder and not a file";
                else
                  err = "Not valid File: " + file.string() + " is of wrong type";

                Log::logMsgThread("[Validate 3D file] " + err);
              }
              // else
              //   result = "Found: " + file.string();
            }

            if (err.empty())
              Log::logMsgThread("[Validate 3D file] Found " + ((is_virtual_file) ? "Virtual File: " + file_name : "File: " + objectFile3D));

            Utils::addElementToMap(mapFilesAndErrors, file_name, err);


          } // end if file is not in map[] element (no duplications)

        } // end <display_object> is valid

      } // end loop over all <display_object> elements

    } // end <leg> is valid
  }   // end loop over all <leg>


  if (err_counter_i > 0)
  {
    set_found_missing_3D_object_files(true); // v3.0.255.3
    outErr = "There might be missing 3D Objects. Check missionx.log file for more information.";
  }

} // validate_display_object_file_existence


// -----------------------------------


void
data_manager::sqlite_test_db_validity(dbase& inDB, const bool isThreaded)
{
  char *zErrMsg = nullptr;

  constexpr std::string_view sql = "select (select feature_version from dual) as feature_version, (select count(icao_id) from xp_airports) as count_xp_airports, (select count(icao_id) from xp_ap_ramps) as count_xp_ramps, (select count(lat) from xp_loc) as count_xp_loc from dual";


  reasultTable.clear();
  int rc = sqlite3_exec(inDB.db, sql.data(), &callback_sqlite_data, nullptr, &zErrMsg);
  if (rc != SQLITE_OK)
  {
    set_flag_rebuild_apt_dat(true);

    if (zErrMsg == nullptr)
      Log::logMsgThread("[sqlite_test_db_validity] SQL error: " + inDB.last_err);
    else
      Log::logMsgThread("[sqlite_test_db_validity] SQL error: " + std::string(zErrMsg));

    sqlite3_free(zErrMsg);
  }
  else
  {
    Log::logMsgThread("[sqlite_test_db_validity] Information was gathered.");
    for (auto &val : reasultTable | std::views::values)
    {
      const auto feature_version = mxUtils::stringToNumber<int>(val["feature_version"]);

      // we only need the version information to decide if to rebuild APT.DAT database
      if (MX_FEATURES_VERSION != feature_version)
        set_flag_rebuild_apt_dat(true);

      Log::logMsgThread("Feature version: " + val["feature_version"] + ", count_xp_airports: " + val["count_xp_airports"] + ", count_xp_ramps: " + val["count_xp_ramps"] + ", count_xp_loc: " + val["count_xp_loc"]);
    }
  }

  if (get_flag_rebuild_apt_dat())
  {
    queFlcActions.push(mx_flc_pre_command::exec_apt_dat_optimization); // v3.0.255.3
    set_flag_rebuild_apt_dat(false);                                             // reset
  }
}

// -----------------------------------

std::map<int, mx_local_fpln_strct>
data_manager::read_and_parse_littleNavMap_fpln(const std::string& inPathAndFile)
{
  ////// LAMBDAS //////////
  const auto read_pos = [] ( const IXMLNode & inParent)
  {
    Point p;
    if (inParent.isEmpty() == false && inParent.nChildNode(mxconst::get_ELEMENT_LNM_Pos().c_str()) > 0)
    {
      const IXMLNode c = inParent.getChildNode(mxconst::get_ELEMENT_LNM_Pos().c_str());
      p.setLat(Utils::readNumericAttrib(c, mxconst::get_ATTRIB_LNM_Lat(), 0.0));
      p.setLon(Utils::readNumericAttrib(c, mxconst::get_ATTRIB_LNM_Lon(), 0.0));
      p.setElevationFt(Utils::readNumericAttrib(c, mxconst::get_ATTRIB_LNM_Alt(), 0.0));
    }
    return p;
  };


  const auto read_node_text = [] ( const IXMLNode & inParent, const std::string& subElementName, std::string inDefault = "")
  {
    if (inParent.isEmpty() == false && inParent.nChildNode(subElementName.c_str()) > 0)
    {
      const IXMLNode c = inParent.getChildNode(subElementName.c_str());
      return std::string(c.getText());
    }

    return inDefault;
  };

  std::string                                  errMsg;
  std::map<int, mx_local_fpln_strct> fpln;
  ////// END LAMBDAS //////////

  // Read Little Nav Map flight plan
  Log::logMsg("[Load] Reading Flight Plan: " + inPathAndFile); // debug
  IXMLDomParser iDom;
  ITCXMLNode    xMainNode = iDom.openFileHelper(inPathAndFile.c_str(), mxconst::get_ELEMENT_LNM_LittleNavmap().c_str(), &errMsg);

  if (errMsg.empty()) // we find main node
  {
    IXMLNode xFlightplan = xMainNode.getChildNode(mxconst::get_ELEMENT_LNM_Flightplan().c_str()).deepCopy();

    if (xFlightplan.isEmpty())
    {
      return fpln;
    }
    else
    {
      // read departure
      if (xFlightplan.nChildNode(mxconst::get_ELEMENT_LNM_Departure().c_str()) > 0)
      {
        const auto          node = xFlightplan.getChildNode ( mxconst::get_ELEMENT_LNM_Departure().c_str () );
        mx_local_fpln_strct leg;


        leg.indx  = 0; // departure is the briefer start location
        leg.p     = read_pos(node);
        leg.name  = "briefer";
        leg.ident = leg.name;

        Utils::addElementToMap(fpln, leg.indx, leg); // briefer
      }

      // read waypoints
      auto xWaypoints = xFlightplan.getChildNode(mxconst::get_ELEMENT_LNM_Waypoints().c_str());
      if (xWaypoints.isEmpty() == false && xWaypoints.nChildNode(mxconst::get_ELEMENT_LNM_Waypoint().c_str()) > 0)
      {
        int        legCounter = 0;
        const auto nWays = xWaypoints.nChildNode(mxconst::get_ELEMENT_LNM_Waypoint().c_str());
        for (int i1 = 0; i1 < nWays; ++i1)
        {
          auto node = xWaypoints.getChildNode(mxconst::get_ELEMENT_LNM_Waypoint().c_str(), i1);
          if (!node.isEmpty())
          {
            mx_local_fpln_strct leg;

            leg.indx   = ++legCounter;
            leg.name   = read_node_text(node, mxconst::get_ELEMENT_LNM_Name());
            leg.ident  = read_node_text(node, mxconst::get_ELEMENT_LNM_Ident());
            leg.type   = read_node_text(node, mxconst::get_ELEMENT_LNM_Type());
            leg.region = read_node_text(node, mxconst::get_ELEMENT_LNM_Region());
            leg.p      = read_pos(node);
            Utils::addElementToMap(fpln, leg.indx, leg);
          }
        }

        if (fpln.size() > static_cast<size_t> ( 0 ) )
        {
          auto iter_last = fpln.end();
          --iter_last;
          ;
          iter_last->second.flag_isLast = true;
        }
      }
    }
  }



  return fpln;
}


// -----------------------------------


IXMLNode
data_manager::init_littlenavmap_missionInfo(IXMLNode& inNode)
{
  assert(inNode.isEmpty() == false && "<mission_info> must be valid node.");
  std::map<std::string, std::string> mapAttributes = { { mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME(), "random.png" }, { mxconst::get_ATTRIB_WRITTEN_BY(), "Mission-X Conversion" }, { mxconst::get_ATTRIB_WEATHER_SETTINGS(), "Check mission briefing" } };

  for (const auto& [attrib, attrib_val] : mapAttributes)
  {
    std::string val_s = Utils::readAttrib(inNode, attrib, attrib_val);
    inNode.updateAttribute(val_s.c_str(), attrib.c_str(), attrib.c_str());
  }

  return inNode;
}

// -----------------------------------

void
data_manager::add_advanceSettingsDateTime_and_Weather_to_node(IXMLNode& xGlobalSettings, IXMLNode& inPropNode, const std::string& inCurrentWeatherDatarefs_s)
{
  // v3.303.8
  // Global Settings starting hour from user preferred settings - if was defined
  const std::string starting_day_s   = Utils::readAttrib(inPropNode,   mxconst::get_PROP_STARTING_DAY(), "");
  const std::string starting_hour    = Utils::readAttrib(inPropNode,   mxconst::get_PROP_STARTING_HOUR(), "");
  const std::string starting_minutes = Utils::readAttrib(inPropNode,   mxconst::get_PROP_STARTING_MINUTE(), "");


  std::string starting_day = Utils::getAndFixStartingDayValue(starting_day_s);

  if (!starting_hour.empty())
  {
    auto xStartTime = xGlobalSettings.getChildNode(mxconst::get_ELEMENT_START_TIME().c_str());
    if (!xStartTime.isEmpty())
    {
      xStartTime.updateAttribute(starting_day.c_str(), mxconst::get_ATTRIB_TIME_DAY_IN_YEAR().c_str(), mxconst::get_ATTRIB_TIME_DAY_IN_YEAR().c_str());
      xStartTime.updateAttribute(starting_hour.c_str(), mxconst::get_ATTRIB_TIME_HOURS().c_str(), mxconst::get_ATTRIB_TIME_HOURS().c_str());
      xStartTime.updateAttribute(starting_minutes.c_str(), mxconst::get_ATTRIB_TIME_MIN().c_str(), mxconst::get_ATTRIB_TIME_MIN().c_str());
    }
  }
  // end v3.303.8

  // v3.303.12 add weather settings based on
  const std::string weather_settings_s             = Utils::readAttrib(inPropNode,   mxconst::get_PROP_WEATHER_USER_PICKED(), "");
  const std::string weather_change_mode_settings_s = Utils::readAttrib(inPropNode,   mxconst::get_PROP_WEATHER_CHANGE_MODE_USER_PICKED(), "");

  if (!weather_settings_s.empty())
  {
    // v3.303.13 added option to store current weather settings.
    if (mxconst::get_VALUE_STORE_CURRENT_WEATHER_DATAREFS() == weather_settings_s)
    {
      Utils::xml_delete_all_subnodes(xGlobalSettings, mxconst::get_ELEMENT_WEATHER());
      IXMLNode xWeather = Utils::xml_get_or_create_node_ptr(xGlobalSettings, mxconst::get_ELEMENT_WEATHER());
      if (!xWeather.isEmpty())
      {
        Utils::xml_set_text(xWeather, inCurrentWeatherDatarefs_s);
      }
    }
    else
    {
      const auto vecPicked = Utils::splitStringToNumbers<int> (weather_settings_s, ",");
      const int  rndPick   = Utils::getRandomIntNumber (0, static_cast<int> (vecPicked.size ()) - 1);

      const auto vecPickedChangeMode = Utils::splitStringToNumbers<int> (weather_change_mode_settings_s, ",");
      const int  rndPickChangeMode   = (vecPickedChangeMode.empty ()) ? -1 : Utils::getRandomIntNumber (0, static_cast<int> (vecPickedChangeMode.size ()) - 1); // if vector is empty then no change mode was picked (-1)

      #ifndef RELEASE
      Log::logMsgThread("[random weather] Picked from vector: " + mxUtils::formatNumber<int>(rndPick));
      #endif // !RELEASE

      if (rndPick >= 0 && rndPick < static_cast<int> ( vecPicked.size () ) )
      {
        const int iWeatherPicked = vecPicked.at(rndPick);

        std::map<int, std::unordered_map<std::string, std::string>>* ptr_mapWeatherPreDefinedStrct = (xplane_ver_i < XP12_VERSION_NO) ? &data_manager::mapWeatherPreDefinedStrct_xp11 : &data_manager::mapWeatherPreDefinedStrct_xp12;
        assert(ptr_mapWeatherPreDefinedStrct != nullptr && "random weather pointer can't be null"); // debug

        #ifndef RELEASE
        Log::logMsgThread("[weather code] Picked: " + mxUtils::formatNumber<int>(iWeatherPicked));
        #endif // !RELEASE
        if (Utils::isElementExists((*ptr_mapWeatherPreDefinedStrct), iWeatherPicked))
        {
          Utils::xml_delete_all_subnodes(xGlobalSettings, mxconst::get_ELEMENT_WEATHER());
          IXMLNode xWeather = Utils::xml_get_or_create_node_ptr(xGlobalSettings, mxconst::get_ELEMENT_WEATHER());
          if (!xWeather.isEmpty())
          {
            int               iWeatherChangeModeValueFromVector = (rndPickChangeMode >= 0 && rndPickChangeMode < static_cast<int> ( vecPickedChangeMode.size () ) ) ? vecPickedChangeMode.at(rndPickChangeMode) : -1; // -1 = invalid value
            const std::string sWeatherDatarefAsText             = get_weather_state(iWeatherPicked) + ((rndPickChangeMode < 0) ? "" : "|sim/weather/region/change_mode=" + mxUtils::formatNumber<int>(iWeatherChangeModeValueFromVector));

            // add the weather datarefs as text
            if (!sWeatherDatarefAsText.empty())
              Utils::xml_set_text(xWeather, sWeatherDatarefAsText);

            #ifndef RELEASE
            Log::logMsgThread("Weather Dataref Added as text:\n" + sWeatherDatarefAsText + "\n"); // debug
            #endif
          }
        } // end if key "iWeatherPicked" exists in map
      }
    } // end else if user picked to store "current X-Plane" settings or to randomly pick one.

  } // end add weather

  // v3.303.14 Weight settings
  if (Utils::readBoolAttrib(inPropNode,   mxconst::get_PROP_ADD_DEFAULT_WEIGHTS_TO_PLANE(), true) == false)
  {
    Utils::xml_delete_all_subnodes(xGlobalSettings, mxconst::get_ELEMENT_BASE_WEIGHTS_KG());
  }
  else
  {
    if (auto xBaseWeights = xGlobalSettings.getChildNode ( mxconst::get_ELEMENT_BASE_WEIGHTS_KG().c_str () ); !xBaseWeights.isEmpty () )
    {
      // get the user defined weights (or default weights)
      const auto pilotWeight      = prop_userDefinedMission_ui.getAttribNumericValue<float> ( mxconst::get_OPT_PILOT_BASE_WEIGHT(), 0.0f );
      const auto storedWeight     = prop_userDefinedMission_ui.getAttribNumericValue<float> ( mxconst::get_OPT_STORAGE_BASE_WEIGHT(), 0.0f );
      const auto passengersWeight = prop_userDefinedMission_ui.getAttribNumericValue<float> ( mxconst::get_OPT_PASSENGERS_BASE_WEIGHT(), 0.0f );

      // set the global_settings weight sub node
      Utils::xml_set_attribute_in_node<float> ( xBaseWeights, mxconst::get_OPT_PILOT_BASE_WEIGHT(), pilotWeight );
      Utils::xml_set_attribute_in_node<float> ( xBaseWeights, mxconst::get_OPT_STORAGE_BASE_WEIGHT(), storedWeight );
      Utils::xml_set_attribute_in_node<float> ( xBaseWeights, mxconst::get_OPT_PASSENGERS_BASE_WEIGHT(), passengersWeight );
    }
  }


}

// -----------------------------------

bool
data_manager::generate_missionx_mission_file_from_convert_screen(mx_base_node                       inPropNode,
                                                                 // std::string                                  inOriginalFlightPlanFileName,
                                                                 IXMLNode&                                    inMainNode,
                                                                 IXMLNode&                                    inout_xGlobalSettingsFromConversionFile,
                                                                 std::map<int, mx_local_fpln_strct> in_map_tableOfParsedFpln,
                                                                 bool                                         inStoreState_b,
                                                                 bool                                         inGenerateNewGlobalSettingsNode_b)
{
  // get <MAPPING> element like in TEMPLATE
  // If debug then write the main node to log in order to see what we have

  assert(inMainNode.isEmpty() == false && "Main node element can't be empty...");
  #ifndef RELEASE
  if (!inMainNode.isEmpty())
  {
    IXMLRenderer render;
    Log::logMsgNone("\nBEFORE IN MEMORY XML\n================\n" + std::string(render.getString(inMainNode)) + "\n");
  }
  #endif

  auto xMAIN_NODE = inMainNode.deepCopy();
  auto xDummy_ptr = xMAIN_NODE.getChildNode(mxconst::get_DUMMY_ROOT_DOC().c_str());
  assert(xDummy_ptr.isEmpty() == false && "Dummy node element is missing, notify the developer !!!");


  //// Construct the <MISSION> element
  // Add <MISSION> element
  auto root = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_MISSION_ELEMENT());
  root.updateAttribute("From_lnm_fpln_to_mission",  mxconst::get_ATTRIB_NAME().c_str(),  mxconst::get_ATTRIB_NAME().c_str());
  root.updateAttribute("Converted from LittleNavMap flight plan to a Mission File", mxconst::get_ATTRIB_TITLE().c_str(), mxconst::get_ATTRIB_TITLE().c_str());

  xMAIN_NODE.addChild(root);


  auto xMissionInfo = xDummy_ptr.getChildNode(mxconst::get_ELEMENT_MISSION_INFO().c_str()).deepCopy();
  xMissionInfo      = init_littlenavmap_missionInfo(xMissionInfo);

  root.addChild(xMissionInfo);
  Utils::add_xml_comment(root);

  // v3.305.1 add global_settings
  // auto xGlobalSettings = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_GLOBAL_SETTINGS().c_str());
  auto xGlobalSettings = (!inout_xGlobalSettingsFromConversionFile.isEmpty() && !inGenerateNewGlobalSettingsNode_b) ? inout_xGlobalSettingsFromConversionFile.deepCopy() : Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_GLOBAL_SETTINGS().c_str());
  root.addChild(xGlobalSettings);
  Utils::add_xml_comment(root);

  IXMLNode xBriefer = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_ELEMENT_BRIEFER());
  root.addChild(xBriefer);
  Utils::add_xml_comment(root);

  IXMLNode xGPS = root.addChild(mxconst::get_ELEMENT_GPS().c_str());
  Utils::add_xml_comment(root);

  IXMLNode xXpdata = root.addChild(xDummy_ptr.getChildNode(mxconst::get_ELEMENT_XPDATA().c_str()).deepCopy());
  Utils::add_xml_comment(root);


  // v3.303.14 add date/time and weather
  // Global Settings starting hour from user preferred settings - if was defined
  if (inGenerateNewGlobalSettingsNode_b) // v3.305.1 Generate a new "global_settings" only if we did not use the stored <global_settings> from the "converter.sav" file.
  {
    const std::string weather_state_as_datarefs_string_s = get_weather_state();
    add_advanceSettingsDateTime_and_Weather_to_node(xGlobalSettings, inPropNode.node, weather_state_as_datarefs_string_s);
    inout_xGlobalSettingsFromConversionFile = xGlobalSettings.deepCopy();
  }



  bool flagFoundBriefer = false;
  bool flagIsFirstLeg   = false;
  auto tableCopy        = in_map_tableOfParsedFpln; // used to find next leg

  // construct the flight plan <flight_plan>
  for (auto& [indx, legData] : in_map_tableOfParsedFpln)
  {
    legData.xConv  = legData.getFplnAsRawXML(); // v3.0.303.4
    bool isBriefer = (indx == 0 || legData.flag_convertToBriefer);

    if (isBriefer || (indx == 1 && flagFoundBriefer == false)) // we pick the briefer(indx=0) or the first flight leg in the table(indx=1) if briefer(indx=0) is not exists
    {
      flagFoundBriefer                  = true;
      const std::string heading_psi_s   = Utils::readAttrib(legData.xLeg, mxconst::get_ATTRIB_HEADING_PSI(), "0");
      const std::string start_elev_ft_s = Utils::readAttrib(legData.xLeg, mxconst::get_ATTRIB_ELEV_FT(), "0");
      const std::string desc_s          = Utils::xml_read_cdata_node(legData.xLeg, "Welcome pilot\nFly the suggested route, prepare the plane and the needed charts.\nBlue Skies\nMission-X");

      // store in conversion
      Utils::xml_set_attribute_in_node_asString(legData.xConv, mxconst::get_ATTRIB_HEADING_PSI(), heading_psi_s, mxconst::get_ELEMENT_FPLN());
      Utils::xml_set_attribute_in_node_asString(legData.xConv, mxconst::get_ATTRIB_ELEV_FT(), start_elev_ft_s, mxconst::get_ELEMENT_FPLN());
      Utils::xml_add_cdata(legData.xConv, desc_s);

      // update briefer
      Utils::xml_search_and_set_attribute_in_IXMLNode(xBriefer, mxconst::get_ATTRIB_STARTING_ICAO(), legData.ident, mxconst::get_ELEMENT_BRIEFER());

      Utils::xml_search_and_set_attribute_in_IXMLNode(xBriefer, mxconst::get_ATTRIB_LAT(), mxUtils::formatNumber<double>(legData.p.getLat(), 8), mxconst::get_ELEMENT_LOCATION_ADJUST());
      Utils::xml_search_and_set_attribute_in_IXMLNode(xBriefer, mxconst::get_ATTRIB_LONG(), mxUtils::formatNumber<double>(legData.p.getLon(), 8), mxconst::get_ELEMENT_LOCATION_ADJUST());
      Utils::xml_search_and_set_attribute_in_IXMLNode(xBriefer, mxconst::get_ATTRIB_ELEV_FT(), start_elev_ft_s, mxconst::get_ELEMENT_LOCATION_ADJUST());
      Utils::xml_search_and_set_attribute_in_IXMLNode(xBriefer, mxconst::get_ATTRIB_HEADING_PSI(), heading_psi_s, mxconst::get_ELEMENT_LOCATION_ADJUST());
      Utils::xml_add_cdata(xBriefer, desc_s);
    }
    else
    {
      if (legData.flag_ignore_leg)
        continue;


      // add to GPS
      if (xGPS.isEmpty() == false)
      {
        assert(legData.p.node.isEmpty() == false && "[generate mission conv] Nav info from FPLN can't be empty");

        IXMLNode xPoint = legData.p.node.deepCopy();
        // v3.305.3 fix wrong element tag name. Was <node> should be <point>
        xPoint.updateName(mxconst::get_ELEMENT_POINT().c_str());

        Utils::xml_search_and_set_attribute_in_IXMLNode(xPoint, mxconst::get_ELEMENT_ICAO(), legData.ident, xPoint.getName());
        Utils::xml_search_and_set_attribute_in_IXMLNode(xPoint, mxconst::get_ATTRIB_LOC_DESC(), legData.getName(), xPoint.getName());

        xGPS.addChild(xPoint);
      }



      if (legData.flag_isLeg == false) // remove flight plan that are not legs
      {
        continue;
      }


      // Add <flight_plan>
      assert(legData.xFlightPlan.isEmpty() == false && " - <flight_plan> can't be empty.");


      // v3.0.303.2 add markers
      if (legData.flag_add_marker)
      {
        // add <display_object>
        auto xDisplayObject = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_ELEMENT_DISPLAY_OBJECT());
        assert(xDisplayObject.isEmpty() == false && "Display Object can't be empty: ");
        assert(legData.marker_type_i < static_cast<int> ( ( mxconst::get_vecMarkerTypeOptions_markers().size () - (size_t)1 ) ) && "Marker type is not in vector ");


        xDisplayObject.updateAttribute(mxconst::get_vecMarkerTypeOptions_markers().at(legData.marker_type_i).c_str(),  mxconst::get_ATTRIB_NAME().c_str(),  mxconst::get_ATTRIB_NAME().c_str()); // marker01..04
        xDisplayObject.updateAttribute((legData.attribName + "marker").c_str(), mxconst::get_ATTRIB_INSTANCE_NAME().c_str(), mxconst::get_ATTRIB_INSTANCE_NAME().c_str());
        // position
        xDisplayObject.updateAttribute(legData.p.getLat_s().c_str(), mxconst::get_ATTRIB_REPLACE_LAT().c_str(), mxconst::get_ATTRIB_REPLACE_LAT().c_str());
        xDisplayObject.updateAttribute(legData.p.getLon_s().c_str(), mxconst::get_ATTRIB_REPLACE_LONG().c_str(), mxconst::get_ATTRIB_REPLACE_LONG().c_str());

        // v3.0.303.2 radius to display
        const std::string distance_to_display_s = (legData.radius_to_display_3D_marker_in_nm_f <= 2.0f) ? "5.0" : mxUtils::formatNumber<float>(legData.radius_to_display_3D_marker_in_nm_f, 2);
        xDisplayObject.updateAttribute(distance_to_display_s.c_str(), mxconst::get_ATTRIB_REPLACE_DISTANCE_TO_DISPLAY_NM().c_str(), mxconst::get_ATTRIB_REPLACE_DISTANCE_TO_DISPLAY_NM().c_str());

        xDisplayObject.updateAttribute("yes", mxconst::get_ATTRIB_TARGET_MARKER_B().c_str(), mxconst::get_ATTRIB_TARGET_MARKER_B().c_str()); // flag the marker as target so user can hide/show them at will
        // elevation
        if (legData.target_trig_strct.flag_on_ground)
        {
          xDisplayObject.updateAttribute("60", mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str(), mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str());
        }
        else
        {
          std::string sign_s;
          switch (legData.target_trig_strct.elev_combo_picked_i)
          {
            case 0: // ignore
            case 1: // onGround
            {
              xDisplayObject.updateAttribute("60", mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str(), mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str());
            }
            break;
            case 2: // min/max - we will place marker at center height
            {
              xDisplayObject.updateAttribute(Utils::formatNumber<float>( static_cast<float> ( legData.target_trig_strct.elev_max + legData.target_trig_strct.elev_min ) * 0.5f, 2).c_str(), mxconst::get_ATTRIB_REPLACE_ELEV_FT().c_str(), mxconst::get_ATTRIB_REPLACE_ELEV_FT().c_str());
            }
            break;
            case 3: // Lower than
            case 4: // above than
            {
              sign_s                   = (legData.target_trig_strct.elev_combo_picked_i == 3) ? "--" : "++";                                                                                                              // which sign to search for ?
              const float elev_f       = ((legData.target_trig_strct.elev_lower_upper.find(sign_s) == 0) ? mxUtils::stringToNumber<float>(legData.target_trig_strct.elev_lower_upper.substr(sign_s.length())) : 1000.0f); // return the number after the "--"/"++"
              const float multiplyBy_f = (legData.target_trig_strct.elev_combo_picked_i == 3) ? 0.5f : 1.2f;
              xDisplayObject.updateAttribute(mxUtils::formatNumber<float>(elev_f * multiplyBy_f, 2).c_str(), mxconst::get_ATTRIB_REPLACE_ELEV_FT().c_str(), mxconst::get_ATTRIB_REPLACE_ELEV_FT().c_str());
            }
            break;
            case 5: // highest allowed elev above ground ---
            case 6: // lowest allowed elevation above ground +++
            {
              sign_s                   = (legData.target_trig_strct.elev_combo_picked_i == 5) ? "---" : "+++";                                                                                                            // which sign to search for ?
              const float elev_f       = ((legData.target_trig_strct.elev_lower_upper.find(sign_s) == 0) ? mxUtils::stringToNumber<float>(legData.target_trig_strct.elev_lower_upper.substr(sign_s.length())) : 1000.0f); // return the number after the "---"/"+++"
              const float multiplyBy_f = (legData.target_trig_strct.elev_combo_picked_i == 5) ? 0.5f : 1.2f;
              xDisplayObject.updateAttribute(mxUtils::formatNumber<float>(elev_f * multiplyBy_f, 2).c_str(), mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str(), mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str());
            }
            break;
            default:
              xDisplayObject.updateAttribute("500", mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str(), mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str());
              break;
          } // end switch elevation picked

        } // end prepare <display_object>

        Utils::xml_delete_all_subnodes(legData.xLeg, mxconst::get_ELEMENT_DISPLAY_OBJECT()); // remove previous display node due to duplications
        legData.xLeg.addChild(xDisplayObject);

      } // legData.flag_add_marker



      root.addChild(legData.xFlightPlan);


      Utils::add_xml_comment(root);
      Utils::add_xml_comment(root);


      // We should always use "deepCopy" or XP will crash the next time the designer will click on [generate] due to empty <DUMMY> element
      // move all messages to xMsgTempl
      for (int i1 = legData.xLeg.nChildNode() - 1; i1 >= 0; --i1)
      {
        auto const lmbda_remove_same_message_name = [](const IXMLNode& inSourceNode_ptr, IXMLNode& inOutMessageTemplateNode_ptr)
        {
          // delete same message name if exists
          std::string  attrib_name_value_s = Utils::readAttrib(inSourceNode_ptr, mxconst::get_ATTRIB_NAME(), ""); // mxconst::ELEMENT_MESSAGE
          IXMLNode    xSameMessage_ptr    = Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(inOutMessageTemplateNode_ptr, mxconst::get_ELEMENT_MESSAGE (), mxconst::get_ATTRIB_NAME(), attrib_name_value_s);
          if (!xSameMessage_ptr.isEmpty())        // if exists
            xSameMessage_ptr.deleteNodeContent(); // delete XML Content
        };

        // move all <message> elements from level 1 or 2 to xMsgTempl
        if (std::string(legData.xLeg.getChildNode(i1).getName()) == mxconst::get_ELEMENT_MESSAGE ())
        {
          #ifdef IBM
          lmbda_remove_same_message_name(legData.xLeg.getChildNode(i1), legData.xMessageTmpl); // v3.0.303.7 remove message with same name
          #else
          IXMLNode node_ptr = legData.xLeg.getChildNode(i1);
          lmbda_remove_same_message_name(node_ptr, legData.xMessageTmpl); // v3.0.303.7 remove message with same name
          #endif

          legData.xConv.addChild(legData.xLeg.getChildNode(i1).deepCopy()); // v3.0.303.4
          legData.xMessageTmpl.addChild(legData.xLeg.getChildNode(i1));
        }
        else if (legData.xLeg.getChildNode(i1).nChildNode() > 0 && std::string(legData.xLeg.getChildNode(i1).getChildNode().getName()) == mxconst::get_ELEMENT_MESSAGE())
        {
          #ifdef IBM
          lmbda_remove_same_message_name(legData.xLeg.getChildNode(i1).getChildNode(), legData.xMessageTmpl); // v3.0.303.7 remove message with same name
          #else
          IXMLNode node_ptr = legData.xLeg.getChildNode(i1).getChildNode();
          lmbda_remove_same_message_name(node_ptr, legData.xMessageTmpl); // v3.0.303.7 remove message with same name
          #endif

          legData.xConv.addChild(legData.xLeg.getChildNode().deepCopy()); // v3.0.303.4
          legData.xMessageTmpl.addChild(legData.xLeg.getChildNode(i1).getChildNode());
        }
      }

      // add link triggers to <leg> only if their name does not have the word "task" in them.
      // This is useful if the information was read from save state and not for a new mission
      for (int i1 = 0; i1 < legData.xTriggers.nChildNode(mxconst::get_ELEMENT_TRIGGER().c_str()); ++i1)
      {
        auto              xTrig    = legData.xTriggers.getChildNode(mxconst::get_ELEMENT_TRIGGER().c_str(), i1);
        const std::string trigName = Utils::readAttrib(xTrig, mxconst::get_ATTRIB_NAME(), "");
        if (!trigName.empty() && trigName.find("task") == std::string::npos)
        {
          // check if we have <link_to_trigger> with same name, if not then add it
          IXMLNode xLink = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(legData.xLeg, mxconst::get_ELEMENT_LINK_TO_TRIGGER(), mxconst::get_ATTRIB_NAME(), trigName, false);
          if (xLink.isEmpty())
          {
            xLink = legData.xLeg.addChild(mxconst::get_ELEMENT_LINK_TO_TRIGGER().c_str());
            xLink.updateAttribute(trigName.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
          }

        } // end if trigName is valid

      } // end loop over all Leg xTriggers and add links to the <leg> element except for triggers with the word "task" in their name


      // Add mandatory target/task to the <flight leg>. This means: <objective>, <task> and <trigger>
      // Add <objective> - start with Trigger(physical location) then <task> then <objective>
      {

        auto xObjective = Utils::xml_get_or_create_node_ptr(legData.xObjectives, mxconst::get_ELEMENT_OBJECTIVE());
        xObjective.updateAttribute((legData.attribName + "_obj").c_str(), mxconst::get_ATTRIB_NAME().c_str()); // only one attribute in this element

        // auto xTrigger = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_ELEMENT_TRIGGER());
        std::string trigName = legData.attribName + "_task_trig";
        auto        xTrigger = Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(legData.xTriggers, mxconst::get_ELEMENT_TRIGGER(), mxconst::get_ATTRIB_NAME(), trigName);


        bool bNewTrigger = true;
        if (xTrigger.isEmpty()) // make sure we do not recreate the trigger. This can happen if we are updating the trigger and then clicking [generate]
        {
          xTrigger = Utils::xml_get_or_create_node_ptr(legData.xTriggers, mxconst::get_ELEMENT_TRIGGER(), mxconst::get_ATTRIB_NAME(), trigName);
        }
        else // v3.0.303.4 Added to make sure that reloaded task triggers will always have correct radius
        {
          bNewTrigger                   = false;
          bool              bFlagFound  = false;
          const std::string length_mt_s = Utils::xml_get_attribute_value_drill(xTrigger, mxconst::get_ATTRIB_LENGTH_MT(), bFlagFound, mxconst::get_ELEMENT_RADIUS());
          int               length_mt_i = 0;

          if (mxUtils::is_number(length_mt_s))
            length_mt_i = mxUtils::stringToNumber<int>(length_mt_s);

          if (legData.target_trig_strct.radius_of_trigger_mt <= 0)
            legData.target_trig_strct.radius_of_trigger_mt = (length_mt_s.empty()) ? 0 : length_mt_i;
        }

        assert(xTrigger.isEmpty() == false && " Target trigger node can't be empty.");

        if (xTrigger.isEmpty() == false)
        {
          xTrigger.updateAttribute(trigName.data(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
          // if new then there is no trigger type present
          if (bNewTrigger)
            xTrigger.updateAttribute(mxconst::get_TRIG_TYPE_RAD().c_str(), mxconst::get_ATTRIB_TYPE().c_str(), mxconst::get_ATTRIB_TYPE().c_str());

          // Determine waypoint radius
          legData.target_trig_strct.radius_of_trigger_mt = legData.validate_and_fix_trigger_radius(); // v3.0.303.4
          Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_LENGTH_MT(), mxUtils::formatNumber<int>(legData.target_trig_strct.radius_of_trigger_mt), mxconst::get_ELEMENT_RADIUS()); // v3.0.303.4

          Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_LAT(), mxUtils::formatNumber<double>(legData.p.lat, 8), mxconst::get_ELEMENT_POINT());
          Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_LONG(), mxUtils::formatNumber<double>(legData.p.lon, 8), mxconst::get_ELEMENT_POINT());
          // trigger <conditions>
          Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND(), legData.target_trig_strct.elev_rule_s, mxconst::get_ELEMENT_CONDITIONS());
          if (legData.target_trig_strct.elev_rule_s == "false")
          {
            Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT(), legData.target_trig_strct.elev_lower_upper, mxconst::get_ELEMENT_ELEVATION_VOLUME());
          }

          legData.xTriggers.addChild(xTrigger);
        }


        IXMLNode xTask = Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(legData.xObjectives, mxconst::get_ELEMENT_TASK(), mxconst::get_ATTRIB_NAME(), legData.attribName + "task");
        if (xTask.isEmpty())
        {
          xTask = Utils::xml_get_or_create_node_ptr(legData.xObjectives, mxconst::get_ELEMENT_TASK(), mxconst::get_ATTRIB_NAME(), legData.attribName + "task");
          xTask.updateAttribute(trigName.data(), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str(), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str()); // set task base on trigger
          xTask.updateAttribute("true", mxconst::get_ATTRIB_MANDATORY().c_str(), mxconst::get_ATTRIB_MANDATORY().c_str());                      // set task as mandatory or plugin will just flag it as success

          xTask.updateAttribute("2", mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str(), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str()); // 2 seconds

          xObjective.addChild(xTask);
        }


        // link objective to <leg>
        auto linkToObjective = Utils::xml_get_or_create_node_ptr(legData.xLeg, mxconst::get_ELEMENT_LINK_TO_OBJECTIVE());
        linkToObjective.updateAttribute(Utils::readAttrib(xObjective, mxconst::get_ATTRIB_NAME(), "").c_str(), mxconst::get_ATTRIB_NAME().c_str());
      }
      // end objective



      legData.xLeg.updateAttribute(legData.attribName.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
      legData.xLeg.updateAttribute(legData.getName().c_str(), mxconst::get_ATTRIB_TITLE().c_str(), mxconst::get_ATTRIB_TITLE().c_str());
      // inject next leg
      for (auto& [i, s] : tableCopy)
      {
        if (i <= legData.indx)
          continue;

        if (s.flag_isLeg && !s.flag_ignore_leg)
        {
          legData.xLeg.updateAttribute(s.attribName.c_str(), mxconst::get_ATTRIB_NEXT_LEG().c_str(), mxconst::get_ATTRIB_NEXT_LEG().c_str());
          break;
        }
      }

      // xFlightPlan.addChild(legData.xLeg.deepCopy(), 0); // no need since it is internal in legData

      if (flagIsFirstLeg == false) // set <briefer> starting flight leg
      {
        flagIsFirstLeg = true;
        Utils::xml_search_and_set_attribute_in_IXMLNode(xBriefer, mxconst::get_ATTRIB_STARTING_LEG(), legData.attribName, mxconst::get_ELEMENT_BRIEFER());
      }
    }
  } // end loop over all waypoints table


  Utils::add_xml_comment(root);
  Utils::add_xml_comment(root, " +++++++ 3D Object Templates +++++++ ");

  // v3.0.302.2
  auto xObj3DTemplate = Utils::xml_get_node_from_XSD_map_as_acopy("template_markers_obj3d"); // v3.0.303.3 this template holds the 4 3D marker types/files
  xObj3DTemplate.updateName(mxconst::get_ELEMENT_OBJECT_TEMPLATES().c_str());
  root.addChild(xObj3DTemplate); // v3.0.303.2 add Object3D Template

  Utils::add_xml_comment(root);
  Utils::add_xml_comment(root);


  // End element
  auto xEnd = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_ELEMENT_END_MISSION());
  root.addChild(xEnd);
  Utils::add_xml_comment(root);



  if (!xMAIN_NODE.isEmpty())
  {
    if (!xDummy_ptr.isEmpty())
      xDummy_ptr.deleteNodeContent();

    auto const lmbda_write_mission = [](const IXMLNode& inIXMLNode, const std::string& file_location)
    {
      assert(inIXMLNode.isEmpty() == false && "XML can't be empty.");
      assert(file_location.empty() == false && "Save location can't be empty.");

      IXMLRenderer  xmlWriter; //
      IXMLErrorInfo errInfo = xmlWriter.writeToFile(inIXMLNode, file_location.c_str(), "ASCII");
      if (errInfo != IXMLError_None)
      {
        std::string translatedError;
        translatedError.clear();

        translatedError = IXMLRenderer::getErrorMessage(errInfo);

        Log::logMsg("[generate conv] Error code while writing: " + translatedError + " (Check save folder is set: " + file_location + ")"); // v3.0.255.3 minor wording and save path modification
      } // end if fail to write
      else
      {
        Log::logMsg("Wrote: " + file_location);
      }
    };

    std::string savePathAndFile = Utils::getMissionxCustomSceneryFolderPath_WithSep(true) + "/random/briefer/random.xml";
    lmbda_write_mission(xMAIN_NODE, savePathAndFile); // no need to send variables since we defined something ahead of time.

    if (inStoreState_b)
    {
      savePathAndFile = Utils::getMissionxCustomSceneryFolderPath_WithSep(true) + "/random/briefer/" + mxconst::get_CONVERTER_FILE();
      auto local_root       = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_CONVERSION_ROOT_DOC());

      if (!local_root.isEmpty())
      {
        // v3.305.1 add global_settings
        auto xStoreGlobalSettingNode = (!inout_xGlobalSettingsFromConversionFile.isEmpty() && !inGenerateNewGlobalSettingsNode_b) ? inout_xGlobalSettingsFromConversionFile.deepCopy() : xGlobalSettings.deepCopy();

        if (!xStoreGlobalSettingNode.isEmpty())
        {
          local_root.addChild(xStoreGlobalSettingNode);
          Utils::add_xml_comment(local_root);
        }


        for ( auto &legData : in_map_tableOfParsedFpln | std::views::values )
        {
          local_root.addChild(legData.xConv.deepCopy());
          Utils::add_xml_comment(local_root); // v3.305.1
        }

        local_root.addChild(xXpdata.deepCopy());

        lmbda_write_mission(local_root, savePathAndFile); // no need to send variables since we defined something ahead of time.
      }

    } // inStoreState_b
  }


  return true; // success
}

// -----------------------------------

std::string
data_manager::getPoint_as_stringFromPlaneCamera(const std::string& inDatarefStringLat, const std::string inDatarefStringLon, char origin_c)
{

  double            elevFt_d = 0.0;
  const auto drefLat  = XPLMFindDataRef(inDatarefStringLat.c_str());
  const auto drefLon  = XPLMFindDataRef(inDatarefStringLon.c_str());
  auto              latVal   = XPLMGetDataf(drefLat);
  auto              lonVal   = XPLMGetDataf(drefLon);

  if (origin_c == 'c')
  {
    double       outX, outY; //, outZ;
    auto         drefElev = XPLMFindDataRef ( "sim/graphics/view/view_z" ); // OGLcoords
    const double z        = XPLMGetDataf ( drefElev );
    XPLMLocalToWorld ( latVal, lonVal, z, &outX, &outY, &elevFt_d );
    latVal = static_cast<float> ( outX );
    lonVal = static_cast<float> ( outY );
  }
  else
  {
    dataref_manager dm;
    // plane elevation in meters
    elevFt_d = dm.getElevation();
  }

  elevFt_d = elevFt_d * meter2feet; // convert to feet

  #ifndef RELEASE
  auto pos = Point(latVal, lonVal, elevFt_d);
  #endif // !RELEASE

  return Point(latVal, lonVal, elevFt_d).get_point_lat_lon_as_string();
}

// -----------------------------------

Point
data_manager::getPlane_or_Camera_position_as_Point(char origin_c)
{
  dataref_const dc;
  double        latVal{ 0.0 };
  double        lonVal{ 0.0 };
  double        z{ 0.0 }; // elevation

  if (origin_c == 'c') // camera
  {
    double outX, outY, outZ;

    // camera position
    latVal = static_cast<double> ( XPLMGetDataf ( dc.dref_camera_view_x_f ) );
    lonVal = static_cast<double> ( XPLMGetDataf ( dc.dref_camera_view_y_f ) );
    z      = static_cast<double> ( XPLMGetDataf ( dc.dref_camera_view_z_f ) );

    // convert local to world
    XPLMLocalToWorld(latVal, lonVal, z, &outX, &outY, &outZ);
    latVal = outX;
    lonVal = outY;
    z      = outZ;
  }
  else // plane
  {
    latVal = static_cast<double> ( XPLMGetDataf ( dc.dref_lat_d ) );
    lonVal = static_cast<double> ( XPLMGetDataf ( dc.dref_lon_d ) );
    z      = static_cast<double> ( XPLMGetDataf ( dc.dref_elev_d ) );
  }

  z *= meter2feet; // v24.03.2 fix elevation to be feet and not meter
  return Point(latVal, lonVal, z);
}

// -------------------------------------

ImVec2
data_manager::getImageSizeBasedOnBorders(float inWidth, float inHeight, float inBoundX, float inBoundY)
{
  ImVec2 size_vec2(inWidth, inHeight);

  float ratio = inBoundX / inWidth;
  if (inWidth > inBoundX)
  {
    size_vec2.x = inWidth * ratio;
    size_vec2.y = inHeight * ratio;
  }
  // make sure Height is in bounds
  if (size_vec2.y > inBoundY)
  {
    ratio       = inBoundY / size_vec2.y;
    size_vec2.x = size_vec2.x * ratio;
    size_vec2.y = size_vec2.y * ratio;
  }

  return size_vec2;
}

// -------------------------------------

void
data_manager::gather_custom_acf_datarefs_as_a_thread(const std::string& inAcfPath, bool* bThreadIsRunning, bool* bAbortThread)
{
  (*bThreadIsRunning)   = true;
  auto startThreadClock = std::chrono::steady_clock::now();


  std::unordered_set<std::string> customDatarefSet;

  fs::path acf_file = inAcfPath;
  if (exists(acf_file) && is_regular_file(acf_file))
  {
    customDatarefSet.clear();

    auto                            acfCustomDatarefSet = system_actions::search_datarefs_in_acf_file(acf_file.string());
    std::unordered_set<std::string> objCustomDatarefs;

    if (*bAbortThread)
    {
      (*bThreadIsRunning) = false;
      return;
    }

    fs::path acf_folder = acf_file.parent_path();
    // std::string debugParentPath_s = acf_folder.string();
    if (is_directory(acf_folder))
    {
      for (auto file : fs::recursive_directory_iterator(acf_folder))
      {
        if (*bAbortThread)
        {
          (*bThreadIsRunning) = false;
          return;
        }

        const auto ext = mxUtils::stringToLower(fs::path(file).extension().string()); // debug

        if (is_regular_file(file) && ext.compare(".obj") == 0)
        {

#ifndef RELEASE
          auto startThreadClock2 = std::chrono::steady_clock::now();
#endif // ! RELEASE

          std::set<std::string> drefKeySetFromObjFile = system_actions::search_datarefs_in_obj_file(file);

#ifndef RELEASE
          auto endCacheLoad = std::chrono::steady_clock::now();
          auto diff_cache   = endCacheLoad - startThreadClock2;
          auto duration     = std::chrono::duration<double, std::milli>(diff_cache).count();
          Log::logMsgThread(">> parsed: " + file.path().string() + ", Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec) ");
#endif

          objCustomDatarefs.insert(drefKeySetFromObjFile.cbegin(), drefKeySetFromObjFile.cend());
        }
      }

      customDatarefSet.insert(objCustomDatarefs.cbegin(), objCustomDatarefs.cend());
      customDatarefSet.insert(acfCustomDatarefSet.cbegin(), acfCustomDatarefSet.cend());

      auto itEnd = customDatarefSet.end();
      auto it    = customDatarefSet.begin();
      while (it != itEnd)
      {
        if ((*it).find("sim/") == 0 || (*it).empty()) // if start with "sim/" delete
        {
          it = customDatarefSet.erase(it);
        }
        else
          ++it;
      }

      // Add write to file code


      const std::string writable{ "y" };
      const std::string cacheFile = acf_folder.string() + "/" + mxconst::get_FILE_CUSTOM_ACF_CACHED_DATAREFS_NAME();

      std::ofstream fout;


      fout.open(cacheFile.c_str(), std::ofstream::out);
      // check open success
      if (fout.fail() || fout.bad())
      {
        Log::logMsgThread("[Error] Fail to find/create target file: [" + cacheFile + "]. Skip operation !!!");
      }
      else
      {
        // write all strings as line by line data sets
        for (const auto& s : customDatarefSet)
          fout << s << "\n";
      }

      if (fout.is_open())
        fout.close();

      // write to file - anonymous block

    } // if folder

  } // end acf_file is regular file


  // cleanup customDatarefSet
  auto endCacheLoad = std::chrono::steady_clock::now();
  auto diff_cache   = endCacheLoad - startThreadClock;
  auto duration     = std::chrono::duration<double, std::milli>(diff_cache).count();
  Log::logAttention("*** Finished [ gather_custom_acf_datarefs_as_a_thread() ]. Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec)  ****", true);


  (*bThreadIsRunning) = false;
} // gather_custom_acf_datarefs_as_a_thread

// -----------------------------------

//
// void
// data_manager::parse_max_weight_line(const std::string& line, std::map<int, float>& mapMaxWeight, int& outMaxStations)
// {
//   std::istringstream iss(line);
//   int                tokenCount = 0;
//
//   std::vector<std::string> vecTokens = mxUtils::split_skipEmptyTokens(line);
//
//   const std::string& lastToken = vecTokens.back();
//
//   // Test number or scientific number
//   float      f_last_token_value = 0.0;
//   const bool bIsNumberic        = mxUtils::isNumeric(lastToken);
//   const bool bIsSceintific      = mxUtils::isScientific(lastToken); // in the LR airbus, some station had scientific weight values.
//
//   if (!bIsNumberic && bIsSceintific)
//     f_last_token_value = static_cast<float>(mxUtils::convertScientificToDecimal(lastToken));
//   else
//     f_last_token_value = mxUtils::stringToNumber<float>(lastToken);
//
//
//   if ((bIsNumberic + bIsSceintific) && vecTokens.size() > 2)
//   {
//     const float v_third_token_num_value = f_last_token_value; // v_third_token_num_value can represent "weight" or "max stations in plane"
//
//     if (const auto pos = vecTokens.at(1).find_last_of('/'); pos != std::string::npos)
//     {
//       if (const std::string v_station_index_str = vecTokens.at (1).substr (pos + 1)
//         ; mxUtils::is_number(v_station_index_str))
//       {
//         const int v_station_index     = mxUtils::stringToNumber<int> (v_station_index_str);
//         mapMaxWeight[v_station_index] = v_third_token_num_value;
//       }
//       else if (v_station_index_str == "count")
//       {
//         outMaxStations = static_cast<int>(v_third_token_num_value);
//       }
//     }
//   }
// }
//
// // -----------------------------------
//
// void
// data_manager::parse_station_name_line(const std::string& line, std::map<int, std::string>& mapStationNames)
// {
//   std::istringstream iss(line);
//   std::string        tokens[10];
//   int                tokenCount = 0;
//
//   while (iss >> tokens[tokenCount] && tokenCount < 10)
//     tokenCount++;
//
//   if (const auto pos = tokens[1].find_last_of ('/')
//     ; pos != std::string::npos)
//   {
//     if (const std::string v_station_index_str = tokens[1].substr (pos + 1)
//       ; mxUtils::is_number(v_station_index_str))
//     {
//       int v_station_index = std::stoi(v_station_index_str);
//
//       std::string v_station_name;
//       for (int i = 2; i < tokenCount; ++i)
//       {
//         if (!v_station_name.empty())
//           v_station_name += " ";
//         v_station_name += tokens[i];
//       }
//       mapStationNames[v_station_index] = v_station_name;
//     }
//   }
// }
//
// // -----------------------------------

//void
//missionx::data_manager::gather_acf_cargo_data(Inventory& inoutPlaneInventory)
//{
//  auto startTimer = std::chrono::steady_clock::now();
//
//  std::array<std::string, 2> arr_text = { "P acf/_fixed_max", "P acf/_fixed_name" };
//
//  char outFileName[512]{ 0 };
//  char outPathAndFile[2048]{ 0 };
//  XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, outFileName, outPathAndFile); // we will only return the file name
//  fs::path acf_file = outPathAndFile;
//  std::ios_base::sync_with_stdio(false);
//  std::cin.tie(nullptr);
//
//  auto planeInventory = Inventory(mxconst::get_ELEMENT_PLANE(), mxconst::get_ELEMENT_PLANE());
//  #ifndef RELEASE
//  Log::log_to_missionx_log(fmt::format("Local inventory before reading [{}] file.\n==========>\n{}\n<=========", acf_file.string(), Utils::xml_get_node_content_as_text(planeInventory.node)));
//  #endif
//
//  if (fs::exists(acf_file) && fs::is_regular_file(acf_file))
//  {
//    std::ifstream file_toRead;
//    file_toRead.open(acf_file.string(), std::ios::in); // read the file
//    if (file_toRead.is_open())
//    {
//      int stations_i = 0;
//      // std::unordered_map<int, std::string> mapStationNames;
//      std::map<int, float>           mapMaxWeights;
//      std::string                    line;
//      char                           ch{ '\0' };
//      int                            char_counter_i = 0;
//      while (file_toRead.get(ch))
//      {
//        if ((char_counter_i > 17) + (ch == '\n'))
//        {
//          if (ch != '\n')
//          {
//            std::string restOfLine;
//            std::getline(file_toRead, restOfLine);
//            line.append(ch + restOfLine); // append to "line" the last character we read + the rest of line.
//          }
//          if (line.starts_with(arr_text[0])) // "P acf/_fixed_max"
//            data_manager::parse_max_weight_line(line, mapMaxWeights, stations_i);
//          else if (line.starts_with(arr_text[1])) // "P acf/_fixed_name"
//            data_manager::parse_station_name_line(line, planeInventory.map_acf_station_names);
//
//          line.clear();
//          char_counter_i = 0; // reset counter
//        }
//        else
//        {
//          line += ch;
//          ++char_counter_i;
//        }
//      }
//
//      // Initialize the inventory with the information. Only inventories with "max weight" higher than 0.0 will be listed.
//      for (const auto& [station_id, maxWeightKg] : mapMaxWeights)
//      {
//        if ((maxWeightKg > 0.0) && mxUtils::isElementExists(planeInventory.map_acf_station_names, station_id))
//        {
//          planeInventory.mapStations[station_id]                    = missionx::station(station_id, planeInventory.map_acf_station_names[station_id]);
//          planeInventory.mapStations[station_id].max_allowed_weight = maxWeightKg;
//
//          // Adding <station> node to "inventory" element.
//          auto stationNode_ptr = planeInventory.addChild(mxconst::get_ELEMENT_STATION(), mxconst::get_ATTRIB_ID(), fmt::format("{}", station_id), "");
//          if (!stationNode_ptr.isEmpty())
//          {
//            stationNode_ptr.updateAttribute(planeInventory.mapStations[station_id].name.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
//            stationNode_ptr.updateAttribute(fmt::format("{}", planeInventory.mapStations[station_id].max_allowed_weight).c_str(), mxconst::get_ATTRIB_WEIGHT_KG().c_str(), mxconst::get_ATTRIB_WEIGHT_KG().c_str());
//            planeInventory.mapStations[station_id].node = stationNode_ptr; // add pointer to node
//          }
//
//          Log::logMsg(fmt::format("Station - {}:{},\tMax Weight: {}kg.", station_id, planeInventory.map_acf_station_names[station_id], maxWeightKg));
//        }
//        else if (mxUtils::isElementExists(planeInventory.map_acf_station_names, station_id))
//        {
//          Log::logMsg(fmt::format("Station - {}:{} Does not have station max weight defined. skipping.", station_id, planeInventory.map_acf_station_names[station_id]));
//        }
//      }
//    } // finish read acf file
//  }   // end evaluate file exists
//
//  #ifndef RELEASE
//  Log::log_to_missionx_log( fmt::format( "Local Inventory Information:\n=================>\n [{}]", planeInventory.get_node_as_text()) ); // debug v24.12.2
//  #endif                                                                                                          // !RELEASE
//
//  if (data_manager::missionState >= mx_mission_state_enum::mission_loaded_from_the_original_file )
//  {
//    missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i = missionx::data_manager::get_inv_layout_based_on_mission_ver_and_compatibility_node(); // v24.12.2
//
//    inoutPlaneInventory = planeInventory;
//  }
//
//
//  #ifndef RELEASE
//  Log::log_to_missionx_log( fmt::format( "Plane Inventory Station Information:\n=================>\n [{}]", inoutPlaneInventory.get_node_as_text()) ); // debug v24.12.2
//  #endif  // !RELEASE
//
//  auto endTimer   = std::chrono::steady_clock::now();
//  auto diff_cache = endTimer - startTimer;
//  auto duration   = std::chrono::duration<double, std::milli>(diff_cache).count();
//  Log::logMsg(">> parsed: " + acf_file.string() + ", Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration * 0.001), 3) + "sec) ");
//}

// -----------------------------------

mx_return
data_manager::check_mandatory_item_move(const IXMLNode& item_node, const std::string& target_inventory_name)
{
  mx_return result(true);

  if (item_node.isEmpty())
  {
    result.result = false;
    result.addErrMsg("Item Node is empty. Notify developer.");
    return result;
  }

  const std::string item_name = Utils::readAttrib(item_node,  mxconst::get_ATTRIB_NAME(), "");

  if (target_inventory_name.empty() + ( ! mxUtils::isElementExists(mapInventories, target_inventory_name) ) )
  {
    result.result = false;
    result.addErrMsg( "Inventory is either 'empty' or not exists. Check your mission definition file, or notify the developer if you think it is a bug." );
    return result;
  }

  if (Utils::readBoolAttrib(item_node, mxconst::get_ATTRIB_MANDATORY(), false))
  {
    const std::string        sAllowedInventories   = Utils::readAttrib(item_node, mxconst::get_ATTRIB_TARGET_INVENTORY(), "");
    std::vector<std::string> vecInventories = mxUtils::split_v2(sAllowedInventories, ",");

    // check if we have at least one valid target inventory and if the external inventory name is in it.
    // This means that if the designer did not define a "target inventory", all inventories are valid.
    if ( (!vecInventories.empty()) && !mxUtils::isStringExistsInVec(vecInventories, target_inventory_name))
    {
      result.result = false;
      result.addErrMsg(fmt::format(R"(Target inventory "{}" is not valid for the item: "{}".)", target_inventory_name, item_name));
      return result;
    }

  } // end if item is mandatory

  return result; // should be always true
}


// -----------------------------------


std::string
data_manager::getTitleOrNameFromNode(mx_base_node& inBaseNode)
{

  if (inBaseNode.node.isEmpty())
    return "";
  else
  {
    std::string result_s = Utils::readAttrib(inBaseNode.node, mxconst::get_ATTRIB_TITLE(), ""); // get title attribute
    if (result_s.empty())
      return mxUtils::trim(inBaseNode.name);
    else
      return mxUtils::trim(result_s);
  }

  return "";
}

// -------------------------------------

void
data_manager::clear_all_datarefs_from_interpolation_map()
{
  mapInterpolDatarefs.clear();

#ifndef RELEASE
  Log::logMsg("[" + std::string(__func__) + "] Cleared all datarefs.");
#endif // !RELEASE
}

// -------------------------------------

void
data_manager::remove_dataref_from_interpolation_map(const std::string& inDatarefs)
{
  std::vector<std::string> vecDrefNames = mxUtils::split_v2(inDatarefs, ",");
  for (const auto& drefString : vecDrefNames)
  {
    if (mxUtils::isElementExists(mapInterpolDatarefs, drefString))
      mapInterpolDatarefs.erase(drefString);

#ifndef RELEASE
    Log::logMsg("[" + std::string(__func__) + "] Releasing interpolation dataref: " + drefString);
#endif // !RELEASE
  }
}

// -------------------------------------


void
data_manager::do_datarefs_interpolation(const int inSeconds, const int inCycles, const std::string& inDatarefs)
{
  std::vector<std::string> vecDrefsWithValues = mxUtils::split_v2(inDatarefs, "|");

  for (const auto& dref_w_val : vecDrefsWithValues)
  {
    std::vector<std::string> vecDrefAndVal = mxUtils::split_v2(dref_w_val, "="); // we should get "dref name" and "value"
    if (vecDrefAndVal.size() > static_cast<size_t> ( 1 ) )                                        // if we have at least 2 values in vector, we pick only the first 2
    {
      std::string name    = vecDrefAndVal.at(0);
      std::string value_s = vecDrefAndVal.at(1);
      if (!value_s.empty())
      {
        const std::string       key = Utils::replaceChar1WithChar2_v2(name, '.', "/");
        dataref_param dref(key);

        // if dataref is valid and is not in the interpolation container
        if (dref.flag_paramReadyToBeUsed && (!mxUtils::isElementExists(mapInterpolDatarefs, key)))
        {
          dref.initInterpolation(inSeconds, inCycles, value_s);

          Utils::addElementToMap(mapInterpolDatarefs, key, dref);
          if (dref.strctInterData.flagIsValid)
          {
#ifndef RELEASE
            Log::logMsg("[" + std::string(__func__) + "] Added interpolation dataref: " + key);
#endif // !RELEASE
          }
          else
            Log::logMsg(fmt::format("[{}] INACTIVE interpolation dataref: {}, delta value might be very small: {:.3f}", __func__, key, dref.strctInterData.delta_scalar));
        }

        // apply_dataref_based_on_key_value(key, value_s);
      } // end if attribute value is not empty
    }
  }
}

// -------------------------------------

void
data_manager::flc_datarefs_interpolation()
{

  // Calculating time from seconds: https://www.programmingnotes.org/2062/cpp-convert-time-from-seconds-into-hours-min-sec-format/
  // secs to HH:MM:SS format using division
  // and modulus
  // hour             = time / 3600;
  // time             = time % 3600;
  // min              = time / 60;
  // time             = time % 60;
  // sec              = time;

  std::deque<std::string> listOfDrefThatCanBeRemoved;

  bool bEndInterpolation{ false }; // v3.305.3
  for (auto& [key, dref] : mapInterpolDatarefs)
  {
    bool local_bInterpolated{ false };

    if (dref.flc_interpolation(local_bInterpolated))
      listOfDrefThatCanBeRemoved.emplace_back(key);

    if (local_bInterpolated)
      bEndInterpolation = true; // only store if we interpolated
  }

  if (bEndInterpolation) // v3.305.3
  {
    const auto iTimeInSeconds = static_cast<int> ( dataref_manager::getLocalTimeSec () );
    const auto iDay           = dataref_manager::getLocalDateDays();
    const auto iTime          = (iTimeInSeconds % 3600);
    const auto iHour          = dataref_manager::getLocalHour();
    const auto iMin           = (iTime / 60);
    const auto iSec           = (iMin > 0) ? (iTime % 60) : 0;
    Log::logMsg("Interpolation Ended [Day:HH:min:ss]: " + mxUtils::formatNumber<int>(iDay) + " " + mxUtils::formatNumber<int>(iHour) + ":" + mxUtils::formatNumber<int>(iMin) + ":" + mxUtils::formatNumber<int>(iSec));
  }

  // remove datarefs from the interpolation map container
  while (!listOfDrefThatCanBeRemoved.empty())
  {
#ifndef RELEASE
    Log::logMsg("[" + std::string(__func__) + "] Releasing interpolation dataref: " + listOfDrefThatCanBeRemoved.front());
#endif // !RELEASE

    mapInterpolDatarefs.erase(listOfDrefThatCanBeRemoved.front());

    listOfDrefThatCanBeRemoved.pop_front();
  }
}

std::string const
data_manager::getMissionSubcategoryTranslationCode(int in_missionCodeType, int in_subCategoryCode)
{
  // assert(missionx::data_manager::mapMissionCategoriesCodes.at(in_missionCodeType) != nullptr &&  "No Mission Code was Found.");
  // assert(missionx::data_manager::mapMissionCategoriesCodes.at(in_missionCodeType).at(in_subCategoryCode) != nullptr && ": No Sub code was Found.");

  mx_mission_subcategory_type code = mapMissionCategoriesCodes.at(in_missionCodeType).at(in_subCategoryCode); // lmbda_get_category_code(in_missionCodeType, in_subCategoryCode);

  switch (code)
  {
    case (mx_mission_subcategory_type::med_any_location):
    case (mx_mission_subcategory_type::med_accident):
    case (mx_mission_subcategory_type::med_outdoors):
      return mxconst::get_GENERATE_TYPE_MEDEVAC();
      break;
    case (mx_mission_subcategory_type::cargo_ga):
    case (mx_mission_subcategory_type::cargo_farming):
    case (mx_mission_subcategory_type::cargo_isolated):
    case (mx_mission_subcategory_type::cargo_heavy):
      return mxconst::get_GENERATE_TYPE_CARGO();
      break;
    case (mx_mission_subcategory_type::oilrig_cargo):
      return mxconst::get_GENERATE_TYPE_OILRIG_CARGO();
      break;
    case (mx_mission_subcategory_type::oilrig_med):
      return mxconst::get_GENERATE_TYPE_OILRIG_MED();
      break;
    case (mx_mission_subcategory_type::oilrig_personnel):
      return mxconst::get_GENERATE_TYPE_OILRIG_CARGO();
      break;
    default:
      break;

  } // end switch



  return "";
}

// -------------------------------------

void
data_manager::addMessageToHistoryList(std::string inWho, std::string inText, mxVec4 inColor)
{

  if (listOfMessageStoryMessages.size() > MAX_MX_PAD_MESSAGES)
    listOfMessageStoryMessages.pop_front();

  // const ImVec4 color(inColor.x, inColor.y, inColor.z, inColor.w);
  listOfMessageStoryMessages.emplace_back(messageLine_strct(inWho, inText, inColor));
}


// -------------------------------------


bool
data_manager::find_and_read_template_file (const std::string &inFileName)
{
  std::string       err;
  const std::string inPath = mx_folders_properties.getAttribStringValue (mxconst::get_FLD_RANDOM_TEMPLATES_PATH(), "", err); // get path to template folder
  // const std::string custom_mission_folder = data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONS_ROOT_PATH(), "", err);     // get path to custom missionx folder "/Custom Scenery/missionx"

  if (fs::directory_entry (inPath).is_directory ()) // if parentDir fail initialization
  {
    std::map<std::string, std::string> mapListFiles;
    if (ListDir::getListOfFiles (inPath.c_str (), mapListFiles, ".xml"))
    {
      // check if inFileName is in the mapListFiles container
      if (mxUtils::isElementExists (mapListFiles, inFileName))
      {
        std::filesystem::path pth            = mapListFiles.at (inFileName);
        const std::string     templatePath_s = pth.remove_filename ().string ();

        err.clear ();
        err = ListDir::read_template_file (templatePath_s, inFileName, mapGenerateMissionTemplateFiles[inFileName], false);
        if (!err.empty ()) // fail if the error text is not empty
          return false;
      }
    }
  }

  // check and return if inFileName is in the mapGenerateMissionTemplateFiles container
  return mxUtils::isElementExists (mapGenerateMissionTemplateFiles, inFileName);
}
void
data_manager::flc_acf_change ()
{
  char outFileName[512]{ 0 };
  char outPathAndFile[2048]{ 0 };
  XPLMGetNthAircraftModel (XPLM_USER_AIRCRAFT, outFileName, outPathAndFile); // we will only return the file name

  if (fmt::format ("{}", outFileName) != data_manager::active_acf)
    data_manager::set_active_acf_and_gather_info (outFileName);
}

  // -------------------------------------

void
data_manager::set_acf (const std::string &inFileName)
{
  data_manager::prev_acf = data_manager::active_acf;
  data_manager::active_acf = inFileName;
}

// -------------------------------------

void
data_manager::set_active_acf_and_gather_info (const std::string& inFileName)
{

    #ifndef RELEASE
    Log::logMsg (fmt::format("Gathering ACF: {} Information.", inFileName) );
    #endif
    data_manager::set_acf (inFileName);

    missionx::Inventory::gather_acf_cargo_data(data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()], true);
    data_manager::dref_acf_station_max_kgs_f_arr.setAndInitializeKey("sim/aircraft/weight/acf_m_station_max");
    missionx::dataref_param::set_dataref_values_into_xplane(data_manager::dref_m_stations_kgs_f_arr ); // force original weight on the new plane

    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::gather_acf_custom_datarefs); // v3.303.13 make sure we will have the plane dataref information

}

// -------------------------------------


void
data_manager::apply_datarefs_based_on_string_parsing(const std::string& inDatarefs, const std::string& inBetweenDatarefsDelimiter, const std::string& inEqualSignChar)
{
  std::vector<std::string> vecDrefsWithValues = mxUtils::split_v2(inDatarefs, inBetweenDatarefsDelimiter);

  for (const auto& dref_w_val : vecDrefsWithValues)
  {
    std::vector<std::string> vecDrefAndVal = mxUtils::split_v2(dref_w_val, inEqualSignChar); // we should get "dref name" and "value"
    if (vecDrefAndVal.size() > static_cast<size_t> ( 1 ) )                                                    // if we have at least 2 values in vector, we pick only the first 2
    {
      std::string name    = vecDrefAndVal.at(0);
      std::string value_s = vecDrefAndVal.at(1);
      if (!value_s.empty())
      {
        const std::string key = mxUtils::trim(Utils::replaceChar1WithChar2_v2(name, '.', "/")); // v3.305.1 added trim ti the  "key" parameter dew to the fact that it can have: "\n" in the key name.
        apply_dataref_based_on_key_value_strings(key, value_s);
      } // end if attribute value is not empty
    }
  }

  // end apply_datarefs_based_on_string_parsing
}

// -------------------------------------

void
data_manager::apply_datarefs_from_text_based_on_parent_node_and_tag_name(IXMLNode& inParentNode, const std::string& inTagName)
{
  if (!inParentNode.isEmpty())
  {
    const int nCounter = (inTagName.empty()) ? 1 : inParentNode.nChildNode(inTagName.c_str());
    for (int i1 = 0; i1 < nCounter; ++i1)
    {
      // get node pointer based on the tag name searched or use the parent node as the attribute holder instead of the child nodes
      auto node = (!inTagName.empty()) ? inParentNode.getChildNode(inTagName.c_str(), i1) : inParentNode;

      if (!node.isEmpty())
      {
        // read all attributes and try to create a temporary dataref that will hold the custom value so we can assign it
        const std::string sElementText = Utils::xml_get_text(node);
        if (!sElementText.empty())
          apply_datarefs_based_on_string_parsing(sElementText);

      } // end if node is valid
    }   // end for loop

  } // end if Parent node is valid
}

// -------------------------------------

void
data_manager::set_success_or_reset_tasks_state(const std::string& inObjeName, const std::string& inTaskList, const enums::mx_action_from_trigger_enum& in_action, const std::string& inCurrentTask)
{
  const auto vecActions = Utils::split_v2(inTaskList);

  for (const auto& task_name : vecActions)
  {
      auto task_ptr = (mxUtils::isElementExists(mapObjectives[inObjeName].mapTasks, task_name)) ?
                        &mapObjectives[inObjeName].mapTasks[task_name]
                       : nullptr;

      if (task_ptr == nullptr)
        continue;

      if (task_ptr->getName() == inCurrentTask)
        continue; // skip same name tasks

      switch (in_action)
      {
        case enums::mx_action_from_trigger_enum::set_success:
      {
        task_ptr->set_success();
        // v25.03.1 set the calling task with the "other_tasks" attribute
        Utils::xml_set_attribute_in_node_asString ( mapObjectives[inObjeName].mapTasks[inCurrentTask].node, mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS(), inTaskList, mxconst::get_ELEMENT_TASK());
        #ifndef RELEASE
          Utils::xml_print_node ( task_ptr->node );
          Utils::xml_print_node ( mapObjectives[inObjeName].mapTasks[inCurrentTask].node );
        #endif
      }
      break;
      case enums::mx_action_from_trigger_enum::reset:
      {
        task_ptr->reset();
      }
      break;
    } // end switch

  } // end loop over tasks

}

// -------------------------------------

void
data_manager::set_trigger_state(const Trigger& inCallingTrig, const std::string& inTrigList, const enums::mx_action_from_trigger_enum& in_action)
{
  const auto vecTriggerNames = Utils::split_v2(inTrigList);

  for (const auto& trig_name : vecTriggerNames)
  {
    // check if trigger exists and if we do not use the calling trigger name
    if (mxUtils::isElementExists( mapTriggers, trig_name ) * (inCallingTrig.name != trig_name) )
    {
      switch (in_action)
      {
        case enums::mx_action_from_trigger_enum::set_success:
        {
          mapTriggers[trig_name].setAllConditionsAreMet_flag( true );
          mapTriggers[trig_name].setTrigState(mx_trigger_state_enum::never_trigger_again);
        }
        break;
        case enums::mx_action_from_trigger_enum::reset:
        {
          mapTriggers[trig_name].setAllConditionsAreMet_flag(false);
          mapTriggers[trig_name].setTrigState(mx_trigger_state_enum::wait_to_be_triggered_again);
        }
        break;
      } // end switch
    } // end if trigger exists in container

  } // end loop over tasks
}

// -------------------------------------

std::string
data_manager::get_weather_state(const int& inCode)
{
  constexpr const static size_t MAX_LENGTH_BEFORE_BREAKING_LINE = (size_t)500;

  std::string sWeatherDatarefAsText;
  std::string last_line_before_break_s;

  std::map<int, std::unordered_map<std::string, std::string>>* mapWeather_ptr = (xplane_ver_i >= XP12_VERSION_NO) ? &data_manager::mapWeatherPreDefinedStrct_xp12 : &data_manager::mapWeatherPreDefinedStrct_xp11;
  assert(mapWeather_ptr != nullptr && "weather pointer can't be null"); // debug

  // Pick the correct code base on "inCode" value
  std::unordered_map<std::string, std::string>* weather_ptr = (inCode == -1) ? &(*mapWeather_ptr)[0] : ((Utils::isElementExists((*mapWeather_ptr), inCode) ? &(*mapWeather_ptr)[inCode] : nullptr));

  sWeatherDatarefAsText.clear();
  last_line_before_break_s.clear();

  if (weather_ptr)
  {
    for (const auto& [key_dref, value_s] : (*weather_ptr))
    {
      // skip change_mode dataref so we won't store "real weather"
      if (xplane_ver_i >= XP12_VERSION_NO && key_dref.find("change_mode") != std::string::npos)
        continue;


      const auto lamda_pick_dref_value_base_on_choice = [](const int inCode, const std::string& defaultValue, const std::string& inKeyDref) -> std::string
      {
        if (inCode >= 0)
          return defaultValue;

        dataref_param dref(inKeyDref); // init the dataref
        return dref.get_dataref_scalar_value_as_string();
      };



      const std::string newDrefLine_s = key_dref + "=" + lamda_pick_dref_value_base_on_choice(inCode, value_s, key_dref) + "|";

      if (std::string_view(last_line_before_break_s + newDrefLine_s).length() > MAX_LENGTH_BEFORE_BREAKING_LINE)
      {
        sWeatherDatarefAsText += "\n";
        last_line_before_break_s = newDrefLine_s; // reset with the new line
      }
      else
        last_line_before_break_s += newDrefLine_s; // append

      sWeatherDatarefAsText += newDrefLine_s;
    }
  }

  return sWeatherDatarefAsText;
}

// -------------------------------------

bool
data_manager::addAndLoadTextureMapNodeToLeg(const std::string& inFileName, const std::string& inLegName)
{
  if (inLegName.empty())
    return false;

  if (mxUtils::isElementExists(mapFlightLegs, inLegName))
  {
    IXMLNode xNewMap_ptr = Utils::xml_get_or_create_node_ptr(mapFlightLegs[inLegName].node, mxconst::get_ELEMENT_MAP(), mxconst::get_ATTRIB_MAP_FILE_NAME(), inFileName);
    if (!xNewMap_ptr.isEmpty())
    {
      if (loadImage(inFileName, mapCurrentMissionTextures))
      {
        Utils::addElementToMap(mapFlightLegs[inLegName].map2DMapsNodes, inFileName, xNewMap_ptr.deepCopy());

        if (inLegName.compare(currentLegName) == 0) // add to current Leg only if this is what was asked
        {
          strct_flight_leg_info_totalMapsCounter += 1;
          Utils::addElementToMap(maps2d_to_display, strct_flight_leg_info_totalMapsCounter, mapCurrentMissionTextures[inFileName]);
        }

        Log::logMsgThread(std::string(__func__) + "] leg: " + inLegName + ", Loaded Texture: " + inFileName); // debug

        return true;
      }
    } // end adding map node and loading texture

  } // end if Leg Element exists

  return false;
}

// -------------------------------------

bool
data_manager::loadImage(const std::string& inFileName, std::map<std::string, mxTextureFile>& in_outTextureMapToStore)
{
  if (inFileName.empty())
    return false;


  const std::string       fldMissionCustom_withSep = mapBrieferMissionList[selectedMissionKey].pathToMissionPackFolderInCustomScenery + XPLMGetDirectorySeparator();
  std::string             errorMsg; // v24.06.1
  mxTextureFile texture;

  texture.setTextureFile(inFileName, fldMissionCustom_withSep);
  if (BitmapReader::loadGLTexture(texture, errorMsg, false)) // load image but do not flip it
  {
    Utils::addElementToMap(in_outTextureMapToStore, inFileName, texture); // store texture data in map

    return true; // success
  }
  else
    Log::logMsgThread(fmt::format("[{}] failed to load Texture: {}{}\nError: ", __func__, fldMissionCustom_withSep, inFileName, errorMsg)); // debug

  return false;
}


// -------------------------------------


Point
data_manager::write_plane_position_to_log_file()
{
  Point p = dataref_manager::getCurrentPlanePointLocation();

  XPLMProbeResult outResult;
  const double    groundElev_ft = Point::getTerrainElevInMeter_FromPoint(p, outResult) * meter2feet;

  // std::string formatedOutput = "PLANE: <point lat=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getLat(), 8) + mxconst::get_QM() + " long=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getLon(), 8)
  //                              + mxconst::get_QM() + " elev_ft=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getElevationInFeet(), 2) + mxconst::get_QM() +
  //                              " \ttemplate=\"\"  loc_desc=\"\" type=\"\"   heading_psi="
  //                              + mxconst::get_QM() + Utils::formatNumber<double>(p.getHeading(), 2) + mxconst::get_QM() + " groundElev=" + mxconst::get_QM() + Utils::formatNumber<double>(groundElev_ft)
  //                              + mxconst::get_QM() + " ft replace_lat=\"" + Utils::formatNumber<double>(p.getLat(), 8) + "\"" + " replace_long=\""
  //                              + Utils::formatNumber<double>(p.getLon(), 8) + "\" />";

  // constexpr const static std::string sFormat = R"(PLANE: <point lat="{:.8f}" long="{:.8f}" elev_ft="{:.2f}" template=""  loc_desc="" type="" heading_psi="{:.2f}" groundElev="{:.2f}" ft replace_lat="{:.8f}" replace_long="{:.8f}" />)";
  constexpr const static char* sFormat = R"(PLANE: <point lat="{:.8f}" long="{:.8f}" elev_ft="{:.2f}" template=""  loc_desc="" type="" heading_psi="{:.2f}" groundElev="{:.2f}" ft replace_lat="{:.8f}" replace_long="{:.8f}" />)";

  const std::string formatedOutput = fmt::format(sFormat, p.getLat(), p.getLon(), p.getElevationInFeet(), p.getHeading(), groundElev_ft, p.getLat(), p.getLon());

  Log::logMsgNone(formatedOutput);

  return p;
}

// -------------------------------------

Point
data_manager::write_camera_position_to_log_file()
{
  XPLMDataRef gHeadingPsiRef = XPLMFindDataRef("sim/graphics/view/view_heading");

  double gHeading, groundElev_ft; // holds plane position
  double x, y, z;
  double outX, outY, outZ;
  x = y = z = 0.0;
  outX = outY = outZ = 0.0;

  x = XPLMGetDataf(drefConst.dref_camera_view_x_f);
  y = XPLMGetDataf(drefConst.dref_camera_view_y_f);
  z = XPLMGetDataf(drefConst.dref_camera_view_z_f);

  XPLMLocalToWorld(x, y, z, &outX, &outY, &outZ);

  outZ *= meter2feet; // v24.03.2 fix elevation to be feet and not meter
  Point           p = Point(outX, outY, outZ);
  XPLMProbeResult outResult;
  groundElev_ft = Point::getTerrainElevInMeter_FromPoint(p, outResult) * meter2feet; //   getTerrainElevFromPoint(gLat, gLon, gElev, &outResult);

  // read longitude/latitude info
  gHeading = XPLMGetDataf(gHeadingPsiRef);
  p.setHeading(gHeading);


  // const std::string formatedOutput = "CAMERA VIEW: <point lat=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getLat(), 8) + mxconst::get_QM() + " long=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getLon(), 8) + mxconst::get_QM() + " elev_ft=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getElevationInFeet(), 2) + mxconst::get_QM() +
  //                              " \ttemplate=\"\"  loc_desc=\"\" type=\"\"    heading_psi=" + mxconst::get_QM() + Utils::formatNumber<double>(p.getHeading(), 2) + mxconst::get_QM() + " groundElev=" + mxconst::get_QM() + Utils::formatNumber<double>(groundElev_ft) + mxconst::get_QM() + " ft  replace_lat=\"" + Utils::formatNumber<double>(p.getLat(), 8) + "\"" + " replace_long=\"" +
  //                              Utils::formatNumber<double>(p.getLon(), 8) + "\" />";
  constexpr const static char* sFormat        = R"(CAMERA VIEW: <point lat="{:.8f}" long="{:.8f}" elev_ft="{:.2f}" template=""  loc_desc="" type="" heading_psi="{:.2f}" groundElev="{:.2f}" replace_lat="{:.8f}" replace_long="{:.8f}" />)";
  const std::string            formatedOutput = fmt::format(sFormat, p.getLat(), p.getLon(), p.getElevationInFeet(), p.getHeading(), groundElev_ft, p.getLat(), p.getLon());
  Log::logMsgNone(formatedOutput);

  return p;
}

// -------------------------------------

std::string
data_manager::write_weather_state_to_log_file()
{

  std::string sWeather = get_weather_state();
  Log::logMsg("\nfn_set_datarefs(\" " + sWeather + " \") \n"); // dump data to missionx.log file
  return sWeather;
}

// -------------------------------------

void
data_manager::apply_dataref_based_on_key_value_strings(const std::string& inKey, std::string& inValue)
{

  dataref_param dref(inKey);
  if (dref.flag_paramReadyToBeUsed)
  {
    switch (dref.getDataRefType())
    {
      case xplmType_Int:
      {
        dref.setValue(mxUtils::stringToNumber<double>(inValue));
        dataref_param::set_dataref_values_into_xplane(dref);
      }
      break;
      case xplmType_Float:
      case (xplmType_Double):
      case (xplmType_Float | xplmType_Double):
      {
        dref.setValue(mxUtils::stringToNumber<double>(inValue, inValue.length()));
        dataref_param::set_dataref_values_into_xplane(dref);
      }
      break;
      case (xplmType_IntArray):
      {
        dref.readArrayDatarefIntoLocalArray();                                  // v3.303.13 added, since it was missing in IntArray but not in floatArray
        int estimatedArraySize_i = Utils::countCharsInString(inValue, ',') + 1; // we add 1 since we believe the designer wrote a valid format, like: "1,2.5,0" and not ",,," or "1,2"
        if (dref.setTargetArray<xplmType_IntArray, int>(estimatedArraySize_i, inValue, false, ","))
          dataref_param::set_dataref_values_into_xplane(dref);
      }
      break;
      case (xplmType_FloatArray):
      {
        dref.readArrayDatarefIntoLocalArray();
        int estimatedArraySize_i = Utils::countCharsInString(inValue, ',') + 1; // we add 1 since we believe the designer wrote a valid format, like: "1,2.5,0" and not ",,," or "1,2"
        if (dref.setTargetArray<xplmType_FloatArray, float>(estimatedArraySize_i, inValue, false, ","))
          dataref_param::set_dataref_values_into_xplane(dref);
      }
      break;
    } // end switch
  }   // end if dref is valid

} // apply_datarefs_based_on_string

// -------------------------------------

// int
// missionx::data_manager::get_inv_layout_based_on_mission_ver_and_compatibility_node ()
// {
//   // int layout_version = 0; // return value
//   //
//   // // Are we in X-Plane 11 ? if so, we have to force it and ignore other settings.
//   // if ( missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO )
//   //   layout_version = XP11_COMPATIBILITY;
//   // else
//   // {
//   //   // Step 1: test Mission file version. If mission file version supports "301" we will set the compatibility to XP11.
//   //   if ( std::ranges::any_of ( missionx::data_manager::vecCurrentMissionFileSupportedVersions, [&] ( auto &elem ) { return elem == missionx::PLUGIN_FILE_VER_XP11; } ) )
//   //     layout_version = missionx::XP11_COMPATIBILITY;
//   //
//   //   // Step 2: Did designer allow the "newer" inventory, if simmer is using X-Plane 12 ?
//   //   layout_version = Utils::readNodeNumericAttrib<int> ( missionx::data_manager::mx_global_settings.xCompatibility_ptr, mxconst::get_ATTRIB_INVENTORY_LAYOUT(), layout_version );
//   //
//   //   // Step 3: If inventory layout is free to choose, then pick the "setup" compatibility value.
//   //   if ( layout_version != missionx::XP11_COMPATIBILITY )
//   //     layout_version = ( missionx::data_manager::flag_setupUseXP11InventoryUI ) ? missionx::XP11_COMPATIBILITY : 0;
//   // }
//
//   return get_inv_layout_based_on_mission_ver_and_compatibility_node(missionx::data_manager::mission_format_versions_s, missionx::data_manager::mx_global_settings.xCompatibility_ptr, missionx::data_manager::flag_setupUseXP11InventoryUI);
// }

  // -------------------------------------


int
data_manager::get_inv_layout_based_on_mission_ver_and_compatibility_node ( const std::string &in_mission_format_versions, const IXMLNode &in_compatibility_node, const bool &in_flag_setupUseXP11InventoryUI )
{
    auto local_vecCurrentMissionFileSupportedVersions = Utils::split(in_mission_format_versions, ',');
    int layout_version = 0;

    // Are we in X-Plane 11 ? if so, we have to force it and ignore other settings.
    if ( xplane_ver_i < XP12_VERSION_NO )
      layout_version = XP11_COMPATIBILITY;
    else
    {
      // Step 1: test Mission file version. If mission file version supports "301" we will set the compatibility to XP11.
      if ( std::ranges::any_of ( local_vecCurrentMissionFileSupportedVersions, [&] ( auto &elem ) { return elem == PLUGIN_FILE_VER_XP11; } ) )
        layout_version = XP11_COMPATIBILITY;

      // Step 2: Did designer allow the "newer" inventory, if simmer is using X-Plane 12 ?
      layout_version = Utils::readNodeNumericAttrib<int> ( in_compatibility_node, mxconst::get_ATTRIB_INVENTORY_LAYOUT(), layout_version );

      // Step 3: If inventory layout is free to choose, then pick the "setup" compatibility value.
      if ( layout_version != XP11_COMPATIBILITY )
        layout_version = ( in_flag_setupUseXP11InventoryUI ) ? XP11_COMPATIBILITY : 0;
    }

    return layout_version;
}

// -------------------------------------


void
data_manager::position_camera(XPLMCameraPosition_t& inCameraPos, float inLat, float inLon)
{
  if ((inLat * inLon) == 0.0f)
  {
    Log::logMsg("[position_camera] Wrong camera position values.");
  }
  else
  {

    mxCameraPosition  = inCameraPos;
    isLosingControl_i = 1;                                             // plugin control
    queFlcActions.push(mx_flc_pre_command::position_camera); // Position camera
  }
} // position_camera

// -------------------------------------

void
data_manager::set_camera_poistion_loc_rule(int inIsLosingControl_i)
{
  if (inIsLosingControl_i != 0 && inIsLosingControl_i != 1)
    inIsLosingControl_i = 0;

  isLosingControl_i = inIsLosingControl_i;
} // position_camera



// -----------------------------------
//
// -----------------------------------
// TimeLapse CLASS
// -----------------------------------



std::string
TimeLapse::set_date_and_hours(const std::string& inDateAndTime, const bool inIgnorePauseMode)
{
  this->flag_ignorePauseMode = inIgnorePauseMode; // v3.305.1

  std::map<int, std::string> mapTime; // 0 = day in year, 1 = hours, 2 = min
  mapTime = Utils::splitStringToMap(inDateAndTime, mxconst::get_COLON());

  int         dayOfYear = -1;
  int         hours     = -1;
  int         minutes   = -1;
  std::string val       = "";

  this->start_local_date_days_i = XPLMGetDatai(dc.dref_local_date_days_i);
  this->local_time_sec_f        = XPLMGetDataf(dc.dref_local_time_sec_f);
  this->zulu_time_sec_f         = XPLMGetDataf(dc.dref_zulu_time_sec_f);

  // store day in year
  if (Utils::isElementExists(mapTime, 0) && !mapTime[0].empty() && Utils::is_number(mapTime[0]))
  {
    dayOfYear = Utils::stringToNumber<int>(mapTime[0]);
    if (dayOfYear >= 0 && dayOfYear <= 364)
      futureDayOfYearAfterTimeLapse = dayOfYear;
    else
      futureDayOfYearAfterTimeLapse = -1;
  }

  // set timelaps with new time and cycle 1
  if (Utils::isElementExists(mapTime, 1) && !mapTime[1].empty() && Utils::is_number(mapTime[1]))
  {
    hours = Utils::stringToNumber<int>(mapTime[1]);
    if (Utils::isElementExists(mapTime, 2) && !mapTime[2].empty() && Utils::is_number(mapTime[2]))
      minutes = Utils::stringToNumber<int>(mapTime[2]);

    if (minutes < 0 || minutes > 59)
      minutes = 0;

    if (hours >= 0 && hours < 24)
    {
      const std::string err = this->timelapse_to_local_hour(hours, minutes, 1, true); // v3.303.8.2 added force immediate time change
      if (!err.empty())
      {
        Log::logDebugBO(err);
        return err;
      }
    }
  }


  return ""; // no errors
}

// -------------------------------------
std::string
TimeLapse::timelapse_add_minutes(const int inMinutesToAdd, const int inHowManyCycles, const bool inIgnorePauseMode)
{
  // get local day
  // get local time
  // get UTC time
  // Check if inMinutesToAdd > 1 and inSecondsToCycle between 1 and 24

  this->flag_ignorePauseMode = inIgnorePauseMode; // v3.305.1

  // do sanity tests
  if (inMinutesToAdd < 1)
    return ("Value is too small. Add more minutes.");

  if (inMinutesToAdd > 1440)
    return ("Value is to big. You should not add more than 24hours in minutes");

  if (inHowManyCycles < 1)
    this->cycles = 1;
  else if (inHowManyCycles > 24)
    this->cycles = 24;
  else
    this->cycles = inHowManyCycles;


  // read datarefs
  this->start_local_date_days_i = XPLMGetDatai(dc.dref_local_date_days_i);
  this->local_time_sec_f        = XPLMGetDataf(dc.dref_local_time_sec_f);
  this->zulu_time_sec_f         = XPLMGetDataf(dc.dref_zulu_time_sec_f);

  // calculate the future time. Make sure we do not pass next day and maybe even next year

  int seconds_to_add                            = inMinutesToAdd * 60; // convert minutes to seconds
  this->total_time_to_add_in_seconds            = static_cast<float> ( seconds_to_add );
  this->expected_time_after_addition_in_seconds = local_time_sec_f + total_time_to_add_in_seconds; // this can be greater than 86400, so we need to be careful

  this->seconds_per_lapse_f = (float)(this->total_time_to_add_in_seconds / this->cycles);


  Timer::start(this->timer, 1.0, "timelapse_add_minutes, cycle: " + Utils::formatNumber<int>(this->cycleCounter));
  this->total_passed  = 0.0f;
  this->flag_isActive = true;

  return EMPTY_STRING;
}


// -------------------------------------

std::string
TimeLapse::timelapse_to_local_hour(int inFutureLocalHour, int inFutureMinutes, int inHowManyCycles, bool flag_inIgnoreActiveState, const bool inIgnorePauseMode) // v3.303.8.2 added flag_inIgnoreActiveState with default "false"
{
  // get local day
  // get local time
  // get UTC time
  // Check if inMinutesToAdd > 1 and inSecondsToCycle between 1 and 24

  this->flag_ignorePauseMode = inIgnorePauseMode; // v3.305.1

  std::string err;
  err.clear();

  // make sure timelapse is not active
  if (!flag_inIgnoreActiveState && data_manager::timelapse.flag_isActive)
    return "TimeLapsed is active. Can't interfere with its action. Aborting your request. Suggestion: Time it differently.";

  // do sanity tests
  if (inFutureLocalHour < 0 || inFutureLocalHour > 23)
    return ("Hours value needs to be between 0-23. Value received: " + Utils::formatNumber<int>(inFutureLocalHour));

  if (inFutureMinutes < 0 || inFutureMinutes > 59)
    return ("Minutes value needs to be between 0-59. Value received: " + Utils::formatNumber<int>(inFutureMinutes));


  // read datarefs
  this->start_local_date_days_i = XPLMGetDatai(dc.dref_local_date_days_i);
  this->local_time_sec_f        = XPLMGetDataf(dc.dref_local_time_sec_f);
  this->zulu_time_sec_f         = XPLMGetDataf(dc.dref_zulu_time_sec_f);


  if (inHowManyCycles < 0) // try to figure cycles for designer. We will use 24h/(24h - inFutureLocalHour)
  {
    // figure delta hours
    int local_time_in_hours = static_cast<int> ( local_time_sec_f / SECONDS_IN_1HOUR_3600 );
    int delta_hours         = 0;
    if (inFutureLocalHour >= local_time_in_hours)
      delta_hours = inFutureLocalHour - local_time_in_hours;
    else
      delta_hours = (24 + inFutureLocalHour) - local_time_in_hours;

    // guess cycles
    inHowManyCycles = delta_hours;
  }

  if (inHowManyCycles < 1)
    inHowManyCycles = 1;

  if (inHowManyCycles > 24)
    inHowManyCycles = 24;

  // calculate the future time. Make sure we do not pass next day and maybe even next year

  float future_time_in_seconds = static_cast<float> ( ( inFutureLocalHour * 3600 ) + ( inFutureMinutes * 60 ) ); // convert hours and minutes to seconds from midnight. This represents the real hour in x-plane but in seconds and for local time. We need to find the delta for UTC.
  float seconds_to_add         = 0.0f;

  // if future_time_in_seconds > local_time_sec_f then we are on same day
  if (future_time_in_seconds >= local_time_sec_f)
    seconds_to_add = future_time_in_seconds - local_time_sec_f;
  else if (future_time_in_seconds < local_time_sec_f) // we need to add 24 hours
    seconds_to_add = (future_time_in_seconds + SECONDS_IN_1DAY - local_time_sec_f);


  this->total_time_to_add_in_seconds            = seconds_to_add;
  this->expected_time_after_addition_in_seconds = local_time_sec_f + total_time_to_add_in_seconds; // this can be greater than 86400, so we need to be careful
  this->cycles                                  = inHowManyCycles;

  this->seconds_per_lapse_f = (float)(this->total_time_to_add_in_seconds / this->cycles);


  Timer::start(this->timer, 1.0, "timelapse to local hour, cycles: " + Utils::formatNumber<int>(this->cycleCounter));
  this->total_passed  = 0.0f;
  this->flag_isActive = true;

  return err;
}

// -------------------------------------

// -------------------------------------

void
TimeLapse::flc_timelapse()
{
  // #ifdef TIMER_FUNC
  //   missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
  // #endif // TIMER_FUNC

  if (this->flag_isActive)
  {
    if (Timer::wasEnded(this->timer, true)) // v3.0.223.1 changed to OS time
    {
      ++this->cycleCounter;
      float seconds_to_add = this->seconds_per_lapse_f; //
      this->total_passed += this->seconds_per_lapse_f;
      // check if total_time is greater than time should pass. if so then just add the delta seconds.
      if (this->total_time_to_add_in_seconds < this->total_passed)
        seconds_to_add = this->total_passed - this->total_time_to_add_in_seconds;

      // we ignore the added seconds that passed during the timelap
      this->zulu_time_sec_f = XPLMGetDataf(dc.dref_zulu_time_sec_f);
      XPLMSetDataf(dc.dref_zulu_time_sec_f, ((float)(this->zulu_time_sec_f + seconds_to_add))); // add time

      // decide if to reset timer and end timelap
      if (this->total_time_to_add_in_seconds <= this->total_passed)
      {
        if (this->expected_time_after_addition_in_seconds >= SECONDS_IN_1DAY) // check if we passed local midnight
        {
          // make sure that during the timelapse date was not change already.
          // Change only if on same day
          int current_local_date_days_i = XPLMGetDatai(dc.dref_local_date_days_i);
          if (current_local_date_days_i == this->start_local_date_days_i)
          {
            int new_days_in_year = this->start_local_date_days_i + 1;
            if (new_days_in_year >= 365 || new_days_in_year < 0) // 364 is last day of the year. 0 is the first day of the year in X-Plane
              new_days_in_year = 0;

            XPLMSetDatai(dc.dref_local_date_days_i, (new_days_in_year));
          }
        }

        // set day of year if was set. Example in setDateAndHour. this will override any day set by this function
        if (this->futureDayOfYearAfterTimeLapse >= 0 && this->futureDayOfYearAfterTimeLapse < 364)
          XPLMSetDatai(dc.dref_local_date_days_i, this->futureDayOfYearAfterTimeLapse);

        this->reset(); // free timelap and ready to other
      }
      else
      {
        this->timer.reset();
        Timer::start(this->timer, 1.0, "timelapse_add_minutes, cycle: " + Utils::formatNumber<int>(this->cycleCounter));
        Log::logDebugBO("Timelapse Cycle:" + Utils::formatNumber<int>(this->cycleCounter));
      }

    } // end if timer passed (wasEnded)

    if (this->cycleCounter > 30)
    {
      Log::logMsgErr("[TimeLapse] Time laps has exceed its maximum cycles. Reseting timelap. Notify developer.");
      this->reset();
    }

  } // end if isActive

} // End TimeLapse::flc_timelapse()


// -------------------------------------

GLobalSettings::GLobalSettings()
{
  if (!this->node.isEmpty())
    this->node.updateName(mxconst::get_GLOBAL_SETTINGS().c_str());
}

// -------------------------------------

GLobalSettings::~GLobalSettings() {}

// -------------------------------------

bool
GLobalSettings::parse_node()
{
  assert(!this->node.isEmpty());

  this->xFolders_ptr        = this->node.getChildNode(mxconst::get_ELEMENT_FOLDERS().c_str());
  this->xStartTime_ptr      = this->node.getChildNode(mxconst::get_ELEMENT_START_TIME().c_str());
  this->xBaseWeight_ptr     = this->node.getChildNode(mxconst::get_ELEMENT_BASE_WEIGHTS_KG().c_str());
  this->xPosition_ptr       = this->node.getChildNode(mxconst::get_ELEMENT_POSITION().c_str());
  this->xDesigner_ptr       = this->node.getChildNode(mxconst::get_ELEMENT_DESIGNER().c_str());
  this->xTimer_ptr          = this->node.getChildNode(mxconst::get_ELEMENT_TIMER().c_str());
  this->xScoring_ptr        = this->node.getChildNode(mxconst::get_ELEMENT_SCORING().c_str());
  this->xWeather_ptr        = this->node.getChildNode(mxconst::get_ELEMENT_WEATHER().c_str());
  this->xSavedWeather_ptr   = this->node.getChildNode(mxconst::get_ELEMENT_SAVED_WEATHER().c_str()); // v3.303.13
  this->xCompatibility_ptr  = this->node.getChildNode(mxconst::get_ELEMENT_COMPATIBILITY().c_str()); // v24.12.2 example: inventory layout [11|12]


  return true;
}

// -------------------------------------

void
GLobalSettings::clear()
{
  if (!this->node.isEmpty())
  {
    this->initBaseNode();
    this->node.updateName(mxconst::get_GLOBAL_SETTINGS().c_str());

    this->xBaseWeight_ptr = this->xDesigner_ptr = this->xFolders_ptr = this->xPosition_ptr = this->xStartTime_ptr = xScoring_ptr = xWeather_ptr = xSavedWeather_ptr = IXMLNode::emptyIXMLNode;
  }
}
data_manager::data_manager()
{
    const auto lmbda_prepare_version_as_string = [] () -> std::string
    {
      std::stringstream ss;
      for (const auto& item: lsSupportedMissionFileVersions)
      {
        ss << "[" << item << "]";
      }
      return ss.str();
    };

    mission_file_supported_versions = lmbda_prepare_version_as_string();

}


// -------------------------------------
// -------------------------------------
// -------------------------------------
