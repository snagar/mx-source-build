#include "Sound.h"
#include "fmod_errors.h"
#include "../../io/Log.hpp"
#include <fmt/core.h>

#ifndef IBM
  #include "../../io/system_actions.h"
#endif
missionx::MxSound::~MxSound(void) {}
// -------------------------

bool
missionx::MxSound::is_sf_state_valid(FMOD_OPENSTATE inState, bool bRepeating)
{
#ifndef RELEASE
  Log::logMsg("[is_sf_state_valid] state: " + Utils::formatNumber<int>(inState));
#endif // !RELEASE


  if (bRepeating && (  inState == FMOD_OPENSTATE_READY || inState == FMOD_OPENSTATE_PLAYING|| inState == FMOD_OPENSTATE_SETPOSITION) )
    return true;
  else if (inState == FMOD_OPENSTATE_READY)
    return true;
  else if (inState == FMOD_OPENSTATE_PLAYING)
    return true;

  return false;
}

// -------------------------

missionx::MxSound::MxSound(void)
{
  this->fmodSystem = nullptr;

  this->wasSoundInitSuccess = false;

  soundFilePath.clear();
  this->fileToPlay_fullPath.clear();
  result = FMOD_OK;

  bgChannelGroup = nullptr;
  commChannelGroup = nullptr;
  ////// WE ARE NOT USING SOUNDEX since we are not using FMOD Studio///////
  // deprecated until we will have to use advance sound scenario
  // memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
  // exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
  // exinfo.nonblockcallback = xx_nonblockcallback;

  // this->playVolume = 0.3f;
}

// -------------------------


FMOD_RESULT
missionx::MxSound::checkResult(FMOD_RESULT result)
{
  if (result != FMOD_OK)
  {
    std::string err(FMOD_ErrorString(result));
    err = "FMOD error! (code: " + Utils::formatNumber<int>(result) + ") " + err + "\n";
    Log::logMsgErr(err);
  }

  return result;
}

// -------------------------

bool
missionx::MxSound::isChannelPlaying(SoundFragment& sf)
{
  bool bIsPlaying = false;

  if (!this->wasSoundInitSuccess)
    return false;

  if (sf.channel != nullptr)
  {
    this->result = sf.channel->isPlaying(&bIsPlaying);
    this->checkResult(this->result);
  }

  return bIsPlaying;
}

// -------------------------
void
missionx::MxSound::initSound()
{

  this->initSoundResult     = FMOD::System_Create(&fmodSystem);
  this->wasSoundInitSuccess = (checkResult(this->initSoundResult) == FMOD_OK) ? true : false;


  if (this->wasSoundInitSuccess)
  {
    unsigned int apiVersion;
    unsigned int drvVersion;

    this->result = fmodSystem->getVersion(&apiVersion);
    checkResult(result);

    std::stringstream stream;
    stream << FMOD_VERSION;
    stream >> std::dec >> drvVersion;
    if (drvVersion < apiVersion)
    {
      Log::logMsg(std::string("FMOD lib version: ").append(Utils::formatNumber<unsigned int>(drvVersion)).append(", doesn't match header version - 0x00010814"));
    }
    else
      Log::logMsg(std::string("FMOD lib version: ").append(Utils::formatNumber<unsigned int>(drvVersion)));
  }
  else
    Log::logMsgErr("Failed to initialize the FMOD SOUND system. Error Code: " + Utils::formatNumber<int>(this->initSoundResult) );

  // added initialization for arrays
  if (this->wasSoundInitSuccess)
  {

    result = fmodSystem->init(32, FMOD_INIT_NORMAL, 0);
    checkResult(result);

    result = fmodSystem->createChannelGroup("missionx", &commChannelGroup);
    if (result != FMOD_OK)
      checkResult(result);

    result = fmodSystem->createChannelGroup("background", &bgChannelGroup);
    if (result != FMOD_OK)
      checkResult(result);
  }
}



// -------------------------

// new body for this function. Prepare specific sound for specific message type
FMOD_RESULT
missionx::MxSound::prepareStreamSoundFile(missionx::mx_message_state_enum& inMsgState, SoundFragment& sf)
{
  result = FMOD_ERR_FILE_BAD;
  if (inMsgState == missionx::mx_message_state_enum::msg_undefined)
  {
    Log::logMsg("Message is undefined state. !!!\nSkipping sound file !!!", format_type::error, false);
    return result;
  }

  if (!this->wasSoundInitSuccess)
    return result;

  // stop running channel and music before opening new sound file
  if (this->isChannelPlaying(sf)) // msg holds the type of channel. Adding sound mix settings should allow the designer to mix sound files.
  {
    this->stopAndReleaseChannel(sf);

  } // end reset sound play if active and new sound should not be mixed.

  ///////////////////////////////////////////
  //// Create Stream to play sound file ////
  //////////////////////////////////////////
  std::string err = "";
  this->fileToPlay_fullPath.clear();
  this->fileToPlay_fullPath = soundFilePath + Utils::readAttrib(sf.node, mxconst::get_ATTRIB_SOUND_FILE(), "");
  if (!err.empty())
    Log::logMsgErr(err); // debug

  #ifndef RELEASE 
  Log::logMsg("Try to create stream for sound file: " + this->fileToPlay_fullPath);
  #endif

  result = fmodSystem->createStream(this->fileToPlay_fullPath.c_str(), FMOD_NONBLOCKING, 0 /*&this->exinfo sounexinfo* always null*/, &sf.sound);
  checkResult(result);
  if (result == FMOD_OK)
  {
    sf.areWeWaitingForSoundFileToLoad = true;
  }
  else
    sf.areWeWaitingForSoundFileToLoad = false;

  return result;
}


//// new function - return specific channel sound file state
//FMOD_OPENSTATE
//missionx::MxSound::getOpenSoundFileState(FMOD::Sound* inSound)
//{
//  if (inSound == nullptr)
//    return FMOD_OPENSTATE_ERROR;
//
//  FMOD_OPENSTATE state;
//  bool           state1, state2;
//  state1 = state2 = false;
//
//  result = inSound->getOpenState(&state, 0, &state1, &state2);
//  if (result != FMOD_OK)
//  {
//    checkResult(result);
//    return FMOD_OPENSTATE_ERROR;
//  }
//
//  return state;
//}


// -----------------------------------

FMOD_OPENSTATE
missionx::MxSound::checkFileLoadState(SoundFragment& sf)
{
  //FMOD_OPENSTATE state = getOpenSoundFileState(sf.sound);
  return sf.getOpenSoundFileState();
}

// -----------------------------------

// new function - Play prepared stream for specific sound type
void
missionx::MxSound::playStreamFile(SoundFragment& sf)
{
  if (sf.sound != nullptr)
  {
    //const FMOD_OPENSTATE state = getOpenSoundFileState(sf.sound); // v24026 deprecated

    sf.getOpenSoundFileState(); // v24026 replaces state
    FMOD::ChannelGroup*  cg; // holds ChannelGroup pointer. If exists it means that we already set it once so we are probably repeating the stream
    result = sf.channel->getChannelGroup(&cg); // cg can be nullptr

    if (this->is_sf_state_valid(sf.openState, sf.isRepeating)) // replaced inIsRepeating with the state of "cg" pointer. If it is true, then it should be treated the same as repeating
    {

      FMOD::ChannelGroup* cg;
      result = sf.channel->getChannelGroup(&cg); // cg can be nullptr
      result = fmodSystem->playSound(sf.sound, cg, true, &sf.channel);        

      if (result != FMOD_OK)
      {
        checkResult(result);
        Log::logMsg(fmt::format("[()] Failed to play sound file: {}, State code: {}", __func__, sf.getSoundFile(), (int)result)); // v24026
      }
      else
      {

        result    = sf.channel->setVolume(sf.volumeToPlay);
        checkResult(result);
        //if (result == FMOD_OK) // PLAY THE FILE // v24026 deprecated
          result = sf.channel->setPaused(false);

        #ifndef RELEASE
        {
          float volToPlay = -1.0f;
          sf.channel->getVolume(&volToPlay);
        }
        #endif // !RELEASE


        int channelCounter = 0;

        if ( cg == nullptr && commChannelGroup != nullptr && this->bgChannelGroup != nullptr)
        {
          switch (sf.channelType) // v3.0.303.6
          {
            case missionx::mx_message_channel_type_enum::background:
              sf.channel->setChannelGroup(this->bgChannelGroup);
              result = bgChannelGroup->getNumChannels(&channelCounter);
              break;

            default: // comm channel - for text messages - missionx::mx_message_channel_type_enum::comm
              sf.channel->setChannelGroup(this->commChannelGroup);
              result = commChannelGroup->getNumChannels(&channelCounter);
              break;
          }


          if (result != FMOD_OK)
            checkResult(result);

        }

        #ifndef RELEASE
        {
          if (cg)
          {

            bool bOut        = false;
            result           = sf.channel->setPaused(false);
            result           = sf.channel->getPaused(&bOut);
            std::string sOut = (bOut) ? "yes" : "no";
            result = sf.channel->isPlaying(&bOut);
            sOut   = (bOut) ? "yes" : "no";
          }
        }
        #endif // !RELEASE


        sf.areWeWaitingForSoundFileToLoad = false;
        sf.isSoundFileReadyToBePlayed     = false;
      }
    }
    else
    {
      //Log::logMsg("[sound:playStream] sf: " + sf.getName() + " state, is not ready, you should wait for next iteration until it will be ready. ");
      Log::logMsg(fmt::format( "[sound:playStream] sf: {} state, is not ready: {}, you should wait for next iteration until it will be ready. ", sf.getName(), (int) sf.openState) );
      //if (sf.isRepeating)
      {
        sf.areWeWaitingForSoundFileToLoad = true;
        sf.isSoundFileReadyToBePlayed     = false;
      }
    }

  }
  else
  {
    Log::logMsg("[sound:playStream] sf: " + sf.getName() + " sound pointer is not set. Won't play file: " + sf.getSoundFile() );
  }

}

// -----------------------------------

void
missionx::MxSound::flc(SoundFragment& sf)
{
//#ifndef RELEASE
//  bool flagIsPlaying = true;
//  auto b = (sf.channel != nullptr) ? sf.channel->isPlaying(&flagIsPlaying) : false;
//  Log::logMsg("[sound:flc] " + sf.getName() + ", file: " + sf.getSoundFile() + ". Is playing: " + ((flagIsPlaying) ? "true" : "false") ); // debug
//#endif                                                                                                                                                                            // !RELEASE

  switch (sf.channelType)
  {
    case missionx::mx_message_channel_type_enum::background:
    {
      // handle instruction commands
      if (!sf.qSoundCommands.empty())
      {
        constexpr static const bool bUseOsClock = true; // v3.306.1c Should always be true  //(sf.timer_type == missionx::enums::mx_timer_type::os);
        #ifndef RELEASE
        Log::logMsg("[sound:flc] "+ sf.getName() + " (bg) (" + mxUtils::formatNumber<float>(sf.timerChannel.getSecondsPassed(bUseOsClock), 2) + "), file: " + sf.getSoundFile()); // debug
        #endif // !RELEASE


        auto& command = sf.qSoundCommands.front(); 
        if (sf.timerChannel.getState() == missionx::mx_timer_state::timer_not_set) // v3.305.4 make sure we start the timer
          missionx::Timer::start(sf.timerChannel, sf.timerChannel.getSecondsToRun());

        if (sf.timerChannel.getSecondsPassed(bUseOsClock) >= command.seconds_to_start_f)
        {
          sf.isRepeating           = false; // v24026 reseting state before command switch

          command.startingVolume_f = (command.bCommandIsActive) ? command.startingVolume_f : sf.volume;
          command.bCommandIsActive = true;
          command.increment_by_f   = 0.0f; // we always reset to 0 so after switch we won't change volume if the increment value is == false (0)
          switch (command.command)
          {
            case '+':
            {
              // calculate how much volume to increment in each step
              if (command.transition_time_f)
              {              
                command.increment_by_f = ((command.new_volume - sf.volume) / (command.transition_time_f - command.step_i)); // value should be positive since we increase
                #ifndef RELEASE
                Log::logMsg("[sound:flc] " + sf.getName() + ", [+] increment_by_f: " + mxUtils::formatNumber<float>(command.increment_by_f, 2) + ", " + sf.getName() + ", step: " + mxUtils::formatNumber<int>(command.step_i));
                #endif
              }
              else // immediate change
              {
                command.step_i = (int)command.transition_time_f;
                sf.setVolume((sf.volume < command.new_volume) ? command.new_volume : sf.volume);
                this->setBackgroundVolume(sf.volumeToPlay);
#ifndef RELEASE
                Log::logMsg("[sound:flc] " + sf.getName() + ", [+] immediate volume change to: " + mxUtils::formatNumber<float>(sf.volume, 2) + ", step: " + mxUtils::formatNumber<int>(command.step_i));
#endif              
              }
            }
            break;
            case '-':
            {
              // calculate how much volume to increment in each step
              if (command.transition_time_f)
              {
                command.increment_by_f = ((sf.volume - command.new_volume) / (command.transition_time_f - command.step_i));
#ifndef RELEASE
                Log::logMsg("[sound:flc] " + sf.getName() + ", [-] increment_by_f: " + mxUtils::formatNumber<float>(command.increment_by_f, 2) + ", step: " + mxUtils::formatNumber<int>(command.step_i));
#endif
              }
              else // immediate
              {
                command.step_i = (int)command.transition_time_f;                
                sf.setVolume((sf.volume > command.new_volume) ? (float)command.new_volume : sf.volume);
                this->setBackgroundVolume(sf.volumeToPlay);
#ifndef RELEASE
                Log::logMsg("[sound:flc] " + sf.getName() + ", [-] immediate volume change to: " + mxUtils::formatNumber<float>(sf.volume, 2) + ", step: " + mxUtils::formatNumber<int>(command.step_i));
#endif
              }
              
            }
            break;
            case '!':
            case 's': // stop
            case 'S': // stop
            {
#ifndef RELEASE
              Log::logMsg("[sound:flc] stopping channel: " + sf.getName() + ", step: " + mxUtils::formatNumber<int>(command.step_i));
#endif

              command.step_i = (int)command.transition_time_f;
              mxUtils::purgeQueueContainer(sf.qSoundCommands);
              //for (size_t i1 = 0; i1 < sf.qSoundCommands.size(); ++i1) // clear all commands in Q
              //  sf.qSoundCommands.pop();

              this->stopAndReleaseChannel(sf);
              sf.timerChannel.setEnd(); // will force release of this channel during next "QMM" iteration
              return; // exit function
            }
            break;
            case 'r': // repeat
            {
              #ifndef RELEASE
                Log::logMsg("[sound:flc] repeat channel: " + sf.getName() + ", At time: " + mxUtils::formatNumber<float>(sf.timerChannel.getSecondsPassed(bUseOsClock), 2) + ", step: " + mxUtils::formatNumber<int>(command.step_i) + ", new_volume: " + mxUtils::formatNumber<int>(command.new_volume) );
              #endif

              // if we define a new volume when repeating the immediately set it or we will use the current playing volume
              if (command.new_volume >= mxconst::MIN_SOUND_VOLUME_F)
              {
                sf.setVolume((float)command.new_volume);
                sf.channel->setVolume(sf.volumeToPlay);
              }
              if (sf.volume <= mxconst::MIN_SOUND_VOLUME_F)
              {
                sf.setVolume(sf.getAttribNumericValue<float>(mxconst::get_ATTRIB_ORIGINAL_SOUND_VOL(), (float)mxconst::DEFAULT_SETUP_MISSION_VOLUME_I));
                sf.channel->setVolume(sf.volumeToPlay);
              }

              sf.timerChannel.setEnd(); // End timer

              if (!command.bCalledRepeatAtLeastOnce)
              { // stop channel
                bool bIsPlaying = false;
                result     = sf.channel->isPlaying(&bIsPlaying);
                this->checkResult(result);
                if (bIsPlaying)
                {
                  result = sf.channel->setPaused(true);
                  checkResult(result);

                }
                command.bCalledRepeatAtLeastOnce = true;
              }
              
              #ifndef RELEASE
                Log::logMsg("[sound:flc] repeat channel: " + sf.getName() + ", " + mxUtils::formatNumber<int>(command.step_i) + ", volume: " + mxUtils::formatNumber<float>(sf.volume) + ", volumeToPlay: " + mxUtils::formatNumber<float>(sf.volumeToPlay));
              #endif // !RELEASE


              //sf.areWeWaitingForSoundFileToLoad = true; // v24026 because it is repeating
              sf.isRepeating                    = true; // v24026 because it is repeating
              this->playStreamFile(sf);                 // we have to call this function until the state of the sf buffer is set and it can play the stream again.
              if (!sf.areWeWaitingForSoundFileToLoad)
                missionx::Timer::start(sf.timerChannel, sf.timerChannel.getSecondsToRun(), sf.timerChannel.getName(), sf.timerChannel.getIsCumulative()); // restart timer so it won't be release in QMM::flc()



              //const FMOD_OPENSTATE state = getOpenSoundFileState(sf.sound);
              #ifndef RELEASE
              const FMOD_OPENSTATE state = sf.getOpenSoundFileState();
              #endif
              
              //TODO Consider removing the "if" condition since we should always remove the repeat ("r") command
              //if ((sf.sound != nullptr && this->is_sf_state_valid(state, command.bCalledRepeatAtLeastOnce)) || (state == FMOD_OPENSTATE_ERROR))
              //{
                sf.removeActiveCommand(); // v3.306.1b remove the command. It is important so we won't have an endless loop
              //}
              
              return; // exit flc
            }
            break;
            case 'l': // loop endlessly until code will stop it or mission will complete
            case 'L':
            {
              #ifndef RELEASE
                Log::logMsg("[sound:flc] Looping channel: " + sf.getName() + ", At time: " + mxUtils::formatNumber<float>(sf.timerChannel.getSecondsPassed(bUseOsClock), 2) + ", step: " + mxUtils::formatNumber<int>(command.step_i) + ", new_volume: " + mxUtils::formatNumber<int>(command.new_volume) );
              #endif
              
              // if we define a new volume when repeating the immediately set it or we will use the current playing volume
              if (command.new_volume >= mxconst::MIN_SOUND_VOLUME_F)
              {
                sf.setVolume((float)command.new_volume);
                sf.channel->setVolume(sf.volumeToPlay);
              }
              if (sf.volume <= mxconst::MIN_SOUND_VOLUME_F)
              {
                sf.setVolume(sf.getAttribNumericValue<float>(mxconst::get_ATTRIB_ORIGINAL_SOUND_VOL(), (float)mxconst::DEFAULT_SETUP_MISSION_VOLUME_I));
                sf.channel->setVolume(sf.volumeToPlay);
              }

              sf.timerChannel.setEnd(); // End timer
              //missionx::Timer::start(sf.timerChannel, sf.timerChannel.getSecondsToRun(), sf.timerChannel.getName(), sf.timerChannel.getIsCumulative()); // restart timer so it won't be release in QMM::flc()

              if (!command.bCalledRepeatAtLeastOnce)
              { // stop channel
                bool bIsPlaying = false;
                result     = sf.channel->isPlaying(&bIsPlaying);
                this->checkResult(result);
                if (bIsPlaying)
                {
                  result = sf.channel->setPaused(true);
                  result = sf.channel->stop();
                  checkResult(result);

                }

                command.bCalledRepeatAtLeastOnce = true;
                command.currentLoop_i++; // should be one (1), since it is the first time we are repeating
              }
              else if (command.loopFor_i > 0)
              {
                command.currentLoop_i++; // should be greater than one (1) since it will happen after the first call
              }
              
              #ifndef RELEASE
                Log::logMsg("[sound:flc] Looping channel: " + sf.getName() + ", " + mxUtils::formatNumber<int>(command.step_i) + ", volume: " + mxUtils::formatNumber<float>(sf.volume) + ", volumeToPlay: " + mxUtils::formatNumber<float>(sf.volumeToPlay));
              #endif // !RELEASE


              //sf.areWeWaitingForSoundFileToLoad = true; // v24026 because it is repeating
              sf.isRepeating                    = true; // v24026 because it is repeating
              this->playStreamFile(sf);                 // we have to call this function until the state of the sf buffer is set and it can play the stream again.
              if (!sf.areWeWaitingForSoundFileToLoad)
                missionx::Timer::start(sf.timerChannel, sf.timerChannel.getSecondsToRun(), sf.timerChannel.getName(), sf.timerChannel.getIsCumulative()); // restart timer so it won't be release in QMM::flc()

              //const FMOD_OPENSTATE state = getOpenSoundFileState(sf.sound);
              const FMOD_OPENSTATE state = sf.getOpenSoundFileState();
              
              // v3.305.4 Added loop restriction. Example if we define "120|L|1" it means that after one loop we should stop              
              if (   (sf.sound != nullptr && !this->is_sf_state_valid(state, command.bCalledRepeatAtLeastOnce)) 
                  || (command.loopFor_i > 0 && command.currentLoop_i >= command.loopFor_i) 
                  || (state == FMOD_OPENSTATE_ERROR)
                )
              {
                #ifndef RELASE
                bool bStatus = this->is_sf_state_valid(state, command.bCalledRepeatAtLeastOnce); // debug
                #endif 
                sf.removeActiveCommand(); // v3.306.1b remove the command. It is important so we won't have an endless loop
                Log::logMsg(fmt::format("Releasing Sound Fragment: {}, File: {}, Loop: {}, state: {}", sf.getName(), sf.getSoundFile(), command.loopFor_i, (int)state));
              }
              
              return; // exit flc
            }
            break;
            default:
              break;
          } // end switch

          // set volume based on "-/+" instructions
          if (command.increment_by_f) // if positive
          {
            sf.setVolume( sf.volume + command.increment_by_f * ((command.command == '+') ? 1.0f : -1.0f)); // increment/decrement according to command sign
            sf.channel->setVolume(sf.volumeToPlay);
#ifndef RELEASE
            Log::logMsg("[sound:flc] " + sf.getName() + ", Setting increment_by_f, volumeToPlay: " + mxUtils::formatNumber<float>(sf.volumeToPlay, 3) + ", step: " + mxUtils::formatNumber<int>(command.step_i));
#endif
          }
          ++command.step_i;
          if (!sf.qSoundCommands.empty() && (float)command.step_i >= command.transition_time_f)
          {
            command.bCommandWasEnded = true;
            sf.removeActiveCommand(); // v3.306.1b

            #ifndef RELEASE
            Log::logMsg("[sound:flc] " + sf.getName() + ", Pop out <mix> command. Is last: " + mxUtils::formatNumber<bool>(sf.qSoundCommands.empty()));
            #endif

          }
        }
      }
      else
      { // v3.305.1c Auto release channel if it does not play anymore
        if (!this->isChannelPlaying(sf))
        {
          #ifndef RELEASE
          Log::logMsg(">> [sound:flc] Releasing " + sf.getName() + ", For file: " + sf.getSoundFile()); // debug
          #endif

          this->stopAndReleaseChannel(sf);
          sf.timerChannel.setEnd();
        }
      } // if no commands
    }
    break;
    default:
      break;
  }
}

// -----------------------------------

void
missionx::MxSound::stopAndReleaseChannel(SoundFragment& sf)
{
  bool bIsPlaying = false;

  if (!this->wasSoundInitSuccess)
    return;

  if (sf.channel != nullptr)
  {
    bIsPlaying = false;
    result     = sf.channel->isPlaying(&bIsPlaying);

    result = sf.channel->stop();
    checkResult(result);

    //if (bIsPlaying)
    //{
    //  //FMOD::ChannelGroup *cg;
    //  //result = sf.channel->getChannelGroup(&cg);
    //  //if (cg)
    //  //{
    //  //  int n;
    //  //  result = cg->getNumChannels(&n);
    //  //  for (int i1 = 0; i1 < n; ++i1)
    //  //  {
    //  //    FMOD::Channel* cn = nullptr;
    //  //    result            = cg->getChannel(i1, &cn);
    //  //    if (checkResult(result) == FMOD_OK)
    //  //    {
    //  //        cn->stop();
    //  //    }
    //  //
    //  //  }
    //  //}
    //  result = sf.channel->stop();
    //  checkResult(result);
    //}
  }

  //FMOD_OPENSTATE state = this->getOpenSoundFileState(sf.sound);
  FMOD_OPENSTATE state = sf.getOpenSoundFileState();

  if (state != FMOD_OPENSTATE_ERROR)
  {
    if (sf.sound != nullptr)
    {
      result = sf.sound->release(); // might break plugin
      checkResult(result);
    }

    sf.reset();
  } // end if state not error and can release
}

// -----------------------------------

void
missionx::MxSound::stopAllPoolChannels()
{
  // to resolve calls before sound system initialized
  if (!this->wasSoundInitSuccess)
    return;

  // loop over ALL channels in channelGroup and stop them
  FMOD::Channel* c;
  bool           bIsPlaying    = false;
  int            numOfChannels = 0;
  result                       = commChannelGroup->getNumChannels(&numOfChannels);
  this->checkResult(result);

  for (int i = 0; i < numOfChannels; i++)
  {
    result = commChannelGroup->getChannel(i, &c); // get channel in group
    if (checkResult(result) == FMOD_OK)
    {
      bIsPlaying = false;
      result     = c->isPlaying(&bIsPlaying);
      this->checkResult(result);
      if (bIsPlaying && c != nullptr)
      {
        result = c->stop();
        checkResult(result);
      }
    } // end check channels in group
  }   // end loop over all channels in group
}

// -----------------------------------

unsigned int
missionx::MxSound::getSoundFileLengthInSec(SoundFragment& sf)
{
  unsigned int length = 0;

  if (!this->wasSoundInitSuccess)
    return 0;

  if (sf.sound != nullptr)
  {
    result = sf.sound->getLength(&length, FMOD_TIMEUNIT_MS);
    checkResult(result);
  }

  return length / 1000; // convert from milliseconds to seconds
}

// -----------------------------------

void
missionx::MxSound::setCommVolume(float inVolume)
{
  if (!this->wasSoundInitSuccess)
    return;

#ifndef RELEASE
  float volume{ 0.3f };
  {
    result = commChannelGroup->getVolume(&volume);
  }

#endif // !RELEASE

  if (commChannelGroup != nullptr)
  {
    result = commChannelGroup->setVolume(inVolume);
    checkResult(result);
  }
}

// -----------------------------------

void
missionx::MxSound::setBackgroundVolume(float inVolumeToPlay)
{
  if (!this->wasSoundInitSuccess)
    return;

  if (bgChannelGroup != nullptr)
  {
    result = bgChannelGroup->setVolume(inVolumeToPlay);
    checkResult(result);
  }
}

// -----------------------------------

void
missionx::MxSound::setMuteSound(bool inFlag)
{
  if (!this->wasSoundInitSuccess)
    return;

  if (commChannelGroup != nullptr)
  {
    result = commChannelGroup->setMute(inFlag);
    checkResult(result);
  }
}

// -----------------------------------

void
missionx::MxSound::setPauseSound(bool inFlag)
{
  if (!this->wasSoundInitSuccess)
    return;

  if (commChannelGroup != nullptr)
  {
    result = commChannelGroup->setPaused(inFlag);
    checkResult(result);
  }
}


// -------------------------

void
missionx::MxSound::update()
{
  if (fmodSystem != nullptr)
    fmodSystem->update();
}

// -------------------------
void
missionx::MxSound::setSoundFilePath(std::string inPath)
{
  this->soundFilePath = inPath;
}

// -------------------------
void
missionx::MxSound::release()
{

  if (fmodSystem != nullptr)
  {
#ifdef LIN
	#ifndef RELEASE
	  [[maybe_unused]]
	  const auto iLinuxFlavor_val = Utils::getNodeText_type_1_5<int>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_LINUX_FLAVOR_CODE_I(), 0); // v3.303.8.1
	#endif
	  if ((int)missionx::mx_linux_distro::arch_manjaro_garudo_endeavour != Utils::getNodeText_type_1_5<int>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_LINUX_FLAVOR_CODE_I(), (int)missionx::mx_linux_distro::debian_ubuntu_distro))
		fmodSystem->release();
#else
	    fmodSystem->release(); // calls close() internally
#endif

    this->wasSoundInitSuccess = false;
  }



} // release


// -------------------------


void
missionx::MxSound::testMp3()
{
  //  Log::logMsg("Create stream to MP3 file");
  //#ifdef IBM
  ////    result = fmodSystem->createStream("C:\\Fun\\hapil.mp3", FMOD_SOFTWARE | FMOD_LOOP_NORMAL | FMOD_NONBLOCKING, 0, &music);
  //  result = fmodSystem->createStream("C:\\Fun\\hapil.mp3", FMOD_DEFAULT, 0, &music);
  //#else
  //  std::string fullPathName = soundFilePath + "test.mp3";
  //  Log::logMsg( fullPathName );
  //#ifdef MAC
  //  char tmpPath[1024];
  //  if (  Utils::ConvertPath ( fullPathName.c_str(), tmpPath, 1000 ) == 0 )
  //  {
  //    fullPathName = tmpPath;
  //  }
  //  Log::logMsg( fullPathName );
  //
  //#endif
  //  result = fmodSystem->createStream( fullPathName.c_str() , FMOD_SOFTWARE, 0, &music);
  //#endif
  //  FmodErrorCheck(result);
  //
  //  Log::logMsg("Play MP3 file");
  //  result = fmodSystem->playSound(music, 0, false, &musicChannel);
  //  FmodErrorCheck(result);
  //  if ( result == FMOD_OK )
  //    musicChannel->setVolume( 0.3f );
}


// -------------------------
// -------------------------
// -------------------------
