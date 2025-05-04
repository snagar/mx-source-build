//#include <format>
#include "read_mission_file.h"


#include "ListDir.h"
#include "../core/QueueMessageManager.h"
//#include "../core/dataref_manager.h"

namespace missionx
{
bool                     read_mission_file::initOK;
std::string              read_mission_file::errMsg;
std::vector<std::string> read_mission_file::vecErrors;

IXMLNode     read_mission_file::dummy_top_node;
IXMLRenderer read_mission_file::xmlWriter;
}


missionx::read_mission_file::read_mission_file()
{
  initOK = true;
  errMsg.clear();
  vecErrors.clear();
}

// -----------------------------------

void
missionx::read_mission_file::addError(std::string inError)
{
  data_manager::errStr = inError;
  if (!Utils::trim(inError).empty())
    Log::add_missionLoadError(inError); // v3.0.241.1                             
}


// -----------------------------------


void
missionx::read_mission_file::readGlobalSettings ( const ITCXMLNode & xParent) //, std::string& inPathToRootMissionFolder)
{
  /* read GlobalSettings - One time implementation */
  //Log::printHeaderToLog("Reading <" + mxconst::get_GLOBAL_SETTINGS() + "> Info");

  data_manager::mx_global_settings.node = xParent.getChildNode(mxconst::get_GLOBAL_SETTINGS().c_str()).deepCopy(); 
  if (data_manager::mx_global_settings.node.isEmpty())
    data_manager::mx_global_settings.node = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_GLOBAL_SETTINGS());


  if (!data_manager::mx_global_settings.node.isEmpty())
  {
    data_manager::mx_global_settings.parse_node();

    data_manager::prepare_new_mission_folders(data_manager::mx_global_settings); // v3.0.241.1

    Log::logMsg(fmt::format("Loaded: {:.<20}\n{:=<40}", mxconst::get_GLOBAL_SETTINGS(), "")); // v3.305.3

    // needed when starting mission automatically, the "opt_forceInventoryLayoutBasedOnVersion_i" might not be initialized correctly, based on mission file.
    // It will cause the "Inventory::parse_node()" to wrongly treat the "inventory layout", in some cases.
    // This initialization should help solving this issue.
    // missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i = missionx::data_manager::get_inv_layout_based_on_mission_ver_and_compatibility_node(); // v24.12.2
    missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i = missionx::data_manager::get_inv_layout_based_on_mission_ver_and_compatibility_node(missionx::data_manager::mission_file_supported_versions, missionx::data_manager::mx_global_settings.xCompatibility_ptr, data_manager::flag_setupUseXP11InventoryUI); // v25.03.1

  } // end global settings

}
// end readGlobalSettings()

// -----------------------------------

void
missionx::read_mission_file::readBriefer(ITCXMLNode& xParent)
{

  data_manager::briefer.node = xParent.getChildNode(mxconst::get_ELEMENT_BRIEFER().c_str()).deepCopy();
  if (data_manager::briefer.node.isEmpty())
  {
    read_mission_file::initOK = false;
    addError("No Briefer Information was found. Fix Mission data.");
    return;
  }
  else
  {
    read_mission_file::initOK = data_manager::briefer.parse_node();

    //// read datarefs_start_cold_and_dark ////
    if (read_mission_file::initOK)
    {
      auto xDatarefStartCold = xParent.getChildNode(mxconst::get_ELEMENT_DATAREFS_START_COLD_AND_DARK().c_str()); // pick ELEMENT_DATAREFS_START_COLD_AND_DARK from parent node
      if (!xDatarefStartCold.isEmpty())
      {
        data_manager::briefer.node.addChild(xDatarefStartCold.deepCopy()); // v3.0.241.2

        std::string textValue = Utils::xml_get_text(xDatarefStartCold); // xDatarefStartCold.getText();
        if (!textValue.empty())
          data_manager::briefer.setStringProperty(mxconst::get_ELEMENT_DATAREFS_START_COLD_AND_DARK(), textValue);
      }
    }
  }


}

// -----------------------------------

// Send the parent node + mapScript + script folder. The function should be used when loading a new mission or saved mission
void
missionx::read_mission_file::readScriptElement(ITCXMLNode& xParent, std::map<std::string, missionx::base_script>& inMapScripts, std::string& inScriptFolder)
{
  // v3.0.241.7 xParent = xScripts = <scripts> element instead of <MISSION>
  Log::printHeaderToLog("Reading Embedded Scripts", false);

  if (!xParent.isEmpty())
  {
    ITCXMLNode  xScriptFile;
    ITCXMLNode  xScriptlet;
    ITCXMLNode  xIncludeFile;
    ITCXMLNode  xSharedVariables;
    std::string script_path;
    int iElements = xParent.nChildNode(mxconst::get_ELEMENT_FILE().c_str());

    for (int i1 = 0; i1 < iElements; i1++)
    {
      script_path.clear();

      xScriptFile = xParent.getChildNode(mxconst::get_ELEMENT_FILE().c_str(), i1);

      if (std::string fileName = Utils::readAttrib(xScriptFile, mxconst::get_ATTRIB_NAME(), "")
          ; !fileName.empty())
      {
        //Log::logMsgNone("Start Reading Scripts: " + fileName);
        missionx::base_script bScript;

        // store file name
        bScript.file_name = fileName;

        // construct absolute path+name
        script_path = std::string(inScriptFolder).append(XPLMGetDirectorySeparator()).append(fileName);
        std::string strError;
        strError.clear();

        // read file content into bScript.script_body
        if (ListDir::readExtScriptFile(script_path, &bScript.script_body))
        {
          // set bScript file name information
          bScript.set_script_file_name(fileName);

          // Read Include files
          int iIncludeFiles = xScriptFile.nChildNode(mxconst::get_ELEMENT_INCLUDE_FILE().c_str());
          for (int i2 = 0; i2 < iIncludeFiles; i2++)
          {
            xIncludeFile = xScriptFile.getChildNode(mxconst::get_ELEMENT_INCLUDE_FILE().c_str(), i2);
            if (!xIncludeFile.isEmpty())
            {
              fileName = Utils::readAttrib(xIncludeFile, mxconst::get_ATTRIB_NAME(), "");
              fileName = Utils::trim(fileName); // split Utils::trim command for Linux compiler
              if (fileName.empty())
                continue;
              else
              {
                fileName = Utils::extractBaseFromString(fileName); // remove any extension of the script file name
                bScript.vecIncludeScriptName.push_back(fileName);  // add filename to vector
              }
            }

          } // end loop over include file names

          // add Script data to global map container.
          // v3.0.241.7 Check if script name is not already in map
          if (Utils::isElementExists(inMapScripts, bScript.script_name_no_ext))
          {
            missionx::Log::logAttention("Script file by the name: \"" + bScript.script_name_no_ext + "\" was already loaded. Skipping loading...");
            continue;
          }
          else
          {
            Utils::addElementToMap(inMapScripts, bScript.script_name_no_ext, bScript); // ADD script name to map. File name is KEY
            Log::logMsg("Script: " + script_path + ", was loaded.");      // debug
          }

        } // end read script file
        else
        {
          Log::logMsg(std::string("Fail to read file: ").append( script_path ).append( ".\nError: " ).append( strError) );
        }
      }
      else
      {
        Log::logMsg("No value for script NAME was found.");
      } // end if "name" attribute is empty

    } // end loop over script files

    /////////// READ Scriptlets ////////////////////
    iElements = xParent.nChildNode(mxconst::get_ELEMENT_SCRIPTLET().c_str());
    for (int i1 = 0; i1 < iElements; i1++)
    {
      xScriptlet = xParent.getChildNode(mxconst::get_ELEMENT_SCRIPTLET().c_str(), i1);
      if (!xScriptlet.isEmpty())
      {
        std::string sName       = Utils::readAttrib(xScriptlet, mxconst::get_ATTRIB_NAME(), "");
        std::string sInclude    = Utils::readAttrib(xScriptlet, mxconst::get_ATTRIB_INCLUDE_FILE(), "");
        std::string sScriptBody = ((xScriptlet.nClear() > 0) ? xScriptlet.getClear().sValue : "");

        if (sName.empty())
        {
          addError("Found Scriptlet without a name. Please fix. Skipping...");
          continue;
        }

        if (sScriptBody.empty())
        {
          addError("Scriptlet: \"" + sName + "\", has no code body. Please fix. Skipping...");
          continue;
        }

        // v3.0.241.7 check if scriptlet name was already loaded
        if (Utils::isElementExists(inMapScripts, sName))
        {
          addError("Scriptlet: " + sName + ", was already defined in previous Flight Legs or in the script element. Skipping code...");
          continue;
        }
        else // add scriptlet
        {
          missionx::base_script bScript;
          bScript.isScriptlet = true;
          bScript.file_name   = sName;
          bScript.set_script_file_name(sName);
          bScript.script_body    = sScriptBody;
          //bScript.scriptlet_body = sScriptBody; // v3.305.3 disabled

          if (!sInclude.empty())
          {
            sInclude = Utils::extractBaseFromString(sInclude); // remove any extension of the script file name
            bScript.vecIncludeScriptName.push_back(sInclude);  // add filename to vector
          }

          Utils::addElementToMap(inMapScripts, bScript.script_name_no_ext, bScript);
        }
        // end adding scriptlets
      }
    }

    //////// SHARED VARIABLES /////////
    // Read all shared variables and initialize the script_manager global variables
    // script_manager::mapScriptGlobalBoolArg
    xSharedVariables = xParent.getChildNode(mxconst::get_ELEMENT_SHARED_VARIABLES().c_str());
    if (!xSharedVariables.isEmpty())
    {
      ITCXMLNode xVar;
      iElements = xSharedVariables.nChildNode(mxconst::get_ELEMENT_VAR().c_str());
      for (auto i1 = 0; i1 < iElements; i1++)
      {
        xVar = xSharedVariables.getChildNode(i1);
        if (!xVar.isEmpty())
        {
          std::string name    = Utils::readAttrib(xVar, mxconst::get_ATTRIB_NAME(), "");
          std::string type    = missionx::mxUtils::stringToLower(Utils::readAttrib(xVar, mxconst::get_ATTRIB_TYPE(), "")); // always convert to lower case
          std::string initVal = Utils::readAttrib(xVar, mxconst::get_ATTRIB_INIT_VAL(), EMPTY_STRING);

          if (name.empty() || type.empty())
          {
            Log::logMsgErr("Found \"Global variable\" without a name. or Type without a value. Skipping...");
            continue;
          }

          // initialize embedded script global maps
          if (type.compare("bool") == 0)
          {
            bool val = false;
            if (!initVal.empty())
              mxUtils::isStringBool(initVal, val);

            if (Utils::isElementExists(missionx::script_manager::mapScriptGlobalBoolArg, name))
            {
              addError("A global variable by the name: " + name + ", and type: " + type + ", is already exists. Skipping variable....");
              continue;
            }
            else
              Utils::addElementToMap(missionx::script_manager::mapScriptGlobalBoolArg, name, val);
          }
          else if (type.compare("number") == 0)
          {
            double val = 0.0;
            if (!initVal.empty() && mxUtils::is_number(initVal))
              val = mxUtils::stringToNumber<double>(initVal);

            if (Utils::isElementExists(missionx::script_manager::mapScriptGlobalDecimalArg, name))
            {
              addError("A global variable by the name: " + name + ", and type: " + type + ", is already exists. Skipping variable....");
              continue;
            }
            else
              Utils::addElementToMap(missionx::script_manager::mapScriptGlobalDecimalArg, name, val);
          }
          else if (type.compare("string") == 0)
          {
            std::string val;
            val.clear();

            if (!initVal.empty())
              val = initVal;

            if (Utils::isElementExists(missionx::script_manager::mapScriptGlobalStringsArg, name))
            {
              addError("A global variable by the name: " + name + ", and type: " + type + ", is already exists. Skipping variable....");
              continue;
            }
            else
              Utils::addElementToMap(missionx::script_manager::mapScriptGlobalStringsArg, name, val);
          }

          Log::logMsgNone("[Shared Var] Added shared argument name: " + mxconst::get_QM() + name + mxconst::get_QM());
        }
      } // end loop over global variables
    }


  } // end handling embedded scripts/scriptlets

  Log::printHeaderToLog("END Reading External Scripts", false, missionx::format_type::footer);
}

// -----------------------------------

void
missionx::read_mission_file::addIncludeFiles(std::map<std::string, missionx::base_script>& inMapScripts)
{
  //////// INCLUDE /////////
  // concatenate include files to script body. We will extract the content from script_manager::map_scripts
  // go over each mapScripts container, and read its vector. If vector size is zero then skip, if not do a search for the script name and concatenate it to the beginning of the current script.
  Log::logMsg(mxconst::get_UNIX_EOL()); // debug beautifying for better readability
  for (auto iter : missionx::script_manager::mapScripts)
  {
    base_script* s = &iter.second;
    std::string  incStr;
    incStr.clear();

    if (s->vecIncludeScriptName.empty())
      continue;

    base_script* i = nullptr;                                  // include bas_script pointer
    for (auto iter_include_filename : s->vecIncludeScriptName) // loop over include files
    {
      auto iter_inc_script = missionx::script_manager::mapScripts.find(iter_include_filename); // search if script was loaded to map_scripts
      if (iter_inc_script == script_manager::mapScripts.end())
      {
        s->script_is_valid = false;
        s->err_desc        = "[" + s->script_name_no_ext + "] ->Missing include file: " + iter_include_filename;
        break; // skip loop
      }
      // concatenate include at the beginning of script
      i = &iter_inc_script->second;

      incStr += i->script_body + "\n";
      Log::logMsg("[" + s->script_name_no_ext + "] Adding include file: " + i->script_name_no_ext); // debug

    } // end loop over include vector

    if (!incStr.empty())
    {
      inMapScripts[s->script_name_no_ext].set_script_body(incStr + s->script_body);
    }


  } // end loop over scripts and concatenating include files
}

// -----------------------------------

void
missionx::read_mission_file::readXPlaneDataElement(ITCXMLNode& xParent)
{

  ITCXMLNode xXPData = xParent.getChildNode(mxconst::get_ELEMENT_XPDATA().c_str());
  if (xXPData.isEmpty())
  {
    Log::logMsgNone("--- No Logic element was found.");
  }
  else
  {
    // read dataref
    readDatarefElement(xXPData);
  }

}

// -----------------------------------

void
missionx::read_mission_file::readDatarefElement(ITCXMLNode& xParent)
{

  ITCXMLNode xDref;
  int        nChilds = 0;

  nChilds = xParent.nChildNode(mxconst::get_ELEMENT_DATAREF().c_str()); // count how many dataref elements
  if (nChilds > 0)
  {
    for (int i1 = 0; i1 < nChilds; i1++)
    {
      ////////////////////////
      // Read  DATAREF info//
      ///////////////////////

      xDref = xParent.getChildNode(mxconst::get_ELEMENT_DATAREF().c_str(), i1);
      if (xDref.isEmpty()) // skip if empty element
        continue;

      dataref_param dref;
      dref.node = xDref.deepCopy();

      if (dref.parse_node())
      {
        // Add dataref to the vector if valid
        if (dref.flag_paramReadyToBeUsed)
        {
          Utils::addElementToMap(missionx::data_manager::mapDref, dref.getName(), dref);
        }
      }
    }

    if (!missionx::data_manager::mapDref.empty())
    {
      Log::printHeaderToLog("Start Reading <" + mxconst::get_ELEMENT_DATAREF() + "> Elements");
      
      for (auto& [name, dref] : missionx::data_manager::mapDref)
      {
        Log::logMsg(fmt::format("Added Dataref: \"{}\", Key: \"{}\" ", name, dref.key));        
      }

      Log::printHeaderToLog("Start Reading <" + mxconst::get_ELEMENT_DATAREF() + "> Elements", false, missionx::format_type::footer);

    }


  } // end if Child > 0

}

// -----------------------------------

void
missionx::read_mission_file::readObjectiveAndTaskElements(ITCXMLNode& xParent)
{
  // Log::printHeaderToLog("!!! Start Reading OBJECTIVES Elements !!!");

  int        nObjectives = xParent.nChildNode(mxconst::get_ELEMENT_OBJECTIVES().c_str()); // v3.0.221.15rc4 support multupple objectives elements
  ITCXMLNode xObjectives;
  std::list<missionx::Objective> listOfObjectivesWithErrors;

  if (nObjectives == 0 && missionx::data_manager::mapObjectives.empty())
  {
    Log::logMsg("--- No high level \"objectives\" elements was found.");
  }
  else
  {
    for (int o1 = 0; o1 < nObjectives; ++o1)
    {
      xObjectives = xParent.getChildNode(mxconst::get_ELEMENT_OBJECTIVES().c_str(), o1); // v3.0.221.15rc4 added support to multiple <objectives> in mission file
      if (xObjectives.isEmpty())
      {
        Log::logMsg("Found empty <objectives>.. skipping");
        continue;
      }
      else
      {
        // read Objective Elements
        ITCXMLNode xObj;
        ITCXMLNode xDesc;
        ITCXMLNode xTask;

        int nChilds = 0;

        nChilds = xObjectives.nChildNode(mxconst::get_ELEMENT_OBJECTIVE().c_str()); // count how many <objective> elements
        if (nChilds > 0)
        {
          for (int i1 = 0; i1 < nChilds; i1++)
          {
            ////////////////////////
            // Read  Objective info//
            ///////////////////////

            xObj = xObjectives.getChildNode(mxconst::get_ELEMENT_OBJECTIVE().c_str(), i1);
            if (xObj.isEmpty()) // skip if empty element
            {
              Log::logMsg("Found Empty Objective. Please check.", format_type::error, false);
              continue;
            }


            missionx::Objective obj;
            obj.node = xObj.deepCopy();

            if (obj.parse_node()) // parse objective node and read all its tasks
            {
              // Add Objective to data_manager map. Validation will be done later
              Utils::addElementToMap(data_manager::mapObjectives, obj.name, obj);

            } // end parse objective
            else
            {
              listOfObjectivesWithErrors.emplace_back(obj);
              //Log::logMsgErr("Failed to parse one of the objectives: " + obj.name + ". Skipping it.");
            }

          } // end loop over all <objectives>
        }   // end if <objectives> has <objective> child
      }     // xObjectives
    }       // end loop over <objectives>

  } // end if there are <objectives> element

  //if (!data_manager::mapObjectives.empty())
  //{
  //  Log::logMsg(fmt::format("Loaded: {:.<20}\n{:=<40}", mxconst::get_ELEMENT_OBJECTIVES(), ""));
  //  Log::logMsg("Valid Objectives:\n");
  //  for (auto& [name, obj] : missionx::data_manager::mapObjectives)
  //  {
  //    Log::logMsg(fmt::format("{:.<20}", name.c_str()) );
  //  }
  //
  //  if (!listOfObjectivesWithErrors.empty())
  //  {
  //    Log::logMsg("Invalid Objectives:\n");
  //    for (auto& obj : listOfObjectivesWithErrors)
  //    {
  //      Log::logMsg("Name:" + obj.getName() + "\n");
  //      for (auto& txt : obj.vecErrors)
  //      {
  //        Log::logMsg(fmt::format("{:>5}", txt));
  //      }
  //    }
  //  }
  //}

} // readObjectiveAndTaskElements

// -----------------------------------

// -----------------------------------

void
missionx::read_mission_file::readFlightPlanElements(std::map<std::string, missionx::Waypoint>&        inMapHoldsFlightLegs,
                                                    IXMLNode&                                     inXmlRootChoices,
                                                    ITCXMLNode&                                   xParent,
                                                    std::string&                                  inPathToRootMissionFolder,
                                                    std::map<std::string, missionx::base_script>& inMapScripts,
                                                    std::string&                                  inScriptFolder)
{
  ITCXMLNode xFlightPlan;
  ITCXMLNode xLeg;

  int nChilds1 = 0;

  for (int type_of_element = 0; type_of_element < 2; ++type_of_element)
  {
    std::string main_element_name, child_element_name;

    int nLegs = 0; // v3.0.221.15rc4 support multi <Waypoints>
    if (type_of_element == 0)
    {
      main_element_name  = mxconst::get_ELEMENT_GOALS();
      child_element_name = mxconst::get_ELEMENT_GOAL();
    }
    else
    {
      main_element_name  = mxconst::get_ELEMENT_FLIGHT_PLAN();
      child_element_name = mxconst::get_ELEMENT_LEG();
    }

    nLegs = xParent.nChildNode(main_element_name.c_str());

    for (int g1 = 0; g1 < nLegs; ++g1)
    {
      xFlightPlan = xParent.getChildNode(main_element_name.c_str(), g1);

      if (xFlightPlan.isEmpty())
      {
        Log::logMsgWarn("Found an empty <" + main_element_name + "> element. Skipping...");
        continue;
      }
      else
      {
        // Order is important: read objectives, triggers and messages before flight leg so its validation will succeed
        missionx::read_mission_file::readTriggers(xFlightPlan);                 // v3.0.223.5 add support for <triggers> inside <flight_plan>
        missionx::read_mission_file::readMessages(xFlightPlan);                 // v3.0.223.5 add support for <message_tempaltes> inside <flight_plan>
        missionx::read_mission_file::readObjectiveAndTaskElements(xFlightPlan); // v3.0.223.5 add support for <objectives> inside <flight_plan>

        nChilds1 = xFlightPlan.nChildNode(child_element_name.c_str());
        if (nChilds1 == 0)
        {
          read_mission_file::initOK = false;
          read_mission_file::errMsg = "No <" + child_element_name + "> element defined.";
          addError(errMsg);

          // return
          continue; // v3.0.221.15rc4
        }

        for (int i1 = 0; i1 < nChilds1; i1++)
        {
          xLeg = xFlightPlan.getChildNode(child_element_name.c_str(), i1);
          if (xLeg.isEmpty())
            continue;

          // Define Flight Leg and main attributes
          missionx::Waypoint leg;
          leg.node = xLeg.deepCopy();


          if (leg.parse_node() == false) {
            addError("[FlightLeg Add] Fail to add Flight Leg " + leg.getName() + ", to global map due to no objectives were link to it. Check definitions for Flight Leg: " + leg.getName() + "!!!");
            continue;
          }
          ///// END PARSE <leg> NODE

          ///// READ commands /////
          // v3.0.221.9 read command strings. aeach command must be part of <commands> element, just like datarefs. If a command is not in the commands list, then it will be ignored and we will move to the next one.
          IXMLNode xFireCommandsAtLegStart = leg.node.getChildNode(mxconst::get_ELEMENT_FIRE_COMMANDS_AT_LEG_START().c_str()); // v3.0.221.15rc5 support leg
          if (xFireCommandsAtLegStart.isEmpty())                                                                         // compatibility to goal
            xFireCommandsAtLegStart = leg.node.getChildNode(mxconst::get_ELEMENT_FIRE_COMMANDS_AT_GOAL_START().c_str());

          // v3.0.221.10 moved "start cold and dark workaround" to mission::start_cold_and_dark(), we wont use commands for that unless designer will add them to first goal
          std::string commands;
          commands.clear(); // v3.0.221.10
          if (!xFireCommandsAtLegStart.isEmpty())
          {
            commands = Utils::readAttrib(xFireCommandsAtLegStart, mxconst::get_ATTRIB_COMMANDS(), EMPTY_STRING);
          }


          if (!commands.empty())
          {
            leg.listCommandsAtTheStartOfFlightLeg = missionx::data_manager::parseStringToCommandRef(commands);
          }

          IXMLNode xFireCommandsAtLegEnd = leg.node.getChildNode(mxconst::get_ELEMENT_FIRE_COMMANDS_AT_LEG_END().c_str()); // v3.0.221.15rc5 support leg
          if (xFireCommandsAtLegEnd.isEmpty())                                                                       // compatibility to goal
            xFireCommandsAtLegEnd = leg.node.getChildNode(mxconst::get_ELEMENT_FIRE_COMMANDS_AT_GOAL_END().c_str());

          if (!xFireCommandsAtLegEnd.isEmpty())
          {
            std::string commands        = Utils::readAttrib(xFireCommandsAtLegEnd, mxconst::get_ATTRIB_COMMANDS(), EMPTY_STRING);
            leg.listCommandsAtTheEndOfTheFlightLeg = missionx::data_manager::parseStringToCommandRef(commands);
          }
          // end of v3.0.221.9 commands

          Utils::addElementToMap(inMapHoldsFlightLegs, leg.getName(), leg); // v3.0.241.7.1
        }                                                                   // end loop over all <leg> elements inside <flight_plan>


        // read choices if any
        nChilds1 = xFlightPlan.nChildNode(mxconst::get_ELEMENT_CHOICES().c_str()); // Choices are part of the <flight_plan> and not <leg>
        if (nChilds1 > 0)
          missionx::read_mission_file::readChoices(inXmlRootChoices, xFlightPlan); // v3.0.241.7.1 added container node element to the function so we will be able to use it from load mission file

        readScriptElement(xFlightPlan, inMapScripts, inScriptFolder); // v3.0.241.7 This is an extension to a flight leg, hence it is not in the "leg.parse_node()" main function.

      } // end if <flight_plan> is empty
    }


  } // end loop over flight Legs


  if (inMapHoldsFlightLegs.size() == (size_t)0) // v3.0.241.7.1 use dynamic container
  {
    read_mission_file::initOK = false;
    read_mission_file::errMsg = "No Flight <legs> element defined.";
    missionx::read_mission_file::addError(errMsg);
  }
}
// end readGoalsElements

void
missionx::read_mission_file::readTriggers(ITCXMLNode& xParent)
{
  ITCXMLNode xTriggers;
  ITCXMLNode xTrigger;
  ITCXMLNode xConditions;
  ITCXMLNode xExtScripts;
  ITCXMLNode xMessages;
  ITCXMLNode xLocAndElev; // location and elevation

  int nChilds1 = 0;

  Log::printHeaderToLog("Start Reading Triggers", false);
  //Log::logMsg("Valid trigger info will be displayed after validating Objectives", format_type::warning, false);

  // v3.0.221.15rc4 Enable multiple <triggers> in one mission file and not just one. The idea is to let the designer to split his/her list of triggers into goals to make it easier to follow (especially for long missions)
  int nTriggers = xParent.nChildNode(mxconst::get_ELEMENT_TRIGGERS().c_str());
  for (int t1 = 0; t1 < nTriggers; ++t1) // v3.0.221.15rc4 Loop over multiple <triggers> (if there are more than one)
  {
    xTriggers = xParent.getChildNode(mxconst::get_ELEMENT_TRIGGERS().c_str(), t1);
    if (xTriggers.isEmpty()) // if the <triggers> element is empty, continue to next one
    {
      continue;
    }
    else
    {
      nChilds1 = xTriggers.nChildNode(mxconst::get_ELEMENT_TRIGGER().c_str());
      if (nChilds1 == 0)
      {
        read_mission_file::errMsg = "No <TRIGGER> elements were defined.";
        missionx::read_mission_file::addError(errMsg);

        return;
      }

      ////////////////////////////
      // read all trigger elements
      ///////////////////////////
      for (int i1 = 0; i1 < nChilds1; i1++)
      {
        Trigger trig;

        xTrigger  = xTriggers.getChildNode(mxconst::get_ELEMENT_TRIGGER().c_str(), i1);
        trig.node = xTrigger.deepCopy(); // v3.0.241.1

        const bool flag_trigIsValid = trig.parse_node();


        if (flag_trigIsValid)
        {
          Utils::addElementToMap(data_manager::mapTriggers, trig.getName(), trig);
        }

      } // end loop over all "<trigger>" children

    } // end if <triggers> is NOT empty

  } // end loop over <triggers> elements

   Log::printHeaderToLog("END Reading Triggers", false, missionx::format_type::footer);
}

void
missionx::read_mission_file::postReadTriggers(std::map<std::string, missionx::Trigger>& inMapTriggers)
{
  for (const auto &[name, trigRef] : inMapTriggers)
  {
//    std::string name = trigName;

    // v3.0.217.4
    if (inMapTriggers[name].pCenter.pointState == mx_point_state::point_undefined || ((inMapTriggers[name].pCenter.getLat() == 0.0) && (inMapTriggers[name].pCenter.getLon() == 0.0))) // make sure point center is defined
      inMapTriggers[name].calcCenterOfArea();


    // CUE Info special settings - on all triggers
    inMapTriggers[name].prepareCueMetaData(); // v3.0.213.7
  }
}

void
missionx::read_mission_file::readInventories ( const ITCXMLNode & xParent)
{
  // inventories element - only one
  ITCXMLNode xLocAndElev;

  int nChild1 = 0;

  Log::printHeaderToLog("Start Reading Inventories", false, format_type::header);
  Log::logMsg("Start reading Pre-Defined Items:");

  ITCXMLNode xInvs = xParent.getChildNode ( mxconst::get_ELEMENT_INVENTORIES().c_str () );
  if (xInvs.isEmpty())
  {
    read_mission_file::errMsg = "!!! No <INVENTORIES> element were defined. !!!";
    Log::logMsg(errMsg);

    return;
  }


  //////////////////////////////////////
  // read PLANE inventory
  Log::logMsg(" "); // space between elements
  missionx::Inventory planeInventory;
  missionx::Inventory acfInventory; // holds the station information based on the "aircraft" file.

  // v24.12.2 station parsing if we are NOT in xp11 compatibility mode
  if ( missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i != missionx::XP11_COMPATIBILITY )
  {
    missionx::Inventory::gather_acf_cargo_data(acfInventory);
  }


  if (const IXMLNode xPlaneInv = xInvs.getChildNode(mxconst::get_ELEMENT_PLANE().c_str()).deepCopy()
     ; xPlaneInv.isEmpty())
  {
    planeInventory.node = IXMLNode::emptyIXMLNode;
  }
  else
  {
    planeInventory.node = xPlaneInv; // .deepCopy(); v24.12.2 removed deepcopy since it was already "deep copied" at creation level.
    #ifndef RELEASE
    Log::logDebugBO(fmt::format("Mission File Plane Node:\n{}", Utils::xml_get_node_content_as_text (planeInventory.node))); // DEBUG
    #endif
    if (!planeInventory.parse_node())
    {
      // planeInventory.mapItems.clear();
      planeInventory.node = IXMLNode::emptyIXMLNode;
    }
  } // end read xInv

  // make sure we have a valid plane node from mission file, or we create it.
  if (planeInventory.node.isEmpty())
  {
    planeInventory.node = IXMLNode::createXMLTopNode(mxconst::get_ELEMENT_PLANE().c_str());
    if (planeInventory.node.isEmpty()) // create new plane node and test if it is empty
    {
      Log::logMsgErr(fmt::format("[{}] There was an issue preparing plane inventory. Please notify developer.", __func__));
    }
    else
    {
      planeInventory.setNodeStringProperty(mxconst::get_ATTRIB_NAME(), mxconst::get_ELEMENT_PLANE()); // v24.12.2
      planeInventory.setNodeStringProperty(mxconst::get_ATTRIB_TYPE(), "");       // v24.12.2
    }
  }
  assert(planeInventory.node.isEmpty() == false && std::string(__func__).append(": Failed to create plane inv node").c_str() ) ; // abort if plane inventory node was failed to be created.

  // v24.12.2 merge the ACF inventory with the plane inventory. The ACF inventory is the target plane Inventory
  if ( missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i != missionx::XP11_COMPATIBILITY )
  {
    // we override the plane inventory with the ACF inventory node, after we "copied" the items to the ACF stations.
    planeInventory.node = missionx::Inventory::mergeAcfAndPlaneInventories2(acfInventory.node, planeInventory.node);
    planeInventory.parse_node();
    // planeInventory = missionx::Inventory::mergeAcfAndPlaneInventories(acfInventory, planeInventory);
  }

  Utils::addElementToMap(missionx::data_manager::mapInventories, mxconst::get_ELEMENT_PLANE(), planeInventory); // v3.0.241.1

  #ifndef RELEASE
  Utils::xml_print_node(missionx::data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()].node); // v24.12.2
  #endif

  //////////////////////////////////////
  // read and store external inventories
  Log::logMsg("<< Read External Inventories >>"); // space between elements

  nChild1 = xInvs.nChildNode(mxconst::get_ELEMENT_INVENTORY().c_str());
  for (int i1 = 0; i1 < nChild1; ++i1)
  {
    if (ITCXMLNode xInv = xInvs.getChildNode(mxconst::get_ELEMENT_INVENTORY().c_str(), i1); !xInv.isEmpty())
    {
      if (missionx::Inventory inv(xInv); inv.parse_node())
      {
        if (const std::string invName = inv.getName(); invName != mxconst::get_ELEMENT_PLANE()) // v24.12.2 make sure not a plane inventory
        {
          Utils::addElementToMap(data_manager::mapInventories, invName, inv);
          // !!! pointer info must be saved after Inventory info was added to Inventory map (for graphical Cue) !!!
          data_manager::mapInventories[invName].prepareCueMetaData();

          Log::logMsg("Added Inventory: " + invName);
        }
      } // end if valid
    }   // end read xInv

  } // end read all Inventory elements


  #ifndef RELEASE
  Log::logMsg("Inventories added: \n==============\n");

  for (const auto &[invName, invRef] : missionx::data_manager::mapInventories)
  {
    Log::logMsg("Inventory: " + invName);
    Log::logMsgNone(xmlWriter.getString(invRef.node));
    Log::logMsg(" \n");
  }
  #endif

  Log::printHeaderToLog(">> END INVENTORY <<<<<<<<<", false, format_type::footer);
}



// -----------------------------------

void
missionx::read_mission_file::readMessages(ITCXMLNode& xParent)
{
  Log::printHeaderToLog("Start Reading Messages", false);

  int childs01 = 0;

  childs01 = xParent.nChildNode(mxconst::get_ELEMENT_MESSAGE_TEMPLATES().c_str());
  if (childs01 > 0)
  {
    for (int i1 = 0; i1 < childs01; i1++) // loop over all "message_templates"
    {
      ITCXMLNode xMessageTemplates = xParent.getChildNode(mxconst::get_ELEMENT_MESSAGE_TEMPLATES ().c_str(), i1);
      if (!xMessageTemplates.isEmpty())
      {
        int nMessages = xMessageTemplates.nChildNode(mxconst::get_ELEMENT_MESSAGE ().c_str());

        for (int i2 = 0; i2 < nMessages; i2++) // loop over all <message> elements
        {
          ITCXMLNode xMessage = xMessageTemplates.getChildNode(mxconst::get_ELEMENT_MESSAGE ().c_str(), i2);
          if (!xMessage.isEmpty())
          {
            std::string mName = Utils::readAttrib(xMessage, mxconst::get_ATTRIB_NAME(), "");

            Message m; // holds text message properties (include the text message)
            m.node = xMessage.deepCopy();
            if (m.parse_node())
            {
              #ifndef RELEASE
              //Log::logMsg(m.to_string()); // debug
              #endif // !RELEASE

              Utils::addElementToMap(data_manager::mapMessages, mName, m);
            }

          } // end message element is empty

        } // end loop over <message>
      }   // end xMessageTemplates is not empty
    }     // end loop over <message_template>
  }       // end loop over all <message_templates>

  Log::printHeaderToLog("END Reading Messages", false, missionx::format_type::footer);
}

// -----------------------------------

void
missionx::read_mission_file::readEndMission(ITCXMLNode& xParent, std::string& inPathToMissionBrieferFolder)
{
  ITCXMLNode  xEndMission;
  ITCXMLNode  xChild;
  std::string num;
  num.clear();
  std::string strVal;
  strVal.clear();
  std::string err;
  err.clear();

  xEndMission = xParent.getChildNode(mxconst::get_ELEMENT_END_MISSION().c_str());
  if (!xEndMission.isEmpty())
  {
    data_manager::endMissionElement.node = xEndMission.deepCopy(); // v3.0.241.1 store end element
  } // end EndMission element exists
}

missionx::mx_location_3d_objects
missionx::read_mission_file::readLocationElement(ITCXMLNode& xNode)
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

bool
missionx::read_mission_file::readPointElement(ITCXMLNode& xNode, missionx::Point& inPoint)
{
  bool pointIsValid = true;

  missionx::mx_location_3d_objects info = read_mission_file::readLocationElement(xNode);

  // location validations
  if (Utils::is_number(info.lat) && Utils::is_number(info.lon))
  {
    inPoint.setLat(mxUtils::stringToNumber<double>(info.lat));
    inPoint.setLon(mxUtils::stringToNumber<double>(info.lon));
  }
  else
  {
    Log::logMsgErr("[read point] One of the coordination Lat/Lon might be malformed: " + mxconst::get_QM() + info.lat + "," + info.lon + mxconst::get_QM() + ". Skipping...");
//    pointIsValid = false;
    return false;
  }

  if (!info.elev.empty() && Utils::is_number(info.elev))
    inPoint.setElevationFt(mxUtils::stringToNumber<double>(info.elev));
  else
    inPoint.setElevationFt(0); // on ground

  // add elevation above ground logic // v3.0.200
  if (Utils::is_number(info.elev_above_ground))
  {
    double elevAboveGround = mxUtils::stringToNumber<double>(info.elev_above_ground);
    if (elevAboveGround != 0.0)
    {
      XPLMProbeResult outProbeResult;
      float           groundElevation = (float)UtilsGraph::getTerrainElevInMeter_FromPoint(inPoint, outProbeResult);
      if (outProbeResult == xplm_ProbeHitTerrain)
        inPoint.setElevationFt(groundElevation + elevAboveGround);
    }
  }
  else
    inPoint.setElevationAboveGroundFt(0); // basically ignore this value and use elevation value instead

  // read tilt information: heading, pitch, roll
  if (!info.heading.empty() && mxUtils::is_number(info.heading))
  {
    inPoint.setHeading(mxUtils::stringToNumber<float>(info.heading));
  }
  else
    inPoint.setHeading(0.0f);

  if (!info.pitch.empty() && mxUtils::is_number(info.pitch))
  {
    inPoint.setPitch(mxUtils::stringToNumber<float>(info.pitch));
  }
  else
    inPoint.setPitch(0.0f);

  if (!info.roll.empty() && mxUtils::is_number(info.roll))
  {
    inPoint.setRoll(mxUtils::stringToNumber<float>(info.roll));
  }
  else
    inPoint.setRoll(0.0f);

  if (!info.speed.empty() && mxUtils::is_number(info.speed))
  {
    inPoint.setSpeedInKmh(mxUtils::stringToNumber<float>(info.speed));
  }
  else
    inPoint.setSpeedInKmh(0.0); // v3.0.253.7
 
  // v3.0.207.5 - adjust heading
  if (!info.adjust_heading.empty() && mxUtils::is_number(info.adjust_heading))
  {
    inPoint.adjust_heading = mxUtils::stringToNumber<float>(info.adjust_heading);
  }
  else
    inPoint.adjust_heading = 0.0f; // v3.0.207.2 default speed 10kmh



  return pointIsValid;
}



missionx::mx_location_3d_objects
missionx::read_mission_file::readConditionElement(ITCXMLNode& xNode)
{
  missionx::mx_location_3d_objects info;

  info.distance_to_display_nm = Utils::readAttrib(xNode, mxconst::get_ATTRIB_DISTANCE_TO_DISPLAY_NM(), "");
  // v3.0.221.15rc5 support leg
  info.keep_until_leg = Utils::readAttrib(xNode, mxconst::get_ATTRIB_KEEP_UNTIL_LEG(), mxconst::get_ATTRIB_KEEP_UNTIL_GOAL(), "", true);

  info.cond_script = Utils::readAttrib(xNode, mxconst::get_ATTRIB_COND_SCRIPT(), "");

  return info;
}



// -----------------------------------

void
missionx::read_mission_file::read3DObjects(ITCXMLNode& xParent)
{

  Log::printHeaderToLog("Reading 3D Objects");

  // read object template node
  ITCXMLNode xObjTemplate = xParent.getChildNode(mxconst::get_ELEMENT_OBJECT_TEMPLATES().c_str());
  if (xObjTemplate.isEmpty())
    return; // skip rest of code


  ITCXMLNode x3dObj;
  ITCXMLNode xConditions;
  ITCXMLNode xLocation;
  ITCXMLNode xTilt;
  ITCXMLNode xPath; // v3.0.202

  int nChilds1 = 0;

  // Read 3D Object
  nChilds1 = xObjTemplate.nChildNode(mxconst::get_ELEMENT_OBJ3D().c_str());
  for (int i1 = 0; i1 < nChilds1; i1++)
  {
    x3dObj = xObjTemplate.getChildNode(mxconst::get_ELEMENT_OBJ3D().c_str(), i1);

    missionx::obj3d mission_obj3d;
    mission_obj3d.node = x3dObj.deepCopy();

    if (mission_obj3d.parse_node())
    {

      // add to global 3D map
      Utils::addElementToMap(data_manager::map3dObj, mission_obj3d.getName(), mission_obj3d);

      // print formatted data
      Log::logMsgNone(mission_obj3d.to_string());
    } // end if parse_node()
  }
  // end loop

  Log::printHeaderToLog("END 3D Objects");
}

// -----------------------------------

void
missionx::read_mission_file::readGPS(ITCXMLNode& xParent)
{
  if (xParent.isEmpty())
    return;

  ITCXMLNode xGPS                = xParent.getChildNode(mxconst::get_ELEMENT_GPS().c_str());
  missionx::data_manager::xmlGPS = xGPS.deepCopy();
}

// -----------------------------------


void
missionx::read_mission_file::readChoices(IXMLNode& inXmlRootChoices, ITCXMLNode& xParent)
{
  // we reset missionx::data_manager::xmlChoices at the beginning of the load_mission_file() function to <mx_choices>

  if (xParent.isEmpty())
    return;


  // we read and merge all <choice> elements into one element: <mx_choices>
  const int nChoices = xParent.nChildNode(mxconst::get_ELEMENT_CHOICES().c_str());
  for (int i1 = 0; i1 < nChoices; ++i1)
  {
    auto xChoices = xParent.getChildNode(mxconst::get_ELEMENT_CHOICES().c_str(), i1); // ITCXMLNode
    if (!xChoices.isEmpty())
    {
      const int nSubChoices = xChoices.nChildNode(mxconst::get_ELEMENT_CHOICE().c_str());
      for (int i2 = 0; i2 < nSubChoices; ++i2)
      {
        auto xChoice_ptr = xChoices.getChildNode(mxconst::get_ELEMENT_CHOICE().c_str(), i2);
        inXmlRootChoices.addChild(xChoice_ptr.deepCopy()); // add <choice> element to <mx_choices>
      }
    }
  }
}
// -----------------------------------
// -----------------------------------


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////   LOAD    LOAD    LOAD   /////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
bool
missionx::read_mission_file::load_mission_file(const std::string& inPathAndFileName, std::string inPathToRootMissionFolder)
{
  // internal function members
  initOK = true; // v3.0.145 fix bug - function always returned "false"
  std::string strLogMsg;
  std::string pluginDataFullFileNameDir;

  // strLogMsg.clear();

  // clear information from current mission (if loaded)
  data_manager::stopMission();
  script_manager::clear(); // v3.0.202a // added to be on the safe side. Need tome tests though.
  data_manager::xLoadInfoNode.initBaseNode(); // v3.305.3

  errMsg.clear();

  read_mission_file::vecErrors.clear();
  data_manager::lstLoadErrors.clear(); // v3.305.3

  /////////////
  dummy_top_node = IXMLNode::emptyIXMLNode;

  dummy_top_node = IXMLNode::createXMLTopNode("xml", TRUE);
  dummy_top_node.addAttribute(mxconst::get_ATTRIB_VERSION().c_str(), "1.0");
  dummy_top_node.addAttribute("encoding", "ISO-8859-1");
  // add disclaimer
  dummy_top_node.addClear("\n\tFile has been created by Mission-X plugin.\n\tAny modification might break or invalidate the file.\n\t", "<!--", "-->");


  missionx::data_manager::xmlChoices = dummy_top_node.deepCopy().addChild(mxconst::get_ELEMENT_MX_CHOICES().c_str()).deepCopy(); // we will create the <mx_choice> node and store it

  // load file
  if (inPathAndFileName.empty())
  {
    errMsg = "[ERROR] No File Picked.";
    Log::logMsg(errMsg); // debug
    initOK = false;
    return false; // fail load
  }
  else
  {
    strLogMsg = "Loading File: " + inPathAndFileName;
    Log::logMsg(strLogMsg); // debug
  }

  strLogMsg.clear();

  try
  {
    missionx::data_manager::set_found_missing_3D_object_files(false); // v3.0.255.3 reset missing 3D files state

    pluginDataFullFileNameDir = inPathAndFileName; 


    Log::logMsg("[Load] Full Path to xml mission: " + pluginDataFullFileNameDir); // debug

    // This creates a new Incredible XML DOM parser:
    IXMLDomParser iDom;

    errMsg.clear();
    // This open and parse the XML file:
    ITCXMLNode xMainNode_local = iDom.openFileHelper(pluginDataFullFileNameDir.c_str(), mxconst::get_MISSION_ELEMENT().c_str(), &errMsg);

    if (errMsg.empty())
    {
      /* read MISSION attributes */

      // clear maps and images data
      data_manager::clearMissionLoadedTextures(); // v3.0.156


      // store the copy of MISSION element
      missionx::data_manager::xMainNode = xMainNode_local.deepCopy(); // v3.0.241.1

      missionx::data_manager::missionState = mx_mission_state_enum::mission_is_being_loaded_from_the_original_file; // v25.03.1, used when parsing [plane] inventory in Inventory::gather_acf_cargo

      // v24.12.2 store the supported version
      missionx::data_manager::mission_file_supported_versions = Utils::readAttrib(missionx::data_manager::xMainNode, "version", "0");
      // missionx::data_manager::vecCurrentMissionFileSupportedVersions = Utils::split(missionx::data_manager::mission_format_versions_s, ',');

      // read global_settings
      readGlobalSettings(xMainNode_local); 

      // read Briefer
      readBriefer(xMainNode_local);

      // read script elements and includes
      std::string f = data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_ATTRIB_SCRIPT_FOLDER_NAME(), "", errMsg);
      ITCXMLNode  xScripts = xMainNode_local.getChildNode ( mxconst::get_ELEMENT_EMBEDDED_SCRIPTS().c_str () );
      if (!xScripts.isEmpty())
        readScriptElement(xScripts, script_manager::mapScripts, f);

      // read Dataref elements
      readXPlaneDataElement(xMainNode_local);

      // read Commands elements
      //readXPlaneDataElement(xMainNode_local); // v3.0.221.9 // v3.303.13 DEPRECATED, duplicate code and commands are read from <leg> element

      // read triggers
      readTriggers(xMainNode_local);

      // read inventories
      readInventories(xMainNode_local); // v3.0.213.1

      // read messages
      readMessages(xMainNode_local);

      // read Objectives
      readObjectiveAndTaskElements(xMainNode_local);

      // read 3D Objects
      read3DObjects(xMainNode_local);

      // read xp commands
      // readCommandElement(xMainNode); // v3.0.221.9

      // read Goals
      readFlightPlanElements(missionx::data_manager::mapFlightLegs, missionx::data_manager::xmlChoices, xMainNode_local, inPathToRootMissionFolder, script_manager::mapScripts, f); // v3.0.200a2 added path

      // basic validations
      if (missionx::data_manager::mapObjectives.empty() )
      {
        missionx::read_mission_file::initOK = false;
        Log::logMsgErr("[" + std::string(__func__) + "] No <objective> element found. Check your mission syntax.");
      }

      // Add include files to scripts/scriptlet that already loaded
      addIncludeFiles(script_manager::mapScripts);


      // read EndMission element
      std::string briefer_folder = inPathToRootMissionFolder + XPLMGetDirectorySeparator() + mxconst::get_BRIEFER_FOLDER() + XPLMGetDirectorySeparator();
      readEndMission(xMainNode_local, briefer_folder);

      // read GPS data if any
      readGPS(xMainNode_local); // v3.0.215.7

      // read choices
      readChoices(missionx::data_manager::xmlChoices, xMainNode_local); // v3.0.231.1


      // VALIDATIONS
      initOK = post_load_validations(xMainNode_local.deepCopy(), inPathAndFileName); // v3.0.255.3 added xMainNode
      if (missionx::read_mission_file::initOK)
      {
        missionx::data_manager::mx_global_settings.flag_loadedFromSavepoint = false;

        // Display story message estimate timings
        post_load_calc_story_message_timings(xMainNode_local.deepCopy()); // v3.305.3
      }
      //// print errors ////
      if (Log::get_deq_errors_container_size())
      {
        Log::printHeaderToLog("Summary of errors/warnings of mission");
        Log::logMsg(Log::print_deq_LoadMissionFileErrors(true));
      }

      //if (!data_manager::loadErrors.empty())
      if (!data_manager::lstLoadErrors.empty())
      {
        Log::printHeaderToLog("Other Errors/Warnings:");
        std::ranges::for_each ( data_manager::lstLoadErrors, [](const std::string& s) { Log::logMsgNone(s); }); // v3.305.3
      }

      #ifndef RELEASE
      Log::printHeaderToLog("DEBUG XML Errors Warnings and Info:");
      Utils::xml_print_node(missionx::data_manager::xLoadInfoNode.node); // v3.305.3
      #endif // !RELEASE


    } // if XML file was loaded and no errors found
    else
    {
      missionx::data_manager::load_error_message = errMsg; // v3.0.251.1
      Log::logMsg("Failed to load XML file ");             // debug
      Log::logMsg(errMsg);
      Log::logMsg("--- End Error ---");
      initOK = false;
    }
  }
  catch (std::exception& ex)
  {
    Log::logMsg(std::string("[Exception In loading Mission File] ").append(ex.what()));
    Log::logMsg(std::string("[Exception In loading Mission File] ").append(ex.what()), Log::LOG_ERROR, false);
    addError("[Fatal Error][Exception In loading Mission File] Notify Developer.");
    initOK = false;
  }


  return initOK;
}

bool
missionx::read_mission_file::post_load_validations(const IXMLNode& xMainNode, const std::string& inPathAndFileName)
{

  std::string messeges;

  Log::printHeaderToLog("Start Validating Objectives");

  // Validate Objectives and their Tasks
  // v3.303.14
  if (missionx::data_manager::mapObjectives.size() <= static_cast<size_t> ( 0 ) )
  {
    Log::logMsgErr("[" + std::string(__func__) + "] No objectives where found.\nPlease check your mission file if you enclosed all<objective> elements inside <objectives> parent element.\nExample:\n<objectives>\n\t<objective...>\n\t...\n\t</objective>\n</objectives>\n");
    //missionx::read_mission_file::initOK = false;
    return false;
  }

  for (auto & [objName, obj] : missionx::data_manager::mapObjectives)
  {

    auto cLogNode = data_manager::xLoadInfoNode.addChild(mxconst::get_ELEMENT_OBJECTIVE(), mxconst::get_ATTRIB_NAME(), objName, "");
    assert(cLogNode.isEmpty() == false && fmt::format("[{}] XML Log Node must not be empty.", __func__).c_str());


    if (data_manager::validateObjective(obj, missionx::data_manager::errStr, messeges)) // Validate that all tasks in Objective have correct configuration (from dependency perspective and mandatory task that is not dependent).
    {
      //Log::logMsgNone("** Objective: \"" + objName + "\"" + ((obj.isValid) ? " is valid." : " is not valid."));
      std::string txt = fmt::format("Objective: \"{}\" {} ", objName, ((obj.isValid) ? " is valid." : " is not valid.")); // v3.305.3
      Utils::xml_add_info_child(cLogNode, txt);                                                                           // v3.305.3

      if (!data_manager::errStr.empty())
        addError(data_manager::errStr);
    }
    else
    {
      const std::string txt = fmt::format("Found errors while validating an objective: '{}'. Please check the mission file!!!", objName); // v3.305.3
      Utils::xml_add_info_child(cLogNode, txt);

      //Log::logMsgErr("Found errors while validating an objective: " + objName + ". Please check the mission file!!!"); // v3.305.3 deprecated
      addError(missionx::data_manager::errStr);


      //read_mission_file::initOK = false; // V3.305.3 DEPRECATED
      return false; // stop validation
    }

  } // end validate Objectives

  // Validate Flight Legs
  if (!missionx::data_manager::validateFlightLegs(missionx::data_manager::errStr, messeges)) // if validation failed
  {
    // v3.305.3
    std::string txt = fmt::format("Found errors while validating the flight legs. Please check the mission file!!!");
    Utils::xml_add_info_child(missionx::data_manager::xLoadInfoNode.node, txt);

    addError(missionx::data_manager::errStr);
    return false; // stop validation
  }

  // v3.0.255.3 check display_object file existence
  bool found_missing_3D_files{ false };

  if (!inPathAndFileName.empty()) // we do not search for 3D object files if mission file is empty, like in "load saved mission"
  {
    auto xGlobalSettings     = xMainNode.getChildNode(mxconst::get_GLOBAL_SETTINGS().c_str(), 0);
    auto xObjectTemplates    = xMainNode.getChildNode(mxconst::get_ELEMENT_OBJECT_TEMPLATES().c_str(), 0);
    int  flight_plan_nodes_i = xMainNode.nChildNode(mxconst::get_ELEMENT_FLIGHT_PLAN().c_str());
    // loop over all <flight_plan> nodes
    for (int i1 = 0; i1 < flight_plan_nodes_i; ++i1)
    {
      auto fp = xMainNode.getChildNode(mxconst::get_ELEMENT_FLIGHT_PLAN().c_str(), i1);
      if (!fp.isEmpty())
      {        
        missionx::data_manager::validate_display_object_file_existence(inPathAndFileName, fp, xGlobalSettings, xObjectTemplates, errMsg);


        if (!errMsg.empty() && !found_missing_3D_files)
        {
          found_missing_3D_files = true;
          addError(errMsg);
        }
      }
    } // end loop over all <flight_plan>s elements
  }   // end if inPathAndFileName are not empty

  errMsg.clear();

  return true;
}

// ------------------------------------

void
missionx::read_mission_file::post_load_calc_story_message_timings(IXMLNode xMainNode)
{
  std::map<std::string, float> mapMessagesToDisplay;
  Message::lineAction4ui.init();

  auto vecNodePtr = Utils::xml_get_all_nodes_pointer_with_tagName(xMainNode, mxconst::get_ELEMENT_MESSAGE ());
  // loop over all <message> nodes.
  for (auto& node : vecNodePtr)
  {
    const std::string name = Utils::readAttrib(node, mxconst::get_ATTRIB_NAME(), "");
    if (!node.isEmpty() && (Utils::readAttrib(node, mxconst::get_ATTRIB_MODE(), "").compare(mxconst::get_MESSAGE_MODE_STORY()) == 0))
    {

      missionx::Message msg;      
      msg.node = node.deepCopy();
      if (msg.parse_node(true))
      {
        int iLineCounter = 0;
        std::string outAction_s   = "";
        bool        bIsActionLine;
        float       cumulativeSeconds = 0.0f;

        while (!msg.dqMsgLines.empty() && iLineCounter < 1000)
        {
          std::string line = msg.get_and_filter_next_line(outAction_s, bIsActionLine, iLineCounter); // v3.305.3 was: lmbda_get_and_filter_next_line(msg->dqMsgLines, outAction_s, bIsActionLine);
          if (line.empty())
          {
            cumulativeSeconds += 1; // adding one since on the next cycle the handling of last line will take place.
            break;
          }

          char action = '\0';

          if (!bIsActionLine) // default TEXT line
          { 
            bIsActionLine = true;
            action        = mxconst::STORY_ACTION_TEXT;
          }

          if (bIsActionLine)
          {
            if (action == '\0') // we did not manually modified it for text
              action = outAction_s[1];

            switch (action)
            {
              case mxconst::STORY_ACTION_PAUSE:
              {
                if (msg.parse_action(mxconst::STORY_ACTION_PAUSE, line))
                {
                  if (Message::lineAction4ui.vals[mxconst::get_STORY_PAUSE_TIME()].empty())
                    cumulativeSeconds += mxconst::DEFAULT_SKIP_MESSAGE_TIMER_IN_SEC_F;
                  else
                    cumulativeSeconds += mxUtils::stringToNumber<float>(Message::lineAction4ui.vals[mxconst::get_STORY_PAUSE_TIME()]);
                }
                cumulativeSeconds += 1; // we add 1 since we need at least one flight loop back cycle to start the pause
              }
              break;
              case mxconst::STORY_ACTION_MSGPAD:
              case mxconst::STORY_ACTION_TEXT:
              {
                if (msg.parse_action(action, line))
                {
                  float fExponentTime = 1.0f;                                      // v3.305.3
                  if (missionx::Message::lineAction4ui.bIgnorePunctuationTiming)   // We won't ignore punctuation totally, we will halve the punctuation timing (0.5f)
                    fExponentTime = mxconst::STORY_DEFAULT_PUNCTUATION_EXPONENT_F; //  return mxconst::STORY_DEFAULT_TIME_BETWEEN_CHARS_SEC_F;

                  // Calculate line time
                  for (const auto& c : Message::lineAction4ui.vals[mxconst::get_STORY_TEXT()])
                  {
                    switch (c)
                    {
                      case '.':
                      case '?':
                      case '!':
                      {
                        cumulativeSeconds += mxconst::STORY_DEFAULT_TIME_AFTER_PERIOD_SEC_F * fExponentTime;
                      }
                      break;
                      case ',':
                      case ';':
                      {
                        cumulativeSeconds += mxconst::STORY_DEFAULT_TIME_AFTER_COMMA_SEC_F;
                      }
                      break;
                      default:
                      {
                        cumulativeSeconds += mxconst::STORY_DEFAULT_TIME_BETWEEN_CHARS_SEC_F;
                        break;
                      }
                    } // end switch
                    
                  } // end loop over all characters

                  // Add default pause time for each text line
                  cumulativeSeconds += mxconst::DEFAULT_SKIP_MESSAGE_TIMER_IN_SEC_F + 1; // We add 1 second since the pause command occurs in the next iteration
                }

              } // end case line is action: TEXT
              break;
              default:
              {
                // we will add 1 second for any other action since it needed a cycle to just taken care off.[h] or [i] would be consumed and only on the next cycle we will dill with the next action.
                cumulativeSeconds += 1.0f; 
              }
              break;
            } // end Switch
          } // end if is bAction Line
        } // end while looping over all message queue

        if (cumulativeSeconds > 0.0f)
        {
          Utils::addElementToMap(mapMessagesToDisplay, name, cumulativeSeconds);           
        }

      }
      else
      {
        Log::logMsg("Errors in message: " + name + ", skipping.");
      }

    } // end if node is valid

  } // end loop over all messages in vector



  // print time information sorted by message name
  if (!mapMessagesToDisplay.empty())
  {
    Log::printHeaderToLog("START Story Messages Time Evaluation (Ordered by name)");
    Log::logMsg( R"(
This is only a rough estimation. 
Since there are many parameters affecting this equation it means that the numbers here are on the optimistic side, you will probably have to add at least ~10 seconds.

For exact timing you will have to let the story mode play the full message sequence and observe the timing in the log file.
)");

    for (const auto& [name, timing] : mapMessagesToDisplay)
    {
      Log::logMsg(fmt::format("{}{:.<60}{}{:.2f} sec.", "Story Message: ", name, "Evaluate timing: ", timing));
    }
    Log::printToLog("END Story Messages Time Evaluation", false, missionx::format_type::footer, true);
  }

  Message::lineAction4ui.init();
  
} // post_load_calc_story_message_timings

////////////////////   LOAD    SAVEPOINT    LOAD   /////////////////////////////////////////////////

bool
missionx::read_mission_file::loadSavePoint()
{

  initOK = true;
  Message::lineAction4ui.init(); // v3.305.3

  static std::map<std::string, missionx::Inventory> mapInventories;    // v3.0.213.7

  errMsg.clear();
  vecErrors.clear();
  data_manager::lstLoadErrors.clear(); // v3.305.3

  std::string saveFile = missionx::data_manager::missionSavepointFilePath;

  try
  {
    Briefer briefer;

    std::map<std::string, missionx::Timer> mapFailureTimers; // v3.0.253.7

    missionx::GLobalSettings newGlobalSettings;
    missionx::mx_base_node   endMissionElement;

    std::map<std::string, missionx::Objective>     mapObjectives;
    std::map<std::string, missionx::Waypoint>      mapFlightLegs;

    std::map<std::string, missionx::dataref_param> mapDref;
    std::map<std::string, missionx::dataref_param> mapDrefInterpolation; // v3.305.3 // v24.05.2

    std::map<std::string, missionx::Trigger>       mapTriggers;
    std::map<std::string, missionx::Message>       mapMessages;

    std::map<std::string, missionx::base_script> mapScripts; // will hold script strings to use between iterations.
    // 3D Objects
    std::map<std::string, missionx::obj3d> map3dLoaded;          // will hold 3d Object file templates
    std::map<std::string, missionx::obj3d> map3dInstancesLoaded; // copied 3D template. Active 3D Objects = rendered or should be rendered


    std::unordered_map<std::string, std::string> mapScriptGlobalStringsArg; // will hold script strings to use between iterations.
    std::unordered_map<std::string, bool>        mapScriptGlobalBoolArg;    // will hold script boolean, this is int=0 or int=1
    std::unordered_map<std::string, double>      mapScriptGlobalDecimalArg; // will hold script decimal numbers (double). It will also holds int numbers we will convert to int before sending to script
    std::deque<missionx::messageLine_strct>      mxpad_messages;            // active mxpad messages // v3.0.19x


    IXMLNode xGPS;
    IXMLNode xFMS;
    IXMLNode xChoices;


    // This creates a new Incredible XML DOM parser:
    IXMLDomParser iDom;
    // This open and parse the XML file:
    ITCXMLNode xMainNodeFile = iDom.openFileHelper(saveFile.c_str(), mxconst::get_ELEMENT_SAVE().c_str(), &errMsg);
    IXMLNode   xMainNode     = xMainNodeFile.deepCopy();

    const std::string save_file_format_versions_s = Utils::readAttrib(xMainNode, "version", "");

    if (!errMsg.empty())
    {
      Log::logMsgNone("Failed to load XML Save file "); // debug
      Log::logMsgErr(errMsg);
      Log::logMsgNone("--- End Error ---");
      initOK = false;
      return initOK;
    }

    // read mission properties
    IXMLNode xChild = xMainNode.getChildNode(mxconst::get_GLOBAL_SETTINGS().c_str());
    if (xChild.isEmpty())
    {
      addError("No mission properties found. Save file is not valid. Aborting.");
      return false;
    }


    newGlobalSettings.node = xChild.deepCopy(); // v3.0.241.1
    if (newGlobalSettings.parse_node()) // v3.0.241.1
      missionx::data_manager::prepare_new_mission_folders(newGlobalSettings);

    // Read Mission Folder properties
    xChild = xMainNode.getChildNode(mxconst::get_PROP_MISSION_FOLDER_PROPERTIES().c_str());
    if (xChild.isEmpty())
    {
      addError("No mission folders properties found. Save file is not valid. Aborting.");
      return false;
    }


    //// read End Element Success ////
    xChild = xMainNode.getChildNode(mxconst::get_ELEMENT_END_MISSION().c_str());
    if (xChild.isEmpty())
    {
      addError("No <end mission success> properties found. Will use defaults.");
    }
    else
      endMissionElement.node = xChild.deepCopy();



    // read Briefer
    briefer.node = xMainNode.getChildNode(mxconst::get_ELEMENT_BRIEFER().c_str()).deepCopy(); // v3.0.241.1
    if (briefer.node.isEmpty())
    {
      addError("No <briefer> properties found in save file. Aborting mission load.");
      return false;
    }

    if (!briefer.parse_node())
    {
      addError("Briefer element is not valid. Aborting mission load.");
      return false;
    }



    // load script information

    // read scripts
    std::string f = missionx::data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_ATTRIB_SCRIPT_FOLDER_NAME(), "", errMsg); // v3.0.241.1

    // read globals
    IXMLNode xEmbedded = xMainNode.getChildNode(mxconst::get_ELEMENT_EMBEDDED_SCRIPTS().c_str());
    if (!xEmbedded.isEmpty())
    {
      read_mission_file::readScriptElement(xEmbedded, mapScripts, f); // we send the parent node, since readScriptElement() is querying for <embedded> element

      // read globals
      int nChilds = xEmbedded.nChildNode(mxconst::get_ELEMENT_SCRIPT_GLOBAL_STRING_PARAMS().c_str());
      for (int i1 = 0; i1 < nChilds; i1++)
      {
        IXMLNode xChild = xEmbedded.getChildNode(mxconst::get_ELEMENT_SCRIPT_GLOBAL_STRING_PARAMS().c_str(), i1);
        if (!xChild.isEmpty())
        {
          std::string name = xChild.getAttribute(mxconst::get_ATTRIB_NAME().c_str());
          std::string val  = (xChild.nClear() > 0) ? xChild.getClear().sValue : "";
          if (!name.empty())
            mapScriptGlobalStringsArg.insert(std::pair<std::string, std::string>(name, val));
        }
      } // end loop over string
      nChilds = xEmbedded.nChildNode(mxconst::get_ELEMENT_SCRIPT_GLOBAL_NUMBER_PARAMS().c_str());
      for (int i1 = 0; i1 < nChilds; i1++)
      {
        IXMLNode xChild = xEmbedded.getChildNode(mxconst::get_ELEMENT_SCRIPT_GLOBAL_NUMBER_PARAMS().c_str(), i1);
        if (!xChild.isEmpty())
        {
          std::string name = xChild.getAttribute(mxconst::get_ATTRIB_NAME().c_str());
          std::string val  = (xChild.nClear() > 0) ? xChild.getClear().sValue : "";
          if (val.empty() || name.empty()) // skip if empty
            continue;

          if (Utils::is_number(val))
          {
            float dVal = Utils::stringToNumber<float>(val);
            mapScriptGlobalDecimalArg.insert(std::pair<std::string, double>(name, dVal));
          }
        }
      } // end loop over global numbers
      nChilds = xEmbedded.nChildNode(mxconst::get_ELEMENT_SCRIPT_GLOBAL_BOOL_PARAMS().c_str());
      for (int i1 = 0; i1 < nChilds; i1++)
      {
        IXMLNode xChild = xEmbedded.getChildNode(mxconst::get_ELEMENT_SCRIPT_GLOBAL_BOOL_PARAMS().c_str(), i1);
        if (!xChild.isEmpty())
        {
          std::string name = xChild.getAttribute(mxconst::get_ATTRIB_NAME().c_str());
          std::string val  = (xChild.nClear() > 0) ? xChild.getClear().sValue : "";
          if (val.empty() || name.empty()) // skip if empty
            continue;

          if (Utils::is_number(val))
          {
            bool dVal = Utils::stringToNumber<bool>(val);
            mapScriptGlobalBoolArg.insert(std::pair<std::string, bool>(name, dVal));
          }
        }
      } // end loop over booleans
    }
    // end load embedded Scripts from ROOT node ///////////////////////////


    // read Flight Legs  ///////////////////////////
    std::string mission_pkg_folder = missionx::data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_ATTRIB_MISSION_PACKAGE_FOLDER_PATH(), "", errMsg);
    readFlightPlanElements(mapFlightLegs, xChoices, xMainNode, mission_pkg_folder, mapScripts, f);



    if (mapFlightLegs.empty()) // v3.0.241.1
    {
      // read embed scripts in ELEMENT_FLIGHT_PLAN <flight_plan>
      addError("!!! No Flight Legs found in save file. Aborting Mission Load !!!!");
      return false; // break only if mandatory
    }


    if (mapFlightLegs.size() < 1)
    {
      addError("No Flight Legs were found. Aborting missions savepoint load.");
      return false;
    }

    // Add include files to scripts/scriptlet that already loaded
    addIncludeFiles(mapScripts);
    ////////// end Flight Plan (loading all flight <leg> s //////////


    //// Read Raw Objectives ////
    if (!read_mission_file::load_saved_elements(mapObjectives, xMainNode, mxconst::get_ELEMENT_OBJECTIVES(), mxconst::get_ELEMENT_OBJECTIVE(), true, errMsg))
    {
      addError(errMsg);
      return false; // break only if mandatory
    }


    //// read triggers ////
    if (read_mission_file::load_saved_elements(mapTriggers, xMainNode, mxconst::get_ELEMENT_TRIGGERS(), "TRIGGER", false, errMsg)) // v3.0.241.1
    {
      // v3.0.213.7
      missionx::read_mission_file::postReadTriggers(mapTriggers);
    }
    else
      addError(errMsg);


    //// read Messages ////
    if (!read_mission_file::load_saved_elements(mapMessages, xMainNode, mxconst::get_ELEMENT_MESSAGE_TEMPLATES (), mxconst::get_ELEMENT_MESSAGE (), false, errMsg))
    {
      addError(errMsg);
    }

    // read Logic
    if (!read_mission_file::load_saved_elements(mapDref, xMainNode, mxconst::get_ELEMENT_LOGIC(), mxconst::get_ELEMENT_DATAREF(), false, errMsg))
    {
      addError(errMsg);
    }

    // read interpolation
    if (!read_mission_file::load_saved_elements(mapDrefInterpolation, xMainNode, mxconst::get_ELEMENT_INTERPOLATION(), mxconst::get_ELEMENT_DATAREF(), false, errMsg))
    {
      addError(errMsg);
    }


    //// READ MX-PAD related information
    for (int i1 = 0; i1 < xMainNode.nChildNode(mxconst::get_ELEMENT_MXPAD_DATA().c_str()); ++i1)
    {
      IXMLNode xMxPad_data = xMainNode.getChildNode(mxconst::get_ELEMENT_MXPAD_DATA().c_str(), i1); // it seem there might be more than 1 mxpad element in save file. This is sort of a  bug, but thie simple workaround should fix this.
      if (!xMxPad_data.isEmpty())
      {
        // read active MX-PAD messages. Main code is in mxconst::QMM
        QueueMessageManager::loadCheckpoint(xMxPad_data, mxpad_messages, errMsg);
        if (!errMsg.empty())
          addError(errMsg);
      }
    }


    ////// READ 3D Objects //////

    IXMLNode x3dObjectRoot = xMainNode.getChildNode(mxconst::get_PROP_OBJECTS_ROOT().c_str());
    if (!x3dObjectRoot.isEmpty())
    {
      if (!read_mission_file::load_saved_elements(map3dLoaded, x3dObjectRoot, mxconst::get_ELEMENT_OBJECT_TEMPLATES(), mxconst::get_ELEMENT_OBJ3D(), false, errMsg))
      {
        addError(errMsg);
      }

      if (!read_mission_file::load_saved_elements(map3dInstancesLoaded, x3dObjectRoot, mxconst::get_PROP_OBJECTS_INSTANCES(), mxconst::get_ELEMENT_OBJ3D(), false, errMsg, mxconst::get_ATTRIB_INSTANCE_NAME()))
      {
        addError(errMsg);
      }
    }


    //////////////////////////////////////////////////////////////
    // read inventories after commiting to the newGlobalSettings
    /////////////////////////////////////////////////////////////
    missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i = missionx::data_manager::get_inv_layout_based_on_mission_ver_and_compatibility_node(save_file_format_versions_s, newGlobalSettings.xCompatibility_ptr, data_manager::flag_setupUseXP11InventoryUI); // v25.03.1
    ////// READ Inventories //////
    if ( IXMLNode xInventories = Utils::xml_get_or_create_node_ptr ( xMainNode, mxconst::get_ELEMENT_INVENTORIES() )
        ; !xInventories.isEmpty())
    {
      //// Read External Inventories
      int nChild = xInventories.nChildNode(mxconst::get_ELEMENT_INVENTORY().c_str());
      for (int i1 = 0; i1 < nChild; ++i1)
      {
        if ( IXMLNode xInventory = xInventories.getChildNode ( mxconst::get_ELEMENT_INVENTORY().c_str (), i1 )
            ; !xInventory.isEmpty())
        {
          if (!missionx::read_mission_file::load_saved_elements(mapInventories, xMainNode, mxconst::get_ELEMENT_INVENTORIES(), mxconst::get_ELEMENT_INVENTORY(), false, errMsg))
            addError(errMsg);
          // v25.03.1 Read <plan> inventory. Fix bug, plane inventory was not read from save file
          if (!missionx::read_mission_file::load_saved_elements(mapInventories, xMainNode, mxconst::get_ELEMENT_INVENTORIES(), mxconst::get_ELEMENT_PLANE(), false, errMsg))
            addError(errMsg);
        }
      } // end loop over external inventories


      // Make sure <plane> element exists. Create it if not.
      IXMLNode xPlane = Utils::xml_get_or_create_node_ptr(xInventories, mxconst::get_ELEMENT_PLANE());

      // read plane inventory
      if (!missionx::read_mission_file::load_saved_elements(mapInventories, xMainNode, mxconst::get_ELEMENT_INVENTORIES(), mxconst::get_ELEMENT_PLANE(), true, errMsg)) // fail if there is no plane inventory
        addError(errMsg);

      // the "Inventory::opt_forceInventoryLayoutBasedOnVersion_i" was initialized after reading the "<global_setting>"
      if (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY
          && mxUtils::isElementExists ( mapInventories, mxconst::get_ELEMENT_PLANE() )
          && mapInventories[mxconst::get_ELEMENT_PLANE()].node.nChildNode(mxconst::get_ELEMENT_ITEM().c_str ()) == 0
          && mapInventories[mxconst::get_ELEMENT_PLANE()].node.nChildNode(mxconst::get_ELEMENT_STATION().c_str ()) > 0)
      {
        // copy all station items to the root <plane> element
        auto iStations = mapInventories[mxconst::get_ELEMENT_PLANE()].node.nChildNode(mxconst::get_ELEMENT_STATION().c_str ());
        for ( auto &plane_station : mapInventories[mxconst::get_ELEMENT_PLANE()].mapStations | std::views::values )
        {
          const auto nItems = plane_station.node.nChildNode(mxconst::get_ELEMENT_ITEM().c_str ());
          for (int i2=0; i2 < nItems; ++i2)
            mapInventories[mxconst::get_ELEMENT_PLANE()].node.addChild ( plane_station.node.getChildNode ( mxconst::get_ELEMENT_ITEM().c_str (), i2 ).deepCopy () );
        }

      } // end reading plane inventory

    } // end loading Inventories


    ///////////////////////////////////////////////
    /////////// Read Failure Timers //////////////
    /////////////////////////////////////////////
    int nTimers = xMainNode.nChildNode(mxconst::get_ELEMENT_TIMER().c_str());
    for (int i1 = 0; i1 < nTimers; ++i1)
    {
      if ( auto xTimer = xMainNode.getChildNode ( mxconst::get_ELEMENT_TIMER().c_str (), i1 )
        ; !xTimer.isEmpty())
      {
        missionx::Timer timer;
        timer.node = xTimer.deepCopy();
        if (timer.parse_node() && timer.getState() != missionx::mx_timer_state::timer_ended) // add timer only if it was not ended
        {
          if (!Utils::isElementExists(mapFailureTimers, timer.getName()))
            Utils::addElementToMap(mapFailureTimers, timer.getName(), timer);
        }
      }
    }



    xGPS     = xMainNode.getChildNode(mxconst::get_ELEMENT_GPS().c_str()).deepCopy();        // v3.0.219.7
    xFMS     = xMainNode.getChildNode(mxconst::get_ELEMENT_FMS().c_str()).deepCopy();        // v3.0.219.7
    xChoices = xMainNode.getChildNode(mxconst::get_ELEMENT_MX_CHOICES().c_str()).deepCopy(); // v3.0.231.1


    // display any error/warning message
    Log::logMsg("Summary of errors/warnings in mission file", format_type::header, false);

    // for (auto e : read_mission_file::vecErrors)
    Log::logMsg(Log::print_deq_LoadMissionFileErrors(true));

    // Prepare current data with Loaded data
    if (initOK)
    {
      missionx::data_manager::stopMission(); // v3.0.145 replaced clear() with stopMission to clear texture and briefer info
      missionx::script_manager::clear();
      missionx::data_manager::mapInterpolDatarefs.clear(); // v3.305.3

      missionx::data_manager::briefer = briefer;

      #ifdef APL // v3.0.160 // fix compilation error. OSX clang does not interpreter the operator "=" as expected.

      Utils::cloneMap(mapFlightLegs, missionx::data_manager::mapFlightLegs);
      Utils::cloneMap(mapObjectives, missionx::data_manager::mapObjectives);

      Utils::cloneMap(mapTriggers, missionx::data_manager::mapTriggers);
      Utils::cloneMap(mapDref, missionx::data_manager::mapDref);
      Utils::cloneMap(mapDrefInterpolation, missionx::data_manager::mapInterpolDatarefs); // v3.305.3
      Utils::cloneMap(mapMessages, missionx::data_manager::mapMessages);

      missionx::data_manager::mx_global_settings = newGlobalSettings; // v3.0.241.1

      missionx::data_manager::endMissionElement = endMissionElement; // v3.0.241.1

      Utils::cloneMap(mapScripts, missionx::script_manager::mapScripts);
      Utils::cloneMap(mapScriptGlobalBoolArg, missionx::script_manager::mapScriptGlobalBoolArg);
      Utils::cloneMap(mapScriptGlobalDecimalArg, missionx::script_manager::mapScriptGlobalDecimalArg);
      Utils::cloneMap(mapScriptGlobalStringsArg, missionx::script_manager::mapScriptGlobalStringsArg);

      missionx::QueueMessageManager::mxpad_messages.clear();          // v3.0.215.3
      missionx::QueueMessageManager::mxpad_messages = mxpad_messages; // v3.0.19x

      Utils::cloneMap(mapInventories, missionx::data_manager::mapInventories);       // v3.0.213.7
      // Utils::cloneMap(mapItemBlueprints, missionx::data_manager::mapItemBlueprints); // v3.0.213.7

      Utils::cloneMap(map3dLoaded, missionx::data_manager::map3dObj);                // v3.0.213.7
      Utils::cloneMap(map3dInstancesLoaded, missionx::data_manager::map3dInstances); // v3.0.213.7
      Utils::cloneMap(mapFailureTimers, missionx::data_manager::mapFailureTimers);   // v3.0.253.7

      #else
      missionx::data_manager::mapFlightLegs = mapFlightLegs;
      missionx::data_manager::mapObjectives = mapObjectives;

      missionx::data_manager::mapTriggers = mapTriggers;
      missionx::data_manager::mapDref     = mapDref;
      missionx::data_manager::mapInterpolDatarefs = mapDrefInterpolation; // v3.305.3
      missionx::data_manager::mapMessages = mapMessages;

      missionx::data_manager::mx_global_settings = newGlobalSettings; // v3.0.241.1
      missionx::data_manager::endMissionElement  = endMissionElement; // v3.0.241.1


      missionx::script_manager::mapScripts                = mapScripts;
      missionx::script_manager::mapScriptGlobalBoolArg    = mapScriptGlobalBoolArg;
      missionx::script_manager::mapScriptGlobalDecimalArg = mapScriptGlobalDecimalArg;
      missionx::script_manager::mapScriptGlobalStringsArg = mapScriptGlobalStringsArg;

      missionx::QueueMessageManager::mxpad_messages.clear();
      missionx::QueueMessageManager::mxpad_messages = mxpad_messages; // v3.0.19x

      missionx::data_manager::mapInventories    = mapInventories;    // v3.0.213.7
      // missionx::data_manager::mapItemBlueprints = mapItemBlueprints; // v3.0.213.7

      missionx::data_manager::map3dObj         = map3dLoaded;          // v3.0.213.7
      missionx::data_manager::map3dInstances   = map3dInstancesLoaded; // v3.0.213.7
      missionx::data_manager::mapFailureTimers = mapFailureTimers;     // v3.0.253.7

      #endif


      missionx::data_manager::xmlGPS       = xGPS.deepCopy();     // v3.0.219.7
      missionx::data_manager::xmlLoadedFMS = xFMS.deepCopy();     // v3.0.231.1
      missionx::data_manager::xmlChoices   = xChoices.deepCopy(); // v3.0.231.1

      // v3.0.241.1 rename <SAVE> main node to <MISSION>
      xMainNode.updateName(mxconst::get_MISSION_ELEMENT().c_str());
      missionx::data_manager::xMainNode = xMainNode.deepCopy();      

      missionx::data_manager::mx_global_settings.flag_loadedFromSavepoint = true;

      initOK = read_mission_file::post_load_validations(xMainNode); // v3.0.255.3 we do not validating existence of 3D object files. Only when loading mission file and after generating random file but not when loading saved mission progress.

      if (missionx::read_mission_file::initOK)
      {

        missionx::data_manager::mission_file_supported_versions = save_file_format_versions_s; // v25.03.1 set the global shared variable.

        //// print errors ////
        Log::logMsg("Summary of errors/warnings of mission", format_type::header, false);
        Log::logMsg(Log::print_deq_LoadMissionFileErrors(true));

        //if (!data_manager::loadErrors.empty())
        if (!data_manager::lstLoadErrors.empty())
        {
          Log::logMsgNone(">> Other Errors/Warnings:");
          std::ranges::for_each ( data_manager::lstLoadErrors, [](const std::string& s) { Log::logMsgNone(s); }); // v3.305.3

          //Log::logMsgWarn(data_manager::loadErrors);
        }
      }
    }
  }
  catch (std::exception& ex)
  {
    Log::logMsg(std::string("[Exception In loading Save File] ").append(ex.what()));
    Log::logMsg(std::string("[Exception In loading Save File] ").append(ex.what()), Log::LOG_ERROR, false);
    addError("[Fatal Error][Exception In loading Mission File] Notify Developer.");
    initOK = false;
  }

  return initOK;
}
// load savepoint

// -----------------------------------
// -----------------------------------
// -----------------------------------
