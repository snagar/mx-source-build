#ifndef IMGUIOPTIONSWINDOW_H_
#define IMGUIOPTIONSWINDOW_H_
//#pragma once

#include "../../core/base_c_includes.h"
#include "../../core/data_manager.h"
#include "../../core/xx_mission_constants.hpp"


// Definitions for OpenFontIcons
#include <IconsFontAwesome5.h>   // inside libs/imgui4xp
#include <ImgWindow/ImgWindow.h> // inside libs/imgui4xp

namespace missionx
{

// Configure one-time setup like fonts
// void configureImgWindow();
// Cleanup one-time setup
void cleanupAfterImgWindow();
// void CalcWinCoords(int& left, int& top, int& right, int& bottom);

class WinImguiOptions : public ImgWindow
{
protected:
  ///// @brief Helper for creating unique IDs
  ///// @details Required when creating many widgets in a loop, e.g. in a table
  // void PushID_formatted(const char* format, ...)    IM_FMTARGS(1);

  ///// @brief Button with on-hover popup helper text
  ///// @param label Text on Button
  ///// @param tip Tooltip text when hovering over the button (or NULL of none)
  ///// @param colFg Foreground/text color (optional, otherwise no change)
  ///// @param colBg Background color (optional, otherwise no change)
  ///// @param size button size, 0 for either axis means: auto size
  // bool ButtonTooltip(const char* label,
  //  const char* tip = nullptr,
  //  ImU32 colFg = IM_COL32(1, 1, 1, 0),
  //  ImU32 colBg = IM_COL32(1, 1, 1, 0),
  //  const ImVec2& size = ImVec2(0, 0));

public:
  // Counter for the number of windows opened
  static int num_win;
  // I am window number...
  const int myWinNum;
  // Note to myself that a change of window mode is requested
  XPLMWindowPositioningMode nextWinPosMode = -1;
  // Our flight loop callback in case we need one
  XPLMFlightLoopID flId = nullptr;

public:
  struct tableDataTy
  {
    std::string reg;
    std::string model;
    std::string typecode;
    std::string owner;
    float       heading   = 0.0f;
    bool        turnsLeft = false;
    bool        filtered  = true; // included in search result?

    // is s (upper-cased!) in any text?
    // bool contains(const std::string& s) const;
  };
  typedef std::vector<tableDataTy> tableDataListTy;

protected:
  tableDataListTy tableList;

public:
  WinImguiOptions(int                  left,
                  int                  top,
                  int                  right,
                  int                  bot,
                  //std::shared_ptr<ImgFontAtlas>& inFontAtlas,
                  XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle, // xplm_WindowDecorationSelfDecoratedResizable
                  XPLMWindowLayer      layer      = xplm_WindowLayerFloatingWindows);
  //~WinImguiOptions() override;

protected:
  // Main function: creates the window's UI
  void buildInterface() override;

public:
  XPLMWindowID     mWindow;
  const static int MAX_WIDTH;
  // v3.0.241.6 modified from 400 to be on same size as mxPad;
  const static int LINE_HEIGHT;
  const static int OPTION_BOTTOM_PADDING;

  int       win_pad{ 75 };      ///< distance from left and top border
  const int win_coll_pad{ 30 }; ///< offset of collated windows

  void flc() override;
  void execAction(missionx::mx_window_actions actionCommand); // special function to handle specific requests from outside of the window
};

}
#endif // IMGUIOPTIONSWINDOW_H_
