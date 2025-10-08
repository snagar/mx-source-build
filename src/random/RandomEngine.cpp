/*
 * RandomEngine.cpp
 *
 *  Created on: Dec 13, 2018
 *      Author: snagar
 */
#include "RandomEngine.h"
#include "../core/dataref_manager.h" // v3.305.2 added
#include "../io/ListDir.h"
#include "../io/system_actions.h"

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
// #define ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL 0

std::thread                         missionx::RandomEngine::thread_ref;
missionx::base_thread::thread_state missionx::RandomEngine::threadState;

std::map<std::string, std::string>                          missionx::RandomEngine::row_gather_db_data;
std::unordered_map<int, std::map<std::string, std::string>> missionx::RandomEngine::resultTable_gather_random_airports;
std::unordered_map<int, std::map<std::string, std::string>> missionx::RandomEngine::resultTable_gather_ramp_data;

//// weather
std::string missionx::RandomEngine::current_weather_datarefs_s;


mx_plane_types missionx::RandomEngine::template_plane_type_enum; // v25.06.1

bool            RandomEngine::flag_force_template_distances_b; // v25.09.1
missionx::Point RandomEngine::planeLocation; // v25.09.2
missionx::NavAidInfo RandomEngine::lastFlightLegNavInfo; // v25.09.2
// bool RandomEngine::flag_picked_from_osm_database; // v25.09.2 deprecated
IXMLNode RandomEngine::xRootTemplate; // v25.09.2
missionx::TemplateFileInfo *RandomEngine::working_tempFile_ptr; // v25.09.2
RandomEngine::random_airport_info_struct RandomEngine::shared_navaid_info; // v25.09.2
std::string RandomEngine::errMsg; // v25.09.2
std::vector<std::string> RandomEngine::vecMissionInfoOverpassUrls; // v25.09.2
int                      RandomEngine::current_url_indx_used_i = mxconst::INT_UNDEFINED; // v25.09.2
std::list<missionx::NavAidInfo> RandomEngine::listNavInfo; // v25.09.2
std::map<XPLMNavRef, missionx::NavAidInfo> RandomEngine::mapNavAidsFromMainThread;                           // v3.0.221.4 holds nav aid data from main plugin thread so thread will process it later in the background



RandomEngine::RandomEngine ()
{

  // this line is to test against other compilers too.
  mapPlaneEnumToStringTypes.clear ();
  for (auto &[key, value] : mapPlaneStringTypesToEnum)
  {
    mapPlaneEnumToStringTypes[value] = key; // for translation
  }

  missionx::RandomEngine::working_tempFile_ptr          = nullptr; // v3.0.241.9
  this->flag_rules_defined_by_user_ui = false; // v3.0.241.9
  // missionx::RandomEngine::flag_picked_from_osm_database = false; // v3.0.241.10

  init ();
}

// -----------------------------------

void
RandomEngine::init ()
{
  // clear
  this->pathToRandomBrieferFolder.clear ();
  this->pathToRandomRootFolder.clear ();

  RandomEngine::errMsg.clear ();

  xTargetMainNode = IXMLNode::emptyIXMLNode;

  xTargetMainNode = IXMLNode::createXMLTopNode ("xml", TRUE);
  xTargetMainNode.addAttribute (mxconst::get_ATTRIB_VERSION ().c_str (), "1.0");
  xTargetMainNode.addAttribute ("encoding", "ASCII"); // "ISO-8859-1");
  // add disclaimer
  xTargetMainNode.addClear ("\n\tFile has been created by Mission-X plug-in.\n\tAny modification might break or invalidate the file.\n\t", "<!--", "-->");

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

  this->mapFlightPlanOrder_si.clear ();
  this->mapFLightPlanOrder_is.clear ();

  RandomEngine::listNavInfo.clear ();
  this->flag_found = false;

  missionx::RandomEngine::template_plane_type_enum = missionx::mx_plane_types::plane_type_any; // v3.0.221.11
  RandomEngine::planeLocation.init ();
  RandomEngine::lastFlightLegNavInfo.init ();

  this->flag_isLastFlightLeg = false; // v3.0.219.11

  expected_slope_at_target_location_d = 0.0f;

  missionx::RandomEngine::threadState.thread_wait_state = missionx::mx_random_thread_wait_state_enum::not_waiting; // v3.0.221.3
  missionx::RandomEngine::mapNavAidsFromMainThread.clear ();
  this->map_customScenery_XPLMNavRef_NavAidsFromMainThread.clear ();

  this->cumulative_location_desc_s.clear ();
}

// -----------------------------------

void
RandomEngine::addTriggersBasedOnTargetLocation (NavAidInfo &inNav, IXMLNode &inSpecialLegSubNode)
{
  IXMLNode xSpecialTriggers;

  if (inSpecialLegSubNode.isEmpty ())
    return;

  // search for set of triggers to add to xTriggers
  const std::string add_triggers_from_template = Utils::readAttrib (inSpecialLegSubNode, mxconst::get_ATTRIB_ADD_TRIGGERS_FROM_TEMPLATE (), ""); // v3.0.221.10 should hold the tag name of the messages we want to xMessages
  if (!add_triggers_from_template.empty ())
    xSpecialTriggers = xRootTemplate.getChildNode (add_triggers_from_template.c_str ()); // if we find an element with this name, we will add all sub elements to xTriggers

  // try to get radius
  // Add pre-defined triggers
  if (!xSpecialTriggers.isEmpty ())
  {
    Utils::add_xml_comment (xTriggers, " (((( Added Triggers )))) ");

    int nChilds = xSpecialTriggers.nChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str ());
    for (int i1 = 0; i1 < nChilds; ++i1)
    {
      IXMLNode cNode = xSpecialTriggers.getChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str (), i1);
      if (!cNode.isEmpty ())
      {
        // Try to modify radius if it is empty at the added trigger node. we use the data from inNav class that should hold the new target location
        std::string trigger_radius_mt = Utils::xml_get_attribute_value_drill (cNode, mxconst::get_ATTRIB_RADIUS_MT (), this->flag_found, mxconst::get_ELEMENT_RADIUS ());
        if (mxUtils::is_number (inNav.radius_mt_suggested_s) && trigger_radius_mt.empty ()) // v3.0.221.10 added the radius_mt to triggers so will be same area as target
          Utils::xml_search_and_set_attribute_in_IXMLNode (cNode, mxconst::get_ATTRIB_RADIUS_MT (), inNav.radius_mt_suggested_s, mxconst::get_ELEMENT_POINT ());

        Utils::xml_search_and_set_attribute_in_IXMLNode (cNode, mxconst::get_ATTRIB_LAT (), inNav.getLat (), mxconst::get_ELEMENT_POINT ());
        Utils::xml_search_and_set_attribute_in_IXMLNode (cNode, mxconst::get_ATTRIB_LONG (), inNav.getLon (), mxconst::get_ELEMENT_POINT ());

        xTriggers.addChild (cNode.deepCopy ());
      }
    }

    Utils::add_xml_comment (xTriggers, " )))) End Added Triggers (((( ");
  }
}

// -----------------------------------

void
RandomEngine::injectCountdownTimers ()
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
    if (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_user_generates_a_mission_layer)
    {
      const bool bAddTimers = Utils::readBoolAttrib (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_ADD_COUNTDOWN (), false);
      if (bAddTimers)
      {
        // loop over all flight legs and add timer to all of them except the last.
        std::string err;
        #ifndef RELEASE
        Log::logMsg ("[DEBUG random] inject countdown timers.", true);
        #endif

        auto lmbda_get_briefer_nav_aid = [&] (const std::string &inName)
        {
          NavAidInfo *nav_ptr = nullptr;
          for (auto &nav : RandomEngine::listNavInfo)
          {
            if (nav.flag_is_brieferOrStartLocation || nav.flightLegName == inName) // same unique Leg name // v3.303.10 added the briefer flag logic
              return &nav;
          }

          return nav_ptr;
        };

        NavAidInfo *brieferNav = lmbda_get_briefer_nav_aid (mxconst::get_ELEMENT_BRIEFER ());
        if (brieferNav == nullptr)
          return;

        NavAidInfo *prevNav = brieferNav; // v3.0.253.9.1 will hold NavAid info to calculate distance. Initialize with the briefer information for calculating distance between start and first leg
        // IXMLNode    prev_leg_ptr = IXMLNode::emptyIXMLNode;

        assert (prevNav != nullptr && "[inject countdown] Failed to find briefer element");

        const int nChilds = this->xFlightLegs.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());

        for (int i1 = 0; i1 < nChilds; ++i1)
        {
          IXMLNode leg_ptr = xFlightLegs.getChildNode (mxconst::get_ELEMENT_LEG ().c_str (), i1); // pointer to <leg> xml element
          if (leg_ptr.isEmpty ())
            continue;

          std::string flight_leg_name = Utils::readAttrib (leg_ptr, mxconst::get_ATTRIB_NAME (), "");

          auto iterEnd = RandomEngine::listNavInfo.end ();
          for (auto iter = RandomEngine::listNavInfo.begin (); iter != iterEnd; ++iter)
          {

            if (iter->flightLegName == flight_leg_name)
            {
              auto         nextIter                  = std::next (iter);
              const double distance_to_next_navaid_d = iter->p - prevNav->p;

              const auto time_relative_to_avg_speed_in_min = distance_to_next_navaid_d / HELICOPTER_AVERAGE_SPEED_IN_KNOTS * 60; // we multiply by 60 minutes to get hours

              const auto minVal    = (time_relative_to_avg_speed_in_min > MIN_SEARCH_TIME_IN_MIN) ? time_relative_to_avg_speed_in_min : MIN_SEARCH_TIME_IN_MIN;
              const auto maxVal    = (minVal <= MIN_SEARCH_TIME_IN_MIN) ? MIN_SEARCH_TIME_IN_MIN + HOVER_TIME + Utils::getRandomRealNumber (5.0, 10.0) : time_relative_to_avg_speed_in_min + HOVER_TIME; // v3.0.255.4 fixed assertion where minVal was larger than maxVal.
              const int  timeInMin = static_cast<int> (Utils::getRandomRealNumber (minVal, maxVal));

              auto xml_timer_ptr = Utils::xml_get_or_create_node_ptr (leg_ptr, mxconst::get_ELEMENT_TIMER ());
              if (!xml_timer_ptr.isEmpty ())
              {
                xml_timer_ptr.updateAttribute ((flight_leg_name + "_timer").c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
                xml_timer_ptr.updateAttribute (mxUtils::formatNumber<int> (timeInMin).c_str (), mxconst::get_ATTRIB_TIME_MIN ().c_str (), mxconst::get_ATTRIB_TIME_MIN ().c_str ());

                if (nextIter != iterEnd)
                  xml_timer_ptr.updateAttribute (nextIter->flightLegName.c_str (), mxconst::get_ATTRIB_RUN_UNTIL_LEG ().c_str (), mxconst::get_ATTRIB_RUN_UNTIL_LEG ().c_str ());
                else
                  xml_timer_ptr.updateAttribute ("", mxconst::get_ATTRIB_RUN_UNTIL_LEG ().c_str (), mxconst::get_ATTRIB_RUN_UNTIL_LEG ().c_str ());

              } // end if we have valid <timer> node pointer

              prevNav = &(*iter);
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
RandomEngine::get_user_wants_to_start_from_plane_position ()
{
  if (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_ils_layer || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_external_fpln_layer || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::flight_leg_info)
    return Utils::readBoolAttrib (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_START_FROM_PLANE_POSITION (), false);

  return false;
}


// -----------------------------------

RandomEngine::~RandomEngine ()
{
  iDomTemplate.clear ();
}

// -----------------------------------

void
RandomEngine::setError (const std::string &inMsg)
{
  RandomEngine::errMsg = inMsg;
  #ifndef RELEASE
  Log::logMsgNone (errMsg, true);
  #endif
}

// -----------------------------------

bool
RandomEngine::exec_generate_mission_thread (const std::string &inKey)
{
  if (missionx::RandomEngine::threadState.flagIsActive)
  {
    RandomEngine::setError ("\"Generate Mission Engine\" is already running. Please wait for it to finish.");
    return false;
  }

  // start thread
  if (!missionx::RandomEngine::threadState.flagIsActive)
  {
    if (missionx::RandomEngine::thread_ref.joinable ()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
      missionx::RandomEngine::thread_ref.join (); // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"

    this->init (); // reset all variables
    RandomEngine::threadState.dataString = inKey;
    missionx::RandomEngine::thread_ref   = std::thread (&missionx::RandomEngine::generateRandomMission, this);
  }

  return true;
}

// -----------------------------------

void
RandomEngine::stop_plugin ()
{
  RandomEngine::threadState.flagAbortThread = true;
  if (RandomEngine::threadState.flagIsActive)
  {
    std::this_thread::sleep_for (std::chrono::seconds (1));
  }
  if (RandomEngine::thread_ref.joinable ()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
    RandomEngine::thread_ref.join (); // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"
}

// -----------------------------------

std::string
RandomEngine::inject_files_into_xml (missionx::TemplateFileInfo *tempFile_ptr)

{
  if (tempFile_ptr != nullptr)
  {
    fs::path template_file = tempFile_ptr->getAbsoluteTemplateXmlFilePath ();
    if (fs::exists (template_file) && fs::is_regular_file (template_file))
    {

      //// Reconstruct the XML file parse template and remove <mission_info>
      std::string err;
      IXMLNode    xMappings{ IXMLNode::emptyIXMLNode };
      IXMLNode    xREPLACE_OPTIONS{ IXMLNode::emptyIXMLNode }; // v3.0.255.4.1

      if (ITCXMLNode x_mapping_node = this->iDomTemplate.openFileHelper (template_file.string ().c_str (), mxconst::get_MAPPING_ROOT_DOC ().c_str (), &err); err.empty () && !x_mapping_node.isEmpty ())
        xMappings = x_mapping_node.deepCopy ();

      err.clear ();
      ITCXMLNode x_replace_options = this->iDomTemplate.openFileHelper (template_file.string ().c_str (), mxconst::get_ELEMENT_TEMPLATE_REPLACE_OPTIONS ().c_str (), &err); // parse xml into ITCXMLNode
      if (err.empty () && !x_replace_options.isEmpty ())
        xREPLACE_OPTIONS = x_replace_options.deepCopy ();

      err.clear ();
      ITCXMLNode xTemplateNode = this->iDomTemplate.openFileHelper (template_file.string ().c_str (), mxconst::get_TEMPLATE_ROOT_DOC ().c_str (), &err); // parse xml into ITCXMLNode

      if (!err.empty () && missionx::RandomEngine::xRootTemplate.isEmpty ()) // check if there is any failure during read
      {
        Log::logMsgThread ("[random error inject find/replace] " + err);
        return missionx::EMPTY_STRING;
      }
      err.clear ();

      auto x_root_template_node = xTemplateNode.deepCopy (); // convert ITCXMLNode to IXMLNode. IXMLNode allow to modify itself

      // IXMLRenderer xmlRender;
      std::string  xml_template_node_content_s =  Utils::xml_get_node_content_as_text (x_root_template_node); //xmlRender.getString (xRootTemplate);

      std::ios_base::sync_with_stdio (false);
      std::cin.tie (nullptr);

      if (!xml_template_node_content_s.empty ())
      {
        const std::string original_xml_template_node_content_s = xml_template_node_content_s; // store original string

        // check if we have valid user pick value // v24.03.2 added "vecReplaceOptions_s" empty check to solve crash
        if (!tempFile_ptr->mapOptionsInfo.empty ())
        {
          bool bDoneReplacement = false;
          for (const auto &[seq_key, option_info] : tempFile_ptr->mapOptionsInfo)
          {
            // Compatibility option, will have a "seq_key = -1", so it needs the "<REPLACE_OPTIONS>" node as parent node, while "seq_key>=0" needs the correct <option> sub node.
            auto pNode = (seq_key < 0) ? tempFile_ptr->nodeReplaceOptions : tempFile_ptr->nodeReplaceOptions.getChildNode (mxconst::get_ELEMENT_OPTION_GROUP ().c_str (), seq_key);
            if (pNode.isEmpty ()) // skip if node is empty
              continue;

            const std::string user_pick_s = option_info.vecReplaceOptions_s.at (static_cast<size_t> (option_info.user_pick_from_replaceOptions_combo_i));
            // find the <opt > node with user_pick_s name
            // v24.12.2 Use the "parent" node to get the correct "<opt>" child node based on the "user_pick_s" value.
            auto optNode_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (pNode, mxconst::get_ELEMENT_OPT (), mxconst::get_ATTRIB_NAME (), user_pick_s, false);
            if (optNode_ptr.isEmpty ())
              continue;


            bDoneReplacement = true;

            // loop over all <find_replace>
            const int nFindReplaceCount = optNode_ptr.nChildNode (mxconst::get_ELEMENT_FIND_REPLACE ().c_str ());
            for (int i = 0; i < nFindReplaceCount; ++i)
            {
              auto              frNode              = optNode_ptr.getChildNode (mxconst::get_ELEMENT_FIND_REPLACE ().c_str (), i);
              const std::string find_s              = Utils::readAttrib (frNode, mxconst::get_ATTRIB_FIND (), "");
              const std::string replace_with_file_s = Utils::readAttrib (frNode, mxconst::get_ATTRIB_REPLACE_WITH (), "");
              if (find_s.empty () || replace_with_file_s.empty ())
                continue;
              else
              {
                fs::path txt_file = tempFile_ptr->filePath + "/" + replace_with_file_s;
                if (fs::exists (txt_file) && fs::is_regular_file (txt_file))
                {
                  char          c = '\0';
                  std::ifstream infs_txt;
                  infs_txt.open (txt_file, std::ios::in);
                  if (infs_txt.is_open ())
                  {
                    std::string line_txt;
                    std::string txt_file_content_s;

                    while (infs_txt.get (c) && !infs_txt.eof ())
                    {
                      txt_file_content_s += c;

                    }; // read all file

                    infs_txt.close ();

                    xml_template_node_content_s = Utils::replaceString (xml_template_node_content_s, find_s, txt_file_content_s, true); // replace string but skip the first occurence since it will hold the find replace
                  }
                }
                else if (!replace_with_file_s.empty ()) // v3.0.255.4.1 if file not exists then use the string as the replacer
                {
                  xml_template_node_content_s = Utils::replaceString (xml_template_node_content_s, find_s, replace_with_file_s, true); // replace string but skip the first occurence since it will hold the find replace
                }


              } // end else if attrib values are valid

            } // end loop over all <find_replace>

          } // end loop over all "multi option" container

          // Write to new template working file ?
          // after finishing the loop check if xml_file_content_s different than original_xml_file_content_s
          if ((xml_template_node_content_s != original_xml_template_node_content_s) && bDoneReplacement)
          {
            // write to tmp file
            fs::path output_file = tempFile_ptr->filePath + "/" + mxconst::get_TEMPLATE_INJECTED_FILE_NAME ();

            // try to delete previous work file
            if (fs::exists (output_file) && fs::is_regular_file (output_file))
            {
              if (fs::remove (output_file))
              {
                Log::logMsgThread ("[random] File: " + output_file.string () + ", deleted.");
              }
              else
              {
                Log::logMsgThread ("[random] File: " + output_file.string () + ", failed to be removed.");

                // abort the running random engine
              }
            }

            // create new XML template file
            auto xml_new_target_main_node = IXMLNode::createXMLTopNode ("xml", TRUE);
            xml_new_target_main_node.addAttribute (mxconst::get_ATTRIB_VERSION ().c_str (), "1.0");
            xml_new_target_main_node.addAttribute ("encoding", "ASCII"); // "ISO-8859-1");
            xml_new_target_main_node.addClear ("\n\tFile has been created by Mission-X plug-in.\n\tAny modification might break or invalidate the file.\n\t", "<!--", "-->");

            // parse the template
            IXMLResults parse_result_strct;
            if (auto newTemplate = this->iDomTemplate.parseString (xml_template_node_content_s.c_str (), mxconst::get_ELEMENT_TEMPLATE ().c_str (), &parse_result_strct).deepCopy (); newTemplate.isEmpty ())
            {
              const std::string translateError = IXMLRenderer::getErrorMessage (parse_result_strct.errorCode);
              Log::logMsgThread ("[ERROR in Template]: \n===================>>\n" + xml_template_node_content_s + "\n<<===========================\n");
              Log::logMsgThread ("[random] error in generated TEMPLATE element. " + translateError + ", line: " + mxUtils::formatNumber<long long> (parse_result_strct.nLine) + ", column: " + mxUtils::formatNumber<int> (parse_result_strct.nColumn) + " \n");
              RandomEngine::setError ("[random] TEMPLATE ERROR: modified template is not a valid XML. Check Log.txt for more information.");
              missionx::RandomEngine::threadState.flagAbortThread = true;
              this->abortThread ();
            }
            else
            {
              // v24.12.2 deprecated code below, because it is used for debug purposes. I'll rewrite it to support the multi-option container
              //// v3.0.255.4.1 copy all attributes from <opt_name><info> over <mission_info>
              // if (optNode_ptr.nChildNode(mxconst::get_ELEMENT_INFO().c_str()) > 0)
              //{
              //   auto missionInfoNode_ptr = newTemplate.getChildNode(mxconst::get_ELEMENT_MISSION_INFO().c_str()); // v3.0.255.4.1
              //   if (!missionInfoNode_ptr.isEmpty())
              //   {
              //     auto infoNode = optNode_ptr.getChildNode(mxconst::get_ELEMENT_INFO().c_str());
              //     Utils::xml_copy_node_attributes_excluding_black_list(infoNode, missionInfoNode_ptr);
              //   }
              // }

              xml_new_target_main_node.addClear (" Modified Template ", "<!--", "-->");
              xml_new_target_main_node.addChild (newTemplate.deepCopy ());

              xml_new_target_main_node.addClear (" Mapping ", "<!--", "-->");
              if (!xMappings.isEmpty ())
                xml_new_target_main_node.addChild (xMappings);

              xml_new_target_main_node.addClear (" Replace Option ", "<!--", "-->");
              if (!xREPLACE_OPTIONS.isEmpty ())
                xml_new_target_main_node.addChild (xREPLACE_OPTIONS);
            }

            IXMLRenderer xmlRender; // v25.09.2 moved before XML write
            IXMLErrorInfo result = xmlRender.writeToFile (xml_new_target_main_node, output_file.string ().c_str ());
            if (result != IXMLError_None)
            {
              const std::string translateError = IXMLRenderer::getErrorMessage (result);
              Log::logMsgThread ("[random] error writing to template file. Error code: " + translateError);
            }

            return output_file.string (); // we always return the new template file since we want the designer to be aware of the errors including if file was not created.
          }
          else
          {
            Log::logMsgThread ("[RandomEngine] Template file and working file content are the same.\n");
          }

        } // end if optNode is not Empty

      } // end if infs_xml is open

    } // end if original XML file exists

  } // end if tempInfo is valid pointer


  return "";
}

// -----------------------------------


bool
RandomEngine::generateRandomMission ()
{
  auto   startThreadClock      = std::chrono::steady_clock::now ();
  double duration              = 0.0;
  bool   flag_copy_leg_as_is_b = false; // v3.0.303 used with templates that has copy_leg_as_is_b="yes"
  bool   flag_surprise_me_b    = false; // v25.06.1
  bool   flag_oilrig_b         = false; // v25.09.1

  std::string err;
  //// Thread initialization state
  missionx::RandomEngine::threadState.flagIsActive       = true;
  missionx::RandomEngine::threadState.flagThreadDoneWork = false;
  missionx::RandomEngine::threadState.flagAbortThread    = false;

  this->reset_sequence_numbers(); // v25.06.1

  missionx::RandomEngine::threadState.startThreadStopper ();

  // missionx::RandomEngine::flag_picked_from_osm_database = false; // v3.0.241.10
  bool        result                                    = true;
  std::string pathToTemplateFile;
  pathToTemplateFile.clear ();
  RandomEngine::listNavInfo.clear ();
  this->setInventories.clear ();
  missionx::RandomEngine::map_flight_legs_translation_from_template.clear (); // v25.09.1

  std::string inKey = missionx::RandomEngine::threadState.dataString;

  /////////////////////////////////////////////////////////////////////
  ////// Read queries from external file //////////////////////////////

  #ifndef RELEASE
  Log::logAttention ("\n=========>\n[random airport] Reading external queries", true);
  #endif

  missionx::RandomEngine::initQueries (); // internal initialization so we will have a baseline to work with.

  err.clear (); // v3.0.223.1

  Utils::read_external_sql_query_file (missionx::data_manager::mapQueries, mxconst::get_SQLITE_OSM_SQLS ());

  /////////// End read queries from external file /////////////
  ////////////////////////////////////////////////////////////

  #ifndef RELEASE
  Log::logAttention ("\n=========>\n[random engine] start generating random mission", true);
  #endif
  if (missionx::RandomEngine::working_tempFile_ptr == nullptr) // v3.0.241.9 work with pointer to File Information
  {
    RandomEngine::setError ("[Random]Failed to find template by the name: " + inKey); // this should be displayed
    missionx::RandomEngine::threadState.flagAbortThread = true;
    return false;
  }

  //// Read from cached file if our current cache is empty
  // this->readOptimizedAptDatIntoCache ();

  //// set folders path ////
  this->pathToRandomRootFolder    = (RandomEngine::working_tempFile_ptr->missionFolderName.empty ()) ? data_manager::mx_folders_properties.getAttribStringValue (mxconst::get_FLD_MISSIONS_ROOT_PATH (), "", err) : RandomEngine::working_tempFile_ptr->filePath; // v3.0.241.10 b2 decide from where to pick template file
  this->pathToRandomBrieferFolder = (RandomEngine::working_tempFile_ptr->missionFolderName.empty ()) ? pathToRandomRootFolder + mxconst::get_FOLDER_SEPARATOR () + mxconst::get_FOLDER_RANDOM_MISSION_NAME () + mxconst::get_FOLDER_SEPARATOR () + mxconst::get_BRIEFER_FOLDER () : pathToRandomRootFolder + mxconst::get_FOLDER_SEPARATOR () + mxconst::get_BRIEFER_FOLDER (); // v3.0.241.10 b2 define the output folder of the template

  // store current plane coordinate
  RandomEngine::planeLocation = missionx::dataref_manager::getCurrentPlanePointLocation ();


  #ifndef RELEASE
  Log::logMsgThread ("[random engine] Working on template: " + inKey + "\n<========");
  #endif

  // Read TEMPLATE xml file using DOM
  pathToTemplateFile = RandomEngine::working_tempFile_ptr->getAbsoluteTemplateXmlFilePath ();
  if (pathToTemplateFile.empty ())
  {
    RandomEngine::setError ("[random engine] Failed generating mission using template: " + inKey);

    missionx::RandomEngine::threadState.flagAbortThread = true;
    return false;
  }

  ////// INJECT user option data ///////
  if (!RandomEngine::working_tempFile_ptr->mapOptionsInfo.empty ()) // find/replace code
  {
    // 1. read user option and store the option name
    // 2. read the template file into a std::string
    // 3. Loop over all <find_replace> elements and replace all XX with the content of file YY
    if (const std::string newTemplateFile = this->inject_files_into_xml (RandomEngine::working_tempFile_ptr);
      !newTemplateFile.empty ())
      pathToTemplateFile = newTemplateFile;

    if (missionx::RandomEngine::threadState.flagAbortThread)
      return false;
  }

  ////// READ MAPPING from template file /////////
  // Validate the <MAPPING> element do exists in template file
  missionx::data_manager::read_element_mapping (pathToTemplateFile); // v3.0.217.4
  if (missionx::data_manager::xmlMappingNode.isEmpty ()) // v3.0.221.15rc3.4
  {
    RandomEngine::setError ("[random] ERROR: Mapping element is missing from template file: " + inKey + ". Fix template file. Aborting mission generating.");
    missionx::RandomEngine::threadState.flagAbortThread = true;

    return false;
  }


  //////// ========= READING FROM TEMPLATE ============= /////////////////
  // read mission template
  err.clear (); // v3.0.223.1
  ITCXMLNode xTemplateNode = this->iDomTemplate.openFileHelper (pathToTemplateFile.c_str (), mxconst::get_TEMPLATE_ROOT_DOC ().c_str (), &err); // parse xml into ITCXMLNode
  missionx::RandomEngine::xRootTemplate      = xTemplateNode.deepCopy (); // convert ITCXMLNode to IXMLNode. IXMLNode allow to modify itself
  if (!err.empty () && missionx::RandomEngine::xRootTemplate.isEmpty ()) // check if there is any failure during read
  {
    RandomEngine::setError (err);
    missionx::RandomEngine::threadState.flagAbortThread = true;
    return false;
  }

  bool flag_created_based_on_content_element = false; // moved the bool declaration here to solve the cross compile issue in gcc and the goto command.

  /////// =================== Prepare TARGET XML Nodes ===================================================
  //// add MISSION root node
  xDummyTopNode = xTargetMainNode.addChild (mxconst::get_MISSION_ELEMENT ().c_str ());
  xDummyTopNode.addAttribute (mxconst::get_ATTRIB_VERSION ().c_str (), missionx::PLUGIN_FILE_VER);

  if (RandomEngine::working_tempFile_ptr->missionFolderName.empty ())
    xDummyTopNode.addAttribute (mxconst::get_ATTRIB_NAME ().c_str (), RandomEngine::working_tempFile_ptr->fileName.c_str ()); // v3.0.241.9 replace direct map with pointer to TemplateFileInfo
  else
    xDummyTopNode.addAttribute (mxconst::get_ATTRIB_NAME ().c_str (), RandomEngine::working_tempFile_ptr->missionFolderName.c_str ()); // v3.0.241.10 b2 handle cases where we read template from custom folder. We need to use the folder name and not template filename which might not be unique


  // prepare main elements for a generated mission file.
  this->xMetadata   = xDummyTopNode.addChild (mxconst::get_ELEMENT_METADATA ().c_str ()); // v24.12.1
  this->xFlightLegs = xDummyTopNode.addChild (mxconst::get_ELEMENT_FLIGHT_PLAN ().c_str ());
  this->xObjectives = xDummyTopNode.addChild (mxconst::get_ELEMENT_OBJECTIVES ().c_str ());
  this->xTriggers   = xDummyTopNode.addChild (mxconst::get_ELEMENT_TRIGGERS ().c_str ());
  this->xInventoris = xDummyTopNode.addChild (mxconst::get_ELEMENT_INVENTORIES ().c_str ());
  this->xGPS        = xDummyTopNode.addChild (mxconst::get_ELEMENT_GPS ().c_str ());
  this->xChoices    = xDummyTopNode.addChild (mxconst::get_ELEMENT_CHOICES ().c_str ());

  Utils::xml_delete_all_subnodes (xRootTemplate, mxconst::get_ELEMENT_METADATA (), true); // v24.12.1
  xRootTemplate.addChild (xMetadata); // v24.12.1 I add the xMetadata to the xRootTemplate, since I use it in many function, and so we have access to it from the Node instead from "this->".

  // read Object Template node from template
  x3DObjTemplate = xRootTemplate.getChildNode (mxconst::get_ELEMENT_OBJECT_TEMPLATES ().c_str ()).deepCopy (); // v3.0.217.6
  if (x3DObjTemplate.isEmpty ())
    x3DObjTemplate = xTargetMainNode.addChild (mxconst::get_ELEMENT_OBJECT_TEMPLATES ().c_str ()); // v3.0.219.1

  // read message templates
  IXMLNode xMessageTemplates = xRootTemplate.getChildNode (mxconst::get_ELEMENT_MESSAGE_TEMPLATES ().c_str ()).deepCopy (); // v3.0.223.4
  if (!xMessageTemplates.isEmpty ())
    xMessages = xMessageTemplates;
  else
    xMessages = xDummyTopNode.addChild (mxconst::get_ELEMENT_MESSAGE_TEMPLATES ().c_str ());

  Log::logDebugBO ("[DEBUG random airport] After preparing new mission file main nodes.", true);

  // Which Mission Type ?
  auto med_cargo_or_oilrig_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::medevac)); // 0 = med, 1 = cargo

  // --------------- Special cases ------------------------------------
  /// Handle special template cases. "user_driven_mission_layer" or "option_external_fpln_layer" layers is handled later on
  int nContentChilds_i = 0; // v25.08.1

  if (this->flag_rules_defined_by_user_ui)
  {

    // call function that injects all the leg skeleton
    if (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_user_generates_a_mission_layer)
    {
      std::string local_err;
      // v24.12.1 Add metadata child info
      assert (xMetadata.isEmpty () == false && fmt::format ("{}, <{}> is invalid. Creation failure. Notify developer.", __func__, mxconst::get_ELEMENT_METADATA ()).c_str ());
      // set the metadata attributes

      #ifndef RELEASE
      // Log::logMsgThread (fmt::format (R"(Before calling "prepare_blank_template_with_flight_legs_based_on_ui":\n{})", Utils::xml_get_node_content_as_text (xRootTemplate)));
      #endif

      if (!this->prepare_blank_template_with_flight_legs_based_on_ui (xRootTemplate, this->xMetadata, local_err))
      {
        RandomEngine::setError (local_err);
        missionx::RandomEngine::threadState.flagAbortThread = true;

        // #ifndef RELEASE
        // Log::logMsg ("\n ============== After calling prepare_blank_template_with_flight_legs_based_on_ui.\nTEMPLATE ==============> \n" + Utils::xml_get_node_content_as_text (xRootTemplate) + "\n\n========= END USER GENERATED TEMPLATE ===========\n\n", true);
        // #endif
      }

      // v25.05.1 check surprise me
      // reading from metadata must come after "prepare_blank_template_with_flight_legs_based_on_ui()" since it initialize it.
      flag_surprise_me_b = Utils::readBoolAttrib (xMetadata, mxconst::get_ATTRIB_SURPRISE_ME_SUB_CAT_B (), false); // v25.06.1

      if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::medevac) && flag_surprise_me_b)
      {
        // v25.06.1
        auto mxReturn = prepare_medevac_surprise_me (xRootTemplate, xMetadata, planeLocation);

        // // FORCE failure for now until all parts will fit.
        // missionx::mx_return mxReturn = false;
        // RandomEngine::setError ("The 'Surprise me' logic is still a Work In Progress.");
        // this->abortThread ();

        if (!mxReturn.result)
        {
          Log::logMsgThread (mxReturn.getInfoAsText ());
          Log::logMsgThread (mxReturn.getErrorsAsText ());
          RandomEngine::setError (mxReturn.getErrorsAsText ());
          missionx::RandomEngine::threadState.flagAbortThread = true;
        }

        // check [abort]
        if (missionx::RandomEngine::threadState.flagAbortThread)
          return false;

      }

      // check oilrig
      if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::oil_rig))
      {
        // if (!this->prepare_mission_based_on_oilrig (xRootTemplate, local_err))
        flag_oilrig_b = true;
        auto func_result = this->prepare_mission_based_on_oilrig2 (xRootTemplate, local_err);
        if (!func_result.result)
        {
          RandomEngine::setError (func_result.getErrorsAsText ());
          missionx::RandomEngine::threadState.flagAbortThread = true;
        }


      } // end if oilrig mission

    } // end handling user_driven_mission_layer
    else if (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_external_fpln_layer)
    {
      if (!this->prepare_mission_based_on_external_fpln (xRootTemplate))
      {
        RandomEngine::setError ("Failed to build mission based on external FPLN !!!");
        missionx::RandomEngine::threadState.flagAbortThread = true;
      }
      else
      {
        goto post_mission_action;
      }
    }
    else if (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_ils_layer)
    {
      if (!this->prepare_mission_based_on_ils_search (xRootTemplate))
      {
        RandomEngine::setError ("Failed to build mission based on ILS FPLN !!!");
        missionx::RandomEngine::threadState.flagAbortThread = true;
      }
      else
      {
        goto post_mission_action;
      }
    }
    else if (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::flight_leg_info)
    {
      if (!this->prepare_mission_based_on_user_fpln_or_simbrief (xRootTemplate))
      {
        RandomEngine::setError ("Failed to build mission based on User/Simbrief FPLN !!!");
        missionx::RandomEngine::threadState.flagAbortThread = true;
      }
      else
      {
        goto post_mission_action;
      }
    }
  }
  // <content> based template
  else if (nContentChilds_i = missionx::RandomEngine::xRootTemplate.getChildNode (mxconst::get_ELEMENT_CONTENT ().c_str ()).nChildNode ()
    ; nContentChilds_i > 0)
  {

    // if (!this->generateRandomMissionBasedOnContent (missionx::RandomEngine::xRootTemplate))
    // if (!this->prepare_random_mission_based_on_content (missionx::RandomEngine::xRootTemplate))
    auto func_result = this->gen_prepare_random_mission_based_on_content (missionx::RandomEngine::xRootTemplate);
    if (!func_result.result)
    {
      Log::logMsgThread (func_result.getErrorsAsText ()); // debug to log
      RandomEngine::setError (func_result.getErrorsAsText ());
      missionx::RandomEngine::threadState.flagAbortThread = true;
      return false;
    }

    flag_created_based_on_content_element = true;
    flag_copy_leg_as_is_b                 = Utils::readBoolAttrib (missionx::RandomEngine::xRootTemplate, mxconst::get_ATTRIB_COPY_LEG_AS_IS_B (), false);
  }


  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;


  // v3.0.219.1
  RandomEngine::parse_3D_object_template_element (xRootTemplate, x3DObjTemplate, RandomEngine::errMsg); // go over <object_template> and resolve any <random_tag> before we will parse <display_object>
  ///// =========================================================================================

  Log::logDebugBO ("[DEBUG random airport] After preparing new mission file main nodes.", true);
  this->setPlaneType (mxUtils::stringToLower (Utils::readAttrib (missionx::RandomEngine::xRootTemplate, mxconst::get_ATTRIB_PLANE_TYPE (), mxconst::get_PLANE_TYPE_HELOS ()))); // v3.0.221.15 Default plane is Helicopter.
  Log::logDebugBO ("[DEBUG random airport] After <briefer_info> node.", true);

  if (!flag_surprise_me_b && !flag_oilrig_b && nContentChilds_i == 0) // v25.08.1 split the "content" code and moved it before the call to "parse_3D_object_template_element"
  {
    // read the briefer element before calling "readFlightLegs_directlyFromTemplate()"
    if (missionx::RandomEngine::threadState.flagAbortThread)
      return false;
    else if (!prepareBrieferAndStartLocation ()) // main <briefer>
    {
      missionx::RandomEngine::threadState.flagAbortThread = true;
      return false;
    }

    Log::logDebugBO ("[DEBUG random airport] After <briefer_and_start_location> node.", true);

    // call "readFlightLegs_directlyFromTemplate()"
    if (!readFlightLegs_directlyFromTemplate ()) // v3.0.217.8
    {
      missionx::RandomEngine::threadState.flagAbortThread = true;
      return false;
    }
  }


  // POST_MISSION_ACTIONS:
post_mission_action:

  // call readMissionInfoElement // v3.0.253.1 moved to this location so fetch external code will create briefer info too.
  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;
  if (!flag_created_based_on_content_element && !gen_read_mission_info_element ()) // we can skip this function call if we built the mission based on content element. We need to read it inside content to have the custom <overpass> element from <mission_info>
    return false;

  Utils::xml_delete_empty_nodes (xDummyTopNode); // v3.0.219.3 remove invalid points

  // Check last 2 legs are different ICAO. Will remove the last one.
  this->check_last_2_legs_if_they_have_same_icao (); // v3.0.255.1

  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  // v25.08.1 Consider removing those messages and just keep the 2nm message.
  // v25.02.1 added the option to suppress messages in the setup screen
  if (const auto bSuppressDistanceMessages = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_ATTRIB_SUPPRESS_DISTANCE_MESSAGES_B (), false); 
  !bSuppressDistanceMessages && data_manager::getGeneratedFromLayer () != missionx::uiLayer_enum::option_external_fpln_layer
  && data_manager::getGeneratedFromLayer () != missionx::uiLayer_enum::option_ils_layer && !flag_copy_leg_as_is_b)
  {
    this->injectMessagesWhileFlyingToDestination ();
  }

  Log::logDebugBO ("[DEBUG random airport] After <message_templates> node.", true);

  // inject specific mission type data
  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  if (data_manager::getGeneratedFromLayer () != missionx::uiLayer_enum::option_external_fpln_layer && data_manager::getGeneratedFromLayer () != missionx::uiLayer_enum::option_ils_layer && data_manager::getGeneratedFromLayer () != missionx::uiLayer_enum::flight_leg_info // v25.03.3
      && !flag_copy_leg_as_is_b && ! flag_surprise_me_b && ! flag_oilrig_b && !flag_created_based_on_content_element) // v25.06.1 added flag_surprise_me_b // v25.09.1 added flag_oilrig_b // v25.09.2 added flag_created_based_on_content_element
    this->injectMissionTypeFeatures ();

  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  this->injectCountdownTimers ();


  // v3.0.221.10 Add <xpdata> element if exists
  if (xRootTemplate.nChildNode (mxconst::get_ELEMENT_XPDATA ().c_str ()) > 0)
    this->xpData = xRootTemplate.getChildNode (mxconst::get_ELEMENT_XPDATA ().c_str ());

  if (xRootTemplate.nChildNode (mxconst::get_ELEMENT_EMBEDDED_SCRIPTS ().c_str ()) > 0)
    this->xEmbedScripts = xRootTemplate.getChildNode (mxconst::get_ELEMENT_EMBEDDED_SCRIPTS ().c_str ());

  this->xScoring       = xRootTemplate.getChildNode (mxconst::get_ELEMENT_SCORING ().c_str ()).deepCopy (); // v3.303.9
  this->xCompatibility = xRootTemplate.getChildNode (mxconst::get_ELEMENT_COMPATIBILITY ().c_str ()).deepCopy (); // v24.12.2

  // v3.0.303 add objectives and triggers if we had a content with copy_leg_as_is_b
  if (flag_copy_leg_as_is_b)
  {
    // v3.303.8 make sure Embedded/Script is available, and also template name is from template itself, good for distinguishing between different template choices
    if (this->xEmbedScripts.isEmpty ())
      this->xEmbedScripts = Utils::xml_get_or_create_node_ptr (xRootTemplate, mxconst::get_ELEMENT_EMBEDDED_SCRIPTS ());

    const std::string template_name = Utils::readAttrib (xRootTemplate, mxconst::get_ATTRIB_NAME (), "");
    if (!template_name.empty ())
      this->xDummyTopNode.updateAttribute (template_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());

    const std::string title_text = Utils::readAttrib (xRootTemplate, mxconst::get_ATTRIB_TITLE (), "");
    if (!title_text.empty ())
      this->xDummyTopNode.updateAttribute (title_text.c_str (), mxconst::get_ATTRIB_TITLE ().c_str (), mxconst::get_ATTRIB_TITLE ().c_str ());

    // add all <objective> nodes
    auto vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName (xRootTemplate, mxconst::get_ELEMENT_OBJECTIVES ());
    for (auto &node : vecNodes)
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xObjectives, node, mxconst::get_ELEMENT_OBJECTIVE (), true);


    // add all triggers
    vecNodes.clear ();
    vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName (xRootTemplate, mxconst::get_ELEMENT_TRIGGERS ());
    for (auto &node : vecNodes)
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xTriggers, node, mxconst::get_ELEMENT_TRIGGER (), true);


    // Add <message_templates> // v3.303.8
    vecNodes.clear ();
    vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName (xRootTemplate, mxconst::get_ELEMENT_MESSAGE ());
    for (auto &node : vecNodes)
      Utils::xml_add_node_to_parent_with_duplicate_filter (this->xMessages, node, mxconst::get_ELEMENT_MESSAGE (), mxconst::get_ATTRIB_NAME ());

    // Add <scriptlet> // v3.303.8
    vecNodes.clear ();
    vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName (xRootTemplate, mxconst::get_ELEMENT_SCRIPTLET ());
    for (auto &node : vecNodes)
      Utils::xml_add_node_to_parent_with_duplicate_filter (this->xEmbedScripts, node, mxconst::get_ELEMENT_SCRIPTLET (), mxconst::get_ATTRIB_NAME ());

    // Add GPS
    for (int i1 = 0; i1 < xRootTemplate.nChildNode (mxconst::get_ELEMENT_GPS ().c_str ()); ++i1)
    {
      auto node = xRootTemplate.getChildNode (mxconst::get_ELEMENT_GPS ().c_str (), i1);
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xGPS, node, mxconst::get_ELEMENT_POINT (), true);
    }
    // Add Inventory
    for (int i1 = 0; i1 < xRootTemplate.nChildNode (mxconst::get_ELEMENT_INVENTORIES ().c_str ()); ++i1)
    {
      auto node = xRootTemplate.getChildNode (mxconst::get_ELEMENT_INVENTORIES ().c_str (), i1);
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xInventoris, node, mxconst::get_ELEMENT_INVENTORY (), true);
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xInventoris, node, mxconst::get_ELEMENT_PLANE (), true);
    }
    // add all 3D object to <object_template> // v3.0.303.2 starting from second element since the first element is always this->x3DObjTemplate that way we won't have duplication of <obj3d> elements.
    for (int i1 = 1; i1 < xRootTemplate.nChildNode (mxconst::get_ELEMENT_OBJECT_TEMPLATES ().c_str ()); ++i1)
    {
      auto node = xRootTemplate.getChildNode (mxconst::get_ELEMENT_OBJECT_TEMPLATES ().c_str (), i1);
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->x3DObjTemplate, node, mxconst::get_ELEMENT_OBJ3D (), true);
    }

    // add <choices>
    for (int i1 = 0; i1 < xRootTemplate.nChildNode (mxconst::get_ELEMENT_CHOICES ().c_str ()); ++i1)
    {
      auto node = xRootTemplate.getChildNode (mxconst::get_ELEMENT_CHOICES ().c_str (), i1);
      Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xChoices, node, mxconst::get_ELEMENT_CHOICE (), true);
    }

    // <global_settings>
    this->xGlobalSettings = xRootTemplate.getChildNode (mxconst::get_GLOBAL_SETTINGS ().c_str ()).deepCopy ();
  }

  // Write to file
  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  if (int nFlightLegs = this->xFlightLegs.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ()); nFlightLegs == 0)
  {
    RandomEngine::setError ("[random] No flight leg has been created. Try to re-generate a mission, tweak the template or re-run APT.DAT optimization (setup screen).");
    this->abortThread ();
    return false;
  }
  else
  {
    // v3.0.221.7 set xBriefer starting flight leg
    if (IXMLNode xml_leg_node = this->xFlightLegs.getChildNode (mxconst::get_ELEMENT_LEG ().c_str ())
      ; !xml_leg_node.isEmpty ())
    {
      std::string firstLegName = Utils::readAttrib (xml_leg_node, mxconst::get_ATTRIB_NAME (), "");
      if (firstLegName.empty ())
      {
        RandomEngine::setError ("[random] Fail to find name of starting <leg> Try to re-generate a mission or tweak the template.");
        this->abortThread ();
        return false;
      }

      xBriefer.updateAttribute (firstLegName.c_str (), mxconst::get_ATTRIB_STARTING_LEG ().c_str (), mxconst::get_ATTRIB_STARTING_LEG ().c_str ());
      Log::logDebugBO ("[random before write]Set starting <leg> >> " + firstLegName + " << ", true);
    }
  }

  // store plane type // v24.12.1
  if (!this->xMetadata.isEmpty ())
  {
    // plane type
    xMetadata.updateAttribute (Utils::readAttrib (missionx::RandomEngine::xRootTemplate, mxconst::get_ATTRIB_PLANE_TYPE (), "").c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ());
    // ui layer (code) from where mission was generated
    xMetadata.updateAttribute (mxUtils::formatNumber<int> (static_cast<int> (data_manager::getGeneratedFromLayer ())).c_str (), mxconst::get_ATTRIB_UI_LAYER ().c_str (), mxconst::get_ATTRIB_UI_LAYER ().c_str ());
  }

  result = writeTargetFile ();

  auto endCacheLoad = std::chrono::steady_clock::now ();
  auto diff_cache   = endCacheLoad - startThreadClock;
  duration          = std::chrono::duration<double, std::milli> (diff_cache).count ();
  Log::logAttention ("*** Finished Generating RANDOM Mission, Duration: " + Utils::formatNumber<double> (duration, 3) + "ms (" + Utils::formatNumber<double> ((duration / 1000), 3) + "sec)  ****", true);

  /// finalize thread
  missionx::RandomEngine::threadState.flagIsActive       = false;
  missionx::RandomEngine::threadState.flagThreadDoneWork = true; // we reset the thread at Mission::flc_aptdat() function

  return result;
}

// -----------------------------------

bool
RandomEngine::gen_read_mission_info_element ()
{
  // bool result = true;

  this->xBrieferInfo = xRootTemplate.getChildNode (mxconst::get_ELEMENT_MISSION_INFO ().c_str ()).deepCopy ();
  if (this->xBrieferInfo.isEmpty ())
  {
    RandomEngine::setError ("No <" + mxconst::get_ELEMENT_MISSION_INFO () + "> was found. Template malformed, abort template generation.");
    return false;
  }

  // override mission image file with random.png
  if (missionx::RandomEngine::working_tempFile_ptr->missionFolderName.empty ())
    Utils::xml_set_attribute_in_node_asString (xBrieferInfo, mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME (), mxconst::get_DEFAULT_RANDOM_IMAGE_FILE (), mxconst::get_ELEMENT_MISSION_INFO ()); // v3.0.217.6
  else
  {
    const std::string imageFileName_s = Utils::readAttrib (xBrieferInfo, mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME (), missionx::RandomEngine::working_tempFile_ptr->getTemplateImageFileName ()); // v3.0.241.1
    Utils::xml_set_attribute_in_node_asString (xBrieferInfo, mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME (), imageFileName_s, mxconst::get_ELEMENT_MISSION_INFO ()); // v3.0.217.6
  }

  // add the template file name to other settings
  // v25.02.1
  const std::string template_name = (missionx::RandomEngine::working_tempFile_ptr != nullptr) ? std::filesystem::path (RandomEngine::working_tempFile_ptr->fullFilePath).filename ().string () : "";

  std::string other_settings = Utils::readAttrib (xBrieferInfo, mxconst::get_ATTRIB_OTHER_SETTINGS (), ""); // v3.0.241.1
  other_settings             = "Based on: " + ((!template_name.empty ()) ? template_name : "Error: No Template Data") + ". " + other_settings;
  Utils::xml_set_attribute_in_node_asString (xBrieferInfo, mxconst::get_ATTRIB_OTHER_SETTINGS (), other_settings, mxconst::get_ELEMENT_MISSION_INFO ()); // v3.0.217.6

  RandomEngine::errMsg.clear ();

  // v3.0.255.4.2 parse mission info designer overpass urls that will override the plugin own list of urls
  missionx::RandomEngine::vecMissionInfoOverpassUrls.clear (); // v3.0.255.4.2
  const auto urls_i = this->xBrieferInfo.getChildNode (mxconst::get_ELEMENT_OVERPASS ().c_str ()).nChildNode (mxconst::get_ELEMENT_URL ().c_str ());
  for (int i1 = 0; i1 < urls_i; ++i1)
  {
    if (std::string text = this->xBrieferInfo.getChildNode (mxconst::get_ELEMENT_OVERPASS ().c_str ()).getChildNode (mxconst::get_ELEMENT_URL ().c_str (), i1).getText ()
      ; !text.empty ())
      missionx::RandomEngine::vecMissionInfoOverpassUrls.emplace_back (text);
  }
  missionx::RandomEngine::current_url_indx_used_i = (missionx::RandomEngine::vecMissionInfoOverpassUrls.empty ()) ? mxconst::INT_UNDEFINED : 0;

  return true;
}

// -----------------------------------



bool
RandomEngine::prepareBrieferAndStartLocation ()
{
  // We will use this function to construct the "<briefer>" element.
  // The description of the mission in the briefer we will fetch from: "<mission_info>" CDATA property.
  IXMLNode xPoint = data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_POINT ().c_str ()).deepCopy (); // v3.0.219.7 added for injectInventory() later.
  if (xPoint.isEmpty ())
  {
    xPoint = data_manager::xmlMappingNode.addChild (mxconst::get_ELEMENT_POINT ().c_str ());
    xPoint.addAttribute (mxconst::get_ATTRIB_LAT ().c_str (), "");
    xPoint.addAttribute (mxconst::get_ATTRIB_LONG ().c_str (), "");
  }
  std::string lat_s, lon_s, icao_s; // will hold string representation of longitude and latitude

  bool result = true;

  IXMLNode xLocationAdjust = xRootTemplate.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ().c_str ()).deepCopy ();
  if (xLocationAdjust.isEmpty ())
  {
    RandomEngine::setError ("[random] No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION () + "> was found. Template malformed, abort template generation !!!");

    return false;
  }

  // rename element tag to <location_adjust >
  xLocationAdjust.updateName (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ());

  // store element properties in "elementBrieferInfoProperties" for internal use, if needed to remove any "clear" data
  const int   nClear      = xLocationAdjust.nClear (); // remove any CDATA or COMMENTS or any clear() type element
  // std::string brieferDesc = Utils::xml_read_cdata_node (xLocationAdjust, ""); // v3.0.241.1 // v3.0.241.9 replace default string with empty string
  std::string brieferDesc = Utils::xml_get_text_or_cdata_text (xLocationAdjust, ""); // v3.0.241.1 // v3.0.241.9 replace default string with empty string
  for (int i = 0; i < nClear; ++i)
    xLocationAdjust.deleteClear (); // v3.0.241.1 change from remove "i" to remove first

  //// Handle location_type
  std::string locationOptionType = Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LOCATION_TYPE (), mxconst::get_ELEMENT_PLANE ());
  if (locationOptionType.empty ())
    locationOptionType = mxconst::get_ELEMENT_PLANE ();

  ////////////////////
  // if value = plane
  if (mxconst::get_ELEMENT_PLANE () == locationOptionType || get_user_wants_to_start_from_plane_position ()) // v3.0.253.11 added prop_start_from_plane_position
  {
    // set xPoint from plane // v3.0.219.7
    lat_s = Utils::formatNumber<double> (RandomEngine::planeLocation.getLat (), 8);
    lon_s = Utils::formatNumber<double> (RandomEngine::planeLocation.getLon (), 8);

    xPoint.updateAttribute (lat_s.c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
    xPoint.updateAttribute (lon_s.c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());

    xLocationAdjust.updateAttribute (lat_s.c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
    xLocationAdjust.updateAttribute (lon_s.c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
    xLocationAdjust.updateAttribute (Utils::formatNumber<double> (RandomEngine::planeLocation.getElevationInFeet (), 2).c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str ());
    xLocationAdjust.updateAttribute (Utils::formatNumber<double> (RandomEngine::planeLocation.getHeading (), 2).c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());
  }
  ////////////////////
  // if value = xy
  else if (mxconst::get_EXPECTED_LOCATION_TYPE_XY () == locationOptionType) // if value = xy
  {
    // check it targetLat/long are set, if yes, then use them
    // if not then check if "random" exists and if its value is not empty. then read the element with points and randomly pick a point.
    // read targetLat/long and see if they are pre-defined from template.

    lat_s  = Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LAT (), "");
    lon_s  = Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LONG (), "");
    icao_s = Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_ICAO_ID (), ""); // v3.303.14

    if (!lat_s.empty () && !lon_s.empty ())
    { // we will use current targetLat/long stored in elementStartLocationProperties
      Log::logMsgThread ("[random] will set start location based on pre-defined location provided in template.");

      // set xPoint from plane // v3.0.219.7
      xPoint.updateAttribute (lat_s.c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
      xPoint.updateAttribute (lon_s.c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
      xPoint.updateAttribute (icao_s.c_str (), mxconst::get_ATTRIB_ICAO_ID ().c_str (), mxconst::get_ATTRIB_ICAO_ID ().c_str ());
    }
    else // try to use "location_value_nm_s" property and fetch a point based on a list of points provided ad-hock
    {
      // v25.08.1 support for "location_properties" attribute that will replace "location_value"
      // const std::string location_xy_random_value = (!Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), "").empty())? Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), "") : Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LOCATION_VALUE (), "");
      const std::string location_xy_random_value = Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), mxconst::get_ATTRIB_LOCATION_VALUE (),  "");
      if (location_xy_random_value.empty () || Utils::is_number (location_xy_random_value)) // SHOULD NOT BE EMPTY OR A NUMBER.
      {
        RandomEngine::setError ("[random] Failed to find valid starting location, No Coordinates or string List of random latitude/longitude were provided, will abort template creation. Please fix the template or change the starting location to plane.");
        return false;
      }
      else
      { // read random element

        const IXMLNode xLocationNodePtr = missionx::RandomEngine::xRootTemplate.getChildNode (location_xy_random_value.c_str ());
        if (xLocationNodePtr.isEmpty ())
        {
          RandomEngine::setError ("[random] Failed to read random element: <" + location_xy_random_value + ">. Please fix the template, aborting random creation.");
          return false;
        }

        // v3.0.221.5 call convert <icao> to <points>
        RandomEngine::shared_navaid_info.parentNode_ptr = xLocationNodePtr; // store pointer to XML node
        missionx::data_manager::waitForPluginCallbackJob ( &RandomEngine::threadState, missionx::mx_flc_pre_command::convert_icao_to_xml_point); // will call missionx::flcPRE() and try to convert any <icao name="icao name" /> to <point targetLat="" targetLon="" />
        // end v3.0.221.5 conversion

        // xPoint = Utils::xml_get_node_randomly_by_name_IXMLNode (xLocationNodePtr, mxconst::get_ELEMENT_POINT (), RandomEngine::errMsg);
        xPoint = Utils::xml_get_node_randomly_by_name_IXMLNode (xLocationNodePtr, mxconst::get_ELEMENT_POINT ());
        // if (!RandomEngine::errMsg.empty ())
        if (xPoint.isEmpty ())
        {
          // RandomEngine::setError (RandomEngine::errMsg);
          RandomEngine::setError (fmt::format ("[{}] Could not randomly find element <point> in {} node.", __func__, xLocationNodePtr.getName ()) );
          return false;
        }

        if (!xPoint.isEmpty ())
        {
          missionx::NavAidInfo navAid;
          navAid.node = xPoint.deepCopy ();
          navAid.syncXmlPointToNav ();

          // try to get Navaid information for briefer. If we fail to find information, we ignore and continue with the original xPoint data
          if (missionx::RandomEngine::filterAndPickRampBasedOnPlaneType (navAid, RandomEngine::errMsg, missionx::mxFilterRampType::start_ramp))
          {
            xPoint = navAid.node.deepCopy ();
            if (xPoint.isEmpty ())
            {
              RandomEngine::setError ("[random] Fail to read filtered briefer starting point. Aborting... notify developer");
              return false;
            }
          }
          RandomEngine::errMsg.clear ();

          lat_s                    = Utils::readAttrib (xPoint, mxconst::get_ATTRIB_LAT (), "");
          lon_s                    = Utils::readAttrib (xPoint, mxconst::get_ATTRIB_LONG (), "");
          const std::string elev_s = Utils::readAttrib (xPoint, mxconst::get_ATTRIB_ELEV_FT (), "");

          if (lat_s.empty () || lon_s.empty ())
          {
            RandomEngine::setError ("[random] Point data does not have mandatory attributes: '" + mxconst::get_ATTRIB_LAT () + "' and '" + mxconst::get_ATTRIB_LONG () + "'. Please fix template. Aborting...");
            return false;
          }

          // set start location "targetLat/long/elev_ft
          Utils::xml_search_and_set_attribute_in_IXMLNode (xLocationAdjust, mxconst::get_ATTRIB_LAT (), lat_s, mxconst::get_ELEMENT_LOCATION_ADJUST ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xLocationAdjust, mxconst::get_ATTRIB_LONG (), lon_s, mxconst::get_ELEMENT_LOCATION_ADJUST ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xLocationAdjust, mxconst::get_ATTRIB_ELEV_FT (), elev_s, mxconst::get_ELEMENT_LOCATION_ADJUST ());

        } // end reading a random < point > element

      } // end using <location_value_nm_s> an element to choose a starting location

    } // end if targetLat/long were defined or based on a location_value_nm_s element

  } // end construct <start_location> based on "xy" (pre-defined targetLat/long or based on ad-hock starting points that we will pick at random


  if (lat_s.empty () || lon_s.empty ()) // v3.0.219.6
  {
    RandomEngine::setError ("[random briefer] Something is wrong with location definition, fix template or notify developer if no solution could be found.");
    return false;
  }


  if (!xPoint.isEmpty ())
  {
    RandomEngine::lastFlightLegNavInfo.node = xPoint.deepCopy ();
    RandomEngine::lastFlightLegNavInfo.syncXmlPointToNav ();
    RandomEngine::lastFlightLegNavInfo.flightLegName = mxconst::get_ELEMENT_BRIEFER ();

    // search for the nearest ICAO or bounding airport relative to plane starting position using the SQLITE database
    RandomEngine::shared_navaid_info.navAid.init ();

    RandomEngine::shared_navaid_info.navAid = missionx::data_manager::getPlaneAirportOrNearestICAO (true, lastFlightLegNavInfo.lat, lastFlightLegNavInfo.lon, true); // v3.303.14
    if (!RandomEngine::shared_navaid_info.navAid.getID ().empty ())
    {
      // if plane is in bounding area, then we are good
      lastFlightLegNavInfo.setName (RandomEngine::shared_navaid_info.navAid.getNavAidName ());
      lastFlightLegNavInfo.setID (RandomEngine::shared_navaid_info.navAid.getID ());
      lastFlightLegNavInfo.height_mt = RandomEngine::shared_navaid_info.navAid.height_mt;
      lastFlightLegNavInfo.navRef    = RandomEngine::shared_navaid_info.navAid.navRef; // v25.05.1

      lastFlightLegNavInfo.synchToPoint ();
      xPoint = lastFlightLegNavInfo.node.deepCopy (); // override xPoint with the added information
    }
    else
    { // try to pick the nearest airport using the "XPLMFindNavAid()" function.

      RandomEngine::shared_navaid_info.navAid.init ();
      // v3.0.221.11 try to get airport information
      if (missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_randomLastFlightLeg_mainThread))
      {
        // check distance and hopefully pick correct airport. Since we are using a fixed distance this might not be a 100% guaranty
        RandomEngine::shared_navaid_info.navAid.synchToPoint ();
        if (double dist = missionx::Point::calcDistanceBetween2Points (RandomEngine::shared_navaid_info.navAid.p, lastFlightLegNavInfo.p); dist <= 1.2 && !RandomEngine::shared_navaid_info.navAid.getID ().empty ())
        {
          lastFlightLegNavInfo.setName (RandomEngine::shared_navaid_info.navAid.getNavAidName ());
          lastFlightLegNavInfo.setID (RandomEngine::shared_navaid_info.navAid.getID ());
          lastFlightLegNavInfo.height_mt = RandomEngine::shared_navaid_info.navAid.height_mt;
          lastFlightLegNavInfo.navRef    = RandomEngine::shared_navaid_info.navAid.navRef; // v25.05.1

          lastFlightLegNavInfo.synchToPoint ();
        }

        xPoint = lastFlightLegNavInfo.node.deepCopy (); // store in xPoint again
      }
      else
      {
        Log::logMsgWarn ("[random briefer] Failed to find Airport NEAR given start location. ", true);
      }
    }
    lastFlightLegNavInfo.flag_is_brieferOrStartLocation = true; // v3.303.10
    lastFlightLegNavInfo.synchToPoint ();

    missionx::RandomEngine::listNavInfo.emplace_back (lastFlightLegNavInfo); // add NavInfo into a list

    // Add to GPS
    if (!xGPS.isEmpty ())
      xGPS.addChild (xPoint.deepCopy ());
  }
  else
  {
    RandomEngine::setError ("[random briefer] Something is wrong with location definition, fix template or notify developer if no solution could be found.");
    return false;
  }

  // create most of the <briefer> node. we will finish it once we will have all Flight Legs
  this->xBriefer = this->xDummyTopNode.addChild (mxconst::get_ELEMENT_BRIEFER ().c_str ());
  this->xBriefer.addAttribute (mxconst::get_ATTRIB_STARTING_LEG ().c_str (), "leg_1"); // leg_1 is default value, but it can be changed when using <content> elements with "Flight Leg sets"

  // v25.05.1 Better ICAO representation in the "briefer" element
  if (lastFlightLegNavInfo.getID ().empty ())
    this->xBriefer.updateAttribute (Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_STARTING_ICAO (), "").c_str (), mxconst::get_ELEMENT_ICAO ().c_str (), mxconst::get_ELEMENT_ICAO ().c_str ()); // v3.303.8.2 copy starting_icao to briefer element
  else
    this->xBriefer.updateAttribute (lastFlightLegNavInfo.getID ().c_str (), mxconst::get_ELEMENT_ICAO ().c_str (), mxconst::get_ELEMENT_ICAO ().c_str ()); // v3.303.8.2 copy starting_icao to briefer element

  this->xBriefer.updateAttribute (Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_POSITION_PREF (), "").c_str (), mxconst::get_ATTRIB_POSITION_PREF ().c_str (), mxconst::get_ATTRIB_POSITION_PREF ().c_str ()); // v3.0.301 B4 Copy the attribute value from <briefer_and_start_location> - position_pref

  // v25.05.1 prepare starting location description
  if (const auto starting_icao = Utils::readAttrib (this->xBriefer, mxconst::get_ELEMENT_ICAO (), "");
    !starting_icao.empty ())
      this->briefer_starting_location_desc = fmt::format ("You will fly from {}({}).", lastFlightLegNavInfo.getNavAidName (), starting_icao);
  else
    this->briefer_starting_location_desc.clear ();


  IXMLNode cNode = xBriefer.addChild (xLocationAdjust); // add <location_adjust> to briefer
  Utils::xml_add_cdata (this->xBriefer, brieferDesc); // v3.0.241.1

  // v3.0.219.7 Add inventory if exists in mapping
  if (data_manager::xmlMappingNode.nChildNode (mxconst::get_ELEMENT_INVENTORY ().c_str ()) > 0)
  {
    this->addInventory (mxconst::get_ELEMENT_BRIEFER (), xPoint, mxInvSource::point); // name of store located at the start location
  }


  return result;
}


// -----------------------------------
// -----------------------------------
// -----------------------------------

IXMLNode
RandomEngine::get_skewed_target_position (const IXMLNode &inRealTargetPositionPoint)
{
  double   outLat, outLon = 0.0; // result from calculation
  IXMLNode skewedPosition = inRealTargetPositionPoint.deepCopy ();

  constexpr double MIN_AWAY_DISTANCE = 0.1;
  constexpr double MAX_AWAY_DISTANCE = mxconst::MAX_AWAY_SKEWED_DISTANCE_NM; // 1.0; // v3.0.253.12
  constexpr double MIN_DEGREES       = 1.0;
  constexpr double MAX_DEGREES       = 359.0;

  const double lat       = Utils::readNumericAttrib (skewedPosition, mxconst::get_ATTRIB_LAT (), 0.0);
  const double lon       = Utils::readNumericAttrib (skewedPosition, mxconst::get_ATTRIB_LONG (), 0.0);
  const auto   dist_d    = Utils::getRandomRealNumber (MIN_AWAY_DISTANCE, MAX_AWAY_DISTANCE);
  const auto   bearing_f = static_cast<float> (Utils::getRandomRealNumber (MIN_DEGREES, MAX_DEGREES));
  // calculate new location
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (outLat, outLon, lat, lon, bearing_f, dist_d);

  Utils::xml_set_attribute_in_node<double> (skewedPosition, mxconst::get_ATTRIB_LAT (), outLat, skewedPosition.getName ());
  Utils::xml_set_attribute_in_node<double> (skewedPosition, mxconst::get_ATTRIB_LONG (), outLon, skewedPosition.getName ());
  Utils::xml_set_attribute_in_node<bool> (skewedPosition, mxconst::get_ATTRIB_IS_SKEWED_POSITION_B (), true, skewedPosition.getName ());

  const std::string real_pos = "lat=" + Utils::readAttrib (inRealTargetPositionPoint, mxconst::get_ATTRIB_LAT (), "") + " lon=" + Utils::readAttrib (inRealTargetPositionPoint, mxconst::get_ATTRIB_LONG (), "");
  Utils::xml_set_attribute_in_node_asString (skewedPosition, mxconst::get_ATTRIB_REAL_POSITION (), real_pos, skewedPosition.getName ());


  return skewedPosition.deepCopy ();
}


// -----------------------------------


bool
RandomEngine::parse_display_object_element (IXMLNode &inFlightLegNode, IXMLNode &inDisplayNode, IXMLNode & in_xRootTemplate, IXMLNode & x3DObjTemplate, double &expected_slope_at_target_location_d, std::string & inout_err)
{
  // 1. Check if <display_object> tag has random_object attribute. If so it will pick one and add to the <object_templates>
  // 2. If random element is not valid, or we failed to find then use: name="".
  // 3. if no name has been provided then return "false", node is not valid.
  // We will use:  xDummyTopNode and x3DObjTemplate (holds pre-defined objects)

  inout_err.clear (); // v25.06.1
  const std::string TAG_NAME = inDisplayNode.getName ();

  bool flag_foundValidRandomNode = false;

  // v3.0.219.10 read information regarding flight leg in water.
  bool flag_isFlightLegInWater = Utils::readBoolAttrib (inFlightLegNode, mxconst::get_PROP_IS_WET (), false);
  // std::string attribIsLegInWater     = Utils::xml_get_attribute_value(inFlightLegNode, mxconst::get_PROP_IS_WET(), flag_found);
  // if (!attribIsLegInWater.empty() && mxconst::get_MX_TRUE().compare(attribIsLegInWater) == 0)
  //   flag_isFlightLegInWater = true;

  std::string name                   = Utils::readAttrib (inDisplayNode, mxconst::get_ATTRIB_NAME (), "");
  std::string randomTag              = Utils::readAttrib (inDisplayNode, mxconst::get_ATTRIB_RANDOM_TAG (), "");
  std::string file_name              = Utils::readAttrib (inDisplayNode, mxconst::get_ATTRIB_FILE_NAME (), "");
  std::string randomWaterTag         = Utils::readAttrib (inDisplayNode, mxconst::get_ATTRIB_RANDOM_WATER_TAG (), "");
  std::string optional_attrib        = Utils::readAttrib (inDisplayNode, mxconst::get_ATTRIB_OPTIONAL (), EMPTY_STRING);
  auto        limit_to_terrain_slope = Utils::readNodeNumericAttrib<float> (inDisplayNode, mxconst::get_ATTRIB_LIMIT_TO_TERRAIN_SLOPE (), 100.0f);


  // check if optional was defined and skip object if was not random picked
  optional_attrib = Utils::replaceChar1WithChar2_v2 (optional_attrib, '%', ""); // v3.0.219.12+ removes any % from string before handling it
  if (!optional_attrib.empty () && Utils::is_digits (optional_attrib))
  {
    int percent = Utils::stringToNumber<int> (optional_attrib);
    if (percent < 0)
      percent = 1;
    if (percent > 100)
      percent = 99;

    int result = Utils::getRandomIntNumber (0, 100);
    if (result > percent) // meaning missed
      return false;
  }


  // check if object creation is limited by terrain slope
  if (limit_to_terrain_slope < 100 && expected_slope_at_target_location_d > limit_to_terrain_slope) // v3.0.219.12+
  {
    Log::logMsg ("3D Object: " + name + ", rejected due to terrain slope.", true); // v3.0.219.12+
    return false;
  }


  if (flag_isFlightLegInWater && !randomWaterTag.empty ()) // v3.0.219.10 switch between terrain random object and water tag object
  {
    #ifndef RELEASE
    Log::logMsgThread ("[parse display object] Replaced randomTag with the water Tag: " + randomWaterTag + ", for display object name: " + name);
    #endif // !RELEASE

    randomTag = randomWaterTag;
  }


  if (!randomTag.empty ())
  {
    IXMLNode tagNode = Utils::xml_get_node_from_node_tree_IXMLNode (in_xRootTemplate, randomTag, false);
    if (!tagNode.isEmpty ())
    {
      // IXMLNode rNode = Utils::xml_get_node_randomly_by_name_IXMLNode (tagNode, mxconst::get_ELEMENT_OBJ3D (), RandomEngine::errMsg, false); // return deepCopy
      IXMLNode rNode = Utils::xml_get_node_randomly_by_name_IXMLNode (tagNode, mxconst::get_ELEMENT_OBJ3D (), false); // return deepCopy
      // RandomEngine::errMsg.clear ();

      // check if rNode already exists
      std::string rNodeAttribName     = Utils::readAttrib (rNode, mxconst::get_ATTRIB_NAME (), "");
      std::string rNodeAttribFileName = Utils::readAttrib (rNode, mxconst::get_ATTRIB_FILE_NAME (), "");
      if (!rNodeAttribName.empty () && !rNodeAttribFileName.empty ())
      {
        // set <display_object> name attribute with the random name
        Utils::xml_search_and_set_attribute_in_IXMLNode (inDisplayNode, mxconst::get_ATTRIB_FILE_NAME (), rNodeAttribFileName, TAG_NAME);
        flag_foundValidRandomNode = true;

        // check 3D Object is also in <object_template>
        bool     flag_objWithSameName_found_in_objectTemplate = false;
        IXMLNode xObj3d_same_name_in_template                 = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (x3DObjTemplate, mxconst::get_ELEMENT_OBJ3D (), mxconst::get_ATTRIB_NAME (), rNodeAttribName, false);
        flag_objWithSameName_found_in_objectTemplate          = (xObj3d_same_name_in_template.isEmpty ()) ? false : true;

        std::string newName = rNodeAttribName;

        IXMLNode xObj3d_with_same_filename_in_objectTemplate_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (x3DObjTemplate, mxconst::get_ELEMENT_OBJ3D (), mxconst::get_ATTRIB_FILE_NAME (), rNodeAttribFileName, false);

        // if we fail to find a template object with the same file name, we should check:
        // if random node mxconst::get_ATTRIB_NAME() was found in object_template
        // if not then we can just add the 3d object with the random element mxconst::get_ATTRIB_NAME()
        // but if we found an object with same name as the random element, then we should construct a new name using: "display_name + random_attrib_name + random real number and also set it to our "display_object" element
        if (xObj3d_with_same_filename_in_objectTemplate_ptr.isEmpty ()) // if no object with such name was found
        {

          if (flag_objWithSameName_found_in_objectTemplate)
          {
            newName = name + "_" + rNodeAttribName + Utils::formatNumber<double> (Utils::getRandomRealNumber (1.0, 10.0), 4);
            rNode.updateAttribute (newName.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
          }

          x3DObjTemplate.addChild (rNode);
          Utils::xml_search_and_set_attribute_in_IXMLNode (inDisplayNode, mxconst::get_ATTRIB_NAME (), newName, TAG_NAME); // we want the display_object name attribute to be the same as the one in the object_template name
        }
        else // found object with same file name in the object_template
        {
          newName = xObj3d_with_same_filename_in_objectTemplate_ptr.getAttribute (mxconst::get_ATTRIB_NAME ().c_str ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (inDisplayNode, mxconst::get_ATTRIB_NAME (), newName, TAG_NAME); //
        }

        return true; // finish random pick
      }
    }
  }
  else if (!file_name.empty () && !name.empty ()) // v3.0.255.1
  {
    std::string leg_name        = Utils::readAttrib (inFlightLegNode, mxconst::get_ATTRIB_NAME (), "dummy_");
    std::string new_object_name = (name.empty ()) ? leg_name + Utils::formatNumber<double> (Utils::getRandomRealNumber (1.0, 10.0), 4) : name;

    // Create temporary <obj3d> node.
    IXMLNode new_obj3d_p = inDisplayNode.addChild (mxconst::get_ELEMENT_OBJ3D ().c_str ()); //  inDisplayNode.deepCopy();

    std::set<std::string> setAttribToCopy = { mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_FILE_NAME (), mxconst::get_ATTRIB_IS_VIRTUAL_B () }; // v3.0.255.4 added ATTRIB_IS_VIRTUAL_B
    Utils::xml_copy_specific_attributes_using_white_list (inDisplayNode, new_obj3d_p, &setAttribToCopy);

    // search for 3D Object with same name
    IXMLNode xObj3d_same_name_in_objectTemplate = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (x3DObjTemplate, mxconst::get_ELEMENT_OBJ3D (), mxconst::get_ATTRIB_NAME (), new_object_name, false);
    if (xObj3d_same_name_in_objectTemplate.isEmpty ())
    {
      new_obj3d_p.updateAttribute (new_object_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
      x3DObjTemplate.addChild (new_obj3d_p.deepCopy ());
    }
    inDisplayNode.updateAttribute (new_object_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());

    new_obj3d_p.deleteNodeContent ();
  }


  // Check if the original name was set or not (validation)
  if (!flag_foundValidRandomNode)
  {
    if (name.empty ())
    {
      inout_err = "[random parse display] Found <display_element> with no attribute name or random_tag.";
      return false;
    }
  }


  return true;
}

// -----------------------------------

void
RandomEngine::parse_3D_object_template_element (IXMLNode &in_root_template_node, IXMLNode &in_3d_obj_template_node, std::string &inout_errMsg)
{
  // 1. Check if <object_template> tag has random_object attribute.
  // 2. If random element has fileName we will use it
  // 3. if no name has been provided then return "false", node is not valid.
  // We will use:  xDummyTopNode and x3DObjTemplate (holds pre-defined objects)

  const int nChilds = in_3d_obj_template_node.nChildNode (mxconst::get_ELEMENT_OBJ3D ().c_str ());
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    IXMLNode xObj3d_node_ptr = in_3d_obj_template_node.getChildNode (mxconst::get_ELEMENT_OBJ3D ().c_str (), i1);

    std::string name           = Utils::readAttrib (xObj3d_node_ptr, mxconst::get_ATTRIB_NAME (), "");
    std::string randomTag      = Utils::readAttrib (xObj3d_node_ptr, mxconst::get_ATTRIB_RANDOM_TAG (), "");
    std::string randomWaterTag = Utils::readAttrib (xObj3d_node_ptr, mxconst::get_ATTRIB_RANDOM_WATER_TAG (), "");

    if (!randomTag.empty ())
    {
      IXMLNode tagNode = Utils::xml_get_node_from_node_tree_IXMLNode (in_root_template_node, randomTag, false);
      if (!tagNode.isEmpty ())
      {
        // IXMLNode rNode = Utils::xml_get_node_randomly_by_name_IXMLNode (tagNode, mxconst::get_ELEMENT_OBJ3D (), inout_errMsg, false); // return deepCopy
        IXMLNode rNode = Utils::xml_get_node_randomly_by_name_IXMLNode (tagNode, mxconst::get_ELEMENT_OBJ3D (), false); // return deepCopy
        // RandomEngine::errMsg.clear ();
        inout_errMsg.clear();

        if (rNode.isEmpty ())
          continue;

        // replace file name with the random one
        std::string rNodeFileName = Utils::readAttrib (rNode, mxconst::get_ATTRIB_FILE_NAME (), "");
        if (!rNodeFileName.empty ())
        {
          // set <display_object> name attribute with the random name
          Utils::xml_search_and_set_attribute_in_IXMLNode (xObj3d_node_ptr, mxconst::get_ATTRIB_FILE_NAME (), rNodeFileName, mxconst::get_ELEMENT_OBJ3D ());
        }
      }
    } // end randomTag

  } // end loop

} // parse_object_template

// -----------------------------------

bool
RandomEngine::readFlightLegs_directlyFromTemplate ()
{

  Log::logDebugBO ("[DEBUG random airport] start readFlightLegsDirectlyFromTemplate()", true);


  //  bool flag_found = false;
  mapFlightPlanOrder_si.clear ();
  mapFLightPlanOrder_is.clear ();


  // Loop over each FlightLeg element and according to its "type" and "expected_location.locationOptionType" attribute we will construct a Leg
  bool result                = true;
  this->flag_isLastFlightLeg = false;

  int nChilds              = xRootTemplate.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());
  int flightLegs_counter_i = 1;
  for (int i1 = 0; i1 < nChilds && !(missionx::RandomEngine::threadState.flagAbortThread); ++i1)
  {
    // set the most basic data for the <leg>. The rest of the <leg> element will be added later
    IXMLNode xFlightLegFromTemplate = xRootTemplate.getChildNode (mxconst::get_ELEMENT_LEG ().c_str (), i1);

    // v3.0.219.11
    if (i1 == (nChilds - 1))
    { // v3.0.251 b2: enhance condition, add user creation legs rules. User can ask for just 1 leg, hence it should not be considered as last
      if (this->flag_rules_defined_by_user_ui && Utils::readNodeNumericAttrib<int> (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_NO_OF_LEGS (), 0) == 1) // number of childs start from 0 and not 1
        this->flag_isLastFlightLeg = false;
      else
        this->flag_isLastFlightLeg = true;
    }

    Log::logDebugBO ("[DEBUG readFlightLegs_directlyFromTemplate] Build <leg> template: " + Utils::readAttrib (xFlightLegFromTemplate, mxconst::get_ATTRIB_NAME (), ""), true);

    // v3.0.219.2
    expected_slope_at_target_location_d = 0.0; // v3.0.219.12+
    // ----------------------------
    // Build the main flight Leg
    // ----------------------------
    IXMLNode xNewFlightLeg = this->buildFlightLeg (flightLegs_counter_i, xFlightLegFromTemplate);

    // skip if Leg node was not valid
    if (xNewFlightLeg.isEmpty ())
      continue;

    this->mission_xml_data.currentLegName = Utils::readAttrib (xNewFlightLeg, mxconst::get_ATTRIB_NAME (), EMPTY_STRING);

    ///////////////////////////////
    /////////////////////////////
    // Set Flight Leg
    /////////////////////////////

    Utils::xml_add_node_to_element_IXMLNode (xFlightLegs, xNewFlightLeg);
    Utils::addElementToMap (mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, flightLegs_counter_i);
    Utils::addElementToMap (mapFLightPlanOrder_is, flightLegs_counter_i, this->mission_xml_data.currentLegName);

    ++flightLegs_counter_i;

    Utils::add_xml_comment (xFlightLegs, " [[[[ ]]]] "); // add comment between 2 Flight Legs

  } // end loop over <leg> template elements


  if (missionx::RandomEngine::threadState.flagAbortThread) // v3.0.219.12+
  {
    RandomEngine::setError ("[Random] Aborted !!!");
    return false;
  }

  this->fill_up_next_leg_attrib_after_flight_plan_was_generated ();

  return result;
}

// -----------------------------------

IXMLNode
RandomEngine::get_content_story (const IXMLNode &xTemplateNode)
{
  IXMLNode cNode;

  if (xTemplateNode.isEmpty ())
  {
    RandomEngine::setError ("[random get content] Node template is empty. Fix template or notify developer. Skipping..");
    return cNode;
  }

  IXMLNode xContent = xTemplateNode.getChildNode (mxconst::get_ELEMENT_CONTENT ().c_str ());
  if (xContent.isEmpty ())
    return cNode;

  // v3.0.221.15
  // get all <content child tags and pick one
  if (int nChilds = xContent.nChildNode (); nChilds > 0)
  {

    const int         randomSubElement_i = Utils::getRandomIntNumber (0, nChilds - 1);
    const std::string randomTag          = xContent.getChildNode (randomSubElement_i).getName (); // get tag name

    /// pick one of the sub content based on inTemplateType string
    // cNode = Utils::xml_get_node_randomly_by_name_IXMLNode (xContent, randomTag, RandomEngine::errMsg, false);
    cNode = Utils::xml_get_node_randomly_by_name_IXMLNode (xContent, randomTag, false);
    RandomEngine::errMsg.clear ();
  }

  return cNode;
}

// -----------------------------------

bool
RandomEngine::extract_flight_leg_set (IXMLNode &inNodeTemplate, const IXMLNode &inSetNode, int &inCounter)
{
  // Read a XML element that should define a Flight Leg set
  const bool flag_copy_leg_as_is_b             = Utils::readBoolAttrib (inSetNode, mxconst::get_ATTRIB_COPY_LEG_AS_IS_B (), false); // v3.0.303
  bool       flag_needToRaiseLastFlightLegFlag = false; // only when this->flag_isLastFlightLeg was true when calling this function do we need to raise the flag when reaching last <leg> element in loop.
  const int  nChilds                           = inSetNode.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());

  if (this->flag_isLastFlightLeg && nChilds > 1)
  {
    this->flag_isLastFlightLeg        = false;
    flag_needToRaiseLastFlightLegFlag = true;
  }

  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    if (flag_needToRaiseLastFlightLegFlag && i1 == (nChilds - 1))
      this->flag_isLastFlightLeg = true;

    IXMLNode xLeg = inSetNode.getChildNode (mxconst::get_ELEMENT_LEG ().c_str (), i1);
    if (xLeg.isEmpty ())
      continue;

    IXMLNode xNewFlightLegNode = (flag_copy_leg_as_is_b) ? xLeg.deepCopy () : this->buildFlightLeg (inCounter, xLeg); // v3.0.303 added the support of attrib flag_copy_leg_as_is_b
    if (xNewFlightLegNode.isEmpty ())
    {
      if (RandomEngine::errMsg.empty ())
        RandomEngine::errMsg = "[random extract set]Fail to generate flight leg from set. Check log for errors and maybe fix template or notify developer. Skipping...";

      RandomEngine::setError (RandomEngine::errMsg); // this will also print the error message
      continue; // skip Leg. In some cases false outcome can occur due to "optional" attribute or other reason that does not need to break the mission build.
    }

    this->mission_xml_data.currentLegName = Utils::readAttrib (xNewFlightLegNode, mxconst::get_ATTRIB_NAME (), "");

    Utils::xml_add_node_to_element_IXMLNode (this->xFlightLegs, xNewFlightLegNode);
    Utils::addElementToMap (mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, inCounter);
    Utils::addElementToMap (mapFLightPlanOrder_is, inCounter, this->mission_xml_data.currentLegName);

    Log::logMsgThread ("\n>> Added Flight Leg: " + this->mission_xml_data.currentLegName + "\n"); // v3.303.11 debug

    ++inCounter;

    Utils::add_xml_comment (this->xFlightLegs, " /////// "); // add comment between 2 <leg> nodes
  }

  // v3.0.303 if flag_copy_leg_as_is_b than add objectives and triggers
  if (flag_copy_leg_as_is_b)
    inNodeTemplate.updateAttribute ("true", mxconst::get_ATTRIB_COPY_LEG_AS_IS_B ().c_str (), mxconst::get_ATTRIB_COPY_LEG_AS_IS_B ().c_str ());


  return true;
}

// -----------------------------------

bool
RandomEngine::build_and_add_flight_leg_from_node (const IXMLNode &inNode, int &inCounter)
{
  IXMLNode xNewFLightLegNode = this->buildFlightLeg (inCounter, inNode);
  // skip if flight leg node is not valid
  if (xNewFLightLegNode.isEmpty ())
  {
    if (RandomEngine::errMsg.empty ())
      RandomEngine::errMsg = "[random build_and_add_flight_leg_from_node] Fail to generate flight leg, this might be ok. Check log for errors/warning, skipping...";

    RandomEngine::setError (RandomEngine::errMsg);
    return true;
  }

  this->mission_xml_data.currentLegName = Utils::readAttrib (xNewFLightLegNode, mxconst::get_ATTRIB_NAME (), "");

  Utils::xml_add_node_to_element_IXMLNode (xFlightLegs, xNewFLightLegNode);
  Utils::addElementToMap (mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, inCounter);
  Utils::addElementToMap (mapFLightPlanOrder_is, inCounter, this->mission_xml_data.currentLegName);

  ++inCounter;

  Utils::add_xml_comment (xFlightLegs, " [[[[ ]]]] "); // add comment between 2 flight legs
  return true;
}

// -----------------------------------

missionx::NavAidInfo
RandomEngine::gen_parse_briefer_and_start_location (IXMLNode &xLocationAdjust)
{
  missionx::NavAidInfo navAid;
  std::string          lat_s, lon_s; // will hold string representation of longitude and latitude

  if (xLocationAdjust.isEmpty ())
  {
    // TODO: This code should move to the calling function
    // RandomEngine::setError ("[random] No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION () + "> was found. Template malformed, abort template generation !!!");
    navAid.err = fmt::format("[{}] No <{}>> was found. Template malformed, abort template generation !!!", __func__, mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ());

    return navAid;
  }

  // rename element tag to <location_adjust >
  xLocationAdjust.updateName (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ());

  // store element properties in "elementBrieferInfoProperties" for internal use, if needed to remove any "clear" data
  const int   nClear      = xLocationAdjust.nClear (); // remove any CDATA or COMMENTS or any clear() type element
  // std::string brieferDesc = Utils::xml_read_cdata_node (xLocationAdjust, ""); // v3.0.241.1 // v3.0.241.9 replace default string with empty string
  navAid.fpln_expected_location_data.desc = Utils::xml_get_text_or_cdata_text (xLocationAdjust, ""); // v3.0.241.1 // v3.0.241.9 replace default string with empty string
  for (int i = 0; i < nClear; ++i)
    xLocationAdjust.deleteClear (); // v3.0.241.1 change from remove "i" to remove first

  //// Handle location_type
  const std::string locationOptionType = Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LOCATION_TYPE (), mxconst::get_ELEMENT_PLANE ());

  ////////////////////
  // if value = plane
  if (mxconst::get_ELEMENT_PLANE () == locationOptionType || get_user_wants_to_start_from_plane_position ()) // v3.0.253.11 added prop_start_from_plane_position
  {
    // v25.09.2
    navAid.lat = static_cast<float>(RandomEngine::planeLocation.lat);
    navAid.lon = static_cast<float>(RandomEngine::planeLocation.lon);

    // set xPoint from plane
    lat_s = Utils::formatNumber<double> (RandomEngine::planeLocation.getLat (), 8);
    lon_s = Utils::formatNumber<double> (RandomEngine::planeLocation.getLon (), 8);


    xLocationAdjust.updateAttribute (lat_s.c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
    xLocationAdjust.updateAttribute (lon_s.c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
    xLocationAdjust.updateAttribute (Utils::formatNumber<double> (RandomEngine::planeLocation.getElevationInFeet (), 2).c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str ());
    xLocationAdjust.updateAttribute (Utils::formatNumber<double> (RandomEngine::planeLocation.getHeading (), 2).c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());
  }
  ////////////////////
  // if value = xy
  else if (mxconst::get_EXPECTED_LOCATION_TYPE_XY () == locationOptionType) // if value = xy
  {
    // check it targetLat/long are set, if yes, then use them
    // if not then check if "random" exists and if its value is not empty. then read the element with points and randomly pick a point.
    // read targetLat/long and see if they are pre-defined from the template.

    navAid.lat = Utils::readNodeNumericAttrib<float> (xLocationAdjust, mxconst::get_ATTRIB_LAT (), 0.0f);
    navAid.lon = Utils::readNodeNumericAttrib<float> (xLocationAdjust, mxconst::get_ATTRIB_LONG (), 0.0f);

    const std::string icao_s = Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_ICAO_ID (), "");

    // if (!lat_s.empty () && !lon_s.empty ())
    if (navAid.is_lat_lon_valid ())
    { // we will use the current targetLat/long stored in elementStartLocationProperties
      navAid.setID (icao_s);
      Log::logMsgThread (fmt::format("[{}] will set start location based on pre-defined location provided in template.", __func__) );
    }
    else // try to use the "location_value_nm_s" property and fetch a point based on a list of points provided ad-hock
    {
      // v25.08.1 support for "location_properties" attribute that will replace "location_value"
      // const std::string location_xy_random_value = (!Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), "").empty())? Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), "") : Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LOCATION_VALUE (), "");
      const std::string location_xy_random_value = Utils::readAttrib (xLocationAdjust, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), mxconst::get_ATTRIB_LOCATION_VALUE (),  "");
      if (location_xy_random_value.empty () || mxUtils::is_number (location_xy_random_value)) // SHOULD NOT BE EMPTY OR A NUMBER.
      {
        // TODO: handle error in the calling function
        // RandomEngine::setError ("[random] Failed to find valid starting location, No Coordinates or string List of random latitude/longitude were provided, will abort template creation. Please fix the template or change the starting location to plane.");
        navAid.init ();
        navAid.err = fmt::format ("[{}:{}] Failed to find valid starting location, No Coordinates or string List of random latitude/longitude were provided, will abort template creation. Please fix the template or change the starting location to plane.", __func__, __LINE__);
        return navAid;
      }

      // read random element
      const IXMLNode xLocationNodePtr = missionx::RandomEngine::xRootTemplate.getChildNode (location_xy_random_value.c_str ());
      if (xLocationNodePtr.isEmpty ())
      {
        // RandomEngine::setError ("[random] Failed to read random element: <" + location_xy_random_value + ">. Please fix the template, aborting random creation.");
        navAid.init ();
        navAid.err = fmt::format ("[{}] Failed to read random element: <{}>. Please fix the template, aborting random creation.", __func__, location_xy_random_value);
        return navAid;
      }

      RandomEngine::shared_navaid_info.parentNode_ptr = xLocationNodePtr; // store pointer to XML node
      missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::convert_icao_to_xml_point); // will call missionx::flcPRE() and try to convert any <icao name="icao name" /> to <point targetLat="" targetLon="" />
      // end v3.0.221.5 conversion

      IXMLNode xPoint = Utils::xml_get_node_randomly_by_name_IXMLNode (xLocationNodePtr, mxconst::get_ELEMENT_POINT ());
      if (xPoint.isEmpty ())
      {
        // RandomEngine::setError (fmt::format ("[{}] Could not randomly find element <point> in {} node.", __func__, xLocationNodePtr.getName ()));
        navAid.init ();
        navAid.err = fmt::format ("[{}] Could not randomly find element <point> in {} node.", __func__, xLocationNodePtr.getName ());
        return navAid;
      }

      navAid.node = xPoint.deepCopy ();
      navAid.syncXmlPointToNav ();

      // try to get Navaid information for briefer. If we fail to find information, we ignore and continue with the original xPoint data
      if (missionx::RandomEngine::filterAndPickRampBasedOnPlaneType (navAid, navAid.err, missionx::mxFilterRampType::start_ramp))
      {
        xPoint = navAid.node.deepCopy ();
        if (xPoint.isEmpty () || !navAid.err.empty () )
        {
          // RandomEngine::setError ("[random] Fail to read filtered briefer starting point. Aborting... notify developer");
          navAid.init ();
          navAid.err = fmt::format ("[{}] Fail to read filtered briefer starting point. Aborting... notify developer.", __func__);
          return navAid;
        }
      }
      RandomEngine::errMsg.clear ();

      const std::string elev_s = Utils::readAttrib (xPoint, mxconst::get_ATTRIB_ELEV_FT (), "");

      navAid.lat                    = Utils::readNodeNumericAttrib <float>(xPoint, mxconst::get_ATTRIB_LAT (), 0.0f);
      navAid.lon                    = Utils::readNodeNumericAttrib <float>(xPoint, mxconst::get_ATTRIB_LONG (), 0.0f);
      navAid.height_mt              = Utils::readNodeNumericAttrib <float>(xPoint, mxconst::get_ATTRIB_ELEV_FT (), 0.0f) * feet2meter;

      if (!navAid.is_lat_lon_valid ())
      {
        // RandomEngine::setError ("[random] Point data does not have mandatory attributes: '" + mxconst::get_ATTRIB_LAT () + "' and '" + mxconst::get_ATTRIB_LONG () + "'. Please fix template. Aborting...");
        navAid.init ();
        navAid.err = fmt::format ("[{}] Point data does not have mandatory attributes, check 'lat' and 'lon' attributes.", __func__);
        return navAid;
      }

      // set start location "targetLat/long/elev_ft
      Utils::xml_search_and_set_attribute_in_IXMLNode (xLocationAdjust, mxconst::get_ATTRIB_LAT (), navAid.getLat (), mxconst::get_ELEMENT_LOCATION_ADJUST ());
      Utils::xml_search_and_set_attribute_in_IXMLNode (xLocationAdjust, mxconst::get_ATTRIB_LONG (), navAid.getLon (), mxconst::get_ELEMENT_LOCATION_ADJUST ());
      Utils::xml_search_and_set_attribute_in_IXMLNode (xLocationAdjust, mxconst::get_ATTRIB_ELEV_FT (), elev_s, mxconst::get_ELEMENT_LOCATION_ADJUST ());

      // end reading a random < point > element
      // end using <location_value_nm_s> an element to choose a starting location

    } // end if targetLat/long were defined or based on a location_value_nm_s element

  } // end construct <start_location> based on "xy" (pre-defined targetLat/long or based on ad-hock starting points that we will pick at random

  return navAid; // TODO: later we need to set the Briefer description and properties
}

// -----------------------------------

missionx::NavAidInfo
RandomEngine::gen_parse_template_leg (missionx::base_thread::thread_state *inoutThreadState, const IXMLNode& xTemplateNode
                                  , const IXMLNode &xml_leg_node_from_template, random_airport_info_struct &inout_shared_navaid
                                  , std::map<int, missionx::NavAidInfo> &in_mission_targets, int &in_leg_counter, const bool is_last_flight_leg
                                  , std::string &outErr)
{
/*
  <wp_hospital_pickup template="land" name="Hospital Pickup">
        <expected_location location_type="webosm" location_value="nm_between=10,30|tag=webosm_hospital|webosm_optimize=n"/>
        <display_object name="marker" instance_name="marker_pickup_patient" target_marker_b="yes" replace_elev_above_ground_ft="60"/>
        <display_object name="ambulance" instance_name="ambulance_end_target" random_tag="ambulance_select" relative_pos_bearing_deg_distance_mt="180|25"/>
        <fire_commands_at_leg_start commands="MIToolXP/External/Start"/>
        <fire_commands_at_leg_end commands="MIToolXP/External/Payload_Load"/>
        <desc>Pick up the patient at {navaid_loc_desc} (distance: ~{distance}, bearing: {bearing_target}).</desc>
  </wp_hospital_pickup>
 */

  outErr.clear ();
  // std::map<int, missionx::NavAidInfo> parsed_target_navaids;

  missionx::NavAidInfo na;
  na.fpln_expected_location_data = missionx::NavAidInfo::parse_expected_location (xml_leg_node_from_template, "custom content", is_last_flight_leg);

  // check if "expected location" is valid
  if (!na.fpln_expected_location_data.error.empty ())
  {
    outErr = na.fpln_expected_location_data.error;
    na.init();
    return na;
  }

  na.fpln_wp_type = (na.fpln_expected_location_data.flight_leg_type_hover_land_or_start.empty ()) ? mxconst::get_FL_TEMPLATE_VAL_LAND () : na.fpln_expected_location_data.flight_leg_type_hover_land_or_start;

  // handle "start" template
  if (mxconst::get_FL_TEMPLATE_VAL_START () == na.fpln_wp_type && mxUtils::isElementExists (in_mission_targets, 0) && in_mission_targets[0].is_lat_lon_valid ())
  {
    na = in_mission_targets[0]; // we want to go back to the start position
    if (na.getID ().empty ())
    {
      inout_shared_navaid.navAid = na;
      if (missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
      {
        // check distance and hopefully pick the correct airport. Since we are using a fixed distance, this might not be a 100% guaranty
        inout_shared_navaid.navAid.synchToPoint ();
        // const double dist = missionx::Point::calcDistanceBetween2Points (inout_random_airport_info_struct.navAid.p, inout_start_navaid.p);
        const double dist = inout_shared_navaid.navAid.p.calcDistanceBetween2Points (na.p, mx_units_of_measure::nm);
        if (dist <= 2.0 && !inout_shared_navaid.navAid.getID ().empty ())
        {

          if (na.getID ().empty () && !inout_shared_navaid.navAid.getID ().empty ())
            na.setID (inout_shared_navaid.navAid.getID ());

          if (na.getNavAidName ().empty () && !inout_shared_navaid.navAid.getNavAidName ().empty ())
            na.setName (inout_shared_navaid.navAid.getNavAidName ());

          na.height_mt = inout_shared_navaid.navAid.height_mt;
          na.navRef    = inout_shared_navaid.navAid.navRef;

          na.synchToPoint (true);
        }
      } // end "mx_flc_pre_command::get_nearest_nav_aid_to_randomLastFlightLeg_mainThread"
    } // end if ID is empty
  } // end handling "start"
  else
  {
    RandomEngine::flag_force_template_distances_b = na.fpln_expected_location_data.flag_force_template_distances_b;

    if (!na.fpln_expected_location_data.error.empty ())
    {
      Log::logMsgErr (na.fpln_expected_location_data.error, true);
      na.init();
      return na;
    }

    // The flightLegName will be overridden in a later function, when we will create the <leg> node.
    const std::string flightLegName = fmt::format ("{}_{}", mxconst::get_ELEMENT_LEG (), in_leg_counter);
    // const std::string flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_LAND ();
    // relevant only in case we use "tag_name" and we pick <points> from it.
    // Check if we have to force flight_leg_type on the random point that we might pick.
    const bool flag_force_flight_leg_type = Utils::readBoolAttrib (xml_leg_node_from_template, mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B (), false);

    missionx::mx_base_node targetProp; // v3.305.1

    // decide location_value_d value
    const std::string location_value_nm_s = mxUtils::getValueFromElement (na.fpln_expected_location_data.mapLocationSplitValues, std::string ("nm"), std::string (""));

    double location_value_d = -1.0;
    if (!location_value_nm_s.empty () && Utils::is_number (location_value_nm_s))
      location_value_d = Utils::stringToNumber<double> (location_value_nm_s, static_cast<int> (location_value_nm_s.length ()));


    targetProp.setStringProperty (mxconst::get_ATTRIB_NAME (), flightLegName); // leg name
    targetProp.setStringProperty (mxconst::get_ATTRIB_TYPE (), na.fpln_wp_type); // leg type
    targetProp.setStringProperty (mxconst::get_ATTRIB_LOCATION_TYPE (), na.fpln_expected_location_data.location_type); // location type
    targetProp.setBoolProperty (mxconst::get_PROP_IS_LAST_FLIGHT_LEG (), is_last_flight_leg); // is the last flight leg?
    targetProp.setBoolProperty (mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B (), flag_force_flight_leg_type); // force leg type ?
    // v25.09.2 unsupported properties, we won't force land or hover anymore.
    // targetProp.setNodeProperty<int> (mxconst::get_ATTRIB_FORCE_TYPE_OF_TEMPLATE (), static_cast<int> (which_type_to_force_enum)); // force level terrain or slope ?
    // targetProp.setNodeProperty<int> (mxconst::get_PROP_NUMBER_OF_LOOPS_TO_FORCE_TYPE_TEMPLATE (), how_many_times_to_loop_i); // a force slope will be used with webosm
    targetProp.setStringProperty ("nm", mxUtils::getValueFromElement (na.fpln_expected_location_data.mapLocationSplitValues, std::string ("nm"), std::string (""))); // location type
    targetProp.setStringProperty ("tag", mxUtils::getValueFromElement (na.fpln_expected_location_data.mapLocationSplitValues, std::string ("tag"), std::string (""))); // location type
    targetProp.setStringProperty ("nm_between", mxUtils::getValueFromElement (na.fpln_expected_location_data.mapLocationSplitValues, std::string ("nm_between"), std::string (""))); // location type
    targetProp.setNodeProperty<double> ("location_value_d", location_value_d); //
    targetProp.setNodeProperty<double> ("location_min_distance_d", na.fpln_expected_location_data.nm_between_min);
    targetProp.setNodeProperty<double> ("location_max_distance_d", na.fpln_expected_location_data.nm_between_max);


    if (is_last_flight_leg)
    {
      // auto result = get_target_or_lastFlightLeg_base_on_XY_or_OSM (na, data.mapLocationSplitValues, targetProp, location_value_d, data.nm_between_min, data.nm_between_max);
      // auto result = get_target_or_lastFlightLeg_base_on_XY_or_OSM (na, na.fpln_expected_location_data.mapLocationSplitValues, targetProp);
      if (mxUtils::isElementExists (in_mission_targets, in_leg_counter - 1))
      {
        const auto result = gen_target_or_last_flight_leg_base_on_xy_or_osm (na, RandomEngine::template_plane_type_enum, na.fpln_expected_location_data.mapLocationSplitValues, targetProp, &in_mission_targets[in_leg_counter - 1]);
        if (!result && !na.err.empty ())
          outErr = na.err;
      }
    }

    // else if (na.fpln_expected_location_data.location_type == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM () || na.fpln_expected_location_data.location_type == mxconst::get_EXPECTED_LOCATION_TYPE_OSM ())
    // {
    //   // auto result = get_targetForHelos_base_XY_OSM_OSMWEB (na, RandomEngine::template_plane_type_enum, data.mapLocationSplitValues, targetProp, location_value_d, data.nm_between_min, data.nm_between_max);
    //   auto result = get_targetForHelos_base_XY_OSM_OSMWEB (na, RandomEngine::template_plane_type_enum, na.fpln_expected_location_data.mapLocationSplitValues, targetProp);
    // }
    // else if (na.fpln_expected_location_data.location_type == mxconst::get_EXPECTED_LOCATION_TYPE_XY ())

    else // handle XY, OSM or OSMWEB
    {
      if (mxUtils::isElementExists (in_mission_targets, in_leg_counter - 1))
      {
        auto result = gen_target_base_on_xy_osm_or_osmweb_types (na, RandomEngine::template_plane_type_enum, na.fpln_expected_location_data.mapLocationSplitValues, targetProp, &in_mission_targets[in_leg_counter - 1]);
        if (!result && !na.err.empty ())
        {
          outErr = na.err;
          na.init ();
        }
      }
    }
  }

  return na;
}

// -----------------------------------

std::map<int, missionx::NavAidInfo>
RandomEngine::gen_content_targets (missionx::base_thread::thread_state *inoutThreadState, const IXMLNode &xTemplateNode, const IXMLNode &xContent, random_airport_info_struct &inout_shared_navaid, std::string &outErr)
{
  std::map<int, missionx::NavAidInfo> target_navaids;

  outErr.clear ();

  ///////////////////////////////////////////
  // Prepare base briefer data base on: parse <briefer_and_start_location> node
  IXMLNode x_briefer_and_start_location_node = xTemplateNode.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ().c_str ()).deepCopy ();
  target_navaids[0] = gen_parse_briefer_and_start_location ( x_briefer_and_start_location_node );
  if ( !target_navaids[0].is_lat_lon_valid () || !target_navaids[0].err.empty () )
  {
    outErr = fmt::format ("[{}] <{}> was not found in the template. Fix the template.", __func__, mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION () );
    target_navaids.clear ();
    return target_navaids;
  }

  //////////////////////////////////////////////////
  // Parse <content> List and build targets from it.
  // example returns: <delivery list="leg_delivery,leg_delivery|optional=50%,leg_delivery,leg_land," plane_type="prop">Hello// pilot.;Today you will fly to few locations to deliver goods. Use your 'inventory' to move items from/to your plane.</delivery>
  const std::string            flightLegList = Utils::readAttrib (xContent, mxconst::get_ATTRIB_LIST (), "");
  const std::list<std::string> listContent   = Utils::splitStringToList (flightLegList, mxconst::get_COMMA_DELIMITER ());
  std::string                  optional;
  int                          target_leg_counter = 0;

  // Loop over <list> items.
  // parse them and decide if they will be included or they are optional
  for (int content_list_item_counter = 0;
      auto tagName : listContent) // loop over the list of "tag_names" that we received from splitting the "list=" attribute
  {
    content_list_item_counter++;
    Log::logDebugBO ("[content tag name: " + tagName, true); // debug

    // Prepare the "tags" and split "rules" into a container for later use.
    ///// Split flight leg to its special rules. First string is the element tag name the rest needs format: "{name}={value}"
    std::vector<std::string>           vecRandomFlightLegRules = mxUtils::split_v2 (tagName, mxconst::get_PIPE_DELIMITER ());
    std::map<std::string, std::string> map_flight_leg_tag_names; // option name, option value
    int                                vecSize = static_cast<int> (vecRandomFlightLegRules.size ());
    // loop over split string
    for (int i1 = 0; i1 < vecSize; ++i1)
    {
      const std::string       &value    = vecRandomFlightLegRules.at (i1);
      std::vector<std::string> vecSplit = mxUtils::split_v2 (value, "=");

      if (i1 == 0) // the first value represents the tag name Example: leg_delivery|optional=50%. The tag name is leg_delivery.
      {
        Utils::addElementToMap (map_flight_leg_tag_names, "tag", vecRandomFlightLegRules.at (i1));
        tagName = vecRandomFlightLegRules.at (i1);
      }
      else // the second value onward is the rules on the tag name. Example: leg_delivery|optional=50%, the rule is: "optional=50%" which translate to 50% to create thie <leg>
      {
        if (vecSplit.size () >= 2)
        {
          const std::string &option_name  = vecSplit.at (0);
          const std::string &option_value = vecSplit.at (1);
          Utils::addElementToMap (map_flight_leg_tag_names, option_name, option_value);
        }
        else
          continue;
      }
    } // end loop over the rules

    // OPTIONAL test
    if (Utils::isElementExists (map_flight_leg_tag_names, mxconst::get_ATTRIB_OPTIONAL ())) // optional
      optional = map_flight_leg_tag_names[mxconst::get_ATTRIB_OPTIONAL ()];

    //// Handle Optional if set, there is no need to continue with the rest of the code if we fail here
    optional = Utils::replaceChar1WithChar2_v2 (optional, '%', ""); // v3.0.219.7 removes any % from string prior to handling it

    #ifndef RELEASE
    if (!optional.empty ())
      Log::logDebugBO ( fmt::format("[{}] Optional value: {}", __func__, optional), true);
    #endif

    // randomly decide if to skip or not based on optional value
    if (!optional.empty () && Utils::is_digits (optional))
    {
      const int percent = Utils::stringToNumber<int> (optional);
      if (const int result = Utils::getRandomIntNumber (0, 100)
        ; result > percent) // meaning missed
      {
        #ifndef RELEASE
        if (!optional.empty ())
          Log::logDebugBO (fmt::format ("[{}] Optional result value: {}, optional value: {}. Skipping tag: {}", __func__, Utils::formatNumber<int> (result), optional, tagName, true));
        #endif

        continue; // skip current leg creation for tagName
      }
    } // end optional

    IXMLNode xFlightLegNodeFromTemplate = Utils::xml_get_node_randomly_by_name_IXMLNode (xTemplateNode, tagName, false);
    if (xFlightLegNodeFromTemplate.isEmpty ())
    {
      outErr = fmt::format ("[{}] \"{}\", element was not found. Please fix template. Aborting mission creation.", __func__, tagName );
      target_navaids.clear ();
      return target_navaids;
    }

    ////////////////////////////////////////
    /// Generate Target from Content List
    ////////////////////////////////////////
    // std::string                         err;
    const bool                          is_last_leg = listContent.size () == static_cast<size_t> (content_list_item_counter);

    // Check if set of flight legs
    const std::string is_element_set_of_flight_legs = Utils::readAttrib (xFlightLegNodeFromTemplate, mxconst::get_ATTRIB_IS_SET_OF_FLIGHT_LEGS (), "");
    std::map<int, missionx::NavAidInfo> navaids;
    if (!is_element_set_of_flight_legs.empty ())
    {
      assert (false && fmt::format ("[{}:{}] Set of flight legs was not implemented yet.", __func__, __LINE__).c_str ());

    }
    else
    {
      navaids.clear ();
      auto na = gen_parse_template_leg (inoutThreadState, xTemplateNode, xFlightLegNodeFromTemplate, inout_shared_navaid, target_navaids, content_list_item_counter, is_last_leg, outErr);

      // Validate no errors during leg parsing from the template
      if (!na.is_lat_lon_valid () || !outErr.empty ())
      {
        target_navaids.clear ();
        return target_navaids;
      }

      // store the original template leg node
      na.fpln_xml_osm_q_or_raw_tmpl_node = xFlightLegNodeFromTemplate.deepCopy ();
      navaids[static_cast<int> (navaids.size ())] = na;
    }

    for (auto& [indx, na] : navaids)
    {
      if (na.is_lat_lon_valid ())
        target_navaids[++target_leg_counter] = na;
    }

  } // end loop over list tags


  return target_navaids;
}

missionx::mx_return
RandomEngine::gen_prepare_random_mission_based_on_content (IXMLNode &xTemplateNode)
{
  missionx::mx_return out_mx_return;

  //// get random content Node if available in template
  IXMLNode xContent = RandomEngine::get_content_story (xTemplateNode /*, inTemplateType*/);

  // we won't support random without content.
  if (xContent.isEmpty ())
  {
    RandomEngine::setError ("No <content> element was found. Aborting random mission creation. To fix this, please add <content> element. Check documentation.");
    return false;
  }

  // Plane type is mandatory at the <content> or <template> level
  // use plane_type from content then from template then throw error if it is not defined or it is not a valid type
  std::string pType = Utils::readAttrib (xContent, mxconst::get_ATTRIB_PLANE_TYPE (), "");
  if (RandomEngine::is_plane_type_valid (pType))
  {
    auto local_plane_type = this->setPlaneType (pType);
    pType                 = RandomEngine::translatePlaneTypeToString (local_plane_type);

    xTemplateNode.updateAttribute (pType.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ()); // fixed type to be what we fetched from xContent
  }
  else if (!this->randomPlaneType.empty ()) // v3.0.223.1 added template plane type fallback. Use the template plane_type attribute if it was not set at the content level
  {
    pType = this->randomPlaneType;
  }
  else
  { // v25.09.2 Added "any" plane fallback.
    this->setPlaneType (""); // set plane type to any
    // RandomEngine::setError (R"(Plane type was not set in template <content> or template level. Please notify the template writer or add plane_type="prop" or plane_type="helos" to the template. Check documentation for more details.)");
    // return false;
  }


  std::string err;
  auto navaid_targets = RandomEngine::gen_content_targets (&RandomEngine::threadState, xTemplateNode, xContent, RandomEngine::shared_navaid_info, err);

  ///////////////////
  // Validations
  ///////////////////
  ///// test min flight leg expected
  if (const auto min_valid_flight_legs_i = Utils::readNodeNumericAttrib<unsigned long> (xContent, mxconst::get_ATTRIB_MIN_VALID_FLIGHT_LEGS (), 1)
      ; min_valid_flight_legs_i > 0 && min_valid_flight_legs_i > navaid_targets.size ())
  {
    out_mx_return.addErrMsg ("Not enough valid targets were found. Aborting.", true);
    return out_mx_return;
  }

  // Test if we have targets
  if (navaid_targets.empty ())
  {
    out_mx_return.addErrMsg ("No valid targets were found. Aborting.", true);
    return out_mx_return;
  }

  // check [abort] by user
  if (RandomEngine::threadState.flagAbortThread)
  {
    out_mx_return.addErrMsg ("User asked to abort.", true);
    return out_mx_return;
  }

  bool flag_one_of_the_targets_above_water = false;
  //-----------------------------------------------
  //--- Analyze Water Bodies / Slope / Leg Name ---
  //-----------------------------------------------
  for (auto &target_navaid : navaid_targets | std::views::values)
  {
    // make sure a waypoint type is set
    if (target_navaid.fpln_wp_type.empty ())
      target_navaid.fpln_wp_type = mxconst::get_FL_TEMPLATE_VAL_LAND();

    target_navaid.fpln_is_wet = get_is_wet_at_point (target_navaid);

    // store wet state if the "flag value" is not true, yet.
    if (!flag_one_of_the_targets_above_water)
      flag_one_of_the_targets_above_water = target_navaid.fpln_is_wet;

    // store slope at the target location
    target_navaid.fpln_slope = get_slope_at_point (target_navaid);

    target_navaid.fpln_leg_name = gen_leg_name ( &this->seq_waypoints, mxconst::get_GPS_WP (),"leg", target_navaid );
  }

  // ----------------------
  // -- Add <briefer> node - Start Location BUT NOT the description.
  // ----------------------
  navaid_targets[0].fpln_navaid_was_already_prepared = true; // force flag
  gen_briefer_node (navaid_targets[0], RandomEngine::shared_navaid_info, flag_one_of_the_targets_above_water);


  // Add <mission_info> from template, we do not generate it.
  IXMLNode x_local_BrieferInfo = xTemplateNode.getChildNode (mxconst::get_ELEMENT_MISSION_INFO ().c_str ());
  if (x_local_BrieferInfo.isEmpty ())
  {
    out_mx_return.addErrMsg (fmt::format("{} element is missing from the base template.", mxconst::get_ELEMENT_MISSION_INFO () ), true) ;
    return out_mx_return;
  }


  // ------------------------------------------------------------------
  // Construct all mission <leg> nodes
  // navaid_targets: [0] = start/briefer, [1], [2]..[N-1] = final location.
  // ------------------------------------------------------------------
  if (!navaid_targets.empty())
  {
    // loop over all targets
    for (auto &[indx, target_navaid] : navaid_targets)
    {
      target_navaid.fpln_seq = indx;

      if (indx == 0) // skip briefer
      {
        target_navaid.fpln_mission_phase = missionx::enums::mx_rnd_mission_phase::start;
        continue;
      }

      // is last flight leg ?
      target_navaid.fpln_is_last_flight_leg = ! ( mxUtils::isElementExists (navaid_targets, indx + 1) );

      // TODO: consider cases where last leg is a "set of legs" and not a standalone NavAid
      if (target_navaid.fpln_is_last_flight_leg)
        target_navaid.fpln_mission_phase = enums::mx_rnd_mission_phase::land_extraction; // represent last waypoint
      else
        target_navaid.fpln_mission_phase = enums::mx_rnd_mission_phase::land_target; // represent target


      if (!target_navaid.fpln_copy_as_is_b)
      {
        // We are basically constructing the mission from the middle waypoint and then need to add the start and end coordinates.
        // Write dedicated functions to only prepare the specific "needed" node.
        // Example: prepare trigger (seq, name, radius)
        IXMLNode xTriggerTargetHover = IXMLNode::emptyIXMLNode;
        IXMLNode xTriggerTargetLand = IXMLNode::emptyIXMLNode;
        IXMLNode xTaskTargetHover = IXMLNode::emptyIXMLNode;
        IXMLNode xTaskTargetLand  = IXMLNode::emptyIXMLNode;


        // ----------------------------------------------------------
        // Prepare the properties for the target triggers and tasks
        // ----------------------------------------------------------


        // Land properties
        const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_trig_land  =
        {
          { "conditions", "plane_on_ground", "true" },
          { "radius", "length_mt", (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())? mxconst::DEFAULT_LAND_OR_INV_RADIUS_MT.data() : mxconst::DEFAULT_HOVER_RADIUS_MT.data () }
        };

        // Hover trigger properties
        const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_trig_hover = {
          { "conditions", "plane_on_ground", "false" },
          { "radius", "length_mt", mxconst::DEFAULT_HOVER_RADIUS_MT.data () },
          { "elevation_volume", "elev_lower_upper_ft", fmt::format("---{}", mxconst::DEFAULT_HOVER_VOL_HEIGHT_FOR_OSM_FT ) }, // example: "---328"
        };


        // Land trigger creation
        xTriggerTargetLand = RandomEngine::gen_trigger_node (this->seq_triggers, "trig", "land", target_navaid, lsAttrib_trig_land, nullptr);
        // Hover trigger creation
        if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
          xTriggerTargetHover = RandomEngine::gen_trigger_node (this->seq_triggers, "trig", "hover", target_navaid, lsAttrib_trig_hover, nullptr);


        // TASK setup will initialize after trigger setup since we need values from the trigger node
        // Land + Hover Task Properties
        const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_land_task_target = {
          { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_BASE_ON_TRIGGER (), Utils::readAttrib (xTriggerTargetLand, mxconst::get_ATTRIB_NAME (), "") },
          { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_MANDATORY (), "true" }, { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC (), "20" },
        };
        const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_hover_task_target = {
          { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_BASE_ON_TRIGGER (), Utils::readAttrib (xTriggerTargetHover, mxconst::get_ATTRIB_NAME (), "") },
          { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_MANDATORY (), "true" },
          { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC (), "20" },
        };

        // Land Task
        xTaskTargetLand    = RandomEngine::gen_task_node (this->seq_tasks, "task", "land", target_navaid, lsAttrib_land_task_target, nullptr);

        // Hover Task
        if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
          xTaskTargetHover = RandomEngine::gen_task_node (this->seq_tasks, "task", "hover", target_navaid, lsAttrib_hover_task_target, nullptr);

        // END Handling LAND + HOVER Triggers and Tasks


        // -------------------------------------------------
        // Add inventory - always create an inventory node

        // prepare attributes to modify
        const auto inv_radius = (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())? mxconst::DEFAULT_LAND_OR_INV_RADIUS_MT.data () : mxconst::DEFAULT_HOVER_RADIUS_MT.data ();
        const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_inv = {
          { mxconst::get_ELEMENT_POINT (), mxconst::get_ATTRIB_LAT (), target_navaid.getLat () },
          { mxconst::get_ELEMENT_POINT (), mxconst::get_ATTRIB_LONG (), target_navaid.getLon () },
          { mxconst::get_ELEMENT_RADIUS (), mxconst::get_ATTRIB_LENGTH_MT (), inv_radius },
          { mxconst::get_ELEMENT_INVENTORY (), mxconst::get_ATTRIB_INHIBIT_MXPAD_B (), "true" }, // inhibit mx-pad toggle when entering inventory area and we are airborne
        };

        target_navaid.fpln_xml_inv_node = gen_inventory_node (indx, target_navaid, map_osm_inventory_track, lsAttrib_inv);

        // DEPRECATED
        // In <content> template we do not generate inventory scripts
        // // todo: move script creation after all targets were generated
        // if (!target_navaid.fpln_xml_inv_node.isEmpty ())
        // {
        //   // create scripts and attach them into the <inventory> as a sub-element.
        //   RandomEngine::gen_target_inventory_scripts (target_navaid, map_osm_inventory_track);
        // }

        // ----------------
        // Create Objective
        // ----------------
        IXMLNode xTargetObjective = RandomEngine::gen_objective_node (this->seq_objectives, "obj", "target");


        // -------------------------------------------------
        // POST Actions to update the Trigger information.
        // We need the names and values that were not available during node creation.
        // -------------------------------------------------
        const auto task_land_name  = Utils::readAttrib (xTaskTargetLand, mxconst::get_ATTRIB_NAME (), "");
        const auto task_hover_name = Utils::readAttrib (xTaskTargetHover, mxconst::get_ATTRIB_NAME (), "");

        const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_outcome_target_trig =
        {
          {mxconst::get_ELEMENT_OUTCOME (), mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS (), fmt::format ("{}{}", task_land_name, (task_hover_name.empty ()? "" : "," + task_hover_name) ) },
          };

        // Set Triggers post attributes
        if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
          Utils::xml_search_and_set_attributes_in_node (xTriggerTargetHover, lsAttrib_outcome_target_trig);
        Utils::xml_search_and_set_attributes_in_node (xTriggerTargetLand, lsAttrib_outcome_target_trig);

        // Link task to objective
        if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
          xTargetObjective.addChild (xTaskTargetHover);
        xTargetObjective.addChild (xTaskTargetLand);

        //-------------------------
        // <leg> element creation
        //-------------------------
        const auto obj_target_name = Utils::readAttrib (xTargetObjective, mxconst::get_ATTRIB_NAME (), "");
        std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_wp_target =
        {
          { mxconst::get_ELEMENT_LEG (), mxconst::get_ATTRIB_TITLE (), target_navaid.gen_locDesc_short () },
          { mxconst::get_ELEMENT_LINK_TO_OBJECTIVE (), mxconst::get_ATTRIB_NAME (), obj_target_name },
        };


        if ( mxUtils::isElementExists (RandomEngine::map_flight_legs_translation_from_template, target_navaid.fpln_seq) )
        {
          target_navaid.fpln_xml_target_leg_node = missionx::data_manager::xmlMappingNode.getChildNode (missionx::RandomEngine::map_flight_legs_translation_from_template[target_navaid.fpln_seq].c_str ()).deepCopy ();
          const bool b_successfully_set_leg_tag  = Utils::xml_set_tag_name (target_navaid.fpln_xml_target_leg_node, mxconst::get_ELEMENT_LEG ());
        }

        target_navaid.fpln_xml_target_leg_node = RandomEngine::gen_leg_node ( mxconst::get_GPS_WP (), "leg", &target_navaid, &lsAttrib_wp_target);
        #ifndef RELEASE
        Log::logMsgThread (fmt::format ("[{}] oilrig leg index: {}, Node element:\n{}\n<-----", __func__, target_navaid.fpln_seq, Utils::xml_get_node_content_as_text (target_navaid.fpln_xml_target_leg_node)));
        #endif


        // Test if the user asked for skew target location
        gen_skew_target_data (target_navaid);

        // Add to main mission nodes
        this->xTriggers.addChild (xTriggerTargetLand);
        this->xTriggers.addChild (xTriggerTargetHover);
        this->xObjectives.addChild (xTargetObjective);

        // find elevation using call to the main thread
        target_navaid.height_mt = this->get_terrain_elevation_at_point_in_mt (target_navaid, RandomEngine::shared_navaid_info);
        target_navaid.synchToPoint ();

        //-------------------------
        // Calculate distances, bearing and initialize the "next_leg" or "starting_leg" of the <leg>/<briefer> nodes
        //-------------------------
        if (mxUtils::isElementExists (navaid_targets, (indx - 1)) )
        {
          // lambda to return next Navaid pointer
          auto lmbda_get_next_navaid_as_ptr =[&](const int local_index) -> missionx::NavAidInfo*
          {
            if (mxUtils::isElementExists (navaid_targets, local_index + 1))
              return &navaid_targets[local_index + 1];

            return nullptr;
          };

          auto next_navaid_ptr = lmbda_get_next_navaid_as_ptr(indx);

          // TODO: Handle "copy_leg_as_is_b" cases in gen_gather_navaid_metadata_relative_to_target() or write a new function
          gen_gather_navaid_metadata_relative_to_target (this->xMetadata, target_navaid, navaid_targets[indx - 1], next_navaid_ptr);

          // If we have a previous <leg> node, we need to update its "next_leg" and "distance" attributes for future usage with messages and descriptions
          if (!navaid_targets[indx - 1].fpln_xml_target_leg_node.isEmpty ())
          {
            // update "starting_leg" or "next_leg" attributes
            const auto current_leg_name = Utils::readAttrib ( target_navaid.fpln_xml_target_leg_node, mxconst::get_ATTRIB_NAME (), "" );
            if (navaid_targets[indx - 1].flag_is_brieferOrStartLocation)
              navaid_targets[indx - 1].fpln_xml_target_leg_node.updateAttribute (current_leg_name.c_str (), mxconst::get_ATTRIB_STARTING_LEG ().c_str (), mxconst::get_ATTRIB_STARTING_LEG ().c_str () );
            else
              navaid_targets[indx - 1].fpln_xml_target_leg_node.updateAttribute (current_leg_name.c_str (), mxconst::get_ATTRIB_NEXT_LEG ().c_str (), mxconst::get_ATTRIB_NEXT_LEG ().c_str () );

            navaid_targets[indx - 1].fpln_xml_target_leg_node.updateAttribute (fmt::format ("{}", navaid_targets[indx - 1].fpln_distance_between_prev_and_current_navaid).c_str (), mxconst::get_ATTRIB_DISTANCE_NM ().c_str (), mxconst::get_ATTRIB_DISTANCE_NM ().c_str () );


            // add <leg> description
            IXMLNode xml_desc_ptr = gen_leg_description (target_navaid.fpln_xml_target_leg_node, target_navaid, next_navaid_ptr);

            // add start messages
            gen_leg_start_messages (this->seq_messages, target_navaid,this->xMessages);

            // add hint messages related to the target land/hover actions
            gen_messages_when_reaching_target_leg (this->seq_triggers, this->seq_messages, target_navaid, this->xMessages, this->xTriggers, xTriggerTargetLand, xTriggerTargetHover);

            // generate a message when nearing the target (2 nm)
            gen_2nm_message (this->seq_triggers, this->seq_messages, target_navaid, this->xMessages, this->xTriggers, xTriggerTargetLand);

            // #ifndef RELEASE
            // Log::logMsgThread ( fmt::format ("[{}]debug debug debug debug debug:\nfpln_xml_target_leg_node:\n\t{}\n\nfpln_xml_osm_q_or_raw_tmpl_node:\n\t{}\n\n<-- debug debug debug debug debug", __func__, Utils::xml_get_node_content_as_text (target_navaid.fpln_xml_target_leg_node), Utils::xml_get_node_content_as_text (target_navaid.fpln_xml_osm_q_or_raw_tmpl_node)) );
            // #endif

            // add 3D object sets
            // gen_add_3d_objects_for_surprise_me_base_on_predefined_attributes (target_navaid, target_navaid.fpln_xml_target_leg_node, xTemplateNode, this->x3DObjTemplate, this->expected_slope_at_target_location_d);
            gen_add_3d_display_object_sets_instances_to_leg (target_navaid, target_navaid.fpln_xml_target_leg_node, xTemplateNode, this->x3DObjTemplate, this->expected_slope_at_target_location_d);

            // add <display_object> from the template custom leg node. Should be a subnode.
            gen_parse_and_add_all_display_objects_in_node (__func__, target_navaid.fpln_xml_osm_q_or_raw_tmpl_node, target_navaid.fpln_xml_target_leg_node, xTemplateNode, this->x3DObjTemplate, this->expected_slope_at_target_location_d);


            // add 3D display objects around the landing
            if (!target_navaid.flag_is_skewed)
              gen_3d_hint_objects_for_land_and_hover (target_navaid, target_navaid.fpln_xml_target_leg_node, next_navaid_ptr);

            gen_parse_3d_instances_in_leg (target_navaid.fpln_xml_target_leg_node, target_navaid);
          }
        } // end if target navaid is not the first or last

        target_navaid.synchToPoint ();
        target_navaid.fpln_xml_target_leg_node = this->xFlightLegs.addChild (target_navaid.fpln_xml_target_leg_node);

        // Add the final flight plan to display in the ui
        this->cumulative_location_desc_s += target_navaid.get_loc_desc () + (( static_cast<int>(navaid_targets.size ()) - 1 == target_navaid.fpln_seq)? "" : ", ");

      } // end if Navaid is not "copy_leg_as_is_b"
      else
      {
        // TODO: add "copy_leg_as_is_b=true" cases
        assert (false && fmt::format("[{}:{}] You have to handle 'copy_leg_as_is_b' cases", __func__, __LINE__).c_str ());
      }

      // check [abort]
      if (RandomEngine::threadState.flagAbortThread)
      {
        out_mx_return.addErrMsg ("User asked to abort.", true);
        return out_mx_return;
      }

    } // end "osm_target" loop over all OSM Target NavAids and construct the base information needed for the mission file


    // ----------------------
    // -- Prepare <GPS> node
    // ----------------------
    // TODO: Handle "copy_leg_as_is_b" cases
    for (const auto &na : navaid_targets | std::views::values)
    {
      auto p_gps_node = na.p.node.deepCopy ();
      auto p_gps_skew_node =  ( na.xml_skewdPointNode.isEmpty ()) ? IXMLNode::emptyIXMLNode : na.xml_skewdPointNode.deepCopy ();

      p_gps_node = Utils::xml_clear_node_attributes_excluding_list (p_gps_node,
                                            { mxconst::get_ATTRIB_LAT (), mxconst::get_ATTRIB_LONG (), mxconst::get_ATTRIB_ELEV_FT ()
                                              , mxconst::get_ELEMENT_ICAO (), mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B ()
                                              , mxconst::get_PROP_IS_WET ()
                                            }, false, true);

      p_gps_skew_node = Utils::xml_clear_node_attributes_excluding_list (p_gps_skew_node,
                                            { mxconst::get_ATTRIB_LAT (), mxconst::get_ATTRIB_LONG (), mxconst::get_ATTRIB_ELEV_FT ()
                                              , mxconst::get_ELEMENT_ICAO (), mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B ()
                                              , mxconst::get_PROP_IS_WET ()
                                            }, false, true);

      if (na.flag_is_skewed && !p_gps_skew_node.isEmpty ())
        this->xGPS.addChild (p_gps_skew_node);
      else
        this->xGPS.addChild (p_gps_node);
    }

    // add Briefer description
    gen_post_briefer_desc (navaid_targets, flag_one_of_the_targets_above_water);
    this->xBriefer = navaid_targets[0].fpln_xml_target_leg_node.deepCopy ();

    // add <mission_info>
    if (!gen_read_mission_info_element ()) // <mission_info>
    {
      missionx::RandomEngine::threadState.flagAbortThread = true;
      out_mx_return.addErrMsg ("No <mission_info> node was found in template.", true);
    }

    // loop over all inventories and add to the global xInventories node
    // TODO: Handle "copy_leg_as_is_b" cases
    for (auto &[key, nav] : navaid_targets )
    {
      // add to inventories
      nav.fpln_xml_inv_node = this->xInventoris.addChild (nav.fpln_xml_inv_node);
    }



    #ifndef RELEASE
    Log::logMsgThread (fmt::format ("-------------- <CONTENT_MISSION> RESULTS - Post {} --------------", __func__));
    Log::logMsgThread (fmt::format ("BRIEFER_INFO:\n{}\n", Utils::xml_get_node_content_as_text (x_local_BrieferInfo)));
    Log::logMsgThread (fmt::format ("BRIEFER:\n{}\n", Utils::xml_get_node_content_as_text (navaid_targets[0].fpln_xml_target_leg_node))); // we store the briefer in [0]
    Log::logMsgThread (fmt::format ("TRIGGERS:\n{}\n", Utils::xml_get_node_content_as_text (this->xTriggers)));
    Log::logMsgThread (fmt::format ("OBJECTIVES:\n{}\n", Utils::xml_get_node_content_as_text (this->xObjectives)));
    Log::logMsgThread (fmt::format ("FLIGHT LEGS:\n{}\n", Utils::xml_get_node_content_as_text (this->xFlightLegs)));
    Log::logMsgThread (fmt::format ("Inventories:\n{}\n", Utils::xml_get_node_content_as_text (this->xInventoris)));
    Log::logMsgThread (fmt::format ("GPS:\n{}\n", Utils::xml_get_node_content_as_text (this->xGPS)));
    Log::logMsgThread (fmt::format ("-------------- END <CONTENT_MISSION> RESULTS - {} --------------", __func__));
    #endif // !RELEASE

  } // End if target container is not empty

  out_mx_return.result = true;
  return out_mx_return;
}

// -----------------------------------

bool
RandomEngine::generateRandomMissionBasedOnContent (IXMLNode &xTemplateNode)
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
  int      legCounter              = 1;
  int      min_valid_flight_legs_i = 1; // v3.0.221.15rc3.2

  mapFlightPlanOrder_si.clear ();
  mapFLightPlanOrder_is.clear ();

  this->flag_isLastFlightLeg = false;
  this->setPlaneType (mxconst::get_PLANE_TYPE_HELOS ()); // initialization will initialize this->randomPlaneType

  mission_types_enum mType = mission_types_enum::random; // v3.0.223.1 changed to always random. medevac or deliver missions are part of the flight leg list and not template oriented.

  //// get random content Node if available in template
  IXMLNode xContent = RandomEngine::get_content_story (xTemplateNode /*, inTemplateType*/);

  // v3.0.223.1 we won't support random without content.
  if (xContent.isEmpty ())
  {
    RandomEngine::setError ("No <content> element was found. Aborting random mission creation. To fix this, please add <content> element. Check documentation.");
    return false;
  }


  // Plane type is now mandatory at content or template level
  // use plane_type from content, if was defined
  std::string pType = Utils::readAttrib (xContent, mxconst::get_ATTRIB_PLANE_TYPE (), "");
  mx_plane_types local_plane_type = mx_plane_types::plane_type_any;
  if (!pType.empty ())
  {
    local_plane_type = this->setPlaneType (Utils::stringToLower (pType));
    xTemplateNode.updateAttribute (this->randomPlaneType.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ()); //// v3.0.221.7 fixed type to be what we fetched from xContent
  }
  else if (!this->randomPlaneType.empty ()) // v3.0.223.1 added template plane type fallback. Use template plane_type attribute if was not set at content level
  {
    pType = this->randomPlaneType;
    local_plane_type = this->setPlaneType (Utils::stringToLower (pType));
  }
  else
  {
    RandomEngine::setError ("Plane type was not set in template <content> or template level. Please notify the template writer or add 'plane_type=\"prop|helos\" to the template. See documentation.");
    return false;
  }


  // update template type so will affect buildFlightLeg() later.
  xTemplateNode.updateAttribute ("", mxconst::get_ATTRIB_TYPE ().c_str (), mxconst::get_ATTRIB_TYPE ().c_str ()); // can be changed during random type

  //// Read the briefer information after we decide the plane type. This will help with the "filter" of the ramp start in the briefer code.
  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  if (!prepareBrieferAndStartLocation ()) // main <briefer>
  {
    missionx::RandomEngine::threadState.flagAbortThread = true;
    return false;
  }

  Log::logDebugBO ("[DEBUG random airport] After <briefer_and_start_location> node in content.", true);

  // v3.0.255.4.2
  if (!gen_read_mission_info_element ()) // <mission_info>
  {
    missionx::RandomEngine::threadState.flagAbortThread = true;
    return false;
  }

  ////// PREPARE Flight Plan BASED ON TYPE
  //    int nChilds = 0;
  std::string content_text;
  content_text.clear ();

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
  min_valid_flight_legs_i = (int)Utils::readNodeNumericAttrib<int> (xContent, mxconst::get_ATTRIB_MIN_VALID_FLIGHT_LEGS (), 1); // v3.0.221.15 rc3.3 // v3.0.303 changed to readNodeNumericAttrib

  std::list<std::string> listContent;

  if (xContent.isEmpty ())
    return false;

  Log::logDebugBO ("[random] picked content element", true);

  // example returns: <delivery list="leg_delivery,leg_delivery|optional=50%,leg_delivery,leg_land," plane_type="prop">Hello// pilot.;Today you will fly to few locations to deliver goods. Use your 'inventory' to move items from/to your plane.</delivery>
  std::string flightLegList = Utils::readAttrib (xContent, mxconst::get_ATTRIB_LIST (), "");

  listContent = Utils::splitStringToList (flightLegList, mxconst::get_COMMA_DELIMITER ());

  auto countListElements = listContent.size ();
  int  counter           = 0;
  for (std::string optional; auto tagName : listContent) // loop over list of "tag_names" that we received from splitting the "list=" attribute
  {
    ++counter;
    if (counter == static_cast<int> (countListElements)) // check if last flight leg in content
      this->flag_isLastFlightLeg = true;

    Log::logDebugBO ("[content tag name: " + tagName, true); // debug v3.0.223.1


    ///// v3.0.221.7 Split flight leg to its special rules. First string is the element tag name the rest needs format: "{name}={value}"
    std::vector<std::string>           vecRandomFlightLegRules = mxUtils::split_v2 (tagName, mxconst::get_PIPE_DELIMITER ());
    std::map<std::string, std::string> mapFlightLegDirectives; // option name, option value
    int                                vecSize = static_cast<int> (vecRandomFlightLegRules.size ());
    for (int i1 = 0; i1 < vecSize; ++i1)
    {
      const std::string       &value    = vecRandomFlightLegRules.at (i1);
      std::vector<std::string> vecSplit = mxUtils::split_v2 (value, "=");

      if (i1 == 0) // first value represents the tag name Example: leg_delivery|optional=50%. The tag name is leg_delivery.
      {
        Utils::addElementToMap (mapFlightLegDirectives, "tag", vecRandomFlightLegRules.at (i1));
        tagName = vecRandomFlightLegRules.at (i1);
      }
      else // the second value onward are the rules on the tag name. Example: leg_delivery|optional=50%, the rule is: "optional=50%" which translate to 50% to create thie <leg>
      {
        if (vecSplit.size () >= 2)
        {
          const std::string &option_name  = vecSplit.at (0);
          const std::string &option_value = vecSplit.at (1);
          Utils::addElementToMap (mapFlightLegDirectives, option_name, option_value);
        }
        else
          continue;
      }
    }

    if (Utils::isElementExists (mapFlightLegDirectives, mxconst::get_ATTRIB_OPTIONAL ())) // optional
      optional = mapFlightLegDirectives[mxconst::get_ATTRIB_OPTIONAL ()];


    //// Handle Optional if set, there is no need to continue with the rest of the code if we fail here
    optional = Utils::replaceChar1WithChar2_v2 (optional, '%', ""); // v3.0.219.7 removes any % from string prior to handling it

    #ifndef RELEASE
    if (!optional.empty ())
      Log::logDebugBO ("[DEBUG generate random based on content element] Optional value: " + optional, true);
    #endif


    // v3.0.221.7 optional test
    if (!optional.empty () && Utils::is_digits (optional))
    {
      int percent = Utils::stringToNumber<int> (optional);
      if (int result = Utils::getRandomIntNumber (0, 100); result > percent) // meaning missed
      {
        #ifndef RELEASE
        if (!optional.empty ())
          Log::logDebugBO (fmt::format ("[DEBUG generate random based on content element] Random result value: {}, optional value: {}. Skipping tag: {}", Utils::formatNumber<int> (result), optional, tagName, true));
        #endif

        continue;
      }
    }

    RandomEngine::errMsg.clear ();
    // xFlightLegNodeFromTemplate = Utils::xml_get_node_randomly_by_name_IXMLNode (xTemplateNode, tagName, RandomEngine::errMsg, false);
    xFlightLegNodeFromTemplate = Utils::xml_get_node_randomly_by_name_IXMLNode (xTemplateNode, tagName, false);
    // if (!errMsg.empty () || xFlightLegNodeFromTemplate.isEmpty ())
    if (xFlightLegNodeFromTemplate.isEmpty ())
    {
      // RandomEngine::setError (((errMsg.empty ()) ? std::string (mxconst::get_QM ()).append (tagName).append (mxconst::get_QM ()).append (" element was not found. Please fix template. Aborting mission creation.") : RandomEngine::errMsg));
      RandomEngine::setError ( fmt::format ("[{}] \"{}\", element was not found. Please fix template. Aborting mission creation.", __func__, tagName ) );
      return false;
    }


    // Check if set of flight legs
    const std::string is_element_set_of_flight_legs = Utils::readAttrib (xFlightLegNodeFromTemplate, mxconst::get_ATTRIB_IS_SET_OF_FLIGHT_LEGS (), "");
    if (!is_element_set_of_flight_legs.empty ())
    {
      this->extract_flight_leg_set (xTemplateNode, xFlightLegNodeFromTemplate, legCounter);
    }
    else
    {
      // During flight leg construction the <leg> might be skipped due to it being optional or malformed. We should not abort the mission creation because of that.
      this->build_and_add_flight_leg_from_node (xFlightLegNodeFromTemplate, legCounter);
    }

  } // end loop over list of flight legs


  // validate mission
  if (xFlightLegs.nChildNode () < min_valid_flight_legs_i)
  {
    RandomEngine::setError ("Failed to create a mission based on <content>. Generated: " + Utils::formatNumber<int> (xFlightLegs.nChildNode ()) + " 'flight legs', but needs minimum: " + Utils::formatNumber<int> (min_valid_flight_legs_i) + " 'legs'. Aborting mission creation. Try to generate a new one.");
    return false;
  }

  content_text = Utils::xml_get_text (xContent); // xContent.getText();

  // This code replaced the for (auto nav : RandomEngine::listNavInfo) just for readability I did not test it yet.
  // This should be more readable, do not think it is faster though.
  const auto lmbda_nav_info_used_random_lat_long = [&] (const missionx::NavAidInfo &navInfo) { return (navInfo.flag_picked_random_lat_long == true); }; // check NavInfo if it has the "flag_picked_random_lat_long == true"
  if (std::ranges::any_of (RandomEngine::listNavInfo, lmbda_nav_info_used_random_lat_long))
  {
    this->setPlaneType (mxconst::get_PLANE_TYPE_HELOS ());
    mType = mission_types_enum::medevac;
  }

  // Modify briefer description
  const std::string message = (content_text.empty ()) ? "Welcome pilot, in this flight you will have to reach a location on your GPS " + std::string (((mType == mission_types_enum::medevac) ? "to assist in evacuation" : "to deliver goods")) + ". The plane type picked for this mission is: " + this->randomPlaneType + ", but you can pick another if you think you can make it." : content_text;
  Utils::xml_add_cdata (this->xBriefer, message);
  Utils::xml_search_and_set_attribute_in_IXMLNode (this->xBrieferInfo, mxconst::get_ATTRIB_PLANE_DESC (), this->randomPlaneType);

  if (missionx::RandomEngine::threadState.flagAbortThread) // v3.0.219.12+
  {
    RandomEngine::setError ("[Random] generateRandomShortMission() Aborted !!!");
    return false;
  }

  this->fill_up_next_leg_attrib_after_flight_plan_was_generated ();

  return true;
}


// -----------------------------------
// -----------------------------------


IXMLNode
RandomEngine::buildFlightLeg (int inFlightLegCounter, const IXMLNode &in_legNodeFromTemplate)
{
  RandomEngine::errMsg.clear ();

  // check if there is any "<leg>" template node in MAPPING element
  if (Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_LEG ()).isEmpty ())
  {
    RandomEngine::setError ("[Random] Could not find flight <leg> mapping. Please notify designer/programmer !!!"); // this should be displayed
    return IXMLNode::emptyIXMLNode;
  }


  // prepare  flight leg elements
  IXMLNode xNewFlightLeg     = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_LEG ()); // holds the new flight <leg> element being constructed.
  IXMLNode xLegTargetTrigger = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_TRIGGER ()); // holds trigger element from MAPPING
  IXMLNode xMapMessage       = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE (), true); // holds message element from MAPPING
  IXMLNode xLegTask          = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_TASK ()); // holds task element from MAPPING
  IXMLNode xLegObjective     = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_OBJECTIVE ()); // holds objective element from MAPPING


  //// Validate XML mapping are set correctly or MAPPING is not complete and need to be fixed.
  if (xNewFlightLeg.isEmpty () || xLegTargetTrigger.isEmpty ())
  {
    RandomEngine::setError ("[random flightLeg] mapping element does not have flight <leg> or <trigger> mapping template. Fix template file or notify designer/programmer.");
    return IXMLNode::emptyIXMLNode;
  }


  // set the most basic data on <leg> element. The rest of the <leg> element will be added later
  IXMLNode xLegFromTemplate = in_legNodeFromTemplate.deepCopy (); // v3.0.219.2
  if (xLegFromTemplate.isEmpty ()) // v3.0.219.2
  {
    RandomEngine::setError ("[random flightLeg] Failed to find a template <leg>. Please fix your template and retry.");
    return IXMLNode::emptyIXMLNode;
  }

  // v3.0.223.4 task_name_for_leg_message and  trigger_name_for_leg_message. these attributes should be in <expected_location> template element
  std::string override_task_name_s    = Utils::xml_get_attribute_value_drill (xLegFromTemplate, mxconst::get_ATTRIB_OVERRIDE_TASK_NAME (), this->flag_found, mxconst::get_ELEMENT_EXPECTED_LOCATION ());
  std::string override_trigger_name_s = Utils::xml_get_attribute_value_drill (xLegFromTemplate, mxconst::get_ATTRIB_OVERRIDE_TRIGGER_NAME (), this->flag_found, mxconst::get_ELEMENT_EXPECTED_LOCATION ());
  // END v3.0.223.4
  bool flag_disable_auto_messages = Utils::readBoolAttrib (xLegFromTemplate, mxconst::get_ATTRIB_DISABLE_AUTO_MESSAGE_B (), false); // v3.0.241.1 replaced  disable_auto_message_s with simpler implementation



  const int flightLegCounter = inFlightLegCounter; // v3.0.241.8 made it const // v3.0.219.2 to keep same code parameters as the original one.

  std::string flight_leg_type_hover_land_or_start = mxUtils::stringToLower (Utils::readAttrib (xLegFromTemplate, mxconst::get_ATTRIB_TEMPLATE (), EMPTY_STRING));
  std::string stored_flight_leg_template_type     = flight_leg_type_hover_land_or_start; // v3.0.253.6 use when we want to force flight leg type and it might be changed during the "buildFlightLeg()" function.
  std::string flightLegName                       = Utils::readAttrib (xLegFromTemplate, mxconst::get_ATTRIB_NAME (), EMPTY_STRING);
  std::string attrib_Optional                     = Utils::readAttrib (xLegFromTemplate, mxconst::get_ATTRIB_OPTIONAL (), EMPTY_STRING); // v3.0.219.6
  std::string attrib_dependsOn                    = Utils::readAttrib (xLegFromTemplate, mxconst::get_ATTRIB_DEPENDS_ON (), EMPTY_STRING); // v3.0.219.11
  bool        skip_auto_task_creation_b           = Utils::readBoolAttrib (xLegFromTemplate, mxconst::get_ATTRIB_SKIP_AUTO_TASK_CREATION_B (), false); // v3.0.221.15 rc4

  attrib_Optional = Utils::replaceChar1WithChar2_v2 (attrib_Optional, '%', ""); // v3.0.219.7 removes any % from string before handling it

  #ifndef RELEASE
  Log::logMsgNone ("\n**** [DEBUG buildFlightLeg] Try to build a Flight Leg for - flight_leg_type: " + flight_leg_type_hover_land_or_start + ", flight_leg_name: " + flightLegName + " **** \n", true);
  #endif


  // v3.0.219.6 optional test
  if (!attrib_Optional.empty () && Utils::is_digits (attrib_Optional))
  {
    int percent = Utils::stringToNumber<int> (attrib_Optional);
    if (percent < 0)
      percent = 1;
    if (percent > 100)
      percent = 99;

    int result = Utils::getRandomIntNumber (0, 100);
    if (result > percent) // meaning missed
      return IXMLNode::emptyIXMLNode;
  }


  // define names
  if (flightLegName.empty () || Utils::isElementExists (mapFlightPlanOrder_si, flightLegName)) // v3.0.241.9 moved this condition before rest of attributes, due to the fact their names are dependent on the leg name. That way, the name can be optional and the plugin
                                                                                               // will provide one. // v3.0.219.9 fix cases where designer uses same flight leg name in template by mistake // v3.0.219.11 use "mapLegOrder_si" instead of "setNames"
    flightLegName = std::string (mxconst::get_ELEMENT_LEG ()) + "_" + Utils::formatNumber<int> (flightLegCounter); // genFlightLegName;


  std::string triggerName               = (override_trigger_name_s.empty ()) ? "trigger_task_" + flightLegName : override_trigger_name_s; // v3.0.223.4 added override_trigger_s
  std::string objectiveName             = "obj_" + flightLegName;
  std::string taskName                  = (override_task_name_s.empty ()) ? "task_" + flightLegName : override_task_name_s; // v3.0.223.4 added override_task_name_s
  std::string message_triggerTargetName = "message_trigger_target_" + flightLegName;
  std::string triggerTargetMessage;



  // skip Leg if it depends on other FLight Leg that is not in the list (meaning - it was not constructed, maybe was skipped or was not valid)
  if (!attrib_dependsOn.empty () && !(Utils::isElementExists (mapFlightPlanOrder_si, attrib_dependsOn))) //
  {
    Log::logDebugBO ("[DEBUG buildFlightLeg] Skip Flight Leg " + flightLegName + ", reason: attribute depends_on point to a <leg> that does not exists: " + attrib_dependsOn + ".", true);
    return IXMLNode::emptyIXMLNode;
  }


  // set mandatory attributes for <leg> element
  Utils::xml_search_and_set_attribute_in_IXMLNode (xNewFlightLeg, mxconst::get_ATTRIB_NAME (), flightLegName, mxconst::get_ELEMENT_LEG ()); // set flight leg name
  Utils::xml_search_and_set_attribute_in_IXMLNode (xNewFlightLeg, mxconst::get_ATTRIB_NAME (), objectiveName, mxconst::get_ELEMENT_LINK_TO_OBJECTIVE ()); // set flight leg link_to_objective

  //// OBJECTIVE  ////
  // check xObjective validity and construct one if needed
  if (xLegObjective.isEmpty ())
  {
    RandomEngine::setError ("[random flightLeg objective] objective template MAPPING was missing, constructing dummy one.");
    xLegObjective = this->xObjectives.addChild (mxconst::get_ELEMENT_OBJECTIVE ().c_str ());
    xLegObjective.addAttribute (mxconst::get_ATTRIB_NAME ().c_str (), objectiveName.c_str ());
  }

  //// PARSE EXPECTED LOCATION  ////
  IXMLNode xExpectedLocation = xLegFromTemplate.getChildNode (mxconst::get_ELEMENT_EXPECTED_LOCATION ().c_str ());
  if (xExpectedLocation.isEmpty ())
  {
    RandomEngine::setError ("[random flightLeg location] Failed to find: " + mxconst::get_ELEMENT_EXPECTED_LOCATION () + ", while parsing Flight Leg: " + flightLegName + ". Please fix template.");
    return IXMLNode::emptyIXMLNode;
  }

  // test if the expected location is valid
  const int force_slope_i                      = Utils::readNodeNumericAttrib<int> (xExpectedLocation, mxconst::get_ATTRIB_FORCE_SLOPED_TERRAIN (), 0);
  const int force_level_terrain_i              = (force_slope_i == 0) ? Utils::readNodeNumericAttrib<int> (xExpectedLocation, mxconst::get_ATTRIB_FORCE_LEVELED_TERRAIN (), 0) : 0; // v3.0.253.9.1 force_slope_i has precedence over "force level terrain_i".
  missionx::data_manager::Max_Slope_To_Land_On = Utils::readNodeNumericAttrib<float> (xExpectedLocation, mxconst::get_ATTRIB_DESIGNER_MAX_SLOPE_TO_LAND (), mxconst::DEFAULT_MAX_SLOPE_TO_LAND_ON); // v3.0.253.9.1
  if (missionx::data_manager::Max_Slope_To_Land_On < 1.0) // v3.0.253.9.1
    missionx::data_manager::Max_Slope_To_Land_On = 1.0; // this is almost means to ignore any slope and just flag the target as land-able

  // v3.0.241.8
  missionx::RandomEngine::flag_force_template_distances_b = Utils::readBoolAttrib (xExpectedLocation, mxconst::get_ATTRIB_FORCE_TEMPLATE_DISTANCES_B (), false); // will be used in get_target() function to disable the "expected distance setup option".


  /////////////////////////////////////////////////////////////////
  ////// SET SHARED INFORMATION/ELEMENTS to ALL <leg> types //////
  ///////////////////////////////////////////////////////////////


  ////////////////////////////////////////////
  // Fail if LOCATION_TYPE is empty

  if (Utils::readAttrib (xExpectedLocation, mxconst::get_ATTRIB_LOCATION_TYPE (), "").empty ())
  {
    RandomEngine::setError ("[random flightLeg location] Fail to find: " + mxconst::get_ATTRIB_LOCATION_TYPE () + ", while parsing template Flight Leg: " + flightLegName + ". Please fix the template.");
    return IXMLNode::emptyIXMLNode;
  }


  auto        location_radius_mt  = Utils::readNodeNumericAttrib<int> (xExpectedLocation, mxconst::get_ATTRIB_RADIUS_MT (), 0); // v3.0.241.8 added support for custom target radius. This for hover cases.
  std::string location_type       = Utils::readAttrib (xExpectedLocation, mxconst::get_ATTRIB_LOCATION_TYPE (), "");
  // v25.08.1 support for "location_properties" attribute that will replace "location_value"
  std::string location_value_nm_s = Utils::readAttrib (xExpectedLocation, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), mxconst::get_ATTRIB_LOCATION_VALUE (),  "", true);

  std::string location_value_restrict_ramp_type_s; // v3.0.221.7 will hold a special ramp type if location value has special string characters after the numbers

  // location value format can be: "{number}|{ramp type}|{min-max},..."
  // mapLocationValueOptions: {name},{value}.
  std::map<int, std::string> mapLocationValueOptions; // v3.0.221.7 will hold the complex options used by "|".
  std::string                location_value_min_max_distance_s, location_value_tag_name_s, location_value_poi_s; // v3.0.221.7 @Daikan used in Random airport pick. "location_value_tag_name_s" will be used to hold element name to search in template.


  //////////////////////////////////
  //// SPECIAL Leg element ////
  ////////////////////////////////
  // Read special directive element
  IXMLNode xSpecialLegDirectives = in_legNodeFromTemplate.getChildNode (mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES ().c_str ()); // v3.0.221.15rc5 support LEG // v3.0.221.9 check if <special... element already exists
  if (xSpecialLegDirectives.isEmpty ())
    xSpecialLegDirectives = xNewFlightLeg.addChild (mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES ().c_str ()); // v3.0.241.8 removed backward support // v3.0.221.8 create special element if not exists // v3.0.221.15rc5 support LEG

  xNewFlightLeg.addChild (xSpecialLegDirectives); // v3.0.221.9 add element if exists

  //////////////////////////////////
  //// TASK SHARED INFORMATION ////
  ////////////////////////////////
  /// TASK - Check xTask

  if (xLegTask.isEmpty () && !skip_auto_task_creation_b)
  {
    xLegTask = xLegObjective.addChild (mxconst::get_ELEMENT_TASK ().c_str ());
  }

  // set Task name
  if (!Utils::xml_search_and_set_attribute_in_IXMLNode (xLegTask, mxconst::get_ATTRIB_NAME (), taskName, mxconst::get_ELEMENT_TASK ()))
  {
    xLegTask.addAttribute (mxconst::get_ATTRIB_NAME ().c_str (), taskName.c_str ());

    Log::logMsgWarn ("[random task]Task node had missing attribute. Added 'name' attribute. Suggest: check your mapping definitions.", true);
  }

  IXMLNode xSpecialTasks;
  IXMLNode xSpecialMessages;
  if (!xSpecialLegDirectives.isEmpty ())
  {
    const std::string attrib_base_on_external_plugin_value_s = Utils::readAttrib (xSpecialLegDirectives, mxconst::get_ATTRIB_BASE_ON_EXTERNAL_PLUGIN (), mxconst::get_MX_FALSE ());
    xLegTask.updateAttribute (attrib_base_on_external_plugin_value_s.c_str (), mxconst::get_ATTRIB_BASE_ON_EXTERNAL_PLUGIN ().c_str (), mxconst::get_ATTRIB_BASE_ON_EXTERNAL_PLUGIN ().c_str ());

    // search for special trigger in template
    const std::string use_trigger_from_template = Utils::readAttrib (xSpecialLegDirectives, mxconst::get_ATTRIB_USE_TRIGGER_NAME_FROM_TEMPLATE (), ""); // v3.0.221.10 should hold the attribute name of the trigger element in the template <trigger name="{name}" >
    if (!use_trigger_from_template.empty ())
    {
      IXMLNode tNode = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (xRootTemplate, mxconst::get_ELEMENT_TRIGGER (), mxconst::get_ATTRIB_NAME (), use_trigger_from_template);
      if (!tNode.isEmpty ())
        xLegTargetTrigger = tNode;
    }

    // search for set of tasks to add to the FlightLeg objective
    const std::string add_tasks_from_template = Utils::readAttrib (xSpecialLegDirectives, mxconst::get_ATTRIB_ADD_TASKS_FROM_TEMPLATE (), ""); // v3.0.221.10 should hold the tag name of the tasks we want to add to one of the flight legs.
    if (!add_tasks_from_template.empty ())
      xSpecialTasks = xRootTemplate.getChildNode (add_tasks_from_template.c_str ()); // if we find an element with this name, we will add all sub elements tasks to objective

    // search for set of messages to add to xDescText
    const std::string add_messages_from_template = Utils::readAttrib (xSpecialLegDirectives, mxconst::get_ATTRIB_ADD_MESSAGES_FROM_TEMPLATE (), ""); // v3.0.221.10 should hold the tag name of the messages we want to xMessages
    if (!add_messages_from_template.empty ())
      xSpecialMessages = xRootTemplate.getChildNode (add_messages_from_template.c_str ()); // if we find an element with this name, we will add all sub elements to xMessages
  }

  if (skip_auto_task_creation_b) // v3.0.221.15 rc4 validate that we have an alternative task (in special <leg> directive element)
  {
    xLegTask = xSpecialTasks.getChildNode (mxconst::get_ELEMENT_TASK ().c_str ());
    if (xLegTask.isEmpty ())
    {
      Log::logMsgErr ("[random task] Could not find alternative special task that will replace the mandatory task that was skipped cause of the use of: skip_auto_task_creation_b attribute. Skipping <leg> creation.", true);
      return IXMLNode::emptyIXMLNode;
    }
  }

  // set base on trigger
  if (!Utils::xml_search_and_set_attribute_in_IXMLNode (xLegTask, mxconst::get_ATTRIB_BASE_ON_TRIGGER (), triggerName, mxconst::get_ELEMENT_TASK ()))
  {
    xLegTask.updateAttribute (triggerName.c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str ());

    Log::logMsgWarn ("[random task]Task node had missing attribute. Added 'base_on' attribute. Suggest: check your mapping definitions.", true);
  }
  //// END TASK INFORMATION ////

  ////////////////////////////////////////////////////////
  // set as mandatory (no need part of template mapping )
  //// SET SHARED OBJECTIVE INFORMATION               ////
  ///////////////////////////////////////////////////////
  if (!Utils::xml_search_and_set_attribute_in_IXMLNode (xLegObjective, mxconst::get_ATTRIB_NAME (), objectiveName, mxconst::get_ELEMENT_OBJECTIVE ()))
  {
    RandomEngine::setError ("[random objective]Failed to set objective property: '" + mxconst::get_ATTRIB_NAME () + "'");
    return IXMLNode::emptyIXMLNode;
  }

  // add task to objective
  xLegObjective.addChild (xLegTask);

  // v3.0.221.10 add all sub specials elements to the <leg> element.
  if (!xSpecialLegDirectives.isEmpty ())
  {
    // Add pre-defined tasks
    if (!xSpecialTasks.isEmpty ())
    {
      Utils::add_xml_comment (xLegObjective, " +++ Added custom Task +++ ");
      int nChilds = xSpecialTasks.nChildNode (mxconst::get_ELEMENT_TASK ().c_str ());
      for (int i1 = (skip_auto_task_creation_b) ? 1 : 0; i1 < nChilds; ++i1) // v3.0.221.15 rc4 if skip_auto_task_creation_b=true then add second task onward, if not then from first. Reason: when skip_auto_task_creation_b is true, then
                                                                             // first task becomes the mandatory one that will use the special trigger
      {
        IXMLNode cNode = xSpecialTasks.getChildNode (mxconst::get_ELEMENT_TASK ().c_str (), i1);
        if (!cNode.isEmpty ())
          xLegObjective.addChild (cNode.deepCopy ());
      }
      Utils::add_xml_comment (xLegObjective, " +++ End custom Task +++ ");
    }

    // Add pre-defined messages
    if (!xSpecialMessages.isEmpty ())
    {
      Utils::add_xml_comment (xMessages, " +++ Added custom message +++ ");

      const int nChilds = xSpecialMessages.nChildNode (mxconst::get_ELEMENT_MESSAGE ().c_str ());
      for (int i1 = 0; i1 < nChilds; ++i1)
      {
        const IXMLNode cNode = xSpecialMessages.getChildNode (mxconst::get_ELEMENT_MESSAGE ().c_str (), i1);
        if (!cNode.isEmpty ())
        {
          xMessages.addChild (cNode.deepCopy ());
        }
      }

      Utils::add_xml_comment (xMessages, " +++ End custom message +++ ");
    }

  } // end adding sub elements from special


  // add objective to its main element: <objectives>
  xObjectives.addChild (xLegObjective);

  /////// END OBJECTIVE Settings /////////

  ///////////////////////////////////
  // set TRIGGER shared properties /
  //////////////////////////////////

  if (!Utils::xml_search_and_set_attribute_in_IXMLNode (xLegTargetTrigger, mxconst::get_ATTRIB_NAME (), triggerName, mxconst::get_ELEMENT_TRIGGER ()))
  {
    RandomEngine::setError ("[random trigger]Failed to set trigger property: '" + mxconst::get_ATTRIB_NAME () + "'");
    return IXMLNode::emptyIXMLNode;
  }

  // v3.0.221.13 Check if trigger has outcome and add if not
  if (xLegTargetTrigger.getChildNode (mxconst::get_ELEMENT_OUTCOME ().c_str ()).isEmpty ())
    xLegTargetTrigger.addChild (mxconst::get_ELEMENT_OUTCOME ().c_str ());


  ///////////////////////////////////
  // READ commands from leg template // v3.0.221.9
  //////////////////////////////////
  IXMLNode xFireCommandsAtLegBegin = xLegFromTemplate.getChildNode (mxconst::get_ELEMENT_FIRE_COMMANDS_AT_LEG_START ().c_str ());
  IXMLNode xFireCommandsAtLegEnd   = xLegFromTemplate.getChildNode (mxconst::get_ELEMENT_FIRE_COMMANDS_AT_LEG_END ().c_str ());

  xNewFlightLeg.addChild (xFireCommandsAtLegBegin);
  xNewFlightLeg.addChild (xFireCommandsAtLegEnd);

  ///////////////////////////////////
  // Add weather element if exists
  //////////////////////////////////

  // v3.303.12
  IXMLNode xWeather = xLegFromTemplate.getChildNode (mxconst::get_ELEMENT_WEATHER ().c_str ());
  if (!xWeather.isEmpty ())
    xNewFlightLeg.addChild (xWeather.deepCopy ());


  /////////////////////////////////////////////////////////////////
  ////// END SHARED INFORMATION/ELEMENTS to ALL <leg> types //////
  ///////////////////////////////////////////////////////////////

  ///////////// CHECK if Flight Leg TYPE needs to be Randomized ////////////////
  if (flight_leg_type_hover_land_or_start.empty ())
  {
    RandomEngine::setError ("Found a <leg> template without type definition. skipping.");
    return IXMLNode::emptyIXMLNode;
  }

  // v3.0.219.3 - support for multi-Leg type to pick
  if (flight_leg_type_hover_land_or_start.find (mxconst::get_COMMA_DELIMITER ()) != std::string::npos) // mxconst::get_COMMA_DELIMITER() = ","
  {
    std::vector<std::string> vecTypes = mxUtils::split_v2 (flight_leg_type_hover_land_or_start, mxconst::get_COMMA_DELIMITER ()); // mxconst::get_COMMA_DELIMITER() = ","
    if (int nTypes = static_cast<int> (vecTypes.size ()); nTypes == 0)
    {
      RandomEngine::setError ("Found a <leg> template without type definition. skipping.");
      return IXMLNode::emptyIXMLNode;
    }
    else if (nTypes == 1)
    {
      flight_leg_type_hover_land_or_start = vecTypes.at (0);
    }
    else // random pick type
    {
      int picked = Utils::getRandomIntNumber (0, nTypes - 1);
      if (picked > nTypes)
        picked = nTypes - 1;

      flight_leg_type_hover_land_or_start = vecTypes.at (picked);
    }
  }


  ///////////// CHECK if LOCATION TYPE need to be Randomized ////////////////
  // v3.0.219.3 - support for multi-location vecTypeValues type to pick
  if (location_type.find (mxconst::get_COMMA_DELIMITER ()) != std::string::npos) // mxconst::get_COMMA_DELIMITER() = ","
  {
    std::vector<std::string> vecTypes      = mxUtils::split_v2 (location_type, mxconst::get_COMMA_DELIMITER ()); // mxconst::get_COMMA_DELIMITER() = ","
    std::vector<std::string> vecTypeValues = mxUtils::split_v2 (location_value_nm_s, mxconst::get_COMMA_DELIMITER ()); // mxconst::get_COMMA_DELIMITER() = ","
    int                      picked        = 0;

    int       nTypes       = static_cast<int> (vecTypes.size ());
    const int nTypesValues = static_cast<int> (vecTypeValues.size ());

    Log::logDebugBO ("[DEBUG pick <leg> type & value] vecTypes: " + Utils::formatNumber<size_t> (vecTypes.size ()) + ", values:" + Utils::formatNumber<size_t> (vecTypeValues.size ()), true);

    if (nTypes == 0)
    {
      RandomEngine::setError ("Found a location type with wrong definition. skipping.");
      return IXMLNode::emptyIXMLNode;
    }
    else if (nTypes == 1)
    {
      location_type = vecTypes.at (0);
      picked        = 0; // meaning first choice
    }
    else // random pick type
    {
      picked = Utils::getRandomIntNumber (0, nTypes - 1);
      if (picked > nTypes)
        picked = nTypes - 1;

      location_type = vecTypes.at (picked);
    }


    Log::logDebugBO ("[DEBUG pick <leg> type] type picked: " + location_type, true);

    //// pick the relative value too. picked can't be bigger than the number of types
    if (nTypesValues >= nTypes || (picked < (nTypesValues - 1)))
      location_value_nm_s = vecTypeValues.at (picked);
    else if (nTypes >= 1 && nTypesValues == 1) // if we have few Types but only 1 location_value_nm_s, then it is shared between all of them
      location_value_nm_s = vecTypeValues.front ();
    else
      location_value_nm_s.clear ();


    if (location_value_nm_s == "_") // if special character that represent empty
      location_value_nm_s.clear ();
  }


  #ifndef RELEASE
  Log::logDebugBO ("[DEBUG random location info] location_type: " + location_type + ", location_value_nm_s=" + location_value_nm_s, true);
  #endif


  std::map<std::string, std::string> mapLocationSplitValues;
  std::vector<std::string>           vecLocationValueSplit_vec;

  ///////////// CHECK if <leg> TYPE overrides location values ////////////////
  /// handle special cases where flight_leg_type (attrib template) is "start"
  if ((mxconst::get_FL_TEMPLATE_VAL_START () == flight_leg_type_hover_land_or_start) || (location_type == mxconst::get_FL_TEMPLATE_VAL_START ())) // v3.0.221.15 consolidate if logic to one  // v3.0.221.7
  {
    flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_START ();
    location_type                       = mxconst::get_FL_TEMPLATE_VAL_START ();
    location_value_nm_s                 = mxconst::get_FL_TEMPLATE_VAL_START ();
  }
  ////////// Check if has special instructions like: "nm=20|ramp=H|nm_between=10-20|tag={some name}"
  else if (!location_value_nm_s.empty ())
  {
    //// v3.0.221.7 replace old logic with new more readable one
    // split between numbers and characters
    vecLocationValueSplit_vec = mxUtils::split_v2 (location_value_nm_s, mxconst::get_PIPE_DELIMITER ()); // "|"

    for (const auto &v : vecLocationValueSplit_vec)
    {
      std::vector<std::string> vecSplit = mxUtils::split_v2 (v, "=");
      if (auto size_i = vecSplit.size (); size_i == 1) // for backwards compatibility, we handle cases where designer use: location_value="10 or _ or {some tag name}" without useing the explicit format
      {
        std::string attribName = Utils::stringToLower (vecSplit.at (0));
        std::string attribValue;

        // try to keep backwards compatibility if only number has been given and not: nm={number}
        if (Utils::is_number (attribName))
        {
          attribValue = attribName;
          attribName  = "nm";
        }
        else if (attribName == "_" || attribName.empty ()) // handle special cases when only "_" is set or attribName is empty
        {
          attribValue = "_";
          attribName  = "nm";
        }
        else
        {
          attribValue = attribName;
          attribName  = "tag";
        }

        Log::logMsgErr (std::string ("[random buildLeg] Found location value without explicit formating. Original format: ").append (location_value_nm_s).append (". Should be: ").append (attribName).append ("=") + attribValue + ". Skipping this directive.", true);
      }
      else if (size_i > 1)
      {
        std::string        attribName  = Utils::stringToLower (vecSplit.at (0));
        const std::string &attribValue = vecSplit.at (1);
        Utils::addElementToMap (mapLocationSplitValues, attribName, attribValue);
      }
      else
        location_value_nm_s.clear ();
    }

    location_value_nm_s.clear (); // ??? bug ??? When clearing the value we ignore the cases where it holds "tag=xxx" data and not any "nm=". This can fail get_target() function

    // prepare local variables according to the split information
    if (Utils::isElementExists (mapLocationSplitValues, "nm")) // represent distance in nm
      location_value_nm_s = mapLocationSplitValues["nm"];

    // replace "_" with empty string
    if (location_value_nm_s == "_") // v3.0.221.7 if special character that represent empty
      location_value_nm_s.clear ();
  }

  Log::logDebugBO ("[DEBUG random location info] location_value_nm_s=" + location_value_nm_s, true);

  //////////////////////////////////////////////////
  ///////// Get TARGET            /////////////////
  // create the objective based on <leg> type
  std::string radius_mt = (location_radius_mt < this->RADIUS_MT_MINIMUM_LENGTH) ? "" : Utils::formatNumber<int> (location_radius_mt); // v3.0.241.8 initialize with the override value designer decided.
  NavAidInfo  newNavInfo; // v3.0.241.8 deprecated "prevNavInfo"; // v3.0.221.2 moved outside of code block
  IXMLNode    xPoint = xNewFlightLeg.addChild (mxconst::get_ELEMENT_POINT ().c_str ());
  // prepare dummy point
  if (xPoint.isEmpty ())
  {
    RandomEngine::setError ("[random flightLeg point] fail to add point element to flight leg. skipping <leg> creation.");
    return IXMLNode::emptyIXMLNode;
  }



  // ============= GET TARGET ===============
  // ============= GET TARGET ===============
  // ============= GET TARGET ===============
  // ============= GET TARGET ===============
  // ============= GET TARGET ===============



  const mx_which_type_to_force which_type_to_force_enum = (force_slope_i > 0) ? mx_which_type_to_force::force_hover : (force_level_terrain_i > 0) ? mx_which_type_to_force::force_flat_terrain_to_land : mx_which_type_to_force::no_force_is_needed;
  const int                    how_many_times_to_loop_i = (which_type_to_force_enum == mx_which_type_to_force::force_hover) ? force_slope_i : (which_type_to_force_enum == mx_which_type_to_force::force_flat_terrain_to_land) ? force_level_terrain_i : 0;

  // v3.0.221.15rc3 read new flag: "force <leg> type" at <leg> template level.
  bool flag_force_flight_leg_type = Utils::readBoolAttrib (xLegFromTemplate, mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B (), false); // relevant only in case we use "tag_name" and we pick <points> from it. Check if we have to force flight_leg_type on the random point that we might pick.

  // v25.06.1

  if (bool flag_is_surprise_osm_leg = Utils::readBoolAttrib (xLegFromTemplate, mxconst::get_ATTRIB_SURPRISE_ME_SUB_CAT_B (), false))
  { // v25.06.1
    newNavInfo.init ();

    newNavInfo.lat = Utils::readNodeNumericAttrib <float>(xLegFromTemplate, mxconst::get_ATTRIB_LAT (), 0.0f);
    newNavInfo.lon = Utils::readNodeNumericAttrib <float>(xLegFromTemplate, mxconst::get_ATTRIB_LONG (), 0.0f);
    newNavInfo.setName ( Utils::readAttrib (xLegFromTemplate, mxconst::get_ATTRIB_NAME (), "") );
    newNavInfo.setID ( Utils::readAttrib (xLegFromTemplate, mxconst::get_ATTRIB_ICAO_ID (), "") );
    newNavInfo.flag_nav_from_webosm = true;

    newNavInfo.synchToPoint ();
    xPoint = newNavInfo.node.deepCopy ();
    if (xPoint.isEmpty ())
    {
      RandomEngine::setError ("[random build <leg>]Failed to set Point coordinates. skipping <leg>: " + flightLegName);
      return IXMLNode::emptyIXMLNode;
    }

    flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_LAND ();

  }
  else
  {
    // get dynamic Target from the "get_target()" function
    int loop_counter_i = 0;
    // v3.0.221.15 rc3.2
    missionx::mx_base_node targetProp; // v3.305.1
    targetProp.setStringProperty (mxconst::get_ATTRIB_NAME (), flightLegName); // leg name
    targetProp.setStringProperty (mxconst::get_ATTRIB_TYPE (), flight_leg_type_hover_land_or_start); // leg type
    targetProp.setStringProperty (mxconst::get_ATTRIB_LOCATION_TYPE (), location_type); // location type
    targetProp.setBoolProperty (mxconst::get_PROP_IS_LAST_FLIGHT_LEG (), this->flag_isLastFlightLeg); // is last flight leg ?
    targetProp.setBoolProperty (mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B (), flag_force_flight_leg_type); // force leg type ?
    targetProp.setNodeProperty<int> (mxconst::get_ATTRIB_FORCE_TYPE_OF_TEMPLATE (), static_cast<int> (which_type_to_force_enum)); // force level terrain or slope ?
    targetProp.setNodeProperty<int> (mxconst::get_PROP_NUMBER_OF_LOOPS_TO_FORCE_TYPE_TEMPLATE (), how_many_times_to_loop_i); // force slope will be used with webosm

    do
    {
      if (!this->get_target (newNavInfo, in_legNodeFromTemplate, missionx::RandomEngine::template_plane_type_enum, mapLocationSplitValues, targetProp))
      {
        return IXMLNode::emptyIXMLNode; // error message should have been set in get_target() function
      }
      // v3.0.253.6 check abort
      if (missionx::RandomEngine::threadState.flagAbortThread)
        return IXMLNode::emptyIXMLNode;

      // Check target duplication
      if (RandomEngine::listNavInfo.size () > static_cast<size_t> (1) && missionx::RandomEngine::check_if_new_target_is_same_as_prev (newNavInfo, RandomEngine::listNavInfo.back ()))
      {
        // skip target creation and try again.
        Log::logMsgThread (fmt::format ("[RandomEngine] Found target [{}] same as previous one [{}]. Will skip and try a new one.", newNavInfo.getID (), listNavInfo.back ().getID ()));
        newNavInfo.init (); // reset Nav Info
      }
      else // navaid is valid and not same as previous one
      {
        newNavInfo.synchToPoint (true); // v25.09.2 force init desc // v3.0.241.10 b3 // hopefully will solve the crash when creating GPS
        xPoint = newNavInfo.node.deepCopy (); // v3.0.221.15 rc3.2
        if (xPoint.isEmpty ())
        {
          RandomEngine::setError ("[random build <leg>]Failed to set Point coordinates. skipping <leg>: " + flightLegName);
          return IXMLNode::emptyIXMLNode;
        }

        // TODO: Consider removing the force directive since we will allow land + hover
        if (which_type_to_force_enum != mx_which_type_to_force::no_force_is_needed)
        {
          expected_slope_at_target_location_d = get_slope_at_point (newNavInfo);
          #ifndef RELEASE
          Log::logMsgThread ("[force_slope] Slope Result: " + Utils::formatNumber<double> (expected_slope_at_target_location_d));
          #endif

          if (which_type_to_force_enum == mx_which_type_to_force::force_hover && expected_slope_at_target_location_d > missionx::data_manager::Max_Slope_To_Land_On)
          {
            flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_HOVER ();
            #ifndef RELEASE
            Log::logMsgThread ("[force_slope] Found slope in landing area: " + Utils::formatNumber<double> (expected_slope_at_target_location_d));
            #endif
            break;
          }
          else if (which_type_to_force_enum == mx_which_type_to_force::force_flat_terrain_to_land && expected_slope_at_target_location_d <= missionx::data_manager::Max_Slope_To_Land_On)
          {
            flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_LAND ();
            #ifndef RELEASE
            Log::logMsgThread ("[force_level] Found landing area: " + Utils::formatNumber<double> (expected_slope_at_target_location_d));
            #endif
            break;
          }
          else
          {
            Log::logMsgThread ("[force_slope] Failed Slope test. Will try to fetch another target....");
          }
        }
      }
    } while (++loop_counter_i < how_many_times_to_loop_i && how_many_times_to_loop_i < 10); // end loop over force slope. Currently only 10 times are allowed

  } // end if the Leg is a surprise me type.

  // v3.0.253.9.1 fail flight leg if force failed
  if (which_type_to_force_enum == mx_which_type_to_force::force_hover && flight_leg_type_hover_land_or_start != mxconst::get_FL_TEMPLATE_VAL_HOVER ())
  {
    Log::logMsgThread ("[force_slope] Failed finding Slope in target area. Will fail current Flight Leg");
    return IXMLNode::emptyIXMLNode;
  }
  else if (which_type_to_force_enum == mx_which_type_to_force::force_flat_terrain_to_land && flight_leg_type_hover_land_or_start != mxconst::get_FL_TEMPLATE_VAL_LAND ())
  {
    Log::logMsgThread ("[force_slope] Failed Finding Landing terrain. Will fail current Flight Leg");
    return IXMLNode::emptyIXMLNode;
  }


  // v3.0.241.8 Build basic target message when entering its area. It could be changed if it is not the last flight leg, and we found out that we need to hover.
  if (this->flag_isLastFlightLeg) // v3.0.241.8 different message to end flight leg
    triggerTargetMessage = "You reached the end of this flight."; // should be sent at the last target location.
  else
    triggerTargetMessage = "You reached the target area of this flight leg.";



  // 1. Check SLOPE for LAND leg type and WATER body that will affect flight_leg_type. Then check special point directives like "template" attribute that should override the "flight leg" template
  if (flight_leg_type_hover_land_or_start == mxconst::get_FL_TEMPLATE_VAL_START () || this->flag_isLastFlightLeg)
  {
    // v25.09.2 [bug] <point> was not added to the target trigger.
    const bool b_node_was_added = Utils::xml_add_node_to_element_IXMLNode (xLegTargetTrigger, xPoint, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ()); // Target point shared between all flight_leg_type
  }
  else //if ((flight_leg_type_hover_land_or_start != mxconst::get_FL_TEMPLATE_VAL_START ()) || (!this->flag_isLastFlightLeg))
  {
    // ---------------------------------------
    // 1. check SLOPE if no navaid name
    // ---------------------------------------
    expected_slope_at_target_location_d = get_slope_at_point (newNavInfo); // v3.0.253.6 use the new wrapper function for that so code will be cleaner
    RandomEngine::errMsg.clear ();

    if (force_slope_i > 0 && expected_slope_at_target_location_d < missionx::data_manager::Max_Slope_To_Land_On) // v3.0.253.6
    {
      Log::logMsgThread ("[force_slope] Failed Slope test. Will fail current Flight Leg");
      return IXMLNode::emptyIXMLNode;
    }

    // v3.0.223.2 check target slope only if template is LAND, and new Navinfo is empty, and we did not force template
    if ((flight_leg_type_hover_land_or_start == mxconst::get_FL_TEMPLATE_VAL_LAND ()) && newNavInfo.getID ().empty () && !newNavInfo.flag_force_picked_same_point_template_as_flight_leg_template_type)
    {
      Log::logDebugBO ("[DEBUG random slope] slope value: " + Utils::formatNumber<double> (expected_slope_at_target_location_d, 2), true);

      // define Leg type based on restricted slope
      if (expected_slope_at_target_location_d > missionx::data_manager::Max_Slope_To_Land_On)
      {
        flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_HOVER ();
        #ifndef RELEASE
        Log::logMsgWarn (fmt::format ("[random asses slope] Changed <leg> type to Hover due to slope being larger than: {}. Found slope in landing area: {}", Utils::formatNumber<float> (missionx::data_manager::Max_Slope_To_Land_On), Utils::formatNumber<double> (expected_slope_at_target_location_d)), true);
        #endif
      }

      Log::logDebugBO ("[DEBUG buildFLightLeg] location: " + location_type + ", After slope decision", true);
    }


    // ---------------------------------------
    // 2. check WET
    // ---------------------------------------
    const bool isWet = this->get_is_wet_at_point (newNavInfo);
    if (!isWet)
    {
      RandomEngine::setError ("[random isWet] Failed to probe for wet. Will treat target coordinates as \"land\". ");
    }

    if (isWet)
    {
      // implement special case where user defined medevac + prop plane || float plane.
      auto plane_type_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types::plane_type_props)); // plane type

      xNewFlightLeg.updateAttribute ("true", mxconst::get_PROP_IS_WET ().c_str (), mxconst::get_PROP_IS_WET ().c_str ());
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_IS_WET (), isWet); // v3.0.241.9 store if we have wet location
      // v3.0.241.9 extended ui design mission to allow ga planes to land in water without forcing hover
      if (flight_leg_type_hover_land_or_start != mxconst::get_FL_TEMPLATE_VAL_HOVER () && !(this->flag_rules_defined_by_user_ui && (plane_type_i >= static_cast<int> (missionx::mx_plane_types::plane_type_props) && plane_type_i <= static_cast<int> (missionx::mx_plane_types::plane_type_ga_floats)))) // v3.0.223.2 will change to hover only if not using "force same template type"
      {
        if (!newNavInfo.flag_force_picked_same_point_template_as_flight_leg_template_type)
          flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_HOVER ();
      }
    }

    // ---------------------------------------
    // 3. Post <point> - construct template type/location description and radius of effect
    // ---------------------------------------
    if (!newNavInfo.flag_force_picked_same_point_template_as_flight_leg_template_type) // v3.0.223.2 if we already picked based on same template then is no room for this test
    {
      // read template type from point and apply to NavAid and Leg node
      std::string pointTemplateType = Utils::stringToLower (Utils::readAttrib (xPoint, mxconst::get_ATTRIB_TEMPLATE (), "")); // v3.0.219.3 this will affect the flight leg type
      if (!pointTemplateType.empty () && ((mxconst::get_FL_TEMPLATE_VAL_LAND () == pointTemplateType) || (mxconst::get_FL_TEMPLATE_VAL_HOVER () == pointTemplateType)) && flight_leg_type_hover_land_or_start != pointTemplateType) // v3.0.219.3 override <leg> type only if different from current leg type
      {
        #ifndef RELEASE
        Log::logMsgWarn ("[random: asses Hover or Land]Forced point flight leg type from: '" + flight_leg_type_hover_land_or_start + "' to: '" + pointTemplateType + "'", true);
        #endif
        flight_leg_type_hover_land_or_start = pointTemplateType; // override flight leg type with suggestion in point
        xNewFlightLeg.updateAttribute (flight_leg_type_hover_land_or_start.c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE ().c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE ().c_str ());
      }
    }

    radius_mt = Utils::readAttrib (xPoint, mxconst::get_ATTRIB_RADIUS_MT (), radius_mt);

    // v25.02.1 add point to target trigger - moved before the LAND_HOVER and HOVER logic
    const bool b_node_was_added = Utils::xml_add_node_to_element_IXMLNode (xLegTargetTrigger, xPoint, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ()); // Target point shared between all flight_leg_type

    // ---------------------------------------
    // Handle LAND_HOVER template
    // ---------------------------------------
    // v25.02.1
    // Gather information from UI layer
    const auto med_cargo_or_oilrig_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::not_defined)); // 0 = med, 1 = cargo

    #ifndef RELEASE
    auto bIsMandatory_debug = Utils::readBoolAttrib (xLegTask, mxconst::get_ATTRIB_MANDATORY (), false);
    auto uiLayer_debug      = data_manager::getGeneratedFromLayer ();
    auto plane_type         = missionx::RandomEngine::template_plane_type_enum;
    #endif

    // Is this a user generated mission ? For now only "user generated" screen supports the "land_hover".
    if (inFlightLegCounter == 1 // first leg, is also the target of the "flight leg"
        && med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::medevac)
        && missionx::RandomEngine::template_plane_type_enum == missionx::mx_plane_types::plane_type_helos
        && data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_user_generates_a_mission_layer
        && Utils::readBoolAttrib (xLegTask, mxconst::get_ATTRIB_MANDATORY (), false) // must be mandatory
    )
    {
      flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ();
      xNewFlightLeg.updateAttribute (flight_leg_type_hover_land_or_start.c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE ().c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE ().c_str ());
      xLegTask.updateAttribute (flight_leg_type_hover_land_or_start.c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE ().c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE ().c_str ());

      const auto s_land_task_name = Utils::readAttrib (xLegTask, mxconst::get_ATTRIB_NAME (), "");

      // get target lat/lon
      int seq = 0;
      // calculate <display_object>, using 350 meters with 24 3D objects.
      auto vecLandTarget = missionx::RandomEngine::gen_land_hover_display_objects (newNavInfo.lat, newNavInfo.lon, 350, 24, seq);
      if (!vecLandTarget.empty ())
      {
        Utils::xml_add_comment (xNewFlightLeg, "<<< Land 3D hint >>>");
        for (auto &xml : vecLandTarget)
          xNewFlightLeg.addChild (xml.deepCopy ());
      }
      vecLandTarget.clear ();
      vecLandTarget = missionx::RandomEngine::gen_land_hover_display_objects (newNavInfo.lat, newNavInfo.lon, 50, 4, seq);
      if (!vecLandTarget.empty ())
      {
        Utils::xml_add_comment (xNewFlightLeg, "<<< Hover 3D hint >>>");
        for (auto &xml : vecLandTarget)
          xNewFlightLeg.addChild (xml.deepCopy ());
      }
      Utils::xml_add_comment (xNewFlightLeg, "<<< --------------- >>>");
      // end <display_object>

      auto xHoverTask    = xLegTask.deepCopy ();
      auto xHoverTrigger = xLegTargetTrigger.deepCopy ();
      // set new names
      const auto hover_task_name    = fmt::format ("leg_{}_hover_task", inFlightLegCounter);
      const auto hover_trigger_name = fmt::format ("leg_{}_trig_hover", inFlightLegCounter);

      // set hover task info
      xHoverTask.updateAttribute (hover_task_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
      xHoverTask.updateAttribute ("true", mxconst::get_ATTRIB_MANDATORY ().c_str (), mxconst::get_ATTRIB_MANDATORY ().c_str ());
      xHoverTask.updateAttribute (hover_trigger_name.c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str ());
      // set hover trigger info
      xHoverTrigger.updateAttribute (hover_trigger_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
      xHoverTrigger.updateAttribute (mxconst::get_TRIG_TYPE_RAD ().c_str (), mxconst::get_ATTRIB_TYPE ().c_str (), mxconst::get_ATTRIB_TYPE ().c_str ());
      Utils::xml_set_attribute_in_node<bool> (xHoverTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND (), false, mxconst::get_ELEMENT_CONDITIONS ());
      Utils::xml_set_attribute_in_node_asString (xHoverTrigger, mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT (), fmt::format ("---{}", mxconst::get_DEFAULT_HOVER_HEIGHT_FT ()), mxconst::get_ELEMENT_ELEVATION_VOLUME ());
      Utils::xml_set_attribute_in_node_asString (xHoverTrigger, mxconst::get_ATTRIB_LENGTH_MT (), fmt::format ("{}", 60), mxconst::get_ELEMENT_RADIUS ());


      // set the success for the crosscheck tasks, since both of them are mandatory.
      Utils::xml_set_attribute_in_node_asString (xHoverTrigger, mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS (), s_land_task_name, mxconst::get_ELEMENT_OUTCOME ());
      Utils::xml_set_attribute_in_node_asString (xLegTargetTrigger, mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS (), hover_task_name, mxconst::get_ELEMENT_OUTCOME ());
      // set the landing trigger radius
      Utils::xml_set_attribute_in_node_asString (xLegTargetTrigger, mxconst::get_ATTRIB_LENGTH_MT (), fmt::format ("{}", 500), mxconst::get_ELEMENT_RADIUS ());

      // Add the hover task and trigger to the template output
      xLegObjective.addChild (xHoverTask);
      this->xTriggers.addChild (xHoverTrigger, xTriggers.nChildNode ());
    }
    //////////////////
    //// Handle HOVER
    else if (mxconst::get_FL_TEMPLATE_VAL_HOVER () == flight_leg_type_hover_land_or_start)
    {
      if (location_radius_mt < this->RADIUS_MT_MINIMUM_LENGTH)
        radius_mt = (radius_mt == "500") ? radius_mt : "200"; //


      IXMLNode elevVolNode_ptr = Utils::xml_get_node_from_node_tree_IXMLNode (xLegTargetTrigger, mxconst::get_ELEMENT_ELEVATION_VOLUME (), false); // pointer to node
      if (elevVolNode_ptr.isEmpty ())
      {
        RandomEngine::setError ("[random hover] Trigger template does not have <elevation_volume> element. Please fix.");
        return IXMLNode::emptyIXMLNode;
      }
      Utils::xml_search_and_set_attribute_in_IXMLNode (xLegTargetTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND (), mxconst::get_MX_FALSE (), mxconst::get_ELEMENT_CONDITIONS ());
      const std::string elev_lower_upper_ft = fmt::format("---{}", mxconst::get_DEFAULT_HOVER_HEIGHT_FT ());

      elevVolNode_ptr.updateAttribute (elev_lower_upper_ft.c_str (), mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT ().c_str (), mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT ().c_str ()); // updating using the XML api do not work somehow. Should investigate.

      ////// try to read hover data entered by designer in <special_leg_directives>  ///////////////
      std::string            specialHoverData = Utils::readAttrib (xSpecialLegDirectives, mxconst::get_ATTRIB_HOVER_TIME_SEC_RANDOM (), mxconst::get_ATTRIB_DEFAULT_RANDOM_HOVER_TIME ()); // "10-30"
      std::list<std::string> listHoverTimes   = Utils::splitStringToList (specialHoverData, "-");
      const std::string     &lowNum_s         = listHoverTimes.front ();
      const std::string     &highNum_s        = listHoverTimes.back ();
      int                    lowNum_i, highNum_i;
      lowNum_i  = 10;
      highNum_i = 30;

      if (Utils::is_number (lowNum_s))
        lowNum_i = Utils::stringToNumber<int> (lowNum_s);

      if (Utils::is_number (highNum_s))
        highNum_i = Utils::stringToNumber<int> (highNum_s);
      ///// end custom hover time //////////////////////////


      const int hoverTime = Utils::getRandomIntNumber (lowNum_i, highNum_i);
      xLegTask.updateAttribute (Utils::formatNumber<int> (hoverTime).c_str (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC ().c_str (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC ().c_str ()); // updating using the XML api do not work somehow. Should investigate.

      Utils::xml_search_and_set_attribute_in_IXMLNode (xNewFlightLeg, mxconst::get_ATTRIB_HOVER_TIME (), Utils::formatNumber<int> (hoverTime), mxconst::get_ELEMENT_LEG ());

      triggerTargetMessage = "Hover above target for " + Utils::formatNumber<int> (hoverTime) + " seconds.";
    }

    missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE (), flight_leg_type_hover_land_or_start); // , missionx::data_manager::prop_userDefinedMission_ui.node,                    missionx::data_manager::prop_userDefinedMission_ui.node.getName()); // v3.0.241.9 store if we have wet location


  } // end if not START flight_leg_type not LAST leg and NAV aid does not have same flag as flight_leg_type.

  if (!radius_mt.empty () && Utils::is_number (radius_mt))
    Utils::xml_search_and_set_attribute_in_IXMLNode (xLegTargetTrigger, mxconst::get_ATTRIB_LENGTH_MT (), radius_mt, mxconst::get_ELEMENT_RADIUS ());

  // delete points that are not valid
  Utils::xml_delete_empty_nodes (xLegTargetTrigger);

  // add trigger to <triggers>
  xTriggers.addChild (xLegTargetTrigger, xTriggers.nChildNode ());

  /////// End shared get_target() handlings ///////////////



  //////////////////////////////////////////
  // ADD DISPLAY_OBJECT                ////
  // check if there is any <display_object_set name="element name to pick from" template="medevac" /> example : <display_object_set name="object_sets" template="medevac" />
  // an <object set> comes before <display_objects>
  // Add <display_object> elements
  const auto lmbda_add_all_display_object_xxx_elements = [&] (const IXMLNode &inParentNode, IXMLNode &inoutTargetNode)
  {
    const int nDisplayObjects = inParentNode.nChildNode ();
    for (int i1 = 0; i1 < nDisplayObjects; ++i1)
    {
      // get sub-node
      auto cNode = inParentNode.getChildNode (i1).deepCopy ();
      if (cNode.isEmpty ())
        continue;

      // filter out sub-nodes that are not <display_xxx> elements
      std::string tag = cNode.getName ();
      if (tag != mxconst::get_ELEMENT_DISPLAY_OBJECT () && tag != mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ())
        continue; // skip elements that are not <display_object> not <display_object_near_plane

      #ifndef RELEASE
      Log::logMsgThread (std::string ("[random object_set]Adding 3D display_objects from: ") + tag + ": " + Utils::readAttrib (cNode, mxconst::get_ATTRIB_NAME (), ""));
      #endif

      // if (this->parse_display_object_element (inoutTargetNode, cNode)) // v3.0.219.1 handle <display_object> options like: optional, random_water or limit_to_terrain_slope

      if (std::string err; parse_display_object_element (inoutTargetNode, cNode, missionx::RandomEngine::xRootTemplate, this->x3DObjTemplate, this->expected_slope_at_target_location_d, err)) // v3.0.219.1 handle <display_object> options like: optional, random_water or limit_to_terrain_slope
      {
        if (tag == mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ())
        {
          //  Force replace_lat / replace_long dummy values to be on the safe side
          cNode.updateAttribute ("1.0", mxconst::get_ATTRIB_REPLACE_LAT ().c_str (), mxconst::get_ATTRIB_REPLACE_LAT ().c_str ());
          cNode.updateAttribute ("1.0", mxconst::get_ATTRIB_REPLACE_LONG ().c_str (), mxconst::get_ATTRIB_REPLACE_LONG ().c_str ());
        }

        inoutTargetNode.addChild (cNode.deepCopy (), inoutTargetNode.nChildNode ());
      }
      else
        RandomEngine::setError (err); // v25.06.1
    }
  };



  if (IXMLNode xDisplayObjectSet = xLegFromTemplate.getChildNode (mxconst::get_ELEMENT_DISPLAY_OBJECT_SET ().c_str ()); !xDisplayObjectSet.isEmpty ())
  {
    std::string random_tag       = Utils::readAttrib (xDisplayObjectSet, mxconst::get_ATTRIB_RANDOM_TAG (), "");
    std::string set_name_to_pick = Utils::readAttrib (xDisplayObjectSet, mxconst::get_ATTRIB_SET_NAME (), "");

    if (mxconst::get_GENERATE_TYPE_MEDEVAC () == set_name_to_pick && expected_slope_at_target_location_d > (missionx::data_manager::Max_Slope_To_Land_On * 3.0f))
    {
      set_name_to_pick = mxconst::get_TERRAIN_TYPE_MEDEVAC_SLOPE ();
      #ifndef RELEASE
      Log::logMsg ("[random object_set]Replace object set type to: " + set_name_to_pick + ", due to slope: " + Utils::formatNumber<double> (expected_slope_at_target_location_d, 2), true);
      #endif
    }


    if (!random_tag.empty ())
    {
      if (IXMLNode xTag = xRootTemplate.getChildNode (random_tag.c_str ()); !xTag.isEmpty ())
      {
        int nChilds       = 0;
        int randomChild_i = -1;
        // check child tag
        if (set_name_to_pick.empty ())
          nChilds = xTag.nChildNode ();
        else
          nChilds = xTag.nChildNode (set_name_to_pick.c_str ());

        #ifndef RELEASE
        Log::logMsg ("[random object_set]Search 3D set_name: " + set_name_to_pick, true);
        #endif

        // Pick a child
        if (nChilds > 0)
        {
          IXMLNode cTagNode;
          randomChild_i = Utils::getRandomIntNumber (0, nChilds - 1);
          if (set_name_to_pick.empty ())
            cTagNode = xTag.getChildNode (randomChild_i);
          else
            cTagNode = xTag.getChildNode (set_name_to_pick.c_str (), randomChild_i);


          if (!cTagNode.isEmpty ())
          {

            #ifndef RELEASE
            const int nDisplayObjects = cTagNode.nChildNode (mxconst::get_ELEMENT_DISPLAY_OBJECT ().c_str ()) + cTagNode.nChildNode (mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ().c_str ());
            #endif
            lmbda_add_all_display_object_xxx_elements (cTagNode, xNewFlightLeg);


            #ifndef RELEASE
            if (nDisplayObjects == 0)
            {
              Log::logMsg (std::string ("[random object_set]Failed to find valid display set: ") + cTagNode.getName () + ". Will try to search for <display_object> instead.", true);
            }
            #endif

          } // end if template tag name was found

        } // end if nChilds > 0

      } // end if xTag is not Empty

    } // end if tag_name is not empty
  }

  /// v3.303.11 - Breaking change, we always add <display_object> and <display_object_near_plane> even if we have a <display_set> used. It is up on the designer to make sure there are no duplications
  lmbda_add_all_display_object_xxx_elements (xLegFromTemplate, xNewFlightLeg); // v3.303.11 add all mxconst::ELEMENT_DISPLAY_OBJECT_xxx to flight_leg that are not part of the set

  #ifndef RELEASE
  Log::logMsg ("[DEBUG random location info] Final location_type: " + location_type + ", location_value_nm_s=" + location_value_nm_s + "\n", true);
  #endif

  // --------------------------------
  // Continue with other shared elements that might need trigger location information
  // --------------------------------
  Utils::xml_search_and_set_attribute_in_IXMLNode (xNewFlightLeg, mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE (), flight_leg_type_hover_land_or_start, mxconst::get_ELEMENT_LEG ()); // v3.0.192.3 add leg type for later use, like message construction
  Utils::xml_search_and_set_attribute_in_IXMLNode (xNewFlightLeg, mxconst::get_ATTRIB_TASK_TRIGGER_NAME (), triggerName, mxconst::get_ELEMENT_LEG ()); // v3.0.192.2 store trigger name for later usage like calculate distance between two legs.

  // v3.0.219.3 set trigger target + message If they are not disabled
  if (!flag_disable_auto_messages)
  {
    IXMLNode xMessageOnTarget = xLegTargetTrigger.deepCopy (); // v3.0.219.3 adds target trigger message while keeping the original xLegTargetTrigger intact

    // v25.02.1 clear the <outcome> element after "copy" since it might hold unwanted attribute values
    auto xOutcome = xMessageOnTarget.getChildNode (mxconst::get_ELEMENT_OUTCOME ().c_str ());
    Utils::xml_clear_node_attributes_excluding_list (xOutcome, {}, true);

    Utils::xml_search_and_set_attribute_in_IXMLNode (xMessageOnTarget, mxconst::get_ATTRIB_NAME (), message_triggerTargetName, mxconst::get_ELEMENT_TRIGGER ()); // v3.0.219.3
    Utils::xml_search_and_set_attribute_in_IXMLNode (xMapMessage, mxconst::get_ATTRIB_NAME (), message_triggerTargetName, mxconst::get_ELEMENT_MESSAGE ()); // v3.0.219.3 the trigger and message elements have the smane name. No conflict
    IXMLNode textMixNode = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (xMapMessage, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT (), false); // direct pointer to Mix node
    #ifndef RELEASE
    Utils::xml_print_node (xMapMessage, true);
    #endif
    if (!textMixNode.isEmpty ())
    {
      // remove or set relevant trigger attributes so message will fire when in physical area
      Utils::xml_search_and_set_attribute_in_IXMLNode (xMessageOnTarget, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "");

      // if (FL_TEMPLATE_VAL_HOVER == flight_leg_type_hover_land_or_start)
      if (flight_leg_type_hover_land_or_start.find (mxconst::get_FL_TEMPLATE_VAL_HOVER ()) != std::string::npos) // v25.02.1 is HOVER in string, since we have also "land_hover" cases
        Utils::xml_search_and_set_attribute_in_IXMLNode (xMessageOnTarget, mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT (), "");


      Utils::xml_search_and_set_attribute_in_IXMLNode (xMessageOnTarget, mxconst::get_ATTRIB_RE_ARM (), mxconst::get_MX_YES (), mxconst::get_ELEMENT_TRIGGER ()); // add rearm
      Utils::xml_search_and_set_attribute_in_IXMLNode (xMessageOnTarget, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED (), message_triggerTargetName, mxconst::get_ELEMENT_OUTCOME ()); // add re-arm
      // add message
      Utils::xml_add_cdata (textMixNode, triggerTargetMessage);

      // add message to messages
      Utils::xml_add_node_to_element_IXMLNode (this->xMessages, xMapMessage);

      // add target trigger to <triggers>
      Utils::xml_add_node_to_element_IXMLNode (this->xTriggers, xMessageOnTarget);

      // Link to current Flight Leg
      IXMLNode linkNode = xNewFlightLeg.addChild (mxconst::get_ELEMENT_LINK_TO_TRIGGER ().c_str ());
      if (!linkNode.isEmpty ())
        Utils::xml_search_and_set_attribute_in_IXMLNode (linkNode, mxconst::get_ATTRIB_NAME (), message_triggerTargetName, mxconst::get_ELEMENT_LINK_TO_TRIGGER ());
    }
    else
    {
      RandomEngine::setError ("[random build] Warning: Failed to find <mix> sub-element in <message> element. Suggestion: Fix Mapping in template file. Will not create target message.");
    }
  } // end if not flag_disable_auto_messages


  // v3.0.219.7 Add inventory if any
  if (flight_leg_type_hover_land_or_start != mxconst::get_FL_TEMPLATE_VAL_START ()) // skip if we return to starting location = "briefer"
    this->addInventory (flightLegName, xLegTargetTrigger);

  /// Store target radius in leg element
  const std::string target_radius_mt = Utils::xml_get_attribute_value_drill (xLegTargetTrigger, mxconst::get_ATTRIB_LENGTH_MT (), this->flag_found, mxconst::get_ELEMENT_RADIUS ());
  if (!target_radius_mt.empty ())
    xNewFlightLeg.updateAttribute (target_radius_mt.c_str (), mxconst::get_ATTRIB_LENGTH_MT ().c_str (), mxconst::get_ATTRIB_LENGTH_MT ().c_str ());


  // v3.0.221.13
  if (newNavInfo.lat == 0.0 || newNavInfo.lon == 0.0)
  {
    #ifndef RELEASE
    Log::logMsg ("[DEBUG new leg] Reject new <leg>: " + flightLegName + ", lat/long info seem 0 and not correct. Skip <leg> build...\n", true);
    #endif

    return IXMLNode::emptyIXMLNode;
  }

  // store xPoint in newNavInfo and later in lastFlightLegNavInfo
  lastFlightLegNavInfo.init ();

  // calculate distance to next flight leg and store in Legs
  if (!RandomEngine::listNavInfo.empty ())
  {
    NavAidInfo   prev_na    = RandomEngine::listNavInfo.back (); // prev nav aid
    const double distance   = Point::calcDistanceBetween2Points (prev_na.p, newNavInfo.p);
    std::string  distance_s = Utils::formatNumber<double> (distance, 3);
    xNewFlightLeg.updateAttribute (distance_s.c_str (), mxconst::get_ATTRIB_DISTANCE_NM ().c_str (), mxconst::get_ATTRIB_DISTANCE_NM ().c_str ());

    if ((distance < static_cast<double> (mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT) && !this->flag_isLastFlightLeg /*v3.0.221.7*/) || (distance <= static_cast<double> (mxconst::DEFAULT_RANDOM_POINT_JUMP_NM)) /*v3.0.221.15*/)
    {
      #ifndef RELEASE
      Log::logMsg ("[DEBUG new <leg>] Reject new <leg>: " + flightLegName + ", distance to <leg> is too short: " + Utils::formatNumber<double> (distance, 2) + "\n", true);
      #endif

      return IXMLNode::emptyIXMLNode;
    }
  }

  ///////////// Prepare Skewed position for GPS and target_markers based instances
  // v3.0.241.8
  const bool flag_display_target_markers_away_from_target = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET (), false);

  auto lmbda_get_skew_position = [&](const IXMLNode &inTargetPoint)
  {
    if (flag_display_target_markers_away_from_target && !this->flag_isLastFlightLeg && missionx::RandomEngine::getPlaneType () == static_cast<uint8_t> (_mx_plane_type::plane_type_helos))
    {
      newNavInfo.flag_is_skewed = true; // v3.0.241.8
      return get_skewed_target_position (inTargetPoint).deepCopy ();
    }

    return inTargetPoint.deepCopy ();
  };

  Utils::xml_set_attribute_in_node<bool> (xPoint, mxconst::get_ATTRIB_IS_TARGET_POINT_B (), true, mxconst::get_ELEMENT_POINT ()); // v3.0.241.8 A skewed point can still be a target so GPS points can be distinguished.
  newNavInfo.xml_skewdPointNode = lmbda_get_skew_position (xPoint.deepCopy ()); // xPoint represents the real position.

  setInstanceProperties (xNewFlightLeg, newNavInfo, this->xDummyTopNode, this->flag_isLastFlightLeg); // v3.0.219.10  // v3.0.241.8 added newNavInfo since it holds target and skew. We also send the flag_display_target_markers_away_from_target

  // GPS GPS GPS GPS
  // Add to GPS
  if (!xGPS.isEmpty () && !xNewFlightLeg.isEmpty ())
  {
    if (newNavInfo.flag_is_skewed) // already asked all question in the lmbda_get_skew_position()
    {
      xGPS.addChild (newNavInfo.xml_skewdPointNode.deepCopy ());
      Log::logDebugBO ("[random] Added GPS skewed target location due to setup preference.", true);
    }
    else
      xGPS.addChild (xPoint.deepCopy ());

    // v3.0.221.4 add poi_tag point elements to GPS. They do not represent targets though.
    const std::string poi_tag = Utils::readAttrib (xPoint, mxconst::get_ATTRIB_POI_TAG (), "");
    if (!poi_tag.empty ())
    {
      int nChilds = missionx::RandomEngine::xRootTemplate.getChildNode (poi_tag.c_str ()).nChildNode (mxconst::get_ELEMENT_POINT ().c_str ());
      for (int i1 = 0; i1 < nChilds; ++i1)
      {
        IXMLNode p = missionx::RandomEngine::xRootTemplate.getChildNode (poi_tag.c_str ()).getChildNode (mxconst::get_ELEMENT_POINT ().c_str (), i1).deepCopy ();
        if (!p.isEmpty ())
          xGPS.addChild (p.deepCopy ());
      }
    }
  }


  // v3.0.221.7 add shared_type to flight leg node. Values should be medevac or delivery. Will be used mainly with external plugins using the custom datarefs: "xpshared/target/type"
  xNewFlightLeg.updateAttribute ("", mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE ().c_str (), mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE ().c_str ());

  if (!xSpecialLegDirectives.isEmpty ()) // v3.0.221.8
    Utils::xml_copy_node_attributes (xNewFlightLeg, xSpecialLegDirectives);

  //// v3.0.221.9 add target location to special directive node
  const std::string lat_s        = Utils::formatNumber<float> (newNavInfo.lat, 8);
  const std::string lon_s        = Utils::formatNumber<float> (newNavInfo.lon, 8);
  const std::string elev_mt_s    = Utils::formatNumber<float> (newNavInfo.height_mt, 2);
  const std::string target_loc_s = lat_s + "|" + lon_s + "|" + elev_mt_s;
  if (!xSpecialLegDirectives.isEmpty ())
  {
    xSpecialLegDirectives.updateAttribute (target_loc_s.c_str (), mxconst::get_ATTRIB_TARGET_POS ().c_str (), mxconst::get_ATTRIB_TARGET_POS ().c_str ());

    this->addTriggersBasedOnTargetLocation (newNavInfo, xSpecialLegDirectives); // v3.0.221.10 we send the newNavInfo class so we can assign the target location to the new added triggers
  }


  IXMLNode    xDescText         = Utils::xml_get_node_from_node_tree_IXMLNode (xLegFromTemplate, mxconst::get_ELEMENT_DESC (), true); // get a copy of DESC message (not the clear CDATA). Simple text between <desc> {text} </desc>
  std::string customLegDescText = Utils::xml_get_text (xDescText);
  if (!customLegDescText.empty ())
  {
    // v3.0.221.11 refine leg message
    customLegDescText = this->prepare_message_with_special_keywords (newNavInfo, customLegDescText); // v3.0.223.4 replaced specific code to be in its own function to be used in other message code during Random mission creation.
    xSpecialLegDirectives.updateAttribute (mxconst::get_MX_YES ().c_str (), mxconst::get_ATTRIB_CUSTOM_FLIGHT_LEG_DESC_FLAG ().c_str (), mxconst::get_ATTRIB_CUSTOM_FLIGHT_LEG_DESC_FLAG ().c_str ());
  }


  //// v3.0.221.9 Add leg description
  IXMLNode xDesc = Utils::xml_get_node_from_node_tree_IXMLNode (xNewFlightLeg, mxconst::get_ELEMENT_DESC (), false);
  if (!xDesc.isEmpty ())
  {
    xDesc.deleteNodeContent ();
    xDesc = IXMLNode (); // v3.0.241.1
  }
  xDesc = xNewFlightLeg.addChild (mxconst::get_ELEMENT_DESC ().c_str ());

  // construct Flight Leg description
  std::string       desc;
  // const std::string loc_desc = (newNavInfo.loc_desc.empty ()) ? newNavInfo.init_locDesc () : newNavInfo.loc_desc;
  if (newNavInfo.loc_desc.empty ())
    newNavInfo.init_locDesc ();
  const std::string loc_desc = newNavInfo.get_loc_desc ();

  if (flight_leg_type_hover_land_or_start == mxconst::get_FL_TEMPLATE_VAL_START ())
  {
    if (customLegDescText.empty ())
      desc = "Fly back to " + loc_desc + ". Expected distance: {distance}. Consult your GPS or map.";
    else
      desc = customLegDescText;
  }
  else
  {
    if (customLegDescText.empty ())
      desc = "Fly to " + loc_desc + ". Expected distance: {distance}. Consult your GPS or map.";
    else
      desc = customLegDescText;
  }
  // desc = RandomEngine::gen_message_with_special_keywords_static (desc, newNavInfo); // v3.0.241.9
  desc = this->prepare_message_with_special_keywords (newNavInfo, desc); // v3.0.241.9

  // v3.0.253.4 add ways around target location to the end of the description. Restriction: target is now skewed
  if (const std::string ways_near_navaid_s = newNavInfo.parse_ways_around (); !ways_near_navaid_s.empty () && newNavInfo.xml_skewdPointNode.isEmpty ())
    desc += "\nAllChildNodes around location: " + ways_near_navaid_s;

  Utils::xml_add_cdata (xDesc, desc);
  //// end v3.0.221.9

  // Add leg_messages // v3.0.223.4
  // store disable auto leg messages in lastFlightLegNavInfo for later use in RandomEngine, like "injectMessagesWhileFlyingToDestination()"
  lastFlightLegNavInfo = newNavInfo; // v3.0.241.8 init lastFlightLegNavInfo before storing it
  lastFlightLegNavInfo.setBoolProperty (mxconst::get_ATTRIB_DISABLE_AUTO_MESSAGE_B (), flag_disable_auto_messages);


  const int nLegMessages = xLegFromTemplate.nChildNode (mxconst::get_DYNAMIC_MESSAGE ().c_str ());
  for (int i1 = 0; i1 < nLegMessages; ++i1)
  {
    IXMLNode xLegDynamicMessageNode = xLegFromTemplate.getChildNode (mxconst::get_DYNAMIC_MESSAGE ().c_str (), i1);
    if (!xLegFromTemplate.isEmpty ())
    {
      std::string text = Utils::xml_get_text (xLegDynamicMessageNode);
      if (!text.empty ())
      {
        text = this->prepare_message_with_special_keywords (newNavInfo, text); // v3.0.223.4 replaced specific code to be in its own function to be used in other message code during Random mission creation.
        // text = RandomEngine::gen_message_with_special_keywords_static (text, newNavInfo); // v3.0.223.4 replaced specific code to be in its own function to be used in other message code during Random mission creation.
        xLegDynamicMessageNode.deleteText ();
        xLegDynamicMessageNode.addText (text.c_str ());
      }
      xNewFlightLeg.addChild (xLegDynamicMessageNode.deepCopy ());
    }
  }
  // End leg_messages

  // v3.0.253.7 add timer (only the first one)
  if (xLegFromTemplate.nChildNode (mxconst::get_ELEMENT_TIMER ().c_str ()) > 0)
    xNewFlightLeg.addChild (xLegFromTemplate.getChildNode (mxconst::get_ELEMENT_TIMER ().c_str ()).deepCopy ());

  // v3.0.221.2 storing new Flight Leg into the original template xml leg
  lastFlightLegNavInfo.xLegFromTemplate = in_legNodeFromTemplate.deepCopy ();
  if (!lastFlightLegNavInfo.xLegFromTemplate.isEmpty ())
  {
    lastFlightLegNavInfo.xLegFromTemplate.updateAttribute (flightLegName.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
    lastFlightLegNavInfo.xLegFromTemplate.updateAttribute (flight_leg_type_hover_land_or_start.c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE ().c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE ().c_str ());
    Utils::xml_search_and_set_attribute_in_IXMLNode (lastFlightLegNavInfo.xLegFromTemplate, mxconst::get_ATTRIB_LOCATION_TYPE (), location_type, mxconst::get_ELEMENT_EXPECTED_LOCATION ());
    // v25.08.1 support for "location_properties" attribute that will replace "location_value"
    Utils::xml_search_and_set_attribute_in_IXMLNode (lastFlightLegNavInfo.xLegFromTemplate, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), location_value_nm_s, mxconst::get_ELEMENT_EXPECTED_LOCATION ());
    // TODO: deprecate the use of "get_ATTRIB_LOCATION_VALUE"
    Utils::xml_search_and_set_attribute_in_IXMLNode (lastFlightLegNavInfo.xLegFromTemplate, mxconst::get_ATTRIB_LOCATION_VALUE (), location_value_nm_s, mxconst::get_ELEMENT_EXPECTED_LOCATION ());
  }
  lastFlightLegNavInfo.flightLegName = flightLegName;
  lastFlightLegNavInfo.synchToPoint (); // store Flight Leg info.
  RandomEngine::listNavInfo.emplace_back (lastFlightLegNavInfo);

  return xNewFlightLeg;
}

// --------------------------------

// --------------------------------

void
RandomEngine::fill_up_next_leg_attrib_after_flight_plan_was_generated ()
{
  ///// Set next_leg attribute for <leg> element /////
  // Loop over all <legs> and place "next leg" in the correct attribute
  // we use the two maps: mapFlightPlanOrder_si and mapFlightPlanOrder_is. One map holds flight_leg and its index and the other map holds the index and the flight_leg. This way we can check if we have "index+1" in mapFlightPlanOrder_is and
  // therefore pick the next leg. There are other options to handle this, but that was quite simple although not as readable as, list container for example.
  const int nFlightLegChilds = xFlightLegs.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());
  for (int i1 = 0; i1 < nFlightLegChilds && !(missionx::RandomEngine::threadState.flagAbortThread); ++i1)
  {
    // bool              flag_found;
    IXMLNode node = xFlightLegs.getChildNode (i1);
    if (const std::string leg_name = Utils::readAttrib (node, mxconst::get_ATTRIB_NAME (), ""); !leg_name.empty ())
    {
      const int seqInMap = mapFlightPlanOrder_si[leg_name]; // get sequence number from flight leg

      // check if we have a flight <leg> with same name. If yes, then set next_leg to that name, if not then leave empty (means = END )
      if (int next_leg_no = seqInMap + 1; Utils::isElementExists (mapFLightPlanOrder_is, next_leg_no))
      {
        const std::string next_flight_leg = mapFLightPlanOrder_is[next_leg_no];
        Utils::xml_search_and_set_attribute_in_IXMLNode (node, mxconst::get_ATTRIB_NEXT_LEG (), next_flight_leg, mxconst::get_ELEMENT_LEG ());
      }
    }

    if (missionx::RandomEngine::threadState.flagAbortThread) // v3.0.219.12+
    {
      RandomEngine::setError ("[Random] Aborted !!!");
      break;
    }
  }
}

// --------------------------------

// void
// RandomEngine::readOptimizedAptDatIntoCache ()
// {
// #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
//
//   if (missionx::data_manager::cachedNavInfo_map.empty ()) // try to read from cached file if empty OR ignore it if user picked the option to ignore it
//   {
//     missionx::data_manager::queThreadMessage.emplace ("!!! Please Wait while trying to read cached data file (only once per X-Plane session is needed) !!!");
//     missionx::data_manager::queFlcActions.push (missionx::mx_flc_pre_command::set_briefer_text_message);
//     // prepare data to send to parser
//     std::string    customAptDat = data_manager::mx_folders_properties.getAttribStringValue (mxconst::get_PROP_MISSIONX_PATH (), "", data_manager::errStr) + mxconst::get_FOLDER_SEPARATOR () + mxconst::get_CUSTOM_APT_DAT_FILE (); //"customAptDat.txt"; // v3.0.219.12+
//     OptimizeAptDat opt;
//     opt.parse_aptdat (&RandomEngine::threadState, customAptDat, "", true); // v3.0.253.6 added bool at the end to flag read for cached // v3.0.255.3 added "" as dummy value since it is not mandatory
//
//     missionx::data_manager::queThreadMessage.emplace ("Finished reading optimized cached file. Continue processing...");
//     missionx::data_manager::queFlcActions.push (missionx::mx_flc_pre_command::set_briefer_text_message);
//   } // end reading cached data from file
// #endif
// }


// --------------------------------

bool
RandomEngine::filterAndPickRampBasedOnPlaneType (missionx::NavAidInfo &navAid, std::string &outErrorMsg, const missionx::mxFilterRampType &inRampFilterType) // const bool& inIgnoreCenterOfRunwayAsRamp)
{
  char *zErrMsg = nullptr;

  std::string                     err;
  missionx::mx_aptdat_cached_info navData;
  auto                            aptNavLine = std::string (navAid.name);

  outErrorMsg.clear ();

  if ((missionx::RandomEngine::threadState.flagAbortThread))
  {
    outErrorMsg = "Need to abort";
    return false;
  }

  mx_plane_types plane_type_enum_to_search = RandomEngine::template_plane_type_enum;

  if (data_manager::db_xp_airports.db_is_open_and_ready)
  {
    int rc = 0;
    //// construct view query (inner query)
    // base on airports_vu
    // we will pick the first result in the ordered result since it should reflect the closest airport based on its lat/lon
    const std::string sql_ap = fmt::format (R"(select icao_id, icao, ap_elev_ft, ap_name, ap_type, ap_lat, ap_lon
                            , mx_calc_distance ( ap_lat, ap_lon, {}, {}, 3440) as dist_nm, 0 as bearing
                            , helipads, ramp_helos, ramp_planes, ramp_props, ramp_turboprops, ramp_jet_heavy, rw_hard, rw_dirt_gravel, rw_grass
                            , rw_water, is_custom from airports_vu where 1 = 1 and icao = '{}' order by dist_nm )",
                                            mxUtils::formatNumber<double> (navAid.lat, 8),
                                            mxUtils::formatNumber<double> (navAid.lon, 8),
                                            navAid.getID ());

    #ifndef RELEASE
    Log::logMsgThread ("[get_random_airport_from_db] Query: \n" + sql_ap + "\n");
    #endif // !RELEASE

    // clear local cache
    RandomEngine::resultTable_gather_random_airports.clear ();
    rc = sqlite3_exec (data_manager::db_xp_airports.db, sql_ap.c_str (), RandomEngine::callback_gather_random_airports_db, nullptr, &zErrMsg);
    if (rc != SQLITE_OK)
    {
      Log::logMsgThread ("[filter and pick ramp] SQL error: " + std::string (zErrMsg));
      sqlite3_free (zErrMsg);
    }
    else
    {
      Log::logMsgThread ("[filter and pick ramp] Information was gathered.");
      #ifndef RELEASE
      for (auto &[row_num, row_data] : RandomEngine::resultTable_gather_random_airports)
      {
        Log::logMsgThread (fmt::format ("\tSeq: {}, icao_id: {}, icao: {}, Distance: {}", mxUtils::formatNumber<int> (row_num), row_data["icao_id"], row_data["icao"], row_data["dist_nm"]));
      }
      #endif // !RELEASE

      if (RandomEngine::resultTable_gather_random_airports.empty ())
      {
        Log::logMsgThread ("[filter and pick ramp] No airports found relative to NavAid: " + navAid.getID ());
        return false;
      }
      auto ap_row = (*RandomEngine::resultTable_gather_random_airports.cbegin ()).second; // fetch the first result

      navAid.flag_is_custom_scenery = (!(ap_row["is_custom"].empty ()));

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

        RandomEngine::resultTable_gather_ramp_data.clear ();
        rc = sqlite3_exec (data_manager::db_xp_airports.db, sql.c_str (), RandomEngine::callback_pick_random_ramp_location_db, nullptr, &zErrMsg);
        if (rc != SQLITE_OK)
        {
          outErrorMsg = "Error during ramp search for plane type: " + translatePlaneTypeToString (plane_type_enum_to_search); // debug
          Log::logMsgThread ("[pick ramp] SQL error: " + std::string (zErrMsg));
          sqlite3_free (zErrMsg);

          return false;
        }
        else
        {
          outErrorMsg.clear ();

          if (RandomEngine::resultTable_gather_ramp_data.empty ())
          {
            outErrorMsg = "No ramp was found for plane type: " + translatePlaneTypeToString (plane_type_enum_to_search) + ", should continue and search"; // debug

            if (missionx::mxFilterRampType::exact_plane_ramp_type == inRampFilterType)
              break; // exit the loop since we want the exact ramp type
            else if (loop01 > 0) // if this is not the first iteration
            {
              // we try to search ramps that are "jets", then "turboprops" and then "prop".
              // we do not search for Helos, nor fighter ramps
              int i1 = static_cast<int> (plane_type_enum_to_search);
              i1--;
              plane_type_enum_to_search = static_cast<missionx::mx_plane_types> (i1);
              outErrorMsg += ": try ramp type for: " + translatePlaneTypeToString (plane_type_enum_to_search); // debug
            }
            else
            {
              plane_type_enum_to_search = missionx::mx_plane_types::plane_type_jets;
            }

            Log::logMsgThread ("[pick ramp] SQL error: " + outErrorMsg);
          }
          else
          {
            // Store ramp location in navaid
            Log::logMsgThread ("[pick ramp] Ramp info gathered.");
            auto ramp                = (*resultTable_gather_ramp_data.cbegin ()).second;
            navAid.lat               = mxUtils::stringToNumber<float> (ramp["lat"], ramp["lat"].length ());
            navAid.lon               = mxUtils::stringToNumber<float> (ramp["lon"], ramp["lon"].length ());
            navAid.heading           = mxUtils::stringToNumber<float> (ramp["heading"], ramp["heading"].length ());
            navAid.ramp_info.uq_name = ramp["name"];
            navAid.ramp_info.jets    = ramp["for_planes"];

            #ifndef RELEASE
            for (auto &row_val : resultTable_gather_ramp_data | std::views::values)
            {
              Log::logMsgThread ("\rRamp: " + row_val["name"] + ", icao_id: " + row_val["icao_id"] + ", icao: " + row_val["icao"]);
            }
            #endif // !RELEASE

            // revert back the template type if and only if it is different. Main reason is if we do not find a ramp location for our plane then we try to find a ramp based on other plane types
            if (RandomEngine::template_plane_type_enum != plane_type_enum_to_search)
              plane_type_enum_to_search = RandomEngine::template_plane_type_enum;

            navAid.synchToPoint ();
            return true; // exit the loop
          }
        } // end if airport result is not empty and we should search for ramp location

      } // end loop

      plane_type_enum_to_search = RandomEngine::template_plane_type_enum; // copy back the original plane type

      // If we reached this location than we failed finding a valid ramp position.
      // fetch the longest runway and return its center
      auto const lmbda_get_query_for_fallback_position_based_on_filter_type = [] (const missionx::mxFilterRampType &inFilterType, missionx::NavAidInfo &inNavAid)
      {
        static const std::string MOVE_PLANE_IN_METERS{ "20" };

        switch (inFilterType)
        {
          case missionx::mxFilterRampType::start_ramp:
          {
            // place plane 5 meters from beginning of the runway
            return "select mx_get_point_based_on_bearing_and_length_in_meters (t1.rw_no_1_lat, rw_no_1_lon, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon), " + MOVE_PLANE_IN_METERS + ") as start_pos, t1.rw_no_1 as name, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading, t1.rw_length_mt from xp_rw t1 where t1.icao= '" + inNavAid.getID () + "' order by rw_length_mt desc limit 1";
          }
          break;
          case missionx::mxFilterRampType::any_ramp_location:
          case missionx::mxFilterRampType::end_ramp:
          {
            return "select mx_get_center_between_2_points(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as start_pos, t1.rw_no_1 || '-' || t1.rw_no_2 as name, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading, t1.rw_length_mt from xp_rw t1 where t1.icao= '" + inNavAid.getID () + "' order by rw_length_mt desc limit 1";
          }
          break;
          default:
            return std::string ("");
        }

        return std::string ("");
      };

      std::string query_start_pos_s = lmbda_get_query_for_fallback_position_based_on_filter_type (inRampFilterType, navAid);
      #ifndef RELEASE
      Log::logMsgThread ("SQL Query to Fetch start pos: \n" + query_start_pos_s + "\n");
      #endif // !RELEASE

      if (!query_start_pos_s.empty ())
      {
        resultTable_gather_ramp_data.clear ();
        rc = sqlite3_exec (data_manager::db_xp_airports.db, query_start_pos_s.c_str (), RandomEngine::callback_pick_random_ramp_location_db, nullptr, &zErrMsg);
        if (rc != SQLITE_OK)
        {
          outErrorMsg = "No ramp was found for plane type: " + translatePlaneTypeToString (plane_type_enum_to_search);
          Log::logMsgThread ("[pick ramp] SQL error: " + std::string (zErrMsg));
          sqlite3_free (zErrMsg);
        }
        else
        {
          outErrorMsg.clear ();

          if (RandomEngine::resultTable_gather_ramp_data.empty ())
            Log::logMsgThread ("[pick ramp] No valid start position was found.");
          else
          {
            Log::logMsgThread ("[pick ramp] Start position info gathered.");
            auto                     ramp        = (*resultTable_gather_ramp_data.cbegin ()).second;
            std::vector<std::string> vecPosition = mxUtils::split (ramp["start_pos"], ',');

            if (vecPosition.size () > static_cast<size_t> (1))
            {
              // Store location in Navaid
              navAid.lat               = mxUtils::stringToNumber<float> (vecPosition.at (0), vecPosition.at (0).length ());
              navAid.lon               = mxUtils::stringToNumber<float> (vecPosition.at (1), vecPosition.at (1).length ());
              navAid.heading           = mxUtils::stringToNumber<float> (ramp["heading"], 6);
              navAid.ramp_info.uq_name = ramp["name"];
              navAid.ramp_info.jets    = "Runway: " + navAid.ramp_info.uq_name;

              navAid.synchToPoint ();
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
RandomEngine::setInstanceProperties (IXMLNode &legNode_ptr, missionx::NavAidInfo &inTargetNavInfo, IXMLNode &inDummyTopNode, const bool &flag_isLastFlightLeg)
{
  // Go over each <display_object> element, and validate its settings.
  // In  v3.0.219.1, we are adding "relative_pos_bearing_deg_distance_mt" attribute which is set as "bearing|distance in meters". If values are valid, then we should fix the "replace_lat/replace_lon" with the new values
  //////// Handle Instances ////////
  std::string err; // v3.0.223.1 replace the class variable with same name. BUG: it failed Flight Leg creation after calling get_target() function, although the error is not critical.

  Utils::xml_delete_empty_nodes (inDummyTopNode); // v3.0.219.3 remove invalid points

  // Prepare point information v3.0.219.5
  IXMLNode xPoint_ptr = inTargetNavInfo.p.node; // v3.0.241.8 using newNavInfo information instead of the xTargetTrigger// Utils::xml_get_node_from_node_tree_IXMLNode(xTargetTrigger, mxconst::get_ELEMENT_POINT(), false); // xPoint is a pointer to original Node

  // store trigger point
  double targetLat = Utils::readNumericAttrib (xPoint_ptr, mxconst::get_ATTRIB_LAT (), 0.0);
  double targetLon = Utils::readNumericAttrib (xPoint_ptr, mxconst::get_ATTRIB_LONG (), 0.0);
  // v3.0.219.5
  std::string              exclude_obj    = Utils::stringToLower (Utils::readAttrib (xPoint_ptr, mxconst::get_ATTRIB_EXCLUDE_OBJ (), "")); // convert to lower case
  std::string              include_obj    = Utils::readAttrib (xPoint_ptr, mxconst::get_ATTRIB_INCLUDE_OBJ (), "");
  std::vector<std::string> vecExclude     = mxUtils::split_v2 (exclude_obj, mxconst::get_COMMA_DELIMITER ()); // mxconst::get_COMMA_DELIMITER() = ","
  std::vector<std::string> vecInclude     = mxUtils::split_v2 (include_obj, mxconst::get_COMMA_DELIMITER ());
  int                      includeObjSize = static_cast<int> (vecInclude.size ());

  std::set<std::string> setExclude;
  for (const auto &s : vecExclude)
    setExclude.insert (s);


  const int nChilds = legNode_ptr.nChildNode (); // v3.303.11 get all children
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    // get node and filter all nodes that are not DISPLAY_OBJECT type
    if (IXMLNode xNode = legNode_ptr.getChildNode (i1); !xNode.isEmpty ())
    {
      // filter by tag name
      const std::string tagName = xNode.getName ();
      if (tagName != mxconst::get_ELEMENT_DISPLAY_OBJECT () && tagName != mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ())
        continue;


      std::string obj3d_name = Utils::readAttrib (xNode, mxconst::get_ATTRIB_NAME (), "");
      /// v3.0.219.5 check and replace object in exclude list
      if (setExclude.contains (Utils::stringToLower (obj3d_name)))
      {
        // check if we can replace with include
        if (includeObjSize == 0)
        {
          Log::logMsg ("[random instance] exclude 3D Object: " + obj3d_name + ", no replacement.", true);
          continue;
        }

        // pick randomly
        if (includeObjSize == 1)
          obj3d_name = vecInclude.at (0);
        else
        {
          int randNum = Utils::getRandomIntNumber (0, includeObjSize - 1);
          if (randNum >= includeObjSize) // to be on the safe side
            randNum = includeObjSize - 1;

          obj3d_name = vecInclude.at (randNum);
        }

        xNode.updateAttribute (obj3d_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str ()); // change element name value
        xNode.deleteAttribute (mxconst::get_ATTRIB_INSTANCE_NAME ().c_str ()); // will be constructed next
      }
      // end v3.0.219.5 exclude/include objects

      std::string instName = Utils::readAttrib (xNode, mxconst::get_ATTRIB_INSTANCE_NAME (), ""); // v3.0.241.9 replaced code with simpler one
      if (instName.empty ())
      {
        instName = obj3d_name + "_" + Utils::readAttrib (legNode_ptr, mxconst::get_ATTRIB_NAME (), "") + "_" + Utils::formatNumber<int> (i1);
        xNode.updateAttribute (instName.c_str (), mxconst::get_ATTRIB_INSTANCE_NAME ().c_str (), mxconst::get_ATTRIB_INSTANCE_NAME ().c_str ()); // v3.0.241.9 replaced the addAttribute with update/add and position in element
      }

      // special validation and initialization of <display_object> element only
      if (tagName == mxconst::get_ELEMENT_DISPLAY_OBJECT ())
      {
        std::string replaceLat                  = Utils::readAttrib (xNode, mxconst::get_ATTRIB_REPLACE_LAT (), "");
        std::string replaceLon                  = Utils::readAttrib (xNode, mxconst::get_ATTRIB_REPLACE_LONG (), "");
        std::string replaceElev_ft              = Utils::readAttrib (xNode, mxconst::get_ATTRIB_REPLACE_ELEV_FT (), "");
        int         replaceElevAboveGround_ft_i = Utils::readNodeNumericAttrib<int> (xNode, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT (), 0);


        // v3.0.219.1 implement location relative to target
        std::string relative_pos_bearing_deg_distance_mt = Utils::readAttrib (xNode, mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT (), "");
        if (const std::vector<int> vecRelativePos = Utils::splitStringToNumbers<int> (relative_pos_bearing_deg_distance_mt, mxconst::get_PIPE_DELIMITER ()); vecRelativePos.size () > 1)
        {
          double newLat, newLon, trigLat, trigLon, newBearing;
          newLat = newLon = trigLat = trigLon = newBearing = 0.0;

          if (targetLat != 0.0 && targetLon != 0.0)
          {
            // calculate new targetLat/long
            auto distance_nm = static_cast<double> (vecRelativePos.at (1)) * meter2nm;
            auto bearing     = static_cast<float> (vecRelativePos.at (0));
            Utils::calcPointBasedOnDistanceAndBearing_2DPlane (newLat, newLon, targetLat, targetLon, bearing, distance_nm);

            // set new targetLat/long in instance replace point data
            const std::string newInstanceLat_s = Utils::formatNumber<double> (newLat, 8);
            const std::string newInstanceLon_s = Utils::formatNumber<double> (newLon, 8);
            Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LAT (), newInstanceLat_s, mxconst::get_ELEMENT_DISPLAY_OBJECT ());
            Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LONG (), newInstanceLon_s, mxconst::get_ELEMENT_DISPLAY_OBJECT ());
          }
        }
        // v3.0.219.1 end relative location calculation

        // v3.303.14 [regression bug fix] reset relative value so plugin won't re-calculate it again when parsing the instance node.
        if (relative_pos_bearing_deg_distance_mt.empty () == false)
        {
          xNode.updateAttribute (relative_pos_bearing_deg_distance_mt.c_str (), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS ().c_str (), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS ().c_str ()); // Keep the value in a debug attribute
          xNode.updateAttribute ("", mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT ().c_str (), mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT ().c_str ()); // reset the value
        }

        else if (!obj3d_name.empty ()) // if we have no relative location information, then place at target position
        {
          // define replace_lat/replace_long WITH TARGET POSITION (LAT/LON) if one of them is not set
          if (replaceLat.empty () || replaceLon.empty ())
          {
            const std::string targetLat_s = Utils::formatNumber<double> (targetLat, 8);
            const std::string targetLon_s = Utils::formatNumber<double> (targetLon, 8);
            Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LAT (), targetLat_s, mxconst::get_ELEMENT_DISPLAY_OBJECT ());
            Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LONG (), targetLon_s, mxconst::get_ELEMENT_DISPLAY_OBJECT ());
          }

          // v3.0.241.7 // v3.0.241.8 removed after the positioning of the 3D instance. See code below
          // set default above ground only if "replace_elev_ft" does not exist and attribute "replace_elev_above_ground_ft" exists
          if (replaceElev_ft.empty () && replaceElevAboveGround_ft_i != 0)
          {
            const std::string replaceElevAboveGround_s = Utils::formatNumber<int> (replaceElevAboveGround_ft_i);
            Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT (), replaceElevAboveGround_s, mxconst::get_ELEMENT_DISPLAY_OBJECT ()); //
          }
        }

        // v3.0.241.8 place target instances not in their exact locations based on SETUP screen.
        //// Inject SETUP options for target_marker <display_object> and helos and if it is not the last target. TODO: Check if we can figure type of location (XY vs picked ones) and if we new the type of mission then we could also rule out
        /// delivery missions
        ///
        const bool flag_display_target_markers_away_from_target = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET (), false);

        if (const bool target_marker_b = Utils::readBoolAttrib (xNode, mxconst::get_ATTRIB_TARGET_MARKER_B (), false); flag_display_target_markers_away_from_target && target_marker_b && !flag_isLastFlightLeg && RandomEngine::getPlaneType () == static_cast<uint8_t> (_mx_plane_type::plane_type_helos) && Utils::readNodeNumericAttrib<int> (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::_ui_mission_type::medevac)) == static_cast<int> (missionx::_ui_mission_type::medevac))
        {
          if (std::string skewed_name = Utils::readAttrib (xNode, mxconst::get_ATTRIB_SKEWED_NAME (), ""); !skewed_name.empty ())
            Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_NAME (), skewed_name, mxconst::get_ELEMENT_DISPLAY_OBJECT ());

          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LAT (), Utils::readAttrib (inTargetNavInfo.xml_skewdPointNode, mxconst::get_ATTRIB_LAT (), mxconst::get_ZERO ()), mxconst::get_ELEMENT_DISPLAY_OBJECT ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LONG (), Utils::readAttrib (inTargetNavInfo.xml_skewdPointNode, mxconst::get_ATTRIB_LONG (), mxconst::get_ZERO ()), mxconst::get_ELEMENT_DISPLAY_OBJECT ());

          Utils::xml_set_attribute_in_node<bool> (xNode, "skewed_position", true, mxconst::get_ELEMENT_DISPLAY_OBJECT ());
        }

      } // end if tag is DISPLAY_OBJECT
    } // end xNode valid
  }


  // v3.303.11 add special <display_object> directive to the "flight_leg"
  // Example: <display_object_near_plane name="crate01" instance_name="crate01_01" relative_pos_bearing_deg_distance_mt="{acf_psi}+90|{wing_span}+1" replace_lat="59.64266000" replace_long="-151.49297030" replace_distance_to_display_nm="10.00" target_marker_b="yes" replace_elev_ft=""/>

  #ifndef RELEASE
  int iDebugChilds = legNode_ptr.nChildNode (mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ().c_str ());
  #endif

  return true;
}



void
RandomEngine::injectMissionTypeFeatures ()
{
  // 1. LOOP over all flight legs
  // 1. add first Leg starting message - "hello pilot, check your GPS, fly to the landing site and pick the injured person."
  // 2. Loop over each flight leg and check if it has next flight leg, if so, then add message to check gps and fly to next leg. If not then construct last location message.
  // 3. Allow custom flight leg description
  constexpr static int FIRST_LEG_INDEX = 0;
  std::string          err;


  #ifndef RELEASE
  Log::logMsg ("[DEBUG random] injectMissionTypeFeatures.", true);
  #endif

  const int nChilds = this->xFlightLegs.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());

  // REVERSE ordering so we could flag is Wet and calculate distances.
  bool flag_has_wet_target = false; // v3.0.241.8 help to flag
  for (int i1 = nChilds - 1; i1 >= 0; --i1)
  {
    IXMLNode leg_ptr = xFlightLegs.getChildNode (mxconst::get_ELEMENT_LEG ().c_str (), i1); // pointer to <leg> xml element
    if (leg_ptr.isEmpty ())
      continue;

    IXMLNode    msg         = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE ()); // copy of <message> node, not a pointer
    IXMLNode    textMix_ptr = Utils::xml_get_node_from_node_tree_IXMLNode (msg, mxconst::get_ELEMENT_MIX (), false); // pointer to
    std::string message;
    std::string message_name;
    std::string flight_leg_name           = Utils::readAttrib (leg_ptr, mxconst::get_ATTRIB_NAME (), "");
    std::string loc_desc                  = Utils::readAttrib (leg_ptr, mxconst::get_ATTRIB_LOC_DESC (), ""); // short description of the random point generated. <leg> element should not have it anymore, since we store it in NavAid. We need to check <special_flight_leg_directive> element.
    std::string loc_desc_short            = loc_desc; // short description of the random point generated. <leg> element should not have it anymore, since we store it in NavAid. We need to check <special_flight_leg_directive> element.
    double      distance_to_prev_navaid_d = -1.0; // negative distance = invalid
    const bool  flag_isWet                = Utils::readBoolAttrib (leg_ptr, mxconst::get_PROP_IS_WET (), false);
    // v25.05.1 nav info
    missionx::NavAidInfo target_nav;

    if (!flag_has_wet_target)
      flag_has_wet_target = flag_isWet;

    if (loc_desc.empty ()) // v3.0.221.10 try to fetch location description from navaid with same flight leg name
    {
      NavAidInfo *prevNav    = nullptr; // v3.0.251.1 b2 add distance to flight leg description
      bool        bFirstTime = true;
      for (auto &nav : RandomEngine::listNavInfo)
      {

        if (nav.flightLegName == flight_leg_name) // same unique Leg name
        {
          if (nav.loc_desc.empty ())
            nav.init_locDesc ();

          // loc_desc       = (nav.loc_desc.empty ()) ? nav.init_locDesc () : nav.loc_desc;
          loc_desc       = nav.get_loc_desc ();
          loc_desc_short = nav.gen_locDesc_short ();
          target_nav.clone (nav);

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
    std::string custom_leg_desc_flag = Utils::stringToLower (Utils::xml_get_attribute_value_drill (leg_ptr, mxconst::get_ATTRIB_CUSTOM_FLIGHT_LEG_DESC_FLAG (), this->flag_found, mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES ())); // v3.0.221.15rc5 add LEG support
    if (mxconst::get_MX_YES () == custom_leg_desc_flag)
    {
      IXMLNode    xDesc = leg_ptr.getChildNode (mxconst::get_ELEMENT_DESC ().c_str ());
      std::string flightLegDesc;
      if (!xDesc.isEmpty ())
        customLegDescText = ((xDesc.nClear () > 0) ? xDesc.getClear ().sValue : missionx::EMPTY_STRING); // description of task: <task ...><![CDATA[task description]]></task>. // NO <desc> element
    }

    // if (loc_desc_short.empty ())
    //   int iStop = 0;
    // v3.0.241.9 store leg locations in a string to display in the briefer.
    if (i1 == (nChilds - 1)) // our loop is from end to start
      cumulative_location_desc_s = loc_desc_short + ((distance_to_prev_navaid_d > -1) ? "(" + Utils::formatNumber<double> (distance_to_prev_navaid_d, 2) + " nm)" : ""); // v3.0.251.1 b2 add distances
    else
    {
      cumulative_location_desc_s = loc_desc_short + ((distance_to_prev_navaid_d > -1) ? "(" + Utils::formatNumber<double> (distance_to_prev_navaid_d, 2) + " nm)" : "") + ", " + cumulative_location_desc_s;
      if (i1 == FIRST_LEG_INDEX) // first location. Used with the setup option "Expose all GPS legs at mission start = false"
        first_location_desc_s = loc_desc_short + ((distance_to_prev_navaid_d > -1) ? "(" + Utils::formatNumber<double> (distance_to_prev_navaid_d, 2) + " nm)" : "");
    }

    message_name = "leg_" + ((flight_leg_name.empty ()) ? Utils::formatNumber<int> (i1) : flight_leg_name) + "_start_message";

    if (i1 == 0) // start <leg>. Create message
    {
      if (customLegDescText.empty ())
      {

        // unknown location means that we do not have a unique name. The name has "coordinate" or "leg" in it.
        const bool        bUnknownLocation = !(target_nav.nav_aid_has_unique_name ()); // v25.06.1
        const std::string start_icao_desc  = (this->briefer_starting_location_desc.empty ()) ? "" : std::string (this->briefer_starting_location_desc).append ("\n");
        const std::string target_loc_desc  = (bUnknownLocation) ? fmt::format ("Head to coordinates: {:.9}/{:.10}\nFly safe.", target_nav.lat, target_nav.lon) : fmt::format (R"("Head to {}". Fly safe.)", loc_desc_short);

        if (flag_isWet)
          message = fmt::format ("Hello pilot. We have uploaded flight coordinates to your GPS.\n{}One of the locations is above water body.\n{}", start_icao_desc, target_loc_desc);
        else
          message = fmt::format ("Hello pilot. We have uploaded flight coordinates to your GPS.\n{}{}", start_icao_desc , target_loc_desc);
      }
      else
        message = customLegDescText;
    }
    else
    {
      // Handle rest of flight legs
      const std::string next_flight_leg = Utils::readAttrib (leg_ptr, mxconst::get_ATTRIB_NEXT_LEG (), "");

      if (customLegDescText.empty ())
      {
        if (flag_found && next_flight_leg.empty ()) // if we found attrib and next_leg is empty then it means that we are at the last flight leg
        {
          message = "Fly to last GPS location " + ((loc_desc.empty ()) ? "" : fmt::format (R"("{}". Land safely.)", loc_desc));
        }
        else if (flag_isWet)
        {
          message = "Fly to the next GPS location " + ((loc_desc.empty ()) ? "(" + next_flight_leg + ")" : fmt::format (R"("{}", it should be above water body.)", loc_desc)); // v3.0.241.8
        }
        else
          message = "Fly to the next GPS location " + ((loc_desc.empty ()) ? "(" + next_flight_leg + ")" : fmt::format (R"("{}")", loc_desc));
      }
      else
        message = customLegDescText;
    }

    // Add the message to mission file and flight leg element
    if (textMix_ptr.isEmpty () || !Utils::xml_add_cdata (textMix_ptr, message))
    {
      #ifndef RELEASE
      Log::logMsgWarn ("[random inject medevac] Message element is NULL. Check Template.", true);
      continue;
      #endif
    }

    Utils::xml_search_and_set_attribute_in_IXMLNode (msg, mxconst::get_ATTRIB_NAME (), message_name, mxconst::get_ELEMENT_MESSAGE ());
    this->xMessages.addChild (msg);
    // add as start_message to flight leg
    Utils::xml_search_and_set_attribute_in_IXMLNode (leg_ptr, mxconst::get_ATTRIB_NAME (), message_name, mxconst::get_ELEMENT_START_LEG_MESSAGE ());
    Utils::add_xml_comment (xMessages, " [[[[ ]]]] "); // add comment between 2 messages
    // end setting XML with message data


  } // END LOOP over all flight leg



  #ifndef RELEASE
  auto med_cargo_or_oilrig_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::not_defined));
  Log::logMsgThread ("med_cargo_or_oilrig_i: " + mxUtils::formatNumber<int> (med_cargo_or_oilrig_i));
  #endif // !RELEASE

  if (Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::not_defined)) == static_cast<int> (missionx::mx_ui_mission_type::oil_rig))
  {
    // add briefer start location.
    if (RandomEngine::listNavInfo.empty () == false)
    {
      NavAidInfo na = RandomEngine::listNavInfo.front ();
      if (mxconst::get_ELEMENT_BRIEFER () == na.flightLegName)
        cumulative_location_desc_s = "(start): " + na.gen_locDesc_short () + ", " + cumulative_location_desc_s;
    }
  }

  //// v3.0.241.9 Add custom briefer description if it is a mission UI based ("WinBrieferGL::user_driven_mission_layer") and its <![CDATA[ ]]> is empty.
  if (this->flag_rules_defined_by_user_ui)
  {
    std::string briefer_desc;
    if (!this->xBriefer.isEmpty ())
      briefer_desc = ((xBriefer.nClear () > 0) ? xBriefer.getClear ().sValue : ""); // description of task: <task ...><![CDATA[task description]]></task>. // NO <desc> element

    briefer_desc = mxUtils::trim (briefer_desc);
    if (briefer_desc.empty () && !xBriefer.isEmpty ())
    {
      briefer_desc = this->briefer_skeleton_message_to_use_in_injectTypeMissionFeature + "\n";
      briefer_desc += this->briefer_starting_location_desc; // v25.05.1
      briefer_desc += (flag_has_wet_target) ? "\nOne of the flight legs is in a water body, make sure you have all needed equipment. " : "";

      // v25.02.1 adding support for LAND_HOVER cases
      if (mxconst::get_FL_TEMPLATE_VAL_HOVER () == Utils::readAttrib (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE (), ""))
        briefer_desc += "\nWe believe you will have to hover above one of the locations, due to the physical terrain limitations.\nMake sure you have the right plane for this mission.";
      else if (mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER () == Utils::readAttrib (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE (), ""))
        briefer_desc += "\nWe believe you could Land or Hover above one of the locations, due to the physical terrain limitations.\nMake sure you have the right plane for this mission.";


      // v25.04.2 - fixed destination exposure, based on setup
      if (missionx::system_actions::pluginSetupOptions.getNodeText_type_1_5<bool> (mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE (), true))
        briefer_desc += "\nExpected route: " + cumulative_location_desc_s + ".";
      else
        briefer_desc += "\nFirst waypoint: " + first_location_desc_s + ".";

      briefer_desc += "\n\nFly Safe !!!";

      Utils::xml_add_cdata (xBriefer, briefer_desc);
    }
  }

  #ifndef RELEASE
  // if (mxUtils::trim (this->cumulative_location_desc_s).back () == ',')
  //   Log::logMsgThread ("Ends with ',' !!!");

  Log::logMsg ("[DEBUG random] after injectMissionTypeFeatures.", true);
  #endif

  // end injectMissionTypeFeatures
}

// -----------------------------------------

void
RandomEngine::injectMessagesWhileFlyingToDestination ()
{
  // Build messages relative to distance between 2 legs as a factor of distance.
  // prepare data before looping over legs
  // loop over legs and get distances between prev and post
  // v3.0.221.9 try to use the information in "special_flight_leg_directives" legs sub element.

  // find briefer point
  if (xBriefer.isEmpty () || xBriefer.getChildNode (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ()).isEmpty ())
  {
    RandomEngine::setError ("[random message] Briefer node is not valid");
    return;
  }

  // prepare trigger node
  IXMLNode trig_template_node = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_TRIGGER ()); // return copy of trigger node
  if (trig_template_node.isEmpty ())
  {
    Log::logMsgWarn ("[random message] Fail to find <trigger> in template", true);
    return;
  }

  // check for outcome node
  IXMLNode xOutcome = trig_template_node.getChildNode (mxconst::get_ELEMENT_OUTCOME ().c_str ());
  if (xOutcome.isEmpty ())
    xOutcome = trig_template_node.addChild (mxconst::get_ELEMENT_OUTCOME ().c_str ());


  // prepare message node from template
  IXMLNode message_node = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE ()); // return copy of Message node
  if (message_node.isEmpty ())
  {
    Log::logMsgWarn ("[random message] Fail to find <message> in template", true);
    return;
  }

  const int nLegChilds = this->xFlightLegs.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());
  int       navCounter = 0;
  int       legCounter = 0;


  NavAidInfo prevNa, currentNa;
  for (const auto &na : RandomEngine::listNavInfo)
  {
    if (navCounter == 0)
    {
      ++navCounter;
      currentNa = na; // this should be the briefer
      continue;
    }

    prevNa.init ();
    prevNa = currentNa;
    prevNa.synchToPoint ();

    currentNa.init ();
    currentNa = na;
    currentNa.synchToPoint ();

    IXMLNode xmlDataNode_ptr;
    // get flight leg
    IXMLNode legNode = xFlightLegs.getChildNode (mxconst::get_ELEMENT_LEG ().c_str (), legCounter);
    if (legNode.isEmpty ())
      continue;

    if (legCounter >= nLegChilds) // should never happen
      break;

    // v3.0.223.4
    std::string err;
    const bool  flag_disable_auto_messages = currentNa.getBoolValue (mxconst::get_ATTRIB_DISABLE_AUTO_MESSAGE_B (), false); // v3.303.14r1 fixed reading the bool value. Changed from numeric read to also support string boolean representation

    xmlDataNode_ptr = legNode; // v3.0.221.9 // v3.0.223.4 Consider removing this line since we need to work only with <special_directive /> element.

    // search Special Flight Leg directive node.
    IXMLNode sNode = legNode.getChildNode (mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES ().c_str ());
    if (!sNode.isEmpty ())
      xmlDataNode_ptr = sNode; // v3.0.221.9


    ++legCounter;
    ++navCounter;
    if (!flag_disable_auto_messages) // create or skip auto distance messages
    {

      // get trigger point
      #ifndef RELEASE
      std::string debugNaFlightLegName = currentNa.flightLegName; // debug - use with debugger
      #endif

      std::string flightLegName   = Utils::readAttrib (legNode, mxconst::get_ATTRIB_NAME (), "");
      std::string legTemplateType = Utils::readAttrib (xmlDataNode_ptr, mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE (), ""); // support leg and keep compatibility


      // v3.0.241.8 prepare two point classes based on the NavAidInfo.flag_is_skewed_point and calculate accordingly
      Point pCurr, pPrev;
      pCurr.node = (currentNa.flag_is_skewed) ? currentNa.xml_skewdPointNode : currentNa.p.node;
      pPrev.node = (prevNa.flag_is_skewed) ? prevNa.xml_skewdPointNode : prevNa.p.node;

      assert (!pCurr.node.isEmpty () && !pPrev.node.isEmpty ());

      double distance_nm;
      bool   flag_msg_skewed = false; // v3.0.241.8 influence message
      if (currentNa.flag_is_skewed && pCurr.parse_node () && pPrev.parse_node ())
      {
        flag_msg_skewed = true;
        distance_nm     = Point::calcDistanceBetween2Points (pCurr, pPrev);
      }
      else
        distance_nm = Point::calcDistanceBetween2Points (currentNa.p, prevNa.p); // fallback - may cause some errors, but its better then nothing


      if (distance_nm < 0.0)
      {
        Log::logMsgErr ("[inject message] Found <leg> without distance_nm attribute. Skipping message for <leg>: " + flightLegName + ". Notify developer.", true);
        continue;
      }
      else
      {
        if (distance_nm < 2.0) // minimal message distance should be 2nm
          distance_nm = 2.0;


        /////////////////////////
        // add distance messages
        const std::string   message_distances = "2,5,15,25,40,60";
        std::vector<double> vecDistances      = Utils::splitStringToNumbers<double> (message_distances, mxconst::get_COMMA_DELIMITER ()); // mxconst::get_COMMA_DELIMITER() = ","


        // comment separator
        Utils::add_xml_comment (xTriggers, " ++++ " + flightLegName + " distance messages +++++ "); // v3.0.219.3

        // loop over vector
        int counter = 0;
        for (const auto dist : vecDistances)
        {
          IXMLNode trigNode = trig_template_node.deepCopy ();
          if (trigNode.isEmpty ())
            continue;

          if (!(distance_nm > dist) && !(distance_nm < 2.0)) // skip message that meant for longer distance. Ie, if distance to target is 5, then do not create message that is meant for 10nm.
            continue;

          // set trigger Name by distance
          const std::string newTriggerName = "message_trig_for_" + flightLegName + "_(" + Utils::formatNumber<double> (dist) + "nm)"; // message_trig_for_leg_1_(5nm)
          Utils::xml_search_and_set_attribute_in_IXMLNode (trigNode, mxconst::get_ATTRIB_NAME (), newTriggerName, mxconst::get_ELEMENT_TRIGGER ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (trigNode, mxconst::get_ATTRIB_PLANE_ON_GROUND (), missionx::EMPTY_STRING, mxconst::get_ELEMENT_CONDITIONS ()); // remove on_ground attribute
          Utils::xml_search_and_set_attribute_in_IXMLNode (trigNode, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED (), newTriggerName, mxconst::get_ELEMENT_OUTCOME ()); // set the message name as the "trigger name" for when_fired

          // set the message name when entering trigger zone
          std::string message = "You are: " + Utils::formatNumber<double> (dist) + " nautical miles from target.";
          if (counter == 0) // the closest message to target
          {
            if (mxconst::get_FL_TEMPLATE_VAL_HOVER () == legTemplateType)
            {
              if (flag_msg_skewed)
                message += " You should look for the target, we did not receive an exact location, it should be near. Remember to hover above it once you reached it."; // v3.0.241.8 added skewed string
              else
                message += " You should look for the target location to hover."; // v3.0.241.8 added skewed string
            }

            else if (mxconst::get_FL_TEMPLATE_VAL_LAND () == legTemplateType)
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
          const int distance_mt = static_cast<int> (dist * missionx::nm2meter);

          if (IXMLNode rNode_ptr = Utils::xml_get_node_from_node_tree_IXMLNode (trigNode, mxconst::get_ELEMENT_RADIUS (), false); rNode_ptr.isEmpty ())
          {
            rNode_ptr = trigNode.addChild (mxconst::get_ELEMENT_RADIUS ().c_str ());
            Utils::xml_add_node_to_element_IXMLNode (trigNode, rNode_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ()); // place node in correct location
          }

          // set radius length attribute
          Utils::xml_search_and_set_attribute_in_IXMLNode (trigNode, mxconst::get_ATTRIB_LENGTH_MT (), Utils::formatNumber<int> (distance_mt), mxconst::get_ELEMENT_RADIUS ());

          //// Define <message>
          IXMLNode mNode       = message_node.deepCopy ();
          IXMLNode textMixNode = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (mNode, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT (), false); // direct pointer to Mix node
          if (textMixNode.isEmpty ())
          {
            Log::logMsgWarn ("[random message] Fail to find <mix> in <message> template", true);
            return;
          }

          Utils::xml_search_and_set_attribute_in_IXMLNode (mNode, mxconst::get_ATTRIB_NAME (), newTriggerName); // message has same name as its trigger
          Utils::xml_add_cdata (textMixNode, message);
          // add message to <messages>
          Utils::xml_add_node_to_element_IXMLNode (this->xMessages, mNode);

          // set trigger Location
          IXMLNode pointNode = trigNode.addChild (mxconst::get_ELEMENT_POINT ().c_str ());
          if (pointNode.isEmpty ())
            continue;

          Utils::xml_search_and_set_attribute_in_IXMLNode (pointNode, mxconst::get_ATTRIB_LAT (), Utils::formatNumber<double> (pCurr.getLat (), 8), mxconst::get_ELEMENT_POINT ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (pointNode, mxconst::get_ATTRIB_LONG (), Utils::formatNumber<double> (pCurr.getLon (), 8), mxconst::get_ELEMENT_POINT ());

          if (!Utils::xml_add_node_to_element_IXMLNode (trigNode, pointNode, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ()))
          {
            Log::logMsgErr ("[random message] Fail to add <point> to trigger. skipping trigger. ", true);
            continue;
          }

          // Add to Triggers element
          xTriggers.addChild (trigNode);

          // Link to current "flight leg"
          IXMLNode linkNode = legNode.addChild (mxconst::get_ELEMENT_LINK_TO_TRIGGER ().c_str ());
          if (!linkNode.isEmpty ())
            Utils::xml_search_and_set_attribute_in_IXMLNode (linkNode, mxconst::get_ATTRIB_NAME (), newTriggerName, mxconst::get_ELEMENT_LINK_TO_TRIGGER ());


        } // end loop over distance vector

      } // end flag_found or node is not empty

    } // end if to generate messages "if (!flag_disable_auto_messages)"

  } // end loop over flight legs
}


// -----------------------------------

void
RandomEngine::addInventory (const std::string &inFlightLegName, const IXMLNode &inSourceNode, const mxInvSource inSourceType)
{
  #ifndef RELEASE
  Log::logMsg ("[DEBUG random airport] before <inventories> node.", true);
  #endif


  std::string    invName = inFlightLegName + " Inventory"; // v24.06.1 changed from: "inv_" + inFlightLegName
  IXMLNode       xPoint;
  const IXMLNode xItemFromMap    = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_ITEM (), true); // return copy of <item> node
  IXMLNode       xInv            = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_INVENTORY (), true); // return copy of <inventory> node
  IXMLNode       xItemBlueprints = Utils::xml_get_node_from_node_tree_IXMLNode (missionx::RandomEngine::xRootTemplate, mxconst::get_ELEMENT_ITEM_BLUEPRINTS (), true); // return copy of <item_blueprints> node from <TEMPLATE> instead of <MAPPING> element.
  // get pointer to inventory location and elevation
  IXMLNode xLocAndElev_ptr = Utils::xml_get_node_from_node_tree_IXMLNode (xInv, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA (), false); // return pointer of <loc_and_elev> node


  // v24.05.1 Read the blueprint items from the external file
  // Read from the external cargo_data.xml if we generate from "user creation screen" or if the blueprint is empty
  if (const std::string subCategory_type = missionx::data_manager::prop_userDefinedMission_ui.getStringAttributeValue (mxconst::get_PROP_MISSION_SUBCATEGORY_LBL (), ""); !subCategory_type.empty () && (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_user_generates_a_mission_layer || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::flight_leg_info || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_ils_layer || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_external_fpln_layer || xItemBlueprints.isEmpty ()))
  {
    auto xExternalNode = Utils::read_external_blueprint_items (mxconst::get_ELEMENT_CARGO (), mxconst::get_ELEMENT_ITEM_BLUEPRINTS (), subCategory_type, true, false);
    if (!xExternalNode.isEmpty ())
      xItemBlueprints = xExternalNode;
  }


  if (mxconst::get_FL_TEMPLATE_VAL_START () == inFlightLegName) // v3.0.219.7 added skip legs by the name start, since it represent the briefer starting location.
    return;

  if (inSourceType == mxInvSource::trigger)
    xPoint = Utils::xml_get_node_from_node_tree_IXMLNode (inSourceNode, mxconst::get_ELEMENT_POINT (), true); // return copy of <point> node
  else
  {
    xPoint = inSourceNode.deepCopy (); // should be point
    xPoint.updateName (mxconst::get_ELEMENT_POINT ().c_str ()); // v25.04.1
  }

  //// validations ////
  if (this->setInventories.contains (invName)) // If inventory exists, exit. This is not an error.
  {
    Log::logMsgWarn ("[random inv] Inventory by the name: " + invName + ", exists, skipping", true);
    return;
  }

  if (xPoint.isEmpty () || xItemFromMap.isEmpty () || xInv.isEmpty () || xItemBlueprints.isEmpty () || xLocAndElev_ptr.isEmpty ()) // handle trigger source validation
  {
    Log::logMsgErr ("[random inv] One of the key elements could not be found. Check if there is any mapping for: \"<point>, <item>, <item_blueprints>, <inventory> and <loc_and_elev_data>\" nodes. skipping inventory creation... ", true);
    return;
  }

  // get number of items in <mapping> element
  const int itemsInBlueprint_i = xItemBlueprints.nChildNode ();
  if (itemsInBlueprint_i == 0)
  {
    Log::logMsgWarn ("[random inv] No items in <item_blueptint> node mapping. Please add <item> nodes to it for random pick.", true);
    return;
  }
  //// End validation ///


  //// Set Inventory information ////
  Utils::xml_search_and_set_attribute_in_IXMLNode (xInv, mxconst::get_ATTRIB_NAME (), invName, mxconst::get_ELEMENT_INVENTORY ());

  // add inventory location
  xLocAndElev_ptr.addChild (xPoint);

  // add inventory radius
  const std::string length_mt = Utils::xml_get_attribute_value_drill (inSourceNode, mxconst::get_ATTRIB_LENGTH_MT (), this->flag_found, mxconst::get_ELEMENT_RADIUS ()); // fetch radius if any. Trigger should have one
  if (flag_found)
    Utils::xml_search_and_set_attribute_in_IXMLNode (xInv, mxconst::get_ATTRIB_LENGTH_MT (), length_mt, mxconst::get_ELEMENT_RADIUS ());
  else
    Utils::xml_search_and_set_attribute_in_IXMLNode (xInv, mxconst::get_ATTRIB_LENGTH_MT (), mxconst::get_DEFAULT_INVENTORY_RADIUS_MT (), mxconst::get_ELEMENT_RADIUS ());

  // add items to inventory randomly
  int minNum = 0;
  int maxNum = itemsInBlueprint_i;
  if (mxconst::get_ELEMENT_BRIEFER () == inFlightLegName)
  {
    minNum = 4;
    maxNum = 12;
  }

  const int numOfItemsToCreate_i = Utils::getRandomIntNumber (minNum, maxNum); // how many items should we create in inventory ?

  // v24.12.2
  std::unordered_map<std::string, IXMLNode> mapItemsInInv = {}; // [barcode, xml pointer]

  for (int i1 = 0; i1 < numOfItemsToCreate_i; ++i1)
  {
    const int pick_i = Utils::getRandomIntNumber (0, itemsInBlueprint_i - 1); // pick random item node

    IXMLNode newItem = xItemBlueprints.getChildNode (mxconst::get_ELEMENT_ITEM ().c_str (), pick_i).deepCopy (); // get a copy of the item node
    if (newItem.isEmpty ())
      continue;

    // v24.05.1 Skip if item attribute "name" or "barcode" are empty
    const std::string sBarcode = Utils::readAttrib (newItem, mxconst::get_ATTRIB_BARCODE (), ""); // v24.12.2
    if (Utils::readAttrib (newItem, mxconst::get_ATTRIB_NAME (), "").empty () || sBarcode.empty ())
      continue;

    // v24.05.1 get original quantity
    const int originalQuantity_i = Utils::readNodeNumericAttrib<int> (newItem, mxconst::get_ATTRIB_QUANTITY (), -1); // v24.05.1 read the quantity. "-1" means not found
    const int rndQuantity        = Utils::getRandomIntNumber (1, ((originalQuantity_i > 0) ? originalQuantity_i : 10)); // pick random quantity

    // v24.12.2 Check for duplicate items based on barcode and merge their quantity
    if (mxUtils::isElementExists (mapItemsInInv, sBarcode) && !mapItemsInInv[sBarcode].isEmpty ())
    {
      const auto newQuantity = rndQuantity + Utils::readNodeNumericAttrib (mapItemsInInv[sBarcode], mxconst::get_ATTRIB_QUANTITY (), 0);
      mapItemsInInv[sBarcode].updateAttribute (mxUtils::formatNumber<int> (newQuantity).c_str (), mxconst::get_ATTRIB_QUANTITY ().c_str (), mxconst::get_ATTRIB_QUANTITY ().c_str ());
      #ifndef DEBUG
      Log::logMsgThread (fmt::format ("\tMerge items: {} in {}.", sBarcode, invName)); // v24.12.2
      #endif // !DEBUG
    }
    else
    {
      mapItemsInInv[sBarcode] = newItem;
      #ifndef DEBUG
      Log::logMsgThread (fmt::format ("Added item: {} to {}.", sBarcode, invName)); // v24.12.2
      #endif // !DEBUG
    }
  }

  // v24.12.2
  for (const auto &nodeItem : mapItemsInInv | std::views::values) // Only iterate over values and not keys
  {
    xInv.addChild (nodeItem);
  }

  #ifndef RELEASE
  Log::logMsgThread ("Added Inventory Content: \n");
  Utils::xml_print_node (xInv, true);
  #endif

  xInventoris.addChild (xInv);

  #ifndef RELEASE
  Log::logMsg ("[DEBUG random airport] after <inventories> node.", true);
  #endif
}


// -----------------------------------
// -----------------------------------
// -----------------------------------

bool
RandomEngine::writeTargetFile ()
{
  bool result = true;
  // Prepare path and file names // v3.0.241.10 b2 extended cases where template was picked from custom mission folder, therefore the output should be {mission folder name}.xml. That way we create uniquness

  const std::string savePathAndFile = (RandomEngine::working_tempFile_ptr->missionFolderName.empty ()) ? this->pathToRandomBrieferFolder + mxconst::get_FOLDER_SEPARATOR () + mxconst::get_RANDOM_MISSION_DATA_FILE_NAME () : this->pathToRandomBrieferFolder + mxconst::get_FOLDER_SEPARATOR () + RandomEngine::working_tempFile_ptr->missionFolderName + ".xml";


  const std::string_view mission_name_con = (!RandomEngine::working_tempFile_ptr->missionFolderName.empty ()) ? RandomEngine::working_tempFile_ptr->missionFolderName : RandomEngine::threadState.dataString;
#ifndef RELEASE
  Log::logMsgThread ("\n[DEBUG random writeTargetFile] Write to file: " + savePathAndFile + "\n");
#endif

  // v3.0.253.12 Adding all mission main element into a dedicated xTargetTopNode element instead of using the xDummyTopNode. This allows us to better control availability of main elements in final XML file
  IXMLNode xTargetTopNode = xTargetMainNode.addChild (mxconst::get_MISSION_ELEMENT ().c_str ());

  assert (!xTargetTopNode.isEmpty () && "[random:writeTargetFile] Fail to create the target <root> node.");
  // Set the attribute of the <MISSION> root element.
  const auto mission_file_format_s = Utils::readAttrib (missionx::RandomEngine::xRootTemplate, mxconst::get_ATTRIB_MISSION_FILE_FORMAT (), missionx::PLUGIN_FILE_VER); // v25.03.1
  xTargetTopNode.addAttribute (mxconst::get_ATTRIB_VERSION ().c_str (), mission_file_format_s.c_str ());
  xTargetTopNode.addAttribute (mxconst::get_ATTRIB_NAME ().c_str (), Utils::readAttrib (this->xDummyTopNode, mxconst::get_ATTRIB_NAME (), mission_name_con.data ()).c_str ());
  xTargetTopNode.addAttribute (mxconst::get_ATTRIB_TITLE ().c_str (), Utils::readAttrib (this->xDummyTopNode, mxconst::get_ATTRIB_TITLE (), "").c_str ()); // v3.303.8

  if (missionx::data_manager::flag_setupEnableDesignerMode)
    xTargetTopNode.addAttribute (mxconst::get_ATTRIB_MISSION_DESIGNER_MODE ().c_str (), "true"); // v24.03.2 add designer mode attrib based on the SETUP screen


  ///// ORGENIZE <MISSION> Element children /////

  // add global setting from mapping
  if (this->xGlobalSettings.isEmpty ()) // v3.0.303.2 add global_settings from Mapping only if it is empty.
    this->xGlobalSettings = data_manager::xmlMappingNode.getChildNode (mxconst::get_GLOBAL_SETTINGS ().c_str ()).deepCopy ();

  // v3.303.9 add scoring
  if (!this->xScoring.isEmpty ())
  {
    Utils::xml_delete_all_subnodes (this->xGlobalSettings, mxconst::get_ELEMENT_SCORING ());
    this->xGlobalSettings.addChild (xScoring.deepCopy ());
  }

  // v24.12.2 Handling compatibility flag. Will only affect template that do not have the "compatibility -> inventory_layout" set for them.
  if (const bool flag_surprise_me = Utils::readBoolAttrib (xMetadata, mxconst::get_ATTRIB_SURPRISE_ME_SUB_CAT_B (), false)
     ; missionx::data_manager::flag_setupUseXP11InventoryUI || flag_surprise_me)
  {
    // ??? In case of compatibility flag, we use the PLUGIN_FILE_VER_XP11 to help force the inventory layout. ???
    xTargetTopNode.updateAttribute (missionx::PLUGIN_FILE_VER_XP11, mxconst::get_ATTRIB_VERSION ().c_str (), mxconst::get_ATTRIB_VERSION ().c_str ());

    if (this->xCompatibility.isEmpty ())
      this->xCompatibility = this->xGlobalSettings.addChild (mxconst::get_ELEMENT_COMPATIBILITY ().c_str ());

    assert (!this->xCompatibility.isEmpty () && fmt::format ("[{}], <compatibility> element can't be empty.", __func__).c_str ());
    if (Utils::readAttrib (this->xCompatibility, mxconst::get_ATTRIB_INVENTORY_LAYOUT (), "").empty ())
      this->xCompatibility.updateAttribute (mxUtils::formatNumber<int> (missionx::XP11_COMPATIBILITY).c_str (), mxconst::get_ATTRIB_INVENTORY_LAYOUT ().c_str (), mxconst::get_ATTRIB_INVENTORY_LAYOUT ().c_str ());
  }

  // v3.303.14 moved date/time and weather advance settings integration into the <global_settings> node to data_manager
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_current_weather_state_and_store_in_RandomEngine))
  {
    missionx::RandomEngine::current_weather_datarefs_s.clear ();
    RandomEngine::setError ("[random write weather] Failed to read current X-Plane weather information.");
  }
  missionx::data_manager::add_advanceSettingsDateTime_and_Weather_to_node (this->xGlobalSettings, missionx::data_manager::prop_userDefinedMission_ui.node, missionx::RandomEngine::current_weather_datarefs_s);

  IXMLNode xDrefStartColdAndDark = missionx::RandomEngine::xRootTemplate.getChildNode (mxconst::get_ELEMENT_DATAREFS_START_COLD_AND_DARK ().c_str ()).deepCopy ();
  if (!xDrefStartColdAndDark.isEmpty () && (Utils::readBoolAttrib (xRootTemplate, mxconst::get_ATTRIB_COPY_LEG_AS_IS_B (), false) == false)) // v3.0.303 add support for special words "{navaid_lat}" and "{navaid_lon}"
  {
    // search for the first navaid and modify the start cold and dark
    if (RandomEngine::listNavInfo.size () > static_cast<size_t> (1))
    {
      // find the first NavAid that briefer is using.
      const std::string firstLeg_s = Utils::readAttrib (this->xBriefer, mxconst::get_ATTRIB_STARTING_LEG (), "");
      bool bFoundFirstLeg = false;
      auto leg            = RandomEngine::listNavInfo.begin ();
      do
      {
        bFoundFirstLeg = (leg->flightLegName == firstLeg_s);
        if (bFoundFirstLeg)
        {
          if (leg->lat != 0.0)
          {
            std::string text = Utils::xml_get_text (xDrefStartColdAndDark);
            text             = Utils::replaceString (text, "{navaid_lat}", leg->getLat (), true);
            text             = Utils::replaceString (text, "{navaid_lon}", leg->getLon (), true);

            Utils::xml_set_text (xDrefStartColdAndDark, text);
          }
        }
        else
          ++leg;
      } while (!bFoundFirstLeg); // end while
    } // end if listNavInfo.size() > 1

  } // xDrefStartColdAndDark


  // v3.0.255.3 test validity of 3D Objects. Inject warning into Briefer
  std::string localErr;
  missionx::data_manager::validate_display_object_file_existence (savePathAndFile, xFlightLegs, xGlobalSettings, x3DObjTemplate, localErr);
  if (!localErr.empty ())
  {
    RandomEngine::setError (localErr);
  }

  if (!xBrieferInfo.isEmpty ()) // v24026
  {
    if (xBrieferInfo.isAttributeSet (mxconst::get_ATTRIB_SHORT_DESC ().c_str ()))
      xBrieferInfo.deleteAttribute (mxconst::get_ATTRIB_SHORT_DESC ().c_str ()); // v24036 - delete short_desc attribute from <briefer_info> element.
  }

  // v24.12.1
  Utils::add_xml_comment (xTargetTopNode);
  xTargetTopNode.addChild (this->xMetadata);
  Utils::add_xml_comment (xTargetTopNode);
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (xBrieferInfo);
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (xGlobalSettings);
  Utils::add_xml_comment (xTargetTopNode, "++++ " + savePathAndFile + " ++++"); // v3.0.255.3 added savePathAndFile after global setting for easier debug

  xTargetTopNode.addChild (xBriefer);
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (xDrefStartColdAndDark); // v3.0.221.15 rc3.5 add start cold and dark
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (xFlightLegs);
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (xObjectives);
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (xTriggers);
  Utils::add_xml_comment (xTargetTopNode);

  Utils::add_xml_comment (xTargetTopNode, " Custom Template Messages ");
  xTargetTopNode.addChild (xMessages);
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (xInventoris); // v3.0.219.7
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (x3DObjTemplate);
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (xpData); // v3.0.221.10 can be empty element
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (this->xChoices); // v3.0.303 choices element
  Utils::add_xml_comment (xTargetTopNode);

  xTargetTopNode.addChild (xEmbedScripts); // v3.0.221.10 can be empty element
  Utils::add_xml_comment (xTargetTopNode);

  // v25.04.2 added storing the GPS Display option we picked to the GPS element
  const bool bGenerateGPS   = Utils::readBoolAttrib (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_GENERATE_GPS_WAYPOINTS (), false);
  const bool bAutoLoadRoute = missionx::system_actions::pluginSetupOptions.getNodeText_type_1_5<bool> (mxconst::get_PROP_AUTO_LOAD_ROUTE_TO_GPS_OR_FMS_B (), mxconst::DEFAULT_AUTO_LOAD_ROUTE_TO_GPS_OR_FMS_B);
  Utils::xml_set_attribute_in_node<bool> (this->xGPS, mxconst::get_PROP_GENERATE_GPS_WAYPOINTS (), bGenerateGPS, mxconst::get_ELEMENT_GPS ());
  Utils::xml_set_attribute_in_node<bool> (this->xGPS, mxconst::get_PROP_AUTO_LOAD_ROUTE_TO_GPS_OR_FMS_B (), bAutoLoadRoute, mxconst::get_ELEMENT_GPS ());
  xTargetTopNode.addChild (this->xGPS);


  // xTargetTopNode.addChild(xEnd);
  //   add end node from Template
  this->xEnd = missionx::RandomEngine::xRootTemplate.getChildNode (mxconst::get_ELEMENT_END_MISSION ().c_str ()).deepCopy (); // v3.305.1 added template read first
  if (xEnd.isEmpty ()) // try to read from mapping if we don't find one in the template
    this->xEnd = data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_END_MISSION ().c_str ()).deepCopy ();
  if (!xEnd.isEmpty ())
    xTargetTopNode.addChild (xEnd);



  Utils::xml_delete_empty_nodes (xTargetTopNode); // v3.0.219.2 remove invalid points


// v3.0.219.3 clear elements if in release
#if defined RELEASE
  std::set<std::string> setLegAttribToClean   = { { mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE () }, { mxconst::get_ATTRIB_SHARED_GOAL_TEMPLATE () }, { mxconst::get_ATTRIB_LOC_DESC () }, { mxconst::get_ATTRIB_TASK_TRIGGER_NAME () }, { mxconst::get_ATTRIB_HOVER_TIME () } };
  std::set<std::string> setPointAttribToClean = { { mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE () }, { mxconst::get_ATTRIB_SHARED_GOAL_TEMPLATE () }, { mxconst::get_ATTRIB_RADIUS_MT () }, { mxconst::get_ATTRIB_LOC_DESC () } };

  Utils::xml_delete_attribute (xTargetMainNode, setLegAttribToClean, mxconst::get_ELEMENT_LEG ());
  Utils::xml_delete_attribute (xTargetMainNode, setPointAttribToClean, mxconst::get_ELEMENT_POINT ());
#endif

  // Delete DUMMY node contents
  if (!this->xDummyTopNode.isEmpty ())
    this->xDummyTopNode.deleteNodeContent ();

  /////////////////////////////
  ////////////////////////////
  // Write to file  /////////
  IXMLRenderer  xmlWriter;
  IXMLErrorInfo errInfo = xmlWriter.writeToFile (this->xTargetMainNode, savePathAndFile.c_str (), "ASCII"); // "ISO-8859-1");
  if (errInfo != IXMLError_None)
  {
    std::string translatedError;
    translatedError.clear ();

    translatedError = IXMLRenderer::getErrorMessage (errInfo);

    setError ("[random] Error code while writing: " + translatedError + " (Check save folder is set: " + savePathAndFile + ")"); // v3.0.255.3 minor wording and save path modification

    missionx::RandomEngine::abortThread (); // v3.0.219.14
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
RandomEngine::callback_gather_random_airports_db (void *data, const int argc, char **argv, char **azColName)
{
  for (int i = 0; i < argc; i++)
  {
    RandomEngine::row_gather_db_data[azColName[i]] = argv[i] ? argv[i] : "";
  }

  resultTable_gather_random_airports[static_cast<int> (resultTable_gather_random_airports.size ())] = RandomEngine::row_gather_db_data;
  RandomEngine::row_gather_db_data.clear ();
  return 0;
}

// -----------------------------------

int
RandomEngine::callback_pick_random_ramp_location_db (void *data, const int argc, char **argv, char **azColName)
{
  for (int i = 0; i < argc; i++)
  {
    RandomEngine::row_gather_db_data[azColName[i]] = argv[i] ? argv[i] : ""; // if you have value return it if null then we put "" - empty string
  }

  resultTable_gather_ramp_data[static_cast<int> (resultTable_gather_ramp_data.size ())] = RandomEngine::row_gather_db_data;
  RandomEngine::row_gather_db_data.clear ();
  return 0;
}

// -----------------------------------
// -----------------------------------
// -----------------------------------

NavAidInfo
RandomEngine::get_random_airport_from_db (missionx::Point &inPoint, const float inMinDistance_nm, const float inMaxDistance_nm, const int inExcludeAngle, missionx::mx_base_node &inProperties, const uint8_t & in_plane_type)
{
  #ifndef RELEASE
  auto start_db_call = std::chrono::steady_clock::now ();
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
  const double pLat = inPoint.getLat ();
  const double pLon = inPoint.getLon ();

  const bool flag_is_last_flight_leg = Utils::readBoolAttrib (inProperties.node, mxconst::get_PROP_IS_LAST_FLIGHT_LEG (), false);

  //// construct view query (inner query)
  // base on xp_airports
  const std::string inner_view = fmt::format("select icao_id, icao, ap_elev_ft, ap_name, ap_type, ap_lat, ap_lon, mx_calc_distance ( ap_lat, ap_lon, {}, {}, 3440) as dist_nm, mx_bearing (ap_lat, ap_lon, {}, {}) as bearing, helipads, ramp_helos, ramp_planes, ramp_props, ramp_turboprops, ramp_jet_heavy, rw_hard, rw_dirt_gravel, rw_grass, rw_water, is_custom, is_oilrig from airports_vu ", mxUtils::formatNumber<double> (pLat, 10), mxUtils::formatNumber<double> (pLon, 10), mxUtils::formatNumber<double> (pLat, 10), mxUtils::formatNumber<double> (pLon, 10)  ); // v3.303.12 added field is_custom

  // Construct distance
  const std::string distance_s = " and dist_nm between " + mxUtils::formatNumber<float> (inMinDistance_nm) + " and " + mxUtils::formatNumber<float> (inMaxDistance_nm);

  //// Construct BEARING data
  const auto lmbda_get_bearing_string = [] (const int in_ExcludeAngle)
  {
    if (in_ExcludeAngle < 0)
      return missionx::EMPTY_STRING;

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
      return " and bearing between " + mxUtils::formatNumber<int> (excludeAngle_Right) + " and " + mxUtils::formatNumber<int> (excludeAngle_Left);

    // L < R then (between 0 and L or between R and 360)
    return " and ( bearing < " + mxUtils::formatNumber<int> (excludeAngle_Left) + " or bearing > " + mxUtils::formatNumber<int> (excludeAngle_Right) + " )";
  };

  const std::string bearing_s = lmbda_get_bearing_string (inExcludeAngle);

  //// AND Bearing construct ////
  const auto lmbda_get_plane_filter_string = [] (const missionx::mx_plane_types inPlaneType)
  {
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

  // auto ramp_type_filter_s = lmbda_get_plane_filter_string (static_cast<missionx::mx_plane_types> (this->getPlaneType ()));
  auto ramp_type_filter_s = lmbda_get_plane_filter_string (static_cast<missionx::mx_plane_types> (in_plane_type));


  // filter airports by runway types (if user asked for)
  const auto lmbda_filter_based_on_rw_type = [] (const std::string &inFilter)
  {
    std::string stmt;
    if (!inFilter.empty ())
    {
      stmt = " and 0 < ( select count(1) from xp_rw xr where xr.rw_surf in " + inFilter + " and xr.icao_id = vu.icao_id )";
    }

    return stmt;
  };

  const std::string subquery_to_filter_rw_type = lmbda_filter_based_on_rw_type (Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FILTER_AIRPORTS_BY_RUNWAY_TYPE (), ""));

  // v25.08.1 add last leg filter
  const std::string filter_by_last_leg = (flag_is_last_flight_leg)? " and is_oilrig = 0 " : "";

  // adding the rw filter to the query
  const std::string sql = "select * from ( " + inner_view + " ) vu where 1 = 1 " + distance_s + bearing_s + ramp_type_filter_s + subquery_to_filter_rw_type + filter_by_last_leg + " order by RANDOM() limit 10"; // v3.0.255.3 use RANDOM() function to pick random row  // older option: any valid row ordered by distance " order by dist_nm" ;


  #ifndef RELEASE
  Log::logMsgThread ("[get_random_airport_from_db] Query: " + sql);
  #endif // !RELEASE



  if (data_manager::db_xp_airports.db_is_open_and_ready)
  {
    char *zErrMsg = nullptr;

    // clear local cache
    RandomEngine::resultTable_gather_random_airports.clear ();
    if (int rc = sqlite3_exec (data_manager::db_xp_airports.db, sql.c_str (), RandomEngine::callback_gather_random_airports_db, nullptr, &zErrMsg); rc != SQLITE_OK)
    {
      Log::logMsgThread ("[get_random_airport_from_db] SQL error: " + std::string (zErrMsg));
      sqlite3_free (zErrMsg);
    }
    else
    {
      Log::logMsgThread ("[get_random_airport_from_db] Information was gathered.");
      #ifndef RELEASE
      for (auto &[row_num, row_data] : RandomEngine::resultTable_gather_random_airports)
      {
        Log::logMsgThread ("\tSeq: " + mxUtils::formatNumber<int> (row_num) + ", icao_id: " + row_data["icao_id"] + ", icao: " + row_data["icao"]);
      }
      #endif // !RELEASE


      // If there is data then pick a ramp
      if (!RandomEngine::resultTable_gather_random_airports.empty ())
      {

        const auto lmbda_get_ramp_filter_based_on_plane_type = [] (missionx::mx_plane_types inPlaneType)
        {
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

        auto row = (*RandomEngine::resultTable_gather_random_airports.cbegin ()).second; // fetch the first result
        nav.setID (row["icao"]);
        nav.setName (row["ap_name"]);
        nav.flag_is_custom_scenery = (!(row["is_custom"].empty ())); // v3.303.12 changed field name to is_custom

        const std::string elev_ft = row["ap_elev_ft"];
        nav.height_mt             = (elev_ft.empty ()) ? 0.0f : mxUtils::stringToNumber<float> (elev_ft) * missionx::feet2meter;

        const std::string select_s     = "select * from ramps_vu where 1 = 1 and for_planes is not null and icao_id = " + row["icao_id"]; // v3.303.14 added "for_planes is not null" to narrow the airports to the ones that there are real ramps
        const std::string filter_ramps = lmbda_get_ramp_filter_based_on_plane_type (static_cast<missionx::mx_plane_types> (in_plane_type));
        const std::string sql_query    = select_s + filter_ramps + " ORDER BY RANDOM() limit 1";

        #ifndef RELEASE
        Log::logMsgThread ("\n[pick ramp sql]\n" + sql_query + "\n"); // debug
        #endif // !RELEASE

        resultTable_gather_ramp_data.clear ();
        const int rc1 = sqlite3_exec (data_manager::db_xp_airports.db, sql_query.c_str (), RandomEngine::callback_pick_random_ramp_location_db, nullptr, &zErrMsg);
        if (rc1 != SQLITE_OK)
        {
          Log::logMsgThread ("[pick ramp] SQL error: " + std::string (zErrMsg));
          sqlite3_free (zErrMsg);
        }
        else
        {
          if (RandomEngine::resultTable_gather_ramp_data.empty ())
            Log::logMsgThread ("[pick ramp] No ramp was found.");
          else
          {
            Log::logMsgThread ("[pick ramp] Ramp info gathered.");
            auto ramp             = (*resultTable_gather_ramp_data.cbegin ()).second;
            nav.lat               = mxUtils::stringToNumber<float> (ramp["lat"], 12);
            nav.lon               = mxUtils::stringToNumber<float> (ramp["lon"], 12);
            nav.ramp_info.uq_name = ramp["name"];
            nav.ramp_info.jets    = ramp["for_planes"];

            #ifndef RELEASE
            for (auto &ramp_data : resultTable_gather_ramp_data | std::views::values)
            {
              Log::logMsgThread ("\ramp: " + ramp_data["name"] + ", icao_id: " + ramp_data["icao_id"] + ", icao: " + ramp_data["icao"]);
            }
            #endif // !RELEASE
          }
        }
      } // end if an airport result is not empty and we should search for ramp location

    } // end if a query returned value

  } // end if DB is open

#ifndef RELEASE
  auto end_db_call = std::chrono::steady_clock::now ();
  auto diff_cache  = end_db_call - start_db_call;
  auto duration    = std::chrono::duration<double, std::milli> (diff_cache).count ();
  Log::logAttention ("*** Finished get_random_airport_from_db. Duration: " + Utils::formatNumber<double> (duration, 3) + "ms (" + Utils::formatNumber<double> ((duration / 1000), 3) + "sec)  ****", true);
#endif // !RELEASE

  return nav;
}



// -----------------------------------

float
RandomEngine::calc_slope_at_point_mainThread (NavAidInfo &inNavAid)
{
  missionx::NavAidInfo north, south, east, west, ne, nw, se, sw;
  const float          radius_in_nm = 20 * missionx::meter2nm; // 20-meter radius minimal radius. which means ~80 meter of land to test

  if (inNavAid.lat == 0 || inNavAid.lon == 0)
    return false;

  //// find point for each direction
  inNavAid.synchToPoint ();

  // DEBUG

  missionx::Point::calcPointBasedOnDistanceAndBearing_2DPlane (inNavAid.p, north.p, 360, radius_in_nm);
  missionx::Point::calcPointBasedOnDistanceAndBearing_2DPlane (inNavAid.p, south.p, 180, radius_in_nm);
  missionx::Point::calcPointBasedOnDistanceAndBearing_2DPlane (inNavAid.p, east.p, 90, radius_in_nm);
  missionx::Point::calcPointBasedOnDistanceAndBearing_2DPlane (inNavAid.p, west.p, 270, radius_in_nm);

  north.p.setElevationMt (Point::getTerrainElevInMeter_FromPoint (north.p, north.p.probe_result));
  south.p.setElevationMt (Point::getTerrainElevInMeter_FromPoint (south.p, south.p.probe_result));
  east.p.setElevationMt (Point::getTerrainElevInMeter_FromPoint (east.p, east.p.probe_result));
  west.p.setElevationMt (Point::getTerrainElevInMeter_FromPoint (west.p, west.p.probe_result));


  north.syncPointToNav ();
  south.syncPointToNav ();
  east.syncPointToNav ();
  west.syncPointToNav ();


  //// calculate slope
  const double slopeNS = Utils::calcSlopeBetween2PointsWithGivenElevation (north.lat, north.lon, south.lat, south.lon, (fabs (north.p.getElevationInFeet () - south.p.getElevationInFeet ())));
  const double slopeWE = Utils::calcSlopeBetween2PointsWithGivenElevation (west.lat, west.lon, east.lat, east.lon, (fabs (west.p.getElevationInFeet () - east.p.getElevationInFeet ())));

#ifndef RELEASE
  Log::logMsgThread ("[random calc_slope_at_point] finished. Slope: " + Utils::formatNumber<double> ((slopeNS < slopeWE) ? slopeWE : slopeNS) + "\n");
#endif // !RELEASE

  return static_cast<float> ((slopeNS < slopeWE) ? slopeWE : slopeNS); // return the biggest slope angle
}


// -----------------------------------

std::string
RandomEngine::translatePlaneTypeToString (const mx_plane_types in_plane_type)
{

  if (Utils::isElementExists (RandomEngine::mapPlaneEnumToStringTypes, in_plane_type))
    return RandomEngine::mapPlaneEnumToStringTypes[in_plane_type];

  return ""; // v3.0.253.1 this->mapPlaneEnumToStringTypes[in_plane_type]; // should return empty string
}

// -----------------------------------

missionx::mx_plane_types
RandomEngine::translatePlaneTypeToEnum (const std::string &in_plane_type)
{
  if (Utils::isElementExists (RandomEngine::mapPlaneStringTypesToEnum, in_plane_type))
    return RandomEngine::mapPlaneStringTypesToEnum[in_plane_type];

  return RandomEngine::mapPlaneStringTypesToEnum[EMPTY_STRING]; // should return any
}

// -----------------------------------

bool
RandomEngine::is_plane_type_valid (const std::string &in_plane_type)
{
  return Utils::isElementExists (RandomEngine::mapPlaneStringTypesToEnum, Utils::stringToLower (in_plane_type));
}

// -----------------------------------

mx_plane_types
RandomEngine::setPlaneType (std::string inPlaneType)
{
  inPlaneType = Utils::stringToLower (inPlaneType);
  if (Utils::isElementExists (RandomEngine::mapPlaneStringTypesToEnum, inPlaneType))
  {
    missionx::RandomEngine::template_plane_type_enum = RandomEngine::mapPlaneStringTypesToEnum[inPlaneType];
    this->randomPlaneType          = inPlaneType;
  }
  else
  {
    missionx::RandomEngine::template_plane_type_enum = missionx::mx_plane_types::plane_type_any;
    this->randomPlaneType.clear ();
  }

  return missionx::RandomEngine::template_plane_type_enum;
}

// -----------------------------------

void
RandomEngine::setPlaneType (const mx_plane_types inPlaneType)
{
  this->randomPlaneType = missionx::RandomEngine::translatePlaneTypeToString (inPlaneType);
  // v3.0.253.1 extended like setPlaneType(std::string) since we need also "this->template_plane_type_enum" to be initialized when searching for ramps
  if (Utils::isElementExists (RandomEngine::mapPlaneStringTypesToEnum, this->randomPlaneType))
  {
    missionx::RandomEngine::template_plane_type_enum = RandomEngine::mapPlaneStringTypesToEnum[this->randomPlaneType];
  }
  else
  {
    Log::logMsgErr ("[setPlaneType enum] Failed to find inPlaneType, will reset to any.", true);
    RandomEngine::template_plane_type_enum = missionx::mx_plane_types::plane_type_any;
    this->randomPlaneType.clear ();
  }
}

// -----------------------------------

uint8_t
RandomEngine::getPlaneType ()
{
  return static_cast<uint8_t> (RandomEngine::template_plane_type_enum);
}


// -----------------------------------
void
RandomEngine::abortThread ()
{

  if (missionx::RandomEngine::threadState.flagIsActive)
    missionx::RandomEngine::threadState.flagAbortThread = true;
}

// -----------------------------------

void
RandomEngine::reset_sequence_numbers ()
{
  this->seq_triggers = 0;
  this->seq_tasks    = 0;
  this->seq_objectives = 0;
  this->seq_waypoints = 0; // flight leg
  this->seq_messages = 0; // messages
}

// -----------------------------------

int
RandomEngine::get_num_of_flight_legs ()
{
  int num = 0;
  if (!xFlightLegs.isEmpty ())
    num = this->xFlightLegs.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());

  return num;
}


// -----------------------------------


bool
RandomEngine::get_target (NavAidInfo &outNewNavInfo, const IXMLNode &inLegFromTemplate, const mx_plane_types in_plane_type_enum, std::map<std::string, std::string> &inMapLocationSplitValues, missionx::mx_base_node &inProperties)
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

  // std::unordered_map<std::string, std::string> config_dc;

  // read options regarding target location
  // v3.0.241.7 // v3.0.241.8 added this->flag_force_template_distances_b to let designer force his "narrative" when it comes to distances.
  const bool flag_override_random_target_min_dist = (missionx::RandomEngine::flag_force_template_distances_b) ? false : missionx::system_actions::pluginSetupOptions.getBoolValue (mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE ());

  // get hide cues option
  const auto slider_random_min_distance_value_d = Utils::getNodeText_type_1_5<double> (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_SLIDER_RANDOM_TARGET_MIN_DISTANCE (), 0.0);

  RandomEngine::errMsg.clear (); // we don't really care about the error

  // v3.0.241.10b2 read from property map to use later in code (using the node instead of the property)
  const std::string inFlightLegName      = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_NAME (), "");
  const std::string inTemplateType       = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_TYPE (), "");
  const std::string inLocationType       = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), "");
  const bool flag_is_last_flight_leg     = Utils::readBoolAttrib (inProperties.node, mxconst::get_PROP_IS_LAST_FLIGHT_LEG (), false);

  /////////////////////////////////////////////////////////////////
  // prepare local variables according to the split information
  std::string       location_value_nm_s               = mxUtils::getValueFromElement (inMapLocationSplitValues, std::string ("nm"), std::string (""));
  const std::string location_value_tag_name_s         = mxUtils::getValueFromElement (inMapLocationSplitValues, std::string ("tag"), std::string (""));
  const std::string location_value_min_max_distance_s = mxUtils::getValueFromElement (inMapLocationSplitValues, std::string ("nm_between"), std::string (""));

  // replace "_" with empty string
  if (location_value_nm_s == "_") // v3.0.221.7 if special character that represent empty
    location_value_nm_s.clear ();

  outNewNavInfo.init ();

  double location_value_d = -1.0;
  if (!location_value_nm_s.empty () && Utils::is_number (location_value_nm_s))
    location_value_d = Utils::stringToNumber<double> (location_value_nm_s, static_cast<int> (location_value_nm_s.length ()));

  Log::logDebugBO ("[get_target] nm/location_value_d: " + Utils::formatNumber<double> (location_value_d, 2), true);

  // between distances
  double location_minDistance_d = 0.0, location_maxDistance_d = 0.0;

  // lambda get_ramp_for_plane_type
  const auto lmbda_get_ramp_for_plane_type = [&] (missionx::NavAidInfo &out_new_nav_info, const float additional_search_radius_f = 10.f)
  {
    // if (!this->filterAndPickRampBasedOnPlaneType(outNewNavInfo, RandomEngine::errMsg)) // try to get Navaid information. If we fail to find information, we ignore and continue with original xPoint data
    if (!missionx::RandomEngine::filterAndPickRampBasedOnPlaneType (out_new_nav_info, RandomEngine::errMsg, missionx::mxFilterRampType::airport_ramp)) // v3.303.12_r2 // try to get Navaid information. If we fail to find information, we ignore and continue with original xPoint data
    {
      // try to gather information of navaid relative to the nearest NavAid we did found
      out_new_nav_info.synchToPoint ();
      RandomEngine::errMsg.clear ();
      RandomEngine::shared_navaid_info.inMinDistance_nm = static_cast<float> (planeLocation.calcDistanceBetween2Points (out_new_nav_info.p)); // we know that the closest distance was not good for us, so we will use it as minimal search radius
      RandomEngine::shared_navaid_info.inMaxDistance_nm = RandomEngine::shared_navaid_info.inMinDistance_nm + additional_search_radius_f; // we add 10 nautical miles to the closest NavAid we found

      if (RandomEngine::shared_navaid_info.inMinDistance_nm < RandomEngine::shared_navaid_info.inStartFromDistance_nm)
        RandomEngine::shared_navaid_info.inMinDistance_nm = RandomEngine::shared_navaid_info.inStartFromDistance_nm;

      #ifndef RELEASE
      Log::logMsgThread ("[random Lambda get ramp for plane] airport does not have valid ramps, will search for airports in distance: " + mxUtils::formatNumber<float> (RandomEngine::shared_navaid_info.inMinDistance_nm, 2) + " and " + mxUtils::formatNumber<float> (RandomEngine::shared_navaid_info.inMaxDistance_nm, 2));
      #endif // !RELEASE

      #ifdef IBM
      out_new_nav_info = this->get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, getPlaneType ()); // v3.0.255.3 test integration
      #else
      const NavAidInfo nav = missionx::RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, getPlaneType ()); // v3.0.255.3 test integration
      outNewNavInfo        = nav;
      #endif

      // Fallback code
      #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
            if (outNewNavInfo.lat == 0 || outNewNavInfo.lon == 0) // fallback code if DB logic failed
            {
              if (!this->waitForPluginCallbackJob (missionx::mx_flc_pre_command::gather_random_airport_mainThread, std::chrono::milliseconds (1000))) // pick random airport. Wait up to 10sec
              {
                RandomEngine::setError ("[random Lambda get ramp for plane Failed to find an airport in expected time. Skipping flight leg: " + inFlightLegName + "Consider sharing these findings with the developer... ");
                return false;
              }

              // Add find the closest airport to last location for location_type = NEAR
              if (inLocationType.compare (mxconst::get_EXPECTED_LOCATION_TYPE_NEAR ()) == 0)
                this->getRandomAirport_localThread (outNewNavInfo, mxconst::get_EXPECTED_LOCATION_TYPE_NEAR ());
              else
                this->getRandomAirport_localThread (outNewNavInfo); // pick random airport from list of valid locations
            } // end fallback code

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
      location_maxDistance_d += mxconst::SLIDER_MAX_RND_DIST; // increasing max distance by itself


  } // we check   location_value_min_max_distance_s only if SETUP screen do not override designer settings
  else if (!location_value_min_max_distance_s.empty ()) // min-max // // v3.0.221.7
  {
    const std::vector<double> vecMinMax = Utils::splitStringToNumbers<double> (location_value_min_max_distance_s, "-, ");
    for (size_t i1 = 0; i1 < vecMinMax.size (); ++i1)
    {
      switch (i1)
      {
        case 0:
        {
          location_minDistance_d = vecMinMax.at (i1);
        }
        break;
        case 1:
        {
          location_maxDistance_d = vecMinMax.at (i1);
        }
        break;
        default:
          break;
      } // end switch
    }
  }

  // v25.09.2
  inProperties.setStringProperty ("nm", mxUtils::getValueFromElement (inMapLocationSplitValues, std::string ("nm"), std::string (""))); // location type
  inProperties.setStringProperty ("tag", mxUtils::getValueFromElement (inMapLocationSplitValues, std::string ("tag"), std::string (""))); // location type
  inProperties.setStringProperty ("nm_between", mxUtils::getValueFromElement (inMapLocationSplitValues, std::string ("nm_between"), std::string (""))); // location type
  inProperties.setNodeProperty<double>("location_value_d", location_value_d); //
  inProperties.setNodeProperty<double>("location_min_distance_d", location_minDistance_d);
  inProperties.setNodeProperty<double>("location_max_distance_d", location_maxDistance_d);

  //////////// end variables preparations ////////////

  if ((inTemplateType == mxconst::get_FL_TEMPLATE_VAL_START ()) || (inLocationType == mxconst::get_FL_TEMPLATE_VAL_START ())) // "start"
  {
    if (missionx::RandomEngine::listNavInfo.empty ())
    {
      RandomEngine::setError ("[random get_target] Failed to find starting location element. Fix Template. Aborting.");
      return false;
    }

    outNewNavInfo = missionx::RandomEngine::listNavInfo.front ();
    outNewNavInfo.synchToPoint (true); // v25.09.1 add force init description
    return true;
  }
  // If defined nothing, then search for NEAR. Get the nearest NavAid relative to the last position if no location_value or location_tag_name were defined. Plane type is not relevant
  if (location_value_nm_s.empty () && location_value_tag_name_s.empty () && location_value_min_max_distance_s.empty () /*v3.0.241.9*/ && (missionx::RandomEngine::lastFlightLegNavInfo.lat != 0.0f && missionx::RandomEngine::lastFlightLegNavInfo.lon != 0.0f))
  {
    outNewNavInfo = missionx::RandomEngine::get_random_airport_from_db (missionx::RandomEngine::lastFlightLegNavInfo.p, 3.0f, 50.0f, -1, inProperties, getPlaneType ()); // v3.0.255.3 test integration
    if (outNewNavInfo.lat == 0 || outNewNavInfo.lon == 0) // fallback code if DB logic failed
    {
      if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_randomLastFlightLeg_mainThread))
      {
        RandomEngine::setError ("[random search nearest airport] Failed to find Airport NEAR given location. Skipping flight leg creation. ");
        return false;
      }
      outNewNavInfo = RandomEngine::shared_navaid_info.navAid;
      // v3.0.253.7 try to better fetch ramp locations if the nearest ramp is not adequate for the plane type, like Sea runway for Helos
      // v3.0.253.7 This will hopefully solve the issue that we see an ambulance waiting on the water :-)
      if (!lmbda_get_ramp_for_plane_type (outNewNavInfo))
      {
        Log::logMsgThread (RandomEngine::errMsg);
        return false;
      }
    }



    #ifndef RELEASE
    if (!RandomEngine::errMsg.empty ())
      Log::logMsgThread (errMsg);
    #endif
    RandomEngine::errMsg.clear ();

    outNewNavInfo.synchToPoint ();
    outNewNavInfo.node.updateAttribute (mxconst::get_FL_TEMPLATE_VAL_LAND ().c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE ().c_str (), mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE ().c_str ()); // override flight leg template type value to LAND since it is an icao
    outNewNavInfo.syncXmlPointToNav ();

    if (outNewNavInfo.lat == 0.0 || outNewNavInfo.lon == 0.0)
    {
      RandomEngine::setError ("[random get_target last location] Failed to find an airport in radius. Try to generate the mission.");

      return false;
    }


    return true;
  }

  if (missionx::RandomEngine::lastFlightLegNavInfo.lat != 0.0f && RandomEngine::lastFlightLegNavInfo.lon != 0.0f)
  {
    // check osm and UI template
    // DEBUG

    #ifndef RELEASE
    if (missionx::data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_user_generates_a_mission_layer) // display DEBUG info only if came from specific layer
    {
      Log::logMsgThread ("Use OSM: " + Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_OSM_CHECKBOX (), "NO"));
      Log::logMsgThread ("Use WEB OSM: " + Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_WEB_OSM_CHECKBOX (), "NO"));
    }
    #endif
    // if "user generates a mission" and they base it on Web/OSM and the plane is Helos, and it is a medevac and not last flight leg
    if (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_user_generates_a_mission_layer && (Utils::readBoolAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_OSM_CHECKBOX (), false) || Utils::readBoolAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_WEB_OSM_CHECKBOX (), false)) && missionx::RandomEngine::template_plane_type_enum == missionx::mx_plane_types::plane_type_helos && Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::not_defined)) == static_cast<int> (missionx::mx_ui_mission_type::medevac) && !flag_is_last_flight_leg)
    {
      // get max radius and find the 4 points that create the rectangle area
      const auto maxRadius_d   = Utils::readNodeNumericAttrib<double> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MAX_DISTANCE_SLIDER (), static_cast<int> (mxconst::SLIDER_MAX_RND_DIST / 2));
      const auto minDistance_d = Utils::readNodeNumericAttrib<double> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MIN_DISTANCE_SLIDER (), location_minDistance_d);

      Point E90, W270, S180, N0;
      missionx::RandomEngine::calculate_bbox_coordinates (N0, S180, E90, W270, RandomEngine::lastFlightLegNavInfo.lat, RandomEngine::lastFlightLegNavInfo.lon, maxRadius_d);

      if (NavAidInfo navAid; missionx::RandomEngine::osm_get_navaid_from_osm (navAid, inMapLocationSplitValues, inProperties, RandomEngine::lastFlightLegNavInfo.lat, RandomEngine::lastFlightLegNavInfo.lon, S180.lat, N0.lat, W270.lon, E90.lon, maxRadius_d, minDistance_d))
      {
        if (navAid.lat != 0.0 && navAid.lon != 0.0)
        {
          outNewNavInfo = navAid;
          outNewNavInfo.synchToPoint ();
          // missionx::RandomEngine::flag_picked_from_osm_database = true; // we can use this
          return true;
        }
      }

      // if OSM data was not found then plugin will try to use the default target search
    }



    if (inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OILRIG ()) // v3.303.14 add OIL-RIG type
    {
      outNewNavInfo.lat = Utils::readNodeNumericAttrib<float> (inLegFromTemplate, mxconst::get_ATTRIB_LAT (), 0.0f);
      outNewNavInfo.lon = Utils::readNodeNumericAttrib<float> (inLegFromTemplate, mxconst::get_ATTRIB_LONG (), 0.0f);
      outNewNavInfo.setID (Utils::readAttrib (inLegFromTemplate, mxconst::get_ATTRIB_ICAO_ID (), ""));
      outNewNavInfo.setName (Utils::readAttrib (inLegFromTemplate, mxconst::get_ATTRIB_AP_NAME (), ""));

      if (outNewNavInfo.lat != 0.0f && outNewNavInfo.lon != 0.0)
        return true;
      else
        return false;
    }

    if ((inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM ()) || (inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ()) || (location_value_tag_name_s.empty () && ((location_value_d > 0.0) || (location_minDistance_d > 0.0 && location_maxDistance_d > 0.0)))) // v3.0.255.3 changed last logic
    {
      // Should we pick a random location for HELOS
      if ((inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_XY () || inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM () || inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ()) && missionx::RandomEngine::template_plane_type_enum == missionx::mx_plane_types::plane_type_helos && !flag_is_last_flight_leg)
      {
        // return get_targetForHelos_base_XY_OSM_OSMWEB (outNewNavInfo, in_plane_type_enum, inMapLocationSplitValues, inProperties, location_value_d, location_minDistance_d, location_maxDistance_d);
        return get_targetForHelos_base_XY_OSM_OSMWEB (outNewNavInfo, in_plane_type_enum, inMapLocationSplitValues, inProperties);
      }

      return get_target_or_lastFlightLeg_base_on_XY_or_OSM (outNewNavInfo, inMapLocationSplitValues, inProperties);
      // v25.09.2 commented code
      // else
      // {
      //   // return get_target_or_lastFlightLeg_base_on_XY_or_OSM (outNewNavInfo, inMapLocationSplitValues, inProperties, location_value_d, location_minDistance_d, location_maxDistance_d);
      //   return get_target_or_lastFlightLeg_base_on_XY_or_OSM (outNewNavInfo, inMapLocationSplitValues, inProperties);
      // } // end handle random x/y or random navaid

      // return false;
    } // end handle random x/y or random navaid

    if (!location_value_tag_name_s.empty ())
    {

      return this->get_target_base_on_tag_name (outNewNavInfo, in_plane_type_enum, inProperties, location_value_tag_name_s, location_value_d, location_minDistance_d, location_maxDistance_d);

    } // end if has tag name
  }
  // end if lastLegNav have values

  return false;
}

// -----------------------------------

double
RandomEngine::get_slope_at_point (const missionx::NavAidInfo &outNavAid)
{
  missionx::RandomEngine::threadState.pipeProperties.setNodeProperty<float> (mxconst::get_ATTRIB_LAT (), outNavAid.lat);
  missionx::RandomEngine::threadState.pipeProperties.setNodeProperty<float> (mxconst::get_ATTRIB_LONG (), outNavAid.lon);
  RandomEngine::shared_navaid_info.p = outNavAid.p;

  double found_slope_d = 0.0;
  if (missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::calculate_slope_for_build_flight_leg_thread))
    found_slope_d = missionx::RandomEngine::threadState.pipeProperties.getAttribNumericValue<double> (mxconst::get_ATTRIB_TERRAIN_SLOPE (), 0.0); // v3.305.1 updated

  RandomEngine::errMsg.clear ();
  return found_slope_d;
}

// -----------------------------------

bool
RandomEngine::get_is_wet_at_point (const missionx::NavAidInfo &inNavAid)
{
  RandomEngine::shared_navaid_info.p = inNavAid.p;
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_is_point_wet))
  {
    RandomEngine::setError ("[random isWet] Failed to probe for wet. Will treat target coordinates as \"land\". ");
  }

  return RandomEngine::shared_navaid_info.isWet;
}

// -----------------------------------

float
RandomEngine::get_terrain_elevation_at_point_in_mt (const missionx::NavAidInfo &inNavAid, random_airport_info_struct &inout_airport_info_struct)
{
  RandomEngine::shared_navaid_info.p = inNavAid.p;
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_terrain_elev_in_point))
  {
    RandomEngine::setError (fmt::format("[{}] Failed to probe for terrain elevation. Will treat target terrain elevation as \"Zero\". ", __func__) );
  }

  return static_cast<float>( RandomEngine::shared_navaid_info.p.getElevationInMeters () );
}

// -----------------------------------


std::string
RandomEngine::prepare_message_with_special_keywords (missionx::NavAidInfo &inNavAid, const std::string &inMessage)
{
  // std::string resultMessage = std::move(inMessage);

  // v3.0.253.4 calculate bearing
  // calculate bearing
  if (!RandomEngine::listNavInfo.empty ())
  {
    // currently we do not handle skewed location since we provide a bearing to the next target and not exact location
    NavAidInfo &prevNav  = RandomEngine::listNavInfo.back ();
    prevNav.bearing_next = static_cast<float> (Utils::calcBearingBetween2Points (prevNav.lat, prevNav.lon, inNavAid.lat, inNavAid.lon));


    inNavAid.bearing_to_current_target   = prevNav.bearing_next;
    inNavAid.bearing_back_to_prev_target = (prevNav.bearing_next + 180.0f > 359.0f) ? prevNav.bearing_next - 180.0f : prevNav.bearing_next + 180.0f;
  }


  // 3.0.241.8 Calculate distance between 2 NavAid
  const auto lmbda_get_distance_between_2_nav_points = [] (NavAidInfo &inTargetNavAid, const std::list<missionx::NavAidInfo> &listNavInfo)
  {
    NavAidInfo prevNav;
    if (!listNavInfo.empty ())
      prevNav = listNavInfo.back ();

    if (prevNav.lat != 0.0f && prevNav.lon != 0.0f)
    {
      std::string err;
      return inTargetNavAid.p - prevNav.p; // v25.06.1 calculate distance using operator
      // return inTargetNavAid.p.calcDistanceBetween2Points (prevNav.p);
    }

    return -1.0; // distance did not calculated due to NavAid targetLat/targetLon values not correct
  };

  inNavAid.fpln_distance_between_prev_and_current_navaid = lmbda_get_distance_between_2_nav_points (inNavAid, RandomEngine::listNavInfo);

  #ifndef RELEASE
  Log::logMsg ("[prepare message] Distance to prev NavAid: " + Utils::formatNumber<double> (inNavAid.fpln_distance_between_prev_and_current_navaid, 0) + "nm", true);
  #endif

  return RandomEngine::gen_message_with_special_keywords_static (inMessage, inNavAid);
  // if (!inMessage.empty ())
  // {
  //   return RandomEngine::gen_message_with_special_keywords_static (inMessage, inNavAid);
    // std::map<std::string, std::string> mapReplaceMessageKeywords; // v3.0.221.11 keyword, value from Navaid
    //
    // //// v3.0.221.11 refine Flight Leg message
    // mapReplaceMessageKeywords["{navaid_name}"]     = std::string (inNavAid.name);
    // mapReplaceMessageKeywords["{navaid_icao}"]     = std::string (inNavAid.ID);
    // mapReplaceMessageKeywords["{navaid_lat}"]      = inNavAid.getLat ();
    // mapReplaceMessageKeywords["{navaid_lon}"]      = inNavAid.getLon ();
    // mapReplaceMessageKeywords["{bearing_target}"]  = mxUtils::formatNumber<float> (inNavAid.bearing_to_current_target, 0);
    // const auto elev_ft_s                           = Utils::formatNumber<float> (inNavAid.height_mt * missionx::meter2feet);
    // mapReplaceMessageKeywords["{navaid_elev}"]     = (inNavAid.height_mt == 0.0f) ? "" : elev_ft_s;
    // // mapReplaceMessageKeywords["{navaid_loc_desc}"] = (inNavAid.loc_desc.empty ()) ? inNavAid.init_locDesc () : inNavAid.loc_desc;
    // mapReplaceMessageKeywords["{navaid_loc_desc}"] = inNavAid.get_loc_desc (); // v25.06.1 TODO: use this function before adding the NavAid to the target list
    // mapReplaceMessageKeywords["{distance}"]        = (distance_d < 0.0) ? "n/a" : (Utils::formatNumber<double> (distance_d, 0) + "nm"); // v3.0.241.8
    //
    // for (const auto &[stringToModify, stringToReplaceWith] : mapReplaceMessageKeywords) // replace all special keywords
    // {
    //   inMessage = Utils::replaceString (inMessage, stringToModify, stringToReplaceWith, true);
    // }
  // }
  // else
  //   Log::logMsgWarn ("[random special_keywords] Received empty message. Skipping...", true);
  //
  // return inMessage;
}

// -----------------------------------

bool
RandomEngine::prepare_blank_template_with_flight_legs_based_on_ui (IXMLNode &pNode, IXMLNode &outMetaNode, std::string &outErr)
{

  std::string location_value_s;
  outErr.clear ();

  // Gather information from UI layer
  const auto med_cargo_or_oilrig_i             = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::not_defined)); // 0 = med, 1 = cargo
  const auto mission_subcategory_indx_picked_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MISSION_SUBCATEGORY (), static_cast<int> (missionx::mx_mission_subcategory_type::not_defined)); //
  const auto uiLayer_debug                     = data_manager::getGeneratedFromLayer (); // v25.02.1

  const std::string CATEGORY_TRANSLATION = missionx::data_manager::get_translate_of_mission_subcategory_code (med_cargo_or_oilrig_i, mission_subcategory_indx_picked_i, outMetaNode); // v3.303.14

  outMetaNode.updateAttribute (CATEGORY_TRANSLATION.c_str (), mxconst::get_ATTRIB_CATEGORY ().c_str (), mxconst::get_ATTRIB_CATEGORY ().c_str ());
  outMetaNode.updateAttribute (mxUtils::formatNumber<int> (med_cargo_or_oilrig_i).c_str (), mxconst::get_PROP_MED_CARGO_OR_OILRIG ().c_str (), mxconst::get_PROP_MED_CARGO_OR_OILRIG ().c_str ());
  outMetaNode.updateAttribute (mxUtils::formatNumber<int> (mission_subcategory_indx_picked_i).c_str (), mxconst::get_PROP_MISSION_SUBCATEGORY ().c_str (), mxconst::get_PROP_MISSION_SUBCATEGORY ().c_str ());


  auto       plane_type_i        = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types::plane_type_props)); // plane type
  const auto no_of_legs_i        = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_NO_OF_LEGS (), 2); // no of legs
  auto       min_distance_slider = Utils::readNodeNumericAttrib<double> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MIN_DISTANCE_SLIDER (), 5.0); // min slider
  auto       max_distance_slider = Utils::readNodeNumericAttrib<double> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MAX_DISTANCE_SLIDER (), 45.0); // max slider

  // Validations
  assert ((!pNode.isEmpty () && !data_manager::prop_userDefinedMission_ui.node.isEmpty ()) && "Empty template or prop_userDefinedMission_ui are empty!"); // debug
  assert (med_cargo_or_oilrig_i > static_cast<int> (missionx::mx_ui_mission_type::not_defined) && ": Main Mission Type can't be undefined. Aborting!!!"); // debug
  assert (CATEGORY_TRANSLATION.empty () == false && ": Sub Category was not found. Aborting!!!"); // debug


  if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::oil_rig)) // v3.303.14 oilrig mission must be helos plane type
  {
    plane_type_i = static_cast<int> (missionx::mx_plane_types::plane_type_helos);
  }

  auto        conv_plane_type_i = static_cast<missionx::_mx_plane_type> (plane_type_i);
  std::string plane_type_s      = missionx::RandomEngine::translatePlaneTypeToString (conv_plane_type_i);

  // Store plane type in the XML node
  missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_PLANE_TYPE_S (), plane_type_s);
  pNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ());
  outMetaNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ()); // v25.05.1

  const auto lmbda_get_ramp_type_H_or_S = [] (auto in_plane_type_i)
  {
    if (in_plane_type_i == static_cast<int> (missionx::mx_plane_types::plane_type_prop_floats) || in_plane_type_i == static_cast<int> (missionx::mx_plane_types::plane_type_ga_floats))
      return "|ramp=S"; // S = Seaports

    if (in_plane_type_i == static_cast<int> (missionx::mx_plane_types::plane_type_helos))
      return "|ramp=H"; // H = Helos

    return "";
  };

  const std::string ramp_type_s = lmbda_get_ramp_type_H_or_S (plane_type_i);


  // Anonymous Block
  {
    // create legs according to mission type: medevac, cargo or oilrig
    for (int i1 = 1; i1 <= no_of_legs_i; ++i1)
    {
      // decide on tag name to pick. leg_medevac / leg_cargo / leg_oilrig.
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

      tag_name += (plane_type_i == static_cast<int> (missionx::mx_plane_types::plane_type_helos)) ? "_helos" : "_plane";
      // End result should be: "leg_medevac_helos" or "leg_oilrig_helos" or "leg_cargo_plane" etc...

      RandomEngine::map_flight_legs_translation_from_template[i1] = tag_name; // v25.09.1

      if (IXMLNode node = missionx::data_manager::xmlMappingNode.getChildNode (tag_name.c_str ()).deepCopy ();
        node.isEmpty ())
      {
        outErr = "Could not find the mapping node: " + tag_name + ", aborting generating mission template.";
        return false;
      }
      else
      {
        node.updateName (mxconst::get_ELEMENT_LEG ().c_str ());
        const std::string legName = std::string (mxconst::get_ELEMENT_LEG ()) + "_" + Utils::formatNumber<int> (i1);
        Utils::xml_set_attribute_in_node_asString (node, mxconst::get_ATTRIB_NAME (), legName, mxconst::get_ELEMENT_LEG ());

        if (i1 < no_of_legs_i || no_of_legs_i == 1)
        {
          std::string location_min_distance_s = Utils::formatNumber<double> (min_distance_slider, 0);
          std::string location_max_distance_s = Utils::formatNumber<double> (max_distance_slider, 0);
          if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::medevac)) // 0 = medical, 1 = cargo, 2 = oilrig
            location_value_s = std::string ("nm=").append (location_max_distance_s).append (ramp_type_s);
          else if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::cargo)) // 0 = medical, 1 = cargo, 2 = oilrig
            location_value_s = std::string ("nm_between=").append (location_min_distance_s).append ("-").append (location_max_distance_s).append (ramp_type_s);
          else if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::oil_rig)) // 0 = medical, 1 = cargo, 2 = oilrig
            location_value_s = std::string ("nm_between=5-80").append (ramp_type_s);

          // v25.08.1 support for "location_properties" attribute that will replace "location_value"
          node.getChildNode (mxconst::get_ELEMENT_EXPECTED_LOCATION ().c_str ()).updateAttribute (location_value_s.c_str (), mxconst::get_ATTRIB_LOCATION_PROPERTIES ().c_str (), mxconst::get_ATTRIB_LOCATION_PROPERTIES ().c_str ());
          // v25.08.1 TODO: deprecate the use of "get_ATTRIB_LOCATION_VALUE" attribute
          node.getChildNode (mxconst::get_ELEMENT_EXPECTED_LOCATION ().c_str ()).updateAttribute (location_value_s.c_str (), mxconst::get_ATTRIB_LOCATION_VALUE ().c_str (), mxconst::get_ATTRIB_LOCATION_VALUE ().c_str ());
        }
        pNode.addChild (node.deepCopy ()); // add the node to template in memory
      }
    }
  }

  // TODO: v25.05.1 the briefer skeleton message needs to be override with the "surprise me" option.
  briefer_skeleton_message_to_use_in_injectTypeMissionFeature = "Hello Pilot\n";

  briefer_skeleton_message_to_use_in_injectTypeMissionFeature += (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::medevac)) ? "You have been assigned to a medevac mission. " : (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::cargo)) ? "You have been assigned to a cargo flight. " : (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::oil_rig)) ? "You have been assigned to an oilrig flight. " : "You have been assigned to a flight. ";

  briefer_skeleton_message_to_use_in_injectTypeMissionFeature += fmt::format ("Your expected transportation is a {}.\n", (conv_plane_type_i == missionx::_mx_plane_type::plane_type_helos) ? "helo" : plane_type_s);

  return true;
}

// -----------------------------------

bool
RandomEngine::prepare_mission_based_on_external_fpln (IXMLNode &pNode)
{
  assert (!pNode.isEmpty () && !data_manager::prop_userDefinedMission_ui.node.isEmpty () && "Empty template or prop_userDefinedMission_ui are empty!");
  auto plane_type_i     = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types::plane_type_props)); // plane type
  auto fpln_id_picked_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FPLN_ID_PICKED (), -1); // max slider

  if (fpln_id_picked_i < 0 || Utils::isElementExists (missionx::data_manager::indexPointer_forExternalFPLN_tableVector, fpln_id_picked_i) == false)
  {
    RandomEngine::setError ("Could not find the flight plan with index id: " + Utils::formatNumber<int> (fpln_id_picked_i) + ", aborting mission template generating.");
    return false;
  }

  // plane type
  auto conv_plane_type_i = static_cast<missionx::_mx_plane_type> (plane_type_i);
  this->setPlaneType (conv_plane_type_i); // set plane type in class level for other function too

  // fetch the fpln struct to work with
  auto const lmbda_get_fpln = [] (const int inPicked_id, const std::vector<missionx::mx_ext_internet_fpln_strct> &inFPLN_vec) // missionx::data_manager::tableExternalFPLN_vec
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

  if (auto fpln = lmbda_get_fpln (fpln_id_picked_i, missionx::data_manager::tableExternalFPLN_vec); fpln.internal_id > -1)
  {
    // NavAidInfo prev_na; // v25.04.2 deprecated, not used
    // create navaids based on polyline. First is starting point + briefer and last is target (mandatory)
    int counter = 0;
    for (auto &wp : fpln.listNavPoints)
    {
      NavAidInfo na;
      na.lat = wp.lat;
      na.lon = wp.lon;

      if (counter == 0 || counter == static_cast<int> (fpln.listNavPoints.size () - 1)) // Fetch Navaid data for first and last waypoints
      {
        RandomEngine::shared_navaid_info.navAid.init ();
        RandomEngine::shared_navaid_info.navAid.lat = na.lat;
        RandomEngine::shared_navaid_info.navAid.lon = na.lon;
        if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
        {
          RandomEngine::setError ("[random prepare fpln from external fpln] Navaid: " + Utils::formatNumber<int> (counter) + " Failed to find Airport NEAR given location. Still using original Navaid.");
        }
        na.synchToPoint ();
        RandomEngine::shared_navaid_info.navAid.synchToPoint ();
        if (const auto distance = na.p.calcDistanceBetween2Points (RandomEngine::shared_navaid_info.navAid.p); distance <= 2.0) // if navaid within 2nm
        {
          na.clone (RandomEngine::shared_navaid_info.navAid); // v25.04.2 changed code to clone
          na.synchToPoint ();
        }

        if (counter == 0)
        {
          if (na.getName ().empty ())
            na.setName (mxconst::get_ELEMENT_BRIEFER ());

          if (missionx::RandomEngine::get_user_wants_to_start_from_plane_position ()) // v3.0.253.11 extend the starting option to plane position also
          {
            // reset starting point
            na.lat = static_cast<float> (RandomEngine::planeLocation.getLat ());
            na.lon = static_cast<float> (RandomEngine::planeLocation.getLon ());
            na.heading = static_cast<float> (RandomEngine::planeLocation.getHeading ());
            na.synchToPoint ();
          }
        }

        // try to locate a ramp or starting point
        if (counter > 0 || (counter == 0 && !missionx::RandomEngine::get_user_wants_to_start_from_plane_position ()))
        {
          if (std::string err; !filterAndPickRampBasedOnPlaneType (na, err, missionx::mxFilterRampType::start_ramp)) // v3.303.12_r2
          {
            Log::logMsgThread (fmt::format ("[{}] {}", __func__, err));
          }
        }
      }
      else
        na.synchToPoint ();


      // v3.0.255.4 add the guessed GPS locations
      if (counter > 0 && counter < static_cast<int> (fpln.listNavPoints.size () - static_cast<size_t> (1)) && fpln.listNavPointsGuessedName.size () >= fpln.listNavPoints.size ())
      {
        // loop over list and pick the counter element
        int internalCounter = 0;
        for (const auto &np_guess : fpln.listNavPointsGuessedName)
        {
          if (internalCounter == counter)
          {
            if (np_guess.name.empty ())
              break; // exit loop and do nothing
            else
            {
              if (na.getID ().empty ())
                na.setID (np_guess.name);
              na.navType = (np_guess.nav_type >= 0) ? np_guess.nav_type : na.navType;
              na.navRef  = np_guess.nav_ref;
              na.synchToPoint ();
              break; // exit loop, found item
            }
          }

          ++internalCounter;
        } // end loop over all listNavPointsGuessedName
      } // end inject guessed NavAid names


      RandomEngine::lastFlightLegNavInfo = na;
      missionx::RandomEngine::listNavInfo.emplace_back (na); // add NavInfo into a list

      // Add to GPS
      if (!xGPS.isEmpty ())
        xGPS.addChild (na.node.deepCopy ());

      // prev_na.clone ( na ); // v25.04.2 deprecated, not in use
      counter++;

    } // end loop over waypoints and gathering NavAid info

    if (missionx::RandomEngine::listNavInfo.size () < 2)
    {
      RandomEngine::setError ("Not enough flight leg information for flight plan id: " + Utils::formatNumber<int> (fpln_id_picked_i) + ", needs at least 2. Aborting mission creation.");
      return false;
    }

    //////////////////////
    // Prepare Main Nodes
    /////////////////////
    std::string plane_type_s = missionx::RandomEngine::translatePlaneTypeToString (conv_plane_type_i); // convert type to string and store it in mission node
    missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_PLANE_TYPE_S (), plane_type_s); //, data_manager::prop_userDefinedMission_ui.node, data_manager::prop_userDefinedMission_ui.node.getName());
    pNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ());

    IXMLNode xLegNode    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_LEG ().c_str ()).deepCopy ();
    IXMLNode xMapTask    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TASK ().c_str ()).deepCopy ();
    IXMLNode xMapTrigger = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str ()).deepCopy ();
    IXMLNode xMapMessage = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE (), true); // holds message element from MAPPING


    // Prepare Briefer
    NavAidInfo naBriefer       = missionx::RandomEngine::listNavInfo.front ();
    IXMLNode   xLocationAdjust = missionx::RandomEngine::xRootTemplate.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ().c_str ()).deepCopy ();
    if (xLocationAdjust.isEmpty ())
    {
      RandomEngine::setError ("[random] No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION () + "> was found. Template malformed, abort template generation !!!");

      return false;
    }

    xLocationAdjust.updateName (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ());
    // remove any clear data
    int               nClear      = xLocationAdjust.nClear (); // remove any CDATA or COMMENTS or any clear() type element
    const std::string from_to_s   = get_short_flight_description_from_to (fpln.fromName_s, fpln.fromICAO_s, fpln.toName_s, fpln.toICAO_s); //"From: " + fpln.fromName_s + "(" + fpln.fromICAO_s + ") to " + fpln.toName_s + "(" + fpln.toICAO_s + ")";
    const std::string brieferDesc = from_to_s + "\n\n" + "Hello pilot.\nYou have assigned a flight that was generated from \"flightplandatabase.com\". Learn the route and fly it according to the flight plan or modify it if you so wish.\n\nBlue skys.";
    const std::string notes       = (fpln.notes_s.empty ()) ? "" : "\n\nnotes:\n" + fpln.notes_s; // add notes if any from flight plan


    for (int i = 0; i < nClear; ++i)
      xLocationAdjust.deleteClear (); // change from remove "i" to remove first

    xLocationAdjust.updateAttribute (naBriefer.getLat ().c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
    xLocationAdjust.updateAttribute (naBriefer.getLon ().c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
    xLocationAdjust.updateAttribute (naBriefer.getHeading_s ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());
    xLocationAdjust.updateAttribute (naBriefer.getRampInfo ().c_str (), mxconst::get_ATTRIB_RAMP_INFO ().c_str (), mxconst::get_ATTRIB_RAMP_INFO ().c_str ());

    RandomEngine::lastFlightLegNavInfo = naBriefer;
    if (naBriefer.getName ().empty ()) // v3.303.10
      RandomEngine::lastFlightLegNavInfo.flightLegName = mxconst::get_ELEMENT_BRIEFER ();

    RandomEngine::lastFlightLegNavInfo.synchToPoint ();

    this->xBriefer = this->xDummyTopNode.addChild (mxconst::get_ELEMENT_BRIEFER ().c_str ());
    this->xBriefer.addAttribute (mxconst::get_ATTRIB_STARTING_LEG ().c_str (), "leg_1"); // leg1 is default value, but it can be changed when using <content> elements with "leg sets"
    IXMLNode cNode = xBriefer.addChild (xLocationAdjust);
    Utils::xml_add_cdata (this->xBriefer, brieferDesc + notes + "\n\n==== suggested waypoints ====\n" + fpln.formated_nav_points_with_guessed_names_s); // v3.0.241.1 // V3.0.255.2 Added gussed waypoints

    // Add inventory if exists in mapping
    if (data_manager::xmlMappingNode.nChildNode (mxconst::get_ELEMENT_INVENTORY ().c_str ()) > 0)
    {
      this->addInventory (mxconst::get_ELEMENT_BRIEFER (), naBriefer.p.node, mxInvSource::point); // name of store will start with briefer
    }


    //// Finished Briefer construction ////


    // Prepare Objective + Tasks + Triggers and Leg flight plan
    if (xLegNode.isEmpty ())
    {
      RandomEngine::setError ("Could not find the mapping node: LEG, aborting mission template generating.");
      return false;
    }
    else
    {
      const std::string legName       = std::string (mxconst::get_ELEMENT_LEG ()) + "_" + Utils::formatNumber<int> (fpln_id_picked_i);
      const std::string objectiveName = legName + "_objective";

      IXMLNode xObjective = this->xObjectives.addChild (mxconst::get_ELEMENT_OBJECTIVE ().c_str ());
      xObjective.updateAttribute (objectiveName.c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
      // create tasks based on waypoint list, except the first one (which is the briefer)
      counter = 0;
      for (auto &na : RandomEngine::listNavInfo)
      {
        counter++;
        if (counter == 1)
          continue; // it is the briefer starting location

        std::string task_name    = "task_" + Utils::formatNumber<int> (counter);
        std::string trigger_name = "trig_" + task_name;

        IXMLNode xTask = xObjective.addChild (mxconst::get_ELEMENT_TASK ().c_str ());
        xTask.updateAttribute (task_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
        xTask.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str ());
        xTask.updateAttribute ("3", mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC ().c_str (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC ().c_str ()); // evaluate success for 3 seconds
        xTask.updateAttribute (((counter == static_cast<int> (RandomEngine::listNavInfo.size ())) ? "yes" : ""), mxconst::get_ATTRIB_MANDATORY ().c_str (), mxconst::get_ATTRIB_MANDATORY ().c_str ()); // evaluate success for 3 seconds


        // add the trigger
        IXMLNode xTrigger = xMapTrigger.deepCopy ();
        xTrigger.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
        xTrigger.updateAttribute ("rad", mxconst::get_ATTRIB_TYPE ().c_str (), mxconst::get_ATTRIB_TYPE ().c_str ()); // set type as radius based "rad".
        Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LAT (), na.getLat (), mxconst::get_ELEMENT_POINT ());
        Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LONG (), na.getLon (), mxconst::get_ELEMENT_POINT ());

        if (counter == static_cast<int> (RandomEngine::listNavInfo.size ()))
        {
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT (), "100", mxconst::get_ELEMENT_RADIUS ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "true", mxconst::get_ELEMENT_CONDITIONS ());
        }
        else
        {
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT (), "4000", mxconst::get_ELEMENT_RADIUS ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "", mxconst::get_ELEMENT_CONDITIONS ());
        }

        this->xTriggers.addChild (xTrigger);
      } // end loop over all waypoints and creating tasks from them

      ////// Construct Flight Leg //////
      static const std::string STARTING_MESSAGE_NAME = "starting_message";
      const std::string        leg_message_name_s    = STARTING_MESSAGE_NAME + "_" + Utils::formatNumber<int> (counter);

      xLegNode.updateAttribute (legName.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
      xLegNode.updateAttribute (from_to_s.c_str (), mxconst::get_ATTRIB_TITLE ().c_str (), mxconst::get_ATTRIB_TITLE ().c_str ());
      Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME (), objectiveName, mxconst::get_ELEMENT_LINK_TO_OBJECTIVE ()); // link to objective
      Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME (), leg_message_name_s, mxconst::get_ELEMENT_START_LEG_MESSAGE ()); // link to objective

      // Add message to flight leg
      IXMLNode xMessage01 = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_MESSAGE ().c_str ()).deepCopy ();
      if (!xMessage01.isEmpty ())
      {
        xMessage01.updateAttribute (leg_message_name_s.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
        IXMLNode mixText = Utils::xml_get_or_create_node_ptr (xMessage01, mxconst::get_ELEMENT_MIX ());
        mixText.updateAttribute ("text", mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE ().c_str (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE ().c_str ());
        const std::string text = "Hello pilot\nYou will fly the route " + from_to_s + ". \n\nGood Luck";
        Utils::xml_add_cdata (mixText, text);

        this->xMessages.addChild (xMessage01);
      }

      // Add Ending Marker
      IXMLNode xDisplayEndLocation = xLegNode.addChild (mxconst::get_ELEMENT_DISPLAY_OBJECT ().c_str ());
      if (!xDisplayEndLocation.isEmpty ())
      {
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_INSTANCE_NAME ().c_str (), std::string ("marker_" + legName).c_str ());
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_NAME ().c_str (), "marker"); // this is the name of the marker in the "template_blank_4_ui.xml" file
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_TARGET_MARKER_B ().c_str (), "true");


        NavAidInfo naLast = RandomEngine::listNavInfo.back ();
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LAT ().c_str (), naLast.getLat ().c_str ());
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LONG ().c_str (), naLast.getLon ().c_str ());
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT ().c_str (), "50"); // display marker 50ft above ground
      }

      // Add Flight Leg DESCRIPTION
      IXMLNode xDesc = xLegNode.getChildNode (mxconst::get_ELEMENT_DESC ().c_str ());
      if (xDesc.isEmpty ())
        xDesc = xLegNode.addChild (mxconst::get_ELEMENT_DESC ().c_str ());

      Utils::xml_add_cdata (xDesc, brieferDesc + notes + "\n\n==== suggested waypoints ====\n" + fpln.formated_nav_points_with_guessed_names_s); // v3.0.241.1 // V3.0.255.2 Added gussed waypoints


      // We only have one leg
      this->mission_xml_data.currentLegName = legName;
      Utils::xml_add_node_to_element_IXMLNode (xFlightLegs, xLegNode);
      Utils::addElementToMap (mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, 1);
      Utils::addElementToMap (mapFLightPlanOrder_is, 1, this->mission_xml_data.currentLegName);

    } // end creating the flight leg
  }
  else
  {
    RandomEngine::setError ("Flight plan is invalid. Index id: " + Utils::formatNumber<int> (fpln_id_picked_i) + ", aborting mission template generating.");
  }

  return true;
}



// -----------------------------------



bool
RandomEngine::prepare_mission_based_on_ils_search (IXMLNode &pNode)
{
  assert (!pNode.isEmpty () && !data_manager::prop_userDefinedMission_ui.node.isEmpty () && "Empty template or prop_userDefinedMission_ui are empty!");

  const auto plane_type_i     = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types::plane_type_props)); // plane type
  const auto fpln_id_picked_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FPLN_ID_PICKED (), -1); // max slider
  auto       fromICAO         = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FROM_ICAO (), "");
  auto       toICAO           = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_TO_ICAO (), "");

  if (fpln_id_picked_i < 0 || Utils::isElementExists (missionx::data_manager::indexPointer_for_ILS_rows_tableVector, fpln_id_picked_i) == false)
  {
    RandomEngine::setError ("Could not find the ILS flight plan with index id: " + Utils::formatNumber<int> (fpln_id_picked_i) + ", aborting mission template generating.");
    return false;
  }
  if (fromICAO.empty ())
  {
    RandomEngine::setError ("[Random ILS Error] No source ICAO was found, aborting mission template generating.");
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
    RandomEngine::setError ("ILS Flight plan is invalid. Index id: " + Utils::formatNumber<int> (fpln_id_picked_i) + ", aborting mission template generating.");
  }
  else
  {

    RandomEngine::shared_navaid_info.navAid.init ();
    RandomEngine::shared_navaid_info.navAid.setID (fromICAO);
    if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
    {
      RandomEngine::setError ( fmt::format( "[{}] Start Navaid: {}. Failed to find Airport using original Navaid. Notify developer.", __func__, fromICAO) );
      return false;
    }
    RandomEngine::shared_navaid_info.navAid.synchToPoint ();
    // if we reached here then we should have startICAO NavAid information and the targetICAO
    NavAidInfo start_na = RandomEngine::shared_navaid_info.navAid;


    // v3.0.253.11 force plane position as starting location
    if (missionx::RandomEngine::get_user_wants_to_start_from_plane_position ()) // v3.0.253.11
    {
      start_na.lat = static_cast<float> (RandomEngine::planeLocation.getLat ());
      start_na.lon = static_cast<float> (RandomEngine::planeLocation.getLon ());
      start_na.heading = static_cast<float> (RandomEngine::planeLocation.getHeading ());
    }


    start_na.synchToPoint ();
    if (start_na.getName ().empty ())
      start_na.setName (mxconst::get_ELEMENT_BRIEFER ());
    // try to locate a ramp
    std::string err;

    // try to locate a ramp v2 - DEBUG
    if (!missionx::RandomEngine::get_user_wants_to_start_from_plane_position () && !filterAndPickRampBasedOnPlaneType (start_na, err, missionx::mxFilterRampType::start_ramp))
    {
      Log::logMsgThread (fmt::format ("[{}] {}", __func__, err));
    }


    RandomEngine::listNavInfo.emplace_back (start_na); // add NavInfo into a list

    // handle target location
    RandomEngine::shared_navaid_info.navAid.init ();
    RandomEngine::shared_navaid_info.navAid.setID (toICAO);
    if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
    {
      RandomEngine::setError (fmt::format ("[{}] Target Navaid: {}, Failed to find Airport using original Navaid. Notify developer.", __func__, toICAO));
      return false;
    }
    RandomEngine::shared_navaid_info.navAid.synchToPoint ();
    NavAidInfo target_na = RandomEngine::shared_navaid_info.navAid;
    target_na.synchToPoint ();


    if (!filterAndPickRampBasedOnPlaneType (target_na, err, missionx::mxFilterRampType::end_ramp)) // v3.303.12_r2
    {
      Log::logMsgThread (fmt::format ("[{}, Target ILS] {}", __func__, err));
    }
    RandomEngine::listNavInfo.emplace_back (target_na); // add NavInfo into a list

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
    std::string plane_type_s = missionx::RandomEngine::translatePlaneTypeToString (conv_plane_type_i); // convert type to string and store it in mission node
    missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_PLANE_TYPE_S (), plane_type_s);
    pNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ());

    IXMLNode xLegNode    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_LEG ().c_str ()).deepCopy ();
    IXMLNode xMapTask    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TASK ().c_str ()).deepCopy ();
    IXMLNode xMapTrigger = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str ()).deepCopy ();
    IXMLNode xMapMessage = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE (), true); // holds message element from MAPPING


    // prepare briefer
    NavAidInfo naBriefer       = RandomEngine::listNavInfo.front ();
    IXMLNode   xLocationAdjust = missionx::RandomEngine::xRootTemplate.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ().c_str ()).deepCopy ();
    if (xLocationAdjust.isEmpty ())
    {
      RandomEngine::setError ("[random ILS] No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION () + "> was found. Template malform, abort template generation !!!");

      return false;
    }
    xLocationAdjust.updateName (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ());
    // remove any clear data
    int               nClear      = xLocationAdjust.nClear (); // remove any CDATA or COMMENTS or any clear() type element
    const std::string from_to_s   = get_short_flight_description_from_to (start_na.getName (), start_na.getID (), target_na.getName (), target_na.getID ()); //"From: " + start_na.getName() + "(" + start_na.getID() + ") to " + target_na.getName() + "(" + target_na.getID() + ")";
    std::string       brieferDesc = from_to_s + "\n\n" + "Hello pilot.\nYou have assigned an ILS flight to " + target_na.getID () + " and runway: " + to_icao.loc_rw_s + ". Learn the route and fly it according to plan or modify it if you so wish.\n\nBlue skys.";
    std::string       notes       = "\n\nDestination Notes:\n==============\nAirport: " + to_icao.toName_s + "(" + to_icao.toICAO_s + ")\tAirport Elev.: " + mxUtils::formatNumber<int> (to_icao.ap_elev_ft_i) + " ft." + "\nEstimate distance: " + mxUtils::formatNumber<double> (to_icao.distnace_d, 0) + "nm. \tRunway to Land: " + to_icao.loc_rw_s + ".\nLocalizer Type: " + to_icao.locType_s + ". \tLocalizer bearing: " + mxUtils::formatNumber<int> (to_icao.loc_bearing_i) + " \tlocalizer frq.: " + mxUtils::getFreqFormated (to_icao.loc_frq_mhz);


    for (int i = 0; i < nClear; ++i)
      xLocationAdjust.deleteClear (); // change from remove "i" to remove first


    xLocationAdjust.updateAttribute (naBriefer.getLat ().c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
    xLocationAdjust.updateAttribute (naBriefer.getLon ().c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
    xLocationAdjust.updateAttribute (naBriefer.getHeading_s ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());
    xLocationAdjust.updateAttribute (naBriefer.getRampInfo ().c_str (), mxconst::get_ATTRIB_RAMP_INFO ().c_str (), mxconst::get_ATTRIB_RAMP_INFO ().c_str ());

    RandomEngine::lastFlightLegNavInfo = naBriefer;
    if (naBriefer.getNavAidName ().empty ()) // v3.303.10
      RandomEngine::lastFlightLegNavInfo.flightLegName = mxconst::get_ELEMENT_BRIEFER ();

    RandomEngine::lastFlightLegNavInfo.synchToPoint ();

    this->xBriefer = this->xDummyTopNode.addChild (mxconst::get_ELEMENT_BRIEFER ().c_str ());
    this->xBriefer.addAttribute (mxconst::get_ATTRIB_STARTING_LEG ().c_str (), "leg_1"); // leg_1 is default value, but it can be changed when using <content> elements with "element sets"
    IXMLNode cNode = xBriefer.addChild (xLocationAdjust);
    Utils::xml_add_cdata (this->xBriefer, brieferDesc + notes); //

    // Add inventory if exists in mapping
    if (data_manager::xmlMappingNode.nChildNode (mxconst::get_ELEMENT_INVENTORY ().c_str ()) > 0)
    {
      // this->injectInventory(mxconst::get_ELEMENT_BRIEFER(), naBriefer.p.node, mxInvSource::point); // name of store will start with briefer
      this->addInventory (mxconst::get_ELEMENT_BRIEFER (), naBriefer.node, mxInvSource::point); // name of store will start with briefer
    }

    //// Finished Briefer construction ////

    // Prepare Objective + Tasks + Triggers and Leg flight plan
    if (xLegNode.isEmpty ())
    {
      RandomEngine::setError ("Could not find the mapping node: LEG, aborting mission template generating.");
      return false;
    }
    else
    {
      const std::string legName       = std::string (mxconst::get_ELEMENT_LEG ()) + "_" + Utils::formatNumber<int> (fpln_id_picked_i);
      const std::string objectiveName = legName + "_objective";

      IXMLNode xObjective = this->xObjectives.addChild (mxconst::get_ELEMENT_OBJECTIVE ().c_str ());
      xObjective.updateAttribute (objectiveName.c_str (), mxconst::get_ATTRIB_NAME ().c_str ());

      // create tasks based on waypoint list, excpe the firt one
      int counter = 0;
      for (auto &na : RandomEngine::listNavInfo)
      {
        counter++;
        if (counter == 1)
          continue; // it is the briefer starting location

        std::string task_name    = "task_" + Utils::formatNumber<int> (counter);
        std::string trigger_name = "trig_" + task_name;

        IXMLNode xTask = xObjective.addChild (mxconst::get_ELEMENT_TASK ().c_str ());
        xTask.updateAttribute (task_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
        xTask.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str ());
        xTask.updateAttribute ("3", mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC ().c_str (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC ().c_str ()); // evaluate success for 3 seconds
        xTask.updateAttribute (((counter == static_cast<int> (RandomEngine::listNavInfo.size ())) ? "yes" : ""), mxconst::get_ATTRIB_MANDATORY ().c_str (), mxconst::get_ATTRIB_MANDATORY ().c_str ()); // evaluate success for 3 seconds

        // add the trigger
        IXMLNode xTrigger = xMapTrigger.deepCopy ();
        xTrigger.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
        xTrigger.updateAttribute ("rad", mxconst::get_ATTRIB_TYPE ().c_str (), mxconst::get_ATTRIB_TYPE ().c_str ()); // set type as radius based "rad".
        Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LAT (), na.getLat (), mxconst::get_ELEMENT_POINT ());
        Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LONG (), na.getLon (), mxconst::get_ELEMENT_POINT ());

        if (counter == static_cast<int> (RandomEngine::listNavInfo.size ()))
        {
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT (), "100", mxconst::get_ELEMENT_RADIUS ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "true", mxconst::get_ELEMENT_CONDITIONS ());
        }
        else
        {
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT (), "4000", mxconst::get_ELEMENT_RADIUS ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "", mxconst::get_ELEMENT_CONDITIONS ());
        }

        this->xTriggers.addChild (xTrigger);

      } // end loop over all listNavInfo

      ////// Construct Flight Leg //////
      static const std::string STARTING_MESSAGE_NAME = "starting_message";
      const std::string        leg_message_name_s    = STARTING_MESSAGE_NAME + "_" + Utils::formatNumber<int> (counter);

      xLegNode.updateAttribute (legName.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
      xLegNode.updateAttribute (from_to_s.c_str (), mxconst::get_ATTRIB_TITLE ().c_str (), mxconst::get_ATTRIB_TITLE ().c_str ());
      Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME (), objectiveName, mxconst::get_ELEMENT_LINK_TO_OBJECTIVE ()); // link to objective
      Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME (), leg_message_name_s, mxconst::get_ELEMENT_START_LEG_MESSAGE ()); // link to objective


      // Add message to flight leg
      IXMLNode xMessage01 = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_MESSAGE ().c_str ()).deepCopy ();
      if (!xMessage01.isEmpty ())
      {
        xMessage01.updateAttribute (leg_message_name_s.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
        IXMLNode mixText = Utils::xml_get_or_create_node_ptr (xMessage01, mxconst::get_ELEMENT_MIX ());
        mixText.updateAttribute ("text", mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE ().c_str (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE ().c_str ());
        const std::string text = "Hello pilot\nYou will fly the route " + from_to_s + ". \n\nGood Luck";
        Utils::xml_add_cdata (mixText, text);

        this->xMessages.addChild (xMessage01);
      }


      // Add Ending Marker
      if (IXMLNode xDisplayEndLocation = xLegNode.addChild (mxconst::get_ELEMENT_DISPLAY_OBJECT ().c_str ()); !xDisplayEndLocation.isEmpty ())
      {
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_INSTANCE_NAME ().c_str (), std::string ("marker_" + legName).c_str ());
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_NAME ().c_str (), "marker"); // this is the name of the marker in the "template_blank_4_ui.xml" file
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_TARGET_MARKER_B ().c_str (), "true");


        NavAidInfo naLast = RandomEngine::listNavInfo.back ();
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LAT ().c_str (), naLast.getLat ().c_str ());
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LONG ().c_str (), naLast.getLon ().c_str ());
        xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT ().c_str (), "50"); // display marker 50ft above ground
      }

      // Add Flight Leg DESCRIPTION
      IXMLNode xDesc = xLegNode.getChildNode (mxconst::get_ELEMENT_DESC ().c_str ());
      if (xDesc.isEmpty ())
        xDesc = xLegNode.addChild (mxconst::get_ELEMENT_DESC ().c_str ());

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


void
RandomEngine::add_waypoints_for_fpln_or_simbrief (IXMLNode &pNode)
{
  if (pNode.isEmpty ())
    return;


  if (const auto s_waypoints = data_manager::prop_userDefinedMission_ui.getChildTextValue (mxconst::get_PROP_ADD_ROUTE_WAYPOINTS ()); !s_waypoints.empty () && pNode.nChildNode () > 0)
  {

    const auto vecWaypoints = mxUtils::split_skipEmptyTokens (s_waypoints);
    for (const auto &waypoint : vecWaypoints)
    {
      if (const auto way = mxUtils::trim (waypoint); !way.empty () && !mxUtils::compare (way, mxconst::get_ROUTE_DCT (), false))
      {
        auto node = pNode.getChildNode (mxconst::get_ELEMENT_POINT ().c_str (), (pNode.nChildNode (mxconst::get_ELEMENT_POINT ().c_str ()) - 1));
        if (!node.isEmpty ())
        {

          auto const lat = Utils::readNodeNumericAttrib<float> (node, mxconst::get_ATTRIB_LAT (), 0.0f);
          auto const lon = Utils::readNodeNumericAttrib<float> (node, mxconst::get_ATTRIB_LONG (), 0.0f);

          // the navaid information holds the "prev" lat/lon and the "Search navaid name ID"
          // Therefore, DO NOT use the NAV Aid information as a valid NavAid, it is only a means to pass search information
          RandomEngine::shared_navaid_info.navAid.init ();
          RandomEngine::shared_navaid_info.navAid.setID (way);
          RandomEngine::shared_navaid_info.navAid.lat = lat;
          RandomEngine::shared_navaid_info.navAid.lon = lon;

          RandomEngine::shared_navaid_info.navAid.synchToPoint (); // the internal Point will be used later

          if (missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_and_guess_nav_aid_info_mainThread))
          {
            if (RandomEngine::shared_navaid_info.navAid.navRef != XPLM_NAV_NOT_FOUND)
            {
              RandomEngine::shared_navaid_info.navAid.synchToPoint ();
              pNode.addChild (RandomEngine::shared_navaid_info.navAid.node.deepCopy ());
            }
            else
            {
              Log::logMsgThread (fmt::format ("[{}] Route Nav: {}, Failed to find its information. Will not add it to the GPS", __func__, way));
            }
          }
          else
          {
            Log::logMsgThread (fmt::format ("[{}] Route Nav: {}, Failed to find its information. Will not add it to the GPS", __func__, way));
          }
        }
      }
    } // end loop over route waypoints
  }
}


// -----------------------------------


bool
RandomEngine::prepare_mission_based_on_user_fpln_or_simbrief (IXMLNode &pNode)
{
  assert (!pNode.isEmpty () && !data_manager::prop_userDefinedMission_ui.node.isEmpty () && "Empty template or prop_userDefinedMission_ui are empty!");

  missionx::mx_ext_internet_fpln_strct fpln;

  auto plane_type_i                             = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types::plane_type_props)); // plane type
  fpln.fpln_unique_id                           = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FPLN_ID_PICKED (), -1); // max slider
  fpln.fromICAO_s                               = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FROM_ICAO (), "");
  fpln.toICAO_s                                 = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_TO_ICAO (), "");
  fpln.formated_nav_points_with_guessed_names_s = data_manager::prop_userDefinedMission_ui.getChildTextValue (mxconst::get_PROP_ADD_ROUTE_WAYPOINTS ());

  if ((fpln.fpln_unique_id < 0) + (fpln.fromICAO_s.empty ()) + (fpln.toICAO_s.empty ()))
  {
    RandomEngine::setError ("Flight plan might not have the FROM/TO ICAO data information. Aborting mission generating.");
    return false;
  }

  // convert to native plane type from "int"
  auto conv_plane_type_i = static_cast<missionx::_mx_plane_type> (plane_type_i);
  this->setPlaneType (conv_plane_type_i); // set plane type in class level for other function too

  {
    {
      RandomEngine::shared_navaid_info.navAid.init ();
      RandomEngine::shared_navaid_info.navAid.setID (fpln.fromICAO_s);

      if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
      {
        RandomEngine::setError (fmt::format ("[{}] Start Navaid: {}, Failed to find Airport using original Navaid. Notify developer.", __func__, fpln.fromICAO_s));
        return false;
      }
      RandomEngine::shared_navaid_info.navAid.synchToPoint ();
      // if we reached here then we should have startICAO NavAid information and the targetICAO
      NavAidInfo start_na (RandomEngine::shared_navaid_info.navAid); // v25.04.2

      // force plane position as starting location, based on user preference
      if (missionx::RandomEngine::get_user_wants_to_start_from_plane_position ()) // TODO: do we need to check "from layer" in function ?
      {
        start_na.lat = static_cast<float> (RandomEngine::planeLocation.getLat ());
        start_na.lon = static_cast<float> (RandomEngine::planeLocation.getLon ());
        start_na.heading = static_cast<float> (RandomEngine::planeLocation.getHeading ());
      }
      start_na.synchToPoint ();
      if (start_na.getName ().empty ())
        start_na.setName (mxconst::get_ELEMENT_BRIEFER ());

      // try to locate a ramp
      std::string err;
      if (!missionx::RandomEngine::get_user_wants_to_start_from_plane_position () && !filterAndPickRampBasedOnPlaneType (start_na, err, missionx::mxFilterRampType::start_ramp))
      {
        Log::logMsgThread (fmt::format ("[{}] {}", __func__, err));
      }

      // add NavInfo into a list
      RandomEngine::listNavInfo.emplace_back (start_na);

      //////////////////////////
      // handle target location
      /////////////////////////
      RandomEngine::shared_navaid_info.navAid.init ();
      RandomEngine::shared_navaid_info.navAid.setID (fpln.toICAO_s);
      if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
      {
        RandomEngine::setError (fmt::format ("[{}] Target Navaid: {}, Failed to find Airport using original Navaid. Notify developer.", __func__, fpln.toICAO_s));
        return false;
      }
      RandomEngine::shared_navaid_info.navAid.synchToPoint ();
      NavAidInfo target_na (RandomEngine::shared_navaid_info.navAid); // v25.04.2, it also calls "syncToPoint()"

      // get ramp location
      if (!filterAndPickRampBasedOnPlaneType (target_na, err, missionx::mxFilterRampType::end_ramp)) // v3.303.12_r2
      {
        Log::logMsgThread (fmt::format ("[{}, Target ILS] {}", __func__, err));
      }
      RandomEngine::listNavInfo.emplace_back (target_na); // add NavInfo into a list

      // Add to GPS
      if (!xGPS.isEmpty ())
      {
        xGPS.addChild (start_na.node.deepCopy ());
        add_waypoints_for_fpln_or_simbrief (xGPS); // v25.04.2 add route waypoints
        xGPS.addChild (target_na.node.deepCopy ());
      }

      //////////////////////
      // Prepare Main Nodes
      /////////////////////
      std::string plane_type_s = this->translatePlaneTypeToString (conv_plane_type_i); // convert type to string and store it in mission node
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_PLANE_TYPE_S (), plane_type_s);
      pNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ());

      IXMLNode xLegNode    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_LEG ().c_str ()).deepCopy ();
      IXMLNode xMapTask    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TASK ().c_str ()).deepCopy ();
      IXMLNode xMapTrigger = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str ()).deepCopy ();
      IXMLNode xMapMessage = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE (), true); // holds message element from MAPPING


      // prepare briefer
      NavAidInfo naBriefer       = RandomEngine::listNavInfo.front ();
      IXMLNode   xLocationAdjust = missionx::RandomEngine::xRootTemplate.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ().c_str ()).deepCopy ();
      if (xLocationAdjust.isEmpty ())
      {
        RandomEngine::setError ("No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION () + "> was found. Template malformed, abort template generation !!!");

        return false;
      }
      xLocationAdjust.updateName (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ());
      // remove any clear data
      int               nClear      = xLocationAdjust.nClear (); // remove any CDATA or COMMENTS or any clear() type element
      const std::string from_to_s   = get_short_flight_description_from_to (start_na.getName (), start_na.getID (), target_na.getName (), target_na.getID ()); //"From: " + start_na.getName() + "(" + start_na.getID() + ") to " + target_na.getName() + "(" + target_na.getID() + ")";
      std::string       brieferDesc = from_to_s + "\n\n" + "Hello pilot.\nYou have assigned a flight to " + target_na.getID () + ". Go over the route and fly it according to plan or modify it if you so wish.\n\nBlue skys.";
      std::string       notes       = "\n\nDestination Notes:\n==============\nAirport: " + target_na.getNavAidName () + "(" + target_na.getID () + ")\nWaypoints:\n" + fpln.formated_nav_points_with_guessed_names_s;


      for (int i = 0; i < nClear; ++i)
        xLocationAdjust.deleteClear ();


      xLocationAdjust.updateAttribute (naBriefer.getLat ().c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
      xLocationAdjust.updateAttribute (naBriefer.getLon ().c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
      xLocationAdjust.updateAttribute (naBriefer.getHeading_s ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());
      xLocationAdjust.updateAttribute (naBriefer.getRampInfo ().c_str (), mxconst::get_ATTRIB_RAMP_INFO ().c_str (), mxconst::get_ATTRIB_RAMP_INFO ().c_str ());

      RandomEngine::lastFlightLegNavInfo = naBriefer;
      if (naBriefer.getNavAidName ().empty ())
        RandomEngine::lastFlightLegNavInfo.flightLegName = mxconst::get_ELEMENT_BRIEFER ();

      RandomEngine::lastFlightLegNavInfo.synchToPoint ();

      this->xBriefer = this->xDummyTopNode.addChild (mxconst::get_ELEMENT_BRIEFER ().c_str ());
      this->xBriefer.addAttribute (mxconst::get_ATTRIB_STARTING_LEG ().c_str (), "leg_1"); // leg_1 is default value, but it can be changed when using <content> elements with "element sets"
      IXMLNode cNode = xBriefer.addChild (xLocationAdjust);
      Utils::xml_add_cdata (this->xBriefer, brieferDesc + notes); //

      // Add inventory if exists in mapping
      if (data_manager::xmlMappingNode.nChildNode (mxconst::get_ELEMENT_INVENTORY ().c_str ()) > 0)
      {
        this->addInventory (mxconst::get_ELEMENT_BRIEFER (), naBriefer.node, mxInvSource::point); // name of store will start with briefer
      }

      //// Finished Briefer construction ////

      // Prepare Objective + Tasks + Triggers and Leg flight plan
      if (xLegNode.isEmpty ())
      {
        RandomEngine::setError ("Could not find the mapping node: LEG, aborting mission template generating.");
        return false;
      }
      else
      {
        const std::string legName       = std::string (mxconst::get_ELEMENT_LEG ()) + "_" + Utils::formatNumber<int> (fpln.fpln_unique_id);
        const std::string objectiveName = legName + "_objective";

        IXMLNode xObjective = this->xObjectives.addChild (mxconst::get_ELEMENT_OBJECTIVE ().c_str ());
        xObjective.updateAttribute (objectiveName.c_str (), mxconst::get_ATTRIB_NAME ().c_str ());

        // create tasks based on waypoint list, except for the first one
        int counter = 0;
        for (auto &na : RandomEngine::listNavInfo)
        {
          counter++;
          if (counter == 1)
            continue; // it is the briefer starting location

          std::string task_name    = "task_" + Utils::formatNumber<int> (counter);
          std::string trigger_name = "trig_" + task_name;

          IXMLNode xTask = xObjective.addChild (mxconst::get_ELEMENT_TASK ().c_str ());
          xTask.updateAttribute (task_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
          xTask.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str ());
          xTask.updateAttribute ("3", mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC ().c_str (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC ().c_str ()); // evaluate success for 3 seconds
          xTask.updateAttribute (((counter == static_cast<int> (RandomEngine::listNavInfo.size ())) ? "yes" : ""), mxconst::get_ATTRIB_MANDATORY ().c_str (), mxconst::get_ATTRIB_MANDATORY ().c_str ()); // evaluate success for 3 seconds

          // add the trigger
          IXMLNode xTrigger = xMapTrigger.deepCopy ();
          xTrigger.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
          xTrigger.updateAttribute ("rad", mxconst::get_ATTRIB_TYPE ().c_str (), mxconst::get_ATTRIB_TYPE ().c_str ()); // set type as radius based "rad".
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LAT (), na.getLat (), mxconst::get_ELEMENT_POINT ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LONG (), na.getLon (), mxconst::get_ELEMENT_POINT ());

          if (counter == static_cast<int> (RandomEngine::listNavInfo.size ()))
          {
            Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT (), "100", mxconst::get_ELEMENT_RADIUS ());
            Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "true", mxconst::get_ELEMENT_CONDITIONS ());
          }
          else
          {
            Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT (), "4000", mxconst::get_ELEMENT_RADIUS ());
            Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "", mxconst::get_ELEMENT_CONDITIONS ());
          }

          this->xTriggers.addChild (xTrigger);

        } // end loop over all listNavInfo

        ////// Construct Flight Leg //////
        static const std::string STARTING_MESSAGE_NAME = "starting_message";
        const std::string        leg_message_name_s    = STARTING_MESSAGE_NAME + "_" + Utils::formatNumber<int> (counter);

        xLegNode.updateAttribute (legName.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
        xLegNode.updateAttribute (from_to_s.c_str (), mxconst::get_ATTRIB_TITLE ().c_str (), mxconst::get_ATTRIB_TITLE ().c_str ());
        Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME (), objectiveName, mxconst::get_ELEMENT_LINK_TO_OBJECTIVE ()); // link to objective
        Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME (), leg_message_name_s, mxconst::get_ELEMENT_START_LEG_MESSAGE ()); // link to objective


        // Add message to flight leg
        IXMLNode xMessage01 = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_MESSAGE ().c_str ()).deepCopy ();
        if (!xMessage01.isEmpty ())
        {
          xMessage01.updateAttribute (leg_message_name_s.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
          IXMLNode mixText = Utils::xml_get_or_create_node_ptr (xMessage01, mxconst::get_ELEMENT_MIX ());
          mixText.updateAttribute ("text", mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE ().c_str (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE ().c_str ());
          const std::string text = "Hello pilot\nYou will fly the route " + from_to_s + ". \n\nGood Luck";
          Utils::xml_add_cdata (mixText, text);

          this->xMessages.addChild (xMessage01);
        }


        // Add Ending Marker
        if (IXMLNode xDisplayEndLocation = xLegNode.addChild (mxconst::get_ELEMENT_DISPLAY_OBJECT ().c_str ()); !xDisplayEndLocation.isEmpty ())
        {
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_INSTANCE_NAME ().c_str (), std::string ("marker_" + legName).c_str ());
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_NAME ().c_str (), "marker"); // this is the name of the marker in the "template_blank_4_ui.xml" file
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_TARGET_MARKER_B ().c_str (), "true");


          NavAidInfo naLast = RandomEngine::listNavInfo.back ();
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LAT ().c_str (), naLast.getLat ().c_str ());
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LONG ().c_str (), naLast.getLon ().c_str ());
          xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT ().c_str (), "50"); // display marker 50ft above ground
        }

        // Add Flight Leg DESCRIPTION
        IXMLNode xDesc = xLegNode.getChildNode (mxconst::get_ELEMENT_DESC ().c_str ());
        if (xDesc.isEmpty ())
          xDesc = xLegNode.addChild (mxconst::get_ELEMENT_DESC ().c_str ());

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

// -----------------------------------

std::map<missionx::enums::mx_osm_region, missionx::structs::BBox>
RandomEngine::gen_quadrant_bboxes (const double centerLat, const double centerLon)
{
  #ifdef RELEASE
   constexpr double rangeNm = 75.0;
  #else
  constexpr double rangeNm = 25.0; // debug
  #endif
  constexpr double exclusionNm = 1.5;

  // Latitudinal deltas
  constexpr double deltaLat = rangeNm * NM_TO_DEG_LAT;
  constexpr double exclusionLat = exclusionNm * NM_TO_DEG_LAT;

  // Longitudinal deltas
  const double deltaLon = mxUtils::nmToDegLon(rangeNm, centerLat);
  const double exclusionLon = mxUtils::nmToDegLon(exclusionNm, centerLat);

  std::map<enums::mx_osm_region, structs::BBox> bboxes;

  // Top Left (NW)
  bboxes[enums::mx_osm_region::nw] = structs::BBox({centerLat + exclusionLat, centerLon - deltaLon, centerLat + deltaLat, centerLon - exclusionLon });

  // Top Right (NE)
  bboxes[enums::mx_osm_region::ne] = structs::BBox({centerLat + exclusionLat, centerLon + exclusionLon, centerLat + deltaLat, centerLon + deltaLon });

  // Bottom Left (SW)
  bboxes[enums::mx_osm_region::sw] = structs::BBox({centerLat - deltaLat, centerLon - deltaLon, centerLat - exclusionLat, centerLon - exclusionLon });

  // Bottom Right (SE)
  bboxes[enums::mx_osm_region::se] = structs::BBox({centerLat - deltaLat, centerLon + exclusionLon, centerLat - exclusionLat, centerLon + deltaLon });

  return bboxes;
}

// -----------------------------------

std::vector<missionx::structs::strct_osm_query>
RandomEngine::gen_osm_analyse (mx_return &out_mx_return, const std::string &xmlFilename, const std::string &in_cache_folder, const double centre_lat, const double centre_lon, IXMLNode &outRootNode)
{
/*
 * The idea is to have four big BBOX areas to search while leaving ~1.5nm of clear central area.
 * The schema below is just an example.
 * |---------------------|--------------------|
 * |                   |                      |
 * |                   |                      |
 * |                   |                      |
 * |     NW            |       NE             |
 * |                   |                      |
 * |                   +-+-+------------------|
 * +                   | P |                  |
 * | ------------------+-+-+                  |
 * |                       |                  |
 * |                       |                  |
 * |     SW                |   SE             |
 * |                       |                  |
 * |                       |                  |
 * |------------------------------------------|
 *
 */

  std::vector<missionx::structs::strct_osm_query> vec_osm_query_analyze_results;
  try
  {
    const std::map<missionx::enums::mx_osm_region, missionx::structs::BBox> map_bbox = gen_quadrant_bboxes (centre_lat, centre_lon);
    missionx::structs::BBox all_bbox;
    if (Utils::isElementExists (map_bbox, enums::mx_osm_region::sw) && Utils::isElementExists (map_bbox, enums::mx_osm_region::ne))
    {
      all_bbox.minLat = map_bbox.at(enums::mx_osm_region::sw).minLat;
      all_bbox.minLon = map_bbox.at(enums::mx_osm_region::sw).minLon;
      all_bbox.maxLat = map_bbox.at(enums::mx_osm_region::ne).maxLat;
      all_bbox.maxLon = map_bbox.at(enums::mx_osm_region::ne).maxLon;

      Log::logMsgThread (fmt::format ("\n--------> All BBOX regions: {} <-----------\n", all_bbox.get_bbox ()));
    }


    std::string err;
    IXMLDomParser parser;

    const auto dom_root = parser.openFileHelper(xmlFilename.c_str(), mxconst::get_EXPECTED_LOCATION_TYPE_OSM().c_str(), &err);
    if (!err.empty ())
    {
      Log::logMsgThread (fmt::format("[{}] Error parsing {}.\n-- Start error -->\n{}\n<-- end error --", __func__, xmlFilename, err) );
      return vec_osm_query_analyze_results;
    }

    const auto root = dom_root.deepCopy();
    #ifndef RELEASE
     auto all_bboxes = all_bbox.get_bbox (); // debug
    #endif

    if ( root.isEmpty() ) { // != "osm"
      Log::logMsgThread (fmt::format("[Error] Root <osm> not found. Name: {}", root.getName () ) );
      return vec_osm_query_analyze_results;
    }

    outRootNode = root.deepCopy (); // return the osm_gen.xml to the calling function

    // init random seed
    std::random_device rd;
    std::mt19937 g(rd()); // Mersenne Twister engine seeded by random_device

    auto nChilds = root.nChildNode("analyze");
    IXMLNode analyzeNode = root.getChildNode("analyze");
    auto n_q_analyze_sub_nodes = analyzeNode.nChildNode("q");
    if (analyzeNode.isEmpty () ||  n_q_analyze_sub_nodes < 1 )
    {
      Log::logMsgThread ( fmt::format("[{}][Error] <analyze> section missing.\n", __func__) );
      return vec_osm_query_analyze_results;
    }

    // Prepare shuffle vector for os "q"
    std::vector <int> vec_shuffle_analyze_nodes;
    for (int i1=0; i1<n_q_analyze_sub_nodes; i1++)
      vec_shuffle_analyze_nodes.push_back(i1);

    // shuffle vector
    std::ranges::shuffle(vec_shuffle_analyze_nodes, g);
    // Fetch the "q" node, parse it and call the main "q" analyze function
    std::vector<std::thread> overpassThreads; // will hold the threaded call

    int node_counter = 0;
    for (int i1=0; i1<n_q_analyze_sub_nodes && node_counter < 4; i1++) // at most, we will handle 4 threads
    {
      IXMLNode qNode = analyzeNode.getChildNode("q", i1);
      if (qNode.isEmpty())
        continue;
      missionx::structs::strct_osm_query q;

      q.id           = qNode.getAttribute ("id");
      q.q_text       = Utils::xml_get_text_or_cdata_text (qNode, "");
      q.cache_folder = in_cache_folder;
      q.topic_q_node = qNode.deepCopy (); // original "<q>"
      q.q_all_bbox   = all_bbox.get_bbox (); // whole bbox region area.
      if (q.q_text.empty ())
        continue;

      vec_osm_query_analyze_results.push_back(std::move(q));
      node_counter++;

    }

    // Loop over all BBOX areas and gather "basic statistics" like how many "ways" but do not retrieve any "<way>/<node>" information.
    for (auto& q : vec_osm_query_analyze_results) {
      overpassThreads.emplace_back(missionx::data_manager::fetch_overpass_info_analyze_thread, &missionx::RandomEngine::threadState, nullptr, &q, map_bbox );
      // Sleep 2 seconds between thread dispatch
      std::this_thread::sleep_for (std::chrono::seconds (2)); // wait for 2 seconds before sending a new request
    }

    // Wait for all overpass threads to finish
    for (auto& t : overpassThreads) {
      if (t.joinable()) t.join();
    }

    // check [abort]
    if (RandomEngine::threadState.flagAbortThread)
    {
      vec_osm_query_analyze_results.clear ();
      return vec_osm_query_analyze_results;
    }

    // debug - display all XML information
    #ifndef RELEASE
    for (auto& analyzed_query : vec_osm_query_analyze_results) {
      Log::logMsgThread ("\n------------------------------------------>\n\n");
      Log::logMsgThread ( "[Overpass] " + analyzed_query.id + "\n");
      Log::logMsgThread ( fmt::format("[tags node] \n{}", Utils::xml_get_node_content_as_text (analyzed_query.xml_q_tags_header_node) ) );

      for (const auto& [k,v]: analyzed_query.osm_queries)
        Log::logMsgThread ( fmt::format("\t{}: {}\n", k, v) );

      Log::logMsgThread ( fmt::format("[Overpass] {} took: {}\n", analyzed_query.id, analyzed_query.get_elapsed_time() ) );
      Log::logMsgThread ( "\n------------------------------------------>\n\n" );
    }
    #endif
    ///// END Analyze /////

  }
  catch (const std::exception& ex) {
    Log::logMsgThread (fmt::format("[{}][Exception] {}\n", __func__, ex.what () ));
    return vec_osm_query_analyze_results;
  }

  return vec_osm_query_analyze_results;
}


// -----------------------------------

void
RandomEngine::gen_gather_navaid_metadata_relative_to_target (const IXMLNode &inoutMetaNode, missionx::NavAidInfo &target_navaid, missionx::NavAidInfo &inout_from_navaid, missionx::NavAidInfo *inout_next_navaid_ptr)
{
  //-----------------------------------
  // Calculate distances and bearing between previous and current navaids
  //-----------------------------------
  
  // Calculate distance
  target_navaid.fpln_distance_between_prev_and_current_navaid     = Point::calcDistanceBetween2Points (target_navaid.p, inout_from_navaid.p);
  inout_from_navaid.fpln_distance_to_next_navaid = target_navaid.fpln_distance_between_prev_and_current_navaid;

  // Calculate bearing
  inout_from_navaid.bearing_next            = static_cast<float> (Utils::mxCalcBearingBetween2Points (inout_from_navaid.lat, inout_from_navaid.lon, target_navaid.lat, target_navaid.lon));
  target_navaid.bearing_to_current_target   = inout_from_navaid.bearing_next;
  target_navaid.bearing_back_to_prev_target = (target_navaid.bearing_to_current_target > 180.0f) ? target_navaid.bearing_to_current_target - 180.0f : target_navaid.bearing_to_current_target + 180.0f;

  //-----------------------------------
  // Calculate distances between current and next navaids
  //-----------------------------------

  if (inout_next_navaid_ptr)
  {
    target_navaid.fpln_distance_to_next_navaid = Point::calcDistanceBetween2Points (target_navaid.p, inout_next_navaid_ptr->p);
  }

  //-----------------------------------
  // gather mission metadata info: task type and mission type
  //-----------------------------------
  const int         ui_picked_task_type_i = Utils::readNodeNumericAttrib<int> (inoutMetaNode, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), 0);
  const std::string category              = mxUtils::stringToLower (Utils::readAttrib (inoutMetaNode, mxconst::get_ATTRIB_CATEGORY (), ""));
  const int         sub_category_i        = Utils::readNodeNumericAttrib<int> (inoutMetaNode, mxconst::get_PROP_MISSION_SUBCATEGORY (), 0);

  switch (ui_picked_task_type_i)
  {
    case static_cast<int> (missionx::mx_ui_mission_type::medevac):
    {
      target_navaid.fpln_task_type = missionx::enums::mx_rnd_task_type::medevac;
      if (sub_category_i == 0) // any location medevac
        target_navaid.fpln_mission_type = missionx::enums::mx_user_picked_mission_type::medevac;
      else if (sub_category_i == 1)
        target_navaid.fpln_mission_type = missionx::enums::mx_user_picked_mission_type::osm_medevac;
      else
        target_navaid.fpln_mission_type = missionx::enums::mx_user_picked_mission_type::osm_gen;
    }
    break;
    case static_cast<int> (missionx::mx_ui_mission_type::oil_rig):
    {
      if (sub_category_i == 0) // cargo
      {
        target_navaid.fpln_task_type = missionx::enums::mx_rnd_task_type::cargo;
        target_navaid.fpln_mission_type = missionx::enums::mx_user_picked_mission_type::oilrig_cargo;
      }
      else
      {
        target_navaid.fpln_task_type = missionx::enums::mx_rnd_task_type::medevac;
        target_navaid.fpln_mission_type = missionx::enums::mx_user_picked_mission_type::oilrig_medevac;
      }
    }
    break;
    case static_cast<int> (missionx::mx_ui_mission_type::cargo):
      [[fallthrough]];
    default: // Cargo will be the default too
    {
      target_navaid.fpln_task_type = missionx::enums::mx_rnd_task_type::cargo;
      target_navaid.fpln_mission_type = missionx::enums::mx_user_picked_mission_type::cargo;
    }
    break;
  }

}

// -----------------------------------

IXMLNode
RandomEngine::gen_trigger_node (int &seq, const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo &inTargetNavAid, const std::list<missionx::structs::strct_node_attribute_key_value> &in_attrib_list, IXMLNode *parentNode)
{
  // get trigger node template
  IXMLNode xTrigger = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str ()).deepCopy ();
  if (xTrigger.isEmpty ())
    xTrigger = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_TRIGGER ());

  assert (!xTrigger.isEmpty () && fmt::format ("[{}] Failed to get a Trigger node template.", __func__).c_str ());

  if (xTrigger.isEmpty ())
    return IXMLNode::emptyIXMLNode;

  // clear <outcome> node attributes
  IXMLNode xml_outcome_node = xTrigger.getChildNode (mxconst::get_ELEMENT_OUTCOME ().c_str ());
  Utils::xml_delete_all_node_attributes (xml_outcome_node);

  const std::string sanitize_navaid_name = mxUtils::sanitize_text (inTargetNavAid.getName ());
  const std::string trig_name            = fmt::format ("{}_{}_target_{}{}", prefix_name, seq, sanitize_navaid_name, (sanitize_navaid_name.empty ()) ? postfix_name : std::string ("-") + postfix_name);
  // set xTrigger information
  xTrigger.updateAttribute (trig_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
  Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LAT (), inTargetNavAid.getLat (), mxconst::get_ELEMENT_POINT ());
  Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LONG (), inTargetNavAid.getLon (), mxconst::get_ELEMENT_POINT ());

  // loop over in_attrib_list and set the trigger node
  Utils::xml_search_and_set_attributes_in_node (xTrigger, in_attrib_list);

  // #ifndef RELEASE
  // Utils::xml_print_node (xTrigger, true);
  // #endif // !RELEASE


  // increase seq
  seq++;
  return xTrigger;
}

// -----------------------------------


IXMLNode
RandomEngine::gen_task_node (int &seq, const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo &inTargetNavAid, const std::list<missionx::structs::strct_node_attribute_key_value> &in_attrib_list, IXMLNode *parentNode)
{
  IXMLNode xTask = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TASK ().c_str ()).deepCopy ();
  if (xTask.isEmpty ())
    xTask = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_TASK ());

  assert (!xTask.isEmpty () && fmt::format ("[{}] Failed to get a Task node template.", __func__).c_str ());

  Utils::xml_delete_all_node_attributes (xTask);
  
  if (xTask.isEmpty ())
    return IXMLNode::emptyIXMLNode;

  const std::string sanitize_navaid_name = mxUtils::sanitize_text (inTargetNavAid.getName ());
  // name example: "task_0_building_{postfix}"
  const std::string name = fmt::format ("{}_{}_{}{}", prefix_name, seq, sanitize_navaid_name, (sanitize_navaid_name.empty ()) ? postfix_name : std::string ("-") + postfix_name);

  // set xTask information
  xTask.updateAttribute (name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
  Utils::xml_search_and_set_attribute_in_IXMLNode (xTask, mxconst::get_ATTRIB_LAT (), inTargetNavAid.getLat (), mxconst::get_ELEMENT_POINT ());
  Utils::xml_search_and_set_attribute_in_IXMLNode (xTask, mxconst::get_ATTRIB_LONG (), inTargetNavAid.getLon (), mxconst::get_ELEMENT_POINT ());

  // loop over in_attrib_list and set the trigger node
  Utils::xml_search_and_set_attributes_in_node (xTask, in_attrib_list);

  // #ifndef RELEASE
  // Utils::xml_print_node (xTask, true);
  // #endif // !RELEASE

  seq++;
  return xTask;
}

// -----------------------------------

IXMLNode
RandomEngine::gen_objective_node (int &seq, const std::string &prefix_name, const std::string &postfix_name, IXMLNode *parentNode)
{
  IXMLNode node = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_OBJECTIVE ().c_str ()).deepCopy ();
  if (node.isEmpty ())
    node = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_OBJECTIVE ());

  assert (!node.isEmpty () && fmt::format ("[{}] Failed to get a Task node template.", __func__).c_str ());

  const std::string name = fmt::format ("{}_{}{}", prefix_name, seq, std::string ("-") + postfix_name);

  node.updateAttribute (name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());

  seq++;
  return node;
}

// -----------------------------------

IXMLNode
RandomEngine::gen_leg_description (IXMLNode &inout_leg_node, missionx::NavAidInfo &in_leg_as_navaid, missionx::NavAidInfo *in_next_leg_as_navaid_ptr) //, random_airport_info_struct &inout_random_airport_info_struct)
{
  // prepare the default description for <leg>
  const std::string target_icao = in_leg_as_navaid.getID ();
  const std::string target_name = in_leg_as_navaid.get_name_or_icao_id ();
  const std::string target_desc = in_leg_as_navaid.gen_locDesc_short ();

  auto lmbda_get_pre_message_for_default_desc_text = [&] ()
  {
    std::string desc_s = target_name;

    if (!target_icao.empty ())
      desc_s.append (fmt::format ("({})", target_icao));

    if (desc_s.empty () && !target_desc.empty ())
      return fmt::format ("Fly to: \"{}\"", mxUtils::sanitize_text (target_desc, "_", ' '));

    return fmt::format ("Fly to \"{}\"", mxUtils::sanitize_text (desc_s, "_", ' '));
  };

  const auto        desc_next_target_text        = lmbda_get_pre_message_for_default_desc_text ();
  const auto        desc_distance_text           = "Expected distance: {distance}";
  const auto        desc_elevation_text          = "(Elev: {navaid_elev}ft)";
  auto              desc_wet_text                = (in_leg_as_navaid.fpln_is_wet) ? "> Your next leg might be above water body.\n" : "\n";
  const std::string default_description_template = fmt::format ("{}.\n{} {}.\n{}--> Fly Safe <--", desc_next_target_text, desc_distance_text, desc_elevation_text, desc_wet_text);

  // get random node copy
  const IXMLNode xml_custom_desc_from_target_leg_node = Utils::xml_get_node_randomly_by_name_IXMLNode (in_leg_as_navaid.fpln_xml_osm_q_or_raw_tmpl_node, mxconst::get_ELEMENT_DESC (), false);

  #ifndef RELEASE
  Log::logMsgThread ( fmt::format ( "[{}] <{}> info:\n{}\n<-- end fpln_xml_target_leg_node \n"
                    , __func__, Utils::xml_get_tag_name(in_leg_as_navaid.fpln_xml_osm_q_or_raw_tmpl_node),  Utils::xml_get_node_content_as_text ( in_leg_as_navaid.fpln_xml_osm_q_or_raw_tmpl_node, "no <desc> nodes found." ) ) );
  #endif

  // construct the final template of the flight leg description if <desc> node in target <leg> is empty.
  const std::string leg_description = Utils::xml_get_text_or_cdata_text (xml_custom_desc_from_target_leg_node, default_description_template); // read custom description

  // replace {special keywords}.
  const std::string final_leg_description_text = RandomEngine::gen_message_with_special_keywords_static (leg_description, in_leg_as_navaid);

  // prepare <desc> and add it to the <leg>
  IXMLNode xml_desc = inout_leg_node.getChildNode (mxconst::get_ELEMENT_DESC ().c_str ());
  if (xml_desc.isEmpty ())
    xml_desc = inout_leg_node.addChild (mxconst::get_ELEMENT_DESC ().c_str ());

  // copy attributes from source custom <desc>, if any
  std::set<std::string> whiteList = {mxconst::get_ATTRIB_RANDOM_TAG (), mxconst::get_ATTRIB_SET_NAME (), mxconst::get_ATTRIB_SLOPE_SET_NAME ()};
  Utils::xml_copy_specific_attributes_using_white_list (xml_custom_desc_from_target_leg_node,  xml_desc, &whiteList);
  Utils::xml_set_text (xml_desc, final_leg_description_text);

  return xml_desc;
}

// -----------------------------------


IXMLNode
RandomEngine::gen_leg_node (const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo *inTargetNavAid, std::list<missionx::structs::strct_node_attribute_key_value> *in_attrib_list, IXMLNode *parentNode)
{
  if (inTargetNavAid == nullptr)
    return IXMLNode::emptyIXMLNode;

  // v25.09.1 extended leg_node to either use existing <leg> or create one. Can handle Oilrig and OSM Surprise me missions.
  IXMLNode leg_node = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_LEG ().c_str ()).deepCopy ();
  if ( ! inTargetNavAid->fpln_xml_target_leg_node.isEmpty () )
    // leg_node will point to the "fpln_xml_target_leg_node"
    leg_node = Utils::xml_merge_source_with_target_node ( leg_node, inTargetNavAid->fpln_xml_target_leg_node );

  // Make sure that the base <leg> sub-nodes are present:
  Utils::xml_add_child_nodes (leg_node, {"start_leg_message", "link_to_objective", "desc", "post_leg_message"});

  if (leg_node.isEmpty ())
    leg_node = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_LEG ());

  assert (!leg_node.isEmpty () && fmt::format ("[{}] Failed to get a Leg node template.", __func__).c_str ());

  if (leg_node.isEmpty ())
    return IXMLNode::emptyIXMLNode;

  // set xTask information
  leg_node.updateAttribute (inTargetNavAid->fpln_leg_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
  Utils::xml_search_and_set_attribute_in_IXMLNode (leg_node, mxconst::get_ATTRIB_LAT (), inTargetNavAid->getLat (), mxconst::get_ELEMENT_POINT ());
  Utils::xml_search_and_set_attribute_in_IXMLNode (leg_node, mxconst::get_ATTRIB_LONG (), inTargetNavAid->getLon (), mxconst::get_ELEMENT_POINT ());

  // loop over in_attrib_list and set the trigger node
  if (in_attrib_list)
    Utils::xml_search_and_set_attributes_in_node (leg_node, (*in_attrib_list));

  // OSM SURPRISE ME ONLY: copy all subnodes nodes from the <q> if present
  bool isEmpty = inTargetNavAid->fpln_xml_osm_q_or_raw_tmpl_node.isEmpty ();

  if (!(inTargetNavAid->fpln_xml_osm_q_or_raw_tmpl_node.isEmpty ()))
  {
    // exclude <inventory> and <desc> nodes. <desc> node will be picked in gen_leg_description() function.
    const std::vector<std::string> in_exclude_nodes = {mxconst::get_ELEMENT_INVENTORY (), mxconst::get_ELEMENT_DESC ()};
    Utils::xml_copy_or_replace_sub_nodes (leg_node, inTargetNavAid->fpln_xml_osm_q_or_raw_tmpl_node, true, &in_exclude_nodes);
  }

  return leg_node.deepCopy ();
}


// -----------------------------------


void
RandomEngine::gen_skew_target_data (missionx::NavAidInfo &in_target_navaid)
{
  in_target_navaid.synchToPoint ();

  auto lmbda_get_skew_position = [&](const IXMLNode &inTargetPoint)
  {
    // const auto plane_type = missionx::RandomEngine::getPlaneType (); // debug
    const bool flag_display_target_markers_away_from_target = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET (), false);
    if (flag_display_target_markers_away_from_target
        && !in_target_navaid.fpln_is_last_flight_leg
        && (missionx::RandomEngine::getPlaneType () <= static_cast<uint8_t> (_mx_plane_type::plane_type_helos)) )
    {
      in_target_navaid.flag_is_skewed = true;
      return get_skewed_target_position (inTargetPoint).deepCopy ();
    }

    in_target_navaid.flag_is_skewed = false;
    return IXMLNode::emptyIXMLNode;
    // return inTargetPoint.deepCopy (); // original code
  };

  IXMLNode xPoint = in_target_navaid.p.node.deepCopy ();
  Utils::xml_set_attribute_in_node<bool> (xPoint, mxconst::get_ATTRIB_IS_TARGET_POINT_B (), true, xPoint.getName ()); // A skewed point can still be a target so GPS points can be distinguished.
  in_target_navaid.xml_skewdPointNode = lmbda_get_skew_position (xPoint.deepCopy ()); // xPoint represents the real position.

}


// -----------------------------------


missionx::NavAidInfo
RandomEngine::gen_briefer_node (missionx::NavAidInfo &inout_start_navaid, RandomEngine::random_airport_info_struct &inout_random_airport_info_struct, const bool in_flag_we_have_target_above_water)
{
  // We will use this function to construct the "<briefer>" element.
  // The description of the mission in the briefer we will fetch from: "<mission_info>" CDATA property.
  inout_start_navaid.synchToPoint ();

  // missionx::NavAidInfo na_briefer;
  ///////////////////////
  // <briefer>
  IXMLNode x_briefer_node = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_BRIEFER () );

  // <location_adjust>
  IXMLNode x_location_adjust = Utils::xml_get_child_node ( x_briefer_node,mxconst::get_ELEMENT_LOCATION_ADJUST ());
  if (x_location_adjust.isEmpty () + x_briefer_node.isEmpty ())
  {
    Log::logMsgThread (fmt::format ("[{}] Fail to fetch internal \"{}\" element from Utils class. Notify developer !!!", __func__, mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ()));
    return inout_start_navaid;
  }

  const int nClear = x_location_adjust.nClear (); // remove any CDATA or COMMENTS or any clear() type element
  for (int i = 0; i < nClear; ++i)
    x_location_adjust.deleteClear ();

  const std::string locationOptionType = mxconst::get_ELEMENT_PLANE ();

  x_location_adjust.updateAttribute (inout_start_navaid.p.getLat_s ().c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
  x_location_adjust.updateAttribute (inout_start_navaid.p.getLon_s ().c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
  x_location_adjust.updateAttribute (Utils::formatNumber<double> (inout_start_navaid.p.getElevationInFeet (), 2).c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str ());
  x_location_adjust.updateAttribute (Utils::formatNumber<double> (inout_start_navaid.p.getHeading (), 2).c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());

  bool b_fetch_navaid_info {false};
  inout_random_airport_info_struct.navAid.init ();
  if ( inout_start_navaid.fpln_navaid_was_already_prepared )
  {
    if (inout_start_navaid.getID ().empty ())
      b_fetch_navaid_info = true;
  }
  else
  {
    // search for the nearest ICAO or bounding airport relative to plane starting position using the SQLITE database
    inout_start_navaid = missionx::data_manager::getPlaneAirportOrNearestICAO (true, inout_start_navaid.lat, inout_start_navaid.lon, true);
    inout_start_navaid.synchToPoint ();
    b_fetch_navaid_info = true;
  }// end if !fpln_navaid_was_already_prepared

  // try to find the nearest airport if we are not inside a valid airport boundary.
  if (inout_start_navaid.getID ().empty () && b_fetch_navaid_info)
  {
    // initialize the starting "lat/lon" coordinates before calling the main thread.
    inout_random_airport_info_struct.navAid = inout_start_navaid;

    if (missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
    {
      // check distance and hopefully pick the correct airport. Since we are using a fixed distance, this might not be a 100% guaranty
      inout_random_airport_info_struct.navAid.synchToPoint ();
      // const double dist = missionx::Point::calcDistanceBetween2Points (inout_random_airport_info_struct.navAid.p, inout_start_navaid.p);
      const double dist = inout_random_airport_info_struct.navAid.p.calcDistanceBetween2Points (inout_start_navaid.p, mx_units_of_measure::nm );
      if ( dist <= 5.0 && !inout_random_airport_info_struct.navAid.getID ().empty ())
      {
        std::string navaid_name;
        if (inout_start_navaid.getName ().empty ())
          navaid_name = fmt::format ("Near {}.", inout_random_airport_info_struct.navAid.getNavAidName ());
        else
          navaid_name = fmt::format ("{}, near {}.", inout_start_navaid.getName (), inout_random_airport_info_struct.navAid.getNavAidName ());

        if (inout_start_navaid.getID ().empty () && !inout_random_airport_info_struct.navAid.getID ().empty ())
          inout_start_navaid.setID (inout_random_airport_info_struct.navAid.getID ());

        inout_start_navaid.setName (navaid_name);
        inout_start_navaid.height_mt = inout_random_airport_info_struct.navAid.height_mt;
        inout_start_navaid.navRef    = inout_random_airport_info_struct.navAid.navRef; // v25.05.1

        // inPlanePosition.synchToPoint ();
      }
    } // end "mx_flc_pre_command::get_nearest_nav_aid_to_randomLastFlightLeg_mainThread"
  } // End starting location is not an airport boundary.

  // v25.09.1 added missing "starting_icao"
  x_briefer_node.updateAttribute (inout_start_navaid.getID ().c_str (), mxconst::get_ATTRIB_STARTING_ICAO ().c_str (), mxconst::get_ATTRIB_STARTING_ICAO ().c_str ());

  inout_start_navaid.flag_is_brieferOrStartLocation = true;
  inout_start_navaid.fpln_xml_target_leg_node = x_briefer_node.deepCopy ();

  inout_start_navaid.synchToPoint (b_fetch_navaid_info);
  return inout_start_navaid;
}

// -----------------------------------

void
RandomEngine::gen_post_briefer_desc (std::map<int, NavAidInfo> &inout_targets, const bool flag_has_wet_target)
{

  if (inout_targets.empty () || !inout_targets.contains (0) || !inout_targets.contains (1))
    return;

  // FYI:
  // in_osm_na_targets[0] = briefer
  // in_osm_na_targets[1] = First Target

  std::string cumulative_location_desc_s;
  for (int i1=0; i1 < static_cast<int>(inout_targets.size ()) && inout_targets.contains (i1); i1++)
  {
    if (i1 == 0) // 0 = briefer
      cumulative_location_desc_s = fmt::format ("(start): {}.\n", inout_targets[i1].get_loc_desc () );
    else
      cumulative_location_desc_s.append (inout_targets[i1].get_loc_desc ()).append (".\n");
  }

  std::string briefer_desc = inout_targets[0].fpln_expected_location_data.desc; // v25.09.2 use the description from the <briefer_and_start_location>

  // skip generic description creation if a pre-defined text was defined in the template file.
  if (briefer_desc.empty ())
  {
    briefer_desc= "Hello Pilot\n";
    briefer_desc += "You have been assigned to a medevac mission. ";
    briefer_desc += fmt::format ("Your expected transportation is a {}.\n", "helo");

    if (!inout_targets[0].getNavAidName ().empty ())
      briefer_desc +=  fmt::format ("You will fly from {}{}.", inout_targets[0].getNavAidName (),  mxUtils::eval_text (!inout_targets[0].getID ().empty (), "(" + inout_targets[0].getID () + ")", ""));
    else if (!inout_targets[0].getID ().empty ())
      briefer_desc +=  fmt::format ("You will fly from {}.", inout_targets[0].getID ());
    else
      briefer_desc +=  fmt::format ("You will fly to {}.", inout_targets[1].get_loc_desc ());
  }

  briefer_desc += (flag_has_wet_target) ? "\n\nOne of the flight legs is above water body, make sure you have all needed equipment. " : "";


  // v25.04.2 - fixed destination exposure, based on setup
  if (missionx::system_actions::pluginSetupOptions.getNodeText_type_1_5<bool> (mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE (), true))
    briefer_desc += "\n\nExpected route:\n" + cumulative_location_desc_s;
  else
    briefer_desc += "\n\nFirst waypoint: " + inout_targets[1].get_loc_desc () + ".";

  briefer_desc += "\n\nFly Safe !!!";

  Utils::xml_add_cdata (inout_targets[0].fpln_xml_target_leg_node, briefer_desc);
}

// -----------------------------------

IXMLNode
RandomEngine::gen_mission_info_node (const IXMLNode &xRootTemplate, const std::string &in_template_name, const std::string &in_template_image_file_name, const std::string &in_mission_folder_name)
{
  IXMLNode xBrieferInfo = xRootTemplate.getChildNode (mxconst::get_ELEMENT_MISSION_INFO ().c_str ()).deepCopy ();
  if (this->xBrieferInfo.isEmpty ())
    return xBrieferInfo;


  // override mission image file with random.png
  if (in_mission_folder_name.empty ())
    Utils::xml_set_attribute_in_node_asString (xBrieferInfo, mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME (), mxconst::get_DEFAULT_RANDOM_IMAGE_FILE (), mxconst::get_ELEMENT_MISSION_INFO ());
  else
  {
    const std::string imageFileName_s = Utils::readAttrib (xBrieferInfo, mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME (), in_template_image_file_name);
    Utils::xml_set_attribute_in_node_asString (xBrieferInfo, mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME (), imageFileName_s, mxconst::get_ELEMENT_MISSION_INFO ());
  }

  // set template information in "other" settings attribute
  std::string other_settings = Utils::readAttrib (xBrieferInfo, mxconst::get_ATTRIB_OTHER_SETTINGS (), ""); // v3.0.241.1
  other_settings             = "Based on: " + ((!in_template_name.empty ()) ? in_template_name : "Error: No Template Data") + ". " + other_settings;
  Utils::xml_set_attribute_in_node_asString (xBrieferInfo, mxconst::get_ATTRIB_OTHER_SETTINGS (), other_settings, mxconst::get_ELEMENT_MISSION_INFO ());


  return xBrieferInfo;
}

// -----------------------------------

IXMLNode
RandomEngine::gen_inventory_node (const int &in_seq, missionx::NavAidInfo & inout_target_navaid, std::unordered_map<int, mx_inventory_track_strct> &inout_map_osm_inventory_track, const std::list<missionx::structs::strct_node_attribute_key_value> &in_attrib_list)
{
  const std::string inv_name = fmt::format ("inv_{}", in_seq);

  // Prepare <inventory> node, and clean it.
  IXMLNode xml_inv_node = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_INVENTORY ());
  Utils::xml_delete_all_subnodes (xml_inv_node, mxconst::get_ELEMENT_ITEM (), true);
  Utils::xml_delete_all_subnodes (xml_inv_node, mxconst::get_ELEMENT_STATION (), true);

  // check if <q> has <inventory> node. Copy all its items into our local inventory

  // #ifndef RELEASE
  // Log::logMsgThread (fmt::format ("[{}] Osm Query content:\n{}\n<-- End OSM Query Content --\n", __func__, Utils::xml_get_node_content_as_text (inout_target_navaid.fpln_xml_osm_q_node) ) ); // DEBUG
  // #endif
  auto n_inv = inout_target_navaid.fpln_xml_osm_q_or_raw_tmpl_node.nChildNode (mxconst::get_ELEMENT_INVENTORY().c_str()); // DEBUG - remove


  IXMLNode external_inv_node = inout_target_navaid.fpln_xml_osm_q_or_raw_tmpl_node.getChildNode (mxconst::get_ELEMENT_INVENTORY().c_str()).deepCopy ();
  if (!external_inv_node.isEmpty())
  {
    xml_inv_node = Inventory::copy_items_from_one_inventory_to_the_other_xp11_style (xml_inv_node.deepCopy (), external_inv_node);
  }

  // set name
  Utils::xml_set_attribute_in_node_asString (xml_inv_node, mxconst::get_ATTRIB_NAME (), inv_name, xml_inv_node.getName());
  // set all attributes based on "in_attrib_list"
  Utils::xml_search_and_set_attributes_in_node (xml_inv_node, in_attrib_list);

  // add inventory track
  if (!mxUtils::isElementExists (inout_map_osm_inventory_track, in_seq) )
  {
    inout_map_osm_inventory_track[in_seq].fpln_seq = inout_target_navaid.fpln_seq;
    inout_map_osm_inventory_track[in_seq].inventory_name = inv_name;
  }

  #ifndef RELEASE
  n_inv = xml_inv_node.nChildNode(mxconst::get_ELEMENT_ITEM ().c_str ());
  Utils::xml_set_attribute_in_node <int>(xml_inv_node, "item_count", n_inv, xml_inv_node.getName());

  Log::logMsgThread (fmt::format ("[{}] Inventory after merge:\n{}\n<-- End Inventory after merge --\n", __func__, Utils::xml_get_node_content_as_text (xml_inv_node) ) ); // DEBUG
  #endif

  return xml_inv_node;
}

// -----------------------------------

void
RandomEngine::gen_target_inventory_scripts (missionx::NavAidInfo &in_target_navaid, std::unordered_map<int, mx_inventory_track_strct> &inout_map_osm_inventory_track)
{
  if (in_target_navaid.fpln_xml_inv_node.isEmpty ())
    return;


  // TODO: we need to have two scripts:
  // One is for "move" the item move.
  // Second "task need to check items are in plane and if need to move them out of plane once done.
  // ** This is best managed as a post step after all target Navaids are set and all inventories are configured.
  // This way we could fetch the expected target inventory name to drop

  const std::string inv_name = Utils::readAttrib (in_target_navaid.fpln_xml_inv_node, mxconst::get_ATTRIB_NAME (), "");
  if (inv_name.empty ())
    return;


  // fn_move_item_from_inv ("from inventory" *, "barcode" *, "quantity", "to inventory", "station_id" v24.12.2)
  const std::string script_name_to_move_item_to_plane = fmt::format ("script_move_items_{}", in_target_navaid.fpln_seq);
  const std::string script_name_to_remove_item_from_plane = fmt::format ("script_remove_items_{}", in_target_navaid.fpln_seq);

  std::string scriptlet_move_header = fmt::format ("<scriptlet name=\"{}\" >", script_name_to_move_item_to_plane);
  std::string scriptlet_move_body;
  std::string scriptlet_remove_header = fmt::format ("<scriptlet name=\"{}\" >", script_name_to_remove_item_from_plane);
  std::string scriptlet_remove_body;
  std::string scriptlet_footer = "</scriptlet>)";

  Log::logMsgThread (fmt::format ("[{}] Inventory content:\n{}\n<-- End inventory --\n", __func__, Utils::xml_get_node_content_as_text (in_target_navaid.fpln_xml_inv_node) ) ); // DEBUG

  // loop over all items and create a "move scriptlet" for them
  const int nItems = in_target_navaid.fpln_xml_inv_node.nChildNode (mxconst::get_ELEMENT_ITEM ().c_str ());
  for (int i1 = 0; i1 < nItems; ++i1)
  {
    IXMLNode          xItem     = in_target_navaid.fpln_xml_inv_node.getChildNode (mxconst::get_ELEMENT_ITEM ().c_str (), i1);
    const std::string barcode_s = Utils::readAttrib (xItem, mxconst::get_ATTRIB_BARCODE (), "");
    if (!barcode_s.empty ())
    {
      scriptlet_move_body.append (fmt::format (R"(fn_move_item_to_plane ( "{}", "{}") )", inv_name, barcode_s));
      scriptlet_remove_body.append (fmt::format ("if fn_is_item_exists_in_plane ( \"{}\" ) then \n"
                                                        "fn_move_item_from_inv (\"plane\", \"{}\")\n"
                                                        "fn_set_trigger_property (mxCurrentTrigger, \"script_conditions_met_b\", \"true\")\n"
                                                        "endif\n", barcode_s, barcode_s
                                                )
                                   );

    }
    std::string scriptlet_move_text = fmt::format ("{}\n\n{}\n{}", scriptlet_move_header, scriptlet_move_body, scriptlet_footer);
    std::string scriptlet_remove_text = fmt::format ("{}\n\n{}\n{}", scriptlet_remove_header, scriptlet_remove_body, scriptlet_footer);

    // Parse the <scriptlet> and check its validity before adding it to the <q> node.
    IXMLDomParser xml_dom_parser;
    IXMLResults   parse_result_strct;
    auto newMoveNode = xml_dom_parser.parseString (scriptlet_move_text.c_str (), mxconst::get_ELEMENT_SCRIPTLET ().c_str (), &parse_result_strct).deepCopy ();
    auto newRemoveNode = xml_dom_parser.parseString (scriptlet_remove_text.c_str (), mxconst::get_ELEMENT_SCRIPTLET ().c_str (), &parse_result_strct).deepCopy ();

    if ( !newMoveNode.isEmpty () && !newRemoveNode.isEmpty () )
    {
      in_target_navaid.fpln_xml_inv_node.addChild (newMoveNode );
      in_target_navaid.fpln_xml_inv_node.addChild (newRemoveNode );

      // track inventory script creation //
      inout_map_osm_inventory_track[in_target_navaid.fpln_seq].flag_created_move_and_remove_item_from_plane_script = true;

      inout_map_osm_inventory_track[in_target_navaid.fpln_seq].move_to_plane_script_name = script_name_to_move_item_to_plane;
      inout_map_osm_inventory_track[in_target_navaid.fpln_seq].remove_from_plane_script_name = script_name_to_remove_item_from_plane;

      inout_map_osm_inventory_track[in_target_navaid.fpln_seq].scriptlet_move_to_plane_text = scriptlet_move_text;
      inout_map_osm_inventory_track[in_target_navaid.fpln_seq].scriptlet_remove_from_plane_text = scriptlet_remove_text;

      inout_map_osm_inventory_track[in_target_navaid.fpln_seq].xml_move_to_plane_script_node = newMoveNode.deepCopy ();
      inout_map_osm_inventory_track[in_target_navaid.fpln_seq].xml_remove_from_plane_script_node = newRemoveNode.deepCopy ();
    }

  } // end loop over all items

}

// -----------------------------------

std::string
RandomEngine::gen_message_with_special_keywords_static (std::string inMessage, missionx::NavAidInfo &in_target_navaid)
{

  if (!inMessage.empty ())
  {
    std::map<std::string, std::string> mapReplaceMessageKeywords;
    //// v3.0.221.11 refine Flight Leg message
    mapReplaceMessageKeywords["{navaid_name}"]     = std::string (in_target_navaid.name);
    mapReplaceMessageKeywords["{navaid_icao}"]     = std::string (in_target_navaid.ID);
    mapReplaceMessageKeywords["{navaid_lat}"]      = in_target_navaid.getLat ();
    mapReplaceMessageKeywords["{navaid_lon}"]      = in_target_navaid.getLon ();
    mapReplaceMessageKeywords["{bearing_target}"]  = mxUtils::formatNumber<float> (in_target_navaid.bearing_to_current_target, 0);
    const auto elev_ft_s                           = Utils::formatNumber<float> (in_target_navaid.height_mt * missionx::meter2feet);
    mapReplaceMessageKeywords["{navaid_elev}"]     = (in_target_navaid.height_mt == 0.0f) ? "" : elev_ft_s;
    mapReplaceMessageKeywords["{navaid_loc_desc}"] = in_target_navaid.get_loc_desc ();
    mapReplaceMessageKeywords["{distance}"]        = (in_target_navaid.fpln_distance_between_prev_and_current_navaid <= 0.0) ? "n/a" : (Utils::formatNumber<double> (in_target_navaid.fpln_distance_between_prev_and_current_navaid, 0) + "nm");

    for (const auto &[stringToModify, stringToReplaceWith] : mapReplaceMessageKeywords) // replace all special keywords
    {
      inMessage = Utils::replaceString (inMessage, stringToModify, stringToReplaceWith, true);
    }
  }
  else
    Log::logMsgWarn (fmt::format ("[{}] Received empty message. Skipping...", __func__), true); // debug

  return inMessage;
}

// -----------------------------------


void
RandomEngine::gen_3d_marker_for_target (IXMLNode &inout_leg_node, missionx::NavAidInfo &in_target_navaid)
{
  assert (!inout_leg_node.isEmpty () && fmt::format("[{}] <leg> element can't be empty !!", __func__).c_str () );

  IXMLNode display_object_node = Utils::xml_get_node_from_XSD_map_as_a_copy ( mxconst::mxconst::get_ELEMENT_DISPLAY_OBJECT () );
  assert (!display_object_node.isEmpty () && fmt::format("[{}][{}] <display_object> element can't be empty !!", __func__, __LINE__ ).c_str () );

  const std::string leg_name = Utils::readAttrib ( inout_leg_node, mxconst::get_ATTRIB_NAME (), "");

  const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_display_target_marker = {
    { mxconst::get_ELEMENT_DISPLAY_OBJECT (), mxconst::get_ATTRIB_NAME (), "marker" },
    { mxconst::get_ELEMENT_DISPLAY_OBJECT (), mxconst::get_ATTRIB_SKEWED_NAME (), "marker_q" },
    { mxconst::get_ELEMENT_DISPLAY_OBJECT (), mxconst::get_ATTRIB_INSTANCE_NAME (), fmt::format("{}_marker_{}", leg_name, in_target_navaid.fpln_seq) },
    { mxconst::get_ELEMENT_DISPLAY_OBJECT (), mxconst::get_ATTRIB_REPLACE_LAT (), in_target_navaid.getLat () },
    { mxconst::get_ELEMENT_DISPLAY_OBJECT (), mxconst::get_ATTRIB_REPLACE_LONG (), in_target_navaid.getLon () },
    { mxconst::get_ELEMENT_DISPLAY_OBJECT (), mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT (), "60" },
  };

  // set the <display_object> node with the prepared "attribute list"
  Utils::xml_search_and_set_attributes_in_node (display_object_node, lsAttrib_display_target_marker);

  inout_leg_node.addChild (display_object_node); // add the marker node
}

// -----------------------------------

void
RandomEngine::gen_leg_start_messages (int &seq, NavAidInfo &inout_target_na, IXMLNode &inout_xml_messages)
{
  // add new messages to the "<leg>'s "start_message" sub-element.
  // The "inout_messages_node" is the main node that holds new created messages.
  if (inout_xml_messages.isEmpty ())
    return;

  inout_target_na.init_locDesc (); // force initializing the location description
  const std::string default_msg_text = fmt::format("Fly to: {}.", inout_target_na.get_loc_desc());
  const std::string msg_name = fmt::format ("start_msg_leg_{}", inout_target_na.fpln_seq);
  // create a new message and add to the inout_messages_node
  if (IXMLNode msg_node = inout_xml_messages.addChild ( mxconst::get_ELEMENT_MESSAGE ().c_str () );
    !msg_node.isEmpty ())
  {
    // set message name
    Utils::xml_set_attribute_in_node_asString ( msg_node, mxconst::get_ATTRIB_NAME (), msg_name, mxconst::get_ELEMENT_MESSAGE ());
    // Add <mix> node
    IXMLNode msg_mix_text_node = msg_node.addChild (mxconst::get_ELEMENT_MIX ().c_str ());
    // Set <mix> type
    Utils::xml_set_attribute_in_node_asString ( msg_mix_text_node, mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT (), msg_mix_text_node.getName ());
    // Add <cdata> message text
    Utils::xml_add_cdata (msg_mix_text_node, default_msg_text);

    // Add the message to the <leg> node as "<start_leg_message>"
    IXMLNode xml_start_msg = Utils::xml_get_or_create_node_ptr (inout_target_na.fpln_xml_target_leg_node, mxconst::get_ELEMENT_START_LEG_MESSAGE ());
    if (xml_start_msg.isEmpty ())
      xml_start_msg = inout_target_na.fpln_xml_target_leg_node.addChild (mxconst::get_ELEMENT_START_LEG_MESSAGE ().c_str ());

    assert ( !xml_start_msg.isEmpty () && fmt::format ("[{}] Failed to create or get node: ", __func__, mxconst::get_ELEMENT_START_LEG_MESSAGE ()).c_str () );

    // set the message name to call
    Utils::xml_set_attribute_in_node_asString ( xml_start_msg, mxconst::get_ATTRIB_NAME (), msg_name, xml_start_msg.getName ());
    seq++;
  }

}

// -----------------------------------

void
RandomEngine::gen_messages_when_reaching_target_leg (int &seq_trig, int &seq_msg, NavAidInfo &inout_target_na, IXMLNode &inout_xml_messages, IXMLNode &inout_xml_triggers, const IXMLNode &in_xml_land_trigger, const IXMLNode &in_xml_hover_trigger)
{
  assert (!inout_xml_messages.isEmpty () && !in_xml_land_trigger.isEmpty () && fmt::format ("[{}] One of the key parameters is empty and not valid.", __func__).c_str () );

  const bool  flag_wp_type_is_land_hover = (inout_target_na.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ());

  // ---------------------------------------
  // Prepare the landing and hover messages
  // ---------------------------------------
  // land message is used only in "land" cases
  const std::string land_msg_name = fmt::format ("msg_{}_leg_{}_target", seq_msg, inout_target_na.fpln_seq); // used only in Landing situations
  const std::string hover_msg_name = fmt::format ("msg_{}_leg_{}_enter_target_hover_area", seq_msg, inout_target_na.fpln_seq);
  // physical area message used only in "land_hover" cases
  const std::string land_msg_when_in_physical_area_name = (flag_wp_type_is_land_hover)? fmt::format ("leg_{}_entered_landing_phys_area_msg_{}", inout_target_na.fpln_seq, seq_msg) : "";
  seq_msg++;


  const auto lmbda_get_land_in_target_text =[flag_wp_type_is_land_hover = flag_wp_type_is_land_hover] (missionx::NavAidInfo &in_na)
  {



    if (in_na.fpln_task_type == enums::mx_rnd_task_type::medevac)
    {
      switch (static_cast<int>( in_na.fpln_mission_type) )
      {
        case static_cast<int>(missionx::enums::mx_user_picked_mission_type::oilrig_medevac):
        {
          constexpr auto land_med_target_oilrig     = "Wait for the Oil Rig team to move the patients in or out of the plane.";
          constexpr auto land_med_extraction_oilrig = "You reached {1}, wait for the patient to be taken out of the plane.";
          if (in_na.fpln_mission_phase == enums::mx_rnd_mission_phase::land_target )
            return std::string(land_med_target_oilrig);

          std::map<int, std::string> data = {{1, in_na.get_loc_desc ()}};
          return std::string( mxUtils::format (land_med_extraction_oilrig, data) );
        }
        break;
        default: // all the rest
        {
          constexpr auto land_wp_land_message       = "Remain on standby until the medical team has transferred the patient into the helicopter."; //"Wait for the medical team to bring the patient into the helicopter.";
          constexpr auto land_hover_wp_land_message = "Wait for the medical team to hoist the patient and load them into the helicopter.";

          if (in_na.fpln_mission_phase == enums::mx_rnd_mission_phase::land_extraction )
          {
            // if (in_na.nav_aid_has_unique_name ())
            return  (flag_wp_type_is_land_hover)? fmt::format ("You landed in the '{}' area. {}", in_na.get_loc_desc (), land_hover_wp_land_message ) :
                                                  fmt::format ("You landed at '{}'. {}", in_na.get_loc_desc (), land_wp_land_message );
          }

          // if (in_na.fpln_mission_phase == enums::mx_rnd_mission_phase::land_target )
          return (flag_wp_type_is_land_hover)? std::string(land_hover_wp_land_message) :
                                               std::string(land_wp_land_message);
        }
      } // end switch fpln_mission_type
    } // end switch fpln_task_type == medevac

    if (in_na.fpln_task_type == enums::mx_rnd_task_type::cargo)
    {
      switch (static_cast<int>( in_na.fpln_mission_type) )
      {
        case static_cast<int>(missionx::enums::mx_user_picked_mission_type::oilrig_cargo):
        {
          constexpr auto land_cargo_target_oilrig     = "Wait for the Oil Rig cargo to be moved out and into the plane.";
          constexpr auto land_cargo_extraction_oilrig = "You reached {1}. Turn off the plane.";

          if (in_na.fpln_mission_phase == enums::mx_rnd_mission_phase::land_target )
            return std::string(land_cargo_target_oilrig);

          std::map<int, std::string> data = {{1, in_na.get_loc_desc ()}};
          return std::string( mxUtils::format (land_cargo_extraction_oilrig, data) );
        }
        break;
        default: // all the rest
        {
          constexpr auto land_cargo_target          = "Move the cargo in or out of the plane.";

          if (in_na.fpln_mission_phase == enums::mx_rnd_mission_phase::land_target )
              return std::string(land_cargo_target);

          constexpr auto land_cargo_extraction = "You reached {1}. Wait to move the last cargo and turn off the plane.";
          std::map<int, std::string> data = {{1, in_na.get_loc_desc ()}};
          return std::string( mxUtils::format (land_cargo_extraction, data) );

        }
      } // end switch fpln_mission_type
    }

    return std::string ("No valid information was gathered. Inform the plugin developer and include the mission file.");
  };

  const std::string landed_in_area_msg_text                = lmbda_get_land_in_target_text (inout_target_na);
  const std::string entered_physical_landing_area_msg_text = "You should be close enough to land";  // "You entered the landing area.";
  const std::string entered_hover_area_msg_text            = fmt::format ("You entered the hover area. {}", ((flag_wp_type_is_land_hover) ? "You can decide if to hover or to land." : ""));


  // <mix> base properties
  const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_mix_text = {
    { mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_ATTRIB_TEXT ()  },
    { mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_LABEL (), "radio" },
    { mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_LABEL_COLOR (), mxconst::get_YELLOW () },
  };

  // -----------------------------
  // create the message nodes
  // -----------------------------

  // Create the template <message> node
  IXMLNode msg_template_node = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_MESSAGE ());
  Utils::xml_delete_all_subnodes (msg_template_node ); // delete all subnodes and prepare one <mix> subnode

  IXMLNode    mix_subnode   = msg_template_node.addChild (mxconst::get_ELEMENT_MIX ().c_str ()); // create <mix> subnode

  Utils::xml_search_and_set_attributes_in_node (mix_subnode, lsAttrib_mix_text); // set base properties

  // create <message> based on template for "land" hover" and "entering physical area"
  IXMLNode msg_land_node                        = msg_template_node.deepCopy ();
  IXMLNode msg_hover_node                       = msg_template_node.deepCopy ();
  IXMLNode msg_land_entered_physical_area_node  = msg_template_node.deepCopy ();
  // IXMLNode msg_you_entered_the_target_area_node = msg_template_node.deepCopy ();

  // set names
  Utils::xml_set_attribute_in_node_asString (msg_land_node, mxconst::get_ATTRIB_NAME (), land_msg_name, msg_land_node.getName ());
  Utils::xml_set_attribute_in_node_asString (msg_hover_node, mxconst::get_ATTRIB_NAME (), hover_msg_name, msg_hover_node.getName ());
  Utils::xml_set_attribute_in_node_asString (msg_land_entered_physical_area_node, mxconst::get_ATTRIB_NAME (), land_msg_when_in_physical_area_name, msg_land_entered_physical_area_node.getName ());

  // set message text
  IXMLNode land_mix_text_node                  = Utils::xml_get_or_create_node_ptr (msg_land_node, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT ());
  IXMLNode hover_mix_text_node                 = Utils::xml_get_or_create_node_ptr (msg_hover_node, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT ());
  IXMLNode land_mix_entered_physical_area_node = Utils::xml_get_or_create_node_ptr (msg_land_entered_physical_area_node, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT ());
  // IXMLNode land_mix_entered_target_area = Utils::xml_get_or_create_node_ptr (msg_you_entered_the_target_area_node, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT ());

  Utils::xml_add_cdata (land_mix_text_node, landed_in_area_msg_text);
  Utils::xml_add_cdata (hover_mix_text_node, entered_hover_area_msg_text);
  Utils::xml_add_cdata (land_mix_entered_physical_area_node, entered_physical_landing_area_msg_text);
  // Utils::xml_add_cdata (land_mix_entered_target_area, MSG_ENTERED_TARGET_AREA_TEXT);


  // --------------------------------------------------
  // create the triggers that will fire the messages
  // --------------------------------------------------

  const std::string trig_land_name  = fmt::format ("trig_{}_leg_{}_msg_land", seq_trig, inout_target_na.fpln_seq );
  const std::string trig_hover_name = fmt::format ("trig_{}_leg_{}_msg_hover", seq_trig, inout_target_na.fpln_seq);

  // Prepare landing trigger attributes
  std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_trig_landing_area = {
    { mxconst::get_ELEMENT_TRIGGER (), mxconst::get_ATTRIB_NAME (), trig_land_name },
    { mxconst::get_ELEMENT_TRIGGER (), mxconst::get_ATTRIB_RE_ARM (), "true" },
    { mxconst::get_ELEMENT_CONDITIONS (), mxconst::get_ATTRIB_PLANE_ON_GROUND (), "true" },
    { mxconst::get_ELEMENT_OUTCOME (), mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED (), land_msg_name },
  };

  if (flag_wp_type_is_land_hover) // we will add the physical entry message only in the case of land + hover navaids
    lsAttrib_trig_landing_area.emplace_back( mxconst::get_ELEMENT_OUTCOME (), mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_ENTERING_PHYSICAL_AREA (), land_msg_when_in_physical_area_name );

  // Prepare hover trigger attributes
  const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_trig_hover_area = {
    { mxconst::get_ELEMENT_TRIGGER (), mxconst::get_ATTRIB_NAME (), trig_hover_name },
    { mxconst::get_ELEMENT_TRIGGER (), mxconst::get_ATTRIB_RE_ARM (), "true" },
    { mxconst::get_ELEMENT_CONDITIONS (), mxconst::get_ATTRIB_PLANE_ON_GROUND (), "false" },
    { mxconst::get_ELEMENT_OUTCOME (), mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED (), hover_msg_name },
  };

  // Duplicate trigger nodes
  IXMLNode trig_land_msg_node  = in_xml_land_trigger.deepCopy ();
  IXMLNode trig_hover_msg_node = in_xml_hover_trigger.deepCopy ();

  // clear land <outcome> node
  IXMLNode xml_outcome_node = trig_land_msg_node.getChildNode (mxconst::get_ELEMENT_OUTCOME ().c_str ());
  Utils::xml_delete_all_node_attributes (xml_outcome_node);
  // clear hover <outcome> node
  xml_outcome_node = trig_hover_msg_node.getChildNode (mxconst::get_ELEMENT_OUTCOME ().c_str ());
  Utils::xml_delete_all_node_attributes (xml_outcome_node);


  // set attributes
  Utils::xml_search_and_set_attributes_in_node (trig_land_msg_node, lsAttrib_trig_landing_area);
  Utils::xml_search_and_set_attributes_in_node (trig_hover_msg_node, lsAttrib_trig_hover_area);

  seq_trig++;

  // add triggers to <triggers> root node
  inout_xml_triggers.addChild (trig_land_msg_node);
  inout_xml_triggers.addChild (trig_hover_msg_node);

  // ----------------------------------------------
  // Add the nodes to the global root nodes
  // ----------------------------------------------

  // add the messages to the <messages_template> root node
  inout_xml_messages.addChild (msg_land_node); // deprecated, only in physical area message is relevant
  // if ( Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (inout_xml_messages, mxconst::get_ELEMENT_MESSAGE (), mxconst::get_ATTRIB_NAME (), DEFAULT_ENTERED_TARGET_AREA_MSG_NAME ).isEmpty () )
  //   inout_xml_messages.addChild (msg_you_entered_the_target_area_node);

  if (flag_wp_type_is_land_hover)
  {
    inout_xml_messages.addChild (msg_land_entered_physical_area_node);
    inout_xml_messages.addChild (msg_hover_node);
  }


  // add triggers to <leg>
  // Utils::xml_add_comment ( inout_target_na.fpln_xml_target_leg_node, " >>> Message Triggers <<< " );

  IXMLNode link_trigger = inout_target_na.fpln_xml_target_leg_node.addChild (mxconst::get_ELEMENT_LINK_TO_TRIGGER ().c_str ());
  Utils::xml_set_attribute_in_node_asString (link_trigger, mxconst::get_ATTRIB_NAME (), trig_land_name, link_trigger.getName ());
  if (flag_wp_type_is_land_hover)
  {
    link_trigger = inout_target_na.fpln_xml_target_leg_node.addChild (mxconst::get_ELEMENT_LINK_TO_TRIGGER ().c_str ());
    Utils::xml_set_attribute_in_node_asString (link_trigger, mxconst::get_ATTRIB_NAME (), trig_hover_name, link_trigger.getName ());
  }

}

// -----------------------------------

void
RandomEngine::gen_2nm_message (int &seq_trig, int &seq_msg, NavAidInfo &inout_target_na, IXMLNode &inout_xml_messages, IXMLNode &inout_xml_triggers, const IXMLNode &in_xml_land_trigger)
{
  assert (!inout_xml_messages.isEmpty () && !in_xml_land_trigger.isEmpty () && fmt::format ("[{}] One of the key parameters is empty and not valid.", __func__).c_str () );
  const bool  flag_wp_type_is_land_hover = (inout_target_na.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ());

  const std::string message_name = fmt::format ("msg_{}_leg_{}_near_target_2m", seq_msg, inout_target_na.fpln_seq);
  seq_msg++;

  const std::string message_text = fmt::format ("You are nearing {}.\n{}",
                                                inout_target_na.get_loc_desc (),
                                                (flag_wp_type_is_land_hover) ? "Look for a landing spot near the target. Alternatively, you may hover above it." : "Prepare for landing."
    );

  // -----------------------------
  // create the message node
  // -----------------------------
  // <mix> base properties
  // Create the template <message> node
  IXMLNode msg_template_node = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_MESSAGE ());
  Utils::xml_delete_all_subnodes (msg_template_node ); // delete all subnodes and prepare one <mix> subnode

  IXMLNode    mix_subnode   = msg_template_node.addChild (mxconst::get_ELEMENT_MIX ().c_str ()); // create <mix> subnode

  // set the attributes of the <mix> subnode
  const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_mix_text = {
    { mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_ATTRIB_TEXT ()  },
    { mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_LABEL (), "radio" },
    { mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_LABEL_COLOR (), mxconst::get_YELLOW () },
  };
  Utils::xml_search_and_set_attributes_in_node (mix_subnode, lsAttrib_mix_text); // set base properties

  // create <message> based on template for "land" hover" and "entering physical area"
  IXMLNode msg_land_node = msg_template_node.deepCopy ();
  Utils::xml_set_attribute_in_node_asString (msg_land_node, mxconst::get_ATTRIB_NAME (), message_name, msg_land_node.getName ());

  IXMLNode land_mix_text_node = Utils::xml_get_or_create_node_ptr (msg_land_node, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT ());
  Utils::xml_add_cdata (land_mix_text_node, message_text);

  // -------------------------------------------------
  // create the trigger that will fire the messages
  // -------------------------------------------------

  const std::string trigger_name  = fmt::format ("trig_{}_leg_{}_near_target_2m", seq_trig, inout_target_na.fpln_seq );
  const std::string radius_mt_2nm = fmt::format("{}", 2.0 * missionx::nm2meter);

  // Prepare landing trigger attributes
  const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_trig_landing_area = {
    { mxconst::get_ELEMENT_TRIGGER (), mxconst::get_ATTRIB_NAME (), trigger_name },
    { mxconst::get_ELEMENT_TRIGGER (), mxconst::get_ATTRIB_RE_ARM (), "true" },
    { mxconst::get_ELEMENT_RADIUS (), mxconst::get_ATTRIB_LENGTH_MT (), radius_mt_2nm },
    { mxconst::get_ELEMENT_CONDITIONS (), mxconst::get_ATTRIB_PLANE_ON_GROUND (), "" },
    { mxconst::get_ELEMENT_OUTCOME (), mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED (), message_name },
  };

  IXMLNode trig_land_msg_node  = in_xml_land_trigger.deepCopy ();
  // clear land <outcome> node
  IXMLNode xml_outcome_node = trig_land_msg_node.getChildNode (mxconst::get_ELEMENT_OUTCOME ().c_str ());
  Utils::xml_delete_all_node_attributes (xml_outcome_node);

  // set attributes
  Utils::xml_search_and_set_attributes_in_node (trig_land_msg_node, lsAttrib_trig_landing_area);
  seq_trig++;

  // add triggers to <triggers> root node
  inout_xml_triggers.addChild (trig_land_msg_node);

  // add the messages to the <messages_template> root node
  inout_xml_messages.addChild (msg_land_node);

  // add trigger to <leg>
  IXMLNode link_trigger_ptr = inout_target_na.fpln_xml_target_leg_node.addChild (mxconst::get_ELEMENT_LINK_TO_TRIGGER ().c_str ());
  Utils::xml_set_attribute_in_node_asString (link_trigger_ptr, mxconst::get_ATTRIB_NAME (), trigger_name, link_trigger_ptr.getName ());

}

// -----------------------------------

void
RandomEngine::gen_parse_and_add_all_display_objects_in_node (const std::string &in_which_func_called, const IXMLNode &in_source_node, IXMLNode &inout_target_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d)
{
  const int nDisplayObjects = in_source_node.nChildNode ();
  for (int i1 = 0; i1 < nDisplayObjects; ++i1)
  {
    // get sub-node
    auto cNode = in_source_node.getChildNode (i1).deepCopy ();
    if (cNode.isEmpty ())
      continue;

    // filter out sub-nodes that are not <display_xxx> elements
    std::string tag = cNode.getName ();
    if (tag != mxconst::get_ELEMENT_DISPLAY_OBJECT () && tag != mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ())
      continue; // skip elements that are not <display_object> not <display_object_near_plane

    #ifndef RELEASE
    Log::logMsgThread (fmt::format ("[{}]Adding 3D display_objects from: {}:{}", in_which_func_called, tag, Utils::readAttrib (cNode, mxconst::get_ATTRIB_NAME (), "") ) );
    #endif

    if (std::string err
      ; RandomEngine::parse_display_object_element (inout_target_node, cNode, in_template_node, inout_x3DObjTemplate, in_expected_slope_at_target_location_d, err)) // v25.06.1 extended function signature // v3.0.219.1 handle <display_object> options like: optional, random_water or limit_to_terrain_slope
    {
      if (tag == mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ())
      {
        //  Force replace_lat or replace_long with fake values to be on the safe side
        cNode.updateAttribute ("1.0", mxconst::get_ATTRIB_REPLACE_LAT ().c_str (), mxconst::get_ATTRIB_REPLACE_LAT ().c_str ());
        cNode.updateAttribute ("1.0", mxconst::get_ATTRIB_REPLACE_LONG ().c_str (), mxconst::get_ATTRIB_REPLACE_LONG ().c_str ());
      }

      inout_target_node.addChild (cNode.deepCopy (), inout_target_node.nChildNode ());
    }
  }

}

// -----------------------------------


void
RandomEngine::gen_3d_hint_objects_for_land_and_hover (const NavAidInfo &inout_target_na, IXMLNode &inout_leg_node, const NavAidInfo *next_navaid_ptr)
{
  // generate 3D hint for landing

  // store stats as int
  const int LANDING_RADIUS_FOR_LAND_HOVER_MT = mxUtils::stringToNumber<int> (mxconst::DEFAULT_LAND_OR_INV_RADIUS_MT.data ());
  const int LANDING_RADIUS_FOR_LAND_ONLY_MT  = mxUtils::stringToNumber<int> (mxconst::DEFAULT_LAND_ONLY_RADIUS_MT.data ());
  const int HOVER_RADIUS_MT                  = mxUtils::stringToNumber<int> (mxconst::DEFAULT_HOVER_RADIUS_MT.data ());

  const std::string NEXT_LEG_NAME = (next_navaid_ptr == nullptr)? "" : next_navaid_ptr->fpln_leg_name;

  int seq = 0;

  // calculate LANDING HINT <display_object>, using 350 meters with 24 3D objects.
  const int landing_radius_mt = (mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER () == inout_target_na.fpln_wp_type) ? LANDING_RADIUS_FOR_LAND_HOVER_MT : LANDING_RADIUS_FOR_LAND_ONLY_MT;
  const int how_many_3d_objects_to_display  = (mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER () == inout_target_na.fpln_wp_type) ? 24 : 4;

  auto vec_display_objects_hint = missionx::RandomEngine::gen_land_hover_display_objects (inout_target_na.lat, inout_target_na.lon, landing_radius_mt, how_many_3d_objects_to_display, seq);
  if (!vec_display_objects_hint.empty ())
  {
      Utils::xml_add_comment (inout_leg_node, "<<< Land 3D hint >>>");

    for (auto &xml : vec_display_objects_hint)
    {
      if ( ! NEXT_LEG_NAME.empty () )
        Utils::xml_set_attribute_in_node_asString (xml, fmt::format("replace_{}", mxconst::get_ATTRIB_KEEP_UNTIL_LEG ()), NEXT_LEG_NAME, xml.getName ()); // add visibility rule

      inout_leg_node.addChild (xml.deepCopy ());
    }
  }

  // calculate HOVER HINT <display_object>, using ~60 meters with 24 3D objects.
  if (mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER () == inout_target_na.fpln_wp_type )
  {
    vec_display_objects_hint.clear ();
    vec_display_objects_hint = missionx::RandomEngine::gen_land_hover_display_objects (inout_target_na.lat, inout_target_na.lon, HOVER_RADIUS_MT, 4, seq);
    if (!vec_display_objects_hint.empty ())
    {
      Utils::xml_add_comment (inout_leg_node, "<<< Hover 3D hint >>>");
      for (auto &xml : vec_display_objects_hint)
      {
        Utils::xml_set_attribute_in_node_asString (xml, fmt::format("replace_{}", mxconst::get_ATTRIB_KEEP_UNTIL_LEG ()), NEXT_LEG_NAME, xml.getName ()); // add visibility rule
        inout_leg_node.addChild (xml.deepCopy ());
      }
    }
  }
  Utils::xml_add_comment (inout_leg_node, "<<< END 3D HINTS >>>");

}

// -----------------------------------

void
RandomEngine::gen_add_3d_objects_for_surprise_me_base_on_predefined_attributes (const NavAidInfo &inout_target_na, IXMLNode &inout_leg_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d)
{
  enum class enum_set_3d_source: uint8_t
  {
    none = 0,
    header = 1,
    desc = 2,
    display_object_set = 3 // compatible with a regular template
  };
  struct set_3d_strct
  {
    std::string random_tag;
    std::string set_name;
    std::string slope_set_name;
  };


  // --------------------------------
  // --- Get <desc> element and figure out which element has the 3D attributes information, the header or the <desc> node.
  // --------------------------------

  std::unordered_map<enum_set_3d_source, set_3d_strct> map_3d_set_attributes;
  // read <desc> node
  const IXMLNode xml_desc_ptr = inout_leg_node.getChildNode (mxconst::get_ELEMENT_DESC ().c_str ());
  // read 3D set related attributes from osm target header
  map_3d_set_attributes[enum_set_3d_source::header].random_tag     = Utils::readAttrib (inout_target_na.fpln_xml_q_tag_header_node, mxconst::get_ATTRIB_RANDOM_TAG (), "");
  map_3d_set_attributes[enum_set_3d_source::header].set_name       = Utils::readAttrib (inout_target_na.fpln_xml_q_tag_header_node, mxconst::get_ATTRIB_SET_NAME (), "");
  map_3d_set_attributes[enum_set_3d_source::header].slope_set_name = Utils::readAttrib (inout_target_na.fpln_xml_q_tag_header_node, mxconst::get_ATTRIB_SLOPE_SET_NAME (), "");
  // read the same from the <desc> sub-element of <leg>.
  map_3d_set_attributes[enum_set_3d_source::desc].random_tag     = Utils::readAttrib (xml_desc_ptr, mxconst::get_ATTRIB_RANDOM_TAG (), "");
  map_3d_set_attributes[enum_set_3d_source::desc].set_name       = Utils::readAttrib (xml_desc_ptr, mxconst::get_ATTRIB_SET_NAME (), "");
  map_3d_set_attributes[enum_set_3d_source::desc].slope_set_name = Utils::readAttrib (xml_desc_ptr, mxconst::get_ATTRIB_SLOPE_SET_NAME (), "");
  // Backwards compatibility, read sub-element <display_object_set> from the <leg>
  map_3d_set_attributes[enum_set_3d_source::display_object_set].random_tag     = Utils::readAttrib (inout_leg_node, mxconst::get_ATTRIB_RANDOM_TAG (), "");
  map_3d_set_attributes[enum_set_3d_source::display_object_set].set_name       = Utils::readAttrib (inout_leg_node, mxconst::get_ATTRIB_SET_NAME (), "");
  map_3d_set_attributes[enum_set_3d_source::display_object_set].slope_set_name = Utils::readAttrib (inout_leg_node, mxconst::get_ATTRIB_SLOPE_SET_NAME (), "");

  const auto lmbda_which_3d_set_to_pick_from =[] (std::unordered_map<enum_set_3d_source, set_3d_strct>& map_3d_set_attributes)
  {
    // v25.09.1 backwards compatibility with <display_object_set> nodes.
    if (!map_3d_set_attributes[enum_set_3d_source::display_object_set].random_tag.empty ())
      return enum_set_3d_source::display_object_set;

    if (map_3d_set_attributes[enum_set_3d_source::desc].random_tag.empty () && !map_3d_set_attributes[enum_set_3d_source::header].random_tag.empty ())
      return enum_set_3d_source::header;

    if (!map_3d_set_attributes[enum_set_3d_source::desc].random_tag.empty ())
      return enum_set_3d_source::desc;

    return enum_set_3d_source::none;
  };

  const enum_set_3d_source picked_3d_set_source = lmbda_which_3d_set_to_pick_from (map_3d_set_attributes);
  if (picked_3d_set_source != enum_set_3d_source::none)
  {
    std::string set_name_to_pick = map_3d_set_attributes[picked_3d_set_source].set_name;
    // check slope and use a slope set if it has value.
    if (inout_target_na.fpln_slope > (missionx::data_manager::Max_Slope_To_Land_On * 3.0f) && !map_3d_set_attributes[picked_3d_set_source].slope_set_name.empty () )
      set_name_to_pick = map_3d_set_attributes[picked_3d_set_source].slope_set_name;

    #ifndef RELEASE
    Log::logMsgThread ( fmt::format ("[{}] Search 3D set_name: {}", __func__, set_name_to_pick) );
    #endif


    //////////////////////////////////////////
    // ADD DISPLAY_OBJECT
    // Find the correct "set"
    // Add all <display_object> elements
    ///////////////////////////////////////

    if (const IXMLNode xTag = in_template_node.getChildNode (map_3d_set_attributes[picked_3d_set_source].random_tag.c_str ())
      ; !xTag.isEmpty ())
    {
      int nSubNodes = 0;
      // check child tag
      if (set_name_to_pick.empty ())
        nSubNodes = xTag.nChildNode ();
      else
        nSubNodes = xTag.nChildNode (set_name_to_pick.c_str ());

      // Pick a <dub-node>
      if (nSubNodes > 0)
      {
        IXMLNode  cTagNode;
        const int randomChild_i = Utils::getRandomIntNumber (0, nSubNodes - 1);
        if (set_name_to_pick.empty ())
          cTagNode = xTag.getChildNode (randomChild_i);
        else
          cTagNode = xTag.getChildNode (set_name_to_pick.c_str (), randomChild_i);

        if (!cTagNode.isEmpty ())
        {
          Utils::xml_add_comment ( inout_leg_node, " >>> Display Objects <<< ");
          // lmbda_add_all_display_object_xxx_elements (__func__, cTagNode, inout_leg_node);
          RandomEngine::gen_parse_and_add_all_display_objects_in_node (__func__, cTagNode, inout_leg_node, in_template_node, inout_x3DObjTemplate, in_expected_slope_at_target_location_d);

          #ifndef RELEASE
          const int nDisplayObjects = cTagNode.nChildNode (mxconst::get_ELEMENT_DISPLAY_OBJECT ().c_str ()) + cTagNode.nChildNode (mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ().c_str ());
          if (nDisplayObjects == 0)
            Log::logMsgThread (fmt::format("[{}] Failed to find a valid display set: {} . Will try to search for <display_object> instead.", __func__, cTagNode.getName () ) );
          #endif

        } // end if the template tag name was found

      } // end if nChilds > 0

    } // end if xTag is not Empty

  } // end if picked_3d_set_source is not "::none"

}


// -----------------------------------


bool
RandomEngine::gen_add_3d_display_object_sets_instances_to_leg (const NavAidInfo &inout_target_na, IXMLNode &inout_leg_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d)
{
  const int nChilds = inout_leg_node.nChildNode (); // get all children
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    // get node and skip all nodes that are not DISPLAY_OBJECT_SET type
    IXMLNode xNode = inout_leg_node.getChildNode (i1);
    if (xNode.isEmpty ())
      continue;

    // filter out by tag name
    const std::string tagName = xNode.getName ();
    if (tagName != mxconst::get_ELEMENT_DISPLAY_OBJECT_SET ())
      continue;

    std::string random_set_to_pick_from = Utils::readAttrib (xNode, mxconst::get_ATTRIB_RANDOM_TAG (), "");
    std::string set_name_to_pick = Utils::readAttrib (xNode, mxconst::get_ATTRIB_SET_NAME (), "");

    //////////////////////////////////////////
    // ADD DISPLAY_OBJECT
    // Find the correct "set"
    // Add all <display_object> elements
    ///////////////////////////////////////

    if (const IXMLNode xTag = Utils::xml_get_node_randomly_by_name_IXMLNode (in_template_node, random_set_to_pick_from)
      ; !xTag.isEmpty ())
    {
      int nSubNodes = 0;
      // check child tag
      if (set_name_to_pick.empty ())
        nSubNodes = xTag.nChildNode ();
      else
        nSubNodes = xTag.nChildNode (set_name_to_pick.c_str ());

      // Pick a <sub-node>
      if (nSubNodes > 0)
      {
        IXMLNode  cTagNode;
        const int randomChild_i = Utils::getRandomIntNumber (0, nSubNodes - 1);
        if (set_name_to_pick.empty ())
          cTagNode = xTag.getChildNode (randomChild_i);
        else
          cTagNode = xTag.getChildNode (set_name_to_pick.c_str (), randomChild_i);

        if (!cTagNode.isEmpty ())
        {
          Utils::xml_add_comment ( inout_leg_node, " >>> Display Objects <<< ");
          RandomEngine::gen_parse_and_add_all_display_objects_in_node (__func__, cTagNode, inout_leg_node, in_template_node, inout_x3DObjTemplate, in_expected_slope_at_target_location_d);

          #ifndef RELEASE
          const int nDisplayObjects = cTagNode.nChildNode (mxconst::get_ELEMENT_DISPLAY_OBJECT ().c_str ()) + cTagNode.nChildNode (mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ().c_str ());
          if (nDisplayObjects == 0)
            Log::logMsgThread (fmt::format("[{}] Failed to find a valid display set: {} . Will try to search for <display_object> instead.", __func__, cTagNode.getName () ) );
          #endif

        } // end if the template tag name was found

      } // end if nChilds > 0

    } // end if xTag is not Empty

  } // end loop over all sub-nodes of the <leg>

  return true;
}

// -----------------------------------

bool
RandomEngine::gen_parse_3d_instances_in_leg (IXMLNode &legNode_ptr, missionx::NavAidInfo &in_target_navaid)
{

  const int nChilds = legNode_ptr.nChildNode (); // v3.303.11 get all children
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    // get node and skip all nodes that are not DISPLAY_OBJECT type
    IXMLNode xNode = legNode_ptr.getChildNode (i1);
    if (xNode.isEmpty ())
      continue;

    // filter out by tag name
    const std::string tagName = xNode.getName ();
    if (tagName != mxconst::get_ELEMENT_DISPLAY_OBJECT () && tagName != mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ())
      continue;


    std::string obj3d_name = Utils::readAttrib (xNode, mxconst::get_ATTRIB_NAME (), "");
    // xNode.updateAttribute (obj3d_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str ()); // change element name value // v25.06.1 useless update attribute
    // xNode.deleteAttribute (mxconst::get_ATTRIB_INSTANCE_NAME ().c_str ()); // will be constructed next


    std::string instName = obj3d_name + "_" + Utils::readAttrib (legNode_ptr, mxconst::get_ATTRIB_NAME (), "") + "_" + Utils::formatNumber<int> (i1);
    Utils::xml_set_attribute_in_node_asString (xNode, mxconst::get_ATTRIB_INSTANCE_NAME (), instName, xNode.getName ());

    // special validation and initialization of <display_object> element only
    if (tagName == mxconst::get_ELEMENT_DISPLAY_OBJECT ())
    {
      std::string replaceLat                  = Utils::readAttrib (xNode, mxconst::get_ATTRIB_REPLACE_LAT (), "");
      std::string replaceLon                  = Utils::readAttrib (xNode, mxconst::get_ATTRIB_REPLACE_LONG (), "");
      std::string replaceElev_ft              = Utils::readAttrib (xNode, mxconst::get_ATTRIB_REPLACE_ELEV_FT (), "");
      int         replaceElevAboveGround_ft_i = Utils::readNodeNumericAttrib<int> (xNode, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT (), 0);


      // v3.0.219.1 calculate 3D object location relative to target
      std::string relative_pos_bearing_deg_distance_mt = Utils::readAttrib (xNode, mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT (), "");
      if (const std::vector<int> vecRelativePos = Utils::splitStringToNumbers<int> (relative_pos_bearing_deg_distance_mt, mxconst::get_PIPE_DELIMITER ()); vecRelativePos.size () > 1)
      {
        double newLat, newLon, trigLat, trigLon, newBearing;
        newLat = newLon = trigLat = trigLon = newBearing = 0.0;

        if (in_target_navaid.lat != 0.0 && in_target_navaid.lon != 0.0)
        {
          // calculate new targetLat/long
          auto distance_nm = static_cast<double> (vecRelativePos.at (1)) * meter2nm;
          auto bearing     = static_cast<float> (vecRelativePos.at (0));
          Utils::calcPointBasedOnDistanceAndBearing_2DPlane (newLat, newLon, in_target_navaid.lat, in_target_navaid.lon, bearing, distance_nm);

          // set new targetLat/long in instance replace point data
          Utils::xml_set_attribute_in_node <double>(xNode, mxconst::get_ATTRIB_REPLACE_LAT (), newLat, xNode.getName ());
          Utils::xml_set_attribute_in_node <double>(xNode, mxconst::get_ATTRIB_REPLACE_LONG (), newLon, xNode.getName ());
        }

        // v25.06.1
        xNode.updateAttribute (relative_pos_bearing_deg_distance_mt.c_str (), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS ().c_str (), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS ().c_str ()); // Keep the value in a debug attribute
        const std::set<std::string> set_attrib_to_del = {mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT ()};
        Utils::xml_delete_attribute (xNode, set_attrib_to_del, xNode.getName ());

        // set default above ground only if "replace_elev_ft" does not exist and attribute "replace_elev_above_ground_ft" exists
        if (replaceElev_ft.empty () && replaceElevAboveGround_ft_i != 0)
          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT (), fmt::format("{}", replaceElevAboveGround_ft_i), mxconst::get_ELEMENT_DISPLAY_OBJECT ()); //

        // end calculate relative location to target of 3D object
      }
      else if ( !relative_pos_bearing_deg_distance_mt.empty () || vecRelativePos.size () == 1 ) // [regression bug fix] reset relative value so plugin won't re-calculate it again when parsing the instance node.
      {
        xNode.updateAttribute (relative_pos_bearing_deg_distance_mt.c_str (), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS ().c_str (), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS ().c_str ()); // Keep the value in a debug attribute
        const std::set<std::string> set_attrib_to_del = {mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT ()};
        Utils::xml_delete_attribute (xNode, set_attrib_to_del, xNode.getName ());
      }
      else if (!obj3d_name.empty ()) // if we have no relative location information, then place at target position
      {
        // define replace_lat/replace_long WITH TARGET POSITION (LAT/LON) if one of them is not set
        if (replaceLat.empty () || replaceLon.empty ())
        {
          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LAT (), in_target_navaid.getLat (), mxconst::get_ELEMENT_DISPLAY_OBJECT ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LONG (), in_target_navaid.getLon (), mxconst::get_ELEMENT_DISPLAY_OBJECT ());
        }

        // set default above ground only if "replace_elev_ft" does not exist and attribute "replace_elev_above_ground_ft" exists
        if (replaceElev_ft.empty () && replaceElevAboveGround_ft_i != 0)
          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT (), fmt::format("{}", replaceElevAboveGround_ft_i), mxconst::get_ELEMENT_DISPLAY_OBJECT ()); //
      }

      // Skew location: place target instances not in their exact locations based on SETUP screen.
      const bool flag_display_target_markers_away_from_target = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET (), false);
      const bool is_medevac_mission = Utils::readNodeNumericAttrib<int> (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::_ui_mission_type::medevac)) == static_cast<int> (missionx::_ui_mission_type::medevac);

      if (const bool target_marker_b = Utils::readBoolAttrib (xNode, mxconst::get_ATTRIB_TARGET_MARKER_B (), false)
        ; flag_display_target_markers_away_from_target
          && target_marker_b
          && ! in_target_navaid.fpln_is_last_flight_leg
          && ! in_target_navaid.xml_skewdPointNode.isEmpty () // v25.06.1
          && is_medevac_mission)
      {
        if (std::string skewed_name = Utils::readAttrib (xNode, mxconst::get_ATTRIB_SKEWED_NAME (), ""); !skewed_name.empty ())
          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_NAME (), skewed_name, mxconst::get_ELEMENT_DISPLAY_OBJECT ());

        Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LAT (), Utils::readAttrib (in_target_navaid.xml_skewdPointNode, mxconst::get_ATTRIB_LAT (), mxconst::get_ZERO ()), mxconst::get_ELEMENT_DISPLAY_OBJECT ());
        Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LONG (), Utils::readAttrib (in_target_navaid.xml_skewdPointNode, mxconst::get_ATTRIB_LONG (), mxconst::get_ZERO ()), mxconst::get_ELEMENT_DISPLAY_OBJECT ());

        Utils::xml_set_attribute_in_node<bool> (xNode, "skewed_position", true, mxconst::get_ELEMENT_DISPLAY_OBJECT ());
      }

    } // end if tag is DISPLAY_OBJECT
  } // end xNode valid

  return true;
}

// -----------------------------------


std::map<int, missionx::NavAidInfo>
RandomEngine::gen_oilrig_targets (missionx::base_thread::thread_state *inoutThreadState, const IXMLNode &in_mapping_root_node, IXMLNode &inout_metadata_node, random_airport_info_struct &inout_shared_navaid, std::string &outErr)
{
  std::map<int, missionx::NavAidInfo> target_navaids;

  // 1. Fetch random oilrig from a database with the starting location
  const std::string STMT_KEY_Q0 = "find_oilrig";
  std::map<int, std::string> q0_columns = { { 0, "distance" }, { 1, "oilrig_icao_id" }, { 2, "oilrig_icao" }, { 3, "oilrig_name" }, { 4, "oilrig_lat" }, { 5, "oilrig_lon" }, { 6, "start_icao_id" }, { 7, "start_icao" }, { 8, "start_ap_name" }, { 9, "start_lat" }, { 10, "start_lon" } };

  // v24025 - There is no reason to override the mapQueries if the "oilrig" query is already in it
  if (!mxUtils::isElementExists (missionx::data_manager::mapQueries, STMT_KEY_Q0))
  {
    missionx::data_manager::mapQueries[STMT_KEY_Q0] = R"(with oilrigs_vu as (
select oilRigVu.icao_id, oilRigVu.icao, oilRigVu.ap_name, oilRigVu.ap_lat, oilRigVu.ap_lon, trunc(oilRigVu.ap_lat) as ap_lat_trunc, trunc(oilRigVu.ap_lon) as ap_lon_trunc
from airports_vu oilRigVu
where oilRigVu.is_oilrig > 0
{1}
),
airports_with_helipads_vu as (
select av.icao_id, av.icao, av.ap_name, av.ap_lat, av.ap_lon, trunc(av.ap_lat) as ap_lat_trunc, trunc(av.ap_lon) as ap_lon_trunc
from airports_vu av
where av.is_oilrig = 0
)
select  mx_calc_distance(ov.ap_lat, ov.ap_lon, awh.ap_lat , awh.ap_lon, 3440) as distance
        , ov.icao_id as oilrig_icao_id, ov.icao as oilrig_icao, ov.ap_name as oilrig_name, ov.ap_lat as oilrig_lat, ov.ap_lon as oilrig_lon
        , awh.icao_id as start_icao_id, awh.icao as start_icao, awh.ap_name as start_ap_name, awh.ap_lat as start_lat, awh.ap_lon as start_lon
from oilrigs_vu ov, airports_with_helipads_vu awh
where 1 = 1
and ov.icao_id != awh.icao_id
and ( awh.ap_lat between ov.ap_lat_trunc - 2 and ov.ap_lat_trunc + 2
      and awh.ap_lon between ov.ap_lon_trunc - 2 and ov.ap_lon_trunc + 2 )
order by RANDOM() limit 1
)";
  }

  std::map<std::string, std::string> row_oilrig_and_start_location;
  Utils::read_external_sql_query_file (missionx::data_manager::mapQueries, mxconst::get_SQLITE_OILRIG_SQLS ()); // v24025


  // v25.08.1 restrict query base on the data_manager::ui_oilrig_globe_part_i value
  const missionx::Point plane = missionx::dataref_manager::getPlanePointLocationThreadSafe ();

  std::map <int, std::string> filter01 = {{1, ""}};
  switch (missionx::data_manager::ui_oilrig_globe_part_i)
  {
    case missionx::PICKED_HALF_GLOBE:
    {
      // Longitude: between -180..0 or 0..180
      if (plane.lon < 0.0)
        filter01[1] = " and oilRigVu.ap_lon between -180.0 and 0.0 and oilRigVu.ap_lat between -90.0 and 90.0 ";
      else
        filter01[1] = " and oilRigVu.ap_lon between 0.0 and 180.0 and oilRigVu.ap_lat between -90.0 and 90.0 ";
    }
      break;
    case missionx::PICKED_QUARTER_GLOBE:
    {
      // Longitude: between -180..0 or 0..180
      if (plane.lon < 0.0)
        filter01[1] = " and oilRigVu.ap_lon between -180.0 and 0.0 ";
      else
        filter01[1] = " and oilRigVu.ap_lon between 0.0 and 180.0 ";

      if (plane.lat >= 0.0)
        filter01[1].append (" and oilRigVu.ap_lat between 0.0 and 90.0 ");
      else
        filter01[1].append (" and oilRigVu.ap_lat between -90.0 and 0.0 ");
    }
      break;
    case missionx::PICKED_LOCAL_REGION_GLOBE:
    case missionx::PICKED_IN_MY_AREA:
    {
      // Longitude: between -180..0 or 0..180    // The values will be defined by the PICKED option
      double LON_BOUND_RADIUS = (missionx::data_manager::ui_oilrig_globe_part_i == PICKED_LOCAL_REGION_GLOBE)? 40.0 : 1;
      double LAT_BOUND_RADIUS = (missionx::data_manager::ui_oilrig_globe_part_i == PICKED_LOCAL_REGION_GLOBE)? 25.0 : 2;
      auto             local_lon_min    = (trunc (plane.lon) - LON_BOUND_RADIUS >= -180.0) ? trunc (plane.lon) - LON_BOUND_RADIUS : trunc (plane.lon) - LON_BOUND_RADIUS + 360.0;
      auto             local_lon_max    = (local_lon_min + LON_BOUND_RADIUS * 2.0 <= 180.0) ? local_lon_min + LON_BOUND_RADIUS * 2.0 : local_lon_min + LON_BOUND_RADIUS * 2.0 - 360.0;

      auto local_lat_min = (trunc (plane.lat) - LAT_BOUND_RADIUS >= -90.0) ? trunc (plane.lat) - LAT_BOUND_RADIUS : trunc (plane.lat) - LAT_BOUND_RADIUS + 180.0;
      auto local_lat_max = (local_lat_min + LAT_BOUND_RADIUS * 2.0 <= 90.0) ? local_lat_min + LAT_BOUND_RADIUS * 2.0 : local_lat_min + LAT_BOUND_RADIUS * 2.0 - 180.0;

      // make sure values are ordered correctly by size
      if (local_lon_min > local_lon_max)
      {
        local_lon_min = local_lon_min + local_lon_max;
        local_lon_max = local_lon_min - local_lon_max;
        local_lon_min = local_lon_min - local_lon_max;
      }
      if (local_lat_min > local_lat_max)
      {
        local_lat_min = local_lat_min + local_lat_max;
        local_lat_max = local_lat_min - local_lat_max;
        local_lat_min = local_lat_min - local_lat_max;
      }

      filter01[1] = fmt::format (" and oilRigVu.ap_lon between {} and {} and oilRigVu.ap_lat between {} and {} ", local_lon_min, local_lon_max, local_lat_min, local_lat_max);
    }
      break;
    default:
      filter01[1] ="";
  }

  missionx::data_manager::mapQueries[STMT_KEY_Q0] = Utils::format ( missionx::data_manager::mapQueries[STMT_KEY_Q0], filter01 );
  // end restrict oilrig region

////////////////// Step 1 ///////////////

  // prepare SQL
  RandomEngine::resultTable_gather_random_airports.clear ();

  #ifndef RELEASE
  Log::logMsgThread (fmt::format("Oilrig Step 1 Query:\n{}", data_manager::mapQueries[STMT_KEY_Q0]));
  #endif
  auto start_time = std::chrono::steady_clock::now ();

  if (data_manager::db_xp_airports.db_is_open_and_ready)
  {
    char *zErrMsg = nullptr;
    RandomEngine::resultTable_gather_random_airports.clear ();
    int rc = sqlite3_exec (data_manager::db_xp_airports.db, data_manager::mapQueries[STMT_KEY_Q0].c_str (), RandomEngine::callback_gather_random_airports_db, nullptr, &zErrMsg);
    if (rc != SQLITE_OK)
    {
      outErr = "[" + std::string (__func__) + "] SQL error: " + std::string (zErrMsg);
      sqlite3_free (zErrMsg);
      return target_navaids;
    }

    auto end_time = std::chrono::steady_clock::now ();
    // elapsed time in milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // elapsed time in seconds (as floating point, with decimals)
    std::chrono::duration<double> sec = end_time - start_time;

    Log::logMsgThread (fmt::format("[{}] Oil Rig information was gathered. Time taken: {}ms/{}s !", __func__, ms.count (), sec.count ()) ); // v25.08.1


    #ifndef RELEASE
    for (auto &[indx, row] : resultTable_gather_random_airports)
    {
      std::string debugOutput_s = "\tseq: " + mxUtils::formatNumber<int> (indx) + ": ";
      for (const auto &colName : q0_columns | std::views::values)
        debugOutput_s += "[" + colName + " = " + row[colName] + "]";

      Log::logMsgThread (debugOutput_s);
    }
    #endif // !RELEASE

    row_oilrig_and_start_location.clear ();
    if (mxUtils::isElementExists (RandomEngine::resultTable_gather_random_airports, 0))
      row_oilrig_and_start_location = RandomEngine::resultTable_gather_random_airports[0];
    else
    {
      outErr = "No Valid information on Oil Rigs and Start location was found.";
      return target_navaids;
    }
  }
  else
  {
    outErr = "[" + std::string (__func__) + "] Database: db_xp_airports is not ready. Aborting !!!";
    return target_navaids;
  }

  ////////////////// Step 2 ///////////////
  // Fetch Navaid information from X-Plane - Double-check what we fetched from the database

  // Get data from X-Plane own Navdata database
  inout_shared_navaid.navAid.init ();

  if (missionx::data_manager::ui_oilrig_globe_part_i == PICKED_IN_MY_AREA)
  {
    inout_shared_navaid.navAid.lat = static_cast<float>(RandomEngine::planeLocation.lat);
    inout_shared_navaid.navAid.lon = static_cast<float>(RandomEngine::planeLocation.lon);
  }
  else
  {
    inout_shared_navaid.navAid.setID (row_oilrig_and_start_location[q0_columns[7]]); // Airport ICAO
    inout_shared_navaid.navAid.lat = mxUtils::stringToNumber<float> (row_oilrig_and_start_location[q0_columns[9]], 8); // Airport Lat
    inout_shared_navaid.navAid.lon = mxUtils::stringToNumber<float> (row_oilrig_and_start_location[q0_columns[10]], 8); // Airport Lon
  }


  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
  {
    outErr = ("[" + std::string (__func__) + "] Start Navaid: " + inout_shared_navaid.navAid.getID () + " Failed to find Airport using query navaid. Notify developer.");
    return target_navaids;
  }
  inout_shared_navaid.navAid.synchToPoint ();
  target_navaids[0] = NavAidInfo (inout_shared_navaid.navAid); // Store the briefer starting location
  std::string err;
  RandomEngine::filterAndPickRampBasedOnPlaneType (target_navaids[0], err, mxFilterRampType::start_ramp);
  target_navaids[0].fpln_navaid_was_already_prepared = true;

  inout_shared_navaid.navAid.init ();
  inout_shared_navaid.navAid.setID (row_oilrig_and_start_location[q0_columns[2]]); // Oil Rig ICAO
  inout_shared_navaid.navAid.lat = mxUtils::stringToNumber<float> (row_oilrig_and_start_location[q0_columns[4]], 8); // Oil Rig Lat
  inout_shared_navaid.navAid.lon = mxUtils::stringToNumber<float> (row_oilrig_and_start_location[q0_columns[5]], 8); // Oil Rig Lon
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
  {
    outErr = ("[" + std::string (__func__) + "] Oil Rig Navaid: " + inout_shared_navaid.navAid.getID () + " Failed to find Oil Rig using query navaid. Notify developer.");
    target_navaids.clear ();
    return target_navaids;
  }
  inout_shared_navaid.navAid.synchToPoint ();

  // store the second navaid
  target_navaids[1] = NavAidInfo (inout_shared_navaid.navAid); // v25.04.2 using the new constructor to initialize the NavAid

  ////////////////////// Step 3 //////////////////
  /// Prepare the landing area. Currently always icao
  ///////////////////////////////////////////////

  float min_dist_nm = 3.0;
  float max_dist_nm = 50.0;

  // Determine the last node name to read from MAPPING based on sub category, decided in prepare_blank_template_with_flight_legs_based_on_ui() function
  const auto sub_category = Utils::readAttrib (inout_metadata_node, mxconst::get_ATTRIB_CATEGORY (), "med");
  // will construct: "leg_{oilrig_xxx}_end_helos"
  std::string last_node_name = fmt::format("leg_{}_end_helos", sub_category);

  const auto last_leg_oilrig_node = in_mapping_root_node.getChildNode (last_node_name.c_str ());
  if ( !last_leg_oilrig_node.isEmpty () )
  {
    auto data  = parse_expected_location (last_leg_oilrig_node, "oilrig", true);
    if (!data.error.empty ())
    {
      Log::logMsgErr (data.error, true);
      target_navaids.clear ();
      return target_navaids;
    }
    // if no errors returned, check and use nm_between values
    if (data.nm_between_min >=0 && data.nm_between_max >=0 )
    {
      min_dist_nm = data.nm_between_min;
      max_dist_nm = data.nm_between_max;
    }
  }

  missionx::mx_base_node leg_property_node;
  leg_property_node.setBoolProperty (mxconst::get_PROP_IS_LAST_FLIGHT_LEG(), true);
  target_navaids[2] = get_random_airport_from_db (target_navaids[1].p, min_dist_nm, max_dist_nm, -1, leg_property_node, getPlaneType ());
  target_navaids[2].fpln_xml_osm_q_or_raw_tmpl_node = last_leg_oilrig_node.deepCopy (); // v25.09.2

  // call synchToPoint for all targets
  for (auto &t : std::views::values (target_navaids))
    t.synchToPoint (true);

  return target_navaids;
}


// -----------------------------------


missionx::structs::strct_expected_location_data
RandomEngine::parse_expected_location (const IXMLNode &in_xml_leg_from_template, const std::string &custom_error_message, const bool is_last_leg)
{
  missionx::structs::strct_expected_location_data data;

  //// PARSE EXPECTED LOCATION  ////
  IXMLNode xExpectedLocation = in_xml_leg_from_template.getChildNode (mxconst::get_ELEMENT_EXPECTED_LOCATION ().c_str ()).deepCopy (); // xLegFromTemplate.getChildNode (mxconst::get_ELEMENT_EXPECTED_LOCATION ().c_str ());
  if (xExpectedLocation.isEmpty ())
  {
    data.error = fmt::format("[{}] Failed to find: {}, while parsing {}. Please fix template.", __func__, mxconst::get_ELEMENT_EXPECTED_LOCATION (), custom_error_message);
    return data;
  }

  RandomEngine::flag_force_template_distances_b = Utils::readBoolAttrib (xExpectedLocation, mxconst::get_ATTRIB_FORCE_TEMPLATE_DISTANCES_B (), false); // will be used in get_target() function to disable the "expected distance setup option".

  // if (location_type_s.empty())
  if (Utils::readAttrib (xExpectedLocation, mxconst::get_ATTRIB_LOCATION_TYPE (), "").empty ())
  {
    data.error = fmt::format ("[{}] {} node is empty.", __func__, mxconst::get_ELEMENT_EXPECTED_LOCATION ());
    return data;
  }

  data.location_type       = Utils::readAttrib (xExpectedLocation, mxconst::get_ATTRIB_LOCATION_TYPE (), "");
  data.location_properties_s = Utils::readAttrib (xExpectedLocation, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), mxconst::get_ATTRIB_LOCATION_VALUE (),  "", true);

  // location properties format can be: "{number}|{ramp type}|{min-max},..."
  // mapLocationValueOptions: {name},{value}.
  std::map<int, std::string> mapLocationValueOptions; // v3.0.221.7 will hold the complex options used by "|".
  std::string                location_value_min_max_distance_s, location_value_tag_name_s, location_value_poi_s; // v3.0.221.7 @Daikan used in Random airport pick. "location_value_tag_name_s" will be used to hold element name to search in template.

  ///////////// CHECK if LOCATION TYPE needs to be Randomized ////////////////
  // support for multi-location vecTypeValues type to pick
  if (data.location_type.find (mxconst::get_COMMA_DELIMITER ()) != std::string::npos)
  {
    std::vector<std::string> vecTypes      = mxUtils::split_v2 (data.location_type, mxconst::get_COMMA_DELIMITER ());
    std::vector<std::string> vecTypesProperties = mxUtils::split_v2 (data.location_properties_s, mxconst::get_COMMA_DELIMITER ());
    int                      picked        = 0;

    int       nTypes       = static_cast<int> (vecTypes.size ());
    const int nTypesProperties = static_cast<int> (vecTypesProperties.size ());

    Log::logDebugBO ("[DEBUG pick <leg> type & value] vecTypes: " + Utils::formatNumber<size_t> (vecTypes.size ()) + ", values:" + Utils::formatNumber<size_t> (vecTypesProperties.size ()), true);

    if (nTypes == 0)
    {
      data.error = "Found a location type with wrong definition. Check type properties. aborting !!!";
      return data;
    }

    if (nTypes == 1)
    {
      data.location_type = vecTypes.at (0);
      picked        = 0; // meaning first choice
    }
    else // random pick type
    {
      picked = Utils::getRandomIntNumber (0, nTypes - 1);
      if (picked > nTypes) // just in case
        picked = nTypes - 1;

      data.location_type = vecTypes.at (picked);
    }

    //// pick the property with the same pick index. picked can't be bigger than the number of types
    if (nTypesProperties >= nTypes || (picked < (nTypesProperties - 1)))
      data.location_properties_s = vecTypesProperties.at (picked);
    else if (nTypes >= 1 && nTypesProperties == 1) // if we have few Types but only 1 location_value_nm_s, then it is shared between all of them
      data.location_properties_s = vecTypesProperties.front ();
    else
      data.location_properties_s.clear ();

    if (data.location_properties_s == "_") // if special character that represents empty
      data.location_properties_s.clear ();
  }

  // The Random Engine needs the "location_properties_s" and "type" to find suitable target.
  // we will prepare all data that is necessary : string or container, and the code will have to pick the correct option from the "data" struct.
  data.flight_leg_type_hover_land_or_start = mxUtils::stringToLower (Utils::readAttrib (in_xml_leg_from_template, mxconst::get_ATTRIB_TEMPLATE (), EMPTY_STRING));

  if ((mxconst::get_FL_TEMPLATE_VAL_START () == data.flight_leg_type_hover_land_or_start) || (data.location_type == mxconst::get_FL_TEMPLATE_VAL_START ())) // v3.0.221.15 consolidate if logic to one  // v3.0.221.7
  {
    data.flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_START ();
    data.location_type                       = mxconst::get_FL_TEMPLATE_VAL_START ();
    data.location_properties_s               = mxconst::get_FL_TEMPLATE_VAL_START ();
    data.mapLocationSplitValues.clear ();
    data.vecLocationValueSplit_vec.clear ();
  }
  ////////// Check if has special instructions like: "nm=20|ramp=H|nm_between=10-20|tag={some name}"
  else if (!data.location_properties_s.empty ())
  {
    //// v3.0.221.7 replace old logic with new more readable one
    // split between numbers and characters
    data.vecLocationValueSplit_vec = mxUtils::split_v2 (data.location_properties_s, mxconst::get_PIPE_DELIMITER ()); // "|"

    for (const auto &v : data.vecLocationValueSplit_vec)
    {
      std::vector<std::string> vecSplit = mxUtils::split_v2 (v, "=");
      if (auto size_i = vecSplit.size ()
        ; size_i == 1) // NO backwards compatibility.
      {
        std::string attribName = Utils::stringToLower (vecSplit.at (0));
        Log::logMsgErr (fmt::format ("[{}] Found location Property without explicit formating: {}={}. Skipping this directive.", __func__, attribName, "{missing value}" ) , true);
      }
      else if (size_i > 1)
      {
        std::string        attribName  = Utils::stringToLower (vecSplit.at (0));
        const std::string &attribValue = vecSplit.at (1);
        Utils::addElementToMap (data.mapLocationSplitValues, attribName, attribValue);
      }
      else
        data.location_properties_s.clear ();
    } // end loop over split location_properties

    data.location_properties_s.clear ();

    // prepare local variables according to the split information
    const std::string local_location_value_min_max_distance_s = mxUtils::getValueFromElement (data.mapLocationSplitValues, std::string ("nm_between"), std::string (""));
    if (!local_location_value_min_max_distance_s.empty ()) // min-max
    {
      const std::vector<double> vecMinMax = Utils::splitStringToNumbers<double> (local_location_value_min_max_distance_s, "-, ");
      for (size_t i1 = 0; i1 < vecMinMax.size (); ++i1)
      {
        switch (i1)
        {
          case 0:
            data.nm_between_min = static_cast<float>(vecMinMax.at(i1));
            data.mapLocationSplitValues["min_distance_nm"] = fmt::format("{:.2f}", vecMinMax.at(i1) );
            break;
          case 1:
            data.nm_between_max = static_cast<float>(vecMinMax.at(i1));
            data.mapLocationSplitValues["max_distance_nm"] = fmt::format("{:.2f}", vecMinMax.at(i1) );
            break;
          default:
            break;
        } // end switch
      }

      // Validate and fix if min > max
      if (data.nm_between_min >=0 && data.nm_between_max >=0 )
      {
        if (data.nm_between_min > data.nm_between_max)
          std::swap(data.nm_between_min, data.nm_between_max);
      }
    } // end "nm_between"

    // prepare local variables according to the split information
    if (Utils::isElementExists (data.mapLocationSplitValues, "nm")) // represent distance in nm
      data.location_properties_s = data.mapLocationSplitValues["nm"];

    // replace "_" with empty string
    if (data.location_properties_s == "_") // if special character that represent empty
      data.location_properties_s.clear ();

  }

Log::logDebugBO ("[DEBUG pick template <leg> type] type picked: " + data.location_type, true);
Log::logDebugBO ("[DEBUG random location info] location_value_nm_s=" + data.location_properties_s, true);

  return data;
}


// -----------------------------------


std::vector<int>
RandomEngine::gen_shuffled_q_from_osm_subject_node (missionx::base_thread::thread_state *inoutThreadState, const IXMLNode &in_root_node, const std::vector<missionx::structs::strct_osm_query> &vec_osm_queries, IXMLNode &out_main_subject_node, missionx::structs::strct_osm_query &out_analyzed_query)
{

  // Shuffle the vector of OSM Analysed Count Queries
  const std::vector<int> vec_shuffle_analyzed_osm_queries = Utils::getShuffledIndexVector (static_cast<int> (vec_osm_queries.size ()) );

  // 3. Loop over the shuffled vector until we find the node, or we reach the end of the vector
  bool flag_successfully_picked_osm_target = false;
  for (auto &pickedIndex : vec_shuffle_analyzed_osm_queries)
  {
    out_analyzed_query = vec_osm_queries.at (pickedIndex);
    // Step 1: Pick subject node with queries
    // First check if there is a subject with "sub query elements".
    Log::logMsgThread (fmt::format ("Picked subject: {}. Will random pick one of the queries related to it.\n", out_analyzed_query.id));
    out_main_subject_node = in_root_node.getChildNode (out_analyzed_query.id.c_str ());
    if (out_main_subject_node.isEmpty ())
    {
      Log::logMsgThread (fmt::format ("[Error] <subject> section missing: {}\n", out_analyzed_query.id));
      continue;
    }

    const int n_queries_in_subject_node = out_main_subject_node.nChildNode ("q");
    if (n_queries_in_subject_node == 0)
    {
      Log::logMsgThread (fmt::format ("[Error] Missing <subject> osm query nodes <q> for: {}\n", out_analyzed_query.id));
      continue;
    }
    // If we reached this line then we have a "subject element" with queries to pick from.


    // Step 2: Pick a random region/bbox and call Overpass
    std::vector<int> vec_shuffle_tags_nodes = Utils::getShuffledIndexVector (out_analyzed_query.xml_q_tags_header_node.nChildNode ("tags"));
    // loop over the shuffled tags node, that also represent the "region" to search.
    for (const auto &rnd_tag : vec_shuffle_tags_nodes)
    {
      auto tag = out_analyzed_query.xml_q_tags_header_node.getChildNode ("tags", rnd_tag);
      Utils::xml_print_node (tag, true); // debug

      out_analyzed_query.q_short_bbox_fmt = Utils::readAttrib (tag, "cached_bbox", "", "", true);
      out_analyzed_query.q_bbox           = Utils::readAttrib (tag, "bbox", "", "", true);
      if (out_analyzed_query.q_bbox.empty ())
        continue;

      return Utils::getShuffledIndexVector (n_queries_in_subject_node);
      
    }
  } // end loop over picked index

  return {}; // return empty vector std::vector<int>
  // end get_osm_topic_subject_and_prep_shuffled_q
}

// -----------------------------------

std::map<int, missionx::NavAidInfo>
RandomEngine::gen_targets_using_osm_queries_from_a_thread (missionx::base_thread::thread_state *inoutThreadState, const IXMLNode &in_root_node, missionx::structs::strct_osm_query &inout_osm_query, random_airport_info_struct &inout_shared_navaid)
{
  constexpr int max_loops_i                   = 3;
  bool          flag_all_osm_queries_are_done = false;
  int           seq                           = 1;

  std::map<int, missionx::NavAidInfo> map_navaids;

  if (inoutThreadState == nullptr)
    return map_navaids;

  // add assert
  inoutThreadState->flagIsActive = true;

  // Drill down until all "next_tag" attributes in <q> nodes would be satisfied or no more "next_tag"s to be had.
  while (!flag_all_osm_queries_are_done && seq < max_loops_i && !inoutThreadState->flagAbortThread)
  {
    // init the query object
    inout_osm_query.q_text                 = Utils::xml_get_text_or_cdata_text (inout_osm_query.xml_query_node_to_search_a_new_target, "");
    inout_osm_query.id                     = Utils::readAttrib (inout_osm_query.xml_query_node_to_search_a_new_target, "id", "");
    inout_osm_query.xml_target_way_element = IXMLNode::emptyIXMLNode;
    inout_osm_query.xml_target_nd_node     = IXMLNode::emptyIXMLNode;

    if (inout_osm_query.q_text.empty ())
    {
      Log::logMsgThread (fmt::format("[{}] Could not initialize 'q_text', exiting function.", __func__) );

      // exit function
      flag_all_osm_queries_are_done = true;
      seq = max_loops_i;
      continue;
    }

    // ------------------
    // Call CURL / Cache
    // ------------------
    missionx::data_manager::fetch_ways_and_target_node_from_overpass_thread (inoutThreadState, nullptr, &inout_osm_query);

    if (!inout_osm_query.xml_target_nd_node.isEmpty () && !inout_osm_query.xml_target_way_element.isEmpty ())
    {
      const auto way_name = Utils::xml_get_attrib_value_based_on_other_attrib_presence (inout_osm_query.xml_target_way_element, "tag", "k", "name", "v", "");
      const auto way_name_en = Utils::xml_get_attrib_value_based_on_other_attrib_presence (inout_osm_query.xml_target_way_element, "tag", "k", "name:en", "v", "");

      #ifndef RELEASE
      Log::logMsgThread (fmt::format( "--> Target way node:\n{}\n", Utils::xml_get_node_content_as_text (inout_osm_query.xml_target_way_element) ) );
      Log::logMsgThread (fmt::format( "--> Target nd node:\n{}\n", Utils::xml_get_node_content_as_text (inout_osm_query.xml_target_nd_node) ) );
      #endif


      // store information based on the fetched "<way>" and "<nd>" elements.
      missionx::NavAidInfo target_navaid;

      // navaid_target.setName (mxUtils::sanitize_text (way_name));
      target_navaid.setName ((way_name_en.empty ()) ? way_name : way_name_en);
      target_navaid.fpln_seq                        = seq;
      target_navaid.lat                             = Utils::readNodeNumericAttrib<float> (inout_osm_query.xml_target_nd_node, mxconst::get_ATTRIB_LAT_OSM (), 0.0f);
      target_navaid.lon                             = Utils::readNodeNumericAttrib<float> (inout_osm_query.xml_target_nd_node, mxconst::get_ATTRIB_LONG_OSM (), 0.0f);
      target_navaid.fpln_wp_type                    = Utils::readAttrib (inout_osm_query.xml_query_node_to_search_a_new_target, "wp_type", "");
      target_navaid.fpln_xml_osm_q_or_raw_tmpl_node = inout_osm_query.xml_query_node_to_search_a_new_target.deepCopy ();
      target_navaid.fpln_xml_way_node               = inout_osm_query.xml_target_way_element.deepCopy ();

      // v25.06.1 clear the <header> node. This is the root node of the current < q > sub-node
      Utils::xml_delete_all_subnodes (inout_osm_query.xml_q_tags_header_node, "", true);
      target_navaid.fpln_xml_q_tag_header_node = inout_osm_query.xml_q_tags_header_node.deepCopy ();

      // Validate lat/lon or exit
      if (target_navaid.lat * target_navaid.lon == 0)
      {
        target_navaid.init ();
        if (seq == 1) // if it is the first iteration, then abort.
        {
          flag_all_osm_queries_are_done = true;
          map_navaids.clear ();
          return map_navaids;
        }
      }

      // search the nearest navaid if not hover_land
      if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND ())
      {
        // Get data from X-Plane own Navdata database
        inout_shared_navaid.navAid.init ();
        missionx::data_manager::fetch_nearest_osm_navaid_from_sqlite ( &target_navaid, &inout_shared_navaid.navAid );
        inout_shared_navaid.navAid.synchToPoint ();
        target_navaid.synchToPoint ();

        #ifndef RELEASE
        Log::logMsgThread (fmt::format ("[{}] After calling fetch_nearest_osm_navaid_from_sqlite()", __func__) );
        #endif


        if (const auto distance = Point::calcDistanceBetween2Points (target_navaid.p, inout_shared_navaid.navAid.p, missionx::mx_units_of_measure::meter)
          ; distance < 1000 ) // if the distance is less than 1km.
        {
          if (target_navaid.getID ().empty ())
            target_navaid.setID (inout_shared_navaid.navAid.getID ());
          if ( ! inout_shared_navaid.navAid.getName ().empty () ) // copy x-plane navaid name over the osm one
            target_navaid.setName (inout_shared_navaid.navAid.getName ());

          target_navaid.lat = inout_shared_navaid.navAid.lat;
          target_navaid.lon = inout_shared_navaid.navAid.lon;
        }
      } // end if LAND

      // Store Target
      target_navaid.synchToPoint (true); // force init_desc
      Utils::addElementToMap (map_navaids, seq, target_navaid);

      // Construct other Navaids if we have any
      #ifndef RELEASE
      Log::logMsgThread (fmt::format ("[{}]: Do we have 'next_tag' ?", __func__) );
      #endif

      // Search NEXT TARGET
      if (std::string next_tag = Utils::readAttrib (inout_osm_query.xml_query_node_to_search_a_new_target, "next_tag", "");
        !next_tag.empty ())
      {
        std::vector<std::string> vec_split_next_tag;
        auto                     vec_shuffled_next_tag = Utils::splitStringAndGetShuffledIndexVector (next_tag, ",", vec_split_next_tag);
        #ifndef RELEASE
        Log::logMsgThread ("Display shuffled Next Tag:");
        for (const auto &val : vec_shuffled_next_tag)
          Log::logMsgThread (fmt::format ("[{}]: {}", val, vec_split_next_tag.at (val)));

        Log::logMsgThread ("<--- End Display shuffled Next Tag ---");
        #endif

        // Fetch the next target, SUBJECT "id" node, based on the "next_tag" attribute value.
        for (size_t counter = 0; const auto &v_index : vec_shuffled_next_tag)
        {
          assert ( static_cast<int>(vec_shuffled_next_tag.size ()) > v_index && fmt::format ("[{}] Shuffled index is out of vector bounds. Split vector size: {}", __func__, vec_split_next_tag.size ()).c_str ());
          counter++;
          const auto &picked_next_tag = vec_split_next_tag.at (v_index);

          // Get the next Subject "id" and validate it
          inout_osm_query.xml_q_tags_header_node = in_root_node.getChildNode (picked_next_tag.c_str ()).deepCopy ();
          if (inout_osm_query.xml_q_tags_header_node.isEmpty () && counter < vec_shuffled_next_tag.size ())
            continue;

          if (inout_osm_query.xml_q_tags_header_node.isEmpty () )
          {
            flag_all_osm_queries_are_done = true;
            seq = max_loops_i;
            break;
          }

          auto vec_shuffle_q                                = Utils::getShuffledIndexVector (inout_osm_query.xml_q_tags_header_node.nChildNode ("q"));
          inout_osm_query.xml_query_node_to_search_a_new_target = inout_osm_query.xml_q_tags_header_node.getChildNode ("q", vec_shuffle_q.front ());
          #ifndef RELEASE
          Log::logMsgThread (fmt::format("[{}] Found next target.\nTag:<{}>\nQuery: {}\n<-- end next target.\n\n", __func__, picked_next_tag, Utils::xml_get_node_content_as_text (inout_osm_query.xml_query_node_to_search_a_new_target) ) ); // debug
          #endif


          break; // skip loop over the shuffled queries vector and return to the "while" loop to fetch next NavAid

        } // end loop over shuffled queries vector

      } // End if there is next_tag
      else
      {
        flag_all_osm_queries_are_done = true;
        // break; // Should exit the "while" loop
      }

    } // end if inout_osm_query is valid
    else
    {
      // sleep 2 seconds
      std::this_thread::sleep_for (std::chrono::seconds (2)); // wait for 2 seconds before sending a new request

      if (inout_osm_query.total_way_count < 1 && inout_osm_query.flag_data_respond_was_valid ) // if no <way> was found, and overpass request was valid
      {
        seq = max_loops_i;
        break;
      }
    }

    ++seq;

  } // end while loop

  // check [abort]
  if (inoutThreadState->flagAbortThread)
    map_navaids.clear ();

  return map_navaids;

}

// -----------------------------------

std::string
RandomEngine::gen_leg_name (int *seq, const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo &inTargetNavAid)
{

    auto seq1      = (seq) ? (*seq) : -1;
    auto nav_name = mxUtils::trim (inTargetNavAid.getName () );

    if (seq)
      (*seq)++;

    return fmt::format ("{}_{}_{}{}", prefix_name, seq1, nav_name, (nav_name.empty ()) ? prefix_name : std::string ("-") + postfix_name);
}

// -----------------------------------

mx_return
RandomEngine::prepare_medevac_surprise_me (IXMLNode &inRootTemplate, const IXMLNode &inoutMetaNode, const missionx::Point& in_plane_location)
{
  // 1. Analyze the OSM data around the plane based on the "osm_gen.xml" file.
  //    Pick only up to four of the analyzed categories.
  // 2. Randomly pick one of the analyzed osm categories.
  // 3. Pick one of the "subject" subcategories queries and fetch its "ways" data.
  // 4. Pick one random "nd" node as your target.

  bool flag_one_of_the_targets_above_water {false};
  missionx::mx_return out_mx_return;
  const std::string osm_gen_xml_filename = fmt::format ("{}/missionx/{}", Utils::getRelativePluginsPath (), Utils::getNodeText_type_6 (missionx::system_actions::pluginSetupOptions.node, mxconst::get_PROP_OSM_GEN_FILE (), mxconst::DEFAULT_OSM_GEN_FILE.data ()));
  const std::string osm_gen_custom_xml_filename = fmt::format ("{}/missionx/{}", Utils::getRelativePluginsPath (), mxconst::DEFAULT_CUSTOM_OSM_GEN_FILE.data () );
  const std::string cache_folder = fmt::format ("{}/{}", Utils::getRelativePluginsPath (), "missionx/db/cache"); // cache folder location should be in missionx/db/cache
  missionx::data_manager::check_cache_folder (cache_folder); // will check if folder exists and if not will create it.

  ////////////////// Step 1 - Call OSM Analyze ///////////////
  IXMLNode osm_gen_xml_root_node = IXMLNode::emptyIXMLNode;

  // use "custom_osm_gen.xml" or original "osm_gen.xml" file.
  const std::string xml_filename = (mxUtils::check_file_exists (osm_gen_custom_xml_filename))? osm_gen_custom_xml_filename : osm_gen_xml_filename;

  // check [abort]
  if (RandomEngine::threadState.flagAbortThread)
  {
    out_mx_return.addErrMsg ("User asked to abort.", true);
    return out_mx_return;
  }

  // ----------------------
  // OSM ANALYZE
  // ----------------------
  const std::vector<missionx::structs::strct_osm_query> vec_osm_queries = gen_osm_analyse (out_mx_return, xml_filename, cache_folder, in_plane_location.lat, in_plane_location.lon, osm_gen_xml_root_node);

  // check [abort]
  if (RandomEngine::threadState.flagAbortThread)
  {
    out_mx_return.addErrMsg ("User asked to abort.", true);
    return out_mx_return;
  }

  // validate there are results or fail the function.
  if (vec_osm_queries.empty ())
  {
    out_mx_return.addErrMsg ("No valid data was found using the webosm. Aborting.", true);
    return out_mx_return;
  }

  //// Shuffle the vector of OSM Analyzed Count Queries and get a target
  std::map<int, NavAidInfo> osm_na_targets;

  missionx::structs::strct_osm_query osm_query; // initialized in "get_osm_topic_subject_and_prep_shuffled_q()" function.
  osm_query.cache_folder = cache_folder; // INITIALIZING THE CACHE FOLDER

  IXMLNode         main_subject_node           = IXMLNode::emptyIXMLNode; // initialized in "gen_shuffled_q_from_osm_subject_node()" function.
  std::vector<int> vec_shuffle_subject_queries = missionx::RandomEngine::gen_shuffled_q_from_osm_subject_node (&RandomEngine::threadState, osm_gen_xml_root_node, vec_osm_queries, main_subject_node, osm_query);
  for (const auto &randomNumber : vec_shuffle_subject_queries)
  {
    // check [abort]
    if (RandomEngine::threadState.flagAbortThread)
    {
      out_mx_return.addErrMsg ("User asked to abort.", true);
      return out_mx_return;
    }


    // Pick the subject query
    osm_query.xml_q_tags_header_node = main_subject_node.deepCopy ();
    osm_query.xml_query_node_to_search_a_new_target = main_subject_node.getChildNode ("q", randomNumber);

    // ----------------------
    // CALL GENERIC OVERPASS
    // ----------------------
    osm_na_targets = gen_targets_using_osm_queries_from_a_thread (&RandomEngine::threadState, osm_gen_xml_root_node, osm_query, RandomEngine::shared_navaid_info );
    if (!osm_na_targets.empty ())
      break; // Exit loop
  } // end loop over shuffled "q" nodes


  // check [abort]
  if (RandomEngine::threadState.flagAbortThread)
  {
    out_mx_return.addErrMsg ("User asked to abort.", true);
    return out_mx_return;
  }

  // fail mission build if no targets were found
  if (osm_na_targets.empty ())
  {
    out_mx_return.addErrMsg ("No valid targets were found. Aborting.", true);
    return out_mx_return;

  }


  //---------------------------------------
  //--- Water Bodies / Slope / Leg Name ---
  //---------------------------------------
  for (auto &target_navaid : osm_na_targets | std::views::values)
  {
    target_navaid.fpln_is_wet = get_is_wet_at_point (target_navaid);

    // store wet state if the "flag value" is not true, yet.
    if (!flag_one_of_the_targets_above_water)
      flag_one_of_the_targets_above_water = target_navaid.fpln_is_wet;

    // store slope at the target location
    target_navaid.fpln_slope = get_slope_at_point (target_navaid);

    target_navaid.fpln_leg_name = gen_leg_name ( &this->seq_waypoints, mxconst::get_GPS_WP (),"leg", target_navaid );
  }

  // ----------------------
  // -- Add <briefer> node - Start Location
  // ----------------------
  NavAidInfo start_navaid;
  start_navaid.p  = in_plane_location;
  start_navaid.syncPointToNav ();

  osm_na_targets[0] = gen_briefer_node (start_navaid, RandomEngine::shared_navaid_info, flag_one_of_the_targets_above_water);



  // ----------------------
  // -- Read and set <mission_info>
  // ----------------------
  IXMLNode xBrieferInfo;
  if (missionx::RandomEngine::working_tempFile_ptr != nullptr)
  {
    auto template_image_file_name = (missionx::RandomEngine::working_tempFile_ptr->getTemplateImageFileName ().empty ())? mxconst::get_DEFAULT_RANDOM_IMAGE_FILE() : missionx::RandomEngine::working_tempFile_ptr->getTemplateImageFileName ();
    auto template_name            = missionx::RandomEngine::working_tempFile_ptr->fullFilePath;
    auto template_folder_name     = missionx::RandomEngine::working_tempFile_ptr->missionFolderName;

    xBrieferInfo = gen_mission_info_node (inRootTemplate, template_name, template_image_file_name, template_folder_name );
  }
  else 
    xBrieferInfo = gen_mission_info_node (inRootTemplate, "", "", "");



  #ifndef RELEASE
  Log::logMsgThread ( fmt::format("--- osm_targets {} ----------------------------->>>", __func__ ) );
  for (auto &[k, na] : osm_na_targets)
    Log::logMsgThread ( fmt::format ("[{}] {}. \tpos: [{}]", k, na.get_loc_desc (), na.get_latLon () ) );
  Log::logMsgThread ( fmt::format("<<<--- End {} -------------------------------\n\n", __func__ ) );
  #endif


  // ------------------------------------------------------------------
  // Construct all mission <leg> nodes
  // ------------------------------------------------------------------
  if (!osm_na_targets.empty())
  {
    // loop over all targets
    for (auto &[indx, target_navaid] : osm_na_targets)
    {
      if (indx == 0) // skip briefer
        continue;

      // decide the "landing type"
      if (target_navaid.fpln_wp_type.empty ())
      {
        if (target_navaid.fpln_seq % 2 == 0)
          target_navaid.fpln_wp_type = mxconst::get_FL_TEMPLATE_VAL_LAND();
        else
          target_navaid.fpln_wp_type = mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ();
      }

      // is last flight leg ?
      target_navaid.fpln_is_last_flight_leg = ! ( mxUtils::isElementExists (osm_na_targets, indx + 1) );

      // We are basically constructing the mission from the middle waypoint and then need to add the start and end coordinates.
      // Write dedicated functions to only prepare the specific "needed" node.
      // Example: prepare trigger (seq, name, radius)
      IXMLNode xTriggerTargetHover = IXMLNode::emptyIXMLNode;
      IXMLNode xTriggerTargetLand = IXMLNode::emptyIXMLNode;
      IXMLNode xTaskTargetHover = IXMLNode::emptyIXMLNode;
      IXMLNode xTaskTargetLand  = IXMLNode::emptyIXMLNode;


      // ----------------------------------------------------------
      // Prepare the properties for the target triggers and tasks
      // ----------------------------------------------------------


      // Land properties
      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_trig_land  =
        {
        { "conditions", "plane_on_ground", "true" },
        { "radius", "length_mt", (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())? mxconst::DEFAULT_LAND_OR_INV_RADIUS_MT.data() : mxconst::DEFAULT_HOVER_RADIUS_MT.data () }
        };

      // Hover trigger properties
      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_trig_hover = {
        { "conditions", "plane_on_ground", "false" },
        { "radius", "length_mt", mxconst::DEFAULT_HOVER_RADIUS_MT.data () },
        { "elevation_volume", "elev_lower_upper_ft", fmt::format("---{}", mxconst::DEFAULT_HOVER_VOL_HEIGHT_FOR_OSM_FT ) }, // example: "---328"
      };
      

      // Land trigger creation
      xTriggerTargetLand = RandomEngine::gen_trigger_node (this->seq_triggers, "trig", "land", target_navaid, lsAttrib_trig_land, nullptr);
      // Hover trigger creation
      if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
        xTriggerTargetHover = RandomEngine::gen_trigger_node (this->seq_triggers, "trig", "hover", target_navaid, lsAttrib_trig_hover, nullptr);


      // TASK setup will initialize after trigger setup since we need values from the trigger node
      // Land + Hover Task Properties
      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_land_task_target = {
        { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_BASE_ON_TRIGGER (), Utils::readAttrib (xTriggerTargetLand, mxconst::get_ATTRIB_NAME (), "") },
        { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_MANDATORY (), "true" }, { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC (), "20" },
      };
      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_hover_task_target = {
        { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_BASE_ON_TRIGGER (), Utils::readAttrib (xTriggerTargetHover, mxconst::get_ATTRIB_NAME (), "") },
        { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_MANDATORY (), "true" },
        { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC (), "20" },
      };

      // Land Task
      xTaskTargetLand    = RandomEngine::gen_task_node (this->seq_tasks, "task", "land", target_navaid, lsAttrib_land_task_target, nullptr);

      // Hover Task
      if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
        xTaskTargetHover = RandomEngine::gen_task_node (this->seq_tasks, "task", "hover", target_navaid, lsAttrib_hover_task_target, nullptr);

      // END Handling LAND + HOVER Triggers and Tasks


      // -------------------------------------------------
      // Add inventory - always create an inventory node

      // prepare attributes to modify
      const auto inv_radius = (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())? mxconst::DEFAULT_LAND_OR_INV_RADIUS_MT.data () : mxconst::DEFAULT_HOVER_RADIUS_MT.data ();
      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_inv = {
        { mxconst::get_ELEMENT_POINT (), mxconst::get_ATTRIB_LAT (), target_navaid.getLat () },
        { mxconst::get_ELEMENT_POINT (), mxconst::get_ATTRIB_LONG (), target_navaid.getLon () },
        { mxconst::get_ELEMENT_RADIUS (), mxconst::get_ATTRIB_LENGTH_MT (), inv_radius },
        { mxconst::get_ELEMENT_INVENTORY (), mxconst::get_ATTRIB_INHIBIT_MXPAD_B (), "true" }, // inhibit mx-pad toggle when entering inventory area and we are airborne
      };

      target_navaid.fpln_xml_inv_node = gen_inventory_node (indx, target_navaid, map_osm_inventory_track, lsAttrib_inv);
      // todo: move script creation after all targets were generated
      if (!target_navaid.fpln_xml_inv_node.isEmpty ())
      {
        // create scripts and attach them into the <inventory> as a sub element.
        RandomEngine::gen_target_inventory_scripts (target_navaid, map_osm_inventory_track);
      }

      // ----------------
      // Create Objective
      IXMLNode xTargetObjective = RandomEngine::gen_objective_node (this->seq_objectives, "obj", "target");


      // -------------------------------------------------
      // POST Actions to update the Trigger information.
      // We need the names and values that were not available during node creation.
      // -------------------------------------------------
      const auto task_land_name  = Utils::readAttrib (xTaskTargetLand, mxconst::get_ATTRIB_NAME (), "");
      const auto task_hover_name = Utils::readAttrib (xTaskTargetHover, mxconst::get_ATTRIB_NAME (), "");

      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_outcome_target_trig =
        {
        {mxconst::get_ELEMENT_OUTCOME (), mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS (), fmt::format ("{}{}", task_land_name, (task_hover_name.empty ()? "" : "," + task_hover_name) ) },
        };

      // Set Triggers post attributes
      if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
        Utils::xml_search_and_set_attributes_in_node (xTriggerTargetHover, lsAttrib_outcome_target_trig);
      Utils::xml_search_and_set_attributes_in_node (xTriggerTargetLand, lsAttrib_outcome_target_trig);

      // Link task to objective
      if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
        xTargetObjective.addChild (xTaskTargetHover);
      xTargetObjective.addChild (xTaskTargetLand);

      //-------------------------
      // <leg> element creation
      //-------------------------
      const auto obj_target_name = Utils::readAttrib (xTargetObjective, mxconst::get_ATTRIB_NAME (), "");
      std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_wp_target =
      {
        { mxconst::get_ELEMENT_LEG (), mxconst::get_ATTRIB_TITLE (), target_navaid.gen_locDesc_short () },
        { mxconst::get_ELEMENT_LINK_TO_OBJECTIVE (), mxconst::get_ATTRIB_NAME (), obj_target_name },
      };
      target_navaid.fpln_xml_target_leg_node = RandomEngine::gen_leg_node ( mxconst::get_GPS_WP (), "leg", &target_navaid, &lsAttrib_wp_target);

      // Test if user asked for skew target location
      gen_skew_target_data (target_navaid);

      // Add to main mission nodes
      this->xTriggers.addChild (xTriggerTargetLand);
      this->xTriggers.addChild (xTriggerTargetHover);
      this->xObjectives.addChild (xTargetObjective);

      // find elevation using call to main thread
      target_navaid.height_mt = this->get_terrain_elevation_at_point_in_mt (target_navaid, RandomEngine::shared_navaid_info);
      target_navaid.synchToPoint ();

      //-------------------------
      // Calculate distances, bearing and initialize the "next_leg" or "starting_leg" of the <leg>/<briefer> nodes
      //-------------------------
      if (mxUtils::isElementExists (osm_na_targets, (indx - 1)) )
      {
        // lambda to return next Navaid pointer
        auto lmbda_get_next_navaid_as_ptr =[&](const int local_index) -> missionx::NavAidInfo* {
          if (mxUtils::isElementExists (osm_na_targets, local_index + 1))
            return &osm_na_targets[local_index + 1];

          return nullptr;
        };

        auto next_navaid_ptr = lmbda_get_next_navaid_as_ptr(indx);

        gen_gather_navaid_metadata_relative_to_target (this->xMetadata, target_navaid, osm_na_targets[indx - 1], next_navaid_ptr);

        // If we have a previous <leg> node, we need to update its "next_leg" and "distance" attributes for future usage with messages and descriptions
        if (!osm_na_targets[indx - 1].fpln_xml_target_leg_node.isEmpty ())
        {
          // update "starting_leg" or "next_leg" attributes
          const auto current_leg_name = Utils::readAttrib ( target_navaid.fpln_xml_target_leg_node, mxconst::get_ATTRIB_NAME (), "" );
          if (osm_na_targets[indx - 1].flag_is_brieferOrStartLocation)
            osm_na_targets[indx - 1].fpln_xml_target_leg_node.updateAttribute (current_leg_name.c_str (), mxconst::get_ATTRIB_STARTING_LEG ().c_str (), mxconst::get_ATTRIB_STARTING_LEG ().c_str () );
          else
            osm_na_targets[indx - 1].fpln_xml_target_leg_node.updateAttribute (current_leg_name.c_str (), mxconst::get_ATTRIB_NEXT_LEG ().c_str (), mxconst::get_ATTRIB_NEXT_LEG ().c_str () );

          osm_na_targets[indx - 1].fpln_xml_target_leg_node.updateAttribute (fmt::format ("{}", osm_na_targets[indx - 1].fpln_distance_between_prev_and_current_navaid).c_str (), mxconst::get_ATTRIB_DISTANCE_NM ().c_str (), mxconst::get_ATTRIB_DISTANCE_NM ().c_str () );


          // add <leg> description
          IXMLNode xml_desc_ptr = gen_leg_description (target_navaid.fpln_xml_target_leg_node, target_navaid, next_navaid_ptr);

          // add target marker // DEPRECATED for now, since we handle the same at the "template" level. I used it for debug purposes on the new implementation.
          // gen_3d_marker_for_target (target_navaid.fpln_xml_target_leg_node, target_navaid);

          // add start messages
          gen_leg_start_messages (this->seq_messages, target_navaid,this->xMessages);

          // add hint messages related to the target land/hover actions
          gen_messages_when_reaching_target_leg (this->seq_triggers, this->seq_messages, target_navaid, this->xMessages, this->xTriggers, xTriggerTargetLand, xTriggerTargetHover);

          // generate message when nearing the target (2nm)
          gen_2nm_message (this->seq_triggers, this->seq_messages, target_navaid, this->xMessages, this->xTriggers, xTriggerTargetLand);

          // add 3D object sets
          gen_add_3d_objects_for_surprise_me_base_on_predefined_attributes (target_navaid, target_navaid.fpln_xml_target_leg_node, inRootTemplate, this->x3DObjTemplate, this->expected_slope_at_target_location_d);

          // add 3D display objects around the landing
          if (!target_navaid.flag_is_skewed)
            gen_3d_hint_objects_for_land_and_hover (target_navaid, target_navaid.fpln_xml_target_leg_node, next_navaid_ptr);

          // initialize <display_object> instance attributes.
          // setInstanceProperties (target_navaid.fpln_xml_target_leg_node, target_navaid, this->xDummyTopNode, (indx == static_cast<int>(osm_na_targets.size () - 1))); // v25.06.1 deprecated flag_display_target_markers_away_from_target

          gen_parse_3d_instances_in_leg (target_navaid.fpln_xml_target_leg_node, target_navaid);
        }
      }

      target_navaid.synchToPoint ();
      target_navaid.fpln_xml_target_leg_node = this->xFlightLegs.addChild (target_navaid.fpln_xml_target_leg_node);

      // check [abort]
      if (RandomEngine::threadState.flagAbortThread)
      {
        out_mx_return.addErrMsg ("User asked to abort.", true);
        return out_mx_return;
      }

      // Add final flight plan to display in the ui
      this->cumulative_location_desc_s += target_navaid.get_loc_desc () + (( static_cast<int>(osm_na_targets.size ()) - 1 == target_navaid.fpln_seq)? "" : ", ");

    } // end "osm_target" loop over all OSM Target NavAids and construct the base information needed for the mission file


    // ----------------------
    // -- Prepare <GPS> node
    // ----------------------
    for (const auto &na : osm_na_targets | std::views::values)
    {
      auto p_gps_node = na.p.node.deepCopy ();
      auto p_gps_skew_node =  ( na.xml_skewdPointNode.isEmpty ()) ? IXMLNode::emptyIXMLNode : na.xml_skewdPointNode.deepCopy ();

      p_gps_node = Utils::xml_clear_node_attributes_excluding_list (p_gps_node,
                                            { mxconst::get_ATTRIB_LAT (), mxconst::get_ATTRIB_LONG (), mxconst::get_ATTRIB_ELEV_FT ()
                                                           , mxconst::get_ELEMENT_ICAO (), mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B ()
                                              , mxconst::get_PROP_IS_WET ()
                                            }, false, true);

      p_gps_skew_node = Utils::xml_clear_node_attributes_excluding_list (p_gps_skew_node,
                                            { mxconst::get_ATTRIB_LAT (), mxconst::get_ATTRIB_LONG (), mxconst::get_ATTRIB_ELEV_FT ()
                                                           , mxconst::get_ELEMENT_ICAO (), mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B ()
                                              , mxconst::get_PROP_IS_WET ()
                                            }, false, true);

      if (na.flag_is_skewed && !p_gps_skew_node.isEmpty ())
        this->xGPS.addChild (p_gps_skew_node);
      else
        this->xGPS.addChild (p_gps_node);
    }


    // add Briefer description
    gen_post_briefer_desc (osm_na_targets, flag_one_of_the_targets_above_water);
    this->xBriefer = osm_na_targets[0].fpln_xml_target_leg_node.deepCopy ();

    // loop over all inventories and add to the global xInventories node
    for (auto &[key, nav] : osm_na_targets )
    {

      // add to inventories
      nav.fpln_xml_inv_node = this->xInventoris.addChild (nav.fpln_xml_inv_node);
    }



    #ifndef RELEASE
    Log::logMsgThread (fmt::format ("-------------- RESULTS - Post {} --------------", __func__));
    Log::logMsgThread (fmt::format ("BRIEFER_INFO:\n{}\n", Utils::xml_get_node_content_as_text (xBrieferInfo)));
    Log::logMsgThread (fmt::format ("BRIEFER:\n{}\n", Utils::xml_get_node_content_as_text (osm_na_targets[0].fpln_xml_target_leg_node))); // we store the briefer in [0]
    Log::logMsgThread (fmt::format ("TRIGGERS:\n{}\n", Utils::xml_get_node_content_as_text (this->xTriggers)));
    Log::logMsgThread (fmt::format ("OBJECTIVES:\n{}\n", Utils::xml_get_node_content_as_text (this->xObjectives)));
    Log::logMsgThread (fmt::format ("FLIGHT LEGS:\n{}\n", Utils::xml_get_node_content_as_text (this->xFlightLegs)));
    Log::logMsgThread (fmt::format ("Inventories:\n{}\n", Utils::xml_get_node_content_as_text (this->xInventoris)));
    Log::logMsgThread (fmt::format ("GPS:\n{}\n", Utils::xml_get_node_content_as_text (this->xGPS)));
    Log::logMsgThread (fmt::format ("-------------- END RESULTS - {} --------------", __func__));
    #endif // !RELEASE

  } // end loop over all target nodes


  return out_mx_return = true;
}


// -----------------------------------


mx_return
RandomEngine::prepare_mission_based_on_oilrig2 (IXMLNode &inRootTemplate, std::string &outErr)
{
  missionx::mx_return out_mx_return;
  auto navaid_targets = gen_oilrig_targets (&RandomEngine::threadState, missionx::data_manager::xmlMappingNode, this->xMetadata, RandomEngine::shared_navaid_info, outErr);

  if (navaid_targets.empty ())
  {
    out_mx_return.addErrMsg ("No valid targets were found. Aborting.", true);
    return out_mx_return;

  }
  // check [abort]
  if (RandomEngine::threadState.flagAbortThread)
  {
    out_mx_return.addErrMsg ("User asked to abort.", true);
    return out_mx_return;
  }

  bool flag_one_of_the_targets_above_water = false;
  //-----------------------------------------------
  //--- Analyze Water Bodies / Slope / Leg Name ---
  //-----------------------------------------------
  for (auto &target_navaid : navaid_targets | std::views::values)
  {
    target_navaid.fpln_wp_type = mxconst::get_FL_TEMPLATE_VAL_LAND();

    target_navaid.fpln_is_wet = get_is_wet_at_point (target_navaid);

    // store wet state if the "flag value" is not true, yet.
    if (!flag_one_of_the_targets_above_water)
      flag_one_of_the_targets_above_water = target_navaid.fpln_is_wet;

    // store slope at the target location
    target_navaid.fpln_slope = get_slope_at_point (target_navaid);

    target_navaid.fpln_leg_name = gen_leg_name ( &this->seq_waypoints, mxconst::get_GPS_WP (),"leg", target_navaid );
  }

  // ----------------------
  // -- Add <briefer> node - Start Location
  // ----------------------
  gen_briefer_node (navaid_targets[0], RandomEngine::shared_navaid_info, flag_one_of_the_targets_above_water);

  // ----------------------
  // -- Read and set <mission_info>
  // ----------------------
  IXMLNode x_local_BrieferInfo;
  if (missionx::RandomEngine::working_tempFile_ptr != nullptr)
  {
    auto template_image_file_name = (missionx::RandomEngine::working_tempFile_ptr->getTemplateImageFileName ().empty ()) ? mxconst::get_DEFAULT_RANDOM_IMAGE_FILE () : missionx::RandomEngine::working_tempFile_ptr->getTemplateImageFileName ();
    auto template_name            = missionx::RandomEngine::working_tempFile_ptr->fullFilePath;
    auto template_folder_name     = missionx::RandomEngine::working_tempFile_ptr->missionFolderName;

    x_local_BrieferInfo = gen_mission_info_node (inRootTemplate, template_name, template_image_file_name, template_folder_name);
  }
  else
    x_local_BrieferInfo = gen_mission_info_node (inRootTemplate, "", "", "");




  // ------------------------------------------------------------------
  // Construct all mission <leg> nodes
  // navaid_targets: [0] = start/briefer, [1] = Oil-rig, [2] = final location.
  // ------------------------------------------------------------------
  if (!navaid_targets.empty())
  {
    // loop over all targets
    for (auto &[indx, target_navaid] : navaid_targets)
    {
      target_navaid.fpln_seq = indx;

      if (indx == 0) // skip briefer
      {
        target_navaid.fpln_mission_phase = missionx::enums::mx_rnd_mission_phase::start;
        continue;
      }

      // decide the "landing type: land or hover_land". In Oil-Rig case all should be Land
      target_navaid.fpln_wp_type = mxconst::get_FL_TEMPLATE_VAL_LAND();

      // is last flight leg ?
      target_navaid.fpln_is_last_flight_leg = ! ( mxUtils::isElementExists (navaid_targets, indx + 1) );

      if (target_navaid.fpln_is_last_flight_leg)
        target_navaid.fpln_mission_phase = enums::mx_rnd_mission_phase::land_extraction; // represent last waypoint
      else
        target_navaid.fpln_mission_phase = enums::mx_rnd_mission_phase::land_target; // represent target

      // We are basically constructing the mission from the middle waypoint and then need to add the start and end coordinates.
      // Write dedicated functions to only prepare the specific "needed" node.
      // Example: prepare trigger (seq, name, radius)
      IXMLNode xTriggerTargetHover = IXMLNode::emptyIXMLNode;
      IXMLNode xTriggerTargetLand = IXMLNode::emptyIXMLNode;
      IXMLNode xTaskTargetHover = IXMLNode::emptyIXMLNode;
      IXMLNode xTaskTargetLand  = IXMLNode::emptyIXMLNode;


      // ----------------------------------------------------------
      // Prepare the properties for the target triggers and tasks
      // ----------------------------------------------------------


      // Land properties
      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_trig_land  =
        {
        { "conditions", "plane_on_ground", "true" },
        { "radius", "length_mt", (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())? mxconst::DEFAULT_LAND_OR_INV_RADIUS_MT.data() : mxconst::DEFAULT_HOVER_RADIUS_MT.data () }
        };

      // Hover trigger properties
      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_trig_hover = {
        { "conditions", "plane_on_ground", "false" },
        { "radius", "length_mt", mxconst::DEFAULT_HOVER_RADIUS_MT.data () },
        { "elevation_volume", "elev_lower_upper_ft", fmt::format("---{}", mxconst::DEFAULT_HOVER_VOL_HEIGHT_FOR_OSM_FT ) }, // example: "---328"
      };


      // Land trigger creation
      xTriggerTargetLand = RandomEngine::gen_trigger_node (this->seq_triggers, "trig", "land", target_navaid, lsAttrib_trig_land, nullptr);
      // Hover trigger creation
      if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
        xTriggerTargetHover = RandomEngine::gen_trigger_node (this->seq_triggers, "trig", "hover", target_navaid, lsAttrib_trig_hover, nullptr);


      // TASK setup will initialize after trigger setup since we need values from the trigger node
      // Land + Hover Task Properties
      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_land_task_target = {
        { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_BASE_ON_TRIGGER (), Utils::readAttrib (xTriggerTargetLand, mxconst::get_ATTRIB_NAME (), "") },
        { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_MANDATORY (), "true" }, { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC (), "20" },
      };
      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_hover_task_target = {
        { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_BASE_ON_TRIGGER (), Utils::readAttrib (xTriggerTargetHover, mxconst::get_ATTRIB_NAME (), "") },
        { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_MANDATORY (), "true" },
        { mxconst::get_ELEMENT_TASK (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC (), "20" },
      };

      // Land Task
      xTaskTargetLand    = RandomEngine::gen_task_node (this->seq_tasks, "task", "land", target_navaid, lsAttrib_land_task_target, nullptr);

      // Hover Task
      if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
        xTaskTargetHover = RandomEngine::gen_task_node (this->seq_tasks, "task", "hover", target_navaid, lsAttrib_hover_task_target, nullptr);

      // END Handling LAND + HOVER Triggers and Tasks


      // -------------------------------------------------
      // Add inventory - always create an inventory node

      // prepare attributes to modify
      const auto inv_radius = (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())? mxconst::DEFAULT_LAND_OR_INV_RADIUS_MT.data () : mxconst::DEFAULT_HOVER_RADIUS_MT.data ();
      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_inv = {
        { mxconst::get_ELEMENT_POINT (), mxconst::get_ATTRIB_LAT (), target_navaid.getLat () },
        { mxconst::get_ELEMENT_POINT (), mxconst::get_ATTRIB_LONG (), target_navaid.getLon () },
        { mxconst::get_ELEMENT_RADIUS (), mxconst::get_ATTRIB_LENGTH_MT (), inv_radius },
        { mxconst::get_ELEMENT_INVENTORY (), mxconst::get_ATTRIB_INHIBIT_MXPAD_B (), "true" }, // inhibit mx-pad toggle when entering inventory area and we are airborne
      };

      target_navaid.fpln_xml_inv_node = gen_inventory_node (indx, target_navaid, map_osm_inventory_track, lsAttrib_inv);
      // todo: move script creation after all targets were generated
      if (!target_navaid.fpln_xml_inv_node.isEmpty ())
      {
        // create scripts and attach them into the <inventory> as a sub-element.
        RandomEngine::gen_target_inventory_scripts (target_navaid, map_osm_inventory_track);
      }

      // ----------------
      // Create Objective
      // ----------------
      IXMLNode xTargetObjective = RandomEngine::gen_objective_node (this->seq_objectives, "obj", "target");


      // -------------------------------------------------
      // POST Actions to update the Trigger information.
      // We need the names and values that were not available during node creation.
      // -------------------------------------------------
      const auto task_land_name  = Utils::readAttrib (xTaskTargetLand, mxconst::get_ATTRIB_NAME (), "");
      const auto task_hover_name = Utils::readAttrib (xTaskTargetHover, mxconst::get_ATTRIB_NAME (), "");

      const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_outcome_target_trig =
        {
        {mxconst::get_ELEMENT_OUTCOME (), mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS (), fmt::format ("{}{}", task_land_name, (task_hover_name.empty ()? "" : "," + task_hover_name) ) },
        };

      // Set Triggers post attributes
      if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
        Utils::xml_search_and_set_attributes_in_node (xTriggerTargetHover, lsAttrib_outcome_target_trig);
      Utils::xml_search_and_set_attributes_in_node (xTriggerTargetLand, lsAttrib_outcome_target_trig);

      // Link task to objective
      if (target_navaid.fpln_wp_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
        xTargetObjective.addChild (xTaskTargetHover);
      xTargetObjective.addChild (xTaskTargetLand);

      //-------------------------
      // <leg> element creation
      //-------------------------
      const auto obj_target_name = Utils::readAttrib (xTargetObjective, mxconst::get_ATTRIB_NAME (), "");
      std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_wp_target =
      {
        { mxconst::get_ELEMENT_LEG (), mxconst::get_ATTRIB_TITLE (), target_navaid.gen_locDesc_short () },
        { mxconst::get_ELEMENT_LINK_TO_OBJECTIVE (), mxconst::get_ATTRIB_NAME (), obj_target_name },
      };


      if ( mxUtils::isElementExists (RandomEngine::map_flight_legs_translation_from_template, target_navaid.fpln_seq) )
      {
        target_navaid.fpln_xml_target_leg_node = missionx::data_manager::xmlMappingNode.getChildNode (missionx::RandomEngine::map_flight_legs_translation_from_template[target_navaid.fpln_seq].c_str ()).deepCopy ();
        const bool b_successfully_set_leg_tag  = Utils::xml_set_tag_name (target_navaid.fpln_xml_target_leg_node, mxconst::get_ELEMENT_LEG ());
      }

      target_navaid.fpln_xml_target_leg_node = RandomEngine::gen_leg_node ( mxconst::get_GPS_WP (), "leg", &target_navaid, &lsAttrib_wp_target);
      #ifndef RELEASE
      Log::logMsgThread (fmt::format ("[{}] oilrig leg index: {}, Node element:\n{}\n<-----", __func__, target_navaid.fpln_seq, Utils::xml_get_node_content_as_text (target_navaid.fpln_xml_target_leg_node)));
      #endif


      // Test if user asked for skew target location
      gen_skew_target_data (target_navaid);

      // Add to main mission nodes
      this->xTriggers.addChild (xTriggerTargetLand);
      this->xTriggers.addChild (xTriggerTargetHover);
      this->xObjectives.addChild (xTargetObjective);

      // find elevation using call to the main thread
      target_navaid.height_mt = this->get_terrain_elevation_at_point_in_mt (target_navaid, RandomEngine::shared_navaid_info);
      target_navaid.synchToPoint ();

      //-------------------------
      // Calculate distances, bearing and initialize the "next_leg" or "starting_leg" of the <leg>/<briefer> nodes
      //-------------------------
      if (mxUtils::isElementExists (navaid_targets, (indx - 1)) )
      {
        // lambda to return next Navaid pointer
        auto lmbda_get_next_navaid_as_ptr =[&](const int local_index) -> missionx::NavAidInfo* {
          if (mxUtils::isElementExists (navaid_targets, local_index + 1))
            return &navaid_targets[local_index + 1];

          return nullptr;
        };

        auto next_navaid_ptr = lmbda_get_next_navaid_as_ptr(indx);

        gen_gather_navaid_metadata_relative_to_target (this->xMetadata, target_navaid, navaid_targets[indx - 1], next_navaid_ptr);

        // If we have a previous <leg> node, we need to update its "next_leg" and "distance" attributes for future usage with messages and descriptions
        if (!navaid_targets[indx - 1].fpln_xml_target_leg_node.isEmpty ())
        {
          // update "starting_leg" or "next_leg" attributes
          const auto current_leg_name = Utils::readAttrib ( target_navaid.fpln_xml_target_leg_node, mxconst::get_ATTRIB_NAME (), "" );
          if (navaid_targets[indx - 1].flag_is_brieferOrStartLocation)
            navaid_targets[indx - 1].fpln_xml_target_leg_node.updateAttribute (current_leg_name.c_str (), mxconst::get_ATTRIB_STARTING_LEG ().c_str (), mxconst::get_ATTRIB_STARTING_LEG ().c_str () );
          else
            navaid_targets[indx - 1].fpln_xml_target_leg_node.updateAttribute (current_leg_name.c_str (), mxconst::get_ATTRIB_NEXT_LEG ().c_str (), mxconst::get_ATTRIB_NEXT_LEG ().c_str () );

          navaid_targets[indx - 1].fpln_xml_target_leg_node.updateAttribute (fmt::format ("{}", navaid_targets[indx - 1].fpln_distance_between_prev_and_current_navaid).c_str (), mxconst::get_ATTRIB_DISTANCE_NM ().c_str (), mxconst::get_ATTRIB_DISTANCE_NM ().c_str () );


          // add <leg> description
          IXMLNode xml_desc_ptr = gen_leg_description (target_navaid.fpln_xml_target_leg_node, target_navaid, next_navaid_ptr);

          // add start messages
          gen_leg_start_messages (this->seq_messages, target_navaid,this->xMessages);

          // add hint messages related to the target land/hover actions
          gen_messages_when_reaching_target_leg (this->seq_triggers, this->seq_messages, target_navaid, this->xMessages, this->xTriggers, xTriggerTargetLand, xTriggerTargetHover);

          // generate a message when nearing the target (2nm)
          gen_2nm_message (this->seq_triggers, this->seq_messages, target_navaid, this->xMessages, this->xTriggers, xTriggerTargetLand);

          // add 3D object sets if they were defined at <leg> or <desc> level
          // gen_add_3d_objects_for_surprise_me_base_on_predefined_attributes (target_navaid, target_navaid.fpln_xml_target_leg_node, inRootTemplate, this->x3DObjTemplate, this->expected_slope_at_target_location_d);

          // add 3D display objects around the landing
          // v25.09.2 add support for <display_object_set>
          gen_add_3d_display_object_sets_instances_to_leg (target_navaid, target_navaid.fpln_xml_target_leg_node, inRootTemplate, this->x3DObjTemplate, this->expected_slope_at_target_location_d);

          // In cases of Oil-Rig missions, ignore custom display objects. Only use "3d object sets"
          // add <display_object> from the template custom leg node. Should be a subnode.
          // gen_parse_and_add_all_display_objects_in_node (__func__, target_navaid.fpln_xml_osm_q_or_raw_tmpl_node, target_navaid.fpln_xml_target_leg_node, inRootTemplate, this->x3DObjTemplate, this->expected_slope_at_target_location_d);

          // if (!target_navaid.flag_is_skewed)
          //   gen_3d_hint_objects_for_land_and_hover (target_navaid, target_navaid.fpln_xml_target_leg_node, next_navaid_ptr);

          gen_parse_3d_instances_in_leg (target_navaid.fpln_xml_target_leg_node, target_navaid);
        }
      } // end if target navaid is not the first or last

      target_navaid.synchToPoint ();
      target_navaid.fpln_xml_target_leg_node = this->xFlightLegs.addChild (target_navaid.fpln_xml_target_leg_node);

      // check [abort]
      if (RandomEngine::threadState.flagAbortThread)
      {
        out_mx_return.addErrMsg ("User asked to abort.", true);
        return out_mx_return;
      }

      // Add the final flight plan to display in the ui
      this->cumulative_location_desc_s += target_navaid.get_loc_desc () + (( static_cast<int>(navaid_targets.size ()) - 1 == target_navaid.fpln_seq)? "" : ", ");

    } // end "osm_target" loop over all OSM Target NavAids and construct the base information needed for the mission file


    // ----------------------
    // -- Prepare <GPS> node
    // ----------------------
    for (const auto &na : navaid_targets | std::views::values)
    {
      auto p_gps_node = na.p.node.deepCopy ();
      auto p_gps_skew_node =  ( na.xml_skewdPointNode.isEmpty ()) ? IXMLNode::emptyIXMLNode : na.xml_skewdPointNode.deepCopy ();

      p_gps_node = Utils::xml_clear_node_attributes_excluding_list (p_gps_node,
                                            { mxconst::get_ATTRIB_LAT (), mxconst::get_ATTRIB_LONG (), mxconst::get_ATTRIB_ELEV_FT ()
                                              , mxconst::get_ELEMENT_ICAO (), mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B ()
                                              , mxconst::get_PROP_IS_WET ()
                                            }, false, true);

      p_gps_skew_node = Utils::xml_clear_node_attributes_excluding_list (p_gps_skew_node,
                                            { mxconst::get_ATTRIB_LAT (), mxconst::get_ATTRIB_LONG (), mxconst::get_ATTRIB_ELEV_FT ()
                                              , mxconst::get_ELEMENT_ICAO (), mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B ()
                                              , mxconst::get_PROP_IS_WET ()
                                            }, false, true);

      if (na.flag_is_skewed && !p_gps_skew_node.isEmpty ())
        this->xGPS.addChild (p_gps_skew_node);
      else
        this->xGPS.addChild (p_gps_node);
    }

    // add Briefer description
    gen_post_briefer_desc (navaid_targets, flag_one_of_the_targets_above_water);
    this->xBriefer = navaid_targets[0].fpln_xml_target_leg_node.deepCopy ();

    // loop over all inventories and add to the global xInventories node
    for (auto &[key, nav] : navaid_targets )
    {
      // add to inventories
      nav.fpln_xml_inv_node = this->xInventoris.addChild (nav.fpln_xml_inv_node);
    }



    #ifndef RELEASE
    Log::logMsgThread (fmt::format ("-------------- RESULTS - Post {} --------------", __func__));
    Log::logMsgThread (fmt::format ("BRIEFER_INFO:\n{}\n", Utils::xml_get_node_content_as_text (x_local_BrieferInfo)));
    Log::logMsgThread (fmt::format ("BRIEFER:\n{}\n", Utils::xml_get_node_content_as_text (navaid_targets[0].fpln_xml_target_leg_node))); // we store the briefer in [0]
    Log::logMsgThread (fmt::format ("TRIGGERS:\n{}\n", Utils::xml_get_node_content_as_text (this->xTriggers)));
    Log::logMsgThread (fmt::format ("OBJECTIVES:\n{}\n", Utils::xml_get_node_content_as_text (this->xObjectives)));
    Log::logMsgThread (fmt::format ("FLIGHT LEGS:\n{}\n", Utils::xml_get_node_content_as_text (this->xFlightLegs)));
    Log::logMsgThread (fmt::format ("Inventories:\n{}\n", Utils::xml_get_node_content_as_text (this->xInventoris)));
    Log::logMsgThread (fmt::format ("GPS:\n{}\n", Utils::xml_get_node_content_as_text (this->xGPS)));
    Log::logMsgThread (fmt::format ("-------------- END RESULTS - {} --------------", __func__));
    #endif // !RELEASE

  } // end loop over all target nodes


  //outErr = "Oilrig is being re-write, Pick other type of mission, thanks.";
  return out_mx_return.addErrMsg (outErr, true); // TODO: replace with "true" once code works as expected.
}




// -----------------------------------



void
RandomEngine::calculate_bbox_coordinates (missionx::Point &outN0, missionx::Point &outS180, missionx::Point &outE90, missionx::Point &outW270, const float inRefLat, const float inRefLon, const double inMaxRadius_d)
{

  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (outN0.lat, outN0.lon, inRefLat, inRefLon, 0, inMaxRadius_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (outS180.lat, outS180.lon, inRefLat, inRefLon, 180, inMaxRadius_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (outE90.lat, outE90.lon, inRefLat, inRefLon, 90, inMaxRadius_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (outW270.lat, outW270.lon, inRefLat, inRefLon, 270, inMaxRadius_d);
}

// -----------------------------------

void
RandomEngine::gather_all_osm_db_files_names_and_path (std::list<std::string> &outListOfFiles)
{
  assert (RandomEngine::working_tempFile_ptr != nullptr && "template pointer is invalid.");

  const static std::string working_folder   = data_manager::mx_folders_properties.getAttribStringValue (mxconst::get_PROP_MISSIONX_PATH (), "", data_manager::errStr) + "/" + mxconst::get_DB_FOLDER_NAME () + "/";
  const static std::string plugin_folder_db = (RandomEngine::working_tempFile_ptr->missionFolderName.empty ()) ? "" : working_tempFile_ptr->filePath + "/" + mxconst::get_DB_FOLDER_NAME () + "/";

  missionx::ListDir::getListOfFilesAsFullPath (working_folder.c_str (), outListOfFiles, mxconst::get_DB_FILE_EXTENSION ());
  missionx::ListDir::getListOfFilesAsFullPath (plugin_folder_db.c_str (), outListOfFiles, mxconst::get_DB_FILE_EXTENSION (), true);
}


// -----------------------------------

bool
RandomEngine::osm_get_navaid_from_osm (NavAidInfo                         &outNavAid,
                                       std::map<std::string, std::string> &inMapLocationSplitValues,
                                       missionx::mx_base_node             &inProperties, // v3.305.1
                                       const double                        sourceLat_d,
                                       const double                        sourceLon_d,
                                       double                              min_lat,
                                       double                              max_lat,
                                       double                              min_lon,
                                       double                              max_lon,
                                       const double                        maxDistance_d,
                                       const double                        minDistance_d)
{
  bool bResult = false;

  const auto lmbda_fix_order_of_min_max = [&] ()
  {
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

  lmbda_fix_order_of_min_max ();

  const std::string expectedLocationType = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), ""); // v3.0.241.10 b2

  // Priority 1: We start with DB search only if it was asked. // v24.12.2 Although searching the DB is a redundant step, it might be useful in edge cases.
  if (Utils::readBoolAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_OSM_CHECKBOX (), false) || (expectedLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM ()) || (expectedLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ()))
  {
    bResult = osm_get_navaid_from_osm_database (outNavAid, inMapLocationSplitValues, inProperties, sourceLat_d, sourceLon_d, min_lat, max_lat, min_lon, max_lon, maxDistance_d, minDistance_d);
    if (bResult)
      return bResult;
  }

  // Priority 2 is for overpass data (web information)
  if (!bResult && (Utils::readBoolAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_WEB_OSM_CHECKBOX (), false) || (expectedLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ())))
  {
    return osm_get_navaid_from_overpass (outNavAid, inMapLocationSplitValues, inProperties, sourceLat_d, sourceLon_d, min_lat, max_lat, min_lon, max_lon, maxDistance_d, minDistance_d);
  }


  return bResult;
}


// ------------------------------------------

bool
RandomEngine::osm_get_navaid_from_overpass (NavAidInfo                         &outNavAid,
                                            std::map<std::string, std::string> &inMapLocationSplitValues,
                                            missionx::mx_base_node             &inProperties, // v3.305.1
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
  const std::string inLocationType             = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), ""); // v3.0.241.10 b2

  const std::string nm_s                 = (Utils::isElementExists (inMapLocationSplitValues, "nm")) ? inMapLocationSplitValues["nm"] : ""; // represent airport to find in between distances
  const std::string keyID_s              = (Utils::isElementExists (inMapLocationSplitValues, "keyid")) ? inMapLocationSplitValues["keyid"] : "icao"; // represent ID of the port, like ICAO or FAA
  const std::string keyname_s            = (Utils::isElementExists (inMapLocationSplitValues, "keyname")) ? inMapLocationSplitValues["keyname"] : "name"; // the key value that represents the name
  const std::string keydesc_s            = (Utils::isElementExists (inMapLocationSplitValues, "keydesc")) ? inMapLocationSplitValues["keydesc"] : "amenity"; // the key value that represent description, like amanity
  const std::string designer_desc_s      = (Utils::isElementExists (inMapLocationSplitValues, "desc")) ? inMapLocationSplitValues["desc"] : ""; // Free string define designer description. Should be short.
  const std::string designer_descforce_s = (Utils::isElementExists (inMapLocationSplitValues, "descforce")) ? inMapLocationSplitValues["descforce"] : ""; // Free string define designer description. Should be short.
  const std::string designer_bounds      = (Utils::isElementExists (inMapLocationSplitValues, "bounds")) ? inMapLocationSplitValues["bounds"] : ""; // v3.0.253.4 coordinates represents bottomLeft and topRight area to fetch from OVERPASS
  const std::string webosm_optimize      = (Utils::isElementExists (inMapLocationSplitValues, mxconst::get_ATTRIB_WEBOSM_OPTIMIZE ())) ? Utils::stringToLower (inMapLocationSplitValues[mxconst::get_ATTRIB_WEBOSM_OPTIMIZE ()]) : "y"; // v3.0.253.4 "y/n". Default yes. Should we do optimization on overpass area ? divide area to 1x1nm ?
  // v3.0.253.9.1 replaces force_slope with mx_which_type_to_force
  auto designer_force_type_attrib                         = static_cast<mx_which_type_to_force> (Utils::readNodeNumericAttrib<int> (inProperties.node, mxconst::get_ATTRIB_FORCE_TYPE_OF_TEMPLATE (), 0));
  int  number_of_times_to_loop_over_force_template_type_i = Utils::readNodeNumericAttrib<int> (inProperties.node, mxconst::get_PROP_NUMBER_OF_LOOPS_TO_FORCE_TYPE_TEMPLATE (), 0); // dependent on flag_force_slope

  // calculate inner bounds = represents minimum distance
  Point E90inner, W270inner, S180inner, N0inner;
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (N0inner.lat, N0inner.lon, sourceLat_d, sourceLon_d, 0, minDistance_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (S180inner.lat, S180inner.lon, sourceLat_d, sourceLon_d, 180, minDistance_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (E90inner.lat, E90inner.lon, sourceLat_d, sourceLon_d, 90, minDistance_d);
  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (W270inner.lat, W270inner.lon, sourceLat_d, sourceLon_d, 270, minDistance_d);

  // define the 4 bounding points
  Point topLeft (max_lat, min_lon);
  Point topRight (max_lat, max_lon);
  Point bottomLeft (min_lat, min_lon);
  Point bottomRight (min_lat, max_lon);

  Point plane_center (sourceLat_d, sourceLon_d);

  // calculate how many boxed are we should create
  double box_length            = 1.0; // One nautical miles (1nm)
  double bounding_distance     = topLeft - topRight;
  int    number_of_inner_boxes = static_cast<int> ((box_length >= 1.0) ? (bounding_distance / box_length) : (bounding_distance * box_length)); // calculate the number of inner boxes

  std::deque<missionx::strct_box> meshList; // will hold all mesh boxes that are inside the expected zone (min/max distance from center.

  // store all mesh boxes in 2D array
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


  if (const bool b_osm_optimize = (webosm_optimize == "y") ? true : false)
  {
    Point col0_prev_line_bottomLeft_point;
    Point prev_topRight_point;
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

        box.calcBoxBasedOn_topLeft (box_length); // calculate all 4 points in the box relative to box.topLeft
        prev_topRight_point = box.topRight; // store first area box "topRight" point for next line calculation
        if (i2 == 0)
          col0_prev_line_bottomLeft_point = box.bottomLeft;

        // calculate if box is inside the search zone
        const double dist_d = box.center - plane_center;
        if (dist_d >= minDistance_d && dist_d <= maxDistance_d)
          meshList.emplace_back (box);


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

    meshList.emplace_back (box);
  }


  // v25.06.1 Populate the vector with sequential indices (0, 1, 2, ...)
  std::vector<int> vec_shuffled_mesh_list = Utils::getShuffledIndexVector (static_cast<int>(meshList.size ()));


  ///// Pick randomly one of the boxes
  int        iTryCounter           = 0;
  const auto meshSize              = static_cast<float> (meshList.size ());
  const auto divider_i             = static_cast<float> (std::pow (10, fmt::format ("{}", meshList.size ()).length ()));
  int        max_boxes_to_search_i = static_cast<int> ((meshSize * 0.1f) * (1.0f - (meshSize / divider_i)));
  #ifndef RELEASE
  Log::logMsgNone ("\t === Will search no more than: " + mxUtils::formatNumber<int> ((max_boxes_to_search_i == 0) ? 1 : max_boxes_to_search_i) + " [bbox] areas  ===", true); // v3.0.253.7 formated a little differently so max ways will be at least 1
  #endif


  const std::string plugin_user_filter = Utils::getNodeText_type_6 (system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_FILTER (), mxconst::get_DEFAULT_OVERPASS_WAYS_FILTER ()); // missionx::system_actions::pluginSetupOptions.getPropertyValue(mxconst::get_OPT_OVERPASS_FILTER(), err);

PICK_RANDOM_OSM_BBOX:
  if (missionx::RandomEngine::threadState.flagAbortThread)
    return false;

  iTryCounter++;
  // if (!meshList.empty ())
  if ( ! vec_shuffled_mesh_list.empty () ) // v25.06.1 we work on the shuffled vector
  {
    std::string err;

    if (meshSize > 1 && iTryCounter > max_boxes_to_search_i) // We try to restrict the number of boxes to search in
    {
      Log::logMsgThread ("[overpass2] Failed to find an area with valid node/way.");
      return false;
    }

    int rnd_box_i = vec_shuffled_mesh_list.back ();
    vec_shuffled_mesh_list.pop_back ();

    auto              box         = meshList.at (rnd_box_i);
    const std::string bounds_s_01 = mxUtils::formatNumber<double> (box.bottomLeft.lat, 8) + "," + mxUtils::formatNumber<double> (box.bottomLeft.lon, 8) + "," + mxUtils::formatNumber<double> (box.topRight.lat, 8) + "," + mxUtils::formatNumber<double> (box.topRight.lon, 8);

    #ifndef RELEASE
    Log::logDebugBO ("[overpass box] boxed area: " + box.print_BL_and_TR (), true);
    Log::logDebugBO ("[overpass ] initial bound: " + bounds_s_01, true);
    #endif // !RELEASE

    // v3.0.253.6 wet box optimization in case of force_sloped_terrain is flagged. If the center of the box is wet, then we will skip it.
    if (designer_force_type_attrib == mx_which_type_to_force::force_hover)
    {
      NavAidInfo nav;
      // check if center of box is wet
      nav.lat = static_cast<float> (box.center.lat);
      nav.lon = static_cast<float> (box.center.lon);
      nav.synchToPoint ();

      if (get_is_wet_at_point (nav))
      {
        #ifndef RELEASE
        Log::logDebugBO ("[force slope] Water body found. Skipped boxed area: " + box.print_BL_and_TR (), true);
        #endif // !RELEASE

        // // Utils::deque_erase_item_at_index<missionx::strct_box> (meshList, rnd_box_i);
        goto PICK_RANDOM_OSM_BBOX;
      }
    }


    //// READ FROM OVERPASS

    // ---------- BEGIN LAMBDA
    const auto lmbda_get_designer_overpass_filter = [&] (const std::string &inLocType)
    {
      std::list<std::string> list_designer_filter;
      std::string            query_filter;

      // this is the second version of this implementation, it will use a free text (not CDATA) to get designer filter
      if (mxUtils::isElementExists (inMapLocationSplitValues, "tag"))
      {
        const std::string root_filter_tag = inMapLocationSplitValues["tag"];
        if (!xRootTemplate.getChildNode (root_filter_tag.c_str ()).isEmpty ()) // if the "tag" string exists in template pick a random sub element from it
        {
          IXMLNode    parent = xRootTemplate.getChildNode (root_filter_tag.c_str ());
          if (const IXMLNode filter_node = Utils::xml_get_node_randomly_by_name_IXMLNode (parent, "")
            ; !filter_node.isEmpty ())
            query_filter = filter_node.getText ();
          else
            query_filter = "";
        }
      }

      return query_filter;
    };
    const std::string designer_filter_s = lmbda_get_designer_overpass_filter (inLocationType);
    const std::string bounds_final      = (designer_bounds.empty ()) ? bounds_s_01 : designer_bounds;

    const auto lmbda_validate_osmweb_filter = [&] (std::string in_filter, std::string &out_filter, std::string &outErr)
    {
      outErr.clear ();

      // if no {{bbox}} was defined then fail the filter
      if (in_filter.find ("({{bbox}})") == std::string::npos)
      {
        out_filter = in_filter;
        outErr     = "No ({{bbox}}) found.";
        return false;
      }

      in_filter = missionx::mxUtils::replaceAll (in_filter, "({{bbox}})", "(" + bounds_final + ")"); // replace all {{bbox}}

      // search for ";out" string.
      if (in_filter.find (";out") == std::string::npos)
      {
        in_filter += (in_filter.back () == ';') ? "out;" : ";out;";
      }
      out_filter = in_filter;
      return true;
    };

    // v3.0.253.9
    std::string overpass_filter_s;
    if (std::string filter_err_s; !lmbda_validate_osmweb_filter (((designer_filter_s.empty ()) ? plugin_user_filter : designer_filter_s), overpass_filter_s, filter_err_s))
    {
      err = "[overpass] Filter is not valid: " + filter_err_s + "\n" + overpass_filter_s + "\n";
      Log::logMsgThread (err);
      RandomEngine::setError (err);
      return false;
    }
    // ------------- END

    const auto lmbda_get_overpass_url = [] (std::vector<std::string> &inVecUrls, int &last_url_indx_used_i, int preferred_init_url_indx_i = 0, bool inLockURL = false)
    {
      if (last_url_indx_used_i > (static_cast<int> (inVecUrls.size ()) - 1))
        last_url_indx_used_i = 0;

      if (inLockURL) // v.3.0.255.4.2 implement lockURL
        return (static_cast<int> (inVecUrls.size ()) >= preferred_init_url_indx_i) ? inVecUrls.at (preferred_init_url_indx_i) : missionx::EMPTY_STRING;

      if (!inVecUrls.empty () && last_url_indx_used_i > mxconst::INT_UNDEFINED)
        return inVecUrls.at (last_url_indx_used_i);

      last_url_indx_used_i = preferred_init_url_indx_i;
      return (inVecUrls.empty ()) ? missionx::EMPTY_STRING : inVecUrls.at (last_url_indx_used_i);
    };

    const bool lock_url_b          = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_LOCK_OVERPASS_URL_TO_USER_PICK (), false); // v3.0.255.4.2
    auto       stored_overpass_url = (RandomEngine::vecMissionInfoOverpassUrls.empty ()) ? lmbda_get_overpass_url (missionx::data_manager::vecOverpassUrls, missionx::data_manager::overpass_last_url_indx_used_i, missionx::data_manager::overpass_user_picked_combo_i, lock_url_b) : lmbda_get_overpass_url (RandomEngine::vecMissionInfoOverpassUrls, ++RandomEngine::current_url_indx_used_i, 0, false);
    err.clear ();

    ++missionx::data_manager::overpass_last_url_indx_used_i; // round-robin

    #ifndef RELEASE
    Log::logMsgThread ("[overpass] WAYS URL: " + stored_overpass_url + "?data=" + overpass_filter_s + "\n");
    #endif // !RELEASE

    //"https://overpass-api.de/api/interpreter?data=(" + overpass_filter_s +");out;"; // EXAMPLE

    const std::string url_s = stored_overpass_url + "?data=" + overpass_filter_s; // v3.0.253.9

    Log::logMsgThread ("[overpass] WAYS URL: " + url_s + "\n");

    const auto result_s = missionx::data_manager::fetch_overpass_info (url_s, err);
    if (missionx::RandomEngine::threadState.flagAbortThread)
      return false;


    if (!err.empty ())
    {
      Log::logMsgThread ("[overpass2] Error while fetching from overpass: " + err);
      goto PICK_RANDOM_OSM_BBOX;
    }
    else if (err.empty () && !result_s.empty ())
    {
      IXMLDomParser iDom;
      auto          xmlOSM             = iDom.parseString (result_s.c_str ()).deepCopy ();
      int           count_nodes_pick_i = 0;

      if (xmlOSM.nChildNode () < 3) // osm always have note + meta, so minimum should be 3
      {
        #ifndef RELEASE
        Log::logMsgThread ("\n ===osm node ==>\n" + Utils::xml_get_node_content_as_text (xmlOSM) + "\n<=== end osm node ===\n");
        #endif // !RELEASE

        Log::logMsgThread ("[overpass] Found no valid sub node elements, will try different way box."); // debug
        // Utils::deque_erase_item_at_index<missionx::strct_box> (meshList, rnd_box_i);


        goto PICK_RANDOM_OSM_BBOX;
      }


    PICK_OSM_CHILD_NODE:
      count_nodes_pick_i++;

      IXMLNode nodeOSM_XML = IXMLNode::emptyIXMLNode;
      IXMLNode xCenterNode; // v3.0.253.9 holds the center node


      // Validate: Check for valid nodes in <osm>
      if (const int nAllChildNodes = xmlOSM.nChildNode (); count_nodes_pick_i > RandomEngine::MAX_OSM_NODES_TO_SEARCH || nAllChildNodes < 1) // will exist after 10 of <sub nodes> tests or if there are no more valid sub nodes
      {
        #ifndef RELEASE
        Log::logMsgThread ("\n ===osm node ==>\n" + Utils::xml_get_node_content_as_text (xmlOSM) + "\n<=== end osm node ===\n");
        #endif // !RELEASE

        Log::logMsgThread ("[overpass] Found no valid sub node elements, will try different way box."); // debug
        // Utils::deque_erase_item_at_index<missionx::strct_box> (meshList, rnd_box_i);
        goto PICK_RANDOM_OSM_BBOX; // pick another box
      }
      else
      {
        IXMLNode picked_random_target_node_ptr = IXMLNode::emptyIXMLNode;

        std::string node_id_s;
        const int   rnd_nodes_i  = Utils::getRandomIntNumber (0, nAllChildNodes - 1);
        IXMLNode    osmChildNode = xmlOSM.getChildNode (rnd_nodes_i);
        // store some information about the picked element
        std::string tagName = osmChildNode.getName ();

        if (tagName != mxconst::get_ELEMENT_NODE_OSM () && tagName != mxconst::get_ELEMENT_WAY_OSM () && tagName != mxconst::get_ELEMENT_REL_OSM ())
        {
          Log::logMsgThread ("Picked unsupported node: <" + tagName + ">. Will fetch other node.\n");
          osmChildNode.deleteNodeContent (); // remove from XML output
          count_nodes_pick_i--; // v3.0.253.9.1 we remove this node from counter so only valid nodes will take into consideration
          goto PICK_OSM_CHILD_NODE;
        }


        int       iNd        = (osmChildNode.isEmpty ()) ? 0 : osmChildNode.nChildNode (mxconst::get_ELEMENT_ND_OSM ().c_str ());
        const int iCenterTag = osmChildNode.nChildNode (mxconst::get_ELEMENT_CENTER ().c_str ()); // should be only 1 or 0

        if (tagName == mxconst::get_ELEMENT_NODE_OSM ())
        {
          picked_random_target_node_ptr = osmChildNode;

          #ifndef RELEASE
          Log::logMsgThread ("Picked osm node: \n" + std::string (IXMLRenderer ().getString (osmChildNode)) + "\n");
          #endif
        }
        else
        {
          bool bFoundCenter = false;

          bFoundCenter   = false;
          const int nd_i = (iNd == 0) ? 0 : Utils::getRandomIntNumber (0, iNd - 1 + iCenterTag); // v3.0.253.9 we add the center tag to the mix. If result is = nd_i then we will pick Center element.


          if (iNd == 0 && iCenterTag == 0 && tagName == mxconst::get_ELEMENT_WAY_OSM ()) // fetch new zone only if <way> tag has no valid sub-elements to use
          {
            Log::logMsgThread ("[overpass2] Found no <nd> element, will try different <osm> child in same box."); // debug

            osmChildNode.deleteNodeContent ();
            goto PICK_OSM_CHILD_NODE;
          }

          // v3.0.253.9 should we pick <nd> or <center> sub-element
          auto lmbda_get_nd_or_center_tag_node = [&] ()
          {
            // consider picking center - 10% chance before the other logic will run
            if (iCenterTag > 0)
            {

              #ifndef release
              Log::logMsgThread ("[overpass] >>> plugin will use <center> element.<<<\n"); // debug
              if (!osmChildNode.isEmpty ())
                Log::logMsgThread (std::string (IXMLRenderer ().getString (osmChildNode)) + "\n");
              #endif
              iNd = nd_i;

              bFoundCenter = true;
              xCenterNode  = osmChildNode.getChildNode (mxconst::get_ELEMENT_CENTER ().c_str ()).deepCopy ();
              return xCenterNode.deepCopy ();
            }

            return osmChildNode.getChildNode (mxconst::get_ELEMENT_ND_OSM ().c_str (), nd_i);
          }; // end lmbda_get_nd_or_center_tag_node

          picked_random_target_node_ptr = lmbda_get_nd_or_center_tag_node ();

          //////////////////////////////////////////////////////
          // Handle <ref> node. if we are using <ref> then we need to fetch the <node> based on its "id" attrib value. <center> already holds the position.
          if (!bFoundCenter)
          {
            IXMLDomParser iDom2;
            node_id_s = Utils::readAttrib (picked_random_target_node_ptr, mxconst::get_ATTRIB_REF_OSM (), ""); // fetch ref attribute
            if (node_id_s.empty ())
            {
              Log::logMsgThread ("[overpass2] Found no 'ref' attribute in <nd>, element maybe malformed, will try other <way> in same area box."); // debug
              osmChildNode.deleteNodeContent ();
              goto PICK_OSM_CHILD_NODE;
            }


            // Get <node> information using <ref id="xxx" /> value
            const std::string node_url_s    = stored_overpass_url + "?data=node(id:" + node_id_s + ");out;";
            const std::string node_result_s = missionx::data_manager::fetch_overpass_info (node_url_s, err);
            if (missionx::RandomEngine::threadState.flagAbortThread)
              return false;

            #ifndef RELEASE
            Log::logMsgThread ("[overpass] NODE URL: " + node_url_s + "\nResult: " + node_result_s + "\n"); // debug
            #endif


            nodeOSM_XML = iDom2.parseString (node_result_s.c_str ()).deepCopy ();
            if (nodeOSM_XML.isEmpty () || node_result_s.empty ())
            {
              Log::logMsgThread ("[overpass2] Failed to fetch a <node>. will try a different area box."); // debug
              // Utils::deque_erase_item_at_index<missionx::strct_box> (meshList, rnd_box_i);
              goto PICK_RANDOM_OSM_BBOX; // pick another box
            }
            else
            {
              // store the <node> value for later use as position
              picked_random_target_node_ptr = nodeOSM_XML.getChildNode (mxconst::get_ELEMENT_NODE_OSM ().c_str ()).deepCopy ();

              #ifndef RELEASE
              auto wayNode_s = std::string (IXMLRenderer ().getString (osmChildNode));
              Log::logMsgThread ("[overpass] Found 'ref' attributes in 'nd' elements. Will pick way based on nd ref: " + node_id_s + "\n");
              Log::logMsgThread ("[overpass] Picked Way:\n" + wayNode_s + "\n");
              #endif // !RELEASE

            } // end if we read <ref> and not <center> element
          }
          // Fetch the node from OVERPASS
        }



        ///////////////////// Test the target/NavAid Node ////////////////////////////
        // get position node and use it to construct the target
        IXMLNode target_node_pos_ptr = picked_random_target_node_ptr.deepCopy (); //

        if (target_node_pos_ptr.isEmpty ())
        {
          Log::logMsgThread ("[overpass2] Failed to fetch a <node>. will try a different way in same box."); // debug
          osmChildNode.deleteNodeContent ();
          goto PICK_OSM_CHILD_NODE;
        }

        /////// DISTANCE TEST ////////
        // v3.0.253.7 - [fix bug] slope was always 0 because we did not initialize the node coordinates before testing the slope. Result was always 0.
        outNavAid.lat = Utils::readNodeNumericAttrib<float> (target_node_pos_ptr, mxconst::get_ATTRIB_LAT (), 0.0f);
        outNavAid.lon = Utils::readNodeNumericAttrib<float> (target_node_pos_ptr, mxconst::get_ATTRIB_LONG_OSM (), 0.0f);

        //// v3.0.255.5 calculate distance even if BBOX information was optimized.  v3.0.255.4.1 validate distance is legit using nm_s and nm_between if not optimized
        {
          if (lastFlightLegNavInfo.lat != 0 && lastFlightLegNavInfo.lon != 0)
          {
            const double distance_to_target = Utils::calcDistanceBetween2Points_nm (lastFlightLegNavInfo.lat, lastFlightLegNavInfo.lon, outNavAid.lat, outNavAid.lon);
            double       nm_d               = (nm_s.empty ()) ? static_cast<double> (mxconst::INT_UNDEFINED) : mxUtils::stringToNumber<double> (nm_s, 2);

            #ifndef RELEASE
            Log::logMsgThread (fmt::format ("[overpass2] Test Distance. Target distance: {}, Allowed distances[nm/between] [nm: {}/ between: {} and {}]", distance_to_target, (nm_d > 0.0) ? mxUtils::formatNumber<double> (nm_d, 2) : "Not Defined", minDistance_d, maxDistance_d)); // debug
            #endif

            if (!missionx::RandomEngine::get_isNavAidInValidDistance (distance_to_target, nm_d, minDistance_d, maxDistance_d))
            {
              #ifndef RELEASE
              Log::logMsgThread (fmt::format ("[overpass2] target picked is not in the correct distance. Picked target in: {}, nm: {}, or between: {} and {}", distance_to_target, nm_d, minDistance_d, maxDistance_d)); // debug
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
          outNavAid.synchToPoint ();
          auto slope_d = get_slope_at_point (outNavAid);

          auto const lmbda_was_expected_slope_correlate_to_force_type = [&] ()
          {
            if (designer_force_type_attrib == RandomEngine::mx_which_type_to_force::force_hover && slope_d < missionx::data_manager::Max_Slope_To_Land_On)
              return false;
            else if (designer_force_type_attrib == RandomEngine::mx_which_type_to_force::force_flat_terrain_to_land && slope_d > missionx::data_manager::Max_Slope_To_Land_On)
              return false;

            return true;
          };

          if (!lmbda_was_expected_slope_correlate_to_force_type ())
          {
            if (designer_force_type_attrib == RandomEngine::mx_which_type_to_force::force_hover)
              Log::logMsgThread ("[overpass2] Failed slope test for overpass node. Found slope: " + mxUtils::formatNumber<double> (slope_d, 2) + ", in: " + outNavAid.get_latLon_name ()); // debug
            else
              Log::logMsgThread ("[overpass2] Failed flat terrain probe for overpass node. Found slope: " + mxUtils::formatNumber<double> (slope_d, 2) + ", in: " + outNavAid.get_latLon_name ()); // debug

            osmChildNode.deleteNodeContent ();

            if (count_nodes_pick_i < number_of_times_to_loop_over_force_template_type_i)
              goto PICK_OSM_CHILD_NODE;
            else
            {
              Log::logMsgThread ("[overpass2] Slopped node failed for: " + mxUtils::formatNumber<int> (number_of_times_to_loop_over_force_template_type_i) + " times, Will try other <way> box"); // v3.0.253.6

              // Utils::deque_erase_item_at_index<missionx::strct_box> (meshList, rnd_box_i);
              goto PICK_RANDOM_OSM_BBOX; // pick another box
            }
          }

        } // end force loop type


        // read and store way metadata
        {
          // Fetch WAY tag information
          const int tags_i = osmChildNode.nChildNode ("tag"); // check
          for (int i1 = 0; i1 < tags_i; ++i1)
          {
            auto tagNode = osmChildNode.getChildNode ("tag", i1);
            // bool              bFound  = false;
             const std::string key   = Utils::readAttrib (tagNode, "k", "");
            const std::string value = Utils::readAttrib (tagNode, "v", "");

            if (key == keyname_s || key == "name" || (key == "name_desc" && outNavAid.getName ().empty ())) // There is duplications but it provides a safety net
            {
              if (!value.empty ())
                outNavAid.setName (value);
            }
            else if (key == keyID_s || (key == "faa") || (key == "icao")) // default: "icao", in US I found faa instead of icao. There is duplications but it provides safety net
            {
              outNavAid.setID (value);
            }
            else if (key == "loc_name")
            {
              if (outNavAid.getName ().empty () && !value.empty ())
                outNavAid.setName (value);

              if (outNavAid.loc_desc.empty ())
                outNavAid.loc_desc = value;
            }
            else if ((key == "description"))
            {
              if (outNavAid.getName ().empty ())
                outNavAid.setName (value);

              if (!value.empty ())
                outNavAid.loc_desc = value;
            }
            else if (key == keydesc_s || key == "amenity") // There is duplications but it provides safety net
            {
              if (outNavAid.loc_desc.empty ())
                outNavAid.loc_desc = value;
            }

          } // end loop over all "<tag k="" v="" />" sub elements

          // add the designer description to the specific picked location only if segment statement is valid.
          if (!designer_descforce_s.empty ())
            outNavAid.loc_desc = designer_desc_s;
          else if (!designer_desc_s.empty () && outNavAid.loc_desc.empty ())
            outNavAid.loc_desc = designer_desc_s;


          // handle cases ware there is no name and id
          if (outNavAid.getName ().empty ()) // v3.0.253.6
            Log::logMsgThread ("[osm_get_navaid_overpass] FYI: Found <way> without a name key/value."); // outNavAid.setName("overpass");
          else
            Log::logMsgThread ("[osm_get_navaid_overpass] FYI: Found <way> with name: " + outNavAid.getName ()); // outNavAid.setName("overpass");



          if (missionx::RandomEngine::threadState.flagAbortThread)
            return false;


          //// Search intersections example:
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
          // Retrieve all the <ways> with name near node with id: 4485963840
          //(
          //  node(id:4485963840);<;
          //)->.a;way.a[name];>
          //;out;
          //
          //(
          //  node(id:568296734);<;
          //  node(id:3557252874);<;
          //)->.a;way.a[name][highway][!building];
          // out;


          if (mxUtils::is_number (node_id_s) || (outNavAid.lat != 0.0f && outNavAid.lon != 0.0f))
          {

            auto const lmbda_who_intersect_point = [&] ()
            {
              std::string filter_s;
              if (!node_id_s.empty ())
              {
                filter_s += "node(id:" + node_id_s + ");<;";
              }

              if (outNavAid.lat != 0.0f && outNavAid.lon != 0.0f)
              {
                filter_s += "way(around:20," + outNavAid.getLat () + "," + outNavAid.getLon () + ");";
              }

              return filter_s; // missionx::EMPTY_STRING;
            };

            // search ways around node
            const std::string around_url_s = stored_overpass_url + "?data=(" + lmbda_who_intersect_point () + ")->.a;way.a['name']['highway'][!'building'];out;";

            #ifndef RELEASE
            Log::logMsgThread ("[overpass] Fetch ways around navaid URL: \n" + around_url_s + "\n");
            #endif // !RELEASE

            const std::string around_result_s = missionx::data_manager::fetch_overpass_info (around_url_s, err);
            if (err.empty () && !around_result_s.empty ())
              outNavAid.xml_osm_around = iDom.parseString (around_result_s.c_str ()).deepCopy ();

            #ifndef RELEASE
            if (!outNavAid.xml_osm_around.isEmpty ())
              Log::logMsgThread ("Ways around navaid: \n" + std::string (IXMLRenderer ().getString (outNavAid.xml_osm_around)) + "\n");
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
RandomEngine::osm_get_navaid_from_osm_database (NavAidInfo                         &outNavAid,
                                                std::map<std::string, std::string> &inMapLocationSplitValues,
                                                // missionx::mxProperties&             inProperties,
                                                missionx::mx_base_node &inProperties, // v3.305.1
                                                double                  sourceLat_d,
                                                double                  sourceLon_d,
                                                double                  min_lat,
                                                double                  max_lat,
                                                double                  min_lon,
                                                double                  max_lon,
                                                double                  maxDistance_d,
                                                double                  minDistance_d)
{
  bool        flag_found_navaid_from_osm = false;
  std::string file_name;

  const std::string inLocationType = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), ""); // v3.0.241.10 b2

  const std::string dbfile               = (Utils::isElementExists (inMapLocationSplitValues, "dbfile")) ? inMapLocationSplitValues["dbfile"] : ""; // represent airport to find in between distances
  const std::string nm_s                 = (Utils::isElementExists (inMapLocationSplitValues, "nm")) ? inMapLocationSplitValues["nm"] : ""; // represent airport to find in between distances
  const std::string keyID_s              = (Utils::isElementExists (inMapLocationSplitValues, "keyid")) ? inMapLocationSplitValues["keyid"] : "icao"; // represent ID of the port, like ICAO or FAA
  const std::string keyname_s            = (Utils::isElementExists (inMapLocationSplitValues, "keyname")) ? inMapLocationSplitValues["keyname"] : "name"; // the key value that represents the name
  const std::string keydesc_s            = (Utils::isElementExists (inMapLocationSplitValues, "keydesc")) ? inMapLocationSplitValues["keydesc"] : "amenity"; // the key value that represent description, like amanity
  const std::string designer_desc_s      = (Utils::isElementExists (inMapLocationSplitValues, "desc")) ? inMapLocationSplitValues["desc"] : ""; // Free string define designer description. Should be short.
  const std::string designer_descforce_s = (Utils::isElementExists (inMapLocationSplitValues, "descforce")) ? inMapLocationSplitValues["descforce"] : ""; // Free string define designer description. Should be short.


#ifndef RELEASE
  Log::logMsgThread ("[osm_get_navaid] dbfile: " + dbfile + ", nm_s: " + nm_s + ", keyname_s: " + keyname_s + ", keydesc_s: " + keydesc_s + ", designer_desc_s: [" + designer_desc_s + "], designer_descforce_s: [" + designer_descforce_s + "]");
#endif


  //// validate min/max lat/lon are correct
  const auto lmbda_fix_order_of_min_max = [&] ()
  {
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
  RandomEngine::gather_all_osm_db_files_names_and_path (list_of_osm_db_files);

  // fetch all files
  if (!list_of_osm_db_files.empty ()) // we filter by the file extension ".db"
  {
    // loop over all files in folder with extension ".db" and try to open database. Try to create table bounds and only then try to read the bound table. If success then check bounds, if failure then skip to next file
    // stop loop once we found the relevant DB.
    for (const auto &file : list_of_osm_db_files)
    {
      missionx::dbase osm_db;

      // const auto db_file = working_folder + file;
      if (file.find (mxconst::get_DB_AIRPORTS_XP ()) != std::string::npos) // skip airports database since it is not OSM
        continue;

      if (missionx::data_manager::db_connect (osm_db, file))
      {
        Log::logMsg ("[random osm] Connected to: " + file + ". Next, try to create bounds table", true);
        osm_db.execute_stmt (missionx::data_manager::mapQueries["create_bounds"]); // we do not check the output

        if (osm_db.prepareNewStatement ("get_tested_bounds", missionx::data_manager::mapQueries["get_tested_bounds"]))
        {
          assert (osm_db.mapStatements["get_tested_bounds"] != nullptr);
          // bind parameters
          osm_db.bind_to_stored_stmt ("get_tested_bounds", missionx::db_types::real_typ, 1, Utils::formatNumber<double> (min_lat, 8));
          osm_db.bind_to_stored_stmt ("get_tested_bounds", missionx::db_types::real_typ, 2, Utils::formatNumber<double> (max_lat, 8));
          osm_db.bind_to_stored_stmt ("get_tested_bounds", missionx::db_types::real_typ, 3, Utils::formatNumber<double> (min_lon, 8));
          osm_db.bind_to_stored_stmt ("get_tested_bounds", missionx::db_types::real_typ, 4, Utils::formatNumber<double> (max_lon, 8));

          bool flag_bounds_are_ok = false;
          int  rc                 = osm_db.step (osm_db.mapStatements["get_tested_bounds"]);
          if (rc == SQLITE_ROW)
          {
            double t_min_lat_d     = sqlite3_column_double (osm_db.mapStatements["get_tested_bounds"], 0);
            double t_max_lat_d     = sqlite3_column_double (osm_db.mapStatements["get_tested_bounds"], 1);
            double t_min_lon_d     = sqlite3_column_double (osm_db.mapStatements["get_tested_bounds"], 2);
            double t_max_lon_d     = sqlite3_column_double (osm_db.mapStatements["get_tested_bounds"], 3);
            int    iMin_lat_result = sqlite3_column_int (osm_db.mapStatements["get_tested_bounds"], 4);
            int    iMax_lat_result = sqlite3_column_int (osm_db.mapStatements["get_tested_bounds"], 5);
            int    iMin_lon_result = sqlite3_column_int (osm_db.mapStatements["get_tested_bounds"], 6);
            int    iMax_lon_result = sqlite3_column_int (osm_db.mapStatements["get_tested_bounds"], 7);

            int count_ones = iMin_lat_result + iMax_lat_result + iMin_lon_result + iMax_lon_result;

            if (!dbfile.empty () && file.find_last_of (dbfile) != std::string::npos)
            { // check if the path holds the dbfilename in it and force using it or skip
              count_ones         = 4; // force OK state so plugin will use the
              flag_bounds_are_ok = true;

              if (nm_s == "0")
              { // force max region lat/lon
                min_lat = t_min_lat_d;
                max_lat = t_max_lat_d;
                min_lon = t_min_lon_d;
                max_lon = t_max_lon_d;

                // recalculate max distance
                if (const double maxDistance = Utils::calcDistanceBetween2Points_nm (min_lat, min_lon, max_lat, max_lon); maxDistance > maxDistance_d)
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

            lmbda_fix_order_of_min_max (); // just in case

          } // end if we have row from bounds


          // v3.0.241.10 b2 In this part of the code, we try to build a dynamic query based on the type of the expected location and the filtering values the designer provided.
          // If the designer did not provide filtering values, then we fall back to the original query.
          const auto lmbda_get_designer_query_filter = [&] (const std::string &inLocType, const std::string &in_nm_s)
          {
            std::list<std::string> list_designer_filter;
            std::string            query_filter;

            if (inLocType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM ())
            {

              // this is the second version of this implementation, it will use a free text (not CDATA) to get designer filter
              if (mxUtils::isElementExists (inMapLocationSplitValues, "tag"))
              {
                const std::string root_filter_tag = inMapLocationSplitValues["tag"];
                if (!RandomEngine::xRootTemplate.getChildNode (root_filter_tag.c_str ()).isEmpty ()) // if the "tag" string exists in template pick a random sub element from it
                {
                  IXMLNode    parent = RandomEngine::xRootTemplate.getChildNode (root_filter_tag.c_str ());
                  IXMLNode    node   = Utils::xml_get_node_randomly_by_name_IXMLNode (parent, "");
                  if (!node.isEmpty ())
                  {
                    query_filter = node.getText ();
                    if (in_nm_s == mxconst::get_ZERO ()) // v3.0.241.10.2 skip distance query, we want any location on the map
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

            const std::string filter_s = lmbda_get_designer_query_filter (inLocationType, nm_s);

            if (filter_s.empty ()) // fallback query, which is the default query
              query_name = "get_way_ids_in_area";
            else
              query_name = "get_designer_way_ids_part1";

            // v3.0.241.10 b2 add the designer location_values directive to the query:
            std::string query = missionx::data_manager::mapQueries[query_name] + filter_s;


            #ifndef RELEASE
            Log::logMsgThread ("[random get osm navaid] query: " + query + "\n");
            Log::logMsgThread ("[random get osm navaid] source location lat/lon: " + Utils::formatNumber<double> (sourceLat_d, 8) + "/" + Utils::formatNumber<double> (sourceLon_d, 8) + "\n");
            #endif

            if (osm_db.prepareNewStatement (query_name, query))
            {
              int i = 1;
              // calculate distance base on plane position
              // New code uses sqlite external function "mx_calc_distance" which only needs two values, source_lat and source_lon
              // mx_calc_distance( ?1, ?2, t1.lat, t1.lon, 3440)
              sqlite3_bind_double (osm_db.mapStatements[query_name], i, sourceLat_d);
              i++; // 1 source lat
              sqlite3_bind_double (osm_db.mapStatements[query_name], i, sourceLon_d);
              i++; // 2 source lon


              // area to search
              sqlite3_bind_double (osm_db.mapStatements[query_name], i, min_lat);
              i++; // 3
              sqlite3_bind_double (osm_db.mapStatements[query_name], i, max_lat);
              i++; // 4
              sqlite3_bind_double (osm_db.mapStatements[query_name], i, min_lon);
              i++; // 5
              sqlite3_bind_double (osm_db.mapStatements[query_name], i, max_lon);
              i++; // 6
              // added distance filtering
              sqlite3_bind_double (osm_db.mapStatements[query_name], i, minDistance_d);
              i++; // 7
              sqlite3_bind_double (osm_db.mapStatements[query_name], i, maxDistance_d);
              i++; // 8

              std::map<int, int> map_ids_in_area;
              int                seq = 1;
              while (sqlite3_step (osm_db.mapStatements[query_name]) == SQLITE_ROW)
              {
                map_ids_in_area[seq] = (sqlite3_column_int (osm_db.mapStatements[query_name], 0));
                ++seq;
              }

              if (map_ids_in_area.empty ())
              {
                Log::logMsgThread ("[osm getNavaid] No points relative to leg position were found in file: " + file + "! location lat/lon: " + Utils::formatNumber<double> (sourceLat_d, 8) + "/" + Utils::formatNumber<double> (sourceLon_d, 8) + "\n");
                flag_found_navaid_from_osm = false;
              }
              else
              {
                int map_size_i = static_cast<int> (map_ids_in_area.size ());

                auto iRandom = Utils::getRandomIntNumber (1, map_size_i);

                const auto lmbda_random_bounds_rules = [&] (const int inSize)
                {
                  if (iRandom > inSize)
                    return inSize - 1;
                  else if (iRandom < 1 && inSize > 0)
                    return 1;

                  return iRandom;
                };

                iRandom = lmbda_random_bounds_rules (static_cast<int> (map_ids_in_area.size ()));

                auto id_to_pick_i = map_ids_in_area[iRandom];

                if (osm_db.prepareNewStatement ("get_way_tag_data_by_id", missionx::data_manager::mapQueries["get_way_tag_data_by_id"]))
                {
                  sqlite3_bind_int (osm_db.mapStatements["get_way_tag_data_by_id"], 1, id_to_pick_i);

                  flag_found_navaid_from_osm = false;

                  while (osm_db.step (osm_db.mapStatements["get_way_tag_data_by_id"]) == SQLITE_ROW)
                  {
                    const auto key   = std::string (reinterpret_cast<const char *> (sqlite3_column_text (osm_db.mapStatements["get_way_tag_data_by_id"], 1)));
                    const auto value = std::string (reinterpret_cast<const char *> (sqlite3_column_text (osm_db.mapStatements["get_way_tag_data_by_id"], 2)));


                    if (key == keyname_s || key == "name") // There is duplications but it provides safety net
                    {
                      if (!value.empty ())
                        outNavAid.setName (value);
                    }
                    else if (key == keyID_s || (key == "faa") || (key == "icao")) // default: "icao", in US I found faa instead of icao. There is duplications but it provides safety net
                    {
                      outNavAid.setID (value);
                    }
                    else if (key == "name_desc" && outNavAid.getName ().empty ())
                    {
                      if (!value.empty ())
                        outNavAid.setName (value);
                    }
                    else if (key == "loc_name")
                    {
                      if (outNavAid.getName ().empty () && !value.empty ())
                        outNavAid.setName (value);

                      if (outNavAid.loc_desc.empty ())
                        outNavAid.loc_desc = value;
                    }
                    else if ((key == "description"))
                    {
                      if (outNavAid.getName ().empty ())
                        outNavAid.setName (value);

                      if (!value.empty ())
                        outNavAid.loc_desc = value;
                    }
                    else if (key == keydesc_s || key == "amenity") // There is duplications but it provides safety net
                    {
                      if (outNavAid.loc_desc.empty ())
                        outNavAid.loc_desc = value;
                    }

                    flag_found_navaid_from_osm = true;
                  }

                  if (flag_found_navaid_from_osm && osm_db.prepareNewStatement ("get_segments_in_way_id", missionx::data_manager::mapQueries["get_segments_in_way_id"]))
                  {
                    assert (osm_db.mapStatements["get_segments_in_way_id"]);

                    // add the designer description to the specific picked location only if segment statement is valid.
                    if (!designer_descforce_s.empty ())
                      outNavAid.loc_desc = designer_desc_s;
                    // else if (!designer_desc_s.empty() && outNavAid.loc_desc.empty())
                    else if (outNavAid.loc_desc.empty ())
                      outNavAid.loc_desc = designer_desc_s;

                    flag_found_navaid_from_osm = false; // we prepared the data, but now we need to pick valid osm segment. This means "function" is success only after storing lat/lon in "outNavAid"

                    sqlite3_bind_int (osm_db.mapStatements["get_segments_in_way_id"], 1, id_to_pick_i);
                    std::map<int, Point> mapPointSegments;
                    seq = 1;
                    while (osm_db.step (osm_db.mapStatements["get_segments_in_way_id"]) == SQLITE_ROW)
                    {
                      Point p;
                      p.lat = sqlite3_column_double (osm_db.mapStatements["get_segments_in_way_id"], 0);
                      p.lon = sqlite3_column_double (osm_db.mapStatements["get_segments_in_way_id"], 1);
                      p.storeDataToPointNode ();
                      Utils::addElementToMap (mapPointSegments, seq, p);

                      ++seq;
                    } // end loop over all segments


                    // pick one point and initial
                    iRandom = Utils::getRandomIntNumber (1, static_cast<int> (mapPointSegments.size ()));
                    iRandom = lmbda_random_bounds_rules (static_cast<int> (mapPointSegments.size ()));

                    if (Utils::isElementExists (mapPointSegments, iRandom))
                    {
                      outNavAid.lat              = static_cast<float> (mapPointSegments[iRandom].lat);
                      outNavAid.lon              = static_cast<float> (mapPointSegments[iRandom].lon);
                      flag_found_navaid_from_osm = true;

#ifndef RELEASE
                      Log::logMsgThread ("\n[osm getNavaid] Picked osm location: " + outNavAid.get_latLon_name () + "\n\n");
#endif
                    }
                  } // end get_segments_in_way_id


                } // end get_way_tag_data_by_id

              } // end we have values in map_ids_in_area

            } // end if the prepared statement succeeded
          }
          else
          {
            // Log::logMsgThread("\n[osm getNavaid] osm file: '" + file + "' is not in the boundaries: {minLat/minLon: " + Utils::formatNumber<double>(min_lat, 8) + ", " + Utils::formatNumber<double>(min_lon, 8) + "} and {maxLat/maxLon: " + Utils::formatNumber<double>(max_lat, 8) + ", " + Utils::formatNumber<double>(max_lon, 8) + "}");
            Log::logMsgThread (fmt::format ("\n[osm getNavaid] osm file: '{}' is not in the boundaries: [minLat/minLon: {}, {}] and [maxLat/maxLon: {}, {}]", file, Utils::formatNumber<double> (min_lat, 8), Utils::formatNumber<double> (min_lon, 8), Utils::formatNumber<double> (max_lat, 8), Utils::formatNumber<double> (max_lon, 8)));
          }

          osm_db.clear_and_reset (osm_db.mapStatements["get_tested_bounds"]);

          if (flag_found_navaid_from_osm) // break if found legit segment
            break;

        } // end get_tested_bounds
        else
        {
          Log::logMsgErr (std::string ("[random decide osm] ") + sqlite3_errmsg (osm_db.db), true);
        }

      } // if connect to osm sqlite file succeed

      osm_db.close_database (); // close database

    } // end loop over mamp of files

  } // end list all files

  return flag_found_navaid_from_osm;

} // end osm_get_navaid_from_osm

// -----------------------------------

void
RandomEngine::initQueries ()
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
RandomEngine::check_if_new_target_is_same_as_prev (missionx::NavAidInfo &inCurrentTargetNav, missionx::NavAidInfo &inPrevNav)
{
  return (inCurrentTargetNav.getID () == inPrevNav.getID ()) && (inCurrentTargetNav.getName () == inPrevNav.getName ()
         && ( ! inCurrentTargetNav.getID().empty() + ! inCurrentTargetNav.getName().empty () ) // v25.06.1 added empty test
         );
}

// -----------------------------------


bool
RandomEngine::check_last_2_legs_if_they_have_same_icao ()
{
  const auto                      size_i = static_cast<int> (RandomEngine::listNavInfo.size ());
  std::list<missionx::NavAidInfo> listNavInfo2 (RandomEngine::listNavInfo.begin (), RandomEngine::listNavInfo.end ());

  if (size_i > 1)
  {
    // get last and 1 before last
    auto last = listNavInfo2.back ();
    listNavInfo2.pop_back ();
    auto preLast = listNavInfo2.back ();

    if ( !(last.getID ().empty ()) && last.getID () == preLast.getID () ) // v25.06.1 added last.getID ().empty () so it test empty content
    {
      RandomEngine::listNavInfo.pop_back (); // pop out the last NavInfo since it and the one before it are at the same ICAO
      #ifndef RELEASE
      Log::logMsgThread ("Removed duplicate last two ICAO: " + last.getID ());
      #endif // !RELEASE
      return false;
    }
  }
  return true;
} // check_last_2_legs



// -----------------------------------

std::string
RandomEngine::get_short_flight_description_from_to (const std::string &inFromName, const std::string &inFromICAO, const std::string &inToName, const std::string &inToICAO)
{
  if (inFromName == mxconst::get_ELEMENT_BRIEFER ())
    return fmt::format (R"(From "{}" to "{} [{}]")", inFromName, inToName, inToICAO);

  return fmt::format (R"(From "{} [{}]" to "{} [{}]")", inFromName, inFromICAO, inToName, inToICAO);
}

// -----------------------------------

std::vector<IXMLNode>
RandomEngine::gen_land_hover_display_objects (const double &inLat, const double &inLon, const int &inRadiusMT, const int &inHowManyObjects, int &inout_seq, const std::string &inFileName)
{
  // 1. validate filename is not empty.
  // 2. validate inRadiusMT and inHowManyObjects are valid.
  // 3. calculate new locations relative to the "target" position.
  std::vector<IXMLNode> vec_3d_display_objects;
  const auto            display_object_node = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_DISPLAY_OBJECT ());
  // basic validation
  if (inFileName.empty () + (inHowManyObjects < 4) + (inRadiusMT < 1) + display_object_node.isEmpty ())
  {
    Log::logMsgThread (fmt::format ("Check values sent to {} function.", __func__));
    return vec_3d_display_objects;
  }

  const auto        angle_f             = 360.0f / (static_cast<float> (inHowManyObjects));
  const std::string obj_template_name_s = "land_hover_marker";

  for (int i1 = 0; i1 < inHowManyObjects; ++i1)
  {
    const auto        bearing         = angle_f * (static_cast<float> (i1));
    auto              xDisplayObj     = display_object_node.deepCopy ();
    const std::string instance_name_s = fmt::format ("land_hover_hint_{}", ++inout_seq);

    // calculate and set the <display_object>
    double target_lat, target_lon;
    Point::mxCalcPointBasedOnDistanceAndBearing_2DPlane (target_lat, target_lon, inLat, inLon, bearing, inRadiusMT);

    // set up the <display_object>
    Utils::xml_set_attribute_in_node_asString (xDisplayObj, mxconst::get_ATTRIB_NAME (), obj_template_name_s, mxconst::get_ELEMENT_DISPLAY_OBJECT ());
    Utils::xml_set_attribute_in_node_asString (xDisplayObj, mxconst::get_ATTRIB_INSTANCE_NAME (), instance_name_s, mxconst::get_ELEMENT_DISPLAY_OBJECT ());
    Utils::xml_set_attribute_in_node_asString (xDisplayObj, mxconst::get_ATTRIB_TARGET_MARKER_B (), "true", mxconst::get_ELEMENT_DISPLAY_OBJECT ());
    Utils::xml_set_attribute_in_node<double> (xDisplayObj, mxconst::get_ATTRIB_REPLACE_LAT (), target_lat, mxconst::get_ELEMENT_DISPLAY_OBJECT ());
    Utils::xml_set_attribute_in_node<double> (xDisplayObj, mxconst::get_ATTRIB_REPLACE_LONG (), target_lon, mxconst::get_ELEMENT_DISPLAY_OBJECT ());
    Utils::xml_set_attribute_in_node<int> (xDisplayObj, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT (), 10, mxconst::get_ELEMENT_DISPLAY_OBJECT ());

    vec_3d_display_objects.push_back (xDisplayObj);

    #ifndef RELEASE
    // Utils::xml_print_node (xDisplayObj, true);
    #endif
  }

  return vec_3d_display_objects;
}

// -----------------------------------

bool
RandomEngine::get_isNavAidInValidDistance (const double &currentDistanceToTarget, const double &in_location_value_d, const double &in_location_minDistance_d, const double &in_location_maxDistance_d)
{
  if (in_location_value_d > 0.0 && currentDistanceToTarget <= in_location_value_d) // location_value_d represents "nm", It has precedence over min/max
    return true;
  else if (in_location_minDistance_d >= 0.0 && in_location_maxDistance_d > in_location_minDistance_d) // check if between min and max values
    return (currentDistanceToTarget >= in_location_minDistance_d && currentDistanceToTarget <= in_location_maxDistance_d);

  return currentDistanceToTarget > static_cast<double> (mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT); // accept distance if in the limit of search airport
}

// -----------------------------------

bool
RandomEngine::get_target_base_on_tag_name (NavAidInfo                   &outNewNavInfo,
                                        mx_plane_types                in_plane_type_enum,
                                        const missionx::mx_base_node &inProperties, // v3.305.1
                                        const std::string            &location_value_tag_name_s,
                                        const double                  location_value_d,
                                        double                        location_minDistance_d,
                                        double                        location_maxDistance_d)
{
  // v3.0.241.7 // v3.0.241.8 added this->flag_force_template_distances_b to let designer force his "narrative" when it comes to distances.
  const bool        flag_override_random_target_min_dist = (this->flag_force_template_distances_b) ? false : missionx::system_actions::pluginSetupOptions.getBoolValue (mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE ());
  const std::string inFlightLegName                      = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_NAME (), "");
  const std::string inTemplateType                       = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_TYPE (), "");
  const std::string inLocationType                       = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), "");
  const bool        flag_force_template_type             = Utils::readBoolAttrib (inProperties.node, mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B (), false);

  IXMLNode xPoint = IXMLNode::emptyIXMLNode; // local xml <point> element representative.

  IXMLNode rNode = missionx::RandomEngine::xRootTemplate.getChildNode (location_value_tag_name_s.c_str ()).deepCopy ();
  if (rNode.isEmpty ())
  {
    setError ("[random get_target rNode] fail to find random pick element. Please fix your template. skipping flight leg: " + inFlightLegName);
    return false;
  }

  RandomEngine::shared_navaid_info.init ();
  RandomEngine::shared_navaid_info.parentNode_ptr = rNode; // store pointer to XML node
  missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::convert_icao_to_xml_point); // will call missionx::flcPRE() and try to convert any <icao name="icao name" /> to <point targetLat="" targetLon="" />

  // NEAR - do we need to find the nearest location ?
  if (inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_NEAR ())
  {
    // Find Nearest NavAid based on given points
    // loop over all points and pick the one that is NEAREST to current point.
    if (lastFlightLegNavInfo.lat != 0 && lastFlightLegNavInfo.lon != 0)
    {
      double last_shortest_dist = mxconst::INT_UNDEFINED;

      const int nChilds = rNode.nChildNode (mxconst::get_ELEMENT_POINT ().c_str ());
      for (int i1 = 0; i1 < nChilds; ++i1)
      {
        IXMLNode cNode = rNode.getChildNode (mxconst::get_ELEMENT_POINT ().c_str (), i1);
        if (cNode.isEmpty ())
          continue;

        missionx::NavAidInfo ni;
        ni.lat      = static_cast<float> (Utils::readNumericAttrib (cNode, mxconst::get_ATTRIB_LAT (), 0.0));
        ni.lon      = static_cast<float> (Utils::readNumericAttrib (cNode, mxconst::get_ATTRIB_LONG (), 0.0));
        ni.loc_desc = Utils::readAttrib (cNode, mxconst::get_ATTRIB_LOC_DESC (), EMPTY_STRING);

        if (ni.lat == 0.0 || ni.lon == 0.0) // skipping if one of the values = 0
          continue;

        const double distance = Utils::calcDistanceBetween2Points_nm (lastFlightLegNavInfo.lat, lastFlightLegNavInfo.lon, ni.lat, ni.lon);

        if (missionx::RandomEngine::get_isNavAidInValidDistance (distance, location_value_d, location_minDistance_d, location_maxDistance_d)) // v3.0.255.4.1 add "nm" and "nm_between" rules
        {
#ifndef RELEASE
          Log::logMsgThread ("[get target based tag] Target: " + ni.loc_desc + " is in a valid distance: " + mxUtils::formatNumber<double> (distance));
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
          // Log::logMsgThread("[get target based tag] Target: " + ni.loc_desc + ", invalid distance: " + mxUtils::formatNumber<double>(distance) + ", Should be nm_between: " + mxUtils::formatNumber<double>(location_minDistance_d) + "-" +
          //                   mxUtils::formatNumber<double>(location_maxDistance_d) + ((location_value_d > 0.0) ? ", or nm: " + mxUtils::formatNumber<double>(location_value_d) : "")); // debug
          Log::logMsgThread (fmt::format ("[get target based tag] Target: {}, invalid distance: {}, Should be nm_between: {:.2f}-{:.2f} {}", ni.loc_desc, distance, location_minDistance_d, location_maxDistance_d, (location_value_d > 0.0) ? fmt::format (", or nm: {:.2f}", location_value_d) : "")); // debug
        }
#endif // !RELEASE
      }
      // v3.0.241.10 b3 missing decision if we found valid: "outNewNavInfo"
      outNewNavInfo.synchToPoint ();
      if (outNewNavInfo.lat != 0.0 && outNewNavInfo.lon != 0.0)
        return true;
    }

  } // end handling pick <points> from <tag name>
  else
  { // pick any <point> from element (not "near" location type)

#ifndef RELEASE
    int nPointChilds = 0;
    nPointChilds     = rNode.nChildNode (mxconst::get_ELEMENT_POINT ().c_str ());
    Log::logMsgThread ("[Pick Point after convert]<point> number: " + Utils::formatNumber<int> (nPointChilds));
    Log::logMsgThread ("[Pick Point] force flight leg template: " + std::string ((flag_force_template_type) ? "yes" : "no") + "\n"); // v3.0.221.15 rc3
#endif

    int loop_counter_i = 0; // we will use this to strict the loop to no more than 2
    do
    {
      ++loop_counter_i;
      if ((location_value_d > 0.0 && !flag_override_random_target_min_dist) || (location_value_d > 0.0 && loop_counter_i > 1)) // this fallback will kick in only if user did not modify "expected location" or if we did not find and point in the first run
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

        IXMLNode rnd_x_point = Utils::xml_get_node_randomly_by_name_and_distance_IXMLNode (rNode, mxconst::get_ELEMENT_POINT (), RandomEngine::lastFlightLegNavInfo.lat, RandomEngine::lastFlightLegNavInfo.lon, err, location_minDistance_d, location_maxDistance_d, true); // remove picked point


        if (!err.empty () || rnd_x_point.isEmpty ())
        {
          RandomEngine::setError ("[get_target from tag_name] " + ((err.empty () && rnd_x_point.isEmpty ()) ? "No more valid points found in tag: " + location_value_tag_name_s + ", meaning, no points are left." : err));
          return false;
        }

        xPoint = rnd_x_point.deepCopy ();

        // check if we need to pick a point that is same as the template type
        if (!flag_force_template_type)
        {
          flag_searchAnotherPoint = false;
          break;
        }

        rnd_x_point.deleteNodeContent (); // decrease the number of points to pick from
        rnd_x_point = IXMLNode::emptyIXMLNode; // v3.0.241.1

        // check if template type is different from xPoint type. We will have to check template point attribute + isWet and slope
        std::string pointTemplate = Utils::stringToLower (Utils::readAttrib (xPoint, mxconst::get_ATTRIB_TEMPLATE (), EMPTY_STRING));

        // we want to test if the picked location template type might be changed due to slope or water body location or if point template is different from given leg_type
        if ((mxconst::get_FL_TEMPLATE_VAL_LAND () == inTemplateType) || (mxconst::get_FL_TEMPLATE_VAL_HOVER () == inTemplateType))
        {
          NavAidInfo nav;
          nav.node = xPoint.deepCopy ();
          nav.syncXmlPointToNav ();

          if (pointTemplate.empty () && !flag_force_template_type)
          {
            double slope = 0.0;
            // check template type will change because of water or slope info, this is relevant only for LAND template type
            #ifndef RELEASE
            Log::logMsgNone ("[get_target] Test if probing target point will change \"flight leg template type\": " + inTemplateType, true);
            #endif
            // small optimization. Moved the slope + isWet code only when it is relevant
            // slope = this->getSlope(nav); // v3.0.253.7 deprecated getSlope() function - duplicate functions
            slope = this->get_slope_at_point (nav);
            if (slope > missionx::data_manager::Max_Slope_To_Land_On && (mxconst::get_FL_TEMPLATE_VAL_LAND () == inTemplateType))
            {
              Log::logDebugBO ("[get_target slope] point has slope: " + Utils::formatNumber<double> (slope, 2) + " is not suitable for template type: " + inTemplateType + "\n", true);
              flag_searchAnotherPoint = true;
            }
            else if (slope <= missionx::data_manager::Max_Slope_To_Land_On && (mxconst::get_FL_TEMPLATE_VAL_HOVER () == inTemplateType))
            {
              Log::logDebugBO ("[get_target slope] point has slope: " + Utils::formatNumber<double> (slope, 2) + " is not suitable for template type: " + inTemplateType + " since plane should be able to land.\n", true);

              flag_searchAnotherPoint = true;
            }


            #ifndef RELEASE
            if (flag_searchAnotherPoint)
              Log::logMsg ("[get_target slope] point slope should change the target \"<leg>\" type. Will try to pick other <point> from tag.", true);
            else
              Log::logMsg ("[get_target slope] point slope will not change target \"<leg>\" type. Will check if target falls in water body.", true);
            #endif


            // check WET only if slope is fine
            if (!flag_searchAnotherPoint)
            {
              bool isWet = false;
              isWet      = this->get_is_wet_at_point (nav); // v3.0.253.7
              if (isWet && (mxconst::get_FL_TEMPLATE_VAL_LAND () == inTemplateType))
              {
                Log::logDebugBO ("\t[get_target water body] point is in water body and not suitable for flight leg type: " + inTemplateType + "\n", true);

                flag_searchAnotherPoint = true;
              }
              else if (!isWet && slope <= missionx::data_manager::Max_Slope_To_Land_On && (mxconst::get_FL_TEMPLATE_VAL_HOVER () == inTemplateType))
              {
                Log::logDebugBO ("\t[get_target water body] point is NOT in water body and therefore we should be able to land which might not be suitable for template type: " + inTemplateType + "\n", true);

                flag_searchAnotherPoint = true;
              }

#ifndef RELEASE
              if (flag_searchAnotherPoint)
                Log::logMsg ("\t[get_target wet] point template type will be changed after testing water body.", true);
              else
                Log::logMsg ("\t[get_target wet] point won't change its template type after testing water body.", true);
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
              Log::logMsg ("\t[get_target same template test] point template is different than flight leg template. Will have to pick another point.", true);
            else
              Log::logMsg ("\t[get_target same template test] point template is same as flight leg template. Target should be valid.", true);
#endif
          }

          if (flag_searchAnotherPoint)
            xPoint = IXMLNode::emptyIXMLNode;
        }
        else
          flag_searchAnotherPoint = false; // exit while loop

#ifndef RELEASE
        nPointChilds = rNode.nChildNode (mxconst::get_ELEMENT_POINT ().c_str ());
        Log::logMsg ("\t[get_target points] After point randomly picked. No. of points left: " + Utils::formatNumber<int> (nPointChilds) + "\n", true);
#endif
      } while (flag_searchAnotherPoint); // end picking a point from pre-defined location or the nearest one


    } while (xPoint.isEmpty () && flag_override_random_target_min_dist && loop_counter_i < 2); // Max loop will be the second time when we will use developer values

    if (!xPoint.isEmpty ())
    {
#ifndef RELEASE
      Log::logMsgThread (">>>>>>>>> xPoint: " + Utils::xml_get_node_content_as_text (xPoint));
#endif

      // end handling all template and location_type
      outNewNavInfo.node = xPoint.deepCopy ();
      outNewNavInfo.syncXmlPointToNav ();
      if (flag_force_template_type)
        outNewNavInfo.flag_force_picked_same_point_template_as_flight_leg_template_type = true;

      return true;
    }
  } // and if near or other template type

  return false;
}

// -----------------------------------


bool
RandomEngine::gen_target_base_on_xy_osm_or_osmweb_types (NavAidInfo &outNewNavInfo
                                                       , mx_plane_types in_plane_type_enum
                                                       , std::map<std::string, std::string> &inMapLocationSplitValues
                                                       , missionx::mx_base_node &inProperties
                                                       , NavAidInfo *prev_na_ptr)
{
  // pick random location. Use the location_value_nm_s as our radius length in nautical miles.
  // Pick random number between 1 and location_value_nm_s (if location value is less than 1 then we will override it with 10nm).
  // Pick random number between 0 and 355
  // Using Utils:: we will get the new location

  assert ( prev_na_ptr != nullptr && fmt::format("[{}] Previous Navigation Aid is mandatory for this function.", __func__).c_str ()); // debug

  if (prev_na_ptr == nullptr)
  {
    outNewNavInfo.init ();
    outNewNavInfo.err = fmt::format ("[{}] Previous navigation data is not accessible.", __func__);
    return false;
  }

  auto       location_value_d        = inProperties.getAttribNumericValue<double> ("location_value_d", -1.0);
  auto       location_min_distance_d = inProperties.getAttribNumericValue<double> ("location_min_distance_d", location_value_d);
  const auto location_max_distance_d = inProperties.getAttribNumericValue<double> ("location_max_distance_d", location_value_d);


  const bool        flag_override_random_target_min_dist = (missionx::RandomEngine::flag_force_template_distances_b) ? false : missionx::system_actions::pluginSetupOptions.getBoolValue (mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE ()); // this->flag_force_template_distances_b to let designer force their "narative" when it comes to distances.
  const std::string inLocationType                       = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), "");

  // Prepare distance to target
  // location_value_d has precedence over nm_between, (v3.0.241.8) unless we defined it in the setup flag_override_random_target_min_dist. we will use nm_between if location_value_d is smaller than 1.0nm
  double nm_random_distance_d         = 1.5;
  double nm_max_distance_osm_radius_d = 0.0; // v3.0.241.10 will hold the expected max radius nm value for OSM based legs
  if ((location_value_d <= 1.0 && (location_min_distance_d > 0.0 && location_max_distance_d > 0.0)) || (flag_override_random_target_min_dist && location_min_distance_d > 0.0 && location_max_distance_d > 0.0)) // v3.0.241.8 added setup flag hint
  {
    // v3.0.241.8 respecting the location_value_d defined by the designer as the min radius distance even the user preferred a higher value
    // It should balance between what the designer believe is best and what user wants. Destination should be between "designer" and "user"
    if (location_value_d < location_min_distance_d && location_value_d > 1.0)
      location_min_distance_d = location_value_d;

    nm_max_distance_osm_radius_d = location_max_distance_d;
    nm_random_distance_d         = Utils::getRandomRealNumber (location_min_distance_d, location_max_distance_d);

    #ifndef RELEASE
    Log::logDebugBO ("[DEBUG get_target] location: " + inLocationType + ", location_minDistance_d: " + Utils::formatNumber<double> (location_min_distance_d, 2) + ", location_maxDistance_d: " + Utils::formatNumber<double> (location_max_distance_d, 2), true);
    #endif
  }
  else
  {
    location_value_d             = (location_value_d <= 1.0) ? 10.0 : location_value_d; // we do not need to handle flag_override_random_target_min_dist since it should have been dealt in the above "if" statement
    nm_max_distance_osm_radius_d = location_value_d;
    nm_random_distance_d         = Utils::getRandomRealNumber (1, location_value_d);

    #ifndef RELEASE
    Log::logDebugBO ("[DEBUG get_target] location: " + inLocationType + ", location_value_nm_s: " + Utils::formatNumber<double> (location_value_d, 2), true);
    #endif
  }


  if (inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM () || inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ())
  {
    // get max radius and find the 4 points that create the rectangle area
    // nm_max_distance_osm_radius_d = max Radius
    // location_minDistance_d = min radius distance
    #ifndef Release
    const double maxRadius_d = nm_max_distance_osm_radius_d;
    #endif

    Point E90, W270, S180, N0;
    RandomEngine::calculate_bbox_coordinates (N0, S180, E90, W270, prev_na_ptr->lat, prev_na_ptr->lon, maxRadius_d);

    if (NavAidInfo navAid; RandomEngine::osm_get_navaid_from_osm (navAid, inMapLocationSplitValues, inProperties, prev_na_ptr->lat, prev_na_ptr->lon, S180.lat, N0.lat, W270.lon, E90.lon, nm_max_distance_osm_radius_d, location_min_distance_d))
    {
      if (navAid.lat != 0.0 && navAid.lon != 0.0)
      {
        outNewNavInfo = navAid;
        outNewNavInfo.synchToPoint ();
        // RandomEngine::flag_picked_from_osm_database = true; // we can use this
        outNewNavInfo.flag_navDataFetchedFromDB = true; // v25.09.2
        return true;
      }
    }

    // if OSM data was not found then plugin will try to use the default target search
  }

  // if we wanted an XY location, or we failed to pick a location based on OSM data, then we will fall back to XY coordinate
  const auto heading_deg = static_cast<float> (Utils::getRandomIntNumber (0, 355));

  double lon;
  double lat = lon = 0.0;

  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (lat, lon, prev_na_ptr->lat, prev_na_ptr->lon, heading_deg, nm_random_distance_d);
  outNewNavInfo.lat      = static_cast<float> (lat);
  outNewNavInfo.lon      = static_cast<float> (lon);
  outNewNavInfo.heading  = heading_deg;
  outNewNavInfo.gen_locDesc_short (); // v25.09.2
  // outNewNavInfo.loc_desc = "Coordinates lat: " + outNewNavInfo.getLat () + ", lon: " + outNewNavInfo.getLon (); // DEPRECATED

  #ifndef RELEASE
  // Log::logDebugBO ("[DEBUG get_target] location: " + inLocationType + ", NavAid.name: " + outNewNavInfo.getNavAidName (), true);
  Log::logDebugBO ( fmt::format("[{}] location: {}, NavAid.name: {}", __func__, inLocationType, outNewNavInfo.getNavAidName ()), true);
  #endif

  // v25.09.2 deprecated, lets see what plugin will use as default
  // outNewNavInfo.setName (mxconst::get_COORDINATES_IN_THE_GPS_S ()); // v3.0.221.7 // v3.0.241.9 replaced string with constant since we use it in NavInfo
  outNewNavInfo.flag_picked_random_lat_long = true;
  outNewNavInfo.synchToPoint (true);

  #ifndef RELEASE
  Log::logDebugBO (fmt::format("[{}] location: {}", __func__, inLocationType), true) ;
  #endif

  return true;
}

// -----------------------------------


bool
RandomEngine::get_targetForHelos_base_XY_OSM_OSMWEB (NavAidInfo                         &outNewNavInfo,
                                                      mx_plane_types                      in_plane_type_enum,
                                                      std::map<std::string, std::string> &inMapLocationSplitValues,
                                                      missionx::mx_base_node             &inProperties)// v3.305.1
                                                      // double                              location_value_d,
                                                      // double                              location_min_distance_d,
                                                      // double                              location_max_distance_d)
{
  // v3.0.219.10
  // pick random location. Use the location_value_nm_s as our radius length in nautical miles.
  // Pick random number between 1 and location_value_nm_s (if location value is less than 1 then we will override it with 10nm).
  // Pick random number between 0 and 355
  // Using Utils:: we will get the new location
  // v3.0.254.3 added support for WEBOSM

  auto       location_value_d        = inProperties.getAttribNumericValue<double> ("location_value_d", -1.0);
  auto       location_min_distance_d = inProperties.getAttribNumericValue<double> ("location_min_distance_d", location_value_d);
  const auto location_max_distance_d = inProperties.getAttribNumericValue<double> ("location_max_distance_d", location_value_d);


  const bool        flag_override_random_target_min_dist = (missionx::RandomEngine::flag_force_template_distances_b) ? false : missionx::system_actions::pluginSetupOptions.getBoolValue (mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE ()); // this->flag_force_template_distances_b to let designer force their "narative" when it comes to distances.
  const std::string inLocationType                       = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), "");

  // Prepare distance to target
  // location_value_d has precedence over nm_between, (v3.0.241.8) unless we defined it in the setup flag_override_random_target_min_dist. we will use nm_between if location_value_d is smaller than 1.0nm
  double nm_random_distance_d         = 1.5;
  double nm_max_distance_osm_radius_d = 0.0; // v3.0.241.10 will hold the expected max radius nm value for OSM based legs
  if ((location_value_d <= 1.0 && (location_min_distance_d > 0.0 && location_max_distance_d > 0.0)) || (flag_override_random_target_min_dist && location_min_distance_d > 0.0 && location_max_distance_d > 0.0)) // v3.0.241.8 added setup flag hint
  {
    // v3.0.241.8 respecting the location_value_d defined by the designer as the min radius distance even the user preferred a higher value
    // It should balance between what the designer believe is best and what user wants. Destination should be between "designer" and "user"
    if (location_value_d < location_min_distance_d && location_value_d > 1.0)
      location_min_distance_d = location_value_d;

    nm_max_distance_osm_radius_d = location_max_distance_d;
    nm_random_distance_d         = Utils::getRandomRealNumber (location_min_distance_d, location_max_distance_d);

    #ifndef RELEASE
    Log::logDebugBO ("[DEBUG get_target] location: " + inLocationType + ", location_minDistance_d: " + Utils::formatNumber<double> (location_min_distance_d, 2) + ", location_maxDistance_d: " + Utils::formatNumber<double> (location_max_distance_d, 2), true);
    #endif
  }
  else
  {
    location_value_d             = (location_value_d <= 1.0) ? 10.0 : location_value_d; // we do not need to handle flag_override_random_target_min_dist since it should have been dealt in the above "if" statement
    nm_max_distance_osm_radius_d = location_value_d;
    nm_random_distance_d         = Utils::getRandomRealNumber (1, location_value_d);

    #ifndef RELEASE
    Log::logDebugBO ("[DEBUG get_target] location: " + inLocationType + ", location_value_nm_s: " + Utils::formatNumber<double> (location_value_d, 2), true);
    #endif
  }


  if (inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM () || inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ())
  {
    // get max radius and find the 4 points that create the rectangle area
    // nm_max_distance_osm_radius_d = max Radius
    // location_minDistance_d = min radius distance
    #ifndef Release
    const double maxRadius_d = nm_max_distance_osm_radius_d;
    #endif

    Point E90, W270, S180, N0;
    RandomEngine::calculate_bbox_coordinates (N0, S180, E90, W270, RandomEngine::lastFlightLegNavInfo.lat, RandomEngine::lastFlightLegNavInfo.lon, maxRadius_d);

    if (NavAidInfo navAid; RandomEngine::osm_get_navaid_from_osm (navAid, inMapLocationSplitValues, inProperties, RandomEngine::lastFlightLegNavInfo.lat, RandomEngine::lastFlightLegNavInfo.lon, S180.lat, N0.lat, W270.lon, E90.lon, nm_max_distance_osm_radius_d, location_min_distance_d))
    {
      if (navAid.lat != 0.0 && navAid.lon != 0.0)
      {
        outNewNavInfo = navAid;
        outNewNavInfo.synchToPoint ();
        // RandomEngine::flag_picked_from_osm_database = true; // we can use this
        outNewNavInfo.flag_navDataFetchedFromDB = true; // v25.09.2
        return true;
      }
    }

    // if OSM data was not found then plugin will try to use the default target search
  }

  // if we wanted an XY location, or we failed to pick a location based on OSM data, then we will fall back to XY coordinate
  const auto heading_deg = static_cast<float> (Utils::getRandomIntNumber (0, 355));

  double lon;
  double lat = lon = 0.0;

  Utils::calcPointBasedOnDistanceAndBearing_2DPlane (lat, lon, RandomEngine::lastFlightLegNavInfo.lat, RandomEngine::lastFlightLegNavInfo.lon, heading_deg, nm_random_distance_d);
  outNewNavInfo.lat      = static_cast<float> (lat);
  outNewNavInfo.lon      = static_cast<float> (lon);
  outNewNavInfo.heading  = heading_deg;
  outNewNavInfo.loc_desc = "Coordinates lat: " + outNewNavInfo.getLat () + ", lon: " + outNewNavInfo.getLon ();

  #ifndef RELEASE
  Log::logDebugBO ("[DEBUG get_target] location: " + inLocationType + ", NavAid.name: " + outNewNavInfo.getNavAidName (), true);
  #endif


  // #ifdef IBM
  outNewNavInfo.setName (mxconst::get_COORDINATES_IN_THE_GPS_S ()); // v3.0.221.7 // v3.0.241.9 replaced string with constant since we use it in NavInfo
  outNewNavInfo.flag_picked_random_lat_long = true;
  outNewNavInfo.synchToPoint ();

  ////// Information for Main Thread Job Request ///////////
  // v3.0.221.3 calculating slope in main callback and not RandomEngine Thread
  // TODO consider removing these lines since we might set them independently in the create flight leg main function
  // missionx::RandomEngine::threadState.pipeProperties.setNumberProperty(mxconst::get_ATTRIB_LAT(), outNewNavInfo.lat);
  // missionx::RandomEngine::threadState.pipeProperties.setNumberProperty(mxconst::get_ATTRIB_LONG(), outNewNavInfo.lon);

  #ifndef RELEASE
  Log::logDebugBO ("[DEBUG get_target] location: " + inLocationType + ", After slope decision", true);
  #endif

  return true;
}

bool
RandomEngine::gen_target_or_last_flight_leg_base_on_xy_or_osm (NavAidInfo &outNewNavInfo, mx_plane_types in_plane_type_enum, std::map<std::string, std::string> &inMapLocationSplitValues
                                                               , missionx::mx_base_node &inProperties, NavAidInfo *prev_na_ptr)
{

  assert ( prev_na_ptr != nullptr && fmt::format("[{}] Previous Navigation Aid is mandatory for this function.", __func__).c_str ()); // debug

  if (prev_na_ptr == nullptr)
  {
    outNewNavInfo.init ();
    outNewNavInfo.err = fmt::format ("[{}] Previous navigation data is not accessible.", __func__);
    return false;
  }

  const auto location_value_d        = inProperties.getAttribNumericValue<double> ("location_value_d", -1.0);
  const auto location_min_distance_d = inProperties.getAttribNumericValue<double> ("location_min_distance_d", location_value_d);
  const auto location_max_distance_d = inProperties.getAttribNumericValue<double> ("location_max_distance_d", location_value_d);

  // added this->flag_force_template_distances_b to let the designer force his "narrative" when it comes to "target" distances.
  const bool        flag_override_random_target_min_dist = (outNewNavInfo.fpln_expected_location_data.flag_force_template_distances_b) ? false : missionx::system_actions::pluginSetupOptions.getBoolValue (mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE ());
  const std::string target_flight_leg_name               = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_NAME (), "");
  const std::string target_location_type                 = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), "");

  // represent a ramp type
  const std::string location_value_restrict_ramp_type_s = mxUtils::getValueFromElement (inMapLocationSplitValues, std::string ("ramp"), std::string (""));

  // search NavAid location in radius
  // prepare shared thread data
  RandomEngine::shared_navaid_info.init ();
  RandomEngine::shared_navaid_info.p = prev_na_ptr->p;
  if (location_value_d > 0.0 && !flag_override_random_target_min_dist)
  {
    RandomEngine::shared_navaid_info.inMaxDistance_nm = static_cast<float> (location_value_d);
  }
  else
  {
    RandomEngine::shared_navaid_info.inMinDistance_nm = static_cast<float> (location_min_distance_d);
    RandomEngine::shared_navaid_info.inMaxDistance_nm = (location_max_distance_d > 0.0) ? static_cast<float> (location_max_distance_d) : mxconst::MAX_RAD_4_OSM_MAX_DIST; // v24.12.2 default distance if not set
  }


  // Search for HELOS last flight leg
  // OSM search first - this code will be used when there is a template or mission template with OSM information in it. It will probably won't be called from the user creation screen
  if ( (target_location_type == mxconst::get_EXPECTED_LOCATION_TYPE_OSM () || target_location_type == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ())
       && in_plane_type_enum == missionx::mx_plane_types::plane_type_helos)
  {
    Point E90, W270, S180, N0;

    // get max radius and find the 4 points that create the rectangle area
    // RandomEngine::shared_navaid_info.inMaxDistance_nm = max Radius
    // location_minDistance_d = min radius distance
    RandomEngine::calculate_bbox_coordinates (N0, S180, E90, W270, prev_na_ptr->lat, prev_na_ptr->lon, RandomEngine::shared_navaid_info.inMaxDistance_nm);
    if (NavAidInfo local_navAid;
       RandomEngine::osm_get_navaid_from_osm (local_navAid, inMapLocationSplitValues, inProperties, prev_na_ptr->lat, prev_na_ptr->lon, S180.lat, N0.lat, W270.lon, E90.lon, RandomEngine::shared_navaid_info.inMaxDistance_nm, location_min_distance_d))
    {
      if (local_navAid.lat != 0.0 && local_navAid.lon != 0.0)
      {
        ////// Test Final NavAid against X-Plane. We will check the closest Navaid to that location and it should be the same. If not we will use, for now the OSM NavAid
        outNewNavInfo = local_navAid;
        outNewNavInfo.flag_navDataFetchedFromDB = true; // v25.09.2
        outNewNavInfo.synchToPoint ();

        // store shared info we prepared in a previous step before calling the OSM function. We will use it after calling the main thread for fallback
        const random_airport_info_struct tmp_info = RandomEngine::shared_navaid_info;

        RandomEngine::shared_navaid_info.navAid.init ();
        RandomEngine::shared_navaid_info.navAid.lat = outNewNavInfo.lat;
        RandomEngine::shared_navaid_info.navAid.lon = outNewNavInfo.lon;

        // test against the nearest navaid
        if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
        {
          // RandomEngine::setError ("[random get_target_or_lastFlightLeg_based_on_XY_or_OSM] Last Navaid. Failed to find Airport NEAR given location. Still using original Navaid: " + outNewNavInfo.gen_locDesc_short ());
          RandomEngine::setError ( fmt::format("[{}] Last Navaid. Failed to find Airport NEAR given location. Will use original Navaid data: {}", __func__, outNewNavInfo.gen_locDesc_short ()) );
          // return false;
        }
        // calculate distance using Point class function.
        outNewNavInfo.synchToPoint ();
        RandomEngine::shared_navaid_info.navAid.synchToPoint ();
        const auto distance = outNewNavInfo.p.calcDistanceBetween2Points (RandomEngine::shared_navaid_info.navAid.p);
        if (( !outNewNavInfo.getID ().empty () && outNewNavInfo.getID () == RandomEngine::shared_navaid_info.navAid.getID () ) || distance <= 1.0)
        {
          outNewNavInfo = RandomEngine::shared_navaid_info.navAid;
          outNewNavInfo.synchToPoint ();
        }

        RandomEngine::shared_navaid_info = tmp_info; // reset status before we used the Shared NavAid

        return true;
      }
    }

    // if OSM data was not found then plugin will try to use the default target search
  }


  /////////////////////////////////////////////
  // Search any last Flight Leg location for any plane
  /////////////////////////////////////////////
  RandomEngine::shared_navaid_info.inRestrictRampType = location_value_restrict_ramp_type_s;
  RandomEngine::shared_navaid_info.inExcludeAngle     = static_cast<int> ( prev_na_ptr->degRelativeToSearchPoint);
  // v3.0.255.3
  if (RandomEngine::shared_navaid_info.inMinDistance_nm < RandomEngine::shared_navaid_info.inStartFromDistance_nm)
    RandomEngine::shared_navaid_info.inMinDistance_nm = RandomEngine::shared_navaid_info.inStartFromDistance_nm;

  #ifdef IBM
  outNewNavInfo = RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, static_cast<uint8_t>(in_plane_type_enum)); // v3.0.255.3 test integration
  #else
  const NavAidInfo nav = RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, static_cast<uint8_t>(in_plane_type_enum) ); // v3.0.255.3 test integration
  outNewNavInfo        = nav;
  #endif

  #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
  if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f)
  {

    if (!this->waitForPluginCallbackJob (missionx::mx_flc_pre_command::gather_random_airport_mainThread, std::chrono::milliseconds (1000))) // pick random airport. Wait up to 10sec
    {
      RandomEngine::setError ("[random get_target_or_lastFlightLeg_based_on_XY_or_OSM first try] Failed to find an airport in expected time. Skipping flight leg: " + target_flight_leg_name + "Maybe share these findings with the developer... ");
      return false;
    }
  }
  #endif // ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL

  // v3.0.241.8 handle: what if we failed to find a NavAid due to the user setup slider or the designer did not provide a big enough radius.
  // We will try with the "designer" area but multiply by 4.
  if (outNewNavInfo.is_lat_lon_valid()  && RandomEngine::mapNavAidsFromMainThread.empty () && location_value_d > 0.0)
  {
    RandomEngine::shared_navaid_info.inMinDistance_nm = RandomEngine::shared_navaid_info.inStartFromDistance_nm;
    RandomEngine::shared_navaid_info.inMaxDistance_nm = static_cast<float> (location_value_d);
    if (RandomEngine::shared_navaid_info.inMinDistance_nm > RandomEngine::shared_navaid_info.inMaxDistance_nm)
      RandomEngine::shared_navaid_info.inMaxDistance_nm = RandomEngine::shared_navaid_info.inMinDistance_nm * 4.0f; // max distance is equal to "start distance" * 4.

    #ifdef IBM
    outNewNavInfo = RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, static_cast<uint8_t>(in_plane_type_enum)); // v3.0.255.3 test integration
    #else
    NavAidInfo local_nav = RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, static_cast<uint8_t>(in_plane_type_enum)); // v3.0.255.3 test integration
    outNewNavInfo  = local_nav;
    #endif

    #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
    if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f) // do we need to fall back to old code?
    {
      if (!this->waitForPluginCallbackJob (missionx::mx_flc_pre_command::gather_random_airport_mainThread, std::chrono::milliseconds (1000))) // pick random airport. Wait up to 10sec
      {
        RandomEngine::setError ("[random get_target_or_lastFlightLeg_based_on_XY_or_OSM second try] Failed to find an airport in expected time. Skipping flight leg: " + target_flight_leg_name + ", Consider sharing these findings with the developer... ");
        return false;
      }
    }
    #endif

  } // end gathering random NavAid

  // Filter location by location_type (NEAR, ICAO, etc...)
  // Add find the closest airport to the last location for location_type = NEAR

  #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
  if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f)
  {
    if (target_location_type.compare (mxconst::get_EXPECTED_LOCATION_TYPE_NEAR ()) == 0)
      this->getRandomAirport_localThread (outNewNavInfo, mxconst::get_EXPECTED_LOCATION_TYPE_NEAR ());
    else
      this->getRandomAirport_localThread (outNewNavInfo); // pick random airport from list of valid locations
  }
  #endif // ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL

  outNewNavInfo.synchToPoint (true);

  // if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f)

  if ( !outNewNavInfo.is_lat_lon_valid () )
  {
    outNewNavInfo.init ();
    outNewNavInfo.err = fmt::format("[{}] Failed to find an airport in radius: {}nm relative to location: {}", __func__, location_value_d, prev_na_ptr->get_latLon_short () );
    RandomEngine::setError (outNewNavInfo.err);
    return false;
  }

  return true;
}


// -----------------------------------


bool
RandomEngine::get_target_or_lastFlightLeg_base_on_XY_or_OSM (NavAidInfo                         &outNewNavInfo,
                                                              std::map<std::string, std::string> &inMapLocationSplitValues,
                                                              missionx::mx_base_node             &inProperties)
                                                              // ,
                                                              // // v3.305.1
                                                              // const double location_value_d,
                                                              // const double location_minDistance_d,
                                                              // const double location_maxDistance_d)
{
  const auto location_value_d        = inProperties.getAttribNumericValue<double> ("location_value_d", -1.0);
  const auto location_min_distance_d = inProperties.getAttribNumericValue<double> ("location_min_distance_d", location_value_d);
  const auto location_max_distance_d = inProperties.getAttribNumericValue<double> ("location_max_distance_d", location_value_d);


  const bool        flag_override_random_target_min_dist = (RandomEngine::flag_force_template_distances_b) ? false : missionx::system_actions::pluginSetupOptions.getBoolValue (mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE ()); // added this->flag_force_template_distances_b to let the designer force his "narrative" when it comes to distances.
  const std::string inFlightLegName                      = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_NAME (), "");
  const std::string inLocationType                       = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), "");

  // represent ramp type
  const std::string location_value_restrict_ramp_type_s = mxUtils::getValueFromElement (inMapLocationSplitValues, std::string ("ramp"), std::string (""));

  NavAidInfo prevNavInfo;
  if (!RandomEngine::listNavInfo.empty ())
  {
    prevNavInfo = RandomEngine::listNavInfo.back ();
    prevNavInfo.synchToPoint (); // not sure if we need this
  }

  // search NavAid location in radius
  // prepare shared thread data
  RandomEngine::shared_navaid_info.init ();
  RandomEngine::shared_navaid_info.p = RandomEngine::lastFlightLegNavInfo.p;
  if (location_value_d > 0.0 && !flag_override_random_target_min_dist)
  {
    RandomEngine::shared_navaid_info.inMaxDistance_nm = static_cast<float> (location_value_d);
  }
  else
  {
    RandomEngine::shared_navaid_info.inMinDistance_nm = static_cast<float> (location_min_distance_d);
    RandomEngine::shared_navaid_info.inMaxDistance_nm = (location_max_distance_d > 0.0) ? static_cast<float> (location_max_distance_d) : mxconst::MAX_RAD_4_OSM_MAX_DIST; // v24.12.2 default distance if not set
  }


  // Search for HELOS last flight leg
  // OSM search first - this code will be used when there is a template or mission template with OSM information in it. It will probably won't be called from user creation screen
  if ((inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_OSM () || inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ()) && missionx::RandomEngine::template_plane_type_enum == missionx::mx_plane_types::plane_type_helos)
  {
    Point E90, W270, S180, N0;

    // get max radius and find the 4 points that create the rectangle area
    // RandomEngine::shared_navaid_info.inMaxDistance_nm = max Radius
    // location_minDistance_d = min radius distance
    RandomEngine::calculate_bbox_coordinates (N0, S180, E90, W270, RandomEngine::lastFlightLegNavInfo.lat, RandomEngine::lastFlightLegNavInfo.lon, RandomEngine::shared_navaid_info.inMaxDistance_nm);
    if (NavAidInfo navAid;
       RandomEngine::osm_get_navaid_from_osm (navAid, inMapLocationSplitValues, inProperties, RandomEngine::lastFlightLegNavInfo.lat, RandomEngine::lastFlightLegNavInfo.lon, S180.lat, N0.lat, W270.lon, E90.lon, RandomEngine::shared_navaid_info.inMaxDistance_nm, location_min_distance_d))
    {
      if (navAid.lat != 0.0 && navAid.lon != 0.0)
      {
        ////// Test Final NavAid against X-Plane. We will check the closest Navaid to that location and it should be the same. If not we will use, for now the OSM NavAid
        outNewNavInfo = navAid;
        outNewNavInfo.flag_navDataFetchedFromDB = true; // v25.09.2
        outNewNavInfo.synchToPoint ();
        // RandomEngine::flag_picked_from_osm_database       = true; // we can use this
        const random_airport_info_struct tmp_info = RandomEngine::shared_navaid_info; // store shared info we prepared in prev step before calling the OSM function. We will use it after calling the main thread for fallback

        RandomEngine::shared_navaid_info.navAid.init ();
        RandomEngine::shared_navaid_info.navAid.lat = outNewNavInfo.lat;
        RandomEngine::shared_navaid_info.navAid.lon = outNewNavInfo.lon;

        // test against nearest navaid
        if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::threadState, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
        {
          RandomEngine::setError ("[random get_target_or_lastFlightLeg_based_on_XY_or_OSM] Last Navaid. Failed to find Airport NEAR given location. Still using original Navaid: " + outNewNavInfo.gen_locDesc_short ());
          // return false;
        }
        outNewNavInfo.synchToPoint ();
        RandomEngine::shared_navaid_info.navAid.synchToPoint ();
        const auto distance = outNewNavInfo.p.calcDistanceBetween2Points (RandomEngine::shared_navaid_info.navAid.p);
        if ((outNewNavInfo.getID () == RandomEngine::shared_navaid_info.navAid.getID () && !outNewNavInfo.getID ().empty ()) || distance <= 1.0)
        {
          outNewNavInfo = RandomEngine::shared_navaid_info.navAid;
          outNewNavInfo.synchToPoint ();
        }

        RandomEngine::shared_navaid_info = tmp_info;

        return true;
      }
    }

    // if OSM data was not found then plugin will try to use default target search
  }


  /////////////////////////////////////////////
  // Search any last Flight Leg location for any plane
  /////////////////////////////////////////////
  RandomEngine::shared_navaid_info.inRestrictRampType = location_value_restrict_ramp_type_s;
  RandomEngine::shared_navaid_info.inExcludeAngle     = static_cast<int> (prevNavInfo.degRelativeToSearchPoint);
  // v3.0.255.3
  if (RandomEngine::shared_navaid_info.inMinDistance_nm < RandomEngine::shared_navaid_info.inStartFromDistance_nm)
    RandomEngine::shared_navaid_info.inMinDistance_nm = RandomEngine::shared_navaid_info.inStartFromDistance_nm;

  #ifdef IBM
  outNewNavInfo = RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, getPlaneType ()); // v3.0.255.3 test integration
  #else
  const NavAidInfo nav = RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, getPlaneType ()); // v3.0.255.3 test integration
  outNewNavInfo        = nav;
  #endif

  #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
  if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f)
  {

    if (!this->waitForPluginCallbackJob (missionx::mx_flc_pre_command::gather_random_airport_mainThread, std::chrono::milliseconds (1000))) // pick random airport. Wait up to 10sec
    {
      RandomEngine::setError ("[random get_target_or_lastFlightLeg_based_on_XY_or_OSM first try] Failed to find an airport in expected time. Skipping flight leg: " + inFlightLegName + "Maybe share these findings with the developer... ");
      return false;
    }
  }
  #endif // ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL

  // v3.0.241.8 handle: what if we failed to find a NavAid due to the user setup slider or the designer not enough given radius. We will try with the "designer" area but multiply by 4.
  if ((outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f) && RandomEngine::mapNavAidsFromMainThread.empty () && location_value_d > 0.0)
  {
    RandomEngine::shared_navaid_info.inMinDistance_nm = RandomEngine::shared_navaid_info.inStartFromDistance_nm;
    RandomEngine::shared_navaid_info.inMaxDistance_nm = static_cast<float> (location_value_d);
    if (RandomEngine::shared_navaid_info.inMinDistance_nm > RandomEngine::shared_navaid_info.inMaxDistance_nm)
    {
      RandomEngine::shared_navaid_info.inMaxDistance_nm = RandomEngine::shared_navaid_info.inMinDistance_nm * 4.0f; // max distance is equel to "start distance" * 4.
    }

    #ifdef IBM
    outNewNavInfo = RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, getPlaneType ()); // v3.0.255.3 test integration
    #else
    NavAidInfo local_nav = RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, getPlaneType ()); // v3.0.255.3 test integration
    outNewNavInfo  = local_nav;
    #endif

    #if (ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL == 1)
    if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f) // do we need to fall back to old code
    {
      if (!this->waitForPluginCallbackJob (missionx::mx_flc_pre_command::gather_random_airport_mainThread, std::chrono::milliseconds (1000))) // pick random airport. Wait up to 10sec
      {
        RandomEngine::setError ("[random get_target_or_lastFlightLeg_based_on_XY_or_OSM second try] Failed to find an airport in expected time. Skipping flight leg: " + inFlightLegName + ", Consider sharing these findings with the developer... ");
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
    if (inLocationType.compare (mxconst::get_EXPECTED_LOCATION_TYPE_NEAR ()) == 0)
      this->getRandomAirport_localThread (outNewNavInfo, mxconst::get_EXPECTED_LOCATION_TYPE_NEAR ());
    else
      this->getRandomAirport_localThread (outNewNavInfo); // pick random airport from list of valid locations
  }
  #endif // ENABLE_GATHER_RANDOM_AIRPORTS_FROM_MAIN_THREAD_CALL

  outNewNavInfo.synchToPoint ();

  if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f)
  {
    RandomEngine::setError ("[random get_target get_target_or_lastFlightLeg_based_on_XY_or_OSM] Failed to find an airport in radius: " + Utils::formatNumber<double> (location_value_d, 2) + "nm relative to location: " + Utils::formatNumber<double> (lastFlightLegNavInfo.lat) + "," + Utils::formatNumber<double> (lastFlightLegNavInfo.lon));
    return false;
  }

  return true;
} // end handle random x/y or random navaid



// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------

} /* namespace missionx */
