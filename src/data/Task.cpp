#include "Task.h"



missionx::Task::Task()
{
  this->cueType = mx_cue_types::cue_none;

  task_type = mx_task_type::undefined_task;
  taskState = mx_task_state::need_evaluation;

  action_code_name.clear();
  isMandatory = false;
  isComplete  = false;
  bForceEvaluationOfTask = false;

  // linked task attributes
  bIsEnabled       = true;
  hasBeenEvaluated = false;
  bScriptCondMet   = false;
  // end linked task

  // others
  bAllConditionsAreMet = true; // v3.0.303

  this->errReason.clear();
  vecDepOnMe.clear();
  listOfIncompleteDependentTasks_s.clear();

  pSlingStart.init();
  pSlingEnd.init();

  setStringProperty (mxconst::get_ATTRIB_NAME (), "");
  setStringProperty (mxconst::get_ATTRIB_TITLE (), "");
  setStringProperty (mxconst::get_ATTRIB_BASE_ON_TRIGGER (), "");
  setStringProperty (mxconst::get_ATTRIB_BASE_ON_SCRIPT (), "");
  setNumberProperty<bool> (mxconst::get_ATTRIB_MANDATORY (), isMandatory);
  setNumberProperty<bool> (mxconst::get_PROP_IS_COMPLETE (), isComplete);
}


//missionx::Task::~Task() {}


void
missionx::Task::flc()
{
}


bool
missionx::Task::isActive()
{
  // check the following sets: enabled && has been evaluated at least once & not complete.
  // OR check if "always need evaluation" and "is enabled" and has been evaluated at least once.
  return ((this->bIsEnabled && this->hasBeenEvaluated && !this->isComplete) || (this->bForceEvaluationOfTask && this->bIsEnabled && this->hasBeenEvaluated));
}

bool
missionx::Task::parse_node()
{
  assert(!this->node.isEmpty());

  std::string taskName                        = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");
  std::string base_on_trigger                 = Utils::readAttrib(this->node, mxconst::get_ATTRIB_BASE_ON_TRIGGER(), "");
  std::string base_on_script                  = Utils::readAttrib(this->node, mxconst::get_ATTRIB_BASE_ON_SCRIPT(), "");
  std::string base_on_command                 = Utils::readAttrib(this->node, mxconst::get_ATTRIB_BASE_ON_COMMAND(), "");
  std::string base_on_sling                   = Utils::readAttrib(this->node, mxconst::get_ATTRIB_BASE_ON_SLING_LOAD(), "");
  bool        b_isPlaceholder                 = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_IS_PLACEHOLDER(), false);

  std::string taskTitle                       = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TITLE(), "");
  std::string taskEvalSuccessForN_sec         = Utils::readAttrib(this->node, mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), "0");
  std::string force_evaluation                = Utils::readAttrib(this->node, mxconst::get_ATTRIB_FORCE_EVALUATION(), mxconst::get_MX_NO());
  std::string taskDepend                      = Utils::readAttrib(this->node, mxconst::get_ATTRIB_DEPENDS_ON_TASK(), "");
  std::string taskMandatory                   = Utils::readAttrib(this->node, mxconst::get_ATTRIB_MANDATORY(), mxconst::get_MX_NO());
  std::string task_use_xxshared_target_status = Utils::readAttrib(this->node, mxconst::get_ATTRIB_BASE_ON_EXTERNAL_PLUGIN(), mxconst::get_MX_NO()); // v3.0.221.9 boolean value
  std::string task_cumulative                 = Utils::readAttrib(this->node, mxconst::get_ATTRIB_CUMULATIVE_TIMER_FLAG(), mxconst::get_MX_NO());   // v3.0.221.11 boolean
  std::string task_enabled                    = Utils::readAttrib(this->node, mxconst::get_ATTRIB_ENABLED(), mxconst::get_MX_YES());                // v3.0.221.15rc4 boolean

  std::string taskDesc = (this->node.nClear() > 0) ? this->node.getClear().sValue : ""; // description of task sa follow: <task ...><![CDATA[task description]]></task>. // NO <desc> element


  // Validations
  if (taskName.empty())
    return false;

  this->name = taskName;

  if (base_on_script.empty() && base_on_trigger.empty() && base_on_command.empty() && base_on_sling.empty() && b_isPlaceholder == false )
  {
    Log::add_missionLoadError("Task: " + taskName + " has no \"base\" attribute set correctly. Please fix.");
    return false;
  }

  // set attributes
  this->setStringProperty(mxconst::get_ATTRIB_NAME(), taskName);
  // set "base_on_trigger" or "base_on_script" but not both since task will be evaluated only against one of those.
  if (!base_on_trigger.empty())
    this->setStringProperty(mxconst::get_ATTRIB_BASE_ON_TRIGGER(), base_on_trigger);
  else if (!base_on_script.empty())
    this->setStringProperty(mxconst::get_ATTRIB_BASE_ON_SCRIPT(), base_on_script);
  else if (!base_on_command.empty()) // v3.0.221.10 // v3.0.303 added else if
    this->setStringProperty(mxconst::get_ATTRIB_BASE_ON_COMMAND(), base_on_command);
  else if (!base_on_sling.empty()) // v3.0.303 added sling load support
    this->setStringProperty(mxconst::get_ATTRIB_BASE_ON_SLING_LOAD(), base_on_sling);

  // v3.0.303 validate sling load
  if ( !base_on_sling.empty() )
  {
    auto slingNode = this->node.getChildNode(base_on_sling.c_str());
    if ( ! validate_sling(slingNode) )
    {
      Log::add_missionLoadError("Task: " + taskName + " has wrong \"" + base_on_sling + "\" element. Please check manadatory attributes and fix the sub node of that task.");
      return false;    
    }
  }


  // fix if depend on itself
  if (taskDepend.compare(taskName) == 0)
  {
    Utils::xml_search_and_set_attribute_in_IXMLNode(this->node, mxconst::get_ATTRIB_DEPENDS_ON_TASK(), "", mxconst::get_ELEMENT_TASK());

    taskDepend.clear();
    Log::logMsg("[AUTOFIX]Found Task dependency with same name as task name: " + mxconst::get_QM() + taskName + mxconst::get_QM() + " clearing dependency. Check and fix mission file.");
  }

  this->setStringProperty(mxconst::get_ATTRIB_TITLE(), taskTitle);
  this->setStringProperty(mxconst::get_ELEMENT_DESC(), taskDesc);             // description of task as follow: <task ...><![CDATA[task description]]></task>. // NO <desc> element
  this->setStringProperty(mxconst::get_ATTRIB_DEPENDS_ON_TASK(), taskDepend); // is task dependent on other task ?
  this->setNumberProperty(mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), taskEvalSuccessForN_sec);
  this->setBoolProperty(mxconst::get_ATTRIB_FORCE_EVALUATION(), force_evaluation);
  this->setBoolProperty(mxconst::get_ATTRIB_MANDATORY(), taskMandatory);                                 // is task mandatory in current objective ?
  this->setBoolProperty(mxconst::get_ATTRIB_BASE_ON_EXTERNAL_PLUGIN(), task_use_xxshared_target_status); 
  this->setBoolProperty(mxconst::get_ATTRIB_CUMULATIVE_TIMER_FLAG(), task_cumulative);                   
  this->setBoolProperty(mxconst::get_ATTRIB_ENABLED(), task_enabled);                                    


  // Add to objective task map
  this->applyPropertiesToLocal(); // set task core attributes according to properties. TODO: Need to be executed at the end of all mission load


  return true;
}

bool
missionx::Task::validate_sling(IXMLNode& inNode)
{
  if (inNode.isEmpty())
  {

    Log::add_missionLoadError("No sub element was found for the sling load support.");
    return false;  
  }

  if (Utils::readAttrib(inNode, mxconst::get_ATTRIB_TYPE(), "").compare("cargo") == 0)
  {
    auto start_lat = Utils::readNumericAttrib(inNode, mxconst::get_ATTRIB_START_LAT(), 0.0);
    auto start_lon = Utils::readNumericAttrib(inNode, mxconst::get_ATTRIB_START_LON(), 0.0);
    auto end_lat = Utils::readNumericAttrib(inNode, mxconst::get_ATTRIB_END_LAT(), 0.0);
    auto end_lon = Utils::readNumericAttrib(inNode, mxconst::get_ATTRIB_END_LON(), 0.0);
    auto init_script_b = !(Utils::readAttrib(inNode, mxconst::get_ATTRIB_INIT_SCRIPT(), "").empty()); // return true if init_script is not empty
    auto cond_script_b = !(Utils::readAttrib(inNode, mxconst::get_ATTRIB_COND_SCRIPT(), "").empty()); // return true if cond_script is not empty

    if (start_lat * start_lon * end_lat * end_lon + (init_script_b && cond_script_b) == 0.0)
    {
      Log::add_missionLoadError("Not all of the mandatory cargo elements are set. Check start+end position.");
      return false;
    }
    else
    {
      // store relevant information in the parent node for easier access later on
      // TODO: copy all attributes to the parent node using Utils::xml_copy_node_attributes_excluding_black_list()
      const std::string parent_tag_name = inNode.getParentNode().getName();

      // v3.303.14 useing QT clang preference.
      auto parentNode = inNode.getParentNode();
      Utils::xml_set_attribute_in_node_asString(parentNode, mxconst::get_ATTRIB_TYPE(), Utils::readAttrib(inNode, mxconst::get_ATTRIB_TYPE(), ""), parent_tag_name);
      Utils::xml_set_attribute_in_node_asString(parentNode, mxconst::get_ATTRIB_START_LAT(), Utils::readAttrib(inNode, mxconst::get_ATTRIB_START_LAT(), ""), parent_tag_name);
      Utils::xml_set_attribute_in_node_asString(parentNode, mxconst::get_ATTRIB_START_LON(), Utils::readAttrib(inNode, mxconst::get_ATTRIB_START_LON(), ""), parent_tag_name);
      Utils::xml_set_attribute_in_node_asString(parentNode, mxconst::get_ATTRIB_END_LAT(), Utils::readAttrib(inNode, mxconst::get_ATTRIB_END_LAT(), ""), parent_tag_name);
      Utils::xml_set_attribute_in_node_asString(parentNode, mxconst::get_ATTRIB_END_LON(), Utils::readAttrib(inNode, mxconst::get_ATTRIB_END_LON(), ""), parent_tag_name);
      Utils::xml_set_attribute_in_node_asString(parentNode, mxconst::get_ATTRIB_WEIGHT_KG(), Utils::readAttrib(inNode, mxconst::get_ATTRIB_WEIGHT_KG(), ""), parent_tag_name);


      this->pSlingStart = Point(start_lat, start_lon);
      this->pSlingEnd   = Point(end_lat, end_lon);
      this->deqPoints.emplace_back(pSlingEnd);

      return true; // valid
    }
  }



  return false; // not valid
}


std::string
missionx::Task::translateTaskState(mx_task_state inState)
{
  return (inState == missionx::mx_task_state::success) ? "success" : (inState == missionx::mx_task_state::was_success) ? "was_success" : "need_evaluation";
}

std::string
missionx::Task::translateTaskType(mx_task_type inTaskType)
{
  switch (inTaskType)
  {
    case mx_task_type::trigger:
      return "trigger";
      break;
    case mx_task_type::script:
      return "script";
      break;
    case mx_task_type::base_on_command:
      return "command";
      break;
    case mx_task_type::base_on_external_plugin:
      return "external plugin";
      break;
    case mx_task_type::base_on_sling_load_plugin:
      return "sling load";
      break;
    case mx_task_type::placeholder:
      return "placeholder";
      break;
    default:
      return "undefined";
      break;
  } // end switch

  return "undefined";
}

// -------------------------------------

std::string
missionx::Task::to_string()
{
  std::string format = "[ Task: \"" + Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "") + "\", type: " + Task::translateTaskType(this->task_type) + ". Is Mandatory: " + ((isMandatory) ? mxconst::get_MX_TRUE() : mxconst::get_MX_FALSE()) + ". Properties: ]\n";
  size_t      length = format.length();
  format += std::string("").append(length, '=');
  format.append("\n");
  format.append(Utils::xml_get_node_content_as_text(this->node)); 

  return format;
}

// -------------------------------------

std::string
missionx::Task::to_string_ui()
{
  std::string taskInfo = "Task: \"" + this->getName() + "\", type: " + Task::translateTaskType(this->task_type) + "(" + ((this->getBoolValue(mxconst::get_ATTRIB_ENABLED(), true)) ? "enabled" : "disabled") + ")";

 return taskInfo;
}

// -------------------------------------

void
missionx::Task::applyPropertiesToLocal()
{
  assert(!this->node.isEmpty()); // v3.0.241.1

  errReason.clear();

  std::string errMsg, base_script, base_trig, base_command /*v3.0.221.10*/;
  errMsg.clear();
  base_script.clear();
  base_trig.clear();

  this->isMandatory = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_MANDATORY(), false);

  if (this->node.isAttributeSet(mxconst::get_PROP_IS_COMPLETE().c_str()))
    this->isComplete = Utils::readBoolAttrib(this->node, mxconst::get_PROP_IS_COMPLETE(), false);

  if (this->node.isAttributeSet(mxconst::get_ATTRIB_FORCE_EVALUATION().c_str()))
    this->bForceEvaluationOfTask = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_FORCE_EVALUATION(), false);

  if (this->node.isAttributeSet(mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str()))
    base_trig = Utils::readAttrib(this->node, mxconst::get_ATTRIB_BASE_ON_TRIGGER(), "");

  if (this->node.isAttributeSet(mxconst::get_ATTRIB_BASE_ON_SCRIPT().c_str()))
    base_script = Utils::readAttrib(this->node, mxconst::get_ATTRIB_BASE_ON_SCRIPT(), "");

  if (this->node.isAttributeSet(mxconst::get_ATTRIB_BASE_ON_COMMAND().c_str()))
    base_command = Utils::readAttrib(this->node, mxconst::get_ATTRIB_BASE_ON_COMMAND(), "");


  if (this->node.isAttributeSet(mxconst::get_ATTRIB_ENABLED().c_str()))
    this->bIsEnabled = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_ENABLED(), true);


  if (this->node.isAttributeSet(mxconst::get_PROP_TASK_STATE().c_str()))
    this->taskState = (missionx::mx_task_state)Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_PROP_TASK_STATE(), (int)missionx::mx_task_state::need_evaluation);

  if (this->node.isAttributeSet(mxconst::get_PROP_SCRIPT_COND_MET_B().c_str()))
    this->bScriptCondMet = Utils::readBoolAttrib(this->node, mxconst::get_PROP_SCRIPT_COND_MET_B(), false);



  if (Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_IS_PLACEHOLDER(), false)) // v3.0.303.7
  {
    this->task_type = mx_task_type::placeholder;
    this->action_code_name.clear();
  }
  else if (Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_BASE_ON_EXTERNAL_PLUGIN(), false))
  {
    this->task_type = mx_task_type::base_on_external_plugin;
    this->action_code_name.clear();
  }
  else
  {
    if (!base_trig.empty())
    {
      this->task_type        = mx_task_type::trigger;
      this->action_code_name = base_trig;
    }
    else if (!base_script.empty())
    {
      this->task_type        = mx_task_type::script;
      this->action_code_name = base_script;
    }
    else if (!base_command.empty()) // v3.0.221.10
    {
      this->task_type        = mx_task_type::base_on_command;
      this->action_code_name = base_command;
    }
    else if ( ! Utils::readAttrib(this->node, mxconst::get_ATTRIB_BASE_ON_SLING_LOAD(), "").empty() ) // v3.0.303
    {
      this->task_type        = mx_task_type::base_on_sling_load_plugin;
      this->action_code_name = mxUtils::stringToLower(  Utils::readAttrib(this->node, mxconst::get_ATTRIB_TYPE(), "") ); // the type holds "cargo" and in the future maybe "fire"
    }
    else
    {
      this->task_type = mx_task_type::undefined_task;
      // this->errReason = "Task: \"" + Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "") + "\", has not been set as " + mxconst::get_QM() + "base_trigger" + mxconst::get_QM() + " or " + mxconst::get_QM() + "base_code" + mxconst::get_QM() + "base_command" + mxconst::get_QM() + "base_external_plugin" + mxconst::get_QM() + "or base_on_sling_load_plugin" + mxconst::get_QM() + ". Task flagged as undefined/disabled.";
      this->errReason = fmt::format(R"(Task: "{}", has not been set as "base_trigger", "base_code", "base_command", "base_external_plugin" or "or base_on_sling_load_plugin".)", Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "") );
    }
  } // end if not flag_base_on_external_plugin
}

// -------------------------------------

void
missionx::Task::storeCoreAttribAsProperties()
{
  this->setNodeProperty<bool>(mxconst::get_ATTRIB_MANDATORY(), this->isMandatory);
  this->setNodeProperty<bool>(mxconst::get_PROP_IS_COMPLETE(), this->isComplete);
  this->setNodeProperty<bool>(mxconst::get_ATTRIB_FORCE_EVALUATION(), this->bForceEvaluationOfTask);
  this->setNodeProperty<bool>(mxconst::get_ATTRIB_ENABLED(), this->bIsEnabled);
  this->setNodeProperty<int>(mxconst::get_PROP_TASK_STATE(), (int)this->taskState);
}

// -------------------------------------

void
missionx::Task::prepareCueMetaData()
{
  if (this->task_type == missionx::mx_task_type::base_on_sling_load_plugin)
  {
    this->cue.node_ptr             = this->node;
    this->cue.cueType              = missionx::mx_cue_types::cue_sling_task;
    this->cue.deqPoints_ptr        = &this->deqPoints;
    this->cue.originName = "SlingLoad: " + Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");

    this->cue.setRadiusAsMeter(mxconst::SLING_LOAD_SUCCESS_RADIUS_MT); 
    this->cue.hasRadius = true;
  }
}

// -------------------------------------


void
missionx::Task::reset()
{
  this->bIsEnabled = true; // v25.03.1 make sure that the task will be evaluated in the next "flight loop back".
  this->bAllConditionsAreMet = false;
  this->setIsTaskComplete(false);
  this->setTaskState(missionx::mx_task_state::need_evaluation);
}

// -------------------------------------

void
Task::set_success()
{
  this->bIsEnabled = false; // v25.03.1 make sure that the task won't be re-evaluated in the next "flight loop back".
  this->bAllConditionsAreMet = true;
  this->setIsTaskComplete(true);
  this->setTaskState(missionx::mx_task_state::success);
}

// -------------------------------------

missionx::mxProperties
missionx::Task::getTaskInfoToSeed()
{
  missionx::mxProperties seedProperties;
  seedProperties.clear();

  // the key will reflect the name to seed
  // (mxTaskState=success/was_success/need_evaluation_),(mxTaskType=trigger/script/undefined), (mxTaskActionName=trigger/script name), (mx_is_complete=true|false), (mx_enabled=true|false),
  // (mx_script_conditions_met_b=true|false), (mxTaskHasBeenEvaluate=true|false), (mx_always_evaluate=true|false), (mx_mandatory=true|false)

  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_EXT_mxTaskActionName(), this->action_code_name);
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_EXT_mxState(), Task::translateTaskState(this->taskState));
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_EXT_mxType(), Task::translateTaskType(this->task_type));
  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_PROP_IS_COMPLETE(), this->isComplete);
  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_ENABLED(), this->bIsEnabled);
  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_PROP_SCRIPT_COND_MET_B(), this->bScriptCondMet);
  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_EXT_mxTaskHasBeenEvaluated(), this->hasBeenEvaluated);
  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_FORCE_EVALUATION(), this->bForceEvaluationOfTask);
  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_MANDATORY(), this->isMandatory);

  return seedProperties;
}


