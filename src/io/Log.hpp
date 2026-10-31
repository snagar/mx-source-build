/*
 * Log.h
 *
 *  Created on: Feb 19, 2012
 *      Author: snagar
 */


#ifndef LOG_H_
#define LOG_H_

#include <queue>
#include <deque>
#include <string>
#include <vector>
#include <mutex>
#include <future>

// Loguru implementation
#define LOGURU_WITH_STREAMS 1       // Enables C++ stream style syntax LOG_S(INFO) << "..."
#define LOGURU_CATCH_SIGSEGV 0      // CRITICAL: Keeps X-Plane in control of signal handling
#include "../../libs/loguru/loguru.hpp"
// end loguru

#include "../core/MxUtils.h"
#include "../core/xx_mission_constants.hpp"

namespace missionx
{

typedef enum _format_type
  : uint8_t
{
  none, // no formatting
  script, // v3.305.2
  none_cr,
  header,
  footer,
  warning,
  attention,
  error
} format_type;

class Log
{
private:
  // v26.10.1 Formats current system time with microsecond resolution.
  static std::string get_formatted_timestamp_micros();
  static std::ofstream s_debugFile;
  static std::string             logFilePath;
  static std::vector<std::string> vecLogCycle; // = {"missionx.log, missionx.1.log, missionx.2.log, missionx.3.log"};
  static std::deque<std::string> deq_LoadMissionFileErrors; // v3.0.241.1

  // create the missionx.log file and cycle it if needed. Must be called before init_loguru.
  static void set_logFilePath(const std::string& inFilePath, bool inCycleLogFiles);
  // initialize the "loguru" library, receives the full path of the ;og file. Called after "set_logFilePath()" function.
  static void init_loguru(const std::string& plugin_folder_path);

public:
  Log();
  virtual ~Log();

  static std::deque<std::string> *get_deq_error_container_ptr () { return &deq_LoadMissionFileErrors; }

  static void        add_missionLoadError(const std::string& inMsg);                         // v3.0.241.1
  static void        clear_deq_LoadMissionFileErrors();                               // v3.0.241.1
  static std::string print_deq_LoadMissionFileErrors(bool inClearAfterPrint = false); // v3.0.241.1
  static size_t      get_deq_errors_container_size();                                 // v3.305.3

  // static writeLogThread writeThread; // v3.0.217.8


  //enum LOG_MODE : uint8_t
  //{
  //  LOG_NO_CR,
  //  LOG_INFO,
  //  // LOG_SCRIPT,
  //  LOG_WARNING,
  //  LOG_DEBUG,
  //  LOG_ERROR
  //};

  static void logMsgNone(const std::string &message);
  
  static void logMsgErr(const std::string &message);

  static void logMsgWarn(const std::string &message);

  static void logAttention(const std::string &message); // v3.0.219.10

  static void logDebugBO(const std::string& message, bool print_to_xp_logfile = false); // v3.0.221.15 rc4 rares - Log in debug build only

  static void log_xplm_debug_string(const std::string& message, const bool &b_add_plugin_name_as_prefix = true, const bool &b_force_write_to_xplane_log_file = true); // v3.0.221.9 // v3.0.301 B3 added  "bool bDecoration"

  // Simple formatting of headers in Log
  static void printHeaderToLog(const std::string &s, format_type format = format_type::header);

  // Simple formatting for data in Log
  static void printToLog(std::string msg, format_type format = format_type::none, bool isHeader = false);

    // static void logMsg(const std::string& message, const int log_mode, const bool isThread = false) { logToFile(message, log_mode); }
  static void logMsg(const std::string& message, format_type format = format_type::none) { printToLog(message, format); }

  static void flc() {};

  static void stop_mission();

  static void set_log_file(const std::string& inLogFilePath, bool inCycleLogFiles); // v3.0.217.8 // v25.03.1 added inCycleLogFiles

  static std::string get_time_as_string_with_ms();

};


} // end namespace

#endif /* LOG_H_ */
