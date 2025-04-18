#ifndef __MX_COLORS_H
#define __MX_COLORS_H
#pragma once


#include "../imgui/imgui.h"
#include <chrono>

namespace missionx
{

namespace color
{
// https://doc.instantreality.org/tools/color_calculator/  GLSL Colors RGB Normalized Decimal = (1.0/255.0)*Color
// https://airtightinteractive.com/util/hex-to-glsl/

// Based on https://web.archive.org/web/20180301041827/https://prideout.net/archive/colors.php
inline static const ImVec4 color_vec4_aliceblue            = { 0.941f, 0.973f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_antiquewhite         = { 0.980f, 0.922f, 0.843f, 1.0f };
inline static const ImVec4 color_vec4_aqua                 = { 0.000f, 1.000f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_aquamarine           = { 0.498f, 1.000f, 0.831f, 1.0f };
inline static const ImVec4 color_vec4_azure                = { 0.941f, 1.000f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_beige                = { 0.961f, 0.961f, 0.863f, 1.0f };
inline static const ImVec4 color_vec4_bisque               = { 1.000f, 0.894f, 0.769f, 1.0f };
inline static const ImVec4 color_vec4_black                = { 0.000f, 0.000f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_blanchedalmond       = { 1.000f, 0.922f, 0.804f, 1.0f };
inline static const ImVec4 color_vec4_blue                 = { 0.000f, 0.000f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_blueviolet           = { 0.541f, 0.169f, 0.886f, 1.0f };
inline static const ImVec4 color_vec4_brown                = { 0.647f, 0.165f, 0.165f, 1.0f };
inline static const ImVec4 color_vec4_burlywood            = { 0.871f, 0.722f, 0.529f, 1.0f };
inline static const ImVec4 color_vec4_cadetblue            = { 0.373f, 0.620f, 0.627f, 1.0f };
inline static const ImVec4 color_vec4_chartreuse           = { 0.498f, 1.000f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_chocolate            = { 0.824f, 0.412f, 0.118f, 1.0f };
inline static const ImVec4 color_vec4_coral                = { 1.000f, 0.498f, 0.314f, 1.0f };
inline static const ImVec4 color_vec4_cornflowerblue       = { 0.392f, 0.584f, 0.929f, 1.0f };
inline static const ImVec4 color_vec4_cornsilk             = { 1.000f, 0.973f, 0.863f, 1.0f };
inline static const ImVec4 color_vec4_crimson              = { 0.863f, 0.078f, 0.235f, 1.0f };
inline static const ImVec4 color_vec4_cyan                 = { 0.000f, 1.000f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_darkblue             = { 0.000f, 0.000f, 0.545f, 1.0f };
inline static const ImVec4 color_vec4_darkcyan             = { 0.000f, 0.545f, 0.545f, 1.0f };
inline static const ImVec4 color_vec4_darkgoldenrod        = { 0.722f, 0.525f, 0.043f, 1.0f };
inline static const ImVec4 color_vec4_darkgray             = { 0.663f, 0.663f, 0.663f, 1.0f };
inline static const ImVec4 color_vec4_darkgreen            = { 0.000f, 0.392f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_darkgrey             = { 0.663f, 0.663f, 0.663f, 1.0f };
inline static const ImVec4 color_vec4_darkkhaki            = { 0.741f, 0.718f, 0.420f, 1.0f };
inline static const ImVec4 color_vec4_darkmagenta          = { 0.545f, 0.000f, 0.545f, 1.0f };
inline static const ImVec4 color_vec4_darkolivegreen       = { 0.333f, 0.420f, 0.184f, 1.0f };
inline static const ImVec4 color_vec4_darkorange           = { 1.000f, 0.549f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_darkorchid           = { 0.600f, 0.196f, 0.800f, 1.0f };
inline static const ImVec4 color_vec4_darkred              = { 0.545f, 0.000f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_darksalmon           = { 0.914f, 0.588f, 0.478f, 1.0f };
inline static const ImVec4 color_vec4_darkseagreen         = { 0.561f, 0.737f, 0.561f, 1.0f };
inline static const ImVec4 color_vec4_darkslateblue        = { 0.282f, 0.239f, 0.545f, 1.0f };
inline static const ImVec4 color_vec4_darkslategray        = { 0.184f, 0.310f, 0.310f, 1.0f };
inline static const ImVec4 color_vec4_darkslategrey        = { 0.184f, 0.310f, 0.310f, 1.0f };
inline static const ImVec4 color_vec4_darkturquoise        = { 0.000f, 0.808f, 0.820f, 1.0f };
inline static const ImVec4 color_vec4_darkviolet           = { 0.580f, 0.000f, 0.827f, 1.0f };
inline static const ImVec4 color_vec4_deeppink             = { 1.000f, 0.078f, 0.576f, 1.0f };
inline static const ImVec4 color_vec4_deepskyblue          = { 0.000f, 0.749f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_dimgray              = { 0.412f, 0.412f, 0.412f, 1.0f };
inline static const ImVec4 color_vec4_dimgrey              = { 0.412f, 0.412f, 0.412f, 1.0f };
inline static const ImVec4 color_vec4_dodgerblue           = { 0.118f, 0.565f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_firebrick            = { 0.698f, 0.133f, 0.133f, 1.0f };
inline static const ImVec4 color_vec4_floralwhite          = { 1.000f, 0.980f, 0.941f, 1.0f };
inline static const ImVec4 color_vec4_forestgreen          = { 0.133f, 0.545f, 0.133f, 1.0f };
inline static const ImVec4 color_vec4_fuchsia              = { 1.000f, 0.000f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_gainsboro            = { 0.863f, 0.863f, 0.863f, 1.0f };
inline static const ImVec4 color_vec4_ghostwhite           = { 0.973f, 0.973f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_gold                 = { 1.000f, 0.843f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_goldenrod            = { 0.855f, 0.647f, 0.125f, 1.0f };
inline static const ImVec4 color_vec4_gray                 = { 0.502f, 0.502f, 0.502f, 1.0f };
inline static const ImVec4 color_vec4_green                = { 0.000f, 0.502f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_greenyellow          = { 0.678f, 1.000f, 0.184f, 1.0f };
inline static const ImVec4 color_vec4_grey                 = { 0.502f, 0.502f, 0.502f, 1.0f };
inline static const ImVec4 color_vec4_honeydew             = { 0.941f, 1.000f, 0.941f, 1.0f };
inline static const ImVec4 color_vec4_hotpink              = { 1.000f, 0.412f, 0.706f, 1.0f };
inline static const ImVec4 color_vec4_indianred            = { 0.804f, 0.361f, 0.361f, 1.0f };
inline static const ImVec4 color_vec4_indigo               = { 0.294f, 0.000f, 0.510f, 1.0f };
inline static const ImVec4 color_vec4_ivory                = { 1.000f, 1.000f, 0.941f, 1.0f };
inline static const ImVec4 color_vec4_khaki                = { 0.941f, 0.902f, 0.549f, 1.0f };
inline static const ImVec4 color_vec4_lavender             = { 0.902f, 0.902f, 0.980f, 1.0f };
inline static const ImVec4 color_vec4_lavenderblush        = { 1.000f, 0.941f, 0.961f, 1.0f };
inline static const ImVec4 color_vec4_lawngreen            = { 0.486f, 0.988f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_lemonchiffon         = { 1.000f, 0.980f, 0.804f, 1.0f };
inline static const ImVec4 color_vec4_lightblue            = { 0.678f, 0.847f, 0.902f, 1.0f };
inline static const ImVec4 color_vec4_lightcoral           = { 0.941f, 0.502f, 0.502f, 1.0f };
inline static const ImVec4 color_vec4_lightcyan            = { 0.878f, 1.000f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_lightgoldenrodyellow = { 0.980f, 0.980f, 0.824f, 1.0f };
inline static const ImVec4 color_vec4_lightgray            = { 0.827f, 0.827f, 0.827f, 1.0f };
inline static const ImVec4 color_vec4_lightgreen           = { 0.565f, 0.933f, 0.565f, 1.0f };
inline static const ImVec4 color_vec4_lightgrey            = { 0.827f, 0.827f, 0.827f, 1.0f };
inline static const ImVec4 color_vec4_lightpink            = { 1.000f, 0.714f, 0.757f, 1.0f };
inline static const ImVec4 color_vec4_lightsalmon          = { 1.000f, 0.627f, 0.478f, 1.0f };
inline static const ImVec4 color_vec4_lightseagreen        = { 0.125f, 0.698f, 0.667f, 1.0f };
inline static const ImVec4 color_vec4_lightskyblue         = { 0.529f, 0.808f, 0.980f, 1.0f };
inline static const ImVec4 color_vec4_lightslategray       = { 0.467f, 0.533f, 0.600f, 1.0f };
inline static const ImVec4 color_vec4_lightslategrey       = { 0.467f, 0.533f, 0.600f, 1.0f };
inline static const ImVec4 color_vec4_lightsteelblue       = { 0.690f, 0.769f, 0.871f, 1.0f };
inline static const ImVec4 color_vec4_lightyellow          = { 1.000f, 1.000f, 0.878f, 1.0f };
inline static const ImVec4 color_vec4_lime                 = { 0.000f, 1.000f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_limegreen            = { 0.196f, 0.804f, 0.196f, 1.0f };
inline static const ImVec4 color_vec4_linen                = { 0.980f, 0.941f, 0.902f, 1.0f };
inline static const ImVec4 color_vec4_magenta              = { 1.000f, 0.000f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_maroon               = { 0.502f, 0.000f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_mediumaquamarine     = { 0.400f, 0.804f, 0.667f, 1.0f };
inline static const ImVec4 color_vec4_mediumblue           = { 0.000f, 0.000f, 0.804f, 1.0f };
inline static const ImVec4 color_vec4_mediumorchid         = { 0.729f, 0.333f, 0.827f, 1.0f };
inline static const ImVec4 color_vec4_mediumpurple         = { 0.576f, 0.439f, 0.859f, 1.0f };
inline static const ImVec4 color_vec4_mediumseagreen       = { 0.235f, 0.702f, 0.443f, 1.0f };
inline static const ImVec4 color_vec4_mediumslateblue      = { 0.482f, 0.408f, 0.933f, 1.0f };
inline static const ImVec4 color_vec4_mediumspringgreen    = { 0.000f, 0.980f, 0.604f, 1.0f };
inline static const ImVec4 color_vec4_mediumturquoise      = { 0.282f, 0.820f, 0.800f, 1.0f };
inline static const ImVec4 color_vec4_mediumvioletred      = { 0.780f, 0.082f, 0.522f, 1.0f };
inline static const ImVec4 color_vec4_midnightblue         = { 0.098f, 0.098f, 0.439f, 1.0f };
inline static const ImVec4 color_vec4_mintcream            = { 0.961f, 1.000f, 0.980f, 1.0f };
inline static const ImVec4 color_vec4_mistyrose            = { 1.000f, 0.894f, 0.882f, 1.0f };
inline static const ImVec4 color_vec4_moccasin             = { 1.000f, 0.894f, 0.710f, 1.0f };
inline static const ImVec4 color_vec4_navajowhite          = { 1.000f, 0.871f, 0.678f, 1.0f };
inline static const ImVec4 color_vec4_navy                 = { 0.000f, 0.000f, 0.502f, 1.0f };
inline static const ImVec4 color_vec4_oldlace              = { 0.992f, 0.961f, 0.902f, 1.0f };
inline static const ImVec4 color_vec4_olive                = { 0.502f, 0.502f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_olivedrab            = { 0.420f, 0.557f, 0.137f, 1.0f };
inline static const ImVec4 color_vec4_orange               = { 1.000f, 0.647f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_orangered            = { 1.000f, 0.271f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_orchid               = { 0.855f, 0.439f, 0.839f, 1.0f };
inline static const ImVec4 color_vec4_palegoldenrod        = { 0.933f, 0.910f, 0.667f, 1.0f };
inline static const ImVec4 color_vec4_palegreen            = { 0.596f, 0.984f, 0.596f, 1.0f };
inline static const ImVec4 color_vec4_paleturquoise        = { 0.686f, 0.933f, 0.933f, 1.0f };
inline static const ImVec4 color_vec4_palevioletred        = { 0.859f, 0.439f, 0.576f, 1.0f };
inline static const ImVec4 color_vec4_papayawhip           = { 1.000f, 0.937f, 0.835f, 1.0f };
inline static const ImVec4 color_vec4_peachpuff            = { 1.000f, 0.855f, 0.725f, 1.0f };
inline static const ImVec4 color_vec4_peru                 = { 0.804f, 0.522f, 0.247f, 1.0f };
inline static const ImVec4 color_vec4_pink                 = { 1.000f, 0.753f, 0.796f, 1.0f };
inline static const ImVec4 color_vec4_plum                 = { 0.867f, 0.627f, 0.867f, 1.0f };
inline static const ImVec4 color_vec4_powderblue           = { 0.690f, 0.878f, 0.902f, 1.0f };
inline static const ImVec4 color_vec4_purple               = { 0.502f, 0.000f, 0.502f, 1.0f };
inline static const ImVec4 color_vec4_red                  = { 1.000f, 0.000f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_rosybrown            = { 0.737f, 0.561f, 0.561f, 1.0f };
inline static const ImVec4 color_vec4_royalblue            = { 0.255f, 0.412f, 0.882f, 1.0f };
inline static const ImVec4 color_vec4_saddlebrown          = { 0.545f, 0.271f, 0.075f, 1.0f };
inline static const ImVec4 color_vec4_salmon               = { 0.980f, 0.502f, 0.447f, 1.0f };
inline static const ImVec4 color_vec4_sandybrown           = { 0.957f, 0.643f, 0.376f, 1.0f };
inline static const ImVec4 color_vec4_seagreen             = { 0.180f, 0.545f, 0.341f, 1.0f };
inline static const ImVec4 color_vec4_seashell             = { 1.000f, 0.961f, 0.933f, 1.0f };
inline static const ImVec4 color_vec4_sienna               = { 0.627f, 0.322f, 0.176f, 1.0f };
inline static const ImVec4 color_vec4_silver               = { 0.753f, 0.753f, 0.753f, 1.0f };
inline static const ImVec4 color_vec4_skyblue              = { 0.529f, 0.808f, 0.922f, 1.0f };
inline static const ImVec4 color_vec4_slateblue            = { 0.416f, 0.353f, 0.804f, 1.0f };
inline static const ImVec4 color_vec4_slategray            = { 0.439f, 0.502f, 0.565f, 1.0f };
inline static const ImVec4 color_vec4_slategrey            = { 0.439f, 0.502f, 0.565f, 1.0f };
inline static const ImVec4 color_vec4_snow                 = { 1.000f, 0.980f, 0.980f, 1.0f };
inline static const ImVec4 color_vec4_springgreen          = { 0.000f, 1.000f, 0.498f, 1.0f };
inline static const ImVec4 color_vec4_steelblue            = { 0.275f, 0.510f, 0.706f, 1.0f };
inline static const ImVec4 color_vec4_tan                  = { 0.824f, 0.706f, 0.549f, 1.0f };
inline static const ImVec4 color_vec4_teal                 = { 0.000f, 0.502f, 0.502f, 1.0f };
inline static const ImVec4 color_vec4_thistle              = { 0.847f, 0.749f, 0.847f, 1.0f };
inline static const ImVec4 color_vec4_tomato               = { 1.000f, 0.388f, 0.278f, 1.0f };
inline static const ImVec4 color_vec4_turquoise            = { 0.251f, 0.878f, 0.816f, 1.0f };
inline static const ImVec4 color_vec4_violet               = { 0.933f, 0.510f, 0.933f, 1.0f };
inline static const ImVec4 color_vec4_wheat                = { 0.961f, 0.871f, 0.702f, 1.0f };
inline static const ImVec4 color_vec4_white                = { 1.000f, 1.000f, 1.000f, 1.0f };
inline static const ImVec4 color_vec4_whitesmoke           = { 0.961f, 0.961f, 0.961f, 1.0f };
inline static const ImVec4 color_vec4_yellow               = { 1.000f, 1.000f, 0.000f, 1.0f };
inline static const ImVec4 color_vec4_yellowgreen          = { 0.604f, 0.804f, 0.196f, 1.0f };
inline static const ImVec4 color_vec4_mx_bluish            = { 0.325f, 0.427f, 0.549f, 1.0f }; // rgb(83, 109, 140)
inline static const ImVec4 color_vec4_mx_dark_bluish       = { 0.380f, 0.500f, 0.640f, 1.0f }; // rgb(38, 50, 64)
inline static const ImVec4 color_vec4_mx_redish            = { 0.560f, 0.123f, 0.204f, 1.0f }; // rgb (143, 31, 52)
inline static const ImVec4 color_vec4_mx_dimgreenyellow    = { 0.439f, 0.600f, 0.284f, 1.0f };
inline static const ImVec4 color_vec4_mx_dimgreen          = { 0.239f, 0.412f, 0.084f, 1.0f };
inline static const ImVec4 color_vec4_mx_dimblack          = { 0.112f, 0.112f, 0.112f, 1.0f };

constexpr const static long long BASE_DURATION_LL = 10;

namespace func
{
  //using seconds = std::chrono::seconds;
//static  std::chrono::time_point<std::chrono::steady_clock> last_run_os_clock;
static  std::chrono::seconds                               deltaOsClock_seconds = std::chrono::seconds(0);


static ImVec4 get_color_based_on_dimnish_through_time (const long long& inDuration, const ImVec4& inCurrentColor, const ImVec4 &inBaseColor)
  {
    if (inDuration <= missionx::color::BASE_DURATION_LL)
      return missionx::color::color_vec4_greenyellow;

    if (inDuration <= ((float)missionx::color::BASE_DURATION_LL * 2.5f))
    {
      // the calculation should be done once - move it to a better location in the class
      ImVec4 vec4GradDelta_f = ImVec4(inBaseColor.x * 0.025f, inBaseColor.y * 0.020f, inBaseColor.z * 0.025f, 1.0f);
      return ImVec4(inCurrentColor.x - vec4GradDelta_f.x, inCurrentColor.y - vec4GradDelta_f.y, inCurrentColor.z - vec4GradDelta_f.z, 1.0f);
    }

    return missionx::color::color_vec4_dimgrey;
  }


static  void
  flcDebugColors(const bool flagWasCalled, long long& timePassed, ImVec4& currentColor, std::chrono::time_point<std::chrono::steady_clock> in_last_run_os_clock, const ImVec4& baseColor = missionx::color::color_vec4_greenyellow)
  {
  auto os_clock = std::chrono::steady_clock::now();
  // calc duration only if we did not cross the BASE_DURATION_LL * 3 timer. This will allow Four color phases
  const auto duration = (timePassed <= missionx::color::BASE_DURATION_LL * 3) ? std::chrono::duration_cast<std::chrono::seconds>(os_clock - in_last_run_os_clock).count() : missionx::color::BASE_DURATION_LL * 10;

  if (flagWasCalled) // there is no use in calculating
  {
      timePassed   = duration;
      currentColor = missionx::color::func::get_color_based_on_dimnish_through_time(duration, currentColor, baseColor);

    return;
  }

  currentColor = missionx::color::color_vec4_white; // no executed
}
} // func

} // currentColor
} // missionx


#endif // __MX_COLORS_H
