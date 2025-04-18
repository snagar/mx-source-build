/*
 * Log.h
 *
 *  Created on: Feb 19, 2012
 *      Author: snagar
 */


#ifndef LOG_H_
#define LOG_H_

#include "../core/base_xp_include.h"
#include <deque>
#include <forward_list>
#include <string>
#include "writeLogThread.h"

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
  //sub_element_lvl_1,
  //sub_element_lvl_2,
  warning,
  attention,
  error
} format_type;

class Log
{
private:
static long long log_seq; // v24.12.2
  static std::string             logFilePath;
  static std::deque<std::string> deq_LoadMissionFileErrors; // v3.0.241.1
public:
  Log();
  virtual ~Log();

  static void        add_missionLoadError(std::string inMsg);                         // v3.0.241.1
  static void        clear_deq_LoadMissionFileErrors();                               // v3.0.241.1
  static std::string print_deq_LoadMissionFileErrors(bool inClearAfterPrint = false); // v3.0.241.1
  static size_t      get_deq_errors_container_size();                                 // v3.305.3

  static writeLogThread writeThread; // v3.0.217.8


  enum LOGMODE : uint8_t
  {
    LOG_NO_CR,
    LOG_INFO,
    LOG_SCRIPT,
    LOG_DEBUG,
    LOG_ERROR
  };

  /*  Snagar */
  static void logMsgThread(std::string message, missionx::format_type format = format_type::none);

  static void logMsg(std::string message, bool isThread = false, missionx::format_type format = format_type::none, bool isHeader = false);

  static void logMsgNone(std::string message, bool isThread = false);
  
  static void logMsgExtScript(std::string message, bool isThread = false);

  static void logMsgNoneCR(std::string message, bool isThread = false);

  static void logMsgErr(std::string message, bool isThread = false);

  static void logMsgWarn(std::string message, bool isThread = false);

  static void logAttention(std::string message, bool isThread = false); // v3.0.219.10

  static void logDebugBO(std::string message, bool isThread = false); // v3.0.221.15 rc4 rares - Log in debug build only

  static void logXPLMDebugString(std::string message, bool bDecoration = true); // v3.0.221.9 // v3.0.301 B3 added  "bool bDecoration"
  static void logTXT(std::string message, bool bDecoration = true); // v3.305.3 write directly to Log.txt and not to missionx.log. Helps when Que is not ready yet.

  static void set_logFile(const std::string& inLogFilePath, const bool inCycleLogFiles) // v3.0.217.8 // v25.03.1 added inCycleLogFiles
  {
    logFilePath = inLogFilePath;
    missionx::writeLogThread::set_logFilePath(inLogFilePath, inCycleLogFiles);
  }


  // Simple formatting of headers in Log
  static void printHeaderToLog(std::string s, bool isThread = false, format_type format = format_type::header);

  // Simple formatting for data in Log
  static void printToLog(std::string s, bool isThread = false, format_type format = format_type::none, bool isHeader = false);


  static void logToFile(std::string msg, int mode, bool isThread = false);



  static void logMsg(std::string message, int log_mode, bool isThread = false) { logToFile(message, log_mode, isThread); }

  static void flc()
  {
    Log::writeThread.flc(); // manage the thread state
  }


  static void stop_mission();

  // static void log_to_logtxt(const std::string &msg); // v24.12.2 always immediate write to Log.txt
  static void log_to_missionx_log(const std::string &msg); // v24.12.2 always immediate write to Log.txt

  static std::string get_time_as_string_with_ms();

};


} // end namespace

#endif /* LOG_H_ */
