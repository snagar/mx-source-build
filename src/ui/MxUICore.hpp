#ifndef MXUICORE_H_
#define MXUICORE_H_

#pragma once

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMUtilities.h"

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

#include "../core/mxconst.h"

namespace missionx
{
typedef enum class _mx_ui_state : std::uint8_t
{
  ui_hidden,               // not visible therefore also disabled
  ui_disabled_and_visible, // disable + visible
  ui_enabled_and_visible   // enabled + visible
} mx_ui_state;

class MxUICore
{
private:

public:
  MxUICore();

  std::string label;

  void init()
  {
    label.clear();
    uiName.clear();

    labelLength       = 0;
    labelLength_px    = 0.0f;
    uiState           = mx_ui_state::ui_enabled_and_visible;
    labelColor_arr[0] = labelColor_arr[1] = labelColor_arr[2] = 1; // white
  }

  // core
  mx_ui_state uiState = mx_ui_state::ui_enabled_and_visible;
  std::string uiName;
  int         labelLength;
  float       labelLength_px;

  // screen bounds
  static int global_desktop_bounds[4]; /* left, bottom, right, top */


  float labelColor_arr[4]; /* RGB White */


  /**

  left:bottom              right:bottom
  0:728====================1024:728
  |
  |
  0:0======================1024:0
  left:top                 right:top

  **/

  // query for the global desktop bounds
  static void refreshGlobalDesktopBoundsValues() { XPLMGetScreenBoundsGlobal(&MxUICore::global_desktop_bounds[0], &MxUICore::global_desktop_bounds[3], &MxUICore::global_desktop_bounds[2], &MxUICore::global_desktop_bounds[1]); }

  static void assignCenterDesktopGlobalBounds(int& outCenterX, int& outCenterY);                                                                       // v3.0.155
  static void calculateWindowCenterRelativeToDesktopGlobalBounds(int inWindowsWidth, int inWindowsHeight, int& outL, int& outB, int& outR, int& outT); // v3.0.155

  struct mxFontMeta
  {
    int   id{ 0 };
    float fSizePx = missionx::mxconst::FONT_PIXEL_13;
    std::string fontName;
    std::string fontLocation;

  };

  struct mxFontData
  {
    
    std::string                       fontName_s;
    std::string                       fontLocation_s;
    std::map<std::string, mxFontMeta> mapMetaData; // v3.305.3 map of unique type and meta data

    mxFontData() { reset(); }

    mxFontData(std::string inName)
    {
      reset();
      fontName_s = inName;
    }

    mxFontData(std::string inName, std::string inLocation)
    {
      reset();
      fontName_s = inName;
      fontLocation_s = inLocation;
    }

    void reset()
    {
      fontName_s.clear();
      fontLocation_s.clear();
      mapMetaData.clear(); // v3.305.3
    }

  };

  static std::string                          mxDefaultFontName;
  static std::map<std::string, mxFontMeta>    mapFontTypeMeta; // v26.08.1
  static std::map<std::string, int>           mapFontTypesBeingUsedInProgram;
  // Deprecated v26.08.1
  // static std::map<std::string, mxFontData>    mapFontsMeta;
  // static std::unordered_map<std::string, int> mapFontTypeToFontID; // map the font type: title, text..., to the real ImGui FontAtlas ID.
  // static std::unordered_map<std::string, mxFontMeta> mapFontTypeToFontID; // map the font type: title, text..., to the real ImGui FontAtlas ID.


};

}



#endif
