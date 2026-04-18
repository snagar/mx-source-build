#ifndef WINIMGUIBRIEFER_H_
#define WINIMGUIBRIEFER_H_
#pragma once

#include "shared_ui_types.hpp"
#include "../../core/thread/base_thread.hpp"

// Definitions for OpenFontIcons
#include <IconsFontAwesome5.h> // inside libs/imgui4xp
#include <ImgWindow/ImgWindow.h> // inside libs/imgui4xp

#include "ui_conv_screen.h"
#include "ui_nav_screen.h"

namespace missionx
{

// v24.12.2
typedef enum class _ui_inv_regions
  : int8_t
{
  plane          = 0, // left side
  external_store = 1
} mx_ui_inv_regions;

enum class mx_ui_text_type // v3.0.301
{
  inputText = 0,
  inputHintText,
  inputMultiLineText,
  text,
  coloredText
};

class WinImguiBriefer : public ImgWindow
{
private:
  bool flag_displayedOnce{ false };

  ImVec4 countdown_textColorVec4;
  ImVec4 countdown_success_textColorVec4; // v25.06.1


public:
  WinImguiBriefer (int                  left,
                   int                  top,
                   int                  right,
                   int                  bot,
                   XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle, // xplm_WindowDecorationSelfDecoratedResizable
                   XPLMWindowLayer      layer      = xplm_WindowLayerFloatingWindows);


  // -------------------------------------------------
  // Child UI classes to decrease the main class size.
  // -------------------------------------------------
  std::unique_ptr<ui_conv_screen> m_ui_conv_screen;
  std::unique_ptr<ui_nav_screen> m_ui_nav_screen;


  void  setLayer (missionx::uiLayer_enum inLayer);
  float calc_and_getNewFontScaledSize (float inNewSize); // value should be +0.N or -0.N
  void  add_abort_all_channels_debug (); // v24026
  void  add_pause_in_2d_mode ();
  void  add_font_size_scale_buttons ();
  void  add_skewed_marker_checkbox (); // v3.0.253.6
  void  add_ui_start_mission_button (missionx::mx_window_actions inActionToExecute = mx_window_actions::ACTION_NONE);
  void  add_ui_warning_messages_button ();
  void  add_ui_ils_vfr_search_airports_button (missionx::mx_window_actions inActionToExecute = mx_window_actions::ACTION_NONE);
  void  add_ui_abort_mission_creation_button (missionx::mx_window_actions inActionToExecute = mx_window_actions::ACTION_ABORT_RANDOM_ENGINE_RUN);
  void  add_ui_expose_all_gps_waypoints (missionx::mx_window_actions inActionToExecute = missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
  void  add_ui_suppress_distance_messages_checkbox_ui (missionx::mx_window_actions inActionToExecute = missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS); // v25.02.1
  void  add_ui_default_weights (); // v25.02.1
  void  add_message_text (); // v3.305.1
  void  add_story_next_button (); // v3.305.1
  void  add_story_message_history_text (); // v3.305.2
  void  add_info_to_flight_leg (); // v3.305.2
  void  add_debug_info (); // v3.305.2
  void  add_flight_planning (); // v24.03.1
  void  add_other_settings_header( bool in_plane_is_helo, bool bPickedMedevacMission, bool bPickedOilRigMission ); // v26.04.1
  void  action_prepare_dynamic_mission_properties_and_call_generate_action(const float &in_distance_min, const float & in_distance_max, const bool & in_add_start_from_plane_position = true); // v26.04.1
  bool  add_ui_generate_button(); // v26.04.2

  void flc () override;
  void execAction (mx_window_actions actionCommand); // special function to handle specific requests from outside the window


  // Counter for the number of windows opened
  static int num_win;
  // I am window number...
  const int myWinNum;
  // Note to myself that a change of window mode is requested
  XPLMWindowPositioningMode nextWinPosMode = -1;
  // Our flight loop callback in case we need one
  XPLMFlightLoopID flId = nullptr;

  struct tableDataTy
  {
    std::string reg;
    std::string model;
    std::string typecode;
    std::string owner;
    float       heading   = 0.0f;
    bool        turnsLeft = false;
    bool        filtered  = true; // included in search result?

    // is s (upper cased!) in any text?
    //bool contains (const std::string &s) const;
  };
  typedef std::vector<tableDataTy> tableDataListTy;

  XPLMWindowID     mWindow;
  const static int WINDOW_MAX_WIDTH;
  const static int WINDOWS_MAX_HEIGHT; //
  const int        LINE_HEIGHT{ 20 };
  const float      BTN_DISTANCE_PAD{ 40.0f };
  const int        BTN_PADDING{ 2 };
  const float      COMPENSATE_OFSET_RIGHT_OF_FIRST_BTN_IN_LINE{ 5.0f }; // when we draw first button, the text does not align correctly relative to middle of button + text, it is offset to the left.
  int              win_pad{ 75 }; ///< distance from left and top border
  const int        win_coll_pad{ 30 }; ///< offset of collated windows

  // // v26.04.1
  // enum class mx_act_phase_enum : uint8_t
  // {
  //   phase_pick = 0,
  //   phase_accept
  // };

  // struct activity_btn_info_strct
  // {
  //   // list parameters (needed for the semi-automation activity screen)
  //   struct st_distance
  //   {
  //     float min {5.0f};
  //     float lowest_max{20.0f};
  //     float max {50.0f};
  //   };
  //   struct st_targets
  //   {
  //     int min {1};
  //     int max {1};
  //   };
  //
  //   int          id{ -1 };
  //   int          final_legs_no_to_generate {1}; // Will hold the final number of legs to generate.
  //
  //   st_distance  distance_min_max {5.0f, 20.0f, 50.0f};
  //   st_targets   legs_min_max {1, 4}; // store the number of targets per "action type picked". Example: Medevac will only have 2 legs.
  //
  //   missionx::enums::mx_semi_activities_enum activity{missionx::enums::mx_semi_activities_enum::act_none};
  //   missionx::mx_plane_types_enum            plane_type{missionx::mx_plane_types_enum::plane_type_any};
  //
  //   std::string  imgName;
  //   std::string  label;
  //   std::string  tip;
  //
  //   // Non-List parameters
  //   std::string desc {""}; // we need initializer
  //   float max_distance_slider_f {0.0f};
  //
  //   void reset()
  //   {
  //     id                        = -1;
  //     final_legs_no_to_generate = 1;
  //     distance_min_max          = {5.0, 20.0f, 50.0};
  //     legs_min_max              = {1, 4};
  //     activity                  = missionx::enums::mx_semi_activities_enum::act_none;
  //     plane_type                = missionx::mx_plane_types_enum::plane_type_any;
  //     imgName.clear();
  //     label.clear();
  //     tip.clear();
  //
  //     // temporary data
  //     desc.clear();
  //     max_distance_slider_f = 0.0f;
  //   }
  //
  //
  //   [[nodiscard]] st_distance get_mission_area() const
  //   {
  //     st_distance mission_area;
  //
  //     if (this->activity < missionx::enums::mx_semi_activities_enum::act_turboprops && (this->max_distance_slider_f - this->distance_min_max.min < 10.0f))
  //       mission_area.min = distance_min_max.min;
  //     else
  //       mission_area.min = max_distance_slider_f - ( 0.5f * (max_distance_slider_f - distance_min_max.min ) );
  //
  //     mission_area.max = max_distance_slider_f;
  //     mission_area.lowest_max = distance_min_max.lowest_max;
  //
  //     return mission_area;
  //   }
  //
  //   void randomize_max_distance()
  //   {
  //     max_distance_slider_f = std::roundf(  static_cast<float>( Utils::getRandomRealNumber(distance_min_max.lowest_max, distance_min_max.max) ) );
  //   }
  //
  //   int randomize_no_of_legs()
  //   {
  //      final_legs_no_to_generate = legs_min_max.min;
  //
  //     if (legs_min_max.min < legs_min_max.max)
  //     {
  //       // randomize pick num of legs
  //       final_legs_no_to_generate = Utils::getRandomIntNumber(legs_min_max.min, legs_min_max.max);
  //     }
  //
  //     return final_legs_no_to_generate;
  //
  //   } // end randomize_no_of_legs
  //
  //   // prepare desc
  //   void prepare_the_semi_activity_description()
  //   {
  //     // Type of plane
  //     const std::string plane_type_desc = (id < 5)? "You will fly a helos mission" : "You will fly a plane mission";
  //
  //     // Flight area description
  //      const auto  [min_area, low_max, max_area] = get_mission_area();
  //     //const auto min_area = calc_min_area_distance();
  //
  //
  //     std::string flight_area_desc = fmt::format("{:.0f} to {:.0f}", min_area, max_area);
  //     if (activity == enums::mx_semi_activities_enum::act_helos_cargo_oilrig || activity == enums::mx_semi_activities_enum::act_helos_medevac_oilrig)
  //       flight_area_desc            = fmt::format("determined based on the oil rig search area");
  //     else if (activity == enums::mx_semi_activities_enum::act_helos_medevac_surprise_me)
  //       flight_area_desc            = fmt::format("determined by the plugin");
  //
  //     const std::string area_desc  = fmt::format("Flight area: {}", flight_area_desc);
  //
  //
  //     // no. of legs
  //     const std::string no_of_legs_desc = fmt::format("You will have: {}", (final_legs_no_to_generate < 2)? " one landing location" : fmt::format(" up to {}, landing locations", final_legs_no_to_generate));
  //
  //     // construct description
  //     desc = fmt::format("{}.\n\n{}.\n{}.", plane_type_desc, area_desc, no_of_legs_desc);
  //   }
  // };

  // ids: 1-4 will be kept for Helos accident, Hellos Surprise me, Hellos oilrig and any medevac activity
  const std::list<activity_btn_info_strct> list_semi_auto_activities = {
    {.id=1, .final_legs_no_to_generate=2, .distance_min_max{5.0f, 10.0f, 30.0f}, .legs_min_max{2, 2}, .activity=missionx::enums::mx_semi_activities_enum::act_helos_medevac_accident, .plane_type=missionx::mx_plane_types_enum::plane_type_helos, .imgName=mxconst::get_BITMAP_BTN_ACT_HELOS_ACC(), .label="Accident", .tip="Respond to a medevac accident at a real-world location (OSM-based)"},
    {.id=2, .final_legs_no_to_generate=2, .distance_min_max{5.0f, 35.0f, 75.0f}, .legs_min_max{2, 2}, .activity=missionx::enums::mx_semi_activities_enum::act_helos_medevac_surprise_me, .plane_type=missionx::mx_plane_types_enum::plane_type_helos, .imgName=mxconst::get_BITMAP_BTN_ACT_HELOS_SRP(), .label="Rescue", .tip="Medevac rescue mission based on OSM location"},
    {.id=3, .final_legs_no_to_generate=2, .distance_min_max{5.0f, 30.0f, 70.0f}, .legs_min_max{2, 2}, .activity=missionx::enums::mx_semi_activities_enum::act_helos_cargo_oilrig, .plane_type=missionx::mx_plane_types_enum::plane_type_helos, .imgName=mxconst::get_BITMAP_BTN_ACT_HELOS_OIL(), .label="Oilrig", .tip="Take a flight to an offshore oil rig"},
    {.id=5, .final_legs_no_to_generate=2, .distance_min_max{20.0f, 30.0f, 80.0f}, .legs_min_max{1, 4}, .activity=missionx::enums::mx_semi_activities_enum::act_props, .plane_type=missionx::mx_plane_types_enum::plane_type_props, .imgName=mxconst::get_BITMAP_BTN_ACT_GA(), .label="GA", .tip="Explore nearby airports and airfields."},
    {.id=6, .final_legs_no_to_generate=1, .distance_min_max{50.0f, 150.0f, 800.0f}, .legs_min_max{1, 2}, .activity=missionx::enums::mx_semi_activities_enum::act_turboprops, .plane_type=missionx::mx_plane_types_enum::plane_type_turboprops, .imgName=mxconst::get_BITMAP_BTN_ACT_TURBOPROP(), .label="Turboprop", .tip="Like Jet, but more versatile"},
    {.id=7, .final_legs_no_to_generate=1, .distance_min_max{120.0f, 200.0f, 1500.0f}, .legs_min_max{1, 1}, .activity=missionx::enums::mx_semi_activities_enum::act_jets, .plane_type=missionx::mx_plane_types_enum::plane_type_jets, .imgName=mxconst::get_BITMAP_BTN_ACT_JET(), .label="Jet", .tip="Soar through the skies in style"},
    {.id = 8, .final_legs_no_to_generate = 1, .distance_min_max{200.0f, 300.0f, 1500.0f}, .legs_min_max{1, 1}, .activity = missionx::enums::mx_semi_activities_enum::act_airline_medium, .plane_type = missionx::mx_plane_types_enum::plane_type_airline, .imgName = mxconst::get_BITMAP_BTN_ACT_AIRLINE(), .label = "Airline", .tip = "Fly medium-haul routes"},
    {.id = 9, .final_legs_no_to_generate = 1, .distance_min_max{500.0f, 600.0f, 6000.0f}, .legs_min_max{1, 1}, .activity = missionx::enums::mx_semi_activities_enum::act_heavy_airline_long, .plane_type = missionx::mx_plane_types_enum::plane_type_heavy_airline, .imgName = mxconst::get_BITMAP_BTN_ACT_AIRLINE_H(), .label = "Airline Heavy", .tip = "Fly long-haul routes"},
    {.id = 10, .final_legs_no_to_generate = 1, .distance_min_max{200.0f, 400.0f, 4000.0f}, .legs_min_max{1, 1}, .activity = missionx::enums::mx_semi_activities_enum::act_cargo_medium, .plane_type = missionx::mx_plane_types_enum::plane_type_cargo, .imgName = mxconst::get_BITMAP_BTN_ACT_CARGO(), .label = "Cargo", .tip = "Fly medium-haul cargo routes"},
    {.id = 11, .final_legs_no_to_generate = 1, .distance_min_max{500.0f, 600.0f, 6000.0f}, .legs_min_max{1, 1}, .activity = missionx::enums::mx_semi_activities_enum::act_cargo_heavy_long, .plane_type = missionx::mx_plane_types_enum::plane_type_heavy_cargo, .imgName = mxconst::get_BITMAP_BTN_ACT_CARGO_H (), .label = "Cargo Heavy", .tip = "Fly long-haul cargo routes"},
  };

  // // -------------------------------------------
  // // -- STRUCT user mission creation variables
  // // -------------------------------------------
  // struct mx_user_create_mission_layer
  // {
  //   enum class mx_dynamic_fpln_screen
  // : uint8_t
  //   {
  //     ext_home = 0,
  //     ext_option_a,
  //     ext_option_b
  //   };
  //
  //   bool flag_first_time{ true };
  //   mx_dynamic_fpln_screen child_screen{ mx_dynamic_fpln_screen::ext_home };
  //
  //   mx_layer_state_enum layer_state{ missionx::mx_layer_state_enum::not_initialized }; // v3.0.253.9
  //
  //   // v26.04.1 semi-automated mission creation
  //   mx_act_phase_enum act_phase_enum{ mx_act_phase_enum::phase_pick }; // v26.04.1
  //   activity_btn_info_strct user_semi_act_picked; // v26.04.1
  //   int                                      headerIndex{ 0 }; // v25.03.3. Used only with mapSetupHeaders
  //   std::unordered_map<int, mx_header_state> mapSemiOptionsHeaders = { { headerIndex, mx_header_state ("Custom Tweaks", true) } }; // header is open by default
  //
  //
  //   int            iRadioMissionTypePicked{ static_cast<int> (missionx::mx_ui_mission_type::medevac) }; // which type of template user picked ?
  //   int            iMissionSubCategoryPicked{ -1 }; // v3.303.14 // v25.06.1 init -1 which is not valid
  //   mx_plane_types_enum iRadioPlaneType{ missionx::mx_plane_types_enum::plane_type_helos };
  //   bool           flag_narrow_helos_filtering{ false };
  //
  //
  //
  //   // sliders
  //   float dyn_sliderVal1 = (float)mxconst::SLIDER_MIN_RND_DIST; // min distance
  //   float dyn_sliderVal2 = dyn_sliderVal1 * 1.2f; // max distance
  //
  //   std::string dyn_slider1_lbl = "Min distance [" + Utils::formatNumber<float> (mxconst::SLIDER_MIN_RND_DIST, 0) + "..." + Utils::formatNumber<float> (mxconst::SLIDER_MIN_RND_DIST * 1.2, 0) + "]";
  //   std::string dyn_slider2_lbl = "Max distance [" + Utils::formatNumber<float> (mxconst::SLIDER_MIN_RND_DIST * 1.2, 0) + "..." + Utils::formatNumber<float> (mxconst::SLIDER_MIN_RND_DIST * 5.0, 0) + "]";
  //
  //   // OSM checkbox
  //   bool flag_use_osm{ false };
  //   bool flag_use_web_osm{ false };
  //
  //   // flight legs
  //   int iNumberOfFlighLegs{ 2 };
  //
  //   // overpass filter // v3.0.253.4
  //   const std::string overpass_original_filter{ mxconst::get_DEFAULT_OVERPASS_WAYS_FILTER () }; // { "[highway=primary][highway=secondary][highway=tertiary][highway=residential][highway=service][highway=living_street][highway=track]" };
  //   std::string       overpass_main_filter{ overpass_original_filter };
  //   std::string       overpass_pre_apply_filter_s{ overpass_main_filter };
  //
  //   // Oilrig
  //   //std::string oilrig_part_of_globe_label; // v25.08.1
  //   std::map <int, std::string> map_pick_oilrig_globe_part {
  //      { missionx::PICKED_GLOBE, " Globe " }
  //     ,{ missionx::PICKED_HALF_GLOBE, " Half Globe " }
  //     ,{ missionx::PICKED_QUARTER_GLOBE, " Quarter Globe " }
  //     ,{ missionx::PICKED_LOCAL_REGION_GLOBE, " Local Region " }
  //     ,{ missionx::PICKED_IN_MY_AREA, " In My Area " }
  //   };
  //   // Filter runway by type
  //   bool                                     flag_pick_any_rw{ true };
  //   std::map<const std::string, bool>        map_filter_runways                      = { { "Grass##filterRunways", false }, { "Dirt/Gravel##filterRunways", false }, { "Concrete/Asphalt##filterRunways", false }, { "water##filterRunways", false } };
  //   std::map<const std::string, std::string> map_filter_runways_translate_to_numbers = { { "Grass##filterRunways", "3" }, { "Dirt/Gravel##filterRunways", "4, 5" }, { "Concrete/Asphalt##filterRunways", "1, 2" }, { "water##filterRunways", "13" } };
  //
  //   void reset_filter_runways_flags ()
  //   {
  //     for (auto &rw_type : map_filter_runways)
  //       rw_type.second = false;
  //
  //     flag_pick_any_rw = true;
  //   }
  //
  //
  //   mx_user_create_mission_layer() = default;
  //
  // };
  // // define the instance
  // mx_user_create_mission_layer strct_user_create_layer;


  //// v3.0.253.11
  //typedef struct _cross_layer_property
  //{
  //  bool flag_start_from_plane_position{ false };
  //  bool flag_generate_gps_waypoints{ true };
  //  bool flag_add_route_waypoints{ false }; // v25.04.2
  //  bool flag_auto_load_route_to_gps_or_fms{ false }; // v25.04.2

  //} mx_cross_layer_property;
  //mx_cross_layer_property strct_cross_layer_properties;


  // Flight plan result string
  std::string asyncSecondMessageLine;
  // bool        flag_generatedRandomFile_success{ false }; // v26.04.3 moved to shared header shared_ui_types


  // MEMBERS //
  void setMessage (const std::string &inMsg, int secToDisplayMessage = 20);

  void clearMessage (); //

  void refreshNewMapsAndImages (missionx::Waypoint &inLeg);
  void initFlightLegChange ();

  void setPluginPausedSim (bool inValue) { missionClassPausedSim = inValue; }
  void setForcePauseSim (bool inValue) { this->forcePauseSim = inValue; }



  // // ----- ILS Layer -----
  // //// user mission creation variables
  // typedef struct _ils_layer
  // {
  //   bool flagNavigatedFromOtherLayer{ false }; // v24025
  //   bool flagForceNavDataTab{ false }; // v24.03.1
  //   bool flagIgnoreDistanceFilter{ false }; // v24.03.1
  //
  //   bool isIFR {true};
  //   bool isVFR {false};
  //
  //
  //   mx_layer_state_enum               layer_state{ missionx::mx_layer_state_enum::not_initialized };
  //   missionx::enums::mx_treeNodeState enum_elevSliderOpenState{ missionx::enums::mx_treeNodeState::closed }; // v24.03.1 converted the flag to enum
  //
  //   int sort_indx{ 0 }; // starts in 0
  //
  //   // from ILS icao parameters
  //   char                 buf1[10] = { "" };
  //   char                 buf2[10] = { "" };
  //   bool                 bFirstTime{ false };
  //   std::string          from_icao{ "" }; // we will fetch the closest icao location to plane location
  //   std::string          to_icao{ "" }; // v3.0.253.3 search specific destination
  //   missionx::NavAidInfo navaid;
  //
  //   int            iRadioTemplate_Med_or_Cargo{ 1 }; // template type should only be cargo
  //   mx_plane_types_enum iRadioPlaneType{ missionx::mx_plane_types_enum::plane_type_props };
  //   // sliders
  //   float       ils_sliderVal1  = (float)mxconst::SLIDER_SHORTEST_MIN_ILS_SEARCH_RADIUS; // min distance
  //   float       ils_sliderVal2  = (float)mxconst::SLIDER_SHORTEST_MAX_ILS_SEARCH_RADIUS; // lowest ILS search radius. 250nm in v3.0.253.6
  //   std::string ils_slider2_lbl = "[" + Utils::formatNumber<float> (ils_sliderVal1, 0) + ".." + Utils::formatNumber<float> (ils_sliderVal2, 0) + "]";
  //
  //   // ILS types
  //   std::string                                ils_types_tree_label; // empty means "any"
  //   std::map<missionx::mx_ils_type_enum, bool> mapCheck_ILS_types;
  //
  //   // RW Length Slider
  //   int         slider_min_rw_length_i{ mxconst::SLIDER_ILS_STARTING_RW_LENGTH_VALUE };
  //   std::string min_rw_length_label_s{ Utils::formatNumber<int> (slider_min_rw_length_i) };
  //
  //   // Minimal RW Width Slider
  //   int slider_min_rw_width_i{ mxconst::SLIDER_ILS_STARTING_RW_WIDTH_VALUE };
  //
  //   // Minimal Airport elevation Slider
  //   int slider_min_airport_elev_ft_i{ mxconst::SLIDER_ILS_STARTING_AIRPORT_ELEV_VALUE_FT };
  //
  //   int                          limit_indx{ 0 }; // v24.03.1 limit ILS rows fetched from DB
  //   constexpr const static char *limit_items[] = { "250", "500", "750", "1000", "1250", "1500", "2000" };
  //
  //
  //   // search ILS from database - progress
  //   std::string filter_query_s; // v24.03.1
  //   missionx::mxFetchState_enum fetch_ils_state{ missionx::mxFetchState_enum::fetch_not_started };
  //   missionx::mxFetchState_enum fetch_nav_state{ missionx::mxFetchState_enum::fetch_not_started };
  //   missionx::mxFetchState_enum fetch_metar_state{ missionx::mxFetchState_enum::fetch_not_started };
  //
  //   std::string asyncFetchMsg_s;
  //   std::string asyncNavFetchMsg_s;
  //   std::string asyncMetarFetchMsg_s;
  //
  //   // Plane location
  //   missionx::Point planePos;
  //
  //   // Nav Data // v24025
  //   std::string                                sNavICAO;
  //   std::unordered_map<int, mx_nav_data_strct> mapNavaidData; // airport data
  //
  //   // ---------------- Members -----------------
  //   _ils_layer () { init_mapChecks_ils_types (); }
  //
  //   void init_mapChecks_ils_types ()
  //   {
  //     mapCheck_ILS_types.clear ();
  //     for (const auto &keyType : missionx::mapILS_types | std::views::keys)
  //       mapCheck_ILS_types[keyType] = false; // reset to false
  //   }
  //
  //   std::string get_ils_types_picked () // will help to construct the types tree title and the SQL statement
  //   {
  //     std::string result_s;
  //     bool        foundAtLeastOne = false;
  //     auto        counter         = 0; // (imapSize > 0) ? 1 : imapSize; // counter follows which iterator we test. If we reached last one, we won't add "," to result_s
  //     auto        foundCounter_i  = 0;
  //     for (auto &v : mapCheck_ILS_types)
  //     {
  //       counter++;
  //       if (v.second)
  //       {
  //         foundCounter_i++;
  //         if (foundAtLeastOne) // add "," only if we have at least 1 value
  //           result_s += ",";
  //         // add label
  //         result_s += "'" + missionx::mapILS_types[v.first] + "'";
  //
  //         foundAtLeastOne = true;
  //       }
  //     }
  //
  //     if (foundCounter_i == counter)
  //       result_s.clear ();
  //
  //     return mxUtils::stringToLower (result_s);
  //   } // get_ils_types_picked
  //
  // } mx_ils_layer;
  //
  // // ILS screen layer struct
  // mx_ils_layer strct_ils_layer;

  // Setup Layer Struct
  //mx_setup_layer strct_setup_layer;

  void set_vecOverpassUrls_char (const std::vector<std::string> &inVecData);

  // ----- Pick Mission List Layer -----
  typedef struct _pick_layer
  {
    bool        bFinished_loading_mission_images{ false };
    std::string last_picked_key;

  } mx_pick_layer;
  mx_pick_layer strct_pick_layer;

  // ----- flight_leg_info Layer -----
  void setExternalInventoryName (const std::string &inName) { this->strct_flight_leg_info.externalInventoryName = inName; };

  typedef struct _flight_leg_info_layer
  {
    constexpr static int WAYPOINT_MAX_WIDTH_I = 100;

    // child layer
    missionx::uiLayer_enum internal_child_layer{ missionx::uiLayer_enum::flight_leg_info }; // holds the inner layers to display in 2D or VR mode. Example: map, inventory or choice layer

    // debug
    bool flagDebugTabIsOpen          = false; // v3.305.3
    bool flagFlightPlanningTabIsOpen = false; // v24.03.1

    // inventory 24.12.2 register item move to plane
    bool     flagItemMoveWasPressedFromExternalInv{ false };
    int      iSliderItemQuantity{ 0 };
    int      iOriginalQuantity{ 0 };
    IXMLNode ptrNodePicked = IXMLNode::emptyIXMLNode;

    void resetItemMove ()
    {
      iSliderItemQuantity                   = 0;
      iOriginalQuantity                     = 0;
      flagItemMoveWasPressedFromExternalInv = false;
      ptrNodePicked                         = IXMLNode::emptyIXMLNode;
    }

    bool setItemMoveFromExternal (const IXMLNode &inNode, const bool &inSetupXP11Compatibility)
    {
      if ((!inNode.isEmpty ()) && (!inSetupXP11Compatibility))
      {
        this->flagItemMoveWasPressedFromExternalInv = true;
        this->ptrNodePicked                         = inNode;
        iSliderItemQuantity                         = 1;
        iOriginalQuantity                           = Utils::readNodeNumericAttrib<int> (inNode, mxconst::get_ATTRIB_QUANTITY (), 0);

#ifndef RELEASE
        Log::log_to_missionx_log (Utils::xml_get_node_content_as_text (this->ptrNodePicked)); // debug
#endif
      }
      else if (inSetupXP11Compatibility)
      {
        this->resetItemMove ();
        return true; // Force item move for XP11 compatibility screen
      }
      else
      {
        this->resetItemMove ();
      }

      return this->flagItemMoveWasPressedFromExternalInv;
    }
    // END v24.12

    // map texture number to display
    int  iMapNumberToDisplay{ 0 }; // less than 1 means nothing to display
    void mapLayerInit ()
    {
      // totalMapsNumber = 0; // v3.303.13
      missionx::data_manager::strct_flight_leg_info_totalMapsCounter = 0;
      iMapNumberToDisplay                                            = 0; // Zero means no Map is set
    }
    // inventory weights
    float acf_m_max{ 0.0f }; // holds max weight plane can carry (payload + fuel)
    float acf_m_empty{ 0.0f }; // holds empty weight of the plane
    float m_fuel_total{ 0.0f }; // holds the fuel weight
    float cg_indicator_f{ 0.0f }; // holds the gauge center of gravity - meter
    float cg_z_prct_f{ 0.0f }; // holds the z center of gravity - %
    float calculated_plane_weight_f{ 0.0f }; // Calculate plane weight
    float outside_air_temp_degc{ 0.0f }; // v24.03.1 outside temp

    std::string plane_virtual_weight_fuel_and_payload; // current_plane_payload_weight_f + m_fuel_total.  holds the supposed weight according to item on plane
    std::string plane_max_weight_allowed; // holds formated value to string
    std::string formated_plane_inv_title_s; //
    std::string externalInventoryName;

    // v24.12.2
    typedef struct _mx_header_state
    {
      bool        bState{ false };
      std::string title{ "n/a" };

      _mx_header_state () {};
      _mx_header_state (std::string inVal_s, bool inBool)
      {
        bState = inBool;
        title  = inVal_s;
      }

      void setState (bool inState)
      {
        if (this->bState != inState)
        {
          this->bState ^= 1;

          if (bState)
            ImGui::SetScrollHereY ();
        }
      }
    } mx_header_state;
    std::map<int, mx_header_state> mapStationHeaders = {};
    // end v24.12.2



    ///// End Summary layer
    missionx::mxTextureFile     endTexture; // holds end texture to display
    std::string                 end_description{ "" };
    bool                        bStatsPressed{ false }; // v3.0.255.1
    bool                        bImagePressed{ false }; // v3.0.255.1
    missionx::mxFetchState_enum fetch_state{ missionx::mxFetchState_enum::fetch_not_started };
    std::string                 asyncFetchMsg_s{ "" };
    std::string                 last_msg_s{ "" };

    void resetChildLayer () { internal_child_layer = missionx::uiLayer_enum::flight_leg_info; }

    // v3.0.303.5 support inventory image click
    ImVec2 vec2_left_image_big{ ImVec2 (90.0f, 130.0f) }; // v3.0.303.5 will hold zoomed image size after deciding the max ratio based on its original size and the inventory child window size
    ImVec2 vec2_right_image_big{ ImVec2 (90.0f, 130.0f) }; // v3.0.303.5 will hold zoomed image size after deciding the max ratio based on its original size and the inventory child window size
    int    left_index_image_clicked{ -1 }; // -1 means non image zoom
    int    right_index_image_clicked{ -1 }; // -1 means non image zoom
    void   reset_inv_image_zoom () { left_index_image_clicked = right_index_image_clicked = -1; }


    // v3.305.1 Story mode
    typedef struct _mx_story_mode_strct
    {
      constexpr static float STORY_TEXT_AREA_HEIGHT_F = 70.0;

      constexpr static auto HISTORY_BUTTON_LABEL = "Toggle History [||]";

      constexpr static auto upperStoryMode_vec2 = ImVec2 (0.0f, 300.0f);
      constexpr static auto small_img_vec2      = ImVec2 (250.0f, 300.0f);
      constexpr static auto med_img_vec2        = ImVec2 (360.0f, 300.0f);
      constexpr static auto background_img_vec2 = ImVec2 (790.0f, 300.0f); // 870=exact overlap between left and right images. This gives us 10px in addition from left and right

      bool bPressedPause{ false };
      bool bPressedHistory{ false }; // v3.305.2
      bool bScrollToEndOfHistoryMessages{ true }; // v3.305.2 we will reset it everytime we press the history button

      size_t          textLength{ 0 };
      size_t          iChar{ 0 };
      size_t          prev_iChar{ 0 };
      missionx::Timer timerForTextProgression;
      missionx::Timer timerForAutoSkip;

      missionx::mx_character characterInfo;
      std::string            sTextToPrint;
      std::string            store_last_message_s;

      void reset ()
      {
        bPressedPause   = false;
        bPressedHistory = false;

        textLength = 0;
        iChar      = 0;
        prev_iChar = 0;
        timerForTextProgression.reset ();
        timerForAutoSkip.reset ();
        characterInfo = missionx::mx_character ();
        sTextToPrint.clear ();
        store_last_message_s.clear ();
      }

      void setAutoSkipTimer (float inNewTime) { missionx::Timer::start (timerForAutoSkip, inNewTime, "AutoSkip"); }

    } mx_story_mode_strct;
    mx_story_mode_strct strct_story_mode;

    constexpr const static size_t DEBUG_GLOBALS_BUFF_SIZE_RSIZET = 4096; // v3.305.3
    constexpr const static size_t DEBUG_BUFF_SIZE_RSIZET         = 10000; // v3.305.3
    char                          online_debug_buff[DEBUG_BUFF_SIZE_RSIZET]{ '\0' }; // v3.305.3
    char                          online_globals_buff[DEBUG_GLOBALS_BUFF_SIZE_RSIZET]{ '\0' }; // v3.305.3
    size_t                        online_debug_buff_size   = 0; // v3.305.3
    size_t                        online_globals_buff_size = 0; // v3.305.3
    std::string                   scriptNameToEdit; // v3.305.3

    constexpr static const int SHORT_FIELD_SIZE = 7;
    constexpr static const int LONG_FIELD_SIZE  = 500;

    std::map<missionx::enums::mx_note_shortField_enum, char[SHORT_FIELD_SIZE]> mapNoteFieldShort;
    std::map<missionx::enums::mx_note_longField_enum, char[LONG_FIELD_SIZE]>   mapNoteFieldLong;
    constexpr const static auto                                                iMaxCharsInLongField = sizeof mapNoteFieldLong[missionx::enums::mx_note_longField_enum::takeoff_notes];
    // v26.04.1
    missionx::enums::mx_note_longField_enum fpln_picked_note {missionx::enums::mx_note_longField_enum::takeoff_notes};

    mx_ext_internet_fpln_strct fpln; // v25.03.3

    missionx::Timer tmPressedClearU, tmPressedClearD, tmPressedClearAll;

    _flight_leg_info_layer()
    {
      initNoteMaps();
      this->resetItemMove(); // v24.12.2
    }

    void initNoteMaps()
    {
      for (auto enumI = missionx::enums::mx_note_shortField_enum::begin; enumI < missionx::enums::mx_note_shortField_enum::end; enumI = static_cast<missionx::enums::mx_note_shortField_enum>(static_cast<size_t>(enumI) + 1))
      {
        // #ifdef IBM
        // memcpy_s(mapNoteFieldShort[enumI], sizeof (mapNoteFieldShort[enumI]), "\0", sizeof ("\0"));
        // #else
        // memcpy(mapNoteFieldShort[enumI], "\0", sizeof ("\0"));
        // #endif

        // v26.04.3
        mxUtils::reset_buffer(mapNoteFieldShort[enumI][0], sizeof(mapNoteFieldShort[enumI]));
      }
      for (auto enumI = missionx::enums::mx_note_longField_enum::begin; enumI < missionx::enums::mx_note_longField_enum::end; enumI = static_cast<missionx::enums::mx_note_longField_enum>(static_cast<size_t>(enumI) + 1))
      {
        // #ifdef IBM
        // memcpy_s(mapNoteFieldLong[enumI], sizeof (mapNoteFieldLong[enumI]), "\0", sizeof ("\0"));
        // #else
        // memcpy(mapNoteFieldLong[enumI], "\0", sizeof ("\0"));
        // #endif

        // v26.04.3
        mxUtils::reset_buffer(mapNoteFieldLong[enumI][0], sizeof(mapNoteFieldLong[enumI]));

      }
    }

    void resetNotesUpperPart()
    {
      for (auto enumI = missionx::enums::mx_note_shortField_enum::begin; enumI < missionx::enums::mx_note_shortField_enum::end; enumI = static_cast<missionx::enums::mx_note_shortField_enum>(static_cast<size_t>(enumI) + 1))
      {
        // #ifdef IBM
        // memcpy_s(mapNoteFieldShort[enumI], sizeof (mapNoteFieldShort[enumI]), "\0", sizeof ("\0"));
        // #else
        // memcpy(mapNoteFieldShort[enumI], "\0", sizeof ("\0"));
        // #endif

        // v26.04.3
        mxUtils::reset_buffer(mapNoteFieldShort[enumI][0], sizeof(mapNoteFieldShort[enumI]));
      }

      // #ifdef IBM
      // memcpy_s(mapNoteFieldLong[missionx::enums::mx_note_longField_enum::waypoints], sizeof (mapNoteFieldLong[missionx::enums::mx_note_longField_enum::waypoints]), "\0", sizeof ("\0"));
      // memcpy_s(mapNoteFieldLong[missionx::enums::mx_note_longField_enum::taxi], sizeof (mapNoteFieldLong[missionx::enums::mx_note_longField_enum::taxi]), "\0", sizeof ("\0"));
      // #else
      // memcpy(mapNoteFieldLong[missionx::enums::mx_note_longField_enum::waypoints], "\0", sizeof ("\0"));
      // memcpy(mapNoteFieldLong[missionx::enums::mx_note_longField_enum::taxi], "\0", sizeof ("\0"));
      // #endif

      // v26.04.3
      mxUtils::reset_buffer(mapNoteFieldLong[missionx::enums::mx_note_longField_enum::waypoints][0], sizeof(mapNoteFieldLong[missionx::enums::mx_note_longField_enum::waypoints]));
      mxUtils::reset_buffer(mapNoteFieldLong[missionx::enums::mx_note_longField_enum::taxi][0], sizeof(mapNoteFieldLong[missionx::enums::mx_note_longField_enum::taxi]));


    }

    void resetNotesLowerPart()
    {
      for (auto enumI = missionx::enums::mx_note_longField_enum::begin; enumI < missionx::enums::mx_note_longField_enum::end; enumI = static_cast<missionx::enums::mx_note_longField_enum>(static_cast<size_t>(enumI) + 1))
      {
        if ((enumI == missionx::enums::mx_note_longField_enum::waypoints) || (enumI == missionx::enums::mx_note_longField_enum::taxi))
          continue;

        // #ifdef IBM
        // memcpy_s(mapNoteFieldLong[enumI], sizeof (mapNoteFieldLong[enumI]), "\0", sizeof ("\0"));
        // #else
        // memcpy(mapNoteFieldLong[enumI], "\0", sizeof ("\0"));
        // #endif

        // v26.04.3
        mxUtils::reset_buffer(mapNoteFieldLong[enumI][0], sizeof(mapNoteFieldLong[enumI]));

      }
    }

    void setNoteShortField(const missionx::enums::mx_note_shortField_enum inEnum, const std::string& inValue)
    {
      // #ifdef IBM
      // memcpy_s(mapNoteFieldShort[inEnum], SHORT_FIELD_SIZE - 1, inValue.c_str(), SHORT_FIELD_SIZE - 1);
      // #else
      // memcpy(mapNoteFieldShort[inEnum], inValue.c_str(), SHORT_FIELD_SIZE - 1);
      // #endif

      // v26.04.3
      mxUtils::copy_string_to_buffer(inValue, mapNoteFieldShort[inEnum][0], sizeof(mapNoteFieldShort[inEnum]));


    }

    void setNoteLongField(const missionx::enums::mx_note_longField_enum inEnum, const std::string& inValue)
    {
      // #ifdef IBM
      // memcpy_s(mapNoteFieldLong[inEnum], LONG_FIELD_SIZE - 1, inValue.c_str(), LONG_FIELD_SIZE - 1);
      // #else
      // memcpy(mapNoteFieldLong[inEnum], inValue.c_str(), LONG_FIELD_SIZE - 1);
      // #endif

      // v26.04.3
      mxUtils::copy_string_to_buffer(inValue, mapNoteFieldLong[inEnum][0], sizeof(mapNoteFieldLong[inEnum]));

    }

  } mx_flight_leg_info_layer;

  mx_flight_leg_info_layer strct_flight_leg_info;



  // ----- External Routes Layer -----
  typedef enum class _mx_ext_fpln_screen
    : uint8_t
  {
    ext_home = 0,
    ext_db_fpln,
    ext_simbrief
  } mx_ext_fpln_screen;

  // ----- External Routes Layer -----
  typedef struct _external_routes_layer
  {
    mx_ext_fpln_screen ext_screen{ mx_ext_fpln_screen::ext_home };

    bool flag_first_time{ true }; // v3.303.10
    bool flag_remove_duplicate_airport_names{ true };
    bool flag_group_by_waypoints{ false };
    bool flag_flightplandatabase_auth_exists{ false }; // v24.06.1 will hold if mxconst::get_SETUP_AUTHORIZATIOJN_KEY() node has a value.

    bool bDisplayPluginsRestart{ false };

    float       ga_range_max_slider_f{ 0.0f };
    int         limit_indx{ 1 };
    int         sort_indx{ 0 }; // starts in 0
    char        buf_from_icao[10]      = { '\0' };
    char        buf_to_icao[10]        = { '\0' };
    char        buf_authorization[256] = { '\0' }; // v3.303.8.3
    std::string from_icao; // we will fetch the closest icao location to plane location
    std::string to_icao; // v3.0.253.3 search specific destination

    missionx::mxFetchState_enum fetch_state{ mxFetchState_enum::fetch_not_started };
    missionx::mxFetchState_enum simbrief_fetch_state{ mxFetchState_enum::fetch_not_started }; // v25.03.3
    missionx::uiLayer_enum      simbrief_called_layer{ missionx::uiLayer_enum::uiLayerUnset }; // v25.03.3 from which layer we called Simbrief Thread

    std::string asyncFetchMsg_s; // will get info from the async process

    missionx::base_thread::strct_thread_state threadState;
  } mx_external_routes_layer;
  mx_external_routes_layer strct_ext_layer;



  //// ----- Custom Template Layer -----
  //typedef struct _generate_template_layer
  //{
  //  bool bFinished_loading_templates{ false };

  //  mx_layer_state_enum layer_state{ missionx::mx_layer_state_enum::not_initialized }; // v3.0.253.9

  //  int    user_pick_from_replaceOptions_combo_i{ mxconst::INT_UNDEFINED };
  //  ImVec2 vec2_replace_options_size{ 150.f, 20.0f }; // v3.0.255.4.1

  //  std::vector<const char *> vecReplaceOptions_char{}; // v3.0.255.4 will store pointers to the <replace_options> element from the TemplateInfo

  //  std::string last_picked_template_key;
  //  std::string selectedTemplateKey; // v25.09.2
  //} mx_generate_template_layer;
  //mx_generate_template_layer strct_generate_template_layer;


  missionx::uiLayer_enum getCurrentLayer () const { return this->currentLayer; }



protected:
  // tableDataListTy     tableList;
  bool missionClassPausedSim{ false };
  bool forcePauseSim{ false }; // force pause when designer ask auto pause after location transition

  // Main function: creates the window's UI
  void buildInterface () override;



private:
  const float TOP_BUTTON_SIZE        = 24.0f;
  const float MAIN_BUTTON_WIN_SIZE_W = 120.0f;
  const float MAIN_BUTTON_WIN_SIZE_H = 90.0f;

  const mx_property_type_as_string_code mxcode; // TODO: should we deprecate this type ?

  ImVec2 vec2_sizeTopBtn         = { TOP_BUTTON_SIZE, TOP_BUTTON_SIZE };
  ImVec2 vec2_size_homeLayer_btn = { MAIN_BUTTON_WIN_SIZE_W, MAIN_BUTTON_WIN_SIZE_H };
  ImVec2 uv0                     = { 0.0f, 0.0f };
  ImVec2 uv1                     = { 1.0f, 1.0f };


  typedef struct _btn_info
  {
    int          id{ 0 };
    uiLayer_enum layer;
    std::string  imgName;
    std::string  label;
    std::string  tip;
  } btn_info;




  //// Keep track of Layer change
  uiLayer_enum currentLayer{ missionx::uiLayer_enum::imgui_home_layer };
  uiLayer_enum prevBrieferLayer{ missionx::uiLayer_enum::imgui_home_layer };

  const std::list<btn_info> listMainBtn = { 
    { 0, missionx::uiLayer_enum::option_setup_layer, mxconst::get_BITMAP_BTN_SETUP_24X18 (), "Setup", "Setup Screen" }, 
    { 1, missionx::uiLayer_enum::option_user_generates_a_mission_layer, mxconst::get_BITMAP_BTN_LAB_24X18 (), "Create", "Generate random mission" }, 
    { 2, missionx::uiLayer_enum::option_generate_mission_from_a_template_layer, mxconst::get_BITMAP_BTN_PREPARE_MISSION_24X18 (), "Templates", "Generate a Mission based on predefined custom templates" }, 
    { 3, missionx::uiLayer_enum::option_mission_list, mxconst::get_BITMAP_LOAD_MISSION (), "Load Mission", "Load a mission and fly it" }, 
    { 4, missionx::uiLayer_enum::flight_leg_info, mxconst::get_BITMAP_BTN_FLY_MISSION_24X18 (), "Flight planning /\nFlight Progress", "Displays flight leg info when mission is active.\nPlan your Flight Plan (Can fetch data from Simbrief)" }, 
    { 5, missionx::uiLayer_enum::option_external_fpln_layer, mxconst::get_BITMAP_BTN_WORLD_PATH_24X18 (), "External FPLN", "Build mission based on external flight plan\nUsing: flightplandatabase.com and Simbrief" }, 
    { 6, missionx::uiLayer_enum::option_ils_layer, mxconst::get_BITMAP_BTN_NAV_24x18 (), "* VFR/ILS Approaches \n* NAV data", "Search for airports around you\nthat have ILS approaches\n or search for nav data." }, 
    { 7, missionx::uiLayer_enum::option_conv_fpln_to_mission, mxconst::get_BITMAP_BTN_CONVERT_FPLN_TO_MISSION_24X18 (), "Conv. FPLN", "Convert LittleNavMap FPLN to mission file." }

  };





  // typedef struct _radio_plane_type
  // {
  //   missionx::mx_plane_types_enum type {missionx::mx_plane_types_enum::plane_type_any};
  //   std::string              label;
  //
  //   float from_slider_min{ (float)mxconst::SLIDER_MIN_RND_DIST };
  //   float to_slider_min{ (float)mxconst::SLIDER_MAX_RND_DIST };
  //   float from_slider_max{ (float)(mxconst::SLIDER_MIN_RND_DIST * 1.2) };
  //   float to_slider_max{ (float)(mxconst::SLIDER_MAX_RND_DIST * 1.2) };
  //
  //   _radio_plane_type () = default;
  //
  //   _radio_plane_type (const missionx::mx_plane_types_enum inType, const std::string& inLabel, const float in_lower_slider_min, const float in_upper_slider_min, const float inMultiplyLowerMin = 1.2f, const float inMultiplyUpperMin = 1.2f)
  //   {
  //     type            = inType;
  //     label           = inLabel;
  //     from_slider_min = in_lower_slider_min;
  //     to_slider_min   = in_upper_slider_min;
  //     from_slider_max = static_cast<float>(static_cast<int>(from_slider_min * inMultiplyLowerMin));
  //     to_slider_max   = static_cast<float>(static_cast<int>(to_slider_min * inMultiplyUpperMin));
  //   }
  //
  // } radio_plane_type;


  void validate_sliders_values (missionx::mx_plane_types_enum inPlaneType);


  ////// v3.303.10 random calendar
  typedef struct _radio_calendar_dateTime_type_strct
  {
    missionx::mx_ui_random_date_time_type type{ missionx::mx_ui_random_date_time_type::xplane_day_and_time };
    std::string                           label;
    std::string                           toolTip;
  } radio_calender_dateTime_type;

  const std::list<radio_calender_dateTime_type> listRandomCalendarRadioLabel = { { missionx::mx_ui_random_date_time_type::xplane_day_and_time, "A", "Pick X-Plane day in year and the hour" }, { missionx::mx_ui_random_date_time_type::os_day_and_time, "B", "Operating System day in year and the hour." }, { missionx::mx_ui_random_date_time_type::any_day_time, "C", "Pick any day in the year.\nPick any hour between 06:00 and 19:00.\nYou can extend it to include night hours." }, { missionx::mx_ui_random_date_time_type::exact_day_and_time, "D", "Pick the exact day of year and hour you would like to fly in." }, { missionx::mx_ui_random_date_time_type::pick_months_and_part_of_preferred_day, "E", "Pick the months and hours you are interested to fly in.\nThe plugin will pick a day and hour in the time frame defined." } };

  ////// v3.303.12 random weather in advanced settings
  typedef struct _radio_weather_options_strct
  {
    missionx::mx_ui_random_weather_options type{ missionx::mx_ui_random_weather_options::use_xplane_weather };
    std::string                            label;
    std::string                            toolTip;
  } radio_weather_options_strct;

  const std::list<radio_weather_options_strct> listRandomWeatherRadioLabel = { { missionx::mx_ui_random_weather_options::use_xplane_weather, "Use X-Plane\nsettings", "Use X-Plane settings" }, { missionx::mx_ui_random_weather_options::use_xplane_weather_and_store, "Use X-Plane\nsettings (Save to file)", "Use X-Plane settings and also store in the mission file." }, { missionx::mx_ui_random_weather_options::pick_pre_defined, "Pick from a predefined\nweather sets.", "Pick from the pre-defined weather sets." } };


  // https://www.britannica.com/dictionary/eb/qa/parts-of-the-day-early-morning-late-morning-etc
  // 0 = midnight, 12 = noon. Values should be between 0..23
  typedef struct _mx_part_of_day
  {
    int start_hour{ 0 };
    int end_hour{ 0 };
    int span_time{ 0 };

    void operator= (const _mx_part_of_day &inVal) { clone (inVal); }

    static void clone (const _mx_part_of_day &inVal) { _mx_part_of_day (inVal.start_hour, inVal.end_hour); }

    _mx_part_of_day () = default;
    _mx_part_of_day (_mx_part_of_day const &) = default;

    _mx_part_of_day (int inStartHour, int inEndtHour)
    {
      if (inStartHour < 0 || (inStartHour > 23))
        inStartHour = 0;
      if (inEndtHour < 0 || (inEndtHour > 23))
        inEndtHour = 0;


      start_hour = inStartHour;
      end_hour   = inEndtHour;

      if (inStartHour > inEndtHour)
        span_time = static_cast<int>(fabs(missionx::HOURS_IN_A_DAY_24 + end_hour - start_hour));
      else
        span_time = static_cast<int>(fabs(end_hour - start_hour));
    }
  } mx_part_of_day;

  int                iMonthCode{ 0 };
  std::map<int, int> mapCalander_days_in_a_month = { { iMonthCode = 1, 31 }, { ++iMonthCode, 28 }, { ++iMonthCode, 31 }, { ++iMonthCode, 30 }, { ++iMonthCode, 31 }, { ++iMonthCode, 30 }, { ++iMonthCode, 31 }, { ++iMonthCode, 31 }, { ++iMonthCode, 30 }, { ++iMonthCode, 31 }, { ++iMonthCode, 30 }, { ++iMonthCode, 31 } };

  const std::map<std::string, int> mapCalander_Months = { { "Any", iMonthCode = 0 }, { "Jan", ++iMonthCode }, { "Feb", ++iMonthCode }, { "Mar", ++iMonthCode }, { "Apr", ++iMonthCode }, { "May", ++iMonthCode }, { "Jun", ++iMonthCode }, { "Jul", ++iMonthCode }, { "Aug", ++iMonthCode }, { "Sep", ++iMonthCode }, { "Oct", ++iMonthCode }, { "Nov", ++iMonthCode }, { "Dec", ++iMonthCode } };


  std::map<int, mx_part_of_day> mapCalander_parts_of_the_day = { { 1, mx_part_of_day (5, 8) }, { 2, mx_part_of_day (8, 10) }, { 3, mx_part_of_day (11, 12) }, { 4, mx_part_of_day (13, 15) }, { 5, mx_part_of_day (16, 17) }, { 6, mx_part_of_day (17, 19) }, { 7, mx_part_of_day (19, 21) }, { 8, mx_part_of_day (21, 4) } };



  // bottom window message line
  const float  FOOTER_REGION_CLEARENCE_PX{ 110.0f };
  const ImVec2 IMAGE_IN_FLIGHT_INFO = ImVec2{ 290.0f, 420.0f };

  //std::string     sBottomMessage{ "> " };
  missionx::Timer timerMessage;

  ///// DEBUG SLIDER HELPER ////
  float fDebugSlider{ 0.0f };
  float fDebugSliderWidth = 400.0f;

  float       xyHelper{ static_cast<float>(missionx::WinImguiBriefer::WINDOW_MAX_WIDTH) }; // used to assist in debug region heights
  const float fdebugMin[2]    = { 200.0f };
  const float fdebugMax[2]    = { 400.0f };
  float       fDebugInitValue = 250.0f;

  ///// END DEBUG SLIDER HELPER ////


  const float fRightPadLight = 12.0f;
  const float fTopToolbarPadding_f{ 40.0f };
  const float fBottomToolbarPadding_f{ 30.0f };
  const float PAD_BETWEEN_CHILD_REGIONS{ 20.0f };
  ImVec2      imvec2_top_toolbar_size            = ImVec2{ 0.0f, 35.0f };
  ImVec2      imvec2_pick_template_top_area_size = ImVec2{ static_cast<float>(this->WINDOW_MAX_WIDTH), 25.0f };
  ImVec2      imvec2_flight_info_top_area_size   = ImVec2{ static_cast<float>(this->WINDOW_MAX_WIDTH), 148.0f };
  ImVec2      imvec2_main_area_size              = ImVec2 (0.0f, 320.0f);
  ImVec2      imvec2_main_pick_mission_area_size = ImVec2 (0.0f, 350.0f);

  void draw_top_toolbar (); // were we place the HOME button and layer name
  void popup_draw_load_warnings (std::string_view inPopupWindowName); // v26.1.1
  void popup_draw_quit_mission (std::string_view inPopupWindowName); // v3.303.8.3
  void draw_popup_extra_data_ext_fpln (std::string_view inPopupWindowName ); // v25.06.1
  void draw_popup_generate_mission_based_on_ext_fpln (std::string_view inPopupWindowName, const missionx::mx_ext_internet_fpln_strct &rowData, const int &picked_fpln_id_i = 0); // v25.03.3
  void draw_globals_online_edit_popup (std::string_view inPopupWindowName, char inType, std::string inKey, std::string inVal); // v3.303.8.3
  void draw_script_online_edit_popup (std::string_view inPopupWindowName, bool &outSave); // v3.303.8.3
  void draw_setup_layer ();
  void draw_home_layer ();
  void draw_dynamic_mission_creation_screen ();
  void draw_dynamic_mission_creation_screen_home ();
  void draw_dynamic_mission_creation_screen_child_1 ();
  void draw_dynamic_mission_creation_screen_child_2 ();
  void draw_template_mission_generator_screen ();
  void draw_flight_leg_info ();
  void child_draw_2D_and_VR_flight_leg_info_mxpad_and_choices_with_tab (); //
  void child_draw_STORY_mode_leg_info (); // mainly draw flight leg description
  void child_flight_leg_info_draw_inventory (); // v24.12.2
  void child_draw_inv_plane_xp12_move_item (Inventory &inout_copied_plane_inventory, const missionx::mx_ui_inv_regions &inRegionType, const ImVec2 &in_vec2_inv_child); // v24.12.2 display the transaction part, above the plane station.
  void child_draw_inv_plane_xp12 (Inventory &inoutPlaneInventory, const missionx::mx_ui_inv_regions &inRegionType, const ImVec2 &in_vec2_inv_child); // v24.12.2
  void child_draw_inv_plane_xp11 (const missionx::mx_ui_inv_regions &inRegionType, const ImVec2 &in_vec2_inv_child); // v24.12.2
  void child_draw_inv_external_store (const ImVec2 &in_vec2_inv_child); // v24.12.2
  void child_flight_leg_info_draw_map (); // it is not just map, can be any informational image
  void child_flight_leg_info_draw_end_summary (); // End mission
  void draw_load_existing_mission_screen ();
  void draw_external_fpln_screen ();
  void draw_child_ext_fpln_home_screen ();
  void draw_child_ext_fpln_db_site_screen ();
  //void draw_ils_screen (); // v3.0.253.6
  // void child_draw_ils_search (); // v25.08.1
  // void child_draw_nav_search (); // v24.02.5
  void draw_about_layer ();
  void add_landing_rate_ui (const missionx::mx_enrout_stats_strct &inStats); // v3.303.14  landing rate performance row
  void add_ui_stats_child (bool isEmbedded = false); // v3.303.14  isEmbedded means that we don't want the BeginChild definition inside the function we will use and external BeginChild

  // const std::string LBL_START_MISSION              = ">> Start Mission <<";
  // const std::string LBL_LOAD_WARNINGS              = "!! Show Warnings !!"; // v26.1.1
  // const std::string LBL_ABORT_THREAD_LABEL         = "!! Abort !!";
  // const std::string FPLN_MORE_DETAILS              = "More Flight Plan Details";
  // const std::string GENERATE_QUESTION              = "Generate Mission From Flight Plan";
  // const std::string GENERATE_ILS_QUESTION          = "Generate Mission From ILS data"; // v3.0.253.6
  // const std::string GENERATE_TEMPLATE_QUESTION     = "Generate Mission From Template data"; // v25.06.1


#ifndef RELEASE
  bool add_ui_test_button (const missionx::mx_flc_pre_command inCommand, const std::string &label, const std::string &tip = "") const
  {
    if (ImGui::SmallButton (label.c_str ()))
    {
      missionx::data_manager::queFlcActions.push (inCommand);
      return true;
    }
    if (!tip.empty ())
      this->mx_add_tooltip (missionx::color::color_vec4_white, tip);


    return true;
  } // add_ui_test_button

#endif // RELEASE

  void add_ui_advance_settings_random_date_time_weather_and_weight_button (int &out_iClockDayOfYearPicked, int &out_iClockHourPicked, int &out_iClockMinutesPicked, const std::string &inTEXT_TYPE = mxconst::get_TEXT_TYPE_TITLE_REG ());
  bool add_ui_checkbox_rerun_random_date_and_time ();


  void display_shared_message_when_optimized_data_is_not_present (missionx::mx_layer_state_enum in_state);

  void add_missing_3d_files_message ();

  //void subDraw_ui_xRadius (IXMLNode &node, int pad_x_i = 0); // display the radius widget. Used for triggers based rad
  //void subDraw_ui_xPolyBox (IXMLNode &pNode, mx_trig_strct_ &inTrig_ptr); // display the poligonal box widget. Used for triggers based box/poly. We have 2 types, bottom left and center based
  //void subDraw_ui_xTrigger_elev (mx_trig_strct_ &inTrig_ptr, IXMLNode &node, bool inResetPick = false); // display the elevation options
  //void subDraw_ui_xScriptlet (IXMLNode &pNode, mxTrig_ui_mode_enm &inMode, mx_trig_strct_ *inTrig_ptr, const std::string inScriptInputLabel = "", missionx::mx_local_fpln_strct *inLegData = nullptr,
                              //char *inOutBuff = nullptr); // display a multiline input text for <scriptlet> element. Add it to the pNode once clicking the [apply] button. Returns the name of the scriptlet.
  // xLinkToNode can be an empty node. We will use it to link to a flight leg
  // void subDraw_ui_xTrigger_main (missionx::mx_local_fpln_strct &inLegData, bool &inNeedRefresh_b, int inLegIndex, std::map<int, missionx::mx_trig_strct_> &inMapOfGlobalTriggers,
  //                                std::vector<std::string> &inVecGlobalTriggers_names); // The function should receive the parent of all <trigger> elements. The ifnormation that will be displayed will be added to it.
  //void subDraw_ui_xTrigger_detail (mx_trig_strct_ &inTrig_ptr, bool &in_out_needRefresh_b, std::string &suggested_name, missionx::mx_local_fpln_strct &inLegData);

  // std::map<int, missionx::mx_local_fpln_strct> read_and_parse_saved_state (const std::string inPathAndFile); // v3.0.303.4 Read stored conversion state



  // const char *clockHours_arr[24]   = { "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23" }; // v3.303.8
  // const char *clockMinutes_arr[12] = { "00", "05", "10", "15", "20", "25", "30", "35", "40", "45", "50", "55" }; // v3.303.8
  // const char *clockDayOfYear_arr[365]{ "0 - Jan", "1",   "2",   "3",   "4",   "5",   "6",   "7",   "8",   "9",   "10",  "11",  "12",  "13",  "14",  "15",  "16",  "17",  "18",  "19",  "20",  "21",  "22",  "23",  "24",  "25",  "26",  "27",  "28",  "29",        "30",  "31 - Feb", "32",  "33",  "34",  "35",  "36",  "37",  "38",  "39",  "40",  "41",  "42",  "43",  "44",  "45",  "46",  "47",  "48",  "49",  "50",  "51",  "52",  "53",  "54",  "55",  "56",  "57",  "58",  "59 - Mar", "60",        "61",  "62",  "63",  "64",  "65",  "66",  "67",  "68",  "69",  "70",  "71",  "72",  "73",  "74",  "75",  "76",  "77",  "78",  "79",  "80",  "81",  "82",  "83",  "84",  "85",  "86",  "87",  "88",  "89",  "90 - Apr",  "91",  "92",  "93",  "94",  "95",  "96",  "97",  "98",  "99",  "100", "101", "102", "103", "104", "105", "106", "107", "108", "109", "110", "111", "112", "113", "114", "115", "116", "117", "118", "119", "120 - May", "121",       "122", "123", "124", "125", "126", "127", "128", "129", "130", "131", "132", "133", "134", "135", "136", "137", "138", "139", "140", "141", "142", "143", "144", "145", "146", "147", "148", "149", "150", "151 - Jun", "152", "153", "154", "155", "156", "157", "158", "159", "160", "161", "162", "163", "164", "165", "166", "167", "168", "169", "170", "171", "172", "173", "174", "175", "176", "177", "178", "179", "180", "181 - Jul", "182",
  //                                      "183",     "184", "185", "186", "187", "188", "189", "190", "191", "192", "193", "194", "195", "196", "197", "198", "199", "200", "201", "202", "203", "204", "205", "206", "207", "208", "209", "210", "211", "212 - Aug", "213", "214",      "215", "216", "217", "218", "219", "220", "221", "222", "223", "224", "225", "226", "227", "228", "229", "230", "231", "232", "233", "234", "235", "236", "237", "238", "239", "240", "241", "242",      "243 - Sep", "244", "245", "246", "247", "248", "249", "250", "251", "252", "253", "254", "255", "256", "257", "258", "259", "260", "261", "262", "263", "264", "265", "266", "267", "268", "269", "270", "271", "272", "273 - Oct", "274", "275", "276", "277", "278", "279", "280", "281", "282", "283", "284", "285", "286", "287", "288", "289", "290", "291", "292", "293", "294", "295", "296", "297", "298", "299", "300", "301", "302", "303",       "304 - Nov", "305", "306", "30",  "308", "309", "310", "311", "312", "313", "314", "315", "316", "317", "318", "319", "320", "321", "322", "323", "324", "325", "326", "327", "328", "329", "330", "331", "332", "333", "334 - Dec", "335", "336", "337", "338", "339", "340", "341", "342", "343", "344", "345", "346", "347", "348", "349", "350", "351", "352", "353", "354", "355", "356", "357", "358", "359", "360", "361", "362", "363", "364" };


  // v3.303.14 sub categories for missions
  // std::vector <const char*> medevac_arr = { "Any Location - Rescue", "Accident (OSM)" };
  // std::vector <const char*> oilrig_arr  = { "Oil Rig Cargo", "Medevac" };   //, "Personnel" };
  // std::vector <const char*> cargo_arr   = { "GA Cargo", "Farming Cargo", "Isolated Areas" };// , "Heavy Cargo" };
  std::vector<const char *> cargo_arr_copy; // will hold the copy of the original cargo_arr
  std::vector<std::string>  vecExternalCategories; // v24.05.1

  //std::unordered_map<int, std::vector<const char *>> mapMissionCategories = {
  //  { static_cast<int> (missionx::mx_ui_mission_type::medevac), data_manager::strct_ui_share_data.medevac_arr },
  //  { static_cast<int> (missionx::mx_ui_mission_type::oil_rig), data_manager::strct_ui_share_data.oilrig_arr },
  //  { static_cast<int> (missionx::mx_ui_mission_type::cargo), data_manager::strct_ui_share_data.cargo_arr }
  //};



  // ADVANCED SETTINGS Struct
  //mx_popup_adv_settings_strct adv_settings_strct;


  // v3.303.10
  void generate_mission_date_based_on_user_preference (int &out_iClockDayOfYearPicked, int &out_iClockHourPicked, int &out_iClockMinutesPicked, const bool &inIncludeNightHours);
  // v3.303.14
  // void addAdvancedSettingsPropertiesBeforeGeneratingRandomMission ();

  void refresh_slider_data_based_on_plane_type (missionx::mx_plane_types_enum inPlaneType); // split the code so it will be simpler to call it from different logic locations.

  // v3.305.3
  void                print_tasks_ui_debug_info(missionx::Objective& inObj); // v3.305.2
  void                print_triggers_ui_debug_info(); // v3.305.3 moved to briefer class
  void                print_datarefs_ui_debug_info(); // v3.305.3 moved to briefer class
  void                print_globals_ui_debug_info(); // v3.305.3
  void                print_scripts_ui_debug_info(); // v3.305.3
  static void         print_interpolated_ui_debug_info(); // v3.305.3
  void                print_messages_ui_debug_info(); // v3.305.4
  static void         add_ui_skip_abort_setup_checkbox(); // v3.305.3
  static void         add_designer_mode_checkbox(); // v24.3.2
  void                add_ui_xp11_comp_checkbox(const bool& inStorePreference); // v24.12.2
  void                add_ui_simbrief_pilot_id(); // v25.03.3
  void                add_ui_flightplandb_key(bool isPopup); // v25.03.3
  void                add_ui_pick_subcategories(const std::vector<const char*>& vecToDisplay); // v25.04.1
  void                add_ui_auto_load_checkbox(const missionx::mx_window_actions& inActionToExecute = missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS); // v25.04.2
  //int                 add_ui_two_option_buttons(bool& bOptA, bool& bOptB, const int& returnValueForA, const int& returnValueForB);
  int                 add_ui_dynamic_options_buttons(const int& inout_picked_lbl, std::map<int, std::string>& map_lbl_and_values);
  static void         add_ui_os_and_xp_clock_times(const float& in_x_pos);
  static void         add_ui_fps();
  //void                callNavData(std::string_view inICAO, bool bNavigatingFromOtherLayer); // v24.03.1
  void                add_ui_semi_act_phase_1_pick (); // v26.04.1
  void                add_ui_semi_act_phase_2_detail (); // v26.04.1
  static bool         add_ui_pick_how_many_legs ( int & inout_radio_value_ref, const std::string & in_label, const int & in_minButtons, const int & in_maxButtons); // v26.04.1
  void                add_ui_oilrig_search_area_buttons ( ); // v26.04.1
  void                add_ui_medevac_surprise_me_warning ( ); // v26.04.1
  const dataref_const dc;
};


}
#endif // WINIMGUIBRIEFER_H_
