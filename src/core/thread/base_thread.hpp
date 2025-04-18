#ifndef BASETHREADH_
#define BASETHREADH_

#pragma once

/******
The class represents thread, and control information on it


**/

#include <chrono>   // v3.0.219.12
#include <ctime>    // v3.0.219.12
#include <iostream> // std::cout
#include <map>      // std::cout
#include <thread>   // std::thread

//#include "../../data/mxProperties.hpp"
#include "../mx_base_node.h"
#include "../MxUtils.h"

namespace missionx
{

class base_thread
{
private:
  missionx::mxconst mx_const;
public:
  base_thread(){};
  virtual ~base_thread(){};

  typedef struct _thread_state
  {
    // Chrono
    std::chrono::time_point<std::chrono::steady_clock> timer;
    std::string                                        duration_s;

    std::atomic<bool> flagIsActive;        // = false; // does thread() running/active
    std::atomic<bool> flagAbortThread;     // = false; // does the plugin exit, thus aborting thread actions
    std::atomic<bool> flagThreadDoneWork; // = false; // true/false. Set to true only when Class::run_thread job finish execution

    std::atomic<int>                   counter;    // will be used for counting long loop operations like apt.dat optimizations
    std::string                        dataString; // string data if needed for the thread, like path.
    std::map<std::string, std::string> mapValues;
    
    missionx::mx_base_node pipeProperties; // v3.305.1

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
      pipeProperties.initBaseNode(); // v3.305.1
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

  std::thread thread_ref; // will hold pointer to the current running thread
};

}
#endif // BASETHREADH_
