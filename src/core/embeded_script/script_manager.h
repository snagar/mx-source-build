#ifndef __SCRIPT_MANAGER_H_
#define __SCRIPT_MANAGER_H_
#pragma once

#include <map>
#include <unordered_map>

#include <stdarg.h>
#include "../../data/mxProperties.hpp"
#include "../Timer.hpp"
#include "base_script.hpp" // holds the script details
#include "my_basic.h"




namespace missionx
{
/*
Class : script manager should manage all occurrences of external script management.
Main engine is "bas" pointer.
All member functions that manage engine memory are not static.
Static members are mainly: maps and get/set functions that are shared between ALL BAS engines
*/

typedef enum _mx_global_types
  : uint8_t
{
  undefined_type,
  bool_type,
  number_type,
  string_type
} mx_global_types;

class script_manager //: public base_thread
{
private:
  std::string MX_ERROR = "MXERROR";

  static char bufPrintFromScript_static[512];
  static std::string sPrintFromScript_static;

  /////// SEED members
  static const int seedArraySize = 7;
  mb_value_t       arraySeed[seedArraySize];


  mb_value_t mxErrorToScript;

  static void _on_error(struct mb_interpreter_t* s, mb_error_e e, const char* m, const char* f, int p, unsigned short row, unsigned short col, int abort_code); // v3.0.96

  static Timer mxPadTimer;

  // parameters for error handling v3.0.198 - compatibility with latest my-basic build
  char           file[1024];
  char*          pFile;
  const char**   ppFile;
  std::string    fileStr;
  int            pos;
  unsigned short row;
  unsigned short col;


  const std::map<std::string, int> mxMapSeedVarsPos = { { mxconst::get_EXT_MX_FUNC_CALL(), 0 },       { mxconst::get_EXT_MX_CURRENT_LEG(), 1 },      { mxconst::get_EXT_MX_CURRENT_OBJ(), 2 },        { mxconst::get_EXT_MX_CURRENT_TASK(), 3 },
                                                      { mxconst::get_EXT_MX_CURRENT_TRIGGER(), 4 }, { mxconst::get_EXT_MX_CURRENT_3DOBJECT(), 5 }, { mxconst::get_EXT_MX_CURRENT_3DINSTANCE(), 6 }/*, { mxconst::EXT_MX_CURRENT_GOAL, 7 }*/ };

public:
  script_manager();
//  virtual ~script_manager();

  bool init(); // return success or failure

  mb_interpreter_t* bas;

  //// SEED Members
  void seedScriptCallArray(missionx::mxProperties& inPropertiesToSeed);
  void seedError(std::string& inErr);
  void seedStringParameter(mb_interpreter_t& bas, std::string& inNameVar, mb_value_t& mbValue, std::string inValue);



  ///////////////////

  bool init_ext_script();
  bool dispose_ext_script();
  bool init_bas();
  bool reset_bas();
  bool parse_script(missionx::base_script* inBScript);
  std::string run_script(const std::string scriptName, missionx::mxProperties& inPropertiesToSeed); // returns status of run script. If empty: all is good, if has string then it holds failure text

  bool ext_sm_init_success;
  bool ext_bas_open;

  static void clear();
  static bool getBoolValue(std::string& key, bool& outValue); // If key won't be found it will return false
  static bool getIntValue(std::string& key, int& outValue);
  static bool getDecimalValue(std::string& key, double& outValue);
  static bool getStringValue(std::string& key, std::string& outValue);
  static bool removeGlobalValue(std::string& key, mx_global_types inType);

  static bool  getPadTimer_wasTimePassed();
  static float getPadTimer_xplaneSecondsPassed();
  static float getPadTimer_osTimePassed();
  static float getPadTimer_secondsToRun();
  static void  resetPadTimer();
  static bool  stopPadTimer(std::string& outErr);
  static bool  startMxPadTimer(float inSecondsToRun, std::string& outErr);


  // checkpoint members
  static void saveCheckpoint(IXMLNode& inParent);


  static std::map<std::string, missionx::base_script> mapScripts; // will hold ALL external scripts. Key = file name without extension

  static std::unordered_map<std::string, std::string> mapScriptGlobalStringsArg; // will hold script strings to use between iterations.
  static std::unordered_map<std::string, bool>        mapScriptGlobalBoolArg;    // will hold script boolean, this is int=0 or int=1
  static std::unordered_map<std::string, double>      mapScriptGlobalDecimalArg; // will hold script float numbers. It will also holds int numbers we will convert to int before sending to script
  static std::map<std::string, XPLMNavRef>  mapScriptGlobalNavRef;     // will hold navigation reference from navigation searches

  static int my_print(struct mb_interpreter_t* s, const char* fmt, ...); // v3.303.14 inline with MY-BASIC changes

  // seed variables into script using mxProperties as mapper.
  // The key of the "map" will be the name of the variable.
  // The function can handle: bool, int, float, double and string
  void seedByProperty(mxProperties& prop);


  /////////// TEMPLATES ///////////
};

}

#endif //__SCRIPT_MANAGER_H_
