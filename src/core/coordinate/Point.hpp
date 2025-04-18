#ifndef POINT_H_
#define POINT_H_

#pragma once

#include <assert.h>

#include "../mx_base_node.h"
#include "../../io/IXMLParser.h"
#include "../../io/Log.hpp"
#include "../Utils.h"
#include "../xx_mission_constants.hpp"
#include <math.h>

using namespace missionx;
// using namespace mxconst;

namespace missionx
{
typedef enum class _object_state
  : uint8_t
{
  point_undefined = 0,
  defined         = 1
} mx_point_state;

class Point : public missionx::mx_base_node 
{
private:
  double elevation_mt;
  double store_elev_ft;
  double store_elev_mt;

  double terrain_elev_mt{ 0.0 }; // will hold terrain probe elevation information

  double elev_above_ground_ft;
  double store_elev_above_ground_ft;

  double storeLat;
  double storeLon;


  float storeHeading;
  float roll, storeRoll;
  float pitch, storePitch;

  std::string err;

  float speed_fts; // feet in seconds
  float speed_kmh; // km in hour

  mutable  missionx::mutex mt_CalcDistBetweenPointsMutex; // v3.305.2 https://stackoverflow.com/questions/28311049/attempting-to-reference-a-deleted-function-when-using-a-mutex && https://stackoverflow.com/questions/30340029/copy-class-with-stdmutex
public:
  

  XPLMProbeInfo_t probe_info;
  bool            flag_wasProbed{ false };

  double lat;
  double lon;
  double elevation_ft;
  float  heading;

  missionx::mx_point_state  pointState;
  XPLMProbeResult probe_result;

  double local_x;
  double local_y;
  double local_z;

  bool elevWasProbed; // does elevation represent terrain info

  float adjust_heading, storeAdjustHeading; // v3.0.207.5

  float timeToWaitOnPoint_sec; // for moving object. wait on next point.


  Point()
  {
    init();
    storeData();    
  }

  //Point(const Point&) = default;
  Point(const Point& inPoint) { 

    std::lock_guard<missionx::mutex> guard(mt_CalcDistBetweenPointsMutex);

    // https://stackoverflow.com/questions/30340029/copy-class-with-stdmutex

    if (!mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT().empty ())
    {
      // (missionx::mx_base_node) (*this) = (missionx::mx_base_node)inPoint; // v3.0.241.1
      const auto elevAboveGround       = Utils::readNumericAttrib (inPoint.node, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), 0.0);
      this->setElevationAboveGroundFt (elevAboveGround); // v3.0.202

      this->lat = inPoint.lat;
      this->lon = inPoint.lon;
      this->setElevationFt (inPoint.elevation_ft);
      this->heading               = inPoint.heading;
      this->adjust_heading        = inPoint.adjust_heading; // v3.0.207.5
      this->pitch                 = inPoint.pitch;
      this->roll                  = inPoint.roll;
      this->speed_fts             = inPoint.speed_fts; // renamed in v3.0.202
      this->speed_kmh             = inPoint.speed_kmh; // v3.0.202
      this->timeToWaitOnPoint_sec = inPoint.timeToWaitOnPoint_sec;


      this->local_x = inPoint.local_x; // v3.0.203
      this->local_y = inPoint.local_y; // v3.0.203
      this->local_z = inPoint.local_z; // v3.0.203

      this->terrain_elev_mt = inPoint.terrain_elev_mt; // v3.0.251.1

      this->pointState = missionx::mx_point_state::defined;

      storeData ();

      storeDataToPointNode (); // v3.0.213.7
    }
  }

  //Point(Point&&) = default;

  Point(const IXMLNode& inNode)
  { 
    this->node = inNode.deepCopy(); 
    this->storeDataFromNodeToPoint(); // v3.0.301 B3
    pointState = missionx::mx_point_state::defined;

  }

  Point (const double inLat, const double inLong)
  {
    init();
    lat        = inLat;
    lon        = inLong;
    pointState = missionx::mx_point_state::defined;
    storeData();

    storeDataToPointNode(); // v3.0.213.7
  }

  Point (const double inLat, const double inLong, const double inElev_ft)
  {
    init();
    lat          = inLat;
    lon          = inLong;
    elevation_ft = inElev_ft;
    elevation_mt = elevation_ft * missionx::feet2meter;
    pointState   = missionx::mx_point_state::defined;
    storeData();

    storeDataToPointNode(); // v3.0.213.7
  }

  void clone(Point& inPoint)
  {
    
    std::lock_guard<missionx::mutex> guard(mt_CalcDistBetweenPointsMutex);

    // (missionx::mx_base_node)(*this) = (missionx::mx_base_node)inPoint; // v3.0.241.1

    this->lat = inPoint.lat;
    this->lon = inPoint.lon;
    this->setElevationFt(inPoint.getElevationInFeet());
    this->heading               = inPoint.heading;
    this->adjust_heading        = inPoint.adjust_heading; // v3.0.207.5
    this->pitch                 = inPoint.pitch;
    this->roll                  = inPoint.roll;
    this->speed_fts             = inPoint.speed_fts; // renamed in v3.0.202
    this->speed_kmh             = inPoint.speed_kmh; // v3.0.202
    this->timeToWaitOnPoint_sec = inPoint.timeToWaitOnPoint_sec;
    this->setElevationAboveGroundFt(inPoint.getElevationAboveGroundInFeet()); // v3.0.202
    this->local_x = inPoint.local_x;                                          // v3.0.203
    this->local_y = inPoint.local_y;                                          // v3.0.203
    this->local_z = inPoint.local_z;                                          // v3.0.203

    this->terrain_elev_mt = inPoint.terrain_elev_mt; // v3.0.251.1

    this->pointState = missionx::mx_point_state::defined;

    storeData();

    storeDataToPointNode(); // v3.0.213.7
  }
  //// Operators Overload
  void operator=(Point inPoint) { clone(inPoint); }


  bool operator<(const Point& p) const { return lat < p.lat || (lat == p.lat && lon < p.lon); }

  double operator-(const Point& p) const { return Utils::calcDistanceBetween2Points_nm(p.lat, p.lon, this->lat, this->lon); }

  //// END Operators Overload


  void init()
  {
    if (!node.isEmpty())
      node.deleteNodeContent();

    if (!mxconst::get_ELEMENT_POINT().empty())
    {
      node = IXMLNode::createXMLTopNode(mxconst::get_ELEMENT_POINT().c_str()).deepCopy(); // v3.303.11
      setElevationFt(0.0);
      setSpeedInKmh(0.0f);   // v3.0.207.2
      setElevationAboveGroundFt(0.0); // v3.0.202
    }

    lat = 0.0;
    lon = 0.0;
    elevWasProbed = false;

    heading        = 0.0f; // v3.0.207.2
    adjust_heading = 0.0f; // v3.0.207.5
    pitch          = 0.0f; // v3.0.207.2
    roll           = 0.0f; // v3.0.207.2

    timeToWaitOnPoint_sec = 0.0f; // v3.0.207.2

    this->local_x = this->local_y = this->local_z = 0.0;
    this->storeLat = this->storeLon = 0.0;
    this->storeHeading = this->storeAdjustHeading = this->storePitch = this->storeRoll = 0.0f;

    pointState = missionx::mx_point_state::point_undefined;
  }

  // -----------------------------------

  void storeData()
  {
    storeLat      = lat;
    storeLon      = lon;
    store_elev_ft = elevation_ft;
    store_elev_mt = elevation_mt;

    storeHeading       = heading;
    storeAdjustHeading = adjust_heading; // v3.0.207.5
    storePitch         = pitch;
    storeRoll          = roll;

    store_elev_above_ground_ft = elev_above_ground_ft; // v3.0.202
  }

  // -----------------------------------

  void setName(std::string inName) // v3.0.241.8
  {
    if (!trim(inName).empty())
      this->setNodeStringProperty(mxconst::get_ATTRIB_NAME(), inName);
  }

  // -----------------------------------
  std::string getName() // v3.0.241.8
  {

    return Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");
  }

  // -----------------------------------
  void setLat(double inValue)
  {
    this->setNodeProperty<double>(mxconst::get_ATTRIB_LAT(), inValue);  // v3.0.241.8

    lat        = inValue;
    pointState = missionx::mx_point_state::defined;
  }

  double getLat() { return Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_LAT(), 0.0); }
  // -----------------------------------
  std::string getLat_s() { return Utils::readAttrib(this->node, mxconst::get_ATTRIB_LAT(), ""); } // v3.0.301
  // -----------------------------------
  std::string getLon_s() { return Utils::readAttrib(this->node, mxconst::get_ATTRIB_LONG(), ""); } // v3.0.301
  // -----------------------------------
  std::string getElevFt_s() { return Utils::readAttrib(this->node, mxconst::get_ATTRIB_ELEV_FT(), ""); } // v3.0.301
   

  // -----------------------------------
  void setLon(double inValue)
  {
    this->setNodeProperty<double>(mxconst::get_ATTRIB_LONG(), inValue);  // v3.0.241.8
    lon        = inValue;
    pointState = missionx::mx_point_state::defined;
  }

  double getLon() { return Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_LONG(), 0.0); }

  // -----------------------------------

  void setHeading(double inValue)
  {
    this->setNodeProperty<double>(mxconst::get_ATTRIB_HEADING_PSI(), inValue);  // v3.0.241.8
    heading    = (float)inValue;
    pointState = missionx::mx_point_state::defined;
  }

  double getHeading() { return Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_HEADING_PSI(), 0.0); }

  // -----------------------------------

  void setRoll(double inValue)
  {
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ROLL(), inValue);  // v3.0.241.8

    roll       = (float)inValue;
    pointState = missionx::mx_point_state::defined;
  }

  double getRoll() { return Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ROLL(), 0.0); }


  // -----------------------------------
  void setPitch(double inValue)
  {
    this->setNodeProperty<double>(mxconst::get_ATTRIB_PITCH(), inValue);  // v3.0.241.8

    pitch      = (float)inValue;
    pointState = missionx::mx_point_state::defined;
  }

  double getPitch() { return Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_PITCH(), 0.0); }


  // -----------------------------------
  void setAdjustHeading(double inValue)
  {
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ADJUST_HEADING(), inValue);  // v3.0.241.8

    adjust_heading = (float)inValue;
    pointState     = missionx::mx_point_state::defined;
  }

  double getAdjustHeading() { return Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ADJUST_HEADING(), 0.0); }


  // -----------------------------------

  void storeDataFromNodeToPoint() // v3.0.301 B3
  {
    assert(!this->node.isEmpty() && "[storeDataFromNodeToPoint] Node is empty!!!");
    this->setLat(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_LAT(), 0.0));
    this->setLon(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_LONG(), 0.0));
    this->setElevationFt(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ELEV_FT(), 0.0));
    this->setHeading(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_HEADING_PSI(), 0.0));
    this->setAdjustHeading(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ADJUST_HEADING(), 0.0));
    this->setPitch(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_PITCH(), 0.0));
    this->setRoll(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ROLL(), 0.0));
    this->setElevationAboveGroundFt(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), 0.0));
    this->setSpeedInKmh(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_SPEED_KMH(), 0.0));
    this->timeToWaitOnPoint_sec = Utils::readNodeNumericAttrib<float>(this->node, mxconst::get_ATTRIB_WAIT_SEC(), 0.0f);
  }

  // -----------------------------------

  void storeDataToPointNode() // v3.0.213.7
  {
    assert(!this->node.isEmpty() && "[storeDataToPointNode] Node is not available");
    this->setNodeProperty<double>(mxconst::get_ATTRIB_LAT(), lat                  ); 
    this->setNodeProperty<double>(mxconst::get_ATTRIB_LONG(), lon                 ); 
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_FT(), elevation_ft     ); 
    this->setNodeProperty<double>(mxconst::get_ATTRIB_HEADING_PSI(), heading      ); 
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ADJUST_HEADING(), adjust_heading); 
    this->setNodeProperty<double>(mxconst::get_ATTRIB_PITCH(), pitch              ); 
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ROLL(), roll                ); 
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), elev_above_ground_ft ); 
    this->setNodeProperty<double>(mxconst::get_ATTRIB_SPEED_KMH(), speed_kmh                       ); 
    this->setNodeProperty<double>(mxconst::get_ATTRIB_WAIT_SEC(), timeToWaitOnPoint_sec            ); 
  }

  // -----------------------------------

  void saveCheckpoint(IXMLNode& inParent)
  {
    IXMLNode point = inParent.addChild(mxconst::get_ELEMENT_POINT().c_str());

    point.addAttribute(mxconst::get_ATTRIB_LAT().c_str(), mxUtils::formatNumber<double>(lat, 8).c_str());
    point.addAttribute(mxconst::get_ATTRIB_LONG().c_str(), mxUtils::formatNumber<double>(lon, 8).c_str());
    point.addAttribute(mxconst::get_ATTRIB_ELEV_FT().c_str(), mxUtils::formatNumber<double>(this->getElevationInFeet(), 3).c_str());

    point.addAttribute(mxconst::get_ATTRIB_HEADING_PSI().c_str(), mxUtils::formatNumber<float>(heading, 2).c_str());
    point.addAttribute(mxconst::get_ATTRIB_ADJUST_HEADING().c_str(), mxUtils::formatNumber<float>(adjust_heading, 2).c_str());
    point.addAttribute(mxconst::get_ATTRIB_PITCH().c_str(), mxUtils::formatNumber<float>(pitch, 2).c_str());
    point.addAttribute(mxconst::get_ATTRIB_ROLL().c_str(), mxUtils::formatNumber<float>(roll, 2).c_str());
    point.addAttribute(mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT().c_str(), mxUtils::formatNumber<double>(elev_above_ground_ft, 2).c_str());
    point.addAttribute(mxconst::get_ATTRIB_SPEED_KMH().c_str(), mxUtils::formatNumber<float>(speed_kmh, 2).c_str());
    point.addAttribute(mxconst::get_ATTRIB_WAIT_SEC().c_str(), mxUtils::formatNumber<float>(timeToWaitOnPoint_sec, 2).c_str());

  }
  // -----------------------------------
  void resetData()
  {
    lat = storeLat;
    lon = storeLon;
    setElevationFt(store_elev_ft);                         // will set feet and meter
    setElevationAboveGroundFt(store_elev_above_ground_ft); // v3.0.202

    heading        = storeHeading;
    adjust_heading = storeAdjustHeading; // v3.0.207.5
    pitch          = storePitch;
    roll           = storeRoll;

    this->calcSimLocalData();

    storeDataToPointNode();
  }

  // -----------------------------------

  void setElevationFt(double inValueInFeet)
  {
    this->elevation_ft = inValueInFeet;
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_FT(), inValueInFeet);  // v3.0.241.1

    elevation_mt = elevation_ft * missionx::feet2meter;
    pointState   = missionx::mx_point_state::defined;
  }

  // -----------------------------------

  void setElevationMt(double inValueInMeter)
  {
    this->elevation_mt = inValueInMeter;
    elevation_ft       = elevation_mt * missionx::meter2feet;
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_FT(), elevation_ft);  // v3.0.241.1

    pointState = missionx::mx_point_state::defined;
  }

  // -----------------------------------

  void setElevationAboveGroundFt(double inValueInFeet)
  {
    this->elev_above_ground_ft = inValueInFeet;
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), inValueInFeet);  // v3.0.241.1

    pointState = missionx::mx_point_state::defined;
  }

  // -----------------------------------

  void setElevationAboveGroundMt(double inValueInMeter)
  {
    elev_above_ground_ft = inValueInMeter * missionx::meter2feet;
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), elev_above_ground_ft);  // v3.0.241.1


    pointState = missionx::mx_point_state::defined;
  }

  // -----------------------------------

  void setSpeedInFts(double inSpeed) // v3.0.202
  {
    this->speed_fts = (float)inSpeed;
    this->speed_kmh = (float)(inSpeed * missionx::fts2kmh);
  }

  // -----------------------------------

  void setSpeedInKmh(double inSpeed) // v3.0.202
  {
    this->speed_kmh = (float)inSpeed;
    this->speed_fts = (float)(inSpeed * missionx::kmh2fts);
  }

  // -----------------------------------

  float getSpeedKmh() { return (this->speed_kmh >= 0.0f) ? this->speed_kmh : 0.0f; }

  // -----------------------------------

  float getSpeedFts() { return (this->speed_fts >= 0.0f) ? this->speed_fts : 0.0f; }

  // -----------------------------------

  double getElevationInMeter()
  {
    const double elev_ft_d = Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ELEV_FT(), 0.0); // v3.0.241.8
    if (elev_ft_d == this->elevation_ft)
      return elev_ft_d * missionx::feet2meter;

    return this->elevation_ft * missionx::feet2meter;
  }

  // -----------------------------------

  double getElevationInFeet()
  {
    const double elev_ft_d = Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ELEV_FT(), 0.0); // v3.0.241.8
    if (elev_ft_d == this->elevation_ft)
      return elev_ft_d;

    return this->elevation_ft;
  }

  // -----------------------------------

  double getElevationAboveGroundInMeter()
  {
    this->elev_above_ground_ft = Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), 0.0); // v3.0.241.8
    return elev_above_ground_ft * missionx::feet2meter; // return elevation in meters
  }

  // -----------------------------------

  double getElevationAboveGroundInFeet()
  {
    // v3.0.213.7
    this->elev_above_ground_ft = Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), 0.0); // v3.0.241.8
    return elev_above_ground_ft;
  }

  // -----------------------------------

  double distanceTo(Point q)
  {
    return std::hypot(this->lat - q.lat, this->lon - q.lon);
  }

  // -----------------------------------
 
  std::string get_point_lat_lon_as_string() // v3.0.301
  {
    return "<point " + mxconst::get_ATTRIB_LAT() + "=\"" + getLat_s() + "\" " + mxconst::get_ATTRIB_LONG() + "=\"" + getLon_s() + "\" " + mxconst::get_ATTRIB_ELEV_FT() + "=\"" + getElevFt_s() + "\" />";
  }


  // -----------------------------------
  
  std::string to_string()
  {
    std::string format;
    format.clear();
    IXMLRenderer xmlWriter;
    format = xmlWriter.getString(this->node);
    xmlWriter.clear(); // v3.0.241.8


    return format;
  }
  // -----------------------------------
  std::string to_string_with_locals() // This function is maninly internal for low level debug with LR if needed
  {
    std::string format;
    format.clear();

    IXMLRenderer xmlWriter;
    format = xmlWriter.getString(this->node);
    format = "local xyz=[" + Utils::formatNumber<double>(this->local_x, 8) + "," + Utils::formatNumber<double>(this->local_y, 8) + "," + Utils::formatNumber<double>(this->local_z, 8) + "]" + " - " + format;
    xmlWriter.clear(); // v3.0.241.8


    return format;
  }
  // -----------------------------------

  static int slope(int x1, int y1, int x2, int y2)
  {
    if (x2 - x1 == 0)
    {
      return 90;
    }

    double tanx, s;
    tanx = (double)((y2 - y1)) / (double)((x2 - x1));
    s    = atan(tanx);
    s    = (180 / missionx::PI) * s;
    return (int)s;
  }

  // -----------------------------------

  int slope(Point& inP)
  {
    if (inP.lat - this->lat == 0)
    {
      return 90;
    }

    double tanx, s;
    tanx = (inP.lon - this->lon) / (inP.lat - this->lat);
    s    = atan(tanx);
    s    = (180 / missionx::PI) * s;
    return (int)s;
  }
  // -----------------------------------

  static float slope(Point& p1, Point& p2)
  {
    if (p2.lat - p1.lat == 0)
    {
      return 90;
    }


    double tanx, s;
    tanx = (p2.lon - p1.lon) / (p2.lat - p1.lat);
    s    = atan(tanx);
    s    = (180 / missionx::PI) * s;
    return (float)s;
  }

  // -----------------------------------

  double calcDistanceBetween2Points_ts(const Point& pTarget, const missionx::mx_units_of_measure inReturnInUnits = missionx::mx_units_of_measure::nm, std::string* err = nullptr)
  {
    std::lock_guard<std::mutex> lock(this->mt_CalcDistBetweenPointsMutex);

    if (pTarget.pointState == missionx::mx_point_state::point_undefined)
    {
      if (err != nullptr)
        (*err) = "Target point state is undefined. Please fix";

      return -1; // not valid
    }

    return Utils::calcDistanceBetween2Points_nm_ts(this->lat, this->lon, pTarget.lat, pTarget.lon, inReturnInUnits);
  }

  // -----------------------------------

  double calcDistanceBetween2Points(const Point& pTarget, const missionx::mx_units_of_measure inReturnInUnits = missionx::mx_units_of_measure::nm, std::string* err = nullptr)
  {
    if (pTarget.pointState == missionx::mx_point_state::point_undefined)
    {
      if (err != nullptr)
        (*err) = "Target point state is undefined. Please fix";

      return -1; // not valid
    }


    return Utils::calcDistanceBetween2Points_nm(this->lat, this->lon, pTarget.lat, pTarget.lon, inReturnInUnits);
  }

  // -----------------------------------

  static double calcDistanceBetween2Points(Point& pOrigin, Point& pTarget, const missionx::mx_units_of_measure inReturnInUnits = mx_units_of_measure::nm, std::string* err = nullptr)
  {
    if (pTarget.pointState == missionx::mx_point_state::point_undefined || pOrigin.pointState == missionx::mx_point_state::point_undefined)
    {
      if (err != nullptr)
        (*err) = "Target or Origin point state is undefined. Please fix";

      return -1; // not valid
    }

    return pOrigin.calcDistanceBetween2Points(pTarget, inReturnInUnits);
  }

  // -----------------------------------

  // calculate target point relative to Origin point, given bearing(heading) and distance in nautical Miles
  void static calcPointBasedOnDistanceAndBearing_2DPlane(Point& pOrigin, Point& pTarget, float inHdg, double inDistance, missionx::mx_units_of_measure inUnits = missionx::mx_units_of_measure::nm)
  {
    float distanceInNm = (float)inDistance;

    if (inUnits != missionx::mx_units_of_measure::nm)
    {
      distanceInNm = Utils::convertToNm(distanceInNm, inUnits);
    }


    Utils::calcPointBasedOnDistanceAndBearing_2DPlane(pTarget.lat, pTarget.lon, pOrigin.lat, pOrigin.lon, inHdg, distanceInNm);

    // v3.0.219.11 added set properties since lat/long variables are not being used in Point, only property values
    pTarget.setLat(pTarget.lat);
    pTarget.setLon(pTarget.lon);
  }

  // v3.0.301 B3
  missionx::Point 
  static calcPointBasedOnDistanceAndBearing_2DPlane(Point& pOrigin, float inHdg, double inDistance, missionx::mx_units_of_measure inUnits = missionx::mx_units_of_measure::nm)
  {
    Point pTarget(0.0, 0.0);

    float distanceInNm = (float)inDistance;

    if (inUnits != missionx::mx_units_of_measure::nm)
    {
      distanceInNm = Utils::convertToNm(distanceInNm, inUnits);
    }


    Utils::calcPointBasedOnDistanceAndBearing_2DPlane(pTarget.lat, pTarget.lon, pOrigin.lat, pOrigin.lon, inHdg, distanceInNm);

    // v3.0.219.11 added set properties since lat/long variables are not being used in Point, only property values
    pTarget.setLat(pTarget.lat);
    pTarget.setLon(pTarget.lon);
    return pTarget;
  }


  // -----------------------------------
  static missionx::mx_location_3d_objects readPointElement(ITCXMLNode& xNode)
  {
    missionx::mx_location_3d_objects info;

    info.lat               = Utils::readAttrib(xNode, mxconst::get_ATTRIB_LAT(), "");
    info.lon               = Utils::readAttrib(xNode, mxconst::get_ATTRIB_LONG(), "");
    info.elev              = Utils::readAttrib(xNode, mxconst::get_ATTRIB_ELEV_FT(), "");
    info.elev_above_ground = Utils::readAttrib(xNode, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), "");

    // v3.0.202 added move 3d info
    info.heading = Utils::readAttrib(xNode, mxconst::get_ATTRIB_HEADING_PSI(), "");
    info.pitch   = Utils::readAttrib(xNode, mxconst::get_ATTRIB_PITCH(), "");
    info.roll    = Utils::readAttrib(xNode, mxconst::get_ATTRIB_ROLL(), "");
    info.speed   = Utils::readAttrib(xNode, mxconst::get_ATTRIB_SPEED_KMH(), "");

    // v3.0.207.5
    info.adjust_heading = Utils::readAttrib(xNode, mxconst::get_ATTRIB_ADJUST_HEADING(), "0"); // zero means no adjustment


    return info;
  }
  // -----------------------------------

//  bool loadPointElement(ITCXMLNode& inPointNode, std::string& outErr)
  bool loadPointElement(ITCXMLNode& inPointNode)
  {
    missionx::mx_location_3d_objects info = Point::readPointElement(inPointNode);

    // location validations
    if (Utils::is_number(info.lat) && Utils::is_number(info.lon))
    {
      setLat(mxUtils::stringToNumber<double>(info.lat));
      setLon(mxUtils::stringToNumber<double>(info.lon));
    }
    else
    {
      Log::logMsgErr("[read point] One of the coordination Lat/Lon might be malformed: " + mxconst::get_QM() + info.lat + "," + info.lon + mxconst::get_QM() + ". Skipping...");
      return false;
    }

    if (!info.elev.empty() && Utils::is_number(info.elev))
      this->setElevationFt(mxUtils::stringToNumber<double>(info.elev));
    else
      this->setElevationFt(0); // on ground

    // add elevation above ground logic // v3.0.200
    if (Utils::is_number(info.elev_above_ground))
    {
      double elevAboveGround = mxUtils::stringToNumber<double>(info.elev_above_ground);
      if (elevAboveGround != 0.0)
      {
        XPLMProbeResult outProbeResult;
        float           groundElevation = (float)Point::getTerrainElevInMeter_FromPoint((*this), outProbeResult);
        if (outProbeResult == xplm_ProbeHitTerrain)
          this->setElevationFt(groundElevation + elevAboveGround);
      }
    }
    else
      this->setElevationAboveGroundFt(0); // basically ignore this value and use elevation value instead

    // read tilt information: heading, pitch, roll
    if (!info.heading.empty() && mxUtils::is_number(info.heading))
    {
      this->setHeading(mxUtils::stringToNumber<float>(info.heading));
    }
    else
      this->setHeading(0.0f);

    if (!info.pitch.empty() && mxUtils::is_number(info.pitch))
    {
      this->setPitch(mxUtils::stringToNumber<float>(info.pitch));
    }
    else
      this->setPitch(0.0f);

    if (!info.roll.empty() && mxUtils::is_number(info.roll))
    {
      this->setRoll(mxUtils::stringToNumber<float>(info.roll));
    }
    else
      this->setRoll(0.0f);

    if (!info.speed.empty() && mxUtils::is_number(info.speed))
    {
      this->setSpeedInKmh(mxUtils::stringToNumber<float>(info.speed));
    }
    else
      this->setSpeedInKmh(0.0); // v3.0.253.7

    // v3.0.207.5 - adjust heading
    if (!info.adjust_heading.empty() && mxUtils::is_number(info.adjust_heading))
    {
      this->setAdjustHeading(mxUtils::stringToNumber<float>(info.adjust_heading));
    }
    else
      this->setAdjustHeading(0.0f); // v3.0.207.2 default speed 10kmh


    return true; // pointIsValid;
  }


  // -----------------------------------
  // read only 1 child point element of inParentPointNode parent element
//  bool readChildPoint(ITCXMLNode& inParentPointNode, std::string& outErr)
//  {
//    outErr.clear();
//    ITCXMLNode xPoint = inParentPointNode.getChildNode(mxconst::get_ELEMENT_POINT().c_str());
//    if (!xPoint.isEmpty())
//      loadPointElement(xPoint, outErr);
//    else
//      outErr = "Point data is empty.";

//    return outErr.empty(); // empty = read successfully data
//  }
  // -----------------------------------

  // call XPLMWorldToLocal() from point lat/lon to local_x,local_y
  void calcSimLocalData()
  {
    // calculate locals from lat long elev. Will need to re-calculate every scenery change
    XPLMWorldToLocal(getLat(), getLon(), getElevationInMeter(), &local_x, &local_y, &local_z);
  }

// ---------------------------------------

  static double getTerrainElevInMeter_FromPoint(missionx::Point& p1, XPLMProbeResult& outResult)
  {
    double local_x = 0.0;
    double local_y = 0.0;
    double local_z = 0.0;
    double lat, lon, elev_mt;
    // lat = lon = elev_mt = 0.0;

    XPLMProbeInfo_t info;

    outResult = -1;

    lat     = p1.getLat();
    lon     = p1.getLon();
    elev_mt = p1.getElevationInMeter();

    XPLMWorldToLocal(lat, lon, 0, &local_x, &local_y, &local_z);

    info = Point::terrain_Y_Probe(local_x, local_y, local_z, outResult);
    if (outResult == 0)
    {
      XPLMLocalToWorld(info.locationX, info.locationY, info.locationZ, &lat, &lon, &elev_mt);
      p1.setBoolProperty(mxconst::get_PROP_IS_WET(), info.is_wet);
      p1.setElevationMt(elev_mt);
    }
    else
      elev_mt = 0.0;

    return elev_mt;
  }


// ---------------------------------------
// ---------------------------------------

  static int probeIsWet(missionx::Point& p1, XPLMProbeResult& outResult)
  {
    XPLMProbeInfo_t info;

    outResult = -1;
    p1.calcSimLocalData();

    info = Point::terrain_Y_Probe(p1.local_x, p1.local_y, p1.local_z, outResult);
    if (outResult == 0)
    {
      p1.setBoolProperty(mxconst::get_PROP_IS_WET(), info.is_wet);
      return info.is_wet;
    }


    return 0;
  }

// ---------------------------------------
  void set_isWet(bool inVal)
  {
    this->setNodeProperty<bool>(mxconst::get_PROP_IS_WET(), inVal); 
  }

// ---------------------------------------

  static XPLMProbeInfo_t terrain_Y_Probe(double& inLocalXgl, double& inLocalYgl, double& inLocalZgl, XPLMProbeResult& outResult)
  {

    XPLMProbeRef probe = NULL;
    // Create a Y probe
    probe = XPLMCreateProbe(xplm_ProbeY);

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

    outResult = XPLMProbeTerrainXYZ(probe, (float)inLocalXgl, (float)inLocalYgl, (float)inLocalZgl, &info);


    XPLMDestroyProbe(probe);

    return info;
  }

  // -----------------------------------
  // Terrain Y probe information is stored in Point::probe_info and returns the probe_result.
  // You can check the "Point::probe_info" struct for coordination information
  XPLMProbeResult calc_terrain_Y_Probe_info()
  {
    XPLMProbeResult probe_result;
    XPLMProbeRef    probe = NULL;

    this->calcSimLocalData();

    // Create a Y probe
    probe = XPLMCreateProbe(xplm_ProbeY);

    this->probe_info.structSize = sizeof(this->probe_info);

    probe_result = XPLMProbeTerrainXYZ(probe, (float)this->local_x, (float)this->local_y, (float)this->local_z, &this->probe_info);

    XPLMDestroyProbe(probe);

    this->flag_wasProbed = true;
    return probe_result;
  }
  // -----------------------------------

  double get_terrain_elev_mt_from_probe()
  {
    XPLMWorldToLocal(this->lat, this->lon, 0, &this->local_x, &this->local_y, &this->local_z);

    if (this->calc_terrain_Y_Probe_info() == xplm_ProbeHitTerrain)
    {
      XPLMLocalToWorld(this->probe_info.locationX, this->probe_info.locationY, this->probe_info.locationZ, &this->lat, &this->lon, &this->terrain_elev_mt); // write elevation to current Point.
      XPLMWorldToLocal(this->lat, this->lon, this->terrain_elev_mt, &this->local_x, &this->local_y, &this->local_z); // calculate with Terrain elevation data so this->local_y will hold terrain level information
      XPLMProbeRef probe = XPLMCreateProbe(xplm_ProbeY);
      this->probe_result = XPLMProbeTerrainXYZ(probe, (float)local_x, (float)local_y, (float)local_z, &this->probe_info); // Once again for improved precision
      XPLMDestroyProbe(probe);

      return this->terrain_elev_mt;
    }

    return 0.0; // return something.
  }
  // -----------------------------------
  // this function takes into account that the get_elevation_xx() function returns the real elevation after calculating the above ground.
  void calc_elevation_include_above_ground_info_and_sync_to_WorldToLocalData() 
  {
    constexpr auto added_elev_mt = 1000.0 * 0.3048; // missionx::feet2meter

    const auto lmbda_get_elevation_in_meters = [&]() {
      this->terrain_elev_mt = this->get_terrain_elev_mt_from_probe();

      const std::string above_ground_ft_s = Utils::readAttrib(this->node, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), "");
      const std::string elev_ft_s         = Utils::readAttrib(this->node, mxconst::get_ATTRIB_ELEV_FT(), "");

      if (!above_ground_ft_s.empty() && mxUtils::stringToNumber<double>(above_ground_ft_s, 2) != 0.0)
      {
        return ( fabs(mxUtils::stringToNumber<double>(above_ground_ft_s) * missionx::feet2meter)) + terrain_elev_mt;
      }
      else if(elev_ft_s.empty())
        return terrain_elev_mt + added_elev_mt;
      else
      {
        const double elev_mt_d = mxUtils::stringToNumber<double>(elev_ft_s) * missionx::feet2meter;
        return (elev_mt_d > this->terrain_elev_mt) ? elev_mt_d : this->terrain_elev_mt;
      }

    };

    this->setElevationMt(lmbda_get_elevation_in_meters());

    this->calcSimLocalData();
  }

  // -----------------------------------

  void clearPoint()
  {
    this->lat     = 0.0;
    this->lon     = 0.0;
    this->local_x = 0.0;
    this->local_y = 0.0;
    this->local_z = 0.0;

    this->elevWasProbed = false;

    this->pitch   = 0.0f;
    this->heading = 0.0f;
    this->roll    = 0.0f;

    this->speed_fts             = 0.0f;
    this->speed_kmh             = 0.0f;
    this->timeToWaitOnPoint_sec = 0.0f;

    this->setElevationFt(0.0);
    this->setElevationAboveGroundFt(0.0); // v3.0.202

    this->storeDataToPointNode(); // v3.0.241.8

    pointState = missionx::mx_point_state::point_undefined;
  }

  // -----------------------------------

  std::string to_string_xy()
  {
    std::string format;
    format.clear();

    format = "lat: " + mxUtils::formatNumber<double>(lat, 8) + ", lon: " + mxUtils::formatNumber<double>(lon, 8);

    return format;
  }

  // -----------------------------------

  std::string format_point_to_savepoint()
  {
    return mxUtils::formatNumber<double>(lat, 8) + "/" + mxUtils::formatNumber<double>(lon, 8) + "/" + mxUtils::formatNumber<double>(elevation_ft, 3) + "|" + mxUtils::formatNumber<double>(heading, 2) + "/" +
           mxUtils::formatNumber<double>(pitch, 2) + "/" + mxUtils::formatNumber<double>(roll, 2) + "|" + mxUtils::formatNumber<double>(speed_fts, 2) + "/" + mxUtils::formatNumber<double>(timeToWaitOnPoint_sec, 2);
  }


  // -----------------------------------

  bool parse_savepoint_format_to_point(std::string inVal)
  {
    std::vector<std::string> vecPositionParts; // Holds the "|" strings
    std::vector<double>      vecLatLonElev;    // Holds lat/lon/elev numbers
    std::vector<double>      vecHeadPitchRoll; // Holds heading/pitch/roll numbers
    std::vector<double>      vecSpeedTimewait; // Holds speed and time to wait on point

    vecPositionParts = mxUtils::split_v2(inVal, "|");
    if (vecPositionParts.size() < 3)
    {
      Log::logMsgErr("[parse_savepoint_format_to_point] Location format is not valid: " + inVal);
      return false;
    }
    else
    {
      vecLatLonElev    = Utils::splitStringToNumbers<double>(vecPositionParts.at(0), "/");
      vecHeadPitchRoll = Utils::splitStringToNumbers<double>(vecPositionParts.at(1), "/");
      vecSpeedTimewait = Utils::splitStringToNumbers<double>(vecPositionParts.at(2), "/");

      auto vecSize = vecLatLonElev.size();
      int  counter = 0;
      for (auto i1 = (size_t)0; i1 < vecSize; ++i1, ++counter)
      // for (auto & num : vecLatLonElev)
      {
        switch (counter)
        {
          case 0:
            this->setLat(vecLatLonElev.at(counter));
            break;
          case 1:
            this->setLon(vecLatLonElev.at(counter));
            break;
          case 2:
            this->setElevationFt(vecLatLonElev.at(counter));
            break;

        } // end switch
        //++counter;
      } // end loop vecLatLonElev

      Log::logDebugBO("Parsed: " + Utils::formatNumber<int>(counter - 1) + " elements for lat/lon/elev_ft - (" + this->getLat_s() + "/" + this->getLon_s() + ")");

      counter = 0;
      vecSize = vecHeadPitchRoll.size();
      for (auto i1 = (size_t)0; i1 < vecSize; ++i1, ++counter)
      {
        switch (counter)
        {
          case 0:
            this->setHeading(vecHeadPitchRoll.at(counter));
            break;
          case 1:
            this->setPitch(vecHeadPitchRoll.at(counter));
            break;
          case 2:
            this->setRoll(vecHeadPitchRoll.at(counter));
            break;

        } // end switch
      } // end loop vecHeadPitchRoll

      Log::logDebugBO("Parsed: " + Utils::formatNumber<int>(counter - 1) + " elements for Heading/Pitch/Roll - (" + Utils::readAttrib(this->node, mxconst::get_ATTRIB_HEADING_PSI(), "") + "/" + Utils::readAttrib(this->node, mxconst::get_ATTRIB_PITCH(), "") + "/" + Utils::readAttrib(this->node, mxconst::get_ATTRIB_ROLL(), "") + ")" );

      vecSize = vecSpeedTimewait.size();
      counter = 0;
      for (auto i1 = (size_t)0; i1 < vecSize; ++i1, ++counter)
      //        for (auto & num : vecSpeedTimewait)
      {
        switch (counter)
        {
          case 0:
            this->setSpeedInFts(vecSpeedTimewait.at(counter));
            break;
          case 1:
            this->timeToWaitOnPoint_sec = (float)vecSpeedTimewait.at(counter);
            break;
        } // end switch
      } // end loop vecSpeedTimewait

    } // end else if vecPositionParts has 3 parts

    return true;
  }

  // -----------------------------------

  static missionx::mx_location_3d_objects readLocationElement(ITCXMLNode& xNode)
  {
    IXMLNode xNode_local = xNode.deepCopy();
    return readLocationElement(xNode_local);
  }

  // -----------------------------------

  static missionx::mx_location_3d_objects readLocationElement(IXMLNode& xNode)
  {
    missionx::mx_location_3d_objects info;

    info.lat               = Utils::readAttrib(xNode, mxconst::get_ATTRIB_LAT(), "");
    info.lon               = Utils::readAttrib(xNode, mxconst::get_ATTRIB_LONG(), "");
    info.elev              = Utils::readAttrib(xNode, mxconst::get_ATTRIB_ELEV_FT(), "");
    info.elev_above_ground = Utils::readAttrib(xNode, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), "");

    // v3.0.202 added move 3d info
    info.heading = Utils::readAttrib(xNode, mxconst::get_ATTRIB_HEADING_PSI(), "");
    info.pitch   = Utils::readAttrib(xNode, mxconst::get_ATTRIB_PITCH(), "");
    info.roll    = Utils::readAttrib(xNode, mxconst::get_ATTRIB_ROLL(), "");
    info.speed   = Utils::readAttrib(xNode, mxconst::get_ATTRIB_SPEED_KMH(), "");

    // v3.0.207.5
    info.adjust_heading = Utils::readAttrib(xNode, mxconst::get_ATTRIB_ADJUST_HEADING(), "0"); // zero means no adjustment


    return info;
  }
  // -----------------------------------

  bool parse_node()
  {
    assert(!this->node.isEmpty()); // v3.0.241.1 abort is node is not set and locationNode is not set

    if (this->node.isEmpty())
    {
      Log::logMsgErr("[parse Point] node is empty. Something is wrong. Skipping point parsing. Notify developer.");
      return false;
    }

    missionx::mx_location_3d_objects info = Point::readLocationElement(this->node); // this function reads the full scope of 3D Object location but it also shares same attributes as point

    // location validations
    if (Utils::is_number(info.lat) && Utils::is_number(info.lon))
    {
      this->setLat(mxUtils::stringToNumber<double>(info.lat));
      this->setLon(mxUtils::stringToNumber<double>(info.lon));
    }
    else
    {
      Log::logMsgErr("[read point] One of the coordination Lat/Lon might be malformed: " + mxconst::get_QM() + info.lat + "," + info.lon + mxconst::get_QM() + ". Skipping...");
      // pointIsValid = false;
      return false;
    }

    if (!info.elev.empty() && Utils::is_number(info.elev))
      this->setElevationFt(mxUtils::stringToNumber<double>(info.elev));
    else
      this->setElevationFt(0); // on ground

    // add elevation above ground logic // v3.0.200
    if (Utils::is_number(info.elev_above_ground))
    {
      double elevAboveGround = mxUtils::stringToNumber<double>(info.elev_above_ground);
      if (elevAboveGround != 0.0)
      {
        XPLMProbeResult outProbeResult;
        float           groundElevation = (float)Point::getTerrainElevInMeter_FromPoint((*this), outProbeResult);
        if (outProbeResult == xplm_ProbeHitTerrain)
          this->setElevationFt(groundElevation + elevAboveGround);
      }
    }
    else
      this->setElevationAboveGroundFt(0); // basically ignore this value and use elevation value instead

    // read tilt information: heading, pitch, roll
    if (!info.heading.empty() && mxUtils::is_number(info.heading))
    {
      this->setHeading(mxUtils::stringToNumber<float>(info.heading));
    }
    else
      this->setHeading(0.0f);

    if (!info.pitch.empty() && mxUtils::is_number(info.pitch))
    {
      this->setPitch(mxUtils::stringToNumber<float>(info.pitch));
    }
    else
      this->setPitch(0.0f);

    if (!info.roll.empty() && mxUtils::is_number(info.roll))
    {
      this->setRoll(mxUtils::stringToNumber<float>(info.roll));
    }
    else
      this->setRoll(0.0f);

    if (!info.speed.empty() && mxUtils::is_number(info.speed))
    {
      this->setSpeedInKmh(mxUtils::stringToNumber<float>(info.speed));
    }
    else
      this->setSpeedInKmh(0.0); // v3.0.253.7

    // v3.0.207.5 - adjust heading
    if (!info.adjust_heading.empty() && mxUtils::is_number(info.adjust_heading))
    {
      this->adjust_heading = mxUtils::stringToNumber<float>(info.adjust_heading);
    }
    else
      this->adjust_heading = 0.0f; // v3.0.207.2 default speed 10kmh



    return true;
  }

  // -----------------------------------
  // -----------------------------------
};

} // namespace

#endif
