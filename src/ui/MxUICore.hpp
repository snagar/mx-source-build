#ifndef MXUICORE_H_
#define MXUICORE_H_

#pragma once

#include "XPLMUtilities.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include <string>
#include <map>
#include <unordered_map>

namespace missionx
{
typedef enum _mx_ui_state : uint8_t
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

  typedef struct _mxFontMeta
  {
    int   id{ 0 };
    float fSizePx{ 13.0 };

    _mxFontMeta() {}
    _mxFontMeta(int inId, float inSize)
    { 
      id = inId;
      fSizePx = inSize;
    }
  } mxFontMeta;

  typedef struct _mxFontData
  {
    
    std::string                       fontName_s;
    std::string                       fontLocation_s;
    std::map<std::string, mxFontMeta> mapMetaData; // v3.305.3 map of unique type and meta data

    _mxFontData() { reset(); }

    _mxFontData(std::string inName)
    {
      reset();
      fontName_s = inName;
    }

    _mxFontData(std::string inName, std::string inLocation)
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

    void initFontMeta(float inSizeVal, std::string inFontType) // we do not define the font ID yet, so it will auto initialize to Zero
    { 
      if (!inFontType.empty())
      {
        this->mapMetaData[inFontType] = mxFontMeta(0, inSizeVal); // we always start with pos 0, it will be determine later when we call sync
      }
      else
      {
        const std::string msg = "missionx:\tFont " + this->fontName_s + ": did not defined font type name. Skipping.";
        XPLMDebugString(msg.c_str());
      }

    }

    void setFontID(const int inIndexInImgui, std::string inFontType)
    {

      // v3.305.3 use map_metaData
      if (mapMetaData.find(inFontType) != mapMetaData.end())
      {
        this->mapMetaData[inFontType].id = inIndexInImgui;
        mapFontTypeToFontID[inFontType]  = inIndexInImgui;
      }
      else
      {
        const std::string msg = "missionx:\tFont " + fontName_s + ": Did not find Font Type: " + inFontType + " in map_metaData. Skipping...";
        XPLMDebugString(msg.c_str());
      }
      

    }

  } mxFontData;

  static std::string                          mxDefaultFontName;
  static std::map<std::string, mxFontData>    mapFontsMeta;
  static std::unordered_map<std::string, int> mapFontTypeToFontID; // map the font type: title, text..., to the real ImGui FontAtlas ID.
  static std::map<std::string, int>           mapFontTypesBeingUsedInProgram;


};

}



#endif
