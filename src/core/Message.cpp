#include "Message.h"
#include "../io/system_actions.h"
#include <fmt/core.h>

namespace missionx
{

std::unordered_map<std::string, missionx::mxTextureFile> Message::mapStoryCachedImages;
std::vector<missionx::mxTextureFile*>                    Message::vecStoryCurrentImages_p { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }; // Make sure to reserve: Message::VEC_STORY_IMAGE_SIZE_I elements, in vector
missionx::mx_line_action_strct                           Message::lineAction4ui;

}

// -----------------------------------

missionx::Message::Message(): mx_base_node()
{
  init();
}

// -----------------------------------

missionx::Message::Message(std::string inMsgName, std::string msgText)
{
  init(); // v3.0.241.2 already in Message::Message()

  this->setName(inMsgName);
  this->setMessageText(msgText); // v3.0.211.1

  this->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)missionx::mx_message_state_enum::msg_not_broadcasted); // important for QMM

  this->applyPropertiesToLocal();
}


// -----------------------------------


missionx::Message::Message(std::string inMsgName, std::string msgText, std::string inTrackName, std::string muteNarator, std::string hideText, std::string inEnabled, std::string inOverrideDisplayTextSecond)
{
  std::string err;
  init();

  this->setName(inMsgName);
  this->setMessageText(msgText); // v3.0.211.1
  this->setTrackedName(inTrackName);

  bool out_val_b = false;
  Utils::isStringBool(muteNarator, out_val_b);
  this->setNodeProperty<bool>(mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR(), out_val_b);

  Utils::isStringBool(hideText, out_val_b);
  this->setNodeProperty<bool>(mxconst::get_ATTRIB_MESSAGE_HIDE_TEXT(), out_val_b);

  Utils::isStringBool(inEnabled, out_val_b);
  this->setNodeProperty<bool>(mxconst::get_ATTRIB_ENABLED(), out_val_b); // do we really need this ?

  this->setNodeStringProperty(mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT(), inOverrideDisplayTextSecond);

  this->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)missionx::mx_message_state_enum::msg_not_broadcasted);

  if (!msgText.empty()) // v3.0.161 fix bug where PROP_MESSAGE_HAS_TEXT_TRACK neverset and QMM fail to call "speakString"
    this->setNodeProperty<bool>(mxconst::get_PROP_MESSAGE_HAS_TEXT_TRACK(), true);

  this->applyPropertiesToLocal();
}



// -----------------------------------
// -----------------------------------
// -----------------------------------

void
missionx::Message::setName(std::string inName)
{
  this->name = inName;
  this->setNodeStringProperty(mxconst::get_ATTRIB_NAME(), inName);
}

// -----------------------------------

void
missionx::Message::setTrackedName(std::string inTrackedName)
{
  this->setNodeStringProperty(mxconst::get_PROP_MESSAGE_TRACK_NAME(), inTrackedName); // v3.0.241.8
  this->trackName = inTrackedName;                                              // v3.0.241.8
}

// -----------------------------------

bool
missionx::Message::parse_action(char action, std::string inLine)
{

  int pos = 3; // 3 is just after [i], but there should have been a space after it, although we do not force it.

  // let's assume that parsing will work and change its state to failure if it does fail.
  Message::lineAction4ui.isReady = true;
  Message::lineAction4ui.state   = missionx::enum_mx_line_state::parsing_ok;
  Message::lineAction4ui.actionCode = action;

  switch (action)
  {
    case mxconst::STORY_ACTION_IMG: // load image
    {
      // get position to display
      missionx::mx_fetch_info info = mxUtils::fetch_next_string(inLine, pos, " ");
      Message::lineAction4ui.vals[mxconst::get_STORY_IMAGE_POS()] = mxUtils::stringToLower( mxUtils::trim(info.token) ); // image position always to lower case

      // get file name
      info = mxUtils::fetch_next_string(inLine, info.lastPos, "\n"); // fetch string until end of line
      Message::lineAction4ui.vals[mxconst::get_ATTRIB_NAME()] = mxUtils::stringToLower(mxUtils::remove_quotes(mxUtils::trim(info.token))); // trim, remove start and end quotes and convert to lowercase
    }
    break;
    case mxconst::STORY_ACTION_TEXT: // text
    case mxconst::STORY_ACTION_MSGPAD: // msg
    {
      pos = 0;    

      missionx::mx_fetch_info info      = mxUtils::fetch_next_string(inLine, pos, " ");
      info.token                        = mxUtils::stringToLower(mxUtils::trim(info.token));

      // Check for special command per line. ">","<"
      // Example: ">t good morning", will print the text without punctuation timer.
      if (info.token.max_size() > (size_t)1)
      {
        switch (info.token.front())
        {
          case '>':
          {
            Message::lineAction4ui.bIgnorePunctuationTiming = true;
            info.token.erase(0,1); // consume the punctuation            
          }
          break;
          case '<':
          {
            Message::lineAction4ui.bIgnorePunctuationTiming = false;
            info.token.erase(0,1); // consume the punctuation
          }
          break;
        }

        // v3.305.3 handle cases where line= ">[m]n"
        info.token = mxUtils::trim(info.token);
        const std::string sAction = fmt::format("[{}]",action);

        //auto iPos = info.token.find(sAction); // debug
        if (info.token.find(sAction) == 0)
        {
          info.token.erase(0, 3);
          info.token = mxUtils::trim(info.token);
        }
      }

      // First Token - character name
      if (mxUtils::isElementExists(Message::mapCharacters, info.token) )
      {
        Message::lineAction4ui.vals[mxconst::get_STORY_CHARACTER_CODE()] = info.token;
        Message::lineAction4ui.characterInfo                       = this->mapCharacters[info.token];

      }
      else  // use default character data
      {
        // define new N/A character = undefined one, with label: n/a and color white (the color we do not need to set, it is white in default - mxRGB)
        if (!mxUtils::isElementExists(this->mapCharacters, mxconst::get_STORY_DEFAULT_TITLE_NA()))
        {
          this->mapCharacters[mxconst::get_STORY_DEFAULT_TITLE_NA()].code  = mxconst::get_STORY_DEFAULT_TITLE_NA();
          this->mapCharacters[mxconst::get_STORY_DEFAULT_TITLE_NA()].label = mxconst::get_STORY_DEFAULT_TITLE_NA();
          Message::lineAction4ui.vals[mxconst::get_STORY_CHARACTER_CODE()] = mxconst::get_STORY_DEFAULT_TITLE_NA();
        }

        Message::lineAction4ui.characterInfo = this->mapCharacters[mxconst::get_STORY_DEFAULT_TITLE_NA()];
      }


      // Second Token - Text
      // Read until the end of the line and not just "\n"
      Message::lineAction4ui.vals[mxconst::get_STORY_TEXT()] = mxUtils::remove_quotes(mxUtils::trim(inLine.substr(info.lastPos) ) ); // trim and remove start and end quotes

      // replace pilot name keyword
      std::string pilotName = Utils::getNodeText_type_6(system_actions::pluginSetupOptions.node, mxconst::SETUP_PILOT_NAME, mxconst::DEFAULT_SETUP_PILOT_NAME);
      if (mxUtils::trim( pilotName).empty() ) // make sure the name is not empty
        pilotName = mxconst::DEFAULT_SETUP_PILOT_NAME;

      // replace all %pilot% with the pilotName value.
      Message::lineAction4ui.vals[mxconst::get_STORY_TEXT()] = mxUtils::replaceAll(Message::lineAction4ui.vals[mxconst::get_STORY_TEXT()], mxconst::REPLACE_KEYWORD_PILOT_NAME, pilotName);

    }
    break;
    case mxconst::STORY_ACTION_PAUSE:
    {
      missionx::mx_fetch_info info = mxUtils::fetch_next_string(inLine, pos, " ");
      info.token                   = mxUtils::trim(info.token);

      if (mxUtils::is_digits(info.token))
      {
        if (mxUtils::stringToNumber<int>(info.token) > Message::MAX_PAUSE_TIME_SEC || mxUtils::stringToNumber<int>(info.token) < 1) // restrict the pause to no more than 60 seconds (1 minute)
          Message::lineAction4ui.vals[mxconst::get_STORY_PAUSE_TIME()] = ""; // use default pause time
        else
          Message::lineAction4ui.vals[mxconst::get_STORY_PAUSE_TIME()] = info.token;
      }
      else 
        Message::lineAction4ui.vals[mxconst::get_STORY_PAUSE_TIME()] = ""; // empty string means use default pause time defined by the plugin.

    }
    break;
    case mxconst::STORY_ACTION_HIDE: // hide main window
    {
      // does nothing, we only call the hide as the last row of the message
    }
    break;

    default:
    {
      Message::lineAction4ui.isReady = false;
      Message::lineAction4ui.state = missionx::enum_mx_line_state::parsing_failure;
    }
    break;
  }

  return Message::lineAction4ui.isReady;
}

// -----------------------------------

void
missionx::Message::set_mxpad_properties(std::string inLabel, std::string inLabelPlace, std::string inColor, std::string inImage)
{
  this->setNodeStringProperty(mxconst::get_ATTRIB_LABEL(), inLabel); 
  this->setNodeStringProperty(mxconst::get_ATTRIB_LABEL_PLACEMENT(), inLabelPlace); 
  this->setNodeStringProperty(mxconst::get_ATTRIB_LABEL_COLOR(), inColor);  // we should convert to RGB using PROP_TEXT_RGB_COLOR
  this->setNodeStringProperty(mxconst::get_PROP_IMAGE_FILE_NAME(), inImage); 

  this->applyPropertiesToLocal();
}

// -----------------------------------

//missionx::Message::~Message() {}

// -----------------------------------
std::unordered_map<std::string, mx_character>
missionx::Message::parseCharacterAttribute()
{
  std::unordered_map<std::string, mx_character> mapResult;

  std::string characters_s = Utils::readAttrib(this->xml_nodeTextTrack_ptr, mxconst::get_ATTRIB_CHARACTERS(), "");

  // split ","
  auto vecAllCharacters = mxUtils::split_v2(characters_s, ",");
  for (const auto& person_s : vecAllCharacters)
  {
    mx_character characterInfo;
    auto vecCharacter = mxUtils::split_v2(person_s, "|");
    int         iCounter     = 0;
    for (const auto& val : vecCharacter)
    {
      switch (iCounter)
      {
        case 0:
          characterInfo.code = val;
          break;
        case 1:
          characterInfo.label = val;
          break;
        case 2:
          characterInfo.color = mxUtils::hexToNormalizedRgb(val);
          break;
          
      }; // end switch

      iCounter++;
    }

    if (iCounter < 2)
    {
      Log::logMsgErr("Message: " + this->getName() + ", has invalid character definition: " + person_s);
    }
    else
      mapResult[characterInfo.code] = characterInfo;

  }



  return mapResult;
}

// -----------------------------------

std::deque<std::string>
missionx::Message::parseStoryMessage()
{
  std::deque<std::string> result;
  std::stringstream buffer;
  auto text = Utils::xml_read_cdata_node(this->xml_nodeTextTrack_ptr, "");
  buffer << text;
  std::string line;

  while (std::getline(buffer, line))
  {
    auto trimLine = mxUtils::trim(line);
    if (!trimLine.empty()) // skip empty lines
      result.emplace_back(mxUtils::trim(line));
  }

  return result;
}
// -----------------------------------

bool
missionx::Message::parse_node(const bool inConsumeWarnings)
{
  bool flag_b = true;

  assert(!this->node.isEmpty());

  this->mapChannels.clear();
  this->xml_nodeTextTrack_ptr = IXMLNode();

  std::string mName = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");

// v3.305.3 no need, always MXPAD
//#ifndef RELEASE
//  std::string mIsMxPad = Utils::readAttrib(this->node, mxconst::get_PROP_IS_MXPAD_MESSAGE(), mxconst::get_MX_YES());
//#endif

  // validation and setup
  if (mName.empty())
  {
    Log::logMsgErr("Found message with no name. Please fix it. Skipping...");
    return false;
  }


  this->setName(mName);

  const std::string mode_s = mxUtils::stringToLower(Utils::readAttrib(this->node, mxconst::get_ATTRIB_MODE(), "")); // v3.305.1
  if (mode_s.compare(mxconst::get_MESSAGE_MODE_STORY()) == 0)
    this->mode = missionx::mx_msg_mode::mode_story;

  // v3.0.241.1 moved special attribute code handling into node level: <message>

  int mixCount = this->node.nChildNode(mxconst::get_ELEMENT_MIX().c_str());
  for (int i3 = 0; i3 < mixCount; i3++)
  {
    // implement CHANNELS
    auto xMix = this->node.getChildNode(mxconst::get_ELEMENT_MIX().c_str(), i3);

    if (xMix.isEmpty())
      continue;

    std::string mChannel = mxUtils::stringToLower(Utils::readAttrib(xMix, mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), ""));
    if (mChannel.empty() && !inConsumeWarnings) // v3.0.213.5
    {
      #ifndef RELEASE
      Log::logMsgErr("Message: " + mxconst::get_QM() + mName + mxconst::get_QM() + ", has a mixture with missing \"track type\". Please fix. Skipping mixture channel...");
      #endif 
      continue;
    }

    if (mxconst::get_CHANNEL_TYPE_TEXT().compare(mChannel) == 0)
    { // Text is stored in the Message class
      this->xml_nodeTextTrack_ptr = xMix;

      if (this->mode == missionx::mx_msg_mode::mode_story)
      {
        this->xml_nodeTextTrack_ptr.updateAttribute(mxconst::get_MX_TRUE().c_str(), mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR().c_str(), mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR().c_str());
      }

      // store mandatory attributes/information as properties for later use like "the text message"
      std::set<std::string> exceptionAttributeSet = { mxconst::get_ATTRIB_NAME(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxconst::get_ATTRIB_MODE() }; // v3.305.1 added exclude "mode" attribute.
      Utils::xml_copy_node_attributes_excluding_black_list(xMix, this->node, &exceptionAttributeSet); // copy all text mix to the main message node.
      this->setNodeProperty<bool>(mxconst::get_PROP_MESSAGE_HAS_TEXT_TRACK(), true);                        // internal indicator that we read a TEXT channel. This is important if to display empty text or not for "messages" without "text" channel.

      std::string mMessage = ((xMix.nClear() > 0) ? xMix.getClear().sValue : "");
      this->setMessage(mMessage); // will store in mx_base_node::mapText{} container

      std::string mLabelPosition = Utils::readAttrib(xMix, mxconst::get_ATTRIB_LABEL_PLACEMENT(), "L", true); // v3.0.197
      mLabelPosition             = mxUtils::translateMxPadLabelPositionToValid(mLabelPosition);
      this->setNodeStringProperty(mxconst::get_ATTRIB_LABEL_PLACEMENT(), mLabelPosition); // update main node
      this->setNodeStringProperty(mxconst::get_ATTRIB_LABEL_PLACEMENT(), mLabelPosition, xMix); // update mix element

      if (this->mode == mx_msg_mode::mode_story)  // v3.305.1
      {
        this->mapCharacters = parseCharacterAttribute();
        this->dqMsgLines    = parseStoryMessage();
      }

      this->applyPropertiesToLocal();
    }
    else
    {                             // Sound channels are stored in SoundFragment class, they need less attributes since they only represent sound files to play

      // skip the "comm" channel in story mode
      if (this->mode == missionx::mx_msg_mode::mode_story && mChannel.compare(mxconst::get_CHANNEL_TYPE_COMM()) == 0)
        continue; 


      missionx::SoundFragment sf; // sound fragment, holds sound file info
      sf.node = xMix;             // v3.0.241.1

      std::string mSoundFile             = Utils::readAttrib(xMix, mxconst::get_ATTRIB_SOUND_FILE(), "");
      std::string mSoundVol              = Utils::readAttrib(xMix, mxconst::get_ATTRIB_SOUND_VOL(), "0");
      std::string mOverrideSecondsToPlay = Utils::readAttrib(xMix, mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_PLAY(), "0"); // if zero, then plugin will try to calculate length based on text length

      // validate if defined sound file name
      if (mSoundFile.empty() && !inConsumeWarnings)
      {
        #ifndef RELEASE
        Log::logMsg("[Warning] Message: " + mName + ", has a '<mix>' with no sound file. skipping this channel...");
        #endif 
        continue; // v3.0.241.9 this should not invalidate the message
      }

      // Convert and validate volume to be between 0 and 100 !
      unsigned int mixVolume = (mxUtils::is_number(mSoundVol)) ? mxUtils::stringToNumber<unsigned int>(mSoundVol) : 30;
      if (mixVolume < 1 || mixVolume > 100)
        mixVolume = 30;


      sf.setNodeStringProperty(mxconst::get_ATTRIB_ORIGINATE_MESSAGE_NAME(), mName);  // v3.0.241.1 added node support // v3.0.223.7 store originate message name. will be used with background channels


      sf.setStringProperty(mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mChannel);
      sf.setStringProperty(mxconst::get_ATTRIB_SOUND_FILE(), mSoundFile);
      sf.setNodeProperty<int>(mxconst::get_ATTRIB_SOUND_VOL(), (int)mixVolume);  // set volume // v3.0.241.1 added node support
      sf.setNodeProperty<int>(mxconst::get_ATTRIB_ORIGINAL_SOUND_VOL(), (int)mixVolume); // v3.306.1b store original sound for future reference
      sf.setNumberProperty(mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_PLAY(), mOverrideSecondsToPlay);

      sf.setNodeStringProperty(mxconst::get_ATTRIB_PARENT_MESSAGE(), mName);  // v3.0.105 // holds the message name the channel was created for.

      // v3.0.303.6 parse ATTRIB_TRACK_INSTRUCTIONS
      if (mxconst::get_CHANNEL_TYPE_BACKGROUND().compare(mChannel) == 0)
      {
        // v3.306.1c deprecated
        //sf.timer_type = mxUtils::translate_timer_time_to_enum( sf.getStringAttributeValue(mxconst::get_ATTRIB_TIMER_TYPE(), mxconst::get_PROP_TIMER_TYPE_XP()) );

        const std::string sInstructions = Utils::readAttrib(xMix, mxconst::get_ATTRIB_TRACK_INSTRUCTIONS(), "");
        // parse string track_instructions="sec|+|40|0,sec|-|30|10,sec|!|0|0"
        std::vector<std::string> vecTrackInstructions = mxUtils::split_v2(sInstructions, ",");
        // loop over all directives and parse each directive action to a map
        for (const auto& s : vecTrackInstructions)
        {
          std::vector<std::string> vecCommands = mxUtils::split_v2(s, "|");
          // store the command
          missionx::mx_track_instructions_strct strctTrackInstructions;

          char last_command = '\0';

          for (size_t i1 = 0; i1 < vecCommands.size(); ++i1)
          {
            switch (i1)
            {
              case 0: // when to start
                strctTrackInstructions.seconds_to_start_f = mxUtils::stringToNumber<float>(vecCommands.at(i1), vecCommands.at(i1).length());
                break;
              case 1: // command
                strctTrackInstructions.command = vecCommands.at(i1).front(); // get first character as the command
                break;
              case 2: // new volume
                if (strctTrackInstructions.command == 'l' || strctTrackInstructions.command == 'L')
                {
                  // We are forcing that the value wont be zero or less than zero, we will force repeat only once
                  strctTrackInstructions.loopFor_i = (mxUtils::stringToNumber<int>(vecCommands.at(i1)) <= 0)? 1 : mxUtils::stringToNumber<int>(vecCommands.at(i1)) ;                  
                }
                else 
                  strctTrackInstructions.new_volume = mxUtils::stringToNumber<int>(vecCommands.at(i1));

                break;
              case 3: // transition time
                strctTrackInstructions.transition_time_f = mxUtils::stringToNumber<float>(vecCommands.at(i1), vecCommands.at(i1).length());
                break;
              default:
                break;
            }
          }

          if (vecCommands.size() < 2)
          {
            Log::logMsg("[error] skipping sound instruction: " + s + ", in message: " + this->getName());
            continue;
          }
          else
          {
            this->qSoundCommands.push(strctTrackInstructions);
          }
        } // end loop over all directives

        // v3.306.1 removed ATTRIB_TRACK_INSTRUCTIONS holding mName since it should hold the background <mix> directive string. It also screws the save file
        //sf.setNodeStringProperty(mxconst::get_ATTRIB_TRACK_INSTRUCTIONS(), mName);  // v3.0.105 // holds the message name the channel was created for.

        // Adding auto "stop" at the end could have been a good idea, unless we use "fade_bg_channel" or "post_script" to handle the way we want to stop the background sound file.
        //if (!this->qSoundCommands.empty())
        //{
        //  auto        cmd = this->qSoundCommands.back();
        //  std::string sCmd = mxUtils::stringToLower( std::string("") + cmd.command);
        //  if (!(sCmd.compare("s") == 0))
        //    this->addStopInstructionCommand();
        //}

        sf.setInstructionsQueue(this->qSoundCommands);
      }        

      sf.applyPropertiesToLocal();

      Utils::addElementToMap(this->mapChannels, sf.getName(), sf); // v3.0.241.1
    }
  }

  return flag_b;
}

// -----------------------------------

void
missionx::Message::init()
{
  this->node.updateName(mxconst::get_ELEMENT_MESSAGE ().c_str()); // v3.0.241.8

  //is_pad = false;
  this->mode = mx_msg_mode::mode_default; // v3.305.1
  this->setNumberProperty<int8_t>(mxconst::get_ATTRIB_MODE(), (int8_t)this->mode);

  this->name.clear();
  this->setStringProperty(mxconst::get_ATTRIB_MESSAGE(), "");
  this->setTrackedName(""); // v3.0.241.8

  this->setBoolProperty(mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR(), false);
  this->setBoolProperty(mxconst::get_ATTRIB_MESSAGE_HIDE_TEXT(), false);
  this->setBoolProperty(mxconst::get_ATTRIB_ENABLED(), true); // do we really need this ?

  this->setNumberProperty<int>(mxconst::get_PROP_COUNTER(), 0);
  this->setNumberProperty<int>(mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT(), 0);
  this->setNumberProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)missionx::mx_message_state_enum::msg_undefined);

  // v3.0.109 // internal indicator that we read a TEXT channel. This is important if to display empty text or not for "messages" without "text" channel. If we read "text" channel even if its message is empty, we still call
  // "XPLMSpeakString" with empty string.
  this->setBoolProperty(mxconst::get_PROP_MESSAGE_HAS_TEXT_TRACK(), false);
  this->setBoolProperty(mxconst::get_PROP_IS_MXPAD_MESSAGE(), false); // v3.0.109

  this->applyPropertiesToLocal();


  this->broadcastFor.clear();

  errMsg.clear();

  if (!mapChannels.empty())
    mapChannels.clear();

}

// -----------------------------------

void
missionx::Message::reset()
{
  init();
  storeCoreAttribAsProperties();
}

// -----------------------------------

std::string
missionx::Message::getMessage()
{
  std::string err;

  const std::string message = this->getNodeStringProperty(mxconst::get_ATTRIB_MESSAGE(), "", true); // v3.303.1, the function will try to find the "text" in the the "mapText" container and if it is not there it will try to fetch it from the element.
  if (!err.empty())
    Log::logMsg(err);

  return message;
}

// -----------------------------------

void
missionx::Message::setMessage(std::string inMsg)
{
  this->setStringProperty(mxconst::get_ATTRIB_MESSAGE(), inMsg, false); // v3.303.11 Changed from ELEMENT_MESSAGE to ATTRIB_MESSAGE to be consistent with QMM
}

// -----------------------------------

void
missionx::Message::setMessage(Message& inMsg)
{
  this->setMessage(inMsg.getMessage());
}

// -----------------------------------

void
missionx::Message::flc()
{
  // Progress/Update Message State
  if (this->msgCounter == 0)
    this->msgState = missionx::mx_message_state_enum::msg_not_broadcasted;
  if (this->msgCounter == 1)
    this->msgState = missionx::mx_message_state_enum::msg_broadcast_once;
  else if (this->msgCounter > 1)
    this->msgState = missionx::mx_message_state_enum::msg_broadcast_few_times;

  this->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)this->msgState); 
}

// -----------------------------------

std::string
missionx::Message::to_string()
{

  std::string format = "\nMessage: " + mxconst::get_QM() + this->getName () + mxconst::get_QM() + ", Track Name: " + mxconst::get_QM() + this->trackName + mxconst::get_QM() + ", state: " + this->translateMessageState (this->msgState) + mxconst::get_UNIX_EOL();

  if (!this->mapChannels.empty())
  {
    format += "@@@ sound files @@@\n";
    for (auto sf : this->mapChannels)
    {
      format += sf.second.to_string();
    }
    format += "@@@ end sound info @@@\n";
  }


  format += "[[Message Properties]]:\n";
  format += "Used in: " + this->broadcastFor + ", called: " + Utils::formatNumber<int>(this->msgCounter) + "\n";
  format += Utils::xml_get_node_content_as_text(this->node); // display properties


  return format;
}

// -----------------------------------

std::string
missionx::Message::translateMessageState(missionx::mx_message_state_enum mState)
{
  switch (mState)
  {
    case mx_message_state_enum::msg_not_broadcasted:
      return "message not broadcast";
      break;
    case mx_message_state_enum::msg_broadcast_once:
      return "broadcast once";
      break;
    case mx_message_state_enum::msg_broadcast_few_times:
      return "broadcast few times";
      break;
    case mx_message_state_enum::msg_in_pool:
      return "message is in \"message pool\". ";
      break;
    case mx_message_state_enum::msg_text_is_ready:
      return "message text is ready to display";
      break;
    case mx_message_state_enum::msg_prepare_channels:
      return "message channels need to be prepared";
      break;
    case mx_message_state_enum::msg_channels_need_to_finish_loading:
      return "message channels are loading stream data...";
      break;
    case mx_message_state_enum::msg_is_ready_to_be_played:
      return "message text and stream are ready to be played";
      break;
    default:
      break;
  } // end switch

  return "undefined";
}

// -----------------------------------

std::string
missionx::Message::getChannelNameWithType(missionx::mx_message_channel_type_enum inType)
{
  std::string channelName;
  channelName.clear();

  for (auto sf : this->mapChannels)
  {
    if (inType == sf.second.channelType)
    {
      channelName = sf.second.getName();
      break;
    }
  }


  return channelName;
}

// -----------------------------------

bool
missionx::Message::releaseChannel(SoundFragment& sf, MxSound& sound)
{

  sound.stopAndReleaseChannel(sf);

  return true;
}

// -----------------------------------

bool
missionx::Message::removeChannel(std::string inName, MxSound& sound)
{
  if (mxUtils::isElementExists(mapChannels, inName))
  {
    releaseChannel(mapChannels[inName], sound); // v3.0.109

    mapChannels.erase(inName); // v3.0.109

    applyPropertiesToLocal();
  }

  return true;
}

// -----------------------------------

void
missionx::Message::storeCoreAttribAsProperties()
{

  this->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)this->msgState); 

  this->setNodeProperty<int>(mxconst::get_PROP_COUNTER(), this->msgCounter); 

  this->setNodeStringProperty(mxconst::get_PROP_MESSAGE_BROADCAST_FOR(), this->broadcastFor); 

  this->setTrackedName(this->trackName); // v3.0.241.8
}

// -----------------------------------

void
missionx::Message::applyPropertiesToLocal()
{
  assert(!this->node.isEmpty());

  std::string err;
  err.clear();

  // v3.0.96 fix bug when creating new message and these attributes are not initialized. The message always repeat itself
  if (this->node.getAttribute(mxconst::get_PROP_COUNTER().c_str()) == NULL)
    this->setNodeProperty<int>(mxconst::get_PROP_COUNTER(), 0); 

  if (this->node.getAttribute(mxconst::get_PROP_MESSAGE_BROADCAST_FOR().c_str()) == NULL)
    this->setNodeStringProperty(mxconst::get_PROP_MESSAGE_BROADCAST_FOR(), "");

  this->msgCounter = (int)Utils::readNumericAttrib(this->node, mxconst::get_PROP_COUNTER(), 0.0);

  // v3.0.241.1 deprecated, since already been done few lines back using the: this->setNodeStringProperty
  // read enum info
  this->msgState = (mx_message_state_enum)Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_PROP_STATE_ENUM(), (int)mx_message_state_enum::msg_undefined); // attribute is inititialized during runtime.
  this->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)this->msgState);

  this->setNodeProperty<bool>(mxconst::get_PROP_IS_MXPAD_MESSAGE(), true);
  //this->is_pad = true;

  this->setNodeStringProperty(mxconst::get_PROP_MESSAGE_TRACK_NAME(), this->trackName); // v3.0.241.8 adding tracked property for all messages, just in case
}

// -----------------------------------

void
missionx::Message::saveCheckpoint(IXMLNode& inParent)
{
  storeCoreAttribAsProperties();

  // v3.305.1 ugly workaround to force mode during save because I could not find where it was reset.
  // What I did find is if we save during the broadcasting of the "story message", the mode is retained, only when the message ends it loss the "mode" value.
  this->node.updateAttribute(((this->mode == missionx::mx_msg_mode::mode_default) ? "" : "story"), mxconst::get_ATTRIB_MODE().c_str(), mxconst::get_ATTRIB_MODE().c_str()); 

  inParent.addChild(this->node.deepCopy()); // v3.0.241.1

}

// -----------------------------------

// -----------------------------------


void
missionx::Message::stopTimer()
{
  this->msgTimer.stop();
}

// -----------------------------------


std::string
missionx::Message::get_and_filter_next_line(std::string& outAction, bool& outIsActionLine_b, int &outLineCounter)
{
  while (!this->dqMsgLines.empty())
  {
    bool        bStartsWithPunctuation  = false;

    std::string line  = this->dqMsgLines.front(); // fetch line
    std::string trimmedLine   = mxUtils::trim(line); 
    if (!trimmedLine.empty())
    {
      char firstChar = trimmedLine.front();
      for (const auto &c : mxconst::get_vecStoryPunctuation())
      {
        if (std::string(c).compare(std::string("") + (firstChar)) == 0)
        {
          bStartsWithPunctuation = true;
          break;
        }
      }
    }

    outIsActionLine_b = std::any_of(begin(mxconst::get_vecStoryActions()),
                                    end(mxconst::get_vecStoryActions()),
                                    [&](std::string inAction)
                                    { 
                                      outAction = inAction;

                                      if (bStartsWithPunctuation)
                                      {
                                        // find first position where there is a character and not a white space
                                        std::string sClearedLine = mxUtils::trim(trimmedLine.substr(1));
                                        #ifndef RELEASE
                                        bool bResult = (line.find(inAction) == 0);
                                        #endif 

                                        return (sClearedLine.find(inAction) == 0);
                                      }
                                      
                                      #ifndef RELEASE
                                      bool bResult = (line.find(inAction) == 0);
                                      #endif 

                                      return (line.find(inAction) == 0);
                                      
                                    });

    // if valid action, pop out
    this->dqMsgLines.pop_front();
    ++outLineCounter;

    // check if [h] action - hide is a special case to handle before it reaches the main logic code
    if (outIsActionLine_b && outAction.compare("[h]") == 0)
    {
      if (this->dqMsgLines.empty()) // if last line then return it since we will late convert the message to enum_mx_line_state::mainMessageEnded;
        return line;

      // consume line. We want to ignore all [h] actions that are not last in the message
      continue;
    }

    return line;
  } // end while loop

  return std::string("");

  
}


// -----------------------------------

//void
//missionx::Message::addStopInstructionCommand()
//{
//  missionx::mx_track_instructions_strct strctTrackInstructions;
//  strctTrackInstructions.seconds_to_start_f = 0.0f;
//  strctTrackInstructions.command            = 's';
//
//  this->qSoundCommands.push(strctTrackInstructions);
//}

// -----------------------------------
// -----------------------------------

missionx::mxProperties
missionx::Message::getInfoToSeed()
{
  missionx::mxProperties seedProperties;
  seedProperties.clear();

  // the key will reflect the name to seed
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_NAME(), this->getName());
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_MESSAGE(), this->getMessage());
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_MODE(), this->getStringAttributeValue(mxconst::get_ATTRIB_MODE(), "" ) ); // v3.305.3 empty value means default message.
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_PROP_NEXT_MSG(), this->getStringAttributeValue(mxconst::get_PROP_NEXT_MSG(), ""));           // v3.305.3
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_OPEN_CHOICE(), this->getStringAttributeValue(mxconst::get_ATTRIB_OPEN_CHOICE(), "")); // v3.305.3 empty value means default message.

  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_ENABLED(), Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_ENABLED(), true) );
  seedProperties.setIntProperty(mxconst::get_MX_() +  mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT(), Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT(), 0));



  return seedProperties;
}
