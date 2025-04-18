#ifndef CUEINFO_H_
#define CUEINFO_H_

#include <deque>
#include <vector>

#include "../../core/coordinate/Point.hpp"
#include "../../core/coordinate/UtilsGraph.hpp"

#include "../../core/Utils.h"


#ifdef IBM

#include <gl\gl.h>
#include <gl\glu.h>
#else
#ifdef LIN
#define TRUE  1
#define FALSE 0

#include <GL/gl.h>
#include <GL/glu.h>
#else

//#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#endif
#endif

using namespace missionx;

namespace missionx
{



class CueInfo
{
private:
  std::string errMsg;
  float       radius;

  static XPLMProbeRef probe;


public:
  CueInfo(void);
  // ~CueInfo(void);

  void                   reset();
  static void            prepareProbe();
  static void            destroyProbe();
  static double          getTerrainElevInMeter_FromPoint(missionx::Point& p1, XPLMProbeResult& outResult);
  static XPLMProbeInfo_t terrain_Y_Probe (const double & inXgl, const double & inYgl, const double & inZgl, XPLMProbeResult& outResult);

  IXMLNode node_ptr{ IXMLNode::emptyIXMLNode }; // v3.0.241.1 point to the root node from mission file. Example <trigger> in most of the cases.

  // Removed v3.0.202a
  /////////////////////////////
  // v2.1.25
  //// Based on: http://stackoverflow.com/questions/7173494/vbos-with-stdvector
  ////
  // typedef struct
  //{
  // float x, y, z;
  // float nx, ny, nz;
  // float u, v;
  //}
  // Vertex;
  //
  // vector<Vertex> vertices;
  // vector<Vertex>::iterator itVerPoint, itVerPointEnd;

  /////////////////////////////

  missionx::mx_cue_types cueType; // which type is the cue
  int                    cueSeq;
  std::string            originName;

  // pointer to source data
  std::deque<missionx::Point>* deqPoints_ptr;        // pointer to points container currently good for triggers but might not be used much since sometime we need much more data on the element being drawn.

  // local variables
  XPLMProbeResult result;
  // bool  isRendered; // v3.0.203 - deprecated, but we might use it later
  bool  canBeRendered; // v3.0.203 Does the trigger lines can be drawn in 3D space ? If this a special trigger that should not be rendered yet, then we will set this to false;
  bool  wasCalculated; // a flag that confirm that the Cue is ready for display
  bool  hasRadius;
  void  setRadiusAsMeter (const float inValue) { radius = inValue; };
  void  setRadiusAsNm (const float inValue) { radius = inValue * missionx::nm2meter; }; // Nautical Miles to Meter
  float getRadiusAsNm() const { return radius * meter2nm; };

  float getRadiusAsMeter() const { return radius; };

  bool didProbeTerrain; // if current cue used probe terrain routine

  Point pSource; // holds PointXY of Object (target, fuelTank, Trigger etc...). Initialized during mission data parsing.
  Point pLast;

  // https://openglcolor.mpeters.me/
  // https://www.w3schools.com/colors/colors_mixer.asp
  struct struct_color
  {
    float R = 0.0f, rr;
    float G = 0.0f, gg;
    float B = 0.0f, bb;

    void setRGB (const float r, const float g, const float b)
    {
      R = r;
      G = g;
      B = b;
    }

    void initRGB_and_StoreColor (const float r, const float g, const float b) // also store original colors
    {
      setRGB(r, g, b);
      rr = r;
      gg = g;
      bb = b;
    }

    void setToRed() { initRGB_and_StoreColor(mxconst::MANDATORY_R, mxconst::MANDATORY_G, mxconst::MANDATORY_B); }

    void setToPurple() { initRGB_and_StoreColor(0.4f, 0.001f, 0.8f); }

    void setToBlue() { initRGB_and_StoreColor(0.01f, 0.01f, 0.9f); }

    void setToYellow() { initRGB_and_StoreColor(0.91f, 0.91f, 0.01f); }

    void setToPastel_yellow() { initRGB_and_StoreColor(0.91f, 0.91f, 0.50f); }

    void setToGreen() { initRGB_and_StoreColor(0.0f, 1.0f, 0.001f); }

    void setToBrown() { initRGB_and_StoreColor(0.60f, 0.20f, 0.001f); }

    void setToBrown_chocolate() { initRGB_and_StoreColor(0.50f, 0.16f, 0.001f); }

    void setToInternational_orange() { initRGB_and_StoreColor(0.99f, 0.33f, 0.001f); }

    void setToPeach_orange() { initRGB_and_StoreColor(0.99f, 0.73f, 0.60f); }

    void setToPastel_red() { initRGB_and_StoreColor(0.99f, 0.40f, 0.40f); }

    void setToDefaultCueColor()
    {
      this->setToPastel_yellow();
      // initRGB_and_StoreColor(0.9f, 0.65f, 0.001f);
    }

    void setToBlack() { initRGB_and_StoreColor(0.001f, 0.001f, 0.001f); }

    void setToWhite() { initRGB_and_StoreColor(1.0f, 1.0f, 1.0f); }

    void setColorToDisable() // default disabled color
    {
      //#declare DimGray = color red 0.329412 green 0.329412 blue 0.329412
      //#declare DimGrey = color red 0.329412 green 0.329412 blue 0.329412
      //#declare Gray = color red 0.752941 green 0.752941 blue 0.752941
      //#declare Grey = color red 0.752941 green 0.752941 blue 0.752941
      //#declare LightGray = color red 0.658824 green 0.658824 blue 0.658824
      //#declare LightGrey = color red 0.658824 green 0.658824 blue 0.658824
      //#declare VLightGray = color red 0.80 green 0.80 blue 0.80
      //#declare VLightGrey = color red 0.80 green 0.80 blue 0.80

      R = 0.80f;
      G = 0.80f;
      B = 0.80f;
    }

    void setColorToEnable() // switch between current RGB color and the stored original ones
    {
      setRGB(rr, gg, bb);
    }

    void setColorToActiveDisabled() // set to disabled but still active // do not remember when I used it
    {
      R = 0.752941f;
      G = 0.752941f;
      B = 0.752941f;
    }

  } color;

  // Holds information
  std::vector<Point> vecPoints; // v3.0.202a wil hold calculated points
  size_t             vecSize;   // will hold the size of vecPoints // used on Refresh moving targets
  bool               isSourceMoving;

  // Members
  void calculateCircleFromPoint(missionx::Point& p1, bool doCloseShape, bool bCalcEachPointElev, int inPointsToCalc = 120);
  void refreshCirclePointsRelativeToSource(missionx::Point& pSource); // relevant for moving Cue
  void refreshPointsAndElevBasedTerrain();

  void calculatePolygonalPointsElevation();

  void clear();
};

} // end namespace

#endif /* CUEINFO_H_ */
