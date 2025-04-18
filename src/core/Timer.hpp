#ifndef TIMER_H_
#define TIMER_H_

#pragma once

#include "../io/IXMLParser.h"
#include "TimeFragment.hpp"
#include "Utils.h"

using namespace missionx;

namespace missionx
{
typedef enum class _timer_state
  : uint8_t
{
  timer_not_set = 0,
  timer_is_set, // v3.0.253.7
  timer_running,
  timer_paused,
  timer_stop,
  timer_ended
} mx_timer_state;

class Timer
{
private:
  ///// v3 /////
  missionx::TimeFragment tf_begin;
  missionx::TimeFragment tf_now;
  missionx::TimeFragment tf_last;                  // v3.0.207.1
  float                  cumulativeXplaneTime_sec; // v3.0.221.11 will hold the secondsPassed and the cumulative seconds between timer stop/start. If we pause the timer then on second start we add the cumulative to the "secondsPassed"
  float                  secondsPassed;
  float                  deltaSecondsBetweenFragments; // v3.0.207.1
  float                  osSecondsPassed;              // store osClock delta
  double                 cumulativeOsTime_sec;         // v3.0.221.11

  float secondsToRun; // how much timer should run before stopping

  mx_timer_state timer_state;

  bool flag_runContinuously;  // if we send "secondsToRun=0" then continuasly run
  bool flag_isCumulative;    // v3.0.221.11 stop won't reset the elapsed time and we need to test against the cumulativeXXX variables instead on the seconds
  bool flag_isBasedSysClock; // v3.0.223.1

  std::string name; // for debug purposes

public:
  IXMLNode node{ IXMLNode().emptyIXMLNode };
  bool     flag_loadedFromSavePoint{ false };

  Timer() { reset(); }

  Timer(Timer const&) = default;
//  Timer(Timer&&) = default;
//  Timer& operator=(const Timer&) = default;
//  Timer& operator=(Timer&&) = default;

  // ----------------------------------------------------
  std::string getName() { return this->name; }
  // ----------------------------------------------------

  bool parse_node()
  {
    if (this->node.isEmpty())
      return false;
    else
    {
      this->reset();

      this->name                     = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");
      this->secondsToRun             = Utils::readNodeNumericAttrib<float>(this->node, mxconst::get_ATTRIB_TIME_MIN(), 0.0f) * missionx::SECONDS_IN_1MINUTE; // convert to minutes
      this->flag_isCumulative        = true;
      this->flag_runContinuously      = false;
      this->flag_isBasedSysClock     = false;
      this->flag_loadedFromSavePoint = false;

      if (this->name.empty())
      {
        Log::logMsg("One of the timers has no [name] attribute was defined for timer. will ignore it");
        return false;
      }
      else if (this->secondsToRun <= 0.0f)
      {
        Log::logMsg("Timer: " + this->name + ", has wrong minutes [min] attribute defined. It is set to 0 or less. Please fix it. Will ignore.");
        return false;
      }

      // check if we have begin fragment information
      float zuluTime_sec                           = Utils::readNodeNumericAttrib<float>(this->node, mxconst::get_ATTRIB_ZULU_TIME_SEC(), -1.0f);                          // begin fragment info
      float total_running_time_sec_since_sim_start = Utils::readNodeNumericAttrib<float>(this->node, mxconst::get_ATTRIB_TOTAL_RUNNING_TIME_SEC_SINCE_SIM_START(), -1.0f); // begin fragment info
      int   dayInYear                              = Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_ATTRIB_DAY_IN_YEAR(), -1);                                 // begin fragment info
      float secondsPassed                          = Utils::readNodeNumericAttrib<float>(this->node, mxconst::get_ATTRIB_SECONDS_PASSED(), -1.0f);                         // timer info
      if (zuluTime_sec >= 0.0f && total_running_time_sec_since_sim_start > 0.0f && dayInYear >= 0 && secondsPassed >= 0.0f)
      {
        this->secondsPassed = secondsPassed;

        this->tf_begin.zuluTime_sec                           = zuluTime_sec;
        this->tf_begin.total_running_time_sec_since_sim_start = total_running_time_sec_since_sim_start;
        this->tf_begin.dayInYear                              = dayInYear;

        const std::chrono::seconds sec{ (int)secondsPassed };
        this->tf_begin.os_clock = std::chrono::steady_clock::now() - sec; // set the begin OS clock relative to the seconds already passed

        flag_loadedFromSavePoint = true;
      }

      if (flag_loadedFromSavePoint)
        this->timer_state = (mx_timer_state)Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_ATTRIB_TIMER_STATE(), (int)mx_timer_state::timer_is_set);
      else 
        this->timer_state = mx_timer_state::timer_is_set; // (missionx::mx_timer_state) Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_ATTRIB_TIMER_STATE(), (int)mx_timer_state::timer_is_set);

      this->tf_last     = this->tf_begin;               // v3.0.303.7 make sure that begin and start timers are the same when parsing

      return true;
    }

    return false;
  }

  // ----------------------------------------------------
  void start_timer_based_on_node()
  {

    missionx::TimeFragment::init(this->tf_begin); // store now
    this->tf_last                      = this->tf_begin;
    this->tf_now                       = this->tf_begin;
    this->deltaSecondsBetweenFragments = 0.0f; // should be ZERO
    this->cumulativeOsTime_sec         = 0.0; // v3.0.303.7
    this->cumulativeXplaneTime_sec     = 0.0; // v3.0.303.7
    this->timer_state                  = missionx::mx_timer_state::timer_running;
  }

  // ----------------------------------------------------
  void start_timer_after_savepoint_load()
  {

    // missionx::TimeFragment::init(this->tf_begin); // store now
    this->tf_last                      = this->tf_begin;
    this->tf_now                       = this->tf_begin;
    this->deltaSecondsBetweenFragments = 0.0f; // should be ZERO
    this->timer_state                  = missionx::mx_timer_state::timer_running;
  }

  // ----------------------------------------------------

  void reset()
  {
    secondsToRun  = 0.0f;
    secondsPassed = osSecondsPassed = 0.0f;
    deltaSecondsBetweenFragments    = 0.0f; // v3.0.207.1
    flag_runContinuously             = false;
    timer_state                     = mx_timer_state::timer_not_set;
    name.clear();

    cumulativeOsTime_sec = cumulativeXplaneTime_sec = 0.0f; // v3.0.221.11
  }


  // ----------------------------------------------------
  void storeCoreAttribAsProperties()
  {
    if (!this->node.isEmpty())
    {
      Utils::xml_set_attribute_in_node_asString(this->node, mxconst::get_ATTRIB_NAME(), this->getName(), mxconst::get_ELEMENT_TIMER()); // v3.305.3

      // begin fragment info
      Utils::xml_set_attribute_in_node<float>(this->node, mxconst::get_ATTRIB_ZULU_TIME_SEC(), this->tf_begin.zuluTime_sec, mxconst::get_ELEMENT_TIMER());
      Utils::xml_set_attribute_in_node<float>(this->node, mxconst::get_ATTRIB_TOTAL_RUNNING_TIME_SEC_SINCE_SIM_START(), this->tf_begin.total_running_time_sec_since_sim_start, mxconst::get_ELEMENT_TIMER());
      Utils::xml_set_attribute_in_node<int>(this->node, mxconst::get_ATTRIB_DAY_IN_YEAR(), this->tf_begin.dayInYear, mxconst::get_ELEMENT_TIMER());
      // timer info
      Utils::xml_set_attribute_in_node<float>(this->node, mxconst::get_ATTRIB_TIME_MIN(), (this->secondsToRun / missionx::SECONDS_IN_1MINUTE), mxconst::get_ELEMENT_TIMER()); // v3.305.3
      Utils::xml_set_attribute_in_node<bool>(this->node, mxconst::get_ATTRIB_RUN_CONTINUOUSLY(), this->flag_runContinuously, mxconst::get_ELEMENT_TIMER());
      Utils::xml_set_attribute_in_node<float>(this->node, mxconst::get_ATTRIB_SECONDS_PASSED(), this->secondsPassed, mxconst::get_ELEMENT_TIMER());
      Utils::xml_set_attribute_in_node<int>(this->node, mxconst::get_ATTRIB_TIMER_STATE(), (int)this->timer_state, mxconst::get_ELEMENT_TIMER());
    }
  }
  // ----------------------------------------------------
  void saveCheckpoint(IXMLNode& inParent)
  {
    storeCoreAttribAsProperties();

    inParent.addChild(this->node);
  }
  // ----------------------------------------------------

  void clone(Timer& inTimer)
  {
    reset();
    this->node                         = inTimer.node.deepCopy(); // v3.0.253.7
    this->tf_begin                     = inTimer.tf_begin;
    this->tf_now                       = inTimer.tf_now;
    this->tf_last                      = inTimer.tf_last;              // v3.0.207.1
    this->deltaSecondsBetweenFragments = this->tf_now - this->tf_last; // v3.0.207.1

    this->cumulativeXplaneTime_sec = inTimer.cumulativeXplaneTime_sec; // v3.0.221.11
    this->cumulativeOsTime_sec     = inTimer.cumulativeOsTime_sec;     // v3.0.221.11

    this->secondsPassed   = inTimer.secondsPassed;
    this->osSecondsPassed = inTimer.osSecondsPassed;
    this->secondsToRun    = inTimer.secondsToRun;

    this->timer_state         = inTimer.timer_state;
    this->flag_runContinuously = inTimer.flag_runContinuously;
    this->name                = inTimer.name;
  }

  // ----------------------------------------------------

  void operator=(Timer& inTimer) { this->clone(inTimer); }

  
  
  static std::string get_debugTimestamp()
  {
    using namespace std::chrono;
    using clock = system_clock;
    std::ostringstream stream;

    const auto current_time_point{ clock::now() };
    const auto current_time{ clock::to_time_t(current_time_point) };
    const auto current_localtime{ *std::localtime(&current_time) };
    const auto current_time_since_epoch{ current_time_point.time_since_epoch() };
    const auto current_milliseconds{ duration_cast<milliseconds>(current_time_since_epoch).count() % 1000 };

    
    stream << std::put_time(&current_localtime, "%T") << "." << std::setw(3) << std::setfill('0') << current_milliseconds;
    return stream.str();    
  }

  // ----------------------------------------------------

  static std::string get_current_time_and_date(std::string date_format = "%Y-%m-%d %H:%M:%S") //  "%Y-%m-%d %X"
  {
    // https://stackoverflow.com/questions/17223096/outputting-date-and-time-in-c-using-stdchrono
    // https://howardhinnant.github.io/date/date.html

    auto now           = std::chrono::system_clock::now();
    auto in_raw_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    // ss << std::put_time(std::localtime(&in_time_t), date_format.c_str());
#if defined LIN || defined MAC
    ss << std::put_time(std::localtime(&in_raw_time_t), date_format.c_str());
#else
    struct std::tm timeinfo;
    localtime_s(&timeinfo, &in_raw_time_t);
    ss << std::put_time(&timeinfo, date_format.c_str());
#endif

    return ss.str();
  }

  // ----------------------------------------------------

  void stop()
  {
    // TODO: need to store seconds run until now
    timer_state = mx_timer_state::timer_stop;
  }

  // ----------------------------------------------------

  void pause()
  {
    // TODO: need to store seconds run until now
    timer_state = mx_timer_state::timer_paused;
  }

  // ----------------------------------------------------
  float getCumulativeXplaneTimeInSec() { return this->cumulativeXplaneTime_sec; }

  // ----------------------------------------------------
  double getCumulativeOSTimeInSec() { return this->cumulativeOsTime_sec; }


  // ----------------------------------------------------


  bool getIsCumulative() { return this->flag_isCumulative; }

  // ----------------------------------------------------

  void setCumulative_flag(bool inVal) { this->flag_isCumulative = inVal; }

  // ----------------------------------------------------

  void setEnd() { timer_state = mx_timer_state::timer_ended; }

  // ----------------------------------------------------
  
  // v3.305.3
  void setSecondsPassedForInterpolationInit( const float & inSecondsPassed) { 
    this->secondsPassed = inSecondsPassed;
  }

  // ----------------------------------------------------
  void resume()
  {
    // TODO: need to find the delta timer was in pause. we should use cumulativeTime and add it to the "secondsPassed"
    timer_state = mx_timer_state::timer_running;
  }

  // ----------------------------------------------------
  float getSecondsToRun() { return secondsToRun; }

  float getSecondsPassed(bool bCalcOsDelta = false)
  {
    if (timer_state == mx_timer_state::timer_running)
    {
      Timer::wasEnded(*this, bCalcOsDelta); // check now - begin
      // return this->secondsPassed;
    }

    if (bCalcOsDelta) // v3.306.1 added os seconds passed from beginning, since it was missing if "bCalcOsDelta" is true.
      return this->getOsSecondsPassed();

    return this->secondsPassed; // X-Plane seconds passwd from beginning.
  }

  float getRemainingTime(bool bCalcOsDelta = false)
  {
    if (bCalcOsDelta)
      return this->secondsToRun - this->osSecondsPassed;

    return this->secondsToRun - this->secondsPassed;
  }


  // ----------------------------------------------------
  float getSecondsPassed_for_TotalXP() { return this->secondsPassed; }
  // ----------------------------------------------------



  float getOsSecondsPassed() { return osSecondsPassed; }

  // ----------------------------------------------------

  bool isRunning() { return (timer_state == mx_timer_state::timer_running) ? true : false; }

  // ----------------------------------------------------

  mx_timer_state getState() { return timer_state; }

  // ----------------------------------------------------

  static void start(Timer& inTimer, float inSecondsToRun = 0.0f, std::string inName = "Timer", bool isCumulative = false)
  {
    inTimer.name              = inName;       // for debug
    inTimer.flag_isCumulative = isCumulative; // v3.0.221.11

    if (inSecondsToRun == 0.0f)
      inTimer.flag_runContinuously = true;
    else
      inTimer.secondsToRun = inSecondsToRun;

    missionx::TimeFragment::init(inTimer.tf_begin); // store now
    inTimer.tf_last                      = inTimer.tf_begin;
    inTimer.tf_now                       = inTimer.tf_begin;
    inTimer.deltaSecondsBetweenFragments = 0.0f; // v3.0.207.1 // should be ZERO
    inTimer.timer_state                  = mx_timer_state::timer_running;

    if (!inTimer.node.isEmpty()) // v3.0.253.7 this code will be effective only when we will initialize the "node" with a valid element object.
    {
      Utils::xml_set_attribute_in_node_asString(inTimer.node, mxconst::get_ATTRIB_NAME(), inTimer.name, inTimer.node.getName());
      Utils::xml_set_attribute_in_node<float>(inTimer.node, mxconst::get_ATTRIB_TIME_MIN(), inTimer.secondsToRun / missionx::SECONDS_IN_1MINUTE, inTimer.node.getName());
    }
  }

  // ----------------------------------------------------
  static void unpause(Timer& inTimer)
  {
    if (inTimer.timer_state == mx_timer_state::timer_paused)
    {
      inTimer.timer_state = mx_timer_state::timer_running;
      missionx::TimeFragment::init(inTimer.tf_last);
      // inTimer.tf_last = inTimer.tf_now;
    }
  }

  // ----------------------------------------------------
  // v24.03.1
  // Progress the clock and calculates delta.
  // Reset clock once it ends
  // Do not use this function to evaluate if clock is in stop state. It is either "not set" or "running".
  static bool evalTimeAndResetOnEnd(missionx::Timer& inTimer, bool checkOsTime = false ) // v3.303.13
  {
    bool bResult = Timer::wasEnded(inTimer, checkOsTime);
    if (bResult)
      inTimer.reset();

    return bResult;
  }

  // ----------------------------------------------------
  // Progress the clock and calculates delta.
  // return true if "secondsPass" > "secondsToRun"
  // return false if "secondsToRun" was not passed or "flag_runContinuously" is true.
  static bool evalTime(missionx::Timer& inTimer, bool checkOsTime = false ) // v3.303.13
  {
    return Timer::wasEnded(inTimer, checkOsTime);
  }

  static bool wasEnded(missionx::Timer& inTimer, bool checkOsTime = false /* v3.0.159*/)
  {
    if (inTimer.timer_state == mx_timer_state::timer_running)
    {
      missionx::TimeFragment::init(inTimer.tf_now);

      if (inTimer.tf_now.dayInYear == inTimer.tf_begin.dayInYear && inTimer.tf_now.zuluTime_sec < inTimer.tf_begin.zuluTime_sec)
      {
        inTimer.tf_begin.zuluTime_sec = inTimer.tf_now.zuluTime_sec - 0.3f; // we reset begin timer to be same as current minus fragment of a second 
        inTimer.tf_last               = inTimer.tf_begin;
      }


      // check seconds passed
      inTimer.secondsPassed                = inTimer.tf_now - inTimer.tf_begin;
      inTimer.deltaSecondsBetweenFragments = inTimer.tf_now - inTimer.tf_last;  // v3.0.207.1
      inTimer.cumulativeXplaneTime_sec += inTimer.deltaSecondsBetweenFragments; // v3.0.221.11
      inTimer.osSecondsPassed = Timer::getOsDurationPassed(inTimer);
      inTimer.cumulativeOsTime_sec += TimeFragment::getOsDurationBetween2TimeFragments(inTimer.tf_now, inTimer.tf_last); // v3.0.221.11
      inTimer.tf_last = inTimer.tf_now;                                                                                  // v3.0.221.11 store current time fragment for cumulative test


//#ifndef RELEASE
//      Log::logMsg("---------------");
//      Log::logMsg(Timer::to_string(inTimer));
//      Log::logMsg("[now] " + inTimer.tf_now.to_string());
//      Log::logMsg("[begin] " + inTimer.tf_begin.to_string());
//      Log::logMsg("---------------");
//#endif // !RELEASE



      if (inTimer.flag_runContinuously)
        return false;

      if (inTimer.flag_isCumulative) // v3.0.221.11
      {
        if ((checkOsTime) ? (inTimer.cumulativeOsTime_sec >= inTimer.secondsToRun) : (inTimer.cumulativeXplaneTime_sec >= inTimer.secondsToRun)) // check based OS time or xplane time (when pause won't progress)
        {
          inTimer.timer_state = missionx::mx_timer_state::timer_ended;
          return true; // time passed, stop timer
        }
      }
      else if ((checkOsTime) ? (inTimer.osSecondsPassed >= inTimer.secondsToRun) : (inTimer.secondsPassed >= inTimer.secondsToRun)) // check based OS time or xplane time (when pause won't progress)
      {
        inTimer.timer_state = missionx::mx_timer_state::timer_ended;
        return true; // time passed, stop timer
      }
    }
    else if (inTimer.timer_state == mx_timer_state::timer_ended )
      return true;

    return false; // if timer state is not "timer_ended" should return false. "timer_stop" does prove that timer reached its target.
  }


  // ----------------------------------------------------
  // test against total_running_time_sec.
  // This function must be used carefully since it does not support the OS time but only X-Plane time. We can use it for moving 3D objects
  static bool wasXplaneTimerEnded(missionx::Timer& inTimer)
  {
    if (inTimer.timer_state == mx_timer_state::timer_running)
    {
      missionx::TimeFragment::init(inTimer.tf_now);
      // check seconds passed
      inTimer.secondsPassed                = inTimer.tf_now.total_running_time_sec_since_sim_start - inTimer.tf_begin.total_running_time_sec_since_sim_start;
      inTimer.deltaSecondsBetweenFragments = inTimer.tf_now.total_running_time_sec_since_sim_start - inTimer.tf_last.total_running_time_sec_since_sim_start;
      inTimer.cumulativeXplaneTime_sec += inTimer.deltaSecondsBetweenFragments;
      inTimer.tf_last = inTimer.tf_now; // store time for next iteration calculation

      if (inTimer.flag_runContinuously)
        return false;

      if (inTimer.flag_isCumulative) // v3.0.221.11
      {
        if (inTimer.cumulativeXplaneTime_sec >= inTimer.secondsToRun) // check based OS time or xplane time (when pause won't progress)
        {
          inTimer.timer_state = missionx::mx_timer_state::timer_ended;
          return true; // time passed, stop timer
        }
      }
      else if (inTimer.secondsPassed >= inTimer.secondsToRun) // check based OS time or xplane time (when pause won't progress)
      {
        inTimer.timer_state = missionx::mx_timer_state::timer_ended;
        return true; // time passed, stop timer
      }
    }
    else if (inTimer.timer_state == mx_timer_state::timer_ended)
      return true;

    return false; // if timer state is not "timer_ended" should return false. "timer_stop" does proove that timer reached its target.
  }

  // ----------------------------------------------------
  static float getDeltaBetween2TimeFragments(Timer& inTimer) // v3.0.207.1
  {
    return inTimer.deltaSecondsBetweenFragments;
  }
  // ----------------------------------------------------

  static float getOsDurationPassed(Timer& inTimer)
  {
    // std::chrono::time_point<std::chrono::steady_clock> os_clock = std::chrono::steady_clock::now(); // fetch NOW
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(inTimer.tf_now.os_clock - inTimer.tf_begin.os_clock).count();
    return (float)duration / 1000; // milliseconds to seconds
  }

  // ----------------------------------------------------

  static std::string translateTimerState(mx_timer_state& inState)
  {
    switch (inState)
    {
      case mx_timer_state::timer_running:
        return "Running";
        break;
      case mx_timer_state::timer_paused:
        return "Paused";
        break;
      case mx_timer_state::timer_stop:
        return "Stopped";
        break;
      case mx_timer_state::timer_ended:
        return "Ended";
        break;
      default:
        break;

    } // end switch

    return "Not Set";

    // end translate
  }


  // ----------------------------------------------------

  static std::string to_string(Timer& inTimer)
  {
    std::string format = "Timer Name: \"" + inTimer.name + "\". State: " + Timer::translateTimerState(inTimer.timer_state) + ". Run Continuasly: " + ((inTimer.flag_runContinuously) ? "Yes" : "No") + ". ";
    format += ((inTimer.flag_runContinuously) ? "Seconds to run: " + Utils::formatNumber<float>(inTimer.secondsToRun) + ", " : EMPTY_STRING) + "Seconds Passed: " + Utils::formatNumber<float>(inTimer.secondsPassed) +
              ". Milliseconds Passed: " + Utils::formatNumber<float>(inTimer.osSecondsPassed) + ", Fragment delta: " + Utils::formatNumber<float>(missionx::Timer::getDeltaBetween2TimeFragments(inTimer)) + "\n";

    return format;
  }

  // ----------------------------------------------------
  
  std::string get_formated_timer_as_text(const std::string prefix_to_foramted_string)
  {
    if (!this->isRunning())
      return "";


    auto remaining_time = this->getRemainingTime();
    auto days           = (int)(remaining_time / missionx::SECONDS_IN_1DAY);
    auto hours          = (int)(remaining_time / missionx::SECONDS_IN_1HOUR_3600);
    auto minutes        = (int)((remaining_time - (hours * missionx::SECONDS_IN_1HOUR_3600)) / missionx::SECONDS_IN_1MINUTE);
    auto seconds        = (int)(remaining_time - (hours * missionx::SECONDS_IN_1HOUR_3600 + minutes * missionx::SECONDS_IN_1MINUTE));

    if (days > 0)
      return prefix_to_foramted_string + mxUtils::formatNumber<int>(days) + ":" + mxUtils::formatNumber<int>(hours) + ":" + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(d:h:m:s)";

    if (hours > 0)
      return prefix_to_foramted_string + mxUtils::formatNumber<int>(hours) + ":" + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(h:m:s)";

    if (minutes > 0)
      return prefix_to_foramted_string + mxUtils::formatNumber<int>(minutes) + ":" + mxUtils::formatNumber<int>(seconds) + "(m:s)";

    return prefix_to_foramted_string + mxUtils::formatNumber<int>(seconds) + "(s)"; // seconds
  }

  // ----------------------------------------------------
};

} // namespace missionx

#endif // TIMER_H_
