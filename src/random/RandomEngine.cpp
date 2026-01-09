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

std::thread                               missionx::RandomEngine::thread_ref;
missionx::base_thread::strct_thread_state missionx::RandomEngine::random_thread_state;

std::map<std::string, std::string>                          missionx::RandomEngine::row_gather_db_data;
std::unordered_map<int, std::map<std::string, std::string>> missionx::RandomEngine::resultTable_gather_random_airports;
std::unordered_map<int, std::map<std::string, std::string>> missionx::RandomEngine::resultTable_gather_ramp_data;

//// weather
std::string missionx::RandomEngine::current_weather_datarefs_s;


mx_plane_types_enum missionx::RandomEngine::template_plane_type_enum; // v25.06.1

// missionx::NavAidInfo RandomEngine::lastFlightLegNavInfo; // v25.09.2
// bool RandomEngine::flag_picked_from_osm_database; // v25.09.2 deprecated
bool                                           RandomEngine::flag_force_template_distances_b; // v25.09.1
missionx::Point                                RandomEngine::planeLocation; // v25.09.2
IXMLNode                                       RandomEngine::xRootTemplate; // v25.09.2
missionx::TemplateFileInfo                    *RandomEngine::working_tempFile_ptr; // v25.09.2
RandomEngine::strct_shared_random_airport_info RandomEngine::shared_navaid_info; // v25.09.2
std::string                                    RandomEngine::errMsg; // v25.09.2
std::vector<std::string>                       RandomEngine::vecMissionInfoOverpassUrls; // v25.09.2
int                                            RandomEngine::current_url_indx_used_i = mxconst::INT_UNDEFINED; // v25.09.2
std::list<missionx::NavAidInfo>                RandomEngine::listNavInfo; // v25.09.2
std::map<XPLMNavRef, missionx::NavAidInfo>     RandomEngine::mapNavAidsFromMainThread; // v3.0.221.4 holds nav aid data from main plugin thread so thread will process it later in the background

IXMLNode RandomEngine::xDrefStartColdAndDark{ IXMLNode::emptyIXMLNode }; // v25.10.1

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

  flag_isLastFlightLeg = false;
  expected_slope_at_target_location_d = 0.0;
  flag_found = false;

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

  RandomEngine::xDrefStartColdAndDark = IXMLNode::emptyIXMLNode;

  // this->mapFlightPlanOrder_si.clear ();
  // this->mapFLightPlanOrder_is.clear ();

  RandomEngine::listNavInfo.clear ();
  this->flag_found = false;

  missionx::RandomEngine::template_plane_type_enum = missionx::mx_plane_types_enum::plane_type_any; // v3.0.221.11
  RandomEngine::planeLocation.init ();
  // RandomEngine::lastFlightLegNavInfo.init ();

  this->flag_isLastFlightLeg = false; // v3.0.219.11

  expected_slope_at_target_location_d = 0.0f;

  missionx::RandomEngine::random_thread_state.thread_wait_state = missionx::mx_random_thread_wait_state_enum::not_waiting; // v3.0.221.3
  missionx::RandomEngine::mapNavAidsFromMainThread.clear ();
  // this->map_customScenery_XPLMNavRef_NavAidsFromMainThread.clear ();

  this->cumulative_location_desc_s.clear ();
}

// -----------------------------------

void
RandomEngine::gen_inject_countdown_timer (const int &current_nav_index, std::map<int, missionx::NavAidInfo> &in_navaid_targets)
{
  static constexpr double HELICOPTER_AVERAGE_SPEED_IN_KNOTS = 75.0;
  static constexpr double MIN_SEARCH_TIME_IN_MIN            = 20.0;
  static constexpr double HOVER_TIME                        = 5.0;

  if (current_nav_index == 0) // skip briefer
    return;

  const auto time_relative_to_avg_speed_in_min = in_navaid_targets[current_nav_index].fpln_distance_between_prev_and_current_navaid / HELICOPTER_AVERAGE_SPEED_IN_KNOTS * 60; // we multiply by 60 minutes to get hours

  const auto minVal    = (time_relative_to_avg_speed_in_min > MIN_SEARCH_TIME_IN_MIN) ? time_relative_to_avg_speed_in_min : MIN_SEARCH_TIME_IN_MIN;
  const auto maxVal    = (minVal <= MIN_SEARCH_TIME_IN_MIN) ? MIN_SEARCH_TIME_IN_MIN + HOVER_TIME + Utils::getRandomRealNumber (5.0, 10.0) : time_relative_to_avg_speed_in_min + HOVER_TIME; // v3.0.255.4 fixed assertion where minVal was larger than maxVal.
  const int  timeInMin = static_cast<int> (Utils::getRandomRealNumber (minVal, maxVal));

  // Add <timer> XML
  auto xml_timer_ptr = Utils::xml_get_or_create_node_ptr (in_navaid_targets[current_nav_index].fpln_xml_target_leg_node, mxconst::get_ELEMENT_TIMER ());
  if (!xml_timer_ptr.isEmpty ())
  {
    xml_timer_ptr.updateAttribute ((in_navaid_targets[current_nav_index].fpln_leg_name + "_timer").c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
    xml_timer_ptr.updateAttribute (mxUtils::formatNumber<int> (timeInMin).c_str (), mxconst::get_ATTRIB_TIME_MIN ().c_str (), mxconst::get_ATTRIB_TIME_MIN ().c_str ());

    if (in_navaid_targets.contains ( current_nav_index + 1) )
      xml_timer_ptr.updateAttribute ( in_navaid_targets[current_nav_index+1].fpln_leg_name.c_str (), mxconst::get_ATTRIB_RUN_UNTIL_LEG ().c_str (), mxconst::get_ATTRIB_RUN_UNTIL_LEG ().c_str ());
    else
      xml_timer_ptr.updateAttribute ("", mxconst::get_ATTRIB_RUN_UNTIL_LEG ().c_str (), mxconst::get_ATTRIB_RUN_UNTIL_LEG ().c_str ());

  } // end if we have a valid <timer> node pointer

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
  if (missionx::RandomEngine::random_thread_state.flagIsActive)
  {
    RandomEngine::setError ("\"Generate Mission Engine\" is already running. Please wait for it to finish.");
    return false;
  }

  // start thread
  if (!missionx::RandomEngine::random_thread_state.flagIsActive)
  {
    if (missionx::RandomEngine::thread_ref.joinable ()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
      missionx::RandomEngine::thread_ref.join (); // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"

    this->init (); // reset all variables
    RandomEngine::random_thread_state.dataString = inKey;
    missionx::RandomEngine::thread_ref   = std::thread (&missionx::RandomEngine::generateRandomMission, this);
  }

  return true;
}

// -----------------------------------

void
RandomEngine::stop_plugin ()
{
  RandomEngine::random_thread_state.flagAbortThread = true;
  if (RandomEngine::random_thread_state.flagIsActive)
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

        // check if we have valid user pick value // v24.03.2 added "vecReplaceOptions_s" empty check to solve the crash
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
                  std::ifstream infs_txt;
                  infs_txt.open (txt_file, std::ios::in);
                  if (infs_txt.is_open ())
                  {
                    char        c = '\0';
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
          // after finishing the loop check if xml_file_content_s different from original_xml_file_content_s
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
              missionx::RandomEngine::random_thread_state.flagAbortThread = true;
              this->abortThread ();
            }
            else
            {
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
  bool   flag_generic_template_b    = false; // v25.09.2
  bool   flag_surprise_me_b    = false; // v25.06.1
  bool   flag_oilrig_b         = false; // v25.09.1

  std::string err;
  //// Thread initialization state
  missionx::RandomEngine::random_thread_state.flagIsActive       = true;
  missionx::RandomEngine::random_thread_state.flagThreadDoneWork = false;
  missionx::RandomEngine::random_thread_state.flagAbortThread    = false;

  this->reset_sequence_numbers(); // v25.06.1

  missionx::RandomEngine::random_thread_state.startThreadStopper ();

  // missionx::RandomEngine::flag_picked_from_osm_database = false; // v3.0.241.10
  bool        result                                    = true;
  std::string pathToTemplateFile;
  pathToTemplateFile.clear ();
  RandomEngine::listNavInfo.clear ();
  // this->setInventories.clear ();
  missionx::RandomEngine::map_flight_legs_translation_from_template.clear (); // v25.09.1
  missionx::RandomEngine::map_osm_inventory_track.clear (); // v25.09.2

  std::string inKey = missionx::RandomEngine::random_thread_state.dataString;

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
    missionx::RandomEngine::random_thread_state.flagAbortThread = true;
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

    missionx::RandomEngine::random_thread_state.flagAbortThread = true;
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

    if (missionx::RandomEngine::random_thread_state.flagAbortThread)
      return false;
  }

  ////// READ MAPPING from template file /////////
  // Validate the <MAPPING> element do exists in template file
  missionx::data_manager::read_element_mapping (pathToTemplateFile); // v3.0.217.4
  if (missionx::data_manager::xmlMappingNode.isEmpty ()) // v3.0.221.15rc3.4
  {
    RandomEngine::setError ("[random] ERROR: Mapping element is missing from template file: " + inKey + ". Fix template file. Aborting mission generating.");
    missionx::RandomEngine::random_thread_state.flagAbortThread = true;

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
    missionx::RandomEngine::random_thread_state.flagAbortThread = true;
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
      if (!this->prepare_blank_template_with_flight_legs_based_on_ui (xRootTemplate, this->xMetadata, local_err))
      {
        RandomEngine::setError (local_err);
        missionx::RandomEngine::random_thread_state.flagAbortThread = true;
      }


      #ifndef RELEASE
      #ifdef LIN
      //////////////////////////////////////
      // Write debug XML to files  /////////
      IXMLRenderer  xmlWriter;
      IXMLErrorInfo errInfo = xmlWriter.writeToFile (xRootTemplate, "/tmp/debug_missionx_template.xml", "ASCII"); // "ISO-8859-1");
      errInfo = xmlWriter.writeToFile (data_manager::xmlMappingNode, "/tmp/debug_missionx_mapping.xml", "ASCII"); // "ISO-8859-1");
      #endif
      #endif


      // v25.05.1 check surprise me
      // reading from metadata must come after "prepare_blank_template_with_flight_legs_based_on_ui()" since it initialize it.
      flag_surprise_me_b = Utils::readBoolAttrib (xMetadata, mxconst::get_ATTRIB_SURPRISE_ME_SUB_CAT_B (), false); // v25.06.1

      if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::medevac) && flag_surprise_me_b)
      {
        // v25.06.1
        auto mxReturn = gen_prepare_medevac_surprise_me (xRootTemplate, xMetadata, planeLocation);

        if (!mxReturn.result)
        {
          Log::logMsgThread (mxReturn.getInfoAsText ());
          Log::logMsgThread (mxReturn.getErrorsAsText ());
          RandomEngine::setError (mxReturn.getErrorsAsText ());
          missionx::RandomEngine::random_thread_state.flagAbortThread = true;
        }

        // check [abort]
        if (missionx::RandomEngine::random_thread_state.flagAbortThread)
          return false;

      }

      // check oilrig
      if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::oil_rig))
      {
        flag_oilrig_b = true;
        auto func_result = this->gen_prepare_mission_based_on_oilrig (xRootTemplate, xMetadata);
        if (!func_result.result)
        {
          RandomEngine::setError (func_result.getErrorsAsText ());
          missionx::RandomEngine::random_thread_state.flagAbortThread = true;
        }


      } // end if oilrig mission

    } // end handling user_driven_mission_layer
    else if (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_external_fpln_layer)
    {
      auto out_result= gen_prepare_mission_based_on_databaseflightplan_site(xRootTemplate, xMetadata);
      if (!out_result.result)
      {
        RandomEngine::setError (out_result.getErrorsAsText ());
        missionx::RandomEngine::random_thread_state.flagAbortThread = true;
      }
      else
        goto post_mission_action;

    }
    else if (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_ils_layer)
    {
      auto out_result= gen_prepare_mission_based_on_ils_search(xRootTemplate, xMetadata);
      if (!out_result.result)
      {
        RandomEngine::setError (out_result.getErrorsAsText ());
        missionx::RandomEngine::random_thread_state.flagAbortThread = true;
      }
      else
        goto post_mission_action;

    }
    else if (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::flight_leg_info)
    {
      auto out_result= gen_prepare_mission_based_on_user_fpln_or_simbrief(xRootTemplate, xMetadata);
      if (!out_result.result)
      {
        RandomEngine::setError (out_result.getErrorsAsText ());
        missionx::RandomEngine::random_thread_state.flagAbortThread = true;
      }
      else
        goto post_mission_action;

    }
  }
  // <content> based template
  else if (nContentChilds_i = missionx::RandomEngine::xRootTemplate.getChildNode (mxconst::get_ELEMENT_CONTENT ().c_str ()).nChildNode ()
    ; nContentChilds_i > 0)
  {
    auto func_result = this->gen_prepare_random_mission_based_on_content (missionx::RandomEngine::xRootTemplate);
    if (!func_result.result)
    {
      Log::logMsgThread (func_result.getErrorsAsText ()); // debug to log
      RandomEngine::setError (func_result.getErrorsAsText ());
      missionx::RandomEngine::random_thread_state.flagAbortThread = true;
      return false;
    }

    flag_created_based_on_content_element = true;
    flag_copy_leg_as_is_b                 = Utils::readBoolAttrib (missionx::RandomEngine::xRootTemplate, mxconst::get_ATTRIB_COPY_LEG_AS_IS_B (), false);
  }


  if (missionx::RandomEngine::random_thread_state.flagAbortThread)
    return false;

  ///// =========================================================================================

  Log::logDebugBO ("[DEBUG random airport] After preparing new mission file main nodes.", true);
  this->setPlaneType (mxUtils::stringToLower (Utils::readAttrib (missionx::RandomEngine::xRootTemplate, mxconst::get_ATTRIB_PLANE_TYPE (), mxconst::get_PLANE_TYPE_HELOS ()))); // v3.0.221.15 Default plane is Helicopter.
  Log::logDebugBO ("[DEBUG random airport] After <briefer_info> node.", true);

  if (!flag_surprise_me_b && !flag_oilrig_b && nContentChilds_i == 0) // v25.08.1 split the "content" code and moved it before the call to "parse_3D_object_template_element"
  {
    // read the briefer element before calling "readFlightLegs_directlyFromTemplate()"
    if (missionx::RandomEngine::random_thread_state.flagAbortThread)
      return false;


    // Construct mission from template <leg>s. The most basic form of mission creation.
    flag_generic_template_b = true;
    auto local_result = gen_prepare_random_mission_based_on_leg_nodes_in_template (RandomEngine::xRootTemplate);
    if (!local_result.result)
    {
      missionx::RandomEngine::setError (local_result.getErrorsAsText ());
      missionx::RandomEngine::random_thread_state.flagAbortThread = true;
      return false;
    }

  }


  // POST_MISSION_ACTIONS:
post_mission_action:

  // call readMissionInfoElement // v3.0.253.1 moved to this location so fetch external code will create briefer info too.
  if (missionx::RandomEngine::random_thread_state.flagAbortThread)
    return false;
  if (!flag_created_based_on_content_element && !flag_generic_template_b && !gen_read_mission_info_element ()) // we can skip this function call if we built the mission based on content element. We need to read it inside content to have the custom <overpass> element from <mission_info>
    return false;

  Utils::xml_delete_empty_nodes (xDummyTopNode); // v3.0.219.3 remove invalid points

  if (missionx::RandomEngine::random_thread_state.flagAbortThread)
    return false;

  // v3.0.221.10 Add <xpdata> element if exists
  if (xRootTemplate.nChildNode (mxconst::get_ELEMENT_XPDATA ().c_str ()) > 0)
    this->xpData = xRootTemplate.getChildNode (mxconst::get_ELEMENT_XPDATA ().c_str ());

  if (xRootTemplate.nChildNode (mxconst::get_ELEMENT_EMBEDDED_SCRIPTS ().c_str ()) > 0)
    this->xEmbedScripts = xRootTemplate.getChildNode (mxconst::get_ELEMENT_EMBEDDED_SCRIPTS ().c_str ());

  this->xScoring       = xRootTemplate.getChildNode (mxconst::get_ELEMENT_SCORING ().c_str ()).deepCopy (); // v3.303.9
  this->xCompatibility = xRootTemplate.getChildNode (mxconst::get_ELEMENT_COMPATIBILITY ().c_str ()).deepCopy (); // v24.12.2

  if (missionx::RandomEngine::random_thread_state.flagAbortThread)
    return false;

  // Final validations
  if (int nFlightLegs = this->xFlightLegs.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ()); nFlightLegs == 0)
  {
    RandomEngine::setError ("[random] No flight leg has been created. Try to re-generate a mission, tweak the template or re-run APT.DAT optimization (setup screen).");
    this->abortThread ();
    return false;
  }

  // store plane type // v24.12.1
  if (!this->xMetadata.isEmpty ())
  {
    // plane type
    xMetadata.updateAttribute (Utils::readAttrib (missionx::RandomEngine::xRootTemplate, mxconst::get_ATTRIB_PLANE_TYPE (), "").c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ());
    // ui layer (code) from where the mission was generated
    xMetadata.updateAttribute (mxUtils::formatNumber<int> (static_cast<int> (data_manager::getGeneratedFromLayer ())).c_str (), mxconst::get_ATTRIB_UI_LAYER ().c_str (), mxconst::get_ATTRIB_UI_LAYER ().c_str ());
  }

  ////////////////////
  // Write to file
  result = writeTargetFile ();

  auto endCacheLoad = std::chrono::steady_clock::now ();
  auto diff_cache   = endCacheLoad - startThreadClock;
  duration          = std::chrono::duration<double, std::milli> (diff_cache).count ();
  Log::logAttention ("*** Finished Generating RANDOM Mission, Duration: " + Utils::formatNumber<double> (duration, 3) + "ms (" + Utils::formatNumber<double> ((duration / 1000), 3) + "sec)  ****", true);

  /// finalize thread
  missionx::RandomEngine::random_thread_state.flagIsActive       = false;
  missionx::RandomEngine::random_thread_state.flagThreadDoneWork = true; // we reset the thread at Mission::flc_aptdat() function

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
// -----------------------------------


IXMLNode
RandomEngine::gen_get_skewed_target_position (const IXMLNode &inRealTargetPositionPoint)
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
RandomEngine::parse_display_object_element (missionx::NavAidInfo *in_target_nav_ptr, IXMLNode &inFlightLegNode, IXMLNode &inDisplayNode, IXMLNode & in_xRootTemplate, IXMLNode & x3DObjTemplate, double &expected_slope_at_target_location_d, std::string & inout_err)
{
  // 1. Check if <display_object> tag has random_object attribute. If so it will pick one and add to the <object_templates>
  // 2. If random element is not valid, or we failed to find then use: name="".
  // 3. if no name has been provided then return "false", node is not valid.
  // We will use:  xDummyTopNode and x3DObjTemplate (holds pre-defined objects)

  inout_err.clear (); // v25.06.1
  const std::string TAG_NAME = inDisplayNode.getName ();

  bool flag_foundValidRandomNode = false;

  // v3.0.219.10 read information regarding flight leg in water.
  bool flag_isFlightLegInWater = (in_target_nav_ptr!=nullptr)? in_target_nav_ptr->fpln_is_wet : Utils::readBoolAttrib (inFlightLegNode, mxconst::get_PROP_IS_WET (), false);
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


  // check if object creation is limited by a terrain slope
  if (limit_to_terrain_slope < 100 && expected_slope_at_target_location_d > limit_to_terrain_slope) // v3.0.219.12+
  {
    Log::logMsg ("3D Object: " + name + ", rejected due to terrain slope.", true); // v3.0.219.12+
    return false;
  }


  if (flag_isFlightLegInWater && !randomWaterTag.empty ()) // v3.0.219.10 switch between terrain random object and water tag object
  {
    #ifndef RELEASE
    Log::logMsgThread (fmt::format("[{}] Replaced randomTag with the water Tag: {}, for display object name: {}", __func__, randomWaterTag, name) );
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


missionx::NavAidInfo
RandomEngine::gen_parse_template_leg (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode& xTemplateNode
                                  , const IXMLNode &xml_leg_node_from_template, strct_shared_random_airport_info &inout_shared_navaid
                                  , std::map<int, missionx::NavAidInfo> &in_mission_targets, const int &in_leg_counter, const bool is_last_flight_leg
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

  missionx::NavAidInfo na;
  na.fpln_expected_location_data = missionx::NavAidInfo::parse_expected_location (xml_leg_node_from_template, "custom content", is_last_flight_leg);

  // check if "expected location" is valid
  if (!na.fpln_expected_location_data.error.empty ())
  {
    outErr = na.fpln_expected_location_data.error;
    na.init();
    return na;
  }

  na.fpln_wp_template_type = (na.fpln_expected_location_data.flight_leg_type_hover_land_or_start.empty ()) ? mxconst::get_FL_TEMPLATE_VAL_LAND () : na.fpln_expected_location_data.flight_leg_type_hover_land_or_start;

  // handle "start" template
  if (mxconst::get_FL_TEMPLATE_VAL_START () == na.fpln_wp_template_type && mxUtils::isElementExists (in_mission_targets, 0) && in_mission_targets[0].is_lat_lon_valid ())
  {
    na.lat = in_mission_targets[0].lat;
    na.lon = in_mission_targets[0].lon;
    na.setID (in_mission_targets[0].getID ());
    na.setName (in_mission_targets[0].getName ());

    if (na.getID ().empty ())
    {
      inout_shared_navaid.navAid = na;
      if (missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
      {
        // check distance and hopefully pick the correct airport. Since we are using a fixed distance, this might not be a 100% guaranty
        inout_shared_navaid.navAid.synchToPoint ();

        const double dist = inout_shared_navaid.navAid.p.calcDistanceBetween2Points (na.p, mx_units_of_measure::nm);
        if (dist <= 2.0 && !inout_shared_navaid.navAid.getID ().empty ())
        {
          // We only want to store the ID/Name of the Navaid, not the position (lat/lon).
          if (na.getID ().empty () && !inout_shared_navaid.navAid.getID ().empty ())
            na.setID (inout_shared_navaid.navAid.getID ());

          if (na.getNavAidName ().empty () && !inout_shared_navaid.navAid.getNavAidName ().empty ())
            na.setName (inout_shared_navaid.navAid.getNavAidName ());

          na.height_mt = inout_shared_navaid.navAid.height_mt;
          na.navRef    = inout_shared_navaid.navAid.navRef;
          na.synchToPoint (true);
        }
      } // end "mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread"
    } // end if ID is empty

    na.flag_is_same_as_start_location = true; // v25.09.2 special flag to later ignore inventory construction.

  } // end handling "start"
  else
  {
    RandomEngine::flag_force_template_distances_b = na.fpln_expected_location_data.flag_force_template_distances_b;

    // The flightLegName will be overridden in a later function, when we will create the <leg> node.
    const std::string flightLegName = fmt::format ("{}_{}", mxconst::get_ELEMENT_LEG (), in_leg_counter);
    // const std::string flight_leg_type_hover_land_or_start = mxconst::get_FL_TEMPLATE_VAL_LAND ();
    // relevant only in case we use "tag_name" and we pick <points> from it.
    // Check if we have to force flight_leg_type on the random point that we might pick.
    const bool flag_force_flight_leg_type = Utils::readBoolAttrib (xml_leg_node_from_template, mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B (), false);

    missionx::mx_base_node targetProp; // v3.305.1

    // decide location_value_d value
    const std::string location_value_nm_s = mxUtils::getValueFromElement (na.fpln_expected_location_data.mapLocationSplitPropertiesValues, std::string ("nm"), std::string (""));

    double location_value_d = -1.0;
    if (!location_value_nm_s.empty () && Utils::is_number (location_value_nm_s))
      location_value_d = Utils::stringToNumber<double> (location_value_nm_s, static_cast<int> (location_value_nm_s.length ()));

    const bool ui_user_picked_webosm = Utils::readBoolAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_WEB_OSM_CHECKBOX (), false);
    const bool ui_user_picked_osm    = Utils::readBoolAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_OSM_CHECKBOX (), false);
    if (!is_last_flight_leg && (ui_user_picked_osm || ui_user_picked_webosm))
    {
      na.fpln_expected_location_data.location_type = ( ui_user_picked_osm )? mxconst::get_EXPECTED_LOCATION_TYPE_OSM () : mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ();

      // distance data
      na.fpln_expected_location_data.nm_between_max = data_manager::prop_userDefinedMission_ui.getAttribNumericValue <float> (mxconst::get_PROP_MAX_DISTANCE_SLIDER (), (mxconst::SLIDER_MAX_RND_DIST / 2.0));
      const float fallback_min_distance = (na.fpln_expected_location_data.nm_between_min > na.fpln_expected_location_data.nm_between_max)
                                          ? na.fpln_expected_location_data.nm_between_max / 2.0f :
                                            (na.fpln_expected_location_data.nm_between_min > 0.0)
                                            ? na.fpln_expected_location_data.nm_between_min : na.fpln_expected_location_data.nm_between_max / 2.0f;

      na.fpln_expected_location_data.nm_between_min = data_manager::prop_userDefinedMission_ui.getAttribNumericValue <float> (mxconst::get_PROP_MIN_DISTANCE_SLIDER (), fallback_min_distance);

    }


    targetProp.setStringProperty (mxconst::get_ATTRIB_NAME (), flightLegName); // leg name
    targetProp.setStringProperty (mxconst::get_ATTRIB_TYPE (), na.fpln_wp_template_type); // leg type
    targetProp.setStringProperty (mxconst::get_ATTRIB_LOCATION_TYPE (), na.fpln_expected_location_data.location_type); // location type
    targetProp.setBoolProperty (mxconst::get_PROP_IS_LAST_FLIGHT_LEG (), is_last_flight_leg); // is the last flight leg?
    targetProp.setBoolProperty (mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B (), flag_force_flight_leg_type); // force leg type ?
    // v25.09.2 unsupported properties, we won't force land or hover anymore.
    // targetProp.setNodeProperty<int> (mxconst::get_ATTRIB_FORCE_TYPE_OF_TEMPLATE (), static_cast<int> (which_type_to_force_enum)); // force level terrain or slope ?
    // targetProp.setNodeProperty<int> (mxconst::get_PROP_NUMBER_OF_LOOPS_TO_FORCE_TYPE_TEMPLATE (), how_many_times_to_loop_i); // a force slope will be used with webosm
    targetProp.setStringProperty ("nm", mxUtils::getValueFromElement (na.fpln_expected_location_data.mapLocationSplitPropertiesValues, std::string ("nm"), std::string (""))); // location type
    targetProp.setStringProperty ("tag", mxUtils::getValueFromElement (na.fpln_expected_location_data.mapLocationSplitPropertiesValues, std::string ("tag"), std::string (""))); // location type
    targetProp.setStringProperty ("nm_between", mxUtils::getValueFromElement (na.fpln_expected_location_data.mapLocationSplitPropertiesValues, std::string ("nm_between"), std::string (""))); // location type
    targetProp.setNodeProperty<double> ("location_value_d", location_value_d); //
    targetProp.setNodeProperty<double> ("location_min_distance_d", na.fpln_expected_location_data.nm_between_min);
    targetProp.setNodeProperty<double> ("location_max_distance_d", na.fpln_expected_location_data.nm_between_max);


    if (missionx::RandomEngine::random_thread_state.flagAbortThread)
    {
      outErr = "User asked to Abort!";
      na.init ();
      return na;
    }

    if (is_last_flight_leg)
    {
      // auto result = get_target_or_lastFlightLeg_base_on_XY_or_OSM (na, data.mapLocationSplitValues, targetProp, location_value_d, data.nm_between_min, data.nm_between_max);
      // auto result = get_target_or_lastFlightLeg_base_on_XY_or_OSM (na, na.fpln_expected_location_data.mapLocationSplitValues, targetProp);
      if (mxUtils::isElementExists (in_mission_targets, in_leg_counter - 1))
      {
        const auto result = gen_target_or_last_flight_leg_base_on_xy_or_osm (na, RandomEngine::template_plane_type_enum, na.fpln_expected_location_data.mapLocationSplitPropertiesValues, targetProp, &in_mission_targets[in_leg_counter - 1]);
        if (!result && !na.err.empty ())
          outErr = na.err;
      }
      return na;
    }

    // Expected location: ICAO, NEAR
    if ( (na.fpln_expected_location_data.location_type == mxconst::get_EXPECTED_LOCATION_TYPE_NEAR ()
       || na.fpln_expected_location_data.location_type == mxconst::get_EXPECTED_LOCATION_TYPE_ICAO ()
       )
      && !in_mission_targets.empty ())
    {
      // if (!RandomEngine::gen_get_target_base_on_tag_name_static (na, na.fpln_expected_location_data.mapLocationSplitValues, targetProp, &in_mission_targets[in_leg_counter - 1]) )

      if (const int i_last_target = static_cast<int>(in_mission_targets.size ()) - 1;
        !RandomEngine::gen_target_base_on_icao_or_near_types (na, RandomEngine::getPlaneType_enum (), na.fpln_expected_location_data.mapLocationSplitPropertiesValues, targetProp, &in_mission_targets[i_last_target]) )
      {
        if (!na.err.empty ())
          outErr = na.err;
        else
          outErr = "Error during call to: 'gen_target_base_on_icao_or_near_types() function.";

        na.init ();
      }
      return na;
    }


    // handle XY, OSM or OSMWEB
    if (in_mission_targets.contains (in_leg_counter - 1) )
    {
      auto result = gen_target_base_on_xy_osm_or_osmweb_types (na, RandomEngine::template_plane_type_enum, na.fpln_expected_location_data.mapLocationSplitPropertiesValues, targetProp, &in_mission_targets[in_leg_counter - 1]);
      if (!result && !na.err.empty ())
      {
        outErr = na.err;
        na.init ();
      }
    }
  }

  return na;
}

// -----------------------------------

std::map<int, missionx::NavAidInfo>
RandomEngine::gen_get_content_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &xTemplateNode, const IXMLNode &xContent, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr)
{
  std::map<int, missionx::NavAidInfo> local_target_navaids;

  outErr.clear ();

  ///////////////////////////////////////////
  // Prepare base briefer data base on: parse <briefer_and_start_location> node
  IXMLNode x_briefer_and_start_location_node = xTemplateNode.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ().c_str ()).deepCopy ();
  local_target_navaids[0] = gen_briefer_phase_01_parse_briefer_and_start_location (xTemplateNode, x_briefer_and_start_location_node );
  if ( !local_target_navaids[0].is_lat_lon_valid () || !local_target_navaids[0].err.empty () )
  {
    outErr = fmt::format ("[{}] <{}> was not found in the template. Fix the template.", __func__, mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION () );
    local_target_navaids.clear ();
    return local_target_navaids;
  }

  //////////////////////////////////////////////////
  // Parse <content> List and build targets from it.
  // example returns: <delivery list="leg_delivery,leg_delivery|optional=50%,leg_delivery,leg_land," plane_type="prop">Hello// pilot.;Today you will fly to few locations to deliver goods. Use your 'inventory' to move items from/to your plane.</delivery>
  const std::string            flightLegList = Utils::readAttrib (xContent, mxconst::get_ATTRIB_LIST (), "");
  const std::list<std::string> listContent   = Utils::splitStringToList (flightLegList, mxconst::get_COMMA_DELIMITER ());
  std::string                  optional;
  // int                          target_leg_counter = 0;

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
      local_target_navaids.clear ();
      return local_target_navaids;
    }

    ////////////////////////////////////////
    /// Generate Target from Content List
    ////////////////////////////////////////
    const bool                          is_last_leg = listContent.size () == static_cast<size_t> (content_list_item_counter);

    // Check if set of flight legs
    const std::string is_element_set_of_flight_legs = Utils::readAttrib (xFlightLegNodeFromTemplate, mxconst::get_ATTRIB_IS_SET_OF_FLIGHT_LEGS (), "");
    if (is_element_set_of_flight_legs.empty ())
    { // process only one template <leg>
      auto na = gen_parse_template_leg (inoutThreadState, xTemplateNode, xFlightLegNodeFromTemplate, inout_shared_navaid, local_target_navaids, static_cast<int>(local_target_navaids.size ()), is_last_leg, outErr);

      // Validate no errors during leg parsing from the template
      if (!na.is_lat_lon_valid () || !outErr.empty () || !na.err.empty ())
      {
        if (!na.err.empty ())
          outErr = na.err;

        local_target_navaids.clear ();
        return local_target_navaids;
      }

      // store the original template leg node
      na.fpln_xml_osm_q_or_raw_tmpl_node = xFlightLegNodeFromTemplate.deepCopy ();
      na.synchToPoint (true);
      local_target_navaids[static_cast<int> (local_target_navaids.size ())] = na;
    }
    else
    { // process set of template <leg>s
      int nChilds = xFlightLegNodeFromTemplate.nChildNode(mxconst::get_ELEMENT_LEG ().c_str ());
      for (int loop_i1 = 0; loop_i1 < nChilds; ++loop_i1)
      {
        // Are we there yet ? (is this the last <leg> in the set and the whole <content> list ?
        // Because a node can be the last in the list, it can also be a "set".
        // We need to reflect this nuance so the "gen_parse_template_leg()" will have the correct data to work with. It this <leg> from the set the last or not ?
        const auto local_is_last_lag = (is_last_leg)? (loop_i1 + 1 == nChilds) : false;

        auto xLeg = xFlightLegNodeFromTemplate.getChildNode (mxconst::get_ELEMENT_LEG ().c_str (), loop_i1);
        auto na = gen_parse_template_leg (inoutThreadState, xTemplateNode, xLeg, inout_shared_navaid, local_target_navaids, static_cast<int>(local_target_navaids.size ()), local_is_last_lag, outErr);
        // Validate no errors during leg parsing from the template
        if (!na.is_lat_lon_valid () || !outErr.empty () || !na.err.empty ())
        {
          if (!na.err.empty ())
            outErr = na.err;

          local_target_navaids.clear ();
          return local_target_navaids;
        }

        // store the original template leg node
        na.fpln_xml_osm_q_or_raw_tmpl_node = xLeg.deepCopy ();
        na.synchToPoint (true);
        local_target_navaids[static_cast<int> (local_target_navaids.size ())] = na;

      } // end loop over all <leg> nodes in the set
    }

    // // Do final validation
    // for (auto& [indx, na] : navaids)
    // {
    //   if (na.is_lat_lon_valid ())
    //     local_target_navaids[++target_leg_counter] = na;
    // }

  } // end loop over list tags


  return local_target_navaids;
}

missionx::mx_return
RandomEngine::gen_prepare_random_mission_based_on_content (IXMLNode &xTemplateNode)
{
  missionx::mx_return out_func_result;

  //// get random content Node if available in template
  IXMLNode xContent = RandomEngine::get_content_story (xTemplateNode);

  // we won't support random without content.
  if (xContent.isEmpty ())
  {
    RandomEngine::setError ("No <content> element was found. Aborting random mission creation. To fix this, please add <content> element. Check documentation.");
    return false;
  }

  // v25.10.2 The plane type should be defined by the <content> template.
  // const auto plane_type_enum_i = RandomEngine::gen_parse_plane_type (data_manager::prop_userDefinedMission_ui, xTemplateNode, inout_meta_node);
  // this->setPlaneType (plane_type_enum_i); // set plane type in class level for other function usage too


  // Plane type is mandatory at the <content> or <template> level
  // use plane_type from content then from template then throw error if it is not defined or it is not a valid type
  std::string pType = Utils::readAttrib (xContent, mxconst::get_ATTRIB_PLANE_TYPE (), "");
  if (RandomEngine::is_plane_type_valid (pType))
  {
    const auto local_plane_type = this->setPlaneType (pType);
    pType                       = RandomEngine::translatePlaneTypeToString (local_plane_type);

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

  if (const bool copy_as_is_b = Utils::readBoolAttrib (xContent, mxconst::get_ATTRIB_COPY_AS_IS_B (), false)
    ;copy_as_is_b)
    return gen_content_option_02_copy_as_is (xTemplateNode, xContent);

  return gen_content_option_01_random_mission_from_content (xTemplateNode, xContent);

  // out_func_result.result = true;
  // return out_func_result;
}



missionx::mx_return
RandomEngine::gen_content_option_01_random_mission_from_content (IXMLNode &xTemplateNode, IXMLNode &xContent)
{

  std::string err;
  missionx::mx_return out_func_result;

  std::map<int, NavAidInfo> navaid_targets = RandomEngine::gen_get_content_targets (&RandomEngine::random_thread_state, xTemplateNode, xContent, RandomEngine::shared_navaid_info, err);

  ///////////////////
  // Validations
  ///////////////////

  // check err
  if (!err.empty () )
  {
    out_func_result.addErrMsg (err, true);
    return out_func_result;
  }

  // test min flight leg expected
  if (const auto min_valid_flight_legs_i = Utils::readNodeNumericAttrib<unsigned long> (xContent, mxconst::get_ATTRIB_MIN_VALID_FLIGHT_LEGS (), 1)
      ; min_valid_flight_legs_i > 0 && min_valid_flight_legs_i > navaid_targets.size ())
  {
    out_func_result.addErrMsg ("Not enough valid targets were found. Aborting.", true);
    return out_func_result;
  }

  // validate navaid targets
  int valid_navaids_i = 0;
  auto navaids_validation = gen_validate_navaids (navaid_targets, valid_navaids_i);
  if (!navaids_validation.result) // if there is a failure
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg (navaids_validation.getErrorsAsText (), true);
    return out_func_result;
  }

  if ( valid_navaids_i != static_cast<int>(navaid_targets.size ()) )
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg ( fmt::format("Valid targets found: {}, is not the same as overall generated targets: {}", valid_navaids_i, navaid_targets.size ()), true);
    return out_func_result;
  }

  // Test if we have targets
  if (navaid_targets.empty ())
  {
    out_func_result.addErrMsg ("No valid targets were found. Aborting.", true);
    return out_func_result;
  }

  // check [abort] by user
  if (RandomEngine::random_thread_state.flagAbortThread)
  {
    out_func_result.addErrMsg ("User asked to abort.", true);
    return out_func_result;
  }

  bool flag_one_of_the_targets_above_water = false;
  //-----------------------------------------------
  //--- Analyze Water Bodies / Slope / Leg Name ---
  //-----------------------------------------------
  for (auto &[indx, target_navaid] : navaid_targets )
  {
    target_navaid.fpln_seq = indx;

    // make sure a waypoint type is set
    if (target_navaid.fpln_wp_template_type.empty ())
      target_navaid.fpln_wp_template_type = mxconst::get_FL_TEMPLATE_VAL_LAND();

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
  gen_briefer_phase_02_base_node_from_navaid (navaid_targets[0], RandomEngine::shared_navaid_info, flag_one_of_the_targets_above_water);


  // Add <mission_info> from the template, we do not generate it.
  IXMLNode x_local_BrieferInfo = xTemplateNode.getChildNode (mxconst::get_ELEMENT_MISSION_INFO ().c_str ());
  if (x_local_BrieferInfo.isEmpty ())
  {
    out_func_result.addErrMsg (fmt::format("{} element is missing from the base template.", mxconst::get_ELEMENT_MISSION_INFO () ), true) ;
    return out_func_result;
  }


  // ------------------------------------------------------------------
  // Construct all mission <leg> nodes
  // navaid_targets: [0] = start/briefer, [1], [2]..[N-1] = final location.
  // ------------------------------------------------------------------
  // add "mission_type" attribute from the content <list>
  const std::string attrib_mission_type = mxUtils::stringToLower ( Utils::readAttrib (xContent, mxconst::get_ATTRIB_MISSION_TYPE (), "") );
  auto attrib_mission_type_enum = mxUtils::translate_mission_type_to_task_type (attrib_mission_type);
  const mx_ui_mission_type translated_mission_type_to_ui_task_type = mxUtils::translate_mission_type_to_med_cargo_or_oilrig_task_type (attrib_mission_type);
  if (translated_mission_type_to_ui_task_type != mx_ui_mission_type::undefined)
  {
    Utils::xml_set_attribute_in_node <int>(this->xMetadata, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (translated_mission_type_to_ui_task_type), this->xMetadata.getName());
    Utils::xml_set_attribute_in_node <int>(this->xMetadata, mxconst::get_ATTRIB_MISSION_TYPE (), static_cast<int> (attrib_mission_type_enum), this->xMetadata.getName());
  }

  gen_create_all_leg_nodes_based_on_navaid_targets (navaid_targets);

  // loop over all targets
  for (auto &[indx, target_navaid] : navaid_targets)
  {

    // Add external inventory
    // will skip any navaid that is the same as the Start location.
    if (!target_navaid.flag_is_same_as_start_location) //
    {
      target_navaid.fpln_xml_inv_node = gen_add_inventory_phase01_node (indx, target_navaid, map_osm_inventory_track);
      //  skip items phase, if it is the last location or inventory node is empty.
      if (!target_navaid.fpln_xml_inv_node.isEmpty () && navaid_targets.contains (indx + 1))
        gen_add_inventory_phase02_add_items (target_navaid);
    }

    if (indx == 0) // skip briefer
    {
      target_navaid.fpln_mission_phase = missionx::enums::mx_rnd_mission_phase::start;
      continue;
    }

    NavAidInfo *next_navaid_ptr = (navaid_targets.contains (indx + 1 ))? &navaid_targets[indx + 1] : nullptr;

    // add start messages
    gen_leg_start_messages (this->seq_messages, target_navaid, navaid_targets, this->xMessages, flag_one_of_the_targets_above_water);

    // add 3D object sets
    // gen_add_3d_objects_for_surprise_me_base_on_predefined_attributes (target_navaid, target_navaid.fpln_xml_target_leg_node, xTemplateNode, this->x3DObjTemplate, this->expected_slope_at_target_location_d);
    gen_3d_add_display_object_sets_instances_to_leg (target_navaid, target_navaid.fpln_xml_target_leg_node, xTemplateNode, this->x3DObjTemplate, this->expected_slope_at_target_location_d);

    // add 3D display objects around the landing
    if (!target_navaid.flag_is_skewed)
    {
      if (target_navaid.fpln_task_type < enums::mx_rnd_task_type::cargo)
        gen_3d_hint_objects_for_land_and_hover (target_navaid, target_navaid.fpln_xml_target_leg_node, next_navaid_ptr);
    }

    gen_3d_parse_instances_in_leg (target_navaid.fpln_xml_target_leg_node, target_navaid);


    // ADD <leg> XML Node
    target_navaid.synchToPoint ();
    target_navaid.fpln_xml_target_leg_node = this->xFlightLegs.addChild (target_navaid.fpln_xml_target_leg_node);

    // add task <trigger> nodes to main trigger node.
    for (auto &node : target_navaid.fpln_leg_vec_trigger_nodes)
      this->xTriggers.addChild (node.deepCopy ());

    // add target navaid <objective> node to the main objectives node
    this->xObjectives.addChild (target_navaid.fpln_leg_objective_node.deepCopy ());


    // check [abort]
    if (RandomEngine::random_thread_state.flagAbortThread)
    {
      out_func_result.addErrMsg ("User asked to abort.", true);
      return out_func_result;
    }

  } // end "Content" loop over all Target NavAids and construct the base information needed for the mission file

  // Add the final flight plan to display in the ui
  this->cumulative_location_desc_s = gen_get_cumulative_fpln_desc (navaid_targets);

  // ----------------------
  // -- Prepare <GPS> node
  // ----------------------
  for (const auto &na : navaid_targets | std::views::values)
  {
    auto p_gps_node      = na.p.node.deepCopy ();
    auto p_gps_skew_node = (na.xml_skewdPointNode.isEmpty ()) ? IXMLNode::emptyIXMLNode : na.xml_skewdPointNode.deepCopy ();

    p_gps_node = Utils::xml_clear_node_attributes_excluding_list (p_gps_node,
                                                                  { mxconst::get_ATTRIB_LAT (), mxconst::get_ATTRIB_LONG (), mxconst::get_ATTRIB_ELEV_FT (), mxconst::get_ELEMENT_ICAO (), mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B (), mxconst::get_PROP_IS_WET ()
                                                                  },
                                                                  false,
                                                                  true);

    p_gps_skew_node = Utils::xml_clear_node_attributes_excluding_list (p_gps_skew_node,
                                                                       { mxconst::get_ATTRIB_LAT (), mxconst::get_ATTRIB_LONG (), mxconst::get_ATTRIB_ELEV_FT (), mxconst::get_ELEMENT_ICAO (), mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B (), mxconst::get_PROP_IS_WET ()
                                                                       },
                                                                       false,
                                                                       true);

    if (na.flag_is_skewed && !p_gps_skew_node.isEmpty ())
      this->xGPS.addChild (p_gps_skew_node);
    else
      this->xGPS.addChild (p_gps_node);
  }

  // add Briefer description
  gen_briefer_phase_03_add_desc (navaid_targets, flag_one_of_the_targets_above_water);
  this->xBriefer = navaid_targets[0].fpln_xml_target_leg_node.deepCopy ();

  // v25.10.1 Add Cold and dark
  RandomEngine::xDrefStartColdAndDark = gen_set_and_get_start_cold_and_dark (xTemplateNode, navaid_targets[1]);

  // add <mission_info>
  if (!gen_read_mission_info_element ()) // <mission_info>
  {
    missionx::RandomEngine::random_thread_state.flagAbortThread = true;
    out_func_result.addErrMsg ("No <mission_info> node was found in template.", true);
  }

  // loop over all inventories and add to the global xInventories node
  for (auto &[key, nav] : navaid_targets)
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


  return out_func_result = true;
}


// -----------------------------------


missionx::mx_return
RandomEngine::gen_content_option_02_copy_as_is (IXMLNode &xTemplateNode, IXMLNode & xContent)
{
  missionx::mx_return  out_func_result = true; // we assume that all is valid

  //////////////////////////////////////////////////
  // Parse <content> List and build targets from it.
  // example returns: <delivery list="leg_delivery,leg_delivery|optional=50%,leg_delivery,leg_land," plane_type="prop">Hello// pilot.;Today you will fly to few locations to deliver goods. Use your 'inventory' to move items from/to your plane.</delivery>
  const std::string            flightLegList = Utils::readAttrib (xContent, mxconst::get_ATTRIB_LIST (), "");
  const std::list<std::string> listContent   = Utils::splitStringToList (flightLegList, mxconst::get_COMMA_DELIMITER ());

  if (listContent.empty ())
  {
    out_func_result.addErrMsg ("No valid tag name in the content list.", true);
    return out_func_result;
  }

  std::string tag_name = listContent.front ();

  // search the <tag name> in xTemplateNode
  auto content_root_node = xTemplateNode.getChildNode (tag_name.c_str ()).deepCopy ();
  if (content_root_node.isEmpty ())
  {
    out_func_result.addErrMsg (fmt::format(R"(No valid tag name: "{}" was found in the template file.)", tag_name ), true);
    return out_func_result;
  }


  // Construct Briefer
  ///////////////////////////////////////////
  // Prepare base briefer data base on: parse <briefer_and_start_location> node
  IXMLNode x_briefer_and_start_location_node = content_root_node.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ().c_str ()).deepCopy ();
  missionx::NavAidInfo na_briefer = gen_briefer_phase_01_parse_briefer_and_start_location (content_root_node, x_briefer_and_start_location_node );
  if ( !na_briefer.is_lat_lon_valid () || !na_briefer.err.empty () )
  {
    out_func_result.addErrMsg ( fmt::format ("[{}] <{}> was not found in the template. Fix the template.", __func__, mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ()), true);
    return out_func_result;

  }

  na_briefer.fpln_navaid_was_already_prepared = true;

  std::string location_adjust_desc = na_briefer.fpln_expected_location_data.desc;

  constexpr bool is_wet = false;
  gen_briefer_phase_02_base_node_from_navaid (na_briefer, RandomEngine::shared_navaid_info, is_wet);
  if (!na_briefer.err.empty ())
  {
    out_func_result.addErrMsg (na_briefer.err, true);
    return out_func_result;
  }

  // add attributes needed to briefer like the mandatory "starting location"
  std::set<std::string> set_of_attribute_to_copy = {mxconst::get_ATTRIB_STARTING_LEG (), mxconst::get_ATTRIB_STARTING_ICAO ()};
  Utils::xml_copy_specific_attributes_using_white_list (x_briefer_and_start_location_node, na_briefer.fpln_xml_target_leg_node, &set_of_attribute_to_copy, false);

  if (!location_adjust_desc.empty ())
    Utils::xml_add_cdata (na_briefer.fpln_xml_target_leg_node, location_adjust_desc);

  // We can't call the gen_briefer_phase_03_add_desc () since we do not have targets
  this->xBriefer = na_briefer.fpln_xml_target_leg_node.deepCopy();

  // The briefer info is taken from the "template node" and not the "content root" node. It is needed by the plugin during template list reading.
  this->xBrieferInfo = xTemplateNode.getChildNode (mxconst::get_ELEMENT_MISSION_INFO ().c_str ()).deepCopy ();


  // copy nodes
  // v3.303.8 make sure Embedded/Script is available, and also template name is from template itself, good for distinguishing between different template choices
  if (this->xEmbedScripts.isEmpty ())
    this->xEmbedScripts = Utils::xml_get_or_create_node_ptr (content_root_node, mxconst::get_ELEMENT_EMBEDDED_SCRIPTS ());

  const std::string template_name = Utils::readAttrib (content_root_node, mxconst::get_ATTRIB_NAME (), "");
  if (!template_name.empty ())
    this->xDummyTopNode.updateAttribute (template_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());

  const std::string title_text = Utils::readAttrib (content_root_node, mxconst::get_ATTRIB_TITLE (), "");
  if (!title_text.empty ())
    this->xDummyTopNode.updateAttribute (title_text.c_str (), mxconst::get_ATTRIB_TITLE ().c_str (), mxconst::get_ATTRIB_TITLE ().c_str ());

  // add all <objective> nodes
  auto vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName (content_root_node, mxconst::get_ELEMENT_OBJECTIVES ());
  for (auto &node : vecNodes)
    Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xObjectives, node, mxconst::get_ELEMENT_OBJECTIVE (), true);

  // add all triggers
  vecNodes.clear ();
  vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName (content_root_node, mxconst::get_ELEMENT_TRIGGERS ());
  for (auto &node : vecNodes)
    Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xTriggers, node, mxconst::get_ELEMENT_TRIGGER (), true);

  // Add <message_templates> // v3.303.8
  vecNodes.clear ();
  vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName (content_root_node, mxconst::get_ELEMENT_MESSAGE ());
  for (auto &node : vecNodes)
    Utils::xml_add_node_to_parent_with_duplicate_filter (this->xMessages, node, mxconst::get_ELEMENT_MESSAGE (), mxconst::get_ATTRIB_NAME ());

  // Add <scriptlet> // v3.303.8
  vecNodes.clear ();
  vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName (content_root_node, mxconst::get_ELEMENT_SCRIPTLET ());
  for (auto &node : vecNodes)
    Utils::xml_add_node_to_parent_with_duplicate_filter (this->xEmbedScripts, node, mxconst::get_ELEMENT_SCRIPTLET (), mxconst::get_ATTRIB_NAME ());

  // Add GPS
  for (int i1 = 0; i1 < content_root_node.nChildNode (mxconst::get_ELEMENT_GPS ().c_str ()); ++i1)
  {
    auto node = content_root_node.getChildNode (mxconst::get_ELEMENT_GPS ().c_str (), i1);
    Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xGPS, node, mxconst::get_ELEMENT_POINT (), true);
  }
  // Add Inventory
  for (int i1 = 0; i1 < content_root_node.nChildNode (mxconst::get_ELEMENT_INVENTORIES ().c_str ()); ++i1)
  {
    auto node = content_root_node.getChildNode (mxconst::get_ELEMENT_INVENTORIES ().c_str (), i1);
    Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xInventoris, node, mxconst::get_ELEMENT_INVENTORY (), true);
    Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xInventoris, node, mxconst::get_ELEMENT_PLANE (), true);
  }

  // add all 3D object to <object_template>, starting from the second element since the first element is always this->x3DObjTemplate that way we won't have duplication of <obj3d> elements.
  if (this->x3DObjTemplate.isEmpty ())
    this->x3DObjTemplate = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_OBJECT_TEMPLATES ());

  const auto nObj3dTemplate = content_root_node.nChildNode (mxconst::get_ELEMENT_OBJECT_TEMPLATES ().c_str ());
  for (int i1 = 0; i1 < content_root_node.nChildNode (mxconst::get_ELEMENT_OBJECT_TEMPLATES ().c_str ()); ++i1)
  {
    auto node_obj_template = content_root_node.getChildNode (mxconst::get_ELEMENT_OBJECT_TEMPLATES ().c_str (), i1);
    Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->x3DObjTemplate, node_obj_template, mxconst::get_ELEMENT_OBJ3D (), true);
  }

  // add <choices>
  for (int i1 = 0; i1 < content_root_node.nChildNode (mxconst::get_ELEMENT_CHOICES ().c_str ()); ++i1)
  {
    auto node = content_root_node.getChildNode (mxconst::get_ELEMENT_CHOICES ().c_str (), i1);
    Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode (this->xChoices, node, mxconst::get_ELEMENT_CHOICE (), true);
  }

  // <global_settings>
  this->xGlobalSettings = content_root_node.getChildNode (mxconst::get_GLOBAL_SETTINGS ().c_str ()).deepCopy ();


  // v25.09.2 flight legs
  if (this->xFlightLegs.isEmpty ())
    this->xFlightLegs = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_FLIGHT_PLAN ());

  #ifndef RELEASE
  const int flight_plan_childs_i = content_root_node.nChildNode(mxconst::get_ELEMENT_FLIGHT_PLAN ().c_str());
  #endif

  // add all legs inside any <flight_plan> found in the root content template
  vecNodes.clear ();
  vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName (content_root_node, mxconst::get_ELEMENT_FLIGHT_PLAN ());
  for (auto &node_fp : vecNodes)
    this->xDummyTopNode.addChild (node_fp.deepCopy()); // add all <flight_plan> directly to the dummy top node (which represent the final mission node).

  // add any leg in the root content template
  vecNodes.clear ();
  vecNodes = Utils::xml_get_all_nodes_pointer_with_tagName (content_root_node, mxconst::get_ELEMENT_LEG ());
  for (auto &node : vecNodes)
    this->xFlightLegs.addChild ( node.deepCopy () );

  if (this->xpData.isEmpty ())
    this->xpData = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_XPDATA ());

  // <xpdata>
  this->xpData = content_root_node.getChildNode (mxconst::get_ELEMENT_XPDATA ().c_str ()).deepCopy ();

  #ifndef RELEASE
  Log::logMsgThread (fmt::format ("-------------- <CONTENT_MISSION> RESULTS - Post {} --------------", __func__));
  Log::logMsgThread (fmt::format ("BRIEFER_INFO:\n{}\n", Utils::xml_get_node_content_as_text (this->xBrieferInfo)));
  Log::logMsgThread (fmt::format ("BRIEFER:\n{}\n", Utils::xml_get_node_content_as_text (this->xBriefer) ) ); // we store the briefer in [0]
  Log::logMsgThread (fmt::format ("TRIGGERS:\n{}\n", Utils::xml_get_node_content_as_text (this->xTriggers)));
  Log::logMsgThread (fmt::format ("OBJECTIVES:\n{}\n", Utils::xml_get_node_content_as_text (this->xObjectives)));
  Log::logMsgThread (fmt::format ("FLIGHT LEGS:\n{}\n", Utils::xml_get_node_content_as_text (this->xFlightLegs)));
  Log::logMsgThread (fmt::format ("Inventories:\n{}\n", Utils::xml_get_node_content_as_text (this->xInventoris)));
  Log::logMsgThread (fmt::format ("GPS:\n{}\n", Utils::xml_get_node_content_as_text (this->xGPS)));
  Log::logMsgThread (fmt::format ("-------------- END <CONTENT_MISSION> RESULTS - {} --------------", __func__));
  #endif // !RELEASE

  return out_func_result = true;
}


// -----------------------------------


std::map<int, missionx::NavAidInfo>
RandomEngine::gen_get_generic_template_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr)
{
  std::map<int, missionx::NavAidInfo> target_navaids;
  outErr.clear ();

  int nChilds              = in_template_node.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());


  ///////////////////////////////////////////
  // ----------------------
  // -- Prepare <briefer> node - Start Location
  // ----------------------
  // // Prepare base briefer data, based on: parse <briefer_and_start_location> node
  IXMLNode x_briefer_and_start_location_node = in_template_node.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ().c_str ()).deepCopy ();
  target_navaids[0] = gen_briefer_phase_01_parse_briefer_and_start_location (in_template_node, x_briefer_and_start_location_node );
  if ( !target_navaids[0].is_lat_lon_valid () || !target_navaids[0].err.empty () )
  {
    outErr = fmt::format ("[{}] <{}> was not found in the template. Fix the template.", __func__, mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION () );
    target_navaids.clear ();
    return target_navaids;
  }


  // The number of a child node starts from 0 and not 1
  // ----------------------
  // -- Prepare <leg> nodes from template
  // ----------------------
  for (int i1 = 0; i1 < nChilds && !(missionx::RandomEngine::random_thread_state.flagAbortThread); ++i1)
  {
    IXMLNode x_leg_node = in_template_node.getChildNode (mxconst::get_ELEMENT_LEG ().c_str (), i1).deepCopy ();
    int leg_counter = static_cast<int>( target_navaids.size () );
    auto na = gen_parse_template_leg (&RandomEngine::random_thread_state, in_template_node, x_leg_node, RandomEngine::shared_navaid_info, target_navaids, leg_counter, (i1 + 1 == nChilds), outErr);

    // check abort
    if (missionx::RandomEngine::random_thread_state.flagAbortThread)
    {
      outErr = "User asked to Abort!";
      target_navaids.clear ();
      return target_navaids;
    }

    // Validate no errors during leg parsing from the template
    if (!na.is_lat_lon_valid () || !outErr.empty () || !na.err.empty ())
    {
      if (!na.err.empty ())
        outErr = na.err;

      target_navaids.clear ();
      return target_navaids;
    }

    na.fpln_xml_osm_q_or_raw_tmpl_node = x_leg_node.deepCopy ();

    target_navaids[static_cast<int> (target_navaids.size ())] = na; // The "zero" is kept for the briefer.

  } // end loop over all template <leg> nodes.

  return target_navaids;
}

// -----------------------------------

missionx::mx_return
RandomEngine::gen_prepare_random_mission_based_on_leg_nodes_in_template (IXMLNode &in_xTemplateNode)
{
  Log::logDebugBO (fmt::format("[{}] start.", __func__), true);

  missionx::mx_return out_func_result;
  int nChilds              = in_xTemplateNode.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());
  int flightLegs_counter_i = 1;
  // gather base information
  const int i_how_many_legs_user_picked = Utils::readNodeNumericAttrib<int> (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_NO_OF_LEGS (), 0);
  bool flag_user_asked_to_generate_only_one_leg = (i_how_many_legs_user_picked == 1);

  // std::map<int, NavAidInfo> navaid_targets;

  //-----------------------------------------------
  // Get all targets including the "starting location" (briefer)
  //-----------------------------------------------
  std::string outErr;
  std::map<int, NavAidInfo> navaid_targets = RandomEngine::gen_get_generic_template_targets (&RandomEngine::random_thread_state, in_xTemplateNode, RandomEngine::shared_navaid_info, outErr);

  if (missionx::RandomEngine::random_thread_state.flagAbortThread)
  {
    navaid_targets.clear ();
    out_func_result.result = false;
    return out_func_result;
  }

  if (!outErr.empty ())
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg (outErr, true);
    return out_func_result;
  }


  //-----------------------------------------------
  //--- Analyze Water Bodies / Slope / Leg Name ---
  //-----------------------------------------------
  bool flag_one_of_the_targets_above_water = false;
  for (auto &target_navaid : navaid_targets | std::views::values)
  {
    target_navaid.fpln_is_wet = get_is_wet_at_point (target_navaid);

    // store wet state if the "flag value" is not true, yet.
    if (!flag_one_of_the_targets_above_water)
      flag_one_of_the_targets_above_water = target_navaid.fpln_is_wet;

    // store slope at the target location
    target_navaid.fpln_slope = get_slope_at_point (target_navaid);

    target_navaid.fpln_leg_name = gen_leg_name ( &this->seq_waypoints, mxconst::get_GPS_WP (),"leg", target_navaid );
  }

  // validate navaid targets
  int valid_navaids_i = 0;
  auto navaids_validation = gen_validate_navaids (navaid_targets, valid_navaids_i);
  if (!navaids_validation.result) // if there is a failure
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg (navaids_validation.getErrorsAsText (), true);
    return out_func_result;
  }

  if ( valid_navaids_i != static_cast<int>(navaid_targets.size ()) )
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg ( fmt::format("Valid targets found: {}, is not the same as overall generated targets: {}", valid_navaids_i, navaid_targets.size ()), true);
    return out_func_result;
  }


  // ----------------------
  // -- Add <briefer> node - Start Location BUT NOT the description.
  // ----------------------
  navaid_targets[0].fpln_navaid_was_already_prepared = true; // force flag
  gen_briefer_phase_02_base_node_from_navaid (navaid_targets[0], RandomEngine::shared_navaid_info, flag_one_of_the_targets_above_water);

  // ------------------------------------------------------------------
  // Construct all mission <leg> nodes
  // navaid_targets: [0] = start/briefer, [1]..[N-1] legs.
  // ------------------------------------------------------------------

  // add "mission_type" attribute from <TEMPLATE>
  const std::string attrib_mission_type = mxUtils::stringToLower ( Utils::readAttrib (in_xTemplateNode, mxconst::get_ATTRIB_MISSION_TYPE (), "") );
  auto attrib_mission_type_enum = mxUtils::translate_mission_type_to_task_type (attrib_mission_type);
  mx_ui_mission_type translated_mission_type_to_ui_task_type = mxUtils::translate_mission_type_to_med_cargo_or_oilrig_task_type (attrib_mission_type);
  if (translated_mission_type_to_ui_task_type != mx_ui_mission_type::undefined)
  {
    Utils::xml_set_attribute_in_node <int>(this->xMetadata, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (translated_mission_type_to_ui_task_type), this->xMetadata.getName ());
    Utils::xml_set_attribute_in_node <int>(this->xMetadata, mxconst::get_ATTRIB_MISSION_TYPE (), static_cast<int> (attrib_mission_type_enum), this->xMetadata.getName ());
  }

  RandomEngine::gen_create_all_leg_nodes_based_on_navaid_targets (navaid_targets);

  for (auto &[indx, target_navaid] : navaid_targets)
  {
    // Add external inventory
    // will skip any navaid that is the same as the Start location.
    if (!target_navaid.flag_is_same_as_start_location ) //
    {
      target_navaid.fpln_xml_inv_node = gen_add_inventory_phase01_node (indx, target_navaid, map_osm_inventory_track);
      //  skip items phase, if it is the last location or inventory node is empty.
      if ( !target_navaid.fpln_xml_inv_node.isEmpty () && navaid_targets.contains (indx+1))
        gen_add_inventory_phase02_add_items (target_navaid);
    }

    if (indx == 0) // skip briefer
    {
      target_navaid.fpln_mission_phase = missionx::enums::mx_rnd_mission_phase::start;
      continue;
    }

    // add start messages
    gen_leg_start_messages (this->seq_messages, target_navaid, navaid_targets, this->xMessages, flag_one_of_the_targets_above_water);

    // add 3D leg hints
    if (!target_navaid.flag_is_skewed)
    {
      if (target_navaid.fpln_task_type < enums::mx_rnd_task_type::cargo) // meaning is medevac
      {
        auto next_leg_ptr = (navaid_targets.contains (indx+1))? &navaid_targets[indx+1] : nullptr;
        gen_3d_hint_objects_for_land_and_hover (target_navaid, target_navaid.fpln_xml_target_leg_node, next_leg_ptr);
      }
    }

    // add 3D display objects around the landing
    // v25.09.2 add support for <display_object_set>
    gen_3d_add_display_object_sets_instances_to_leg (target_navaid, target_navaid.fpln_xml_target_leg_node, in_xTemplateNode, this->x3DObjTemplate, this->expected_slope_at_target_location_d);

    // prepare the 3D instances
    gen_3d_parse_instances_in_leg (target_navaid.fpln_xml_target_leg_node, target_navaid);


    // v25.10.1
    const bool b_add_timers = Utils::readBoolAttrib (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_ADD_COUNTDOWN (), false);
    if (b_add_timers)
      RandomEngine::gen_inject_countdown_timer (indx, navaid_targets);

    // ADD <leg> XML Node
    target_navaid.synchToPoint ();
    target_navaid.fpln_xml_target_leg_node = this->xFlightLegs.addChild (target_navaid.fpln_xml_target_leg_node);

    // add task <trigger> nodes to main trigger node.
    for (auto &node : target_navaid.fpln_leg_vec_trigger_nodes)
      this->xTriggers.addChild (node.deepCopy ());

    // add target navaid <objective> node to the main objectives node
    this->xObjectives.addChild (target_navaid.fpln_leg_objective_node.deepCopy ());

  } // END loop over targets to add final touches to each flight leg and adding to the main xFlightLegs node.


  // prepare a flight plan to show the end user
  this->cumulative_location_desc_s = gen_get_cumulative_fpln_desc (navaid_targets);

  // check [abort]
  if (RandomEngine::random_thread_state.flagAbortThread)
  {
    out_func_result.addErrMsg ("User asked to abort.", true);
    return out_func_result;
  }

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
  gen_briefer_phase_03_add_desc (navaid_targets, flag_one_of_the_targets_above_water);
  this->xBriefer = navaid_targets[0].fpln_xml_target_leg_node.deepCopy ();

  // v25.10.1 Add Cold and dark
  RandomEngine::xDrefStartColdAndDark = gen_set_and_get_start_cold_and_dark (in_xTemplateNode, navaid_targets[1]);

  // add <mission_info>
  if (!gen_read_mission_info_element ()) // <mission_info>
  {
    missionx::RandomEngine::random_thread_state.flagAbortThread = true;
    out_func_result.addErrMsg ("No <mission_info> node was found in template.", true);
  }


  // Add all inventories to the global xInventories node
  for (auto &[key, nav] : navaid_targets )
  {
    // add to inventories
    if (!nav.fpln_xml_inv_node.isEmpty ())
      nav.fpln_xml_inv_node = this->xInventoris.addChild (nav.fpln_xml_inv_node);
  }


  #ifndef RELEASE
  Log::logMsgThread (fmt::format ("-------------- <CONTENT_MISSION> RESULTS - Post {} --------------", __func__));
  Log::logMsgThread (fmt::format ("BRIEFER_INFO:\n{}\n", Utils::xml_get_node_content_as_text (this->xBriefer)));
  Log::logMsgThread (fmt::format ("BRIEFER:\n{}\n", Utils::xml_get_node_content_as_text (navaid_targets[0].fpln_xml_target_leg_node))); // we store the briefer in [0]
  Log::logMsgThread (fmt::format ("TRIGGERS:\n{}\n", Utils::xml_get_node_content_as_text (this->xTriggers)));
  Log::logMsgThread (fmt::format ("OBJECTIVES:\n{}\n", Utils::xml_get_node_content_as_text (this->xObjectives)));
  Log::logMsgThread (fmt::format ("FLIGHT LEGS:\n{}\n", Utils::xml_get_node_content_as_text (this->xFlightLegs)));
  Log::logMsgThread (fmt::format ("Inventories:\n{}\n", Utils::xml_get_node_content_as_text (this->xInventoris)));
  Log::logMsgThread (fmt::format ("GPS:\n{}\n", Utils::xml_get_node_content_as_text (this->xGPS)));
  Log::logMsgThread (fmt::format ("-------------- END <CONTENT_MISSION> RESULTS - {} --------------", __func__));
  #endif // !RELEASE



  // func_result.addErrMsg ("W.I.P", true); // debug remove once all functions work as expected
  out_func_result.result = true;
  return out_func_result;
}

// -----------------------------------

void
RandomEngine::gen_create_all_leg_nodes_based_on_navaid_targets (std::map<int, NavAidInfo> &navaid_targets, const bool in_only_2_legs)
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

      if (target_navaid.fpln_is_last_flight_leg)
        target_navaid.fpln_mission_phase = enums::mx_rnd_mission_phase::land_extraction; // represent last waypoint
      else
        target_navaid.fpln_mission_phase = enums::mx_rnd_mission_phase::land_target; // represent target

      // We are basically constructing the mission from the middle waypoint and then need to add the start and end coordinates.
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
        { "radius", "length_mt", (target_navaid.fpln_wp_template_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())? mxconst::DEFAULT_LAND_OR_INV_RADIUS_MT.data() : mxconst::DEFAULT_HOVER_RADIUS_MT.data () }
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
      if (target_navaid.fpln_wp_template_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
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
      if (target_navaid.fpln_wp_template_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
        xTaskTargetHover = RandomEngine::gen_task_node (this->seq_tasks, "task", "hover", target_navaid, lsAttrib_hover_task_target, nullptr);

      // END Handling LAND + HOVER Triggers and Tasks


      // todo: move script creation after all targets were generated
      if (!target_navaid.fpln_xml_inv_node.isEmpty ())
      {
        // create scripts and attach them into the <inventory> as a subelement.
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
        { mxconst::get_ELEMENT_OUTCOME (), mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS (), fmt::format ("{}{}", task_land_name, (task_hover_name.empty () ? "" : "," + task_hover_name)) },
      };

      // Set Triggers post attributes
      if (target_navaid.fpln_wp_template_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
        Utils::xml_search_and_set_attributes_in_node (xTriggerTargetHover, lsAttrib_outcome_target_trig);
      Utils::xml_search_and_set_attributes_in_node (xTriggerTargetLand, lsAttrib_outcome_target_trig);

      // Link task to objective
      if (target_navaid.fpln_wp_template_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())
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
        const bool b_debug_leg_creation_success  = Utils::xml_set_tag_name (target_navaid.fpln_xml_target_leg_node, mxconst::get_ELEMENT_LEG ());
      }

      target_navaid.fpln_xml_target_leg_node = RandomEngine::gen_leg_node ( mxconst::get_GPS_WP (), "leg", &target_navaid, &lsAttrib_wp_target);
      #ifndef RELEASE
      Log::logMsgThread (fmt::format ("[{}] <leg> index: {}, Node element:\n{}\n<-----", __func__, target_navaid.fpln_seq, Utils::xml_get_node_content_as_text (target_navaid.fpln_xml_target_leg_node)));
      #endif


      // Test if the user asked for skew target location
      get_skew_target_data (target_navaid);

      // Store trigger and objective nodes to add to main mission file
      xTriggerTargetLand.updateAttribute ( mxconst::get_FL_TEMPLATE_VAL_LAND ().c_str (), mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ().c_str (), mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ().c_str ());
      target_navaid.fpln_leg_vec_trigger_nodes.push_back (xTriggerTargetLand);
      if (!xTriggerTargetHover.isEmpty ())
      {
        xTriggerTargetHover.updateAttribute ( mxconst::get_FL_TEMPLATE_VAL_HOVER ().c_str (), mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ().c_str (), mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ().c_str ());
        target_navaid.fpln_leg_vec_trigger_nodes.push_back (xTriggerTargetHover);
      }

      target_navaid.fpln_leg_vec_task_nodes.push_back (xTaskTargetLand);
      if (!xTaskTargetHover.isEmpty())
        target_navaid.fpln_leg_vec_task_nodes.push_back (xTaskTargetHover);

      target_navaid.fpln_leg_objective_node = xTargetObjective;

      // find elevation using call to the main thread
      target_navaid.height_mt = missionx::RandomEngine::get_terrain_elevation_at_point_in_mt (target_navaid);
      target_navaid.synchToPoint ();

      //-------------------------
      // Calculate distances, bearing and initialize the "next_leg" or "starting_leg" of the <leg>/<briefer> nodes
      //-------------------------
      if (navaid_targets.contains (indx - 1))
      {
        // store next target pointer
        const auto next_navaid_ptr = (navaid_targets.contains (indx + 1))? &navaid_targets[indx + 1] : nullptr;
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
        }

        // add <leg> description
        IXMLNode xml_desc_ptr = gen_leg_description (target_navaid.fpln_xml_target_leg_node, target_navaid, next_navaid_ptr);

        // add hint messages related to the target land/hover actions
        if (!in_only_2_legs || (in_only_2_legs && target_navaid.fpln_is_last_flight_leg))
        {
          gen_messages_when_reaching_target_leg (this->seq_triggers, this->seq_messages, target_navaid, this->xMetadata, this->xMessages, this->xTriggers, xTriggerTargetLand, xTriggerTargetHover);

          // generate a message when nearing the target (2nm)
          gen_2nm_to_N_nm_message (this->seq_triggers, this->seq_messages, target_navaid, this->xMessages, this->xTriggers, xTriggerTargetLand);
        }

        // is wet
        Utils::xml_set_attribute_in_node <bool>( target_navaid.fpln_xml_target_leg_node, mxconst::get_PROP_IS_WET(), target_navaid.fpln_is_wet, mxconst::get_ELEMENT_LEG ());

        // slope
        Utils::xml_set_attribute_in_node <double>( target_navaid.fpln_xml_target_leg_node, mxconst::get_ATTRIB_TERRAIN_SLOPE (), target_navaid.fpln_slope, mxconst::get_ELEMENT_LEG ());


      } // end if target navaid is not the first or last

    } // end "osm_target" loop over all OSM Target NavAids and construct the base information needed for the mission file

}

// -----------------------------------

std::string
RandomEngine::gen_get_cumulative_fpln_desc (std::map<int, NavAidInfo> &navaid_targets)
{
  const auto b_user_pref_display_all_flight_legs = missionx::system_actions::pluginSetupOptions.getNodeText_type_1_5 <bool>(mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE (), true);

  int counter = 0;
  std::string cumulative_fpln_desc;
  const auto last_seq_i = static_cast<int>(navaid_targets.size ()) - 1;


  for (auto &target_navaid : navaid_targets | std::views::values)
  {
    // std::string prefix = (target_navaid.flag_fetched_from_webosm)? "(webosm)": ""; // Done in Navaid level
    std::string postfix = ( last_seq_i == target_navaid.fpln_seq)? "" : ", ";

    // cumulative_fpln_desc += fmt::format ("{}{}{}", prefix, target_navaid.get_loc_desc (), postfix);
    cumulative_fpln_desc += fmt::format ("{}{}", target_navaid.get_loc_desc (), postfix);

    counter++;
    if ( counter > 1 && !b_user_pref_display_all_flight_legs )
      break; // exit the loop, shows only the first two legs, which are the briefer and first navaid
  }

  return cumulative_fpln_desc;
}

// -----------------------------------

missionx::mx_return
RandomEngine::gen_validate_navaids (std::map<int, NavAidInfo> &navaid_targets, int &inout_valid_navaids)
{
  std::string err;
  missionx::mx_return func_result = true; // we assume that all targets are valid

  inout_valid_navaids = 0;

  // loop over all navaids and conduct validations
  for (auto &[indx, navaid] : navaid_targets)
  {
    err.clear ();
    if (navaid.is_navaid_valid (err))
      inout_valid_navaids++;
    else
      func_result.addErrMsg (err, true);
  }

  return func_result;
}

// -----------------------------------

IXMLNode
RandomEngine::gen_set_and_get_start_cold_and_dark (IXMLNode &xTemplateNode, NavAidInfo &navaid)
{
  if (xTemplateNode.isEmpty () || !navaid.is_lat_lon_valid ())
  {
    Log::logMsgThread ( fmt::format("[{}] One of the data arguments is invalid.", __func__));
    return IXMLNode::emptyIXMLNode;
  }

  IXMLNode xDrefStartColdAndDark = xTemplateNode.getChildNode (mxconst::get_ELEMENT_DATAREFS_START_COLD_AND_DARK ().c_str ()).deepCopy ();

  if (xDrefStartColdAndDark.isEmpty ())
    return IXMLNode::emptyIXMLNode;

  std::string text = Utils::xml_get_text (xDrefStartColdAndDark);
  text             = Utils::replaceString (text, "{navaid_lat}", navaid.getLat (), true);
  text             = Utils::replaceString (text, "{navaid_lon}", navaid.getLat (), true);

  Utils::xml_set_text (xDrefStartColdAndDark, text);

  return xDrefStartColdAndDark;
}

  // -------------------------------------

bool
RandomEngine::gen_get_rw_metadata (const std::string &in_icao, int &out_rw_count, float &out_longest_rw)
{

  char *zErrMsg  = nullptr;
  out_rw_count   = 0;
  out_longest_rw = 0.0f;

  if (data_manager::db_xp_airports.db_is_open_and_ready)
  {
    int rc = 0;
    //// construct view query (inner query)
    // based on airports_vu  // we will pick the first result in the ordered result since it should reflect the closest airport based on its lat/lon
    const std::string sql_ap = fmt::format ("select count(1) as num_of_rw, max (rw_length_mt) as longest_rw from xp_rw where icao = '{}'", in_icao);
    #ifndef RELEASE
    Log::logMsgThread (fmt::format ("[{}] Search Airport Query for Runway metadata\n{}\n", __func__, sql_ap));
    #endif // !RELEASE

    data_manager::reasultTable.clear (); // clear table before adding new query output
    rc = sqlite3_exec (data_manager::db_xp_airports.db, sql_ap.c_str (), missionx::data_manager::callback_sqlite_data, nullptr, &zErrMsg);
    if (rc != SQLITE_OK)
    {
      Log::logMsgThread (fmt::format ("[{}] SQL Query Error: \n{}\n", __func__, zErrMsg));
      sqlite3_free (zErrMsg);

      return false;
    }

    // get data from table
    Log::logMsgThread (fmt::format ("[{}] Runway Metadata information was gathered.\n", __func__));
    if (!data_manager::reasultTable.empty ())
    {
      auto row       = data_manager::reasultTable.cbegin ()->second;
      out_rw_count   = std::atoi (row["num_of_rw"].c_str ());
      out_longest_rw = static_cast<float> (std::atof (row["longest_rw"].c_str ()));
    }

    return true;
  } // end if database is open

  return false;
}

// -----------------------------------



// --------------------------------

missionx::mx_return
RandomEngine::gen_get_ramp_based_on_plane_type (missionx::NavAidInfo &inout_target_navaid, const mx_plane_types_enum &in_plane_type_enum_to_search, const missionx::mxFilterRampType &inRampFilterType)
{
  char *zErrMsg = nullptr;
  missionx::mx_return result = true;
  missionx::mx_plane_types_enum local_plane_type_enum_to_search = in_plane_type_enum_to_search;

  auto aptNavLine = std::string (inout_target_navaid.name);

  if (data_manager::db_xp_airports.db_is_open_and_ready)
  {
    int rc = 0;
    //// construct view query (inner query)
    // based on airports_vu  // we will pick the first result in the ordered result since it should reflect the closest airport based on its lat/lon
    const std::string sql_ap = fmt::format (R"(select icao_id, icao, ap_elev_ft, ap_name, ap_type, ap_lat, ap_lon
                            , mx_calc_distance ( ap_lat, ap_lon, {}, {}, 3440) as dist_nm, 0 as bearing
                            , helipads, ramp_helos, ramp_planes, ramp_props, ramp_turboprops, ramp_jet_heavy, rw_hard, rw_dirt_gravel, rw_grass
                            , rw_water, is_custom from airports_vu where 1 = 1 and icao = '{}' order by dist_nm )",
                                            mxUtils::formatNumber<double> (inout_target_navaid.lat, 8),
                                            mxUtils::formatNumber<double> (inout_target_navaid.lon, 8),
                                            inout_target_navaid.getID ());

    #ifndef RELEASE
    Log::logMsgThread (fmt::format("[{}] Search Airport Query for ramps. Plane type: {}\n{}\n", __func__, translatePlaneTypeToString (in_plane_type_enum_to_search),  sql_ap) );
    #endif // !RELEASE

    // clear local cache
    RandomEngine::resultTable_gather_random_airports.clear ();
    rc = sqlite3_exec (data_manager::db_xp_airports.db, sql_ap.c_str (), RandomEngine::callback_gather_random_airports_db, nullptr, &zErrMsg);
    if (rc != SQLITE_OK)
    {
      Log::logMsgThread (fmt::format("[{}] SQL Query Error: \n{}\n", __func__, zErrMsg) );
      sqlite3_free (zErrMsg);
    }
    else
    {
      Log::logMsgThread (fmt::format("[{}] Ramp information was gathered.\n", __func__ ) );
      #ifndef RELEASE
      for (auto &[row_num, row_data] : RandomEngine::resultTable_gather_random_airports)
      {
        Log::logMsgThread (fmt::format ("[{}]\tSeq: {}, icao_id: {}, icao: {}, Distance: {}", __func__, mxUtils::formatNumber<int> (row_num), row_data["icao_id"], row_data["icao"], row_data["dist_nm"]));
      }
      #endif // !RELEASE

      if (RandomEngine::resultTable_gather_random_airports.empty ())
      {
        result.addErrMsg (fmt::format("[{}] No airports found relative to Navaid: {}.\n", __func__, inout_target_navaid.getID () ), true);
        return result;
      }
      auto ap_row = RandomEngine::resultTable_gather_random_airports.cbegin ()->second; // fetch the first result

      inout_target_navaid.flag_is_custom_scenery = (!(ap_row["is_custom"].empty ()));

      // to build the query based on plane types
      // we add space at the beginning of the filter
      int calculate_type_direction = -1; // -1 = drill down, +1 = drill up
      for (int loop01 = 0; loop01 < 4; ++loop01)
      {
        std::string ramp_filter_stmt_s;
        switch (local_plane_type_enum_to_search)
        {
          case missionx::mx_plane_types_enum::plane_type_any:
            ramp_filter_stmt_s = "";
            break;
          case missionx::mx_plane_types_enum::plane_type_helos:
            ramp_filter_stmt_s = " and helos > 0 "; // pick all airports that have helos ramps (heliports or any airport with helos in it). The view we use calculated the number of helos ramps so it is easy to distinguish between them.
            break;
          case missionx::mx_plane_types_enum::plane_type_ga_floats:
          case missionx::mx_plane_types_enum::plane_type_ga:
          case missionx::mx_plane_types_enum::plane_type_props:
            ramp_filter_stmt_s = " and props + turboprops > 0 and fighters = 0  "; // make sure only props locations are picked exclude "fighter" ramps
            break;
          case missionx::mx_plane_types_enum::plane_type_turboprops:
            ramp_filter_stmt_s = " and props + turboprops > 0 and fighters = 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
            break;
          case missionx::mx_plane_types_enum::plane_type_jets:
            ramp_filter_stmt_s = " and jet + terminal > 0 and fighters = 0 "; // make sure jet is being picked. Filter out turbo or prop candidates
            break;
          case missionx::mx_plane_types_enum::plane_type_heavy:
            ramp_filter_stmt_s = " and heavy + terminal > 0 and fighters = 0 "; // make sure heavy is being picked. Filter out turbo or prop candidates
            break;
          case missionx::mx_plane_types_enum::plane_type_fighter:
            ramp_filter_stmt_s = " and fighter > 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
            break;
          default:
            break;
        }

        const std::string select_s     = "select * from ramps_vu where 1 = 1 and icao_id = " + ap_row["icao_id"];
        const std::string filter_ramps = ramp_filter_stmt_s;
        const std::string sql_ramp     = select_s + filter_ramps + " ORDER BY RANDOM() limit 1";

        #ifndef RELEASE
        Log::logMsgThread (fmt::format("[{}] Ramp Q for type: {}\n{}\n", __func__, translatePlaneTypeToString (in_plane_type_enum_to_search),  sql_ramp) );
        #endif // !RELEASE

        RandomEngine::resultTable_gather_ramp_data.clear ();
        rc = sqlite3_exec (data_manager::db_xp_airports.db, sql_ramp.c_str (), RandomEngine::callback_pick_random_ramp_location_db, nullptr, &zErrMsg);
        if (rc != SQLITE_OK)
        {
          result.addErrMsg ( fmt::format("[{}] Error during ramp search for plane type: {}", __func__, translatePlaneTypeToString (in_plane_type_enum_to_search) ), true ); // debug
          result.addErrMsg (fmt::format("[{}] Fail to pick a ramp, SQL Error: \n{}\n", __func__, zErrMsg), true );
          sqlite3_free (zErrMsg);

          return result;
        }


        if (RandomEngine::resultTable_gather_ramp_data.empty ())
        {
          result.addInfoMsg (fmt::format ("[{}] No ramp was found for plane type: {}, should continue and search", __func__, translatePlaneTypeToString (in_plane_type_enum_to_search))); // debug

          if (missionx::mxFilterRampType::exact_plane_ramp_type == inRampFilterType)
            break; // exit the loop since we want the exact ramp type
          if (loop01 > 0) // if this is not the first iteration
          {
            // we try to search ramps that are "jets", then "turboprops" and then "prop".
            // we do not search for Helos, nor fighter ramps
            int plane_type_code = static_cast<int> (local_plane_type_enum_to_search);
            plane_type_code += calculate_type_direction; // decrease/increase the code number - will affect a ramp type based on the enum number
            local_plane_type_enum_to_search = static_cast<missionx::mx_plane_types_enum> (plane_type_code);

            // Check boundaries
            if (local_plane_type_enum_to_search <= missionx::mx_plane_types_enum::plane_type_helos || local_plane_type_enum_to_search > missionx::mx_plane_types_enum::plane_type_heavy)
              break; // exit the loop. We did not find a suitable ramp

            result.addInfoMsg (fmt::format ("[{}]: Search for alternate ramp type for plane: {}", __func__, translatePlaneTypeToString (local_plane_type_enum_to_search))); // debug
          }
          else
          {
            // based on plane type, decide if to drill down or up
            if ( mxUtils::mx_between <int>(static_cast<int> (in_plane_type_enum_to_search), static_cast<int> (mx_plane_types_enum::plane_type_helos), static_cast<int> (mx_plane_types_enum::plane_type_turboprops), enums::mx_between_types::gt_min_less_max ))
            {
              local_plane_type_enum_to_search = missionx::mx_plane_types_enum::plane_type_turboprops;
              calculate_type_direction = 1; // we drill up
            }
            else if (in_plane_type_enum_to_search == missionx::mx_plane_types_enum::plane_type_helos || in_plane_type_enum_to_search == missionx::mx_plane_types_enum::plane_type_fighter)
            {
              local_plane_type_enum_to_search = missionx::mx_plane_types_enum::plane_type_ga;
              calculate_type_direction = 1; // we drill up
            }
            else
            {
              local_plane_type_enum_to_search = missionx::mx_plane_types_enum::plane_type_heavy;
              calculate_type_direction = -1;
            }
          }

        } // end did not find ramp in database
        else
        { // found ramp
          // Store ramp location in navaid
          Log::logMsgThread ("[pick ramp] Ramp info gathered.");
          auto ramp                                     = resultTable_gather_ramp_data.cbegin ()->second;
          inout_target_navaid.lat                       = mxUtils::stringToNumber<float> (ramp["lat"], ramp["lat"].length ());
          inout_target_navaid.lon                       = mxUtils::stringToNumber<float> (ramp["lon"], ramp["lon"].length ());
          inout_target_navaid.heading                   = mxUtils::stringToNumber<float> (ramp["heading"], ramp["heading"].length ());
          inout_target_navaid.ramp_info.uq_name         = ramp["name"];
          inout_target_navaid.ramp_info.ramp_for_planes = ramp["for_planes"];

          #ifndef RELEASE
          for (auto &row_val : resultTable_gather_ramp_data | std::views::values)
          {
            // Log::logMsgThread (fmt::format("\rRamp: " + row_val["name"] + ", icao_id: " + row_val["icao_id"] + ", icao: " + row_val["icao"]) );
            Log::logMsgThread (fmt::format ("\rRamp: {}, icao_id: {}, icao: {}", row_val["name"], row_val["icao_id"], row_val["icao"]));
          }
          #endif // !RELEASE

          // // revert the template type if and only if it is different. Main reason is if we do not find a ramp location for our plane then we try to find a ramp based on other plane types
          // local_plane_type_enum_to_search = in_plane_type_enum_to_search;

          inout_target_navaid.synchToPoint ();
          result = true;
          return result; // exit the loop
        }
        // end if an airport result is not empty and we should search for ramp location

      } // end loop

      // revert the plane type to its original value
      local_plane_type_enum_to_search = in_plane_type_enum_to_search;


      // If we reached this location than we failed to find a valid ramp position. We will use the runway as a ramp location.
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

      std::string query_start_pos_s = lmbda_get_query_for_fallback_position_based_on_filter_type (inRampFilterType, inout_target_navaid);
      #ifndef RELEASE
      Log::logMsgThread ("SQL Query to Fetch start pos: \n" + query_start_pos_s + "\n");
      #endif // !RELEASE

      if (!query_start_pos_s.empty ())
      {
        resultTable_gather_ramp_data.clear ();
        rc = sqlite3_exec (data_manager::db_xp_airports.db, query_start_pos_s.c_str (), RandomEngine::callback_pick_random_ramp_location_db, nullptr, &zErrMsg);
        if (rc != SQLITE_OK)
        {
          result.addInfoMsg ( fmt::format( "[{}] No ramp was found for plane type: {}", __func__, translatePlaneTypeToString (local_plane_type_enum_to_search), false ) );
          result.addInfoMsg ( fmt::format( "[{}] SQL error: {}", __func__, zErrMsg, false ) );
          sqlite3_free (zErrMsg);
        }
        else
        {
          if (RandomEngine::resultTable_gather_ramp_data.empty ())
            Log::logMsgThread ("[pick ramp] No valid start position was found.");
          else
          {
            Log::logMsgThread ("[pick ramp] Start position info gathered.");
            auto                     ramp        = resultTable_gather_ramp_data.cbegin ()->second;
            std::vector<std::string> vecPosition = mxUtils::split (ramp["start_pos"], ',');

            if (vecPosition.size () > static_cast<size_t> (1))
            {
              // Store location in Navaid
              inout_target_navaid.lat               = mxUtils::stringToNumber<float> (vecPosition.at (0), vecPosition.at (0).length ());
              inout_target_navaid.lon               = mxUtils::stringToNumber<float> (vecPosition.at (1), vecPosition.at (1).length ());
              inout_target_navaid.heading           = mxUtils::stringToNumber<float> (ramp["heading"], 6);
              inout_target_navaid.ramp_info.uq_name = ramp["name"];
              inout_target_navaid.ramp_info.ramp_for_planes    = "Runway: " + inout_target_navaid.ramp_info.uq_name;

              inout_target_navaid.synchToPoint ();
              return result = true;
            }
          } // end if we fetched the center of the runway as the ramp data

        } // end if sqlite statement is legit one

      } // end if we have query for fallback start position - either start of a runway or the center of the runway.

    } // end if airport information query returned data

  } // end if Database is open


  return result;
}


// --------------------------------

bool
RandomEngine::filterAndPickRampBasedOnPlaneType (missionx::NavAidInfo &navAid, std::string &outErrorMsg, const missionx::mxFilterRampType &inRampFilterType) // const bool& inIgnoreCenterOfRunwayAsRamp)
{
  char *zErrMsg = nullptr;

  std::string                     err;
  // missionx::mx_aptdat_cached_info navData;
  auto                            aptNavLine = std::string (navAid.name);

  outErrorMsg.clear ();

  if ((missionx::RandomEngine::random_thread_state.flagAbortThread))
  {
    outErrorMsg = "Need to abort";
    return false;
  }

  mx_plane_types_enum plane_type_enum_to_search = RandomEngine::template_plane_type_enum;

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
      auto ap_row = RandomEngine::resultTable_gather_random_airports.cbegin ()->second; // fetch the first result

      navAid.flag_is_custom_scenery = (!(ap_row["is_custom"].empty ()));

      // build the query based on plane types
      // we add space at the beginning of the filter
      for (int loop01 = 0; loop01 < 4; ++loop01)
      {
        std::string ramp_filter_stmt_s;
        switch (plane_type_enum_to_search)
        {
          case missionx::mx_plane_types_enum::plane_type_any:
            ramp_filter_stmt_s = "";
            break;
          case missionx::mx_plane_types_enum::plane_type_helos:
            ramp_filter_stmt_s = " and helos > 0 "; // pick all airports that have helos ramps (heliports or any airport with helos in it). The view we use calculated the number of helos ramps so it is easy to distinguish between them.
            break;
          case missionx::mx_plane_types_enum::plane_type_ga_floats:
          case missionx::mx_plane_types_enum::plane_type_ga:
          case missionx::mx_plane_types_enum::plane_type_props:
            ramp_filter_stmt_s = " and props + turboprops > 0 and lower(for_planes) not like '%fighter%' "; // make sure only props locations are picked exclude "fighter" ramps
            break;
          case missionx::mx_plane_types_enum::plane_type_turboprops:
            ramp_filter_stmt_s = " and props + turboprops > 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
            break;
          case missionx::mx_plane_types_enum::plane_type_jets:
          case missionx::mx_plane_types_enum::plane_type_heavy:
            ramp_filter_stmt_s = " and jet_n_heavy > 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
            break;
          case missionx::mx_plane_types_enum::plane_type_fighter:
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
              plane_type_enum_to_search = static_cast<missionx::mx_plane_types_enum> (i1);
              outErrorMsg += ": try ramp type for: " + translatePlaneTypeToString (plane_type_enum_to_search); // debug
            }
            else
            {
              plane_type_enum_to_search = missionx::mx_plane_types_enum::plane_type_jets;
            }

            Log::logMsgThread ("[pick ramp] SQL error: " + outErrorMsg);
          }
          else
          {
            // Store ramp location in navaid
            Log::logMsgThread ("[pick ramp] Ramp info gathered.");
            auto ramp                = resultTable_gather_ramp_data.cbegin ()->second;
            navAid.lat               = mxUtils::stringToNumber<float> (ramp["lat"], ramp["lat"].length ());
            navAid.lon               = mxUtils::stringToNumber<float> (ramp["lon"], ramp["lon"].length ());
            navAid.heading           = mxUtils::stringToNumber<float> (ramp["heading"], ramp["heading"].length ());
            navAid.ramp_info.uq_name = ramp["name"];
            navAid.ramp_info.ramp_for_planes    = ramp["for_planes"];

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
            auto                     ramp        = resultTable_gather_ramp_data.cbegin ()->second;
            std::vector<std::string> vecPosition = mxUtils::split (ramp["start_pos"], ',');

            if (vecPosition.size () > static_cast<size_t> (1))
            {
              // Store location in Navaid
              navAid.lat               = mxUtils::stringToNumber<float> (vecPosition.at (0), vecPosition.at (0).length ());
              navAid.lon               = mxUtils::stringToNumber<float> (vecPosition.at (1), vecPosition.at (1).length ());
              navAid.heading           = mxUtils::stringToNumber<float> (ramp["heading"], 6);
              navAid.ramp_info.uq_name = ramp["name"];
              navAid.ramp_info.ramp_for_planes    = "Runway: " + navAid.ramp_info.uq_name;

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



// void
// RandomEngine::injectMissionTypeFeatures ()
// {
//   // 1. LOOP over all flight legs
//   // 1. add first Leg starting message - "hello pilot, check your GPS, fly to the landing site and pick the injured person."
//   // 2. Loop over each flight leg and check if it has next flight leg, if so, then add message to check gps and fly to next leg. If not then construct last location message.
//   // 3. Allow custom flight leg description
//   constexpr static int FIRST_LEG_INDEX = 0;
//   std::string          err;
//
//
//   #ifndef RELEASE
//   Log::logMsg ("[DEBUG random] injectMissionTypeFeatures.", true);
//   #endif
//
//   const int nChilds = this->xFlightLegs.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());
//
//   // REVERSE ordering so we could flag is Wet and calculate distances.
//   bool flag_has_wet_target = false; // v3.0.241.8 help to flag
//   for (int i1 = nChilds - 1; i1 >= 0; --i1)
//   {
//     IXMLNode leg_ptr = xFlightLegs.getChildNode (mxconst::get_ELEMENT_LEG ().c_str (), i1); // pointer to <leg> xml element
//     if (leg_ptr.isEmpty ())
//       continue;
//
//     IXMLNode    msg         = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE ()); // copy of <message> node, not a pointer
//     IXMLNode    textMix_ptr = Utils::xml_get_node_from_node_tree_IXMLNode (msg, mxconst::get_ELEMENT_MIX (), false); // pointer to
//     std::string message;
//     std::string message_name;
//     std::string flight_leg_name           = Utils::readAttrib (leg_ptr, mxconst::get_ATTRIB_NAME (), "");
//     std::string loc_desc                  = Utils::readAttrib (leg_ptr, mxconst::get_ATTRIB_LOC_DESC (), ""); // short description of the random point generated. <leg> element should not have it anymore, since we store it in NavAid. We need to check <special_flight_leg_directive> element.
//     std::string loc_desc_short            = loc_desc; // short description of the random point generated. <leg> element should not have it anymore, since we store it in NavAid. We need to check <special_flight_leg_directive> element.
//     double      distance_to_prev_navaid_d = -1.0; // negative distance = invalid
//     const bool  flag_isWet                = Utils::readBoolAttrib (leg_ptr, mxconst::get_PROP_IS_WET (), false);
//     // v25.05.1 nav info
//     missionx::NavAidInfo target_nav;
//
//     if (!flag_has_wet_target)
//       flag_has_wet_target = flag_isWet;
//
//     if (loc_desc.empty ()) // v3.0.221.10 try to fetch location description from navaid with same flight leg name
//     {
//       NavAidInfo *prevNav    = nullptr; // v3.0.251.1 b2 add distance to flight leg description
//       bool        bFirstTime = true;
//       for (auto &nav : RandomEngine::listNavInfo)
//       {
//
//         if (nav.flightLegName == flight_leg_name) // same unique Leg name
//         {
//           if (nav.loc_desc.empty ())
//             nav.init_locDesc ();
//
//           // loc_desc       = (nav.loc_desc.empty ()) ? nav.init_locDesc () : nav.loc_desc;
//           loc_desc       = nav.get_loc_desc ();
//           loc_desc_short = nav.gen_locDesc_short ();
//           target_nav.clone (nav);
//
//           if (!bFirstTime) // we can calculate distance
//           {
//             distance_to_prev_navaid_d = nav.p - prevNav->p;
//             prevNav                   = &nav;
//           }
//
//           break;
//         }
//
//         if (bFirstTime) // should be briefer NavAid
//         {
//           bFirstTime                = false;
//           distance_to_prev_navaid_d = -1;
//         }
//
//         prevNav = &nav;
//       }
//     }
//
//     // v3.0.221.11 search for custom flight leg message
//     std::string customLegDescText;
//     std::string custom_leg_desc_flag = Utils::stringToLower (Utils::xml_get_attribute_value_drill (leg_ptr, mxconst::get_ATTRIB_CUSTOM_FLIGHT_LEG_DESC_FLAG (), this->flag_found, mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES ())); // v3.0.221.15rc5 add LEG support
//     if (mxconst::get_MX_YES () == custom_leg_desc_flag)
//     {
//       IXMLNode    xDesc = leg_ptr.getChildNode (mxconst::get_ELEMENT_DESC ().c_str ());
//       std::string flightLegDesc;
//       if (!xDesc.isEmpty ())
//         customLegDescText = ((xDesc.nClear () > 0) ? xDesc.getClear ().sValue : missionx::EMPTY_STRING); // description of task: <task ...><![CDATA[task description]]></task>. // NO <desc> element
//     }
//
//     // if (loc_desc_short.empty ())
//     //   int iStop = 0;
//     // v3.0.241.9 store leg locations in a string to display in the briefer.
//     if (i1 == (nChilds - 1)) // our loop is from end to start
//       cumulative_location_desc_s = loc_desc_short + ((distance_to_prev_navaid_d > -1) ? "(" + Utils::formatNumber<double> (distance_to_prev_navaid_d, 2) + " nm)" : ""); // v3.0.251.1 b2 add distances
//     else
//     {
//       cumulative_location_desc_s = loc_desc_short + ((distance_to_prev_navaid_d > -1) ? "(" + Utils::formatNumber<double> (distance_to_prev_navaid_d, 2) + " nm)" : "") + ", " + cumulative_location_desc_s;
//       if (i1 == FIRST_LEG_INDEX) // first location. Used with the setup option "Expose all GPS legs at mission start = false"
//         first_location_desc_s = loc_desc_short + ((distance_to_prev_navaid_d > -1) ? "(" + Utils::formatNumber<double> (distance_to_prev_navaid_d, 2) + " nm)" : "");
//     }
//
//     message_name = "leg_" + ((flight_leg_name.empty ()) ? Utils::formatNumber<int> (i1) : flight_leg_name) + "_start_message";
//
//     if (i1 == 0) // start <leg>. Create message
//     {
//       if (customLegDescText.empty ())
//       {
//
//         // unknown location means that we do not have a unique name. The name has "coordinate" or "leg" in it.
//         const bool        bUnknownLocation = !(target_nav.nav_aid_has_unique_name ()); // v25.06.1
//         const std::string start_icao_desc  = (this->briefer_starting_location_desc.empty ()) ? "" : std::string (this->briefer_starting_location_desc).append ("\n");
//         const std::string target_loc_desc  = (bUnknownLocation) ? fmt::format ("Head to coordinates: {:.9}/{:.10}\nFly safe.", target_nav.lat, target_nav.lon) : fmt::format (R"("Head to {}". Fly safe.)", loc_desc_short);
//
//         if (flag_isWet)
//           message = fmt::format ("Hello pilot. We have uploaded flight coordinates to your GPS.\n{}One of the locations is above water body.\n{}", start_icao_desc, target_loc_desc);
//         else
//           message = fmt::format ("Hello pilot. We have uploaded flight coordinates to your GPS.\n{}{}", start_icao_desc , target_loc_desc);
//       }
//       else
//         message = customLegDescText;
//     }
//     else
//     {
//       // Handle rest of flight legs
//       const std::string next_flight_leg = Utils::readAttrib (leg_ptr, mxconst::get_ATTRIB_NEXT_LEG (), "");
//
//       if (customLegDescText.empty ())
//       {
//         if (flag_found && next_flight_leg.empty ()) // if we found attrib and next_leg is empty then it means that we are at the last flight leg
//         {
//           message = "Fly to last GPS location " + ((loc_desc.empty ()) ? "" : fmt::format (R"("{}". Land safely.)", loc_desc));
//         }
//         else if (flag_isWet)
//         {
//           message = "Fly to the next GPS location " + ((loc_desc.empty ()) ? "(" + next_flight_leg + ")" : fmt::format (R"("{}", it should be above water body.)", loc_desc)); // v3.0.241.8
//         }
//         else
//           message = "Fly to the next GPS location " + ((loc_desc.empty ()) ? "(" + next_flight_leg + ")" : fmt::format (R"("{}")", loc_desc));
//       }
//       else
//         message = customLegDescText;
//     }
//
//     // Add the message to mission file and flight leg element
//     if (textMix_ptr.isEmpty () || !Utils::xml_add_cdata (textMix_ptr, message))
//     {
//       #ifndef RELEASE
//       Log::logMsgWarn ("[random inject medevac] Message element is NULL. Check Template.", true);
//       continue;
//       #endif
//     }
//
//     Utils::xml_search_and_set_attribute_in_IXMLNode (msg, mxconst::get_ATTRIB_NAME (), message_name, mxconst::get_ELEMENT_MESSAGE ());
//     this->xMessages.addChild (msg);
//     // add as start_message to flight leg
//     Utils::xml_search_and_set_attribute_in_IXMLNode (leg_ptr, mxconst::get_ATTRIB_NAME (), message_name, mxconst::get_ELEMENT_START_LEG_MESSAGE ());
//     Utils::add_xml_comment (xMessages, " [[[[ ]]]] "); // add comment between 2 messages
//     // end setting XML with message data
//
//
//   } // END LOOP over all flight leg
//
//
//
//   #ifndef RELEASE
//   auto med_cargo_or_oilrig_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::not_defined));
//   Log::logMsgThread ("med_cargo_or_oilrig_i: " + mxUtils::formatNumber<int> (med_cargo_or_oilrig_i));
//   #endif // !RELEASE
//
//   if (Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::not_defined)) == static_cast<int> (missionx::mx_ui_mission_type::oil_rig))
//   {
//     // add briefer start location.
//     if (RandomEngine::listNavInfo.empty () == false)
//     {
//       NavAidInfo na = RandomEngine::listNavInfo.front ();
//       if (mxconst::get_ELEMENT_BRIEFER () == na.flightLegName)
//         cumulative_location_desc_s = "(start): " + na.gen_locDesc_short () + ", " + cumulative_location_desc_s;
//     }
//   }
//
//   //// v3.0.241.9 Add custom briefer description if it is a mission UI based ("WinBrieferGL::user_driven_mission_layer") and its <![CDATA[ ]]> is empty.
//   if (this->flag_rules_defined_by_user_ui)
//   {
//     std::string briefer_desc;
//     if (!this->xBriefer.isEmpty ())
//       briefer_desc = ((xBriefer.nClear () > 0) ? xBriefer.getClear ().sValue : ""); // description of task: <task ...><![CDATA[task description]]></task>. // NO <desc> element
//
//     briefer_desc = mxUtils::trim (briefer_desc);
//     if (briefer_desc.empty () && !xBriefer.isEmpty ())
//     {
//       briefer_desc = this->briefer_skeleton_message_to_use_in_injectTypeMissionFeature + "\n";
//       briefer_desc += this->briefer_starting_location_desc; // v25.05.1
//       briefer_desc += (flag_has_wet_target) ? "\nOne of the flight legs is in a water body, make sure you have all needed equipment. " : "";
//
//       // v25.02.1 adding support for LAND_HOVER cases
//       if (mxconst::get_FL_TEMPLATE_VAL_HOVER () == Utils::readAttrib (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE (), ""))
//         briefer_desc += "\nWe believe you will have to hover above one of the locations, due to the physical terrain limitations.\nMake sure you have the right plane for this mission.";
//       else if (mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER () == Utils::readAttrib (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_ATTRIB_SHARED_TEMPLATE_TYPE (), ""))
//         briefer_desc += "\nWe believe you could Land or Hover above one of the locations, due to the physical terrain limitations.\nMake sure you have the right plane for this mission.";
//
//
//       // v25.04.2 - fixed destination exposure, based on setup
//       if (missionx::system_actions::pluginSetupOptions.getNodeText_type_1_5<bool> (mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE (), true))
//         briefer_desc += "\nExpected route: " + cumulative_location_desc_s + ".";
//       else
//         briefer_desc += "\nFirst waypoint: " + first_location_desc_s + ".";
//
//       briefer_desc += "\n\nFly Safe !!!";
//
//       Utils::xml_add_cdata (xBriefer, briefer_desc);
//     }
//   }
//
//   #ifndef RELEASE
//   // if (mxUtils::trim (this->cumulative_location_desc_s).back () == ',')
//   //   Log::logMsgThread ("Ends with ',' !!!");
//
//   Log::logMsg ("[DEBUG random] after injectMissionTypeFeatures.", true);
//   #endif
//
//   // end injectMissionTypeFeatures
// }

// -----------------------------------------

// void
// RandomEngine::injectMessagesWhileFlyingToDestination ()
// {
//   // Build messages relative to distance between 2 legs as a factor of distance.
//   // prepare data before looping over legs
//   // loop over legs and get distances between prev and post
//   // v3.0.221.9 try to use the information in "special_flight_leg_directives" legs sub element.
//
//   // find briefer point
//   if (xBriefer.isEmpty () || xBriefer.getChildNode (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ()).isEmpty ())
//   {
//     RandomEngine::setError ("[random message] Briefer node is not valid");
//     return;
//   }
//
//   // prepare trigger node
//   IXMLNode trig_template_node = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_TRIGGER ()); // return copy of trigger node
//   if (trig_template_node.isEmpty ())
//   {
//     Log::logMsgWarn ("[random message] Fail to find <trigger> in template", true);
//     return;
//   }
//
//   // check for outcome node
//   IXMLNode xOutcome = trig_template_node.getChildNode (mxconst::get_ELEMENT_OUTCOME ().c_str ());
//   if (xOutcome.isEmpty ())
//     xOutcome = trig_template_node.addChild (mxconst::get_ELEMENT_OUTCOME ().c_str ());
//
//
//   // prepare message node from template
//   IXMLNode message_node = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE ()); // return copy of Message node
//   if (message_node.isEmpty ())
//   {
//     Log::logMsgWarn ("[random message] Fail to find <message> in template", true);
//     return;
//   }
//
//   const int nLegChilds = this->xFlightLegs.nChildNode (mxconst::get_ELEMENT_LEG ().c_str ());
//   int       navCounter = 0;
//   int       legCounter = 0;
//
//
//   NavAidInfo prevNa, currentNa;
//   for (const auto &na : RandomEngine::listNavInfo)
//   {
//     if (navCounter == 0)
//     {
//       ++navCounter;
//       currentNa = na; // this should be the briefer
//       continue;
//     }
//
//     prevNa.init ();
//     prevNa = currentNa;
//     prevNa.synchToPoint ();
//
//     currentNa.init ();
//     currentNa = na;
//     currentNa.synchToPoint ();
//
//     IXMLNode xmlDataNode_ptr;
//     // get flight leg
//     IXMLNode legNode = xFlightLegs.getChildNode (mxconst::get_ELEMENT_LEG ().c_str (), legCounter);
//     if (legNode.isEmpty ())
//       continue;
//
//     if (legCounter >= nLegChilds) // should never happen
//       break;
//
//     // v3.0.223.4
//     std::string err;
//     const bool  flag_disable_auto_messages = currentNa.getBoolValue (mxconst::get_ATTRIB_DISABLE_AUTO_MESSAGE_B (), false); // v3.303.14r1 fixed reading the bool value. Changed from numeric read to also support string boolean representation
//
//     xmlDataNode_ptr = legNode; // v3.0.221.9 // v3.0.223.4 Consider removing this line since we need to work only with <special_directive /> element.
//
//     // search Special Flight Leg directive node.
//     IXMLNode sNode = legNode.getChildNode (mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES ().c_str ());
//     if (!sNode.isEmpty ())
//       xmlDataNode_ptr = sNode; // v3.0.221.9
//
//
//     ++legCounter;
//     ++navCounter;
//     if (!flag_disable_auto_messages) // create or skip auto distance messages
//     {
//
//       // get trigger point
//       #ifndef RELEASE
//       std::string debugNaFlightLegName = currentNa.flightLegName; // debug - use with debugger
//       #endif
//
//       std::string flightLegName   = Utils::readAttrib (legNode, mxconst::get_ATTRIB_NAME (), "");
//       std::string legTemplateType = Utils::readAttrib (xmlDataNode_ptr, mxconst::get_ATTRIB_SHARED_FLIGHT_LEG_TEMPLATE (), ""); // support leg and keep compatibility
//
//
//       // v3.0.241.8 prepare two point classes based on the NavAidInfo.flag_is_skewed_point and calculate accordingly
//       Point pCurr, pPrev;
//       pCurr.node = (currentNa.flag_is_skewed) ? currentNa.xml_skewdPointNode : currentNa.p.node;
//       pPrev.node = (prevNa.flag_is_skewed) ? prevNa.xml_skewdPointNode : prevNa.p.node;
//
//       assert (!pCurr.node.isEmpty () && !pPrev.node.isEmpty ());
//
//       double distance_nm;
//       bool   flag_msg_skewed = false; // v3.0.241.8 influence message
//       if (currentNa.flag_is_skewed && pCurr.parse_node () && pPrev.parse_node ())
//       {
//         flag_msg_skewed = true;
//         distance_nm     = Point::calcDistanceBetween2Points (pCurr, pPrev);
//       }
//       else
//         distance_nm = Point::calcDistanceBetween2Points (currentNa.p, prevNa.p); // fallback - may cause some errors, but its better then nothing
//
//
//       if (distance_nm < 0.0)
//       {
//         Log::logMsgErr ("[inject message] Found <leg> without distance_nm attribute. Skipping message for <leg>: " + flightLegName + ". Notify developer.", true);
//         continue;
//       }
//       else
//       {
//         if (distance_nm < 2.0) // minimal message distance should be 2nm
//           distance_nm = 2.0;
//
//
//         /////////////////////////
//         // add distance messages
//         const std::string   message_distances = "2,5,15,25,40,60";
//         std::vector<double> vecDistances      = Utils::splitStringToNumbers<double> (message_distances, mxconst::get_COMMA_DELIMITER ()); // mxconst::get_COMMA_DELIMITER() = ","
//
//
//         // comment separator
//         Utils::add_xml_comment (xTriggers, " ++++ " + flightLegName + " distance messages +++++ "); // v3.0.219.3
//
//         // loop over vector
//         int counter = 0;
//         for (const auto dist : vecDistances)
//         {
//           IXMLNode trigNode = trig_template_node.deepCopy ();
//           if (trigNode.isEmpty ())
//             continue;
//
//           if (!(distance_nm > dist) && !(distance_nm < 2.0)) // skip message that meant for longer distance. Ie, if distance to target is 5, then do not create message that is meant for 10nm.
//             continue;
//
//           // set trigger Name by distance
//           const std::string newTriggerName = "message_trig_for_" + flightLegName + "_(" + Utils::formatNumber<double> (dist) + "nm)"; // message_trig_for_leg_1_(5nm)
//           Utils::xml_search_and_set_attribute_in_IXMLNode (trigNode, mxconst::get_ATTRIB_NAME (), newTriggerName, mxconst::get_ELEMENT_TRIGGER ());
//           Utils::xml_search_and_set_attribute_in_IXMLNode (trigNode, mxconst::get_ATTRIB_PLANE_ON_GROUND (), missionx::EMPTY_STRING, mxconst::get_ELEMENT_CONDITIONS ()); // remove on_ground attribute
//           Utils::xml_search_and_set_attribute_in_IXMLNode (trigNode, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED (), newTriggerName, mxconst::get_ELEMENT_OUTCOME ()); // set the message name as the "trigger name" for when_fired
//
//           // set the message name when entering trigger zone
//           std::string message = "You are: " + Utils::formatNumber<double> (dist) + " nautical miles from target.";
//           if (counter == 0) // the closest message to target
//           {
//             if (mxconst::get_FL_TEMPLATE_VAL_HOVER () == legTemplateType)
//             {
//               if (flag_msg_skewed)
//                 message += " You should look for the target, we did not receive an exact location, it should be near. Remember to hover above it once you reached it."; // v3.0.241.8 added skewed string
//               else
//                 message += " You should look for the target location to hover."; // v3.0.241.8 added skewed string
//             }
//
//             else if (mxconst::get_FL_TEMPLATE_VAL_LAND () == legTemplateType)
//             {
//               if (flag_msg_skewed)
//                 message += " target should be around this location. Once you locate it, land carefully."; // v3.0.241.8 added skewed string
//               else
//                 message += " You are nearing your target destination."; // v3.0.241.8 modified message
//             }
//             else
//             {
//               message += " You should look for the target location."; // v3.0.241.8 added skewed string
//             }
//           }
//
//           ++counter;
//
//           // Define Triggers message Radius
//           const int distance_mt = static_cast<int> (dist * missionx::nm2meter);
//
//           if (IXMLNode rNode_ptr = Utils::xml_get_node_from_node_tree_IXMLNode (trigNode, mxconst::get_ELEMENT_RADIUS (), false); rNode_ptr.isEmpty ())
//           {
//             rNode_ptr = trigNode.addChild (mxconst::get_ELEMENT_RADIUS ().c_str ());
//             Utils::xml_add_node_to_element_IXMLNode (trigNode, rNode_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ()); // place node in correct location
//           }
//
//           // set radius length attribute
//           Utils::xml_search_and_set_attribute_in_IXMLNode (trigNode, mxconst::get_ATTRIB_LENGTH_MT (), Utils::formatNumber<int> (distance_mt), mxconst::get_ELEMENT_RADIUS ());
//
//           //// Define <message>
//           IXMLNode mNode       = message_node.deepCopy ();
//           IXMLNode textMixNode = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (mNode, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT (), false); // direct pointer to Mix node
//           if (textMixNode.isEmpty ())
//           {
//             Log::logMsgWarn ("[random message] Fail to find <mix> in <message> template", true);
//             return;
//           }
//
//           Utils::xml_search_and_set_attribute_in_IXMLNode (mNode, mxconst::get_ATTRIB_NAME (), newTriggerName); // message has same name as its trigger
//           Utils::xml_add_cdata (textMixNode, message);
//           // add message to <messages>
//           Utils::xml_add_node_to_element_IXMLNode (this->xMessages, mNode);
//
//           // set trigger Location
//           IXMLNode pointNode = trigNode.addChild (mxconst::get_ELEMENT_POINT ().c_str ());
//           if (pointNode.isEmpty ())
//             continue;
//
//           Utils::xml_search_and_set_attribute_in_IXMLNode (pointNode, mxconst::get_ATTRIB_LAT (), Utils::formatNumber<double> (pCurr.getLat (), 8), mxconst::get_ELEMENT_POINT ());
//           Utils::xml_search_and_set_attribute_in_IXMLNode (pointNode, mxconst::get_ATTRIB_LONG (), Utils::formatNumber<double> (pCurr.getLon (), 8), mxconst::get_ELEMENT_POINT ());
//
//           if (!Utils::xml_add_node_to_element_IXMLNode (trigNode, pointNode, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ()))
//           {
//             Log::logMsgErr ("[random message] Fail to add <point> to trigger. skipping trigger. ", true);
//             continue;
//           }
//
//           // Add to Triggers element
//           xTriggers.addChild (trigNode);
//
//           // Link to current "flight leg"
//           IXMLNode linkNode = legNode.addChild (mxconst::get_ELEMENT_LINK_TO_TRIGGER ().c_str ());
//           if (!linkNode.isEmpty ())
//             Utils::xml_search_and_set_attribute_in_IXMLNode (linkNode, mxconst::get_ATTRIB_NAME (), newTriggerName, mxconst::get_ELEMENT_LINK_TO_TRIGGER ());
//
//
//         } // end loop over distance vector
//
//       } // end flag_found or node is not empty
//
//     } // end if to generate messages "if (!flag_disable_auto_messages)"
//
//   } // end loop over flight legs
// }
//
//
// // -----------------------------------

void
RandomEngine::gen_add_inventory_phase02_add_items (missionx::NavAidInfo &inOutNavAidInfo)
{
  const std::string inFlightLegName = (inOutNavAidInfo.flag_is_brieferOrStartLocation)? "Briefer" : inOutNavAidInfo.getName ();
  std::string    invName = inFlightLegName + " Inventory"; // v24.06.1 changed from: "inv_" + inFlightLegName
  IXMLNode       xPoint;
  const IXMLNode xItemFromMap    = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_ITEM (), true); // return copy of <item> node
  IXMLNode       xItemBlueprints = Utils::xml_get_node_from_node_tree_IXMLNode (missionx::RandomEngine::xRootTemplate, mxconst::get_ELEMENT_ITEM_BLUEPRINTS (), true); // return copy of <item_blueprints> node from <TEMPLATE> instead of <MAPPING> element.

  // get pointer to inventory location and elevation
  IXMLNode xLocAndElev_ptr = Utils::xml_get_node_from_node_tree_IXMLNode (inOutNavAidInfo.fpln_xml_inv_node, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA (), false); // return pointer of <loc_and_elev> node

  // v24.05.1 Read the blueprint items from the external file
  // Read from the external cargo_data.xml if we generate from "user creation screen" or if the blueprint is empty
  if (const std::string subCategory_type = missionx::data_manager::prop_userDefinedMission_ui.getStringAttributeValue (mxconst::get_PROP_MISSION_SUBCATEGORY_LBL (), "")
    ; !subCategory_type.empty ()
      && (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_user_generates_a_mission_layer || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::flight_leg_info || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_ils_layer || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_external_fpln_layer || xItemBlueprints.isEmpty ()))
  {
    auto xExternalNode = Utils::read_external_blueprint_items (mxconst::get_ELEMENT_CARGO (), mxconst::get_ELEMENT_ITEM_BLUEPRINTS (), subCategory_type, true, false);
    if (!xExternalNode.isEmpty ())
      xItemBlueprints = xExternalNode;
  }

  // validations
  if ( inOutNavAidInfo.fpln_xml_inv_node.isEmpty () || xItemBlueprints.isEmpty () || xLocAndElev_ptr.isEmpty ())
  {
    Log::logMsgErr (fmt::format("[{}] One of the key elements could not be found. Check if there is any mapping for: \"<point>, <item>, <item_blueprints>, <inventory> and <loc_and_elev_data>\" nodes. skipping inventory creation... ", __func__), true);
    return;
  }

  // validate items exists
  const int itemsInBlueprint_i = xItemBlueprints.nChildNode ();
  if (itemsInBlueprint_i == 0)
  {
    Log::logMsgWarn (fmt::format("[{}] No items in <item_blueptint> node mapping. Please add <item> nodes to it for random pick.", __func__), true);
    return;
  }
  //// End validation ///

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
      #ifndef RELEASE
      Log::logMsgThread (fmt::format ("\tMerge items: {} in {}.", sBarcode, invName)); // v24.12.2
      #endif
    }
    else
    {
      mapItemsInInv[sBarcode] = newItem;
      #ifndef RELEASE
      Log::logMsgThread (fmt::format ("Added item: {} to {}.", sBarcode, invName)); // v24.12.2
      #endif
    }
  } // end add items randomly

  // v24.12.2 add the items to the external inventory
  for (const auto &nodeItem : mapItemsInInv | std::views::values) // Only iterate over values and not keys
    inOutNavAidInfo.fpln_xml_inv_node.addChild (nodeItem);

  // end gen_add_inventory_phase02_add_items
}

// -----------------------------------

// void
// RandomEngine::addInventory (const std::string &inFlightLegName, const IXMLNode &inSourceNode, const mxInvSource inSourceType)
// {
//   #ifndef RELEASE
//   // Log::logMsg ("[DEBUG random airport] before <inventories> node.", true);
//   Log::logMsgThread ( fmt::format("[{}] before <inventories> node.", __func__));
//   #endif
//
//
//   std::string    invName = inFlightLegName + " Inventory"; // v24.06.1 changed from: "inv_" + inFlightLegName
//   IXMLNode       xPoint;
//   const IXMLNode xItemFromMap    = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_ITEM (), true); // return copy of <item> node
//   IXMLNode       xInv            = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_INVENTORY (), true); // return copy of <inventory> node
//   IXMLNode       xItemBlueprints = Utils::xml_get_node_from_node_tree_IXMLNode (missionx::RandomEngine::xRootTemplate, mxconst::get_ELEMENT_ITEM_BLUEPRINTS (), true); // return copy of <item_blueprints> node from <TEMPLATE> instead of <MAPPING> element.
//   // get pointer to inventory location and elevation
//   IXMLNode xLocAndElev_ptr = Utils::xml_get_node_from_node_tree_IXMLNode (xInv, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA (), false); // return pointer of <loc_and_elev> node
//
//
//   // v24.05.1 Read the blueprint items from the external file
//   // Read from the external cargo_data.xml if we generate from "user creation screen" or if the blueprint is empty
//   if (const std::string subCategory_type = missionx::data_manager::prop_userDefinedMission_ui.getStringAttributeValue (mxconst::get_PROP_MISSION_SUBCATEGORY_LBL (), "")
//     ; !subCategory_type.empty ()
//       && (data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_user_generates_a_mission_layer || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::flight_leg_info || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_ils_layer || data_manager::getGeneratedFromLayer () == missionx::uiLayer_enum::option_external_fpln_layer || xItemBlueprints.isEmpty ()))
//   {
//     auto xExternalNode = Utils::read_external_blueprint_items (mxconst::get_ELEMENT_CARGO (), mxconst::get_ELEMENT_ITEM_BLUEPRINTS (), subCategory_type, true, false);
//     if (!xExternalNode.isEmpty ())
//       xItemBlueprints = xExternalNode;
//   }
//
//
//   if (mxconst::get_FL_TEMPLATE_VAL_START () == inFlightLegName) // v3.0.219.7 added skip legs by the name start, since it represent the briefer starting location.
//     return;
//
//   if (inSourceType == mxInvSource::trigger)
//     xPoint = Utils::xml_get_node_from_node_tree_IXMLNode (inSourceNode, mxconst::get_ELEMENT_POINT (), true); // return copy of <point> node
//   else
//   {
//     xPoint = inSourceNode.deepCopy (); // should be point
//     xPoint.updateName (mxconst::get_ELEMENT_POINT ().c_str ()); // v25.04.1
//   }
//
//   //// validations ////
//   if (this->setInventories.contains (invName)) // If inventory exists, exit. This is not an error.
//   {
//     Log::logMsgWarn ("[random inv] Inventory by the name: " + invName + ", exists, skipping", true);
//     return;
//   }
//
//   if (xPoint.isEmpty () || xItemFromMap.isEmpty () || xInv.isEmpty () || xItemBlueprints.isEmpty () || xLocAndElev_ptr.isEmpty ()) // handle trigger source validation
//   {
//     Log::logMsgErr ("[random inv] One of the key elements could not be found. Check if there is any mapping for: \"<point>, <item>, <item_blueprints>, <inventory> and <loc_and_elev_data>\" nodes. skipping inventory creation... ", true);
//     return;
//   }
//
//   // get number of items in <mapping> element
//   const int itemsInBlueprint_i = xItemBlueprints.nChildNode ();
//   if (itemsInBlueprint_i == 0)
//   {
//     Log::logMsgWarn ("[random inv] No items in <item_blueptint> node mapping. Please add <item> nodes to it for random pick.", true);
//     return;
//   }
//   //// End validation ///
//
//
//   //// Set Inventory information ////
//   Utils::xml_search_and_set_attribute_in_IXMLNode (xInv, mxconst::get_ATTRIB_NAME (), invName, mxconst::get_ELEMENT_INVENTORY ());
//
//   // add inventory location
//   xLocAndElev_ptr.addChild (xPoint);
//
//   // add inventory radius
//   const std::string length_mt = Utils::xml_get_attribute_value_drill (inSourceNode, mxconst::get_ATTRIB_LENGTH_MT (), this->flag_found, mxconst::get_ELEMENT_RADIUS ()); // fetch radius if any. Trigger should have one
//   if (flag_found)
//     Utils::xml_search_and_set_attribute_in_IXMLNode (xInv, mxconst::get_ATTRIB_LENGTH_MT (), length_mt, mxconst::get_ELEMENT_RADIUS ());
//   else
//     Utils::xml_search_and_set_attribute_in_IXMLNode (xInv, mxconst::get_ATTRIB_LENGTH_MT (), mxconst::get_DEFAULT_INVENTORY_RADIUS_MT (), mxconst::get_ELEMENT_RADIUS ());
//
//   // add items to inventory randomly
//   int minNum = 0;
//   int maxNum = itemsInBlueprint_i;
//   if (mxconst::get_ELEMENT_BRIEFER () == inFlightLegName)
//   {
//     minNum = 4;
//     maxNum = 12;
//   }
//
//   const int numOfItemsToCreate_i = Utils::getRandomIntNumber (minNum, maxNum); // how many items should we create in inventory ?
//
//   // v24.12.2
//   std::unordered_map<std::string, IXMLNode> mapItemsInInv = {}; // [barcode, xml pointer]
//
//   for (int i1 = 0; i1 < numOfItemsToCreate_i; ++i1)
//   {
//     const int pick_i = Utils::getRandomIntNumber (0, itemsInBlueprint_i - 1); // pick random item node
//
//     IXMLNode newItem = xItemBlueprints.getChildNode (mxconst::get_ELEMENT_ITEM ().c_str (), pick_i).deepCopy (); // get a copy of the item node
//     if (newItem.isEmpty ())
//       continue;
//
//     // v24.05.1 Skip if item attribute "name" or "barcode" are empty
//     const std::string sBarcode = Utils::readAttrib (newItem, mxconst::get_ATTRIB_BARCODE (), ""); // v24.12.2
//     if (Utils::readAttrib (newItem, mxconst::get_ATTRIB_NAME (), "").empty () || sBarcode.empty ())
//       continue;
//
//     // v24.05.1 get original quantity
//     const int originalQuantity_i = Utils::readNodeNumericAttrib<int> (newItem, mxconst::get_ATTRIB_QUANTITY (), -1); // v24.05.1 read the quantity. "-1" means not found
//     const int rndQuantity        = Utils::getRandomIntNumber (1, ((originalQuantity_i > 0) ? originalQuantity_i : 10)); // pick random quantity
//
//     // v24.12.2 Check for duplicate items based on barcode and merge their quantity
//     if (mxUtils::isElementExists (mapItemsInInv, sBarcode) && !mapItemsInInv[sBarcode].isEmpty ())
//     {
//       const auto newQuantity = rndQuantity + Utils::readNodeNumericAttrib (mapItemsInInv[sBarcode], mxconst::get_ATTRIB_QUANTITY (), 0);
//       mapItemsInInv[sBarcode].updateAttribute (mxUtils::formatNumber<int> (newQuantity).c_str (), mxconst::get_ATTRIB_QUANTITY ().c_str (), mxconst::get_ATTRIB_QUANTITY ().c_str ());
//       #ifndef DEBUG
//       Log::logMsgThread (fmt::format ("\tMerge items: {} in {}.", sBarcode, invName)); // v24.12.2
//       #endif // !DEBUG
//     }
//     else
//     {
//       mapItemsInInv[sBarcode] = newItem;
//       #ifndef DEBUG
//       Log::logMsgThread (fmt::format ("Added item: {} to {}.", sBarcode, invName)); // v24.12.2
//       #endif // !DEBUG
//     }
//   }
//
//   // v24.12.2
//   for (const auto &nodeItem : mapItemsInInv | std::views::values) // Only iterate over values and not keys
//   {
//     xInv.addChild (nodeItem);
//   }
//
//   #ifndef RELEASE
//   Log::logMsgThread ("Added Inventory Content: \n");
//   Utils::xml_print_node (xInv, true);
//   #endif
//
//   xInventoris.addChild (xInv);
//
//   #ifndef RELEASE
//   Log::logMsg ("[DEBUG random airport] after <inventories> node.", true);
//   #endif
// }
//
// // -----------------------------------

// -----------------------------------
// -----------------------------------

bool
RandomEngine::writeTargetFile ()
{
  bool result = true;
  // Prepare path and file names // v3.0.241.10 b2 extended cases where template was picked from custom mission folder, therefore the output should be {mission folder name}.xml. That way we create uniquness

  const std::string savePathAndFile = (RandomEngine::working_tempFile_ptr->missionFolderName.empty ()) ? this->pathToRandomBrieferFolder + mxconst::get_FOLDER_SEPARATOR () + mxconst::get_RANDOM_MISSION_DATA_FILE_NAME () : this->pathToRandomBrieferFolder + mxconst::get_FOLDER_SEPARATOR () + RandomEngine::working_tempFile_ptr->missionFolderName + ".xml";


  const std::string_view mission_name_con = (!RandomEngine::working_tempFile_ptr->missionFolderName.empty ()) ? RandomEngine::working_tempFile_ptr->missionFolderName : RandomEngine::random_thread_state.dataString;
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
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_current_weather_state_and_store_in_RandomEngine))
  {
    missionx::RandomEngine::current_weather_datarefs_s.clear ();
    RandomEngine::setError ("[random write weather] Failed to read current X-Plane weather information.");
  }
  missionx::data_manager::add_advanceSettingsDateTime_and_Weather_to_node (this->xGlobalSettings, missionx::data_manager::prop_userDefinedMission_ui.node, missionx::RandomEngine::current_weather_datarefs_s);

  // Deprecate, use new static: RandomEngine::xDrefStartColdAndDark.
  // IXMLNode xDrefStartColdAndDark = missionx::RandomEngine::xRootTemplate.getChildNode (mxconst::get_ELEMENT_DATAREFS_START_COLD_AND_DARK ().c_str ()).deepCopy ();
  // if (!xDrefStartColdAndDark.isEmpty () && (Utils::readBoolAttrib (xRootTemplate, mxconst::get_ATTRIB_COPY_LEG_AS_IS_B (), false) == false)) // v3.0.303 add support for special words "{navaid_lat}" and "{navaid_lon}"
  // {
  //
  //   // find the first NavAid that briefer is using.
  //   const std::string firstLeg_s = Utils::readAttrib (this->xBriefer, mxconst::get_ATTRIB_STARTING_LEG (), "");
  //   bool bFoundFirstLeg = false;
  //   int leg_counter            = this->xFlightLegs.nChildNode ();
  //   for (int i1 = 0; i1 < leg_counter; ++i1)
  //   {
  //     auto leg_node = this->xFlightLegs.getChildNode (mxconst::get_ELEMENT_LEG ().c_str (), i1);
  //     const auto flightLegName = Utils::readAttrib (leg_node, mxconst::get_ATTRIB_NAME (), "");
  //     bFoundFirstLeg = (flightLegName == firstLeg_s);
  //     if (bFoundFirstLeg)
  //     {
  //       if (!leg_node.isEmpty ())
  //       {
  //         std::string text = Utils::xml_get_text (xDrefStartColdAndDark);
  //         text             = Utils::replaceString (text, "{navaid_lat}", leg->getLat (), true);
  //         text             = Utils::replaceString (text, "{navaid_lon}", leg->getLon (), true);
  //
  //         Utils::xml_set_text (xDrefStartColdAndDark, text);
  //       }
  //     }
  //   } // end loop
  //
  //
  //
  // } // xDrefStartColdAndDark


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

  xTargetTopNode.addChild (RandomEngine::xDrefStartColdAndDark); // v3.0.221.15 rc3.5 add start cold and dark
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
  const auto lmbda_get_plane_filter_string = [] (const missionx::mx_plane_types_enum inPlaneType)
  {
    std::string stmt;
    switch (inPlaneType)
    {
      case missionx::mx_plane_types_enum::plane_type_any:
        return missionx::EMPTY_STRING;
        break;
      case missionx::mx_plane_types_enum::plane_type_helos:
        stmt = " and (helipads + ramp_helos) > 0 "; //
        break;
      case missionx::mx_plane_types_enum::plane_type_ga_floats:
        stmt = " and ap_type in ( 1, 16 ) and ramp_planes > 0 ";
        break;
      case missionx::mx_plane_types_enum::plane_type_ga:
      case missionx::mx_plane_types_enum::plane_type_props:
        stmt = " and ap_type = 1 and ramp_props > 0 "; //
        break;
      case missionx::mx_plane_types_enum::plane_type_turboprops:
        stmt = " and ap_type = 1 and ramp_turboprops > 0 "; //
        break;
      case missionx::mx_plane_types_enum::plane_type_jets:
      case missionx::mx_plane_types_enum::plane_type_heavy:
        stmt = " and ap_type = 1 and ramp_jet_heavy > 0 "; //
        break;
      default:
        break;
    }

    return stmt;
  };

  // auto ramp_type_filter_s = lmbda_get_plane_filter_string (static_cast<missionx::mx_plane_types> (this->getPlaneType ()));
  auto ramp_type_filter_s = lmbda_get_plane_filter_string (static_cast<missionx::mx_plane_types_enum> (in_plane_type));


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
  Log::logMsgThread (fmt::format ("[{}] Query: {}", __func__, sql) );
  #endif // !RELEASE



  if (data_manager::db_xp_airports.db_is_open_and_ready)
  {
    char *zErrMsg = nullptr;

    // clear local cache
    RandomEngine::resultTable_gather_random_airports.clear ();
    if (int rc = sqlite3_exec (data_manager::db_xp_airports.db, sql.c_str (), RandomEngine::callback_gather_random_airports_db, nullptr, &zErrMsg); rc != SQLITE_OK)
    {
      Log::logMsgThread (fmt::format ("[{}] SQL error: {}", __func__, std::string (zErrMsg)) );
      sqlite3_free (zErrMsg);
    }
    else
    {
      Log::logMsgThread (fmt::format ("[{}] Information was gathered.", __func__) );
      #ifndef RELEASE
      for (auto &[row_num, row_data] : RandomEngine::resultTable_gather_random_airports)
        Log::logMsgThread ( fmt::format ("\tSeq: {}, icao_id: {}, icao: {}", row_num, row_data["icao_id"], row_data["icao"]) );
      #endif // !RELEASE


      // If there is data then pick a ramp
      if (!RandomEngine::resultTable_gather_random_airports.empty ())
      {

        const auto lmbda_get_ramp_filter_based_on_plane_type = [] (missionx::mx_plane_types_enum inPlaneType)
        {
          std::string stmt;
          switch (inPlaneType)
          {
            case missionx::mx_plane_types_enum::plane_type_any:
              stmt = "";
              break;
            case missionx::mx_plane_types_enum::plane_type_helos:
              stmt = " and helos > 0 "; // pick all airports that have helos ramps (heliports or any airport with helos in it). The view we use calculated the number of helos ramps so it is easy to distinguish between them.
              break;
            case missionx::mx_plane_types_enum::plane_type_ga_floats:
            case missionx::mx_plane_types_enum::plane_type_ga:
            case missionx::mx_plane_types_enum::plane_type_props:
            case missionx::mx_plane_types_enum::plane_type_turboprops:
              stmt = " and props + turboprops > 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
              break;
            case missionx::mx_plane_types_enum::plane_type_jets:
            case missionx::mx_plane_types_enum::plane_type_heavy:
              stmt = " and jet_n_heavy > 0 "; // make sure only airports are being picked with at list 1 ramp for planes (not heliport or sea airports)
              break;
            default:
              break;
          }

          return stmt;
        };

        auto row = RandomEngine::resultTable_gather_random_airports.cbegin ()->second; // fetch the first result
        nav.setID (row["icao"]);
        nav.setName (row["ap_name"]);
        // v25.10.1 store airport lat/lon even if there is no ramp. In most cases, the middle of the runway.
        nav.lat = mxUtils::stringToNumber<float> (row["ap_lat"], 8);
        nav.lon = mxUtils::stringToNumber<float> (row["ap_lon"], 8);

        nav.flag_is_custom_scenery = (!(row["is_custom"].empty ())); // v3.303.12 changed field name to is_custom

        const std::string elev_ft = row["ap_elev_ft"];
        nav.height_mt             = (elev_ft.empty ()) ? 0.0f : mxUtils::stringToNumber<float> (elev_ft) * missionx::feet2meter;

        const std::string select_s     = "select * from ramps_vu where 1 = 1 and for_planes is not null and icao_id = " + row["icao_id"]; // v3.303.14 added "for_planes is not null" to narrow the airports to the ones that there are real ramps
        const std::string filter_ramps = lmbda_get_ramp_filter_based_on_plane_type (static_cast<missionx::mx_plane_types_enum> (in_plane_type));
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
            auto ramp             = resultTable_gather_ramp_data.cbegin ()->second;
            nav.lat               = mxUtils::stringToNumber<float> (ramp["lat"], 12);
            nav.lon               = mxUtils::stringToNumber<float> (ramp["lon"], 12);
            nav.ramp_info.uq_name = ramp["name"];
            nav.ramp_info.ramp_for_planes    = ramp["for_planes"];

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
  Log::logAttention (fmt::format ("*** Finished {}. Duration: {:.3f}ms ({:.3f}sec  ****)", __func__, duration, (duration / 1000)), true);
#endif // !RELEASE

  return nav;
}


// -----------------------------------

float
RandomEngine::calc_slope_at_point_mainThread (NavAidInfo &inNavAid)
{
  missionx::NavAidInfo north, south, east, west, ne, nw, se, sw;
  constexpr float      radius_in_nm = 20 * missionx::meter2nm; // 20-meter radius minimal radius. which means ~80 meter of land to test

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
RandomEngine::translatePlaneTypeToString (const mx_plane_types_enum in_plane_type)
{

  if (Utils::isElementExists (RandomEngine::mapPlaneEnumToStringTypes, in_plane_type))
    return RandomEngine::mapPlaneEnumToStringTypes[in_plane_type];

  return ""; // v3.0.253.1 this->mapPlaneEnumToStringTypes[in_plane_type]; // should return empty string
}

// -----------------------------------

missionx::mx_plane_types_enum
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

mx_plane_types_enum
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
    missionx::RandomEngine::template_plane_type_enum = missionx::mx_plane_types_enum::plane_type_any;
    this->randomPlaneType.clear ();
  }

  return missionx::RandomEngine::template_plane_type_enum;
}

// -----------------------------------

void
RandomEngine::setPlaneType (const mx_plane_types_enum inPlaneType)
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
    RandomEngine::template_plane_type_enum = missionx::mx_plane_types_enum::plane_type_any;
    this->randomPlaneType.clear ();
  }
}

// -----------------------------------

uint8_t
RandomEngine::getPlaneType ()
{
  return static_cast<uint8_t> (RandomEngine::template_plane_type_enum);
}

mx_plane_types_enum
RandomEngine::getPlaneType_enum ()
{
  return RandomEngine::template_plane_type_enum;
}


// -----------------------------------
void
RandomEngine::abortThread ()
{

  if (missionx::RandomEngine::random_thread_state.flagIsActive)
    missionx::RandomEngine::random_thread_state.flagAbortThread = true;
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


double
RandomEngine::get_slope_at_point (const missionx::NavAidInfo &outNavAid)
{
  missionx::RandomEngine::random_thread_state.pipeProperties.setNodeProperty<float> (mxconst::get_ATTRIB_LAT (), outNavAid.lat);
  missionx::RandomEngine::random_thread_state.pipeProperties.setNodeProperty<float> (mxconst::get_ATTRIB_LONG (), outNavAid.lon);
  RandomEngine::shared_navaid_info.p = outNavAid.p;

  double found_slope_d = 0.0;
  if (missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::calculate_slope_for_build_flight_leg_thread))
    found_slope_d = missionx::RandomEngine::random_thread_state.pipeProperties.getAttribNumericValue<double> (mxconst::get_ATTRIB_TERRAIN_SLOPE (), 0.0); // v3.305.1 updated

  RandomEngine::errMsg.clear ();
  return found_slope_d;
}

// -----------------------------------

bool
RandomEngine::get_is_wet_at_point (const missionx::NavAidInfo &inNavAid)
{
  RandomEngine::shared_navaid_info.p = inNavAid.p;
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_is_point_wet))
  {
    RandomEngine::setError ("[random isWet] Failed to probe for wet. Will treat target coordinates as \"land\". ");
  }

  return RandomEngine::shared_navaid_info.isWet;
}

// -----------------------------------


float
RandomEngine::get_terrain_elevation_at_point_in_mt (const missionx::NavAidInfo &inNavAid)
{
  RandomEngine::shared_navaid_info.p = inNavAid.p;
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_terrain_elev_in_point))
  {
    RandomEngine::setError (fmt::format("[{}] Failed to probe for terrain elevation. Will treat target terrain elevation as \"Zero\". ", __func__) );
  }

  return static_cast<float>( RandomEngine::shared_navaid_info.p.getElevationInMeters () );
}

// -----------------------------------



bool
RandomEngine::prepare_blank_template_with_flight_legs_based_on_ui (IXMLNode &pNode, IXMLNode &outMetaNode, std::string &outErr)
{
  std::string location_value_s;
  outErr.clear ();

  // Gather information from UI layer
  const auto med_cargo_or_oilrig_i             = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::undefined)); // 0 = med, 1 = cargo
  const auto mission_subcategory_indx_picked_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MISSION_SUBCATEGORY (), static_cast<int> (missionx::mx_mission_subcategory_type::not_defined)); //
  const auto uiLayer_debug                     = data_manager::getGeneratedFromLayer (); // v25.02.1

  const std::string CATEGORY_TRANSLATION = missionx::data_manager::get_translate_of_mission_subcategory_code (med_cargo_or_oilrig_i, mission_subcategory_indx_picked_i, outMetaNode); // v3.303.14

  outMetaNode.updateAttribute (CATEGORY_TRANSLATION.c_str (), mxconst::get_ATTRIB_CATEGORY ().c_str (), mxconst::get_ATTRIB_CATEGORY ().c_str ());
  outMetaNode.updateAttribute (mxUtils::formatNumber<int> (med_cargo_or_oilrig_i).c_str (), mxconst::get_PROP_MED_CARGO_OR_OILRIG ().c_str (), mxconst::get_PROP_MED_CARGO_OR_OILRIG ().c_str ());
  outMetaNode.updateAttribute (mxUtils::formatNumber<int> (mission_subcategory_indx_picked_i).c_str (), mxconst::get_PROP_MISSION_SUBCATEGORY ().c_str (), mxconst::get_PROP_MISSION_SUBCATEGORY ().c_str ());


  // auto       plane_type_i        = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types_enum::plane_type_props)); // plane type
  const auto no_of_legs_i        = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_NO_OF_LEGS (), 2); // no of legs
  auto       min_distance_slider = Utils::readNodeNumericAttrib<double> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MIN_DISTANCE_SLIDER (), 5.0); // min slider
  auto       max_distance_slider = Utils::readNodeNumericAttrib<double> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MAX_DISTANCE_SLIDER (), 45.0); // max slider

  // // Validations
  // assert ((!pNode.isEmpty () && !data_manager::prop_userDefinedMission_ui.node.isEmpty ()) && "Empty template or prop_userDefinedMission_ui are empty!"); // debug
  // assert (med_cargo_or_oilrig_i > static_cast<int> (missionx::mx_ui_mission_type::undefined) && ": Main Mission Type can't be undefined. Aborting!!!"); // debug
  // assert (CATEGORY_TRANSLATION.empty () == false && ": Sub Category was not found. Aborting!!!"); // debug
  //
  // // Force helos for oilrig missions
  // if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::oil_rig))
  // {
  //   plane_type_i = static_cast<int> (missionx::mx_plane_types_enum::plane_type_helos);
  // }
  // auto        conv_plane_type_i = static_cast<missionx::def_mx_plane_type_enum> (plane_type_i);


  auto        plane_type_enum_i = RandomEngine::gen_parse_plane_type (data_manager::prop_userDefinedMission_ui, pNode, outMetaNode);
  std::string plane_type_s      = missionx::RandomEngine::translatePlaneTypeToString (plane_type_enum_i);

  // Store plane type in the XML node
  missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_PLANE_TYPE_S (), plane_type_s);
  pNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ());
  outMetaNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ()); // v25.05.1

  const auto lmbda_get_ramp_type_H_or_S_or_any = [] (auto in_plane_type_i)
  {
    if ((in_plane_type_i == missionx::mx_plane_types_enum::plane_type_prop_floats) || (in_plane_type_i == missionx::mx_plane_types_enum::plane_type_ga_floats))
      return "|ramp=S"; // S = Seaports

    if (in_plane_type_i == missionx::mx_plane_types_enum::plane_type_helos)
      return "|ramp=H"; // H = Helos

    return "";
  };

  const std::string ramp_type_s = lmbda_get_ramp_type_H_or_S_or_any (plane_type_enum_i);

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

    tag_name += (plane_type_enum_i == missionx::mx_plane_types_enum::plane_type_helos)? "_helos" : "_plane";
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


  // TODO: v25.05.1 the briefer skeleton message needs to be override with the "surprise me" option.
  briefer_skeleton_message_to_use_in_injectTypeMissionFeature = "Hello Pilot\n";

  briefer_skeleton_message_to_use_in_injectTypeMissionFeature += (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::medevac)) ? "You have been assigned to a medevac mission. " : (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::cargo)) ? "You have been assigned to a cargo flight. " : (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::oil_rig)) ? "You have been assigned to an oilrig flight. " : "You have been assigned to a flight. ";

  briefer_skeleton_message_to_use_in_injectTypeMissionFeature += fmt::format ("Your expected transportation is a {}.\n", (plane_type_enum_i == missionx::def_mx_plane_type_enum::plane_type_helos) ? "helo" : plane_type_s);

  return true;
}

// -----------------------------------

std::map<int, NavAidInfo>
RandomEngine::gen_get_databaseflightplan_site_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr)
{
  std::map<int, NavAidInfo> navaid_targets;
  const auto fpln_id_picked_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FPLN_ID_PICKED (), -1); // max slider

  if (fpln_id_picked_i < 0 || Utils::isElementExists (missionx::data_manager::indexPointer_forExternalFPLN_tableVector, fpln_id_picked_i) == false)
  {
    outErr = fmt::format("Could not find the flight plan with index id: {}, aborting mission template generating.", fpln_id_picked_i);
    return navaid_targets;
  }

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
        if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
        {
          navaid_targets.clear ();
          outErr = fmt::format ("[{}] Navaid: {}, Failed to find Airport NEAR given location. Still using original Navaid.", __func__, counter);
          return navaid_targets;
        }
        na.synchToPoint ();
        RandomEngine::shared_navaid_info.navAid.synchToPoint ();
        if (const auto distance = na.p.calcDistanceBetween2Points (RandomEngine::shared_navaid_info.navAid.p); distance <= 2.0) // if navaid within 2 nm
        {
          na.clone (RandomEngine::shared_navaid_info.navAid); // v25.04.2 changed code to clone
          na.synchToPoint ();
        }

        missionx::mx_return search_ramp_result = true;
        if (counter == 0) // briefer
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
          else
          {
            search_ramp_result = gen_get_ramp_based_on_plane_type (na, getPlaneType_enum (), (counter == 0) ? missionx::mxFilterRampType::start_ramp : missionx::mxFilterRampType::end_ramp);
          }
        }
        else 
        {
          search_ramp_result = gen_get_ramp_based_on_plane_type (na, getPlaneType_enum (), missionx::mxFilterRampType::end_ramp);
        }

        // check for errors or information
        if (!search_ramp_result.result || search_ramp_result.getInfoIndex())
        {
          Log::logMsgThread (fmt::format ("[{}] {}\n", __func__, search_ramp_result.getErrorsAndInfoAsText ()));
        }

      } // finish gathering info for first and last navaids
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

            if (na.getID ().empty ())
              na.setID (np_guess.name);
            na.navType = (np_guess.nav_type >= 0) ? np_guess.nav_type : na.navType;
            na.navRef  = np_guess.nav_ref;
            na.synchToPoint ();
            break; // exit loop, found item
          }

          ++internalCounter;
        } // end loop over all listNavPointsGuessedName
      } // end inject guessed NavAid names

      navaid_targets[counter] = na;
      counter++;

    } // end loop over waypoints and gathering NavAid info

    // add briefer description
    const std::string from_to_s   = get_short_flight_description_from_to (fpln.fromName_s, fpln.fromICAO_s, fpln.toName_s, fpln.toICAO_s); //"fpln.fromName_s + "(" + fpln.fromICAO_s + ") to " + fpln.toName_s + "(" + fpln.toICAO_s + ")";
    const std::string brieferDesc = "Hello pilot.\nYou have been assigned a flight generated from \"flightplandatabase.com\". Fly: " + from_to_s + ". Learn the route and fly it according to the flight plan or modify it if you so wish.\n\nBlue skys.";
    const std::string notes       = (fpln.notes_s.empty ()) ? "" : "\n\nnotes:\n" + fpln.notes_s; // add notes if any from flight plan

    navaid_targets[0].fpln_expected_location_data.desc = brieferDesc + notes + "\n\n==== suggested waypoints ====\n" + ((fpln.formated_nav_points_with_guessed_names_s != "false")? fpln.formated_nav_points_with_guessed_names_s: "");


    // do a basic validation
    for (auto &[indx, na] : navaid_targets)
    {
      if (!na.is_lat_lon_valid ())
        outErr += fmt::format ("[()]] Flight leg: {} is invalid. Aborting.\n", indx);
    }
  }

  if (!outErr.empty())
    navaid_targets.clear();

  return navaid_targets;
}

// -----------------------------------

missionx::mx_return
RandomEngine::gen_prepare_mission_based_on_databaseflightplan_site (IXMLNode &in_xTemplateNode, IXMLNode & inout_meta_node)
{
  assert (!in_xTemplateNode.isEmpty () && !data_manager::prop_userDefinedMission_ui.node.isEmpty () && fmt::format("[{}] Empty template or prop_userDefinedMission_ui are empty!", __func__).c_str () );

  missionx::mx_return out_func_result = true;

  // const auto plane_type_i     = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types_enum::plane_type_props)); // plane type
  // v25.10.2
  const auto plane_type_enum_i = RandomEngine::gen_parse_plane_type (data_manager::prop_userDefinedMission_ui, in_xTemplateNode, inout_meta_node);
  this->setPlaneType (plane_type_enum_i); // set plane type in class level for other function usage too

  std::string outErr;
  auto navaid_targets = gen_get_databaseflightplan_site_targets (&RandomEngine::random_thread_state, in_xTemplateNode, RandomEngine::shared_navaid_info, outErr);

    // validations
  if (!outErr.empty () || navaid_targets.empty ())
  {
    // missionx::RandomEngine::setError (outErr);
    if (outErr.empty ())
      outErr = "No valid targets were generated.";

    out_func_result.addErrMsg (outErr, true);
    return out_func_result;
  }

  //////////////////////
  // Prepare Main Nodes
  /////////////////////
  for (auto &target_navaid : navaid_targets | std::views::values)
  {
    target_navaid.fpln_leg_name  = gen_leg_name (&this->seq_waypoints, mxconst::get_GPS_WP (), "leg", target_navaid);
  }

  // force cargo type mission. Used when calling "gen_gather_navaid_metadata_relative_to_target()" function.
  Utils::xml_set_attribute_in_node <int>(this->xMetadata, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::cargo));

  // validate navaid targets
  int valid_navaids_i = 0;
  auto navaids_validation = gen_validate_navaids (navaid_targets, valid_navaids_i);
  if (!navaids_validation.result) // if there is a failure
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg (navaids_validation.getErrorsAsText (), true);
    return out_func_result;
  }

  // we must have 2 navaids or more
  if ( valid_navaids_i != static_cast<int>(navaid_targets.size ()) && valid_navaids_i < 2)
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg ( fmt::format("Valid targets found: {}, is not the same as overall generated targets: {}", valid_navaids_i, navaid_targets.size ()), true);
    return out_func_result;
  }

  std::string plane_type_s = missionx::RandomEngine::translatePlaneTypeToString (getPlaneType_enum ()); // convert type to string and store it in mission node
  missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_PLANE_TYPE_S (), plane_type_s); //, data_manager::prop_userDefinedMission_ui.node, data_manager::prop_userDefinedMission_ui.node.getName());
  in_xTemplateNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ());


  // ----------------------
  // -- Add <briefer> node - Start Location BUT NOT the description.
  // ----------------------
  navaid_targets[0].fpln_navaid_was_already_prepared = true; // force flag
  gen_briefer_phase_02_base_node_from_navaid (navaid_targets[0], RandomEngine::shared_navaid_info, false);

  // ------------------------------------------------------------------
  // Construct all mission <leg> nodes
  // navaid_targets: [0] = start/briefer, [1]..[N-1] legs.
  // ------------------------------------------------------------------
  RandomEngine::gen_create_all_leg_nodes_based_on_navaid_targets (navaid_targets, true);

  for (auto &[indx, target_navaid] : navaid_targets)
  {
    // Add external inventory
    // will skip any navaid that is the same as the Start location.
    if (!target_navaid.flag_is_same_as_start_location ) //
    {
      target_navaid.fpln_xml_inv_node = gen_add_inventory_phase01_node (indx, target_navaid, map_osm_inventory_track);
      //  skip the items phase if it is the last location or the inventory node is empty.
      if ( !target_navaid.fpln_xml_inv_node.isEmpty () && navaid_targets.contains (indx+1))
        gen_add_inventory_phase02_add_items (target_navaid);
    }

    if (indx == 0) // skip briefer
    {
      target_navaid.fpln_mission_phase = missionx::enums::mx_rnd_mission_phase::start;
      continue;
    }

    // we only want the first and last navaids
    if ( indx != static_cast<int>( navaid_targets.size () - 1 ) )
      continue;

    // Fix briefer "starting_leg", since we ignored all the navaids in between.
    // We override the code in "gen_create_all_leg_nodes_based_on_navaid_targets()" function specifically for the briefer.
    if (navaid_targets.contains (0) && navaid_targets[0].flag_is_brieferOrStartLocation)
    {
      const auto current_leg_name = Utils::readAttrib ( target_navaid.fpln_xml_target_leg_node, mxconst::get_ATTRIB_NAME (), "" );
      navaid_targets[0].fpln_xml_target_leg_node.updateAttribute (current_leg_name.c_str (), mxconst::get_ATTRIB_STARTING_LEG ().c_str (), mxconst::get_ATTRIB_STARTING_LEG ().c_str () );
    }


    // add start messages
    gen_leg_start_messages (this->seq_messages, target_navaid, navaid_targets, this->xMessages, false);

    // add 3d marker
    gen_add_3d_marker_to_current_target (target_navaid.fpln_xml_target_leg_node, target_navaid);

    // add 3D display objects around the landing
    // add support for <display_object_set>
    gen_3d_add_display_object_sets_instances_to_leg (target_navaid, target_navaid.fpln_xml_target_leg_node, in_xTemplateNode, this->x3DObjTemplate, this->expected_slope_at_target_location_d);

    gen_3d_parse_instances_in_leg (target_navaid.fpln_xml_target_leg_node, target_navaid);

    target_navaid.synchToPoint ();
    target_navaid.fpln_xml_target_leg_node = this->xFlightLegs.addChild (target_navaid.fpln_xml_target_leg_node);

    // add task trigger nodes to main trigger node.
    for (auto &node : target_navaid.fpln_leg_vec_trigger_nodes)
      this->xTriggers.addChild (node.deepCopy ());

    // add target navaid objective node to the main objectives node
    this->xObjectives.addChild (target_navaid.fpln_leg_objective_node.deepCopy ());

  } // end loop over all targets and adding final touches to each flight leg and adding them to the main mission nodes.

  // prepare a flight plan to show the end user
  this->cumulative_location_desc_s = gen_get_cumulative_fpln_desc (navaid_targets);


  // check [abort]
  if (RandomEngine::random_thread_state.flagAbortThread)
  {
    out_func_result.addErrMsg ("User asked to abort.", true);
    return out_func_result;
  }

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


    this->xGPS.addChild (p_gps_node); // no skewed navaids
  }

  // ----------------------------
  // add Briefer description
  // ----------------------------
  gen_briefer_phase_03_add_desc (navaid_targets, false);
  this->xBriefer = navaid_targets[0].fpln_xml_target_leg_node.deepCopy ();

  // v25.10.1 Add Cold and dark
  RandomEngine::xDrefStartColdAndDark = gen_set_and_get_start_cold_and_dark (in_xTemplateNode, navaid_targets[1]);


  // add <mission_info>
  if (!gen_read_mission_info_element ()) // <mission_info>
  {
    missionx::RandomEngine::random_thread_state.flagAbortThread = true;
    out_func_result.addErrMsg ("No <mission_info> node was found in template.", true);
  }


  // Add all inventories to the global xInventories node
  for (auto &[key, nav] : navaid_targets )
  {
    // add to inventories
    if (!nav.fpln_xml_inv_node.isEmpty ())
      nav.fpln_xml_inv_node = this->xInventoris.addChild (nav.fpln_xml_inv_node);
  }

  #ifndef RELEASE
  Log::logMsgThread (fmt::format ("-------------- <CONTENT_MISSION> RESULTS - Post {} --------------", __func__));
  Log::logMsgThread (fmt::format ("BRIEFER_INFO:\n{}\n", Utils::xml_get_node_content_as_text (this->xBriefer)));
  Log::logMsgThread (fmt::format ("BRIEFER:\n{}\n", Utils::xml_get_node_content_as_text (navaid_targets[0].fpln_xml_target_leg_node))); // we store the briefer in [0]
  Log::logMsgThread (fmt::format ("TRIGGERS:\n{}\n", Utils::xml_get_node_content_as_text (this->xTriggers)));
  Log::logMsgThread (fmt::format ("OBJECTIVES:\n{}\n", Utils::xml_get_node_content_as_text (this->xObjectives)));
  Log::logMsgThread (fmt::format ("FLIGHT LEGS:\n{}\n", Utils::xml_get_node_content_as_text (this->xFlightLegs)));
  Log::logMsgThread (fmt::format ("Inventories:\n{}\n", Utils::xml_get_node_content_as_text (this->xInventoris)));
  Log::logMsgThread (fmt::format ("GPS:\n{}\n", Utils::xml_get_node_content_as_text (this->xGPS)));
  Log::logMsgThread (fmt::format ("-------------- END <CONTENT_MISSION> RESULTS - {} --------------", __func__));
  #endif // !RELEASE

  return out_func_result;
}


// -----------------------------------

std::map<int, NavAidInfo>
RandomEngine::gen_get_ils_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_template_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr)
{
  std::map<int, NavAidInfo> navaid_targets;

  const auto fpln_id_picked_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FPLN_ID_PICKED (), -1); // max slider
  auto       fromICAO         = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FROM_ICAO (), "");
  auto       toICAO           = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_TO_ICAO (), "");

  if (fpln_id_picked_i < 0 || Utils::isElementExists (missionx::data_manager::indexPointer_for_ILS_rows_tableVector, fpln_id_picked_i) == false)
  {
    outErr = fmt::format("Could not find the ILS flight plan with index id: {}, aborting mission template generating.", fpln_id_picked_i);
    return navaid_targets;
  }
  if (fromICAO.empty ())
  {
    outErr = "No source ICAO was found, aborting mission template generating.";
    return navaid_targets;
  }

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
    outErr = fmt::format ("ILS Flight plan is invalid. Index id: {}, aborting mission template generating.", fpln_id_picked_i);
    return navaid_targets;
  }

  RandomEngine::shared_navaid_info.navAid.init ();
  RandomEngine::shared_navaid_info.navAid.setID (fromICAO);
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
  {
    outErr = fmt::format( "[{}] Start Navaid: {}. Failed to find Airport using original Navaid. Notify developer.", __func__, fromICAO);
    return navaid_targets;
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
  else 
  {
    auto search_start_ramp_result = RandomEngine::gen_get_ramp_based_on_plane_type (start_na, RandomEngine::getPlaneType_enum (), mxFilterRampType::start_ramp);
    if (!search_start_ramp_result.result)
    {
      Log::logMsgThread (fmt::format ("[{}] {}", __func__, search_start_ramp_result.getErrorsAndInfoAsText ()));
    }
  }

  start_na.synchToPoint ();
  if (start_na.getName ().empty ())
    start_na.setName (mxconst::get_ELEMENT_BRIEFER ());

  // try to locate a ramp
  // outErr.clear ();
  // if (!missionx::RandomEngine::get_user_wants_to_start_from_plane_position () && !filterAndPickRampBasedOnPlaneType (start_na, outErr, missionx::mxFilterRampType::start_ramp))  
  // outErr.clear ();


  // handle target location
  RandomEngine::shared_navaid_info.navAid.init ();
  RandomEngine::shared_navaid_info.navAid.setID (toICAO);
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
  {
    outErr = fmt::format ("[{}] Target Navaid: {}, Failed to find Airport using original Navaid. Notify developer.", __func__, toICAO);
    return navaid_targets;
  }
  RandomEngine::shared_navaid_info.navAid.synchToPoint ();
  NavAidInfo target_na = RandomEngine::shared_navaid_info.navAid;
  target_na.synchToPoint ();

  // Locate target ramp
  // outErr.clear ();
  // if (!filterAndPickRampBasedOnPlaneType (target_na, outErr, missionx::mxFilterRampType::end_ramp))
  // v25.10.2
  auto search_end_ramp_result = RandomEngine::gen_get_ramp_based_on_plane_type (target_na, RandomEngine::getPlaneType_enum (), mxFilterRampType::end_ramp);
  if (!search_end_ramp_result.result)
  {
    Log::logMsgThread (fmt::format ("[{}, Target ILS] {}", __func__, search_end_ramp_result.getErrorsAndInfoAsText ()));
  }
  // outErr.clear ();

  if ( !start_na.is_lat_lon_valid () || !target_na.is_lat_lon_valid ())
  {
    outErr = fmt::format("[{}] One of the navaids is invalid. Aborting. Consider notifying the developer.", __func__);
    return navaid_targets;
  }

  // Fill the briefer description so we won't use the "generic" briefer text.
  const std::string from_to_s   = get_short_flight_description_from_to (start_na.getName (), start_na.getID (), target_na.getName (), target_na.getID ()); //"From: " + start_na.getName() + "(" + start_na.getID() + ") to " + target_na.getName() + "(" + target_na.getID() + ")";
  std::string       brieferDesc = "Fly " + from_to_s + "\n\n" + "Hello pilot.\nYou have been assigned an ILS flight to " + target_na.getID () + " and runway: " + to_icao.loc_rw_s + ". Learn the route and fly it according to plan or modify it if you so wish.\n\nBlue skys.";
  std::string       notes       = "\n\nDestination Notes:\n==============\nAirport: " + to_icao.toName_s + "(" + to_icao.toICAO_s + ")\tAirport Elev.: " + mxUtils::formatNumber<int> (to_icao.ap_elev_ft_i) + " ft." + "\nEstimate distance: " + mxUtils::formatNumber<double> (to_icao.distnace_d, 0) + "nm. \tRunway to Land: " + to_icao.loc_rw_s + ".\nLocalizer Type: " + to_icao.locType_s + ". \tLocalizer bearing: " + mxUtils::formatNumber<int> (to_icao.loc_bearing_i) + " \tlocalizer frq.: " + mxUtils::getFreqFormated (to_icao.loc_frq_mhz);
  start_na.fpln_expected_location_data.desc = fmt::format("{}{}", brieferDesc, notes);


  navaid_targets[0] = start_na;
  navaid_targets[1] = target_na;

  return navaid_targets;

}


// -----------------------------------



missionx::mx_return
RandomEngine::gen_prepare_mission_based_on_ils_search (IXMLNode &in_xTemplateNode, IXMLNode &inout_meta_node)
{
  std::string outErr;
  missionx::mx_return out_func_result = true;

  // const auto plane_type_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types_enum::plane_type_props)); // plane type
  // this->setPlaneType (static_cast<missionx::mx_plane_types_enum> (plane_type_i)); // set plane type in class level for other function too
  // v25.10.2
  const auto plane_type_enum_i = RandomEngine::gen_parse_plane_type (data_manager::prop_userDefinedMission_ui, in_xTemplateNode, inout_meta_node);
  this->setPlaneType (plane_type_enum_i); // set plane type in class level for other function usage too


  auto navaid_targets = gen_get_ils_targets (&RandomEngine::random_thread_state, in_xTemplateNode, RandomEngine::shared_navaid_info, outErr);
  if (!outErr.empty () || navaid_targets.empty ())
  {
    // missionx::RandomEngine::setError (outErr);
    if (outErr.empty ())
      outErr = "No valid targets were generated.";

    out_func_result.addErrMsg (outErr, true);
    return out_func_result;
  }

  //////////////////////
  // Prepare Main Nodes
  /////////////////////
  for (auto &target_navaid : navaid_targets | std::views::values)
  {
    target_navaid.fpln_leg_name  = gen_leg_name (&this->seq_waypoints, mxconst::get_GPS_WP (), "leg", target_navaid);
  }
  // force cargo type mission. Used when calling "gen_gather_navaid_metadata_relative_to_target()" function.
  Utils::xml_set_attribute_in_node <int>(this->xMetadata, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::cargo));

  // validate navaid targets
  int valid_navaids_i = 0;
  auto navaids_validation = gen_validate_navaids (navaid_targets, valid_navaids_i);
  if (!navaids_validation.result) // if there is a failure
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg (navaids_validation.getErrorsAsText (), true);
    return out_func_result;
  }

  if ( valid_navaids_i != static_cast<int>(navaid_targets.size ()) )
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg ( fmt::format("Valid targets found: {}, is not the same as overall generated targets: {}", valid_navaids_i, navaid_targets.size ()), true);
    return out_func_result;
  }

  // ----------------------
  // -- Add <briefer> node - Start Location BUT NOT the description.
  // ----------------------
  navaid_targets[0].fpln_navaid_was_already_prepared = true; // force flag
  gen_briefer_phase_02_base_node_from_navaid (navaid_targets[0], RandomEngine::shared_navaid_info, false);

  // ------------------------------------------------------------------
  // Construct all mission <leg> nodes
  // navaid_targets: [0] = start/briefer, [1]..[N-1] legs.
  // ------------------------------------------------------------------
  RandomEngine::gen_create_all_leg_nodes_based_on_navaid_targets (navaid_targets);

  for (auto &[indx, target_navaid] : navaid_targets)
  {
    // Add external inventory
    // will skip any navaid that is the same as the Start location.
    if (!target_navaid.flag_is_same_as_start_location ) //
    {
      target_navaid.fpln_xml_inv_node = gen_add_inventory_phase01_node (indx, target_navaid, map_osm_inventory_track);
      //  skip items phase, if it is the last location or inventory node is empty.
      if ( !target_navaid.fpln_xml_inv_node.isEmpty () && navaid_targets.contains (indx+1))
        gen_add_inventory_phase02_add_items (target_navaid);
    }

    if (indx == 0) // skip briefer
    {
      target_navaid.fpln_mission_phase = missionx::enums::mx_rnd_mission_phase::start;
      continue;
    }

    // add start messages
    gen_leg_start_messages (this->seq_messages, target_navaid, navaid_targets, this->xMessages, false);

    // add 3d marker
    gen_add_3d_marker_to_current_target (target_navaid.fpln_xml_target_leg_node, target_navaid);

    gen_3d_parse_instances_in_leg (target_navaid.fpln_xml_target_leg_node, target_navaid);

    target_navaid.synchToPoint ();
    target_navaid.fpln_xml_target_leg_node = this->xFlightLegs.addChild (target_navaid.fpln_xml_target_leg_node);

    // add task trigger nodes to main trigger node.
    for (auto &node : target_navaid.fpln_leg_vec_trigger_nodes)
      this->xTriggers.addChild (node.deepCopy ());

    // add target navaid objective node to the main objectives node
    this->xObjectives.addChild (target_navaid.fpln_leg_objective_node.deepCopy ());

  } // end loop over all targets and adding final touches to each flight leg and adding them to the main mission nodes.

  // prepare a flight plan to show the end user
  this->cumulative_location_desc_s = gen_get_cumulative_fpln_desc (navaid_targets);

  // check [abort]
  if (RandomEngine::random_thread_state.flagAbortThread)
  {
    out_func_result.addErrMsg ("User asked to abort.", true);
    return out_func_result;
  }

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


    this->xGPS.addChild (p_gps_node); // no skewed navaids
  }

  // ----------------------------
  // add Briefer description
  // ----------------------------
  gen_briefer_phase_03_add_desc (navaid_targets, false);
  this->xBriefer = navaid_targets[0].fpln_xml_target_leg_node.deepCopy ();

  // v25.10.1 Add Cold and dark
  RandomEngine::xDrefStartColdAndDark = gen_set_and_get_start_cold_and_dark (in_xTemplateNode, navaid_targets[1]);


  // add <mission_info>
  if (!gen_read_mission_info_element ()) // <mission_info>
  {
    missionx::RandomEngine::random_thread_state.flagAbortThread = true;
    out_func_result.addErrMsg ("No <mission_info> node was found in template.", true);
  }

  // Add all inventories to the global xInventories node
  for (auto &[key, nav] : navaid_targets )
  {
    // add to inventories
    if (!nav.fpln_xml_inv_node.isEmpty ())
      nav.fpln_xml_inv_node = this->xInventoris.addChild (nav.fpln_xml_inv_node);
  }

  #ifndef RELEASE
  Log::logMsgThread (fmt::format ("-------------- <CONTENT_MISSION> RESULTS - Post {} --------------", __func__));
  Log::logMsgThread (fmt::format ("BRIEFER_INFO:\n{}\n", Utils::xml_get_node_content_as_text (this->xBriefer)));
  Log::logMsgThread (fmt::format ("BRIEFER:\n{}\n", Utils::xml_get_node_content_as_text (navaid_targets[0].fpln_xml_target_leg_node))); // we store the briefer in [0]
  Log::logMsgThread (fmt::format ("TRIGGERS:\n{}\n", Utils::xml_get_node_content_as_text (this->xTriggers)));
  Log::logMsgThread (fmt::format ("OBJECTIVES:\n{}\n", Utils::xml_get_node_content_as_text (this->xObjectives)));
  Log::logMsgThread (fmt::format ("FLIGHT LEGS:\n{}\n", Utils::xml_get_node_content_as_text (this->xFlightLegs)));
  Log::logMsgThread (fmt::format ("Inventories:\n{}\n", Utils::xml_get_node_content_as_text (this->xInventoris)));
  Log::logMsgThread (fmt::format ("GPS:\n{}\n", Utils::xml_get_node_content_as_text (this->xGPS)));
  Log::logMsgThread (fmt::format ("-------------- END <CONTENT_MISSION> RESULTS - {} --------------", __func__));
  #endif // !RELEASE

  return out_func_result; // should be true

}


// -----------------------------------



// bool
// RandomEngine::prepare_mission_based_on_ils_search (IXMLNode &pNode)
// {
//   assert (!pNode.isEmpty () && !data_manager::prop_userDefinedMission_ui.node.isEmpty () && "Empty template or prop_userDefinedMission_ui are empty!");
//
//   const auto plane_type_i     = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types_enum::plane_type_props)); // plane type
//   const auto fpln_id_picked_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FPLN_ID_PICKED (), -1); // max slider
//   auto       fromICAO         = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FROM_ICAO (), "");
//   auto       toICAO           = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_TO_ICAO (), "");
//
//   if (fpln_id_picked_i < 0 || Utils::isElementExists (missionx::data_manager::indexPointer_for_ILS_rows_tableVector, fpln_id_picked_i) == false)
//   {
//     RandomEngine::setError ("Could not find the ILS flight plan with index id: " + Utils::formatNumber<int> (fpln_id_picked_i) + ", aborting mission template generating.");
//     return false;
//   }
//   if (fromICAO.empty ())
//   {
//     RandomEngine::setError ("[Random ILS Error] No source ICAO was found, aborting mission template generating.");
//     return false;
//   }
//
//
//   // convert to native plane type from "int"
//   const auto conv_plane_type_i = static_cast<missionx::def_mx_plane_type_enum> (plane_type_i);
//   this->setPlaneType (conv_plane_type_i); // set plane type in class level for other function too
//
//   // fetch the fpln struct to work with
//   auto const lmbda_get_ils_data = [] (const int inPicked_id, const std::vector<missionx::mx_ils_airport_row_strct> &inRow_vec)
//   {
//     missionx::mx_ils_airport_row_strct dummy; // initialize dummy.seq = -1
//     for (auto f : inRow_vec)
//     {
//       if (f.seq == inPicked_id)
//         return f;
//     }
//
//     return dummy;
//   };
//
//   auto to_icao = lmbda_get_ils_data (fpln_id_picked_i, missionx::data_manager::table_ILS_rows_vec);
//   if (to_icao.seq < 0)
//   {
//     RandomEngine::setError ("ILS Flight plan is invalid. Index id: " + Utils::formatNumber<int> (fpln_id_picked_i) + ", aborting mission template generating.");
//   }
//   else
//   {
//
//     RandomEngine::shared_navaid_info.navAid.init ();
//     RandomEngine::shared_navaid_info.navAid.setID (fromICAO);
//     if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
//     {
//       RandomEngine::setError ( fmt::format( "[{}] Start Navaid: {}. Failed to find Airport using original Navaid. Notify developer.", __func__, fromICAO) );
//       return false;
//     }
//     RandomEngine::shared_navaid_info.navAid.synchToPoint ();
//     // if we reached here then we should have startICAO NavAid information and the targetICAO
//     NavAidInfo start_na = RandomEngine::shared_navaid_info.navAid;
//
//
//     // v3.0.253.11 force plane position as starting location
//     if (missionx::RandomEngine::get_user_wants_to_start_from_plane_position ()) // v3.0.253.11
//     {
//       start_na.lat = static_cast<float> (RandomEngine::planeLocation.getLat ());
//       start_na.lon = static_cast<float> (RandomEngine::planeLocation.getLon ());
//       start_na.heading = static_cast<float> (RandomEngine::planeLocation.getHeading ());
//     }
//
//
//     start_na.synchToPoint ();
//     if (start_na.getName ().empty ())
//       start_na.setName (mxconst::get_ELEMENT_BRIEFER ());
//     // try to locate a ramp
//     std::string err;
//
//     // try to locate a ramp v2 - DEBUG
//     if (!missionx::RandomEngine::get_user_wants_to_start_from_plane_position () && !filterAndPickRampBasedOnPlaneType (start_na, err, missionx::mxFilterRampType::start_ramp))
//     {
//       Log::logMsgThread (fmt::format ("[{}] {}", __func__, err));
//     }
//
//
//     RandomEngine::listNavInfo.emplace_back (start_na); // add NavInfo into a list
//
//     // handle target location
//     RandomEngine::shared_navaid_info.navAid.init ();
//     RandomEngine::shared_navaid_info.navAid.setID (toICAO);
//     if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
//     {
//       RandomEngine::setError (fmt::format ("[{}] Target Navaid: {}, Failed to find Airport using original Navaid. Notify developer.", __func__, toICAO));
//       return false;
//     }
//     RandomEngine::shared_navaid_info.navAid.synchToPoint ();
//     NavAidInfo target_na = RandomEngine::shared_navaid_info.navAid;
//     target_na.synchToPoint ();
//
//
//     if (!filterAndPickRampBasedOnPlaneType (target_na, err, missionx::mxFilterRampType::end_ramp)) // v3.303.12_r2
//     {
//       Log::logMsgThread (fmt::format ("[{}, Target ILS] {}", __func__, err));
//     }
//     RandomEngine::listNavInfo.emplace_back (target_na); // add NavInfo into a list
//
//     // Add to GPS
//     if (!xGPS.isEmpty ())
//     {
//       xGPS.addChild (start_na.node.deepCopy ());
//       xGPS.addChild (target_na.node.deepCopy ());
//       #ifndef RELEASE
//       Utils::xml_print_node (xGPS, true);
//       #endif // !RELEASE
//     }
//
//     //////////////////////
//     // Prepare Main Nodes
//     /////////////////////
//     std::string plane_type_s = missionx::RandomEngine::translatePlaneTypeToString (conv_plane_type_i); // convert type to string and store it in mission node
//     missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_PLANE_TYPE_S (), plane_type_s);
//     pNode.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ());
//
//     IXMLNode xLegNode    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_LEG ().c_str ()).deepCopy ();
//     IXMLNode xMapTask    = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TASK ().c_str ()).deepCopy ();
//     IXMLNode xMapTrigger = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str ()).deepCopy ();
//     IXMLNode xMapMessage = Utils::xml_get_node_from_node_tree_IXMLNode (data_manager::xmlMappingNode, mxconst::get_ELEMENT_MESSAGE (), true); // holds message element from MAPPING
//
//
//     // prepare briefer
//     NavAidInfo naBriefer       = RandomEngine::listNavInfo.front ();
//     IXMLNode   xLocationAdjust = missionx::RandomEngine::xRootTemplate.getChildNode (mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ().c_str ()).deepCopy ();
//     if (xLocationAdjust.isEmpty ())
//     {
//       RandomEngine::setError ("[random ILS] No <" + mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION () + "> was found. Template malform, abort template generation !!!");
//
//       return false;
//     }
//     xLocationAdjust.updateName (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ());
//     // remove any clear data
//     int               nClear      = xLocationAdjust.nClear (); // remove any CDATA or COMMENTS or any clear() type element
//     const std::string from_to_s   = get_short_flight_description_from_to (start_na.getName (), start_na.getID (), target_na.getName (), target_na.getID ()); //"From: " + start_na.getName() + "(" + start_na.getID() + ") to " + target_na.getName() + "(" + target_na.getID() + ")";
//     std::string       brieferDesc = from_to_s + "\n\n" + "Hello pilot.\nYou have assigned an ILS flight to " + target_na.getID () + " and runway: " + to_icao.loc_rw_s + ". Learn the route and fly it according to plan or modify it if you so wish.\n\nBlue skys.";
//     std::string       notes       = "\n\nDestination Notes:\n==============\nAirport: " + to_icao.toName_s + "(" + to_icao.toICAO_s + ")\tAirport Elev.: " + mxUtils::formatNumber<int> (to_icao.ap_elev_ft_i) + " ft." + "\nEstimate distance: " + mxUtils::formatNumber<double> (to_icao.distnace_d, 0) + "nm. \tRunway to Land: " + to_icao.loc_rw_s + ".\nLocalizer Type: " + to_icao.locType_s + ". \tLocalizer bearing: " + mxUtils::formatNumber<int> (to_icao.loc_bearing_i) + " \tlocalizer frq.: " + mxUtils::getFreqFormated (to_icao.loc_frq_mhz);
//
//
//     for (int i = 0; i < nClear; ++i)
//       xLocationAdjust.deleteClear (); // change from remove "i" to remove first
//
//
//     xLocationAdjust.updateAttribute (naBriefer.getLat ().c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
//     xLocationAdjust.updateAttribute (naBriefer.getLon ().c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
//     xLocationAdjust.updateAttribute (naBriefer.getHeading_s ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());
//     xLocationAdjust.updateAttribute (naBriefer.getRampInfo ().c_str (), mxconst::get_ATTRIB_RAMP_INFO ().c_str (), mxconst::get_ATTRIB_RAMP_INFO ().c_str ());
//
//     RandomEngine::lastFlightLegNavInfo = naBriefer;
//     if (naBriefer.getNavAidName ().empty ()) // v3.303.10
//       RandomEngine::lastFlightLegNavInfo.flightLegName = mxconst::get_ELEMENT_BRIEFER ();
//
//     RandomEngine::lastFlightLegNavInfo.synchToPoint ();
//
//     this->xBriefer = this->xDummyTopNode.addChild (mxconst::get_ELEMENT_BRIEFER ().c_str ());
//     this->xBriefer.addAttribute (mxconst::get_ATTRIB_STARTING_LEG ().c_str (), "leg_1"); // leg_1 is default value, but it can be changed when using <content> elements with "element sets"
//     IXMLNode cNode = xBriefer.addChild (xLocationAdjust);
//     Utils::xml_add_cdata (this->xBriefer, brieferDesc + notes); //
//
//     // Add inventory if exists in mapping
//     if (data_manager::xmlMappingNode.nChildNode (mxconst::get_ELEMENT_INVENTORY ().c_str ()) > 0)
//     {
//       // this->injectInventory(mxconst::get_ELEMENT_BRIEFER(), naBriefer.p.node, mxInvSource::point); // name of store will start with briefer
//       this->addInventory (mxconst::get_ELEMENT_BRIEFER (), naBriefer.node, mxInvSource::point); // name of store will start with briefer
//     }
//
//     //// Finished Briefer construction ////
//
//     // Prepare Objective + Tasks + Triggers and Leg flight plan
//     if (xLegNode.isEmpty ())
//     {
//       RandomEngine::setError ("Could not find the mapping node: LEG, aborting mission template generating.");
//       return false;
//     }
//     else
//     {
//       const std::string legName       = std::string (mxconst::get_ELEMENT_LEG ()) + "_" + Utils::formatNumber<int> (fpln_id_picked_i);
//       const std::string objectiveName = legName + "_objective";
//
//       IXMLNode xObjective = this->xObjectives.addChild (mxconst::get_ELEMENT_OBJECTIVE ().c_str ());
//       xObjective.updateAttribute (objectiveName.c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
//
//       // create tasks based on waypoint list, excpe the firt one
//       int counter = 0;
//       for (auto &na : RandomEngine::listNavInfo)
//       {
//         counter++;
//         if (counter == 1)
//           continue; // it is the briefer starting location
//
//         std::string task_name    = "task_" + Utils::formatNumber<int> (counter);
//         std::string trigger_name = "trig_" + task_name;
//
//         IXMLNode xTask = xObjective.addChild (mxconst::get_ELEMENT_TASK ().c_str ());
//         xTask.updateAttribute (task_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
//         xTask.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str (), mxconst::get_ATTRIB_BASE_ON_TRIGGER ().c_str ());
//         xTask.updateAttribute ("3", mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC ().c_str (), mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC ().c_str ()); // evaluate success for 3 seconds
//         xTask.updateAttribute (((counter == static_cast<int> (RandomEngine::listNavInfo.size ())) ? "yes" : ""), mxconst::get_ATTRIB_MANDATORY ().c_str (), mxconst::get_ATTRIB_MANDATORY ().c_str ()); // evaluate success for 3 seconds
//
//         // add the trigger
//         IXMLNode xTrigger = xMapTrigger.deepCopy ();
//         xTrigger.updateAttribute (trigger_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
//         xTrigger.updateAttribute ("rad", mxconst::get_ATTRIB_TYPE ().c_str (), mxconst::get_ATTRIB_TYPE ().c_str ()); // set type as radius based "rad".
//         Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LAT (), na.getLat (), mxconst::get_ELEMENT_POINT ());
//         Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LONG (), na.getLon (), mxconst::get_ELEMENT_POINT ());
//
//         if (counter == static_cast<int> (RandomEngine::listNavInfo.size ()))
//         {
//           Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT (), "100", mxconst::get_ELEMENT_RADIUS ());
//           Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "true", mxconst::get_ELEMENT_CONDITIONS ());
//         }
//         else
//         {
//           Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_LENGTH_MT (), "4000", mxconst::get_ELEMENT_RADIUS ());
//           Utils::xml_search_and_set_attribute_in_IXMLNode (xTrigger, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "", mxconst::get_ELEMENT_CONDITIONS ());
//         }
//
//         this->xTriggers.addChild (xTrigger);
//
//       } // end loop over all listNavInfo
//
//       ////// Construct Flight Leg //////
//       static const std::string STARTING_MESSAGE_NAME = "starting_message";
//       const std::string        leg_message_name_s    = STARTING_MESSAGE_NAME + "_" + Utils::formatNumber<int> (counter);
//
//       xLegNode.updateAttribute (legName.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
//       xLegNode.updateAttribute (from_to_s.c_str (), mxconst::get_ATTRIB_TITLE ().c_str (), mxconst::get_ATTRIB_TITLE ().c_str ());
//       Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME (), objectiveName, mxconst::get_ELEMENT_LINK_TO_OBJECTIVE ()); // link to objective
//       Utils::xml_search_and_set_attribute_in_IXMLNode (xLegNode, mxconst::get_ATTRIB_NAME (), leg_message_name_s, mxconst::get_ELEMENT_START_LEG_MESSAGE ()); // link to objective
//
//
//       // Add message to flight leg
//       IXMLNode xMessage01 = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_MESSAGE ().c_str ()).deepCopy ();
//       if (!xMessage01.isEmpty ())
//       {
//         xMessage01.updateAttribute (leg_message_name_s.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
//         IXMLNode mixText = Utils::xml_get_or_create_node_ptr (xMessage01, mxconst::get_ELEMENT_MIX ());
//         mixText.updateAttribute ("text", mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE ().c_str (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE ().c_str ());
//         const std::string text = "Hello pilot\nYou will fly the route " + from_to_s + ". \n\nGood Luck";
//         Utils::xml_add_cdata (mixText, text);
//
//         this->xMessages.addChild (xMessage01);
//       }
//
//
//       // Add Ending Marker
//       if (IXMLNode xDisplayEndLocation = xLegNode.addChild (mxconst::get_ELEMENT_DISPLAY_OBJECT ().c_str ()); !xDisplayEndLocation.isEmpty ())
//       {
//         xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_INSTANCE_NAME ().c_str (), std::string ("marker_" + legName).c_str ());
//         xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_NAME ().c_str (), "marker"); // this is the name of the marker in the "template_blank_4_ui.xml" file
//         xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_TARGET_MARKER_B ().c_str (), "true");
//
//
//         NavAidInfo naLast = RandomEngine::listNavInfo.back ();
//         xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LAT ().c_str (), naLast.getLat ().c_str ());
//         xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_LONG ().c_str (), naLast.getLon ().c_str ());
//         xDisplayEndLocation.addAttribute (mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT ().c_str (), "50"); // display marker 50ft above ground
//       }
//
//       // Add Flight Leg DESCRIPTION
//       IXMLNode xDesc = xLegNode.getChildNode (mxconst::get_ELEMENT_DESC ().c_str ());
//       if (xDesc.isEmpty ())
//         xDesc = xLegNode.addChild (mxconst::get_ELEMENT_DESC ().c_str ());
//
//       Utils::xml_add_cdata (xDesc, brieferDesc + notes);
//
//       // We only have one leg
//       this->mission_xml_data.currentLegName = legName;
//       Utils::xml_add_node_to_element_IXMLNode (xFlightLegs, xLegNode);
//       Utils::addElementToMap (mapFlightPlanOrder_si, this->mission_xml_data.currentLegName, 1);
//       Utils::addElementToMap (mapFLightPlanOrder_is, 1, this->mission_xml_data.currentLegName);
//
//     } // end xLegNode is valid or not. end creating the flight leg
//
//   } // end prepare mission
//
//
//   return true;
// }


// -----------------------------------


void
RandomEngine::add_waypoints_for_fpln_or_simbrief (IXMLNode &pNode)
{
  if (pNode.isEmpty ())
    return;


  if (const auto s_waypoints = data_manager::prop_userDefinedMission_ui.getChildTextValue (mxconst::get_PROP_ADD_ROUTE_WAYPOINTS (), "");
    s_waypoints != "false" && !s_waypoints.empty () && pNode.nChildNode () > 0) // v25.10.2 added "false" test
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

          if (missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_and_guess_nav_aid_info_mainThread))
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

std::map<int, NavAidInfo>
RandomEngine::gen_get_user_fpln_or_simbrief_targets (missionx::base_thread::strct_thread_state *inoutThreadState
                                                    , const IXMLNode &in_template_node
                                                    , strct_shared_random_airport_info &inout_shared_navaid
                                                    , std::string &outErr)
{
  missionx::mx_ext_internet_fpln_strct fpln;
  std::map<int, NavAidInfo> navaid_targets;

  outErr.clear ();

  auto plane_type_i                             = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types_enum::plane_type_props)); // plane type
  fpln.fpln_unique_id                           = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FPLN_ID_PICKED (), -1); // max slider
  fpln.fromICAO_s                               = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_FROM_ICAO (), "");
  fpln.toICAO_s                                 = Utils::readAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_TO_ICAO (), "");
  fpln.formated_nav_points_with_guessed_names_s = data_manager::prop_userDefinedMission_ui.getChildTextValue (mxconst::get_PROP_ADD_ROUTE_WAYPOINTS ());

  #ifndef RELEASE
  auto debug_plane_type_enum_i = getPlaneType_enum ();
  #endif

  if ((fpln.fpln_unique_id < 0) + (fpln.fromICAO_s.empty ()) + (fpln.toICAO_s.empty ()))
  {
    navaid_targets.clear ();
    outErr = "Flight plan may not contain valid FROM/TO ICAO information. Aborting mission generation.";
    return navaid_targets;
  }

  RandomEngine::shared_navaid_info.navAid.init ();
  RandomEngine::shared_navaid_info.navAid.setID (fpln.fromICAO_s);

  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
  {
    navaid_targets.clear ();
    outErr = fmt::format ("[{}] Start Navaid: {}, Failed to find Airport using original Navaid. Notify developer.", __func__, fpln.fromICAO_s);
    return navaid_targets;
  }
  RandomEngine::shared_navaid_info.navAid.synchToPoint ();
  // if we reached here then we should have a valid startICAO NavAid information
  NavAidInfo start_na (RandomEngine::shared_navaid_info.navAid);

  // force plane position as starting location, based on user preference
  if (missionx::RandomEngine::get_user_wants_to_start_from_plane_position ())
  {
    start_na.lat     = static_cast<float> (RandomEngine::planeLocation.getLat ());
    start_na.lon     = static_cast<float> (RandomEngine::planeLocation.getLon ());
    start_na.heading = static_cast<float> (RandomEngine::planeLocation.getHeading ());

    start_na.synchToPoint ();
    if (start_na.getName ().empty ())
      start_na.setName (mxconst::get_ELEMENT_BRIEFER ());
  }
  else
  {
    auto search_start_ramp_result = RandomEngine::gen_get_ramp_based_on_plane_type (start_na, getPlaneType_enum(), missionx::mxFilterRampType::start_ramp);
    if (!search_start_ramp_result.result || search_start_ramp_result.getInfoIndex ())
    {
      Log::logMsgThread (fmt::format ("[{}] {}", __func__, search_start_ramp_result.getErrorsAndInfoAsText () ) );
    }
  }



  // try to locate a ramp
  // outErr.clear ();
  // if (!missionx::RandomEngine::get_user_wants_to_start_from_plane_position () && !filterAndPickRampBasedOnPlaneType (start_na, outErr, missionx::mxFilterRampType::start_ramp))
  // outErr.clear (); // we do not fail if there is no ramp

  // auto result = RandomEngine::gen_get_ramp_based_on_plane_type (start_na, getPlaneType_enum(), missionx::mxFilterRampType::start_ramp);
  // if (!missionx::RandomEngine::get_user_wants_to_start_from_plane_position () && !result.result )
  // {
  //   Log::logMsgThread (fmt::format ("[{}] {}", __func__, result.getErrorsAndInfoAsText () ) );
  // }
  // outErr.clear (); // we do not fail if there is no ramp


  //////////////////////////
  // handle target location
  /////////////////////////
  RandomEngine::shared_navaid_info.navAid.init ();
  RandomEngine::shared_navaid_info.navAid.setID (fpln.toICAO_s);
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nav_aid_info_mainThread))
  {
    navaid_targets.clear ();
    outErr = fmt::format ("[{}] Target Navaid: {}, Failed to find Airport using original Navaid. Notify developer.", __func__, fpln.toICAO_s);
    return navaid_targets;
  }
  RandomEngine::shared_navaid_info.navAid.synchToPoint ();
  NavAidInfo target_na (RandomEngine::shared_navaid_info.navAid); // v25.04.2, it also calls "syncToPoint()"

  // get ramp location
  // outErr.clear ();
  // if (!filterAndPickRampBasedOnPlaneType (target_na, outErr, missionx::mxFilterRampType::end_ramp)) // v3.303.12_r2


  auto search_end_ramp_result = RandomEngine::gen_get_ramp_based_on_plane_type (target_na, getPlaneType_enum(), missionx::mxFilterRampType::end_ramp);
  if (!search_end_ramp_result.result || search_end_ramp_result.getInfoIndex ())
  {
    Log::logMsgThread (fmt::format ("[{}, Target ILS] {}", __func__, search_end_ramp_result.getErrorsAndInfoAsText ()));
  }
  // outErr.clear ();

  // Construct Briefer Description
  const std::string from_to_s   = get_short_flight_description_from_to (start_na.getName (), start_na.getID (), target_na.getName (), target_na.getID ());
  std::string       brieferDesc = "Hello pilot.\nYou have been assigned a flight " + from_to_s + ".\nGo over the route and fly it according to plan or modify it if you so wish.\n\nBlue skys.";
  std::string       notes       = "\n\nDestination Notes:\n==============\nAirport: " + target_na.getNavAidName () + "(" + target_na.getID () + ")\nWaypoints:\n" + ((fpln.formated_nav_points_with_guessed_names_s != "false")? fpln.formated_nav_points_with_guessed_names_s: "");

  start_na.fpln_expected_location_data.desc = fmt::format ("{}\n\n{}", brieferDesc, notes);

  // Add to list of targets
  if (start_na.is_lat_lon_valid () && target_na.is_lat_lon_valid ())
  {
    navaid_targets[0] = start_na;
    navaid_targets[1] = target_na;
  }

  if (navaid_targets.empty ())
    outErr = fmt::format ("[{}] One or more navaids are invalid.", __func__);

  return navaid_targets;
}


// -----------------------------------


missionx::mx_return
RandomEngine::gen_prepare_mission_based_on_user_fpln_or_simbrief (IXMLNode &in_xTemplateNode, IXMLNode & inout_meta_node)
{
  assert (!in_xTemplateNode.isEmpty () && !data_manager::prop_userDefinedMission_ui.node.isEmpty () && fmt::format("[{}] Empty template or prop_userDefinedMission_ui node.", __func__).c_str ());

  missionx::mx_return out_func_result = true;
  // v25.10.2
  const auto plane_type_enum_i = RandomEngine::gen_parse_plane_type (data_manager::prop_userDefinedMission_ui, in_xTemplateNode, inout_meta_node);
  this->setPlaneType (plane_type_enum_i); // set plane type in class level for other function usage too

  std::string outErr;
  std::map<int, NavAidInfo> navaid_targets = gen_get_user_fpln_or_simbrief_targets (&RandomEngine::random_thread_state, in_xTemplateNode, RandomEngine::shared_navaid_info, outErr );
  if (!outErr.empty () || navaid_targets.empty ())
  {
    // missionx::RandomEngine::setError (outErr);
    if (outErr.empty ())
      outErr = "No valid targets were generated.";

    out_func_result.addErrMsg (outErr, true);
    return out_func_result;
  }

  auto plane_type_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types_enum::plane_type_props)); // plane type

  // convert to native plane type from "int"
  auto conv_plane_type_i = static_cast<missionx::def_mx_plane_type_enum> (plane_type_i);
  this->setPlaneType (conv_plane_type_i); // set plane type in class level for other function usage too

  //////////////////////
  // Prepare Main Nodes
  /////////////////////
  for (auto &target_navaid : navaid_targets | std::views::values)
  {
    target_navaid.fpln_leg_name  = gen_leg_name (&this->seq_waypoints, mxconst::get_GPS_WP (), "leg", target_navaid);
  }
  // force cargo type mission. Used when calling "gen_gather_navaid_metadata_relative_to_target()" function.
  Utils::xml_set_attribute_in_node <int>(this->xMetadata, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::cargo));

  // validate navaid targets
  int valid_navaids_i = 0;
  auto navaids_validation = gen_validate_navaids (navaid_targets, valid_navaids_i);
  if (!navaids_validation.result) // if there is a failure
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg (navaids_validation.getErrorsAsText (), true);
    return out_func_result;
  }

  if ( valid_navaids_i != static_cast<int>(navaid_targets.size ()) )
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg ( fmt::format("Valid targets found: {}, is not the same as overall generated targets: {}", valid_navaids_i, navaid_targets.size ()), true);
    return out_func_result;
  }

  // ----------------------
  // -- Add <briefer> node - Start Location BUT NOT the description.
  // ----------------------
  navaid_targets[0].fpln_navaid_was_already_prepared = true; // force flag
  gen_briefer_phase_02_base_node_from_navaid (navaid_targets[0], RandomEngine::shared_navaid_info, false);

  // ------------------------------------------------------------------
  // Construct all mission <leg> nodes
  // navaid_targets: [0] = start/briefer, [1]..[N-1] legs.
  // ------------------------------------------------------------------
  RandomEngine::gen_create_all_leg_nodes_based_on_navaid_targets (navaid_targets);

  for (auto &[indx, target_navaid] : navaid_targets)
  {
    // Add external inventory
    // will skip any navaid that is the same as the Start location.
    if (!target_navaid.flag_is_same_as_start_location ) //
    {
      target_navaid.fpln_xml_inv_node = gen_add_inventory_phase01_node (indx, target_navaid, map_osm_inventory_track);
      //  skip items phase, if it is the last location or inventory node is empty.
      if ( !target_navaid.fpln_xml_inv_node.isEmpty () && navaid_targets.contains (indx+1))
        gen_add_inventory_phase02_add_items (target_navaid);
    }

    if (indx == 0) // skip briefer
    {
      target_navaid.fpln_mission_phase = missionx::enums::mx_rnd_mission_phase::start;
      continue;
    }

    // add start messages
    gen_leg_start_messages (this->seq_messages, target_navaid, navaid_targets, this->xMessages, false);

    // add 3d marker
    gen_add_3d_marker_to_current_target (target_navaid.fpln_xml_target_leg_node, target_navaid);

    gen_3d_parse_instances_in_leg (target_navaid.fpln_xml_target_leg_node, target_navaid);

    target_navaid.synchToPoint ();
    target_navaid.fpln_xml_target_leg_node = this->xFlightLegs.addChild (target_navaid.fpln_xml_target_leg_node);

    // add task trigger nodes to main trigger node.
    for (auto &node : target_navaid.fpln_leg_vec_trigger_nodes)
      this->xTriggers.addChild (node.deepCopy ());

    // add target navaid objective node to the main objectives node
    this->xObjectives.addChild (target_navaid.fpln_leg_objective_node.deepCopy ());

  } // end adding final touches to each flight leg and adding them to the main mission nodes.

  // prepare a flight plan to show the end user
  this->cumulative_location_desc_s = gen_get_cumulative_fpln_desc (navaid_targets);

  // check [abort]
  if (RandomEngine::random_thread_state.flagAbortThread)
  {
    out_func_result.addErrMsg ("User asked to abort.", true);
    return out_func_result;
  }

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


    this->xGPS.addChild (p_gps_node); // no skewed navaids
  }

  // ----------------------------
  // add Briefer description
  // ----------------------------
  gen_briefer_phase_03_add_desc (navaid_targets, false);
  this->xBriefer = navaid_targets[0].fpln_xml_target_leg_node.deepCopy ();

  // v25.10.1 Add Cold and dark
  RandomEngine::xDrefStartColdAndDark = gen_set_and_get_start_cold_and_dark (in_xTemplateNode, navaid_targets[1]);

  // add <mission_info>
  if (!gen_read_mission_info_element ()) // <mission_info>
  {
    missionx::RandomEngine::random_thread_state.flagAbortThread = true;
    out_func_result.addErrMsg ("No <mission_info> node was found in template.", true);
  }

  // Add all inventories to the global xInventories node
  for (auto &[key, nav] : navaid_targets )
  {
    // add to inventories
    if (!nav.fpln_xml_inv_node.isEmpty ())
      nav.fpln_xml_inv_node = this->xInventoris.addChild (nav.fpln_xml_inv_node);
  }

  #ifndef RELEASE
  Log::logMsgThread (fmt::format ("-------------- <CONTENT_MISSION> RESULTS - Post {} --------------", __func__));
  Log::logMsgThread (fmt::format ("BRIEFER_INFO:\n{}\n", Utils::xml_get_node_content_as_text (this->xBriefer)));
  Log::logMsgThread (fmt::format ("BRIEFER:\n{}\n", Utils::xml_get_node_content_as_text (navaid_targets[0].fpln_xml_target_leg_node))); // we store the briefer in [0]
  Log::logMsgThread (fmt::format ("TRIGGERS:\n{}\n", Utils::xml_get_node_content_as_text (this->xTriggers)));
  Log::logMsgThread (fmt::format ("OBJECTIVES:\n{}\n", Utils::xml_get_node_content_as_text (this->xObjectives)));
  Log::logMsgThread (fmt::format ("FLIGHT LEGS:\n{}\n", Utils::xml_get_node_content_as_text (this->xFlightLegs)));
  Log::logMsgThread (fmt::format ("Inventories:\n{}\n", Utils::xml_get_node_content_as_text (this->xInventoris)));
  Log::logMsgThread (fmt::format ("GPS:\n{}\n", Utils::xml_get_node_content_as_text (this->xGPS)));
  Log::logMsgThread (fmt::format ("-------------- END <CONTENT_MISSION> RESULTS - {} --------------", __func__));
  #endif // !RELEASE

  return out_func_result; // should be true
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
      overpassThreads.emplace_back(missionx::data_manager::fetch_overpass_info_analyze_thread, &missionx::RandomEngine::random_thread_state, nullptr, &q, map_bbox );
      // Sleep 2 seconds between thread dispatch
      std::this_thread::sleep_for (std::chrono::seconds (2)); // wait for 2 seconds before sending a new request
    }

    // Wait for all overpass threads to finish
    for (auto& t : overpassThreads) {
      if (t.joinable()) t.join();
    }

    // check [abort]
    if (RandomEngine::random_thread_state.flagAbortThread)
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
  const int         ui_picked_task_type_i = Utils::readNodeNumericAttrib<int> (inoutMetaNode, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::undefined));
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
RandomEngine::gen_leg_description (IXMLNode &in_xml_leg_node, missionx::NavAidInfo &inout_navaid_target, missionx::NavAidInfo *in_next_leg_as_navaid_ptr) //, random_airport_info_struct &inout_random_airport_info_struct)
{
  // prepare the default description for <leg>
  const std::string target_icao = inout_navaid_target.getID ();
  const std::string target_name = inout_navaid_target.get_name_or_icao_id ();
  const std::string target_desc = inout_navaid_target.gen_locDesc_short ();
  const std::string skewed_target_desc = inout_navaid_target.get_skewed_desc (); // v25.09.2 read the skewed coordinates

  auto lmbda_get_pre_message_for_default_desc_text = [&] ()
  {
    std::string desc_s = target_name;

    if (inout_navaid_target.flag_is_skewed)
      return fmt::format ("Fly to: {}.\nYou will have to search for the target around that location.\n", inout_navaid_target.get_skewed_desc ());

    if (!target_icao.empty ())
      desc_s.append (fmt::format ("({})", target_icao));

    if (desc_s.empty () && !target_desc.empty ())
      return fmt::format ("Fly to: \"{}\".", mxUtils::sanitize_text (target_desc, "_", ' '));

    return fmt::format ("Fly to \"{}\".", mxUtils::sanitize_text (desc_s, "_", ' '));
  };

  const auto        desc_next_target_text        = lmbda_get_pre_message_for_default_desc_text ();
  const auto        desc_distance_text           = "Expected distance: {distance}";
  const auto        desc_elevation_text          = "(Elev: {navaid_elev}ft)";
  auto              desc_wet_text                = (inout_navaid_target.fpln_is_wet) ? "> Your next leg might be above water body.\n" : "\n";
  const std::string default_description_template = fmt::format ("{}\n{} {}.\n\n{}--> Fly Safe <--", desc_next_target_text, desc_distance_text, desc_elevation_text, desc_wet_text);

  // get random node copy
  const IXMLNode xml_custom_desc_from_target_leg_node = Utils::xml_get_node_randomly_by_name_IXMLNode (inout_navaid_target.fpln_xml_osm_q_or_raw_tmpl_node, mxconst::get_ELEMENT_DESC (), false);

  #ifndef RELEASE
  Log::logMsgThread ( fmt::format ( "[{}] <{}> info:\n{}\n<-- end fpln_xml_target_leg_node \n"
                    , __func__, Utils::xml_get_tag_name(inout_navaid_target.fpln_xml_osm_q_or_raw_tmpl_node),  Utils::xml_get_node_content_as_text ( inout_navaid_target.fpln_xml_osm_q_or_raw_tmpl_node, "no <desc> nodes found." ) ) );
  #endif

  // construct the final template of the flight leg description if <desc> node in target <leg> is empty.
  const std::string leg_description = (inout_navaid_target.flag_is_skewed)? default_description_template : Utils::xml_get_text_or_cdata_text (xml_custom_desc_from_target_leg_node, default_description_template); // read custom description

  // replace {special keywords}.
  const std::string final_leg_description_text = RandomEngine::gen_message_with_special_keywords_static (leg_description, inout_navaid_target);

  // prepare <desc> and add it to the <leg>
  IXMLNode xml_desc = in_xml_leg_node.getChildNode (mxconst::get_ELEMENT_DESC ().c_str ());
  if (xml_desc.isEmpty ())
    xml_desc = in_xml_leg_node.addChild (mxconst::get_ELEMENT_DESC ().c_str ());

  // copy attributes from source custom <desc>, if any
  std::set<std::string> whiteList = {mxconst::get_ATTRIB_RANDOM_TAG (), mxconst::get_ATTRIB_SET_NAME (), mxconst::get_ATTRIB_SLOPE_SET_NAME ()};
  Utils::xml_copy_specific_attributes_using_white_list (xml_custom_desc_from_target_leg_node,  xml_desc, &whiteList);
  Utils::xml_set_text (xml_desc, final_leg_description_text);

  return xml_desc;
}

// -----------------------------------


IXMLNode
RandomEngine::gen_leg_node (const std::string &prefix_name, const std::string &postfix_name, missionx::NavAidInfo *inTargetNavAid, const std::list<missionx::structs::strct_node_attribute_key_value> *in_attrib_list, IXMLNode *parentNode)
{
  if (inTargetNavAid == nullptr)
    return IXMLNode::emptyIXMLNode;

  #ifndef RELEASE
  bool isEmpty = inTargetNavAid->fpln_xml_osm_q_or_raw_tmpl_node.isEmpty (); // debug
  #endif


  // v25.09.1 extended leg_node to either use existing <leg> or create one. Can handle Oilrig and OSM Surprise me missions.
  IXMLNode leg_node = missionx::data_manager::xmlMappingNode.getChildNode (mxconst::get_ELEMENT_LEG ().c_str ()).deepCopy ();
  if ( ! inTargetNavAid->fpln_xml_target_leg_node.isEmpty () )
    // leg_node will point to the "fpln_xml_target_leg_node"
    leg_node = Utils::xml_merge_source_with_target_node ( leg_node, inTargetNavAid->fpln_xml_target_leg_node );
  // OSM SURPRISE ME ONLY: copy all subnodes nodes from the <q> if present
  else if (!(inTargetNavAid->fpln_xml_osm_q_or_raw_tmpl_node.isEmpty ()))
  {
    // exclude <inventory> and <desc> nodes. <desc> node will be picked in gen_leg_description() function.
    const std::vector<std::string> in_exclude_nodes = {mxconst::get_ELEMENT_INVENTORY (), mxconst::get_ELEMENT_DESC ()};
    Utils::xml_copy_or_replace_sub_nodes (leg_node, inTargetNavAid->fpln_xml_osm_q_or_raw_tmpl_node, true, &in_exclude_nodes);
  }

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


  return leg_node.deepCopy ();
}


// -----------------------------------


void
RandomEngine::get_skew_target_data (missionx::NavAidInfo &in_target_navaid)
{
  in_target_navaid.synchToPoint ();

  auto lmbda_get_skew_position = [&](const IXMLNode &inTargetPoint)
  {
    // const auto plane_type = missionx::RandomEngine::getPlaneType (); // debug
    const bool flag_display_target_markers_away_from_target = Utils::getNodeText_type_1_5<bool> (system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET (), false);
    if (flag_display_target_markers_away_from_target
        && !in_target_navaid.fpln_is_last_flight_leg
        && (missionx::RandomEngine::getPlaneType () <= static_cast<uint8_t> (def_mx_plane_type_enum::plane_type_helos)) )
    {
      in_target_navaid.flag_is_skewed = true;
      return gen_get_skewed_target_position (inTargetPoint).deepCopy ();
    }

    in_target_navaid.flag_is_skewed = false;
    return IXMLNode::emptyIXMLNode;
    // return inTargetPoint.deepCopy (); // original code
  };

  IXMLNode xPoint = in_target_navaid.p.node.deepCopy ();
  Utils::xml_set_attribute_in_node<bool> (xPoint, mxconst::get_ATTRIB_IS_TARGET_POINT_B (), true, xPoint.getName ()); // A skewed point can still be a target so GPS points can be distinguished.
  in_target_navaid.xml_skewdPointNode = lmbda_get_skew_position (xPoint.deepCopy ()); // xPoint represents the real position.

  in_target_navaid.skewed_location.lat = Utils::readNodeNumericAttrib <double>( in_target_navaid.xml_skewdPointNode, mxconst::get_ATTRIB_LAT (), 0.0 );
  in_target_navaid.skewed_location.lon = Utils::readNodeNumericAttrib <double>( in_target_navaid.xml_skewdPointNode, mxconst::get_ATTRIB_LONG (), 0.0 );
}


// -----------------------------------

missionx::NavAidInfo
RandomEngine::gen_briefer_phase_01_parse_briefer_and_start_location (const IXMLNode &in_xTemplate, IXMLNode &x_briefer_and_start_location)
{
  missionx::NavAidInfo navAid;
  std::string          lat_s, lon_s; // will hold string representation of longitude and latitude

  if (x_briefer_and_start_location.isEmpty ())
  {
    navAid.err = fmt::format("[{}] No <{}>> was found. Template malformed, abort template generation !!!", __func__, mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ());
    return navAid;
  }

  // store the original node
  navAid.fpln_xml_osm_q_or_raw_tmpl_node = x_briefer_and_start_location.deepCopy ();

  // rename element tag to <location_adjust >
  x_briefer_and_start_location.updateName (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ());

  // store element properties in "elementBrieferInfoProperties" for internal use, if needed to remove any "clear" data
  const int   nClear      = x_briefer_and_start_location.nClear (); // remove any CDATA or COMMENTS or any clear() type element
  navAid.fpln_expected_location_data.desc = mxUtils::trim ( Utils::xml_get_text_or_cdata_text (x_briefer_and_start_location, ""), " "); // v25.09.2 added trim spaces from start/end of text // v3.0.241.1 // v3.0.241.9 replace default string with empty string
  for (int i = 0; i < nClear; ++i)
    x_briefer_and_start_location.deleteClear (); // v3.0.241.1 change from remove "i" to remove first

  //// Handle location_type
  const std::string locationOptionType = Utils::readAttrib (x_briefer_and_start_location, mxconst::get_ATTRIB_LOCATION_TYPE (), mxconst::get_ELEMENT_PLANE ());

  ////////////////////
  // if value = plane
  if (mxconst::get_ELEMENT_PLANE () == locationOptionType || get_user_wants_to_start_from_plane_position ()) // v3.0.253.11 added prop_start_from_plane_position
  {
    // v25.09.2
    navAid.lat = static_cast<float>(RandomEngine::planeLocation.lat);
    navAid.lon = static_cast<float>(RandomEngine::planeLocation.lon);
    navAid.heading = RandomEngine::planeLocation.heading;

    // set xPoint from plane
    lat_s = Utils::formatNumber<double> (RandomEngine::planeLocation.getLat (), 8);
    lon_s = Utils::formatNumber<double> (RandomEngine::planeLocation.getLon (), 8);


    x_briefer_and_start_location.updateAttribute (lat_s.c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
    x_briefer_and_start_location.updateAttribute (lon_s.c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
    x_briefer_and_start_location.updateAttribute (Utils::formatNumber<double> (RandomEngine::planeLocation.getElevationInFeet (), 2).c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str ());
    x_briefer_and_start_location.updateAttribute (navAid.getHeading_s ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());
  }
  ////////////////////
  // if value = xy
  else if (mxconst::get_EXPECTED_LOCATION_TYPE_XY () == locationOptionType) // if value = xy
  {
    // check if target lat/long are set, if yes, then use them
    // if not, then check if "random" exists and if its value is not empty. then read the element with points and randomly pick a point.
    // read targetLat/long and see if they are pre-defined from the template.

    navAid.lat = Utils::readNodeNumericAttrib<float> (x_briefer_and_start_location, mxconst::get_ATTRIB_LAT (), 0.0f);
    navAid.lon = Utils::readNodeNumericAttrib<float> (x_briefer_and_start_location, mxconst::get_ATTRIB_LONG (), 0.0f);
    navAid.heading = Utils::readNodeNumericAttrib<float> (x_briefer_and_start_location, mxconst::get_ATTRIB_HEADING_PSI (), RandomEngine::planeLocation.heading);
    navAid.height_mt = (Utils::readNodeNumericAttrib <float>(x_briefer_and_start_location, mxconst::get_ATTRIB_ELEV_FT (), 0.0f)) * missionx::feet2meter;
    // Handle ICAO
    const std::string icao_s = Utils::readAttrib (x_briefer_and_start_location, mxconst::get_ATTRIB_STARTING_ICAO (), "");
    if (navAid.is_lat_lon_valid ())
    { // we will use the current targetLat/long stored in elementStartLocationProperties
      navAid.setID (icao_s);
      Log::logMsgThread (fmt::format("[{}] will set start location based on pre-defined location provided in template.", __func__) );
    }
    else // try to use the "location_value_nm_s" property and fetch a point based on a list of points provided ad-hock
    {
      // v25.08.1 support for "location_properties" attribute that will replace "location_value"
      const std::string location_xy_random_value = Utils::readAttrib (x_briefer_and_start_location, mxconst::get_ATTRIB_LOCATION_PROPERTIES (), mxconst::get_ATTRIB_LOCATION_VALUE (),  "");
      if (location_xy_random_value.empty () || mxUtils::is_number (location_xy_random_value)) // SHOULD NOT BE EMPTY OR A NUMBER.
      {
        navAid.init ();
        navAid.err = fmt::format ("[{}:{}] Failed to find valid starting location, No Coordinates or string List of random latitude/longitude were provided, will abort template creation. Please fix the template or change the starting location to plane.", __func__, __LINE__);
        return navAid;
      }

      // read random element
      const IXMLNode xLocationNodePtr = in_xTemplate.getChildNode (location_xy_random_value.c_str ()).deepCopy ();
      if (xLocationNodePtr.isEmpty ())
      {
        navAid.init ();
        navAid.err = fmt::format ("[{}] Failed to read random element: <{}>. Please fix the template, aborting random creation.", __func__, location_xy_random_value);
        return navAid;
      }

      RandomEngine::shared_navaid_info.parentNode_ptr = xLocationNodePtr; // store pointer to XML node
      missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::convert_icao_to_xml_point); // will call missionx::flcPRE() and try to convert any <icao name="icao name" /> to <point targetLat="" targetLon="" />

      IXMLNode xPoint = Utils::xml_get_node_randomly_by_name_IXMLNode (xLocationNodePtr, mxconst::get_ELEMENT_POINT ());
      if (xPoint.isEmpty ())
      {
        navAid.init ();
        navAid.err = fmt::format ("[{}] Could not randomly find element <point> in {} node.", __func__, xLocationNodePtr.getName ());
        return navAid;
      }

      navAid.node = xPoint.deepCopy ();
      navAid.syncXmlPointToNav ();

      // try to get Navaid information for briefer. If we fail to find information, we ignore and continue with the original xPoint data
      // if (missionx::RandomEngine::filterAndPickRampBasedOnPlaneType (navAid, navAid.err, missionx::mxFilterRampType::start_ramp))

      auto search_ramp_result = RandomEngine::gen_get_ramp_based_on_plane_type (navAid, RandomEngine::getPlaneType_enum (), mxFilterRampType::start_ramp);
      if (search_ramp_result.result)
      {
        xPoint = navAid.node.deepCopy ();
        if (xPoint.isEmpty () || !navAid.err.empty () )
        {
          navAid.init ();
          navAid.err = fmt::format ("[{}] Fail to read filtered briefer starting point. Aborting... notify developer.", __func__);
          return navAid;
        }
      }
      RandomEngine::errMsg.clear ();

      const std::string elev_s = Utils::readAttrib (xPoint, mxconst::get_ATTRIB_ELEV_FT (), "");

      navAid.lat                    = Utils::readNodeNumericAttrib <float>(xPoint, mxconst::get_ATTRIB_LAT (), 0.0f);
      navAid.lon                    = Utils::readNodeNumericAttrib <float>(xPoint, mxconst::get_ATTRIB_LONG (), 0.0f);
      navAid.heading                = Utils::readNodeNumericAttrib <float>(xPoint, mxconst::get_ATTRIB_HEADING_PSI (), RandomEngine::planeLocation.heading);
      navAid.height_mt              = Utils::readNodeNumericAttrib <float>(xPoint, mxconst::get_ATTRIB_ELEV_FT (), 0.0f) * feet2meter;

      if (!navAid.is_lat_lon_valid ())
      {
        // RandomEngine::setError ("[random] Point data does not have mandatory attributes: '" + mxconst::get_ATTRIB_LAT () + "' and '" + mxconst::get_ATTRIB_LONG () + "'. Please fix template. Aborting...");
        navAid.init ();
        navAid.err = fmt::format ("[{}] Point data does not have mandatory attributes, check 'lat' and 'lon' attributes.", __func__);
        return navAid;
      }

      // set start location "target lat/long/elev_ft
      Utils::xml_search_and_set_attribute_in_IXMLNode (x_briefer_and_start_location, mxconst::get_ATTRIB_LAT (), navAid.getLat (), mxconst::get_ELEMENT_LOCATION_ADJUST ());
      Utils::xml_search_and_set_attribute_in_IXMLNode (x_briefer_and_start_location, mxconst::get_ATTRIB_LONG (), navAid.getLon (), mxconst::get_ELEMENT_LOCATION_ADJUST ());
      Utils::xml_search_and_set_attribute_in_IXMLNode (x_briefer_and_start_location, mxconst::get_ATTRIB_ELEV_FT (), elev_s, mxconst::get_ELEMENT_LOCATION_ADJUST ());
      Utils::xml_search_and_set_attribute_in_IXMLNode (x_briefer_and_start_location, mxconst::get_ATTRIB_HEADING_PSI (), navAid.getHeading_s (), mxconst::get_ELEMENT_LOCATION_ADJUST ());


      // end reading a random < point > element
      // end using <location_value_nm_s> an element to choose a starting location

    } // end if targetLat/long were defined or based on a location_value_nm_s element

  } // end construct <start_location> based on "xy" (pre-defined targetLat/long or based on ad-hock starting points that we will pick at random

  // Generates the briefer starting message. Should be stored in the "briefer" NavAid ([0])
  // use of shared_navaid_info.navAid to search if a plane is in an airport boundary.
  missionx::RandomEngine::shared_navaid_info.init ();
  missionx::RandomEngine::shared_navaid_info.navAid.lat = navAid.lat;
  missionx::RandomEngine::shared_navaid_info.navAid.lon = navAid.lon;

  missionx::RandomEngine::shared_navaid_info.navAid = data_manager::get_plane_airport_or_nearest_icao(true, RandomEngine::shared_navaid_info.navAid.lat, RandomEngine::shared_navaid_info.navAid.lon, true);
  if (!RandomEngine::shared_navaid_info.navAid.getID ().empty ())
  {
    shared_navaid_info.navAid.synchToPoint(true);
    navAid.fpln_msg_text = fmt::format ("You will fly from {}({}).", shared_navaid_info.navAid.getNavAidName (), shared_navaid_info.navAid.getID () );
  }


  return navAid; // later we will set the Briefer description and properties. This function handles the "base briefer" navaid
}

// -----------------------------------


missionx::NavAidInfo
RandomEngine::gen_briefer_phase_02_base_node_from_navaid (missionx::NavAidInfo &inout_start_navaid, RandomEngine::strct_shared_random_airport_info &inout_strct_shared_navaid_info, const bool in_flag_we_have_target_above_water)
{
  // We will use this function to construct the "<briefer>" element.
  // The description of the mission in the briefer we will fetch from: "<mission_info>" CDATA property.
  inout_start_navaid.synchToPoint ();

  // missionx::NavAidInfo na_briefer;
  ///////////////////////
  // <briefer>
  IXMLNode x_briefer_node = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_BRIEFER () );

  // <location_adjust>
  auto const lmbda_get_location_adjust_node =[&]()
  {
    if (inout_start_navaid.fpln_xml_osm_q_or_raw_tmpl_node.isEmpty ())
    {
      if (auto node = x_briefer_node.getChildNode (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ())
        ; !node.isEmpty ())
        return node;

      return IXMLNode::emptyIXMLNode;
    }

    // delete the <location_adjust> from the XSD
    Utils::xml_delete_all_subnodes (x_briefer_node, mxconst::get_ELEMENT_LOCATION_ADJUST (), true);
    // attach the stored <briefer_and_start_location> node
    inout_start_navaid.fpln_xml_osm_q_or_raw_tmpl_node = x_briefer_node.addChild (inout_start_navaid.fpln_xml_osm_q_or_raw_tmpl_node);

    return inout_start_navaid.fpln_xml_osm_q_or_raw_tmpl_node;
  };
  IXMLNode x_location_adjust_ptr = lmbda_get_location_adjust_node();

  if (x_location_adjust_ptr.isEmpty () + x_briefer_node.isEmpty ())
  {
    inout_start_navaid.err = fmt::format ("[{}] Fail to fetch internal \"{}\" element from Utils class. Notify developer !!!", __func__, mxconst::get_ELEMENT_BRIEFER_AND_START_LOCATION ());
    Log::logMsgThread (inout_start_navaid.err);
    return inout_start_navaid;
  }

  // update the <location_adjust> node name, to be on the safe side.
  x_location_adjust_ptr.updateName (mxconst::get_ELEMENT_LOCATION_ADJUST ().c_str ());

  const int nClear = x_location_adjust_ptr.nClear (); // remove any CDATA or COMMENTS or any clear() type element
  for (int i = 0; i < nClear; ++i)
    x_location_adjust_ptr.deleteClear ();

  const std::string locationOptionType = mxconst::get_ELEMENT_PLANE ();

  x_location_adjust_ptr.updateAttribute (inout_start_navaid.getLat ().c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
  x_location_adjust_ptr.updateAttribute (inout_start_navaid.getLon ().c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
  x_location_adjust_ptr.updateAttribute (Utils::formatNumber<double> (inout_start_navaid.p.getElevationInFeet (), 2).c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str ());
  x_location_adjust_ptr.updateAttribute (Utils::formatNumber<double> (inout_start_navaid.heading, 2).c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());

  bool b_fetch_navaid_info {false};
  inout_strct_shared_navaid_info.navAid.init ();
  if ( inout_start_navaid.fpln_navaid_was_already_prepared )
  {
    if (inout_start_navaid.getID ().empty ())
      b_fetch_navaid_info = true;
  }
  else
  {
    // search for the nearest ICAO or bounding airport relative to plane starting position using the SQLITE database
    inout_start_navaid = missionx::data_manager::get_plane_airport_or_nearest_icao (true, inout_start_navaid.lat, inout_start_navaid.lon, true);
    inout_start_navaid.synchToPoint ();
    b_fetch_navaid_info = true;
  }// end if !fpln_navaid_was_already_prepared

  // try to find the nearest airport if we are not inside a valid airport boundary.
  if (inout_start_navaid.getID ().empty () && b_fetch_navaid_info)
  {
    // initialize the starting "lat/lon" coordinates before calling the main thread.
    inout_strct_shared_navaid_info.navAid = inout_start_navaid;

    // fetch the ICAO the plane is in its boundary, if not then fetch the closest airport to plane location, hopefully it is in ~5nm range.
    inout_strct_shared_navaid_info.navAid = missionx::data_manager::get_plane_airport_or_nearest_icao (true, inout_start_navaid.lat, inout_start_navaid.lon, true);
    inout_strct_shared_navaid_info.navAid.synchToPoint ();
    if (!inout_strct_shared_navaid_info.navAid.getID ().empty ())
    {
      inout_start_navaid.setID (inout_strct_shared_navaid_info.navAid.getID ());
      inout_start_navaid.setName (inout_strct_shared_navaid_info.navAid.getNavAidName ());
    }
    else
    {
      // Try to find the closest location, but we should not use it as a starting_icao location.
      inout_strct_shared_navaid_info.init ();
      inout_strct_shared_navaid_info.navAid = inout_start_navaid;
      if (missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
      {
        // check distance and hopefully pick the correct airport. Since we are using a fixed distance, this might not be a 100% guaranty
        inout_strct_shared_navaid_info.navAid.synchToPoint ();
        const double dist = inout_strct_shared_navaid_info.navAid.p.calcDistanceBetween2Points (inout_start_navaid.p, mx_units_of_measure::nm );
        if ( dist <= 5.0 && !inout_strct_shared_navaid_info.navAid.getID ().empty ())
        {
          std::string navaid_name;
          if (inout_start_navaid.getName ().empty ())
            navaid_name = fmt::format ("Near {}.", inout_strct_shared_navaid_info.navAid.getNavAidName ());
          else
            navaid_name = fmt::format ("{}, near {}.", inout_start_navaid.getName (), inout_strct_shared_navaid_info.navAid.getNavAidName ());

          if (inout_start_navaid.getID ().empty () && !inout_strct_shared_navaid_info.navAid.getID ().empty ())
            inout_start_navaid.setID (inout_strct_shared_navaid_info.navAid.getID ());

          inout_start_navaid.setName (navaid_name);
          inout_start_navaid.height_mt = inout_strct_shared_navaid_info.navAid.height_mt;
          inout_start_navaid.navRef    = inout_strct_shared_navaid_info.navAid.navRef; // v25.05.1
        }
      } // end "mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread"

    } // end searching for the nearest navaid

  } // End starting location is not in an airport boundary.


  // v25.09.1 added missing "starting_icao" // v25.09.2 only if it was defined in the <briefer_and_start_location>
  const std::string briefer_and_start_location_starting_icao = Utils::readAttrib (inout_start_navaid.fpln_xml_osm_q_or_raw_tmpl_node, mxconst::get_ATTRIB_STARTING_ICAO (), "" );
  if (!briefer_and_start_location_starting_icao.empty ())
    x_briefer_node.updateAttribute (briefer_and_start_location_starting_icao.c_str (), mxconst::get_ATTRIB_STARTING_ICAO ().c_str (), mxconst::get_ATTRIB_STARTING_ICAO ().c_str ());

  // x_briefer_node.updateAttribute (inout_start_navaid.getID ().c_str (), mxconst::get_ATTRIB_STARTING_ICAO ().c_str (), mxconst::get_ATTRIB_STARTING_ICAO ().c_str ());

  inout_start_navaid.flag_is_brieferOrStartLocation = true;
  inout_start_navaid.fpln_xml_target_leg_node = x_briefer_node.deepCopy ();

  inout_start_navaid.synchToPoint (b_fetch_navaid_info);
  return inout_start_navaid;
}

// -----------------------------------

void
RandomEngine::gen_briefer_phase_03_add_desc (std::map<int, NavAidInfo> &inout_targets, const bool flag_has_wet_target)
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

  std::string briefer_desc = mxUtils::trim ( inout_targets[0].fpln_expected_location_data.desc, ""); // v25.09.2 use the description from the <briefer_and_start_location>

  // v25.10.1 check if the raw node has a <desc> element with description text. We use the same technique in "gen_leg_description()" function
  if (briefer_desc.empty ())
  {
    // get <desc> from the raw <q>
    auto x_desc_node = inout_targets[0].fpln_xml_osm_q_or_raw_tmpl_node.getChildNode (mxconst::get_ELEMENT_DESC ().c_str ());
    if (!x_desc_node.isEmpty ())
      briefer_desc = Utils::xml_get_text_or_cdata_text (x_desc_node, "");
  }

  // Create a generic description if no pre-defined text was defined in the template file.
  if ( mxUtils::trim ( briefer_desc ).empty ())
  {
    const auto lmbda_get_the_generic_briefer_desc_header = [&] () -> std::string {
      std::string desc_s;

      auto med_cargo_or_oilrig_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::undefined)); // 0 = med, 1 = cargo
      if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::medevac))
      {
        return fmt::format ("You have been assigned to a medevac mission. Your expected transportation is a {}.\n", "helo");
      }

      if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::cargo))
      {
        return fmt::format ("You have been assigned to a transportation mission.\n");
      }

      if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::oil_rig))
      {
        return fmt::format ("You have been assigned to an Oil-Rig mission.\n");
      }

      return "";
    };

    // Construct the generic briefer message
    briefer_desc= "Hello Pilot\n\n";
    briefer_desc += lmbda_get_the_generic_briefer_desc_header ();

    if (!inout_targets[0].getNavAidName ().empty ())
      briefer_desc +=  fmt::format ("You will fly from {}{}.", inout_targets[0].getNavAidName (),  mxUtils::eval_text (!inout_targets[0].getID ().empty (), "(" + inout_targets[0].getID () + ")", ""));
    else if (!inout_targets[0].getID ().empty ())
      briefer_desc +=  fmt::format ("You will fly from {}.", inout_targets[0].getID ());
    else
      briefer_desc +=  fmt::format ("You will fly to {}.", inout_targets[1].get_loc_desc ());
  } // end if no "custom description" was found.

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
RandomEngine::gen_add_inventory_phase01_node (const int &in_seq, missionx::NavAidInfo & inout_navaid, std::unordered_map<int, mx_inventory_track_strct> &inout_map_osm_inventory_track, const float &in_radius, const std::list<missionx::structs::strct_node_attribute_key_value> *in_override_attrib_list)
{
  // If we already have an inventory, skip this phase
  if (!inout_navaid.fpln_xml_inv_node.isEmpty ())
    return inout_navaid.fpln_xml_inv_node;

  // Define Inventory Name:
  const auto lmbda_get_inv_name=[&]()
  {
    if (mxUtils::trim (inout_navaid.getNavAidName () ).empty () )
      return fmt::format("inv_{}", in_seq);

    return mxUtils::trim(inout_navaid.getNavAidName ());
  };

  const std::string inv_name = lmbda_get_inv_name();

  // check if the inventory name exists
  bool flag_inventory_is_new = true;
  for (const auto &[indx, inv]: inout_map_osm_inventory_track)
  {
    if (inv.inventory_name == inv_name)
      return IXMLNode::emptyIXMLNode;
  }


  const auto lmbda_get_radius = [&]()
  {
    if (in_radius > 0.0)
      return std::to_string (in_radius);

    return std::string ( (inout_navaid.fpln_wp_template_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER())? mxconst::DEFAULT_LAND_OR_INV_RADIUS_MT.data () : mxconst::DEFAULT_HOVER_RADIUS_MT.data () );
  };
  const auto inv_radius = lmbda_get_radius();

  // prepare inventory attributes to modify
  const std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_inv = {
    { mxconst::get_ELEMENT_POINT (), mxconst::get_ATTRIB_LAT (), inout_navaid.getLat () },
    { mxconst::get_ELEMENT_POINT (), mxconst::get_ATTRIB_LONG (), inout_navaid.getLon () },
    { mxconst::get_ELEMENT_RADIUS (), mxconst::get_ATTRIB_LENGTH_MT (), inv_radius },
    // inhibit mx-pad toggle when entering the inventory area and we are airborne
    { mxconst::get_ELEMENT_INVENTORY (), mxconst::get_ATTRIB_INHIBIT_MXPAD_B (), "true" },
  };


  // Prepare the < inventory > node and clean it.
  IXMLNode xml_inv_node = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_INVENTORY ());
  Utils::xml_delete_all_subnodes (xml_inv_node, mxconst::get_ELEMENT_ITEM (), true);
  Utils::xml_delete_all_subnodes (xml_inv_node, mxconst::get_ELEMENT_STATION (), true);

  // check if <q> has <inventory> node. Copy all its items into our local inventory

  #ifndef RELEASE
  // Log::logMsgThread (fmt::format ("[{}] Osm Query content:\n{}\n<-- End OSM Query Content --\n", __func__, Utils::xml_get_node_content_as_text (inout_target_navaid.fpln_xml_osm_q_node) ) ); // DEBUG
  auto n_inv = inout_navaid.fpln_xml_osm_q_or_raw_tmpl_node.nChildNode (mxconst::get_ELEMENT_INVENTORY().c_str()); // DEBUG - remove
  #endif

  IXMLNode external_inv_node = inout_navaid.fpln_xml_osm_q_or_raw_tmpl_node.getChildNode (mxconst::get_ELEMENT_INVENTORY().c_str()).deepCopy ();
  if (!external_inv_node.isEmpty())
  {
    xml_inv_node = Inventory::copy_items_from_one_inventory_to_the_other_xp11_style (xml_inv_node.deepCopy (), external_inv_node);
  }

  // set name
  Utils::xml_set_attribute_in_node_asString (xml_inv_node, mxconst::get_ATTRIB_NAME (), inv_name, xml_inv_node.getName());
  // set all attributes based on "lsAttrib_inv"
  Utils::xml_search_and_set_attributes_in_node (xml_inv_node, lsAttrib_inv);
  if (in_override_attrib_list != nullptr)
    Utils::xml_search_and_set_attributes_in_node (xml_inv_node, (*in_override_attrib_list) );

  // add inventory track
  if (!mxUtils::isElementExists (inout_map_osm_inventory_track, in_seq) )
  {
    inout_map_osm_inventory_track[in_seq].fpln_seq       = inout_navaid.fpln_seq;
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
RandomEngine::gen_add_3d_marker_to_current_target (IXMLNode &inout_leg_node, missionx::NavAidInfo &in_target_navaid)
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
RandomEngine::gen_leg_start_messages (int &seq, NavAidInfo &inout_target_na, std::map<int, NavAidInfo> &navaid_targets, IXMLNode &inout_xml_messages, const bool &flag_one_of_targets_is_above_water_body)
{
  // add new messages to the "<leg>'s "start_message" sub-element.
  // The "inout_messages_node" is the main node that holds new created messages.
  if (inout_xml_messages.isEmpty ())
    return;

  inout_target_na.init_locDesc (); // force initializing the location description

  const auto lmbda_get_default_msg_text =[&]()
  {
    if ( inout_target_na.fpln_seq == 1) // first navaid, needs a longer description
    {
      std::string water_body_text;
      if (flag_one_of_targets_is_above_water_body)
        water_body_text = "\nOne of the locations is above water body.";

      if (inout_target_na.flag_is_skewed)
        return fmt::format ("Hello pilot. We have uploaded flight coordinates to your GPS.\n{}{}\nFly to: {}.\nYou will have to search the target around that location.", navaid_targets[0].fpln_msg_text, water_body_text, inout_target_na.get_skewed_desc ());

      return fmt::format ("Hello pilot. We have uploaded flight coordinates to your GPS.\n{}{}\nFly to: {}", navaid_targets[0].fpln_msg_text, water_body_text, inout_target_na.get_loc_desc ());
    }

    return fmt::format("Fly to: {}.", inout_target_na.get_loc_desc());
  };

  // const std::string default_msg_text = fmt::format("Fly to: {}.", inout_target_na.get_loc_desc());
  const std::string default_msg_text = lmbda_get_default_msg_text();
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
RandomEngine::gen_messages_when_reaching_target_leg (int &seq_trig, int &seq_msg, NavAidInfo &inout_target_na, IXMLNode &in_metadata_node, IXMLNode &inout_xml_messages, IXMLNode &inout_xml_triggers, const IXMLNode &in_xml_land_trigger, const IXMLNode &in_xml_hover_trigger)
{
  assert (!inout_xml_messages.isEmpty () && !in_xml_land_trigger.isEmpty () && fmt::format ("[{}] One of the key parameters is empty and not valid.", __func__).c_str () );

  const bool  flag_wp_type_is_land_hover = (inout_target_na.fpln_wp_template_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ());

  // ---------------------------------------
  // Prepare the landing and hover messages
  // ---------------------------------------
  // land message is used only in "land" cases
  const std::string land_msg_name = fmt::format ("msg_{}_leg_{}_target", seq_msg, inout_target_na.fpln_seq); // used only in Landing situations
  const std::string hover_msg_name = fmt::format ("msg_{}_leg_{}_enter_target_hover_area", seq_msg, inout_target_na.fpln_seq);
  // physical area message used only in "land_hover" cases
  const std::string land_msg_when_in_physical_area_name = (flag_wp_type_is_land_hover)? fmt::format ("leg_{}_entered_landing_phys_area_msg_{}", inout_target_na.fpln_seq, seq_msg) : "";
  seq_msg++;

  const int template_mission_type = Utils::readNodeNumericAttrib<int> (in_metadata_node, mxconst::get_ATTRIB_MISSION_TYPE (), 0);

  const auto lmbda_get_land_in_target_text =[flag_wp_type_is_land_hover = flag_wp_type_is_land_hover, template_mission_type = template_mission_type] (missionx::NavAidInfo &in_na)
  {
    if (in_na.fpln_task_type == enums::mx_rnd_task_type::medevac)
    {
      switch (static_cast<int>( in_na.fpln_mission_type) )
      {
        case static_cast<int>(missionx::enums::mx_user_picked_mission_type::oilrig_medevac):
        {
          constexpr auto land_med_target_oilrig     = "Wait for the Oil Rig team to move the patient in or out of the plane.";
          constexpr auto land_med_extraction_oilrig = "You reached {1}, wait for the patient to be taken out of the plane.";
          if (in_na.fpln_mission_phase == enums::mx_rnd_mission_phase::land_target )
            return std::string(land_med_target_oilrig);

          std::map<int, std::string> data = {{1, in_na.get_loc_desc ()}};
          return std::string( mxUtils::format (land_med_extraction_oilrig, data) );
        }
        break;
        default: // all the rest
        {
          constexpr auto land_wp_land_message       = "Remain on standby until the medical team has transferred the patient."; //"Wait for the medical team to bring the patient into the helicopter.";
          constexpr auto land_hover_wp_land_message = "Wait for the medical team to hoist the patient and load them into the helicopter.";

          if (in_na.fpln_mission_phase == enums::mx_rnd_mission_phase::land_extraction )
          {
            // if (in_na.nav_aid_has_unique_name ())
            return  (flag_wp_type_is_land_hover)? fmt::format ("You landed in the '{}' area. {}", in_na.get_loc_desc (), land_hover_wp_land_message ) :
                                                  fmt::format ("You landed at '{}'. {}", in_na.get_loc_desc (), land_wp_land_message );
          }

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
          constexpr auto land_cargo_target_oilrig     = "Wait for the Oil Rig cargo to be moved in or out of the plane.";
          constexpr auto land_cargo_extraction_oilrig = "You reached {1}. Turn off the plane.";

          if (in_na.fpln_mission_phase == enums::mx_rnd_mission_phase::land_target )
            return std::string(land_cargo_target_oilrig);

          std::map<int, std::string> data = {{1, in_na.get_loc_desc ()}};
          return std::string( mxUtils::format (land_cargo_extraction_oilrig, data) );
        }
        break;
        default: // all the rest
        {

          const auto land_cargo_target = (template_mission_type == static_cast<int> (enums::mx_rnd_task_type::passenger)) ? "Wait for the passengers to move in or out of the plane." : "Move the cargo in or out of the plane.";

          if (in_na.fpln_mission_phase == enums::mx_rnd_mission_phase::land_target )
              return std::string(land_cargo_target);

          const auto land_cargo_extraction = (template_mission_type == static_cast<int> (enums::mx_rnd_task_type::passenger))? "You reached {1}. Wait until all passengers left, then shut down the aircraft." :  "You reached {1}. Wait until all passengers and cargo have been unloaded, then shut down the aircraft.";
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
  if (!inout_target_na.flag_is_skewed)
    inout_xml_triggers.addChild (trig_hover_msg_node);

  // ----------------------------------------------
  // Add the nodes to the global root nodes
  // ----------------------------------------------

  // add the messages to the <messages_template> root node
  inout_xml_messages.addChild (msg_land_node); // deprecated, only in physical area message is relevant

  if (flag_wp_type_is_land_hover && !inout_target_na.flag_is_skewed)
  {
    inout_xml_messages.addChild (msg_land_entered_physical_area_node);
    inout_xml_messages.addChild (msg_hover_node);
  }


  // add triggers to <leg>
  IXMLNode link_trigger = inout_target_na.fpln_xml_target_leg_node.addChild (mxconst::get_ELEMENT_LINK_TO_TRIGGER ().c_str ());
  Utils::xml_set_attribute_in_node_asString (link_trigger, mxconst::get_ATTRIB_NAME (), trig_land_name, link_trigger.getName ());
  if (flag_wp_type_is_land_hover && !inout_target_na.flag_is_skewed)
  {
    link_trigger = inout_target_na.fpln_xml_target_leg_node.addChild (mxconst::get_ELEMENT_LINK_TO_TRIGGER ().c_str ());
    Utils::xml_set_attribute_in_node_asString (link_trigger, mxconst::get_ATTRIB_NAME (), trig_hover_name, link_trigger.getName ());
  }

}

// -----------------------------------

void
RandomEngine::gen_2nm_to_N_nm_message (int &seq_trig, int &seq_msg, NavAidInfo &inout_target_na, IXMLNode &inout_xml_messages, IXMLNode &inout_xml_triggers, const IXMLNode &in_xml_land_trigger)
{
  assert (!inout_xml_messages.isEmpty () && !in_xml_land_trigger.isEmpty () && fmt::format ("[{}] One of the key parameters is empty and not valid.", __func__).c_str () );
  const bool  flag_wp_type_is_land_hover = (inout_target_na.fpln_wp_template_type == mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ());

  const std::string message_name = fmt::format ("msg_{}_leg_{}_near_target_2nm_to_Nnm{}", seq_msg, inout_target_na.fpln_seq, ((inout_target_na.flag_is_skewed)? "_skewed" : ""));
  seq_msg++;

  const auto lmbda_get_message_text =[&]()
  {
    if (inout_target_na.flag_is_skewed)
      return fmt::format ("You are nearing the search area. [{}]. The target should be somewhere around the suggested location.", inout_target_na.get_skewed_desc ());

    // if we have a unique target location description, we should use it, or else, we will use a generic message.
    const std::string target_description = (inout_target_na.nav_aid_has_unique_name ())? fmt::format ("You are nearing {}", inout_target_na.get_loc_desc () )
                                                                                            : "You are nearing the target location." ;

    return fmt::format ("{}\t{}", target_description,
                        (flag_wp_type_is_land_hover) ? "Look for a landing spot near the target. Alternatively, you may hover above it." : "Prepare for landing." );

  };
  const std::string message_text = lmbda_get_message_text(); // v25.09.2 return regular or skewed related text

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

  // create <message> based on a template for "land" hover" and "entering physical area"
  IXMLNode msg_2m_to_Xnm_land_node = msg_template_node.deepCopy ();
  Utils::xml_set_attribute_in_node_asString (msg_2m_to_Xnm_land_node, mxconst::get_ATTRIB_NAME (), message_name, msg_2m_to_Xnm_land_node.getName ());

  IXMLNode land_mix_text_node = Utils::xml_get_or_create_node_ptr (msg_2m_to_Xnm_land_node, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT ());
  Utils::xml_add_cdata (land_mix_text_node, message_text);

  // -------------------------------------------------
  // create the trigger that will fire the messages
  // -------------------------------------------------

  int rw_count = 0;
  float longest_rw_f = 0.0f;
  const auto data_found = RandomEngine::gen_get_rw_metadata(inout_target_na.getID (), rw_count, longest_rw_f);
  const auto lmbda_get_trigger_radius_based_on_rw_data_in_mt = [&]()
  {
    const mx_plane_types_enum plane_type = RandomEngine::getPlaneType_enum ();
    if (plane_type == mx_plane_types_enum::plane_type_helos || !data_found || static_cast<float>(rw_count) * longest_rw_f == 0.0f)
      return 2.0f * nm2meter;

    // if longest runway is shorter than 1500 meters
    if (longest_rw_f < 1501.0f && rw_count < 3 && plane_type < mx_plane_types_enum::plane_type_jets)
      return 2.0f * nm2meter;

    return 6.0f * nm2meter;
  };

  const auto distance_in_meters = lmbda_get_trigger_radius_based_on_rw_data_in_mt();

  // const std::string trigger_name  = fmt::format ("trig_{}_leg_{}_near_target_2m", seq_trig, inout_target_na.fpln_seq );
  const std::string trigger_name  = fmt::format ("trig_{}_{}", seq_trig, message_name ); // v25.09.2 the trig name will reflect a skewed position
  // const std::string radius_mt_2nm = fmt::format("{}", 2.0f * missionx::nm2meter);
  const std::string radius_in_nm = fmt::format("{}", distance_in_meters);

  // Prepare landing trigger attributes
  std::list<missionx::structs::strct_node_attribute_key_value> lsAttrib_trig_landing_area = {
    { mxconst::get_ELEMENT_TRIGGER (), mxconst::get_ATTRIB_NAME (), trigger_name },
    { mxconst::get_ELEMENT_TRIGGER (), mxconst::get_ATTRIB_RE_ARM (), "true" },
    { mxconst::get_ELEMENT_RADIUS (), mxconst::get_ATTRIB_LENGTH_MT (), radius_in_nm },
    { mxconst::get_ELEMENT_CONDITIONS (), mxconst::get_ATTRIB_PLANE_ON_GROUND (), "" },
    { mxconst::get_ELEMENT_OUTCOME (), mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED (), message_name },
  };

  // v25.09.2 add Skewed trigger positioning
  if (inout_target_na.flag_is_skewed)
  {
    lsAttrib_trig_landing_area.push_back ({ mxconst::get_ELEMENT_POINT (), mxconst::get_ATTRIB_LAT (), fmt::format("{}", inout_target_na.skewed_location.lat) });
    lsAttrib_trig_landing_area.push_back ({ mxconst::get_ELEMENT_POINT (), mxconst::get_ATTRIB_LONG (), fmt::format("{}", inout_target_na.skewed_location.lon) });
  }

  IXMLNode trig_2nm_to_Nth_nm_land_msg_node  = in_xml_land_trigger.deepCopy ();
  // clear land <outcome> node
  IXMLNode xml_outcome_node = trig_2nm_to_Nth_nm_land_msg_node.getChildNode (mxconst::get_ELEMENT_OUTCOME ().c_str ());
  Utils::xml_delete_all_node_attributes (xml_outcome_node);

  // set attributes
  Utils::xml_search_and_set_attributes_in_node (trig_2nm_to_Nth_nm_land_msg_node, lsAttrib_trig_landing_area);
  seq_trig++;

  // add triggers to <triggers> root node
  inout_xml_triggers.addChild (trig_2nm_to_Nth_nm_land_msg_node);

  // add the messages to the <messages_template> root node
  inout_xml_messages.addChild (msg_2m_to_Xnm_land_node);

  // add trigger to <leg>
  IXMLNode link_trigger_ptr = inout_target_na.fpln_xml_target_leg_node.addChild (mxconst::get_ELEMENT_LINK_TO_TRIGGER ().c_str ());
  Utils::xml_set_attribute_in_node_asString (link_trigger_ptr, mxconst::get_ATTRIB_NAME (), trigger_name, link_trigger_ptr.getName ());

}

// -----------------------------------

void
RandomEngine::gen_parse_and_add_all_display_objects_in_node (const std::string &in_which_func_called, missionx::NavAidInfo &in_target_navaid, const IXMLNode &in_source_node, IXMLNode &inout_target_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d)
{
  const int nDisplayObjects = in_source_node.nChildNode ();
  for (int i1 = 0; i1 < nDisplayObjects; ++i1)
  {
    // get sub-node
    auto x_display_node = in_source_node.getChildNode (i1).deepCopy ();
    if (x_display_node.isEmpty ())
      continue;

    // filter out sub-nodes that are not <display_xxx> elements
    std::string tag = x_display_node.getName ();
    if (tag != mxconst::get_ELEMENT_DISPLAY_OBJECT () && tag != mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ())
      continue; // skip elements that are not <display_object> not <display_object_near_plane

    #ifndef RELEASE
    Log::logMsgThread (fmt::format ("[{}]Adding 3D display_objects from: {}:{}", in_which_func_called, tag, Utils::readAttrib (x_display_node, mxconst::get_ATTRIB_NAME (), "") ) );
    #endif

    // if (std::string err
    //   ; RandomEngine::parse_display_object_element (in_source_node, cNode, in_template_node, inout_x3DObjTemplate, in_expected_slope_at_target_location_d, err)) // v25.06.1 extended function signature // v3.0.219.1 handle <display_object> options like: optional, random_water or limit_to_terrain_slope
    if (std::string err
      ; RandomEngine::parse_display_object_element (&in_target_navaid, inout_target_node, x_display_node, in_template_node, inout_x3DObjTemplate, in_expected_slope_at_target_location_d, err)) // v25.06.1 extended function signature // v3.0.219.1 handle <display_object> options like: optional, random_water or limit_to_terrain_slope
    {
      if (tag == mxconst::get_ELEMENT_DISPLAY_OBJECT_NEAR_PLANE ())
      {
        //  Force replace_lat or replace_long with fake values to be on the safe side
        x_display_node.updateAttribute ("1.0", mxconst::get_ATTRIB_REPLACE_LAT ().c_str (), mxconst::get_ATTRIB_REPLACE_LAT ().c_str ());
        x_display_node.updateAttribute ("1.0", mxconst::get_ATTRIB_REPLACE_LONG ().c_str (), mxconst::get_ATTRIB_REPLACE_LONG ().c_str ());
      }

      inout_target_node.addChild (x_display_node.deepCopy (), inout_target_node.nChildNode ());
    }
  }

}

// -----------------------------------


void
RandomEngine::gen_3d_hint_objects_for_land_and_hover (const NavAidInfo &inout_target_na, IXMLNode &inout_leg_node, const NavAidInfo *next_navaid_ptr)
{
  // skip, if it is the last leg
  if (inout_target_na.fpln_is_last_flight_leg)
    return;

  // generate 3D hint for landing

  // store stats as int
  const int LANDING_RADIUS_FOR_LAND_HOVER_MT = mxUtils::stringToNumber<int> (mxconst::DEFAULT_LAND_OR_INV_RADIUS_MT.data ());
  const int LANDING_RADIUS_FOR_LAND_ONLY_MT  = mxUtils::stringToNumber<int> (mxconst::DEFAULT_LAND_ONLY_RADIUS_MT.data ());
  const int HOVER_RADIUS_MT                  = mxUtils::stringToNumber<int> (mxconst::DEFAULT_HOVER_RADIUS_MT.data ());

  const std::string NEXT_LEG_NAME = (next_navaid_ptr == nullptr)? "" : next_navaid_ptr->fpln_leg_name;

  int seq = 0;

  // calculate LANDING HINT <display_object>, using 350 meters with 24 3D objects.
  const int landing_radius_mt = (mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER () == inout_target_na.fpln_wp_template_type) ? LANDING_RADIUS_FOR_LAND_HOVER_MT : LANDING_RADIUS_FOR_LAND_ONLY_MT;
  const int how_many_3d_objects_to_display  = (mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER () == inout_target_na.fpln_wp_template_type) ? 24 : 4;

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
  if (mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER () == inout_target_na.fpln_wp_template_type )
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
RandomEngine::gen_add_3d_objects_for_surprise_me_base_on_predefined_attributes (NavAidInfo &inout_target_na, IXMLNode &inout_leg_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d)
{
  enum class enum_set_3d_source: uint8_t
  {
    none = 0,
    header = 1,
    q_node = 2,
    desc = 3,
    display_object_set = 4, // compatible with a regular template
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
  // read 3D set related attributes from osm target <q> node
  map_3d_set_attributes[enum_set_3d_source::q_node].random_tag     = Utils::readAttrib (inout_target_na.fpln_xml_osm_q_or_raw_tmpl_node , mxconst::get_ATTRIB_RANDOM_TAG (), "");
  map_3d_set_attributes[enum_set_3d_source::q_node].set_name       = Utils::readAttrib (inout_target_na.fpln_xml_osm_q_or_raw_tmpl_node, mxconst::get_ATTRIB_SET_NAME (), "");
  map_3d_set_attributes[enum_set_3d_source::q_node].slope_set_name = Utils::readAttrib (inout_target_na.fpln_xml_osm_q_or_raw_tmpl_node, mxconst::get_ATTRIB_SLOPE_SET_NAME (), "");
  // read the same from the <desc> sub-element of <leg>.
  map_3d_set_attributes[enum_set_3d_source::desc].random_tag     = Utils::readAttrib (xml_desc_ptr, mxconst::get_ATTRIB_RANDOM_TAG (), "");
  map_3d_set_attributes[enum_set_3d_source::desc].set_name       = Utils::readAttrib (xml_desc_ptr, mxconst::get_ATTRIB_SET_NAME (), "");
  map_3d_set_attributes[enum_set_3d_source::desc].slope_set_name = Utils::readAttrib (xml_desc_ptr, mxconst::get_ATTRIB_SLOPE_SET_NAME (), "");
  // Backwards compatibility, read sub-element <display_object_set> from the <leg>
  map_3d_set_attributes[enum_set_3d_source::display_object_set].random_tag     = Utils::readAttrib (inout_leg_node, mxconst::get_ATTRIB_RANDOM_TAG (), "");
  map_3d_set_attributes[enum_set_3d_source::display_object_set].set_name       = Utils::readAttrib (inout_leg_node, mxconst::get_ATTRIB_SET_NAME (), "");
  map_3d_set_attributes[enum_set_3d_source::display_object_set].slope_set_name = Utils::readAttrib (inout_leg_node, mxconst::get_ATTRIB_SLOPE_SET_NAME (), "");

  const auto lmbda_which_3d_set_to_pick_from =[&] ()
  {
    // v25.09.1 backwards compatibility with <display_object_set> nodes.
    if (!map_3d_set_attributes[enum_set_3d_source::display_object_set].random_tag.empty ())
      return enum_set_3d_source::display_object_set;

    if (!map_3d_set_attributes[enum_set_3d_source::desc].random_tag.empty () && !map_3d_set_attributes[enum_set_3d_source::desc].set_name.empty ())
      return enum_set_3d_source::desc;

    // v25.12.1
    if (!map_3d_set_attributes[enum_set_3d_source::q_node].random_tag.empty () && !map_3d_set_attributes[enum_set_3d_source::q_node].set_name.empty ())
      return enum_set_3d_source::q_node;

    if (!map_3d_set_attributes[enum_set_3d_source::header].random_tag.empty () && !map_3d_set_attributes[enum_set_3d_source::header].set_name.empty ())
      return enum_set_3d_source::header;

    // fallback
    return enum_set_3d_source::none;
  };

  const enum_set_3d_source picked_3d_set_source = lmbda_which_3d_set_to_pick_from ();

  if (picked_3d_set_source != enum_set_3d_source::none)
  {
    // random pick one of the values in each attribute "random_tag", "set_name" and "slope_set_name"
    // random pick "random_tag"
    const std::string random_tag_node_name = Utils::get_shuffled_value_from_string_value (map_3d_set_attributes[picked_3d_set_source].random_tag);

    // random pick "set_name"
    std::string set_name_node_to_pick = Utils::get_shuffled_value_from_string_value (map_3d_set_attributes[picked_3d_set_source].set_name);

    // random pick "slope_set_name"
    const std::string slope_set_node_to_pick = Utils::get_shuffled_value_from_string_value (map_3d_set_attributes[picked_3d_set_source].slope_set_name);

    // check slope and use a slope set if it has value.
    if (inout_target_na.fpln_slope > (missionx::data_manager::Max_Slope_To_Land_On * 3.0f) && !slope_set_node_to_pick.empty () )
      set_name_node_to_pick = slope_set_node_to_pick; //map_3d_set_attributes[picked_3d_set_source].slope_set_name;

    #ifndef RELEASE
    Log::logMsgThread ( fmt::format ("[{}] Search 3D set_name: {}", __func__, set_name_node_to_pick) );
    #endif


    //////////////////////////////////////////
    // ADD DISPLAY_OBJECT
    // Find the correct "set"
    // Add all <display_object> elements
    ///////////////////////////////////////

    if (const IXMLNode xTag = in_template_node.getChildNode (random_tag_node_name.c_str ())
      ; !xTag.isEmpty ())
    {
      int nSubNodes = 0;
      // check child tag
      if (set_name_node_to_pick.empty ())
        nSubNodes = xTag.nChildNode ();
      else
        nSubNodes = xTag.nChildNode (set_name_node_to_pick.c_str ());

      // Pick a <dub-node>
      if (nSubNodes > 0)
      {
        IXMLNode  cTagNode;
        const int randomChild_i = Utils::getRandomIntNumber (0, nSubNodes - 1);
        if (set_name_node_to_pick.empty ())
          cTagNode = xTag.getChildNode (randomChild_i);
        else
          cTagNode = xTag.getChildNode (set_name_node_to_pick.c_str (), randomChild_i);

        if (!cTagNode.isEmpty ())
        {
          Utils::xml_add_comment ( inout_leg_node, " >>> Display Objects <<< ");
          RandomEngine::gen_parse_and_add_all_display_objects_in_node (__func__, inout_target_na, cTagNode, inout_leg_node, in_template_node, inout_x3DObjTemplate, in_expected_slope_at_target_location_d);

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
RandomEngine::gen_3d_add_display_object_sets_instances_to_leg (NavAidInfo &inout_target_na, IXMLNode &inout_leg_node, IXMLNode &in_template_node, IXMLNode &inout_x3DObjTemplate, double &in_expected_slope_at_target_location_d)
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
          RandomEngine::gen_parse_and_add_all_display_objects_in_node (__func__, inout_target_na, cTagNode, inout_leg_node, in_template_node, inout_x3DObjTemplate, in_expected_slope_at_target_location_d);

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
RandomEngine::gen_3d_parse_instances_in_leg (IXMLNode &legNode_ptr, missionx::NavAidInfo &in_target_navaid)
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

    std::string instName = obj3d_name + "_" + Utils::readAttrib (legNode_ptr, mxconst::get_ATTRIB_NAME (), "") + "_" + Utils::formatNumber<int> (i1);
    Utils::xml_set_attribute_in_node_asString (xNode, mxconst::get_ATTRIB_INSTANCE_NAME (), instName, xNode.getName ());

    // special validation and initialization of <display_object> element only
    if (tagName == mxconst::get_ELEMENT_DISPLAY_OBJECT ())
    {
      std::string replaceLat = Utils::readAttrib(xNode, mxconst::get_ATTRIB_REPLACE_LAT(), "");
      std::string replaceLon = Utils::readAttrib(xNode, mxconst::get_ATTRIB_REPLACE_LONG(), "");
      std::string replaceElev_ft = Utils::readAttrib(xNode, mxconst::get_ATTRIB_REPLACE_ELEV_FT(), "");
      const std::string replacePitch = Utils::readAttrib(xNode, mxconst::get_ATTRIB_REPLACE_PITCH(), "");
      const std::string replaceRoll = Utils::readAttrib(xNode, mxconst::get_ATTRIB_REPLACE_ROLE(), "");

      int         replaceElevAboveGround_ft_i = Utils::readNodeNumericAttrib<int> (xNode, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT (), 0);


      // v3.0.219.1 calculate 3D object location relative to the target
      std::string relative_pos_bearing_deg_distance_mt = Utils::readAttrib (xNode, mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT (), "");

      // if (const std::vector<int> vecRelativePos = Utils::splitStringToNumbers<int> (relative_pos_bearing_deg_distance_mt, mxconst::get_PIPE_DELIMITER ())

      // v25.12.1 we read the "relative_pos_bearing_deg_distance_mt" attribute as a complex string of "two" variables that we will convert to numbers.
      if (const auto relative_pos_list = Utils::splitStringToList (relative_pos_bearing_deg_distance_mt, mxconst::get_PIPE_DELIMITER ())
        ; relative_pos_list.size () > 1)
      {
        double newLat, newLon, trigLat, trigLon, newBearing;
        newLat = newLon = trigLat = trigLon = newBearing = 0.0;

        if (in_target_navaid.lat * in_target_navaid.lon != 0.0)
        {
          // v25.12.1 add {vec} support
          // bearing calculation
          auto degrees_s = mxUtils::replaceAll(relative_pos_list.front(), "{vec}", fmt::format("{}", in_target_navaid.fpln_target_node_estimate_vector));
          auto calc_expression = calc(degrees_s);
          auto calced_bearing_exp = calc_expression.calculateExpression();
          auto distance_in_meters = mxUtils::stringToNumber<double> (relative_pos_list.back(), 4);
          // end v25.12.1

          // calculate new targetLat/long
          auto distance_nm = distance_in_meters * meter2nm;
          auto bearing     = static_cast<float> (calced_bearing_exp);
          Utils::calcPointBasedOnDistanceAndBearing_2DPlane (newLat, newLon, in_target_navaid.lat, in_target_navaid.lon, bearing, distance_nm);

          // set the new target Lat/long in instance replace point data
          Utils::xml_set_attribute_in_node <double>(xNode, mxconst::get_ATTRIB_REPLACE_LAT (), newLat, xNode.getName ());
          Utils::xml_set_attribute_in_node <double>(xNode, mxconst::get_ATTRIB_REPLACE_LONG (), newLon, xNode.getName ());
        }

        // v25.06.1
        xNode.updateAttribute (relative_pos_bearing_deg_distance_mt.c_str (), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS ().c_str (), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS ().c_str ()); // Keep the value in a debug attribute
        const std::set<std::string> set_attrib_to_del = {mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT ()};
        Utils::xml_delete_attribute (xNode, set_attrib_to_del, xNode.getName ());

        // set default above ground only if "replace_elev_ft" does not exist and the attribute "replace_elev_above_ground_ft" exists
        if (replaceElev_ft.empty () && replaceElevAboveGround_ft_i != 0)
          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT (), fmt::format("{}", replaceElevAboveGround_ft_i), mxconst::get_ELEMENT_DISPLAY_OBJECT ()); //

        // end calculating relative location to target of 3D object
      }
      else if ( !relative_pos_bearing_deg_distance_mt.empty () || relative_pos_list.size () == 1 ) // [regression bug fix] reset relative value so the plugin won't re-calculate it again when parsing the instance node.
      {
        xNode.updateAttribute (relative_pos_bearing_deg_distance_mt.c_str (), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS ().c_str (), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS ().c_str ()); // Keep the value in a debug attribute
        const std::set<std::string> set_attrib_to_del = {mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT ()};
        Utils::xml_delete_attribute (xNode, set_attrib_to_del, xNode.getName ());
      }
      else if (!obj3d_name.empty ()) // if we have no relative location information, then place at the target position
      {
        // define replace_lat/replace_long WITH TARGET POSITION (LAT/LON) if one of them is not set
        if (replaceLat.empty () || replaceLon.empty ())
        {
          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LAT (), in_target_navaid.getLat (), mxconst::get_ELEMENT_DISPLAY_OBJECT ());
          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_LONG (), in_target_navaid.getLon (), mxconst::get_ELEMENT_DISPLAY_OBJECT ());
        }

        // set default above ground only if "replace_elev_ft" does not exist and the attribute "replace_elev_above_ground_ft" exists
        if (replaceElev_ft.empty () && replaceElevAboveGround_ft_i != 0)
          Utils::xml_search_and_set_attribute_in_IXMLNode (xNode, mxconst::get_ATTRIB_REPLACE_ELEV_ABOVE_GROUND_FT (), fmt::format("{}", replaceElevAboveGround_ft_i), mxconst::get_ELEMENT_DISPLAY_OBJECT ()); //
      } // end bearing calculation with or without {vec}

      // v25.12.1 Calculate object 3D heading
      std::string replace_heading_psi = Utils::readAttrib (xNode, mxconst::get_ATTRIB_REPLACE_HEADING_PSI (), "");
      if (!mxUtils::trim(replace_heading_psi).empty() && mxUtils::is_number(replace_heading_psi) )
      {
        const auto heading_psi_expression_s = mxUtils::replaceAll(replace_heading_psi, "{vec}", fmt::format("{}", in_target_navaid.fpln_target_node_estimate_vector));
        auto calc_heading_expression = calc(heading_psi_expression_s);
        auto heading_psi_d = (calc_heading_expression.calculateExpression());

        Utils::xml_set_attribute_in_node <double>(xNode, mxconst::get_ATTRIB_REPLACE_HEADING_PSI (), heading_psi_d, xNode.getName ());
      }

      // add any custom pitch/role. TODO: consider using copy attributes except
      Utils::xml_set_attribute_in_node_asString(xNode, mxconst::get_ATTRIB_REPLACE_PITCH(), replacePitch, xNode.getName ());
      Utils::xml_set_attribute_in_node_asString(xNode, mxconst::get_ATTRIB_REPLACE_ROLE(), replaceRoll, xNode.getName ());
      // end v25.12.1

      // Skew location: place target instances not in their exact locations based on the SETUP screen.
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




    } // end if the tag is DISPLAY_OBJECT
  } // end xNode valid

  return true;
}

// -----------------------------------


std::map<int, missionx::NavAidInfo>
RandomEngine::gen_oilrig_targets (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_mapping_root_node, IXMLNode &inout_metadata_node, strct_shared_random_airport_info &inout_shared_navaid, std::string &outErr)
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


  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
  {
    outErr = ("[" + std::string (__func__) + "] Start Navaid: " + inout_shared_navaid.navAid.getID () + " Failed to find Airport using query navaid. Notify developer.");
    return target_navaids;
  }
  inout_shared_navaid.navAid.synchToPoint ();
  target_navaids[0] = NavAidInfo (inout_shared_navaid.navAid); // Store the briefer starting location
  // std::string err;
  // RandomEngine::filterAndPickRampBasedOnPlaneType (target_navaids[0], err, mxFilterRampType::start_ramp);
  auto search_ramp_result = RandomEngine::gen_get_ramp_based_on_plane_type (target_navaids[0], RandomEngine::getPlaneType_enum (), mxFilterRampType::start_ramp);
  target_navaids[0].fpln_navaid_was_already_prepared = true;

  inout_shared_navaid.navAid.init ();
  inout_shared_navaid.navAid.setID (row_oilrig_and_start_location[q0_columns[2]]); // Oil Rig ICAO
  inout_shared_navaid.navAid.lat = mxUtils::stringToNumber<float> (row_oilrig_and_start_location[q0_columns[4]], 8); // Oil Rig Lat
  inout_shared_navaid.navAid.lon = mxUtils::stringToNumber<float> (row_oilrig_and_start_location[q0_columns[5]], 8); // Oil Rig Lon
  if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
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
    data.mapLocationSplitPropertiesValues.clear ();
    data.vecLocationPropertiesSplit_vec.clear ();
  }
  ////////// Check if has special instructions like: "nm=20|ramp=H|nm_between=10-20|tag={some name}"
  else if (!data.location_properties_s.empty ())
  {
    //// v3.0.221.7 replace old logic with new more readable one
    // split between numbers and characters
    data.vecLocationPropertiesSplit_vec = mxUtils::split_v2 (data.location_properties_s, mxconst::get_PIPE_DELIMITER ()); // "|"

    for (const auto &v : data.vecLocationPropertiesSplit_vec)
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
        Utils::addElementToMap (data.mapLocationSplitPropertiesValues, attribName, attribValue);
      }
      else
        data.location_properties_s.clear ();
    } // end loop over split location_properties

    data.location_properties_s.clear ();

    // prepare local variables according to the split information
    const std::string local_location_value_min_max_distance_s = mxUtils::getValueFromElement (data.mapLocationSplitPropertiesValues, std::string ("nm_between"), std::string (""));
    if (!local_location_value_min_max_distance_s.empty ()) // min-max
    {
      const std::vector<double> vecMinMax = Utils::splitStringToNumbers<double> (local_location_value_min_max_distance_s, "-, ");
      for (size_t i1 = 0; i1 < vecMinMax.size (); ++i1)
      {
        switch (i1)
        {
          case 0:
            data.nm_between_min = static_cast<float>(vecMinMax.at(i1));
            data.mapLocationSplitPropertiesValues["min_distance_nm"] = fmt::format("{:.2f}", vecMinMax.at(i1) );
            break;
          case 1:
            data.nm_between_max = static_cast<float>(vecMinMax.at(i1));
            data.mapLocationSplitPropertiesValues["max_distance_nm"] = fmt::format("{:.2f}", vecMinMax.at(i1) );
            break;
          default:
            break;
        } // end switch
      }

      // Validate and fix if min > max
      // if (data.nm_between_min >=0 && data.nm_between_max >=0 )
      if (data.nm_between_min >=0 && data.nm_between_max >=0 )
      {
        mxUtils::mx_eval_min_max (data.nm_between_min, data.nm_between_max);
        // if (data.nm_between_min > data.nm_between_max)
        //   std::swap(data.nm_between_min, data.nm_between_max);
      }
    } // end "nm_between"

    // prepare local variables according to the split information
    if (Utils::isElementExists (data.mapLocationSplitPropertiesValues, "nm")) // represent distance in nm
      data.location_properties_s = data.mapLocationSplitPropertiesValues["nm"];

    // replace "_" with empty string
    if (data.location_properties_s == "_") // if special character that represents empty
      data.location_properties_s.clear ();

  }

  Log::logDebugBO ("[DEBUG pick template <leg> type] type picked: " + data.location_type, true);
  Log::logDebugBO ("[DEBUG random location info] location_value_nm_s=" + data.location_properties_s, true);

  return data;
}


// -----------------------------------


std::vector<int>
RandomEngine::gen_shuffled_q_from_osm_subject_node (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_root_node, const std::vector<missionx::structs::strct_osm_query> &vec_osm_queries, IXMLNode &out_main_subject_node, missionx::structs::strct_osm_query &out_analyzed_query)
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
RandomEngine::gen_get_targets_using_osm_queries_from_a_thread (missionx::base_thread::strct_thread_state *inoutThreadState, const IXMLNode &in_root_node, missionx::structs::strct_osm_query &inout_osm_query, strct_shared_random_airport_info &inout_shared_navaid)
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
    // get TARGET using CURL / Cache
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
      target_navaid.fpln_wp_template_type           = Utils::readAttrib (inout_osm_query.xml_query_node_to_search_a_new_target, "wp_type", "");
      target_navaid.fpln_xml_osm_q_or_raw_tmpl_node = inout_osm_query.xml_query_node_to_search_a_new_target.deepCopy ();
      target_navaid.fpln_xml_way_node               = inout_osm_query.xml_target_way_element.deepCopy ();
      target_navaid.fpln_xml_next_node_to_find_vector = inout_osm_query.xml_next_node_to_find_vector.deepCopy();
      target_navaid.fpln_target_node_estimate_vector  = inout_osm_query.target_node_estimate_vector;

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
      if (target_navaid.fpln_wp_template_type == mxconst::get_FL_TEMPLATE_VAL_LAND ())
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
RandomEngine::gen_prepare_medevac_surprise_me (IXMLNode &inRootTemplate, const IXMLNode &inoutMetaNode, const missionx::Point& in_plane_location)
{
  // 1. Analyze the OSM data around the plane based on the "osm_gen.xml" file.
  //    Pick only up to four of the analyzed categories.
  // 2. Randomly pick one of the analyzed osm categories.
  // 3. Pick one of the "subject" subcategories queries and fetch its "ways" data.
  // 4. Pick one random "nd" node as your target.

  bool flag_one_of_the_targets_above_water {false};
  missionx::mx_return out_func_result;
  const std::string osm_gen_xml_filename = fmt::format ("{}/missionx/{}", Utils::getRelativePluginsPath (), Utils::getNodeText_type_6 (missionx::system_actions::pluginSetupOptions.node, mxconst::get_PROP_OSM_GEN_FILE (), mxconst::DEFAULT_OSM_GEN_FILE.data ()));
  const std::string osm_gen_custom_xml_filename = fmt::format ("{}/missionx/{}", Utils::getRelativePluginsPath (), mxconst::DEFAULT_CUSTOM_OSM_GEN_FILE.data () );
  const std::string cache_folder = fmt::format ("{}/{}", Utils::getRelativePluginsPath (), "missionx/db/cache"); // cache folder location should be in missionx/db/cache
  missionx::data_manager::check_cache_folder (cache_folder); // will check if folder exists and if not will create it.

  ////////////////// Step 1 - Call OSM Analyze ///////////////
  IXMLNode osm_gen_xml_root_node = IXMLNode::emptyIXMLNode;

  // use "custom_osm_gen.xml" or original "osm_gen.xml" file.
  const std::string xml_filename = (mxUtils::check_file_exists (osm_gen_custom_xml_filename))? osm_gen_custom_xml_filename : osm_gen_xml_filename;

  Log::logMsgThread( fmt::format("[{}] Will read xml file: '{}'", __func__, xml_filename) );

  // check [abort]
  if (RandomEngine::random_thread_state.flagAbortThread)
  {
    out_func_result.addErrMsg ("User asked to abort.", true);
    return out_func_result;
  }

  // ----------------------
  // OSM ANALYZE - Step 01
  // ----------------------
  const std::vector<missionx::structs::strct_osm_query> vec_osm_queries = gen_osm_analyse (out_func_result, xml_filename, cache_folder, in_plane_location.lat, in_plane_location.lon, osm_gen_xml_root_node);

  // check [abort]
  if (RandomEngine::random_thread_state.flagAbortThread)
  {
    out_func_result.addErrMsg ("User asked to abort.", true);
    return out_func_result;
  }

  // validate there are results or fail the function.
  if (vec_osm_queries.empty ())
  {
    out_func_result.addErrMsg ("No valid data was found using the webosm. Aborting.", true);
    return out_func_result;
  }

  //// Shuffle the vector of OSM Analyzed Count Queries and get a target
  std::map<int, NavAidInfo> navaid_targets;

  missionx::structs::strct_osm_query osm_query; // initialized in "get_osm_topic_subject_and_prep_shuffled_q()" function.
  osm_query.cache_folder = cache_folder; // INITIALIZING THE CACHE FOLDER

  IXMLNode         main_subject_node           = IXMLNode::emptyIXMLNode; // initialized in "gen_shuffled_q_from_osm_subject_node()" function.
  std::vector<int> vec_shuffle_subject_queries = missionx::RandomEngine::gen_shuffled_q_from_osm_subject_node (&RandomEngine::random_thread_state, osm_gen_xml_root_node, vec_osm_queries, main_subject_node, osm_query);
  for (const auto &randomNumber : vec_shuffle_subject_queries)
  {
    // check [abort]
    if (RandomEngine::random_thread_state.flagAbortThread)
    {
      out_func_result.addErrMsg ("User asked to abort.", true);
      return out_func_result;
    }


    // Pick the subject query
    osm_query.xml_q_tags_header_node = main_subject_node.deepCopy ();
    osm_query.xml_query_node_to_search_a_new_target = main_subject_node.getChildNode ("q", randomNumber);

    // ----------------------
    //  >> GET TARGETS  <<  - CALL GENERIC OVERPASS - Step 2
    // ----------------------
    navaid_targets = gen_get_targets_using_osm_queries_from_a_thread (&RandomEngine::random_thread_state, osm_gen_xml_root_node, osm_query, RandomEngine::shared_navaid_info );
    if (!navaid_targets.empty ())
      break; // Exit loop
  } // end loop over shuffled "q" nodes


  // check [abort]
  if (RandomEngine::random_thread_state.flagAbortThread)
  {
    out_func_result.addErrMsg ("User asked to abort.", true);
    return out_func_result;
  }

  // fail mission build if no targets were found
  if (navaid_targets.empty ())
  {
    out_func_result.addErrMsg ("No valid targets were found. Aborting.", true);
    return out_func_result;

  }


  //---------------------------------------
  //--- Water Bodies / Slope / Leg Name ---
  //---------------------------------------
  for (auto &[indx, target_navaid] : navaid_targets)
  {
    target_navaid.fpln_seq = indx;

    target_navaid.fpln_is_wet = get_is_wet_at_point (target_navaid);

    // store wet state if the "flag value" is not true, yet.
    if (!flag_one_of_the_targets_above_water)
      flag_one_of_the_targets_above_water = target_navaid.fpln_is_wet;

    // store slope at the target location
    target_navaid.fpln_slope = get_slope_at_point (target_navaid);

    target_navaid.fpln_leg_name = gen_leg_name ( &this->seq_waypoints, mxconst::get_GPS_WP (),"leg", target_navaid );
  }

  // validate navaid targets
  int valid_navaids_i = 0;
  auto navaids_validation = gen_validate_navaids (navaid_targets, valid_navaids_i);
  if (!navaids_validation.result) // if there is a failure
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg (navaids_validation.getErrorsAsText (), true);
    return out_func_result;
  }

  if ( valid_navaids_i != static_cast<int>(navaid_targets.size ()) )
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg ( fmt::format("Valid targets found: {}, is not the same as overall generated targets: {}", valid_navaids_i, navaid_targets.size ()), true);
    return out_func_result;
  }


  // ----------------------
  // -- Add <briefer> node - Start Location
  // ----------------------
  NavAidInfo start_navaid;
  start_navaid.p  = in_plane_location;
  start_navaid.syncPointToNav ();

  navaid_targets[0] = gen_briefer_phase_02_base_node_from_navaid (start_navaid, RandomEngine::shared_navaid_info, flag_one_of_the_targets_above_water);



  // ----------------------
  // -- Read and set <mission_info>
  // ----------------------
  IXMLNode x_local_BrieferInfo;
  if (missionx::RandomEngine::working_tempFile_ptr != nullptr)
  {
    auto template_image_file_name = (missionx::RandomEngine::working_tempFile_ptr->getTemplateImageFileName ().empty ())? mxconst::get_DEFAULT_RANDOM_IMAGE_FILE() : missionx::RandomEngine::working_tempFile_ptr->getTemplateImageFileName ();
    auto template_name            = missionx::RandomEngine::working_tempFile_ptr->fullFilePath;
    auto template_folder_name     = missionx::RandomEngine::working_tempFile_ptr->missionFolderName;

    x_local_BrieferInfo = gen_mission_info_node (inRootTemplate, template_name, template_image_file_name, template_folder_name );
  }
  else
    x_local_BrieferInfo = gen_mission_info_node (inRootTemplate, "", "", "");


  #ifndef RELEASE
  Log::logMsgThread ( fmt::format("--- osm_targets {} ----------------------------->>>", __func__ ) );
  for (auto &[k, na] : navaid_targets)
    Log::logMsgThread ( fmt::format ("[{}] {}. \tpos: [{}]", k, na.get_loc_desc (), na.get_latLon () ) );
  Log::logMsgThread ( fmt::format("<<<--- End {} -------------------------------\n\n", __func__ ) );
  #endif


  // ------------------------------------------------------------------
  // Construct all mission <leg> nodes
  // ------------------------------------------------------------------

  gen_create_all_leg_nodes_based_on_navaid_targets (navaid_targets);

  // loop over all targets and add specific settings
  for (auto &[indx, target_navaid] : navaid_targets)
  {
    // Add external inventory
    // will skip any navaid that is the same as the Start location.
    if (!target_navaid.flag_is_same_as_start_location) //
    {
      if (target_navaid.fpln_xml_inv_node.isEmpty ())
      {
        target_navaid.fpln_xml_inv_node = gen_add_inventory_phase01_node (indx, target_navaid, map_osm_inventory_track);
        //  skip items phase, if it is the last location or inventory node is empty.
        if (!target_navaid.fpln_xml_inv_node.isEmpty () && navaid_targets.contains (indx + 1))
          gen_add_inventory_phase02_add_items (target_navaid);
      }
    }

    if (indx == 0) // skip briefer
    {
      target_navaid.fpln_mission_phase = missionx::enums::mx_rnd_mission_phase::start;
      continue;
    }

    // decide the "landing type"
    if (target_navaid.fpln_wp_template_type.empty ())
    {
      if (target_navaid.fpln_seq % 2 == 0)
        target_navaid.fpln_wp_template_type = mxconst::get_FL_TEMPLATE_VAL_LAND ();
      else
        target_navaid.fpln_wp_template_type = mxconst::get_FL_TEMPLATE_VAL_LAND_HOVER ();
    }

    // todo: move script creation after all targets were generated
    if (!target_navaid.fpln_xml_inv_node.isEmpty ())
    {
      // create scripts and attach them into the <inventory> as a subelement.
      RandomEngine::gen_target_inventory_scripts (target_navaid, map_osm_inventory_track);
    }

    //-------------------------
    // Calculate distances, bearing and initialize the "next_leg" or "starting_leg" of the <leg>/<briefer> nodes
    //-------------------------

    auto next_navaid_ptr = navaid_targets.contains (indx+1)? &navaid_targets[indx+1] : nullptr ;

    // add start messages
    gen_leg_start_messages (this->seq_messages, target_navaid, navaid_targets, this->xMessages, flag_one_of_the_targets_above_water);

    // add 3D object sets
    gen_add_3d_objects_for_surprise_me_base_on_predefined_attributes (target_navaid, target_navaid.fpln_xml_target_leg_node, inRootTemplate, this->x3DObjTemplate, this->expected_slope_at_target_location_d);

    // add 3D display objects around the landing
    if (!target_navaid.flag_is_skewed)
      gen_3d_hint_objects_for_land_and_hover (target_navaid, target_navaid.fpln_xml_target_leg_node, next_navaid_ptr);

    // parse and convert the <display_object> into valid instances
    gen_3d_parse_instances_in_leg (target_navaid.fpln_xml_target_leg_node, target_navaid);

    // v25.10.1
    const bool b_add_timers = Utils::readBoolAttrib (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_ADD_COUNTDOWN (), false);
    if (b_add_timers)
      RandomEngine::gen_inject_countdown_timer (indx, navaid_targets);

    target_navaid.synchToPoint ();
    target_navaid.fpln_xml_target_leg_node = this->xFlightLegs.addChild (target_navaid.fpln_xml_target_leg_node);

    // add task trigger nodes to main trigger node.
    for (auto &node : target_navaid.fpln_leg_vec_trigger_nodes)
      this->xTriggers.addChild (node.deepCopy ());

    // add target navaid objective node to the main objectives node
    this->xObjectives.addChild (target_navaid.fpln_leg_objective_node.deepCopy ());


    // check [abort]
    if (RandomEngine::random_thread_state.flagAbortThread)
    {
      out_func_result.addErrMsg ("User asked to abort.", true);
      return out_func_result;
    }

  } // end loop over all OSM Target NavAids and construct the base information needed for the mission file

  // Add the final flight plan to display in the ui
  this->cumulative_location_desc_s = gen_get_cumulative_fpln_desc (navaid_targets);

  // ----------------------
  // -- Prepare <GPS> node
  // ----------------------
  for (const auto &na : navaid_targets | std::views::values)
  {
    auto p_gps_node      = na.p.node.deepCopy ();
    auto p_gps_skew_node = (na.xml_skewdPointNode.isEmpty ()) ? IXMLNode::emptyIXMLNode : na.xml_skewdPointNode.deepCopy ();

    p_gps_node = Utils::xml_clear_node_attributes_excluding_list (p_gps_node,
                                                                  { mxconst::get_ATTRIB_LAT (), mxconst::get_ATTRIB_LONG (), mxconst::get_ATTRIB_ELEV_FT (), mxconst::get_ELEMENT_ICAO (), mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B (), mxconst::get_PROP_IS_WET ()
                                                                  },
                                                                  false,
                                                                  true);

    p_gps_skew_node = Utils::xml_clear_node_attributes_excluding_list (p_gps_skew_node,
                                                                       { mxconst::get_ATTRIB_LAT (), mxconst::get_ATTRIB_LONG (), mxconst::get_ATTRIB_ELEV_FT (), mxconst::get_ELEMENT_ICAO (), mxconst::get_ATTRIB_NAME (), mxconst::get_ATTRIB_IS_SKEWED_POSITION_B (), mxconst::get_PROP_IS_WET ()
                                                                       },
                                                                       false,
                                                                       true);

    if (na.flag_is_skewed && !p_gps_skew_node.isEmpty ())
      this->xGPS.addChild (p_gps_skew_node);
    else
      this->xGPS.addChild (p_gps_node);
  }


  // add Briefer description
  gen_briefer_phase_03_add_desc (navaid_targets, flag_one_of_the_targets_above_water);
  this->xBriefer = navaid_targets[0].fpln_xml_target_leg_node.deepCopy ();

  // v25.10.1 Add Cold and dark
  RandomEngine::xDrefStartColdAndDark = gen_set_and_get_start_cold_and_dark (inRootTemplate, navaid_targets[1]);

  // loop over all inventories and add to the global xInventories node
  for (auto &[key, nav] : navaid_targets)
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


  return out_func_result = true;
}


// -----------------------------------


mx_return
RandomEngine::gen_prepare_mission_based_on_oilrig (IXMLNode &inRootTemplate, IXMLNode & inout_meta_node)
{
  std::string outErr;
  missionx::mx_return out_func_result = true;

  // v25.10.2
  const auto plane_type_enum_i = RandomEngine::gen_parse_plane_type (data_manager::prop_userDefinedMission_ui, inRootTemplate, inout_meta_node);
  this->setPlaneType (plane_type_enum_i); // set plane type in class level for other function usage too

  auto navaid_targets = gen_oilrig_targets (&RandomEngine::random_thread_state, missionx::data_manager::xmlMappingNode, this->xMetadata, RandomEngine::shared_navaid_info, outErr);

  if (!outErr.empty ())
  {
    out_func_result.addErrMsg (outErr, true);
    navaid_targets.clear ();
    return out_func_result;
  }

  if (navaid_targets.empty ())
  {
    out_func_result.addErrMsg ("No valid targets were found. Aborting.", true);
    return out_func_result;
  }


  // check [abort]
  if (RandomEngine::random_thread_state.flagAbortThread)
  {
    out_func_result.addErrMsg ("User asked to abort.", true);
    return out_func_result;
  }

  bool flag_one_of_the_targets_above_water = false;
  //-----------------------------------------------
  //--- Analyze Water Bodies / Slope / Leg Name ---
  //-----------------------------------------------
  for (auto &[indx, target_navaid] : navaid_targets)
  {
    target_navaid.fpln_seq = indx;

    target_navaid.flag_is_skewed = false;
    target_navaid.fpln_wp_template_type = mxconst::get_FL_TEMPLATE_VAL_LAND();

    target_navaid.fpln_is_wet = get_is_wet_at_point (target_navaid);

    // store wet state if the "flag value" is not true, yet.
    if (!flag_one_of_the_targets_above_water)
      flag_one_of_the_targets_above_water = target_navaid.fpln_is_wet;

    // store slope at the target location
    // target_navaid.fpln_slope = get_slope_at_point (target_navaid);

    // is last flight leg = defined in "gen_create_all_leg_nodes_based_on_navaid_targets()"

    target_navaid.fpln_leg_name = gen_leg_name ( &this->seq_waypoints, mxconst::get_GPS_WP (),"leg", target_navaid );

  } // end analyze targets

  // validate navaid targets
  int valid_navaids_i = 0;
  auto navaids_validation = gen_validate_navaids (navaid_targets, valid_navaids_i);
  if (!navaids_validation.result) // if there is a failure
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg (navaids_validation.getErrorsAsText (), true);
    return out_func_result;
  }

  if ( valid_navaids_i != static_cast<int>(navaid_targets.size ()) )
  {
    navaid_targets.clear ();
    out_func_result.addErrMsg ( fmt::format("Valid targets found: {}, is not the same as overall generated targets: {}", valid_navaids_i, navaid_targets.size ()), true);
    return out_func_result;
  }

  // ----------------------
  // -- Add <briefer> node - Start Location
  // ----------------------
  // navaid_targets[0].fpln_navaid_was_already_prepared = true; // force flag ?? do we need to add this ?
  gen_briefer_phase_02_base_node_from_navaid (navaid_targets[0], RandomEngine::shared_navaid_info, flag_one_of_the_targets_above_water);

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
  // Action type: "extraction/land"
  // ------------------------------------------------------------------
  gen_create_all_leg_nodes_based_on_navaid_targets (navaid_targets);

  // loop over all targets and add finish touch
  for (auto &[indx, target_navaid] : navaid_targets)
  {
    // target_navaid.fpln_seq = indx;

    // Add external inventory
    // will skip any navaid that is the same as the Start location.
    if (!target_navaid.flag_is_same_as_start_location ) //
    {
      target_navaid.fpln_xml_inv_node = gen_add_inventory_phase01_node (indx, target_navaid, map_osm_inventory_track);
      //  skip items phase, if it is the last location or inventory node is empty.
      if ( !target_navaid.fpln_xml_inv_node.isEmpty () && navaid_targets.contains (indx+1))
        gen_add_inventory_phase02_add_items (target_navaid);
    }

    if (indx == 0) // skip briefer
    {
      target_navaid.fpln_mission_phase = missionx::enums::mx_rnd_mission_phase::start;
      continue;
    }


    // add start messages
    gen_leg_start_messages (this->seq_messages, target_navaid, navaid_targets, this->xMessages, flag_one_of_the_targets_above_water);

    // // add hint messages related to the target land/hover actions
    // gen_messages_when_reaching_target_leg (this->seq_triggers, this->seq_messages, target_navaid, this->xMessages, this->xTriggers, xTriggerTargetLand, xTriggerTargetHover);


    // add 3D display objects around the landing
    // v25.09.2 add support for <display_object_set>
    gen_3d_add_display_object_sets_instances_to_leg (target_navaid, target_navaid.fpln_xml_target_leg_node, inRootTemplate, this->x3DObjTemplate, this->expected_slope_at_target_location_d);

    gen_3d_parse_instances_in_leg (target_navaid.fpln_xml_target_leg_node, target_navaid);

    // v25.10.1
    const bool b_add_timers = Utils::readBoolAttrib (missionx::data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_ADD_COUNTDOWN (), false);
    if (b_add_timers)
      RandomEngine::gen_inject_countdown_timer (indx, navaid_targets);

    // ADD <leg> XML Node
    target_navaid.synchToPoint ();
    target_navaid.fpln_xml_target_leg_node = this->xFlightLegs.addChild (target_navaid.fpln_xml_target_leg_node);

    // add task <trigger> nodes to main trigger node.
    for (auto &node : target_navaid.fpln_leg_vec_trigger_nodes)
      this->xTriggers.addChild (node.deepCopy ());

    // add target navaid <objective> node to the main objectives node
    this->xObjectives.addChild (target_navaid.fpln_leg_objective_node.deepCopy ());

  } // end loop over all Targets

  // Add the final flight plan to display in the ui
  this->cumulative_location_desc_s = gen_get_cumulative_fpln_desc (navaid_targets);


  // check [abort]
  if (RandomEngine::random_thread_state.flagAbortThread)
  {
    out_func_result.addErrMsg ("User asked to abort.", true);
    return out_func_result;
  }

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


    this->xGPS.addChild (p_gps_node); // no skewed navaids
  }


  // ----------------------------
  // add Briefer description
  // ----------------------------
  gen_briefer_phase_03_add_desc (navaid_targets, flag_one_of_the_targets_above_water);
  this->xBriefer = navaid_targets[0].fpln_xml_target_leg_node.deepCopy ();

  RandomEngine::xDrefStartColdAndDark = gen_set_and_get_start_cold_and_dark (inRootTemplate, navaid_targets[1]);

  // loop over all inventories and add to the global xInventories node
  for (auto &[key, nav] : navaid_targets)
  {
    // add to inventories
    nav.fpln_xml_inv_node = this->xInventoris.addChild (nav.fpln_xml_inv_node);
  }


  #ifndef RELEASE
  Log::logMsgThread (fmt::format ("-------------- RESULTS OIL-RIG - Post {} --------------", __func__));
  Log::logMsgThread (fmt::format ("BRIEFER_INFO:\n{}\n", Utils::xml_get_node_content_as_text (x_local_BrieferInfo)));
  Log::logMsgThread (fmt::format ("BRIEFER:\n{}\n", Utils::xml_get_node_content_as_text (navaid_targets[0].fpln_xml_target_leg_node))); // we store the briefer in [0]
  Log::logMsgThread (fmt::format ("TRIGGERS:\n{}\n", Utils::xml_get_node_content_as_text (this->xTriggers)));
  Log::logMsgThread (fmt::format ("OBJECTIVES:\n{}\n", Utils::xml_get_node_content_as_text (this->xObjectives)));
  Log::logMsgThread (fmt::format ("FLIGHT LEGS:\n{}\n", Utils::xml_get_node_content_as_text (this->xFlightLegs)));
  Log::logMsgThread (fmt::format ("Inventories:\n{}\n", Utils::xml_get_node_content_as_text (this->xInventoris)));
  Log::logMsgThread (fmt::format ("GPS:\n{}\n", Utils::xml_get_node_content_as_text (this->xGPS)));
  Log::logMsgThread (fmt::format ("-------------- END OIL-RIG RESULTS - {} --------------", __func__));
  #endif // !RELEASE

  // } // end loop over all target nodes

  return out_func_result;
}

// -----------------------------------

missionx::mx_plane_types_enum
RandomEngine::gen_parse_plane_type (missionx::mx_base_node &in_user_property_ui_node, IXMLNode & inout_parent_node, IXMLNode &inout_meta_node)
{
  const auto med_cargo_or_oilrig_i             = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::undefined)); // 0 = med, 1 = cargo
  const auto mission_subcategory_indx_picked_i = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_MISSION_SUBCATEGORY (), static_cast<int> (missionx::mx_mission_subcategory_type::not_defined)); //
  #ifndef RELEASE
  const auto uiLayer_debug                     = data_manager::getGeneratedFromLayer (); // v25.02.1
  #endif

  const std::string CATEGORY_TRANSLATION = missionx::data_manager::get_translate_of_mission_subcategory_code (med_cargo_or_oilrig_i, mission_subcategory_indx_picked_i, inout_meta_node); // v3.303.14
  auto       plane_type_i        = Utils::readNodeNumericAttrib<int> (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types_enum::plane_type_props)); // plane type

  // validations
  assert (med_cargo_or_oilrig_i > static_cast<int> (missionx::mx_ui_mission_type::undefined) && ": Main Mission Type can't be undefined. Aborting!!!"); // debug
  assert (CATEGORY_TRANSLATION.empty () == false && ": Sub Category was not found. Aborting!!!"); // debug

  // Force helos for oilrig missions
  if (med_cargo_or_oilrig_i == static_cast<int> (missionx::mx_ui_mission_type::oil_rig))
  {
    plane_type_i = static_cast<int> (missionx::mx_plane_types_enum::plane_type_helos);
  }
  auto        conv_plane_type_i = static_cast<missionx::def_mx_plane_type_enum> (plane_type_i);
  std::string plane_type_s      = missionx::RandomEngine::translatePlaneTypeToString (conv_plane_type_i);

  // Store plane type in the XML node
  in_user_property_ui_node.setNodeStringProperty (mxconst::get_PROP_PLANE_TYPE_S (), plane_type_s);
  inout_parent_node.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ());
  inout_meta_node.updateAttribute (plane_type_s.c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str (), mxconst::get_ATTRIB_PLANE_TYPE ().c_str ()); // v25.05.1

  return conv_plane_type_i;
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
  assert (RandomEngine::working_tempFile_ptr != nullptr && fmt::format("[{}] Template pointer is invalid.", __func__).c_str () );

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
                                       NavAidInfo*                         prev_navaid_ptr,
                                       double                              min_lat,
                                       double                              max_lat,
                                       double                              min_lon,
                                       double                              max_lon,
                                       const double                        maxDistance_d,
                                       const double                        minDistance_d)
{
  assert (prev_navaid_ptr != nullptr && fmt::format ("Invalid previous navaid pointer. Notify developer.", __func__).c_str ()); // v25.10.1

  bool bResult = false;

  if (prev_navaid_ptr == nullptr) // v25.10.1
    return false;

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
    bResult = osm_get_navaid_from_osm_database (outNavAid, inMapLocationSplitValues, inProperties, prev_navaid_ptr->lat, prev_navaid_ptr->lon, min_lat, max_lat, min_lon, max_lon, maxDistance_d, minDistance_d);
    outNavAid.flag_fetched_from_db = true;
    if (bResult)
      return bResult;
  }

  // Priority 2 is for overpass data (web information)
  if (!bResult && (Utils::readBoolAttrib (data_manager::prop_userDefinedMission_ui.node, mxconst::get_PROP_USE_WEB_OSM_CHECKBOX (), false) || (expectedLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ())))
  {
    outNavAid.flag_fetched_from_db = false; // v25.09.2
    return osm_get_navaid_from_overpass (outNavAid, inMapLocationSplitValues, inProperties, prev_navaid_ptr->lat, prev_navaid_ptr->lon, min_lat, max_lat, min_lon, max_lon, maxDistance_d, minDistance_d);
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

  outNavAid.flag_fetched_from_webosm = true;

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
  if (missionx::RandomEngine::random_thread_state.flagAbortThread)
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
    if (missionx::RandomEngine::random_thread_state.flagAbortThread)
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
            if (missionx::RandomEngine::random_thread_state.flagAbortThread)
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
          // if (lastFlightLegNavInfo.lat != 0 && lastFlightLegNavInfo.lon != 0)
          if (sourceLat_d * sourceLon_d != 0.0)
          {
            const double distance_to_target = Utils::calcDistanceBetween2Points_nm (sourceLat_d, sourceLon_d, outNavAid.lat, outNavAid.lon);
            double       nm_d               = (nm_s.empty ()) ? static_cast<double> (mxconst::INT_UNDEFINED) : mxUtils::stringToNumber<double> (nm_s, 2);

            #ifndef RELEASE
            Log::logMsgThread (fmt::format ("[overpass2] Test Distance. Target distance: {}, Allowed distances[nm/between] [nm: {}/ between: {} and {}]", distance_to_target, (nm_d > 0.0) ? mxUtils::formatNumber<double> (nm_d, 2) : "Not Defined", minDistance_d, maxDistance_d)); // debug
            #endif

            if (!missionx::RandomEngine::gen_get_is_navaid_in_a_valid_distance (distance_to_target, nm_d, minDistance_d, maxDistance_d))
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



          if (missionx::RandomEngine::random_thread_state.flagAbortThread)
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
            outNavAid.flag_fetched_from_webosm = true;
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


std::string
RandomEngine::get_short_flight_description_from_to (const std::string &inFromName, const std::string &inFromICAO, const std::string &inToName, const std::string &inToICAO)
{
  if (inFromName == mxconst::get_ELEMENT_BRIEFER ())
    return fmt::format (R"(from "{}" to "{} [{}]")", inFromName, inToName, inToICAO);

  return fmt::format (R"(from "{} [{}]" to "{} [{}]")", inFromName, inFromICAO, inToName, inToICAO);
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
RandomEngine::gen_get_is_navaid_in_a_valid_distance (const double &currentDistanceToTarget, const double &in_location_value_d, const double &in_location_minDistance_d, const double &in_location_maxDistance_d)
{
  if (in_location_value_d > 0.0 && currentDistanceToTarget <= in_location_value_d) // location_value_d represents "nm", It has precedence over min/max
    return true;
  else if (in_location_minDistance_d >= 0.0 && in_location_maxDistance_d > in_location_minDistance_d) // check if between min and max values
    return (currentDistanceToTarget >= in_location_minDistance_d && currentDistanceToTarget <= in_location_maxDistance_d);

  return currentDistanceToTarget > static_cast<double> (mxconst::MIN_DISTANCE_TO_SEARCH_AIRPORT); // accept distance if in the limit of search airport
}

// -----------------------------------

bool
RandomEngine::gen_get_target_base_on_tag_name (NavAidInfo &outNewNavInfo
                                                  , std::map<std::string, std::string>& inMapLocationSplitValues
                                                  , missionx::mx_base_node &inProperties
                                                  , NavAidInfo *prev_na_ptr
                                                  )
{

  assert ( prev_na_ptr != nullptr && fmt::format("[{}] Previous Navigation Aid is mandatory for this function.", __func__).c_str ()); // debug

  if (prev_na_ptr == nullptr)
  {
    outNewNavInfo.init ();
    outNewNavInfo.err = fmt::format ("[{}] Previous navigation data is not accessible.", __func__);
    return false;
  }


  // flag_force_template_distances_b to let designer force his "narrative" when it comes to distances.
  const bool        flag_override_random_target_min_dist = (missionx::RandomEngine::flag_force_template_distances_b) ? false : missionx::system_actions::pluginSetupOptions.getBoolValue (mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE ());
  const std::string inFlightLegName                      = inProperties.getNodeStringProperty ( mxconst::get_ATTRIB_NAME (), "" );
  const std::string inTemplateType                       = inProperties.getNodeStringProperty ( mxconst::get_ATTRIB_TYPE (), "");
  const std::string inLocationType                       = inProperties.getNodeStringProperty ( mxconst::get_ATTRIB_LOCATION_TYPE (), "");
  const bool        flag_force_template_type             = inProperties.getBoolValue (mxconst::get_ATTRIB_PICK_LOCATION_BASED_ON_SAME_TEMPLATE_B (), false);

  const std::string location_value_tag_name_s =inProperties.getNodeStringProperty ("tag");
  const auto location_value_d        = inProperties.getAttribNumericValue<double> ("location_value_d", -1.0);
  auto location_min_distance_d = inProperties.getAttribNumericValue<double> ("location_min_distance_d", location_value_d);
  auto location_max_distance_d = inProperties.getAttribNumericValue<double> ("location_max_distance_d", location_min_distance_d * 4.0);


  IXMLNode xPoint = IXMLNode::emptyIXMLNode; // local xml <point> element representative.

  IXMLNode rNode = missionx::RandomEngine::xRootTemplate.getChildNode (location_value_tag_name_s.c_str ()).deepCopy ();
  if (rNode.isEmpty ())
  {
    setError ("[random get_target rNode] fail to find random pick element. Please fix your template. skipping flight leg: " + inFlightLegName);
    return false;
  }

  RandomEngine::shared_navaid_info.init ();
  RandomEngine::shared_navaid_info.parentNode_ptr = rNode; // store pointer to XML node
  missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::convert_icao_to_xml_point); // will call missionx::flcPRE() and try to convert any <icao name="icao name" /> to <point targetLat="" targetLon="" />

  // NEAR - do we need to find the nearest location?
  if (inLocationType == mxconst::get_EXPECTED_LOCATION_TYPE_NEAR ())
  {
    // Find Nearest NavAid based on given points
    // loop over all points and pick the one that is NEAREST to the current point.
    if (prev_na_ptr->is_lat_lon_valid ())
    {
      missionx::NavAidInfo picked_ni;
      double               last_shortest_dist = mxconst::INT_UNDEFINED;

      const int nChilds = rNode.nChildNode (mxconst::get_ELEMENT_POINT ().c_str ());
      for (int i1 = 0; i1 < nChilds; ++i1)
      {
        IXMLNode cNode = rNode.getChildNode (mxconst::get_ELEMENT_POINT ().c_str (), i1);
        if (cNode.isEmpty ())
          continue;

        missionx::NavAidInfo ni;
        ni.lat      = static_cast<float> (Utils::readNumericAttrib (cNode, mxconst::get_ATTRIB_LAT (), 0.0));
        ni.lon      = static_cast<float> (Utils::readNumericAttrib (cNode, mxconst::get_ATTRIB_LONG (), 0.0));
        ni.loc_desc = Utils::readAttrib (cNode, mxconst::get_ATTRIB_LOC_DESC (), "");

        if (ni.lat == 0.0 || ni.lon == 0.0) // skipping if one of the values = 0
          continue;

        const double distance = Utils::calcDistanceBetween2Points_nm (prev_na_ptr->lat, prev_na_ptr->lon, ni.lat, ni.lon);

        if (missionx::RandomEngine::gen_get_is_navaid_in_a_valid_distance (distance, location_value_d, location_min_distance_d, location_max_distance_d)) // v3.0.255.4.1 add "nm" and "nm_between" rules
        {
          #ifndef RELEASE
          Log::logMsgThread ( fmt::format("[{}] Target: {} is in a valid distance: {}", __func__, ni.loc_desc, mxUtils::formatNumber<double> (distance)) );
          #endif // !RELEASE

          if (last_shortest_dist < 0.0 || distance < last_shortest_dist) // if first time or "new distance" shorter than "last_shortest_dist"
          {
            last_shortest_dist = distance;
            picked_ni      = ni; // store closest point
          }
        }
        #ifndef RELEASE
        else
        {
          Log::logMsgThread (fmt::format ("[get target based tag] Target: {}, invalid distance: {}, Should be nm_between: {:.2f}-{:.2f} {}", ni.loc_desc, distance, location_min_distance_d, location_max_distance_d, (location_value_d > 0.0) ? fmt::format (", or nm: {:.2f}", location_value_d) : "")); // debug
        }
        #endif // !RELEASE
      }
      // v25.09.2 we only store the Coordinates and name
      outNewNavInfo.lat = picked_ni.lat;
      outNewNavInfo.lon = picked_ni.lon;
      outNewNavInfo.setID (picked_ni.getID ());
      outNewNavInfo.setName ( picked_ni.getName () );

      outNewNavInfo.synchToPoint (true);
      if (outNewNavInfo.is_lat_lon_valid ())
        return true;
    }

  } // end handling <points> from <tag name>
  else
  {
    // pick any <point> from the element (not "near" location type)

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
        location_min_distance_d = 0.0;
        location_max_distance_d = location_value_d; // if location_value_nm_s has value then use it as max and ignore location_minDistance_d original value.
      }

      bool flag_searchAnotherPoint = false; // v3.0.221.15 rc3
      do
      {
        // reset flag_searchAnotherPoint
        std::string err;
        flag_searchAnotherPoint = false;

        IXMLNode rnd_x_point = Utils::xml_get_node_randomly_by_name_and_distance_IXMLNode (rNode, mxconst::get_ELEMENT_POINT (), prev_na_ptr->lat, prev_na_ptr->lon, err, location_min_distance_d, location_max_distance_d, true); // remove picked point

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

        // check if the template type is different from xPoint type. We will have to check template point attribute + isWet and slope
        std::string pointTemplate = Utils::stringToLower (Utils::readAttrib (xPoint, mxconst::get_ATTRIB_TEMPLATE (), ""));

        flag_searchAnotherPoint = false; // exit while loop

        #ifndef RELEASE
        nPointChilds = rNode.nChildNode (mxconst::get_ELEMENT_POINT ().c_str ());
        Log::logMsgThread (fmt::format("\t[{}] After point randomly picked. No. of points left: {}\n", __func__, Utils::formatNumber<int> (nPointChilds) ) );
        #endif
      } while (flag_searchAnotherPoint); // end picking a point from a pre-defined location or the nearest one

    } while (xPoint.isEmpty () && flag_override_random_target_min_dist && loop_counter_i < 2); // Max loop will be the second time when we use developer values

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
RandomEngine::gen_target_base_on_icao_or_near_types (NavAidInfo &outNewNavInfo
                                                  , mx_plane_types_enum in_plane_type_enum
                                                  , std::map<std::string, std::string> &inMapLocationSplitValues
                                                  , missionx::mx_base_node &inProperties
                                                  , NavAidInfo *prev_na_ptr)
{
  assert ( prev_na_ptr != nullptr && fmt::format("[{}] Previous Navigation Aid is mandatory for this function.", __func__).c_str ()); // debug

  if (prev_na_ptr == nullptr)
  {
    outNewNavInfo.init ();
    outNewNavInfo.err = fmt::format ("[{}] Previous navigation data is not accessible.", __func__);
    return false;
  }

  // Gather needed data
  auto       location_value_nm_s               = mxUtils::getValueFromElement (inMapLocationSplitValues, std::string ("nm"), std::string (""));

  auto       location_value_d          = inProperties.getAttribNumericValue<double> ("location_value_d", -1.0);
  auto       location_min_distance_d   = inProperties.getAttribNumericValue<double> ("location_min_distance_d", location_value_d);
  auto       location_max_distance_d   = inProperties.getAttribNumericValue<double> ("location_max_distance_d", location_value_d);
  const auto location_value_tag_name_s = inProperties.getStringAttributeValue ("tag", "");

  // replace "_" with empty string
  if (location_value_nm_s == "_") // if special character that represent empty
    location_value_nm_s.clear ();


  const bool        flag_override_random_target_min_dist = (missionx::RandomEngine::flag_force_template_distances_b) ? false : missionx::system_actions::pluginSetupOptions.getBoolValue (mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE ()); // this->flag_force_template_distances_b to let designer force their "narrative" when it comes to distances.
  const std::string inLocationType                       = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), "");

  // Prepare distance to target
  // location_value_d has precedence over nm_between, unless we defined it in the setup flag_override_random_target_min_dist. we will use nm_between if location_value_d is smaller than 1.0nm
  // double nm_random_distance_d         = 1.5;
  // double nm_max_distance_osm_radius_d = 0.0; // will hold the expected max radius nm value for OSM based legs

  // validate location_max_distance_d
  if (location_value_d > location_max_distance_d)
    location_max_distance_d = location_value_d;

  if (location_max_distance_d < 0.0)
    location_max_distance_d = (location_value_d > 0.0)? location_value_d : 50;

  if (location_min_distance_d > location_max_distance_d)
  {
    auto tmp_val = location_max_distance_d;
    location_max_distance_d = location_min_distance_d;
    location_min_distance_d = tmp_val;
  }

  // validate min distance
  if (location_min_distance_d > location_value_d )
    location_min_distance_d = location_value_d;

  if (location_min_distance_d < 0.0)
    location_min_distance_d = ((4.0 * 1.5) < location_max_distance_d)? 1.5 : location_max_distance_d * 0.25; // distance in nm

  // Search ICAO
  if (mxconst::get_EXPECTED_LOCATION_TYPE_ICAO() == inLocationType )
  {
    outNewNavInfo = missionx::RandomEngine::get_random_airport_from_db (prev_na_ptr->p, static_cast<float>(location_min_distance_d), static_cast<float>(location_max_distance_d), static_cast<int>(prev_na_ptr->bearing_back_to_prev_target), inProperties, static_cast<uint8_t>(in_plane_type_enum));
    if (outNewNavInfo.is_lat_lon_valid())
      return true;

    outNewNavInfo.err = fmt::format ("[{}] ICAO navaid search, produced an invalid navaid result.", __func__);
    return false;
  }

  // If defined nothing, then search for NEAR.
  // Get the nearest NavAid relative to the last position if no location_value or location_tag_name were defined. Plane type is not relevant
  if ( (location_value_nm_s.empty () && location_value_tag_name_s.empty () ) || mxconst::get_EXPECTED_LOCATION_TYPE_NEAR() == inLocationType )
  {
    outNewNavInfo = missionx::RandomEngine::get_random_airport_from_db (prev_na_ptr->p, 3.0f, (50.0 > location_max_distance_d)? 50.0f : static_cast<float>(location_max_distance_d) , -1, inProperties, static_cast<uint8_t>(in_plane_type_enum) );
    if (outNewNavInfo.is_lat_lon_valid())
      return true;

    outNewNavInfo.err = fmt::format ("[{}] Near navaid search, produced an invalid navaid result.", __func__);
    return false;
  }

  // Fetch target based on <tag> name
  if (!location_value_tag_name_s.empty ())
  {
    return RandomEngine::gen_get_target_base_on_tag_name (outNewNavInfo, inMapLocationSplitValues, inProperties, prev_na_ptr);
  } // end navaid based on tag name



  outNewNavInfo.err = fmt::format ("[{}] Failed to fetch a navaid. Notify Developer.", __func__);
  return false;
}

// -----------------------------------


bool
RandomEngine::gen_target_base_on_xy_osm_or_osmweb_types (NavAidInfo &outNewNavInfo
                                                       , mx_plane_types_enum in_plane_type_enum
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
  auto location_max_distance_d = inProperties.getAttribNumericValue<double> ("location_max_distance_d", location_value_d);


  const bool        flag_override_random_target_min_dist = (missionx::RandomEngine::flag_force_template_distances_b) ? false : missionx::system_actions::pluginSetupOptions.getBoolValue (mxconst::get_OPT_OVERRIDE_RANDOM_TARGET_MIN_DISTANCE ()); // this->flag_force_template_distances_b to let designer force their "narative" when it comes to distances.
  const std::string inLocationType                       = Utils::readAttrib (inProperties.node, mxconst::get_ATTRIB_LOCATION_TYPE (), "");

  // Prepare distance to target
  // location_value_d has precedence over nm_between, (v3.0.241.8) unless we defined it in the setup flag_override_random_target_min_dist. we will use nm_between if location_value_d is smaller than 1.0nm
  double nm_random_distance_d         = 1.5;
  double nm_max_distance_osm_radius_d = 0.0; // v3.0.241.10 will hold the expected max radius nm value for OSM-based legs

  // validate distances are valid, and they respect "location_value_d" value, that represents the designer distance pick.
  if (location_value_d > location_max_distance_d && location_value_d >= 0.0)
    location_max_distance_d = location_value_d;

  if (location_min_distance_d >= location_max_distance_d)
    location_min_distance_d = location_max_distance_d / 2.0;

  if (location_min_distance_d > location_value_d && location_value_d >= 0.0)
    location_min_distance_d = location_value_d;

  if ( (location_min_distance_d > 0.0 && location_max_distance_d > 0.0) || (flag_override_random_target_min_dist && location_min_distance_d > 0.0 && location_max_distance_d > 0.0)) // v3.0.241.8 added setup flag hint
  {
    // v3.0.241.8 respecting the location_value_d defined by the designer as the min radius distance even the user preferred a higher value
    // It should balance between what the designer believe is best and what user wants. Destination should be between "designer" and "user"
    // if (location_value_d < location_min_distance_d && location_value_d > 1.0)
    //   location_min_distance_d = location_value_d;

    nm_max_distance_osm_radius_d = location_max_distance_d;
    nm_random_distance_d         = Utils::getRandomRealNumber (location_min_distance_d, location_max_distance_d);

    #ifndef RELEASE
    Log::logDebugBO (fmt::format("[{}] location: {}, location_min_distance_d: {}, location_max_distance_d: {}", __func__,inLocationType, location_min_distance_d, location_max_distance_d), true);
    #endif
  }
  else
  {
    location_value_d             = (location_value_d <= 1.0) ? 10.0 : location_value_d; // we do not need to handle flag_override_random_target_min_dist since it should have been dealt in the above "if" statement
    nm_max_distance_osm_radius_d = location_value_d;
    nm_random_distance_d         = Utils::getRandomRealNumber (1, location_value_d);

    #ifndef RELEASE
    Log::logDebugBO (fmt::format("[{}] location: {}, location_value_nm_s: {}", __func__,inLocationType, location_value_d), true);
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

    if (NavAidInfo navAid
      // ; RandomEngine::osm_get_navaid_from_osm (navAid, inMapLocationSplitValues, inProperties, prev_na_ptr->lat, prev_na_ptr->lon, S180.lat, N0.lat, W270.lon, E90.lon, nm_max_distance_osm_radius_d, location_min_distance_d))
      ; RandomEngine::osm_get_navaid_from_osm (navAid, inMapLocationSplitValues, inProperties, prev_na_ptr, S180.lat, N0.lat, W270.lon, E90.lon, nm_max_distance_osm_radius_d, location_min_distance_d))
    {
      if (navAid.is_lat_lon_valid () )
      {
        outNewNavInfo.copy_target_nav_data_only (navAid); // will do synch to point
        return true;
      }
    }

  }
  ////////// END OSM / WEB OSM target ////////////////


  // if OSM data was not found then plugin will try to use the default target search
  // v25.09.2
  outNewNavInfo.flag_fetched_from_db = false;
  outNewNavInfo.flag_fetched_from_webosm = false;

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
  Log::logDebugBO ( fmt::format("[{}] location: {}, NavAid.name: {}", __func__, inLocationType, outNewNavInfo.getNavAidName ()), true);
  #endif

  // v25.09.2 deprecated, lets see what plugin will use as default
  outNewNavInfo.flag_picked_random_lat_long = true;
  outNewNavInfo.synchToPoint (true);

  #ifndef RELEASE
  Log::logDebugBO (fmt::format("[{}] location: {}", __func__, inLocationType), true) ;
  #endif

  return true;
}

// -----------------------------------



bool
RandomEngine::gen_target_or_last_flight_leg_base_on_xy_or_osm (NavAidInfo &outNewNavInfo, mx_plane_types_enum in_plane_type_enum, std::map<std::string, std::string> &inMapLocationSplitValues
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
  auto       location_min_distance_d = inProperties.getAttribNumericValue<double> ("location_min_distance_d", (location_value_d > 0.0) ? location_value_d : 1.0);
  auto       location_max_distance_d = inProperties.getAttribNumericValue<double> ("location_max_distance_d", (location_value_d > location_min_distance_d) ? location_value_d : location_min_distance_d * 10.0);

  mxUtils::mx_eval_min_always_smaller_than_max <double> (location_min_distance_d, location_max_distance_d, 0.1);

  #ifndef RELEASE
  if (location_value_d < 0.0)
    int i = 0;
  #endif


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
    RandomEngine::shared_navaid_info.inMinDistance_nm = (location_min_distance_d > 0.0)? static_cast<float> (location_min_distance_d) : 0.0f;
    RandomEngine::shared_navaid_info.inMaxDistance_nm = (location_max_distance_d > 0.0) ? static_cast<float> (location_max_distance_d) : mxconst::MAX_RAD_4_OSM_MAX_DIST; // v24.12.2 default distance if not set
  }

  // Search for HELOS last flight leg
  // OSM search first - this code will be used when there is a template or mission template with OSM information in it. It will probably won't be called from the user creation screen
  if ( (target_location_type == mxconst::get_EXPECTED_LOCATION_TYPE_OSM () || target_location_type == mxconst::get_EXPECTED_LOCATION_TYPE_WEBOSM ())
       && in_plane_type_enum == missionx::mx_plane_types_enum::plane_type_helos)
  {
    Point E90, W270, S180, N0;

    // get max radius and find the 4 points that create the rectangle area
    // RandomEngine::shared_navaid_info.inMaxDistance_nm = max Radius
    // location_minDistance_d = min radius distance
    RandomEngine::calculate_bbox_coordinates (N0, S180, E90, W270, prev_na_ptr->lat, prev_na_ptr->lon, RandomEngine::shared_navaid_info.inMaxDistance_nm);
    if (NavAidInfo local_navAid;
       // RandomEngine::osm_get_navaid_from_osm (local_navAid, inMapLocationSplitValues, inProperties, prev_na_ptr->lat, prev_na_ptr->lon, S180.lat, N0.lat, W270.lon, E90.lon, RandomEngine::shared_navaid_info.inMaxDistance_nm, location_min_distance_d))
       RandomEngine::osm_get_navaid_from_osm (local_navAid, inMapLocationSplitValues, inProperties, prev_na_ptr, S180.lat, N0.lat, W270.lon, E90.lon, RandomEngine::shared_navaid_info.inMaxDistance_nm, location_min_distance_d))
    {
      if (local_navAid.is_lat_lon_valid ()) // none of the coordinates = 0.0
      {
        ////// Test Final NavAid against X-Plane. We will check the closest Navaid to that location and it should be the same. If not we will use, for now the OSM NavAid
        // outNewNavInfo = local_navAid;
        outNewNavInfo.copy_target_nav_data_only ( local_navAid ); // will call synchToPoint() too

        // store shared info we prepared in a previous step before calling the OSM function. We will use it after calling the main thread for fallback
        const strct_shared_random_airport_info tmp_info = RandomEngine::shared_navaid_info;

        RandomEngine::shared_navaid_info.navAid.init ();
        RandomEngine::shared_navaid_info.navAid.lat = outNewNavInfo.lat;
        RandomEngine::shared_navaid_info.navAid.lon = outNewNavInfo.lon;

        // test against the nearest navaid
        if (!missionx::data_manager::waitForPluginCallbackJob (&RandomEngine::random_thread_state, missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread))
        {
          RandomEngine::setError ( fmt::format("[{}] Last Navaid. Failed to find an airport NEAR given location. Will use original Navaid data: {}", __func__, outNewNavInfo.gen_locDesc_short ()) );
        }
        // calculate distance using the Point class function.
        // outNewNavInfo.synchToPoint (); // v25.09.2 deprecated, outNewNavInfo.copy_target_nav_data_only() will call syncToPoint()

        RandomEngine::shared_navaid_info.navAid.synchToPoint ();
        const auto distance = outNewNavInfo.p.calcDistanceBetween2Points (RandomEngine::shared_navaid_info.navAid.p);
        if (( !outNewNavInfo.getID ().empty () && outNewNavInfo.getID () == RandomEngine::shared_navaid_info.navAid.getID () ) || distance <= 1.0)
        {
          outNewNavInfo.copy_target_nav_data_only (RandomEngine::shared_navaid_info.navAid);
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
  if (RandomEngine::shared_navaid_info.inMinDistance_nm < RandomEngine::shared_navaid_info.inStartFromDistance_nm && mxconst::get_EXPECTED_LOCATION_TYPE_ICAO () != location_value_restrict_ramp_type_s && mxconst::get_EXPECTED_LOCATION_TYPE_NEAR () != location_value_restrict_ramp_type_s )
    RandomEngine::shared_navaid_info.inMinDistance_nm = RandomEngine::shared_navaid_info.inStartFromDistance_nm;

  NavAidInfo nav = RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, static_cast<uint8_t>(in_plane_type_enum) ); // v3.0.255.3 test integration
  outNewNavInfo.copy_target_nav_data_only (nav);

  // FALLBACK
  // v3.0.241.8 handle: what if we failed to find a NavAid due to the user setup slider or the designer did not provide a big enough radius.
  // We will try with the "designer" area but multiply by 4.
  if (!outNewNavInfo.is_lat_lon_valid()  && RandomEngine::mapNavAidsFromMainThread.empty () && location_value_d > 0.0)
  {
    RandomEngine::shared_navaid_info.inMinDistance_nm = RandomEngine::shared_navaid_info.inStartFromDistance_nm;
    RandomEngine::shared_navaid_info.inMaxDistance_nm = static_cast<float> (location_value_d);
    if (RandomEngine::shared_navaid_info.inMinDistance_nm > RandomEngine::shared_navaid_info.inMaxDistance_nm )
      RandomEngine::shared_navaid_info.inMaxDistance_nm = RandomEngine::shared_navaid_info.inMinDistance_nm * 4.0f; // max distance is equal to "start distance" * 4.

    NavAidInfo local_nav = RandomEngine::get_random_airport_from_db (RandomEngine::shared_navaid_info.p, RandomEngine::shared_navaid_info.inMinDistance_nm, RandomEngine::shared_navaid_info.inMaxDistance_nm, RandomEngine::shared_navaid_info.inExcludeAngle, inProperties, static_cast<uint8_t>(in_plane_type_enum)); // v3.0.255.3 test integration
    outNewNavInfo.copy_target_nav_data_only ( local_nav ); // v25.09.2

  } // end gathering random NavAid

  // Filter location by location_type (NEAR, ICAO, etc...)
  // Add find the closest airport to the last location for location_type = NEAR


  outNewNavInfo.synchToPoint (true);

  // if (outNewNavInfo.lat == 0.0f || outNewNavInfo.lon == 0.0f)

  if ( !outNewNavInfo.is_lat_lon_valid () )
  {
    outNewNavInfo.init ();
    outNewNavInfo.err = fmt::format("[{}] Failed to find an airport in radius: {}nm relative to location: {}", __func__, location_value_d, prev_na_ptr->get_latLon_short () );
    RandomEngine::setError (outNewNavInfo.err);
    #ifndef RELEASE
    Log::logMsgThread (outNewNavInfo.err); // debug
    #endif

    return false;
  }

  return true;
}


// -----------------------------------

// -----------------------------------



} /* namespace missionx */
