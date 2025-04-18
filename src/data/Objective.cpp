#include "Objective.h"



missionx::Objective::Objective()
{
  init();
}


missionx::Objective::~Objective() {}

// -------------------------------------------

void
missionx::Objective::init()
{
  isValid     = true;
  isMandatory = false;
  isComplete  = false;
  mapTasks.clear();
  vecNotDependentTasks.clear();
  vecMandatoryTasks.clear();
  vecSentences.clear();
  errReason.clear();
  vecErrors.clear();
}

// -------------------------------------------

//void
//missionx::Objective::flc(std::string& inGoalName)
//{
//  // 1. loop over all tasks
//  // 2. If a Task is dependent on other task, then get name recursively and execute Task.flc
//}

// -------------------------------------------

bool
missionx::Objective::init_doesObjectiveHasAlwaysEvalTask()
{
  bool               found  = false;
  bool               result = false;
  static std::string err;
  err.clear();

  for (auto iter : this->mapTasks)
  {
    result = Utils::readBoolAttrib(this->mapTasks[iter.first].node, mxconst::get_ATTRIB_FORCE_EVALUATION(), false);
    if (result)
    {
      found = true;
      break;
    }
  }

  this->hasTaskWithAlwaysEvalProperty = found;
  this->setNodeProperty<bool>(mxconst::get_PROP_HAS_ALWAYS_TASK(), found);  // v3.0.241.1
  return found;
}

// -------------------------------------------

bool
missionx::Objective::initTaskDependencyInformation()
{
  std::string err;

  err.clear();

  // objective specific reset attributes
  this->isMandatory = false;
  this->vecMandatoryTasks.clear();    // construct from scratch
  this->vecNotDependentTasks.clear(); // construct from scratch

  // loop over ALL tasks
  for (auto iter : this->mapTasks)
  {
    std::string taskName = iter.first;

    // check Task is defined
    if (mapTasks[taskName].task_type == missionx::mx_task_type::undefined_task)
      continue;

    // check if tested Task is mandatory
    if (mapTasks[taskName].isMandatory)
    {
      this->vecMandatoryTasks.insert(taskName); // add to mandatory tasks
    }


    // check if have dependencies
    const std::string dependsOnTask = Utils::readAttrib(mapTasks[taskName].node, mxconst::get_ATTRIB_DEPENDS_ON_TASK(), ""); // v3.0.241.1
    if (dependsOnTask.compare(taskName) == 0)
    {
      mapTasks[taskName].setNodeStringProperty(mxconst::get_ATTRIB_DEPENDS_ON_TASK(), ""); // v3.0.241.1
      vecErrors.emplace_back("Task: " + taskName + ", Has dependency with same name."); // v3.305.3

      Log::logMsg("[AUTOFIX]Found Task dependency with same name as task name: " + mxconst::get_QM() + taskName + mxconst::get_QM() + " clearing dependency. Check and fix mission file.", format_type::attention, false);
    }


    if (dependsOnTask.empty())
    {
      this->vecNotDependentTasks.insert(taskName);

      // continue;
    }
    else
    {
      // check if "dependsOnTask" is in the Objectives list
      if (!Utils::isElementExists(mapTasks, dependsOnTask))
      {
        mapTasks[taskName].task_type = missionx::mx_task_type::undefined_task;
        mapTasks[taskName].errReason = "Failed to find a dependent task by the name: " + mxconst::get_QM() + taskName + mxconst::get_QM() + ". flag task as invalid. Check mission data file and fix settings";
        vecErrors.emplace_back(this->errReason); // v3.305.3

        mapTasks[taskName].setNodeProperty<int>(mxconst::get_ATTRIB_TYPE(), (int)missionx::mx_task_type::undefined_task);  // v3.0.241.1
        Log::logMsgErr(mapTasks[taskName].errReason);
      }
      else if (mapTasks[dependsOnTask].task_type == mx_task_type::undefined_task) // check parent is valid. Invalidate if not.
      {
        mapTasks[taskName].task_type = mx_task_type::undefined_task;
        mapTasks[taskName].errReason = "Depends on a task which was flagged: undefined, meaning invalid. Parent task Name: " + dependsOnTask + mxconst::get_UNIX_EOL();
        vecErrors.emplace_back(this->errReason); // v3.305.3

        mapTasks[taskName].setNodeProperty<int>(mxconst::get_ATTRIB_TYPE(), (int)missionx::mx_task_type::undefined_task);  // v3.0.241.1

        Log::logMsg(mapTasks[taskName].errReason, format_type::attention, false);
      }
      else // found dependent and adding current task name to parent
        this->mapTasks[dependsOnTask].vecDepOnMe.insert(taskName);
    }

  } // end loop over tasks



  if (!this->vecMandatoryTasks.empty())
  {
    this->isMandatory = true;
    this->setNodeProperty<bool>(mxconst::get_ATTRIB_MANDATORY(), true);  // v3.0.241.1
  }

  this->init_doesObjectiveHasAlwaysEvalTask(); // initialize hasTaskWithAlwaysEvalProperty

  if (this->vecNotDependentTasks.empty())
  {
    this->isValid   = false; // invalidate objective
    this->errReason = "All tasks has dependent property. Must have one task independent. Invalidating Objective. Fix mission.";
    vecErrors.emplace_back(this->errReason); // v3.305.3

    this->setNodeProperty<bool>(mxconst::get_PROP_IS_VALID(), this->isValid); // v3.0.241.1

    Log::logMsgErr(this->errReason);
  }

  this->storeCoreAttribAsProperties();

  return this->isValid;
} // initTaskDependencyInformation

// -------------------------------------------

bool
missionx::Objective::parse_node()
{
  assert(!this->node.isEmpty()); // v3.0.241.1
  this->vecErrors.clear(); // v3.305.3

  std::string objName  = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");  // read objective name
  std::string objTitle = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TITLE(), ""); 


  // Validations
  if (objName.empty())
    return false;

  this->name = objName; // v3.303.11
  this->setStringProperty(mxconst::get_ATTRIB_NAME(), objName);   // set Name
  this->setStringProperty(mxconst::get_ATTRIB_TITLE(), objTitle); // read title


  // Read sub elements Tasks
  int nChilds2 = this->node.nChildNode(mxconst::get_ELEMENT_TASK().c_str()); // count how many TASK elements
  if (nChilds2 > 0)
  {
    std::string err;
    for (int i1 = 0; i1 < nChilds2; i1++)
    {
      err.clear();

      IXMLNode xTask = this->node.getChildNode(mxconst::get_ELEMENT_TASK().c_str(), i1);
      if (xTask.isEmpty()) // skip if empty element
        continue;

      missionx::Task task;

      task.node = xTask;
      if (task.parse_node())
      {
        task.prepareCueMetaData(); // v3.0.303
        if (Utils::addElementToMap(this->mapTasks, task.name, task, err)) // v3.303.11 changed reading name directly from task
        {
          // debug output
          const std::string s = task.to_string();
          Log::logMsg(s, format_type::none, false);
        }
        else
        {
          const std::string errorMessage = "[Task]Fail to add task: " + Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "") + ". Reason: " + err + "\n";
          vecErrors.emplace_back(errorMessage);
          Log::add_missionLoadError(errorMessage);
          continue;
        }
      }
      else
        continue; // skip that task


    } // end loop over task elements

  } // end if there are task elements
  else
  {
    Log::logMsgErr("[" + std::string(__func__) + "] No tasks in objective: " + objName + ", objective is invalid.");
    return false;
  }


  return true;
}

// -------------------------------------------

std::string
missionx::Objective::to_string()
{
  std::string format;
  format.clear();

  format = "Properties for Objective \"" + this->name + "\":" + mxconst::get_UNIX_EOL();

  size_t length = format.length();
  format += std::string("").append(length, '=') + mxconst::get_UNIX_EOL();

  format += Utils::xml_get_node_content_as_text(this->node); 

  format += "=== Tasks ===\n";
  for (auto iter : this->mapTasks)
  {
    Task t = iter.second;
    format += t.to_string();
  }
  format += "=== End Tasks ===" + mxconst::get_UNIX_EOL();


  return format;
}

// -------------------------------------------

void
missionx::Objective::storeCoreAttribAsProperties()
{
  this->setNodeProperty<bool>(mxconst::get_PROP_IS_VALID(), this->isValid);                               // v3.0.241.1
  this->setNodeProperty<bool>(mxconst::get_ATTRIB_MANDATORY(), this->isMandatory);                        // v3.0.241.1
  this->setNodeProperty<bool>(mxconst::get_PROP_IS_COMPLETE(), this->isComplete);                         // v3.0.241.1
  this->setNodeProperty<bool>(mxconst::get_PROP_HAS_ALWAYS_TASK(), this->hasTaskWithAlwaysEvalProperty);  // v3.0.241.1


}
// -------------------------------------------

void
missionx::Objective::saveCheckpoint(IXMLNode& inParent)
{
  storeCoreAttribAsProperties();

  IXMLNode xObjective = this->node.deepCopy();

  inParent.addChild(xObjective);

}

// -------------------------------------------

void
missionx::Objective::applyPropertiesToLocal()
{

  this->isValid     = Utils::readBoolAttrib(this->node, mxconst::get_PROP_IS_VALID(), false);
  this->isMandatory = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_MANDATORY(), false);
  this->isComplete  = Utils::readBoolAttrib(this->node, mxconst::get_PROP_IS_COMPLETE(), false);

  init_doesObjectiveHasAlwaysEvalTask();
  this->initTaskDependencyInformation();
}

// -------------------------------------------
// -------------------------------------------
// -------------------------------------------
// -------------------------------------------
