#ifndef WAYPOINT_H_
#define WAYPOINT_H_
#pragma once

#include "../ui/gl/CueInfo.h"
#include "../core/mx_base_node.h"
#include "../core/Timer.hpp"
#include "mxProperties.hpp"

#include <list>
#include <unordered_map>

using namespace missionx;

namespace missionx
{
  namespace enums
  {
    typedef enum class _flightLegState_enum
      : uint8_t
    {
      leg_undefined,
      leg_success,
      leg_success_do_post, // v3.305.1
      leg_is_ready_for_next_flight_leg // v3.305.1
    } mx_flightLeg_state;

  } // enums namespace

class Waypoint : public missionx::mx_base_node //: public mxProperties
{
private:
  bool isValid{};
  // core attributes
  bool isComplete{}; // isComplete means we do not need to evaluate. It does not mean success
  bool isCleanFromRunningMessages{}; // v3.305.1 true: if there are still messages in the QMM false if there are none. False is good since we can continue to next flight leg
  missionx::enums::mx_flightLeg_state goal_state;

  std::string errMsg{ "" };

public:

  // instance info
  std::map<std::string, missionx::mxProperties> map3dInstance_prop; // v3.0.200 //instance name, obj3d specific attributes to apply on templates
  std::map<std::string, IXMLNode> map2DMapsNodes; // v3.0.242.7.1
  Waypoint();
  ~Waypoint();

  // XML Node
  IXMLNode            xGPS{ IXMLNode().emptyIXMLNode }; // v3.0.253.7 holds pointer to the LEG GPS element
  IXMLNode            xmlSpecialDirectives_ptr{ IXMLNode().emptyIXMLNode };
  std::list<IXMLNode> list_raw_dynamic_messages_in_leg;

  // core attributes
  bool hasMandatory{}; // internal flag that means: "one of the objectives in the goal has mandatory task". Without mandatory flag, goal will always be flagged as success.
  bool isFirstTime;  // v3.0.207.5 flag the goal as new one, so we need to call "pre_goal_script" and "start_message" // only Mission::flc() and Mission::START_MISSION() functions can change the status to true and it is reset again in
                     // Mission::flc_goal() to false.

  // core members
  bool parse_node();           // v3.0.241.1
  void prepareCueMetaData(){}; // v3.0.241.1 dummy function so read_mission_file::load_saved_elements() tempalte will work


  bool               getIsDummyLeg(); // v3.303.12
  bool               getIsComplete();
  bool               getIsFreeFromRunningMessages(); // v3.305.1
  void               setIsFreeFromRunningMessages(bool inVal); // v3.305.1


  missionx::enums::mx_flightLeg_state getFlightLegState();
  // we will use the mapProperties to hold the list of instances designer consider to display in this goal
  // Key: reflect the Instance Name, Value: a string "3d object" name
  //mxProperties list_displayInstances; // v3.0.200 holds "instance name" and "3d object template name". The instance will be removed from map once it it displayed
  std::unordered_map<std::string, std::string> list_displayInstances; // v3.0.200 holds "instance name" and "3d object template name". The instance will be removed from map once it it displayed

  // member
  void setIsComplete(bool inIsComplete, missionx::enums::mx_flightLeg_state inState);


  // core containers
  std::list<std::string>   listObjectivesInFlightLeg;     // key = value = objective name

  // trigers related lists
  std::list<std::string>   listTriggers;                  // key = value = trigger name
  std::list<std::string>   listTriggersByDistance;        // key = value = trigger name of all triggers we can evaluate their distance like rad/poly
  std::list<std::string>   listTriggersOthers;            // key = value = trigger name of all triggers that we can't evaluate their distance, like scripts
  std::list<std::string>   listTriggersByDisatnce_thread; // key = value = trigger name

  std::list<IXMLNode>      xml_listObjectivesInGoal_ptr;  // key = value = objective name
  std::list<IXMLNode>      xml_listTriggers;              // key = value = trigger name
  std::vector<std::string> vecSentences;                  // v3.0.148 display in ui

  std::map<std::string, IXMLNode> mapFlightLeg_sub_nodes_ptr; // v3.0.241.1 I hope not to use this map and stick to the <leg> element

  void        init();
  std::string to_string();
  std::string to_string_ui_leg_info();

  // CueInfo
  bool                         flag_cue_was_calculated;
  std::list<missionx::CueInfo> listCueInfoToDisplayInFlightLeg; // v3.0.203

  // checkpoint
  void storeCoreAttribAsProperties();
  void saveCheckpoint(IXMLNode& inParent);
  void applyPropertiesToGoal();

  // commands
  std::list<std::string> listCommandsAtTheStartOfFlightLeg;
  std::list<std::string> listCommandsAtTheEndOfTheFlightLeg;

  std::map<std::string, missionx::Timer> mapFailTimers; // v3.0.253.7 will store any <timer> information in Leg level, will be initialized once leg starts
};

} // missionx

#endif
