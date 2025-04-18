#ifndef SOUND_H_
#define SOUND_H_

#pragma once

/**************

**************/


#include "fmod.hpp"
#include "SoundFragment.h"

namespace missionx
{

class MxSound
{
private:
  static const int MAX_SOUND_CHANNELS_NO = 4;
  std::string      errMsg;


public:
  MxSound(void);
  ~MxSound(void);

  static bool is_sf_state_valid(FMOD_OPENSTATE inState, bool bRepeating);

  // core attributes
  bool                wasSoundInitSuccess;
  std::string         soundFilePath;
  std::string         fileToPlay_fullPath;
  FMOD::System*       fmodSystem{ NULL };
  FMOD::ChannelGroup* commChannelGroup; /* (w) each channel that defined in the "playStream" will be added to this channelGroup that also named "missionx"  */
  FMOD::ChannelGroup* bgChannelGroup; /* (w) each channel that defined in the "playStream" will be added to this channelGroup that also named "missionx"  */

  void* extradriverdata = 0;

  FMOD_RESULT        initSoundResult;
  FMOD_RESULT        result;
  static FMOD_RESULT checkResult(FMOD_RESULT result);

  // core members
  void initSound(); // initialize SOUND System
  void update();    // Call every flc() for internal sound house keeping
  void release();   // stop all sound and release sound system. Will need new initialization

  // members related to sound file
  //FMOD_OPENSTATE getOpenSoundFileState(FMOD::Sound* sound);
  FMOD_OPENSTATE checkFileLoadState(SoundFragment& sf);
  FMOD_RESULT    prepareStreamSoundFile(missionx::mx_message_state_enum& inMsgState, SoundFragment& sf);
  void           playStreamFile(SoundFragment& sf);

  void setSoundFilePath(std::string inPath);
  bool isChannelPlaying(SoundFragment& sf); // is the channel for the specific  SoundFragment type is playing ?

  void flc(SoundFragment& sf); // v3.0.303.6
  void stopAndReleaseChannel(SoundFragment& sf);
  void stopAllPoolChannels();

  unsigned int getSoundFileLengthInSec(SoundFragment& sf);

  void         setCommVolume(float inVolume); // v3.0.303.6
  void         setBackgroundVolume(float inVolume); // v3.0.303.6
  void         setMuteSound(bool inFlag);
  void         setPauseSound(bool inFlag);

  void testMp3();
};

} // end namespace

#endif
