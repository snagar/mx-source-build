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
#include <atomic> // v26.10.2

namespace missionx
{

// v26.10.2 the in-flight stats copy (tmp_stats.db), made by the writer thread right after one of its commits
enum class mx_stats_copy_state : uint8_t
{
  idle = 0,  // no copy asked
  requested, // the main thread asked for a copy. The next writer thread makes it
  copying,   // the writer thread is making it
  ready,     // the copy file is ready to read
  failed     // the copy failed: get_copy_error_cai()
};

class GatherStats : public base_thread
{
public:
  inline static bool   flag_ignore_stats_setup {false}; // v26.10.1

  struct mx_struct_stats_gather
  {
    // The following information will be initiated every flight loopback
    int    local_date_days{};
    int    onground_any_i{}, onground_all_i{}; // v3.303.8.3
    float  groundspeed{}, airspeed{}, heading_no_mag{}, heading_mag{}, pitch{}, roll{}, faxil_gear{}, brake_L{}, brake_R{};
    float  vh_ind_fpm{}, Qrad{}, Q{}; // v3.303.8.3  // v3.303.14 removedd y_agl since we use agl column
    float  vvi{} /*vertical velocity mt/s */, gforce_normal{}, gforce_axil{}, fnrml_total{}, AoA{} /*degrees*/;
    float  prev_total_running_time_sec{}, total_running_time_sec{};
    float  local_time_sec{}, vvi_fpm_pilot{}, flap_ratio{};
    float  fAGL{ 0.0 }; // v3.303.14
    float  m_total{ 0.0 }; // v3.303.14 current total weight = payload + fuel

    // v3.303.14 add cumulative distance
    float  fCumulativeDistance_nm{ 0.0f };

    double lat{}, lon{}, elev{}; // , local_x, local_y, local_z;

    std::string activity; // manly used when injecting information

    // The following information will be updated by Mission.class.
    std::string step_name;
    std::map<std::string, unsigned int> mapStepCounter; // key: step name, value: counter

    mx_struct_stats_gather() { init_(); }

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

    void clone(const mx_struct_stats_gather* inStats)
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

  };

private:
  const std::string INS_SQL = "ins_sql";

  const dataref_const dref_const;
  dbase* db; // points to Mission db.
  // Plane *plane; // we will use it to get Dataref and ongoing information
  // Step *step; // current objective
  // structMissionGeneralData *mission_data; // mission struct data
  std::string prepIns, threadIns; // prepIns: append insert command every 5sec. threadIns will get "prepIns" value while prepIns will be cleared and be ready for next string constructions. This will be done only if thread is not active.

  float fTotalSecBetweenLoopBackCall; // cumulative time between flb, once reach 5sec should build insert command string

  //void clearQueueContainer(); // v3.303.8.3 deprecated - not in use
  bool thread_should_write_info {false};                 // a flag that assist in preventing the thread to start in certain circumstances.
  bool exec_gather_thread(strct_thread_state* inout_thread_state_ptr, bool in_flag_allow_copy); //, Plane::mx_struct_stats_gather inStats); // each statement will finish with reset(). v26.10.2 in_flag_allow_copy: true only on the writer thread (the in-flight stats copy never runs on the main thread)

  // v26.10.2 the in-flight stats copy. copy_file_path and copy_error are written before copy_state changes, and read after it changed
  std::atomic<missionx::mx_stats_copy_state> copy_state{ missionx::mx_stats_copy_state::idle };
  std::atomic<bool>                          flag_abort_copy{ false }; // set by abort_copy_cai(): the writer drops the copy (or does not start it)
  std::string                                copy_file_path;
  std::string                                copy_error;
  void make_copy_cai(); // writer thread, after its commit: "VACUUM INTO" copy_file_path

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
  void clear_queue(); // v26.10.1 empty qInjectPlaneStats under the mutex
  void create_meta_tables_cai() const; // v26.10.2 flight_info and plane_meta tables ("if not exists")

public:
  GatherStats();
  ~GatherStats() override;

  void init();

  strct_thread_state xxthread_state;

  void flc();

  void setDb(dbase* inDb, const std::string &missionState_s);


  // stop plugin
  void stop_plugin();
  void stop_gather();
  // v26.10.1 stats database lifecycle
  void wait_for_writer();                    // join the running writer thread (no abort, its rows are written)
  void detach_db(bool in_flush_pending_rows); // stop writing and forget the connection. true: write the queued rows first
  void flush_pending_rows_cai();                  // v26.10.2 wait for the writer, then write the queued rows on this thread (main thread). The connection stays attached
  int  get_last_line_id_cai() const { return line_seq; } // v26.10.2 last stats.line_id written. Read it only when no writer runs (after wait_for_writer() or flush_pending_rows_cai())

  // v26.10.2 the in-flight stats copy (main thread). The writer thread makes the copy right after its next commit, so the live file is never read while it is written.
  bool                         request_copy_cai(const std::string &in_file_path); // false: a copy is already asked or being made
  missionx::mx_stats_copy_state get_copy_state_cai() const { return copy_state.load(); }
  std::string                  get_copy_error_cai() const { return (copy_state.load() == missionx::mx_stats_copy_state::failed) ? copy_error : std::string(); }
  void                         reset_copy_cai(); // back to "idle". Call it only when no writer runs, or when the state is "ready" or "failed"
  void                         abort_copy_cai(); // the mission is not running: drop the copy. Safe while the writer runs (a running copy is interrupted)

  void initTotalRunningTimeInSec();
  void gather_stats_data(); // v3.0.255.1

  void         gather_and_store_stats(const std::string &inActivity);
  void         set_activity_type(const std::string &inActivity) { this->stats.activity = inActivity; }
  void         store_stats();
  const dbase* get_db_ptr() { return this->db; }

  void reset()
  {
    this->prev_stats.init_();
    this->stats.init_();
    this->clear_queue(); // v26.10.1
  };

  void init_seq_from_checkpoint_stats(); // v3.303.8.3

  mx_struct_stats_gather get_stats_object() {return stats; }
  void set_cumulative_distance_nm_cai(const float in_distance_nm) { stats.fCumulativeDistance_nm = in_distance_nm; } // v26.10.2 checkpoint load: continue the distance from the saved leg row

  // Inject stats into Queue
  void addInjectedPlaneStat(mx_struct_stats_gather &inStat);

  void copyLastStatsToUiStruct(missionx::mx_enrout_stats_strct& inoutStats);


};

} /* namespace missionx */

#endif /* PUBLIC_THREAD_GATHERSTATS_H_ */
