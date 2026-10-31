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
#include <filesystem>
#include <sstream>
#include <fstream>

#include "XPLMUtilities.h"

using namespace missionx;

namespace missionx
{
std::string             Log::logFilePath;
std::vector<std::string> Log::vecLogCycle; // v26.10.1
std::deque<std::string> Log::deq_LoadMissionFileErrors;
}



missionx::Log::Log() {}

missionx::Log::~Log()
{
  // TODO Auto-generated destructor stub
}

void
missionx::Log::add_missionLoadError(const std::string& inMsg)
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
missionx::Log::logMsgNone(const std::string &message)
{
  printToLog(message, format_type::none);
}

void
missionx::Log::logMsgErr(const std::string &message)
{
  printToLog(message, format_type::error);
}

void
missionx::Log::logMsgWarn(const std::string &message)
{
  printToLog(message, format_type::warning);
}

void
missionx::Log::logAttention(const std::string &message)
{
  printToLog(message, format_type::attention);
}

void
missionx::Log::logDebugBO(const std::string& message, bool print_to_xp_logfile) // log debug in Build Only mode
{
  #ifndef RELEASE
  if (print_to_xp_logfile)
    XPLMDebugString(fmt::format("{}\n",message).c_str());

  printToLog("[#debug] " + message);
  #endif
}


void
missionx::Log::log_xplm_debug_string(const std::string& message, const bool &b_add_plugin_name_as_prefix, const bool &b_force_write_to_xplane_log_file)
{
  // missionx::writeLogThread::add_message(message); // v3.305.2
  logMsg(message);

  const std::string out = (b_add_plugin_name_as_prefix)? "missionx: " + message : message;
  if (b_force_write_to_xplane_log_file)
    XPLMDebugString(out.c_str()); // not thread safe

}

void
missionx::Log::printHeaderToLog(const std::string &s, const format_type format)
{
  printToLog(s, format, true);
}

// Simple formatting for data in Log
void
missionx::Log::printToLog(std::string msg, const format_type format, const bool isHeader)
{
  loguru::Verbosity verb = loguru::Verbosity_INFO;

  if (isHeader)
  {
    if (format == format_type::header)
    {
      msg = std::string("\n##############################################################\n") + "=====> " + msg + "\n";
    }
    else if (format == format_type::footer)
    {
      msg = std::string("\n<===== ") + msg + "\n##############################################################\n";
    }
  } // end if header
  else
  {
    switch (format)
    {
      case (format_type::warning):
      {
        msg = "[Warning] " + msg;
        // logMode = Log::LOG_WARNING;
        verb = loguru::Verbosity_WARNING;

      }
      break;
      case (format_type::attention):
      {
        msg = "!!!!!! " + msg + " !!!!!!";
      }
      break;
      case (format_type::none_cr):
      { 
        // logMode = Log::LOG_NO_CR;
      } 
      break; 
      case (format_type::error):
      {
        msg = "[ERROR] " + msg;
        // logMode = Log::LOG_ERROR;
        verb = loguru::Verbosity_ERROR;
      }
      break;
      default:
        break;
    }
  }

  // v26.10.1
  loguru::log(verb, logFilePath.c_str(), 0, "%s", msg.c_str());
}



void
missionx::Log::stop_mission()
{
  Log::clear_deq_LoadMissionFileErrors(); // v3.0.241.1
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


std::string Log::get_formatted_timestamp_micros()
{
  using namespace std::chrono;

  const auto now = system_clock::now();
  const auto duration = now.time_since_epoch();

  const auto seconds = duration_cast<std::chrono::seconds>(duration);
  const auto microseconds = duration_cast<std::chrono::microseconds>(duration - seconds);

  std::time_t time_t_now = system_clock::to_time_t(now);
  std::tm tm_buf{};

  #if defined(_WIN32)
  localtime_s(&tm_buf, &time_t_now);
  #else
  localtime_r(&time_t_now, &tm_buf);
  #endif

  char timeBuffer[128];
  std::snprintf(timeBuffer, sizeof(timeBuffer), "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
                tm_buf.tm_year + 1900,
                tm_buf.tm_mon + 1,
                tm_buf.tm_mday,
                tm_buf.tm_hour,
                tm_buf.tm_min,
                tm_buf.tm_sec,
                static_cast<long>(microseconds.count()));

  return {timeBuffer};
}


void Log::set_logFilePath(const std::string& inFilePath, bool inCycleLogFiles)
{
  std::filesystem::path pathObj = inFilePath;
  std::filesystem::path base_folder = pathObj.parent_path();

  if (inCycleLogFiles)
  {
    int counter = 4;
    for (auto i = vecLogCycle.rbegin (); i != vecLogCycle.rend (); ++i)
    {
      std::filesystem::path logFile = base_folder.string () + "/" + (*i);
      if (counter == 4)
      {
        if (std::filesystem::exists (logFile))
          std::filesystem::remove (logFile);

        --counter;
        continue;
      }

      std::filesystem::path logFilePrev = base_folder.string () + "/" + (*--i);
      ++i; // one step back and one forward
      if (std::filesystem::exists (logFile))
      {
        std::filesystem::copy_file (logFile, logFilePrev, std::filesystem::copy_options::overwrite_existing);
      }

      --counter;

      if (counter <= 0)
        break;
    } // end cycle log files
  } // end !inDontCycleLogFile

  // Prepare the file
  std::ofstream ofs;
  ofs.open(logFilePath .c_str(), std::ofstream::trunc); // can we create/open the file ?
  if (ofs.fail())
  {
    assert(false && "Fail to open plugin log file");
  }
  ofs.close();
}


void Log::init_loguru(const std::string& plugin_folder_path)
{
  // 1. Create dummy argc and argv
  int argc = 1;
  char app_name[] = "missionx";
  char* argv[] = { app_name, nullptr };

  // 2. Configure options (disable command-line '-v' parsing)
  loguru::Options options;
  options.verbosity_flag = nullptr;

  // 3. Initialize Loguru
  loguru::init(argc, argv, options);

  // 4. Name the main X-Plane thread for thread-identified log lines
  loguru::set_thread_name("XP-MainThread");

  // 5. Add primary missionx.log file sink (Truncate or Append mode)
  const std::string& log_path = plugin_folder_path; // plugin_folder_path + "/missionx.log";
  loguru::add_file(log_path.c_str(), loguru::Append, loguru::Verbosity_INFO);

  // 6. Optional: Silence standard terminal output if you don't want stdout noise
  loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
}


void Log::set_log_file(const std::string& inLogFilePath, const bool inCycleLogFiles)
{
    logFilePath = inLogFilePath;
    // missionx::writeLogThread::set_logFilePath(inLogFilePath, inCycleLogFiles);
    set_logFilePath(logFilePath, inCycleLogFiles);
    init_loguru(logFilePath);
}
// Simple formatting of headers in Log

