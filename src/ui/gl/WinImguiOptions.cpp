#include "WinImguiOptions.h"

#define STB_IMAGE_IMPLEMENTATION

//
// Global data and functions
//

// The raw TTF data of OpenFontIcons has been generated into the following file
#include "fa-solid-900.inc"

namespace missionx
{
const int WinImguiOptions::MAX_WIDTH             = 410;
const int WinImguiOptions::LINE_HEIGHT           = 25;
const int WinImguiOptions::OPTION_BOTTOM_PADDING = 10;

//////////////////////// Global Namespace variables //////////////////////////
int missionx::WinImguiOptions::num_win;

//////////////////////// END Global Namespace variables //////////////////////////
}

//////////////////////// CLASS Members  //////////////////////////
namespace missionx
{

void
WinImguiOptions::flc()
{
  // do not display in VR
  if (missionx::mxvr::vr_display_missionx_in_vr_mode)
  {
    this->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW);
  }
  else if (this->nextWinPosMode >= 0)
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



} // flc

// --------------------------------------

WinImguiOptions::WinImguiOptions(int left, int top, int right, int bot, XPLMWindowDecoration decoration, XPLMWindowLayer layer)
  : ImgWindow(left, top, right, bot, decoration, layer)
  , myWinNum(++num_win) // assign a unique window number
{
  // Disable reading/writing of "imgui.ini"
  ImGuiIO& io    = ImGui::GetIO();
  io.IniFilename = nullptr;

  // We take the parameter combination "SelfDecorateResizable" + "LayerFlightOverlay"
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
  SetWindowTitle(std::string("Mission-X v") + missionx::FULL_VERSION);
  SetWindowResizingLimits(150, 80, WinImguiOptions::MAX_WIDTH, 130); // minW. minH. maxW, maxH
  ImgWindow::SetVisible (false);

  this->mWindow = this->GetWindowId();
}

//WinImguiOptions::~WinImguiOptions() {}


void
WinImguiOptions::buildInterface()
{
  const std::string title_s = (missionx::data_manager::mxChoice.currentChoiceTitle_s.empty()) ? "Pick an Option " : missionx::data_manager::mxChoice.currentChoiceTitle_s;

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // v3.305.1
  ImGui::TextColored( missionx::color::color_vec4_yellow, "%s", title_s.c_str());
  this->mxUiReleaseLastFont(); // v3.305.1

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
    static float btnWidth   = ImGui::CalcTextSize(ICON_FA_WINDOW_MAXIMIZE).x + 5;
  this->mxUiReleaseLastFont();

  const bool   bBtnPopOut = !this->IsPoppedOut();
  const bool bBtnVR = missionx::mxvr::vr_display_missionx_in_vr_mode && !IsInVR();
  int        numBtn = bBtnPopOut + bBtnVR; // +bBtnPopIn;

  if (numBtn > 0)
  {
    this->mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());

      // Setup colors for window sizing buttons
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive));    // dark gray
      ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);                              // transparent
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_ScrollbarGrab)); // lighter gray

      if (bBtnVR)
      {
        // Same line, but right-alinged
        ImGui::SameLine(mxUiGetContentWidth() - (numBtn * btnWidth));
        if (this->ButtonTooltip(ICON_FA_EXTERNAL_LINK_SQUARE_ALT, "Move into VR"))
          nextWinPosMode = xplm_WindowVR;
        --numBtn;
      }

      if (bBtnPopOut)
      {
        // Same line, but right-alinged
        ImGui::SameLine(mxUiGetContentWidth() - (numBtn * btnWidth));
        if (this->ButtonTooltip(ICON_FA_WINDOW_MAXIMIZE, "Pop out into separate window"))
          nextWinPosMode = xplm_WindowPopOut;
        --numBtn;
      }

      // Restore colors
      ImGui::PopStyleColor(3);

    this->mxUiReleaseLastFont();

    // Window mode should be set outside drawing calls to avoid crashes
    if (nextWinPosMode >= 0 && flId)
      XPLMScheduleFlightLoop(flId, -1.0, 1);
  } // end drawing popout/in buttons


  // Display title text


  // loop over options

  this->mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // v3.305.1
  ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 160, 0, 255)); // Green
  for (int i1 = 0; i1 < missionx::data_manager::mxChoice.nVecSize_i; ++i1) // mxChoice.mapOptions size must be equal to :mxChoice.vecXmlOptions
  {
    int radioChoice = -1;
    if (Utils::isElementExists(missionx::data_manager::mxChoice.mapOptions, i1))
    {

      if (!missionx::data_manager::mxChoice.mapOptions[i1].flag_hidden) // check if to hide the option
      {

        if (ImGui::RadioButton(missionx::data_manager::mxChoice.mapOptions[i1].text.c_str(), radioChoice == i1))
        {
          radioChoice = i1;
          // flag_picked_one_of_the_buttons = true;
          if (missionx::data_manager::mxChoice.optionPicked_key_i != missionx::data_manager::mxChoice.mapOptions[i1].key_i) // we test this so we won't have repeating same action calls
          {
            missionx::data_manager::mxChoice.optionPicked_key_i = missionx::data_manager::mxChoice.mapOptions[i1].key_i;
            missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::handle_option_picked_from_choice);
#ifndef RELEASE
            Log::logDebugBO("[draw choice options] Picked: " + Utils::formatNumber<int>(missionx::data_manager::mxChoice.mapOptions[i1].key_i) + ", " + missionx::data_manager::mxChoice.mapOptions[i1].text); // debug
#endif
          }
        }
      }
    }
  } // end loop over all options
  ImGui::PopStyleColor(1);
  this->mxUiReleaseLastFont(); // v3.305.1

} // buildInterface


void
WinImguiOptions::execAction(missionx::mx_window_actions actionCommand)
{
  switch (actionCommand)
  {
    case missionx::mx_window_actions::ACTION_HIDE_WINDOW:
    {
      if (this->GetVisible())
        this->toggleWindowState();
    }
    break;
    case missionx::mx_window_actions::ACTION_TOGGLE_WINDOW:
    case missionx::mx_window_actions::ACTION_TOGGLE_CHOICE_WINDOW:
    {
      // hide choice window if in running state
      if (missionx::mxvr::vr_display_missionx_in_vr_mode || missionx::mxvr::flag_in_vr || this->IsInVR())
      {
        this->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW); // always hide in VR mode
      }
      else if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running) // if in running and not popped out then we can toggle freely
      {
        if (!this->IsPoppedOut())
          this->toggleWindowState();
      }
      else
        this->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW); // always hide if not in running
    }
    break;
    default:
      break;

  } // end switch actions
}

} // missionx
