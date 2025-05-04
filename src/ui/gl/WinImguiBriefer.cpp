#include "WinImguiBriefer.h"
#include "../../core/QueueMessageManager.h"
#include "../../io/ListDir.h"
#include "../../random/RandomEngine.h"

#include "../../io/OptimizeAptDat.h"

// Image processing (for reading "imgui_demo.jpg"
#define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"

//
// MARK: Global data and functions
//

// The raw TTF data of OpenFontIcons has been generated into the following file
#include "fa-solid-900.inc"

namespace missionx
{
constexpr const int WinImguiBriefer::WINDOW_MAX_WIDTH   = 800; // 900; // 800
constexpr const int WinImguiBriefer::WINDOWS_MAX_HEIGHT = 460; // 500; // 450

#ifndef RELEASE
int   gIntHelper   = 100;  // used with our slider helper
float gFloatHelper = 0.0f;
#endif
//////////////////////// Global Namespace variables //////////////////////////
int missionx::WinImguiBriefer::num_win;
//////////////////////// END Global Namespace variables //////////////////////////
}

//////////////////////// CLASS Members  //////////////////////////
namespace missionx
{
std::string err;
// ------------ contractor --------------
WinImguiBriefer::WinImguiBriefer (const int left, const int top, const int right, const int bot, const XPLMWindowDecoration decoration, const XPLMWindowLayer layer)
  : ImgWindow(left, top, right, bot, decoration, layer)
  , myWinNum(++num_win) // assign a unique window number
{
  // Disable reading/writing of "imgui.ini"
  ImGuiIO& io    = ImGui::GetIO();
  io.IniFilename = nullptr;

  // We take the parameter combination "SelfDecorateResizeable" + "LayerFlightOverlay"
  // to mean: simulate HUD
  if (decoration == xplm_WindowDecorationSelfDecoratedResizable && (layer == xplm_WindowLayerFlightOverlay || layer == xplm_WindowLayerFloatingWindows))
  {
    // let's set a fairly transparent, barely visible background
    ImGuiStyle& style               = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0, 150);
    // There's no window decoration, so to move the window we need to
    // activate a "drag area", here a small strip (roughly double text height)
    // at the top of the window, i.e. the window can be moved by
    // dragging a spot near the window's top
    SetWindowDragArea(0, 5, INT_MAX, 5 + 2 * static_cast<int> (data_manager::FONT_SIZE));
  }

  // Define our own window title
  #ifdef RELEASE
  //SetWindowTitle(fmt::format("Mission-X ({})", __DATE__));
  SetWindowTitle(fmt::format("Mission-X (v{}.{})", PLUGIN_VER_MAJOR, PLUGIN_VER_MINOR));
  #else
  SetWindowTitle(std::string("Mission-X v") + missionx::FULL_VERSION);
  #endif // !RELEASE

  
  SetWindowResizingLimits((int)(missionx::WinImguiBriefer::WINDOW_MAX_WIDTH / 2), (int)(missionx::WinImguiBriefer::WINDOWS_MAX_HEIGHT / 2), WinImguiBriefer::WINDOW_MAX_WIDTH, this->WINDOWS_MAX_HEIGHT); // minW. minH. maxW, maxH
  ImgWindow::SetVisible (false);

  this->mWindow = this->GetWindowId();

  //this->strct_user_create_layer.validate_sliders_values();
  this->validate_sliders_values(missionx::mx_plane_types::plane_type_helos); // v24.12.1 validate based on helos distances constraints

  fDebugSlider = this->fDebugInitValue;


  this->strct_setup_layer.bDisplayTargetMarkers = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS(), true); // v3.0.254.10 one time initialization
  this->strct_setup_layer.bGPSImmediateExposure = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE(), true);
  this->strct_setup_layer.bPauseIn2D            = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_PAUSE_IN_2D(), mxconst::DEFAULT_AUTO_PAUSE_IN_2D); // v3.0.253.9.1
  this->strct_setup_layer.bPauseInVR            = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_PAUSE_IN_VR(), mxconst::DEFAULT_AUTO_PAUSE_IN_VR); // v3.0.253.9.1
  this->strct_setup_layer.bCycleLogFiles        = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_OPT_CYCLE_LOG_FILES(), true); // v25.03.1
  // We will force XP11 layout if X-Plane is v11.x, else, we will read from the properties file.
  missionx::data_manager::flag_setupUseXP11InventoryUI  = (missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO)? true : Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::SETUP_USE_XP11_INV_LAYOUT, (missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO)); // v24.12.2

  // v3.0.255.2
  this->strct_setup_layer.bOverideCustomExternalFPLN_folders = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_OPT_WRITE_CONVERTED_FPLN_TO_XPLANE_FOLDER(),mxconst::DEFAULT_WRITE_CONVERTED_FMS_TO_XPLANE_FOLDER); // v3.0.255.2 initialize strct_setup_layer.bOverideCustomExternalFPLN_folders
  // v3.0.303.6
  this->strct_setup_layer.bForceNormalizedVolume = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_NORMALIZE_VOLUME_B(), false);
  this->strct_setup_layer.iNormalizedVolume_val  = Utils::getNodeText_type_1_5<int>(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_NORMALIZED_VOLUME(), mxconst::DEFAULT_SETUP_MISSION_VOLUME_I);

  // v3.305.1
  this->strct_setup_layer.setPilotName(Utils::getNodeText_type_6(system_actions::pluginSetupOptions.node, mxconst::SETUP_PILOT_NAME, mxconst::DEFAULT_SETUP_PILOT_NAME)); // initialize pilot name from preference file

  // v25.03.3
  this->strct_setup_layer.setSimbriefPilotID (Utils::getNodeText_type_6 (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_SIMBRIEF_PILOT_ID(), "")); // initialize  Simbrief pilot ID from preference file

  // Initialize local day - always initialize to 90days and 23 o'clock. Will be re-initialized on first briefer open which provide better result.
  adv_settings_strct.iClockDayOfYearPicked = dataref_manager::getLocalDateDays();
  adv_settings_strct.iClockHourPicked      = dataref_manager::getLocalHour();
  adv_settings_strct.iClockMinutesPicked   = dataref_manager::getLocalMinutes (); // v25.04.2


  // v25.04.1 reserve the original "cargo_arr" before reading from the external cargo.xml file
  // this->cargo_arr_copy.reserve (cargo_arr.size()); // Allocate space
  this->cargo_arr_copy.clear ();
  for (const auto str : this->cargo_arr)
  {
    const auto acopy = new char[strlen(str) + 2];
    // sprintf (acopy, "%s", str);
    snprintf (acopy,strlen(str + 1),"%s", str);
    this->cargo_arr_copy.push_back (acopy);
  }

  // v24.05.1 Read categories
  this->vecExternalCategories.clear();
  this->vecExternalCategories = Utils::read_external_categories(mxconst::get_GENERATE_TYPE_CARGO());
  if (!this->vecExternalCategories.empty())
  {
    this->cargo_arr.clear();
    for (auto &val : this->vecExternalCategories)
      this->cargo_arr.emplace_back(val.c_str());

    // Utils::addElementToMap (this->mapMissionCategories, static_cast<int>(missionx::mx_mission_type::cargo), this->cargo_arr );
    this->mapMissionCategories[static_cast<int> (missionx::mx_mission_type::cargo)] = this->cargo_arr;
  }

  strct_user_create_layer.dyn_slider1_lbl = "[" + Utils::formatNumber<float>(this->mapListPlaneRadioLabel[missionx::mx_plane_types::plane_type_helos].from_slider_min, 0) + "..." + Utils::formatNumber<float>(this->mapListPlaneRadioLabel[missionx::mx_plane_types::plane_type_helos].from_slider_max, 0) + "]";
  strct_user_create_layer.dyn_slider2_lbl = "[" + Utils::formatNumber<float>(this->mapListPlaneRadioLabel[missionx::mx_plane_types::plane_type_helos].to_slider_min, 0) + "..." + Utils::formatNumber<float>(this->mapListPlaneRadioLabel[missionx::mx_plane_types::plane_type_helos].to_slider_max, 0) + "]";


  // v25.02.1
  this->strct_setup_layer.bSuppressDistanceMessages = Utils::getNodeText_type_1_5<bool> ( missionx::system_actions::pluginSetupOptions.node, mxconst::get_ATTRIB_SUPPRESS_DISTANCE_MESSAGES_B(), false );

  // v25.04.2 Auto Load GPS waypoints into GPS on mission start
  this->strct_cross_layer_properties.flag_auto_load_route_to_gps_or_fms = missionx::system_actions::pluginSetupOptions.getNodeText_type_1_5 <bool>(mxconst::get_PROP_AUTO_LOAD_ROUTE_TO_GPS_OR_FMS_B (), true);

} // End Constructor



// ------------ destructor --------------
// WinImguiBriefer::~WinImguiBriefer() {}

// ------------ set layer --------------
void
WinImguiBriefer::setLayer(missionx::uiLayer_enum inLayer)
{
  this->prevBrieferLayer = this->currentLayer;
  this->currentLayer     = inLayer;
}

// -------------------------------------

float
WinImguiBriefer::calc_and_getNewFontScaledSize(float inNewSize)
{
  const float newFontScaleSize = this->strct_setup_layer.fPreferredFontScale + (inNewSize);
  if (newFontScaleSize > this->strct_setup_layer.fFontMaxScaleSize || newFontScaleSize < this->strct_setup_layer.fFontMinScaleSize) // make sure boundaries are kept
    return this->strct_setup_layer.fPreferredFontScale;

  return newFontScaleSize;
}

// -------------------------------------

void
WinImguiBriefer::add_abort_all_channels_debug()
{  
  missionx::WinImguiBriefer::HelpMarker("Abort all active channels.\nCan help when a message has a long background or comm mix running.");
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_orange);
  if (ImGui::Button("Stop All Channels"))
  {
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::sound_abort_all_channels);
  }
  ImGui::PopStyleColor();
}

// -------------------------------------

void
WinImguiBriefer::add_pause_in_2d_mode()
{
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_orange);
  if (ImGui::Checkbox("Pause in 2D mode (ignored when popped out)", &this->strct_setup_layer.bPauseIn2D))
  {
    // ADD set option value
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_AUTO_PAUSE_IN_2D(), this->strct_setup_layer.bPauseIn2D);
    this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
  }
  ImGui::PopStyleColor();
}

// -------------------------------------

void
WinImguiBriefer::add_font_size_scale_buttons()
{
  ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE); // v3.305.1
  
  //// +/- font size
  if (ImGui::SmallButton(" + "))
  {
    this->strct_setup_layer.fPreferredFontScale = this->calc_and_getNewFontScaledSize(0.1f);
  }
  this->mx_add_tooltip(missionx::color::color_vec4_white, "Increase font scale");
  ImGui::SameLine();
  ImGui::Text("/");
  ImGui::SameLine();

  if (ImGui::SmallButton(" - "))
  {
    this->strct_setup_layer.fPreferredFontScale = this->calc_and_getNewFontScaledSize(-0.1f);
  }
  this->mx_add_tooltip(missionx::color::color_vec4_white, "Decrease font scale");

  ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale); // v3.305.1
}

// -------------------------------------

void
WinImguiBriefer::add_skewed_marker_checkbox()
{
  ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
  if (ImGui::Checkbox(std::string("Place markers near targets and not above them (in +/-" + mxUtils::formatNumber<double>(mxconst::MAX_AWAY_SKEWED_DISTANCE_NM, 1) + "nm radius).").c_str(), &this->strct_setup_layer.bPlaceMarkersAwayFromTarget))
  {
    // ADD set option value
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET(), this->strct_setup_layer.bPlaceMarkersAwayFromTarget);
    missionx::system_actions::store_plugin_options();
  }
} // add_skewed_marker_checkbox

// ------------ Add start button --------------

void
WinImguiBriefer::add_ui_start_mission_button(missionx::mx_window_actions inActionToExecute)
{
  int iStyle = 0;
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
  iStyle++;
  ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lime);
  iStyle++;
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_azure);
  iStyle++;
  if (ImGui::Button(this->LBL_START_MISSION.c_str()))
    this->execAction(inActionToExecute); // should hide the window
  ImGui::PopStyleColor(iStyle);
}
// ------------ add abort thread button --------------

void
WinImguiBriefer::add_ui_abort_mission_creation_button(missionx::mx_window_actions inActionToExecute)
{
  int iStyle = 0;
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
  iStyle++;
  ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_aliceblue);
  iStyle++;
  if (ImGui::Button(this->LBL_ABORT_THREAD_LABEL.c_str()))
    this->execAction(inActionToExecute); // should hide the window
  ImGui::PopStyleColor(iStyle);
}

// --------------------------

void
WinImguiBriefer::add_ui_expose_all_gps_waypoints (const missionx::mx_window_actions inActionToExecute)
{
  if (this->getCurrentLayer() == missionx::uiLayer_enum::option_user_generates_a_mission_layer)
  {
    missionx::WinImguiBriefer::HelpMarker("Will be saved in the preference file.");
    ImGui::SameLine();
  }

  if (ImGui::Checkbox("Expose All GPS legs at mission start", &this->strct_setup_layer.bGPSImmediateExposure))
  {
    // ADD set option value
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE(), this->strct_setup_layer.bGPSImmediateExposure);
    this->execAction(inActionToExecute);
  }
}

// --------------------------

void
WinImguiBriefer::add_ui_suppress_distance_messages_checkbox_ui ( missionx::mx_window_actions inActionToExecute )
{
  missionx::WinImguiBriefer::HelpMarker ( "When the Random engine is generating a mission, do you want to suppress the progress messages as you near the target ?\nWill be saved in the preference file." );
  ImGui::SameLine ();

  if ( ImGui::Checkbox ( "Suppress distance messages, to a waypoint, when generating a random mission.", &this->strct_setup_layer.bSuppressDistanceMessages ) )
  {
    // ADD set option value
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool> ( mxconst::get_ATTRIB_SUPPRESS_DISTANCE_MESSAGES_B(), this->strct_setup_layer.bSuppressDistanceMessages );
    this->execAction ( inActionToExecute );
  }
}

// --------------------------

void
WinImguiBriefer::add_ui_default_weights ()
{
  // ------------------------
  // -- Default Weight
  // ------------------------
  ImGui::Checkbox("Add default base weights.\nThis will be added to the weight of any item you pick from an external inventory.\n(Not advisable for aircraft larger than General Aviation)", &this->adv_settings_strct.flag_add_default_weight_settings);
  if (this->adv_settings_strct.flag_add_default_weight_settings)
  {
    missionx::WinImguiBriefer::HelpMarker("Define Pilot Weight (0..200)");
    ImGui::SameLine ();
    ImGui::SetNextItemWidth ( 100.0f );
    if (ImGui::InputInt ( "Pilot Weight", &this->adv_settings_strct.pilot_base_weight_i, 5, 100, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal ) )
    {
      if (this->adv_settings_strct.pilot_base_weight_i < 0)
        this->adv_settings_strct.pilot_base_weight_i = 0;
      if (this->adv_settings_strct.pilot_base_weight_i > 200)
        this->adv_settings_strct.pilot_base_weight_i = 200;
    }
    if (missionx::data_manager::flag_setupUseXP11InventoryUI)
    {
      missionx::WinImguiBriefer::HelpMarker("Define the total passenger weight (0..50000)");
      ImGui::SameLine ();
      ImGui::SetNextItemWidth ( 100.0f );
      if (ImGui::InputInt ( "Total Passengers Weight", &this->adv_settings_strct.passengers_base_weight_i, 5, 100, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal ))
      {
        if (this->adv_settings_strct.passengers_base_weight_i < 0)
          this->adv_settings_strct.passengers_base_weight_i = 0;
        if (this->adv_settings_strct.passengers_base_weight_i > 50000)
          this->adv_settings_strct.passengers_base_weight_i = 50000;
      }

      missionx::WinImguiBriefer::HelpMarker("Total Cargo Weight (0..80000)");
      ImGui::SameLine ();
      ImGui::SetNextItemWidth ( 100.0f );
      if (ImGui::InputInt ( "Total Cargo Weight", &this->adv_settings_strct.cargo_base_weight, 5, 100, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal ))
        {
          if (this->adv_settings_strct.cargo_base_weight < 0)
            this->adv_settings_strct.cargo_base_weight = 0;
          if (this->adv_settings_strct.cargo_base_weight > 80000)
            this->adv_settings_strct.cargo_base_weight = 80000;
        }
    } // display only in XP11 inventory type
  }
}

// --------------------------

void
WinImguiBriefer::add_message_text()
{
  // display message text in Yellow
  if (!this->sBottomMessage.empty())
  {
    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
    ImGui::TextWrapped("%s", sBottomMessage.c_str());
    ImGui::PopStyleColor(1);
  }
}

// --------------------------

void
WinImguiBriefer::add_story_next_button()
{
  const auto lmbda_should_we_use_os_clock = [&]()
  {
    if (this->IsInsideSim())
    {
      return true; // we want OS clock
    }

    return false; // XP clock
  };

  const auto lmbda_progress_to_next_message = [&]()
  {
    this->strct_flight_leg_info.strct_story_mode.timerForAutoSkip.reset();     // reset auto skip clock
    Message::lineAction4ui.state = missionx::enum_mx_line_state::action_ended; // end action
  };                                                                           // end lmbda

  if (missionx::data_manager::flag_setupAutoSkipStoryMessage)
    ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_orange);

  if (ImGui::Button( std::string("Next " + Utils::from_u8string( ICON_FA_STEP_FORWARD)).c_str() ) || missionx::data_manager::flag_setupAutoSkipStoryMessage)
  {
    if (Message::lineAction4ui.actionCode == mxconst::STORY_ACTION_TEXT)
    {
      Message::lineAction4ui.bUserPressNextInTextActionMode = true;
    }

    lmbda_progress_to_next_message();
  }
  else if (missionx::Timer::wasEnded(this->strct_flight_leg_info.strct_story_mode.timerForAutoSkip, lmbda_should_we_use_os_clock()))
  {
    lmbda_progress_to_next_message();
  }
  else if (Message::lineAction4ui.actionCode == mxconst::STORY_ACTION_PAUSE)
  {
    ImGui::SameLine();
    ImGui::TextColored(missionx::color::color_vec4_beige, "%.0f", this->strct_flight_leg_info.strct_story_mode.timerForAutoSkip.getRemainingTime(lmbda_should_we_use_os_clock()));
  }

  if (missionx::data_manager::flag_setupAutoSkipStoryMessage)
    ImGui::PopStyleColor();
}

// -------------------------------

void
WinImguiBriefer::add_story_message_history_text()
{
  constexpr const static float fTitleHeight = 30.0f;
  constexpr const static float fPADDING     = 10.0f;

  ImVec2 vec2Window = this->mxUiGetWindowContentWxH();

  // draw end buttons
  ImGui::SameLine(vec2Window.x * 0.5f - 50.0f);
  if (ImGui::Button("Scroll to the End"))
  {
    strct_flight_leg_info.strct_story_mode.bScrollToEndOfHistoryMessages ^= 1;
  }
  
  ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_mx_dimblack);
  // since we are inside a child we will receive the child window height.
  ImGui::BeginChild("child_draw_history_story_messages", ImVec2(0.0f, ImGui::GetWindowHeight() - this->fTopToolbarPadding_f), ImGuiChildFlags_Borders);

  // Draw lines
  for (const auto& msg : missionx::data_manager::listOfMessageStoryMessages)
  {
    ImGui::TextColored(ImVec4(msg.imvec4Color.x, msg.imvec4Color.y, msg.imvec4Color.z, msg.imvec4Color.w), "%s", msg.label.c_str());
    ImGui::TextWrapped("%s", msg.message_text.c_str());
    ImGui::Spacing();
  }

  if (strct_flight_leg_info.strct_story_mode.bScrollToEndOfHistoryMessages)
  {
    ImGui::SetScrollHereY(1.0f);
    strct_flight_leg_info.strct_story_mode.bScrollToEndOfHistoryMessages ^= 1;
  }

  ImGui::EndChild();
  ImGui::PopStyleColor();
  //ImGui::Separator();
}

// -------------------------------

void
WinImguiBriefer::add_info_to_flight_leg()
{
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
  if (mxUtils::isElementExists(data_manager::mapBrieferMissionList, missionx::data_manager::selectedMissionKey))
  {
    ImGui::TextWrapped("%s", data_manager::mapBrieferMissionList[missionx::data_manager::selectedMissionKey].missionDesc.c_str());
  }
  else if (mxUtils::isElementExists(data_manager::mapGenerateMissionTemplateFiles, missionx::data_manager::selectedMissionKey))
  {
    ImGui::TextWrapped("%s", missionx::data_manager::mapGenerateMissionTemplateFiles[missionx::data_manager::selectedMissionKey].description.c_str());
  }
  this->mxUiReleaseLastFont();
}



void
WinImguiBriefer::add_debug_info()
{
  constexpr const float        fTitleHeight = 30.0f;
  constexpr const static float fPADDING     = 10.0f;

  ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);

  if (missionx::data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running && ImGui::BeginTabBar("FlightLegInfo", ImGuiTabBarFlags_None))
  {
    if (ImGui::BeginTabItem("Flight Leg Info"))
    {
      ImGui::BeginGroup();
      ImGui::BeginChild("flightLegInfoTabDebug", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - fPADDING), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar); // v3.305.3
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
        {
          auto&       leg = missionx::data_manager::mapFlightLegs[missionx::data_manager::currentLegName];
          std::string objName, tasksInfo;
          objName.clear();
          tasksInfo.clear();

          // Display Leg
          ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", leg.to_string_ui_leg_info().c_str());
          ImGui::Spacing();

          for (const auto& objName : leg.listObjectivesInFlightLeg)
          {
            auto& obj = data_manager::mapObjectives[objName];

            // Display Objective + tasks
            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
            ImGui::TextColored(missionx::color::color_vec4_aqua, "Objective: %s:", objName.c_str());

            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_SMALL());
            {
              ImGui::TextColored(missionx::color::color_vec4_grey, "(%s)", "ACM: all conditions are met, SCM: script condition met, TE: timer ended, In Area: physical + elev");

              this->print_tasks_ui_debug_info(obj);
            }
            this->mxUiReleaseLastFont(2);
          }

        } // end flight leg debug info
        this->mxUiReleaseLastFont();
      }
      ImGui::EndChild();
      ImGui::EndGroup();

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Triggers"))
    {
      ImGui::BeginGroup();
      ImGui::BeginChild("flightLegTriggerInfoDebug", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar ); // v3.305.3
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
        ImGui::TextColored(missionx::color::color_vec4_lime, "%s\n", "Display Triggers that are linked to the current flight leg:");
        ImGui::TextColored(missionx::color::color_vec4_grey, "%s", "X-Plane should not be in PAUSE state. Use the debug buttons at your own risk !!!");
        this->mxUiReleaseLastFont();

        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_SMALL());
        this->print_triggers_ui_debug_info();
        this->mxUiReleaseLastFont();
      }
      ImGui::EndChild();
      ImGui::EndGroup();

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Scripts"))
    {
      ImGui::BeginGroup();
      ImGui::BeginChild("flightLegScriptsInfoDebug", ImVec2(0.0f, ImGui::GetContentRegionAvail().y)); // v3.305.3
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
        ImGui::TextColored(missionx::color::color_vec4_lime, "%s\n", "Scripts available in current mission: \n(some might not be present at first and will be added once they are called - external file)");
        ImGui::Spacing();
        ImGui::Separator();
        this->mxUiReleaseLastFont();

        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_SMALL());
        this->print_scripts_ui_debug_info();
        this->mxUiReleaseLastFont();
      }
      ImGui::EndChild();
      ImGui::EndGroup();

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("DataRefs"))
    {
      ImGui::BeginGroup();
      ImGui::BeginChild("flightLegDatarefInfoDebug", ImVec2(0.0f, ImGui::GetContentRegionAvail().y)); // v3.305.3
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
        ImGui::TextColored(missionx::color::color_vec4_lime, "%s\n", "Datarefs defined in current mission:");
        ImGui::Spacing();
        ImGui::Separator();
        this->mxUiReleaseLastFont();

        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_SMALL());
        this->print_datarefs_ui_debug_info();
        this->mxUiReleaseLastFont();
      }
      ImGui::EndChild();
      ImGui::EndGroup();

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Globals"))
    {
      ImGui::BeginGroup();
      ImGui::BeginChild("GlobalScriptParameters", ImVec2(0.0f, ImGui::GetContentRegionAvail().y)); // v3.305.3
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
        ImGui::TextColored(missionx::color::color_vec4_lime, "%s\n", "Global parameters set by a script or the mission:");
        ImGui::Spacing();
        ImGui::Separator();
        this->mxUiReleaseLastFont();

        this->print_globals_ui_debug_info();

      }
      ImGui::EndChild();
      ImGui::EndGroup();

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Interpolation"))
    {
      ImGui::BeginGroup();
      ImGui::BeginChild("flightLegInterpolationDebug", ImVec2(0.0f, ImGui::GetContentRegionAvail().y)); // v3.305.3
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
        ImGui::TextColored(missionx::color::color_vec4_lime, "%s\n", "List of active interpolated datarefs:");
        ImGui::Spacing();
        ImGui::Separator();
        this->mxUiReleaseLastFont();

        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_SMALL());
        this->print_interpolated_ui_debug_info();
        this->mxUiReleaseLastFont();
      }
      ImGui::EndChild();
      ImGui::EndGroup();

      ImGui::EndTabItem();
    }

    if (missionx::data_manager::flag_setupShowDebugMessageTab && ImGui::BeginTabItem("Messages##TabDebugMessages"))
    {
      ImGui::BeginGroup();
      ImGui::BeginChild("debugAllMessages", ImVec2(0.0f, ImGui::GetContentRegionAvail().y)); // v3.305.3
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
          ImGui::TextColored(missionx::color::color_vec4_lime, "%s", "List of all messages. Make sure to un-pause. Standard messages are colored white.");
          this->add_pause_in_2d_mode();
          ImGui::SameLine(0.0f, 50.0f);
          this->add_abort_all_channels_debug();
          ImGui::Spacing();
          ImGui::Separator();
        this->mxUiReleaseLastFont();

        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_SMALL());
          this->print_messages_ui_debug_info();
        this->mxUiReleaseLastFont();
      }
      ImGui::EndChild();
      ImGui::EndGroup();

      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }


}

// ------------ flight notes --------------

void
WinImguiBriefer::add_flight_planning()
{
  constexpr static const auto flagsForShortInput     = ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank;
  constexpr static auto       mid_btn_size_vec2_vc   = ImVec2 (80.0f, 35.0f); // middle button size (SAVE/LOAD)
  constexpr static auto       simbrief_btn_size_vec2 = ImVec2 (45.0f, 45.0f); // Simbrief button. The size is uneven for visual symmetry
  constexpr static auto       multiLineSize_vec2_wp  = ImVec2 (-FLT_MIN, 37.0f); // waypoint multiline
  constexpr static auto       multiLineSize_vec2_vi  = ImVec2 (-FLT_MIN, 70.0f); // inner multi input size
  constexpr static auto       multiLineSize_vec2_vc  = ImVec2 (255.0f, multiLineSize_vec2_vi.y + 20.0f); // child size - for each "bottom multi line" notes

  const auto win_size_vec2          = this->mxUiGetWindowContentWxH ();
  constexpr auto multiLineSize_vec2_wpc = ImVec2 (0.0f, 40.0f); // child size for waypoints multi line

  const bool bFetchInProcess = this ->mxStartUiDisableState( this->strct_ext_layer.simbrief_fetch_state == missionx::mxFetchState_enum::fetch_in_process || missionx::data_manager::flag_generate_engine_is_running );
  ImGui::BeginGroup();
  {
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_SMALL());
    // ImGui::TextDisabled("%s", "Fill in your pre-flight or fetch from Simbrief. and flight notes so the most important staff will be available to you during flight.");
    ImGui::TextDisabled("%s", "Fetch pre-flight info from SimBrief or fill in manually");
    ImGui::SameLine(win_size_vec2.x - 80.0f);
    ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", "Temp:");
    ImGui::SameLine();
    ImGui::TextDisabled("%.2f", this->strct_flight_leg_info.outside_air_temp_degc);
    this->mxUiReleaseLastFont();

    // DEPARTURE Line
    const std::string_view sFromICAO = this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::fromICAO];
    const std::string_view sToICAO = this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::toICAO];

    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
    {
      if (sFromICAO.length() < 3)
        ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", "Departure:");
      else 
      {
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_greenyellow);
        if (ImGui::Button("Departure:"))
          this->callNavData(this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::fromICAO], true); // v24.03.1
        this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Fetch Departure nav data.");
        ImGui::PopStyleColor();
      }

      ImGui::SameLine(85.0f);
      ImGui::SetNextItemWidth(40.0f);
      ImGui::InputTextWithHint("##FromICAO", "ICAO", this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::fromICAO], sizeof this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::fromICAO], flagsForShortInput);
      ImGui::SameLine();
      ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", ":");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(40.0f);
      ImGui::InputTextWithHint("##FromPARK", "Park", this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::fromParkLoc], missionx::WinImguiBriefer::mx_flight_leg_info_layer::SHORT_FIELD_SIZE, flagsForShortInput);
      ImGui::SameLine();
      ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", ":");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(40.0f);
      ImGui::InputTextWithHint("##FromRW", "RW", this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::fromRunway], missionx::WinImguiBriefer::mx_flight_leg_info_layer::SHORT_FIELD_SIZE, flagsForShortInput);
      ImGui::SameLine();
      ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", ":");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(80.0f);
      ImGui::InputTextWithHint("##FromSID", "SID", this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::fromSID], missionx::WinImguiBriefer::mx_flight_leg_info_layer::SHORT_FIELD_SIZE, flagsForShortInput);
      ImGui::SameLine();
      ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", ":");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(80.0f);
      ImGui::InputTextWithHint("##FromTrans", "Transition", this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::fromTrans], missionx::WinImguiBriefer::mx_flight_leg_info_layer::SHORT_FIELD_SIZE, flagsForShortInput);
      // Taxi
      ImGui::SameLine(0.0f, 20.0f);
      ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", "Taxi");
      
      // Simbrief
      ImGui::SameLine (0.0f, 180.0f);
      missionx::WinImguiBriefer::mxUiHelpMarker (missionx::color::color_vec4_aqua, "Get latest Simbrief flight plan (you have to set Pilot ID in the [setup] screen)");
      ImGui::SameLine ();
      const auto posVec3 = ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY());
      
      if (ImGui::ImageButton("Simbrief", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_SIMBRIEF_ICO()].gTexture))
                       , simbrief_btn_size_vec2 ) )
      {
        this->execAction (mx_window_actions::ACTION_FETCH_FPLN_FROM_SIMBRIEF_SITE );
      }
      this->mx_add_tooltip (missionx::color::color_vec4_white, "Get Simbrief flight plan");

      ImGui::SetCursorPos(ImVec2(posVec3.x, posVec3.y ));
      ImGui::NewLine ();
      ImGui::Spacing ();
      ImGui::Spacing ();
      ImGui::Spacing ();


      // ARRIVAL Line
      if (sToICAO.length() < 3)
        ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", "Arrival:");
      else
      {
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_greenyellow);
        if (ImGui::Button("Arrival:"))
          this->callNavData(this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::toICAO], true); // v24.03.1
        this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Fetch Arrival nav data.");
        ImGui::PopStyleColor();
      }
      ImGui::SameLine(85.0f);
      ImGui::SetNextItemWidth(40.0f);
      ImGui::InputTextWithHint("##toICAO", "ICAO", this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::toICAO], missionx::WinImguiBriefer::mx_flight_leg_info_layer::SHORT_FIELD_SIZE, flagsForShortInput);
      ImGui::SameLine();
      ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", ":");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(40.0f);
      ImGui::InputTextWithHint("##toPARK", "Park", this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::toParkLoc], missionx::WinImguiBriefer::mx_flight_leg_info_layer::SHORT_FIELD_SIZE, flagsForShortInput);
      ImGui::SameLine();
      ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", ":");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(40.0f);
      ImGui::InputTextWithHint("##toRW", "RW", this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::toRunway], missionx::WinImguiBriefer::mx_flight_leg_info_layer::SHORT_FIELD_SIZE, flagsForShortInput);
      ImGui::SameLine();
      ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", ":");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(80.0f);
      ImGui::InputTextWithHint("##toSTAR", "STAR", this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::toSTAR], missionx::WinImguiBriefer::mx_flight_leg_info_layer::SHORT_FIELD_SIZE, flagsForShortInput);
      ImGui::SameLine();
      ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", ":");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(80.0f);
      ImGui::InputTextWithHint("##toTrans", "Transition", this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::toTrans], missionx::WinImguiBriefer::mx_flight_leg_info_layer::SHORT_FIELD_SIZE, flagsForShortInput);
      // Taxi
      ImGui::SameLine(0.0f, 20.0f);
      ImGui::SetNextItemWidth(200.0f);
      ImGui::InputTextWithHint("##taxi", "Taxi Route", this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::taxi], missionx::WinImguiBriefer::mx_flight_leg_info_layer::LONG_FIELD_SIZE, ImGuiInputTextFlags_CharsUppercase);
      ImGui::SameLine();
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
      if (ImGui::Button(mxUtils::from_u8string(ICON_FA_TRASH).c_str()))
      {
        memcpy(this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::taxi], "\0", sizeof("\0")); // should work cross platforms but not very safe
      }
      this->mxUiReleaseLastFont();

      // Display WAYPOINTS in its own line
      ImGui::TextColored(missionx::color::color_vec4_burlywood, "%s", "Enter Route Waypoints:");

      ImGui::BeginChild("waypoints##Child", multiLineSize_vec2_wpc, ImGuiChildFlags_None, ImGuiWindowFlags_None);
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_lightgoldenrodyellow);
        ImGui::InputTextMultiline("##Waypoints", this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::waypoints]
                                , sizeof(this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::waypoints])
                                , multiLineSize_vec2_wp, ImGuiInputTextFlags_CharsUppercase);
        ImGui::PopStyleColor();
        this->mxUiReleaseLastFont ();
      }
      ImGui::EndChild();


      // -----------------
      // Center BUTTONS
      // -----------------
      // v25.03.3
      ImGui::NewLine (); 
      ImGui::SameLine (0.0f);
      if (ImGui::Button("Clear Waypoints"))
      {
        this->strct_flight_leg_info.setNoteLongField (missionx::enums::mx_note_longField_enum::waypoints, "");
      }

      // SAVE BUTTON
      ImGui::SameLine(win_size_vec2.x * 0.5f, 0.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_grey);
      // if (ImGui::Button("Save", ImVec2(80.0f, multiLineSize_vec2_wp.y)) )
      if (ImGui::Button("Save", mid_btn_size_vec2_vc ) )
      {
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::save_notes_info);
      }
      // LOAD BUTTON
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::Button("Load", mid_btn_size_vec2_vc ) )
      {
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::load_notes_info);
      }
      ImGui::PopStyleColor();
      
      // clear Flight Plan buttons

      //////// Lambda ///////////////////////
      const auto lmbda_start_timer_color = [&] (const bool inState)
      {
        if (inState)
        {
          ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_darkgreen);
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_lightgreen);          
        }
      };

      const auto lmbda_end_timer_color = [] (const bool inState)
      {
        if (inState)
          ImGui::PopStyleColor(2);
      };
      ///////// END Lambda /////////////////

      missionx::Timer::evalTimeAndResetOnEnd(this->strct_flight_leg_info.tmPressedClearU, true);
      missionx::Timer::evalTimeAndResetOnEnd(this->strct_flight_leg_info.tmPressedClearD, true);
      missionx::Timer::evalTimeAndResetOnEnd(this->strct_flight_leg_info.tmPressedClearAll, true);

      ImGui::SameLine (0.0f, 50.0f);
      const auto posVec2 = ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY());
      bool bChangeToGreen = this->strct_flight_leg_info.tmPressedClearU.isRunning();

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());

      lmbda_start_timer_color(bChangeToGreen);
      if (ImGui::Button(std::string("Clear " + mxUtils::from_u8string(ICON_FA_ARROW_UP)).c_str(), ImVec2(80.0f, mid_btn_size_vec2_vc.y * 0.45f)))
      {
        if (this->strct_flight_leg_info.tmPressedClearU.isRunning() ) // if not running
        {
          this->strct_flight_leg_info.tmPressedClearU.reset();
          this->strct_flight_leg_info.resetNotesUpperPart();          
        }
        else
        {
          missionx::Timer::start(this->strct_flight_leg_info.tmPressedClearU, 3.0f, "TimerU");
          this->setMessage("You need to click twice to clear the upper screen.", 3);
        }
      }
      lmbda_end_timer_color(bChangeToGreen);

      // place the second button
      ImGui::SetCursorPos(ImVec2(posVec2.x, posVec2.y + mid_btn_size_vec2_vc.y * 0.5f));

      bChangeToGreen = this->strct_flight_leg_info.tmPressedClearD.isRunning();

      lmbda_start_timer_color(bChangeToGreen);
      if (ImGui::Button(std::string("Clear " + mxUtils::from_u8string(ICON_FA_ARROW_DOWN)).c_str(), ImVec2(80.0f, mid_btn_size_vec2_vc.y * 0.45f)))
      {
        if (this->strct_flight_leg_info.tmPressedClearD.isRunning()) // if not running
        {
          this->strct_flight_leg_info.tmPressedClearD.reset();
          this->strct_flight_leg_info.resetNotesLowerPart();         
        }
        else
        {
          missionx::Timer::start(this->strct_flight_leg_info.tmPressedClearD, 3.0f, "TimerD");
          this->setMessage("You need to click twice to clear the lower screen.", 3);
        }
      }   
      lmbda_end_timer_color(bChangeToGreen);

      this->mxUiReleaseLastFont(); // release title regular

    }
    this->mxUiReleaseLastFont();
  }
  ImGui::EndGroup();

  ImGui::Separator();


  ///////////////////////////
  // LONG Input Text Fields
  /////////////////////////
  ImGui::BeginGroup();
  {

    // OPTION 2 - Three vertical items
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
    {      
      float i = 1.0f; // assist in calculating text header position

      /////////////////
      // Input items
      ////////////////

       ///////// VERTICAL MULTI LINE INPUT FIELDS
      // Headers for Takeoff/Cruise/Descent
      std::string buff_s = this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::takeoff_notes];

      ImGui::TextColored(missionx::color::color_vec4_burlywood, "Takeoff Notes:" );
      ImGui::SameLine(0.0f, 5.0f);
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL());
      ImGui::TextDisabled("%s", fmt::format("{}/{}", buff_s.length(), missionx::WinImguiBriefer::mx_flight_leg_info_layer::iMaxCharsInLongField).c_str() );
      this->mxUiReleaseLastFont();


      buff_s = this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::cruise_notes];
      ImGui::SameLine(multiLineSize_vec2_vc.x * i + (5.0f * i));
      ImGui::TextColored(missionx::color::color_vec4_burlywood, "%s", "Cruise Notes:");
      ImGui::SameLine(0.0f, 5.0f);
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL());
      ImGui::TextDisabled("%s", fmt::format("{}/{}", buff_s.length(), missionx::WinImguiBriefer::mx_flight_leg_info_layer::iMaxCharsInLongField).c_str() );
      this->mxUiReleaseLastFont();

      i++;

      buff_s = this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::descent_notes];
      ImGui::SameLine(multiLineSize_vec2_vc.x * i + (5.0f * i));
      ImGui::TextColored(missionx::color::color_vec4_goldenrod, "%s", "Descent Notes:");
      ImGui::SameLine(0.0f, 5.0f);
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL());
      ImGui::TextDisabled("%s", fmt::format("{}/{}", buff_s.length(), missionx::WinImguiBriefer::mx_flight_leg_info_layer::iMaxCharsInLongField).c_str() );
      this->mxUiReleaseLastFont();

      // MultiLine Input for Takeoff/Cruise/Descent
      ImGui::BeginChild("takeoff##Child1", multiLineSize_vec2_vc, ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_bisque);
      ImGui::InputTextMultiline("##Departure", this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::takeoff_notes], missionx::WinImguiBriefer::mx_flight_leg_info_layer::LONG_FIELD_SIZE, multiLineSize_vec2_vi, ImGuiInputTextFlags_AllowTabInput);
      ImGui::PopStyleColor();
      ImGui::EndChild();

      ImGui::SameLine(0.0f, 5.0f);
      
      ImGui::BeginChild("cruise##Child2", multiLineSize_vec2_vc, ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_bisque);
      ImGui::InputTextMultiline("##Cruise", this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::cruise_notes], missionx::WinImguiBriefer::mx_flight_leg_info_layer::LONG_FIELD_SIZE, multiLineSize_vec2_vi, ImGuiInputTextFlags_AllowTabInput);
      ImGui::PopStyleColor();
      ImGui::EndChild();

      ImGui::SameLine(0.0f, 5.0f);
      
      ImGui::BeginChild("descent##Child3", multiLineSize_vec2_vc, ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_orange);
      ImGui::InputTextMultiline("##Descent", this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::descent_notes], missionx::WinImguiBriefer::mx_flight_leg_info_layer::LONG_FIELD_SIZE, multiLineSize_vec2_vi, ImGuiInputTextFlags_AllowTabInput);
      ImGui::PopStyleColor();
      ImGui::EndChild();


    }

    this->mxUiReleaseLastFont();
  
  }
  ImGui::EndGroup();

  this->mxEndUiDisableState(bFetchInProcess);

  // -----------------
  // Bottom BUTTONS
  // -----------------
  ImGui::BeginGroup ();
      // v25.03.3
    if (data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running)
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
      ImGui::Spacing ();

      if (!missionx::RandomEngine::threadState.flagIsActive)
      {
        // this->add_ui_advance_settings_random_date_time_weather_and_weight_button(this->adv_settings_strct.iClockDayOfYearPicked, this->adv_settings_strct.iClockHourPicked, this->adv_settings_strct.iClockMinutesPicked);

        // -------------------
        // Generate Mission Popup Button - FPLN screen
        // -------------------
        ImGui::SameLine (0.0f, 20.0f);
        ImGui::SameLine (win_size_vec2.x * 0.4f);
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_grey);
        if (ImGui::Button (">> Open Generate Mission Window <<", ImVec2 (250, 0)))
        {
          this->strct_flight_leg_info.fpln.reset ();
          this->strct_flight_leg_info.fpln.internal_id                              = 0;
          this->strct_flight_leg_info.fpln.waypoints_i                              = 0;
          this->strct_flight_leg_info.fpln.fromICAO_s                               = std::string (this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::fromICAO]);
          this->strct_flight_leg_info.fpln.toICAO_s                                 = std::string (this->strct_flight_leg_info.mapNoteFieldShort[missionx::enums::mx_note_shortField_enum::toICAO]);
          this->strct_flight_leg_info.fpln.formated_nav_points_with_guessed_names_s = std::string (this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::waypoints]);
          this->strct_flight_leg_info.fpln.notes_s                                  = "Based on Simbrief/User flight plan data.";
          // display modal window and rest of information

          // basic validation
          if (this->strct_flight_leg_info.fpln.toICAO_s.empty () || this->strct_flight_leg_info.fpln.fromICAO_s.empty ())
          {
            this->setMessage ("Check Departure/Arival ICAO fields, they must have values before generating a mission from them." );
          }
          else
          {
            // Generate mission from this after showing "are you sure" modal window
            // IXMLNode node_ptr = missionx::data_manager::prop_userDefinedMission_ui.node;
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int> (missionx::_mission_type::cargo)); //, node_ptr, node_ptr.getName());
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_NO_OF_LEGS(), 0); // legs will be dectate by RandomEngine. Should only be 1 and simmer will add the rest
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_USE_OSM_CHECKBOX(), false); // always false
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_USE_WEB_OSM_CHECKBOX(), false); // always false

            missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_FROM_ICAO(), this->strct_flight_leg_info.fpln.fromICAO_s);
            missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_TO_ICAO(), this->strct_flight_leg_info.fpln.toICAO_s);

            ImGui::OpenPopup (GENERATE_QUESTION.c_str ());
          }
        }
        ImGui::PopStyleColor (); // pop style color for "generate popup button"

        const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        this->draw_popup_generate_mission_based_on_ext_fpln (GENERATE_QUESTION, this->strct_flight_leg_info.fpln, this->strct_flight_leg_info.fpln.internal_id);

        // -------------------
        // START BUTTON - FPLN
        // -------------------
        if (!this->asyncSecondMessageLine.empty())
        {
          // ImGui::SameLine((win_size_vec2.x * 0.75f) - (ImGui::CalcTextSize(this->LBL_START_MISSION.c_str()).x * 0.6f));
          ImGui::SameLine(0.0f, 60.0f);
          this->add_ui_start_mission_button(missionx::mx_window_actions::ACTION_START_RANDOM_MISSION);
        }

      }
      else // show abort button
      {
        // ABORT button
        ImGui::NewLine ();
        // ImGui::SameLine (win_size_vec2.x*0.5f, 0.0f);
        ImGui::SameLine ();
        missionx::WinImguiBriefer::HelpMarker("Abort the background process.");
        ImGui::SameLine();
        // ImGui::SameLine(25.0f, 0.0f);
        this->add_ui_abort_mission_creation_button(); // Add Abort Random Engine
      }


      this->mxUiReleaseLastFont ();
    } // end if mission is not running
  ImGui::EndGroup ();



}

// ------------ flc --------------
void
WinImguiBriefer::flc()
{
  #ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
  #endif // TIMER_FUNC

  // v3.305.1
  if (this->GetVisible() && !this->flag_displayedOnce)
  {
    this->flag_displayedOnce = true;
    this->SetWindowPositioningMode(xplm_WindowPositionFree);
  }

  // Force showing Mission-X window during story mode. We use the 
  if (!this->GetVisible()  
      && this->strct_flight_leg_info.internal_child_layer == missionx::uiLayer_enum::flight_leg_info_story_mode 
      && data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running 
      && this->currentLayer == missionx::uiLayer_enum::flight_leg_info       
      && Message::lineAction4ui.state > missionx::enum_mx_line_state::undefined 
      && Message::lineAction4ui.state < missionx::enum_mx_line_state::mainMessageEnded )
  {
    this->execAction(missionx::mx_window_actions::ACTION_SHOW_WINDOW);
  }
  // end v3.305.1

  // check Simbrief thread state
  if (this->strct_ext_layer.simbrief_fetch_state == missionx::mxFetchState_enum::fetch_ended)
  {
    // check vector size
    if (!data_manager::tableExternalFPLN_simbrief_vec.empty () && this->strct_ext_layer.simbrief_called_layer == missionx::uiLayer_enum::flight_leg_info)
    {
      if (const missionx::mx_ext_internet_fpln_strct &fpln = data_manager::tableExternalFPLN_simbrief_vec.front ()
        ; !fpln.simbrief_route.empty () )
      {
        this->strct_flight_leg_info.setNoteShortField (missionx::enums::mx_note_shortField_enum::fromICAO, fpln.fromICAO_s);
        this->strct_flight_leg_info.setNoteShortField (missionx::enums::mx_note_shortField_enum::fromRunway, fpln.simbrief_from_rw);
        this->strct_flight_leg_info.setNoteShortField (missionx::enums::mx_note_shortField_enum::fromTrans, fpln.simbrief_from_trans_alt);
        this->strct_flight_leg_info.setNoteShortField (missionx::enums::mx_note_shortField_enum::toICAO, fpln.toICAO_s);
        this->strct_flight_leg_info.setNoteShortField (missionx::enums::mx_note_shortField_enum::toRunway, fpln.simbrief_to_rw);
        this->strct_flight_leg_info.setNoteShortField (missionx::enums::mx_note_shortField_enum::toTrans, fpln.simbrief_to_trans_alt);

        this->strct_flight_leg_info.setNoteLongField (missionx::enums::mx_note_longField_enum::waypoints, fpln.simbrief_route);

        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::save_notes_info);
      }
    }
    // reset state
    this->strct_ext_layer.simbrief_fetch_state = missionx::mxFetchState_enum::fetch_not_started;
  }


  // DEBUG COLORING: v3.305.3 script coloring for debug
  if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running && !data_manager::currentLegName.empty() )
  {
    // v3.305.3 if we need to abort mission on timeout
    this->countdown_textColorVec4 = (Utils::readBoolAttrib(missionx::data_manager::mapFailureTimers[missionx::data_manager::lowestFailTimerName_s].node, mxconst::get_ATTRIB_FAIL_ON_TIMEOUT_B(), true)) ? missionx::color::color_vec4_plum : missionx::color::color_vec4_beige;

    for (auto &script : missionx::script_manager::mapScripts | std::views::values)
    {
      script.flcDebugColors();
    }
    for (auto& key : data_manager::mapFlightLegs[data_manager::currentLegName].listTriggers)
    {
      data_manager::mapTriggers[key].strct_debug.flcDebugColors();
    }
  }
  // END DEBUG COLORING

  if (this->currentLayer == missionx::uiLayer_enum::flight_leg_info && this->strct_flight_leg_info.internal_child_layer == missionx::uiLayer_enum::flight_leg_info_inventory)
  {

    // data_manager::current_plane_payload_weight_f = missionx::data_manager::calculatePlaneWeight(data_manager::planeInventoryCopy, false, missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i); // v24.12.2 // v3.0.241.1 added inventory weight calculation for copied plane node to be reflected in Inventory screen
    data_manager::internally_calculateAndStorePlaneWeight(data_manager::planeInventoryCopy, false, missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i); // v24.12.2 // v3.0.241.1 added inventory weight calculation for copied plane node to be reflected in Inventory screen


    // Gather plane weight information when displaying inventory screen
    this->strct_flight_leg_info.acf_m_empty    = missionx::dataref_manager::getDataRefValue<float> ("sim/aircraft/weight/acf_m_empty");
    this->strct_flight_leg_info.acf_m_max      = missionx::dataref_manager::getDataRefValue<float> ("sim/aircraft/weight/acf_m_max");
    this->strct_flight_leg_info.m_fuel_total   = missionx::dataref_manager::getDataRefValue<float> ("sim/flightmodel/weight/m_fuel_total");
    this->strct_flight_leg_info.cg_indicator_f = missionx::dataref_manager::getDataRefValue<float> ("sim/cockpit2/gauges/indicators/CG_indicator");
    this->strct_flight_leg_info.cg_z_prct_f    = missionx::dataref_manager::getDataRefValue<float> ("sim/flightmodel2/misc/cg_offset_z_mac");

    // Prepare plane inventory weight information
    this->strct_flight_leg_info.calculated_plane_weight_f             = strct_flight_leg_info.acf_m_empty + strct_flight_leg_info.m_fuel_total + data_manager::current_plane_payload_weight_f;
    this->strct_flight_leg_info.plane_virtual_weight_fuel_and_payload = fmt::format("{:.2f}",this->strct_flight_leg_info.calculated_plane_weight_f);
    if (this->strct_flight_leg_info.calculated_plane_weight_f > this->strct_flight_leg_info.acf_m_max)
      this->strct_flight_leg_info.formated_plane_inv_title_s = fmt::format ("(!!{}!!/{})", this->strct_flight_leg_info.plane_virtual_weight_fuel_and_payload, this->strct_flight_leg_info.plane_max_weight_allowed );
    else
      this->strct_flight_leg_info.formated_plane_inv_title_s = fmt::format ("({}/{})", this->strct_flight_leg_info.plane_virtual_weight_fuel_and_payload, this->strct_flight_leg_info.plane_max_weight_allowed );

    strct_flight_leg_info.plane_max_weight_allowed = fmt::format("{:.2f}",this->strct_flight_leg_info.acf_m_max);
    // end plane weight calculation
  }

  if (Timer::wasEnded(this->timerMessage, true)) // message stopper should be based on OS time, since briefer pauses X-Plane while OS time continue ticking. Solve message never fades.
  {
    if (RandomEngine::threadState.flagIsActive && !RandomEngine::threadState.flagThreadDoneWork) // v3.0.223.1 display better progress information to simmer.
    {
      std::string time_s = missionx::Timer::get_current_time_and_date();
      this->setMessage("[" + time_s + "] Generate mission is still running... " + RandomEngine::threadState.getDuration() + "sec", 5);
    }
    else if (this->strct_ext_layer.fetch_state == missionx::mxFetchState_enum::fetch_in_process && this->strct_ext_layer.threadState.flagIsActive) // v24.06.1 add timer progress to external FPLN
    {
      std::string time_s = missionx::Timer::get_current_time_and_date();
      this->setMessage("[" + time_s + "] Data request is still in progress: " + this->strct_ext_layer.threadState.getDuration() + "sec", 5);
    }
    else
    {
      this->sBottomMessage.clear();
      this->timerMessage.reset(); // v24.03.1 reset timer state so bottom message won't continuously clear the status bottom message.
    }

  }

  // If visible: automatically move briefer into or outside VR based on user preference
  if (this->GetVisible())
  {
    if (this->IsInVR() && !missionx::mxvr::flag_in_vr)         // if our window in VR but x-plane is not in VR mode then move to 2D mode.
      nextWinPosMode = xplm_WindowPositionFree;                // place briefer
    else if (!this->IsInVR() && missionx::mxvr::vr_display_missionx_in_vr_mode) // if our window is visible in 2D mode but X-Plane is in VR mode and we support VR mode then move to VR
      nextWinPosMode = xplm_WindowVR;                          // automaticaly place window in VR
  }

  if (this->nextWinPosMode >= 0)
  {
    this->SetWindowPositioningMode(this->nextWinPosMode);
    // If we pop in, then we need to explicitly set a position for the window to appear
    if (this->nextWinPosMode == xplm_WindowPositionFree)
    {
      int left, top, right, bottom;
      this->GetCurrentWindowGeometry(left, top, right, bottom);
      // Normalize to our starting position (WIN_PAD|WIN_PAD), but keep size unchanged
      const int width  = right - left;
      const int height = top - bottom;
      Utils::CalcWinCoords(width, height, this->win_pad, this->win_coll_pad, left, top, right, bottom);
      right  = left + width;
      bottom = top - height;
      this->SetWindowGeometry(left, top, right, bottom);
    }
    this->nextWinPosMode = -1;
  }

  // Check if to pause xplane if briefer window is open
  if ((this->strct_setup_layer.bPauseIn2D && this->GetVisible() 
       && !missionx::dataref_manager::isSimPause() && !this->IsPoppedOut() 
       && !missionx::mxvr::flag_in_vr
      ) 
      || this->forcePauseSim) // If forcePauseSim or if Briefer UI is open and not popped-out and sim is not in PAUSE mode then pause sim
  {
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::pause_xplane);
  }
  // release pause state if it was paused by plugin and was not released after window closed or if we are in VR state
  else if ((this->GetVisible() == false && this->missionClassPausedSim 
            && missionx::dataref_manager::isSimPause() && !this->forcePauseSim
           )
           || 
           (this->GetVisible() && this->missionClassPausedSim && missionx::dataref_manager::isSimPause() 
            && (this->IsPoppedOut() || missionx::mxvr::flag_in_vr) 
            && !this->forcePauseSim)
          ) 
  {
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::unpause_xplane);
  }

  // CURL thread status - reset the external layer state so [fetch] button will be available
  if (this->strct_ext_layer.fetch_state == missionx::mxFetchState_enum::fetch_guess_wp_ended) // v3.0.255.2 modified from fetch_ended
    this->strct_ext_layer.fetch_state = missionx::mxFetchState_enum::fetch_not_started;

  // handle messages from async fetch external flight plan
  if (this->currentLayer == missionx::uiLayer_enum::option_external_fpln_layer)
  {
    if (!this->strct_ext_layer.asyncFetchMsg_s.empty())
    {
      std::string restart_suggestion_message            = (this->strct_ext_layer.asyncFetchMsg_s.find("Protocol https not supported") == std::string::npos) ? "" : ". Suggest to reload all plugins, it might solve the issue.";
      this->strct_ext_layer.bDisplayPluginsRestart = (restart_suggestion_message.empty()) ? false : true;

      this->setMessage(this->strct_ext_layer.asyncFetchMsg_s + restart_suggestion_message);
      this->strct_ext_layer.asyncFetchMsg_s.clear();
    }
  } // end handle messages from asynch fetch process

  // Handle messages from ILS thread
  else if (currentLayer == missionx::uiLayer_enum::option_ils_layer ) // v3.0.253.6 handle messages from the data_manager::fetch_ils_rw_from_sqlite()
  {
    std::string msg;
    if (!this->strct_ils_layer.asyncFetchMsg_s.empty())
    {
      this->strct_ils_layer.asyncFetchMsg_s.append(fmt::format("(limited to {} rows)", this->strct_ils_layer.limit_items[this->strct_ils_layer.limit_indx])); // v24.03.1 add the limit message.
      msg = this->strct_ils_layer.asyncFetchMsg_s;
      this->strct_ils_layer.asyncFetchMsg_s.clear();
    }
    else if (!this->strct_ils_layer.asyncNavFetchMsg_s.empty() + !this->strct_ils_layer.asyncMetarFetchMsg_s.empty())
    {
      msg = (this->strct_ils_layer.asyncMetarFetchMsg_s.empty()) ? this->strct_ils_layer.asyncNavFetchMsg_s : this->strct_ils_layer.asyncMetarFetchMsg_s;
      this->strct_ils_layer.asyncNavFetchMsg_s.clear();
      this->strct_ils_layer.asyncMetarFetchMsg_s.clear();
    }

    if (!msg.empty())
      this->setMessage(msg); // v24.03.1 changed display time to default 20 seconds from eight seconds should be enough
    
  }


  if (currentLayer == missionx::uiLayer_enum::flight_leg_info && this->strct_flight_leg_info.flagFlightPlanningTabIsOpen)
  {
    this->strct_flight_leg_info.outside_air_temp_degc = XPLMGetDataf(this->dc.dref_outside_air_temp_degc);
  }
  

  if (missionx::data_manager::flag_apt_dat_optimization_is_running + OptimizeAptDat::aptState.flagIsActive)
  {
    if (this->currentLayer != missionx::uiLayer_enum::option_setup_layer && this->currentLayer != missionx::uiLayer_enum::flight_leg_info && this->currentLayer != missionx::uiLayer_enum::about_layer )
      this->setLayer(missionx::uiLayer_enum::imgui_home_layer);
  }
} // flc

// ------------ contains --------------
bool
WinImguiBriefer::tableDataTy::contains(const std::string& s) const
{
  // try finding s in all our texts
  for (const std::string& t : { reg, model, typecode, owner })
  {
    std::string l = t;
    if (mxUtils::stringToUpper(l).find(s) != std::string::npos)
      return true;
  }

  // not found
  return false;
}



std::map<std::string, std::string>
WinImguiBriefer::read_fpln_files()
{
  const std::string                  pathToRead = "Output/FMS plans";
  std::map<std::string, std::string> mapListOfFiles;
  std::string                        filter = ".lnmpln";
  missionx::ListDir::getListOfFiles(pathToRead.c_str(), mapListOfFiles, filter);
  return mapListOfFiles;
}



void
WinImguiBriefer::set_vecOverpassUrls_char(const std::vector<std::string>& inVecData)
{
  if (!inVecData.empty())
  {
    this->strct_setup_layer.vecOverpassUrls_char.clear();
    for (auto& v : inVecData)
    {
      this->strct_setup_layer.vecOverpassUrls_char.emplace_back(v.c_str());
    }

    // this->strct_setup_layer.overpass_user_picked_combo_i = 0;
    missionx::data_manager::overpass_user_picked_combo_i = 0;
  }
}

// ------------ buildInterface --------------
void
WinImguiBriefer::buildInterface()
{
  ///////// Top TOOLBAR and Title - Title of layer and Navigation buttons

  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive)); // dark gray
  ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);                           // transparent
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(51, 153, 51, 255));              // green  https://www.w3schools.com/colors/colors_picker.asp
  draw_top_toolbar();                                                                     // TOOLBAR
  ImGui::PopStyleColor(3);


  ///////// Region 1 - main body of buttons
  switch (this->currentLayer)
  {
    case missionx::uiLayer_enum::uiLayerUnset:
    case missionx::uiLayer_enum::imgui_home_layer:
    {
      this->draw_home_layer();
    }
    break;
    case missionx::uiLayer_enum::option_generate_mission_from_a_template_layer:
    {
      this->draw_template_mission_generator_screen();
    }
    break;
    case missionx::uiLayer_enum::option_user_generates_a_mission_layer:
    {
      // this->draw_user_create_mission_layer();
      this->draw_dynamic_mission_creation_screen();
    }
    break;
    case missionx::uiLayer_enum::option_setup_layer:
    {
      this->draw_setup_layer();
    }
    break;
    case missionx::uiLayer_enum::flight_leg_info:
    {
      this->draw_flight_leg_info();
    }
    break;
    case missionx::uiLayer_enum::option_mission_list:
    {
      this->draw_load_existing_mission_screen();
    }
    break;
    case missionx::uiLayer_enum::option_conv_fpln_to_mission:
    {
      this->draw_conv_main_fpln_to_mission_window();
    }
    break;
    case missionx::uiLayer_enum::option_external_fpln_layer:
    {
      this->draw_external_fpln_screen();
    }
    break;
    case missionx::uiLayer_enum::option_ils_layer:
    {
      this->draw_ils_screen();
    }
    break;
    case missionx::uiLayer_enum::about_layer:
    {
      this->draw_about_layer();
    }
    break;
    default:
      break;
  } // end switch


  ///////// Region 2 - Footer area
  ImGui::BeginGroup();
  ImGui::BeginChild("briefer_bottom_region_01");
  {
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_MSG_BOTTOM()); // v3.303.14
    {
      // display message text in Yellow
      this->add_message_text(); // v3.305.1 converted to function to be more modular

      // display warning if 3D files from libraries are missing
      this->add_missing_3d_files_message(); // v3.0.255.3

      // display Flight plan result in Green
      if (!this->asyncSecondMessageLine.empty() && (this->currentLayer != missionx::uiLayer_enum::option_mission_list && this->currentLayer != missionx::uiLayer_enum::option_conv_fpln_to_mission))
      {
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_limegreen); // lime green
        ImGui::TextWrapped("%s", asyncSecondMessageLine.c_str());
        ImGui::PopStyleColor(1);
      }

      if (missionx::data_manager::flag_generate_engine_is_running) // v3.0.255.4
      {
        const std::string err = missionx::data_manager::overpass_fetch_err;
        if (!err.empty())
        {
          ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_orange); // orange
          ImGui::TextWrapped("%s", err.c_str());
          ImGui::PopStyleColor(1);
        }
      }
    }
    this->mxUiReleaseLastFont(); // v3.303.14
  }
  ImGui::EndChild(); // end region 2
  ImGui::EndGroup();

  /////// End Footer //////
} // buildInterface

void
WinImguiBriefer::validate_sliders_values(missionx::mx_plane_types inPlaneType)
{
  if (this->strct_user_create_layer.dyn_sliderVal1 < this->mapListPlaneRadioLabel[inPlaneType].from_slider_min)
    this->strct_user_create_layer.dyn_sliderVal1 = this->mapListPlaneRadioLabel[inPlaneType].from_slider_min;
  else if (this->strct_user_create_layer.dyn_sliderVal1 > this->mapListPlaneRadioLabel[inPlaneType].from_slider_max)
    this->strct_user_create_layer.dyn_sliderVal1 = this->mapListPlaneRadioLabel[inPlaneType].from_slider_max;

  if (this->strct_user_create_layer.dyn_sliderVal2 < this->mapListPlaneRadioLabel[inPlaneType].to_slider_min)
    this->strct_user_create_layer.dyn_sliderVal2 = this->mapListPlaneRadioLabel[inPlaneType].to_slider_min;
  else if (this->strct_user_create_layer.dyn_sliderVal2 > this->mapListPlaneRadioLabel[inPlaneType].to_slider_max)
    this->strct_user_create_layer.dyn_sliderVal2 = this->mapListPlaneRadioLabel[inPlaneType].to_slider_max;

}

// -------------------------------------------

void
WinImguiBriefer::generate_mission_date_based_on_user_preference(int& out_iClockDayOfYearPicked, int& out_iClockHourPicked, int& out_iClockMinutesPicked, const bool& inIncludeNightHours)
{
  switch (this->adv_settings_strct.iRadioRandomDateTime_pick)
  {
    case missionx::mx_ui_random_date_time_type::xplane_day_and_time:
    {
      out_iClockDayOfYearPicked = dataref_manager::getLocalDateDays();
      out_iClockHourPicked      = dataref_manager::getLocalHour();
      out_iClockMinutesPicked   = dataref_manager::getLocalMinutes ();
    }
    break;
    case missionx::mx_ui_random_date_time_type::os_day_and_time:
    {
      const missionx::mx_clock_time_strct osClock = Utils::get_os_time ();
      out_iClockDayOfYearPicked                   = osClock.dayInYear;
      out_iClockHourPicked                        = osClock.hour;
      out_iClockMinutesPicked                     = osClock.minutes;
    }
    break;
    case missionx::mx_ui_random_date_time_type::any_day_time:
    {
      const int fromHour = (inIncludeNightHours) ? 0 : 6;   // initialize with midnight or 6 am
      const int toHour   = (inIncludeNightHours) ? 23 : 19; // initialize with 11pm or 19 pm

      out_iClockDayOfYearPicked = Utils::getRandomIntNumber(0, 355);
      out_iClockHourPicked      = Utils::getRandomIntNumber(fromHour, toHour);
      out_iClockMinutesPicked   = Utils::getRandomIntNumber(0, 59); // v25.04.3
    }
    break;
    case missionx::mx_ui_random_date_time_type::pick_months_and_part_of_preferred_day:
    {
      std::map<int, int>            mapMonthPicked;
      std::map<int, mx_part_of_day> mapTimeInDayPicked; // holds which part of day we picked

      out_iClockMinutesPicked = 0; // v25.04.2 TODO: should we add randomness to the minutes, or should we always reset to zero ?

      // Which months were selected
      for (int y = 0; y < 3; y++)
        for (int x = 0; x < 4; x++)
        {
          if (this->adv_settings_strct.selected_dateTime_by_user_arr[y][x])
            Utils::addElementToMap(mapMonthPicked, this->adv_settings_strct.selected_month_no[y][x], mapCalander_days_in_a_month[this->adv_settings_strct.selected_month_no[y][x]]);
        }
      if (mapMonthPicked.empty() || this->adv_settings_strct.flag_checkAnyMonth)
        out_iClockDayOfYearPicked = Utils::getRandomIntNumber(0, 355);
      else
      {
        const int rnd                = Utils::getRandomIntNumber(1, static_cast<int> (mapMonthPicked.size ()));
        int       counter            = 1; // we start from 1 since the random started from 1
        int       picked_month_key_i = 1; // 1 == jan
        for (auto it = mapMonthPicked.cbegin(); it != mapMonthPicked.cend(); ++it, ++counter)
        {
          if (counter >= rnd)
          {
            picked_month_key_i = it->first;
            break;
          }
        }

        // loop over all months until we reach the month key while sum the days in the month
        int sumDays = 0;
        for (const auto& [monthKey_i, daysInMonth_i] : mapCalander_days_in_a_month)
        {
          if (monthKey_i == picked_month_key_i)
          {
            sumDays += Utils::getRandomIntNumber(1, daysInMonth_i);
            out_iClockDayOfYearPicked = sumDays;
            break;
          }
          else
            sumDays += daysInMonth_i;
        } // end loop

      } // end if we picked a month from the list

      /////////////// Calculate Time of day
      for (int y = 0; y < 4; y++)
        for (int x = 0; x < 2; x++)
        {
          if (this->adv_settings_strct.selectedTime[y][x])
          {
            const int key = this->adv_settings_strct.selected_time_no[y][x];
            // #ifndef RELEASE
            //             mx_part_of_day [[maybe_unused]] mxPartOfDay = mapCalander_parts_of_the_day[key]; // debug
            // #endif // !RELEASE

            Utils::addElementToMap(mapTimeInDayPicked, key, mapCalander_parts_of_the_day[key]);
          }
        }

      if (this->adv_settings_strct.checkPartOfDay_b || mapTimeInDayPicked.empty())
        out_iClockHourPicked = Utils::getRandomIntNumber(6, 19); // pick an hour between 06:00 and 19:00
      else                                                       // if picked valid hours
      {
        const int rnd     = Utils::getRandomIntNumber(1, static_cast<int> (mapTimeInDayPicked.size ()));
        int       counter = 1;
        for (auto it = mapTimeInDayPicked.cbegin(); it != mapTimeInDayPicked.end(); ++it, ++counter) // skip to the month we picked in the "rnd"
        {
          if (counter >= rnd)
          {
            out_iClockHourPicked = Utils::getRandomIntNumber(0, it->second.span_time) + it->second.start_hour; // calculate the picked hour
            if (out_iClockHourPicked > 23)                                                                     // if greater than 23 hours
              out_iClockHourPicked -= missionx::HOURS_IN_A_DAY_24;                                             // decrease 1 day = 24 hours
            break;
          }
        }
      }
    }
    break;
    case missionx::mx_ui_random_date_time_type::exact_day_and_time:
    {
    }
    break;
    default: // the two other options need more input from user
      break;
  } // end internal radio switch
}

// -------------------------------------------

void
WinImguiBriefer::addAdvancedSettingsPropertiesBeforeGeneratingRandomMission()
{
  // v3.303.14 added advance weather/time settings
  missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_STARTING_DAY(), this->clockDayOfYear_arr[this->adv_settings_strct.iClockDayOfYearPicked]);
  missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_STARTING_HOUR(), mxUtils::stringToNumber<int>(this->clockHours_arr[this->adv_settings_strct.iClockHourPicked]));
  missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_STARTING_MINUTE (), this->adv_settings_strct.iClockMinutesPicked );

  // Added weather information for RandomEngine
  switch (this->adv_settings_strct.iWeatherType_user_picked)
  {
    case missionx::mx_ui_random_weather_options::pick_pre_defined:
    {
      // store values in the prop_userDefinedMission_ui. During Random mission generation the weather will be picked from the list
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_WEATHER_USER_PICKED(), this->adv_settings_strct.get_weather_picked_by_user());
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_WEATHER_CHANGE_MODE_USER_PICKED(), this->adv_settings_strct.get_weather_change_mode_picked_by_user()); // v3.303.13
    }
    break;
    case missionx::mx_ui_random_weather_options::use_xplane_weather_and_store:
    {
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_WEATHER_USER_PICKED(), mxconst::get_VALUE_STORE_CURRENT_WEATHER_DATAREFS());
      missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute(mxconst::get_PROP_WEATHER_CHANGE_MODE_USER_PICKED().c_str()); // v3.303.13
    }
    break;
    default: // use_xplane_weather - does not store in random.xml file
    {
      missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute(mxconst::get_PROP_WEATHER_USER_PICKED().c_str());
      missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute(mxconst::get_PROP_WEATHER_CHANGE_MODE_USER_PICKED().c_str()); // v3.303.13
    }
    break;
  } // end switch on weather options

  // Add weight setting
  missionx::data_manager::prop_userDefinedMission_ui.setBoolProperty(mxconst::get_PROP_ADD_DEFAULT_WEIGHTS_TO_PLANE(), this->adv_settings_strct.flag_add_default_weight_settings);
  if (this->adv_settings_strct.flag_add_default_weight_settings)
  {
    missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> ( mxconst::get_OPT_PILOT_BASE_WEIGHT(), this->adv_settings_strct.pilot_base_weight_i );
    // v25.02.1
    if (missionx::data_manager::flag_setupUseXP11InventoryUI)
    {
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> ( mxconst::get_OPT_PASSENGERS_BASE_WEIGHT(), this->adv_settings_strct.passengers_base_weight_i );
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> ( mxconst::get_OPT_STORAGE_BASE_WEIGHT(), this->adv_settings_strct.cargo_base_weight );
    }
    else
    {
      missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute ( mxconst::get_OPT_PASSENGERS_BASE_WEIGHT().c_str () );
      missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute ( mxconst::get_OPT_STORAGE_BASE_WEIGHT().c_str () );
    }
  }
  else // remove default weight attributes if the "add default wights is not flagged
  {
    missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute ( mxconst::get_OPT_PILOT_BASE_WEIGHT().c_str () );
    missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute ( mxconst::get_OPT_PASSENGERS_BASE_WEIGHT().c_str () );
    missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute ( mxconst::get_OPT_STORAGE_BASE_WEIGHT().c_str () );
  }

}


// -------------------------------------------


void
WinImguiBriefer::refresh_slider_data_based_on_plane_type(missionx::mx_plane_types inPlaneType)
{
  strct_user_create_layer.dyn_slider1_lbl = "[" + Utils::formatNumber<float>(this->mapListPlaneRadioLabel[this->strct_user_create_layer.iRadioPlaneType].from_slider_min, 0) + "..." + Utils::formatNumber<float>(this->mapListPlaneRadioLabel[this->strct_user_create_layer.iRadioPlaneType].from_slider_max, 0) + "]";
  strct_user_create_layer.dyn_slider2_lbl = "[" + Utils::formatNumber<float>(this->mapListPlaneRadioLabel[this->strct_user_create_layer.iRadioPlaneType].to_slider_min, 0) + "..." + Utils::formatNumber<float>(this->mapListPlaneRadioLabel[this->strct_user_create_layer.iRadioPlaneType].to_slider_max, 0) + "]";

  this->validate_sliders_values(inPlaneType);

}


// ------------ print_tasks_ui_debug_info --------------
void
WinImguiBriefer::print_tasks_ui_debug_info(missionx::Objective& inObj)
{
  const auto lmbda_task_state_info = [](missionx::Objective& obj, const std::string& inTaskName)
  {
    auto&       tsk = obj.mapTasks[inTaskName];
    std::string state;
    std::string taskTypeName = inTaskName + " (" + Task::translateTaskType(tsk.task_type);


    switch (tsk.task_type)
    {
      case missionx::mx_task_type::trigger:
      {
        const auto trigName = tsk.getStringAttributeValue(mxconst::get_ATTRIB_BASE_ON_TRIGGER(), "");
        taskTypeName += "," + trigName;

        if (mxUtils::isElementExists(data_manager::mapTriggers, trigName))
        {
          auto& trg = data_manager::mapTriggers[trigName];

          state = "ACM: " + trg.getStringAttributeValue(mxconst::get_PROP_All_COND_MET_B(), mxconst::get_MX_FALSE()) + ", In Area: " + ((trg.bPlaneIsInTriggerArea) ? mxconst::get_MX_TRUE() : mxconst::get_MX_FALSE()) + ", SCM: " + ((trg.bScriptCondMet) ? mxconst::get_MX_TRUE() : mxconst::get_MX_FALSE()) + ", TE: " + (Timer::wasEnded(trg.timer) ? mxconst::get_MX_TRUE() : mxconst::get_MX_FALSE());
        }
        else
          state = "Trigger data is invalid. Contact the developer.";
      }
      break;
      case missionx::mx_task_type::script:
      {
        taskTypeName += "," + tsk.getStringAttributeValue(mxconst::get_ATTRIB_BASE_ON_SCRIPT(), "No Script Name");
        state = "ACM: " + tsk.getStringAttributeValue(mxconst::get_PROP_All_COND_MET_B(), mxconst::get_MX_FALSE()) + ", SCM: " + tsk.getStringAttributeValue(mxconst::get_PROP_SCRIPT_COND_MET_B(), mxconst::get_MX_FALSE());
      }
      break;
      case missionx::mx_task_type::placeholder:
      case missionx::mx_task_type::base_on_sling_load_plugin:
      case missionx::mx_task_type::base_on_external_plugin:
      {
        state = "ACM: " + tsk.getStringAttributeValue(mxconst::get_PROP_All_COND_MET_B(), mxconst::get_MX_FALSE());
      }
      break;
      case missionx::mx_task_type::base_on_command:
      {
        state = (tsk.command_ref.flag_wasClicked) ? "was clicked" : "not clicked";
      }
      break;
      default:
      {
        taskTypeName = "No Info";
        state        = "n/a";
      }
    } // end switch

    return taskTypeName + "): " + ((obj.mapTasks[inTaskName].getBoolValue(mxconst::get_ATTRIB_ENABLED(), true)) ? "Enabled" : "Disabled") + ": " + ((tsk.isMandatory) ? "Is Mandatory" : "Optional") + ". " + state;
  };


  const auto lmbda_print_task_info = [&](const int inTreeId_i, const std::string& taskName, std::string& tasksInfo)
  {
    auto& tsk = inObj.mapTasks[taskName];
    // tasksInfo = inObj.mapTasks[taskName].to_string_ui(); // v3.305.3 deprecated, we override it with the lmbda below

    tasksInfo = lmbda_task_state_info(inObj, taskName);

    // DISPLAY TREE NODE
    if (inObj.mapTasks[taskName].task_type == missionx::mx_task_type::trigger && ImGui::TreeNode(reinterpret_cast<void *> (static_cast<intptr_t> (inTreeId_i)), "%s", tasksInfo.c_str())) // trigger Tree node
    {
      auto taskBasedOnName = inObj.mapTasks[taskName].getStringAttributeValue(mxconst::get_ATTRIB_BASE_ON_TRIGGER(), "");
      if (mxUtils::isElementExists(data_manager::mapTriggers, taskBasedOnName))
      {
        auto& trg = data_manager::mapTriggers[taskBasedOnName];
        ImGui::TextColored(missionx::color::color_vec4_magenta, "%s", trg.to_string_ui_info_gist().c_str()); // v3.305.3
        ImGui::NewLine();
        ImGui::TextColored(missionx::color::color_vec4_beige, "%s", trg.to_string_ui_info().c_str());

        ImGui::TreePop();
      }
    }
    else if (inObj.mapTasks[taskName].task_type != missionx::mx_task_type::trigger)
      ImGui::TextColored(missionx::color::color_vec4_beige, "%s", tasksInfo.c_str());
  };

  std::string tasksInfo;

  std::set<std::string> taskUsedNamesSet;
  taskUsedNamesSet.clear();

  int treeId_i = 0;
  for (const auto& taskName : inObj.vecMandatoryTasks)
  {
    auto& tsk = inObj.mapTasks[taskName];
    tasksInfo = inObj.mapTasks[taskName].to_string_ui();
    tasksInfo = lmbda_task_state_info(inObj, taskName);
    //tasksInfo = "\t" + lmbda_task_state_info(inObj, taskName);

    lmbda_print_task_info(treeId_i, taskName, tasksInfo);

    taskUsedNamesSet.emplace(taskName);

    ++treeId_i;
  }

  ImGui::Spacing();
  ImGui::Separator();

  for (auto& taskName : inObj.vecNotDependentTasks)
  {
    if (taskUsedNamesSet.contains(taskName)) // skip already displayed tasks
      continue;

    auto& tsk = inObj.mapTasks[taskName];
    tasksInfo = inObj.mapTasks[taskName].to_string_ui();
    tasksInfo = lmbda_task_state_info(inObj, taskName);
    //tasksInfo = "\t" + lmbda_task_state_info(inObj, taskName);

    lmbda_print_task_info(treeId_i, taskName, tasksInfo);

    ++treeId_i;
  }
}

// ------------ print_triggers_ui_debug_info --------------
void
WinImguiBriefer::print_triggers_ui_debug_info()
{
  std::list<std::string>  lstExecutedTrigger;
  std::list<std::string>  lstNotExecutedTrigger;
  std::list<std::string>* lstPointer = nullptr;

  // Sort which script was executed and which not + color
  for (auto& KeyTrigName : data_manager::mapFlightLegs[data_manager::currentLegName].listTriggers)
  {
    auto& trig = missionx::data_manager::mapTriggers[KeyTrigName];
    if (trig.strct_debug.flagWasEvaluatedSuccessfully && (((!trig.strct_debug.sStartExecutionTime.empty()) + (trig.trigState > missionx::mx_trigger_state_enum::never_triggered && trig.trigState < missionx::mx_trigger_state_enum::wait_to_be_triggered_again)) > 0))
    {
      // if (trig.strct_debug.timePassed < missionx::color::BASE_DURATION_LL * 3) // put short term duration at the top
      //   lstExecutedTrigger.emplace_front(KeyTrigName);
      // else
      lstExecutedTrigger.emplace_back(KeyTrigName);
    }
    else
      lstNotExecutedTrigger.emplace_back(KeyTrigName);
  }

  ImGui::Spacing();
  ImGui::Separator();

  int treeId_i = 0;

  // -- loop over called list
  for (int i = 0; i < 2; ++i)
  {
    lstPointer = (i == 0) ? &lstExecutedTrigger : &lstNotExecutedTrigger;

    for (const auto& trigName : (*lstPointer))
    {
      if (trigName.empty())
        continue;


    auto const lmbda_get_trigger_header = [&]()
      {
        // calculate distance from plane if trigger type is RAD/Poly or Camera
        if (((data_manager::mapTriggers[trigName].getTriggerType().compare(mxconst::get_TRIG_TYPE_RAD()) == 0) + (data_manager::mapTriggers[trigName].getTriggerType().compare(mxconst::get_TRIG_TYPE_POLY()) == 0) + (data_manager::mapTriggers[trigName].getTriggerType().compare(mxconst::get_TRIG_TYPE_CAMERA()) == 0)) > 0)
        {
          missionx::Point planePos            = dataref_manager::getCurrentPlanePointLocation();
          const auto      distance_from_plane = Point::calcDistanceBetween2Points(planePos, data_manager::mapTriggers[trigName].pCenter);
          return " (" + mxUtils::formatNumber<double>(distance_from_plane, 2) + "nm)";
        }

        return std::string("");
      };



      auto &trigger = data_manager::mapTriggers[trigName];

      const std::string nodeText = trigger.get_string_debug_as_header() + lmbda_get_trigger_header() + ((!trigger.strct_debug.sStartExecutionTime.empty()) ? "(" + trigger.strct_debug.sStartExecutionTime + ">>" + trigger.strct_debug.sEndExecutionTime + ")" : "");


      ImGui::PushStyleColor(ImGuiCol_Text, trigger.strct_debug.color); // tree text color
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, missionx::color::color_vec4_darkorange);
      {
        if (ImGui::TreeNode(reinterpret_cast<void *> (static_cast<intptr_t> (treeId_i)), "%s", nodeText.c_str()))
        {
          // v3.305.3
          const std::string btnTriggerFireLbl = "Force Trigger When Fire##" + trigName;
          const std::string btnTriggerLeftLbl = "Force Trigger When Left##" + trigName;
          if (ImGui::Button(btnTriggerFireLbl.c_str()))
          {
            data_manager::mapTriggers[trigName].strct_debug.set_debug_state(missionx::mx_trigger_state_enum::never_triggered);
          }
          ImGui::SameLine(0.0f, 10.0f);
          if (ImGui::Button(btnTriggerLeftLbl.c_str()))
          {
            data_manager::mapTriggers[trigName].strct_debug.set_debug_state(missionx::mx_trigger_state_enum::inside_trigger_zone);
          }
          ImGui::Spacing();

          ImGui::TextColored(missionx::color::color_vec4_grey, "(%s)", "ACM: all conditions are met, SCM: script condition met, TE: timer ended, In Area: physical + elev");

          ImGui::TextColored(missionx::color::color_vec4_magenta, "%s", trigger.to_string_ui_info_gist().c_str()); // v3.305.3
          ImGui::TextColored(missionx::color::color_vec4_magenta, "%s", trigger.to_string_ui_info_conditions().c_str()); // v3.305.3
          ImGui::TextColored(missionx::color::color_vec4_yellowgreen, "Timing: %s>>%s", trigger.strct_debug.sStartExecutionTime.c_str(), trigger.strct_debug.sEndExecutionTime.c_str()); // v3.305.3
          ImGui::NewLine();
          ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_beige);
          {
            ImGui::TextWrapped("%s", data_manager::mapTriggers[trigName].to_string_ui_info().c_str());
          }
          ImGui::PopStyleColor();

          ImGui::TreePop(); // end tree
        }
      } // end Style for tree node header color
      ImGui::PopStyleColor(2);

      ImGui::Spacing();
      ImGui::Separator();

      ++treeId_i;
    } // end loop over all scripts

    if (i == 0 && !lstPointer->empty())
    {
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Separator();
    }


  } // end loop "i" over list pointer



  //for (auto& trigName : data_manager::mapFlightLNav Inegs[data_manager::currentLegName].listTriggers)
  //{
  //  if (trigName.empty())
  //    continue;
  //
  //  // v3.305.3
  //  auto const lmbda_get_trigger_header = [&]()
  //  {
  //    // calculate distance from plane if trigger type is RAD/Poly or Camera
  //    if (((data_manager::mapTriggers[trigName].getTriggerType().compare(mxconst::get_TRIG_TYPE_RAD()) == 0) + (data_manager::mapTriggers[trigName].getTriggerType().compare(mxconst::get_TRIG_TYPE_POLY()) == 0) + (data_manager::mapTriggers[trigName].getTriggerType().compare(mxconst::get_TRIG_TYPE_CAMERA()) == 0)) > 0)
  //    {
  //      missionx::Point planePos            = dataref_manager::getCurrentPlanePointLocation();
  //      const auto      distance_from_plane = Point::calcDistanceBetween2Points(planePos, data_manager::mapTriggers[trigName].pCenter);
  //      return " (" + mxUtils::formatNumber<double>(distance_from_plane, 2) + "nm)";
  //    }
  //
  //    return std::string("");
  //  };
  //
  //  const std::string nodeText = data_manager::mapTriggers[trigName].get_string_debug_as_header() + lmbda_get_trigger_header();
  //
  //  if (ImGui::TreeNode((void*)(intptr_t)treeId_i, "%s", nodeText.c_str())) // v3.305.3 fixed tree node
  //  {
  //    // v3.305.3
  //    const std::string btnTriggerFireLbl = "Force Trigger When Fire##" + trigName;
  //    const std::string btnTriggerLeftLbl = "Force Trigger When Left##" + trigName;
  //    ImGui::TextColored(missionx::color::color_vec4_grey, "%s", "X-Plane should not be in PAUSE state.\nUse the debug buttons at your own risk !!!");
  //    if (ImGui::Button(btnTriggerFireLbl.c_str()))
  //    {
  //      data_manager::mapTriggers[trigName].strct_debug.set_debug_state(missionx::mx_trigger_state_enum::never_triggered);
  //    }
  //    ImGui::SameLine(0.0f, 10.0f);
  //    if (ImGui::Button(btnTriggerLeftLbl.c_str()))
  //    {
  //      data_manager::mapTriggers[trigName].strct_debug.set_debug_state(missionx::mx_trigger_state_enum::inside_trigger_zone);
  //    }
  //    ImGui::Spacing();
  //
  //    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_beige);
  //    {
  //      ImGui::TextWrapped("%s", data_manager::mapTriggers[trigName].to_string_ui_info().c_str());
  //    }
  //    ImGui::PopStyleColor();
  //
  //    ImGui::TreePop(); // end tree
  //  }
  //
  //  ++treeId_i; // v3.305.3
  //
  //  ImGui::Separator();
  //} // end loop over all triggers


}

// ------------ print_datarefs_ui_debug_info --------------

void
WinImguiBriefer::print_datarefs_ui_debug_info()
{
  for (auto& [name, dref] : missionx::data_manager::mapDref)
  {
    // ImGui::TextColored(missionx::color::color_vec4_bisque, "%s", dref.to_string_ui_info().c_str());

    ImGui::TextColored(missionx::color::color_vec4_bisque, "%s: ", dref.key.c_str());
    ImGui::SameLine();
    {
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_beige);
      {
        ImGui::TextWrapped("%s", dref.to_string_ui_info_value_only().c_str());
      }
      ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
  } // end dref loop

}

// ------------ print_globals_ui_debug_info --------------

void
WinImguiBriefer::print_globals_ui_debug_info()
{
  static std::string keyPicked, valPicked;
  static char        typePicked = '\0';
  int         seq        = 0;
  std::string lbl;


  const auto lmbda_prepare_globalbuff = [&]() {
#ifdef IBM
    strncpy_s(strct_flight_leg_info.online_globals_buff, strct_flight_leg_info.DEBUG_GLOBALS_BUFF_SIZE_RSIZET - 2, valPicked.c_str(), valPicked.length());
#else
    strncpy(strct_flight_leg_info.online_globals_buff, valPicked.c_str(), ((valPicked.length() < strct_flight_leg_info.DEBUG_GLOBALS_BUFF_SIZE_RSIZET) ? valPicked.length() : strct_flight_leg_info.DEBUG_GLOBALS_BUFF_SIZE_RSIZET - 1));
#endif

  };



  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
  ImGui::TextColored(missionx::color::color_vec4_aqua, "%s", "Bool:");
  this->mxUiReleaseLastFont();

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_SMALL());
  for (auto& [key, val] : missionx::script_manager::mapScriptGlobalBoolArg)
  {
    lbl = fmt::format("...##ModifyGlobal{}", ++seq);
    if ( ImGui::SmallButton(lbl.c_str()) )
    {
      keyPicked = key;
      valPicked = (val) ? "true" : "false";
      typePicked = 'b';

      lmbda_prepare_globalbuff();
      ImGui::OpenPopup(this->POPUP_ONLINE_GLOBALS_EDIT.c_str());
    }
    ImGui::SameLine();
    ImGui::TextColored(missionx::color::color_vec4_bisque, "%s", key.c_str());
    ImGui::SameLine();
    ImGui::TextColored(missionx::color::color_vec4_beige, "%s", (val) ? "true" : "false");
  }
  this->mxUiReleaseLastFont();

  ImGui::Separator();
  ImGui::Spacing();
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
  ImGui::TextColored(missionx::color::color_vec4_aqua, "%s", "Numbers:");
  this->mxUiReleaseLastFont();

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_SMALL());
  for (auto & [key, val] : missionx::script_manager::mapScriptGlobalDecimalArg)
  {
    lbl = fmt::format("...##ModifyGlobal{}", ++seq);
    if (ImGui::SmallButton(lbl.c_str()))
    {
      keyPicked  = key;
      valPicked  = fmt::format("{}", val);
      typePicked = 'n';

      lmbda_prepare_globalbuff();
      ImGui::OpenPopup(this->POPUP_ONLINE_GLOBALS_EDIT.c_str());
    }
    ImGui::SameLine();
    ImGui::TextColored(missionx::color::color_vec4_bisque, "%s", key.c_str());
    ImGui::SameLine();
    ImGui::TextColored(missionx::color::color_vec4_beige, "%.6f", val);
  }
  this->mxUiReleaseLastFont();


  ImGui::Separator();
  ImGui::Spacing();
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
  ImGui::TextColored(missionx::color::color_vec4_aqua, "%s", "Strings:");
  this->mxUiReleaseLastFont();

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_SMALL());
  for (auto & [key, val] : missionx::script_manager::mapScriptGlobalStringsArg)
  {
    lbl = fmt::format("...##ModifyGlobal{}", ++seq);
    if (ImGui::SmallButton(lbl.c_str()))
    {
      keyPicked  = key;
      valPicked  = val;
      typePicked = 's';

      lmbda_prepare_globalbuff();
      ImGui::OpenPopup(this->POPUP_ONLINE_GLOBALS_EDIT.c_str());
    }
    ImGui::SameLine();
    ImGui::TextColored(missionx::color::color_vec4_bisque, "%s", key.c_str());
    ImGui::SameLine();
    ImGui::TextColored(missionx::color::color_vec4_beige, "%s", val.c_str());
  }
  this->mxUiReleaseLastFont();

  this->draw_globals_online_edit_popup(this->POPUP_ONLINE_GLOBALS_EDIT, typePicked, keyPicked, valPicked);


}

// ------------ print_scripts_ui_debug_info --------------

void
WinImguiBriefer::print_scripts_ui_debug_info()
{
  bool bSaveFlag = false;
  int  treeId_i  = 0;

  std::list<std::string>  lstExecutedScripts;
  std::list<std::string>        lstNotExecutedScripts;
  const std::list<std::string>* lstPointer = nullptr;

  // Sort which script was executed and which not + color
  for (auto& [key, script] : missionx::script_manager::mapScripts)
  {
    if (((script.flagWasCalled) + (!script.sExecutionTime.empty())) > 0)
    {
        lstExecutedScripts.emplace_back(key);
    }
    else
      lstNotExecutedScripts.emplace_back(key);

  }


  // -- loop over called list
  for (int i = 0; i < 2; ++i)
  {
    lstPointer = (i == 0) ? &lstExecutedScripts : &lstNotExecutedScripts;

    for (const auto& name : (*lstPointer))
    {
      const auto script = &missionx::script_manager::mapScripts[name];
      ImGui::PushStyleColor(ImGuiCol_Text, script->color); // tree text color
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, missionx::color::color_vec4_darkorange);
      {
        if (ImGui::TreeNode(reinterpret_cast<void *> (static_cast<intptr_t> (treeId_i)), "%s", script->to_string_debug().c_str()))
        {
          const auto size = script->script_body.length(); // debug
          if (!script->script_body.empty() && (script->script_body.length() < missionx::WinImguiBriefer::mx_flight_leg_info_layer::DEBUG_BUFF_SIZE_RSIZET - 1) )
          {
            ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_white); // tree text color
            if ( ImGui::Button(std::string("Edit In Memory##btn").append(mxUtils::formatNumber<int>(treeId_i)).c_str()))
            {

                for (int i_count = 0; i_count < missionx::WinImguiBriefer::mx_flight_leg_info_layer::DEBUG_BUFF_SIZE_RSIZET - 1; ++i_count)
                    this->strct_flight_leg_info.online_debug_buff[i_count] = '\0';

                #ifdef IBM
                strncpy_s(strct_flight_leg_info.online_debug_buff, strct_flight_leg_info.DEBUG_BUFF_SIZE_RSIZET - 2, script->script_body.c_str(), script->script_body.length());
                #else
                strncpy(strct_flight_leg_info.online_debug_buff
                        , script->script_body.c_str()
                        , ((script->script_body.length() < missionx::WinImguiBriefer::mx_flight_leg_info_layer::DEBUG_BUFF_SIZE_RSIZET)? script->script_body.length():missionx::WinImguiBriefer::mx_flight_leg_info_layer::DEBUG_BUFF_SIZE_RSIZET-1));
                #endif
                const size_t pos                             = (script->script_body.length() < missionx::WinImguiBriefer::mx_flight_leg_info_layer::DEBUG_BUFF_SIZE_RSIZET - 2) ? script->script_body.length() : missionx::WinImguiBriefer::mx_flight_leg_info_layer::DEBUG_BUFF_SIZE_RSIZET - 1;
                strct_flight_leg_info.online_debug_buff[pos] = '\0'; // add NULL
                strct_flight_leg_info.online_debug_buff_size = sizeof(*strct_flight_leg_info.online_debug_buff);

                this->strct_flight_leg_info.scriptNameToEdit = name;
                ImGui::OpenPopup(this->POPUP_ONLINE_SCRIPT_EDIT.c_str());
            }
            ImGui::PopStyleColor();
          }
          else if (!script->script_body.empty() )
            ImGui::TextColored(missionx::color::color_vec4_azure, "%s '%lu': %lu", "Script code length is longer than ", missionx::WinImguiBriefer::mx_flight_leg_info_layer::DEBUG_BUFF_SIZE_RSIZET-2, script->script_body.length()); // debug

          if (!script->script_is_valid)
          {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_white);
            ImGui::TextWrapped("Error:\n%s", script->err_desc.c_str());
            ImGui::PopStyleColor();
          }

          ImGui::TextColored(missionx::color::color_vec4_azure, "%s", "Script Code:");


          const std::string childId = mxUtils::formatNumber<int>(treeId_i) + "##ScriptBody";
          ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_white); // internal text
          ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_mx_bluish);
          {
            ImGui::BeginChild(childId.c_str(), ImVec2(0.0f, ((script->script_body.size() > static_cast<size_t> (500)) ? 180.0f : 120.0f)), ImGuiChildFlags_Borders); //, ImGuiWindowFlags_AlwaysHorizontalScrollbar); // Sub Window
            ImGui::TextWrapped("%s", script->script_body.c_str());
            ImGui::EndChild();
          }
          ImGui::PopStyleColor(2);

          this->draw_script_online_edit_popup(this->POPUP_ONLINE_SCRIPT_EDIT, bSaveFlag);
          if (bSaveFlag)
          {
            script->script_body = std::string(this->strct_flight_leg_info.online_debug_buff);
            script->script_is_valid = true; // reset valid state
            script->err_desc.clear();
            this->strct_flight_leg_info.online_debug_buff[0] = '\0';
          }


          ImGui::TreePop(); // end tree
        }
      } // end Style for tree node header color
      ImGui::PopStyleColor(2);

      ImGui::Spacing();
      ImGui::Separator();

      ++treeId_i;
    } // end loop over all scripts

    if (i == 0 && !lstPointer->empty())
    {
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Separator();

    }


  } // end loop "i" over list pointer

}

// -------------------------------------------

void
WinImguiBriefer::print_interpolated_ui_debug_info()
{

  for (auto &dref : missionx::data_manager::mapInterpolDatarefs | std::views::values)
  {
    // ImGui::TextColored(missionx::color::color_vec4_bisque, "%s", dref.to_string_ui_info().c_str());
    auto        lastSlashPos = dref.key.find_last_of('/');
    std::string sName        = (lastSlashPos != std::string::npos) ? dref.key.substr(lastSlashPos + 1) : dref.key;

    ImGui::TextColored(missionx::color::color_vec4_bisque,
                       "%s",
                       fmt::format("{:.<40}, Cycles: {}/{}, Time Passed: {:.2f} sec", sName + ((dref.strctInterData.flagIsValid) ? "" : " (Inactive)") , dref.strctInterData.currentCycleCounter_i, dref.strctInterData.for_how_many_cycles_i, dref.strctInterData.timerToRun.getSecondsPassed(), dref.strctInterData.sTargetValues).c_str());
    ImGui::TextColored(missionx::color::color_vec4_grey, "%s", fmt::format("Target: {} | Start: {} | Delta: {}", dref.strctInterData.sTargetValues, dref.strctInterData.sStartValues, dref.strctInterData.getDelta(dref.dataRefType)).c_str());
    // print current value of dataref
    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_beige);
      ImGui::TextWrapped("%s", dref.to_string_ui_info_value_only().c_str());
    ImGui::PopStyleColor();


    ImGui::Spacing();
    ImGui::Separator();
  } // end dref loop
}

// -------------------------------------------

void
WinImguiBriefer::print_messages_ui_debug_info()
{
  int counter = 0;
  for (auto &msg : missionx::data_manager::mapMessages | std::views::values)
  {
    bool bIsStoryMode = (msg.mode == missionx::mx_msg_mode::mode_story);
    missionx::WinImguiBriefer::HelpMarker("Use at your own risk.");
    ImGui::SameLine();
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
    if (this->ButtonTooltip(fmt::format("{}{}{}", Utils::from_u8string(ICON_FA_PLAY), "##PlayMessage", counter++).c_str(), "Play Message"))
    {
      if (QueueMessageManager::is_queue_empty())
      {
        std::string err;
        auto        clonedMsg = msg;
        if (clonedMsg.parse_node(true))
          QueueMessageManager::addMessage(clonedMsg, missionx::EMPTY_STRING, err);
        else
          this->setMessage("Failed to parse message: " + msg.getName(), 6);
      }
      else
      {
        this->setMessage(fmt::format("The message Queue is not empty. There are: {} in queue.", QueueMessageManager::listPoolMsg.size() + QueueMessageManager::listPadQueueMessages.size()), 6);
      }

    }
    this->mxUiReleaseLastFont();

    ImGui::SameLine();
    ImGui::TextColored(((bIsStoryMode)?missionx::color::color_vec4_aqua : missionx::color::color_vec4_white), "%s", msg.name.c_str());

    ImGui::Spacing();
    ImGui::Separator();

  } // end messages loop

}

// -------------------------------------------

void
WinImguiBriefer::add_ui_skip_abort_setup_checkbox()
{
  // v3.305.3 skip abort script on script error
  missionx::WinImguiBriefer::HelpMarker("Ignore script errors and do not abort the mission.\nThis can allow you to fix it in the \"debug|script\" tab.");
  ImGui::SameLine();
  ImGui::Checkbox("Ignore \"script errors\" - do not abort the mission on compilation ERROR.", &missionx::data_manager::flag_setupSkipMissionAbortIfScriptIsInvalid);
}

// -------------------------------------------

void
WinImguiBriefer::add_designer_mode_checkbox()
{
  // v24.03.2 Designer mode flag
  missionx::WinImguiBriefer::HelpMarker(
    "Enable Designer mode from the setup screen instead from the XML file. The XML file has precedence over this setting, and it will update it when loading a mission.\nMakes the option flexiable during mission tests.\nIn debug binaries it is set to 'true' by default.\nUsage: Affects behaviour of 'force_leg_name' in the global_settings.");
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_aqua);
  ImGui::Checkbox("Enable \"Designer Mode\"", &missionx::data_manager::flag_setupEnableDesignerMode);
  ImGui::PopStyleColor();
}


// -------------------------------------------


void
WinImguiBriefer::add_ui_xp11_comp_checkbox (const bool &inStorePreference)
{
  // v24.12.2 compatibility flag
  missionx::WinImguiBriefer::HelpMarker ("Decide if inventory layout will be compatible with X-Plane 11 or 12. XP12 supports \"station\" locations inside acf. XP11 - one big container, payload distribute evenly.\n\nMight be ignored, if designer force specific inventory layout in the template file.");
  ImGui::SameLine ();
  ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_aqua);
  if (ImGui::Checkbox ("XP11 Inventory Layout", &missionx::data_manager::flag_setupUseXP11InventoryUI))
  {
    // ADD set option value
    if (inStorePreference)
    {
      missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool> (mxconst::SETUP_USE_XP11_INV_LAYOUT, missionx::data_manager::flag_setupUseXP11InventoryUI);
      this->execAction (missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
    }
  }
  ImGui::PopStyleColor ();
}

// -------------------------------------------

void
WinImguiBriefer::add_ui_simbrief_pilot_id ()
{
  const auto lmbda_save_simbrief_settings =[&]()
  {
    Utils::xml_search_and_set_node_text(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_SIMBRIEF_PILOT_ID(), std::string(this->strct_setup_layer.buf_simbrief_pilot_id), this->mxcode.STRING, true);
    this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
  };

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
  {
    missionx::WinImguiBriefer::HelpMarker ("Stores your Simbrief Pilot ID, so we could fetch your last flight plan.\nCan be found in your 'Account Settings' menu");
    ImGui::SameLine ();
    ImGui::TextColored (missionx::color::color_vec4_turquoise, "Your Simbrief pilot ID:");
    ImGui::SameLine ();
    ImGui::SetNextItemWidth (120.0f);
    if (ImGui::InputTextWithHint ("##SimbriefPilotID", "Simbrief Pilot ID", this->strct_setup_layer.buf_simbrief_pilot_id, static_cast<int> (sizeof (this->strct_setup_layer.buf_simbrief_pilot_id)), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue))
    {
      lmbda_save_simbrief_settings ();
    }
    ImGui::SameLine ();
    if (ImGui::Button ("Save##SaveSimBriefButton"))
    {
      lmbda_save_simbrief_settings ();
    }
  }
  this->mxUiReleaseLastFont ();
}


// -------------------------------------------


void
WinImguiBriefer::add_ui_flightplandb_key (const bool isPopup)
{
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG());
  {
    ImGui::TextColored (missionx::color::color_vec4_greenyellow, "flightplandatabase.com:");
    ImGui::TextColored (missionx::color::color_vec4_turquoise, "Enter authorization key (if you have one):");
    ImGui::SetNextItemWidth (300.0f);
    ImGui::InputText ("##AuthorizationInputText", this->strct_ext_layer.buf_authorization, sizeof (this->strct_ext_layer.buf_authorization) - 1);


    if (isPopup)
    {
      ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_darkgrey);
      ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::NewLine ();
      ImGui::Separator ();
    }
    else
      ImGui::SameLine ();

    {
      this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
      if (ImGui::Button ("Save", ImVec2 (60, 0)))
      {
        if (isPopup)
          ImGui::CloseCurrentPopup ();

        Utils::xml_search_and_set_node_text (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_AUTHORIZATION_KEY(), std::string (this->strct_ext_layer.buf_authorization), this->mxcode.STRING, true);
        this->execAction (missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);

        if (mxUtils::trim (std::string (this->strct_ext_layer.buf_authorization)).empty ())
          this->strct_ext_layer.flag_flightplandatabase_auth_exists = false;
        else
          this->strct_ext_layer.flag_flightplandatabase_auth_exists = true;
      }
      this->mxUiReleaseLastFont ();
    }
    if (isPopup)
      ImGui::PopStyleColor (2); // Release two button style settings
  }
  this->mxUiReleaseLastFont ();
}

// -------------------------------------------


void
WinImguiBriefer::add_ui_pick_subcategories (const std::vector<const char *> &vecToDisplay)
{
  // iMissionSubCategoryPicked should always be smaller than the vecToDisplay.size () because the numbering starts in "0"
  if (static_cast<int> (vecToDisplay.size ()) <= this->strct_user_create_layer.iMissionSubCategoryPicked)
    this->strct_user_create_layer.iMissionSubCategoryPicked = 0;

  missionx::WinImguiBriefer::HelpMarker ("Pick one of the sub categories");
  ImGui::SameLine ();
  // auto vecToDisplay = this->mapMissionCategories[this->strct_user_create_layer.iRadioMissionTypePicked];
  ImGui::SetNextItemWidth (250.0f);
  ImGui::Combo ("##mission_subcategory", &this->strct_user_create_layer.iMissionSubCategoryPicked, vecToDisplay.data (), static_cast<int> (vecToDisplay.size ()));
  // always store the subcategory value - NOT FPS friendly
  missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_MISSION_SUBCATEGORY_LBL (), vecToDisplay.at (this->strct_user_create_layer.iMissionSubCategoryPicked));
}

// -------------------------------------------

void
WinImguiBriefer::add_ui_auto_load_checkbox (const missionx::mx_window_actions &inActionToExecute)
{
  ImGui::Spacing ();
  ImGui::SameLine (0.0f, 10.0f);
  missionx::WinImguiBriefer::HelpMarker("On mission start, the GPS waypoints will be loaded.\nThe setting will be saved into the preference file.", missionx::color::color_vec4_aqua);
  ImGui::SameLine();
  if (ImGui::Checkbox ("Auto Load Route.\n(not advisable for FMS)", &this->strct_cross_layer_properties.flag_auto_load_route_to_gps_or_fms) ) // v25.04.2
  {
    // ADD set option value
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_PROP_AUTO_LOAD_ROUTE_TO_GPS_OR_FMS_B(), this->strct_cross_layer_properties.flag_auto_load_route_to_gps_or_fms);
    this->execAction(inActionToExecute);
  }
}

// -------------------------------------------


void WinImguiBriefer::callNavData(std::string_view inICAO, bool bNavigatingFromOtherLayer) // v24.03.1
{
  if (this->strct_ils_layer.fetch_nav_state != missionx::mxFetchState_enum::fetch_in_process)
  {
    this->strct_ils_layer.sNavICAO = inICAO;

    auto size = sizeof this->strct_ils_layer.buf1;
#ifdef IBM
    strncpy_s(this->strct_ils_layer.buf1, sizeof this->strct_ils_layer.buf1, inICAO.data(), sizeof this->strct_ils_layer.buf1);
#else
    strncpy(this->strct_ils_layer.buf1, inICAO.data(), sizeof this->strct_ils_layer.buf1);
#endif
    this->strct_ils_layer.flagForceNavDataTab = true;

    this->strct_ils_layer.fetch_nav_state = missionx::mxFetchState_enum::fetch_not_started;
    this->execAction(mx_window_actions::ACTION_FETCH_NAV_INFORMATION);

    if (bNavigatingFromOtherLayer)
      this->execAction(missionx::mx_window_actions::ACTION_OPEN_NAV_LAYER);
  }
  else
    this->setMessage("A previous request is running in the background.");

}


// ------------ setMessage --------------
void
WinImguiBriefer::setMessage (const std::string &inMsg, int secToDisplayMessage)
{
  this->sBottomMessage = inMsg;
  missionx::Timer::start(this->timerMessage, static_cast<float> (secToDisplayMessage));
}

// -------------------------------------------

void
WinImguiBriefer::clearMessage()
{
  this->sBottomMessage.clear();
  missionx::data_manager::set_found_missing_3D_object_files(false); // v3.0.255.3 reset missing 3D files state
}

// ------------ initFlightLegChange --------------

void
WinImguiBriefer::refreshNewMapsAndImages(missionx::Waypoint& inLeg)
{
  // for (auto& [mapName, mapNode] : data_manager::mapFlightLegs[data_manager::currentLegName].map2DMapsNodes) // v3.0.241.7.1
  for (auto& [mapName, mapNode] : inLeg.map2DMapsNodes)
  {
    if (mxUtils::isElementExists(data_manager::mapCurrentMissionTextures, mapName) == false) // if map keyName does not exists
    {
      missionx::data_manager::strct_flight_leg_info_totalMapsCounter += 1;
      Utils::addElementToMap(missionx::data_manager::maps2d_to_display, missionx::data_manager::strct_flight_leg_info_totalMapsCounter, data_manager::mapCurrentMissionTextures[mapName]);
    }
  }
}

void
WinImguiBriefer::initFlightLegChange()
{
  this->strct_flight_leg_info.mapLayerInit();
  missionx::data_manager::maps2d_to_display.clear(); // v3.0.303.5 reset maps to display

  // initialize Flight Leg Maps
  if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running || data_manager::missionState == missionx::mx_mission_state_enum::mission_loaded_from_the_original_file || data_manager::missionState == missionx::mx_mission_state_enum::mission_loaded_from_savepoint)
  {
    // initialize maps vecTextures from flight Leg
    for (auto& [mapName, mapNode] : data_manager::mapFlightLegs[data_manager::currentLegName].map2DMapsNodes) // v3.0.241.7.1
    {
      if (mxUtils::isElementExists(data_manager::mapCurrentMissionTextures, mapName)) // if map keyName exists = m.first
      {
        missionx::data_manager::strct_flight_leg_info_totalMapsCounter += 1;
        Utils::addElementToMap(missionx::data_manager::maps2d_to_display, missionx::data_manager::strct_flight_leg_info_totalMapsCounter, data_manager::mapCurrentMissionTextures[mapName]);
      }
    }

    // Set Layer
    setLayer(uiLayer_enum::flight_leg_info);
    this->strct_flight_leg_info.resetChildLayer();

    if (!missionx::data_manager::maps2d_to_display.empty())
      this->strct_flight_leg_info.iMapNumberToDisplay = 1;
  }
  else if (data_manager::missionState > missionx::mx_mission_state_enum::mission_is_running)
  {
    setLayer(uiLayer_enum::flight_leg_info);
    this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info_end_summary; // DISPLAY End Layer: Success or Failure
  }
}

// -------------------------------------------
// ------------ draw Top title layer
// -------------------------------------------
void
WinImguiBriefer::draw_top_toolbar()
{
  const float win_width = ImGui::GetWindowWidth();
  ImGui::BeginChild("draw_top_toolbar_01", this->imvec2_top_toolbar_size);
  {
    ///////////////// TITLE AREA //////////////////
    // If we are a transparent HUD-like window then we draw 3 lines that look
    // a bit like a head...so people know where to drag the window to move it

    if (HasWindowDragArea())
    {
      ImGui::SameLine();
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      ImVec2      pos_start = ImGui::GetCursorPos();
      float       x_end     = mxUiGetContentWidth() - 75;
      for (int i = 0; i < 3; i++)
      {
        draw_list->AddLine(pos_start, { x_end, pos_start.y }, IM_COL32(0xa0, 0xa0, 0xa0, 255), 1.0f);
        pos_start.y += 5;
      }
    }

    // Button with fixed width 30 and standard height
    // to pop out the window in an OS window
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());

    static float btnWidth  = ImGui::CalcTextSize(mxUtils::from_u8string(ICON_FA_WINDOW_MAXIMIZE).c_str()).x + 5;
    const bool   bBtnPopIn = IsPoppedOut(); // || IsInVR();
    const bool   bBtnVR    = missionx::mxvr::vr_display_missionx_in_vr_mode && !IsInVR();
    int          numBtn    = bBtnPopIn + bBtnVR;

    if (numBtn > 0)
    {
      // Setup colors for window sizing buttons
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive));    // dark gray
      ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);                              // transparent
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_ScrollbarGrab)); // lighter gray

      if (bBtnVR)
      {
        // Same line, but right-alinged
        ImGui::SameLine(mxUiGetContentWidth() - (numBtn * btnWidth));
        if (this->ButtonTooltip(mxUtils::from_u8string(ICON_FA_EXTERNAL_LINK_SQUARE_ALT).c_str(), "Move into VR"))
          nextWinPosMode = xplm_WindowVR;
        --numBtn;
      }
      if (bBtnPopIn)
      {
        // Same line, but right-alinged
        ImGui::SameLine(mxUiGetContentWidth() - (numBtn * btnWidth));
        if (this->ButtonTooltip(mxUtils::from_u8string(ICON_FA_WINDOW_RESTORE).c_str(), "Move back into X-Plane"))
          nextWinPosMode = xplm_WindowPositionFree;
        --numBtn;
      }

      // Restore colors
      ImGui::PopStyleColor(3);

    } // end drawing popout/in buttons

    this->mxUiReleaseLastFont();

    ///////// SETUP BUTTON
    if (this->currentLayer != missionx::uiLayer_enum::option_setup_layer && this->currentLayer != missionx::uiLayer_enum::imgui_home_layer
        && !this->strct_ils_layer.flagNavigatedFromOtherLayer)
    {
      ImGui::SameLine(0.01f); // v3.0.253.9.1 we set to 0.01 since 0.0f do not display in popout window

      if (ImGui::ImageButton("SetupButtonImage", (void*)static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_TOOLBAR_SETUP_64x64()].gTexture), this->vec2_sizeTopBtn)) //
      {
        this->setLayer(missionx::uiLayer_enum::option_setup_layer);
      }
      this->mx_add_tooltip(missionx::color::color_vec4_white, "Setup Screen");
    }
    else if ((this->currentLayer == missionx::uiLayer_enum::option_setup_layer && this->currentLayer != this->prevBrieferLayer) || this->strct_ils_layer.flagNavigatedFromOtherLayer)
    {
      ImGui::SameLine(0.01f); // v3.0.253.9.1 we set to 0.01 since 0.0f do not display in popout window // should be beginning of toolbar

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
      if (ImgWindow::ButtonTooltip(mxUtils::from_u8string(ICON_FA_REPLY).append( "##BackButtonInSetup").c_str(), "Back", IM_COL32(255, 255, 0, 255), IM_COL32(0, 0, 0, 255), ImVec2(32.0f, 32.0f))) //
      {
        this->strct_ils_layer.flagNavigatedFromOtherLayer = false; // v24025
        // change to last prev layer
        this->setLayer(this->prevBrieferLayer);
        this->strct_setup_layer.is_first_time = true; // v3.0.253.6 force re-read of first time values like "overpass url" from options map
      }
      this->mxUiReleaseLastFont();

      // tooltip
      this->mx_add_tooltip(missionx::color::color_vec4_white, "Back to previous screen");
    }

    ///////// Title in the TOP Toolbar

    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_TOOLBAR());
    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_aqua); //
    ImGui::SameLine(0.0f, 10.0f);
    switch (this->currentLayer)
    {
      case missionx::uiLayer_enum::imgui_home_layer:
      {
        ImGui::Text("HOME");
      }
      break;
      case missionx::uiLayer_enum::option_generate_mission_from_a_template_layer:
      {
        ImGui::Text("Generate Custom Mission");
      }
      break;
      case missionx::uiLayer_enum::option_mission_list:
      {
        ImGui::Text("Pick an Adventure to Fly");
      }
      break;
      case missionx::uiLayer_enum::option_user_generates_a_mission_layer:
      {
        ImGui::Text("User Create Random Mission");
      }
      break;
      case missionx::uiLayer_enum::option_setup_layer:
      {
        ImGui::Text("Setup");
      }
      break;
      case missionx::uiLayer_enum::option_ils_layer:
      {
        ImGui::Text("Search Airports with ILS/Nav info");
      }
      break;
      case missionx::uiLayer_enum::option_external_fpln_layer:
      {
        ImGui::Text("Search Flight Plans");
        //ImGui::SameLine(0.0f, 270.0f);
        //ImGui::Text("flightplandatabase.com");

        // ImGui::SameLine((win_width / 2.0f) - 70.0f);
        // const std::string_view popupWindowName = "Authorization Key";
        //
        // this->mxUiReleaseLastFont(); // we release the font so we could use a different one later on
        //
        // if (this->strct_ext_layer.flag_flightplandatabase_auth_exists)
        //     ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_deepskyblue); // v24.06.1 set icon color if we have auth
        // else
        //     ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_black);    // set default icon color
        //
        // ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_antiquewhite); // set hover color
        //
        // this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
        // if (ImgWindow::ButtonTooltip(mxUtils::from_u8string(ICON_FA_USER_LOCK).c_str(), popupWindowName.data()))
        // {
        //   ImGui::OpenPopup(popupWindowName.data());
        // }
        // this->mxUiReleaseLastFont();
        //
        // ImGui::PopStyleColor(2);
        //
        // this->mx_add_tooltip(missionx::color::color_vec4_white, "!!! Optional !!!\nEnter Authorization Key from flightplandatabase.com\nValid authorization key should grant you ~1500 requests per 24 hours.\nIf you do not have one than you are limited to 100 requests per 24 hours.");
        //
        // this->popup_draw_authorization_key(popupWindowName);
      }
      break;
      default:
        break;
    }
    ImGui::PopStyleColor(1);
    ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);

    this->mxUiReleaseLastFont();

    ///////// HOME BUTTON - center
    ImGui::SameLine((win_width / 2.0f) - (vec2_sizeTopBtn.x / 2.0f));
    if (ImGui::ImageButton("HomeButtonImage", (void*)static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_HOME()].gTexture), this->vec2_sizeTopBtn)) // Home Button (will navigate dependent of the
    {
      this->strct_generate_template_layer.user_pick_from_replaceOptions_combo_i = mxconst::INT_UNDEFINED; // v3.0.255.4 reset user pick
      this->strct_ils_layer.flagNavigatedFromOtherLayer                         = false; // v24025
      this->strct_ext_layer.ext_screen                                          = mx_ext_fpln_screen::ext_home; // v25.03.3 reset external fpln to main screen

      switch (this->currentLayer)
      {
        case missionx::uiLayer_enum::uiLayerUnset:
        case missionx::uiLayer_enum::imgui_home_layer:
        {
          // do nothing
        }
        break;
        default:
        {
          // navigate main page
          this->setLayer(missionx::uiLayer_enum::imgui_home_layer);
          missionx::data_manager::set_found_missing_3D_object_files(false); // v3.0.255.3 reset missing 3D files state
        }
        break;
      } // end switch
    }   // end HOME button
    // tooltip
    this->mx_add_tooltip(missionx::color::color_vec4_white, "Home Button");

    // Failure Timer at toolbar level
    if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
    {
      ImGui::SameLine(0.0f, 20.0f);
      const std::string title_s = (missionx::data_manager::formated_fail_timer_as_text.empty()) ? "" : missionx::data_manager::formated_fail_timer_as_text;
      if (!title_s.empty())
      {
        ImGui::TextColored(this->countdown_textColorVec4, "Countdown: %s", title_s.c_str());
      }
    }

    ///////// ABOUT BUTTON
    if (this->currentLayer == missionx::uiLayer_enum::imgui_home_layer)
    {
      ImGui::SameLine(mxUiGetContentWidth() - 60.0f);
      if (ImGui::ImageButton("AboutButtonImage", (void*)static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_ABOUT_64x64()].gTexture), this->vec2_sizeTopBtn)) //
      {
        this->setLayer(missionx::uiLayer_enum::about_layer);
      }
      this->mx_add_tooltip(missionx::color::color_vec4_white, "About Mission-X");
    }

    if (data_manager::missionState > missionx::mx_mission_state_enum::mission_loaded_from_savepoint)
    {
      constexpr std::string_view popupWindowName = "QuitMission?";

      if (this->currentLayer != missionx::uiLayer_enum::imgui_home_layer && this->currentLayer != missionx::uiLayer_enum::about_layer) // v3.0.253.2 display QUIT in all layers except HOME layer. This to not conflict with the "about" button
      {
        ImGui::SameLine(mxUiGetContentWidth() - 60.0f);
        if (ImGui::ImageButton("QuitButtonImage", (void*)static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_QUIT_48x48()].gTexture), this->vec2_sizeTopBtn)) // QUIT Button (will navigate dependent of the
        {
          ImGui::OpenPopup(popupWindowName.data());
        }
        this->mx_add_tooltip(missionx::color::color_vec4_white, "Quit current running mission.");
      }

      popup_draw_quit_mission(popupWindowName);
    }
  }
  ImGui::EndChild();
}


// ------------ popup_draw_quit_mission

void
WinImguiBriefer::popup_draw_quit_mission (std::string_view inPopupWindowName)
{
  ImVec2 center (ImGui::GetIO ().DisplaySize.x * 0.5f, ImGui::GetIO ().DisplaySize.y * 0.5f);
  ImGui::SetNextWindowPos (center, ImGuiCond_Appearing, ImVec2 (0.5f, 0.5f));

  ImGui::PushStyleColor (ImGuiCol_ChildBg, missionx::color::color_vec4_black);
  {
    if (ImGui::BeginPopupModal (inPopupWindowName.data (), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::BeginChild ("Are you sure you want to quit##QuitMission", ImVec2 (390.0f, 50.0f), ImGuiChildFlags_None);
      {
        int iStyle = 0;
        ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
        iStyle++;
        ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_mx_dimblack);
        iStyle++;
        ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_yellowgreen);
        iStyle++;
        ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_TextSelectedBg, missionx::color::color_vec4_black);
        iStyle++;

        this->mxUiSetFont (mxconst::get_TEXT_TYPE_MSG_POPUP());
        ImGui::TextColored (missionx::color::color_vec4_yellow, "Are you sure you want to Quit the mission ?\n");
        ImGui::Separator ();
        ImGui::Spacing ();

        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG());
        // ImGui::SetWindowFontScale(1.1f);
        if (ImGui::Button (std::string ("Yes Quit " + Utils::from_u8string (ICON_FA_SIGN_OUT_ALT)).c_str (), ImVec2 (90, 0)))
        {
          ImGui::CloseCurrentPopup ();
          this->execAction (missionx::mx_window_actions::ACTION_QUIT_MISSION);
        }
        ImGui::SameLine (0.0f, 20.0f);

        if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
        {
          if (ImGui::Button (std::string ("Save " + Utils::from_u8string (ICON_FA_SAVE) + " & Quit " + Utils::from_u8string (ICON_FA_SIGN_OUT_ALT)).c_str (), ImVec2 (130, 0)))
          {
            ImGui::CloseCurrentPopup ();
            this->execAction (missionx::mx_window_actions::ACTION_QUIT_MISSION_AND_SAVE);
          }
        }

        ImGui::SameLine (0.0f, 20.0f);
        ImGui::SetItemDefaultFocus ();
        if (ImGui::Button (std::string ("Oops, go back " + Utils::from_u8string (ICON_FA_REPLY)).c_str (), ImVec2 (130, 0)))
        {
          ImGui::CloseCurrentPopup ();
        }

        // ImGui::SetWindowFontScale(1.0f);

        ImGui::PopStyleColor (iStyle); // pop out before EndPopup

        this->mxUiReleaseLastFont (2);
      }
      ImGui::EndChild ();

      ImGui::EndPopup ();
    } // end quit popup
  }
  ImGui::PopStyleColor ();
}



void
WinImguiBriefer::draw_popup_generate_mission_based_on_ext_fpln (const std::string_view inPopupWindowName, const missionx::mx_ext_internet_fpln_strct &rowData, const int &picked_fpln_id_i)
{
  ImGui::SetNextWindowSize(ImVec2(640.0f, 410.0f));

  ImGui::PushStyleColor(ImGuiCol_PopupBg, missionx::color::color_vec4_black);
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG());
  {
    if (ImGui::BeginPopupModal (inPopupWindowName.data (), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      const ImVec2 modal_center (mxUiGetContentWidth () * 0.5f, ImGui::GetWindowHeight () * 0.5f);

      if (rowData.internal_id == picked_fpln_id_i)
      {
        static int             plane_type_i = static_cast<int> (missionx::mx_plane_types::plane_type_props);
        static constexpr float child_h      = 340.0;
        ImGui::BeginChild ("fpln_details_left_side", ImVec2 (modal_center.x - 5.0f, child_h), ImGuiChildFlags_Borders);
        {
          ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", "To:");
          ImGui::SameLine (0.0f, 1.0f); // one space
          ImGui::TextColored (missionx::color::color_vec4_greenyellow, "%s %s", rowData.toICAO_s.c_str (), (rowData.toName_s.empty ()) ? "" : rowData.toName_s.substr (0, 30).c_str() );

          ImGui::NewLine ();
          ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", "Waypoints coordinates: ");
          ImGui::TextWrapped ("%s", rowData.formated_nav_points_with_guessed_names_s.c_str ());

          ImGui::Separator ();
          ImGui::TextColored (missionx::color::color_vec4_yellow, "notes: ");
          ImGui::TextWrapped ("%s", rowData.notes_s.c_str ());
        }
        ImGui::EndChild ();

        ImGui::SameLine ();

        ImGui::BeginChild ("fpln_options_right_side", ImVec2 (modal_center.x - 5.0f, child_h), ImGuiChildFlags_Borders);
        {
          ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
          ImGui::Text ("Pick your preferred plane:");
          ImGui::PopStyleColor ();
          ImGui::RadioButton ("Heli", &plane_type_i, static_cast<int> (missionx::mx_plane_types::plane_type_helos));
          ImGui::SameLine ();
          ImGui::RadioButton ("Props", &plane_type_i, static_cast<int> (missionx::mx_plane_types::plane_type_props));
          ImGui::SameLine ();
          ImGui::RadioButton ("Floats", &plane_type_i, static_cast<int> (missionx::mx_plane_types::plane_type_ga_floats));
          ImGui::RadioButton ("Turbo Prop", &plane_type_i, static_cast<int> (missionx::mx_plane_types::plane_type_turboprops));
          ImGui::SameLine ();
          ImGui::RadioButton ("Jet", &plane_type_i, static_cast<int> (missionx::mx_plane_types::plane_type_jets));
          ImGui::SameLine ();
          ImGui::RadioButton ("Heavy", &plane_type_i, static_cast<int> (missionx::mx_plane_types::plane_type_heavy));

          ImGui::NewLine (); // v3.0.253.11
          if (this->getCurrentLayer () == missionx::uiLayer_enum::flight_leg_info) // v25.04.2
          {
            missionx::WinImguiBriefer::mxUiHelpMarker (missionx::color::color_vec4_aqua, "Ensure that your plane is placed in the Departure airport before pressing the [Start] button.");
            ImGui::SameLine ();
          }

          ImGui::Checkbox ("Start from plane position", &this->strct_cross_layer_properties.flag_start_from_plane_position); // v3.0.253.11
          ImGui::Spacing ();
          ImgWindow::mxUiHelpMarker (missionx::color::color_vec4_aqua, "Will create a Departure and Arrival entries.");
          ImGui::SameLine ();
          ImGui::Checkbox ("Generate GPS waypoints.", &this->strct_cross_layer_properties.flag_generate_gps_waypoints); // v3.0.253.12

          if (this->getCurrentLayer () == missionx::uiLayer_enum::flight_leg_info && this->strct_cross_layer_properties.flag_generate_gps_waypoints)
          {
            ImGui::Spacing ();
            ImGui::SameLine (0.0f, 10.0f);
            ImgWindow::mxUiHelpMarker (missionx::color::color_vec4_aqua, R"(Add the "Route Waypoints" to the Flight Plan.)");
            ImGui::SameLine ();
            ImGui::Checkbox ("Add Route Waypoints.", &this->strct_cross_layer_properties.flag_add_route_waypoints); // v25.04.2
          }
          this->add_ui_auto_load_checkbox (); // v25.04.2


          ImGui::Spacing (); // v3.303.14.2 added default weight to the generate screen
          ImGui::Checkbox ("Add default base weights.\n(Not advisable for planes > GAs)", &this->adv_settings_strct.flag_add_default_weight_settings);
          // v25.04.1
          ImGui::Spacing ();
          this->add_ui_pick_subcategories (this->mapMissionCategories[static_cast<int>(missionx::mx_mission_type::cargo)]);
          ImGui::Spacing ();
          this->add_ui_advance_settings_random_date_time_weather_and_weight_button2(this->adv_settings_strct.iClockDayOfYearPicked, this->adv_settings_strct.iClockHourPicked, this->adv_settings_strct.iClockMinutesPicked);
          ImGui::Spacing (); // v24.03.2
          add_designer_mode_checkbox (); // v24.03.2 Designer mode flag
        }
        ImGui::EndChild ();

        ImGui::Separator ();
        ImGui::NewLine ();
        ImGui::SameLine (modal_center.x * 0.4f);

        // v3.303.10
        static bool bRerunRandomDateTime{ false };
        bRerunRandomDateTime = add_ui_checkbox_rerun_random_date_and_time ();

        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG());
        ImGui::SameLine (0.0f, 5.0f);
        if (ImGui::Button (">> Generate <<", ImVec2 (120, 0)))
        {
          if (bRerunRandomDateTime) // v3.303.10
            this->execAction (missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);

          // Prepare and call ACTION_GENERATE_RANDOM_MISSION
          IXMLNode node_ptr = missionx::data_manager::prop_userDefinedMission_ui.node;

          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_FPLN_ID_PICKED(), picked_fpln_id_i);
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_PLANE_TYPE_I(), plane_type_i);
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_START_FROM_PLANE_POSITION(), this->strct_cross_layer_properties.flag_start_from_plane_position); // v3.0.253.11 start from plane position
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_GENERATE_GPS_WAYPOINTS(), this->strct_cross_layer_properties.flag_generate_gps_waypoints); // v3.0.253.12 generate GPS waypoints
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_AUTO_LOAD_ROUTE_TO_GPS_OR_FMS_B (), this->strct_cross_layer_properties.flag_auto_load_route_to_gps_or_fms); // v25.04.2

          // v24.03.1 Sub Category Text
          // Store the label of the sub category, if the vector has the data
          if (const auto vecToDisplay = this->mapMissionCategories[static_cast<int> (missionx::mx_mission_type::cargo)]; vecToDisplay.size () > this->strct_user_create_layer.iMissionSubCategoryPicked)
            missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_MISSION_SUBCATEGORY_LBL (), vecToDisplay.at (this->strct_user_create_layer.iMissionSubCategoryPicked));

          // v25.04.2 check layer and add route waypoints
          auto current_layer = this->getCurrentLayer ();
          if (this->getCurrentLayer () == missionx::uiLayer_enum::flight_leg_info && this->strct_cross_layer_properties.flag_add_route_waypoints)
          {
            // add the FMS waypoint
            if (!mxUtils::trim (std::string (this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::waypoints])).empty ())
              missionx::data_manager::prop_userDefinedMission_ui.addChildText (mxconst::get_PROP_ADD_ROUTE_WAYPOINTS (), this->strct_flight_leg_info.mapNoteFieldLong[missionx::enums::mx_note_longField_enum::waypoints]); // v25.04.2

            #ifndef RELEASE
            Utils::xml_print_node (missionx::data_manager::prop_userDefinedMission_ui.node);
            #endif
          }
          else
          {
            if (auto node = missionx::data_manager::prop_userDefinedMission_ui.getChild (mxconst::get_PROP_ADD_ROUTE_WAYPOINTS ()); !node.isEmpty ())
              node.deleteNodeContent ();
          }

          this->addAdvancedSettingsPropertiesBeforeGeneratingRandomMission ();

          this->selectedTemplateKey = mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI ();
          this->setMessage ("Generating mission is in progress, please wait...", 10);

          ImGui::CloseCurrentPopup ();
          this->execAction (mx_window_actions::ACTION_GENERATE_RANDOM_MISSION);
        }
        this->mxUiReleaseLastFont (); // v25.04.2 Release the "generate button font"

        ImGui::SetItemDefaultFocus ();
        ImGui::SameLine (modal_center.x * 1.25f);
        if (ImGui::Button ("Cancel", ImVec2 (120, 0)))
        {
          ImGui::CloseCurrentPopup ();
        }

      } // rowData.internal_id == picked_fpln_id_i

      ImGui::EndPopup ();
    }
  }
  this->mxUiReleaseLastFont ();
  ImGui::PopStyleColor();

}



// ------------ draw_conv_popup_which_global_settings_to_save

void
WinImguiBriefer::draw_conv_popup_which_global_settings_to_save(std::string_view inPopupWindowName)
{
  bool flagSave = false;

  const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(550.0f, 150.0f));


  ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_black);
  {
    if (ImGui::BeginPopupModal(inPopupWindowName.data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      int iStyle = 0;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      iStyle++;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_dodgerblue);
      iStyle++;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_white);
      iStyle++;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_TextSelectedBg, missionx::color::color_vec4_black);
      iStyle++;

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_MSG_POPUP());
      ImGui::TextColored(missionx::color::color_vec4_yellow, "Pick which Global Settings you would like to save:\n---------------------------------------------------------\n1. The one you loaded from the converter save file, \tor\n2. Generate a new one based on the Advanced Settings.\n");

      // ImGui::NewLine();
      ImGui::Separator();

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
      if (ImGui::Button("Use existing Global Settings", ImVec2(210, 0)))
      {
        flagSave                                                                  = true;
        this->strct_conv_layer.flag_use_loaded_globalSetting_from_conversion_file = true;
      }
      this->mx_add_tooltip(missionx::color::color_vec4_white, "The plugin will use the <global_settings> loaded from the \"converter.sav\" file.");

      ImGui::SameLine(0.0f, 20.0f);
      ImGui::SetItemDefaultFocus();
      if (ImGui::Button("Generate a new Global Settings", ImVec2(210, 0)))
      {
        flagSave                                                                  = true;
        this->strct_conv_layer.flag_use_loaded_globalSetting_from_conversion_file = false;
      }
      this->mx_add_tooltip(missionx::color::color_vec4_white, "The plugin will use the Advanced Settings information to generate a new <global_settings> element.\nThis includes weather/time and weight");

      ImGui::SameLine(0.0f, 20.0f);
      ImGui::SetItemDefaultFocus();
      if (ImGui::Button("Cancel", ImVec2(70, 0)))
      {
        flagSave = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::PopStyleColor(iStyle); // pop out before EndPopup

      this->mxUiReleaseLastFont(2);

      if (flagSave)
      {
        ImGui::CloseCurrentPopup();
        this->setMessage("Please wait while generating the mission from the Flight Plan.", 10);
        this->execAction(missionx::mx_window_actions::ACTION_GENERATE_MISSION_FROM_LNM_FPLN);
      }

      ImGui::EndPopup();
    } // end quit popup
  }
  ImGui::PopStyleColor();
}

// ------------ draw_globals_online_edit_popup

void
WinImguiBriefer::draw_globals_online_edit_popup(std::string_view inPopupWindowName, char inType, std::string inKey, std::string inVal)
{
  const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(500.0f, 150.0f));

  ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_black);
  {
    if (ImGui::BeginPopupModal(inPopupWindowName.data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      int iStyle = 0;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      iStyle++;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_dodgerblue);
      iStyle++;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_white);
      iStyle++;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_TextSelectedBg, missionx::color::color_vec4_black);
      iStyle++;

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_MSG_POPUP());

      ImGui::TextColored(missionx::color::color_vec4_yellow, "Modify the Global parameter and save:");
      if (ImGui::InputText("##EditGlobalParam", this->strct_flight_leg_info.online_globals_buff, sizeof(this->strct_flight_leg_info.online_globals_buff)))
      {
        this->strct_flight_leg_info.online_globals_buff_size = sizeof(this->strct_flight_leg_info.online_globals_buff);
      }
      // ImGui::TextColored(missionx::color::color_vec4_beige, "Script length: %i", sizeof(*this->strct_flight_leg_info.online_globals_buff));


      if (ImGui::Button("Save & Close", ImVec2(140, 0)))
      {
        switch (inType)
        {
          case 'b':
          {
            bool bResult = false;
            if (mxUtils::isStringBool( this->strct_flight_leg_info.online_globals_buff, bResult ))
            {
                  script_manager::mapScriptGlobalBoolArg[inKey] = bResult;
                  this->setMessage("Stored Bool Value.", 4);
                  ImGui::CloseCurrentPopup();

            }
          }
          break;
          case 'n':
          {
            if (mxUtils::is_digits( this->strct_flight_leg_info.online_globals_buff ))
            {
                  script_manager::mapScriptGlobalDecimalArg[inKey] = mxUtils::stringToNumber<double>(this->strct_flight_leg_info.online_globals_buff, 8);
                  this->setMessage("Stored Number Value.", 4);
                  ImGui::CloseCurrentPopup();

            }
          }
          break;
          case 's': // text ?
          {
            if (mxUtils::isElementExists(script_manager::mapScriptGlobalStringsArg, inKey))
            {
                  script_manager::mapScriptGlobalStringsArg[inKey] = std::string(this->strct_flight_leg_info.online_globals_buff);
            }
          }
          break;
        } // end switch


      }
      ImGui::SameLine(0.0f, 50.0f);
      if (ImGui::Button("Close", ImVec2(70, 0)))
      {
        ImGui::CloseCurrentPopup();
      }

      this->mxUiReleaseLastFont();

      ImGui::PopStyleColor(iStyle);

      ImGui::EndPopup();

    } // end popup
  }
  ImGui::PopStyleColor();
}

// ------------ draw_script_online_edit_popup

void
WinImguiBriefer::draw_script_online_edit_popup(std::string_view inPopupWindowName, bool& outSave)
{
  const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(750.0f, 350.0f));

  ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_black);
  {
    if (ImGui::BeginPopupModal(inPopupWindowName.data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      int iStyle = 0;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      iStyle++;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_dodgerblue);
      iStyle++;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_white);
      iStyle++;
      ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_TextSelectedBg, missionx::color::color_vec4_black);
      iStyle++;

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_MSG_POPUP());
      // v3.305.3 skip abort script on script error
      this->add_ui_skip_abort_setup_checkbox();


      ImGui::TextColored(missionx::color::color_vec4_yellow, "Edit the script and save:");
      if (ImGui::InputTextMultiline("##OnlineScriptEdit", this->strct_flight_leg_info.online_debug_buff, sizeof(this->strct_flight_leg_info.online_debug_buff), ImVec2(700.0f, 240.0f)))
      {
        this->strct_flight_leg_info.online_debug_buff_size = sizeof(this->strct_flight_leg_info.online_debug_buff);
      }
      //ImGui::TextColored(missionx::color::color_vec4_beige, "Script length: %i", sizeof(*this->strct_flight_leg_info.online_debug_buff));


      if (ImGui::Button("Save", ImVec2(70, 0)))
      {
        outSave = true;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine(0.0f, 50.0f);
      if (ImGui::Button("Cancel", ImVec2(70, 0)))
      {
        outSave = false;
        //delete[] this->strct_flight_leg_info.online_debug_buff;
        this->strct_flight_leg_info.online_debug_buff[0] = '\0';
        ImGui::CloseCurrentPopup();
      }

      this->mxUiReleaseLastFont();

      ImGui::PopStyleColor(iStyle);

       ImGui::EndPopup();

    } // end popup

  }
  ImGui::PopStyleColor();



}

// ------------ popup_draw_authorization_key

// void
// WinImguiBriefer::popup_draw_authorization_key(std::string_view inPopupWindowName)
// {
//   ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
//   ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
//
//   ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
//   ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_antiquewhite); //
//   {
//     if (ImGui::BeginPopupModal(inPopupWindowName.data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
//     {
//       ImGui::Spacing ();
//       ImGui::Separator ();
//
//       this->add_ui_flightplandb_key (true);
//
//       // Add Cancel button
//       ImGui::SameLine(0.0f, 80.0f);
//       ImGui::SetItemDefaultFocus();
//
//       this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG());
//       ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_darkgrey);
//       ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_black);
//       if (ImGui::Button ("Cancel", ImVec2 (80, 0)))
//       {
//         ImGui::CloseCurrentPopup();
//       }
//       ImGui::PopStyleColor (2);
//       this->mxUiReleaseLastFont ();
//
//       ImGui::EndPopup();
//     } // end AuthKey popup
//   }   // end style
//   ImGui::PopStyleColor(1);
// }


// -------------------------------------------
// ------------ draw setup layer
// -------------------------------------------
void
WinImguiBriefer::draw_setup_layer()
{
  const static std::string IMAGE          = mxconst::get_BITMAP_BTN_SETUP_24X18();
  static constexpr float   img_ps_f       = 0.5f;
  float                    pos_x          = data_manager::mapCachedPluginTextures[IMAGE].sImageData.getW_f() * img_ps_f;
  const static std::string slider_label_s = "[" + Utils::formatNumber<double>(mxconst::SLIDER_MIN_RND_DIST, 0) + ".." + Utils::formatNumber<double>(mxconst::SLIDER_MAX_RND_DIST, 0) + "]";

  auto         win_size_vec2 = ImGui::GetWindowSize();
  const auto   modal_w       = mxUiGetContentWidth();
  const auto   modal_h       = ImGui::GetWindowHeight();
  const ImVec2 modal_center(modal_w * 0.5f, modal_h * 0.5f);
  const auto   vec2_multiLine_dim = ImVec2(win_size_vec2.x - 25.0f, 120.0f);


  //------------------------------------------------
  //                  First Time
  //------------------------------------------------
  if (this->strct_setup_layer.is_first_time)
  {
    std::string stored_overpass_url = Utils::getNodeText_type_6(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_URL(), mxconst::get_DEFAULT_OVERPASS_URL());
    std::memcpy(this->strct_setup_layer.overpass_url_buf, stored_overpass_url.c_str(), missionx::WinImguiBriefer::mx_setup_layer::OSM_BUFF_SIZE_I);

    // v3.303.9.1
    if (!missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_SCORING().c_str()).isEmpty())
    {
      IXMLNode    node          = missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_SCORING().c_str());
      std::string nodeContent_s = Utils::xml_get_node_content_as_text(node);
      std::memcpy(this->strct_setup_layer.default_scoring_buf, nodeContent_s.c_str(), sizeof(this->strct_setup_layer.default_scoring_buf) - 1);
    }

    this->strct_setup_layer.flag_lock_overpass_url = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_LOCK_OVERPASS_URL_TO_USER_PICK(), false);

    // initialize the combo option from the preference file. Search for same URL string
    for (auto i = static_cast<size_t> (0); i < this->strct_setup_layer.vecOverpassUrls_char.size(); ++i)
    {
      if (stored_overpass_url == this->strct_setup_layer.vecOverpassUrls_char.at(i))
      {
        missionx::data_manager::overpass_user_picked_combo_i = static_cast<int> (i);
        break;
      }
    }


    #ifdef LIN
    this->strct_setup_layer.iLinuxFlavor_val = Utils::getNodeText_type_1_5<int>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_LINUX_FLAVOR_CODE_I(), 0); // v3.303.8.1
    #endif


    this->strct_setup_layer.is_first_time = false;
  } // end first time



  //------------------------------------------------
  //                  Draw Setup Layer
  //------------------------------------------------
  ImGui::BeginChild("draw_setup_layer_01", ImVec2(0.0f, static_cast<float> (WINDOWS_MAX_HEIGHT) - FOOTER_REGION_CLEARENCE_PX));
  {
    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
    {
      ///////// Image ////////
      ImGui::BeginGroup();
      ImGui::Image(reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[IMAGE].gTexture)), ImVec2(data_manager::mapCachedPluginTextures[IMAGE].sImageData.getW_f() * img_ps_f, data_manager::mapCachedPluginTextures[IMAGE].sImageData.getH_f() * img_ps_f));
      ImGui::EndGroup();
    }
    ImGui::PopStyleColor(1);
  }



  //------------------------------------------------
  //          START CollapsingHeader SETUP NODES
  //------------------------------------------------


  pos_x += 50.0f;
  ImGui::SameLine(pos_x);
  ImGui::BeginGroup();
  {
    //------------------------------------------------
    //                  Pilot Name
    //------------------------------------------------

    // v3.305.1 added Pilot name for story mode in <message> element.
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
    missionx::WinImguiBriefer::HelpMarker("> Stores name in preference file only when pressing [Enter]\n> Only used in Messages that use the \"story mode\".\n> Might be extended in future builds to all missions.");
    ImGui::SameLine();
    ImGui::TextColored(missionx::color::color_vec4_turquoise, "Your pilot nick name:");
    ImGui::SameLine();
    this->mx_add_tooltip(missionx::color::color_vec4_white, "Press [Enter] to store value.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputTextWithHint("##PilotNickName", "Pilot Nickname", this->strct_setup_layer.buf_pilotName, 10, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue))
    {
      Utils::xml_search_and_set_node_text(system_actions::pluginSetupOptions.node, mxconst::SETUP_PILOT_NAME, std::string(this->strct_setup_layer.buf_pilotName), this->mxcode.STRING, true);
      this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
    }

    ImGui::Spacing(); // v3.305.3 moved


    this->add_pause_in_2d_mode();
    ImGui::Separator();

    bool bDesignerMode = Utils::readNodeNumericAttrib<int>( missionx::system_actions::pluginSetupOptions.node,mxconst::get_OPT_ENABLE_DESIGNER_MODE(), false); // 0 = false
    if (ImGui::Checkbox("Toggle Designer Mode", &bDesignerMode))
    {
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::toggle_designer_mode);
    }

    ImGui::SameLine(0.0f, 25.0f);
    bool bCueInfo = Utils::readNodeNumericAttrib<int>( missionx::system_actions::pluginSetupOptions.node,mxconst::get_OPT_DISPLAY_VISUAL_CUES(), false); // 0 = false
    if (ImGui::Checkbox("Toggle Cue Info", &bCueInfo))
    {
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::toggle_cue_info_mode);
    }


    this->mxUiReleaseLastFont();
    ImGui::Separator();
    ImGui::Spacing();



    int indx = 0;
    //------------------------------------------------
    //                  General Settings Group
    //------------------------------------------------
    ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);


    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());

    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
      ImGui::TextColored(missionx::color::color_vec4_yellow, "GPS Waypoints & Markers:");

      // GPS Exposure
      if (data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
      {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      }

      this->add_ui_expose_all_gps_waypoints();


      if (data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
      {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
      }

      // target markers
      if (ImGui::Checkbox("Display Target Markers", &this->strct_setup_layer.bDisplayTargetMarkers))
      {
        // ADD set option value
        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_DISPLAY_TARGET_MARKERS(), this->strct_setup_layer.bDisplayTargetMarkers);
        this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }

      ImGui::NewLine();

      this->add_ui_suppress_distance_messages_checkbox_ui (); // v25.02.1

      ImGui::NewLine ();

      ImGui::TextColored(missionx::color::color_vec4_yellow, "Font Scale:");

      missionx::WinImguiBriefer::HelpMarker("Font Scale will resize font based on software interpolation code.\nIt is not the same as picking a larger font pixel.");
      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::SliderFloat("Preferred Font Scale, Example:", &this->strct_setup_layer.fPreferredFontScale, this->strct_setup_layer.fFontMinScaleSize, this->strct_setup_layer.fFontMaxScaleSize, "scale %.1f"))
      {
        // ADD set option value
        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<float>(mxconst::get_SETUP_SLIDER_FONT_SCALE_SIZE(), this->strct_setup_layer.fPreferredFontScale);
        this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);
      ImGui::TextColored(missionx::color::color_vec4_aqua, "Mission-X v3.x");
      ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);

      // Inventory X11 compatibility mode // 24.12.2
      if (missionx::data_manager::xplane_ver_i > missionx::XP12_VERSION_NO)
      {
        ImGui::NewLine();

        // disable while running
        const bool bDisable = this->mxStartUiDisableState(missionx::data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running);
        {
          ImGui::TextColored(missionx::color::color_vec4_yellow, "Inventory Layout:");
          ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
          this->add_ui_xp11_comp_checkbox(true); // v24.12.2
        }
        this->mxEndUiDisableState(bDisable);
      }

      ImGui::NewLine();

      ImGui::TextColored(missionx::color::color_vec4_yellow, "Pause X-Plane when Mission-X screen is open in:");
      ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
      if (ImGui::Checkbox("Pause in VR mode", &this->strct_setup_layer.bPauseInVR))
      {
        // ADD set option value
        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_AUTO_PAUSE_IN_VR(), this->strct_setup_layer.bPauseInVR);
        this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }

      ImGui::Separator(); // v3.305.1

      // Cycle Mission-X Log File
      ImGui::TextColored(missionx::color::color_vec4_yellow, "Cycle plugin log files:");
      missionx::WinImguiBriefer::HelpMarker("Cycle Mission-X log file in every new session.\nThe plugin will keep few versions of the logfile for debug or as a reference.\n\nYou can find the files in the plugin folder.");
      ImGui::SameLine ();
      if (ImGui::Checkbox("Cycle log files", &this->strct_setup_layer.bCycleLogFiles))
      {
        // ADD set option value
        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_CYCLE_LOG_FILES(), this->strct_setup_layer.bCycleLogFiles);
        this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }

      ImGui::Separator();
      ImGui::NewLine();

      this->mxUiReleaseLastFont();

    } // end collapse


    //------------------------------------------------
    //      Simbrief + flightplandatabase.com
    //------------------------------------------------
    ++indx; // 1

    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {
      this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG());
      {
        ImGui::Spacing ();
        ImGui::TextColored (missionx::color::color_vec4_greenyellow, "Simbrief:");

        this->add_ui_simbrief_pilot_id ();
        ImGui::Spacing ();
        ImGui::Separator ();
        this->add_ui_flightplandb_key (false);
      }
      this->mxUiReleaseLastFont ();

      ImGui::Spacing ();
      ImGui::Separator ();
    }
    //------------------------------------------------
    //                  APT Dat optimizations
    //------------------------------------------------
    ++indx; // 2

    ImGui::PushStyleColor(ImGuiCol_Header, missionx::color::color_vec4_grey);

    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {
      std::string err;

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text("Execute the apt.dat optimization scripts:");
      ImGui::PopStyleColor(1); // back to white

      ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      {
        if (data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
        {
          ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("Run Apt.Dat Optimization", ImVec2(200.0f, 30.0f)))
        {
          if (missionx::data_manager::flag_apt_dat_optimization_is_running || OptimizeAptDat::aptState.flagIsActive)
            this->setMessage("Apt.DAT is running.... please wait.");
          else
            missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::exec_apt_dat_optimization); // v3.0.253.6
        }
        if (data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
        {
          ImGui::PopItemFlag();
          ImGui::PopStyleVar();
        }
      }
      ImGui::PopStyleColor(3);

      ImGui::Separator(); // v3.305.1
      ImGui::NewLine();   // v3.305.1


      this->mxUiReleaseLastFont(); // v3.305.1
    }

    ImGui::PopStyleColor(); // header color

    //------------------------------------------------
    //                  Tools Group
    //------------------------------------------------

    ++indx; // 3
    ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
    //if (ImGui::CollapsingHeader("TOOLS"))
    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {
      constexpr const static float BTN_WIDTH_F = 300.0f; // v3.305.1

      missionx::WinImguiBriefer::HelpMarker("Create external flight plan files based on the \"fpln_folders.ini\" settings and the current GPS flight plan.");
      ImGui::SameLine();

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text("Create external FPLN based of GPS");
      ImGui::PopStyleColor(1); // back to white

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      if (ImGui::Button("Write GPS to External FPLN", ImVec2(BTN_WIDTH_F, 25.0f)))
      {
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::write_fpln_to_external_folder); //
      }
      ImGui::PopStyleColor(3); // back to white

      ImGui::NewLine();


      // v3.303.14 Write Plane Coordinates
      missionx::WinImguiBriefer::HelpMarker("Write Plane coordinates into the missionx.log file.");
      ImGui::SameLine();

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text("Dump Plane coordinates into missionx.log file");
      ImGui::PopStyleColor(1); // back to white

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      if (ImGui::Button("Dump Plane coordinates", ImVec2(BTN_WIDTH_F, 25.0f)))
      {
        missionx::data_manager::write_plane_position_to_log_file();
        this->setMessage("Wrote Plane coordinates to missionx.log file.", 5); // v3.305.3
      }
      ImGui::PopStyleColor(3); // back to white


      ImGui::NewLine();

      // v3.303.14 Write Camera Coordinates
      missionx::WinImguiBriefer::HelpMarker("Write Camera coordinates into the missionx.log file.");
      ImGui::SameLine();

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text("Dump Camera coordinates into missionx.log file");
      ImGui::PopStyleColor(1); // back to white

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      if (ImGui::Button("Dump Camera coordinates", ImVec2(BTN_WIDTH_F, 25.0f)))
      {
        missionx::data_manager::write_camera_position_to_log_file();
        this->setMessage("Wrote Camera coordinates to missionx.log file.", 5); // v3.305.3
      }
      ImGui::PopStyleColor(3); // back to white


      ImGui::NewLine();

      // v3.303.13 Write weather data
      missionx::WinImguiBriefer::HelpMarker("Write Weather datarefs into the missionx.log file so you could use that in a scriptlet.\nXP12 designers are encourage to double check the \"change_mode, variability, turbulence, cloud_height_msl\" datarefs and tweak them for better experience.");
      ImGui::SameLine();

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text("Dump weather related datarefs into missionx.log file");
      ImGui::PopStyleColor(1); // back to white

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      if (ImGui::Button("Dump Weather Datarefs", ImVec2(BTN_WIDTH_F, 25.0f)))
      {
        missionx::data_manager::write_weather_state_to_log_file();
        this->setMessage("Wrote Weather info into missionx.log file.", 5); // v3.305.3
      }
      ImGui::PopStyleColor(3); // back to white


      ImGui::NewLine();

      missionx::WinImguiBriefer::HelpMarker("!!!This is a template designer feature!!!\n\nReads the file '{XP}/Custom Scenery/missionx/random/points.xml' and probe <point> locations to test if it is water or land.\nThe information will be used by the Random Engine (Read the designer guide, check the Appendix)");
      ImGui::SameLine();

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text("Probe <point> elements, best used if you are in the same DSF area.");
      ImGui::PopStyleColor(1); // back to white

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      if (ImGui::Button("Update <point> elements in points.xml file", ImVec2(BTN_WIDTH_F, 25.0f)))
      {
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::update_point_in_file_with_template_based_on_probe); //
      }
      ImGui::PopStyleColor(3); // back to white


      ImGui::Separator();
      ImGui::NewLine();


      this->mxUiReleaseLastFont(); // v3.305.1

    } // TOOLS


    //------------------------------------------------
    //                  Normalized Volume Group
    //------------------------------------------------
    ++indx; // 4

    // v3.0.303.6
    //if (ImGui::CollapsingHeader("Normalize Mission Sound Volume"))
    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1

      ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      {
        ImGui::Text("Force Normalized Mission Volume: ");

        missionx::WinImguiBriefer::HelpMarker("Normalized mission volume means that we force a max volume over the mission volume definition (NOT X-Plane sound volume).\nThe effect takes place when the sound file starts to play and not during play.");
        ImGui::SameLine();
        ImGui::Text("Force Volume ?: ");
        ImGui::SameLine();
      }
      ImGui::PopStyleColor(1); // back to white

      if (ImGui::Checkbox("##normalizedCheckbox", &this->strct_setup_layer.bForceNormalizedVolume))
      {
        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_NORMALIZE_VOLUME_B(), this->strct_setup_layer.bForceNormalizedVolume);
        this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }
      if (this->strct_setup_layer.bForceNormalizedVolume)
      {
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::SetNextItemWidth(100.0f);
        if (ImGui::InputInt("(0..100)##volumeValue", &this->strct_setup_layer.iNormalizedVolume_val, 1, 2))
        {
          if (this->strct_setup_layer.iNormalizedVolume_val < static_cast<int> (mxconst::MIN_SOUND_VOLUME_F))
            this->strct_setup_layer.iNormalizedVolume_val = static_cast<int> (mxconst::MIN_SOUND_VOLUME_F);
          else if (this->strct_setup_layer.iNormalizedVolume_val > static_cast<int> (mxconst::MAX_SOUND_VOLUME_F))
            this->strct_setup_layer.iNormalizedVolume_val = static_cast<int> (mxconst::MAX_SOUND_VOLUME_F);

          missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<int>(mxconst::get_SETUP_NORMALIZED_VOLUME(), this->strct_setup_layer.iNormalizedVolume_val);
          this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
        }
      }

      ImGui::Separator();
      ImGui::NewLine();

      this->mxUiReleaseLastFont(); // v3.305.1
    }



    //------------------------------------------------
    //                  OVERPASS
    //------------------------------------------------
    indx++; // 5 Overpass
    //if (ImGui::CollapsingHeader("OVERPASS Setup"))
    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1
      ImGui::PushItemWidth(450.0f);
      {

        ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
        ImGui::TextColored(missionx::color::color_vec4_yellow, "Preferred Overpass URL:");
        ImGui::PushID("##overpassCustomUrl");
        if (ImGui::Combo("##OverPassURLs", &missionx::data_manager::overpass_user_picked_combo_i, this->strct_setup_layer.vecOverpassUrls_char.data(), static_cast<int> (this->strct_setup_layer.vecOverpassUrls_char.size ())))
        {
#ifndef RELEASE
          const std::string stored_overpass_url = this->strct_setup_layer.vecOverpassUrls_char.at(missionx::data_manager::overpass_user_picked_combo_i); // for debug purposes only
#endif                                                                                                                                                   // !RELEASE
          missionx::data_manager::overpass_last_url_indx_used_i = missionx::data_manager::overpass_user_picked_combo_i;                                  // v3.0.255.4.1 store user picked in last run so we will start from it during next run

          Utils::xml_search_and_set_node_text(system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_URL(), std::string(this->strct_setup_layer.vecOverpassUrls_char.at(missionx::data_manager::overpass_user_picked_combo_i)), this->mxcode.STRING, true);

          this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
        }
        ImGui::PopID();
        ImGui::PopItemWidth();
        // Lock URL - checkbox
        ImGui::SameLine();
        if (ImGui::Checkbox("Lock URL", &this->strct_setup_layer.flag_lock_overpass_url))
        {
          missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_LOCK_OVERPASS_URL_TO_USER_PICK(), ((this->strct_setup_layer.flag_lock_overpass_url) ? true : false));
          this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
        }

        ImGui::Separator(); // v3.305.1
        ImGui::NewLine();   // v3.305.1

      }

      this->mxUiReleaseLastFont(); // v3.305.1
    }
    //------------------------------------------------
    //                  Medevac Group
    //------------------------------------------------
    indx++; // 6 Medevac
    //if (ImGui::CollapsingHeader("Medevac Setup"))
    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());  // v3.305.1

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text("Settings Will be applied only to helos missions");
      ImGui::Text("and it depends on template setup");
      ImGui::PopStyleColor(1); // back to white

      ImGui::NewLine();

      add_skewed_marker_checkbox(); // v3.0.253.6

      ImGui::Separator(); // v3.305.1
      ImGui::NewLine();   // v3.305.1


      this->mxUiReleaseLastFont(); // v3.305.1
    }

    //------------------------------------------------
    //                  External Flight Plan
    //------------------------------------------------
    //if (ImGui::CollapsingHeader("External Flight Plan Setup"))
    indx++; // 7 External Flight Plan
    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text("Override external flight plan location and write to X-Plane's 'FMS plans' folder");
      ImGui::Text("(ignores fpln_folders.ini custom locations)");
      ImGui::PopStyleColor(1); // back to white
      if (ImGui::Checkbox("Ignore custom FPLN folders and write to X-Plane 'FMS Plans' instead.", &this->strct_setup_layer.bOverideCustomExternalFPLN_folders))
      {
        // ADD set option value
        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_WRITE_CONVERTED_FPLN_TO_XPLANE_FOLDER(), this->strct_setup_layer.bOverideCustomExternalFPLN_folders);
        this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }

      ImGui::Separator(); // v3.305.1
      ImGui::NewLine();   // v3.305.1

      this->mxUiReleaseLastFont(); // v3.305.1
    }

    //------------------------------------------------
    //                  Default Scoring
    //------------------------------------------------
    //if (ImGui::CollapsingHeader("Default Scoring"))
    indx++; // 8 Default Scoring
    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1

      // Description Text
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text("Modify the default scoring XML node in your preferences file.");
      ImGui::Text("It will be effective only if mission file has no <scoring> element defined.");
      ImGui::Text("!!! Make changes at your own risk. Only modify the attribute values !!!");
      ImGui::PopStyleColor(1); // back to white


      //////////////// Buttons ////////////////
      {
        // Parse and store button
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
        if (ImGui::Button("Parse and Store"))
        {
          std::string scoringNode_s(this->strct_setup_layer.default_scoring_buf);

          IXMLDomParser iDomTemplate;
          IXMLResults   parse_result_strct;
          IXMLNode      xScoring_node = iDomTemplate.parseString(scoringNode_s.c_str(), mxconst::get_ELEMENT_SCORING().c_str(), &parse_result_strct).deepCopy(); // parse xml into ITCXMLNode
          if (parse_result_strct.errorCode == IXMLError_None)
          {
            if (!missionx::data_manager::xMissionxPropertiesNode.isEmpty())
            {
              if (!missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_SCORING().c_str()).isEmpty())
                missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_SCORING().c_str()).deleteNodeContent();

              missionx::data_manager::xMissionxPropertiesNode.addChild(xScoring_node.deepCopy());
              missionx::system_actions::store_plugin_options();
              this->setMessage("Default <scoring> information was stored into preference file.");
            }

#ifndef RELEASE
            Log::logMsg("Valid <scoring>:\n" + scoringNode_s);
#endif // !RELEASE
          }
          else
          {
            std::string err_s = std::string(IXMLPullParser::getErrorMessage(parse_result_strct.errorCode)) + " [line/col:" + mxUtils::formatNumber<long long>(parse_result_strct.nLine) + "/" + mxUtils::formatNumber<int>(parse_result_strct.nColumn) + "]";
            this->setMessage(err_s);
#ifndef RELEASE
            Log::logMsg(err_s);
#endif // !RELEASE

            Log::logMsg("Not valid <scoring>:\n" + scoringNode_s);
          }


        } // end [Parse and Store]
        ImGui::PopStyleColor(3);
        this->mx_add_tooltip(missionx::color::color_vec4_yellow, "The XML string you entered must be valid or it won't be stored");

        // Reset button
        ImGui::SameLine(0.0f, 50.0f);
        if (ImGui::Button("reset data", ImVec2(100.0f, 0.0f)))
        {
          if (!missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_SCORING().c_str()).isEmpty())
          {
            IXMLNode    node          = missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_SCORING().c_str());
            std::string nodeContent_s = Utils::xml_get_node_content_as_text(node);
            std::memcpy(this->strct_setup_layer.default_scoring_buf, nodeContent_s.c_str(), sizeof(this->strct_setup_layer.default_scoring_buf) - 1);
            this->setMessage("Reset Data");
          }
          else
            this->setMessage("Nothing to reset.");
        }
      } // end buttons


      // Input Text
      ImGui::InputTextMultiline("##scoringMultiLine", this->strct_setup_layer.default_scoring_buf, sizeof(this->strct_setup_layer.default_scoring_buf), vec2_multiLine_dim);
      ImGui::TextColored(missionx::color::color_vec4_beige, "%zu of %zu", std::string(this->strct_setup_layer.default_scoring_buf).length(), sizeof(this->strct_setup_layer.default_scoring_buf));

      ImGui::Separator(); // v3.305.1
      ImGui::NewLine();   // v3.305.1

      this->mxUiReleaseLastFont(); // v3.305.1
    }


    //------------------------------------------------
    //                  Linux Troubleshoot
    //------------------------------------------------
    indx++; // 9 Linux troubleshoot // v24.03.2 moved outside #if directive
    #ifdef LIN
    // To change visual labeling you have to change the order in the mapSetupHeaders first.

    //if (ImGui::CollapsingHeader("Linux: Troubleshoot - only if X-Plane 12 crashes during exit or mission quit."))
    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1
      ImGui::TextColored(missionx::color::color_vec4_aqua, "only if X-Plane 12 crashes during exit or mission quit.");

      ImGui::TextColored(missionx::color::color_vec4_yellow, "During X-Plane 12 tests, I found out that the FMOD library has issues in specific function cases.\nThe symptom was: X-Plane became unresponsive. ");
      ImGui::TextColored(missionx::color::color_vec4_yellow, "If that happens, please pick the Linux Distro version you are using and\nthe plugin will try to use a workaround for it.");
      if (ImGui::Combo("Linux Flavor##LinuxFlavorCombo", &this->strct_setup_layer.iLinuxFlavor_val, this->strct_setup_layer.vecLinuxComboCodes_s.data(), static_cast<int> (this->strct_setup_layer.vecLinuxComboCodes_s.size ())))
      {
        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<int>(mxconst::get_SETUP_LINUX_FLAVOR_CODE_I(), this->strct_setup_layer.iLinuxFlavor_val);
        this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }

      this->mxUiReleaseLastFont(); // v3.305.1
    } // end Linux troubleshoot

    #endif



    //------------------------------------------------
    //                  Advanced: unsaved options
    //------------------------------------------------


    //if (ImGui::CollapsingHeader("Designer: Unsaved Options"))
    indx++; // 10 Unsaved Options
    this->strct_setup_layer.mapSetupHeaders[indx].setState((ImGui::CollapsingHeader(this->strct_setup_layer.mapSetupHeaders[indx].title.c_str())));
    if (this->strct_setup_layer.mapSetupHeaders[indx].bState)
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1

      ImGui::TextColored(missionx::color::color_vec4_yellow, "The following options won't be stored in the Mission-X preference file.");
      ImGui::TextColored(missionx::color::color_vec4_yellow, "You will have to set them every new X-Plane session.");

      // v24.03.2 plane/camera location
      ImGui::Separator();
      {
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputDouble("##Latitude", &this->strct_setup_layer.coord.lat, 0.0, 0.0, "%.8f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", "/");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputDouble("##Longitude", &this->strct_setup_layer.coord.lon, 0.0, 0.0, "%.8f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", "Elv.");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50.0f);
        ImGui::InputDouble("##Elevation", &this->strct_setup_layer.coord.elevation_ft, 0.0, 0.0, "%.0f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", "psi");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50.0f);
        ImGui::InputFloat("##Heading", &this->strct_setup_layer.coord.heading, 0.0, 0.0, "%.2f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();

        // Plane/Camera buttons switch
        if (this->strct_setup_layer.bPressedPlane)
          ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_greenyellow);
          if (ImGui::Button("Plane"))
          {
            this->strct_setup_layer.coord           = missionx::data_manager::write_plane_position_to_log_file();
            this->strct_setup_layer.btn_coord_state = this->mx_btn_coordinate_state_enum::plane;
          }
        if (this->strct_setup_layer.bPressedPlane)
          ImGui::PopStyleColor();

        ImGui::SameLine();

        if (this->strct_setup_layer.bPressedCamera)
          ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_greenyellow);
          if (ImGui::Button("Camera"))
          {
            this->strct_setup_layer.coord           = missionx::data_manager::write_camera_position_to_log_file();
            this->strct_setup_layer.btn_coord_state = this->mx_btn_coordinate_state_enum::camera;
          }
        if (this->strct_setup_layer.bPressedCamera)
          ImGui::PopStyleColor();


        switch (this->strct_setup_layer.btn_coord_state)
        {
          case mx_btn_coordinate_state_enum::plane:
          {
            this->strct_setup_layer.bPressedPlane  = true;
            this->strct_setup_layer.bPressedCamera = false;
          }
          break;
          case mx_btn_coordinate_state_enum::camera:
          {
            this->strct_setup_layer.bPressedPlane  = false;
            this->strct_setup_layer.bPressedCamera = true;
          }
          break;
          default:
          {
            this->strct_setup_layer.bPressedPlane  = false;
            this->strct_setup_layer.bPressedCamera = false;
          }
        } // end switch
      }
      ImGui::Separator();


      // v24.03.2 Designer mode flag
      add_designer_mode_checkbox();
      //missionx::WinImguiBriefer::HelpMarker("Enable Designer mode from the setup screen instead from the XML file. The XML file has precidence over this setting, and it will update it when loading a mission.\nMakes the option flexiable during mission tests.\nIn debug binaries it is set to 'true' by default.\nUsage: Affects behaviour of 'force_leg_name' in the global_settings.");
      //ImGui::SameLine();
      //ImGui::Checkbox("Enable \"Designer Mode\"", &missionx::data_manager::flag_setupEnableDesignerMode);

      // v3.305.2 Debug tab during flight
      missionx::WinImguiBriefer::HelpMarker("Display the \"debug\" tab during mission flight.\nBest used with unpausing in 2D mode, so info will be updated in real time.");
      ImGui::SameLine();
      ImGui::Checkbox("Display \"debug\" tab.", &missionx::data_manager::flag_setupShowDebugTabDuringMission);

      // Display Message debug tab
      ImGui::NewLine();
      ImGui::SameLine(0.0f,28.0f);

      missionx::WinImguiBriefer::HelpMarker(
        R"(Opt-In and display the message debug tab.
ATTENTION !!! It allows you to execute the message even if it is not the correct flight leg. !!!
1. Use to debug messages in story mode and testing background sounds.
2. Use when you want to test a specific message while bypasses leg restriction (be careful).

Use at your own risk!!!
)");
      bool bDisable = this->mxStartUiDisableState(!missionx::data_manager::flag_setupShowDebugTabDuringMission);
      {
        ImGui::SameLine();
        ImGui::Checkbox("Display message debug tab.", &missionx::data_manager::flag_setupShowDebugMessageTab);
      }
      this->mxEndUiDisableState( bDisable );


      // v3.305.3 skip abort script on script error
      add_ui_skip_abort_setup_checkbox();

      // Display/Hide Auto skip option in the story mode screen
      missionx::WinImguiBriefer::HelpMarker("Display \"force skip\" option in story mode to make messages complete faster.");
      ImGui::SameLine();
      ImGui::Checkbox("Display \"force skip\" option in story mode.", &missionx::data_manager::flag_setupDisplayAutoSkipInStoryMessage);

      // Disable / Enable "auto skip" if and only if the option is checked.
      ImGui::NewLine();
      ImGui::SameLine(0.0f, 28.0f);
      missionx::WinImguiBriefer::HelpMarker("When in story mode, force skip. This will actually make the story mode message display immediately");

      bDisable = this->mxStartUiDisableState(!missionx::data_manager::flag_setupDisplayAutoSkipInStoryMessage);
        // Toggle Auto Skip
        ImGui::SameLine();
        ImGui::Checkbox("Force Auto Skip, in \"story\" based messages.", &missionx::data_manager::flag_setupAutoSkipStoryMessage);
      this->mxEndUiDisableState( bDisable );
      // end Disable / Enable "auto skip action"


      // Force Heading
      missionx::WinImguiBriefer::HelpMarker("Force plane heading change even if we are close to the starting position.");
      ImGui::SameLine();
      ImGui::Checkbox("When positioning a plane, enforce heading even if start location is in 20m", &missionx::data_manager::flag_setupChangeHeadingEvenIfPlaneIn_20meter_radius);

      // Force plane position
      missionx::WinImguiBriefer::HelpMarker("Force the positioning of the plane even if it is near or at the starting location - will call XPLMPlaceUserAtLocation() function.");
      ImGui::SameLine();
      ImGui::Checkbox("Force plane positioning at mission start", &missionx::data_manager::flag_setupForcePlanePositioningAtMissionStart);

      ImGui::Separator(); // v3.305.1
      // v24.03.2
      missionx::WinImguiBriefer::mxUiHelpMarker(missionx::color::color_vec4_yellow, "Write weather datarefs to both missionx.log file and to the screen.");
      ImGui::SameLine();
      if (ImGui::TreeNode("Get Weather Information") )
      {
        static std::string sLastWeatherInformation;
        if (ImGui::Button("Get Current Weather Information"))
        {
          sLastWeatherInformation = missionx::data_manager::write_weather_state_to_log_file();
        }
        ImGui::InputTextMultiline("##WeatherInformation", sLastWeatherInformation.data(), sLastWeatherInformation.length(), ImVec2(550.0f, 70.0f), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
        ImGui::TreePop();
      } // end showing weather info on demand

      ImGui::Separator();


      // Dedicated buttons to print the parts of the mission
      missionx::WinImguiBriefer::HelpMarker("Dump key parts of the loaded XML mission file into the log file.");
      ImGui::SameLine();
      ImGui::TextColored(missionx::color::color_vec4_aqua, "%s", "The following buttons will dump the active mission XML parts into the Log file.\nYou can achieve the same by saving your work and checking the saved file.\nThat way you can understand what the plugin sees.");

      if (ImGui::Button("Global Settings"))
      {
        Log::printHeaderToLog("In Memory: Global Settings");
        Utils::xml_print_node(missionx::data_manager::mx_global_settings.node);
      }
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::Button("Briefer"))
      {
        Log::printHeaderToLog("In Memory: Briefer");
        Utils::xml_print_node(missionx::data_manager::briefer.node);
      }
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::Button("Datarefs"))
      {
        Log::printHeaderToLog("In Memory: Datarefs");
        std::for_each(missionx::data_manager::mapDref.begin(),
                      missionx::data_manager::mapDref.end(),
                      [](std::pair<const std::string, missionx::dataref_param>& pair)
                      {
                        pair.second.readDatarefValue_into_missionx_plugin();
                        Log::logMsg(fmt::format("Name: {:.<30}, Key: {:.<60}, Current Value: {}. ", pair.first, pair.second.key, pair.second.get_dataref_scalar_value_as_string()));
                      });
      }
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::Button("End Element"))
      {
        Log::printHeaderToLog("In Memory: End Element");
        Utils::xml_print_node(missionx::data_manager::endMissionElement.node);
      }

      ImGui::Separator();
      if (ImGui::Button("Flight Legs"))
      {
        Log::printHeaderToLog("In Memory: Flight Legs");
        std::for_each(missionx::data_manager::mapFlightLegs.begin(), missionx::data_manager::mapFlightLegs.end(), [](std::pair<const std::string, missionx::Waypoint>& pair) { Log::logMsg(fmt::format("Flight Leg Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text(pair.second.node))); });
      }
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::Button("Objectives"))
      {
        Log::printHeaderToLog("In Memory: Objectives");
        std::for_each(missionx::data_manager::mapObjectives.begin(), missionx::data_manager::mapObjectives.end(), [](std::pair<const std::string, missionx::Objective>& pair) { Log::logMsg(fmt::format("Objective Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text(pair.second.node))); });
      }
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::Button("Triggers"))
      {
        Log::printHeaderToLog("In Memory: Triggers");
        std::for_each(missionx::data_manager::mapTriggers.begin(),
                      missionx::data_manager::mapTriggers.end(),
                      [](std::pair<const std::string, missionx::Trigger>& pair)
                      {
                        Log::logMsg(fmt::format("Trigger Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text(pair.second.node)));
                      });
      }

      ImGui::Separator();
      if (ImGui::Button("Regular Messages"))
      {
        Log::printHeaderToLog("In Memory: Regular Messages");
        std::for_each(missionx::data_manager::mapMessages.begin(),
                      missionx::data_manager::mapMessages.end(),
                      [](std::pair<const std::string, missionx::Message>& pair)
                      {
                        if (pair.second.mode != missionx::mx_msg_mode::mode_story)
                          Log::logMsg(fmt::format("Message Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text(pair.second.node)));
                      });
      }
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::Button("Story Messages"))
      {
        Log::printHeaderToLog("In Memory: Story Messages");
        std::for_each(missionx::data_manager::mapMessages.begin(),
                      missionx::data_manager::mapMessages.end(),
                      [](std::pair<const std::string, missionx::Message>& pair)
                      {
                        if (pair.second.mode == missionx::mx_msg_mode::mode_story)
                          Log::logMsg(fmt::format("Message Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text(pair.second.node)));
                      });
      }
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::Button("Choices"))
      {
        Log::printHeaderToLog("In Memory: Choices");
        Utils::xml_print_node(missionx::data_manager::xmlChoices);
      }

      ImGui::Separator();
      if (ImGui::Button("Inventories"))
      {
        Log::printHeaderToLog("In Memory: Inventories");
        std::for_each(missionx::data_manager::mapInventories.begin(), missionx::data_manager::mapInventories.end(), [](std::pair<const std::string, missionx::Inventory>& pair) { Log::logMsg(fmt::format("Inventory Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text(pair.second.node))); });
      }
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::Button("3D Objects Templates"))
      {
        Log::printHeaderToLog("In Memory: 3D Object Templates");
        std::for_each(missionx::data_manager::map3dObj.begin(), missionx::data_manager::map3dObj.end(), [](std::pair<const std::string, missionx::obj3d>& pair) { Log::logMsg(fmt::format("3D Object Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text(pair.second.node))); });
      }
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::Button("Instanced 3D Objects"))
      {
        Log::printHeaderToLog("In Memory: Instanced 3D Objects");
        std::for_each(missionx::data_manager::map3dInstances.begin(), missionx::data_manager::map3dInstances.end(), [](std::pair<const std::string, missionx::obj3d>& pair) { Log::logMsg(fmt::format("3D Instance Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text(pair.second.node))); });
      }

      this->mxUiReleaseLastFont(); // v3.305.1

      ImGui::Separator(); // v3.305.1
      ImGui::NewLine();   // v3.305.1
    }



    this->mxUiReleaseLastFont(); // v3.305.1
  }
  ImGui::EndGroup(); // end setting UI groups

  ImGui::EndChild();
}




// ---------------------------------------------------
// ------------ draw home layer with Columns
// ---------------------------------------------------
void
WinImguiBriefer::draw_home_layer()
{
  static ImVec2 VEC2_BTN_SIZE = ImVec2(160.0f, 120.0f);

  // loop over all listMainBtn and display in buttons
  int                numButtons  = 0; // which number we draw in current row. Every time we overflow window we will need to reset this number.
  static constexpr float win_padding = 30.0f;

  ///////// Main Layer Buttons
  ImGui::BeginChild("draw_home_layer_01", ImVec2(0.0f, ImGui::GetWindowHeight() - this->fBottomToolbarPadding_f - this->fTopToolbarPadding_f - win_padding));

  //ImGui::SetWindowFontScale(1.2f);
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_springgreen); // green

  int columns = static_cast<int> (mxUiGetContentWidth () / VEC2_BTN_SIZE.x);
  columns     = columns < 1 ? 1 : columns;

  #ifdef ADD_TEST_BUTTON01
    add_test_button(mx_flc_pre_command::special_test_place_instance, "Place an Instance", "Place an instance at camera location.\nWe use file: \"Dir_Flood_Sm.obj\"");
    ImGui::Separator();
  #endif

  ImGui::Columns(columns, "draw_home_layer_columns", false);
  {
    for (auto& btn : this->listMainBtn)
    {
      numButtons++;

      ImGui::PushID(numButtons);

      // ------ Disable Decision
      bool disabled = false;
      if (missionx::data_manager::flag_apt_dat_optimization_is_running || OptimizeAptDat::aptState.flagIsActive || missionx::data_manager::flag_generate_engine_is_running) // includes all except SETUP
      {
        if (btn.layer != missionx::uiLayer_enum::option_setup_layer ) // disable all except "setup layer" and ILS, I would like to allow simmers to use the ILS to fetch info for ILS
          disabled = true;
      }

      else if (missionx::data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
      {
        if (   btn.layer != missionx::uiLayer_enum::flight_leg_info && btn.layer != missionx::uiLayer_enum::option_setup_layer
            && btn.layer != missionx::uiLayer_enum::option_conv_fpln_to_mission && btn.layer != missionx::uiLayer_enum::option_ils_layer)
          disabled = true;
      }

      if (btn.layer == missionx::uiLayer_enum::option_ils_layer)
      {
        this->strct_ils_layer.flagIgnoreDistanceFilter = false; // v24.03.1 reset "ignore distance" option.
        this->strct_ils_layer.to_icao.clear();
      }

      if (disabled)
      {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      }

      // v24.03.1 Different highlight for active mission for the "Briefer Info" layer
      if (btn.layer == missionx::uiLayer_enum::flight_leg_info && missionx::data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
      {
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_green);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_greenyellow);
      }


      // DISPLAY ICONS
      ImGui::BeginGroup();
      if (ImGui::ImageButton("DisplayIconsButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[btn.imgName].gTexture)), VEC2_BTN_SIZE, uv0, uv1, missionx::color::color_vec4_black))
      {
        this->clearMessage();

        if (btn.layer == missionx::uiLayer_enum::uiLayerUnset)
        {
          this->setMessage("Current button is not supported.");
        }
        else if (missionx::data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running &&
                 (btn.layer != missionx::uiLayer_enum::flight_leg_info && btn.layer != missionx::uiLayer_enum::option_setup_layer
                   && btn.layer != missionx::uiLayer_enum::option_conv_fpln_to_mission && btn.layer != missionx::uiLayer_enum::option_ils_layer))
        {
          this->setMessage("Mission is running. Can't open this screen !!!");
        }
        else
        {
          this->setLayer(btn.layer);


          // handle special actions per layer
          if (btn.layer == missionx::uiLayer_enum::option_user_generates_a_mission_layer)
          {
            // v3.303.8 initialize current clock when pressing the
            if (this->strct_user_create_layer.flag_first_time)
            {
              this->adv_settings_strct.iClockDayOfYearPicked = dataref_manager::getLocalDateDays();
              this->adv_settings_strct.iClockHourPicked      = dataref_manager::getLocalHour();
              this->strct_user_create_layer.flag_first_time  = false;
            }

            this->strct_user_create_layer.iMissionSubCategoryPicked = 0; // v25.04.1
            this->strct_user_create_layer.layer_state = missionx::mx_layer_state_enum::not_initialized;
            this->setMessage("Please wait while validating presence of the data...", 3);
            missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::imgui_check_validity_of_db_file);
          }
          else if (btn.layer == missionx::uiLayer_enum::option_generate_mission_from_a_template_layer)
          {
            this->strct_generate_template_layer.last_picked_template_key.clear(); // v24.12.2 Fix cases were re-entering template will not clean the right region and will show a phantom options from last pick.
            this->strct_generate_template_layer.layer_state = missionx::mx_layer_state_enum::not_initialized;
            this->setMessage("Please wait while validating presence of the data...", 3);
            missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::imgui_check_validity_of_db_file);
          }
          else if (btn.layer == missionx::uiLayer_enum::option_mission_list)
          {
            this->strct_pick_layer.last_picked_key.clear();
            this->strct_pick_layer.bFinished_loading_mission_images = false;
            this->setMessage("Please wait while loading mission files...", 8);
            missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::imgui_prepare_mission_files_briefer_info);
          }
          else if (btn.layer == missionx::uiLayer_enum::option_external_fpln_layer)
          {
            this->strct_user_create_layer.iMissionSubCategoryPicked = 0; // v25.04.1
          }
          else if (btn.layer == missionx::uiLayer_enum::option_ils_layer) // extra layer of caution even though it should be disabled by default. TODO: determine if to remove this test after behavior checks
          {
            if (missionx::data_manager::flag_apt_dat_optimization_is_running || OptimizeAptDat::aptState.flagIsActive)
            {
              this->setMessage("Please wait while APT.DAT optimization is running...", 8);
              this->setLayer(this->prevBrieferLayer);
            }
            else
            {
              this->strct_user_create_layer.iMissionSubCategoryPicked = 0; // v25.04.1
              this->strct_ils_layer.layer_state = missionx::mx_layer_state_enum::not_initialized;
              this->setMessage("Please wait while validating presence of the data...", 3);
              missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::imgui_check_presence_of_db_ils_data);
            }
          }
        } // end handle special layer
      }
      // Handle Tooltip

      // v24.03.1 popStyleColor for active mission button
      if (btn.layer == missionx::uiLayer_enum::flight_leg_info && missionx::data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
      {
        ImGui::PopStyleColor(2);
      }
      else
        this->strct_user_create_layer.iMissionSubCategoryPicked = 0; // v25.04.1


      // Tooltip
      if (!btn.tip.empty())
        this->mx_add_tooltip(missionx::color::color_vec4_yellow, btn.tip);

      //// Handle button label location
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
      {
        if (btn.layer == missionx::uiLayer_enum::flight_leg_info && missionx::data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
          ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_aqua);

        ImGui::Text("%s", btn.label.c_str());

        if (btn.layer == missionx::uiLayer_enum::flight_leg_info && missionx::data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
          ImGui::PopStyleColor();
      }

      this->mxUiReleaseLastFont();

      if (disabled) // v24.06.1 moved from before the tooltip to after the button labels to better represent "inaccessibility".
      {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
      }

      ImGui::EndGroup();
      ImGui::PopID();

      if (numButtons % columns == 0)
      {
        ImGui::NewLine();
      }

      ImGui::NextColumn();
    } // end loop over buttons
  }   // end columns

  ImGui::PopStyleColor(1);

  ImGui::EndChild();
}

// ---------------------------------------------------

void
WinImguiBriefer::draw_dynamic_mission_creation_screen()
{
  const auto win_size_vec2 = this->mxUiGetWindowContentWxH();

  if (this->strct_user_create_layer.layer_state == missionx::mx_layer_state_enum::success_can_draw)
  {
    static constexpr float img_ps_f = 0.5f;
    float                  pos_x    = data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_LAB_24X18 ()].sImageData.getW_f () * img_ps_f;

    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());

    ImGui::BeginChild("draw_user_create_mission_layer_01", ImVec2(0.0f, win_size_vec2.y * 0.77f), ImGuiChildFlags_Borders);
    {
      ///////// Image ////////
      ImGui::BeginGroup();
      ImGui::Image(reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_LAB_24X18()].gTexture)), ImVec2(data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_LAB_24X18()].sImageData.getW_f() * img_ps_f, data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_LAB_24X18()].sImageData.getH_f() * img_ps_f));
      ImGui::EndGroup();

      pos_x += 50.0f;
      ImGui::SameLine(pos_x);
      ImGui::BeginGroup();

      //------------------------------------------------
      //                  Topic 1 - Header
      //------------------------------------------------

      // label
      // v3.0.253.9 explanation
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellowgreen); //
      HelpMarker("For best results - 'run APT.dat optimization' at least once or after XP update or a new scenery is added.\nYou can find it in the SETUP screen.");
      ImGui::PopStyleColor(1);
      ImGui::SameLine(0.0f, 2.0f);

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text(" Pick Type of Mission:");
      ImGui::PopStyleColor(1);

      ImGui::SameLine(0.0f, 50.0f);
      this->add_ui_advance_settings_random_date_time_weather_and_weight_button2(this->adv_settings_strct.iClockDayOfYearPicked, this->adv_settings_strct.iClockHourPicked, this->adv_settings_strct.iClockMinutesPicked); // v3.303.10 convert the random dateTime button to a self contain function

      if (ImGui::RadioButton("Medevac", this->strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (missionx::mx_mission_type::medevac)))
      {
        this->strct_user_create_layer.iRadioMissionTypePicked   = static_cast<int> (missionx::mx_mission_type::medevac);
        this->strct_user_create_layer.iMissionSubCategoryPicked = 0; // reset sub category combo
        this->strct_user_create_layer.flag_use_web_osm          = true;
        this->strct_user_create_layer.iRadioPlaneType           = missionx::mx_plane_types::plane_type_helos;

        this->refresh_slider_data_based_on_plane_type(this->strct_user_create_layer.iRadioPlaneType); // v3.303.14
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Cargo", this->strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (missionx::mx_mission_type::cargo)))
      {
        this->strct_user_create_layer.iRadioMissionTypePicked   = static_cast<int> (missionx::mx_mission_type::cargo);
        this->strct_user_create_layer.iMissionSubCategoryPicked = 0; // reset sub category combo
        this->strct_user_create_layer.flag_use_web_osm          = false;
      }

      ImGui::SameLine();
      if (ImGui::RadioButton("Oil Rig", this->strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (missionx::mx_mission_type::oil_rig)))
      {
        this->strct_user_create_layer.iRadioMissionTypePicked   = static_cast<int> (missionx::mx_mission_type::oil_rig);
        this->strct_user_create_layer.iMissionSubCategoryPicked = 0; // reset sub category combo
        this->strct_user_create_layer.flag_use_web_osm          = false;
        this->strct_user_create_layer.iRadioPlaneType           = missionx::mx_plane_types::plane_type_helos;

        this->refresh_slider_data_based_on_plane_type(this->strct_user_create_layer.iRadioPlaneType); // v3.303.14
      }
      ImGui::SameLine();
      missionx::WinImguiBriefer::HelpMarker("The plugin will place you randomly in a location near an Oil Rig.\nThe Oil Rig data is highly dependent on the Oil Rig information found in the apt.dat files, and not on what you see in your maps.");


      // sub categories
      {
        const auto vecToDisplay = this->mapMissionCategories[this->strct_user_create_layer.iRadioMissionTypePicked];
        this->add_ui_pick_subcategories(vecToDisplay); // v25.04.1
      }

      ImGui::Separator();
      ImGui::NewLine();

      const auto lmbda_get_title = [](int& inMissionType)
      {
        switch (static_cast<missionx::mx_mission_type> (inMissionType))
        {
          case missionx::mx_mission_type::medevac:
            return "Medevac Settings";
            break;
          case missionx::mx_mission_type::cargo:
            return "Cargo Settings";
            break;
          case missionx::mx_mission_type::oil_rig:
            return "Oil Rig Settings";
            break;
          default:
            break;
        }

        return "";
      };
      const std::string Topic2_title = lmbda_get_title(this->strct_user_create_layer.iRadioMissionTypePicked);

      //------------------------------------------------
      //                  Topic 2 - plane type and area filter
      //------------------------------------------------

      //ImGui::PushStyleColor(ImGuiCol_Header, missionx::color::color_vec4_beige);
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, missionx::color::color_vec4_azure);
      ImGui::PushStyleColor(ImGuiCol_Header, missionx::color::color_vec4_coral);
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_navy); // Text

      if (ImGui::CollapsingHeader(Topic2_title.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // dynamic title
      {
        // ImGui::GetStateStorage()->SetInt(ImGui::GetID("Other Settings"), 0);     // collapse "Other Settings"

        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_white); // internal color

        // ------------------------
        // -- Skewed 3D Markers
        // ------------------------
        if (this->strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (missionx::mx_mission_type::medevac)) // v3.0.253.6
        {
          add_skewed_marker_checkbox();
          ImGui::NewLine();
        }

        // ------------------------
        // -- Preferred Plane
        // ------------------------
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
        ImGui::Text("Pick Preferred Plane:");
        ImGui::PopStyleColor(1);

        for (const auto& planeTypeLabel : this->mapListPlaneRadioLabel | std::views::values) // v24.12.1
        {

          // Filter out all plane types that are not Helos for Medevac and Oil Rig missions
          if (this->strct_user_create_layer.iRadioMissionTypePicked != static_cast<int> (missionx::mx_mission_type::cargo) && planeTypeLabel.type != mx_plane_types::plane_type_helos)
            continue;

          if (ImGui::RadioButton(planeTypeLabel.label.c_str(), this->strct_user_create_layer.iRadioPlaneType == planeTypeLabel.type))
          {
            this->strct_user_create_layer.iRadioPlaneType = planeTypeLabel.type;
            this->refresh_slider_data_based_on_plane_type(this->strct_user_create_layer.iRadioPlaneType); // v24.12.1 deprecated
          }
          ImGui::SameLine();
        }

        //// v3.0.253.11 added the PROP_START_FROM_PLANE_POSITION checkbox

        // ------------------------
        // -- Helos only option to narrow ramp type
        // ------------------------
        if (this->strct_user_create_layer.iRadioPlaneType == missionx::mx_plane_types::plane_type_helos)
        {
          ImGui::NewLine();
          ImGui::Checkbox("Narrow helos ramp locations", &strct_user_create_layer.flag_narrow_helos_filtering);
          ImGui::NewLine();
        }
        else
        {
          // ------------------------
          // -- Pick Runway Type
          // ------------------------
          auto count_filters_picked = static_cast<size_t> (0);
          // Runway filter
          ImGui::NewLine();

          ImGui::Checkbox("Pick Any Runway##filterRunways", &this->strct_user_create_layer.flag_pick_any_rw);
          // display the filter tree if changed to false
          if (this->strct_user_create_layer.flag_pick_any_rw == false)
          {
            ImGui::Text("Include airports that have:");

            for (auto& [rw_lbl, b_picked] : this->strct_user_create_layer.map_filter_runways)
            {
              ImGui::SameLine(0.0f, 10.0f); //
              ImGui::Checkbox(rw_lbl.c_str(), &b_picked);
              if (b_picked) // if picked
                count_filters_picked++;
            }

            if (count_filters_picked == this->strct_user_create_layer.map_filter_runways.size())
              this->strct_user_create_layer.reset_filter_runways_flags();
          }

          ImGui::NewLine();
        }


        //------------------------------------------------
        //           Row 3 - sliders
        //------------------------------------------------
        const static std::string popupOverpassWindowName = "overpass filter popup";

        if (this->strct_user_create_layer.iRadioMissionTypePicked != static_cast<int>(missionx::mx_mission_type::oil_rig))
        {
          ImGui::Columns(2);
          {
            ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
            ImGui::Text("Pick Minimum Flight Leg Distance: ");
            ImGui::PopStyleColor(1);
            if (ImGui::SliderFloat(strct_user_create_layer.dyn_slider1_lbl.c_str(), &strct_user_create_layer.dyn_sliderVal1, this->mapListPlaneRadioLabel[this->strct_user_create_layer.iRadioPlaneType].from_slider_min, this->mapListPlaneRadioLabel[this->strct_user_create_layer.iRadioPlaneType].from_slider_max, "%.2f nm") ){
              if (strct_user_create_layer.dyn_sliderVal2 <= strct_user_create_layer.dyn_sliderVal1)
                strct_user_create_layer.dyn_sliderVal2 = strct_user_create_layer.dyn_sliderVal1 + 100.0f;

              this->validate_sliders_values(this->strct_user_create_layer.iRadioPlaneType);
            }

            ImGui::NextColumn();
            {
              ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
              ImGui::Text("Pick Maximum Flight Leg Distance: ");
              ImGui::PopStyleColor(1);
              if (ImGui::SliderFloat(strct_user_create_layer.dyn_slider2_lbl.c_str(), &strct_user_create_layer.dyn_sliderVal2, this->mapListPlaneRadioLabel[this->strct_user_create_layer.iRadioPlaneType].to_slider_min, this->mapListPlaneRadioLabel[this->strct_user_create_layer.iRadioPlaneType].to_slider_max, "%.2f nm"))
              {
                if (strct_user_create_layer.dyn_sliderVal1 > strct_user_create_layer.dyn_sliderVal2)
                  strct_user_create_layer.dyn_sliderVal1 = strct_user_create_layer.dyn_sliderVal2 - 100.0f;

                this->validate_sliders_values(this->strct_user_create_layer.iRadioPlaneType);

              }
            }
          }
          ImGui::Columns();


          if (strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (_mission_type::medevac) &&
              data_manager::mapMissionCategoriesCodes.at(strct_user_create_layer.iRadioMissionTypePicked).at(this->strct_user_create_layer.iMissionSubCategoryPicked) > missionx::mx_mission_subcategory_type::med_any_location) // display OSM WEB and OSM DB only if it is not cargo type
          {
            ImGui::NewLine();

            strct_user_create_layer.flag_use_web_osm = true;
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Checkbox("Use web OSM", &strct_user_create_layer.flag_use_web_osm);
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();

            this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Prefer search from the web over local OSM database search.");
            if (strct_user_create_layer.flag_use_web_osm)
            {
              if (missionx::data_manager::flag_generate_engine_is_running) // v3.0.255.4 fix bug where user exit the popup window and it crashes x-plane during mission creation in the background
              {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
              }

              ImGui::SameLine(0.0f, 10.0f);
              if (ImGui::Button("filter ..."))
              {
                ImGui::OpenPopup(popupOverpassWindowName.c_str());
              }
              this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Advance options to filter the data fetched");

              if (missionx::data_manager::flag_generate_engine_is_running) // v3.0.255.4 fix bug where user exit the popup window and it crashes x-plane during mission creation in the background
              {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
              }
            }

            ImGui::SameLine(0.0f, 150.0f);

            ImGui::Checkbox("Use local OSM db file", &strct_user_create_layer.flag_use_osm);
            this->mx_add_tooltip(missionx::color::color_vec4_yellow, "If OSM database is available, then try to get data from it before the Web OSM.");

            //// OSM Web filter popup
            // auto vec2WindowSize = this->mxUiGetWindowContentWxH();
            ImGui::SetNextWindowSize(ImVec2(590.0f, 230.0f));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_black);
            {
              if (ImGui::BeginPopupModal(popupOverpassWindowName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
              {
                constexpr static const int OSM_BUFF_SIZE_I           = 2047;
                static bool                b_first_time              = true;
                static char                buf1[OSM_BUFF_SIZE_I + 1] = "";

                if (b_first_time) // reset main overpass filter
                {
                  // std::string err;
                  std::string stored_filter = Utils::getNodeText_type_6(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_FILTER(), mxconst::get_DEFAULT_OVERPASS_WAYS_FILTER());
                  if (stored_filter.empty())
                  {
                    stored_filter = this->strct_user_create_layer.overpass_original_filter;
                    Utils::xml_search_and_set_node_text(system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_FILTER(), stored_filter, this->mxcode.STRING, true); // "6" = string type
                    this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
                  }

                  std::memcpy(buf1, stored_filter.c_str(), OSM_BUFF_SIZE_I);

                  this->strct_user_create_layer.overpass_main_filter = stored_filter;
                  b_first_time                                       = false;
                }

                ImGui::BeginGroup();
                ImGui::BeginChild("popupOverpassWindow##PopupOverpassWindowName");
                {
                  int iStyle = 0;
                  ImGui::BeginChild("internal##PopupOverpassWindowName");

                  ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
                  iStyle++;
                  ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_dodgerblue);
                  iStyle++;
                  ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_white);
                  iStyle++;
                  ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_TextSelectedBg, missionx::color::color_vec4_black);
                  iStyle++;

                  ImGui::PushItemWidth(500.0f);
                  ImGui::TextColored(missionx::color::color_vec4_yellow, "Define informational filters to impact requested data from overpass site.\nFormat must be \"way[key=value]({{bbox}});\"  Fill in the key/value.\nFor space use \"%%20\" or \"+\": \";out%%20center;\"\n");
                  ImGui::PopItemWidth();

                  ImGui::NewLine();

                  /////////////////////
                  // main filter
                  ////////////////////
                  ImGui::TextColored(missionx::color::color_vec4_yellow, "Ways filter length:");
                  ImGui::SameLine();
                  ImGui::TextColored(missionx::color::color_vec4_beige, "%i chars", static_cast<int>(std::string(buf1).length()));
                  ImGui::SameLine(this->mxUiGetContentWidth() - 40.0f, 0.0f);
                  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
                  if (ImGui::Button("reset##resetButton1"))
                  {
                    std::memcpy(buf1, this->strct_user_create_layer.overpass_original_filter.c_str(), OSM_BUFF_SIZE_I);
                    this->strct_user_create_layer.overpass_pre_apply_filter_s = this->strct_user_create_layer.overpass_original_filter;
                  }
                  this->mxUiReleaseLastFont();

                  ImGui::BeginChild("overpassFilter##PopupOverpassWindowName", ImVec2(0.0f, 55.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysHorizontalScrollbar); // InputTextWithHint

                  ImGui::PushItemWidth(static_cast<float>(OSM_BUFF_SIZE_I) + 1.0f);
                  ImGui::PushID("##overpassCustomFilter");
                  if (ImGui::InputTextWithHint("", "way['key'='value']({{bbox}});out;", buf1, OSM_BUFF_SIZE_I)) // , ImGuiInputTextFlags_CharsNoBlank)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
                  {
                    this->strct_user_create_layer.overpass_pre_apply_filter_s = std::string(buf1);
                  }
                  ImGui::PopID();
                  ImGui::PopItemWidth();

                  ImGui::EndChild();


                  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
                  ImGui::Separator();

                  ImGui::SameLine(0.0f, 20.0f);
                  ImGui::SetItemDefaultFocus();
                  if (ImGui::Button("Apply", ImVec2(150, 0)))
                  {
                    this->strct_user_create_layer.overpass_pre_apply_filter_s = std::string(buf1);
                    this->strct_user_create_layer.overpass_main_filter        = this->strct_user_create_layer.overpass_pre_apply_filter_s;
                    Utils::xml_search_and_set_node_text(system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_FILTER(), this->strct_user_create_layer.overpass_main_filter, this->mxcode.STRING, true); // "6" = string type


                    this->execAction(missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);

                    ImGui::CloseCurrentPopup();
                  }
                  ImGui::SameLine(0.0f, 20.0f);
                  ImGui::SetItemDefaultFocus();
                  if (ImGui::Button("Cancel", ImVec2(90, 0)))
                  {
                    std::memcpy(buf1, this->strct_user_create_layer.overpass_main_filter.c_str(), OSM_BUFF_SIZE_I);
                    this->strct_user_create_layer.overpass_pre_apply_filter_s = this->strct_user_create_layer.overpass_main_filter;
                    ImGui::CloseCurrentPopup();
                  }

                  this->mxUiReleaseLastFont();

                  ImGui::PopStyleColor(iStyle); // pop out before EndPopup

                  ImGui::EndChild();
                }

                ImGui::EndChild();
                ImGui::EndGroup();

                ImGui::EndPopup();
              } // end quit popup
            }
            ImGui::PopStyleColor(); // child background
          }                         // end OSM checkboxes
          else
          {
            strct_user_create_layer.flag_use_web_osm = false;
          }
        } // end if not Oil Rig

        ImGui::PopStyleColor(1); // disable internal text color

        ImGui::NewLine();

        // ------------------------
        // -- How Many Flight Legs ?
        // ------------------------
        int iMinLegsVal = 1;
        int iMaxLegsVal = 4;
        // calculate default number of flight legs per mission and plane type
        if ((strct_user_create_layer.iRadioPlaneType == missionx::mx_plane_types::plane_type_helos && strct_user_create_layer.iRadioMissionTypePicked == static_cast<int>(missionx::_mission_type::medevac)) || (this->strct_user_create_layer.iRadioMissionTypePicked == static_cast<int>(missionx::mx_mission_type::oil_rig)))
        {
          iMinLegsVal                                = 2;
          iMaxLegsVal                                = 2;
          strct_user_create_layer.iNumberOfFlighLegs = iMinLegsVal; // we're automatically resetting the starting/picked legs to 2 and not 1
        }
        // v24.12.1 jets and heavies have only "one" flight leg.
        else if (strct_user_create_layer.iRadioPlaneType > missionx::mx_plane_types::plane_type_turboprops && strct_user_create_layer.iRadioMissionTypePicked == static_cast<int>(missionx::_mission_type::cargo))
        {
          iMinLegsVal                                = 1;
          iMaxLegsVal                                = 1;
          strct_user_create_layer.iNumberOfFlighLegs = iMinLegsVal; // we're automatically resetting the starting/picked legs to only one, for jets and heavies.
        }

        if (this->strct_user_create_layer.iRadioMissionTypePicked != static_cast<int>(missionx::mx_mission_type::medevac) && this->strct_user_create_layer.iRadioMissionTypePicked != static_cast<int>(missionx::mx_mission_type::oil_rig) && strct_user_create_layer.iRadioPlaneType < missionx::mx_plane_types::plane_type_jets)
        {
          ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
          ImGui::Text("How Many Flight Legs ? ");                                   // between 2 and 4
          ImGui::PopStyleColor(1);

          ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_white); // internal color
          for (int i = iMinLegsVal; i <= iMaxLegsVal; ++i)
          {
            if (ImGui::RadioButton(Utils::formatNumber<int>(i).c_str(), strct_user_create_layer.iNumberOfFlighLegs == i))
              strct_user_create_layer.iNumberOfFlighLegs = i;

            ImGui::SameLine();
          }
          ImGui::PopStyleColor();

          ImGui::NewLine();
        }

      } // End Collapse Topic 2 & 3

      //------------------------------------------------
      //     Topic 4 (basically the columns have few rows, but we count them as 1)
      //------------------------------------------------

      if (ImGui::CollapsingHeader("Other Settings"))
      {

        // ------------------------
        // Compatibility - Inventory XP11
        // ------------------------
        this->add_ui_xp11_comp_checkbox(false); // v24.12.2
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_white); // internal color

        // ------------------------
        // -- Countdown
        // ------------------------
        if ((strct_user_create_layer.iRadioPlaneType == missionx::mx_plane_types::plane_type_helos && strct_user_create_layer.iRadioMissionTypePicked == static_cast<int>(missionx::_mission_type::medevac)) || (this->strct_user_create_layer.iRadioMissionTypePicked == static_cast<int>(missionx::mx_mission_type::oil_rig)))
        {
          // ImGui::SameLine(0.0f, 100.0f);
          ImGui::Checkbox("Add Countdown", &this->strct_setup_layer.bAddCountdown);
          this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Add random countdown to each leg to add to the challenge.");

          ImGui::Separator();
        }
        else
          ImGui::NewLine();


        // ------------------------
        // -- Suppress Distance Messages
        // ------------------------
        this->add_ui_suppress_distance_messages_checkbox_ui (); // v25.02.1
        ImGui::Separator ();

        // ------------------------
        // -- GPS Waypoints
        // ------------------------

        ImGui::Checkbox("Generate GPS waypoints.", &this->strct_cross_layer_properties.flag_generate_gps_waypoints); // v3.0.253.12
        ImGui::SameLine(0.0f, 90.0f);
        this->add_ui_expose_all_gps_waypoints();

        this->add_ui_auto_load_checkbox (); // v25.04.2

        // ------------------------
        // -- Default Weight
        // ------------------------
        ImGui::NewLine();
        // ImGui::Checkbox("Add default base weights.\n(Not advisable for planes > GAs)", &this->adv_settings_strct.flag_add_default_weight_settings);
        this->add_ui_default_weights();

        ImGui::PopStyleColor(1); // pop-out internal text color settings
      }                          // end "Other Settings" Collapsing Header

      ImGui::PopStyleColor(3); // end Collapse Header special color - button hovered,button,text,text selected bg

      ImGui::NewLine();
      ImGui::NewLine();

      ImGui::EndGroup();
    }
    ImGui::EndChild(); // end main dynamic creation area child

    this->mxUiReleaseLastFont();

    // ------------------------
    // ------------------------
    // ------------------------
    //     Footer Buttons
    // ------------------------


    // ImGui::NewLine(); // v3.305.1 removed
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
    ImGui::BeginGroup();
    {

      if (missionx::data_manager::flag_generate_engine_is_running && this->sBottomMessage.empty())
      {
        this->setMessage("Random Engine is running, please wait...");
      }
      else if (missionx::data_manager::flag_apt_dat_optimization_is_running && sBottomMessage.empty())
      {
        this->setMessage("Can't Generate mission, apt dat optimization is currently running. Please wait for it to finish first !!!");
      }
      else if (!(missionx::data_manager::flag_generate_engine_is_running || missionx::data_manager::flag_apt_dat_optimization_is_running))
      {
        // Regenerate Random Date Time // v3.303.10
        static bool bRerunRandomDateTime{ false };
        bRerunRandomDateTime = add_ui_checkbox_rerun_random_date_and_time();

        ImGui::SameLine();

        this->flag_generatedRandomFile_success = false;
        constexpr auto lbl              = "Generate a Mission Based on User Preferences";

        // -----------------------
        // DISPLAY GENERATE BUTTON
        // -----------------------
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_orange);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_azure);
        if (ImGui::Button(lbl))
        {
          if (bRerunRandomDateTime) // v3.303.10
            this->execAction(missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);

          this->selectedTemplateKey = mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI();

          IXMLNode node_ptr = missionx::data_manager::prop_userDefinedMission_ui.node;

          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_MED_CARGO_OR_OILRIG(), strct_user_create_layer.iRadioMissionTypePicked);
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_MISSION_SUBCATEGORY(), strct_user_create_layer.iMissionSubCategoryPicked); // v3.303.14
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_PLANE_TYPE_I(), static_cast<int>(strct_user_create_layer.iRadioPlaneType));
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_NO_OF_LEGS(), strct_user_create_layer.iNumberOfFlighLegs);
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double>(mxconst::get_PROP_MIN_DISTANCE_SLIDER(), (double)strct_user_create_layer.dyn_sliderVal1);
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double>(mxconst::get_PROP_MAX_DISTANCE_SLIDER(), (double)strct_user_create_layer.dyn_sliderVal2);
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_ADD_COUNTDOWN(), false); // v3.0.253.9.1 default value for helos/planes + cargo missions
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_GENERATE_GPS_WAYPOINTS(), this->strct_cross_layer_properties.flag_generate_gps_waypoints);

          // v24.03.1 Sub Category Text
          auto vecToDisplay = this->mapMissionCategories[this->strct_user_create_layer.iRadioMissionTypePicked];
          // Store the label of the sub category, if the vector has the data
          if (vecToDisplay.size() >= this->strct_user_create_layer.iMissionSubCategoryPicked)
            missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_MISSION_SUBCATEGORY_LBL(), vecToDisplay.at(this->strct_user_create_layer.iMissionSubCategoryPicked));


          // v3.0.253.6 no osm search for cargo missions
          if (strct_user_create_layer.iRadioMissionTypePicked != static_cast<int>(_mission_type::medevac)) // USE OSM WEB or OSM DB only if it is not cargo type
          {
            strct_user_create_layer.flag_use_osm     = false;
            strct_user_create_layer.flag_use_web_osm = false;
          }
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_USE_OSM_CHECKBOX(), strct_user_create_layer.flag_use_osm);
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_USE_WEB_OSM_CHECKBOX(), strct_user_create_layer.flag_use_web_osm);

          if (this->strct_user_create_layer.iRadioPlaneType == missionx::mx_plane_types::plane_type_helos) // v3.0.253.9.1 Add specific Helos properties based on user definitions and restrictions // v3.0.253.7 narrow helos ramp filtering,
                                                                                                           // means to store mainly rw code 101 and airports that have [H] in them ignore ramp code 1300 that are for "helos"
          {
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_NARROW_HELOS_RAMP_SEARCH(), strct_user_create_layer.flag_narrow_helos_filtering); //, node_ptr, node_ptr.getName());
            // v3.0.253.1.9 added countdown for helos + medevac
            if (strct_user_create_layer.iRadioMissionTypePicked == static_cast<int>(missionx::_mission_type::medevac) || strct_user_create_layer.iRadioMissionTypePicked == static_cast<int>(missionx::_mission_type::oil_rig))
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_ADD_COUNTDOWN(), this->strct_setup_layer.bAddCountdown);
          }
          else
          {
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_NARROW_HELOS_RAMP_SEARCH(), false);
          }

          // v3.303.12 weather
          this->addAdvancedSettingsPropertiesBeforeGeneratingRandomMission();


          const auto lmbda_build_filter_for_runway_types = [&]()
          {
            int         counter{ 0 };
            std::string query;
            query.clear();
            if (this->strct_user_create_layer.iRadioPlaneType == missionx::mx_plane_types::plane_type_helos || this->strct_user_create_layer.flag_pick_any_rw)
              return query; // empty string no filter

            for (auto& [rw_type, b_picked] : this->strct_user_create_layer.map_filter_runways)
            {
              if (b_picked)
              {
                if (counter == 0)
                  query = " ( ";
                else
                  query += ", ";

                query += this->strct_user_create_layer.map_filter_runways_translate_to_numbers[rw_type];
                counter++;
              }
            }

            if (counter > 0)
              query += " )";

            #ifndef RELEASE
                        Log::logMsg("Query filter: " + query);
            #endif // !RELEASE

            return query;
          }; // end lambda


          // PROP_FILTER_AIRPORTS_BY_RUNWAY_TYPE - prepare the runway filter string
          #ifndef RELEASE
          const std::string filter_query = lmbda_build_filter_for_runway_types();
          missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_FILTER_AIRPORTS_BY_RUNWAY_TYPE(), filter_query); //, node_ptr, node_ptr.getName());
          #else
          missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_FILTER_AIRPORTS_BY_RUNWAY_TYPE(), lmbda_build_filter_for_runway_types()); //, node_ptr, node_ptr.getName());
          #endif

          this->asyncSecondMessageLine.clear();
          this->setMessage("Random Engine is running, please wait...", 10);
          this->execAction(missionx::mx_window_actions::ACTION_GENERATE_RANDOM_MISSION); // should hide the window
        }

        ImGui::PopStyleColor(3);
      }
      else if (missionx::RandomEngine::threadState.flagIsActive)
      {

        missionx::WinImguiBriefer::HelpMarker("Abort the background process.");
        ImGui::SameLine(25.0f, 0.0f);
        this->add_ui_abort_mission_creation_button(); // Add Abort Random Engine
      }

      if (!this->asyncSecondMessageLine.empty())
      {
        ImGui::SameLine((mxUiGetContentWidth() * 0.75f) - (ImGui::CalcTextSize(this->LBL_START_MISSION.c_str()).x * 0.5f));
        this->add_ui_start_mission_button(missionx::mx_window_actions::ACTION_START_RANDOM_MISSION);
      }

      ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);

    }
    ImGui::EndGroup();
    this->mxUiReleaseLastFont();


  } // end if (this->strct_user_create_layer.layer_state == missionx::mx_layer_state_enum::success_can_draw)
  else if (this->strct_user_create_layer.layer_state == missionx::mx_layer_state_enum::failed_data_is_not_present || this->strct_user_create_layer.layer_state == missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly)
  {
    /// TODO: consider removing this part of code since we always force the creation of the SQLite DB in order to use the plugin features.
    ImGui::NewLine();

    if (this->strct_user_create_layer.layer_state == missionx::mx_layer_state_enum::failed_data_is_not_present)
    {
      this->display_shared_message_when_optimized_data_is_not_present(missionx::mx_layer_state_enum::failed_data_is_not_present);
    }
    else
    {
      this->display_shared_message_when_optimized_data_is_not_present(missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly);
    }
  }
  else // display wait
  {
    ImGui::NewLine();

    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_BIG());
    ImGui::TextColored(missionx::color::color_vec4_magenta, "Please wait while testing validity of the data.... ");
    this->mxUiReleaseLastFont(); // v3.303.14 release the last 2 fonts we pushed


    if (this->strct_user_create_layer.layer_state < missionx::mx_layer_state_enum::validating_data)
    {
      this->strct_user_create_layer.layer_state = missionx::mx_layer_state_enum::validating_data;
    }
  }


} // draw_dynamic_mission_creation_screen

// ---------------------------------------------------

// ---------- draw_user_create_mission_layer() --------------

void
WinImguiBriefer::draw_template_mission_generator_screen()
{
  // auto win_size_vec2 = ImGui::GetContentRegionAvail();
  auto win_size_vec2 = this->mxUiGetWindowContentWxH();

  if (this->strct_generate_template_layer.layer_state == missionx::mx_layer_state_enum::success_can_draw)
  {
    float pos_x = 0.0f;

    ///////// Header ////////
    ImGui::BeginGroup();
    ImGui::BeginChild("draw_generate_layer_01", ImVec2(0.0f, imvec2_pick_template_top_area_size.y));
    ImGui::SameLine(pos_x + 5);
    HelpMarker("Pick one of the templates images and read its description. You can then press the [generate] button to create a random mission based on the template.\nFor best results - 'run APT.dat optimization' at least once. You can "
               "find it in the SETUP screen."); // v3.0.253.9 added optimization wording
    ImGui::SameLine();

    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14

    ImGui::TextColored(missionx::color::color_vec4_yellow, "Please pick a template.");
    ImGui::SameLine(0.0f, 135.0f);
    add_skewed_marker_checkbox(); // v3.0.253.6

    this->mxUiReleaseLastFont(); // v3.303.14

    ImGui::EndChild();
    ImGui::EndGroup();

    //////////////////////////////////////////////////////////////////////////
    ///////// List of images and their details when pressed on image ////////

    if (this->strct_generate_template_layer.bFinished_loading_templates)
    {
      //
      // Holds the vertical regions width, [0]=Left, [1]=Right
      constexpr static const float fLeftRegionSize = 300.0f;
      constexpr const char*        names[]         = { "Templates", "Template Description" };

      constexpr float region_width_arr[] = { fLeftRegionSize, 0.0f };

      ImGui::PushID("##VerticalScrolling");

      for (int i = 0; i < 2; ++i)
      {
        if (i > 0)
          ImGui::SameLine();
        ImGui::BeginGroup();

        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
        {
          ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", names[i]);
          if (i == 1)
          {
            ImGui::SameLine(0.0f, 10.0f);
            add_font_size_scale_buttons();
            this->mxUiReleaseLastFont(); // v3.303.14

            // v3.303.12
            ImGui::SameLine(0.0f, 20.0f);
            this->add_ui_advance_settings_random_date_time_weather_and_weight_button2(this->adv_settings_strct.iClockDayOfYearPicked, this->adv_settings_strct.iClockHourPicked, this->adv_settings_strct.iClockMinutesPicked, mxconst::get_TEXT_TYPE_TITLE_SMALLEST());
          }
          else
          {
            ImGui::Spacing();            // v3.303.14
            this->mxUiReleaseLastFont(); // v3.303.14
          }
        }
        // Draw 2 regions
        constexpr auto combo_label_s = "Select an Option";

        const ImGuiID child_id         = ImGui::GetID(reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        const bool    child_is_visible = ImGui::BeginChild(child_id, ImVec2(region_width_arr[i], win_size_vec2.y * 0.66f), ImGuiChildFlags_Borders);
        if (child_is_visible) // Avoid calling SetScrollHereY when running with culled items
        {
          if (i == 0) // region 1 - vertical
          {
            // DISPLAY IMAGES
            int imgNo = 0; // v3.303.14
            for (const auto& key : data_manager::mapGenerateMissionTemplateFiles | std::views::keys)
            {
              int         iStyle       = 0;
              // std::string key          = key; // key
              bool        bSetKeyStyle = false;

              if (!key.empty() && (this->selectedTemplateKey == key || this->strct_generate_template_layer.last_picked_template_key == key))
              {
                bSetKeyStyle = true;
                ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_green);
                iStyle++; //
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_greenyellow);
                iStyle++; //
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_green);
                iStyle++;
              }

              // Draw template image button
              ImGui::PushID(imgNo);
              {
                if (!key.empty() && data_manager::mapGenerateMissionTemplateFiles[key].imageFile.gTexture && ImGui::ImageButton("##templateImg", (void*)static_cast<intptr_t> (data_manager::mapGenerateMissionTemplateFiles[key].imageFile.gTexture), ImVec2(240.0f, 190.0f), this->uv0, this->uv1))
                {
                  // prepare template briefer text so we will display it in the detailed region
                  this->selectedTemplateKey                                    = key;
                  this->strct_generate_template_layer.last_picked_template_key = key;
                  this->setMessage("Picked Template: " + this->strct_generate_template_layer.last_picked_template_key);

                  // v3.0.255.4 display template inject options
                  auto* pickedTemplateInfo_ptr = &missionx::data_manager::mapGenerateMissionTemplateFiles[this->strct_generate_template_layer.last_picked_template_key];
                  assert(pickedTemplateInfo_ptr != nullptr && "1. Failed to map mapGenerateMissionTemplateFiles pointer!!!");

                  size_t longestText_i               = 0;
                  size_t longestTextInVector_array_i = 0;
                  size_t indx                        = 0;
                  this->strct_generate_template_layer.vecReplaceOptions_char.clear();
                  //this->strct_generate_template_layer.mapReplaceOption_ui.clear(); // v24.12.2

                  //this->strct_generate_template_layer.mapReplaceOption_ui[0]; // initialize <opt> key.
                  for (auto& v : pickedTemplateInfo_ptr->vecReplaceOptions_s)
                  {
                    this->strct_generate_template_layer.vecReplaceOptions_char.emplace_back(v.c_str()); // store opt keyName
                    if (v.length() > longestText_i)                                                     // v3.0.303 calculate which cell has the longest text
                    {
                      longestTextInVector_array_i = indx;
                      longestText_i               = v.length();
                    }
                    ++indx;
                  }

                  pickedTemplateInfo_ptr->size_of_vecReplaceOptions_n                       = this->strct_generate_template_layer.vecReplaceOptions_char.size();
                  this->strct_generate_template_layer.user_pick_from_replaceOptions_combo_i = (pickedTemplateInfo_ptr->size_of_vecReplaceOptions_n > 0) ? mxconst::INT_FIRST_0 : mxconst::INT_UNDEFINED; // reset user pick

                  // this->mxUiGetContentWidth() returns the Image region not the description. the "30.f" is the space between the regions.
                  //const float region_width_x = (region_width_arr[1] <= 0.0f) ? (win_size_vec2.x - this->mxUiGetContentWidth() - 30.0f) : region_width_arr[1];

                  // Calculate combo item length
                  if (this->strct_generate_template_layer.user_pick_from_replaceOptions_combo_i > mxconst::INT_UNDEFINED)
                  {
                    // calculate text length in pixel + scale
                    const auto combo_spacer_f               = 35.0f * this->strct_setup_layer.fPreferredFontScale; // space that we want to add to string pixel length to have a better visually combo size
                    const auto combo_label_s_pixel_length_f = ImGui::CalcTextSize(combo_label_s).x * this->strct_setup_layer.fPreferredFontScale;
                    const auto help_marker_width_pixel_length_f = ImGui::CalcTextSize(" (?) ").x * this->strct_setup_layer.fPreferredFontScale;
                    const auto max_option_pixel_length_f        = ImGui::CalcTextSize(pickedTemplateInfo_ptr->vecReplaceOptions_s.at( longestTextInVector_array_i).c_str()).x * this->strct_setup_layer.fPreferredFontScale; // v3.0.303.2 fixed length of combo

                    if (max_option_pixel_length_f + combo_label_s_pixel_length_f + combo_spacer_f > region_width_arr[1])
                      this->strct_generate_template_layer.vec2_replace_options_size.x = (region_width_arr[1] - (combo_label_s_pixel_length_f + help_marker_width_pixel_length_f) ); // - (20.0f * this->strct_setup_layer.fPreferredFontScale)); // the 20.0f is for the (?) helper at the start of the line
                    else
                      this->strct_generate_template_layer.vec2_replace_options_size.x = max_option_pixel_length_f + combo_spacer_f; // 25.0f represents the drop down combo icon
                  }


                  // v24.12.2 Calculate the MULTI COMBO data into the "strct_generate_template_layer.mapReplaceOption_ui"
                  // Hopefully the code below will replace the code above.
                  for (auto& [seq_key_i, strctOptInfo] : pickedTemplateInfo_ptr->mapOptionsInfo)
                  {
                    // store combo label based on the option name, or default label (seq_key_i < 0)
                    strctOptInfo.combo_label_s = (seq_key_i < 0) ? combo_label_s : strctOptInfo.name;


                    pickedTemplateInfo_ptr->mapOptionsInfo[seq_key_i].refresh_vecReplaceOptions_char();

                      // calculate text length in pixel + scale
                    if (strctOptInfo.user_pick_from_replaceOptions_combo_i > mxconst::INT_UNDEFINED)
                    {
                      const auto combo_spacer_f               = 35.0f * this->strct_setup_layer.fPreferredFontScale; // space that we want to add to string pixel length to have a better visually combo size
                      //const auto combo_label_s_pixel_length_f = ImGui::CalcTextSize(combo_label_s).x * this->strct_setup_layer.fPreferredFontScale;
                      const auto combo_label_s_pixel_length_f     = ImGui::CalcTextSize(strctOptInfo.combo_label_s.c_str()).x * this->strct_setup_layer.fPreferredFontScale;
                      const auto help_marker_width_pixel_length_f = ImGui::CalcTextSize(" (?) ").x * this->strct_setup_layer.fPreferredFontScale;
                      const auto max_option_pixel_length_f    = ImGui::CalcTextSize(strctOptInfo.longestTextInVector_s.c_str()).x * this->strct_setup_layer.fPreferredFontScale; // calculate combo length based on text size.

                      if (max_option_pixel_length_f + combo_label_s_pixel_length_f + combo_spacer_f > region_width_arr[1])
                        strctOptInfo.combo_width_f = (region_width_arr[1] - (combo_label_s_pixel_length_f + help_marker_width_pixel_length_f)); // (20.0f * this->strct_setup_layer.fPreferredFontScale)); // the 20.0f is for the (?) helper at the start of the line
                      else
                        strctOptInfo.combo_width_f = max_option_pixel_length_f + combo_spacer_f; // combo_spacer_f represents the drop down combo icon
                    }
                  }
                  // v24.12.2

                  #ifndef RELEASE
                  Log::logMsg("Picked: " + this->strct_generate_template_layer.last_picked_template_key); // debug
                  #endif
                }
              }
              ImGui::PopID();
              ++imgNo;

              if (bSetKeyStyle)
                ImGui::PopStyleColor(iStyle);
            } // end loop over all images
          }
          else
          { // Display the "picked" template description and options, or the "generated template info"
            if (!this->strct_generate_template_layer.last_picked_template_key.empty())
            {
              // after generated mission from template, display the "info" text.
              if (this->flag_generatedRandomFile_success)
              {
                const std::string fileName = (this->strct_generate_template_layer.last_picked_template_key.find(mxconst::get_XML_EXTENSION()) == std::string::npos) ? this->strct_generate_template_layer.last_picked_template_key + mxconst::get_XML_EXTENSION() : this->strct_generate_template_layer.last_picked_template_key;

                ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);
                if (Utils::isElementExists(data_manager::mapGenerateMissionTemplateFiles, this->strct_generate_template_layer.last_picked_template_key))
                {

                  // Display the mission briefing description after generating the mission file
                  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
                  ImGui::TextWrapped("%s", missionx::data_manager::mapGenerateMissionTemplateFiles[this->strct_generate_template_layer.last_picked_template_key].description.c_str());
                  this->mxUiResetAllFontsToDefault();
                }
                ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
              }
              else
              { // Display the "template" options, and basic description.

                ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);
                // show options combo box
                // v24.12.2 multi options
                if (mxUtils::isElementExists(data_manager::mapGenerateMissionTemplateFiles, this->strct_generate_template_layer.last_picked_template_key))
                {
                  auto* pickedTemplateInfo_ptr = &missionx::data_manager::mapGenerateMissionTemplateFiles[this->strct_generate_template_layer.last_picked_template_key];
                  assert(pickedTemplateInfo_ptr != nullptr && "2. Failed to map mapGenerateMissionTemplateFiles pointer!!!");

                  if (!pickedTemplateInfo_ptr->mapOptionsInfo.empty())
                  {
                    for (auto& [seq_key, option_info] : pickedTemplateInfo_ptr->mapOptionsInfo)
                    {
                      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow); //
                      HelpMarker("Select a Template Option from the combo list.\nIt will dynamically affect the mission creation output.");
                      ImGui::PopStyleColor(1);
                      ImGui::SameLine(0.0f, 2.0f);

                      this->mxUiSetFont(mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1());

                      // Combo
                      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_orange);
                      ImGui::SetNextItemWidth(option_info.combo_width_f);
                      ImGui::Combo(fmt::format("##Pick template options{}", seq_key).c_str(), &option_info.user_pick_from_replaceOptions_combo_i, option_info.vecReplaceOptions_char.data(), static_cast<int> (option_info.vecReplaceOptions_char.size ()));
                      ImGui::PopStyleColor();
                      // label
                      ImGui::SameLine(0.0f, 2.0f);
                      ImGui::TextColored(missionx::color::color_vec4_lime, "%s", option_info.combo_label_s.c_str());

                      this->mxUiReleaseLastFont();
                    }

                    ImGui::Separator();
                    ImGui::Separator();
                  }
                }
                // v24.12.2 end multi options



                this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
                ImGui::TextWrapped("%s", data_manager::mapGenerateMissionTemplateFiles[this->strct_generate_template_layer.last_picked_template_key].desc_from_vector_with_tabs_s.c_str());
                this->mxUiResetAllFontsToDefault();
                ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
              }
            }
          }
        }

        ImGui::EndChild();
        ImGui::EndGroup();
      }
      ImGui::PopID();

      // -------------------
      // ------- Buttons
      // -------------------

      //ImGui::NewLine();
      ImGui::BeginGroup();

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14

      // v24.12.2 compatibility checkbox
      this->add_ui_xp11_comp_checkbox(false);
      ImGui::SameLine();

      ///// Display Generate or Start buttons
      if (!this->selectedTemplateKey.empty() && !missionx::data_manager::flag_generate_engine_is_running)
      {
        int styleCounter                       = 0;
        this->flag_generatedRandomFile_success = false; // this will also assist in hiding the "start" button since we are generating
        ImGui::SameLine(region_width_arr[0] + 10.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_red);
        styleCounter++;
        if (ImGui::Button("Generate Mission From Template"))
        {
          // v3.303.12 added weather
          switch (this->adv_settings_strct.iWeatherType_user_picked)
          {
            case missionx::mx_ui_random_weather_options::pick_pre_defined:
            {
              // store values in the prop_userDefinedMission_ui. During Random mission generation the weather will be picked from the list
              missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_WEATHER_USER_PICKED(), this->adv_settings_strct.get_weather_picked_by_user());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_WEATHER_CHANGE_MODE_USER_PICKED(), this->adv_settings_strct.get_weather_change_mode_picked_by_user()); // v3.303.13
            }
            break;
            case missionx::mx_ui_random_weather_options::use_xplane_weather_and_store:
            {
              missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_WEATHER_USER_PICKED(), mxconst::get_VALUE_STORE_CURRENT_WEATHER_DATAREFS());
              missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute(mxconst::get_PROP_WEATHER_CHANGE_MODE_USER_PICKED().c_str()); // v3.303.13
            }
            break;
            default:
            {
              missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute(mxconst::get_PROP_WEATHER_USER_PICKED().c_str());
              missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute(mxconst::get_PROP_WEATHER_CHANGE_MODE_USER_PICKED().c_str()); // v3.303.13
            }
            break;
          }

          this->execAction(missionx::mx_window_actions::ACTION_GENERATE_RANDOM_MISSION); // should hide the window
        }
        ImGui::SameLine(0.0f, 10.0f); // v24.03.2
        missionx::WinImguiBriefer::add_designer_mode_checkbox(); // v24.03.2

        ImGui::PopStyleColor(styleCounter);
      }
      else if (missionx::RandomEngine::threadState.flagIsActive)
      {
        ImGui::SameLine(region_width_arr[0] + 10.0f);
        this->add_ui_abort_mission_creation_button(); // Add Abort Random Engine
      }
      else if (data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running && this->flag_generatedRandomFile_success && this->selectedTemplateKey.empty() && !missionx::data_manager::flag_generate_engine_is_running /* make sure that thread is not running */) //
      {
        ImGui::SameLine(region_width_arr[0] + 10.0f); // pad to the right so the button will better aligned with above frame.
        this->add_ui_start_mission_button(missionx::mx_window_actions::ACTION_START_RANDOM_MISSION);
      }
      ImGui::EndGroup();
      // ImGui::NewLine(); // v3.305.1 deprecated
    }
    else
    {
      ImGui::NewLine();

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED()); // v3.303.14
      ImGui::TextColored(missionx::color::color_vec4_magenta, "Please wait while loading the templates.... ");
      this->mxUiReleaseLastFont(); // v3.303.14

    }
  }
  else if (this->strct_generate_template_layer.layer_state == missionx::mx_layer_state_enum::failed_data_is_not_present || this->strct_generate_template_layer.layer_state == missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly)
  {
    ImGui::NewLine();

    if (this->strct_generate_template_layer.layer_state == missionx::mx_layer_state_enum::failed_data_is_not_present)
    {
      this->display_shared_message_when_optimized_data_is_not_present(missionx::mx_layer_state_enum::failed_data_is_not_present);
    }
    else
    {
      this->display_shared_message_when_optimized_data_is_not_present(missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly);
    }
  }
  else // display wait
  {
    ImGui::NewLine();

    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_BIG());
    // ImGui::SetWindowFontScale(2.0f);
    ImGui::TextColored(missionx::color::color_vec4_magenta, "Please wait while testing validity of the data.... ");
    // ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
    this->mxUiReleaseLastFont(); // v3.303.14 release the last 2 fonts we pushed

    if (this->strct_generate_template_layer.layer_state < missionx::mx_layer_state_enum::validating_data)
    {
      this->strct_generate_template_layer.layer_state = missionx::mx_layer_state_enum::validating_data;
    }
  }
  this->mxUiResetAllFontsToDefault(); // v3.303.14

} // draw_mission_generator_layer

// ---------------------------------------------------

void
WinImguiBriefer::draw_flight_leg_info()
{
  constexpr const static ImVec2 IMVEC2_TOP_IMAGE_SIZE = { 87.0f, 126.0f };
  constexpr const static float IMG_GROUP_WIDTH = 102.0;

  auto win_size_vec2 = this->mxUiGetWindowContentWxH();


  constexpr const static float child_w[] = { IMG_GROUP_WIDTH, 0 }; // 102, 0; 0 = stretch to the end

  // ///////////////////////////////////////////
  // TOOLBAR: DRAW specific layer Toolbar Buttons
  // ///////////////////////////////////////////
  if (missionx::data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
  {
    constexpr const static float TOOLBAR_SPACE_PAD = 40.0f;
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS); // transparent
    ImGui::BeginChild("draw_top_toolbar_01");
    {
      float iButtonsAdded = 1.0;
      // Target Marker status icon
      const bool bDisplayMarkers = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS(), true);
      ImGui::SameLine(this->vec2_sizeTopBtn.x, 10.0f);
      if (!bDisplayMarkers)
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.25f);

      if (ImGui::ImageButton("TargetMarkerButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_TARGET_MARKER_ICON()].gTexture)), this->vec2_sizeTopBtn))
      {
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::toggle_target_marker_option);
      }
      this->mx_add_tooltip(missionx::color::color_vec4_white, "Toggle 3D target hint object"); // v24.06.1

      if (!bDisplayMarkers)
        ImGui::PopStyleVar();

      ImGui::SameLine(mxUiGetContentWidth() * 0.85f);

      // v3.305.1 hide button if we have an active message type story / active timer / gather acf stats
      if (missionx::Message::lineAction4ui.actionCode == '\0' && !missionx::data_manager::timelapse.flag_isActive && false == missionx::data_manager::flag_gather_acf_info_thread_is_running &&
          ImGui::ImageButton("SaveButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_SAVE_48x48()].gTexture)), this->vec2_sizeTopBtn))
      {
        this->execAction(missionx::mx_window_actions::ACTION_CREATE_SAVEPOINT);
      }

      this->mx_add_tooltip(missionx::color::color_vec4_white, "Create Savepoint");

      ImGui::SameLine(mxUiGetContentWidth() * 0.15f + TOOLBAR_SPACE_PAD * iButtonsAdded);
      if (ImGui::ImageButton("InvButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_INVENTORY_MXPAD()].gTexture)), this->vec2_sizeTopBtn))
      {
        this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_INVENTORY);
      }
      this->mx_add_tooltip(missionx::color::color_vec4_white, "Toggle Inventory");

      iButtonsAdded++;
      if (missionx::data_manager::strct_flight_leg_info_totalMapsCounter > 0)
      {
        ImGui::SameLine(mxUiGetContentWidth() * 0.15f + TOOLBAR_SPACE_PAD * iButtonsAdded);
        if (ImGui::ImageButton("MXpadButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_MAP_MXPAD()].gTexture)), this->vec2_sizeTopBtn))
        {
          this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_MAP);
        }
      }
      this->mx_add_tooltip(missionx::color::color_vec4_white, "Toggle Information/Map");

      iButtonsAdded++;

      ImGui::SameLine(mxUiGetContentWidth() * 0.15f + TOOLBAR_SPACE_PAD * iButtonsAdded);
      if (ImGui::ImageButton("NavInfo", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_NAVINFO()].gTexture)), this->vec2_sizeTopBtn))
      {
        this->execAction(missionx::mx_window_actions::ACTION_OPEN_NAV_LAYER);
      }
      this->mx_add_tooltip(missionx::color::color_vec4_white, "Open ILS/Nav Screen");
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(1);
  }

  // ----- END Toolbar Buttons draw

  float y_shorten = 0.0f;
  if (this->strct_flight_leg_info.internal_child_layer == missionx::uiLayer_enum::flight_leg_info_map2d)
    y_shorten = 50.0f; //
  else if ((this->strct_flight_leg_info.internal_child_layer == missionx::uiLayer_enum::flight_leg_info_end_summary ) + (this->strct_flight_leg_info.flagDebugTabIsOpen) + (this->strct_flight_leg_info.flagFlightPlanningTabIsOpen))
    y_shorten = 100.0f; //
  else if (this->strct_flight_leg_info.internal_child_layer == missionx::uiLayer_enum::flight_leg_info_inventory)
    y_shorten = 120.0f; //


  // v24.03.1 display upper part only if not in story mode or mission is running
  if ((Message::lineAction4ui.state == missionx::enum_mx_line_state::undefined) && (missionx::data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running))
  {

    ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale); // v3.305.1


    // Left image
    int i = 0;
    ImGui::BeginGroup();
    ImGui::BeginChild("FlightLegImage", ImVec2(child_w[i], this->imvec2_flight_info_top_area_size.y - y_shorten), ImGuiChildFlags_Borders, ((i == 0) ? ImGuiWindowFlags_NoScrollbar : ImGuiWindowFlags_None)); // height of image area
    {
      ImGui::Image(reinterpret_cast<void *> (static_cast<intptr_t> (missionx::data_manager::xp_mapMissionIconImages[missionx::data_manager::selectedMissionKey].gTexture)), IMVEC2_TOP_IMAGE_SIZE);
    }
    ImGui::EndChild();
    ImGui::EndGroup();

    // Right top description/Title
    ImGui::SameLine();
    i = 1;
    ImGui::BeginGroup();
    ImGui::BeginChild("FlightLegRightTop", ImVec2(child_w[i], this->imvec2_flight_info_top_area_size.y - y_shorten), ImGuiChildFlags_Borders, ((i == 0) ? ImGuiWindowFlags_NoScrollbar : ImGuiWindowFlags_None)); // height of image area
    {
      if (this->strct_flight_leg_info.internal_child_layer == missionx::uiLayer_enum::flight_leg_info_end_summary)
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
        ImGui::TextWrapped("%s", this->strct_flight_leg_info.end_description.c_str());
        this->mxUiReleaseLastFont();
      }
      else
      {
        const std::string sLegTitle = "Title: " + data_manager::getTitleOrNameFromNode(data_manager::mapFlightLegs[data_manager::currentLegName]);
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED()); // v3.303.14 // v24026
        {
          ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
          ImGui::TextWrapped("%s", sLegTitle.c_str());
          ImGui::PopStyleColor();
        }
        //ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", sLegTitle.c_str());
        this->mxUiReleaseLastFont();


        if (this->IsInVR()) // When in VR mode, display flight leg detail in upper region
        {
          const std::string sDesc = data_manager::mapFlightLegs[data_manager::currentLegName].getNodeStringProperty(mxconst::get_ELEMENT_DESC(), "", true); // read description first from mapText and then from xml node.
          this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.303.14
          ImGui::TextWrapped("%s", sDesc.c_str());
          this->mxUiReleaseLastFont();
        }
      }
    }
    ImGui::EndChild();
    ImGui::EndGroup();

    ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE); // v3.305.1
  }


  // --------------------------
  // Main Body part of Flight Leg screen.
  // --------------------------

  // v24.3.1 Force "uiLayer_enum::flight_leg_info" if mission is not in active state.
  if (missionx::data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running)
    this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info;


  switch (this->strct_flight_leg_info.internal_child_layer)
  {
    case missionx::uiLayer_enum::flight_leg_info:
    {
      child_draw_2D_and_VR_flight_leg_info_mxpad_and_choices_with_tab();

      // force story mode in 2D or VR mode
      if (Message::lineAction4ui.state >= missionx::enum_mx_line_state::ready) // v3.305.1 workaround if story message did not cause the internal layer switch
        this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info_story_mode;

    }
    break;
    case missionx::uiLayer_enum::flight_leg_info_story_mode:
    {
      this->child_draw_STORY_mode_leg_info();
      if (Message::lineAction4ui.state == missionx::enum_mx_line_state::undefined) // reset layout if message ended
        this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info;
    }
    break;
    case missionx::uiLayer_enum::flight_leg_info_map2d:
    {
      child_flight_leg_info_draw_map();
    }
    break;
    case missionx::uiLayer_enum::flight_leg_info_inventory:
    {
      child_flight_leg_info_draw_inventory();
    }
    break;
    case missionx::uiLayer_enum::flight_leg_info_end_summary:
    {
      child_flight_leg_info_draw_end_summary();
    }
    break;
    default:
      break;
  } // end switch between child layers
} // end draw_flight_leg_info

// ------------------------------------------------

void
WinImguiBriefer::child_draw_2D_and_VR_flight_leg_info_mxpad_and_choices_with_tab()
{
  const bool bDisableTabs = (missionx::data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running);

  float              child_w[]     = { 0.0f, 0.0f };
  constexpr const static float fTitleHeight = 30.0f;

  ImVec2 vec2Window = this->mxUiGetWindowContentWxH();

  if (missionx::data_manager::mxChoice.is_choice_set())
  {
    child_w[0] = 350.0f;
    child_w[1] = 426.0f;
  }
  else
  {
    child_w[0] = 0.0f; // full window width
    child_w[1] = 0.0f; // full window width
  }

  ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);

  ImGuiTabBarFlags mainTab_bar_flags = ImGuiTabBarFlags_Reorderable; // ImGuiTabBarFlags_None;
  if (ImGui::BeginTabBar("FlightLegInfo", mainTab_bar_flags))
  {
    ImGuiTabItemFlags itemTab_bar_flags = ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_NoReorder; // ImGuiTabItemFlags_None;

    if (!bDisableTabs)
    {
      // Display description tab for 2D mode
      if (!this->IsInVR() && ImGui::BeginTabItem("Description", nullptr, itemTab_bar_flags))
      {
        ImGui::BeginGroup();
        ImGui::BeginChild("child_draw_2D_flight_leg_info", ImVec2(0.0f, ImGui::GetWindowHeight() - imvec2_flight_info_top_area_size.y - this->fTopToolbarPadding_f - this->fBottomToolbarPadding_f - fTitleHeight), ImGuiChildFlags_Borders);
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.303.14
        ImGui::TextWrapped("%s", data_manager::mapFlightLegs[data_manager::currentLegName].getNodeStringProperty(mxconst::get_ELEMENT_DESC(), "", true).c_str());
        this->mxUiReleaseLastFont();
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::EndTabItem();
        //    }
      }
      else if (this->IsInVR() && ImGui::BeginTabItem("Flight Leg", nullptr, itemTab_bar_flags)) // display the VR description
      {
        ImGui::PushID("##SplitFlightLegInfo01");

        for (int i = 0; i < 2; ++i)
        {
          if (i > 0)
            ImGui::SameLine();

          ImGui::BeginGroup();
          constexpr static const char* names[] = { "Messages", "Choice Window" };

          if (i == 0 || (i == 1 && missionx::data_manager::mxChoice.is_choice_set()))
          {
            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", names[i]);
            this->mxUiReleaseLastFont();
          }

          // Draw 2 regions
          const ImGuiID child_id = ImGui::GetID(reinterpret_cast<void *> (static_cast<intptr_t> (i)));
          ImGui::BeginChild(child_id, ImVec2(child_w[i], ImGui::GetWindowHeight() - imvec2_flight_info_top_area_size.y - this->fTopToolbarPadding_f - this->fBottomToolbarPadding_f - fTitleHeight - this->PAD_BETWEEN_CHILD_REGIONS), ImGuiChildFlags_Borders);
          if (i == 0)
          {
            std::string store_last_message_s;
            for (const auto& msg : missionx::QueueMessageManager::mxpad_messages) // calculate the overall height of all messages lines
            {
              ImGui::BeginGroup();
              ImGui::PushStyleColor(ImGuiCol_Text, missionx::WinImguiBriefer::getColorAsImVec4(msg.label_color));
              this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
              ImGui::SetNextItemWidth(60.0f);
              ImGui::Text("%s", msg.label.c_str());
              this->mxUiReleaseLastFont();
              ImGui::PopStyleColor(1);
              ImGui::EndGroup();

              ImGui::BeginGroup();
              ImGui::SameLine(70.0f);
              this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
              ImGui::TextWrapped("%s", msg.message_text.c_str());
              this->mxUiReleaseLastFont();
              store_last_message_s = msg.message_text; // v3.303.13
              ImGui::EndGroup();
            }
            // Should we move the scroll ?
            if (store_last_message_s != this->strct_flight_leg_info.last_msg_s) // if last message in queue is not the same as the stored last message
            {
              //__debugbreak;
              this->strct_flight_leg_info.last_msg_s = store_last_message_s;
              ImGui::SetScrollHereY(1.0f);
            }
          }
          else // display options area
          {
            if (missionx::data_manager::mxChoice.is_choice_set())
            {
              // loop over options
              ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 160, 0, 255)); // Green

              this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
              for (int i1 = 0; i1 < missionx::data_manager::mxChoice.nVecSize_i; ++i1) // mxChoice.mapOptions size must be equal to :mxChoice.vecXmlOptions
              {
                int radioChoice = -1;
                if (Utils::isElementExists(missionx::data_manager::mxChoice.mapOptions, i1))
                {
                  if (!missionx::data_manager::mxChoice.mapOptions[i1].flag_hidden)
                  {
                    if (ImGui::RadioButton(missionx::data_manager::mxChoice.mapOptions[i1].text.c_str(), radioChoice == i1))
                    {
                      radioChoice = i1;
                      if (missionx::data_manager::mxChoice.optionPicked_key_i != missionx::data_manager::mxChoice.mapOptions[i1].key_i) // does the last action equals current action? we test this so we won't have repeating same action calls
                      {
                        missionx::data_manager::mxChoice.optionPicked_key_i = missionx::data_manager::mxChoice.mapOptions[i1].key_i;
                        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::handle_option_picked_from_choice);

                        Log::logDebugBO("[imguiBrifer flight leg info] Picked: " + Utils::formatNumber<int>(missionx::data_manager::mxChoice.mapOptions[i1].key_i) + ", " + missionx::data_manager::mxChoice.mapOptions[i1].text); // debug
                      }
                    } // end radio draw
                  }   // end if not hidden
                }     // end if found option keyName in choice (can be sequence number also).
              }       // end loop over all options
              this->mxUiReleaseLastFont();
              ImGui::PopStyleColor(1);
            }
          }

          ImGui::EndChild();
          ImGui::EndGroup();
        } // end loop

        ImGui::PopID();

        ImGui::EndTabItem();
      }

      // add the other shared tabs

      if (ImGui::BeginTabItem("stats"))
      {
        // int i = 1; // pick the child_w[1] width value
        ImGui::BeginGroup();
        this->add_ui_stats_child();
        ImGui::EndGroup();

        ImGui::EndTabItem();
      }
      if (!missionx::data_manager::listOfMessageStoryMessages.empty() && ImGui::BeginTabItem("story message history")) // v3.305.2
      {

        // Draw history text
        ImGui::BeginGroup();
        ImGui::BeginChild("child_draw_history_story_messages", ImVec2(0.0f, ImGui::GetWindowHeight() - imvec2_flight_info_top_area_size.y - this->fTopToolbarPadding_f - this->fBottomToolbarPadding_f - fTitleHeight), ImGuiChildFlags_Borders);
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.303.14
        this->add_story_message_history_text();
        this->mxUiReleaseLastFont();
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("info"))
      {
        ImGui::BeginGroup();
        ImGui::BeginChild("child_draw_info_text", ImVec2(0.0f, ImGui::GetWindowHeight() - imvec2_flight_info_top_area_size.y - this->fTopToolbarPadding_f - this->fBottomToolbarPadding_f - fTitleHeight), ImGuiChildFlags_Borders);
        this->add_info_to_flight_leg();
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::EndTabItem();
      }

      // v3.305.2 In DEBUG build default is "true", in RELEASE is it false.
      this->strct_flight_leg_info.flagDebugTabIsOpen = false; // v3.305.3
      if (missionx::data_manager::flag_setupShowDebugTabDuringMission && ImGui::BeginTabItem("debug"))
      {
        this->strct_flight_leg_info.flagDebugTabIsOpen ^= 1; // v3.305.3 true
        ImGui::BeginGroup();
        ImGui::BeginChild("child_draw_debug_info##FlightLegDebugInfo", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - this->PAD_BETWEEN_CHILD_REGIONS), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
        this->add_debug_info();
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::EndTabItem();
      }
    }

    this->strct_flight_leg_info.flagFlightPlanningTabIsOpen = false;
    if (ImGui::BeginTabItem("Flight Planning/Notes", NULL, ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoReorder))
    {
      this->strct_flight_leg_info.flagFlightPlanningTabIsOpen ^= 1;
      ImGui::BeginGroup();
      const auto final_fpln_tab_padding = (missionx::data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running)? this->PAD_BETWEEN_CHILD_REGIONS + 20.0f : this->PAD_BETWEEN_CHILD_REGIONS;
      ImGui::BeginChild("FlightNotes", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - final_fpln_tab_padding), ImGuiChildFlags_Borders);
      this->add_flight_planning();
      ImGui::EndChild();
      ImGui::EndGroup();

      ImGui::EndTabItem();
    }


    ImGui::EndTabBar();
  }

  ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);

}



// ------------------------------------------------

void
WinImguiBriefer::child_draw_STORY_mode_leg_info()
{
  auto vec2Window = this->mxUiGetWindowContentWxH();

  ImGui::BeginGroup();
  auto debugWindowHeight = ImGui::GetWindowHeight();
  ImGui::BeginChild("MessageStoryImages", this->strct_flight_leg_info.strct_story_mode.upperStoryMode_vec2, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

  if (!this->strct_flight_leg_info.strct_story_mode.bPressedHistory) // v3.305.2
  {

    assert(missionx::Message::vecStoryCurrentImages_p.size() >= Message::VEC_STORY_IMAGE_SIZE_I && "Message story size is less than expected");
    ///////////////////////////////
    // Draw Images
    //////////////////////////////
    if (Message::vecStoryCurrentImages_p.at(Message::IMG_BACKROUND) != nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_BACKROUND)->sImageData.pData == nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_BACKROUND)->gTexture != 0)
    {
      ImGui::SameLine(vec2Window.x * 0.5f - (this->strct_flight_leg_info.strct_story_mode.background_img_vec2.x * 0.5f)); // center of screen
      ImGui::Image((void*)static_cast<intptr_t> (Message::vecStoryCurrentImages_p.at (Message::IMG_BACKROUND)->gTexture), this->strct_flight_leg_info.strct_story_mode.background_img_vec2);
    }
    // draw left image
    if (Message::vecStoryCurrentImages_p.at(Message::IMG_LEFT) != nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_LEFT)->sImageData.pData == nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_LEFT)->gTexture != 0)
    {
      ImGui::SameLine(5.0f); // left image will start from 5.0f
      ImGui::Image((void*)static_cast<intptr_t> (Message::vecStoryCurrentImages_p.at (Message::IMG_LEFT)->gTexture), this->strct_flight_leg_info.strct_story_mode.small_img_vec2);
    }
    // draw right image
    if (Message::vecStoryCurrentImages_p.at(Message::IMG_RIGHT) != nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_RIGHT)->sImageData.pData == nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_RIGHT)->gTexture != 0)
    {
      ImGui::SameLine(vec2Window.x - this->strct_flight_leg_info.strct_story_mode.small_img_vec2.x - 5.0f); // Left of screen
      ImGui::Image((void*)static_cast<intptr_t> (Message::vecStoryCurrentImages_p.at (Message::IMG_RIGHT)->gTexture), this->strct_flight_leg_info.strct_story_mode.small_img_vec2);
    }
    // draw center image
    if (Message::vecStoryCurrentImages_p.at(Message::IMG_CENTER) != nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_CENTER)->sImageData.pData == nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_CENTER)->gTexture != 0)
    {
      ImGui::SameLine(vec2Window.x * 0.5f - (this->strct_flight_leg_info.strct_story_mode.small_img_vec2.x * 0.5f)); // center of screen
      ImGui::Image((void*)static_cast<intptr_t> (Message::vecStoryCurrentImages_p.at (Message::IMG_CENTER)->gTexture), this->strct_flight_leg_info.strct_story_mode.small_img_vec2);
    }
    // draw left med image
    if (Message::vecStoryCurrentImages_p.at(Message::IMG_LEFT_MED) != nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_LEFT_MED)->sImageData.pData == nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_LEFT_MED)->gTexture != 0)
    {
      ImGui::SameLine(5.0f); // left image will start from 0 + 5px
      ImGui::Image((void*)static_cast<intptr_t> (Message::vecStoryCurrentImages_p.at (Message::IMG_LEFT_MED)->gTexture), this->strct_flight_leg_info.strct_story_mode.med_img_vec2);
    }
    // draw right med image
    if (Message::vecStoryCurrentImages_p.at(Message::IMG_RIGHT_MED) != nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_RIGHT_MED)->sImageData.pData == nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_RIGHT_MED)->gTexture != 0)
    {
      ImGui::SameLine(vec2Window.x - this->strct_flight_leg_info.strct_story_mode.med_img_vec2.x - 5.0f); // Right of screen
      ImGui::Image((void*)static_cast<intptr_t> (Message::vecStoryCurrentImages_p.at (Message::IMG_RIGHT_MED)->gTexture), this->strct_flight_leg_info.strct_story_mode.med_img_vec2);
    }
    // draw center med image
    if (Message::vecStoryCurrentImages_p.at(Message::IMG_CENTER_MED) != nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_CENTER_MED)->sImageData.pData == nullptr && Message::vecStoryCurrentImages_p.at(Message::IMG_CENTER_MED)->gTexture != 0)
    {
      ImGui::SameLine(vec2Window.x * 0.5f - (this->strct_flight_leg_info.strct_story_mode.med_img_vec2.x * 0.5f)); // center of screen
      ImGui::Image((void*)static_cast<intptr_t> (Message::vecStoryCurrentImages_p.at (Message::IMG_CENTER_MED)->gTexture), this->strct_flight_leg_info.strct_story_mode.med_img_vec2);
    }
  }
  else
  {
    ImGui::BeginGroup();
      this->add_story_message_history_text();
    ImGui::EndGroup();
  } // end if to display history
  ImGui::EndChild();
  ImGui::EndGroup();

  ImGui::BeginGroup();

  ///////////////////////////////
  // Title & Skip/Next Buttons
  //////////////////////////////


  // print title
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
  {
    ImGui::TextColored(this->mxConvertMxVec4ToImVec4(this->strct_flight_leg_info.strct_story_mode.characterInfo.getColorAsVec4()), "%s", this->strct_flight_leg_info.strct_story_mode.characterInfo.label.c_str());
  }
  this->mxUiReleaseLastFont();

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());

  // Add [Next] / [Skip] Buttons
  if (Message::lineAction4ui.state == missionx::enum_mx_line_state::broadcasting)
  {


    if (missionx::data_manager::flag_setupAutoSkipStoryMessage)
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_orange);


    ImGui::SameLine(vec2Window.x - 100.0f);
    if (!this->strct_flight_leg_info.strct_story_mode.bPressedHistory && ImGui::Button(std::string("Skip " + Utils::from_u8string(ICON_FA_FAST_FORWARD)).c_str() ) || missionx::data_manager::flag_setupAutoSkipStoryMessage)
    {
      this->strct_flight_leg_info.strct_story_mode.iChar = Message::lineAction4ui.vals[mxconst::get_STORY_TEXT()].length() ; // this will cause the whole sentence to be displayed
    }

    if (missionx::data_manager::flag_setupAutoSkipStoryMessage)
      ImGui::PopStyleColor();
  }
  else if (Message::lineAction4ui.state == missionx::enum_mx_line_state::broadcasting_paused)
  {
    if (!this->strct_flight_leg_info.strct_story_mode.bPressedHistory)
    {
      ImGui::SameLine(vec2Window.x - 100.0f);
      this->add_story_next_button();
    }

  } // end broadcasting_paused

  // v3.305.2
  const std::string sHISTORY_BUTTON = "Toggle History " + mxUtils::from_u8string(ICON_FA_PAUSE);
  ImGui::SameLine(vec2Window.x * 0.5f - (ImGui::CalcTextSize(sHISTORY_BUTTON.c_str()).x * 0.5f)); // v3.305.3
  if (ImgWindow::ButtonTooltip(sHISTORY_BUTTON.c_str(), "Message History and Pause Mode"))
  {
    this->strct_flight_leg_info.strct_story_mode.bPressedHistory ^= 1;
    this->strct_flight_leg_info.strct_story_mode.bScrollToEndOfHistoryMessages = true;
  }

  if (missionx::data_manager::flag_setupDisplayAutoSkipInStoryMessage)
  {
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_CheckMark, missionx::color::color_vec4_orange);
      ImGui::Checkbox("Auto Skip", &missionx::data_manager::flag_setupAutoSkipStoryMessage);
      ImGui::PopStyleColor();
  }


  this->mxUiReleaseLastFont();



  ImGui::BeginChild("MessageStoryText", ImVec2(0.0f, this->strct_flight_leg_info.strct_story_mode.STORY_TEXT_AREA_HEIGHT_F), ImGuiChildFlags_Borders,
                    ImGuiWindowFlags_None); // extra 10.0 padding to see bottom message // extra -30 for commit/cancel buttons
  {
    ///////////////////////////////////////////////
    // Handle Message Body and Pause actions Logic
    //////////////////////////////////////////////

    if (!this->strct_flight_leg_info.strct_story_mode.bPressedHistory)
    {
      switch (Message::lineAction4ui.actionCode)
      {
        case mxconst::STORY_ACTION_TEXT:
        {
          switch (Message::lineAction4ui.state)
          {
            case missionx::enum_mx_line_state::ready:
            {
              this->strct_flight_leg_info.strct_story_mode.reset(); // reset iChar, bPressPause timerForTextProgression, timerAutoSkip and textLength.

              Message::lineAction4ui.state = missionx::enum_mx_line_state::broadcasting;
              missionx::Timer::start(this->strct_flight_leg_info.strct_story_mode.timerForTextProgression, mxconst::STORY_DEFAULT_TIME_BETWEEN_CHARS_SEC_F);
              this->strct_flight_leg_info.strct_story_mode.characterInfo = Message::lineAction4ui.characterInfo;
              this->strct_flight_leg_info.strct_story_mode.sTextToPrint  = Message::lineAction4ui.vals[mxconst::get_STORY_TEXT()];
              this->strct_flight_leg_info.strct_story_mode.textLength    = this->strct_flight_leg_info.strct_story_mode.sTextToPrint.length();
              this->strct_flight_leg_info.strct_story_mode.prev_iChar    = static_cast<size_t> (0);
            }
            break;
            case missionx::enum_mx_line_state::broadcasting:
            {


              // Print Text
              if (this->strct_flight_leg_info.strct_story_mode.iChar < this->strct_flight_leg_info.strct_story_mode.textLength)
              {
                if (missionx::Timer::wasEnded(this->strct_flight_leg_info.strct_story_mode.timerForTextProgression, true)) // We should use OS time and not X-Plane clock since we might be in "pause" state.
                {
                  //// v3.305.1 Decide how much time to set the timer based on punctuations
                  const auto lmbda_get_seconds_to_wait = [&]()
                  {
                    float fExponentTime = 1.0f; // v3.305.3
                    if (missionx::Message::lineAction4ui.bIgnorePunctuationTiming) // We won't ignore punctuation totally, we will halve the punctuation timing (0.5f)
                      fExponentTime = mxconst::STORY_DEFAULT_PUNCTUATION_EXPONENT_F; //  return mxconst::STORY_DEFAULT_TIME_BETWEEN_CHARS_SEC_F;

                    {
                      switch (this->strct_flight_leg_info.strct_story_mode.sTextToPrint.at(this->strct_flight_leg_info.strct_story_mode.iChar))
                      {
                        case '.':
                        case '?':
                        case '!':
                        {
                          return mxconst::STORY_DEFAULT_TIME_AFTER_PERIOD_SEC_F * fExponentTime;
                        }
                        break;
                        case ',':
                        case ';':
                        {
                          return mxconst::STORY_DEFAULT_TIME_AFTER_COMMA_SEC_F;
                        }
                        break;
                      }
                    }
                    return mxconst::STORY_DEFAULT_TIME_BETWEEN_CHARS_SEC_F;
                  };


                  const float fTimeToWaitBetweenChars = lmbda_get_seconds_to_wait();
                  missionx::Timer::start(this->strct_flight_leg_info.strct_story_mode.timerForTextProgression, fTimeToWaitBetweenChars);
                  ++this->strct_flight_leg_info.strct_story_mode.iChar;
                }
              }
              else
              {
#ifndef RELEASE
                Log::logMsg("Finished Message for: " + Message::lineAction4ui.characterInfo.label);
#endif // !RELEASE

                this->strct_flight_leg_info.strct_story_mode.timerForTextProgression.reset();
                Message::lineAction4ui.state = missionx::enum_mx_line_state::broadcasting_paused;

                // v3.305.2 add text to history
                missionx::data_manager::addMessageToHistoryList(Message::lineAction4ui.characterInfo.label, this->strct_flight_leg_info.strct_story_mode.sTextToPrint, Message::lineAction4ui.characterInfo.getColorAsVec4());

                this->strct_flight_leg_info.strct_story_mode.setAutoSkipTimer(mxconst::DEFAULT_SKIP_MESSAGE_TIMER_IN_SEC_F);
              }
            }
            break;
            default:
              break;
          } // switch
        }
        break;

        case mxconst::STORY_ACTION_PAUSE:
        {
          if (Message::lineAction4ui.state == missionx::enum_mx_line_state::ready)
          {
            missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::set_story_auto_pause_timer); // we moved the logic to the mission::flcPre()
            Message::lineAction4ui.state = missionx::enum_mx_line_state::broadcasting_paused;
          }
        }
        break;
        default:
          break;
      }
    }


    ////////////////
    // Print TEXT
    ///////////////
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_MED());

    ImGui::TextWrapped("%s", this->strct_flight_leg_info.strct_story_mode.sTextToPrint.substr(0, this->strct_flight_leg_info.strct_story_mode.iChar).c_str());
    if (this->strct_flight_leg_info.strct_story_mode.prev_iChar != this->strct_flight_leg_info.strct_story_mode.iChar)
    {
      this->strct_flight_leg_info.strct_story_mode.prev_iChar = this->strct_flight_leg_info.strct_story_mode.iChar;
      ImGui::SetScrollHereY(1.0f);
    }

    this->mxUiReleaseLastFont();
  }
  ImGui::EndChild();
  ImGui::EndGroup();
}


// ------------------------------------------------

void
WinImguiBriefer::child_flight_leg_info_draw_inventory()
{
  std::string         str;
  ImVec2              vec2_btnSize = { 250.0f, this->strct_setup_layer.fPreferredFontScale * data_manager::FONT_SIZE + 6.0f }; // need to be dynamic
  constexpr static const ImVec2 vec2_portrait(50.0f, 40.0f);  // v3.0.303.5

  const auto window_content_xy = this->mxUiGetWindowContentWxH ();

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
  {

    ImGui::PushID("##invFlightLegInfo");
    static constexpr float                                      f_title_padding                = 24.0f;
    static constexpr float                                      f_bottom_cancel_commit_padding = 30.0f;
    static constexpr std::array<missionx::mx_ui_inv_regions, 2> region_arr                     = { _ui_inv_regions::plane, _ui_inv_regions::external_store };

    // prepare inventory keyName
    const std::string external_name = (this->strct_flight_leg_info.externalInventoryName.empty()) ? "No External Inventory" : this->strct_flight_leg_info.externalInventoryName;

    for (auto iRegion : region_arr) // 2 regions
    {
      if (iRegion > _ui_inv_regions::plane)
        ImGui::SameLine();
      ImGui::BeginGroup();
      const static char* names[] = { "Plane Inventory ", "External Inventory: " };

      // draw inventory titles
      if (iRegion == _ui_inv_regions::plane)
      {
        // Print plane weight
        ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", names[static_cast<int>(mx_ui_inv_regions::plane)]); // yellow
        ImGui::SameLine();
        if (this->strct_flight_leg_info.formated_plane_inv_title_s.find("!!") != std::string::npos)
          ImGui::TextColored(missionx::color::color_vec4_red, "%s", this->strct_flight_leg_info.formated_plane_inv_title_s.c_str()); // red
        else
          ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", this->strct_flight_leg_info.formated_plane_inv_title_s.c_str()); // green yellow

        ImGui::SameLine (0.0f, 20.0f);
        ImGui::TextColored (missionx::color::color_vec4_aqua, "%s", fmt::format("CG: {:.2f}cm | {:.2f}%", this->strct_flight_leg_info.cg_indicator_f*100, this->strct_flight_leg_info.cg_z_prct_f).c_str ());
      }
      else if (iRegion == _ui_inv_regions::external_store)
      {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", names[static_cast<int>(iRegion)]);
        ImGui::SameLine();
        ImGui::TextColored(missionx::color::color_vec4_greenyellow, "%s", external_name.c_str());
      }

      const ImGuiID child_id       = ImGui::GetID(reinterpret_cast<void*>(static_cast<intptr_t>(iRegion)));
      const ImVec2  vec2_inv_child = ImVec2(mxUiGetContentWidth() * 0.5f - 10.0f, ImGui::GetWindowHeight() - this->fTopToolbarPadding_f - this->fBottomToolbarPadding_f - f_title_padding - f_bottom_cancel_commit_padding - 25.0f); // v24.12.2

      ImGui::BeginChild(child_id, vec2_inv_child, ImGuiChildFlags_Borders); // extra 10.0 padding to see bottom message // extra -30 for commit/cancel buttons
      {
        // draw
        if (iRegion == _ui_inv_regions::plane) // draw plane inventory
        {
          if ((missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY) + (missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO))
            this->child_draw_inv_plane_xp11(iRegion, vec2_inv_child);
          else
          {
            if (this->strct_flight_leg_info.flagItemMoveWasPressedFromExternalInv)
              this->child_draw_inv_plane_xp12_move_item(data_manager::planeInventoryCopy, iRegion, vec2_inv_child);

            this->child_draw_inv_plane_xp12(data_manager::planeInventoryCopy, iRegion, vec2_inv_child);
          }
        }
        //////////////////////////
        /// EXTERNAL INVENTORY
        //////////////////////////
        else // external inventory
        {
          if (!this->strct_flight_leg_info.externalInventoryName.empty())
            this->child_draw_inv_external_store(vec2_inv_child);
        } // end external inventory information
      }
      ImGui::EndChild();
      ImGui::EndGroup();
    }
    ImGui::PopID();

    ////////// Bottom Action Buttons
    ////////////////////////////////
    ImGui::NewLine();
    ImGui::BeginGroup();
    {
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_crimson);
      {
        if (ImGui::Button("Cancel / Close"))
          this->execAction(mx_window_actions::ACTION_TOGGLE_INVENTORY);
      }
      ImGui::PopStyleColor(1);
      this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Cancel Transaction. Return Items to their original location.");

      ImGui::SameLine(window_content_xy.x * 0.5f);

      // color_vec4_darkgreen for the two commit buttons
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_darkgreen);
      {
        if (ImGui::Button("Commit"))
          this->execAction(mx_window_actions::ACTION_COMMIT_TRANSFER);
        this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Accept transaction, do not close inventory window.");

        ImGui::SameLine(window_content_xy.x * 0.85f);
        if (ImGui::Button("Commit / Close"))
        {
          this->execAction(mx_window_actions::ACTION_COMMIT_TRANSFER);
          this->execAction(mx_window_actions::ACTION_TOGGLE_INVENTORY);
        }
        this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Accept transaction and close inventory window.");
      }
      ImGui::PopStyleColor(1);

    }
    ImGui::EndGroup();
  }
  this->mxUiReleaseLastFont();
}


// ------------------------------------------------


void
WinImguiBriefer::child_draw_inv_plane_xp12_move_item(Inventory& inout_copied_plane_inventory, const missionx::mx_ui_inv_regions& inRegionType, const ImVec2& in_vec2_inv_child)
{
  if ( (!this->strct_flight_leg_info.flagItemMoveWasPressedFromExternalInv) + (this->strct_flight_leg_info.ptrNodePicked.isEmpty()))
    return;

  const auto container_dim = this->mxUiGetWindowContentWxH();

  const std::string barcode         = Utils::readAttrib(this->strct_flight_leg_info.ptrNodePicked, mxconst::get_ATTRIB_BARCODE(), "");
  const std::string itemName        = Utils::readAttrib(this->strct_flight_leg_info.ptrNodePicked, mxconst::get_ATTRIB_NAME(), barcode);
  const std::string image_file_name = Utils::readAttrib(this->strct_flight_leg_info.ptrNodePicked, mxconst::get_PROP_IMAGE_FILE_NAME(), "");
  const int         max_quantity_i  = Utils::readNodeNumericAttrib<int>(this->strct_flight_leg_info.ptrNodePicked, mxconst::get_ATTRIB_QUANTITY(), 0);

  ImGui::BeginChild("MoveItemToPlane", ImVec2(-10.0f, -50.0f), ImGuiChildFlags_Borders );
  {
    // Title
    ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", "How many:");
    ImGui::SameLine();
    ImGui::TextColored(missionx::color::color_vec4_aqua, "%s", itemName.c_str());
    ImGui::SameLine();
    ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", " to move");

    // Quantity Slider
    if (ImGui::Button("-"))
      this->strct_flight_leg_info.iSliderItemQuantity = (this->strct_flight_leg_info.iSliderItemQuantity - 1 >= 0) ? --this->strct_flight_leg_info.iSliderItemQuantity : this->strct_flight_leg_info.iSliderItemQuantity;

    ImGui::SameLine();
    ImGui::SliderInt("##SlideItemQuantity", &this->strct_flight_leg_info.iSliderItemQuantity, 0, max_quantity_i);
    ImGui::SameLine();

    if (ImGui::Button("+"))
      this->strct_flight_leg_info.iSliderItemQuantity = (this->strct_flight_leg_info.iSliderItemQuantity + 1 <= max_quantity_i) ? ++this->strct_flight_leg_info.iSliderItemQuantity : this->strct_flight_leg_info.iSliderItemQuantity;

    ImGui::SameLine(0.0f, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_orangered);
    {
      if (ImGui::Button("Close"))
        this->strct_flight_leg_info.resetItemMove();
    }
    ImGui::PopStyleColor();
    ImGui::Separator();


    ImGui::TextColored(missionx::color::color_vec4_aqua, "Pick where to move the item/s:");
    ImGui::Spacing();
    // Display buttons only if "move action" is still valid.
    if (this->strct_flight_leg_info.flagItemMoveWasPressedFromExternalInv)
      for (int btn_indx = 1; const auto& [station_id, station_name] : inout_copied_plane_inventory.map_acf_station_names)
      {
        if (btn_indx % 4 == 0)
        {
          ImGui::NewLine();
          btn_indx = 0;
        }
        else
          ImGui::SameLine(0.0f, 10.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_blueviolet);
        if (ImGui::Button(fmt::format("{}##InvPick", station_name).c_str()) && this->strct_flight_leg_info.iSliderItemQuantity) // make the item move if "slider quantity > 0"
        {
          // v24.12.2 there is no need for "move mandatory items" check, since we can move any item to the plane
          if (max_quantity_i - this->strct_flight_leg_info.iSliderItemQuantity >= 0)
          {
            // Do the ITEM MOVE - not committing yet.
            auto actionResult = inout_copied_plane_inventory.mapStations[station_id].add_item(this->strct_flight_leg_info.ptrNodePicked, this->strct_flight_leg_info.iSliderItemQuantity);
            if (actionResult.result)
            {
              // make sure the slider does not have a value larger than the "max_quantity_i"
              this->strct_flight_leg_info.iSliderItemQuantity = (max_quantity_i - this->strct_flight_leg_info.iSliderItemQuantity > 0) ? 1 : 0;
              if (this->strct_flight_leg_info.iSliderItemQuantity == 0)
                this->strct_flight_leg_info.resetItemMove(); // auto hide the move layer if quantity is Zero.
            }
            else
            {
              const auto msg = fmt::format("{}\n{}", actionResult.getErrorsAsText(), actionResult.getInfoAsText());
              this->setMessage(msg);
              #ifndef RELEASE
              Log::logMsg(msg);
              #endif
            }
          } // end if we have enough items
          else
          {
            this->setMessage(fmt::format("Not enough [{}] left to move.", itemName, 5));
          } // end move item evaluation

        } // end button ImGui
        ImGui::PopStyleColor();

        ++btn_indx;
      } // end loop over stations

    // Display the item image at the center of the "child" window. Make sure the image width is correctly compute.
    if (!image_file_name.empty() && data_manager::xp_mapInvImages[image_file_name].gTexture)
    {
      const auto lmbda_image_ratio = [&]()
      {
        if (data_manager::xp_mapInvImages[image_file_name].sImageData.getW_f() > container_dim.x)
          return container_dim.x / data_manager::xp_mapInvImages[image_file_name].sImageData.getW_f();

        return 1.0f;
      };

      const float wRatio_f = lmbda_image_ratio();
      const float xPos_f = std::fabs((container_dim.x - data_manager::xp_mapInvImages[image_file_name].sImageData.getW_f()*wRatio_f) * 0.5f);
      ImGui::NewLine();
      ImGui::SameLine(xPos_f);
      ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(data_manager::xp_mapInvImages[image_file_name].gTexture)), ImVec2(data_manager::xp_mapInvImages[image_file_name].sImageData.getW_f() * wRatio_f, data_manager::xp_mapInvImages[image_file_name].sImageData.getH_f() * wRatio_f) , this->uv0, this->uv1 );
    }

  } // end child
  ImGui::EndChild();
  ImGui::Separator();
}


// ------------------------------------------------


void WinImguiBriefer::child_draw_inv_plane_xp12(Inventory &inoutPlaneInventory, const missionx::mx_ui_inv_regions& inRegionType, const ImVec2& in_vec2_inv_child)
{
  static constexpr ImVec2 vec2_portrait(50.0f, 40.0f);

  const ImVec2 vec2_btnSize = { 250.0f, this->strct_setup_layer.fPreferredFontScale * data_manager::FONT_SIZE + 6.0f }; // need to be dynamic

  // loop over station containers and create "collapsible" widget for them
  for (auto &[station_id, acf_station]: inoutPlaneInventory.mapStations )
  {
    const auto vec2_inv_child = ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 150.0f);
    const std::string station_weight_information_s = fmt::format("{}", (0.0f < acf_station.max_allowed_weight )? fmt::format("{}/{}",acf_station.total_weight_in_station_f, acf_station.max_allowed_weight) : "" ); // add max suggested weight

    this->strct_flight_leg_info.mapStationHeaders[station_id].setState((ImGui::CollapsingHeader( fmt::format("{} [{}]kg ###station{}", this->strct_flight_leg_info.mapStationHeaders[station_id].title, station_weight_information_s, station_id).c_str() )));
    if (this->strct_flight_leg_info.mapStationHeaders[station_id].bState)
    {
      // create a child
      // loop over all items
      const ImGuiID child_id = ImGui::GetID(reinterpret_cast<void*>(static_cast<uint16_t>(station_id * 100)));
      ImGui::BeginChild(child_id, vec2_inv_child, ImGuiChildFlags_Borders); // extra 10.0 padding to see bottom message // extra -30 for commit/cancel buttons
      {
        const int nItems = acf_station.node.nChildNode(mxconst::get_ELEMENT_ITEM().c_str());

        for (int iPlaneItemLoop = 0; iPlaneItemLoop < nItems; ++iPlaneItemLoop)
        {
          IXMLNode          xPlane_Item_ptr  = acf_station.node.getChildNode (mxconst::get_ELEMENT_ITEM().c_str (), iPlaneItemLoop);
          const std::string barcode          = Utils::readAttrib (xPlane_Item_ptr, mxconst::get_ATTRIB_BARCODE(), "");
          const std::string itemName         = Utils::readAttrib (xPlane_Item_ptr, mxconst::get_ATTRIB_NAME(), barcode);
          const std::string quantity_s       = Utils::readAttrib (xPlane_Item_ptr, mxconst::get_ATTRIB_QUANTITY(), "0");
          const std::string image_file_name  = Utils::readAttrib (xPlane_Item_ptr, mxconst::get_PROP_IMAGE_FILE_NAME(), ""); // v3.0.303.5
          const auto        weight_f         = Utils::readNodeNumericAttrib<float> (xPlane_Item_ptr, mxconst::get_ATTRIB_WEIGHT_KG(), 0.0f);
          const auto        bItemIsMandatory = Utils::readBoolAttrib (xPlane_Item_ptr, mxconst::get_ATTRIB_MANDATORY(), false);

          if (itemName.empty() + barcode.empty()) // bug check
          {
            #ifndef RELEASE
            Log::logMsgErr("Found item without name or barcode in plane inventory. Try to fix inventory item definition. Notify developer if this is not the case. Skipping !!!");
            #endif
            continue; // skip
          }

          if (this->strct_flight_leg_info.left_index_image_clicked > -1 && this->strct_flight_leg_info.left_index_image_clicked != iPlaneItemLoop) // v3.0.303.5 we skip inventory lines that are not in focus
            continue;


          ImGui::PushID(iPlaneItemLoop); // display ID if any
          if (!image_file_name.empty() && data_manager::xp_mapInvImages[image_file_name].gTexture)
          {
            if (auto width1 = mxUiGetContentWidth(); (this->strct_flight_leg_info.left_index_image_clicked + 1) && this->strct_flight_leg_info.vec2_left_image_big.x < width1) // since left_index_image_clicked can have -1..n we add "1" so it will be the false logic value
              ImGui::SameLine((width1 - this->strct_flight_leg_info.vec2_left_image_big.x) * 0.5f);                                 // center image

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 0.0f)); // v4.23.4
            {
              if (ImGui::ImageButton("##InvLeftItemButtonImage", reinterpret_cast<void*>(static_cast<intptr_t>(data_manager::xp_mapInvImages[image_file_name].gTexture)), ((this->strct_flight_leg_info.left_index_image_clicked == -1) ? vec2_portrait : this->strct_flight_leg_info.vec2_left_image_big), this->uv0, this->uv1))
              {
                if (this->strct_flight_leg_info.left_index_image_clicked > -1)
                  this->strct_flight_leg_info.left_index_image_clicked = -1;
                else
                {
                  this->strct_flight_leg_info.left_index_image_clicked = iPlaneItemLoop;
                  // calculate image size for zoom display
                  this->strct_flight_leg_info.vec2_left_image_big = missionx::data_manager::getImageSizeBasedOnBorders(data_manager::xp_mapInvImages[image_file_name].sImageData.getW_f(), data_manager::xp_mapInvImages[image_file_name].sImageData.getH_f(), in_vec2_inv_child.x * 0.85f, in_vec2_inv_child.y * 0.85f);
                }
              }
            }
            ImGui::PopStyleVar();
          }
          ImGui::PopID();

          if (this->strct_flight_leg_info.left_index_image_clicked == -1)
          {
            bool bPressed = false;
            const std::string str = fmt::format("[{}] [{}kg] {}{}", quantity_s, Utils::formatNumber<float>(weight_f), itemName, ((bItemIsMandatory) ? "(*)" : ""));

            ImGui::SameLine(0.0f, 5.0f);
            ImGui::PushID(iPlaneItemLoop);
            {
              if (ImGui::Button(str.substr(0, 50).c_str(), vec2_btnSize))
                bPressed = true;
              this->mx_add_tooltip(missionx::color::color_vec4_white, str);
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::PushID(iPlaneItemLoop);
            {
              if (ImGui::ArrowButton("##right", ImGuiDir_Right))
                bPressed = true;
              this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Move to external inventory");
            }
            ImGui::PopID();

            ImGui::NewLine();

            if (bPressed)
            {
              if (this->strct_flight_leg_info.externalInventoryName.empty())
              {
                this->setMessage("No external inventory is present.", 10);
              }
              else
              {

                // check mandatory
                if (bItemIsMandatory)
                {
                  auto md_result = missionx::data_manager::check_mandatory_item_move(xPlane_Item_ptr, this->strct_flight_leg_info.externalInventoryName);
                  if (!md_result.result)
                  {
                    this->setMessage(md_result.getErrorsAsText(), 10);
                    bPressed = false;
                    continue;
                  }
                }

                // move item only if it exists and if external inventory is available
                if (const int quantity_i = Utils::stringToNumber<int>(quantity_s);
                   !(this->strct_flight_leg_info.externalInventoryName.empty())
                   && quantity_i > 0
                   && missionx::data_manager::externalInventoryCopy.node.nChildNode(mxconst::get_ELEMENT_ITEM().c_str()) < mxconst::MAX_ITEM_IN_INVENTORY )
                {
                  missionx::Inventory::move_item_to_station(acf_station, xPlane_Item_ptr, missionx::data_manager::externalInventoryCopy.node, barcode); // we also calculate station weight after this action.

                  #ifndef RELEASE
                  Log::logMsg("[DEBUG] ===> Inventories Information after Item Move <===\n");
                  Utils::xml_print_node(missionx::data_manager::planeInventoryCopy.node);
                  Utils::xml_print_node(missionx::data_manager::externalInventoryCopy.node);
                  #endif

                }
                else
                  this->setMessage(fmt::format("Check quantity of \"{}\" or target inventory might be full.", itemName), 10);
              }
              break; // exit plane item loop
            }
          }
        } // end loop over plane items

      } // end child
      ImGui::EndChild();
    }

  } // end loop over plane items

}


// ------------------------------------------------


void WinImguiBriefer::child_draw_inv_plane_xp11(const missionx::mx_ui_inv_regions &inRegionType, const ImVec2 &in_vec2_inv_child)
{
  ImVec2 vec2_btnSize = { 250.0f, this->strct_setup_layer.fPreferredFontScale * data_manager::FONT_SIZE + 6.0f }; // need to be dynamic
  static constexpr ImVec2 vec2_portrait(50.0f, 40.0f);

  const int nItems = missionx::data_manager::planeInventoryCopy.node.nChildNode(mxconst::get_ELEMENT_ITEM().c_str());


  for (int iPlaneItemLoop = 0; iPlaneItemLoop < nItems; ++iPlaneItemLoop)
  {
    IXMLNode          xPlane_Item_ptr = missionx::data_manager::planeInventoryCopy.node.getChildNode(mxconst::get_ELEMENT_ITEM().c_str(), iPlaneItemLoop);
    const std::string barcode         = Utils::readAttrib(xPlane_Item_ptr, mxconst::get_ATTRIB_BARCODE(), "");
    const std::string itemName        = Utils::readAttrib(xPlane_Item_ptr, mxconst::get_ATTRIB_NAME(), "");
    const std::string quantity_s      = Utils::readAttrib(xPlane_Item_ptr, mxconst::get_ATTRIB_QUANTITY(), "0");
    const std::string image_file_name  = Utils::readAttrib(xPlane_Item_ptr, mxconst::get_PROP_IMAGE_FILE_NAME(), ""); // v3.0.303.5
    const auto        weight_f         = Utils::readNodeNumericAttrib<float>(xPlane_Item_ptr, mxconst::get_ATTRIB_WEIGHT_KG(), 0.0f);
    const auto        bItemIsMandatory = Utils::readBoolAttrib(xPlane_Item_ptr, mxconst::get_ATTRIB_MANDATORY(), false);

    if (itemName.empty() || barcode.empty()) // bug check
    {
      #ifndef RELEASE
      Log::logMsgErr("Found item without name or barcode in plane inventory. Try to fix inventory item definition. Notify developer if this is not the case. Skipping !!!");
      #endif
      continue; // skip
    }

    if (this->strct_flight_leg_info.left_index_image_clicked > -1 && this->strct_flight_leg_info.left_index_image_clicked != iPlaneItemLoop) // v3.0.303.5 we skip inventory lines that are not in focus
      continue;

    std::string str = fmt::format("[{}] [{}kg] {}{}",quantity_s,Utils::formatNumber<float>(weight_f, 2), itemName, (bItemIsMandatory)?"*":"" );

    ImGui::PushID(iPlaneItemLoop); // display ID if any
    if (!image_file_name.empty() && data_manager::xp_mapInvImages[image_file_name].gTexture)
    {
      // since left_index_image_clicked can have -1...n we add "1" so it will be the false logic value
      if (const auto width1 = mxUiGetContentWidth ()
        ; (this->strct_flight_leg_info.left_index_image_clicked + 1) && this->strct_flight_leg_info.vec2_left_image_big.x < width1)
        ImGui::SameLine((width1 - this->strct_flight_leg_info.vec2_left_image_big.x) * 0.5f); // center image

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 0.0f)); // v4.23.4
      {
        if (ImGui::ImageButton("##InvLeftItemButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::xp_mapInvImages[image_file_name].gTexture)), ((this->strct_flight_leg_info.left_index_image_clicked == -1) ? vec2_portrait : this->strct_flight_leg_info.vec2_left_image_big), this->uv0, this->uv1))
        {
          if (this->strct_flight_leg_info.left_index_image_clicked > -1)
            this->strct_flight_leg_info.left_index_image_clicked = -1;
          else
          {
            this->strct_flight_leg_info.left_index_image_clicked = iPlaneItemLoop;
            // calculate image size for zoom display
            this->strct_flight_leg_info.vec2_left_image_big = missionx::data_manager::getImageSizeBasedOnBorders(data_manager::xp_mapInvImages[image_file_name].sImageData.getW_f(), data_manager::xp_mapInvImages[image_file_name].sImageData.getH_f(), in_vec2_inv_child.x * 0.85f, in_vec2_inv_child.y * 0.85f);
          }
        }
      }
      ImGui::PopStyleVar();
    }
    ImGui::PopID();

    if (this->strct_flight_leg_info.left_index_image_clicked == -1)
    {
      bool bPressed = false;
      ImGui::SameLine(0.0f, 5.0f);
      ImGui::PushID(iPlaneItemLoop);
      if (ImGui::Button(str.substr(0, 50).c_str(), vec2_btnSize))
        bPressed = true;
      this->mx_add_tooltip(missionx::color::color_vec4_white, str);

      ImGui::PopID();

      ImGui::SameLine();
      ImGui::PushID(iPlaneItemLoop);
      if (ImGui::ArrowButton("##right", ImGuiDir_Right))
        bPressed = true;
      this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Move to external inventory");

      ImGui::PopID();

      ImGui::NewLine();

      if (bPressed)
      {
        if (this->strct_flight_leg_info.externalInventoryName.empty())
        {
          this->setMessage("No external inventory is present.", 10);
        }
        else
        {
          // check mandatory
          auto md_result = missionx::data_manager::check_mandatory_item_move(xPlane_Item_ptr, this->strct_flight_leg_info.externalInventoryName);
          if (!md_result.result)
          {
            this->setMessage(md_result.getErrorsAsText(), 10);
            return; // exit function/loop
          }


          // move item
          if (int quantity_i = Utils::stringToNumber<int>(quantity_s)
            ;!(this->strct_flight_leg_info.externalInventoryName.empty()) && quantity_i > 0 && missionx::data_manager::externalInventoryCopy.node.nChildNode(mxconst::get_ELEMENT_ITEM().c_str()) < mxconst::MAX_ITEM_IN_INVENTORY /* check if we do not overflow */)
          {
            int targetQuantity_i = 0;

            // check if target inventory has the item with same barcode
            IXMLNode targetItemNode_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(missionx::data_manager::externalInventoryCopy.node, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), barcode, false);
            if (targetItemNode_ptr.isEmpty()) // if not available we need to create it
            {
              targetItemNode_ptr = missionx::data_manager::externalInventoryCopy.node.addChild(xPlane_Item_ptr.deepCopy()); // attach
              targetQuantity_i   = 1;
            }
            else
              targetQuantity_i = Utils::readNodeNumericAttrib<int>(targetItemNode_ptr, mxconst::get_ATTRIB_QUANTITY(), 0) + 1;

            --quantity_i; // now we have one less
            //// update nodes
            Utils::xml_set_attribute_in_node<int>(xPlane_Item_ptr, mxconst::get_ATTRIB_QUANTITY(), quantity_i, mxconst::get_ELEMENT_ITEM());          // set plane item quantity
            Utils::xml_set_attribute_in_node<int>(targetItemNode_ptr, mxconst::get_ATTRIB_QUANTITY(), targetQuantity_i, mxconst::get_ELEMENT_ITEM()); // set external item inventory quantity

            #ifndef RELEASE
            std::string plane_item_node  = missionx::data_manager::xmlRender.getString(missionx::data_manager::planeInventoryCopy.node, 1);
            std::string target_item_node = missionx::data_manager::xmlRender.getString(missionx::data_manager::externalInventoryCopy.node, 1);

            Log::logDebugBO(fmt::format("items info: {}\n\n{}\n", plane_item_node, target_item_node) );
            #endif
          }
          else
            this->setMessage(fmt::format("Check quantity of \"{}\" or target inventory might be full.", itemName), 10);
        }
        break; // exit item loop
      }
    }
  } // end loop over plane items

}


// ------------------------------------------------

void
WinImguiBriefer::child_draw_inv_external_store(const ImVec2& in_vec2_inv_child)
{
  const ImVec2 vec2_btnSize = { 250.0f, this->strct_setup_layer.fPreferredFontScale * data_manager::FONT_SIZE + 6.0f }; // need to be dynamic
  static constexpr ImVec2 vec2_portrait(50.0f, 40.0f);

  const bool bDisable = this->mxStartUiDisableState( this->strct_flight_leg_info.flagItemMoveWasPressedFromExternalInv );

  const int nItems = missionx::data_manager::externalInventoryCopy.node.nChildNode(mxconst::get_ELEMENT_ITEM().c_str());
  for (int iExternalItemLoop = 0; iExternalItemLoop < nItems && iExternalItemLoop < mxconst::MAX_ITEM_IN_INVENTORY; ++iExternalItemLoop)
  {
    IXMLNode          xExternal_Inv_Item_ptr = missionx::data_manager::externalInventoryCopy.node.getChildNode (mxconst::get_ELEMENT_ITEM().c_str (), iExternalItemLoop);
    const std::string barcode                = Utils::readAttrib (xExternal_Inv_Item_ptr, mxconst::get_ATTRIB_BARCODE(), "");
    const std::string itemName               = Utils::readAttrib (xExternal_Inv_Item_ptr, mxconst::get_ATTRIB_NAME(), barcode);
    const std::string quantity_s             = Utils::readAttrib (xExternal_Inv_Item_ptr, mxconst::get_ATTRIB_QUANTITY(), "0");
    const std::string image_file_name        = Utils::readAttrib (xExternal_Inv_Item_ptr, mxconst::get_PROP_IMAGE_FILE_NAME(), ""); // v3.0.303.5
    const auto        weight_f               = Utils::readNodeNumericAttrib<float> (xExternal_Inv_Item_ptr, mxconst::get_ATTRIB_WEIGHT_KG(), 0.0f);
    const auto        bMandatory             = Utils::readBoolAttrib (xExternal_Inv_Item_ptr, mxconst::get_ATTRIB_MANDATORY(), false);

    // pressedBarcode.clear();

    if (itemName.empty() || barcode.empty()) // bug checker
    {
      #ifndef RELEASE
      Log::logMsgErr("Found item without name or barcode in external inventory. Notify 4. Skipping !!!");
      #endif
      continue; // skip
    }

    if (this->strct_flight_leg_info.right_index_image_clicked > -1 && this->strct_flight_leg_info.right_index_image_clicked != iExternalItemLoop) // v3.0.303.5 we skip inventory lines that are not in focus
      continue;

    // DRAW BUTTONS
    bool bPressedExternal = false;
    if (this->strct_flight_leg_info.right_index_image_clicked == -1)
    {
      ImGui::PushID(iExternalItemLoop);
      ImGui::SameLine(30.0f);

      const auto item_quantity = mxUtils::stringToNumber<int>(quantity_s);

      if (ImGui::ArrowButton("##left", ImGuiDir_Left) && item_quantity)
        bPressedExternal = this->strct_flight_leg_info.setItemMoveFromExternal(xExternal_Inv_Item_ptr, (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY) );

      this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Move to Plane");
      ImGui::PopID();

      const std::string str = fmt::format("[{}] [{}kg] {}{}",item_quantity, weight_f, itemName, ((bMandatory)? "(*)" : "")); // v24.12.2 formated string
      ImGui::SameLine();
      if (ImGui::Button(str.substr(0, 50).c_str(), vec2_btnSize) && item_quantity)
        bPressedExternal = this->strct_flight_leg_info.setItemMoveFromExternal(xExternal_Inv_Item_ptr, (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY));

      this->mx_add_tooltip(missionx::color::color_vec4_white, str);
    }

    ImGui::PushID(iExternalItemLoop);
    if (!image_file_name.empty() && data_manager::xp_mapInvImages[image_file_name].gTexture)
    {
      auto x1 = mxUiGetContentWidth();
      if ((this->strct_flight_leg_info.right_index_image_clicked + 1) && this->strct_flight_leg_info.vec2_right_image_big.x < x1) // since right_index_image_clicked can have -1..n we add "1" so it will be the false logic value
        ImGui::SameLine((x1 - this->strct_flight_leg_info.vec2_right_image_big.x) * 0.5f);                                  // center image
      else
        ImGui::SameLine(0.0f, 5.0f);

      if (ImGui::ImageButton("##InvRightItemButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::xp_mapInvImages[image_file_name].gTexture)), ((this->strct_flight_leg_info.right_index_image_clicked == -1) ? vec2_portrait : this->strct_flight_leg_info.vec2_right_image_big), this->uv0, this->uv1)) // padding 4
      {
        if (this->strct_flight_leg_info.right_index_image_clicked > -1)
          this->strct_flight_leg_info.right_index_image_clicked = -1;
        else
        {
          this->strct_flight_leg_info.right_index_image_clicked = iExternalItemLoop;
          // calculate image size for zoom display
          this->strct_flight_leg_info.vec2_right_image_big = missionx::data_manager::getImageSizeBasedOnBorders(data_manager::xp_mapInvImages[image_file_name].sImageData.getW_f(), data_manager::xp_mapInvImages[image_file_name].sImageData.getH_f(), in_vec2_inv_child.x * 0.85f, in_vec2_inv_child.y * 0.85f);
        }
      }
    }
    ImGui::PopID();

    ImGui::NewLine();

    // HANDLE BUTTON PRESSED __ONLY IF__ WE USE XP11 COMPATIBILITY CODE. THIS CODE MUST NOT RUN for XP12 Mode
    // auto outcome = (bPressedExternal) + (missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO);
    if ( (bPressedExternal) * (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY) )
    {
      if (int quantity_i = Utils::stringToNumber<int>(quantity_s); !(this->strct_flight_leg_info.externalInventoryName.empty()) && quantity_i > 0)
      {
        int targetQuantity_i = 0;

        // get plane item node pointer
        IXMLNode targetItemNode_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(missionx::data_manager::planeInventoryCopy.node, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), barcode, false);
        if (targetItemNode_ptr.isEmpty())
        {
          targetItemNode_ptr = missionx::data_manager::planeInventoryCopy.node.addChild(xExternal_Inv_Item_ptr.deepCopy()); // attach the child xml item to the xExternal_Inv_Item_ptr.deepCopy(); // make a copy of the item = create new item
          targetQuantity_i   = 1;
          assert(targetItemNode_ptr.isEmpty() == false && fmt::format("[{}] Target inv node is empty()", __func__).c_str());
        }
        else
          targetQuantity_i = static_cast<int>(Utils::readNumericAttrib(targetItemNode_ptr, mxconst::get_ATTRIB_QUANTITY(), 0.0) + 1.0);

        --quantity_i; // decrease from source inventory item
        //// update nodes
        Utils::xml_set_attribute_in_node<int>(xExternal_Inv_Item_ptr, mxconst::get_ATTRIB_QUANTITY(), quantity_i, mxconst::get_ELEMENT_ITEM());   // decrease from external inventory
        Utils::xml_set_attribute_in_node<int>(targetItemNode_ptr, mxconst::get_ATTRIB_QUANTITY(), targetQuantity_i, mxconst::get_ELEMENT_ITEM()); // set in plane inventory

        std::string inv_item_node    = missionx::data_manager::xmlRender.getString(missionx::data_manager::externalInventoryCopy.node, 1);
        std::string target_item_node = missionx::data_manager::xmlRender.getString(missionx::data_manager::planeInventoryCopy.node, 1);

        Log::logDebugBO(fmt::format("items info: \n{}\n{}\n"
                      , Utils::xml_get_node_content_as_text(missionx::data_manager::externalInventoryCopy.node)
                      , Utils::xml_get_node_content_as_text(missionx::data_manager::planeInventoryCopy.node) ) );
      }
      else
        this->setMessage(fmt::format("Check quantity of \"{}\" or target inventory might be full.", itemName), 5);

      break;
    }
  } // end loop over external inventory items

  this->mxEndUiDisableState(bDisable);
}

// ------------------------------------------------

void
WinImguiBriefer::child_flight_leg_info_draw_map()
{
  static bool bMapPressed = false;
  const static ImVec2 vec2_estimate_button_size = ImGui::CalcTextSize("<< Prev");
  constexpr ImVec2    max_image_bounds(750.0f, 235.0f);

  // Display NEXT, PREV Buttons
  ImGui::NewLine();
  ImGui::BeginGroup();
  {
    const float pos_x = ImGui::GetWindowWidth() * 0.5f - vec2_estimate_button_size.x;
    if (missionx::data_manager::strct_flight_leg_info_totalMapsCounter > 1)
    {
      int iStyle = 0;
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      iStyle++;
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_darkorange);
      iStyle++;

      ImGui::SameLine(pos_x);

      if (ImGui::Button("<< Prev"))
      {
        if (strct_flight_leg_info.iMapNumberToDisplay - 1 <= 0)
          strct_flight_leg_info.iMapNumberToDisplay = missionx::data_manager::strct_flight_leg_info_totalMapsCounter;
        else
          strct_flight_leg_info.iMapNumberToDisplay--;
      }

      this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Previous map");

      ImGui::SameLine(0.0f, 20.0f);
      if (ImGui::Button("Next >>"))
      {
        if (missionx::data_manager::strct_flight_leg_info_totalMapsCounter > 1)
        {
          if (strct_flight_leg_info.iMapNumberToDisplay + 1 > missionx::data_manager::strct_flight_leg_info_totalMapsCounter)
            strct_flight_leg_info.iMapNumberToDisplay = 1;
          else
            strct_flight_leg_info.iMapNumberToDisplay++;
        }
      }
      this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Next Map");

      ImGui::PopStyleColor(iStyle);
    }
  }
  ImGui::EndGroup();

  ImGui::BeginGroup();
  ImGui::BeginChild("child_flight_leg_info_draw_map", ImVec2(0.0f, ImGui::GetWindowHeight() - imvec2_flight_info_top_area_size.y - this->fBottomToolbarPadding_f - this->fTopToolbarPadding_f), ImGuiChildFlags_None,
                    ImGuiWindowFlags_HorizontalScrollbar); // extra 10.0 padding to see bottom message // extra -30 for commit/cancel buttons
  {
    // v3.303.13 special check if designer added new image and strct_flight_leg_info.iMapNumberToDisplay is ZERO (0) then set it to 1
    if (!missionx::data_manager::maps2d_to_display.empty() && this->strct_flight_leg_info.iMapNumberToDisplay <= 0)
    {
      this->strct_flight_leg_info.iMapNumberToDisplay = (*missionx::data_manager::maps2d_to_display.begin()).first;
    }

    if (this->strct_flight_leg_info.iMapNumberToDisplay < 1)
    {
      ImGui::NewLine();
      ImGui::SetWindowFontScale(2.0f);
      ImGui::TextColored(missionx::color::color_vec4_magenta, "There are no maps or informational images... move along... ");
      ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
    }
    else
    {
      if (Utils::isElementExists(missionx::data_manager::maps2d_to_display, this->strct_flight_leg_info.iMapNumberToDisplay))
      {
        const auto sizeW = missionx::data_manager::maps2d_to_display[this->strct_flight_leg_info.iMapNumberToDisplay].sImageData.getW_f();
        const auto sizeH = missionx::data_manager::maps2d_to_display[this->strct_flight_leg_info.iMapNumberToDisplay].sImageData.getH_f();

        ImVec2 size_vec2(sizeW, sizeH); // v3.0.303.5 initialize with image original size
        if (!bMapPressed)
        {
          size_vec2 = missionx::data_manager::getImageSizeBasedOnBorders(sizeW, sizeH, max_image_bounds.x, max_image_bounds.y); // v3.0.303.5
        }

        auto x1 = mxUiGetContentWidth();
        if (size_vec2.x < x1)
          ImGui::SameLine((x1 - size_vec2.x) * 0.5f); // center image

        if (ImGui::ImageButton("MapOrImageButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (missionx::data_manager::maps2d_to_display[this->strct_flight_leg_info.iMapNumberToDisplay].gTexture)), size_vec2))
        {
          bMapPressed = !bMapPressed;
        }
      }
    }
  }
  ImGui::EndChild();
  ImGui::EndGroup();
} // end child_flight_leg_info_draw_map

// ------------------------------------------------

void
WinImguiBriefer::child_flight_leg_info_draw_end_summary()
{
  constexpr const ImVec2   sizeChild(0.0f, 315.0f);
  constexpr const ImVec2   max_image_bounds(775.0f, 300.0f);
  const static float fWinWidth = mxUiGetContentWidth();
  const auto         sizeW     = this->strct_flight_leg_info.endTexture.sImageData.getW_f();
  const auto         sizeH     = this->strct_flight_leg_info.endTexture.sImageData.getH_f();

  ImVec2      imgSize_vec2(sizeW, sizeH); // // v3.0.303.5 initialize with image original size
  static bool bImagePressed = false;

  // v3.0.255.1 add stats button
  ImGui::BeginGroup();
  if (ImGui::Button((this->strct_flight_leg_info.bStatsPressed) ? "image" : "stats"))
  {
    this->strct_flight_leg_info.bStatsPressed = !this->strct_flight_leg_info.bStatsPressed;
    if (this->strct_flight_leg_info.bStatsPressed)
      this->strct_flight_leg_info.fetch_state = missionx::mxFetchState_enum::fetch_not_started; // force re-read from DB next [stats] press.
  }
  ImGui::EndGroup();

  if (this->strct_flight_leg_info.bStatsPressed)
  {
    // call read from sqlite, stats table.
    if (this->strct_flight_leg_info.fetch_state == missionx::mxFetchState_enum::fetch_not_started)
    {
      ImGui::BeginGroup();

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED()); // v3.303.14
      ImGui::TextColored(missionx::color::color_vec4_yellow, "Please wait while reading the stats...");
      this->mxUiReleaseLastFont(); // v3.303.14

      ImGui::EndGroup();

      this->execAction(missionx::mx_window_actions::ACTION_FETCH_MISSION_STATS);
    }
    else if (this->strct_flight_leg_info.fetch_state == missionx::mxFetchState_enum::fetch_ended)
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1()); // v3.303.14

      ImGui::BeginGroup();
      ImGui::TextColored(missionx::color::color_vec4_yellowgreen, "Time Flew: ");
      ImGui::SameLine();
      ImGui::TextColored(missionx::color::color_vec4_white, "%s", data_manager::mission_stats_from_query.time_flew_format_s.c_str());

      // 3.303.8.3 show min/max stats
      const auto fThird  = fWinWidth * 0.333f;
      const auto f2Third = fWinWidth * 0.666f;

      if (missionx::data_manager::mx_global_settings.xScoring_ptr.isEmpty())
        ImGui::TextColored(missionx::color::color_vec4_orangered, "!!! No Scoring information is available !!!");

      ImGui::TextColored(missionx::color::color_vec4_yellowgreen, "Min Pitch: %.2f", data_manager::mission_stats_from_query.minPitch);
      ImGui::SameLine(fThird, 0.0f);
      ImGui::TextColored(missionx::color::color_vec4_yellowgreen, "Max Pitch: %.2f", data_manager::mission_stats_from_query.maxPitch);
      ImGui::SameLine(f2Third, 0.0f);
      ImGui::TextColored(missionx::color::color_vec4_beige, "Score: %.2f", data_manager::mission_stats_from_query.pitchScore);

      ImGui::TextColored(missionx::color::color_vec4_yellowgreen, "Min Roll: %.2f", data_manager::mission_stats_from_query.minRoll);
      ImGui::SameLine(fThird, 0.0f);
      ImGui::TextColored(missionx::color::color_vec4_yellowgreen, "Max Roll: %.2f", data_manager::mission_stats_from_query.maxRoll);
      ImGui::SameLine(f2Third, 0.0f);
      ImGui::TextColored(missionx::color::color_vec4_beige, "Score: %.2f", data_manager::mission_stats_from_query.rollScore);

      ImGui::TextColored(missionx::color::color_vec4_yellowgreen, "Min G: %.2f", data_manager::mission_stats_from_query.minGforce);
      ImGui::SameLine(fThird, 0.0f);
      ImGui::TextColored(missionx::color::color_vec4_yellowgreen, "Max G: %.2f", data_manager::mission_stats_from_query.maxGforce);
      ImGui::SameLine(f2Third, 0.0f);
      ImGui::TextColored(missionx::color::color_vec4_beige, "Score: %.2f", data_manager::mission_stats_from_query.gForceScore);

      ImGui::TextColored(missionx::color::color_vec4_beige, "Total Flight Score: %.2f", data_manager::mission_stats_from_query.pitchScore + data_manager::mission_stats_from_query.rollScore + data_manager::mission_stats_from_query.gForceScore);

      // min/max

      ImGui::Separator();
      for (auto& land : missionx::data_manager::mission_stats_from_query.vecLandingStatsResults)
      {

        ImGui::Text("Landed in: ");
        ImGui::SameLine();
        ImGui::TextColored(missionx::color::color_vec4_green, "%s", land.icao.c_str());

        ImGui::SameLine();
        ImGui::Text(", Runway: ");
        ImGui::SameLine();
        ImGui::TextColored(missionx::color::color_vec4_lightblue, "%s", land.runway_s.c_str());

        ImGui::SameLine();
        ImGui::Text(", Accuracy from center: ");
        ImGui::SameLine();
        ImGui::TextColored(missionx::color::color_vec4_beige, "%s", mxUtils::formatNumber<double>(land.distance_from_center_of_rw_d, 2).c_str());

        ImGui::SameLine();
        ImGui::Text(" => Score: ");
        ImGui::SameLine();
        ImGui::TextColored(missionx::color::color_vec4_white, "%s", mxUtils::formatNumber<double>(land.score_centerLine, 4).c_str());
      }
      ImGui::Separator();

      ImGui::EndGroup();

      this->mxUiReleaseLastFont(); // v3.303.14

      ////// Draw per waypoint stats
      this->add_ui_stats_child(true); // v3.303.14

      ////// Draw stats graph using ImPlot
      ImGui::BeginGroup();
      ImGui::BeginChild("child_statistics", sizeChild, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar); // draw on the all the remaining window area

      if (ImPlot::BeginPlot("Mission Stats", ImVec2(-1, 0))) //, ImPlotFlags_YAxis2) )
      {

        ImPlot::SetupAxes("Elapsed Time", "Elev. ft");
        ImPlot::SetupAxesLimits(0.0,
                                static_cast<double> (data_manager::mission_stats_from_query.vecSeq_d.size ()),
                                ((data_manager::mission_stats_from_query.min_elev_ft > 0.0) ? 0.0 : data_manager::mission_stats_from_query.min_elev_ft),
                                (data_manager::mission_stats_from_query.max_elev_ft > data_manager::mission_stats_from_query.max_airspeed) ? data_manager::mission_stats_from_query.max_elev_ft + 100.0 : 1000.0);


        ImPlot::SetupAxis(ImAxis_X1, "Timelapse", ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_Lock);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, static_cast<int> (missionx::data_manager::mission_stats_from_query.vecSeq_d.size ()));

        ImPlot::SetupAxis(ImAxis_Y1, "Elevation", ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_Lock);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, static_cast<int> (missionx::data_manager::mission_stats_from_query.max_elev_ft + 1000.0));

        ImPlot::SetupAxis(ImAxis_Y2, "Airspeed", ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_Lock);
        ImPlot::SetupAxisLimits(ImAxis_Y2, 0.0, static_cast<int> (missionx::data_manager::mission_stats_from_query.max_airspeed + 100.0));

        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
        ImPlot::PlotLine("Elevation ft", missionx::data_manager::mission_stats_from_query.vecSeq_d.data(), missionx::data_manager::mission_stats_from_query.vecElev_ft_d.data(), static_cast<int> (missionx::data_manager::mission_stats_from_query.vecSeq_d.size ()));

        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
        ImPlot::PlotLine("Air Speed", missionx::data_manager::mission_stats_from_query.vecSeq_d.data(), missionx::data_manager::mission_stats_from_query.vecAirSpeed_d.data(), static_cast<int> (missionx::data_manager::mission_stats_from_query.vecSeq_d.size ()));

        ImPlot::EndPlot();
      }

      ImGui::EndChild();
      ImGui::EndGroup();
    }
  }
  else // display image
  {
    if (this->strct_flight_leg_info.endTexture.gTexture == 0)
      return;



    ImGui::BeginGroup();
    ImGui::BeginChild("child_flight_leg_info_draw_end_summary", sizeChild, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar); // draw on the all the remaining window area


    // if (!bImagePressed + (imgSize_vec2.x < max_image_bounds.x))
    if (!bImagePressed) // no need for the second test because it is only relevant when pressing the button
    {
      imgSize_vec2 = missionx::data_manager::getImageSizeBasedOnBorders(sizeW, sizeH, max_image_bounds.x, max_image_bounds.y); // v3.0.303.5
      ImGui::SameLine((fWinWidth - imgSize_vec2.x) * 0.5f);
    }

    if (ImGui::ImageButton("EndButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (this->strct_flight_leg_info.endTexture.gTexture)), imgSize_vec2))
    {
      bImagePressed = !bImagePressed;
    }

    ImGui::EndChild();

    ImGui::EndGroup();
  }
}

// ------------------------------------------------

void
WinImguiBriefer::draw_load_existing_mission_screen()
{
  if (this->strct_pick_layer.bFinished_loading_mission_images)
  {
    const auto                   win_size_vec2           = this->mxUiGetWindowContentWxH();
    constexpr static const float fImageContainerWidth_px = 280.0f;

    constexpr float child_w[] = { fImageContainerWidth_px, 0.0f }; // v3.305.1 0.0f = until the end of the window so no calculation is needed from our end

    ImGui::PushID("##VerticalPickMissionScrolling");
    for (int i = 0; i < 2; ++i)
    {
      if (i > 0)
        ImGui::SameLine();
      ImGui::BeginGroup();
      const char* names[] = { "Pick a Mission", "Mission Description" };

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
      ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", names[i]);

      // add font size buttons
      if (i == 1)
      {
        ImGui::SameLine(0.0f, 100.0f);
        add_font_size_scale_buttons();
      }

      this->mxUiReleaseLastFont();


      // Draw 2 regions
      const ImGuiID child_id = ImGui::GetID(reinterpret_cast<void *> (static_cast<intptr_t> (i)));
      ImGui::BeginChild(child_id, ImVec2(child_w[i], win_size_vec2.y * 0.75f), ImGuiChildFlags_Borders);
      {
        if (i == 0)
        {
          // DISPLAY IMAGES
          int imgIdNo = 0; // v3.303.14

          for (const auto &imageFileKey : data_manager::mapBrieferMissionListLocator | std::views::values) // missionx::data_manager::xp_mapMissionIconImages)
          {
            int iStyle = 0;
            if (imageFileKey.empty())
              continue;

            if (mxUtils::isElementExists(missionx::data_manager::xp_mapMissionIconImages, imageFileKey))
            {
              static constexpr ImVec2 vec2_landscape(240.0f, 190.0f);
              static constexpr ImVec2 vec2_portrait(145.0f, 210.0f);
              // calculate image ration and decide which size to use the new one: 240x190 or the old one 290x420. portrait vs landscape
              const bool bIsLandscape = (data_manager::xp_mapMissionIconImages[imageFileKey].sImageData.getW_f() / data_manager::xp_mapMissionIconImages[imageFileKey].sImageData.getH_f() > 1.0f) ? true : false;

              bool bSetKeyStyle = false;
              if (this->strct_pick_layer.last_picked_key == imageFileKey)
              {
                bSetKeyStyle = true;
                ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_green);
                iStyle++; // );
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_greenyellow);
                iStyle++;
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_green);
                iStyle++;
              }

              // display image button with the correct ratio for backwards compatibility. The image should be centered
              if (bIsLandscape)    // v3.0.255.4.1 added so image separator will be same for landscape and portrait
                ImGui::SameLine(); // center portrait image
              else
                ImGui::SameLine((child_w[0] - vec2_portrait.x) / 2.0f); // center portrait image

              ImGui::PushID(imgIdNo); // v3.303.14
              {
                if (data_manager::xp_mapMissionIconImages[imageFileKey].gTexture && ImGui::ImageButton("##MxImgBtn", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::xp_mapMissionIconImages[imageFileKey].gTexture)), (bIsLandscape) ? vec2_landscape : vec2_portrait, this->uv0, this->uv1))
                {
                  // prepare template briefer text so we will display it in the detailed region
                  this->strct_pick_layer.last_picked_key = imageFileKey;
                  this->setMessage("Picked: " + this->strct_pick_layer.last_picked_key);

                  // v3.0.251.b2 fix cases when pick different missions but [start mission] button is not reset to [load mission]
                  if (data_manager::missionState > missionx::mx_mission_state_enum::mission_undefined)
                    data_manager::missionState = missionx::mx_mission_state_enum::mission_undefined;
                  #ifndef RELEASE
                  Log::logMsg("Picked: " + this->strct_pick_layer.last_picked_key); // debug
                  #endif
                }
              }
              ImGui::PopID(); // v3.303.14
              ++imgIdNo;      // v3.303.14


              if (bSetKeyStyle)
                ImGui::PopStyleColor(iStyle);

              ImGui::NewLine(); //  so we won't have overlapping images (landscape and portrait)
            }
            else
            {
              continue; // skip this mission
            }

          } // end loop over all images
        }
        else
        {
          if (!this->strct_pick_layer.last_picked_key.empty())
          {
            ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);
            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL());

            ImGui::Text("Title: ");
            ImGui::SameLine();
            ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", data_manager::mapBrieferMissionList[this->strct_pick_layer.last_picked_key].missionTitle.c_str());
            ImGui::Text("Written By: ");
            ImGui::SameLine();
            ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", data_manager::mapBrieferMissionList[this->strct_pick_layer.last_picked_key].written_by.c_str());
            ImGui::Text("Plane Description: ");
            ImGui::SameLine();
            ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", data_manager::mapBrieferMissionList[this->strct_pick_layer.last_picked_key].planeTypeDesc.c_str());
            ImGui::Text("Difficulty: ");
            ImGui::SameLine();
            ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", data_manager::mapBrieferMissionList[this->strct_pick_layer.last_picked_key].difficultyDesc.c_str());
            ImGui::Text("Estimate Time: ");
            ImGui::SameLine();
            ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", data_manager::mapBrieferMissionList[this->strct_pick_layer.last_picked_key].estimateTimeDesc.c_str());
            ImGui::Text("Weather Setup: ");
            ImGui::SameLine();
            ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", data_manager::mapBrieferMissionList[this->strct_pick_layer.last_picked_key].weatherDesc.c_str());
            ImGui::Text("Scenery Setup: ");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
            ImGui::TextWrapped("%s", data_manager::mapBrieferMissionList[this->strct_pick_layer.last_picked_key].scenery_settings.c_str());
            ImGui::PopStyleColor(1);
            ImGui::Text("Other Settings: ");
            ImGui::SameLine();
            ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", data_manager::mapBrieferMissionList[this->strct_pick_layer.last_picked_key].other_settings.c_str());
            // ----- finish mission properties

            ImGui::Separator();

            // display briefer details
            ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
            ImGui::TextWrapped("%s", data_manager::mapBrieferMissionList[this->strct_pick_layer.last_picked_key].missionDesc.c_str());

            this->mxUiResetAllFontsToDefault();
            ImGui::PopStyleColor(1);

            ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
          }
        }
      }

      ImGui::EndChild();
      ImGui::EndGroup();
    }
    ImGui::PopID();

    // ------------------------
    // ------- Buttons
    // ------------------------

    ImGui::NewLine();
    ImGui::BeginGroup();
    ///// Display Load Buttons
    constexpr float pos_x                    = child_w[0] + 10.0f;
    if (!this->strct_pick_layer.last_picked_key.empty())
    {
      bool bDrawLoadSavepointButton = false;
      if ((data_manager::missionState < missionx::mx_mission_state_enum::pre_mission_running) && (data_manager::missionState != missionx::mx_mission_state_enum::mission_loaded_from_savepoint))
      {
        ImGui::SameLine(pos_x);
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_blueviolet);

        constexpr auto popupReminder = "Do you want to load the Save Point";

        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());

        if (ImGui::Button("Load Savepoint"))
          ImGui::OpenPopup(popupReminder); // v3.303.12_r3 make it a popup

        this->mxUiReleaseLastFont();

        ImGui::PopStyleColor(1);
        bDrawLoadSavepointButton = true;

        //// Randomize Date and Time popup
        ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(450.0f, 140.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_black);

        // Start reminder popup
        if (ImGui::BeginPopupModal(popupReminder, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
          ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_black);
          ImGui::BeginChild("quiteModalChild", ImVec2(0.0f, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_None);

          this->mxUiSetFont(mxconst::get_TEXT_TYPE_MSG_POPUP());

          ImGui::TextWrapped("1. Remember to load and start the plane prior to loading the save point \n\t(if that was the state when you took it).\n2. It is best to load a save point that was taken on the ground.\n3. You might need to disable the servo (auto pilot) if Yoke/Stick do not auto center.");
          ImGui::Separator();

          this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
          if (ImGui::Button("Continue Loading the Savepoint"))
          {
            this->execAction(missionx::mx_window_actions::ACTION_LOAD_SAVEPOINT); // should hide the window
            ImGui::CloseCurrentPopup();
          }
          ImGui::SameLine(0.0f, 50.0f);
          if (ImGui::Button("Cancel"))
          {
            ImGui::CloseCurrentPopup();
          }

          this->mxUiReleaseLastFont(2);

          ImGui::EndChild();
          ImGui::PopStyleColor(); // inner child

          ImGui::EndPopup();
        } // end Reminder Popup

        ImGui::PopStyleColor(1);
      }

      // place the next button
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());

      if (bDrawLoadSavepointButton)
      {
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
      }
      else
        ImGui::SameLine(pos_x);
      // draw "load mission" or "start mission" buttons
      if (data_manager::missionState < missionx::mx_mission_state_enum::mission_loaded_from_the_original_file)
      {
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_blueviolet);
        if (ImGui::Button("Load Mission file"))
          this->execAction(missionx::mx_window_actions::ACTION_LOAD_MISSION); // should hide the window
        ImGui::PopStyleColor(1);
      }
      else if (data_manager::missionState == missionx::mx_mission_state_enum::mission_loaded_from_the_original_file || data_manager::missionState == missionx::mx_mission_state_enum::mission_loaded_from_savepoint) // mission loaded state is decided in Mission class
      {
        this->add_ui_start_mission_button(missionx::mx_window_actions::ACTION_START_MISSION);
      }

      this->mxUiReleaseLastFont();

    } // end if strct_pick_layer.key has value
    ImGui::EndGroup();
  } // display Pick Layer
  else
  {
    ImGui::NewLine();

    // ImGui::SetWindowFontScale(2.0f);
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_BIG());
    ImGui::TextColored(missionx::color::color_vec4_magenta, "Please wait while loading mission files.... ");
    this->mxUiReleaseLastFont();

    ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
  }
}

// ------------------------------------------------

void
WinImguiBriefer::draw_external_fpln_screen ()
{

  switch (this->strct_ext_layer.ext_screen)
  {
    case mx_ext_fpln_screen::ext_db_fpln:
    {
      draw_child_ext_fpln_db_site_screen();
    }
    break;
    case mx_ext_fpln_screen::ext_simbrief:
    {
      setLayer (missionx::uiLayer_enum::flight_leg_info);
      missionx::data_manager::queFlcActions.push (missionx::mx_flc_pre_command::fetch_simbrief_fpln);
    }
    break;
    default: // mx_ext_fpln_screen::ext_home
      draw_child_ext_fpln_home_screen ();
  }

}



void
WinImguiBriefer::draw_child_ext_fpln_home_screen ()
{
  constexpr static const auto btn_size_vec2 = ImVec2(200.0f, 200.0f);
  const auto win_size_vec2 = this->mxUiGetWindowContentWxH ();

  ImGui::Spacing ();
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_BIG());
  ImGui::TextWrapped ("%s", "Pick one of the options below.");
  ImGui::Separator ();
  this->mxUiReleaseLastFont ();

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG());
  ImGui::Spacing ();
  ImGui::Spacing ();
  ImGui::Spacing ();
  if (ImGui::BeginTable ("Flight Plan Options", 2))
  {
    ImGui::TableNextColumn ();
    {
      if (ImGui::ImageButton ("FlightPlanDB", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_FLIGHTPLANDB()].gTexture)), btn_size_vec2))
      {
        if (this->strct_ext_layer.from_icao.empty ())
        {
          NavAidInfo nNavAid = data_manager::getPlaneAirportOrNearestICAO ();
          if (!nNavAid.getID ().empty ())
            std::memcpy (this->strct_ext_layer.buf_from_icao, nNavAid.ID, 10);
          // Change layer
          this->strct_ext_layer.from_icao = std::string (this->strct_ext_layer.buf_from_icao);
        }

        this->strct_ext_layer.ext_screen = mx_ext_fpln_screen::ext_db_fpln;
      } // end imageButton - flightplandb
      this->mx_add_tooltip (missionx::color::color_vec4_white, "Fetch flight plans based on flightplandatabase.com data.");
      // bottom title
      ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "%s", R"(Fetch Flight plan from "flightplandatabase.com")");

      ImGui::BeginChild ("flightplandatabase Key", ImVec2 (0.0f, 0.0f), ImGuiChildFlags_Borders);
      this->add_ui_flightplandb_key (false);
      ImGui::EndChild ();
    } // end flightplandb tab column

    ImGui::TableNextColumn ();
    {
      if (ImGui::ImageButton ("Simbrief", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_SIMBRIEF_BIG()].gTexture)), btn_size_vec2 ))
      {
        this->strct_ext_layer.ext_screen = mx_ext_fpln_screen::ext_simbrief;
      }
      this->mx_add_tooltip (missionx::color::color_vec4_white, "Construct a flight plan based on Simbrief data.");
      // bottom title
      ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "%s", R"(Fetch Flight plan from "Simbrief")");

      ImGui::BeginChild ("Simbrief Pilot ID", ImVec2 (0.0f, 0.0f), ImGuiChildFlags_Borders);
      this->add_ui_simbrief_pilot_id ();
      ImGui::EndChild ();
    } // end Simbrief tab column
    ImGui::EndTable ();
  }
  this->mxUiReleaseLastFont ();

}



void WinImguiBriefer::draw_child_ext_fpln_db_site_screen ()
{
  const auto                   win_size_vec2  = this->mxUiGetWindowContentWxH ();
  constexpr const static float ga_range_begin = 0.0f, ga_range_end = 9000.0f, ga_steps = 20.0f;
  constexpr const static char *limit_items[] = { "10", "20", "30", "40", "60", "80", "100" };
  constexpr const static char *sort_items[]  = { "created", "updated", "popularity", "distance" };

  // first time
  if (this->strct_ext_layer.flag_first_time)
  {
    this->strct_ext_layer.flag_first_time ^= 1;
    if (this->strct_ext_layer.flag_remove_duplicate_airport_names)
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_REMOVE_DUPLICATE_ICAO_ROWS(), this->strct_ext_layer.flag_remove_duplicate_airport_names);

    // v24.06.1
    if ( mxUtils::trim( Utils::getNodeText_type_6(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_AUTHORIZATION_KEY(), "" ) ).empty() )
        this->strct_ext_layer.flag_flightplandatabase_auth_exists = false;
    else
        this->strct_ext_layer.flag_flightplandatabase_auth_exists = true;
  }


  ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.305.1

  missionx::WinImguiBriefer::HelpMarker("This screen is dependent on \"flightplandatabase.com\".\n Fill in the information to query from the external site.");
  ImGui::SameLine();
  ImGui::TextUnformatted("Flight plans are fetched from \"flightplandatabase.com\". It is only for Flight Simulation.");

  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
  ImGui::TextUnformatted("You must fill one of the fields in Yellow labels");
  ImGui::PopStyleColor();

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1
  ImGui::BeginChild("draw_external_fpln_layer_01", ImVec2(0.0f, win_size_vec2.y * 0.30f), ImGuiChildFlags_Borders); // consume 1/3 of screen
  {
    missionx::WinImguiBriefer::HelpMarker("FROM or TO fields needs to have a value");
    ImGui::SameLine();
    {
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
      ImGui::PushItemWidth(200.0f);
      ImGui::PushID("#FromIcao");
      if (ImGui::InputText("", this->strct_ext_layer.buf_from_icao, 8, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
      {
        this->strct_ext_layer.from_icao = std::string(this->strct_ext_layer.buf_from_icao);
      }
      ImGui::PopID();
      this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Enter departure airport ICAO code");

      ImGui::SameLine();

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
      if (ImgWindow::ButtonTooltip(mxUtils::from_u8string(ICON_FA_TRASH_ALT).append( "##ClearFromICAO" ).c_str(), "Clear from ICAO"))
      {
        this->strct_ext_layer.from_icao.clear();
        memset(this->strct_ext_layer.buf_from_icao, 0, sizeof this->strct_ext_layer.buf_from_icao);
      }
      this->mxUiReleaseLastFont();

      ImGui::SameLine();
      if (ImGui::Button("From ICAO")) // first time initialization or manual ICAO fetch
      {
        NavAidInfo nNavAid = data_manager::getPlaneAirportOrNearestICAO();
        if (!nNavAid.getID().empty())
          std::memcpy(this->strct_ext_layer.buf_from_icao, nNavAid.ID, 10);

        this->strct_ext_layer.from_icao = std::string(this->strct_ext_layer.buf_from_icao); // first initialization
      }
      this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Plugin will pick closest airport ICAO for departure");

      // v3.0.253.3 add TO search location
      ImGui::SameLine(0.0f, 10.0f);
      ImGui::PushID("##ToICAO");
      if (ImGui::InputText("", this->strct_ext_layer.buf_to_icao, 8, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
      {
        this->strct_ext_layer.to_icao = std::string(this->strct_ext_layer.buf_to_icao);
      }
      ImGui::PopID();
      this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Enter Arrival airport ICAO code");

      ImGui::SameLine();
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
      if (ImgWindow::ButtonTooltip(mxUtils::from_u8string(ICON_FA_TRASH_ALT).append( "##ClearToICAO" ).c_str(), "Clear to ICAO"))
      {
        this->strct_ext_layer.to_icao.clear();
        memset(this->strct_ext_layer.buf_to_icao, 0, sizeof this->strct_ext_layer.buf_to_icao);
      }
      this->mxUiReleaseLastFont();

      ImGui::SameLine();
      ImGui::LabelText("##To ICAO Label", "To ICAO");

      ImGui::PopStyleColor();
      ImGui::PopItemWidth();

      // Display restart plugin button if https failed
      if (this->strct_ext_layer.bDisplayPluginsRestart && missionx::data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running)
      {
        const static std::string msg = "Restarting all plugins is a last resort and it might not have the expected effect.\nI suggest to do it:\n1. Only once per session.\n2.You should do it while using LR plane.\n3.If you find the "
                                       "conflicting plugin please notify developer.";
        ImGui::SameLine(0.0f, 20.0);
        missionx::WinImguiBriefer::HelpMarker(msg.c_str());

        int iStyles = 0;
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_red);
        iStyles++;
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_white);
        iStyles++;
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
        iStyles++;
        if (ImGui::Button("!!! Reload All Plugins !!!"))
        {
          this->strct_ext_layer.bDisplayPluginsRestart = false;
          missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::restart_all_plugins);
        }
        this->mx_add_tooltip(missionx::color::color_vec4_yellow, msg);
        ImGui::PopStyleColor(iStyles);
      }
    }

    ImGui::PushItemWidth(520.0f);
    {
      missionx::WinImguiBriefer::HelpMarker("Zero value means any range.\nMax manual range is 9000nm\nIf you receive Error 500, try to limit the range.");
      ImGui::SameLine();
      ImGui::DragFloat("Range (Zero = any range)", &this->strct_ext_layer.ga_range_max_slider_f, ga_steps, ga_range_begin, ga_range_end, "Max Range: %.0f nm (Limit range if you get error 500)");

      missionx::WinImguiBriefer::HelpMarker("How many rows to retrieve. Default 20, Max 100");
      ImGui::SameLine();
      ImGui::Combo("Limit rows", &this->strct_ext_layer.limit_indx, limit_items, IM_ARRAYSIZE(limit_items)); // default is 20 rows

      missionx::WinImguiBriefer::HelpMarker("Sort options. Data is initially sorted by the external site and not sorted locally.");
      ImGui::SameLine();
      ImGui::Combo("Sort By", &this->strct_ext_layer.sort_indx, sort_items, IM_ARRAYSIZE(sort_items)); // default is "created"   { "created", "updated", "popularity", "distance" }
    }
    ImGui::PopItemWidth();

    // v3.303.8.3
    missionx::WinImguiBriefer::HelpMarker("Result should only contain one row per airport, remove duplications based on airport code and name.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Remove duplicate airports", &this->strct_ext_layer.flag_remove_duplicate_airport_names))
    {
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_REMOVE_DUPLICATE_ICAO_ROWS(), this->strct_ext_layer.flag_remove_duplicate_airport_names);
    }
    if (this->strct_ext_layer.flag_remove_duplicate_airport_names)
    {
      ImGui::SameLine(0.0f, 30.0f);
      if (ImGui::Checkbox("Group By Waypoints", &this->strct_ext_layer.flag_group_by_waypoints))
      {
        missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_GROUP_DUPLICATES_BY_WAYPOINTS(), this->strct_ext_layer.flag_group_by_waypoints);
      }
    }
  }
  ImGui::EndChild();
  ImGui::EndGroup();

this->mxUiReleaseLastFont(2); // v3.305.1 title and reg text

  // ------------ Buttons -----------------------

  //  Display button only if we are not processing the fetch
  if ((this->strct_ext_layer.fetch_state != missionx::mxFetchState_enum::fetch_in_process && this->strct_ext_layer.fetch_state != missionx::mxFetchState_enum::fetch_ended && this->strct_ext_layer.fetch_state != missionx::mxFetchState_enum::fetch_guess_wp) &&
      (!this->strct_ext_layer.from_icao.empty() || !this->strct_ext_layer.to_icao.empty()))
  {
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());

    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_orange);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_azure);
    if (ImGui::Button("Fetch Flight Plans"))
    {
      // call fetch information data Try to use mutex and std::async
      IXMLNode node_ptr = missionx::data_manager::prop_userDefinedMission_ui.node;
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int> (_mission_type::cargo));
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_PLANE_TYPE_I(), static_cast<int> (missionx::mx_plane_types::plane_type_ga_floats));
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_NO_OF_LEGS(), 0); // Ignore, set by the external site
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double>(mxconst::get_PROP_MIN_DISTANCE_SLIDER(), 0.0);
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double>(mxconst::get_PROP_MAX_DISTANCE_SLIDER(), (float)this->strct_ext_layer.ga_range_max_slider_f);
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_USE_OSM_CHECKBOX(), false); // not OSM but external site

      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_FROM_ICAO(), std::string(this->strct_ext_layer.buf_from_icao));
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_TO_ICAO(), this->strct_ext_layer.to_icao.c_str());            // v3.0.253.3 // Ignore, set by the external site
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_LIMIT(), limit_items[this->strct_ext_layer.limit_indx]);      // Ignore, set by the external site
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_SORT_FPLN_BY(), sort_items[this->strct_ext_layer.sort_indx]); // Ignore, set by the external site

      this->execAction(missionx::mx_window_actions::ACTION_FETCH_FPLN_FROM_EXTERNAL_SITE);
    }

    ImGui::PopStyleColor(3);

    this->mxUiReleaseLastFont();

    // // v3.303.10
    // ImGui::SameLine(0.0f, 300.0f);
    // this->add_ui_advance_settings_random_date_time_weather_and_weight_button(this->adv_settings_strct.iClockDayOfYearPicked, this->adv_settings_strct.iClockHourPicked, this->adv_settings_strct.iClockMinutesPicked); // v3.303.10 convert the random dateTime button to a self contain function
  }
  else
  {
    ImGui::NewLine();
  }


  // ------------ Flight plan Table -----------------------

  const bool bEnableState = this->mxStartUiDisableState(this->strct_ext_layer.fetch_state != missionx::mxFetchState_enum::fetch_not_started); // v24.03.1 disable table until fetch is ended


  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1

  ImGui::BeginGroup();
  ImGui::BeginChild("draw_external_fpln_layer_02", ImVec2(0.0f, win_size_vec2.y * 0.36f), ImGuiChildFlags_Borders); // consume 1/3 of screen
  {
    // --- Flight plan TABLE
    {

      // Alternating rows slightly grayish
      ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, IM_COL32(0x1a, 0x1a, 0x1a, 0xff));
      static constexpr int COLUMN_NUM = 6;

      if (ImGui::BeginTable("Table", COLUMN_NUM, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit
                            // CONSIDER: ImGuiTableFlags_SizingStretchSame
                            // ImGuiTableFlags_ScrollFreezeTopRow |
                            // ImGuiTableFlags_ScrollFreezeLeftColumn
                            ))
      {
        ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible

        // Set up the columns of the table
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
        {
          ImGui::TableSetupColumn("TO", ImGuiTableColumnFlags_None, 260); // to ICAO + (keyName)
          ImGui::TableSetupColumn("Distance", ImGuiTableColumnFlags_DefaultSort, 80);
          ImGui::TableSetupColumn("WPT", ImGuiTableColumnFlags_None, 40);
          ImGui::TableSetupColumn("Plan ID", ImGuiTableColumnFlags_None, 70);
          ImGui::TableSetupColumn("Popularity", ImGuiTableColumnFlags_None, 90);
          ImGui::TableSetupColumn("Generate", ImGuiTableColumnFlags_NoSort, 70); // will open modal window that will display lat/lon + notes
          ImGui::TableHeadersRow();
        }
        ImGui::PopStyleColor();


        if (this->strct_ext_layer.fetch_state == missionx::mxFetchState_enum::fetch_ended)
        {
          this->execAction(mx_window_actions::ACTION_GUESS_WAYPOINTS);
        }
        else if (!missionx::data_manager::tableExternalFPLN_vec.empty() && missionx::data_manager::s_thread_sync_mutex.locked_by_caller() == false) // v3.0.255.4.3 added special class missionx::mutex that can return a bool value if mutex is being locked
        {
          // Sort the data if and as needed
          ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
          if (sortSpecs && sortSpecs->Specs && sortSpecs->SpecsCount >= 1 && sortSpecs->SpecsDirty && data_manager::tableExternalFPLN_vec.size() > 1) // v24.06.1 added "SpecsDirty" test
          {
            // tableDataListTy
            // We sort only by one column, no multi-column sort yet
            const ImGuiTableColumnSortSpecs& colSpec = *(sortSpecs->Specs);

            // We directly sort the tableList: tableExternalFPLN_vec
            std::
              ranges::
                sort (data_manager::tableExternalFPLN_vec, // lambda function
                      [colSpec](const _ext_internet_fpln_strct& a, const _ext_internet_fpln_strct &b)
                      {
                        int cmp = // less than 0 if a < b
                          colSpec.ColumnIndex == 0   ? a.toICAO_s.compare(b.toICAO_s)
                          : colSpec.ColumnIndex == 1 ? static_cast<int> (a.distnace_d - b.distnace_d)
                          : colSpec.ColumnIndex == 2 ? a.waypoints_i - b.waypoints_i
                          : colSpec.ColumnIndex == 3 ? a.fpln_unique_id - b.fpln_unique_id
                          : colSpec.ColumnIndex == 4 ? a.popularity_i - b.popularity_i
                          //: colSpec.ColumnIndex == 5 ? 0 // v24.03.1 info button
                          // : colSpec.ColumnIndex == 5 ? 0
                                                     : 0;

                        return colSpec.SortDirection == ImGuiSortDirection_Ascending ? cmp < 0 : cmp > 0;
                      });

            sortSpecs->SpecsDirty ^= 1; // v24.06.1 set to false
          }

          // Add rows to the table
          static int picked_fpln_id_i = -1;
          for (auto rowData : data_manager::tableExternalFPLN_vec) // v24.03.1 Replaces loop with automated one
          {
            //auto& td = td;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_mx_bluish);
            if (ImGui::Button(fmt::format("{} ({})", rowData.toICAO_s, rowData.toName_s).c_str()))
            {
              this->callNavData(rowData.toICAO_s, true);
            }
            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", static_cast<float> (rowData.distnace_d));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%i", rowData.waypoints_i);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%i", rowData.fpln_unique_id);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%i", rowData.popularity_i);
            ImGui::TableSetColumnIndex(5);

            // ---- Actions
            if (ImGui::Button(fmt::format("   ...   ###ButtonGen{}", rowData.internal_id).c_str()))
            {
              // display modal window and rest of information
              picked_fpln_id_i = rowData.internal_id; // store picked FPLN id

              // Generate mission from this after showing "are you sure" modal window
              IXMLNode node_ptr = missionx::data_manager::prop_userDefinedMission_ui.node;
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int> (missionx::_mission_type::cargo));   //, node_ptr, node_ptr.getName());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_PLANE_TYPE_I(), static_cast<int> (missionx::mx_plane_types::plane_type_ga)); //, node_ptr, node_ptr.getName());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_NO_OF_LEGS(), rowData.waypoints_i);                              //, node_ptr, node_ptr.getName());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double>(mxconst::get_PROP_MIN_DISTANCE_SLIDER(), 0.0);                                //, node_ptr, node_ptr.getName());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double>(mxconst::get_PROP_MAX_DISTANCE_SLIDER(), rowData.distnace_d);                   //, node_ptr, node_ptr.getName());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_USE_OSM_CHECKBOX(), false);                                   //, node_ptr, node_ptr.getName());

              ImGui::OpenPopup(GENERATE_QUESTION.c_str());
            }

            ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            // ImGui::SetNextWindowSize(ImVec2(620.0f, 380.0f));
            this->draw_popup_generate_mission_based_on_ext_fpln (GENERATE_QUESTION, rowData, picked_fpln_id_i);
          } // end loop over all vector
        }   // end should we display table or not

        // End of table
        ImGui::EndTable();
      } // END ImGui::BeginTable

      ImGui::PopStyleColor();
    } // end draw table if data_manager::tableExternalFPLN_vec is not empty
  }
  ImGui::EndChild();
  ImGui::EndGroup();

  this->mxUiReleaseLastFont();


  this->mxEndUiDisableState(bEnableState); // v24.03.1 disable table until fetch is ended

  if (data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running && this->flag_generatedRandomFile_success && this->selectedTemplateKey.empty() && !missionx::data_manager::flag_generate_engine_is_running /* make sure that thread is not running */) //
  {
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
    this->add_ui_start_mission_button(missionx::mx_window_actions::ACTION_START_RANDOM_MISSION);
    this->mxUiReleaseLastFont();
  }

  ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);
}

// -----------------------------------------------
// -----------------------------------------------

void
WinImguiBriefer::draw_ils_screen()
{
  // constexpr const static float CHILD_SIZE_MODIFIER_F = 0.15f;

  auto win_size_vec2 = this->mxUiGetWindowContentWxH();
  ImGuiID elevVerticalSlider_gid = 0;

  ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);

  if (this->strct_ils_layer.layer_state == missionx::mx_layer_state_enum::success_can_draw) // display the success screen - main search screen
  {
    // auto win_size_vec2 = ImGui::GetWindowSize(); // v3.305.1 removed

    missionx::WinImguiBriefer::HelpMarker("The NAV information search screen allows you to search for airports with ILS approaches.\nYou can filter which types of airports you are looking for or\nLet the plugin randomize the filtering for you.\n\nIt will not "
                     "generate your FMS nor fetch the ILS Plates for you, this will be up to you.\n");
    ImGui::SameLine();
    ImGui::TextUnformatted("NAV information depends on the data collected from X-Plane and Custom Sceneries.");

     this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1
     ImGui::BeginGroup();
     ImGui::BeginChild("##NavDataMainTabChild", ImVec2(-5.0f, -35.0f));
     {
      if (ImGui::BeginTabBar("NavDataMainTab", ImGuiTabBarFlags_None))
      {
        if (ImGui::BeginTabItem("ILS Search"))
        {
          this->child_draw_ils_search();

          ImGui::EndTabItem();
        }

        ImGuiTabItemFlags tabFlags = (!this->strct_ils_layer.flagForceNavDataTab) ? ImGuiTabItemFlags_None :ImGuiTabItemFlags_SetSelected; // v24.03.1
        if (ImGui::BeginTabItem("Nav Information", nullptr,tabFlags))
        {
          this->child_draw_nav_search();

          this->strct_ils_layer.flagForceNavDataTab = false; // reset state

          ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
      } // end Main Nav Tab

     }  // End Child
     ImGui::EndChild();
     ImGui::EndGroup();
     this->mxUiReleaseLastFont(); // v3.305.1


  }
  // Display failure message
  else if (this->strct_ils_layer.layer_state == missionx::mx_layer_state_enum::failed_data_is_not_present || this->strct_ils_layer.layer_state == missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly)
  {
    ImGui::NewLine();

    if (this->strct_ils_layer.layer_state == missionx::mx_layer_state_enum::failed_data_is_not_present)
    {
      this->display_shared_message_when_optimized_data_is_not_present(missionx::mx_layer_state_enum::failed_data_is_not_present);
    }
    else
    {
      this->display_shared_message_when_optimized_data_is_not_present(missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly);
    }
  }
  else // display wait
  {
    ImGui::NewLine();

    // ImGui::SetWindowFontScale(2.0f);
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_BIG());
    ImGui::TextColored(missionx::color::color_vec4_magenta, "Please wait while plugin tests the validity of the data.... ");
    this->mxUiReleaseLastFont();
    ImGui::SetWindowFontScale(mxconst::DEFAULT_BASE_FONT_SCALE);

    if (this->strct_ils_layer.layer_state < missionx::mx_layer_state_enum::validating_data)
    {
      this->strct_ils_layer.layer_state = missionx::mx_layer_state_enum::validating_data;
    }
  }

  ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);
}


// -----------------------------------------------


void
WinImguiBriefer::child_draw_ils_search ()
{

  constexpr static auto        elevVerticalTreeNodeName = "Elev. slider";
  constexpr const static float CHILD_SIZE_MODIFIER_F    = 0.15f;

  auto win_size_vec2 = this->mxUiGetWindowContentWxH ();

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1
  ImGui::BeginGroup (); // group 1
  // ImGui::BeginChild ("draw_ils_layer_01", ImVec2 (0.0f, win_size_vec2.y * 0.45f), ImGuiChildFlags_Borders); // consume 1/3 of screen
  auto       uiUpperChildInfo     = ImGui::GetCurrentWindow ();
  const auto uiUpperChildSizeVec2 = uiUpperChildInfo->Size;
  {
    const auto child_vec2 = ImVec2(this->mxUiGetWindowContentWxH ().x * 0.49f, this->mxUiGetWindowContentWxH ().y * 0.44f);

    ImGui::BeginGroup ();
    ImGui::BeginChild ("Left ILS", child_vec2, ImGuiChildFlags_Borders );
    {
      // From/To ICAO tree
      if (ImGui::TreeNode (reinterpret_cast<void *> (static_cast<intptr_t> (1)), "%s", fmt::format ("From/To: {}/{}", this->strct_ils_layer.from_icao, this->strct_ils_layer.to_icao).c_str ()))
      {
        missionx::WinImguiBriefer::HelpMarker ("Enter optional starting ICAO airport.");
        ImGui::SameLine ();
        ImGui::PushItemWidth (100.0f);
        if (ImGui::InputText ("##From_ILS_Icao_text", this->strct_ils_layer.buf1, 8, ImGuiInputTextFlags_CharsUppercase)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
        {
          this->strct_ils_layer.from_icao = std::string (this->strct_ils_layer.buf1);
        }
        this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Optional: enter departure airport ICAO code");

        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG());
        {
          ImGui::SameLine ();
          if (ImgWindow::ButtonTooltip (mxUtils::from_u8string (ICON_FA_TRASH_ALT).c_str (), "Clear##ClearFromICAO")) // should have been ImGui::ButtonTooltip
          {
            this->strct_ils_layer.from_icao.clear ();
            memset (this->strct_ils_layer.buf1, 0, sizeof this->strct_ils_layer.buf1);
          }
        }
        this->mxUiReleaseLastFont ();

        ImGui::SameLine ();
        if (ImGui::Button ("From ICAO") || this->strct_ils_layer.bFirstTime) // first time initialization or manual ICAO fetch
        {
          #ifdef IBM
          this->strct_ils_layer.navaid = data_manager::getPlaneAirportOrNearestICAO ();
          #else
          auto tempNav                 = data_manager::getPlaneAirportOrNearestICAO ();
          this->strct_ils_layer.navaid = tempNav;
          #endif
          if (!this->strct_ils_layer.navaid.getID ().empty ())
            std::memcpy (this->strct_ils_layer.buf1, this->strct_ils_layer.navaid.ID, 10);

          this->strct_ils_layer.from_icao  = std::string (this->strct_ils_layer.buf1); // first initialization
          this->strct_ils_layer.sNavICAO   = this->strct_ils_layer.from_icao; // v24.03.2 NavData ICAO will also be initialized.
          this->strct_ils_layer.bFirstTime = false;
        }

        //////////////////
        // New Line
        // TO input item
        /////////////////
        missionx::WinImguiBriefer::HelpMarker ("Enter optional destination ICAO airport.\nThe plugin will search for all ICAOs containing the string you entered.");
        ImGui::SameLine ();
        ImGui::PushItemWidth (100.0f);
        if (ImGui::InputText ("##To_ILS_Icao_text", this->strct_ils_layer.buf2, 8, ImGuiInputTextFlags_CharsUppercase)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
        {
          this->strct_ils_layer.to_icao = mxUtils::trim (std::string (this->strct_ils_layer.buf2));
        }
        this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Optional: enter arrival airport ICAO code");
        ImGui::SameLine ();
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG());
        {
          ImGui::SameLine ();
          if (ImgWindow::ButtonTooltip (mxUtils::from_u8string (ICON_FA_TRASH_ALT).c_str (), "Clear##ClearToICAO")) // should have been ImGui::ButtonTooltip
          {
            this->strct_ils_layer.to_icao.clear ();
            memset (this->strct_ils_layer.buf2, 0, sizeof this->strct_ils_layer.buf2);
          }
        }
        this->mxUiReleaseLastFont ();
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_white, "To ICAO");

        if (this->strct_ils_layer.to_icao.length () < 2) // v3.24.1 We can ignore distance only
          this->strct_ils_layer.flagIgnoreDistanceFilter = false;

        ImGui::SameLine (0.0f, 5.0f);
        missionx::WinImguiBriefer::HelpMarker ("You must enter Two or more search characters to enable the option to ignore 'precise distance' filter.\nThe search result will be limited to 250 rows.");

        const bool bICAO_isEmpty = this->mxStartUiDisableState (this->strct_ils_layer.to_icao.empty ()); // v24.03.1 disable line ?
        ImGui::SameLine ();
        ImGui::Checkbox ("Ignore Dist.", &this->strct_ils_layer.flagIgnoreDistanceFilter);
        this->mxEndUiDisableState (bICAO_isEmpty); // v24.03.1 disable line ?


        ImGui::TreePop ();
      } // END FROM/TO Tree

      // Max Distance
      {
        const bool bIgnoreDistanceFilter = this->mxStartUiDisableState (this->strct_ils_layer.flagIgnoreDistanceFilter); // v24.03.1 disable line ?

        ImGui::TextColored (missionx::color::color_vec4_yellow, "Pick Maximum Flight Leg Distance");
        ImGui::PushID ("##Slider_ILS_MaxDistance");
        {
          if (ImGui::SliderFloat ("", &strct_ils_layer.ils_sliderVal2, mxconst::SLIDER_SHORTEST_MAX_ILS_SEARCH_RADIUS, mxconst::SLIDER_ILS_MAX_SEARCH_RADIUS, "%.0f nm"))
          {
            // calc and construct low/high label for slider
            if (strct_ils_layer.ils_sliderVal2 / 500.0f > 1.0f)
              strct_ils_layer.ils_sliderVal1 = strct_ils_layer.ils_sliderVal2 * 0.75f;
            else if (strct_ils_layer.ils_sliderVal2 / 250.0f > 1.0f)
              strct_ils_layer.ils_sliderVal1 = strct_ils_layer.ils_sliderVal2 * 0.5f;
            else
              strct_ils_layer.ils_sliderVal1 = mxconst::SLIDER_SHORTEST_MIN_ILS_SEARCH_RADIUS;

            strct_ils_layer.ils_slider2_lbl = "[" + Utils::formatNumber<float> (strct_ils_layer.ils_sliderVal1, 0) + ".." + Utils::formatNumber<float> (strct_ils_layer.ils_sliderVal2, 0) + "]";
          }
        }
        ImGui::PopID ();
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", strct_ils_layer.ils_slider2_lbl.c_str ());

        this->mxEndUiDisableState (bIgnoreDistanceFilter); // disable line ?

      } // end Max Distance

      // Minimum Runway Length && Minimum Runway Width mt.
      {
        // ImGui::NewLine();
        ImGui::Spacing (); // v3.305.1

        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
        ImGui::SliderInt ("Min Runway Length", &strct_ils_layer.slider_min_rw_length_i, mxconst::SLIDER_ILS_SHORTEST_RW_LENGTH_MT, mxconst::SLIDER_ILS_LOGEST_RW_LENGTH_MT, "%i meters");
        ImGui::PopStyleColor (1);
        this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Pick minimal runway Length filter");

        // ImGui::NewLine();
        ImGui::Spacing (); // v3.305.1

        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
        ImGui::SliderInt ("Min Runway Width", &strct_ils_layer.slider_min_rw_width_i, mxconst::SLIDER_ILS_SHORTEST_RW_WIDTH_MT, mxconst::SLIDER_ILS_WIDEST_RW_WIDTH_MT, "%i meters");
        ImGui::PopStyleColor (1);
        this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Pick minimal runway Width filter");
      } //  Minimum Runway Length && Minimum Runway Width mt.

      // Limit Rows
      {
        ImGui::SetNextItemWidth (70.0f);
        ImGui::Combo ("Limit rows", &this->strct_ils_layer.limit_indx, this->strct_ils_layer.limit_items, IM_ARRAYSIZE (this->strct_ils_layer.limit_items)); // default is 250 rows
        ImGui::SameLine ();
        missionx::WinImguiBriefer::mxUiHelpMarker (missionx::color::color_vec4_aquamarine, fmt::format ("How many rows to retrieve. Default {}.\nCan drastically affect FPS.", this->strct_ils_layer.limit_items[0]).c_str ());
      } // limit rows

    }
    ImGui::EndChild (); // end left ILS child
    ImGui::EndGroup (); // end left group

    ImGui::SameLine (this->mxUiGetWindowContentWxH().x * 0.5f);

    ImGui::BeginGroup ();
    ImGui::BeginChild ("Right ILS", child_vec2, ImGuiChildFlags_Borders );
    {
      // Which ILS types to search
      {
        const std::string ils_type_picked_s = strct_ils_layer.get_ils_types_picked ();
        const std::string picked_lbl_s      = ((ils_type_picked_s.empty ()) ? "Any NAV type" : ils_type_picked_s);

        ImGui::TextColored (missionx::color::color_vec4_yellow, "Type:");
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_white, "%s", picked_lbl_s.c_str ());

        {
          if (ImGui::TreeNode (reinterpret_cast<void *> (static_cast<intptr_t> (2)), "%s", "NAV Filtering"))
          {
            ImGui::NewLine ();

            // loop over all ils in mapCheck_ILS_types and display state
            int counter = 0;
            for (auto &[keyType, bVal] : this->strct_ils_layer.mapCheck_ILS_types)
            {
              counter++;
              if (counter % 4 == 0)
                ImGui::NewLine ();

              ImGui::SameLine (); // we always need same line. New line is special case

              ImGui::Checkbox (missionx::mapILS_types[keyType].c_str (), &bVal); // no need to handle picked checkbox, since we are handling it prior to tree display
            }
            ImGui::Separator ();
            ImGui::TreePop ();
          }
        }
      } // end ILS types to search

      // Minimum airport elevation
    {
      const std::string min_ap_elev_ft_s = "Min Airport Elevation:";

      ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", min_ap_elev_ft_s.c_str ());
      ImGui::SameLine ();
      ImGui::TextColored (missionx::color::color_vec4_white, "%s", (Utils::formatNumber<int> (this->strct_ils_layer.slider_min_airport_elev_ft_i) + "ft").c_str ());

      ImGui::SameLine (0.0f, 10.0f);

      if (ImGui::TreeNode (reinterpret_cast<void *> (static_cast<intptr_t> (3)), "%s", "Elev. slider"))
      {
        if (this->strct_ils_layer.enum_elevSliderOpenState == missionx::enums::mx_treeNodeState::closed)
          this->strct_ils_layer.enum_elevSliderOpenState = missionx::enums::mx_treeNodeState::opened;

        static constexpr float vertical_slider_spacing_f = 180.0f;
        static constexpr float vertical_slider_width_f   = 80.0f;
        ImGui::NewLine ();
        ImGui::SameLine (0.0f, vertical_slider_spacing_f);
        ImGui::VSliderInt ("##airportElevSliderInt", ImVec2 (vertical_slider_width_f, 80.0f), &this->strct_ils_layer.slider_min_airport_elev_ft_i, mxconst::SLIDER_ILS_LOWEST_AIRPORT_ELEV_FT, mxconst::SLIDER_ILS_HIGHEST_AIRPORT_ELEV_FT, "%i");

        ImGui::NewLine ();
        ImGui::SameLine (0.0f, vertical_slider_spacing_f);
        if (ImGui::Button ("Reset", ImVec2 (vertical_slider_width_f, 30.0f)))
        {
          this->strct_ils_layer.slider_min_airport_elev_ft_i = mxconst::SLIDER_ILS_STARTING_AIRPORT_ELEV_VALUE_FT;
          ImGui::SetScrollHereY (1.0f);
        }

        if (this->strct_ils_layer.enum_elevSliderOpenState == missionx::enums::mx_treeNodeState::opened)
        {
          ImGui::SetScrollHereY (1.0f);
          this->strct_ils_layer.enum_elevSliderOpenState = missionx::enums::mx_treeNodeState::was_opened;
        }


        ImGui::TreePop ();
      }
      else
        this->strct_ils_layer.enum_elevSliderOpenState = missionx::enums::mx_treeNodeState::closed; // when close we must reset it

    } // Minimum airport elevation



    }
    ImGui::EndChild ();
    ImGui::EndGroup ();
  }

  // ImGui::EndChild (); // end outer child
  ImGui::EndGroup ();

  this->mxUiReleaseLastFont (); // v3.305.1

  //------------------------------------------------
  //     search ILS runways button
  //------------------------------------------------
  ImGui::NewLine ();
  ImGui::BeginGroup ();
  {
    if (missionx::data_manager::flag_generate_engine_is_running && this->sBottomMessage.empty ())
    {
      this->setMessage ("Random Engine is running, please wait...");
    }
    else if (missionx::data_manager::flag_apt_dat_optimization_is_running && sBottomMessage.empty ())
    {
      this->setMessage ("Can't Generate mission, apt dat optimization is currently running. Please wait for it to finish first !!!");
    }

    const static std::string lbl = "Search for ILS airports based on user pref.";
    ImGui::SameLine (0.0f, 5.0f);

    int style_i = 0;
    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
    style_i++;
    ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_indigo);
    style_i++;

    this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG());
    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_orange);
    ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_azure);
    if (ImGui::Button (lbl.c_str ()))
    {

      // select icao, round(distance_nm) as distance_nm, loc_rw, loc_type, frq_mhz, loc_bearing, rw_length_mt, rw_width, ap_elev, ap_name, surf_type_text, bearing_from_to_icao
      // from (
      // select xp_loc.icao
      //       , mx_calc_distance ({}, {}, xp_loc.lat, xp_loc.lon, 3440) as distance_nm
      //       , xp_loc.loc_rw, xp_loc.loc_type, xp_loc.frq_mhz, xp_loc.loc_bearing, xp_rw.rw_length_mt, xp_rw.rw_width, xa.ap_elev, xa.ap_name
      //       , case xp_rw.rw_surf when 1 then 'Asphalt' when 2 then 'Concrete' when 3 then 'Turf or grass' when 4 then 'Dirt' when 5 then 'Gravel' when 12 then 'Dry lakebed' when 13 then 'Water runways' when 14 then 'Snow or ice' when 15 then 'Transparent' else 'other' end as surf_type_text
      //       , mx_bearing({}, {}, xp_loc.lat, xp_loc.lon) as bearing_from_to_icao
      // from xp_loc, xp_rw, xp_airports xa
      // where xp_rw.icao = xp_loc.icao
      // and (xp_rw.rw_no_1 = xp_loc.loc_rw or xp_rw.rw_no_2 = xp_loc.loc_rw)
      // and xa.icao = xp_rw.icao
      //)
      // where 1 = 1


      // v24.03.1 The search code is split into two parts, the filter is constructed from the UI and the base query is provided in the "data_manager::fetch_ils_rw_from_sqlite()" function.
      std::string sql_filter = " and rw_length_mt >= " + mxUtils::formatNumber<int> (strct_ils_layer.slider_min_rw_length_i); // " and rw_length_mt >= 1000 "
      sql_filter += " and rw_width >= " + mxUtils::formatNumber<int> (strct_ils_layer.slider_min_rw_width_i); // " and rw_width >= 45 "
      sql_filter += " and ap_elev >= " + mxUtils::formatNumber<int> (strct_ils_layer.slider_min_airport_elev_ft_i); // " and ap_elev >= 0 "
      if (!this->strct_ils_layer.to_icao.empty ()) // v24.03.1 add the TO
        sql_filter += fmt::format (" and icao like '%{}%' ", this->strct_ils_layer.to_icao);


      if (this->strct_ils_layer.get_ils_types_picked ().empty ())
        sql_filter += " ";
      else
        sql_filter += " and lower(loc_type) in ( " + this->strct_ils_layer.get_ils_types_picked () + " )"; //

      if (!this->strct_ils_layer.flagIgnoreDistanceFilter) // v24.03.1
        sql_filter += " and distance_nm between " + mxUtils::formatNumber<float> (strct_ils_layer.ils_sliderVal1, 0) + " and " + mxUtils::formatNumber<float> (strct_ils_layer.ils_sliderVal2, 0); // " and distance between 50 and 100 "

      sql_filter += fmt::format (" LIMIT {} ", missionx::WinImguiBriefer::mx_ils_layer::limit_items[this->strct_ils_layer.limit_indx]); // v24.03.1 We always limit rows, for UI performance reasons


      this->strct_ils_layer.filter_query_s = sql_filter; // v24.03.1 store the final filter for the thread use.

      if (this->strct_ils_layer.from_icao.empty ())
      {
        #ifdef IBM
        this->strct_ils_layer.navaid = data_manager::getPlaneAirportOrNearestICAO ();
        #else
        auto tempNav                 = data_manager::getPlaneAirportOrNearestICAO ();
        this->strct_ils_layer.navaid = tempNav;
        #endif
      }

      if ((!this->strct_ils_layer.from_icao.empty ()) && (this->strct_ils_layer.navaid.getID ().empty () || this->strct_ils_layer.navaid.lat == 0 || this->strct_ils_layer.navaid.lon == 0))
      {
        #ifdef IBM
        this->strct_ils_layer.navaid = data_manager::getICAO_info (this->strct_ils_layer.from_icao);
        #else
        auto tempNav                 = data_manager::getICAO_info (this->strct_ils_layer.from_icao);
        this->strct_ils_layer.navaid = tempNav;
        #endif
      }

      // last validation
      if (this->strct_ils_layer.navaid.getID ().empty ()) // if navaid ID is still empty then pick plane position
      {
        this->setMessage ("Could not initialize starting ICAO. Please consider entering it manually.");
      }
      else
      {
        this->strct_ils_layer.fetch_ils_state  = missionx::mxFetchState_enum::fetch_not_started;
        this->flag_generatedRandomFile_success = false; // reset state if already generated information. Will hide Start button until next mission generated.

        this->execAction (missionx::mx_window_actions::ACTION_FETCH_ILS_AIRPORTS);
      }
    }
    ImGui::PopStyleColor (3);

    // Display START mission button
    if (data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running && this->flag_generatedRandomFile_success && this->selectedTemplateKey.empty () && !missionx::data_manager::flag_generate_engine_is_running /* make sure that thread is not running */) //
    {
      ImGui::SameLine (win_size_vec2.x * 0.5f);
      // this->mxUiSetFont (TEXT_TYPE_TITLE_REG);
      this->add_ui_start_mission_button (missionx::mx_window_actions::ACTION_START_RANDOM_MISSION);
      // this->mxUiReleaseLastFont ();
    }

    this->mxUiReleaseLastFont ();

    ImGui::PopStyleColor (style_i);

    // // v3.303.10
    // ImGui::SameLine(0.0f, 120.0f);
    // this->add_ui_advance_settings_random_date_time_weather_and_weight_button(this->adv_settings_strct.iClockDayOfYearPicked, this->adv_settings_strct.iClockHourPicked, this->adv_settings_strct.iClockMinutesPicked); // v3.303.10 convert the random dateTime button to a self contain function
  }
  ImGui::EndGroup ();

  //------------------------------------------------
  //     Airports Query Result Table
  //------------------------------------------------

  const float fStartButtonHeight = (data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running && this->flag_generatedRandomFile_success && this->selectedTemplateKey.empty () && !missionx::data_manager::flag_generate_engine_is_running) ? 25.0f : 0.0f;

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1

  ImGui::BeginGroup ();
  // ImGui::BeginChild ("draw_ils_layer_table_02", ImVec2 (0.0f, win_size_vec2.y - uiUpperChildSizeVec2.y - 0.0f /*buttons*/ - 10.0f /*bottom message space*/ - fStartButtonHeight /* Is StartButton visible */), ImGuiChildFlags_Borders); // Size relative to the upper child size
  {
    ImGui::PushStyleColor (ImGuiCol_TableRowBgAlt, IM_COL32 (0x1a, 0x1a, 0x1a, 0xff));
    constexpr const static int COLUMN_NUM = 11;

    if (ImGui::BeginTable ("TableILS", COLUMN_NUM, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit
                           // ImGuiTableFlags_ScrollFreezeTopRow | ImGuiTableFlags_ScrollFreezeLeftColumn
                           ))
    {
      ImGui::TableSetupScrollFreeze (0, 1); // Make top row always visible

      // Set up the columns of the table
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
      {
        ImGui::TableSetupColumn ("TO", ImGuiTableColumnFlags_None, 210); // to ICAO + (keyName)
        ImGui::TableSetupColumn ("Dist.", ImGuiTableColumnFlags_DefaultSort, 45);
        ImGui::TableSetupColumn ("ILS Type", ImGuiTableColumnFlags_None, 70);
        ImGui::TableSetupColumn ("Freq.", ImGuiTableColumnFlags_None, 50);
        ImGui::TableSetupColumn ("RW", ImGuiTableColumnFlags_None, 30);
        ImGui::TableSetupColumn ("Len mt", ImGuiTableColumnFlags_None, 55);
        ImGui::TableSetupColumn ("Width", ImGuiTableColumnFlags_None, 50);
        ImGui::TableSetupColumn ("Elev ft.", ImGuiTableColumnFlags_None, 65);
        // ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_NoSort, 50);    //
        ImGui::TableSetupColumn ("Gen.", ImGuiTableColumnFlags_NoSort, 50); //
        ImGui::TableSetupColumn ("Surface", ImGuiTableColumnFlags_NoSort, 90); // v3.0.253.13
        ImGui::TableSetupColumn ("Bearing", ImGuiTableColumnFlags_NoSort, 50); // v3.0.253.13 bearing between start and target icao runway
        ImGui::TableHeadersRow ();
      }
      ImGui::PopStyleColor ();

      if (!missionx::data_manager::table_ILS_rows_vec.empty () && this->strct_ils_layer.fetch_ils_state != missionx::mxFetchState_enum::fetch_in_process && missionx::data_manager::s_thread_sync_mutex.locked_by_caller () == false)
      {
        // Sort the data if and as needed
        ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs ();
        if (sortSpecs && sortSpecs->SpecsDirty && sortSpecs->Specs && sortSpecs->SpecsCount >= 1 && data_manager::table_ILS_rows_vec.size () > 1)
        {
          // tableDataListTy
          // We sort only by one column, no multi-column sort
          const ImGuiTableColumnSortSpecs &colSpec = *(sortSpecs->Specs);

          // We directly sort the tableList: tableExternalFPLN_vec
          std::ranges::sort (data_manager::table_ILS_rows_vec, // lambda function
                             [colSpec] (const missionx::mx_ils_airport_row_strct &a, const missionx::mx_ils_airport_row_strct &b)
                             {
                               const int cmp = // less than 0 if a < b
                                 colSpec.ColumnIndex == 0   ? a.toICAO_s.compare (b.toICAO_s)
                                 : colSpec.ColumnIndex == 1 ? static_cast<int> (a.distnace_d - b.distnace_d)
                                 : colSpec.ColumnIndex == 2 ? a.locType_s.compare (b.locType_s)
                                 : colSpec.ColumnIndex == 3 ? 0
                                                            : // no sorting
                                   colSpec.ColumnIndex == 4 ? a.loc_rw_s.compare (b.loc_rw_s)
                                 : colSpec.ColumnIndex == 5 ? a.rw_length_mt_i - b.rw_length_mt_i
                                 : colSpec.ColumnIndex == 6 ? static_cast<int> (a.rw_width_d - b.rw_width_d)
                                                            : // width of rw
                                   colSpec.ColumnIndex == 7 ? (int)(a.ap_elev_ft_i - b.ap_elev_ft_i)
                                                            : // elevation ft.
                                   // colSpec.ColumnIndex == 8 ? 0 // v24.03.1 info button - replaced localizer bearing
                                   colSpec.ColumnIndex == 8 ? 0
                                                            : // GEN button - don't sort
                                   colSpec.ColumnIndex == 9 ? 0
                                                            : // Surface - don't sort surface type v3.0.253.13
                                   colSpec.ColumnIndex == 10 ? static_cast<int> (a.bearing_from_to_icao_d - b.bearing_from_to_icao_d)
                                                             : // v3.0.253.13
                                   0; // last option should have : 0 at the end like else

                               return colSpec.SortDirection == ImGuiSortDirection_Ascending ? cmp < 0 : cmp > 0;
                             });

          sortSpecs->SpecsDirty = false;
        } // end Sorting logic

        // Add rows to the table
        static int picked_fpln_id_i = -1;
        for (const auto &rowData : data_manager::table_ILS_rows_vec)
        {
          int i = 0;
          // auto& td = *td;
          ImGui::TableNextRow ();
          ImGui::TableSetColumnIndex (i);
          i++;

          ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_mx_bluish);
          if (ImGui::Button (fmt::format ("{} ({}){}", rowData.toICAO_s, ((rowData.toName_s.length () > 23) ? rowData.toName_s.substr (0, 20) + "..." : rowData.toName_s), fmt::format ("##ButtonInfo{}", rowData.seq)).c_str ()))
          {
            this->callNavData (rowData.toICAO_s, false);
          }
          ImGui::PopStyleColor ();

          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::Text ("%.0f", static_cast<float> (rowData.distnace_d));
          this->mx_add_tooltip (missionx::color::color_vec4_beige, "Distance in nautical miles.");
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::TextUnformatted (rowData.locType_s.c_str ()); // Type
          ImGui::TableSetColumnIndex (i);
          i++;
          const std::string sLocTypesToFormat = "VORDMENDBILS-CAT-IILS-CAT-IIIILS-CAT-IILOC";
          const std::string locTypeUpperCase  = mxUtils::stringToUpper (rowData.locType_s);
          const bool        bFormatFrq        = (sLocTypesToFormat.find (locTypeUpperCase) != std::string::npos);
          std::string       frq_s             = mxUtils::formatNumber<int> (rowData.loc_frq_mhz);
          frq_s                               = (bFormatFrq) ? frq_s.insert (3, 1, '.') : frq_s;
          ImGui::TextUnformatted (frq_s.c_str ());
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::TextUnformatted (rowData.loc_rw_s.c_str ()); // on which rw
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::Text ("%i", rowData.rw_length_mt_i);
          this->mx_add_tooltip (missionx::color::color_vec4_beige, "Runway length in meters.");
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::Text ("%.2f", static_cast<float> (rowData.rw_width_d));
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::Text ("%i", rowData.ap_elev_ft_i); // elevation ft.

          ImGui::TableSetColumnIndex (i);
          i++;

          // Last field, the creation mission
          // ---- Actions
          if (ImGui::Button (fmt::format (" ... ###ButtonGen{}", rowData.seq).c_str ())) // v24.03.1 replaced buff with fmt::format
          {
            // display modal window and rest of information
            picked_fpln_id_i = rowData.seq; // store picked seq

            // Generate mission from this after showing "are you sure" modal window
            IXMLNode node_ptr = missionx::data_manager::prop_userDefinedMission_ui.node;
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int> (mx_mission_type::cargo)); //, node_ptr, node_ptr.getName()); // always cargo
            // missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>(mxconst::get_PROP_PLANE_TYPE_I(), static_cast<int> (strct_ils_layer.iRadioPlaneType));         //, node_ptr, node_ptr.getName());
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_NO_OF_LEGS(), 0); //, node_ptr, node_ptr.getName()); // legs will be dectated by RandomEngine. Should only be 1 and simmer will add the rest
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double> (mxconst::get_PROP_MIN_DISTANCE_SLIDER(), (double)strct_ils_layer.ils_sliderVal1); //, node_ptr, node_ptr.getName());
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double> (mxconst::get_PROP_MAX_DISTANCE_SLIDER(), (double)strct_ils_layer.ils_sliderVal2); //, node_ptr, node_ptr.getName());
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_USE_OSM_CHECKBOX(), false); //, node_ptr, node_ptr.getName());     // always false
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_USE_WEB_OSM_CHECKBOX(), false); //, node_ptr, node_ptr.getName()); // always false

            missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_FROM_ICAO(), strct_ils_layer.navaid.getID ()); //, node_ptr, node_ptr.getName()); //
            missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_TO_ICAO(), rowData.toICAO_s); //, node_ptr, node_ptr.getName());                 //

            ImGui::OpenPopup (GENERATE_ILS_QUESTION.c_str ());
          }

          // v3.0.253.13 Other information about runway/airport
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::TextUnformatted (rowData.surfType_s.c_str ()); // Surface Type
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::Text ("%.0f", static_cast<float> (rowData.bearing_from_to_icao_d)); // bearing between start and target icao
          this->mx_add_tooltip (missionx::color::color_vec4_beige, "Bearing to airport relative to plane position.");


          // DISPLAY POPUP
          ImVec2 center (ImGui::GetIO ().DisplaySize.x * 0.5f, ImGui::GetIO ().DisplaySize.y * 0.5f); // center of screen
          ImGui::SetNextWindowPos (center, ImGuiCond_Appearing, ImVec2 (0.5f, 0.5f));
          ImGui::SetNextWindowSize (ImVec2 (480.0f, 415.0f));

          ImGui::PushStyleColor (ImGuiCol_PopupBg, missionx::color::color_vec4_black);
          {
            if (ImGui::BeginPopupModal (GENERATE_ILS_QUESTION.c_str (), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
              ImVec2 modal_center (mxUiGetContentWidth () * 0.5f, ImGui::GetWindowHeight () * 0.5f);
              if (rowData.seq == picked_fpln_id_i)
              {
                this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG());
                ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", "To: ");
                this->mxUiReleaseLastFont ();

                ImGui::SameLine (0.0f, 1.0f); // one space
                ImGui::TextColored (missionx::color::color_vec4_greenyellow, "%s", (rowData.toICAO_s + " - " + rowData.toName_s.substr (0, 30)).c_str ());

                // Pick Plane Type
                ImGui::NewLine ();
                // label
                ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
                ImGui::Text ("Pick Preferred Plane:");
                ImGui::PopStyleColor (1);
                ImGui::NewLine ();

                for (const auto &planeTypeLabel : this->mapListPlaneRadioLabel | std::views::values) // v24.12.1
                {
                  if (planeTypeLabel.type == mx_plane_types::plane_type_helos || planeTypeLabel.type == mx_plane_types::plane_type_ga_floats)
                    continue;

                  ImGui::SameLine ();
                  if (ImGui::RadioButton (planeTypeLabel.label.c_str (), this->strct_ils_layer.iRadioPlaneType == planeTypeLabel.type))
                  {
                    this->strct_ils_layer.iRadioPlaneType = planeTypeLabel.type;
                  }
                } // end loop over all plane types

                ImGui::NewLine ();
                ImGui::Checkbox ("Start from plane position", &this->strct_cross_layer_properties.flag_start_from_plane_position); // v3.0.253.11 // v3.0.253.12 reposition checkbox in the popup generate window
                ImGui::Spacing ();
                ImGui::Checkbox ("Generate GPS waypoints.", &this->strct_cross_layer_properties.flag_generate_gps_waypoints); // v3.0.253.12
                this->add_ui_auto_load_checkbox (); // v25.04.2
                ImGui::Spacing (); // v3.303.14.2 added default weight to the generate screen
                ImGui::Checkbox ("Add default base weights.\n(Not advisable for planes > GAs)", &this->adv_settings_strct.flag_add_default_weight_settings);
                ImGui::Spacing ();

                this->add_ui_pick_subcategories (this->mapMissionCategories[static_cast<int> (missionx::mx_mission_type::cargo)]);
                ImGui::Spacing ();
                // v3.303.10 // v25.04.1 moved advance button to the popup window for better flow
                this->add_ui_advance_settings_random_date_time_weather_and_weight_button2 (this->adv_settings_strct.iClockDayOfYearPicked, this->adv_settings_strct.iClockHourPicked, this->adv_settings_strct.iClockMinutesPicked); // v3.303.10 convert the random dateTime button to a self contain function
                ImGui::Spacing ();
                add_designer_mode_checkbox (); // v24.03.2 Designer mode flag

                ImGui::NewLine ();
                ImGui::Separator ();
                ImGui::NewLine ();
                ImGui::NewLine ();
                ImGui::SameLine (modal_center.x * 0.2f);
                ImGui::SetItemDefaultFocus ();

                // v3.303.10
                static bool bRerunRandomDateTime{ false };

                // display the option only if we are not in the middle of a running mission
                if (missionx::data_manager::missionState != missionx::mx_mission_state_enum::mission_is_running)
                {

                  bRerunRandomDateTime = add_ui_checkbox_rerun_random_date_and_time ();
                  ImGui::SameLine (0.0f, 5.0f);

                  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG());
                }

                if (missionx::data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
                {
                  ImGui::TextColored (missionx::color::color_vec4_aqua, "%s", "Can't generate at this time.");
                }
                else if (ImGui::Button (">> Generate <<", ImVec2 (120, 0)))
                {
                  if (bRerunRandomDateTime) // v3.303.10
                    this->execAction (missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);

                  // Prepare and call ACTION_GENERATE_RANDOM_MISSION
                  data_manager::prop_userDefinedMission_ui.setNodeProperty<int>            (mxconst::get_PROP_FPLN_ID_PICKED(), picked_fpln_id_i);
                  missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int>  (mxconst::get_PROP_PLANE_TYPE_I(), static_cast<int> (strct_ils_layer.iRadioPlaneType)); // v25.04.1 moved into popup
                  missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_START_FROM_PLANE_POSITION(), this->strct_cross_layer_properties.flag_start_from_plane_position); // v3.0.253.11 start from plane position
                  missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_GENERATE_GPS_WAYPOINTS(), this->strct_cross_layer_properties.flag_generate_gps_waypoints); // v3.0.253.12 generate GPS waypoints


                  if (const auto vecToDisplay = this->mapMissionCategories[static_cast<int> (missionx::mx_mission_type::cargo)]; vecToDisplay.size () > this->strct_user_create_layer.iMissionSubCategoryPicked)
                    missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_MISSION_SUBCATEGORY_LBL(), vecToDisplay.at (this->strct_user_create_layer.iMissionSubCategoryPicked));

                  this->addAdvancedSettingsPropertiesBeforeGeneratingRandomMission (); // v3.303.14


                  this->selectedTemplateKey = mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI();
                  this->setMessage ("Generating mission is in progress, please wait...", 10);

                  ImGui::CloseCurrentPopup ();
                  this->execAction (mx_window_actions::ACTION_GENERATE_RANDOM_MISSION);
                }
                ImGui::SetItemDefaultFocus ();
                ImGui::SameLine (modal_center.x * 1.40f);
                // back button
                if (ImGui::Button ("Back", ImVec2 (80, 0)))
                {
                  ImGui::CloseCurrentPopup ();
                }
                this->mxUiReleaseLastFont ();
              }

              ImGui::EndPopup ();
            } // END POPUP ILS
          }
          ImGui::PopStyleColor ();
        } // end for iteration loop
      }

      ImGui::EndTable ();
    } // END ImGui::BeginTable

    ImGui::PopStyleColor ();
  }
  //ImGui::EndChild ();
  ImGui::EndGroup ();

  this->mxUiReleaseLastFont ();

   // END DRAW ILS SCREEN
}


// -----------------------------------------------

void WinImguiBriefer::child_draw_nav_search()
{
  const auto win_size_vec2 = this->mxUiGetWindowContentWxH();

  ImGui::BeginGroup();
  ImGui::PushItemWidth(100.0f);
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
  {
    ImGui::TextColored(missionx::color::color_vec4_aqua, "%s", "Enter airport ICAO:");
    ImGui::SameLine();
    if (ImGui::InputText("##NavInput", this->strct_ils_layer.buf1, 8, ImGuiInputTextFlags_CharsUppercase)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
    {
      this->strct_ils_layer.sNavICAO = std::string(this->strct_ils_layer.buf1);
    }
    this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Enter airport name to fetch its data.");

  }
  this->mxUiReleaseLastFont(); // release text regular

  // ---------------
  //  Row 1 - Search
  // ---------------


  // search button
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
  ImGui::SameLine();
  if (ImGui::Button(">> Search <<"))
  {
    if (this->strct_ils_layer.fetch_metar_state == missionx::mxFetchState_enum::fetch_in_process)
    {
      this->setMessage("Active METAR fetch is in progress, please wait for it to finish or [Abort] the action.");

    }
    else
    {
      this->strct_ils_layer.sNavICAO = mxUtils::trim(this->strct_ils_layer.sNavICAO);

      if (this->strct_ils_layer.sNavICAO.empty())
      {
        this->setMessage("Please enter a valid ICAO.", 6);
      }
      else
      {
        // get Navaid information
        this->strct_ils_layer.navaid = missionx::data_manager::getICAO_info(this->strct_ils_layer.sNavICAO);
        if (this->strct_ils_layer.navaid.navRef)
        {
          this->strct_ils_layer.fetch_nav_state = missionx::mxFetchState_enum::fetch_not_started;

          this->execAction(mx_window_actions::ACTION_FETCH_NAV_INFORMATION);
        }
        else
        {
          this->setMessage("Navaid: " + this->strct_ils_layer.sNavICAO + " is invalid.", 6);
        }
      }
    }

  }


  this->mxUiReleaseLastFont(); // release title regular
  ImGui::SameLine(0.0f, 5.0f);
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
  if (ImgWindow::ButtonTooltip(mxUtils::from_u8string(ICON_FA_TRASH_ALT).append("##ClearNavICAO").c_str(), "Clear Nav ICAO"))
  {
    this->strct_ils_layer.sNavICAO.clear();
    memset(this->strct_ils_layer.buf1, 0, sizeof this->strct_ils_layer.buf1);
  }

  // v24.03.2 abort button
  if (this->strct_ils_layer.fetch_metar_state == missionx::mxFetchState_enum::fetch_in_process)
  {
    ImGui::SameLine(0.0f, 20.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_white);
    if (ImGui::Button(" Abort Metar Request "))
    {
      missionx::data_manager::threadStateMetar.flagAbortThread = true;
      this->setMessage("Trying to abort Metar fetch thread. Give it a few seconds to be released.", 8);
    }
    ImGui::PopStyleColor(3);
  }


  this->mxUiReleaseLastFont();


  ImGui::EndGroup();

  ImGui::Separator();

  // ---------------
  //  Row 2 - Nav Data to display
  // ---------------


  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
  ImGui::BeginGroup();
  ImGui::BeginChild("draw_nav_info_output", ImVec2(0.0f, win_size_vec2.y - 55.0f ), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar );
  {
    if (!this->strct_ils_layer.mapNavaidData.empty() && this->strct_ils_layer.fetch_nav_state != missionx::mxFetchState_enum::fetch_in_process
        /*&& missionx::data_manager::s_thread_sync_mutex.locked_by_caller() == false*/)
    {
      for (const auto& [key, data] : this->strct_ils_layer.mapNavaidData)
      {
        ////////////////////
        // Print ICAO title
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_BIG());
        {

          this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
          {
            ImGui::TextColored(missionx::color::color_vec4_orange, "%s, %.0fnm", data.sApDesc.c_str(), data.dDistance);
            ImGui::SameLine();
            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
              ImGui::TextColored(missionx::color::color_vec4_dimgray, "(icao_id: %i)", data.icao_id);
            this->mxUiReleaseLastFont();
          }
          this->mxUiReleaseLastFont();

          ///////////////////////////////////
          // v24.03.1 METAR information
          this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
            this->mxUiHelpMarker(missionx::color::color_vec4_greenyellow, "Place, Day+Time, COR/AUTO/NIL, Wind, Visibility, Weather, Clouds, Temperature, Air-Pressure, Trend\n\nhttps://metar-taf.com/explanation\nhttps://www.flightutilities.com/MRonline.aspx\n\nMETAR data is downloaded and dependent on flightplandatabase.com availability.\nThe site limits the number of anonymous requests to ~100 per hour, You can extend it to ~1000 if you will add your \"API authorization key\" in the External FPLN screen.");
          this->mxUiReleaseLastFont();

          if (this->strct_ils_layer.fetch_metar_state >= missionx::mxFetchState_enum::fetch_in_process)
          {
            ImGui::SameLine();
            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
            ImGui::TextColored(missionx::color::color_vec4_dimgray, "%s", ((this->strct_ils_layer.fetch_metar_state == missionx::mxFetchState_enum::fetch_in_process)? "Fetching METAR... Please wait (will try 3 times with 5 sec sleep interval)."
                                                                            : (data.sMetar.empty())? "No METAR data was found."
                                                                            : data.sMetar.c_str() )
                              );
            this->mxUiReleaseLastFont();
          }


          ///////////////////////////////////
          // v24.03.1 Frequencies information
          if (!data.listFrq.empty())
          {
            ImGui::Spacing();
            ImGui::Spacing();

            // title
            ImGui::TextColored(missionx::color::color_vec4_beige, "%s", "Tower Frequencies");

            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
            {
              bool nonEven = true;
              for (const auto& rw : data.listFrq)
              {

                ImGui::TextColored(missionx::color::color_vec4_navajowhite, "%s", rw.field2.c_str());
                if (nonEven)
                  ImGui::SameLine(win_size_vec2.x*0.5f);

                nonEven ^= 1; // toggle
              }

              if (!nonEven)
                ImGui::NewLine();
            }
            this->mxUiReleaseLastFont();
          }

          //////////////////////////////////
          // Runway information
          if (!data.mapRunwayData.empty())
          {
            ImGui::NewLine();
            ImGui::Spacing();

            // title
            ImGui::TextColored(missionx::color::color_vec4_beige, "%s", "Runways");

            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
            {
              for (const auto& [key, rw] : data.mapRunwayData)
              {
                ImGui::TextColored(missionx::color::color_vec4_lemonchiffon, "%s", rw.desc.c_str());
                ImGui::SameLine();
                ImGui::TextColored(missionx::color::color_vec4_dimgray, "(%.0fnm)", rw.dDistance);
              }
            }
            this->mxUiReleaseLastFont();
          }

          // Nearby Navigation aids information
          if (!data.listVor.empty())
          {
            ImGui::NewLine();
            ImGui::Spacing();

            // title
            ImGui::TextColored(missionx::color::color_vec4_beige, "%s", "Navigation Aids");

            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL());
            {
              ImGui::TextColored(missionx::color::color_vec4_grey, "%s", "{ident} {frq}  ({brg. ap},{type}) {name} {dist. ap} ");
            }
            this->mxUiReleaseLastFont();

            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
            {
              bool nonEven = true;
              for (const auto& loc : data.listVor)
              {
                constexpr const int SPACING_FOR_NAVAIDS = 60;
                ImGui::TextColored(missionx::color::color_vec4_cornsilk, "%s", loc.field1.c_str());
                ImGui::SameLine(((nonEven) ? SPACING_FOR_NAVAIDS : (win_size_vec2.x * 0.5f) + SPACING_FOR_NAVAIDS - 2)); // absolute position
                ImGui::TextColored(missionx::color::color_vec4_cornsilk, "%s", loc.field2.c_str());
                ImGui::SameLine();
                ImGui::TextColored(missionx::color::color_vec4_dimgray, "(%.2fnm)", loc.dDistance);
                if (nonEven)
                  ImGui::SameLine(win_size_vec2.x * 0.5f);

                nonEven ^= 1; // toggle
              }

              if (!nonEven)
                ImGui::NewLine();

            }
            this->mxUiReleaseLastFont();
          }

          // Localizer information
          if (!data.listLoc.empty())
          {
            ImGui::NewLine();
            ImGui::Spacing();

            // title
            ImGui::TextColored(missionx::color::color_vec4_beige, "%s", "Localizers");

            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v24.06.1
            {
                ImGui::TextColored(missionx::color::color_vec4_yellowgreen, "%s", "Will display ILS and LOC first and then the other localizer types."); // v24.06.1
            }
            this->mxUiReleaseLastFont(); // v24.06.1


            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
            {
              // v24.06.1 visual separation between standard localizer frequencies and other type of navigation frequencies (LPV, VOR, DME).
              int loc_type = 1; // 1 = ILS or LOC, if it is not equal to "one", then it is
              int flag_loc_type_changed_x_times = 0; // I'll add "1" everytime I'll find a loc_type that is not equal to "one".

              for (const auto& loc : data.listLoc)
              {
                  // v24.06.1 special separation logic
                  if (loc.iField1 != 1 )
                      ++flag_loc_type_changed_x_times;

                  if ( flag_loc_type_changed_x_times == 1 ){
                      ImGui::Separator();
                      ImGui::Separator();
                      ImGui::Spacing();
                  }

                ImGui::TextColored(missionx::color::color_vec4_navajowhite, "%s", loc.field2.c_str());
                ImGui::SameLine();
                ImGui::TextColored(missionx::color::color_vec4_dimgray, "(%.0fnm)", loc.dDistance);
                ImGui::Separator();
              }
            }
            this->mxUiReleaseLastFont();
          }

          this->mxUiReleaseLastFont(); // release the TITLE BIG font
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Separator();
        ImGui::Separator();
        ImGui::Spacing();
      } // end loop over Nav Data Map


    } // end if async Query was done and we have Nav data

  }
  ImGui::EndChild();
  ImGui::EndGroup();
  this->mxUiReleaseLastFont();


}

// -----------------------------------------------


void
WinImguiBriefer::draw_conv_popup_datarefs(IXMLNode& inXpData)
{
  auto         win_size_vec2 = ImGui::GetWindowSize(); // ImGui::GetContentRegionAvail();
  const auto   modal_w       = mxUiGetContentWidth();
  const auto   modal_h       = ImGui::GetWindowHeight();
  const ImVec2 modal_center(modal_w * 0.5f, modal_h * 0.5f);
  const ImVec2 vec2_multiLine_dim = ImVec2(win_size_vec2.x - 25.0f, 120.0f);

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
  ImGui::TextColored(missionx::color::color_vec4_yellow, R"(In this screen you can add datarefs to the <xpdata> main element.
You have to enter correct XML elements. Example:
<dataref name="gearForce" key="sim/flightmodel/forces/faxil_gear"/>
Other option is to add this information after generating the mission file, if you so prefer.)");
  this->mxUiReleaseLastFont();

  // close window button
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
  ImGui::NewLine();
  ImGui::SameLine(win_size_vec2.x - 90.0f);
  if (ImGui::Button("Close", ImVec2(70.0f, 0.0f)))
    ImGui::CloseCurrentPopup();
  ImGui::PopStyleColor(3);

  ImGui::BeginGroup();
  ImGui::BeginChild("MultiLineTextEdit", ImVec2(0.0f, vec2_multiLine_dim.y + 10.0f));
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.303.14
  ImGui::InputTextMultiline("##xpdataMultiLine", this->strct_conv_layer.buff_dataref, sizeof(this->strct_conv_layer.buff_dataref), vec2_multiLine_dim);
  this->mxUiReleaseLastFont();
  ImGui::EndChild();
  ImGui::EndGroup();

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
  ImGui::TextColored(missionx::color::color_vec4_beige, "%zu of %zu", std::string(this->strct_conv_layer.buff_dataref).length(), sizeof(this->strct_conv_layer.buff_dataref));

  // Parse and store button
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
  if (ImGui::Button("Parse and Store", ImVec2(110.0f, 0.0f)))
  {
    IXMLDomParser iDomTemplate;
    IXMLResults   parse_result_strct;

    std::string xpdata_s    = "<" + mxconst::get_ELEMENT_XPDATA() + "> " + std::string(this->strct_conv_layer.buff_dataref) + "</" + mxconst::get_ELEMENT_XPDATA() + ">"; // holds the user entered XML string
    IXMLNode    xpdata_node = iDomTemplate.parseString(xpdata_s.c_str(), mxconst::get_ELEMENT_XPDATA().c_str(), &parse_result_strct).deepCopy();                    // parse xml into ITCXMLNode
    if (parse_result_strct.errorCode == IXMLError_None)
    {
      Utils::xml_delete_all_subnodes(inXpData, mxconst::get_ELEMENT_DATAREF());
      for (int i1 = 0; i1 < xpdata_node.nChildNode(mxconst::get_ELEMENT_DATAREF().c_str()); ++i1)
        inXpData.addChild(xpdata_node.getChildNode(mxconst::get_ELEMENT_DATAREF().c_str(), i1).deepCopy()); // add all <dataref> childs to the global xpdata


      this->setMessage("<xpdata> information was stored");

#ifndef RELEASE
      Log::logMsg("Valid <xpdata>:\n" + xpdata_s);
#endif // !RELEASE
    }
    else
    {
      std::string err_s = std::string(IXMLPullParser::getErrorMessage(parse_result_strct.errorCode)) + " [line/col:" + mxUtils::formatNumber<long long>(parse_result_strct.nLine) + "/" + mxUtils::formatNumber<int>(parse_result_strct.nColumn) + "]";
      this->setMessage(err_s);
#ifndef RELEASE
      Log::logMsg(err_s);
#endif // !RELEASE

      Log::logMsg("Not valid <xpdata>:\n" + xpdata_s);
    }
  }
  ImGui::PopStyleColor(3);

  this->mxUiResetAllFontsToDefault(); // v3.303.14
  this->mx_add_tooltip(missionx::color::color_vec4_yellow, "The XML string you entered must be valid or it won't be stored");

  // v3.305.1
  this->add_message_text();
}

// -----------------------------------------------

void
WinImguiBriefer::draw_conv_popup_globalSettings(IXMLNode& inOutGlobalSettingsNode)
{
  constexpr const static float min_multiLineWidth_px = 5000.0f;
  auto                         win_size_vec2         = ImGui::GetWindowSize(); // ImGui::GetContentRegionAvail();
  const auto                   modal_w               = mxUiGetContentWidth();
  const auto                   modal_h               = ImGui::GetWindowHeight();
  const ImVec2                 modal_center(modal_w * 0.5f, modal_h * 0.5f);

  // const ImVec2 vec2_multiLine_dim = ImVec2((float)sizeof(this->strct_conv_layer.buff_globalSettings) + 10.0f, 200.0f);
  // Calculate the multi text width as a function of the text length relative to windows width. Minimal width is "current context windows" width.
  // const float  buff_length        = (float)std::string(this->strct_conv_layer.buff_globalSettings).length();
  const float  text_width_px      = ImGui::CalcTextSize(this->strct_conv_layer.buff_globalSettings).x;
  const ImVec2 vec2_multiLine_dim = ImVec2(((text_width_px + 2000.0f) < min_multiLineWidth_px) ? min_multiLineWidth_px : text_width_px + min_multiLineWidth_px, 200.0f);

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
  ImGui::TextColored(missionx::color::color_vec4_yellow, R"(In this screen you can edit the <global_settings> main element.
You have to enter correct XML elements.
Other option is to add this information after generating the mission file, if you so prefer.)");
  this->mxUiReleaseLastFont();


  // close window button
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
  ImGui::NewLine();
  ImGui::SameLine(win_size_vec2.x - 90.0f);
  if (ImGui::Button("Close", ImVec2(70.0f, 0.0f)))
    ImGui::CloseCurrentPopup();
  ImGui::PopStyleColor(3);
  this->mxUiReleaseLastFont();


  ///// Draw multi line
  ImGui::BeginGroup();

  // ImGuiID child_id = ImGui::GetID("GlobalSettings##MultiLineTextEdit");
  // ImGui::BeginChild(child_id, ImVec2(0.0f, vec2_multiLine_dim.y), true, ImGuiWindowFlags_HorizontalScrollbar);

  ImGui::BeginChild("MultiLineTextEdit", ImVec2(0.0f, vec2_multiLine_dim.y + 10.0f), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysHorizontalScrollbar);

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
  ImGui::InputTextMultiline("##texDataMultiLine", this->strct_conv_layer.buff_globalSettings, sizeof(this->strct_conv_layer.buff_globalSettings), vec2_multiLine_dim);
  this->mxUiReleaseLastFont();

  ImGui::EndChild();
  ImGui::EndGroup();
  ///// End draw multi line

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
  ImGui::TextColored(missionx::color::color_vec4_beige, "%zu of %zu", std::string(this->strct_conv_layer.buff_globalSettings).length(), sizeof(this->strct_conv_layer.buff_globalSettings));

  // Parse and store button
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
  if (ImGui::Button("Parse and Store", ImVec2(110.0f, 0.0f)))
  {
    IXMLDomParser iDomTemplate;
    IXMLResults   parse_result_strct;

    std::string xml_data_s  = "<" + mxconst::get_GLOBAL_SETTINGS() + "> " + std::string(this->strct_conv_layer.buff_globalSettings) + "</" + mxconst::get_GLOBAL_SETTINGS() + ">"; // holds the user entered XML string
    IXMLNode    xResultNode = iDomTemplate.parseString(xml_data_s.c_str(), mxconst::get_GLOBAL_SETTINGS().c_str(), &parse_result_strct).deepCopy();                          // parse xml into ITCXMLNode
    if (parse_result_strct.errorCode == IXMLError_None)
    {
      Utils::xml_delete_all_subnodes(inOutGlobalSettingsNode, "", true);
      for (int i1 = 0; i1 < xResultNode.nChildNode(); ++i1)
        inOutGlobalSettingsNode.addChild(xResultNode.getChildNode(i1).deepCopy()); // add all <sub elements> childs to the global_settings element


      this->setMessage("<global_settings> information was stored");

#ifndef RELEASE
      Log::logMsg("Valid <global_settings>:\n" + xml_data_s);
#endif // !RELEASE
    }
    else
    {
      std::string err_s = std::string(IXMLPullParser::getErrorMessage(parse_result_strct.errorCode)) + " [line/col:" + mxUtils::formatNumber<long long>(parse_result_strct.nLine) + "/" + mxUtils::formatNumber<int>(parse_result_strct.nColumn) + "]";
      this->setMessage(err_s);
#ifndef RELEASE
      Log::logMsg(err_s);
#endif // !RELEASE

      Log::logMsg("Not valid <global_settings>:\n" + xml_data_s);
    }
  }
  ImGui::PopStyleColor(3);

  this->mxUiResetAllFontsToDefault(); // v3.303.14
  this->mx_add_tooltip(missionx::color::color_vec4_yellow, "The XML string you entered must be valid or it won't be stored.");

  this->add_message_text();
}

// -----------------------------------------------
void
WinImguiBriefer::draw_conv_popup_flight_leg_detail(missionx::mx_local_fpln_strct& inLegData)
{
  auto         win_size_vec2 = ImGui::GetWindowSize(); // ImGui::GetContentRegionAvail();
  const auto   modal_w       = mxUiGetContentWidth();
  const auto   modal_h       = ImGui::GetWindowHeight();
  const ImVec2 modal_center(modal_w * 0.5f, modal_h * 0.5f);
  const ImVec2 vec2_multiLine_dim = ImVec2(win_size_vec2.x - 50.0f, 40.0f);

  // draw only the picked row data and not for every row
  if (this->strct_conv_layer.way_row_picked_i == inLegData.indx)
  {
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
    // close window button
    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
    if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
      ImGui::CloseCurrentPopup();
    ImGui::PopStyleColor(3);

    // Title
    ImGui::SameLine(modal_center.x - (ImGui::CalcTextSize(inLegData.getName().c_str()).x / 2.0f)); // position text in the middle of the screen
    ImGui::TextColored(missionx::color::color_vec4_lime, "%s", inLegData.getName().c_str());

    int ii = 0; // i: tracks the array buffer, so we have to hardcode its value after each CollapsingHeader.

    // welcome
    ImGui::TextColored(missionx::color::color_vec4_beige, "Enter information when en-route to: %s. Try not to overdo with information.", inLegData.getName().c_str());
    ImGui::TextColored(missionx::color::color_vec4_yellow, "En-Route Description:");

    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_purple);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, missionx::color::color_vec4_beige);
    if (ImGui::InputTextMultiline("###flightLegDesciption", inLegData.buff_arr[ii], sizeof(inLegData.buff_arr[ii]), vec2_multiLine_dim, ImGuiInputTextFlags_None))
    {
      Utils::xml_add_cdata(inLegData.xLeg, inLegData.buff_arr[ii]);
    }
    ImGui::PopStyleColor(2);

    ImGui::NewLine();


    ImGui::PushStyleColor(ImGuiCol_Header, missionx::color::color_vec4_orangered);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, missionx::color::color_vec4_tomato);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, missionx::color::color_vec4_orangered);
    {
      ++ii;                                     // 1 - 4
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
      if (ImGui::CollapsingHeader("Actions at the start/end of the Flight Leg"))
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14

        // START
        ImGui::TextColored(missionx::color::color_vec4_yellow, "[optional] Send message at the start of the Flight Leg: ");
        if (ImGui::InputTextMultiline("###messageSentOnFlightLegStart", inLegData.buff_arr[ii], sizeof(inLegData.buff_arr[ii]), vec2_multiLine_dim, ImGuiInputTextFlags_None))
        {
          IXMLNode nodeChild, targetNode;                                                                        // the targetNode is the node we are going to do work on
          IXMLNode node = Utils::xml_get_or_create_node_ptr(inLegData.xLeg, mxconst::get_ELEMENT_START_LEG_MESSAGE()); // this is our main node
          assert(node.isEmpty() == false && "Failed creating node.");

          if (node.nChildNode(mxconst::get_ELEMENT_MESSAGE().c_str()) == 0)
          {
            nodeChild = Utils::xml_get_or_create_node_ptr(node, mxconst::get_ELEMENT_MESSAGE());
            assert(nodeChild.isEmpty() == false && "Failed creating nodeChild <message> for start_leg_message.");

            const std::string val = inLegData.attribName + "_start_message";
            Utils::xml_set_attribute_in_node_asString(nodeChild, mxconst::get_ATTRIB_NAME(), val);
            node.updateAttribute(val.c_str(), mxconst::get_ATTRIB_NAME().c_str());

          }
          else
            nodeChild = node.getChildNode(mxconst::get_ELEMENT_MESSAGE().c_str());

          assert(nodeChild.isEmpty() == false && "Failed retrieving nodeChild.");
          targetNode = Utils::xml_get_or_create_node_ptr(nodeChild, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxconst::get_CHANNEL_TYPE_TEXT());
          Utils::xml_add_cdata(targetNode, inLegData.buff_arr[ii]);

#ifndef RELEASE
          Utils::xml_print_node(inLegData.xLeg);
#endif // !RELEASE
        }

        ImGui::NewLine();
        ++ii;

        ImGui::TextColored(missionx::color::color_vec4_yellow, "[optional] Commands to run at the start of the Flight Leg: ");
        if (ImGui::InputTextWithHint("###commandsAtStartOfLeg", "sim/command/xxx", inLegData.buff_arr[ii], sizeof(inLegData.buff_arr[ii]), ImGuiInputTextFlags_None))
        {
          IXMLNode node = Utils::xml_get_or_create_node_ptr(inLegData.xLeg, mxconst::get_ELEMENT_FIRE_COMMANDS_AT_LEG_START());
          node.updateAttribute(inLegData.buff_arr[ii], mxconst::get_ATTRIB_COMMANDS().c_str());
        }

        ImGui::NewLine();


        // END
        ++ii;
        ImGui::TextColored(missionx::color::color_vec4_yellow, "[optional] Send message when arriving to the waypoint: ");
        if (ImGui::InputTextMultiline("###messageSentatEndOfFlightLeg", inLegData.buff_arr[ii], sizeof(inLegData.buff_arr[ii]), vec2_multiLine_dim, ImGuiInputTextFlags_None))
        {

          IXMLNode nodeChild, targetNode;                                                                      // the targetNode is the node we are going to do work on
          IXMLNode node = Utils::xml_get_or_create_node_ptr(inLegData.xLeg, mxconst::get_ELEMENT_END_LEG_MESSAGE()); // this is our main node
          assert(node.isEmpty() == false && "Failed creating node <end_leg_message>");

          if (node.nChildNode(mxconst::get_ELEMENT_MESSAGE().c_str()) == 0)
          {
            nodeChild = Utils::xml_get_or_create_node_ptr(node, mxconst::get_ELEMENT_MESSAGE());
            assert(nodeChild.isEmpty() == false && "Failed creating nodeChild <element_message>");

            const std::string val = inLegData.attribName + "_end_message";
            Utils::xml_set_attribute_in_node_asString(nodeChild, mxconst::get_ATTRIB_NAME(), val);
            node.updateAttribute(val.c_str(), mxconst::get_ATTRIB_NAME().c_str());
          }
          else
            nodeChild = node.getChildNode(mxconst::get_ELEMENT_MESSAGE().c_str());

          assert(nodeChild.isEmpty() == false && "Failed retrieving nodeChild.");
          targetNode = Utils::xml_get_or_create_node_ptr(nodeChild, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxconst::get_CHANNEL_TYPE_TEXT());
          Utils::xml_add_cdata(targetNode, inLegData.buff_arr[ii]);
        }
        ImGui::NewLine();
        ++ii;

        ImGui::TextColored(missionx::color::color_vec4_yellow, "[optional] Commands to run when arriving to the waypoint: ");
        if (ImGui::InputTextWithHint("###commandsAtEndOfLeg", "sim/command/xxx", inLegData.buff_arr[ii], sizeof(inLegData.buff_arr[ii]), ImGuiInputTextFlags_None))
        {
          IXMLNode node = Utils::xml_get_or_create_node_ptr(inLegData.xLeg, mxconst::get_ELEMENT_FIRE_COMMANDS_AT_LEG_END());
          node.updateAttribute(inLegData.buff_arr[ii], mxconst::get_ATTRIB_COMMANDS().c_str());
        }

      } // Tasks at start/end of the Flight Leg



      // Waypoint Radius, Elevation & 3D Marker Settings
      ii = 5;                                   // we start counting from 0
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
      if (ImGui::CollapsingHeader("Waypoint Radius, Elevation & 3D Marker settings when reaching the waypoint"))
      {
        static const std::vector<const char*> vecElevOptions = { "Ignore (plane in physical area)", "On Ground", "Restrict to min/max elevation", "lower than...", "above than...", "Highest allowed elev above ground", "Lowest allowed elev above ground" };
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14

        //// Trigger Area Radius ////
        ImGui::TextColored(missionx::color::color_vec4_greenyellow, "Pick Waypoint radius of effect (in meters): ");
        ImGui::SameLine(0.0f, 5.0f);
        if (ImGui::InputInt("Waypoint area radius", &inLegData.target_trig_strct.radius_of_trigger_mt, 100))
        {
          if (inLegData.target_trig_strct.radius_of_trigger_mt <= 0)
            inLegData.target_trig_strct.radius_of_trigger_mt = 100;
          if (inLegData.target_trig_strct.radius_of_trigger_mt > 500000)
            inLegData.target_trig_strct.radius_of_trigger_mt = 500000;
        }

        ImGui::NewLine();

        //// Target elevation rules ////
        ImGui::TextColored(missionx::color::color_vec4_greenyellow, "Elevation you want the plane to be when reaching the waypoint (overrides waypoint table on ground option): ");
        ImGui::SetNextItemWidth(350.0f);
        if (ImGui::Combo("###ComboWhereDoYouWantPlane", &inLegData.target_trig_strct.elev_combo_picked_i, vecElevOptions.data(), static_cast<int> (vecElevOptions.size ())))
          inLegData.target_trig_strct.elev_lower_upper.clear();

        // Show/Hide 3D Marker checkbox
        ImGui::SameLine(0.0f, 30.0f);
        ImGui::Checkbox("Display 3D Marker##displayMarkerInHeader", &inLegData.flag_add_marker);

        // Display sliders/options based on user pick
        switch (inLegData.target_trig_strct.elev_combo_picked_i)
        {
          case 0: // ignore
            inLegData.target_trig_strct.elev_rule_s.clear();
            inLegData.target_trig_strct.flag_on_ground = false;
            break;
          case 1: // onGround
            inLegData.target_trig_strct.elev_rule_s    = mxconst::get_MX_TRUE();
            inLegData.target_trig_strct.flag_on_ground = true;
            break;
          case 2: // min/max
          {
            inLegData.target_trig_strct.elev_rule_s    = mxconst::get_MX_FALSE();
            inLegData.target_trig_strct.flag_on_ground = false;

            ImGui::TextColored(missionx::color::color_vec4_greenyellow, "Pick min/max elevation volume (-4000ft..15000ft) ");
            if (ImGui::InputInt("Min. Elevation###MinElevationTrig", &inLegData.target_trig_strct.elev_min, 100))
            {
              if (inLegData.target_trig_strct.elev_min >= inLegData.target_trig_strct.elev_max)
                inLegData.target_trig_strct.elev_max = inLegData.target_trig_strct.elev_min + 1000;
              else if (inLegData.target_trig_strct.elev_min > 20000)
                inLegData.target_trig_strct.elev_min = 20000;
              else if (inLegData.target_trig_strct.elev_min < -4000)
                inLegData.target_trig_strct.elev_min = -4000;
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(" .. ");
            ImGui::SameLine();
            if (ImGui::InputInt("Max. Elevation###MaxElevationTrig", &inLegData.target_trig_strct.elev_max, 100))
            {
              if (inLegData.target_trig_strct.elev_max <= inLegData.target_trig_strct.elev_min)
                inLegData.target_trig_strct.elev_min = inLegData.target_trig_strct.elev_max - 1000;
              else if (inLegData.target_trig_strct.elev_max < 1000)
                inLegData.target_trig_strct.elev_max = 1000;
              else if (inLegData.target_trig_strct.elev_max > 150000)
                inLegData.target_trig_strct.elev_max = 150000;
            }
            inLegData.target_trig_strct.elev_lower_upper = mxUtils::formatNumber<int>(inLegData.target_trig_strct.elev_min) + "|" + mxUtils::formatNumber<int>(inLegData.target_trig_strct.elev_max);
          }
          break;
          case 3: // lower than
          {
            inLegData.target_trig_strct.elev_rule_s = mxconst::get_MX_FALSE();
            ImGui::TextColored(missionx::color::color_vec4_greenyellow, "Pick upper elevation (plane should fly below it): ");

            ImGui::SameLine(0.0f, 30.0f);
            if (ImGui::InputInt("###InputUpperElevSlider", &inLegData.target_trig_strct.slider_elev_value_i, 100))
              inLegData.target_trig_strct.elev_lower_upper = "--" + mxUtils::formatNumber<int>(inLegData.target_trig_strct.slider_elev_value_i);

            ImGui::SameLine(0.0f, 30.0f);
            ImGui::TextColored(missionx::color::color_vec4_white, "%s", inLegData.target_trig_strct.elev_lower_upper.c_str());
          }
          break;
          case 4: // above than
          {
            inLegData.target_trig_strct.elev_rule_s = mxconst::get_MX_FALSE();
            ImGui::TextColored(missionx::color::color_vec4_greenyellow, "Pick lowest elevation (plane should fly above it): ");

            ImGui::SameLine(0.0f, 30.0f);
            if (ImGui::InputInt("###InputLowestElevSlider", &inLegData.target_trig_strct.slider_elev_value_i, 100))
              inLegData.target_trig_strct.elev_lower_upper = "++" + mxUtils::formatNumber<int>(inLegData.target_trig_strct.slider_elev_value_i);

            ImGui::SameLine(0.0f, 30.0f);
            ImGui::TextColored(missionx::color::color_vec4_white, "%s", inLegData.target_trig_strct.elev_lower_upper.c_str());
          }
          break;
          case 5: // highest allowed elev above ground
          {
            inLegData.target_trig_strct.elev_rule_s = mxconst::get_MX_FALSE();
            ImGui::TextColored(missionx::color::color_vec4_beige, "Pick elevation above ground - plane should fly below that elevation:");

            if (ImGui::InputInt("###RelativeElevAboveGround_flyBelow", &inLegData.target_trig_strct.slider_elev_value_i, 100))
              inLegData.target_trig_strct.elev_lower_upper = "---" + mxUtils::formatNumber<int>(inLegData.target_trig_strct.slider_elev_value_i);

            ImGui::SameLine(0.0f, 10.0f);
            ImGui::TextColored(missionx::color::color_vec4_white, "%s", inLegData.target_trig_strct.elev_lower_upper.c_str());
          }
          break;
          case 6: // lowest allowed elevation above ground
          {
            inLegData.target_trig_strct.elev_rule_s = mxconst::get_MX_FALSE();
            ImGui::TextColored(missionx::color::color_vec4_beige, "Pick elevation above ground - plane should fly above that elevation:");

            if (ImGui::InputInt("###RelativeElevAboveGround_flyAbove", &inLegData.target_trig_strct.slider_elev_value_i, 100))
              inLegData.target_trig_strct.elev_lower_upper = "+++" + mxUtils::formatNumber<int>(inLegData.target_trig_strct.slider_elev_value_i);

            ImGui::SameLine(0.0f, 10.0f);

            ImGui::TextColored(missionx::color::color_vec4_white, "%s", inLegData.target_trig_strct.elev_lower_upper.c_str());
          }
          break;
        } // end switch



        if (inLegData.flag_add_marker)
        {
          ImGui::NewLine();
          ImGui::TextColored(missionx::color::color_vec4_greenyellow, "How do you want to set the 3D Marker:");

          ImGui::TextUnformatted("Marker Type: ");
          ImGui::SameLine();
          ImGui::SetNextItemWidth(250.0f);
          ImGui::Combo("###3DMarkerTypes", &inLegData.marker_type_i, mxconst::get_vecMarkerTypeOptions().data(), static_cast<int> (mxconst::get_vecMarkerTypeOptions().size ()));

          ImGui::TextUnformatted("Distance to display marker in Nautical Miles (2-50): ");
          ImGui::SameLine();
          ImGui::SetNextItemWidth(100.0f);
          if (ImGui::InputFloat("##Distance to display marker", &inLegData.radius_to_display_3D_marker_in_nm_f, 0.5, 1.0, "%.2f")) // minimum 2.0 nm
          {
            if (inLegData.radius_to_display_3D_marker_in_nm_f < 2.0f)
              inLegData.radius_to_display_3D_marker_in_nm_f = 2.0f;
            else if (inLegData.radius_to_display_3D_marker_in_nm_f > 50.0f)
              inLegData.radius_to_display_3D_marker_in_nm_f = 50.0f;
          }
        }
      } // elevation & 3D Marker header


      // Triggers
      ii = 5;
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
        if (ImGui::CollapsingHeader("Triggers / Events during en-route"))
        {
          this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
          subDraw_ui_xTrigger_main(inLegData, this->strct_conv_layer.flag_refreshTriggerListFrom_xNode, inLegData.indx, this->strct_conv_layer.mapOfGlobalTriggers, this->strct_conv_layer.vecGlobalTriggers_names);
        }
      }
      ii = 5;
    } // end pushStyle
    ImGui::PopStyleColor(3);

    this->mxUiResetAllFontsToDefault(); // v3.303.14
  }
} // draw_conv_popup_flight_leg_detail



// -----------------------------------------------

void
WinImguiBriefer::draw_conv_popup_briefer(missionx::mx_local_fpln_strct& inLegData)
{
  auto         win_size_vec2 = ImGui::GetContentRegionAvail();
  const ImVec2 modal_center(win_size_vec2.x * 0.5f, win_size_vec2.y * 0.5f);

  if (this->strct_conv_layer.way_row_picked_i == inLegData.indx)
  {
    // int ii = 0;
    inLegData.iCurrentBuf = 0;

    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
    // close window button
    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
    if (ImGui::Button("Close", ImVec2(120, 0)))
      ImGui::CloseCurrentPopup();
    ImGui::PopStyleColor(3);

    // Title
    constexpr static const char* title_s = "Briefer and Mission Information";
    ImGui::SameLine(modal_center.x - (ImGui::CalcTextSize(title_s).x / 2.0f));
    ImGui::TextColored(missionx::color::color_vec4_lime, title_s);

    // welcome
    ImGui::TextColored(missionx::color::color_vec4_beige, "Enter the mission information (what user sees when they click on the mission image) and Enter the briefer description");
    ImGui::NewLine();

    ImGui::PushStyleColor(ImGuiCol_Header, missionx::color::color_vec4_orangered);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, missionx::color::color_vec4_tomato);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, missionx::color::color_vec4_orangered);
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
      if (ImGui::CollapsingHeader("< Mission Info >"))
      {
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_purple);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, missionx::color::color_vec4_beige);
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
        ImGui::TextColored(missionx::color::color_vec4_yellow, "Written By: ");
        ImGui::SameLine();
        if (ImGui::InputTextWithHint("###WrittenBy", "== your name ==", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof(inLegData.buff_arr[inLegData.iCurrentBuf]), ImGuiInputTextFlags_None))
        {
          this->strct_conv_layer.xConvInfo.updateAttribute(inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_WRITTEN_BY().c_str(), mxconst::get_ATTRIB_WRITTEN_BY().c_str()); // 0
        }
        ++inLegData.iCurrentBuf;

        ImGui::SameLine(0.0f, 20.0f);
        ImGui::TextColored(missionx::color::color_vec4_yellow, "Estimate Time: ");
        ImGui::SameLine();
        if (ImGui::InputTextWithHint("###EstimateTime", "~45min, ~60min, ~90min etc..", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof(inLegData.buff_arr[inLegData.iCurrentBuf]), ImGuiInputTextFlags_None))
        {
          this->strct_conv_layer.xConvInfo.updateAttribute(inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_ESTIMATE_TIME().c_str(), mxconst::get_ATTRIB_ESTIMATE_TIME().c_str()); // 1
        }
        ++inLegData.iCurrentBuf;

        ImGui::NewLine();

        ImGui::TextColored(missionx::color::color_vec4_yellow, "Weather Settings: ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(250.0f);
        if (ImGui::InputTextWithHint("###WeatherSettings", "User Preferred Settings / Set to Overcast", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof(inLegData.buff_arr[inLegData.iCurrentBuf]), ImGuiInputTextFlags_None))
        {
          this->strct_conv_layer.xConvInfo.updateAttribute(inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_WEATHER_SETTINGS().c_str(), mxconst::get_ATTRIB_WEATHER_SETTINGS().c_str()); // 3
        }
        ++inLegData.iCurrentBuf;

        ImGui::NewLine();
        ImGui::TextColored(missionx::color::color_vec4_yellow, "Other settings: ");
        ImVec2 vec2_dimentions = ImVec2(win_size_vec2.x - 50.0f, 60.0f);

        if (ImGui::InputTextMultiline("###OtherSettings", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof(inLegData.buff_arr[inLegData.iCurrentBuf]), vec2_dimentions))
        {
          this->strct_conv_layer.xConvInfo.updateAttribute(inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_OTHER_SETTINGS().c_str(), mxconst::get_ATTRIB_OTHER_SETTINGS().c_str()); // 4
        }
        ++inLegData.iCurrentBuf;

        ImGui::PopStyleColor(2);

        ImGui::NewLine();
      } // < Mission Info >

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
      inLegData.iCurrentBuf = 4;                // we ned this since when the "collapse header" is closed, the code won't run so the numbering will start in 0 and not in the correct buffer
      if (ImGui::CollapsingHeader("< briefer > and Starting location"))
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
        // inLegData.iCurrentBuf is used through the next few widgets
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_purple);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, missionx::color::color_vec4_beige);

        ImGui::TextColored(missionx::color::color_vec4_yellow, "Start Heading: ");
        ImGui::SameLine();
        if (ImGui::InputTextWithHint("###BrieferStartHeading", "0..359", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof(inLegData.buff_arr[inLegData.iCurrentBuf]), ImGuiInputTextFlags_CharsDecimal))
        {
          inLegData.xLeg.updateAttribute(inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_HEADING_PSI().c_str(), mxconst::get_ATTRIB_HEADING_PSI().c_str()); // 3
        }
        ImGui::SameLine(0.0f, 5.0f);

        const auto lmbda_getAndSetHeading = [&inLegData](XPLMDataRef inRef)
        {
          auto        heading_f = XPLMGetDataf(inRef);
          std::string heading_s = mxUtils::formatNumber<int>(static_cast<int> (heading_f), 0);
          inLegData.setBuff(inLegData.iCurrentBuf, heading_s);

          inLegData.xLeg.updateAttribute(inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_HEADING_PSI().c_str(), mxconst::get_ATTRIB_HEADING_PSI().c_str());
        };

        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
        if (ImGui::Button("P"))
        {
          lmbda_getAndSetHeading(missionx::drefConst.dref_heading_true_psi_f);
        }
        this->mx_add_tooltip(missionx::color::color_vec4_white, "Get Plane Heading.");
        ImGui::SameLine(0.0f, 5.0f);
        if (ImGui::Button("C"))
        {
          const XPLMDataRef dref = XPLMFindDataRef("sim/graphics/view/view_heading"); // camera heading

          lmbda_getAndSetHeading(dref);
        }
        this->mx_add_tooltip(missionx::color::color_vec4_white, "Get Camera Heading.");
        ImGui::PopStyleColor();
        // end of same inLegData.iCurrentBuf

        ++inLegData.iCurrentBuf;

        ImGui::NewLine();

        ImGui::TextColored(missionx::color::color_vec4_yellow, "Mission Description (%i chars): ", missionx::LOG_BUFF_SIZE);
        ImGui::TextColored(missionx::color::color_vec4_beige, "%zu", std::string(inLegData.buff_arr[inLegData.iCurrentBuf]).length());
        const ImVec2 vec2_dimentions = ImVec2(win_size_vec2.x - 50.0f, 60.0f);
        if (ImGui::InputTextMultiline("###BrieferMissionDesc", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof(inLegData.buff_arr[inLegData.iCurrentBuf]), vec2_dimentions)) // 5
        {
          Utils::xml_add_cdata(inLegData.xLeg, inLegData.buff_arr[inLegData.iCurrentBuf]);
        }

        ImGui::PopStyleColor(2);

        ImGui::NewLine();
      } // < Mission Info >

      inLegData.iCurrentBuf = 6;
    }
    ImGui::PopStyleColor(3);

    this->mxUiResetAllFontsToDefault();
  }
}

// -----------------------------------------------

void
WinImguiBriefer::subDraw_popup_user_lat_lon(mx_trig_strct_& inout_trig)
{
  static ImVec2 vec2Pos;
  auto          win_size_vec2 = ImGui::GetWindowSize();
  const ImVec2  center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(win_size_vec2.x / 2.0f, 100.0f));

  {
    //// draw 2 text items as lat and long
    ImGui::TextColored(missionx::color::color_vec4_yellow, "Enter Latitude / Longitude");
    ImGui::TextColored(missionx::color::color_vec4_yellow, "Lat/Lon ");
    ImGui::SameLine();
    ImGui::InputFloat("##userCustomLat", &vec2Pos.x);
    ImGui::SameLine(0.0f, 2.0f);
    ImGui::TextColored(missionx::color::color_vec4_yellow, "/");
    ImGui::SameLine();
    ImGui::InputFloat("##userCustomLon", &vec2Pos.y);

    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
    if (ImGui::Button("Apply##ApplyUserLatLon"))
    {
      Point p(vec2Pos.x, vec2Pos.y);
      inout_trig.setBuff(inout_trig.iCurrentBuf, p.get_point_lat_lon_as_string());
      ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor(4);

    ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);
    if (ImGui::Button("Cancel##CancelUserLatLon"))
      ImGui::CloseCurrentPopup();
  }
}

// -----------------------------------------------

void
WinImguiBriefer::subDraw_popup_outcome(mx_trig_strct_& inout_trig, IXMLNode& inMessageTemplates)
{
  IXMLNode xOutcome = Utils::xml_get_or_create_node_ptr(inout_trig.node_ptr, mxconst::get_ELEMENT_OUTCOME());

  assert(xOutcome.isEmpty() == false && "<outcome> element can't be empty.");

  const auto lmbda_get_outcome_list_of_attributes_from_node = [](IXMLNode& in_xOutcome)
  {
    std::vector<const char*> vecOutcomeAttributes;
    for (int i1 = 0; i1 < in_xOutcome.nAttribute(); ++i1)
    {
      vecOutcomeAttributes.emplace_back(in_xOutcome.getAttributeName(i1));
    }

    return vecOutcomeAttributes;
  };

  static int  attrib_picked_i = -1;
  std::string attrib_label_cc;
  const auto  vecAttributeList =
    lmbda_get_outcome_list_of_attributes_from_node(xOutcome); // we fill vector from <outcome> element attribute names. When we update its attribute the memory address changes, therefore we must refresh the vector every time. We can solve that if we will create the attributes ahead of time and not dynamically based on the XML header.

  // List of options
  ImGui::SetNextItemWidth(180.0f);
  ImGui::Combo("##comboListOfOutcomeAttribs", &attrib_picked_i, vecAttributeList.data(), static_cast<int> (vecAttributeList.size ()));
  // close button
  ImGui::SameLine(ImGui::GetWindowWidth() - 100.0f);
  if (ImGui::Button("Close##closeOutcomePopup"))
    ImGui::CloseCurrentPopup();

  if (attrib_picked_i > -1 && attrib_picked_i < (static_cast<int> (vecAttributeList.size ()) - 1))
    attrib_label_cc = std::string(vecAttributeList[attrib_picked_i]);

  if (attrib_label_cc.empty() == false)
  {
    // first input text, represent keyName or value (if index < 2 than it represent keyName)
    ImGui::InputText(attrib_label_cc.c_str(), inout_trig.buffArray[inout_trig.iCurrentBuf], sizeof(inout_trig.buffArray[inout_trig.iCurrentBuf]));
    if (attrib_picked_i < 2)
      this->mx_add_tooltip(missionx::color::color_vec4_beige, "Enter an alias to the message"); // TODO maybe create the keyName internaly so simmer will only focus on the text
    else if (attrib_picked_i < 4)
      this->mx_add_tooltip(missionx::color::color_vec4_beige, "Enter an alias to the script you will create later / or the  script exists"); // TODO maybe create the keyName internaly so simmer will only focus on the text
    else
      this->mx_add_tooltip(missionx::color::color_vec4_beige, "Enter text that represent the option you picked (command or dataref list divided by comma (\",\")");

    // second input text only if outcome is message
    if (attrib_picked_i < 2) // create message for options 0,1 in combo
    {
      ++inout_trig.iCurrentBuf;
      ImGui::SetNextItemWidth(300.0f);
      ImGui::InputText("Enter Message Text##outcomeText", inout_trig.buffArray[inout_trig.iCurrentBuf], sizeof(inout_trig.buffArray[inout_trig.iCurrentBuf]));
      this->mx_add_tooltip(missionx::color::color_vec4_beige, "Enter message text"); // TODO maybe create the keyName internaly so simmer will only focus on the text
    }

    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
    if (ImGui::Button("Apply##ApplyOutcomeChanges"))
    {
      std::string attrib_val1_s = (attrib_picked_i < 2) ? inout_trig.getBuff(inout_trig.iCurrentBuf - 1) : inout_trig.getBuff(inout_trig.iCurrentBuf); // if user picked options 0,1 in combo then we have 2 input text
      if (attrib_picked_i < 2)                                                                                                                         // trig_fire or trig_left
      {
        std::string attrib_val2_s = inout_trig.getBuff(inout_trig.iCurrentBuf); // message text

        if (attrib_val1_s.empty()) // if empty then reset <outcome> attrib
          xOutcome.updateAttribute("", attrib_label_cc.c_str(), attrib_label_cc.c_str());
        else
        {
          xOutcome.updateAttribute(attrib_val1_s.c_str(), attrib_label_cc.c_str(), attrib_label_cc.c_str()); // this will allow to assign message keyName without text, in cases where messages was created already.
          if (!attrib_val2_s.empty())
          {
            IXMLNode xMessage = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(inMessageTemplates, mxconst::get_ELEMENT_MESSAGE(), mxconst::get_ATTRIB_NAME(), attrib_val1_s, false);

            if (xMessage.isEmpty()) // not found
              xMessage = Utils::xml_create_message(attrib_val1_s, attrib_val2_s);

            if (xMessage.isEmpty() == false)
              inMessageTemplates.addChild(xMessage);
          }
        }
      }
      else
        xOutcome.updateAttribute(attrib_val1_s.c_str(), attrib_label_cc.c_str(), attrib_label_cc.c_str()); // this will allow to assign the text or reset it if empty for <script> and <commands>
    }
    ImGui::PopStyleColor(4);
  }


  ImGui::PushStyleColor(ImGuiCol_Separator, missionx::color::color_vec4_white);
  ImGui::Separator();
  ImGui::PopStyleColor();

  if (!xOutcome.isEmpty())
  {
    IXMLRenderer xmlRenderer;
    std::string  outcom_s = xmlRenderer.getString(xOutcome);
    char         buf[missionx::LOG_BUFF_SIZE]{ 0 };
#ifdef IBM
    memcpy_s(buf, sizeof(buf), outcom_s.c_str(), (sizeof(buf) > outcom_s.length()) ? outcom_s.length() : sizeof(buf));
#else
    memcpy(buf, outcom_s.c_str(), (sizeof(buf) > outcom_s.length()) ? outcom_s.length() : sizeof(buf));
#endif

    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_antiquewhite);
    ImGui::BeginChild("##outcomeXML_text", ImVec2(ImGui::GetContentRegionAvail().x, 60.0f));
    {
      ImGui::TextWrapped("%s", xmlRenderer.getString(xOutcome));
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(2);

    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_white);
    ImGui::InputTextMultiline("##trigOutcomeMultiLine", buf, sizeof(buf), ImVec2(ImGui::GetContentRegionAvail().x, 30.0f), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    this->mx_add_tooltip(missionx::color::color_vec4_yellow, "You can copy the <outcome> text.");
  }
}

// -----------------------------------------------

void
WinImguiBriefer::subDraw_fpln_table(IXMLNode& inMainNode, std::map<int, missionx::mx_local_fpln_strct>& in_map_tableOfParsedFpln)
{
  {

    auto win_size_vec2 = ImGui::GetWindowSize();

    // Alternating rows slightly grayish
    ImGui::PushStyleColor(ImGuiCol_TableRowBg, IM_COL32(0x20, 0x20, 0x20, 0xff));
    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, IM_COL32(0x30, 0x30, 0x30, 0xff));
    constexpr int col_i      = 10;
    int           COLUMN_NUM = (this->strct_conv_layer.flag_foundBriefer_index0) ? col_i : col_i + 1;
    {

      missionx::WinImguiBriefer::HelpMarker("The table holds the list of waypoints from the imported flight plan.\n\"Leg\": Mark as a waypoints you must reach.\n\t\tIndex 0 is your 'start' location.\n\"GD\": Flag flight leg to be on ground.\n\"BR\": Convert Index 1 waypoint to briefer, only if index 0 is unavailable.\n\"Details\": Opens a popup.\n\"IG\": Ignore this "
                       "waypoint. Won't be part of the GPS.");
      if (ImGui::BeginTable("Table_fpln_lnm", COLUMN_NUM, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit))
      {
        ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible

        // Set up the columns of the table - TBD
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
        {
          ImGui::TableSetupColumn("Indx", ImGuiTableColumnFlags_None, 20.0f);   // Index, row number
          ImGui::TableSetupColumn("Ident", ImGuiTableColumnFlags_None, 100.0f); // to ICAO + (keyName)
          ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_None, 75.0f);

          ImGui::TableSetupColumn("Leg", ImGuiTableColumnFlags_None, 30.0f); // Is this a flight leg ?
          ImGui::TableSetupColumn("GD", ImGuiTableColumnFlags_None, 30.0f);  // on ground ?

          if (this->strct_conv_layer.flag_foundBriefer_index0 == false)
            ImGui::TableSetupColumn("BR", ImGuiTableColumnFlags_None, 30.0f); // Convert to briefer so we will have our departure data if we have no briefer waypoint

          ImGui::TableSetupColumn("Lat / Lon", ImGuiTableColumnFlags_None, 150.0f);
          ImGui::TableSetupColumn("Distance nm", ImGuiTableColumnFlags_None, 85.0f); // distance between waypoints

          ImGui::TableSetupColumn("Settings", ImGuiTableColumnFlags_None, 70.0f); // will open modal window that will display lat/lon + notes
          ImGui::TableSetupColumn("Marker", ImGuiTableColumnFlags_None, 30.0f);   // v3.0.303.2 Display Marker at leg location
          ImGui::TableSetupColumn("IG", ImGuiTableColumnFlags_None, 30.0f);       // ignore the waypoint during mission build
          ImGui::TableHeadersRow();
        }
        ImGui::PopStyleColor();


        //// Add ROWS to the table
        for (auto& [indx, legData] : in_map_tableOfParsedFpln)
        {
          int  i1                  = 0;
          bool flagDisableRowColor = false;
          if (legData.flag_ignore_leg)
          {
            flagDisableRowColor = true;
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive)); // dark gray
          }


          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(i1); // 0
          ImGui::TextUnformatted(mxUtils::formatNumber<int>(indx).c_str());

          ++i1;
          ImGui::TableSetColumnIndex(i1); // 1 keyName (ident)

          if (flagDisableRowColor == false)
          {
            ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_bisque);
          }
          if (legData.name.empty())
            ImGui::Text("%s", legData.ident.c_str());
          else
            ImGui::Text("%s", (std::string(legData.name) + ((legData.ident.compare(legData.name) == 0) ? "" : "(" + legData.ident + ")")).c_str());

          if (flagDisableRowColor == false)
            ImGui::PopStyleColor();

          ++i1;
          ImGui::TableSetColumnIndex(i1); // 2 type
          ImGui::Text("%s", legData.type.c_str());

          ++i1;
          ImGui::TableSetColumnIndex(i1);                         // 3 is leg checkbox
          if (legData.indx > 0 && !legData.flag_convertToBriefer) // briefer won't be a leg
          {
            char buf[64];
            snprintf(buf, sizeof(buf) - 1, "###isLeg%i", legData.indx);
            if (ImGui::Checkbox(buf, &legData.flag_isLeg))
              this->strct_conv_layer.flag_refresh_table_from_file = true;

            this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Is this waypoint a location you must reach ?\nIf not then it will be a waypoint in your GPS (optional).\nTip: In most cases first two rows are probably briefer and starting locations.");
          }

          ++i1;
          ImGui::TableSetColumnIndex(i1);                         // 4 on Ground / Airborne
          if (legData.indx > 0 && !legData.flag_convertToBriefer) // skip briefer
          {
            char buf[64];
            snprintf(buf, sizeof(buf) - 1, "###onGroundOrAirborne%i", legData.indx);
            if (ImGui::Checkbox(buf, &legData.target_trig_strct.flag_on_ground))
              legData.target_trig_strct.elev_rule_s = (legData.target_trig_strct.flag_on_ground) ? "true" : "false";
            this->mx_add_tooltip(missionx::color::color_vec4_yellow, "On Ground");
          }

          if (this->strct_conv_layer.flag_foundBriefer_index0 == false) // convert to briefer Hide Show Column
          {
            ++i1;
            ImGui::TableSetColumnIndex(i1); // convert to briefer - depends on >strct_conv_layer.flag_foundBriefer_index0
            if (legData.indx == 1)          // skip briefer
            {
              char buf[64];
              snprintf(buf, sizeof(buf) - 1, "###ConvertToBriefer%i", legData.indx);
              if (ImGui::Checkbox(buf, &legData.flag_convertToBriefer))
                this->strct_conv_layer.flag_refresh_table_from_file = true;

              this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Convert Leg to Briefer");
            }
          }


          ++i1;
          ImGui::TableSetColumnIndex(i1); // lat/lon (alt)
          {
            char buf[64];
            char bufLatLon[30];

            snprintf(buf, sizeof(buf) - 1, "###lnmLatLon%i", legData.indx);
            snprintf(bufLatLon, sizeof(bufLatLon) - 1, "%.6f/%.6f", static_cast<float> (legData.p.getLat ()), static_cast<float> (legData.p.getLon ()));

            ImGui::SetNextItemWidth(180.0f);
            if (legData.indx % 2 == 0)
              ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0x20, 0x20, 0x20, 0xff));
            else
              ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0x30, 0x30, 0x30, 0xff));

            ImGui::InputText(buf, bufLatLon, sizeof(bufLatLon), ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();
          }

          ++i1;
          ImGui::TableSetColumnIndex(i1); // distance
          {
            ImGui::TextColored(missionx::color::color_vec4_bisque, "%.2f (%.2f)", legData.distToPrevWaypoint, legData.cumulativeDist);
          }

          ++i1;
          ImGui::TableSetColumnIndex(i1); // popup - details
          if (flagDisableRowColor == false)
          {
            if (legData.indx == 0 || legData.flag_isLeg || legData.flag_convertToBriefer)
            {

              char buf[64];
              if (legData.indx == 0 || (legData.indx == 1 && legData.flag_convertToBriefer)) // briefer row
                snprintf(buf, sizeof(buf) - 1, "Briefer###LegSetting%i", legData.indx);
              else
                snprintf(buf, sizeof(buf) - 1, "Flight Leg###LegSetting%i", legData.indx);


              if (ImGui::Button(buf))
              {
                this->strct_conv_layer.way_row_picked_i = legData.indx;

                if (legData.indx == 0 || (legData.indx == 1 && legData.flag_convertToBriefer)) // briefer popup
                  ImGui::OpenPopup(POPUP_BRIEFER_SETTINGS.c_str());
                else
                {
                  this->strct_conv_layer.flag_refreshTriggerListFrom_xNode = true;
                  ImGui::OpenPopup(POPUP_FLIGHT_LEG_SETTINGS.c_str());
                }
              } // button

            } // end if to display button
          }   // ignore


          ++i1;                                                                                                 // v3.0.303.2
          ImGui::TableSetColumnIndex(i1);                                                                       // 3D marker
          if (legData.indx && legData.flag_isLeg && !legData.flag_convertToBriefer && !legData.flag_ignore_leg) // Show option only for picked legs that are not briefer or ignored
          {
            char buf[64];
            snprintf(buf, sizeof(buf) - 1, "###add3DMarker%i", legData.indx);

            ImGui::Checkbox(buf, &legData.flag_add_marker);
            this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Place 3D marker at leg location.");
          }


          ++i1;
          ImGui::TableSetColumnIndex(i1);                             // Checkbox: Ignore waypoint line when build mission file
          if (legData.indx && legData.flag_convertToBriefer == false) // briefer won't be a leg
          {
            char buf[64];
            snprintf(buf, sizeof(buf) - 1, "###ignoreWaypoint%i", legData.indx);

            ImGui::Checkbox(buf, &legData.flag_ignore_leg);
            this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Ignore this waypoint when building the mission file?");
          }
          //// End of table columns


          ///// FLIGHT LEG DETAIL POPUP
          ImGui::PushStyleColor(ImGuiCol_PopupBg, missionx::color::color_vec4_blue);
          {

            const auto popupHeight_f = ImGui::GetIO().DisplaySize.y * 0.85f;

            const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f - 30.0f);
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(win_size_vec2.x - 20.0f, popupHeight_f));

            if (ImGui::BeginPopupModal(POPUP_FLIGHT_LEG_SETTINGS.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
              draw_conv_popup_flight_leg_detail(legData);
              ImGui::EndPopup();
            }

            ///// FLIGHT LEG DETAIL POPUP
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(win_size_vec2.x - 20.0f, popupHeight_f));

            if (ImGui::BeginPopupModal(POPUP_BRIEFER_SETTINGS.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {

              draw_conv_popup_briefer(legData);
              ImGui::EndPopup();
            }

          } // end popup block code handling
          ImGui::PopStyleColor();


          if (flagDisableRowColor) // if we ignored line then reset style
            ImGui::PopStyleColor();
        } // end loop over table rows

        if (this->strct_conv_layer.flag_refresh_table_from_file)
        {
          auto table_ptr = ImGui::GetCurrentTable();
          ImGui::TableSetColumnWidthAutoAll(table_ptr);
          this->strct_conv_layer.flag_refresh_table_from_file = false;
        }
        // End of table
        ImGui::EndTable();
      } // END ImGui::BeginTable
    }
    ImGui::PopStyleColor(2);
  }
}


// -----------------------------------------------


bool
WinImguiBriefer::validate_conversion_table(IXMLNode& inMainNode, std::map<int, missionx::mx_local_fpln_strct> in_map_tableOfParsedFpln)
{

  // check if we have at least one flight leg, if not then the last one will be the active flight plan
  bool flag_found_active_flight_leg{ false };
  for (auto& [indx, legData] : in_map_tableOfParsedFpln)
  {
    if (indx == 0) // ignore briefer
      continue;

    if (legData.flag_ignore_leg)
      continue;

    if (legData.flag_isLeg)
      flag_found_active_flight_leg = true;
  }
  if (flag_found_active_flight_leg == false)
  {
    this->setMessage("Could not find any active flight leg. Flag at least one waypoint as a \"Leg\"", 8);
    return false;
  }

  return true;
}


// -----------------------------------------------

void
WinImguiBriefer::draw_conv_main_fpln_to_mission_window()
{
  auto win_size_vec2 = ImGui::GetContentRegionAvail();

  ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);

  // First Time code
  if (this->strct_conv_layer.flag_first_time)
  {
    this->strct_conv_layer.conv_sub_ui = mx_conv_sub_ui::conv_pick_fpln; // enum value

    this->strct_conv_layer.file_picked_i    = -1;
    this->strct_conv_layer.way_row_picked_i = -1;
    this->strct_conv_layer.vecFileList_char.clear();
    this->strct_conv_layer.mapFileList.clear();
    this->strct_conv_layer.set_conv_map_files(this->read_fpln_files()); // set the mapFileList and the vecFileList_char

    this->strct_conv_layer.flag_first_time = false;
  }


  constexpr static std::string_view welcome_str_vu             = R"(Welcome to flight plan conversion screen. This screen should be used only in 2D mode and not in VR.
In this screen you will pick a flight plan based on LittleNavMap (".lnmpln") from X-Plane FPLN folder ("Output/FMS plans").
)";
  constexpr static std::string_view design_mission_fpln_str_vu = R"(Design the mission Flight Plan based on the parsed LittleNavMap file.
1. Your briefer waypoint is your starting location (usually line 0. If it is absent, you can convert line 1 to be a briefer "BR")
2. Pick waypoints that are mandatory to pass, you must have at least one. Fill the information of that flight leg using the respective button in the "details column".
3. Once you [Generate] the mission, you can load it from "Load Mission" screen (Pick the Random image - the first one, in that screen).
)";

  constexpr static std::string_view triggers_fpln_str_vu = R"(The "Trigger or Event screen" allows you to define different ways to interact with the simmer.
You can define: "Radius, Polygonal, Script or even Camera" based areas of effect, depends on your needs.
Each trigger must have a unique name in all the mission file.
)";

  constexpr static std::string_view features_not_implemented_str_vu = R"(The list of features that were not implemented:
> start/end screen.
> script editor.
> tasks - there is only the mandatory task.
> choices

There are other options that are best handle manually inside an editor and not in the Mission-X Conversion window.
(Read the designer guide))";


  // Display Welcome text //
  ImGui::BeginGroup();
  {
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_aquamarine);
    ImGui::TextWrapped("%s", (this->strct_conv_layer.conv_sub_ui == mx_conv_sub_ui::conv_pick_fpln) ? welcome_str_vu.data() : (this->strct_conv_layer.conv_sub_ui == mx_conv_sub_ui::conv_design_fpln) ? design_mission_fpln_str_vu.data() : triggers_fpln_str_vu.data());
    ImGui::PopStyleColor();
    this->mxUiReleaseLastFont(); // v3.303.14
  }
  ImGui::Separator();

  ImGui::EndGroup();



  // Display list of fpln or design mission //
  switch (this->strct_conv_layer.conv_sub_ui)
  {
    case (mx_conv_sub_ui::conv_pick_fpln):
    {
      ImGui::BeginGroup();
      {
        missionx::WinImguiBriefer::HelpMarker(features_not_implemented_str_vu.data()); // v3.0.301 B4 added unsupported features example

        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14

        ImGui::TextColored(missionx::color::color_vec4_yellow, "Pick a file from the list:");

        ImGui::SameLine(win_size_vec2.x * 0.75f);
        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
        if (ImGui::Button(" Load Saved State File "))
        {
          this->strct_conv_layer.flag_load_conversion_file = true; // will cause the file information to be refreshed in the next run
          this->strct_conv_layer.file_picked_i             = -1;   // reset pick state so we will only see the "converter" file keyName
          std::string conv_file                            = Utils::getMissionxCustomSceneryFolderPath_WithSep(true) + "/random/briefer/" + mxconst::get_CONVERTER_FILE();
        }
        ImGui::PopStyleColor(3);

        this->mxUiReleaseLastFont(); // v3.303.14

        this->mx_add_tooltip(missionx::color::color_vec4_yellowgreen, "Only loads last conversion 'saved state' file - if exists.");


        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL()); // v3.303.14
        if (ImGui::BeginListBox("##ListOfFlightPlanFiles"))
        {
          for (int n = 0; n < static_cast<int> (this->strct_conv_layer.vecFileList_char.size ()); n++)
          {
            const bool is_selected = (this->strct_conv_layer.file_picked_i == n);
            if (ImGui::Selectable(this->strct_conv_layer.vecFileList_char.at(n), is_selected)) // , 0, ImVec2(350.0f, 150.0f)
              this->strct_conv_layer.file_picked_i = n;

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndListBox();
        }
        this->mxUiReleaseLastFont(); // v3.303.14

        // bottom buttons
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
        if (ImGui::Button(" Refresh File List "))
        {
          this->strct_conv_layer.flag_first_time = true; // will cause the file information to be refreshed in the next run
        }
        if (this->strct_conv_layer.file_picked_i > -1 || this->strct_conv_layer.flag_load_conversion_file)
        {
          ImGui::SameLine(0.0f, 20.0f);
          if (ImGui::Button("  Read Flight Plan File and continue to step 2...  ") || this->strct_conv_layer.flag_load_conversion_file)
          {

            std::string file;
            std::string filePath;

            // v3.305.1 reset xSavedGlobalSettingsNode Node
            this->strct_conv_layer.xSavedGlobalSettingsNode = IXMLNode();

            if (this->strct_conv_layer.flag_load_conversion_file)
            {
              file     = mxconst::get_CONVERTER_FILE();
              filePath = Utils::getMissionxCustomSceneryFolderPath_WithSep(true) + "/random/briefer/" + file;
            }
            else if (this->strct_conv_layer.file_picked_i > -1)
            {
              file     = this->strct_conv_layer.vecFileList_char.at(this->strct_conv_layer.file_picked_i);
              filePath = this->strct_conv_layer.mapFileList[file];
            }
            else
              break;

            missionx::data_manager::map_tableOfParsedFpln.clear();

            if (this->strct_conv_layer.flag_load_conversion_file)
            {
              // call read_and_parse_conversion_file
              missionx::data_manager::map_tableOfParsedFpln = read_and_parse_saved_state(filePath); // will update this->strct_conv_layer.xXPlaneDataRef_global and this->strct_conv_layer.xSavedGlobalSettingsNode
            }
            else
              missionx::data_manager::map_tableOfParsedFpln = missionx::data_manager::read_and_parse_littleNavMap_fpln(filePath);


            if (missionx::data_manager::map_tableOfParsedFpln.size() == static_cast<size_t> (0))
            {
              this->setMessage("No information was loaded.");
            }
            else // prepare a new in memory XML
            {
              this->strct_conv_layer.xConvMainNode = IXMLNode::createXMLTopNode("xml", TRUE);
              this->strct_conv_layer.xConvMainNode.addAttribute(mxconst::get_ATTRIB_VERSION().c_str(), "1.0");
              this->strct_conv_layer.xConvMainNode.addAttribute("encoding", "ASCII"); // "ISO-8859-1");
              this->strct_conv_layer.xConvMainNode.addClear("\n\tFile has been created by Mission-X plug-in.\n\tAny modification might break or invalidate the file.\n\t", "<!--", "-->");

              IXMLDomParser iDomTemplate;
              IXMLResults   parse_result_strct;
              this->strct_conv_layer.xConvDummy = iDomTemplate.parseString(std::string(this->strct_conv_layer.DUMMY_SKELATON_ELEMENT.data()).c_str(), mxconst::get_DUMMY_ROOT_DOC().c_str(), &parse_result_strct).deepCopy(); // parse xml into ITCXMLNode
              this->strct_conv_layer.xConvMainNode.addChild(this->strct_conv_layer.xConvDummy);

              // Add mission info
              if (!this->strct_conv_layer.xConvInfo.isEmpty())
                this->strct_conv_layer.xConvInfo.deleteNodeContent();

              this->strct_conv_layer.xConvInfo = this->strct_conv_layer.xConvDummy.addChild(mxconst::get_ELEMENT_MISSION_INFO().c_str());
              Utils::xml_add_comment(this->strct_conv_layer.xConvDummy, " ----------------- ");

              // main <xpdata>
              if (this->strct_conv_layer.flag_load_conversion_file)
                this->strct_conv_layer.xConvDummy.addChild(this->strct_conv_layer.xXPlaneDataRef_global);
              else
                this->strct_conv_layer.xXPlaneDataRef_global = this->strct_conv_layer.xConvDummy.addChild(mxconst::get_ELEMENT_XPDATA().c_str());

              Utils::xml_add_comment(this->strct_conv_layer.xConvDummy, " ----------------- ");


              // main <triggers>
              if (!this->strct_conv_layer.xTriggers_global.isEmpty())
                this->strct_conv_layer.xTriggers_global.deleteNodeContent();

              this->strct_conv_layer.xTriggers_global = this->strct_conv_layer.xConvDummy.addChild(mxconst::get_ELEMENT_TRIGGERS().c_str());
              Utils::xml_add_comment(this->strct_conv_layer.xConvDummy, " ----------------- ");



              // flag if we have index 0 or not (briefer)
              this->strct_conv_layer.flag_foundBriefer_index0     = Utils::isElementExists(data_manager::map_tableOfParsedFpln, 0); // do we have index 0 (key = 0) in the map ?
              this->strct_conv_layer.flag_refresh_table_from_file = true;

              // make sure we have xLeg IXMLNode for each row
              bool   isNotFirstTime{ false };
              double last_cumulative = 0.0;
              Point  pPrev;
              for (auto& [indx, legData] : data_manager::map_tableOfParsedFpln)
              {
                if (legData.xLeg.isEmpty() || this->strct_conv_layer.flag_load_conversion_file)
                {
                  legData.xFlightPlan = this->strct_conv_layer.xConvDummy.addChild(mxconst::get_ELEMENT_FLIGHT_PLAN().c_str());

                  // Add <leg>
                  if (this->strct_conv_layer.flag_load_conversion_file)
                  {
                    legData.xFlightPlan.addChild(legData.xLeg); // use the <leg> from "saved state" file.

                    // v3.0.303.7 move all loaded <scriptlet> inside <scripts> to flightPlan
                    int iScriptlets = legData.xLoadedScripts.nChildNode(mxconst::get_ELEMENT_SCRIPTLET().c_str());
                    for (int i1 = 0; i1 < iScriptlets; ++i1)
                      legData.xFlightPlan.addChild(legData.xLoadedScripts.getChildNode(mxconst::get_ELEMENT_SCRIPTLET().c_str(), i1)); // move <scriptlets> from <script> to <flight_plan> element
                  }
                  else
                  {
                    legData.xLeg       = legData.xFlightPlan.addChild(mxconst::get_ELEMENT_LEG().c_str());
                    legData.attribName = mxconst::get_ELEMENT_LEG() + Utils::formatNumber<int>(legData.indx);
                    legData.xLeg.updateAttribute(legData.attribName.c_str(), mxconst::get_ATTRIB_NAME().c_str());
                  }

                  // <objectives>
                  legData.xObjectives = legData.xFlightPlan.addChild(mxconst::get_ELEMENT_OBJECTIVES().c_str());

                  // Add <triggers>
                  if (legData.xTriggers.isEmpty()) // v3.0.303.4
                    legData.xTriggers = legData.xFlightPlan.addChild(mxconst::get_ELEMENT_TRIGGERS().c_str());
                  else
                    legData.xFlightPlan.addChild(legData.xTriggers);

                  // Add <message_templates>
                  if (legData.xMessageTmpl.isEmpty()) // v3.0.303.4
                    legData.xMessageTmpl = legData.xFlightPlan.addChild(mxconst::get_ELEMENT_MESSAGE_TEMPLATES ().c_str());
                  else
                    legData.xFlightPlan.addChild(legData.xMessageTmpl);
                }
                // calculate distances
                if (isNotFirstTime)
                {
                  legData.distToPrevWaypoint = Point::calcDistanceBetween2Points(pPrev, legData.p);
                  legData.cumulativeDist     = last_cumulative + legData.distToPrevWaypoint;
                }
                pPrev           = legData.p;
                last_cumulative = legData.cumulativeDist;
                isNotFirstTime  = true;
              }

              this->strct_conv_layer.conv_sub_ui = mx_conv_sub_ui::conv_design_fpln;
            } // end else if map table holds values
          }   // end pressed "read lnvmap flight plan"
        }
        this->mxUiReleaseLastFont(); // v3.303.14
      }
      ImGui::EndGroup();
      // v3.0.301 B4

      // features not supported

    } // conv_pick_fpln
    break;



    case (mx_conv_sub_ui::conv_design_fpln):
    {
      static bool  bRerunRandomDateTime{ false };
      const ImVec2 child_size_vec2 = ImVec2(win_size_vec2.x - 10.0f, 250.0f);

      ImGui::BeginGroup();
      ImGui::BeginChild("TableOfLNM_Waypoints", child_size_vec2, ImGuiChildFlags_Borders);
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
        ImGui::TextColored(missionx::color::color_vec4_bisque, "Picked File: ");
        ImGui::SameLine(0.0f, 2.0f);

        if (this->strct_conv_layer.file_picked_i > -1)
        {
          ImGui::TextColored(missionx::color::color_vec4_green, "%s", this->strct_conv_layer.vecFileList_char.at(this->strct_conv_layer.file_picked_i));
        }
        else
        {
          ImGui::TextColored(missionx::color::color_vec4_green, "%s", mxconst::get_CONVERTER_FILE().c_str());
        }
        this->mxUiReleaseLastFont();


        /////// BUTTONS //////
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14

        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
        if (ImGui::Button("Cancel"))
        {
          this->strct_conv_layer.flag_load_conversion_file = false; // v3.0.303.4  reset "reset conversion state"
          this->strct_conv_layer.conv_sub_ui               = mx_conv_sub_ui::conv_pick_fpln;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(win_size_vec2.x - 230.0f); // draw button from right

        ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
        ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgreen);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
        if (ImGui::Button(">> Generate <<"))
        {
          if (bRerunRandomDateTime) // v3.303.10
            this->execAction(missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);

          if (this->strct_conv_layer.xSavedGlobalSettingsNode.isEmpty())
          {
            this->strct_conv_layer.flag_use_loaded_globalSetting_from_conversion_file = false; // v3.305.1
            this->setMessage("Please wait while generating the mission from the Flight Plan.", 10);
            this->execAction(missionx::mx_window_actions::ACTION_GENERATE_MISSION_FROM_LNM_FPLN);
          }
          else // open popup
          {
            ImGui::OpenPopup(POPUP_PICK_GLOBAL_SETTING_NODE.c_str());
          }
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(); // draw button from right
        ImGui::Checkbox("Store State##conversionScreen", &this->strct_conv_layer.flag_store_state);

        this->mxUiReleaseLastFont();

        // ------------ Little Nav Map Table -----------------------

        subDraw_fpln_table(this->strct_conv_layer.xConvMainNode, missionx::data_manager::map_tableOfParsedFpln);

      } // end table group

      // display POPUP
      draw_conv_popup_which_global_settings_to_save(POPUP_PICK_GLOBAL_SETTING_NODE);


      ImGui::EndChild();
      ImGui::EndGroup();


      // ------------ Bottom Table Buttons -----------------------

      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);

      if (ImGui::Button("Dataref Editor"))
      {
        ImGui::OpenPopup(POPUP_DATAREF_SETTINGS.c_str());
      }

      // v3.305.1 [Global Settings] editor button
      if (!(this->strct_conv_layer.xSavedGlobalSettingsNode.isEmpty()))
      {
        ImGui::SameLine(0.0f, 10.0f);
        if (ImGui::Button("GlobalSettings Editor"))
        {
          ImGui::OpenPopup(POPUP_GLOBAL_SETTINGS.c_str());
        }
      }


      ImGui::PopStyleColor(3);

      this->mxUiReleaseLastFont();

      // v3.303.14 add the advance window that hold: date and weather settings
      ImGui::SameLine(0.0f, 50.0f);
      bRerunRandomDateTime = add_ui_checkbox_rerun_random_date_and_time();
      ImGui::SameLine();
      this->add_ui_advance_settings_random_date_time_weather_and_weight_button2 (this->adv_settings_strct.iClockDayOfYearPicked, this->adv_settings_strct.iClockHourPicked, this->adv_settings_strct.iClockMinutesPicked);

      ImGui::SameLine(0.0f, 25.0f);
      missionx::WinImguiBriefer::add_designer_mode_checkbox(); // v24.03.2


      // display POPUP
      const auto popupHeight_f = ImGui::GetIO().DisplaySize.y * 0.75f;

      const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
      ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowSize(ImVec2(win_size_vec2.x - 20.0f, popupHeight_f));
      ImGui::PushStyleColor(ImGuiCol_PopupBg, missionx::color::color_vec4_blue);
      {
        if (ImGui::BeginPopupModal(POPUP_DATAREF_SETTINGS.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
          draw_conv_popup_datarefs(this->strct_conv_layer.xXPlaneDataRef_global);
          ImGui::EndPopup();
        }

        // v3.305.1 GlobalSettings Popup
        ImGui::SetNextWindowSize(ImVec2(win_size_vec2.x - 20.0f, popupHeight_f));
        if (ImGui::BeginPopupModal(POPUP_GLOBAL_SETTINGS.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
          draw_conv_popup_globalSettings(this->strct_conv_layer.xSavedGlobalSettingsNode);
          ImGui::EndPopup();
        }
      }
      ImGui::PopStyleColor();
    }
    break;

    default:
    {
    }
    break;
  } // end switch between layers

  ImGui::SetWindowFontScale(this->strct_setup_layer.fPreferredFontScale);
}

// -----------------------------------------------

void
WinImguiBriefer::add_landing_rate_ui(const missionx::mx_enrout_stats_strct& inStats)
{
  //  const auto pctTdWeight_f = inStats.fTouchDownWeight / inStats.fTouchDown_acf_m_max;
  ImGui::TextColored(missionx::color::color_vec4_aqua,
                     "Touch Down rate: %.2ffpm(%.2f m/s), AVG 15m Landing rate: %.2ffpm(%.2f m/s) (Hard Landing > 2 m/s)",
                     inStats.fTouchDown_landing_rate_vh_ind_fpm,
                     inStats.fTouchDown_landing_rate_vh_ind_fpm * missionx::fpm2ms,
                     inStats.fLanding_vh_ind_fpm_avg_15_meters,
                     inStats.fLanding_vh_ind_fpm_avg_15_meters * missionx::fpm2ms);

  ImGui::TextColored(missionx::color::color_vec4_chocolate, "Touch Down G: %.2f, AVG last 15m G: %.2f", inStats.fTouchDown_gforce_normal, inStats.fLanding_gforce_normal_avg_15_meters);
}


// -----------------------------------------------

void
WinImguiBriefer::add_ui_stats_child(const bool isEmbedded)
{
  constexpr float fTitleHeight      = 30.0f;
  const auto      strctCurrentStats = missionx::data_manager::gather_stats.get_stats_object();
  const auto      fuel_usage_kg     = missionx::dataref_manager::getDataRefValue<float>("sim/cockpit2/fuel/fuel_totalizer_sum_kg");


  ImGui::BeginGroup();

  if (!isEmbedded)
  {
    ImGui::BeginChild("stats_flight_legs", ImVec2(0.0f, ImGui::GetWindowHeight() - imvec2_flight_info_top_area_size.y - this->fTopToolbarPadding_f - this->fBottomToolbarPadding_f - fTitleHeight), ImGuiChildFlags_Borders);
  }

  if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
  {
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL());

    ImGui::TextWrapped("Overall Distance flew: %10.2f nm\tFuel Consumed: %5.2f", strctCurrentStats.fCumulativeDistance_nm, fuel_usage_kg);
    ImGui::TextWrapped("Current Leg flew: %5.2f nm\tStarting Fuel: %5.2f\tPayload: %6.2f", (strctCurrentStats.fCumulativeDistance_nm - data_manager::strct_currentLegStats4UIDisplay.fCumulativeDistanceFlew_beforeCurrentLeg), data_manager::strct_currentLegStats4UIDisplay.fStartingFuel, data_manager::strct_currentLegStats4UIDisplay.fStartingPayload);

    if (data_manager::strct_currentLegStats4UIDisplay.fTouchDown_landing_rate_vh_ind_fpm != 0.0f)
    {
      const auto pctTdWeight_f = data_manager::strct_currentLegStats4UIDisplay.fTouchDownWeight / data_manager::strct_currentLegStats4UIDisplay.fTouchDown_acf_m_max;
      ImGui::TextColored(missionx::color::color_vec4_white, "Touch Down Weight: %6.2f\tPlane Max Weight: %3.2f\tScore: %3.0f points.", data_manager::strct_currentLegStats4UIDisplay.fTouchDownWeight, data_manager::strct_currentLegStats4UIDisplay.fTouchDown_acf_m_max, pctTdWeight_f * 100.0f);
      add_landing_rate_ui(data_manager::strct_currentLegStats4UIDisplay);
    }
    this->mxUiReleaseLastFont();
    /// display the previous leg stats
    ImGui::NewLine();
    ImGui::Separator();
  }

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
  for (const auto& stats : data_manager::vecPreviousLegStats4UIDisplay)
  {
    const auto pctTdWeight_f = stats.fTouchDownWeight / stats.fTouchDown_acf_m_max;
    ImGui::TextColored(missionx::color::color_vec4_beige, "Waypoint Name: %s", stats.sLegName.c_str());
    ImGui::TextColored(missionx::color::color_vec4_white, "Overall Distance flew: %10.2f nm\tLeg Flew: %5.2f", stats.fCumulativeDistanceFlew_beforeCurrentLeg + stats.fDistanceFlew, stats.fDistanceFlew);
    ImGui::TextColored(missionx::color::color_vec4_grey, "Start Fuel: %5.2f\tEnd Fuel: %5.2f\tFuel Consumed: %5.2f", stats.fStartingFuel, stats.fEndFuel, stats.fStartingFuel - stats.fEndFuel);
    ImGui::TextColored(missionx::color::color_vec4_white, "Start Payload: %6.2f\tEnd Payload: %6.2f", stats.fStartingPayload, stats.fEndPayload);
    ImGui::TextColored(missionx::color::color_vec4_white, "Touch Down Weight: %6.2f\tPlane Max Weight: %3.0f\tScore: %3.0f points.", stats.fTouchDownWeight, stats.fTouchDown_acf_m_max, pctTdWeight_f * 100.0f);

    add_landing_rate_ui(stats);

    ImGui::Separator();
  }
  this->mxUiReleaseLastFont();

  if (!isEmbedded)
    ImGui::EndChild();

  ImGui::EndGroup();
}


// -----------------------------------------------

void
WinImguiBriefer::draw_about_layer()
{
  const auto         win_size_vec2 = this->mxUiGetWindowContentWxH();
  static constexpr float win_padding   = 30.0f;
  // ImGui::BeginChild("draw_about_layer_01", ImVec2(0.0f, ImGui::GetWindowHeight() - this->fBottomToolbarPadding_f - this->fTopToolbarPadding_f - win_padding));
  ImGui::BeginChild("draw_about_layer_01", ImVec2(0.0f, win_size_vec2.y * 0.85f));
  {
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_SMALL());
    {
      ImGui::TextColored ( missionx::color::color_vec4_aqua, "%s%s", "Mission-X v", missionx::FULL_VERSION.c_str () );
      ImGui::TextColored ( missionx::color::color_vec4_greenyellow, "%s", "Mission-X was written by Saar Nagar <snagar.dev@protonmail.com>" );
      ImGui::TextColored ( missionx::color::color_vec4_greenyellow, "%s", "Licensed under AFPL license, see license folder for more detail." );
      ImGui::TextColored ( missionx::color::color_vec4_yellow, "%s", "Library Used");


      this->mxUiSetFont(mxconst::get_TEXT_TYPE_MSG_BOTTOM());
      ImGui::Separator();
      ImGui::TextUnformatted("Uses the IMGUI library v" IMGUI_VERSION ", Copyright (c) 2014-2023 Omar Cornut.");
      ImGui::TextUnformatted("Uses the ImPlot library v" IMPLOT_VERSION ", MIT License - Copyright (c) 2020 Evan Pezent.");
      ImGui::TextUnformatted("Uses the Public (Source) Components from XSquawkBox, Copyright (c) 2018-2020 Christopher Collins.");
      ImGui::TextUnformatted("Uses the CURL transfer library, Copyright (c) 1996 - 2020, Daniel Stenberg.");
      ImGui::TextUnformatted("Uses the IXMLParser library, Copyright (c) 2013, Frank Vanden Berghen - All rights reserved (AFPL License).");
      ImGui::TextUnformatted("Uses the Freetype2 library, Copyright 1996-2002, 2006 by David Turner, Robert Wilhelm, and Werner Lemberg (Freetype License).");
      ImGui::TextUnformatted("Uses polyline-cpp, Copyright (c) 2016 Josh Baker.");
      ImGui::TextUnformatted("Uses the SQLite library (v" SQLITE_VERSION "), Public Domain.");
      ImGui::TextUnformatted("Uses the MY-BASIC library, Copyright (C) 2011 - 2020 Wang Renxin.");
      ImGui::TextUnformatted("Uses the FMOD library, Free Indie License.");
      ImGui::TextUnformatted("Uses the STB public domain libraries, by Sean Barrett.");
      ImGui::TextUnformatted("Uses the FMT library, by Victor Zverovich, License MIT."); // v3.305.3
      ImGui::TextUnformatted(fmt::format("Uses {}, v{}, {}, License MIT.", std::string(nlohmann::json::meta()["name"]), std::string(nlohmann::json::meta()["version"]["string"]), std::string(nlohmann::json::meta()["copyright"])).c_str()); // v3.305.3
      ImGui::Separator();

      ImGui::TextUnformatted("Icons http://www.freepik.com - Designed by Photoroyalty / Freepik.");
      ImGui::TextUnformatted("Font-Awesome https://github.com/FortAwesome/Font-Awesome.");
      ImGui::TextUnformatted("The \"Load Mission\" texture was contributed by @FlightOfImagination - thanks.");
      ImGui::Separator();

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
      ImGui::TextWrapped("I would like to thank all testers and contributors to this plugin I highly appreciate it and this plugin features are probably partially because of your invaluable input.");
      ImGui::PopStyleColor(1);
      ImGui::TextUnformatted("@Daikan, Ptimib, @FlightOfImagination, wolfram and more. Thanks for your cooperation.");
      // Logos
      ImGui::Separator();
      ImGui::Image((void*)static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_FMOD_LOGO()].gTexture), ImVec2(182.0f, 48.0f));
    }
    this->mxUiResetAllFontsToDefault(); // v3.303.14 pop out all pushed fonts

    ImGui::NewLine();
    ImGui::Separator();

    ImGui::TextColored(missionx::color::color_vec4_aqua, "%s", "Used Fonts:");
    for (const auto& [fontType, fontId] : MxUICore::mapFontTypeToFontID)
    {
      if (mxUtils::isElementExists(MxUICore::mapFontTypesBeingUsedInProgram, fontType))
      {
        std::string sText = "Testing 1 2 3 (" + std::string(ImgWindow::sFont1->getAtlas()->Fonts[fontId]->ConfigData->Name) + ")";
        ImGui::PushFont(ImgWindow::sFont1->getAtlas()->Fonts[fontId]);
        ImGui::TextWrapped("[%s] %s", fontType.c_str(), sText.c_str());
        ImGui::PopFont();
      }
    }

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();
    ImGui::TextColored(missionx::color::color_vec4_aqua, "%s", "All Fonts in Memory:");
    for (int i = 0; i < ImgWindow::sFont1->getAtlas()->Fonts.size(); ++i)
    {
      std::string sText = "Testing 1 2 3 (" + std::string(ImgWindow::sFont1->getAtlas()->Fonts[i]->ConfigData->Name) + ")";
      ImGui::PushFont(ImgWindow::sFont1->getAtlas()->Fonts[i]);
      ImGui::TextWrapped("%s", sText.c_str());
      ImGui::PopFont();
    }
  }
  ImGui::EndChild();
}


// -----------------------------------------------


// void
// WinImguiBriefer::add_ui_advance_settings_random_date_time_weather_and_weight_button(int& out_iClockDayOfYearPicked, int& out_iClockHourPicked, int& out_iClockMinutesPicked, const std::string& inTEXT_TYPE)
// {
//   constexpr auto popupRandomize_Weather_DateTime = "set_weather_date_and_time_rules";
//   missionx::WinImguiBriefer::HelpMarker("Configure Preferred Weather, Default Weight and Date/Time.\nYou can disable default weight when flying online and you don't want Mission-X to mess with the weights.");
//   ImGui::SameLine();
//   this->mxUiSetFont(inTEXT_TYPE); // default TEXT_TYPE_TITLE_REG
//   if (ImGui::Button("Advance Settings"))
//   {
//     ImGui::OpenPopup(popupRandomize_Weather_DateTime); // v3.303.10 make it a popup
//   }
//   this->mxUiReleaseLastFont();
//
//   this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Configure Preferred Weather, Default Weight and Date/Time");
//
//   ImGui::SameLine(0.0f, 5.0f);
//   ImGui::TextColored(missionx::color::color_vec4_lightgoldenrodyellow, "Day: %i, hour: %i", out_iClockDayOfYearPicked, out_iClockHourPicked);
//
//
//   //// Randomize Date and Time popup
//   const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
//   ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
//   ImGui::SetNextWindowSize(ImVec2(650.0f, 350.0f));
//   ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_black);
//
//   if (ImGui::BeginPopupModal(popupRandomize_Weather_DateTime, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
//   {
//     static float fRadioPadding = 20.0f;
//     // float        win_width     = mxUiGetContentWidth();
//     float  win_height = ImGui::GetWindowHeight();
//     ImVec2 modal_center(mxUiGetContentWidth() * 0.5f, ImGui::GetWindowHeight() * 0.5f);
//
//     /////////////////////////////////
//     //// Start Tab Child/Group /////
//     ///////////////////////////////
//
//     ImGui::BeginGroup();                                                        // v3.305.1
//     ImGui::BeginChild("##MainAdvancedSettingTabWindow", ImVec2(-5.0f, -35.0f)); // v3.305.1
//
//     // start TABs
//     ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
//     if (ImGui::BeginTabBar("AdvancedWeatherAndTimeSettings", tab_bar_flags))
//     {
//       if (ImGui::BeginTabItem("Date and Time"))
//       {
//         {
//           ImGui::BeginChild("random_date_and_time", ImVec2(-5.0f, 0.0f), ImGuiChildFlags_Borders); // v3.305.1 // The Apply button will be shown after the child and not part of it
//           {
//             // --------- Option Text -----------
//
//             this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
//             ImGui::TextColored(missionx::color::color_vec4_yellow, "How the engine should pick the date and time ?");
//             this->mxUiReleaseLastFont();
//
//             this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
//             ImGui::TextWrapped("Pick X-Plane\nDate/Time");
//             ImGui::SameLine(0.0f, 40.0f);
//             ImGui::TextWrapped("Pick OS\nDate/Time");
//             ImGui::SameLine(0.0f, 40.0f);
//             ImGui::TextWrapped("Pick Any Time");
//             ImGui::SameLine(0.0f, 40.0f);
//             ImGui::TextWrapped("Pick Exact\nDate/Time");
//             ImGui::SameLine(0.0f, 40.0f);
//             ImGui::TextWrapped("Pick Preferred\nMonths/Time");
//             ImGui::NewLine();
//             this->mxUiReleaseLastFont();
//
//             // --------- Option Radio Buttons -----------
//             ImGui::SameLine(fRadioPadding, 0.0f);
//
//             for (const auto &[type, label, toolTip] : this->listRandomCalendarRadioLabel)
//             {
//               if (ImGui::RadioButton(label.c_str(), type == this->adv_settings_strct.iRadioRandomDateTime_pick))
//               {
//                 this->adv_settings_strct.iRadioRandomDateTime_pick = type;
//
//                 switch (this->adv_settings_strct.iRadioRandomDateTime_pick)
//                 {
//                   case missionx::mx_ui_random_date_time_type::current_day_and_time:
//                   {
//                     out_iClockDayOfYearPicked = dataref_manager::getLocalDateDays(); // strct_user_create_layer.iClockDayOfYearPicked
//                     out_iClockHourPicked      = dataref_manager::getLocalHour();     // strct_user_create_layer.iClockHourPicked
//                     out_iClockMinutesPicked   = dataref_manager::getLocalMinutes (); // v25.04.2 How many minutes passed since the start of the hour
//                   }
//                   break;
//                   case missionx::mx_ui_random_date_time_type::os_day_and_time:
//                   {
//                     missionx::mx_clock_time_strct osClock = Utils::get_os_time();
//                     out_iClockDayOfYearPicked             = osClock.dayInYear;
//                     out_iClockHourPicked                  = osClock.hour;
//                     out_iClockMinutesPicked               = mxUtils::calc_minutes_from_seconds (osClock.seconds_in_day); // v25.04.2 How many minutes passed since the start of the hour
//                   }
//                   break;
//                   case missionx::mx_ui_random_date_time_type::any_day_time:
//                   {
//                     this->execAction(missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);
//                   }
//                   break;
//                   default: // the two other options need more input from user
//                     break;
//                 } // end internal radio switch
//               }
//               this->mx_add_tooltip(missionx::color::color_vec4_yellow, toolTip);
//
//               fRadioPadding += 110.0f;
//               ImGui::SameLine(fRadioPadding, 0.0f);
//             }
//             fRadioPadding = 20.0; // reset to start of line
//
//             // --------- Draw User Options  -----------
//             ImGui::NewLine();
//             ImGui::Separator();
//             switch (this->adv_settings_strct.iRadioRandomDateTime_pick)
//             {
//               case missionx::mx_ui_random_date_time_type::any_day_time:
//               {
//                 if (ImGui::Checkbox("##checkIncludeNightHours", &this->adv_settings_strct.flag_includeNightHours))
//                 {
//                   // this->strct_user_create_layer.flag_includeNightHours ^= 1;
//                   this->execAction(missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);
//                 }
//                 ImGui::SameLine(0.0f, 2.0f);
//                 this->mxUiSetFont(mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1());
//                 ImGui::TextColored(missionx::color::color_vec4_burlywood, "Include night hours.");
//                 this->mxUiReleaseLastFont();
//               }
//               break;
//               case missionx::mx_ui_random_date_time_type::exact_day_and_time:
//               {
//
//                 if (this->IsInVR())
//                 {
//                   ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
//                   HelpMarker("Reset to current Day and Hour");
//                   ImGui::PopStyleColor(1);
//                   ImGui::SameLine();
//                 }
//
//                 this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
//                 if (ImgWindow::ButtonTooltip(mxUtils::from_u8string(ICON_FA_SYNC).append( "##syncDayInYearAndHours" ).c_str(), "Reset to local day and hour"))
//                 {
//                   out_iClockDayOfYearPicked = dataref_manager::getLocalDateDays(); // strct_user_create_layer.iClockDayOfYearPicked
//                   out_iClockHourPicked      = dataref_manager::getLocalHour();     // strct_user_create_layer.iClockHourPicked
//                 }
//                 this->mxUiReleaseLastFont();
//
//                 ImGui::SameLine();
//                 ImGui::TextColored(missionx::color::color_vec4_yellow, "Day:");
//
//                 ImGui::SameLine();
//                 ImGui::SetNextItemWidth(80.0f);
//                 ImGui::Combo("##DayOfYear", &out_iClockDayOfYearPicked, this->clockDayOfYear_arr, IM_ARRAYSIZE(this->clockDayOfYear_arr));
//
//                 ImGui::SameLine(0.0f, 10.0f);
//                 ImGui::TextColored(missionx::color::color_vec4_yellow, "HH:mm");
//                 ImGui::SameLine();
//
//                 ImGui::SetNextItemWidth(50.0f);
//                 ImGui::Combo("##StartHours", &out_iClockHourPicked, this->clockHours_arr, IM_ARRAYSIZE(this->clockHours_arr));
//                 ImGui::SameLine();
//                 ImGui::TextColored(missionx::color::color_vec4_yellow, ":");
//                 ImGui::SameLine();
//                 ImGui::SetNextItemWidth(50.0f);
//                 ImGui::Combo("##StartMinutes", &out_iClockMinutesPicked, this->clockMinutes_arr, IM_ARRAYSIZE(this->clockMinutes_arr));
//               }
//               break;
//               case missionx::mx_ui_random_date_time_type::pick_months_and_part_of_preferred_day:
//               {
//
//                 ImGui::BeginGroup();
//                 {
//                   ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellowgreen);
//                   ImGui::Checkbox("Pick Any Month", &this->adv_settings_strct.flag_checkAnyMonth);
//                   ImGui::PopStyleColor();
//
//
//                   for (int y = 0; y < 3; y++)
//                     for (int x = 0; x < 4; x++)
//                     {
//                       if (x > 0)
//                         ImGui::SameLine();
//                       ImGui::PushID(y * 4 + x);
//                       if (ImGui::Selectable(this->adv_settings_strct.selected_lbl[y][x].c_str(), this->adv_settings_strct.selected_dateTime_by_user_arr[y][x] != 0, (this->adv_settings_strct.flag_checkAnyMonth) ? ImGuiSelectableFlags_Disabled : 0, ImVec2(30.0f, 30.0f)))
//                       {
//                         // Toggle clicked cell
//                         this->adv_settings_strct.selected_dateTime_by_user_arr[y][x] ^= 1;
//                       }
//                       ImGui::PopID();
//                     } // end X loop
//
//                   ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_green);
//                   this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
//                   if (ImGui::Button("Pick Random Day/Time"))
//                   {
//                     this->execAction(missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);
//                   }                            // end push button
//                   this->mxUiReleaseLastFont(); // v3.303.14
//                   ImGui::PopStyleColor();
//                 }
//                 ImGui::EndGroup();
//
//
//                 ImGui::SameLine(0.0f, 60.0f);
//
//                 // Time of day
//                 this->mxUiSetFont(mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1()); // v3.303.14
//                 ImGui::BeginGroup();
//                 {
//                   ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellowgreen);
//                   ImGui::Checkbox("Pick Any Hour", &this->adv_settings_strct.checkPartOfDay_b);
//                   ImGui::PopStyleColor();
//
//                   if (ImGui::BeginTable("Pick Any Hour Table", 2, ImGuiTableFlags_Borders)) // ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
//                   {
//                     for (int y = 0; y < 4; y++)
//                       for (int x = 0; x < 2; x++)
//                       {
//                         ImGui::TableNextColumn();
//
//                         ImGui::PushID(y * 4 + x);
//
//                         if (ImGui::Selectable(this->adv_settings_strct.selected_time_lbl[y][x].c_str(), this->adv_settings_strct.selectedTime[y][x] != 0, (this->adv_settings_strct.checkPartOfDay_b) ? ImGuiSelectableFlags_Disabled : 0, ImVec2(200.0f, 25.0f)))
//                         {
//                           // Toggle clicked cell
//                           this->adv_settings_strct.selectedTime[y][x] ^= 1; // I think this means shift 1
//                         }
//                         ImGui::PopID();
//                       }
//                     ImGui::EndTable();
//                   } // end time in day table
//                 }
//                 ImGui::EndGroup();
//                 this->mxUiReleaseLastFont(); // v3.303.14
//
//               } // pick_months_and_part_of_preferred_day
//               break;
//               default:
//                 break;
//             }
//           }
//           ImGui::EndChild();
//         }
//         ImGui::EndTabItem();
//       }
//       if (ImGui::BeginTabItem("Weather Settings"))
//       {
//         { // begin tab
//           ImGui::BeginChild("random_weather_tab", ImVec2(-5.0f, 0.0f), ImGuiChildFlags_Borders); // The Apply button will be shown after the child and not part of it
//           {
//             // --------- Weather Header Text -----------
//             this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
//             ImGui::TextColored(missionx::color::color_vec4_yellow, "How do you want to setup the weather ?");
//             this->mxUiReleaseLastFont();
//
//             ImGui::NewLine();
//
//             fRadioPadding = 20.0f;
//             ImGui::SameLine(fRadioPadding, 0.0f);
//
//             this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.303.14
//             for (const auto& radio : this->listRandomWeatherRadioLabel)
//             {
//               if (ImGui::RadioButton(radio.label.c_str(), radio.type == this->adv_settings_strct.iWeatherType_user_picked))
//               {
//                 this->adv_settings_strct.iWeatherType_user_picked = radio.type;
//                 if (radio.type == missionx::mx_ui_random_weather_options::pick_pre_defined)
//                 {
//                   // reset weather mode change to default static
//                   for (int y = 0; y < missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_mode_y; y++)
//                     for (int x = 0; x < missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_mode_x; x++)
//                     {
//                       if (y == missionx::WinImguiBriefer::mx_popup_adv_settings_strct::DEFAULT_WEATHER_MODE_Y && x == missionx::WinImguiBriefer::mx_popup_adv_settings_strct::DEFAULT_WEATHER_MODE_X)
//                         this->adv_settings_strct.selected_weather_mode_by_user_arr_0_1_xp12[y][x] = 1; // static is highlighted
//                       else
//                         this->adv_settings_strct.selected_weather_mode_by_user_arr_0_1_xp12[y][x] = 0; // not picked
//                     }
//                 }
//               }
//
//               this->mx_add_tooltip(missionx::color::color_vec4_yellow, radio.toolTip);
//
//               ImGui::SameLine(0.0f, fRadioPadding);
//             } // end loop over all radio options
//
//             this->mxUiReleaseLastFont();
//
//             fRadioPadding = 20.0; // reset to start of line
//
//             ImGui::NewLine();
//             ImGui::Separator();
//             switch (this->adv_settings_strct.iWeatherType_user_picked)
//             {
//               case missionx::mx_ui_random_weather_options::pick_pre_defined:
//               {
//                 // decide which array to pick
//                 this->mxUiSetFont(mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1()); // v3.303.14
//                 ImGui::BeginGroup();
//                 {
//                   if (ImGui::BeginTable("Pick Custom Weather Type Table", missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_x, ImGuiTableFlags_Borders)) // ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
//                   {
//                     for (int y = 0; y < missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_y; y++)
//                       for (int x = 0; x < missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_x; x++)
//                       {
//                         ImGui::TableNextColumn();
//
//                         ImGui::PushID(y * missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_x + x);
//                         if (ImGui::Selectable((*this->adv_settings_strct.ptr_selected_weather_lbl)[y][x].c_str(), (*this->adv_settings_strct.ptr_selected_weather_by_user_arr)[y][x] != 0, 0, ImVec2(95.0f, 25.0f))) //(this->flag_pickAnyWeatherType) ? ImGuiSelectableFlags_Disabled : 0, ImVec2(85.0f, 25.0f)))
//                         {
//                           // Toggle clicked cell
//                           if ((*this->adv_settings_strct.ptr_selected_weather_code)[y][x] >= 0)
//                             (*this->adv_settings_strct.ptr_selected_weather_by_user_arr)[y][x] ^= 1;
//
//                           // Log::logMsg("Clicked: " + this->adv_settings_strct.selected_weather_lbl_xp12[y][x]); // debug
//                         }
//                         ImGui::PopID();
//                       } // end X loop over weather array and // end Y loop over weather array
//
//                     ImGui::EndTable();
//                   } // end table
//                 }   // end ui group
//                 ImGui::EndGroup();
//                 this->mxUiReleaseLastFont(); // v3.303.14
//
//               }
//               break;
//               default:
//                 break;
//
//             } // switch
//
//           } // end child
//           ImGui::EndChild();
//
//         }                    // end tab
//         ImGui::EndTabItem(); // end weather TAB
//       }
//       if (ImGui::BeginTabItem("Weight Settings"))
//       {
//
//         { // begin tab
//           // ImGui::BeginChild("weight_tab", ImVec2(win_width - 10.0f, win_height - 95.0f), true);
//           ImGui::BeginChild("weight_tab", ImVec2(-5.0f, 0.0f), ImGuiChildFlags_Borders); // v3.305.1
//           {
//             this->mxUiSetFont(mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1()); // v3.303.14
//             this->add_ui_xp11_comp_checkbox ( false );
//             ImGui::Separator ();
//             ImGui::NewLine ();
//             this->add_default_weight_ui (); // v25.02.1
//             this->mxUiReleaseLastFont();
//           }
//           ImGui::EndChild();
//         }
//         ImGui::EndTabItem(); // end weather TAB
//       }
//       ImGui::EndTabBar();
//     }
//
//     ImGui::EndChild(); // v3.305.1
//     ImGui::EndGroup(); // v3.305.1
//
//
//     // --------- BUTTONS -----------
//     ImGui::Spacing();
//     ImGui::SameLine(modal_center.x * 0.75f);
//     ImGui::SetCursorPosY(win_height - 35.0f);
//     ImGui::BeginGroup();
//
//     this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
//     if (ImGui::Button("Apply & Close##AdvSettingsApplyAndClose", ImVec2(150, 0)))
//     {
//       ImGui::CloseCurrentPopup();
//     }
//     this->mxUiReleaseLastFont();
//
//     ImGui::SameLine(0.0f, 20.0f);
//     ImGui::SetCursorPosY(ImGui::GetCursorPos().y + 5.0f);
//     ImGui::TextColored(missionx::color::color_vec4_lightgoldenrodyellow, "Picked Day: %i, %i:%i", this->adv_settings_strct.iClockDayOfYearPicked, this->adv_settings_strct.iClockHourPicked, this->adv_settings_strct.iClockMinutesPicked);
//     this->mx_add_tooltip(missionx::color::color_vec4_lightyellow, "Day of Year and Hour");
//     ImGui::EndGroup();
//
//     ////// End Popup //////
//     ImGui::EndPopup();
//   } // end randomize date and time popup}
//
//   ImGui::PopStyleColor(1); // end popup background color
//
//
// } // end add_ui_advance_settings_random_date_time_weather_and_weight_button function

// -----------------------------------------------

void
WinImguiBriefer::add_ui_advance_settings_random_date_time_weather_and_weight_button2(int& out_iClockDayOfYearPicked, int& out_iClockHourPicked, int& out_iClockMinutesPicked, const std::string& inTEXT_TYPE)
{
  constexpr auto popupRandomize_Weather_DateTime = "set_weather_date_and_time_rules";
  missionx::WinImguiBriefer::HelpMarker("Configure Preferred Weather, Default Weight and Date/Time.\nYou can disable default weight when flying online and you don't want Mission-X to mess with the weights.");
  ImGui::SameLine();
  this->mxUiSetFont(inTEXT_TYPE); // default TEXT_TYPE_TITLE_REG
  if (ImGui::Button("Advance Settings"))
  {
    ImGui::OpenPopup(popupRandomize_Weather_DateTime); // v3.303.10 make it a popup
  }
  this->mxUiReleaseLastFont();

  this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Configure Preferred Weather, Default Weight and Date/Time");

  // Display the Date + Time adjacent the button
  ImGui::SameLine(0.0f, 5.0f);
  ImGui::TextColored(missionx::color::color_vec4_lightgoldenrodyellow, "Day: %i, %i:%i", out_iClockDayOfYearPicked, out_iClockHourPicked, out_iClockMinutesPicked);


  //// Randomize Date and Time popup
  const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(650.0f, 350.0f));
  ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_black);

  if (ImGui::BeginPopupModal(popupRandomize_Weather_DateTime, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
  {
    static float fRadioPadding = 20.0f;
    // float        win_width     = mxUiGetContentWidth();
    const float  win_height = ImGui::GetWindowHeight();
    const ImVec2 modal_center(mxUiGetContentWidth() * 0.5f, ImGui::GetWindowHeight() * 0.5f);

    /////////////////////////////////
    //// Start Tab Child/Group /////
    ///////////////////////////////

    ImGui::BeginGroup();                                                        // v3.305.1
    ImGui::BeginChild("##MainAdvancedSettingTabWindow", ImVec2(-5.0f, -35.0f)); // v3.305.1

    // start TABs
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("AdvancedWeatherAndTimeSettings", tab_bar_flags))
    {
      if (ImGui::BeginTabItem("Date and Time"))
      {
        {
          ImGui::BeginChild("random_date_and_time", ImVec2(-5.0f, 0.0f), ImGuiChildFlags_Borders); // v3.305.1 // The Apply button will be shown after the child and not part of it
          {
            // --------- Option Text -----------

            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
            ImGui::TextColored(missionx::color::color_vec4_yellow, "How the engine should pick the date and time ?");
            this->mxUiReleaseLastFont();

            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG());
            ImGui::TextWrapped("Pick X-Plane\nDate/Time");
            ImGui::SameLine(0.0f, 40.0f);
            ImGui::TextWrapped("Pick OS\nDate/Time");
            ImGui::SameLine(0.0f, 40.0f);
            ImGui::TextWrapped("Pick Any Time");
            ImGui::SameLine(0.0f, 40.0f);
            ImGui::TextWrapped("Pick Exact\nDate/Time");
            ImGui::SameLine(0.0f, 40.0f);
            ImGui::TextWrapped("Pick Preferred\nMonths/Time");
            ImGui::NewLine();
            this->mxUiReleaseLastFont();

            // --------- Option Radio Buttons -----------
            ImGui::SameLine(fRadioPadding, 0.0f);

            for (const auto &[type, label, toolTip] : this->listRandomCalendarRadioLabel)
            {
              if (ImGui::RadioButton(label.c_str(), type == this->adv_settings_strct.iRadioRandomDateTime_pick))
              {
                this->adv_settings_strct.iRadioRandomDateTime_pick = type;

                switch (this->adv_settings_strct.iRadioRandomDateTime_pick)
                {
                  case missionx::mx_ui_random_date_time_type::xplane_day_and_time:
                  {
                    out_iClockDayOfYearPicked = dataref_manager::getLocalDateDays(); // strct_user_create_layer.iClockDayOfYearPicked
                    out_iClockHourPicked      = dataref_manager::getLocalHour();     // strct_user_create_layer.iClockHourPicked
                    out_iClockMinutesPicked   = dataref_manager::getLocalMinutes (); // v25.04.2 How many minutes passed since the start of the hour
                  }
                  break;
                  case missionx::mx_ui_random_date_time_type::os_day_and_time:
                  {
                    missionx::mx_clock_time_strct osClock = Utils::get_os_time();
                    out_iClockDayOfYearPicked             = osClock.dayInYear;
                    out_iClockHourPicked                  = osClock.hour;
                    out_iClockMinutesPicked               = osClock.minutes; // v25.04.2 How many minutes passed since the start of the hour
                  }
                  break;
                  case missionx::mx_ui_random_date_time_type::any_day_time:
                  {
                    this->execAction(missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);
                  }
                  break;
                  default: // the two other options need more input from user
                    break;
                } // end internal radio switch
              }
              this->mx_add_tooltip(missionx::color::color_vec4_yellow, toolTip);

              fRadioPadding += 110.0f;
              ImGui::SameLine(fRadioPadding, 0.0f);
            }
            fRadioPadding = 20.0; // reset to start of line

            // --------- Draw User Options  -----------
            ImGui::NewLine();
            ImGui::Separator();
            switch (this->adv_settings_strct.iRadioRandomDateTime_pick)
            {
              case missionx::mx_ui_random_date_time_type::any_day_time:
              {
                if (ImGui::Checkbox("##checkIncludeNightHours", &this->adv_settings_strct.flag_includeNightHours))
                {
                  this->execAction(missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);
                }
                ImGui::SameLine(0.0f, 2.0f);
                this->mxUiSetFont(mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1());
                ImGui::TextColored(missionx::color::color_vec4_burlywood, "Include night hours.");
                this->mxUiReleaseLastFont();
              }
              break;
              case missionx::mx_ui_random_date_time_type::exact_day_and_time:
              {

                if (this->IsInVR())
                {
                  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
                  HelpMarker("Reset to current Day and Hour");
                  ImGui::PopStyleColor(1);
                  ImGui::SameLine();
                }

                this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
                if (ImgWindow::ButtonTooltip(mxUtils::from_u8string(ICON_FA_SYNC).append( "##syncDayInYearAndHours" ).c_str(), "Reset to local day and hour"))
                {
                  out_iClockDayOfYearPicked = dataref_manager::getLocalDateDays(); // strct_user_create_layer.iClockDayOfYearPicked
                  out_iClockHourPicked      = dataref_manager::getLocalHour();     // strct_user_create_layer.iClockHourPicked
                  out_iClockMinutesPicked   = dataref_manager::getLocalMinutes (); // v25.04.2 How many minutes passed since the start of the hour
                }
                this->mxUiReleaseLastFont();

                ImGui::SameLine();
                ImGui::TextColored(missionx::color::color_vec4_yellow, "Day:");

                ImGui::SameLine();
                ImGui::SetNextItemWidth(80.0f);
                ImGui::Combo("##DayOfYear", &out_iClockDayOfYearPicked, this->clockDayOfYear_arr, IM_ARRAYSIZE(this->clockDayOfYear_arr));

                ImGui::SameLine(0.0f, 10.0f);
                ImGui::TextColored(missionx::color::color_vec4_yellow, "HH:mm");
                ImGui::SameLine();

                ImGui::SetNextItemWidth(50.0f);
                ImGui::Combo("##StartHours", &out_iClockHourPicked, this->clockHours_arr, IM_ARRAYSIZE(this->clockHours_arr));
                ImGui::SameLine();
                ImGui::TextColored(missionx::color::color_vec4_yellow, ":");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(50.0f);
                static int st_minutes_picked = -1; // v25.04.2 added static variable
                ImGui::Combo("##StartMinutes", &st_minutes_picked, this->clockMinutes_arr, IM_ARRAYSIZE(this->clockMinutes_arr));
                if (st_minutes_picked >= 0)
                  out_iClockMinutesPicked = st_minutes_picked * 5;
              }
              break;
              case missionx::mx_ui_random_date_time_type::pick_months_and_part_of_preferred_day:
              {

                ImGui::BeginGroup();
                {
                  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellowgreen);
                  ImGui::Checkbox("Pick Any Month", &this->adv_settings_strct.flag_checkAnyMonth);
                  ImGui::PopStyleColor();


                  for (int y = 0; y < 3; y++)
                    for (int x = 0; x < 4; x++)
                    {
                      if (x > 0)
                        ImGui::SameLine();
                      ImGui::PushID(y * 4 + x);
                      if (ImGui::Selectable(this->adv_settings_strct.selected_lbl[y][x].c_str(), this->adv_settings_strct.selected_dateTime_by_user_arr[y][x] != 0, (this->adv_settings_strct.flag_checkAnyMonth) ? ImGuiSelectableFlags_Disabled : 0, ImVec2(30.0f, 30.0f)))
                      {
                        // Toggle clicked cell
                        this->adv_settings_strct.selected_dateTime_by_user_arr[y][x] ^= 1;
                      }
                      ImGui::PopID();
                    } // end X loop

                  ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_green);
                  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
                  if (ImGui::Button("Pick Random Day/Time"))
                  {
                    this->execAction(missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);
                  }                            // end push button
                  this->mxUiReleaseLastFont(); // v3.303.14
                  ImGui::PopStyleColor();
                }
                ImGui::EndGroup();


                ImGui::SameLine(0.0f, 60.0f);

                // Time of day
                this->mxUiSetFont(mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1()); // v3.303.14
                ImGui::BeginGroup();
                {
                  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellowgreen);
                  ImGui::Checkbox("Pick Any Hour", &this->adv_settings_strct.checkPartOfDay_b);
                  ImGui::PopStyleColor();

                  if (ImGui::BeginTable("Pick Any Hour Table", 2, ImGuiTableFlags_Borders)) // ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
                  {
                    for (int y = 0; y < 4; y++)
                      for (int x = 0; x < 2; x++)
                      {
                        ImGui::TableNextColumn();

                        ImGui::PushID(y * 4 + x);

                        if (ImGui::Selectable(this->adv_settings_strct.selected_time_lbl[y][x].c_str(), this->adv_settings_strct.selectedTime[y][x] != 0, (this->adv_settings_strct.checkPartOfDay_b) ? ImGuiSelectableFlags_Disabled : 0, ImVec2(200.0f, 25.0f)))
                        {
                          // Toggle clicked cell
                          this->adv_settings_strct.selectedTime[y][x] ^= 1; // I think this means shift 1
                        }
                        ImGui::PopID();
                      }
                    ImGui::EndTable();
                  } // end time in day table
                }
                ImGui::EndGroup();
                this->mxUiReleaseLastFont(); // v3.303.14

              } // pick_months_and_part_of_preferred_day
              break;
              default:
                break;
            }
          }
          ImGui::EndChild();
        }
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Weather Settings"))
      {
        { // begin tab
          ImGui::BeginChild("random_weather_tab", ImVec2(-5.0f, 0.0f), ImGuiChildFlags_Borders); // The Apply button will be shown after the child and not part of it
          {
            // --------- Weather Header Text -----------
            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
            ImGui::TextColored(missionx::color::color_vec4_yellow, "How do you want to setup the weather ?");
            this->mxUiReleaseLastFont();

            ImGui::NewLine();

            fRadioPadding = 20.0f;
            ImGui::SameLine(fRadioPadding, 0.0f);

            this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.303.14
            for (const auto& radio : this->listRandomWeatherRadioLabel)
            {
              if (ImGui::RadioButton(radio.label.c_str(), radio.type == this->adv_settings_strct.iWeatherType_user_picked))
              {
                this->adv_settings_strct.iWeatherType_user_picked = radio.type;
                if (radio.type == missionx::mx_ui_random_weather_options::pick_pre_defined)
                {
                  // reset weather mode change to default static
                  for (int y = 0; y < missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_mode_y; y++)
                    for (int x = 0; x < missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_mode_x; x++)
                    {
                      if (y == missionx::WinImguiBriefer::mx_popup_adv_settings_strct::DEFAULT_WEATHER_MODE_Y && x == missionx::WinImguiBriefer::mx_popup_adv_settings_strct::DEFAULT_WEATHER_MODE_X)
                        this->adv_settings_strct.selected_weather_mode_by_user_arr_0_1_xp12[y][x] = 1; // static is highlighted
                      else
                        this->adv_settings_strct.selected_weather_mode_by_user_arr_0_1_xp12[y][x] = 0; // not picked
                    }
                }
              }

              this->mx_add_tooltip(missionx::color::color_vec4_yellow, radio.toolTip);

              ImGui::SameLine(0.0f, fRadioPadding);
            } // end loop over all radio options

            this->mxUiReleaseLastFont();

            fRadioPadding = 20.0; // reset to start of line

            ImGui::NewLine();
            ImGui::Separator();
            switch (this->adv_settings_strct.iWeatherType_user_picked)
            {
              case missionx::mx_ui_random_weather_options::pick_pre_defined:
              {
                // decide which array to pick
                this->mxUiSetFont(mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1()); // v3.303.14
                ImGui::BeginGroup();
                {
                  if (ImGui::BeginTable("Pick Custom Weather Type Table", missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_x, ImGuiTableFlags_Borders)) // ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
                  {
                    for (int y = 0; y < missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_y; y++)
                      for (int x = 0; x < missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_x; x++)
                      {
                        ImGui::TableNextColumn();

                        ImGui::PushID(y * missionx::WinImguiBriefer::mx_popup_adv_settings_strct::weather_x + x);
                        if (ImGui::Selectable((*this->adv_settings_strct.ptr_selected_weather_lbl)[y][x].c_str(), (*this->adv_settings_strct.ptr_selected_weather_by_user_arr)[y][x] != 0, 0, ImVec2(95.0f, 25.0f))) //(this->flag_pickAnyWeatherType) ? ImGuiSelectableFlags_Disabled : 0, ImVec2(85.0f, 25.0f)))
                        {
                          // Toggle clicked cell
                          if ((*this->adv_settings_strct.ptr_selected_weather_code)[y][x] >= 0)
                            (*this->adv_settings_strct.ptr_selected_weather_by_user_arr)[y][x] ^= 1;

                          // Log::logMsg("Clicked: " + this->adv_settings_strct.selected_weather_lbl_xp12[y][x]); // debug
                        }
                        ImGui::PopID();
                      } // end X loop over weather array and // end Y loop over weather array

                    ImGui::EndTable();
                  } // end table
                }   // end ui group
                ImGui::EndGroup();
                this->mxUiReleaseLastFont(); // v3.303.14

              }
              break;
              default:
                break;

            } // switch

          } // end child
          ImGui::EndChild();

        }                    // end tab
        ImGui::EndTabItem(); // end weather TAB
      }
      if (ImGui::BeginTabItem("Weight Settings"))
      {

        { // begin tab
          // ImGui::BeginChild("weight_tab", ImVec2(win_width - 10.0f, win_height - 95.0f), true);
          ImGui::BeginChild("weight_tab", ImVec2(-5.0f, 0.0f), ImGuiChildFlags_Borders); // v3.305.1
          {
            this->mxUiSetFont(mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1()); // v3.303.14
            this->add_ui_xp11_comp_checkbox ( false );
            ImGui::Separator ();
            ImGui::NewLine ();
            this->add_ui_default_weights (); // v25.02.1
            this->mxUiReleaseLastFont();
          }
          ImGui::EndChild();
        }
        ImGui::EndTabItem(); // end weather TAB
      }
      ImGui::EndTabBar();
    }

    ImGui::EndChild(); // v3.305.1
    ImGui::EndGroup(); // v3.305.1


    // --------- BUTTONS -----------
    ImGui::Spacing();
    ImGui::SameLine(modal_center.x * 0.75f);
    ImGui::SetCursorPosY(win_height - 35.0f);
    ImGui::BeginGroup();

    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.303.14
    if (ImGui::Button("Apply & Close##AdvSettingsApplyAndClose", ImVec2(150, 0)))
    {
      ImGui::CloseCurrentPopup();
    }
    this->mxUiReleaseLastFont();

    ImGui::SameLine(0.0f, 20.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPos().y + 5.0f);
    ImGui::TextColored(missionx::color::color_vec4_lightgoldenrodyellow, "Picked Day: %i, %i:%i", out_iClockDayOfYearPicked, out_iClockHourPicked, out_iClockMinutesPicked);
    this->mx_add_tooltip(missionx::color::color_vec4_lightyellow, "Day of Year and Hour");
    ImGui::EndGroup();

    ////// End Popup //////
    ImGui::EndPopup();
  } // end randomize date and time popup}

  ImGui::PopStyleColor(1); // end popup background color


} // end add_ui_advance_settings_random_date_time_weather_and_weight_button function

// -----------------------------------------------

bool
WinImguiBriefer::add_ui_checkbox_rerun_random_date_and_time()
{
  // Regenerate Random Date Time // v3.303.10
  static bool              bRerunRandomDateTime{ false };
  static const std::string tip_text = "Pick a new Date every time you Generate a Mission.\n(Depends on your \"Date Time rules settings\" - default is current date/time)";
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellowgreen); //
  HelpMarker(tip_text.c_str());
  ImGui::PopStyleColor(1);
  ImGui::SameLine(0.0f, 2.0f);
  ImGui::Checkbox("##RerunRandomDateTime", &bRerunRandomDateTime);
  this->mx_add_tooltip(missionx::color::color_vec4_yellowgreen, tip_text.c_str());

  return bRerunRandomDateTime;
}

// -----------------------------------------------

void
WinImguiBriefer::display_shared_message_when_optimized_data_is_not_present(missionx::mx_layer_state_enum in_state)
{
  switch (in_state)
  {
    case missionx::mx_layer_state_enum::failed_data_is_not_present:
    {
      {
        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_BIG()); // v3.303.14
        ImGui::TextColored(missionx::color::color_vec4_magenta, "There was an error while validating data ");

        this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED()); // v3.303.14
        ImGui::TextColored(missionx::color::color_vec4_yellow, "Suggestion: re-run apt.dat optimization and set database initialization too (found in the plugin setup page)");
        ImGui::NewLine();
        ImGui::TextColored(missionx::color::color_vec4_lightgreen, "If you want to automate this process, please click the button ");
        ImGui::SameLine();
        if (ImGui::SmallButton("Run Auto Optimization"))
        {
          if (missionx::data_manager::flag_apt_dat_optimization_is_running || OptimizeAptDat::aptState.flagIsActive)
          {
            this->setMessage("Apt.DAT is running.... please wait.");
          }
          else
          {
            missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::exec_apt_dat_optimization); // v3.0.253.6
          }
        } // end run Auto Optimization

        ImGui::TextColored(missionx::color::color_vec4_lightgreen, "it might take you back to home screen");
        ImGui::NewLine();
        ImGui::NewLine();
        ImGui::NewLine();

        this->mxUiReleaseLastFont(2); // v3.303.14 release the last 2 fonts we pushed
      }                               // end failed_data_is_not_present
    }                                 // end case
    break;
    case missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly:
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_BIG()); // v3.303.14
      ImGui::TextColored(missionx::color::color_vec4_magenta, "FATAL ERROR - Plugin failed to access airports database file ");
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED()); // v3.303.14

      ImGui::TextColored(missionx::color::color_vec4_yellow, "Suggestion: delete the database file in {missionx/db} plugin folder and reload X-Plane.");
      ImGui::TextColored(missionx::color::color_vec4_yellow, R"(Once loaded, enable the "Create cache SQLite DB format" option in the "setup" screen, then re-run "apt.dat" optimization.)");

      ImGui::NewLine();
      ImGui::NewLine();
      ImGui::NewLine();

      this->mxUiReleaseLastFont(2); // v3.303.14 release the last 2 fonts we pushed
    }
    break;
    default:
      break;
  } // end switch
} // end display_shared_message_when_optimized_data_is_not_present


// -----------------------------------------------


void
WinImguiBriefer::add_missing_3d_files_message()
{
  if (missionx::data_manager::get_are_there_missing_3D_object_files())
  {

    ImGui::Image((void*)static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_WARN_SMALL_32x28()].gTexture), ImVec2(16.0f, 14.0f));
    ImGui::SameLine();
    ImGui::Text("There might be missing 3D files. Check missionx.log file for more information.");
  }
}

// -----------------------------------------------


void
WinImguiBriefer::subDraw_ui_xRadius(IXMLNode& node, int pad_x_i)
{
  int val_i = Utils::readNodeNumericAttrib<int>(node, mxconst::get_ATTRIB_LENGTH_MT(), missionx::MIN_RAD_UI_VALUE_MT);

  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - pad_x_i);

  ImGui::SetNextItemWidth(120.0f);
  if (ImGui::InputInt("##TrigRadius", &val_i, 100))
  {
    if (val_i < missionx::MIN_RAD_UI_VALUE_MT)
      val_i = missionx::MIN_RAD_UI_VALUE_MT; // validate value
    if (val_i > missionx::MAX_RAD_UI_VALUE_MT)
      val_i = missionx::MAX_RAD_UI_VALUE_MT; // validate value

    const std::string val_s = mxUtils::formatNumber<int>(val_i);
    node.updateAttribute(val_s.c_str(), mxconst::get_ATTRIB_LENGTH_MT().c_str(), mxconst::get_ATTRIB_LENGTH_MT().c_str());
  }

  this->mx_add_tooltip(missionx::color::color_vec4_darkorange, mxUtils::formatNumber<float>((static_cast<float> (val_i) * missionx::meter2nm), 2) + " nm");

  ImGui::SameLine(0.0f, 3.0f);
  ImGui::TextColored(missionx::color::color_vec4_cyan, "meters");
}


// -----------------------------------------------


void
WinImguiBriefer::subDraw_ui_xPolyBox(IXMLNode& pNode, mx_trig_strct_& inTrig_ptr) // xLocAndElev, inTrig_ptr
{
  assert(pNode.isEmpty() == false && "[polyBox] Parent Node could not be empty");
  // p1 = trig_ptr.pos
  // p2-------------p3
  // |              |
  // p1-------------p4

  //        "*" = pCenter = center of box
  // p2-------------p3
  // |       *      |
  // p1-------------p4

  // store first Point as the starting calculation point to display to the user - if exists
  // Display starting lat/lon   // display heading of boxed trigger
  // Display Meters %heading    // display Meters %heading - 90.0
  // Apply button - will delete all existing <points> and then construct new ones

  static float heading_f{ 0.0f };
  static int   iMetersVector1{ 100 };
  static int   iMetersVector2{ 100 };
  static int   iVectorLengthBT_StoredInTrigger{ 0 };
  static int   iVectorLengthLR_StoredInTrigger{ 0 };
  IXMLNode     buttomLeft_or_center_xPoint_ptr = Utils::xml_get_or_create_node_ptr(pNode, mxconst::get_ELEMENT_POINT()); // get or create first elemnt <point>, can represent the bottomLeft or center of the trigers triangle area

  inTrig_ptr.pos.setLat(Utils::readNumericAttrib(buttomLeft_or_center_xPoint_ptr, mxconst::get_ATTRIB_LAT(), 0.0));
  inTrig_ptr.pos.setLon(Utils::readNumericAttrib(buttomLeft_or_center_xPoint_ptr, mxconst::get_ATTRIB_LONG(), 0.0));
  inTrig_ptr.pos.setElevationFt(Utils::readNumericAttrib(buttomLeft_or_center_xPoint_ptr, mxconst::get_ATTRIB_ELEV_FT(), 0.0));
  inTrig_ptr.setBuff(inTrig_ptr.iCurrentBuf, inTrig_ptr.pos.get_point_lat_lon_as_string());

  iVectorLengthBT_StoredInTrigger = Utils::readNodeNumericAttrib<int>(inTrig_ptr.node_ptr, mxconst::get_ATTRIB_VECTOR_BT_LENGTH_MT(), 0);
  iVectorLengthLR_StoredInTrigger = Utils::readNodeNumericAttrib<int>(inTrig_ptr.node_ptr, mxconst::get_ATTRIB_VECTOR_LR_LENGTH_MT(), 0);

  ImGui::Checkbox("Plane in center of box", &inTrig_ptr.flag_first_point_is_center_cbox);



  // Position of Plane
  ImGui::TextColored(missionx::color::color_vec4_yellow, "Calculate the trigger's boundaries");
  if (inTrig_ptr.flag_first_point_is_center_cbox) // v3.0.301 B4
    ImGui::TextColored(missionx::color::color_vec4_yellow, "Center of trigger area:");
  else
    ImGui::TextColored(missionx::color::color_vec4_yellow, "Bottom Left Position: ");

  ImGui::SameLine();
  ImGui::InputText("###trigBoxPolyBottomLeftPos", inTrig_ptr.buffArray[inTrig_ptr.iCurrentBuf], sizeof(inTrig_ptr.buffArray[inTrig_ptr.iCurrentBuf]), ImGuiInputTextFlags_ReadOnly);
  ImGui::SameLine();
  if (ImGui::Button("C##cBoxTrig"))
  {
    inTrig_ptr.pos = missionx::data_manager::getPlane_or_Camera_position_as_Point('c');       // c = camera
    inTrig_ptr.setBuff(inTrig_ptr.iCurrentBuf, inTrig_ptr.pos.get_point_lat_lon_as_string()); // camera pos
    if (buttomLeft_or_center_xPoint_ptr.isEmpty() == false)
    {
      buttomLeft_or_center_xPoint_ptr.updateAttribute(inTrig_ptr.pos.getLat_s().c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
      buttomLeft_or_center_xPoint_ptr.updateAttribute(inTrig_ptr.pos.getLon_s().c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());
      buttomLeft_or_center_xPoint_ptr.updateAttribute(inTrig_ptr.pos.getElevFt_s().c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str());
    }
    if (inTrig_ptr.flag_first_point_is_center_cbox)
    {
      inTrig_ptr.node_ptr.updateAttribute(inTrig_ptr.pos.getLat_s().c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
      inTrig_ptr.node_ptr.updateAttribute(inTrig_ptr.pos.getLon_s().c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());
    }
  }
  this->mx_add_tooltip(missionx::color::color_vec4_white, "Get Camera position.");

  // heading

  ImGui::TextColored(missionx::color::color_vec4_orange, "First Vector Heading:");
  this->mx_add_tooltip(missionx::color::color_vec4_yellow, "The plugin will calculate the box as Heading and Heading + 90 deg vectors");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(110.0f);
  if (ImGui::InputFloat("###trigBoxVectorHeading", &heading_f))
  {
    if (heading_f < 0.0f)
      heading_f = 359.0f;
    if (heading_f > 359.0f)
      heading_f = 0.0f;
  }

  ImGui::SameLine();
  if (ImGui::Button("C##trigBoxCameraHading"))
  {
    const XPLMDataRef dref = XPLMFindDataRef("sim/graphics/view/view_heading"); // camera heading
    heading_f              = XPLMGetDataf(dref);
    inTrig_ptr.setBuff(inTrig_ptr.iCurrentBuf, mxUtils::formatNumber<float>(heading_f, 2));
  }
  this->mx_add_tooltip(missionx::color::color_vec4_white, "Get Camera Heading.");
  ImGui::SameLine(0.0f, 10.0f);
  ImGui::TextColored(missionx::color::color_vec4_white, "Second vector heading = +90deg = %.2fdeg", (heading_f + 90.0f > 359.0f) ? heading_f + 90.0f - 360.0f : heading_f + 90.0f);

  // Vector Length
  if (inTrig_ptr.flag_first_point_is_center_cbox) // v3.0.301 B4
    ImGui::TextColored(missionx::color::color_vec4_antiquewhite, "Final length will be twice the vector's entered numbers.");

  ImGui::TextColored(missionx::color::color_vec4_orange, "First Vector Length (mt):");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(80.0f);
  if (ImGui::InputInt("###trigBoxVectorLength1", &iMetersVector1, 25, ImGuiInputTextFlags_ReadOnly))
  {
    if (iMetersVector1 < 5)
      iMetersVector1 = 5;
    if (iMetersVector1 > 90000)
      iMetersVector1 = 90000;
  }
  if (inTrig_ptr.flag_first_point_is_center_cbox) // v3.0.301 B4
    this->mx_add_tooltip(missionx::color::color_vec4_darkorange, "Length = 2 * " + mxUtils::formatNumber<float>((static_cast<float> (iMetersVector1) * missionx::meter2nm), 2) + " nm");
  else
    this->mx_add_tooltip(missionx::color::color_vec4_darkorange, mxUtils::formatNumber<float>((static_cast<float> (iMetersVector1) * missionx::meter2nm), 2) + " nm");

  ImGui::SameLine(0.0f, 10.0f);

  ImGui::TextColored(missionx::color::color_vec4_orange, "Second Vector Length (mt):");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(80.0f);
  if (ImGui::InputInt("###trigBoxVectorLength2", &iMetersVector2, 25, ImGuiInputTextFlags_ReadOnly))
  {
    if (iMetersVector2 < 5)
      iMetersVector2 = 5;
    if (iMetersVector2 > 90000)
      iMetersVector2 = 90000;
  }
  this->mx_add_tooltip(missionx::color::color_vec4_darkorange, mxUtils::formatNumber<float>((static_cast<float> (iMetersVector2) * missionx::meter2nm), 2) + " nm");

  ImGui::SameLine(0.0f, 10.0f);
  if (ImGui::Button("C##distanceRelativeToPlane"))
  {
    if (inTrig_ptr.pos.getLat() == 0.0 || inTrig_ptr.pos.getLon() == 0.0)
    {
      this->setMessage("Your starting position is not valid.", 10);
    }
    else
    {
      Point plane    = missionx::data_manager::getPlane_or_Camera_position_as_Point('c'); // p = plane
      iMetersVector2 = static_cast<int> (plane.calcDistanceBetween2Points (inTrig_ptr.pos, missionx::mx_units_of_measure::meter));
    }
  }
  this->mx_add_tooltip(missionx::color::color_vec4_darkorange, "Calculate Second vector distance relative to camera location (camera.pos - start.pos)\nUse camera in plane for easier positioning using the map.");

  // display original vector lengths
  if (iVectorLengthBT_StoredInTrigger)
  {
    ImGui::TextColored(missionx::color::color_vec4_cyan, "Current Trigger stored vector Lengths are: %i and %i respectively", iVectorLengthBT_StoredInTrigger, iVectorLengthLR_StoredInTrigger);
  }

  // APPLY
  ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
  if (ImGui::Button("Calculate Trigger Boundaries##trigBoxApply"))
  {
    if (inTrig_ptr.pos.getLat() == 0.0 || inTrig_ptr.pos.getLat() == 0.0)
    {
      this->setMessage("Your starting position is not valid.", 10);
    }
    else
    {
      // p1 = trig_ptr.pos = bottom left
      // p2-------------p3
      // |              |
      // p1-------------p4
      //       "*" = pCenter = center of box
      // p2-------------p3
      // |       *      |
      // p1-------------p4
      std::string DUMMY_SKELATON_ELEMENT{};
      // Point p1; // p2, p3, p4;

      if (inTrig_ptr.flag_first_point_is_center_cbox)
      {
        // store special info on the triggers main element
        inTrig_ptr.node_ptr.updateAttribute("true", mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B().c_str(), mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B().c_str());
        inTrig_ptr.node_ptr.updateAttribute(inTrig_ptr.pos.getLat_s().c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
        inTrig_ptr.node_ptr.updateAttribute(inTrig_ptr.pos.getLon_s().c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());

        DUMMY_SKELATON_ELEMENT = "<DUMMY> " + inTrig_ptr.pos.get_point_lat_lon_as_string() + " </DUMMY>";
      }
      else
      {

        //// store special info on the triggers main element
        inTrig_ptr.node_ptr.updateAttribute("", mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B().c_str(), mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B().c_str());
        inTrig_ptr.node_ptr.updateAttribute("", mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
        inTrig_ptr.node_ptr.updateAttribute("", mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());


        DUMMY_SKELATON_ELEMENT = "<DUMMY> " + inTrig_ptr.pos.get_point_lat_lon_as_string() + " </DUMMY>";
      }

      inTrig_ptr.node_ptr.updateAttribute(mxUtils::formatNumber<int>(iMetersVector1).c_str(), mxconst::get_ATTRIB_VECTOR_BT_LENGTH_MT().c_str(), mxconst::get_ATTRIB_VECTOR_BT_LENGTH_MT().c_str());
      inTrig_ptr.node_ptr.updateAttribute(mxUtils::formatNumber<int>(iMetersVector2).c_str(), mxconst::get_ATTRIB_VECTOR_LR_LENGTH_MT().c_str(), mxconst::get_ATTRIB_VECTOR_LR_LENGTH_MT().c_str());

      IXMLNode xRectangle = Utils::xml_get_or_create_node_ptr(pNode, mxconst::get_ELEMENT_RECTANGLE());
      assert(xRectangle.isEmpty() == false && (std::string(__func__).append(": ").append(pNode.getName()).append(" - Failed to create node: ").c_str()));

      xRectangle.updateAttribute(mxUtils::formatNumber<float>(heading_f, 2).c_str(), mxconst::get_ATTRIB_HEADING_PSI().c_str(), mxconst::get_ATTRIB_HEADING_PSI().c_str());
      xRectangle.updateAttribute((mxUtils::formatNumber<int>(iMetersVector1) + "|" + mxUtils::formatNumber<int>(iMetersVector2)).c_str(), mxconst::get_ATTRIB_DIMENSIONS().c_str(), mxconst::get_ATTRIB_DIMENSIONS().c_str());
      xRectangle.updateAttribute((inTrig_ptr.flag_first_point_is_center_cbox) ? mxconst::get_MX_TRUE().c_str() : mxconst::get_MX_FALSE().c_str(), mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B().c_str(), mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B().c_str());



#ifndef RELEASE
      Log::logMsg("Rectangular main point: " + inTrig_ptr.pos.get_point_lat_lon_as_string());
#endif // !RELEASE



      Utils::xml_delete_all_subnodes(pNode, mxconst::get_ELEMENT_POINT()); // delete all <point> sub nodes

      // this part is only to simplify and beautify the trigger output
      IXMLResults   parse_result_strct;
      IXMLDomParser iDomTemplate;
      auto          xDummy = iDomTemplate.parseString(DUMMY_SKELATON_ELEMENT.c_str(), mxconst::get_DUMMY_ROOT_DOC().c_str(), &parse_result_strct).deepCopy(); // parse xml into ITCXMLNode

      if (xDummy.nChildNode(mxconst::get_ELEMENT_POINT().c_str()) > 0)
      {
        inTrig_ptr.setBuff(inTrig_ptr.iCurrentBuf, inTrig_ptr.pos.get_point_lat_lon_as_string()); // camera pos, reformat point to have lat/lon/elev information
        pNode.addChild(xDummy.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), 0).deepCopy());        // this is the starting position of the rectangular shape

        if (inTrig_ptr.flag_first_point_is_center_cbox)
          this->setMessage("Calculated triggers boundaries for center rectangular point.", 10);
        else
          this->setMessage("Calculated triggers boundaries for bottomLeft rectangular point.", 10);
      }
      else
        this->setMessage("Failed calculation, not enough points, try to rerun the trigger calculation button.", 10);
    }
  }
  ImGui::PopStyleColor(4);
}


// -----------------------------------------------


void
WinImguiBriefer::subDraw_ui_xTrigger_elev(mx_trig_strct_& inTrig_ptr, IXMLNode& node, bool inResetPick)
{

  static const std::vector<const char*> vecElevOptions           = { "min/max elev", "lower than..", "above than..", "max elev above ground", "min elev above ground" };
  static const std::vector<const char*> vecElevOptions_sign_trns = { "|", "--", "++", "---", "+++" };
  static int                            elev_ft_arr[2]           = { 0, 0 };
  static std::string                    elev_s{ "" };

  if (inResetPick)
  {
    elev_s.clear();
  }

  ImGui::SetNextItemWidth(140.0f);
  if (ImGui::Combo("##dynElevPick", &inTrig_ptr.trig_ui_elev_type_combo_indx, vecElevOptions.data(), static_cast<int> (vecElevOptions.size ())))
  {
    elev_s.clear();
    inTrig_ptr.node_ptr.updateAttribute(mxUtils::formatNumber<int>(inTrig_ptr.trig_ui_elev_type_combo_indx).c_str(), mxconst::get_CONV_ATTRIB_trig_ui_elev_type_combo_indx().c_str(), mxconst::get_CONV_ATTRIB_trig_ui_elev_type_combo_indx().c_str());
  }

  if (inTrig_ptr.trig_ui_elev_type_combo_indx > -1)
    ImGui::SameLine();

  switch (inTrig_ptr.trig_ui_elev_type_combo_indx)
  {
    case 0: // min/max
    {
      ImGui::SetNextItemWidth(160.0f);
      if (ImGui::InputInt2("ft.##TrigElevFt", elev_ft_arr))
      {
      }
      // validation
      if ((elev_ft_arr[1] - elev_ft_arr[0]) < 500)
      {
        elev_ft_arr[1] = elev_ft_arr[0] + 500; // we make sure that the min/max elevation is no less than 500ft
      }
      elev_s = mxUtils::formatNumber<int>(elev_ft_arr[0]) + "|" + mxUtils::formatNumber<int>(elev_ft_arr[1]);
    }
    break;
    case 1:
    case 2:
    case 3:
    case 4:
    {
      ImGui::SetNextItemWidth(120.0f);
      ImGui::InputInt("##TrigElevFt", &elev_ft_arr[0], 100);
      if (elev_ft_arr[0] < 0) // make sure value is not negative so we won't have: "++-200" or "---200" when we wanted "--200"
        elev_ft_arr[0] = 0;

      elev_s = vecElevOptions_sign_trns.at(inTrig_ptr.trig_ui_elev_type_combo_indx) + mxUtils::formatNumber<int>(elev_ft_arr[0]);
    }
    break;
    default:
      break;
  } // end switch


  node.updateAttribute(elev_s.c_str(), mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT().c_str(), mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT().c_str());
}


// ------------ ui scriptlet editing widget --------------

void
WinImguiBriefer::subDraw_ui_xScriptlet(IXMLNode& pNode, mxTrig_ui_mode_enm& inMode, mx_trig_strct_* inTrig_ptr, const std::string inScriptInputLabel, missionx::mx_local_fpln_strct* inLegData, char* inOutBuff)
{
  static int  script_index = 0;
  static char buff[missionx::LOG_BUFF_SIZE]{ 0 };

  const auto lmbda_get_buff = [&]()
  {
    if (inTrig_ptr == nullptr)
    {
      if (inOutBuff == nullptr)
        return buff;
      else
        return inOutBuff;
    }

    return inTrig_ptr->buffArray[inTrig_ptr->iCurrentBuf];
  };

  auto working_buff = lmbda_get_buff();

  int leg_index_i = (inLegData == nullptr) ? 0 : inLegData->indx;

  // extract xCondition if available
  auto        xConditions_ptr  = (inTrig_ptr == nullptr) ? IXMLNode::emptyIXMLNode : Utils::xml_get_or_create_node_ptr(inTrig_ptr->node_ptr, mxconst::get_ELEMENT_CONDITIONS());
  std::string scriptlet_name_s = Utils::readAttrib(xConditions_ptr, mxconst::get_ATTRIB_COND_SCRIPT(), "");
  if (scriptlet_name_s.empty())
  {
    scriptlet_name_s = "leg_" + mxUtils::formatNumber<int>(leg_index_i) + "_scriptlet_" + mxUtils::formatNumber<int>(script_index);

    xConditions_ptr.updateAttribute(scriptlet_name_s.c_str(), mxconst::get_ATTRIB_COND_SCRIPT().c_str(), mxconst::get_ATTRIB_COND_SCRIPT().c_str());

    ++script_index;
  }


  IXMLNode xScriptlet_ptr = (scriptlet_name_s.empty()) ? IXMLNode::emptyIXMLNode : Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(pNode, mxconst::get_ELEMENT_SCRIPTLET(), mxconst::get_ATTRIB_NAME(), scriptlet_name_s);
  if (xScriptlet_ptr.isEmpty())
    xScriptlet_ptr = Utils::xml_get_or_create_node_ptr(pNode, mxconst::get_ELEMENT_SCRIPTLET(), mxconst::get_ATTRIB_NAME(), scriptlet_name_s);

  assert(xScriptlet_ptr.isEmpty() == false && " scriptlet cNode can't be empty");

  std::string script_cdata_s = (xScriptlet_ptr.isEmpty()) ? "" : Utils::xml_read_cdata_node(xScriptlet_ptr, "");
  if (inTrig_ptr)
    inTrig_ptr->setBuff(inTrig_ptr->iCurrentBuf, script_cdata_s);



  ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", ((inScriptInputLabel.empty()) ? "Enter <scriptlet> Code:" : inScriptInputLabel.c_str()));
  this->mx_add_tooltip(missionx::color::color_vec4_white,
                    "The script must handle when to fire the trigger.\nIt can also manage messages and other supported commands.\nIt is up to you to decide the complexity of the code.\nBest practice: use \"Trigger Outcome\" to handle messages and post fire staff.\n\nCheck \"Designer Guide\" for more explanations.");

  if (ImGui::InputTextMultiline("##scriptletUi", working_buff, missionx::LOG_BUFF_SIZE, ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 60.0f), ImGuiInputTextFlags_AllowTabInput))
  {
    std::string buff_s = std::string(working_buff);

    assert(scriptlet_name_s.empty() == false && "scriptlet name attribute can't be empty");

    Utils::xml_add_cdata(xScriptlet_ptr, buff_s);

  } // update origin node and buff after each change


  // display characters left to write
  ImGui::TextColored(missionx::color::color_vec4_floralwhite, "%zu", missionx::LOG_BUFF_SIZE - std::string_view(working_buff).length());

} // subDraw_ui_xScriptlet



// ------------ ui Trigger main code --------------



void
WinImguiBriefer::subDraw_ui_xTrigger_main(missionx::mx_local_fpln_strct& inLegData, bool& in_out_needRefresh_b, int inLegIndex, std::map<int, missionx::mx_trig_strct_>& inMapOfGlobalTriggers, std::vector<std::string>& inVecGlobalTriggers_names)
{
  static mx_trig_strct_* trig_ptr = nullptr;

  assert(inLegData.xTriggers.isEmpty() == false && "[subDraw_ui_xTrigger_main] Leg triggers can't be empty");

  // parse xTriggers
  if (in_out_needRefresh_b)
  {
    trig_ptr                             = nullptr; // static pointer reset
    int index                            = 0;
    this->strct_conv_layer.trig_picked_i = -1; // no pick
    in_out_needRefresh_b                 = false;
    inMapOfGlobalTriggers.clear();
    inVecGlobalTriggers_names.clear();
    for (int i1 = 0; i1 < inLegData.xTriggers.nChildNode(mxconst::get_ELEMENT_TRIGGER().c_str()); ++i1)
    {
      missionx::mx_trig_strct_ trig;
      auto                     nodeTrig = inLegData.xTriggers.getChildNode(mxconst::get_ELEMENT_TRIGGER().c_str(), i1);
      trig.trig_name_s                  = Utils::readAttrib(nodeTrig, mxconst::get_ATTRIB_NAME(), "");
      trig.trig_type_s                  = Utils::readAttrib(nodeTrig, mxconst::get_ATTRIB_TYPE(), "");
      trig.trig_onGround_s              = Utils::readAttrib(nodeTrig, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "");

      if (trig.trig_name_s.empty())
      {
        Log::logMsg("Found trigger without name. skipping");
        continue;
      }
      else if (Utils::isElementExistsInVec(inVecGlobalTriggers_names, trig.trig_name_s))
      {
        Log::logMsg("Trigger by the name: " + trig.trig_name_s + " is already exists. Skipping...");
        continue;
      }

      if (mxconst::get_TRIG_TYPE_RAD().compare(trig.trig_type_s) == 0)
        trig.trig_type_indx = 0;
      else if (mxconst::get_TRIG_TYPE_SCRIPT().compare(trig.trig_type_s) == 0)
        trig.trig_type_indx = 1;
      else if (mxconst::get_TRIG_TYPE_POLY().compare(trig.trig_type_s) == 0)
        trig.trig_type_indx = 2;
      else if (mxconst::get_TRIG_TYPE_CAMERA().compare(trig.trig_type_s) == 0) // v3.0.303.7 fix unsupported camera type trigger
        trig.trig_type_indx = 3;
      else
      {
        Log::logMsg("Reading tirgger: " + trig.trig_name_s + " has unsupporte trigger type, by this UI screen. It might be supported by the plugin though. Check designer guide. Skipping...");
        continue;
      }


      trig.flag_first_point_is_center_cbox = Utils::readBoolAttrib(nodeTrig, mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B(), false); // v3.0.303.4 // boolean


      trig.indx     = index;
      trig.node_ptr = nodeTrig;
      inVecGlobalTriggers_names.emplace_back(trig.trig_name_s);
      Utils::addElementToMap(inMapOfGlobalTriggers, index, trig);
      ++index;

    } // end loop over all triggers

  } // end re-read xNode information for <trigger>

  //////////// Side by Side

  static std::string suggested_name;
  // Child 1: no border, enable horizontal scrollbar
  {
    ////// Trigger information
    if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::naTrigger)
      ImGui::TextColored(missionx::color::color_vec4_yellow, "[opt] Add custom Events/Triggers");
    else
      ImGui::TextColored(missionx::color::color_vec4_yellow, "Trigger Information: ");

    ImGui::BeginChild("left_ChildListTriggers", ImVec2(ImGui::GetContentRegionAvail().x * 0.22f, 100.0f), ImGuiChildFlags_None, ImGuiWindowFlags_None); // v24.06.1 replaced "GetWindowContentRegionMax()" with "GetContentRegionAvail()"
    {
      //// LIST
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      {
        //// Show Add / Cancel addition of new trigger
        if (this->strct_conv_layer.trig_ui_mode > mxTrig_ui_mode_enm::naTrigger)
        {
          if (ImGui::Button(" Cancel Operation "))
          {
            if (trig_ptr)
            {
              if (trig_ptr->node_ptr.isEmpty() == false && trig_ptr->copyOfNode_ptr.isEmpty() == false)
              {
                if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::newTrigger)
                {
                  trig_ptr->node_ptr.deleteNodeContent();
                  trig_ptr->init();
                }
                else if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::editTrigger)
                {
                  auto pNode = trig_ptr->node_ptr.getParentNode();
                  if (!pNode.isEmpty())
                  {
                    trig_ptr->node_ptr.deleteNodeContent();
                    trig_ptr->node_ptr = trig_ptr->copyOfNode_ptr.deepCopy();
                    pNode.addChild(trig_ptr->node_ptr, trig_ptr->indx);
                  }

                } // end cancel operation
              }
              in_out_needRefresh_b = true; // refresh after cancel
            }

            this->strct_conv_layer.trig_ui_mode = mxTrig_ui_mode_enm::naTrigger;
          }
        }
        else if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::naTrigger)
        {
          if (ImGui::Button(" +++ Add Trigger +++"))
          {
            this->strct_conv_layer.trig_ui_mode = mxTrig_ui_mode_enm::newTrigger;
            this->strct_conv_layer.trigger.init();
            this->strct_conv_layer.trigger.node_ptr = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_ELEMENT_TRIGGER());
            this->strct_conv_layer.trigger.indx     = this->strct_conv_layer.trig_seq;
            // v3.0.303.7
            auto              xLocElevData = Utils::xml_get_or_create_node_ptr(this->strct_conv_layer.trigger.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA());
            auto              xRadius      = Utils::xml_get_or_create_node_ptr(xLocElevData, mxconst::get_ELEMENT_RADIUS());
            int               val_i        = Utils::readNodeNumericAttrib<int>(xRadius, mxconst::get_ATTRIB_LENGTH_MT(), missionx::MIN_RAD_UI_VALUE_MT);
            const std::string val_s        = mxUtils::formatNumber<int>(val_i);
            xRadius.updateAttribute(val_s.c_str(), mxconst::get_ATTRIB_LENGTH_MT().c_str(), mxconst::get_ATTRIB_LENGTH_MT().c_str());
            // end v3.0.303.7

            trig_ptr                 = &this->strct_conv_layer.trigger;
            trig_ptr->copyOfNode_ptr = trig_ptr->node_ptr.deepCopy(); // store a copy to revert to
            ++this->strct_conv_layer.trig_seq;

            suggested_name = "trig_" + mxUtils::formatNumber<int>(inLegIndex) + "_" + mxUtils::formatNumber<int>(this->strct_conv_layer.trig_seq) + "_" + Utils::get_hash_string(Utils::get_time_as_string());

            (*trig_ptr).node_ptr.updateAttribute(suggested_name.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
          }
        }
      } // end style colors
      ImGui::PopStyleColor(4);

      // Display list of triggers
      bool vDisableCombo = false;
      if (this->strct_conv_layer.trig_ui_mode != mxTrig_ui_mode_enm::naTrigger) // newTrigger / editTrigger
      {
        ImGui::BeginDisabled();
        vDisableCombo = true;
      }

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      if (ImGui::BeginListBox("##ListOfTriggerNames"))
      {
        for (auto& [indx, data] : inMapOfGlobalTriggers)
        {
          const bool is_selected = (this->strct_conv_layer.trig_picked_i == indx);
          if (ImGui::Selectable(data.trig_name_s.c_str(), is_selected))
          {

            this->strct_conv_layer.trig_picked_i = indx;
            trig_ptr                             = &this->strct_conv_layer.mapOfGlobalTriggers[indx];
            trig_ptr->copyOfNode_ptr             = trig_ptr->node_ptr.deepCopy(); // store a copy to revert to

            this->strct_conv_layer.trig_ui_mode = mxTrig_ui_mode_enm::editTrigger;
            suggested_name                      = trig_ptr->trig_name_s;
          }
          // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
          if (is_selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
      } // end ListBox

      if (vDisableCombo)
        ImGui::EndDisabled();
    }
    ImGui::EndChild();
  }

  // Child 2:
  if (trig_ptr != nullptr && this->strct_conv_layer.trig_ui_mode > mxTrig_ui_mode_enm::naTrigger)
  {
    subDraw_ui_xTrigger_detail((*trig_ptr), in_out_needRefresh_b, suggested_name, inLegData);
  }
}

// ---------------------------------------------
// ------------ ui Trigger detail --------------

void
WinImguiBriefer::subDraw_ui_xTrigger_detail(mx_trig_strct_& inTrig_ptr, bool& in_out_needRefresh_b, std::string& suggested_name, missionx::mx_local_fpln_strct& inLegData)
{
  bool                   flag_create_trigger = false; // used with the button "create/update" trigger
  const std::string_view btnStoreLabel_sv    = (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::newTrigger) ? "Create Trigger##CreateOrUpdateTriggerButton" : "Update Trigger##CreateOrUpdateTriggerButton";

  const auto lmbda_update_trig_type = [&]()
  {
    inTrig_ptr.trig_type_s = std::string(this->strct_conv_layer.vecTrigType_list.at(inTrig_ptr.trig_type_indx)); // debug
    inTrig_ptr.node_ptr.updateAttribute(this->strct_conv_layer.vecTrigType_list_trans.at(inTrig_ptr.trig_type_indx), mxconst::get_ATTRIB_TYPE().c_str(), mxconst::get_ATTRIB_TYPE().c_str());

    inTrig_ptr.node_ptr.updateAttribute(mxUtils::formatNumber<int>(inTrig_ptr.trig_type_indx).c_str(), mxconst::get_CONV_ATTRIB_trig_ui_type_combo_indx().c_str(), mxconst::get_CONV_ATTRIB_trig_ui_type_combo_indx().c_str());
  };

  inTrig_ptr.iCurrentBuf = 0;

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
  ImGui::SameLine();

  ImGui::BeginChild("right_ChildTriggerDetails", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 300.0f), ImGuiChildFlags_None, window_flags); // v24.06.1 replaced: "GetContentRegionMax()" with "GetContentRegionAvail()"
  {

    // draw trigger element
    if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::editTrigger)
    {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, missionx::color::color_vec4_maroon);

      IXMLRenderer      xmlRenderer;
      const std::string print_s = xmlRenderer.getString(inTrig_ptr.node_ptr);
      char              print_buff[2048]; // { 0 };
#ifdef IBM
      memcpy_s(print_buff, sizeof(print_buff), print_s.c_str(), (print_s.length() > sizeof(print_buff)) ? sizeof(print_buff) : print_s.length());
#else
      memcpy(print_buff, print_s.c_str(), (print_s.length() > sizeof(print_buff)) ? sizeof(print_buff) : print_s.length());
#endif // IBM

      // readonly multi line input. We display 10 rows
      ImGui::InputTextMultiline("###triggerXMLoutput", print_buff, sizeof(print_buff), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() * 12.0f), ImGuiInputTextFlags_ReadOnly);

      ImGui::PopStyleColor();

      // CREATE Trigger Button
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      if (ImGui::Button(btnStoreLabel_sv.data()))
        flag_create_trigger = true;
      ImGui::PopStyleColor(4); // create trigger button


      ImGui::PushStyleColor(ImGuiCol_Separator, missionx::color::color_vec4_white);
      ImGui::Separator();
      ImGui::PopStyleColor();
    }


    ImGui::TextColored(missionx::color::color_vec4_yellow, "Type:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(85.0f);
    if (ImGui::Combo("##TriggerTypeCombo", &inTrig_ptr.trig_type_indx, this->strct_conv_layer.vecTrigType_list.data(), static_cast<int> (this->strct_conv_layer.vecTrigType_list.size ())))
    {
      lmbda_update_trig_type();

      // v3.0.303.7 initialize RAD/Camera based trigger
      if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::rad) || inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::camera))
      {
        auto              xLocElevData = Utils::xml_get_or_create_node_ptr(inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA());
        auto              xRadius      = Utils::xml_get_or_create_node_ptr(xLocElevData, mxconst::get_ELEMENT_RADIUS());
        int               val_i        = Utils::readNodeNumericAttrib<int>(xRadius, mxconst::get_ATTRIB_LENGTH_MT(), missionx::MIN_RAD_UI_VALUE_MT);
        const std::string val_s        = mxUtils::formatNumber<int>(val_i);
        xRadius.updateAttribute(val_s.c_str(), mxconst::get_ATTRIB_LENGTH_MT().c_str(), mxconst::get_ATTRIB_LENGTH_MT().c_str());
      }

      // clear buff[0] data
      inTrig_ptr.resetBuff(inTrig_ptr.iCurrentBuf);
    }

    // display radius widget if trigger type is rad
    if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::rad) || inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::camera))
    {
      auto xLocElevData = Utils::xml_get_or_create_node_ptr(inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA());
      auto xRadius      = Utils::xml_get_or_create_node_ptr(xLocElevData, mxconst::get_ELEMENT_RADIUS());
      ImGui::SameLine();
      subDraw_ui_xRadius(xRadius, 15);
    }

    // Constraint // no buff
    if (inTrig_ptr.trig_type_indx != static_cast<int> (missionx::mx_trig_type_enum::script) && inTrig_ptr.trig_type_indx != static_cast<int> (missionx::mx_trig_type_enum::camera)) // if not camera then no elevation info is needed, will always be ignored
    {
      ImGui::TextColored(missionx::color::color_vec4_yellow, "Elev ft:");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100.0f);
      if (ImGui::Combo("###ConstraintPosition", &inTrig_ptr.trig_plane_pos_combo_indx, this->strct_conv_layer.vecTrigOnGround_list.data(), static_cast<int> (this->strct_conv_layer.vecTrigOnGround_list.size ())))
      {
        inTrig_ptr.trig_onGround_s = std::string(this->strct_conv_layer.vecTrigOnGround_list.at(inTrig_ptr.trig_plane_pos_combo_indx)); // debug
        Utils::xml_search_and_set_attribute_in_IXMLNode(inTrig_ptr.node_ptr, mxconst::get_ATTRIB_PLANE_ON_GROUND(), this->strct_conv_layer.vecTrigOnGround_list_trans.at(inTrig_ptr.trig_plane_pos_combo_indx), mxconst::get_ELEMENT_CONDITIONS());
        Utils::xml_search_and_set_attribute_in_IXMLNode(inTrig_ptr.node_ptr, mxconst::get_CONV_ATTRIB_trig_ui_plane_pos_combo_indx(), mxUtils::formatNumber<int>(inTrig_ptr.trig_plane_pos_combo_indx), inTrig_ptr.node_ptr.getName());
      }
      if (inTrig_ptr.trig_plane_pos_combo_indx == 2) // 2 = airborne
      {
        ImGui::SameLine(0.0f, 10.0f);
        auto xLocElevData = Utils::xml_get_or_create_node_ptr(inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA());
        auto xElevVolume  = Utils::xml_get_or_create_node_ptr(xLocElevData, mxconst::get_ELEMENT_ELEVATION_VOLUME());
        subDraw_ui_xTrigger_elev(inTrig_ptr, xElevVolume);
      }
    }

    // lat/lon  // use buff
    if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::rad) || inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::camera))
    {
      if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::newTrigger)
        ImGui::TextColored(missionx::color::color_vec4_yellow, "Enter center of event:");
      else
        ImGui::TextColored(missionx::color::color_vec4_yellow, "Update center of event: "); // mxTrig_ui_mode_enm::editTrigger

      ImGui::InputText("###trigCenterPosition", inTrig_ptr.buffArray[inTrig_ptr.iCurrentBuf], sizeof(inTrig_ptr.buffArray[inTrig_ptr.iCurrentBuf]), ImGuiInputTextFlags_ReadOnly); // buffArray  // we will store the points only when clicking apply


      //// position // use buff based on rad
      ImGui::SameLine();
      if (ImGui::Button("P##pTrig"))
      {
        inTrig_ptr.setBuff(inTrig_ptr.iCurrentBuf, missionx::data_manager::getPoint_as_stringFromPlaneCamera("sim/flightmodel/position/latitude", "sim/flightmodel/position/longitude"));
      }
      this->mx_add_tooltip(missionx::color::color_vec4_darkorange, "plane position");

      ImGui::SameLine();
      if (ImGui::Button("C##cTrig"))
      {
        inTrig_ptr.setBuff(inTrig_ptr.iCurrentBuf, missionx::data_manager::getPoint_as_stringFromPlaneCamera("sim/graphics/view/view_x", "sim/graphics/view/view_y", 'c')); // camera pos
      }
      ImGui::SameLine(0.0f, 5.0f);
      this->mx_add_tooltip(missionx::color::color_vec4_darkorange, "camera position");

      ImGui::SameLine();
      if (ImGui::Button("U##uTrig"))
      {
        ImGui::OpenPopup(POPUP_USER_LAT_LON.c_str());
      }
      ImGui::SameLine(0.0f, 5.0f);
      this->mx_add_tooltip(missionx::color::color_vec4_darkorange, "User defined position");

      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      if (ImGui::Button("Apply Position##applyToLocAndElev"))
      {
        auto loc_and_elev_ptr = inTrig_ptr.node_ptr.getChildNode(mxconst::get_ELEMENT_LOC_AND_ELEV_DATA().c_str());
        Utils::xml_delete_all_subnodes(loc_and_elev_ptr, mxconst::get_ELEMENT_POINT());

        std::string val_buff_s = inTrig_ptr.getBuff(inTrig_ptr.iCurrentBuf);
        auto        childNode  = Utils::xml_create_node_from_string(val_buff_s);

        if (childNode.isEmpty())
          this->setMessage("Point is not valid: " + inTrig_ptr.getBuff(inTrig_ptr.iCurrentBuf), 10);
        else
        {
          loc_and_elev_ptr.addChild(childNode);
          this->setMessage("", 1);
        }
      }
      ImGui::PopStyleColor(4);

      ImGui::PushStyleColor(ImGuiCol_ChildBg, missionx::color::color_vec4_black);
      if (ImGui::BeginPopupModal(POPUP_USER_LAT_LON.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
      {
        subDraw_popup_user_lat_lon(inTrig_ptr);
        ImGui::EndPopup();
      }
      ImGui::PopStyleColor();

      ++inTrig_ptr.iCurrentBuf; // if we display positioning we should increment the buff array position too

      // End positioning of trigger
    }
    else if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::poly)) // v3.0.301 B3
    {
      auto xLocAndElev = Utils::xml_get_or_create_node_ptr(inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA());

      ImGui::PushStyleColor(ImGuiCol_Separator, missionx::color::color_vec4_white);
      ImGui::Separator();
      ImGui::PopStyleColor();

      subDraw_ui_xPolyBox(xLocAndElev, inTrig_ptr);
      ++inTrig_ptr.iCurrentBuf;
    }
    else if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::script)) // v3.0.301 B3
    {
      ImGui::PushStyleColor(ImGuiCol_Separator, missionx::color::color_vec4_white);
      ImGui::Separator();
      ImGui::PopStyleColor();


      subDraw_ui_xScriptlet(inLegData.xFlightPlan, this->strct_conv_layer.trig_ui_mode, &inTrig_ptr, "Enter the trigger's condition <scriptlet> code to fire the trigger:", &inLegData, inTrig_ptr.buffArray[inTrig_ptr.iCurrentBuf]);
      ++inTrig_ptr.iCurrentBuf;
    }



    ImGui::PushStyleColor(ImGuiCol_Separator, missionx::color::color_vec4_white);
    ImGui::Separator();
    ImGui::PopStyleColor();
    // Outcome button + popup // use buff

    // calculate position
    ImGui::NewLine();
    ImGui::SameLine(0.0, ImGui::GetContentRegionAvail().x * 0.33f);

    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
    if (ImGui::Button("Trigger Outcome Settings##btnOutcomeTrigPopup"))
    {
      ImGui::OpenPopup(POPUP_TRIG_OUTCOME.c_str());
    }
    ImGui::PopStyleColor(4);

    { // POPUP Outcome
      const auto   win_size_vec2 = ImGui::GetWindowSize();
      const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
      ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowSize(ImVec2(win_size_vec2.x * 0.85f, 220.0f));
      if (ImGui::BeginPopupModal(POPUP_TRIG_OUTCOME.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
      {
        subDraw_popup_outcome(inTrig_ptr, inLegData.xMessageTmpl);
        ImGui::EndPopup();
        ++inTrig_ptr.iCurrentBuf;
      }
    }


    // draw trigger element
    if (this->strct_conv_layer.trig_ui_mode != mxTrig_ui_mode_enm::editTrigger)
    {

      ImGui::PushStyleColor(ImGuiCol_Separator, missionx::color::color_vec4_white);
      ImGui::Separator();
      ImGui::PopStyleColor();

      // CREATE Trigger Button
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor(ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      if (ImGui::Button(btnStoreLabel_sv.data()))
        flag_create_trigger = true;
      ImGui::PopStyleColor(4); // create trigger button



      IXMLRenderer      xmlRenderer;
      const std::string print_s = xmlRenderer.getString(inTrig_ptr.node_ptr);
      char              print_buff[2048]; // { 0 };
#ifdef IBM
      memcpy_s(print_buff, sizeof(print_buff), print_s.c_str(), (print_s.length() > sizeof(print_buff)) ? sizeof(print_buff) : print_s.length());
#else
      memcpy(print_buff, print_s.c_str(), (print_s.length() > sizeof(print_buff)) ? sizeof(print_buff) : print_s.length());
#endif // IBM

      // readonly multi line input. We display 10 rows
      ImGui::InputTextMultiline("###triggerXMLoutput", print_buff, sizeof(print_buff), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() * 12.0f), ImGuiInputTextFlags_ReadOnly);
    }


  } // end Right Child
  ImGui::EndChild();



  // Create/Update the trigger node
  if (flag_create_trigger)
  {
    bool flag_trig_is_valid = true;
    // do validation
    if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::rad) || inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::camera))
    {

      // v3.0.301 B3
      // remove the cond_script attribute from <conditions>, this should only be used with "script based trigger"
      auto xConditions = Utils::xml_get_or_create_node_ptr(inTrig_ptr.node_ptr, mxconst::get_ELEMENT_CONDITIONS());
      xConditions.updateAttribute("", mxconst::get_ATTRIB_COND_SCRIPT().c_str(), mxconst::get_ATTRIB_COND_SCRIPT().c_str());

      auto   xLocElevData = Utils::xml_get_or_create_node_ptr(inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA());
      auto   xPoint       = Utils::xml_get_or_create_node_ptr(xLocElevData, mxconst::get_ELEMENT_POINT());
      double lat          = Utils::readNumericAttrib(xPoint, mxconst::get_ATTRIB_LAT(), 0.0f);
      double lon          = Utils::readNumericAttrib(xPoint, mxconst::get_ATTRIB_LONG(), 0.0f);
      if (lat == 0.0 || lon == 0.0)
      {
        flag_trig_is_valid = false;
        this->setMessage("Trigger is not valid. Position data is not valid");
      }

      // v3.0.304.4 validate radius value
      [[maybe_unused]] bool flag_found = false;
      if (Utils::xml_get_attribute_value_drill(xLocElevData, mxconst::get_ATTRIB_LENGTH_MT(), flag_found, mxconst::get_ELEMENT_RADIUS()).empty())
      {
        flag_trig_is_valid = false;
        this->setMessage("Trigger is not valid. Set Radius");
      }

      // v3.0.303.7 if trig type = camera then reset on_ground attribute to empty (which means ignore elevation), we might consider ---10 instead
      if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::camera))
        Utils::xml_search_and_set_attribute_in_IXMLNode(inTrig_ptr.node_ptr, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "", mxconst::get_ELEMENT_CONDITIONS());
    }
    else if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::poly))
    {
      auto xLocAndElev = Utils::xml_get_or_create_node_ptr(inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA());
      assert(xLocAndElev.isEmpty() == false && "<Trigger> must have <loc_and_elev_data> element.");

      if (xLocAndElev.nChildNode(mxconst::get_ELEMENT_POINT().c_str()) > 0)
      {
        flag_trig_is_valid = true;
      }
      else
      {
        this->setMessage("Trigger is not valid. Not enough <point> elements.");
        flag_trig_is_valid = false;
      }
    }
    else // v3.0.303.7 remove <loc_and_elev_data> since it is a script based
    {
      Utils::xml_delete_all_subnodes(inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA());
    }


    if (flag_trig_is_valid)
    {
      lmbda_update_trig_type();

      in_out_needRefresh_b = true;

      if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::newTrigger)
      {

        inLegData.xTriggers.addChild(inTrig_ptr.node_ptr.deepCopy());
        if (inLegData.xLeg.isEmpty() == false)
        {
          auto xLink = inLegData.xLeg.addChild(mxconst::get_ELEMENT_LINK_TO_TRIGGER().c_str());
          xLink.updateAttribute(suggested_name.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());

          this->setMessage("Added Trigger: " + suggested_name, 5);
        }
      }
      else if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::editTrigger)
      {
        this->setMessage("Updated Trigger: " + suggested_name, 5);
      }
      else
        this->setMessage("", 1);


      this->strct_conv_layer.trig_ui_mode = mxTrig_ui_mode_enm::naTrigger; // reset the trigger creation ui status
    }                                                                      // finish save changes
  }
  // END CREATE TRIGGER NODE
}

// -----------------------------------------------

std::map<int, missionx::mx_local_fpln_strct>
WinImguiBriefer::read_and_parse_saved_state(const std::string inPathAndFile)
{
  std::string                                  errMsg;
  std::map<int, missionx::mx_local_fpln_strct> fpln;


  // Read Little Nav Map flight plan
  Log::logMsg("[Load] Reading Stored Conversion Screen Saved State File: " + inPathAndFile); // debug
  IXMLDomParser iDom;
  ITCXMLNode    xMainNode = iDom.openFileHelper(inPathAndFile.c_str(), mxconst::get_CONVERSION_ROOT_DOC().c_str(), &errMsg);

  if (errMsg.empty() == false) // error
  {
    this->strct_conv_layer.flag_load_conversion_file = false;
    Log::logMsg(errMsg);
  }
  else
  {
    int legCounter       = xMainNode.nChildNode(mxconst::get_ELEMENT_FPLN().c_str());
    int triggerCounter_i = 0;

    for (int i1 = 0; i1 < legCounter; ++i1)
    {
      missionx::mx_local_fpln_strct leg;
      auto                          xLeg = xMainNode.getChildNode(mxconst::get_ELEMENT_FPLN().c_str(), i1).deepCopy();

      xLeg.updateName(mxconst::get_ELEMENT_LEG().c_str());
      leg.xLeg = xLeg.deepCopy();


      if (xLeg.isEmpty() == false)
      {
        leg.indx       = Utils::readNodeNumericAttrib<int>(xLeg, mxconst::get_ATTRIB_INDEX(), i1);
        leg.name       = Utils::readAttrib(xLeg, mxconst::get_ELEMENT_LNM_Name(), std::string("NAME").append(mxUtils::formatNumber<int>(i1)));
        leg.ident      = Utils::readAttrib(xLeg, mxconst::get_ELEMENT_LNM_Ident(), leg.name);
        leg.attribName = Utils::readAttrib(xLeg, mxconst::get_ATTRIB_NAME(), std::string("NAME").append(mxUtils::formatNumber<int>(i1)));
        leg.type       = Utils::readAttrib(xLeg, mxconst::get_ELEMENT_LNM_Type(), "");

        leg.flag_isLeg            = Utils::readBoolAttrib(xLeg, mxconst::get_CONV_ATTRIB_isLeg(), false);
        leg.flag_isLast           = Utils::readBoolAttrib(xLeg, mxconst::get_CONV_ATTRIB_isLast(), false);
        leg.flag_ignore_leg       = Utils::readBoolAttrib(xLeg, mxconst::get_CONV_ATTRIB_ignore_leg(), false);
        leg.flag_convertToBriefer = Utils::readBoolAttrib(xLeg, mxconst::get_CONV_ATTRIB_convertToBriefer(), false);
        leg.flag_add_marker       = Utils::readBoolAttrib(xLeg, mxconst::get_CONV_ATTRIB_add_marker(), false);

        leg.marker_type_i                       = Utils::readNodeNumericAttrib<int>(xLeg, mxconst::get_CONV_ATTRIB_markerType(), 0);                     // 0 = default marker in combo
        leg.radius_to_display_3D_marker_in_nm_f = Utils::readNodeNumericAttrib<float>(xLeg, mxconst::get_CONV_ATTRIB_radius_to_display_marker(), 10.0f); // 10 = default display radius

        // Target Point
        auto xPoint = xLeg.getChildNode(mxconst::get_ELEMENT_POINT().c_str());
        if (!xPoint.isEmpty())
        {
          leg.p.node = xPoint.deepCopy();
          leg.p.parse_node();
        }

        // Target elevation
        auto xTargetTrig = xLeg.getChildNode(mxconst::get_ELEMENT_TRIGGER().c_str());
        if (!xTargetTrig.isEmpty())
        {

          leg.target_trig_strct.elev_combo_picked_i  = Utils::readNodeNumericAttrib<int>(xTargetTrig, mxconst::get_CONV_ATTRIB_elev_combo_picked_i(), 0);
          leg.target_trig_strct.slider_elev_value_i  = Utils::readNodeNumericAttrib<int>(xTargetTrig, mxconst::get_CONV_ATTRIB_slider_elev_value_i(), 0);
          leg.target_trig_strct.radius_of_trigger_mt = Utils::readNodeNumericAttrib<int>(xTargetTrig, mxconst::get_ATTRIB_LENGTH_MT(), 100); // trigger area, if not set than use 2000 meters

          leg.target_trig_strct.elev_min         = Utils::readNodeNumericAttrib<int>(xTargetTrig, mxconst::get_ATTRIB_ELEV_MIN_FT(), 0);
          leg.target_trig_strct.elev_max         = Utils::readNodeNumericAttrib<int>(xTargetTrig, mxconst::get_ATTRIB_ELEV_MAX_FT(), 0);
          leg.target_trig_strct.elev_rule_s      = Utils::readAttrib(xTargetTrig, mxconst::get_CONV_ATTRIB_elev_rule_s(), "");
          leg.target_trig_strct.elev_lower_upper = Utils::readAttrib(xTargetTrig, mxconst::get_ATTRIB_ELEV_MIN_MAX_FT(), "");
          leg.target_trig_strct.flag_on_ground   = Utils::readBoolAttrib(xTargetTrig, mxconst::get_CONV_ATTRIB_on_ground(), false);
        }

        // read <BUFFERS>
        const auto buf_i = xLeg.getChildNode(mxconst::get_ELEMENT_BUFFERS().c_str()).nChildNode(mxconst::get_ELEMENT_BUFF().c_str());

        assert(buf_i <= leg.MAX_ARRAY && "Convert state has more buffers than allowed.");

        for (int i2 = 0; i2 < buf_i; ++i2)
        {
          std::string buf_s = Utils::xml_read_cdata_node(xLeg.getChildNode(mxconst::get_ELEMENT_BUFFERS().c_str()).getChildNode(mxconst::get_ELEMENT_BUFF().c_str(), i2), "");
          leg.setBuff(i2, buf_s);
        }

        // read <triggers>
        leg.xTriggers = xLeg.getChildNode(mxconst::get_ELEMENT_TRIGGERS().c_str()).deepCopy();
        triggerCounter_i += leg.xTriggers.nChildNode(mxconst::get_ELEMENT_TRIGGER().c_str()); // count triggers

        // read <message_templates>
        leg.xMessageTmpl = xLeg.getChildNode(mxconst::get_ELEMENT_MESSAGE_TEMPLATES ().c_str()).deepCopy();

        // v3.0.303.7 Add Scriptlets
        leg.xLoadedScripts = Utils::xml_get_or_create_node(xLeg, mxconst::get_ELEMENT_SCRIPTS());

        assert(leg.xLoadedScripts.isEmpty() == false && " Scripts node is empty()");

        for (int i3 = 0; i3 < xLeg.nChildNode(mxconst::get_ELEMENT_SCRIPTLET().c_str()); ++i3)
        {
          leg.xLoadedScripts.addChild(xLeg.getChildNode(mxconst::get_ELEMENT_SCRIPTLET().c_str(), i3).deepCopy());
#ifndef RELEASE
          Utils::xml_print_node(leg.xLoadedScripts);
#endif // !RELEASE
        }


        // delete <leg>
        Utils::xml_delete_all_subnodes(leg.xLeg, "", true); // delete all child nodes including cdata This should allow a clean save format after loading the saved state

        Utils::addElementToMap(fpln, leg.indx, leg);
      } // if leg is valid

    } // end loop

    // set the trigger sequence:
    this->strct_conv_layer.trig_seq = triggerCounter_i + 1; // v3.0.304.4


    // Dataref information
    IXMLNode xXpdata = xMainNode.getChildNode(mxconst::get_ELEMENT_XPDATA().c_str()).deepCopy();
    if (!xXpdata.isEmpty())
    {
      std::string xpdata_4096_s;
      this->strct_conv_layer.xXPlaneDataRef_global = xXpdata.deepCopy();
      for (int i1 = 0; i1 < xXpdata.nChildNode(mxconst::get_ELEMENT_DATAREF().c_str()); ++i1)
      {
        IXMLRenderer render;
        xpdata_4096_s += render.getString(xXpdata.getChildNode(mxconst::get_ELEMENT_DATAREF().c_str(), i1));
      }

#ifdef IBM
      memcpy_s(this->strct_conv_layer.buff_dataref, sizeof(this->strct_conv_layer.buff_dataref), xpdata_4096_s.c_str(), sizeof(this->strct_conv_layer.buff_dataref) - 1);
#else
      memcpy(this->strct_conv_layer.buff_dataref, xpdata_4096_s.c_str(), sizeof(this->strct_conv_layer.buff_dataref) - 1);
#endif
      this->strct_conv_layer.xXPlaneDataRef_global = xXpdata.deepCopy();
    }

    // v3.305.1 store <global_settings> sub nodes in buffer
    auto xGlobalSetting = xMainNode.getChildNode(mxconst::get_GLOBAL_SETTINGS().c_str()).deepCopy();
    if (!xGlobalSetting.isEmpty())
    {
      std::string data_4096_s;
      for (int i1 = 0; i1 < xGlobalSetting.nChildNode(); ++i1) // read all sub elements
      {
        IXMLRenderer render;
        data_4096_s += render.getString(xGlobalSetting.getChildNode(i1));
      }

#ifdef IBM
      memcpy_s(this->strct_conv_layer.buff_globalSettings, sizeof(this->strct_conv_layer.buff_globalSettings), data_4096_s.c_str(), sizeof(this->strct_conv_layer.buff_globalSettings) - 1);
#else
      memcpy(this->strct_conv_layer.buff_globalSettings, data_4096_s.c_str(), sizeof(this->strct_conv_layer.buff_globalSettings) - 1);
#endif

      this->strct_conv_layer.xSavedGlobalSettingsNode = xGlobalSetting.deepCopy();
    }

  } // end else if


  return fpln;
}




// ------------ execActions --------------

void
WinImguiBriefer::execAction(mx_window_actions actionCommand)
{
  switch (actionCommand)
  {
    case missionx::mx_window_actions::ACTION_SHOW_WINDOW:
    {
      if (!this->GetVisible())
        this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
    }
    break;
    case missionx::mx_window_actions::ACTION_HIDE_WINDOW:
    {
      if (this->GetVisible())
        this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
    }
    break;
    case missionx::mx_window_actions::ACTION_TOGGLE_WINDOW:
    {
      this->toggleWindowState();
    }
    break;
    case missionx::mx_window_actions::ACTION_TOGGLE_BRIEFER:
    {
      if (this->adv_settings_strct.flag_firstTimeOpenBriefer)
      {
        // Initialize local day
        this->adv_settings_strct.iClockDayOfYearPicked = dataref_manager::getLocalDateDays();
        this->adv_settings_strct.iClockHourPicked      = dataref_manager::getLocalHour();
        this->adv_settings_strct.flag_firstTimeOpenBriefer ^= 1;

        // v24.03.1
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::load_notes_info);
      }

      if (this->GetVisible() && data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
      {
        const bool val_pause_in_2d = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_PAUSE_IN_2D(), mxconst::DEFAULT_AUTO_PAUSE_IN_2D);

        if (this->currentLayer == uiLayer_enum::flight_leg_info)
        {
          if (this->strct_flight_leg_info.internal_child_layer == missionx::uiLayer_enum::flight_leg_info_inventory || this->currentLayer == uiLayer_enum::flight_leg_info_map2d)
          {
            this->strct_flight_leg_info.resetChildLayer();

            if (!missionx::mxvr::vr_display_missionx_in_vr_mode && (this->IsPoppedOut() == false && val_pause_in_2d)) // toggle window only if in 2D mode and we are not popped out or we auto pause xplane in 2D mode
              this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);                   // should hide briefer window in 2D mode
          }
        } // end handling visible + flight_leg_info
        else
          this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW); // fixed bug when briefer was not toggeling in some visible cases
      }
      else
        this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
    }
    break;
    case missionx::mx_window_actions::ACTION_TOGGLE_CHOICE_WINDOW:
    {
      // hide choice window if in running state
      if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
      {
        this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
      }
      else
        this->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW);
    }
    break;
    case missionx::mx_window_actions::ACTION_POST_TEMPLATE_LOAD_DISPLAY_IMGUI_GENERATE_TEMPLATES_IMAGES:
    {
      this->strct_generate_template_layer.bFinished_loading_templates = true;
    }
    break;
    case missionx::mx_window_actions::ACTION_GENERATE_RANDOM_MISSION:
    {
      if (missionx::data_manager::flag_generate_engine_is_running) //
      {
        const std::string msg = "Random Engine is running. Please wait for it to finish first !!!";
        XPLMSpeakString(msg.c_str());
        missionx::Log::logMsg(msg); // debug
        this->setMessage(msg, 10);
      }
      else if (missionx::data_manager::flag_apt_dat_optimization_is_running) //
      {
        const std::string msg = "Can't Generate mission, apt dat optimization is currently running. Please wait for it to finish first !!!";
        XPLMSpeakString(msg.c_str());
        missionx::Log::logMsg(msg); // debug
        this->setMessage(msg, 10);
      }
      else
      {
        if (this->currentLayer != missionx::uiLayer_enum::option_user_generates_a_mission_layer)
          missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_USE_WEB_OSM_CHECKBOX(), false); // v3.0.253.4

        data_manager::setGenerateLayerFrom(this->currentLayer); // store the layer from which we called the generate random engine. This will help the RandomEngine decide if to read from UI interface or not.
        this->asyncSecondMessageLine.clear();
        missionx::data_manager::set_found_missing_3D_object_files(false); // v3.0.255.3 reset missing 3D files state

        // v25.02.1 Deprecated call to "mx_flc_pre_command::imgui_reload_templates_data_and_images", instead we will check and read template from "mx_flc_pre_command::imgui_generate_random_mission_file"
        // v25.01.2 - hotfix - When calling "[generate]" and not from the "template" screen, then we must initialize the templates, so we will have the "TemplateInfo" information for the "template_blank_4_ui.xml" file.
        if (this->currentLayer != missionx::uiLayer_enum::option_generate_mission_from_a_template_layer)
          missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::imgui_reload_templates_data_and_images); // since we have changed the flow in IMGUI, we have to initialize the templates list to make sure we have all templates available for the Random engine

        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::imgui_generate_random_mission_file);     // Special imgui flcPRE() directive
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_FETCH_FPLN_FROM_SIMBRIEF_SITE:
    {
      if (this->strct_ext_layer.simbrief_fetch_state == missionx::mxFetchState_enum::fetch_not_started)
      {
        const std::string pilot_id_s = Utils::getNodeText_type_6 (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_SIMBRIEF_PILOT_ID(), "");
        if (!pilot_id_s.empty ())
        {
          missionx::data_manager::tableExternalFPLN_simbrief_vec.clear ();          

          this->strct_flight_leg_info.resetNotesUpperPart (); // clear the upper part of the Flight Plan screen

          this->strct_ext_layer.simbrief_fetch_state = missionx::mxFetchState_enum::fetch_in_process;
          this->strct_ext_layer.threadState.init ();

          this->strct_ext_layer.simbrief_called_layer = this->getCurrentLayer ();
          missionx::data_manager::mFetchFutures.push_back (std::async (std::launch::async, missionx::data_manager::fetch_fpln_from_simbrief_site, &this->strct_ext_layer.threadState, pilot_id_s , &this->strct_ext_layer.simbrief_fetch_state, &this->strct_ext_layer.asyncFetchMsg_s));
          this->setMessage ("Fetching information from 'Simbrief site', may take up to one minute.", 8);
        }
        else 
          this->setMessage ("Check your Simbrief Pilot ID, It might be invalid or not set.", 8);
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_FETCH_FPLN_FROM_EXTERNAL_SITE:
    {
      if (this->strct_ext_layer.fetch_state == missionx::mxFetchState_enum::fetch_not_started)
      {
        this->strct_ext_layer.bDisplayPluginsRestart = false;
        missionx::data_manager::tableExternalFPLN_vec.clear();
        missionx::data_manager::indexPointer_forExternalFPLN_tableVector.clear();

        this->strct_ext_layer.fetch_state = missionx::mxFetchState_enum::fetch_in_process;        
        this->strct_ext_layer.threadState.init();// v24.06.1

        missionx::data_manager::mFetchFutures.push_back(std::async(std::launch::async, missionx::data_manager::fetch_fpln_from_external_site, &this->strct_ext_layer.threadState, missionx::data_manager::prop_userDefinedMission_ui.node, &this->strct_ext_layer.fetch_state, &this->strct_ext_layer.asyncFetchMsg_s));
        this->setMessage("Fetching information from 'external site', may take up to one minute.", 8); // v24.06.1
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_FETCH_ILS_AIRPORTS:
    {
      if (this->strct_ils_layer.fetch_ils_state == missionx::mxFetchState_enum::fetch_not_started)
      {
        missionx::data_manager::table_ILS_rows_vec.clear();
        missionx::data_manager::indexPointer_for_ILS_rows_tableVector.clear();

        this->strct_ils_layer.asyncFetchMsg_s.clear();
        this->strct_ils_layer.fetch_ils_state = missionx::mxFetchState_enum::fetch_in_process;

        missionx::data_manager::mFetchFutures.push_back(std::async(std::launch::async, missionx::data_manager::fetch_ils_rw_from_sqlite, &this->strct_ils_layer.navaid, &this->strct_ils_layer.filter_query_s, &this->strct_ils_layer.fetch_ils_state, &this->strct_ils_layer.asyncFetchMsg_s));
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_FETCH_NAV_INFORMATION:
    {
      if (this->strct_ils_layer.fetch_nav_state == missionx::mxFetchState_enum::fetch_not_started)
      {
        this->strct_ils_layer.mapNavaidData.clear(); // clear container data before filling it
        this->strct_ils_layer.asyncFetchMsg_s.clear(); // clear the progress message from the thread
        this->asyncSecondMessageLine.clear(); // clear the second type of message from the thread

        this->strct_ils_layer.fetch_metar_state = missionx::mxFetchState_enum::fetch_not_started; // we will call metar after Nav info will be fetched.
        this->strct_ils_layer.fetch_nav_state   = missionx::mxFetchState_enum::fetch_in_process;  // change the state of the "fetch" task to "in progress"
        this->strct_ils_layer.planePos          = missionx::dataref_manager::getPlanePointLocationThreadSafe(); // get plane position for distance calculations
        
        missionx::data_manager::mFetchFutures.push_back(std::async(std::launch::async, missionx::data_manager::fetch_nav_data_from_sqlite, &this->strct_ils_layer.mapNavaidData, &this->strct_ils_layer.sNavICAO, &this->strct_ils_layer.planePos, &this->strct_ils_layer.fetch_nav_state, &this->strct_ils_layer.asyncNavFetchMsg_s));
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_FETCH_MISSION_STATS: // v3.0.255.1
    {
      if (this->strct_flight_leg_info.fetch_state == missionx::mxFetchState_enum::fetch_not_started)
      {
        missionx::data_manager::mission_stats_from_query.reset();

        this->strct_flight_leg_info.asyncFetchMsg_s.clear();
        this->strct_flight_leg_info.fetch_state = missionx::mxFetchState_enum::fetch_in_process;

        missionx::data_manager::mFetchFutures.push_back(std::async(std::launch::async, missionx::data_manager::fetch_last_mission_stats, &this->strct_flight_leg_info.fetch_state, &this->strct_flight_leg_info.asyncFetchMsg_s));
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_START_RANDOM_MISSION:
    {
      // We need to make sure that the mission information needed by the load function is ready and accessible
      // we need to call MissionMenuHandler (Mission::MENU_OPEN_LIST_OF_MISSIONS: )
      // make sure this->mediaBriefer.selectedMissionKey = mxconst::get_RANDOM_MISSION_DATA_FILE_NAME() or equal to the template_mission_folder keyName (has no ".xml" extention // v3.0.241.10 b3 // reset and hide generate file button after
      // Successful creation we need to call missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::load_mission); // place action in Queue. Mission class will pick it and handle it. we need to call

      // clear flight plan upon mission start
      asyncSecondMessageLine.clear();

      const bool val_pause_in_2d = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_PAUSE_IN_2D(), mxconst::DEFAULT_AUTO_PAUSE_IN_2D);

      if (this->IsPoppedOut() == false && val_pause_in_2d && missionx::mxvr::vr_display_missionx_in_vr_mode == false) // v3.0.253.9.1 add more rules when to hide missionx main window
        this->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW);                           // v3.0.219.3 fix bug where window was not displayed but mouse did not register right click actions.

      #ifndef RELEASE
      missionx::Log::logDebugBO("[imguiWinBriefer] Pressed Start Random Mission. Will call Load and Start Mission too."); // debug
      #endif
      if (this->lastSelectedTemplateKey.find(mxconst::get_XML_EXTENSION()) != std::string::npos)
        missionx::data_manager::selectedMissionKey = mxconst::get_RANDOM_MISSION_DATA_FILE_NAME();
      else
        missionx::data_manager::selectedMissionKey = this->lastSelectedTemplateKey + mxconst::get_XML_EXTENSION(); // lastSelectedTemplateKey holds the folder keyName so we ned to add the extension

      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::start_random_mission); // place action in Queue. Mission class will pick it and handle it.

      // v3.0.253.3 reset current airport location
      if (this->getCurrentLayer() == missionx::uiLayer_enum::option_external_fpln_layer)
        this->strct_ext_layer.from_icao.clear();
    }
    break;
    case missionx::mx_window_actions::ACTION_LOAD_SAVEPOINT:
    {
      missionx::data_manager::selectedMissionKey = this->strct_pick_layer.last_picked_key;      // v3.0.251.1 b2 fix bug where load savepoint did not initialize missionx::data_manager::selectedMissionKey and the other classes were not aware of the picked mission keyName
      this->setMessage("Loading Savepoint, Please wait...", 5);                                 //
      missionx::Log::logDebugBO("[WinImguiBriefer] Pressed Load Savepoint.");                   // debug
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::load_savepoint); // place action in Queue. Mission class will pick it and handle it.
    }
    break;
    case missionx::mx_window_actions::ACTION_LOAD_MISSION:
    {
      // We need to make sure that the mission information needed by the load function is ready and accessible
      missionx::data_manager::selectedMissionKey = this->strct_pick_layer.last_picked_key;
      this->setMessage("Loading Mission, Please wait...", 10);                                //
      missionx::Log::logMsg("[WinImguiBriefer] Pressed Load Mission.");                       // debug
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::load_mission); // place action in Queue. Mission class will pick it and handle it.
    }
    break;
    case missionx::mx_window_actions::ACTION_START_MISSION:
    {
      if (missionx::data_manager::flag_apt_dat_optimization_is_running) // v3.0.219.12
      {
        std::string msg = "Can't start mission while apt.dat optimization is running. Please wait for it to finish first !!!";
        XPLMSpeakString(msg.c_str());
        missionx::Log::logMsg(msg); // debug
        this->setMessage(msg, 10);
      }
      else
      {
        asyncSecondMessageLine.clear();

        // We need to make sure that the mission information needed by the load function is ready and accessible
        missionx::Log::logDebugBO("[WinImguiBriefer] Pressed Start Mission.");                   // debug
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::start_mission); //
        this->setMessage("Starting mission... Please wait...", 10);                              //
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_QUIT_MISSION:
    {
      this->clearMessage();
      this->strct_flight_leg_info.resetChildLayer(); // reset to default child layer

      missionx::Log::logDebugBO("[WinImguiBriefer] Pressed Quit Mission.");                   // debug
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::stop_mission); // TODO: We should consider an intermediate state
    }
    break;
    case missionx::mx_window_actions::ACTION_QUIT_MISSION_AND_SAVE: // v3.0.251.1 b2
    {
      this->setMessage("Creating Savepoint and quitting mission, Please wait...", 5);
      this->strct_flight_leg_info.resetChildLayer(); // reset to default child layer

      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::create_savepoint_and_quit); //
    }
    break;
    case missionx::mx_window_actions::ACTION_GUESS_WAYPOINTS:
    {
      this->setMessage("Analyzing Fetched Data... Please Wait.", 5); // v24.06.1
      this->strct_ext_layer.fetch_state = missionx::mxFetchState_enum::fetch_guess_wp;
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::guess_waypoints_from_external_fpln_site); //
    }
    break;
    case missionx::mx_window_actions::ACTION_TOGGLE_MAP:
    {
      // 1. mission is running and there are map vecTextures in this goal then
      // 1.1 if briefer is close-set brieferLayer to flight_leg_info_map2d and toggle briefer window.
      // 1.2 if briefer is open - set brieferLayer to flight_leg_info_map2d

      if (this->GetVisible() && data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
      {
        this->setLayer(missionx::uiLayer_enum::flight_leg_info);
        if (this->strct_flight_leg_info.internal_child_layer == missionx::uiLayer_enum::flight_leg_info_map2d)
          this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info; // toggle between flight_leg_info_map2d and leg info
        else
          this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info_map2d;
      }
      else if (this->GetVisible() == false && data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
      {
        this->setLayer(missionx::uiLayer_enum::flight_leg_info);
        this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info_map2d;

        this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
      }
      else
      {
        this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info;
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_OPEN_STORY_LAYOUT: // v3.305.1
    {
      if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
      {
        // Force show window
        this->execAction(missionx::mx_window_actions::ACTION_SHOW_WINDOW);

        this->setLayer(missionx::uiLayer_enum::flight_leg_info);
        this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info_story_mode;
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_TOGGLE_INVENTORY:
    {
      // 1. mission is running
      // 1.1 if briefer is closed - set brieferLayer to "inventory" layer and toggle briefer window.
      // 1.2 if briefer is open - set brieferLayer to "inventory"
      // 1.3 if briefer in "Inventory" layer then close

      // missionx::data_manager::prepareInventoryCopies(this->strct_flight_leg_info.externalInventoryName);

      this->strct_flight_leg_info.reset_inv_image_zoom(); // v3.0.303.5 reset all zoom indexes to "-1"
      this->strct_flight_leg_info.resetItemMove();        // v24.12.2

      // Decide which layer to display
      if (this->GetVisible() && data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running) // Handle when mission is running and briefer is visible
      {
        this->setLayer(missionx::uiLayer_enum::flight_leg_info);

        if (this->strct_flight_leg_info.internal_child_layer == missionx::uiLayer_enum::flight_leg_info_inventory)
          this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info;
        else
          this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info_inventory;
      }
      else if (this->GetVisible() == false && data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
      {
        this->setLayer(missionx::uiLayer_enum::flight_leg_info);
        this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info_inventory;

        this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
      }
      else
      {
        this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info;
      }

      // v24.12.2 Prepare inventory copies and check stations existence and create if they are not present
      if (this->strct_flight_leg_info.internal_child_layer == missionx::uiLayer_enum::flight_leg_info_inventory)
      {
        // Clone inventories nodes of PLANE and the external inventory.
        missionx::data_manager::prepareInventoryCopies(this->strct_flight_leg_info.externalInventoryName);

        // all conditions must be met
        if ((data_manager::xplane_ver_i > missionx::XP12_VERSION_NO) && (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i != missionx::XP11_COMPATIBILITY))
        {
          // create station titles - // IS IT a REDUNDANT code ? Do we really need to refresh it if we don't change planes ?
          this->strct_flight_leg_info.mapStationHeaders.clear();
          for (const auto& [station_id, station_name] : data_manager::planeInventoryCopy.map_acf_station_names)
          {
            // We create inventory <station> nodes in data_manager::gather_acf_cargo_data()
            this->strct_flight_leg_info.mapStationHeaders[station_id].title = station_name; // set the title name based on station name
          }
        } // end if stations are not present
      }
      else
      {
        missionx::data_manager::planeInventoryCopy.reset_inventory_content();
        missionx::data_manager::externalInventoryCopy.reset_inventory_content();
      }
      // END v24.12.2
    }
    break;
    case missionx::mx_window_actions::ACTION_COMMIT_TRANSFER:
    {
      this->strct_flight_leg_info.reset_inv_image_zoom(); // v3.0.303.5

      // Call savepoint action
      if (!this->strct_flight_leg_info.externalInventoryName.empty())
      {
        // loop and erase any item with 0 quantity
        //missionx::data_manager::erase_items_with_zero_quantity_after_manual_inventory_transaction(); // 24.12.2 deprecate, use data_manager::erase_empty_inventory_item_nodes() instead

        #ifndef RELEASE        
        Log::log_to_missionx_log(fmt::format("\n================> Before deleting items with zero quantity:\n{}\n\n{}", Utils::xml_get_node_content_as_text(data_manager::planeInventoryCopy.node), Utils::xml_get_node_content_as_text(data_manager::externalInventoryCopy.node)));

        data_manager::erase_empty_inventory_item_nodes(data_manager::planeInventoryCopy.node);
        data_manager::erase_empty_inventory_item_nodes(data_manager::externalInventoryCopy.node);

        Log::log_to_missionx_log(fmt::format("\n================> After deleting items with zero quantity:\n{}\n\n{}", Utils::xml_get_node_content_as_text(data_manager::planeInventoryCopy.node), Utils::xml_get_node_content_as_text(data_manager::externalInventoryCopy.node)));

        #endif

        // calculate weight

        // copy xml back
        missionx::Log::log_to_missionx_log("Copying external inventory transaction: " + this->strct_flight_leg_info.externalInventoryName); // debug
        missionx::data_manager::mapInventories[this->strct_flight_leg_info.externalInventoryName] = missionx::data_manager::externalInventoryCopy;

        missionx::Log::log_to_missionx_log("Copying plane transaction."); // debug
        missionx::data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()] = missionx::data_manager::planeInventoryCopy;

        #ifndef RELEASE
        Log::log_to_missionx_log(fmt::format("\n================> New PLANE Inventory: [\n{}\n]", Utils::xml_get_node_content_as_text(data_manager::planeInventoryCopy.node)) ); // DEBUG
        #endif


        missionx::Log::logDebugBO("Calculating plane payload."); // debug
        // data_manager::current_plane_payload_weight_f = missionx::data_manager::calculatePlaneWeight(data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()], true, missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i); // v3.0.213.3 set weight according to mission
        data_manager::internally_calculateAndStorePlaneWeight (data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()], true, missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i); // v3.0.213.3 set weight according to mission
      }

      // missionx::Log::logDebugBO("return to flight_leg_info layer."); //
      // this->execAction(mx_window_actions::ACTION_TOGGLE_BRIEFER); // v25.03.3 disable closing inventory back to leg info
    }
    break;
    case missionx::mx_window_actions::ACTION_SHOW_END_SUMMARY_LAYER:
    {
      this->setLayer(missionx::uiLayer_enum::flight_leg_info);
      this->strct_flight_leg_info.internal_child_layer = missionx::uiLayer_enum::flight_leg_info_end_summary;
      if (!this->GetVisible())
        this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
    }
    break;
    case missionx::mx_window_actions::ACTION_CREATE_SAVEPOINT:
    {
      // Call savepoint action
      this->setMessage("Creating Savepoint, Please wait...", 5);                                  // v3.0.160
      missionx::Log::logMsg("[WinImgBrieferGL] Pressed Create Savepoint.");                       // debug
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::create_savepoint); //
    }
    break;
    case missionx::mx_window_actions::ACTION_ABORT_RANDOM_ENGINE_RUN:
    {
      if (missionx::RandomEngine::threadState.flagIsActive)
      {
        // Call savepoint action
        this->setMessage("Aborting, Please wait...", 5);                                               // v3.0.160
        missionx::Log::logMsg("[WinImgBrieferGL] Aborting RandomEngine.");                             // debug
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::abort_random_engine); //
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS: // v3.0.255.4.2
    {
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::save_user_setup_options); //
    }
    break;
    case missionx::mx_window_actions::ACTION_GENERATE_MISSION_FROM_LNM_FPLN: // v3.0.301
    {
      if (validate_conversion_table(this->strct_conv_layer.xConvMainNode, missionx::data_manager::map_tableOfParsedFpln))
      {
        // v3.303.14 added advance weather/time settings
        this->addAdvancedSettingsPropertiesBeforeGeneratingRandomMission();

        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::generate_mission_from_littlenavmap_fpln);
        this->asyncSecondMessageLine.clear(); // v3.303.8 erase last flight plan string. This is important since once we generate a mission file it is also the random mission in the "load mission" screen.
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME: // v3.0.303.11
    {
      this->generate_mission_date_based_on_user_preference(this->adv_settings_strct.iClockDayOfYearPicked, this->adv_settings_strct.iClockHourPicked, this->adv_settings_strct.iClockMinutesPicked, this->adv_settings_strct.flag_includeNightHours);
    }
    break;
    case missionx::mx_window_actions::ACTION_RESET_BRIEFER_POSITION: // v3.305.1
    {
      this->flag_displayedOnce = false;
      this->SetWindowPositioningMode(xplm_WindowCenterOnMonitor);
    }
    break;
    case missionx::mx_window_actions::ACTION_OPEN_NAV_LAYER: // v24025
    {
      if (this->strct_ils_layer.layer_state != missionx::mx_layer_state_enum::success_can_draw )
      {
        this->strct_ils_layer.layer_state = missionx::mx_layer_state_enum::not_initialized;       
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::imgui_check_presence_of_db_ils_data);
      }

      this->strct_ils_layer.flagNavigatedFromOtherLayer = true;
      this->setLayer(missionx::uiLayer_enum::option_ils_layer);
    }
    break;
    default:
      break;
  } // end switch actions
} // end execute actions
} // namespace missionx
