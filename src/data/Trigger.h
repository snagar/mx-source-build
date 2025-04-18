#ifndef TRIGGER_H_
#define TRIGGER_H_

#pragma once

#include "mx_colors.hpp"
#include "../core/coordinate/Points.hpp"
#include "../core/Timer.hpp"
#include "../io/IXMLParser.h" // v3.0.241.1
#include "../ui/gl/CueInfo.h" // v3.0.202a
#include "mxProperties.hpp"

using namespace missionx;
// using namespace std;

namespace missionx
{
typedef enum class _trigger_state
  : uint8_t
{
  never_triggered = 0,
  trig_fired,                 // v3.0.213.4 deprecated entered, // = triggered, is one time pointState that fires "script_name_when_enter" code.
  inside_trigger_zone,        // plane in trigger area or logic function.
  left,                       // = plane left area. One time event, it fires: "script_name_when_left". The plug-in will alter the pointState to "was_not_triggered"
  wait_to_be_triggered_again, // = triggered at least one time
  never_trigger_again         // = triggered at least once, but we do not want to trigger it again.
} mx_trigger_state_enum;

// trigger elevation enum represent the volume type created. There must be some kind of min and max to create a volume
typedef enum class _trigger_elev
  : uint8_t
{
  script,
  not_defined, // important when we just want physical area and no elevation check
  on_ground,
  below,
  min_max,
  above,
  in_air // v3.0.213.4 if designer did not define volume, plugin will assume in_air
} mx_trigger_elev_type_enum;



class Trigger : public Points
{
protected:
  std::string  err;
  mx_cue_types cueType;

public:
  using Points::Points;
  using Points::saveCheckpoint;
  using Points::to_string;

  // v3.305.1c holds the state of the trigger to evaluate if allConditionsAreMet
  bool flag_inPhysicalArea          {false};
  bool flag_inRestrictElevationArea {false};
  bool flag_isOnGround              {false};

  bool flag_inPhysicalArea_fromThread   {false}; // v3.305.2

  Trigger();
//  ~Trigger();

  // core attributes for triggers
  bool     parse_node();    // v3.0.241.1 parse the xml node to see if valid
  IXMLNode xConditions;     // v3.0.241.1
  IXMLNode xOutcome;        // v3.0.241.1
  IXMLNode xLocAndElev_ptr; // v3.0.241.1

  // special elements
  IXMLNode xRadius;         // v3.0.241.1
  IXMLNode xRect;           // v3.0.253.5 used in poly triggers to create automated area based on 1 x Point and rectangle definition
  IXMLNode xElevVol;        // v3.0.241.1
  IXMLNode xReferencePoint; // v3.0.241.1
  IXMLNode xSetDatarefs;    // v3.303.12
  IXMLNode xSetDatarefs_on_exit; // v3.303.12

  void xml_subNodeDiscovery();


  bool        isEnabled;
  bool        isLinked; // is this trigger linked to another object ? if so, then it will be skipped in the stand alone tests.
  std::string linkedTo; // we can add here the name of the objects that are linked to this trigger, for debugging

  bool bAllConditionsAreMet; // used to flag any trigger as success. Can be combined with bScriptCondMet.
  bool bScriptCondMet;    // used to only flag scripts defined in "cond_script" as success. distinguish between a condition script and other script results in trigger.
  bool bEnteringPhysicalAreaMessageFiredOnce{ false }; // v3.305.1b Needs ""Location + Elevation + onGround". used only with physical based triggers to send a message before all conditions are met. Example: "shutdown engine and pull the park break".
  bool bPlaneIsInTriggerArea{ false }; // v3.305.1c Only need "Location + Elevation" used with "when_leave" outcome. We can only leave a trigger that the plane was in it. This is not equal to "all_conditions_are_met_b"

  // core enum
  mx_trigger_state_enum     trigState;
  mx_trigger_elev_type_enum trigElevType;

  missionx::Timer timer; // calculates time trigger needs to be valid before it flagged "entered"

  // members
  std::string getName() { return this->name; } // to solve template flow, needs class to support getName
  void        applyPropertiesToLocal();
  void        eval_physical_conditions_are_met(missionx::Point &p); // v3.305.1.c
  bool        eval_all_conditions_are_met_exclude_timer(); // v3.305.1.c

  void re_arm(); // v3.0.219.1

  bool               parseElevationVolume(std::string inMinMax, std::string& outErr);
  static std::string translateTrigElevType(mx_trigger_elev_type_enum inType);
  static std::string translateTrigState(mx_trigger_state_enum inState);
  void               progressTriggerStates(); // progress "enter" and "left" states

  // location
  bool isInPhysicalArea(Point& pObject);
  bool isInElevationArea(Point& pObject);
  bool isOnGround(Point& pObject); // v3.305.1c
  bool isPlaneInSlope();

  // checkpoint
  void storeCoreAttribAsProperties();
  void saveCheckpoint(IXMLNode& inParent);
  //bool loadCheckpoint(ITCXMLNode& inParent, std::string& outErr);

  // The function will return a mxProperties that will hold the key/value to seed to script
  missionx::mxProperties  getInfoToSeed();

  // this function is relevant only for triggers that the designer did not provide the center point
  void calcCenterOfArea();

  // clean condition data
  void clear_condition_properties(); // v3.0.223.4 for leg_message
  // clean outcome data
  void clear_outcome_properties(); // v3.0.223.4 for leg_message


  // debug
  std::string to_string();
  const std::string get_string_debug_as_header(); // v3.305.2
  const std::string to_string_ui_info(); // v3.305.2  
  const std::string to_string_ui_info_gist(); // v3.305.3
  const std::string to_string_ui_info_conditions(); // v3.305.3
  // v3.305.3 debug struct
  typedef struct __mx_debug 
  {
    mx_trigger_state_enum original_trigState           = missionx::mx_trigger_state_enum::never_triggered; // will hold the trigger real state before switching with the debug state
    mx_trigger_state_enum force_trigState              = missionx::mx_trigger_state_enum::never_triggered;
    bool                  bForceDebug                  = false;
    bool                  flagWasEvaluatedSuccessfully = false; // v3.305.3 if evaluation reached the last stage of "fire/leave".

    long long                                          timePassed = 0; // v3.305.3
    ImVec4                                             color      = missionx::color::color_vec4_white;
    std::chrono::time_point<std::chrono::steady_clock> last_run_os_clock;
    std::string                                        sStartExecutionTime, sEndExecutionTime;

    __mx_debug() { 
      reset();
    }

    void reset(bool inIncludeTimings = true)
    {
      this->original_trigState = missionx::mx_trigger_state_enum::never_triggered;
      this->force_trigState    = missionx::mx_trigger_state_enum::never_triggered;
      this->bForceDebug        = false;

      if (inIncludeTimings)
      {
        this->sStartExecutionTime.clear();
        this->sEndExecutionTime.clear();
      }
    }

    void set_debug_state(const missionx::mx_trigger_state_enum inState)
    { 
      this->force_trigState = inState;
      this->bForceDebug     = true;
    }

    void store_and_switch_real_state(missionx::mx_trigger_state_enum &inState)
    { 
      this->original_trigState = inState;
      inState                  = this->force_trigState;
    }

    void restore_real_trig_state(missionx::mx_trigger_state_enum &inState)
    { 
      inState                  = this->original_trigState;
      this->reset(false);
    }

    bool eval_allConditionAreMetBasedOnDebugFlagAndState() 
    { 
      if (this->bForceDebug && this->force_trigState == missionx::mx_trigger_state_enum::never_triggered)
        return true; // test trigger enter outcome

      return false; // we want to test trigger left outcome
    }

    std::string get_string_of_debug_info ()
    { return std::string("Fired event: ").append( ((force_trigState == missionx::mx_trigger_state_enum::never_triggered) ? "Enter area" : "Leaving Area") );
    }

    void setWasEvaluatedSuccessfully(const bool inVal)
    {
      if (inVal)
      {
        this->last_run_os_clock = std::chrono::steady_clock::now();
        this->timePassed        = 0; // reset timer
      }

      this->flagWasEvaluatedSuccessfully = inVal;
    }


    void flcDebugColors() 
    { 
      missionx::color::func::flcDebugColors(this->flagWasEvaluatedSuccessfully, this->timePassed, this->color, this->last_run_os_clock);
    }
  } mx_debug;

  mx_debug strct_debug;

  ////////// cue /////////
private:
  CueInfo cue;

public:
  CueInfo  getCopyOfCueInfo() { return cue; };
  CueInfo& getCueInfo_ptr() { return cue; };
  void     prepareCueMetaData(); // v3.0.213.7

  // v3.0.207.1
  std::string getTriggerType()
  {
    return Utils::readAttrib(this->node, mxconst::get_ATTRIB_TYPE(), ""); // v3.0.241.1 
  }


  // v3.0.221.11
  void setCumulative_flag(bool inVal);
  void setAllConditionsAreMet_flag(bool inVal);
  void setTrigState(missionx::mx_trigger_state_enum inState);
};

}

#endif
