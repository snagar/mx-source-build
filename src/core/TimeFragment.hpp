#ifndef TIME_H_
#define TIME_H_

#pragma once

/*******
TimeFragment.h class should only hold the current X-Plane time and no more once it was initialized.
All calculation on miliseconds and delta time, should be done in Timer Class.
*/

#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include <string>

#include "Utils.h"
#include "dataref_const.h"
#include <chrono>
#include <ctime>
#include <time.h>

// using namespace std;

namespace missionx
{
// define state of timer // v3.0.207.1 // deprecated since we use missionx::mx_timer_state
// typedef enum _time_status
//{
//  time_not_set,
//  time_set
//} mx_time_status;

/**
TimeFragment class stores current time information in X-Plane (a snapshot of the time) this is not clock information.
It stores:
1. total_running_time_sec_since_sim_start: how many seconds passed from sim start. Useful with draw() callbacks and 3D Object movement.
2. zuluTime_sec: seconds from zulu midnight
3. dayInYear: days from start of year.
4. os_clock: OS clock (store seconds not time)
5. os_clock_end: OS clock (store seconds not time)
*/
class TimeFragment
{
private:
  const missionx::dataref_const drefConst;
  //const float ZULU_TIME_OFFSET = 0.0f; // v3.0.303.7 how much current zulu can be smaller than previous zulu. This might be a bug in XP, since 2 consequitive calls to zulu_sec gave the first timer a higher zulu sec time than the next one which should never occur
  //    std::chrono::time_point<std::chrono::steady_clock> os_clock_end;

public:
  using nano_s  = std::chrono::nanoseconds;
  using micro_s = std::chrono::microseconds;
  using milli_s = std::chrono::milliseconds;
  using seconds = std::chrono::seconds;
  using minutes = std::chrono::minutes;
  using hours   = std::chrono::hours;

  // timer core attributes
  float total_running_time_sec_since_sim_start{ 0.0f }; // how many seconds passed from the start of the sim
  float zuluTime_sec{ 0.0f };                           // holds current iteration zulu seconds from midnight.
  int   dayInYear{ 0 };                              // store days from start of year.

  // Chrono
  std::chrono::time_point<std::chrono::steady_clock> os_clock;
  std::chrono::milliseconds                          deltaOsClock_milli = std::chrono::milliseconds(0);

  TimeFragment(){
    os_clock                               = std::chrono::steady_clock::now();
    dayInYear                              = XPLMGetDatai(drefConst.dref_local_date_days_i); // this is a problem since we pick zulu time but local day, and sometime they do not reflect correctly.
    zuluTime_sec                           = XPLMGetDataf(drefConst.dref_zulu_time_sec_f);
    total_running_time_sec_since_sim_start = XPLMGetDataf(drefConst.dref_total_running_time_sec_f); // store X-Plane running seconds from start of sim
  }

  TimeFragment(TimeFragment const&) = default; // this will explicitly add compiler generated copy ctor (https://stackoverflow.com/questions/51863588/warning-definition-of-implicit-copy-constructor-is-deprecated)

  void clone(TimeFragment& in_time_fragment)
  {
    this->total_running_time_sec_since_sim_start = in_time_fragment.total_running_time_sec_since_sim_start;
    this->zuluTime_sec                           = in_time_fragment.zuluTime_sec;
    this->dayInYear                              = in_time_fragment.dayInYear;

    this->os_clock = in_time_fragment.os_clock;
  }

  // Operators ///////////

  void operator=(TimeFragment& in_time_fragment) { clone(in_time_fragment); }


  std::string to_string() { 
    
    return std::string("[tf] total_running_time_sec_since_sim_start: " + mxUtils::formatNumber<float>(total_running_time_sec_since_sim_start, 8) + ", dayInYear: " + mxUtils::formatNumber<int>(dayInYear) + ", zuluTime_sec: " + mxUtils::formatNumber<float>(zuluTime_sec, 7));
    
  }


  // return (now - then) = (currentDayInYear-thenDayInYear)*(secondsInDay_86400) + nowSecondsFromMidnight - thenSecondsFromMidnight
  float operator-(TimeFragment& inTime)
  {
    // bug: the problem with custom time is that current day time might be different (newer, in the future) than the custom one. For example, mission day in year is "12" but current day in year is "13".
    //      We need a rule to use the "inTime.dayInYear" and only if our current "this->zuluTime_sec" is smaller than "inTime.zuluTime_sec" than we should pick (inTime.dayInYear + 1)
    //      TODO: need to decide when to use this->dayInYear

    if (this->dayInYear < inTime.dayInYear) // if current day in year is smaller than starting day in year than we need to add 365 to the current day in year. This is an edge case where we transitioned a year
    {
      this->dayInYear += 365;
    }

    float current_seconds = this->dayInYear * missionx::SECONDS_IN_1DAY + this->zuluTime_sec;
    float begin_seconds   = inTime.dayInYear * missionx::SECONDS_IN_1DAY + inTime.zuluTime_sec;

    const float delta_time = (current_seconds - begin_seconds < 0.0f) ? 0.0f : current_seconds - begin_seconds; // we won't return minus value, we will return fragment of a second just in case. Not sure this is a good idea.

    return delta_time;
  }


  // Currently return the delta time in milliseconds between two osClock (fragment time)
  static double getOsDurationBetween2TimeFragments(TimeFragment& t2, TimeFragment& t1)
  {
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2.os_clock - t1.os_clock).count();
    return duration / 1000.0; // milliseconds to seconds
  }

  static float getOsDurationBetweenNowAndStart(TimeFragment& inStartTimeFragment)
  {
    std::chrono::time_point<std::chrono::steady_clock> os_clock = std::chrono::steady_clock::now(); // fetch NOW
    const auto                                         duration = std::chrono::duration_cast<std::chrono::milliseconds>(os_clock - inStartTimeFragment.os_clock).count();
    return (float)duration / 1000; // milliseconds to seconds
  }

  // initialize class. There is no default contractor
  static void init(TimeFragment& outTime)
  {
    outTime.os_clock                               = std::chrono::steady_clock::now();
    outTime.dayInYear                              = XPLMGetDatai(outTime.drefConst.dref_local_date_days_i); // this is a problem since we pick zulu time but local day, and sometime they do not reflect correctly.
    outTime.zuluTime_sec                           = XPLMGetDataf(outTime.drefConst.dref_zulu_time_sec_f);
    outTime.total_running_time_sec_since_sim_start = XPLMGetDataf(outTime.drefConst.dref_total_running_time_sec_f); // store X-Plane running seconds from start of sim
    //#endif
  }



  float getTimePassedSec() { return zuluTime_sec; }
};

}

#endif
