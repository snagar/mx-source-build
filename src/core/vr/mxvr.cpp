#ifndef MXVR_CPP
#define MXVR_CPP

#include "mxvr.h"

namespace missionx
{
bool  missionx::mxvr::flag_in_vr;    // holds if we are in X-Plane VR state
int   missionx::mxvr::vr_display_missionx_in_vr_mode; // Check if we are in VR but also checks if we support VR using our options. This way we might fly in VR but user asked not to display briefer in VR, to ignore this state.
//float missionx::mxvr::vr_ratio;      // v3.0.221.6

//const float mxvr::VR_RATIO      = 0.75f;
//const float mxvr::DEFAULT_RATIO = 1.0f;

mxvr::mxvr()
{
  flag_in_vr    = false;
  //vr_ratio      = 1.0f;
  //vr_display_missionx_in_vr_mode = 0;
}


mxvr::~mxvr() {}

}

#endif
