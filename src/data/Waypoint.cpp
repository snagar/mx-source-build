#include "Waypoint.h"
#include "../core/vr/mxvr.h"


Waypoint::Waypoint()
{
  this->setStringProperty (mxconst::get_ATTRIB_TITLE (), "");
  this->setStringProperty (mxconst::get_ATTRIB_NEXT_LEG (), "");
  isFirstTime = false; // v3.0.207.5 // only Mission::flc() and Mission::START_MISSION() functions can change the status to true and it is reset again in Mission::flc_goal() to false.
  flag_cue_was_calculated = false; // should be change from the flc_cue() function
  goal_state   = missionx::enums::mx_flightLeg_state::leg_undefined;

  init();
}

// -----------------------------------

missionx::Waypoint::~Waypoint()
{
  listCueInfoToDisplayInFlightLeg.clear();
}

// -----------------------------------

bool
missionx::Waypoint::parse_node()
{
  assert(!this->node.isEmpty());

  // Read MAIN attributes and elements that do not need to update "data_manager" class

  std::string legName     = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");
  std::string legTitle    = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TITLE(), "");
  std::string leg_nextLeg = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NEXT_LEG(), mxconst::get_ATTRIB_NEXT_GOAL(), EMPTY_STRING, true);
  if (legName.empty()) // skip
  {
    Log::logMsg("[FlightLeg]Found flight <leg> with no name. Check mission definition.", format_type::error, false);
    Log::add_missionLoadError(errMsg);
    return false;
  }

  this->name = legName; // v3.303.11
  const bool flag_isDummy = this->getIsDummyLeg();

  // read description
  IXMLNode    xDesc   = this->node.getChildNode(mxconst::get_ELEMENT_DESC().c_str());
  std::string legDesc = Utils::xml_read_cdata_node(this->node, ""); // try to read cdata in <leg>    

  if (!xDesc.isEmpty() && legDesc.empty()) // v3.305.3 Changed order of CDATA read. We start from <leg> and if not then from <desc> sub element.
    legDesc = Utils::xml_read_cdata_node(xDesc, "");

  this->setStringProperty(mxconst::get_ATTRIB_NAME(), legName);
  this->setStringProperty(mxconst::get_ATTRIB_TITLE(), legTitle);
  this->setStringProperty(mxconst::get_ATTRIB_NEXT_LEG(), leg_nextLeg);

  this->setStringProperty(mxconst::get_ELEMENT_DESC(), legDesc, false);

  this->vecSentences = Utils::sentenceTokenizerWithBoundaries(legDesc, mxconst::get_SPACE(), missionx::MAX_CHARS_IN_BRIEFER_LINE_2D, ";");
  int nChilds2       = this->node.nChildNode(mxconst::get_ELEMENT_LINK_TO_OBJECTIVE().c_str());

  if (!flag_isDummy) // v3.303.12 read objectives only if <leg> was not flag as "dummy"
  {
    // read objectives.
    // check if there are any objectives link before reading goals attributes.
    if (nChilds2 == 0) // no objectives
    {
      Log::logMsg("[FlightLeg] " + legName + ", has no Objectives linked to it. Please fix it. Skipping definitions.", format_type::error, false);
      return false;
    }
    else
    {
      for (int i2 = 0; i2 < nChilds2; i2++)
      {
        IXMLNode xLinkObjective = this->node.getChildNode(mxconst::get_ELEMENT_LINK_TO_OBJECTIVE().c_str(), i2);
        if (!xLinkObjective.isEmpty())
        {
          std::string objName = Utils::readAttrib(xLinkObjective, mxconst::get_ATTRIB_NAME(), "");
          if (objName.empty())
          {
            Log::logMsg("[FlightLeg Objective Link] Found empty link name in <leg>: " + this->getName() + ". Skipping.");
            continue;
          }

          this->listObjectivesInFlightLeg.push_back(objName); // should be unique and hashed
          this->xml_listObjectivesInGoal_ptr.push_back(xLinkObjective);

        } // end if link to object exists
      }   // end loop over object links
    }
  }

  // read flight leg triggers
  nChilds2 = this->node.nChildNode(mxconst::get_ELEMENT_LINK_TO_TRIGGER().c_str());
  for (int i2 = 0; i2 < nChilds2; i2++)
  {
    IXMLNode xLink = this->node.getChildNode(mxconst::get_ELEMENT_LINK_TO_TRIGGER().c_str(), i2);
    if (!xLink.isEmpty())
    {
      std::string trigName = Utils::readAttrib(xLink, mxconst::get_ATTRIB_NAME(), "");
      if (trigName.empty())
        continue;

      this->listTriggers.push_back(trigName);
      this->xml_listTriggers.push_back(xLink);
    }

  } // end reading triggers


  // v3.0.207.5 add minor goal extensions to make designer life a little easier - send message at the start and end of a goal and execure one time scripts (pre and end of goal)
  IXMLNode xStartLegMessage, xEndLegMessage;
  IXMLNode xPreLegScript, xPostLegScript;

  xStartLegMessage = this->node.getChildNode(mxconst::get_ELEMENT_START_LEG_MESSAGE().c_str()); // v3.0.221.15rc5 add LEG support
  if (xStartLegMessage.isEmpty())
    xStartLegMessage = this->node.getChildNode(mxconst::get_ELEMENT_START_GOAL_MESSAGE().c_str()); // compatibility

  if (!xStartLegMessage.isEmpty())
  {
    std::string value = Utils::readAttrib(xStartLegMessage, mxconst::get_ATTRIB_NAME(), "");
    if (!value.empty())
      this->setNodeStringProperty(mxconst::get_ELEMENT_START_LEG_MESSAGE(), value); //, this->node, mxconst::get_ELEMENT_LEG());

    Utils::addElementToMap(this->mapFlightLeg_sub_nodes_ptr, mxconst::get_ELEMENT_START_LEG_MESSAGE(), xStartLegMessage);
  }

  xEndLegMessage = this->node.getChildNode(mxconst::get_ELEMENT_END_LEG_MESSAGE().c_str()); // v3.0.221.15rc5 add LEG support
  if (xEndLegMessage.isEmpty())
    xEndLegMessage = this->node.getChildNode(mxconst::get_ELEMENT_END_GOAL_MESSAGE().c_str()); // compatibility

  if (!xEndLegMessage.isEmpty())
  {
    std::string value = Utils::readAttrib(xEndLegMessage, mxconst::get_ATTRIB_NAME(), "");
    if (!value.empty())
      this->setNodeStringProperty(mxconst::get_ELEMENT_END_LEG_MESSAGE(), value); //, this->node, mxconst::get_ELEMENT_LEG());

    Utils::addElementToMap(this->mapFlightLeg_sub_nodes_ptr, mxconst::get_ELEMENT_END_LEG_MESSAGE(), xEndLegMessage);
  }


  xPreLegScript = this->node.getChildNode(mxconst::get_ELEMENT_PRE_LEG_SCRIPT().c_str()); // v3.0.221.15rc5 add LEG support

  if (!xPreLegScript.isEmpty())
  {
    std::string value = Utils::readAttrib(xPreLegScript, mxconst::get_ATTRIB_NAME(), "");
    if (!value.empty())
      this->setNodeStringProperty(mxconst::get_ELEMENT_PRE_LEG_SCRIPT(), value); //, this->node, mxconst::get_ELEMENT_LEG());

    Utils::addElementToMap(this->mapFlightLeg_sub_nodes_ptr, mxconst::get_ELEMENT_PRE_LEG_SCRIPT(), xPreLegScript);
  }


  xPostLegScript = this->node.getChildNode(mxconst::get_ELEMENT_POST_LEG_SCRIPT().c_str()); // v3.0.221.15rc5 add LEG support

  if (!xPostLegScript.isEmpty())
  {
    std::string value = Utils::readAttrib(xPostLegScript, mxconst::get_ATTRIB_NAME(), "");
    if (!value.empty())
      this->setNodeStringProperty(mxconst::get_ELEMENT_POST_LEG_SCRIPT(), value); //, this->node, mxconst::get_ELEMENT_LEG());

    Utils::addElementToMap(this->mapFlightLeg_sub_nodes_ptr, mxconst::get_ELEMENT_POST_LEG_SCRIPT(), xPostLegScript);
  }
  // end v3.0.207.5 goal extensions

  // v3.0.224.2 draw script
  IXMLNode xDrawScript = this->node.getChildNode(mxconst::get_ELEMENT_DRAW_SCRIPT().c_str());
  if (!xDrawScript.isEmpty())
  {
    std::string script_name = Utils::readAttrib(xDrawScript, mxconst::get_ATTRIB_NAME(), "");
    if (!script_name.empty())
      this->setNodeStringProperty(mxconst::get_ELEMENT_DRAW_SCRIPT(), script_name); //, this->node, mxconst::get_ELEMENT_LEG());

    Utils::addElementToMap(this->mapFlightLeg_sub_nodes_ptr, mxconst::get_ELEMENT_DRAW_SCRIPT(), xDrawScript);
  }

  // v3.0.224.3 add leg_message
  int nLegMessages = this->node.nChildNode(mxconst::get_DYNAMIC_MESSAGE().c_str());
  for (int i1 = 0; i1 < nLegMessages; ++i1)
  {
    IXMLNode xLegDynamicMessageNode = this->node.getChildNode(mxconst::get_DYNAMIC_MESSAGE().c_str(), i1).deepCopy();

    this->list_raw_dynamic_messages_in_leg.push_back(xLegDynamicMessageNode); // we add the raw IXMLNode to the list. We do not parse it yet.
  }

  //// READ 2D Maps and Information
  this->map2DMapsNodes.clear(); // Map information code was DEPRECATED, will read during missionx::readCurrentMissionTextures() function

  //// Metar Element ////
  // METAR file
  IXMLNode xMetar = this->node.getChildNode(mxconst::get_ELEMENT_METAR().c_str());
  if (!xMetar.isEmpty())
  {
    // Metar filename
    this->setNodeStringProperty(mxconst::get_ATTRIB_METAR_FILE_NAME(), Utils::readAttrib(xMetar, mxconst::get_ATTRIB_METAR_FILE_NAME().c_str(), "")); //, this->node, mxconst::get_ELEMENT_LEG());
    this->setNodeProperty<bool>(mxconst::get_ATTRIB_FORCE_CUSTOM_METAR_FILE(), Utils::readBoolAttrib(xMetar, mxconst::get_ATTRIB_FORCE_CUSTOM_METAR_FILE(), true)); //, this->node, mxconst::get_ELEMENT_LEG());

    Utils::addElementToMap(this->mapFlightLeg_sub_nodes_ptr, mxconst::get_ELEMENT_METAR(), xMetar);
  }


  // v3.0.221.8 store the goal XML special element attribute or the goal element itself
  // v3.0.221.15rc5 add LEG support
  if (this->node.nChildNode(mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES().c_str()) > 0)
  {
    this->xmlSpecialDirectives_ptr = this->node.getChildNode(mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES().c_str());
  }
  else
  {
    this->xmlSpecialDirectives_ptr = this->node.addChild(mxconst::get_ELEMENT_SPECIAL_LEG_DIRECTIVES().c_str());

    Utils::xml_copy_node_attributes(this->node, this->xmlSpecialDirectives_ptr);
  }

  if (!flag_isDummy) // v3.303.12 skip if <leg> is dummy - no GPS and Fail Timer
  {
    this->xGPS = this->node.getChildNode(mxconst::get_ELEMENT_GPS().c_str()); // v3.0.253.7

    // v3.0.253.7 Fail Timer
    if (this->node.nChildNode(mxconst::get_ELEMENT_TIMER().c_str()) > 0)
    {
      missionx::Timer failTimer;
      failTimer.node = this->node.getChildNode(mxconst::get_ELEMENT_TIMER().c_str());
      if (failTimer.parse_node())
      {
        Utils::addElementToMap(this->mapFailTimers, failTimer.getName(), failTimer);
      }
    }
  }

  return true;
}

// -----------------------------------

bool
missionx::Waypoint::getIsDummyLeg()
{
  return Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_IS_DUMMY(), false);
}

// -----------------------------------

bool
missionx::Waypoint::getIsComplete()
{
  return this->isComplete;
}

// -----------------------------------

bool
missionx::Waypoint::getIsFreeFromRunningMessages()
{
  return this->isCleanFromRunningMessages;
}

// -----------------------------------

void
missionx::Waypoint::setIsFreeFromRunningMessages(bool inVal)
{
  this->isCleanFromRunningMessages = inVal;
}

// -----------------------------------

missionx::enums::mx_flightLeg_state
missionx::Waypoint::getFlightLegState()
{
  return this->goal_state;
}

// -----------------------------------


void
missionx::Waypoint::setIsComplete(bool inIsComplete, missionx::enums::mx_flightLeg_state inState)
{
  this->isComplete = inIsComplete;
  this->goal_state = inState;

  this->setNodeProperty<bool>(mxconst::get_PROP_IS_COMPLETE(), inIsComplete); // this->node, mxconst::get_ELEMENT_LEG());   // v3.0.241.1
  this->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)inState); //, this->node, mxconst::get_ELEMENT_LEG()); // v3.0.241.1 TODO: decide if it is really needed
}

// -----------------------------------


void
missionx::Waypoint::init()
{
  isValid      = true;
  hasMandatory = false;
  isComplete   = false;
  goal_state   = missionx::enums::mx_flightLeg_state::leg_undefined;
  errMsg.clear();

  listObjectivesInFlightLeg.clear();
  listTriggers.clear();
  listTriggersByDistance.clear(); // v3.305.2
  listTriggersOthers.clear(); // v3.305.2
  listTriggersByDisatnce_thread.clear(); // v3.305.2
  xml_listObjectivesInGoal_ptr.clear();
  xml_listTriggers.clear();
  vecSentences.clear();

  list_displayInstances.clear(); // v3.305.1 deprecated not in use
  map2DMapsNodes.clear(); // v3.0.241.7.1

  listCueInfoToDisplayInFlightLeg.clear(); // v3.0.203

  xmlSpecialDirectives_ptr = IXMLNode::emptyIXMLNode; // v3.0.221.7

  listCommandsAtTheStartOfFlightLeg.clear(); // v3.0.221.9
  listCommandsAtTheEndOfTheFlightLeg.clear();        // v3.0.221.9

  this->list_raw_dynamic_messages_in_leg.clear(); // v3.0.223.4
}

// -----------------------------------

std::string
missionx::Waypoint::to_string()
{
  std::string format;
  format.clear();

  format              = "Flight Leg: " + mxconst::get_QM() + this->getName () + mxconst::get_QM() + mxconst::get_UNIX_EOL();
  const size_t length = format.length();
  format += std::string("").append(length, '=') + "\n ";

  format += "Objectives in Leg: ";
  for (const auto& o : this->listObjectivesInFlightLeg)
    format += fmt::format(R"("{}",)", o );

  format += mxconst::get_UNIX_EOL() + "Triggers in Leg: ";
  for (auto t : this->listTriggers)
    format += fmt::format (R"("{}",)", t);


  format += "\n" + Utils::xml_get_node_content_as_text(this->node) + "\n";

  return format;
}

// -----------------------------------

std::string
missionx::Waypoint::to_string_ui_leg_info()
{
  return "Leg Name: " + this->getName() + (!this->getIsDummyLeg()? "" : "\t( Is dummy )" );
}

// -----------------------------------


void
missionx::Waypoint::storeCoreAttribAsProperties()
{
  // v3.0.241.1 use node
  this->setNodeProperty<bool>(mxconst::get_PROP_IS_VALID(), this->isValid); //, this->node, mxconst::get_ELEMENT_LEG());
  this->setNodeProperty<bool>(mxconst::get_PROP_IS_COMPLETE(), this->isComplete); //, this->node, mxconst::get_ELEMENT_LEG());
  this->setNodeProperty<bool>(mxconst::get_PROP_HAS_MANDATORY(), this->hasMandatory); //, this->node, mxconst::get_ELEMENT_LEG());

  // we do not store "isCompleteSuccess" since only complete success or not completed Legs can be saved to disk.
  // For saved "isComplete" Legs, we assume "complete success" else "undefined" = "-1"
}

// -----------------------------------

void
missionx::Waypoint::saveCheckpoint(IXMLNode& inParent)
{
  storeCoreAttribAsProperties();

  this->setNodeProperty<bool>(mxconst::get_PROP_LOADED_FROM_CHECKPOINT(), this->hasMandatory); //, this->node, mxconst::get_ELEMENT_LEG()); // we just need to test if element is present to know information was read from save file.

  inParent.addChild(this->node.deepCopy()); // v3.0.241.1
}

// -----------------------------------



void
missionx::Waypoint::applyPropertiesToGoal()
{
  std::string err;
  err.clear();

  this->isValid      = this->getAttribNumericValue<bool>(mxconst::get_PROP_IS_VALID(), false, err);
  this->isComplete   = this->getAttribNumericValue<bool>(mxconst::get_PROP_IS_COMPLETE(), false, err);
  this->hasMandatory = this->getAttribNumericValue<bool>(mxconst::get_PROP_HAS_MANDATORY(), false, err);

  const std::string descText = this->getNodeStringProperty(mxconst::get_ELEMENT_DESC(), "", true); // v3.303.11 try to fetch text from map, if not then try to read from element attribute

  vecSentences = Utils::sentenceTokenizerWithBoundaries(descText, mxconst::get_SPACE(), ((missionx::mxvr::vr_display_missionx_in_vr_mode) ? missionx::MAX_CHARS_IN_BRIEFER_LINE_3D : missionx::MAX_CHARS_IN_BRIEFER_LINE_2D), ";"); // v3.0.221.7 added VR consideration
  // VR consideration
}

// -----------------------------------

// -----------------------------------

// -----------------------------------

