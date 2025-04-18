#ifndef UTILSGRAPH_H_
#define UTILSGRAPH_H_
#pragma once

#include "Point.hpp"
#include "XPLMGraphics.h"


class UtilsGraph
{
public:
  static double getTerrainElevInMeter_FromPoint(missionx::Point& p1, XPLMProbeResult& outResult)
  {
    double lat, lon, elev_mt;
    lat = lon = elev_mt = 0.0;

    XPLMProbeInfo_t info;

    outResult = -1;
    p1.calcSimLocalData();

    info = UtilsGraph::terrain_Y_Probe(p1.local_x, p1.local_y, p1.local_z, outResult);

    if (outResult == 0)
    {
      XPLMLocalToWorld(info.locationX, info.locationY, info.locationZ, &lat, &lon, &elev_mt);
    }
    else
      elev_mt = 0.0;

    return elev_mt;
  }

  // ---------------------------------------

  static XPLMProbeInfo_t getTerrainInfo_FromPoint(missionx::Point& p1, XPLMProbeResult& outResult)
  {
    XPLMProbeInfo_t info;

    outResult = -1;
    p1.calcSimLocalData();

    info = UtilsGraph::terrain_Y_Probe(p1.local_x, p1.local_y, p1.local_z, outResult);

    return info;
  }

  // ---------------------------------------

  static XPLMProbeInfo_t terrain_Y_Probe(double& inXgl, double& inYgl, double& inZgl, XPLMProbeResult& outResult)
  {

    // XPLMProbeResult probeResult;
    XPLMProbeRef probe = NULL;
    // Create a Y probe
    probe = XPLMCreateProbe(xplm_ProbeY);

    static XPLMProbeInfo_t info;
    info.structSize = sizeof(info);

    outResult = XPLMProbeTerrainXYZ(probe, (float)inXgl, (float)inYgl, (float)inZgl, &info);


    XPLMDestroyProbe(probe);

    return info;
  }

  // ---------------------------------------


}; // CLASS


#endif // UTILSGRAPH_H_
