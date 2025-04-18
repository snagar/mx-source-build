#ifndef ENDMISSION_H_
#define ENDMISSION_H_

#pragma once

#include "../core/mx_base_node.h"

namespace missionx
{

class EndMission : missionx::mx_base_node // : public mxProperties
{
public:

  EndMission() { init(); }


  void init() {}

  std::string to_string()
  {

    Log::printHeaderToLog("End Mission Information", false, missionx::format_type::header);
    return Utils::xml_get_node_content_as_text(this->node) + "\n";
  }
};

}
#endif
