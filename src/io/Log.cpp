/*
 * Log.cpp
 *
 *  Created on: Feb 19, 2012
 *      Author: snagar
 */

/**************

Updated: 24-nov-2012

Done: Nothing to change. New in NET. Not exists in 2.05


ToDo:


**************/
#include "Log.hpp"
#include <cassert>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>
//#include <thread>


using namespace missionx;

namespace missionx
{
long long               Log::log_seq {0};
std::string             Log::logFilePath;
writeLogThread          Log::writeThread; // v3.0.217.8
std::deque<std::string> Log::deq_LoadMissionFileErrors;
}



missionx::Log::Log() {}

missionx::Log::~Log()
{
  // TODO Auto-generated destructor stub
}

void
missionx::Log::add_missionLoadError(std::string inMsg)
{
  if (!inMsg.empty())
    Log::deq_LoadMissionFileErrors.push_back(inMsg);
}

void
missionx::Log::clear_deq_LoadMissionFileErrors()
{
  Log::deq_LoadMissionFileErrors.clear();
}

std::string
missionx::Log::print_deq_LoadMissionFileErrors(bool inClearAfterPrint)
{
  std::string formatedString;

  for (const auto &s : Log::deq_LoadMissionFileErrors)
  {
    formatedString += s + "\n";
  }

  if (inClearAfterPrint)
    Log::clear_deq_LoadMissionFileErrors();

  return formatedString;
}

size_t
missionx::Log::get_deq_errors_container_size()
{
  return Log::deq_LoadMissionFileErrors.size();
}

void
missionx::Log::logMsgThread(std::string message, missionx::format_type format) // v3.0.223.6 added the thread option
{
  printToLog(message, true, format);
}

/*  Snagar */
void
missionx::Log::logMsg(std::string message, bool isThread, missionx::format_type format, bool isHeader)
{
  printToLog(message, isThread, format, isHeader);
}

void
missionx::Log::logMsgNone(std::string message, bool isThread)
{
  printToLog(message, isThread, format_type::none);
}

void
missionx::Log::logMsgExtScript(std::string message, bool isThread)
{
  printToLog(message, isThread, format_type::script);
}

void
missionx::Log::logMsgNoneCR(std::string message, bool isThread)
{
  printToLog(message, isThread, format_type::none_cr);
}

void
missionx::Log::logMsgErr(std::string message, bool isThread)
{
  printToLog(message, isThread, format_type::error);
}

void
missionx::Log::logMsgWarn(std::string message, bool isThread)
{
  printToLog(message, isThread, format_type::warning);
}

void
missionx::Log::logAttention(std::string message, bool isThread)
{
  printToLog(message, isThread, format_type::attention);
}

void
missionx::Log::logDebugBO(std::string message, bool isThread) // log debug in Build Only mode
{
#ifndef RELEASE
  printToLog("[#debug] " + message, isThread);
#endif
}


void
missionx::Log::logXPLMDebugString(std::string message, bool bDecoration)
{
  Log::writeThread.add_message(message); // v3.305.2

  const std::string out = (bDecoration)? "missionx: " + message : message;
  XPLMDebugString(out.c_str()); // not thread safe
}

void
missionx::Log::logTXT(std::string message, bool bDecoration)
{
  const std::string out = (bDecoration)? "missionx: " + message : message;
  XPLMDebugString(out.c_str()); // not thread safe
}



// Simple formatting of headers in Log
void
missionx::Log::printHeaderToLog(std::string s, bool isThread, format_type format)
{
  printToLog(s, isThread, format, true);
}

// Simple formatting for data in Log
void
missionx::Log::printToLog(std::string s, bool isThread, format_type format, bool isHeader)
{
  Log::LOGMODE logMode = Log::LOG_INFO;

  if (isHeader)
  {
    if (format == format_type::header)
    {
      s = std::string("\n##############################################################\n") + "=====> " + s + "\n";
    }
    else if (format == format_type::footer)
    {
      s = std::string("\n<===== ") + s + "\n##############################################################\n";
    }
  } // end if header
  else
  {
    switch (format)
    {
      case (format_type::warning):
      {
        s = "[Warning] " + s;
      }
      break;
      case (format_type::attention):
      {
        s = "!!!!!! " + s + " !!!!!!";
      }
      break;
      case (format_type::none_cr):
      { 
        logMode = Log::LOG_NO_CR; 
      } 
      break; 
      case (format_type::script):
      {
        logMode = Log::LOG_SCRIPT;
      }
      break;
      case (format_type::error):
      {
        s = "[ERROR] " + s;
      }
      break;
      default:
        break;
    }
  }

  Log::logToFile(s, logMode, isThread); // none
}


void
missionx::Log::logToFile(std::string msg, int mode, bool isThread)
{
//#ifdef TIMER_FUNC
//  auto m_startTimepoint = std::chrono::high_resolution_clock::now();
//#endif // TIMER_FUNC


  std::string errStr;
  std::string out = "";

  switch (mode)
  {
    case LOG_INFO:
    {
      out = msg + "\n";
    }
    break;
    case LOG_NO_CR:
    {
      out = msg;
    }
    break;

    case LOG_SCRIPT:
    {
      out = "[ex script]" +  msg; // v3.305.2
    }
    break;
    case LOG_ERROR:
    {
      out = "[ERROR]: " + msg + "\n";
    }
    break;
    case LOG_DEBUG:
    {
      out = "[DEBUG]: " + msg + "\n";
    }
    break;
    default:
      break;
  }

  // v24.12.2 try again to differ between thread and immediate mode
#ifndef RELEASE
  if (isThread)
    return missionx::writeLogThread::add_message(out);

  missionx::Log::log_to_missionx_log(out);
#else
  missionx::writeLogThread::add_message(out);
#endif

}

void
missionx::Log::stop_mission()
{
  missionx::writeLogThread::stop_plugin();

  Log::clear_deq_LoadMissionFileErrors(); // v3.0.241.1
}


// void
// Log::log_to_logtxt(const std::string& msg)
// {
// #ifndef RELEASE
//   XPLMDebugString(msg.c_str());
// #endif
// }


void
Log::log_to_missionx_log(const std::string& msg)
{
#ifndef RELEASE
  const std::string log_file = missionx::writeLogThread::getLogFilePath(); // debug

  if (missionx::writeLogThread::getLogFilePath().empty())
    return;

  std::ofstream ofs;
  ofs.open(missionx::writeLogThread::getLogFilePath().c_str(), std::ofstream::app); // can we create/open the file ?
  if (ofs.fail())
  {
    assert(false && fmt::format("[{}] Fail to open plugin log file: '{}'", __func__, missionx::writeLogThread::getLogFilePath().c_str()).c_str());
  }

  ofs << "[" << ++Log::log_seq << "][" << Log::get_time_as_string_with_ms() << "] " << msg;

  if (ofs.bad())
  {
    missionx::writeLogThread::qLogMessages_mainThread.push(std::string("Fail to write to log file: ") + missionx::writeLogThread::getLogFilePath());
  }

  if (ofs.is_open())
    ofs.close();


#else
  // in release mode, we only use the thread route.
  missionx::writeLogThread::add_message(msg);
#endif

}


std::string
Log::get_time_as_string_with_ms()
{
  // https://stackoverflow.com/questions/16357999/current-date-and-time-as-string
  // Get the current time point
  const auto now = std::chrono::system_clock::now();
  // Convert to time_t to extract calendar time
  const auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::tm local_tm = *std::localtime(&time_t_now);
  // Extract milliseconds
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::ostringstream oss;
  oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(4) << ms.count();

  return oss.str();
}
