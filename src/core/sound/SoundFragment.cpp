#include "SoundFragment.h"
#include "fmod_errors.h"
#include <assert.h>

namespace missionx
{
unsigned int      SoundFragment::seqKey     = 0; // we will have to reset seqKey in QMM when reseting QMM on "START_MISSION"
const std::string SoundFragment::prefixName = "sf";
}

// -------------------------

std::string
missionx::SoundFragment::getSoundFile()
{
  return Utils::readAttrib(this->node, mxconst::get_ATTRIB_SOUND_FILE(), "");
}

// -------------------------

void
missionx::SoundFragment::setSoundFile(std::string& inFileName)
{
  setStringProperty(mxconst::get_ATTRIB_SOUND_FILE(), inFileName);
}

// -------------------------

missionx::SoundFragment::SoundFragment()
{
  reset();
  this->name = generateName();
}

// -------------------------

missionx::SoundFragment::SoundFragment(std::string inName, std::string inChannelTypeAsString, std::string inSoundFile, int inSoundVol, float inSecondsToPlay, missionx::enums::mx_timer_type inTimerType)
{
  reset();
  this->name = inName;
  this->setStringProperty(mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), inChannelTypeAsString);
  this->setSoundFile(inSoundFile);
  this->setNodeProperty<int>(mxconst::get_ATTRIB_SOUND_VOL(), inSoundVol);
  this->setNodeProperty<int>(mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_PLAY(), (int)inSecondsToPlay);
  this->setNodeProperty<int>(mxconst::get_ATTRIB_TIMER_TYPE(), (int)inTimerType);

  this->applyPropertiesToLocal();
}
// -------------------------

// missionx::SoundFragment::~SoundFragment()
//{
//
// }
// -------------------------


FMOD_OPENSTATE
missionx::SoundFragment::getOpenSoundFileState()
{
  if (this->sound == nullptr)
    return FMOD_OPENSTATE_ERROR;

  bool           state1, state2;
  state1 = state2 = false;

  auto result = this->sound->getOpenState(&this->openState, 0, &state1, &state2);
  if (result != FMOD_OK)
  {
    checkResult(result);
    return FMOD_OPENSTATE_ERROR;
  }

  return this->openState;
}

// -------------------------

FMOD_RESULT
missionx::SoundFragment::checkResult(FMOD_RESULT result)
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

void
missionx::SoundFragment::removeActiveCommand()
{
  if (!this->qSoundCommands.empty())
  {
    this->lastCommand = this->qSoundCommands.front();
    this->qSoundCommands.pop();
  }
}


// -------------------------


void
missionx::SoundFragment::reset()
{
  sound                          = nullptr;
  channel                        = nullptr;
  areWeWaitingForSoundFileToLoad = false;
  isSoundFileReadyToBePlayed     = false;
  isRepeating                    = false;
  volume                         = (float)mxconst::DEFAULT_SETUP_MISSION_VOLUME_I;
  volumeToPlay                   = (float)mxconst::DEFAULT_SETUP_MISSION_VOLUME_I * 0.01f;

  mxUtils::purgeQueueContainer(qSoundCommands); // v3.305.1
  
}

// -------------------------

unsigned int
missionx::SoundFragment::getSeqKey()
{
  const auto val = missionx::SoundFragment::seqKey;
  ++missionx::SoundFragment::seqKey; // v3.0.223.6 fixed code, we returned the value before promoting it. Oops.

  return val;
}

// -------------------------

std::string
missionx::SoundFragment::generateName()
{
  return std::string(SoundFragment::prefixName) + mxUtils::formatNumber<unsigned int>(getSeqKey());
}

// -------------------------

void
missionx::SoundFragment::storeCoreAttribAsProperties()
{                                                               // volume here represent user defined volume not FMOD float one.
  setNodeStringProperty(mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxUtils::translateMessageChannelTypeToString(this->channelType)); // // Mix channel type
  setNodeProperty<int>(mxconst::get_ATTRIB_SOUND_VOL(), (int)volume);                                                                   // volume here represent user defined volume not FMOD float one.
}

// -------------------------

void
missionx::SoundFragment::setVolume(const float& inVol) // v3.0.303.6
{
  this->setNodeProperty<float>(mxconst::get_ATTRIB_SOUND_VOL(), inVol); // v3.0.241.1 update node with volume change
  this->applyPropertiesToLocal();
}

// -------------------------

void
missionx::SoundFragment::applyPropertiesToLocal()
{
  assert(!this->node.isEmpty());

  std::string err;
  err.clear();

  this->volume = Utils::readNodeNumericAttrib<float>(this->node, mxconst::get_ATTRIB_SOUND_VOL(), (float)mxconst::DEFAULT_SETUP_MISSION_VOLUME_I); // (float)getPropertyValue<int>(ATTRIB_SOUND_VOL, err); // volume here represent user defined volume not FMOD float one.
  if (volume < 0.0f || volume > mxconst::MAX_SOUND_VOLUME_F)
  {
    // v3.306.1b use original volume settings to reset the volume
    //float originalVolume = Utils::readNodeNumericAttrib<float>(this->node, mxconst::get_ATTRIB_ORIGINAL_SOUND_VOL(), (float)mxconst::DEFAULT_SETUP_MISSION_VOLUME_I);    
    //volume = (originalVolume < 0.0f)?(float)mxconst::DEFAULT_SETUP_MISSION_VOLUME_I : originalVolume;

    volume = (float)mxconst::DEFAULT_SETUP_MISSION_VOLUME_I;

    this->setNodeProperty<float>(mxconst::get_ATTRIB_SOUND_VOL(), volume); //, this->node, mxconst::get_ELEMENT_MIX()); // v3.0.241.1 update node with volume change
  }

  if (volume >= mxconst::MIN_SOUND_VOLUME_F) // v3.0.303.6
    this->volumeToPlay = volume / 100.0f;

  std::string typeStr = Utils::readAttrib(this->node, mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), ""); // Mix channel type
  this->channelType   = mxUtils::translateMessageTypeToEnum(typeStr);

}


// -------------------------
// -------------------------
// -------------------------
