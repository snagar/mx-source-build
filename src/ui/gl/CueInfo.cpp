#include "CueInfo.h"
#include "../../core/Utils.h"

/**************
Reviewd: 25-nov-2012

Replaces:

Done:
1. No Change

ToDo:

**************/

namespace missionx
{
XPLMProbeRef missionx::CueInfo::probe;
}

missionx::CueInfo::CueInfo(void)
{
  missionx::CueInfo::probe = nullptr;
  this->node_ptr           = IXMLNode::emptyIXMLNode;

  deqPoints_ptr        = nullptr;

  cueType        = missionx::mx_cue_types::cue_none;
  isSourceMoving = false;
  canBeRendered = false;

  hasRadius = false;
  radius    = 0.0f;

  result = -1; // probe result

  didProbeTerrain = false;

  cueSeq = 0;
  errMsg.clear();

  originName.clear();

  vecPoints.clear();
  vecSize = static_cast<size_t> (0);

  reset();
}


// missionx::CueInfo::~CueInfo(void) {}

void
missionx::CueInfo::reset()
{
  wasCalculated = false; // was the Cue Points calculated
  cueSeq        = 0;
  vecPoints.clear();
  vecSize = static_cast<size_t> (0);

  didProbeTerrain = false;
  canBeRendered   = false;
}

void
missionx::CueInfo::prepareProbe()
{
  if (CueInfo::probe != nullptr)
    XPLMDestroyProbe(CueInfo::probe);

  CueInfo::probe = nullptr;
  CueInfo::probe = XPLMCreateProbe(xplm_ProbeY);
}

void
missionx::CueInfo::destroyProbe()
{
  if (CueInfo::probe != nullptr)
    XPLMDestroyProbe(CueInfo::probe);
}


void
missionx::CueInfo::calculateCircleFromPoint(Point& inPoint, const bool doCloseShape, const bool bCalcEachPointElev, const int inPointsToCalc)
{
  static float angle                  = 0.0f;
  vecPoints.clear();

  if (hasRadius && radius != 0.0)
  {
    double groundElevationInMeter = 0.0;
    #ifndef RELEASE
    Log::logDebugBO("[calculateCircle] Calculating Circle for: " + ((!this->node_ptr.isEmpty()) ? Utils::readAttrib(this->node_ptr, mxconst::get_ATTRIB_NAME(), "") : "\n")); // debug
    #endif
    const double p_x = inPoint.getLat ();
    const double p_y = inPoint.getLon ();
    const double p_z = inPoint.getElevationInMeter (); // return as meters

    Point pFirst; // to help close the shape

    if (((this->cueType == missionx::mx_cue_types::cue_trigger) || (this->cueType == missionx::mx_cue_types::cue_inventory)|| (this->cueType == missionx::mx_cue_types::cue_sling_task)) && !this->wasCalculated)
      vecPoints.clear(); // clear vector points if this is the first time we call calculateCircleFromPoint and in area


    // loop over all points in poly
    missionx::CueInfo::prepareProbe();
    for (int i = 0; i < inPointsToCalc; i++)
    {
      Point p1;
      groundElevationInMeter = 0.0;

      angle = static_cast<float> ((i * 2.0 * PI / inPointsToCalc) * RadToDeg);
      // Utils::logMsg( "Degree Angle: " + Utils::formatNumber(i) + ": " + Utils::formatNumber(angle) );

      // p2 = new Point(); // removed v3.0.202a
      double outLat, outLon;
      outLat = outLon = 0.0;
      Utils::calcPointBasedOnDistanceAndBearing_2DPlane(outLat, outLon, p_x, p_y, angle, (double)getRadiusAsNm() /*v3.0.203 fix radius calculation*/);
      p1.setLat(outLat);
      p1.setLon(outLon);

      result = -1;
      p1.calcSimLocalData();
      groundElevationInMeter = CueInfo::getTerrainElevInMeter_FromPoint(p1, result);

      if (bCalcEachPointElev)
      {
        p1.setElevationMt(groundElevationInMeter);
        p1.elevWasProbed = true;
      }
      else
      {
        if (groundElevationInMeter > p_z)
        {
          p1.setElevationMt(groundElevationInMeter);
          p1.elevWasProbed = true;
        }

        else
          p1.setElevationMt(p_z);
      }


      p1.setHeading(angle);  // store the angle calculated for refresh
      p1.calcSimLocalData(); // calculate local info
      vecPoints.push_back(p1);

      #ifdef DEBUG_WRONG_TERRAIN_ELEV
      Log::logMsgNone(Utils::formatNumber<int>(i) + "\t:" + p1.to_string_with_locals());
      #endif


      if (i == 0) // if first point
        pFirst = p1;
      else
        pLast = p1; // store last pointer in vector

      // Utils::logMsg( "[calculateCircleFromPoint] " + p2.to_string()  ); // debug // Display point info
    } // end loop create circle points

    XPLMDestroyProbe(missionx::CueInfo::probe);

    if (doCloseShape)
    {
      vecPoints.push_back(pFirst);

      // Utils::logMsg(pFirst.to_string() ); // debug
      Log::logDebugBO(pFirst.to_string()); // debug
    }


    if (bCalcEachPointElev)
    {
      this->didProbeTerrain = true;
      // Utils::logMsg("Calc terrain point using probe...");
      Log::logDebugBO("Calc terrain point using probe...");
    }

    angle               = 0.0f;
    this->wasCalculated = true;
    vecSize             = vecPoints.size();

  } // if strRad has value
}

void
missionx::CueInfo::refreshCirclePointsRelativeToSource(missionx::Point& pSource)
{

  if (this->wasCalculated /*&& this->pSource*/)
  {
    Point *p1 = nullptr;
    // for (p = vecPoints.begin(), itPointEnd = vecPoints.end(); p < itPointEnd; p++)
    for (auto itPoint : vecPoints)
    {
      p1 = &itPoint;

      double outLat, outLon;
      outLat = outLon = 0.0;
      Utils::calcPointBasedOnDistanceAndBearing_2DPlane(outLat, outLon, pSource.getLat(), pSource.getLon(), (float)p1->getHeading(), (double)getRadiusAsNm() /*v3.0.203 fix radius calculation*/);
      p1->setLat(outLat);
      p1->setLon(outLon);
      p1->setElevationMt(pSource.getElevationInMeter());
      p1->calcSimLocalData(); // calculate local info
    }                         // end loop iterator
    wasCalculated = true;
  }
  // refreshCirclePointsRelativeToSource
}



void
missionx::CueInfo::refreshPointsAndElevBasedTerrain()
{
  Log::logDebugBO("IN refreshPointsAndElevBasedTerrain. " + this->originName + mxconst::get_UNIX_EOL()); // debug

  missionx::CueInfo::prepareProbe();
  for (auto p : vecPoints)
  {
    if (p.elevWasProbed)
    {
      result = -1;
      p.setElevationMt(CueInfo::getTerrainElevInMeter_FromPoint(p, result));
      if (result > xplm_ProbeHitTerrain) // if probe error or missed
      {
        Log::logDebugBO("[re-probe] Probe MISSED. setting elevation to Zero."); // debug
        p.setElevationMt(0.0);
      }

    }
    p.calcSimLocalData(); // v3.0.203

  } // end loop over all vectors point

  XPLMDestroyProbe(missionx::CueInfo::probe);
  vecSize = vecPoints.size();

  // end refreshElevBasedTerrain
}


void
missionx::CueInfo::calculatePolygonalPointsElevation()
{
  if (this->node_ptr.isEmpty() || this->deqPoints_ptr == nullptr)
  {
    Log::logMsgWarn("CueInfo fail polygonal calculation due to NULL pointer value. Notify programmer, skipping calculation...");
    return;
  }

  assert( !this->node_ptr.isEmpty() && "No trigger node pointer. Notify programmer"); // v3.0.253.5

  std::string trig_name            = Utils::readAttrib (this->node_ptr, mxconst::get_ATTRIB_NAME(), ""); // debug v3.0.253.5
  auto        attrib_elev_ft_lower = Utils::readNodeNumericAttrib<double> (this->node_ptr, mxconst::get_ATTRIB_ELEV_FT(), 0.0);
  std::string area_calc_type       = Utils::readAttrib (this->node_ptr, mxconst::get_ATTRIB_TYPE(), "");

  bool isOnGround = false; // used in code, but needs to be based on strPlaneOnGround value.
  bool wasFound   = false; // v3.0.253.5
  std::string strIsPlaneOnGround = Utils::xml_get_attribute_value_drill(this->node_ptr, mxconst::get_ATTRIB_PLANE_ON_GROUND(), wasFound, mxconst::get_ELEMENT_CONDITIONS()); // v3.0.253.5 fix bug - get the "plane on ground" attribute from <consditions>. Hopefully correct the issue

  // v3.0.217.7 parse string value into bool
  if (!strIsPlaneOnGround.empty())
    Utils::isStringBool(strIsPlaneOnGround, isOnGround);


  if (this->cueType == missionx::mx_cue_types::cue_inventory) // v3.0.313.1
  {
    isOnGround         = true;             // always for inventory
    strIsPlaneOnGround = mxconst::get_MX_TRUE(); // v3.0.217.7 to be consistent with the original logic

    attrib_elev_ft_lower = 0.0; // always on ground
  }

  // loop over all points in poly
  missionx::CueInfo::probe = nullptr;
  // Create a Y probe
  missionx::CueInfo::probe = XPLMCreateProbe(xplm_ProbeY);

  for (const auto& p : (*this->deqPoints_ptr))
  {
    XPLMProbeResult result                 = -1;
    double          groundElevationInMeter = 0.0;


    Point p1 = p; // p1 is a clone of the real trigger point, so we can modify its values for GL drawing
    p1.setElevationFt(attrib_elev_ft_lower);

    groundElevationInMeter = missionx::CueInfo::getTerrainElevInMeter_FromPoint(p1, result);

    if (isOnGround || attrib_elev_ft_lower == 0.0 || strIsPlaneOnGround.empty() /* v3.0.217.7 if not set then cue should draw from ground elevation */)
    {
      p1.setElevationMt(groundElevationInMeter);
      p1.elevWasProbed      = true;
      this->didProbeTerrain = true;
    }
    else if (!(attrib_elev_ft_lower == 0.0) && (groundElevationInMeter > (attrib_elev_ft_lower * feet2meter)))
    {
      p1.setElevationMt(groundElevationInMeter);
      p1.elevWasProbed      = true;
      this->didProbeTerrain = true;
    }


    p1.calcSimLocalData();
    this->vecPoints.push_back(p1);

    this->wasCalculated = true;
  }

  // v3.0.231.2 add the first point twice to close the poly in XPlane 3D world
  if (!this->vecPoints.empty())
    this->vecPoints.push_back(this->vecPoints.front());


  XPLMDestroyProbe(missionx::CueInfo::probe);

  vecSize = vecPoints.size();
}



void
missionx::CueInfo::clear()
{

  this->vecPoints.clear();
  vecSize = vecPoints.size();

  this->pLast.clearPoint(); // v3.0.202a
}

/* *************************************************************** */

double
missionx::CueInfo::getTerrainElevInMeter_FromPoint(missionx::Point& p1, XPLMProbeResult& outResult)
{
  double lat, lon, elev_mt;
  lat = lon = elev_mt = 0.0;

  outResult = -1;
  p1.calcSimLocalData();

  XPLMProbeInfo_t info = CueInfo::terrain_Y_Probe (p1.local_x, p1.local_y, p1.local_z, outResult);

  if (outResult == 0)
  {
    XPLMLocalToWorld(info.locationX, info.locationY, info.locationZ, &lat, &lon, &elev_mt);
  }
  else
    elev_mt = 0.0;

  return elev_mt;
}

/* *************************************************************** */

XPLMProbeInfo_t
missionx::CueInfo::terrain_Y_Probe (const double & inXgl, const double & inYgl, const double & inZgl, XPLMProbeResult& outResult)
{

  // XPLMProbeResult probeResult;
  // missionx::CueInfo::probe = NULL;
  //// Create a Y probe
  // probe = XPLMCreateProbe(xplm_ProbeY);

  static XPLMProbeInfo_t info;
  // initialized info struct.
  info.is_wet     = 0;
  info.locationX  = 0.0f;
  info.locationY  = 0.0f;
  info.locationZ  = 0.0f;
  info.normalX    = 0.0f;
  info.normalY    = 0.0f;
  info.normalZ    = 0.0f;
  info.velocityX  = 0.0f;
  info.velocityY  = 0.0f;
  info.velocityZ  = 0.0f;
  info.structSize = sizeof(info);

  outResult = XPLMProbeTerrainXYZ(missionx::CueInfo::probe, static_cast<float> (inXgl), static_cast<float> (inYgl), static_cast<float> (inZgl), &info);

  // XPLMDestroyProbe(probe);

  return info;
}
