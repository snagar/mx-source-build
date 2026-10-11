//
// Created by saar.nagar on 2026-10-08.
//

#ifndef MISSIONX_UI_DYNAMIC_MISSION_SCREEN_H
#define MISSIONX_UI_DYNAMIC_MISSION_SCREEN_H

#include "../core/mx_img_window.h"
#include "shared_ui_types.hpp"
#include "../../io/system_actions.h"
#include "../../core/data_manager.h"

namespace missionx {

// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------

class ui_dynamic_mission_screen : public mx_img_window {

protected:
  void buildInterface() override {};

private:
  // I am window number...
  const int myWinNum;

  const mx_property_type_as_string_code mxcode;

public:
  ui_dynamic_mission_screen(const int left, const int top, const int right, const int bot, const XPLMWindowDecoration decoration, const XPLMWindowLayer layer
              , const int &in_win_num
              )
              : mx_img_window (left, top, right, bot, decoration, layer)
                , myWinNum(in_win_num)
  {};
  ~ui_dynamic_mission_screen();

  // ----- Pointers to parent functions -----
  std::function<void(std::string, int)> set_bottom_message_line1;
  std::function<void()> add_designer_mode_checkbox;
  std::function<bool()> add_ui_checkbox_rerun_random_date_and_time;
  std::function<void(const std::string&)> add_ui_advance_settings_random_date_time_weather_and_weight_button; // v26.08.1
  // execAction (mx_window_actions actionCommand)
  std::function<void(missionx::mx_window_actions)> execAction;
  // void  add_message_text ();
  std::function<void()> add_message_text;
  // display message when airport database is not available
  std::function<void(missionx::mx_layer_state_enum)> display_shared_message_when_optimized_data_is_not_present;
  // display ui [start] button
  std::function<void(missionx::mx_window_actions)> add_ui_start_mission_button;
  // add ui autoload checkbox
  std::function<void(missionx::mx_window_actions)> add_ui_auto_load_checkbox;
  // add ui autoload checkbox
  std::function<void(const std::vector<const char *>)> add_ui_pick_subcategories;
  // add ui is amphibian checkbox
  std::function<void()> add_ui_is_amphibian;
  // v26.09.2 add "collapsing header" last mission description
  std::function<void(const bool)> add_briefer_description_post_mission_creation;
  // v26.09.2 add ui LLM options
  std::function<void(const enums::llm_ui_options_enum)> add_ui_llm_options;
  // v26.09.3 add abort button. Default action is: "mx_window_actions::ACTION_ABORT_RANDOM_ENGINE_RUN"
  std::function<void(const missionx::mx_window_actions inActionToExecute)> add_ui_abort_mission_creation_button;
  // v26.09.3 add add_ui_no_inventories_checkbox
  std::function<void(ImVec4 inColor)> add_ui_no_inventories_checkbox;
  // v26.09.3  draw_popup_generate_mission_based_on_ext_fpln
  std::function<void(std::string_view inPopupWindowName, const missionx::mx_ext_internet_fpln_strct &rowData, const int &picked_fpln_id_i)> draw_popup_generate_mission_based_on_ext_fpln;
  // v26.09.3  call_nav_screen_callNavData
  std::function<void(std::string_view inICAO, bool bNavigatingFromOtherLayer)> call_nav_screen_callNavData;
  // v26.09.3  add_ui_pause_in_2d_mode
  std::function<void()> add_ui_pause_in_2d_mode;
// v26.09.3  add_ui_expose_all_gps_waypoints
  std::function<void()> add_ui_expose_all_gps_waypoints;
// v26.09.3  add_ui_xp11_comp_checkbox
  std::function<void(const bool &inStorePreference)> add_ui_xp11_comp_checkbox;
  // v26.09.3  add_ui_suppress_distance_messages_checkbox_ui
  std::function<void(missionx::mx_window_actions inActionToExecute)> add_ui_suppress_distance_messages_checkbox_ui;
  // v26.09.3  add_ui_simbrief_pilot_id
  std::function<void()> add_ui_simbrief_pilot_id;
// v26.09.3  add_ui_flightplandb_key
  std::function<void(bool isPopup)> add_ui_flightplandb_key;
  // v26.09.3  add_ui_ai_use_ai_checkbox
  std::function<void()> add_ui_ai_use_ai_checkbox;
  // v26.09.3  add_ui_ai_server_url
  std::function<void()> add_ui_ai_server_url;
  // v26.09.3  add_ui_skewed_marker_checkbox
  std::function<void()> add_ui_skewed_marker_checkbox;
  // v26.09.3  add_ui_designer_mode_checkbox
  std::function<void()> add_ui_designer_mode_checkbox;
  // v26.09.3  add_ui_skip_abort_setup_checkbox
  std::function<void()> add_ui_skip_abort_setup_checkbox;
  // v26.09.3  add_ui_medevac_surprise_me_warning
  std::function<void()> add_ui_medevac_surprise_me_warning;
  // v26.09.3  add_ui_mission_description
  std::function<void(const std::string& in_descriptions)> add_ui_mission_description;
  // v26.09.3  add_ui_oilrig_search_area_buttons
  std::function<void()> add_ui_oilrig_search_area_buttons;
  // v26.09.3  add_ui_is_ga_cross_country_checkbox
  std::function<void()> add_ui_is_ga_cross_country_checkbox;
  // add_ui_pick_how_many_legs
  std::function<bool(int & inout_radio_value_ref, const std::string & in_label, const int & in_minButtons, const int & in_maxButtons)> add_ui_pick_how_many_legs;
  // add_other_settings_header
  std::function<void(bool in_plane_is_helo, bool bPickedMedevacMission, bool bPickedOilRigMission)> add_other_settings_header;
  // v26.09.3  add_ui_generate_button
  std::function<bool()> add_ui_generate_button;
  // v26.09.3  action_prepare_dynamic_mission_properties_and_call_generate_action
  std::function<void(const float &in_distance_min, const float & in_distance_max, const bool & in_add_start_from_plane_position)> action_prepare_dynamic_mission_properties_and_call_generate_action;
  // v26.09.3  add_ui_warning_messages_button
  std::function<void()> add_ui_warning_messages_button;
  // v26.09.3  gather_semi_act_phase1_data_based_on_picked_activity
  std::function<void(mx_user_create_mission_layer &in_usr_layer_strct, bool in_is_first_time)> gather_semi_act_phase1_data_based_on_picked_activity;
  // v26.09.3  gather_semi_act_phase1_data_based_on_picked_activity
  std::function<void(missionx::mx_plane_types_enum inPlaneType)> refresh_slider_data_based_on_plane_type;
  // v26.09.3  gather_semi_act_phase1_data_based_on_picked_activity
  std::function<void(missionx::mx_plane_types_enum inPlaneType)> validate_sliders_values;

  // -----------------------------------
  // Members
  // -----------------------------------
  void                add_ui_semi_act_phase_1_pick (); // v26.04.1
  void                add_ui_semi_act_phase_2_detail (); // v26.04.1

  void draw_dynamic_mission_creation_screen_child_1 ();
  void draw_dynamic_mission_creation_screen_child_2 ();

  void draw_dynamic_mission_creation_screen();

  void draw_dynamic_mission_creation_screen_home ();

};

} // missionx

#endif //MISSIONX_UI_DYNAMIC_MISSION_SCREEN_H
