#include "ext_script.h"
#include "../QueueMessageManager.h"

namespace missionx
{
missionx::mxProperties ext_script::internalBufferBetweenFunctions;
bool                   ext_script::canUseInternalBuffer;
char                   ext_script::emptyCharString{ '\n' };
}

missionx::ext_script::ext_script()
{
  ext_script::canUseInternalBuffer = false;
}

missionx::ext_script::~ext_script() {}

void
missionx::ext_script::register_my_functions(struct mb_interpreter_t* inBas)
{
  std::string name;
  name.clear();

  Log::logXPLMDebugString(" ----- Gloabl Variables related functions -----\n"); // v3.0.205.3

  name = mxUtils::stringToUpper("FN_SET_GLOBAL_BOOL");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_global_bool);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  //  name = mxUtils::stringToUpper("FN_GET_OR_CREATE_GLOBAL_BOOL"); // v3.303.14
  //  mb_register_func(inBas, name.c_str(), ext_script::ext_get_or_create_global_bool);
  //  Log::logXPLMDebugString("Registered Function " + name + " - [v3.304.14]\n");

  name = mxUtils::stringToUpper("FN_GET_GLOBAL_BOOL");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_or_create_global_bool); // was ext_get_global_bool
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_IS_GLOBAL_BOOL_EXISTS");
  mb_register_func(inBas, name.c_str(), ext_script::ext_is_global_bool_exists);
  Log::logXPLMDebugString("Registered Function " + name + "\n");


  name = mxUtils::stringToUpper("FN_SET_GLOBAL_INT");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_global_int);
  Log::logXPLMDebugString("Registered Function " + name + " - Replaces FN_STORE_GLOBAL_INT() \n");
  name = mxUtils::stringToUpper("FN_STORE_GLOBAL_INT");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_global_int);
  Log::logXPLMDebugString("Registered Function " + name + " - Will be DEPRECATED and renamed to fn_set_global_int \n");

  //  name = mxUtils::stringToUpper("FN_GET_OR_CREATE_GLOBAL_INT"); // v3.303.14
  //  mb_register_func(inBas, name.c_str(), ext_script::ext_get_or_create_global_int);
  //  Log::logXPLMDebugString("Registered Function " + name + " - [v3.304.14]\n");

  name = mxUtils::stringToUpper("FN_GET_GLOBAL_INT");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_or_create_global_int); // was ext_get_global_int
  Log::logXPLMDebugString("Registered Function " + name + "\n");


  name = mxUtils::stringToUpper("FN_SET_GLOBAL_DECIMAL"); // alias to FN_STORE_GLOBAL_DECIMAL
  mb_register_func(inBas, name.c_str(), ext_script::ext_store_global_decimal);
  Log::logXPLMDebugString("Registered Function " + name + "\n");
  name = mxUtils::stringToUpper("FN_STORE_GLOBAL_DECIMAL");
  mb_register_func(inBas, name.c_str(), ext_script::ext_store_global_decimal);
  Log::logXPLMDebugString("Registered Function " + name + " - Will be DEPRECATED and renamed to FN_SET_GLOBAL_DECIMAL \n");

  //  name = mxUtils::stringToUpper("FN_GET_OR_CREATE_GLOBAL_DECIMAL"); // v3.303.14
  //  mb_register_func(inBas, name.c_str(), ext_script::ext_get_or_create_global_decimal);
  //  Log::logXPLMDebugString("Registered Function " + name + " - [v3.304.14]\n");

  name = mxUtils::stringToUpper("FN_GET_GLOBAL_DECIMAL");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_or_create_global_decimal); // was ext_get_global_decimal
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_IS_GLOBAL_NUMBER_EXISTS");
  mb_register_func(inBas, name.c_str(), ext_script::ext_is_global_number_exists);
  Log::logXPLMDebugString("Registered Function " + name + "\n");


  name = mxUtils::stringToUpper("FN_SET_GLOBAL_STRING");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_global_string);
  Log::logXPLMDebugString("Registered Function " + name + "\n");
  name = mxUtils::stringToUpper("FN_STORE_GLOBAL_STRING");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_global_string);
  Log::logXPLMDebugString("Registered Function " + name + " - Will be DEPRECATED and renamed to FN_SET_GLOBAL_STRING \n");

  //  name = mxUtils::stringToUpper("FN_GET_OR_CREATE_GLOBAL_STRING"); // v3.303.14
  //  mb_register_func(inBas, name.c_str(), ext_script::ext_get_or_create_global_string);
  //  Log::logXPLMDebugString("Registered Function " + name + " - [v3.304.14]\n");

  name = mxUtils::stringToUpper("FN_GET_GLOBAL_STRING");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_or_create_global_string); // was ext_get_global_string
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_IS_GLOBAL_STRING_EXISTS");
  mb_register_func(inBas, name.c_str(), ext_script::ext_is_global_string_exists);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  // v3.0.112
  name = mxUtils::stringToUpper("FN_REMOVE_GLOBAL_BOOL");
  mb_register_func(inBas, name.c_str(), ext_script::ext_remove_global_bool);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_REMOVE_GLOBAL_NUMBER");
  mb_register_func(inBas, name.c_str(), ext_script::ext_remove_global_number);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_REMOVE_GLOBAL_STRING");
  mb_register_func(inBas, name.c_str(), ext_script::ext_remove_global_string);
  Log::logXPLMDebugString("Registered Function " + name + "\n");



  Log::logXPLMDebugString(" ----- Dataref / Commands related functions -----\n"); // v3.0.205.3

  name = mxUtils::stringToUpper("FN_GET_DREF_VALUE");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_dref_as_number);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_GET_DREF_VAL_AS_STR");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_dref_as_string);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SET_DREF_VALUE");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_dref_value);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SET_DATAREFS");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_datarefs);
  Log::logXPLMDebugString("Registered Function " + name + " - v3.304.13\n");

  name = mxUtils::stringToUpper("FN_SET_DATAREFS_INTERPOLATION"); // v3.303.13
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_datarefs_interpolation);
  Log::logXPLMDebugString("Registered Function " + name + " (int sec, int cycles, str ) - v3.304.13  The string should be in the format: [key=value|key2=value1,value2|...]  \n");

  name = mxUtils::stringToUpper("FN_RESET_INTERPOLATION_LIST"); // v3.305.3
  mb_register_func(inBas, name.c_str(), ext_script::ext_reset_interpolation_list);
  Log::logXPLMDebugString("Registered Function " + name + " ( ) - v3.306.3  Clear all datarefs in interpolation list.\n");

  name = mxUtils::stringToUpper("FN_REMOVE_DREF_FROM_INTERPOLATION_LIST"); // v3.305.3
  mb_register_func(inBas, name.c_str(), ext_script::ext_remove_dref_from_interpolation_list);
  Log::logXPLMDebugString("Registered Function " + name + " ( str ) - v3.306.3 A string of datarefs divided by comma in the format: [key,key2,key3,...].\n");

  name = mxUtils::stringToUpper("FN_EXECUTE_COMMANDS"); // v3.0.221.11+ send commands using comma delimeted
  mb_register_func(inBas, name.c_str(), ext_script::ext_execute_commands);
  Log::logXPLMDebugString("Registered Function " + name + "\n");



  Log::logXPLMDebugString(" ----- Objective/Task/Triggers related functions -----\n"); // v3.0.205.3

  name = mxUtils::stringToUpper("FN_SET_TASK_PROPERTY_IN_OBJECTIVE");                                                                        // v3.0.205.2
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_task_property_in_objective);                                                     // v3.0.205.2
  Log::logXPLMDebugString("Registered Function " + name + " (can be use in any stage of code, like linked trigger in Flight Leg level.)\n"); // v3.0.205.2

  name = mxUtils::stringToUpper("FN_GET_TASK_INFO_IN_OBJECTIVE");                                                                            // v3.0.221.15rc4
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_task_info_in_objective);                                                         // v3.0.221.15rc4
  Log::logXPLMDebugString("Registered Function " + name + " (can be use in any stage of code, like linked trigger in Flight Leg level.)\n"); // v3.0.205.2

  name = mxUtils::stringToUpper("FN_GET_TASK_INFO");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_task_info);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_RESET_TASK_TIMER");
  mb_register_func(inBas, name.c_str(), ext_script::ext_reset_task_timer);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SET_TASK_PROPERTY");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_task_property);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SET_CARGO_END_POSITION"); // v3.0.303
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_cargo_end_position);
  Log::logXPLMDebugString("Registered Function " + name + ". Used with init_script attribute to init cargo position using a script and not attributes. Start position can be set directly to its dataref.\n");

  name = mxUtils::stringToUpper("FN_GET_TRIGGER_INFO");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_trigger_info);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SET_TRIGGER_PROPERTY");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_trigger_property);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_RESET_TRIGGER_TIMER");
  mb_register_func(inBas, name.c_str(), ext_script::ext_reset_trigger_timer);
  Log::logXPLMDebugString("Registered Function " + name + "\n");



  // messages
  Log::logXPLMDebugString(" ----- Message related functions -----\n"); // v3.0.205.3

  name = mxUtils::stringToUpper("FN_GET_MESSAGE_INFO");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_message_info);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SET_MESSAGE_PROPERTY");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_message_property);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SET_MESSAGE_LABEL_PROPERTIES"); // v3.0.223.7 since all messges can be mxpad
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_message_label_properties);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_CREATE_NEW_COMM_MESSAGE");
  mb_register_func(inBas, name.c_str(), ext_script::ext_create_new_comm_message);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SEND_COMM_MESSAGE");
  mb_register_func(inBas, name.c_str(), ext_script::ext_send_comm_message);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SEND_TEXT_MESSAGE");
  mb_register_func(inBas, name.c_str(), ext_script::ext_send_text_message);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SET_COMM_CHANNEL");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_comm_channel);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SET_BACKGROUND_CHANNEL");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_background_channel);
  Log::logXPLMDebugString("Registered Function " + name + "\n");
  // end messages


  // mxconst::QMM related functions
  Log::logXPLMDebugString(" ----- Queue Message related functions -----\n"); // v3.0.223.7

  name = mxUtils::stringToUpper("FN_END_CURRENT_MESSAGE");
  mb_register_func(inBas, name.c_str(), ext_script::ext_end_current_message);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  // name = mxUtils::stringToUpper("FN_END_CURRENT_MESSAGE_AND_BACKGROUND");
  // mb_register_func(inBas, name.c_str(), ext_script::ext_end_current_message_and_background);
  // Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_ABORT_CURRENT_MESSAGE");
  mb_register_func(inBas, name.c_str(), ext_script::ext_abort_current_message);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_ABORT_BG_CHANNEL"); // v3.306.1b
  mb_register_func(inBas, name.c_str(), ext_script::ext_abort_bg_channel);
  Log::logXPLMDebugString("Registered Function " + name + " - new in v3.306.1b. Stop specific Background Channel, Use the name of the message you defined for the background sound.\n");

  name = mxUtils::stringToUpper("FN_ABORT_ALL_CHANNELS"); // v3.306.1b
  mb_register_func(inBas, name.c_str(), ext_script::ext_abort_all_channels);
  Log::logXPLMDebugString("Registered Function " + name + " - new in v3.306.1c. Stop all channels in the sound pool, background and communication.\n");

  name = mxUtils::stringToUpper("FN_FADE_OUT_BG_CHANNEL"); // v3.306.1b
  mb_register_func(inBas, name.c_str(), ext_script::ext_fade_out_bg_channel);
  Log::logXPLMDebugString("Registered Function " + name + " - new in v3.306.1b. Send the name of the message to fade its bg channel.\n");


  // name = mxUtils::stringToUpper("FN_IS_MXPAD_QUEUE_EMPTY");
  name = mxUtils::stringToUpper("FN_IS_MSG_QUEUE_EMPTY");
  mb_register_func(inBas, name.c_str(), ext_script::ext_is_msg_queue_empty);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  // end mxPad related functions


  ///// Flight Leg functions

  Log::logXPLMDebugString(" ----- Flight Leg related functions -----\n"); // v3.0.205.3

  name = mxUtils::stringToUpper("FN_SET_NEXT_LEG"); // v3.0.221.15rc5 add LEG support
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_next_leg);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_STOP_DRAW_SCRIPT"); // V3.0.223.1 beta2
  mb_register_func(inBas, name.c_str(), ext_script::ext_stop_draw_script);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_SET_DRAW_SCRIPT_NAME"); // V3.0.223.1 beta2
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_draw_script_name);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_GET_CURRENT_LEG_DESC"); // V3.305.3
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_current_leg_desc);
  Log::logXPLMDebugString("Registered Function " + name + " (new v3.306.3)\n");

  name = mxUtils::stringToUpper("FN_SET_LEG_DESC"); // V3.305.3
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_leg_desc);
  Log::logXPLMDebugString("Registered Function " + name + " (new v3.306.3) You can send only (text) or (legName,text) \n");



  ///// INVENTORY functions

  Log::logXPLMDebugString(" ----- Inventory related functions -----\n"); // v3.0.213.3
  name = mxUtils::stringToUpper("FN_GET_INV_LAYOUT_TYPE"); // v24.12.2
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_inv_layout_type);
  Log::logXPLMDebugString("Registered Function " + name + " - new v24.12.2, Returns the Inventory compatibility layout. 11=Compatible with XP11.\n");

  name = mxUtils::stringToUpper("FN_MOVE_ITEM_TO_PLANE"); // v24.12.2
  mb_register_func(inBas, name.c_str(), ext_script::ext_move_item_to_plane);
  Log::logXPLMDebugString("Registered Function " + name + " - new v24.12.2, compatible with inventory <station> layout.\n");

  name = mxUtils::stringToUpper("FN_IS_ITEM_EXISTS_IN_PLANE");
  mb_register_func(inBas, name.c_str(), ext_script::ext_is_item_exists_in_plane); // v3.0.213.3
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_IS_ITEM_EXISTS_IN_EXT_INVENTORY");
  mb_register_func(inBas, name.c_str(), ext_script::ext_is_item_exists_in_ext_inventory); // v3.0.213.3
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_MOVE_ITEM_FROM_INV"); // v3.0.221.15.rc4
  mb_register_func(inBas, name.c_str(), ext_script::ext_move_item_from_inv);
  Log::logXPLMDebugString("Registered Function " + name + "\n");




  ///// TIMELAPSE related functions

  Log::logXPLMDebugString(" ----- Timelapse related functions -----\n"); // v3.0.221.15rc5
  name = mxUtils::stringToUpper("FN_TIMELAPSE_ADD_MINUTES");
  mb_register_func(inBas, name.c_str(), ext_script::ext_timelapse_add_minutes); // v3.0.221.15rc5
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_TIMELAPSE_TO_LOCAL_HOUR");
  mb_register_func(inBas, name.c_str(), ext_script::ext_timelapse_to_local_hour); // v3.0.221.15rc5
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_IS_TIMELAPSE_ACTIVE");
  mb_register_func(inBas, name.c_str(), ext_script::ext_is_timelapse_active); // v3.0.221.15rc5
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  //// Time functions
  name = mxUtils::stringToUpper("FN_SET_LOCAL_TIME");
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_local_time); // v3.0.221.15rc5
  Log::logXPLMDebugString("Registered Function " + name + "\n");



  ///// Other functions

  Log::logXPLMDebugString(" ----- Other functions -----\n");
  name = mxUtils::stringToUpper("FN_GET_XP_VERSION"); // v3.305.1b ext_get_xp_version
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_xp_version);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_GET_DISTANCE_TO_REFERENCE_POINT");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_distance_to_reference_point);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_GET_DISTANCE_TO_COORDINATE");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_distance_to_coordinate);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_GET_DISTANCE_BETWEEN_TWO_POINTS_MT"); // v3.0.303
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_distance_between_two_points_mt);
  Log::logXPLMDebugString("Registered Function " + name + " - return the distance between two points in meters.\n");

  name = mxUtils::stringToUpper("FN_GET_DISTANCE_TO_INSTANCE_IN_METERS"); // v3.0.241.10 b3
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_distance_to_instance_in_meters);
  Log::logXPLMDebugString("Registered Function " + name + " \n");

  name = mxUtils::stringToUpper("FN_GET_ELEV_TO_INSTANCE_IN_FEET"); // v3.0.241.10 b3
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_elev_to_instance_in_feet);
  Log::logXPLMDebugString("Registered Function " + name + " \n");

  name = mxUtils::stringToUpper("FN_GET_BEARING_TO_INSTANCE"); // v3.0.241.10 b3
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_bearing_to_instance);
  Log::logXPLMDebugString("Registered Function " + name + " \n");

  name = mxUtils::stringToUpper("FN_ADD_GPS_XY_ENTRY"); // v3.0.241.10 b3
  mb_register_func(inBas, name.c_str(), ext_script::ext_add_gps_xy_entry);
  Log::logXPLMDebugString("Registered Function " + name + " \n");

  name = mxUtils::stringToUpper("FN_GET_NAV_INFO");
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_nav_info);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_ABORT_MISSION");
  mb_register_func(inBas, name.c_str(), ext_script::ext_abort_mission);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  name = mxUtils::stringToUpper("FN_UPDATE_END"); // v24.02.5  fn_update_end
  mb_register_func(inBas, name.c_str(), ext_script::ext_update_end);
  Log::logXPLMDebugString("Registered Function " + name + " (v24.02.5) Update the <end_mission> sub elements.\n");

  name = mxUtils::stringToUpper("FN_INJECT_METAR_FILE"); // V3.0.223.1
  mb_register_func(inBas, name.c_str(), ext_script::ext_inject_metar_file);
  Log::logXPLMDebugString("Registered Function " + name + "\n");

  ///// Position plane
  Log::logXPLMDebugString(" ----- Position Functions -----\n"); // v3.0.231.1

  name = mxUtils::stringToUpper("FN_POSITION_PLANE"); // V3.0.225.5
  mb_register_func(inBas, name.c_str(), ext_script::ext_position_plane);
  Log::logXPLMDebugString("Registered Function " + name + " (lat,long,elev_mt,heading,speed_in_meter_per_seconds) all values are numbers without double quotes \n");

  name = mxUtils::stringToUpper("FN_POSITION_CAMERA"); // V3.0.303.7
  mb_register_func(inBas, name.c_str(), ext_script::ext_position_camera);
  Log::logXPLMDebugString("Registered Function " + name + " (x,y,z,heading,pitch,roll) all values are numbers without double quotes \n");

  ///// Choice Window functions
  Log::logXPLMDebugString(" ----- Choice Window functions -----\n"); // v3.0.231.1

  name = mxUtils::stringToUpper("FN_SET_CHOICE_NAME"); // V3.0.231.1
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_choice_name);
  Log::logXPLMDebugString("Registered Function " + name + " (\"choice name\") \n");

  name = mxUtils::stringToUpper("FN_GET_ACTIVE_CHOICE_NAME"); // V3.305.3
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_active_choice_name);
  Log::logXPLMDebugString("Registered Function " + name + " () v3.306.3\n");

  name = mxUtils::stringToUpper("FN_DISPLAY_CHOICE_WINDOW"); // V3.0.231.1
  mb_register_func(inBas, name.c_str(), ext_script::ext_display_choice_window);
  Log::logXPLMDebugString("Registered Function " + name + " () \n");

  name = mxUtils::stringToUpper("FN_HIDE_CHOICE_WINDOW"); // V3.0.231.1
  mb_register_func(inBas, name.c_str(), ext_script::ext_hide_choice_window);
  Log::logXPLMDebugString("Registered Function " + name + " () \n");



  Log::logXPLMDebugString(" ----- Plane Related functions -----\n"); // v3.0.301 B3

  name = mxUtils::stringToUpper("FN_GET_AIRCRAFT_MODEL"); // V3.0.301 B3
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_aircraft_model);
  Log::logXPLMDebugString("Registered Function " + name + " (), returns string\n");



  Log::logXPLMDebugString(" ----- UI Related functions -----\n"); // v3.0.303.7

  name = mxUtils::stringToUpper("FN_OPEN_INVENTORY_SCREEN"); // V3.0.303.7
  mb_register_func(inBas, name.c_str(), ext_script::ext_open_inventory_screen);
  Log::logXPLMDebugString("Registered Function " + name + " ()\n");

  name = mxUtils::stringToUpper("FN_OPEN_IMAGE_SCREEN"); // V3.0.303.7
  mb_register_func(inBas, name.c_str(), ext_script::ext_open_image_screen);
  Log::logXPLMDebugString("Registered Function " + name + " ()\n");

  name = mxUtils::stringToUpper("FN_LOAD_IMAGE_TO_LEG"); // V3.303.13
  mb_register_func(inBas, name.c_str(), ext_script::ext_load_image_to_leg);
  Log::logXPLMDebugString("Registered Function " + name + " ({image file name}, {leg name}). Will be displayed in the map screen\n");

  name = mxUtils::stringToUpper("FN_HIDE_3D_MARKERS"); // V3.0.303.7
  mb_register_func(inBas, name.c_str(), ext_script::ext_hide_3D_markers);
  Log::logXPLMDebugString("Registered Function " + name + " ()\n");

  name = mxUtils::stringToUpper("FN_SHOW_3D_MARKERS"); // V3.0.303.7
  mb_register_func(inBas, name.c_str(), ext_script::ext_show_3D_markers);
  Log::logXPLMDebugString("Registered Function " + name + " ()\n");



  Log::logXPLMDebugString(" ----- Weather Related functions -----\n"); // v3.303.13

  name = mxUtils::stringToUpper("FN_SET_PREDEFINE_WEATHER_CODE"); // V3.303.13
  mb_register_func(inBas, name.c_str(), ext_script::ext_set_predefine_weather_code);
  Log::logXPLMDebugString("Registered Function " + name + " (integer number) XP12(0..8) XP11(0..7)\n");


  Log::logXPLMDebugString(" ----- Three Stopper Related functions -----\n"); // v3.303.13

  name = mxUtils::stringToUpper("FN_START_TIMER"); // V3.303.13
  mb_register_func(inBas, name.c_str(), ext_script::ext_start_timer);
  Log::logXPLMDebugString("Registered Function " + name + " (integer - number between 1 and 3,  float - seconds to run) returns success or fail\n");
  name = mxUtils::stringToUpper("FN_STOP_TIMER"); // V3.303.13
  mb_register_func(inBas, name.c_str(), ext_script::ext_stop_timer);
  Log::logXPLMDebugString("Registered Function " + name + " (integer code between 1 and 3)  returns the time passed in seconds and reset the timer\n");
  name = mxUtils::stringToUpper("FN_GET_TIMER_TIME_PASSED"); // V3.303.13
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_timer_time_passed);
  Log::logXPLMDebugString("Registered Function " + name + " (integer - number between 1 and 3) returns the time it runs in seconds\n");
  name = mxUtils::stringToUpper("FN_GET_TIMER_ENDED"); // V3.303.13
  mb_register_func(inBas, name.c_str(), ext_script::ext_get_timer_ended);
  Log::logXPLMDebugString("Registered Function " + name + " (integer - number between 1 and 3) returns 1 if ended and 0 if not\n");

  Log::logXPLMDebugString(" ----- END Embedded functions -----\n");
}


// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------

bool
missionx::ext_script::checkString(const char* inString, std::string& outValue)
{
  outValue.clear();

  if (inString == nullptr)
  {
    Log::logMsg("[EXT Script]No String was sent to function !!!");
    return false;
  }

  outValue = inString; // split lines for GCC compiler
  outValue = Utils::trim(outValue);

  if (outValue.empty())
  {
    Log::logMsg("[EXT Script]String is empty !!!");
    return false;
  }

  return true;
}


// -----------------------------------

int
missionx::ext_script::ext_set_global_bool(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 2; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;
  int                   outValue = 0;

  key.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outValue));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));


  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive the number of expected arguments. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (mxUtils::isElementExists(script_manager::mapScriptGlobalBoolArg, key))
  {
    script_manager::mapScriptGlobalBoolArg[key] = (bool)outValue;
  }
  else
  {
    Utils::addElementToMap(script_manager::mapScriptGlobalBoolArg, key, (bool)outValue);
  }


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_global_bool(mb_interpreter_t* s, void** l)
{
  return ext_script::ext_get_or_create_global_bool(s, l);
}

// -----------------------------------

int
missionx::ext_script::ext_set_global_int(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 2; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;
  int                   outValue = 0;


  key.clear();
  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outValue));
    counter++;
  }

  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive expected arguments. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (mxUtils::isElementExists(script_manager::mapScriptGlobalDecimalArg, key))
  {
    script_manager::mapScriptGlobalDecimalArg[key] = (double)outValue;
  }
  else
  {
    Utils::addElementToMap(script_manager::mapScriptGlobalDecimalArg, key, (double)outValue);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_global_int(mb_interpreter_t* s, void** l)
{

  return ext_script::ext_get_or_create_global_int(s, l);
}

int
missionx::ext_script::ext_get_xp_version(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1;
  constexpr const short counter                  = 0;
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  int                   result                   = MB_FUNC_OK;
  std::string           errMsg;
  int                   returnValue  = 0;
  int                   defaultValue = 0;


  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));

  mb_check(mb_attempt_close_bracket(s, l));

  std::string xp_version_s = mxUtils::formatNumber<int>(missionx::data_manager::xplane_ver_i);
  xp_version_s             = xp_version_s.substr(0, 2);
  returnValue              = mxUtils::stringToNumber<int>(xp_version_s);

  missionx::data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, (int)returnValue);

  return MB_FUNC_OK;
}

// -----------------------------------

int
missionx::ext_script::ext_get_or_create_global_int(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;
  int                   returnValue      = 0;
  int                   defaultValue     = 0;
  bool                  bHasDefaultValue = false;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &defaultValue));
    counter++;
    bHasDefaultValue = true;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive global parameter name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, returnValue);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, returnValue);
  }


  if (script_manager::getIntValue(key, returnValue))
  {
    mb_push_int(s, l, returnValue);
  }
  else
  {
    returnValue = (bHasDefaultValue) ? defaultValue : 0; // Zero is the default integer number if "key" was not found.
    Utils::addElementToMap(script_manager::mapScriptGlobalDecimalArg, key, (double)returnValue);
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, returnValue);
  }

  return MB_FUNC_OK;
}

// -----------------------------------

int
missionx::ext_script::ext_get_or_create_global_bool(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;
  bool                  returnValue      = false;
  int                   defaultValue     = 0;
  bool                  bHasDefaultValue = false;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &defaultValue));
    counter++;
    bHasDefaultValue = true;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive global parameter name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, returnValue);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, returnValue);
  }


  if (script_manager::getBoolValue(key, returnValue))
  {
    mb_push_int(s, l, returnValue);
  }
  else
  {
    returnValue = (bHasDefaultValue) ? (bool)defaultValue : false; // Zero is the default integer number if "key" was not found.
    Utils::addElementToMap(script_manager::mapScriptGlobalBoolArg, key, returnValue);
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, (int)returnValue);
  }

  return MB_FUNC_OK;
}

// -----------------------------------

int
missionx::ext_script::ext_get_or_create_global_decimal(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;
  double                returnValue      = 0.0;
  double                defaultValue     = 0.0;
  bool                  bHasDefaultValue = false;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &defaultValue));
    counter++;
    bHasDefaultValue = true;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive global parameter name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnValue);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnValue);
  }


  if (script_manager::getDecimalValue(key, returnValue))
  {
    mb_push_real(s, l, returnValue);
  }
  else
  {
    returnValue = (bHasDefaultValue) ? defaultValue : 0.0; // Zero is the default integer number if "key" was not found.
    Utils::addElementToMap(script_manager::mapScriptGlobalDecimalArg, key, returnValue);
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnValue);
  }

  return MB_FUNC_OK;
}

// -----------------------------------

int
missionx::ext_script::ext_get_or_create_global_string(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;
  std::string           value{ "" };
  char*                 returnValue      = NULL;
  char*                 defaultValue     = NULL;
  bool                  bHasDefaultValue = false;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &defaultValue));
    counter++;
    bHasDefaultValue = true;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive global parameter name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    returnValue = mb_memdup("", (unsigned)(1)); // `+1` means to allocate and copy one extra byte of the ending '\0'

    mb_push_string(s, l, returnValue);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    returnValue = mb_memdup("", (unsigned)(1));
    mb_push_string(s, l, returnValue);
  }


  if (script_manager::getStringValue(key, value))
  {
    returnValue = mb_memdup(value.c_str(), unsigned(value.length() + (size_t)1));
    mb_push_string(s, l, returnValue);
  }
  else
  {
    value = (bHasDefaultValue) ? defaultValue : &ext_script::emptyCharString; // Zero is the default integer number if "key" was not found.
    Utils::addElementToMap(script_manager::mapScriptGlobalStringsArg, key, value);
    returnValue = mb_memdup(value.c_str(), unsigned(value.length() + (size_t)1));

    missionx::data_manager::sm.seedError(errMsg);
    mb_push_string(s, l, returnValue);
  }

  return MB_FUNC_OK;
}


// -----------------------------------

int
ext_script::ext_set_global_decimal(mb_interpreter_t* s, void** l)
{
  return ext_script::ext_store_global_decimal(s, l);
}

// -----------------------------------

int
missionx::ext_script::ext_store_global_decimal(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 2; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;
  double                outValue = 0.0;

  key.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &outValue));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));


  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive expected variable list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (mxUtils::isElementExists(script_manager::mapScriptGlobalDecimalArg, key))
  {
    script_manager::mapScriptGlobalDecimalArg[key] = outValue;
  }
  else
  {
    Utils::addElementToMap(script_manager::mapScriptGlobalDecimalArg, key, outValue);
  }

  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, true);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_global_decimal(mb_interpreter_t* s, void** l)
{
  return ext_script::ext_get_or_create_global_decimal(s, l);
}

// -----------------------------------

int
missionx::ext_script::ext_is_global_number_exists(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;

  key.clear();
  double returnValue = 0.0; // false

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive global parameter name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnValue);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnValue);
  }

  if (mxUtils::isElementExists(script_manager::mapScriptGlobalDecimalArg, key))
  {
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    data_manager::sm.seedError(errMsg); // Error message should be empty, since it is not an error
    mb_push_int(s, l, false);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_is_global_bool_exists(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;

  key.clear();
  double returnValue = 0.0; // false

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive global parameter name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnValue);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnValue);
  }

  if (mxUtils::isElementExists(script_manager::mapScriptGlobalBoolArg, key))
  {
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    data_manager::sm.seedError(errMsg); // Error message should be empty, since it is not an error
    mb_push_int(s, l, false);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_is_global_string_exists(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;

  key.clear();
  double returnValue = 0.0; // false

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive global parameter name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnValue);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnValue);
  }

  if (mxUtils::isElementExists(script_manager::mapScriptGlobalStringsArg, key))
  {
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    data_manager::sm.seedError(errMsg); // Error message should be empty, since it is not an error
    mb_push_int(s, l, false);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_global_string(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 2; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  char*                 outValue                 = NULL;
  std::string           errMsg;
  std::string           key;

  key.clear();
  std::string stringToStore;
  stringToStore.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outValue));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive expected arguments. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  stringToStore = std::string((outValue == nullptr) ? "" : outValue); // we will not trim value

  if (mxUtils::isElementExists(script_manager::mapScriptGlobalStringsArg, key))
  {
    script_manager::mapScriptGlobalStringsArg[key] = stringToStore;
  }
  else
  {
    Utils::addElementToMap(script_manager::mapScriptGlobalStringsArg, key, stringToStore);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_global_string(mb_interpreter_t* s, void** l)
{
  return ext_script::ext_get_or_create_global_string(s, l);
}

// -----------------------------------

int
missionx::ext_script::ext_get_dref_as_number(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  char*             outKey   = NULL;
  std::string       errMsg;
  std::string       key;
  errMsg.clear();
  key.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    //    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (checkString(outKey, key) && Utils::isElementExists(missionx::data_manager::mapDref, key))
  {
    missionx::data_manager::sm.seedError(errMsg);
    missionx::dataref_param& dref = missionx::data_manager::mapDref[key];
    dref.readDatarefValue_into_missionx_plugin();
    double val = dref.getValue<double>();

    mb_push_real(s, l, val);
  }
  else
  {
    errMsg = std::string(funcName) + ": No Dataref by the name: " + mxconst::get_QM() + key + mxconst::get_QM() + ". Please use correct Dataref name. Check <dref> element.";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, 0.0);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_dref_as_string(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  char*             outKey   = NULL;
  char*             value;
  std::string       errMsg;
  std::string       valAsString;
  std::string       key;

  errMsg.clear();
  valAsString.clear();
  key.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    //    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (checkString(outKey, key) && Utils::isElementExists(missionx::data_manager::mapDref, key))
  {
    missionx::data_manager::sm.seedError(errMsg);
    missionx::dataref_param& dref = missionx::data_manager::mapDref[key];
    dref.readDatarefValue_into_missionx_plugin();
    double val  = dref.getValue<double>();
    valAsString = Utils::formatNumber<double>(val);

    value = mb_memdup(valAsString.c_str(), (unsigned)(valAsString.length() + 1)); // `+1` means to allocate and copy one extra byte of the ending '\0'
    mb_push_string(s, l, value);
  }
  else
  {
    errMsg = std::string(funcName) + ": No Dataref by the name: " + mxconst::get_QM() + key + mxconst::get_QM() + ". Please use correct Dataref name. Check <dref> element.";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, 0.0f);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_dref_value(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 2; // dataref name, value as scalar number
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = nullptr;
  double                outValue                 = 0.0;

  std::string errMsg;
  std::string key;

  key.clear();
  errMsg.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &outValue));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive expected parameters. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (checkString(outKey, key) && Utils::isElementExists(missionx::data_manager::mapDref, key))
  {

    missionx::dataref_param& dref = missionx::data_manager::mapDref[key];

    dref.setValue(outValue);                            // set dref internal value as double value. will cast when reading it
    dataref_param::set_dataref_values_into_xplane(dref); // set the value itself into x-plane
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true); // success
  }
  else
  {
    errMsg = "[ERROR]No Dataref by the name: " + mxconst::get_QM() + key + mxconst::get_QM() + ". Please use correct DataRef name. Check <dref> element.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_task_info(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1;
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outName                  = NULL;
  std::string           errMsg;
  std::string           name;

  errMsg.clear();
  name.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive expected parameters. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (checkString(outName, name))
  {
    // check if we have an objective information
    if (Utils::isElementExists(data_manager::smPropSeedValues.mapProperties, mxconst::get_EXT_MX_CURRENT_OBJ()))
    {
      std::string objName = data_manager::smPropSeedValues.getPropertyValue(mxconst::get_EXT_MX_CURRENT_OBJ(), errMsg);
      if (!objName.empty() && Utils::isElementExists(missionx::data_manager::mapObjectives[objName].mapTasks, name))
      {
        Task*                  task         = &missionx::data_manager::mapObjectives[objName].mapTasks[name];
        missionx::mxProperties taskInfoProp = task->getTaskInfoToSeed();

        // return status
        data_manager::sm.seedByProperty(taskInfoProp);
        data_manager::sm.seedError(errMsg);
        mb_push_int(s, l, true);
      }
    }
    else
    {
      // return status
      errMsg = "No Task by the name: " + mxconst::get_QM() + name + mxconst::get_QM() + " was found.";
      missionx::data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
    } // end if element exists

  } // end checkString
  else
  {
    errMsg = "First argument sent is empty? Check its validity.";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_reset_task_timer(mb_interpreter_t* s, void** l)
{
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  constexpr const size_t noOfAttributesFromScript = 1;
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outName                  = NULL;
  std::string            errMsg;
  std::string            name;

  errMsg.clear();
  name.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));


  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive expected parameters. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // check if we have an objective information
  if (Utils::isElementExists(data_manager::smPropSeedValues.mapProperties, mxconst::get_EXT_MX_CURRENT_OBJ()))
  {
    std::string objName = data_manager::smPropSeedValues.getPropertyValue(mxconst::get_EXT_MX_CURRENT_OBJ(), errMsg);
    if (!objName.empty() && Utils::isElementExists(missionx::data_manager::mapObjectives[objName].mapTasks, name))
    {
      missionx::data_manager::mapObjectives[objName].mapTasks[name].timer.reset();

      // return status
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, true);
    }
  }
  else
  {
    // return status
    errMsg = "No Task by the name: " + mxconst::get_QM() + name + mxconst::get_QM() + " was found.";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  } // end if element exists


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_task_property(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 3;
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outTaskName              = nullptr;
  char*                  outPropName              = nullptr;
  char*                  outPropValue             = nullptr;
  std::string            errMsg;
  std::string            taskName;
  std::string            propName;
  std::string            propValue;

  errMsg.clear();
  propName.clear();
  propValue.clear();

  // Properties allowed to manipulate.
  static std::map<std::string, missionx::mx_property_type> mapProperties = { { mxconst::get_ATTRIB_TITLE(), missionx::mx_property_type::MX_STRING },          { mxconst::get_ATTRIB_BASE_ON_TRIGGER(), missionx::mx_property_type::MX_STRING }, { mxconst::get_ATTRIB_BASE_ON_SCRIPT(), missionx::mx_property_type::MX_STRING }, { mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), missionx::mx_property_type::MX_INT },
                                                                             { mxconst::get_ATTRIB_FORCE_EVALUATION(), missionx::mx_property_type::MX_BOOL }, { mxconst::get_ATTRIB_ENABLED(), missionx::mx_property_type::MX_BOOL },           { mxconst::get_PROP_SCRIPT_COND_MET_B(), missionx::mx_property_type::MX_BOOL },  { mxconst::get_PROP_IS_COMPLETE(), missionx::mx_property_type::MX_BOOL } };

  std::map<std::string, missionx::MxKeyValuePropertiesTypes> mapUpdatableProperties = { { mxconst::get_ATTRIB_TITLE(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_TITLE(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_BASE_ON_TRIGGER(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_BASE_ON_TRIGGER(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_BASE_ON_SCRIPT(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_BASE_ON_SCRIPT(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), missionx::mx_property_type::MX_INT) },
                                                                                        { mxconst::get_PROP_SCRIPT_COND_MET_B(), missionx::MxKeyValuePropertiesTypes(mxconst::get_PROP_SCRIPT_COND_MET_B(), missionx::mx_property_type::MX_BOOL) },
                                                                                        { mxconst::get_ATTRIB_ENABLED(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_ENABLED(), missionx::mx_property_type::MX_BOOL) },
                                                                                        { mxconst::get_ATTRIB_FORCE_EVALUATION(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_FORCE_EVALUATION(), missionx::mx_property_type::MX_BOOL) },
                                                                                        { mxconst::get_PROP_IS_COMPLETE(), missionx::MxKeyValuePropertiesTypes(mxconst::get_PROP_IS_COMPLETE(), missionx::mx_property_type::MX_BOOL) } };

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outTaskName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outPropName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outPropValue));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // convert to string
  taskName  = std::string(outTaskName);
  propName  = std::string(outPropName);
  propValue = std::string(outPropValue);

  // check mandatory attributes have value
  if (propName.empty() || taskName.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received EMPTY Task/Property name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // check if we have an objective seeded information

  std::string objName = data_manager::smPropSeedValues.getPropertyValue(mxconst::get_EXT_MX_CURRENT_OBJ(), errMsg);
  if (objName.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find Objective in this context. Please fix your function call timing. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }
  else if (Utils::isElementExists(mapUpdatableProperties, propName))
  {
    const missionx::MxKeyValuePropertiesTypes elementNameType = mapUpdatableProperties[propName];
    if (applyProperty(s, l, funcName, mapProperties, propName, propValue, taskName, data_manager::mapObjectives[objName].mapTasks, &elementNameType))
    {
      // return status success
      missionx::data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, true);
    }
  }
  else // property was not found
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find updatable property: " + propName;
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }



  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_cargo_end_position(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 3;
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outTaskName              = NULL;
  double                lat_d{ 0.0 }, lon_d{ 0.0 };
  std::string           errMsg;
  std::string           taskName;

  errMsg.clear();
  taskName.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outTaskName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &lat_d));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &lon_d));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript || lat_d * lon_d == 0.0)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list or Lat/Long values. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // convert to string
  taskName = (outTaskName == NULL) ? "" : std::string(outTaskName);

  // Get the "task" object using the task name and stored objective name
  std::string objName = data_manager::smPropSeedValues.getPropertyValue(mxconst::get_EXT_MX_CURRENT_OBJ(), errMsg);
  if (objName.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find the Objective of task: " + taskName + " in this context.Please fix your function call timing. skipping command. ";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }
  else
  {
    auto& task = data_manager::mapObjectives[objName].mapTasks[taskName];
    if (task.task_type == missionx::mx_task_type::base_on_sling_load_plugin)
    {
      task.pSlingEnd = Point(lat_d, lon_d);
    }
  } // finish setting end cargo position location



  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_reset_trigger_timer(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1;
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outName                  = NULL;
  std::string           errMsg;
  std::string           name;

  errMsg.clear();
  name.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive expected parameters. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (checkString(outName, name) && Utils::isElementExists(missionx::data_manager::mapTriggers, name))
  {
    missionx::data_manager::mapTriggers[name].timer.reset();
    // return status
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    // return status
    errMsg = "No trigger by the name: " + mxconst::get_QM() + name + mxconst::get_QM() + " was found.";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_trigger_info(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1;
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outName                  = NULL;
  std::string           errMsg;
  std::string           name;

  errMsg.clear();
  name.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR]" + std::string(funcName) + " did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }


  if (checkString(outName, name))
  {
    // check if we have an objective information
    std::string name = std::string(outName);
    if (Utils::isElementExists(missionx::data_manager::mapTriggers, name))
    {
      Trigger*               trig     = &missionx::data_manager::mapTriggers[name];
      missionx::mxProperties infoProp = trig->getInfoToSeed();

      // return status
      data_manager::sm.seedByProperty(infoProp);
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, true);
    }
    else
    {
      // return status
      errMsg = "[ERROR]" + std::string(funcName) + ": No Trigger by the name: " + mxconst::get_QM() + name + mxconst::get_QM() + " was found.";
      missionx::data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
    } // end if element exists

  } // end checkString
  else
  {
    errMsg = "[ERROR]" + std::string(funcName) + ": First argument sent - " + mxconst::get_QM() + std::string((outName == nullptr) ? "" : outName) + mxconst::get_QM() + ", is not valid.";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_trigger_property(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 3;
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outName                  = nullptr;
  char*                  outPropName              = nullptr;
  char*                  outPropValue             = nullptr;
  std::string            errMsg;
  std::string            name;
  std::string            propName;
  std::string            propValue;

  errMsg.clear();
  propName.clear();
  propValue.clear();

  // Properties allowed to manipulate.
  static std::map<std::string, missionx::mx_property_type>   mapProperties          = { { mxconst::get_ATTRIB_ELEV_MIN_MAX_FT(), missionx::mx_property_type::MX_STRING },
                                                                                        { mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED(), missionx::mx_property_type::MX_STRING },
                                                                                        { mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_LEFT(), missionx::mx_property_type::MX_STRING },
                                                                                        { mxconst::get_ATTRIB_POST_SCRIPT(), missionx::mx_property_type::MX_STRING },
                                                                                        { mxconst::get_PROP_All_COND_MET_B(), missionx::mx_property_type::MX_BOOL },
                                                                                        { mxconst::get_PROP_SCRIPT_COND_MET_B(), missionx::mx_property_type::MX_BOOL },
                                                                                        { mxconst::get_ATTRIB_ENABLED(), missionx::mx_property_type::MX_BOOL },
                                                                                        { mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT(), missionx::mx_property_type::MX_STRING },
                                                                                        { mxconst::get_ATTRIB_RE_ARM(), missionx::mx_property_type::MX_BOOL } };
  std::map<std::string, missionx::MxKeyValuePropertiesTypes> mapUpdatableProperties = { { mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ELEMENT_OUTCOME(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_LEFT(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ELEMENT_OUTCOME(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_POST_SCRIPT(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ELEMENT_TRIGGER(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_PROP_All_COND_MET_B(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ELEMENT_TRIGGER(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_PROP_SCRIPT_COND_MET_B(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ELEMENT_TRIGGER(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_ENABLED(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ELEMENT_TRIGGER(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ELEMENT_ELEVATION_VOLUME(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_RE_ARM(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ELEMENT_TRIGGER(), missionx::mx_property_type::MX_STRING) } };


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outPropName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outPropValue));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // convert to string
  name      = std::string(outName);
  propName  = std::string(outPropName);
  propValue = std::string(outPropValue);

  // check mandatory attributes have value
  if (propName.empty() || name.empty())
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received EMPTY Trigger/Property name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (Utils::isElementExists(mapUpdatableProperties, propName))
  {
    const missionx::MxKeyValuePropertiesTypes elementNameType = mapUpdatableProperties[propName];
    if (applyProperty(s, l, funcName, mapProperties, propName, propValue, name, data_manager::mapTriggers, &elementNameType)) // in the future we need to change mapProperties to the mapUpdatableProperties to eliminate duplications
    {
      // return status success
      missionx::data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, true);
    }
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_message_info(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1;
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outName                  = nullptr;
  std::string           errMsg;
  std::string           name;

  errMsg.clear();
  name.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR]" + std::string(funcName) + " did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }


  if (checkString(outName, name))
  {
    // check if we have an objective information
    std::string name = (outName) ? std::string(outName) : "";
    if (Utils::isElementExists(missionx::data_manager::mapMessages, name))
    {
      Message*               msg      = &missionx::data_manager::mapMessages[name];
      missionx::mxProperties infoProp = msg->getInfoToSeed();

      // return status
      data_manager::sm.seedByProperty(infoProp);
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, true);
    }
    else
    {
      // return status
      errMsg = "[ERROR]" + std::string(funcName) + ": No Message by the name: " + mxconst::get_QM() + name + mxconst::get_QM() + " was found.";
      missionx::data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
    } // end if element exists

  } // end checkString
  else
  {
    if (outName)
      errMsg = "[ERROR]" + std::string(funcName) + ": First argument sent - " + mxconst::get_QM() + std::string(outName) + mxconst::get_QM() + ", is not valid.";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_message_property(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 3;
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outName                  = nullptr;
  char*                  outPropName              = nullptr;
  char*                  outPropValue             = nullptr;
  std::string            errMsg;
  std::string            name;
  std::string            propName;
  std::string            propValue;

  errMsg.clear();
  propName.clear();
  propValue.clear();

  // Properties allowed to manipulate.
  std::map<std::string, missionx::mx_property_type> mapProperties = { { mxconst::get_ATTRIB_MESSAGE(), missionx::mx_property_type::MX_STRING },
                                                                      { mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT(), missionx::mx_property_type::MX_INT },
                                                                      { mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR(), missionx::mx_property_type::MX_BOOL },
                                                                      { mxconst::get_ATTRIB_MESSAGE_HIDE_TEXT(), missionx::mx_property_type::MX_BOOL },
                                                                      { mxconst::get_ATTRIB_ENABLED(), missionx::mx_property_type::MX_BOOL },
                                                                      { mxconst::get_ATTRIB_LABEL(), missionx::mx_property_type::MX_STRING },
                                                                      { mxconst::get_ATTRIB_LABEL_COLOR(), missionx::mx_property_type::MX_STRING },
                                                                      // v3.305.3
                                                                      { mxconst::get_PROP_NEXT_MSG(), missionx::mx_property_type::MX_STRING },
                                                                      { mxconst::get_ATTRIB_ADD_MINUTES(), missionx::mx_property_type::MX_STRING },
                                                                      { mxconst::get_ATTRIB_TIMELAPSE_TO_LOCAL_HOURS(), missionx::mx_property_type::MX_STRING },
                                                                      { mxconst::get_ATTRIB_SET_DAY_HOURS(), missionx::mx_property_type::MX_STRING },
                                                                      { mxconst::get_ATTRIB_POST_SCRIPT(), missionx::mx_property_type::MX_STRING },
                                                                      { mxconst::get_ATTRIB_OPEN_CHOICE(), missionx::mx_property_type::MX_STRING },
                                                                      { mxconst::get_ATTRIB_FADE_BG_CHANNEL(), missionx::mx_property_type::MX_STRING } };

  std::map<std::string, missionx::MxKeyValuePropertiesTypes> mapUpdatableProperties = { { mxconst::get_ATTRIB_MESSAGE(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_MESSAGE(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT(), missionx::mx_property_type::MX_INT) },
                                                                                        { mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR(), missionx::mx_property_type::MX_BOOL) },
                                                                                        { mxconst::get_ATTRIB_MESSAGE_HIDE_TEXT(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_MESSAGE_HIDE_TEXT(), missionx::mx_property_type::MX_BOOL) },
                                                                                        { mxconst::get_PROP_SCRIPT_COND_MET_B(), missionx::MxKeyValuePropertiesTypes(mxconst::get_PROP_SCRIPT_COND_MET_B(), missionx::mx_property_type::MX_BOOL) },
                                                                                        { mxconst::get_ATTRIB_ENABLED(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_ENABLED(), missionx::mx_property_type::MX_BOOL) },
                                                                                        { mxconst::get_ATTRIB_LABEL(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_LABEL(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_LABEL_COLOR(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_LABEL_COLOR(), missionx::mx_property_type::MX_STRING) },
                                                                                        // v3.305.3
                                                                                        { mxconst::get_PROP_NEXT_MSG(), missionx::MxKeyValuePropertiesTypes(mxconst::get_PROP_NEXT_MSG(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_ADD_MINUTES(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_ADD_MINUTES(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_TIMELAPSE_TO_LOCAL_HOURS(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_TIMELAPSE_TO_LOCAL_HOURS(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_SET_DAY_HOURS(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_SET_DAY_HOURS(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_POST_SCRIPT(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_POST_SCRIPT(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_OPEN_CHOICE(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_OPEN_CHOICE(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_OPEN_CHOICE(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_FADE_BG_CHANNEL(), missionx::mx_property_type::MX_STRING) }


  };

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outPropName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outPropValue));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // convert to string
  name      = std::string(outName);
  propName  = std::string(outPropName);
  propValue = std::string(outPropValue);

  // check mandatory attributes have value
  if (propName.empty() || name.empty())
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received EMPTY Trigger/Property name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }


  if (Utils::isElementExists(mapUpdatableProperties, propName))
  {
    const missionx::MxKeyValuePropertiesTypes elementNameType = mapUpdatableProperties[propName];
    if (applyProperty(s, l, funcName, mapProperties, propName, propValue, name, data_manager::mapMessages, &elementNameType)) // in the future we need to change mapProperties to the mapUpdatableProperties to eliminate duplications
    {
      // return status success
      missionx::data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, true);
    }
  }

  return result;
}


// -----------------------------------

int
missionx::ext_script::ext_set_message_label_properties(mb_interpreter_t* s, void** l)
{
  // message name must be unique, label: represent text, placement: [L|R|""]. Empty means text will be written accros all row.
  // color: [white|yellow|green|yellow|red] in lower case
  constexpr const short noOfAttributesFromScript = 5; // message_name(s), label(s), label_placement (s), color (s)
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");

  typedef enum
    : uint8_t
  {
    msg_name        = 0,
    label           = 1,
    label_placement = 2,
    color           = 3,
    image           = 4 // not implemented yet
  } outVals;


  //  static const double invalidResult = -1.0; // -1 represent invalid outcome
  short                    counter = 0;
  int                      result  = MB_FUNC_OK;
  std::string              errMsg;
  std::vector<char*>       vecAttrib;
  std::vector<std::string> vecValues;

  vecAttrib.clear();
  vecValues.clear();


  // initialize vector
  for (int i = 0; i < noOfAttributesFromScript; i++)
  {
    char* dummy = nullptr;
    vecAttrib.push_back(dummy);
  }

  // Read attributes from MY-BASIC
  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  bool found = true;
  for (int i = 0; i < noOfAttributesFromScript && found; i++)
  {
    if (mb_has_arg(s, l))
    {
      mb_check(mb_pop_string(s, l, &vecAttrib[i]));
      ++counter;
      found = true;
    }
    else
      found = false;
  } // end loop
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < (noOfAttributesFromScript - (noOfAttributesFromScript - 1))) // Only message name us mandatory, all other attributes can be optional
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive minimal expected parameters. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // Apply values to string vector and add missing values
  for (int i = 0; i < counter; ++i)
  {
    vecValues.push_back(std::string(vecAttrib.at(i)));
  }
  if (counter < noOfAttributesFromScript) // this can occour if "image" is not sent.
  {
    for (int i = counter; i < noOfAttributesFromScript; ++i)
      vecValues.push_back(EMPTY_STRING);
  }

  // Check message name
  std::string name = vecValues.front(); // first value should represent existing message name
  if (name.empty())
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", has empty message name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // Fail if message does not exists
  if (!Utils::isElementExists(data_manager::mapMessages, name))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ". Message by the name: " + mxconst::get_QM() + name + mxconst::get_QM() + " does not exists. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }
  else
  {
    if (vecValues.size() >= (size_t)noOfAttributesFromScript)
    {
      // translate/convert values before modifying the message
      // Label Placemenet. If not a legal string then fallback to Left
      vecValues[outVals::label_placement] = mxUtils::translateMxPadLabelPositionToValid(vecValues[outVals::label_placement]); // v3.0.197 moved logic to mxUtils. Shared with read_mission_file. Will return "L", "R" or "". Default "L"

      // handle color
      vecValues[outVals::color] = mxUtils::stringToLower(vecValues[outVals::color]);
      if (vecValues[outVals::color].compare("white") != 0 && vecValues[outVals::color].compare("yellow") != 0 && vecValues[outVals::color].compare("red") != 0 && vecValues[outVals::color].compare("green") != 0 && vecValues[outVals::color].compare("blue") != 0)
      {
        vecValues[outVals::color] = "white"; // fallback color
      }

      // data_manager::mapMessages[name].setBoolProperty(PROP_IS_MXPAD_MESSAGE, true); // v3.0.241.1 no need, all messages are mxpad
      data_manager::mapMessages[name].set_mxpad_properties(vecValues[outVals::label], vecValues[outVals::label_placement], vecValues[outVals::color], EMPTY_STRING); // last is the image name which is EMPTY in these builds.

      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, true);

      return result;
    }

  } // end if Message exists or not


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_send_comm_message(mb_interpreter_t* s, void** l)
{

  constexpr const short noOfAttributesFromScript = 1; // message name(s), track name(s) optional.
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outName                  = NULL;
  char*                 outTrackName             = NULL;
  int                   outOneTimeFlag           = false;

  std::string errMsg;
  std::string messageName;
  std::string trackName;

  errMsg.clear();
  messageName.clear();
  trackName.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outTrackName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outOneTimeFlag));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (outTrackName != nullptr)
    trackName = std::string(outTrackName);

  if (checkString(outName, messageName))
  {
    if (outOneTimeFlag)
    {
      if (!mxUtils::isElementExists(missionx::QueueMessageManager::mapTrackedMessages, messageName)) // if message is not in the tracked messages
      {
        missionx::QueueMessageManager::addMessageToQueue(messageName, messageName, errMsg); // message name is also tracked name. Designer should provide only pre-defined "message name" in XML mission file
      }
      else
      {
        errMsg = "Message was already sent once. Skipping message...";
        data_manager::sm.seedError(errMsg);
        mb_push_int(s, l, false);
        return result;
      }
    }
    else if (QueueMessageManager::addMessageToQueue(messageName, trackName, errMsg))
    {
      // return status
      missionx::data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, true);
    }
    else
    {
      missionx::data_manager::sm.seedError(errMsg); // errMsg is set in addMessageToQueueList();
      mb_push_int(s, l, false);
    }
  }
  else
  {
    // return status
    errMsg = "Name given is malformed or incorrect. Skipping message...";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_send_text_message(mb_interpreter_t* s, void** l)
{
  // constexpr const short noOfAttributesFromScript = 2;
  const std::string funcName       = std::string(__func__).replace(0, 4, "fn_");
  short             counter        = 0;
  int               result         = MB_FUNC_OK;
  char*             outTextMessage = nullptr;
  char*             outTrackName   = nullptr;
  int               outOneTimeFlag = false; // v3.0.205.3

  std::string errMsg;
  std::string textMessage;
  std::string trackName;

  errMsg.clear();
  textMessage.clear();
  trackName.clear();



  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outTextMessage));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outTrackName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outOneTimeFlag));
    counter++;
  }

  mb_check(mb_attempt_close_bracket(s, l));

  if (counter == 0)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }


  trackName = (outTrackName == nullptr) ? missionx::EMPTY_STRING : std::string(outTrackName);

  if (checkString(outTextMessage, textMessage))
  {
    // Decide which branch to handle, the oneTime or the generic always send text
    if (outOneTimeFlag)
    {
      if (!trackName.empty() && !mxUtils::isElementExists(missionx::QueueMessageManager::mapTrackedMessages, trackName))
      {
        missionx::QueueMessageManager::addTextAsNewMessageToQueue(trackName, textMessage, trackName); // Designer should provide "track name" or it will be skipped. The  "qmm.mapTrackedMessages" helps to fine already called messages.
      }
      else
      {
        errMsg = "Track Name is empty or Text message already exists. Skip calling one time text message...";
        data_manager::sm.seedError(errMsg);
        mb_push_int(s, l, false);
        return result;
      }
    }
    else // if not track name has been provided, then just send the text as a new message. Plugin will provide unique name
      missionx::QueueMessageManager::addTextAsNewMessageToQueue(EMPTY_STRING, textMessage, trackName);


    // return status success
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    // return status
    errMsg = "Name given is malformed or incorrect. Skipping message...";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_end_current_message(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;
  std::string       name;

  errMsg.clear();
  name.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));

  mb_check(mb_attempt_close_bracket(s, l));

  if (!missionx::QueueMessageManager::listPoolMsg.empty())
  {
    missionx::QueueMessageManager::listPoolMsg.front().msgTimer.setEnd(); // 3.0.241.6 replaced map with list container
  }

  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, true);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_abort_current_message(mb_interpreter_t* s, void** l)
{

  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));

  if (!missionx::QueueMessageManager::listPoolMsg.empty())
  {
    const std::string name = missionx::QueueMessageManager::listPoolMsg.front().getName();

    missionx::QueueMessageManager::listPoolMsg.front().msgTimer.setEnd();                                     // 3.0.241.6 replaced map with list container
    missionx::QueueMessageManager::listPoolMsg.front().msgState = missionx::mx_message_state_enum::msg_abort; // 3.0.241.6 replaced map with list container

    if (Utils::isElementExists(missionx::QueueMessageManager::mapPlayingBackgroundSF, name))
      missionx::QueueMessageManager::mapPlayingBackgroundSF[name].timerChannel.setEnd();
  }

  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, true);
  return result;
}

// -----------------------------------


int
missionx::ext_script::ext_abort_bg_channel(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; // objective name, task name, property, value
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outBgChannelName         = nullptr;

  std::string outBgChannelName_s;
  std::string errMsg;

  outBgChannelName_s.clear();
  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outBgChannelName));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }


  outBgChannelName_s = std::string(outBgChannelName);

  if (Utils::isElementExists(missionx::QueueMessageManager::mapPlayingBackgroundSF, outBgChannelName_s))
    missionx::QueueMessageManager::mapPlayingBackgroundSF[outBgChannelName_s].timerChannel.setEnd();

  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, true);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_abort_all_channels(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 0; // objective name, task name, property, value
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  int                   result                   = MB_FUNC_OK;

  std::string errMsg;
  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));

  mb_check(mb_attempt_close_bracket(s, l));

  missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::sound_abort_all_channels);

  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, true);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_fade_out_bg_channel(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; // objective name, task name, property, value
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  double                outFadeSteps_d           = 0.0;
  char*                 outBgChannelName         = nullptr;

  std::string outBgChannelName_s;
  std::string errMsg;

  outBgChannelName_s.clear();
  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outBgChannelName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &outFadeSteps_d));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter < noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";

#ifndef RELEASE
    Log::logMsg(errMsg);
#endif // !RELEASE

    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  checkString(outBgChannelName, outBgChannelName_s);
  // outBgChannelName_s = std::string(outBgChannelName);

#ifndef RELEASE
  Log::logMsg(std::string(funcName) + " - Try to fade out bg message: " + outBgChannelName_s);
#endif // !RELEASE


  missionx::QueueMessageManager::fade_bg_channel(outBgChannelName_s, outFadeSteps_d);

  // if (Utils::isElementExists(missionx::QueueMessageManager::mapPlayingBackgroundSF, outBgChannelName_s))
  //{
  //   missionx::mx_track_instructions_strct fadeCommand;
  //   fadeCommand.seconds_to_start_f = 0.0f; // immediate
  //   fadeCommand.command            = '-'; // decrease command
  //   fadeCommand.new_volume         = 0;   // expected volume to decrease to
  //   fadeCommand.transition_time_f  = (outFadeSteps_d <= 0,0) ? mxconst::DEFAULT_SF_FADE_SECONDS_F : (float)outFadeSteps_d; // Use used fade steps or plugin default


  //  mxUtils::purgeQueueContainer(missionx::QueueMessageManager::mapPlayingBackgroundSF[outBgChannelName_s].qSoundCommands);
  //  missionx::QueueMessageManager::mapPlayingBackgroundSF[outBgChannelName_s].qSoundCommands.emplace(fadeCommand);

  //  missionx::QueueMessageManager::mapPlayingBackgroundSF[outBgChannelName_s].timerChannel.setEnd();
  //}


  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, true);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_is_msg_queue_empty(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));

  // mxconst::QMm.listPadQueueMessages can be empty while the message is still playing, we pop out the message once it is handled by the broadcast loop
  if (QueueMessageManager::is_queue_empty())
  {
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    missionx::data_manager::sm.seedError(errMsg); // In this case there is no error message. Only the boolean value is important
    mb_push_int(s, l, false);
  }

  data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_task_property_in_objective(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 4; // objective name, task name, property, value
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outObjectiveName         = nullptr;
  char*                  outTaskName              = nullptr;
  char*                  outPropName              = nullptr;
  char*                  outPropValue             = nullptr;
  std::string            errMsg;
  std::string            objectiveName;
  std::string            taskName;
  std::string            propName;
  std::string            propValue;

  errMsg.clear();
  objectiveName.clear();
  taskName.clear();
  propName.clear();
  propValue.clear();

  // Properties allowed to manipulate. This is duplication with ext_set_task_property.
  static std::map<std::string, missionx::mx_property_type> mapProperties = { { mxconst::get_ATTRIB_TITLE(), missionx::mx_property_type::MX_STRING },          { mxconst::get_ATTRIB_BASE_ON_TRIGGER(), missionx::mx_property_type::MX_STRING }, { mxconst::get_ATTRIB_BASE_ON_SCRIPT(), missionx::mx_property_type::MX_STRING }, { mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), missionx::mx_property_type::MX_INT },
                                                                             { mxconst::get_ATTRIB_FORCE_EVALUATION(), missionx::mx_property_type::MX_BOOL }, { mxconst::get_ATTRIB_ENABLED(), missionx::mx_property_type::MX_BOOL },           { mxconst::get_PROP_SCRIPT_COND_MET_B(), missionx::mx_property_type::MX_BOOL },  {  mxconst::get_PROP_IS_COMPLETE(), missionx::mx_property_type::MX_BOOL } };

  std::map<std::string, missionx::MxKeyValuePropertiesTypes> mapUpdatableProperties = { { mxconst::get_ATTRIB_TITLE(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_TITLE(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_BASE_ON_TRIGGER(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_BASE_ON_TRIGGER(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_BASE_ON_SCRIPT(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_BASE_ON_SCRIPT(), missionx::mx_property_type::MX_STRING) },
                                                                                        { mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), missionx::mx_property_type::MX_INT) },
                                                                                        { mxconst::get_PROP_SCRIPT_COND_MET_B(), missionx::MxKeyValuePropertiesTypes(mxconst::get_PROP_SCRIPT_COND_MET_B(), missionx::mx_property_type::MX_BOOL) },
                                                                                        { mxconst::get_ATTRIB_ENABLED(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_ENABLED(), missionx::mx_property_type::MX_BOOL) },
                                                                                        { mxconst::get_ATTRIB_FORCE_EVALUATION(), missionx::MxKeyValuePropertiesTypes(mxconst::get_ATTRIB_FORCE_EVALUATION(), missionx::mx_property_type::MX_BOOL) },
                                                                                        { mxconst::get_PROP_IS_COMPLETE(), missionx::MxKeyValuePropertiesTypes(mxconst::get_PROP_IS_COMPLETE(), missionx::mx_property_type::MX_BOOL) } };


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outObjectiveName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outTaskName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outPropName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outPropValue));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // convert to string
  objectiveName = std::string(outObjectiveName);
  taskName      = std::string(outTaskName);
  propName      = std::string(outPropName);
  propValue     = std::string(outPropValue);

  // check mandatory attributes have value
  if (propName.empty() || taskName.empty() || objectiveName.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received EMPTY Objective/Task/Property name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // get current goal
  if (data_manager::smPropSeedValues.hasProperty(mxconst::get_EXT_MX_CURRENT_LEG())) // v3.0.221.15rc5 add support to LEG // v3.0.303.7 deprecated: data_manager::smPropSeedValues.hasProperty(mxconst::EXT_MX_CURRENT_GOAL)
  {
    std::string currentFlightLeg;
    currentFlightLeg.clear();

    if (data_manager::smPropSeedValues.hasProperty(mxconst::get_EXT_MX_CURRENT_LEG())) // v3.0.221.15rc5 add LEG support
      currentFlightLeg = data_manager::smPropSeedValues.getPropertyValue(mxconst::get_EXT_MX_CURRENT_LEG(), errMsg);
    // else
    //   currentGoal = data_manager::smPropSeedValues.getPropertyValue(mxconst::EXT_MX_CURRENT_GOAL, errMsg);

    if (currentFlightLeg.empty())
    {
      errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find Goal in this context. Please fix your function call. skipping command.";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
      return result;
    }

    // goal exists as seeded parmeter but foes it exists in mission map
    if (Utils::isElementExists(data_manager::mapFlightLegs, currentFlightLeg))
    {
      if (Utils::isElementExistsInList<std::string>(data_manager::mapFlightLegs[currentFlightLeg].listObjectivesInFlightLeg, objectiveName)) // and check if objective exists in goal
      {
        // search task in objective
        if (Utils::isElementExists(data_manager::mapObjectives[objectiveName].mapTasks, taskName))
        {

          if (Utils::isElementExists(mapUpdatableProperties, propName))
          {
            const missionx::MxKeyValuePropertiesTypes elementNameType = mapUpdatableProperties[propName];
            if (applyProperty(s, l, funcName, mapProperties, propName, propValue, taskName, data_manager::mapObjectives[objectiveName].mapTasks, &elementNameType)) // in the future we need to change mapProperties to the
                                                                                                                                                                    // mapUpdatableProperties to eliminate duplications
            {
              // return status success
              missionx::data_manager::sm.seedError(errMsg);
              mb_push_int(s, l, true);
            }
          }

          // if (applyProperty(s, l, funcName, mapProperties, propName, propValue, taskName, data_manager::mapObjectives[objectiveName].mapTasks))
          //{
          //  // return status success
          //  missionx::data_manager::sm.seedError(errMsg);
          //  mb_push_int(s, l, true);
          //}
        }
        else // fail to find task in objective
        {
          errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find Task: " + mxconst::get_QM() + taskName + mxconst::get_QM() + " in Objective: " + mxconst::get_QM() + objectiveName + mxconst::get_QM() + ". Please fix your function call. skipping command.";
          data_manager::sm.seedError(errMsg);
          mb_push_int(s, l, false);
          return result;
        }
      }
      else // fail to find objective in goal
      {
        errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find Objective: " + mxconst::get_QM() + objectiveName + mxconst::get_QM() + " in goal: " + mxconst::get_QM() + currentFlightLeg + mxconst::get_QM() + ". Please fix your function call. skipping command.";
        data_manager::sm.seedError(errMsg);
        mb_push_int(s, l, false);
        return result;
      }
    }


  } // end if goal exists in seeded parameters

  return result;

} // end set_task_property_in_objective()

// -----------------------------------

int
missionx::ext_script::ext_get_task_info_in_objective(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 2; // objective name, task name, property, value
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outObjectiveName         = nullptr;
  char*                  outTaskName              = nullptr;
  std::string            errMsg;
  std::string            objectiveName;
  std::string            taskName;

  errMsg.clear();
  objectiveName.clear();
  taskName.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outObjectiveName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outTaskName));
    counter++;
  }

  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // convert to string
  objectiveName = std::string(outObjectiveName);
  taskName      = std::string(outTaskName);

  // check mandatory attributes have value
  if (taskName.empty() || objectiveName.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received EMPTY Objective/Task name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // get current goal
  // if (Utils::isElementExists(data_manager::smPropSeedValues.mapProperties, mxconst::EXT_MX_CURRENT_GOAL))
  if (data_manager::smPropSeedValues.hasProperty(mxconst::get_EXT_MX_CURRENT_LEG())) // v3.0.221.15rc5 add support to LEG // data_manager::smPropSeedValues.hasProperty(mxconst::EXT_MX_CURRENT_GOAL)
  {
    // std::string currentGoal = data_manager::smPropSeedValues.getPropertyValue(mxconst::EXT_MX_CURRENT_GOAL, errMsg);
    std::string currentGoal;
    currentGoal.clear();

    if (data_manager::smPropSeedValues.hasProperty(mxconst::get_EXT_MX_CURRENT_LEG())) // v3.0.221.15rc5 add LEG support
      currentGoal = data_manager::smPropSeedValues.getPropertyValue(mxconst::get_EXT_MX_CURRENT_LEG(), errMsg);
    // else
    //   currentGoal = data_manager::smPropSeedValues.getPropertyValue(mxconst::EXT_MX_CURRENT_GOAL, errMsg);


    if (currentGoal.empty())
    {
      errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find Goal in this context. Please fix your function call. skipping command.";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
      return result;
    }

    // goal exists as seeded parmeter but does it exists in mission map
    if (Utils::isElementExists(data_manager::mapFlightLegs, currentGoal))
    {
      if (Utils::isElementExistsInList<std::string>(data_manager::mapFlightLegs[currentGoal].listObjectivesInFlightLeg, objectiveName)) // and check if objective exists in goal
      {
        // search task in objective
        if (Utils::isElementExists(data_manager::mapObjectives[objectiveName].mapTasks, taskName))
        {
          Task*                  task         = &missionx::data_manager::mapObjectives[objectiveName].mapTasks[taskName];
          missionx::mxProperties taskInfoProp = task->getTaskInfoToSeed();

          // return status
          data_manager::sm.seedByProperty(taskInfoProp);
          data_manager::sm.seedError(errMsg);
          mb_push_int(s, l, true);
        }
        else // fail to find task in objective
        {
          errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find Task: " + mxconst::get_QM() + taskName + mxconst::get_QM() + " in Objective: " + mxconst::get_QM() + objectiveName + mxconst::get_QM() + ". Please fix your function call. skipping command.";
          data_manager::sm.seedError(errMsg);
          mb_push_int(s, l, false);
          return result;
        }
      }
      else // fail to find objective in goal
      {
        errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find Objective: " + mxconst::get_QM() + objectiveName + mxconst::get_QM() + " in goal: " + mxconst::get_QM() + currentGoal + mxconst::get_QM() + ". Please fix your function call. skipping command.";
        data_manager::sm.seedError(errMsg);
        mb_push_int(s, l, false);
        return result;
      }
    }


  } // end if goal exists in seeded parameters

  return result;
}

// -----------------------------------


int
missionx::ext_script::ext_set_comm_channel(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 4; // msgName(s), inSoundFile(s), inSecondsToplay(f), inSoundVol(i)
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  std::string           errMsg;
  char*                 outMsgName       = nullptr;
  char*                 outSoundFile     = nullptr;
  double                outSecondsToPlay = 0;
  int                   outSoundVol      = 30;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outMsgName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outSoundFile));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &outSecondsToPlay));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outSoundVol));
    counter++;
  }


  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive all expected parameters. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (set_channel(funcName, outMsgName, missionx::mx_message_channel_type_enum::comm, outSoundFile, (float)outSecondsToPlay, outSoundVol, errMsg))
  {
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_background_channel(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 4; // msgName(s), inSoundFile(s), inSecondsToplay(f), inSoundVol(i)
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  std::string           errMsg;
  char*                 outMsgName       = nullptr;
  char*                 outSoundFile     = nullptr;
  double                outSecondsToPlay = 0;
  int                   outSoundVol      = 30;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outMsgName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outSoundFile));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &outSecondsToPlay));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outSoundVol));
    counter++;
  }


  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive all expected parameters. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (set_channel(funcName, outMsgName, missionx::mx_message_channel_type_enum::background, outSoundFile, (float)outSecondsToPlay, outSoundVol, errMsg))
  {
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }


  return result;
}

// -----------------------------------

bool
missionx::ext_script::set_channel(std::string funcName, char* inMsgName, missionx::mx_message_channel_type_enum inChannelType, char* inSoundFile, float inSecondsToplay, int inSoundVol, std::string& outErr)
{
  // Apply values to string vector and add missing values
  std::string msgName   = std::string(inMsgName);
  std::string soundFile = std::string(inSoundFile);

  if (msgName.empty() || soundFile.empty())
  {
    outErr = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received empty message name or sound file name. Please fix your script. skipping command.";
    return false;
  }


  return missionx::QueueMessageManager::setMessageChannel(msgName, inChannelType, soundFile, inSecondsToplay, outErr, inSoundVol);
}

int
missionx::ext_script::ext_create_new_comm_message(mb_interpreter_t* s, void** l)
{
  constexpr const short    noOfAttributesFromScript = 6; // name(s), message(s), trackName(s), mute_xplane_narrator(b), hide_text(b), override_seconds_to_display_text(i)
  const std::string        funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                    counter                  = 0;
  int                      result                   = MB_FUNC_OK;
  std::string              errMsg;
  std::vector<char*>       vecAttrib;
  std::vector<std::string> vecValues;

  vecAttrib.clear();
  vecValues.clear();



  // initialize vector
  for (int i = 0; i < noOfAttributesFromScript; i++)
  {
    char* dummy = nullptr;
    vecAttrib.push_back(dummy);
  }

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  bool found = true;
  for (int i = 0; i < noOfAttributesFromScript && found; i++)
  {
    if (mb_has_arg(s, l))
    {
      mb_check(mb_pop_string(s, l, &vecAttrib[i]));
      counter++;
      found = true;
    }
    else
      found = false;
  } // end loop
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < 2)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive minimal expected parameters. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // Apply values to string vector and add missing values
  for (int i = 0; i < counter; i++)
  {
    vecValues.push_back(std::string(vecAttrib.at(i)));
  }
  if (counter < noOfAttributesFromScript)
  {
    for (int i = counter; i < noOfAttributesFromScript; i++) // resize vector to its expected size: noOfAttributesFromScript
      vecValues.push_back(EMPTY_STRING);
  }

  // Do some validations:
  // Check message name
  std::string name = vecValues.front(); // first value should represent message name
  if (name.empty())
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", has empty message name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (Utils::isElementExists(data_manager::mapMessages, name))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ". Message by the name: " + mxconst::get_QM() + name + mxconst::get_QM() + " already exists. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }
  else
  {
    if (vecValues.size() >= (size_t)noOfAttributesFromScript)
    {
      //          name(s), message(s), trackName(s), mute_xplane_narrator(b), hide_text(b), override_seconds_to_display_text(i)
      // Message msg (name, vecValues[1], vecValues[2], mxUtils::emptyReplace(vecValues[3], MX_NO), mxUtils::emptyReplace(vecValues[4], MX_NO) , mxUtils::emptyReplace(vecValues[5], ZERO) );
      Message msg; // v3.0.241.2 construct an empty message, we will handle the details later

      // v3.0.241.1 added XML support. Get template node from MAPPING element and use it to construct a new message node.
      msg.node = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_ELEMENT_MESSAGE ()); // prepare a message element (maybe we should do it in the constructor)
      if (!msg.node.isEmpty())
      {
        msg.setName(name);
        // msg.setNodeStringProperty(mxconst::get_ATTRIB_NAME(), name, msg.node, this->mx_const.this->mx_const.ELEMENT_MESSAGE); // v3.303.11 deprecated, already in "setName() function"

        IXMLNode xMixText_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(msg.node, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_TYPE(), mxconst::get_CHANNEL_TYPE_TEXT(), false);
        if (!xMixText_ptr.isEmpty())
        {
          Utils::xml_add_cdata(xMixText_ptr, vecValues[1]); // CDATA
          msg.setNodeStringProperty(mxconst::get_ATTRIB_MESSAGE_MUTE_XPLANE_NARRATOR(), mxUtils::emptyReplace(vecValues[3], mxconst::get_MX_NO()), xMixText_ptr);
          msg.setNodeStringProperty(mxconst::get_ATTRIB_MESSAGE_HIDE_TEXT(), mxUtils::emptyReplace(vecValues[4], mxconst::get_MX_NO()), xMixText_ptr);
          msg.setNodeStringProperty(mxconst::get_ATTRIB_MESSAGE_OVERRIDE_SECONDS_TO_DISPLAY_TEXT(), vecValues[5], xMixText_ptr);
        }

        if (msg.parse_node())
        {
          Utils::addElementToMap(data_manager::mapMessages, name, msg);
          data_manager::sm.seedError(errMsg);
          mb_push_int(s, l, true);
        }
        else
        {
          errMsg = "Failed to parse message node: " + name;
          data_manager::sm.seedError(errMsg);
          mb_push_int(s, l, false);
        }
      } // node is not empty
      else
      {
        errMsg = "[create_new_comm_message: failed. Failed to parse the node: \n" + std::string(missionx::data_manager::xmlRender.getString(msg.node));
        data_manager::sm.seedError(errMsg);
        mb_push_int(s, l, false);
      }
    }

  } // end if Message exists or not

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_distance_to_coordinate(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 2; // latitude & longitude
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  static const double   invalidResult            = -1.0; // -1 represent invalid outcome
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  std::string           errMsg;

  Point pTarget;
  Point pPlane;

  double lat, lon; // v3.0.213.7
  lat = lon = 0.0;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &lat));
    pTarget.setLat(lat); // v3.0.213.7
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &lon));
    pTarget.setLon(lon); // v3.0.213.7
    // pTarget.pointState = missionx::mx_point_state::defined;  // v3.0.213.7 removed since setLat/Lon already set Point as defined.
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, invalidResult);
    return result;
  }

  pPlane          = dataref_manager::getPlanePointLocationThreadSafe();
  double distance = pPlane.calcDistanceBetween2Points(pTarget, missionx::mx_units_of_measure::nm, &errMsg);
  mb_push_real(s, l, distance);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_distance_to_reference_point(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1;
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outName                  = nullptr;
  static const double   invalidResult            = -1.0; // -1 represent invalid outcome
  std::string           errMsg;
  std::string           name;

  errMsg.clear();
  name.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName)); // task name
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, invalidResult);
    return result;
  }

  if (checkString(outName, name))
  {

    // search a task with the given name, and extract its trigger name to be evaluated
    // check if we have an objective information and

    Task* task = nullptr;
    if (Utils::isElementExists(data_manager::smPropSeedValues.mapProperties, mxconst::get_EXT_MX_CURRENT_OBJ()))
    {
      std::string objName = data_manager::smPropSeedValues.getPropertyValue(mxconst::get_EXT_MX_CURRENT_OBJ(), errMsg);
      if (!objName.empty()) // empty = skip this is not an error, we will continue and search in triggers map
      {
        task = &data_manager::mapObjectives[objName].mapTasks[name];
      }
    } // end if Objective


    // search trigger
    if (task == nullptr)
    {
      // return status
      errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ". Wrong Task name givven. Correct code and try again. Skipping.";
      missionx::data_manager::sm.seedError(errMsg);
      mb_push_real(s, l, invalidResult);
    }
    else if (task->task_type == missionx::mx_task_type::trigger)
    {
      name = task->action_code_name; // get trigger name from task

      if (Utils::isElementExists(missionx::data_manager::mapTriggers, name))
      {

        if (data_manager::mapTriggers[name].pCenter.pointState == missionx::mx_point_state::point_undefined)
        {
          errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ". Trigger " + mxconst::get_QM() + name + mxconst::get_QM() + " has no valid \"reference point /center point\". Please fix mission data. skipping command.";
          data_manager::sm.seedError(errMsg);
          mb_push_real(s, l, invalidResult);
          return result;
        }


        Point  pPlane   = dataref_manager::getPlanePointLocationThreadSafe();
        double distance = pPlane.calcDistanceBetween2Points(data_manager::mapTriggers[name].pCenter, missionx::mx_units_of_measure::nm, &errMsg);

        // return status
        missionx::data_manager::sm.seedError(errMsg);
        mb_push_real(s, l, distance);
      }
      else
      {
        // return status
        errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + "No Trigger by the name: " + mxconst::get_QM() + name + mxconst::get_QM() + " was found.";
        missionx::data_manager::sm.seedError(errMsg);
        mb_push_real(s, l, invalidResult);
      }
    }
    else
    {
      // return status
      errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ". Task picked is not based on Trigger. Skipping.";
      missionx::data_manager::sm.seedError(errMsg);
      mb_push_real(s, l, invalidResult);
    }
  }
  else
  {
    // return status
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ". Trigger Name given is malformed or incorrect. Skipping.";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, invalidResult);
  }


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_nav_info(mb_interpreter_t* s, void** l)
{
  constexpr const short    noOfAttributesFromScript = 8; // XPLMNavRef name (s), inNameFragment(*char), inIDFragment (*char), b_getPlaneLatLon(bool), inLat(float), long(float), inFrequency(int), XPLMNavType (int)
  const std::string        funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                    counter                  = 0;
  int                      result                   = MB_FUNC_OK;
  std::string              errMsg;
  std::vector<char*>       vecAttrib;
  std::vector<std::string> vecValues;
  missionx::mxProperties   seedOutput;
  vecAttrib.clear();
  vecValues.clear();

  // initialize vector
  for (int i = 0; i < noOfAttributesFromScript; i++)
  {
    char* dummy = nullptr;
    vecAttrib.push_back(dummy);
  }

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  bool found = true;
  for (int i = 0; i < noOfAttributesFromScript && found; i++)
  {
    if (mb_has_arg(s, l))
    {
      mb_check(mb_pop_string(s, l, &vecAttrib[i]));
      counter++;
      found = true;
    }
    else
      found = false;
  } // end loop
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive enough parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // Apply values to string vector and add missing values
  for (auto iter : vecAttrib)
  {
    vecValues.push_back(std::string(iter));
  }
  if (counter < noOfAttributesFromScript)
  {
    for (int i = counter; i < noOfAttributesFromScript; i++)
      vecValues.push_back(EMPTY_STRING);
  }
  counter             = 0;
  std::string navName = vecValues.front();
  counter++; // 0
  char* searchNameFrag = (vecValues.at(counter).empty()) ? NULL : (char*)vecValues.at(counter).c_str();
  counter++; // 1
  char* searchIDFrag = (vecValues.at(counter).empty()) ? NULL : (char*)vecValues.at(counter).c_str();
  counter++; // 2
  bool  b_getPlaneLatLong = false;
  float searchLat, searchLon;
  searchLat = searchLon = 0.0f;

  if (Utils::isStringBool(vecValues.at(counter), b_getPlaneLatLong)) // 3
  {
    if (b_getPlaneLatLong)
    {
      Point pPlane = missionx::dataref_manager::getPlanePointLocationThreadSafe();
      searchLat    = (float)pPlane.getLat();
      searchLon    = (float)pPlane.getLon();
    }
  }
  counter++; // 4
  // check if plane lat/long were initialized. If not than try to read from function
  if (searchLat == 0.0f || searchLon == 0.0f)
  {
    searchLat = (vecValues.at(counter).empty()) ? 0 : (mxUtils::is_number(vecValues.at(counter)) ? mxUtils::stringToNumber<float>(vecValues.at(counter)) : 0);
    counter++; // 5
    searchLon = (vecValues.at(counter).empty()) ? 0 : (mxUtils::is_number(vecValues.at(counter)) ? mxUtils::stringToNumber<float>(vecValues.at(counter)) : 0);
    counter++; // 6
  }
  else
    counter += 2; // we need to add 2 since we skip 2 cells

  // radio freq
  int searchFreq = (vecValues.at(counter).empty()) ? 0 : (mxUtils::is_number(vecValues.at(counter)) ? mxUtils::stringToNumber<int>(vecValues.at(counter)) : 0);
  counter++; // 7
  // enum {
  //  xplm_Nav_Unknown = 0
  //  , xplm_Nav_Airport = 1
  //  , xplm_Nav_NDB = 2
  //  , xplm_Nav_VOR = 4
  //  , xplm_Nav_ILS = 8
  //  , xplm_Nav_Localizer = 16
  //  , xplm_Nav_GlideSlope = 32
  //  , xplm_Nav_OuterMarker = 64
  //  , xplm_Nav_MiddleMarker = 128
  //  , xplm_Nav_InnerMarker = 256
  //  , xplm_Nav_Fix = 512
  //  , xplm_Nav_DME = 1024
  //  , xplm_Nav_LatLon = 2048
  //};
  // typedef int XPLMNavType;
  XPLMNavType navType = (vecValues.at(counter).empty()) ? xplm_Nav_Unknown : (mxUtils::is_number(vecValues.at(counter)) ? mxUtils::stringToNumber<int>(vecValues.at(counter)) : xplm_Nav_Unknown);
  counter++; // 8
  if (navType == xplm_Nav_Unknown)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ". No navigation type has been provided. Please fix. Failing function.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // translate user data into pointers
  int*   inSearchFreq = (searchFreq == 0) ? NULL : &searchFreq;
  float* inSearchLat  = (searchLat == 0) ? NULL : &searchLat;
  float* inSearchLon  = (searchLon == 0) ? NULL : &searchLon;

  // XPLM_NAV_NOT_FOUND = -1
  XPLMNavRef navRef = XPLMFindNavAid(searchNameFrag, searchIDFrag, inSearchLat, inSearchLon, inSearchFreq, navType);
  if (navRef == XPLM_NAV_NOT_FOUND)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ". No navigation aid was found.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // XPLM_API void                 XPLMGetNavAidInfo(
  //  XPLMNavRef           inRef,
  //  XPLMNavType *        outType,    /* Can be NULL */
  //  float *              outLatitude,    /* Can be NULL */
  //  float *              outLongitude,    /* Can be NULL */
  //  float *              outHeight,    /* Can be NULL */
  //  int *                outFrequency,    /* Can be NULL */
  //  float *              outHeading,    /* Can be NULL */
  //  char *               outID,    /* Can be NULL */
  //  char *               outCommands,    /* Can be NULL */
  //  char *               outReg);    /* Can be NULL */

  float outHeight  = 0.0f;
  float outHeading = 0.0f;
  char  outID[48];
  char  outName[256];
  char  outReg[2]; // byte output. 1 for in DSF 0 for not in DSF
  outReg[0] = '\0';
  // outReg[1] = '\0';

  unsigned char isInDSF;
  // #ifdef IBM
  //   std::byte isInDSF;
  // #else
  //   unsigned char isInDSF;
  // #endif

  XPLMGetNavAidInfo(navRef, &navType, &searchLat, &searchLon, &outHeight, &searchFreq, &outHeading, outID, outName, outReg);
  outReg[1] = '\0';

  isInDSF = outReg[0];

  std::string outValue = mxUtils::formatNumber<int>(navType);

  // seed properties as strings
  seedOutput.setIntProperty(mxconst::get_EXT_mxNavType(), navType);
  seedOutput.setProperty<double>(mxconst::get_EXT_mxNavLat(), searchLat);
  seedOutput.setProperty<double>(mxconst::get_EXT_mxNavLon(), searchLon);
  seedOutput.setProperty<double>(mxconst::get_EXT_mxNavHeight(), (double)outHeight);
  seedOutput.setIntProperty(mxconst::get_EXT_mxNavFreq(), searchFreq);
  seedOutput.setProperty<double>(mxconst::get_EXT_mxNavHead(), (double)outHeading);
  seedOutput.setStringProperty(mxconst::get_EXT_mxNavID(), std::string(outID));
  seedOutput.setStringProperty(mxconst::get_EXT_mxNavName(), std::string(outName));
  seedOutput.setIntProperty(mxconst::get_EXT_mxNavRegion(), (int)isInDSF);

  data_manager::sm.seedByProperty(seedOutput);



  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, true);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_remove_global_bool(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;
  //	int returnValue = 0;


  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive global parameter name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }

  script_manager::removeGlobalValue(key, missionx::mx_global_types::bool_type);
  mb_push_int(s, l, true);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_remove_global_number(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;
  //	int returnValue = 0;


  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive global parameter name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }

  script_manager::removeGlobalValue(key, missionx::mx_global_types::number_type);
  mb_push_int(s, l, true);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_remove_global_string(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outKey                   = NULL;
  std::string           errMsg;
  std::string           key;
  //	int returnValue = 0;


  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outKey));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive global parameter name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (!checkString(outKey, key))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }

  script_manager::removeGlobalValue(key, missionx::mx_global_types::string_type);
  mb_push_int(s, l, true);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_abort_mission(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1;
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outReason                = NULL;
  std::string           errMsg;
  std::string           abortReason;

  errMsg.clear();
  abortReason.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outReason));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (checkString(outReason, abortReason))
  {
    missionx::data_manager::mx_global_settings.setNodeStringProperty(mxconst::get_PROP_MISSION_ABORT_REASON(), abortReason); // v3.303.11
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::abort_mission);

    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    // return status
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ". Script did not provide a reason to abort. Please fix script. Skipping.";
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_update_end(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 3;
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outTagName               = NULL;
  char*                 outAttribName            = NULL;
  char*                 outAttribValue           = NULL;
  std::string           errMsg;
  std::string           inTagName;
  std::string           inAttribName;
  std::string           inAttribValue;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outTagName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outAttribName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outAttribValue));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter != noOfAttributesFromScript)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if ((!checkString(outTagName, inTagName)) || (!checkString(outAttribName, inAttribName)) || (!checkString(outAttribValue, inAttribValue)))
  {
    errMsg = fmt::format("One of the parameters is invalid, Tag Name: {}, Attrib Name: {}, Attrib Value: {}", inTagName, inAttribName, inAttribValue);
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  if (Utils::xml_search_and_set_attribute_in_IXMLNode(data_manager::endMissionElement.node, inAttribName, inAttribValue)) // if success
  {
    errMsg.clear();
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else // failed updating
  {
    // return status
    errMsg = fmt::format("Could not modify requested attribute. Tag Name: {}, Attrib Name: {}, Attrib Value: {}", inTagName, inAttribName, inAttribValue);
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }
  return result;
}

// -----------------------------------

// v3.0.221.11+
int
missionx::ext_script::ext_execute_commands(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; // provide xplane commands to execute example: "sim/engine/mixture_off"
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  size_t                 counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outCommands              = nullptr;
  std::string            errMsg;
  std::string            commands_s;

  errMsg.clear();
  commands_s.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outCommands));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter < noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  commands_s = std::string(outCommands);

  // check mandatory attributes have value
  if (commands_s.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received EMPTY commands list (as comma delimeted string). Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // call the command executer
  errMsg = missionx::data_manager::execute_commands(commands_s);
  data_manager::sm.seedError(errMsg);
  if (errMsg.empty())
    mb_push_int(s, l, true); // success
  else
    mb_push_int(s, l, false); // false


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_next_leg(mb_interpreter_t* s, void** l) // v3.0.205.3
{
  constexpr const size_t noOfAttributesFromScript = 1; // provide next goal name
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outName                  = nullptr;
  std::string            errMsg;
  std::string            nextLegName;

  errMsg.clear();
  nextLegName.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  nextLegName = std::string(outName);

  // check mandatory attributes have value
  if (nextLegName.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received EMPTY \"next\" Goal name value. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // check if we have a GOAL seeded information, in current context/scope
  if (data_manager::smPropSeedValues.hasProperty(mxconst::get_EXT_MX_CURRENT_LEG())) // v3.0.221.15rc5 add support to LEG // data_manager::smPropSeedValues.hasProperty(mxconst::EXT_MX_CURRENT_GOAL) ||
  {
    std::string seededGoalName;
    seededGoalName.clear();

    if (data_manager::smPropSeedValues.hasProperty(mxconst::get_EXT_MX_CURRENT_LEG())) // v3.0.221.15rc5 add LEG support
      seededGoalName = data_manager::smPropSeedValues.getPropertyValue(mxconst::get_EXT_MX_CURRENT_LEG(), errMsg);
    // else
    //   seededGoalName = data_manager::smPropSeedValues.getPropertyValue(mxconst::EXT_MX_CURRENT_GOAL, errMsg);


    if (seededGoalName.empty())
    {
      errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not find Goal name in this scope. Please fix your function call. skipping command.";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
      return result;
    }

    // check if "next goal name" is not equal current goal
    if (seededGoalName.compare(nextLegName) == 0)
    {
      errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not set \"next goal\" name to be same as the current active goal. Please fix your function call. skipping command.";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
      return result;
    }

    // check if "next goal" is not complete since we do not have reset mechanism
    if (Utils::isElementExists(missionx::data_manager::mapFlightLegs, nextLegName) && (missionx::data_manager::mapFlightLegs[nextLegName].getIsComplete()))
    {
      errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " could not set \"next goal\" name to be a complete goal. Please fix your function call. skipping command.";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
      return result;
    }

    missionx::data_manager::mapFlightLegs[seededGoalName].setStringProperty(mxconst::get_ATTRIB_NEXT_LEG(), nextLegName);
    // return status success
    missionx::data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_stop_draw_script(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;
  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));

  missionx::data_manager::draw_script.clear();
  // return status success
  missionx::data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, true);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_draw_script_name(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  short             counter  = 0;
  int               result   = MB_FUNC_OK;
  char*             outName  = nullptr;
  std::string       errMsg;
  std::string       new_script_name;

  errMsg.clear();
  new_script_name.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outName));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter > 0)
    new_script_name = std::string(outName);

  missionx::data_manager::draw_script = new_script_name;
  // return status success
  missionx::data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, true);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_current_leg_desc(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 0; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  std::string           errMsg;
  std::string           sText{ "" };
  char*                 returnValue = NULL;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));

  sText       = data_manager::mapFlightLegs[data_manager::currentLegName].getNodeStringProperty(mxconst::get_ELEMENT_DESC(), "", true);
  returnValue = mb_memdup(sText.c_str(), unsigned(sText.length() + (size_t)1));
  missionx::data_manager::sm.seedError(errMsg);
  mb_push_string(s, l, returnValue);


  return MB_FUNC_OK;
}

// -----------------------------------

int
missionx::ext_script::ext_set_leg_desc(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 0; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  std::string           errMsg{ "" };
  std::string           sText{ "" };
  std::string           sLegName{ "" };
  char*                 returnValue = NULL;
  char*                 outString1  = NULL;
  char*                 outString2  = NULL;
  bool                  bSuccess    = false;


  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outString1));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outString2));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < 1)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", Function did not received arguments. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return MB_FUNC_OK;
  }

  if (counter == 1)
  {
    sLegName = data_manager::currentLegName;

    if (!checkString(outString1, sText))
    {
      errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty text. Please fix your script. skipping command.";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
    }
  }

  if (counter > 1)
  {
    if (!checkString(outString1, sLegName))
    {
      errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty Flight Leg Name. Please fix your script. skipping command.";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
    }
    if (!checkString(outString2, sText))
    {
      errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty text. Please fix your script. skipping command.";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, false);
    }
  }


  if (mxUtils::isElementExists(data_manager::mapFlightLegs, sLegName))
  {
    bSuccess = true;
    data_manager::mapFlightLegs[sLegName].setNodeStringProperty(mxconst::get_ELEMENT_DESC(), sText, true);
    Utils::xml_add_cdata(data_manager::mapFlightLegs[sLegName].node, sText); // replace the original description to replace leg description and to be reflected in savepoint.
  }
  else
  {
    bSuccess = false;
    errMsg   = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", Could not find current leg: " + sLegName + ". Skipping.";
  }

  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, bSuccess);


  return MB_FUNC_OK;
}


// -----------------------------------

int
missionx::ext_script::ext_timelapse_add_minutes(mb_interpreter_t* s, void** l)
{
  const std::string funcName          = std::string(__func__).replace(0, 4, "fn_");
  short             counter           = 0;
  int               result            = MB_FUNC_OK;
  char*             outMinutes        = nullptr;
  char*             outSecondsToCycle = nullptr;
  std::string       errMsg;
  std::string       minutes_s;
  std::string       howManyCycles_s; // how many seconds should pass until the end.   1<= x <=24

  errMsg.clear();
  minutes_s.clear();
  howManyCycles_s.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outMinutes));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outSecondsToCycle));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter <= 0)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  if (outMinutes != nullptr)
    minutes_s = std::string(outMinutes);
  if (outSecondsToCycle != nullptr)
    howManyCycles_s = std::string(outSecondsToCycle);

  // validate and default initialization to attributes if were not defined
  if (!Utils::is_number(minutes_s))
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received MINUTES in wrong datatype: " + minutes_s;
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  if (howManyCycles_s.empty() || !Utils::is_number(howManyCycles_s))
    howManyCycles_s = "1";

  // make sure timelap is not active + start it
  if (missionx::data_manager::timelapse.flag_isActive)
  {
    errMsg = "TimeLapsed is active. Can't interfier with its action. Aborting your request. Suggestion: Time it differently.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }
  else
  {
    int minutes_i         = Utils::stringToNumber<int>(minutes_s);
    int how_many_cycles_i = Utils::stringToNumber<int>(howManyCycles_s);
    errMsg                = missionx::data_manager::timelapse.timelapse_add_minutes(minutes_i, how_many_cycles_i);
    if (!errMsg.empty())
    {
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, 0);
      return result;
    }
  }

  // send success;
  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, 1);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_timelapse_to_local_hour(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; // next_local_hour:minutes_s, how_many_cycles
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  size_t                 counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outNextLocalHour         = nullptr;
  // char* outMinutes = nullptr;
  char*       outSecondsToCycle = nullptr;
  std::string errMsg;
  std::string nextLocalHour_s;
  std::string howManyCycles_s; // how many seconds should pass until the end.   1<= x <=24

  errMsg.clear();
  nextLocalHour_s.clear();
  howManyCycles_s.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outNextLocalHour));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outSecondsToCycle));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter < noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  if ((outNextLocalHour != nullptr))
    nextLocalHour_s = std::string(outNextLocalHour);

  if ((outSecondsToCycle != nullptr))
    howManyCycles_s = std::string(outSecondsToCycle);

  // validate and default initialization to attributes if were not defined
  if (nextLocalHour_s.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received HOURS in wrong value type: " + nextLocalHour_s;
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }


  if (howManyCycles_s.empty() || !Utils::is_number(howManyCycles_s))
    howManyCycles_s = "-1";

  // make sure timelapse is not active + start it
  if (missionx::data_manager::timelapse.flag_isActive)
  {
    errMsg = "TimeLapsed is active. Can't interfier with its action. Aborting your request. Suggestion: Time it differently.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }
  else
  {
    int nextLocalHour_i    = 0;
    int minutes_i          = 0;
    int cycles_i           = -1;
    int seconds_to_cycle_i = Utils::stringToNumber<int>(howManyCycles_s);

    errMsg = missionx::Utils::convert_string_to_24_min_numbers(nextLocalHour_s, nextLocalHour_i, minutes_i, cycles_i);
    if (errMsg.empty())
      errMsg = missionx::data_manager::timelapse.timelapse_to_local_hour(nextLocalHour_i, minutes_i, ((cycles_i < 0) ? seconds_to_cycle_i : cycles_i)); // send cycles from script or from the format H24:MI:cycles

    if (!errMsg.empty())
    {
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, 0);
      return result;
    }
  }

  // send success;
  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, 1);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_is_timelapse_active(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;
  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));

  if (missionx::data_manager::timelapse.flag_isActive)
  {
    // send yes = is active, so we cant use it
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 1);
  }
  else
  {
    // send no = we can use the timelap
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
  }


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_local_time(mb_interpreter_t* s, void** l)
{
  const std::string funcName        = std::string(__func__).replace(0, 4, "fn_");
  short             counter         = 0;
  int               result          = MB_FUNC_OK;
  char*             outNewLocalHour = nullptr;
  char*             outNewMinutes   = nullptr;
  char*             outNewDayInYear = nullptr;
  std::string       errMsg;
  std::string       newLocalHour_s;
  std::string       newMinutes_s;
  std::string       newDayOfYear_s; // how many seconds should pass until the end.   1<= x <=24

  errMsg.clear();
  newLocalHour_s.clear();
  newMinutes_s.clear();
  newDayOfYear_s.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outNewLocalHour));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outNewMinutes));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outNewDayInYear));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter <= 0)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  if ((outNewLocalHour != nullptr))
    newLocalHour_s = std::string(outNewLocalHour);
  if ((outNewMinutes != nullptr))
    newMinutes_s = std::string(outNewMinutes);
  if ((outNewDayInYear != nullptr))
    newDayOfYear_s = std::string(outNewDayInYear);


  // validate and default initialization to attributes if were not defined
  if (newLocalHour_s.empty() || !Utils::is_number(newLocalHour_s))
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received HOURS in wrong value type: " + newLocalHour_s;
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  if (!Utils::is_number(newMinutes_s))
    newMinutes_s = "0";

  if (newDayOfYear_s.empty() || !Utils::is_number(newDayOfYear_s))
    newDayOfYear_s = "-1"; // ignore year configuration


  int newLocalHour_i = Utils::stringToNumber<int>(newLocalHour_s);
  int newMinutes_i   = Utils::stringToNumber<int>(newMinutes_s);
  int newDayOfYear_i = Utils::stringToNumber<int>(newDayOfYear_s);

  // call set time function
  errMsg = missionx::data_manager::set_local_time(newLocalHour_i, newMinutes_i, newDayOfYear_i);
  if (!errMsg.empty())
  {
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
  }
  else
  {
    // send success;
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 1);
  }

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_inject_metar_file(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 1; // seconds
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                 counter                  = 0;
  int                   result                   = MB_FUNC_OK;
  char*                 outValue                 = nullptr;
  std::string           errMsg;
  std::string           file_name_s;

  errMsg.clear();
  file_name_s.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outValue));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }
  if (outValue)
    file_name_s = std::string(outValue);

  // check mandatory attributes have value
  if (file_name_s.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received empty file name value. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }


  missionx::data_manager::inject_metar_file(file_name_s);
  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, 0);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_position_plane(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 5;
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");


  typedef enum
    : uint8_t
  {
    lat     = 0,
    lon     = 1,
    elev_mt = 2,
    heading = 3,
    speed   = 4
  } outVals;

  short                    counter = 0;
  int                      result  = MB_FUNC_OK;
  std::string              errMsg;
  std::vector<double>      vecAttrib;
  std::vector<std::string> vecValues;

  vecAttrib.clear();
  vecValues.clear();

  // initialize vector
  for (int i = 0; i < noOfAttributesFromScript; i++)
    vecAttrib.push_back(0.0);


  // Read attributes from MY-BASIC
  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  bool found = true;
  for (int i = 0; i < noOfAttributesFromScript && found; i++)
  {
    if (mb_has_arg(s, l))
    {
      mb_check(mb_pop_real(s, l, &vecAttrib[i]));
      ++counter;
      found = true;
    }
    else
      found = false;
  } // end loop
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < noOfAttributesFromScript) // Only message name us mandatory, all other attributes can be optional
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive all needed parameters. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }


  // Read all information
  std::map<uint8_t, double> mapPos;
  for (int i1 = 0; i1 < noOfAttributesFromScript; ++i1)
  {
    switch (i1)
    {
      case outVals::lat:
      {
        const double val_d = vecAttrib.at(i1);
        if (val_d != 0.0)
          Utils::addElementToMap(mapPos, outVals::lat, val_d);
      }
      break;
      case outVals::lon:
      {

        const double val_d = vecAttrib.at(i1);
        if (val_d != 0.0)
          Utils::addElementToMap(mapPos, outVals::lon, val_d);
      }
      break;
      case outVals::elev_mt:
      {

        const double val_d = vecAttrib.at(i1);
        Utils::addElementToMap(mapPos, outVals::elev_mt, val_d);
      }
      break;
      case outVals::heading:
      {

        const double val_d = vecAttrib.at(i1);
        Utils::addElementToMap(mapPos, outVals::heading, val_d);
      }
      break;
      case outVals::speed:
      {
        const double val_d = vecAttrib.at(i1);
        if (val_d >= 0.0)
          Utils::addElementToMap(mapPos, outVals::speed, val_d);
      }
      break;
      default:
        Log::logMsgNone("[fn_position_plane] Found value that is not supported: " + Utils::formatNumber<int>(i1) + ", Notify developer.");
    } // end switch
  }

  if (mapPos.size() >= (size_t)noOfAttributesFromScript)
  {
    // position the plane
    XPLMPlaceUserAtLocation(mapPos[outVals::lat], mapPos[outVals::lon], (float)mapPos[outVals::elev_mt], (float)mapPos[outVals::heading], (float)mapPos[outVals::speed]);
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", might receive a value it does not support or expected, please double check your parameters, make sure they are all in double quotes \". Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }



  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_position_camera(mb_interpreter_t* s, void** l)
{
  constexpr const short noOfAttributesFromScript = 6; //
  constexpr const short mandatoryArgs            = 3; //
  const std::string     funcName                 = std::string(__func__).replace(0, 4, "fn_");

  // typedef struct {float x;    float y;    float z;    float pitch;    float heading;    float roll;    float zoom;  } XPLMCameraPosition_t;

  typedef enum
    : uint8_t
  {
    x = 0, // lat
    y = 1, // lon
    z = 2, // elev
    h = 3, // heading
    p = 4, // pitch
    r = 5  // roll
  } outVals;

  short                    counter = 0;
  int                      result  = MB_FUNC_OK;
  std::string              errMsg;
  std::vector<double>      vecAttrib;
  std::vector<std::string> vecValues;

  vecAttrib.clear();
  vecValues.clear();

  // initialize vector
  for (int i = 0; i < noOfAttributesFromScript; i++)
    vecAttrib.push_back(0.0);


  // Read attributes from MY-BASIC
  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  bool found = true;
  for (int i = 0; i < noOfAttributesFromScript && found; ++i)
  {
    if (mb_has_arg(s, l))
    {
      mb_check(mb_pop_real(s, l, &vecAttrib[i]));
      ++counter;
      found = true;
    }
    else
      found = false;
  } // end loop
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < mandatoryArgs) // check if minimal mandatory args were received
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive all needed parameters. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }


  // Read all information
  std::map<uint8_t, float> mapPos = { { 0, 0.0f }, { 1, 0.0f }, { 2, 0.0f }, { 3, 0.0f }, { 4, 0.0f }, { 5, 0.0f } };
  for (int i1 = 0; i1 < noOfAttributesFromScript; ++i1)
  {
    switch (i1)
    {
      case outVals::x:
      {
        const float val_f = (float)vecAttrib.at(i1);
        if (val_f != 0.0)
          mapPos[i1] = val_f;
      }
      break;
      case outVals::y:
      {

        const float val_f = (float)vecAttrib.at(i1);
        if (val_f != 0.0)
          mapPos[i1] = val_f;
      }
      break;
      case outVals::z:
      {

        const float val_f = (float)vecAttrib.at(i1);
        mapPos[i1]        = val_f;
      }
      break;
      case outVals::h:
      {

        const float val_f = (float)vecAttrib.at(i1);
        mapPos[i1]        = val_f;
      }
      break;
      case outVals::p:
      {
        const float val_f = (float)vecAttrib.at(i1);
        if (val_f >= 0.0)
          mapPos[i1] = val_f;
      }
      break;
      case outVals::r:
      {
        const float val_f = (float)vecAttrib.at(i1);
        if (val_f >= 0.0)
          mapPos[i1] = val_f;
      }
      break;
      default:
        Log::logMsgNone("[fn_position_plane] Found value that is not supported: " + Utils::formatNumber<int>(i1) + ", Notify developer.");
    } // end switch
  }

  if (Utils::isElementExists(mapPos, 0) && Utils::isElementExists(mapPos, 1) && Utils::isElementExists(mapPos, 2))
  {
    double x, y, z;
    XPLMWorldToLocal(mapPos[0], mapPos[1], (float)(mapPos[2] * missionx::feet2meter), &x, &y, &z);


    // position the plane
    XPLMCameraPosition_t cp;
    cp.x       = (float)x;
    cp.y       = (float)y;
    cp.z       = (float)z;
    cp.heading = mapPos[3];
    cp.pitch   = mapPos[4];
    cp.roll    = mapPos[5];
    cp.zoom    = 0.0f; // no zoom

    missionx::data_manager::position_camera(cp, mapPos[0], mapPos[1]); // send camera position, latitude, longitude

    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, true);
  }
  else
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", might receive a value it does not support or expected, please double check your parameters, make sure they are all in double quotes \". Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
  }



  return result;
}


// -----------------------------------


int
missionx::ext_script::ext_set_choice_name(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; // provide next goal name
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outValue                 = nullptr;
  std::string            errMsg;


  errMsg.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outValue));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  const std::string choice_name = std::string(outValue);

  // check mandatory attributes have value
  if (choice_name.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received empty \"Choice Name\" value. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
  }
  else
  {
    if (missionx::data_manager::prepare_choice_options(choice_name)) // if exists then also prepare data_manager::mxChoice class
    {
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, 1);
    }
    else
    {
      errMsg = "There is no Choice by the name: " + mxconst::get_QM() + choice_name + mxconst::get_QM() + ".";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, 0);
    }
  }


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_active_choice_name(mb_interpreter_t* s, void** l)
{
  //  constexpr const size_t noOfAttributesFromScript = 0;
  const std::string funcName    = std::string(__func__).replace(0, 4, "fn_");
  char*             returnValue = nullptr;
  int               result      = MB_FUNC_OK;
  std::string       errMsg;

  errMsg.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));

  returnValue = mb_memdup(missionx::data_manager::mxChoice.currentChoiceName_beingDisplayed_s.c_str(), unsigned(missionx::data_manager::mxChoice.currentChoiceName_beingDisplayed_s.length() + (size_t)1));

  missionx::data_manager::sm.seedError(errMsg);
  mb_push_string(s, l, returnValue);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_display_choice_window(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;

  errMsg.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));

  missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::display_choice_window);

  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, 1);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_hide_choice_window(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;

  errMsg.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));

  missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::hide_choice_window);

  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, 1);

  return result;
}


// -----------------------------------


int
missionx::ext_script::ext_get_distance_between_two_points_mt(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 4;
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  const int              invalidResult            = -1; // distance can't be negative
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  double                 lat1_d, lon1_d, lat2_d, lon2_d;
  std::string            errMsg;

  lat1_d = lon1_d = lat2_d = lon2_d = 0.0;

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &lat1_d));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &lon1_d));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &lat2_d));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &lon2_d));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if ((counter != noOfAttributesFromScript) + (lat1_d * lon1_d * lat2_d * lon2_d == 0.0))
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list or one of the coordinates is 0. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, invalidResult);
    return result;
  }


  missionx::Point p1(lat1_d, lon1_d);
  missionx::Point p2(lat2_d, lon2_d);


  data_manager::sm.seedError(errMsg);
  mb_push_real(s, l, p1.calcDistanceBetween2Points(p2, missionx::mx_units_of_measure::meter));

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_distance_to_instance_in_meters(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; // provide instance name
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outValue                 = nullptr; // instance name
  std::string            errMsg;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outValue));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, 1.0);
    return result;
  }

  const std::string instance_name = std::string(outValue);

  // check mandatory attributes have value
  if (instance_name.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received empty \"Instance Name\" value. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, -1.0);
  }
  else
  {
    // Calculate distance to instance
    // 1. Check if instance is active
    // 1.1 If not then return -1
    // 1.2 If yes, then get instance location + plane location and return the distance
    const double distance_d = missionx::data_manager::get_distance_of_plane_to_instance_in_meters(instance_name, errMsg);
    if (distance_d >= 0.0)
    {
      data_manager::sm.seedError(errMsg);
      mb_push_real(s, l, distance_d);
    }
    else
    {
      data_manager::sm.seedError(errMsg);
      mb_push_real(s, l, -1.0);
    }
  }


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_elev_to_instance_in_feet(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; //  provide instance name
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outValue                 = nullptr; // instance name
  std::string            errMsg;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outValue));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, -1.0);
    return result;
  }

  const std::string instance_name = std::string(outValue);

  // check mandatory attributes have value
  if (instance_name.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received empty \"Instance Name\" value. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, -1.0);
  }
  else
  {
    // get elevation difference between plane and instance
    const double elev_distance_d = missionx::data_manager::get_elev_of_plane_relative_to_instance_in_feet(instance_name, errMsg);
    if (elev_distance_d >= 0.0)
    {
      data_manager::sm.seedError(errMsg);
      mb_push_real(s, l, elev_distance_d);
    }
    else
    {
      data_manager::sm.seedError(errMsg);
      mb_push_real(s, l, -1.0);
    }
  }


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_bearing_to_instance(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; // provide instance name
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outValue                 = nullptr; // instance name
  std::string            errMsg;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outValue));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, -1.0);
    return result;
  }

  const std::string instance_name = std::string(outValue);

  // check mandatory attributes have value
  if (instance_name.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received empty \"Instance Name\" value. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, -1.0);
  }
  else
  {
    // get elevation difference between plane and instance
    const double bearing_d = missionx::data_manager::get_bearing_of_plane_to_instance_in_deg(instance_name, errMsg);
    if (bearing_d >= 0.0)
    {
      data_manager::sm.seedError(errMsg);
      mb_push_real(s, l, bearing_d);
    }
    else
    {
      data_manager::sm.seedError(errMsg);
      mb_push_real(s, l, -1.0);
    }
  }


  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_add_gps_xy_entry(mb_interpreter_t* s, void** l)
{
  const std::string funcName      = std::string(__func__).replace(0, 4, "fn_");
  const double      invalidResult = -1.0; // -1 represent invalid outcome
  short             counter       = 0;
  int               result        = MB_FUNC_OK;
  std::string       errMsg;

  Point pTarget;
  Point pPlane;

  int    entry_i = -1; // add to the end of the enry list, which is also the default
  double lat, lon, elev_ft;
  lat = lon = elev_ft = 0.0;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &lat));
    pTarget.setLat(lat);
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &lon));
    pTarget.setLon(lon);
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &elev_ft));
    pTarget.setLon(lon);
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &entry_i));
    pTarget.setLon(lon);
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter < 2)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Needs at least: lat/lon. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, invalidResult);
    return result;
  }

  Point p(lat, lon, elev_ft);
  missionx::data_manager::addLatLonEntryToGPS(p, entry_i);

  mb_push_int(s, l, 1); // success

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_aircraft_model(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  int               index    = XPLM_USER_AIRCRAFT; // XPLM_USER_AIRCRAFT = 0

  char        outFileName[512]{ 0 };
  char        outPath[1024]{ 0 };
  std::string errMsg;
  std::string fileName_s;

  errMsg.clear();
  fileName_s.clear();

  // read only first value if present, if not then we use the default "0"
  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &index));
  }
  mb_check(mb_attempt_close_bracket(s, l));

  XPLMGetNthAircraftModel(index, outFileName, outPath); // we will only return the file name

  checkString(outFileName, fileName_s);

  // we always want to return a string, even an empty one
  missionx::data_manager::sm.seedError(errMsg);
  auto value = mb_memdup(fileName_s.c_str(), (unsigned)(fileName_s.length() + 1)); // `+1` means to allocate and copy one extra byte of the ending '\0'
  mb_push_string(s, l, value);


  return result;
}


// -----------------------------------

int
missionx::ext_script::ext_open_inventory_screen(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;

  // read only first value if present, if not then we use the default "0"
  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));

  mb_check(mb_attempt_close_bracket(s, l));

  missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::open_inventory_layout);

  // always success
  missionx::data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_open_image_screen(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;

  // read only first value if present, if not then we use the default "0"
  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));

  mb_check(mb_attempt_close_bracket(s, l));

  missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::open_map_layout);

  // always success
  missionx::data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------


int
missionx::ext_script::ext_load_image_to_leg(mb_interpreter_t* s, void** l)
{
  [[maybe_unused]] constexpr const short noOfAttributesFromScript = 2; //
  const std::string                      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                                  counter                  = 0;
  int                                    result                   = MB_FUNC_OK;
  char*                                  outFileName              = NULL; // mandatory
  char*                                  outLegName               = NULL; // optional
  std::string                            errMsg;
  std::string                            fileName;
  std::string                            legName; // optional

  fileName.clear();
  legName.clear();


  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outFileName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outLegName));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  if (counter == 0)
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive expected arguments. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  // check mandatory value - File Name
  if (!checkString(outFileName, fileName))
  {
    errMsg = "[ERROR] Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", received an empty mandatory File Name. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, false);
    return result;
  }

  checkString(outLegName, legName);
  if (legName.empty() || (mxUtils::isElementExists(missionx::data_manager::mapFlightLegs, legName) == false)) // if legName is empty or legName does not exists then add to current running leg
    legName = missionx::data_manager::currentLegName;

  if (missionx::data_manager::addAndLoadTextureMapNodeToLeg(fileName, legName))
  {
    errMsg.clear();
  }
  else
    errMsg = "Fail to load texture: " + fileName + " To Flight Leg: " + legName + ". Check Log.txt for more messages.";

  missionx::data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_hide_3D_markers(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;

  errMsg.clear();

  // read only first value if present, if not then we use the default "0"
  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));

  mb_check(mb_attempt_close_bracket(s, l));

  missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::hide_target_marker_option);

  // always success
  missionx::data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_show_3D_markers(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  int               result   = MB_FUNC_OK;
  std::string       errMsg;

  // read only first value if present, if not then we use the default "0"
  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));

  mb_check(mb_attempt_close_bracket(s, l));

  missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::show_target_marker_option);

  // always success
  missionx::data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_predefine_weather_code(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; // provide next goal name
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  int                    iWeatherType             = 0;
  std::string            errMsg;

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &iWeatherType));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }


  missionx::data_manager::apply_datarefs_based_on_string_parsing(missionx::data_manager::get_weather_state(iWeatherType));


  data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_datarefs(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; // provide next goal name
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outValue                 = nullptr;
  std::string            errMsg;
  std::string            sValue;

  sValue.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outValue));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  sValue = std::string(outValue);
  missionx::data_manager::apply_datarefs_based_on_string_parsing(sValue);


  data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_set_datarefs_interpolation(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 3; // seconds in each cycle, cycle number, datarefs to interpolate as string
  const std::string      funcName_s               = std::string(__func__).replace(0, 4, "fn_");
  const char*            funcName                 = funcName_s.c_str(); //  "fn_set_datarefs_interpolation"
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  int                    outSeconds               = 0;
  int                    outCycles                = 0;
  char*                  outValue                 = nullptr;
  std::string            errMsg;
  std::string            sValue;


  sValue.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outSeconds));
    ++counter;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outCycles));
    ++counter;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outValue));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  sValue = std::string(outValue);
  missionx::data_manager::do_datarefs_interpolation(outSeconds, outCycles, sValue);


  data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_reset_interpolation_list(mb_interpreter_t* s, void** l)
{
  [[maybe_unused]] constexpr const size_t noOfAttributesFromScript = 0;
  const std::string                       funcName                 = std::string(__func__).replace(0, 4, "fn_");
  int                                     result                   = MB_FUNC_OK;
  std::string                             errMsg;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));

  missionx::data_manager::clear_all_datarefs_from_interpolation_map();

  data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_remove_dref_from_interpolation_list(mb_interpreter_t* s, void** l)
{
  [[maybe_unused]] constexpr const size_t noOfAttributesFromScript = 1;
  const std::string                       funcName_s               = std::string(__func__).replace(0, 4, "fn_");
  short                                   counter                  = 0;
  int                                     result                   = MB_FUNC_OK;
  std::string                             errMsg;
  char*                                   drefList_ptr = nullptr;
  std::string                             drefList_s;

  errMsg.clear();
  drefList_s.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &drefList_ptr));
    ++counter;
  }

  mb_check(mb_attempt_close_bracket(s, l));
  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName_s + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  drefList_s = std::string(drefList_ptr);
  missionx::data_manager::remove_dataref_from_interpolation_map(drefList_s);

  data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_start_timer(mb_interpreter_t* s, void** l)
{
  [[maybe_unused]] constexpr const size_t noOfAttributesFromScript = 2; // timer number is mandatory and how much time to run in seconds
  const std::string                       funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                                   counter                  = 0;
  int                                     result                   = MB_FUNC_OK;
  int                                     outTimerNo               = 0;
  double                                  outTimerSeconds          = 0.0;
  std::string                             errMsg;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outTimerNo));
    ++counter;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_real(s, l, &outTimerSeconds));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter == 0)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  if (counter == 1)
    outTimerSeconds = 86400.0; // 24 hours

  if (!missionx::data_manager::mxThreeStoppers.start_timer(outTimerNo, (float)outTimerSeconds))
  {
    errMsg = "Timer did not started, please check if the Timer Number is between 1 and 3.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }


  data_manager::sm.seedError(errMsg);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_stop_timer(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; // timer number (1-3)
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  int                    outTimerNo               = 0;
  double                 returnTimePassed         = 0.0;
  std::string            errMsg;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outTimerNo));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }


  returnTimePassed = missionx::data_manager::mxThreeStoppers.stopTimer(outTimerNo);

  if (returnTimePassed < 0.0)
  {
    errMsg = "No information returned, please check if the Timer Number is between 1 and 3.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnTimePassed);
    return result;
  }


  data_manager::sm.seedError(errMsg);
  mb_push_real(s, l, returnTimePassed);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_timer_time_passed(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; // timer number (1-3)
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  int                    outTimerNo               = 0;
  double                 returnTimePassed         = 0.0;
  std::string            errMsg;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outTimerNo));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }


  returnTimePassed = missionx::data_manager::mxThreeStoppers.getTimerTimePassed(outTimerNo);

  if (returnTimePassed < 0.0)
  {
    errMsg = "No information returned, please check if the Timer Number is between 1 and 3.";
    data_manager::sm.seedError(errMsg);
    mb_push_real(s, l, returnTimePassed);
    return result;
  }


  data_manager::sm.seedError(errMsg);
  mb_push_real(s, l, returnTimePassed);
  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_get_timer_ended(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 1; // timer number (1-3)
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  int                    outTimerNo               = 0;
  int                    returnTimeWasPasswd_i    = 0;
  std::string            errMsg;

  errMsg.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outTimerNo));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  if (outTimerNo >= 1 && outTimerNo <= 3)
  {
    returnTimeWasPasswd_i = missionx::data_manager::mxThreeStoppers.getTimerEnded(outTimerNo);
  }
  else
  {
    errMsg = "You provided wrong timer code number, must be 1,2 or 3";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, returnTimeWasPasswd_i);
    return result;
  }


  data_manager::sm.seedError(errMsg);
  mb_push_real(s, l, returnTimeWasPasswd_i);
  return result;
}



// -----------------------------------

int
missionx::ext_script::ext_is_item_exists_in_plane(mb_interpreter_t* s, void** l)
{

  constexpr const size_t noOfAttributesFromScript = 1; // provide next goal name
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outValue                 = nullptr;
  std::string            errMsg;
  std::string            barcode;

  errMsg.clear();
  barcode.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outValue));
    ++counter;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  barcode = std::string(outValue);

  // check mandatory attributes have value
  if (barcode.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received empty barcode value. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  assert(missionx::Utils::isElementExists(missionx::data_manager::mapInventories, mxconst::get_ELEMENT_PLANE()) && std::string(__func__).append(": No plane inventory node!!!").c_str());

  int quantity_i = get_inventory_barcode_quantity(mxconst::get_ELEMENT_PLANE(), barcode, errMsg);

  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, quantity_i);


  return result;
}

// -----------------------------------

int
missionx::ext_script::get_inventory_barcode_quantity(const std::string& inventoryName_s, const std::string& inBarcodeValueToSearch, std::string& errMsg)
{
  // Quantity value meanings: q<0 meaning no item in the inventory.
  int quantity_i = 0;
  errMsg.clear();

  if (Utils::isElementExists(data_manager::mapInventories, inventoryName_s))
  {
    quantity_i = data_manager::mapInventories[inventoryName_s].get_item_quantity(inBarcodeValueToSearch);
    if (quantity_i < 1)
    {
      errMsg = fmt::format("No item barcode: {}, found in inventory: {}.",inBarcodeValueToSearch, inventoryName_s);
      quantity_i = 0; // force reset to Zero to make sure no negative numbers
    }

  }
  else
    errMsg = "Inventory: " + inventoryName_s + ", is not present. please fix your code.";

  return quantity_i;
}

// -----------------------------------

int
missionx::ext_script::ext_is_item_exists_in_ext_inventory(mb_interpreter_t* s, void** l)
{
  constexpr const size_t noOfAttributesFromScript = 2; // inventory_name, barcode
  const std::string      funcName                 = std::string(__func__).replace(0, 4, "fn_");
  short                  counter                  = 0;
  int                    result                   = MB_FUNC_OK;
  char*                  outInvName               = nullptr;
  char*                  outBarcode               = nullptr;
  std::string            errMsg;
  std::string            invName;
  std::string            barcode;

  errMsg.clear();
  barcode.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outInvName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outBarcode));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter != noOfAttributesFromScript)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  barcode = std::string(outBarcode);
  invName = std::string(outInvName);

  // check mandatory attributes have value
  if (barcode.empty() || invName.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received empty inventory name or barcode value. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  const int quantity_i = get_inventory_barcode_quantity(invName, barcode, errMsg);

  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, quantity_i);

  return result;
}

// -----------------------------------

int
missionx::ext_script::ext_move_item_from_inv(mb_interpreter_t* s, void** l)
{
  // An inventory item can be moved from source store to target.
  // or if target is not set or available then discard the item.

  const std::string funcName       = std::string(__func__).replace(0, 4, "fn_");
  int               counter        = 0;
  constexpr int     result         = MB_FUNC_OK;
  char*             outFromInvName = nullptr;
  char*             outBarcode     = nullptr;
  char*             outQuantity    = nullptr;
  char*             outToInvName   = nullptr;
  char*             outStationId   = nullptr;
  std::string       errMsg;
  std::string       fromInvName;
  std::string       barcode;
  std::string       quantity;
  std::string       toInvName; // can be empty
  std::string       StationId = "0"; // can be empty

  errMsg.clear();
  fromInvName.clear();
  barcode.clear();
  quantity.clear();
  toInvName.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outFromInvName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outBarcode));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outQuantity));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outToInvName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outStationId));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter < 2)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }
  else if (counter == 2)
  {
    quantity  = "1";
    StationId = "0";
  }

  fromInvName = (outFromInvName == nullptr) ? missionx::EMPTY_STRING : std::string(outFromInvName);
  barcode     = (outBarcode == nullptr) ? missionx::EMPTY_STRING : std::string(outBarcode);
  if (quantity.empty() )
    checkString(outQuantity, quantity);

  if (counter < 4)
    toInvName.clear();
  else
    checkString(outToInvName, toInvName);

  // v24.12.2 Read and validate station id information.
  if ( counter > 4 )
  {
    // StationId = (outStationId == nullptr)? "0" : std::string(outStationId);
    checkString(outStationId, StationId);
    if (StationId.empty() || !(mxUtils::is_digits(StationId)) )
      StationId = "0";
  }


  int nQuantityToMove = 1;
  if (Utils::is_number(quantity))
    nQuantityToMove = std::abs(Utils::stringToNumber<int>(quantity));


  // check mandatory attributes have value
  if (barcode.empty() || fromInvName.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received empty source inventory name or barcode value. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  // check and skip if from and to inventories are the same
  if (fromInvName == toInvName) // same location, return with success and don't do anything
  {
    errMsg.clear();
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 1);
    return result;
  }

  if (!Utils::isElementExists(missionx::data_manager::mapInventories, fromInvName)) // check if source inventory name exists
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received inventory name: " + mxconst::get_QM() + fromInvName + mxconst::get_QM() + " that is not exists. Please fix script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }
  
  if (!Utils::isElementExists(missionx::data_manager::mapInventories, toInvName)) // check if target inventory name exists, if not then treat it as discard.
  {
    toInvName.clear(); // ignore = discard item
  }

  // Make the transfer if item barcode exists
  if (missionx::data_manager::mapInventories[fromInvName].isItemExistsInInventory(barcode))
  {
    if (toInvName.empty())
    { // Discard item
      missionx::data_manager::mapInventories[fromInvName].discardItem(barcode, nQuantityToMove, mxUtils::stringToNumber<int>(StationId));
    }
    else
    { // add the "nQuantityToMove" to the target inventory
      auto item_node = missionx::data_manager::mapInventories[fromInvName].get_item_node_ptr(barcode);
      auto addResult = missionx::data_manager::mapInventories[toInvName].add_item(item_node, nQuantityToMove, mxUtils::stringToNumber<int>(StationId));
      // Check action result
      if (addResult.result == false)
      {
        errMsg.append(addResult.getErrorsAsText());
        data_manager::sm.seedError(errMsg);
        mb_push_int(s, l, 0);
        return result;
      }
    }

    // loop and erase any item with 0 quantity
    missionx::data_manager::erase_items_with_zero_quantity_in_all_inventories();

    // recalculate plane weight if we moved items to/from plane
    if (fromInvName == mxconst::get_ELEMENT_PLANE() || toInvName == mxconst::get_ELEMENT_PLANE())
    {
      missionx::data_manager::internally_calculateAndStorePlaneWeight(data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()], true, missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i); // v3.0.241.1 added "true" to change plane weight
    }


    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 1);
    return result;
  }

  errMsg = "Item with barcode: " + mxconst::get_QM() + barcode + mxconst::get_QM() + " was not found in inventory: " + mxconst::get_QM() + fromInvName + mxconst::get_QM();
  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, 0);

  return result;
}

// -----------------------------------

int
ext_script::ext_move_item_to_plane(mb_interpreter_t* s, void** l)
{
  // An inventory item can be moved from source store to target.
  // Source inventory is mandatory
  const std::string funcName       = std::string(__func__).replace(0, 4, "fn_");
  int               counter        = 0;
  constexpr int     result         = MB_FUNC_OK;
  char*             outFromInvName = nullptr;
  char*             outBarcode     = nullptr;
  int               outQuantity    = 1;
  int               outStationIndx = -1;
  std::string       errMsg;
  std::string       fromInvName;
  std::string       barcode;
  // const std::string toPlane = mxconst::get_ELEMENT_PLANE();


  errMsg.clear();
  fromInvName.clear();
  barcode.clear();

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outFromInvName));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_string(s, l, &outBarcode));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outQuantity));
    counter++;
  }
  if (mb_has_arg(s, l))
  {
    mb_check(mb_pop_int(s, l, &outStationIndx));
    counter++;
  }
  mb_check(mb_attempt_close_bracket(s, l));

  // Check if all attributes were read
  if (counter < 2)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + ", did not receive correct parameter list. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  fromInvName = (outFromInvName == nullptr) ? missionx::EMPTY_STRING : std::string(outFromInvName);
  barcode     = (outBarcode == nullptr) ? missionx::EMPTY_STRING : std::string(outBarcode);

  if (counter == 2)
  {
    outQuantity    = 1;
    outStationIndx = 0; // should be Pilot
  }

  // check mandatory attributes have value
  if (barcode.empty() || fromInvName.empty())
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received empty source inventory name or barcode value. Please fix your script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  // check and skip if from and to inventories are the same
  if (fromInvName == mxconst::get_ELEMENT_PLANE()) // same location, return with success and don't do anything
  {
    errMsg.clear();
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 1);
    return result;
  }

  if (!Utils::isElementExists(missionx::data_manager::mapInventories, fromInvName)) // check if inventory name exists or not (and if it is not plane)
  {
    errMsg = "Function: " + mxconst::get_QM() + funcName + mxconst::get_QM() + " received inventory name: " + mxconst::get_QM() + fromInvName + mxconst::get_QM() + " that is not exists. Please fix script. skipping command.";
    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 0);
    return result;
  }

  // Make the transfer
  if (missionx::data_manager::mapInventories[fromInvName].isItemExistsInInventory(barcode))
  {
   auto item_node = missionx::data_manager::mapInventories[fromInvName].get_item_node_ptr(barcode);
   if (!missionx::data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()].add_item(item_node, std::abs(outQuantity), outStationIndx).result)
    {
      errMsg = "Failed to move the item. Check log file for more information.";
      data_manager::sm.seedError(errMsg);
      mb_push_int(s, l, 0); // FAIL
      return result;
    }

    // loop and erase any item with 0 quantity
    missionx::data_manager::erase_items_with_zero_quantity_in_all_inventories(); // v24.12.2 added cleanup
    // recalculate plane weight if we moved items to/from plane
    missionx::data_manager::internally_calculateAndStorePlaneWeight(data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()], true, missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i); // v3.0.241.1 added "true" to change plane weight

    data_manager::sm.seedError(errMsg);
    mb_push_int(s, l, 1); // SUCCESS
    return result;        // SUCCESS
  }

  errMsg = "Item with barcode: " + mxconst::get_QM() + barcode + mxconst::get_QM() + " was not found in inventory: " + mxconst::get_QM() + fromInvName + mxconst::get_QM();
  data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, 0); // FAIL

  return result;
}


// -----------------------------------

int
ext_script::ext_get_inv_layout_type(mb_interpreter_t* s, void** l)
{
  const std::string funcName = std::string(__func__).replace(0, 4, "fn_");
  std::string       errMsg;

  mb_assert(s && l);
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));

  // always success
  missionx::data_manager::sm.seedError(errMsg);
  mb_push_int(s, l, ((missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY)? missionx::XP11_COMPATIBILITY : missionx::XP12_COMPATIBILITY ) ); // FAIL
  return MB_FUNC_OK;
}
