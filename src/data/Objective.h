#ifndef OBJECTIVE_H_
#define OBJECTIVE_H_
#pragma once

#include "Task.h"
#include <vector>

using namespace missionx;

namespace missionx
{

class Objective : public missionx::mx_base_node //: public mxProperties
{
public:

  Objective();
  ~Objective();

  // core attributes
  bool isValid;
  bool isMandatory;
  bool isComplete;
  bool hasTaskWithAlwaysEvalProperty; // this should not be stored in save game, but re-evaluate using: "init_doesObjectiveHasAlwaysEvalTask()" function

  // errors
  std::string errReason;
  std::vector <std::string> vecErrors; // v3.305.3

  // core containers
  std::map<std::string, missionx::Task> mapTasks;
  std::unordered_set<std::string>       vecNotDependentTasks;
  std::unordered_set<std::string>       vecMandatoryTasks;
  std::vector<std::string>              vecSentences; // v3.0.148 display in ui

  // member
  void init();
//  void flc(std::string& inGoalName);
  bool init_doesObjectiveHasAlwaysEvalTask();
  // returns true if has at least one task that is independent.
  bool initTaskDependencyInformation();
  bool parse_node();           // v3.0.241.1
  void prepareCueMetaData(){}; // v3.0.241.1 dummy function so read_mission_file::load_saved_elements() tempalte will work

  // checkpoint
  void storeCoreAttribAsProperties();
  void saveCheckpoint(IXMLNode& inParent);
  void applyPropertiesToLocal();

  // debug
  std::string to_string(); // debug
};

} // end missionx

#endif
