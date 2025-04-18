#ifndef _EXT_SCRIPT_H_
#define _EXT_SCRIPT_H_
#pragma once
#include <assert.h>
#include <iostream>
#include <map>
#include <string>

#include "../data_manager.h"

//using namespace missionx;
//using namespace mxconst;

namespace missionx
{


class ext_script
{
private:
  static char emptyCharString; // v3.303.14 resolve warning when initializing *char with empty string
  // internal usage, if script sent key string correctly
  static bool checkString(const char* inKey, std::string& outKey);

  // this is an internal way to store and reuse information between two "ext_xxx" functions.
  // example: function "create_new_mxpad_message" calls "create_new_comm_message" and then need to call "set_message_as_pad".
  // we turn on flag "useInternalBuffer"
  static missionx::mxProperties internalBufferBetweenFunctions; // property buffer to move information between 2 functions for special cases. Use sparsingly.
  static bool                   canUseInternalBuffer;

public:
  ext_script();
  ~ext_script();



  static void register_my_functions(struct mb_interpreter_t* inBas);


  static int ext_set_global_bool(mb_interpreter_t* s, void** l);    //
  static int ext_get_global_bool(mb_interpreter_t* s, void** l);    //
  static int ext_remove_global_bool(mb_interpreter_t* s, void** l); // v3.0.112

  static int ext_set_global_int(mb_interpreter_t* s, void** l); //
  static int ext_get_global_int(mb_interpreter_t* s, void** l); //
  static int ext_get_xp_version(mb_interpreter_t* s, void** l); // v3.305.1b

  static int ext_get_or_create_global_int(mb_interpreter_t* s, void** l);    // v3.303.14
  static int ext_get_or_create_global_bool(mb_interpreter_t* s, void** l);    // v3.303.14
  static int ext_get_or_create_global_decimal(mb_interpreter_t* s, void** l);    // v3.303.14
  static int ext_get_or_create_global_string(mb_interpreter_t* s, void** l);    // v3.303.14
  static int ext_get_current_leg_desc(mb_interpreter_t* s, void** l);           // v3.305.3 get current flight leg description
  static int ext_set_leg_desc(mb_interpreter_t* s, void** l);           // v3.305.3 overrides flight leg description

  static int ext_set_global_decimal(mb_interpreter_t* s, void** l);    //
  static int ext_store_global_decimal(mb_interpreter_t* s, void** l);    //
  static int ext_get_global_decimal(mb_interpreter_t* s, void** l);      //

  static int ext_remove_global_number(mb_interpreter_t* s, void** l);    // v3.0.112 - for decimal and int numbers

  static int ext_is_global_number_exists(mb_interpreter_t* s, void** l); // v3.0.161
  static int ext_is_global_bool_exists(mb_interpreter_t* s, void** l);   // v3.0.161
  static int ext_is_global_string_exists(mb_interpreter_t* s, void** l); // v3.0.161

  static int ext_set_global_string(mb_interpreter_t* s, void** l);    //
  static int ext_get_global_string(mb_interpreter_t* s, void** l);    //
  static int ext_remove_global_string(mb_interpreter_t* s, void** l); // v3.0.112

  static int ext_get_dref_as_number(mb_interpreter_t* s, void** l); //
  static int ext_get_dref_as_string(mb_interpreter_t* s, void** l); //


  // set value for scalar parameters, not arrays
  static int ext_set_dref_value(mb_interpreter_t* s, void** l); //


  // TASK
  // Task information for external script (mxTaskState,mxTaskType,mxTaskActionName,mxTaskIsComplete,mx_enabled,script_conditions_met_b,mxTaskHasBeenEvaluate,mx_always_evaluate,mx_mandatory)
  static int ext_get_task_info(mb_interpreter_t* s, void** l);

  // allow ext script to reset Task timer. Script should provide the "Task" name. function returns true if success and mxError if fail
  static int ext_reset_task_timer(mb_interpreter_t* s, void** l);

  // Modify task property, script need to send three strings, the first one represent the task name, the second represent attribute and the third represents the value.
  // The function will convert to bool, int, float, double or string
  static int ext_set_task_property(mb_interpreter_t* s, void** l);
  static int ext_set_cargo_end_position(mb_interpreter_t* s, void** l); // v3.0.303 used with task based cargo. You have to define end position of cargo, which is the success position. Provide "task name", "lat" and "lon"




  // TRIGGER
  // return "bool" as success, and seed "bAllConditionMet", bScriptCondMet", "trigState", "isEnabled", "isLinked",
  static int ext_get_trigger_info(mb_interpreter_t* s, void** l);

  // allow ext script to reset triggers timer. Script should provide the "trigger" name. function returns true if success and mxError if fail
  static int ext_reset_trigger_timer(mb_interpreter_t* s, void** l);

  // Modify trigger property, script need to send three strings, the first one represent the task name, the second represent attribute and the third represents the value.
  // The function will convert to bool, int, float, double or string
  static int ext_set_trigger_property(mb_interpreter_t* s, void** l);

  // MESSAGE
  // construct new Message object and add it to data_manager::mapMessages
  // Function attributes: name(s), message(s), mute_xplane_narrator(b), hide_text(b), override_seconds_to_display_text(i)
  static int ext_create_new_comm_message(mb_interpreter_t* s, void** l); // v3.0.96
                                                                         // return "bool" as success, and seed "name, message text, enabled, message_override"
  static int ext_get_message_info(mb_interpreter_t* s, void** l);
  static int ext_set_message_property(mb_interpreter_t* s, void** l);
  // static int ext_set_message_as_mxpad(mb_interpreter_t * s, void ** l); // v3.0.241.1 This function is deprecated, since all messages are now mxpad.
  static int ext_set_message_label_properties(mb_interpreter_t* s, void** l); // v3.0.211

//  static bool set_channel(mb_interpreter_t* s, void** l, std::string funcName, char* inMsgName, missionx::mx_message_channel_type_enum inChannelType, char* inSoundFile, float inSecondsToplay, int inSoundVol, std::string& outErr);
  static bool set_channel(std::string funcName, char* inMsgName, missionx::mx_message_channel_type_enum inChannelType, char* inSoundFile, float inSecondsToplay, int inSoundVol, std::string& outErr);


  ////// mxconst::QMM - QueueMessageManager /////

  // The function will send message name + track name. The track name is optional so function can receive between 1 to 2 arguments.
  static int ext_send_comm_message(mb_interpreter_t* s, void** l);   // renames in v3.0.96
  static int ext_send_text_message(mb_interpreter_t* s, void** l);   // renamed in v3.0.100
  static int ext_end_current_message(mb_interpreter_t* s, void** l); // renamed in v3.0.223.7 - will set the message timer as finished so in next mxconst::QMM flc() it will call the post message function.
  static int ext_abort_current_message(mb_interpreter_t* s, void** l); // renamed in v3.0.223.7 - will set the message timer as finished, set message state to "abort_msg" and set background file timer as ended too. This will skip post message step.
  static int ext_abort_bg_channel(mb_interpreter_t* s, void** l); // v3.306.1b
  static int ext_abort_all_channels(mb_interpreter_t* s, void** l); // v3.306.1c
  static int ext_fade_out_bg_channel(mb_interpreter_t* s, void** l); // v3.306.1b
                                                                     



  static int ext_set_comm_channel(mb_interpreter_t* s, void** l);       // v3.0.109
  static int ext_set_background_channel(mb_interpreter_t* s, void** l); // v3.0.109

  // MXPAD messages //// MXPAD messages //// MXPAD messages //// MXPAD messages ////
  static int ext_is_msg_queue_empty(mb_interpreter_t* s, void** l); // v3.0.110 // check mxconst::QMM listPadQueueMessages if empty. Means, no messages should be broadcast.
  // most cases


  static int ext_set_task_property_in_objective(mb_interpreter_t* s, void** l); // v3.0.205.2 allow mxpad script to modify propertied in current goal task, due to the fact that all objectives and tasks are outside its script scope.
  static int ext_get_task_info_in_objective(mb_interpreter_t* s, void** l);     // v3.0.221.15rc4 allow linked triggers to goal/leg to read task information

  ////// INVENTORY RELATED //////////
  static int get_inventory_barcode_quantity(const std::string& inventoryName_s, const std::string& inBarcodeValueToSearch, std::string& errMsg);
  static int ext_is_item_exists_in_plane(mb_interpreter_t* s, void** l);         // v3.0.213.3
  static int ext_is_item_exists_in_ext_inventory(mb_interpreter_t* s, void** l); // v3.0.213.3

  static int ext_move_item_from_inv(mb_interpreter_t* s, void** l); // v3.0.221.15rc4 move item from any inventory to any inventory. If destination inventory is empty then discard
  static int ext_move_item_to_plane(mb_interpreter_t* s, void** l); // v24.12.2 should handle "station" elements too.
  static int ext_get_inv_layout_type(mb_interpreter_t* s, void** l); // v24.12.2 Returns layout compatibility. 11 for XP11 anything else, supports station.


  /////// Extended functionality functions //////////

  // function return distance of plane from certain coordination
  // Designer should send "lat", "long" as decimal values.
  // function return -1 if information is invalid and mxError string.
  static int ext_get_distance_to_coordinate(mb_interpreter_t* s, void** l);


  // send task or trigger name and receive a double + mxError.
  // If double = -1, then we did not find trigger or adequate information to calculate.
  static int ext_get_distance_to_reference_point(mb_interpreter_t* s, void** l);


  // uses: XPLMFindNavAid, XPLMGetNavAidInfo (XPLMNavRef + XPLMFindFirstNavAidOfType)
  static int ext_get_nav_info(mb_interpreter_t* s, void** l);

  //// END Element related functions
  static int ext_abort_mission(mb_interpreter_t* s, void** l); // script should send reason for abort
  static int ext_update_end(mb_interpreter_t* s, void** l); // receives 3 string params subelement (s), attrib_name (s), value (s)


  // v3.0.221.11+ execute commands
  static int ext_execute_commands(mb_interpreter_t* s, void** l);


  //////// FLight Leg functions //////////////
  static int ext_set_next_leg(mb_interpreter_t* s, void** l);         // v3.0.205.3
  static int ext_stop_draw_script(mb_interpreter_t* s, void** l);     // v3.0.223.1 remove draw script from leg
  static int ext_set_draw_script_name(mb_interpreter_t* s, void** l); // v3.0.223.1 modify the name of the draw script


  ////// TIMELAP RELATED //////////
  static int ext_timelapse_add_minutes(mb_interpreter_t* s, void** l);   // v3.0.221.15rc5 add minutes to local hour in timelapse
  static int ext_timelapse_to_local_hour(mb_interpreter_t* s, void** l); // v3.0.221.15rc5 add minutes to the next closest local hour. can be next day or same day.
  static int ext_is_timelapse_active(mb_interpreter_t* s, void** l);     // v3.0.221.15rc5 status of the timelap. If active then no reason to set it, maybe wait for next itteration.

  ////// TIME RELATED //////////
  static int ext_set_local_time(mb_interpreter_t* s, void** l); // v3.0.221.15rc5 hour, minutes, day_of_year 0-364

  ////// METAR RELATED //////////
  static int ext_inject_metar_file(mb_interpreter_t* s, void** l); // v3.0.223.1 inject metar file

  ////// Teleport functions //////////
  static int ext_position_plane(mb_interpreter_t* s, void** l); // v3.0.225.5 Expose XPLMPlaceUserAtLocation(p1.getLat(), p1.getLon(), (float)p1.getElevationInMeter(), (float)heading_psi, (float)startingSpeed);
  static int ext_position_camera(mb_interpreter_t* s, void** l); // v3.0.303.7 

  ////// Choice Window related functions //////////
  static int ext_set_choice_name(mb_interpreter_t* s, void** l);       // v3.0.231.1 set and initialize current choice
  static int ext_get_active_choice_name(mb_interpreter_t* s, void** l); // v3.305.3
  static int ext_display_choice_window(mb_interpreter_t* s, void** l); // v3.0.231.1 make/create the choice window
  static int ext_hide_choice_window(mb_interpreter_t* s, void** l);    // v3.0.231.1 hide/destroy choice window

  // Distance related functions
  static int ext_get_distance_between_two_points_mt(mb_interpreter_t* s, void** l); // v3.0.303 return distance between 2 points in meters
  static int ext_get_distance_to_instance_in_meters(mb_interpreter_t* s, void** l); // v3.0.241.10 b3
  static int ext_get_elev_to_instance_in_feet(mb_interpreter_t* s, void** l);       // v3.0.241.10 b3
  static int ext_get_bearing_to_instance(mb_interpreter_t* s, void** l);            // v3.0.241.10 b3 calculate bearing between 2 points relative to north
  static int ext_add_gps_xy_entry(mb_interpreter_t* s, void** l);                   // v3.0.241.10 b3

  // User Plane information
  static int ext_get_aircraft_model(mb_interpreter_t* s, void** l); // v3.0.301 B3 expose XPLMGetNthAircraftModel but only for your plane: XPLM_USER_AIRCRAFT by default

  ///// Display inventory or map screens. TODO: consider flight info too.
  static int ext_open_inventory_screen(mb_interpreter_t* s, void** l); // v3.0.303.7
  static int ext_open_image_screen(mb_interpreter_t* s, void** l);   // v3.0.303.7
  static int ext_load_image_to_leg(mb_interpreter_t* s, void** l);   // v3.303.13
  
  
  ///// Hide/Show 3D Markers
  static int ext_hide_3D_markers(mb_interpreter_t* s, void** l); // v3.0.303.7
  static int ext_show_3D_markers(mb_interpreter_t* s, void** l); // v3.0.303.7


  ///// Weather Related
  static int ext_set_predefine_weather_code(mb_interpreter_t* s, void** l); // v3.303.13 - set weather as a number
  static int ext_set_datarefs(mb_interpreter_t* s, void** l); // v3.303.13 format: "{key=value}|{key2=value2}" should call missionx::data_manager::apply_datarefs_based_on_string_parsing()
  static int ext_set_datarefs_interpolation(mb_interpreter_t* s, void** l); // v3.303.13 Target datarefs in format: "{key=value}|{key2=value2}" should call missionx::data_manager::apply_datarefs_based_on_string_parsing()
  static int ext_reset_interpolation_list(mb_interpreter_t* s, void** l); // v3.305.3 clear all interpolation container
  static int ext_remove_dref_from_interpolation_list(mb_interpreter_t* s, void** l); // v3.305.3 clear all interpolation container

  ///// Three Stoppers
  static int ext_start_timer(mb_interpreter_t* s, void** l); // v3.303.13 int, positive int > 0 Provide timer number 1-3 and the time to run in game seconds. Start timer will also reset a running timer
  static int ext_stop_timer(mb_interpreter_t* s, void** l); // v3.303.13 int, provide the timer number 1-3 - will reset the timer, will return the passed time until stop as a real number.
  static int ext_get_timer_time_passed (mb_interpreter_t* s, void** l); // v3.303.13 int, provide the timer number 1-3 returns a "real" number. Similar to "stop_timer" function only we do not stop the timer.
  static int ext_get_timer_ended (mb_interpreter_t* s, void** l); // v3.303.13 int, provide the timer number 1-3 returns 1 if time ended and 0 if not








  // C++ Template
  // applyProperty function modify a component "property" map with the relevent key:value data.
  // The programmer needs to provide the properties map that holds all allowed properties to be modified, this is per ext_xxx function.
  // From the property map list, we need the name of the property to modify and its new value.
  // Then we need the target key name and target map that holds the key object and should have mxProperty implemented.

  // The template assumes we are using a component with "mxProperties"

  // inValidPropertyMap = hold valid properties function allow to modify.
  // inPropertyNameToModify = holds property name to change from inValidPropertyMap
  // inNewValue: the property targets new value
  // keyElement: the target key object from target map.
  // inElementsMap: the target map component that holds are target object to modify.

  // example: apply_property ( "set_message_as_mxpad" /*function name for error message*/, mapProperties /*from calling function*/, PROP_IS_MXPAD_MESSAGE, "true", data_manager::mapMessages)
  // this can be done almost for any CLASS that inherits mxProperty

  template<typename Container> // T represent the Feature to modify: Task, Message
  static bool applyProperty(mb_interpreter_t*                                  s,
                            void**                                             l,
                            const std::string&                                 funcName,
                            std::map<std::string, missionx::mx_property_type>& inValidPropertyMap,
                            std::string&                                       inPropertyNameToModify,
                            std::string&                                       inNewValue,
                            const typename Container::key_type&                keyElement,
                            Container&                                         inElementsMap,
                            const missionx::MxKeyValuePropertiesTypes*         inKeyType = nullptr)
  {
    // typename Container::mapped_type dummy;
    // typename Container::const_iterator iter = inMap.find(key);


    // return found;
    bool        result = false;
    std::string errMsg;
    errMsg.clear();
    std::string propName  = inPropertyNameToModify;
    std::string propValue = inNewValue;

    if (Utils::isElementExists(inElementsMap, keyElement))
    {
      typename Container::mapped_type* mapType = &inElementsMap[keyElement];
      mapType->storeCoreAttribAsProperties();

      // check if property to change is valid
      if (!Utils::isElementExists(inValidPropertyMap, propName))
      {
        errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find property: " + mxconst::get_QM() + propName + mxconst::get_QM() + " in valid property list to change. Please fix your script. skipping command...";
        data_manager::sm.seedError(errMsg);
        mb_push_int(s, l, false);
        return result;
      }

      //  // get property type from map
      missionx::mx_property_type pType = inValidPropertyMap[propName];
      switch (pType)
      {
        case missionx::mx_property_type::MX_BOOL:
        {
          bool val = false;
          if (Utils::isStringBool(propValue, val))
          {
            if (inKeyType == nullptr)
              mapType->setBoolProperty(propName, val);
            else
            {
#ifdef IBM
              mapType->template setNodeProperty<bool>(inPropertyNameToModify, val);
#else
              mapType->setNodeProperty(inPropertyNameToModify, val); //, mapType->node, mapType->node.getName());          // v3.0.241.1 key_s represent element name that holds the attribute value
#endif
            }


            mapType->applyPropertiesToLocal();
          }
          else
          {
            errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received value that is not a BOOLEAN for property: " + mxconst::get_QM() + propName + mxconst::get_QM() + ". Please fix script. Skipping command...";
            data_manager::sm.seedError(errMsg);
            mb_push_int(s, l, false);
            return result;
          }
        }
        break;
        case missionx::mx_property_type::MX_INT:
        case missionx::mx_property_type::MX_FLOAT:
        case missionx::mx_property_type::MX_DOUBLE:
        {
          double val = 0;
          if (Utils::is_number(propValue))
          {
            val = mxUtils::stringToNumber<double>(propValue);

            if (pType == missionx::mx_property_type::MX_INT)
            {
              if (inKeyType == nullptr)
              {
#ifdef IBM
                mapType->template setNodeProperty<int>(propName, (int)val);
#else
                mapType->setNodeProperty(propName, (int)val);
#endif
              }
              else
              {
#ifdef IBM
                mapType->template setNodeProperty<int>(inPropertyNameToModify, (int)val); //, mapType->node, mapType->node.getName()); // v3.0.241.1 key_s represent element name that holds the attribute value
#else
                mapType->setNodeProperty(inPropertyNameToModify, (int)val); //, mapType->node, mapType->node.getName());   // v3.0.241.1 key_s represent element name that holds the attribute value
#endif
              }
            }
            else if (pType == missionx::mx_property_type::MX_FLOAT)
            {
              if (inKeyType == nullptr)
              {
                mapType->setNumberProperty(propName, static_cast<float> (val));
              }
              else
              {
#ifdef IBM
                mapType->template setNodeProperty<float>(inPropertyNameToModify, (float)val); //, mapType->node, mapType->node.getName()); // v3.0.241.1 key_s represent element name that holds the attribute value
#else
                mapType->setNodeProperty(inPropertyNameToModify, (float)val); //, mapType->node, mapType->node.getName()); // v3.0.241.1 key_s represent element name that holds the attribute value
#endif
              }
            }
            else if (pType == missionx::mx_property_type::MX_DOUBLE)
            {
              if (inKeyType == nullptr)
              {
                mapType->setNumberProperty(propName, val);
              }
              else
              {
#ifdef IBM
                mapType->template setNodeProperty<double>(inPropertyNameToModify, val); //, mapType->node, mapType->node.getName()); // v3.0.241.1 key_s represent element name that holds the attribute value
#else
                mapType->setNodeProperty(inPropertyNameToModify, val); //, mapType->node, mapType->node.getName());        // v3.0.241.1 key_s represent element name that holds the attribute value
#endif
              }
            }

            mapType->applyPropertiesToLocal();
          }
          else
          {
            errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received value that is not a NUMBER for property: " + mxconst::get_QM() + propName + mxconst::get_QM() + ". Please fix script. Skipping command...";
            data_manager::sm.seedError(errMsg);
            mb_push_int(s, l, false);
            return result;
          }
        }
        break;
        case missionx::mx_property_type::MX_STRING:
        {
          if (inKeyType == nullptr)
            mapType->setStringProperty(propName, propValue);
          else
            mapType->setNodeStringProperty(inPropertyNameToModify, propValue);  // v3.0.241.1 key_s represent element name that holds the attribute value

          mapType->applyPropertiesToLocal();
        }
        break;
        default:
        {
          errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received property - " + mxconst::get_QM() + propName + mxconst::get_QM() + " with a type it can't parse. Please fix script and notify developer if it fails. Skipping command...";
          data_manager::sm.seedError(errMsg);
          mb_push_int(s, l, false);
          return result;
        }
        break;
      } // end switch
    }
    else
    {
      errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ". Message was not found. Please fix your script. skipping command...";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
      return result;
    } // end if element exists


    return true;
  }
  // end template applyProperty
};

}

#endif
