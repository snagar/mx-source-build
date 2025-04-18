#ifndef __BASE_SCRIPT_H
#define __BASE_SCRIPT_H
#pragma once
//#include "../../data/mxProperties.hpp"
#include <string>
#include <chrono>

#include "../Utils.h"
#include "mx_colors.hpp"


/***
This class holds information of loaded BAS script
script name: Name of file. (unique)
script body: Content of file as string.
err_desc: Holds any error message as debug info.
script_is_valid: boolean that flags if script is valid to use. It will be skipped if not.
fStartExec & fEndExec: time in miliseconds it took the script to execute.
fTimeExec: fEndExec - fStartExec

**/

namespace missionx
{
class base_script
{
private:

public:
  //constexpr const static long long BASE_DURATION_LL = 10;
  using seconds = std::chrono::seconds;


  bool  flagWasCalled; // v3.305.3 debug screen
  bool  isScriptlet;
  bool  script_is_valid;

  double dSecondsFromLastCall;

  //std::string scriptlet_body;
  std::string script_body;        // include + scriptlet_body
  std::string script_name_no_ext; // holds script file name without the extension (until first dot)
  std::string file_name;          // holds original script file name
  std::string err_desc;
  std::string sExecutionTime; // v3.305.3 debug screen
  std::string sEndExecutionTime; // v3.305.3 debug screen

  std::chrono::time_point<std::chrono::steady_clock> last_run_os_clock;
  std::chrono::seconds                               deltaOsClock_seconds = std::chrono::seconds(0);


  ImVec4 color = missionx::color::color_vec4_white; //

  // just like include in C/C++. We evaluate this only after loading ALL script files. Missing include files will flag the script as invalid.
  std::vector<std::string> vecIncludeScriptName;
  long long timePassed = 0; // v3.305.3

  //

  base_script()
  {
    script_body.clear();
    file_name.clear();
    script_name_no_ext.clear();
    err_desc.clear();
    script_is_valid = true;
    isScriptlet     = false;

    dSecondsFromLastCall = 0.0;

    flagWasCalled = false;
    sExecutionTime.clear();
    sEndExecutionTime.clear();

    last_run_os_clock = std::chrono::steady_clock::now();
  }

  void set_script_body(std::string inScriptBody)
  {
    script_body.clear();
    this->script_body = inScriptBody;
  }

  // Function will split the filename and place it in "file_name" and "script_name_no_ext" variables. Delimeter is always "."
  void set_script_file_name(std::string inFileName) { this->script_name_no_ext = Utils::extractBaseFromString(inFileName); }

  
  //////////////////////////////////////////////////////////////////////

  void setWasCalled (const bool inVal)
  {
    if (inVal)
    {
      last_run_os_clock = std::chrono::steady_clock::now();
      this->timePassed  = 0; // reset timer
    }

    this->flagWasCalled = inVal;
    missionx::color::func::flcDebugColors(this->flagWasCalled, this->timePassed, this->color, this->last_run_os_clock);
    //this->flcDebugColors();
  }

// ----------------------

  void flcDebugColors()
  {
    missionx::color::func::flcDebugColors(this->flagWasCalled, this->timePassed, this->color, this->last_run_os_clock);

    //auto       os_clock = std::chrono::steady_clock::now();
    //// calc duration only if we did not cross the BASE_DURATION_LL * 3 timer. This will allow Four color phases
    //const auto duration = (this->timePassed <= missionx::color::BASE_DURATION_LL * 3) ? std::chrono::duration_cast<std::chrono::seconds>(os_clock - this->last_run_os_clock).count() : missionx::color::BASE_DURATION_LL * 10;

    //if (this->flagWasCalled) // there is no use in calculating 
    //{
    //  this->timePassed = duration;
    //  color            = missionx::color::func::get_color_based_on_dimnish_through_time(duration, color);
    //   
    //  return;
    //}
    //
    //color = missionx::color::color_vec4_white; // no executed

  }

// ----------------------

  std::string to_string_debug() {

    return file_name + " (" + ((script_is_valid) ? "valid" : "invalid") + ":" + ((flagWasCalled) ? "Executed: " + sExecutionTime + " >> " + sEndExecutionTime : "Not Exec") + ")";
  }

// ----------------------

  std::string get_execution_timestamps_debug() {

    return sExecutionTime + " >> " + sEndExecutionTime;
  }

// ----------------------

  // checkpoint
  void storeCoreAttribAsProperties()
  {
    //name = this->file_name; // property name // v3.305.3 deprecated
  }

// ----------------------

  void saveCheckpoint(IXMLNode& inParent)
  {
    storeCoreAttribAsProperties();

    if (this->isScriptlet)
    {
      IXMLNode xFile = inParent.addChild(mxconst::get_ELEMENT_SCRIPTLET().c_str());
      xFile.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), file_name.c_str());

      // get first include file
      if (!this->vecIncludeScriptName.empty())
      {
        const auto i = this->vecIncludeScriptName.front();
        xFile.addAttribute(mxconst::get_ELEMENT_INCLUDE_FILE().c_str(), i.c_str());
      }

      //xFile.addClear(this->scriptlet_body.c_str()); // v3.305.3 disabled
      xFile.addClear(this->script_body.c_str());
    }
    else // not scriptlet
    {
      IXMLNode xFile = inParent.addChild(mxconst::get_ELEMENT_FILE().c_str());
      xFile.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), file_name.c_str());
      for (const auto &incName : vecIncludeScriptName)
      {
        IXMLNode xIncludeFile = xFile.addChild(mxconst::get_ELEMENT_INCLUDE_FILE().c_str());
        xIncludeFile.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), incName.c_str());
      }
    }
  }

// ----------------------

};

}

#endif // !__BASE_SCRIPT_H
