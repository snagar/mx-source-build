#include "WinImguiMxpad.h"
#include "../../core/QueueMessageManager.h"

// All our headers combined
//#include <imgui4xp.h>
//#include <imgui/imgui.h>

// Image processing (for reading "imgui_demo.jpg"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


//
// MARK: Global data and functions
//

// The raw TTF data of OpenFontIcons has been generated into the following file
#include "fa-solid-900.inc"
#include <IconsFontAwesome5.h> // inside libs/imgui4xp

namespace missionx
{
const int WinImguiMxpad::WINDOWS_WIDTH      = 410;
const int WinImguiMxpad::WINDOWS_MAX_HEIGHT = 170;
const int WinImguiMxpad::LINE_HEIGHT        = 20;

//////////////////////// Global Namespace variables //////////////////////////
int missionx::WinImguiMxpad::num_win;

///// Calculate window's standard coordinates -> Moved to Utils



//////////////////////// END Global Namespace variables //////////////////////////
}

//////////////////////// CLASS Members  //////////////////////////
namespace missionx
{

WinImguiMxpad::WinImguiMxpad(int left, int top, int right, int bot, XPLMWindowDecoration decoration, XPLMWindowLayer layer)
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
    // at the top of the window, ie. the window can be moved by
    // dragging a spot near the window's top
    SetWindowDragArea(0, 5, INT_MAX, 5 + 2 * int(data_manager::FONT_SIZE));
  }

  // Define our own window title
  // SetWindowTitle("Imgui v" IMGUI_VERSION " for X-Plane  by William Good");
  SetWindowTitle("MX-PAD");
  SetWindowResizingLimits(WinImguiMxpad::WINDOWS_WIDTH, WinImguiMxpad::WINDOWS_MAX_HEIGHT, WinImguiMxpad::WINDOWS_WIDTH, WinImguiMxpad::WINDOWS_MAX_HEIGHT); // minW. minH. maxW, maxH
  ImgWindow::SetVisible (false);

  this->mWindow = this->GetWindowId();
}

WinImguiMxpad::~WinImguiMxpad() {}


void
WinImguiMxpad::buildInterface()
{
  // this->GetCurrentWindowGeometry(l, t, r, b); // get current window geometry

  constexpr static const float TOP_BUTTON_SIZE = 14.0f;
  constexpr ImVec2             size            = { TOP_BUTTON_SIZE, TOP_BUTTON_SIZE };
  constexpr ImVec2             uv0             = { 0.0f, 0.0f };
  constexpr ImVec2             uv1             = { 1.0f, 1.0f };
  constexpr static auto        TITLE           = "MX-PAD";
  std::string                  err;
  const int                    autoHideOptionVal         = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_HIDE_SHOW_MXPAD(), false);
  const std::string            autoHideTextureState_name = (autoHideOptionVal) ? mxconst::get_BITMAP_AUTO_HIDE_EYE_FOCUS() : mxconst::get_BITMAP_AUTO_HIDE_EYE_FOCUS_DISABLED(); // used on mxpad upper toolbar. Display auto hide state

  if (!this->GetVisible())
    return;


  /////////// TITLE + Quick Bar ///////////////
  ImGui::BeginGroup();
  {

    const std::string title = (missionx::data_manager::formated_fail_timer_as_text.empty()) ? TITLE : missionx::data_manager::formated_fail_timer_as_text;

    // Title timer
    {
      ImGui::TextColored(this->countdown_textColorVec4, "%s", title.c_str()); // v3.305.3 changed to TextColored      
    }

    // v25.02.1 Display Success timer countdown
    if (!missionx::data_manager::strct_success_timer_info.triggerNameWithShortestSuccessTimer.empty())
    {
      if (missionx::data_manager::lowestFailTimerName_s.empty())
        ImGui::SameLine(0.0f, 5.0f);
      else
        ImGui::SameLine();

      ImGui::TextColored(this->countdown_success_textColorVec4, "%s", fmt::format("{}{}", (missionx::data_manager::lowestFailTimerName_s.empty())? "ST: " : "| ST: "
                                                                                                     , missionx::data_manager::mapTriggers[missionx::data_manager::strct_success_timer_info.triggerNameWithShortestSuccessTimer].timer.get_formated_timer_as_text("")
                                                                                     ).c_str() );
    }


    {
      ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);            // transparent
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 160, 0, 255)); // green

      // inventory hint button
      ImGui::SameLine(0.0f, 10.0f);
      if (!missionx::data_manager::active_external_inventory_name.empty())
      {
        if (ImGui::ImageButton("InvMxPadButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_INVENTORY_MXPAD()].gTexture)), size, uv0, uv1)) // padding 2
        {
          missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::open_inventory_layout);
        }
      }

      // map / information
      ImGui::SameLine(0.0f, 10.0f);
      if (!missionx::data_manager::maps2d_to_display.empty())
      {
        if (ImGui::ImageButton("MapMxPadButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_MAP_MXPAD()].gTexture)), size, uv0, uv1)) // padding 2
        {
          missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::open_map_layout);
        }
      }

      // auto show/hide mxpad
      ImGui::SameLine(0.0f, 10.0f);
      if (ImGui::ImageButton("AutoHideMxPadButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[autoHideTextureState_name].gTexture)), size, uv0, uv1)) // padding 2
      {
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::toggle_auto_hide_show_mxpad_option);
      }

      // Target Marker status icon
      ImGui::SameLine(0.0f, 10.0f);
      const bool bDisplayMarkers = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS(), true);

      if (!bDisplayMarkers)
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.25f);

      if (ImGui::ImageButton("ToggleTargetMarkerMxPadButtonImage", reinterpret_cast<void *> (static_cast<intptr_t> (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_TARGET_MARKER_ICON()].gTexture)), size, uv0, uv1)) // padding 2
      {
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::toggle_target_marker_option);
      }

      if (!bDisplayMarkers)
        ImGui::PopStyleVar();


      // Restore colors
      ImGui::PopStyleColor(2);

    } // end anonymous block


    // If we are a transparent HUD-like window then we draw 3 lines that look
    // a bit like a head...so people know where to drag the window to move it
    if (HasWindowDragArea())
    {
      ImGui::SameLine();
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      ImVec2      pos_start = ImGui::GetCursorPos();
      float       x_end     = mxUiGetContentWidth() - 75; // v3.0.255.4.3 was: ImGui::GetWindowContentRegionWidth() - 75;
      for (int i = 0; i < 3; i++)
      {
        draw_list->AddLine(pos_start, { x_end, pos_start.y }, IM_COL32(0xa0, 0xa0, 0xa0, 255), 1.0f);
        pos_start.y += 5;
      }
    }
  }
  ImGui::EndGroup(); // end title group

  // Button with fixed width 30 and standard height
  // to pop out the window in an OS window
  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
  static float btnWidth   = ImGui::CalcTextSize(mxUtils::from_u8string( ICON_FA_WINDOW_MAXIMIZE ).c_str() ).x + 5;
  this->mxUiReleaseLastFont();

  const bool   bBtnPopOut = !this->IsPoppedOut();
  const bool   bBtnPopIn  = IsPoppedOut() || IsInVR();
  int numBtn = bBtnPopOut + bBtnPopIn; // +bBtnVR;

  if (numBtn > 0)
  {
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());

      // Setup colors for window sizing buttons
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive));    // dark gray
      ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);                              // transparent
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_ScrollbarGrab)); // lighter gray

      if (bBtnPopIn)
      {
        // Same line, but right-aligned
        ImGui::SameLine(mxUiGetContentWidth() - (numBtn * btnWidth));
        if (this->ButtonTooltip(mxUtils::from_u8string(ICON_FA_WINDOW_RESTORE).c_str(), "Move back into X-Plane"))
          nextWinPosMode = xplm_WindowPositionFree;
        --numBtn;
      }
      if (bBtnPopOut)
      {
        // Same line, but right-aligned
        ImGui::SameLine(mxUiGetContentWidth() - (numBtn * btnWidth));
        if (this->ButtonTooltip(mxUtils::from_u8string(ICON_FA_WINDOW_MAXIMIZE).c_str(), "Pop out into separate window"))
          nextWinPosMode = xplm_WindowPopOut;
        --numBtn;
      }

      // Restore colors
      ImGui::PopStyleColor(3);

    this->mxUiReleaseLastFont();

  } // end drawing popout/in buttons

  ////// Display text //////
  /*------------------------------------------------
   *                  Text Messages
   *------------------------------------------------*/
  {

    ImGui::BeginChild("mxpad2d_textMsg", ImVec2(0.0f, static_cast<float> (WINDOWS_MAX_HEIGHT) - 50.0f)); // -50 = top title

    std::string store_last_message_s;
    for (const auto &msg : missionx::QueueMessageManager::mxpad_messages) // calculate the overall height of all messages lines
    {
      this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1 mxpad font

        ImGui::BeginGroup();
        ImGui::TextColored(missionx::WinImguiMxpad::getColorAsImVec4(msg.label_color) , "%s", msg.label.c_str());

        ImGui::EndGroup();

        ImGui::BeginGroup();
        ImGui::SameLine(45.0f, 2.0f);
        ImGui::TextWrapped("%s", msg.message_text.c_str());
        store_last_message_s = msg.message_text;
        ImGui::EndGroup();

      this->mxUiReleaseLastFont();
    }
    // Should we move the scroll ?
    if ( store_last_message_s != this->last_msg_s ) // if last message in queue is not the same as the stored last message
    {
      //__debugbreak();
      this->last_msg_s = store_last_message_s;
      ImGui::SetScrollHereY(1.0f);
      ++this->qmm_counter;
    }

    ImGui::EndChild();
  }



} // buildInterface


void
WinImguiMxpad::execAction (const mx_window_actions actionCommand)
{
  switch (actionCommand)
  {
    case missionx::mx_window_actions::ACTION_HIDE_WINDOW:
    {
      if (this->GetVisible())
      {
        this->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
      }
    }
    break;
    case missionx::mx_window_actions::ACTION_TOGGLE_WINDOW:
    {
      this->toggleWindowState();

      if (this->GetVisible())
      {
        ImGui::SetWindowFocus();
        if (!XPLMIsWindowInFront(this->mWindow))
          XPLMBringWindowToFront(this->mWindow);
      }
    }
    break;
    default:
      break;

  } // end switch actions
}


void
WinImguiMxpad::flc()
{

  // handle pop windows
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


  // v3.0.215.1 implement the auto hide option
  if (Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_HIDE_SHOW_MXPAD(), false) && !this->IsPoppedOut()) // if auto hide and not popped out
    flc_autoHideMXPAD();

  //// v3.0.221.7
  if (data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
  {
    if (this->GetVisible())
    {      
      // v3.305.3 moved code from buildinstance to the flc()
      if (missionx::data_manager::lowestFailTimerName_s.empty())
        this->countdown_textColorVec4 = missionx::color::color_vec4_white;
      else
      {
        // v3.305.3 if we need to abort mission on timeout
        if (Utils::readBoolAttrib(missionx::data_manager::mapFailureTimers[missionx::data_manager::lowestFailTimerName_s].node, mxconst::get_ATTRIB_FAIL_ON_TIMEOUT_B(), true))
          this->countdown_textColorVec4 = missionx::color::color_vec4_plum;
        else
          this->countdown_textColorVec4 = missionx::color::color_vec4_beige;
      }


      if (missionx::mxvr::vr_display_missionx_in_vr_mode)
        this->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW);

      // v25.02.1
      if (!missionx::data_manager::strct_success_timer_info.triggerNameWithShortestSuccessTimer.empty())
      {
        if (missionx::data_manager::mapTriggers[missionx::data_manager::strct_success_timer_info.triggerNameWithShortestSuccessTimer].timer.getRemainingTime() < 4.0f)
          this->countdown_success_textColorVec4 = missionx::color::color_vec4_lime;
        else
          this->countdown_success_textColorVec4 = missionx::color::color_vec4_deepskyblue;
      }


    }
  }

} // flc


void
WinImguiMxpad::flc_autoHideMXPAD()
{

  if ((missionx::mxvr::vr_display_missionx_in_vr_mode || missionx::mxvr::flag_in_vr))
  {
    if (this->GetVisible())
      this->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW);

    return; // skip this code since we should not draw in VR mode
  }
  else if (this->GetVisible()) // hide if visible and no messages are broadcasting
  {
    if (QueueMessageManager::listPoolMsg.empty() && QueueMessageManager::listPadQueueMessages.empty() && missionx::data_manager::active_external_inventory_name.empty() 
        && (XPLMGetWindowIsVisible(this->optionsWindow) == false) ) /* nbSel holds number of selection options being displayed*/
    {
      execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
      this->wasHiddenByAutoHideOption = true; // need to come after the action since we always reset this value to false in execAction function.
    }
  }
  else // show mx-pad if there are
  {
    if (!QueueMessageManager::listPoolMsg.empty() || !QueueMessageManager::listPadQueueMessages.empty() 
     || !missionx::data_manager::active_external_inventory_name.empty() || XPLMGetWindowIsVisible(this->optionsWindow))
    {
      execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
      this->wasHiddenByAutoHideOption = true; // need to come after the action since we always reset this value to false in execAction function.
    }
  }
}


void
WinImguiMxpad::resetMxpadWindowPosition()
{
  missionx::MxUICore::refreshGlobalDesktopBoundsValues ();

  const int l = MxUICore::global_desktop_bounds[2] - WINDOWS_WIDTH - 10;
  const int b = MxUICore::global_desktop_bounds[3] - WINDOWS_MAX_HEIGHT - 60;
  const int r = l + WINDOWS_WIDTH;      // right
  const int t = b + WINDOWS_MAX_HEIGHT; // top

  XPLMSetWindowGeometry(this->GetWindowId(), l, t, r, b);
}

void
WinImguiMxpad::reset()
{
  this->SetVisible(false);
}

} // missionx
