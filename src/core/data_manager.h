#ifndef DATA_MANAGER_H_
#define DATA_MANAGER_H_

#pragma once


//#ifdef MAC
//  #if __has_include(<format>) && (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 135000)
//  #define GHC_USE_STD_FS
//    #include <format>
//  #endif
//  #ifndef GHC_USE_STD_FS
//    #include <fmt/core.h>
//
//  #endif
//#else 
//  // Linux and Windows
//  #include <format>
//#endif


#include "XPLMMap.h" // v3.305.4
#include "XPLMCamera.h" // v3.0.303.7

#include "mxconst.h"

#ifdef USE_HTTPLIB
#include <httplib.h> // cpp-httplib https://github.com/yhirose/cpp-httplib
#endif


#include <queue>
#include <set>
#include <unordered_map>
#include <format> // v25.04.2 added standard format library in the hope to replace "fmt/core.h" which is a 3rd library.
#include <fmt/core.h>


#include "curl/curl.h"
#include "thread/gather_stats.h" // v3.303.14
#include "../data/Briefer.hpp"
#include "../data/Objective.h"
#include "../data/Waypoint.h"
#include "../data/Trigger.h"
#include "../inv/Inventory.h" // v3.0.213.1
#include "../logic/dataref_param.h"
#include "Message.h"
#include "../ui/core/TextureFile.h" // v3.305.1 replaced mxButtonTexture
#include "../io/BrieferInfo.h"
#include "obj3d/obj3d.h" // v3.0.200
#include "../io/TemplateFileInfo.hpp" // v3.0.217.2
//#include "../ui/core/MxShader.hpp" // v3.305.1 - not in use but maybe in the future
#include "mx_base_node.h" // v3.303.11
#include "../data/Choice.h"       // v3.0.231.1
#include "coordinate/NavAidInfo.hpp"
#include "../io/dbase.h" // v3.0.241.10
#include "../core/embeded_script/script_manager.h" // v3.303.14 moved here from Task class, and being used by ext_script class.
#include "../../libs/imgui4xp/imgui/imgui.h" // v3.303.14 instead of "ImgWindow.h" to resolve the ImVec2 in the header
#include "../../libs/imgui4xp/colors/mx_colors.hpp" // v3.305.3 moved colors to its own file so we could integrate in any class without complex dependency


namespace missionx
{

typedef struct _mx_nav_data_strct
{
  typedef struct _mx_row3 // localizer
  {
    // Query 
    double      dDistance;
    std::string field1; // can be any string field
    std::string field2; // can be any string field
    int iField1; // v24.06.1
  } mx_row3_4_5_strct;

  typedef struct _mx_row2 // runway
  {
    // Query 2 row
    double      dDistance;
    std::string rw_key;
    std::string desc;

  } mx_row2_strct;

  // Query 1 data
  int         seq{ 0 };
  int         icao_id{ -1 };
  double      dDistance = { -1.0 };
  double      ap_lat    = { 0.0 };
  double      ap_lon    = { 0.0 };

  std::string icao{ "" };
  std::string sApDesc{ "" };

  std::string sMetar{ "" }; // fetched from flightplandatabase.com/weather/{icao}

  // Query 2 rw data
  std::unordered_map<std::string, mx_row2_strct> mapRunwayData;

  // Query 3+4 Vors and loc
  std::list<mx_row3_4_5_strct> listVor;
  std::list<mx_row3_4_5_strct> listLoc;
  std::list<mx_row3_4_5_strct> listFrq; // v24.03.1

} mx_nav_data_strct;

/**
data_manager holds ALL STATIC information that we want to share with the plugin, example: tasks, objective, folder information, global settings, seed ext parameters etc.
It sored the information in static maps, and has key functions to add tasks/objectives and goals to maps.
*/

// set of commands that need to be execute from "plugin" or "flc" phase since we don't want to interrupt flow of code
typedef enum class _flc_commands
  : uint8_t
{
  abort_mission,  // 0
  abort_random_engine,                         // v3.0.253.6
  calculate_slope_for_build_flight_leg_thread, // v3.0.221.3
  convert_icao_to_xml_point,                   // v3.0.221.5
  create_savepoint,                            // v3.0.151
  create_savepoint_and_quit,                   // v3.0.251.1 b2
  disable_aptdat_optimize_menu,                // v3.0.219.12
  disable_generator_menu,                      // v3.0.219.12
  display_choice_window,                       // v3.0.231.1
  dont_control_camera,                         // v3.0.303.7 XPLMDontControlCamera() - reset camera
  enable_aptdat_optimize_menu, // 10           // v3.0.219.12
  enable_generator_menu,                       // v3.0.219.12
  eval_end_flight_leg_after_all_qmm_broadcasted,    // v3.305.1 same as "end_mission_after_all_qmm_broadcasted" only at the flight leg level, and it might replace it too.
  end_mission_after_all_qmm_broadcasted,       // v3.0.241.7.1
  exec_apt_dat_optimization,                   // v3.0.253.6
  execute_xp_command,                          // v3.0.221.9 allow plugin to call "XPLMCommandOnce()" to execute a mapped command
  fetch_simbrief_fpln,                         // v25.03.3
  force_pause_xplane,
  fetch_metar_data_after_nav_info,             // v24.03.1
  gather_acf_cargo_data,                                 // v24.12.2
  gather_acf_custom_datarefs,                            // v3.303.9.1
  gather_random_airport_mainThread, // 20                       // v3.0.221.4
  generate_mission_from_littlenavmap_fpln,                      // v3.0.301 converts imported LNM flight plan to a missionx mission file.
  get_current_weather_state_and_store_in_RandomEngine,          // v3.303.13
  get_is_point_wet,                                             // v3.0.221.4
  get_and_guess_nav_aid_info_mainThread,                        // v25.04.2 used in Random Engine, when we try to add the "simbrief/fpln" waypoint route.
  get_nav_aid_info_mainThread,                                  // v3.0.253.6
  get_nearest_nav_aid_to_custom_lat_lon_mainThread,             // v3.0.241.10 b2
  get_nearest_nav_aid_to_randomLastFlightLeg_mainThread,        // v3.0.221.4
  guess_waypoints_from_external_fpln_site,                      // v3.0.255.2
  handle_option_picked_from_choice,                             // v3.0.231.1
  hide_choice_window,                                           // v3.0.231.1
  hide_briefer_window_in_2D, // 30                              // v3.305.1
  hide_target_marker_option,                                    // v3.0.255.4.1
  imgui_check_presence_of_db_ils_data,                        // v3.0.253.6 Used with option_ils_layer. We will check DB state and xp_rw and xp_loc rows presence.
  imgui_check_validity_of_db_file,                       // v3.0.253.9 Will help force the simmer to execute "run APT.dat optimization"
  imgui_generate_random_mission_file,                    // v3.0.251.1
  imgui_prepare_mission_files_briefer_info,              // v3.0.251.1 same as MENU_OPEN_LIST_OF_MISSIONS
  imgui_reload_templates_data_and_images,                // v3.0.251.1 only reload templates and image
  inject_metar_file,                                     // v3.0.223.1
  load_mission,
  load_notes_info,                                        // v24.03.1
  load_savepoint, // 40                                  // v3.0.151
  load_briefer_textures,                                // v25.04.2
  open_inventory_layout,                                 // v3.0.213.2 mainly from MXPAD, when simmer clicks on the inventory hint
  open_map_layout,                                       // v3.0.231.1 mainly from MXPAD, when simmer clicks on the map hint
  open_story_layout,                                     // v3.305.1 used when we have active story message
  pause_xplane,
  position_camera,                     // v3.0.303.7 position camera view, based on https://developer.x-plane.com/code-sample/camera/
  position_plane,                      // when we start a mission we need to move plane to its location. This is true to new and loaded savepoint/checkpoint Should be created by plugin
  post_async_inv_image_binding,        // v3.0.303.5
  post_async_story_image_binding,      // v3.305.1
  post_mission_load_change_to_running, // 50 //  v3.0.223.5
  post_position_plane,                 // can be called after position plane
  post_story_message_cache_cleanup,    // v3.305.1
  read_async_inv_image_files,          // v3.0.303.5
  restart_all_plugins,                 // v3.0.253.1
  save_notes_info,                     // v24.03.1
  save_user_setup_options,             // v3.0.255.4.2 save user setup preference
  set_briefer_text_message,            // v3.0.219.12+ send message to user from a threaded code (for example)
  set_story_auto_pause_timer,          // v3.305.1 used with: strct_flight_leg_info.strct_story_mode.timerForAutoSkip
  set_time,                            // v3.0.219.7
  show_target_marker_option,           // v3.0.255.4.1
  special_test_place_instance, // 60   // v3.0.251.1 used to test special action from imgui button.
  start_mission,
  start_random_mission, // v3.0.219.1
  stop_mission,
  stop_position_camera,                 // v3.0.303.7 position camera view, based on https://developer.x-plane.com/code-sample/camera/
  sound_abort_all_channels,             // v3.305.1c abort and clean all channels in Sound class. Usefull when there are many channels and some might not be cleaned gracefully
  toggle_auto_hide_show_mxpad_option,
  toggle_cue_info_mode, // v3.305.4
  toggle_designer_mode, // v3.305.4
  toggle_target_marker_option, // v3.0.253.9.1
  unpause_xplane,  // 70
  update_point_in_file_with_template_based_on_probe, // v3.0.255.4.4 used in setup and tools
  write_fpln_to_external_folder,                     // v3.0.255.4.4 used in setup and tools screen so we won't call it from draw callback
} mx_flc_pre_command;

typedef enum class _mission_state
  : uint8_t
{
  mission_undefined                              = 0,
  mission_stopped                                = 2,
  mission_is_being_loaded_from_the_original_file = 10,
  mission_loaded_from_the_original_file          = 20,
  mission_was_generated                          = 30,
  mission_loaded_from_savepoint                  = 40,
  pre_mission_running                            = 50,
  mission_is_running                             = 60,
  mission_completed_success                      = 70,
  mission_completed_failure                      = 80,
  stop_all_async_processes                       = 90,
  process_points                                 = 100,
  mission_aborted                                = 110
} mx_mission_state_enum;





typedef struct _local_fpln_strct
{
  static const int MAX_ARRAY = 10;

  bool        flag_isLeg{ false };
  bool        flag_isLast{ false };
  bool        flag_ignore_leg{ false };
  bool        flag_convertToBriefer{ false };
  bool        flag_add_marker{ false }; // v3.0.303.2, will be shown only for checked legs

  int         marker_type_i{ 0 }; // v3.0.303.3 0 = default between 0..3
  int         indx{ -1 }; // holds the line number
  int         iCurrentBuf{ 0 };
  float       radius_to_display_3D_marker_in_nm_f{ 10.0f }; // v3.0.303.3 holds radius in nm

  double      distToPrevWaypoint{ 0.0 }, cumulativeDist{ 0.0 };

  std::string name;
  std::string ident;
  std::string type;
  std::string region;

  std::string attribName;

  missionx::Point p; // location
  IXMLNode  xFlightPlan;
  IXMLNode  xLeg;
  IXMLNode  xTriggers;
  IXMLNode  xMessageTmpl;
  IXMLNode  xObjectives;
  IXMLNode  xLoadedScripts;

  char      buff_arr[MAX_ARRAY][missionx::LOG_BUFF_SIZE]{ {'\0'} }; // 2048
  

  IXMLNode xConv; // v3.0.303.4 holds the conversion node info during save

          // reset buff
  void resetBuff(int indx)
  {
    assert(indx < MAX_ARRAY && "Tried to reset a cell not in array. ");

    memset(buff_arr[indx], '\0', sizeof(buff_arr[indx]));
  }

  void setBuff(int indx, std::string inVal_s)
  {
    assert(indx < MAX_ARRAY && "__func__ Plugin tried to write to undefined memory cell.");

    if (indx < MAX_ARRAY)
    {
      resetBuff(indx);
#ifdef IBM
      memcpy_s(buff_arr[indx], sizeof(buff_arr[indx]), inVal_s.c_str(), (inVal_s.length() > sizeof(buff_arr[indx]) ? sizeof(buff_arr[indx]) : inVal_s.length())); // we copy the memory based on which buffer do not exceeds the buffer.
#else
      memcpy(buff_arr[indx], inVal_s.c_str(), inVal_s.length());
#endif

    } // end if in bounderies
  }


  typedef struct target_trig_ // inner struct
  {
    bool flag_on_ground{ false };
    int elev_min{ 0 };
    int elev_max{ 0 };
    int elev_combo_picked_i{ -1 };

    int         slider_elev_value_i{ 0 }; // v3.0.303.4
    int         radius_of_trigger_mt{ 0 }; // v3.0.303.4

    std::string elev_rule_s{ "" };
    std::string elev_lower_upper{ "" };

    void init()
    {      
      flag_on_ground  = false;
      elev_min = 0;
      elev_max = 0;
      elev_combo_picked_i = -1;
      slider_elev_value_i = 0;
      radius_of_trigger_mt = 0;

      elev_rule_s.clear();
      elev_lower_upper.clear();
    }

  } target_trig_strct_;
  target_trig_strct_ target_trig_strct;


  void reset() { 
    indx        = -1;
    flag_isLast = false;
    flag_isLeg  = false;
    flag_ignore_leg = false;
    flag_convertToBriefer = false;
    flag_add_marker       = false; // v3.0.303.2

    distToPrevWaypoint = 0.0;
    cumulativeDist = 0.0 ;
    name.clear();
    type.clear();
    region.clear();
    attribName.clear();
    p.init();

    xFlightPlan = IXMLNode::emptyIXMLNode;
    xLeg = IXMLNode::emptyIXMLNode;
    xLoadedScripts = IXMLNode::emptyIXMLNode;
    resetBuffPopBrief();
  }

  void resetBuffPopBrief()
  {
    for (auto& b : buff_arr)    
      memset(b, '\0', sizeof(b));    
  }

  std::string getName()
  {
    return (name.empty()) ? ident : name + "(" + ident + ")";
  }

  int validate_and_fix_trigger_radius() {

    // Determine waypoint radius for target <trigger> task
    if (this->target_trig_strct.radius_of_trigger_mt <= 0)
    {
      if (this->flag_isLast)
        return missionx::conv::DEFAULT_RADIUS_LENGTH_FOR_GROUND_WP;
      else
        return missionx::conv::DEFAULT_RADIUS_LENGTH_FOR_AIRBORNE_WP;
    }

    return this->target_trig_strct.radius_of_trigger_mt; // no change
  }

  IXMLNode getFplnAsRawXML() 
  {
    //  "<DUMMY> </DUMMY>"

    IXMLNode xNode = this->xLeg.deepCopy();
    xNode.updateName(mxconst::get_ELEMENT_FPLN().c_str());

    // Index, Name
    Utils::xml_set_attribute_in_node<int>(xNode, mxconst::get_ATTRIB_INDEX(), this->indx, mxconst::get_ELEMENT_FPLN());
    Utils::xml_set_attribute_in_node_asString(xNode, mxconst::get_ELEMENT_LNM_Name(), this->name, mxconst::get_ELEMENT_FPLN());
    Utils::xml_set_attribute_in_node_asString(xNode, mxconst::get_ATTRIB_NAME(), this->attribName, mxconst::get_ELEMENT_FPLN());
    Utils::xml_set_attribute_in_node_asString(xNode, mxconst::get_ATTRIB_TYPE(), this->type, mxconst::get_ELEMENT_FPLN());
    Utils::xml_set_attribute_in_node_asString(xNode, mxconst::get_ELEMENT_LNM_Ident(), this->ident, mxconst::get_ELEMENT_FPLN());

    // Point
    Utils::xml_set_attribute_in_node<double>(xNode, mxconst::get_ATTRIB_LAT(), this->p.getLat(), mxconst::get_ELEMENT_FPLN());
    Utils::xml_set_attribute_in_node<double>(xNode, mxconst::get_ATTRIB_LONG(), this->p.getLon(), mxconst::get_ELEMENT_FPLN());

    // Boolean info
    Utils::xml_set_attribute_in_node<bool>(xNode, mxconst::get_CONV_ATTRIB_isLeg(), this->flag_isLeg, mxconst::get_ELEMENT_FPLN());
    Utils::xml_set_attribute_in_node<bool>(xNode, mxconst::get_CONV_ATTRIB_isLast(), this->flag_isLast, mxconst::get_ELEMENT_FPLN());
    Utils::xml_set_attribute_in_node<bool>(xNode, mxconst::get_CONV_ATTRIB_ignore_leg(), this->flag_ignore_leg, mxconst::get_ELEMENT_FPLN());
    Utils::xml_set_attribute_in_node<bool>(xNode, mxconst::get_CONV_ATTRIB_convertToBriefer(), this->flag_convertToBriefer, mxconst::get_ELEMENT_FPLN());
    Utils::xml_set_attribute_in_node<bool>(xNode, mxconst::get_CONV_ATTRIB_add_marker(), this->flag_add_marker, mxconst::get_ELEMENT_FPLN());
    
    // internal
    Utils::xml_set_attribute_in_node<int>(xNode, mxconst::get_CONV_ATTRIB_markerType(), this->marker_type_i, mxconst::get_ELEMENT_FPLN());
    Utils::xml_set_attribute_in_node<float>(xNode, mxconst::get_CONV_ATTRIB_radius_to_display_marker(), this->radius_to_display_3D_marker_in_nm_f, mxconst::get_ELEMENT_FPLN());

    // Point information as XML
    xNode.addChild(this->p.node.deepCopy());

    // Trigger - target - this is a specialized trigger, it only holds the mandatory flight leg target. There might be other mandatory target triggers but we only care about the picked flight leg position as target
    IXMLNode xTrigger = xNode.addChild(mxconst::get_ELEMENT_TRIGGER().c_str());


    // Determine waypoint radius for target <trigger> task
    this->target_trig_strct.radius_of_trigger_mt = this->validate_and_fix_trigger_radius();
    Utils::xml_set_attribute_in_node<int>(xTrigger, mxconst::get_ATTRIB_LENGTH_MT(), this->target_trig_strct.radius_of_trigger_mt, mxconst::get_ELEMENT_TRIGGER()); // v3.0.304.4

    Utils::xml_set_attribute_in_node<bool>(xTrigger, mxconst::get_CONV_ATTRIB_on_ground(), this->target_trig_strct.flag_on_ground, mxconst::get_ELEMENT_TRIGGER());
    Utils::xml_set_attribute_in_node<int>(xTrigger, mxconst::get_ATTRIB_ELEV_MIN_FT(), this->target_trig_strct.elev_min, mxconst::get_ELEMENT_TRIGGER());
    Utils::xml_set_attribute_in_node<int>(xTrigger, mxconst::get_ATTRIB_ELEV_MAX_FT(), this->target_trig_strct.elev_max, mxconst::get_ELEMENT_TRIGGER());
    Utils::xml_set_attribute_in_node<int>(xTrigger, mxconst::get_CONV_ATTRIB_elev_combo_picked_i(), this->target_trig_strct.elev_combo_picked_i, mxconst::get_ELEMENT_TRIGGER());
    Utils::xml_set_attribute_in_node<int>(xTrigger, mxconst::get_CONV_ATTRIB_slider_elev_value_i(), this->target_trig_strct.slider_elev_value_i, mxconst::get_ELEMENT_TRIGGER());

    Utils::xml_set_attribute_in_node_asString(xTrigger, mxconst::get_CONV_ATTRIB_elev_rule_s(), this->target_trig_strct.elev_rule_s, mxconst::get_ELEMENT_TRIGGER());
    Utils::xml_set_attribute_in_node_asString(xTrigger, mxconst::get_ATTRIB_ELEV_MIN_MAX_FT(), this->target_trig_strct.elev_lower_upper, mxconst::get_ELEMENT_TRIGGER());

    // Add xTriggers
    xNode.addChild(this->xTriggers.deepCopy());

    // Add Messages
    xNode.addChild(this->xMessageTmpl.deepCopy());

    // v3.0.303.7 Add Scriptlets
    for (int i1 = 0; i1 < this->xFlightPlan.nChildNode(mxconst::get_ELEMENT_SCRIPTLET().c_str()); ++i1)
    {
      xNode.addChild(this->xFlightPlan.getChildNode(mxconst::get_ELEMENT_SCRIPTLET().c_str(), i1).deepCopy());
    }

    IXMLNode xBuffers = xNode.addChild(mxconst::get_ELEMENT_BUFFERS().c_str());
    for (auto s : buff_arr)
    {
      IXMLNode xBuf = xBuffers.addChild(mxconst::get_ELEMENT_BUFF().c_str());
      Utils::xml_add_cdata(xBuf, s);
    }

    return xNode.deepCopy();
  }
} mx_local_fpln_strct;




typedef struct _mx_sceneryOrPlaneLoad_state_strct
{
  // v3.305.1b
  // Valid combinations:
  // flagSceneryOrAirportLoaded = false => evaluate triggers.
  // flagSceneryOrAirportLoaded = true && timer < 2.0  => do not evaluate triggers
  // flagSceneryOrAirportLoaded = true && timer > 2.0  => reset state and evaluate
  

  bool            flagSceneryOrAirportLoaded{ false };
  int             iPostLoadedSceneryOrAirports_counter_before_re_evaluate_triggers{ 0 }; // the combination of these two parameters wil determined if we will evaluate flight leg or triggers
  constexpr const static float SECONDS_TO_RUN = 2.0f;
  missionx::Timer timer;

  void set_flagSceneryOrAirportLoaded(const bool in_bSceneryLoaded)
  { 
    flagSceneryOrAirportLoaded = in_bSceneryLoaded;
    iPostLoadedSceneryOrAirports_counter_before_re_evaluate_triggers = 0;
    timer.reset();
    if (in_bSceneryLoaded)
    {
      missionx::Timer::start(timer, SECONDS_TO_RUN, "TimerSkipLegAfterSceneryLoad");
    }

  }

  void reset() { this->set_flagSceneryOrAirportLoaded(false); }

  bool getCanPluginContinueEvaluation()
  { 
    if (!flagSceneryOrAirportLoaded)
      return true;

    if (flagSceneryOrAirportLoaded && !Timer::evalTime(this->timer) ) //&& iPostLoadedSceneryOrAirports_counter_before_re_evaluate_triggers < 10)
    {
      ++iPostLoadedSceneryOrAirports_counter_before_re_evaluate_triggers;
      #ifndef RELEASE
        Log::logMsg("iPostLoadedSceneryOrAirports_counter_before_re_evaluate_triggers: " + mxUtils::formatNumber<int>(iPostLoadedSceneryOrAirports_counter_before_re_evaluate_triggers));
      #endif // !RELEASE

    }
    else
      this->reset();

    return false;
  }


} mx_sceneryOrPlaneLoad_state_strct;



/////////////////////////////////////
//// TimeLapse CLASS
/////////////////////////////////////

class TimeLapse
{
private:
  int                  start_local_date_days_i; // store the day before starting timelapse
  float                local_time_sec_f;
  float                zulu_time_sec_f;
  static dataref_const dc;

  int cycleCounter;


public:
  virtual ~TimeLapse(){};

  bool  flag_isActive;
  bool  flag_ignorePauseMode; // v3.305.1 evaluate timelapse even if sim is in pause state
  float seconds_per_lapse_f;
  float total_time_to_add_in_seconds;            // total seconds that we need to cover. "total_time_to_add_in_seconds = desired_local_time - current_local_time"
  float expected_time_after_addition_in_seconds; // total_time_to_add_in_seconds + local_time_seconds. We should not touch this value during iteration since at the end we will decide if to add local day.
  float total_passed;                            // how much time passed so far. Increased by "seconds_per_lap" every second.
  int cycles; // cycles = overall_laps_time_in_sec / total_time_to_add_in_seconds. How many seconds the timer will have to run.

  Timer timer;

  int futureDayOfYearAfterTimeLapse; // after timelapse to hours change day of year. "-1" means do not change.

  TimeLapse()
  {
    this->flag_isActive                             = false;
    this->flag_ignorePauseMode                      = false; // v3.305.1
    this->expected_time_after_addition_in_seconds   = 0;
    this->seconds_per_lapse_f                       = 0.0f;
    this->total_time_to_add_in_seconds              = 0.0f;
    this->total_passed                              = 0.0f;
    // this->overall_laps_time_in_sec = 0;
    this->cycles = 0;

    this->timer.reset();

    this->start_local_date_days_i = 0;
    this->local_time_sec_f        = 0.0f;
    this->zulu_time_sec_f         = 0.0f;

    this->cycleCounter = 0;

    this->futureDayOfYearAfterTimeLapse = -1;
  };

  void reset()
  {
    this->flag_isActive                             = false;
    this->flag_ignorePauseMode                      = false; // v3.305.1
    this->expected_time_after_addition_in_seconds   = 0;
    this->seconds_per_lapse_f                       = 0.0f;
    this->total_time_to_add_in_seconds              = 0.0f;
    this->total_passed                              = 0.0f;
    // this->overall_laps_time_in_sec = 0;
    this->cycles = 0;
    this->timer.reset();

    this->start_local_date_days_i = 0;
    this->local_time_sec_f        = 0.0f;
    this->zulu_time_sec_f         = 0.0f;

    this->cycleCounter = 0;

    this->futureDayOfYearAfterTimeLapse = -1;
  }

  std::string set_date_and_hours(const std::string& inDateAndTime, const bool inIgnorePauseMode = false);                     // return error string, if there is any
  std::string timelapse_add_minutes(const int inMinutesToAdd, const int inHowManyCycles, const bool inIgnorePauseMode = false); // how many minutes to add and in how many seconds to do that. We will calculate the seconds to pass per cycle.
  void        flc_timelapse();                                                            // test timer and progress UTC time + local_day accordingly


  std::string timelapse_to_local_hour(int inFutureLocalHour, int inFutureMinutes, int inHowManyCycles, bool flag_inIgnoreActiveState = false, const bool inIgnorePauseMode = false); // provide the local hour and minutes you want to progress and in how fast. We will calculate the seconds to pass per cycle.
};


/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

/////////////////////////////////////
//// ThreeStoppers CLASS
/////////////////////////////////////

class ThreeStoppers
{
private:
  std::map<int, missionx::Timer> mapThreeStoppers;

public:

  ~ThreeStoppers() {}

  ThreeStoppers() 
  { 
    for (int i=1; i<4; ++i)
    {
      missionx::Timer t;
      Utils::addElementToMap(mapThreeStoppers, i, t);
    }

  }

  bool start_timer(int inTimerNo, float inSecondsToRun) 
  { 
    if (mxUtils::isElementExists(this->mapThreeStoppers, inTimerNo))
    {
      if (this->mapThreeStoppers[inTimerNo].isRunning())
        this->mapThreeStoppers[inTimerNo].reset();

      missionx::Timer::start( this->mapThreeStoppers[inTimerNo], inSecondsToRun );
      return true;
    }

    return false; // timer not exists
  }


  float stopTimer(int inTimerNo) 
  { 
    if (mxUtils::isElementExists(this->mapThreeStoppers, inTimerNo))
    {
      float returnVal = this->mapThreeStoppers[inTimerNo].getSecondsPassed();
      this->mapThreeStoppers[inTimerNo].reset();
      return returnVal;
    }

    return -1.0f; // not exists, an error
  }


  float getTimerTimePassed(int inTimerNo) 
  { 
    if (mxUtils::isElementExists(this->mapThreeStoppers, inTimerNo))
    {
      return this->mapThreeStoppers[inTimerNo].getSecondsPassed();
    }

    return -1.0f; // not exists, an error
  }

  bool getTimerEnded(int inTimerNo) 
  { 
    if (mxUtils::isElementExists(this->mapThreeStoppers, inTimerNo))
    {
      return  missionx::Timer::wasEnded( this->mapThreeStoppers[inTimerNo] );
    }

    return true; //
  }

  void resetAllTimers() 
  {
    for (auto& [key, tm] : mapThreeStoppers)
    {
      tm.reset();      
    }
  }

  void endMission() 
  { 
    resetAllTimers();
  }


  void stopPlugin() 
  { 
    resetAllTimers();  
  }
};




/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

/////////////////////////////////////
//// GLobalSettings CLASS
/////////////////////////////////////


class GLobalSettings : public mx_base_node //: public missionx::mxProperties
{
public:
  GLobalSettings();
  ~GLobalSettings();

  //mx_base_node node = mx_base_node(mxconst::get_GLOBAL_SETTINGS());

  bool flag_loadedFromSavepoint{ false };
  bool parse_node();

  // node pointers
  IXMLNode xFolders_ptr;
  IXMLNode xStartTime_ptr;
  IXMLNode xPosition_ptr;
  IXMLNode xBaseWeight_ptr;
  IXMLNode xDesigner_ptr;
  IXMLNode xTimer_ptr;
  IXMLNode xScoring_ptr; // v3.303.8.3
  IXMLNode xWeather_ptr; // v3.303.12
  IXMLNode xSavedWeather_ptr; // v3.303.13
  IXMLNode xCompatibility_ptr; // v24.12.2


  void clear();
};

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

/////////////////////////////////////
//// data_manager CLASS
/////////////////////////////////////


class data_manager
{
private:
  static uiLayer_enum generate_from_layer; // holds the layer were the RandomEngine was called from. This way we know if to use the UI Template options or not.
  static std::string active_acf;
  static std::string prev_acf;

public:
  data_manager();
  //virtual ~data_manager();

  // XPLANE version
  static int xplane_ver_i;
  static int xplane_using_modern_driver_b;
  static std::string mission_file_supported_versions; // v24.12.2
  // static std::vector<std::string> vecCurrentMissionFileSupportedVersions; // v24.12.2

  // missionx_conf node from missionx_conf.xml file
  static IXMLNode xMissionxPropertiesNode;

    // Map Layer
  static XPLMMapLayerID g_layer; // v3.305.4
  static int            s_num_cached_coords;
  static float          s_cached_x_coords[MAX_COORDS];   // The map x coordinates at which we will draw our icons; only the range [0, s_num_cached_coords) are valid
  static float          s_cached_y_coords[MAX_COORDS];   // The map y coordinates at which we will draw our icons; only the range [0, s_num_cached_coords) are valid
  static float          s_cached_lon_coords[MAX_COORDS]; // The real latitudes that correspond to our cached map (x, y) coordinates; only the range [0, s_num_cached_coords) are valid
  static float          s_cached_lat_coords[MAX_COORDS]; // The real latitudes that correspond to our cached map (x, y) coordinates; only the range [0, s_num_cached_coords) are valid
  static float          s_icon_width;                    // The width, in map units, that we should draw our icons.

  // XPSDK Plugin ID
  static const std::string plugin_sig;   // = "missionx_snagar.dev";
  static XPLMPluginID      mx_plugin_id; // XPLMFindPluginBySignature(const char* inSignature);

  constexpr const static float FONT_SIZE = 13.0; // TODO: will be deprecated 


  // Flight Leg statistics
  static mx_enrout_stats_strct                        strct_currentLegStats4UIDisplay; // v3.303.14 holds current leg stats
  static std::vector<missionx::mx_enrout_stats_strct> vecPreviousLegStats4UIDisplay; // holds finished leg stats
  static void gatherFlightLegStartStats(); // v3.303.14
  static void gatherFlightLegEndStatsAndStoreInVector(); // v3.303.14


  // position camera
  static XPLMCameraPosition_t mxCameraPosition;
  static int                  isLosingControl_i;

  // From which layer we called random engine
  static void                   setGenerateLayerFrom(missionx::uiLayer_enum inLayerName) { data_manager::generate_from_layer = inLayerName; }
  static missionx::uiLayer_enum getGeneratedFromLayer() { return data_manager::generate_from_layer; }
  static float                  Max_Slope_To_Land_On;

  // Timers defined by mission files. We should not have more than 3 timers. 1 Global and 2 internal at most that can fail the mission
  static std::map<std::string, missionx::Timer> mapFailureTimers;
  static std::string                            lowestFailTimerName_s;
  static std::string                            formated_fail_timer_as_text;
  static std::string                            get_fail_timer_title_formated_for_imgui(const std::string& in_default_title, const std::string prefix_to_foramted_string = "");
  static std::string                            get_fail_timer_in_formated_text(const std::string prefix_to_foramted_string = "");

  static missionx::mx_success_timer_info strct_success_timer_info; // v25.02.1

  // VR Related
  static XPLMDataRef g_vr_dref; // holds if in VR mode
  static void        flc_vr_state();
  static int         prev_vr_is_enabled_state; // store last loopback state of the VR

  // TimeLapse
  static missionx::TimeLapse timelapse;
  static std::string        set_local_time(int inHours, int inMinutes, int inDayOfYear_0_364, bool gForceTimeSet = false);
  //static bool               flag_mission_time_was_set; // v3.303.8.2 we will ignore timelapse until this flag will be true

  static void readPluginTextures();

  // core attributes
  static missionx::script_manager sm;

  static void seedAdHocParams(const std::string& inParams_s, mxProperties& inSmPropSeedValues); // v3.305.3 needs to be called prior to the sm.run_script()
  static bool execScript(const std::string& scriptName, mxProperties& inSmPropSeedValues, std::string_view inFailureMessage);

#ifdef APL
  // workaround to MAC not conserving sm pointer or ext_xxx values after initialization.
  static bool              ext_sm_init_success;
  static bool              ext_bas_open;
  static mb_interpreter_t* bas;
#endif

  static std::map<std::string, missionx::Objective>     mapObjectives;
  static std::map<std::string, missionx::Waypoint>      mapFlightLegs;
  static std::map<std::string, missionx::dataref_param> mapDref;
  static std::map<std::string, missionx::Trigger>       mapTriggers;

  // Inventory related
  static std::map<std::string, missionx::Inventory> mapInventories;    // v3.0.213.1
  // static std::map<std::string, missionx::Item>      mapItemBlueprints; // v3.0.213.1
  static missionx::Inventory planeInventoryCopy;    // v24.12.2


  static IXMLNode            xMainNode;                      // v3.0.241.1 Holds the copy of the <MISSION> root element
  static IXMLNode            xmlBlueprints;                  // v3.0.241.1 item blueprints
  static missionx::Inventory externalInventoryCopy;          // v3.0.241.1 external inventory copy // v24.12.2 renamed
  static std::string         active_external_inventory_name; // v3.0.251.1

  static std::map<std::string, missionx::Message> mapMessages;
  static std::map<std::string, missionx::obj3d>   map3dObj;                     // v3.0.200 //name, obj3d template// Holds static 3D Objects and should also hold moving objects (in future)
  static std::map<std::string, missionx::obj3d>   map3dInstances;               // v3.0.200 //name, obj3d instance // Holds static 3D Objects and should also hold moving objects (in future)
  static std::set<std::string>                    listDisplayStatic3dInstances; // v3.0.200 //instance name,  // pointer to instance object. When loading savepoint, we need to add pointers based on "canBeDisplayed" property.
  static std::set<std::string> listDisplayMoving3dInstances; // v3.0.202 //moving instance name,  // Only holds the instance name, the data is stored in map3dInstances // if this map is empty we can unregister draw callback.
  static void                  addInstanceNameToDisplayList(const std::string& instName);   // v3.0.207.1 // instance name to correct list based on the type of the 3D Object
  static void                  delInstanceNameFromDisplayList(const std::string& instName); // v3.0.207.1 // instance name to correct list based on the type of the 3D Object


  static IXMLNode xmlMappingNode; // v3.0.217.4 holds <MAPPING> element and its nodes that are used as a template to the "random.xml" mission this should replace xmlTemplateNodes. We copy nodes using deepCopy() function.
  static IXMLNode xmlGPS;         // v3.0.215.7 holds gps route xy data. Garmin G530.
  static IXMLNode xmlLoadedFMS;   // v3.0.231.1 holds FMS info from savepoint


  static Briefer                                      briefer; // holds briefer data
  static std::map<std::string, missionx::BrieferInfo> mapBrieferMissionList;
  static std::map<int, std::string>                   mapBrieferMissionListLocator; // will hold sequential number that will hold the key of mapBrieferMissionList.

  static std::map<std::string, std::string>             mapCurrentMissionTexturePathLocator; // {unique filename, path to file}   during mission load store all goals/global_settings/end status images
  static std::map<std::string, missionx::mxTextureFile> mapCurrentMissionTextures;           // current running mission images

  // USED by generic buttons and not NK. Plugin bitmap files map, no need to store in savepoint
  static std::map<std::string, missionx::mxTextureFile>    mapCachedPluginTextures;                      // plugin specific vecTextures
  static std::map<std::string, missionx::TemplateFileInfo> mapGenerateMissionTemplateFiles;        // Generate mission vecTextures // v3.0.217.2
  static missionx::TemplateFileInfo                        user_driven_template_info;              // v3.0.241.9 used in conjunction of the mission ui build layer
  static std::map<int, std::string>                        mapGenerateMissionTemplateFilesLocator; // v3.0.217.2 locator for template vecTextures. used in the briefer layer

  static void clearMissionLoadedTextures();
  static void releaseMessageStoryCachedTextures(); // v3.305.1
  static void clearRandomTemplateTextures(); // v3.0.217.2

  // Mission related
  static std::string           activeMissionName;
  static mx_mission_state_enum missionState;
  static std::string           currentLegName;
  
  static std::string           translate_enum_mission_state_to_string(mx_mission_state_enum mState);
  static mx_mission_state_enum translate_mission_state_to_enum(const std::string& mState); // v25.03.1 moved from mission.h



  static missionx::mx_base_node endMissionElement;
  static missionx::mx_base_node prop_userDefinedMission_ui; // v3.0.241.9


  static missionx::GLobalSettings mx_global_settings;        // v3.0.241.1 specialized class, will replace "mx_mission_properties_old"

  static mx_base_node mx_folders_properties;

  static mxProperties smPropSeedValues;

  static std::string missionSavepointFilePath;
  static std::string missionSavepointDrefFilePath;


  // Mission Images related
  static int                                              iMissionImageCounter;
  static std::map<std::string, missionx::mxTextureFile>   xp_mapMissionIconImages;
  static std::map<std::string, missionx::mxTextureFile>   xp_mapInvImages; // v3.0.303.5 inventory images
  static void                                             loadAllMissionsImages();
  static void                                             loadInventoryImages(); // v3.0.303.5 inventory images
  static void                                             loadStoryImage(missionx::Message *msg, const std::string &inImageName_vu); // v3.305.1
  static bool                                             flag_finished_load_inventory_images; // v3.0.303.5 inventory images

  static std::string errStr;

  // Choices v3.0.231.2 Choices
  static IXMLNode         xmlChoices; // v3.0.231.1 store all Choice sets inside one <choices> element
  static missionx::Choice mxChoice;   // v3.0.231.1 stores last pick XML choice
  static bool             prepare_choice_options(const std::string& inChoiceName);
  static bool             is_choice_name_exists(std::string& inChoiceName);

  // RealityXP related
  static std::string                      write_fpln_to_external_folder(); // v3.0.241.2 our first external utility is RealityXP.
  static std::deque<missionx::NavAidInfo> extractNavaidInfoFromPlanesFMS();
  static std::string                      prepare_flight_plan_for_GTN_RXP(std::deque<missionx::NavAidInfo>& inNavList);
  static std::string                      prepare_flight_plan_for_XPLN11(std::deque<missionx::NavAidInfo>& inNavList);
  static IXMLNode                         prepare_flight_plan_for_GNS530_RXP (const std::deque<missionx::NavAidInfo>& inNavList);
  static IXMLNode                         prepare_flight_plan_for_LTLNAV (const std::deque<missionx::NavAidInfo>& inNavList);


  // General members
  static void clear();
  static void stopMission(); // clears data specific to active mission. Let than clear()
  static void pluginStop();  // will be called when stop_plugin is called
  static void init();
  static void init_static();
  static void release_static();
  static void preparePluginFolders();

  static bool validateObjective(Objective& inObj, std::string& outError, std::string& outMsg);
  static bool validateFlightLegs(std::string& outError, std::string& outMsg);

  // v3.0.303 search target POI on all tasks in a given flight leg
  static bool search_for_targetPOI(IXMLNode& legNode, const std::string& objName);
  

  static void prepare_new_mission_folders ( const missionx::GLobalSettings& in_xmlGlobalSettings); // Prepare mission folder files like sound,object,scripts,metar

  static void addDataref(dataref_param inDref);

  static void setAbortMission(const std::string& inReason);

  // Inventory related
  static float current_plane_payload_weight_f;                       // v3.0.221.9 holds the weight of the plane after moving items between plane and inventory
  static void  prepareInventoryCopies(const std::string& inInventoryName); // copy original items to a copy container to work with Inventory layer
  static void  erase_items_with_zero_quantity_in_all_inventories();
  static void  erase_empty_inventory_item_nodes(const IXMLNode& pNode, const int &inLevel = 0); // v24.12.2

  // should be called on mission start/transaction commit/plane change
  static float calculatePlaneWeight(const missionx::Inventory& inSourceInventory, const bool &flag_apply_to_dataref, const int &inLayoutVersion);

  // v24.12.2: Calculates plane weight, and store it in: data_manager::current_plane_payload_weight_f
  static void  internally_calculateAndStorePlaneWeight(const missionx::Inventory& inSourceInventory, const bool &flag_apply_to_dataref, const int &inLayoutVersion);

  static void saveCheckpoint(IXMLNode& inParent); //    >>> load checkpoint is in read_mission_file class <<<

  // Add errors to loadErrors string
  //static std::string loadErrors;
  static std::list<std::string> lstLoadErrors; // v3.305.3
  static missionx::mx_base_node xLoadInfoNode; // v3.305.3

  


  static void        addLoadErr(const std::string& inErr);


  // Queue actions in Flight Loop Callback
  static std::queue<std::string> queThreadMessage; // holds messages from threads to display in briefer. use: "missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::set_briefer_text_message);" but set the que before hand
  static std::queue<missionx::mx_flc_pre_command> queFlcActions;
  static std::queue<missionx::mx_flc_pre_command> postFlcActions; // v3.0.146 // we will place actions in this Queue if we want them to happen after flc() so xplane updates its state. Example pause after position plane
  static void                                     clearAllQueues();

  ////// messages - Moved to QueMessageManager //////
  static int msgSeq;
  ////// END messages //////

  // CueInfo
  static void resetCueSettings();
  static void flc_cue(cue_actions_enum inAction);
  static int  seqCueInfo;
  static void drawCueInfo(); // v3.0.203
  static void deleteCueFromListCueInfoToDisplayInFlightLeg(const std::string& name_s);

  static void drawCueInfoOn2DMap(const float* inMapBoundsLeftTopRightBottom, const XPLMMapProjectionID * projection); // v3.305.4
  
  // GPS / FMS
  static void clearFMSEntries();
  static void setGPS(); // v3.0.215.7
  // Add entry to GPS/FMS.
  // -1 = add to the end. This is the default behavior
  static void addLatLonEntryToGPS(Point& inPoint, const int& inEntry = -1); // v3.0.241.10 b3

  // APT DAT OPTIMIZATION FLAGS
  static bool                                                   flag_apt_dat_optimization_is_running;
  static bool                                                   flag_generate_engine_is_running;
  static std::map<std::string, missionx::mx_aptdat_cached_info> cachedNavInfo_map;

  // Commands handling // v3.0.221.9
  static std::list<std::string>                       parseStringToCommandRef(const std::string& inCommandsAsString);
  static std::map<std::string, missionx::BindCommand> mapCommands;          // v3.0.221.9 store the command string and the reference to it using the XPLMFindCommand() function
  static std::map<std::string, missionx::BindCommand> mapCommandsWithTimer; // v3.0.221.15 rc3.5 holds commands that need to hold the press.
  static std::deque<std::string>                      queCommands;          // v3.0.221.9 holds a queue of commands to execute. Should execute a command once each flb, and not all together.
  static std::deque<std::string>                      queCommandsWithTimer; // v3.0.221.15; holds a queue of commands with begin/end timer to execute.

  // v3.0.231.1 map2d moved from briefer to data_manager. Accessible to all windows (including MXPAD).
  static std::map<int, missionx::mxTextureFile> maps2d_to_display; // v3.0.211.2 changed map data type, the nk_image will be created on the fly when needed // v3.0.200a2 current running mission images

  static IXMLRenderer xmlRender;

  // DISTNACE Related functions relative to Instances.
  // Return values: "-1": Instance is not active/available
  //                "Nn": distance in meters.
  static double get_distance_of_plane_to_instance_in_meters(const std::string& inInstanceName, std::string& errMsg);
  static double get_elev_of_plane_relative_to_instance_in_feet(const std::string& inInstanceName, std::string& errMsg);
  static double get_bearing_of_plane_to_instance_in_deg(const std::string& inInstanceName, std::string& errMsg);

  // Key Sniffer
  /*
   * MyKeySniffer
   *
   * This routnine receives keystrokes from the simulator as they are pressed.
   * A separate message is received for each key press and release as well as
   * keys being held down.
   *
   */
  /* record the key data. */
  typedef struct _mx_keySniffer
  {
    static char         keySnifferVirtualKey;
    static char         keySnifferChar;
    static XPLMKeyFlags keySnifferFlags;

    _mx_keySniffer()
    {
      keySnifferFlags      = 0;
      keySnifferVirtualKey = 0;
      keySnifferChar       = 0;
    }

  } mx_keySniffer;

  static mx_keySniffer mx_global_key_sniffer;


  //// folders ///
  static std::string getMissionsRootPath();                    // v3.0.129
  static void        read_element_mapping(const std::string& inFile); // v3.0.217.4
  static int         seqElementId;

  static missionx::Point getCameraLocationTerrainInfo();


  ///// SHADER
  //// static missionx::MxShader mxFontShader;


#define MAX_VERTEX_MEMORY  512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024


  // INSTANCING - https://developer.x-plane.com/code-sample/x-plane-10-instancing-compatibility-wrapper/#xplm_instanceh
  static void loadAll3dObjectFiles();
  static void releaseMissionInstancesAnd3DObjects();
  static void refresh3DInstancesAndPointLocation(); // will set all instances "isDisplay" to false and will re-calculate local coordinate


  // v3.0.221.7 SHARED DATAREFS
  static std::map<std::string, missionx::dataref_param> mapSharedParams;
  static void                                           setSharedDatarefData();

  static std::string start_cold_and_dark_drefs;
  static std::string execute_commands(const std::string& inCommands); // v3.0.221.11+

  // METAR
  static std::string current_metar_file_s;
  static std::string metar_file_to_inject_s;
  static void        inject_metar_file(const std::string& inFileName); // this will set metar_file_s value and will set the flcQue to execute metar injection

  // Draw Script - v3.0.224.2
  static std::string draw_script;

  // parse leg_message v3.0.223.4
  static std::string selectedMissionKey;
  static void        parse_leg_dynamic_messages(missionx::Waypoint& inLeg);
  static void        parse_leg_DisplayObjects(missionx::Waypoint& inLeg);
  static void        translate_relative_3d_display_object_text(std::string& inOutText); // v3.303.11 modify special keyword measures that should be dynamic to acf, like wing_span

  static void refresh_3d_objects_and_cues_after_location_transition();

  // SQLite
  static missionx::dbase db_xp_airports;
  static missionx::dbase db_cache; // v3.0.255.3
  static missionx::dbase db_stats; // v3.0.255.1 holds active mission plane stats
  static bool            init_xp_airport_db();
  static bool            init_xp_airport_db2();
  static bool            delete_db_file(const std::string & inPathAndFile); // v3.303.8.3
  static bool            init_stats_db(); // v3.0.255.1
  static bool            db_connect(missionx::dbase& db, const std::string& inPathToDbFile);
  static void            db_close_all_databases();
  static void            db_close_database(dbase &inDB); // v3.303.8.3

  // External FlightPlan
  static std::vector<missionx::mx_ext_internet_fpln_strct>    tableExternalFPLN_simbrief_vec; // v25.03.3
  static std::vector<missionx::mx_ext_internet_fpln_strct>    tableExternalFPLN_vec;
  static std::map<int, missionx::mx_ext_internet_fpln_strct*> indexPointer_forExternalFPLN_tableVector; // index pointer to the vector tableExternalFPLN_vec

  static void                           fetch_METAR(std::unordered_map<int, mx_nav_data_strct>* mapNavaidData, missionx::mxFetchState_enum* outState, std::string* outStatusMessage, std::string* outErrorMsg, bool* lockThread);
  static IXMLNode                       jsonConvertedNode_xml; // holds the json converted data so we would use IXMLParser to read data safely
  static std::vector<std::future<void>> mFetchFutures;         //  holds async pointers

  static void fetch_fpln_from_simbrief_site(missionx::base_thread::thread_state* inoutThreadState, const std::string &in_pilot_id, missionx::mxFetchState_enum* outState, std::string* outStatusMessage);
  static void fetch_fpln_from_external_site(missionx::base_thread::thread_state* inoutThreadState, const IXMLNode& inUserPref, missionx::mxFetchState_enum* outState, std::string* outStatusMessage);

  static int                      overpass_counter_i;            // v3.0.255.4.1
  static std::vector<std::string> vecOverpassUrls;               // v3.0.255.4.1
  static int                      overpass_user_picked_combo_i;  // v3.0.255.4.1
  static int                      overpass_last_url_indx_used_i; // v3.0.255.4.1
  static std::string              fetch_overpass_info(const std::string& in_url_s, std::string& outError); // This function is part of the RandomEngine class call, which is threaded already.

  // ILS SQLITE
  static void                                               fetch_ils_rw_from_sqlite(missionx::NavAidInfo* inFrom_navaid, std::string* inFilterQuery, missionx::mxFetchState_enum* outState, std::string* outStatusMessage);
  static void                                               fetch_nav_data_from_sqlite(std::unordered_map<int, mx_nav_data_strct>* mapNavaidData, std::string* inICAO, missionx::Point* inPlanePos, missionx::mxFetchState_enum* outState, std::string* outStatusMessage);
  static std::vector<missionx::mx_ils_airport_row_strct>    table_ILS_rows_vec;
  static std::map<int, missionx::mx_ils_airport_row_strct*> indexPointer_for_ILS_rows_tableVector;                                                          // index pointer to the vector table_ILS_rows_vec
  static void                                               fetch_last_mission_stats(missionx::mxFetchState_enum* outState, std::string* outStatusMessage); // the query is a constant one on same table "stats"
  static missionx::mx_mission_stats_strct                   mission_stats_from_query;
  static std::vector<missionx::mx_stats_data>               get_landing_stats_as_vec(); // v3.303.8.3
  static missionx::GatherStats                              gather_stats;               // v3.0.255.1

  //static bool fetch_does_airport_has_valid_rw_based_on_filters_sqlite(missionx::NavAidInfo* inFrom_navaid, std::string* inFilter, std::string* outStatusMessage);
  static void reset_runway_search_filter();

  // cURL  
  static missionx::mutex s_thread_sync_mutex;
  static CURL*       curl;
  static std::string curl_result_s;

  // Error Mission Load String
  static std::string load_error_message; // holds the ERROR string when trying to load a mission file or any file(?). Should display in the uiImguiBrifer window message footer line.

  // will fetch the closest ICAO to plane location. // v3.303.8.3 extended function by allowing snding custom position instead of getting the plane position. This is used with the stats table after the mission ended and not during flight.
  static missionx::NavAidInfo getPlaneAirportOrNearestICAO(const bool& inOnlySearchInDatabase = false, const double& inLat = 0.0, const double& inLon = 0.0, bool inIsThread = false);  // v3.303.14 added inIsThread
  static missionx::NavAidInfo getICAO_info(const std::string& inICAO); // will fetch the closest ICAO to plane location
  static missionx::NavAidInfo get_and_guess_nav_info(const std::string& in_id_nav_name, const missionx::Point &prevPoint); // will fetch the closest guest navaid to a given coordinate

  // v3.0.255.2
  static missionx::mx_wp_guess_result get_nearest_guessed_navaid_based_on_coordinate(missionx::mxVec2f& inPos) noexcept;

  // v3.0.255.3
  static void validate_display_object_file_existence(const std::string& inMissionFile, const IXMLNode& parentOfLegNodes_ptr, const IXMLNode &inGlobalSettingsNode, const IXMLNode &inObjectTemplatesNode, std::string& outErr);
  static void set_found_missing_3D_object_files(bool inVal) { flag_found_missing_3D_object_files = inVal; }
  static bool get_are_there_missing_3D_object_files() { return flag_found_missing_3D_object_files; }

  // sqlite related functions
  static void                                                        sqlite_test_db_validity(dbase& inDB, const bool isThreaded = false);
  static void                                                        set_flag_rebuild_apt_dat(const bool inVal) { flag_rebuild_apt_dat = inVal; }
  static bool                                                        get_flag_rebuild_apt_dat() { return flag_rebuild_apt_dat; }
  static std::map<std::string, std::string>                          row_gather_db_data;
  static std::unordered_map<int, std::map<std::string, std::string>> reasultTable;

  // v3.0.255.4 add overpas error message variable to display to end user
  static std::string overpass_fetch_err;

  // v3.0.301
  static std::map<int, missionx::mx_local_fpln_strct> read_and_parse_littleNavMap_fpln(const std::string &inPathAndFile);
  static std::map<int, missionx::mx_local_fpln_strct> map_tableOfParsedFpln;

  static IXMLNode init_littlenavmap_missionInfo(IXMLNode & inNode);

  /// <summary>
  /// Prepare flight plan in the random folder based on the LittleNavMap flight plan table.
  /// </summary>
  /// <param name="inOriginalFlightPlanFileName">the original fpln file name</param>
  /// <param name="inMainNode">this is the dummy node we created in the briefer screen "mx_conv_layer.xConvMainNode". The struct is: <DUMMY><leg[]><conv_info>
  /// We should conver <conv_info> to <mission_info> and each Leg in the table into its own flight_leg.
  /// </param>
  /// 
  /// <param name="in_tableOfLoadedFpln">Holds all flight legs and their order. It also contain whether a waypoint is ignored or is a flight leg or "just" a way point</param>
  /// <returns></returns>
  static void add_advanceSettingsDateTime_and_Weather_to_node(IXMLNode &xGlobalSettings, IXMLNode & inPropNode, const std::string & inCurrentWeatherDatarefs_s);
  static bool generate_missionx_mission_file_from_convert_screen(missionx::mx_base_node inPropNode, IXMLNode& inMainNode, IXMLNode& inGlobalSettingsFromConversionFile, std::map<int, missionx::mx_local_fpln_strct> in_map_tableOfParsedFpln, bool inStoreState_b, bool inGenerateNewGlobalSettingsNode);

  // Trigger UI specialized function members
  static std::string     getPoint_as_stringFromPlaneCamera(const std::string& inDatarefStringLat, const std::string inDatarefStringLon, char origin_c = 'p'); // origin should be "p" for plane or "c" for camera
  static missionx::Point getPlane_or_Camera_position_as_Point(char origin_c = 'p');                                                                          // origin should be "p" for plane or "c" for camera

  // global non stored setup options
  static bool flag_setupChangeHeadingEvenIfPlaneIn_20meter_radius;
  static bool flag_setupForcePlanePositioningAtMissionStart;
  static bool flag_setupEnableDesignerMode; // v24.03.2
  static bool flag_setupShowDebugTabDuringMission;
  static bool flag_setupSkipMissionAbortIfScriptIsInvalid; // v3.305.3
  static bool flag_setupDisplayAutoSkipInStoryMessage; // v3.305.3
  static bool flag_setupAutoSkipStoryMessage; // v3.305.3
  static bool flag_setupShowDebugMessageTab; // v3.305.4
  static bool flag_setupUseXP11InventoryUI; // v24.12.2 toggle between inventory ui layout (with stations, xp12, and without xp11).

  // static int get_inv_layout_based_on_mission_ver_and_compatibility_node();
  // v25.03.1
  static int get_inv_layout_based_on_mission_ver_and_compatibility_node(const std::string &in_mission_format_versions, const IXMLNode & in_compatibility_node, const bool &in_flag_setupUseXP11InventoryUI);

  static void position_camera(XPLMCameraPosition_t &inCameraPos, float inLat, float inLon);
  static void set_camera_poistion_loc_rule(int inIsLosingControl_i);

  static ImVec2 getImageSizeBasedOnBorders(float inWidth, float inHeight, float inBoundX, float inBoundY);

  // v3.303.9.1 
  static bool flag_gather_acf_info_thread_is_running;
  static bool flag_abort_gather_acf_info_thread;
  static int  iGatherAcfTryCounter; // v3.303.13
  static void gather_custom_acf_datarefs_as_a_thread(const std::string &inAcfPath, bool *bThreadIsRunning, bool *bAbortThread); // this function should run as a thread and use flag_gather_acf_info_thread_is_running to flag its activity We will call this function every mission start or plane change while mission is running.
  // v24.12.2
  //static void gather_acf_cargo_data(Inventory &inoutPlaneInventory); // v24.12.2 this function will run in the main flight callback.
  // static void parse_max_weight_line(const std::string& line, std::map<int, float>& mapMaxWeight, int& outMaxStations); // v24.12.2
  // static void parse_station_name_line(const std::string& line, std::map<int, std::string>& mapStationNames);                         // v24.12.2
  static missionx::dataref_param dref_acf_station_max_kgs_f_arr; // v24.12.2
  static missionx::dataref_param dref_m_stations_kgs_f_arr; // v24.12.2

  // Check if item can move to the "target inventory".
  static missionx::mx_return     check_mandatory_item_move(const IXMLNode & item_node, const std::string & target_inventory_name ); // v24.12.2

  // v3.303.11
  static std::string getTitleOrNameFromNode(mx_base_node& inBaseNode); // return title or name if they have value. Title has precedence.

  
  static void apply_dataref_based_on_key_value_strings(const std::string& inKey, std::string& inValue); // v3.303.12  
  static void apply_datarefs_based_on_string_parsing(const std::string& inDatarefs, const std::string& inBetweenDatarefsDelimiter = "|", const std::string& inEqualSignChar = "="); // v3.303.12  
  static void apply_datarefs_from_text_based_on_parent_node_and_tag_name(IXMLNode& inParentNode, const std::string& inTagName);// v3.305.1 renamed   // specialized function to pick the "TEXT" clear value of an element and then call  
  static void set_success_or_reset_tasks_state(const std::string& inObjName, const std::string& inTaskList, const missionx::enums::mx_action_from_trigger_enum& in_action, const std::string& inCurrentTask = ""); // v25.02.1 mainly used from triggers
  static void set_trigger_state(const missionx::Trigger &inCallingTrig, const std::string& inTrigList, const missionx::enums::mx_action_from_trigger_enum& in_action); // v25.02.1 mainly used from triggers


  static std::map<int, std::unordered_map<std::string, std::string> > mapWeatherPreDefinedStrct_xp11;
  static std::map<int, std::unordered_map<std::string, std::string> > mapWeatherPreDefinedStrct_xp12;

  // v3.303.13
  static std::string get_weather_state(const int& inCode = -1); // will prepare a string of the weather datarefs or string of weather based on pre-defined dataref values
  static bool        addAndLoadTextureMapNodeToLeg(const std::string& inFileName, const std::string& inLegName);
  
  // image
  static missionx::mutex s_ImageLoadMutex; // v3.303.14
  static missionx::mutex s_StoryImageLoadMutex; // v3.305.1 Added in the hope it prevent the sim from crashing
  static missionx::mutex mt_SortTriggersMutex; // v3.305.2 Used with the sorting trigger function to optimize evaluation performace

  static bool loadImage(const std::string& inFileName, std::map<std::string, mxTextureFile>& in_outTextureMapToStore);

  // v3.305.2
  #ifdef USE_TRIGGER_OPTIMIZATION // v3.305.2
  static void optimizeLegTriggers_thread(base_thread::thread_state* outThreadState, missionx::Point* inRefPoint, std::map<std::string, missionx::Trigger> * inMapTriggers, std::list<std::string>* inListTriggersByDistance, std::list<std::string>* outListTriggers_ptr);
  
  typedef struct _mx_opt_leg_triggers_strct
  {
    missionx::Point           optLegTriggers_thread_planePos;
    base_thread::thread_state optLegTriggers_thread_state;
    std::thread               optLegTriggers_thread_ref;
    bool                      optLegTriggers_thread_use_flag;
  } mx_opt_leg_triggers_strct;

  static mx_opt_leg_triggers_strct optimize_leg_triggers_strct;
  #endif

  static int             strct_flight_leg_info_totalMapsCounter; // shared data with the ui briefer
  static missionx::Point write_plane_position_to_log_file();     // 3.303.14
  static missionx::Point write_camera_position_to_log_file();    // 3.303.14
  static std::string     write_weather_state_to_log_file();      // will write the current datarefs related to weather into the log file so designer could use it to define a weather state they prefer to use for their missions. IT might not be 100% reproducable every time but it is better then nothing.

  static missionx::ThreeStoppers mxThreeStoppers;

  //static std::unordered_map<std::string, missionx::dataref_param> mapInterpolDatarefs; // v3.303.13
  static std::map<std::string, missionx::dataref_param> mapInterpolDatarefs; // v3.303.13
  static void clear_all_datarefs_from_interpolation_map(); // v3.305.3
  static void remove_dataref_from_interpolation_map(const std::string& inDatarefs); // v3.305.3
  static void do_datarefs_interpolation(const int inSeconds, const int inCycles, const std::string& inDatarefs); // v3.303.13
  static void flc_datarefs_interpolation(); // v3.303.13

  // v3.303.14 Oil Rig
  static const std::unordered_map<int, std::vector<missionx::mx_mission_subcategory_type>> mapMissionCategoriesCodes;

  static const std::string getMissionSubcategoryTranslationCode(int in_missionCodeType, int in_subCategoryCode);

  //// v3.305.1b
  static mx_sceneryOrPlaneLoad_state_strct strct_sceneryOrPlaneLoadState;

  // v3.305.2
  static std::list<missionx::messageLine_strct> listOfMessageStoryMessages;
  static void                                   addMessageToHistoryList(std::string inWho, std::string inText,missionx::mxVec4 inColor );

  // v24.02.5
  static std::map<std::string, std::string> mapQueries;
  static missionx::base_thread::thread_state threadStateMetar;

  // v25.02.1 function will return "true" if "template load" was successful.
  static bool find_and_read_template_file(const std::string &inFileName);

  // v25.03.1
  static void flc_acf_change();
  static void set_acf(const std::string& inFileName); // only set the current plane filename without calling "gather_acf_info" function.
  static void set_active_acf_and_gather_info(const std::string& inFileName); // set the current plane filename and call the "gather_acf_info" function.

 private:
  static bool flag_found_missing_3D_object_files;
  static bool flag_rebuild_apt_dat;
  
}; // end class

}


#endif
