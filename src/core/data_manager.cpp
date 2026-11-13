 #include <optional>
#include "../io/ListDir.h"
#include "xx_mission_constants.hpp"


#include <filesystem>

// #include "mx_img_window.h"
#include <nlohmann/json.hpp>

#include "data_manager.h"

#include <algorithm> // v26.10.2
#include <array>
#include <cctype>    // v26.10.2
#include <ctime>     // v26.10.2
#include <string_view> // v26.10.2
#include <cstdio>
// #include <forward_list> // v3.305.3
#include <fstream>
#include <iostream>
#include <list>
#include <unordered_set>

// #include "../../libs/freetype2.orig/src/gzip/zutil.h"
#include "../../libs/polyline-cpp/src/SimplePolyline.h" // v3.303.14
#include "../core/dataref_manager.h"                    // v3.305.2 moved from header
#include "../io/OptimizeAptDat.h"                       // v3.303.14
#include "../io/system_actions.h"                       // v3.0.241.1
#include "../ui/core/BitmapReader.h"                    // v3.303.14
#include "coordinate/NavAidInfo.hpp"
#include "../../libs/minizip/minizip/zip.h"
#include "../../libs/minizip/minizip/unzip.h"

namespace missionx
{
// missionx missionx_conf file node
IXMLNode data_manager::xMissionxProperties_node;

const std::string              data_manager::plugin_sig = "missionx_snagar.dev";
std::vector<std::future<void>> mImageLoadFutures; // we use future<void> since out function returns void.

std::filesystem::path missionx::data_manager::ca_path = fmt::format( "{}cacert.pem", Utils::getPluginDirectoryWithSep()); // v26.01.1

script_manager data_manager::sm;
#ifdef APL
mb_interpreter_t* data_manager::bas;
bool              data_manager::ext_sm_init_success;
bool              data_manager::ext_bas_open;
#endif

XPLMHostApplicationID data_manager::host_id; // v25.09.1
int                   data_manager::xplm_version; // v25.09.1
int                   data_manager::xplane_ver_i; // v3.0.241.5

int data_manager::xplane_using_modern_driver_b; // v3.0.241.5

int         data_manager::msgSeq;
std::string data_manager::selectedMissionKey; // v3.0.241.1

// initialize map
std::map<std::string, Waypoint>  data_manager::mapFlightLegs;
std::map<std::string, Objective> data_manager::mapObjectives;
std::map<std::string, Trigger>   data_manager::mapTriggers;
std::map<std::string, Inventory> data_manager::mapInventories; // v3.0.213.1
Inventory                        data_manager::planeInventoryCopy; // v24.12.2
IXMLNode                         data_manager::xmlBlueprints; // v3.0.241.1
Inventory                        data_manager::externalInventoryCopy; // v3.0.241.1 external inventory copy
std::string                      data_manager::active_external_inventory_name; // v3.0.251.1
bool                             data_manager::flag_inhibit_mxpad_when_entering_inventory_and_airborne; // v25.06.1

std::map<std::string, Message> data_manager::mapMessages;

// xml related
IXMLNode     data_manager::xMainNode;
IXMLRenderer data_manager::xmlRender;
IXMLNode     data_manager::xmlMappingNode;
IXMLNode     data_manager::xmlGPS;
IXMLNode     data_manager::xmlLoadedFMS;


// 3d objects
std::map<std::string, obj3d> data_manager::st_map3d_obj;                     // v3.0.200
std::map<std::string, obj3d> data_manager::map3dInstances;               // v3.0.200 //[name, obj3d instance] // Holds static and moving 3D Objects. We keep different list of 3D Objects in listDisplayStatic3dInstances and listDisplayMoving3dInstances
std::set<std::string>                  data_manager::listDisplayStatic3dInstances; // v3.0.200 //name, // Holds static 3D Objects
std::set<std::string>                  data_manager::listDisplayMoving3dInstances; // v3.0.202 //name, // Holds moving 3D Objects

std::map<std::string, dataref_param> data_manager::mapDref;

mx_base_node data_manager::mx_folders_properties;
std::string            data_manager::errStr;
std::list<std::string> data_manager::lstLoadErrors; // v3.305.3
mx_base_node data_manager::xLoadInfoNode; // v3.305.3



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
std::deque<mx_flc_pre_command> data_manager::queFlcActions;
std::deque<mx_flc_pre_command> data_manager::postFlcActions; // v3.0.146

std::map<std::string, mxTextureFile>    data_manager::mapCachedPluginTextures;                // v3.0.118 // plugin specific vecTextures
std::map<std::string, mxTextureFile>    data_manager::mapTemplatesTextures;                   // v26.09.3
std::map<std::string, TemplateFileInfo> data_manager::mapGenerateMissionTemplateFiles;        // Generate mission vecTextures // v3.0.217.2
std::map<int, std::string>              data_manager::mapGenerateMissionTemplateFilesLocator; // Generate mission vecTextures // v3.0.217.2
TemplateFileInfo                        data_manager::user_driven_template_info;              // v3.0.241.9 store user driven bare bone template to build on
int                                     data_manager::seqElementId{ 0 };

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
// bool data_manager::flag_generate_engine_is_running;

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

missionx::mx_stats_view_data data_manager::last_flight_stats_view; // v26.10.2 was: mx_mission_stats_strct mission_stats_from_query

uiLayer_enum data_manager::generate_from_layer;

std::string                                    data_manager::load_error_message;      // v3.0.251.1
int                                            data_manager::iMissionImageCounter;    // v3.0.251.1
std::map<std::string, mxTextureFile> data_manager::xp_mapMissionIconImages; // v3.0.251.1
std::map<std::string, mxTextureFile> data_manager::xp_mapInvImages;         // v3.0.303.5

//// External Flight plans
std::vector<mx_external_fpln_strct>     data_manager::tableExternalFPLN_simbrief_vec; // v25.03.3
std::vector<mx_external_fpln_strct>     data_manager::tableExternalFPLN_vec;
std::map<int, mx_external_fpln_strct *> data_manager::indexPointer_forExternalFPLN_tableVector;
std::vector<std::future<void>>              data_manager::mFetchFutures; //  holds async pointers
bool                                        data_manager::flag_is_stats_request_is_active{ false }; // v26.10.2
int                                         data_manager::in_flight_stats_reset_seq{ 0 };          // v26.10.2
std::future<void>                           data_manager::futStatsRoute; // v26.10.2 route job (stats_meta_build_route_thread_cai)
IXMLNode                                    data_manager::jsonConvertedNode_xml;

// ILS Query Filtering
std::vector<mx_ils_airport_row_strct>    data_manager::table_ILS_rows_vec;
std::map<int, mx_ils_airport_row_strct*> data_manager::indexPointer_for_ILS_rows_tableVector; // index pointer to the vector table_ILS_rows_vec

mutex data_manager::s_thread_sync_mutex;   // defined here and not in the class header
mutex data_manager::s_ImageLoadMutex;      // v3.303.14 Used in loadInventoryImages(), also solves non-POD when defined in namespace level
mutex data_manager::s_StoryImageLoadMutex; // v3.305.1 Used in loadStoryImages()
mutex data_manager::mt_SortTriggersMutex;  // v3.305.2 Used in sort
mutex data_manager::mt_request_get_curl_request_respond_mutex; // v26.06.4 Used in get_curl_request_respond()

int                      data_manager::curl_call_counter_i;                                      // v3.0.255.4.1
std::vector<std::string> data_manager::vecOverpassUrls;                                         // v3.0.255.4.1
int                      data_manager::overpass_user_picked_combo_i;                            // v3.0.255.4.1
int                      data_manager::overpass_last_url_indx_used_i{ mxconst::INT_UNDEFINED }; // v3.0.255.4.1

std::string data_manager::curl_result_s;
//std::string data_manager::error_message_line3; // v3.0.255.4

float data_manager::Max_Slope_To_Land_On{ mxconst::DEFAULT_MAX_SLOPE_TO_LAND_ON };

bool data_manager::flag_found_missing_3D_object_files{ false }; // v3.0.255.3 used in UI
bool data_manager::flag_rebuild_apt_dat{ false };               // v3.0.255.3 help decide if to rebuild the apt.dat. Basically calling: missionx::data_manager::queFlcActions.push_back(missionx::mx_flc_pre_command::exec_apt_dat_optimization)

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
#ifndef RELEASE
bool data_manager::flag_setupUseDraw2DMapInReleaseMode{ true }; // v25.10.1
#else
bool data_manager::flag_setupUseDraw2DMapInReleaseMode{ false }; // v25.10.1
#endif

int data_manager::ui_ifr_or_vfr_i = missionx::PICKED_IFR; // v25.08.1
int data_manager::ui_oilrig_globe_part_i = missionx::PICKED_LOCAL_REGION_GLOBE; // v25.08.1

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

// bool data_manager::flag_llm_use_llm_to_generate_a_mission{false}; // v26.09.1 // v26.09.2 part of: "strct_ui_share_data"

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

std::vector<int> data_manager::vecMissionCategoriesCodes = { static_cast<int> (mx_ui_mission_type::medevac), static_cast<int> (mx_ui_mission_type::cargo), static_cast<int> (mx_ui_mission_type::oil_rig) };

GatherStats                        data_manager::gather_stats;
mx_enrout_stats_strct              data_manager::strct_currentLegStats4UIDisplay; // v3.303.14 holds current leg stats
std::vector<mx_enrout_stats_strct> data_manager::vecPreviousLegStats4UIDisplay;   // v3.303.14

//// v3.305.1b
mx_sceneryOrPlaneLoad_state_strct data_manager::strct_mx_xplane_metadata;


// v3.305.2
std::list<messageLine_strct> data_manager::listOfMessageStoryMessages;
#ifdef USE_TRIGGER_OPTIMIZATION // v3.305.2
data_manager::mx_opt_leg_triggers_strct missionx::data_manager::optimize_leg_triggers_strct;
#endif // USE_TRIGGER_OPTIMIZATION // v3.305.2

// v3.305.4
XPLMMapLayerID data_manager::g_layer             = nullptr;
int            data_manager::s_num_cached_coords = 0;
float          data_manager::s_cached_x_coords[MAX_COORDS];
float          data_manager::s_cached_y_coords[MAX_COORDS];
float          data_manager::s_cached_lon_coords[MAX_COORDS];
float          data_manager::s_cached_lat_coords[MAX_COORDS];
float          data_manager::s_xp_map2d_icon_width = 0;

// v24.02.5
std::map<std::string, std::string> data_manager::mapQueries;

// v24.03.1
base_thread::strct_thread_state data_manager::threadStateMetar;

// v24.12.2
dataref_param data_manager::dref_acf_station_max_kgs_f_arr;
dataref_param data_manager::dref_m_stations_kgs_f_arr;
std::string data_manager::mission_file_supported_versions; // v24.12.2

// v26.09.2
missionx::structs::def_strct_acf_info data_manager::active_acf_info;

// v25.05.1
//missionx::data_manager::ui_shared_data_def_struct missionx::data_manager::strct_ui_share_data;
//std::string missionx::data_manager::ui_shared_data_def_struct::error_message_line3; // v26.04.4 when resides inside an inline static struct, there is no need for further declaraion.

// v25.09.1
missionx::NavAidInfo                missionx::data_manager::shared_navaid_between_threads;
std::string                         missionx::data_manager::post_optimization_outcome;
missionx::base_thread::strct_thread_state missionx::data_manager::fetch_metar_from_flcpre_thread_state;

// -------------------------------------

// v26.01.1
  std::list<std::string> missionx::data_manager::lst_of_failed_3d_obj_to_load;
  std::list<std::string> missionx::data_manager::lst_of_errors_and_warnings_during_mission_validation;

// v26.02.1
  missionx::structs::mx_clock_time_strct missionx::data_manager::shared_clock_time;

// -------------------------------------

size_t
my_write(void* buffer, const size_t size, size_t nmemb, void* param)
{
  std::string &text      = *static_cast<std::string *> ( param );
  const size_t       totalsize = size * nmemb;
  text.append(static_cast<char*>(buffer), totalsize);
  return totalsize;
}

// -------------------------------------


} // namespace missionx

// -------------------------------------

// missionx::data_manager::data_manager() {}

// -------------------------------------

// missionx::data_manager::~data_manager() {}


int
data_manager::callback_sqlite_data (void *data, int argc, char **argv, char **azColName)
{
  data_manager::row_gather_db_data.clear ();
  for (int i = 0; i < argc; i++)
    data_manager::row_gather_db_data[azColName[i]] = argv[i] ? argv[i] : "";

  data_manager::reasultTable[static_cast<int> (data_manager::reasultTable.size ())] = data_manager::row_gather_db_data;
  data_manager::row_gather_db_data.clear ();

  return 0;
}


// -------------------------------------

std::string missionx::data_manager::get_mission_outline_base(const std::unordered_map<std::string, std::string>& map_llm_requests_messages) 
{ 
  std::ostringstream oss;
  std::ranges::for_each(data_manager::strct_ui_share_data.map_llm_requests_messages,
                        [&oss](const auto& item)
                        {
                          // C++17 structured binding (or use item.first and item.second)
                          const auto& [key, value] = item;
                          if (key != llm_category::activity_number_picked) // skip this category
                            oss << key << ": " << value << "\n";
                        });

  // Add time data based on advanced settings
  const auto day_in_year_i   = data_manager::prop_userDefinedMission_ui.getAttribNumericValue<int>(mxconst::get_PROP_STARTING_DAY(), -1);
  const std::string starting_hour    = Utils::readAttrib(data_manager::prop_userDefinedMission_ui.node,   mxconst::get_PROP_STARTING_HOUR(), "");
  const std::string starting_minutes = Utils::readAttrib(data_manager::prop_userDefinedMission_ui.node,   mxconst::get_PROP_STARTING_MINUTE(), "");
  if (day_in_year_i >= 0)
    oss << "Day in Year: " << mxUtils::get_month_and_day (day_in_year_i) << "\n";
  if (!starting_hour.empty())
    oss << "Hour: " << starting_hour << "\n";
  if (!starting_minutes.empty())
    oss << "Minutes: " << starting_minutes << "\n";

 return oss.str(); 
}

// -------------------------------------

bool
data_manager::extract_missionx_library(const std::filesystem::path& zipPath, const std::filesystem::path& customSceneryDir)
{
#ifndef RELEASE 
Log::logMsg(fmt::format("[{}] Extracting missionx library:\n\tfrom: [{}]\n\tto: [{}].\n", __func__, zipPath.string(), customSceneryDir.string()));
#endif // !RELEASE 


  // 1. Cross-platform path handling for file opening
#if defined(_WIN32)
    //unzFile zip = unzOpen64(zipPath.wstring().c_str());
    unzFile zip = unzOpen64(zipPath.string().c_str());
#else
    unzFile zip = unzOpen64(zipPath.string().c_str());
#endif

    if (!zip) {
        return false;
    }

    unz_global_info64 globalInfo;
    if (unzGetGlobalInfo64(zip, &globalInfo) != UNZ_OK) {
        unzClose(zip);
        return false;
    }

    constexpr size_t BUFFER_SIZE = 8192;
    std::vector<char> buffer(BUFFER_SIZE);

    for (uLong i = 0; i < globalInfo.number_entry; ++i) {
        char filename[512];
        unz_file_info64 fileInfo;

        if (unzGetCurrentFileInfo64(zip, &fileInfo, filename, sizeof(filename), nullptr, 0, nullptr, 0) != UNZ_OK) {
            unzClose(zip);
            return false;
        }

        // Standardize POSIX forward slashes from zip header into target path
        fs::path targetPath = customSceneryDir / fs::path(filename).lexically_normal();

        // Check if entry is a directory
        if (filename[strlen(filename) - 1] == '/' || filename[strlen(filename) - 1] == '\\') {
            fs::create_directories(targetPath);
        } else {
            // Ensure parent directory exists
            fs::create_directories(targetPath.parent_path());

            if (unzOpenCurrentFile(zip) != UNZ_OK) {
                unzClose(zip);
                return false;
            }

            // std::ios::trunc forces overriding pre-existing scenery files
            std::ofstream outFile(targetPath, std::ios::binary | std::ios::trunc);
            if (!outFile.is_open()) {
                unzCloseCurrentFile(zip);
                unzClose(zip);
                return false;
            }

            int readBytes = 0;
            do {
                readBytes = unzReadCurrentFile(zip, buffer.data(), BUFFER_SIZE);
                if (readBytes < 0) {
                    unzCloseCurrentFile(zip);
                    unzClose(zip);
                    return false;
                }
                if (readBytes > 0) {
                    outFile.write(buffer.data(), readBytes);
                }
            } while (readBytes > 0);

            outFile.close();
            unzCloseCurrentFile(zip);
        }

        if (i < globalInfo.number_entry - 1) {
            if (unzGoToNextFile(zip) != UNZ_OK) {
                unzClose(zip);
                return false;
            }
        }
    }

    unzClose(zip);
    return true;
}

// -------------------------------------

void data_manager::ensure_missionx_random_folder_exists()
{
  // const fs::path targetDir = "Custom Scenery/missionx/random";
  // init custom scenery folder path
  const std::filesystem::path target_dir = std::filesystem::weakly_canonical (std::filesystem::path(fmt::format("{}/{}", data_manager::getMissionsRootPath(), "random")));
  // init random_pack path
  const std::filesystem::path random_pack_file_s = std::filesystem::weakly_canonical(fmt::format("{}{}", mx_folders_properties.getStringAttributeValue(mxconst::get_PROP_MISSIONX_PATH(), ""), mxconst::get_MISSIONX_RANDOM_PACK_FILENAME()));


  try {
    // create_directories creates the folder and any missing parent directories.
    // It returns true if a NEW directory was created, or false if it already existed.
    if (fs::create_directories(target_dir))    
    {
      Log::log_xplm_debug_string(fmt::format("[{}] Folder created successfully: [{}].\n", __func__, target_dir.string()));
      if (data_manager::extract_missionx_library(random_pack_file_s, target_dir.parent_path()))
        Log::log_xplm_debug_string(fmt::format("[{}] Random pack: '{}' extracted successfully to: '{}'.\n", __func__, random_pack_file_s.string(), target_dir.string()));
      else
        Log::log_xplm_debug_string(fmt::format("[{}] !!! Failed extracting Random pack: '{}'!!!\n", __func__, random_pack_file_s.string(), target_dir.string()));
    }
    else
      Log::log_xplm_debug_string(fmt::format("[{}] Folder already exists: [{}].\n", __func__, target_dir.string()));
  }
  catch (const fs::filesystem_error& e) {
    Log::log_xplm_debug_string ( fmt::format("!! [{}] Filesystem error: {}\n\n", __func__, e.what() ) );
  }

}

// -------------------------------------

std::string data_manager::translatePlaneTypeToString(mx_plane_types_enum in_plane_type){
  if (mapPlaneEnumToStringTypes.contains( in_plane_type))
    return mapPlaneEnumToStringTypes[in_plane_type];

  return ""; // v3.0.253.1 this->mapPlaneEnumToStringTypes[in_plane_type]; // should return empty string

}

// -------------------------------------

missionx::mx_plane_types_enum data_manager::translatePlaneTypeToEnum(const std::string& in_plane_type){
  if ( mapPlaneStringTypesToEnum.contains( in_plane_type))
    return mapPlaneStringTypesToEnum[in_plane_type];

  return mapPlaneStringTypesToEnum[EMPTY_STRING]; // should return any
}


// -------------------------------------

void
data_manager::gatherFlightLegStartStats()
{
  strct_currentLegStats4UIDisplay.init();
  strct_currentLegStats4UIDisplay.sLegName                                 = currentLegName;
  strct_currentLegStats4UIDisplay.fStartingFuel                            = dataref_manager::getDataRefValue<float>("sim/flightmodel/weight/m_fuel_total");
  strct_currentLegStats4UIDisplay.fStartingPayload                         = dataref_manager::getDataRefValue<float>("sim/flightmodel/weight/m_fixed");
  strct_currentLegStats4UIDisplay.fCumulativeDistanceFlew_beforeCurrentLeg = gather_stats.get_stats_object().fCumulativeDistance_nm;
  strct_currentLegStats4UIDisplay.iStartLineId                             = gather_stats.get_last_line_id_cai(); // v26.10.2

  // v26.10.2 the leg row is added now (status "running"). A crash or an abort before the leg ends still shows which leg it was
  stats_meta_begin_leg_cai(strct_currentLegStats4UIDisplay);
}

// -------------------------------------

void
data_manager::gatherFlightLegEndStatsAndStoreInVector()
{
  // store end stats
  strct_currentLegStats4UIDisplay.fEndFuel      = dataref_manager::getDataRefValue<float>("sim/flightmodel/weight/m_fuel_total");
  strct_currentLegStats4UIDisplay.fEndPayload   = dataref_manager::getDataRefValue<float>("sim/flightmodel/weight/m_fixed");
  strct_currentLegStats4UIDisplay.fDistanceFlew = gather_stats.get_stats_object().fCumulativeDistance_nm - strct_currentLegStats4UIDisplay.fCumulativeDistanceFlew_beforeCurrentLeg;
  strct_currentLegStats4UIDisplay.sStatus       = mxconst::get_LEG_STATUS_COMPLETED(); // v26.10.2

  // v26.10.2 close the leg row in the stats database (history and the stats screen read the legs from there)
  stats_meta_update_leg_cai(strct_currentLegStats4UIDisplay, mxconst::get_LEG_STATUS_COMPLETED());

  // insert at the beginning of the vector
  vecPreviousLegStats4UIDisplay.insert(vecPreviousLegStats4UIDisplay.begin(), strct_currentLegStats4UIDisplay);
}

// -------------------------------------

std::string
data_manager::get_fail_timer_title_formated_for_imgui(const std::string& in_default_title, const std::string& prefix_to_formated_string)
{
  if (lowestFailTimerName_s.empty())
    return in_default_title;

  const auto remaining_time = mapFailureTimers[lowestFailTimerName_s].getRemainingTime();
  const auto days           = static_cast<int> ( remaining_time / SECONDS_IN_1DAY );
  const auto hours          = static_cast<int> ( remaining_time / SECONDS_IN_1HOUR_3600 );
  const auto minutes        = static_cast<int> ( (remaining_time - ( hours * SECONDS_IN_1HOUR_3600 )) / SECONDS_IN_1MINUTE );
  const auto seconds        = static_cast<int> ( remaining_time - ( hours * SECONDS_IN_1HOUR_3600 + minutes * SECONDS_IN_1MINUTE ) );

  if (days > 0)
    return prefix_to_formated_string + mxUtils::formatNumber<int>(days) + ":" + mxUtils::formatNumber<int>(hours) + ":" + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(d:h:m:s)";

  return prefix_to_formated_string + mxUtils::formatNumber<int>(hours) + ":" + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(h:m:s)";
}

// -------------------------------------

std::string
data_manager::get_fail_timer_in_formated_text(const std::string& prefix_to_formated_string)
{
  if (lowestFailTimerName_s.empty())
    return "";

  auto remaining_time = mapFailureTimers[lowestFailTimerName_s].getRemainingTime();
  auto days           = static_cast<int> ( remaining_time / SECONDS_IN_1DAY );
  auto hours          = static_cast<int> ( remaining_time / SECONDS_IN_1HOUR_3600 );
  auto minutes        = static_cast<int> ( ( remaining_time - ( hours * SECONDS_IN_1HOUR_3600 ) ) / SECONDS_IN_1MINUTE );
  auto seconds        = static_cast<int> ( remaining_time - ( hours * SECONDS_IN_1HOUR_3600 + minutes * SECONDS_IN_1MINUTE ) );

  if (days > 0)
    return prefix_to_formated_string + mxUtils::formatNumber<int>(days) + ":" + mxUtils::formatNumber<int>(hours) + ":" + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(d:h:m:s)";

  return prefix_to_formated_string + mxUtils::formatNumber<int>(hours) + ":" + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(h:m:s)";
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

  const auto  new_local_time_in_seconds               = static_cast<float> (inHours * SECONDS_IN_1HOUR_3600 + inMinutes * SECONDS_IN_1MINUTE);
  const float delta_between_currentLocal_and_newLocal = new_local_time_in_seconds - local_time_sec_f; // we want to minus to add later to zulu

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
  const std::array<std::string, 38> textureName_arr = { mxconst::get_BITMAP_LOAD_MISSION(),
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
                                                  mxconst::get_BITMAP_BTN_CONVERT_FPLN_TO_MISSION_15X15(), // v26.10.2 square image for the "Generate" screen
                                                  mxconst::get_BITMAP_BTN_STATS_24X18(), // v26.10.2

                                                  mxconst::get_BITMAP_BTN_NAVINFO(),
                                                  mxconst::get_BITMAP_BTN_SIMBRIEF_BIG(),
                                                  mxconst::get_BITMAP_BTN_SIMBRIEF_ICO(),
                                                  mxconst::get_BITMAP_BTN_FLIGHTPLANDB(),

                                                  mxconst::get_BITMAP_BTN_FULL_CONTROL(),
                                                  mxconst::get_BITMAP_BTN_SOME_CONTROL(),
                                                  mxconst::get_BITMAP_BTN_FULL_AUTOMATION(),
                                                  mxconst::get_BITMAP_BTN_ACT_GA(),
                                                  mxconst::get_BITMAP_BTN_ACT_TURBOPROP(),
                                                  mxconst::get_BITMAP_BTN_ACT_JET(),
                                                  mxconst::get_BITMAP_BTN_ACT_AIRLINE(),
                                                  mxconst::get_BITMAP_BTN_ACT_AIRLINE_H(),
                                                  mxconst::get_BITMAP_BTN_ACT_CARGO(),
                                                  mxconst::get_BITMAP_BTN_ACT_CARGO_H(),
                                                  mxconst::get_BITMAP_BTN_ACT_HELOS_ACC(),
                                                  mxconst::get_BITMAP_BTN_ACT_HELOS_SRP(),
                                                  mxconst::get_BITMAP_BTN_ACT_HELOS_OIL(),

  };

  // ranged for loop is supported
  for (const auto& f : textureName_arr)
  {
    mxTextureFile tFile;
    tFile.fileName = f;
    tFile.filePath = bitmapPath;

    // BitmapReader::load_textute_and_bind(tFile, errorMsg, false); // v3.0.253.8 do not flip image
    if (BitmapReader::load_texture_no_bind(tFile, errorMsg, false)) // v26.09.3
    {
      mapCachedPluginTextures[tFile.fileName] = tFile;
      // v25.08.1
      const std::string feedback = fmt::format("Loaded bitmap without binding: {} [{}]\n", tFile.getAbsoluteFileLocation (), tFile.texture_hash_simple); // debug
      Log::log_xplm_debug_string(feedback, false); // debug
    }
  } // end texture load without binding

  // v26.09.3 set the bind action to call when x-plane world loads
  set_texture_bind_for_flc_pre(enums::textures_type_enum::plugin_start, mx_flc_pre_command::bind_textures);
}

// -------------------------------------

void data_manager::set_texture_bind_for_flc_pre(const missionx::enums::textures_type_enum in_texture_type, const mx_flc_pre_command in_action_to_call)
{
  g_queue_texture_to_load.push_back(in_texture_type);
  //queFlcActions.push_back(in_action_to_call);
  postFlcActions.push_back(in_action_to_call);
};



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
      const std::string& valu = vecNameValue.at(1);

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
data_manager::addInstanceNameToTheDisplayList(const std::string& instName) // v3.0.207.1
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
  const int entries = XPLMCountFMSEntries();
  for (int i = (entries - 1); i >= 0; --i) // entries starts from 0..N-1
    XPLMClearFMSEntry(i);
}

// -------------------------------------

void
data_manager::clearMissionLoadedTextures()
{
  // clear maps and images data
  XPLMDebugString("\n\t--- Releasing mission textures ---"); // debug
  mapCurrentMissionTexturePathLocator.clear();
  BitmapReader::destroy_textures(mapCurrentMissionTextures); // v26.09.3
  mapCurrentMissionTextures.clear();

  XPLMDebugString("\n\tReleasing inventory textures."); // debug
  BitmapReader::destroy_textures(xp_mapInvImages); // v26.09.3
  xp_mapInvImages.clear();

  XPLMDebugString("\n\tReleasing Message Story textures."); // debug
  releaseMessageStoryCachedTextures(); // v3.305.1

  XPLMDebugString("\n\t--- End Releasing mission textures ---"); // debug

  // v26.09.3 deprecated and moved to "clear_pplugiin_textures"
  // XPLMDebugString("\n\tReleasing Random Template textures."); // debug
  // clearRandomTemplateTextures();
}

void
data_manager::clear_plugin_textures()
{

  XPLMDebugString("\n\t--- Release core plugin textures ---"); // debug

  XPLMDebugString("\n\tReleasing cached textures."); // debug
  BitmapReader::destroy_textures(data_manager::mapCachedPluginTextures); // v26.09.3
  data_manager::mapCachedPluginTextures.clear();

  XPLMDebugString("\n\tReleasing mission list textures."); // debug
  BitmapReader::destroy_textures(data_manager::xp_mapMissionIconImages); // v26.09.3
  data_manager::xp_mapMissionIconImages.clear();

  XPLMDebugString("\n\tReleasing template list textures."); // debug
  clearRandomTemplateTextures();

  XPLMDebugString("\n\t--- End Release core plugin textures ---\n"); // debug

}


// -------------------------------------


void
data_manager::releaseMessageStoryCachedTextures()
{
  // v3.305.1 clear Message story cached images
  Message::vecStoryCurrentImages_p.clear();
  for (size_t loop1 = 0; loop1 < Message::VEC_STORY_IMAGE_SIZE_I; ++loop1) // should have 7
    Message::vecStoryCurrentImages_p.emplace_back(nullptr);

  BitmapReader::destroy_textures(Message::mapStoryCachedImages); // v26.09.3
  Message::mapStoryCachedImages.clear();
}

// -------------------------------------

void
data_manager::clearRandomTemplateTextures()
{

  mapGenerateMissionTemplateFilesLocator.clear(); // clear locator for layer set
  BitmapReader::destroy_textures(data_manager::mapTemplatesTextures);

  mapGenerateMissionTemplateFiles.clear();
  mapTemplatesTextures.clear(); // v26.09.3

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

  for (auto& [file, texture] : xp_mapInvImages)
  {
    mxTextureFile btnImage;
    // mxTextureFile textureFile;
    btnImage.fileName = file;
    btnImage.filePath = mapBrieferMissionList[selectedMissionKey].pathToMissionPackFolderInCustomScenery + XPLMGetDirectorySeparator();

    if (BitmapReader::load_texture_no_bind(btnImage, errorMsg, false)) // load image but do not flip it
    {
      // store vecTextures as generic texture button
      texture = btnImage;
      texture.store_hash();

      Log::logMsg(fmt::format("Loaded Inventory Texture: {}\n", texture.fileName)); // debug

    } // end if loaded texture
    else
    {
      Log::logMsg( fmt::format("Failed to load Inventory texture file: {}\nError: {}\n", file, errorMsg) );
    }
  }

   if (!xp_mapInvImages.empty())
    postFlcActions.push_back(mx_flc_pre_command::inv_post_async_inv_image_binding);

}

// -------------------------------------

#ifdef USE_TRIGGER_OPTIMIZATION // v3.305.2
void
missionx::data_manager::optimizeLegTriggers_thread(base_thread::strct_thread_state* outThreadState, missionx::Point* inRefPoint, std::map<std::string, missionx::Trigger>* inMapTriggers, std::list<std::string>* inListTriggersByDistance, std::list<std::string>* outListTriggers_ptr)
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
    Log::logMsg((!inImageName_vu.empty()) ? "Loading: " + inImageName_vu : "Release image.");
    #endif // !RELEASE

    if (mxUtils::isElementExists(Message::mapStoryCachedImages, inImageName_vu))
    {
      #ifndef RELEASE
      Log::logMsg("Found image in cache: " + inImageName_vu + ", Skipping load.");
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
      Log::logMsg("Final image filename: " + btnImage.fileName);
      #endif // !RELEASE

      std::string errorMsg; // v24.06.1
      if (!btnImage.fileName.empty() && BitmapReader::load_texture_no_bind(btnImage, errorMsg, false))
      {
        Utils::addElementToMap(Message::mapStoryCachedImages, inImageName_vu, btnImage);
        Message::lineAction4ui.state = enum_mx_line_state::ready;

        Log::logMsg("Loaded Story Texture: " + btnImage.fileName); // debug

      } // end if loaded texture
      else
      {
        if (inImageName_vu.empty())
          Log::logMsg("Releasing image in position: " + Message::lineAction4ui.vals[mxconst:: mxconst::get_STORY_IMAGE_POS()]); // debug
        else
          Log::logMsg("Failed loading Story Texture: " + inImageName_vu); // debug

      } // end if Texture loaded
    }   // end if Texture file name is not in the message cache

    queFlcActions.push_back(mx_flc_pre_command::post_async_story_image_binding); // Always call this action so it will mark the [i] action as "action_ended". It also binding unbound textures.

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
data_manager::load_all_missions_images_no_bind() // used in list mission screen (for example)
{
  iMissionImageCounter = 0;
  std::string errorMsg; // v24.06.1

  // v26.09.3 release textures
  if (!xp_mapMissionIconImages.empty())
    BitmapReader::destroy_textures(xp_mapMissionIconImages);
  xp_mapMissionIconImages.clear(); // v26.09.3

  for (auto& [file_s, mInfo] : mapBrieferMissionList)
  {
    if (!mInfo.briefer_image.fileName.empty() && !mInfo.briefer_image.filePath.empty())
    {
      if (BitmapReader::load_texture_no_bind(mInfo.briefer_image, errorMsg, false)) // load image but do not flip it
      {
                
        // Store in texture container
        xp_mapMissionIconImages[file_s] = mInfo.briefer_image; // v26.09.3 store the mxTextureFile in our generic image map for later use

        Log::logMsg(fmt::format("Loaded Mission Image - not bind texture: {} [{}]", mInfo.briefer_image.fileName, mInfo.briefer_image.texture_hash_simple)); // debug
      }
      else
      {
        Log::logMsg (errorMsg);
      }
    }

  } // end loop over valid missions

  // v26.09.3 add bind_texture_request
  if (!xp_mapMissionIconImages.empty())
    data_manager::set_texture_bind_for_flc_pre(enums::textures_type_enum::mission_list_screen, mx_flc_pre_command::bind_textures);

  Log::logMsg("!! After loading all mission list images, without binding !! ");
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
data_manager::is_choice_name_exists(const std::string& inChoiceName)
{
  #ifndef RELEASE
  const IXMLNode xNode = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(xmlChoices,  mxconst::get_ELEMENT_CHOICE(),  mxconst::get_ATTRIB_NAME(), inChoiceName, false); // debug
  #endif                                                                                                                                                                                                 // !RELEASE

  // search <choice> element with name: inChoiceName. If found then it is not empty node.
  return !(Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(xmlChoices,  mxconst::get_ELEMENT_CHOICE(),  mxconst::get_ATTRIB_NAME(), inChoiceName, false).isEmpty());
}


// -------------------------------------

std::string
data_manager::write_fpln_to_external_folder(const std::map<int, missionx::NavAidInfo>& in_map_fms_entries)
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
      Log::logAttention(err); // false = not thread message
    }
    else
    {
      // PREPARE data based on the FMS // v25.05.1 added lambda to dynamically use the <container> or the <fms> based on available data.
      const auto lmbda_get_fms_navaid_info =[] (const std::map<int, missionx::NavAidInfo>& local_map_fms_entries)
      {
        // Due to duplicate FMS entries.
        auto fms_entries = extractNavaidInfoFromPlanesFMS (); // will be empty if plane has FMS and not G1000/G530/G4xx
        if (!fms_entries.empty())
          return fms_entries;

        std::map<int, missionx::NavAidInfo> gps_navaid_entries = local_map_fms_entries;
        if (local_map_fms_entries.empty())
          gps_navaid_entries = parse_gps_node(); // try to read GPS info from mission file, ignores navaid display restrictions.

        std::deque<NavAidInfo> fmsNavAids;
        for (const auto &navAid : gps_navaid_entries | std::views::values)
          fmsNavAids.push_back(navAid);

        return fmsNavAids;
      };

      if (std::deque<NavAidInfo> fmsNavAids = lmbda_get_fms_navaid_info (in_map_fms_entries);
        !fmsNavAids.empty())
      {
        std::ofstream outFPLN_file;
        // read each line and parse it
        std::string line;
        while (getline(infs, line))
        {
          // skip, if it has comment string or is empty
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
              if (fpln_type == GNSRXP)
              {
                return ".fpl";
              }
              if (fpln_type == LTLNAV)
              {
                return ".lnmpln";
              }
              if (fpln_type == XPLN11)
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
                Log::logAttention(err); // false = not thread message
              }
              else
              {
                #ifndef RELEASE
                Log::logMsg("[write_fpln_to_external_folder] folder: " + fpln_custom_missionx_path); // false = not thread message
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
  constexpr size_t MAX_GTN_ICAO_STRING_LENGTH = 5;        // according to "GTN Flight Plan and User Waypoint transfer.pdf". Paragraph 2.4 En Route. ":F:<Waypoint identifier, up to five characters>
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
        Log::logMsg("[lat] angle: " + mxUtils::formatNumber<float>(navInfo.lat, 6));
        #endif
        std::string lat_s = Utils::DegreesMinutesSecondsLat_XP((double)navInfo.lat);
        #ifndef RELEASE
        Log::logMsg(" after:" + lat_s + ", angle: " + mxUtils::formatNumber<float>(navInfo.lon, 6) + " ");
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
  float elev_f = 0.0f;
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
      navInfo = get_plane_airport_or_nearest_icao(true, navPos.lat, navPos.lon);
      // Copy back original position
      navInfo.lat = navPos.lat;
      navInfo.lon = navPos.lon;
      if (navInfo.flag_fetched_from_db && navInfo.navRef != XPLM_NAV_NOT_FOUND) // did we found nav aid in a boundary in a valid ICAO ?
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

    if (!(b_isItAnAirport) && Utils::isStringIsValidArithmetic(icao))  // v3.0.303.2 use WPn(lat/lon) instead of just "lat/lon"
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

    // fictive elevation
    if (counter == 1 || counter == NUMENR )
      elev_f = (navInfo.height_mt * missionx::meter2feet > 0.0f)? navInfo.height_mt * missionx::meter2feet : 10.0f;
    else
      elev_f = 2000.0f;

    // Handle waypoints
    switch (navInfo.navType)
    {
      case xplm_Nav_Airport:
      {
        if (counter >= NUMENR - 1)
            elev_f = 0.0f; // on ground

        waypoints_s += fmt::format("{} {} {} {} {} {} \n", 1, icao, ((counter == 1) ? AIRPORT_DEPARTURE : (counter == NUMENR) ? AIRPORT_DEST : DIRECT), elev_f, navInfo.getLat(), navInfo.getLon());
      }
      break;
      case xplm_Nav_NDB:
      {
        waypoints_s += fmt::format("{} {} {} {} {} {} \n", 2, icao, ((counter == 1) ? NON_AIRPORT_DEPARTURE : (counter == NUMENR) ? NON_AIRPORT_DEST : DIRECT), elev_f, navInfo.getLat(), navInfo.getLon()); // Not an airport should be DES code, I also changed first enroute to DEP from ADEP
      }
      break;
      case xplm_Nav_VOR:
      {
        waypoints_s += fmt::format("{} {} {} {} {} {} \n", 3, icao, ((counter == 1) ? NON_AIRPORT_DEPARTURE : (counter == NUMENR) ? NON_AIRPORT_DEST : DIRECT), elev_f, navInfo.getLat(), navInfo.getLon()); // Not an airport should be DES code, I also changed first enroute to DEP from ADEP
      }
      break;
      case xplm_Nav_Fix:
      {
        waypoints_s += fmt::format("{} {} {} {} {} {} \n", 11, icao, ((counter == 1) ? NON_AIRPORT_DEPARTURE : (counter == NUMENR) ? NON_AIRPORT_DEST : DIRECT), elev_f, navInfo.getLat(), navInfo.getLon()); // Not an airport should be DES code, I also changed first enroute to DEP from ADEP
      }
      break;
      default: // unnamed lat/lon and other types
      {
        waypoints_s += fmt::format("{} {} {} {} {} {} \n", 28, icao, ((counter == 1) ? NON_AIRPORT_DEPARTURE : (counter == NUMENR) ? NON_AIRPORT_DEST : DIRECT), elev_f, navInfo.getLat(), navInfo.getLon()); // Not an airport should be DES code, I also changed first enroute to DEP from ADEP
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
        if (navInfo.get_name_or_icao_id().empty()
           || (navInfo.get_name_or_icao_id().front() == '+' || navInfo.get_name_or_icao_id().front() == '-') )
          name = ident_s;
        else
          name = navInfo.get_name_or_icao_id();

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

  mapMessages.clear();

  errStr.clear();
  briefer.clear();

  lstLoadErrors.clear(); // v3.305.3 replaces load Errors

  missionSavepointFilePath.clear();
  missionSavepointDrefFilePath.clear();
  mx_global_settings.clear(); // v3.0.241.1

  smPropSeedValues.clear();

  map3dInstances.clear();
  st_map3d_obj.clear();
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

  close_stats_db(true); // v26.10.2 flush=true: a stopped mission keeps its last rows, and its outcome is written. v26.10.1 was: db_close_database(db_stats). Stops the writer thread first.

  mapInterpolDatarefs.clear(); // v3.303.13

  gather_stats.reset();                   // v3.303.14
  strct_currentLegStats4UIDisplay.init(); // v3.303.14
  vecPreviousLegStats4UIDisplay.clear();  // v3.303.14

  listOfMessageStoryMessages.clear(); // v3.305.2

  missionx::data_manager::active_acf_info.reset(); // v26.09.2
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


  //curl = curl_easy_init();
  //if (!curl)
  //{
  //  throw std::runtime_error("[data_manager] Couldn't initialize curl");
  //}

  xmlMappingNode = IXMLNode::emptyIXMLNode;

  draw_script.clear();

  XPLMGetVersions (&xplane_ver_i, &xplm_version, &host_id); // v25.09.1
  xplane_ver_i = XPLMGetDatai (dc.dref_xplane_version_i);

  xplane_using_modern_driver_b = XPLMGetDatai(dc.dref_xplane_using_modern_driver_b);

  prop_userDefinedMission_ui.node.updateName(mxconst::get_ELEMENT_UI_PROPERTIES().c_str());

  listOfMessageStoryMessages.clear();

  mapQueries.clear();

  lst_of_failed_3d_obj_to_load.clear(); // v26.01.1
  lst_of_errors_and_warnings_during_mission_validation.clear (); // v26.01.1
}

// -------------------------------------

void
data_manager::release_static()
{

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

  mx_folders_properties.setStringProperty(mxconst::get_FLD_MISSIONS_ROOT_PATH(), getMissionsRootPath()); // v3.0.129 {full path}/Custom Scenery/missionx

  // v3.0.217.1 Random folders
  mx_folders_properties.setStringProperty(mxconst::get_FLD_RANDOM_MISSION_PATH(), mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONS_ROOT_PATH(), "", errStr) + XPLMGetDirectorySeparator() + "random"); // v3.0.217.1
  mx_folders_properties.setStringProperty(mxconst::get_FLD_RANDOM_TEMPLATES_PATH(), mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", errStr) + "templates");                              // v3.0.217.1

  // v3.0.219.9 AptDat folders
  mx_folders_properties.setStringProperty(mxconst::get_FLD_CUSTOM_SCENERY_FOLDER_PATH(), Utils::getCustomSceneryRelativePath());    // "Custom Scenery". This is not absolute path
  if (xplane_ver_i < XP12_VERSION_NO)                                                                               // less than 120,000 means it is xp11
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
  // holds plane position
  double y,    z;
  double outX, outY, outZ;
  double x                   = y    = z    = 0.0;
  outX                       = outY = outZ = 0.0;
  XPLMDataRef gHeadingPsiRef = nullptr; // heading - true north


  x = XPLMGetDataf(drefConst.dref_camera_view_x_f);
  y = XPLMGetDataf(drefConst.dref_camera_view_y_f);
  z = XPLMGetDataf(drefConst.dref_camera_view_z_f);

  XPLMLocalToWorld(x, y, z, &outX, &outY, &outZ);
  outZ *= meter2feet; // v24.03.2 fix elevation to be feet and not meter
  Point           p = Point(outX, outY, outZ);
  XPLMProbeResult outResult;
  double          groundElev_mt = Point::getTerrainElevInMeter_FromPoint(p, outResult);
  p.setElevationMt(groundElev_mt);

  // read longitude/latitude info
  double gHeading = XPLMGetDataf(gHeadingPsiRef);
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

        outError += fmt::format(R"(In Objective: "{}", {})", objName, txt);
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
          const std::string linked_name_vu = "[" + cTaskName + "]";
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

            const std::string txt = fmt::format("Script: '{}', is not valid. Please fix it. In task: '{}', it was marked as undefined.", scriptName, cTaskName); // v3.305.3
            Utils::xml_add_error_child(cLogNode, txt);                                                                                                    // v3.305.3

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
  // 2. loop over all Legs Objectives and check if one of them is mandatory and if it has TARGET_POI

  bool                   flag_found          = false;
  bool                   flagAllLegsAreValid = true;
  std::list<std::string> listEraseFromLeg; // holds trigger names to erase from a global leg list

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
    std::string desc          = Utils::xml_get_text_or_cdata_text(xDesc, Utils::xml_get_text_or_cdata_text(leg.node, "")); // v3.305.3 fixing wrong info that there is no description, we take into consideration dorect description // v3.303.14 fixed desc warning //   leg.getAttribStringValue(mxconst::get_ELEMENT_DESC(), "" , data_manager::errStr);
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
      const std::string txt = fmt::format(R"(Flight Leg: "{}", has attribute '{}' with the value: "{}" which does not exists. Please fix leg settings.)", legName, mxconst::get_ATTRIB_NEXT_LEG(), nextLeg);
      Utils::xml_add_warning_child(cLogNode, txt);

      outMsg += txt;
    }

    bool foundMandatoryObjective = false;
    if (!leg.getIsDummyLeg())
    {
      // v3.0.241.1 loop over all objective list in Leg and remove those that are not in mapObjectives (meaning: not valid)
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
        const std::string& objName = iterObj; // string - for readability

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

      // try to figure a target position
      if (foundMandatoryObjective && target_pos.empty())
      {
        bool flag_mandatory_task_found = false;
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
data_manager::addDataref(const dataref_param& inDref)
{
  mapDref.insert(make_pair(inDref.getName(), inDref));
}

// -------------------------------------

void
data_manager::setAbortMission(const std::string& inReason)
{
  mx_global_settings.setNodeStringProperty(mxconst::get_PROP_MISSION_ABORT_REASON(), inReason); // v3.303.11
  queFlcActions.push_back(mx_flc_pre_command::abort_mission);
}

// -------------------------------------

void
data_manager::prepareInventoryCopies(const std::string& inInventoryName)
{
  // prepare XML copy
  Log::logDebugBO("[prepareInventoryCopies] start\n");
  assert(mxUtils::isElementExists(data_manager::mapInventories, mxconst::get_ELEMENT_PLANE()) && fmt::format("[{}] Plane inventory was not initialized.", __func__).c_str()); // v24.12.2

  externalInventoryCopy.init();
  if (!inInventoryName.empty() && mapInventories.contains(inInventoryName))
    externalInventoryCopy = mapInventories[inInventoryName]; // v3.0.241.1
  else
    externalInventoryCopy.init();


  //  v24.12.2 Copy Plane inventory
  planeInventoryCopy = mapInventories[mxconst::get_ELEMENT_PLANE()];

  #ifndef RELEASE
  Log::logMsg(fmt::format("==========>\nLocal inventory after Coping Plane Inventory:\n[{}\n].\n====> Source Inventory:\n[{}\n]\n<=========\n", Utils::xml_get_node_content_as_text(planeInventoryCopy.node), Utils::xml_get_node_content_as_text(mapInventories[mxconst::get_ELEMENT_PLANE()].node)));
  #endif


  // v3.0.221.9 calculate but do not apply to dataref since we did not commit
  // v24.12.2 added "force inventory layout"
  data_manager::current_plane_payload_weight_f = calculatePlaneWeight(planeInventoryCopy, false, Inventory::opt_forceInventoryLayoutBasedOnVersion_i);

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
    queFlcActions.push_back(mx_flc_pre_command::inv_read_async_inv_image_files);
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

void data_manager::set_plane_fuel()
{
  dataref_param local_dref_acf_tank_ratio_f_arr (dataref_const::dref_acf_tank_ratio_f, "sim/aircraft/overflow/acf_tank_rat");
  dataref_param local_dref_m_fuel_f_arr (dataref_const::dref_m_fuel_f_arr, "sim/flightmodel/weight/m_fuel");

  local_dref_acf_tank_ratio_f_arr.readArrayDatarefIntoLocalArray();
  local_dref_m_fuel_f_arr.readArrayDatarefIntoLocalArray();

  const auto mission_block_fuel = Utils::readNodeNumericAttrib <float>( mx_global_settings.xBaseWeight_ptr, mxconst::get_OPT_FUEL_BASE_WEIGHT(), 0.0f );
  if (mission_block_fuel <= 0.0f)
    return;


  const auto& tank_ratios = local_dref_acf_tank_ratio_f_arr.out_vecArrayFloatValues;
  auto& fuel_tanks        = local_dref_m_fuel_f_arr.out_vecArrayFloatValues;

  // 1. Ensure target vector matches the available tanks
  assert (tank_ratios.size() == fuel_tanks.size() && "'Fuel tank ratio array' is not equal to 'Fuel Tanks array'.\n");

  // 1. Fetch total aircraft fuel capacity (kg) to calculate each tank's physical max capacity.
  // Dataref: "sim/aircraft/weight/acf_m_fuel_tot"
  const float total_aircraft_fuel_capacity = missionx::dataref_manager::getDataRefValue<float>(dataref_const::dref_rw_acf_m_fuel_tot_kg); // v26.10.2 removed "* lbs2kg", the dataref values are kg

  distribute_weight(tank_ratios, fuel_tanks, mission_block_fuel, total_aircraft_fuel_capacity, true);

  // write back to x-plane
  int counter_fuel_tanks = 0;
  std::string m_tanks_s;
  std::stringstream ss;
  //
  std::ranges::for_each(fuel_tanks,
                        [&](auto& fValue)
                        {
                          ss << (counter_fuel_tanks == 0 ? std::string() : std::string(",")) << fValue;
                          counter_fuel_tanks++;
                        });

  m_tanks_s = ss.str();


  int iResult = local_dref_m_fuel_f_arr.setTargetArray<xplmType_FloatArray, float>(static_cast<int>( counter_fuel_tanks ), m_tanks_s, true, ",");

}


// -------------------------------------


float
data_manager::calculatePlaneWeight(const Inventory& inSourceInventory, const bool& flag_apply_to_dataref, const int& inLayoutVersion)
{
  // float              planePayloadWeight = 0.0f;
  float              passengersWeight, storedWeight;
  float pilotWeight = passengersWeight = storedWeight = 0.0f;

  // v24.03.2 Added compatibility attribute
  pilotWeight      = static_cast<float> ( Utils::readNumericAttrib ( mx_global_settings.xBaseWeight_ptr, mxconst::get_OPT_PILOT_BASE_WEIGHT(), mxconst::get_OPT_PILOT(), 0.0f ) ); // was mxconst::DEFAULT_PILOT_WEIGHT
  storedWeight     = static_cast<float> ( Utils::readNumericAttrib ( mx_global_settings.xBaseWeight_ptr, mxconst::get_OPT_STORAGE_BASE_WEIGHT(), mxconst::get_OPT_STORAGE(), 0.0f ) );
  passengersWeight = static_cast<float> ( Utils::readNumericAttrib ( mx_global_settings.xBaseWeight_ptr, mxconst::get_OPT_PASSENGERS_BASE_WEIGHT(), mxconst::get_OPT_PASSENGERS(), 0.0f ) );

  // v26.09.3 Added re-initialization before modifying it.
  data_manager::dref_acf_station_max_kgs_f_arr.setAndInitializeKey("sim/aircraft/weight/acf_m_station_max");
  data_manager::dref_m_stations_kgs_f_arr.setAndInitializeKey("sim/flightmodel/weight/m_stations");


  // add inventory
  // v3.0.241.1 added weight configuration from Copied Plane Node
  const auto lmbda_calculate_items_weight = [&](const Inventory& inInventory, const int iLayoutVersion)
  {
    float plane_inventory_weight = 0.0f;

    if (inInventory.node.isEmpty())
      return 0.0f;


    // Calculate the weight of the plane inventory, If any.
    if (iLayoutVersion == XP11_COMPATIBILITY) // XP11 compatibility calculation
    {
      const auto invNode   = inInventory.node;
      const int  nChilds_i = invNode.nChildNode (mxconst::get_ELEMENT_ITEM ().c_str ());
      for (int i1 = 0; i1 < nChilds_i; ++i1)
      {
        const auto itemWeight   = Utils::readNodeNumericAttrib<float>(invNode.getChildNode(mxconst::get_ELEMENT_ITEM().c_str(), i1), mxconst::get_ATTRIB_WEIGHT_KG(), 0.0f);
        const auto itemQuantity = Utils::readNodeNumericAttrib<float>(invNode.getChildNode(mxconst::get_ELEMENT_ITEM().c_str(), i1), mxconst::get_ATTRIB_QUANTITY(), 0.0f);

        plane_inventory_weight += (itemWeight * itemQuantity);
      }
    } // XP11 Layout
    else
    { // stations: XP12
      for (const auto& acf_station : planeInventoryCopy.mapStations | std::views::values)
      {
        plane_inventory_weight += acf_station.total_weight_in_station_f;
      }
    }

    return plane_inventory_weight;
  };

  // calculate <plane> inventory
  const auto planeInventoryWeight = lmbda_calculate_items_weight(inSourceInventory.node.isEmpty() ? mapInventories[mxconst::get_ELEMENT_PLANE()] : inSourceInventory, Inventory::opt_forceInventoryLayoutBasedOnVersion_i);

  const float payload_only_weight = storedWeight + passengersWeight + planeInventoryWeight;
  const float planePayloadWeight = pilotWeight + payload_only_weight;

  // store weight in Dataref
  if (planePayloadWeight > 0.0f && flag_apply_to_dataref)
  {
    if ( inLayoutVersion == missionx::XP11_COMPATIBILITY )
    {
      dataref_manager::set_xplane_dataref_value("sim/flightmodel/weight/m_fixed", planePayloadWeight);
    }
    else
    { // XP12 Stations
      // set station weight based on the plane inventory map_stations container.
      if (dref_m_stations_kgs_f_arr.flag_paramReadyToBeUsed && dref_m_stations_kgs_f_arr.arraySize > 0 && dref_acf_station_max_kgs_f_arr.flag_paramReadyToBeUsed && dref_acf_station_max_kgs_f_arr.arraySize > 0)
      {
        // calculate usable stations, if they have a value greater than 0 in them.
        auto number_of_usable_stations = static_cast<int>(std::ranges::count_if(
                                                            dref_acf_station_max_kgs_f_arr.out_vecArrayFloatValues,
                                                                [](const float val) { return val > 0.0f; }
                                                          ));


        // prepare a vector with the station weights
        std::string       m_stations_s;
        // int               m_stations_size_i = 0;
        size_t counter_stations = 0;

        // if (auto vec_m_stations = mapInventories[mxconst::get_ELEMENT_PLANE()].get_inventory_station_weights_as_vector(dref_m_stations_kgs_f_arr.arraySize)
        //     ; !vec_m_stations.empty())
        if (!dref_m_stations_kgs_f_arr.out_vecArrayFloatValues.empty() && dref_m_stations_kgs_f_arr.arraySize > 0)
        {
          // calculate average weight per statiion, without the pilot weight
          // const auto station_number_without_pilot = (vec_m_stations.size() - 1 > 0)? vec_m_stations.size() - 1 : 1;
          const auto station_number_without_pilot = (number_of_usable_stations - 1 > 0)? number_of_usable_stations - 1 : 1;
          const auto avg_weight_per_station_exclude_pilot = payload_only_weight / static_cast<float>(station_number_without_pilot);
          auto weight_to_distribute = planePayloadWeight;


          std::stringstream ss;
          // Inject weights
          for (auto &station : dref_m_stations_kgs_f_arr.out_vecArrayFloatValues)
          {
            if (counter_stations == 0 && dref_m_stations_kgs_f_arr.out_vecArrayFloatValues.at ( 0 ) < mxconst::MINIMUM_EXPECTED_PILOT_WEIGHT_IN_STATION && pilotWeight > 0.0f)
              dref_m_stations_kgs_f_arr.out_vecArrayFloatValues.at(counter_stations) = pilotWeight;

            counter_stations++;
            break; // exit after handling the pilot station
          }

          // distribute the rest of the weight
          constexpr static float MAX_ALLOWED_PAYLOAD_WEIGHT = 0.0f;
          distribute_weight(dref_acf_station_max_kgs_f_arr.out_vecArrayFloatValues, dref_m_stations_kgs_f_arr.out_vecArrayFloatValues, planePayloadWeight, MAX_ALLOWED_PAYLOAD_WEIGHT, false);

          m_stations_s.clear();
          counter_stations = 0;
          std::ranges::for_each(dref_m_stations_kgs_f_arr.out_vecArrayFloatValues,
                                [&](auto& fValue)
                                {
                                  ss << (counter_stations == 0 ? std::string() : std::string(",")) << fValue;
                                  counter_stations++;
                                });

          m_stations_s = ss.str();
        }

        int iResult = dref_m_stations_kgs_f_arr.setTargetArray<xplmType_FloatArray, float>(static_cast<int>( counter_stations ), m_stations_s, true, ",");
      }
    } // end XP12 stations calculation
  } // end if planePayloadWeight > 0.0f && flag_apply_to_dataref

  // return the calculated plane weight based on the mission file values.
  return planePayloadWeight;
}

// -------------------------------------

void data_manager::distribute_weight(const std::vector<float>& max_weights, std::vector<float>& target_weights, const float weight_to_distribute, const float max_allowed_weight, const bool flag_reset_all_target_weights)
{
if (target_weights.size() != max_weights.size())
    {
        target_weights.resize(max_weights.size(), 0.0f);
    }

    // 1. Detect if max_weights represents ratios (all non-zero values <= 1.0f)
    float max_val = 0.0f;
    for (float val : max_weights)
    {
        if (val > max_val)
            max_val = val;
    }

    const bool is_ratio = (max_val > 0.0f && max_val <= 1.0f);

    if (is_ratio)
    {
        assert(max_allowed_weight > 0.0f && "max_allowed_weight must be > 0.0f when max_weights holds ratios (0..1).");
        if (max_allowed_weight <= 0.0f)
            return;
    }

    // 2. Reset targets and select start index:
    // If flag_reset_all_target_weights is true (e.g., fuel tanks): reset all and start at index 0.
    // If false (e.g., payload stations): keep index 0 (pilot) untouched and start at index 1.
    const size_t start_index = flag_reset_all_target_weights ? 0 : 1;

    if (flag_reset_all_target_weights)
    {
        // should be used with fuel
        std::fill(target_weights.begin(), target_weights.end(), 0.0f);
    }
    else
    {
        // In the case of payload, we skip the first station, since it belongs to the pilot.
        for (size_t i = start_index; i < target_weights.size(); ++i)
            target_weights[i] = 0.0f;
    }

    // 3. Precompute capacities and collect usable stations
    std::vector<float> capacities(max_weights.size(), 0.0f);
    std::vector<size_t> usable_stations;
    usable_stations.reserve(max_weights.size());

    for (size_t i = start_index; i < max_weights.size(); ++i)
    {
        if (max_weights[i] > 0.0f)
        {
            capacities[i] = is_ratio ? (max_weights[i] * max_allowed_weight) : max_weights[i];
            usable_stations.push_back(i);
        }
    }

    if (usable_stations.empty() || weight_to_distribute <= 0.0f)
        return;

    std::vector<size_t> active_elements_in_vector = usable_stations;
    float remaining_weight = weight_to_distribute;

    // Phase 1: Distribute evenly, capping stations that reach max allowable capacity
    while (!active_elements_in_vector.empty() && remaining_weight > 0.001f)
    {
        const float fair_share = remaining_weight / static_cast<float>(active_elements_in_vector.size());
        bool capped_any = false;

        std::vector<size_t> next_active;
        next_active.reserve(active_elements_in_vector.size());

        for (size_t idx : active_elements_in_vector)
        {
            const float capacity_left = capacities[idx] - target_weights[idx];

            if (capacity_left <= fair_share)
            {
                target_weights[idx] = capacities[idx];
                remaining_weight -= capacity_left;
                capped_any = true;
            }
            else
            {
                next_active.push_back(idx);
            }
        }

        if (!capped_any)
        {
            for (size_t idx : active_elements_in_vector)
                target_weights[idx] += fair_share;

            remaining_weight = 0.0f;
            break;
        }

        active_elements_in_vector = std::move(next_active);
    }

    // Phase 2: Overweight fallback
    // If all usable stations reached capacity, share excess weight equally
    if (remaining_weight > 0.001f)
    {
        const float excess_share = remaining_weight / static_cast<float>(usable_stations.size());
        for (size_t idx : usable_stations)
            target_weights[idx] += excess_share;
    }
}

// -------------------------------------

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
    for ( const auto &key : st_map3d_obj | std::views::keys )
      st_map3d_obj[key].saveCheckpoint(x3dTemplates);

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
    queFlcActions.pop_front();

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
      if ((mxconst::get_TRIG_TYPE_RAD() == trigType || mxconst::get_TRIG_TYPE_CAMERA() == trigType) && !mapTriggers[trigName].deqPoints.empty())
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
        for (const auto& p : cue.vecPoints)
        {
          Point pUpper = p; // clone p
          pUpper.setElevationFt(attrib_elev_ft_upper);
          pUpper.calcSimLocalData();

          result                              = -1;
          const double groundElevationInMeter = CueInfo::getTerrainElevInMeter_FromPoint(pUpper, result);

          if (groundElevationInMeter > pUpper.getElevationInMeters())
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
          for (const auto& p : listUpperPoints)
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
          } // end iterate over Points
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

          int  counter = 0;

          for (auto& point : cue_ptr.vecPoints)
          {
            if (counter == 0) // v3.0.231.2 added starting directional color to help designers figure out which way he/she positioned the plane - clockwise or counter clock wize.
            {
              const CueInfo::struct_color tmpC = cue_ptr.color;
              cue_ptr.color.setToRed();
              glColor3f(cue_ptr.color.R, cue_ptr.color.G, cue_ptr.color.B); // draw GL as RED
              cue_ptr.color.setRGB(tmpC.R, tmpC.G, tmpC.B); // return to the default color
            }
            else if (counter == 1)
              glColor3f(cue_ptr.color.R, cue_ptr.color.G, cue_ptr.color.B);

            counter++;

            // draw point, will be enclosed by the GL_LINE_LOOP
            XPLMMapProject((*projection_ptr), point.getLat(), point.getLon(), &x, &y);
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

bool
data_manager::is_gps_point_a_target_cai(IXMLNode in_point)
{
  // An explicit flag always wins.
  if (in_point.isAttributeSet(mxconst::get_ATTRIB_IS_TARGET_POINT_B().c_str()))
    return Utils::readBoolAttrib(in_point, mxconst::get_ATTRIB_IS_TARGET_POINT_B(), true);

  // A "point of interest" is something you can navigate to, but it is not a target.
  if (in_point.isAttributeSet(mxconst::get_ATTRIB_POI_TAG().c_str()))
    return false;

  // A point that was saved as a VOR/NDB/Fix (LittleNavMap conversion for example) is a route waypoint, not a target.
  const int nav_type_i = Utils::readNodeNumericAttrib<int>(in_point, mxconst::get_ATTRIB_NAV_TYPE(), xplm_Nav_Unknown);
  if (nav_type_i == xplm_Nav_VOR || nav_type_i == xplm_Nav_NDB || nav_type_i == xplm_Nav_Fix)
    return false;

  // Any other coordinate is treated as a target (Random Engine targets, skewed positions, hand made coordinates).
  return true;
}

// -------------------------------------

missionx::NavAidInfo
data_manager::get_fms_entry_for_gps_point_cai(IXMLNode in_point, const bool in_is_route_edge, const bool in_restore_as_saved)
{
  const auto        point_lat_f  = Utils::readNodeNumericAttrib<float>(in_point, mxconst::get_ATTRIB_LAT(), 0.0f);
  const auto        point_lon_f  = Utils::readNodeNumericAttrib<float>(in_point, mxconst::get_ATTRIB_LONG(), 0.0f);
  const auto        point_elev_ft_f = Utils::readNodeNumericAttrib<float>(in_point, mxconst::get_ATTRIB_ELEV_FT(), 0.0f);
  const std::string point_icao_s = Utils::readAttrib(in_point, mxconst::get_ELEMENT_ICAO(), "");
  const int         point_nav_type_i = Utils::readNodeNumericAttrib<int>(in_point, mxconst::get_ATTRIB_NAV_TYPE(), xplm_Nav_Unknown);

  // The fallback: the point coordinate as is.
  missionx::NavAidInfo lat_lon_entry(point_lat_f, point_lon_f, point_elev_ft_f * missionx::feet2meter);
  lat_lon_entry.navRef  = XPLM_NAV_NOT_FOUND;
  lat_lon_entry.navType = xplm_Nav_LatLon;

  if (point_lat_f == 0.0f && point_lon_f == 0.0f)
    return lat_lon_entry;

  // Search the navaid closest to the point, optionally by ID, and accept it only if it is within in_max_distance_nm.
  const auto lmbda_find_navaid_near_point = [&](const std::string& in_navaid_id, const XPLMNavType in_nav_types, const double in_max_distance_nm) -> std::optional<missionx::NavAidInfo>
  {
    float search_lat_f = point_lat_f;
    float search_lon_f = point_lon_f;
    const XPLMNavRef nav_ref = XPLMFindNavAid(nullptr, (in_navaid_id.empty()) ? nullptr : in_navaid_id.c_str(), &search_lat_f, &search_lon_f, nullptr, in_nav_types);
    if (nav_ref == XPLM_NAV_NOT_FOUND)
      return std::nullopt;

    missionx::NavAidInfo found_navaid;
    found_navaid.navRef = nav_ref;
    XPLMGetNavAidInfo(nav_ref, &found_navaid.navType, &found_navaid.lat, &found_navaid.lon, &found_navaid.height_mt, &found_navaid.freq, &found_navaid.heading, found_navaid.ID, found_navaid.name, nullptr);

    if (const double distance_nm = mxUtils::mxCalcDistanceBetween2Points(found_navaid.lat, found_navaid.lon, point_lat_f, point_lon_f);
        distance_nm > in_max_distance_nm)
      return std::nullopt;

    return found_navaid;
  };

  // Saved mission (<fms> from a checkpoint): restore the entry exactly as it was saved.
  if (in_restore_as_saved)
  {
    if (point_nav_type_i > xplm_Nav_Unknown && point_nav_type_i != xplm_Nav_LatLon && !point_icao_s.empty())
    {
      if (const auto saved_navaid = lmbda_find_navaid_near_point(point_icao_s, point_nav_type_i, mxconst::GPS_NAVAID_MATCH_RADIUS_NM))
        return *saved_navaid;
    }
    return lat_lon_entry;
  }

  // New mission, rule 1: any coordinate inside an airport is replaced by the airport ICAO.
  //                      A skewed position is never replaced, it was moved away from the target on purpose.
  if (const bool is_skewed_position = Utils::readBoolAttrib(in_point, mxconst::get_ATTRIB_IS_SKEWED_POSITION_B(), false);
      !is_skewed_position)
  {
    // Step 1: is the coordinate inside an airport boundary (or on one of its runways) ? Plugin airports database.
    const auto lmbda_find_airport_by_boundary = [&]() -> std::optional<missionx::NavAidInfo>
    {
      std::lock_guard<std::mutex> lock(s_thread_sync_mutex); // db_xp_airports and its statements are shared with background jobs
      if (!init_xp_airport_db())
        return std::nullopt;

      auto boundary_airport = get_plane_airport_or_nearest_icao(true, point_lat_f, point_lon_f, false); // true = database only, false = not a thread
      if (!boundary_airport.flag_fetched_from_db || boundary_airport.navRef == XPLM_NAV_NOT_FOUND)
        return std::nullopt;

      XPLMGetNavAidInfo(boundary_airport.navRef, &boundary_airport.navType, &boundary_airport.lat, &boundary_airport.lon, &boundary_airport.height_mt, &boundary_airport.freq, &boundary_airport.heading, boundary_airport.ID, boundary_airport.name, nullptr);
      return boundary_airport;
    };

    if (const auto boundary_airport = lmbda_find_airport_by_boundary())
    {
      Log::logMsg(fmt::format("[{}] ({:.5f}, {:.5f}) is inside the boundary of {}.", __func__, point_lat_f, point_lon_f, boundary_airport->getID()));
      return *boundary_airport;
    }

    // Step 2: no boundary match, search the airport nearest to the coordinate, up to GPS_AIRPORT_MATCH_RADIUS_NM (~1.2nm).
    if (!point_icao_s.empty())
    {
      if (const auto airport = lmbda_find_navaid_near_point(point_icao_s, xplm_Nav_Airport, mxconst::GPS_AIRPORT_MATCH_RADIUS_NM))
        return *airport;
    }
    if (const auto nearest_airport = lmbda_find_navaid_near_point("", xplm_Nav_Airport, mxconst::GPS_AIRPORT_MATCH_RADIUS_NM))
      return *nearest_airport;
  }

  // New mission, rule 2: an in-between waypoint that sits on a VOR/NDB/Fix is replaced by the navaid, but only if it is not a target.
  if (!in_is_route_edge && !is_gps_point_a_target_cai(in_point))
  {
    constexpr XPLMNavType route_navaid_types = xplm_Nav_VOR | xplm_Nav_NDB | xplm_Nav_Fix;
    if (!point_icao_s.empty())
    {
      const XPLMNavType search_types = (point_nav_type_i > xplm_Nav_Unknown && point_nav_type_i != xplm_Nav_LatLon) ? point_nav_type_i : route_navaid_types;
      if (const auto named_navaid = lmbda_find_navaid_near_point(point_icao_s, search_types, mxconst::GPS_NAVAID_MATCH_RADIUS_NM))
        return *named_navaid;
    }
    if (const auto nearest_navaid = lmbda_find_navaid_near_point("", route_navaid_types, mxconst::GPS_NAVAID_MATCH_RADIUS_NM))
      return *nearest_navaid;
  }

  return lat_lon_entry;
}

// -------------------------------------

void
data_manager::set_fms_entry_cai(const int in_entry_index, const missionx::NavAidInfo& in_entry)
{
  const int altitude_ft_i = static_cast<int>(in_entry.height_mt * missionx::meter2feet); // FMS altitudes are in feet

  if (in_entry.navRef != XPLM_NAV_NOT_FOUND)
    XPLMSetFMSEntryInfo(in_entry_index, in_entry.navRef, altitude_ft_i);
  else
    XPLMSetFMSEntryLatLon(in_entry_index, in_entry.lat, in_entry.lon, altitude_ft_i);
}

// -------------------------------------

std::map<int, missionx::NavAidInfo>
data_manager::setGPS()
{
  const bool        bUseGPS         = (xmlLoadedFMS.isEmpty ()) ? true : false; // v25.04.2 decide if to use GPS or FMS, we will use the bool value to decide if to read the auto_load option or not.
  const IXMLNode    xFMS_ptr        = (bUseGPS) ? xmlGPS : xmlLoadedFMS; // pick the FMS element or GPS dependent on availability in the mission/checkpoint file.
  std::map<int, missionx::NavAidInfo> map_fms_entries; // v25.05.1

  if (xFMS_ptr.isEmpty())
    return map_fms_entries;

  // v25.04.2
  if (bUseGPS)
  {
    const bool bAutoLoad    = Utils::readBoolAttrib (data_manager::xmlGPS, mxconst::get_PROP_AUTO_LOAD_ROUTE_TO_GPS_OR_FMS_B (), mxconst::DEFAULT_AUTO_LOAD_ROUTE_TO_GPS_OR_FMS_B);
    const bool bGenerateGPS = Utils::readBoolAttrib (data_manager::xmlGPS, mxconst::get_PROP_GENERATE_GPS_WAYPOINTS (), false);
    if ( !(bGenerateGPS && bAutoLoad) )
    {
      Log::logMsg(fmt::format("[{}] GPS was not loaded. generate_gps_waypoints: {}, auto_load_route_to_gps_or_fms_b: {}", __func__, bGenerateGPS, bAutoLoad));
      return map_fms_entries; // exit the code in this function.
    }
  }

  // clear GPS
  clearFMSEntries(); // v3.0.253.7

  // v26.10.2 the <gps> element is resolved with the "new mission" rules (ICAO replacement). A saved <fms> element is restored as is.
  map_fms_entries = parse_gps_node();
  for (const auto& [entry_index, fms_entry] : map_fms_entries)
  {
    set_fms_entry_cai(entry_index, fms_entry);
    Log::logMsg(fmt::format("[{}] FMS entry {}: {} ({:.5f}, {:.5f})", __func__, entry_index, (fms_entry.navRef != XPLM_NAV_NOT_FOUND) ? fms_entry.getID() : std::string("lat/lon"), fms_entry.lat, fms_entry.lon));
  }

  // Decide on which FMS entry to point
  const int entries = XPLMCountFMSEntries ();
  int iCurrentGpsActiveEntry = static_cast<int> ( Utils::readNumericAttrib ( xFMS_ptr, mxconst::get_ATTRIB_DESTINATION_ENTRY(), -1 ) );

  // validations
  if (entries > 0 && iCurrentGpsActiveEntry < 0)
    iCurrentGpsActiveEntry = 1;
  else if ( iCurrentGpsActiveEntry < 0 )
    iCurrentGpsActiveEntry = 0;

  // set FMS only if positive
  if (iCurrentGpsActiveEntry)
    XPLMSetDestinationFMSEntry(iCurrentGpsActiveEntry); // set GPS/FMS active entry

  return map_fms_entries;
}

// -------------------------------------

std::map<int, missionx::NavAidInfo> data_manager::parse_gps_node()
{
  // v26.10.2 read only: builds the FMS entries list from <gps> (new mission rules) or from the saved <fms> (restore as is).
  //          It does NOT write into the X-Plane FMS, setGPS() does that. Also used to write the external flight plan file.
  const bool     bUseGPS = (xmlLoadedFMS.isEmpty ()) ? true : false;
  const IXMLNode xFMS_ptr = (bUseGPS) ? xmlGPS : xmlLoadedFMS;
  std::map<int, missionx::NavAidInfo> map_fms_entries; // v25.05.1

  if (xFMS_ptr.isEmpty())
    return map_fms_entries;

  const int  nChilds              = xFMS_ptr.nChildNode(mxconst::get_ELEMENT_POINT().c_str());
  const bool flag_restore_as_saved = !bUseGPS;

  int valid_entries_counter = 0;
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    const IXMLNode cNode_ptr        = xFMS_ptr.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), i1);
    const bool     is_route_edge    = (i1 == 0 || i1 == nChilds - 1); // first and last waypoints must be an airport or a coordinate, never a VOR/NDB/Fix
    const auto     fms_entry        = get_fms_entry_for_gps_point_cai(cNode_ptr, is_route_edge, flag_restore_as_saved);

    Utils::addElementToMap (map_fms_entries, valid_entries_counter, fms_entry);
    ++valid_entries_counter;
  } // end loop over all child <point>s

  return map_fms_entries;
}

// -------------------------------------

void
data_manager::addLatLonEntryToGPS(Point& inPoint, const int& inEntry)
{

  {
    int entries = XPLMCountFMSEntries ();
    if (inEntry < 0)
      XPLMSetFMSEntryLatLon(entries, static_cast<float> ( inPoint.getLat () ), static_cast<float> ( inPoint.getLon () ), static_cast<int> ( inPoint.getElevationInMeters () ) );
    else
      XPLMSetFMSEntryLatLon(inEntry, static_cast<float> ( inPoint.getLat () ), static_cast<float> ( inPoint.getLon () ), static_cast<int> ( inPoint.getElevationInMeters () ) );
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
    switch ( static_cast<int> ( vecCmdSplit.size () ) )
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
data_manager::stop_plugin()
{
  // v26.09.3 clear METAR thread
  constexpr static int wait_time = 5;
  data_manager::threadStateMetar.flagAbortThread = true;
  if (data_manager::threadStateMetar.flagIsActive)
  {
    XPLMDebugString (fmt::format("\nmissionx: data_manager: [{}] Waiting for threadStateMetar.flagIsActive.\n Wait for {} seconds.", __func__, wait_time).c_str()); // debug
    std::this_thread::sleep_for(std::chrono::seconds(wait_time));
  }

  // clear mission related textures, not plugin.
  clearMissionLoadedTextures();

  seqCueInfo = 0; // v3.0.202a
}

// -------------------------------------

void
data_manager::loadAll3dObjectFiles()
{
  data_manager::lst_of_failed_3d_obj_to_load.clear();

  std::string objName;
  objName.clear();

  for ( auto &obj : st_map3d_obj | std::views::values )
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
    {
      Log::logMsg("Failed Loading: " + obj.file_and_path);
      data_manager::lst_of_failed_3d_obj_to_load.emplace_back(obj.file_and_path); // v26.01.1
    }

  } // end loop over all files

  // v3.303.11 look up and set all instances that were loaded from savepoint
  if (mx_global_settings.flag_loadedFromSavepoint)
  {
    for (auto& [key, instObj] : map3dInstances)
    {
      const auto obj3d_template_name = instObj.getName();
      if (mxUtils::isElementExists(st_map3d_obj, obj3d_template_name))
      {
        instObj.g_object_ref = st_map3d_obj[obj3d_template_name].g_object_ref;
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

    IXMLNode xml_legSpeciallNode_ptr = mapFlightLegs[currentLegName].xmlSpecialDirectives_ptr;
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
    std::string dref_name = "xpshared/target/lat"; // holds target latitude
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
    queFlcActions.push_back(mx_flc_pre_command::execute_xp_command);

  return err;
}

// -------------------------------------

void
data_manager::inject_metar_file(const std::string& inFileName)
{
  if (!inFileName.empty())
  {
    metar_file_to_inject_s = inFileName;
    queFlcActions.push_back(mx_flc_pre_command::inject_metar_file); // v3.0.223.1
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

      m.node = Utils::xml_get_node_from_XSD_map_as_a_copy( mxconst::get_ELEMENT_MESSAGE ()); // debug prepare message node

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
data_manager::parse_leg_DisplayObjects(Waypoint& inLeg)
{
  ////////////////////////////
  // read 3d object instances

  // v3.305.3
  auto cLogNode = Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(xLoadInfoNode.node, mxconst::get_ELEMENT_LEG(), mxconst::get_ATTRIB_NAME(), inLeg.getName());
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

  int nChilds2 = inLeg.node.nChildNode();
  for (int i2 = 0; i2 < nChilds2; ++i2)
  {
    mx_base_node instProp; // will hold specific properties for a 3D instance
    instProp.node = inLeg.node.getChildNode(i2).deepCopy(); // v3.303.11 fetch next <display_node>
    if (!instProp.node.isEmpty())
    {
      const std::string tag_s = instProp.node.getName();
      // filter childs
      if (tag_s != mxconst::get_ELEMENT_DISPLAY_OBJECT() && tag_s != mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE())
        continue;

      if (tag_s == mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE()) // rename <display_object_near_plane> to <display_object> and force a fictive lat/long positioning
      {
        instProp.node.updateName(mxconst::get_ELEMENT_DISPLAY_OBJECT().c_str());
        // Force replace_lat / replace_long dummy values to be on the safe side
        instProp.node.updateAttribute("1.0", mxconst::get_ATTRIB_REPLACE_LAT().c_str(), mxconst::get_ATTRIB_REPLACE_LAT().c_str());
        instProp.node.updateAttribute("1.0", mxconst::get_ATTRIB_REPLACE_LONG().c_str(), mxconst::get_ATTRIB_REPLACE_LONG().c_str());
      }

      // read basic attribute data
      std::string instance_name = Utils::readAttrib(instProp.node, mxconst::get_ATTRIB_INSTANCE_NAME(), "");
      std::string obj3d_name    = Utils::readAttrib(instProp.node,  mxconst::get_ATTRIB_NAME(), "");
      std::string link_task     = Utils::readAttrib(instProp.node, mxconst::get_ATTRIB_LINK_TASK(), "");

      #ifndef RELEASE
      if (obj3d_name.find("ambulance") != std::string::npos)
      {
        int i_stop = 0;
      }
      #endif

      if ( obj3d_name.empty() ||  !data_manager::st_map3d_obj.contains(obj3d_name) )
      {
        // v3.305.3
        const std::string txt = fmt::format("The <display_object>: '{}' is missing a reference node in the <object_templates> root node. Skipping instance.", obj3d_name);
        Utils::xml_add_warning_child(cLogNode, txt);

        addLoadErr(txt);
        continue;
      }

      if (instance_name.empty())
      {
        // v3.305.3
        const std::string txt = fmt::format("[{}]Found <display_object> without instance name, check: '{}'. Skipping instance.", __func__, obj3d_name);
        Utils::xml_add_warning_child(cLogNode, txt);

        addLoadErr(txt);
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

          // search for the specific value in the map and if it was found then continue processing. If not then skip with log message.
          const auto& propIter = mapOfAllowedReplaceAttributesInDisplayObject.find(attribWithoutPrefix);
          if (propIter == iterEnd)
          {
            // v3.305.3
            const std::string txt = fmt::format("Instance: '{}', Found unrecognized replacement attribute name: '{}'. Fix the attribute name or ask the developer to support it.", instance_name, attribWithoutPrefix);
            Utils::xml_add_warning_child(cLogNode, txt);
            continue;
          }

        } // end if found prefix_
      }   // end loop over attributes



      // v3.0.253.7 Fix bug where missing <display_object> lat or lon were not copied over from their Obj3D if they do not exists
      bool flag_found = false;
      if (Utils::readAttrib(instProp.node, mxconst::get_ATTRIB_LAT(), "").empty())
      {
        const std::string obj_lat = Utils::xml_get_attribute_value_drill(st_map3d_obj[obj3d_name].node, mxconst::get_ATTRIB_LAT(), flag_found, mxconst::get_ELEMENT_LOCATION());
        if (flag_found)
          instProp.setNodeProperty<double>(mxconst::get_ATTRIB_LAT(), mxUtils::stringToNumber<double>(obj_lat));
      }
      if (Utils::readAttrib(instProp.node, mxconst::get_ATTRIB_LONG(), "").empty())
      {
        flag_found                 = false;
        const std::string obj_long = Utils::xml_get_attribute_value_drill(st_map3d_obj[obj3d_name].node, mxconst::get_ATTRIB_LONG(), flag_found, mxconst::get_ELEMENT_LOCATION());
        if (flag_found)
          instProp.setNodeProperty<double>(mxconst::get_ATTRIB_LONG(), mxUtils::stringToNumber<double>(obj_long));
      }

      // add instance to 3D Object as a sub-element for future use (if the instance is not part of it, yet).
      if (st_map3d_obj[obj3d_name].node.getChildNodeWithAttribute(mxconst::get_ELEMENT_DISPLAY_OBJECT().c_str(), mxconst::get_ATTRIB_INSTANCE_NAME().c_str(), instance_name.c_str()).isEmpty())
        st_map3d_obj[obj3d_name].node.addChild(instProp.node.deepCopy()); // add display objects element to the 3D Object as a future instance

      #ifndef RELEASE
      Log::logMsg(fmt::format("[{}] Added DisplayObject: {}\n", __func__, Utils::xml_get_node_content_as_text(instProp.node) ) );
      #endif // !RELEASE

      // add instance to Flight Leg set
      Utils::addElementToMap(inLeg.list_displayInstances, instance_name, obj3d_name); // v3.305.1 converted list to a map
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
      inOutText = Utils::replaceString(inOutText, key, dref.get_dataref_scalar_value_as_string(), true);
    }
  }

  //// Do the same using mission dtarefs and not just pre-defined ones
  for (auto& [key, dref] : mapDref)
  {
    if (dref.flag_paramReadyToBeUsed && dref.dataRefType < xplmType_FloatArray) // if not an array or data (string)
    {
      dref.readDatarefValue_into_missionx_plugin();
      inOutText = Utils::replaceString(inOutText, key, dref.get_dataref_scalar_value_as_string(), true);
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
data_manager::init_in_memory_xp_airport_db()
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

std::string
data_manager::get_stats_db_path()
{
  // v26.10.1 same string as before in init_stats_db(), now shared with the stats fetch
  return mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", errStr) + mxconst::get_DB_FOLDER_NAME() + "/" + mxconst::get_DB_STATS_XP();
}

// -------------------------------------

bool
data_manager::init_stats_db()
{
  const std::string path_to_stats_db_s = get_stats_db_path(); // v26.10.1
  stats_meta_wait_for_route_cai(); // v26.10.2 the previous flight's route job writes to the same file
  reset_in_flight_stats_request_cai(); // v26.10.2 a new flight (or a checkpoint load): no in-flight stats request, no old copy. No writer runs yet
  if (db_connect(db_stats, path_to_stats_db_s))
  {
    Log::logMsg(fmt::format("[{}] Connected to: {}", __func__, path_to_stats_db_s) );
    return true;
  }

  return false;
}

// -------------------------------------

void
data_manager::close_stats_db(const bool in_flush_pending_rows, const bool in_finalize_flight)
{
  // v26.10.1 the writer thread uses db_stats. Stop it (and optionally write its last rows) before closing.
  gather_stats.detach_db(in_flush_pending_rows);
  reset_in_flight_stats_request_cai(); // v26.10.2 the writer is stopped: drop the in-flight stats request and its copy

  if (db_stats.db && db_stats.db_is_open_and_ready)
  {
    // v26.10.2 the writer is detached, the connection is still open: write the flight outcome.
    //          Only the first close of a flight writes it (flight_info.outcome is still "running"), later closes change nothing.
    std::string outcome_written;
    if (in_finalize_flight)
      outcome_written = stats_meta_end_flight_cai();

    db_stats.close_database();
    Log::logMsg(fmt::format("[{}] Stats database closed.", __func__));

    // v26.10.2 build the route on its own thread and connection. Not for "interrupted": X-Plane or the plugin is stopping.
    if (!outcome_written.empty() && outcome_written != mxconst::get_FLIGHT_OUTCOME_INTERRUPTED())
    {
      stats_meta_wait_for_route_cai(); // a previous job, if any
      futStatsRoute = std::async(std::launch::async, data_manager::stats_meta_build_route_thread_cai, get_stats_db_path(), outcome_written, stats_history_get_max_files_cai()); // v26.10.2 the setup option is read here, on the main thread
    }

  }
}

// -------------------------------------
// v26.10.2 In-flight stats copy file: next to the live stats file. Not in the history folder, so the stats screen list never shows it.
std::string
data_manager::get_in_flight_stats_db_path_cai()
{
  return (std::filesystem::path(get_stats_db_path()).parent_path() / mxconst::get_STATS_IN_FLIGHT_FILE_NAME()).string();
}

// -------------------------------------
// v26.10.2 No in-flight stats request: mission start, end, stop and checkpoint load (init_stats_db() / close_stats_db()).
//          Called when no writer thread runs, so the copy state can go back to idle. The UI drops its data when the sequence changes.
//          The copy file is deleted. If the stats tab still reads it (Windows keeps it open), the next copy deletes it before it is written.
void
data_manager::reset_in_flight_stats_request_cai()
{
  flag_is_stats_request_is_active = false;
  ++in_flight_stats_reset_seq;
  gather_stats.reset_copy_cai();

  std::error_code ec;
  std::filesystem::remove(get_in_flight_stats_db_path_cai(), ec);
}

// -------------------------------------
// v26.10.2 flcPRE action "prepare_in_flight_stats" (the "stats" tab was clicked, flag_is_stats_request_is_active is already true).
//          A checkpoint save/load or a mission stop waits in the action queues: postpone to the next flight loop, so the copy
//          and the checkpoint (which waits for the writer) do not run together. The sim paused: no writer runs, the request waits.
//          Only while the mission runs: any other state drops the request.
void
data_manager::handle_in_flight_stats_request_cai()
{
  if (!flag_is_stats_request_is_active)
    return; // reset since the click (the mission ended or a checkpoint was loaded)

  const auto lmbda_is_checkpoint_action_waiting = []() -> bool
  {
    const auto lmbda_is_checkpoint_action = [](const missionx::mx_flc_pre_command in_command)
    {
      return in_command == missionx::mx_flc_pre_command::create_savepoint || in_command == missionx::mx_flc_pre_command::create_savepoint_and_quit ||
             in_command == missionx::mx_flc_pre_command::load_savepoint || in_command == missionx::mx_flc_pre_command::stop_mission ||
             in_command == missionx::mx_flc_pre_command::abort_mission;
    };
    return std::ranges::any_of(queFlcActions, lmbda_is_checkpoint_action) || std::ranges::any_of(postFlcActions, lmbda_is_checkpoint_action);
  };

  // only while the mission runs. Any other state drops the request (abort_in_flight_stats_request_if_not_running_cai() does the same every flight loop)
  if (missionState != missionx::mx_mission_state_enum::mission_is_running || GatherStats::flag_ignore_stats_setup || gather_stats.get_db_ptr() == nullptr)
  {
    Log::logMsg(fmt::format("[{}] No in-flight stats: the mission is not running or the stats are not written.", __func__));
    abort_in_flight_stats_request_cai();
    return;
  }

  // not yet: a checkpoint save/load or a mission stop waits in the action queues
  if (lmbda_is_checkpoint_action_waiting())
  {
    postFlcActions.push_back(missionx::mx_flc_pre_command::prepare_in_flight_stats);
    return;
  }

  if (gather_stats.request_copy_cai(get_in_flight_stats_db_path_cai()))
    Log::logMsg(fmt::format("[{}] In-flight stats copy requested: '{}'.", __func__, get_in_flight_stats_db_path_cai()));
}

// -------------------------------------
// v26.10.2 Drop the in-flight stats request. Safe while the writer runs: a copy in progress is interrupted and deleted by the writer.
//          The UI drops its request state when the sequence changes. The copy file itself is deleted by reset_in_flight_stats_request_cai().
void
data_manager::abort_in_flight_stats_request_cai()
{
  flag_is_stats_request_is_active = false;
  ++in_flight_stats_reset_seq;
  gather_stats.abort_copy_cai();
}

// -------------------------------------
// v26.10.2 Every flight loop (Mission::flc(), paused or not): the in-flight stats copy is only for a running mission.
//          Any other state (stopped, ended, loading a checkpoint...) aborts a request and a copy in progress.
void
data_manager::abort_in_flight_stats_request_if_not_running_cai()
{
  if (missionState == missionx::mx_mission_state_enum::mission_is_running)
    return;

  // "copying" without an active request = already aborted, the writer is finishing it (do not interrupt it again every flight loop)
  const auto copy_state = gather_stats.get_copy_state_cai();
  if (flag_is_stats_request_is_active || (copy_state != missionx::mx_stats_copy_state::idle && copy_state != missionx::mx_stats_copy_state::copying))
  {
    Log::logMsg(fmt::format("[{}] The mission is not running: the in-flight stats request is dropped.", __func__));
    abort_in_flight_stats_request_cai();
  }
}

// -------------------------------------
// v26.10.2 User plane .acf file name, and its path relative to the X-Plane folder (the record stays valid if X-Plane is moved).
//          If the path is not under the X-Plane folder, the full path is returned. Main thread only.
static void
stats_meta_get_user_acf_cai(std::string& out_acf_file, std::string& out_acf_relative_path)
{
  char acf_file[512]{};
  char acf_path[2048]{};
  XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, acf_file, acf_path);

  char sys_path[2048]{};
  XPLMGetSystemPath(sys_path);

  out_acf_file          = acf_file;
  out_acf_relative_path = acf_path;

  const std::string sys_path_s = sys_path;
  if (!sys_path_s.empty() && out_acf_relative_path.starts_with(sys_path_s))
    out_acf_relative_path = out_acf_relative_path.substr(sys_path_s.length());
}

// -------------------------------------
// v26.10.2 Write the flight_info row and the first plane_meta row (line_id 0) into the stats database.
//          Main thread only (reads datarefs). Called from Mission::START_MISSION() after gather_stats.setDb(), before any stats writer thread runs.
//          New mission: setDb() created empty tables, both rows are added.
//          Checkpoint : the tables came back with the saved stats DB and already have their rows. flight_info is kept.
//                       A plane_meta row is added only if the current plane is not the last plane in the table.
//                       A checkpoint saved before v26.10.2 has empty tables, so the rows are added now.
//          Call it after gather_stats.init_seq_from_checkpoint_stats(), so a checkpoint plane row gets the right line_id.
void
data_manager::stats_meta_begin_flight_cai()
{
  if (GatherStats::flag_ignore_stats_setup)
    return;

  if (db_stats.db == nullptr || !db_stats.db_is_open_and_ready)
  {
    Log::logMsgErr(fmt::format("[{}] Stats database is not open. Flight metadata was not written.", __func__));
    return;
  }

  // ----- flight_info -----
  std::string mission_title = Utils::readAttrib(xMainNode, mxconst::get_ATTRIB_TITLE(), "");
  if (mission_title.empty())
    mission_title = Utils::readAttrib(xMainNode, mxconst::get_ATTRIB_NAME(), "");

  // The scoring rules this flight is scored with. Stored so the stats screen can be rebuilt later with the same rules.
  const std::string scoring_xml = Utils::xml_get_node_content_as_text(get_scoring_with_landing_defaults());

  // "OR IGNORE": id is always 1, so an existing row (checkpoint) is kept.
  const std::string sql_flight_info = fmt::format(R"(INSERT OR IGNORE INTO {} (id, flight_id, schema_version, plugin_version, xp_version, mission_key, mission_title, outcome, scoring_xml)
VALUES (1, STRFTIME('%Y%m%d_%H%M%S', 'now', 'localtime'), ?1, ?2, ?3, ?4, ?5, ?6, ?7))",
                                                  mxconst::get_DB_TABLE_FLIGHT_INFO());

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_stats.db, sql_flight_info.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_int (stmt, 1, mxconst::STATS_SCHEMA_VERSION);
    sqlite3_bind_text(stmt, 2, missionx::FULL_VERSION.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 3, XPLMGetDatai(drefConst.dref_xplane_version_i));
    sqlite3_bind_text(stmt, 4, selectedMissionKey.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, mission_title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, mxconst::get_FLIGHT_OUTCOME_RUNNING().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, scoring_xml.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
      if (sqlite3_changes(db_stats.db) > 0)
        Log::logMsg(fmt::format("[{}] {} row added. Mission: '{}' ({})", __func__, mxconst::get_DB_TABLE_FLIGHT_INFO(), mission_title, selectedMissionKey));
      else
        Log::logMsg(fmt::format("[{}] {} row already exists (checkpoint), kept.", __func__, mxconst::get_DB_TABLE_FLIGHT_INFO()));
    }
    else
      Log::logMsgErr(fmt::format("[{}] Failed to write {}: {}", __func__, mxconst::get_DB_TABLE_FLIGHT_INFO(), sqlite3_errmsg(db_stats.db)));
  }
  else
    Log::logMsgErr(fmt::format("[{}] Failed to prepare {} insert: {}", __func__, mxconst::get_DB_TABLE_FLIGHT_INFO(), sqlite3_errmsg(db_stats.db)));

  sqlite3_finalize(stmt); // no-op on nullptr

  // ----- plane_meta: only when the table is empty (new mission, or a checkpoint from before v26.10.2) -----
  int plane_rows = -1;
  stmt           = nullptr;
  if (sqlite3_prepare_v2(db_stats.db, fmt::format("SELECT count(*) FROM {}", mxconst::get_DB_TABLE_PLANE_META()).c_str(), -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
    plane_rows = sqlite3_column_int(stmt, 0);
  else
    Log::logMsgErr(fmt::format("[{}] Failed to count {} rows: {}", __func__, mxconst::get_DB_TABLE_PLANE_META(), sqlite3_errmsg(db_stats.db)));

  sqlite3_finalize(stmt);

  if (plane_rows == 0)
    stats_meta_insert_plane_cai(0);
  else if (plane_rows > 0) // checkpoint
    stats_meta_plane_loaded_cai();
}

// -------------------------------------
// v26.10.2 Add a plane_meta row for the current user plane.
//          in_line_id: last stats row written before this plane was loaded (0 = mission start).
//          Main thread only (reads datarefs). The stats writer thread must not run (it uses the same connection).
bool
data_manager::stats_meta_insert_plane_cai(const int in_line_id)
{
  if (GatherStats::flag_ignore_stats_setup)
    return false;

  if (db_stats.db == nullptr || !db_stats.db_is_open_and_ready)
  {
    Log::logMsgErr(fmt::format("[{}] Stats database is not open. Plane metadata was not written.", __func__));
    return false;
  }

  // read a string (byte array) dataref. Empty when the dataref does not exist.
  const auto read_dref_text = [](const char* in_dref_name) -> std::string {
    const XPLMDataRef dref = XPLMFindDataRef(in_dref_name);
    if (dref == nullptr)
      return {};

    char buf[512]{}; // longest one is acf_author (byte[500])
    XPLMGetDatab(dref, buf, 0, static_cast<int>(sizeof(buf)) - 1); // the last char stays '\0'
    return mxUtils::trim(std::string(buf));
  };

  // read an int dataref. -1 when the dataref does not exist (X-Plane 11 has no "sim/aircraft2/metadata" datarefs).
  const auto read_dref_int = [](const char* in_dref_name) -> int {
    const XPLMDataRef dref = XPLMFindDataRef(in_dref_name);
    return (dref == nullptr) ? -1 : XPLMGetDatai(dref);
  };

  // plane file, and its path relative to the X-Plane folder
  std::string acf_file;
  std::string acf_path_s;
  stats_meta_get_user_acf_cai(acf_file, acf_path_s);

  // X-Plane 12 categories, a plane can have more than one. Empty in X-Plane 11.
  static const std::vector<std::pair<const char*, const char*>> vecCategories = { { "sim/aircraft2/metadata/is_helicopter", "helicopter" },
                                                                                  { "sim/aircraft2/metadata/is_vtol", "vtol" },
                                                                                  { "sim/aircraft2/metadata/is_glider", "glider" },
                                                                                  { "sim/aircraft2/metadata/is_seaplane", "seaplane" },
                                                                                  { "sim/aircraft2/metadata/is_airliner", "airliner" },
                                                                                  { "sim/aircraft2/metadata/is_cargo", "cargo" },
                                                                                  { "sim/aircraft2/metadata/is_military", "military" },
                                                                                  { "sim/aircraft2/metadata/is_general_aviation", "general_aviation" },
                                                                                  { "sim/aircraft2/metadata/is_ultralight", "ultralight" },
                                                                                  { "sim/aircraft2/metadata/is_experimental", "experimental" },
                                                                                  { "sim/aircraft2/metadata/is_sci_fi", "sci_fi" } };
  std::string category;
  for (const auto& [dref_name, label] : vecCategories)
  {
    if (read_dref_int(dref_name) == 1)
      category += (category.empty() ? "" : ",") + std::string(label);
  }

  int is_helicopter = read_dref_int("sim/aircraft2/metadata/is_helicopter");
  if (is_helicopter < 0) // X-Plane 11: same test as dataref_manager::CalculateEstimatedFuelTimeHours()
    is_helicopter = (read_dref_int("sim/aircraft/artstability/acf_has_clutch") > 0) ? 1 : 0;

  const std::string acf_icao   = read_dref_text("sim/aircraft/view/acf_ICAO");
  const std::string acf_desc   = read_dref_text("sim/aircraft/view/acf_descrip");
  const std::string acf_author = read_dref_text("sim/aircraft/view/acf_author");

  const float empty_kg    = XPLMGetDataf(drefConst.dref_acf_m_empty_weight);
  const float max_kg      = XPLMGetDataf(drefConst.dref_acf_m_max_weight);
  const float max_fuel_kg = XPLMGetDataf(drefConst.dref_rw_acf_m_fuel_tot_kg); // the dataref values are kg
  const float vno_kias    = XPLMGetDataf(drefConst.dref_acf_vno_f);
  const int   num_engines = read_dref_int("sim/aircraft/engine/acf_num_engines");

  const std::string sql = fmt::format(R"(INSERT INTO {} (line_id, acf_file, acf_path, acf_icao, acf_desc, acf_author, category, is_helicopter, empty_kg, max_kg, max_fuel_kg, vno_kias, num_engines)
VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13))",
                                      mxconst::get_DB_TABLE_PLANE_META());

  bool          result = false;
  sqlite3_stmt* stmt   = nullptr;
  if (sqlite3_prepare_v2(db_stats.db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_int   (stmt, 1, in_line_id);
    sqlite3_bind_text  (stmt, 2, acf_file.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 3, acf_path_s.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 4, acf_icao.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 5, acf_desc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 6, acf_author.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 7, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int   (stmt, 8, is_helicopter);
    sqlite3_bind_double(stmt, 9, empty_kg);
    sqlite3_bind_double(stmt, 10, max_kg);
    sqlite3_bind_double(stmt, 11, max_fuel_kg);
    sqlite3_bind_double(stmt, 12, vno_kias);
    sqlite3_bind_int   (stmt, 13, num_engines);

    result = (sqlite3_step(stmt) == SQLITE_DONE);
    if (result)
      Log::logMsg(fmt::format("[{}] {} row added. line_id: {}, plane: {} ({}), category: '{}', helicopter: {}", __func__, mxconst::get_DB_TABLE_PLANE_META(), in_line_id, acf_file, acf_icao, category, is_helicopter));
    else
      Log::logMsgErr(fmt::format("[{}] Failed to write {}: {}", __func__, mxconst::get_DB_TABLE_PLANE_META(), sqlite3_errmsg(db_stats.db)));
  }
  else
    Log::logMsgErr(fmt::format("[{}] Failed to prepare {} insert: {}", __func__, mxconst::get_DB_TABLE_PLANE_META(), sqlite3_errmsg(db_stats.db)));

  sqlite3_finalize(stmt);
  return result;
}

// -------------------------------------
// v26.10.2 The user plane was (re)loaded during the flight, or a checkpoint was loaded.
//          Adds a plane_meta row only when the plane differs from the last row in the table: X-Plane also sends XPLM_MSG_PLANE_LOADED
//          when the same plane is reloaded, and for AI planes.
//          The queued stats rows are written first, so the new row's line_id is the last row sampled with the previous plane.
//          Main thread only.
void
data_manager::stats_meta_plane_loaded_cai()
{
  if (GatherStats::flag_ignore_stats_setup)
    return;

  if (gather_stats.get_db_ptr() == nullptr || db_stats.db == nullptr || !db_stats.db_is_open_and_ready) // no mission stats are written
    return;

  std::string acf_file;
  std::string acf_path;
  stats_meta_get_user_acf_cai(acf_file, acf_path);

  // last plane in the table
  std::string   last_acf_file;
  std::string   last_acf_path;
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_stats.db, fmt::format("SELECT IFNULL(acf_file, ''), IFNULL(acf_path, '') FROM {} ORDER BY rowid DESC LIMIT 1", mxconst::get_DB_TABLE_PLANE_META()).c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
      last_acf_file = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      last_acf_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    }
  }
  else
    Log::logMsgErr(fmt::format("[{}] Failed to read the last {} row: {}", __func__, mxconst::get_DB_TABLE_PLANE_META(), sqlite3_errmsg(db_stats.db)));

  sqlite3_finalize(stmt);

  if (acf_file == last_acf_file && acf_path == last_acf_path)
    return; // same plane

  gather_stats.flush_pending_rows_cai(); // after this, get_last_line_id_cai() is the last row of the previous plane
  stats_meta_insert_plane_cai(gather_stats.get_last_line_id_cai());
}

// -------------------------------------
// v26.10.2 A flight leg starts: add its leg_stats row with status "running" and the start values (fuel, payload, distance so far).
//          A row that is still "running" belongs to a leg that never ended (example: a script moved to another leg): it is marked "skipped".
//          The queued stats rows are written first: the writer thread uses the same connection, and after the flush
//          get_last_line_id_cai() is the last row before this leg (inout_started_leg.iStartLineId).
//          Sets inout_started_leg.iLegSeq, used later by stats_meta_update_leg_cai(). Main thread only.
bool
data_manager::stats_meta_begin_leg_cai(missionx::mx_enrout_stats_strct &inout_started_leg)
{
  if (GatherStats::flag_ignore_stats_setup)
    return false;

  if (gather_stats.get_db_ptr() == nullptr || db_stats.db == nullptr || !db_stats.db_is_open_and_ready) // no mission stats are written
  {
    Log::logMsgErr(fmt::format("[{}] Stats database is not open. Leg '{}' has no {} row.", __func__, inout_started_leg.sLegName, mxconst::get_DB_TABLE_LEG_STATS()));
    return false;
  }

  gather_stats.flush_pending_rows_cai();
  inout_started_leg.iStartLineId = gather_stats.get_last_line_id_cai();
  inout_started_leg.sStatus      = mxconst::get_FLIGHT_OUTCOME_RUNNING();

  // ---- a previous leg that never ended ----
  const std::string sql_skip = fmt::format("UPDATE {} SET status = ?1, end_line_id = ?2, end_time = STRFTIME('%Y-%m-%d %H:%M:%f', 'now') WHERE status = ?3", mxconst::get_DB_TABLE_LEG_STATS());
  sqlite3_stmt*     stmt     = nullptr;
  if (sqlite3_prepare_v2(db_stats.db, sql_skip.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_text(stmt, 1, mxconst::get_LEG_STATUS_SKIPPED().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 2, inout_started_leg.iStartLineId);
    sqlite3_bind_text(stmt, 3, mxconst::get_FLIGHT_OUTCOME_RUNNING().c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_stats.db) > 0)
      Log::logMsg(fmt::format("[{}] {} leg row(s) did not end, marked '{}'.", __func__, sqlite3_changes(db_stats.db), mxconst::get_LEG_STATUS_SKIPPED()));
  }
  sqlite3_finalize(stmt);
  stmt = nullptr;

  // ---- the new leg ----
  const std::string sql = fmt::format("INSERT INTO {} (leg_name, status, start_line_id, distance_before_nm, start_fuel_kg, start_payload_kg) VALUES (?1, ?2, ?3, ?4, ?5, ?6)",
                                      mxconst::get_DB_TABLE_LEG_STATS());

  bool result = false;
  if (sqlite3_prepare_v2(db_stats.db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_text  (stmt, 1, inout_started_leg.sLegName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 2, inout_started_leg.sStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int   (stmt, 3, inout_started_leg.iStartLineId);
    sqlite3_bind_double(stmt, 4, inout_started_leg.fCumulativeDistanceFlew_beforeCurrentLeg);
    sqlite3_bind_double(stmt, 5, inout_started_leg.fStartingFuel);
    sqlite3_bind_double(stmt, 6, inout_started_leg.fStartingPayload);

    result = (sqlite3_step(stmt) == SQLITE_DONE);
    if (result)
    {
      inout_started_leg.iLegSeq = static_cast<int>(sqlite3_last_insert_rowid(db_stats.db));
      Log::logMsg(fmt::format("[{}] {} row {} added. leg: '{}', start line_id: {}, start fuel: {:.1f} kg, distance so far: {:.2f} nm", __func__, mxconst::get_DB_TABLE_LEG_STATS(), inout_started_leg.iLegSeq,
                              inout_started_leg.sLegName, inout_started_leg.iStartLineId, inout_started_leg.fStartingFuel, inout_started_leg.fCumulativeDistanceFlew_beforeCurrentLeg));
    }
    else
      Log::logMsgErr(fmt::format("[{}] Failed to write {}: {}", __func__, mxconst::get_DB_TABLE_LEG_STATS(), sqlite3_errmsg(db_stats.db)));
  }
  else
    Log::logMsgErr(fmt::format("[{}] Failed to prepare {} insert: {}", __func__, mxconst::get_DB_TABLE_LEG_STATS(), sqlite3_errmsg(db_stats.db)));

  sqlite3_finalize(stmt);
  return result;
}

// -------------------------------------
// v26.10.2 Write the end values of a running leg: last stats line_id, distance, fuel and payload (read from the sim now), and in_status.
//          in_status: "completed" (leg end), "running" (checkpoint progress), or the flight outcome (the flight ended during the leg).
//          Only a row that is still "running" is changed, so a completed leg keeps its values.
//          Works after the writer was stopped (flight end): the queued rows are then already written. Main thread only.
bool
data_manager::stats_meta_update_leg_cai(missionx::mx_enrout_stats_strct &inout_leg, const std::string &in_status)
{
  if (GatherStats::flag_ignore_stats_setup)
    return false;

  if (inout_leg.iLegSeq <= 0) // the leg has no row: the stats database was not open when it started
    return false;

  if (db_stats.db == nullptr || !db_stats.db_is_open_and_ready)
  {
    Log::logMsgErr(fmt::format("[{}] Stats database is not open. Leg '{}' was not updated.", __func__, inout_leg.sLegName));
    return false;
  }

  if (gather_stats.get_db_ptr() != nullptr) // the writer still runs: write its queue first
    gather_stats.flush_pending_rows_cai();

  inout_leg.iEndLineId    = gather_stats.get_last_line_id_cai();
  inout_leg.fEndFuel      = dataref_manager::getDataRefValue<float>("sim/flightmodel/weight/m_fuel_total");
  inout_leg.fEndPayload   = dataref_manager::getDataRefValue<float>("sim/flightmodel/weight/m_fixed");
  inout_leg.fDistanceFlew = gather_stats.get_stats_object().fCumulativeDistance_nm - inout_leg.fCumulativeDistanceFlew_beforeCurrentLeg;

  const std::string sql = fmt::format(R"(UPDATE {} SET status = ?1, end_line_id = ?2, end_time = STRFTIME('%Y-%m-%d %H:%M:%f', 'now'),
 distance_nm = ?3, end_fuel_kg = ?4, end_payload_kg = ?5
 WHERE leg_seq = ?6 AND status = ?7)",
                                      mxconst::get_DB_TABLE_LEG_STATS());

  bool          result = false;
  sqlite3_stmt* stmt   = nullptr;
  if (sqlite3_prepare_v2(db_stats.db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_text  (stmt, 1, in_status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int   (stmt, 2, inout_leg.iEndLineId);
    sqlite3_bind_double(stmt, 3, inout_leg.fDistanceFlew);
    sqlite3_bind_double(stmt, 4, inout_leg.fEndFuel);
    sqlite3_bind_double(stmt, 5, inout_leg.fEndPayload);
    sqlite3_bind_int   (stmt, 6, inout_leg.iLegSeq);
    sqlite3_bind_text  (stmt, 7, mxconst::get_FLIGHT_OUTCOME_RUNNING().c_str(), -1, SQLITE_TRANSIENT);

    result = (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_stats.db) > 0);
    if (result)
    {
      inout_leg.sStatus = in_status;
      Log::logMsg(fmt::format("[{}] {} row {} '{}': status '{}', line_id: {} - {}, distance: {:.2f} nm, fuel: {:.1f} -> {:.1f} kg", __func__, mxconst::get_DB_TABLE_LEG_STATS(), inout_leg.iLegSeq, inout_leg.sLegName,
                              in_status, inout_leg.iStartLineId, inout_leg.iEndLineId, inout_leg.fDistanceFlew, inout_leg.fStartingFuel, inout_leg.fEndFuel));
    }
    else if (sqlite3_errcode(db_stats.db) != SQLITE_OK && sqlite3_errcode(db_stats.db) != SQLITE_DONE)
      Log::logMsgErr(fmt::format("[{}] Failed to update {}: {}", __func__, mxconst::get_DB_TABLE_LEG_STATS(), sqlite3_errmsg(db_stats.db)));
    // else: the row is not "running" any more (already completed). Nothing to do
  }
  else
    Log::logMsgErr(fmt::format("[{}] Failed to prepare {} update: {}", __func__, mxconst::get_DB_TABLE_LEG_STATS(), sqlite3_errmsg(db_stats.db)));

  sqlite3_finalize(stmt);
  return result;
}

// -------------------------------------
// v26.10.2 Checkpoint save: the stats file is copied as it is on disk.
//          Write the queued stats rows, and the current leg progress (distance, fuel, payload, still "running"),
//          so a checkpoint load can continue the leg. Main thread only.
void
data_manager::stats_meta_save_leg_progress_cai()
{
  if (GatherStats::flag_ignore_stats_setup || gather_stats.get_db_ptr() == nullptr)
    return;

  if (strct_currentLegStats4UIDisplay.iLegSeq > 0)
    stats_meta_update_leg_cai(strct_currentLegStats4UIDisplay, mxconst::get_FLIGHT_OUTCOME_RUNNING()); // flushes the queue too
  else
    gather_stats.flush_pending_rows_cai();
}

// -------------------------------------
// v26.10.2 Checkpoint load: the saved stats database came back with the legs flown before it.
//          - The ended legs rebuild vecPreviousLegStats4UIDisplay, newest first (same order as gatherFlightLegEndStatsAndStoreInVector()),
//            so the in-mission stats panel and the end screen show them.
//          - The "running" leg is the leg the checkpoint was saved in. A loaded leg does not start again (gatherFlightLegStartStats() is not called),
//            so strct_currentLegStats4UIDisplay is restored from that row, and the flight distance continues from its saved progress.
//          Main thread only. Call after the stats database was opened.
void
data_manager::stats_meta_load_legs_into_memory_cai()
{
  vecPreviousLegStats4UIDisplay.clear();

  if (db_stats.db == nullptr || !db_stats.db_is_open_and_ready)
    return;

  const std::string sql = fmt::format("SELECT IFNULL(leg_name, ''), IFNULL(start_line_id, 0), IFNULL(end_line_id, 0), IFNULL(distance_before_nm, 0.0), IFNULL(distance_nm, 0.0), "
                                      "IFNULL(start_fuel_kg, 0.0), IFNULL(end_fuel_kg, 0.0), IFNULL(start_payload_kg, 0.0), IFNULL(end_payload_kg, 0.0), leg_seq, IFNULL(status, '') "
                                      "FROM {} ORDER BY leg_seq DESC",
                                      mxconst::get_DB_TABLE_LEG_STATS());

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_stats.db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
  {
    // a checkpoint from an older build: the table is missing or has other columns
    Log::logMsgErr(fmt::format("[{}] Failed to read {}: {}", __func__, mxconst::get_DB_TABLE_LEG_STATS(), sqlite3_errmsg(db_stats.db)));
    sqlite3_finalize(stmt);
    return;
  }

  bool flag_found_running_leg = false;
  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    missionx::mx_enrout_stats_strct leg;
    leg.init();
    leg.sLegName                                 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    leg.iStartLineId                             = sqlite3_column_int(stmt, 1);
    leg.iEndLineId                               = sqlite3_column_int(stmt, 2);
    leg.fCumulativeDistanceFlew_beforeCurrentLeg = static_cast<float>(sqlite3_column_double(stmt, 3));
    leg.fDistanceFlew                            = static_cast<float>(sqlite3_column_double(stmt, 4));
    leg.fStartingFuel                            = static_cast<float>(sqlite3_column_double(stmt, 5));
    leg.fEndFuel                                 = static_cast<float>(sqlite3_column_double(stmt, 6));
    leg.fStartingPayload                         = static_cast<float>(sqlite3_column_double(stmt, 7));
    leg.fEndPayload                              = static_cast<float>(sqlite3_column_double(stmt, 8));
    leg.iLegSeq                                  = sqlite3_column_int(stmt, 9);
    leg.sStatus                                  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));

    if (leg.sStatus == mxconst::get_FLIGHT_OUTCOME_RUNNING() && !flag_found_running_leg) // the newest running row: the current leg
    {
      flag_found_running_leg          = true;
      strct_currentLegStats4UIDisplay = leg;

      const float distance_so_far_nm = leg.fCumulativeDistanceFlew_beforeCurrentLeg + leg.fDistanceFlew;
      gather_stats.set_cumulative_distance_nm_cai(distance_so_far_nm);

      Log::logMsg(fmt::format("[{}] Current leg restored: '{}' (row {}), start fuel: {:.1f} kg, distance so far: {:.2f} nm", __func__, leg.sLegName, leg.iLegSeq, leg.fStartingFuel, distance_so_far_nm));
      continue;
    }

    vecPreviousLegStats4UIDisplay.emplace_back(leg);
  }
  sqlite3_finalize(stmt);

  Log::logMsg(fmt::format("[{}] {} ended leg(s) restored from the checkpoint stats.", __func__, vecPreviousLegStats4UIDisplay.size()));
}

// -------------------------------------
// v26.10.2 The flight ended: write os_end_time, outcome, outcome_reason, crashed and crash_source into flight_info.
//          Called from close_stats_db() after the writer was detached (the last rows are written), before the connection closes.
//          The outcome comes from the mission state at that moment:
//            success / failure / aborted : the end screen (ACTION_SHOW_END_SUMMARY_LAYER).
//            stopped                     : the user stopped the mission (Mission::stopMission() sets mission_stopped before data_manager::clear()).
//            interrupted                 : the mission was still running (plugin stop, X-Plane exit).
//          Only a row with outcome "running" is updated, so the first close of the flight wins.
//          Crash: a "crash" stats row (XPLM_MSG_PLANE_CRASHED), else the has_crashed dataref (crash message missed or crashes are not reported).
//          Main thread only (reads a dataref).
std::string
data_manager::stats_meta_end_flight_cai()
{
  if (GatherStats::flag_ignore_stats_setup)
    return {};

  if (db_stats.db == nullptr || !db_stats.db_is_open_and_ready)
    return {};

  std::string outcome_written; // returned: empty when nothing was updated

  std::string outcome;
  switch (missionState)
  {
    case mx_mission_state_enum::mission_completed_success:
      outcome = mxconst::get_FLIGHT_OUTCOME_SUCCESS();
      break;
    case mx_mission_state_enum::mission_completed_failure:
      outcome = mxconst::get_FLIGHT_OUTCOME_FAILURE();
      break;
    case mx_mission_state_enum::mission_aborted:
      outcome = mxconst::get_FLIGHT_OUTCOME_ABORTED();
      break;
    case mx_mission_state_enum::mission_is_running:
      outcome = mxconst::get_FLIGHT_OUTCOME_INTERRUPTED();
      break;
    default: // mission_stopped and any other state
      outcome = mxconst::get_FLIGHT_OUTCOME_STOPPED();
      break;
  }

  // v26.10.2 a leg that did not end (crash, abort, stop) gets the flight outcome as its status. A completed leg is not changed.
  //          "interrupted" (X-Plane or the plugin stopped): the leg stays "running", the checkpoint can continue it.
  if (outcome != mxconst::get_FLIGHT_OUTCOME_INTERRUPTED())
    stats_meta_update_leg_cai(strct_currentLegStats4UIDisplay, outcome);

  // the reason is only meaningful for failure and abort. Success keeps it empty (the property can hold an older value).
  std::string outcome_reason;
  if (outcome == mxconst::get_FLIGHT_OUTCOME_FAILURE() || outcome == mxconst::get_FLIGHT_OUTCOME_ABORTED())
    outcome_reason = Utils::readAttrib(mx_global_settings.node, mxconst::get_PROP_MISSION_ABORT_REASON(), "");

  // crash state from the sim. -1: dataref not found
  const XPLMDataRef dref_has_crashed = XPLMFindDataRef("sim/flightmodel2/misc/has_crashed");
  const int         has_crashed      = (dref_has_crashed == nullptr) ? -1 : XPLMGetDatai(dref_has_crashed);

  // ?1 outcome, ?2 reason, ?3 "crash" activity, ?4 sim_message, ?5 has_crashed, ?6 dataref, ?7 "running"
  const std::string sql = fmt::format(R"(UPDATE {0}
   SET os_end_time    = STRFTIME('%Y-%m-%d %H:%M:%f', 'now'),
       outcome        = ?1,
       outcome_reason = NULLIF(?2, ''),
       crash_source   = CASE WHEN EXISTS (SELECT 1 FROM stats WHERE activity = ?3) THEN ?4
                             WHEN ?5 = 1 THEN ?6
                             ELSE NULL END,
       crashed        = CASE WHEN EXISTS (SELECT 1 FROM stats WHERE activity = ?3) OR ?5 = 1 THEN 1 ELSE 0 END
 WHERE id = 1
   AND outcome = ?7)",
                                      mxconst::get_DB_TABLE_FLIGHT_INFO());

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_stats.db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_text(stmt, 1, outcome.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, outcome_reason.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, mxconst::get_STATS_CRASH().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, mxconst::get_CRASH_SOURCE_SIM_MESSAGE().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 5, has_crashed);
    sqlite3_bind_text(stmt, 6, mxconst::get_CRASH_SOURCE_DATAREF().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, mxconst::get_FLIGHT_OUTCOME_RUNNING().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
      if (sqlite3_changes(db_stats.db) > 0)
      {
        outcome_written = outcome;
        Log::logMsg(fmt::format("[{}] {} outcome: '{}', reason: '{}', has_crashed dataref: {}", __func__, mxconst::get_DB_TABLE_FLIGHT_INFO(), outcome, outcome_reason, has_crashed));
      }
      else
        Log::logMsg(fmt::format("[{}] {} outcome was already written, kept.", __func__, mxconst::get_DB_TABLE_FLIGHT_INFO()));
    }
    else
      Log::logMsgErr(fmt::format("[{}] Failed to update {}: {}", __func__, mxconst::get_DB_TABLE_FLIGHT_INFO(), sqlite3_errmsg(db_stats.db)));
  }
  else
    Log::logMsgErr(fmt::format("[{}] Failed to prepare {} update: {}", __func__, mxconst::get_DB_TABLE_FLIGHT_INFO(), sqlite3_errmsg(db_stats.db)));

  sqlite3_finalize(stmt);
  return outcome_written;
}

// -------------------------------------
// v26.10.2 Build flight_info.route_s from the stats rows, example: "YBTL > YBTL(x3) > nav(-18.752/146.580) > YPAM".
//          Points: the first stats row (mission running), then every landing that is not a bounce (STATS_BOUNCE_WINDOW_SEC).
//          Each point is the airport whose boundary contains it (plugin airports database, same lookup as the stats screen landing table),
//          else "nav(lat/lon)" with 3 decimals. Landings at the same place in a row are collapsed into "NAME(xN)", the start point is never collapsed.
//          Runs with std::async (started by close_stats_db()), with its own connection to the stats file.
//          Holds s_thread_sync_mutex: db_xp_airports and its statements are shared with the stats fetch and other jobs.
//          v26.10.2 After the route, the flight is saved into the history folder when it reached the end screen (success, failure, aborted).
void
data_manager::stats_meta_build_route_thread_cai(const std::string in_stats_db_path, const std::string in_outcome, const int in_history_max_files)
{
  std::lock_guard<std::mutex> lock(s_thread_sync_mutex);

  missionx::dbase local_db; // closed by the destructor
  if (!db_connect(local_db, in_stats_db_path))
  {
    Log::logMsgErr(fmt::format("[{}] Could not open the stats database, no route was built: {}", __func__, local_db.last_err));
    return;
  }

  // the first row, and every landing that is not a bounce
  const std::string sql_points = fmt::format(R"(WITH pts AS (
  SELECT line_id, lat, lon, activity, local_date_days * 86400.0 + local_time_sec AS t,
         max(CASE WHEN activity = '{0}' THEN local_date_days * 86400.0 + local_time_sec END) OVER (ORDER BY line_id ROWS BETWEEN UNBOUNDED PRECEDING AND 1 PRECEDING) AS prev_landing_t
  FROM stats
  WHERE activity = '{0}'
     OR line_id = (SELECT min(line_id) FROM stats)
)
SELECT line_id, lat, lon
FROM pts
WHERE IFNULL(activity, '') <> '{0}'
   OR IFNULL(t - prev_landing_t, 999999.0) > {1}
ORDER BY line_id)",
                                             mxconst::get_STATS_LANDING(),
                                             mxconst::STATS_BOUNCE_WINDOW_SEC);

  std::vector<std::string> vecNames;
  sqlite3_stmt*            stmt = nullptr;
  if (sqlite3_prepare_v2(local_db.db, sql_points.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      const double lat = sqlite3_column_double(stmt, 1);
      const double lon = sqlite3_column_double(stmt, 2);

      std::string name;
      if (lat * lon != 0.0) // get_plane_airport_or_nearest_icao() uses the plane position when lat * lon is 0
      {
        if (const auto navaid = get_plane_airport_or_nearest_icao(false, lat, lon, true); !navaid.getID().empty())
          name = navaid.getID();
      }

      if (name.empty())
        name = fmt::format("nav({:.3f}/{:.3f})", lat, lon);

      vecNames.emplace_back(name);
    }
  }
  else
    Log::logMsgErr(fmt::format("[{}] Failed to prepare the route query: {}", __func__, sqlite3_errmsg(local_db.db)));

  sqlite3_finalize(stmt);

  if (vecNames.empty())
    Log::logMsg(fmt::format("[{}] No stats rows, no route.", __func__));
  else
  {
  // The start point is kept as is. Landings with the same name in a row are collapsed: "YBTL > YBTL > YBTL > YBTL" = "YBTL > YBTL(x3)" (pattern practice)
  std::string route_s = vecNames.front();
  for (size_t i = 1; i < vecNames.size();)
  {
    size_t count = 1;
    while (i + count < vecNames.size() && vecNames[i + count] == vecNames[i])
      ++count;

    if (!route_s.empty())
      route_s += " > ";

    route_s += (count > 1) ? fmt::format("{}(x{})", vecNames[i], count) : vecNames[i];
    i += count;
  }

  stmt = nullptr;
  if (sqlite3_prepare_v2(local_db.db, fmt::format("UPDATE {} SET route_s = ?1 WHERE id = 1", mxconst::get_DB_TABLE_FLIGHT_INFO()).c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_text(stmt, 1, route_s.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_DONE)
      Log::logMsg(fmt::format("[{}] Route: {}", __func__, route_s));
    else
      Log::logMsgErr(fmt::format("[{}] Failed to write the route: {}", __func__, sqlite3_errmsg(local_db.db)));
  }
  else
    Log::logMsgErr(fmt::format("[{}] Failed to prepare the route update: {}", __func__, sqlite3_errmsg(local_db.db)));

  sqlite3_finalize(stmt);
  } // end route

  // v26.10.2 history copy: only flights that reached the end screen
  const bool reached_end_screen = (in_outcome == mxconst::get_FLIGHT_OUTCOME_SUCCESS() || in_outcome == mxconst::get_FLIGHT_OUTCOME_FAILURE() || in_outcome == mxconst::get_FLIGHT_OUTCOME_ABORTED());
  if (reached_end_screen && in_history_max_files > 0)
    stats_history_save_cai(local_db, in_stats_db_path, in_history_max_files);
  else if (reached_end_screen)
    Log::logMsg(fmt::format("[{}] History is off ({} = 0), the flight was not saved.", __func__, mxconst::get_OPT_STATS_HISTORY_MAX_FILES()));
}

// -------------------------------------
// v26.10.2 The setup option, limited to 0..STATS_HISTORY_MAX_FILES_LIMIT. 0 = history off. Main thread (reads the setup node).
int
data_manager::stats_history_get_max_files_cai()
{
  const int value = system_actions::pluginSetupOptions.getNodeText_type_1_5<int>(mxconst::get_OPT_STATS_HISTORY_MAX_FILES(), mxconst::STATS_HISTORY_DEFAULT_FILES);
  return std::clamp(value, 0, mxconst::STATS_HISTORY_MAX_FILES_LIMIT);
}

// -------------------------------------
// v26.10.2 Save the flight into {stats db folder}/history/{stats file name}-{yyyymmdd_hhmmss}.db (OS local time).
//          Once per flight: flight_info.archived goes from 0 to 1 first. If it is already 1, nothing is saved
//          (it is 0 again only after a new mission start, or after a checkpoint load: the checkpoint copy was saved mid-flight).
//          "VACUUM INTO" writes a new, compact database file with all tables, views and indexes. The live file is not changed (except "archived").
//          Runs in the route job (own connection, s_thread_sync_mutex is held).
bool
data_manager::stats_history_save_cai(missionx::dbase &in_live_db, const std::string &in_stats_db_path, const int in_history_max_files)
{
  namespace fs = std::filesystem;

  const auto lmbda_exec = [&](const std::string &in_sql) -> bool {
    char *err_msg = nullptr;
    const int rc  = sqlite3_exec(in_live_db.db, in_sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK)
    {
      Log::logMsgErr(fmt::format("[stats_history_save_cai] SQL failed: {}. SQL: {}", (err_msg != nullptr) ? err_msg : "unknown error", in_sql));
      sqlite3_free(err_msg);
      return false;
    }
    return true;
  };

  // ---- 1. once per flight ----
  if (!lmbda_exec(fmt::format("UPDATE {} SET archived = 1 WHERE id = 1 AND archived = 0", mxconst::get_DB_TABLE_FLIGHT_INFO())))
    return false;

  if (sqlite3_changes(in_live_db.db) == 0)
  {
    Log::logMsg(fmt::format("[{}] This flight is already in the history (or has no {} row). Not saved again.", __func__, mxconst::get_DB_TABLE_FLIGHT_INFO()));
    return false;
  }

  // on any failure below: the flight was not saved, set "archived" back
  const auto lmbda_undo_archived = [&]() { lmbda_exec(fmt::format("UPDATE {} SET archived = 0 WHERE id = 1", mxconst::get_DB_TABLE_FLIGHT_INFO())); };

  // ---- 2. folder ----
  const fs::path  live_path(in_stats_db_path);
  const fs::path  history_folder = live_path.parent_path() / mxconst::get_STATS_HISTORY_FOLDER_NAME();
  std::error_code ec;
  fs::create_directories(history_folder, ec);
  if (ec)
  {
    Log::logMsgErr(fmt::format("[{}] Could not create the history folder '{}': {}", __func__, history_folder.string(), ec.message()));
    lmbda_undo_archived();
    return false;
  }

  // ---- 3. file name: {stats file name}-{yyyymmdd_hhmmss}.db, local OS time. "_2", "_3"... if the name exists ----
  const std::string file_prefix = live_path.filename().string() + "-";

  const std::time_t now = std::time(nullptr);
  std::tm           tm_buf{};
  #if defined(_WIN32)
  localtime_s(&tm_buf, &now);
  #else
  localtime_r(&now, &tm_buf);
  #endif
  char timestamp[32]{};
  std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm_buf);

  fs::path history_file = history_folder / fmt::format("{}{}.db", file_prefix, timestamp);
  for (int n = 2; fs::exists(history_file, ec) && n < 100; ++n)
    history_file = history_folder / fmt::format("{}{}_{}.db", file_prefix, timestamp, n);

  // ---- 4. copy ----
  if (!lmbda_exec("VACUUM INTO " + mxUtils::sql_quote(history_file.string())))
  {
    fs::remove(history_file, ec); // a partial file, if any
    lmbda_undo_archived();
    return false;
  }

  Log::logMsg(fmt::format("[{}] Flight saved: {}", __func__, history_file.string()));

  // ---- 5. keep the newest N ----
  stats_history_prune_cai(history_folder.string(), file_prefix, in_history_max_files);
  return true;
}

// -------------------------------------
// v26.10.2 Keep the newest in_history_max_files history files, delete the older ones.
//          Only files named exactly "{prefix}yyyymmdd_hhmmss.db" or "{prefix}yyyymmdd_hhmmss_N.db" are counted.
//          Other files (example: a renamed copy "{prefix}06.01-bounce-crash.db") are never counted or deleted.
//          The names sort by time, so the sort is by name.
void
data_manager::stats_history_prune_cai(const std::string &in_history_folder, const std::string &in_file_prefix, const int in_history_max_files)
{
  namespace fs = std::filesystem;

  std::vector<std::string> vecNames;
  std::error_code          ec;
  for (const auto &entry : fs::directory_iterator(in_history_folder, ec))
  {
    if (entry.is_regular_file(ec) && stats_history_is_history_file_name_cai(entry.path().filename().string(), in_file_prefix)) // v26.10.2 shared with the stats screen list
      vecNames.emplace_back(entry.path().filename().string());
  }
  if (ec)
  {
    Log::logMsgErr(fmt::format("[{}] Could not read the history folder '{}': {}", __func__, in_history_folder, ec.message()));
    return;
  }

  if (static_cast<int>(vecNames.size()) <= in_history_max_files)
    return;

  // "_N" sorts after the plain name of the same second, so a descending sort keeps it first (newer)
  std::ranges::sort(vecNames, std::greater<>());
  for (size_t i = static_cast<size_t>(in_history_max_files); i < vecNames.size(); ++i)
  {
    const fs::path file = fs::path(in_history_folder) / vecNames[i];
    if (fs::remove(file, ec))
      Log::logMsg(fmt::format("[{}] Removed old history file: {}", __func__, vecNames[i]));
    else
      Log::logMsgErr(fmt::format("[{}] Could not remove '{}': {}", __func__, file.string(), ec.message()));
  }
}

// -------------------------------------
// v26.10.2 The history folder: {folder of the live stats file}/history
std::string
data_manager::get_stats_history_folder_cai()
{
  const std::filesystem::path live_path(get_stats_db_path());
  return (live_path.parent_path() / mxconst::get_STATS_HISTORY_FOLDER_NAME()).string();
}

// -------------------------------------
// v26.10.2 History file names start with the live file name and a dash: "stats_xp.sqlite.db-"
std::string
data_manager::get_stats_history_file_prefix_cai()
{
  return std::filesystem::path(get_stats_db_path()).filename().string() + "-";
}

// -------------------------------------
// v26.10.2 A history file name is exactly "{prefix}yyyymmdd_hhmmss.db" or "{prefix}yyyymmdd_hhmmss_N.db".
//          Used by the cleanup (stats_history_prune_cai) and by the stats screen list, so both see the same files.
//          A renamed copy ("{prefix}06.01-bounce-crash.db") is not a history file.
bool
data_manager::stats_history_is_history_file_name_cai(const std::string &in_file_name, const std::string &in_file_prefix)
{
  constexpr std::string_view ext = ".db";
  if (!in_file_name.starts_with(in_file_prefix) || !in_file_name.ends_with(ext))
    return false;

  const std::string_view middle(in_file_name.data() + in_file_prefix.size(), in_file_name.size() - in_file_prefix.size() - ext.size());
  // yyyymmdd_hhmmss = 15 chars, then optional "_N"
  if (middle.size() < 15)
    return false;
  for (size_t i = 0; i < 15; ++i)
  {
    const bool ok = (i == 8) ? (middle[i] == '_') : (std::isdigit(static_cast<unsigned char>(middle[i])) != 0);
    if (!ok)
      return false;
  }
  if (middle.size() == 15)
    return true;
  if (middle[15] != '_' || middle.size() == 16)
    return false;
  return std::all_of(middle.begin() + 16, middle.end(), [](const char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; });
}

// -------------------------------------
// v26.10.2 The stats screen list: one row for each history file, newest first.
//          Each file is opened read-only for a few small queries: flight_info, the first plane, the landings and the time flew.
//          A file that can not be read is still listed (is_readable = false), so the user sees it.
//          Background thread (std::async): no XPLM calls. Does not use the airports database, so s_thread_sync_mutex is not needed.
void
data_manager::fetch_stats_history_list_cai(std::vector<missionx::mx_stats_history_row>* out_vecRows, mxFetchState_enum* outState, std::string* outStatusMessage)
{
  namespace fs = std::filesystem;

  out_vecRows->clear();
  outStatusMessage->clear();

  const std::string history_folder = get_stats_history_folder_cai();
  const std::string file_prefix    = get_stats_history_file_prefix_cai();

  // ---- the file names, newest first (the names sort by time) ----
  std::vector<std::string> vecFileNames;
  std::error_code          ec;
  if (fs::exists(history_folder, ec))
  {
    for (const auto &entry : fs::directory_iterator(history_folder, ec))
    {
      if (entry.is_regular_file(ec) && stats_history_is_history_file_name_cai(entry.path().filename().string(), file_prefix))
        vecFileNames.emplace_back(entry.path().filename().string());
    }
  }
  if (ec)
  {
    (*outStatusMessage) = fmt::format("Could not read the history folder: {}", ec.message());
    Log::logMsgErr(fmt::format("[{}] Could not read '{}': {}", __func__, history_folder, ec.message()));
    (*outState) = mxFetchState_enum::fetch_ended;
    return;
  }
  std::ranges::sort(vecFileNames, std::greater<>());

  // text column, "" for NULL
  const auto lmbda_column_text = [](sqlite3_stmt* in_stmt, const int in_column) -> std::string {
    const auto* text = sqlite3_column_text(in_stmt, in_column);
    return (text == nullptr) ? std::string() : std::string(reinterpret_cast<const char*>(text));
  };

  // run a query and call in_lmbda_read_row for each row. False when the query fails (example: a missing table in an older file)
  const auto lmbda_query = [](sqlite3* in_db, const std::string &in_sql, const std::function<void(sqlite3_stmt*)> &in_lmbda_read_row) -> bool {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(in_db, in_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
      sqlite3_finalize(stmt);
      return false;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW)
      in_lmbda_read_row(stmt);
    sqlite3_finalize(stmt);
    return true;
  };

  for (const auto &file_name : vecFileNames)
  {
    missionx::mx_stats_history_row row;
    row.file_name = file_name;
    row.file_path = (fs::path(history_folder) / file_name).string();

    // "yyyymmdd_hhmmss" from the file name (OS local time when the flight was saved) -> "yyyy-mm-dd hh:mm"
    const std::string stamp = file_name.substr(file_prefix.size(), 15);
    row.saved_local_time_s  = fmt::format("{}-{}-{} {}:{}", stamp.substr(0, 4), stamp.substr(4, 2), stamp.substr(6, 2), stamp.substr(9, 2), stamp.substr(11, 2));

    sqlite3* db = nullptr;
    if (sqlite3_open_v2(row.file_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
    {
      row.error_s = (db != nullptr) ? sqlite3_errmsg(db) : "could not open the file";
      sqlite3_close(db);
      out_vecRows->emplace_back(row);
      continue;
    }
    sqlite3_busy_timeout(db, 500); // the newest file may still be written by the history save

    lmbda_query(db, "PRAGMA user_version", [&](sqlite3_stmt* stmt) { row.schema_version = sqlite3_column_int(stmt, 0); });

    const bool has_flight_info = lmbda_query(db, fmt::format("SELECT mission_title, route_s, outcome, IFNULL(crashed, 0) FROM {} WHERE id = 1", mxconst::get_DB_TABLE_FLIGHT_INFO()), [&](sqlite3_stmt* stmt) {
      row.mission_title = lmbda_column_text(stmt, 0);
      row.route_s       = lmbda_column_text(stmt, 1);
      row.outcome       = lmbda_column_text(stmt, 2);
      row.crashed       = (sqlite3_column_int(stmt, 3) != 0);
    });

    // the first plane flown, and how many planes
    lmbda_query(db, fmt::format("SELECT IFNULL(NULLIF(acf_desc, ''), acf_file), acf_icao, (SELECT COUNT(*) FROM {0}) FROM {0} ORDER BY line_id, rowid LIMIT 1", mxconst::get_DB_TABLE_PLANE_META()), [&](sqlite3_stmt* stmt) {
      row.plane_s      = lmbda_column_text(stmt, 0);
      row.plane_icao_s = lmbda_column_text(stmt, 1);
      row.planes_count = sqlite3_column_int(stmt, 2);
    });

    const bool has_stats = lmbda_query(db, fmt::format("SELECT COUNT(*) FROM stats WHERE activity = '{}'", mxconst::get_STATS_LANDING()), [&](sqlite3_stmt* stmt) { row.landings_count = sqlite3_column_int(stmt, 0); });

    // same time flew rule as the end screen: the sum of the time between rows, without gaps (pause, time change)
    lmbda_query(db,
                fmt::format("SELECT IFNULL(SUM(dt), 0.0) FROM (SELECT (local_date_days * 86400.0 + local_time_sec) - LAG(local_date_days * 86400.0 + local_time_sec) OVER (ORDER BY line_id) AS dt FROM stats) v1 WHERE dt > 0.0 AND dt <= {}",
                            mxconst::STATS_MAX_TIME_GAP_SEC),
                [&](sqlite3_stmt* stmt) { row.time_flew_sec = sqlite3_column_double(stmt, 0); });

    sqlite3_close(db);

    row.is_readable = has_stats;
    if (!has_stats)
      row.error_s = "no stats table";
    else if (!has_flight_info)
      row.route_s = "(older file, no flight info)";

    out_vecRows->emplace_back(row);
  }

  Log::logMsg(fmt::format("[{}] {} history file(s) in '{}'.", __func__, out_vecRows->size(), history_folder));
  (*outState) = mxFetchState_enum::fetch_ended;
}

// -------------------------------------
// v26.10.2 Wait until the route job finished (if one runs). Main thread.
void
data_manager::stats_meta_wait_for_route_cai()
{
  if (!futStatsRoute.valid())
    return;

  try
  {
    futStatsRoute.get(); // waits, and makes the future empty
  }
  catch (const std::exception& e)
  {
    Log::logMsgErr(fmt::format("[{}] The route job failed: {}", __func__, e.what()));
  }
}

// -------------------------------------

bool
data_manager::db_connect(dbase& db, const std::string& inPathToDbFile)
{

  db.set_db_path_and_file(inPathToDbFile);
  if (db.open_database())
    return true;

  Log::logMsgErr(fmt::format("[{}] Failed to connect to database file: {}", __func__, inPathToDbFile) );

  return false;
}

// -------------------------------------

void
data_manager::db_close_all_databases()
{
  stats_meta_wait_for_route_cai(); // v26.10.2 it uses db_xp_airports and the stats file
  db_xp_airports.close_database();
  close_stats_db(false); // v26.10.1 was: db_stats.close_database(). Stops the writer thread first.
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
data_manager::fetch_nearest_osm_navaid_from_sqlite (missionx::NavAidInfo *inFrom_navaid, missionx::NavAidInfo *out_navaid)
{
  if (inFrom_navaid == nullptr || out_navaid == nullptr)
    return;

  std::lock_guard<std::mutex> lock(s_thread_sync_mutex);

  std::string query = R"(select v1.icao, v1.ap_name, v1.ap_lat, v1.ap_lon, v1.distance
from  (
select mx_calc_distance({1}, {2}, v1.ap_lat , v1.ap_lon, 3440) as distance, v1.icao_id, v1.icao, v1.ap_name, v1.ap_lat, v1.ap_lon
from airports_vu v1
where 1 = 1
and {1} between trunc(v1.ap_lat) - 2.0 and trunc(v1.ap_lat) + 2.0
and {2} between trunc(v1.ap_lon) - 2.0 and trunc(v1.ap_lon) + 2.0
) as v1
where  1= 1
and distance <= 0.53
order by distance asc
limit 1
)";


  if (init_xp_airport_db() && db_xp_airports.db_is_open_and_ready)
  {
    const std::string stmt_uq_name = "nearest_navaid_query_from_db";
    // read query from query.xml file.
    Utils::read_external_sql_query_file(data_manager::mapQueries, mxconst::get_SQLITE_ILS_SQLS());
    if (mxUtils::isElementExists(mapQueries, stmt_uq_name))
    {
      query = mapQueries[stmt_uq_name];
      #ifndef RELEASE
      Log::logMsg(fmt::format ("[{}] Picked query from sql.xml file.", __func__) );
      #endif
    }


    // prepare query with lat/lon
    std::map<int, std::string> mapArgs = { { 1, inFrom_navaid->getLat () }, { 2, inFrom_navaid->getLon () } };
    query                              = mxUtils::format (query, mapArgs);

    #ifndef RELEASE
    Log::logMsg (fmt::format ("[{}] Query:\n{}", __func__, query));
    Log::logMsg (fmt::format ("[{}] Query lat/lon: {}\n", __func__, inFrom_navaid->get_latLon ())); // debug
    #endif

    // add query to "global query" map if it does not exist
    if (!mxUtils::isElementExists(mapQueries, stmt_uq_name)) // add the query to mapQueries
      mapQueries[stmt_uq_name] = query;

    // prepare and execute the statement
    if (db_xp_airports.prepareNewStatement(stmt_uq_name, query))
    {
      assert(data_manager::db_xp_airports.mapStatements[stmt_uq_name] != nullptr && fmt::format("[{}] Could not find the query.", __func__ ).c_str () );
      // There should only be one row
      int seq = 0;
      while (sqlite3_step(db_xp_airports.mapStatements[stmt_uq_name]) == SQLITE_ROW && seq < 1)
      {
        // mx_ils_airport_row_strct row;
        int column_indx = 0;
        out_navaid->setID (std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], column_indx) ) ) );
        column_indx++;
        out_navaid->setName (std::string(reinterpret_cast<const char*>(sqlite3_column_text(db_xp_airports.mapStatements[stmt_uq_name], column_indx) ) ) );
        column_indx++;
        out_navaid->lat = static_cast<float> ( sqlite3_column_double(db_xp_airports.mapStatements[stmt_uq_name], column_indx) );
        column_indx++;
        out_navaid->lon = static_cast<float> ( sqlite3_column_double(db_xp_airports.mapStatements[stmt_uq_name], column_indx) );

        seq++; // force exit
      } // end while

    } // end prepared stmt and data fetching

  } // if init_xp_airport_db() && db_xp_airports.db_is_open_and_ready

}

// -------------------------------------

void
data_manager::fetch_ils_rw_from_sqlite(NavAidInfo* inFrom_navaid, std::string* inFilterQuery, mxFetchState_enum* outState, std::string* outStatusMessage)
{
  outStatusMessage->clear();
  std::lock_guard<std::mutex> lock(s_thread_sync_mutex);

  // v24.03.1 add support for custom "base query" for the ILS search screen
  std::string sFilterQuery = (*inFilterQuery);
  // v24.06.1 added xx_rank column as an additional sorting

  const auto lmbda_get_query = [in_ifr_or_vfr = missionx::data_manager::ui_ifr_or_vfr_i] ()
  {
    if (in_ifr_or_vfr == missionx::PICKED_IFR)
    {
      return R"(select icao, round(distance_nm) as distance_nm, loc_rw, loc_type, frq_mhz, loc_bearing, rw_length_mt, rw_width, ap_elev, ap_name, surf_type_text, bearing_from_to_icao
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
    }

    // VFR Query - PICKED_VFR
    return R"(select icao, round(distance_nm) as distance_nm, '' as loc_rw, '' as loc_type, 0 as frq_mhz, 0 as loc_bearing, rw_length_mt, rw_width, ap_elev, ap_name, surf_type_text, bearing_from_to_icao
from (
select xa.icao
      , mx_calc_distance ({}, {}, xa.ap_lat, xa.ap_lon, 3440) as distance_nm
      , xp_rw.rw_length_mt, xp_rw.rw_width, xa.ap_elev, xa.ap_name
      , case xp_rw.rw_surf when 1 then 'Asphalt' when 2 then 'Concrete' when 3 then 'Turf or grass' when 4 then 'Dirt' when 5 then 'Gravel' when 12 then 'Dry lakebed' when 13 then 'Water runways' when 14 then 'Snow or ice' when 15 then 'Transparent' else 'no data' end as surf_type_text
      , mx_bearing({}, {}, xa.ap_lat, xa.ap_lon) as bearing_from_to_icao
from xp_rw, xp_airports xa
where 1 = 1
and xa.icao_id = xp_rw.icao_id
)
where 1 = 1
)";

  };

  std::string query = lmbda_get_query();

  if (init_xp_airport_db() && db_xp_airports.db_is_open_and_ready)
  {

    //////////////////////////////////////////
    // read custom SQL first from sql.xml file
    //////////////////////////////////////////
    const std::string stmt_base_ils_search_query_name = (data_manager::ui_ifr_or_vfr_i == missionx::PICKED_IFR)? "base_ils_search_query" : "base_vfr_search_query"; // v24.03.1 // v25.08.1
    const std::string stmt_uq_name                    = "ils_search_query";      // v24.03.1 renamed the statement final name.

    // v24.03.1 Reading SQL from sql.xml file
    Utils::read_external_sql_query_file(mapQueries, mxconst::get_SQLITE_ILS_SQLS());
    if (mxUtils::isElementExists(mapQueries, stmt_base_ils_search_query_name))
    {
      query = mapQueries[stmt_base_ils_search_query_name];
      #ifndef RELEASE
      Log::logMsg("Picked query from sql.xml file.");
      #endif
    }

    query += " " + sFilterQuery;  // Construct the full query
    if (!mxUtils::isElementExists(mapQueries, stmt_uq_name)) // add the query to mapQueries
      mapQueries[stmt_uq_name] = query;

    // Replace the BINDINGS
    // v24.03.1 the query will use the fmt::format library to modify the query, instead of using bindings
    std::map<int, std::string> mapArgs = { { 1, inFrom_navaid->getLat() }, { 2, inFrom_navaid->getLon() }, { 3, inFrom_navaid->getLat() }, { 4, inFrom_navaid->getLon() } };
    query                              = mxUtils::format(query, mapArgs);

    #ifndef RELEASE
    Log::logMsg("[ils layer] Query:\n" + query);                                    // debug
    Log::logMsg("[ils layer] Query lat/lon:" + inFrom_navaid->get_latLon() + "\n"); // debug
    #endif

    // prepare query statement
    if (db_xp_airports.prepareNewStatement(stmt_uq_name, query))
    {
      assert(data_manager::db_xp_airports.mapStatements[stmt_uq_name] != nullptr);

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

  (*outState) = mxFetchState_enum::fetch_ended; // once we change the state, the UI can show/hide the relevant layers/widgets
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
      Log::logMsg("Using fetch_nav_data_step1 from sql.xml file");
      #endif
    }
    else
      mapQueries[stmt_uq_name] = query;

    std::map<int, std::string> mapArgs = { { 1, inPlanePos->getLat_s() }, { 2, inPlanePos->getLon_s() }, { 3, sICAO } };
    query                              = mxUtils::format(query, mapArgs);

    #ifndef RELEASE
    Log::logMsg(fmt::format("Nav Data Query 1:\n{}", query));
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
            Log::logMsg("Using fetch_rw_info_step2 from sql.xml file");
            #endif
          }
          else
            mapQueries[stmt_uq_name] = query;

          std::map<int, std::string> map_args = { { 1, inPlanePos->getLat_s() }, { 2, inPlanePos->getLon_s() }, { 3, mxUtils::formatNumber(data.icao_id) } };
          query                               = mxUtils::format(query, map_args);


          #ifndef RELEASE
          Log::logMsg(fmt::format("Nav Data Query 2:\n{}", query));
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
              Log::logMsg("Using fetch_vor_ndb_dme_info_step3 from sql.xml file");
              #endif
            }
            else
              mapQueries[stmt_vor] = query;

            std::map<int, std::string> local_map_args = { { 1, mxUtils::formatNumber(data.ap_lat) },  { 2, mxUtils::formatNumber(data.ap_lon) },  { 3, mxUtils::formatNumber(data.ap_lat) },  { 4, mxUtils::formatNumber(data.ap_lon) }
                                                       ,  { 5, mxUtils::formatNumber(data.ap_lat) },  { 6, mxUtils::formatNumber(data.ap_lon) },  { 7, mxUtils::formatNumber(data.ap_lat) },  { 8, mxUtils::formatNumber(data.ap_lon) }
                                                       ,  { 9, mxUtils::formatNumber(data.ap_lat) },  { 10, mxUtils::formatNumber(data.ap_lon) }, { 11, mxUtils::formatNumber(data.ap_lat) }, { 12, mxUtils::formatNumber(data.ap_lon) }
                                                       ,  { 13, mxUtils::formatNumber(data.ap_lat) }, { 14, mxUtils::formatNumber(data.ap_lon) }, { 15, mxUtils::formatNumber(data.ap_lat) }, { 16, mxUtils::formatNumber(data.ap_lon) }
                                                        };
            query                                     = mxUtils::format(query, local_map_args); // v25.03.3 fixed map name sent to query.


            #ifndef RELEASE
            Log::logMsg(fmt::format("Nav Data Query 3:\n{}", query));
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
              Log::logMsg("Using fetch_loc_info_step4 from sql.xml file");
              #endif
            }
            else
              mapQueries[stmt_loc] = query;

            std::map<int, std::string> map_args2 = { { 1, inPlanePos->getLat_s() }, { 2, inPlanePos->getLon_s() }, { 3, sICAO }
                                                   , { 4, inPlanePos->getLat_s() }, { 5, inPlanePos->getLon_s() }, { 6, sICAO } };
            query                                = mxUtils::format(query, map_args2);


            #ifndef RELEASE
            Log::logMsg(fmt::format("Nav Data Query 4:\n{}", query));
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
              Log::logMsg("Using fetch_ap_frq_step5 from sql.xml file");
              #endif
            }
            else
              mapQueries[stmt_frq] = query;

            std::map<int, std::string> map_args3 = { { 1, mxUtils::formatNumber(data.icao_id) } }; // v24.05.2
            query                              = mxUtils::format(query, map_args3);

            #ifndef RELEASE
            Log::logMsg(fmt::format("Nav Data Query 5:\n{}", query));
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
        postFlcActions.push_back(mx_flc_pre_command::fetch_metar_data_after_nav_info);
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


// -------------------------------------
// v26.10.2 End screen: read the live stats file into last_flight_stats_view.
void
data_manager::fetch_last_mission_stats(mxFetchState_enum* outState, std::string* outStatusMessage)
{
  fetch_mission_stats_cai(get_stats_db_path(), &last_flight_stats_view, outState, outStatusMessage);
}

// -------------------------------------
// v26.10.2 Read one stats file into out_view_data: the stats rows, the summary, the landings and the metadata rows.
//          Opens its own read-only connection. Holds s_thread_sync_mutex (the landing list uses the airports database).
//          Background thread: no XPLM calls, and the mission globals are not changed.
void
data_manager::fetch_mission_stats_cai(const std::string in_stats_db_path, missionx::mx_stats_view_data* out_view_data, mxFetchState_enum* outState, std::string* outStatusMessage)
{

  outStatusMessage->clear();
  std::lock_guard<std::mutex> lock(s_thread_sync_mutex);

  // v26.10.1 own read-only connection. stats_db is closed when the end screen is shown.
  //          The dbase destructor closes it when the function returns.
  //          v26.10.2 any stats file: the live one (end screen) or a history copy (stats screen).
  out_view_data->reset();
  out_view_data->stats_db_path = in_stats_db_path;
  missionx::mx_mission_stats_strct& out_stats = out_view_data->stats; // short name for the many lines below

  missionx::dbase local_stats_db_ro;
  local_stats_db_ro.set_db_path_and_file(in_stats_db_path);
  if (!local_stats_db_ro.open_database_read_only())
  {
    (*outStatusMessage) = "Could not open the stats database.\n" + local_stats_db_ro.last_err;
    Log::logMsgErr(fmt::format("[{}] Could not open '{}': {}", __func__, in_stats_db_path, local_stats_db_ro.last_err));
    (*outState)         = mxFetchState_enum::fetch_ended;
    return;
  }

  // v26.10.2 flight_info, plane_meta and leg_stats, then the scoring the flight was graded with
  fetch_stats_meta_rows_cai(local_stats_db_ro, *out_view_data);
  IXMLNode xScoring = get_scoring_for_stats_view_cai(out_view_data->flight_info);
  out_view_data->flag_scoring_missing = xScoring.isEmpty();

  const std::string query_stats = "select elev, flap_ratio * 100 as flap_ratio, local_date_days, local_time_sec, case when airspeed < 0.0 then 0.0 else airspeed end as airspeed , groundspeed, faxil_gear, roll, IFNULL(activity,'') from stats order by line_id";

  if (local_stats_db_ro.db_is_open_and_ready)
  {
    const std::string stmt_uq_name = "fetch_plane_stats";
    // prepare query statement
    if (local_stats_db_ro.prepareNewStatement(stmt_uq_name, query_stats))
    {
      assert(local_stats_db_ro.mapStatements[stmt_uq_name] != nullptr && fmt::format("[{}] Failed to create stmt: {}", __func__, stmt_uq_name).data());

      int seq = 0;

      std::map<int, missionx::mx_row_stats_strct> map_stats_rows;
      std::map<int, missionx::mx_row_stats_strct> map_activity_stats_rows;
      std::vector <std::string> vec_activity_pacing = {"takeoff", "landing"};

      double elapsed_sec        = 0.0; // v26.10.2 flight clock for the graphs
      double prev_row_clock_sec = 0.0; // v26.10.2

      while (sqlite3_step(local_stats_db_ro.mapStatements[stmt_uq_name]) == SQLITE_ROW)
      {
        missionx::mx_row_stats_strct row;

        int i = 0;
        row.Seq_d = static_cast<double> ( seq );
        out_stats.vecSeq_d.emplace_back( row.Seq_d ); // 0 column line_id (seq) we will use internal numbering so we always start from ZERO

        const auto elev_mt = static_cast<double> ( sqlite3_column_int ( local_stats_db_ro.mapStatements[stmt_uq_name], i ) );
        i++; // column 1 elevation in meters
        const auto elev_ft = elev_mt * meter2feet;
        row.Elev_mt_d = elev_mt;
        row.Elev_ft_d = elev_ft;
        out_stats.vecElev_mt_d.emplace_back(row.Elev_mt_d); // 1 column Elevation meters
        out_stats.vecElev_ft_d.emplace_back(row.Elev_ft_d); // store elevation as feet
        // Min Max
        mxUtils::eval_min_max<double>(elev_ft, out_stats.min_elev_ft, out_stats.max_elev_ft);

        if (seq == 0)
          out_stats.min_elev_ft = out_stats.max_elev_ft = elev_ft;

        row.Flaps_d = static_cast<double> ( sqlite3_column_int ( local_stats_db_ro.mapStatements[stmt_uq_name], i ) );
        out_stats.vecFlaps_d.emplace_back( row.Flaps_d );
        i++; // 2 column flaps
        row.DayInYear_d = static_cast<double> ( sqlite3_column_int ( local_stats_db_ro.mapStatements[stmt_uq_name], i ) );
        out_stats.vecDayInYear_d.emplace_back( row.DayInYear_d );
        i++; // 3 column day in year
        row.LocalTime_sec_d = static_cast<double> ( sqlite3_column_int ( local_stats_db_ro.mapStatements[stmt_uq_name], i ) );
        out_stats.vecLocalTime_sec_d.emplace_back(row.LocalTime_sec_d);

        // v26.10.2 flight clock (minutes) for the graphs: add the time between rows, without gaps (pause, time change). Same rule as "time flew".
        //          The precise time (double): rows written in the same second still get their own time.
        const double row_clock_sec = row.DayInYear_d * 86400.0 + sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name], i);
        if (seq > 0)
        {
          const double dt_sec = row_clock_sec - prev_row_clock_sec;
          if (dt_sec > 0.0 && dt_sec <= mxconst::STATS_MAX_TIME_GAP_SEC)
            elapsed_sec += dt_sec;
        }
        prev_row_clock_sec = row_clock_sec;
        out_stats.vecElapsedMin_d.emplace_back(elapsed_sec / 60.0);

        i++; // 4 column local time in seconds

        const auto air_speed = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name], i);
        i++; // 5 column Air speed
        row.AirSpeed_d = air_speed;
        out_stats.vecAirSpeed_d.emplace_back(row.AirSpeed_d);
        // Min Max
        mxUtils::eval_min_max<double>(air_speed, out_stats.min_airspeed, out_stats.max_airspeed);

        row.GroundSpeed_d = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name], i);
        out_stats.vecGroundSpeed_d.emplace_back(row.GroundSpeed_d);
        i++; // 6 column ground speed
        row.Faxil_d = static_cast<double> ( sqlite3_column_int ( local_stats_db_ro.mapStatements[stmt_uq_name], i ) );
        out_stats.vecFaxil_d.emplace_back(row.Faxil_d);
        i++; // 7 column Faxil
        row.Roll_d = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name], i);
        out_stats.vecRoll_d.emplace_back(row.Roll_d);
        i++; // 8 column roll
        row.Activity_s = std::string(reinterpret_cast<const char*>(sqlite3_column_text(local_stats_db_ro.mapStatements[stmt_uq_name], i)));
        out_stats.vecActivity_s.emplace_back(row.Activity_s);
        i++; // 9. column Activity: takeoff / landing

        seq++;
      }

      // v26.10.1 build the plot series once. ~2 points per bucket, so the graph stays far below 64K vertices
      //          (16-bit ImDrawIdx when IMGWINDOW_USE_PANEL_GRAPHICS is defined) no matter how long the flight was.
      constexpr size_t PLOT_BUCKETS = 1000;
      // Decimate for elevation
      mxUtils::decimate_min_max(out_stats.vecSeq_d, out_stats.vecElev_ft_d,  PLOT_BUCKETS,
                                out_stats.vecPlotElevSeq_d, out_stats.vecPlotElev_ft_d);
      // Decimate for speed
      mxUtils::decimate_min_max(out_stats.vecSeq_d, out_stats.vecAirSpeed_d, PLOT_BUCKETS,
                                out_stats.vecPlotAirSeq_d,  out_stats.vecPlotAirSpeed_d);

      // v26.10.2 the minute of each plotted point. The decimation keeps the row number (seq = index of vecElapsedMin_d)
      const auto lmbda_seq_to_minutes = [&](const std::vector<double> &in_vecSeq, std::vector<double> &out_vecMinutes) {
        out_vecMinutes.clear();
        out_vecMinutes.reserve(in_vecSeq.size());
        for (const double seq_d : in_vecSeq)
        {
          const auto row_index = static_cast<size_t>(seq_d);
          out_vecMinutes.emplace_back((row_index < out_stats.vecElapsedMin_d.size()) ? out_stats.vecElapsedMin_d[row_index] : 0.0);
        }
      };
      lmbda_seq_to_minutes(out_stats.vecPlotElevSeq_d, out_stats.vecPlotElevMin_d);
      lmbda_seq_to_minutes(out_stats.vecPlotAirSeq_d, out_stats.vecPlotAirMin_d);
      out_stats.total_elapsed_min = (out_stats.vecElapsedMin_d.empty()) ? 0.0 : out_stats.vecElapsedMin_d.back();

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
    // v26.10.2 Sum the sim time between consecutive rows. A gap that is negative or longer than STATS_MAX_TIME_GAP_SEC is skipped:
    //          a time-of-day change, or a pause. Rows are written about every second (20 per second in approach mode), so a real gap is small.
    //          Day changes are handled by local_date_days * 86400. Was: max - min of local_time_sec per day, which counted a time change as flight time.
    const std::string query_time_fly = fmt::format("select ifnull(sum(dt), 0.0) as total_mission_time_sec from ( select (local_date_days * 86400.0 + local_time_sec) - lag(local_date_days * 86400.0 + local_time_sec) over (order by line_id) as dt from stats ) v1 where dt > 0.0 and dt <= {}", mxconst::STATS_MAX_TIME_GAP_SEC);
    if (const std::string stmt_uq_name2 = "query_time_flew"; local_stats_db_ro.prepareNewStatement(stmt_uq_name2, query_time_fly))
    {
      assert(local_stats_db_ro.mapStatements[stmt_uq_name2] != nullptr && fmt::format("[{}] Failed to create stmt2: {}", __func__, stmt_uq_name2).data());

      double cumulative_time_flew = 0.0;
      while (sqlite3_step(local_stats_db_ro.mapStatements[stmt_uq_name2]) == SQLITE_ROW)
      {
        const double active_time_in_sec_in_aday_d = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name2], 0);
        cumulative_time_flew += active_time_in_sec_in_aday_d;
      }
      out_stats.time_flew_sec_d    = cumulative_time_flew;
      out_stats.time_flew_format_s = Utils::format_number_as_hours(cumulative_time_flew);

    } // end if "query_time_fly" is valid

    // Read more statistic queries like: max faxil/max roll/max AoA/maybe max G/
    //       Other stats could be: Center landing, airport ICAO plane landed and on which runway.
    const std::string query_min_max_stats = "SELECT v1.min_pitch, v1.max_pitch, v1.min_roll, v1.max_roll, v1.min_gforce_normal, v1.max_gforce_normal, v1.min_gforce_axil, v1.max_gforce_axil "
                                            ", v1.min_airspeed, v1.max_airspeed, v1.min_groundspeed, v1.max_groundspeed, v1.min_vvi_fpm_pilot, v1.max_vvi_fpm_pilot ,v1.min_vh_ind "
                                            ", v1.max_vh_ind, v1.min_aoa, v1.max_aoa FROM stats_summary v1";

    const std::string stmt_uq_name3 = "query_min_max_stats_vu";

    #ifndef RELEASE
    Log::logMsg("query min/max stats:" + query_min_max_stats);
    #endif // !RELEASE


    if (local_stats_db_ro.prepareNewStatement(stmt_uq_name3, query_min_max_stats))
    {
      assert(local_stats_db_ro.mapStatements[stmt_uq_name3] != nullptr && fmt::format("[{}] Failed to create stmt3: {}", __func__, stmt_uq_name3).data());

      if (local_stats_db_ro.step(local_stats_db_ro.mapStatements[stmt_uq_name3]) == SQLITE_ROW) // fetch 1 row
      {
        int iCol                                        = 0;
        out_stats.minPitch = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.maxPitch = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.minRoll = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.maxRoll = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.minGforce = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.maxGforce = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.minFaxilG = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.maxFaxilG = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.minAirspeed = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.maxAirspeed = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.minGroundSpeed = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
        out_stats.maxGroundspeed = sqlite3_column_double(local_stats_db_ro.mapStatements[stmt_uq_name3], iCol);
        iCol++;
      }

      // v3.303.9.1 use default scoring if there is none in the mission file
      // v26.10.2 xScoring comes from get_scoring_for_stats_view_cai() (defaults included). The thread no longer writes the mission scoring global.
      if (!xScoring.isEmpty())
      {
        // pitch
        const auto vecPitchScoring          = Utils::xml_extract_scoring_subElements ( xScoring, mxconst::get_ELEMENT_PITCH() );
        out_stats.pitchScore = Utils::getScoreAfterAnalyzeMinMax ( vecPitchScoring, out_stats.minPitch, out_stats.maxPitch );
        // roll
        const auto vecRollScoring          = Utils::xml_extract_scoring_subElements ( xScoring, mxconst::get_ELEMENT_ROLL() );
        out_stats.rollScore = Utils::getScoreAfterAnalyzeMinMax ( vecRollScoring, out_stats.minRoll, out_stats.maxRoll );
        // gForce
        const auto vecGForceScoring          = Utils::xml_extract_scoring_subElements ( xScoring, mxconst::get_ELEMENT_GFORCE() );
        out_stats.gForceScore = Utils::getScoreAfterAnalyzeMinMax ( vecGForceScoring, out_stats.minGforce, out_stats.maxGforce );
      }
    } // end if "query_min_max_stats" is valid
  }
  else
  {
    (*outStatusMessage) = "Could not work on the stats database.\n" + local_stats_db_ro.last_err;
  }



  ///// v3.0.308.4.3
  out_stats.vecLandingStatsResults = get_landing_stats_as_vec(local_stats_db_ro, xScoring); // v26.10.1 will get landing information + center line score

  // v26.10.1 landing analysis: height steps, touchdown summary, score, grade and notes
  out_stats.mapLandingAnalysis = fetch_landing_analysis(local_stats_db_ro, out_stats.vecLandingStatsResults); // v26.10.2 the landing rows carry the runway of each touchdown
  if (!out_stats.mapLandingAnalysis.empty())
  {
    for (auto &[td_line_id, landing] : out_stats.mapLandingAnalysis)
    {
      score_landing_analysis(landing, xScoring);
      build_landing_findings(landing);
    }

    // #ifndef RELEASE
    // v26.10.1 debug: print the landing analysis to missionx.log
    for (const auto &[td_line_id, landing] : out_stats.mapLandingAnalysis)
    {
      Log::logMsg(fmt::format("\n[{}] Landing line_id: {}, grade: {}, score: {:.1f}, weight: {:.0f}kg, td_fpm: {:.0f}, gear_load: {:.2f} (G {:.2f} - {:.2f}, peak {:.2f}), glide: {:.2f} ({:.0f} fpm), td_bank: {:.1f}, td_pitch: {:.1f}",
                              __func__, td_line_id, landing.grade_s, landing.landing_score, landing.td_weight_kg, landing.td_fpm,
                              landing.td_gear_load, landing.td_g_avg, landing.pre_td_g, landing.td_g_peak,
                              landing.final_max_glide_ratio, landing.final_max_sink_fpm, landing.td_bank, landing.td_pitch));
      if (landing.has_runway_data) // v26.10.2
        Log::logMsg(fmt::format("[{}] Landing line_id: {}, runway {}: touchdown {:.0f} m past the threshold, {:.1f} m from the center line, {:.0f} m left. Overrun: {} ({:.0f} kt, {:.0f} m), excursion: {} ({:.0f} kt, {:.0f} m)",
                                __func__, td_line_id, landing.landing_runway_s, landing.td_from_threshold_m, landing.td_lateral_m, landing.runway_left_m,
                                landing.is_overrun, landing.overrun_gs_kt, landing.overrun_max_m, landing.is_excursion, landing.excursion_gs_kt, landing.excursion_max_m));

      for (const auto &row : landing.vecSteps)
        Log::logMsg(fmt::format("  {:>6} {} agl:{:6.1f}ft ias:{:5.1f} gs:{:5.1f} vs:{:6.0f} pitch:{:5.1f} bank:{:5.1f} g:{:4.2f} flaps:{:4.2f} {:5.1f}s {:6.0f}m",
                                row.step_label, (row.same_row_as_next ? "-" : " "), row.agl_ft, row.ias_kt, row.gs_kt, row.vs_fpm,
                                row.pitch, row.roll, row.g, row.flap_ratio, row.sec_to_td, row.dist_to_td_mt));

      Log::logMsg(fmt::format("  bounce: count {}, max {:.2f} ft, {:.2f} sec, re-contact {:.0f} fpm", landing.bounce_count, landing.max_bounce_agl_ft, landing.max_bounce_sec, landing.worst_recontact_fpm));
      Log::logMsg(fmt::format("  score: base {:.1f}, bounce penalty {:.1f}, final {:.1f}", landing.base_score, landing.bounce_penalty, landing.landing_score));

      for (const auto &note : landing.vecFindings)
        Log::logMsg(fmt::format("  note: {}", note));
    }
    // #endif
  }


  (*outState) = mxFetchState_enum::fetch_ended; // once we change the state, the UI can show/hide the relevant layers/widgets
}

// -------------------------------------
// v26.10.2 Read the metadata rows of a stats file into out_view_data.
//          flight_info (one row), plane_meta (the planes flown) and leg_stats (the finished legs, newest first).
//          A file written before v26.10.2 has no metadata tables, and a schema 1 file has no leg_stats: those parts stay empty.
void
data_manager::fetch_stats_meta_rows_cai(missionx::dbase &in_stats_db, missionx::mx_stats_view_data &out_view_data)
{
  // text column, "" for NULL
  const auto lmbda_column_text = [](sqlite3_stmt* in_stmt, const int in_column) -> std::string {
    const auto* text = sqlite3_column_text(in_stmt, in_column);
    return (text == nullptr) ? std::string() : std::string(reinterpret_cast<const char*>(text));
  };

  // prepare a query. Returns nullptr when the table is missing (an older file). This is logged, it is not an error
  const auto lmbda_prepare = [&](const std::string &in_sql, const std::string &in_table_name) -> sqlite3_stmt* {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(in_stats_db.db, in_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
      Log::logMsg(fmt::format("[{}] No '{}' data in '{}': {}", __func__, in_table_name, out_view_data.stats_db_path, sqlite3_errmsg(in_stats_db.db)));
      sqlite3_finalize(stmt);
      return nullptr;
    }
    return stmt;
  };

  // ---- schema version ----
  if (sqlite3_stmt* stmt = lmbda_prepare("PRAGMA user_version", "user_version"); stmt != nullptr)
  {
    if (sqlite3_step(stmt) == SQLITE_ROW)
      out_view_data.flight_info.schema_version = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
  }

  // ---- flight_info ----
  const std::string sql_flight_info = fmt::format("SELECT flight_id, plugin_version, mission_key, mission_title, os_start_time, os_end_time, outcome, outcome_reason, "
                                                  "IFNULL(crashed, 0), crash_source, route_s, scoring_xml FROM {} WHERE id = 1",
                                                  mxconst::get_DB_TABLE_FLIGHT_INFO());
  if (sqlite3_stmt* stmt = lmbda_prepare(sql_flight_info, mxconst::get_DB_TABLE_FLIGHT_INFO()); stmt != nullptr)
  {
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
      auto &info          = out_view_data.flight_info;
      info.has_row        = true;
      info.flight_id      = lmbda_column_text(stmt, 0);
      info.plugin_version = lmbda_column_text(stmt, 1);
      info.mission_key    = lmbda_column_text(stmt, 2);
      info.mission_title  = lmbda_column_text(stmt, 3);
      info.os_start_time  = lmbda_column_text(stmt, 4);
      info.os_end_time    = lmbda_column_text(stmt, 5);
      info.outcome        = lmbda_column_text(stmt, 6);
      info.outcome_reason = lmbda_column_text(stmt, 7);
      info.crashed        = (sqlite3_column_int(stmt, 8) != 0);
      info.crash_source   = lmbda_column_text(stmt, 9);
      info.route_s        = lmbda_column_text(stmt, 10);
      info.scoring_xml    = lmbda_column_text(stmt, 11);
    }
    sqlite3_finalize(stmt);
  }

  // ---- plane_meta ----
  const std::string sql_planes = fmt::format("SELECT IFNULL(line_id, 0), acf_file, acf_icao, acf_desc, category, IFNULL(is_helicopter, 0), "
                                             "IFNULL(empty_kg, 0.0), IFNULL(max_kg, 0.0), IFNULL(max_fuel_kg, 0.0) FROM {} ORDER BY line_id, rowid",
                                             mxconst::get_DB_TABLE_PLANE_META());
  if (sqlite3_stmt* stmt = lmbda_prepare(sql_planes, mxconst::get_DB_TABLE_PLANE_META()); stmt != nullptr)
  {
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      missionx::mx_stats_plane_info plane;
      plane.line_id       = sqlite3_column_int(stmt, 0);
      plane.acf_file      = lmbda_column_text(stmt, 1);
      plane.acf_icao      = lmbda_column_text(stmt, 2);
      plane.acf_desc      = lmbda_column_text(stmt, 3);
      plane.category      = lmbda_column_text(stmt, 4);
      plane.is_helicopter = (sqlite3_column_int(stmt, 5) == 1);
      plane.empty_kg      = static_cast<float>(sqlite3_column_double(stmt, 6));
      plane.max_kg        = static_cast<float>(sqlite3_column_double(stmt, 7));
      plane.max_fuel_kg   = static_cast<float>(sqlite3_column_double(stmt, 8));
      out_view_data.vecPlanes.emplace_back(plane);
    }
    sqlite3_finalize(stmt);
  }

  // ---- leg_stats (newest first) ----
  const std::string sql_legs = fmt::format("SELECT IFNULL(leg_name, ''), IFNULL(start_line_id, 0), IFNULL(end_line_id, 0), IFNULL(distance_before_nm, 0.0), IFNULL(distance_nm, 0.0), "
                                           "IFNULL(start_fuel_kg, 0.0), IFNULL(end_fuel_kg, 0.0), IFNULL(start_payload_kg, 0.0), IFNULL(end_payload_kg, 0.0), leg_seq, IFNULL(status, '') "
                                           "FROM {} ORDER BY leg_seq DESC",
                                           mxconst::get_DB_TABLE_LEG_STATS());
  if (sqlite3_stmt* stmt = lmbda_prepare(sql_legs, mxconst::get_DB_TABLE_LEG_STATS()); stmt != nullptr)
  {
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      missionx::mx_enrout_stats_strct leg;
      leg.init();
      leg.sLegName                                 = lmbda_column_text(stmt, 0);
      leg.iStartLineId                             = sqlite3_column_int(stmt, 1);
      leg.iEndLineId                               = sqlite3_column_int(stmt, 2);
      leg.fCumulativeDistanceFlew_beforeCurrentLeg = static_cast<float>(sqlite3_column_double(stmt, 3));
      leg.fDistanceFlew                            = static_cast<float>(sqlite3_column_double(stmt, 4));
      leg.fStartingFuel                            = static_cast<float>(sqlite3_column_double(stmt, 5));
      leg.fEndFuel                                 = static_cast<float>(sqlite3_column_double(stmt, 6));
      leg.fStartingPayload                         = static_cast<float>(sqlite3_column_double(stmt, 7));
      leg.fEndPayload                              = static_cast<float>(sqlite3_column_double(stmt, 8));
      leg.iLegSeq                                  = sqlite3_column_int(stmt, 9);
      leg.sStatus                                  = lmbda_column_text(stmt, 10);
      out_view_data.vecLegs.emplace_back(leg);
    }
    sqlite3_finalize(stmt);
  }

  Log::logMsg(fmt::format("[{}] '{}': schema {}, flight_info: {}, outcome: '{}', planes: {}, legs: {}", __func__, out_view_data.stats_db_path, out_view_data.flight_info.schema_version,
                          (out_view_data.flight_info.has_row ? "yes" : "no"), out_view_data.flight_info.outcome, out_view_data.vecPlanes.size(), out_view_data.vecLegs.size()));
}

// -------------------------------------
// v26.10.2 The waypoint list of the flight path plots: Departure, one entry per leg (leg_stats), Full flight.
//          Main thread, reads only the view data that fetch_mission_stats_cai() filled.
std::vector<missionx::mx_path_section>
data_manager::build_flight_path_sections_cai(const missionx::mx_stats_view_data& in_view_data)
{
  std::vector<missionx::mx_path_section> vecSections;

  // the airport of a takeoff or landing row (landing table), "" when unknown
  const auto lmbda_icao_of_activity_in_range = [&in_view_data](const std::string& in_activity, const int in_from_line_id, const int in_to_line_id, const bool in_pick_last) -> std::string {
    std::string icao_found;
    for (const auto& landing_row : in_view_data.stats.vecLandingStatsResults)
    {
      const bool is_in_range = landing_row.line_id >= in_from_line_id && (in_to_line_id <= 0 || landing_row.line_id <= in_to_line_id);
      if (!is_in_range || landing_row.activity_s != in_activity || landing_row.icao.empty() || landing_row.icao == "N/A")
        continue;

      icao_found = landing_row.icao;
      if (!in_pick_last)
        break;
    }
    return icao_found;
  };

  // leg_stats rows are newest first, the list is in the order flown
  std::vector<missionx::mx_enrout_stats_strct> vecLegsInFlightOrder(in_view_data.vecLegs.rbegin(), in_view_data.vecLegs.rend());

  // Departure
  {
    missionx::mx_path_section departure;
    departure.type          = missionx::mx_path_section::type_enum::departure;
    departure.start_line_id = (vecLegsInFlightOrder.empty()) ? 0 : vecLegsInFlightOrder.front().iStartLineId;
    departure.end_line_id   = (vecLegsInFlightOrder.empty()) ? 0 : vecLegsInFlightOrder.front().iEndLineId;
    const std::string icao  = lmbda_icao_of_activity_in_range("takeoff", 0, 0, false);
    departure.label         = (icao.empty()) ? std::string("Departure") : fmt::format("Departure - {}", icao);
    vecSections.emplace_back(departure);
  }

  // one entry per leg
  for (const auto& leg : vecLegsInFlightOrder)
  {
    missionx::mx_path_section leg_section;
    leg_section.type          = missionx::mx_path_section::type_enum::leg;
    leg_section.leg_seq       = leg.iLegSeq;
    leg_section.leg_status    = leg.sStatus;
    leg_section.start_line_id = leg.iStartLineId;
    leg_section.end_line_id   = leg.iEndLineId; // 0 for a running leg or a crash: up to the last row

    const std::string icao  = lmbda_icao_of_activity_in_range("landing", leg.iStartLineId, leg.iEndLineId, true);
    const std::string where = (icao.empty()) ? leg.sLegName : icao;
    leg_section.label       = fmt::format("Leg {} - {}{}", leg.iLegSeq, where, (leg_section.is_skipped_cai()) ? " (skipped)" : "");
    vecSections.emplace_back(leg_section);
  }

  // Full flight
  {
    missionx::mx_path_section full_flight;
    full_flight.type  = missionx::mx_path_section::type_enum::full_flight;
    full_flight.label = "Full flight";
    vecSections.emplace_back(full_flight);
  }

  return vecSections;
}

// -------------------------------------
// FLIGHT_PATH_WINDOW_BEGIN (the standalone test extracts the code between the markers)
// v26.10.2 Pick the rows of one (section, range), thin them to STATS_PATH_MAX_POINTS and convert them to plot units.
//          No database and no XPLM calls: rows of one section, ordered by line_id.
//          Arrival (leg):  anchor = the last "landing" row. "up to N nm" = N nm of flown path before it, then the rollout.
//          Departure:      anchor = the first "takeoff" row. "up to N nm" = the takeoff roll, then N nm of flown path after it.
//          Distances are along the flown path. A reposition (a jump longer than STATS_PATH_REPOSITION_NM) adds no distance and starts a new segment.
missionx::mx_flight_path_data
data_manager::compute_flight_path_window_cai(const std::vector<missionx::mx_flight_path_row>& in_rows, const missionx::mx_path_section& in_section, const missionx::mx_path_range_enum in_range)
{
  missionx::mx_flight_path_data path;
  path.rows_read = static_cast<int>(in_rows.size());

  const size_t rows_count = in_rows.size();
  if (rows_count == 0)
    return path;

  // along-path distance (nm) from the first row, and where the repositions are
  std::vector<double> vecCumulative_nm(rows_count, 0.0);
  std::vector<bool>   vecJumpBefore(rows_count, false);
  for (size_t i = 1; i < rows_count; ++i)
  {
    const double step_nm = mxUtils::mxCalcDistanceBetween2Points(in_rows[i - 1].lat, in_rows[i - 1].lon, in_rows[i].lat, in_rows[i].lon);
    vecJumpBefore[i]     = step_nm > mxconst::STATS_PATH_REPOSITION_NM;
    vecCumulative_nm[i]  = vecCumulative_nm[i - 1] + ((vecJumpBefore[i]) ? 0.0 : step_nm);
  }

  const bool is_departure = in_section.type == missionx::mx_path_section::type_enum::departure;
  const bool is_leg       = in_section.type == missionx::mx_path_section::type_enum::leg;

  // anchor: first takeoff (departure) or the last touchdown (leg)
  int    anchor                 = -1;
  double last_landing_clock_sec = -1.0;
  for (size_t i = 0; i < rows_count; ++i)
  {
    if (is_departure && in_rows[i].activity == "takeoff")
    {
      anchor = static_cast<int>(i);
      break;
    }
    if (is_leg && in_rows[i].activity == "landing")
    {
      // keep the last touchdown, but not a bounce: a landing less than STATS_BOUNCE_WINDOW_SEC after the previous one belongs to it
      const bool is_bounce = anchor >= 0 && Utils::is_landing_a_bounce_cai (last_landing_clock_sec, in_rows[i].clock_sec);
      last_landing_clock_sec = in_rows[i].clock_sec;
      if (!is_bounce)
        anchor = static_cast<int>(i);
    }
  }

  const double range_nm = (in_range == missionx::mx_path_range_enum::up_to_5nm) ? 5.0 : 20.0; // v26.10.2 was 2 nm: too short, only the end of the final
  const bool   is_whole = in_section.type == missionx::mx_path_section::type_enum::full_flight || (in_range == missionx::mx_path_range_enum::whole_leg && !is_departure);

  size_t first = 0;
  size_t last  = rows_count - 1;
  size_t ref   = (anchor >= 0) ? static_cast<size_t>(anchor) : 0; // the point distances are measured from

  if (is_whole)
  {
    path.shown_nm    = vecCumulative_nm[last] - vecCumulative_nm[first];
    path.range_label = (in_section.type == missionx::mx_path_section::type_enum::full_flight) ? fmt::format("full flight ({:.1f} nm)", path.shown_nm) : fmt::format("whole leg ({:.1f} nm)", path.shown_nm);
  }
  else if (is_departure)
  {
    // the takeoff roll: back from the takeoff, at most STATS_PATH_TAKEOFF_ROLL_NM, stop where the plane stood still
    first = ref;
    while (first > 0 && (vecCumulative_nm[ref] - vecCumulative_nm[first - 1]) <= mxconst::STATS_PATH_TAKEOFF_ROLL_NM && !vecJumpBefore[first])
    {
      --first;
      if (in_rows[first].groundspeed_ms < mxconst::STATS_PATH_STOPPED_GS_MS)
        break;
    }
    // range_nm after the takeoff
    last = ref;
    while (last + 1 < rows_count && (vecCumulative_nm[last + 1] - vecCumulative_nm[ref]) <= range_nm)
      ++last;

    path.shown_nm           = vecCumulative_nm[last] - vecCumulative_nm[ref];
    path.flag_range_clipped = (last == rows_count - 1) && path.shown_nm < range_nm;
  }
  else // arrival
  {
    if (anchor < 0)
      ref = rows_count - 1; // no landing in the leg (hover, overfly): count back from the leg end

    first = ref;
    while (first > 0 && (vecCumulative_nm[ref] - vecCumulative_nm[first - 1]) <= range_nm)
      --first;

    // after the landing: the rollout, until the plane slowed down (or the leg ended)
    last = ref;
    if (anchor >= 0)
    {
      while (last + 1 < rows_count && in_rows[last].groundspeed_ms >= mxconst::STATS_PATH_ROLLOUT_END_GS_MS)
        ++last;
    }

    path.shown_nm           = vecCumulative_nm[ref] - vecCumulative_nm[first];
    path.flag_range_clipped = (first == 0) && path.shown_nm < range_nm;
  }

  if (!is_whole)
  {
    const std::string section_word = (is_departure) ? "section" : "leg";
    path.range_label = (path.flag_range_clipped) ? fmt::format("up to {:.0f} nm ({} is {:.1f} nm)", range_nm, section_word, path.shown_nm) : fmt::format("up to {:.0f} nm", range_nm);
  }

  // thin: keep the highest and lowest AGL of each bucket (bounces and the flare stay visible), and the anchor
  std::vector<double> vecRowIndex, vecAgl;
  vecRowIndex.reserve(last - first + 1);
  vecAgl.reserve(last - first + 1);
  for (size_t i = first; i <= last; ++i)
  {
    vecRowIndex.emplace_back(static_cast<double>(i));
    vecAgl.emplace_back(in_rows[i].agl_mt);
  }

  std::vector<double> vecKeptIndex, vecKeptAgl;
  const size_t        buckets = static_cast<size_t>(mxconst::STATS_PATH_MAX_POINTS / 2 - 1); // decimate_min_max keeps up to 2 points per bucket + the first and last
  mxUtils::decimate_min_max(vecRowIndex, vecAgl, buckets, vecKeptIndex, vecKeptAgl);

  std::vector<size_t> vecKept;
  vecKept.reserve(vecKeptIndex.size() + 2);
  for (const double index_d : vecKeptIndex)
    vecKept.emplace_back(static_cast<size_t>(index_d));
  if (vecKept.empty() || vecKept.back() != last)
    vecKept.emplace_back(last);
  if (anchor >= 0 && static_cast<size_t>(anchor) >= first && static_cast<size_t>(anchor) <= last && !std::ranges::binary_search(vecKept, static_cast<size_t>(anchor)))
    vecKept.insert(std::ranges::upper_bound(vecKept, static_cast<size_t>(anchor)), static_cast<size_t>(anchor));

  // flight clock of the window (minutes), same gap rule as "Time Flew"
  std::vector<double> vecWindowMinutes(last - first + 1, 0.0);
  for (size_t i = first + 1; i <= last; ++i)
  {
    const double dt_sec                = in_rows[i].clock_sec - in_rows[i - 1].clock_sec;
    const double add_sec               = (dt_sec > 0.0 && dt_sec <= mxconst::STATS_MAX_TIME_GAP_SEC) ? dt_sec : 0.0;
    vecWindowMinutes[i - first] = vecWindowMinutes[i - 1 - first] + add_sec / 60.0;
  }

  // plot units: feet east/north of the origin (anchor, else the first point of the window)
  const size_t origin    = (anchor >= 0 && static_cast<size_t>(anchor) >= first && static_cast<size_t>(anchor) <= last) ? static_cast<size_t>(anchor) : first;
  const double origin_nm = vecCumulative_nm[(anchor >= 0) ? ref : origin];

  path.origin_lat    = in_rows[origin].lat;
  path.origin_lon    = in_rows[origin].lon;
  path.origin_alt_ft = in_rows[origin].elev_mt * missionx::meter2feet;

  size_t previous_kept = first;
  for (const size_t i : vecKept)
  {
    const double distance_ft = mxUtils::mxCalcDistanceBetween2Points(in_rows[origin].lat, in_rows[origin].lon, in_rows[i].lat, in_rows[i].lon, missionx::mx_units_of_measure::ft);
    const double bearing_rad = mxUtils::mxCalcBearingBetween2Points(in_rows[origin].lat, in_rows[origin].lon, in_rows[i].lat, in_rows[i].lon) * missionx::PI / 180.0;

    // a reposition between the previous kept point and this one starts a new segment
    bool has_jump = path.vecEast_ft.empty();
    for (size_t raw = previous_kept + 1; raw <= i && !has_jump; ++raw)
      has_jump = vecJumpBefore[raw];
    if (has_jump)
      path.vecSegmentStart.emplace_back(path.vecEast_ft.size());

    if (anchor >= 0 && i == static_cast<size_t>(anchor))
    {
      path.anchor_index   = static_cast<int>(path.vecEast_ft.size());
      path.anchor_line_id = in_rows[i].line_id;
    }

    path.vecEast_ft.emplace_back(distance_ft * std::sin(bearing_rad));
    path.vecNorth_ft.emplace_back(distance_ft * std::cos(bearing_rad));
    path.vecAlt_ft.emplace_back(in_rows[i].elev_mt * missionx::meter2feet);
    path.vecAgl_ft.emplace_back(in_rows[i].agl_mt * missionx::meter2feet);
    path.vecAirspeed_kt.emplace_back(in_rows[i].airspeed_kt);
    path.vecDist_nm.emplace_back(vecCumulative_nm[i] - origin_nm);
    path.vecMinutes.emplace_back(vecWindowMinutes[i - first]);

    previous_kept = i;
  }

  return path;
}
// FLIGHT_PATH_WINDOW_END

// -------------------------------------
// v26.10.2 Background thread (std::async): read the rows of one section from a stats file and build its plot data.
//          Own read-only connection, no XPLM calls, no shared state: each (section, range) request owns its out_path, so requests never overwrite each other.
void
data_manager::fetch_flight_path_section_cai(const std::string in_stats_db_path, const missionx::mx_path_section in_section, const missionx::mx_path_range_enum in_range, missionx::mx_flight_path_data* out_path, mxFetchState_enum* out_state,
                                        std::string* out_status_message)
{
  out_status_message->clear();
  *out_path = missionx::mx_flight_path_data();

  missionx::dbase local_stats_db_ro;
  local_stats_db_ro.set_db_path_and_file(in_stats_db_path);
  if (!local_stats_db_ro.open_database_read_only())
  {
    (*out_status_message) = "Could not open the stats database.\n" + local_stats_db_ro.last_err;
    Log::logMsgErr(fmt::format("[{}] Could not open '{}': {}", __func__, in_stats_db_path, local_stats_db_ro.last_err));
    (*out_state) = mxFetchState_enum::fetch_ended;
    return;
  }

  // the rows of the section. end_line_id 0 = up to the last row
  std::string where_clause = fmt::format("where line_id >= {}", in_section.start_line_id);
  if (in_section.end_line_id > 0)
    where_clause += fmt::format(" and line_id <= {}", in_section.end_line_id);

  const std::string query_path = fmt::format("select line_id, lat, lon, elev, agl, case when airspeed < 0.0 then 0.0 else airspeed end, groundspeed, local_date_days * 86400.0 + local_time_sec, IFNULL(activity, '') "
                                             "from stats {} order by line_id",
                                             where_clause);

  std::vector<missionx::mx_flight_path_row> vecRows;
  if (const std::string stmt_uq_name = "fetch_flight_path_section_cai"; local_stats_db_ro.prepareNewStatement(stmt_uq_name, query_path))
  {
    sqlite3_stmt* stmt = local_stats_db_ro.mapStatements[stmt_uq_name];
    while (stmt != nullptr && sqlite3_step(stmt) == SQLITE_ROW)
    {
      missionx::mx_flight_path_row row;
      row.line_id        = sqlite3_column_int(stmt, 0);
      row.lat            = sqlite3_column_double(stmt, 1);
      row.lon            = sqlite3_column_double(stmt, 2);
      row.elev_mt        = sqlite3_column_double(stmt, 3);
      row.agl_mt         = sqlite3_column_double(stmt, 4);
      row.airspeed_kt    = sqlite3_column_double(stmt, 5);
      row.groundspeed_ms = sqlite3_column_double(stmt, 6);
      row.clock_sec      = sqlite3_column_double(stmt, 7);
      row.activity       = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
      vecRows.emplace_back(row);
    }
  }
  else
  {
    (*out_status_message) = "Failed to prepare the flight path statement. Notify developer !!!";
    (*out_state)          = mxFetchState_enum::fetch_ended;
    return;
  }

  *out_path = compute_flight_path_window_cai(vecRows, in_section, in_range);

  (*out_status_message) = (out_path->empty_cai()) ? std::string("No flight path rows found.") : fmt::format("{} points from {} rows.", out_path->vecEast_ft.size(), out_path->rows_read);
  Log::logMsg(fmt::format("[{}] '{}' {}: {}", __func__, in_section.label, out_path->range_label, *out_status_message));
  (*out_state) = mxFetchState_enum::fetch_ended;
}

// -------------------------------------
// v26.10.2 The <scoring> to grade a stats file with.
//          flight_info.scoring_xml: the mission scoring with the landing defaults, stored when the flight started. A history flight is graded the way it was flown.
//          A file without it (written before v26.10.2): get_scoring_with_landing_defaults(), the current mission scoring or the defaults.
IXMLNode
data_manager::get_scoring_for_stats_view_cai(const missionx::mx_stats_flight_info &in_flight_info)
{
  if (!in_flight_info.scoring_xml.empty())
  {
    IXMLResults   parse_result;
    IXMLDomParser xml_parser;
    auto          xScoring = xml_parser.parseString(in_flight_info.scoring_xml.c_str(), mxconst::get_ELEMENT_SCORING().c_str(), &parse_result);
    if (parse_result.errorCode == IXMLError_None && !xScoring.isEmpty())
      return xScoring.deepCopy();

    Log::logMsgErr(fmt::format("[{}] Could not read the stored <scoring> (error code: {}). Using the default scoring.", __func__, static_cast<int>(parse_result.errorCode)));
  }

  return get_scoring_with_landing_defaults();
}


// -------------------------------------


std::vector<mx_stats_data>
data_manager::get_landing_stats_as_vec(missionx::dbase &in_stats_db, IXMLNode in_xScoring)
{
  std::vector<mx_stats_data> vecActivity; // store stmt01 data

  // Fetch time passes between activities. The first row will always be 0
  // The query will convert days into seconds to have a correct delta result between "current - previous" fields
  // columns return:  line_id, plane_lat, plane_lon, elev_mt, activity and time_passed (between activities)

  // Gather pacing info
  const std::string stmt01 = R"(select line_id, lat as plane_lat, lon as plane_lon, activity
, local_date_days, local_time_sec
, case when airspeed < 0.0 then 0.0 else airspeed end as airspeed
, groundspeed, roll
, IFNULL( mx_calc_distance( lat, lon, IFNULL(lag(lat) over (order by line_id), lat), IFNULL(lag(lon) over (order by line_id), lon), 6370880), 0.0) as dist_diff
, CASE WHEN local_date_days >= IFNULL(lag (local_date_days) over (order by line_id), -1)
       THEN IFNULL( (local_date_days * 86400 + local_time_sec) - (lag (local_date_days) over (order by line_id) * 86400 + lag (local_time_sec) over (order by line_id)), 0.0)
            ELSE IFNULL( ( (local_date_days + 365) * 86400 + local_time_sec) - (lag (local_date_days) over (order by line_id) * 86400 + lag (local_time_sec) over (order by line_id)), 0.0)
       END as diff_time
, date_time as os_date_time
, IFNULL ( ( julianday(date_time) - julianday( lag (date_time) over (order by line_id) ) ) * 86400.0, 0.0) as os_date_time_pacing
, IFNULL ( ( julianday( lead (date_time) over (order by line_id) ) - julianday(date_time) ) * 86400.0, 0.0) as lead_os_date_time_pacing
from stats
where 1 = 1
and activity is not null
and activity in ('takeoff', 'landing')
order by line_id
)";

  #ifndef RELEASE
  Log::logMsg("\nstmt01: " + stmt01); // debug
  #endif // !RELEASE

  const std::string unique_name_s01 = "fetch_activity_stats";

  const auto vecCenterLineScores = Utils::xml_extract_scoring_subElements(in_xScoring, mxconst::get_ELEMENT_CENTER_LINE()); // v26.10.2 was: the mission scoring global


  // Q1: gather activity data from stats db
  if (in_stats_db.prepareNewStatement(unique_name_s01, stmt01, false))
  {
    std::map<int, mx_stats_data> map_landing_data;

    // Loop over all activities that are "landing" and gather airport and runway information (drill down)
    bool b_found_bounce = false;
    int index = 1;
    while (in_stats_db.step(in_stats_db.mapStatements[unique_name_s01]) == SQLITE_ROW) // fetch 1 row))
    {
      mx_stats_data data;
      int           iCol01 = 0;

      data.line_id = sqlite3_column_int(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.plane.lat = sqlite3_column_double(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.plane.lon = sqlite3_column_double(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.activity_s = mxUtils::stringToLower(std::string(reinterpret_cast<const char*>(sqlite3_column_text(in_stats_db.mapStatements[unique_name_s01], iCol01)))); // takeoff or landing
      iCol01++;
      // v25.09.2
      data.local_date_days = sqlite3_column_int(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.local_time_sec = sqlite3_column_double(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.airspeed = sqlite3_column_double(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.groundspeed = sqlite3_column_double(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.roll = sqlite3_column_double(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.diff_distance_meters = sqlite3_column_double(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.diff_time_sec = sqlite3_column_double(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.os_date_fmt = mxUtils::stringToLower(std::string(reinterpret_cast<const char*>(sqlite3_column_text(in_stats_db.mapStatements[unique_name_s01], iCol01)))); // takeoff or landing
      iCol01++;
      data.os_date_fmt_time_pacing = sqlite3_column_double(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;
      data.lead_os_date_time_pacing = sqlite3_column_double(in_stats_db.mapStatements[unique_name_s01], iCol01);
      iCol01++;

      data.bInitialized = true;

      map_landing_data[index] = data;
      index++;
    } // end Q1 - gather landing data

    mx_stats_data *bounce_data_pointer = nullptr;
    double         last_landing_clock_sec = -1.0; // v26.10.2 time of the last "landing" row (days * 86400 + sec)
    for (auto& [seq, data] : map_landing_data)
    {
      // v26.10.2 A long bounce can write a second "landing" row without a "takeoff" row between them (the plane was not airborne long enough for a takeoff).
      //          Same rule as the <stats_sqls> landing queries: a landing less than STATS_BOUNCE_WINDOW_SEC after the previous landing is a bounce of it.
      if (mxconst::get_STATS_LANDING() == data.activity_s)
      {
        const double landing_clock_sec = data.local_date_days * 86400.0 + data.local_time_sec;
        const bool   is_bounce         = Utils::is_landing_a_bounce_cai (last_landing_clock_sec, landing_clock_sec);
        last_landing_clock_sec         = landing_clock_sec;

        if (is_bounce && !vecActivity.empty () && mxconst::get_STATS_LANDING() == vecActivity.back ().activity_s)
        {
          auto &touchdown                        = vecActivity.back ();
          touchdown.did_landing_contained_bounce = true;
          touchdown.score_centerLine             = 0.0;
          touchdown.bounce += 1;
          Log::logMsg (fmt::format ("[{}] Landing row {} is a bounce of the landing at row {}.", __func__, data.line_id, touchdown.line_id));
          continue;
        }
      }

      // BOUNCE DETECTION
      if ( b_found_bounce || mxconst::get_STATS_TAKEOFF() == data.activity_s )
      {
        // detect bounce
        if ( mxUtils::isElementExists (map_landing_data, seq - 1) && data.diff_time_sec < 60.0  )
        {
          b_found_bounce = true;
          data.bounce = 1;

          if ( mxconst::get_STATS_TAKEOFF() == data.activity_s )
          {
            if (!vecActivity.empty ())
            {
              data.icao = vecActivity.back ().icao;
              data.runway_s = vecActivity.back ().runway_s;
            }
            vecActivity.emplace_back(data);
          }

          if (bounce_data_pointer == nullptr && !vecActivity.empty () && vecActivity.size () > 1) // one time until next reset
            bounce_data_pointer =  &vecActivity[vecActivity.size () - 2];

          if (bounce_data_pointer)
          {
            bounce_data_pointer->score_centerLine=0.0;
            bounce_data_pointer->did_landing_contained_bounce=true;
            // add bounce only if lead > 0 and <= 60 seconds
            if ( mxUtils::mx_between <double>( data.lead_os_date_time_pacing,0.0, 60.0, enums::mx_between_types::gt_min_eqles_max) )
              bounce_data_pointer->bounce += 1;
          }

          // skip only if it is not a landing activity. Landing activity should gather the data too
          if ( mxconst::get_STATS_LANDING() != data.activity_s )
            continue; // skip the line that is part of the bounce
        }
        else
        {
          // reset state
          b_found_bounce = false;
          bounce_data_pointer = nullptr;
        }
      }

      if (mxconst::get_STATS_LANDING() == data.activity_s) // landing stats ?
      {
        assert(data_manager::db_xp_airports.db && "db_xp_airports was not initialized correctly. Notify developer or try to restart X-Plane.");

        // Q2: fetch the airport (icao_id) the plane landed in
        {
          if (auto navaid = get_plane_airport_or_nearest_icao(false, data.plane.lat, data.plane.lon, true);
              !navaid.getID().empty())
          {
            data.icao    = navaid.getID();
            data.icao_id = navaid.icao_id;
          }
        }

        // -- Q3 fetch distance from the center of the rw, if plane landed
        // -- Neads plane position.
        // -- We added a special case where there are no boundary, we will only use ICAO.
        // -- This should be ok because even in duplicate ICAO cases, the query won't retrieve runways that the plane is not inside their boundary
        // v26.10.2 also both runway ends, the displaced thresholds and the width: the landing runway check (data_manager::analyse_landing_on_runway_cai())
        std::string stmt03 = R"(select xp_rw.rw_no_1 || '-' || xp_rw.rw_no_2 as runway
, mx_get_shortest_distance_to_rw_vector ( {}, {}, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon) as distance_from_center
, mx_is_plane_in_rw_area( {}, {}, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width) as is_plane_on_rw
, xp_rw.rw_no_1, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, IFNULL(xp_rw.rw_no_1_disp_hold, 0.0) as rw_no_1_disp_hold
, xp_rw.rw_no_2, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, IFNULL(xp_rw.rw_no_2_disp_hold, 0.0) as rw_no_2_disp_hold
, xp_rw.rw_width
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

        // Q3 fetch touchdown relative to the center of the runway data
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

            // v26.10.2 both runway ends (columns read by their alias name)
            sqlite3_stmt *stmt03_ptr = db_xp_airports.mapStatements[unique_name_s03];
            const auto    columns03  = dbase::get_column_indexes(stmt03_ptr);
            data.runway.end_1_name        = dbase::read_column_text(stmt03_ptr, columns03, "rw_no_1");
            data.runway.end_1_lat         = dbase::read_column_double(stmt03_ptr, columns03, "rw_no_1_lat");
            data.runway.end_1_lon         = dbase::read_column_double(stmt03_ptr, columns03, "rw_no_1_lon");
            data.runway.end_1_displaced_m = dbase::read_column_double(stmt03_ptr, columns03, "rw_no_1_disp_hold");
            data.runway.end_2_name        = dbase::read_column_text(stmt03_ptr, columns03, "rw_no_2");
            data.runway.end_2_lat         = dbase::read_column_double(stmt03_ptr, columns03, "rw_no_2_lat");
            data.runway.end_2_lon         = dbase::read_column_double(stmt03_ptr, columns03, "rw_no_2_lon");
            data.runway.end_2_displaced_m = dbase::read_column_double(stmt03_ptr, columns03, "rw_no_2_disp_hold");
            data.runway.width_m           = dbase::read_column_double(stmt03_ptr, columns03, "rw_width");
            data.runway.is_valid          = (data.runway.width_m > 0.0) && (data.runway.end_1_lat != data.runway.end_2_lat || data.runway.end_1_lon != data.runway.end_2_lon);

          } // end Q3 step

        } // END Q3

        vecActivity.emplace_back(data);

      }// end landing activity stats

    } // end loop over all rows

  }   // End Q1 prepare statement

  return vecActivity; // landing_stats;
}


// -------------------------------------
// v26.10.1 Height steps and touchdown summary of every landing in the stats database.
//          Runs in the fetch thread on the read-only connection. Queries come from sql.xml <stats_sqls>.
// v26.10.2 in_vecLandingStats: the rows of get_landing_stats_as_vec(), for the runway of each touchdown (runway check, step 5)
std::map<int, missionx::mx_landing_analysis>
data_manager::fetch_landing_analysis(missionx::dbase &in_stats_db, const std::vector<missionx::mx_stats_data> &in_vecLandingStats)
{
  std::map<int, missionx::mx_landing_analysis> mapResult; // key: td_line_id

  // Read the queries into a local map. We are in the fetch thread, so we do not write into the shared data_manager::mapQueries.
  std::map<std::string, std::string> mapStatsQueries;
  Utils::read_external_sql_query_file(mapStatsQueries, mxconst::get_SQLITE_STATS_SQLS());

  // v26.10.2 the bounce window is one value for the queries and for the route (flight_info.route_s). The crash window is used by "landing_crash".
  const std::string bounce_window_s = fmt::format("{:.1f}", mxconst::STATS_BOUNCE_WINDOW_SEC);
  const std::string crash_window_s  = fmt::format("{:.1f}", mxconst::STATS_CRASH_AFTER_LANDING_SEC);
  const std::string bounce_min_sec_s    = fmt::format("{:.2f}", mxconst::STATS_BOUNCE_MIN_SEC);    // v26.10.2
  const std::string bounce_min_agl_ft_s = fmt::format("{:.2f}", mxconst::STATS_BOUNCE_MIN_AGL_FT); // v26.10.2
  const std::string rollout_max_sec_s   = fmt::format("{:.1f}", mxconst::STATS_RUNWAY_ROLLOUT_MAX_SEC); // v26.10.2
  for (auto &query : mapStatsQueries | std::views::values)
  {
    query = mxUtils::replaceAll(query, "{bounce_window_sec}", bounce_window_s);
    query = mxUtils::replaceAll(query, "{crash_window_sec}", crash_window_s);
    query = mxUtils::replaceAll(query, "{bounce_min_sec}", bounce_min_sec_s);
    query = mxUtils::replaceAll(query, "{bounce_min_agl_ft}", bounce_min_agl_ft_s);
    query = mxUtils::replaceAll(query, "{rollout_max_sec}", rollout_max_sec_s);
  }

  const std::string stmt_steps_name   = "landing_height_steps";
  const std::string stmt_summary_name = "landing_touchdown_summary";

  if (!mapStatsQueries.contains(stmt_steps_name) || !mapStatsQueries.contains(stmt_summary_name))
  {
    Log::logMsgErr(fmt::format(R"([{}] sql.xml <{}> does not have the "{}" and "{}" queries. Landing analysis skipped.)", __func__, mxconst::get_SQLITE_STATS_SQLS(), stmt_steps_name, stmt_summary_name));
    return mapResult;
  }

  // ---- 1. height steps ----
  if (in_stats_db.prepareNewStatement(stmt_steps_name, mapStatsQueries[stmt_steps_name]))
  {
    sqlite3_stmt *stmt    = in_stats_db.mapStatements[stmt_steps_name];
    const auto    columns = dbase::get_column_indexes(stmt); // columns are read by their alias name

    while (in_stats_db.step(stmt) == SQLITE_ROW)
    {
      const int td_line_id = dbase::read_column_int(stmt, columns, "td_line_id");

      missionx::mx_landing_step_row row;
      row.step_order    = dbase::read_column_int(stmt, columns, "step_order");
      row.step_agl_ft   = dbase::read_column_double(stmt, columns, "step_agl_ft");
      row.step_label    = dbase::read_column_text(stmt, columns, "step_label");
      row.row_line_id   = dbase::read_column_int(stmt, columns, "row_line_id");
      row.agl_ft        = dbase::read_column_double(stmt, columns, "agl_ft");
      row.ias_kt        = dbase::read_column_double(stmt, columns, "ias_kt");
      row.gs_kt         = dbase::read_column_double(stmt, columns, "gs_kt");
      row.vs_fpm        = dbase::read_column_double(stmt, columns, "vs_fpm");
      row.pitch         = dbase::read_column_double(stmt, columns, "pitch");
      row.roll          = dbase::read_column_double(stmt, columns, "roll");
      row.g             = dbase::read_column_double(stmt, columns, "g");
      row.flap_ratio    = dbase::read_column_double(stmt, columns, "flap_ratio");
      row.sec_to_td     = dbase::read_column_double(stmt, columns, "sec_to_td");
      row.dist_to_td_mt = dbase::read_column_double(stmt, columns, "dist_to_td_mt");

      mapResult[td_line_id].td_line_id = td_line_id;
      mapResult[td_line_id].vecSteps.emplace_back(row);
    }

    // Mark a step that uses the same stats row as the step below it (no own sample at that height, mostly 1 Hz data).
    // The lower step keeps the row, so "TD" always shows its values.
    for (auto &[td_line_id, landing] : mapResult)
    {
      for (size_t i = 0; i + 1 < landing.vecSteps.size(); ++i)
      {
        if (landing.vecSteps[i].row_line_id == landing.vecSteps[i + 1].row_line_id)
          landing.vecSteps[i].same_row_as_next = true;
      }
    }
  }
  else
  {
    Log::logMsgErr(fmt::format("[{}] Failed to prepare \"{}\": {}", __func__, stmt_steps_name, in_stats_db.last_err));
  }


  // ---- 2. touchdown summary ----
  if (in_stats_db.prepareNewStatement(stmt_summary_name, mapStatsQueries[stmt_summary_name]))
  {
    sqlite3_stmt *stmt    = in_stats_db.mapStatements[stmt_summary_name];
    const auto    columns = dbase::get_column_indexes(stmt); // columns are read by their alias name

    while (in_stats_db.step(stmt) == SQLITE_ROW)
    {
      const int td_line_id = dbase::read_column_int(stmt, columns, "td_line_id");
      auto     &landing    = mapResult[td_line_id];
      landing.td_line_id   = td_line_id;

      landing.has_summary  = true;
      landing.td_fpm       = dbase::read_column_double(stmt, columns, "td_fpm");
      landing.td_bank      = dbase::read_column_double(stmt, columns, "td_bank");
      landing.td_pitch     = dbase::read_column_double(stmt, columns, "td_pitch");
      landing.td_gs_kt     = dbase::read_column_double(stmt, columns, "td_gs_kt");
      landing.td_weight_kg = dbase::read_column_double(stmt, columns, "td_weight_kg");

      landing.td_g_avg      = dbase::read_column_double(stmt, columns, "td_g_avg");
      landing.td_g_peak     = dbase::read_column_double(stmt, columns, "td_g_peak");
      landing.pre_td_g      = dbase::read_column_double(stmt, columns, "pre_td_g");
      landing.has_gear_load = !dbase::is_column_null(stmt, columns, "td_gear_load");
      landing.td_gear_load  = dbase::read_column_double(stmt, columns, "td_gear_load");

      landing.final_max_sink_fpm    = dbase::read_column_double(stmt, columns, "final_max_sink_fpm");
      landing.has_final_glide       = !dbase::is_column_null(stmt, columns, "final_max_glide_ratio");
      landing.final_max_glide_ratio = dbase::read_column_double(stmt, columns, "final_max_glide_ratio");
      landing.final_max_bank        = dbase::read_column_double(stmt, columns, "final_max_bank");
      landing.final_steep_sink_fpm   = dbase::read_column_double(stmt, columns, "final_steep_sink_fpm");   // v26.10.2 0 when an older sql.xml has no such column
      landing.final_steep_normal_fpm = dbase::read_column_double(stmt, columns, "final_steep_normal_fpm"); // v26.10.2
    }
  }
  else
  {
    Log::logMsgErr(fmt::format("[{}] Failed to prepare \"{}\": {}", __func__, stmt_summary_name, in_stats_db.last_err));
  }

  // ---- 3. bounces (v26.10.1, optional query) ----
  const std::string stmt_bounce_name = "landing_bounces";
  if (!mapStatsQueries.contains(stmt_bounce_name))
  {
    Log::logMsgWarn(fmt::format("[{}] sql.xml <{}> has no \"{}\" query. Bounces are not analysed.", __func__, mxconst::get_SQLITE_STATS_SQLS(), stmt_bounce_name));
  }
  else if (in_stats_db.prepareNewStatement(stmt_bounce_name, mapStatsQueries[stmt_bounce_name]))
  {
    sqlite3_stmt *stmt    = in_stats_db.mapStatements[stmt_bounce_name];
    const auto    columns = dbase::get_column_indexes(stmt); // columns are read by their alias name

    while (in_stats_db.step(stmt) == SQLITE_ROW)
    {
      const int td_line_id = dbase::read_column_int(stmt, columns, "td_line_id");
      if (!mapResult.contains(td_line_id))
        continue; // no steps/summary for this touchdown

      auto &landing               = mapResult[td_line_id];
      landing.has_bounce_data     = true;
      landing.bounce_count        = dbase::read_column_int(stmt, columns, "bounce_count", 0);
      landing.max_bounce_agl_ft   = dbase::read_column_double(stmt, columns, "max_bounce_agl_ft");   // NULL -> 0 when no bounce
      landing.max_bounce_sec      = dbase::read_column_double(stmt, columns, "max_bounce_sec");
      landing.worst_recontact_fpm = dbase::read_column_double(stmt, columns, "worst_recontact_fpm");
    }
  }
  else
  {
    Log::logMsgErr(fmt::format("[{}] Failed to prepare \"{}\": {}", __func__, stmt_bounce_name, in_stats_db.last_err));
  }

  // ---- 4. crash at the landing (v26.10.2, optional query) ----
  const std::string stmt_crash_name = "landing_crash";
  if (!mapStatsQueries.contains(stmt_crash_name))
  {
    Log::logMsgWarn(fmt::format("[{}] sql.xml <{}> has no \"{}\" query. Crashes are not matched to landings.", __func__, mxconst::get_SQLITE_STATS_SQLS(), stmt_crash_name));
  }
  else if (in_stats_db.prepareNewStatement(stmt_crash_name, mapStatsQueries[stmt_crash_name]))
  {
    sqlite3_stmt *stmt    = in_stats_db.mapStatements[stmt_crash_name];
    const auto    columns = dbase::get_column_indexes(stmt); // columns are read by their alias name

    while (in_stats_db.step(stmt) == SQLITE_ROW)
    {
      const int td_line_id = dbase::read_column_int(stmt, columns, "td_line_id");
      if (!mapResult.contains(td_line_id))
        continue; // no steps/summary for this touchdown

      mapResult[td_line_id].is_crash = (dbase::read_column_int(stmt, columns, "crash_rows", 0) > 0);
    }
  }
  else
  {
    Log::logMsgErr(fmt::format("[{}] Failed to prepare \"{}\": {}", __func__, stmt_crash_name, in_stats_db.last_err));
  }

  // ---- 5. position on the runway: touchdown distance, overrun, excursion (v26.10.2, optional query) ----
  const std::string stmt_rollout_name = "landing_rollout";
  if (!mapStatsQueries.contains(stmt_rollout_name))
  {
    Log::logMsgWarn(fmt::format("[{}] sql.xml <{}> has no \"{}\" query. The runway position of the landings is not analysed.", __func__, mxconst::get_SQLITE_STATS_SQLS(), stmt_rollout_name));
  }
  else if (in_stats_db.prepareNewStatement(stmt_rollout_name, mapStatsQueries[stmt_rollout_name]))
  {
    std::map<int, std::vector<missionx::mx_rollout_row>> mapRolloutRows; // key: td_line_id

    sqlite3_stmt *stmt    = in_stats_db.mapStatements[stmt_rollout_name];
    const auto    columns = dbase::get_column_indexes(stmt); // columns are read by their alias name
    while (in_stats_db.step(stmt) == SQLITE_ROW)
    {
      missionx::mx_rollout_row row;
      row.line_id      = dbase::read_column_int(stmt, columns, "line_id");
      row.lat          = dbase::read_column_double(stmt, columns, "lat");
      row.lon          = dbase::read_column_double(stmt, columns, "lon");
      row.gs_kt        = dbase::read_column_double(stmt, columns, "gs_kt");
      row.heading_true = dbase::read_column_double(stmt, columns, "heading_true");
      row.is_on_ground = (dbase::read_column_int(stmt, columns, "onground_any", 0) != 0);

      mapRolloutRows[dbase::read_column_int(stmt, columns, "td_line_id")].emplace_back(row);
    }

    for (const auto &landing_row : in_vecLandingStats)
    {
      if (!landing_row.runway.is_valid || !mapResult.contains(landing_row.line_id) || !mapRolloutRows.contains(landing_row.line_id))
        continue; // not on a known runway, or no analysis for this touchdown

      analyse_landing_on_runway_cai(landing_row.runway, mapRolloutRows[landing_row.line_id], mapResult[landing_row.line_id]);
    }
  }
  else
  {
    Log::logMsgErr(fmt::format("[{}] Failed to prepare \"{}\": {}", __func__, stmt_rollout_name, in_stats_db.last_err));
  }

  return mapResult;
}

// RUNWAY_CHECK_BEGIN (the standalone test extracts the code between the markers)
// v26.10.2 Position of one landing on its runway. No database and no XPLM calls.
//          in_rows: the stats rows from the touchdown on (sql.xml: landing_rollout), ordered by line_id. The first row is the touchdown.
//          The runway end the plane landed from is the one whose direction is closest to the plane heading at touchdown.
//          Only rows with a wheel on the ground count (a bounce or a touch-and-go is not an overrun).
//          Overrun:   passed the far end by more than STATS_RUNWAY_END_MARGIN_M before slowing below STATS_RUNWAY_ROLLOUT_END_KT.
//          Excursion: the plane center more than STATS_RUNWAY_SIDE_MARGIN_M past the side edge, between both ends, above STATS_RUNWAY_SIDE_EXIT_MIN_KT.
//          The check ends when the rollout ends (no overrun), when the plane turns off the side of the runway slower than STATS_RUNWAY_SIDE_EXIT_MIN_KT,
//          or when it stops (STATS_PATH_STOPPED_GS_MS). After an overrun it goes on until the plane stops: overrun_max_m is how far past the end it got.
//          Taxiing on a taxiway after the landing is not part of it (KPDX: 25 kt on the parallel taxiway was flagged as an excursion).
void
data_manager::analyse_landing_on_runway_cai(const missionx::mx_runway_geometry &in_runway, const std::vector<missionx::mx_rollout_row> &in_rows, missionx::mx_landing_analysis &inout_landing)
{
  if (!in_runway.is_valid || in_rows.empty())
    return;

  const double length_m = mxUtils::mxCalcDistanceBetween2Points(in_runway.end_1_lat, in_runway.end_1_lon, in_runway.end_2_lat, in_runway.end_2_lon, missionx::mx_units_of_measure::meter);
  if (length_m <= 0.0)
    return;

  // the end the plane landed from
  const auto  &td_row          = in_rows.front();
  const double heading_1_to_2  = mxUtils::mxCalcBearingBetween2Points(in_runway.end_1_lat, in_runway.end_1_lon, in_runway.end_2_lat, in_runway.end_2_lon);
  double       heading_diff    = std::fmod(std::fabs(td_row.heading_true - heading_1_to_2), 360.0);
  heading_diff                 = (heading_diff > 180.0) ? 360.0 - heading_diff : heading_diff;
  const bool   flag_from_end_1 = (heading_diff <= 90.0);

  const double end_lat        = (flag_from_end_1) ? in_runway.end_1_lat : in_runway.end_2_lat;
  const double end_lon        = (flag_from_end_1) ? in_runway.end_1_lon : in_runway.end_2_lon;
  const double runway_heading = (flag_from_end_1) ? heading_1_to_2 : std::fmod(heading_1_to_2 + 180.0, 360.0);
  const double displaced_m    = std::clamp((flag_from_end_1) ? in_runway.end_1_displaced_m : in_runway.end_2_displaced_m, 0.0, length_m);
  const double half_width_m   = in_runway.width_m * 0.5;

  const auto lmbda_position_on_runway = [&](const missionx::mx_rollout_row &in_row) { return Utils::get_position_on_runway_m_cai(end_lat, end_lon, runway_heading, in_row.lat, in_row.lon); };

  // ---- touchdown ----
  const auto [td_along_m, td_lateral_m] = lmbda_position_on_runway(td_row);

  inout_landing.has_runway_data     = true;
  inout_landing.landing_runway_s    = (flag_from_end_1) ? in_runway.end_1_name : in_runway.end_2_name;
  inout_landing.runway_length_m     = length_m;
  inout_landing.runway_width_m      = in_runway.width_m;
  inout_landing.landing_distance_m  = length_m - displaced_m;
  inout_landing.td_from_threshold_m = td_along_m - displaced_m;
  inout_landing.td_lateral_m        = td_lateral_m;
  inout_landing.runway_left_m       = length_m - td_along_m;

  // ---- rollout ----
  bool flag_in_rollout = true; // until the groundspeed drops below STATS_RUNWAY_ROLLOUT_END_KT
  for (const auto &row : in_rows)
  {
    if (!row.is_on_ground)
      continue;

    if (flag_in_rollout && row.gs_kt < mxconst::STATS_RUNWAY_ROLLOUT_END_KT)
      flag_in_rollout = false;

    if (!flag_in_rollout && !inout_landing.is_overrun)
      break; // a normal rollout ended: nothing more to check

    const auto [along_m, lateral_m] = lmbda_position_on_runway(row);

    // past the far end
    const double past_end_m = along_m - length_m;
    if (flag_in_rollout && !inout_landing.is_overrun && past_end_m > mxconst::STATS_RUNWAY_END_MARGIN_M)
    {
      inout_landing.is_overrun    = true;
      inout_landing.overrun_gs_kt = row.gs_kt;
    }
    if (inout_landing.is_overrun)
      inout_landing.overrun_max_m = std::max(inout_landing.overrun_max_m, past_end_m);

    // past the side edge, between both ends
    const double past_edge_m = std::fabs(lateral_m) - half_width_m;
    if (along_m >= 0.0 && along_m <= length_m && past_edge_m > mxconst::STATS_RUNWAY_SIDE_MARGIN_M)
    {
      if (row.gs_kt < mxconst::STATS_RUNWAY_SIDE_EXIT_MIN_KT)
        break; // turned off the runway (a taxiway): the landing is over

      if (!inout_landing.is_excursion)
        inout_landing.excursion_gs_kt = row.gs_kt;

      inout_landing.is_excursion    = true;
      inout_landing.excursion_max_m = std::max(inout_landing.excursion_max_m, past_edge_m);
    }

    if (row.gs_kt * missionx::OneNmh2meterInSecond < mxconst::STATS_PATH_STOPPED_GS_MS)
    {
      inout_landing.has_stopped = true;
      break; // stopped: the taxi after it is not part of the landing
    }
  }
}
// RUNWAY_CHECK_END


// -------------------------------------
// v26.10.1 Copy of the mission <scoring>. Every landing element the mission does not define is copied from the default <scoring> (Utils XSD map).
//          We work on a copy, so the mission settings are not changed (this runs in the fetch thread).
IXMLNode
data_manager::get_scoring_with_landing_defaults()
{
  IXMLNode xDefaultScoring = Utils::xml_get_node_from_XSD_map_as_a_copy(mxconst::get_ELEMENT_SCORING());

  IXMLNode xScoring;
  if (mx_global_settings.xScoring_ptr.isEmpty())
    xScoring = xDefaultScoring.deepCopy();
  else
    xScoring = mx_global_settings.xScoring_ptr.deepCopy();

  const std::vector<std::string> vecLandingElements = { mxconst::get_ELEMENT_TD_FPM(),
                                                        mxconst::get_ELEMENT_TD_GEAR_LOAD(),
                                                        mxconst::get_ELEMENT_FINAL_GLIDE(),
                                                        mxconst::get_ELEMENT_TD_BANK(),
                                                        mxconst::get_ELEMENT_TD_BOUNCE_PENALTY(), // v26.10.1
                                                        mxconst::get_ELEMENT_LANDING_GRADE() };

  for (const auto &element_name : vecLandingElements)
  {
    if (xScoring.getChildNode(element_name.c_str()).isEmpty())
      xScoring.addChild(Utils::xml_get_or_create_node(xDefaultScoring, element_name, true)); // true = return a copy of the default element
  }

  return xScoring;
}


// -------------------------------------
// v26.10.1 Landing score (0-100) and grade.
//   base score = sum of (measure score * weight) / sum of (best score * weight) * 100.
//     A measure without a value, without a valid <scoring> definition, or with weight="0" is not part of the score.
//   bounce penalty = <td_bounce_penalty> points for the highest bounce, plus "extra_bounce" for each bounce after the first.
//   final score = max(0, base score + bounce penalty). The grade comes from the final score.
//   "Call the Doctor" when the touchdown fpm or gear load, or the re-contact fpm after a bounce, is above the <landing_grade> limits, whatever the score.
//   v26.10.2 "Impact" when the same values are above the impact limits: replaces the grade, the score is kept.
//            "Crash" when X-Plane reported a crash during the landing or its rollout (is_crash, set by fetch_landing_analysis()):
//            the landing is not graded (scores are 0, the UI shows "not graded"). The impact/doctor flags are still set for the notes.
//   v26.10.2 "Runway overrun" / "Runway excursion" (analyse_landing_on_runway_cai()): the landing score is 0. The base score is kept for the UI.
//            Order: Crash, Impact, Runway overrun, Runway excursion, Call the Doctor, then the score grades.
void
data_manager::score_landing_analysis(missionx::mx_landing_analysis &inout_landing, IXMLNode in_xScoring)
{
  float score_sum      = 0.0f; // the weighted scores the landing got
  float best_score_sum = 0.0f; // the highest weighted scores possible

  const auto lmbda_add_measure = [&](const std::string &in_element_name, const bool in_has_value, const double in_value)
  {
    const auto vecScores = Utils::xml_extract_scoring_subElements(in_xScoring, in_element_name);
    if (!in_has_value || vecScores.empty())
      return;

    IXMLNode    xElement = in_xScoring.getChildNode(in_element_name.c_str());
    const float weight   = Utils::readNodeNumericAttrib<float>(xElement, mxconst::get_ATTRIB_SCORE_WEIGHT(), 1.0f);
    if (weight <= 0.0f)
      return; // measure turned off

    float best_score = 0.0f;
    for (const auto &score_range : vecScores)
      best_score = std::max(best_score, score_range.score);

    best_score_sum += best_score * weight;
    score_sum += Utils::getScoreForValue(vecScores, in_value) * weight;
  };

  lmbda_add_measure(mxconst::get_ELEMENT_TD_FPM(),       inout_landing.has_summary,     inout_landing.td_fpm);
  lmbda_add_measure(mxconst::get_ELEMENT_TD_GEAR_LOAD(), inout_landing.has_gear_load,   inout_landing.td_gear_load);
  lmbda_add_measure(mxconst::get_ELEMENT_FINAL_GLIDE(),  inout_landing.has_final_glide, inout_landing.final_max_glide_ratio);
  lmbda_add_measure(mxconst::get_ELEMENT_TD_BANK(),      inout_landing.has_summary,     inout_landing.td_bank);

  if (best_score_sum <= 0.0f)
  {
    inout_landing.base_score     = 0.0f;
    inout_landing.bounce_penalty = 0.0f;
    inout_landing.landing_score  = 0.0f;
    inout_landing.grade_s        = (inout_landing.is_crash) ? "Crash" : (inout_landing.is_overrun) ? "Runway overrun" : (inout_landing.is_excursion) ? "Runway excursion" : "N/A"; // v26.10.2
    inout_landing.is_doctor      = inout_landing.is_crash;                     // v26.10.2 UI color
    return;
  }

  inout_landing.base_score = (score_sum / best_score_sum) * 100.0f;

  // ---- bounce penalty ----
  inout_landing.bounce_penalty = 0.0f;
  if (inout_landing.has_bounce_data && inout_landing.bounce_count > 0)
  {
    const std::vector<std::string> vecPenaltyAttributes = { mxconst::get_ATTRIB_BOUNCE_SMALL(), mxconst::get_ATTRIB_BOUNCE_MEDIUM(), mxconst::get_ATTRIB_BOUNCE_LARGE() };
    const auto vecPenalty = Utils::xml_extract_scoring_subElements(in_xScoring, mxconst::get_ELEMENT_TD_BOUNCE_PENALTY(), vecPenaltyAttributes);

    IXMLNode    xPenalty           = in_xScoring.getChildNode(mxconst::get_ELEMENT_TD_BOUNCE_PENALTY().c_str());
    const float extra_bounce_point = Utils::readNodeNumericAttrib<float>(xPenalty, mxconst::get_ATTRIB_BOUNCE_EXTRA(), -3.0f);

    float penalty = 0.0f;
    if (!vecPenalty.empty())
      penalty = std::fabs(Utils::getScoreForValue(vecPenalty, inout_landing.max_bounce_agl_ft)); // highest bounce

    penalty += std::fabs(extra_bounce_point) * static_cast<float>(inout_landing.bounce_count - 1); // each bounce after the first

    inout_landing.bounce_penalty = -penalty; // always takes points off
  }

  inout_landing.landing_score = std::max(0.0f, inout_landing.base_score + inout_landing.bounce_penalty);

  // ---- grade ----
  IXMLNode    xGrade           = in_xScoring.getChildNode(mxconst::get_ELEMENT_LANDING_GRADE().c_str());
  const float butter_min       = Utils::readNodeNumericAttrib<float>(xGrade, mxconst::get_ATTRIB_GRADE_BUTTER(), 90.0f);
  const float smooth_min       = Utils::readNodeNumericAttrib<float>(xGrade, mxconst::get_ATTRIB_GRADE_SMOOTH(), 75.0f);
  const float accept_min       = Utils::readNodeNumericAttrib<float>(xGrade, mxconst::get_ATTRIB_GRADE_ACCEPTABLE(), 55.0f);
  const float firm_min         = Utils::readNodeNumericAttrib<float>(xGrade, mxconst::get_ATTRIB_GRADE_FIRM(), 35.0f);
  const float doctor_fpm       = Utils::readNodeNumericAttrib<float>(xGrade, mxconst::get_ATTRIB_GRADE_DOCTOR_FPM(), 600.0f);
  const float doctor_gear_load = Utils::readNodeNumericAttrib<float>(xGrade, mxconst::get_ATTRIB_GRADE_DOCTOR_GEAR_LOAD(), 1.5f);

  const bool is_hard_fpm       = inout_landing.has_summary && (inout_landing.td_fpm > doctor_fpm);
  const bool is_hard_gear_load = inout_landing.has_gear_load && (inout_landing.td_gear_load > doctor_gear_load);
  const bool is_hard_recontact = inout_landing.has_bounce_data && (inout_landing.bounce_count > 0) && (inout_landing.worst_recontact_fpm > doctor_fpm); // v26.10.1

  inout_landing.is_doctor           = (is_hard_fpm || is_hard_gear_load || is_hard_recontact);
  inout_landing.is_doctor_recontact = (is_hard_recontact && !is_hard_fpm && !is_hard_gear_load); // only the re-contact was too hard

  // v26.10.2 impact: the same measures, higher limits. The sink rate is the main signal: at 20 rows per second the G spike can fall between two samples.
  const float impact_fpm       = Utils::readNodeNumericAttrib<float>(xGrade, mxconst::get_ATTRIB_GRADE_IMPACT_FPM(), 1200.0f);
  const float impact_gear_load = Utils::readNodeNumericAttrib<float>(xGrade, mxconst::get_ATTRIB_GRADE_IMPACT_GEAR_LOAD(), 3.0f);

  const bool is_impact_fpm       = inout_landing.has_summary && (inout_landing.td_fpm > impact_fpm);
  const bool is_impact_gear_load = inout_landing.has_gear_load && (inout_landing.td_gear_load > impact_gear_load);
  const bool is_impact_recontact = inout_landing.has_bounce_data && (inout_landing.bounce_count > 0) && (inout_landing.worst_recontact_fpm > impact_fpm);

  inout_landing.is_impact           = (is_impact_fpm || is_impact_gear_load || is_impact_recontact);
  inout_landing.is_impact_recontact = (is_impact_recontact && !is_impact_fpm && !is_impact_gear_load); // only the re-contact was an impact

  if (inout_landing.is_crash || inout_landing.is_impact)
    inout_landing.is_doctor = true; // v26.10.2 the UI colors the grade and the TD row by is_doctor

  if (inout_landing.is_crash)
    inout_landing.grade_s = "Crash";
  else if (inout_landing.is_impact)
    inout_landing.grade_s = "Impact";
  else if (inout_landing.is_overrun) // v26.10.2
    inout_landing.grade_s = "Runway overrun";
  else if (inout_landing.is_excursion) // v26.10.2
    inout_landing.grade_s = "Runway excursion";
  else if (inout_landing.is_doctor)
    inout_landing.grade_s = "Call the Doctor";
  else if (inout_landing.landing_score >= butter_min)
    inout_landing.grade_s = "Butter";
  else if (inout_landing.landing_score >= smooth_min)
    inout_landing.grade_s = "Smooth";
  else if (inout_landing.landing_score >= accept_min)
    inout_landing.grade_s = "Acceptable";
  else if (inout_landing.landing_score >= firm_min)
    inout_landing.grade_s = "Firm";
  else
    inout_landing.grade_s = "Hard";

  // v26.10.2 a crashed landing is not graded
  if (inout_landing.is_crash)
  {
    inout_landing.base_score     = 0.0f;
    inout_landing.bounce_penalty = 0.0f;
    inout_landing.landing_score  = 0.0f;
  }
  else if (inout_landing.is_overrun || inout_landing.is_excursion) // v26.10.2 off the runway: score 0, the base score stays for the UI
  {
    inout_landing.landing_score = 0.0f;
  }
}


// -------------------------------------
// v26.10.1 Notes displayed under the landing table. The limits here are for the notes only, they are not part of the score.
void
data_manager::build_landing_findings(missionx::mx_landing_analysis &inout_landing)
{
  constexpr double FLARE_STEP_FT           = 50.0;  // the flare is judged from this step down to touchdown
  constexpr double NO_FLARE_TD_RATIO       = 0.6;   // touchdown sink still above 60% of the 50ft sink = little or no flare
  constexpr double NO_FLARE_MIN_TD_FPM     = 300.0; // ...only when the touchdown was firmer than this
  constexpr double LONG_FLOAT_SEC          = 7.0;   // time from 50ft to touchdown
  constexpr double STEEP_FINAL_GLIDE_RATIO = 1.7;   // max sink above 1.7x the sink of a normal (3 degree) approach at the same groundspeed, between 300ft and 50ft
  constexpr double FINAL_BANK_LIMIT_DEG    = 10.0;  // between 300ft and touchdown
  constexpr double TD_BANK_LIMIT_DEG       = 5.0;   // at touchdown

  inout_landing.vecFindings.clear();

  // the 50ft step row (nullptr if the descent did not have it)
  const missionx::mx_landing_step_row *step_flare = nullptr;
  for (const auto &row : inout_landing.vecSteps)
  {
    if (row.step_agl_ft == FLARE_STEP_FT)
      step_flare = &row;
  }

  if (inout_landing.is_crash) // v26.10.2
  {
    inout_landing.vecFindings.emplace_back(fmt::format("X-Plane reported a crash during the landing or the rollout (up to {:.0f} sec after touchdown). The landing is not graded.", mxconst::STATS_CRASH_AFTER_LANDING_SEC));
  }

  if (inout_landing.is_impact_recontact) // v26.10.2 the first touchdown was below the impact limits, the re-contact after the bounce was not
  {
    inout_landing.vecFindings.emplace_back(fmt::format("Impact: re-contact after a bounce at {:.0f} fpm.", inout_landing.worst_recontact_fpm));
  }
  else if (inout_landing.is_impact) // v26.10.2
  {
    if (inout_landing.has_gear_load)
      inout_landing.vecFindings.emplace_back(fmt::format("Impact: {:.0f} fpm at touchdown, extra gear load {:.2f}x the plane weight.", inout_landing.td_fpm, inout_landing.td_gear_load));
    else
      inout_landing.vecFindings.emplace_back(fmt::format("Impact: {:.0f} fpm at touchdown.", inout_landing.td_fpm));
  }
  else if (inout_landing.is_doctor_recontact) // v26.10.1 the first touchdown was fine, the re-contact after the bounce was not
  {
    inout_landing.vecFindings.emplace_back(fmt::format("Hard re-contact after a bounce: {:.0f} fpm. Call the Doctor.", inout_landing.worst_recontact_fpm));
  }
  else if (inout_landing.is_doctor)
  {
    if (inout_landing.has_gear_load)
      inout_landing.vecFindings.emplace_back(fmt::format("Hard landing: {:.0f} fpm at touchdown, extra gear load {:.2f}x the plane weight. Call the Doctor.", inout_landing.td_fpm, inout_landing.td_gear_load));
    else
      inout_landing.vecFindings.emplace_back(fmt::format("Hard landing: {:.0f} fpm at touchdown. Call the Doctor.", inout_landing.td_fpm));
  }

  // v26.10.2 off the runway
  if (inout_landing.is_overrun)
  {
    const std::string stop_s = (inout_landing.has_stopped) ? fmt::format("stopped {:.0f} m past it", inout_landing.overrun_max_m) : fmt::format("went at least {:.0f} m past it", inout_landing.overrun_max_m);
    inout_landing.vecFindings.emplace_back(fmt::format("Runway overrun: passed the end of runway {} at {:.0f} kt and {}. Landing score 0.", inout_landing.landing_runway_s, inout_landing.overrun_gs_kt, stop_s));
  }
  if (inout_landing.is_excursion)
    inout_landing.vecFindings.emplace_back(fmt::format("Runway excursion: left the side of runway {} at {:.0f} kt, up to {:.0f} m past the edge. Landing score 0.", inout_landing.landing_runway_s, inout_landing.excursion_gs_kt, inout_landing.excursion_max_m));

  // v26.10.2 where on the runway it touched down
  if (inout_landing.has_runway_data && inout_landing.landing_distance_m > 0.0)
  {
    if (inout_landing.td_from_threshold_m < 0.0)
      inout_landing.vecFindings.emplace_back(fmt::format("Touched down {:.0f} m before the displaced threshold of runway {}: that part is for rolling, not for landing.", -inout_landing.td_from_threshold_m, inout_landing.landing_runway_s));
    else if (inout_landing.td_from_threshold_m > inout_landing.landing_distance_m * mxconst::STATS_RUNWAY_LONG_LANDING_RATIO)
      inout_landing.vecFindings.emplace_back(fmt::format("Long landing: touched down {:.0f} m past the threshold, {:.0f}% into the {:.0f} m landing distance. {:.0f} m of runway were left.",
                                                         inout_landing.td_from_threshold_m, (inout_landing.td_from_threshold_m / inout_landing.landing_distance_m) * 100.0, inout_landing.landing_distance_m, inout_landing.runway_left_m));
  }

  if (inout_landing.bounce_count > 0) // v26.10.1
  {
    const std::string bounce_s = fmt::format("Bounced {} {}: up to {:.1f} ft and {:.1f} sec in the air, re-contact at {:.0f} fpm", inout_landing.bounce_count, (inout_landing.bounce_count == 1) ? "time" : "times", inout_landing.max_bounce_agl_ft, inout_landing.max_bounce_sec, inout_landing.worst_recontact_fpm);
    if (inout_landing.is_crash) // v26.10.2 not graded: no penalty to show
      inout_landing.vecFindings.emplace_back(bounce_s + ".");
    else
      inout_landing.vecFindings.emplace_back(fmt::format("{} ({:.0f} points).", bounce_s, inout_landing.bounce_penalty));
  }

  if (step_flare != nullptr && inout_landing.has_summary)
  {
    const double sink_at_flare_step = std::abs(step_flare->vs_fpm);
    if (inout_landing.td_fpm > NO_FLARE_MIN_TD_FPM && inout_landing.td_fpm > sink_at_flare_step * NO_FLARE_TD_RATIO)
      inout_landing.vecFindings.emplace_back(fmt::format("Little or no flare: {:.0f} fpm at 50 ft, {:.0f} fpm at touchdown.", sink_at_flare_step, inout_landing.td_fpm));

    if (step_flare->sec_to_td > LONG_FLOAT_SEC)
      inout_landing.vecFindings.emplace_back(fmt::format("Long float: {:.1f} sec and {:.0f} m from 50 ft to touchdown.", step_flare->sec_to_td, step_flare->dist_to_td_mt));
  }

  // v26.10.2 "normal" = the sink of a 3 degree approach at the groundspeed of the steepest moment (older sql.xml: the ratio only)
  if (inout_landing.has_final_glide && inout_landing.final_max_glide_ratio > STEEP_FINAL_GLIDE_RATIO)
  {
    if (inout_landing.final_steep_normal_fpm > 0.0)
      inout_landing.vecFindings.emplace_back(fmt::format("Steep final: sinking {:.0f} fpm between 300 ft and 50 ft. At that groundspeed a normal approach sinks about {:.0f} fpm.", inout_landing.final_steep_sink_fpm, inout_landing.final_steep_normal_fpm));
    else
      inout_landing.vecFindings.emplace_back(fmt::format("Steep final: sinking up to {:.0f} fpm between 300 ft and 50 ft, {:.1f} times the sink of a normal approach at your groundspeed.", inout_landing.final_max_sink_fpm, inout_landing.final_max_glide_ratio));
  }

  if (inout_landing.final_max_bank > FINAL_BANK_LIMIT_DEG)
    inout_landing.vecFindings.emplace_back(fmt::format("Bank up to {:.0f} deg below 300 ft.", inout_landing.final_max_bank));

  if (inout_landing.has_summary && inout_landing.td_bank > TD_BANK_LIMIT_DEG)
    inout_landing.vecFindings.emplace_back(fmt::format("Bank {:.1f} deg at touchdown: wing strike risk.", inout_landing.td_bank));

  if (inout_landing.has_summary && inout_landing.td_pitch < 0.0)
    inout_landing.vecFindings.emplace_back(fmt::format("Nose low touchdown (pitch {:.1f} deg): the nose wheel may touch first.", inout_landing.td_pitch));

  if (inout_landing.vecFindings.empty())
    inout_landing.vecFindings.emplace_back("Stable approach and touchdown.");
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
// v26.11.1 fetch_METAR() - sources are tried in order, each one only fills what is still missing:
//   1. X-Plane (XPLM 400+)       - METAR only
//   2. aviationweather.gov       - raw text, no key, METAR + TAF
//   3. flightplandatabase.com    - JSON, METAR + TAF
// -------------------------------------

void
data_manager::fetch_METAR(std::unordered_map<int, mx_nav_data_strct>* mapNavaidData, mxFetchState_enum* outState, std::string* outStatusMessage, std::string* outErrorMsg, const bool* lockThread)
{
  (*outState) = mxFetchState_enum::fetch_in_process;
  outStatusMessage->clear();

  if (threadStateMetar.flagAbortThread)
  {
    (*outState) = mxFetchState_enum::fetch_ended;
    return;
  }

  threadStateMetar.flagIsActive       = true;
  threadStateMetar.flagThreadDoneWork = false;

  // v26.11.1 the old "if (*lockThread) std::lock_guard..." released the lock immediately. Hold it for the whole function.
  std::unique_lock lock(s_thread_sync_mutex, std::defer_lock);
  if (*lockThread)
    lock.lock();


  ////////////////////////////
  ////// Helper lambdas //////

  // HTTP GET. Returns the response body, or an empty string on failure / "no content" (HTTP 204).
  const auto lmbda_http_get = [](const std::string& in_url, const std::string& in_login_pass = "") -> std::string
  {
    missionx::structs::curl_request_data request_data;
    request_data.url_s          = in_url;
    request_data.timeout        = 45L;
    request_data.login_pass     = in_login_pass;
    request_data.how_many_tries = 2;

    const auto request_result = data_manager::get_curl_request_respond(request_data);
    if (request_result.res_curl == CURLE_OK && request_result.http_status == 200)
      return request_result.response_text;

    Log::logMsg(fmt::format("[fetch_METAR] No data from: {}. HTTP status: {}. {}", in_url, request_result.http_status, request_result.request_err));
    return "";
  };

  // Join multi-line reports (TAF change groups) into one line. Returns empty if the text does not mention the ICAO.
  const auto lmbda_clean_report = [](const std::string& in_text, const std::string& in_icao) -> std::string
  {
    std::string report;
    for (const auto& token : mxUtils::split_v2(in_text, " \t\r\n", false))
      report += (report.empty() ? "" : " ") + token;

    return (mxUtils::find_text(report, in_icao, false) == std::string::npos) ? "" : report;
  };


  ////////////////////////////
  ////// Weather sources //////

  // 1. X-Plane - ask the flight loop callback to call XPLMGetMETARForAirport()
  const auto lmbda_fetch_from_xplane = [](mx_nav_data_strct& nav)
  {
    data_manager::fetch_metar_from_flcpre_thread_state.init();
    data_manager::shared_navaid_between_threads.init();
    data_manager::shared_navaid_between_threads.setID(nav.icao);

    if (!missionx::data_manager::waitForPluginCallbackJob(&data_manager::fetch_metar_from_flcpre_thread_state, missionx::mx_flc_pre_command::get_metar_for_airport, std::chrono::milliseconds(3000)))
      Log::logMsg(fmt::format("[fetch_METAR] Callback timeout for: '{}({})'.", "get_metar_for_airport ", nav.icao));
    else if (data_manager::shared_navaid_between_threads.sMetar.empty())
      Log::logMsg(fmt::format("[fetch_METAR] Failed to find Metar info for ICAO: {} using: XPLMGetMETARForAirport() and 'get_metar_for_airport'.", nav.icao));
    else
      nav.sMetar =  fmt::format("{}\n(X-Plane)", data_manager::shared_navaid_between_threads.sMetar);
  };

  // 2. aviationweather.gov (NOAA) - raw text, no key
  const auto lmbda_fetch_from_aviationweather = [&](mx_nav_data_strct& nav)
  {
    constexpr static const char* base_url_s = "https://aviationweather.gov/api/data";

    if (nav.sMetar.empty())
      nav.sMetar = fmt::format("{}\n(aviationweather.gov)", lmbda_clean_report(lmbda_http_get(fmt::format("{}/metar?ids={}&format=raw", base_url_s, nav.icao)), nav.icao) );

    if (nav.sTaf.empty() && !threadStateMetar.flagAbortThread)
      nav.sTaf = lmbda_clean_report(lmbda_http_get(fmt::format("{}/taf?ids={}&format=raw", base_url_s, nav.icao)), nav.icao);
  };

  // 3. flightplandatabase.com - JSON with "METAR" and "TAF" keys
  const auto lmbda_fetch_from_flightplandatabase = [&](mx_nav_data_strct& nav)
  {
    const std::string url_s     = fmt::format("https://api.flightplandatabase.com/weather/{}", nav.icao);
    const std::string authKey_s = Utils::getNodeText_type_6(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_AUTHORIZATION_KEY(), "");

    std::string result_s = lmbda_http_get(url_s, authKey_s);
    if (result_s.empty())
      return;

    #ifndef RELEASE
    Log::logMsg(result_s);
    #endif

    try
    {
      // if the respond holds more than one object, use the last one
      if (std::ranges::count(result_s, '{') > 1)
        result_s = result_s.substr(result_s.rfind('{'));

      const nlohmann::json js = nlohmann::json::parse(result_s, nullptr, true);
      if (nav.sMetar.empty())
        nav.sMetar = fmt::format("{}\n(fplndb)", Utils::getJsonValue(js, "METAR", "") );
      if (nav.sTaf.empty())
        nav.sTaf = Utils::getJsonValue(js, "TAF", "");
    }
    catch (nlohmann::json::parse_error& ex)
    {
      Log::logMsg(fmt::format("{}.\n== JSON: ==>\n{}\n<== END JSON ==", ex.what(), result_s));
      (*outErrorMsg) = ex.what();
    }
  };


  /////////////////////////////////////////
  ////// Loop over all Nav Aids //////

  for (auto& nav : *mapNavaidData | std::views::values)
  {
    if (threadStateMetar.flagAbortThread)
      break;

    nav.sMetar.clear();
    nav.sTaf.clear();

    std::string metar_from_s;
    std::string taf_from_s;

    // Run one source, unless we already have both reports. Remember who gave us what.
    const auto lmbda_try_source = [&](const std::string& in_source_name, const auto& in_fetch_func)
    {
      if (threadStateMetar.flagAbortThread || !nav.sMetar.empty() )
      {
        // Log::logMsg(fmt::format("[{}] {}", __func__, (threadStateMetar.flagAbortThread)? "User request abort.": fmt::format("Metar found: {}", nav.sMetar ) ));
        return;
      }

      in_fetch_func(nav);

      if (metar_from_s.empty() && !nav.sMetar.empty())
        metar_from_s = in_source_name;
      if (taf_from_s.empty() && !nav.sTaf.empty())
        taf_from_s = in_source_name;
    };

    if (data_manager::xplm_version >= 400)
      lmbda_try_source("X-Plane", lmbda_fetch_from_xplane);
    if (!metar_from_s.empty())
      lmbda_try_source("aviationweather.gov", lmbda_fetch_from_aviationweather);
    if (!metar_from_s.empty())
      lmbda_try_source("flightplandatabase.com", lmbda_fetch_from_flightplandatabase);

    (*outStatusMessage) = fmt::format("Finished fetching METAR information for: {}. METAR: {}, TAF: {}.", nav.icao,
                                      mxUtils::eval_text(!metar_from_s.empty(), metar_from_s, "not found"),
                                      mxUtils::eval_text(!taf_from_s.empty(), taf_from_s, "not found"));
    Log::logMsg((*outStatusMessage));

  } // end loop over all Nav Aids

  if (threadStateMetar.flagAbortThread)
    (*outStatusMessage) = "Metar thread aborted.";

  threadStateMetar.flagThreadDoneWork = true;
  threadStateMetar.flagIsActive       = false;
  threadStateMetar.flagAbortThread    = false;
  (*outState)                         = mxFetchState_enum::fetch_ended;
}


// -------------------------------------

void
data_manager::fetch_fpln_from_simbrief_site (missionx::base_thread::strct_thread_state *inoutThreadState, const std::string &in_pilot_id, missionx::mxFetchState_enum *outState, std::string *outStatusMessage)
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
  const std::string full_url_s = fmt::format ("https://www.simbrief.com/api/xml.fetcher.php?userid={}", in_pilot_id);

  //// Fetch information
  std::string err;
  std::string cert_loc_s = data_manager::mx_folders_properties.getStringAttributeValue (mxconst::get_PROP_MISSIONX_PATH(), "");
  missionx::structs::strct_curl_result simbrief_st_curl_result;

  long httpStatus = 0;

  // try up to three times
  std::string          msg;
  constexpr static int MAX_TRIES = 3;
  for (int loop01 = 0; loop01 < MAX_TRIES && !flag_http_success; ++loop01)
  {
    msg.clear();

    // execute the REQUEST
    // simbrief_st_curl_result = data_manager::get_curl_request_respond(full_url_s);
    // v26.08.1
    missionx::structs::curl_request_data strct_curl_data {.url_s = full_url_s};
    simbrief_st_curl_result = data_manager::get_curl_request_respond(strct_curl_data);

    // #ifdef MX_ENABLE_HTTP_REQUESTS
    if (CURLE_OK != simbrief_st_curl_result.res_curl)
    {
      msg = fmt::format("[{}] cURL error code: {}\n", __func__, Utils::formatNumber<int>(simbrief_st_curl_result.res_curl));
      Log::logMsg(msg);
    }
    // #else
    //   simbrief_st_curl_result.res_curl    = -1;
    //   simbrief_st_curl_result.request_err = "cURL is disabled in this build.";
    // #endif // !DISABLE_CURL

    if (!simbrief_st_curl_result.request_err.empty())
    {
      msg = fmt::format("cURL HTTP status: {}", simbrief_st_curl_result.request_err);
    }
    else // parse json
    {
      flag_http_success   = true;
      (*outStatusMessage) = "cURL fetch success.";
    }

    if (!msg.empty())
      (*outStatusMessage) = fmt::format("Try {} of {}. {}", loop01 + 1, MAX_TRIES, msg);

    Log::logMsg((*outStatusMessage)); // debug

    // Wait for 2 seconds
    std::this_thread::sleep_for(std::chrono::seconds(2)); // v26.01.1 wait for 2 seconds
  } // end internal fetch loop. Try few times before giving up.
  // end data_manager::curl - handling cURL

  //// Parse result
  if (flag_http_success)
  {
    simbrief_st_curl_result.response_text = mxUtils::replaceAll(simbrief_st_curl_result.response_text, "<br>", ""); // v25.06.1

    IXMLReaderStringSZ iReaderSZ(simbrief_st_curl_result.response_text.c_str());
    IXMLResults        parse_result;
    IXMLDomParser      idom;
    auto               xml_ofp = idom.parseString(simbrief_st_curl_result.response_text.c_str(), mxconst::get_ELEMENT_SIMBRIEF_ROOT_OFP().c_str(), &parse_result);
    if (parse_result.errorCode == IXMLError_None)
    {
      mx_external_fpln_strct fpln;
      fpln.internal_id = 0;

      missionx::mx_base_node ofp_node;
      ofp_node.node = xml_ofp.deepCopy();
      // read the route nodes. So far there are three
      auto node_general     = ofp_node.node.getChildNode(mxconst::get_ELEMENT_GENERAL().c_str());
      auto node_origin      = ofp_node.node.getChildNode(mxconst::get_ELEMENT_ORIGIN().c_str());
      auto node_destination = ofp_node.node.getChildNode(mxconst::get_ELEMENT_DESTINATION().c_str());
      auto node_alternate   = ofp_node.node.getChildNode(mxconst::get_ELEMENT_ALTERNATE().c_str()); // v26.02.1
      auto node_fuel        = ofp_node.node.getChildNode("fuel"); // v25.06.1 fetch fuel information
      auto node_navlog      = ofp_node.node.getChildNode("navlog"); // v25.06.1 fetch TOC, top of climb, data
      auto node_weights     = ofp_node.node.getChildNode("weights"); // v25.06.1 fetch TOC, top of climb, data

      if (!node_general.isEmpty())
      {
        auto icaoairln_node       = node_general.getChildNode(mxconst::get_ELEMENT_ICAO_AIRLINE().c_str());
        auto flnum_node           = node_general.getChildNode(mxconst::get_ELEMENT_FLIGHT_NUMBER().c_str());
        auto route_node           = node_general.getChildNode(mxconst::get_ELEMENT_ROUTE().c_str());
        auto route_ifps_node      = node_general.getChildNode(mxconst::get_ELEMENT_ROUTE_IFPS().c_str());
        auto route_navigraph_node = node_general.getChildNode(mxconst::get_ELEMENT_ROUTE_NAVIGRAPH().c_str());
        auto cruise_profile_node  = node_general.getChildNode("costindex"); // v25.06.1

        fpln.simbrief_icao_airline_s  = Utils::xml_get_text(icaoairln_node);
        fpln.simbrief_flightNumber_s  = Utils::xml_get_text(flnum_node);
        fpln.simbrief_route           = Utils::xml_get_text(route_node);
        fpln.simbrief_route_ifps      = Utils::xml_get_text(route_ifps_node);
        fpln.simbrief_route_navigraph = Utils::xml_get_text(route_navigraph_node);
        fpln.simbrief_costindex       = Utils::xml_get_text(cruise_profile_node); // v25.06.1
      }

      if (!node_origin.isEmpty())
      {
        auto icao_code_node       = node_origin.getChildNode(mxconst::get_ELEMENT_ICAO_CODE().c_str());
        auto name_node            = node_origin.getChildNode(mxconst::get_ATTRIB_NAME().c_str());
        auto trans_alt_node       = node_origin.getChildNode(mxconst::get_ELEMENT_TRANS_ALT().c_str());
        auto plan_rwy_node        = node_origin.getChildNode(mxconst::get_ELEMENT_PLAN_RWY().c_str());
        auto plan_metar_node      = node_origin.getChildNode(mxconst::get_ELEMENT_METAR().c_str());
        auto plan_metar_time_node = node_origin.getChildNode(mxconst::get_ELEMENT_METAR_TIME().c_str());

        fpln.fromICAO_s = Utils::xml_get_text(icao_code_node);
        fpln.fromName_s = Utils::xml_get_text(name_node);

        fpln.simbrief_origin_rw         = Utils::xml_get_text(plan_rwy_node);
        fpln.simbrief_origin_trans_alt  = Utils::xml_get_text(trans_alt_node);
        fpln.simbrief_origin_metar      = Utils::xml_get_text(plan_metar_node);
        fpln.simbrief_origin_metar_time = Utils::xml_get_text(plan_metar_time_node);
      }

      if (!node_destination.isEmpty())
      {
        auto icao_code_node       = node_destination.getChildNode(mxconst::get_ELEMENT_ICAO_CODE().c_str());
        auto name_node            = node_destination.getChildNode(mxconst::get_ATTRIB_NAME().c_str());
        auto trans_alt_node       = node_destination.getChildNode(mxconst::get_ELEMENT_TRANS_ALT().c_str());
        auto plan_rwy_node        = node_destination.getChildNode(mxconst::get_ELEMENT_PLAN_RWY().c_str());
        auto plan_metar_node      = node_destination.getChildNode(mxconst::get_ELEMENT_METAR().c_str());
        auto plan_metar_time_node = node_destination.getChildNode(mxconst::get_ELEMENT_METAR_TIME().c_str());

        fpln.toICAO_s = Utils::xml_get_text(icao_code_node);
        fpln.toName_s = Utils::xml_get_text(name_node);

        fpln.simbrief_destination_rw         = Utils::xml_get_text(plan_rwy_node);
        fpln.simbrief_destination_trans_alt  = Utils::xml_get_text(trans_alt_node);
        fpln.simbrief_destination_metar      = Utils::xml_get_text(plan_metar_node);
        fpln.simbrief_destination_metar_time = Utils::xml_get_text(plan_metar_time_node);
      }

      if (!node_alternate.isEmpty())
      {
        auto alternate_icao_node       = node_alternate.getChildNode(mxconst::get_ELEMENT_ICAO_CODE().c_str());
        fpln.simbrief_alternate_icao_s = Utils::xml_get_text(alternate_icao_node);
      }

      if (!node_fuel.isEmpty())
      {
        auto enroute_burn_node    = node_fuel.getChildNode("enroute_burn");
        auto reserve_node         = node_fuel.getChildNode("reserve");
        auto plan_ramp_block_fuel = node_fuel.getChildNode("plan_ramp");

        fpln.simbrief_enroute_burn         = Utils::xml_get_text(enroute_burn_node);
        fpln.simbrief_reserve              = Utils::xml_get_text(reserve_node);
        fpln.simbrief_plan_ramp_block_fuel = Utils::xml_get_text(plan_ramp_block_fuel);
      }

      if (!node_navlog.isEmpty())
      {
        // find <fix> subnode with a subnode <ident> and value "TOC"
        auto toc_fix_node = Utils::xml_get_parent_node_from_node_tree_by_sub_tag_name_and_text_value(node_navlog, "ident", "TOC", 0);

        auto simbrief_altitude_feet_node = Utils::xml_get_child_node(toc_fix_node, "altitude_feet");
        auto simbrief_wind_dir_node      = Utils::xml_get_child_node(toc_fix_node, "wind_dir");
        auto simbrief_wind_spd_node      = Utils::xml_get_child_node(toc_fix_node, "wind_spd");
        auto simbrief_oat_isa_dev_node   = Utils::xml_get_child_node(toc_fix_node, "oat_isa_dev");
        auto simbrief_oat_node           = Utils::xml_get_child_node(toc_fix_node, "oat");
        auto simbrief_via_airway         = Utils::xml_get_child_node(toc_fix_node, "via_airway");

        fpln.simbrief_toc_elev_ft    = Utils::xml_get_text(simbrief_altitude_feet_node);
        fpln.simbrief_toc_wind       = fmt::format("{}/{}", Utils::xml_get_text(simbrief_wind_dir_node), Utils::xml_get_text(simbrief_wind_spd_node));
        fpln.simbrief_toc_wind_ISA   = Utils::xml_get_text(simbrief_oat_isa_dev_node);
        fpln.simbrief_toc_wind_OAT   = Utils::xml_get_text(simbrief_oat_node);
        fpln.simbrief_toc_via_airway = Utils::xml_get_text(simbrief_via_airway);
      }

      if (!node_weights.isEmpty())
      {
        auto pax_count_actual_node = Utils::xml_get_child_node(node_weights, "pax_count_actual");
        auto cargo_node = Utils::xml_get_child_node(node_weights, "cargo");
        auto payload_node = Utils::xml_get_child_node(node_weights, "payload");
        auto est_tow_node = Utils::xml_get_child_node(node_weights, "est_tow"); // TOW estimation

        fpln.simbrief_passengers     = mxUtils::stringToNumber<int>( Utils::xml_get_text(pax_count_actual_node) );
        fpln.simbrief_cargo          = mxUtils::stringToNumber<float>(Utils::xml_get_text(cargo_node) );
        fpln.simbrief_payload        = mxUtils::stringToNumber<float>( Utils::xml_get_text(payload_node) );
        fpln.simbrief_takeoff_weight = Utils::xml_get_text(est_tow_node);

        fpln.simbrief_pasengers_weight = fpln.simbrief_payload - fpln.simbrief_cargo;
      }

      // v26.09.3
      const auto local_simbrief_fuel_block = mxUtils::stringToNumber<float>(fpln.simbrief_plan_ramp_block_fuel);
      const auto fuel_block = (local_simbrief_fuel_block > 1000.0f)? (local_simbrief_fuel_block / 1000.0f) : local_simbrief_fuel_block;
      const auto fuel_block_weight_type = (local_simbrief_fuel_block > 1000.0f) ? "ton" : "kg";

      fpln.more_info = fmt::format("General:\n\tAirline/FlightNo: {}/{}, Cost Index: {}, Alternate: {}\n\n"
                                   "Weights:\n\t{:<10}: {:>8} kg\n\t{:<10}: {:>8} kg\n\t{:<10}: {:>8} {}\n\n"
                                   "Fuel:\n\t{:<10}: {:>8}\n\t{:<10}: {:>8}\n\t{:<10}: {:>8}\n\n"
                                   "T O C:\n\t{:<10}: {:>8}\n\t{:<10}: {:>8}\n\t{:<10}: {:>8}\n\t{:<10}: {:>8}\n\t{:<10}: {:>8}\n\n"
                                   // <general>
                                   // , "Airline/FlightNo: "
                                 , fpln.simbrief_icao_airline_s, fpln.simbrief_flightNumber_s
                                   // , "Cost Index"
                                 , fpln.simbrief_costindex
                                   // , "Alter"
                                 , fpln.simbrief_alternate_icao_s
                                   // Weights
                                 // , "Payload", mxUtils::formatNumber(fpln.simbrief_payload), "Est. TOW", fpln.simbrief_takeoff_weight, "Fuel", fmt::format("{:.1f}", mxUtils::stringToNumber<float>(fpln.simbrief_plan_ramp_block_fuel) / 1000.0f) // <weights>
                                 , "Payload", mxUtils::formatNumber(fpln.simbrief_payload), "Est. TOW", fpln.simbrief_takeoff_weight, "Fuel", fmt::format("{:.1f}", fuel_block), fuel_block_weight_type // <weights>
                                   // Fuel
                                 , "Block Fuel", fpln.simbrief_plan_ramp_block_fuel, "Trip", fpln.simbrief_enroute_burn, "Reserve", fpln.simbrief_reserve // <fuel>
                                   // T.O.C
                                 , "Elev ft.", fpln.simbrief_toc_elev_ft, "Wind", fpln.simbrief_toc_wind, "W.ISA Dev", fpln.simbrief_toc_wind_ISA // Top Of Climb
                                 , "OAT", fpln.simbrief_toc_wind_OAT
                                 , "Airway", ((fpln.simbrief_toc_via_airway.empty()) ? "n/a" : fpln.simbrief_toc_via_airway)
                                  ); // Right aligned: |{:>6}|

      // fill origin / cruise and destination info
      fpln.origin_info = fmt::format("Metar Time: {}\nMetar: {}\n", fpln.simbrief_origin_metar_time, fpln.simbrief_origin_metar);
      fpln.cruise_info = fmt::format("Elev ft.: {}\nWind: {}\nW.ISA Dev: {}\nOAT: {}\nAirway: {}"
                                   , fpln.simbrief_toc_elev_ft, fpln.simbrief_toc_wind
                                   , fpln.simbrief_toc_wind_ISA, fpln.simbrief_toc_wind_OAT
                                   , ((fpln.simbrief_toc_via_airway.empty()) ? "n/a" : fpln.simbrief_toc_via_airway)
                                    );
      fpln.destination_info = fmt::format("Metar Time: {}\nMetar: {}\n", fpln.simbrief_destination_metar_time, fpln.simbrief_destination_metar);


      data_manager::tableExternalFPLN_simbrief_vec.emplace_back(fpln);
      data_manager::strct_ui_share_data.strct_last_simbrief_fpln = fpln;
    }

    #ifndef RELEASE
    Log::logMsg("Curl Result: " + simbrief_st_curl_result.response_text);
    #endif
  }
  #ifndef RELEASE
  Log::logMsg (fmt::format("\nSimbrief URL: {}\n", full_url_s) ); // debug
  Log::logMsg (fmt::format("\ncacert path: {}\n", data_manager::ca_path.string()) ); // debug
  #endif


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
data_manager::fetch_fpln_from_flightplandatabase_site(base_thread::strct_thread_state* inoutThreadState, const IXMLNode& inUserPref, mxFetchState_enum* outState, std::string* outStatusMessage)
{
  assert(outStatusMessage && fmt::format("[{}] outStatusMessage must have a valid pointer.", __func__).c_str()); // v26.01.4

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

  const auto lmbda_build_q = [](bool& isFirstTime, std::string in_q, const std::string& attrib, const std::string& val = "")
  {
    if (!val.empty())
    {
      in_q += (isFirstTime) ? attrib + "=" + val : "&" + attrib + "=" + val;
      isFirstTime = false;
    }

    return in_q;
  };

  /// Prepare the query string
  q = lmbda_build_q(bIsFirstTime, q,   mxconst::get_PROP_FROM_ICAO(), val_s);

  // v3.0.253.3 added toICAO as a search criteria
  val_s = Utils::readAttrib(inUserPref,   mxconst::get_PROP_TO_ICAO(), "");
  q     = lmbda_build_q(bIsFirstTime, q,   mxconst::get_PROP_TO_ICAO(), val_s);

  // v25.12.1 Added min value
  val_s = Utils::readAttrib(inUserPref,   mxconst::get_PROP_MIN_DISTANCE_SLIDER(), "");
  q     = lmbda_build_q(bIsFirstTime, q, "distanceMin", val_s);

  val_s = Utils::readAttrib(inUserPref,   mxconst::get_PROP_MAX_DISTANCE_SLIDER(), "");
  q     = lmbda_build_q(bIsFirstTime, q, "distanceMax", val_s);


  val_s = Utils::readAttrib(inUserPref,   mxconst::get_PROP_LIMIT(), "");
  q     = lmbda_build_q(bIsFirstTime, q,   mxconst::get_PROP_LIMIT(), val_s);


  val_s = Utils::readAttrib(inUserPref,   mxconst::get_PROP_SORT_FPLN_BY(), "");
  q     = lmbda_build_q(bIsFirstTime, q,   mxconst::get_PROP_SORT_FPLN_BY(), val_s);

  const std::string url_s      = "api.flightplandatabase.com";
  const std::string full_url_s = mxUtils::trim("https://" + url_s + ":443" + q);
  Log::logMsg("flightplandatabase url: " + full_url_s); // debug

  //// Fetch information
  std::string err;
  std::string cert_loc_s = mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", err);

  std::string msg;
  //long httpStatus = 0;
  constexpr int max_loop = 3;
  int loop_counter = 0;
  while (loop_counter < max_loop && !flag_http_success)
  {
    flag_http_success = false;

// #ifdef MX_ENABLE_HTTP_REQUESTS

    missionx::structs::curl_request_data request_data;
    request_data.timeout        = 45L;
    request_data.url_s          = full_url_s;
    request_data.login_pass     = authKey_s;
    request_data.how_many_tries = 1;
    request_data.headers = {mxconst::OVERPASS_POST_HEADER};

    (*outState)                                         = mxFetchState_enum::fetch_in_process;
    missionx::structs::strct_curl_result request_result = data_manager::get_curl_request_respond(request_data);
    if (CURLE_OK != request_result.res_curl)
    {
      Log::logMsg("cURL error code: " + Utils::formatNumber<int>(request_result.res_curl) + "\n");
    }

    
    if (request_result.http_status != 200 || !request_result.request_err.empty())
    {
      switch (request_result.http_status)
      {
      case 503:
      case 522:
        msg          = fmt::format("cURL HTTP status: {}. Error: {}", request_result.http_status, "Site might be unavailable.");
        loop_counter = max_loop;
        break;
      case 500:
        msg          = fmt::format("cURL HTTP status: {}. Error: {}", request_result.http_status, "Remote internal server error.");
        loop_counter = max_loop;
        break;
      case 504:
        msg = fmt::format("Try {}/{}: cURL HTTP status: {}. Error: {}", (loop_counter + 1), max_loop, request_result.http_status, "Gateway Timeout.");
        break;
      default:
        msg = fmt::format("Try {}/{}: cURL HTTP status: {}. {}", (loop_counter + 1), max_loop, request_result.http_status, result_s);
        break;
      }
      // (*outStatusMessage) = msg;
      result_s.clear();

      #ifndef RELEASE
      Log::logMsg((*outStatusMessage)); // debug
      #endif

      std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    if (request_result.http_status == 200) // parse json
    {
      result_s            = request_result.response_text;
      flag_http_success   = true;
      (*outStatusMessage) = "cURL fetch success.";
    }

    loop_counter++;

    // ABORT CHECK
    if (inoutThreadState && inoutThreadState->flagAbortThread)
    {
      flag_http_success = false;
      loop_counter      = max_loop + 1;
      msg               = "User ask to abort.";
    }

// #else
//     msg = "cURL is disabled in this build.";
//     flag_http_success = false;
//     curl_result_s.clear();
//     loop_counter = max_loop + 1;
// #endif // !DISABLE_CURL


  } // end while loop

  if (!msg.empty())
    (*outStatusMessage) = msg;


  //// Parse result
  if (flag_http_success)
  {
    int              counter = 0;
    std::set<size_t> setHashIcaoName;

    #ifndef RELEASE
    std::map<size_t, bool> mapHashIcaoAndName;
    #endif // !RELEASE

    nlohmann::json jSON = nlohmann::json::parse(result_s);

    for (auto& js : jSON)
    {
      #ifndef RELEASE
      Log::logMsg(fmt::format("JSON size: {}\nJSON Line:\n{}\n", js.size(), js.dump()));
      #endif
      mx_external_fpln_strct fpln;

      fpln.distnace_d        = Utils::getJsonValue<double>(js, mx_fplndb_json_keys.Z_KEY_distance, nlohmann::detail::value_t::number_float, 0.0);
      fpln.encode_polyline_s = Utils::getJsonValue(js, mx_fplndb_json_keys.Z_KEY_encodedPolyline, "");
      fpln.simbrief_flightNumber_s    = Utils::getJsonValue(js, mx_fplndb_json_keys.Z_KEY_flightNumber, "");
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
      if (bUserAskedToRemoveDuplicateICAO && bUserAskedToGroupByICAOAndWaypoints) // include waypoints in the filter hash
        hashIcaoAndAirportName = std::hash<std::string>{}(mxUtils::stringToLower(fpln.toICAO_s + fpln.toName_s + mxUtils::formatNumber<int>(fpln.waypoints_i)));
      else // exclude waypoints in filter hash
        hashIcaoAndAirportName = std::hash<std::string>{}(mxUtils::stringToLower(fpln.toICAO_s + fpln.toName_s));

      if (bUserAskedToRemoveDuplicateICAO && (setHashIcaoName.contains(hashIcaoAndAirportName)))
        continue; // skip current fpln

      setHashIcaoName.insert(hashIcaoAndAirportName); // insert hash

      #ifndef RELEASE
      const std::string waypoints_s = mxUtils::formatNumber<int>(fpln.waypoints_i);

      std::string last_toICAO_s = fpln.toICAO_s;
      std::string last_toName_s = fpln.toName_s;
      Utils::addElementToMap(mapHashIcaoAndName, hashIcaoAndAirportName, true);
      #endif // !RELEASE


      fpln.internal_id = counter; // we will use it if we need to show extra details of specific flight plan

      tableExternalFPLN_vec.emplace_back(fpln);
      Utils::addElementToMap(indexPointer_forExternalFPLN_tableVector, counter, &tableExternalFPLN_vec.back()); // adding pointer data to std::map index like container


      counter++;
      if (counter > 5000)
        break;
    } // end loop over all <a> nodes or JSON lines

    if (flag_http_success)
      curl_result_s = result_s; // store JSON output as a string
    else
      curl_result_s.clear();

    #ifndef RELEASE
    Log::logMsg("Curl Result: " + curl_result_s);
    #endif

    // #endif //original #endif, moved after JSON to XML parsing.

    if (counter == 0)
      (*outStatusMessage) = "No flight plans were found for this location.";
    else
      (*outStatusMessage) = mxUtils::formatNumber<int>(counter) + " rows fetched"; // v3.303.8.3

  } // if flag_http_success

  assert (inoutThreadState && fmt::format("[{}] Thread state must be valid.", __func__).c_str ());

  inoutThreadState->flagIsActive       = false;
  inoutThreadState->flagThreadDoneWork = true;
  inoutThreadState->flagAbortThread    = false;

  (*outState) = mxFetchState_enum::fetch_ended; // once we change the state, the UI can show/hide the relevant layers/widgets

  Log::logMsg (fmt::format("[{}] Finished curl job", __func__) ) ;

}


// -------------------------------------
// -------------------------------------
// -------------------------------------

missionx::structs::strct_curl_result
data_manager::get_curl_request_respond(missionx::structs::curl_request_data& in_request_data, const int& in_call_id)
{
  const auto lmbda_extract_host = [](const std::string& url) -> std::string
  {
    if (url.empty())
      return {};

    CURLU* url_handle = curl_url();
    if (!url_handle)
      return {};

    if (curl_url_set(url_handle, CURLUPART_URL, url.c_str(), 0) != CURLUE_OK)
    {
      curl_url_cleanup(url_handle);
      return {};
    }

    char* host = nullptr;
    std::string result;
    if (curl_url_get(url_handle, CURLUPART_HOST, &host, 0) == CURLUE_OK && host != nullptr)
    {
      result = host;
      curl_free(host);
    }

    curl_url_cleanup(url_handle);
    return result;
  };

  missionx::structs::strct_curl_result st_result;

  // #ifdef MX_ENABLE_HTTP_REQUESTS


  st_result.reset();
  st_result.res_curl = CURLE_FAILED_INIT;

  if (in_request_data.url_s.empty())
  {
    st_result.request_err = fmt::format("Call ID: {}, URL is empty.", in_call_id);
    return st_result;
  }

  const std::string base_url = in_request_data.url_s.substr(0, in_request_data.url_s.find('?'));

  CURL* curl_func = curl_easy_init();
  if (!curl_func)
  {
    st_result.request_err = fmt::format("Call ID: {}, curl_easy_init() failed.", in_call_id);
    return st_result;
  }

  // 1. Prepare HTTP headers once outside the retry loop
  struct curl_slist* p_headers = nullptr;
  for (const auto& header : in_request_data.headers)
  {
    p_headers = curl_slist_append(p_headers, header.c_str());
  }

  Log::logMsg(fmt::format("[{}] Call ID: {}\t url: \"{}\"", __func__, data_manager::curl_call_counter_i, in_request_data.url_s));

  // Error buffer scoped with curl_func to avoid dangling pointers
  char errBuff[CURL_ERROR_SIZE] = {0};

  // 2. Set static options once
  curl_easy_setopt(curl_func, CURLOPT_NOSIGNAL, 1L); // Essential for thread safety on Linux
  curl_easy_setopt(curl_func, CURLOPT_URL, in_request_data.url_s.c_str());
  curl_easy_setopt(curl_func, CURLOPT_USERAGENT, APP_NAME);
  curl_easy_setopt(curl_func, CURLOPT_CONNECTTIMEOUT, in_request_data.connection_timeout);
  curl_easy_setopt(curl_func, CURLOPT_TIMEOUT, in_request_data.timeout);
  curl_easy_setopt(curl_func, CURLOPT_FOLLOWLOCATION, in_request_data.follow_location);
  curl_easy_setopt(curl_func, CURLOPT_SSL_VERIFYPEER, in_request_data.ssl_verify_peer);
  curl_easy_setopt(curl_func, CURLOPT_SSL_VERIFYHOST, in_request_data.ssl_verify_host);
  curl_easy_setopt(curl_func, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl_func, CURLOPT_ERRORBUFFER, errBuff);
  curl_easy_setopt(curl_func, CURLOPT_WRITEFUNCTION, my_write);
  curl_easy_setopt(curl_func, CURLOPT_WRITEDATA, &st_result.response_text);

  // CA Certificate check using noexcept error codes
  std::error_code error_code;
  if (!data_manager::ca_path.empty() &&
      std::filesystem::is_regular_file(data_manager::ca_path, error_code) && !error_code)
  {
    curl_easy_setopt(curl_func, CURLOPT_CAINFO, data_manager::ca_path.string().c_str());
  }

  if (!in_request_data.login_pass.empty())
  {
    curl_easy_setopt(curl_func, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl_func, CURLOPT_USERPWD, in_request_data.login_pass.c_str());
  }

  if (!in_request_data.post_request_field_s.empty())
  {
    curl_easy_setopt(curl_func, CURLOPT_POSTFIELDS, in_request_data.post_request_field_s.c_str());
  }

  if (p_headers)
  {
    curl_easy_setopt(curl_func, CURLOPT_HTTPHEADER, p_headers);
  }

  // 3. Retry loop
  long httpStatus = 0;
  const int max_tries = std::max(1, in_request_data.how_many_tries);

  for (int attempt = 0; attempt < max_tries; ++attempt)
  {
    st_result.reset();
    errBuff[0] = '\0';


    Log::logMsg(fmt::format("[{}] TRY. Call ID: {} Attempt: {}/{} url: \"{}\"",
                                  __func__, in_call_id, attempt + 1, max_tries, in_request_data.url_s));

    // Back off only on retries, avoiding latency on the first attempt
    // if (attempt > 0)
    // {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // }


    st_result.res_curl = curl_easy_perform(curl_func);

    curl_easy_getinfo(curl_func, CURLINFO_RESPONSE_CODE, &st_result.http_status);
    httpStatus = st_result.http_status;
    st_result.request_err = std::string(errBuff);

    if (st_result.res_curl == CURLE_OK && httpStatus >= 200 && httpStatus < 300)
    {
      // #ifndef RELEASE
      Log::logMsg(fmt::format("[{}] SUCCESS: Call ID: {}, Has success code. Will force exit CURL loop, based on respond code: {}\nQuery: {}\n", __func__, in_call_id, static_cast<int>(st_result.res_curl), in_request_data.url_s ) );
      // #endif
      // v26.10.1 store base_url
      if (data_manager::last_successfull_url_s != base_url)
      {
        data_manager::last_successfull_url_s = base_url;
        Log::logMsg(fmt::format("[{}] NEW last successfully url: {}.\n", __func__, data_manager::last_successfull_url_s ) );
      }

      break; // Successful request
    }

    data_manager::last_successfull_url_s.clear(); // v26.10.1

    Log::logMsg(fmt::format("[{}] FAILURE. Call ID: {}, CURL code: {}, HTTP status: {}. {}",
                                  __func__, in_call_id, static_cast<int>(st_result.res_curl), httpStatus,
                                  (attempt + 1 < max_tries) ? "Retrying..." : "No retries left."));

    // user progress message in the error line
    if (in_request_data.how_many_tries > (attempt + 1))
      data_manager::strct_ui_share_data.error_message_line3 = fmt::format("Failed to fetch data from {}. Will try again.", base_url);

    // Fatal configuration errors that should never be retried
    if (st_result.res_curl == CURLE_UNSUPPORTED_PROTOCOL ||
        st_result.res_curl == CURLE_URL_MALFORMAT ||
        st_result.res_curl == CURLE_NOT_BUILT_IN)
    {
      Log::logMsg(fmt::format("[{}] SERVER FAILURE. Call ID: {}, Has Server Failure code '{}'. Will force exit CURL loop. {}\n", __func__, in_call_id, static_cast<int>(st_result.res_curl), base_url ) );
      #ifndef RELEASE
      data_manager::strct_ui_share_data.error_message_line3 = fmt::format("Server error: {}, URL: {}. Will try again.", static_cast<int>(st_result.res_curl), base_url);
      #endif
      break;
    }
  } // end loop over curl calls

  // 4. Detailed error diagnostics
  if (httpStatus != 200 || st_result.res_curl != CURLE_OK)
  {
    const std::string host_only_s = lmbda_extract_host(base_url);
    st_result.request_err = fmt::format("Call ID: {}, HTTP: {}, Curl Code: {} ({}) - Host: {}",
                                        in_call_id, httpStatus, static_cast<int>(st_result.res_curl),
                                        st_result.request_err.empty() ? curl_easy_strerror(st_result.res_curl) : st_result.request_err,
                                        host_only_s);

    if (httpStatus == 504)
      st_result.request_err.append(" : Gateway Timeout");
    else if (httpStatus == 429)
      st_result.request_err.append(" : Rate Limited / Too Many Requests");

    if (mxUtils::find_text(st_result.response_text, "Error", false) != std::string::npos)
    {
      Log::logMsg(fmt::format("[{}] Call ID: {}, Response contains server error: {}",
                                    __func__, in_call_id, st_result.response_text));
    }
  }
  else
  {
    st_result.request_err.clear();
  }

  // 5. Cleanup libcurl resources
  curl_easy_setopt(curl_func, CURLOPT_ERRORBUFFER, nullptr);
  curl_easy_cleanup(curl_func);

  if (p_headers)
  {
    curl_slist_free_all(p_headers);
  }

  data_manager::strct_ui_share_data.error_message_line3 = st_result.request_err; // v3.0.255.4 error_message_line3 will get the error. This will clear the last error message error_message_line3 had.

  // #else
  // st_result.reset();
  // st_result.res_curl    = -1;
  // st_result.request_err = "CURL is disabled in this build. Please enable CURL to use this feature.";
  // #endif // !DISABLE_CURL

  return st_result;
}

// -------------------------------------

NavAidInfo
data_manager::get_plane_airport_or_nearest_icao(const bool& inOnlySearchInDatabase, const double& inLat, const double& inLon, const bool inIsThread)
{
  // The following function will first try to figure out if the plane inside one of the airport boundary which is stored in the SQLITE database.
  // If not then it will fall back to the original code by using the XPSDK.
  bool flagFoundPlaneInAirportArea{ false }; // v3.303.8.3


  NavAidInfo navAid;
  navAid.navRef       = XPLM_NAV_NOT_FOUND;
  Point planePosition = dataref_manager::getPlanePointLocationThreadSafe();
  // v3.303.8.3 add the option to receive plane location relative to a pre-defined location and not to the current plane position.
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

//  NO Need to create fake boundaries, if an airport does not have a boundary, then it might be an inactive airport.
//  We could try and use "distance" instead, but that is good, only in edge cases.
  std::map<int, std::string> mapArgs = { { 1, planePosition.getLat_s() }, { 2, planePosition.getLon_s() } }; // v24.05.2
  query                              = mxUtils::format(query, mapArgs);



  #ifndef RELEASE
  Log::logMsg( fmt::format("{} Plane in airport boundary query:\n{}", __func__, query) );
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

        navAid.flag_fetched_from_db = true; // v24.03.1
      } // end if sql fetched at least one row (we will only pick the first row

    } // end if the prepare statement succeeded.

  } // end airport search using SQLite boundary query

  // v26.10.2 Not inside an airport boundary: is the point on a runway? Some airports have a boundary that does not cover the whole runway
  //          (example: 05S, touchdowns at the west end were outside it). Same runway test as the landing table (mx_is_plane_in_rw_area).
  if (!flagFoundPlaneInAirportArea && db_xp_airports.db_is_open_and_ready)
  {
    std::string query_rw = R"(
select xa.icao, xa.ap_name, xa.ap_lat, xa.ap_lon, xa.ap_elev, xa.icao_id
from xp_rw, xp_airports xa
where xa.icao_id = xp_rw.icao_id
and xp_rw.rw_no_1_lat between {1} - 0.1 and {1} + 0.1
and xp_rw.rw_no_1_lon between {2} - 0.15 and {2} + 0.15
and mx_is_plane_in_rw_area( {1}, {2}, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width ) = 1
limit 1
)";
    std::map<int, std::string> mapArgsRw = { { 1, planePosition.getLat_s() }, { 2, planePosition.getLon_s() } };
    query_rw                             = mxUtils::format(query_rw, mapArgsRw);

    if (const std::string stmt_rw_name = "fetch_airport_where_plane_is_on_its_runway";
        db_xp_airports.prepareNewStatement(stmt_rw_name, query_rw))
    {
      sqlite3_stmt* stmt_rw = db_xp_airports.mapStatements[stmt_rw_name];
      if (stmt_rw != nullptr && sqlite3_step(stmt_rw) == SQLITE_ROW)
      {
        navAid.setID(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt_rw, 0))));
        navAid.setName(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt_rw, 1))));
        navAid.lat       = static_cast<float>(sqlite3_column_double(stmt_rw, 2));
        navAid.lon       = static_cast<float>(sqlite3_column_double(stmt_rw, 3));
        navAid.height_mt = static_cast<float>(sqlite3_column_double(stmt_rw, 4));
        navAid.icao_id   = sqlite3_column_int(stmt_rw, 5);

        navAid.flag_fetched_from_db = true;
        flagFoundPlaneInAirportArea = true;

        #ifndef RELEASE
        Log::logMsg(fmt::format("[{}] {}/{} is outside the airport boundaries, but on a runway of {}.", __func__, planePosition.getLat_s(), planePosition.getLon_s(), navAid.getID()));
        #endif
      }
    }
  } // end runway search

  // v25.06.1 if we did not find lat/lon in the database
  if (navAid.lat * navAid.lon == 0.0)
  {
    navAid.lat = static_cast<float> (inLat);
    navAid.lon = static_cast<float> (inLon);
    navAid.synchToPoint ();
  }

  if (inOnlySearchInDatabase + inIsThread)
  {
    // Since we only want data from the database, we do not use the XPLMGetNavAidInfo() function.
    if (flagFoundPlaneInAirportArea && !inIsThread)
      navAid.navRef = XPLMFindNavAid(nullptr, navAid.ID, &navAid.lat, &navAid.lon, nullptr, xplm_Nav_Airport); // we are still using XPlane library to re-fetch information but this time we set the ICAO and lat/lon beforehand

    return navAid;
  }

  // We can search icao using the XPSDK because we are not in a thread
  if (flagFoundPlaneInAirportArea)
    navAid.navRef = XPLMFindNavAid(nullptr, navAid.ID, &navAid.lat, &navAid.lon, nullptr, xplm_Nav_Airport); // we are still using XPlane library to re-fetch information but this time we set the ICAO and lat/lon beforehand
  else
  {
    navAid.flag_fetched_from_db                = false; // v24.03.1
    navAid.flag_navDataFetchedFromXPLMGetNavAidInfo = true;  // v24.03.1
    navAid.navRef                                   = XPLMFindNavAid(nullptr, nullptr, &posVec2.x, &posVec2.y, nullptr, xplm_Nav_Airport);
  }

  if (navAid.navRef != XPLM_NAV_NOT_FOUND) // fetch more information
    XPLMGetNavAidInfo(navAid.navRef, &navAid.navType, &navAid.lat, &navAid.lon, &navAid.height_mt, &navAid.freq, &navAid.heading, navAid.ID, navAid.name, nullptr);
  else
    navAid.init();


  return navAid;
}

// -------------------------------------

NavAidInfo
data_manager::get_icao_info_closest_to_plane (const std::string &inICAO)
{
  NavAidInfo  nNavAid;
  const Point planePosition = dataref_manager::getPlanePointLocationThreadSafe ();
  ImVec2      posVec2 (static_cast<float> (planePosition.lat), static_cast<float> (planePosition.lon));

  nNavAid.navRef = XPLMFindNavAid (nullptr, inICAO.c_str (), &posVec2.x, &posVec2.y, nullptr, xplm_Nav_Airport); // find airport with the id: inICAO closest to plane
  if (nNavAid.navRef == XPLM_NAV_NOT_FOUND)
    nNavAid.init ();
  else
    XPLMGetNavAidInfo (nNavAid.navRef, &nNavAid.navType, &nNavAid.lat, &nNavAid.lon, &nNavAid.height_mt, &nNavAid.freq, &nNavAid.heading, nNavAid.ID, nNavAid.name, nullptr);

  return nNavAid;
}

// -----------------------------------


missionx::NavAidInfo
data_manager::get_and_guess_nav_info (const std::string &in_id_nav_name, const missionx::Point &prevPoint)
{
  NavAidInfo  nNavAid;
  auto lat = static_cast<float>(prevPoint.lat);
  auto lon = static_cast<float>(prevPoint.lon);
  nNavAid.navRef = XPLMFindNavAid (nullptr, in_id_nav_name.c_str (), &lat, &lon, nullptr, xplm_Nav_Airport|xplm_Nav_NDB|xplm_Nav_VOR|xplm_Nav_Fix|xplm_Nav_DME); // find navaid

  if (nNavAid.navRef == XPLM_NAV_NOT_FOUND)
    nNavAid.init ();
  else
    XPLMGetNavAidInfo (nNavAid.navRef, &nNavAid.navType, &nNavAid.lat, &nNavAid.lon, &nNavAid.height_mt, &nNavAid.freq, &nNavAid.heading, nNavAid.ID, nNavAid.name, nullptr);

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
  data_manager::lst_of_failed_3d_obj_to_load.clear(); // v26.1.1

  for (int i2 = 0; i2 < legs_i; ++i2)
  {
    const auto leg = parentOfLegNodes_ptr.getChildNode(mxconst::get_ELEMENT_LEG().c_str(), i2);
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

          if (!Utils::isElementExists(mapFilesAndErrors, file_name)) // if not already exists
          {
            bool is_virtual_file = false;
            std::string err;

            // check if file exists
            // Lambda function to search the file //
            const auto lmbda_get_3d_file_location = [file_name = file_name, missionPackageFolder = missionPackageFolder, inGlobalSettingsNode = inGlobalSettingsNode, missionFilePath = missionFilePath](bool& outIsVirtual)
            {
              // 1. search virtual
              // 2. Search as a physical file
              XPLMObjectRef g_object_ref;

              const int i = XPLMLookupObjects(file_name.c_str(), 0, 0, Utils::load_cb_dummy, &g_object_ref); // The dummy function is important so we won't crash
              if (i)
              {
                outIsVirtual = true;
                return file_name + " (" + mxUtils::formatNumber<int>(i) + ")";
              }

              // search as physical
              const fs::path display_file = file_name;
              #ifndef RELEASE
              const std::string filename     = display_file.filename ().string (); // debug
              const std::string relativePath = display_file.relative_path ().string (); // debug
              #endif

              if (display_file.filename () == display_file.relative_path ()) // this is probably locally defined 3D object and not an external library 3D object
              {
                // get 3D Object location from global_settings
                // Concatenate the file_name with the mission root folder (missionPackageFolder).
                bool              flag_found               = false;
                const IXMLNode    globalSettings_cloneNode = inGlobalSettingsNode.deepCopy ();
                const std::string obj3d_custom_folder_name = Utils::xml_get_attribute_value_drill (globalSettings_cloneNode, mxconst::get_ATTRIB_OBJ3D_FOLDER_NAME (), flag_found, mxconst::get_ELEMENT_FOLDERS ());

                #ifndef RELEASE
                const std::string debug_filePath = (obj3d_custom_folder_name.empty ()) ? missionPackageFolder + file_name : missionPackageFolder + obj3d_custom_folder_name + "/" + file_name;
                #endif // !RELEASE

                return (obj3d_custom_folder_name.empty ()) ? fmt::format("{}{}", missionPackageFolder, file_name) : fmt::format ("{}{}/{}", missionPackageFolder, obj3d_custom_folder_name, file_name );
              }

              // v3.303.14 check if file string starts with ".." which means relative to Mission Pack location
              if (file_name.find ("..") == 0 || ((file_name.find ('/') == std::string::npos) && (file_name.find ('\\') == std::string::npos))) // if we provide only 3D Object name and there is no path in it
                return missionFilePath + file_name; // construct file location relative to mission briefer folder location, example: "{xp11}/Custom Scenery/missionx/random/briefer" + file_name

              return "Custom Scenery/" + file_name; // v3.303.14 Add "Head of Custom Scenery" folder to the file name, which means "relative to X-Plane root folder".

              // return EMPTY_STRING;
            };

            std::string objectFile3D = lmbda_get_3d_file_location(is_virtual_file); // The function returns file location and if it is from a virtual object. It tests for Virtual and then constructs the Physical Path as string.

            if (is_virtual_file)
            {

              if (objectFile3D.empty()) // we did not find virtual file
              {
                ++err_counter_i;
                // err = fmt::format("[Validate 3D file] Not valid File: \"{}\", is not a valid virtual file. Copy the correct value from the library: EXPORT VIRTUAL PHYSICAL. See: https://developer.x-plane.com/article/library-library-txt-file-format-specification/", file_name);
                err = fmt::format("File: \"{}\", is NOT a valid virtual file. Copy the correct value from the library.txt file", file_name);
                Log::logMsg(err);
              }
            }
            else if ( fs::path file = objectFile3D; !exists(file) || !fs::directory_entry(file).is_regular_file())
            {
              ++err_counter_i;
              if (!exists (file))
                err = "File NOT Found: " + file.string ();
              else if (!fs::directory_entry (file).is_regular_file ())
                err = "Not a File ! " + file.string () + " is a folder.";
              else
                err = "Not a valid File: " + file.string () + " is of wrong type";

              Log::logMsg ("[Validate 3D file] " + err);
            }

            if (err.empty())
              Log::logMsg("[Validate 3D file] Found " + ((is_virtual_file) ? "Virtual File: " + file_name : "File: " + objectFile3D));
            else
              data_manager::lst_of_failed_3d_obj_to_load.emplace_back(err); // v26.1.1 add the error to the error list

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
data_manager::sqlite_test_db_validity(const dbase& inDB, const bool isThreaded)
{
  char *zErrMsg = nullptr;

  constexpr std::string_view sql = "select (select feature_version from dual) as feature_version, (select count(icao_id) from xp_airports) as count_xp_airports, (select count(icao_id) from xp_ap_ramps) as count_xp_ramps, (select count(lat) from xp_loc) as count_xp_loc from dual";


  reasultTable.clear();
  int rc = sqlite3_exec(inDB.db, sql.data(), &data_manager::callback_sqlite_data, nullptr, &zErrMsg);
  if (rc != SQLITE_OK)
  {
    set_flag_rebuild_apt_dat(true);

    if (zErrMsg == nullptr)
      Log::logMsg("[sqlite_test_db_validity] SQL error: " + inDB.last_err);
    else
      Log::logMsg("[sqlite_test_db_validity] SQL error: " + std::string(zErrMsg));

    sqlite3_free(zErrMsg);
  }
  else
  {
    Log::logMsg("[sqlite_test_db_validity] Information was gathered.");
    for (auto &val : reasultTable | std::views::values)
    {
      const auto feature_version = mxUtils::stringToNumber<int>(val["feature_version"]);

      // we only need the version information to decide if to rebuild APT.DAT database
      if (MX_FEATURES_VERSION != feature_version)
        set_flag_rebuild_apt_dat(true);

      missionx::mx_clock_time_strct osClock             = Utils::get_os_time();
      int                           iClockHourPicked    = osClock.hour;
      int                           iClockMinutesPicked = osClock.minutes; // v25.04.2 How many minutes passed since the start of the hour
      std::string_view              db_info_vu          = fmt::format("[{}:{}] {}", osClock.hour, osClock.minutes, data_manager::post_optimization_outcome);

      //data_manager::post_optimization_outcome = "Feature version:" + val["feature_version"] + ", Airports: " + val["count_xp_airports"] + ", Ramps: " + val["count_xp_ramps"] + ", Localizers: " + val["count_xp_loc"];
      data_manager::post_optimization_outcome = fmt::format("[{}:{} hh:min] Feature version: {}, Airports: {}, Ramps: {}, Localizers: {}", osClock.hour, osClock.minutes, val["feature_version"], val["count_xp_airports"], val["count_xp_ramps"], val["count_xp_loc"]);
      Log::logMsg(data_manager::post_optimization_outcome );
    }
  }

  if (get_flag_rebuild_apt_dat())
  {
    queFlcActions.push_back(mx_flc_pre_command::exec_apt_dat_optimization); // v3.0.255.3
    set_flag_rebuild_apt_dat(false); // reset
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
  ITCXMLNode    x_main_node = iDom.openFileHelper(inPathAndFile.c_str(), mxconst::get_ELEMENT_LNM_LittleNavmap().c_str(), &errMsg);

  if (errMsg.empty()) // we find main node
  {
    IXMLNode xFlightplan = x_main_node.getChildNode(mxconst::get_ELEMENT_LNM_Flightplan().c_str()).deepCopy();

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
data_manager::add_advanceSettingsDateTime_and_Weather_to_node(IXMLNode& xGlobalSettings, const IXMLNode& inPropNode, const std::string& inCurrentWeatherDatarefs_s)
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
      Log::logMsg("[random weather] Picked from vector: " + mxUtils::formatNumber<int>(rndPick));
      #endif // !RELEASE

      if (rndPick >= 0 && rndPick < static_cast<int> ( vecPicked.size () ) )
      {
        const int iWeatherPicked = vecPicked.at(rndPick);

        std::map<int, std::unordered_map<std::string, std::string>>* ptr_mapWeatherPreDefinedStrct = (xplane_ver_i < XP12_VERSION_NO) ? &data_manager::mapWeatherPreDefinedStrct_xp11 : &data_manager::mapWeatherPreDefinedStrct_xp12;
        assert(ptr_mapWeatherPreDefinedStrct != nullptr && "random weather pointer can't be null"); // debug

        #ifndef RELEASE
        Log::logMsg("[weather code] Picked: " + mxUtils::formatNumber<int>(iWeatherPicked));
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
            Log::logMsg("Weather Dataref Added as text:\n" + sWeatherDatarefAsText + "\n"); // debug
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
      const auto fuel_base_weight = prop_userDefinedMission_ui.getAttribNumericValue<float> ( mxconst::get_OPT_FUEL_BASE_WEIGHT(), 0.0f );

      // set the global_settings weight sub node
      Utils::xml_set_attribute_in_node<float> ( xBaseWeights, mxconst::get_OPT_PILOT_BASE_WEIGHT(), pilotWeight );
      Utils::xml_set_attribute_in_node<float> ( xBaseWeights, mxconst::get_OPT_STORAGE_BASE_WEIGHT(), storedWeight );
      Utils::xml_set_attribute_in_node<float> ( xBaseWeights, mxconst::get_OPT_PASSENGERS_BASE_WEIGHT(), passengersWeight );
      Utils::xml_set_attribute_in_node<float> ( xBaseWeights, mxconst::get_OPT_FUEL_BASE_WEIGHT(), fuel_base_weight); // v26.09.2
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
  auto root = Utils::xml_get_node_from_XSD_map_as_a_copy(mxconst::get_MISSION_ELEMENT());
  root.updateAttribute("From_lnm_fpln_to_mission",  mxconst::get_ATTRIB_NAME().c_str(),  mxconst::get_ATTRIB_NAME().c_str());
  root.updateAttribute("Converted from LittleNavMap flight plan to a Mission File", mxconst::get_ATTRIB_TITLE().c_str(), mxconst::get_ATTRIB_TITLE().c_str());

  xMAIN_NODE.addChild(root);


  auto xMissionInfo = xDummy_ptr.getChildNode(mxconst::get_ELEMENT_MISSION_INFO().c_str()).deepCopy();
  xMissionInfo      = init_littlenavmap_missionInfo(xMissionInfo);

  root.addChild(xMissionInfo);
  Utils::add_xml_comment(root);

  // v3.305.1 add global_settings
  auto xGlobalSettings = (!inout_xGlobalSettingsFromConversionFile.isEmpty() && !inGenerateNewGlobalSettingsNode_b) ? inout_xGlobalSettingsFromConversionFile.deepCopy() : Utils::xml_get_node_from_XSD_map_as_a_copy(mxconst::get_GLOBAL_SETTINGS());
  root.addChild(xGlobalSettings);
  Utils::add_xml_comment(root);

  IXMLNode xBriefer = Utils::xml_get_node_from_XSD_map_as_a_copy(mxconst::get_ELEMENT_BRIEFER());
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
      // const std::string desc_s          = Utils::xml_read_cdata_node(legData.xLeg, "Welcome pilot\nFly the suggested route, prepare the plane and the needed charts.\nBlue Skies\nMission-X");
      const std::string desc_s          = Utils::xml_get_text_or_cdata_text(legData.xLeg, "Welcome pilot\nFly the suggested route, prepare the plane and the needed charts.\nBlue Skies\nMission-X");

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
        auto xDisplayObject = Utils::xml_get_node_from_XSD_map_as_a_copy(mxconst::get_ELEMENT_DISPLAY_OBJECT());
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
  auto xObj3DTemplate = Utils::xml_get_node_from_XSD_map_as_a_copy("template_markers_obj3d"); // v3.0.303.3 this template holds the 4 3D marker types/files
  xObj3DTemplate.updateName(mxconst::get_ELEMENT_OBJECT_TEMPLATES().c_str());
  root.addChild(xObj3DTemplate); // v3.0.303.2 add Object3D Template

  Utils::add_xml_comment(root);
  Utils::add_xml_comment(root);


  // End element
  auto xEnd = Utils::xml_get_node_from_XSD_map_as_a_copy(mxconst::get_ELEMENT_END_MISSION());
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
      auto local_root       = Utils::xml_get_node_from_XSD_map_as_a_copy(mxconst::get_CONVERSION_ROOT_DOC());

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
data_manager::getPoint_as_stringFromPlaneCamera(const std::string& inDatarefStringLat, const std::string& inDatarefStringLon, char origin_c)
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
    // dataref_manager dm;
    // plane elevation in meters
    elevFt_d = missionx::dataref_manager::getElevation();
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
  return {latVal, lonVal, z}; // return Point
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
data_manager::gather_custom_acf_datarefs_as_a_thread(const std::string& inAcfPath, bool* bThreadIsRunning, const bool* bAbortThread)
{
  (*bThreadIsRunning)   = true;
  auto startThreadClock = std::chrono::steady_clock::now();

  fs::path acf_file = inAcfPath;
  if (exists(acf_file) && is_regular_file(acf_file))
  {
    std::unordered_set<std::string> customDatarefSet;
    customDatarefSet.clear();

    auto                            acfCustomDatarefSet = system_actions::search_datarefs_in_acf_file(acf_file.string());

    if (*bAbortThread)
    {
      (*bThreadIsRunning) = false;
      return;
    }

    fs::path acf_folder = acf_file.parent_path();
    if (is_directory(acf_folder))
    {
      std::unordered_set<std::string> objCustomDatarefs;
      for (const auto& file : fs::recursive_directory_iterator(acf_folder))
      {
        if (*bAbortThread)
        {
          (*bThreadIsRunning) = false;
          return;
        }

        const auto ext = mxUtils::stringToLower(fs::path(file).extension().string());
        if (is_regular_file(file) && ext == ".obj")
        {

          #ifndef RELEASE
          auto startThreadClock2 = std::chrono::steady_clock::now();
          #endif // ! RELEASE

          std::set<std::string> drefKeySetFromObjFile = system_actions::search_datarefs_in_obj_file(file);

          #ifndef RELEASE
          auto endCacheLoad = std::chrono::steady_clock::now();
          auto diff_cache   = endCacheLoad - startThreadClock2;
          auto duration     = std::chrono::duration<double, std::milli>(diff_cache).count();
          Log::logMsg(">> parsed: " + file.path().string() + ", Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec) ");
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
        if (it->find("sim/") == 0 || it->empty()) // if start with "sim/" delete
        {
          it = customDatarefSet.erase(it);
        }
        else
          ++it;
      }

      // write to file
      const std::string writable{ "y" };
      const std::string cacheFile = acf_folder.string() + "/" + mxconst::get_FILE_CUSTOM_ACF_CACHED_DATAREFS_NAME();

      std::ofstream fout;
      fout.open(cacheFile.c_str(), std::ofstream::out);
      // check open success
      if (fout.fail() || fout.bad())
      {
        Log::logMsg("[Error] Fail to find/create target file: [" + cacheFile + "]. Skip operation !!!");
      }
      else
      {
        // write all strings as line-by-line data sets
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
  Log::logAttention("*** Finished [ gather_custom_acf_datarefs_as_a_thread() ]. Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec)  ****");


  (*bThreadIsRunning) = false;
} // gather_custom_acf_datarefs_as_a_thread

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
data_manager::getTitleOrNameFromNode(const mx_base_node& inBaseNode)
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
      const std::string& name    = vecDrefAndVal.at(0);
      const std::string& value_s = vecDrefAndVal.at(1);
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

std::string
data_manager::get_translate_of_mission_subcategory_code (const int in_missionCodeType, int in_mission_subcategory, IXMLNode &outMetaNode)
{
  // v25.05.1 Changed the function logic so we won't be dependent on the mx_mission_subcategory_type.
  // We will focus only on the "in_missionCodeType" and a "key word" in the picked sub category.
  // We will remove the "data_manager::mapMissionCategoriesCodes" from the code.

  if (in_mission_subcategory < 0)
    in_mission_subcategory = 0;

  if (mxUtils::isElementExistsInVec<int> (data_manager::vecMissionCategoriesCodes, in_missionCodeType))
  {
    outMetaNode.updateAttribute ( "", mxconst::get_ATTRIB_SURPRISE_ME_SUB_CAT_B ().c_str (), mxconst::get_ATTRIB_SURPRISE_ME_SUB_CAT_B ().c_str ());

    switch (in_missionCodeType)
    {
      case static_cast<int> (missionx::mx_ui_mission_type::medevac):
      {
        assert (in_mission_subcategory < static_cast<int>(data_manager::strct_ui_share_data.medevac_arr.size ()) && fmt::format ("[{}] subcategory index: '{}', is out of bound for medevac.", __func__, in_mission_subcategory).c_str ());
        const std::string code_text = data_manager::strct_ui_share_data.medevac_arr.at (in_mission_subcategory);
        if (mxUtils::stringToLower (code_text).find (mxconst::SEARCH_SURPRISE_ME_TEXT) != std::string::npos)
          Utils::xml_set_attribute_in_node <bool>(outMetaNode, mxconst::get_ATTRIB_SURPRISE_ME_SUB_CAT_B (), true, outMetaNode.getName () ); // special attribute that will affect how the mission will be built.

        return mxconst::get_GENERATE_TYPE_MEDEVAC ();
      }
      break;
      case static_cast<int> (missionx::mx_ui_mission_type::cargo):
      {
        assert (in_mission_subcategory < static_cast<int>( data_manager::strct_ui_share_data.cargo_arr.size ()) && fmt::format ("[{}] subcategory index: '{}', is out of bound for cargo.", __func__, in_mission_subcategory).c_str ());
        const std::string code_text = data_manager::strct_ui_share_data.cargo_arr.at (in_mission_subcategory);
        return mxconst::get_GENERATE_TYPE_CARGO ();
      }
      break;
      case static_cast<int> (missionx::mx_ui_mission_type::oil_rig):
      {
        assert (in_mission_subcategory < static_cast<int>(data_manager::strct_ui_share_data.oilrig_arr.size ()) && fmt::format ("[{}] subcategory index: '{}', is out of bound for oilrig.", __func__, in_mission_subcategory).c_str ());
        const std::string code_text = data_manager::strct_ui_share_data.oilrig_arr.at (in_mission_subcategory);
        // check if code_text has "med" in the cargo text
        if (mxUtils::stringToLower (code_text).find ("med") != std::string::npos)
          return mxconst::get_GENERATE_TYPE_OILRIG_MED ();

        return mxconst::get_GENERATE_TYPE_OILRIG_CARGO ();
      }
      break;
      default:
        break;
    } // end switch
  }


  return "";


}

// -------------------------------------

void
data_manager::addMessageToHistoryList(const std::string& inWho, const std::string &inText, const mxVec4 inColor)
{

  if (listOfMessageStoryMessages.size() > MAX_MX_PAD_MESSAGES)
    listOfMessageStoryMessages.pop_front();

  listOfMessageStoryMessages.emplace_back(inWho, inText, inColor);
}


// -------------------------------------


bool
data_manager::find_and_read_template_file (const std::string &inFileName)
{
  std::string       err;
  const std::string inPath = mx_folders_properties.getAttribStringValue (mxconst::get_FLD_RANDOM_TEMPLATES_PATH(), "", err); // get path to template folder

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

// -------------------------------------

structs::def_strct_acf_info data_manager::get_current_acf(){ return data_manager::active_acf_info; }

// -------------------------------------

void data_manager::trigger_acf_change()
{
  missionx::data_manager::acf_refresh_info();

  // v26.10.2 record the plane change in the stats database (plane_meta)
  if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
    data_manager::stats_meta_plane_loaded_cai();

  if (!data_manager::active_acf_info.active_acf.empty()
      && !data_manager::active_acf_info.active_acf_path.empty()
      && data_manager::active_acf_info.prev_acf != missionx::data_manager::active_acf_info.active_acf)
  {
    // also calls get_plane_icao() and set_acf()
    data_manager::acf_gather_cargo_info();
  }
}

// -------------------------------------

void
data_manager::acf_refresh_info()
{  
  char outFileName[512]{ 0 };
  char outPathAndFile[2048]{ 0 };
  XPLMGetNthAircraftModel ( XPLM_USER_AIRCRAFT, outFileName, outPathAndFile ); // we will only return the file name

  // v26.09.2
  const auto prev_acf = active_acf_info.active_acf;
  data_manager::active_acf_info = {.active_acf = outFileName, .active_acf_path = outPathAndFile, .active_acf_icao = missionx::dataref_manager::get_plane_icao(), .prev_acf = prev_acf};
 
  missionx::dataref_manager::gather_active_acf_base_info_for_llm();

  // return data_manager::active_acf_info;
}


// -------------------------------------


void data_manager::acf_gather_cargo_info()
{
    missionx::Inventory::gather_acf_cargo_data (data_manager::mapInventories[mxconst::get_ELEMENT_PLANE ()], true);
  data_manager::dref_acf_station_max_kgs_f_arr.setAndInitializeKey ("sim/aircraft/weight/acf_m_station_max");
  missionx::dataref_param::set_dataref_values_into_xplane (data_manager::dref_m_stations_kgs_f_arr); // force original weight on the new plane

  missionx::data_manager::queFlcActions.push_back (missionx::mx_flc_pre_command::gather_acf_custom_datarefs); // v3.303.13 make sure we will have the plane dataref information
}

void
data_manager::acf_refresh_and_gather_active_plane_information()
{
  data_manager::acf_refresh_info();
  data_manager::acf_gather_cargo_info();
}

// -------------------------------------

IXMLNode
data_manager::get_default_overpass_urls_node ()
{
  IXMLNode overpass_node = IXMLNode::emptyIXMLNode;
  IXMLDomParser dummy;
  auto xDomOverpass = dummy.parseString ( mxconst::get_OVERPASS_XML_URLS().c_str(), mxconst::get_ELEMENT_OVERPASS ().c_str() );
  if (!xDomOverpass.isEmpty())
  {
    overpass_node = xDomOverpass.deepCopy ();
  }

  return overpass_node;
}

// -------------------------------------

std::vector<std::string>
data_manager::get_default_overpass_urls_as_vector (const IXMLNode &inNode)
{
  std::vector<std::string > vecUrls;

  if (!inNode.isEmpty ())
  {
    int nodeCounter_i = inNode.nChildNode(mxconst::get_ELEMENT_URL ().c_str ());
    for (int i1=0; i1 < nodeCounter_i; ++i1)
    {
      const auto node = inNode.getChildNode (mxconst::get_ELEMENT_URL ().c_str (), i1);
      if (const auto text = Utils::xml_get_text (node);
          !text.empty ())
        vecUrls.emplace_back (text);
    }
  }

  return vecUrls;
}

// -------------------------------------


std::string missionx::data_manager::gen_get_next_overpass_url(const std::string & in_last_successfull_url, const bool in_ignore_user_preference)
{
  // v26.10.1
  if (!in_last_successfull_url.empty())
    return in_last_successfull_url;

  static size_t last_index = 0;
  const bool    b_lock_to_preferred_url = missionx::system_actions::pluginSetupOptions.getNodeText_type_1_5<bool>(mxconst::get_SETUP_LOCK_OVERPASS_URL_TO_USER_PICK(), false);
  std::string   url        = gen_get_user_preferred_overpass_url();

  if ( !b_lock_to_preferred_url || in_ignore_user_preference )
  {
    const auto lmbda_get_next_url = [&]()-> std::string
    {
      for (unsigned int trials = 0; trials < data_manager::vecOverpassUrls.size(); ++trials)
      {
        if (data_manager::strct_ui_share_data.map_ui_user_picks_overpass_urls.contains(static_cast<int>(last_index))
          && data_manager::strct_ui_share_data.map_ui_user_picks_overpass_urls[static_cast<int>(last_index)])
        {
          return (data_manager::vecOverpassUrls.size() > last_index) ? data_manager::vecOverpassUrls.at(last_index) : "";
        }

        if ((++last_index) >= data_manager::vecOverpassUrls.size())
          last_index = 0;
      }

      return "";
    }; // end lambda

    url = lmbda_get_next_url();

    ++last_index;
    // check if we need to reset
    if (last_index >= data_manager::vecOverpassUrls.size() )
      last_index = 0;
  }

  // fallback to default url ?
  if (url.empty())
    url = mxconst::get_DEFAULT_OVERPASS_URL();

  return url;
}

// -------------------------------------

std::string missionx::data_manager::gen_get_user_preferred_overpass_url()
{
  return missionx::system_actions::pluginSetupOptions.getNodeText_type_6(mxconst::get_OPT_OVERPASS_URL(), mxconst::get_DEFAULT_OVERPASS_URL());
}


// -------------------------------------


void
data_manager::check_cache_folder (const std::string &in_cache_folder_name)
{
  std::filesystem::path cache_path(in_cache_folder_name);

  // Check if the directory exists
  if (!std::filesystem::exists(cache_path)) {
    Log::logMsg ( fmt::format( "Cache folder '{}' does not exist. Creating it...\n", in_cache_folder_name) );
    // Attempt to create the directory
    try {
      if (std::filesystem::create_directories(cache_path)) {
        Log::logMsg ( fmt::format( "Cache folder created successfully.\n" ) );
      } else {
        // This 'else' might be hit if create_directories returns false,
        // which can happen if a file with the same name exists,
        // or if there are permission issues, but no exception is thrown.
        Log::logMsg ( fmt::format( "Failed to create cache folder (unknown reason, no exception).\n" ) );
      }
    } catch (const std::filesystem::filesystem_error& e) {
      // Catch specific filesystem errors (e.g., permission denied)
      Log::logMsg ( fmt::format( "Error creating cache folder: '{}'\n", e.what() ) );
    }
  } else {
    Log::logMsg ( fmt::format( "Cache folder '{}' already exists.\n", in_cache_folder_name ) );
  }
}

// -------------------------------------


void
data_manager::fetch_overpass_info_analyze_thread (missionx::base_thread::strct_thread_state *inoutThreadState
                                                , missionx::structs::strct_osm_query *q
                                                , const std::set<enums::mx_osm_region_enum>& in_search_bbox_area_list
                                                , const std::map<missionx::enums::mx_osm_region_enum, missionx::structs::strct_bbox> &in_map_bbox)
{
  if (inoutThreadState == nullptr)
    return; // abort function execution

  assert (inoutThreadState != nullptr && fmt::format ("[{}] Thread state is not initialized correctly.", __func__).c_str ());

  inoutThreadState->flagIsActive       = true;

  if (q == nullptr)
    return;

  q->start_time = std::chrono::steady_clock::now ();

  // check [abort]
  if (inoutThreadState->flagAbortThread)
  {
    q->xml_target_nd_node     = IXMLNode::emptyIXMLNode;
    q->xml_target_way_element = IXMLNode::emptyIXMLNode;
    return;
  }

  for (const auto &[loc, bbox] : in_map_bbox)
  {
    // apply restriction. Skip areas not in the "set_areas_to_work_on"
    if ( ! in_search_bbox_area_list.contains (loc) )
    {
      Log::logMsg( fmt::format("[{}] Skipping area: \"{}\". Not included in the search area.", __func__, Utils::get_mx_osm_region_trans(loc)));
      continue;
    }


    std::string filledQuery = q->q_text;
    q->q_bbox               = bbox.get_bbox ();
    q->q_short_bbox_fmt     = bbox.get_bbox_short ();

    if (const size_t pos = filledQuery.find (q->BBOX_STR);
      pos != std::string::npos)
    {
      bool found_valid_count_node = false;
      int  iTry                   = 0;

      filledQuery = mxUtils::replaceAll (filledQuery, q->BBOX_STR, q->q_bbox);
      filledQuery = mxUtils::replaceAll (filledQuery, q->ALL_BBOX_STR, q->q_all_bbox);

      q->osm_queries[mxUtils::get_mx_osm_region_trans (loc)] = filledQuery;

// #ifdef MX_ENABLE_HTTP_REQUESTS
      std::string fullQuery = fmt::format("data={}", curl_easy_escape(nullptr, filledQuery.c_str(), 0));
// #else
//       std::string fullQuery = fmt::format("data={}", filledQuery.c_str());
// #endif // !DISABLE_CURL


      q->sanitized_id   = mxUtils::sanitize_text (q->id, " ,'\"\t\n");
      q->sanitized_bbox = mxUtils::sanitize_text (q->q_short_bbox_fmt, " ,'\"\t\n");
      // Write to cached output folder
      const std::string filename = fmt::format ("{}/cached_{}_{}.xml", q->cache_folder, q->sanitized_id, q->sanitized_bbox);
      structs::strct_curl_result strct_result;

      bool used_cached_file = false;

      try
      {
        if (std::filesystem::exists (filename))
        {
          if (std::ifstream inFile (filename);
            inFile.is_open ())
          {
            strct_result.response_text.assign ((std::istreambuf_iterator<char> (inFile)), std::istreambuf_iterator<char> ());
            inFile.close ();
            // Check cached result has valid "count" node data
            IXMLDomParser xmlParser;
            auto          root = xmlParser.parseString (strct_result.response_text.c_str ()).deepCopy ();

            IXMLNode nodeCount = Utils::xml_get_node_from_node_tree_IXMLNode (root, "count", true);
            if (nodeCount.isEmpty ())
            {
              used_cached_file = false;
              try
              {
                std::filesystem::remove (filename);
              }
              catch (const std::exception &ex)
              {
                Log::logMsg (fmt::format ("[Exception during cache file removal]\n\t{}\n", ex.what ()));
                Log::logMsg (fmt::format ("[Cache filename]\n\t{}\n", filename));
              }
            }
            else
            {
              used_cached_file = true;
              Log::logMsg (fmt::format ("Using cache {}\n", filename));
            }

          }
        }
      }
      catch (const std::exception &ex)
      {
        Log::logMsg (fmt::format ("[Exception during cache creation]\n\t{}\n", ex.what ()));
      }

      // check [abort]
      if (inoutThreadState->flagAbortThread)
      {
        q->xml_target_nd_node     = IXMLNode::emptyIXMLNode;
        q->xml_target_way_element = IXMLNode::emptyIXMLNode;
        return;
      }

      std::string overpass_url;

      if (!used_cached_file)
      {
        // Call OVERPASS using cURL and make sure we do not have ABORT request.
        for (size_t loop1 = 0; loop1 < data_manager::strct_ui_share_data.map_ui_user_picks_overpass_urls.size() && !found_valid_count_node; ++loop1)
        {
          // check [abort]
          if (inoutThreadState->flagAbortThread)
          {
            q->xml_target_nd_node     = IXMLNode::emptyIXMLNode;
            q->xml_target_way_element = IXMLNode::emptyIXMLNode;
            return;
          }

          overpass_url = data_manager::gen_get_next_overpass_url(data_manager::last_successfull_url_s);
          // v26.08.1
          missionx::structs::curl_request_data curl_conn_data = {.url_s = overpass_url, .post_request_field_s = fullQuery, .headers = {mxconst::OVERPASS_POST_HEADER} };
          strct_result = get_curl_request_respond(curl_conn_data);

          Log::logMsg(fmt::format("[{}] URL: {}", __func__, overpass_url));

// #ifndef MX_ENABLE_HTTP_REQUESTS
//           int CURLE_OK = 0;
// #endif // !DISABLE_CURL


          if (CURLE_OK == strct_result.res_curl)
          {
            // v26.10.1
            // if (data_manager::last_successfull_url_s != overpass_url)
            //   data_manager::last_successfull_url_s = overpass_url;
            found_valid_count_node = true;
            Log::logMsg(fmt::format("[{}] Query ID: [{}] BBOX: {}\nresult: {}\n", __func__, q->id, q->q_bbox, strct_result.response_text));
            std::this_thread::sleep_for(std::chrono::seconds(2)); // wait for 2 seconds before sending a new request
          }
          else
          {
            // res != CURLE_OK
            // #ifdef MX_ENABLE_HTTP_REQUESTS
            Log::logMsg(fmt::format("[{}]\tCurl error: \n\t{}\n", __func__, curl_easy_strerror(strct_result.res_curl)));
            // #endif // !DISABLE_CURL

            std::this_thread::sleep_for(std::chrono::seconds(5)); // Sleep longer
          }

        } // End CURL call loop, over overpass url

      } // end if "not to use cache"

      // -------------
      // check [abort]
      // -------------
      if (inoutThreadState->flagAbortThread)
      {
        q->xml_target_nd_node     = IXMLNode::emptyIXMLNode;
        q->xml_target_way_element = IXMLNode::emptyIXMLNode;
        return;
      }


      // -------------
      // Handle response data
      // -------------
      if ( found_valid_count_node + used_cached_file )
      {
        IXMLDomParser xmlParser;
        auto          xml_count_node      = xmlParser.parseString (strct_result.response_text.c_str ()).deepCopy ();
        IXMLNode      nodeCount = Utils::xml_get_node_from_node_tree_IXMLNode (xml_count_node, "count", true);
        if (!nodeCount.isEmpty ())
        {
          nodeCount.updateName ("tags");
          nodeCount.updateAttribute (q->sanitized_id.c_str (), "id", "id");
          nodeCount.updateAttribute (mxUtils::get_mx_osm_region_trans (loc).c_str (), "region", "region");
          nodeCount.updateAttribute (q->q_bbox.c_str (), "bbox", "bbox");
          nodeCount.updateAttribute (q->sanitized_bbox.c_str (), "cached_bbox", "cached_bbox");

          q->total_way_count = 0;
          auto tag_node      = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (nodeCount, "tag", "k", "ways", false, false);
          q->total_way_count = mxUtils::stringToNumber<int> (Utils::xml_get_attribute_value (tag_node, "v", "0"));

          if (!q->xml_q_tags_header_node.isEmpty () && (q->total_way_count > 0))
          {
            q->xml_q_tags_header_node.addChild (nodeCount);

            // write to cache file
            if (std::ofstream outFile (filename);
              outFile.is_open ())
            {
              outFile << strct_result.response_text;
              outFile.close ();
            }
          }
        }
        else
        {
          Log::logMsg (fmt::format ("Overpass Url: {}", overpass_url));
          Log::logMsg (fmt::format ("\tRegion: {}, has no <count>.\n Query:\n{}\nNode Element:\n\t{}\n", mxUtils::get_mx_osm_region_trans (loc), filledQuery, Utils::xml_get_node_content_as_text (nodeCount, "> Empty Node")));
          Log::logMsg (fmt::format ("\tResponse: {}\n", strct_result.response_text.c_str ()));
        }
      }
    }
  }

  q->end_time = std::chrono::steady_clock::now ();
}


 // -------------------------------------


void
data_manager::fetch_ways_and_target_node_from_overpass_thread (missionx::base_thread::strct_thread_state *inoutThreadState, std::string *outStatusMessage, missionx::structs::strct_osm_query *q)
{
  // https://wiki.openstreetmap.org/wiki/Overpass_API

  if (q == nullptr)
    return;

  bool        useCache = false;
  std::string response_text;
  missionx::structs::strct_curl_result st_curl_result;

  q->start_time = std::chrono::steady_clock::now ();
  q->flag_data_respond_was_valid = false;

  // prepare request
  std::string filledQuery = q->q_text;
  const size_t pos_all_bbox = filledQuery.find(q->ALL_BBOX_STR);
  const size_t pos_bbox = filledQuery.find(q->BBOX_STR);

// #ifdef MX_ENABLE_HTTP_REQUESTS

  if ((pos_bbox == std::string::npos) && (pos_all_bbox == std::string::npos) )
  {
    Log::logMsg (fmt::format ("[{}] No Valid BBOX filter was found. Aborting target search.\n", __func__) );
    q->end_time = std::chrono::steady_clock::now ();
    return;
  }

  filledQuery = mxUtils::replaceAll (filledQuery, q->BBOX_STR, q->q_bbox); // replace all occurrences.
  filledQuery = mxUtils::replaceAll (filledQuery, q->ALL_BBOX_STR, q->q_all_bbox); // replace all occurrences.
  q->q_unescaped_request = fmt::format ("data={}", filledQuery.c_str ());
  q->q_escaped_request = fmt::format ("data={}", curl_easy_escape (nullptr, filledQuery.c_str (), 0));

  // #ifndef RELEASE
  Log::logMsg (fmt::format ("[{}] >>> 1. Pick a random <way> <<\n Query: {}\n", __func__, q->q_unescaped_request));
  // #endif

  const std::string filename = fmt::format ("{}/cached_ways_{}_{}.xml", q->cache_folder, q->id, q->q_short_bbox_fmt);

  // ------------------
  // get <way> from CACHE
  // ------------------

  try
  {

    if (std::filesystem::exists (filename))
    {
      if (std::ifstream inFile (filename);
        inFile.is_open ())
      {
        response_text.assign ((std::istreambuf_iterator<char> (inFile)), std::istreambuf_iterator<char> ());
        inFile.close ();
        // Check cached result has ways node data
        IXMLDomParser xmlParser;
        IXMLResults   parseResult;
        auto          xml_osm_node = xmlParser.parseString (response_text.c_str (), "osm", &parseResult).deepCopy ();
        // debug and validations
        Log::logMsg (fmt::format ("Parse result: {} {}, Lines: {}", static_cast<int> (parseResult.errorCode), (static_cast<int> (parseResult.errorCode) == 0) ? "(ok)" : "", parseResult.nLine));

        if (parseResult.errorCode != IXMLError_None)
        {
          Log::logMsg (fmt::format ("Returned XML Error Code: {}\nRaw Result XML: {}\n", IXMLDomParser::getErrorMessage (parseResult.errorCode), Utils::xml_get_node_content_as_text (xml_osm_node)));
          useCache = false;
          std::filesystem::remove (filename);
        }
        else
        {

          int         nodeCount = xml_osm_node.nChildNode (mxconst::get_ELEMENT_WAY_OSM().c_str()); // second validation
          std::string root_name = xml_osm_node.getName (); // debug

          if (nodeCount < 1)
          {
            useCache = false;
            std::filesystem::remove (filename);
            q->flag_data_respond_was_valid = false;
          }
          else
          {
            useCache                       = true;
            q->flag_data_respond_was_valid = true;
            Log::logMsg (fmt::format ("Using cache {}\n", filename));
          }
        }
      }
    }
  }
  catch (const std::exception &ex)
  {
    useCache = false;
    Log::logMsg (fmt::format ("[Exception during cache creation]\n\t{}\n", ex.what ()));
    Log::logMsg (fmt::format ("[Expected filename]: {}\n", filename));
  }
  // end reading cache file

  // check [abort]
  if (inoutThreadState->flagAbortThread)
  {
    q->flag_data_respond_was_valid = false;
    q->total_way_count             = 0;
    q->reset_osm_target_data_only();
    return;
  }


  // ------------------
  // get <way> FROM OVERPASS if cache was not used
  // ------------------
  if (!useCache)
  {
    bool     flag_curl_results_are_ok = false;
    size_t   curl_call_counter_i_2       = 0;

    // Call Overpass up to "data_manager::vecOverpassUrls.size ()" times
    while (!flag_curl_results_are_ok && curl_call_counter_i_2 < data_manager::vecOverpassUrls.size ())
    {
      std::this_thread::sleep_for (std::chrono::seconds (2)); // wait before sending a new request

      // Get overpass url
      // const auto &overpass_url = (curl_call_counter_i_2 < data_manager::vecOverpassUrls.size ()) ? data_manager::vecOverpassUrls.at (curl_call_counter_i_2)
      //                                                                                          : mxconst::get_DEFAULT_OVERPASS_URL ();

      const auto &overpass_url = data_manager::gen_get_next_overpass_url(data_manager::last_successfull_url_s);

      #ifndef RELEASE
      Log::logMsg (fmt::format ("[{}] Fetching way data from overpass: {}.\nFilter:\n{}\n<---\n", __func__, overpass_url, q->q_unescaped_request));
      #endif

      response_text.clear ();

      // ----------------------------------
      // -- CURL REQUEST - Gather the <way> sub-nodes information
      // ----------------------------------
      std::string errBuff_s;
      st_curl_result.reset(); // v26.03.1
      // v26.08.1
      missionx::structs::curl_request_data curl_conn_data = {.url_s = overpass_url, .post_request_field_s = q->q_unescaped_request, .headers = {mxconst::OVERPASS_POST_HEADER} };
      st_curl_result = data_manager::get_curl_request_respond(curl_conn_data);

      response_text  = st_curl_result.response_text;
      errBuff_s      = st_curl_result.request_err;

      curl_call_counter_i_2++;

      if (errBuff_s.empty())
      {
        if (mxUtils::find_text (response_text, "Error", false) != std::string::npos)
        {
          Log::logMsg (fmt::format ("[{}] There might be an issue with the retrieved data from: {}.\n Query Text: {} \nResponse text: {}\n<-- end response --\n Will try another url", __func__, overpass_url, filledQuery, response_text));
          std::this_thread::sleep_for (std::chrono::seconds (2)); // wait for 2 seconds before sending a new request
        }
        else
          flag_curl_results_are_ok = true;
      } // end internal test


      // check [abort]
      if (inoutThreadState->flagAbortThread)
      {
        q->flag_data_respond_was_valid = false;
        q->total_way_count             = 0;
        q->reset_osm_target_data_only();
        return;
      }

    } // end while loop


    if (st_curl_result.res_curl == CURLE_OK)
    {
      q->flag_data_respond_was_valid = true;
      Log::logMsg (fmt::format ("[{}] Query ID: {} BBOX: {}\n", __func__, q->id, q->q_bbox));

      // write to cache file
      if (std::ofstream outFile (filename);
        outFile.is_open ())
      {
        outFile << response_text << "\n";
        outFile.close ();
      }
    }
    else
    {
      // res != CURLE_OK
      response_text.clear ();
      q->flag_data_respond_was_valid = false;
      Log::logMsg(fmt::format("[{}]\tCurl error: \n\t{}", __func__, curl_easy_strerror(st_curl_result.res_curl)));
    }

  } // End while loop

  // sleep for 2 seconds
  std::this_thread::sleep_for (std::chrono::seconds (2)); // wait for 2 seconds before sending a new request

  if (!response_text.empty ())
  {
    // -------------------
    // -- Parse OSM node to get the <way> sub-nodes.
    // -------------------
    IXMLDomParser xmlParser;
    IXMLResults   parseResult;
    auto          xml_osm_node = xmlParser.parseString (response_text.c_str (), "osm", &parseResult).deepCopy ();

    #ifndef RELEASE
    Log::logMsg (fmt::format ("[{}] (xml_osm_node) Parse result osm: {}, nLine: {}\n", __func__, static_cast<int> (parseResult.errorCode), parseResult.nLine));
    #endif

    // Get <way> sub-nodes in <osm> root node.
    q->total_way_count = xml_osm_node.nChildNode (mxconst::get_ELEMENT_WAY_OSM().c_str());

    if (q->total_way_count > 0)
    {
      // Get shuffled vector index
      const std::vector<int> vecShuffleWayNodes = Utils::getShuffledIndexVector_byType<int> (q->total_way_count);

      for (const auto &way_shuffled_index : vecShuffleWayNodes)
      {
        // store the target <way>
        q->xml_target_way_element = xml_osm_node.getChildNode(mxconst::get_ELEMENT_WAY_OSM().c_str(), way_shuffled_index).deepCopy();
        #ifndef RELEASE
        Log::logMsg (fmt::format ("[{}] Picked <way>: {}\n", __func__, Utils::xml_get_node_content_as_text (q->xml_target_way_element)));
        #endif

        const int nd_count = q->xml_target_way_element.nChildNode (mxconst::get_ELEMENT_ND_OSM().c_str());
        if (nd_count < 1)
          continue;

        auto rand_node_num = Utils::getRandomIntNumber (0, nd_count - 1);

        // ------------
        // fetch node (nd) and a "ref" attribute from it if there is no lat/lon
        // ------------

        auto nd_node = q->xml_target_way_element.getChildNode(mxconst::get_ELEMENT_ND_OSM().c_str(), rand_node_num);

        NavAidInfo na;
        na.lat = Utils::readNodeNumericAttrib<float>(nd_node, mxconst::get_ATTRIB_LAT_OSM(), 0.0f);
        na.lon = Utils::readNodeNumericAttrib<float>(nd_node, mxconst::get_ATTRIB_LONG_OSM(), 0.0f);
        // Sometimes the <nd> node might hold the "lat/lon" coordinate
        if (na.is_lat_lon_valid())
        {
          q->xml_target_nd_node = nd_node.deepCopy();
          data_manager::find_vector_between_two_osm_nodes(q, rand_node_num);
          if (q->is_osm_target_valid())
            break; // exit the loop and function
          na.init();
          q->reset_osm_target_data_only();
        }

        // --------------------------
        // -------------------------
        // ------------------------
        // Fallback and try to read <node> from <nd id=""> attribute
        // ------------------------

        bool   flag_curl_results_are_ok = false;
        size_t url_node_loop_counter_i  = 0;

        std::this_thread::sleep_for(std::chrono::seconds(2)); // wait few seconds before sending a new request

        // Call Overpass to get the data for the picked <nd>.
        // We will loop over the "data_manager::vecOverpassUrls.size ()" if there is any error.
        while (!flag_curl_results_are_ok && url_node_loop_counter_i < data_manager::vecOverpassUrls.size ())
        {
          const auto& overpass_url = data_manager::gen_get_next_overpass_url(data_manager::last_successfull_url_s);

          // ---------------------------
          // Construct NODE search from "ref" attribute
          // ---------------------------
          response_text.clear();
          auto node_ref_id              = Utils::readAttrib(nd_node, "ref", "", "-1", true);
          filledQuery                   = fmt::format("[out:xml][timeout:20];node({})({});out body;", q->q_all_bbox, node_ref_id);
          const std::string s_curl_node_query = fmt::format("data={}", curl_easy_escape(nullptr, filledQuery.c_str(), 0));

          // #ifndef RELEASE
          auto nd_ref_start_time = std::chrono::steady_clock::now();
          Log::logMsg(fmt::format("[{}] >>> 2. Fetch Node ref: {} for way id: {} <<<\nFrom Overpass URL: {}\nFilter: {}\n<---\n", __func__, Utils::readAttrib(nd_node, mxconst::get_ATTRIB_REF_OSM(), "n/a ref"), Utils::readAttrib(q->xml_target_way_element, mxconst::get_ATTRIB_ID(), "n/a way"), overpass_url, filledQuery));
          // #endif

          // ----------------------------------
          // -- CURL REQUEST - Gather <node> information using the <nd id=""> attribute.
          // ----------------------------------
          std::string errBuff_s;
          st_curl_result.reset(); // v26.03.1
          // v26.08.1
          missionx::structs::curl_request_data curl_conn_data = {.url_s = overpass_url, .post_request_field_s = s_curl_node_query, .headers = {mxconst::OVERPASS_POST_HEADER} };
          // try 2 times to fetch <nd> information
          st_curl_result = data_manager::get_curl_request_respond(curl_conn_data);
          response_text  = st_curl_result.response_text;
          errBuff_s      = st_curl_result.request_err;

          url_node_loop_counter_i++;
          if (st_curl_result.res_curl == CURLE_OK)
          {
            if (mxUtils::find_text(response_text, "Error", false) != std::string::npos)
            {
              Log::logMsg(fmt::format("[{}] Found Error string in respond text from: {}.\n Query Text: {}\n\nResponse text: {}\n<-- end response --\n Will try another url", __func__, overpass_url, filledQuery, response_text));
              Log::logMsg(fmt::format("[{}]\tCurl error: \n\t{}\n", __func__, curl_easy_strerror(st_curl_result.res_curl)));
              std::this_thread::sleep_for(std::chrono::seconds(2)); // wait few seconds before sending a new request
            }
            else
            {
              flag_curl_results_are_ok = true;
              break;
            }
          } // end internal test
          else
          {
            // respond code was not CURLE_OK
            Log::logMsg(fmt::format("[{}] There might be an issue with the retrieved data from: {}.\n Query Text: {}\n\nResponse text: {}\n<-- end response --\n Will try another url", __func__, overpass_url, filledQuery, response_text));
            Log::logMsg(fmt::format("[{}]\tCurl error: \n\t{}\n", __func__, curl_easy_strerror(st_curl_result.res_curl)));
            std::this_thread::sleep_for(std::chrono::seconds(3)); // wait few seconds before sending a new request
          }

          // #ifndef RELEASE
          auto nd_ref_end_time   = std::chrono::steady_clock::now();
          auto diff_nd_start_end = nd_ref_end_time - nd_ref_start_time;
          auto nd_duration       = std::chrono::duration<double, std::milli>(diff_nd_start_end).count();
          Log::logMsg(fmt::format("[{}]\t\t >> Duration: {:.3f}ms ({:.2f}sec) - ref: {} <<\n\n", __func__, nd_duration, (nd_duration / 1000.0), Utils::readAttrib(nd_node, mxconst::get_ATTRIB_REF_OSM(), "n/a ref")));
          // #endif

          // check [abort]
          if (inoutThreadState->flagAbortThread)
          {
            q->flag_data_respond_was_valid = false;
            q->total_way_count             = 0;
            q->reset_osm_target_data_only();
            // q->xml_target_nd_node          = IXMLNode::emptyIXMLNode;
            // q->xml_target_way_element      = IXMLNode::emptyIXMLNode;
            return;
          }

        } // end fetch specific node from Overpass


        if (st_curl_result.res_curl == CURLE_OK)
        {
          IXMLDomParser local_xmlParser;
          auto          xml_ref_nd_node = local_xmlParser.parseString(response_text.c_str()).deepCopy();
          #ifndef RELEASE
          Log::logMsg(fmt::format("--> [{}] <nd>: Target Node:\n{}", __func__, Utils::xml_get_node_content_as_text(xml_ref_nd_node))); // debug
          #endif
          q->xml_target_nd_node = xml_ref_nd_node.getChildNode("node").deepCopy();
          data_manager::find_vector_between_two_osm_nodes(q, rand_node_num);

          q->end_time = std::chrono::steady_clock::now();
          return; // force loop exit
          // --- END Searching Vector between nodes -------

        } // end searching vector

        Log::logMsg(fmt::format("\tReading Node Curl error: \n\t{}\n", curl_easy_strerror(st_curl_result.res_curl)));
      } // end loop "shuffle way nodes"
    } // end nodeCound > 0, meaning we have valid way nodes in the response

  } // end respond is not empty
  // end if we found the BBOX string

  if (!q->xml_target_nd_node.isEmpty())
    Log::logMsg( fmt::format("[{}] FOUND target node: {}\n", __func__, Utils::xml_get_node_content_as_text(q->xml_target_nd_node) ) );
  else
    Log::logMsg( fmt::format("[{}] DID NOT FIND target node. Query: {}\n", __func__, filledQuery ) );

  q->end_time = std::chrono::steady_clock::now ();

}

// -------------------------------------

mx_return missionx::data_manager::find_vector_between_two_osm_nodes(missionx::structs::strct_osm_query* q, int& in_nd_node_ref_index)
{
  missionx::mx_return result = false;
  if (q && q->xml_target_way_element.isEmpty())
  {
    result.addErrMsg(fmt::format("[{}] Way node is empty.", __func__), true);
    return result;
  }

  if (q->xml_target_nd_node.isEmpty())
  {
    result.addErrMsg(fmt::format("[{}] Target node is empty.", __func__), true);
    return result;
  }


  // -----------------------------------------------------
  // FIND VECTOR to next node in <way>. // call the next or previous node relative to the picked node
  // -----------------------------------------------------
  // using the in_nd_node_ref_index, we will check if there is a node index smaller than it or larger than it.
  auto const lmbda_get_next_node = [](const int& in_nd_node_ref_index, const IXMLNode& in_way)
  {
    const auto nd_count = in_way.nChildNode(mxconst::get_ELEMENT_ND_OSM().c_str());

    // preferred result, next node
    if ((in_nd_node_ref_index + 1) <= (nd_count - 1))
      return in_nd_node_ref_index + 1;


    // Try previous node
    if ((in_nd_node_ref_index - 1) > -1)
      return in_nd_node_ref_index - 1;

    return -1;
  };

  // calculate the bearing/vector between the two nodes
  auto const lmbda_calc_vector = [](const IXMLNode& in_node_target1, const IXMLNode& in_node2) -> double
  {
    const auto lat1 = Utils::readNodeNumericAttrib<float>(in_node_target1, mxconst::get_ATTRIB_LAT_OSM(), 0.0f);
    const auto lon1 = Utils::readNodeNumericAttrib<float>(in_node_target1, mxconst::get_ATTRIB_LONG_OSM(), 0.0f);
    const auto lat2 = Utils::readNodeNumericAttrib<float>(in_node2, mxconst::get_ATTRIB_LAT_OSM(), 0.0f);
    const auto lon2 = Utils::readNodeNumericAttrib<float>(in_node2, mxconst::get_ATTRIB_LONG_OSM(), 0.0f);
    if ((lat1 * lon1 * lat2 * lon2) == 0.0f)
      return 0.0;

    return mxUtils::mxCalcBearingBetween2Points(lat1, lon1, lat2, lon2);
  };

  Log::logMsg(fmt::format("[{}] >>> Try Fetching the Vector Between Nodes <<<", __func__));
  size_t trial_counter = 0L;
  missionx::structs::strct_curl_result st_curl_result_for_step3;

  do
  {
    trial_counter++;

    const auto node_to_pick_n = lmbda_get_next_node(in_nd_node_ref_index, q->xml_target_way_element);
    if (node_to_pick_n > -1)
    {
      // get subnode <ref>
      auto nd_node_on_vector = q->xml_target_way_element.getChildNode(mxconst::get_ELEMENT_ND_OSM().c_str(), node_to_pick_n);
      if (!nd_node_on_vector.isEmpty() && !data_manager::vecOverpassUrls.empty())
      {
        // Check if we have a valid "lat/lon" attributes in <nd> node, instead of quering the "ref" value for <node>.
        missionx::NavAidInfo na;
        na.lat = Utils::readNodeNumericAttrib<float>(nd_node_on_vector, mxconst::get_ATTRIB_LAT_OSM(), 0.0f);
        na.lon = Utils::readNodeNumericAttrib<float>(nd_node_on_vector, mxconst::get_ATTRIB_LONG_OSM(), 0.0f);
        if (na.is_lat_lon_valid())
        {
          q->xml_next_node_to_find_vector = nd_node_on_vector.deepCopy();
          q->target_node_estimate_vector = lmbda_calc_vector(q->xml_target_nd_node, q->xml_next_node_to_find_vector);

          result = true;
          return result;
        }

        // Fallback if <nd> does not have valid "lat/lon"
        //
        // prepare query
        auto node_ref_id = Utils::readAttrib(nd_node_on_vector, mxconst::get_ATTRIB_REF_OSM(), "", "-1", true);

        // prepare the osm filter
        std::string next_filledQuery         = fmt::format("[out:xml][timeout:20][bbox:{}];node({});out body;", q->q_all_bbox, node_ref_id);
// #ifdef MX_ENABLE_HTTP_REQUESTS
        std::string s_curl_vector_node_query = fmt::format("data={}", curl_easy_escape(nullptr, next_filledQuery.c_str(), 0));
// #else
//         std::string s_curl_vector_node_query = fmt::format("data={}", next_filledQuery);
// #endif // !DISABLE_CURL

        // -----------------------
        // call cURL
        // -----------------------
        // std::string result_text;
        // const auto& overpass_url_to_fetch_ref = (trial_counter < data_manager::vecOverpassUrls.size()) ? data_manager::vecOverpassUrls.at(trial_counter) : mxconst::get_DEFAULT_OVERPASS_URL();
        const auto& overpass_url_s = data_manager::gen_get_next_overpass_url(data_manager::last_successfull_url_s);

        std::this_thread::sleep_for(std::chrono::seconds(2)); // wait for a few seconds
        // st_curl_result_for_step3 = data_manager::get_curl_request_respond(overpass_url_to_fetch_ref, s_curl_vector_node_query, 2);
        // v26.08.1
        missionx::structs::curl_request_data curl_conn_data = {.url_s = overpass_url_s, .post_request_field_s = s_curl_vector_node_query, .headers= {mxconst::OVERPASS_POST_HEADER} };
        st_curl_result_for_step3 = data_manager::get_curl_request_respond(curl_conn_data);

        const std::string result_text = st_curl_result_for_step3.response_text;

        #ifndef RELEASE
        if (!st_curl_result_for_step3.request_err.empty())
          Log::logMsg(fmt::format("[{}] Node fetch try no.:{}. Error:\n{}", __func__, trial_counter, st_curl_result_for_step3.request_err)); // debug
        #endif

        if (st_curl_result_for_step3.request_err.empty())
        {
          IXMLDomParser iDom;
          auto xml_next_nd_node = iDom.parseString(result_text.c_str()).getChildNode(mxconst::get_ELEMENT_NODE_OSM().c_str());

          if (!xml_next_nd_node.isEmpty())
          {
            q->xml_next_node_to_find_vector = xml_next_nd_node.deepCopy(); // store just in case we will need it
            q->target_node_estimate_vector  = lmbda_calc_vector(q->xml_target_nd_node, q->xml_next_node_to_find_vector);
            result.reset();

            return (result = true);
          }

        }
      }
    } // end "lmbda_get_next_node()" returned a valid nd index
  }
  while (!st_curl_result_for_step3.request_err.empty() && trial_counter < 3);
  // End calling cURL to find node info


  return result;
}

// -------------------------------------

missionx::mx_return missionx::data_manager::gen_request_from_the_llm_server(missionx::base_thread::strct_thread_state* inoutThreadState, missionx::structs::curl_request_data &in_curl_request_data, const std::string& user_prompt, const std::string& system_prompt)
{
  missionx::mx_return result_llm = false;


  Log::logMsg("url: " + in_curl_request_data.url_s); // debug

  // Prepare cURL
  bool        flag_http_success = false;
  int         iCurlTry          = 0;
  long        httpStatus        = 0;
  std::string cert_loc_s        = missionx::data_manager::mx_folders_properties.getStringAttributeValue(mxconst::get_PROP_MISSIONX_PATH(), "");
  std::string response_data;

  #ifndef RELEASE
    Log::logMsg(fmt::format("[{}] mission outline: {}", __func__, user_prompt));
  #endif

  // Build the payload structure natively from "the "system_prompt" and "user_prompt"
  nlohmann::json payload = {
                          {"model", "local-model"},
                          {"messages", nlohmann::json::array(
                              {
                                {
                                  {"role", "system"}
                                  , {"content", system_prompt}
                                   }
                                , {
                                    {"role", "user"}
                                  , {"content", user_prompt}
                                }
                              }
                              )
                          }
                          , {"temperature", 0.9}
  };


  // the ".dump" will take care of special json characters.
  std::string        json_payload = payload.dump();
  in_curl_request_data.post_request_field_s = json_payload;
  // in_curl_request_data.headers = "Content-Type: application/json";
  in_curl_request_data.connection_timeout = 20L;
  // in_curl_request_data.timeout = 60L; // deprecated, we get the value from the setup screen.
  in_curl_request_data.headers = {mxconst::OVERPASS_POST_HEADER};

  #ifndef RELEASE
  Log::logMsg(fmt::format("[{}] {}", __func__, json_payload));
  #endif

  // Call LLM server using cURL
  auto curl_request_result = data_manager::get_curl_request_respond(in_curl_request_data);

  if (curl_request_result.request_err.empty())
  {
    result_llm.reset();
    result_llm = true;
    result_llm.string_value = curl_request_result.response_text;
  }

  return result_llm;
}


// -------------------------------------

missionx::mx_return 
missionx::data_manager::gen_request_mission_leg_from_llm (missionx::base_thread::strct_thread_state* inoutThreadState, missionx::structs::curl_request_data& in_curl_request_data, const std::string& mission_outline) 
{ 
  // The following function should retrieve an ICAO from the LLM server.

  missionx::mx_return result_llm = false;

  Log::logMsg("url: " + in_curl_request_data.url_s); // debug

  // Prepare cURL
  bool        flag_http_success = false;
  int         iCurlTry          = 0;
  long        httpStatus        = 0;
  std::string cert_loc_s        = missionx::data_manager::mx_folders_properties.getStringAttributeValue(mxconst::get_PROP_MISSIONX_PATH(), "");
  std::string response_data;

#ifndef RELEASE
  Log::logMsg(fmt::format("[{}] mission outline: {}", __func__, mission_outline));
#endif

  const std::string system_prompt = "You will provide the airport information based on the format requested by the user."
                                    //"You must not provide prefix answers like: Okay, here’s a flight simulator mission brief based on your request"
                                    "If you do not find information, you must return an empty string as a result and nothing more.";

  // Build the payload structure natively from "the "system_prompt" and "mission_outline"
  nlohmann::json payload = {
    {"model", "local-model"}, 
    {"messages", nlohmann::json::array(
          {
            {
              {"role", "system"}, 
              {"content", system_prompt}
            }, 
          {
            {"role", "user"}, 
            {"content", mission_outline}
          }
      })
  }, 
    {"temperature", 0.6}};


  // the ".dump" will take care of special json characters.
  std::string json_payload                  = payload.dump();
  in_curl_request_data.post_request_field_s = json_payload;
  // in_curl_request_data.headers = "Content-Type: application/json";
  in_curl_request_data.connection_timeout = 20L;
  in_curl_request_data.how_many_tries     = 1; // we force only one try
  in_curl_request_data.headers = {mxconst::OVERPASS_POST_HEADER};

#ifndef RELEASE
  Log::logMsg(fmt::format("[{}] {}", __func__, json_payload));
#endif

  // Call LLM server using cURL
  auto curl_request_result = data_manager::get_curl_request_respond(in_curl_request_data);

  if (curl_request_result.request_err.empty())
  {
    result_llm.reset();
    result_llm              = true;
    result_llm.string_value = curl_request_result.response_text;
  }

  return result_llm;
}


// -------------------------------------

missionx::structs::curl_request_data 
missionx::data_manager::get_llm_user_setup_info_to_use_with_curl() 
{ 
  const auto llm_connection_timeout = missionx::system_actions::pluginSetupOptions.getNodeText_type_1_5<long>(mxconst::get_OPT_AI_LLM_SERVER_TIMEOUT(), 60L);
  const auto server_url             = missionx::system_actions::pluginSetupOptions.getNodeText_type_6(mxconst::get_OPT_AI_SERVER_URL(), "");
  auto       bearer_auth_key        = system_actions::pluginSetupOptions.getNodeText_type_6(mxconst::get_OPT_AI_AUTH_KEY(), mxconst::get_OPT_AI_DEFAULT_AUTH_KEY());

  if (mxUtils::trim(bearer_auth_key).empty())
    bearer_auth_key = mxconst::get_OPT_AI_DEFAULT_AUTH_KEY();

  std::vector<std::string> curl_headers = {"Content-Type: application/json", fmt::format("Authorization: Bearer {}", bearer_auth_key)};

  missionx::structs::curl_request_data curl_conn_data = {.timeout = llm_connection_timeout, .url_s = server_url, .headers = curl_headers};

  return curl_conn_data;
}

// -------------------------------------

std::unordered_map<std::string, std::vector<std::string>> 
missionx::data_manager::parse_llm_leg_data(const std::string& in_text) 
{ 
  std::unordered_map<std::string, std::vector<std::string>> result;
  std::istringstream                                        stream(in_text);
  std::string                                               line;

  while (std::getline(stream, line))
  {
    // Skip empty lines or carriage returns
    if (line.empty() || line == "\r")
    {
      continue;
    }

    // Locate the key-value separator ':'
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos)
    {
      continue;
    }

    std::string key  = line.substr(0, colon_pos);
    std::string rest = line.substr(colon_pos + 1);

    // Locate the pipe '|' separating the count from values
    size_t pipe_pos = rest.find('|');
    if (pipe_pos == std::string::npos)
    {
      continue;
    }

    int count = 0;
    try
    {
      count = std::stoi(rest.substr(0, pipe_pos));
    }
    catch (...)
    {
      continue; // Handle potential parsing errors for invalid count numbers
    }

    // Parse remaining pipe-separated value tokens
    std::vector<std::string> values;
    // values.reserve(count);

    std::istringstream val_stream(rest.substr(pipe_pos + 1));
    std::string        val;
    while (std::getline(val_stream, val, '|'))
    {
      // Strip trailing \r if processing Windows newline formats (\r\n)
      if (!val.empty() && val.back() == '\r')
      {
        val.pop_back();
      }
      values.push_back(val);
    }

    result[key] = std::move(values);
  }

  return result;
}


// -------------------------------------
// -------------------------------------

bool
data_manager::waitForPluginCallbackJob (missionx::base_thread::strct_thread_state *out_state_ptr, missionx::mx_flc_pre_command inQueuedCommand, std::chrono::milliseconds inWaitTimeMilliseconds, int inLimitWaitCounter)
{
  if (out_state_ptr)
  {
    int waitCounter = 0;

    out_state_ptr->thread_wait_state = missionx::mx_random_thread_wait_state_enum::waiting_for_plugin_callback_job; // set busy state
    missionx::data_manager::queFlcActions.emplace_back (inQueuedCommand); // provide the queued command

    std::this_thread::sleep_for (std::chrono::milliseconds (inWaitTimeMilliseconds));

    while (out_state_ptr->thread_wait_state < missionx::mx_random_thread_wait_state_enum::finished_plugin_callback_job && (waitCounter < inLimitWaitCounter) && !out_state_ptr->flagAbortThread && out_state_ptr->flagIsActive)
    {
      ++waitCounter;
      std::this_thread::sleep_for (std::chrono::milliseconds (inWaitTimeMilliseconds));
    }

    out_state_ptr->thread_wait_state = missionx::mx_random_thread_wait_state_enum::not_waiting; // reset state

    if (waitCounter >= inLimitWaitCounter || out_state_ptr->flagAbortThread)
      return false;
  }
  return true;
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
      const std::string& name    = vecDrefAndVal.at(0);
      std::string value_s = vecDrefAndVal.at(1);
      if (!value_s.empty())
      {
        const std::string key = mxUtils::trim(Utils::replaceChar1WithChar2_v2(name, '.', "/")); // v3.305.1 added trim ti the  "key" parameter dew to the fact that it can have: "\n" in the key name.
        apply_dataref_based_on_key_value_strings(key, value_s);
      } // end if the attribute value is not empty
    }
  }

  // end apply_datarefs_based_on_string_parsing
}

// -------------------------------------

void
data_manager::apply_datarefs_from_text_based_on_parent_node_and_tag_name(const IXMLNode& inParentNode, const std::string& inTagName)
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
  const auto vecActions = Utils::split_v2(inTaskList, ","); // v25.06.1 added delimiter since the default one did not use the correct delimiter.

  for (const auto& task_name : vecActions)
  {
      const auto task_ptr = (mxUtils::isElementExists(mapObjectives[inObjeName].mapTasks, task_name)) ?
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
  const auto vecTriggerNames = Utils::split_v2(inTrigList, ",");

  for (const auto& trig_name : vecTriggerNames)
  {
    // check if trigger exists and if we do not use the calling trigger name
    if (mxUtils::isElementExists( mapTriggers, trig_name ) && (inCallingTrig.name != trig_name) )
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

void data_manager::SetFuelEquallyAcrossActiveTanks(const float totalFuelKg)
{
  constexpr static int NUM_OF_TANKS = 9;
  // Locate the required X-Plane datarefs
  XPLMDataRef numTanksRef = XPLMFindDataRef("sim/aircraft/overflow/acf_num_tanks");
  XPLMDataRef tankRatRef = XPLMFindDataRef("sim/aircraft/overflow/acf_tank_rat");
  XPLMDataRef fuelQtyRef = XPLMFindDataRef("sim/flightmodel/weight/m_fuel");

  if (!numTanksRef || !fuelQtyRef) {
    return; // Required datarefs are missing
  }

  // Retrieve the total number of configured tanks[cite: 1]
  int maxTanks = XPLMGetDatai (numTanksRef);
  if (maxTanks <= 0) {
    return;
  }

  // Retrieve tank capacity ratios to determine active tanks
  std::vector<float> tankRatios(NUM_OF_TANKS, 0.0f);
  if (tankRatRef) {
    XPLMGetDatavf(tankRatRef, tankRatios.data(), 0, NUM_OF_TANKS);
  }

  // Check if we have an active tank (ratio > 0.0)
  int activeTankCount = 0;
  for (int i = 0; i < maxTanks && i < NUM_OF_TANKS && activeTankCount == 0; ++i) {
    if (!tankRatRef || tankRatios[i] > 0.0f) {
      activeTankCount++;
    }
  }

  if (activeTankCount == 0) {
    return;
  }

  // Distribute the fuel proportionally according to each tank's relative capacity ratio
  float fuelQuantities[NUM_OF_TANKS] = {0.0f};
  for (int i = 0; i < maxTanks && i < NUM_OF_TANKS; ++i) {
    if (tankRatios[i] > 0.0f) {
      fuelQuantities[i] = totalFuelKg * tankRatios[i];
    }
  }

  // Apply the updated fuel quantities to X-Plane
  XPLMSetDatavf(fuelQtyRef, fuelQuantities, 0, maxTanks);
}

// -------------------------------------

std::string
data_manager::get_weather_state(const int& inCode)
{
  constexpr const static auto MAX_LENGTH_BEFORE_BREAKING_LINE = static_cast<size_t> (500);

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

        if (inLegName == currentLegName) // add to current Leg only if this is what was asked
        {
          strct_flight_leg_info_totalMapsCounter += 1;
          Utils::addElementToMap(maps2d_to_display, strct_flight_leg_info_totalMapsCounter, mapCurrentMissionTextures[inFileName]);
        }

        Log::logMsg(std::string(__func__) + "] leg: " + inLegName + ", Loaded Texture: " + inFileName); // debug

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
  if (BitmapReader::load_textute_and_bind(texture, errorMsg, false)) // load image but do not flip it
  {
    Utils::addElementToMap(in_outTextureMapToStore, inFileName, texture); // store texture data in map

    return true; // success
  }
  else
    Log::logMsg(fmt::format("[{}] failed to load Texture: {}{}\nError: ", __func__, fldMissionCustom_withSep, inFileName, errorMsg)); // debug

  return false;
}


// -------------------------------------


Point
data_manager::write_plane_position_to_log_file()
{
  Point p = dataref_manager::getCurrentPlanePointLocation();

  XPLMProbeResult outResult;
  const double    groundElev_ft = Point::getTerrainElevInMeter_FromPoint(p, outResult) * meter2feet;

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

  // holds plane position
  double y,    z;
  double outX, outY, outZ;
  double x = y    = z    = 0.0;
  outX     = outY = outZ = 0.0;

  x = XPLMGetDataf(drefConst.dref_camera_view_x_f);
  y = XPLMGetDataf(drefConst.dref_camera_view_y_f);
  z = XPLMGetDataf(drefConst.dref_camera_view_z_f);

  XPLMLocalToWorld(x, y, z, &outX, &outY, &outZ);

  outZ *= meter2feet; // v24.03.2 fix elevation to be feet and not meter
  Point           p = Point(outX, outY, outZ);
  XPLMProbeResult outResult;
  double          groundElev_ft = Point::getTerrainElevInMeter_FromPoint(p, outResult) * meter2feet; //   getTerrainElevFromPoint(gLat, gLon, gElev, &outResult);

  // read longitude/latitude info
  double gHeading = XPLMGetDataf(gHeadingPsiRef);
  p.setHeading(gHeading);


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
      default:
        break;
    } // end switch
  }   // end if dref is valid

} // apply_datarefs_based_on_string

// -------------------------------------
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
data_manager::position_camera(const XPLMCameraPosition_t& inCameraPos, const float inLat, const float inLon)
{
  if ((inLat * inLon) == 0.0f)
  {
    Log::logMsg("[position_camera] Wrong camera position values.");
  }
  else
  {

    mxCameraPosition  = inCameraPos;
    isLosingControl_i = 1;                                             // plugin control
    queFlcActions.push_back(mx_flc_pre_command::position_camera); // Position camera
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


// -------------------------------------

mx_return data_manager::get_is_wet_at_point_thread_unsafe(const missionx::NavAidInfo& inNavAid, missionx::base_thread::strct_thread_state& inout_thread_state, structs::strct_shared_random_airport_info& inout_shared_navaid_info)
{
  mx_return result (false); // we initialize as "no wet"
  inout_shared_navaid_info.p = inNavAid.p;
  if (!missionx::data_manager::waitForPluginCallbackJob(&inout_thread_state, missionx::mx_flc_pre_command::get_is_point_wet))
  {
    inout_shared_navaid_info.isWet = false;
    result.addErrMsg(fmt::format("[{}] Failed to probe for wet. Will treat target coordinates as \"land\". ", __func__) );
  }

  return inout_shared_navaid_info.isWet;
}








// -----------------------------------
//
// -----------------------------------
// TimeLapse CLASS
// -----------------------------------



std::string
TimeLapse::set_date_and_hours(const std::string& inDateAndTime, const bool inIgnorePauseMode)
{
  this->flag_ignorePauseMode = inIgnorePauseMode; // v3.305.1

  // 0 = day in year, 1 = hours, 2 = min
  std::map<int, std::string> mapTime = Utils::splitStringToMap(inDateAndTime, mxconst::get_COLON());

  std::string val;

  this->start_local_date_days_i = XPLMGetDatai(dc.dref_local_date_days_i);
  this->local_time_sec_f        = XPLMGetDataf(dc.dref_local_time_sec_f);
  this->zulu_time_sec_f         = XPLMGetDataf(dc.dref_zulu_time_sec_f);

  // store day in the year
  if (Utils::isElementExists(mapTime, 0) && !mapTime[0].empty() && Utils::is_number(mapTime[0]))
  {
    int dayOfYear = -1;
    dayOfYear     = Utils::stringToNumber<int>(mapTime[0]);
    if (dayOfYear >= 0 && dayOfYear <= 364)
      futureDayOfYearAfterTimeLapse = dayOfYear;
    else
      futureDayOfYearAfterTimeLapse = -1;
  }

  // set timelaps with new time and cycle 1
  if (Utils::isElementExists(mapTime, 1) && !mapTime[1].empty() && Utils::is_number(mapTime[1]))
  {
    int minutes = -1;
    const int hours   = Utils::stringToNumber<int>(mapTime[1]);
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
  this->expected_time_after_addition_in_seconds = local_time_sec_f + total_time_to_add_in_seconds; // this can be greater than 86,400, so we need to be careful

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

  auto future_time_in_seconds = static_cast<float> ( ( inFutureLocalHour * 3600 ) + ( inFutureMinutes * 60 ) ); // convert hours and minutes to seconds from midnight. This represents the real hour in x-plane but in seconds and for local time. We need to find the delta for UTC.
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
      Log::logMsgErr("[TimeLapse] Time laps has exceed its maximum cycles. Resetting timelap. Notify developer.");
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

GLobalSettings::~GLobalSettings() = default;

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

    // this line is to test against other compilers too.
    mapPlaneEnumToStringTypes.clear();
    for (auto& [key, value] : mapPlaneStringTypesToEnum)
    {
      mapPlaneEnumToStringTypes[value] = key; // for translation
    }


}

// -------------------------------------
// -------------------------------------
// -------------------------------------
