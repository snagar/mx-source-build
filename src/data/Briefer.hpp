#ifndef BRIEFER_H_
#define BRIEFER_H_

#pragma once
#include "../core/coordinate/Point.hpp"
#include "../io/Log.hpp"
#include "../core/mx_base_node.h"

//#include "../core/coordinate/UtilsGraph.hpp" // v3.303.14 removed


namespace missionx
{

class Briefer : public missionx::mx_base_node 
{
private:
public:
  IXMLNode xLocationAdjust;
  bool     has_valid_location_adjust;

  // for position
  static bool need_to_pause_xplane;

  Briefer();

  void init();


  void clear() { init(); }

  // v3.0.241.1
  bool parse_node();


  std::string to_string()
  {
    Log::printHeaderToLog("Reading <briefer> element", false, format_type::header);
    return Utils::xml_get_node_content_as_text(this->node) + "\nNeed location adjust: " + ((this->has_valid_location_adjust) ? mxconst::get_MX_YES() : mxconst::get_MX_NO());
  }

  void positionPlane(const bool inflag_setupForcePlanePositioning = false,const bool inflag_setupChangeHeadingEvenIfPlaneIn_20meter_radius = false, const int inXplaneVersion_i = 0);
  void setPlaneHeading(const float & inHeading);
  void positionPlane_v10(missionx::Point& inNewPosition, const bool inForceHeading_b);

  // -------------------------------------------

  void postPositionPlane() {}
};


} // missionx

#endif
