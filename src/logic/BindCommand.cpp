#include "BindCommand.h"



BindCommand::BindCommand()
{
  this->flag_isActive   = false;
  this->flag_wasClicked = false;

  this->flag_justClicked          = false;
  this->cool_time_between_press_f = 1.0f;
  this->time_when_pressed.reset();
  this->ref          = NULL;
  this->phase        = BindCommand::mxstatus_command::cmd_not_pressed;
  this->secToExecute = 0.0f;
}

// ---------------------------------------

BindCommand::~BindCommand()
{
}

// ---------------------------------------

bool
missionx::BindCommand::setAndListenToCommand(std::string inCommandPath)
{
  if (inCommandPath.empty())
    return false;

  this->name = inCommandPath;

  this->ref = XPLMFindCommand(inCommandPath.c_str());
  if (this->ref == NULL)
  {
#if defined DEBUG || defined _DEBUG
    Log::logMsg("[bindCommand set command] Fail to find command: " + inCommandPath);
#endif
    return false;
  }


  // listen
  XPLMRegisterCommandHandler(this->ref, missionx::BindCommand::commandHandler, 1, reinterpret_cast<void*>(this));

  return true;
}
// setAndListenToCommand


// ---------------------------------------

void
missionx::BindCommand::executeCommand()
{
  if (this->secToExecute > 0.0f)
  {
    this->phase = mxstatus_command::cmd_manual_begin;
    XPLMCommandBegin(this->ref);
    Timer::start(this->time_when_pressed, this->secToExecute);
  }
  else
  {
    XPLMCommandOnce(this->ref);
    Timer::start(this->time_when_pressed, this->cool_time_between_press_f);
  }
  this->flag_justClicked = true;
  this->flag_wasClicked  = true;
}

// ---------------------------------------

int
missionx::BindCommand::commandListener(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
  if (inPhase == xplm_CommandBegin && this->flag_isActive) // // Handle original command pressed state
  {
    if (flag_justClicked && !Timer::wasEnded(time_when_pressed, true)) // if already clicked and still in cooldown state
      return 1;

    flag_justClicked = true;
    flag_wasClicked  = true;

    Timer::start(time_when_pressed, cool_time_between_press_f);
  }
  return 1;
}

// ---------------------------------------

void
missionx::BindCommand::flc()
{
  if (this->secToExecute > 0.0f)
  {
    switch (this->phase)
    {
    case mxstatus_command::cmd_not_pressed:
      {
        phase = mxstatus_command::cmd_manual_begin;
        missionx::Timer::start(this->timer, this->secToExecute);
        XPLMCommandBegin(this->ref);
#ifndef RELEASE
        Log::logMsg("[BindCommand] Begin command: " + getName());
#endif
      }
      break;
      case mxstatus_command::cmd_manual_begin:
      {
#ifndef RELEASE
        Log::logMsg("[BindCommand] Test command: " + getName() + ", timer: " + Utils::formatNumber<float>(this->timer.getSecondsPassed(), 4));
#endif
        if (missionx::Timer::wasEnded(this->timer))
        {
          this->phase = mxstatus_command::cmd_end;
          XPLMCommandEnd(this->ref);
#ifndef RELEASE
          Log::logMsg("[BindCommand] Command: " + getName() + " ENDED, timer: " + Utils::formatNumber<float>(this->timer.getSecondsPassed(), 4));
#endif
        }
      }
      break;
      default:
      {
        // End command and mark as ended so we should erase from timer
        this->phase = mxstatus_command::cmd_end;
        XPLMCommandEnd(this->ref);
      }
    } // end switch

  }
  else if (this->flag_justClicked && this->phase != mxstatus_command::cmd_end)
  {
    if (Timer::wasEnded(this->time_when_pressed, true))
    {
      this->flag_justClicked = false;
      this->time_when_pressed.reset();
    }
  }
}

// ---------------------------------------
// ---------------------------------------
