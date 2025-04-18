#ifndef MXVR_H_
#define MXVR_H_
#pragma once

namespace missionx
{

class mxvr
{
public:
  ~mxvr();
  mxvr();

  static bool  flag_in_vr;
  static int   vr_display_missionx_in_vr_mode; // v3.0.221.6
  //static float vr_ratio;      // v3.0.221.6

  //static const float VR_RATIO;      // = 0.75f;
  //static const float DEFAULT_RATIO; //= 1.0f;
};

}
#endif