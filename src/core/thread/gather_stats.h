/*
 * gather_stats.h
 *
 *  Created on: Apr 3, 2021
 *      Author: snagar
 */

#ifndef GATHER_STATS_H_
#define GATHER_STATS_H_

//#include "../data_manager.h"
#include "../../io/dbase.h"
#include "base_thread.hpp"

namespace missionx
{

class GatherStats : public base_thread
{
private:
  const std::string INS_SQL = "ins_sql";

  const dataref_const dref_const;

public:
  GatherStats();
  virtual ~GatherStats();

  void init();

  thread_state xxthread_state;

  void flc();

  void setDb(dbase* inDb, const std::string &missionState_s);


  // stop plugin
  void stop_plugin();
  void stop_gather();

  void initTotalRunningTimeInSec();
  void gather_stats_data(); // v3.0.255.1

  typedef struct _stats_gather_
  {
    // The following information will be initiated every flight loopback
    int    local_date_days;
    int    onground_any_i, onground_all_i; // v3.303.8.3
    float  groundspeed, airspeed, heading_no_mag, heading_mag, pitch, roll, faxil_gear, brake_L, brake_R;
    float  vh_ind_fpm, Qrad, Q; // v3.303.8.3  // v3.303.14 removedd y_agl since we use agl column
    float  vvi /*vertical velocity mt/s */, gforce_normal, gforce_axil, fnrml_total, AoA /*degrees*/;
    float  prev_total_running_time_sec, total_running_time_sec;
    float  local_time_sec, vvi_fpm_pilot, flap_ratio;
    float  fAGL{ 0.0 }; // v3.303.14
    float  m_total{ 0.0 }; // v3.303.14 current total weight = payload + fuel

    // v3.303.14 add cumulative distance
    float  fCumulativeDistance_nm{ 0.0f };

    double lat, lon, elev; // , local_x, local_y, local_z;

    std::string activity; // manly used when injecting information

    // The following information will be updated by Mission.class.
    std::string step_name;
    std::map<std::string, unsigned int> mapStepCounter; // key: step name, value: counter

    _stats_gather_() { init_(); }

    void init_()
    {
      lat = lon = elev /*= local_x = local_y = local_z*/ = 0.0;
      groundspeed = airspeed = heading_mag = heading_no_mag = pitch = roll = total_running_time_sec = prev_total_running_time_sec = faxil_gear = brake_L = brake_R = 0.0f;
      vvi = gforce_axil = gforce_normal = fnrml_total = AoA = 0.0f;
      local_time_sec                          = 0.0f;
      local_date_days                         = 0;
      vh_ind_fpm = Qrad = Q/* = y_agl*/ = 0.0f; // v3.303.8.3
      onground_all_i = onground_any_i = 0; // v3.303.8.3
      fCumulativeDistance_nm = 0.0f; // v3.303.14
      fAGL                            = 0.0; // v3.303.14
      m_total                         = 0.0; // v3.303.14
      step_name.clear();
      // seq = iStepSeqId = iStepSeqIdCounter = 0;

      activity.clear();

      mapStepCounter.clear();
    }

    void clone(_stats_gather_* inStats)
    {

      init_();

      local_date_days = inStats->local_date_days;
      local_time_sec  = inStats->local_time_sec;

      lat  = inStats->lat;
      lon  = inStats->lon;
      elev = inStats->elev;
      fAGL = inStats->fAGL; // v3.303.14
      m_total = inStats->m_total; // v3.303.14

      airspeed    = inStats->airspeed; // indicated airspeed
      groundspeed = inStats->groundspeed;
      vvi         = inStats->vvi;

      faxil_gear    = inStats->faxil_gear;
      brake_L       = inStats->brake_L;
      brake_R       = inStats->brake_R;
      gforce_normal = inStats->gforce_normal;
      gforce_axil   = inStats->gforce_axil;
      fnrml_total   = inStats->fnrml_total;

      AoA   = inStats->AoA; // angle of attack (alpha)
      pitch = inStats->pitch;
      roll  = inStats->roll;

      heading_no_mag = inStats->heading_no_mag;
      heading_mag    = inStats->heading_mag;

      step_name = inStats->step_name;
      mapStepCounter = inStats->mapStepCounter;
      activity       = inStats->activity;

      // v3.303.8.3
      vh_ind_fpm = inStats->vh_ind_fpm;
      Qrad        = inStats->Qrad;
      Q           = inStats->Q;
      //y_agl       = inStats->y_agl;      
      onground_all_i = inStats->onground_all_i;     
      onground_any_i = inStats->onground_any_i; 

      fCumulativeDistance_nm = inStats->fCumulativeDistance_nm; // v3.303.14

    }

  } mx_struct_stats_gather;

  void         gather_and_store_stats(const std::string &inActivity); 
  void         set_activity_type(const std::string &inActivity) { this->stats.activity = inActivity; }
  void         store_stats();
  const dbase* get_db_ptr() { return this->db; }

  void reset()
  {
    this->prev_stats.init_();
    this->stats.init_();
  };

  void init_seq_from_checkpoint_stats(); // v3.303.8.3

  mx_struct_stats_gather get_stats_object() {return stats; }

  // Inject stats into Queue
  void addInjectedPlaneStat(_stats_gather_ &inStat);

  void copyLastStatsToUiStruct(missionx::mx_enrout_stats_strct& inoutStats);

 private:
  dbase* db; // points to Mission db.
  // Plane *plane; // we will use it to get Dataref and ongoing information
  // Step *step; // current objective
  // structMissionGeneralData *mission_data; // mission struct data
  std::string prepIns, threadIns; // prepIns: append insert command every 5sec. threadIns will get "prepIns" value while prepIns will be cleared and be ready for next string constructions. This will be done only if thread is not active.

  float fTotalSecBetweenLoopBackCall; // cumulative time between flb, once reach 5sec should build insert command string

  //void clearQueueContainer(); // v3.303.8.3 deprecated - not in use
  bool thread_should_write_info {false};                 // a flag that assist in preventing the thread to start in certain circumstances.
  bool exec_gather_thread(thread_state* xxthread_state); //, Plane::mx_struct_stats_gather inStats); // each statement will finish with reset()

  /*******
   * line_seq & step_seq will allow us to group each step to its own set.
   * this is useful for returned steps, even though it is very rear. Example: When we will run stats per step, we will be able to distinguish between each step and returned steps.
   *
   */
  int line_seq{1}; // each mission_start, we reset this value to 1
  //int step_seq; // each step change we increase the value by 1


  mx_struct_stats_gather stats;
  mx_struct_stats_gather prev_stats;

  ///// Injecting Queue Container /////
  std::queue<mx_struct_stats_gather> qInjectPlaneStats;
};

} /* namespace missionx */

#endif /* PUBLIC_THREAD_GATHERSTATS_H_ */
