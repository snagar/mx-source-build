//
// Created by saar.nagar on 4/16/2026.
//

#ifndef MISSIONX_UI_NAV_SCREEN_H
#define MISSIONX_UI_NAV_SCREEN_H

#include "shared_ui_types.hpp"
#include <ImgWindow/ImgWindow.h> // inside libs/imgui4xp

namespace missionx {


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

class ui_nav_screen : public ImgWindow {
protected:
  void buildInterface() override {};

private:
  // I am window number...
  const int myWinNum;

  // mx_setup_layer*              strct_setup_layer;
  // mx_popup_adv_settings_strct* adv_settings_strct;

public:
  // --------- Constructors ----------
  ui_nav_screen(const int left, const int top, const int right, const int bot, const XPLMWindowDecoration decoration, const XPLMWindowLayer layer
              , const int &in_win_num
              )
              : ImgWindow (left, top, right, bot, decoration, layer)
                , myWinNum(in_win_num)
                {};


  // ----- virtual functions ----------------
  void flc() override {};

  // ----- Pointers to parent functions -----

  std::function<void(std::string, int)> set_bottom_message_line1;
  std::function<void()> add_designer_mode_checkbox;
  std::function<bool()> add_ui_checkbox_rerun_random_date_and_time;
  // void add_ui_advance_settings_random_date_time_weather_and_weight_button (int &out_iClockDayOfYearPicked, int &out_iClockHourPicked, int &out_iClockMinutesPicked, const std::string &inTEXT_TYPE = mxconst::get_TEXT_TYPE_TITLE_REG ());
  std::function<void(int&, int&, int&, const std::string&)> add_ui_advance_settings_random_date_time_weather_and_weight_button;
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


  // -----------------------------------
  // Members
  // -----------------------------------
  void draw_ils_screen (); // v3.0.253.6
  void child_draw_ils_search (); // v25.08.1
  void child_draw_nav_search (); // v24.02.5

  // ui members
  int add_ui_two_option_buttons (bool &bOptA, bool &bOptB, const int &returnValueForA, const int &returnValueForB);
  void add_ui_ils_vfr_search_airports_button (missionx::mx_window_actions inActionToExecute);
  void callNavData (std::string_view inICAO, bool bNavigatingFromOtherLayer); // v24.03.1






};

} // missionx

#endif //MISSIONX_UI_NAV_SCREEN_H
