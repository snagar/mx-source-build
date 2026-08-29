//
// Created by Saar.Nagar on 2026-08-10.
//
#include <cassert>
#include "mx_img_window.h"
#include "../../../libs/imgui4xp/imgui/imgui_internal.h"

namespace missionx {
int            mx_img_window::iFontQueue{ 0 };

mx_img_window::mx_img_window(const int left, const int top, const int right, const int bottom,
                const XPLMWindowDecoration decoration, const XPLMWindowLayer layer,
                const bool cursors)
      : ImgWindow(left, top, right, bottom, decoration, layer, cursors)
{
  mImPlotContext = ImPlot::CreateContext();
}


mx_img_window::~mx_img_window()
{
  if (mImPlotContext) {
    ImPlot::DestroyContext(mImPlotContext);
  }
}

void mx_img_window::toggleWindowState()
{
  if (this->GetVisible())
    this->SetVisible(false); // hide
  else
    this->SetVisible(true); // show
}

void mx_img_window::PushID_formatted(const char* format, ...)
{
  // format the variable string
  va_list args;
  char    sz[500];
  va_start(args, format);
  vsnprintf(sz, sizeof(sz), format, args);
  va_end(args);
  // Call the actual push function
  ImGui::PushID(sz);
}

bool mx_img_window::ButtonTooltip(const char* label, const char* tip, ImU32 colFg, ImU32 colBg, const ImVec2& size)
{
  // Setup colors
  if (colFg != IM_COL32(1, 1, 1, 0))
    ImGui::PushStyleColor(ImGuiCol_Text, colFg);
  if (colBg != IM_COL32(1, 1, 1, 0))
    ImGui::PushStyleColor(ImGuiCol_Button, colBg);

  // do the button
  bool b = ImGui::Button(label, size);

  // restore previous colors
  if (colBg != IM_COL32(1, 1, 1, 0))
    ImGui::PopStyleColor();
  if (colFg != IM_COL32(1, 1, 1, 0))
    ImGui::PopStyleColor();

  // do the tooltip
  if (tip && ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", tip);

  // return if button pressed
  return b;
}

ImVec4 mx_img_window::mx_get_color_as_im_vec4(const std::string& inColor_s)
{
  if ( missionx::mxconst::get_YELLOW() == inColor_s)
    return {1.0f, 1.0f, 0.0f, 1.0};
  else if (missionx::mxconst::get_WHITE() == inColor_s)
    return {1.0f, 1.0f, 1.0f, 1.0};
  else if (missionx::mxconst::get_GREEN() == inColor_s)
    return {0.0f, 1.0f, 0.0f, 1.0};
  else if (missionx::mxconst::get_RED() == inColor_s)
    return {1.0f, 0.0f, 0.0f, 1.0};
  else if (missionx::mxconst::get_BLUE() == inColor_s)
    return {0.0f, 0.0f, 1.0f, 1.0};
  else if (missionx::mxconst::get_ORANGE() == inColor_s)
    return {1.0f, 0.5f, 0.0f, 1.0};
  else if (missionx::mxconst::get_PURPLE() == inColor_s)
    return {1.0f, 0.0f, 1.0f, 1.0};
  else if (missionx::mxconst::get_BLACK() == inColor_s)
    return {0.0f, 0.0f, 0.0f, 1.0};

  return {1.0f, 1.0f, 1.0f, 1.0}; // white
}

void mx_img_window::HelpMarker(const char* desc, ImVec4 inTextColor)
{
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered())
  {
    mx_img_window::mxUiSetFont(mxconst::get_TEXT_TYPE_FIX_HINT_MED());

    ImGui::PushStyleColor(ImGuiCol_Text, inTextColor);

    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();

    ImGui::PopStyleColor();

    mx_img_window::mxUiReleaseLastFont();
  }
}

void mx_img_window::mxUiHelpMarker(ImVec4 inTextColor, const char* desc)
{
  HelpMarker(desc, inTextColor);
}

void mx_img_window::mx_add_tooltip(const ImVec4 inColor, const std::string& inTip) const
{
  if (!IsInVR() && ImGui::IsItemHovered())
  {
    ImGui::PushStyleColor(ImGuiCol_Text, inColor); // dark gray

    ImGui::BeginTooltip();
    // ImGui::PushItemWidth(200.0f);
    ImGui::Text("%s", inTip.c_str());
    // ImGui::PopItemWidth();
    ImGui::EndTooltip();

    ImGui::PopStyleColor(1);
  }
}

ImVec4 mx_img_window::mxConvertMxVec4ToImVec4(const missionx::mxVec4& inMxVec4)
{
  return {inMxVec4.x, inMxVec4.y, inMxVec4.z, inMxVec4.w};
}

float mx_img_window::mxUiGetContentWidth()
{
  const ImGuiWindow* window = GImGui->CurrentWindow;
  return window->ContentRegionRect.GetWidth();
}

float mx_img_window::mxUiGetContentHeight()
{
  const ImGuiWindow* window = GImGui->CurrentWindow;
  return window->ContentRegionRect.GetHeight();
}

ImVec2 mx_img_window::mxUiGetWindowContentWxH()
{
  return {mxUiGetContentWidth(), mxUiGetContentHeight()};
}

void mx_img_window::mxUiSetDefaultFont()
{
  missionx::mx_img_window::mxUiResetAllFontsToDefault();
}

void mx_img_window::mxUiResetAllFontsToDefault()
{
  while (missionx::mx_img_window::iFontQueue--)
    ImGui::PopFont();

  missionx::mx_img_window::iFontQueue = 0;
}

void mx_img_window::mxUiSetFont(const std::string& inTextType)
{
  assert(!missionx::MxUICore::mapFontTypeMeta.empty() && "No fonts were loaded.");

  // read font meta data for inTextType. Holds Font size.
  if (missionx::MxUICore::mapFontTypeMeta.contains(inTextType))
  {
    const auto ftm = missionx::MxUICore::mapFontTypeMeta[inTextType];
    missionx::MxUICore::mapFontTypesBeingUsedInProgram[inTextType] = ftm.id;

    ImGui::PushFont(ImgWindow::sFontAtlas->getAtlas()->Fonts[ftm.id], ftm.fSizePx);
    ++missionx::mx_img_window::iFontQueue;
  }
}

void mx_img_window::mxUiReleaseLastFont(const int inHowManyCycles)
{
  for (int loop1 = 0; loop1 < inHowManyCycles; ++loop1)
  {
    if (missionx::mx_img_window::iFontQueue > 0)
    {
      ImGui::PopFont();
      missionx::mx_img_window::iFontQueue--;
    }
  }
}

bool mx_img_window::mxStartUiDisableState(const bool in_true_exp_to_disable)
{
  if (!in_true_exp_to_disable) // if expression is false, skip.
    return in_true_exp_to_disable;

  ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

  return in_true_exp_to_disable;
}

void mx_img_window::mxEndUiDisableState(const bool in_true_exp_to_disable)
{
  if (! in_true_exp_to_disable)
    return;

  ImGui::PopItemFlag();
  ImGui::PopStyleVar();
}

bool mx_img_window::mxUiButtonTooltip(const char* label, const char* tip, ImVec4 colFg, ImVec4 colBg, const ImVec2& size)
{
  // Setup colors
  ImGui::PushStyleColor(ImGuiCol_Text, colFg);
  ImGui::PushStyleColor(ImGuiCol_Button, colBg);

  // do the button
  bool b = ImGui::Button(label, size);

  // restore previous colors
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();

  // do the tooltip
  if (tip && ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", tip);

  // return if button pressed
  return b;
}

} // missionx namespace