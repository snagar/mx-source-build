#pragma once
#ifndef WRITELOGTHREAD_H_
#define WRITELOGTHREAD_H_

#include <queue>
#include <string>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <future>

#include "../core/MxUtils.h"
#include "../core/xx_mission_constants.hpp"

namespace missionx
{

class writeLogThread //: public base_thread
{
private:
  typedef struct _thread_state
  {
    // Chrono
    std::chrono::time_point<std::chrono::steady_clock> timer;
    std::string                                        duration_s;

    std::atomic<bool> flagIsActive;        // = false; // does thread() running/active
    std::atomic<bool> flagAbortThread;     // = false; // does the plugin exit, thus aborting thread actions
    std::atomic<bool> flagThreadDoneWork; // = false; // true/false. Set to true only when Class::run_thread job finish execution

    std::string                        dataString; // string data if needed for the thread, like path.
    std::map<std::string, std::string> mapValues;
    std::atomic<int>                   counter; // will be used for counting long loop operations like apt.dat optimizations

    std::atomic<mx_random_thread_wait_state_enum> thread_wait_state;

    void init()
    {
      flagIsActive        = false;
      flagAbortThread     = false;
      flagThreadDoneWork = false;
      dataString.clear();
      mapValues.clear();
      counter = 0;

      startThreadStopper();
    }

    void startThreadStopper() { timer = std::chrono::steady_clock::now(); }

    std::string getDuration()
    {
      std::chrono::time_point<std::chrono::steady_clock> end      = std::chrono::steady_clock::now();
      double                                             duration = (double)std::chrono::duration_cast<std::chrono::seconds>(end - timer).count();

      duration_s = mxUtils::formatNumber<double>((duration));
      return duration_s;
    }

  } thread_state;

  static std::thread thread_ref; // will hold pointer to the current running thread
  static std::vector<std::string> vecLogCycle; // = {"missionx.log, missionx.1.log, missionx.2.log, missionx.3.log"};
public:
  writeLogThread();
  virtual ~writeLogThread();

  static void flc();
  static void stop_plugin();

  static void add_message(const std::string& inMsg);

  static void clear_all_messages();

  static void set_logFilePath(const std::string& inFilePath, bool inCycleLogFiles);


  static void set_abortWrite(const bool inVal) { writeLogThread::tState.flagAbortThread = inVal; }

  static std::queue<std::string> qLogMessages_mainThread; // v3.0.221.4

  static std::string getLogFilePath() { return staticLogFilePath; } // v24.12.2

  static bool getThreadLogIsActive() {return writeLogThread::tState.flagIsActive; }
private:
  // parameters
  static long long seq;
  static thread_state tState;
  static std::string  staticLogFilePath;

  static char buff[21];

  // holds messages
  static std::queue<std::string> qLogMessages; // v3.0.217.8

  // members
  static void init();
  static void exec_thread(thread_state* xxthread_state); // generic name for: "do your staff"

  static std::vector<std::future<bool>> mWriteFuture; //  holds async pointers
  static std::mutex                     s_write_mutex;  
};

}
#endif // WRITELOGTHREAD_H_
