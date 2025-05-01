#include "QueueMessageManager.h"

namespace missionx
{
std::map<std::string, missionx::Message>       QueueMessageManager::mapTrackedMessages;
std::list<missionx::Message>                   QueueMessageManager::listPadQueueMessages;
std::map<std::string, missionx::SoundFragment> QueueMessageManager::mapPlayingBackgroundSF; // v3.0.223.7
std::list<missionx::Message>                   QueueMessageManager::listPoolMsg;

MxSound QueueMessageManager::sound;
Message QueueMessageManager::messagePad; // only for PAD messages

const float QueueMessageManager::SECONDS_PER_LINE = 5.0f;

std::deque<missionx::messageLine_strct> QueueMessageManager::mxpad_messages;            // mxpad should use LIFO  // v3.0.192 will hold mxpad text messages. We will try to store only last MAX_MX_PAD_MESSAGES messages
//std::deque<missionx::messageLine_strct> QueueMessageManager::mxpad_messages_in_briefer; // v3.0.221.7 mxpad should use LIFO  // v3.0.192 will hold mxpad text messages. We will try to store only last MAX_MX_PAD_MESSAGES messages
bool QueueMessageManager::flag_message_is_broadcasting;

bool QueueMessageManager::soundChannelsArePaused; // v3.0.207.3 optimization flag

bool                   QueueMessageManager::muteSound = false; // v3.0.223.1 change to static
std::string            QueueMessageManager::errMsg    = "";
std::list<std::string> QueueMessageManager::listEraseKeys;
std::list<std::string> QueueMessageManager::listEraseBackgroundKeys;

int QueueMessageManager::msgSeq;

}


// -----------------------------------


missionx::QueueMessageManager::QueueMessageManager() {}

// -----------------------------------

//missionx::QueueMessageManager::~QueueMessageManager() {}

// -----------------------------------

void
missionx::QueueMessageManager::initStatic()
{
  if (!QueueMessageManager::mapTrackedMessages.empty())
    QueueMessageManager::mapTrackedMessages.clear();

  if (!QueueMessageManager::listPadQueueMessages.empty())
  {
    QueueMessageManager::listPadQueueMessages.clear();
  }

  if (!QueueMessageManager::mapPlayingBackgroundSF.empty())
  {
    QueueMessageManager::mapPlayingBackgroundSF.clear();
  }

  if (!QueueMessageManager::listPoolMsg.empty())
  {
    QueueMessageManager::stopAllPoolChannels();
    QueueMessageManager::listPoolMsg.clear();
  }

  QueueMessageManager::sound.release();

  QueueMessageManager::messagePad.init();

  if (!QueueMessageManager::mxpad_messages.empty()) // v3.0.192
    QueueMessageManager::mxpad_messages.clear();

  //if (!QueueMessageManager::mxpad_messages_in_briefer.empty()) // v3.0.221.7
  //  QueueMessageManager::mxpad_messages_in_briefer.clear();

  QueueMessageManager::flag_message_is_broadcasting = false; // v3.0.209.1

  QueueMessageManager::muteSound = false;               // v3.0.223.1 change to static
  QueueMessageManager::errMsg    = "";                  // v3.0.223.1 change to static
  QueueMessageManager::listEraseKeys.clear();           // v3.0.223.1 change to static
  QueueMessageManager::listEraseBackgroundKeys.clear(); // v3.0.223.7
  QueueMessageManager::msgSeq = 0;                      // v3.0.223.1 change to static
}


// -----------------------------------

bool
missionx::QueueMessageManager::is_queue_empty()
{
  if (listPoolMsg.empty() && listPadQueueMessages.empty() && !flag_message_is_broadcasting)
    return true;

  if (listPadQueueMessages.empty() && listPoolMsg.size() == 1) // v3.305.1b
  {
    auto msg = listPoolMsg.front();
    if (msg.mode == missionx::mx_msg_mode::mode_default && msg.getBoolValue(mxconst::get_ATTRIB_FALLTHROUGH_B(), false))
    {

      #ifndef RELEASE
        Log::logMsg("[qmm] Found Fallthrough attribute for message: " + msg.getName()); // debug
      #endif

      return true;
    }
  }


  return false;
}

// -----------------------------------

bool
missionx::QueueMessageManager::addMessageToQueue(const std::string& inMessageName, const std::string& inTrackedName, std::string& outErr)
{

  outErr.clear();

  if (!inMessageName.empty())
  {
    // Log::logMsg("Calling Message: " + inMessageName); // debug
    if (Utils::isElementExists(missionx::data_manager::mapMessages, inMessageName))
    {
      missionx::Message msg = missionx::data_manager::mapMessages[inMessageName]; // create a copy I believe
#ifndef RELEASE
      Log::logMsg("Added Message: " + msg.getName() + "\n" + msg.getMessage()); // debug
#endif

      if (!msg.trackName.empty() && Utils::isElementExists(QueueMessageManager::mapTrackedMessages, msg.trackName)) // v3.0.241.8 if message tracked name is set and it is already being tracked, then do not add message to queue.
      {
#ifndef RELEASE
        Log::logMsgWarn("Message: " + msg.getName() + ", won't be added to queue list since message track name: " + msg.trackName + ", was already set. Will ignore new track name: [" + inTrackedName + "] if exists.");
#endif // !RELEASE

        return false;
      }

      msg.setTrackedName(inTrackedName); // v3.0.241.8

      const bool flag_added = QueueMessageManager::addMessageToQueueList(msg, outErr); // v3.0.241.8 skipped addMessage. Done code internaly  //  QueueMessageManager::addMessage(msg, inTrackedName, outErr);
      if (!flag_added)
        return false;
    }
    else
    {
      outErr = "No Message by the name: " + mxconst::get_QM() + inMessageName + mxconst::get_QM() + " was found.";
      return false;
    }
  }


  return true;
}


// -----------------------------------

void
missionx::QueueMessageManager::addTextAsNewMessageToQueue(std::string msgName, std::string msgText, std::string inTrackName, bool muteNarator, bool hideText, bool inEnabled, int inOverrideDisplayTextSecond)
{
  if (msgText.empty())
    return; // skip action

  // construct a name if none given
  if (msgName.empty())
  {
    msgName = mxconst::get_QMM_DUMMY_MSG_NAME_PREFIX() + Utils::formatNumber<int>(++data_manager::msgSeq);
    inTrackName.clear();
  }

  if (!inTrackName.empty())
  {
    if (Utils::isElementExists(QueueMessageManager::mapTrackedMessages, inTrackName))
      return;
  }


  const std::string override_seconds_s = Utils::formatNumber<int>(inOverrideDisplayTextSecond);
  Message           msg(msgName, msgText, inTrackName, (muteNarator) ? mxconst::get_MX_YES() : mxconst::get_MX_NO(), (hideText) ? mxconst::get_MX_YES() : mxconst::get_MX_NO(), (inEnabled) ? mxconst::get_MX_YES() : mxconst::get_MX_NO(), override_seconds_s); // This should also set PROP_MESSAGE_HAS_TEXT_TRACK

  if (!msg.trackName.empty())
  {
    if (!Utils::isElementExists(QueueMessageManager::mapTrackedMessages, inTrackName))
      QueueMessageManager::addMessageToTrackedMap(msg);
  }

  addMessageToQueueList(msg, data_manager::errStr);
}


// -----------------------------------
bool
missionx::QueueMessageManager::setMessageChannel(std::string msgName, missionx::mx_message_channel_type_enum inChannelType, std::string inSoundFile, float inSecondsToplay, std::string& outErr, int inSoundVol)
{
  // search message in mapMessages
  // check if has channel with value "pad". Warn in log.
  // create SoundFragment and assign it implace of the old one.

  outErr.clear();
  if (mxUtils::isElementExists(data_manager::mapMessages, msgName))
  {
    std::string channelName = data_manager::mapMessages[msgName].getChannelNameWithType(inChannelType);
    if (channelName.empty())
    {
      channelName = SoundFragment::generateName();
    }
    else
    {
      Log::logMsgWarn("[set channel]replacing existing channel. old Sound file:" + mxconst::get_QM() + data_manager::mapMessages[msgName].mapChannels[channelName].getSoundFile() + mxconst::get_QM() + ", with new sound: " + mxconst::get_QM() + inSoundFile + mxconst::get_QM());
      // remove + release existing channel
      data_manager::mapMessages[msgName].removeChannel(channelName, QueueMessageManager::sound);
    }

    // create the channel
    SoundFragment newSf = SoundFragment(
      channelName,
      (inChannelType == mx_message_channel_type_enum::comm) ? mxconst::get_CHANNEL_TYPE_COMM() : mxconst::get_CHANNEL_TYPE_BACKGROUND(),
      inSoundFile,
      inSoundVol,
      inSecondsToplay);
    data_manager::mapMessages[msgName].mapChannels[channelName] = newSf;

    //// set node accordingly //v3.0.241.1
    if (!data_manager::mapMessages[msgName].node.isEmpty())
    {
      IXMLNode xMix = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(data_manager::mapMessages[msgName].node, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_TYPE(), channelName, false);
      if (!xMix.isEmpty())
      {
        data_manager::mapMessages[msgName].setNodeStringProperty(mxconst::get_ATTRIB_SOUND_FILE(), inSoundFile, xMix);
        data_manager::mapMessages[msgName].setNodeProperty<int>(xMix, mxconst::get_ATTRIB_SOUND_VOL(), inSoundVol);
        data_manager::mapMessages[msgName].setNodeProperty<float>(xMix, mxconst::get_ATTRIB_SOUND_VOL(), inSecondsToplay);

#ifndef RELEASE
        Log::logMsgNone("message mix info: " + std::string(missionx::data_manager::xmlRender.getString(data_manager::mapMessages[msgName].node)) + "\n");
#endif
      }
    }


    data_manager::mapMessages[msgName].applyPropertiesToLocal();
  }
  else
  {
    outErr = "[set channel] No Message by the name: " + mxconst::get_QM() + msgName + mxconst::get_QM() + ". Please fix call. ignoring...";
    return false;
  }

  return true;
}


// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------



bool
missionx::QueueMessageManager::addMessage(missionx::Message inMsg, std::string inTrackedName, std::string& outErr)
{
  outErr.clear();
  inMsg.trackName = inTrackedName;
  return addMessageToQueueList(inMsg, outErr);
}


// -----------------------------------

void
missionx::QueueMessageManager::flc()
{

#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
#endif // TIMER_FUNC

  QueueMessageManager::muteSound = system_actions::pluginSetupOptions.getBoolValue(mxconst::get_OPT_MUTE_MX_SOUNDS(), false);
  QueueMessageManager::muteSound = false; // debug

  // v3.305.1 get msg in listPool
  auto lmbda_get_next_msg_ptr = [](std::list<missionx::Message>& inListPoolMsg, std::list<missionx::Message>& inListInQueMessages) -> missionx::Message*
  {
    if (!inListPoolMsg.empty())
    {
      return &inListPoolMsg.front();
    }

    if (!inListInQueMessages.empty())
      return &inListInQueMessages.front();

    return nullptr;
  };
  auto* msgInstance_ptr     = lmbda_get_next_msg_ptr(QueueMessageManager::listPoolMsg, QueueMessageManager::listPadQueueMessages);
  bool  flag_nextMsgIsStory = (msgInstance_ptr == nullptr) ? false : msgInstance_ptr->mode == missionx::mx_msg_mode::mode_story;
  // end v3.305.1 check type of next/current message to determined if to pause or not

  // v3.0.207.3  Handle pause state
  //if (missionx::dataref_manager::isSimPause() && missionx::Message::lineAction4ui.state == missionx::enum_mx_line_state::undefined) // v3.305.1 Added special condition so QMM will continue evaluation even if sim in pause mode // v3.0.207.3 skip Mission logic while in pause state
  if (missionx::dataref_manager::isSimPause() && !(flag_nextMsgIsStory) ) // v3.305.1 Added special condition so QMM will continue evaluation even if sim in pause mode // v3.0.207.3 skip Mission logic while in pause state
  {
    // try to pause all "sound"
    if (!missionx::QueueMessageManager::soundChannelsArePaused)
      missionx::QueueMessageManager::pauseAllPoolChannels(); // v3.0.207.3

    missionx::QueueMessageManager::soundChannelsArePaused = true;
    return;
  }
  else
  {
    if (missionx::QueueMessageManager::soundChannelsArePaused)
    {
      missionx::QueueMessageManager::unPauseAllPoolChannels(); // v3.0.207.3
      missionx::QueueMessageManager::soundChannelsArePaused = false;
    }

    // check if currently we are broadcasting a COMM message.
    // if not, check comm queue
    // if not empty get FIFO and assign to broadcastCommMsg
    // Check if message time is set, if not calculate message time to display/play

    // Sound flc(). We need to take into consideration Sound specific channel
    // In sound, we have 3 different sound fragments. One for "communication" message, one for "pad" message and one for "background" sound
    // The SOUND class uses "Message" type to fetch the specific SoundFragment array information and handle it

    flc_comm(); // adds queued messages to the "broadcast pool"
  }


  if (!QueueMessageManager::listPoolMsg.empty() && !missionx::data_manager::timelapse.flag_isActive)
  {
    flc_msg_pool();
  }

  // v3.0.241.1 erase the messages that ended
  while (!QueueMessageManager::listEraseKeys.empty()) // free pool
  {
    std::string key = QueueMessageManager::listEraseKeys.front();
    QueueMessageManager::listEraseKeys.pop_front();

    // remove message from message pool
    auto it1 = listPoolMsg.begin();
    while (it1 != listPoolMsg.end())
    {
      if ((*it1).getName().compare(key) == 0)
      {
        (*it1).reset();
        listPoolMsg.erase((it1));
        break; // exit while loop
      }
      ++it1;
    }

    Log::logMsg("[qmm] Freed Message: " + key); // debug
  }

  // v3.0.303.6 removed the "if" statement
  // check background SF v3.0.223.7 
  // loop over all background SF and flag which SF to remove
  for (auto& [key, sf] : QueueMessageManager::mapPlayingBackgroundSF)
  {
    if (missionx::Timer::wasEnded(sf.timerChannel, true) && sf.qSoundCommands.empty()) // v3.306.1 added qSoundCommand logic. We end only if all commands are done.
      QueueMessageManager::listEraseBackgroundKeys.push_back(key); // name of originate message
    else
      QueueMessageManager::sound.flc(sf);        
  }

  // erase background SF
  while (!QueueMessageManager::listEraseBackgroundKeys.empty()) // free pool
  {
    std::string key = QueueMessageManager::listEraseBackgroundKeys.front();
    QueueMessageManager::listEraseBackgroundKeys.pop_front();

    QueueMessageManager::sound.stopAndReleaseChannel(QueueMessageManager::mapPlayingBackgroundSF[key]);
    QueueMessageManager::mapPlayingBackgroundSF.erase(key);
//#ifndef RELEASE
    Log::logMsg("[qmm] Erasing background sound message: " + key); // debug
//#endif                                               
  }



  // v3.0.303.6 modifying sound volume on the fly
  const bool bNormalizeComm      = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_NORMALIZE_VOLUME_B(), false);
  const int  iNormalize_comm_val = Utils::getNodeText_type_1_5<int>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_NORMALIZED_VOLUME(), mxconst::DEFAULT_SETUP_MISSION_VOLUME_I);

  if (bNormalizeComm)
    QueueMessageManager::sound.setCommVolume(((float)iNormalize_comm_val) / 100.0f);

  QueueMessageManager::sound.update(); // important for Sound System cleanup and progress

  // check mute options
  bool isSoundSystemMute = false;
  if (QueueMessageManager::sound.commChannelGroup != nullptr)
    QueueMessageManager::sound.commChannelGroup->getMute(&isSoundSystemMute);

  if (QueueMessageManager::muteSound && !isSoundSystemMute)
    QueueMessageManager::sound.setMuteSound(true);
  else if (!QueueMessageManager::muteSound && isSoundSystemMute)
    QueueMessageManager::sound.setMuteSound(false);

  // mxpad remove old messages if we exceed MAX_MX_PAD_MESSAGES
  if (QueueMessageManager::mxpad_messages.size() > missionx::MAX_MX_PAD_MESSAGES)
  {
    QueueMessageManager::mxpad_messages.pop_front();            // pitmib v3.0.205.1
  }
}

// -----------------------------------

void
missionx::QueueMessageManager::flc_comm()
{


  // Handle Message in Queue
  if (!QueueMessageManager::listPadQueueMessages.empty())
  {
    while (!QueueMessageManager::listPadQueueMessages.empty())
    {
      const size_t pool_jobs_size = QueueMessageManager::listPoolMsg.size();
      if (pool_jobs_size <= MAX_MESSAGES_IN_POOL)
      {
        Message newMessageInstance;
        // get message from Queue
        newMessageInstance          = QueueMessageManager::listPadQueueMessages.front(); // v3.303.11 stop using class operator. So far it works fine
        newMessageInstance.msgState = missionx::mx_message_state_enum::msg_in_pool;
        newMessageInstance.setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), static_cast<int> (newMessageInstance.msgState)); //, newMessage_ptr.node, newMessage_ptr.node.getName());

        QueueMessageManager::listPoolMsg.push_back(newMessageInstance);
        QueueMessageManager::listPadQueueMessages.pop_front();

        // we will calculate text and channels during flc_msg_pool();
      }
      else
        break; // exit while loop
    }          // end while loop
  }

}


// -----------------------------------

void
missionx::QueueMessageManager::flc_msg_pool()
{ // loop over mapPoolMessage // working channels
  // handle each state change

  int counter = 0;
  for (auto& m : QueueMessageManager::listPoolMsg)
  {
    ++counter;
    QueueMessageManager::progressMessage(&m, counter);


  } // end loop over message map
}
// end flc_msg_pool()

// -----------------------------------

void
missionx::QueueMessageManager::progressMessage(Message* msgInstance, int& inMsgInQueue_i)
{
  // static bool found_mxpad_message = false; //
  assert(!msgInstance->node.isEmpty());

  std::string key = Utils::readAttrib(msgInstance->node, mxconst::get_ATTRIB_NAME(), ""); // msg->getName();
  if (key.empty())                                                          // v3.0.241.2 try to fallback to old code in cases we construct a message using: "fn_send_text_message"
    key = msgInstance->getName();

#ifdef DEBUG_QMM
  IXMLRenderer writer;
  Log::logMsg("[qmm::progressMsg] Key: " + key + ", xml node: \n");
  Log::logMsgNone(writer.getString(msg->node));
  writer.clear();
#endif // DEBUG_QMM

  // first step - gather some info
  const auto lmbda_message_has_comm_sf = [&](std::pair<std::string, missionx::SoundFragment> inSf) { return (inSf.second.channelType == mx_message_channel_type_enum::comm); }; // check if SoundFragment is "comm" channel type
  const bool flag_message_has_comm_sf  = std::any_of(begin(msgInstance->mapChannels), end(msgInstance->mapChannels), lmbda_message_has_comm_sf);                                                // return true if at least one of the "mapChannels" returns true


  // second step// msg_prepare_channels
  if (msgInstance->msgState <= mx_message_state_enum::msg_prepare_channels) // if not finished prepare channel
  {
    // check if we have any channels settings in message
    if (msgInstance->mapChannels.empty())
    {
      msgInstance->msgState = mx_message_state_enum::msg_is_ready_to_be_played;
      msgInstance->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)msgInstance->msgState); 
    }
    else
    {
      // loop over channels and try to initialize them
      // We then need to wait for files to be uploaded
      size_t counter = 0;
      for (auto& [cKey, sf] : msgInstance->mapChannels)
      {
        if ((sf.channelType != mx_message_channel_type_enum::background) && (sf.channelType != mx_message_channel_type_enum::comm))
          continue;

        if (sound.prepareStreamSoundFile(msgInstance->msgState, sf) == FMOD_OK)
          counter++;

      } // end loop over channels and prepare them

      // do we need to change state of Message

      if (( counter > 0) || (counter >= msgInstance->mapChannels.size()) || sound.initSoundResult != FMOD_OK)
      {
        msgInstance->msgState = mx_message_state_enum::msg_channels_need_to_finish_loading;
        msgInstance->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)msgInstance->msgState);
      }
    }


  } // end prepare channels


  // third step  // msg_channels_need_to_finish_loading
  if (msgInstance->msgState == mx_message_state_enum::msg_channels_need_to_finish_loading)
  {

    if (msgInstance->mapChannels.empty())
    {
      msgInstance->msgState = mx_message_state_enum::msg_is_ready_to_be_played;
      msgInstance->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)msgInstance->msgState);
    }
    else
    {
      size_t counter = 0;
      for (auto& [cKey, sf] : msgInstance->mapChannels)
      {
        //std::string cKey = iter.first;

        //SoundFragment* sf = &msgInstance->mapChannels[cKey];

        // v3.0.194 deprecate pad - changed to comm
        if ((sf.channelType != mx_message_channel_type_enum::comm) && (sf.channelType != mx_message_channel_type_enum::background)) // v3.0.223.6 added background test
          continue;

        if (sf.areWeWaitingForSoundFileToLoad) // if file is still loading to memory
        {
          // check file load state
          // FMOD_OPENSTATE state;
          if ((sf.sound != nullptr))
          {
            const auto fmod_file_state = sf.getOpenSoundFileState();  // v24026 //sound.getOpenSoundFileState(sf.sound);
            if (fmod_file_state == FMOD_OPENSTATE_READY)
            {
              counter++;
              sf.isSoundFileReadyToBePlayed = true;
              sf.openState                  = FMOD_OPENSTATE_READY;
            }
            else if (fmod_file_state != FMOD_OPENSTATE_ERROR) // v3.305.4 solve racing issue with file taking time to load, it should not be ERRORED automatically.
            {
              Log::logMsg(fmt::format("[{}] Waiting for file: {}, to open. FMOD Open State: {}", __func__, sound.fileToPlay_fullPath, (int)fmod_file_state));
              continue;
            }
            else
            {
              Log::logMsg(fmt::format("[{}] Failed to open sound file: {}. FMOD Open State: {}", __func__, sound.fileToPlay_fullPath, (int)fmod_file_state));
              counter++;
              sf.isSoundFileReadyToBePlayed = false;
              sf.openState                  = FMOD_OPENSTATE_ERROR;
            }
          }
        }

      } // end loop over Sound Fragments

      if ((counter > 1) || (counter >= msgInstance->mapChannels.size()) || sound.initSoundResult != FMOD_OK )
      {
        msgInstance->msgState = mx_message_state_enum::msg_is_ready_to_be_played;
        msgInstance->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)msgInstance->msgState);
      }

    } // end check message has channels to check


  } // end msg_channels_need_to_finish_loading



  //// BROADCAST //////////
  // fifth step - We can broadcast message
  if (msgInstance->msgState == mx_message_state_enum::msg_is_ready_to_be_played && inMsgInQueue_i == 1 && !QueueMessageManager::soundChannelsArePaused /* handle pause state. We should not continue when in pause */ 
      && !QueueMessageManager::flag_message_is_broadcasting && !missionx::data_manager::timelapse.flag_isActive /*v3.0.223.1 wait for timelapse to end*/)
  {



    // Mode = default = simple text message
    switch (msgInstance->mode)
    {
      case mx_msg_mode::mode_default:
      {
        QueueMessageManager::prepareMessageText(msgInstance, flag_message_has_comm_sf);
        break;
      }
      case mx_msg_mode::mode_story:
      {
        QueueMessageManager::prepareMessageStory(msgInstance);
        break;
      }
      default:
      {
        assert(false && "The message progress should not reach this state.");
      }
      
    } // end switch message mode


  } // end msg_is_ready_to_be_played - call play


  // handle message timing (when it should flag as ended
  if (msgInstance->msgState > mx_message_state_enum::msg_is_ready_to_be_played && msgInstance->msgState < mx_message_state_enum::msg_is_ready_for_post_message_actions) // msg_broadcast_once,  msg_broadcast_few_times
  {
    QueueMessageManager::evalCommChannelProgress(msgInstance, flag_message_has_comm_sf, key); // v3.305.1 moved handling of comm channel to its own function
  }

  // v3.305.1
  if (msgInstance->msgState >= mx_message_state_enum::msg_is_ready_for_post_message_actions)
  {
    if (!(msgInstance->msgState == missionx::mx_message_state_enum::msg_abort)) // v3.0.223.7 allow to abort a message without post actions. Used in ext_script::ext_abort_current_message
      missionx::QueueMessageManager::postMessage(msgInstance);                  // v3.0.223.1 handle special message directives

    QueueMessageManager::listEraseKeys.push_back(key); // line is relevant, since designer can change is_mxpad attribute from "yes" to "no"

    // if (QueueMessageManager::flag_message_is_broadcasting) // v3.305.1 we always reset to false so why use "if" statement.
    QueueMessageManager::flag_message_is_broadcasting = false;
  }

}



// -----------------------------------

void
missionx::QueueMessageManager::prepareMessageStory(Message* msg)
{
  static int stLineCounter = 0;

//#ifndef RELEASE
  static std::chrono::steady_clock::time_point startStoryMessageClock;
  if (Message::lineAction4ui.isInit)
    startStoryMessageClock = std::chrono::steady_clock::now();
//#endif

  if (Message::lineAction4ui.isInit)
  {
    #ifndef RELEASE
        Log::logMsg(std::string(__func__) + ", Calling  playSoundFilesFromAllChannels() for message: " + msg->getName()); // debug
    #endif // !RELEASE

    //// Play sound files from all channels if present
    if (sound.initSoundResult == FMOD_OK)
      QueueMessageManager::playSoundFilesFromAllChannels(msg, QueueMessageManager::SECONDS_PER_LINE * (float)msg->dqMsgLines.size()); // v3.305.1 moved handling of all channel to its own function

  }


  if (Message::lineAction4ui.state < missionx::enum_mx_line_state::init)
  {
    Message::lineAction4ui.reset();
    Message::lineAction4ui.state = missionx::enum_mx_line_state::init;
    Message::lineAction4ui.bIgnorePunctuationTiming = Utils::readBoolAttrib(msg->node, mxconst::get_ATTRIB_IGNORE_PUNCTUATIONS_B(), false);

    std::string outAction_s = "";
    bool        bIsActionLine;
    std::string line = msg->get_and_filter_next_line(outAction_s, bIsActionLine, stLineCounter); // v3.305.3 was: lmbda_get_and_filter_next_line(msg->dqMsgLines, outAction_s, bIsActionLine);

    if ( (!msg->dqMsgLines.empty()) + (!line.empty()) )
    {
      //msg->dqMsgLines.pop_front(); // removing last action from queue // already done in "lmbda" function
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::open_story_layout); // open Briefer screen and in the flight leg so simmer immediately will see the Story message
    }
    else
    {
      Message::lineAction4ui.state = missionx::enum_mx_line_state::mainMessageEnded;
    }

    // fail Message if a line is empty
    if (line.empty())
    {
      #ifndef RELEASE
        Log::logMsgWarn("[qmm] Message: " + msg->getName() + ", has reached an empty line. Line: " + mxUtils::formatNumber<int>(stLineCounter) + ".");
      #endif // !RELEASE

      Message::lineAction4ui.state = missionx::enum_mx_line_state::mainMessageEnded;      
    }
    else if (bIsActionLine)
    {
      assert(outAction_s.length() > (size_t)2 && "Action must be 3 chars in length"); // at least 2 characters

      char action = outAction_s[1];
      switch (action)
      {
        case mxconst::STORY_ACTION_IMG:
        {
          #ifndef RELEASE
          Log::logMsg("[Line " + mxUtils::formatNumber<int>(stLineCounter) + "] Action Load Image: " + line );
          #endif // !RELEASE

          Message::lineAction4ui.state = missionx::enum_mx_line_state::parsing;
          if (msg->parse_action(mxconst::STORY_ACTION_IMG, line) && Message::lineAction4ui.state == missionx::enum_mx_line_state::parsing_ok )
          {
            Message::lineAction4ui.state = missionx::enum_mx_line_state::loading;
            auto future                  = std::async(std::launch::async, &missionx::data_manager::loadStoryImage, msg, Message::lineAction4ui.vals[mxconst::get_ATTRIB_NAME()]);
          }
        }
        break;
        case mxconst::STORY_ACTION_PAUSE:
        {
          if (Message::lineAction4ui.bUserPressNextInTextActionMode) // do we need to skip this command ?
          {
            Message::lineAction4ui.bUserPressNextInTextActionMode = false;
            Message::lineAction4ui.state                          = missionx::enum_mx_line_state::action_ended;
          }
          else
          {
            Message::lineAction4ui.state = missionx::enum_mx_line_state::parsing;
            if (msg->parse_action(mxconst::STORY_ACTION_PAUSE, line))
            {
              Message::lineAction4ui.state = missionx::enum_mx_line_state::ready;
            }
          }


        }
        break;
        case mxconst::STORY_ACTION_MSGPAD:
        {

          Message::lineAction4ui.state = missionx::enum_mx_line_state::parsing;

          if (msg->parse_action(mxconst::STORY_ACTION_MSGPAD, line))
          {
            Message::lineAction4ui.state = missionx::enum_mx_line_state::ready;

            Message::lineAction4ui.actionCode = mxconst::STORY_ACTION_TEXT; // convert the action code to text, since [m] is a special case of TEXT message that also being displayed in the mxpad.

            // add mxpad lines
            messageLine_strct mxPadline;
            mxPadline.label          = "info";
            mxPadline.label_position = "L";
            mxPadline.label_color    = mxconst::get_YELLOW();
            mxPadline.message_text   = Message::lineAction4ui.vals[mxconst::get_STORY_TEXT()]; // get the text line to display

            if (missionx::mxvr::vr_display_missionx_in_vr_mode)
            {
              Utils::sentenceTokenizerWithBoundaries(mxPadline.textLines, mxPadline.message_text, mxconst::get_SPACE(), 55); // in VR
            }
            else
            {
              Utils::sentenceTokenizerWithBoundaries(mxPadline.textLines, mxPadline.message_text, mxconst::get_SPACE(), 55); // in 2D
            }
            mxpad_messages.push_back(mxPadline);

          }
          else
          {
            Message::lineAction4ui.state = missionx::enum_mx_line_state::parsing_failure;
            missionx::Log::logMsgErr("[qmm] Message: " + msg->getName() + ", failed to parse story line: \n" + line + "\n<<\n");
          }



        }
        break;
        case mxconst::STORY_ACTION_HIDE:
        {
          if (msg->parse_action(mxconst::STORY_ACTION_HIDE, line))
          {
            Message::lineAction4ui.state = missionx::enum_mx_line_state::ready;
          }

          if (msg->dqMsgLines.empty())
            Message::lineAction4ui.state = missionx::enum_mx_line_state::mainMessageEnded;
        }
        break;
        default:
        {
          std::string err = fmt::format("Action '{}' is not supported.", action);
          XPLMDebugString(err.c_str());
          //assert(false && err.c_str());
        }

      } // end switch
    } // end if bIsActionLine
    else if (Message::lineAction4ui.state != missionx::enum_mx_line_state::mainMessageEnded)
    {
      
      Message::lineAction4ui.bUserPressNextInTextActionMode = false; // reset the user pause flag on new text message

      // this is a text line -> implicit [t] action
      Message::lineAction4ui.state = missionx::enum_mx_line_state::parsing;
      if (msg->parse_action(mxconst::STORY_ACTION_TEXT, line)) // implicit action "t", it is not part of the line command. The command should look: p "some text....."
      {
        Message::lineAction4ui.state = missionx::enum_mx_line_state::ready;
      }
      else
      {
        Message::lineAction4ui.state = missionx::enum_mx_line_state::parsing_failure;
        missionx::Log::logMsgErr("Message: " + msg->getName() + ", failed to parse story line: \n" + line + "\n<<\n");
      }

    } // end if TEXT and not end of message

  } // if current story message line is not in progress


  // Do we end the message ?
  if (Message::lineAction4ui.state == missionx::enum_mx_line_state::mainMessageEnded)
  {
//#ifndef RELEASE
    auto end  = std::chrono::steady_clock::now();
    auto diff = end - startStoryMessageClock;
    auto duration  = std::chrono::duration<double, std::milli>(diff).count();
    Log::logAttention("Story Message Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec), for: " + msg->getName());
//#endif // !RELEASE



    stLineCounter = 0;
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::post_story_message_cache_cleanup); // call only for loaded image

    if (Message::lineAction4ui.actionCode == mxconst::STORY_ACTION_HIDE)
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::hide_briefer_window_in_2D); // call only for loaded image

    Message::lineAction4ui.init(); // initialize the state of the static story message line so ui can return to its default screen    
    msg->msgState = missionx::mx_message_state_enum::msg_is_ready_for_post_message_actions; // this should progress the message to the post message actions

  }

}


// -----------------------------------


void
missionx::QueueMessageManager::prepareMessageText(Message* msg, bool flag_message_has_comm_sf)
{
  #ifndef RELEASE
  int iDebug = 0;
  if (iDebug) // we need to change the iDebug value using the debugger
  {
    XPLMDebugString(Utils::xml_get_node_content_as_text(msg->node).c_str() );
  }
  #endif 


  // broadcast TEXT after calculating message text timer
  std::string messageText = msg->getMessage();
  messageText             = Utils::replaceChar1WithChar2_v2(messageText, ';', mxconst::get_SPACE());
  Utils::replaceCharsWithString(messageText, mxconst::get_WIN_EOL(), mxconst::get_SPACE()); // v3.0.191


  bool muteNarator = Utils::readBoolAttrib(msg->node, mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR(), false);
  bool hideText    = Utils::readBoolAttrib(msg->node, mxconst::get_ATTRIB_MESSAGE_HIDE_TEXT(), false);

  float secondsToDisplayText_userDefine = Utils::readNodeNumericAttrib<float>(msg->node, mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT(), 0.0f);
  float secondsCalcPerLine_userDefined  = Utils::readNodeNumericAttrib<float>(msg->node, mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_CALC_PER_LINE(), 0.0f);


  msg->msgCounter++;

  // If we support voice in message and this message is not muted and we want to display text message...
  {
    messageLine_strct mxPadline;

    mxPadline.label          = Utils::readAttrib(msg->node, mxconst::get_ATTRIB_LABEL(), "radio");
    mxPadline.label_position = Utils::readAttrib(msg->node, mxconst::get_ATTRIB_LABEL_PLACEMENT(), "L");
    mxPadline.label_color    = Utils::readAttrib(msg->node, mxconst::get_ATTRIB_LABEL_COLOR(), mxconst::get_YELLOW());


    mxPadline.message_text = messageText;

    // v3.0.221.7 try to decrease the number of mxpad characters in VR
    if (missionx::mxvr::vr_display_missionx_in_vr_mode)
    {
      Utils::sentenceTokenizerWithBoundaries(mxPadline.textLines, mxPadline.message_text, mxconst::get_SPACE(), 55); // in VR
    }
    else
    {
      Utils::sentenceTokenizerWithBoundaries(mxPadline.textLines, mxPadline.message_text, mxconst::get_SPACE(), 55); // in 2D
    }

    const auto lmbda_get_seconds_to_display_text = [&]()
    {
      if (secondsToDisplayText_userDefine > 0.0f)
        return secondsToDisplayText_userDefine;
      else if (secondsCalcPerLine_userDefined > 0.0f)
        return secondsCalcPerLine_userDefined * mxPadline.textLines.size();
      else
        return QueueMessageManager::SECONDS_PER_LINE * mxPadline.textLines.size();
    };

    secondsToDisplayText_userDefine = lmbda_get_seconds_to_display_text();
    mxPadline.textLines.clear(); // v3.305.1 release text since it won;t be needed anymore

    if (hideText)
    {
      Log::logDebugBO("[qmm] Skip message text. Won't be displayed in mx-pad"); // debug
    }
    else
    {
      // Draw text in 2D PAD
      Log::logDebugBO("[qmm] Broadcast PAD message: " + messageText);

      // v3.0.192 add new mxpad text to mxpad display container
      mxpad_messages.push_back(mxPadline);                     // display in MX-PAD window   // pitmib v3.0.205.1

      // v3.0.213.7 allow mxpad text to be narrated if it does not have channels
      if (!muteNarator && !muteSound /*from Option screen */) // v3.0.223.6 removed channel test since all messages are mx-pad type
      {

        Log::logDebugBO("[qmm] Broadcast mxpad using narrator: " + messageText);

        XPLMSpeakString(messageText.c_str()); // plugin
        QueueMessageManager::flag_message_is_broadcasting = true;
      } // end broadcast mxpad with narrator instead of channel
    }
  }


  msg->flc(); // progress state. Will change from msg_not_broadcasted to msg_broadcast_xxx which is bigger than msg_is_ready_to_be_played

  // store message in tracked map
  if (!msg->trackName.empty()) // v3.0.241.1 all messages can be tracked since the default is that all messages are mx_pad
    missionx::QueueMessageManager::addMessageToTrackedMap((*msg));



  if (!flag_message_has_comm_sf) // v3.0.223.7 if we do not have comm SoundFragment, set timer using secondsToDisplayText_userDefine
  {
    if (!secondsToDisplayText_userDefine) // to be on the safe side
      secondsToDisplayText_userDefine = QueueMessageManager::SECONDS_PER_LINE;
    // update the message node
    msg->setNodeProperty<double>(mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT(), secondsToDisplayText_userDefine); //, msg->node, mxconst::get_ELEMENT_MESSAGE());
    msg->msgTimer.reset();
    Timer::start(msg->msgTimer, secondsToDisplayText_userDefine, msg->getName()); // start stopper
  }
  else
    msg->msgTimer.setEnd(); // we do not time the TEXT message only the "comm" channel

    //// Play sound files from all channels if present

  if (sound.initSoundResult == FMOD_OK)
    QueueMessageManager::playSoundFilesFromAllChannels(msg, secondsToDisplayText_userDefine); // v3.305.1 moved handling of all channel to its own function
}


// -----------------------------------


void
missionx::QueueMessageManager::playSoundFilesFromAllChannels(Message* msg, float secondsToDisplayText_userDefine)
{
  // v3.0.303.6 read setup normailized volume and decide if to modify the sound channel
  const bool bNormalize     = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_NORMALIZE_VOLUME_B(), false);
  const int  iNormalize_val = Utils::getNodeText_type_1_5<int>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_NORMALIZED_VOLUME(), mxconst::DEFAULT_SETUP_MISSION_VOLUME_I);

  // Play sound files from all channels if present
  for (auto& [sfKey, sf] : msg->mapChannels)
  {
    // const std::string sfKey = s.first;
    switch (sf.channelType)
    {
      case mx_message_channel_type_enum::comm:
      {
        if (sf.isSoundFileReadyToBePlayed && sf.sound != nullptr && msg->mode == missionx::mx_msg_mode::mode_default) // v3.305.1 added message mode type must be default and not story
        {
          // v3.0.303.6 read setup normailized volume and decide if to modify the sound channel
          if (bNormalize && sf.volume > (float)iNormalize_val)
            sf.setVolume((float)iNormalize_val); // set the normalized volume


          QueueMessageManager::sound.playStreamFile(sf); // try to play stream

          if (QueueMessageManager::sound.isChannelPlaying(sf))
          {

            float secondsToPlay = secondsToDisplayText_userDefine; // instead of 0.0f
            if (secondsToPlay <= 0.0f)                             // v3.0.190 if no overide text time was defined then fetch mixture play time.
              secondsToPlay = Utils::readNodeNumericAttrib<float>(sf.node, mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_PLAY(), 0.0f);

            // start timer, calculate if seconds to play is 0 (zero)
            if (secondsToPlay <= 0.0f)
            {
              // try to figure out the length of the file from sound
              secondsToPlay = (float)QueueMessageManager::sound.getSoundFileLengthInSec(sf);

              if (secondsToPlay <= 0) // make sure that we have something
                secondsToPlay = 3.0f;

              Log::logDebugBO("[qmm] Calculated sound file, length: " + Utils::formatNumber<float>(secondsToPlay, 2)); // debug
            }
            sf.setNumberProperty(mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_PLAY(), secondsToPlay);
            std::string timerName = sf.getName() + "_" + mxUtils::translateMessageChannelTypeToString(sf.channelType); // create timer name
            Timer::start(sf.timerChannel, secondsToPlay, timerName);                                                   // start stopper
          }
        }
      }
      break;
      case mx_message_channel_type_enum::background:
      {
        if (sf.isSoundFileReadyToBePlayed && sf.sound != nullptr)
        {

          missionx::SoundFragment background_sf = sf; //

          QueueMessageManager::sound.playStreamFile(background_sf); // try to play stream

          if (QueueMessageManager::sound.isChannelPlaying(background_sf))
          {
            float secondsToPlay = Utils::readNodeNumericAttrib<float>(background_sf.node, mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_PLAY(), 0.0f); // v3.0.223.6 "background" should always pick its own length and not from TEXT
            if (secondsToPlay <= 0.0f)
              secondsToPlay = (float)QueueMessageManager::sound.getSoundFileLengthInSec(background_sf);

            if (secondsToPlay <= 0.0f) // make sure that we have something
              secondsToPlay = (secondsToDisplayText_userDefine > 0.0f) ? secondsToDisplayText_userDefine : 3;

            background_sf.setNumberProperty(mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_PLAY(), secondsToPlay);                                         // v3.0.209.1 changed from sf->setIntProperty()
            std::string timerName = background_sf.getName() + "_" + mxUtils::translateMessageChannelTypeToString(background_sf.channelType); // create timer name
            Timer::start(background_sf.timerChannel, secondsToPlay, timerName);                                                              // start stopper

            const std::string sBackgroundKey = Utils::readAttrib(background_sf.node, mxconst::get_ATTRIB_ORIGINATE_MESSAGE_NAME(), "");
            if (!sBackgroundKey.empty())
            {
              Utils::addElementToMap(missionx::QueueMessageManager::mapPlayingBackgroundSF, sBackgroundKey, background_sf); // will be used to control playing progress.
            }
          } // end if playing SF
        }   // end if background SF ready to be played
      }
      break;
      default:
        break;
    } // end switch over channels

  } // end loop over channels
}

// -----------------------------------

void
missionx::QueueMessageManager::evalCommChannelProgress(Message* msg, const bool flag_message_has_comm_sf, const std::string& key)
{
  bool flag_timerEnded, flag_commChannelEndedPlaying; // hold timeout flag
  flag_timerEnded = flag_commChannelEndedPlaying = false;

  // Check TEXT channel
  if (Timer::wasEnded(msg->msgTimer, true) ) // + (msg->mode == mx_msg_mode::mode_story) ) // v3.305.1 Added story_mode exception, since it does not have internal timer
  {
    flag_timerEnded = true;
  }

  // Check COMM channel
  // loop over channels and check if comm channel ended. If channel is not "comm" then assume the timer of "background" channel is handled elsewhere
  if (flag_message_has_comm_sf && sound.initSoundResult == FMOD_OK) // v24.03.2 added initSoundResult if FMOD sound failed to initialize we auto end the playback
  {
    auto lmbda_message_comm_timer_ended = [](std::pair<const std::string, missionx::SoundFragment>& inIterSf)
    {
      if (inIterSf.second.channelType == mx_message_channel_type_enum::comm)
      {
        if (Timer::wasEnded(inIterSf.second.timerChannel, true) || inIterSf.second.timerChannel.getState() == missionx::mx_timer_state::timer_not_set)
          return true;
        else
          return false;
      }

      return true;                                                                                                              // for background or non "comm" channel types.
    };                                                                                                                          // check if SoundFragment is "comm" channel type
    flag_commChannelEndedPlaying = std::all_of(begin(msg->mapChannels), end(msg->mapChannels), lmbda_message_comm_timer_ended); // return true if _all_ "mapChannels" returns true
  }
  else
    flag_commChannelEndedPlaying = true;


  if (flag_timerEnded && flag_commChannelEndedPlaying)
  {
    // reset COMM MIX channels only
    for (auto iter = msg->mapChannels.begin(); iter != msg->mapChannels.end(); ++iter)
    {
      std::string cKey = iter->first;
      if (msg->mapChannels[cKey].channelType == mx_message_channel_type_enum::comm)
      {
        SoundFragment* sf = &msg->mapChannels[cKey];
        QueueMessageManager::sound.stopAndReleaseChannel(*sf);
      }
    }

    msg->msgState = mx_message_state_enum::msg_is_ready_for_post_message_actions;

  }
}



// -----------------------------------

void
missionx::QueueMessageManager::postMessage(Message* msg)
{
  std::string err;

  // v3.305.3 moved post_script before processing the other message attributes. // exec script after message broadcasted
  const std::string post_script_s = Utils::readAttrib(msg->node, mxconst::get_ATTRIB_POST_SCRIPT(), "");
  if (!post_script_s.empty())
  {
    data_manager::smPropSeedValues.setStringProperty(mxconst::get_EXT_MX_QM_MESSAGE(), msg->getName()); // v3.0.223.1 store message broadcast name
    missionx::data_manager::execScript(post_script_s, data_manager::smPropSeedValues, "[qmm][msg] Post Message script: " + post_script_s + " is invalid. Aborting Mission");
    data_manager::smPropSeedValues.removeProperty(mxconst::get_EXT_MX_QM_MESSAGE()); // v3.0.223.1 remove the message name after script was executed
  }

  // read from xml node and each time decide if we need next time option or not. First time found "wins". Order: ATTRIB_SET_DAY_HOURS, ATTRIB_TIMELAPSE_TO_LOCAL_HOURS, ATTRIB_ADD_MINUTES
  const std::string nextLocalTime_s = Utils::readAttrib(msg->node, mxconst::get_ATTRIB_SET_DAY_HOURS(), "");

  if (!nextLocalTime_s.empty())
  {
    err = missionx::data_manager::timelapse.set_date_and_hours(nextLocalTime_s, (msg->mode == missionx::mx_msg_mode::mode_story));
  }
  else
  {
    const std::string nextLocalHour_s = Utils::readAttrib(msg->node, mxconst::get_ATTRIB_TIMELAPSE_TO_LOCAL_HOURS(), "");
    if (!nextLocalHour_s.empty())
    {
      int nextLocalHour_i = 0;
      int minutes_i       = 0;
      int cycles_i        = -1;
      err                 = missionx::Utils::convert_string_to_24_min_numbers(nextLocalHour_s, nextLocalHour_i, minutes_i, cycles_i);
      if (err.empty())
        err = missionx::data_manager::timelapse.timelapse_to_local_hour(nextLocalHour_i, minutes_i, cycles_i, false, (msg->mode == missionx::mx_msg_mode::mode_story));
    }
    else
    {
      if (!Utils::readAttrib(msg->node, mxconst::get_ATTRIB_ADD_MINUTES(), "").empty())
      {
        const static float max_min_to_skip = 240.0f;
        double             minutes_d       = Utils::readNodeNumericAttrib<double>(msg->node, mxconst::get_ATTRIB_ADD_MINUTES(), 0.0);
        if (minutes_d > 1 && minutes_d <= max_min_to_skip)
        {
          int how_many_cycles_i = (int)((float)(mxconst::MAX_LAPS_I) * ((minutes_d / max_min_to_skip))); //
          if (how_many_cycles_i > mxconst::MAX_LAPS_I || minutes_d < 30.0)
            how_many_cycles_i = 1;

          err = missionx::data_manager::timelapse.timelapse_add_minutes((int)minutes_d, how_many_cycles_i, (msg->mode == missionx::mx_msg_mode::mode_story));
        }
        else if (minutes_d != 0 && (minutes_d > 240 || minutes_d < 1))
        {
          Log::logDebugBO(std::string("Your message attribute: ") + mxconst::get_ATTRIB_ADD_MINUTES() + ", has a value that is not between 1 and 240. skipped add_minutes() function.");
        }
      }
    }
  }

  // inject metar
  const std::string metar_file_name_s = Utils::readAttrib(msg->node, mxconst::get_ATTRIB_INJECT_METAR_FILE(), "");
  if (!metar_file_name_s.empty())
  {
    missionx::data_manager::inject_metar_file(metar_file_name_s);
  }


  // v3.0.303.7 open choice
  const std::string choice_s = Utils::readAttrib(msg->node, mxconst::get_ATTRIB_OPEN_CHOICE(), "");
  if (!choice_s.empty())
  {
    missionx::data_manager::prepare_choice_options(choice_s);
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::display_choice_window);
  }

  // v3.305.3 auto bg fade ATTRIB_FADE_BG_CHANNEL
  const std::string bg_fade_s = mxUtils::replaceAll(Utils::readAttrib(msg->node, mxconst::get_ATTRIB_FADE_BG_CHANNEL(), ""), mxconst::REPLACE_KEYWORD_SELF, msg->name); // v3.305.4 support %self%
  if (!bg_fade_s.empty())
  {
    double      seconds = mxconst::DEFAULT_SF_FADE_SECONDS_F;
    std::string name;

    std::vector<std::string> vecBgRules = mxUtils::split_v2(bg_fade_s, ",");

    int i1 = 0;
    for (const auto& val : vecBgRules) // do not loop more than 2 values
    {
      if (i1 >= 2)
        break;

      if (i1 == 0)
      {
        name = val;
      }
      else if (i1 == 1)
      {
        seconds = ((mxUtils::is_digits(val)) ? mxUtils::stringToNumber<double>(val, 2) : mxconst::DEFAULT_SF_FADE_SECONDS_F);
      }

      ++i1;
    }
    //for (int i1 = 0; i1 < vecBgRules.size() && i1 < 2; ++i1) // do not loop more than 2 values
    //{
    //  if (i1 == 0)
    //  {
    //    name = vecBgRules.at(0);
    //  }
    //  else if (i1 == 1)
    //  {
    //    seconds = ((mxUtils::is_digits(vecBgRules.at(1))) ? mxUtils::stringToNumber<double>(vecBgRules.at(1), 2) : mxconst::DEFAULT_SF_FADE_SECONDS_F);
    //  }
    //}

    #ifndef RELEASE
    Log::logMsg(fmt::format("[debug] Will fade bg channel: \"{}\", in {} seconds", name, seconds));
    #endif

    QueueMessageManager::fade_bg_channel(name, seconds);
  }
  

  // check if has next_msg property with value and recursively call addMessageToQueueList().
  std::string next_msg_s = Utils::readAttrib(msg->node, mxconst::get_PROP_NEXT_MSG(), "");
  if (!next_msg_s.empty())
    missionx::QueueMessageManager::addMessageToQueue(next_msg_s, "", err);


} // end postMessage

  // -----------------------------------

void
missionx::QueueMessageManager::fade_bg_channel(const std::string& inMessageName_s, const double& inSecondsUntilFadeOut_d)
{
  if (  Utils::isElementExists(missionx::QueueMessageManager::mapPlayingBackgroundSF, inMessageName_s) 
     && QueueMessageManager::mapPlayingBackgroundSF[inMessageName_s].isPlaying() )
  {
    missionx::mx_track_instructions_strct fadeCommand;
    fadeCommand.seconds_to_start_f = 0.0f; // immediate
    fadeCommand.command            = '-';  // decrease command
    fadeCommand.new_volume         = 0;    // expected volume to decrease to
    fadeCommand.transition_time_f  = (inSecondsUntilFadeOut_d <= 0.0) ? mxconst::DEFAULT_SF_FADE_SECONDS_F : (float)inSecondsUntilFadeOut_d; // Use used fade steps or plugin default


    mxUtils::purgeQueueContainer(missionx::QueueMessageManager::mapPlayingBackgroundSF[inMessageName_s].qSoundCommands);
    missionx::QueueMessageManager::mapPlayingBackgroundSF[inMessageName_s].qSoundCommands.emplace(fadeCommand);

    missionx::QueueMessageManager::mapPlayingBackgroundSF[inMessageName_s].timerChannel.setEnd();
  }
}


  // -----------------------------------

void
missionx::QueueMessageManager::saveCheckpoint(IXMLNode& inParent)
{
  // create root MXPAD element
  Utils::xml_add_comment(inParent, " ===== MXPAD Data ===== "); // v3.303.8

  IXMLNode xMxPad_data = inParent.addChild(mxconst::get_ELEMENT_MXPAD_DATA().c_str());
  {
    // loop over Active MXPAD Messages
    IXMLNode xMxPadMessages = xMxPad_data.addChild(mxconst::get_ELEMENT_MXPAD_ACTIVE_MESSAGES().c_str());

    // save all active MXPAD messages in MXPAD window
    for (auto &m : QueueMessageManager::mxpad_messages)
    {

      IXMLNode xMessage = xMxPadMessages.addChild(mxconst::get_ELEMENT_MESSAGE ().c_str());
      xMessage.addAttribute(mxconst::get_ATTRIB_LABEL().c_str(), m.label.c_str());
      xMessage.addAttribute(mxconst::get_ATTRIB_LABEL_COLOR().c_str(), m.label_color.c_str());
      xMessage.addAttribute(mxconst::get_ATTRIB_LABEL_PLACEMENT().c_str(), m.label_position.c_str());
      xMessage.addClear(m.message_text.c_str());
    }

  }
}

// -----------------------------------

bool
missionx::QueueMessageManager::loadCheckpoint(ITCXMLNode& inParent, std::deque<missionx::messageLine_strct>& outMessages, std::string& outErr)
{
  outErr.clear();
  outMessages.clear();

  ITCXMLNode xMessages = inParent.getChildNode(mxconst::get_ELEMENT_MXPAD_ACTIVE_MESSAGES().c_str());
#ifndef RELEASE
  std::string debugXMessages = missionx::data_manager::xmlRender.getString(xMessages);
  missionx::data_manager::xmlRender.clear();
#endif
  if (!xMessages.isEmpty())
  {
    //const missionx::mxconst mx_const; // v25.04.2
    int nChilds = xMessages.nChildNode(mxconst::get_ELEMENT_MESSAGE ().c_str());
    for (int i = 0; i < nChilds; i++)
    {
      ITCXMLNode  xChild = xMessages.getChildNode(i);
      messageLine_strct line;

      std::string val;
      val.clear();

      line.label          = Utils::trim(Utils::readAttrib(xChild, mxconst::get_ATTRIB_LABEL(), ""));
      line.label_color    = Utils::trim(Utils::readAttrib(xChild, mxconst::get_ATTRIB_LABEL_COLOR(), ""));
      line.label_position = Utils::trim(Utils::readAttrib(xChild, mxconst::get_ATTRIB_LABEL_PLACEMENT(), ""));

      line.message_text = (xChild.nClear() > 0) ? xChild.getClear().sValue : missionx::EMPTY_STRING;
      line.message_text = mxUtils::trim(line.message_text);

      Utils::sentenceTokenizerWithBoundaries(line.textLines, line.message_text, mxconst::get_SPACE(), mxconst::MXPAD_LINE_WIDTH);
      line.textLines.clear(); // v3.305.1
      outMessages.push_back(line);
    }
  }


  return true;
}

// -----------------------------------

bool
missionx::QueueMessageManager::addMessageToQueueList(missionx::Message& inMsgInstance, std::string& outErr)
{
  outErr.clear();
  std::string message = inMsgInstance.getMessage();

  // Skip if same as last message
  if (!listPadQueueMessages.empty())
  {
    missionx::Message lastMessage = listPadQueueMessages.back();
    if (lastMessage == inMsgInstance)
    {
      outErr = "New message is same as last message in queue. Skipping...";
      Log::logMsgErr(outErr); // debug
      return false;
    }
  }

  // add to Queue
  if (QueueMessageManager::listPadQueueMessages.size() <= MAX_MX_PAD_MESSAGES) // modified v3.0.110
  {
    missionx::QueueMessageManager::listPadQueueMessages.push_back(inMsgInstance);
  }
  else
  {
    outErr = "[add pad msg]Can't add message to queue. Queue is full.";
    Log::logMsgWarn(outErr);
    return false;
  }



  return true;
}

// -----------------------------------
// This function is called from the
void
missionx::QueueMessageManager::addMessageToTrackedMap(Message inMsg)
{
  // check if a tracked message with exact name is present. If so, then change counter and override the message string.
  if (Utils::isElementExists(QueueMessageManager::mapTrackedMessages, inMsg.trackName))
  {
    inMsg.msgCounter += QueueMessageManager::mapTrackedMessages[inMsg.trackName].msgCounter; // will reflect cumulative
    QueueMessageManager::mapTrackedMessages[inMsg.trackName].flc();
  }
  else
  {
    Utils::addElementToMap(QueueMessageManager::mapTrackedMessages, inMsg.trackName, inMsg); // What if message trackName(key) is already present ?
    QueueMessageManager::mapTrackedMessages[inMsg.trackName].flc();
  }
}


// -----------------------------------

void
missionx::QueueMessageManager::pauseAllPoolChannels()
{
  for (auto& itMsgInstance : QueueMessageManager::listPoolMsg)
  {
    const std::string key = itMsgInstance.getName();
    for (auto &iter2 : itMsgInstance.mapChannels)
    {
      if ((itMsgInstance.mapChannels[key].sound != nullptr) || (itMsgInstance.mapChannels[key].channel != nullptr))
        QueueMessageManager::sound.setPauseSound(true);
    }
  }
}

// -----------------------------------

void
missionx::QueueMessageManager::unPauseAllPoolChannels()
{
  for (auto itMsgInstance : QueueMessageManager::listPoolMsg)
  {
    const std::string key = itMsgInstance.getName();
    for (auto &iter2 : itMsgInstance.mapChannels)
    {
      if ((itMsgInstance.mapChannels[key].sound != nullptr) || (itMsgInstance.mapChannels[key].channel != nullptr))
        QueueMessageManager::sound.setPauseSound(false);
    }
  }
}

// -----------------------------------

void
missionx::QueueMessageManager::stopAllPoolChannels()
{
  for (auto itMsgInstance : QueueMessageManager::listPoolMsg)
  {
    const std::string key = itMsgInstance.getName();
    for (auto& [chName, sf] : itMsgInstance.mapChannels)
    {
      //if ((itMsgInstance.mapChannels[key].sound != nullptr) || (itMsgInstance.mapChannels[key].channel != nullptr))
      //  QueueMessageManager::sound.stopAndReleaseChannel(itMsgInstance.mapChannels[key]);
      if ((sf.sound != nullptr) || (sf.channel != nullptr))
        QueueMessageManager::sound.stopAndReleaseChannel(sf);
    }
  }

  // v3.0.223.7 stop all background playing channels
  for (auto& [key, sf] : QueueMessageManager::mapPlayingBackgroundSF)
    QueueMessageManager::sound.stopAndReleaseChannel(sf);

  QueueMessageManager::flag_message_is_broadcasting = false; // v3.0.209.1
}


// -----------------------------------
