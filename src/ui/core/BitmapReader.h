#ifndef BITMAPREADER_H
#define BITMAPREADER_H

/**************


**************/

#include <array>

//#ifdef LIN
//#include "../../src/core/unix/make_unique_unix.h"
//#endif

#include "../../core/MxUtils.h"
#include "../../io/Log.hpp"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"

#include "TextureFile.h"

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
//#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_SIMD // remove SSE2 implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // in "libs/stb" folder

using namespace missionx;

namespace missionx
{

class BitmapReader
{
public:
  BitmapReader();

  static bool loadGLTexture(mxTextureFile& inTextureFile, std::string &outErr, bool flipImage_b = true, bool is_sync_b = true); // v3.0.140 added flip flag for nuklear library since it might not need it.

  static bool loadImageStb(std::string fileName, mxTextureFile::IMAGEDATA* ImageData, bool inFlipImage_b, std::string &outErr);


  //////////////// DEPRECATED /////////////////
  
  // XPLMDrawingPhase draw_phase;    // holds which phase to display widget
  // XPLMDrawingPhase current_phase; // holds current drawing phase
  //
  // void set_draw_phase(XPLMDrawingPhase inPhase) { draw_phase = inPhase; }
  // void set_current_draw_phase(XPLMDrawingPhase inPhase) { current_phase = inPhase; }
  //
  //
  // static std::vector<uint8_t> readFile(const char* path, std::string& errMsg); //->std::vector<uint8_t>;
  //
  //// Used for dragging plugin panel window. x,y,left,top,right,bottom
  // bool CoordInCloseRect(float x, float y, float l, float t, float r, [[maybe_unused]] float b) { return (((x >= l) && (x <= l + 8) && (y < t) && (y >= t - 8)) || ((x <= r) && (x >= r - 8) && (y < t) && (y >= t - 8))); }
  //
private:
  // XPLMDataRef RED, GREEN, BLUE, COCKPIT_LIGHTS, LIGHTS_ON;
  //
  //
  // saar
  // float imageWidth, imageHeight, imageWidthRatio, imageHeightRatio; // saar

};

} // namespace
#endif // BASE_BITMAP_H
