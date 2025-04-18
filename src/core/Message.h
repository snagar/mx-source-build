#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <unordered_map>
#include <deque>
#include "../core/xx_mission_constants.hpp"
#include "../core/mx_base_node.h"
#include "../data/mxProperties.hpp" // used in seed info for external scripts
#include "Timer.hpp"
#include "sound/Sound.h"
#include "sound/SoundFragment.h"
#include "../ui/core/TextureFile.h"

using namespace missionx;
// using namespace mxconst;

namespace missionx
{

typedef struct _mx_character
{
  mxRGB       color;
  std::string code;
  std::string label;

  missionx::mxVec4 getColorAsVec4() { return missionx::mxVec4(color.r, color.g, color.b, 1.0f); }

} mx_character;

// the order of this _mx_line_state is important
typedef enum class _mx_line_state : uint8_t
{
  undefined = 0,
  parsing_failure,
  //loading_failure,
  action_ended,
  init,
  parsing,
  parsing_ok,
  loading,  // used with image action init
  ready,    // image loaded and ready, text is ready, pause is ready, choice is ready  
  broadcasting,  // used with text line and not action line
  broadcasting_paused, // used when broadcasting is done and the pause action is displayed.
  mainMessageEnded,

} enum_mx_line_state;

typedef struct _mx_line_action_strct
{
  missionx::enum_mx_line_state state{ enum_mx_line_state::undefined };

  char actionCode = '\0';
  bool bUserPressNextInTextActionMode = false;
  bool isReady    = false;
  bool bIgnorePunctuationTiming{ false }; // Should we use special timer for specific punctuations. Defined once at the message level so we should not reset it every line.
  bool isInit = true;

  std::unordered_map<std::string, std::string> vals;
  mx_character                                 characterInfo;

  _mx_line_action_strct() { 
    init();
  }

  void init() { 
    isInit = true;

    state      = missionx::enum_mx_line_state::undefined; 
    actionCode = '\0';
    bUserPressNextInTextActionMode = false;
    isReady    = false;
    bIgnorePunctuationTiming       = false;
    vals.clear();
    characterInfo = mx_character(); // reset to default
  }

  // reset does not reset bUserPressNextInTextActionMode flag
  void reset() { 
    isInit     = false;
    state      = missionx::enum_mx_line_state::undefined; 
    actionCode = '\0';
    isReady    = false;
    vals.clear();
    characterInfo = mx_character(); // reset to default
  }
} mx_line_action_strct;




class Message : public missionx::mx_base_node //: public mxProperties
{
private:
  std::string errMsg;

public:
  void setName(std::string inName);
  void setTrackedName(std::string inTrackedName);


  Timer                           msgTimer; // holds the time the message is being displayed
  missionx::mx_message_state_enum msgState;

  //bool   is_pad;
  int    msgCounter; // how many time message was broadcast.

  missionx::mx_msg_mode mode; // v3.305.1

  std::string broadcastFor; // holds the names of objects that message was broadcast for.
  std::string trackName;    // will be used for tracking messages inside queue message manager

  std::map<std::string, missionx::SoundFragment> mapChannels;           // will hold Sound fragments information
  IXMLNode                                       xml_nodeTextTrack_ptr; // will hold "text"
  IXMLNode                                       xml_nodeCommTrack_ptr; // v3.0.303.7 will hold "comm"
  std::queue<missionx::mx_track_instructions_strct> qSoundCommands;        // v3.0.303.6 holds all sound commands that needs to be handle serially 

  static std::unordered_map<std::string, missionx::mxTextureFile> mapStoryCachedImages;    // v3.305.1  
  static std::vector<missionx::mxTextureFile*>                    vecStoryCurrentImages_p; // v3.305.1 // Make sure to reserve: Message::VEC_STORY_IMAGE_SIZE_I elements, in vector
  static constexpr const size_t                                   VEC_STORY_IMAGE_SIZE_I{ 7 }; // v3.305.1
  static constexpr const int                                      MAX_PAUSE_TIME_SEC{ 60 }; // v3.305.1

  constexpr const static int IMG_LEFT = 0; // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static int IMG_RIGHT = 1; // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static int IMG_CENTER = 2; // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static int IMG_BACKROUND = 3; // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static int IMG_LEFT_MED = 4; // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static int IMG_RIGHT_MED = 5; // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static int IMG_CENTER_MED = 6; // v3.305.1 image pointer position in vecStoryCurrentImages_p 

  constexpr const static char* IMG_POS_S_LEFT       = "l";  // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static char* IMG_POS_S_RIGHT      = "r";  // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static char* IMG_POS_S_CENTER     = "c";  // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static char* IMG_POS_S_BACKROUND  = "bg";  // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static char* IMG_POS_S_LEFT_MED   = "lm";    // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static char* IMG_POS_S_RIGHT_MED  = "rm";        // v3.305.1 image pointer position in vecStoryCurrentImages_p 
  constexpr const static char* IMG_POS_S_CENTER_MED = "cm";        // v3.305.1 image pointer position in vecStoryCurrentImages_p 

  std::unordered_map<std::string, mx_character> mapCharacters; // v3.305.1 k=name value=color
  std::unordered_map<std::string, mx_character> parseCharacterAttribute(); // v3.305.1
  std::deque<std::string>                       parseStoryMessage();       // v3.305.1
  std::deque<std::string>                       dqMsgLines; // v3.305.1
  
  static missionx::mx_line_action_strct lineAction4ui;
  bool                                  parse_action(char action, std::string inLine); // Parse the action code "i/t/c/p" and stores in he Message::lineAction4ui.vals[] map.

  // constructors / distractors
  Message();
  Message(std::string inMsgName, std::string msgText);
  Message(std::string inMsgName, std::string msgText, std::string inTrackName, std::string muteNarator = mxconst::get_MX_FALSE(), std::string hideText = mxconst::get_MX_FALSE(), std::string inEnabled = mxconst::get_MX_TRUE(), std::string inOverrideDisplayTextSecond = "8"); // default to 8 seconds
  //~Message();

  // xml related // v3.0.241.1
  bool parse_node(const bool inConsumeWarnings = false); // v3.305.3 added this flag for timing cases only.

  // members
  void init();
  void reset(); // calls init + stores properties

  // property members
  std::string getMessage();
  void        setMessage(std::string inMsg);
  void        setMessage(Message& inMsg);
  void        set_mxpad_properties(std::string inLabel, std::string inLabelPlace, std::string inColor, std::string inImage = EMPTY_STRING);

  // Operators //////////

  // operator members
  void clone(Message& inMsg)
  {
    // (missionx::mx_base_node)(*this) = (missionx::mx_base_node)inMsg;
        
    this->msgTimer     = inMsg.msgTimer;
    this->msgState     = inMsg.msgState;
    this->msgCounter   = inMsg.msgCounter;
    this->broadcastFor = inMsg.broadcastFor;
    this->trackName    = inMsg.trackName;
    this->errMsg       = inMsg.errMsg;

#ifdef APL

    this->mapChannels.clear();
    for (auto c : inMsg.mapChannels)
      Utils::addElementToMap(this->mapChannels, c.first, c.second);

#else
    this->mapChannels = inMsg.mapChannels;
#endif
    //this->is_pad = inMsg.is_pad; // v3.0.110 fix bug when copying messages type pad

    storeCoreAttribAsProperties();
  }


  bool operator==(Message& inMsg) { return (this->getMessage().compare(inMsg.getMessage()) == 0); }


  // other members
  void        flc();
  std::string to_string();
  std::string translateMessageState(mx_message_state_enum mState); // v3.0.109

  std::string getChannelNameWithType(missionx::mx_message_channel_type_enum inType); // If return EMPTY string, it means no channel by that type was found.
  bool        releaseChannel(SoundFragment& sf, MxSound& sound);                     // v3.0.109
  bool        removeChannel(std::string inName, MxSound& sound);                     // v3.0.109

  void setMessageText(std::string inText) { this->setStringProperty(mxconst::get_ATTRIB_MESSAGE(), inText); } // v3.0.211.1


  // checkpoint
  void storeCoreAttribAsProperties();
  void applyPropertiesToLocal();
  void saveCheckpoint(IXMLNode& inParent);
  void prepareCueMetaData(){}; // dummy function for the read_mission_file template: "load_saved_elements"

  // Timer
  void stopTimer();

  // v3.305.3 
  std::string get_and_filter_next_line(std::string& outAction, bool& outIsActionLine_b, int& outLineCounter);

  // v3.305.4
  //void addStopInstructionCommand(); // v3.305.4 after last instruction pop out, we should add the "stop" instruction

  // The function will return a mxProperties that will hold the key/value to seed to script
  missionx::mxProperties getInfoToSeed();
};

} // missionx
#endif
