#ifndef SOUNDFRAGMENT_H_
#define SOUNDFRAGMENT_H_

#pragma once

//#include "../../data/mxProperties.hpp"
#include "../mx_base_node.h"
#include "../Timer.hpp"
#include "fmod.hpp"
//#include "fmod_errors.h"

using namespace missionx;
// using namespace mxconst;

namespace missionx
{

// Holds information regarding current sound and Message <mix> element
class SoundFragment : public missionx::mx_base_node // missionx::mxProperties
{
private:
  std::string              errMsg;
  static unsigned int      seqKey;
  static const std::string prefixName; //
public:
  SoundFragment();
  SoundFragment(std::string inName, std::string inChannelTypeAsString, std::string inSoundFile, int inSoundVol, float inSecondsToPlay, missionx::enums::mx_timer_type inTimerType = missionx::enums::mx_timer_type::xp); // for channel creation. Used in addPadChannel/addCommChannel/addBackgroundChannel
  //virtual ~SoundFragment();

  FMOD_OPENSTATE openState; // v3.305.4
  FMOD_OPENSTATE getOpenSoundFileState();
  FMOD_RESULT checkResult(FMOD_RESULT result);

  void removeActiveCommand();

  //missionx::enums::mx_timer_type timer_type{ missionx::enums::mx_timer_type::xp };
  missionx::mx_track_instructions_strct lastCommand; // v3.306.1b


#ifdef APL
  void operator=(const missionx::SoundFragment &sf) {
    this->errMsg     = sf.errMsg;
    this->seqKey     = sf.seqKey;
    //this->timer_type = sf.timer_type; // v3.306.1

    this->sound                          = (sf.sound == nullptr) ? nullptr : sf.sound;
    this->channel                        = (sf.channel == nullptr) ? nullptr : sf.channel;
    this->areWeWaitingForSoundFileToLoad = sf.areWeWaitingForSoundFileToLoad;
    this->isSoundFileReadyToBePlayed     = sf.isSoundFileReadyToBePlayed;
    this->isRepeating                    = sf.isRepeating;
    this->volume                         = sf.volume;
    this->volumeToPlay                   = sf.volumeToPlay;
    this->timerChannel.clone(timerChannel);
    this->channelType    = sf.channelType;
    this->qSoundCommands = sf.qSoundCommands;
  }
#endif
  
  
  void                reset();
  static unsigned int getSeqKey();
  static std::string  generateName();

  // core attributes for Sound use
  FMOD::Sound*   sound;                          // used by Sound
  FMOD::Channel* channel;                        // used by Sound
  bool           isRepeating; // v24026 used by Sound when repeating command
  bool           areWeWaitingForSoundFileToLoad; // used by Sound
  bool           isSoundFileReadyToBePlayed;     // used by Sound


  // Mixture Channel info
  float volume;       // used by Mix
  float volumeToPlay; // used by Sound

  // mix members
  Timer                                  timerChannel;
  missionx::mx_message_channel_type_enum channelType;
  std::queue<missionx::mx_track_instructions_strct> qSoundCommands; // v3.0.303.6 holds all sound commands that needs to be handle serially 
  void                                              setInstructionsQueue(std::queue<missionx::mx_track_instructions_strct> inQ) { qSoundCommands = inQ; } // v3.0.303.6

  std::string getSoundFile();
  void        setSoundFile(std::string& inFileName);

  std::string getMixtureSoundType() { return Utils::readAttrib(this->node, mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), ""); }

  // savepoint
  void storeCoreAttribAsProperties();
  void setVolume(const float& inVol); // v3.0.303.6 will affect the volume settings in the xml <node> and then wil call the applyPropertiesToLocal() function
  void applyPropertiesToLocal();

  bool isPlaying() // v3.305.4
  {
    bool bIsPlaying = false;
    //if (this->sound && this->channel)
    if (this->sound )
    {      
      this->channel->isPlaying (&bIsPlaying);      

      return bIsPlaying;
    }

    return false;
  }

  std::string to_string() // v3.0.211.1
  {
    std::string format;
    format.clear();

    format += "type: " + getMixtureSoundType() + ", file name: " + getSoundFile() + ", volume: " + Utils::formatNumber<float>(volume) + mxconst::get_UNIX_EOL();

    return format;
  }
};
} // namespace

#endif
