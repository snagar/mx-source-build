//
// Created by Saar.Nagar on 2026-08-10.
//

#ifndef MISSIONX_MX_IMG_WINDOW_H
#define MISSIONX_MX_IMG_WINDOW_H
#include <queue>

#include "../../../libs/imgui4xp/ImgWindow/ImgWindow.h"
#include "mx_colors.hpp"
#include "../../../libs/imgui4xp/imgui/implot/implot.h" // v3.0.255.1
#include "../MxUICore.hpp" // v3.303.14 holds Font related data
#include "../../core/xx_mission_constants.hpp" // v3.303.14 holds Font related data



namespace missionx {

class mx_img_window : public ImgWindow {
private:
  static int     iFontQueue;
  ImPlotContext* mImPlotContext = {nullptr};
public:
  // Inherits all constructors from ImgWindow automatically
  // using ImgWindow::ImgWindow;

  mx_img_window(int left, int top, int right, int bottom,
                  XPLMWindowDecoration decoration, XPLMWindowLayer layer,
                  bool cursors = true);

  ~mx_img_window();

  // Extend ImgWindow
  ImVec2 vec2_plus(ImVec2 v1, ImVec2 v2) { return ImVec2(v1.x + v2.x, v1.y + v1.y); };
  ImVec2 vec2_minus(ImVec2 v1, ImVec2 v2) { return ImVec2(v1.x - v2.x, v1.y - v1.y); };
  ImVec2 vec2_multi(ImVec2 v1, ImVec2 v2) { return ImVec2(v1.x * v2.x, v1.y * v1.y); };
  ImVec2 vec2_multi_num(ImVec2 v1, float fVal) { return ImVec2(v1.x * fVal, v1.y * fVal); };

  void toggleWindowState();
  // These functions where originally in "starter window" and in their oen namespace.
  // I don't see the reason not to move them into the base window class.
  // @brief Helper for creating unique IDs
  // @details Required when creating many widgets in a loop, e.g. in a table
  static void PushID_formatted(const char* format, ...); // v3.303.14

  static bool ButtonTooltip(const char* label,
        const char* tip = nullptr,
        ImU32 colFg = IM_COL32(1, 1, 1, 0),
        ImU32 colBg = IM_COL32(1, 1, 1, 0),
        const ImVec2& size = ImVec2(0, 0));

  static ImVec4 mx_get_color_as_im_vec4(const std::string& inColor_s); // used with mx_pad messages color

  // Use mxconst::get_TEXT_TYPE_xxx
  static void HelpMarker(const char* desc, ImVec4 inTextColor = missionx::color::color_vec4_white); // from IMGUI demo // v3.303.14 added default color
  static void mxUiHelpMarker(ImVec4 inTextColor,const char* desc); // saar, missionx. Prefer color first

  void mx_add_tooltip(ImVec4 inColor/* = missionx::color::color_vec4_white*/, const std::string& inTip) const;

  static ImVec4 mxConvertMxVec4ToImVec4(const missionx::mxVec4 & inMxVec4 ); // the function replaces a deprecated ImGui::GetWindowContentRegionWidth(). The new replacer functions does not seem to do the trick in some cases, so I have copied the logic of the original function into my own one in the hope it will continue and server me.
  static float mxUiGetContentWidth(); // the function replaces a deprecated ImGui::GetWindowContentRegionWidth(). The new replacer functions does not seem to do the trick in some cases, so I have copied the logic of the original function into my own one in the hope it will continue and server me.
  static float mxUiGetContentHeight(); //
  static ImVec2 mxUiGetWindowContentWxH(); //

  static void mxUiSetDefaultFont();
  static void mxUiResetAllFontsToDefault();

  static void mxUiSetFont(const std::string& inTextType); // set the font using the font type name liked TEXT_TYPE_TITLE_REG, TEXT_TYPE_TITLE_BIG, TEXT_TYPE_TEXT_REG
  static void mxUiReleaseLastFont(int inHowManyCycles = 1); // pop out Fonts we pushed, default is only the last one but you can release more than one. Updates iFontQueue.

  static bool mxStartUiDisableState(bool in_true_exp_to_disable); // v24.02.6 true means not disable
  static void mxEndUiDisableState(bool in_true_exp_to_disable);   // v24.02.6 true means not disable

  static bool mxUiButtonTooltip(const char* label, const char* tip = nullptr, ImVec4 colFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4 colBg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), const ImVec2& size = ImVec2(0, 0));



};

} // missionx

#endif //MISSIONX_MX_IMG_WINDOW_H
