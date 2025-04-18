#ifndef BINDCOMMAND_H_
#define BINDCOMMAND_H_
#pragma once

#include "../core/Timer.hpp"
#include "../core/Utils.h"
//#include "../data/mxProperties.hpp"
//#include "../data/mxProperties.hpp"

namespace missionx
{
class BindCommand
{
private:
  
public:
  BindCommand();
  virtual ~BindCommand();

  // we will use the setName() and getName() to store the "command path"
  typedef enum class _status_command
    : uint8_t
  {
    cmd_not_pressed  = 0,
    cmd_manual_begin = 1,
    cmd_continue     = 2,
    cmd_end          = 4
  } mxstatus_command;
  mxstatus_command phase;

  XPLMCommandRef ref;
  float          secToExecute;

  bool flag_isActive;
  bool flag_justClicked;
  bool flag_wasClicked;

  // v3.305.1
  std::string name{ "" };
  std::string getName() { return name; }

  // timer
  missionx::Timer time_when_pressed;
  missionx::Timer timer; // for long pressed commands
  float           cool_time_between_press_f;


  static int commandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
  {
    BindCommand* mxCommand_ptr = reinterpret_cast<BindCommand*>(inRefcon);

    if (mxCommand_ptr)
      return mxCommand_ptr->commandListener(inCommand, inPhase, NULL);

    return 1;
  }

  bool setAndListenToCommand(std::string inCommandPath);
  void executeCommand();
  int  commandListener(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);


  // members
  void flc();
};
}

#endif
