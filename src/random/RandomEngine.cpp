/*
 * RandomEngine.cpp
 *
 *  Created on: Dec 13, 2018
 *      Author: snagar
 */
#include "RandomEngine.h"
#include "../io/system_actions.h"
#include "../io/ListDir.h"
#include "../core/dataref_manager.h" // v3.305.2 added

#include <algorithm>
// #include <math.h>
#include <cmath>

#ifdef MAC
#if __has_include(<filesystem>) && (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif

#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif
#else // Linux and IBM
#include <filesystem>
#include <utility>
namespace fs = std::filesystem;
#endif

namespace missionx
{
//#define ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL 0

std::thread                         missionx::RandomEngine::thread_ref;
missionx::base_thread::thread_state missionx::RandomEngine::threadState;

std::map<std::string, std::string>                          missionx::RandomEngine::row_gather_db_data;
std::unordered_map<int, std::map<std::string, std::string>> missionx::RandomEngine::resultTable_gather_random_airports;
std::unordered_map<int, std::map<std::string, std::string>> missionx::RandomEngine::resultTable_gather_ramp_data;

//// weather 
std::string missionx::RandomEngine::current_weather_datarefs_s;


RandomEngine::RandomEngine()
{

  // this line is to test against other compilers too.
  mapPlaneEnumToStringTypes.clear();
  mapPlaneStringTypesToEnum.clear();
  mapPlaneStringTypesToEnum[""]            = missionx::mx_plane_types::plane_type_any;
  mapPlaneStringTypesToEnum["helos"]       = missionx::mx_plane_types::plane_type_helos;
  mapPlaneStringTypesToEnum["prop"]        = missionx::mx_plane_types::plane_type_props;
  mapPlaneStringTypesToEnum["prop_floats"] = missionx::mx_plane_types::plane_type_prop_floats;
  mapPlaneStringTypesToEnum["heavy"]       = missionx::mx_plane_types::plane_type_heavy;
  mapPlaneStringTypesToEnum["turboprops"]  = missionx::mx_plane_types::plane_type_turboprops;
  mapPlaneStringTypesToEnum["jet"]         = missionx::mx_plane_types::plane_type_jets;

  mapPlaneStringTypesToEnum["ga"]        = missionx::mx_plane_types::plane_type_ga;
  mapPlaneStringTypesToEnum["ga_floats"] = missionx::mx_plane_types::plane_type_ga_floats;


  for ( auto &[key, value]  : mapPlaneStringTypesToEnum)
  {
    mapPlaneEnumToStringTypes[value] = key; // for translation
  }

  this->working_tempFile_ptr          = nullptr; // v3.0.241.9
  this->flag_rules_defined_by_user_ui = false;   // v3.0.241.9
  this->flag_picked_from_osm_database = false;   // v3.0.241.10

  init();
}

// -----------------------------------

void
RandomEngine::init()
{
  // clear
  this->pathToRandomBrieferFolder.clear();
  this->pathToRandomRootFolder.clear();

  this->errMsg.clear();

  xTargetMainNode = IXMLNode::emptyIXMLNode;

  xTargetMainNode = IXMLNode::createXMLTopNode("xml", TRUE);
  xTargetMainNode.addAttribute(mxconst::get_ATTRIB_VERSION().c_str(), "1.0");
  xTargetMainNode.addAttribute("encoding", "ASCII"); // "ISO-8859-1");
  // add disclaimer
  xTargetMainNode.addClear("\n\tFile has been created by Mission-X plug-in.\n\tAny modification might break or invalidate the file.\n\t", "<!--", "-->");

  xRootTemplate = IXMLNode::emptyIXMLNode;
  xDummyTopNode = IXMLNode::emptyIXMLNode; // holds the <MISSION> element

  data_manager::xmlMappingNode = IXMLNode::emptyIXMLNode;


  this->xFlightLegs     = IXMLNode::emptyIXMLNode; // holds briefer element information
  this->xGlobalSettings = IXMLNode::emptyIXMLNode; // holds global settings information
  this->xBrieferInfo    = IXMLNode::emptyIXMLNode; // holds briefer element information
  this->xBriefer        = IXMLNode::emptyIXMLNode;
  this->xObjectives     = IXMLNode::emptyIXMLNode;
  this->xTriggers       = IXMLNode::emptyIXMLNode;
  this->xInventoris     = IXMLNode::emptyIXMLNode;
  this->xMessages       = IXMLNode::emptyIXMLNode;
  this->xEnd            = IXMLNode::emptyIXMLNode; // holds end element information
  this->xGPS            = IXMLNode::emptyIXMLNode; // holds GPS coordinates
  this->x3DObjTemplate  = IXMLNode::emptyIXMLNode; // holds 3D Object Templates
  this->xChoices        = IXMLNode::emptyIXMLNode; // holds <choices> options.
  this->xpData          = IXMLNode::emptyIXMLNode; // holds datarefs elements.
  this->xEmbedScripts   = IXMLNode::emptyIXMLNode; // holds <embedded_scripts> data

  this->mapFlightPlanOrder_si.clear();
  this->mapFLightPlanOrder_is.clear();

  this->listNavInfo.clear();
  this->flag_found = false;

  this->template_plane_type_enum = missionx::mx_plane_types::plane_type_any; // v3.0.221.11
  this->planeLocation.init();
  this->lastFlightLegNavInfo.init();

  this->flag_isLastFlightLeg = false; // v3.0.219.11

  expected_slope_at_target_location_d = 0.0f;

  missionx::RandomEngine::threadState.thread_wait_state = missionx::mx_random_thread_wait_state_enum::not_waiting; // v3.0.221.3
  this->mapNavAidsFromMainThread.clear();
  this->map_customScenery_XPLMNavRef_NavAidsFromMainThread.clear();

  this->cumulative_location_desc_s.clear();
}

// -----------------------------------

void
RandomEngine::addTriggersBasedOnTargetLocation(NavAidInfo& inNav, IXMLNode& inSpecialLegSubNode)
{
  IXMLNode xSpecialTriggers;

  if (inSpecialLegSubNode.isEmpty())
    return;


  // search for set of triggers to add to xTriggers
  const std::string add_triggers_from_template = Utils::readAttrib(inSpecialLegSubNode, mxconst::get_ATTRIB_ADD_TRIGGERS_FROM_TEMPLATE(), ""); // v3.0.221.10 should hold the tag name of the messages we want to xMessages
  if (!add_triggers_from_template.empty())
    xSpecialTriggers = xRootTemplate.getChildNode(add_triggers_from_template.c_str()); // if we find an element with this name, we will add all sub elements to xTriggers


  // try to get radius


  // Add pre-defined triggers
  if (!xSpecialTriggers.isEmpty())
  {
    Utils::add_xml_comment(xTriggers, " (((( Added Triggers )))) ");



    int nChilds = xSpecialTriggers.nChildNode(mxconst::get_ELEMENT_TRIGGER().c_str());
    for (int i1 = 0; i1 < nChilds; ++i1)
    {
      IXMLNode cNode = xSpecialTriggers.getChildNode(mxconst::get_ELEMENT_TRIGGER().c_str(), i1);
      if (!cNode.isEmpty())
      {
        // Try to modify radius if it is empty at the added trigger node. we use the data from inNav class that should hold the new target location
        std::string trigger_radius_mt = Utils::xml_get_attribute_value_drill(cNode, mxconst::get_ATTRIB_RADIUS_MT(), this->flag_found, mxconst::get_ELEMENT_RADIUS());
        if (mxUtils::is_number(inNav.radius_mt_suggested_s) && trigger_radius_mt.empty()) // v3.0.221.10 added the radius_mt to triggers so will be same area as target
          Utils::xml_search_and_set_attribute_in_IXMLNode(cNode, mxconst::get_ATTRIB_RADIUS_MT(), inNav.radius_mt_suggested_s, mxconst::get_ELEMENT_POINT());

        Utils::xml_search_and_set_attribute_in_IXMLNode(cNode, mxconst::get_ATTRIB_LAT(), inNav.getLat(), mxconst::get_ELEMENT_POINT());
        Utils::xml_search_and_set_attribute_in_IXMLNode(cNode, mxconst::get_ATTRIB_LONG(), inNav.getLon(), mxconst::get_ELEMENT_POINT());

        xTriggers.addChild(cNode.deepCopy());
      }
    }


    Utils::add_xml_comment(xTriggers, " )))) End Added Triggers (((( ");
  }
}

// -----------------------------------

void
RandomEngine::injectCountdownTimers()
{
  static constexpr double HELICOPTER_AVERAGE_SPEED_IN_KNOTS = 75.0;
  static constexpr double MIN_SEARCH_TIME_IN_MIN            = 20.0;
  static constexpr double HOVER_TIME                        = 5.0;
  // Check if we generate from "USer creation layer" and we want to add Countdown Timers
  // 1. LOOP over all flight legs start from last one
  // 2. If last then skip, but store its NavAid
  // 3. If not Last one then calculate distance between prevNav and CurNav and calculate a time in minutes
  // 3.1 inject <timer>
  // 4. Continue until first flight leg
  if (this->flag_rules_defined_by_user_ui)
  {
    if (data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_user_generates_a_mission_layer)
    {
      const bool bAddTimers = Utils::readBoolAttrib(missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_ADD_COUNTDOWN(), false);
      if (bAddTimers)
      {
        // loop over all flight legs and add timer to all of them except the last.
        std::string err;
        #ifndef RELEASE
        Log::logMsg("[DEBUG random] inject countdown timers.", true);
        #endif

        auto lmbda_get_briefer_nav_aid = [&](const std::string& inName) {
          NavAidInfo* nav_ptr = nullptr;
          for (auto& nav : this->listNavInfo)
          {
            if (nav.flag_is_brieferOrStartLocation || nav.flightLegName == inName) // same unique Leg name // v3.303.10 added the briefer flag logic
              return &nav;
          }

          return nav_ptr;
        };

        NavAidInfo* brieferNav = lmbda_get_briefer_nav_aid(mxconst::get_ELEMENT_BRIEFER());
        if (brieferNav == nullptr)
          return;

        NavAidInfo* prevNav      = brieferNav; // v3.0.253.9.1 will hold NavAid info to calculate distance. Initialize with the briefer information for calculating distance between start and first leg
        // IXMLNode    prev_leg_ptr = IXMLNode::emptyIXMLNode;

        assert(prevNav != nullptr && "[inject countdown] Failed to find briefer element");

        const int nChilds = this->xFlightLegs.nChildNode(mxconst::get_ELEMENT_LEG().c_str());

        for (int i1 = 0; i1 < nChilds; ++i1)
        {
          IXMLNode leg_ptr = xFlightLegs.getChildNode(mxconst::get_ELEMENT_LEG().c_str(), i1); // pointer to <leg> xml element
          if (leg_ptr.isEmpty())
            continue;

          std::string flight_leg_name = Utils::readAttrib(leg_ptr, mxconst::get_ATTRIB_NAME(), "");

          auto iterEnd = this->listNavInfo.end();
          for (auto iter = this->listNavInfo.begin(); iter != iterEnd; ++iter)
          {

            if (iter->flightLegName == flight_leg_name)
            {
              auto         nextIter                  = std::next(iter);
              const double distance_to_next_navaid_d = iter->p - prevNav->p;

              const auto time_relative_to_avg_speed_in_min = distance_to_next_navaid_d / HELICOPTER_AVERAGE_SPEED_IN_KNOTS * 60; // we multiply by 60 minutes to get hours

              const auto minVal = (time_relative_to_avg_speed_in_min > MIN_SEARCH_TIME_IN_MIN) ? time_relative_to_avg_speed_in_min : MIN_SEARCH_TIME_IN_MIN;
              const auto maxVal = (minVal <= MIN_SEARCH_TIME_IN_MIN) ? MIN_SEARCH_TIME_IN_MIN + HOVER_TIME + Utils::getRandomRealNumber(5.0, 10.0)
                                                                     : time_relative_to_avg_speed_in_min + HOVER_TIME; // v3.0.255.4 fixed assertion where minVal was larger than maxVal.
              const int timeInMin = static_cast<int>(Utils::getRandomRealNumber(minVal, maxVal));

              auto xml_timer_ptr = Utils::xml_get_or_create_node_ptr(leg_ptr, mxconst::get_ELEMENT_TIMER());
              if (!xml_timer_ptr.isEmpty())
              {
                xml_timer_ptr.updateAttribute((flight_leg_name + "_timer").c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
                xml_timer_ptr.updateAttribute(mxUtils::formatNumber<int>(timeInMin).c_str(), mxconst::get_ATTRIB_TIME_MIN().c_str(), mxconst::get_ATTRIB_TIME_MIN().c_str());

                if (nextIter != iterEnd)
                  xml_timer_ptr.updateAttribute(nextIter->flightLegName.c_str(), mxconst::get_ATTRIB_RUN_UNTIL_LEG().c_str(), mxconst::get_ATTRIB_RUN_UNTIL_LEG().c_str());
                else
                  xml_timer_ptr.updateAttribute("", mxconst::get_ATTRIB_RUN_UNTIL_LEG().c_str(), mxconst::get_ATTRIB_RUN_UNTIL_LEG().c_str());

              } // end if we have valid <timer> node pointer

              prevNav      = &(*iter);
              // prev_leg_ptr = leg_ptr;
              break; // Exit for loop
            }

          } // end loop over iterator

        } // end loop over "flight_leg"s

      } // end - user flagged he/she wants timers

    } // end generated from layer: option_user_generates_a_mission_layer

  } // end if flag_rules_defined_by_user_ui
}

// -----------------------------------

bool
RandomEngine::get_user_wants_to_start_from_plane_position()
{
  if (data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_ils_layer
      || data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_external_fpln_layer
      || data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::flight_leg_info
      )
    return Utils::readBoolAttrib(missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_START_FROM_PLANE_POSITION(), false);

  return false;
}


// -----------------------------------

RandomEngine::~RandomEngine()
{
  iDomTemplate.clear();
}

// -----------------------------------

void
RandomEngine::setError(const std::string& inMsg)
{
  this->errMsg = inMsg;
  #ifndef RELEASE
  Log::logMsgNone(errMsg, true);
  #endif
}

// -----------------------------------

bool
RandomEngine::exec_generate_mission_thread(const std::string& inKey)
{

  if (missionx::RandomEngine::threadState.flagIsActive)
  {
    this->setError("\"Generate Mission Engine\" is already running. Please wait for it to finish.");
    return false;
  }


  // start thread
  if (!missionx::RandomEngine::threadState.flagIsActive)
  {
    if (missionx::RandomEngine::thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
      missionx::RandomEngine::thread_ref.join();       // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"

    this->init(); // reset all variables
    RandomEngine::threadState.dataString = inKey;
    missionx::RandomEngine::thread_ref   = std::thread(&missionx::RandomEngine::generateRandomMission, this);
  }

  return true;
}

// -----------------------------------

void
RandomEngine::stop_plugin()
{
  RandomEngine::threadState.flagAbortThread = true;
  if (RandomEngine::threadState.flagIsActive)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  if (RandomEngine::thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
    RandomEngine::thread_ref.join();       // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"

}

// -----------------------------------

std::string
RandomEngine::inject_files_into_xml(missionx::TemplateFileInfo* tempFile_ptr)
{
  if (tempFile_ptr != nullptr)
  {
    fs::path template_file = tempFile_ptr->getAbsoluteTemplateXmlFilePath();
    if (fs::exists(template_file) && fs::is_regular_file(template_file))
    {

      //// Reconstruct the XML file parse template and remove <mission_info>
      std::string err;
      IXMLNode    xMappings{ IXMLNode::emptyIXMLNode };
      IXMLNode    xREPLACE_OPTIONS{ IXMLNode::emptyIXMLNode }; // v3.0.255.4.1

      if ( ITCXMLNode x_mapping_node = this->iDomTemplate.openFileHelper ( template_file.string ().c_str (), mxconst::get_MAPPING_ROOT_DOC().c_str (), &err )
          ; err.empty() && !x_mapping_node.isEmpty())
        xMappings = x_mapping_node.deepCopy();

      err.clear();
      ITCXMLNode x_replace_options = this->iDomTemplate.openFileHelper(template_file.string().c_str(), mxconst::get_ELEMENT_TEMPLATE_REPLACE_OPTIONS().c_str(), &err); // parse xml into ITCXMLNode
      if (err.empty() && !x_replace_options.isEmpty())
        xREPLACE_OPTIONS = x_replace_options.deepCopy();

      err.clear();
      ITCXMLNode xTemplateNode = this->iDomTemplate.openFileHelper(template_file.string().c_str(), mxconst::get_TEMPLATE_ROOT_DOC().c_str(), &err); // parse xml into ITCXMLNode

      if (!err.empty() && this->xRootTemplate.isEmpty()) // check if there is any failure during read
      {
        Log::logMsgThread("[random error inject find/replace] " + err);
        return missionx::EMPTY_STRING;
      }
      err.clear();

      auto xRootTemplate = xTemplateNode.deepCopy(); // convert ITCXMLNode to IXMLNode. IXMLNode allow to modify itself

      IXMLRenderer xmlRender;
      std::string  xml_template_node_content_s = xmlRender.getString(xRootTemplate);

      std::ios_base::sync_with_stdio(false);
      std::cin.tie(nullptr);

      if (!xml_template_node_content_s.empty())
      {
        const std::string original_xml_template_node_content_s = xml_template_node_content_s; // store original string

        // check if we have valid user pick value // v24.03.2 added "vecReplaceOptions_s" empty check to solve crash
        if (!tempFile_ptr->mapOptionsInfo.empty())
        {
          bool bDoneReplacement = false;
          for (const auto& [seq_key, option_info] : tempFile_ptr->mapOptionsInfo)
          {
            // Compatibility option, will have a "seq_key = -1", so it needs the "<REPLACE_OPTIONS>" node as parent node, while "seq_key>=0" needs the correct <option> sub node.
            auto pNode = (seq_key < 0) ? tempFile_ptr->nodeReplaceOptions : tempFile_ptr->nodeReplaceOptions.getChildNode(mxconst::get_ELEMENT_OPTION_GROUP().c_str(), seq_key);
            if (pNode.isEmpty()) // skip if node is empty
              continue;

            const std::string user_pick_s = option_info.vecReplaceOptions_s.at(static_cast<size_t>(option_info.user_pick_from_replaceOptions_combo_i));
            // find the <opt > node with user_pick_s name
            // v24.12.2 Use the "parent" node to get the correct "<opt>" child node based on the "user_pick_s" value.
            auto optNode_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(pNode, mxconst::get_ELEMENT_OPT(), mxconst::get_ATTRIB_NAME(), user_pick_s, false);
            if (optNode_ptr.isEmpty())
              continue;


            bDoneReplacement = true;

            // loop over all <find_replace>
            const int nFindReplaceCount = optNode_ptr.nChildNode(mxconst::get_ELEMENT_FIND_REPLACE().c_str());
            for (int i = 0; i < nFindReplaceCount; ++i)
            {
              auto              frNode              = optNode_ptr.getChildNode(mxconst::get_ELEMENT_FIND_REPLACE().c_str(), i);
              const std::string find_s              = Utils::readAttrib(frNode, mxconst::get_ATTRIB_FIND(), "");
              const std::string replace_with_file_s = Utils::readAttrib(frNode, mxconst::get_ATTRIB_REPLACE_WITH(), "");
              if (find_s.empty() || replace_with_file_s.empty())
                continue;
              else
              {
                fs::path txt_file = tempFile_ptr->filePath + "/" + replace_with_file_s;
                if (fs::exists(txt_file) && fs::is_regular_file(txt_file))
                {
                  char          c = '\0';
                  std::ifstream infs_txt;
                  infs_txt.open(txt_file, std::ios::in);
                  if (infs_txt.is_open())
                  {
                    std::string line_txt;
                    std::string txt_file_content_s;

                    while (infs_txt.get(c) && !infs_txt.eof())
                    {
                      txt_file_content_s += c;

                    }; // read all file

                    infs_txt.close();

                    xml_template_node_content_s = Utils::replaceStringWithOtherString(xml_template_node_content_s, find_s, txt_file_content_s, true); // replace string but skip the first occurence since it will hold the find replace
                  }
                }
                else if (!replace_with_file_s.empty()) // v3.0.255.4.1 if file not exists then use the string as the replacer
                {
                  xml_template_node_content_s = Utils::replaceStringWithOtherString(xml_template_node_content_s, find_s, replace_with_file_s, true); // replace string but skip the first occurence since it will hold the find replace
                }


              } // end else if attrib values are valid

            } // end loop over all <find_replace>

          }   // end loop over all "multi option" container

          // Write to new template working file ?
          // after finishing the loop check if xml_file_content_s different than original_xml_file_content_s
          if ((xml_template_node_content_s != original_xml_template_node_content_s) * bDoneReplacement)
          {
            // write to tmp file
            fs::path output_file = tempFile_ptr->filePath + "/" + mxconst::get_TEMPLATE_INJECTED_FILE_NAME();

            // try to delete previous work file
            if (fs::exists(output_file) && fs::is_regular_file(output_file))
            {
              if (fs::remove(output_file))
              {
                Log::logMsgThread("[random] File: " + output_file.string() + ", deleted.");
              }
              else
              {
                Log::logMsgThread("[random] File: " + output_file.string() + ", failed to be removed.");

                // abort the running random engine
              }
            }

            // create new XML template file
            auto xTargetMainNode = IXMLNode::createXMLTopNode("xml", TRUE);
            xTargetMainNode.addAttribute(mxconst::get_ATTRIB_VERSION().c_str(), "1.0");
            xTargetMainNode.addAttribute("encoding", "ASCII"); // "ISO-8859-1");
            xTargetMainNode.addClear("\n\tFile has been created by Mission-X plug-in.\n\tAny modification might break or invalidate the file.\n\t", "<!--", "-->");

            // parse the template
            IXMLResults parse_result_strct;
            if ( auto newTemplate = this->iDomTemplate.parseString ( xml_template_node_content_s.c_str (), mxconst::get_ELEMENT_TEMPLATE().c_str (), &parse_result_strct ).deepCopy ()
                ; newTemplate.isEmpty())
            {
              const std::string translateError = IXMLRenderer::getErrorMessage(parse_result_strct.errorCode);
              Log::logMsgThread("[ERROR in Template]: \n===================>>\n" + xml_template_node_content_s + "\n<<===========================\n");
              Log::logMsgThread("[random] error in generated TEMPLATE element. " + translateError + ", line: " + mxUtils::formatNumber<long long>(parse_result_strct.nLine) + ", column: " + mxUtils::formatNumber<int>(parse_result_strct.nColumn) + " \n");
              this->setError("[random] TEMPLATE ERROR: modified template is not a valid XML. Check Log.txt for more information.");
              missionx::RandomEngine::threadState.flagAbortThread = true;
              this->abortThread();
            }
            else
            {
              // v24.12.2 deprecated code below, because it is used for debug purposes. I'll rewrite it to support the multi-option container
              //// v3.0.255.4.1 copy all attributes from <opt_name><info> over <mission_info>
              //if (optNode_ptr.nChildNode(mxconst::get_ELEMENT_INFO().c_str()) > 0)
              //{
              //  auto missionInfoNode_ptr = newTemplate.getChildNode(mxconst::get_ELEMENT_MISSION_INFO().c_str()); // v3.0.255.4.1
              //  if (!missionInfoNode_ptr.isEmpty())
              //  {
              //    auto infoNode = optNode_ptr.getChildNode(mxconst::get_ELEMENT_INFO().c_str());
              //    Utils::xml_copy_node_attributes_excluding_black_list(infoNode, missionInfoNode_ptr);
              //  }
              //}

              xTargetMainNode.addClear(" Modified Template ", "<!--", "-->");
              xTargetMainNode.addChild(newTemplate.deepCopy());

              xTargetMainNode.addClear(" Mapping ", "<!--", "-->");
              if (!xMappings.isEmpty())
                xTargetMainNode.addChild(xMappings);

              xTargetMainNode.addClear(" Replace Option ", "<!--", "-->");
              if (!xREPLACE_OPTIONS.isEmpty())
                xTargetMainNode.addChild(xREPLACE_OPTIONS);
            }

            IXMLErrorInfo result = xmlRender.writeToFile(xTargetMainNode, output_file.string().c_str());
            if (result != IXMLError_None)
            {
              const std::string translateError = IXMLRenderer::getErrorMessage(result);
              Log::logMsgThread("[random] error writing to template file. Error code: " + translateError);
            }

            return output_file.string(); // we always return the new template file since we want the designer to be aware of the errors including if file was not created.
          }
          else
          {
            Log::logMsgThread("[RandomEngine] Template file and working file content are the same.\n");
          }

        } // end if optNode is not Empty

      } // end if infs_xml is open

    } // end if original XML file exists

  } // end if tempInfo is valid pointer


  return "";
}

// -----------------------------------


bool
RandomEngine::generateRandomMission()
{
  auto        startThreadClock      = std::chrono::steady_clock::now ();
  double      duration              = 0.0;
  bool        flag_copy_leg_as_is_b = false; // v3.0.303 used with templates that has copy_leg_as_is_b="yes"
  std::string err;
  //// Thread initialization state
  missionx::RandomEngine::threadState.flagIsActive       = true;
  missionx::RandomEngine::threadState.flagThreadDoneWork = false;
  missionx::RandomEngine::threadState.flagAbortThread    = false;

  missionx::RandomEngine::threadState.startThreadStopper();

  this->flag_picked_from_osm_database = false; // v3.0.241.10
  bool        result                  = true;
  std::string pathToTemplateFile;
  pathToTemplateFile.clear();
  this->listNavInfo.clear();
  this->setInventories.clear();

  std::string inKey  = missionx::RandomEngine::threadState.dataString;

  /////////////////////////////////////////////////////////////////////
  ////// Read queries from external file //////////////////////////////

  #ifndef RELEASE
  Log::logAttention("\n=========>\n[random airport] Reading external queries", true);
  #endif

  this->initQueries(); // internal initialization so we will have a baseline to work with.

  err.clear(); // v3.0.223.1

  Utils::read_external_sql_query_file(missionx::data_manager::mapQueries, mxconst::get_SQLITE_OSM_SQLS());

  /////////// End read queries from external file /////////////
  ////////////////////////////////////////////////////////////



  #ifndef RELEASE
  Log::logAttention("\n=========>\n[random engine] start generating random mission", true);
  #endif
  if (this->working_tempFile_ptr == nullptr) // v3.0.241.9 work with pointer to File Information
  {
    this->setError("[Random]Failed to find template by the name: " + inKey); // this should be displayed
    missionx::RandomEngine::threadState.flagAbortThread = true;
    return false;
  }


  //// Use CACHED data or read from optimized apt dat file ////
  //// Read from cached file if our current cache is empty
  #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
  this->readOptimizedAptDatIntoCache();
  #endif


  //// set folders path ////
  this->pathToRandomRootFolder    = (this->working_tempFile_ptr->missionFolderName.empty()) ? data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONS_ROOT_PATH(), "", err) : this->working_tempFile_ptr->filePath; // v3.0.241.10 b2 decide from where to pick template file
  this->pathToRandomBrieferFolder = (this->working_tempFile_ptr->missionFolderName.empty())
                                      ? pathToRandomRootFolder + mxconst::get_FOLDER_SEPARATOR() + mxconst::get_FOLDER_RANDOM_MISSION_NAME() + mxconst::get_FOLDER_SEPARATOR() + mxconst::get_BRIEFER_FOLDER()
                                      : pathToRandomRootFolder + mxconst::get_FOLDER_SEPARATOR() + mxconst::get_BRIEFER_FOLDER(); // v3.0.241.10 b2 define the output folder of the template

  // store current plane coordinate
  this->planeLocation = missionx::dataref_manager::getCurrentPlanePointLocation();


  #ifndef RELEASE
  Log::logMsg("[random engine] Working on template: " + inKey + "\n<========", true);
  #endif

  // Read TEMPLATE xml file using DOM
  pathToTemplateFile = this->working_tempFile_ptr->getAbsoluteTemplateXmlFilePath();
  if (pathToTemplateFile.empty())
  {
    this->setError("[random engine] Failed generating mission using template: " + inKey);

    missionx::RandomEngine::threadState.flagAbortThread = true;
    return false;
  }

  ////// INJECT user option data ///////
  if ( ! this->working_tempFile_ptr->mapOptionsInfo.empty() ) // find/replace code
  {
    // 1. read user option and store the option name
    // 2. read the template file into a std::string
    // 3. Loop over all <find_replace> elements and replace all XX with the content of file YY
    const std::string newTemplateFile = this->inject_files_into_xml(this->working_tempFile_ptr); // v3.0.255.4
    if (!newTemplateFile.empty())
      pathToTemplateFile = newTemplateFile;

    if (missionx::RandomEngine::threadState.flagAbortThread)
      return false;
  }

  ////// READ MAPPING from template file /////////
  // Validate the <MAPPING> element do exists in template file
  missionx::data_manager::read_element_mapping(pathToTemplateFile); // v3.0.217.4
  if (missionx::data_manager::xmlMappingNode.isEmpty()) // v3.0.221.15rc3.4
  {
    this->setError("[random] ERROR: Mapping element is missing from template file: " + inKey + ". Fix template file. Aborting mission generating.");
    missionx::RandomEngine::threadState.flagAbortThread = true;

    return false;
  }


  //////// ========= READING FROM TEMPLATE ============= /////////////////
  // read mission template
  err.clear(); // v3.0.223.1
  ITCXMLNode xTemplateNode = this->iDomTemplate.openFileHelper(pathToTemplateFile.c_str(), mxconst::get_TEMPLATE_ROOT_DOC().c_str(), &err); // parse xml into ITCXMLNode
  this->xRootTemplate      = xTemplateNode.deepCopy(); // convert ITCXMLNode to IXMLNode. IXMLNode allow to modify itself
  if (!err.empty() && this->xRootTemplate.isEmpty())   // check if there is any failure during read
  {
    this->setError(err);

    missionx::RandomEngine::threadState.flagAbortThread = true;

    return false;
  }

  bool flag_created_based_on_content_element = false; // moved the bool declaration here to solve the cross compile issue in gcc and the goto command.

  /////// =================== Prepare TARGET XML Nodes ===================================================
  //// add MISSION root node
  xDummyTopNode = xTargetMainNode.addChild(mxconst::get_MISSION_ELEMENT().c_str());
  xDummyTopNode.addAttribute(mxconst::get_ATTRIB_VERSION().c_str(), missionx::PLUGIN_FILE_VER);

  if (this->working_tempFile_ptr->missionFolderName.empty())
    xDummyTopNode.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), this->working_tempFile_ptr->fileName.c_str()); // v3.0.241.9 replace direct map with pointer to TemplateFileInfo
  else
    xDummyTopNode.addAttribute (mxconst::get_ATTRIB_NAME().c_str (), this->working_tempFile_ptr->missionFolderName.c_str ()); // v3.0.241.10 b2 handle cases where we read template from custom folder. We need to use the folder name and not template filename which might not be unique


  // prepare main elements for generated mission file.
  this->xMetadata   = xDummyTopNode.addChild(mxconst::get_ELEMENT_METADATA().c_str()); // v24.12.1
  this->xFlightLegs = xDummyTopNode.addChild(mxconst::get_ELEMENT_FLIGHT_PLAN().c_str());
  this->xObjectives = xDummyTopNode.addChild(mxconst::get_ELEMENT_OBJECTIVES().c_str());
  this->xTriggers   = xDummyTopNode.addChild(mxconst::get_ELEMENT_TRIGGERS().c_str());
  this->xInventoris = xDummyTopNode.addChild(mxconst::get_ELEMENT_INVENTORIES().c_str());
  this->xGPS        = xDummyTopNode.addChild(mxconst::get_ELEMENT_GPS().c_str());
  this->xChoices    = xDummyTopNode.addChild(mxconst::get_ELEMENT_CHOICES().c_str());

  Utils::xml_delete_all_subnodes(xRootTemplate, mxconst::get_ELEMENT_METADATA(), true); // v24.12.1
  xRootTemplate.addChild(xMetadata); // v24.12.1 I add the xMetadata to the xRootTemplate, since I use it in many function and so we have access to it from the Node instead from "this->".

  // read Object Template node from template
  x3DObjTemplate = xRootTemplate.getChildNode(mxconst::get_ELEMENT_OBJECT_TEMPLATES().c_str()).deepCopy(); // v3.0.217.6
  if (x3DObjTemplate.isEmpty())
    x3DObjTemplate = xTargetMainNode.addChild(mxconst::get_ELEMENT_OBJECT_TEMPLATES().c_str()); // v3.0.219.1

  // read message templates
  IXMLNode xMessageTemplates = xRootTemplate.getChildNode(mxconst::get_ELEMENT_MESSAGE_TEMPLATES ().c_str()).deepCopy(); // v3.0.223.4
  if (!xMessageTemplates.isEmpty())
    xMessages = xMessageTemplates;
  else
    xMessages = xDummyTopNode.addChild(mxconst::get_ELEMENT_MESSAGE_TEMPLATES ().c_str());

  Log::logDebugBO("[DEBUG random airport] After preparing new mission file main nodes.", true);

  // Which Mission Type ?
  auto med_cargo_or_oilrig_i = Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int> ( missionx::mx_mission_type::medevac ) ); // 0 = med, 1 = cargo

  // --------------- Special cases ------------------------------------
  /// Handle special template cases: "user_driven_mission_layer" or Layer: "option_external_fpln_layer" is handled later on
  if (this->flag_rules_defined_by_user_ui)
  {
    // call function that injects all the legs skeleton
    if (data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_user_generates_a_mission_layer)
    {
      std::string local_err;
      // v24.12.1 Add metadata child info
      assert(xMetadata.isEmpty() == false && fmt::format("{}, <{}> is invalid. Creation failure. Notify developer.", __func__, mxconst::get_ELEMENT_METADATA()).c_str());
      // set the metadata attributes

      #ifndef RELEASE
        Log::logMsgThread( fmt::format(R"(Before calling "prepare_blank_template_with_flight_legs_based_on_ui":\n{})", Utils::xml_get_node_content_as_text ( xRootTemplate ) ) );
      #endif

      if (!this->prepare_blank_template_with_flight_legs_based_on_ui(xRootTemplate, this->xMetadata, local_err)) // TODO: consider removing the xMetadata, it is not being used in the function
      {
        this->setError(local_err);
        missionx::RandomEngine::threadState.flagAbortThread = true;

        #ifndef RELEASE
        IXMLRenderer xmlWriter;
        Log::logMsg("\n ============== Using user defined TEMPLATE ============== \n" + Utils::xml_get_node_content_as_text (xRootTemplate) + "\n\n========= END USER GENERATED TEMPLATE ===========\n\n", true);
        xmlWriter.clear();
        #endif
      }

      // check oilrig
      if (med_cargo_or_oilrig_i == static_cast<int>(missionx::mx_mission_type::oil_rig))
      {
        if (!this->prepare_mission_based_on_oilrig(xRootTemplate, local_err))
        {
          this->setError(local_err);
          missionx::RandomEngine::threadState.flagAbortThread = true;
        }
        else
        {
          //goto post_mission_action;
        }

      } // end if oilrig mission

    } // end handling user_driven_mission_layer

    else if (data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_external_fpln_layer)
    {
      if (!this->prepare_mission_based_on_external_fpln(xRootTemplate))
      {
        this->setError("Failed to build mission based on external FPLN !!!");
        missionx::RandomEngine::threadState.flagAbortThread = true;
      }
      else
      {
        goto post_mission_action;
      }
    }
    else if (data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_ils_layer)
    {
      if (!this->prepare_mission_based_on_ils_search(xRootTemplate))
      {
        this->setError("Failed to build mission based on ILS FPLN !!!");
        missionx::RandomEngine::threadState.flagAbortThread = true;
      }
      else
      {
        goto post_mission_action;
      }
    }
    else if (data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::flight_leg_info)
    {
      if (!this->prepare_mission_based_on_user_fpln_or_simbrief(xRootTemplate))
      {
        this->setError("Failed to build mission based on User/Simbrief FPLN !!!");
        missionx::RandomEngine::threadState.flagAbortThread = true;
      }
      else
      {
        goto post_mission_action;
      }
    }

  }

  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;


  // v3.0.219.1
  this->parse_3D_object_template_element(); // go over <object_template> and resolve any <random_tag> before we will parse <display_object>
  ///// =========================================================================================

  Log::logDebugBO("[DEBUG random airport] After preparing new mission file main nodes.", true);
  this->setPlaneType(Utils::stringToLower(Utils::readAttrib(this->xRootTemplate, mxconst::get_ATTRIB_PLANE_TYPE(), mxconst::get_PLANE_TYPE_HELOS()))); // v3.0.221.15 Default plane is Helicopter.
  Log::logDebugBO("[DEBUG random airport] After <briefer_info> node.", true);

  ///// We should read all tags and pick randomly from them.
  ///// If content available then we will call "generateRandomMissionBasedOnContent()" function

  { // added anonymous block to solve the cross compiling of nContentChilds_i with the "goto command conflict"
    if ( int nContentChilds_i = this->xRootTemplate.getChildNode ( mxconst::get_ELEMENT_CONTENT().c_str () ).nChildNode ()
        ; nContentChilds_i > 0)
    {
      if (!this->generateRandomMissionBasedOnContent(this->xRootTemplate))
      {
        missionx::RandomEngine::threadState.flagAbortThread = true;
        return false;
      }

      flag_created_based_on_content_element = true;
      flag_copy_leg_as_is_b = Utils::readBoolAttrib(this->xRootTemplate, mxconst::get_ATTRIB_COPY_LEG_AS_IS_B(), false);
    }
    else // else: readFlightLegs_directlyFromTemplate(). This is the "simple" template code, that reads flight legs sequentially
    {

      // read briefer element before calling "readFlightLegs_directlyFromTemplate()"
      if (missionx::RandomEngine::threadState.flagAbortThread)
        return false;
      else if (!readBrieferAndStartLocation()) // main <briefer>
      {
        missionx::RandomEngine::threadState.flagAbortThread = true;
        return false;
      }

      Log::logDebugBO("[DEBUG random airport] After <briefer_and_start_location> node.", true);

      // call "readFlightLegs_directlyFromTemplate()"
      if (!readFlightLegs_directlyFromTemplate()) // v3.0.217.8
      {
        missionx::RandomEngine::threadState.flagAbortThread = true;
        return false;
      }
    }
  }

  // POST_MISSION_ACTIONS:
post_mission_action:

  // call readMissionInfoElement // v3.0.253.1 moved to this location so fetch external code will create briefer info too.
  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;
  if ( !flag_created_based_on_content_element && !readMissionInfoElement () ) // we can skip this function call if we built the mission based on content element. We need to read it inside content to have the custom <overpass> element from <mission_info>
    return false;

  Utils::xml_delete_empty_nodes(xDummyTopNode); // v3.0.219.3 remove invalid points

  // Check last 2 legs are not the same ICAO. Will remove the last one.
  this->check_last_2_legs_if_they_have_same_icao(); // v3.0.255.1

  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  // v25.02.1 added suppress message, setup, option
  if ( const auto bSuppressDistanceMessages = Utils::getNodeText_type_1_5<bool> ( system_actions::pluginSetupOptions.node, mxconst::get_ATTRIB_SUPPRESS_DISTANCE_MESSAGES_B(), false )
            ; !bSuppressDistanceMessages
              && data_manager::getGeneratedFromLayer() != missionx::uiLayer_enum::option_external_fpln_layer
              && data_manager::getGeneratedFromLayer() != missionx::uiLayer_enum::option_ils_layer
              && !flag_copy_leg_as_is_b)
  {
    this->injectMessagesWhileFlyingToDestination ();
  }

  Log::logDebugBO("[DEBUG random airport] After <message_templates> node.", true);

  // inject specific mission type data
  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  if ( data_manager::getGeneratedFromLayer () != missionx::uiLayer_enum::option_external_fpln_layer
    && data_manager::getGeneratedFromLayer () != missionx::uiLayer_enum::option_ils_layer
    && data_manager::getGeneratedFromLayer () != missionx::uiLayer_enum::flight_leg_info // v25.03.3
    && !flag_copy_leg_as_is_b )
    this->injectMissionTypeFeatures ();

  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  this->injectCountdownTimers();


  // v3.0.221.10 Add <xpdata> element if exists
  if (xRootTemplate.nChildNode(mxconst::get_ELEMENT_XPDATA().c_str()) > 0)
    this->xpData = xRootTemplate.getChildNode(mxconst::get_ELEMENT_XPDATA().c_str());

  if (xRootTemplate.nChildNode(mxconst::get_ELEMENT_EMBEDDED_SCRIPTS().c_str()) > 0)
    this->xEmbedScripts = xRootTemplate.getChildNode(mxconst::get_ELEMENT_EMBEDDED_SCRIPTS().c_str());

  this->xScoring = xRootTemplate.getChildNode(mxconst::get_ELEMENT_SCORING().c_str()).deepCopy(); // v3.303.9
  this->xCompatibility = xRootTemplate.getChildNode(mxconst::get_ELEMENT_COMPATIBILITY().c_str()).deepCopy(); // v24.12.2

  // v3.0.303 add objectives and triggers if we had a content with copy_leg_as_is_b
  if (flag_copy_leg_as_is_b)
  {
    // v3.303.8 make sure Embedded/Script is available and also template name is from template itself, good for distinguishing between different template choices
    if (this->xEmbedScripts.isEmpty())
      this->xEmbedScripts = Utils::xml_get_or_create_node_ptr(xRootTemplate, mxconst::get_ELEMENT_EMBEDDED_SCRIPTS());

    const std::string template_name = Utils::readAttrib(xRootTemplate, mxconst::get_ATTRIB_NAME(), "");
    if (!template_name.empty())
      this->xDummyTopNode.updateAttribute(template_name.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());

    const std::string title_text = Utils::readAttrib(xRootTemplate, mxconst::get_ATTRIB_TITLE(), "");
    if (!title_text.empty())
      this->xDummyTopNode.updateAttribute(title_text.c_str(), mxconst::get_ATTRIB_TITLE().c_str(), mxconst::get_ATTRIB_TITLE().c_str());

    // add all objectives
    auto vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName(xRootTemplate, mxconst::get_ELEMENT_OBJECTIVES());
    for (auto &node : vecNodes)
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode(this->xObjectives, node, mxconst::get_ELEMENT_OBJECTIVE(), true);


    // add all triggers
    vecNodes.clear();
    vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName(xRootTemplate, mxconst::get_ELEMENT_TRIGGERS());
    for (auto &node : vecNodes)
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode(this->xTriggers, node, mxconst::get_ELEMENT_TRIGGER(), true);


    // Add <message_templates> // v3.303.8
    vecNodes.clear();
    vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName(xRootTemplate, mxconst::get_ELEMENT_MESSAGE ());
    for (auto &node : vecNodes)
      Utils::xml_add_node_to_parent_with_duplicate_filter(this->xMessages, node, mxconst::get_ELEMENT_MESSAGE (), mxconst::get_ATTRIB_NAME());

    // Add <scriptlet> // v3.303.8
    vecNodes.clear();
    vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName(xRootTemplate, mxconst::get_ELEMENT_SCRIPTLET());
    for (auto &node : vecNodes)
      Utils::xml_add_node_to_parent_with_duplicate_filter(this->xEmbedScripts, node, mxconst::get_ELEMENT_SCRIPTLET(), mxconst::get_ATTRIB_NAME() );

    // Add GPS
    for (int i1 = 0; i1 < xRootTemplate.nChildNode(mxconst::get_ELEMENT_GPS().c_str()); ++i1)
    {
      auto node = xRootTemplate.getChildNode(mxconst::get_ELEMENT_GPS().c_str(), i1);
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode(this->xGPS, node, mxconst::get_ELEMENT_POINT(), true);
    }
    // Add Inventory
    for (int i1 = 0; i1 < xRootTemplate.nChildNode(mxconst::get_ELEMENT_INVENTORIES().c_str()); ++i1)
    {
      auto node = xRootTemplate.getChildNode(mxconst::get_ELEMENT_INVENTORIES().c_str(), i1);
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode(this->xInventoris, node, mxconst::get_ELEMENT_INVENTORY(), true);
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode(this->xInventoris, node, mxconst::get_ELEMENT_PLANE(), true);
    }
    // add all 3D object to <object_template> // v3.0.303.2 starting from second element since the first element is always this->x3DObjTemplate that way we won't have duplication of <obj3d> elements.
    for (int i1 = 1; i1 < xRootTemplate.nChildNode(mxconst::get_ELEMENT_OBJECT_TEMPLATES().c_str()); ++i1)
    {
      auto node = xRootTemplate.getChildNode(mxconst::get_ELEMENT_OBJECT_TEMPLATES().c_str(),i1);
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode(this->x3DObjTemplate, node, mxconst::get_ELEMENT_OBJ3D(), true);
    }

    // add <choices>
    for (int i1 = 0; i1 < xRootTemplate.nChildNode(mxconst::get_ELEMENT_CHOICES().c_str()); ++i1)
    {
      auto node = xRootTemplate.getChildNode(mxconst::get_ELEMENT_CHOICES().c_str(),i1);
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode(this->xChoices, node, mxconst::get_ELEMENT_CHOICE(), true);
    }

    // <global_settings>
    this->xGlobalSettings = xRootTemplate.getChildNode(mxconst::get_GLOBAL_SETTINGS().c_str()).deepCopy();
  }

  // Write to file
  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  if ( int nFlightLegs = this->xFlightLegs.nChildNode ( mxconst::get_ELEMENT_LEG().c_str () )
      ; nFlightLegs == 0)
  {
    this->setError("[random] No flight leg has been created. Try to re-generate a mission, tweak the template or re-run APT.DAT optimization (setup screen).");
    this->abortThread();
    return false;
  }
  else
  {
    // v3.0.221.7 set xBriefer starting flight leg
    IXMLNode xml_leg_node = this->xFlightLegs.getChildNode(mxconst::get_ELEMENT_LEG().c_str());
    if (!xml_leg_node.isEmpty())
    {
      std::string firstLegName = Utils::readAttrib(xml_leg_node, mxconst::get_ATTRIB_NAME(), "");
      if (firstLegName.empty())
      {
        this->setError("[random] Fail to find name of starting <leg> Try to re-generate a mission or tweak the template.");
        this->abortThread();
        return false;
      }

      xBriefer.updateAttribute(firstLegName.c_str(), mxconst::get_ATTRIB_STARTING_LEG().c_str(), mxconst::get_ATTRIB_STARTING_LEG().c_str());
      Log::logDebugBO("[random before write]Set starting <leg> >> " + firstLegName + " << ", true);
    }
  }

  // store plane type // v24.12.1
  if (!this->xMetadata.isEmpty())
  {
    // plane type
    xMetadata.updateAttribute(Utils::readAttrib(this->xRootTemplate, mxconst::get_ATTRIB_PLANE_TYPE(), "").c_str(), mxconst::get_ATTRIB_PLANE_TYPE().c_str(), mxconst::get_ATTRIB_PLANE_TYPE().c_str());
    // ui layer (code) from where mission was generated
    xMetadata.updateAttribute(mxUtils::formatNumber<int>(static_cast<int> (data_manager::getGeneratedFromLayer ())).c_str(), mxconst::get_ATTRIB_UI_LAYER().c_str(), mxconst::get_ATTRIB_UI_LAYER().c_str());
  }

  result = writeTargetFile();

  auto endCacheLoad = std::chrono::steady_clock::now();
  auto diff_cache   = endCacheLoad - startThreadClock;
  duration          = std::chrono::duration<double, std::milli>(diff_cache).count();
  Log::logAttention("*** Finished Generating RANDOM Mission, Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec)  ****", true);

  /// finalize thread
  missionx::RandomEngine::threadState.flagIsActive       = false;
  missionx::RandomEngine::threadState.flagThreadDoneWork = true; // we reset the thread at Mission::flc_aptdat() function

  return result;
}

// -----------------------------------

// return true, means main plugin thread finished job before limit.
bool
RandomEngine::waitForPluginCallbackJob(missionx::mx_flc_pre_command inQueuedCommand, std::chrono::milliseconds inWaitTimeMilliseconds, int inLimitWaitCounter)
{
  int waitCounter = 0;

  missionx::RandomEngine::threadState.thread_wait_state = missionx::mx_random_thread_wait_state_enum::waiting_for_plugin_callback_job; // set busy state
  missionx::data_manager::queFlcActions.emplace(inQueuedCommand);  // provide the queued command

  // wait for the queued command to finish
  #ifndef RELEASE
  while (missionx::RandomEngine::threadState.thread_wait_state < missionx::mx_random_thread_wait_state_enum::finished_plugin_callback_job
         && !missionx::RandomEngine::threadState.flagAbortThread && missionx::RandomEngine::threadState.flagIsActive)
  #else
  while (missionx::RandomEngine::threadState.thread_wait_state < missionx::mx_random_thread_wait_state_enum::finished_plugin_callback_job
    && (waitCounter < inLimitWaitCounter) && !missionx::RandomEngine::threadState.flagAbortThread && missionx::RandomEngine::threadState.flagIsActive)
  #endif
  {
    ++waitCounter;
    std::this_thread::sleep_for(std::chrono::milliseconds(inWaitTimeMilliseconds));
  }

  missionx::RandomEngine::threadState.thread_wait_state = missionx::mx_random_thread_wait_state_enum::not_waiting; // reset state

  if (waitCounter >= inLimitWaitCounter || missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  return true;
}

// -----------------------------------

bool
RandomEngine::readMissionInfoElement()
{
  bool result = true;


  this->xBrieferInfo = xRootTemplate.getChildNode(mxconst::get_ELEMENT_MISSION_INFO().c_str()).deepCopy();
  if (this->xBrieferInfo.isEmpty())
  {
    this->setError("No <" + mxconst::get_ELEMENT_MISSION_INFO() + "> was found. Template malformed, abort template generation.");
    return false;
  }

  // override mission image file with random.png
  if (this->working_tempFile_ptr->missionFolderName.empty())
    Utils::xml_set_attribute_in_node_asString(xBrieferInfo, mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME(), mxconst::get_DEFAULT_RANDOM_IMAGE_FILE(), mxconst::get_ELEMENT_MISSION_INFO()); // v3.0.217.6
  else
  {
    const std::string imageFileName_s = Utils::readAttrib(xBrieferInfo, mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME(), this->working_tempFile_ptr->getTemplateImageFileName()); // v3.0.241.1
    Utils::xml_set_attribute_in_node_asString(xBrieferInfo, mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME(), imageFileName_s, mxconst::get_ELEMENT_MISSION_INFO());                     // v3.0.217.6
  }

  // add template file name to other settings
  // v25.02.1
  // const std::filesystem::path pth = (this->working_tempFile_ptr != nullptr)? this->working_tempFile_ptr->fullFilePath : "";
  // const std::string template_name = pth.filename().string();
  const std::string template_name = (this->working_tempFile_ptr != nullptr)? std::filesystem::path(this->working_tempFile_ptr->fullFilePath).filename().string(): "";

  std::string other_settings = Utils::readAttrib(xBrieferInfo, mxconst::get_ATTRIB_OTHER_SETTINGS(), ""); // v3.0.241.1
  other_settings             = "Based on: " + ((!template_name.empty())? template_name : "Error: No Template Data") + ". " + other_settings;
  Utils::xml_set_attribute_in_node_asString(xBrieferInfo, mxconst::get_ATTRIB_OTHER_SETTINGS(), other_settings, mxconst::get_ELEMENT_MISSION_INFO()); // v3.0.217.6

  this->errMsg.clear();

  // v3.0.255.4.2 parse mission info designer overpass urls that will override the plugin own list of urls
  this->vecMissionInfoOverpassUrls.clear(); // v3.0.255.4.2
  const auto urls_i = this->xBrieferInfo.getChildNode(mxconst::get_ELEMENT_OVERPASS().c_str()).nChildNode(mxconst::get_ELEMENT_URL().c_str());
  for (int i1 = 0; i1 < urls_i; ++i1)
  {
    if (std::string text = this->xBrieferInfo.getChildNode(mxconst::get_ELEMENT_OVERPASS().c_str()).getChildNode(mxconst::get_ELEMENT_URL().c_str(), i1).getText()
      ; !text.empty())
      this->vecMissionInfoOverpassUrls.emplace_back(text);
  }
  this->current_url_indx_used_i = (this->vecMissionInfoOverpassUrls.empty()) ? mxconst::INT_UNDEFINED : 0;



  return result;
}

// -----------------------------------

bool
RandomEngine::readBrieferAndStartLocation()
{
  // We will use this function to construct the "<briefer>" element.
  // The description of the mission in the briefer we will fetch from: "<mission_info>" CDATA property.
  IXMLNode xPoint = data_manager::xmlMappingNode.getChildNode(mxconst::get_ELEMENT_POINT().c_str()).deepCopy(); // v3.0.219.7 added for injectInventory() later.
  if (xPoint.isEmpty())
  {
    xPoint = data_manager::xmlMappingNode.addChild(mxconst::get_ELEMENT_POINT().c_str());
    xPoint.addAttribute(mxconst::get_ATTRIB_LAT().c_str(), "");
    xPoint.addAttribute(mxconst::get_ATTRIB_LONG().c_str(), "");
  }
  std::string lat_s, lon_s, icao_s; // will hold string representation of longitude and latitude

  bool result = true;

  IXMLNode xLocationAdjust = xRootTemplate.getChildNode(mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION().c_str()).deepCopy();
  if (xLocationAdjust.isEmpty())
  {
    this->setError("[random] No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION() + "> was found. Template malformed, abort template generation !!!");

    return false;
  }

  // rename element tag to <location_adjust >
  xLocationAdjust.updateName(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str());

  // store element properties in "elementBrieferInfoProperties" for internal use, if needed to remove any "clear" data
  int         nClear      = xLocationAdjust.nClear();                        // remove any CDATA or COMMENTS or any clear() type element
  std::string brieferDesc = Utils::xml_read_cdata_node(xLocationAdjust, ""); // v3.0.241.1 // v3.0.241.9 replace default string with empty string
  for (int i = 0; i < nClear; ++i)
    xLocationAdjust.deleteClear(); // v3.0.241.1 change from remove "i" to remove first

  //// Handle location_type
  //bool        flag_found         = false;
  std::string locationOptionType = Utils::readAttrib(xLocationAdjust, mxconst::get_ATTRIB_LOCATION_TYPE(), mxconst::get_ELEMENT_PLANE());
  if (locationOptionType.empty())
    locationOptionType = mxconst::get_ELEMENT_PLANE();

  ////////////////////
  // if value = plane
  if (mxconst::get_ELEMENT_PLANE() == locationOptionType || get_user_wants_to_start_from_plane_position()) // v3.0.253.11 added prop_start_from_plane_position
  {
    // set xPoint from plane // v3.0.219.7
    lat_s = Utils::formatNumber<double>(this->planeLocation.getLat(), 8);
    lon_s = Utils::formatNumber<double>(this->planeLocation.getLon(), 8);
    xPoint.updateAttribute(lat_s.c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
    xPoint.updateAttribute(lon_s.c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());

    xLocationAdjust.updateAttribute(lat_s.c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
    xLocationAdjust.updateAttribute(lon_s.c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());
    xLocationAdjust.updateAttribute(Utils::formatNumber<double>(this->planeLocation.getElevationInFeet(), 2).c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str());
    xLocationAdjust.updateAttribute(Utils::formatNumber<double>(this->planeLocation.getHeading(), 2).c_str(), mxconst::get_ATTRIB_HEADING_PSI().c_str(), mxconst::get_ATTRIB_HEADING_PSI().c_str());
  }
  ////////////////////
  // if value = xy
  else if (mxconst::get_EXPECTED_LOCATION_TYPE_XY() == locationOptionType) // if value = xy
  {
    // check it targetLat/long are set, if yes, then use them
    // if not then check if "random" exists and if its value is not empty. then read the element with points and randomly pick a point.
    // read targetLat/long and see if they are pre-defined from template.

    lat_s  = Utils::readAttrib(xLocationAdjust, mxconst::get_ATTRIB_LAT(), "");
    lon_s  = Utils::readAttrib(xLocationAdjust, mxconst::get_ATTRIB_LONG(), "");
    icao_s = Utils::readAttrib(xLocationAdjust, mxconst::get_ATTRIB_ICAO_ID(), ""); // v3.303.14

    if (!lat_s.empty() && !lon_s.empty())
    { // we will use current targetLat/long stored in elementStartLocationProperties
      Log::logMsgThread("[random] will set start location based on pre-defined location provided in template.");

      // set xPoint from plane // v3.0.219.7
      xPoint.updateAttribute(lat_s.c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
      xPoint.updateAttribute(lon_s.c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());
      xPoint.updateAttribute(icao_s.c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str());
    }
    else // try to use "location_value_nm_s" property and fetch a point based on a list of points provided ad-hock
    {
      std::string location_xy_random_value = Utils::readAttrib(xLocationAdjust, mxconst::get_ATTRIB_LOCATION_VALUE(), "");
      if (location_xy_random_value.empty() || Utils::is_number(location_xy_random_value)) // SHOULD NOT BE EMPTY OR A NUMBER.
      {
        this->setError("[random] Failed to find valid starting location, No Coordinates or string List of random latitude/longitude were provided, will abort template creation. Please fix the template or change the starting location to plane.");
        return false;
      }
      else
      { // read random element

        IXMLNode xLocationNodePtr = this->xRootTemplate.getChildNode(location_xy_random_value.c_str());
        if (xLocationNodePtr.isEmpty())
        {
          this->setError("[random] Failed to read random element: <" + location_xy_random_value + ">. Please fix the template, aborting random creation.");
          return false;
        }

        // v3.0.221.5 call convert <icao> to <points>
        this->shared_navaid_info.parentNode_ptr = xLocationNodePtr;                               // store pointer to XML node
        this->waitForPluginCallbackJob(missionx::mx_flc_pre_command::convert_icao_to_xml_point); // will call missionx::flcPRE() and try to convert any <icao name="icao name" /> to <point targetLat="" targetLon="" />
        // end v3.0.221.5 conversion

        xPoint = Utils::xml_get_node_randomly_by_name_IXMLNode(xLocationNodePtr, mxconst::get_ELEMENT_POINT(), this->errMsg);
        if (!this->errMsg.empty())
        {
          this->setError(this->errMsg);
          return false;
        }

        if (!xPoint.isEmpty())
        {
          missionx::NavAidInfo navAid;
          navAid.node = xPoint.deepCopy();
          navAid.syncXmlPointToNav();

          // #ifndef RELEASE
          //   missionx::NavAidInfo navAid2 = navAid;
          //   navAid2.node                 = xPoint.deepCopy();
          //   navAid2.syncXmlPointToNav();
          //   //this->filterAndPickRampBasedOnPlaneType(navAid2, this->errMsg);
          //   this->filterAndPickRampBasedOnPlaneType2(navAid2, this->errMsg, missionx::mxFilterRampType::start_ramp);
          //   Log::logMsgThread("ramp: " + navAid2.getName());
          // #endif // !RELEASE



          // try to get Navaid information for briefer. If we fail to find information, we ignore and continue with original xPoint data
          //if (this->filterAndPickRampBasedOnPlaneType(navAid, this->errMsg))
          if (this->filterAndPickRampBasedOnPlaneType(navAid, this->errMsg, missionx::mxFilterRampType::start_ramp ) )
          {
            xPoint = navAid.node.deepCopy();
            if (xPoint.isEmpty())
            {
              this->setError("[random] Fail to read filtered briefer starting point. Aborting... notify developer");
              return false;
            }
          }
          this->errMsg.clear();

          lat_s  = Utils::readAttrib(xPoint, mxconst::get_ATTRIB_LAT(), missionx::EMPTY_STRING);
          lon_s  = Utils::readAttrib(xPoint, mxconst::get_ATTRIB_LONG(), missionx::EMPTY_STRING);
          const std::string elev_s = Utils::readAttrib(xPoint, mxconst::get_ATTRIB_ELEV_FT(), missionx::EMPTY_STRING);

          if (lat_s.empty() || lon_s.empty())
          {
            this->setError("[random] Point data does not have mandatory attributes: '" + mxconst::get_ATTRIB_LAT() + "' and '" + mxconst::get_ATTRIB_LONG() + "'. Please fix template. Aborting...");
            return false;
          }

          // set start location "targetLat/long/elev_ft
          Utils::xml_search_and_set_attribute_in_IXMLNode(xLocationAdjust, mxconst::get_ATTRIB_LAT(), lat_s, mxconst::get_ELEMENT_LOCATION_ADJUST());
          Utils::xml_search_and_set_attribute_in_IXMLNode(xLocationAdjust, mxconst::get_ATTRIB_LONG(), lon_s, mxconst::get_ELEMENT_LOCATION_ADJUST());
          Utils::xml_search_and_set_attribute_in_IXMLNode(xLocationAdjust, mxconst::get_ATTRIB_ELEV_FT(), elev_s, mxconst::get_ELEMENT_LOCATION_ADJUST());

        } // end reading random <point> element

      } // end using <location_value_nm_s> element to choose starting location

    } // end if targetLat/long were defined or based on location_value_nm_s element

  } // end construct <start_location> based on "xy" (pre-defined targetLat/long or based on ad-hock starting points that we will pick in random


  if (lat_s.empty() || lon_s.empty()) // v3.0.219.6
  {
    this->setError("[random briefer] Something is wrong with location definition, fix template or notify developer if no solution could be found.");
    return false;
  }


  if (!xPoint.isEmpty())
  {
    this->lastFlightLegNavInfo.node = xPoint.deepCopy();
    this->lastFlightLegNavInfo.syncXmlPointToNav();
    this->lastFlightLegNavInfo.flightLegName = mxconst::get_ELEMENT_BRIEFER();

    // search for nearest ICAO or bounding airport relative to plane starting position using the SQLITE database
    this->shared_navaid_info.navAid.init();
    this->shared_navaid_info.navAid = missionx::data_manager::getPlaneAirportOrNearestICAO(true, lastFlightLegNavInfo.lat, lastFlightLegNavInfo.lon, true); // v3.303.14
    if (!this->shared_navaid_info.navAid.getID().empty() )
    {
      // if plane is in bounding area then we are good
      lastFlightLegNavInfo.setName(this->shared_navaid_info.navAid.getNavAidName());
      lastFlightLegNavInfo.setID(this->shared_navaid_info.navAid.getID());
      lastFlightLegNavInfo.height_mt = this->shared_navaid_info.navAid.height_mt;
      lastFlightLegNavInfo.synchToPoint();
      xPoint = lastFlightLegNavInfo.node.deepCopy(); // override xPoint with the added information
    }
    else
    { // try to pick the nearest airport using the "XPLMFindNavAid()" function.

      this->shared_navaid_info.navAid.init();
      // v3.0.221.11 try to get airport information
      if (this->waitForPluginCallbackJob(missionx::mx_flc_pre_command::get_nearest_nav_aid_to_randomLastFlightLeg_mainThread))
      {
        // check distance and hopefully pick correct airport. Since we are using a fixed distance this might not be a 100% guaranty
        this->shared_navaid_info.navAid.synchToPoint();
        if ( double dist = missionx::Point::calcDistanceBetween2Points ( this->shared_navaid_info.navAid.p, lastFlightLegNavInfo.p )
          ; dist <= 1.2 && !this->shared_navaid_info.navAid.getID().empty())
        {
          lastFlightLegNavInfo.setName(this->shared_navaid_info.navAid.getNavAidName());
          lastFlightLegNavInfo.setID(this->shared_navaid_info.navAid.getID());
          lastFlightLegNavInfo.height_mt = this->shared_navaid_info.navAid.height_mt;
          lastFlightLegNavInfo.synchToPoint();
        }

        xPoint = lastFlightLegNavInfo.node.deepCopy(); // store in xPoint again
      }
      else
      {
        Log::logMsgWarn("[random briefer] Failed to find Airport NEAR given start location. ", true);
      }

    }
    lastFlightLegNavInfo.flag_is_brieferOrStartLocation = true; // v3.303.10
    lastFlightLegNavInfo.synchToPoint();

    this->listNavInfo.emplace_back(lastFlightLegNavInfo); // add NavInfo into a list

    // Add to GPS
    if (!xGPS.isEmpty())
      xGPS.addChild(xPoint.deepCopy());
  }
  else
  {
    this->setError("[random briefer] Something is wrong with location definition, fix template or notify developer if no solution could be found.");
    return false;
  }

  // create most of the <briefer> node. we will finish it once we will have all Flight Legs
  this->xBriefer = this->xDummyTopNode.addChild(mxconst::get_ELEMENT_BRIEFER().c_str());
  this->xBriefer.addAttribute(mxconst::get_ATTRIB_STARTING_LEG().c_str(), "leg_1"); // leg_1 is default value, but it can be changed when using <content> elements with "Flight Leg sets"
  this->xBriefer.updateAttribute(Utils::readAttrib(xLocationAdjust, mxconst::get_ATTRIB_STARTING_ICAO(), "").c_str(), mxconst::get_ELEMENT_ICAO().c_str(), mxconst::get_ELEMENT_ICAO().c_str());  // v3.303.8.2 copy starting_icao to briefer element
  this->xBriefer.updateAttribute(Utils::readAttrib(xLocationAdjust, mxconst::get_ATTRIB_POSITION_PREF(), "").c_str(), mxconst::get_ATTRIB_POSITION_PREF().c_str(), mxconst::get_ATTRIB_POSITION_PREF().c_str()); // v3.0.301 B4 Copy the attribute value from <briefer_and_start_location> - position_pref

  IXMLNode cNode = xBriefer.addChild(xLocationAdjust); // add <location_adjust> to briefer
  Utils::xml_add_cdata(this->xBriefer, brieferDesc); // v3.0.241.1

  // v3.0.219.7 Add inventory if exists in mapping
  if (data_manager::xmlMappingNode.nChildNode(mxconst::get_ELEMENT_INVENTORY().c_str()) > 0)
  {
    this->addInventory(mxconst::get_ELEMENT_BRIEFER(), xPoint, mxInvSource::point); // name of store located at the start location
  }


  return result;
}


// -----------------------------------
// -----------------------------------
// -----------------------------------

IXMLNode
RandomEngine::get_skewed_target_position (const IXMLNode & inRealTargetPositionPoint)
{
  double   outLat, outLon = 0.0; // result from calculation
  IXMLNode skewedPosition = inRealTargetPositionPoint.deepCopy();

  constexpr double MIN_AWAY_DISTANCE = 0.1;
  constexpr double MAX_AWAY_DISTANCE = mxconst::MAX_AWAY_SKEWED_DISTANCE_NM; // 1.0; // v3.0.253.12
  constexpr double MIN_DEGREES       = 1.0;
  constexpr double MAX_DEGREES       = 359.0;

  const double lat       = Utils::readNumericAttrib(skewedPosition, mxconst::get_ATTRIB_LAT(), 0.0);
  const double lon       = Utils::readNumericAttrib(skewedPosition, mxconst::get_ATTRIB_LONG(), 0.0);
  const auto   dist_d    = Utils::getRandomRealNumber(MIN_AWAY_DISTANCE, MAX_AWAY_DISTANCE);
  const auto  bearing_f = static_cast<float>(Utils::getRandomRealNumber(MIN_DEGREES, MAX_DEGREES));
  // calculate new location
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane(outLat, outLon, lat, lon, bearing_f, dist_d);

  Utils::xml_set_attribute_in_node<double>(skewedPosition, mxconst::get_ATTRIB_LAT(), outLat, skewedPosition.getName());
  Utils::xml_set_attribute_in_node<double>(skewedPosition, mxconst::get_ATTRIB_LONG(), outLon, skewedPosition.getName());
  Utils::xml_set_attribute_in_node<bool>(skewedPosition, mxconst::get_ATTRIB_IS_SKEWED_POSITION_B(), true, skewedPosition.getName());

  const std::string real_pos = "lat=" + Utils::readAttrib(inRealTargetPositionPoint, mxconst::get_ATTRIB_LAT(), "") + " lon=" + Utils::readAttrib(inRealTargetPositionPoint, mxconst::get_ATTRIB_LONG(), "");
  Utils::xml_set_attribute_in_node_asString(skewedPosition, mxconst::get_ATTRIB_REAL_POSITION(), real_pos, skewedPosition.getName());


  return skewedPosition.deepCopy();
}


// -----------------------------------


bool
RandomEngine::parse_display_object_element(IXMLNode& inFlightLegNode, IXMLNode& inDisplayNode)
{
  // 1. Check if <display_object> tag has random_object attribute. If so it will pick one and add to the <object_templates>
  // 2. If random element is not valid, or we failed to find then use: name="".
  // 3. if no name has been provided then return "false", node is not valid.
  // We will use:  xDummyTopNode and x3DObjTemplate (holds pre-defined objects)

  const std::string TAG_NAME = inDisplayNode.getName();

  bool flag_foundValidRandomNode = false;

  // v3.0.219.10 read information regarding flight leg in water.
  bool        flag_isFlightLegInWater = Utils::readBoolAttrib(inFlightLegNode, mxconst::get_PROP_IS_WET(), false);
  //std::string attribIsLegInWater     = Utils::xml_get_attribute_value(inFlightLegNode, mxconst::get_PROP_IS_WET(), flag_found);
  //if (!attribIsLegInWater.empty() && mxconst::get_MX_TRUE().compare(attribIsLegInWater) == 0)
  //  flag_isFlightLegInWater = true;

  std::string name                   = Utils::readAttrib(inDisplayNode, mxconst::get_ATTRIB_NAME(), "");
  std::string randomTag              = Utils::readAttrib(inDisplayNode, mxconst::get_ATTRIB_RANDOM_TAG(), "");
  std::string file_name              = Utils::readAttrib(inDisplayNode, mxconst::get_ATTRIB_FILE_NAME(), "");
  std::string randomWaterTag         = Utils::readAttrib(inDisplayNode, mxconst::get_ATTRIB_RANDOM_WATER_TAG(), "");
  std::string optional_attrib        = Utils::readAttrib(inDisplayNode, mxconst::get_ATTRIB_OPTIONAL(), EMPTY_STRING);
  auto        limit_to_terrain_slope = Utils::readNodeNumericAttrib<float> ( inDisplayNode, mxconst::get_ATTRIB_LIMIT_TO_TERRAIN_SLOPE(), 100.0f );


  // check if optional was defined and skip object if was not random picked
  optional_attrib = Utils::replaceChar1WithChar2_v2(optional_attrib, '%', ""); // v3.0.219.12+ removes any % from string before handling it
  if (!optional_attrib.empty() && Utils::is_digits(optional_attrib))
  {
    int percent = Utils::stringToNumber<int>(optional_attrib);
    if (percent < 0)
      percent = 1;
    if (percent > 100)
      percent = 99;

    int result = Utils::getRandomIntNumber(0, 100);
    if (result > percent) // meaning missed
      return false;
  }


  // check if object creation is limited by terrain slope
  if (limit_to_terrain_slope < 100 && expected_slope_at_target_location_d > limit_to_terrain_slope) // v3.0.219.12+
  {
    Log::logMsg("3D Object: " + name + ", rejected due to terrain slope.", true); // v3.0.219.12+
    return false;
  }


  if (flag_isFlightLegInWater && !randomWaterTag.empty()) // v3.0.219.10 switch between terrain random object and water tag object
  {
#ifndef RELEASE
    Log::logMsgThread("[parse display object] Replaced randomTag with the water Tag: " + randomWaterTag + ", for display object name: " + name);
#endif // !RELEASE

    randomTag = randomWaterTag;
  }


  if (!randomTag.empty())
  {
    IXMLNode tagNode = Utils::xml_get_node_from_node_tree_IXMLNode(xRootTemplate, randomTag, false);
    if (!tagNode.isEmpty())
    {
      IXMLNode rNode = Utils::xml_get_node_randomly_by_name_IXMLNode(tagNode, mxconst::get_ELEMENT_OBJ3D(), this->errMsg, false); // return deepCopy
      this->errMsg.clear();

      // check if rNode already exists
      std::string rNodeAttribName     = Utils::readAttrib(rNode, mxconst::get_ATTRIB_NAME(), "");
      std::string rNodeAttribFileName = Utils::readAttrib(rNode, mxconst::get_ATTRIB_FILE_NAME(), "");
      if (!rNodeAttribName.empty() && !rNodeAttribFileName.empty())
      {
        // set <display_object> name attribute with the random name
        Utils::xml_search_and_set_attribute_in_IXMLNode(inDisplayNode, mxconst::get_ATTRIB_FILE_NAME(), rNodeAttribFileName, TAG_NAME);
        flag_foundValidRandomNode = true;

        // check 3D Object is also in <object_template>
        bool     flag_objWithSameName_found_in_objectTemplate = false;
        IXMLNode xObj3d_same_name_in_template                 = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(x3DObjTemplate, mxconst::get_ELEMENT_OBJ3D(), mxconst::get_ATTRIB_NAME(), rNodeAttribName, false);
        flag_objWithSameName_found_in_objectTemplate          = (xObj3d_same_name_in_template.isEmpty()) ? false : true;

        std::string newName = rNodeAttribName;

        IXMLNode xObj3d_with_same_filename_in_objectTemplate_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(x3DObjTemplate, mxconst::get_ELEMENT_OBJ3D(), mxconst::get_ATTRIB_FILE_NAME(), rNodeAttribFileName, false);

        // if we fail to find a template object with same file name we should check:
        // if random node mxconst::get_ATTRIB_NAME() was found in object_template
        // if not then we can just add the 3d object with the random element mxconst::get_ATTRIB_NAME()
        // but if we found an object with same name as the random element, then we should construct a new name using: "display_name + random_attrib_name + random real number and also set it to our "display_object" element
        if (xObj3d_with_same_filename_in_objectTemplate_ptr.isEmpty()) // if no object with such name was found
        {

          if (flag_objWithSameName_found_in_objectTemplate)
          {
            newName = name + "_" + rNodeAttribName + Utils::formatNumber<double>(Utils::getRandomRealNumber(1.0, 10.0), 4);
            rNode.updateAttribute(newName.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
          }

          x3DObjTemplate.addChild(rNode);
          Utils::xml_search_and_set_attribute_in_IXMLNode(inDisplayNode, mxconst::get_ATTRIB_NAME(), newName, TAG_NAME); // we want the display_object name attribute to be the same as the one in the object_template name
        }
        else // found object with same file name in the object_template
        {
          // v3.0.255.4 add the "isLibFile" value is it is not empty so plugin will know if this is a lib file or regular file
          // v3.303.14 deprecated
          //if (!isLibFile.empty())
          //  xObj3d_with_same_filename_in_objectTemplate_ptr.updateAttribute(isLibFile.c_str(), mxconst::get_ATTRIB_IS_VIRTUAL_B().c_str(), mxconst::get_ATTRIB_IS_VIRTUAL_B().c_str());

          newName = xObj3d_with_same_filename_in_objectTemplate_ptr.getAttribute(mxconst::get_ATTRIB_NAME().c_str());
          Utils::xml_search_and_set_attribute_in_IXMLNode(inDisplayNode, mxconst::get_ATTRIB_NAME(), newName, TAG_NAME); //
        }


        return true; // finish random pick
      }
    }
  }
  else if (!file_name.empty() && !name.empty()) // v3.0.255.1
  {
    std::string leg_name        = Utils::readAttrib(inFlightLegNode, mxconst::get_ATTRIB_NAME(), "dummy_");
    std::string new_object_name = (name.empty()) ? leg_name + Utils::formatNumber<double>(Utils::getRandomRealNumber(1.0, 10.0), 4) : name;

    // Create temporary <obj3d> node.
    IXMLNode new_obj3d_p = inDisplayNode.addChild(mxconst::get_ELEMENT_OBJ3D().c_str()); //  inDisplayNode.deepCopy();

    std::set<std::string> setAttribToCopy = { mxconst::get_ATTRIB_NAME(), mxconst::get_ATTRIB_FILE_NAME(), mxconst::get_ATTRIB_IS_VIRTUAL_B() }; // v3.0.255.4 added ATTRIB_IS_VIRTUAL_B
    Utils::xml_copy_specific_attributes_using_white_list(inDisplayNode, new_obj3d_p, &setAttribToCopy);

    // search for 3D Object with same name
    IXMLNode xObj3d_same_name_in_objectTemplate = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(x3DObjTemplate, mxconst::get_ELEMENT_OBJ3D(), mxconst::get_ATTRIB_NAME(), new_object_name, false);
    if (xObj3d_same_name_in_objectTemplate.isEmpty())
    {
      new_obj3d_p.updateAttribute(new_object_name.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
      x3DObjTemplate.addChild(new_obj3d_p.deepCopy());
    }
    inDisplayNode.updateAttribute(new_object_name.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());

    new_obj3d_p.deleteNodeContent();
  }


  // Check if original name was set or not (validation)
  if (!flag_foundValidRandomNode)
  {
    if (name.empty())
    {
      this->setError("[random parse display] Found <display_element> with no attribute name or random_tag.");
      return false;
    }
  }


  return true;
}

// -----------------------------------

void
RandomEngine::parse_3D_object_template_element()
{
  // 1. Check if <object_template> tag has random_object attribute.
  // 2. If random element has fileName we will use it
  // 3. if no name has been provided then return "false", node is not valid.
  // We will use:  xDummyTopNode and x3DObjTemplate (holds pre-defined objects)

  int nChilds = x3DObjTemplate.nChildNode(mxconst::get_ELEMENT_OBJ3D().c_str());
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    IXMLNode xObj3d_node_ptr = x3DObjTemplate.getChildNode(mxconst::get_ELEMENT_OBJ3D().c_str(), i1);

    std::string name           = Utils::readAttrib(xObj3d_node_ptr, mxconst::get_ATTRIB_NAME(), "");
    std::string randomTag      = Utils::readAttrib(xObj3d_node_ptr, mxconst::get_ATTRIB_RANDOM_TAG(), "");
    std::string randomWaterTag = Utils::readAttrib(xObj3d_node_ptr, mxconst::get_ATTRIB_RANDOM_WATER_TAG(), "");

    if (!randomTag.empty())
    {
      IXMLNode tagNode = Utils::xml_get_node_from_node_tree_IXMLNode(xRootTemplate, randomTag, false);
      if (!tagNode.isEmpty())
      {
        IXMLNode rNode = Utils::xml_get_node_randomly_by_name_IXMLNode(tagNode, mxconst::get_ELEMENT_OBJ3D(), errMsg, false); // return deepCopy
        this->errMsg.clear();

        if (rNode.isEmpty())
          continue;

        // replace file name with the random one
        std::string rNodeFileName = Utils::readAttrib(rNode, mxconst::get_ATTRIB_FILE_NAME(), "");
        if (!rNodeFileName.empty())
        {
          // v3.0.255.4 Add isLibFile // v3.303.14 DEPRECATED ATTRIB_IS_VIRTUAL_B
          //std::string isLibFile_s = Utils::readAttrib(rNode, mxconst::get_ATTRIB_IS_VIRTUAL_B(), "");
          //xObj3d_node_ptr.updateAttribute(isLibFile_s.c_str(), mxconst::get_ATTRIB_IS_VIRTUAL_B().c_str(), mxconst::get_ATTRIB_IS_VIRTUAL_B().c_str());

          // set <display_object> name attribute with the random name
          Utils::xml_search_and_set_attribute_in_IXMLNode(xObj3d_node_ptr, mxconst::get_ATTRIB_FILE_NAME(), rNodeFileName, mxconst::get_ELEMENT_OBJ3D());
        }
      }
    } // end randomTag

  } // end loop

} // parse_object_template

// -----------------------------------

bool
RandomEngine::readFlightLegs_directlyFromTemplate()
{

  Log::logDebugBO("[DEBUG random airport] start readFlightLegsDirectlyFromTemplate()", true);


//  bool flag_found = false;
  mapFlightPlanOrder_si.clear();
  mapFLightPlanOrder_is.clear();


  // Loop over each FlightLeg element and according to its "type" and "expected_location.locationOptionType" attribute we will construct a Leg
  bool result                = true;
  this->flag_isLastFlightLeg = false;

  int nChilds              = xRootTemplate.nChildNode(mxconst::get_ELEMENT_LEG().c_str());
  int flightLegs_counter_i = 1;
  for (int i1 = 0; i1 < nChilds && !(missionx::RandomEngine::threadState.flagAbortThread); ++i1)
  {
    // set the most basic data for the <leg>. The rest of the <leg> element will be added later
    IXMLNode xFlightLegFromTemplate = xRootTemplate.getChildNode(mxconst::get_ELEMENT_LEG().c_str(), i1);

    // v3.0.219.11
    if (i1 == (nChilds - 1))
    { // v3.0.251 b2: enhance condition, add user creation legs rules. User can ask for just 1 leg, hence it should not be considered as last
      if (this->flag_rules_defined_by_user_ui && Utils::readNodeNumericAttrib<int>(missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_NO_OF_LEGS(), 0) == 1) // number of childs start from 0 and not 1
        this->flag_isLastFlightLeg = false;
      else
        this->flag_isLastFlightLeg = true;
    }

    Log::logDebugBO("[DEBUG readFlightLegs_directlyFromTemplate] Build <leg> template: " + Utils::readAttrib(xFlightLegFromTemplate, mxconst::get_ATTRIB_NAME(), ""), true);

    // v3.0.219.2
    expected_slope_at_target_location_d = 0.0; // v3.0.219.12+
    // ----------------------------
    // Build the main flight Leg
    // ----------------------------
    IXMLNode xNewFlightLeg = this->buildFlightLeg(flightLegs_counter_i, xFlightLegFromTemplate);

    // skip if Leg node was not valid
    if (xNewFlightLeg.isEmpty())
      continue;

    this->mission_xml_data.currentLegName = Utils::readAttrib(xNewFlightLeg, mxconst::get_ATTRIB_NAME(), EMPTY_STRING);

    ///////////////////////////////
    /////////////////////////////
    // Set Flight Leg
    /////////////////////////////

    Utils::xml_add_node_to_element_IXMLNode(xFlightLegs, xNewFlightLeg);
    Utils::addElementToMap(mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, flightLegs_counter_i);
    Utils::addElementToMap(mapFLightPlanOrder_is, flightLegs_counter_i, this->mission_xml_data.currentLegName);

    ++flightLegs_counter_i;

    Utils::add_xml_comment(xFlightLegs, " [[[[ ]]]] "); // add comment between 2 Flight Legs

  } // end loop over <leg> template elements


  if (missionx::RandomEngine::threadState.flagAbortThread) // v3.0.219.12+
  {
    this->setError("[Random] Aborted !!!");
    return false;
  }

  this->fill_up_next_leg_attrib_after_flight_plan_was_generated();

  return result;
}

// -----------------------------------

IXMLNode
RandomEngine::get_content_story(const IXMLNode& xTemplateNode )
{
  IXMLNode cNode;

  if (xTemplateNode.isEmpty() )
  {
    this->setError("[random get content] Node template is empty. Fix template or notify developer. Skipping..");
    return cNode;
  }

  IXMLNode xContent = xTemplateNode.getChildNode(mxconst::get_ELEMENT_CONTENT().c_str());
  if (xContent.isEmpty())
    return cNode;

  // v3.0.221.15
  // get all <content child tags and pick one
  if ( int nChilds = xContent.nChildNode ()
      ; nChilds > 0)
  {

    const int         randomSubElement_i = Utils::getRandomIntNumber ( 0, nChilds - 1 );
    const std::string randomTag          = xContent.getChildNode(randomSubElement_i).getName(); // get tag name

    /// pick one of the sub content based on inTemplateType string
    cNode = Utils::xml_get_node_randomly_by_name_IXMLNode(xContent, randomTag, this->errMsg, false);
    this->errMsg.clear();
  }

  return cNode;
}

// -----------------------------------

bool
RandomEngine::extract_flight_leg_set(IXMLNode &inNodeTemplate, const IXMLNode& inSetNode, int& inCounter)
{
  // Read a XML element that should define a Flight Leg set
  const bool flag_copy_leg_as_is_b             = Utils::readBoolAttrib(inSetNode, mxconst::get_ATTRIB_COPY_LEG_AS_IS_B(), false); // v3.0.303
  bool       flag_needToRaiseLastFlightLegFlag = false; // only when this->flag_isLastFlightLeg was true when calling this function do we need to raise the flag when reaching last <leg> element in loop.
  const int  nChilds                           = inSetNode.nChildNode(mxconst::get_ELEMENT_LEG().c_str());

  if (this->flag_isLastFlightLeg && nChilds > 1)
  {
    this->flag_isLastFlightLeg        = false;
    flag_needToRaiseLastFlightLegFlag = true;
  }

  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    if (flag_needToRaiseLastFlightLegFlag && i1 == (nChilds - 1))
      this->flag_isLastFlightLeg = true;

    IXMLNode xLeg = inSetNode.getChildNode(mxconst::get_ELEMENT_LEG().c_str(), i1);
    if (xLeg.isEmpty())
      continue;

    IXMLNode xNewFlightLegNode = (flag_copy_leg_as_is_b) ? xLeg.deepCopy() : this->buildFlightLeg(inCounter, xLeg); // v3.0.303 added the support of attrib flag_copy_leg_as_is_b
    if (xNewFlightLegNode.isEmpty())
    {
      if (this->errMsg.empty())
        this->errMsg = "[random extract set]Fail to generate flight leg from set. Check log for errors and maybe fix template or notify developer. Skipping...";

      this->setError(this->errMsg); // this will also print the error message
      continue; // skip Leg. In some cases false outcome can occur due to "optional" attribute or other reason that does not need to break the mission build.
    }

    this->mission_xml_data.currentLegName = Utils::readAttrib(xNewFlightLegNode, mxconst::get_ATTRIB_NAME(), "");

    Utils::xml_add_node_to_element_IXMLNode(this->xFlightLegs, xNewFlightLegNode);
    Utils::addElementToMap(mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, inCounter);
    Utils::addElementToMap(mapFLightPlanOrder_is, inCounter, this->mission_xml_data.currentLegName);

    Log::logMsgThread("\n>> Added Flight Leg: " + this->mission_xml_data.currentLegName + "\n"); // v3.303.11 debug

    ++inCounter;

    Utils::add_xml_comment(this->xFlightLegs, " /////// "); // add comment between 2 <leg> nodes
  }

  // v3.0.303 if flag_copy_leg_as_is_b than add objectives and triggers
  if (flag_copy_leg_as_is_b)
    inNodeTemplate.updateAttribute("true", mxconst::get_ATTRIB_COPY_LEG_AS_IS_B().c_str(), mxconst::get_ATTRIB_COPY_LEG_AS_IS_B().c_str());


  return true;
}

// -----------------------------------

bool
RandomEngine::build_and_add_flight_leg_from_node(const IXMLNode& inNode, int& inCounter)
{
  IXMLNode xNewFLightLegNode = this->buildFlightLeg(inCounter, inNode);
  // skip if flight leg node is not valid
  if (xNewFLightLegNode.isEmpty())
  {
    if (this->errMsg.empty())
      this->errMsg = "[random build_and_add_flight_leg_from_node] Fail to generate flight leg, this might be ok. Check log for errors/warning, skipping...";

    this->setError(this->errMsg);
    return true;
  }

  this->mission_xml_data.currentLegName = Utils::readAttrib(xNewFLightLegNode, mxconst::get_ATTRIB_NAME(), "");

  Utils::xml_add_node_to_element_IXMLNode(xFlightLegs, xNewFLightLegNode);
  Utils::addElementToMap(mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, inCounter);
  Utils::addElementToMap(mapFLightPlanOrder_is, inCounter, this->mission_xml_data.currentLegName);

  ++inCounter;

  Utils::add_xml_comment(xFlightLegs, " [[[[ ]]]] "); // add comment between 2 flight legs
  return true;
}

// -----------------------------------

bool
RandomEngine::generateRandomMissionBasedOnContent(IXMLNode& xTemplateNode)
{
  typedef enum _mission_type
    : uint8_t
  {
    medevac  = 1,
    delivery = 2,
    random   = 3
  } mission_types_enum;

  enum _plane_type : uint8_t
  {
    helos = 1,
    props = 2
  }; // plane_types_enum;

  IXMLNode xFlightLegNodeFromTemplate;
  int legCounter              = 1;
  int min_valid_flight_legs_i = 1; // v3.0.221.15rc3.2

  mapFlightPlanOrder_si.clear();
  mapFLightPlanOrder_is.clear();

  this->flag_isLastFlightLeg = false;
  this->setPlaneType(mxconst::get_PLANE_TYPE_HELOS()); // initialization will initialize this->randomPlaneType

  mission_types_enum mType = mission_types_enum::random; // v3.0.223.1 changed to always random. medevac or deliver missions are part of the flight leg list and not template oriented.

  //// get random content Node if available in template
  IXMLNode xContent = this->get_content_story(xTemplateNode /*, inTemplateType*/);

  // v3.0.223.1 we won't support random without content.
  if (xContent.isEmpty())
  {
    this->setError("No <content> element was found. Aborting random mission creation. To fix this, please add <content> element. Check documentation.");
    return false;
  }


  // Plane type is now mandatory at content or template level
  // use plane_type from content, if was defined
  std::string pType = Utils::readAttrib(xContent, mxconst::get_ATTRIB_PLANE_TYPE(), "");
  if ( ! pType.empty() )
  {
    this->setPlaneType(Utils::stringToLower(pType));
    xTemplateNode.updateAttribute(this->randomPlaneType.c_str(), mxconst::get_ATTRIB_PLANE_TYPE().c_str(), mxconst::get_ATTRIB_PLANE_TYPE().c_str()); //// v3.0.221.7 fixed type to be what we fetched from xContent
  }
  else if (!this->randomPlaneType.empty()) // v3.0.223.1 added template plane type fallback. Use template plane_type attribute if was not set at content level
  {
    pType = this->randomPlaneType;
  }
  else
  {
    this->setError("Plane type was not set in template <content> or template level. Please notify the template writer or add 'plane_type=\"prop|helos\" to the template. See documentation.");
    return false;
  }


  // update template type so will affect buildFlightLeg() later.
  xTemplateNode.updateAttribute("", mxconst::get_ATTRIB_TYPE().c_str(), mxconst::get_ATTRIB_TYPE().c_str()); // can be changed during random type

  //// Read the briefer information after we decide the plane type. This will help with the "filter" of the ramp start in the briefer code.
  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  if (!readBrieferAndStartLocation()) // main <briefer>
  {
    missionx::RandomEngine::threadState.flagAbortThread = true;
    return false;
  }

  Log::logDebugBO("[DEBUG random airport] After <briefer_and_start_location> node in content.", true);

  // v3.0.255.4.2
  if (!readMissionInfoElement()) // <mission_info>
  {
    missionx::RandomEngine::threadState.flagAbortThread = true;
    return false;
  }

  ////// PREPARE Flight Plan BASED ON TYPE
  //    int nChilds = 0;
  std::string content_text;
  content_text.clear();

  // 24.12.2 Deprecate switch - mType is always  "random"
  // switch (mType)
  // {
  //   case mission_types_enum::random:
  //   {
    // 1. loop over each flight leg type
    // 2. extract optional value if present (search for "|" and pick the number.
    // 3. pick an element using the name given.
    // 3.1 Check if it is a FlightLeg set, if yes extract its elements and build a <leg> from them
    // 3.2 If not a <leg> set then build <leg>
    // 4. pick next <leg> in the loop
    // 5. after loop do finish work
    min_valid_flight_legs_i = (int)Utils::readNodeNumericAttrib<int>(xContent, mxconst::get_ATTRIB_MIN_VALID_FLIGHT_LEGS(), 1); // v3.0.221.15 rc3.3 // v3.0.303 changed to readNodeNumericAttrib

    std::list<std::string> listContent;
    listContent.clear();

    if (xContent.isEmpty())
      return false;

    Log::logDebugBO("[random] picked content element", true);

    // example returns: <delivery list="leg_delivery,leg_delivery|optional=50%,leg_delivery,leg_land," plane_type="prop">Hello// pilot.;Today you will fly to few locations to deliver goods. Use your 'inventory' to move items from/to your plane.</delivery>
    std::string flightLegList = Utils::readAttrib(xContent, mxconst::get_ATTRIB_LIST(), "");

    listContent = Utils::splitStringToList(flightLegList, mxconst::get_COMMA_DELIMITER());

    //      bool flag_isThisTheFirstFlightLegInList = true;
    auto countListElements = listContent.size();
    int  counter           = 0;
    for (std::string optional; auto tagName : listContent) // loop over list of "tag_names" that we received from splitting the "list=" attribute
    {
      ++counter;
      if (counter == static_cast<int>(countListElements)) // check if last flight leg in content
        this->flag_isLastFlightLeg = true;

      Log::logDebugBO("[content tag name: " + tagName, true); // debug v3.0.223.1


      ///// v3.0.221.7 Split flight leg to its special rules. First string is the element tag name the rest needs format: "{name}={value}"
      std::vector<std::string>           vecRandomFlightLegRules = mxUtils::split_v2(tagName, mxconst::get_PIPE_DELIMITER());
      std::map<std::string, std::string> mapFlightLegDirectives; // option name, option value
      int                                vecSize = static_cast<int>(vecRandomFlightLegRules.size());
      for (int i1 = 0; i1 < vecSize; ++i1)
      {
        const std::string&       value    = vecRandomFlightLegRules.at(i1);
        std::vector<std::string> vecSplit = mxUtils::split_v2(value, "=");

        if (i1 == 0) // first value represents the tag name Example: leg_delivery|optional=50%. The tag name is leg_delivery.
        {
          Utils::addElementToMap(mapFlightLegDirectives, "tag", vecRandomFlightLegRules.at(i1));
          tagName = vecRandomFlightLegRules.at(i1);
        }
        else // the second value onward are the rules on the tag name. Example: leg_delivery|optional=50%, the rule is: "optional=50%" which translate to 50% to create thie <leg>
        {
          if (vecSplit.size() >= 2)
          {
            const std::string& option_name  = vecSplit.at(0);
            const std::string& option_value = vecSplit.at(1);
            Utils::addElementToMap(mapFlightLegDirectives, option_name, option_value);
          }
          else
            continue;
        }
      }

      if (Utils::isElementExists(mapFlightLegDirectives, mxconst::get_ATTRIB_OPTIONAL())) // optional
        optional = mapFlightLegDirectives[mxconst::get_ATTRIB_OPTIONAL()];


      //// Handle Optional if set, there is no need to continue with the rest of the code if we fail here
      optional = Utils::replaceChar1WithChar2_v2(optional, '%', ""); // v3.0.219.7 removes any % from string prior to handling it

      #ifndef RELEASE
      if (!optional.empty())
        Log::logDebugBO("[DEBUG generate random based on content element] Optional value: " + optional, true);
      #endif


      // v3.0.221.7 optional test
      if (!optional.empty() && Utils::is_digits(optional))
      {
        int percent = Utils::stringToNumber<int>(optional);
        if ( int result = Utils::getRandomIntNumber ( 0, 100 )
            ; result > percent) // meaning missed
        {
          #ifndef RELEASE
          if (!optional.empty())
            Log::logDebugBO( fmt::format ( "[DEBUG generate random based on content element] Random result value: {}, optional value: {}. Skipping tag: {}", Utils::formatNumber<int>(result), optional, tagName, true) );
          #endif

          continue;
        }
      }

      this->errMsg.clear();
      xFlightLegNodeFromTemplate = Utils::xml_get_node_randomly_by_name_IXMLNode(xTemplateNode, tagName, this->errMsg, false);
      if (!errMsg.empty() || xFlightLegNodeFromTemplate.isEmpty())
      {
        this->setError(((errMsg.empty()) ? std::string( mxconst::get_QM()).append(tagName).append( mxconst::get_QM()).append(" element was not found. Please fix template. Aborting mission creation.") : this->errMsg));
        return false;
      }


      // Check if set of flight legs
      const std::string is_element_set_of_flight_legs = Utils::readAttrib(xFlightLegNodeFromTemplate, mxconst::get_ATTRIB_IS_SET_OF_FLIGHT_LEGS(), "");
      if ( ! is_element_set_of_flight_legs.empty() )
      {
        this->extract_flight_leg_set(xTemplateNode, xFlightLegNodeFromTemplate, legCounter);
      }
      else
      {
        // During flight leg construction the <leg> might be skipped due to it being optional or malformed. We should not abort the mission creation because of that.
        this->build_and_add_flight_leg_from_node(xFlightLegNodeFromTemplate, legCounter);
      }

    } // end loop over list of flight legs


    // validate mission
    if (xFlightLegs.nChildNode() < min_valid_flight_legs_i)
    {
      this->setError("Failed to create a mission based on <content>. Generated: " + Utils::formatNumber<int>(xFlightLegs.nChildNode()) + " 'flight legs', but needs minimum: " + Utils::formatNumber<int>(min_valid_flight_legs_i) +
                     " 'legs'. Aborting mission creation. Try to generate a new one.");
      return false;
    }

    content_text = Utils::xml_get_text(xContent); // xContent.getText();

  // This code replaced the for (auto nav : this->listNavInfo) just for readability I did not test it yet.
  // This should be more readable, do not think it is faster though.
  const auto lmbda_nav_info_used_random_lat_long  = [&](const missionx::NavAidInfo& navInfo) { return (navInfo.flag_picked_random_lat_long == true); }; // check NavInfo if it has the "flag_picked_random_lat_long == true"
  if ( std::ranges::any_of ( this->listNavInfo, lmbda_nav_info_used_random_lat_long ) )
  {
    this->setPlaneType(mxconst::get_PLANE_TYPE_HELOS());
    mType = mission_types_enum::medevac;
  }

  // Modify briefer description
  const std::string message = (content_text.empty()) ? "Welcome pilot, in this flight you will have to reach a location on your GPS " + std::string(((mType == mission_types_enum::medevac) ? "to assist in evacuation" : "to deliver goods")) +
                                                   ". The plane type picked for this mission is: " + this->randomPlaneType + ", but you can pick another if you think you can make it."
                                               : content_text;
  Utils::xml_add_cdata(this->xBriefer, message);
  Utils::xml_search_and_set_attribute_in_IXMLNode(this->xBrieferInfo, mxconst::get_ATTRIB_PLANE_DESC(), this->randomPlaneType);

  if (missionx::RandomEngine::threadState.flagAbortThread) // v3.0.219.12+
  {
    this->setError("[Random] generateRandomShortMission() Aborted !!!");
    return false;
  }

  this->fill_up_next_leg_attrib_after_flight_plan_was_generated();

  return true;
}


// -----------------------------------
// -----------------------------------


IXMLNode
RandomEngine::buildFlightLeg(int inFlightLegCounter, const IXMLNode& in_legNodeFromTemplate)
{
  this->errMsg.clear();

  // check if there is any "<leg>" template node in MAPPING element
  if (Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::xmlMappingNode, mxconst::get_ELEMENT_LEG()).isEmpty())
  {
    this->setError("[Random] Could not find flight <leg> mapping. Please notify designer/programmer !!!"); // this should be displayed
    return IXMLNode::emptyIXMLNode;
  }


  // prepare  flight leg elements
  IXMLNode xNewFlightLeg     = Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::xmlMappingNode, mxconst::get_ELEMENT_LEG());           // holds the new flight <leg> element being constructed.
  IXMLNode xLegTargetTrigger = Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::xmlMappingNode, mxconst::get_ELEMENT_TRIGGER());       // holds trigger element from MAPPING
  IXMLNode xMapMessage       = Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE (), true); // holds message element from MAPPING
  IXMLNode xLegTask          = Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::xmlMappingNode, mxconst::get_ELEMENT_TASK());          // holds task element from MAPPING
  IXMLNode xLegObjective     = Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::xmlMappingNode, mxconst::get_ELEMENT_OBJECTIVE());     // holds objective element from MAPPING


  //// Validate XML mapping are set correctly or MAPPING is not complete and need to be fixed.
  if (xNewFlightLeg.isEmpty() || xLegTargetTrigger.isEmpty())
  {
    this->setError("[random flightLeg] mapping element does not have flight <leg> or <trigger> mapping template. Fix template file or notify designer/programmer.");
    return IXMLNode::emptyIXMLNode;
  }


  // set the most basic data on <leg> element. The rest of the <leg> element will be added later
  IXMLNode xLegFromTemplate = in_legNodeFromTemplate.deepCopy(); // v3.0.219.2
  if (xLegFromTemplate.isEmpty())                                 // v3.0.219.2
  {
    this->setError("[random flightLeg] Failed to find a template <leg>. Please fix your template and retry.");
    return IXMLNode::emptyIXMLNode;
  }

  // v3.0.223.4 task_name_for_leg_message and  trigger_name_for_leg_message. these attributes should be in <expected_location> template element
  std::string override_task_name_s    = Utils::xml_get_attribute_value_drill(xLegFromTemplate, mxconst::get_ATTRIB_OVERRIDE_TASK_NAME(), this->flag_found, mxconst::get_ELEMENT_EXPECTED_LOCATION());
  std::string override_trigger_name_s = Utils::xml_get_attribute_value_drill(xLegFromTemplate, mxconst::get_ATTRIB_OVERRIDE_TRIGGER_NAME(), this->flag_found, mxconst::get_ELEMENT_EXPECTED_LOCATION());
  // END v3.0.223.4
  bool flag_disable_auto_messages = Utils::readBoolAttrib(xLegFromTemplate, mxconst::get_ATTRIB_DISABLE_AUTO_MESSAGE_B(), false); // v3.0.241.1 replaced  disable_auto_message_s with simpler implementation



  const int flightLegCounter = inFlightLegCounter; // v3.0.241.8 made it const // v3.0.219.2 to keep same code parameters as the original one.

  std::string flight_leg_type_hover_land_or_start = mxUtils::stringToLower(Utils::readAttrib(xLegFromTemplate, mxconst::get_ATTRIB_TEMPLATE(), EMPTY_STRING));
  std::string stored_flight_leg_template_type     = flight_leg_type_hover_land_or_start; // v3.0.253.6 use when we want to force flight leg type and it might be changed during the "buildFlightLeg()" function.
  std::string flightLegName                       = Utils::readAttrib(xLegFromTemplate, mxconst::get_ATTRIB_NAME(), EMPTY_STRING);
  std::string attrib_Optional                     = Utils::readAttrib(xLegFromTemplate, mxconst::get_ATTRIB_OPTIONAL(), EMPTY_STRING);                        // v3.0.219.6
  std::string attrib_dependsOn                    = Utils::readAttrib(xLegFromTemplate, mxconst::get_ATTRIB_DEPENDS_ON(), EMPTY_STRING);                      // v3.0.219.11
  bool        skip_auto_task_creation_b           = Utils::readBoolAttrib(xLegFromTemplate, mxconst::get_ATTRIB_SKIP_AUTO_TASK_CREATION_B(), false); // v3.0.221.15 rc4

  attrib_Optional = Utils::replaceChar1WithChar2_v2(attrib_Optional, '%', ""); // v3.0.219.7 removes any % from string before handling it

  #ifndef RELEASE
  Log::logMsgNone("\n**** [DEBUG buildFlightLeg] Try to build a Flight Leg for - flight_leg_type: " + flight_leg_type_hover_land_or_start + ", flight_leg_name: " + flightLegName + " **** \n", true);
  #endif


  // v3.0.219.6 optional test
  if (!attrib_Optional.empty() && Utils::is_digits(attrib_Optional))
  {
    int percent = Utils::stringToNumber<int>(attrib_Optional);
    if (percent < 0)
      percent = 1;
    if (percent > 100)
      percent = 99;

    int result = Utils::getRandomIntNumber(0, 100);
    if (result > percent) // meaning missed
      return IXMLNode::emptyIXMLNode;
  }


  // define names
  if (flightLegName.empty() ||
      Utils::isElementExists(mapFlightPlanOrder_si, flightLegName)) // v3.0.241.9 moved this condition before rest of attributes, due to the fact their names are dependent on the leg name. That way, the name can be optional and the plugin
                                                                    // will provide one. // v3.0.219.9 fix cases where designer uses same flight leg name in template by mistake // v3.0.219.11 use "mapLegOrder_si" instead of "setNames"
    flightLegName = std::string(mxconst::get_ELEMENT_LEG()) + "_" + Utils::formatNumber<int>(flightLegCounter); // genFlightLegName;


  std::string triggerName               = (override_trigger_name_s.empty()) ? "trigger_task_" + flightLegName : override_trigger_name_s; // v3.0.223.4 added override_trigger_s
  std::string objectiveName             = "obj_" + flightLegName;
  std::string taskName                  = (override_task_name_s.empty()) ? "task_" + flightLegName : override_task_name_s; // v3.0.223.4 added override_task_name_s
  std::string message_triggerTargetName = "message_trigger_target_" + flightLegName;
  std::string triggerTargetMessage;



  // skip Leg if it depends on other FLight Leg that is not in the list (meaning - it was not constructed, maybe was skipped or was not valid)
  if (!attrib_dependsOn.empty() && !(Utils::isElementExists(mapFlightPlanOrder_si, attrib_dependsOn))) //
  {
    Log::logDebugBO("[DEBUG buildFlightLeg] Skip Flight Leg " + flightLegName + ", reason: attribute depends_on point to a <leg> that does not exists: " + attrib_dependsOn + ".", true);
    return IXMLNode::emptyIXMLNode;
  }


  // set mandatory attributes for <leg> element
  Utils::xml_search_and_set_attribute_in_IXMLNode(xNewFlightLeg, mxconst::get_ATTRIB_NAME(), flightLegName, mxconst::get_ELEMENT_LEG());               // set flight leg name
  Utils::xml_search_and_set_attribute_in_IXMLNode(xNewFlightLeg, mxconst::get_ATTRIB_NAME(), objectiveName, mxconst::get_ELEMENT_LINK_TO_OBJECTIVE()); // set flight leg link_to_objective

  //// OBJECTIVE  ////
  // check xObjective validity and construct one if needed
  if (xLegObjective.isEmpty())
  {
    this->setError("[random flightLeg objective] objective template MAPPING was missing, constructing dummy one.");
    xLegObjective = this->xObjectives.addChild(mxconst::get_ELEMENT_OBJECTIVE().c_str());
    xLegObjective.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), objectiveName.c_str());
  }

  //// PARSE EXPECTED LOCATION  ////
  IXMLNode xExpectedLocation = xLegFromTemplate.getChildNode(mxconst::get_ELEMENT_EXPECTED_LOCATION().c_str());
  if (xExpectedLocation.isEmpty())
  {
    this->setError("[random flightLeg location] Failed to find: " + mxconst::get_ELEMENT_EXPECTED_LOCATION() + ", while parsing Flight Leg: " + flightLegName + ". Please fix template.");
    return IXMLNode::emptyIXMLNode;
  }

  // test if expected location is valid
  const int force_slope_i         = Utils::readNodeNumericAttrib<int>(xExpectedLocation, mxconst::get_ATTRIB_FORCE_SLOPED_TERRAIN(), 0);
  const int force_level_terrain_i = (force_slope_i == 0) ? Utils::readNodeNumericAttrib<int>(xExpectedLocation, mxconst::get_ATTRIB_FORCE_LEVELED_TERRAIN(), 0) : 0; // v3.0.253.9.1 force_slope_i has precedence over "force level terrain_i".
  missionx::data_manager::Max_Slope_To_Land_On = Utils::readNodeNumericAttrib<float>(xExpectedLocation, mxconst::get_ATTRIB_DESIGNER_MAX_SLOPE_TO_LAND(), mxconst::DEFAULT_MAX_SLOPE_TO_LAND_ON); // v3.0.253.9.1
  if (missionx::data_manager::Max_Slope_To_Land_On < 1.0)                                                                                                                                   // v3.0.253.9.1
    missionx::data_manager::Max_Slope_To_Land_On = 1.0; // this is almost means to ignore any slope and just flag the target as land-able

  // v3.0.241.8
  this->flag_force_template_distances_b = Utils::readBoolAttrib(xExpectedLocation, mxconst::get_ATTRIB_FORCE_TEMPLATE_DISTANCES_B(), false); // will be used in get_target() function to disable the "expected distance setup option".


  /////////////////////////////////////////////////////////////////
  ////// SET SHARED INFORMATION/ELEMENTS to ALL <leg> types //////
  ///////////////////////////////////////////////////////////////


  ////////////////////////////////////////////
  // Fail if LOCATION_TYPE is empty

  // if (location_type_s.empty())
  if (Utils::readAttrib(xExpectedLocation, mxconst::get_ATTRIB_LOCATION_TYPE(), "").empty())
  {
    this->setError("[random flightLeg location] Fail to find: " + mxconst::get_ATTRIB_LOCATION_TYPE() + ", while parsing template Flight Leg: " + flightLegName + ". Please fix the template.");
    return IXMLNode::emptyIXMLNode;
  }


  auto        location_radius_mt  = Utils::readNodeNumericAttrib<int>(xExpectedLocation, mxconst::get_ATTRIB_RADIUS_MT(), 0); // v3.0.241.8 added support for custom target radius. This for hover cases.
  std::string location_type       = Utils::readAttrib(xExpectedLocation, mxconst::get_ATTRIB_LOCATION_TYPE(), "");
  std::string location_value_nm_s = Utils::readAttrib(xExpectedLocation, mxconst::get_ATTRIB_LOCATION_VALUE(), "");
  std::string location_value_restrict_ramp_type_s; // v3.0.221.7 will hold special ramp type if location value has special string characters after the numbers

  // location value format can be: "{number}|{ramp type}|{min-max},..."
  // mapLocationValueOptions: {name},{value}.
  std::map<int, std::string> mapLocationValueOptions;                                             // v3.0.221.7 will hold the coomplex options used by "|".
  std::string location_value_min_max_distance_s, location_value_tag_name_s, location_value_poi_s; // v3.0.221.7 @Daikan used in Random airport pick. "location_value_tag_name_s" will be used to hold element name to search in template.


  //////////////////////////////////
  //// SPECIAL Leg element ////
  ////////////////////////////////
  // Read special directive element
  IXMLNode xSpecialLegDirectives = in_legNodeFromTemplate.getChildNode(mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES().c_str()); // v3.0.221.15rc5 support LEG // v3.0.221.9 check if <special... element already exists
  if (xSpecialLegDirectives.isEmpty())
    xSpecialLegDirectives = xNewFlightLeg.addChild(mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES().c_str()); // v3.0.241.8 removed backward support // v3.0.221.8 create special element if not exists // v3.0.221.15rc5 support LEG

  xNewFlightLeg.addChild(xSpecialLegDirectives); // v3.0.221.9 add element if exists

  //////////////////////////////////
  //// TASK SHARED INFORMATION ////
  ////////////////////////////////
  /// TASK - Check xTask

  if (xLegTask.isEmpty() && !skip_auto_task_creation_b)
  {
    xLegTask = xLegObjective.addChild(mxconst::get_ELEMENT_TASK().c_str());
  }

  // set Task name
  if (!Utils::xml_search_and_set_attribute_in_IXMLNode(xLegTask, mxconst::get_ATTRIB_NAME(), taskName, mxconst::get_ELEMENT_TASK()))
  {
    xLegTask.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), taskName.c_str());

    Log::logMsgWarn("[random task]Task node had missing attribute. Added 'name' attribute. Suggest: check your mapping definitions.", true);
  }

  IXMLNode xSpecialTasks;
  IXMLNode xSpecialMessages;
  if (!xSpecialLegDirectives.isEmpty())
  {
    const std::string attrib_base_on_external_plugin_value_s = Utils::readAttrib(xSpecialLegDirectives, mxconst::get_ATTRIB_BASE_ON_EXTERNAL_PLUGIN(), mxconst::get_MX_FALSE());
    xLegTask.updateAttribute(attrib_base_on_external_plugin_value_s.c_str(), mxconst::get_ATTRIB_BASE_ON_EXTERNAL_PLUGIN().c_str(), mxconst::get_ATTRIB_BASE_ON_EXTERNAL_PLUGIN().c_str());

    // search for special trigger in template
    const std::string use_trigger_from_template = Utils::readAttrib(xSpecialLegDirectives, mxconst::get_ATTRIB_USE_TRIGGER_NAME_FROM_TEMPLATE(), ""); // v3.0.221.10 should hold the attribute name of the trigger element in the template <trigger name="{name}" >
    if (!use_trigger_from_template.empty())
    {
      IXMLNode tNode = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(xRootTemplate, mxconst::get_ELEMENT_TRIGGER(), mxconst::get_ATTRIB_NAME(), use_trigger_from_template);
      if (!tNode.isEmpty())
        xLegTargetTrigger = tNode;
    }

    // search for set of tasks to add to the FlightLeg objective
    const std::string add_tasks_from_template = Utils::readAttrib(xSpecialLegDirectives, mxconst::get_ATTRIB_ADD_TASKS_FROM_TEMPLATE(), ""); // v3.0.221.10 should hold the tag name of the tasks we want to add to one of the flight legs.
    if (!add_tasks_from_template.empty())
      xSpecialTasks = xRootTemplate.getChildNode(add_tasks_from_template.c_str()); // if we find an element with this name, we will add all sub elements tasks to objective

    // search for set of messages to add to xDescText
    const std::string add_messages_from_template = Utils::readAttrib(xSpecialLegDirectives, mxconst::get_ATTRIB_ADD_MESSAGES_FROM_TEMPLATE(), ""); // v3.0.221.10 should hold the tag name of the messages we want to xMessages
    if (!add_messages_from_template.empty())
      xSpecialMessages = xRootTemplate.getChildNode(add_messages_from_template.c_str()); // if we find an element with this name, we will add all sub elements to xMessages
  }

  if (skip_auto_task_creation_b) // v3.0.221.15 rc4 validate that we have an alternative task (in special <leg> directive element)
  {
    xLegTask = xSpecialTasks.getChildNode(mxconst::get_ELEMENT_TASK().c_str());
    if (xLegTask.isEmpty())
    {
      Log::logMsgErr("[random task] Could not find alternative special task that will replace the mandatory task that was skipped cause of the use of: skip_auto_task_creation_b attribute. Skipping <leg> creation.", true);
      return IXMLNode::emptyIXMLNode;
    }
  }

  // set base on trigger
  if (!Utils::xml_search_and_set_attribute_in_IXMLNode(xLegTask, mxconst::get_ATTRIB_BASE_ON_TRIGGER(), triggerName, mxconst::get_ELEMENT_TASK()))
  {
    xLegTask.updateAttribute(triggerName.c_str(), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str(), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str());

    Log::logMsgWarn("[random task]Task node had missing attribute. Added 'base_on' attribute. Suggest: check your mapping definitions.", true);
  }
  //// END TASK INFORMATION ////

  ////////////////////////////////////////////////////////
  // set as mandatory (no need part of template mapping )
  //// SET SHARED OBJECTIVE INFORMATION               ////
  ///////////////////////////////////////////////////////
  if (!Utils::xml_search_and_set_attribute_in_IXMLNode(xLegObjective, mxconst::get_ATTRIB_NAME(), objectiveName, mxconst::get_ELEMENT_OBJECTIVE()))
  {
    this->setError("[random objective]Failed to set objective property: '" + mxconst::get_ATTRIB_NAME() + "'");
    return IXMLNode::emptyIXMLNode;
  }

  // add task to objective
  xLegObjective.addChild(xLegTask);

  // v3.0.221.10 add all sub specials elements to the <leg> element.
  if (!xSpecialLegDirectives.isEmpty())
  {
    // Add pre-defined tasks
    if (!xSpecialTasks.isEmpty())
    {
      Utils::add_xml_comment(xLegObjective, " +++ Added custom Task +++ ");
      int nChilds = xSpecialTasks.nChildNode(mxconst::get_ELEMENT_TASK().c_str());
      for (int i1 = (skip_auto_task_creation_b) ? 1 : 0; i1 < nChilds; ++i1) // v3.0.221.15 rc4 if skip_auto_task_creation_b=true then add second task onward, if not then from first. Reason: when skip_auto_task_creation_b is true, then
                                                                             // first task becomes the mandatory one that will use the special trigger
      {
        IXMLNode cNode = xSpecialTasks.getChildNode(mxconst::get_ELEMENT_TASK().c_str(), i1);
        if (!cNode.isEmpty())
          xLegObjective.addChild(cNode.deepCopy());
      }
      Utils::add_xml_comment(xLegObjective, " +++ End custom Task +++ ");
    }

    // Add pre-defined messages
    if (!xSpecialMessages.isEmpty())
    {
      Utils::add_xml_comment(xMessages, " +++ Added custom message +++ ");

      const int nChilds = xSpecialMessages.nChildNode(mxconst::get_ELEMENT_MESSAGE ().c_str());
      for (int i1 = 0; i1 < nChilds; ++i1)
      {
        const IXMLNode cNode = xSpecialMessages.getChildNode(mxconst::get_ELEMENT_MESSAGE ().c_str(), i1);
        if (!cNode.isEmpty())
        {
          xMessages.addChild(cNode.deepCopy());
        }
      }

      Utils::add_xml_comment(xMessages, " +++ End custom message +++ ");
    }

  } // end adding sub elements from special


  // add objective to its main element: <objectives>
  xObjectives.addChild(xLegObjective);

  /////// END OBJECTIVE Settings /////////

  ///////////////////////////////////
  // set TRIGGER shared properties /
  //////////////////////////////////

  if (!Utils::xml_search_and_set_attribute_in_IXMLNode(xLegTargetTrigger, mxconst::get_ATTRIB_NAME(), triggerName, mxconst::get_ELEMENT_TRIGGER()))
  {
    this->setError("[random trigger]Failed to set trigger property: '" + mxconst::get_ATTRIB_NAME() + "'");
    return IXMLNode::emptyIXMLNode;
  }

  // v3.0.221.13 Check if trigger has outcome and add if not
  if (xLegTargetTrigger.getChildNode(mxconst::get_ELEMENT_OUTCOME().c_str()).isEmpty())
    xLegTargetTrigger.addChild(mxconst::get_ELEMENT_OUTCOME().c_str());


  ///////////////////////////////////
  // READ commands from leg template // v3.0.221.9
  //////////////////////////////////
  IXMLNode xFireCommandsAtLegBegin = xLegFromTemplate.getChildNode(mxconst::get_ELEMENT_FIRE_COMMANDS_AT_LEG_START().c_str());
  IXMLNode xFireCommandsAtLegEnd   = xLegFromTemplate.getChildNode(mxconst::get_ELEMENT_FIRE_COMMANDS_AT_LEG_END().c_str());

  xNewFlightLeg.addChild(xFireCommandsAtLegBegin);
  xNewFlightLeg.addChild(xFireCommandsAtLegEnd);

  ///////////////////////////////////
  // Add weather element if exists
  //////////////////////////////////

  // v3.303.12
  IXMLNode xWeather = xLegFromTemplate.getChildNode(mxconst::get_ELEMENT_WEATHER().c_str());
  if (!xWeather.isEmpty())
    xNewFlightLeg.addChild(xWeather.deepCopy());


  /////////////////////////////////////////////////////////////////
  ////// END SHARED INFORMATION/ELEMENTS to ALL <leg> types //////
  ///////////////////////////////////////////////////////////////

  ///////////// CHECK if Flight Leg TYPE needs to be Randomized ////////////////
  if (flight_leg_type_hover_land_or_start.empty())
  {
    this->setError("Found a <leg> template without type definition. skipping.");
    return IXMLNode::emptyIXMLNode;
  }

  // v3.0.219.3 - support for multi-Leg type to pick
  if (flight_leg_type_hover_land_or_start.find(mxconst::get_COMMA_DELIMITER()) != std::string::npos) // mxconst::get_COMMA_DELIMITER() = ","
  {
    std::vector<std::string> vecTypes = mxUtils::split_v2(flight_leg_type_hover_land_or_start, mxconst::get_COMMA_DELIMITER()); // mxconst::get_COMMA_DELIMITER() = ","
    if ( int nTypes = static_cast<int> ( vecTypes.size () )
        ; nTypes == 0)
    {
      this->setError("Found a <leg> template without type definition. skipping.");
      return IXMLNode::emptyIXMLNode;
    }
    else if (nTypes == 1)
    {
      flight_leg_type_hover_land_or_start = vecTypes.at(0);
    }
    else // random pick type
    {
      int picked = Utils::getRandomIntNumber(0, nTypes - 1);
      if (picked > nTypes)
        picked = nTypes - 1;

      flight_leg_type_hover_land_or_start = vecTypes.at(picked);
    }
  }


  ///////////// CHECK if LOCATION TYPE need to be Randomized ////////////////
  // v3.0.219.3 - support for multi-location vecTypeValues type to pick
  if (location_type.find(mxconst::get_COMMA_DELIMITER()) != std::string::npos) // mxconst::get_COMMA_DELIMITER() = ","
  {
    std::vector<std::string> vecTypes      = mxUtils::split_v2(location_type, mxconst::get_COMMA_DELIMITER());       // mxconst::get_COMMA_DELIMITER() = ","
    std::vector<std::string> vecTypeValues = mxUtils::split_v2(location_value_nm_s, mxconst::get_COMMA_DELIMITER()); // mxconst::get_COMMA_DELIMITER() = ","
    int                      picked        = 0;

    int       nTypes       = static_cast<int> ( vecTypes.size () );
    const int nTypesValues = static_cast<int> ( vecTypeValues.size () );

    Log::logDebugBO("[DEBUG pick <leg> type & value] vecTypes: " + Utils::formatNumber<size_t>(vecTypes.size()) + ", values:" + Utils::formatNumber<size_t>(vecTypeValues.size()), true);

    if (nTypes == 0)
    {
      this->setError("Found a location type with wrong definition. skipping.");
      return IXMLNode::emptyIXMLNode;
    }
    else if (nTypes == 1)
    {
      location_type = vecTypes.at(0);
      picked        = 0; // meaning first choice
    }
    else // random pick type
    {
      picked = Utils::getRandomIntNumber(0, nTypes - 1);
      if (picked > nTypes)
        picked = nTypes - 1;

      location_type = vecTypes.at(picked);
    }


    Log::logDebugBO("[DEBUG pick <leg> type] type picked: " + location_type, true);

    //// pick the relative value too. picked can't be bigger than the number of types
    if (nTypesValues >= nTypes || (picked < (nTypesValues - 1)))
      location_value_nm_s = vecTypeValues.at(picked);
    else if (nTypes >= 1 && nTypesValues == 1) // if we have few Types but only 1 location_value_nm_s, then it is shared between all of them
      location_value_nm_s = vecTypeValues.front();
    else
      location_value_nm_s.clear();


    if (location_value_nm_s == "_") // if special character that represent empty
      location_value_nm_s.clear();
  }


  #ifndef RELEASE
  Log::logDebugBO("[DEBUG random location info] location_type: " + location_type + ", location_value_nm_s=" + location_value_nm_s, true);
  #endif


  std::map<std::string, std::string> mapLocationSplitValues;
  std::vector<std::string>           vecLocationValueSplit_vec;
  vecLocationValueSplit_vec.clear();


  ///////////// CHECK if <leg> TYPE overrides location values ////////////////
  /// handle special cases where flight_leg_type (attrib template) is "start"
  if ((mxconst::get_FL_TEMPLATE_VAL_START() == flight_leg_type_hover_land_or_start) || (location_type == mxconst::get_FL_TEMPLATE_VAL_START())) // v3.0.221.15 consolidate if logic to one  // v3.0.221.7
  {
    flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_START();
    location_type                       = mxconst::get_FL_TEMPLATE_VAL_START();
    location_value_nm_s                 = mxconst::get_FL_TEMPLATE_VAL_START();
  }
  ////////// Check if has special instructions like: "nm=20|ramp=H|nm_between=10-20|tag={some name}"
  else if (!location_value_nm_s.empty())
  {
    //// v3.0.221.7 replace old logic with new more readable one
    // split between numbers and characters
    vecLocationValueSplit_vec = mxUtils::split_v2(location_value_nm_s, mxconst::get_PIPE_DELIMITER()); // "|"

    for (const auto &v : vecLocationValueSplit_vec)
    {
      std::vector<std::string> vecSplit = mxUtils::split_v2(v, "=");
      if (auto size_i = vecSplit.size(); size_i == 1) // for backwards compatibility, we handle cases where designer use: location_value="10 or _ or {some tag name}" without useing the explicit format
      {
        std::string attribName = Utils::stringToLower(vecSplit.at(0));
        std::string attribValue;

        // try to keep backwards compatibility if only number has been given and not: nm={number}
        if (Utils::is_number(attribName))
        {
          attribValue = attribName;
          attribName  = "nm";
        }
        else if (attribName == "_" || attribName.empty()) // handle special cases when only "_" is set or attribName is empty
        {
          attribValue = "_";
          attribName  = "nm";
        }
        else
        {
          attribValue = attribName;
          attribName  = "tag";
        }


        Log::logMsgErr(std::string("[random buildLeg] Found location value without explicit formating. Original format: ").append ( location_value_nm_s ).append ( ". Should be: " ).append ( attribName ).append ( "=" ) + attribValue + ". Skipping this directive.", true);
        // Utils::addElementToMap(mapLocationSplitValues, attribName, attribValue);
      }
      else if (size_i > 1)
      {
        std::string attribName  = Utils::stringToLower(vecSplit.at(0));
        const std::string& attribValue = vecSplit.at(1);
        Utils::addElementToMap(mapLocationSplitValues, attribName, attribValue);
      }
      else
        location_value_nm_s.clear();
    }

    location_value_nm_s.clear(); // ??? bug ??? When clearing the value we ignore the cases where it holds "tag=xxx" data and not any "nm=". This can fail get_target() function

    // prepare local variables according to the split information
    if (Utils::isElementExists(mapLocationSplitValues, "nm")) // represent distance in nm
      location_value_nm_s = mapLocationSplitValues["nm"];

    // replace "_" with empty string
    if (location_value_nm_s == "_") // v3.0.221.7 if special character that represent empty
      location_value_nm_s.clear();
  }

  Log::logDebugBO("[DEBUG random location info] location_value_nm_s=" + location_value_nm_s, true);

  //////////////////////////////////////////////////
  ///////// Get TARGET            /////////////////
  // create the objective based on <leg> type
  std::string radius_mt = (location_radius_mt < this->RADIUS_MT_MINIMUM_LENGTH) ? "" : Utils::formatNumber<int>(location_radius_mt); // v3.0.241.8 initialize with the override value designer decided.
  NavAidInfo  newNavInfo; // v3.0.241.8 deprecated "prevNavInfo"; // v3.0.221.2 moved outside of code block
  IXMLNode    xPoint = xNewFlightLeg.addChild(mxconst::get_ELEMENT_POINT().c_str());
  // prepare dummy point
  if (xPoint.isEmpty())
  {
    this->setError("[random flightLeg point] fail to add point element to flight leg. skipping <leg> creation.");
    return IXMLNode::emptyIXMLNode;
  }



  // ============= GET TARGET ===============
  // ============= GET TARGET ===============
  // ============= GET TARGET ===============
  // ============= GET TARGET ===============
  // ============= GET TARGET ===============



  const mx_which_type_to_force which_type_to_force_enum =
    (force_slope_i > 0) ? mx_which_type_to_force::force_hover
                        : (force_level_terrain_i > 0) ? mx_which_type_to_force::force_flat_terrain_to_land
                                                      : mx_which_type_to_force::no_force_is_needed;
  const int how_many_times_to_loop_i = (which_type_to_force_enum == mx_which_type_to_force::force_hover) ? force_slope_i
                                                      : (which_type_to_force_enum == mx_which_type_to_force::force_flat_terrain_to_land) ? force_level_terrain_i
                                                      : 0;

  // v3.0.221.15rc3 read new flag: "force <leg> type" at <leg> template level.
  bool flag_force_flight_leg_type = Utils::readBoolAttrib(xLegFromTemplate, mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B(), false); // relevant only in case we use "tag_name" and we pick <points> from it. Check if we have to force flight_leg_type on the random point that we might pick.

  // v3.0.221.15 rc3.2
  missionx::mx_base_node targetProp; // v3.305.1

  targetProp.setStringProperty ( mxconst::get_ATTRIB_NAME(), flightLegName ); // leg name
  targetProp.setStringProperty ( mxconst::get_ATTRIB_TYPE(), flight_leg_type_hover_land_or_start ); // leg type
  targetProp.setStringProperty ( mxconst::get_ATTRIB_LOCATION_TYPE(), location_type ); // location type
  targetProp.setBoolProperty ( mxconst::get_PROP_IS_LAST_FLIGHT_LEG(), this->flag_isLastFlightLeg ); // is last flight leg ?
  targetProp.setBoolProperty ( mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B(), flag_force_flight_leg_type ); // force leg type ?
  targetProp.setNodeProperty<int> ( mxconst::get_ATTRIB_FORCE_TYPE_OF_TEMPLATE(), static_cast<int> ( which_type_to_force_enum ) ); // force level terrain or slope ?
  targetProp.setNodeProperty<int> ( mxconst::get_PROP_NUMBER_OF_LOOPS_TO_FORCE_TYPE_TEMPLATE(), how_many_times_to_loop_i ); // force slope will be used with webosm

  int loop_counter_i = 0;

  do
  {
    if (!this->get_target(newNavInfo, in_legNodeFromTemplate, this->template_plane_type_enum, mapLocationSplitValues, targetProp))
    {
      return IXMLNode::emptyIXMLNode; // error message should have been set in get_target() function
    }
    // v3.0.253.6 check abort
    if (missionx::RandomEngine::threadState.flagAbortThread)
      return IXMLNode::emptyIXMLNode;

    // Check target duplication
    if (this->listNavInfo.size() > static_cast<size_t>(1) && this->check_if_new_target_is_same_as_prev(newNavInfo, this->listNavInfo.back()))
    {
      // skip target creation and try again.
      Log::logMsgThread(fmt::format("[RandomEngine] Found target [{}] same as previous one [{}]. Will skip and try a new one.",newNavInfo.getID(), listNavInfo.back().getID() ));
      newNavInfo.init(); // reset Nav Info
    }
    else // navaid is valid and not same as previous one
    {
      newNavInfo.synchToPoint();           // v3.0.241.10 b3 // hopefully will solve the crash when creating GPS
      xPoint = newNavInfo.node.deepCopy(); // v3.0.221.15 rc3.2
      if (xPoint.isEmpty())
      {
        this->setError("[random build <leg>]Failed to set Point coordinates. skipping <leg>: " + flightLegName);
        return IXMLNode::emptyIXMLNode;
      }

      // TODO: Consider removing the force directive since we will allow land + hover
      if (which_type_to_force_enum != mx_which_type_to_force::no_force_is_needed)
      {
        expected_slope_at_target_location_d = get_slope_at_point(newNavInfo);
        #ifndef RELEASE
        Log::logMsgThread("[force_slope] Slope Result: " + Utils::formatNumber<double>(expected_slope_at_target_location_d));
        #endif

        if (which_type_to_force_enum == mx_which_type_to_force::force_hover && expected_slope_at_target_location_d > missionx::data_manager::Max_Slope_To_Land_On)
        {
          flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_HOVER();
          #ifndef RELEASE
          Log::logMsgThread("[force_slope] Found slope in landing area: " + Utils::formatNumber<double>(expected_slope_at_target_location_d));
          #endif
          break;
        }
        else if (which_type_to_force_enum == mx_which_type_to_force::force_flat_terrain_to_land && expected_slope_at_target_location_d <= missionx::data_manager::Max_Slope_To_Land_On)
        {
          flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_LAND();
          #ifndef RELEASE
          Log::logMsgThread("[force_level] Found landing area: " + Utils::formatNumber<double>(expected_slope_at_target_location_d));
          #endif
          break;
        }
        else
        {
          Log::logMsgThread("[force_slope] Failed Slope test. Will try to fetch another target....");
        }
      }
    }
  } while (++loop_counter_i < how_many_times_to_loop_i && how_many_times_to_loop_i < 10); // end loop over force slope. Currently only 10 times are allowed


  // v3.0.253.9.1 fail flight leg if force failed
  if (which_type_to_force_enum == mx_which_type_to_force::force_hover && flight_leg_type_hover_land_or_start != mxconst::get_FL_TEMPLATE_VAL_HOVER())
  {
    Log::logMsgThread("[force_slope] Failed finding Slope in target area. Will fail current Flight Leg");
    return IXMLNode::emptyIXMLNode;
  }
  else if (which_type_to_force_enum == mx_which_type_to_force::force_flat_terrain_to_land && flight_leg_type_hover_land_or_start != mxconst::get_FL_TEMPLATE_VAL_LAND())
  {
    Log::logMsgThread("[force_slope] Failed Finding Landing terrain. Will fail current Flight Leg");
    return IXMLNode::emptyIXMLNode;
  }


  // v3.0.241.8 Build basic target message when entering its area. It could be changed if it is not the last flight leg, and we found out that we need to hover.
  if (this->flag_isLastFlightLeg)                                 // v3.0.241.8 different message to end flight leg
    triggerTargetMessage = "You reached the end of this flight."; // should be sent at the last target location.
  else
    triggerTargetMessage = "You reached the target area of this flight leg.";



  // 1. Check SLOPE for LAND leg type and WATER body that will affect flight_leg_type. Then check special point directives like "template" attribute that should override the "flight leg" template
  if ( (flight_leg_type_hover_land_or_start != mxconst::get_FL_TEMPLATE_VAL_START()) || (!this->flag_isLastFlightLeg) )
  {
    // ---------------------------------------
    // 1. check SLOPE if no navaid name
    // ---------------------------------------
    expected_slope_at_target_location_d = get_slope_at_point(newNavInfo); // v3.0.253.6 use the new wrapper function for that so code will be cleaner
    this->errMsg.clear();

    if (force_slope_i > 0 && expected_slope_at_target_location_d < missionx::data_manager::Max_Slope_To_Land_On) // v3.0.253.6
    {
      Log::logMsgThread("[force_slope] Failed Slope test. Will fail current Flight Leg");
      return IXMLNode::emptyIXMLNode;
    }

    // v3.0.223.2 check target slope only if template is LAND, and new Navinfo is empty, and we did not force template
    if ((flight_leg_type_hover_land_or_start == mxconst::get_FL_TEMPLATE_VAL_LAND()) && newNavInfo.getID().empty() && !newNavInfo.flag_force_picked_same_point_template_as_flight_leg_tempalte_type)
    {
      Log::logDebugBO("[DEBUG random slope] slope value: " + Utils::formatNumber<double>(expected_slope_at_target_location_d, 2), true);

      // define Leg type based on restricted slope
      if (expected_slope_at_target_location_d > missionx::data_manager::Max_Slope_To_Land_On)
      {
        flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_HOVER();
        #ifndef RELEASE
        Log::logMsgWarn( fmt::format("[random asses slope] Changed <leg> type to Hover due to slope being larger than: {}. Found slope in landing area: {}", Utils::formatNumber<float>(missionx::data_manager::Max_Slope_To_Land_On), Utils::formatNumber<double>(expected_slope_at_target_location_d)), true);
        #endif
      }

      Log::logDebugBO("[DEBUG buildFLightLeg] location: " + location_type + ", After slope decision", true);
    }


    // ---------------------------------------
    // 2. check WET
    // ---------------------------------------
    const bool isWet = this->get_is_wet_at_point(newNavInfo);
    if (!isWet)
    {
      this->setError("[random isWet] Failed to probe for wet. Will treat target coordinates as \"land\". ");
    }

    if (isWet)
    {
      // implement special case where user defined medevac + prop plane || float plane.
      auto plane_type_i = Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I(), static_cast<int>(missionx::mx_plane_types::plane_type_props)); // plane type

      xNewFlightLeg.updateAttribute("true", mxconst::get_PROP_IS_WET().c_str(), mxconst::get_PROP_IS_WET().c_str());
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool>(mxconst::get_PROP_IS_WET(), isWet); // v3.0.241.9 store if we have wet location
      // v3.0.241.9 extended ui design mission to allow ga planes to land in water without forcing hover
      if (flight_leg_type_hover_land_or_start != mxconst::get_FL_TEMPLATE_VAL_HOVER() &&
          !(this->flag_rules_defined_by_user_ui &&
            (plane_type_i >= static_cast<int>(missionx::mx_plane_types::plane_type_props) && plane_type_i <= static_cast<int>(missionx::mx_plane_types::plane_type_ga_floats))
           )
         ) // v3.0.223.2 will change to hover only if not using "force same template type"
      {
        if (!newNavInfo.flag_force_picked_same_point_template_as_flight_leg_tempalte_type)
          flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_HOVER();
      }
    }

    // ---------------------------------------
    // 3. Post <point> - construct template type/location description and radius of effect
    // ---------------------------------------
    if (!newNavInfo.flag_force_picked_same_point_template_as_flight_leg_tempalte_type) // v3.0.223.2 if we already picked based on same template then is no room for this test
    {
      // read template type from point and apply to NavAid and Leg node
      std::string pointTemplateType = Utils::stringToLower(Utils::readAttrib(xPoint, mxconst::get_ATTRIB_TEMPLATE(), EMPTY_STRING)); // v3.0.219.3 this will affect the flight leg type
      if ( !pointTemplateType.empty () && ( ( mxconst::get_FL_TEMPLATE_VAL_LAND() == pointTemplateType ) || ( mxconst::get_FL_TEMPLATE_VAL_HOVER() == pointTemplateType ) ) &&
          flight_leg_type_hover_land_or_start != pointTemplateType) // v3.0.219.3 override <leg> type only if different from current leg type
      {
        #ifndef RELEASE
        Log::logMsgWarn("[random asses Hover or Land]Forced point flight leg type from: '" + flight_leg_type_hover_land_or_start + "' to: '" + pointTemplateType + "'", true);
        #endif
        flight_leg_type_hover_land_or_start = pointTemplateType; // override flight leg type with suggestion in point
        xNewFlightLeg.updateAttribute(flight_leg_type_hover_land_or_start.c_str(), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE().c_str(), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE().c_str());
      }
    }

    radius_mt = Utils::readAttrib(xPoint, mxconst::get_ATTRIB_RADIUS_MT(), radius_mt);

    // v25.02.1 add point to target trigger - moved before the LAND_HOVER and HOVER logic
    Utils::xml_add_node_to_element_IXMLNode ( xLegTargetTrigger, xPoint, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA() ); // Target point shared between all flight_leg_type

    // ---------------------------------------
    // Handle LAND_HOVER template
    // ---------------------------------------
    // v25.02.1
    // Gather information from UI layer
    const auto med_cargo_or_oilrig_i = Utils::readNodeNumericAttrib<int> ( data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int> ( missionx::mx_mission_type::not_defined ) ); // 0 = med, 1 = cargo
    // const auto mission_subcategory_i = Utils::readNodeNumericAttrib<int> ( data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MISSION_SUBCATEGORY(), static_cast<int> ( missionx::mx_mission_subcategory_type::not_defined ) );

    #ifndef RELEASE
    auto bIsMandatory_debug = Utils::readBoolAttrib ( xLegTask, mxconst::get_ATTRIB_MANDATORY(), false );
    auto uiLayer_debug      = data_manager::getGeneratedFromLayer ();
    auto plane_type         = this->template_plane_type_enum;
    #endif

    if ( inFlightLegCounter == 1  // first leg, is also the target of the "flight leg"
           && med_cargo_or_oilrig_i          == static_cast<int> ( missionx::mx_mission_type::medevac )
           && this->template_plane_type_enum == missionx::mx_plane_types::plane_type_helos
           && data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_user_generates_a_mission_layer // Is this a user generated mission ? For now only "user generated" screen supports the "land_hover".
           && Utils::readBoolAttrib ( xLegTask, mxconst::get_ATTRIB_MANDATORY(), false ) // must be mandatory           
       )
      {
        flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER();
        xNewFlightLeg.updateAttribute ( flight_leg_type_hover_land_or_start.c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE().c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE().c_str () );
        xLegTask.updateAttribute ( flight_leg_type_hover_land_or_start.c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE().c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE().c_str () );

        const auto s_land_task_name = Utils::readAttrib ( xLegTask, mxconst::get_ATTRIB_NAME(), "" );

        // get target lat/lon
        int seq = 0;
        // calculate <display_object>, using 350 meters with 24 3D objects.
        auto vecLandTarget = this->calc_land_hover_display_objects ( newNavInfo.lat, newNavInfo.lon, 350, 24, seq );
        if (!vecLandTarget.empty ())
        {
          Utils::xml_add_comment ( xNewFlightLeg, "<<< Land 3D hint >>>" );
          for ( auto &xml : vecLandTarget )
            xNewFlightLeg.addChild ( xml.deepCopy () );
        }
        vecLandTarget.clear ();
        vecLandTarget = this->calc_land_hover_display_objects ( newNavInfo.lat, newNavInfo.lon, 50, 4, seq );
        if ( !vecLandTarget.empty () )
        {
          Utils::xml_add_comment ( xNewFlightLeg, "<<< Hover 3D hint >>>" );
          for ( auto &xml : vecLandTarget )
            xNewFlightLeg.addChild ( xml.deepCopy () );
        }
        Utils::xml_add_comment ( xNewFlightLeg, "<<< --------------- >>>" );
        // end <display_object>

        auto xHoverTask    = xLegTask.deepCopy ();
        auto xHoverTrigger = xLegTargetTrigger.deepCopy ();
        // set new names
        const auto hover_task_name    = fmt::format ( "leg_{}_hover_task", inFlightLegCounter );
        const auto hover_trigger_name = fmt::format ( "leg_{}_trig_hover", inFlightLegCounter );

        // set hover task info
        xHoverTask.updateAttribute ( hover_task_name.c_str (), mxconst::get_ATTRIB_NAME().c_str (), mxconst::get_ATTRIB_NAME().c_str () );
        xHoverTask.updateAttribute ( "true", mxconst::get_ATTRIB_MANDATORY().c_str (), mxconst::get_ATTRIB_MANDATORY().c_str () );
        xHoverTask.updateAttribute ( hover_trigger_name.c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str () );
        // set hover trigger info
        xHoverTrigger.updateAttribute ( hover_trigger_name.c_str (), mxconst::get_ATTRIB_NAME().c_str (), mxconst::get_ATTRIB_NAME().c_str () );
        xHoverTrigger.updateAttribute ( mxconst::get_TRIG_TYPE_RAD().c_str (), mxconst::get_ATTRIB_TYPE().c_str (), mxconst::get_ATTRIB_TYPE().c_str () );
        Utils::xml_set_attribute_in_node<bool> ( xHoverTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND(), false, mxconst::get_ELEMENT_CONDITIONS() );
        Utils::xml_set_attribute_in_node_asString ( xHoverTrigger, mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT(), fmt::format ( "---{}", mxconst::get_DEFAULT_HOVER_HEIGHT_FT() ), mxconst::get_ELEMENT_ELEVATION_VOLUME() );
        Utils::xml_set_attribute_in_node_asString ( xHoverTrigger, mxconst::get_ATTRIB_LENGTH_MT(), fmt::format ( "{}", 60 ), mxconst::get_ELEMENT_RADIUS() );


        // set the success for the crosscheck tasks, since both of them are mandatory.
        Utils::xml_set_attribute_in_node_asString ( xHoverTrigger, mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS(), s_land_task_name, mxconst::get_ELEMENT_OUTCOME() );
        Utils::xml_set_attribute_in_node_asString ( xLegTargetTrigger, mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS(), hover_task_name, mxconst::get_ELEMENT_OUTCOME() );
        // set the landing trigger radius
        Utils::xml_set_attribute_in_node_asString ( xLegTargetTrigger, mxconst::get_ATTRIB_LENGTH_MT(), fmt::format ( "{}", 500 ), mxconst::get_ELEMENT_RADIUS() );

        // Add the hover task and trigger to the template output
        xLegObjective.addChild ( xHoverTask );
        this->xTriggers.addChild ( xHoverTrigger, xTriggers.nChildNode () );

      }
    //////////////////
    //// Handle HOVER
    else if (mxconst::get_FL_TEMPLATE_VAL_HOVER() == flight_leg_type_hover_land_or_start)
    {
      if (location_radius_mt < this->RADIUS_MT_MINIMUM_LENGTH)
        radius_mt = (radius_mt == "500") ? radius_mt : "200"; //


      IXMLNode elevVolNode_ptr = Utils::xml_get_node_from_node_tree_IXMLNode(xLegTargetTrigger, mxconst::get_ELEMENT_ELEVATION_VOLUME(), false); // pointer to node
      if (elevVolNode_ptr.isEmpty())
      {
        this->setError("[random hover] Trigger template does not have <elevation_volume> element. Please fix.");
        return IXMLNode::emptyIXMLNode;
      }
      Utils::xml_search_and_set_attribute_in_IXMLNode(xLegTargetTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND(), mxconst::get_MX_FALSE(), mxconst::get_ELEMENT_CONDITIONS());
      const std::string elev_lower_upper_ft = (std::string("---") + Utils::formatNumber<int>(mxconst::get_DEFAULT_HOVER_HEIGHT_FT()));

      elevVolNode_ptr.updateAttribute(elev_lower_upper_ft.c_str(), mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT().c_str(), mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT().c_str()); // updating using the XML api do not work somehow. Should investigate.

      ////// try to read hover data entered by designer in <special_leg_directives>  ///////////////
      std::string            specialHoverData = Utils::readAttrib(xSpecialLegDirectives, mxconst::get_ATTRIB_HOVER_TIME_SEC_RANDOM(), mxconst::get_ATTRIB_DEFAULT_RANDOM_HOVER_TIME()); // "10-30"
      std::list<std::string> listHoverTimes   = Utils::splitStringToList(specialHoverData, "-");
      const std::string&     lowNum_s         = listHoverTimes.front();
      const std::string&     highNum_s        = listHoverTimes.back();
      int                    lowNum_i, highNum_i;
      lowNum_i  = 10;
      highNum_i = 30;

      if (Utils::is_number(lowNum_s))
        lowNum_i = Utils::stringToNumber<int>(lowNum_s);

      if (Utils::is_number(highNum_s))
        highNum_i = Utils::stringToNumber<int>(highNum_s);
      ///// end custom hover time //////////////////////////


      const int hoverTime = Utils::getRandomIntNumber(lowNum_i, highNum_i);
      xLegTask.updateAttribute(
        Utils::formatNumber<int>(hoverTime).c_str(), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str(), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str()); // updating using the XML api do not work somehow. Should investigate.

      Utils::xml_search_and_set_attribute_in_IXMLNode(xNewFlightLeg, mxconst::get_ATTRIB_HOVER_TIME(), Utils::formatNumber<int>(hoverTime), mxconst::get_ELEMENT_LEG());

      triggerTargetMessage = "Hover above target for " + Utils::formatNumber<int>(hoverTime) + " seconds.";
    }

    missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE(), flight_leg_type_hover_land_or_start); // , missionx::data_manager::prop_userDefinedMission_ui.node,                    missionx::data_manager::prop_userDefinedMission_ui.node.getName()); // v3.0.241.9 store if we have wet location


  } // end if not START flight_leg_type not LAST leg and NAV aid does not have same flag as flight_leg_type.
  //else
  //{
  //  // do specific "start" <leg> things
  //}

  if (!radius_mt.empty() && Utils::is_number(radius_mt))
    Utils::xml_search_and_set_attribute_in_IXMLNode(xLegTargetTrigger, mxconst::get_ATTRIB_LENGTH_MT(), radius_mt, mxconst::get_ELEMENT_RADIUS());

  // delete points that are not valid
  Utils::xml_delete_empty_nodes(xLegTargetTrigger);

  //// add point to target trigger, moved before evaluating HOVER and LAND_HOVER
  //Utils::xml_add_node_to_element_IXMLNode(xLegTargetTrigger, xPoint, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA()); // Target point shared between all flight_leg_type


  // add trigger to <triggers>
  xTriggers.addChild(xLegTargetTrigger, xTriggers.nChildNode());

  /////// End shared get_target() handlings ///////////////



  //////////////////////////////////////////
  // ADD DISPLAY_OBJECT                ////
  // check if there is any <display_object_set name="element name to pick from" template="medevac" /> example : <display_object_set name="object_sets" template="medevac" />
  // an <object set> comes before <display_objects>
  // Add <display_object> elements
  const auto lmbda_add_all_display_object_xxx_elements = [&] ( const IXMLNode & inParentNode, IXMLNode& inoutTargetNode)
  {
    const int nDisplayObjects = inParentNode.nChildNode();
    for (int i1 = 0; i1 < nDisplayObjects; ++i1)
    {
      // get sub-node
      auto cNode = inParentNode.getChildNode(i1).deepCopy();
      if (cNode.isEmpty())
        continue;

      // filter out sub-nodes that are not <display_xxx> elements
      std::string tag = cNode.getName();
      if (tag != mxconst::get_ELEMENT_DISPLAY_OBJECT() && tag != mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE())
        continue; // skip elements that are not <display_object> not <display_object_near_plane

      #ifndef RELEASE
      Log::logMsgThread(std::string("[random object_set]Adding 3D display_objects from: ") + tag + ": " + Utils::readAttrib(cNode, mxconst::get_ATTRIB_NAME(), "") );
      #endif

      if (this->parse_display_object_element(inoutTargetNode, cNode)) // v3.0.219.1 handle <display_object> options like: optional, random_water or limit_to_terrain_slope
      {
        if (tag == mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE())
        {
          //  Force replace_lat / replace_long dummy values to be on the safe side
          cNode.updateAttribute("1.0", mxconst::get_ATTRIB_REPLACE_LAT().c_str(), mxconst::get_ATTRIB_REPLACE_LAT().c_str());
          cNode.updateAttribute("1.0", mxconst::get_ATTRIB_REPLACE_LONG().c_str(), mxconst::get_ATTRIB_REPLACE_LONG().c_str());
        }

        inoutTargetNode.addChild(cNode.deepCopy(), inoutTargetNode.nChildNode());
      }
    }

  };



  if (IXMLNode xDisplayObjectSet = xLegFromTemplate.getChildNode(mxconst::get_ELEMENT_DISPLAY_OBJECT_SET().c_str())
    ; !xDisplayObjectSet.isEmpty())
  {
    std::string random_tag       = Utils::readAttrib(xDisplayObjectSet, mxconst::get_ATTRIB_RANDOM_TAG(), "");
    std::string set_name_to_pick = Utils::readAttrib(xDisplayObjectSet, mxconst::get_ATTRIB_SET_NAME(), "");

    if (mxconst::get_GENERATE_TYPE_MEDEVAC() == set_name_to_pick && expected_slope_at_target_location_d > (missionx::data_manager::Max_Slope_To_Land_On * 3.0f))
    {
      set_name_to_pick = mxconst::get_TERRAIN_TYPE_MEDEVAC_SLOPE();
      #ifndef RELEASE
      Log::logMsg("[random object_set]Replace object set type to: " + set_name_to_pick + ", due to slope: " + Utils::formatNumber<double>(expected_slope_at_target_location_d, 2), true);
      #endif
    }


    if (!random_tag.empty())
    {
      if (IXMLNode xTag = xRootTemplate.getChildNode(random_tag.c_str())
        ; !xTag.isEmpty())
      {
        int nChilds       = 0;
        int randomChild_i = -1;
        // check child tag
        if (set_name_to_pick.empty())
          nChilds = xTag.nChildNode();
        else
          nChilds = xTag.nChildNode(set_name_to_pick.c_str());

          /// Pick a child
        #ifndef RELEASE
        Log::logMsg("[random object_set]Search 3D set_name: " + set_name_to_pick, true);
        #endif

        if (nChilds > 0)
        {
          IXMLNode cTagNode;
          randomChild_i = Utils::getRandomIntNumber(0, nChilds - 1);
          if (set_name_to_pick.empty())
            cTagNode = xTag.getChildNode(randomChild_i);
          else
            cTagNode = xTag.getChildNode(set_name_to_pick.c_str(), randomChild_i);


          if (!cTagNode.isEmpty())
          {

            #ifndef RELEASE
            const int nDisplayObjects = cTagNode.nChildNode(mxconst::get_ELEMENT_DISPLAY_OBJECT().c_str()) + cTagNode.nChildNode(mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE().c_str());
            #endif
            lmbda_add_all_display_object_xxx_elements(cTagNode, xNewFlightLeg);


            #ifndef RELEASE
            if (nDisplayObjects == 0)
            {
              Log::logMsg(std::string("[random object_set]Failed to find valid display set: ") + cTagNode.getName() + ". Will try to search for <display_object> instead.", true);
            }
            #endif

          } // end if template tag name was found

        } // end if nChilds > 0

      } // end if xTag is not Empty

    } // end if tag_name is not empty
  }

  /// v3.303.11 - Breaking change, we always add <display_object> and <display_object_near_plane> even if we have a <display_set> used. It is up on the designer to make sure there are no duplications
  lmbda_add_all_display_object_xxx_elements(xLegFromTemplate, xNewFlightLeg); // v3.303.11 add all mxconst::ELEMENT_DISPLAY_OBJECT_xxx to flight_leg that are not part of the set

  #ifndef RELEASE
  Log::logMsg("[DEBUG random location info] Final location_type: " + location_type + ", location_value_nm_s=" + location_value_nm_s + "\n", true);
  #endif

  // --------------------------------
  // Continue with other shared elements that might need trigger location information
  // --------------------------------
  Utils::xml_search_and_set_attribute_in_IXMLNode(xNewFlightLeg, mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE(), flight_leg_type_hover_land_or_start, mxconst::get_ELEMENT_LEG()); // v3.0.192.3 add leg type for later use, like message construction
  Utils::xml_search_and_set_attribute_in_IXMLNode(xNewFlightLeg, mxconst::get_ATTRIB_TASK_TRIGGER_NAME(), triggerName, mxconst::get_ELEMENT_LEG()); // v3.0.192.2 store trigger name for later usage like calculate distance between two legs.

  // v3.0.219.3 set trigger target + message If they are not disabled
  if (!flag_disable_auto_messages)
  {
    IXMLNode xMessageOnTarget = xLegTargetTrigger.deepCopy(); // v3.0.219.3 adds target trigger message while keeping the original xLegTargetTrigger intact

    // v25.02.1 clear the <outcome> element after "copy" since it might hold unwanted attribute values
    auto xOutcome = xMessageOnTarget.getChildNode ( mxconst::get_ELEMENT_OUTCOME().c_str () );
    Utils::xml_clear_node_attributes_excluding_list ( xOutcome, "", true );

    Utils::xml_search_and_set_attribute_in_IXMLNode(xMessageOnTarget, mxconst::get_ATTRIB_NAME(), message_triggerTargetName, mxconst::get_ELEMENT_TRIGGER()); // v3.0.219.3
    Utils::xml_search_and_set_attribute_in_IXMLNode(xMapMessage, mxconst::get_ATTRIB_NAME(), message_triggerTargetName, mxconst::get_ELEMENT_MESSAGE ());    // v3.0.219.3 the trigger and message elements have the smane name. No conflict
    IXMLNode textMixNode = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(xMapMessage, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxconst::get_CHANNEL_TYPE_TEXT(), false); // direct pointer to Mix node
    #ifndef RELEASE
    Utils::xml_print_node(xMapMessage, true);
    #endif
    if (!textMixNode.isEmpty())
    {
      // remove or set relevant trigger attributes so message will fire when in physical area
      Utils::xml_search_and_set_attribute_in_IXMLNode ( xMessageOnTarget, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "" );

      //if (FL_TEMPLATE_VAL_HOVER == flight_leg_type_hover_land_or_start)
      if (flight_leg_type_hover_land_or_start.find(mxconst::get_FL_TEMPLATE_VAL_HOVER()) != std::string::npos ) // v25.02.1 is HOVER in string, since we have also "land_hover" cases      
        Utils::xml_search_and_set_attribute_in_IXMLNode(xMessageOnTarget, mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT(), "");
      

      Utils::xml_search_and_set_attribute_in_IXMLNode(xMessageOnTarget, mxconst::get_ATTRIB_RE_ARM(), mxconst::get_MX_YES(), mxconst::get_ELEMENT_TRIGGER());                                     // add rearm
      Utils::xml_search_and_set_attribute_in_IXMLNode(xMessageOnTarget, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED(), message_triggerTargetName, mxconst::get_ELEMENT_OUTCOME()); // add re-arm
      // add message
      Utils::xml_add_cdata(textMixNode, triggerTargetMessage);

      // add message to messages
      Utils::xml_add_node_to_element_IXMLNode(this->xMessages, xMapMessage);

      // add target trigger to <triggers>
      Utils::xml_add_node_to_element_IXMLNode(this->xTriggers, xMessageOnTarget);

      // Link to current Flight Leg
      IXMLNode linkNode = xNewFlightLeg.addChild(mxconst::get_ELEMENT_LINK_TO_TRIGGER().c_str());
      if (!linkNode.isEmpty())
        Utils::xml_search_and_set_attribute_in_IXMLNode(linkNode, mxconst::get_ATTRIB_NAME(), message_triggerTargetName, mxconst::get_ELEMENT_LINK_TO_TRIGGER());
    }
    else
    {
      this->setError("[random build] Warning: Failed to find <mix> sub-element in <message> element. Suggestion: Fix Mapping in template file. Will not create target message.");
    }
  } // end if not flag_disable_auto_messages


  // v3.0.219.7 Add inventory if any
  if (flight_leg_type_hover_land_or_start != mxconst::get_FL_TEMPLATE_VAL_START()) // skip if we return to starting location = "briefer"
    this->addInventory(flightLegName, xLegTargetTrigger);

  /// Store target radius in leg element
  const std::string target_radius_mt = Utils::xml_get_attribute_value_drill(xLegTargetTrigger, mxconst::get_ATTRIB_LENGTH_MT(), this->flag_found, mxconst::get_ELEMENT_RADIUS());
  if (!target_radius_mt.empty())
    xNewFlightLeg.updateAttribute(target_radius_mt.c_str(), mxconst::get_ATTRIB_LENGTH_MT().c_str(), mxconst::get_ATTRIB_LENGTH_MT().c_str());


  // v3.0.221.13
  if (newNavInfo.lat == 0.0 || newNavInfo.lon == 0.0)
  {
    #ifndef RELEASE
    Log::logMsg("[DEBUG new leg] Reject new <leg>: " + flightLegName + ", lat/long info seem 0 and not correct. Skip <leg> build...\n", true);
    #endif

    return IXMLNode::emptyIXMLNode;
  }

  // store xPoint in newNavInfo and later in lastFlightLegNavInfo
  lastFlightLegNavInfo.init();

  // calculate distance to next flight leg and store in Legs
  if (!this->listNavInfo.empty())
  {
    NavAidInfo   prev_na    = this->listNavInfo.back(); // prev nav aid
    const double distance   = Point::calcDistanceBetween2Points(prev_na.p, newNavInfo.p);
    std::string  distance_s = Utils::formatNumber<double>(distance, 3);
    xNewFlightLeg.updateAttribute(distance_s.c_str(), mxconst::get_ATTRIB_DISTANCE_NM().c_str(), mxconst::get_ATTRIB_DISTANCE_NM().c_str());

    if ((distance < static_cast<double>(mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT) && !this->flag_isLastFlightLeg /*v3.0.221.7*/) || (distance <= static_cast<double>(mxconst::DEFAULT_RANDOM_POINT_JUMP_NM)) /*v3.0.221.15*/)
    {
      #ifndef RELEASE
      Log::logMsg("[DEBUG new <leg>] Reject new <leg>: " + flightLegName + ", distance to <leg> is too short: " + Utils::formatNumber<double>(distance, 2) + "\n", true);
      #endif

      return IXMLNode::emptyIXMLNode;
    }
  }

  ///////////// Prepare Skewed position for GPS and target_markers based instances
  // v3.0.241.8
  const bool flag_display_target_markers_away_from_target = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET(), false);
  auto lmbda_get_skew_position = [&](IXMLNode inTargetPoint) {
    if (flag_display_target_markers_away_from_target && !this->flag_isLastFlightLeg && this->getPlaneType() == static_cast<uint8_t>(_mx_plane_type::plane_type_helos))
    {
      newNavInfo.flag_is_skewed = true; // v3.0.241.8
      return get_skewed_target_position(inTargetPoint).deepCopy();
    }

    return inTargetPoint.deepCopy();
  };

  Utils::xml_set_attribute_in_node<bool>(xPoint, mxconst::get_ATTRIB_IS_TARGET_POINT_B(), true, mxconst::get_ELEMENT_POINT()); // v3.0.241.8 A skewed point can still be a target so GPS points can be distinguished.
  newNavInfo.xml_skewdPointNode = lmbda_get_skew_position(xPoint.deepCopy());                                      // xPoint represents the real position.

  setInstanceProperties(xNewFlightLeg, newNavInfo, flag_display_target_markers_away_from_target); // v3.0.219.10  // v3.0.241.8 added newNavInfo since it holds target and skew. We also send the flag_display_target_markers_away_from_target

  // GPS GPS GPS GPS
  // Add to GPS
  if (!xGPS.isEmpty() && !xNewFlightLeg.isEmpty())
  {
    if (newNavInfo.flag_is_skewed) // already asked all question in the lmbda_get_skew_position()
    {
      xGPS.addChild(newNavInfo.xml_skewdPointNode.deepCopy());
      Log::logDebugBO("[random] Added GPS skewed target location due to setup preference.", true);
    }
    else
      xGPS.addChild(xPoint.deepCopy());

    // v3.0.221.4 add poi_tag point elements to GPS. They do not represent targets though.
    const std::string poi_tag = Utils::readAttrib(xPoint, mxconst::get_ATTRIB_POI_TAG(), "");
    if ( ! poi_tag.empty() )
    {
      int nChilds = this->xRootTemplate.getChildNode(poi_tag.c_str()).nChildNode(mxconst::get_ELEMENT_POINT().c_str());
      for (int i1 = 0; i1 < nChilds; ++i1)
      {
        IXMLNode p = this->xRootTemplate.getChildNode(poi_tag.c_str()).getChildNode(mxconst::get_ELEMENT_POINT().c_str(), i1).deepCopy();
        if (!p.isEmpty())
          xGPS.addChild(p.deepCopy());
      }
    }
  }


  // v3.0.221.7 add shared_type to flight leg node. Values should be medevac or delivery. Will be used mainly with external plugins using the custom datarefs: "xpshared/target/type"
  xNewFlightLeg.updateAttribute( "", mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE().c_str(), mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE().c_str());

  if (!xSpecialLegDirectives.isEmpty())                                    // v3.0.221.8
    Utils::xml_copy_node_attributes(xNewFlightLeg, xSpecialLegDirectives);

  //// v3.0.221.9 add target location to special directive node
  const std::string lat_s        = Utils::formatNumber<float>(newNavInfo.lat, 8);
  const std::string lon_s        = Utils::formatNumber<float>(newNavInfo.lon, 8);
  const std::string elev_mt_s    = Utils::formatNumber<float>(newNavInfo.height_mt, 2);
  const std::string target_loc_s = lat_s + "|" + lon_s + "|" + elev_mt_s;
  if (!xSpecialLegDirectives.isEmpty())
  {
    xSpecialLegDirectives.updateAttribute(target_loc_s.c_str(), mxconst::get_ATTRIB_TARGET_POS().c_str(), mxconst::get_ATTRIB_TARGET_POS().c_str());

    this->addTriggersBasedOnTargetLocation(newNavInfo, xSpecialLegDirectives); // v3.0.221.10 we send the newNavInfo class so we can assign the target location to the new added triggers
  }


  IXMLNode    xDescText         = Utils::xml_get_node_from_node_tree_IXMLNode(xLegFromTemplate, mxconst::get_ELEMENT_DESC(), true); // get a copy of DESC message (not the clear CDATA). Simple text between <desc> {text} </desc>
  std::string customLegDescText = Utils::xml_get_text(xDescText);
  if (!customLegDescText.empty())
  {
    // v3.0.221.11 refine leg message
    customLegDescText = this->prepare_message_with_special_keywords(newNavInfo, customLegDescText); // v3.0.223.4 replaced specific code to be in its own function to be used in other message code during Random mission creation.
    xSpecialLegDirectives.updateAttribute(mxconst::get_MX_YES().c_str(), mxconst::get_ATTRIB_CUSTOM_FLIGHT_LEG_DESC_FLAG().c_str(), mxconst::get_ATTRIB_CUSTOM_FLIGHT_LEG_DESC_FLAG().c_str());
  }


  //// v3.0.221.9 Add leg description
  IXMLNode xDesc = Utils::xml_get_node_from_node_tree_IXMLNode(xNewFlightLeg, mxconst::get_ELEMENT_DESC(), false);
  if (!xDesc.isEmpty())
  {
    xDesc.deleteNodeContent();
    xDesc = IXMLNode(); // v3.0.241.1
  }
  xDesc = xNewFlightLeg.addChild(mxconst::get_ELEMENT_DESC().c_str());

  // construct Flight Leg description
  std::string       desc;
  const std::string loc_desc = (newNavInfo.loc_desc.empty()) ? newNavInfo.get_locDesc() : newNavInfo.loc_desc;

  if (flight_leg_type_hover_land_or_start == mxconst::get_FL_TEMPLATE_VAL_START())
  {
    if (customLegDescText.empty())
      desc = "Fly back to " + loc_desc + ". expected distance: {distance}. Consult your GPS or map.";
    else
      desc = customLegDescText;
  }
  else
  {
    if (customLegDescText.empty())
      desc = "Fly to " + loc_desc + ". expected distance: {distance}. Consult your GPS or map.";
    else
      desc = customLegDescText;
  }
  desc = this->prepare_message_with_special_keywords(newNavInfo, desc); // v3.0.241.9

  // v3.0.253.4 add ways around target location to the end of the description. Restriction: target is now skewed
  if (const std::string ways_near_navaid_s = newNavInfo.parse_ways_around()
      ; !ways_near_navaid_s.empty() && newNavInfo.xml_skewdPointNode.isEmpty())
    desc += "\nAllChildNodes around location: " + ways_near_navaid_s;

  Utils::xml_add_cdata(xDesc, desc);
  //// end v3.0.221.9

  // Add leg_messages // v3.0.223.4
  // store disable auto leg messages in lastFlightLegNavInfo for later use in RandomEngine, like "injectMessagesWhileFlyingToDestination()"

  lastFlightLegNavInfo = newNavInfo; // v3.0.241.8 init lastFlightLegNavInfo before storing it
  lastFlightLegNavInfo.setBoolProperty(mxconst::get_ATTRIB_DISABLE_AUTO_MESSAGE_B(), flag_disable_auto_messages);


  const int nLegMessages = xLegFromTemplate.nChildNode(mxconst::get_DYNAMIC_MESSAGE().c_str());
  for (int i1 = 0; i1 < nLegMessages; ++i1)
  {
    IXMLNode xLegDynamicMessageNode = xLegFromTemplate.getChildNode(mxconst::get_DYNAMIC_MESSAGE().c_str(), i1);
    if (!xLegFromTemplate.isEmpty())
    {
      std::string text = Utils::xml_get_text(xLegDynamicMessageNode);
      if (!text.empty())
      {
        text = this->prepare_message_with_special_keywords(newNavInfo, text); // v3.0.223.4 replaced specific code to be in its own function to be used in other message code during Random mission creation.
        xLegDynamicMessageNode.deleteText();
        xLegDynamicMessageNode.addText(text.c_str());
      }
      xNewFlightLeg.addChild(xLegDynamicMessageNode.deepCopy());
    }
  }
  // End leg_messages

  // v3.0.253.7 add timer (only the first one)
  if (xLegFromTemplate.nChildNode(mxconst::get_ELEMENT_TIMER().c_str()) > 0)
    xNewFlightLeg.addChild(xLegFromTemplate.getChildNode(mxconst::get_ELEMENT_TIMER().c_str()).deepCopy());

  // v3.0.221.2 storing new Flight Leg into the original template xml leg
  lastFlightLegNavInfo.xLegFromTemplate = in_legNodeFromTemplate.deepCopy();
  if (!lastFlightLegNavInfo.xLegFromTemplate.isEmpty())
  {
    lastFlightLegNavInfo.xLegFromTemplate.updateAttribute(flightLegName.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
    lastFlightLegNavInfo.xLegFromTemplate.updateAttribute(flight_leg_type_hover_land_or_start.c_str(), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE().c_str(), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE().c_str());
    Utils::xml_search_and_set_attribute_in_IXMLNode(lastFlightLegNavInfo.xLegFromTemplate, mxconst::get_ATTRIB_LOCATION_TYPE(), location_type, mxconst::get_ELEMENT_EXPECTED_LOCATION());
    Utils::xml_search_and_set_attribute_in_IXMLNode(lastFlightLegNavInfo.xLegFromTemplate, mxconst::get_ATTRIB_LOCATION_VALUE(), location_value_nm_s, mxconst::get_ELEMENT_EXPECTED_LOCATION());
  }
  lastFlightLegNavInfo.flightLegName = flightLegName;
  lastFlightLegNavInfo.synchToPoint(); // store Flight Leg info.
  this->listNavInfo.emplace_back(lastFlightLegNavInfo);

  return xNewFlightLeg;
}

// --------------------------------

// --------------------------------

void
RandomEngine::fill_up_next_leg_attrib_after_flight_plan_was_generated()
{
  ///// Set next_leg attribute for <leg> element /////
  // Loop over all <legs> and place "next leg" in the correct attribute
  // we use the two maps: mapFlightPlanOrder_si and mapFlightPlanOrder_is. One map holds flight_leg and its index and the other map holds the index and the flight_leg. This way we can check if we have "index+1" in mapFlightPlanOrder_is and
  // therefore pick the next leg. There are other options to handle this, but that was quite simple although not as readable as, list container for example.
  const int nFlightLegChilds = xFlightLegs.nChildNode(mxconst::get_ELEMENT_LEG().c_str());
  for (int i1 = 0; i1 < nFlightLegChilds && !(missionx::RandomEngine::threadState.flagAbortThread); ++i1)
  {
    //bool              flag_found;
    IXMLNode          node     = xFlightLegs.getChildNode(i1);
    if ( const std::string leg_name = Utils::readAttrib ( node, mxconst::get_ATTRIB_NAME(), "" )
        ; ! leg_name.empty() )
    {
      const int seqInMap    = mapFlightPlanOrder_si[leg_name]; // get sequence number from flight leg

      // check if we have a flight <leg> with same name. If yes, then set next_leg to that name, if not then leave empty (means = END )
      if ( int next_leg_no = seqInMap + 1
          ; Utils::isElementExists(mapFLightPlanOrder_is, next_leg_no))
      {
        const std::string next_flight_leg = mapFLightPlanOrder_is[next_leg_no];
        Utils::xml_search_and_set_attribute_in_IXMLNode(node, mxconst::get_ATTRIB_NEXT_LEG(), next_flight_leg, mxconst::get_ELEMENT_LEG());
      }
    }

    if (missionx::RandomEngine::threadState.flagAbortThread) // v3.0.219.12+
    {
      this->setError("[Random] Aborted !!!");
      break;
    }
  }
}

// --------------------------------

void
RandomEngine::readOptimizedAptDatIntoCache()
{
#if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)

  if (missionx::data_manager::cachedNavInfo_map.empty()) // try to read from cached file if empty OR ignore it if user picked the option to ignore it
  {
    missionx::data_manager::queThreadMessage.emplace("!!! Please Wait while trying to read cached data file (only once per X-Plane session is needed) !!!");
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::set_briefer_text_message);
    // prepare data to send to parser
    std::string    customAptDat = data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", data_manager::errStr) + mxconst::get_FOLDER_SEPARATOR() + mxconst::get_CUSTOM_APT_DAT_FILE(); //"customAptDat.txt"; // v3.0.219.12+
    OptimizeAptDat opt;
    opt.parse_aptdat(&RandomEngine::threadState, customAptDat, "", true); // v3.0.253.6 added bool at the end to flag read for cached // v3.0.255.3 added "" as dummy value since it is not mandatory

    missionx::data_manager::queThreadMessage.emplace("Finished reading optimized cached file. Continue processing...");
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::set_briefer_text_message);
  } // end reading cached data from file
#endif

}


// --------------------------------

bool
RandomEngine::filterAndPickRampBasedOnPlaneType(missionx::NavAidInfo& navAid, std::string& outErrorMsg, const missionx::mxFilterRampType& inRampFilterType) //const bool& inIgnoreCenterOfRunwayAsRamp)
{
  char* zErrMsg = nullptr;

  std::string                     err;
  missionx::mx_aptdat_cached_info navData;
  auto                            aptNavLine = std::string(navAid.name);

  outErrorMsg.clear();

  if ((missionx::RandomEngine::threadState.flagAbortThread))
  {
    outErrorMsg = "Need to abort";
    return false;
  }

  mx_plane_types plane_type_enum_to_search = this->template_plane_type_enum;

  if (data_manager::db_xp_airports.db_is_open_and_ready)
  {
    int rc = 0;
    //// construct view query (inner query)
    // base on airports_vu
    // const std::string sql_ap =  "select icao_id, icao, ap_elev_ft, ap_name, ap_type, ap_lat, ap_lon " \
    //                             ", mx_calc_distance ( ap_lat, ap_lon," + mxUtils::formatNumber<double>(navAid.lat, 8) + ", " + mxUtils::formatNumber<double>(navAid.lon, 8) + ", 3440) as dist_nm, 0 as bearing " \
    //                             ", helipads, ramp_helos, ramp_planes, ramp_props, ramp_turboprops, ramp_jet_heavy, rw_hard, rw_dirt_gravel, rw_grass " \
    //                             ", rw_water, is_custom from airports_vu where 1 = 1 and icao = '" + navAid.getID() + "' order by dist_nm"; // we will pick the first result in the ordered result since it should reflect the closest airport based on its lat/lon

    // we will pick the first result in the ordered result since it should reflect the closest airport based on its lat/lon
    const std::string sql_ap = fmt::format( R"(select icao_id, icao, ap_elev_ft, ap_name, ap_type, ap_lat, ap_lon
                            , mx_calc_distance ( ap_lat, ap_lon, {}, {}, 3440) as dist_nm, 0 as bearing
                            , helipads, ramp_helos, ramp_planes, ramp_props, ramp_turboprops, ramp_jet_heavy, rw_hard, rw_dirt_gravel, rw_grass
                            , rw_water, is_custom from airports_vu where 1 = 1 and icao = '{}' order by dist_nm )"
                            , mxUtils::formatNumber<double>(navAid.lat, 8), mxUtils::formatNumber<double>(navAid.lon, 8), navAid.getID() );

    #ifndef RELEASE
    Log::logMsgThread("[get_random_airport_from_db] Query: \n" + sql_ap + "\n");
    #endif // !RELEASE

    // clear local cache
    RandomEngine::resultTable_gather_random_airports.clear();
    rc = sqlite3_exec(data_manager::db_xp_airports.db, sql_ap.c_str(), RandomEngine::callback_gather_random_airports_db, nullptr, &zErrMsg);
    if (rc != SQLITE_OK)
    {
      Log::logMsgThread("[filter and pick ramp] SQL error: " + std::string(zErrMsg));
      sqlite3_free(zErrMsg);
    }
    else
    {
      Log::logMsgThread("[filter and pick ramp] Information was gathered.");
      #ifndef RELEASE
      for (auto &[row_num, row_data] : RandomEngine::resultTable_gather_random_airports)
      {
        Log::logMsgThread(fmt::format("\tSeq: {}, icao_id: {}, icao: {}, Distance: {}", mxUtils::formatNumber<int>(row_num), row_data["icao_id"], row_data["icao"], row_data["dist_nm"] ) );
      }
      #endif // !RELEASE

      if (RandomEngine::resultTable_gather_random_airports.empty() )
      {
        Log::logMsgThread("[filter and pick ramp] No airports found relative to NavAid: " + navAid.getID());
        return false;
      }
      auto ap_row = ( *RandomEngine::resultTable_gather_random_airports.cbegin () ).second; // fetch the first result

      navAid.flag_is_custom_scenery = ( !( ap_row["is_custom"].empty () ) );

      // build the query based on plane types
      // we add space at the beginning of the filter
      for (int loop01 = 0; loop01 < 4; ++loop01)
      {
        std::string ramp_filter_stmt_s;
        switch (plane_type_enum_to_search)
        {
          case missionx::mx_plane_types::plane_type_any:
            ramp_filter_stmt_s = "";
            break;
          case missionx::mx_plane_types::plane_type_helos:
            ramp_filter_stmt_s = " and helos > 0 "; // pick all airports that have helos ramps (heliports or any airport with helos in it). The view we use calculated the number of helos ramps so it is easy to distinguish between them.
            break;
          case missionx::mx_plane_types::plane_type_ga_floats:
          case missionx::mx_plane_types::plane_type_ga:
          case missionx::mx_plane_types::plane_type_props:
            ramp_filter_stmt_s = " and props + turboprops > 0 and lower(for_planes) not like '%fighter%' "; // make sure only props locations are picked exclude "fighter" ramps
            break;
          case missionx::mx_plane_types::plane_type_turboprops:
            ramp_filter_stmt_s = " and props + turboprops > 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
            break;
          case missionx::mx_plane_types::plane_type_jets:
          case missionx::mx_plane_types::plane_type_heavy:
            ramp_filter_stmt_s = " and jet_n_heavy > 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
            break;
          case missionx::mx_plane_types::plane_type_fighter:
            ramp_filter_stmt_s = " and fighter > 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
            break;
          default:
            break;
        }

        const std::string select_s     = "select * from ramps_vu where 1 = 1 and icao_id = " + ap_row["icao_id"];
        const std::string filter_ramps = ramp_filter_stmt_s;
        const std::string sql          = select_s + filter_ramps + " ORDER BY RANDOM() limit 1";

        RandomEngine::resultTable_gather_ramp_data.clear();
        rc = sqlite3_exec(data_manager::db_xp_airports.db, sql.c_str(), RandomEngine::callback_pick_random_ramp_location_db, nullptr, &zErrMsg);
        if (rc != SQLITE_OK)
        {
          outErrorMsg = "Error during ramp search for plane type: " + translatePlaneTypeToString(plane_type_enum_to_search); // debug
          Log::logMsgThread("[pick ramp] SQL error: " + std::string(zErrMsg));
          sqlite3_free(zErrMsg);

          return false;
        }
        else
        {
          outErrorMsg.clear();

          if (RandomEngine::resultTable_gather_ramp_data.empty())
          {
            outErrorMsg = "No ramp was found for plane type: " + translatePlaneTypeToString(plane_type_enum_to_search) + ", should continue and search"; // debug

            if (missionx::mxFilterRampType::exact_plane_ramp_type == inRampFilterType)
              break; // exit the loop since we want the exact ramp type
            else if ( loop01 > 0) // if this is not the first iteration
            {
              // we try to search ramps that are "jets", then "turboprops" and then "prop".
              // we do not search for Helos, nor fighter ramps
              int i1 = static_cast<int> ( plane_type_enum_to_search );
              i1--;
              plane_type_enum_to_search = static_cast<missionx::mx_plane_types> ( i1 );
              outErrorMsg += ": try ramp type for: " + translatePlaneTypeToString(plane_type_enum_to_search); // debug
            }
            else
            {
              plane_type_enum_to_search = missionx::mx_plane_types::plane_type_jets;
            }

            Log::logMsgThread("[pick ramp] SQL error: " + outErrorMsg);

          }
          else
          {
            // Store ramp location in navaid
            Log::logMsgThread("[pick ramp] Ramp info gathered.");
            auto ramp                = (*resultTable_gather_ramp_data.cbegin()).second;
            navAid.lat               = mxUtils::stringToNumber<float>(ramp["lat"], ramp["lat"].length());
            navAid.lon               = mxUtils::stringToNumber<float>(ramp["lon"], ramp["lon"].length());
            navAid.heading           = mxUtils::stringToNumber<float>(ramp["heading"], ramp["heading"].length());
            navAid.ramp_info.uq_name = ramp["name"];
            navAid.ramp_info.jets    = ramp["for_planes"];

            #ifndef RELEASE
            for ( auto &row_val : resultTable_gather_ramp_data | std::views::values )
            {
              Log::logMsgThread("\rRamp: " + row_val["name"] + ", icao_id: " + row_val["icao_id"] + ", icao: " + row_val["icao"]);
            }
            #endif // !RELEASE

            // revert back the template type if and only if it is different. Main reason is if we do not find a ramp location for our plane then we try to find a ramp based on other plane types
            if (this->template_plane_type_enum != plane_type_enum_to_search)
              plane_type_enum_to_search = this->template_plane_type_enum;

            navAid.synchToPoint();
            return true; // exit the loop
          }
        } // end if airport result is not empty and we should search for ramp location

      } // end loop

      plane_type_enum_to_search = this->template_plane_type_enum; // copy back the original plane type

      // If we reached this location than we failed finding a valid ramp position.
      // fetch longest runway and return its center

      auto const lmbda_get_query_for_fallback_position_based_on_filter_type = [](const missionx::mxFilterRampType& inFilterType, missionx::NavAidInfo & inNavAid )
      {
        static const std::string MOVE_PLANE_IN_METERS{ "20" };

        switch (inFilterType)
        {
          case missionx::mxFilterRampType::start_ramp:
          {
            // place plane 5 meters from beginning of the runway
            //return "select t1.rw_no_1_lat || ',' || rw_no_1_lon as start_pos, t1.rw_no_1 as name, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading, t1.rw_length_mt from xp_rw t1 where t1.icao= '" + inNavAid.getID() + "' order by rw_length_mt desc limit 1";
            return "select mx_get_point_based_on_bearing_and_length_in_meters (t1.rw_no_1_lat, rw_no_1_lon, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon), " + MOVE_PLANE_IN_METERS +
                   ") as start_pos, t1.rw_no_1 as name, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading, t1.rw_length_mt from xp_rw t1 where t1.icao= '" + inNavAid.getID() + "' order by rw_length_mt desc limit 1";
          }
          break;
          case missionx::mxFilterRampType::any_ramp_location:
          case missionx::mxFilterRampType::end_ramp:
          {
            return "select mx_get_center_between_2_points(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as start_pos, t1.rw_no_1 || '-' || t1.rw_no_2 as name, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading, t1.rw_length_mt from xp_rw t1 where t1.icao= '" + inNavAid.getID() + "' order by rw_length_mt desc limit 1";
          }
          break;
          default:
            return std::string("");
        }

        return std::string("");
      };

      std::string query_start_pos_s = lmbda_get_query_for_fallback_position_based_on_filter_type(inRampFilterType, navAid);
      #ifndef RELEASE
      Log::logMsgThread("SQL Query to Fetch start pos: \n" + query_start_pos_s + "\n");
      #endif // !RELEASE

      if ( !query_start_pos_s.empty() )
      {
        resultTable_gather_ramp_data.clear();
        rc = sqlite3_exec(data_manager::db_xp_airports.db, query_start_pos_s.c_str(), RandomEngine::callback_pick_random_ramp_location_db, nullptr, &zErrMsg);
        if (rc != SQLITE_OK)
        {
          outErrorMsg = "No ramp was found for plane type: " + translatePlaneTypeToString(plane_type_enum_to_search);
          Log::logMsgThread("[pick ramp] SQL error: " + std::string(zErrMsg));
          sqlite3_free(zErrMsg);
        }
        else
        {
          outErrorMsg.clear();

          if (RandomEngine::resultTable_gather_ramp_data.empty())
            Log::logMsgThread("[pick ramp] No valid start position was found.");
          else
          {
            Log::logMsgThread("[pick ramp] Start position info gathered.");
            auto                     ramp        = (*resultTable_gather_ramp_data.cbegin()).second;
            std::vector<std::string> vecPosition = mxUtils::split(ramp["start_pos"], ',');

            if (vecPosition.size() > (size_t)1)
            {
              // Store location in Navaid
              navAid.lat               = mxUtils::stringToNumber<float>(vecPosition.at(0), vecPosition.at(0).length());
              navAid.lon               = mxUtils::stringToNumber<float>(vecPosition.at(1), vecPosition.at(1).length());
              navAid.heading           = mxUtils::stringToNumber<float>(ramp["heading"], 6);
              navAid.ramp_info.uq_name = ramp["name"];
              navAid.ramp_info.jets    = "Runway: " + navAid.ramp_info.uq_name;

              navAid.synchToPoint();
              return true;
            }
          } // end if we fetched the center of the runway as the ramp data

        } // end if sqlite statement is legit one

      } // end if we have query for fallback start position - either start of a runway or the center of the runway.

    } // end if airport information query returned data

  } // end if Database is open

  return false;
}

// --------------------------------

bool
RandomEngine::setInstanceProperties(IXMLNode& legNode_ptr, missionx::NavAidInfo& inTargetNavInfo, bool flag_place_target_markers_near_target)
{
  // Go over each <display_object> element, and validate its settings.
  // In  v3.0.219.1, we are adding "relative_pos_bearing_deg_distance_mt" attribute which is set as "bearing|distance in meters". If values are valid, then we should fix the "replace_lat/replace_lon" with the new values
  //////// Handle Instances ////////
  std::string err; // v3.0.223.1 replace the class variable with same name. BUG: it failed Flight Leg creation after calling get_target() function, although the error is not critical.

  Utils::xml_delete_empty_nodes(xDummyTopNode); // v3.0.219.3 remove invalid points

  // Prepare point information v3.0.219.5
  IXMLNode xPoint_ptr =
    inTargetNavInfo.p.node; // v3.0.241.8 using newNavInfo information instead of the xTargetTrigger// Utils::xml_get_node_from_node_tree_IXMLNode(xTargetTrigger, mxconst::get_ELEMENT_POINT(), false); // xPoint is a pointer to original Node

  // store trigger point
  double targetLat = Utils::readNumericAttrib(xPoint_ptr, mxconst::get_ATTRIB_LAT(), 0.0);
  double targetLon = Utils::readNumericAttrib(xPoint_ptr, mxconst::get_ATTRIB_LONG(), 0.0);
  // v3.0.219.5
  std::string              exclude_obj    = Utils::stringToLower(Utils::readAttrib(xPoint_ptr, mxconst::get_ATTRIB_EXCLUDE_OBJ(), "")); // convert to lower case
  std::string              include_obj    = Utils::readAttrib(xPoint_ptr, mxconst::get_ATTRIB_INCLUDE_OBJ(), "");
  std::vector<std::string> vecExclude     = mxUtils::split_v2(exclude_obj, mxconst::get_COMMA_DELIMITER()); // mxconst::get_COMMA_DELIMITER() = ","
  std::vector<std::string> vecInclude     = mxUtils::split_v2(include_obj, mxconst::get_COMMA_DELIMITER());
  int                      includeObjSize = static_cast<int> ( vecInclude.size () );

  std::set<std::string> setExclude;
  for (const auto &s : vecExclude)
    setExclude.insert(s);


  const int nChilds = legNode_ptr.nChildNode(); // v3.303.11 get all children
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    // get node and filter all nodes that are not DISPLAY_OBJECT type
    if (IXMLNode xNode = legNode_ptr.getChildNode(i1)
        ;!xNode.isEmpty())
    {
      // filter by tag name
      const std::string tagName = xNode.getName();
      if (tagName != mxconst::get_ELEMENT_DISPLAY_OBJECT() && tagName != mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE())
        continue;


      std::string obj3d_name = Utils::readAttrib(xNode, mxconst::get_ATTRIB_NAME(), "");
      /// v3.0.219.5 check and replace object in exclude list
      if (setExclude.contains(Utils::stringToLower(obj3d_name) ) )
      {
        // check if we can replace with include
        if (includeObjSize == 0)
        {
          Log::logMsg("[random instance] exclude 3D Object: " + obj3d_name + ", no replacement.", true);
          continue;
        }

        // pick randomly
        if (includeObjSize == 1)
          obj3d_name = vecInclude.at(0);
        else
        {
          int randNum = Utils::getRandomIntNumber(0, includeObjSize - 1);
          if (randNum >= includeObjSize) // to be on the safe side
            randNum = includeObjSize - 1;

          obj3d_name = vecInclude.at(randNum);
        }

        xNode.updateAttribute(obj3d_name.c_str(), mxconst::get_ATTRIB_NAME().c_str()); // change element name value
        xNode.deleteAttribute(mxconst::get_ATTRIB_INSTANCE_NAME().c_str()); // will be constructed next
      }
      // end v3.0.219.5 exclude/include objects

      std::string instName = Utils::readAttrib(xNode, mxconst::get_ATTRIB_INSTANCE_NAME(), ""); // v3.0.241.9 replaced code with simpler one
      if (instName.empty())
      {
        instName = obj3d_name + "_" + Utils::readAttrib(legNode_ptr, mxconst::get_ATTRIB_NAME(), "") + "_" + Utils::formatNumber<int>(i1);
        xNode.updateAttribute(instName.c_str(), mxconst::get_ATTRIB_INSTANCE_NAME().c_str(), mxconst::get_ATTRIB_INSTANCE_NAME().c_str()); // v3.0.241.9 replaced the addAttribute with update/add and position in element
      }

      // special validation and initialization of <display_object> element only
      if (tagName == mxconst::get_ELEMENT_DISPLAY_OBJECT())
      {
        std::string replaceLat                  = Utils::readAttrib(xNode, mxconst::get_ATTRIB_REPLACE_LAT(), "");
        std::string replaceLon                  = Utils::readAttrib(xNode, mxconst::get_ATTRIB_REPLACE_LONG(), "");
        std::string replaceElev_ft              = Utils::readAttrib(xNode, mxconst::get_ATTRIB_REPLACE_ELEV_FT(), "");
        int         replaceElevAboveGround_ft_i = Utils::readNodeNumericAttrib<int>(xNode, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT(), 0);


        // v3.0.219.1 implement location relative to target
        std::string            relative_pos_bearing_deg_distance_mt = Utils::readAttrib ( xNode, mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT(), "" );
        if ( const std::vector<int> vecRelativePos = Utils::splitStringToNumbers<int> ( relative_pos_bearing_deg_distance_mt, mxconst::get_PIPE_DELIMITER() )
            ; vecRelativePos.size() > 1)
        {
          double newLat, newLon, trigLat, trigLon, newBearing;
          newLat = newLon = trigLat = trigLon = newBearing = 0.0;

          if (targetLat != 0.0 && targetLon != 0.0)
          {
            // calculate new targetLat/long
            auto distance_nm = static_cast<double> ( vecRelativePos.at ( 1 ) ) * meter2nm;
            auto bearing     = static_cast<float> ( vecRelativePos.at ( 0 ) );
            Utils::calcPointBasedOnDistanceAndBearing_2DPlane(newLat, newLon, targetLat, targetLon, bearing, distance_nm);

            // set new targetLat/long in instance replace point data
            const std::string newInstanceLat_s = Utils::formatNumber<double>(newLat, 8);
            const std::string newInstanceLon_s = Utils::formatNumber<double>(newLon, 8);
            Utils::xml_search_and_set_attribute_in_IXMLNode(xNode, mxconst::get_ATTRIB_REPLACE_LAT(), newInstanceLat_s, mxconst::get_ELEMENT_DISPLAY_OBJECT());
            Utils::xml_search_and_set_attribute_in_IXMLNode(xNode, mxconst::get_ATTRIB_REPLACE_LONG(), newInstanceLon_s, mxconst::get_ELEMENT_DISPLAY_OBJECT());
          }
        }
        // v3.0.219.1 end relative location calculation

        // v3.303.14 [regression bug fix] reset relative value so plugin won't re-calculate it again when parsing the instance node.
        if (relative_pos_bearing_deg_distance_mt.empty() == false)
        {
          xNode.updateAttribute(relative_pos_bearing_deg_distance_mt.c_str(), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS().c_str(), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS().c_str()); // Keep the value in a debug attribute
          xNode.updateAttribute("", mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT().c_str(), mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT().c_str()); // reset the value
        }

        else if (!obj3d_name.empty()) // if we have no relative location information, then place at target position
        {
          // define replace_lat/replace_long WITH TARGET POSITION (LAT/LON) if one of them is not set
          if (replaceLat.empty() || replaceLon.empty())
          {
            const std::string targetLat_s = Utils::formatNumber<double>(targetLat, 8);
            const std::string targetLon_s = Utils::formatNumber<double>(targetLon, 8);
            Utils::xml_search_and_set_attribute_in_IXMLNode(xNode, mxconst::get_ATTRIB_REPLACE_LAT(), targetLat_s, mxconst::get_ELEMENT_DISPLAY_OBJECT());
            Utils::xml_search_and_set_attribute_in_IXMLNode(xNode, mxconst::get_ATTRIB_REPLACE_LONG(), targetLon_s, mxconst::get_ELEMENT_DISPLAY_OBJECT());
          }

          // v3.0.241.7 // v3.0.241.8 removed after the positioning of the 3D instance. See code below
          // set default above ground only if "replace_elev_ft" does not exist and attribute "replace_elev_above_ground_ft" exists
          if (replaceElev_ft.empty() && replaceElevAboveGround_ft_i != 0 )
          {
            const std::string replaceElevAboveGround_s = Utils::formatNumber<int>(replaceElevAboveGround_ft_i);
            Utils::xml_search_and_set_attribute_in_IXMLNode(xNode, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT(), replaceElevAboveGround_s, mxconst::get_ELEMENT_DISPLAY_OBJECT()); //
          }
        }

        // v3.0.241.8 place target instances not in their exact locations base on SETUP screen.
        //// Inject SETUP options for target_marker <display_object> and helos and if it is not the last target. TODO: Check if we can figure type of location (XY vs picked ones) and if we new the type of mission then we could also rule out
        /// delivery missions
        if ( const bool target_marker_b = Utils::readBoolAttrib ( xNode, mxconst::get_ATTRIB_TARGET_MARKER_B(), false )
            ; flag_place_target_markers_near_target
            && target_marker_b && !flag_isLastFlightLeg
            && this->getPlaneType() == static_cast<uint8_t> ( _mx_plane_type::plane_type_helos )
            && Utils::readNodeNumericAttrib<int>(missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int> ( missionx::_mission_type::medevac ) ) == static_cast<int> ( missionx::_mission_type::medevac ) )
        {
          if ( std::string skewed_name = Utils::readAttrib ( xNode, mxconst::get_ATTRIB_SKEWED_NAME(), "" )
              ; !skewed_name.empty())
            Utils::xml_search_and_set_attribute_in_IXMLNode(xNode, mxconst::get_ATTRIB_NAME(), skewed_name, mxconst::get_ELEMENT_DISPLAY_OBJECT());

          Utils::xml_search_and_set_attribute_in_IXMLNode(xNode, mxconst::get_ATTRIB_REPLACE_LAT(), Utils::readAttrib(inTargetNavInfo.xml_skewdPointNode, mxconst::get_ATTRIB_LAT(), mxconst::get_ZERO()), mxconst::get_ELEMENT_DISPLAY_OBJECT());
          Utils::xml_search_and_set_attribute_in_IXMLNode(xNode, mxconst::get_ATTRIB_REPLACE_LONG(), Utils::readAttrib(inTargetNavInfo.xml_skewdPointNode, mxconst::get_ATTRIB_LONG(), mxconst::get_ZERO()), mxconst::get_ELEMENT_DISPLAY_OBJECT());

          Utils::xml_set_attribute_in_node<bool>(xNode, "skewed_position", true, mxconst::get_ELEMENT_DISPLAY_OBJECT());
        }

      } // end if tag is DISPLAY_OBJECT
    } // end xNode valid
  }


  // v3.303.11 add special <display_object> directive to the "flight_leg"
  // Example: <display_object_near_plane name="crate01" instance_name="crate01_01" relative_pos_bearing_deg_distance_mt="{acf_psi}+90|{wing_span}+1" replace_lat="59.64266000" replace_long="-151.49297030" replace_distance_to_display_nm="10.00" target_marker_b="yes" replace_elev_ft=""/>

    #ifndef RELEASE
    int iDebugChilds = legNode_ptr.nChildNode(mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE().c_str());
    #endif

  return true;
}



void
RandomEngine::injectMissionTypeFeatures()
{
  // 1. LOOP over all flight legs
  // 1. add first Leg starting message - "hello pilot, check your GPS, fly to the landing site and pick the injured person."
  // 2. Loop over each flight leg and check if it has next flight leg, if so, then add message to check gps and fly to next leg. If not then construct last location message.
  // 3. Allow custom flight leg description
  std::string err;


  #ifndef RELEASE
  Log::logMsg("[DEBUG random] injectMissionTypeFeatures.", true);
  #endif

  const int nChilds = this->xFlightLegs.nChildNode(mxconst::get_ELEMENT_LEG().c_str());

  // REVERSE ordering so we could flag is Wet.
  bool flag_has_wet_target = false; // v3.0.241.8 help to flag
  for (int i1 = nChilds - 1; i1 >= 0; --i1)
  {
    IXMLNode leg_ptr = xFlightLegs.getChildNode(mxconst::get_ELEMENT_LEG().c_str(), i1); // pointer to <leg> xml element
    if (leg_ptr.isEmpty())
      continue;

    IXMLNode    msg         = Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE ()); // copy of <message> node, not a pointer
    IXMLNode    textMix_ptr = Utils::xml_get_node_from_node_tree_IXMLNode(msg, mxconst::get_ELEMENT_MIX(), false);                       // pointer to
    std::string message;
    message.clear();
    std::string message_name;
    message_name.clear();
    std::string flight_leg_name           = Utils::readAttrib(leg_ptr, mxconst::get_ATTRIB_NAME(), "");
    std::string loc_desc                  = Utils::readAttrib(leg_ptr, mxconst::get_ATTRIB_LOC_DESC(), ""); // short description of the random point generated. <leg> element should not have it anymore, since we store it in NavAid. We need to check <special_flight_leg_directive> element.
    std::string loc_desc_short            = loc_desc;        // short description of the random point generated. <leg> element should not have it anymore, since we store it in NavAid. We need to check <special_flight_leg_directive> element.
    double      distance_to_prev_navaid_d = -1.0;            // negative distance = invalid
    const bool  flag_isWet                = Utils::readBoolAttrib(leg_ptr, mxconst::get_PROP_IS_WET(), false);

    if (!flag_has_wet_target)
      flag_has_wet_target = flag_isWet;

    if (loc_desc.empty()) // v3.0.221.10 try to fetch location description from navaid with same flight leg name
    {
      NavAidInfo* prevNav    = nullptr; // v3.0.251.1 b2 add distance to flight leg description
      bool        bFirstTime = true;
      for (auto& nav : this->listNavInfo)
      {

        if (nav.flightLegName == flight_leg_name) // same unique Leg name
        {
          loc_desc       = (nav.loc_desc.empty()) ? nav.get_locDesc() : nav.loc_desc;
          loc_desc_short = nav.get_locDesc_short(); // +((nav.flag_nav_from_webosm) ? "(webosm)" : "");

          if (!bFirstTime) // we can calculate distance
          {
            distance_to_prev_navaid_d = nav.p - prevNav->p;
            prevNav                   = &nav;
          }

          break;
        }

        if (bFirstTime) // should be briefer NavAid
        {
          bFirstTime                = false;
          distance_to_prev_navaid_d = -1;
        }

        prevNav = &nav;
      }
    }

    // v3.0.221.11 search for custom flight leg message
    std::string customLegDescText;
    std::string custom_leg_desc_flag = Utils::stringToLower(Utils::xml_get_attribute_value_drill(leg_ptr, mxconst::get_ATTRIB_CUSTOM_FLIGHT_LEG_DESC_FLAG(), this->flag_found, mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES()));  // v3.0.221.15rc5 add LEG support
    if (mxconst::get_MX_YES() == custom_leg_desc_flag)
    {
      IXMLNode    xDesc = leg_ptr.getChildNode(mxconst::get_ELEMENT_DESC().c_str());
      std::string flightLegDesc;
      if (!xDesc.isEmpty())
        customLegDescText = ((xDesc.nClear() > 0) ? xDesc.getClear().sValue : missionx::EMPTY_STRING); // description of task: <task ...><![CDATA[task description]]></task>. // NO <desc> element
    }

    // v3.0.241.9 store leg locations in a string to display in the briefer.
    if (i1 == (nChilds - 1))                                                                                                                                            // our loop is from end to start
      cumulative_location_desc_s = loc_desc_short + ((distance_to_prev_navaid_d > -1) ? "(" + Utils::formatNumber<double>(distance_to_prev_navaid_d, 2) + " nm)" : ""); // v3.0.251.1 b2 add distances
    else
      cumulative_location_desc_s = loc_desc_short + ((distance_to_prev_navaid_d > -1) ? "(" + Utils::formatNumber<double>(distance_to_prev_navaid_d, 2) + " nm)" : "") + ", " + cumulative_location_desc_s;



    message_name = "leg_" + ((flight_leg_name.empty()) ? Utils::formatNumber<int>(i1) : flight_leg_name) + "_start_message";

    if (i1 == 0) // start <leg>. Create message
    {
      if (customLegDescText.empty())
      {
        if (flag_isWet)
          message = fmt::format("Hello pilot. We have uploaded flight coordinates to your GPS. One of the locations is above water body. Head to first location: {}", ((loc_desc.empty()) ? "" : fmt::format(R"("{}". Fly safe.)", loc_desc) ) );
        else
          message = "Hello pilot. We have uploaded flight coordinates to your GPS. Head to first location " + ((loc_desc.empty()) ? "" : fmt::format(R"("{}". Fly safe.)", loc_desc) );
      }
      else
        message = customLegDescText;
    }
    else
    {
      // Handle rest of flight legs
      const std::string next_flight_leg = Utils::readAttrib(leg_ptr, mxconst::get_ATTRIB_NEXT_LEG(), "");

      if ( customLegDescText.empty () )
      {
        if ( flag_found && next_flight_leg.empty () ) // if we found attrib and next_leg is empty then it means that we are at the last flight leg
        {
          message = "Fly to last GPS location " + ( ( loc_desc.empty () ) ? "" : fmt::format ( R"("{}". Land safely.)", loc_desc ) );
        }
        else if ( flag_isWet )
        {
          message = "Fly to the next GPS location " + ( ( loc_desc.empty () ) ? "(" + next_flight_leg + ")" : fmt::format ( R"("{}", it should be above water body.)", loc_desc ) ); // v3.0.241.8
        }
        else
          message = "Fly to the next GPS location " + ( ( loc_desc.empty () ) ? "(" + next_flight_leg + ")" : fmt::format ( R"("{}")", loc_desc ) );
      }
      else
        message = customLegDescText;
    }

    // Add the message to mission file and flight leg element
    if (textMix_ptr.isEmpty() || !Utils::xml_add_cdata(textMix_ptr, message))
    {
      #ifndef RELEASE
      Log::logMsgWarn("[random inject medevac] Message element is NULL. Check Template.", true);
      continue;
      #endif
    }

    Utils::xml_search_and_set_attribute_in_IXMLNode(msg, mxconst::get_ATTRIB_NAME(), message_name, mxconst::get_ELEMENT_MESSAGE ());
    this->xMessages.addChild(msg);
    // add as start_message to flight leg
    Utils::xml_search_and_set_attribute_in_IXMLNode(leg_ptr, mxconst::get_ATTRIB_NAME(), message_name, mxconst::get_ELEMENT_START_LEG_MESSAGE());
    Utils::add_xml_comment(xMessages, " [[[[ ]]]] "); // add comment between 2 messages
    // end setting XML with message data


  } // END LOOP over all flight leg


  //
  #ifndef RELEASE
  auto med_cargo_or_oilrig_i = Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int> ( missionx::mx_mission_type::not_defined ) );
  Log::logMsgThread("med_cargo_or_oilrig_i: " + mxUtils::formatNumber<int>(med_cargo_or_oilrig_i));
  #endif // !RELEASE

  if (Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int> ( missionx::mx_mission_type::not_defined ) ) == static_cast<int> ( missionx::mx_mission_type::oil_rig ) )
  {
    // add briefer start location.
    if (this->listNavInfo.empty() == false)
    {
      NavAidInfo na = this->listNavInfo.front();
      if (mxconst::get_ELEMENT_BRIEFER() == na.flightLegName)
        cumulative_location_desc_s = "(start): " + na.get_locDesc_short() + ", " + cumulative_location_desc_s;
    }
  }

  //// v3.0.241.9 Add custom briefer description if it is a mission UI based ("WinBrieferGL::user_driven_mission_layer") and its <![CDATA[ ]]> is empty.
  if (this->flag_rules_defined_by_user_ui)
  {
    std::string briefer_desc;
    if (!this->xBriefer.isEmpty())
      briefer_desc = ((xBriefer.nClear() > 0) ? xBriefer.getClear().sValue : ""); // description of task: <task ...><![CDATA[task description]]></task>. // NO <desc> element

    briefer_desc = mxUtils::trim(briefer_desc);
    if (briefer_desc.empty() && !xBriefer.isEmpty())
    {
      briefer_desc = this->briefer_skeleton_message_to_use_in_injectTypeMissionFeature;
      briefer_desc += (flag_has_wet_target) ? "\nOne of the flight legs is in a water body, make sure you have all needed equipment. " : "";
      // v25.02.1 adding support for LAND_HOVER cases
      if ( mxconst::get_FL_TEMPLATE_VAL_HOVER() == Utils::readAttrib ( missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE(), "" ) )
        briefer_desc += "\nWe believe you will have to hover above one of the locations, due to the physical terrain limitations.\nMake sure you have the right plane for this mission.";
      else if( mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER() == Utils::readAttrib ( missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE(), "" ) )
        briefer_desc += "\nWe believe you could Land or Hover above one of the locations, due to the physical terrain limitations.\nMake sure you have the right plane for this mission.";

      briefer_desc += "\nExpected Destinations: " + cumulative_location_desc_s + ".";
      briefer_desc += "\n\nFly Safe !!!";

      Utils::xml_add_cdata(xBriefer, briefer_desc);
    }
  }



  #ifndef RELEASE
  Log::logMsg("[DEBUG random] after injectMissionTypeFeatures.", true);
  #endif

  // end injectMissionTypeFeatures
}

// -----------------------------------------

void
RandomEngine::injectMessagesWhileFlyingToDestination()
{
  // Build messages relative to distance between 2 legs as a factor of distance.
  // prepare data before looping over legs
  // loop over legs and get distances between prev and post
  // v3.0.221.9 try to use the information in "special_flight_leg_directives" legs sub element.

  // find briefer point
  if (xBriefer.isEmpty() || xBriefer.getChildNode(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str()).isEmpty())
  {
    this->setError("[random message] Briefer node is not valid");
    return;
  }

  // prepare trigger node
  IXMLNode trig_template_node = Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::xmlMappingNode, mxconst::get_ELEMENT_TRIGGER()); // return copy of trigger node
  if (trig_template_node.isEmpty())
  {
    Log::logMsgWarn("[random message] Fail to find <trigger> in template", true);
    return;
  }

  // check for outcome node
  IXMLNode xOutcome = trig_template_node.getChildNode(mxconst::get_ELEMENT_OUTCOME().c_str());
  if (xOutcome.isEmpty())
    xOutcome = trig_template_node.addChild(mxconst::get_ELEMENT_OUTCOME().c_str());

  
  // prepare message node from template
  IXMLNode message_node = Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE ()); // return copy of Message node
  if ( message_node.isEmpty () )
  {
    Log::logMsgWarn ( "[random message] Fail to find <message> in template", true );
    return;
  }

  const int nLegChilds = this->xFlightLegs.nChildNode(mxconst::get_ELEMENT_LEG().c_str());
  int navCounter = 0;
  int legCounter = 0;


  NavAidInfo prevNa, currentNa;
  for (const auto &na : this->listNavInfo)
  {
    if (navCounter == 0)
    {
      ++navCounter;
      currentNa = na; // this should be the briefer
      continue;
    }

    prevNa.init();
    prevNa = currentNa;
    prevNa.synchToPoint();

    currentNa.init();
    currentNa = na;
    currentNa.synchToPoint();

    IXMLNode xmlDataNode_ptr;
    // get flight leg
    IXMLNode legNode = xFlightLegs.getChildNode(mxconst::get_ELEMENT_LEG().c_str(), legCounter);
    if (legNode.isEmpty())
      continue;

    if (legCounter >= nLegChilds) // should never happen
      break;

    // v3.0.223.4
    std::string err;
    const bool  flag_disable_auto_messages = currentNa.getBoolValue(mxconst::get_ATTRIB_DISABLE_AUTO_MESSAGE_B(), false); // v3.303.14r1 fixed reading the bool value. Changed from numeric read to also support string boolean representation

    xmlDataNode_ptr = legNode; // v3.0.221.9 // v3.0.223.4 Consider removing this line since we need to work only with <special_directive /> element.

    // search Special Flight Leg directive node.
    IXMLNode sNode = legNode.getChildNode(mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES().c_str());
    if (!sNode.isEmpty())
      xmlDataNode_ptr = sNode; // v3.0.221.9


    ++legCounter;
    ++navCounter;
    if (!flag_disable_auto_messages) // create or skip auto distance messages
    {

      // get trigger point
      #ifndef RELEASE
      std::string debugNaFlightLegName = currentNa.flightLegName; // debug - use with debugger
      #endif

      std::string flightLegName   = Utils::readAttrib(legNode, mxconst::get_ATTRIB_NAME(), "");
      std::string legTemplateType = Utils::readAttrib(xmlDataNode_ptr, mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE(), ""); // support leg and keep compatibility


      // v3.0.241.8 prepare two point classes based on the NavAidInfo.flag_is_skewed_point and calculate accordingly
      Point pCurr, pPrev;
      pCurr.node = (currentNa.flag_is_skewed) ? currentNa.xml_skewdPointNode : currentNa.p.node;
      pPrev.node = (prevNa.flag_is_skewed) ? prevNa.xml_skewdPointNode : prevNa.p.node;

      assert(!pCurr.node.isEmpty() && !pPrev.node.isEmpty());

      double distance_nm;
      bool   flag_msg_skewed = false; // v3.0.241.8 influence message
      if (currentNa.flag_is_skewed && pCurr.parse_node() && pPrev.parse_node())
      {
        flag_msg_skewed = true;
        distance_nm     = Point::calcDistanceBetween2Points(pCurr, pPrev);
      }
      else
        distance_nm = Point::calcDistanceBetween2Points(currentNa.p, prevNa.p); // fallback - may cause some errors, but its better then nothing


      if (distance_nm < 0.0)
      {
        Log::logMsgErr("[inject message] Found <leg> without distance_nm attribute. Skipping message for <leg>: " + flightLegName + ". Notify developer.", true);
        continue;
      }
      else
      {
        if (distance_nm < 2.0) // minimal message distance should be 2nm
          distance_nm = 2.0;


        /////////////////////////
        // add distance messages
        const std::string   message_distances = "2,5,15,25,40,60";
        std::vector<double> vecDistances      = Utils::splitStringToNumbers<double>(message_distances, mxconst::get_COMMA_DELIMITER()); // mxconst::get_COMMA_DELIMITER() = ","


        // comment separator
        Utils::add_xml_comment(xTriggers, " ++++ " + flightLegName + " distance messages +++++ "); // v3.0.219.3

        // loop over vector
        int counter = 0;
        for ( const auto dist : vecDistances)
        {
          IXMLNode trigNode = trig_template_node.deepCopy();
          if (trigNode.isEmpty())
            continue;

          if (!(distance_nm > dist) && !(distance_nm < 2.0)) // skip message that meant for longer distance. Ie, if distance to target is 5, then do not create message that is meant for 10nm.
            continue;

          // set trigger Name by distance
          const std::string newTriggerName = "message_trig_for_" + flightLegName + "_(" + Utils::formatNumber<double>(dist) + "nm)"; // message_trig_for_leg_1_(5nm)
          Utils::xml_search_and_set_attribute_in_IXMLNode(trigNode, mxconst::get_ATTRIB_NAME(), newTriggerName, mxconst::get_ELEMENT_TRIGGER());
          Utils::xml_search_and_set_attribute_in_IXMLNode(trigNode, mxconst::get_ATTRIB_PLANE_ON_GROUND(), missionx::EMPTY_STRING, mxconst::get_ELEMENT_CONDITIONS()); // remove on_ground attribute
          Utils::xml_search_and_set_attribute_in_IXMLNode(trigNode, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED(), newTriggerName, mxconst::get_ELEMENT_OUTCOME());    // set the message name as the "trigger name" for when_fired

          // set the message name when entering trigger zone
          std::string message = "You are: " + Utils::formatNumber<double>(dist) + " nautical miles from target.";
          if (counter == 0) // the closest message to target
          {
            if (mxconst::get_FL_TEMPLATE_VAL_HOVER() == legTemplateType)
            {
              if (flag_msg_skewed)
                message += " You should look for the target, we did not receive an exact location, it should be near. Remember to hover above it once you reached it."; // v3.0.241.8 added skewed string
              else
                message += " You should look for the target location to hover."; // v3.0.241.8 added skewed string
            }

            else if (mxconst::get_FL_TEMPLATE_VAL_LAND() == legTemplateType)
            {
              if (flag_msg_skewed)
                message += " target should be around this location. Once you locate it, land carefully."; // v3.0.241.8 added skewed string
              else
                message += " You are nearing your target destination."; // v3.0.241.8 modified message
            }
            else
            {
              message += " You should look for the target location."; // v3.0.241.8 added skewed string
            }
          }

          ++counter;

          // Define Triggers message Radius
          const int distance_mt = static_cast<int> ( dist * missionx::nm2meter );

          if ( IXMLNode rNode_ptr = Utils::xml_get_node_from_node_tree_IXMLNode ( trigNode, mxconst::get_ELEMENT_RADIUS(), false )
              ; rNode_ptr.isEmpty())
          {
            rNode_ptr = trigNode.addChild(mxconst::get_ELEMENT_RADIUS().c_str());
            Utils::xml_add_node_to_element_IXMLNode(trigNode, rNode_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA()); // place node in correct location
          }

          // set radius length attribute
          Utils::xml_search_and_set_attribute_in_IXMLNode(trigNode, mxconst::get_ATTRIB_LENGTH_MT(), Utils::formatNumber<int>(distance_mt), mxconst::get_ELEMENT_RADIUS());

          //// Define <message>
          IXMLNode mNode       = message_node.deepCopy();
          IXMLNode textMixNode = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(mNode, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxconst::get_CHANNEL_TYPE_TEXT(), false); // direct pointer to Mix node
          if (textMixNode.isEmpty())
          {
            Log::logMsgWarn("[random message] Fail to find <mix> in <message> template", true);
            return;
          }

          Utils::xml_search_and_set_attribute_in_IXMLNode(mNode, mxconst::get_ATTRIB_NAME(), newTriggerName); // message has same name as its trigger
          Utils::xml_add_cdata(textMixNode, message);
          // add message to <messages>
          Utils::xml_add_node_to_element_IXMLNode(this->xMessages, mNode);

          // set trigger Location
          IXMLNode pointNode = trigNode.addChild(mxconst::get_ELEMENT_POINT().c_str());
          if (pointNode.isEmpty())
            continue;

          Utils::xml_search_and_set_attribute_in_IXMLNode(pointNode, mxconst::get_ATTRIB_LAT(), Utils::formatNumber<double>(pCurr.getLat(), 8), mxconst::get_ELEMENT_POINT());
          Utils::xml_search_and_set_attribute_in_IXMLNode(pointNode, mxconst::get_ATTRIB_LONG(), Utils::formatNumber<double>(pCurr.getLon(), 8), mxconst::get_ELEMENT_POINT());

          if (!Utils::xml_add_node_to_element_IXMLNode(trigNode, pointNode, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA()))
          {
            Log::logMsgErr("[random message] Fail to add <point> to trigger. skipping trigger. ", true);
            continue;
          }

          // Add to Triggers element
          xTriggers.addChild(trigNode);

          // Link to current "flight leg"
          IXMLNode linkNode = legNode.addChild(mxconst::get_ELEMENT_LINK_TO_TRIGGER().c_str());
          if (!linkNode.isEmpty())
            Utils::xml_search_and_set_attribute_in_IXMLNode(linkNode, mxconst::get_ATTRIB_NAME(), newTriggerName, mxconst::get_ELEMENT_LINK_TO_TRIGGER());


        } // end loop over distance vector

      } // end flag_found or node is not empty

    } // end if to generate messages "if (!flag_disable_auto_messages)"

  } // end loop over flight legs
}


// -----------------------------------

void
RandomEngine::addInventory(const std::string& inFlightLegName, const IXMLNode & inSourceNode, const mxInvSource inSourceType)
{
  #ifndef RELEASE
  Log::logMsg("[DEBUG random airport] before <inventories> node.", true);
  #endif


  std::string invName = inFlightLegName + " Inventory"; // v24.06.1 changed from: "inv_" + inFlightLegName
  IXMLNode       xPoint;
  const IXMLNode xItemFromMap    = Utils::xml_get_node_from_node_tree_IXMLNode ( data_manager::xmlMappingNode, mxconst::get_ELEMENT_ITEM(), true ); // return copy of <item> node
  IXMLNode       xInv            = Utils::xml_get_node_from_node_tree_IXMLNode ( data_manager::xmlMappingNode, mxconst::get_ELEMENT_INVENTORY(), true ); // return copy of <inventory> node
  IXMLNode       xItemBlueprints = Utils::xml_get_node_from_node_tree_IXMLNode ( this->xRootTemplate, mxconst::get_ELEMENT_ITEM_BLUEPRINTS(), true ); // return copy of <item_blueprints> node from <TEMPLATE> instead of <MAPPING> element.
  // get pointer to inventory location and elevation
  IXMLNode xLocAndElev_ptr = Utils::xml_get_node_from_node_tree_IXMLNode(xInv, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA(), false); // return pointer of <loc_and_elev> node


  // v24.05.1 Read the blueprint items from the external file
  // Read from the external cargo_data.xml if we generate from "user creation screen" or if the blueprint is empty
  if (const std::string subCategory_type = missionx::data_manager::prop_userDefinedMission_ui.getStringAttributeValue (mxconst::get_PROP_MISSION_SUBCATEGORY_LBL(), "")
    ; !subCategory_type.empty () &&
      (data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_user_generates_a_mission_layer
      || data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::flight_leg_info
      || data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_ils_layer
      || data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_external_fpln_layer
      || xItemBlueprints.isEmpty()
      )
    )
  {
    auto xExternalNode = Utils::read_external_blueprint_items(mxconst::get_ELEMENT_CARGO(), mxconst::get_ELEMENT_ITEM_BLUEPRINTS(), subCategory_type, true, false);
    if (!xExternalNode.isEmpty())
      xItemBlueprints = xExternalNode;
  }


  if (mxconst::get_FL_TEMPLATE_VAL_START() == inFlightLegName) // v3.0.219.7 added skip legs by the name start, since it represent the briefer starting location.
    return;

  if (inSourceType == mxInvSource::trigger)
    xPoint = Utils::xml_get_node_from_node_tree_IXMLNode(inSourceNode, mxconst::get_ELEMENT_POINT(), true); // return copy of <point> node
  else {
    xPoint = inSourceNode.deepCopy (); // should be point
    xPoint.updateName (mxconst::get_ELEMENT_POINT().c_str ()); // v25.04.1
  }

  //// validations ////
  if (this->setInventories.contains(invName ) ) // If inventory exists, exit. This is not an error.
  {
    Log::logMsgWarn("[random inv] Inventory by the name: " + invName + ", exists, skipping", true);
    return;
  }

  if (xPoint.isEmpty() || xItemFromMap.isEmpty() || xInv.isEmpty() || xItemBlueprints.isEmpty() || xLocAndElev_ptr.isEmpty()) // handle trigger source validation
  {
    Log::logMsgErr("[random inv] One of the key elements could not be found. Check if there is any mapping for: \"<point>, <item>, <item_blueprints>, <inventory> and <loc_and_elev_data>\" nodes. skipping inventory creation... ", true);
    return;
  }

  // get number of items in <mapping> element
  const int itemsInBlueprint_i = xItemBlueprints.nChildNode();
  if (itemsInBlueprint_i == 0)
  {
    Log::logMsgWarn("[random inv] No items in <item_blueptint> node mapping. Please add <item> nodes to it for random pick.", true);
    return;
  }
  //// End validation ///


  //// Set Inventory information ////
  Utils::xml_search_and_set_attribute_in_IXMLNode(xInv, mxconst::get_ATTRIB_NAME(), invName, mxconst::get_ELEMENT_INVENTORY());

  // add inventory location
  xLocAndElev_ptr.addChild(xPoint);

  // add inventory radius
  const std::string length_mt = Utils::xml_get_attribute_value_drill(inSourceNode, mxconst::get_ATTRIB_LENGTH_MT(), this->flag_found, mxconst::get_ELEMENT_RADIUS()); // fetch radius if any. Trigger should have one
  if (flag_found)
    Utils::xml_search_and_set_attribute_in_IXMLNode(xInv, mxconst::get_ATTRIB_LENGTH_MT(), length_mt, mxconst::get_ELEMENT_RADIUS());
  else
    Utils::xml_search_and_set_attribute_in_IXMLNode(xInv, mxconst::get_ATTRIB_LENGTH_MT(), mxconst::get_DEFAULT_INVENTORY_RADIUS_MT(), mxconst::get_ELEMENT_RADIUS());

  // add items to inventory randomly
  int minNum = 0;
  int maxNum = itemsInBlueprint_i;
  if ( mxconst::get_ELEMENT_BRIEFER() == inFlightLegName )
  {
    minNum = 4;
    maxNum = 12;
  }

  const int numOfItemsToCreate_i = Utils::getRandomIntNumber(minNum, maxNum); // how many items should we create in inventory ?

  // v24.12.2
  std::unordered_map<std::string, IXMLNode> mapItemsInInv = {}; // [barcode, xml pointer]

  for (int i1 = 0; i1 < numOfItemsToCreate_i; ++i1)
  {
    const int pick_i = Utils::getRandomIntNumber(0, itemsInBlueprint_i - 1); // pick random item node

    IXMLNode newItem = xItemBlueprints.getChildNode(mxconst::get_ELEMENT_ITEM().c_str(), pick_i).deepCopy(); // get a copy of the item node
    if (newItem.isEmpty())
      continue;

    // v24.05.1 Skip if item attribute "name" or "barcode" are empty
    const std::string sBarcode = Utils::readAttrib(newItem, mxconst::get_ATTRIB_BARCODE(), ""); // v24.12.2
    if( Utils::readAttrib(newItem, mxconst::get_ATTRIB_NAME(), "").empty() || sBarcode.empty())
      continue;

    // v24.05.1 get original quantity
    const int originalQuantity_i = Utils::readNodeNumericAttrib<int>(newItem, mxconst::get_ATTRIB_QUANTITY(), -1);  // v24.05.1 read the quantity. "-1" means not found
    const int rndQuantity = Utils::getRandomIntNumber(1, ((originalQuantity_i > 0)?originalQuantity_i : 10) );  // pick random quantity

    // v24.12.2 Check for duplicate items based on barcode and merge their quantity
    if (mxUtils::isElementExists(mapItemsInInv, sBarcode) && !mapItemsInInv[sBarcode].isEmpty() )
    {
      const auto newQuantity = rndQuantity + Utils::readNodeNumericAttrib(mapItemsInInv[sBarcode], mxconst::get_ATTRIB_QUANTITY(), 0);
      mapItemsInInv[sBarcode].updateAttribute(mxUtils::formatNumber<int>(newQuantity).c_str(), mxconst::get_ATTRIB_QUANTITY().c_str(), mxconst::get_ATTRIB_QUANTITY().c_str() );
      #ifndef DEBUG
      Log::logMsgThread(fmt::format("\tMerge items: {} in {}.", sBarcode, invName)); // v24.12.2
      #endif                                                                            // !DEBUG

    }
    else
    {
      mapItemsInInv[sBarcode] = newItem;
      #ifndef DEBUG
      Log::logMsgThread(fmt::format("Added item: {} to {}.", sBarcode, invName)); // v24.12.2
      #endif                                                                            // !DEBUG
    }

  }

  // v24.12.2
  for ( const auto & nodeItem : mapItemsInInv | std::views::values) // Only iterate over values and not keys
  {
    xInv.addChild(nodeItem);
  }

    #ifndef RELEASE
    Log::logMsgThread("Added Inventory Content: \n");
    Utils::xml_print_node(xInv, true);
    #endif

  xInventoris.addChild(xInv);

  #ifndef RELEASE
  Log::logMsg("[DEBUG random airport] after <inventories> node.", true);
  #endif
}


// -----------------------------------
// -----------------------------------
// -----------------------------------

bool
RandomEngine::writeTargetFile()
{
  bool result = true;
  // Prepare path and file names // v3.0.241.10 b2 extended cases where template was picked from custom mission folder, therefore the output should be {mission folder name}.xml. That way we create uniquness

  const std::string savePathAndFile = (this->working_tempFile_ptr->missionFolderName.empty()) ? this->pathToRandomBrieferFolder + mxconst::get_FOLDER_SEPARATOR() + mxconst::get_RANDOM_MISSION_DATA_FILE_NAME()
                                                                                              : this->pathToRandomBrieferFolder + mxconst::get_FOLDER_SEPARATOR() + this->working_tempFile_ptr->missionFolderName + ".xml";


  const std::string_view mission_name_con = (!this->working_tempFile_ptr->missionFolderName.empty ())? this->working_tempFile_ptr->missionFolderName :  RandomEngine::threadState.dataString;
  #ifndef RELEASE
  Log::logMsgThread("\n[DEBUG random writeTargetFile] Write to file: " + savePathAndFile + "\n");
  #endif

  // v3.0.253.12 Adding all mission main element into a dedicated xTargetTopNode element instead of using the xDummyTopNode. This allows us to better control availability of main elements in final XML file
  IXMLNode xTargetTopNode = xTargetMainNode.addChild(mxconst::get_MISSION_ELEMENT().c_str());

  assert(!xTargetTopNode.isEmpty() && "[random:writeTargetFile] Fail to create the target <root> node.");
  // Set the attribute of the <MISSION> root element.
  const auto mission_file_format_s = Utils::readAttrib ( this->xRootTemplate, mxconst::get_ATTRIB_MISSION_FILE_FORMAT(), missionx::PLUGIN_FILE_VER ); // v25.03.1
  xTargetTopNode.addAttribute(mxconst::get_ATTRIB_VERSION().c_str(), mission_file_format_s.c_str() );
  xTargetTopNode.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), Utils::readAttrib(this->xDummyTopNode, mxconst::get_ATTRIB_NAME(), mission_name_con.data ()).c_str());
  xTargetTopNode.addAttribute(mxconst::get_ATTRIB_TITLE().c_str(), Utils::readAttrib(this->xDummyTopNode, mxconst::get_ATTRIB_TITLE(), "").c_str()); // v3.303.8

  if (missionx::data_manager::flag_setupEnableDesignerMode)
    xTargetTopNode.addAttribute(mxconst::get_ATTRIB_MISSION_DESIGNER_MODE().c_str(), "true" ); // v24.03.2 add designer mode attrib based on the SETUP screen


  ///// ORGENIZE <MISSION> Element children /////

  // add global setting from mapping
  if (this->xGlobalSettings.isEmpty()) // v3.0.303.2 add global_settings from Mapping only if it is empty.
    this->xGlobalSettings = data_manager::xmlMappingNode.getChildNode(mxconst::get_GLOBAL_SETTINGS().c_str()).deepCopy();

  // v3.303.9 add scoring
  if (!this->xScoring.isEmpty())
  {
    Utils::xml_delete_all_subnodes(this->xGlobalSettings, mxconst::get_ELEMENT_SCORING());
    this->xGlobalSettings.addChild(xScoring.deepCopy());
  }

  // v24.12.2 Handling compatibility flag. Will only affect template that do not have the "compatibility -> inventory_layout" set for them.
  if (missionx::data_manager::flag_setupUseXP11InventoryUI)
  {
    // ??? In case of compatibility flag, we use the PLUGIN_FILE_VER_XP11 to help force the inventory layout. ???
    xTargetTopNode.updateAttribute(missionx::PLUGIN_FILE_VER_XP11, mxconst::get_ATTRIB_VERSION().c_str(), mxconst::get_ATTRIB_VERSION().c_str() );

    if (this->xCompatibility.isEmpty())
      this->xCompatibility = this->xGlobalSettings.addChild(mxconst::get_ELEMENT_COMPATIBILITY().c_str());

    assert( !this->xCompatibility.isEmpty() && fmt::format("[{}], <compatibility> element can't be empty.", __func__).c_str() );
    if (Utils::readAttrib(this->xCompatibility, mxconst::get_ATTRIB_INVENTORY_LAYOUT(), "" ).empty() )
      this->xCompatibility.updateAttribute(mxUtils::formatNumber<int>(missionx::XP11_COMPATIBILITY).c_str(), mxconst::get_ATTRIB_INVENTORY_LAYOUT().c_str(), mxconst::get_ATTRIB_INVENTORY_LAYOUT().c_str() );

  }

  // v3.303.14 moved date/time and weather advance settings integration into the <global_settings> node to data_manager
  if (!missionx::RandomEngine::waitForPluginCallbackJob(missionx::mx_flc_pre_command::get_current_weather_state_and_store_in_RandomEngine))
  {
   missionx::RandomEngine::current_weather_datarefs_s.clear();
   this->setError("[random write weather] Failed to read current X-Plane weather information.");
  }
  missionx::data_manager::add_advanceSettingsDateTime_and_Weather_to_node(this->xGlobalSettings, missionx::data_manager::prop_userDefinedMission_ui.node, missionx::RandomEngine::current_weather_datarefs_s);

  IXMLNode xDrefStartColdAndDark = this->xRootTemplate.getChildNode(mxconst::get_ELEMENT_DATAREFS_START_COLD_AND_DARK().c_str()).deepCopy();
  if (!xDrefStartColdAndDark.isEmpty() && (Utils::readBoolAttrib(xRootTemplate, mxconst::get_ATTRIB_COPY_LEG_AS_IS_B(), false) == false)) // v3.0.303 add support for special words "{navaid_lat}" and "{navaid_lon}"
  {
    this->update_start_cold_and_dark_with_special_keywords(xDrefStartColdAndDark);
  }

  // v3.0.255.3 test validity of 3D Objects. Inject warning into Briefer
  std::string localErr;
  missionx::data_manager::validate_display_object_file_existence(savePathAndFile, xFlightLegs, xGlobalSettings, x3DObjTemplate, localErr);
  if (!localErr.empty())
  {
    this->setError(localErr);
  }

  if (!xBrieferInfo.isEmpty()) // v24026
  {
    if ( xBrieferInfo.isAttributeSet(mxconst::get_ATTRIB_SHORT_DESC().c_str()) )
      xBrieferInfo.deleteAttribute(mxconst::get_ATTRIB_SHORT_DESC().c_str()); // v24036 - delete short_desc attribute from <briefer_info> element.
  }

  // v24.12.1
  Utils::add_xml_comment(xTargetTopNode);
  xTargetTopNode.addChild(this->xMetadata);
  Utils::add_xml_comment(xTargetTopNode);
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(xBrieferInfo);
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(xGlobalSettings);
  Utils::add_xml_comment(xTargetTopNode, "++++ " + savePathAndFile + " ++++"); // v3.0.255.3 added savePathAndFile after global setting for easier debug

  xTargetTopNode.addChild(xBriefer);
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(xDrefStartColdAndDark); // v3.0.221.15 rc3.5 add start cold and dark
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(xFlightLegs);
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(xObjectives);
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(xTriggers);
  Utils::add_xml_comment(xTargetTopNode);

  Utils::add_xml_comment(xTargetTopNode, " Custom Template Messages ");
  xTargetTopNode.addChild(xMessages);
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(xInventoris); // v3.0.219.7
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(x3DObjTemplate);
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(xpData); // v3.0.221.10 can be empty element
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(this->xChoices); // v3.0.303 choices element
  Utils::add_xml_comment(xTargetTopNode);

  xTargetTopNode.addChild(xEmbedScripts); // v3.0.221.10 can be empty element
  Utils::add_xml_comment(xTargetTopNode);

  // v3.0.253.12 add <GPS> only if user asks for it
  auto const lmbda_should_we_add_gps = [&]() {
    if (data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_user_generates_a_mission_layer || data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_ils_layer ||
        data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_external_fpln_layer)
    {
      return Utils::readBoolAttrib(missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_GENERATE_GPS_WAYPOINTS(), true);
    }

    return true;
  };

  if (lmbda_should_we_add_gps())
  {
    xTargetTopNode.addChild(xGPS);
    Utils::add_xml_comment(xTargetTopNode);
  }

  //xTargetTopNode.addChild(xEnd);
  //  add end node from Template
  this->xEnd = this->xRootTemplate.getChildNode(mxconst::get_ELEMENT_END_MISSION().c_str()).deepCopy(); // v3.305.1 added template read first
  if (xEnd.isEmpty())                                                                             // try to read from mapping if we don't find one in the template
    this->xEnd = data_manager::xmlMappingNode.getChildNode(mxconst::get_ELEMENT_END_MISSION().c_str()).deepCopy();
  if (!xEnd.isEmpty())
    xTargetTopNode.addChild(xEnd);



  Utils::xml_delete_empty_nodes(xTargetTopNode); // v3.0.219.2 remove invalid points


  // v3.0.219.3 clear elements if in release
  #if defined RELEASE
  std::set<std::string> setLegAttribToClean  = { { mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE () }, { mxconst::get_ATTRIB_SHARED_GOAL_TEMPLATE () }, { mxconst::get_ATTRIB_LOC_DESC () }, { mxconst::get_ATTRIB_TASK_TRIGGER_NAME () }, { mxconst::get_ATTRIB_HOVER_TIME () } };
  std::set<std::string> setPointAttribToClean = { { mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE () }, { mxconst::get_ATTRIB_SHARED_GOAL_TEMPLATE () }, { mxconst::get_ATTRIB_RADIUS_MT () }, { mxconst::get_ATTRIB_LOC_DESC () } };

  Utils::xml_delete_attribute(xTargetMainNode, setLegAttribToClean, mxconst::get_ELEMENT_LEG());
  Utils::xml_delete_attribute(xTargetMainNode, setPointAttribToClean, mxconst::get_ELEMENT_POINT());
  #endif

  // Delete DUMMY node contents
  if (!this->xDummyTopNode.isEmpty())
    this->xDummyTopNode.deleteNodeContent();

  /////////////////////////////
  ////////////////////////////
  // Write to file  /////////
  IXMLRenderer  xmlWriter;
  IXMLErrorInfo errInfo = xmlWriter.writeToFile(this->xTargetMainNode, savePathAndFile.c_str(), "ASCII"); // "ISO-8859-1");
  if (errInfo != IXMLError_None)
  {
    std::string translatedError;
    translatedError.clear();

    translatedError = IXMLRenderer::getErrorMessage(errInfo);

    setError("[random] Error code while writing: " + translatedError + " (Check save folder is set: " + savePathAndFile + ")"); // v3.0.255.3 minor wording and save path modification

    missionx::RandomEngine::abortThread(); // v3.0.219.14
    result = false;
  } // end if fail to write

  if (result) // v25.03.1
    missionx::data_manager::missionState = missionx::mx_mission_state_enum::mission_was_generated;
  else
    missionx::data_manager::missionState = missionx::mx_mission_state_enum::mission_undefined;

  return result;
}

// -----------------------------------
// -----------------------------------
// -----------------------------------



int
RandomEngine::callback_gather_random_airports_db(void* data, const int argc, char** argv, char** azColName)
{
  int i;
  for (i = 0; i < argc; i++)
  {
    RandomEngine::row_gather_db_data[azColName[i]] = argv[i] ? argv[i] : "";
  }

  resultTable_gather_random_airports[static_cast<int> ( resultTable_gather_random_airports.size () )] = row_gather_db_data;
  row_gather_db_data.clear();
  return 0;
}

// -----------------------------------

int
RandomEngine::callback_pick_random_ramp_location_db(void* data, const int argc, char** argv, char** azColName)
{
  for ( int i = 0; i < argc; i++)
  {
    RandomEngine::row_gather_db_data[azColName[i]] = argv[i] ? argv[i] : ""; // if you have value return it if null then we put "" - empty string
  }

  resultTable_gather_ramp_data[static_cast<int> ( resultTable_gather_ramp_data.size () )] = row_gather_db_data;
  row_gather_db_data.clear();
  return 0;
}

// -----------------------------------
// -----------------------------------
// -----------------------------------

NavAidInfo
RandomEngine::get_random_airport_from_db(missionx::Point& inPoint, const float inMinDistance_nm, const float inMaxDistance_nm, const int inExcludeAngle)
{
  #ifndef RELEASE
  auto start_db_call = std::chrono::steady_clock::now();
  #endif
  /*
    select * from
    (
        select icao_id, icao, ap_name, mx_calc_distance ( ap_lat, ap_lon, -19.25418870, 146.77017290, 3440) as dist_nm, mx_bearing (ap_lat, ap_lon, -19.25418870, 146.77017290) as bearing from xp_airports
    ) v1
    where 1 = 1
    and dist_nm between 5.0 and 30.0
    and bearing between N1 and N2  <-- optional if bearing received is less than zero
  */

  missionx::NavAidInfo nav;

  // The point can be plane location or airport in the flight plan that we need to find other airport relative to it.
  const double pLat = inPoint.getLat();
  const double pLon = inPoint.getLon();

  //// construct view query (inner query)
  // base on xp_airports
   const std::string inner_view = "select icao_id, icao, ap_elev_ft, ap_name, ap_type, ap_lat, ap_lon, mx_calc_distance ( ap_lat, ap_lon," + mxUtils::formatNumber<double>(pLat, 10) + ", " + mxUtils::formatNumber<double>(pLon, 10) +
                                 ", 3440) as dist_nm, mx_bearing (ap_lat, ap_lon, " + mxUtils::formatNumber<double>(pLat, 10) + ", " + mxUtils::formatNumber<double>(pLon, 10) +
                                 ") as bearing, helipads, ramp_helos, ramp_planes, ramp_props, ramp_turboprops, ramp_jet_heavy, rw_hard, rw_dirt_gravel, rw_grass, rw_water, is_custom from airports_vu "; // v3.303.12 added field is_custom

  // Construct distance
  const std::string distance_s = " and dist_nm between " + mxUtils::formatNumber<float>(inMinDistance_nm) + " and " + mxUtils::formatNumber<float>(inMaxDistance_nm);

  //// Construct BEARING data
  const auto lmbda_get_bearing_string = [] ( const int in_ExcludeAngle) {
    if (in_ExcludeAngle < 0)
      return missionx::EMPTY_STRING;
    else
    {
      auto excludeAngle_tmp = in_ExcludeAngle;
      if (excludeAngle_tmp > -1)
      {
        excludeAngle_tmp -= 180; // we need to exclude the opposite direction of the original angle.
        if (excludeAngle_tmp < 0)
          excludeAngle_tmp += 360;
      }
      const int excludeAngle = excludeAngle_tmp;


      // create bigger bearing exclusion so we won't fetch those airports
      const int excludeAngle_Left  = (excludeAngle - 5 < 0) ? excludeAngle - 5 + 360 : excludeAngle - 5;
      const int excludeAngle_Right = (excludeAngle + 5 > 359) ? 360 - excludeAngle : excludeAngle + 5;

      if (excludeAngle_Left > excludeAngle_Right) // L > R
        return " and bearing between " + mxUtils::formatNumber<int>(excludeAngle_Right) + " and " + mxUtils::formatNumber<int>(excludeAngle_Left);
      else // L < R then (between 0 and L or between R and 360)
        return " and ( bearing < " + mxUtils::formatNumber<int>(excludeAngle_Left) + " or bearing > " + mxUtils::formatNumber<int>(excludeAngle_Right) + " )";
    }

    return missionx::EMPTY_STRING;
  };

  const std::string bearing_s = lmbda_get_bearing_string(inExcludeAngle);

  //// AND Bearing construct ////
  const auto lmbda_get_plane_filter_string = [] ( const missionx::mx_plane_types inPlaneType) {
    std::string stmt;
    switch (inPlaneType)
    {
      case missionx::mx_plane_types::plane_type_any:
        return missionx::EMPTY_STRING;
        break;
      case missionx::mx_plane_types::plane_type_helos:
        stmt = " and (helipads + ramp_helos) > 0 "; //
        break;
      case missionx::mx_plane_types::plane_type_ga_floats:
        stmt = " and ap_type in ( 1, 16 ) and ramp_planes > 0 ";
        break;
      case missionx::mx_plane_types::plane_type_ga:
      case missionx::mx_plane_types::plane_type_props:
        stmt = " and ap_type = 1 and ramp_props > 0 "; //
        break;
      case missionx::mx_plane_types::plane_type_turboprops:
        stmt = " and ap_type = 1 and ramp_turboprops > 0 "; //
        break;
      case missionx::mx_plane_types::plane_type_jets:
      case missionx::mx_plane_types::plane_type_heavy:
        stmt = " and ap_type = 1 and ramp_jet_heavy > 0 "; //
        break;
      default:
        break;
    }

    return stmt;
  };

  auto ramp_type_filter_s = lmbda_get_plane_filter_string( static_cast<missionx::mx_plane_types> ( this->getPlaneType () ) );


  // filter airports by runway types (if user asked for)
  const auto lmbda_filter_based_on_rw_type = [](const std::string& inFilter) {
    std::string stmt;
    if (!inFilter.empty())
    {
      stmt = " and 0 < ( select count(1) from xp_rw xr where xr.rw_surf in " + inFilter + " and xr.icao_id = vu.icao_id )";
    }

    return stmt;
  };

  const std::string subquery_to_filter_rw_type = lmbda_filter_based_on_rw_type(Utils::readAttrib(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FILTER_AIRPORTS_BY_RUNWAY_TYPE(), ""));


  // adding the rw filter to the query
  const std::string sql = "select * from ( " + inner_view + " ) vu where 1 = 1 " + distance_s + bearing_s + ramp_type_filter_s + subquery_to_filter_rw_type +
                          " order by RANDOM() limit 10"; // v3.0.255.3 use RANDOM() function to pick random row  // older option: any valid row ordered by distance " order by dist_nm" ;


  #ifndef RELEASE
  Log::logMsgThread("[get_random_airport_from_db] Query: " + sql);
  #endif // !RELEASE



  if (data_manager::db_xp_airports.db_is_open_and_ready)
  {
    char *zErrMsg = nullptr;

    // clear local cache
    RandomEngine::resultTable_gather_random_airports.clear();
    if ( int rc = sqlite3_exec ( data_manager::db_xp_airports.db, sql.c_str (), RandomEngine::callback_gather_random_airports_db, nullptr, &zErrMsg )
        ; rc != SQLITE_OK)
    {
      Log::logMsgThread("[get_random_airport_from_db] SQL error: " + std::string(zErrMsg));
      sqlite3_free(zErrMsg);
    }
    else
    {
      Log::logMsgThread("[get_random_airport_from_db] Information was gathered.");
      #ifndef RELEASE
      for (auto &[row_num, row_data] : RandomEngine::resultTable_gather_random_airports)
      {
        Log::logMsgThread("\tSeq: " + mxUtils::formatNumber<int>(row_num) + ", icao_id: " + row_data["icao_id"] + ", icao: " + row_data["icao"]);
      }
      #endif // !RELEASE


      // If there is data then pick a ramp
      if (!RandomEngine::resultTable_gather_random_airports.empty())
      {

        const auto lmbda_get_ramp_filter_based_on_plane_type = [](missionx::mx_plane_types inPlaneType) {
          std::string stmt;
          switch (inPlaneType)
          {
            case missionx::mx_plane_types::plane_type_any:
              stmt = "";
              break;
            case missionx::mx_plane_types::plane_type_helos:
              stmt = " and helos > 0 "; // pick all airports that have helos ramps (heliports or any airport with helos in it). The view we use calculated the number of helos ramps so it is easy to distinguish between them.
              break;
            case missionx::mx_plane_types::plane_type_ga_floats:
            case missionx::mx_plane_types::plane_type_ga:
            case missionx::mx_plane_types::plane_type_props:
            case missionx::mx_plane_types::plane_type_turboprops:
              stmt = " and props + turboprops > 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
              break;
            case missionx::mx_plane_types::plane_type_jets:
            case missionx::mx_plane_types::plane_type_heavy:
              stmt = " and jet_n_heavy > 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
              break;
            default:
              break;
          }

          return stmt;
        };

        auto row = (*RandomEngine::resultTable_gather_random_airports.cbegin()).second; // fetch the first result
        nav.setID(row["icao"]);
        nav.setName(row["ap_name"]);
        nav.flag_is_custom_scenery = ( !( row["is_custom"].empty () ) ); // v3.303.12 changed field name to is_custom

        const std::string elev_ft = row["ap_elev_ft"];
        nav.height_mt             = (elev_ft.empty()) ? 0.0f : mxUtils::stringToNumber<float>(elev_ft) * missionx::feet2meter;

        const std::string select_s     = "select * from ramps_vu where 1 = 1 and for_planes is not null and icao_id = " + row["icao_id"]; // v3.303.14 added "for_planes is not null" to narrow the airports to the ones that there are real ramps
        const std::string filter_ramps = lmbda_get_ramp_filter_based_on_plane_type( static_cast<missionx::mx_plane_types> ( this->getPlaneType () ) );
        const std::string sql_query    = select_s + filter_ramps + " ORDER BY RANDOM() limit 1";

          #ifndef RELEASE
            Log::logMsgThread("\n[pick ramp sql]\n" + sql_query + "\n"); // debug
          #endif // !RELEASE

        resultTable_gather_ramp_data.clear();
        const int rc1 = sqlite3_exec(data_manager::db_xp_airports.db, sql_query.c_str(), RandomEngine::callback_pick_random_ramp_location_db, nullptr, &zErrMsg);
        if (rc1 != SQLITE_OK)
        {
          Log::logMsgThread("[pick ramp] SQL error: " + std::string(zErrMsg));
          sqlite3_free(zErrMsg);
        }
        else
        {
          if (RandomEngine::resultTable_gather_ramp_data.empty())
            Log::logMsgThread("[pick ramp] No ramp was found.");
          else
          {
            Log::logMsgThread("[pick ramp] Ramp info gathered.");
            auto ramp             = (*resultTable_gather_ramp_data.cbegin()).second;
            nav.lat               = mxUtils::stringToNumber<float>(ramp["lat"], 12);
            nav.lon               = mxUtils::stringToNumber<float>(ramp["lon"], 12);
            nav.ramp_info.uq_name = ramp["name"];
            nav.ramp_info.jets    = ramp["for_planes"];

            #ifndef RELEASE
            for ( auto &ramp_data : resultTable_gather_ramp_data | std::views::values )
            {
              Log::logMsgThread("\ramp: " + ramp_data["name"] + ", icao_id: " + ramp_data["icao_id"] + ", icao: " + ramp_data["icao"]);
            }
            #endif // !RELEASE
          }
        }
      } // end if airport result is not empty and we should search for ramp location


    } // end if query returned value

  } // end if DB is open

  #ifndef RELEASE
  auto end_db_call = std::chrono::steady_clock::now();
  auto diff_cache  = end_db_call - start_db_call;
  auto duration    = std::chrono::duration<double, std::milli>(diff_cache).count();
  Log::logAttention("*** Finished get_random_airport_from_db. Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec)  ****", true);
  #endif // !RELEASE

  return nav;
}



// -----------------------------------


void
RandomEngine::gatherRandomAirport_mainThread(const Point& inPoint, float inMaxDistance_nm, int inExcludeAngle, float inStartFromDistance_nm)
{
  // TODO: deprecate this function
  assert(1 == 2 && "[gatherRandomAirport_mainThread] Should not be called.");

#if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)

  // do 360 degrees swipe in 10deg angle
  // search for airports in distance of "inMaxDistance/10nm", exclude the "opposite angle" we came from.
  // Count the num of airports in std::map and random pick.
  NavAidInfo dummy;
  float      addToSearchRadius_nm = 0.0f; // v3.0.219.9 affect how many nm to jump until reach max distance. It will be bigger as distance of search will grow

  int counter = 0; // define loop boundary of no more than 1000 iteration in loop

  // validate max distance, should be in the calling function and not here.
  inMaxDistance_nm = (inMaxDistance_nm < DEFAULT_RANDOM_POINT_JUMP_NM) ? DEFAULT_RANDOM_POINT_JUMP_NM : inMaxDistance_nm;
  inMaxDistance_nm = (inMaxDistance_nm < mxconst::MAX_DISTANCE_TO_SEARCH_AIRPORT) ? inMaxDistance_nm : mxconst::MAX_DISTANCE_TO_SEARCH_AIRPORT;
  float angleInRad = 0;

  #ifndef RELEASE
  Log::logMsg("[random gatherRandomAirport_mainThread] gathering random navaid", true);
  #endif

  double p_x = inPoint.getLat();
  double p_y = inPoint.getLon();

  // store unique names of our airports
  std::set<std::string> setStoredICAO; // setStoredICAO.clear();

  // calculate exclude Angle
  int excludeAngle = -1;
  if (inExcludeAngle > -1)
  {
    inExcludeAngle -= 180; // we need to exclude the opposite direction of the original angle.
    if (inExcludeAngle < 0)
      inExcludeAngle += 360;

    excludeAngle = inExcludeAngle;
  }

  if (inMaxDistance_nm < 30)
    addToSearchRadius_nm = 0.0f;
  else if (inMaxDistance_nm < 50)
    addToSearchRadius_nm = std::fabs(5.0f - mxconst::DEFAULT_RANDOM_DEGREES_TO_EXCLUDE); // we want jumps every 5nm
  else if (inMaxDistance_nm < 100)
    addToSearchRadius_nm = std::fabs(8.0f - mxconst::DEFAULT_RANDOM_DEGREES_TO_EXCLUDE); // we want jumps every 5nm
  else if (inMaxDistance_nm < 250)
    addToSearchRadius_nm = std::fabs(15.0f - mxconst::DEFAULT_RANDOM_DEGREES_TO_EXCLUDE); // we want jumps every 5nm
  else if (inMaxDistance_nm < 500)
    addToSearchRadius_nm = std::fabs(25.0f - mxconst::DEFAULT_RANDOM_DEGREES_TO_EXCLUDE); // we want jumps every 5nm
  else if (inMaxDistance_nm <= 5000)
    addToSearchRadius_nm = std::fabs(50.0f - mxconst::DEFAULT_RANDOM_DEGREES_TO_EXCLUDE); // we want jumps every 5nm
  else if (inMaxDistance_nm <= 15000)
    addToSearchRadius_nm = std::fabs(75.0f - mxconst::DEFAULT_RANDOM_DEGREES_TO_EXCLUDE); // we want jumps every 5nm
  else
    addToSearchRadius_nm = std::fabs(100.0f - mxconst::DEFAULT_RANDOM_DEGREES_TO_EXCLUDE); // we want jumps every 5nm

  NavAidInfo closestNavAid; // store airport data

  const int MAX_COUNTER_ALLOWED_I = 1500;

  //// Pick airports in circled area /////
  for (float deg = 0.0f; deg < 360.0f && counter < MAX_COUNTER_ALLOWED_I; (deg += 10.0f))
  {
    // skip search in certain restricted angles so we won't return to same location, although that can happen.
    // -1 = do not exclude angle
    if (excludeAngle > -1 && deg >= (excludeAngle - mxconst::DEFAULT_RANDOM_DEGREES_TO_EXCLUDE) && deg <= (excludeAngle += mxconst::DEFAULT_RANDOM_DEGREES_TO_EXCLUDE))
      continue;

    angleInRad = (float)(deg * 2.0f * ((float)PI) / ((float)NUM_CIRCLE_POINTS)) * RadToDeg; // convert angle to Radians

    // calculate point and pick airport near it
    for (float radius_nm = (float)inStartFromDistance_nm; ((radius_nm <= inMaxDistance_nm) && (counter < MAX_COUNTER_ALLOWED_I)); (radius_nm += (mxconst::DEFAULT_RANDOM_POINT_JUMP_NM + addToSearchRadius_nm)))
    {
      missionx::NavAidInfo navAid;
      ++counter;

      navAid.degRelativeToSearchPoint = deg; // v3.0.219.6

      double lat, lon;
      lat = lon = 0.0;

      Utils::calcPointBasedOnDistanceAndBearing_2DPlane(lat, lon, p_x, p_y, angleInRad, radius_nm);
      float lat_f = (float)lat; // convert to float
      float lon_f = (float)lon; // convert to float

      // we use the targetLat/targetLon to search for airport nav ref
      navAid.navRef = XPLMFindNavAid(NULL, NULL, &lat_f, &lon_f, NULL, xplm_Nav_Airport);

      // skip if no navaid found
      if (navAid.navRef == XPLM_NAV_NOT_FOUND)
      {
        #ifndef RELEASE
        Log::logMsg("Failed to find ICAO in lat: " + Utils::formatNumber<float>(lat_f, 8) + ", lon: " + Utils::formatNumber<float>(lon_f, 8), true);
        #endif
        continue;
      }

      if (Utils::isElementExists(this->mapNavAidsFromMainThread, navAid.navRef))
        continue;


      // fetch and store information on navAid if in correct distance
      XPLMGetNavAidInfo(navAid.navRef, &navAid.navType, &navAid.lat, &navAid.lon, &navAid.height_mt, &navAid.freq, &navAid.heading, navAid.ID, navAid.name, navAid.inRegion);


      if (Utils::isElementExists(data_manager::cachedNavInfo_map, navAid.getID())) // v3.0.253.6 add custom scenery information for future pick
      {
        if (data_manager::cachedNavInfo_map[navAid.getID()].isCustom)
        {
          Utils::addElementToMap(this->map_customScenery_XPLMNavRef_NavAidsFromMainThread, navAid.getID(), navAid.navRef);
          navAid.flag_is_custom_scenery = true;
        }
      }

      // add navaid to map for future use
      Utils::addElementToMap(this->mapNavAidsFromMainThread, navAid.navRef, navAid);

    } // end loop over navaids in the distance of that degree

  } // end loop over 360 deg

#endif
}


// -----------------------------------


float
RandomEngine::calc_slope_at_point_mainThread(NavAidInfo& inNavAid)
{
  missionx::NavAidInfo north, south, east, west, ne, nw, se, sw;
  const float          radius_in_nm = 20 * missionx::meter2nm; // 20-meter radius minimal radius. which means ~80 meter of land to test

  if (inNavAid.lat == 0 || inNavAid.lon == 0)
    return false;

  //// find point for each direction
  inNavAid.synchToPoint();

  // DEBUG

  missionx::Point::calcPointBasedOnDistanceAndBearing_2DPlane(inNavAid.p, north.p, 360, radius_in_nm);
  missionx::Point::calcPointBasedOnDistanceAndBearing_2DPlane(inNavAid.p, south.p, 180, radius_in_nm);
  missionx::Point::calcPointBasedOnDistanceAndBearing_2DPlane(inNavAid.p, east.p, 90, radius_in_nm);
  missionx::Point::calcPointBasedOnDistanceAndBearing_2DPlane(inNavAid.p, west.p, 270, radius_in_nm);

  north.p.setElevationMt(Point::getTerrainElevInMeter_FromPoint(north.p, north.p.probe_result));
  south.p.setElevationMt(Point::getTerrainElevInMeter_FromPoint(south.p, south.p.probe_result));
  east.p.setElevationMt(Point::getTerrainElevInMeter_FromPoint(east.p, east.p.probe_result));
  west.p.setElevationMt(Point::getTerrainElevInMeter_FromPoint(west.p, west.p.probe_result));


  north.syncPointToNav();
  south.syncPointToNav();
  east.syncPointToNav();
  west.syncPointToNav();


  //// calculate slope
  const double slopeNS = Utils::calcSlopeBetween2PointsWithGivenElevation ( north.lat, north.lon, south.lat, south.lon, ( fabs ( north.p.getElevationInFeet () - south.p.getElevationInFeet () ) ) );
  const double slopeWE = Utils::calcSlopeBetween2PointsWithGivenElevation (west.lat, west.lon, east.lat, east.lon, (fabs(west.p.getElevationInFeet() - east.p.getElevationInFeet())));

  #ifndef RELEASE
  Log::logMsgThread("[random calc_slope_at_point] finished. Slope: " + Utils::formatNumber<double>((slopeNS < slopeWE) ? slopeWE : slopeNS) + "\n");
  #endif // !RELEASE

  return static_cast<float> ( ( slopeNS < slopeWE ) ? slopeWE : slopeNS ); // return the biggest slope angle
}


// -----------------------------------

std::string
RandomEngine::translatePlaneTypeToString ( const mx_plane_types in_plane_type)
{

  if (Utils::isElementExists(this->mapPlaneEnumToStringTypes, in_plane_type))
    return this->mapPlaneEnumToStringTypes[in_plane_type];

  return ""; // v3.0.253.1 this->mapPlaneEnumToStringTypes[in_plane_type]; // should return empty string
}

// -----------------------------------

missionx::mx_plane_types
RandomEngine::translatePlaneTypeToEnum(const std::string& in_plane_type)
{
  if (Utils::isElementExists(this->mapPlaneStringTypesToEnum, in_plane_type))
    return this->mapPlaneStringTypesToEnum[in_plane_type];

  return this->mapPlaneStringTypesToEnum[EMPTY_STRING]; // should return any
}

// -----------------------------------

void
RandomEngine::setPlaneType(std::string inPlaneType)
{
  inPlaneType = Utils::stringToLower(inPlaneType);
  if (Utils::isElementExists(this->mapPlaneStringTypesToEnum, inPlaneType))
  {
    this->template_plane_type_enum = this->mapPlaneStringTypesToEnum[inPlaneType];
    this->randomPlaneType          = inPlaneType;
  }
  else
  {
    this->template_plane_type_enum = missionx::mx_plane_types::plane_type_any;
    this->randomPlaneType.clear();
  }
}

// -----------------------------------

void
RandomEngine::setPlaneType ( const mx_plane_types inPlaneType)
{
  this->randomPlaneType = this->translatePlaneTypeToString(inPlaneType);
  // v3.0.253.1 extended like setPlaneType(std::string) since we need also "this->template_plane_type_enum" to be initialized when searching for ramps
  if (Utils::isElementExists(this->mapPlaneStringTypesToEnum, this->randomPlaneType))
  {
    this->template_plane_type_enum = this->mapPlaneStringTypesToEnum[this->randomPlaneType];
  }
  else
  {
    Log::logMsgErr("[setPlaneType enum] Failed to find inPlaneType, will reset to any.", true);
    this->template_plane_type_enum = missionx::mx_plane_types::plane_type_any;
    this->randomPlaneType.clear();
  }
}

// -----------------------------------

uint8_t
RandomEngine::getPlaneType()
{
  return static_cast<uint8_t> ( this->template_plane_type_enum );
}


// -----------------------------------
void
RandomEngine::abortThread()
{

  if (missionx::RandomEngine::threadState.flagIsActive)
    missionx::RandomEngine::threadState.flagAbortThread = true;
}

// -----------------------------------

int
RandomEngine::get_num_of_flight_legs()
{
  int num = 0;
  if (!xFlightLegs.isEmpty())
    num = this->xFlightLegs.nChildNode(mxconst::get_ELEMENT_LEG().c_str());

  return num;
}


// -----------------------------------


bool
RandomEngine::get_target(NavAidInfo& outNewNavInfo, const IXMLNode & inLegFromTemplate, const mx_plane_types in_plane_type_enum, std::map<std::string, std::string>& inMapLocationSplitValues, missionx::mx_base_node& inProperties)
{
  /////
  // The function needs to pick the correct point from a set of rules.
  // To do that, we:
  // 1. read leg information from inProperties class:
  // 2. from location value which resides in "inMapLocationSplitValues" map container and was prepared in createLeg function like: nm=10|tag={}...
  // 3. Check if we have min/max distance to take into consideration
  //
  // If leg type = start it means we need to return to starting location. Pick information from listNavInfo.front()
  // If location value does not hold "nm" not "tag" information but our last leg is valid (has targetLat/long) then "find nearest ICAO" relative to last nav aid.
  // else if "lastLegNavInfo" is valid and ( ("location_value_tag_name_s" is  empty but "location_value_d > 0") OR  (location_minDistance_d && location_maxDistance_d have values) ) then
  //    if (EXPECTED_LOCATION_TYPE_XY) and (template_plane_type_enum == RandomEngine::mx_plane_types::plane_type_helos) && !flag_isLastLeg
  //       pick a point in the area. This is _not_ ICAO.
  //    else
  //       search NavAid location in radius (include the min/max restrictions) using: missionx::mx_flc_pre_command::gather_random_airport_mainThread
  //    else
  //       if location_value_tag_name_s has value and inLocationType = EXPECTED_LOCATION_TYPE_NEAR then search closest NavAid in tag to last position of plane.
  //       else if location_value_tag_name_s has value then pick <point> from the "tag" element. Take into consideration if designer ask to "flag_force_leg_type" so only points with attribute "template" and same type value can be picked.
  //               In this specific case, we need to flag the NavAid as forcedLegType so no "slope/wet" tests will be done.
  //
  /////

  // read options regarding target location
  // v3.0.241.7 // v3.0.241.8 added this->flag_force_template_distances_b to let designer force his "narrative" when it comes to distances.
  const bool flag_override_random_target_min_dist = (this->flag_force_template_distances_b)? false : missionx::system_actions::pluginSetupOptions.getBoolValue(mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE());

  // get hide cues option
  const auto slider_random_min_distance_value_d =
    Utils::getNodeText_type_1_5<double>(system_actions::pluginSetupOptions.node,
                                        mxconst::get_SETUP_SLIDER_RANDOM_TARGET_MIN_DISTANCE(),
                                        0.0);
  this->errMsg.clear();                       // we don't really care about the error

  // v3.0.241.10b2 read from property map to use later in code (using the node instead of the property)
  const std::string inFlightLegName          = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_NAME(), "");
  const std::string inTemplateType           = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_TYPE(), "");
  const std::string inLocationType           = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE(), "");
  const bool        flag_isLastFlightLeg     = Utils::readBoolAttrib(inProperties.node, mxconst::get_PROP_IS_LAST_FLIGHT_LEG(), false);

  /////////////////////////////////////////////////////////////////
  // prepare local variables according to the split information
  std::string       location_value_nm_s               = mxUtils::getValueFromElement(inMapLocationSplitValues, std::string("nm"), std::string(""));
  const std::string location_value_tag_name_s         = mxUtils::getValueFromElement(inMapLocationSplitValues, std::string("tag"), std::string(""));
  const std::string location_value_min_max_distance_s = mxUtils::getValueFromElement(inMapLocationSplitValues, std::string("nm_between"), std::string(""));

  // replace "_" with empty string
  if (location_value_nm_s == "_") // v3.0.221.7 if special character that represent empty
    location_value_nm_s.clear();

  outNewNavInfo.init();

  double location_value_d = -1.0;
  if (!location_value_nm_s.empty() && Utils::is_number(location_value_nm_s))
    location_value_d = Utils::stringToNumber<double>(location_value_nm_s, static_cast<int> ( location_value_nm_s.length () ) );

  Log::logDebugBO("[get_target] nm/location_value_d: " + Utils::formatNumber<double>(location_value_d, 2), true);

  // between distances
  double location_minDistance_d = 0.0, location_maxDistance_d = 0.0;

  // lambda get_ramp_for_plane_type
  const auto lmbda_get_ramp_for_plane_type = [&](missionx::NavAidInfo& out_new_nav_info, const float additional_search_radius_f = 10.f) {
    //if (!this->filterAndPickRampBasedOnPlaneType(outNewNavInfo, this->errMsg)) // try to get Navaid information. If we fail to find information, we ignore and continue with original xPoint data
    if (!this->filterAndPickRampBasedOnPlaneType(out_new_nav_info, this->errMsg, missionx::mxFilterRampType::airport_ramp)) // v3.303.12_r2 // try to get Navaid information. If we fail to find information, we ignore and continue with original xPoint data
    {
      // try to gather information of navaid relative to the nearest NavAid we did found
      out_new_nav_info.synchToPoint();
      this->errMsg.clear();
      this->shared_navaid_info.inMinDistance_nm = static_cast<float> ( planeLocation.calcDistanceBetween2Points ( out_new_nav_info.p ) );       // we know that the closest distance was not good for us, so we will use it as minimal search radius
      this->shared_navaid_info.inMaxDistance_nm = this->shared_navaid_info.inMinDistance_nm + additional_search_radius_f; // we add 10 nautical miles to the closest NavAid we found

      if (this->shared_navaid_info.inMinDistance_nm < this->shared_navaid_info.inStartFromDistance_nm)
        this->shared_navaid_info.inMinDistance_nm = this->shared_navaid_info.inStartFromDistance_nm;

      #ifndef RELEASE
      Log::logMsgThread("[random Lambda get ramp for plane] airport does not have valid ramps, will search for airports in distance: " + mxUtils::formatNumber<float>(this->shared_navaid_info.inMinDistance_nm, 2) + " and " + mxUtils::formatNumber<float>(this->shared_navaid_info.inMaxDistance_nm, 2));
      #endif // !RELEASE

      #ifdef IBM
      out_new_nav_info = this->get_random_airport_from_db(this->shared_navaid_info.p, this->shared_navaid_info.inMinDistance_nm, this->shared_navaid_info.inMaxDistance_nm, this->shared_navaid_info.inExcludeAngle); // v3.0.255.3 test integration
      #else
      const NavAidInfo nav = this->get_random_airport_from_db(this->shared_navaid_info.p, this->shared_navaid_info.inMinDistance_nm, this->shared_navaid_info.inMaxDistance_nm, this->shared_navaid_info.inExcludeAngle); // v3.0.255.3 test integration
      outNewNavInfo  = nav;
      #endif

      // Fallback code
      #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
      if (outNewNavInfo.lat == 0 || outNewNavInfo.lon == 0) // fallback code if DB logic failed
      {
        if (!this->waitForPluginCallbackJob(missionx::mx_flc_pre_command::gather_random_airport_mainThread, std::chrono::milliseconds(1000))) // pick random airport. Wait up to 10sec
        {
          this->setError("[random Lambda get ramp for plane Failed to find an airport in expected time. Skipping flight leg: " + inFlightLegName + "Consider sharing these findings with the developer... ");
          return false;
        }

        // Add find the closest airport to last location for location_type = NEAR
        if (inLocationType.compare(mxconst::get_EXPECTED_LOCATION_TYPE_NEAR()) == 0)
          this->getRandomAirport_localThread(outNewNavInfo, mxconst::get_EXPECTED_LOCATION_TYPE_NEAR());
        else
          this->getRandomAirport_localThread(outNewNavInfo); // pick random airport from list of valid locations
      }                                                      // end fallback code

      #endif // ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL


    } // end if found Random NavAid or ramp information

    return true;
  }; // end lmbda



  // Setup takes presidency to the "nm_between" attribute in the template BUT NOT the "nm" attribute in case of <element points>
  // v3.0.241.7 add min target distance settings
  if (flag_override_random_target_min_dist && slider_random_min_distance_value_d >= mxconst::SLIDER_MIN_RND_DIST && !this->flag_rules_defined_by_user_ui)
  {
    location_minDistance_d = slider_random_min_distance_value_d;
    location_maxDistance_d = mxconst::SLIDER_MAX_RND_DIST;
    if (location_minDistance_d > (mxconst::SLIDER_MAX_RND_DIST - 20.0)) // 20.0 represent the min nm expected distance the simmer should choose, before we add the maxSliderValue to it. Example, user picked expected distance "10nm" we
                                                                               // use the "maxSliderValue". If user picked "30nm" then we us "mxSlider + mxSlider" to have bigger area of min/max to search for NavAids
      location_maxDistance_d += mxconst::SLIDER_MAX_RND_DIST;           // increasing max distance by itself


  }                                                    // we check   location_value_min_max_distance_s only if SETUP screen do not override designer settings
  else if (!location_value_min_max_distance_s.empty()) // min-max // // v3.0.221.7
  {
    const std::vector<double> vecMinMax = Utils::splitStringToNumbers<double>(location_value_min_max_distance_s, "-, ");
    for (size_t i1 = 0; i1 < vecMinMax.size(); ++i1)
    {
      switch (i1)
      {
        case 0:
        {
          location_minDistance_d = vecMinMax.at(i1);
        }
        break;
        case 1:
        {
          location_maxDistance_d = vecMinMax.at(i1);
        }
        break;
        default:
          break;
      } // end switch
    }
  }

  // v24.12.2 Deprecated - code never used
  // if (this->listNavInfo.size() > static_cast<size_t>(0))
  // {
  //   NavAidInfo prevNavInfo = this->listNavInfo.back();
  //   prevNavInfo.synchToPoint(); // not sure if we need this
  // }

  //////////// end variables preparations ////////////

  if ((inTemplateType == mxconst::get_FL_TEMPLATE_VAL_START()) || (inLocationType == mxconst::get_FL_TEMPLATE_VAL_START())) // "start"
  {
    if (this->listNavInfo.empty())
    {
      this->setError("[random get_target] Failed to find starting location element. Fix Template. Aborting.");
      return false;
    }

    outNewNavInfo = this->listNavInfo.front();
    outNewNavInfo.synchToPoint();
    return true;
  }
  // If defined nothing then search for NEAR. Get nearest NavAid relative to last position if no location_value or location_tag_name were defined. Plane type is not relevant
  else if (location_value_nm_s.empty() && location_value_tag_name_s.empty() && location_value_min_max_distance_s.empty() /*v3.0.241.9*/ && (this->lastFlightLegNavInfo.lat != 0.0f && this->lastFlightLegNavInfo.lon != 0.0f))
  {
    #ifdef IBM
    outNewNavInfo = this->get_random_airport_from_db(this->lastFlightLegNavInfo.p, 3.0f, 50.0f, -1); // v3.0.255.3 test integration
    #else
    NavAidInfo nav = this->get_random_airport_from_db(this->lastFlightLegNavInfo.p, 3.0f, 50.0f, -1); // v3.0.255.3 test integration
    outNewNavInfo  = nav;
    #endif
    if (outNewNavInfo.lat == 0 || outNewNavInfo.lon == 0) // fallback code if DB logic failed
    {
      if (!missionx::RandomEngine::waitForPluginCallbackJob(missionx::mx_flc_pre_command::get_nearest_nav_aid_to_randomLastFlightLeg_mainThread))
      {
        this->setError("[random search nearest airport] Failed to find Airport NEAR given location. Skipping flight leg creation. ");
        return false;
      }
      outNewNavInfo = this->shared_navaid_info.navAid;
      // v3.0.253.7 try to better fetch ramp locations if the nearest ramp is not adequate for the plane type, like Sea runway for Helos
      // v3.0.253.7 This will hopefully solve the issue that we see an ambulance waiting on the water :-)
      if (!lmbda_get_ramp_for_plane_type(outNewNavInfo))
      {
        Log::logMsgThread(this->errMsg);
        return false;
      }
    }



    #ifndef RELEASE
    if (!this->errMsg.empty())
      Log::logMsgThread(errMsg);
    #endif
    this->errMsg.clear();

    outNewNavInfo.synchToPoint();
    outNewNavInfo.node.updateAttribute(
      mxconst::get_FL_TEMPLATE_VAL_LAND().c_str(), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE().c_str(), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE().c_str()); // override flight leg template type value to LAND since it is an icao
    outNewNavInfo.syncXmlPointToNav();

    if (outNewNavInfo.lat == 0.0 || outNewNavInfo.lon == 0.0)
    {
      this->setError("[random get_target last location] Failed to find an airport in radius. Try to generate the mission.");

      return false;
    }


    return true;
  }
  else if (this->lastFlightLegNavInfo.lat != 0.0f && this->lastFlightLegNavInfo.lon != 0.0f)
  {
    // check osm and UI template
    // DEBUG

    #ifndef RELEASE
    if (missionx::data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_user_generates_a_mission_layer) // display DEBUG info only if came from specific layer
    {
      Log::logMsgThread("Use OSM: " + Utils::readAttrib(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_OSM_CHECKBOX(), "NO"));
      Log::logMsgThread("Use WEB OSM: " + Utils::readAttrib(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_WEB_OSM_CHECKBOX(), "NO"));
    }
    #endif
    // if "user generates a mission" and they base it on Web/OSM and the plane is Helos, and it is a medevac and not last flight leg
    if (data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_user_generates_a_mission_layer &&
        (Utils::readBoolAttrib(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_OSM_CHECKBOX(), false) || Utils::readBoolAttrib(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_WEB_OSM_CHECKBOX(), false)) &&
        this->template_plane_type_enum == missionx::mx_plane_types::plane_type_helos && Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int> ( missionx::mx_mission_type::not_defined ) ) == static_cast<int> ( missionx::mx_mission_type::medevac ) && !flag_isLastFlightLeg)
    {
      // get max radius and find the 4 points that create the rectangle area
      const auto maxRadius_d   = Utils::readNodeNumericAttrib<double>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MAX_DISTANCE_SLIDER(), static_cast<int> ( mxconst::SLIDER_MAX_RND_DIST / 2 ) );
      const auto minDistance_d = Utils::readNodeNumericAttrib<double>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MIN_DISTANCE_SLIDER(), location_minDistance_d);

      Point E90, W270, S180, N0;
      this->calculate_bbox_coordinates(N0, S180, E90, W270, this->lastFlightLegNavInfo.lat, this->lastFlightLegNavInfo.lon, maxRadius_d);

      if (NavAidInfo navAid; this->osm_get_navaid_from_osm(navAid, inMapLocationSplitValues, inProperties, this->lastFlightLegNavInfo.lat, this->lastFlightLegNavInfo.lon, S180.lat, N0.lat, W270.lon, E90.lon, maxRadius_d, minDistance_d))
      {
        if (navAid.lat != 0.0 && navAid.lon != 0.0)
        {
          outNewNavInfo = navAid;
          outNewNavInfo.synchToPoint();
          this->flag_picked_from_osm_database = true; // we can use this
          return true;
        }
      }

      // if OSM data was not found then plugin will try to use default target search
    }



    if (inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OILRIG()) // v3.303.14 add OIL RIG type
    {
      outNewNavInfo.lat = Utils::readNodeNumericAttrib<float>(inLegFromTemplate, mxconst::get_ATTRIB_LAT(), 0.0f);
      outNewNavInfo.lon = Utils::readNodeNumericAttrib<float>(inLegFromTemplate, mxconst::get_ATTRIB_LONG(), 0.0f);
      outNewNavInfo.setID(Utils::readAttrib(inLegFromTemplate, mxconst::get_ATTRIB_ICAO_ID(), ""));
      outNewNavInfo.setName(Utils::readAttrib(inLegFromTemplate, mxconst::get_ATTRIB_AP_NAME(), ""));

      if (outNewNavInfo.lat != 0.0f && outNewNavInfo.lon != 0.0)
        return true;
      else
        return false;
    }
    else if ((inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM()) || (inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM()) ||
        (location_value_tag_name_s.empty() && ((location_value_d > 0.0) || (location_minDistance_d > 0.0 && location_maxDistance_d > 0.0)))) // v3.0.255.3 changed last logic
    {
      // Should we pick random location for HELOS
      if ((inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_XY() || inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM() || inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM()) &&
          this->template_plane_type_enum == missionx::mx_plane_types::plane_type_helos && !flag_isLastFlightLeg)
      {
        return get_targetForHelos_based_XY_OSM_OSMWEB(outNewNavInfo, in_plane_type_enum, inMapLocationSplitValues, inProperties, location_value_d, location_minDistance_d, location_maxDistance_d);
      }
      else
      {
        return get_target_or_lastFlightLeg_based_on_XY_or_OSM(outNewNavInfo, in_plane_type_enum, inMapLocationSplitValues, inProperties, location_value_d, location_minDistance_d, location_maxDistance_d);
      } // end handle random x/y or random navaid


      return false;
    }
    else if (!location_value_tag_name_s.empty())
    {

      return this->get_targetBasedOnTagName(outNewNavInfo, in_plane_type_enum, inProperties, location_value_tag_name_s, location_value_d, location_minDistance_d, location_maxDistance_d);

    } // end if has tag name
  }   // end if lastLegNav have values

  return false;
}

// -----------------------------------

double
RandomEngine::get_slope_at_point(missionx::NavAidInfo& outNavAid)
{
  missionx::RandomEngine::threadState.pipeProperties.setNodeProperty<float>(mxconst::get_ATTRIB_LAT(), outNavAid.lat);
  missionx::RandomEngine::threadState.pipeProperties.setNodeProperty<float>(mxconst::get_ATTRIB_LONG(), outNavAid.lon);
  this->shared_navaid_info.p = outNavAid.p;

  double found_slope_d = 0.0;
  if (missionx::RandomEngine::waitForPluginCallbackJob(missionx::mx_flc_pre_command::calculate_slope_for_build_flight_leg_thread))
    found_slope_d = missionx::RandomEngine::threadState.pipeProperties.getAttribNumericValue<double>(mxconst::get_ATTRIB_TERRAIN_SLOPE(), 0.0); // v3.305.1 updated

  this->errMsg.clear();
  return found_slope_d;
}

// -----------------------------------

bool
RandomEngine::get_is_wet_at_point ( const missionx::NavAidInfo& inNavAid)
{
  this->shared_navaid_info.p = inNavAid.p;
  if (!missionx::RandomEngine::waitForPluginCallbackJob(missionx::mx_flc_pre_command::get_is_point_wet))
  {
    this->setError("[random isWet] Failed to probe for wet. Will treat target coordinates as \"land\". ");
  }

  return this->shared_navaid_info.isWet;
}

// -----------------------------------

void
RandomEngine::update_start_cold_and_dark_with_special_keywords(IXMLNode& inDatarefStartColdAndDarkNode)
{
  // find the first NavAid that briefer is using.
  const std::string firstLeg_s     = Utils::readAttrib(this->xBriefer, mxconst::get_ATTRIB_STARTING_LEG(), "");

  if (this->listNavInfo.size() > static_cast<size_t> ( 1 ) )
  {
    bool bFoundFirstLeg = false;
    auto leg = this->listNavInfo.begin();
    while ( !bFoundFirstLeg )
    {
      bFoundFirstLeg = (leg->flightLegName == firstLeg_s);
      if (!bFoundFirstLeg)
        ++leg;
    }

    if (bFoundFirstLeg)
    {
      if ((*leg).lat != 0.0)
      {
        std::string text = Utils::xml_get_text(inDatarefStartColdAndDarkNode);
        text             = Utils::replaceStringWithOtherString(text, "{navaid_lat}", (*leg).getLat(), true);
        text             = Utils::replaceStringWithOtherString(text, "{navaid_lon}", (*leg).getLon(), true);

        Utils::xml_set_text(inDatarefStartColdAndDarkNode, text);
      }

    }

  }

}

// -----------------------------------

std::string
RandomEngine::prepare_message_with_special_keywords(missionx::NavAidInfo& inNavAid, std::string inMessage)
{
  // std::string resultMessage = std::move(inMessage);

  // v3.0.253.4 calculate bearing
  // calculate bearing
  if (!this->listNavInfo.empty())
  {
    // currently we do not handle skewed location since we provide a bearing to the next target and not exact location
    NavAidInfo& prevNav  = this->listNavInfo.back();
    prevNav.bearing_next = static_cast<float> ( Utils::calcBearingBetween2Points ( prevNav.lat, prevNav.lon, inNavAid.lat, inNavAid.lon ) );


    inNavAid.bearing_to_current_target   = prevNav.bearing_next;
    inNavAid.bearing_back_to_prev_target = (prevNav.bearing_next + 180.0f > 359.0f) ? prevNav.bearing_next - 180.0f : prevNav.bearing_next + 180.0f;
  }


  // 3.0.241.8 Calculate distance between 2 NavAid
  const auto lmbda_get_distance_between_2_nav_points = [](NavAidInfo& inTargetNavAid, const std::list<missionx::NavAidInfo> &listNavInfo)
  {
    NavAidInfo prevNav;
    if (!listNavInfo.empty())
      prevNav = listNavInfo.back();

    if (prevNav.lat != 0.0f && prevNav.lon != 0.0f)
    {
      std::string err;
      return inTargetNavAid.p.calcDistanceBetween2Points(prevNav.p);
    }

    return -1.0; // distance did not calculated due to NavAid targetLat/targetLon values not correct
  };

  const double distance_d = lmbda_get_distance_between_2_nav_points(inNavAid, this->listNavInfo);
#ifndef RELEASE
  Log::logMsg("[prepare message] Distance to prev NavAid: " + Utils::formatNumber<double>(distance_d, 0) + "nm", true);
#endif

  if (!inMessage.empty())
  {
    //// v3.0.221.11 refine Flight Leg message
    mapReplaceMessageKeywords["{navaid_name}"]     = std::string(inNavAid.name);
    mapReplaceMessageKeywords["{navaid_icao}"]     = std::string(inNavAid.ID);
    mapReplaceMessageKeywords["{navaid_lat}"]      = inNavAid.getLat();
    mapReplaceMessageKeywords["{navaid_lon}"]      = inNavAid.getLon();
    mapReplaceMessageKeywords["{bearing_target}"]  = mxUtils::formatNumber<float> ( inNavAid.bearing_to_current_target, 0 );
    const auto elev_ft_s                           = Utils::formatNumber<float> ( inNavAid.height_mt * missionx::meter2feet );
    mapReplaceMessageKeywords["{navaid_elev}"]     = (inNavAid.height_mt == 0.0f) ? "" : elev_ft_s;
    mapReplaceMessageKeywords["{navaid_loc_desc}"] = (inNavAid.loc_desc.empty()) ? inNavAid.get_locDesc() : inNavAid.loc_desc;
    mapReplaceMessageKeywords["{distance}"]        = (distance_d < 0.0) ? "n/a" : (Utils::formatNumber<double>(distance_d, 0) + "nm"); // v3.0.241.8

    for (const auto &[stringToModify, stringToReplaceWith] : this->mapReplaceMessageKeywords) // replace all special keywords
    {
      inMessage = Utils::replaceStringWithOtherString(inMessage, stringToModify, stringToReplaceWith, true);
    }
  }
  else
    Log::logMsgWarn("[random special_keywords] Received empty message. Skipping...", true);

  return inMessage;
}

// -----------------------------------

bool
RandomEngine::prepare_blank_template_with_flight_legs_based_on_ui(IXMLNode& pNode, IXMLNode& outMetaNode, std::string& outErr)
{

  std::string location_value_s;
  outErr.clear();

  // Gather information from UI layer
  const auto med_cargo_or_oilrig_i = Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG(), static_cast<int>(missionx::mx_mission_type::not_defined)); // 0 = med, 1 = cargo
  const auto mission_subcategory_i = Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MISSION_SUBCATEGORY(), static_cast<int>(missionx::mx_mission_subcategory_type::not_defined)); // 0 = med, 1 = cargo
  const auto uiLayer_debug         = data_manager::getGeneratedFromLayer (); // v25.02.1

  const std::string CATEGORY_TRANSLATION = missionx::data_manager::getMissionSubcategoryTranslationCode(med_cargo_or_oilrig_i, mission_subcategory_i); // v3.303.14

  outMetaNode.updateAttribute(CATEGORY_TRANSLATION.c_str(), mxconst::get_ATTRIB_CATEGORY().c_str(), mxconst::get_ATTRIB_CATEGORY().c_str());
  outMetaNode.updateAttribute(mxUtils::formatNumber<int>(med_cargo_or_oilrig_i).c_str(), mxconst::get_PROP_MED_CARGO_OR_OILRIG().c_str(), mxconst::get_PROP_MED_CARGO_OR_OILRIG().c_str());
  outMetaNode.updateAttribute(mxUtils::formatNumber<int>(mission_subcategory_i).c_str(), mxconst::get_PROP_MISSION_SUBCATEGORY().c_str(), mxconst::get_PROP_MISSION_SUBCATEGORY().c_str());


  auto plane_type_i          = Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I(), static_cast<int>(missionx::mx_plane_types::plane_type_props)); // plane type
  auto no_of_legs_i          = Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_NO_OF_LEGS(), 2);                                                 // no of legs
  auto min_distance_slider   = Utils::readNodeNumericAttrib<double>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MIN_DISTANCE_SLIDER(), 5.0);                                   // min slider
  auto max_distance_slider   = Utils::readNodeNumericAttrib<double>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MAX_DISTANCE_SLIDER(), 45.0);                                  // max slider

  // Validations
  assert( (!pNode.isEmpty() && !data_manager::prop_userDefinedMission_ui.node.isEmpty()) && "Empty template or prop_userDefinedMission_ui are empty!"); // debug
  assert(med_cargo_or_oilrig_i > static_cast<int>(missionx::mx_mission_type::not_defined) && ": Main Mission Type can't be undefined. Aborting!!!");     // debug
  assert(CATEGORY_TRANSLATION.empty() == false && ": Sub Category was not found. Aborting!!!");                                             // debug


  if (med_cargo_or_oilrig_i == static_cast<int>(missionx::mx_mission_type::oil_rig)) // v3.303.14 oilrig mission must be helos plane type
  {
    plane_type_i = static_cast<int>(missionx::mx_plane_types::plane_type_helos);
  }

  auto        conv_plane_type_i = static_cast<missionx::_mx_plane_type> ( plane_type_i );
  std::string plane_type_s      = this->translatePlaneTypeToString ( conv_plane_type_i );

  // Store plane type in the XML node
  missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_PLANE_TYPE_S(), plane_type_s);
  pNode.updateAttribute(plane_type_s.c_str(), mxconst::get_ATTRIB_PLANE_TYPE().c_str(), mxconst::get_ATTRIB_PLANE_TYPE().c_str());

  const auto lmbda_get_ramp_type_H_or_S = [](auto in_plane_type_i) {
    if (in_plane_type_i == static_cast<int>(missionx::mx_plane_types::plane_type_prop_floats) || in_plane_type_i == static_cast<int>(missionx::mx_plane_types::plane_type_ga_floats))
      return "|ramp=S"; // S = Seaports

    if (in_plane_type_i == static_cast<int>(missionx::mx_plane_types::plane_type_helos))
      return "|ramp=H"; // H = Helos

    return "";
  };

  const std::string ramp_type_s = lmbda_get_ramp_type_H_or_S(plane_type_i);


  // Anonymous Block
  {
    // create legs according to mission type: medevac, cargo or oilrig
    for (int i1 = 1; i1 <= no_of_legs_i; ++i1)
    {
      // decide on tag name to pick. leg_medevac / leg_cargo.
      std::string tag_name;

      if (i1 == no_of_legs_i) // have we reached the last leg ?
      {
        if (no_of_legs_i == 1) // v3.0.251.1 b2 If user asked for only 1 leg, then do not flag it as end leg.
          tag_name = "leg_" + CATEGORY_TRANSLATION;
        else
          tag_name = "leg_" + CATEGORY_TRANSLATION + "_end";

      }
      else
      {
        tag_name = "leg_" + CATEGORY_TRANSLATION;
      }

      tag_name += (plane_type_i == static_cast<int>(missionx::mx_plane_types::plane_type_helos)) ? "_helos" : "_plane";
      // End result should be: "leg_medevac_helos" or "leg_oilrig_helos" or "leg_cargo_plane" etc...


      IXMLNode node = missionx::data_manager::xmlMappingNode.getChildNode(tag_name.c_str()).deepCopy();
      if (node.isEmpty())
      {
        outErr = "Could not find the mapping node: " + tag_name + ", aborting generating mission template.";
        return false;
      }
      else
      {
        node.updateName(mxconst::get_ELEMENT_LEG().c_str());
        const std::string legName = std::string(mxconst::get_ELEMENT_LEG()) + "_" + Utils::formatNumber<int>(i1);
        Utils::xml_set_attribute_in_node_asString(node, mxconst::get_ATTRIB_NAME(), legName, mxconst::get_ELEMENT_LEG());

        if (i1 < no_of_legs_i || no_of_legs_i == 1)
        {
          std::string location_min_distance_s = Utils::formatNumber<double>(min_distance_slider, 0);
          std::string location_max_distance_s = Utils::formatNumber<double>(max_distance_slider, 0);
          if (med_cargo_or_oilrig_i == static_cast<int>(missionx::mx_mission_type::medevac)) // 0 = medical, 1 = cargo, 2 = oilrig
            location_value_s = std::string("nm=").append( location_max_distance_s ).append( ramp_type_s);
          else if (med_cargo_or_oilrig_i == static_cast<int>(missionx::mx_mission_type::cargo)) // 0 = medical, 1 = cargo, 2 = oilrig
            location_value_s = std::string("nm_between=").append( location_min_distance_s ).append( "-" ).append( location_max_distance_s ).append( ramp_type_s );
          else if (med_cargo_or_oilrig_i == static_cast<int>(missionx::mx_mission_type::oil_rig)) // 0 = medical, 1 = cargo, 2 = oilrig
            location_value_s = std::string("nm_between=5-80").append( ramp_type_s );

          node.getChildNode(mxconst::get_ELEMENT_EXPECTED_LOCATION().c_str()).updateAttribute(location_value_s.c_str(), mxconst::get_ATTRIB_LOCATION_VALUE().c_str(), mxconst::get_ATTRIB_LOCATION_VALUE().c_str());
        }
        pNode.addChild(node.deepCopy()); // add the node to template in memory
      }
    }
  }

  briefer_skeleton_message_to_use_in_injectTypeMissionFeature = "Hello Pilot\n";

  briefer_skeleton_message_to_use_in_injectTypeMissionFeature += (med_cargo_or_oilrig_i == static_cast<int>(missionx::mx_mission_type::medevac)) ? "You have been assigned to a medevac mission. " :
                                                                 (med_cargo_or_oilrig_i == static_cast<int>(missionx::mx_mission_type::cargo))? "You have been assigned to a cargo flight. " :
                                                                 (med_cargo_or_oilrig_i == static_cast<int>(missionx::mx_mission_type::oil_rig))? "You have been assigned to an oilrig flight. " :
                                                                 "You have been assigned to a flight. "
                                                                 ;

  briefer_skeleton_message_to_use_in_injectTypeMissionFeature += "Your expected transportation is a " + plane_type_s + ".\n";

  return true;
}

// -----------------------------------

bool
RandomEngine::prepare_mission_based_on_external_fpln(IXMLNode& pNode)
{
  assert(!pNode.isEmpty() && !data_manager::prop_userDefinedMission_ui.node.isEmpty() && "Empty template or prop_userDefinedMission_ui are empty!");
  auto plane_type_i     = Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I(), static_cast<int>(missionx::mx_plane_types::plane_type_props)); // plane type
  auto fpln_id_picked_i = Utils::readNodeNumericAttrib<int>(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FPLN_ID_PICKED(), -1);                                            // max slider

  if (fpln_id_picked_i < 0 || Utils::isElementExists(missionx::data_manager::indexPointer_forExternalFPLN_tableVector, fpln_id_picked_i) == false)
  {
    this->setError("Could not find the flight plan with index id: " + Utils::formatNumber<int>(fpln_id_picked_i) + ", aborting mission template generating.");
    return false;
  }

  // plane type
  auto conv_plane_type_i = static_cast<missionx::_mx_plane_type>(plane_type_i);
  this->setPlaneType(conv_plane_type_i); // set plane type in class level for other function too

  // fetch the fpln struct to work with
  auto const lmbda_get_fpln = [](const int inPicked_id, const std::vector<missionx::mx_ext_internet_fpln_strct>& inFPLN_vec) // missionx::data_manager::tableExternalFPLN_vec
  {
    missionx::mx_ext_internet_fpln_strct dummy;
    dummy.internal_id = -1;
    for (auto f : inFPLN_vec)
    {
      if (f.internal_id == inPicked_id)
        return f;
    }

    return dummy;
  };

  if ( auto fpln = lmbda_get_fpln ( fpln_id_picked_i, missionx::data_manager::tableExternalFPLN_vec )
      ; fpln.internal_id > -1)
  {
    NavAidInfo prev_na;
    // create navaids based on polyline. First is starting point + briefer and last is target (mandatory)
    int counter = 0;
    for (auto& wp : fpln.listNavPoints)
    {
      NavAidInfo na;
      na.lat = wp.lat;
      na.lon = wp.lon;

      if (counter == 0 || counter == static_cast<int>(fpln.listNavPoints.size() - 1)) // Fetch Navaid data for first and last waypoints
      {
        this->shared_navaid_info.navAid.init();
        this->shared_navaid_info.navAid.lat = na.lat;
        this->shared_navaid_info.navAid.lon = na.lon;
        if (!missionx::RandomEngine::waitForPluginCallbackJob(missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
        {
          this->setError("[random prepare fpln from external fpln] Navaid: " + Utils::formatNumber<int>(counter) + " Failed to find Airport NEAR given location. Still using original Navaid.");
        }
        na.synchToPoint();
        this->shared_navaid_info.navAid.synchToPoint();
        if ( const auto distance = na.p.calcDistanceBetween2Points ( this->shared_navaid_info.navAid.p )
            ; distance <= 2.0) // if navaid within 2nm
        {
          na = this->shared_navaid_info.navAid;
          na.synchToPoint();
        }

        if (counter == 0)
        {
          if (na.getName().empty())
            na.setName(mxconst::get_ELEMENT_BRIEFER());

          if (missionx::RandomEngine::get_user_wants_to_start_from_plane_position()) // v3.0.253.11 extend the starting option to plane position also
          {
            // reset starting point
            na.lat = static_cast<float>(this->planeLocation.getLat());
            na.lon = static_cast<float>(this->planeLocation.getLon());
            na.synchToPoint();
          }
        }

        // try to locate a ramp or starting point
        if (counter > 0 || (counter == 0 && !missionx::RandomEngine::get_user_wants_to_start_from_plane_position()))
        {
          if (std::string err; !filterAndPickRampBasedOnPlaneType(na, err, missionx::mxFilterRampType::start_ramp)) // v3.303.12_r2
          {
            Log::logMsgThread(fmt::format("[{}] {}", __func__, err) );
          }
        }
      }
      else
        na.synchToPoint();


      // v3.0.255.4 add the guessed GPS locations
      if (counter > 0 && counter < static_cast<int>(fpln.listNavPoints.size() - static_cast<size_t>(1)) && fpln.listNavPointsGuessedName.size() >= fpln.listNavPoints.size())
      {
        // loop over list and pick the counter element
        int internalCounter = 0;
        for (const auto &np_guess : fpln.listNavPointsGuessedName)
        {
          if (internalCounter == counter)
          {
            if (np_guess.name.empty())
              break; // exit loop and do nothing
            else
            {
              if (na.getID().empty())
                na.setID(np_guess.name);
              na.navType = (np_guess.nav_type >= 0) ? np_guess.nav_type : na.navType;
              na.navRef  = np_guess.nav_ref;
              na.synchToPoint();
              break; // exit loop, found item
            }
          }

          ++internalCounter;
        } // end loop over all listNavPointsGuessedName
      }   // end inject guessed NavAid names


      this->lastFlightLegNavInfo = na;
      this->listNavInfo.emplace_back(na); // add NavInfo into a list

      // Add to GPS
      if (!xGPS.isEmpty())
        xGPS.addChild(na.node.deepCopy());


      prev_na = na;
      counter++;

    } // end loop over waypoints and gathering NavAid info

    if (this->listNavInfo.size() < 2)
    {
      this->setError("Not enough flight leg information for flight plan id: " + Utils::formatNumber<int>(fpln_id_picked_i) + ", needs at least 2. Aborting mission creation.");
      return false;
    }

    //////////////////////
    // Prepare Main Nodes
    /////////////////////
    std::string plane_type_s = this->translatePlaneTypeToString(conv_plane_type_i); // convert type to string and store it in mission node
    missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty(mxconst::get_PROP_PLANE_TYPE_S(), plane_type_s); //, data_manager::prop_userDefinedMission_ui.node, data_manager::prop_userDefinedMission_ui.node.getName());
    pNode.updateAttribute(plane_type_s.c_str(), mxconst::get_ATTRIB_PLANE_TYPE().c_str(), mxconst::get_ATTRIB_PLANE_TYPE().c_str());

    IXMLNode xLegNode    = missionx::data_manager::xmlMappingNode.getChildNode(mxconst::get_ELEMENT_LEG().c_str()).deepCopy();
    IXMLNode xMapTask    = missionx::data_manager::xmlMappingNode.getChildNode(mxconst::get_ELEMENT_TASK().c_str()).deepCopy();
    IXMLNode xMapTrigger = missionx::data_manager::xmlMappingNode.getChildNode(mxconst::get_ELEMENT_TRIGGER().c_str()).deepCopy();
    IXMLNode xMapMessage = Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE(), true); // holds message element from MAPPING


    // Prepare Briefer
    NavAidInfo naBriefer       = this->listNavInfo.front();
    IXMLNode   xLocationAdjust = this->xRootTemplate.getChildNode(mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION().c_str()).deepCopy();
    if (xLocationAdjust.isEmpty())
    {
      this->setError("[random] No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION() + "> was found. Template malformed, abort template generation !!!");

      return false;
    }

    xLocationAdjust.updateName(mxconst::get_ELEMENT_LOCATION_ADJUST().c_str());
    // remove any clear data
    int               nClear      = xLocationAdjust.nClear();    // remove any CDATA or COMMENTS or any clear() type element
    const std::string from_to_s   = get_short_flight_description_from_to(fpln.fromName_s, fpln.fromICAO_s, fpln.toName_s, fpln.toICAO_s); //"From: " + fpln.fromName_s + "(" + fpln.fromICAO_s + ") to " + fpln.toName_s + "(" + fpln.toICAO_s + ")";
    const std::string brieferDesc =
      from_to_s + "\n\n" + "Hello pilot.\nYou have assigned a flight that was generated from \"flightplandatabase.com\". Learn the route and fly it according to the flight plan or modify it if you so wish.\n\nBlue skys.";
    const std::string notes = (fpln.notes_s.empty()) ? "" : "\n\nnotes:\n" + fpln.notes_s; // add notes if any from flight plan


    for (int i = 0; i < nClear; ++i)
      xLocationAdjust.deleteClear(); // change from remove "i" to remove first

    xLocationAdjust.updateAttribute(naBriefer.getLat().c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
    xLocationAdjust.updateAttribute(naBriefer.getLon().c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());
    xLocationAdjust.updateAttribute(naBriefer.getHeading_s().c_str(), mxconst::get_ATTRIB_HEADING_PSI().c_str(), mxconst::get_ATTRIB_HEADING_PSI().c_str());
    xLocationAdjust.updateAttribute(naBriefer.getRampInfo().c_str(), mxconst::get_ATTRIB_RAMP_INFO().c_str(), mxconst::get_ATTRIB_RAMP_INFO().c_str());

    this->lastFlightLegNavInfo = naBriefer;
    if (naBriefer.getName().empty()) // v3.303.10
      this->lastFlightLegNavInfo.flightLegName = mxconst::get_ELEMENT_BRIEFER();

    this->lastFlightLegNavInfo.synchToPoint();

    this->xBriefer = this->xDummyTopNode.addChild(mxconst::get_ELEMENT_BRIEFER().c_str());
    this->xBriefer.addAttribute(mxconst::get_ATTRIB_STARTING_LEG().c_str(), "leg_1"); // leg1 is default value, but it can be changed when using <content> elements with "leg sets"
    IXMLNode cNode = xBriefer.addChild(xLocationAdjust);
    Utils::xml_add_cdata(this->xBriefer, brieferDesc + notes + "\n\n==== suggested waypoints ====\n" + fpln.formated_nav_points_with_guessed_names_s); // v3.0.241.1 // V3.0.255.2 Added gussed waypoints

    // Add inventory if exists in mapping
    if (data_manager::xmlMappingNode.nChildNode(mxconst::get_ELEMENT_INVENTORY().c_str()) > 0)
    {
      this->addInventory(mxconst::get_ELEMENT_BRIEFER(), naBriefer.p.node, mxInvSource::point); // name of store will start with briefer
    }


    //// Finished Briefer construction ////


    // Prepare Objective + Tasks + Triggers and Leg flight plan
    if (xLegNode.isEmpty())
    {
      this->setError("Could not find the mapping node: LEG, aborting mission template generating.");
      return false;
    }
    else
    {
      const std::string legName       = std::string(mxconst::get_ELEMENT_LEG()) + "_" + Utils::formatNumber<int>(fpln_id_picked_i);
      const std::string objectiveName = legName + "_objective";

      IXMLNode xObjective = this->xObjectives.addChild(mxconst::get_ELEMENT_OBJECTIVE().c_str());
      xObjective.updateAttribute(objectiveName.c_str(), mxconst::get_ATTRIB_NAME().c_str());
      // create tasks based on waypoint list, except the first one (which is the briefer)
      counter = 0;
      for (auto& na : this->listNavInfo)
      {
        counter++;
        if (counter == 1)
          continue; // it is the briefer starting location

        std::string task_name    = "task_" + Utils::formatNumber<int>(counter);
        std::string trigger_name = "trig_" + task_name;

        IXMLNode xTask = xObjective.addChild(mxconst::get_ELEMENT_TASK().c_str());
        xTask.updateAttribute(task_name.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
        xTask.updateAttribute(trigger_name.c_str(), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str(), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str());
        xTask.updateAttribute("3", mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str(), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str());                             // evaluate success for 3 seconds
        xTask.updateAttribute(((counter == static_cast<int> ( this->listNavInfo.size () ) ) ? "yes" : ""), mxconst::get_ATTRIB_MANDATORY().c_str(), mxconst::get_ATTRIB_MANDATORY().c_str()); // evaluate success for 3 seconds


        // add the trigger
        IXMLNode xTrigger = xMapTrigger.deepCopy();
        xTrigger.updateAttribute(trigger_name.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
        xTrigger.updateAttribute("rad", mxconst::get_ATTRIB_TYPE().c_str(), mxconst::get_ATTRIB_TYPE().c_str()); // set type as radius based "rad".
        Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_LAT(), na.getLat(), mxconst::get_ELEMENT_POINT());
        Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_LONG(), na.getLon(), mxconst::get_ELEMENT_POINT());

        if (counter == static_cast<int>(this->listNavInfo.size()))
        {
          Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_LENGTH_MT(), "100", mxconst::get_ELEMENT_RADIUS());
          Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "true", mxconst::get_ELEMENT_CONDITIONS());
        }
        else
        {
          Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_LENGTH_MT(), "4000", mxconst::get_ELEMENT_RADIUS());
          Utils::xml_search_and_set_attribute_in_IXMLNode(xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "", mxconst::get_ELEMENT_CONDITIONS());
        }

        this->xTriggers.addChild(xTrigger);
      } // end loop over all waypoints and creating tasks from them

      ////// Construct Flight Leg //////
      static const std::string STARTING_MESSAGE_NAME = "starting_message";
      const std::string        leg_message_name_s    = STARTING_MESSAGE_NAME + "_" + Utils::formatNumber<int>(counter);

      xLegNode.updateAttribute(legName.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
      xLegNode.updateAttribute(from_to_s.c_str(), mxconst::get_ATTRIB_TITLE().c_str(), mxconst::get_ATTRIB_TITLE().c_str());
      Utils::xml_search_and_set_attribute_in_IXMLNode(xLegNode, mxconst::get_ATTRIB_NAME(), objectiveName, mxconst::get_ELEMENT_LINK_TO_OBJECTIVE());      // link to objective
      Utils::xml_search_and_set_attribute_in_IXMLNode(xLegNode, mxconst::get_ATTRIB_NAME(), leg_message_name_s, mxconst::get_ELEMENT_START_LEG_MESSAGE()); // link to objective

      // Add message to flight leg
      IXMLNode xMessage01 = missionx::data_manager::xmlMappingNode.getChildNode(mxconst::get_ELEMENT_MESSAGE().c_str()).deepCopy();
      if (!xMessage01.isEmpty())
      {
        xMessage01.updateAttribute(leg_message_name_s.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());
        IXMLNode mixText = Utils::xml_get_or_create_node_ptr(xMessage01, mxconst::get_ELEMENT_MIX());
        mixText.updateAttribute("text", mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE().c_str(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE().c_str());
        const std::string text = "Hello pilot\nYou will fly the route " + from_to_s + ". \n\nGood Luck";
        Utils::xml_add_cdata(mixText, text);

        this->xMessages.addChild(xMessage01);
      }

      // Add Ending Marker
      IXMLNode xDisplayEndLocation = xLegNode.addChild(mxconst::get_ELEMENT_DISPLAY_OBJECT().c_str());
      if (!xDisplayEndLocation.isEmpty())
      {
        xDisplayEndLocation.addAttribute(mxconst::get_ATTRIB_INSTANCE_NAME().c_str(), std::string("marker_" + legName).c_str());
        xDisplayEndLocation.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), "marker"); // this is the name of the marker in the "template_blank_4_ui.xml" file
        xDisplayEndLocation.addAttribute(mxconst::get_ATTRIB_TARGET_MARKER_B().c_str(), "true");


        NavAidInfo naLast = this->listNavInfo.back();
        xDisplayEndLocation.addAttribute(mxconst::get_ATTRIB_REPLACE_LAT().c_str(), naLast.getLat().c_str());
        xDisplayEndLocation.addAttribute(mxconst::get_ATTRIB_REPLACE_LONG().c_str(), naLast.getLon().c_str());
        xDisplayEndLocation.addAttribute(mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str(), "50"); // display marker 50ft above ground
      }

      // Add Flight Leg DESCRIPTION
      IXMLNode xDesc = xLegNode.getChildNode(mxconst::get_ELEMENT_DESC().c_str());
      if (xDesc.isEmpty())
        xDesc = xLegNode.addChild(mxconst::get_ELEMENT_DESC().c_str());

      Utils::xml_add_cdata(xDesc, brieferDesc + notes + "\n\n==== suggested waypoints ====\n" + fpln.formated_nav_points_with_guessed_names_s); // v3.0.241.1 // V3.0.255.2 Added gussed waypoints


      // We only have one leg
      this->mission_xml_data.currentLegName = legName;
      Utils::xml_add_node_to_element_IXMLNode(xFlightLegs, xLegNode);
      Utils::addElementToMap(mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, 1);
      Utils::addElementToMap(mapFLightPlanOrder_is, 1, this->mission_xml_data.currentLegName);

    } // end creating the flight leg
  }
  else
  {
    this->setError("Flight plan is invalid. Index id: " + Utils::formatNumber<int>(fpln_id_picked_i) + ", aborting mission template generating.");
  }

  return true;
}



// -----------------------------------



bool
RandomEngine::prepare_mission_based_on_ils_search (IXMLNode &pNode)
{
  assert (!pNode.isEmpty () && !data_manager::prop_userDefinedMission_ui.node.isEmpty () && "Empty template or prop_userDefinedMission_ui are empty!");

  const auto plane_type_i     = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I(), static_cast<int> (missionx::mx_plane_types::plane_type_props)); // plane type
  const auto fpln_id_picked_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FPLN_ID_PICKED(), -1); // max slider
  auto       fromICAO         = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FROM_ICAO(), "");
  auto       toICAO           = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_TO_ICAO(), "");

  if (fpln_id_picked_i < 0 || Utils::isElementExists (missionx::data_manager::indexPointer_for_ILS_rows_tableVector, fpln_id_picked_i) == false)
  {
    this->setError ("Could not find the ILS flight plan with index id: " + Utils::formatNumber<int> (fpln_id_picked_i) + ", aborting mission template generating.");
    return false;
  }
  if (fromICAO.empty ())
  {
    this->setError ("[Random ILS Error] No source ICAO was found, aborting mission template generating.");
    return false;
  }


  // convert to native plane type from "int"
  const auto conv_plane_type_i = static_cast<missionx::_mx_plane_type> (plane_type_i);
  this->setPlaneType (conv_plane_type_i); // set plane type in class level for other function too

  // fetch the fpln struct to work with
  auto const lmbda_get_ils_data = [] (const int inPicked_id, const std::vector<missionx::mx_ils_airport_row_strct> &inRow_vec)
  {
    missionx::mx_ils_airport_row_strct dummy; // initialize dummy.seq = -1
    for (auto f : inRow_vec)
    {
      if (f.seq == inPicked_id)
        return f;
    }

    return dummy;
  };

  auto to_icao = lmbda_get_ils_data (fpln_id_picked_i, missionx::data_manager::table_ILS_rows_vec);
  if (to_icao.seq < 0)
  {
    this->setError ("ILS Flight plan is invalid. Index id: " + Utils::formatNumber<int> (fpln_id_picked_i) + ", aborting mission template generating.");
  }
  else
  {

    this->shared_navaid_info.navAid.init ();
    this->shared_navaid_info.navAid.setID (fromICAO);
    if (!missionx::RandomEngine::waitForPluginCallbackJob (missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
    {
      this->setError ("[random prepare ILS fpln ] Start Navaid: " + fromICAO + " Failed to find Airport using original Navaid. Notify developer.");
      return false;
    }
    this->shared_navaid_info.navAid.synchToPoint ();
    // if we reached here then we should have startICAO NavAid information and the targetICAO
    NavAidInfo start_na = this->shared_navaid_info.navAid;


    // v3.0.253.11 force plane position as starting location
    if (missionx::RandomEngine::get_user_wants_to_start_from_plane_position ()) // v3.0.253.11
    {
      start_na.lat = static_cast<float> (this->planeLocation.getLat ());
      start_na.lon = static_cast<float> (this->planeLocation.getLon ());
    }


    start_na.synchToPoint ();
    if (start_na.getName ().empty ())
      start_na.setName (mxconst::get_ELEMENT_BRIEFER());
    // try to locate a ramp
    std::string err;

    // try to locate a ramp v2 - DEBUG
    if (!missionx::RandomEngine::get_user_wants_to_start_from_plane_position () && !filterAndPickRampBasedOnPlaneType (start_na, err, missionx::mxFilterRampType::start_ramp))
    {
      Log::logMsgThread (fmt::format("[{}] {}", __func__, err) );
    }


    this->listNavInfo.emplace_back (start_na); // add NavInfo into a list

    // handle target location
    this->shared_navaid_info.navAid.init ();
    this->shared_navaid_info.navAid.setID (toICAO);
    if (!missionx::RandomEngine::waitForPluginCallbackJob (missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
    {
      this->setError (fmt::format ("[{}] Target Navaid: {}, Failed to find Airport using original Navaid. Notify developer.", __func__, toICAO) );
      return false;
    }
    this->shared_navaid_info.navAid.synchToPoint ();
    NavAidInfo target_na = this->shared_navaid_info.navAid;
    target_na.synchToPoint ();


    if (!filterAndPickRampBasedOnPlaneType (target_na, err, missionx::mxFilterRampType::end_ramp)) // v3.303.12_r2
    {
      Log::logMsgThread (fmt::format("[{}, Target ILS] {}", __func__, err) );
    }
    this->listNavInfo.emplace_back (target_na); // add NavInfo into a list

    // Add to GPS
    if (!xGPS.isEmpty ())
    {
      xGPS.addChild (start_na.node.deepCopy ());
      xGPS.addChild (target_na.node.deepCopy ());
      #ifndef RELEASE
      Utils::xml_print_node (xGPS, true);
      #endif // !RELEASE
    }

    //////////////////////
    // Prepare Main Nodes
    /////////////////////
    std::string plane_type_s = this->translatePlaneTypeToString (conv_plane_type_i); // convert type to string and store it in mission node
    missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_PLANE_TYPE_S(), plane_type_s);
    pNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE().c_str (), mxconst::get_ATTRIB_PLANE_TYPE().c_str ());

    IXMLNode xLegNode    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_LEG().c_str ()).deepCopy ();
    IXMLNode xMapTask    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TASK().c_str ()).deepCopy ();
    IXMLNode xMapTrigger = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TRIGGER().c_str ()).deepCopy ();
    IXMLNode xMapMessage = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE(), true); // holds message element from MAPPING


    // prepare briefer
    NavAidInfo naBriefer       = this->listNavInfo.front ();
    IXMLNode   xLocationAdjust = this->xRootTemplate.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION().c_str ()).deepCopy ();
    if (xLocationAdjust.isEmpty ())
    {
      this->setError ("[random ILS] No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION() + "> was found. Template melform, abort template generation !!!");

      return false;
    }
    xLocationAdjust.updateName (mxconst::get_ELEMENT_LOCATION_ADJUST().c_str ());
    // remove any clear data
    int               nClear      = xLocationAdjust.nClear (); // remove any CDATA or COMMENTS or any clear() type element
    const std::string from_to_s   = get_short_flight_description_from_to (start_na.getName (), start_na.getID (), target_na.getName (), target_na.getID ()); //"From: " + start_na.getName() + "(" + start_na.getID() + ") to " + target_na.getName() + "(" + target_na.getID() + ")";
    std::string       brieferDesc = from_to_s + "\n\n" + "Hello pilot.\nYou have assigned an ILS flight to " + target_na.getID () + " and runway: " + to_icao.loc_rw_s + ". Learn the route and fly it according to plan or modify it if you so wish.\n\nBlue skys.";
    std::string       notes       = "\n\nDestination Notes:\n==============\nAirport: " + to_icao.toName_s + "(" + to_icao.toICAO_s + ")\tAirport Elev.: " + mxUtils::formatNumber<int> (to_icao.ap_elev_ft_i) + " ft." + "\nEstimate distance: " + mxUtils::formatNumber<double> (to_icao.distnace_d, 0) + "nm. \tRunway to Land: " + to_icao.loc_rw_s +
                        ".\nLocalizer Type: " + to_icao.locType_s + ". \tLocalizer bearing: " + mxUtils::formatNumber<int> (to_icao.loc_bearing_i) + " \tlocalizer frq.: " + mxUtils::getFreqFormated (to_icao.loc_frq_mhz);


    for (int i = 0; i < nClear; ++i)
      xLocationAdjust.deleteClear (); // change from remove "i" to remove first


    xLocationAdjust.updateAttribute (naBriefer.getLat ().c_str (), mxconst::get_ATTRIB_LAT().c_str (), mxconst::get_ATTRIB_LAT().c_str ());
    xLocationAdjust.updateAttribute (naBriefer.getLon ().c_str (), mxconst::get_ATTRIB_LONG().c_str (), mxconst::get_ATTRIB_LONG().c_str ());
    xLocationAdjust.updateAttribute (naBriefer.getHeading_s ().c_str (), mxconst::get_ATTRIB_HEADING_PSI().c_str (), mxconst::get_ATTRIB_HEADING_PSI().c_str ());
    xLocationAdjust.updateAttribute (naBriefer.getRampInfo ().c_str (), mxconst::get_ATTRIB_RAMP_INFO().c_str (), mxconst::get_ATTRIB_RAMP_INFO().c_str ());

    this->lastFlightLegNavInfo = naBriefer;
    if (naBriefer.getNavAidName ().empty ()) // v3.303.10
      this->lastFlightLegNavInfo.flightLegName = mxconst::get_ELEMENT_BRIEFER();

    this->lastFlightLegNavInfo.synchToPoint ();

    this->xBriefer = this->xDummyTopNode.addChild (mxconst::get_ELEMENT_BRIEFER().c_str ());
    this->xBriefer.addAttribute (mxconst::get_ATTRIB_STARTING_LEG().c_str (), "leg_1"); // leg_1 is default value, but it can be changed when using <content> elements with "element sets"
    IXMLNode cNode = xBriefer.addChild (xLocationAdjust);
    Utils::xml_add_cdata (this->xBriefer, brieferDesc + notes); //

    // Add inventory if exists in mapping
    if (data_manager::xmlMappingNode.nChildNode (mxconst::get_ELEMENT_INVENTORY().c_str ()) > 0)
    {
      // this->injectInventory(mxconst::get_ELEMENT_BRIEFER(), naBriefer.p.node, mxInvSource::point); // name of store will start with briefer
      this->addInventory (mxconst::get_ELEMENT_BRIEFER(), naBriefer.node, mxInvSource::point); // name of store will start with briefer
    }

    //// Finished Briefer construction ////

    // Prepare Objective + Tasks + Triggers and Leg flight plan
    if (xLegNode.isEmpty ())
    {
      this->setError ("Could not find the mapping node: LEG, aborting mission template generating.");
      return false;
    }
    else
    {
      const std::string legName       = std::string (mxconst::get_ELEMENT_LEG()) + "_" + Utils::formatNumber<int> (fpln_id_picked_i);
      const std::string objectiveName = legName + "_objective";

      IXMLNode xObjective = this->xObjectives.addChild (mxconst::get_ELEMENT_OBJECTIVE().c_str ());
      xObjective.updateAttribute (objectiveName.c_str (), mxconst::get_ATTRIB_NAME().c_str ());

      // create tasks based on waypoint list, excpe the firt one
      int counter = 0;
      for (auto &na : this->listNavInfo)
      {
        counter++;
        if (counter == 1)
          continue; // it is the briefer starting location

        std::string task_name    = "task_" + Utils::formatNumber<int> (counter);
        std::string trigger_name = "trig_" + task_name;

        IXMLNode xTask = xObjective.addChild (mxconst::get_ELEMENT_TASK().c_str ());
        xTask.updateAttribute (task_name.c_str (), mxconst::get_ATTRIB_NAME().c_str (), mxconst::get_ATTRIB_NAME().c_str ());
        xTask.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str ());
        xTask.updateAttribute ("3", mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str ()); // evaluate success for 3 seconds
        xTask.updateAttribute (((counter == static_cast<int> (this->listNavInfo.size ())) ? "yes" : ""), mxconst::get_ATTRIB_MANDATORY().c_str (), mxconst::get_ATTRIB_MANDATORY().c_str ()); // evaluate success for 3 seconds

        // add the trigger
        IXMLNode xTrigger = xMapTrigger.deepCopy ();
        xTrigger.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_NAME().c_str (), mxconst::get_ATTRIB_NAME().c_str ());
        xTrigger.updateAttribute ("rad", mxconst::get_ATTRIB_TYPE().c_str (), mxconst::get_ATTRIB_TYPE().c_str ()); // set type as radius based "rad".
        Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LAT(), na.getLat (), mxconst::get_ELEMENT_POINT());
        Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LONG(), na.getLon (), mxconst::get_ELEMENT_POINT());

        if (counter == static_cast<int> (this->listNavInfo.size ()))
        {
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT(), "100", mxconst::get_ELEMENT_RADIUS());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "true", mxconst::get_ELEMENT_CONDITIONS());
        }
        else
        {
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT(), "4000", mxconst::get_ELEMENT_RADIUS());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "", mxconst::get_ELEMENT_CONDITIONS());
        }

        this->xTriggers.addChild (xTrigger);

      } // end loop over all listNavInfo

      ////// Construct Flight Leg //////
      static const std::string STARTING_MESSAGE_NAME = "starting_message";
      const std::string        leg_message_name_s    = STARTING_MESSAGE_NAME + "_" + Utils::formatNumber<int> (counter);

      xLegNode.updateAttribute (legName.c_str (), mxconst::get_ATTRIB_NAME().c_str (), mxconst::get_ATTRIB_NAME().c_str ());
      xLegNode.updateAttribute (from_to_s.c_str (), mxconst::get_ATTRIB_TITLE().c_str (), mxconst::get_ATTRIB_TITLE().c_str ());
      Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME(), objectiveName, mxconst::get_ELEMENT_LINK_TO_OBJECTIVE()); // link to objective
      Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME(), leg_message_name_s, mxconst::get_ELEMENT_START_LEG_MESSAGE()); // link to objective


      // Add message to flight leg
      IXMLNode xMessage01 = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_MESSAGE().c_str ()).deepCopy ();
      if (!xMessage01.isEmpty ())
      {
        xMessage01.updateAttribute (leg_message_name_s.c_str (), mxconst::get_ATTRIB_NAME().c_str (), mxconst::get_ATTRIB_NAME().c_str ());
        IXMLNode mixText = Utils::xml_get_or_create_node_ptr (xMessage01, mxconst::get_ELEMENT_MIX());
        mixText.updateAttribute ("text", mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE().c_str (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE().c_str ());
        const std::string text = "Hello pilot\nYou will fly the route " + from_to_s + ". \n\nGood Luck";
        Utils::xml_add_cdata (mixText, text);

        this->xMessages.addChild (xMessage01);
      }


      // Add Ending Marker
      if (IXMLNode xDisplayEndLocation = xLegNode.addChild (mxconst::get_ELEMENT_DISPLAY_OBJECT().c_str ()); !xDisplayEndLocation.isEmpty ())
      {
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_INSTANCE_NAME().c_str (), std::string ("marker_" + legName).c_str ());
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_NAME().c_str (), "marker"); // this is the name of the marker in the "template_blank_4_ui.xml" file
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_TARGET_MARKER_B().c_str (), "true");


        NavAidInfo naLast = this->listNavInfo.back ();
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LAT().c_str (), naLast.getLat ().c_str ());
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LONG().c_str (), naLast.getLon ().c_str ());
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str (), "50"); // display marker 50ft above ground
      }

      // Add Flight Leg DESCRIPTION
      IXMLNode xDesc = xLegNode.getChildNode (mxconst::get_ELEMENT_DESC().c_str ());
      if (xDesc.isEmpty ())
        xDesc = xLegNode.addChild (mxconst::get_ELEMENT_DESC().c_str ());

      Utils::xml_add_cdata (xDesc, brieferDesc + notes);

      // We only have one leg
      this->mission_xml_data.currentLegName = legName;
      Utils::xml_add_node_to_element_IXMLNode (xFlightLegs, xLegNode);
      Utils::addElementToMap (mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, 1);
      Utils::addElementToMap (mapFLightPlanOrder_is, 1, this->mission_xml_data.currentLegName);

    } // end xLegNode is valid or not. end creating the flight leg

  } // end prepare mission


  return true;
}


// -----------------------------------


bool
RandomEngine::prepare_mission_based_on_user_fpln_or_simbrief (IXMLNode &pNode)
{
  assert(!pNode.isEmpty() && !data_manager::prop_userDefinedMission_ui.node.isEmpty() && "Empty template or prop_userDefinedMission_ui are empty!");

  missionx::mx_ext_internet_fpln_strct fpln;

  auto plane_type_i     = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I(), static_cast<int> (missionx::mx_plane_types::plane_type_props)); // plane type
  fpln.fpln_unique_id = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FPLN_ID_PICKED(), -1); // max slider
  fpln.fromICAO_s         = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FROM_ICAO(), "");
  fpln.toICAO_s           = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_TO_ICAO(), "");
  fpln.formated_nav_points_with_guessed_names_s = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_ATTRIB_FORMATED_NAV_POINTS(), "");

  if ((fpln.fpln_unique_id < 0) + (fpln.fromICAO_s.empty()) + (fpln.toICAO_s.empty()))
  {
    this->setError("Flight plan might not have the FROM/TO ICAO data information. Aborting mission generating.");
    return false;
  }

  // convert to native plane type from "int"
  auto conv_plane_type_i = static_cast<missionx::_mx_plane_type>(plane_type_i);
  this->setPlaneType(conv_plane_type_i); // set plane type in class level for other function too

  // if (!missionx::data_manager::tableExternalFPLN_simbrief_vec.empty ())
  {
    // if ( !fpln.fromICAO_s.empty ())
    {
      this->shared_navaid_info.navAid.init ();
      this->shared_navaid_info.navAid.setID (fpln.fromICAO_s);

      if (!missionx::RandomEngine::waitForPluginCallbackJob (missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
      {
        this->setError (fmt::format ("[{}] Start Navaid: {}, Failed to find Airport using original Navaid. Notify developer.", __func__, fpln.fromICAO_s));
        return false;
      }
      this->shared_navaid_info.navAid.synchToPoint ();
      // if we reached here then we should have startICAO NavAid information and the targetICAO
      NavAidInfo start_na = this->shared_navaid_info.navAid;

      // force plane position as starting location, based on user preference
      if (missionx::RandomEngine::get_user_wants_to_start_from_plane_position ()) // TODO: do we need to check "from layer" in function ?
      {
        start_na.lat = static_cast<float> (this->planeLocation.getLat ());
        start_na.lon = static_cast<float> (this->planeLocation.getLon ());
      }
      start_na.synchToPoint ();
      if (start_na.getName ().empty ())
        start_na.setName (mxconst::get_ELEMENT_BRIEFER());

      // try to locate a ramp
      std::string err;
      if (!missionx::RandomEngine::get_user_wants_to_start_from_plane_position () && !filterAndPickRampBasedOnPlaneType (start_na, err, missionx::mxFilterRampType::start_ramp))
      {
        Log::logMsgThread (fmt::format ("[{}] {}", __func__, err));
      }

      // add NavInfo into a list
      this->listNavInfo.emplace_back (start_na);

      //////////////////////////
      // handle target location
      /////////////////////////
      this->shared_navaid_info.navAid.init ();
      this->shared_navaid_info.navAid.setID (fpln.toICAO_s);
      if (!missionx::RandomEngine::waitForPluginCallbackJob (missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
      {
        this->setError (fmt::format ("[{}] Target Navaid: {}, Failed to find Airport using original Navaid. Notify developer.", __func__, fpln.toICAO_s));
        return false;
      }
      this->shared_navaid_info.navAid.synchToPoint ();
      NavAidInfo target_na = this->shared_navaid_info.navAid;
      target_na.synchToPoint ();
      // get ramp location
      if (!filterAndPickRampBasedOnPlaneType (target_na, err, missionx::mxFilterRampType::end_ramp)) // v3.303.12_r2
      {
        Log::logMsgThread (fmt::format ("[{}, Target ILS] {}", __func__, err));
      }
      this->listNavInfo.emplace_back (target_na); // add NavInfo into a list

      // Add to GPS
      if (!xGPS.isEmpty ())
      {
        xGPS.addChild (start_na.node.deepCopy ());
        xGPS.addChild (target_na.node.deepCopy ());
        #ifndef RELEASE
        Utils::xml_print_node (xGPS, true);
        #endif // !RELEASE
      }

      //////////////////////
      // Prepare Main Nodes
      /////////////////////
      std::string plane_type_s = this->translatePlaneTypeToString (conv_plane_type_i); // convert type to string and store it in mission node
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_PLANE_TYPE_S(), plane_type_s);
      pNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE().c_str (), mxconst::get_ATTRIB_PLANE_TYPE().c_str ());

      IXMLNode xLegNode    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_LEG().c_str ()).deepCopy ();
      IXMLNode xMapTask    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TASK().c_str ()).deepCopy ();
      IXMLNode xMapTrigger = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TRIGGER().c_str ()).deepCopy ();
      IXMLNode xMapMessage = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE(), true); // holds message element from MAPPING


      // prepare briefer
      NavAidInfo naBriefer       = this->listNavInfo.front ();
      IXMLNode   xLocationAdjust = this->xRootTemplate.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION().c_str ()).deepCopy ();
      if (xLocationAdjust.isEmpty ())
      {
        this->setError ("No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION() + "> was found. Template malformed, abort template generation !!!");

        return false;
      }
      xLocationAdjust.updateName (mxconst::get_ELEMENT_LOCATION_ADJUST().c_str ());
      // remove any clear data
      int               nClear      = xLocationAdjust.nClear (); // remove any CDATA or COMMENTS or any clear() type element
      const std::string from_to_s   = get_short_flight_description_from_to (start_na.getName (), start_na.getID (), target_na.getName (), target_na.getID ()); //"From: " + start_na.getName() + "(" + start_na.getID() + ") to " + target_na.getName() + "(" + target_na.getID() + ")";
      std::string       brieferDesc = from_to_s + "\n\n" + "Hello pilot.\nYou have assigned a flight to " + target_na.getID () + ". Go over the route and fly it according to plan or modify it if you so wish.\n\nBlue skys.";
      std::string       notes       = "\n\nDestination Notes:\n==============\nAirport: " + target_na.getNavAidName () + "(" + target_na.getID () + ")\nWaypoints:\n" + fpln.formated_nav_points_with_guessed_names_s;


      for (int i = 0; i < nClear; ++i)
        xLocationAdjust.deleteClear ();


      xLocationAdjust.updateAttribute (naBriefer.getLat ().c_str (), mxconst::get_ATTRIB_LAT().c_str (), mxconst::get_ATTRIB_LAT().c_str ());
      xLocationAdjust.updateAttribute (naBriefer.getLon ().c_str (), mxconst::get_ATTRIB_LONG().c_str (), mxconst::get_ATTRIB_LONG().c_str ());
      xLocationAdjust.updateAttribute (naBriefer.getHeading_s ().c_str (), mxconst::get_ATTRIB_HEADING_PSI().c_str (), mxconst::get_ATTRIB_HEADING_PSI().c_str ());
      xLocationAdjust.updateAttribute (naBriefer.getRampInfo ().c_str (), mxconst::get_ATTRIB_RAMP_INFO().c_str (), mxconst::get_ATTRIB_RAMP_INFO().c_str ());

      this->lastFlightLegNavInfo = naBriefer;
      if (naBriefer.getNavAidName ().empty ())
        this->lastFlightLegNavInfo.flightLegName = mxconst::get_ELEMENT_BRIEFER();

      this->lastFlightLegNavInfo.synchToPoint ();

      this->xBriefer = this->xDummyTopNode.addChild (mxconst::get_ELEMENT_BRIEFER().c_str ());
      this->xBriefer.addAttribute (mxconst::get_ATTRIB_STARTING_LEG().c_str (), "leg_1"); // leg_1 is default value, but it can be changed when using <content> elements with "element sets"
      IXMLNode cNode = xBriefer.addChild (xLocationAdjust);
      Utils::xml_add_cdata (this->xBriefer, brieferDesc + notes); //

      // Add inventory if exists in mapping
      if (data_manager::xmlMappingNode.nChildNode (mxconst::get_ELEMENT_INVENTORY().c_str ()) > 0)
      {
        this->addInventory (mxconst::get_ELEMENT_BRIEFER(), naBriefer.node, mxInvSource::point); // name of store will start with briefer
      }

      //// Finished Briefer construction ////

      // Prepare Objective + Tasks + Triggers and Leg flight plan
      if (xLegNode.isEmpty ())
      {
        this->setError ("Could not find the mapping node: LEG, aborting mission template generating.");
        return false;
      }
      else
      {
        const std::string legName       = std::string (mxconst::get_ELEMENT_LEG()) + "_" + Utils::formatNumber<int> (fpln.fpln_unique_id);
        const std::string objectiveName = legName + "_objective";

        IXMLNode xObjective = this->xObjectives.addChild (mxconst::get_ELEMENT_OBJECTIVE().c_str ());
        xObjective.updateAttribute (objectiveName.c_str (), mxconst::get_ATTRIB_NAME().c_str ());

        // create tasks based on waypoint list, except for the first one
        int counter = 0;
        for (auto &na : this->listNavInfo)
        {
          counter++;
          if (counter == 1)
            continue; // it is the briefer starting location

          std::string task_name    = "task_" + Utils::formatNumber<int> (counter);
          std::string trigger_name = "trig_" + task_name;

          IXMLNode xTask = xObjective.addChild (mxconst::get_ELEMENT_TASK().c_str ());
          xTask.updateAttribute (task_name.c_str (), mxconst::get_ATTRIB_NAME().c_str (), mxconst::get_ATTRIB_NAME().c_str ());
          xTask.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER().c_str ());
          xTask.updateAttribute ("3", mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str ()); // evaluate success for 3 seconds
          xTask.updateAttribute (((counter == static_cast<int> (this->listNavInfo.size ())) ? "yes" : ""), mxconst::get_ATTRIB_MANDATORY().c_str (), mxconst::get_ATTRIB_MANDATORY().c_str ()); // evaluate success for 3 seconds

          // add the trigger
          IXMLNode xTrigger = xMapTrigger.deepCopy ();
          xTrigger.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_NAME().c_str (), mxconst::get_ATTRIB_NAME().c_str ());
          xTrigger.updateAttribute ("rad", mxconst::get_ATTRIB_TYPE().c_str (), mxconst::get_ATTRIB_TYPE().c_str ()); // set type as radius based "rad".
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LAT(), na.getLat (), mxconst::get_ELEMENT_POINT());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LONG(), na.getLon (), mxconst::get_ELEMENT_POINT());

          if (counter == static_cast<int> (this->listNavInfo.size ()))
          {
            Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT(), "100", mxconst::get_ELEMENT_RADIUS());
            Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "true", mxconst::get_ELEMENT_CONDITIONS());
          }
          else
          {
            Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT(), "4000", mxconst::get_ELEMENT_RADIUS());
            Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "", mxconst::get_ELEMENT_CONDITIONS());
          }

          this->xTriggers.addChild (xTrigger);

        } // end loop over all listNavInfo

        ////// Construct Flight Leg //////
        static const std::string STARTING_MESSAGE_NAME = "starting_message";
        const std::string        leg_message_name_s    = STARTING_MESSAGE_NAME + "_" + Utils::formatNumber<int> (counter);

        xLegNode.updateAttribute (legName.c_str (), mxconst::get_ATTRIB_NAME().c_str (), mxconst::get_ATTRIB_NAME().c_str ());
        xLegNode.updateAttribute (from_to_s.c_str (), mxconst::get_ATTRIB_TITLE().c_str (), mxconst::get_ATTRIB_TITLE().c_str ());
        Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME(), objectiveName, mxconst::get_ELEMENT_LINK_TO_OBJECTIVE()); // link to objective
        Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME(), leg_message_name_s, mxconst::get_ELEMENT_START_LEG_MESSAGE()); // link to objective


        // Add message to flight leg
        IXMLNode xMessage01 = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_MESSAGE().c_str ()).deepCopy ();
        if (!xMessage01.isEmpty ())
        {
          xMessage01.updateAttribute (leg_message_name_s.c_str (), mxconst::get_ATTRIB_NAME().c_str (), mxconst::get_ATTRIB_NAME().c_str ());
          IXMLNode mixText = Utils::xml_get_or_create_node_ptr (xMessage01, mxconst::get_ELEMENT_MIX());
          mixText.updateAttribute ("text", mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE().c_str (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE().c_str ());
          const std::string text = "Hello pilot\nYou will fly the route " + from_to_s + ". \n\nGood Luck";
          Utils::xml_add_cdata (mixText, text);

          this->xMessages.addChild (xMessage01);
        }


        // Add Ending Marker
        if (IXMLNode xDisplayEndLocation = xLegNode.addChild (mxconst::get_ELEMENT_DISPLAY_OBJECT().c_str ()); !xDisplayEndLocation.isEmpty ())
        {
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_INSTANCE_NAME().c_str (), std::string ("marker_" + legName).c_str ());
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_NAME().c_str (), "marker"); // this is the name of the marker in the "template_blank_4_ui.xml" file
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_TARGET_MARKER_B().c_str (), "true");


          NavAidInfo naLast = this->listNavInfo.back ();
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LAT().c_str (), naLast.getLat ().c_str ());
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LONG().c_str (), naLast.getLon ().c_str ());
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT().c_str (), "50"); // display marker 50ft above ground
        }

        // Add Flight Leg DESCRIPTION
        IXMLNode xDesc = xLegNode.getChildNode (mxconst::get_ELEMENT_DESC().c_str ());
        if (xDesc.isEmpty ())
          xDesc = xLegNode.addChild (mxconst::get_ELEMENT_DESC().c_str ());

        Utils::xml_add_cdata (xDesc, brieferDesc + notes);

        // We only have one leg
        this->mission_xml_data.currentLegName = legName;
        Utils::xml_add_node_to_element_IXMLNode (xFlightLegs, xLegNode);
        Utils::addElementToMap (mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, 1);
        Utils::addElementToMap (mapFLightPlanOrder_is, 1, this->mission_xml_data.currentLegName);

      } // end xLegNode is valid or not. end creating the flight leg

    } // if struct is valid

  } // if simbrief vector has values


  return true;
}



bool
RandomEngine::prepare_mission_based_on_oilrig ( const IXMLNode & inRootTemplate, std::string& outErr)
{
  // 1. Fetch random oilrig from database with the starting location


  const std::string STMT_KEY_Q0 = "find_oilrig";


  std::map<int, std::string> q0_columns = {
    { 0, "distance" }, { 1, "oilrig_icao_id" }, { 2, "oilrig_icao" }, { 3, "oilrig_name" }, { 4, "oilrig_lat" }, { 5, "oilrig_lon" }
   ,{ 6, "start_icao_id" }, { 7, "start_icao" }, { 8, "start_ap_name" }, { 9, "start_lat" }, { 10, "start_lon" }
  };

  // v24025 - There is no reason to override the mapQueries if the "oilrig" query is already in it
  if (!mxUtils::isElementExists(missionx::data_manager::mapQueries, STMT_KEY_Q0))
  {
    missionx::data_manager::mapQueries[STMT_KEY_Q0] = R"(with oilrigs_vu as (
select oilRigVu.icao_id, oilRigVu.icao, oilRigVu.ap_name, oilRigVu.ap_lat, oilRigVu.ap_lon, trunc(oilRigVu.ap_lat) as ap_lat_trunc, trunc(oilRigVu.ap_lon) as ap_lon_trunc
from airports_vu oilRigVu
where oilRigVu.is_oilrig > 0
),
airports_with_helipads_vu as (
select av.icao_id, av.icao, av.ap_name, av.ap_lat, av.ap_lon, trunc(av.ap_lat) as ap_lat_trunc, trunc(av.ap_lon) as ap_lon_trunc
from airports_vu av
where av.is_oilrig = 0
-- and av.ramp_helos + av.helipads > 0
)
select  mx_calc_distance(ov.ap_lat, ov.ap_lon, awh.ap_lat , awh.ap_lon, 3440) as distance
        , ov.icao_id as oilrig_icao_id, ov.icao as oilrig_icao, ov.ap_name as oilrig_name, ov.ap_lat as oilrig_lat, ov.ap_lon as oilrig_lon
        , awh.icao_id as start_icao_id, awh.icao as start_icao, awh.ap_name as start_ap_name, awh.ap_lat as start_lat, awh.ap_lon as start_lon
from oilrigs_vu ov, airports_with_helipads_vu awh
where 1 = 1
and ov.icao_id != awh.icao_id
and ( awh.ap_lat_trunc between ov.ap_lat_trunc - 2 and ov.ap_lat_trunc + 2
      and awh.ap_lon_trunc between ov.ap_lon_trunc - 2 and ov.ap_lon_trunc + 2 )
order by RANDOM() limit 1
)";
  }


  std::string err;

  std::map<std::string, std::string> row_oilrig_and_start_location;

  Utils::read_external_sql_query_file(missionx::data_manager::mapQueries, mxconst::get_SQLITE_OILRIG_SQLS()); // v24025

  // v24025 - moved code to Utils::read_external_sql_query_file()


  ////////////////// Step 1 ///////////////

  // prepare SQL
  RandomEngine::resultTable_gather_random_airports.clear();

  if (data_manager::db_xp_airports.db_is_open_and_ready)
  {
    char* zErrMsg = 0;
    RandomEngine::resultTable_gather_random_airports.clear();
    int rc = sqlite3_exec(data_manager::db_xp_airports.db, missionx::data_manager::mapQueries[STMT_KEY_Q0].c_str(), RandomEngine::callback_gather_random_airports_db, nullptr, &zErrMsg);
    if (rc != SQLITE_OK)
    {
      outErr = "[" + std::string(__func__) + "] SQL error: " + std::string(zErrMsg);
      sqlite3_free(zErrMsg);
      return false;
    }
    else
    {
      Log::logMsgThread("[" + std::string(__func__) + "] Oil Rig information was gathered.");
#ifndef RELEASE
      for (auto& [indx, row] : resultTable_gather_random_airports)
      {
        std::string debugOutput_s = "\tseq: " + mxUtils::formatNumber<int>(indx) + ": ";
        for ( const auto &colName : q0_columns | std::views::values )
          debugOutput_s += "[" + colName + " = " + row[colName] + "]";

        Log::logMsgThread(debugOutput_s);
      }
#endif // !RELEASE

      row_oilrig_and_start_location.clear();
      if (mxUtils::isElementExists(RandomEngine::resultTable_gather_random_airports, 0))
        row_oilrig_and_start_location = RandomEngine::resultTable_gather_random_airports[0];
      else
      {
        outErr = "No Valid information on Oil Rigs and Start location was found.";
        return false;
      }

    }
  }
  else
  {
    outErr = "[" + std::string(__func__) + "] Database: db_xp_airports is not ready. Aborting !!!";
    return false;
  }

  ////////////////// Step 2 ///////////////
  // Fetch Navaid information from X-Plane - Double check what we fetched from the database
  NavAidInfo start_na, target_na;
  start_na.init();
  target_na.init();


  // Get data from X-Plane own Navdata database
  this->shared_navaid_info.navAid.init();
  this->shared_navaid_info.navAid.setID(row_oilrig_and_start_location[q0_columns[7]]); // Airport ICAO
  this->shared_navaid_info.navAid.lat = mxUtils::stringToNumber<float>(row_oilrig_and_start_location[q0_columns[9]], 8); // Airport Lat
  this->shared_navaid_info.navAid.lon = mxUtils::stringToNumber<float>(row_oilrig_and_start_location[q0_columns[10]], 8); // Airport Lon
  if (!missionx::RandomEngine::waitForPluginCallbackJob(missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
  {
    this->setError("[" + std::string( __func__) + "] Start Navaid: " + this->shared_navaid_info.navAid.getID() + " Failed to find Airport using query navaid. Notify developer.");
    return false;
  }
  this->shared_navaid_info.navAid.synchToPoint();
  start_na = this->shared_navaid_info.navAid;

  this->shared_navaid_info.navAid.init();
  this->shared_navaid_info.navAid.setID(row_oilrig_and_start_location[q0_columns[2]]); // Oil Rig ICAO
  this->shared_navaid_info.navAid.lat = mxUtils::stringToNumber<float>(row_oilrig_and_start_location[q0_columns[4]], 8); // Oil Rig Lat
  this->shared_navaid_info.navAid.lon = mxUtils::stringToNumber<float>(row_oilrig_and_start_location[q0_columns[5]], 8); // Oil Rig Lon
  if (!missionx::RandomEngine::waitForPluginCallbackJob(missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
  {
    this->setError("[" + std::string(__func__) + "] Oil Rig Navaid: " + this->shared_navaid_info.navAid.getID() + " Failed to find Oil Rig using query navaid. Notify developer.");
    return false;
  }
  this->shared_navaid_info.navAid.synchToPoint();
  target_na = this->shared_navaid_info.navAid;


  ////////////////// Step 3 ///////////////
  // Reconstruct the template so it could create the mission using the generic code
  start_na.synchToPoint();
  if (start_na.getName().empty())
    start_na.setName(mxconst::get_ELEMENT_BRIEFER());

   RandomEngine::resultTable_gather_random_airports.clear();
  if (!filterAndPickRampBasedOnPlaneType(start_na, err, missionx::mxFilterRampType::start_ramp))
  {
    Log::logMsgThread("[" + std::string(__func__) + "] Oil Rig starting location error: " + err);
  }

  target_na.synchToPoint();
  if (!filterAndPickRampBasedOnPlaneType(target_na, err, missionx::mxFilterRampType::any_ramp_location))
  {
    Log::logMsgThread("[" + std::string(__func__) + "] Oil Rig Ramp search error: " + err);
  }


  // Init Briefer related information
  IXMLNode xBrieferAndStartLocation = inRootTemplate.getChildNode(mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION().c_str());
  if (xBrieferAndStartLocation.isEmpty())
  {
    this->setError("[" + std::string(__func__) + "] No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION() + "> was found. Template malformed, abort template generation !!!");

    return false;
  }

  xBrieferAndStartLocation.updateAttribute(mxconst::get_EXPECTED_LOCATION_TYPE_XY().c_str(), mxconst::get_ATTRIB_LOCATION_TYPE().c_str(), mxconst::get_ATTRIB_LOCATION_TYPE().c_str());
  xBrieferAndStartLocation.updateAttribute(start_na.getLat().c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
  xBrieferAndStartLocation.updateAttribute(start_na.getLon().c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());
  xBrieferAndStartLocation.updateAttribute(start_na.getID().c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str());


  // read all <leg> elements from inRootTemplate and modify with the Oil Rig information
  std::map<std::string, IXMLNode> mapLegs;

  int nChildLegs = inRootTemplate.nChildNode(mxconst::get_ELEMENT_LEG().c_str());
  for (int loop1 = 0; loop1 < nChildLegs; ++loop1)
  {
    if ( IXMLNode pNode = inRootTemplate.getChildNode ( mxconst::get_ELEMENT_LEG().c_str (), loop1 )
        ; !pNode.isEmpty())
    {
      if ( const std::string name = Utils::readAttrib ( pNode, mxconst::get_ATTRIB_NAME(), "" )
          ; name.empty() == false)
        Utils::addElementToMap(mapLegs, name, pNode);
    }
  }
  // We assume that the first leg will be the first element in the MAP
  if (mapLegs.empty())
  {
    outErr = "[" + std::string(__func__) + "] Failed to find <" + mxconst::get_ELEMENT_LEG() + "> in template. Template might be malformed or of wrong version.";
    return false;
  }
  else
  {
    // get first node
    const auto        firstNodeIter = mapLegs.begin ();
    const std::string key           = firstNodeIter->first;

    mapLegs[key].updateAttribute(target_na.getLat().c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
    mapLegs[key].updateAttribute(target_na.getLon().c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());
    mapLegs[key].updateAttribute(target_na.getID().c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str(), mxconst::get_ATTRIB_ICAO_ID().c_str());
    mapLegs[key].updateAttribute(target_na.getName().c_str(), mxconst::get_ATTRIB_AP_NAME().c_str(), mxconst::get_ATTRIB_AP_NAME().c_str());

    if ( IXMLNode xExpectedLegLocation = mapLegs[key].getChildNode ( mxconst::get_ELEMENT_EXPECTED_LOCATION().c_str () )
        ; xExpectedLegLocation.isEmpty())
    {
      outErr = "No <leg> sub element: 'expected_location' was found for leg: " + key + ". Template might be malformed. Aborting... ";
      return false;
    }
    else
    {
      xExpectedLegLocation.updateAttribute(mxconst::get_EXPECTED_LOCATION_TYPE_OILRIG().c_str(), mxconst::get_ATTRIB_LOCATION_TYPE().c_str(), mxconst::get_ATTRIB_LOCATION_TYPE().c_str());
    }

  }


  #ifndef RELEASE
  Utils::xml_print_node(inRootTemplate, true);
  #endif // !RELEASE




  return true; //true; // for now we always return false until this function will be ready
}



// -----------------------------------










void
RandomEngine::calculate_bbox_coordinates(missionx::Point& outN0, missionx::Point& outS180, missionx::Point& outE90, missionx::Point& outW270, const float inRefLat, const float inRefLon, const double inMaxRadius_d)
{

  Utils::calcPointBasedOnDistanceAndBearing_2DPlane(outN0.lat, outN0.lon, inRefLat, inRefLon, 0, inMaxRadius_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane(outS180.lat, outS180.lon, inRefLat, inRefLon, 180, inMaxRadius_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane(outE90.lat, outE90.lon, inRefLat, inRefLon, 90, inMaxRadius_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane(outW270.lat, outW270.lon, inRefLat, inRefLon, 270, inMaxRadius_d);
}

// -----------------------------------

void
RandomEngine::gather_all_osm_db_files_names_and_path(std::list<std::string>& outListOfFiles)
{
  assert(this->working_tempFile_ptr != nullptr && "template pointer is invalid.");

  const static std::string working_folder   = data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", data_manager::errStr) + "/" + mxconst::get_DB_FOLDER_NAME() + "/";
  const static std::string plugin_folder_db = (this->working_tempFile_ptr->missionFolderName.empty()) ? "" : this->working_tempFile_ptr->filePath + "/" + mxconst::get_DB_FOLDER_NAME() + "/";

  missionx::ListDir::getListOfFilesAsFullPath(working_folder.c_str(), outListOfFiles, mxconst::get_DB_FILE_EXTENSION());
  missionx::ListDir::getListOfFilesAsFullPath(plugin_folder_db.c_str(), outListOfFiles, mxconst::get_DB_FILE_EXTENSION(), true);
}


// -----------------------------------

bool
RandomEngine::osm_get_navaid_from_osm(NavAidInfo&                         outNavAid,
                                      std::map<std::string, std::string>& inMapLocationSplitValues,
                                      missionx::mx_base_node&             inProperties, // v3.305.1
                                      const double                        sourceLat_d,
                                      const double                        sourceLon_d,
                                      double                              min_lat,
                                      double                              max_lat,
                                      double                              min_lon,
                                      double                              max_lon,
                                      const double                        maxDistance_d,
                                      const double                        minDistance_d )
{
  bool bResult = false;

  const auto lmbda_fix_order_of_min_max = [&]() {
    double tmp_d = 0.0;
    if (max_lat < min_lat)
    {
      tmp_d   = max_lat;
      max_lat = min_lat;
      min_lat = tmp_d;
    }

    if (max_lon < min_lon)
    {
      tmp_d   = max_lon;
      max_lon = min_lon;
      min_lon = tmp_d;
    }
  };

  lmbda_fix_order_of_min_max();

  const std::string expectedLocationType = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE(), ""); // v3.0.241.10 b2

  // Priority 1: We start with DB search only if it was asked. // v24.12.2 Although searching the DB is a redundant step, it might be useful in edge cases.
  if (Utils::readBoolAttrib(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_OSM_CHECKBOX(), false)
    || (expectedLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM())
    || (expectedLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM())
    )
  {
    bResult = osm_get_navaid_from_osm_database(outNavAid, inMapLocationSplitValues, inProperties, sourceLat_d, sourceLon_d, min_lat, max_lat, min_lon, max_lon, maxDistance_d, minDistance_d);
    if (bResult)
      return bResult;
  }

  // Priority 2 is for overpass data (web information)
  if (!bResult && (Utils::readBoolAttrib(data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_WEB_OSM_CHECKBOX(), false) || (expectedLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM())))
  {
    return osm_get_navaid_from_overpass(outNavAid, inMapLocationSplitValues, inProperties, sourceLat_d, sourceLon_d, min_lat, max_lat, min_lon, max_lon, maxDistance_d, minDistance_d);
  }


  return bResult;
}


// ------------------------------------------

bool
RandomEngine::osm_get_navaid_from_overpass(NavAidInfo&                         outNavAid,
                                           std::map<std::string, std::string>& inMapLocationSplitValues,
                                           missionx::mx_base_node&             inProperties, // v3.305.1
                                           double                              sourceLat_d,
                                           double                              sourceLon_d,
                                           double                              min_lat,
                                           double                              max_lat,
                                           double                              min_lon,
                                           double                              max_lon,
                                           double                              maxDistance_d,
                                           double                              minDistance_d)
{
  // calculate bounding box
  // create a 1nm mesh of points on that box (each box has its own: "topLeft,topRight,bottomLeft,bottomRight" coordinates.
  // Remove the boxes that are too close to the center (less than min distance)
  // random pick one of the rest of the boxes.
  // fetch overpass data
  // check if there is valid info in it, if not then remove the box from the valid list and pick again.
  // ** We should allow no more than 5 failures until we will bail out and continue with the older code that would pick information randomly from x-plane area

  bool              flag_found_navaid_from_osm = false;
  const std::string inLocationType             = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE(), ""); // v3.0.241.10 b2

  const std::string nm_s                 = (Utils::isElementExists(inMapLocationSplitValues, "nm")) ? inMapLocationSplitValues["nm"] : "";                  // represent airport to find in between distances
  const std::string keyID_s              = (Utils::isElementExists(inMapLocationSplitValues, "keyid")) ? inMapLocationSplitValues["keyid"] : "icao";        // represent ID of the port, like ICAO or FAA
  const std::string keyname_s            = (Utils::isElementExists(inMapLocationSplitValues, "keyname")) ? inMapLocationSplitValues["keyname"] : "name";    // the key value that represents the name
  const std::string keydesc_s            = (Utils::isElementExists(inMapLocationSplitValues, "keydesc")) ? inMapLocationSplitValues["keydesc"] : "amenity"; // the key value that represent description, like amanity
  const std::string designer_desc_s      = (Utils::isElementExists(inMapLocationSplitValues, "desc")) ? inMapLocationSplitValues["desc"] : "";              // Free string define designer description. Should be short.
  const std::string designer_descforce_s = (Utils::isElementExists(inMapLocationSplitValues, "descforce")) ? inMapLocationSplitValues["descforce"] : "";    // Free string define designer description. Should be short.
  const std::string designer_bounds      = (Utils::isElementExists(inMapLocationSplitValues, "bounds")) ? inMapLocationSplitValues["bounds"] : ""; // v3.0.253.4 coordinates represents bottomLeft and topRight area to fetch from OVERPASS
  const std::string webosm_optimize      = (Utils::isElementExists(inMapLocationSplitValues, mxconst::get_ATTRIB_WEBOSM_OPTIMIZE())) ? Utils::stringToLower(inMapLocationSplitValues[mxconst::get_ATTRIB_WEBOSM_OPTIMIZE()])
                                                                                                                          : "y"; // v3.0.253.4 "y/n". Default yes. Should we do optimization on overpass area ? divide area to 1x1nm ?
  // v3.0.253.9.1 replaces force_slope with mx_which_type_to_force
  auto                   designer_force_type_attrib                         = static_cast<mx_which_type_to_force> ( Utils::readNodeNumericAttrib<int> ( inProperties.node, mxconst::get_ATTRIB_FORCE_TYPE_OF_TEMPLATE(), 0 ) );
  int                    number_of_times_to_loop_over_force_template_type_i = Utils::readNodeNumericAttrib<int>(inProperties.node, mxconst::get_PROP_NUMBER_OF_LOOPS_TO_FORCE_TYPE_TEMPLATE(), 0); // dependent on flag_force_slope

  // calculate inner bounds = represents minimum distance
  Point E90inner, W270inner, S180inner, N0inner;
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane(N0inner.lat, N0inner.lon, sourceLat_d, sourceLon_d, 0, minDistance_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane(S180inner.lat, S180inner.lon, sourceLat_d, sourceLon_d, 180, minDistance_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane(E90inner.lat, E90inner.lon, sourceLat_d, sourceLon_d, 90, minDistance_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane(W270inner.lat, W270inner.lon, sourceLat_d, sourceLon_d, 270, minDistance_d);

  // define the 4 bounding points
  Point topLeft(max_lat, min_lon);
  Point topRight(max_lat, max_lon);
  Point bottomLeft(min_lat, min_lon);
  Point bottomRight(min_lat, max_lon);

  Point plane_center(sourceLat_d, sourceLon_d);

  // calculate how many boxed are we should create
  double box_length            = 1.0; // One nautical miles (1nm)
  double bounding_distance     = topLeft - topRight;
  int    number_of_inner_boxes = (int)((box_length >= 1.0) ? (bounding_distance / box_length) : (bounding_distance * box_length)); // calculate the number of inner boxes

  std::deque<missionx::strct_box> meshList; // will hold all mesh boxes that are inside the expected zone (min/max distance from center.

  // store all mesh boxes in 2D array
  Point prev_topRight_point;             // stores the first area box so next line will pick its "bottomLeft" as the starting point for calculation
  Point col0_prev_line_bottomLeft_point; // stores the last area box calculated so next box will pick its "bottomLeft" point for calculation

  // Example how we divide the area, each box has topLeft,topRight,bottomLeft and bottomRight.
  // each new line need to pick the topLeft of previous line.
  // each new box on the horizontal axes, needs to pick the previous topRight box.
  // 0  topR
  //+---++---++---++---+
  //|   ||   ||   ||   |
  //+===++===++===++===+
  //+---++---++---++---+
  //|   ||   ||   ||   |
  //+===++===++===++===+
  //+---++---++---++---+
  //|   ||   ||   ||   |
  //+===++===++===++===+
  // bLeft


  if ( const bool b_osm_optimize = ( webosm_optimize == "y" ) ? true : false )
  {
    for (int i1 = 0; i1 < number_of_inner_boxes; ++i1)
    {

      for (int i2 = 0; i2 < number_of_inner_boxes; ++i2)
      {
        missionx::strct_box box;

        if (i1 == 0 && i2 == 0) // first time initialization for first box in line 0 (we start to count from line 0)
        {
          // first time
          box.topLeft = topLeft;
        }
        else if (i1 > 0 && i2 == 0)
        {
          box.topLeft = col0_prev_line_bottomLeft_point;
        }
        else
        {
          box.topLeft = prev_topRight_point;
        }

        box.calcBoxBasedOn_topLeft(box_length); // calculate all 4 points in the box relative to box.topLeft
        prev_topRight_point = box.topRight;     // store first area box "topRight" point for next line calculation
        if (i2 == 0)
          col0_prev_line_bottomLeft_point = box.bottomLeft;

        // calculate if box is inside the search zone
        const double dist_d = box.center - plane_center;
        if (dist_d >= minDistance_d && dist_d <= maxDistance_d)
          meshList.emplace_back(box);


      } // end loop over inner row mesh - X axes (0,0)(0,1)(0,2)...(0,n-1)


    } // end outer loop over row mesh - Y axes (0,0)(1,0)(2,0)...(n-1,0)
  }
  else
  {
    // we will use the full area for the full search - risky but fine for some cases like specific search for hospitals or helipads that are rear even in big areas, they are not like roads or rivers or coast lines
    // We have to take into consideration min/max values since the box area include all distances include those that are excluded (no pun intended).
    missionx::strct_box box;
    box.topLeft     = topLeft;
    box.topRight    = topRight;
    box.bottomLeft  = bottomLeft;
    box.bottomRight = bottomRight;
    box.center      = plane_center;

    meshList.emplace_back(box);
  }

  ///// Pick randomly one of the boxes

  int iTryCounter           = 0;
  int meshSize              = static_cast<int> ( meshList.size () );
  int divider_i             = static_cast<int> ( std::pow ( 10, mxUtils::formatNumber<int> ( meshSize ).length () ) );
  int max_boxes_to_search_i = static_cast<int> ( ( meshSize * 0.1 ) * ( 1.0 - meshSize / ( divider_i ) ) );
#ifndef RELEASE
  Log::logMsgNone("\t === Will search no more than: " + mxUtils::formatNumber<int>((max_boxes_to_search_i == 0) ? 1 : max_boxes_to_search_i) + " [bbox] areas  ===",
                  true); // v3.0.253.7 formated a little differently so max ways will be at least 1
#endif                   // !RELEASE


  const std::string plugin_user_filter = Utils::getNodeText_type_6(
    system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_FILTER(), mxconst::get_DEFAULT_OVERPASS_WAYS_FILTER()); // missionx::system_actions::pluginSetupOptions.getPropertyValue(mxconst::get_OPT_OVERPASS_FILTER(), err);

PICK_RANDOM_OSM_BBOX:
  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  iTryCounter++;
  if (!meshList.empty())
  {
    std::string err;

    if (meshSize > 1 && iTryCounter > max_boxes_to_search_i) // We try to restrict the number of boxes to search in
    {
      Log::logMsgThread("[overpass2] Failed to find an area with valid node/way.");
      return false;
    }

    int               rnd_box_i   = Utils::getRandomIntNumber ( 0, static_cast<int> ( meshList.size () - 1 ) );
    auto              box         = meshList.at ( rnd_box_i );
    const std::string bounds_s_01 = mxUtils::formatNumber<double> ( box.bottomLeft.lat, 8 ) + "," + mxUtils::formatNumber<double> ( box.bottomLeft.lon, 8 ) + "," + mxUtils::formatNumber<double> ( box.topRight.lat, 8 ) + "," + mxUtils::formatNumber<double> ( box.topRight.lon, 8 );

    #ifndef RELEASE
    Log::logDebugBO("[overpass box] boxed area: " + box.print_BL_and_TR(), true);
    Log::logDebugBO("[overpass ] initial bound: " + bounds_s_01, true);
    #endif // !RELEASE

    // v3.0.253.6 wet box optimization in case of force_sloped_terrain is flagged. If center of box is wet, then we will skip it.
    if (designer_force_type_attrib == mx_which_type_to_force::force_hover)
    {
      NavAidInfo nav;
      // check if center of box is wet
      nav.lat = static_cast<float> ( box.center.lat );
      nav.lon = static_cast<float> ( box.center.lon );
      nav.synchToPoint();

      if (this->get_is_wet_at_point(nav))
      {
        #ifndef RELEASE
        Log::logDebugBO("[force slope] Water body found. Skipped boxed area: " + box.print_BL_and_TR(), true);
        #endif // !RELEASE

        Utils::deque_erase_item_at_index<missionx::strct_box>(meshList, rnd_box_i);
        goto PICK_RANDOM_OSM_BBOX;
      }
    }


    //// READ FROM OVERPASS

    // ---------- BEGIN LAMBDA
    const auto lmbda_get_designer_overpass_filter = [&](const std::string& inLocType) {
      std::list<std::string> list_designer_filter;
      std::string            query_filter;

      // this is the second version of this implementation, it will use a free text (not CDATA) to get designer filter
      if (mxUtils::isElementExists(inMapLocationSplitValues, "tag"))
      {
        const std::string root_filter_tag = inMapLocationSplitValues["tag"];
        if (!xRootTemplate.getChildNode(root_filter_tag.c_str()).isEmpty()) // if the "tag" string exists in template pick a random sub element from it
        {
          std::string out_err;
          IXMLNode    parent      = xRootTemplate.getChildNode(root_filter_tag.c_str());
          if ( const IXMLNode filter_node = Utils::xml_get_node_randomly_by_name_IXMLNode(parent, "", out_err); !filter_node.isEmpty())
          {
            query_filter = filter_node.getText();
          }
          else
            query_filter = "";
        }
      }


      return query_filter;
    };
    const std::string designer_filter_s = lmbda_get_designer_overpass_filter(inLocationType);
    const std::string bounds_final      = (designer_bounds.empty()) ? bounds_s_01 : designer_bounds;

    const auto lmbda_validate_osmweb_filter = [&] ( std::string in_filter, std::string& out_filter, std::string& outErr) {
      outErr.clear();

      // if no {{bbox}} was defined then fail the filter
      if (in_filter.find("({{bbox}})") == std::string::npos)
      {
        out_filter = in_filter;
        outErr     = "No ({{bbox}}) found.";
        return false;
      }

      in_filter = missionx::mxUtils::replaceAll(in_filter, "({{bbox}})", "(" + bounds_final + ")"); // replace all {{bbox}}

      // search for ";out" string.
      if (in_filter.find(";out") == std::string::npos)
      {
        in_filter += (in_filter.back() == ';') ? "out;" : ";out;";
      }
      out_filter = in_filter;
      return true;
    };

    // v3.0.253.9
    std::string overpass_filter_s, filter_err_s;
    if (!lmbda_validate_osmweb_filter(((designer_filter_s.empty()) ? plugin_user_filter : designer_filter_s), overpass_filter_s, filter_err_s))
    {
      err = "[overpass] Filter is not valid: " + filter_err_s + "\n" + overpass_filter_s + "\n";
      Log::logMsgThread(err);
      this->setError(err);
      return false;
    }
    // ------------- END

    const auto lmbda_get_overpass_url = [](std::vector<std::string>& inVecUrls, int& last_url_indx_used_i, int preferred_init_url_indx_i = 0, bool inLockURL = false) {
      if (last_url_indx_used_i > ( static_cast<int> ( inVecUrls.size () ) - 1))
        last_url_indx_used_i = 0;

      if (inLockURL) // v.3.0.255.4.2 implement lockURL
        return ( static_cast<int> ( inVecUrls.size () ) >= preferred_init_url_indx_i) ? inVecUrls.at(preferred_init_url_indx_i) : missionx::EMPTY_STRING;

      if (!inVecUrls.empty() && last_url_indx_used_i > mxconst::INT_UNDEFINED)
        return inVecUrls.at(last_url_indx_used_i);

      last_url_indx_used_i = preferred_init_url_indx_i;
      return (inVecUrls.empty()) ? missionx::EMPTY_STRING : inVecUrls.at(last_url_indx_used_i);
    };

    const bool lock_url_b = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_LOCK_OVERPASS_URL_TO_USER_PICK(), false); // v3.0.255.4.2
    auto       stored_overpass_url = (this->vecMissionInfoOverpassUrls.empty())
                                 ? lmbda_get_overpass_url(missionx::data_manager::vecOverpassUrls, missionx::data_manager::overpass_last_url_indx_used_i, missionx::data_manager::overpass_user_picked_combo_i, lock_url_b)
                                 : lmbda_get_overpass_url(this->vecMissionInfoOverpassUrls, ++this->current_url_indx_used_i, 0, false);
    err.clear();

    ++missionx::data_manager::overpass_last_url_indx_used_i; // round-robin

    #ifndef RELEASE
    Log::logMsgThread("[overpass] WAYS URL: " + stored_overpass_url + "?data=" + overpass_filter_s + "\n");
    #endif // !RELEASE

    //"https://overpass-api.de/api/interpreter?data=(" + overpass_filter_s +");out;"; // EXAMPLE

    const std::string url_s = stored_overpass_url + "?data=" + overpass_filter_s; // v3.0.253.9

    Log::logMsgThread("[overpass] WAYS URL: " + url_s + "\n");

    const auto result_s = missionx::data_manager::fetch_overpass_info(url_s, err);
    if (missionx::RandomEngine::threadState.flagAbortThread)
      return false;


    if (!err.empty())
    {
      Log::logMsgThread("[overpass2] Error while fetching from overpass: " + err);
      goto PICK_RANDOM_OSM_BBOX;
    }
    else if (err.empty() && !result_s.empty())
    {
      IXMLDomParser iDom;
      auto          xmlOSM             = iDom.parseString(result_s.c_str()).deepCopy();
      int           count_nodes_pick_i = 0;

      if (xmlOSM.nChildNode() < 3) // osm always have note + meta, so minimum should be 3
      {
        #ifndef RELEASE
        {
          IXMLRenderer xmlWriter;
          Log::logMsgThread("\n ===osm node ==>\n" + std::string(xmlWriter.getString(xmlOSM)) + "\n<=== end osm node ===\n");
          xmlWriter.clear();
        }
        #endif // !RELEASE

        Log::logMsgThread("[overpass] Found no valid sub node elements, will try different way box."); // debug
        Utils::deque_erase_item_at_index<missionx::strct_box>(meshList, rnd_box_i);


        goto PICK_RANDOM_OSM_BBOX;
      }


    PICK_OSM_CHILD_NODE:
      count_nodes_pick_i++;

      IXMLNode nodeOSM_XML = IXMLNode::emptyIXMLNode;
      ;

      IXMLNode xCenterNode; // v3.0.253.9 holds the center node


      // validate there are valid nodes in <osm>
      const int nAllChildNodes = xmlOSM.nChildNode(); // v3.0.253.9 holds all <osm> subnodes. Can be <way> or <node> //xmlOSM.nChildNode(mxconst::get_ELEMENT_WAY_OSM().c_str());
      if (count_nodes_pick_i > MAX_OSM_NODES_TO_SEARCH || nAllChildNodes < 1) // will exist after 10 of <sub nodes> tests or if there are no more valid sub nodes
      {
        #ifndef RELEASE
        {
          IXMLRenderer xmlWriter;
          Log::logMsgThread("\n ===osm node ==>\n" + std::string(xmlWriter.getString(xmlOSM)) + "\n<=== end osm node ===\n");
          xmlWriter.clear();
        }
        #endif // !RELEASE

        Log::logMsgThread("[overpass] Found no valid sub node elements, will try different way box."); // debug
        Utils::deque_erase_item_at_index<missionx::strct_box>(meshList, rnd_box_i);
        goto PICK_RANDOM_OSM_BBOX; // pick another box
      }
      else
      {
        IXMLNode picked_random_target_node_ptr = IXMLNode::emptyIXMLNode;

        std::string node_id_s;
        const int   rnd_nodes_i  = Utils::getRandomIntNumber(0, nAllChildNodes - 1);
        IXMLNode    osmChildNode = xmlOSM.getChildNode(rnd_nodes_i);
        // store some information about the picked element
        std::string tagName = osmChildNode.getName();

        if (tagName != mxconst::get_ELEMENT_NODE_OSM() && tagName != mxconst::get_ELEMENT_WAY_OSM() && tagName != mxconst::get_ELEMENT_REL_OSM())
        {
          Log::logMsgThread("Picked unsupported node: <" + tagName + ">. Will fetch other node.\n");
          osmChildNode.deleteNodeContent(); // remove from XML output
          count_nodes_pick_i--;             // v3.0.253.9.1 we remove this node from counter so only valid nodes will take into consideration
          goto PICK_OSM_CHILD_NODE;
        }


        int       iNd        = (osmChildNode.isEmpty()) ? 0 : osmChildNode.nChildNode(mxconst::get_ELEMENT_ND_OSM().c_str());
        const int iCenterTag = osmChildNode.nChildNode(mxconst::get_ELEMENT_CENTER().c_str()); // should be only 1 or 0

        if (tagName == mxconst::get_ELEMENT_NODE_OSM())
        {
          picked_random_target_node_ptr = osmChildNode;

          #ifndef RELEASE
          Log::logMsgThread("Picked osm node: \n" + std::string(IXMLRenderer().getString(osmChildNode)) + "\n");
          #endif
        }
        else
        {
          bool bFoundCenter = false;

          bFoundCenter     = false;
          const int nd_i   = (iNd == 0) ? 0 : Utils::getRandomIntNumber(0, iNd - 1 + iCenterTag); // v3.0.253.9 we add the center tag to the mix. If result is = nd_i then we will pick Center element.


          if (iNd == 0 && iCenterTag == 0 && tagName == mxconst::get_ELEMENT_WAY_OSM()) // fetch new zone only if <way> tag has no valid sub-elements to use
          {
            Log::logMsgThread("[overpass2] Found no <nd> element, will try different <osm> child in same box."); // debug

            osmChildNode.deleteNodeContent();
            goto PICK_OSM_CHILD_NODE;
          }

          // v3.0.253.9 should we pick <nd> or <center> sub-element
          auto lmbda_get_nd_or_center_tag_node = [&]() {
            // consider picking center - 10% chance before the other logic will run
            if (iCenterTag > 0)
            {

              #ifndef release
              Log::logMsgThread("[overpass] >>> plugin will use <center> element. will use it <<<\n"); // debug
              if (!osmChildNode.isEmpty())
                Log::logMsgThread(std::string(IXMLRenderer().getString(osmChildNode)) + "\n");
              #endif
              iNd = nd_i;

              bFoundCenter = true;
              xCenterNode  = osmChildNode.getChildNode(mxconst::get_ELEMENT_CENTER().c_str()).deepCopy();
              return xCenterNode.deepCopy();
            }


            return osmChildNode.getChildNode(mxconst::get_ELEMENT_ND_OSM().c_str(), nd_i);
          }; // end lmbda_get_nd_or_center_tag_node

          picked_random_target_node_ptr = lmbda_get_nd_or_center_tag_node();

          //////////////////////////////////////////////////////
          // Handle <ref> node. if we are using <ref> then we need to fetch the <node> based on its "id" attrib value. <center> already holds the position.
          if (!bFoundCenter)
          {
            IXMLDomParser iDom2;
            node_id_s = Utils::readAttrib(picked_random_target_node_ptr, mxconst::get_ATTRIB_REF_OSM(), ""); // fetch ref attribute
            if (node_id_s.empty())
            {
              Log::logMsgThread("[overpass2] Found no 'ref' attribute in <nd>, element maybe malformed, will try other <way> in same area box."); // debug
              osmChildNode.deleteNodeContent();
              goto PICK_OSM_CHILD_NODE;
            }


            // Get <node> information using <ref id="xxx" /> value
            const std::string node_url_s    = stored_overpass_url + "?data=node(id:" + node_id_s + ");out;";
            const std::string node_result_s = missionx::data_manager::fetch_overpass_info(node_url_s, err);
            if (missionx::RandomEngine::threadState.flagAbortThread)
              return false;

            #ifndef RELEASE
            Log::logMsgThread("[overpass] NODE URL: " + node_url_s + "\nResult: " + node_result_s + "\n"); // debug
            #endif


            nodeOSM_XML = iDom2.parseString(node_result_s.c_str()).deepCopy();
            if (nodeOSM_XML.isEmpty() || node_result_s.empty())
            {
              Log::logMsgThread("[overpass2] Failed to fetch a <node>. will try a different area box."); // debug
              Utils::deque_erase_item_at_index<missionx::strct_box>(meshList, rnd_box_i);
              goto PICK_RANDOM_OSM_BBOX; // pick another box
            }
            else
            {
              // store the <node> value for later use as position
              picked_random_target_node_ptr = nodeOSM_XML.getChildNode(mxconst::get_ELEMENT_NODE_OSM().c_str()).deepCopy();

              #ifndef RELEASE
              auto wayNode_s = std::string(IXMLRenderer().getString(osmChildNode));
              Log::logMsgThread("[overpass] Found 'ref' attributes in 'nd' elements. Will pick way based on nd ref: " + node_id_s + "\n");
              Log::logMsgThread("[overpass] Picked Way:\n" + wayNode_s + "\n");
              #endif // !RELEASE

            } // end if we read <ref> and not <center> element
          }
          // Fetch the node from OVERPASS
        }



        ///////////////////// Test the target/NavAid Node ////////////////////////////
        // get position node and use it to construct the target
        IXMLNode target_node_pos_ptr = picked_random_target_node_ptr.deepCopy(); //

        if (target_node_pos_ptr.isEmpty())
        {
          Log::logMsgThread("[overpass2] Failed to fetch a <node>. will try a different way in same box."); // debug
          osmChildNode.deleteNodeContent();
          goto PICK_OSM_CHILD_NODE;
        }

        /////// DISTANCE TEST ////////
        // v3.0.253.7 - [fix bug] slope was always 0 because we did not initialize the node coordinates before testing the slope. Result was always 0.
        outNavAid.lat = Utils::readNodeNumericAttrib<float>(target_node_pos_ptr, mxconst::get_ATTRIB_LAT(), 0.0f);
        outNavAid.lon = Utils::readNodeNumericAttrib<float>(target_node_pos_ptr, mxconst::get_ATTRIB_LONG_OSM(), 0.0f);

        //// v3.0.255.5 calculate distance even if BBOX information was optimized.  v3.0.255.4.1 validate distance is legit using nm_s and nm_between if not optimized
        {
          if (lastFlightLegNavInfo.lat != 0 && lastFlightLegNavInfo.lon != 0)
          {
            const double distance_to_target = Utils::calcDistanceBetween2Points_nm(lastFlightLegNavInfo.lat, lastFlightLegNavInfo.lon, outNavAid.lat, outNavAid.lon);
            double       nm_d               = (nm_s.empty()) ? (double)mxconst::INT_UNDEFINED : mxUtils::stringToNumber<double>(nm_s, 2);

            #ifndef RELEASE
            Log::logMsgThread(fmt::format("[overpass2] Test Distance. Target distance: {}, Allowed distances[nm/between] [nm: {}/ between: {} and {}]",distance_to_target, (nm_d > 0.0) ? mxUtils::formatNumber<double>(nm_d,2): "Not Defined", minDistance_d, maxDistance_d ) ); // debug
            #endif

            if (!this->get_isNavAidInValidDistance(distance_to_target, nm_d, minDistance_d, maxDistance_d))
            {
              #ifndef RELEASE
              Log::logMsgThread(fmt::format("[overpass2] target picked is not in the correct distance. Picked target in: {}, nm: {}, or between: {} and {}", distance_to_target, nm_d, minDistance_d, maxDistance_d) ); // debug
              goto PICK_OSM_CHILD_NODE;
              #endif
            }
          }

        } // end validate distance



        /////// SLOPE TEST ////////
        // v3.0.253.6 check slope if needed
        // v3.0.253.9.1
        if (designer_force_type_attrib > RandomEngine::mx_which_type_to_force::no_force_is_needed && number_of_times_to_loop_over_force_template_type_i > 0)
        {
          outNavAid.synchToPoint();
          auto slope_d = get_slope_at_point(outNavAid);

          auto const lmbda_was_expected_slope_correlate_to_force_type = [&]() {
            if (designer_force_type_attrib == RandomEngine::mx_which_type_to_force::force_hover && slope_d < missionx::data_manager::Max_Slope_To_Land_On)
              return false;
            else if (designer_force_type_attrib == RandomEngine::mx_which_type_to_force::force_flat_terrain_to_land && slope_d > missionx::data_manager::Max_Slope_To_Land_On)
              return false;

            return true;
          };

          if (!lmbda_was_expected_slope_correlate_to_force_type())
          {
            if (designer_force_type_attrib == RandomEngine::mx_which_type_to_force::force_hover)
              Log::logMsgThread("[overpass2] Failed slope test for overpass node. Found slope: " + mxUtils::formatNumber<double>(slope_d, 2) + ", in: " + outNavAid.get_latLon_name()); // debug
            else
              Log::logMsgThread("[overpass2] Failed flat terrain probe for overpass node. Found slope: " + mxUtils::formatNumber<double>(slope_d, 2) + ", in: " + outNavAid.get_latLon_name()); // debug

            osmChildNode.deleteNodeContent();

            if (count_nodes_pick_i < number_of_times_to_loop_over_force_template_type_i)
              goto PICK_OSM_CHILD_NODE;
            else
            {
              Log::logMsgThread("[overpass2] Slopped node failed for: " + mxUtils::formatNumber<int>(number_of_times_to_loop_over_force_template_type_i) + " times, Will try other <way> box"); // v3.0.253.6

              Utils::deque_erase_item_at_index<missionx::strct_box>(meshList, rnd_box_i);
              goto PICK_RANDOM_OSM_BBOX; // pick another box
            }
          }

        } // end force loop type


        // read and store way metadata
        {
          // Fetch WAY tag information
          const int tags_i = osmChildNode.nChildNode("tag"); // check
          for (int i1 = 0; i1 < tags_i; ++i1)
          {
            auto              tagNode = osmChildNode.getChildNode("tag", i1);
            //bool              bFound  = false;
            const std::string key   = Utils::readAttrib(tagNode, "k", "");
            const std::string value = Utils::readAttrib(tagNode, "v", "");

            if (key == keyname_s || key == "name" || (key == "name_desc" && outNavAid.getName().empty()) ) // There is duplications but it provides a safety net
            {
              if (!value.empty())
                outNavAid.setName(value);
            }
            else if (key == keyID_s || (key == "faa") || (key == "icao")) // default: "icao", in US I found faa instead of icao. There is duplications but it provides safety net
            {
              outNavAid.setID(value);
            }
            // else if (key == "name_desc" && outNavAid.getName().empty())
            // {
            //   if (!value.empty())
            //     outNavAid.setName(value);
            // }
            else if (key == "loc_name")
            {
              if (outNavAid.getName().empty() && !value.empty())
                outNavAid.setName(value);

              if (outNavAid.loc_desc.empty())
                outNavAid.loc_desc = value;
            }
            else if ((key == "description"))
            {
              if (outNavAid.getName().empty())
                outNavAid.setName(value);

              if (!value.empty())
                outNavAid.loc_desc = value;
            }
            else if (key == keydesc_s || key == "amenity") // There is duplications but it provides safety net
            {
              if (outNavAid.loc_desc.empty())
                outNavAid.loc_desc = value;
            }

          } // end loop over all "<tag k="" v="" />" sub elements

          // add the designer description to the specific picked location only if segment statement is valid.
          if (!designer_descforce_s.empty())
            outNavAid.loc_desc = designer_desc_s;
          else if (!designer_desc_s.empty() && outNavAid.loc_desc.empty())
            outNavAid.loc_desc = designer_desc_s;


          // handle cases ware there is no name and id
          if (outNavAid.getName().empty())                                                             // v3.0.253.6
            Log::logMsgThread("[osm_get_navaid_overpass] FYI: Found <way> without a name key/value."); // outNavAid.setName("overpass");
          else
            Log::logMsgThread("[osm_get_navaid_overpass] FYI: Found <way> with name: " + outNavAid.getName()); // outNavAid.setName("overpass");



          if (missionx::RandomEngine::threadState.flagAbortThread)
            return false;


          //// Fetch nodes around. Search intersections

          // Display all parents related to node N
          //(
          //  node(id:300209203);<;
          //)
          //;out;
          //
          //// Picking all ways/nodes
          //(
          //  way(id:147398677);node(w)->.n1;
          //  way(id:298792994);node(w)->.n2;
          //  node.n1.n2;
          //)
          //;out;
          //
          // https://gis.stackexchange.com/questions/128044/a-way-to-search-street-intersection-on-openstreetmap
          // Retrieve all ways with name near node with id: 4485963840
          //(
          //  node(id:4485963840);<;
          //)->.a;way.a[name];>
          //;out;
          //
          //
          //
          //(
          //  node(id:568296734);<;
          //  node(id:3557252874);<;
          //)->.a;way.a[name][highway][!building];
          // out;



          if (mxUtils::is_number(node_id_s) || (outNavAid.lat != 0.0f && outNavAid.lon != 0.0f))
          {

            auto const lmbda_who_intersect_point = [&]() {
              std::string filter_s;
              if (!node_id_s.empty())
              {
                filter_s += "node(id:" + node_id_s + ");<;";
              }

              if (outNavAid.lat != 0.0f && outNavAid.lon != 0.0f)
              {
                filter_s += "way(around:20," + outNavAid.getLat() + "," + outNavAid.getLon() + ");";
              }

              return filter_s; // missionx::EMPTY_STRING;
            };

            // search ways around node
            const std::string around_url_s = stored_overpass_url + "?data=(" + lmbda_who_intersect_point() + ")->.a;way.a['name']['highway'][!'building'];out;";

            #ifndef RELEASE
            Log::logMsgThread("[overpass] Fetch ways around navaid URL: \n" + around_url_s + "\n");
            #endif // !RELEASE

            const std::string around_result_s = missionx::data_manager::fetch_overpass_info(around_url_s, err);
            if (err.empty() && !around_result_s.empty())
              outNavAid.xml_osm_around = iDom.parseString(around_result_s.c_str()).deepCopy();

            #ifndef RELEASE
            if (!outNavAid.xml_osm_around.isEmpty())
              Log::logMsgThread("Ways around navaid: \n" + std::string(IXMLRenderer().getString(outNavAid.xml_osm_around)) + "\n");
            #endif // !RELEASE

            flag_found_navaid_from_osm     = true;
            outNavAid.flag_nav_from_webosm = true;
          }
        }
      }
    } // end if we fetched information and there are no issues
  }


  return flag_found_navaid_from_osm;
}

// ------------------------------------------
// ------------------------------------------
// ------------------------------------------

bool
RandomEngine::osm_get_navaid_from_osm_database(NavAidInfo&                         outNavAid,
                                               std::map<std::string, std::string>& inMapLocationSplitValues,
                                               //missionx::mxProperties&             inProperties,
                                               missionx::mx_base_node&             inProperties, // v3.305.1
                                               double                              sourceLat_d,
                                               double                              sourceLon_d,
                                               double                              min_lat,
                                               double                              max_lat,
                                               double                              min_lon,
                                               double                              max_lon,
                                               double                              maxDistance_d,
                                               double                              minDistance_d)
{
  bool        flag_found_navaid_from_osm = false;
  std::string file_name;

  const std::string inLocationType = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE(), ""); // v3.0.241.10 b2

  const std::string dbfile               = (Utils::isElementExists(inMapLocationSplitValues, "dbfile")) ? inMapLocationSplitValues["dbfile"] : "";          // represent airport to find in between distances
  const std::string nm_s                 = (Utils::isElementExists(inMapLocationSplitValues, "nm")) ? inMapLocationSplitValues["nm"] : "";                  // represent airport to find in between distances
  const std::string keyID_s              = (Utils::isElementExists(inMapLocationSplitValues, "keyid")) ? inMapLocationSplitValues["keyid"] : "icao";        // represent ID of the port, like ICAO or FAA
  const std::string keyname_s            = (Utils::isElementExists(inMapLocationSplitValues, "keyname")) ? inMapLocationSplitValues["keyname"] : "name";    // the key value that represents the name
  const std::string keydesc_s            = (Utils::isElementExists(inMapLocationSplitValues, "keydesc")) ? inMapLocationSplitValues["keydesc"] : "amenity"; // the key value that represent description, like amanity
  const std::string designer_desc_s      = (Utils::isElementExists(inMapLocationSplitValues, "desc")) ? inMapLocationSplitValues["desc"] : "";              // Free string define designer description. Should be short.
  const std::string designer_descforce_s = (Utils::isElementExists(inMapLocationSplitValues, "descforce")) ? inMapLocationSplitValues["descforce"] : "";    // Free string define designer description. Should be short.


  #ifndef RELEASE
  Log::logMsgThread("[osm_get_navaid] dbfile: " + dbfile + ", nm_s: " + nm_s + ", keyname_s: " + keyname_s + ", keydesc_s: " + keydesc_s + ", designer_desc_s: [" + designer_desc_s + "], designer_descforce_s: [" + designer_descforce_s +"]");
  #endif


  //// validate min/max lat/lon are correct
  const auto lmbda_fix_order_of_min_max = [&]() {
    double tmp_d = 0.0;
    if (max_lat < min_lat)
    {
      tmp_d   = max_lat;
      max_lat = min_lat;
      min_lat = tmp_d;
    }

    if (max_lon < min_lon)
    {
      tmp_d   = max_lon;
      max_lon = min_lon;
      min_lon = tmp_d;
    }
  };

  std::list<std::string> list_of_osm_db_files;
  this->gather_all_osm_db_files_names_and_path(list_of_osm_db_files);

  // fetch all files
  if (!list_of_osm_db_files.empty()) // we filter by the file extension ".db"
  {
    // loop over all files in folder with extension ".db" and try to open database. Try to create table bounds and only then try to read the bound table. If success then check bounds, if failure then skip to next file
    // stop loop once we found the relevant DB.
    for (const auto& file : list_of_osm_db_files)
    {
      missionx::dbase osm_db;

      // const auto db_file = working_folder + file;
      if (file.find(mxconst::get_DB_AIRPORTS_XP()) != std::string::npos) // skip airports database since it is not OSM
        continue;

      if (missionx::data_manager::db_connect(osm_db, file))
      {
        Log::logMsg("[random osm] Connected to: " + file + ". Next, try to create bounds table", true);
        osm_db.execute_stmt(missionx::data_manager::mapQueries["create_bounds"]); // we do not check the output

        if (osm_db.prepareNewStatement("get_tested_bounds", missionx::data_manager::mapQueries["get_tested_bounds"]))
        {
          assert(osm_db.mapStatements["get_tested_bounds"] != nullptr);
          // bind parameters
          osm_db.bind_to_stored_stmt("get_tested_bounds", missionx::db_types::real_typ, 1, Utils::formatNumber<double>(min_lat, 8));
          osm_db.bind_to_stored_stmt("get_tested_bounds", missionx::db_types::real_typ, 2, Utils::formatNumber<double>(max_lat, 8));
          osm_db.bind_to_stored_stmt("get_tested_bounds", missionx::db_types::real_typ, 3, Utils::formatNumber<double>(min_lon, 8));
          osm_db.bind_to_stored_stmt("get_tested_bounds", missionx::db_types::real_typ, 4, Utils::formatNumber<double>(max_lon, 8));

          bool flag_bounds_are_ok = false;
          int  rc                 = osm_db.step(osm_db.mapStatements["get_tested_bounds"]);
          if (rc == SQLITE_ROW)
          {
            double t_min_lat_d     = sqlite3_column_double(osm_db.mapStatements["get_tested_bounds"], 0);
            double t_max_lat_d     = sqlite3_column_double(osm_db.mapStatements["get_tested_bounds"], 1);
            double t_min_lon_d     = sqlite3_column_double(osm_db.mapStatements["get_tested_bounds"], 2);
            double t_max_lon_d     = sqlite3_column_double(osm_db.mapStatements["get_tested_bounds"], 3);
            int    iMin_lat_result = sqlite3_column_int(osm_db.mapStatements["get_tested_bounds"], 4);
            int    iMax_lat_result = sqlite3_column_int(osm_db.mapStatements["get_tested_bounds"], 5);
            int    iMin_lon_result = sqlite3_column_int(osm_db.mapStatements["get_tested_bounds"], 6);
            int    iMax_lon_result = sqlite3_column_int(osm_db.mapStatements["get_tested_bounds"], 7);

            int count_ones = iMin_lat_result + iMax_lat_result + iMin_lon_result + iMax_lon_result;

            if (!dbfile.empty() && file.find_last_of(dbfile) != std::string::npos)
            {                         // check if the path holds the dbfilename in it and force using it or skip
              count_ones         = 4; // force OK state so plugin will use the
              flag_bounds_are_ok = true;

              if (nm_s == "0")
              { // force max region lat/lon
                min_lat = t_min_lat_d;
                max_lat = t_max_lat_d;
                min_lon = t_min_lon_d;
                max_lon = t_max_lon_d;

                // recalculate max distance
                if ( const double maxDistance = Utils::calcDistanceBetween2Points_nm ( min_lat, min_lon, max_lat, max_lon )
                    ; maxDistance > maxDistance_d)
                  maxDistance_d = maxDistance;
              }
            }


            if (count_ones == 4) // count bound points. 4 means we are in the DB file boundaries. Less than 4 means that one of our search points is outside the boundaries and we need to decide if to continue search or not. In current build
                                 // 3 and 4 are OK
            {
              flag_bounds_are_ok = true;
            }
            else if (count_ones > 2) // if we only have 2 good bounds then we still allow, but we will modify the 2 min/max with t_min/t_max numbers
            {
              // find the smallest point and change the max distance accordingly and the min or max value
              if (iMin_lat_result == 0)
              {
                min_lat = t_min_lat_d;
              }
              if (iMax_lat_result == 0)
              {
                max_lat = t_max_lat_d;
              }
              if (iMin_lon_result == 0)
              {
                min_lon = t_min_lon_d;
              }
              if (iMax_lon_result == 0)
              {
                max_lon = t_max_lon_d;

              } // end figure which point needs fixing

              flag_bounds_are_ok = true;

            } // end if we have 3 good points
            else
              flag_bounds_are_ok = false;


            lmbda_fix_order_of_min_max(); // just in case


          } // end if we have row from bounds


          // v3.0.241.10 b2 In this part of the code, we try to build a dynamic query based on the type of the expected location and the filtering values the designer provided.
          // If the designer did not provide filtering values, then we fall back to the original query.


          const auto lmbda_get_designer_query_filter = [&](const std::string& inLocType, const std::string &in_nm_s ) {
            std::list<std::string> list_designer_filter;
            std::string            query_filter;

            if (inLocType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM())
            {

              // this is the second version of this implementation, it will use a free text (not CDATA) to get designer filter
              if (mxUtils::isElementExists(inMapLocationSplitValues, "tag"))
              {
                const std::string root_filter_tag = inMapLocationSplitValues["tag"];
                if (!xRootTemplate.getChildNode(root_filter_tag.c_str()).isEmpty()) // if the "tag" string exists in template pick a random sub element from it
                {
                  std::string err;
                  IXMLNode    parent = xRootTemplate.getChildNode(root_filter_tag.c_str());
                  IXMLNode    node   = Utils::xml_get_node_randomly_by_name_IXMLNode(parent, "", err);
                  if (!node.isEmpty())
                  {
                    query_filter = node.getText();
                    if (in_nm_s == mxconst::get_ZERO()) // v3.0.241.10.2 skip distance query, we want any location on the map
                      query_filter += " ) "; // close inner view
                    else
                      query_filter += missionx::data_manager::mapQueries["get_designer_way_ids_part2"]; //  " ) where distance between ?8 and ?9 ";
                  }
                }
              }
            }

            return query_filter;
          };


          if (flag_bounds_are_ok)
          {
            std::string query_name;

            const std::string filter_s = lmbda_get_designer_query_filter(inLocationType, nm_s);

            if (filter_s.empty()) // fallback query, which is the default query
              query_name = "get_way_ids_in_area";
            else
              query_name = "get_designer_way_ids_part1";

            // v3.0.241.10 b2 add the designer location_values directive to the query:
            std::string query = missionx::data_manager::mapQueries[query_name] + filter_s;


            #ifndef RELEASE
            Log::logMsgThread("[random get osm navaid] query: " + query + "\n");
            Log::logMsgThread("[random get osm navaid] source location lat/lon: " + Utils::formatNumber<double>(sourceLat_d, 8) + "/" + Utils::formatNumber<double>(sourceLon_d, 8) + "\n");
            #endif

            if (osm_db.prepareNewStatement(query_name, query))
            {
              int i = 1;
              // calculate distance base on plane position
              // New code uses sqlite external function "mx_calc_distance" which only needs two values, source_lat and source_lon
              // mx_calc_distance( ?1, ?2, t1.lat, t1.lon, 3440)
              sqlite3_bind_double(osm_db.mapStatements[query_name], i, sourceLat_d);
              i++; // 1 source lat
              sqlite3_bind_double(osm_db.mapStatements[query_name], i, sourceLon_d);
              i++; // 2 source lon


              // area to search
              sqlite3_bind_double(osm_db.mapStatements[query_name], i, min_lat);
              i++; // 3
              sqlite3_bind_double(osm_db.mapStatements[query_name], i, max_lat);
              i++; // 4
              sqlite3_bind_double(osm_db.mapStatements[query_name], i, min_lon);
              i++; // 5
              sqlite3_bind_double(osm_db.mapStatements[query_name], i, max_lon);
              i++; // 6
              // added distance filtering
              sqlite3_bind_double(osm_db.mapStatements[query_name], i, minDistance_d);
              i++; // 7
              sqlite3_bind_double(osm_db.mapStatements[query_name], i, maxDistance_d);
              i++; // 8

              std::map<int, int> map_ids_in_area;
              int                seq = 1;
              while (sqlite3_step(osm_db.mapStatements[query_name]) == SQLITE_ROW)
              {
                map_ids_in_area[seq] = (sqlite3_column_int(osm_db.mapStatements[query_name], 0));
                ++seq;
              }

              if (map_ids_in_area.empty())
              {
                Log::logMsgThread("[osm getNavaid] No points relative to leg position were found in file: " + file + "! location lat/lon: " + Utils::formatNumber<double>(sourceLat_d, 8) + "/" + Utils::formatNumber<double>(sourceLon_d, 8) +
                                  "\n");
                flag_found_navaid_from_osm = false;
              }
              else
              {
                int map_size_i = static_cast<int> ( map_ids_in_area.size () );

                auto iRandom = Utils::getRandomIntNumber(1, map_size_i);

                const auto lmbda_random_bounds_rules = [&] ( const int inSize) {
                  if (iRandom > inSize)
                    return inSize - 1;
                  else if (iRandom < 1 && inSize > 0)
                    return 1;

                  return iRandom;
                };

                iRandom = lmbda_random_bounds_rules( static_cast<int> ( map_ids_in_area.size () ) );

                auto id_to_pick_i = map_ids_in_area[iRandom];

                if (osm_db.prepareNewStatement("get_way_tag_data_by_id", missionx::data_manager::mapQueries["get_way_tag_data_by_id"]))
                {
                  sqlite3_bind_int(osm_db.mapStatements["get_way_tag_data_by_id"], 1, id_to_pick_i);

                  flag_found_navaid_from_osm = false;

                  while (osm_db.step(osm_db.mapStatements["get_way_tag_data_by_id"]) == SQLITE_ROW)
                  {
                    const auto        key   = std::string( reinterpret_cast<const char *> ( sqlite3_column_text ( osm_db.mapStatements["get_way_tag_data_by_id"], 1 ) ) );
                    const auto        value = std::string( reinterpret_cast<const char *> ( sqlite3_column_text ( osm_db.mapStatements["get_way_tag_data_by_id"], 2 ) ) );


                    if (key == keyname_s || key == "name") // There is duplications but it provides safety net
                    {
                      if (!value.empty())
                        outNavAid.setName(value);
                    }
                    else if (key == keyID_s || (key == "faa") || (key == "icao")) // default: "icao", in US I found faa instead of icao. There is duplications but it provides safety net
                    {
                      outNavAid.setID(value);
                    }
                    else if (key == "name_desc" && outNavAid.getName().empty())
                    {
                      if (!value.empty())
                        outNavAid.setName(value);
                    }
                    else if (key == "loc_name")
                    {
                      if (outNavAid.getName().empty() && !value.empty())
                        outNavAid.setName(value);

                      if (outNavAid.loc_desc.empty())
                        outNavAid.loc_desc = value;
                    }
                    else if ((key == "description"))
                    {
                      if (outNavAid.getName().empty())
                        outNavAid.setName(value);

                      if (!value.empty())
                        outNavAid.loc_desc = value;
                    }
                    else if (key == keydesc_s || key == "amenity") // There is duplications but it provides safety net
                    {
                      if (outNavAid.loc_desc.empty())
                        outNavAid.loc_desc = value;
                    }

                    flag_found_navaid_from_osm = true;
                  }

                  if (flag_found_navaid_from_osm && osm_db.prepareNewStatement("get_segments_in_way_id", missionx::data_manager::mapQueries["get_segments_in_way_id"]))
                  {
                    assert(osm_db.mapStatements["get_segments_in_way_id"]);

                    // add the designer description to the specific picked location only if segment statement is valid.
                    if (!designer_descforce_s.empty())
                      outNavAid.loc_desc = designer_desc_s;
                    // else if (!designer_desc_s.empty() && outNavAid.loc_desc.empty())
                    else if (outNavAid.loc_desc.empty())
                      outNavAid.loc_desc = designer_desc_s;

                    flag_found_navaid_from_osm = false; // we prepared the data, but now we need to pick valid osm segment. This means "function" is success only after storing lat/lon in "outNavAid"

                    sqlite3_bind_int(osm_db.mapStatements["get_segments_in_way_id"], 1, id_to_pick_i);
                    std::map<int, Point> mapPointSegments;
                    seq = 1;
                    while (osm_db.step(osm_db.mapStatements["get_segments_in_way_id"]) == SQLITE_ROW)
                    {
                      Point p;
                      p.lat = sqlite3_column_double(osm_db.mapStatements["get_segments_in_way_id"], 0);
                      p.lon = sqlite3_column_double(osm_db.mapStatements["get_segments_in_way_id"], 1);
                      p.storeDataToPointNode();
                      Utils::addElementToMap(mapPointSegments, seq, p);

                      ++seq;
                    } // end loop over all segments


                    // pick one point and initial
                    iRandom = Utils::getRandomIntNumber(1, static_cast<int> ( mapPointSegments.size () ) );
                    iRandom = lmbda_random_bounds_rules( static_cast<int> ( mapPointSegments.size () ) );

                    if (Utils::isElementExists(mapPointSegments, iRandom))
                    {
                      outNavAid.lat              = static_cast<float> ( mapPointSegments[iRandom].lat );
                      outNavAid.lon              = static_cast<float> ( mapPointSegments[iRandom].lon );
                      flag_found_navaid_from_osm = true;

                      #ifndef RELEASE
                      Log::logMsgThread("\n[osm getNavaid] Picked osm location: " + outNavAid.get_latLon_name() + "\n\n");
                      #endif
                    }
                  } // end get_segments_in_way_id


                } // end get_way_tag_data_by_id

              } // end we have values in map_ids_in_area

            } // end if prepared statement succeeded
          }
          else
          {
            Log::logMsgThread("\n[osm getNavaid] osm file: '" + file + "' is not in the boundaries: {minLat/minLon: " + Utils::formatNumber<double>(min_lat, 8) + ", " + Utils::formatNumber<double>(min_lon, 8) +
                              "} and {maxLat/maxLon: " + Utils::formatNumber<double>(max_lat, 8) + ", " + Utils::formatNumber<double>(max_lon, 8) + "}");
          }

          osm_db.clear_and_reset(osm_db.mapStatements["get_tested_bounds"]);

          if (flag_found_navaid_from_osm) // break if found legit segment
            break;

        } // end get_tested_bounds
        else
        {
          Log::logMsgErr(std::string("[random decide osm] ") + sqlite3_errmsg(osm_db.db), true);
        }

      } // if connect to osm sqlite file succeed

      osm_db.close_database(); // close database

    } // end loop over mamp of files

  } // end list all files

  return flag_found_navaid_from_osm;

} // end osm_get_navaid_from_osm

// -----------------------------------

void
RandomEngine::initQueries()
{

  missionx::data_manager::mapQueries["create_bounds"]       = "create table if not exists bounds as select min (t1.lat) as min_lat, max(t1.lat) as max_lat, min(t1.lon) as min_lon, max(t1.lon) as max_lon from way_street_node_data t1";
  missionx::data_manager::mapQueries["get_way_ids_in_area"] = R"(
		select id, distance_nm from ( select distinct t2.id, mx_calc_distance( ?1, ?2, t1.lat, t1.lon, 3440) as distance_nm
		from way_street_node_data t1, way_tag_data t2 where t2.key_attrib = 'highway'
		and t2.val_attrib in('primary', 'secondary', 'tertiary', 'residential', 'service', 'living_street', 'track')
		and t1.id = t2.id and t1.lat between ?3 and ?4
		and t1.lon between ?5 and ?6 )
		where distance_nm between ?7 and ?8
)";


  missionx::data_manager::mapQueries["get_way_tag_data_by_id"] = "select t2.id, t2.key_attrib, t2.val_attrib from way_tag_data t2 where t2.id = ?1 ";
  missionx::data_manager::mapQueries["get_segments_in_way_id"] = "select t1.lat, t1.lon from way_street_node_data t1 where t1.id = ?1 order by t1.node_id";

  missionx::data_manager::mapQueries["get_tested_bounds"] = R"(select min_lat, max_lat, min_lon, max_lon,
			case when min_lat <= ?1 then 1 else 0 end as min_lat_test,
			case when max_lat >= ?2 then 1 else 0 end as max_lat_test,
			case when min_lon <= ?3 then 1 else 0 end as min_lon_test,
			case when max_lon >= ?4 then 1 else 0 end as max_lon_test from bounds limit 1
)";


  // get_designer_way_ids_part1 holds partial SQL. the other part is in get_designer_way_ids_part2
  missionx::data_manager::mapQueries["get_designer_way_ids_part1"] = R"(
	select id, distance_nm from
	(
		select distinct wtd.id, mx_calc_distance( ?1, ?2, wsnd.lat, wsnd.lon, 3440) as distance_nm
		from way_street_node_data wsnd, way_tag_data wtd
		where 1 = 1
		and wsnd.id = wtd.id
		and wsnd.lat between ?3 and ?4
		and wsnd.lon between ?5 and ?6
)";

  missionx::data_manager::mapQueries["get_designer_way_ids_part2"] = " ) where distance_nm between ?7 and ?8 "; // v3.0.255.2 changed bind numbers from ?8,?9 to ?7,?8 since we are using "mx_calc_distance()" function which needs less 1 bind value
}

// -----------------------------------

bool
RandomEngine::check_if_new_target_is_same_as_prev(missionx::NavAidInfo& inCurrentTargetNav, missionx::NavAidInfo& inPrevNav)
{
  return (inCurrentTargetNav.getID() == inPrevNav.getID()) && (inCurrentTargetNav.getName() == inPrevNav.getName());
}

// -----------------------------------


bool
RandomEngine::check_last_2_legs_if_they_have_same_icao()
{
  const auto                      size_i = static_cast<int> ( this->listNavInfo.size () );
  std::list<missionx::NavAidInfo> listNavInfo2(this->listNavInfo.begin(), this->listNavInfo.end());

  if (size_i > 1)
  {
    // get last and 1 before last
    auto last = listNavInfo2.back();
    listNavInfo2.pop_back();
    auto preLast = listNavInfo2.back();

    if (last.getID() == preLast.getID())
    {
      this->listNavInfo.pop_back(); // pop out the last NavInfo since it and the one before it are at the same ICAO
      #ifndef RELEASE
      Log::logMsgThread("Removed duplicate last two ICAO: " + last.getID());
      #endif // !RELEASE
      return false;
    }

  }
  return true;
} // check_last_2_legs



// -----------------------------------

std::string
RandomEngine::get_short_flight_description_from_to ( const std::string &inFromName, const std::string &inFromICAO, const std::string &inToName, const std::string &inToICAO )
{
  if ( inFromName == mxconst::get_ELEMENT_BRIEFER() )
    return  fmt::format (R"(From "{}" to "{} [{}]")", inFromName, inToName, inToICAO);

  return  fmt::format (R"(From "{} [{}]" to "{} [{}]")", inFromName, inFromICAO, inToName, inToICAO);
}

// -----------------------------------

std::vector<IXMLNode>
RandomEngine::calc_land_hover_display_objects ( const double &inLat, const double &inLon, const int &inRadiusMT, const int &inHowManyObjects, int &inout_seq, const std::string &inFileName )
{
  // 1. validate filename is not empty.
  // 2. validate inRadiusMT and inHowManyObjects are valid.
  // 3. calculate new locations relative to the "target" position.
  std::vector<IXMLNode> vec_3d_display_objects;
  const auto display_object_node = Utils::xml_get_node_from_XSD_map_as_acopy ( mxconst::get_ELEMENT_DISPLAY_OBJECT() );
  // basic validation
  if ( inFileName.empty () + ( inHowManyObjects < 4 ) + ( inRadiusMT < 1 ) + display_object_node.isEmpty () )
  {
    Log::logMsgThread ( fmt::format ( "Check values sent to {} function.", __func__ ) );
    return vec_3d_display_objects;
  }

  const auto angle_f = 360.0f / (static_cast<float>(inHowManyObjects));
  const std::string obj_template_name_s = "land_hover_marker";

  for (int i1=0; i1 < inHowManyObjects; ++i1)
  {
    const auto        bearing        = angle_f * ( static_cast<float> ( i1 ) );
    auto              xDisplayObj= display_object_node.deepCopy ();
    const std::string instance_name_s     = fmt::format ( "land_hover_hint_{}", ++inout_seq );

    // calculate and set the <display_object>
    double target_lat, target_lon;
    Point::mxCalcPointBasedOnDistanceAndBearing_2DPlane ( target_lat, target_lon, inLat, inLon, bearing, inRadiusMT );

    // setup the <display_object>
    Utils::xml_set_attribute_in_node_asString ( xDisplayObj, mxconst::get_ATTRIB_NAME(), obj_template_name_s, mxconst::get_ELEMENT_DISPLAY_OBJECT() );
    Utils::xml_set_attribute_in_node_asString ( xDisplayObj, mxconst::get_ATTRIB_INSTANCE_NAME(), instance_name_s, mxconst::get_ELEMENT_DISPLAY_OBJECT() );
    Utils::xml_set_attribute_in_node_asString ( xDisplayObj, mxconst::get_ATTRIB_TARGET_MARKER_B(), "true", mxconst::get_ELEMENT_DISPLAY_OBJECT() );
    Utils::xml_set_attribute_in_node<double> ( xDisplayObj, mxconst::get_ATTRIB_REPLACE_LAT(), target_lat, mxconst::get_ELEMENT_DISPLAY_OBJECT() );
    Utils::xml_set_attribute_in_node<double> ( xDisplayObj, mxconst::get_ATTRIB_REPLACE_LONG(), target_lon, mxconst::get_ELEMENT_DISPLAY_OBJECT() );
    Utils::xml_set_attribute_in_node<int> ( xDisplayObj, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT(), 10, mxconst::get_ELEMENT_DISPLAY_OBJECT() );

    vec_3d_display_objects.push_back ( xDisplayObj );

    #ifndef RELEASE
      Utils::xml_print_node ( xDisplayObj, true );
    #endif
  }

  return vec_3d_display_objects;
}

// -----------------------------------

bool
RandomEngine::get_isNavAidInValidDistance(const double& currentDistanceToTarget, const double& in_location_value_d, const double& in_location_minDistance_d, const double& in_location_maxDistance_d)
{
  if (in_location_value_d > 0.0 && currentDistanceToTarget <= in_location_value_d) // location_value_d represents "nm", It has precedence over min/max
    return true;
  else if (in_location_minDistance_d >= 0.0 && in_location_maxDistance_d > in_location_minDistance_d) // check if between min and max values
    return (currentDistanceToTarget >= in_location_minDistance_d && currentDistanceToTarget <= in_location_maxDistance_d);

  return currentDistanceToTarget > static_cast<double> ( mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT ); // accept distance if in the limit of search airport
}

// -----------------------------------

bool
RandomEngine::get_targetBasedOnTagName(NavAidInfo&             outNewNavInfo,
                                       mx_plane_types          in_plane_type_enum,
                                       const missionx::mx_base_node& inProperties, // v3.305.1
                                       const std::string&      location_value_tag_name_s,
                                       const double            location_value_d,
                                       double                  location_minDistance_d,
                                       double                  location_maxDistance_d)
{
  // v3.0.241.7 // v3.0.241.8 added this->flag_force_template_distances_b to let designer force his "narrative" when it comes to distances.
  const bool        flag_override_random_target_min_dist = (this->flag_force_template_distances_b) ? false : missionx::system_actions::pluginSetupOptions.getBoolValue(mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE());
  const std::string inFlightLegName          = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_NAME(), "");
  const std::string inTemplateType           = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_TYPE(), "");
  const std::string inLocationType           = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE(), "");
  const bool        flag_force_template_type = Utils::readBoolAttrib(inProperties.node, mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B(), false);

  IXMLNode xPoint = IXMLNode::emptyIXMLNode; // local xml <point> element representative.

  IXMLNode rNode = this->xRootTemplate.getChildNode(location_value_tag_name_s.c_str()).deepCopy();
  if (rNode.isEmpty())
  {
    setError("[random get_target rNode] fail to find random pick element. Please fix your template. skipping flight leg: " + inFlightLegName);
    return false;
  }

  this->shared_navaid_info.init();
  this->shared_navaid_info.parentNode_ptr = rNode;                                         // store pointer to XML node
  this->waitForPluginCallbackJob(missionx::mx_flc_pre_command::convert_icao_to_xml_point); // will call missionx::flcPRE() and try to convert any <icao name="icao name" /> to <point targetLat="" targetLon="" />

  // NEAR - do we need to find the nearest location ?
  if (inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_NEAR())
  {
    // Find Nearest NavAid based on given points
    // loop over all points and pick the one that is NEAREST to current point.
    if (lastFlightLegNavInfo.lat != 0 && lastFlightLegNavInfo.lon != 0)
    {
      double last_shortest_dist = mxconst::INT_UNDEFINED;

      const int nChilds = rNode.nChildNode(mxconst::get_ELEMENT_POINT().c_str());
      for (int i1 = 0; i1 < nChilds; ++i1)
      {
        IXMLNode cNode = rNode.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), i1);
        if (cNode.isEmpty())
          continue;

        missionx::NavAidInfo ni;
        ni.lat      = static_cast<float> ( Utils::readNumericAttrib ( cNode, mxconst::get_ATTRIB_LAT(), 0.0 ) );
        ni.lon      = static_cast<float> ( Utils::readNumericAttrib ( cNode, mxconst::get_ATTRIB_LONG(), 0.0 ) );
        ni.loc_desc = Utils::readAttrib(cNode, mxconst::get_ATTRIB_LOC_DESC(), EMPTY_STRING);

        if (ni.lat == 0.0 || ni.lon == 0.0) // skipping if one of the values = 0
          continue;

        const double distance = Utils::calcDistanceBetween2Points_nm(lastFlightLegNavInfo.lat, lastFlightLegNavInfo.lon, ni.lat, ni.lon);

        if (this->get_isNavAidInValidDistance(distance, location_value_d, location_minDistance_d, location_maxDistance_d)) // v3.0.255.4.1 add "nm" and "nm_between" rules
        {
          #ifndef RELEASE
          Log::logMsgThread("[get target based tag] Target: " + ni.loc_desc + " is in a valid distance: " + mxUtils::formatNumber<double>(distance));
          #endif // !RELEASE

          if (last_shortest_dist < 0.0 || distance < last_shortest_dist) // if first time or "new distance" shorter than "last_shortest_dist"
          {
            last_shortest_dist = distance;
            outNewNavInfo      = ni; // v3.0.221.15 rc3.2 // store closest point
          }
        }
        #ifndef RELEASE
        else
        {
          Log::logMsgThread("[get target based tag] Target: " + ni.loc_desc + ", invalid distance: " + mxUtils::formatNumber<double>(distance) + ", Should be nm_between: " + mxUtils::formatNumber<double>(location_minDistance_d) + "-" +
                            mxUtils::formatNumber<double>(location_maxDistance_d) + ((location_value_d) ? ", or nm: " + mxUtils::formatNumber<double>(location_value_d) : "")); // debug
        }
        #endif // !RELEASE
      }
      // v3.0.241.10 b3 missing decision if we found valid: "outNewNavInfo"
      outNewNavInfo.synchToPoint();
      if (outNewNavInfo.lat != 0.0 && outNewNavInfo.lon != 0.0)
        return true;
    }

  } // end handling pick <points> from <tag name>
  else
  { // pick any <point> from element (not "near" location type)

    #ifndef RELEASE
    int nPointChilds = 0;
    nPointChilds     = rNode.nChildNode(mxconst::get_ELEMENT_POINT().c_str());
    Log::logMsgThread("[Pick Point after convert]<point> number: " + Utils::formatNumber<int>(nPointChilds));
    Log::logMsgThread("[Pick Point] force flight leg template: " + std::string((flag_force_template_type) ? "yes" : "no") + "\n"); // v3.0.221.15 rc3
    #endif

    int loop_counter_i = 0; // we will use this to strict the loop to no more than 2
    do
    {
      ++loop_counter_i;
      if ((location_value_d > 0.0 && !flag_override_random_target_min_dist) ||
          (location_value_d > 0.0 && loop_counter_i > 1)) // this fallback will kick in only if user did not modify "expected location" or if we did not find and point in the first run
      {
        location_minDistance_d = 0.0;
        location_maxDistance_d = location_value_d; // if location_value_nm_s has value then use it as max and ignore location_minDistance_d original value.
      }

      bool flag_searchAnotherPoint = false; // v3.0.221.15 rc3
      do
      {
        // reset flag_searchAnotherPoint
        std::string err;
        flag_searchAnotherPoint = false;

        IXMLNode rnd_x_point = Utils::xml_get_node_randomly_by_name_and_distance_IXMLNode(rNode, mxconst::get_ELEMENT_POINT(), this->lastFlightLegNavInfo.lat, this->lastFlightLegNavInfo.lon, err, location_minDistance_d, location_maxDistance_d, true); // remove picked point


        if (!err.empty() || rnd_x_point.isEmpty())
        {
          this->setError("[get_target from tag_name] " + ((err.empty() && rnd_x_point.isEmpty()) ? "No more valid points found in tag: " + location_value_tag_name_s + ", meaning, no points are left." : err));
          return false;
        }

        xPoint = rnd_x_point.deepCopy();

        // check if we need to pick a point that is same as the template type
        if (!flag_force_template_type)
        {
          flag_searchAnotherPoint = false;
          break;
        }

        rnd_x_point.deleteNodeContent();       // decrease the number of points to pick from
        rnd_x_point = IXMLNode::emptyIXMLNode; // v3.0.241.1

        // check if template type is different from xPoint type. We will have to check template point attribute + isWet and slope
        std::string pointTemplate = Utils::stringToLower(Utils::readAttrib(xPoint, mxconst::get_ATTRIB_TEMPLATE(), EMPTY_STRING));

        // we want to test if the picked location template type might be changed due to slope or water body location or if point template is different from given leg_type
        if ((mxconst::get_FL_TEMPLATE_VAL_LAND() == inTemplateType) || (mxconst::get_FL_TEMPLATE_VAL_HOVER() == inTemplateType) )
        {
          NavAidInfo nav;
          nav.node = xPoint.deepCopy();
          nav.syncXmlPointToNav();

          if (pointTemplate.empty() && !flag_force_template_type)
          {
            double slope = 0.0;
            // check template type will change because of water or slope info, this is relevant only for LAND template type
            #ifndef RELEASE
            Log::logMsgNone("[get_target] Test if probing target point will change \"flight leg template type\": " + inTemplateType, true);
            #endif
            // small optimization. Moved the slope + isWet code only when it is relevant
            // slope = this->getSlope(nav); // v3.0.253.7 deprecated getSlope() function - duplicate functions
            slope = this->get_slope_at_point(nav);
            if (slope > missionx::data_manager::Max_Slope_To_Land_On && (mxconst::get_FL_TEMPLATE_VAL_LAND() == inTemplateType))
            {
              Log::logDebugBO("[get_target slope] point has slope: " + Utils::formatNumber<double>(slope, 2) + " is not suitable for template type: " + inTemplateType + "\n", true);
              flag_searchAnotherPoint = true;
            }
            else if (slope <= missionx::data_manager::Max_Slope_To_Land_On && (mxconst::get_FL_TEMPLATE_VAL_HOVER() == inTemplateType))
            {
              Log::logDebugBO("[get_target slope] point has slope: " + Utils::formatNumber<double>(slope, 2) + " is not suitable for template type: " + inTemplateType + " since plane should be able to land.\n", true);

              flag_searchAnotherPoint = true;
            }


            #ifndef RELEASE
            if (flag_searchAnotherPoint)
              Log::logMsg("[get_target slope] point slope should change the target \"<leg>\" type. Will try to pick other <point> from tag.", true);
            else
              Log::logMsg("[get_target slope] point slope will not change target \"<leg>\" type. Will check if target falls in water body.", true);
            #endif


            // check WET only if slope is fine
            if (!flag_searchAnotherPoint)
            {
              bool isWet = false;
              isWet = this->get_is_wet_at_point(nav); // v3.0.253.7
              if (isWet && (mxconst::get_FL_TEMPLATE_VAL_LAND() == inTemplateType))
              {
                Log::logDebugBO("\t[get_target water body] point is in water body and not suitable for flight leg type: " + inTemplateType + "\n", true);

                flag_searchAnotherPoint = true;
              }
              else if (!isWet && slope <= missionx::data_manager::Max_Slope_To_Land_On && (mxconst::get_FL_TEMPLATE_VAL_HOVER() == inTemplateType))
              {
                Log::logDebugBO("\t[get_target water body] point is NOT in water body and therefore we should be able to land which might not be suitable for template type: " + inTemplateType + "\n", true);

                flag_searchAnotherPoint = true;
              }

              #ifndef RELEASE
              if (flag_searchAnotherPoint)
                Log::logMsg("\t[get_target wet] point template type will be changed after testing water body.", true);
              else
                Log::logMsg("\t[get_target wet] point won't change its template type after testing water body.", true);
              #endif
            }
          }
          else
          {
            if (pointTemplate == inTemplateType) // If same leg_type then OK (this will also deal with hover or land cases too)
              flag_searchAnotherPoint = false;
            else
              flag_searchAnotherPoint = true;

            #ifndef RELEASE
            if (flag_searchAnotherPoint)
              Log::logMsg("\t[get_target same template test] point template is different than flight leg template. Will have to pick another point.", true);
            else
              Log::logMsg("\t[get_target same template test] point template is same as flight leg template. Target should be valid.", true);
            #endif
          }

          if (flag_searchAnotherPoint)
            xPoint = IXMLNode::emptyIXMLNode;
        }
        else
          flag_searchAnotherPoint = false; // exit while loop

        #ifndef RELEASE
        nPointChilds = rNode.nChildNode(mxconst::get_ELEMENT_POINT().c_str());
        Log::logMsg("\t[get_target points] After point randomly picked. No. of points left: " + Utils::formatNumber<int>(nPointChilds) + "\n", true);
        #endif
      } while (flag_searchAnotherPoint); // end picking a point from pre-defined location or the nearest one


    } while (xPoint.isEmpty() && flag_override_random_target_min_dist && loop_counter_i < 2); // Max loop will be the second time when we will use developer values

    if (!xPoint.isEmpty())
    {
      #ifndef RELEASE
      IXMLRenderer render;
      Log::logMsgThread(">>>>>>>>> xPoint: " + std::string(render.getString(xPoint)));
      render.clear();
      #endif

      // end handling all template and location_type
      outNewNavInfo.node = xPoint.deepCopy();
      outNewNavInfo.syncXmlPointToNav();
      if (flag_force_template_type)
        outNewNavInfo.flag_force_picked_same_point_template_as_flight_leg_tempalte_type = true;

      return true;
    }
  } // and if near or other template type

  return false;
}


// -----------------------------------


bool
RandomEngine::get_targetForHelos_based_XY_OSM_OSMWEB(NavAidInfo&                         outNewNavInfo,
                                                     mx_plane_types                      in_plane_type_enum,
                                                     std::map<std::string, std::string>& inMapLocationSplitValues,
                                                     missionx::mx_base_node&             inProperties, // v3.305.1
                                                     double                              location_value_d,
                                                     double                              location_minDistance_d,
                                                     const double                        location_maxDistance_d)
{
  // v3.0.219.10
  // pick random location. Use the location_value_nm_s as our radius length in nautical miles.
  // Pick random number between 1 and location_value_nm_s (if location value is less than 1 then we will override it with 10nm).
  // Pick random number between 0 and 355
  // Using Utils:: we will get the new location
  // v3.0.254.3 added support for WEBOSM


  const bool flag_override_random_target_min_dist = (this->flag_force_template_distances_b) ? false: missionx::system_actions::pluginSetupOptions.getBoolValue(mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE() ); // this->flag_force_template_distances_b to let designer force their "narative" when it comes to distances.
  const std::string inLocationType = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE(), "");

  // Prepare distance to target
  // location_value_d has precedence over nm_between, (v3.0.241.8) unless we defined it in the setup flag_override_random_target_min_dist. we will use nm_between if location_value_d is smaller than 1.0nm
  double nm_random_distance_d         = 1.5;
  double nm_max_distance_osm_radius_d = 0.0; // v3.0.241.10 will hold the expected max radius nm value for OSM based legs
  if ((location_value_d <= 1.0 && (location_minDistance_d > 0.0 && location_maxDistance_d > 0.0)) || (flag_override_random_target_min_dist && location_minDistance_d > 0.0 && location_maxDistance_d > 0.0)) // v3.0.241.8 added setup flag hint
  {
    // v3.0.241.8 respecting the location_value_d defined by the designer as the min radius distance even the user preferred a higher value
    // It should balance between what the designer believe is best and what user wants. Destination should be between "designer" and "user"
    if (location_value_d < location_minDistance_d && location_value_d > 1.0)
      location_minDistance_d = location_value_d;

    nm_max_distance_osm_radius_d = location_maxDistance_d;
    nm_random_distance_d         = Utils::getRandomRealNumber(location_minDistance_d, location_maxDistance_d);

    #ifndef RELEASE
    Log::logDebugBO ( "[DEBUG get_target] location: " + inLocationType + ", location_minDistance_d: " + Utils::formatNumber<double> ( location_minDistance_d, 2 ) + ", location_maxDistance_d: " + Utils::formatNumber<double> ( location_maxDistance_d, 2 ), true );
    #endif
  }
  else
  {
    location_value_d             = (location_value_d <= 1.0) ? 10.0 : location_value_d; // we do not need to handle flag_override_random_target_min_dist since it should have been dealt in the above "if" statement
    nm_max_distance_osm_radius_d = location_value_d;
    nm_random_distance_d         = Utils::getRandomRealNumber(1, location_value_d);

    #ifndef RELEASE
    Log::logDebugBO("[DEBUG get_target] location: " + inLocationType + ", location_value_nm_s: " + Utils::formatNumber<double>(location_value_d, 2), true);
    #endif
  }


  if (inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM() || inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM())
  {
    // get max radius and find the 4 points that create the rectangle area
    // nm_max_distance_osm_radius_d = max Radius
    // location_minDistance_d = min radius distance
    #ifndef Release
    const double maxRadius_d   = nm_max_distance_osm_radius_d;
    #endif

    Point E90, W270, S180, N0;
    this->calculate_bbox_coordinates(N0, S180, E90, W270, this->lastFlightLegNavInfo.lat, this->lastFlightLegNavInfo.lon, maxRadius_d);

    if ( NavAidInfo navAid;
        this->osm_get_navaid_from_osm ( navAid, inMapLocationSplitValues, inProperties, this->lastFlightLegNavInfo.lat, this->lastFlightLegNavInfo.lon, S180.lat, N0.lat, W270.lon, E90.lon, nm_max_distance_osm_radius_d, location_minDistance_d ) )
    {
      if (navAid.lat != 0.0 && navAid.lon != 0.0)
      {
        outNewNavInfo = navAid;
        outNewNavInfo.synchToPoint();
        this->flag_picked_from_osm_database = true; // we can use this
        return true;
      }
    }

    // if OSM data was not found then plugin will try to use default target search
  }

  // if we wanted an XY location, or we failed picking location based on OSM data then we will fall back to XY coordinate
  const auto heading_deg = static_cast<float> ( Utils::getRandomIntNumber ( 0, 355 ) );

  double lon;
  double lat = lon = 0.0;

  Utils::calcPointBasedOnDistanceAndBearing_2DPlane(lat, lon, this->lastFlightLegNavInfo.lat, this->lastFlightLegNavInfo.lon, heading_deg, nm_random_distance_d);
  outNewNavInfo.lat      = static_cast<float> ( lat );
  outNewNavInfo.lon      = static_cast<float> ( lon );
  outNewNavInfo.heading  = heading_deg;
  outNewNavInfo.loc_desc = "Coordinates lat: " + outNewNavInfo.getLat() + ", lon: " + outNewNavInfo.getLon();

  #ifndef RELEASE
  Log::logDebugBO("[DEBUG get_target] location: " + inLocationType + ", NavAid.name: " + outNewNavInfo.getNavAidName(), true);
  #endif


  //#ifdef IBM
  outNewNavInfo.setName(mxconst::get_COORDINATES_IN_THE_GPS_S()); // v3.0.221.7 // v3.0.241.9 replaced string with constant since we use it in NavInfo
  outNewNavInfo.flag_picked_random_lat_long = true;
  outNewNavInfo.synchToPoint();

  ////// Information for Main Thread Job Request ///////////
  // v3.0.221.3 calculating slope in main callback and not RandomEngine Thread
  // TODO consider removing these lines since we might set them independently in the create flight leg main function
  missionx::RandomEngine::threadState.pipeProperties.setNumberProperty(mxconst::get_ATTRIB_LAT(), outNewNavInfo.lat);
  missionx::RandomEngine::threadState.pipeProperties.setNumberProperty(mxconst::get_ATTRIB_LONG(), outNewNavInfo.lon);

  #ifndef RELEASE
  Log::logDebugBO("[DEBUG get_target] location: " + inLocationType + ", After slope decision", true);
  #endif

  return true;
}


// -----------------------------------


bool
RandomEngine::get_target_or_lastFlightLeg_based_on_XY_or_OSM(NavAidInfo&                         outNewNavInfo,
                                                             mx_plane_types                      in_plane_type_enum,
                                                             std::map<std::string, std::string>& inMapLocationSplitValues,
                                                             missionx::mx_base_node&             inProperties, // v3.305.1
                                                             const double                        location_value_d,
                                                             const double                        location_minDistance_d,
                                                             const double                        location_maxDistance_d)
{

  const bool flag_override_random_target_min_dist = (this->flag_force_template_distances_b)? false: missionx::system_actions::pluginSetupOptions.getBoolValue(mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE()); // added this->flag_force_template_distances_b to let designer force his "narative" when it comes to distances.
  const std::string inFlightLegName = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_NAME(), "");
  const std::string inLocationType  = Utils::readAttrib(inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE(), "");

  // represent ramp type
  const std::string location_value_restrict_ramp_type_s = mxUtils::getValueFromElement(inMapLocationSplitValues, std::string("ramp"), std::string(""));

  NavAidInfo prevNavInfo;
  if (!this->listNavInfo.empty())
  {
    prevNavInfo = this->listNavInfo.back();
    prevNavInfo.synchToPoint(); // not sure if we need this
  }

  // search NavAid location in radius
  // prepare shared thread data
  this->shared_navaid_info.init();
  this->shared_navaid_info.p = this->lastFlightLegNavInfo.p;
  if (location_value_d > 0.0 && !flag_override_random_target_min_dist)
  {
    this->shared_navaid_info.inMaxDistance_nm = static_cast<float> ( location_value_d );
  }
  else
  {
    this->shared_navaid_info.inMinDistance_nm = static_cast<float> ( location_minDistance_d );
    this->shared_navaid_info.inMaxDistance_nm = (location_maxDistance_d) ? static_cast<float>(location_maxDistance_d) : mxconst::MAX_RAD_4_OSM_MAX_DIST; // v24.12.2 default distance if not set
  }


  // Search for HELOS last flight leg
  // OSM search first - this code will be used when there is a template or mission template with OSM information in it. It will probably won't be called from user creation screen
  if ((inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM() || inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM()) && this->template_plane_type_enum == missionx::mx_plane_types::plane_type_helos)
  {
    Point      E90, W270, S180, N0;

    // get max radius and find the 4 points that create the rectangle area
    // this->shared_navaid_info.inMaxDistance_nm = max Radius
    // location_minDistance_d = min radius distance
    this->calculate_bbox_coordinates(N0, S180, E90, W270, this->lastFlightLegNavInfo.lat, this->lastFlightLegNavInfo.lon, this->shared_navaid_info.inMaxDistance_nm);
    if ( NavAidInfo navAid
        ; this->osm_get_navaid_from_osm(navAid, inMapLocationSplitValues, inProperties, this->lastFlightLegNavInfo.lat, this->lastFlightLegNavInfo.lon
                                        , S180.lat, N0.lat, W270.lon, E90.lon
                                        , this->shared_navaid_info.inMaxDistance_nm, location_minDistance_d))
    {
      if (navAid.lat != 0.0 && navAid.lon != 0.0)
      {
        ////// Test Final NavAid against X-Plane. We will check the closest Navaid to that location and it should be the same. If not we will use, for now the OSM NavAid
        outNewNavInfo = navAid;
        outNewNavInfo.synchToPoint();
        this->flag_picked_from_osm_database = true;                     // we can use this
        const random_airport_info_struct tmp_info = this->shared_navaid_info; // store shared info we prepared in prev step before calling the OSM function. We will use it after calling the main thread for fallback

        this->shared_navaid_info.navAid.init();
        this->shared_navaid_info.navAid.lat = outNewNavInfo.lat;
        this->shared_navaid_info.navAid.lon = outNewNavInfo.lon;

        // test against nearest navaid
        if (!this->waitForPluginCallbackJob(missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
        {
          this->setError("[random get_target_or_lastFlightLeg_based_on_XY_or_OSM] Last Navaid. Failed to find Airport NEAR given location. Still using original Navaid: " + outNewNavInfo.get_locDesc_short());
          // return false;
        }
        outNewNavInfo.synchToPoint();
        this->shared_navaid_info.navAid.synchToPoint();
        const auto distance = outNewNavInfo.p.calcDistanceBetween2Points(this->shared_navaid_info.navAid.p);
        if ((outNewNavInfo.getID() == this->shared_navaid_info.navAid.getID()) || distance <= 1.0)
        {
          outNewNavInfo = this->shared_navaid_info.navAid;
          outNewNavInfo.synchToPoint();
        }

        this->shared_navaid_info = tmp_info;

        return true;
      }
    }

    // if OSM data was not found then plugin will try to use default target search
  }


  /////////////////////////////////////////////
  // Search any last Flight Leg location for any plane
  /////////////////////////////////////////////
  this->shared_navaid_info.inRestrictRampType = location_value_restrict_ramp_type_s;
  this->shared_navaid_info.inExcludeAngle     = static_cast<int> ( prevNavInfo.degRelativeToSearchPoint );
  // v3.0.255.3
  if (this->shared_navaid_info.inMinDistance_nm < this->shared_navaid_info.inStartFromDistance_nm)
    this->shared_navaid_info.inMinDistance_nm = this->shared_navaid_info.inStartFromDistance_nm;

#ifdef IBM
  outNewNavInfo = this->get_random_airport_from_db(this->shared_navaid_info.p, this->shared_navaid_info.inMinDistance_nm, this->shared_navaid_info.inMaxDistance_nm, this->shared_navaid_info.inExcludeAngle); // v3.0.255.3 test integration

#else

  NavAidInfo nav = this->get_random_airport_from_db(this->shared_navaid_info.p, this->shared_navaid_info.inMinDistance_nm, this->shared_navaid_info.inMaxDistance_nm, this->shared_navaid_info.inExcludeAngle); // v3.0.255.3 test integration
  outNewNavInfo  = nav;
#endif

  if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f)
  {
    #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
    if (!this->waitForPluginCallbackJob(missionx::mx_flc_pre_command::gather_random_airport_mainThread, std::chrono::milliseconds(1000))) // pick random airport. Wait up to 10sec
    {
      this->setError("[random get_target_or_lastFlightLeg_based_on_XY_or_OSM first try] Failed to find an airport in expected time. Skipping flight leg: " + inFlightLegName + "Maybe share these findings with the developer... ");
      return false;
    }
    #endif // ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL
  }


  // v3.0.241.8 handle: what if we failed to find a NavAid due to the user setup slider or the designer not enough given radius. We will try with the "designer" area but multiply by 4.
  if ((outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f) && this->mapNavAidsFromMainThread.empty() && location_value_d > 0.0)
  {
    this->shared_navaid_info.inMinDistance_nm = this->shared_navaid_info.inStartFromDistance_nm;
    this->shared_navaid_info.inMaxDistance_nm = static_cast<float> ( location_value_d );
    if (this->shared_navaid_info.inMinDistance_nm > this->shared_navaid_info.inMaxDistance_nm)
    {
      this->shared_navaid_info.inMaxDistance_nm = this->shared_navaid_info.inMinDistance_nm * 4.0f; // max distance is equel to "start distance" * 4.
    }

    #ifdef IBM
    outNewNavInfo = this->get_random_airport_from_db(this->shared_navaid_info.p, this->shared_navaid_info.inMinDistance_nm, this->shared_navaid_info.inMaxDistance_nm, this->shared_navaid_info.inExcludeAngle); // v3.0.255.3 test integration
    #else
    NavAidInfo nav = this->get_random_airport_from_db(this->shared_navaid_info.p, this->shared_navaid_info.inMinDistance_nm, this->shared_navaid_info.inMaxDistance_nm, this->shared_navaid_info.inExcludeAngle); // v3.0.255.3 test integration
    outNewNavInfo  = nav;
    #endif

    #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
    if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f) // do we need to fall back to old code
    {
      if (!this->waitForPluginCallbackJob(missionx::mx_flc_pre_command::gather_random_airport_mainThread, std::chrono::milliseconds(1000))) // pick random airport. Wait up to 10sec
      {
        this->setError("[random get_target_or_lastFlightLeg_based_on_XY_or_OSM second try] Failed to find an airport in expected time. Skipping flight leg: " + inFlightLegName + ", Consider sharing these findings with the developer... ");
        return false;
      }
    }
    #endif

  } // end gathering random NavAid

  // Filter location by location_type (NEAR, ICAO etc...)
  // Add find the closest airport to last location for location_type = NEAR

  #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
  if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f)
  {
    if (inLocationType.compare(mxconst::get_EXPECTED_LOCATION_TYPE_NEAR()) == 0)
      this->getRandomAirport_localThread(outNewNavInfo, mxconst::get_EXPECTED_LOCATION_TYPE_NEAR());
    else
      this->getRandomAirport_localThread(outNewNavInfo); // pick random airport from list of valid locations
  }
  #endif // ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL

  outNewNavInfo.synchToPoint();

  if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f)
  {
    this->setError("[random get_target get_target_or_lastFlightLeg_based_on_XY_or_OSM] Failed to find an airport in radius: " + Utils::formatNumber<double>(location_value_d, 2) +
                   "nm relative to location: " + Utils::formatNumber<double>(lastFlightLegNavInfo.lat) + "," + Utils::formatNumber<double>(lastFlightLegNavInfo.lon));
    return false;
  }

  return true;
} // end handle random x/y or random navaid



// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------

} /* namespace missionx */
