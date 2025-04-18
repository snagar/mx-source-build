#ifndef TASK_H_
#define TASK_H_
#pragma once
#include "../core/Timer.hpp"
#include "../core/mx_base_node.h" // v3.303.11
//#include "../core/embeded_script/script_manager.h"
#include "../logic/BindCommand.h" // v3.0.221.10
#include "mxProperties.hpp"
#include "../ui/gl/CueInfo.h" // v3.0.301

using namespace missionx;

namespace missionx
{

typedef enum class _task_state
  : uint8_t
{
  need_evaluation,
  success,
  was_success,
  failed // v3.0.221.9
} mx_task_state;



typedef enum class _task_type
  : uint8_t
{
  undefined_task,
  trigger,
  script,
  base_on_command,        // v3.0.221.10
  base_on_external_plugin, // v3.0.221.9
  base_on_sling_load_plugin, // v3.0.303
  placeholder // v3.0.303.7
} mx_task_type;



// Task will hold all the relevant properties in the mapProperties
class Task : public mx_base_node// : public mxProperties
{
private:
  CueInfo cue; // v3.0.303

public:

  Task();
  //~Task();

  // core attributes
  mx_task_type    task_type;
  mx_task_state   taskState;
  missionx::Timer timer;

  bool hasBeenEvaluated; // no need to store in savepoint
  bool bScriptCondMet;   // not sure we need to store it in savepoint used with Timer. We do not want the script to directly modify task state as "complete" so timer would be able to evaluate itself for the duration of the timer.

  bool isMandatory;            // a mandatory task is part of success or failure of the objective and mission.
  bool isComplete;             // task ended.
  bool bForceEvaluationOfTask; // Always evaluate "task" even if it is flagged: "complete". // mapped to property ATTRIB_FORCE_EVALUATION
  bool bIsEnabled;             // governed by plug-in, but can be altered by code(?). // If linked task depends on other task and other task is not complete then it is disabled.
  bool bAllConditionsAreMet;   // v3.0.303

  std::string           errReason;        // If task is undefined, check err reason
  std::string           action_code_name; // holds the trigger/script name. Initialized from attribs: "ATTRIB_BASE_ON_TRIGGER" and "ATTRIB_BASE_ON_SCRIPT"
  missionx::BindCommand command_ref;      // v3.0.221.10 will hold command path and reference if valid

  std::unordered_set<std::string> vecDepOnMe;                       // store tasks name dependent on this task
  std::string                     listOfIncompleteDependentTasks_s; // for monitoring and debug purposes


  // members
  void flc();
  bool isActive(); // v3.0.200 - used in display 3D Object
  bool parse_node();
  bool validate_sling(IXMLNode& inNode); // v3.0.303 

  // Set core values after reading properties.
  void applyPropertiesToLocal();

  // simple format of task properties
  static std::string translateTaskState(mx_task_state inState);
  static std::string translateTaskType(mx_task_type inTaskType);
  std::string        to_string();
  std::string        to_string_ui(); // v3.305.2

  // checkpoint
  void storeCoreAttribAsProperties();

  void setIsTaskComplete(bool inVal)
  {
    this->isComplete = inVal;

    // v3.0.241.1 update the xml node
    this->setNodeProperty<bool>(mxconst::get_PROP_IS_COMPLETE(), inVal); 
  }

  void setTaskState(missionx::mx_task_state inVal)
  {
    this->taskState = inVal;

    // v3.0.241.1 update the xml node
    this->setNodeProperty<int>(mxconst::get_PROP_TASK_STATE(), (int)inVal); 
  }

  void set_bIsEnabled(bool inVal)
  {
    this->bIsEnabled = inVal;

    // v3.0.241.1 update the xml node
    this->setNodeProperty<bool>(mxconst::get_ATTRIB_ENABLED(), inVal);
  }

  void set_bForceEvaluationOfTask(bool inVal)
  {
    this->bForceEvaluationOfTask = inVal;

    // v3.0.241.1 update the xml node
    this->setNodeProperty<bool>(mxconst::get_ATTRIB_FORCE_EVALUATION(), inVal);
  }

  // The function will return a mxProperties that will hold the key/value to seed to script
  missionx::mxProperties getTaskInfoToSeed();

  // v3.0.303
  CueInfo  getCopyOfCueInfo() { return cue; };
  CueInfo& getCueInfo_ptr() { return cue; };
  void     prepareCueMetaData(); 
  void     reset(); // v25.02.1
  void     set_success(); // v25.02.1

  mx_cue_types                cueType;
  std::deque<missionx::Point> deqPoints;
  Point                       pSlingStart, pSlingEnd;
};

}

#endif
