#ifndef POINTS_H_
#define POINTS_H_

#pragma once

#include <deque>

#include "Point.hpp"
#include <algorithm>
#include <math.h>
#include <unordered_set>
#include "../mx_base_node.h"

using namespace missionx;
// using namespace mxconst;

namespace missionx
{
// v3.0.253.5 added strct_box
typedef struct _strct_box
{
  Point topLeft;
  Point topRight;
  Point bottomLeft;
  Point bottomRight;
  Point center;

  void calcBoxBasedOn_topLeft(double in_distance)
  {
    Utils::calcPointBasedOnDistanceAndBearing_2DPlane(topRight.lat, topRight.lon, topLeft.lat, topLeft.lon, 90.0f, in_distance);             // relative to topLeft
    Utils::calcPointBasedOnDistanceAndBearing_2DPlane(bottomLeft.lat, bottomLeft.lon, topLeft.lat, topLeft.lon, 180.0f, in_distance);        // relative to topLeft
    Utils::calcPointBasedOnDistanceAndBearing_2DPlane(bottomRight.lat, bottomRight.lon, bottomLeft.lat, bottomLeft.lon, 90.0f, in_distance); // relative to bottomLeft
    center.lat = (bottomLeft.lat + topRight.lat) * 0.5;
    center.lon = (bottomLeft.lon + topRight.lon) * 0.5;
  }

  void calcBoxBasedOn_bottomLeft_edgeDistances_and_heading(double vector_length_nm_from_bottom_to_top, double vector_length_nm_left_to_right, float initial_deg_f)
  {
    const auto init_rect_bearing_f        = mxUtils::convert_skewed_bearing_to_degrees(initial_deg_f);
    const auto next_rect_bearing_add_90_f = mxUtils::convert_skewed_bearing_to_degrees(init_rect_bearing_f + 90.0f);

    // calc topLeft
    Utils::calcPointBasedOnDistanceAndBearing_2DPlane(topLeft.lat, topLeft.lon, bottomLeft.lat, bottomLeft.lon, init_rect_bearing_f, vector_length_nm_from_bottom_to_top); // topLeft relative to bottomLeft

    // calc bottomRight
    Utils::calcPointBasedOnDistanceAndBearing_2DPlane(bottomRight.lat, bottomRight.lon, bottomLeft.lat, bottomLeft.lon, next_rect_bearing_add_90_f, vector_length_nm_left_to_right); // bottomRight relative to bottomLeft

    // calc topRight from calculated topLeft
    Utils::calcPointBasedOnDistanceAndBearing_2DPlane(topRight.lat, topRight.lon, topLeft.lat, topLeft.lon, next_rect_bearing_add_90_f, vector_length_nm_left_to_right); // topRight relative to topLeft

    center.setLat((bottomLeft.lat + topRight.lat) * 0.5);
    center.setLon((bottomLeft.lon + topRight.lon) * 0.5);

    topLeft.storeDataToPointNode();
    topRight.storeDataToPointNode();
    bottomLeft.storeDataToPointNode();
    bottomRight.storeDataToPointNode();
    center.storeDataToPointNode();
  }

  std::string print_BL_and_TR()
  {
    return "bL:" + mxUtils::formatNumber<double>(bottomLeft.lat, 6) + "," + mxUtils::formatNumber<double>(bottomLeft.lon, 6) + ".tR:" + mxUtils::formatNumber<double>(topRight.lat, 6) + "," + mxUtils::formatNumber<double>(topRight.lon, 6);
  }

} strct_box;

// ---------------------------------------



class Points : public mx_base_node //public mxProperties
{
public:
  // Constructor
  Points() {
    init();
  }

  // core attributes
  Point          pCenter;
  mx_point_state pointsState;
  std::deque<missionx::Point> deqPoints;

  // ---------------------------------------
  // members

  void clone(std::deque<missionx::Point>& inVecPoints)
  {
    init();
    this->deqPoints = inVecPoints;

    pointsState = mx_point_state::defined;
  }

  // ---------------------------------------

  void addPoint(missionx::Point p)
  {
    this->deqPoints.push_back(p);
    pointsState = mx_point_state::defined;
  }

  // ---------------------------------------

  void clear() { init(); }


  // ---------------------------------------
  void init()
  {
    pointsState = mx_point_state::point_undefined;
    deqPoints.clear();
  }

  // ---------------------------------------


  ////////////////
  // Convex Haul//
  ///////////////
  // https://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain#C.2B.2B

  // 2D cross product of OA and OB vectors, i.e. z-component of their 3D cross product.
  // Returns a positive value, if OAB makes a counter-clockwise turn,
  // negative for clockwise turn, and zero if the points are collinear.
  // cross was renamed to ccw
  static double ccw(missionx::Point& O, missionx::Point& A, missionx::Point& B)
  {
    return (A.getLat() - O.getLat()) * (B.getLon() - O.getLon()) - (A.getLon() - O.getLon()) * (B.getLat() - O.getLat());
  }

  // ---------------------------------------

  // Returns a list of points on the convex hull in counter-clockwise order.
  // Note: the last point in the returned list is the same as the first one.
  // static std::vector<missionx::Point> convex_hull(std::vector<missionx::Point> P)
//  static void convex_hull(std::vector<missionx::Point>& P)
//  {
//    size_t n = P.size(), k = 0;
//    if (n == 1)
//      return;

//    std::vector<missionx::Point> H(2 * n);

//    // Sort points lexicographically
//    std::sort(P.begin(), P.end());

//    // Build lower hull
//    for (size_t i = 0; i < n; ++i)
//    {
//      while (k >= (size_t)2 && Points::ccw(H[k - (size_t)2], H[k - 1], P[i]) <= 0.0)
//        k--;
//      H[k++] = P[i];
//    }

//    // Build upper hull
//    for (size_t i = n - 2, t = k + 1; i >= 0; i--)
//    {
//      // while (k >= t && Points::cross(H[k - 2], H[k - 1], P[i]) <= 0) k--;
//      while (k >= t && Points::ccw(H[k - (size_t)2], H[k - (size_t)1], P[i]) <= 0.0)
//        k--;
//      H[k++] = P[i];
//    }

//    H.resize(k - (size_t)1);

//    P = H;
//    // return H;
//  }


  //static void convex_hull(std::vector<missionx::Point>& P)
  //{
  //  int n = (int)P.size(), k = 0;
  //  if (n == 1)
  //    return;

  //  std::vector<missionx::Point> H(2 * n);

  //  // Sort points lexicographically
  //  std::sort(P.begin(), P.end());

  //  // Build lower hull
  //  for (int i = 0; i < n; ++i)
  //  {
  //    while (k >= 2 && Points::ccw(H[k - 2], H[k - 1], P[i]) <= 0.0)
  //      k--;
  //    H[k++] = P[i];
  //  }

  //  // Build upper hull
  //  for (int i = n - 2, t = k + 1; i >= 0; i--)
  //  {
  //    // while (k >= t && Points::cross(H[k - 2], H[k - 1], P[i]) <= 0) k--;
  //    while (k >= t && Points::ccw(H[k - 2], H[k - 1], P[i]) <= 0.0)
  //      k--;
  //    H[k++] = P[i];
  //  }

  //  H.resize(k - (int)1);

  //  P = H;
  //  // return H;
  //}


  /* ********************************** */

  bool isPointInPolyArea(Point& point)
  {

    /* The coordinates of the plane coordinations */
    double px, py;
    Point p1;
    Point p2;

    px = point.getLat();
    py = point.getLon();

    // Calculate How many times the ray crosses the area segments
    int crossings = 0;

    double x1, x2;

    size_t counter = 0;
    for (auto &itPoint : deqPoints)
    {
      counter++;
      p1 = (itPoint);
      if (counter < deqPoints.size())
        p2 = deqPoints[counter];
      else
        p2 = deqPoints[0];

      // This is done to ensure that we get the same result when
      // the line goes from left to right and right to left
      if (p1.getLat() < p2.getLat())
      {
        x1 = p1.getLat();
        x2 = p2.getLat();
      }
      else
      {
        x1 = p2.getLat();
        x2 = p1.getLat();
      }

      // First check if the ray is possible to cross the line
      if (px > x1 && px <= x2 && (py < p1.getLon() || py <= p2.getLon()))
      {
        static const double eps = 0.000001f;


        // Calculate the equation of the line
        double dx = p2.getLat() - p1.getLat();
        double dy = p2.getLon() - p1.getLon();
        double k;

        if (fabs(dx) < eps)
        {
#ifdef IBM
          k = std::numeric_limits<double>::infinity(); // max();  //INFINITY; // math.h
#else
          k = std::numeric_limits<double>::max();
#endif
        }
        else
        {
          k = dy / dx;
        }

        double m = p1.getLon() - k * p1.getLat();

        // Find if the ray crosses the line
        double y2 = k * px + m;
        if (py <= y2)
        {
          crossings++;
        }
      }

    } // end for loop

    // Evaluate if plane in Area
    return (crossings % 2 == 1);
  }


  // ---------------------------------------

  // Does line segment intersect ray?
  bool contains(double x0, double y0)
  {
    Point p(x0, y0);

    return contains(p);
  }

  // ---------------------------------------

  // Does line segment intersect ray?
  bool contains(Point p)
  {
    int crossings = 0;

    size_t vSize = deqPoints.size();

    for (size_t i = 0; i + 1 < vSize; i++)
    {
      double x1, x2;
//      x1 = x2 = 0.0;

      Point p1 = deqPoints[i];
      Point p2 = deqPoints[i + 1];

      // this switch between latitudes, fixes the convex hull based vector.
      // This is done to ensure that we get the same result when
      // the line goes from left to right and right to left (clockwise or counter clockwise)
      if (p1.getLat() < p2.getLat())
      {
        x1 = p1.getLat();
        x2 = p2.getLat();
      }
      else
      {
        x1 = p2.getLat();
        x2 = p1.getLat();
      }

      double slope = (p2.getLon() - p1.getLon()) / (x2 - x1);
      bool   cond1 = (x1 <= p.getLat()) && (p.getLat() < x2);
      bool   cond2 = (x2 <= p.getLat()) && (p.getLat() < x1);
      bool   above = (p.getLon() < slope * (p.getLat() - x1) + p1.getLon());

      if (above && (cond1 || cond2) )
        crossings++;
    }

    return (crossings % 2 != 0);
  }

  // ---------------------------------------

  // checkpoint
  void saveCheckpoint(IXMLNode& inParent)
  {
    IXMLNode points = inParent.addChild(mxconst::get_PROP_POINTS().c_str());

    for (auto &p : deqPoints)
      p.saveCheckpoint(points);
  }


  // ---------------------------------------
  // readChildPointElements allow you to read multiple point child elements under parent element.
  static void readChildPointElements(ITCXMLNode& inParentPointNode, std::deque<missionx::Point>& outDeqPoints, int inLimit, std::string& outErr)
  {
    int nChilds = inParentPointNode.nChildNode(mxconst::get_ELEMENT_POINT().c_str());
    inLimit     = (inLimit <= 0) ? nChilds : inLimit; // if limit is 0 then we fetch all points

    for (int i1 = 0; i1 < nChilds && i1 < inLimit; ++i1)
    {
      Point      p;
      ITCXMLNode xPoint = inParentPointNode.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), i1);
      if (!xPoint.isEmpty())
      {
        p.loadPointElement(xPoint);
        if (outErr.empty())
          outDeqPoints.push_back(p);
      }
    }
  }
  // ---------------------------------------

  bool isPointInRadiusArea(Point& trigPoint, float inRadius_nm, Point& objectPoint)
  {
    double distance = 0.0f;

    distance = Utils::calcDistanceBetween2Points_nm(objectPoint.getLat(), objectPoint.getLon(), trigPoint.getLat(), trigPoint.getLon());

    return ((inRadius_nm >= (float)distance) ? true : false);
  }


  // ---------------------------------------
  static Point calculateCenterOfShape(std::deque<missionx::Point>& inDqPoints)
  {
    Point p;

    if (!inDqPoints.empty())
    {

      double       sumLat  = 0.0;
      double       sumLon  = 0.0;
      unsigned int counter = 0;
      //      for ( itPoint = inDqPoints.begin(); itPoint < inDqPoints.end(); itPoint++ )
      for (auto itPoint : inDqPoints)
      {
        sumLat += itPoint.getLat();
        sumLon += itPoint.getLon();
        counter++;
      } // end loop

      p.setLat((sumLat / counter));
      p.setLon((sumLon / counter));
      p.pointState = missionx::mx_point_state::defined;


#ifdef DEBUG_AREA
      Log::logMsg(std::string("[Area calc center] lat:") + mxUtils::formatNumber<double>(p.getLat()) + ", lon: " + mxUtils::formatNumber<double>(p.getLon())); // debug
#endif


    } // end if Point is empty

    return p;
  }
  // end calculateCenterOfShape


  // ---------------------------------------
  static bool collinear(Point a, Point b, Point c) // on same plane
  {
    return ccw(a, b, c) == 0;
  }

  // ---------------------------------------
  std::string to_string()
  {
    int         counter = 1;
    std::string format;
    format.clear();
    format = "\nPoints defined: " + ((pointsState == mx_point_state::defined) ? mxconst::get_MX_YES() : mxconst::get_MX_NO()) + ", List of stored points:" + mxconst::get_UNIX_EOL();
    for (auto p : deqPoints)
    {
      format += "point " + Utils::formatNumber<int>(counter) + ": " + p.to_string_xy() + mxconst::get_UNIX_EOL();
      counter++;
    }

    format += "\n[Center] " + pCenter.to_string() + mxconst::get_UNIX_EOL();

    return format;
  }

  // ---------------------------------------
  std::string to_string_ui_info()
  {
    int         counter = 1;
    std::string format;
    format.clear();
    format = "\nPoints defined: " + ((pointsState == mx_point_state::defined) ? mxconst::get_MX_YES() : mxconst::get_MX_NO()) + ", List of stored points:" + "\n";
    for (auto p : deqPoints)
    {
      format += "\tpoint " + Utils::formatNumber<int>(counter) + ": " + p.to_string_xy() + "\n";
      counter++;
    }

    return format;
  }
};

}

#endif
