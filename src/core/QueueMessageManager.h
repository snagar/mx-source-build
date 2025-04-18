#ifndef QUEMESSAGEMANAGER_H_
#define QUEMESSAGEMANAGER_H_

#pragma once

#include <deque>
#include <list> // for GCC
#include <unordered_map>

#include "../core/dataref_manager.h" // v3.0.207.3
#include "../core/thread/base_thread.hpp"
#include "../core/xx_mission_constants.hpp"
#include "../io/system_actions.h" // to read options values
#include "Message.h"
#include "Utils.h"
#include "sound/Sound.h"

#include "data_manager.h" // v3.0.223.1

/******
The QueueMessageManager (aka QMM) class should manage and prioritize any kind of message or sound that should be broadcast to simmer.
The information must be in Message class format.
If we want to allow just receiving "one time" message, then we must construct a Message class from it.

QMM should store two different Queues, one for communication broadcast, and one for the mission notification Text messages (top right)
QMM will also store last 20th messages.

One of the challenges is synchronized different sound files at the same time, but that will be at later stage.

*/


namespace missionx
{
inline static const size_t MAX_MESSAGES_IN_POOL = 5;
//inline static const size_t MAX_MX_PAD_MESSAGES  = 20; // v3.0.110

class QueueMessageManager : public base_thread
{
private:
  static const float SECONDS_PER_LINE;
  static bool        muteSound; // v3.0.223.1 change to static
public:
  QueueMessageManager();
  //~QueueMessageManager();
  static void initStatic();

  // core attributes/members
  static std::list<missionx::Message>                   listPadQueueMessages;
  static std::map<std::string, missionx::SoundFragment> mapPlayingBackgroundSF; // v3.0.223.7
  static std::map<std::string, missionx::Message>       mapTrackedMessages;
  static std::list<missionx::Message>                   listPoolMsg;

  static MxSound sound;
  static Message messagePad; // only for PAD messages
  // members ////

  // 3.0.241.8
  static bool is_queue_empty(); // are no messages being broadcast or in message queue ?

  // from data_manager
  static int  msgSeq;
  static bool addMessageToQueue(const std::string& inMessageName, const std::string& inTrackedName, std::string& outErr);
  static void addTextAsNewMessageToQueue(std::string msgName, std::string msgText, std::string inTrackName = EMPTY_STRING, bool muteNarator = false, bool hideText = false, bool inEnabled = true, int inOverrideDisplayTextSecond = 0);
  // get Message name to search mapScriptMxPads, we assume that channel type is "pad",
  static bool setMessageChannel(std::string msgName, missionx::mx_message_channel_type_enum inChannelType, std::string inSoundFile, float inSecondsToplay, std::string& outErr, int inSoundVol = 30);


  // Will dispatch the message to COMM or 3D Text
  static bool addMessage(missionx::Message inMsg, std::string inTrackedName, std::string& outErr);

  // NON STATIC
  static void flc(); // main flight loop callback
  static void flc_comm();
  static void flc_msg_pool();
  static void progressMessage(Message* msg, int& inMsgInQueue_i); // receive MSG class and message in queue from listPoolMsg. Only the first one should be broadcast and not the second one even if it is ready.
  static void postMessage(Message* msg);
  static void fade_bg_channel(const std::string &inMessageName_s, const double& inSecondsUntilFadeOut_f); // v3.305.3


  // SAVE / LOAD
  static void saveCheckpoint(IXMLNode& inParent);
  static bool loadCheckpoint(ITCXMLNode& inParent, std::deque<missionx::messageLine_strct>& outMessages, std::string& outErr);

  static std::deque<missionx::messageLine_strct> mxpad_messages; // mxpad should use LIFO. mxpad text messages. We will try to store only last 50 messages

  static bool flag_message_is_broadcasting; // v3.0.209.1 - flag if XPLMSpeakString is active. If so, then we need to let other messages in the que to wait.

  static void stopAllPoolChannels(); // v3.305.1c moved to public

private:
  // CONSTANTS
  const static size_t MAX_LINES_ALLOWED_IN_MESSAGE = 10;

  static std::string            errMsg;
  static std::list<std::string> listEraseKeys;
  static std::list<std::string> listEraseBackgroundKeys;


  // members
  // Message will be added to the communication messages
  static bool addMessageToQueueList(missionx::Message& inMsg, std::string& outErr);
  static void addMessageToTrackedMap(Message inMsg);

  static bool soundChannelsArePaused;   // v3.0.207.3 optimization flag
  static void pauseAllPoolChannels();   // v3.0.207.3
  static void unPauseAllPoolChannels(); // v3.0.207.3

  static void prepareMessageStory(Message* msg);  // v3.305.1
  static void prepareMessageText(Message* msg, bool flag_message_has_comm_sf);                                     // v3.305.1
  static void playSoundFilesFromAllChannels(Message* msg, float secondsToDisplayText_userDefine); // v3.305.1
  static void evalCommChannelProgress(Message* msg, const bool flag_message_has_comm_sf, const std::string &key);  // v3.305.1


};

} // missionx

#endif
