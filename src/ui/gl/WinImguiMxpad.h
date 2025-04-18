#ifndef WINIMGUIMXPAD_H_
#define WINIMGUIMXPAD_H_
//#pragma once

#include "../../core/base_c_includes.h"
#include "../../core/data_manager.h"
#include "../../core/xx_mission_constants.hpp"


// Definitions for OpenFontIcons
#include <ImgWindow/ImgWindow.h> // inside libs/imgui4xp

namespace missionx
{

// Configure one-time setup like fonts
// void configureImgWindow();
// Cleanup one-time setup
// void cleanupAfterImgWindow();
// void CalcWinCoords(int& left, int& top, int& right, int& bottom);

class WinImguiMxpad : public ImgWindow
{
public:
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
    bool contains(const std::string& s) const;
  };
  typedef std::vector<tableDataTy> tableDataListTy;

protected:
  tableDataListTy tableList;

public:
  WinImguiMxpad(int                  left,
                int                  top,
                int                  right,
                int                  bot,
                //std::shared_ptr<ImgFontAtlas>& inFontAtlas,
                XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle, // xplm_WindowDecorationSelfDecoratedResizable
                XPLMWindowLayer      layer      = xplm_WindowLayerFloatingWindows);
  ~WinImguiMxpad() override;

protected:
  // Main function: creates the window's UI
  void buildInterface() override;

  bool wasHiddenByAutoHideOption; // will be filled by flc_autoHideOptionCheck()

private:


public:
  XPLMWindowID mWindow;
  XPLMWindowID optionsWindow{ 0 }; // v3.303.13 will hold a reference to the options window

  const static int WINDOWS_WIDTH;      
  const static int WINDOWS_MAX_HEIGHT; 
  const static int LINE_HEIGHT;

  int       win_pad{ 75 };      ///< distance from left and top border
  const int win_coll_pad{ 30 }; ///< offset of collated windows
  void      flc() override;
  void      execAction(mx_window_actions actionCommand); // special function to handle specific requests from outside of the window

  void flc_autoHideMXPAD();                                                            // check if MX-Pad needs to be hidden automatically because of the "Auto Hide Option" is set (OPT_AUTO_HIDE_SHOW_MXPAD)
  bool getWasHiddenByAutoHideOption() { return this->wasHiddenByAutoHideOption; }                //
  void setWasHiddenByAutoHideOption(bool inValue) { this->wasHiddenByAutoHideOption = inValue; } //
  void resetMxpadWindowPosition();                                                               // sould be called during start of mission, will set windows position at the top right.
  void reset();                                                                                  // reset variables for next reload of missions.
private:
  ImVec4 countdown_textColorVec4; // v3.305.3
  ImVec4 countdown_success_textColorVec4; // v25.02.1

  static const int BTN_WIDTH = 40;
  // float            scroll_y  = 0.0f;
  // float            scroll_max_y = 0.0f;
  size_t      qmm_counter = (size_t)0;
  std::string last_msg_s;

};

}
#endif // WINIMGUIMXPAD_H_