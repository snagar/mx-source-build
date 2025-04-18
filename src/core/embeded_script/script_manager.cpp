#include "script_manager.h"
#include <iostream>
#include <sstream>

#include "../../io/Log.hpp"
#include "../Utils.h"

using namespace missionx;

namespace missionx
{
std::map<std::string, missionx::base_script> missionx::script_manager::mapScripts; // will hold script strings to use between iterations.

std::unordered_map<std::string, std::string> missionx::script_manager::mapScriptGlobalStringsArg; // will hold script strings to use between BAS scripts.
std::unordered_map<std::string, bool>        missionx::script_manager::mapScriptGlobalBoolArg;    // will hold script boolean, this is int=0 or int=1
std::unordered_map<std::string, double>      missionx::script_manager::mapScriptGlobalDecimalArg; // will hold script decimal numbers (double). It will also holds int numbers we will convert to int before sending to script

std::map<std::string, XPLMNavRef>  missionx::script_manager::mapScriptGlobalNavRef;     // v3.0.194

Timer missionx::script_manager::mxPadTimer;

char missionx::script_manager::bufPrintFromScript_static[512]{ '\0' };
std::string missionx::script_manager::sPrintFromScript_static{ "" };
}


void
missionx::script_manager::_on_error(mb_interpreter_t* s, mb_error_e e, const char* m, const char* f, int p, unsigned short row, unsigned short col, int abort_code)
{
  mb_unrefvar(s);
  mb_unrefvar(p);
  std::string err;
  err.clear();
  std::string M = (m == nullptr ) ? missionx::EMPTY_STRING : std::string(m);
  std::string F = (f == nullptr ) ? missionx::EMPTY_STRING : std::string(f);

  if (e != SE_NO_ERR)
  {
    if (f)
    {
      if (e == SE_RN_REACHED_TO_WRONG_FUNCTION) // v3.303.14 MY-BASIC internal naming changes
      {
        err = "Error:\n    Ln " + mxUtils::formatNumber<unsigned short>(row) + ", Col " + mxUtils::formatNumber<unsigned short>(col) + " in Func: " + F + "\n    Code " + mxUtils::formatNumber<int>(e) + ", Abort Code " +
              mxUtils::formatNumber<int>(abort_code) + "\n    Message: " + M + ".\n";
        Log::logMsgNone(err);
      }
      else
      {
        int extended_abort_code = (e == SE_EA_EXTENDED_ABORT) ? abort_code - MB_EXTENDED_ABORT : abort_code;
        err                     = "Error:\n    Ln " + mxUtils::formatNumber<unsigned short>(row) + ", Col " + mxUtils::formatNumber<unsigned short>(col) + " in File: " + F + "\n    Code " + mxUtils::formatNumber<int>(e) + ", Abort Code " +
              mxUtils::formatNumber<int>(extended_abort_code) + "\n    Message: " + M + ".\n";
        Log::logMsgNone(err);

      }
    }
    else
    {
      int extended_abort_code = (e == SE_EA_EXTENDED_ABORT) ? abort_code - MB_EXTENDED_ABORT : abort_code;
      err                     = "Error:\n    Ln " + mxUtils::formatNumber<unsigned short>(row) + ", Col " + mxUtils::formatNumber<unsigned short>(col) + " in File: " + F + "\n    Code " + mxUtils::formatNumber<int>(e) + ", Abort Code " +
            mxUtils::formatNumber<int>(extended_abort_code) + "\n    Message: " + M + ".\n";
      Log::logMsgNone(err);

    }
  }
}

missionx::script_manager::script_manager()
{
  this->mxErrorToScript.type = MB_DT_NIL;  
  this->ext_sm_init_success = false;
  this->ext_bas_open        = false;
  this->bas                 = NULL;

  file[0] = '\0';
  fileStr.clear();
  pos = 0;
  row = (short)0;
  col = (short)0;

  this->pFile = nullptr;
  this->ppFile = nullptr;
}


//missionx::script_manager::~script_manager() {}

bool
missionx::script_manager::init()
{
  bool result = false;

#ifndef RELEASE
  Log::logXPLMDebugString("[script_manager] init() \n"); // debug
#endif
  if (init_ext_script())
    if (init_bas())
    {
      mb_set_printer(bas, this->my_print);

      result = true;
    }

  return result;
}

// -----------------------------------

void
missionx::script_manager::seedScriptCallArray(missionx::mxProperties& inPropertiesToSeed)
{
  static std::string err;
  err.clear();

  // prepare seeded data
  // loop over translation map
  // attribute names: EXT_MX_FUNC_CALL, EXT_MX_CURRENT_GOAL, PROP_MX_COBJ_GOAL, PROP_MX_TASK_GOAL, PROP_MX_TRIGGER_GOAL, EXT_MX_CURRENT_3DOBJECT, EXT_MX_CURRENT_3DINSTANCE
  for (auto &[name, arrayLoc] : this->mxMapSeedVarsPos)
  {
//    std::string name     = iter.first;
//    int         arrayLoc = iter.second;

    // getValue from properties map. If not exists then return empty string

    std::string value = inPropertiesToSeed.getPropertyValue(name, err);
    if (!err.empty())
    {
      value.clear();
      Log::logMsg(err);
    }


    char* cString = mb_memdup(value.c_str(), (unsigned)(value.length() + 1)); // `+1` means to allocate and copy one extra byte of the ending '\0'
    if (arrayLoc > (seedArraySize - 1))
    {
      Log::logMsgErr("[critical]Tried to insert seed value, out of bounds. Property name:" + mxconst::get_QM() + name + mxconst::get_QM() + ", array location: " + Utils::formatNumber<int>(arrayLoc));
      continue;
    }

    mb_make_string(this->arraySeed[arrayLoc], cString);
    mb_add_var(this->bas, NULL, Utils::stringToUpper(name).c_str(), this->arraySeed[arrayLoc], true);
  }
}
// seedScriptCallArray


// -----------------------------------

void
missionx::script_manager::seedError(std::string& inErr)
{
  char* cString = mb_memdup(inErr.c_str(), (unsigned)(inErr.length() + 1)); // `+1` means to allocate and copy one extra byte of the ending '\0'
  mb_make_string(this->mxErrorToScript, cString);
  mb_add_var(this->bas, NULL, Utils::stringToUpper(MX_ERROR).c_str(), this->mxErrorToScript, true);
}

// -----------------------------------

bool
missionx::script_manager::init_ext_script()
{
  this->ext_sm_init_success = false;

  if (mb_init() == MB_FUNC_OK)
  {
    this->ext_sm_init_success = true;
    Log::logXPLMDebugString("[External Script Initialized]\n");
  }
  else
    Log::logXPLMDebugString("ERROR !!! [External Script Fail Initialization]!!!\n");

  return this->ext_sm_init_success;
}

// -----------------------------------

bool
missionx::script_manager::dispose_ext_script()
{
#ifndef RELEASE
  Log::logMsg("[script_manager] dispose_ext_script() "); // debug
#endif

  // release BAS
  if (mb_close(&bas) == MB_FUNC_OK)
  {
    this->ext_bas_open = false;
    Log::logMsg("[Closed BAS Interface]");
  }
  else
    Log::logMsg("[Fail to close BAS interface]");

  // Dispose MB
  if (mb_dispose() == MB_FUNC_OK)
  {
    Log::logMsg("[External Script Manager Disposed]");
    this->ext_sm_init_success = false; // reset the pointState to flase so we need to re-initStatic.
    return true;
  }

  Log::logMsgErr("[External Script fail Dispose()]");
  return false;
}

// -----------------------------------

bool
missionx::script_manager::init_bas()
{
  this->ext_bas_open = false;

  if (this->ext_sm_init_success)
  {
    if (mb_open(&this->bas) == MB_FUNC_OK)
    {
      this->ext_bas_open = true;

      mb_set_error_handler(bas, _on_error); // v3.0.96
      mb_rem_res_fun(bas, INPUT);           // v3.0.98 disable INPUT command

      // return true;
    }
  }

  if (this->ext_bas_open) // debug
    Log::logMsg("[External Interface Initialized]");
  else
    Log::logMsgErr("[External Interface failed Initialization]");

  return this->ext_bas_open;
}

bool
missionx::script_manager::reset_bas()
{

  if (this->ext_bas_open)
    if (mb_reset(&this->bas, false, true) == MB_FUNC_OK)
    {
      return true;
    }


  return false;
}

// -----------------------------------

bool
missionx::script_manager::parse_script(missionx::base_script* inBScript)
{
  bool result = false;

  if (inBScript != nullptr)
  {
    if (!this->ext_bas_open) // skip if BAS is not initialized
    {
      Log::logMsgErr("[EXT Error] bas interface is not open. Notify developer!!!");
      return false;
    }

    if (mb_load_string(this->bas, inBScript->script_body.c_str(), ';') == MB_FUNC_OK)
    {
      inBScript->script_is_valid = true;
      inBScript->err_desc.clear();
      result = true;
    }
    else
    {
      inBScript->script_is_valid = false;
      mb_error_e  err_no         = mb_get_last_error(this->bas, this->ppFile, &this->pos, &this->row, &this->col);
      std::string err_str(mb_get_error_desc(err_no));
      inBScript->err_desc = err_str;
      result              = false;
    }
  }

  this->reset_bas();
  return result;
}

// -----------------------------------
std::string
missionx::script_manager::run_script(const std::string scriptName, missionx::mxProperties& inPropertiesToSeed)
{
  std::string            result_s;
  missionx::base_script* basScript;

  if (!this->ext_bas_open) // skip if BAS interface is not initialized
  {
    result_s = "[EXT Error] bas interface is not open. Notify developer.";
    return result_s;
  }

  // get Script class
  auto itScript = mapScripts.find(scriptName);
  if (itScript == mapScripts.end())
  {

    result_s = "Could not find script file: " + scriptName; // v3.0.241.7.1
    return result_s;                                                // fail to find script
  }

  basScript = &mapScripts.find(scriptName)->second; // get basScript

  if (basScript->script_is_valid)
  {
#ifdef DEBUG_SCRIPT
    Log::logMsgNone(basScript->script_body); // debug
#endif

    //// prepare seeded data
    this->seedByProperty(inPropertiesToSeed); // dynamically  register all relevant seeded values

    if (mb_load_string(this->bas, basScript->script_body.c_str(), false) == MB_FUNC_OK)
    {
      basScript->script_is_valid = true;
      basScript->err_desc.clear();

#ifdef DEBUG_SCRIPT
      Log::logMsg("Calling script: " + QM + scriptName + QM);
#endif

      int scriptResult = mb_run(bas, false);

      if (scriptResult == MB_FUNC_OK)
      {
      } // end execute of script
      else
      { // script failed
        basScript->script_is_valid = false;
        mb_error_e  err_no         = mb_get_last_error(bas, this->ppFile, &this->pos, &this->row, &this->col);
        std::string err_str(mb_get_error_desc(err_no));
        basScript->err_desc = err_str;
        result_s            = "Ln " + mxUtils::formatNumber<int>(this->row) + ", Col " + mxUtils::formatNumber<int>(this->col) + ", File.func: <" + basScript->file_name + ">\n" + err_str; // Abort codes can be find in Log.txt file
                   
      }
    }
    else
    {
      basScript->script_is_valid = false;
      mb_error_e  err_no         = mb_get_last_error(bas, this->ppFile, &this->pos, &this->row, &this->col);
      std::string err_str(mb_get_error_desc(err_no));
      basScript->err_desc = err_str;
      result_s            = "Failed Loading Script: Ln " + mxUtils::formatNumber<int>(this->row) + ", Col " + mxUtils::formatNumber<int>(this->col) + ", File.func: <" + basScript->file_name + ">\n" + "Code " + mxUtils::formatNumber<int>(err_no) + "\n" + err_str;

    }
  }

  this->reset_bas();
  return result_s;
}

// -----------------------------------
void
missionx::script_manager::clear()
{
  script_manager::mapScripts.clear();                // static
  script_manager::mapScriptGlobalBoolArg.clear();    // static
  script_manager::mapScriptGlobalDecimalArg.clear(); // static
  script_manager::mapScriptGlobalStringsArg.clear(); // static
  script_manager::mapScriptGlobalNavRef.clear();     // static v3.0.194
  script_manager::mxPadTimer.reset();                // static v3.0.194
}

// -----------------------------------
bool
missionx::script_manager::getBoolValue(std::string& key, bool& outValue)
{
  bool found = false;

  auto it = script_manager::mapScriptGlobalBoolArg.find(key);
  if (it != script_manager::mapScriptGlobalBoolArg.end())
  {
    outValue = (bool)(*it).second;
    found    = true;
  }

  return found;
}

// -----------------------------------
bool
missionx::script_manager::getIntValue(std::string& key, int& outValue)
{
  bool found = false;

  auto it = script_manager::mapScriptGlobalDecimalArg.find(key);
  if (it != script_manager::mapScriptGlobalDecimalArg.end())
  {
    outValue = (int)(*it).second;
    found    = true;
  }

  return found;
}

// -----------------------------------

bool
missionx::script_manager::getDecimalValue(std::string& key, double& outValue)
{
  bool found = false;

  auto it = script_manager::mapScriptGlobalDecimalArg.find(key);
  if (it != script_manager::mapScriptGlobalDecimalArg.end())
  {
    outValue = (*it).second;
    found    = true;
  }

  return found;
}

// -----------------------------------

bool
missionx::script_manager::getStringValue(std::string& key, std::string& outValue)
{
  bool found = false;

  auto it = script_manager::mapScriptGlobalStringsArg.find(key);
  if (it != script_manager::mapScriptGlobalStringsArg.end())
  {
    outValue = (*it).second;
    found    = true;
  }

  return found;
}

float
missionx::script_manager::getPadTimer_xplaneSecondsPassed()
{
  return mxPadTimer.getSecondsPassed();
}

float
missionx::script_manager::getPadTimer_osTimePassed()
{
  return mxPadTimer.getOsSecondsPassed();
}

float
missionx::script_manager::getPadTimer_secondsToRun()
{
  return (float)mxPadTimer.getSecondsToRun();
}

void
missionx::script_manager::resetPadTimer()
{
  mxPadTimer.reset();
}

bool
missionx::script_manager::stopPadTimer(std::string& outErr)
{
  outErr.clear();

  if (mxPadTimer.getState() == mx_timer_state::timer_not_set)
  {
    outErr = "Timer never started. Nothing to stop.";
    return false;
  }

  mxPadTimer.stop();

  return true;
}

bool
missionx::script_manager::startMxPadTimer(float inSecondsToRun, std::string& outErr)
{
  outErr.clear();

  if (mxPadTimer.getState() == mx_timer_state::timer_running)
  {
    outErr = "[Warning] Timer was running, will reset it.";
  }

  mxPadTimer.reset();
  Timer::start(mxPadTimer, inSecondsToRun, "pad_timer");

  return true;
}

bool
missionx::script_manager::getPadTimer_wasTimePassed()
{
  return Timer::wasEnded(mxPadTimer);
}

// -----------------------------------

void
missionx::script_manager::saveCheckpoint(IXMLNode& inParent)
{

  IXMLNode xEmbedded = inParent.addChild(mxconst::get_ELEMENT_EMBEDDED_SCRIPTS().c_str());
  for (auto &[key, bScript] : mapScripts)
  {
    // base_script bScript = iter.second;
    bScript.saveCheckpoint(xEmbedded);
  }
  // Store global parameters
  for (const auto &[key, val] : missionx::script_manager::mapScriptGlobalStringsArg)
  {
    IXMLNode    xChild = xEmbedded.addChild(mxconst::get_ELEMENT_SCRIPT_GLOBAL_STRING_PARAMS().c_str());
    // std::string key    = iter.first;
    // auto        val    = iter.second;
    xChild.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), key.c_str());
    xChild.addClear(val.c_str());
  }
  for (const auto &[key,val] : missionx::script_manager::mapScriptGlobalDecimalArg)
  {
    IXMLNode    xChild = xEmbedded.addChild(mxconst::get_ELEMENT_SCRIPT_GLOBAL_NUMBER_PARAMS().c_str());
    // std::string key    = iter.first;
    // auto        val    = iter.second;
    xChild.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), key.c_str());
    xChild.addClear(Utils::formatNumber<double>(val).c_str());
  }

  for (const auto &[key, val] : missionx::script_manager::mapScriptGlobalBoolArg)
  {
    IXMLNode    xChild = xEmbedded.addChild(mxconst::get_ELEMENT_SCRIPT_GLOBAL_BOOL_PARAMS().c_str());
    // std::string key    = iter.first;
    // auto        val    = iter.second;
    xChild.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), key.c_str());
    xChild.addClear(Utils::formatNumber<bool>(val).c_str());
  }
}

// -----------------------------------

template<typename T>
void
parse_variadic(std::ostream& o, T t)
{
  o << t;
}

// -----------------------------------

int
missionx::script_manager::my_print(struct mb_interpreter_t* s, const char* fmt, ...)
{
  int         result = 0;
  std::string output;
  output.clear();

#ifndef RELEASE // v3.0.161 disable printing from MY-BASIC when compiling in RELEASE


  std::ostringstream oss;
  parse_variadic(oss, fmt);

  va_list args;
  va_start(args, fmt);

  //static char bufPrintFromScript_static[512]{'\0'};
  size_t len = sizeof(script_manager::bufPrintFromScript_static) - 1;

  va_list argptr;

  va_start(argptr, fmt);
  parse_variadic(oss, argptr);

  result = vsnprintf(script_manager::bufPrintFromScript_static, len, fmt, argptr);
  if (result < 0)
  {
    Log::logMsgErr( std::string(__func__) + ": Encoding error.\n");
  }

  va_end(argptr);
  if (result >= 0)
  {
    //Log::logXPLMDebugString(script_manager::bufPrintFromScript_static, false);

    if (script_manager::bufPrintFromScript_static[0] != '\n')
      script_manager::sPrintFromScript_static.append(script_manager::bufPrintFromScript_static); //  +" ";

    if (script_manager::bufPrintFromScript_static[0] == '\n')
    {
      Log::logMsg(script_manager::sPrintFromScript_static); // v3.305.2 write to default log which should end in the missionx.log and not Log.txt
      script_manager::sPrintFromScript_static.clear();
    }
  }

#endif

  return result;
}

// -----------------------------------

void
missionx::script_manager::seedStringParameter(mb_interpreter_t& bas, std::string& inNameVar, mb_value_t& mbValue, std::string inValue)
{
  char* cString = mb_memdup(inValue.c_str(), (unsigned)(inValue.length() + 1)); // `+1` means to allocate and copy one extra byte of the ending '\0'
  mb_make_string(mbValue, cString);
  mb_add_var(&bas, NULL, Utils::stringToUpper(inNameVar).c_str(), mbValue, true);

} // end template

// -----------------------------------

bool
missionx::script_manager::removeGlobalValue(std::string& key, mx_global_types inType)
{
  switch (inType)
  {
    case mx_global_types::bool_type:
      script_manager::mapScriptGlobalBoolArg.erase(key);
      break;
    case mx_global_types::number_type:
      script_manager::mapScriptGlobalDecimalArg.erase(key);
      break;
    case mx_global_types::string_type:
      script_manager::mapScriptGlobalStringsArg.erase(key);
      break;
    default:
      return false;
  }

  return true;
}

// -----------------------------------

void
missionx::script_manager::seedByProperty(mxProperties& prop)
{
  const size_t n = prop.mapProperties.size();

  if (n <= 0)
    return;

  std::vector<mb_value_t> vecMbValues;
  vecMbValues.clear();
  for (size_t i = 0; i < n; ++i)
  {
    mb_value_t mbVal; //
    vecMbValues.push_back(mbVal);
  }

  size_t counter = 0;
  for (auto &[key, p] : prop.mapProperties)
  {
    switch (p.type_enum)
    {
      case missionx::mx_property_type::MX_STRING:
      {
        std::string val = p.getValue();

        char* cString = mb_memdup(val.c_str(), (unsigned)(val.length() + 1)); // `+1` means to allocate and copy one extra byte of the ending '\0'

        mb_make_string(vecMbValues[counter], cString);
        if (this->bas)
        {
          mb_add_var(this->bas, NULL, Utils::stringToUpper(key).c_str(), vecMbValues[counter], true);
        }
      }
      break;
      case missionx::mx_property_type::MX_BOOL:
      {
        bool val = p.getValue<bool>();
        mb_make_bool(vecMbValues[counter], val);
        if (this->bas)
        {
          mb_add_var(this->bas, NULL, Utils::stringToUpper(key).c_str(), vecMbValues[counter], true);
        }
      }
      break;
      case missionx::mx_property_type::MX_INT:
      {
        int val = p.getValue<int>();
        mb_make_int(vecMbValues[counter], val);
        if (this->bas)
        {
          mb_add_var(this->bas, NULL, Utils::stringToUpper(key).c_str(), vecMbValues[counter], true);
        }
      }
      break;
      case missionx::mx_property_type::MX_FLOAT:
      case missionx::mx_property_type::MX_DOUBLE:
      {
        double val = p.getValue<double>();
        mb_make_real(vecMbValues[counter], val);
        if (this->bas)
        {
          mb_add_var(this->bas, NULL, Utils::stringToUpper(key).c_str(), vecMbValues[counter], true);
        }
      }
      break;
      default:
        break;
    } // end switch


    counter++;
  }
}

// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------
