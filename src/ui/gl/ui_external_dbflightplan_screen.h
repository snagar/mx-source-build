//
// Created by saar.nagar on 2026-10-07.
//

#ifndef MISSIONX_UI_EXTERNAL_DBFLIGHTPLAN_SCREEN_H
#define MISSIONX_UI_EXTERNAL_DBFLIGHTPLAN_SCREEN_H
#include <functional>
#include <string>

#include "shared_ui_types.hpp"
#include "../core/mx_img_window.h"
#include "../../core/data_manager.h"

// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------

namespace missionx {

class ui_external_dbflightplan_screen : public mx_img_window {
protected:
  void buildInterface() override {};

private:
  // I am window number...
  const int myWinNum;

public:
  ui_external_dbflightplan_screen(const int left, const int top, const int right, const int bot, const XPLMWindowDecoration decoration, const XPLMWindowLayer layer
              , const int &in_win_num
              )
              : mx_img_window (left, top, right, bot, decoration, layer)
                , myWinNum(in_win_num)
  {};
  ~ui_external_dbflightplan_screen();

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
  // v26.09.3 add popup: draw_popup_generate_mission_based_on_ext_fpln
  std::function<void(std::string_view inPopupWindowName, const missionx::mx_ext_internet_fpln_strct &rowData, const int &picked_fpln_id_i)> draw_popup_generate_mission_based_on_ext_fpln;
  // v26.09.3 add popup: draw_popup_generate_mission_based_on_ext_fpln
  std::function<void(std::string_view inICAO, bool bNavigatingFromOtherLayer)> call_nav_screen_callNavData;


  // -----------------------------------
  // Members
  // -----------------------------------
  void draw_child_ext_fpln_db_site_screen();

};

} // missionx

#endif //MISSIONX_UI_EXTERNAL_DBFLIGHTPLAN_SCREEN_H
