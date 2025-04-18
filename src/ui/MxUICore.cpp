#include "MxUICore.hpp"

namespace missionx
{
int MxUICore::global_desktop_bounds[4]; // holds global desktop bounds


std::string                                 MxUICore::mxDefaultFontName{ "" };
std::map<std::string, MxUICore::mxFontData> MxUICore::mapFontsMeta;
std::unordered_map<std::string, int>        MxUICore::mapFontTypeToFontID;
std::map<std::string, int>                  MxUICore::mapFontTypesBeingUsedInProgram;


}

missionx::MxUICore::MxUICore()
{
  init();
}

void
missionx::MxUICore::assignCenterDesktopGlobalBounds(int& outCenterX, int& outCenterY)
{
  missionx::MxUICore::refreshGlobalDesktopBoundsValues(); // v3.0.151 // fix window positioning

  outCenterX = outCenterY = 0;
  outCenterX              = (MxUICore::global_desktop_bounds[2] - MxUICore::global_desktop_bounds[0]) / 2;
  outCenterY              = (MxUICore::global_desktop_bounds[3] - MxUICore::global_desktop_bounds[1]) / 2;
}

void
missionx::MxUICore::calculateWindowCenterRelativeToDesktopGlobalBounds(int inWindowsWidth, int inWindowsHeight, int& outL, int& outB, int& outR, int& outT)
{
  int centerX, centerY;
  // get desktop display center
  MxUICore::assignCenterDesktopGlobalBounds(centerX, centerY); // get display middle position

  // calculate briefer position
  outL = centerX - (inWindowsWidth / 2);  // left
  outB = centerY - (inWindowsHeight / 2); // bottom
  outR = outL + inWindowsWidth;           // right
  outT = outB + inWindowsHeight;          // top
}
