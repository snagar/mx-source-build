#include "gather_stats.h"
#include "../dataref_manager.h"

namespace missionx
{
static std::mutex mutexGatherStats; // global

GatherStats::GatherStats()
{
  init();
}

GatherStats::~GatherStats()
{
  this->stop_gather();
}

void
GatherStats::init()
{
  xxthread_state.init();
  this->line_seq = 1;
  this->db       = nullptr;
  this->prepIns.clear();
  this->threadIns.clear();
  this->fTotalSecBetweenLoopBackCall = 0.0f;
}

// ----------------------------------------------------
void
GatherStats::flc()
{
//#ifdef TIMER_FUNC
//  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
//#endif // TIMER_FUNC

  this->gather_stats_data();

  // check and reset thread state
  if (xxthread_state.flagIsActive && xxthread_state.flagThreadDoneWork)
    xxthread_state.init();

  // add delta passed time to fTotalSecBetweenLoopBackCall.
  fTotalSecBetweenLoopBackCall += this->stats.total_running_time_sec - this->stats.prev_total_running_time_sec;
  {
    // we should not call the thread if it is active
    if (!xxthread_state.flagIsActive)
    {
      // place stats inside the queue.
      std::lock_guard<std::mutex> lock(missionx::mutexGatherStats);
      this->addInjectedPlaneStat(this->stats); // we standardize data storage inside a queue container. This way our GATHER_STATS_INTERVAL_SEC stats will be injected with the Draw Loopback calls.

      // Call Thread to flush data into SQLite
      if (!xxthread_state.flagIsActive && this->db != nullptr )
      {
        xxthread_state.init();
        if (thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
          thread_ref.join();       // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"

        thread_ref = std::thread(&GatherStats::exec_gather_thread, this, &xxthread_state);
      }

      fTotalSecBetweenLoopBackCall = 0.0f;
    } // end activate thread
  }   // end if GATHER_STATS_INTERVAL_SEC
}

// ----------------------------------------------------

void
GatherStats::setDb(dbase* inDb, const std::string &missionState_s)
{
  db = inDb;
  if (db != nullptr)
  {
    // clear stats DB
    std::string sqlStmt = "DROP TABLE if exists stats ";

    if (missionState_s.empty())
    {


      db->execute_stmt(sqlStmt);

      sqlStmt = "CREATE TABLE if not exists stats "
                "( line_id INTEGER, vvi_fpm_pilot FLOAT, flap_ratio FLOAT, local_date_days INT, local_time_sec FLOAT, lat DOUBLE, "
                " lon DOUBLE, elev DOUBLE, agl FLOAT, airspeed FLOAT, groundspeed FLOAT, vh_ind FLOAT, faxil_gear FLOAT,"
                " brakes_L FLOAT, brakes_R FLOAT,"
                " gforce_normal FLOAT, gforce_axil FLOAT, fnrml_total_nw FLOAT, AoA FLOAT, pitch FLOAT, roll FLOAT, heading_mag FLOAT, "
                " heading_no_mag FLOAT, activity TEXT "
                " ,onground_any INT, onground_all INT ,vh_ind_fpm FLOAT, Qrad FLOAT, Q FLOAT, m_total FLOAT )"; // v3.303.14 added m_total v3.303.8.3 added onground_any, onground_all, vvi_ind_fpm_f, Qrad_f, Q_f, y_agl_f

      if (!db->execute_stmt(sqlStmt))
      {
        Log::logMsgErr("[Stats] Failed to create 'stats' table: " + db->last_err);
      }

      // v3.303.8.1 add stats_summary view that show min_max
      sqlStmt = "DROP VIEW if exists stats_summary ";
      db->execute_stmt(sqlStmt);

      sqlStmt = R"(
CREATE VIEW stats_summary AS
	SELECT min(t1.vvi_fpm_pilot) AS min_vvi_fpm_pilot,
		   max(t1.vvi_fpm_pilot) AS max_vvi_fpm_pilot,
		   min(t1.airspeed) AS min_airspeed,
		   max(t1.airspeed) AS max_airspeed,
		   min(t1.groundspeed) AS min_groundspeed,
		   max(t1.groundspeed) AS max_groundspeed,
		   min(t1.vh_ind) AS min_vh_ind,
		   max(t1.vh_ind) AS max_vh_ind,
		   min(t1.gforce_normal) AS min_gforce_normal,
		   max(t1.gforce_normal) AS max_gforce_normal,
		   min(t1.gforce_axil) AS min_gforce_axil,
		   max(t1.gforce_axil) AS max_gforce_axil,
		   min(t1.aoa) AS min_aoa,
		   max(t1.aoa) AS max_aoa,
		   min(t1.pitch) AS min_pitch,
		   max(t1.pitch) AS max_pitch,
		   min(t1.roll) AS min_roll,
		   max(t1.roll) AS max_roll
	  FROM stats t1
    		)";
      if (!db->execute_stmt(sqlStmt))
      {
        Log::logMsgErr("[Stats] Failed to create 'stats' table: " + db->last_err);
      }
    } // end if missionState_s is empty - rebuild objects

    // v3.303.8.3 This prepared statement always need to be executed.
        
    // Create the "insert" statement to the stats table
    sqlStmt = "insert into stats "
              "(line_id, vvi_fpm_pilot, flap_ratio, local_date_days, local_time_sec, lat, " // 6
              "lon, elev, agl, airspeed, groundspeed, vh_ind, faxil_gear, "                 // 7
              "brakes_L, brakes_R,"                                                         // 2
              "gforce_normal, gforce_axil, fnrml_total_nw, AoA, pitch, roll, heading_mag, "  // 7
              "heading_no_mag, activity, "                                                  // 2
              "onground_any, onground_all, vh_ind_fpm, Qrad, Q, m_total  ) "                // 7
              " values (?, ?, ?, ?, ?, ?,  ?, ?, ?, ?, ?, ?, ?,  ?, ?,  ?, ?, ?, ?, ?, ?, ?,  ?, ?,  ?, ?, ?, ?, ?, ?);"; // total 30 cols

    if (!db->prepareNewStatement(INS_SQL, sqlStmt))
    {
      Log::logMsgErr("[Stats] Failed to create 'insert' statement: " + db->last_err);
    }


  } // end if db is not nullptr
}


// ----------------------------------------------------
void
GatherStats::stop_plugin()
{
  this->stop_gather();
}

// ----------------------------------------------------
void
GatherStats::stop_gather()
{
  if (this->xxthread_state.flagIsActive)
    this->xxthread_state.flagAbortThread = true;

  if (this->thread_ref.joinable())
    this->thread_ref.join();
}

// ----------------------------------------------------
void
GatherStats::initTotalRunningTimeInSec()
{
  this->stats.total_running_time_sec      = missionx::dataref_manager::getTotalRunningTimeSec(); // XPLMGetDataf(dref_const.dref_total_running_time_sec_f);  //this->getTotalRunningTimeSec();
  this->stats.prev_total_running_time_sec = this->stats.total_running_time_sec;
}


// ----------------------------------------------------
void
GatherStats::gather_stats_data()
{
//#ifdef TIMER_FUNC
//  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
//#endif // TIMER_FUNC

  this->prev_stats = this->stats;// v3.303.14 store previous stats

  this->stats.local_date_days = missionx::dataref_manager::getLocalDateDays(); //   XPLMGetDatai(dref_const.dref_local_date_days_i); //  this->getLocalDateDays();
  this->stats.local_time_sec  = missionx::dataref_manager::getLocalTimeSec();  //  XPLMGetDataf(dref_const.dref_local_time_sec_f);

  this->stats.lat  = missionx::dataref_manager::getLat();
  this->stats.lon  = missionx::dataref_manager::getLong();
  this->stats.elev = missionx::dataref_manager::getElevation();
  this->stats.fAGL = missionx::dataref_manager::getAGL(); // // v3.303.14

  this->stats.airspeed      = missionx::dataref_manager::getAirspeed();      //   XPLMGetDataf(dref_const.indicated_airspeed_f);  // missionx::dataref_manager::getAi
  this->stats.groundspeed   = missionx::dataref_manager::getGroundSpeed();   // XPLMGetDataf(dref_const.dref_groundspeed_f); //
  this->stats.vvi           = missionx::dataref_manager::get_vh_ind();       // XPLMGetDataf(dref_const.dref_vh_ind_f); // vertical velocity mt/s
  this->stats.vvi_fpm_pilot = XPLMGetDataf(dref_const.dref_vvi_fpm_pilot_f); //  FPM - can assist in determeans landing
  this->stats.flap_ratio    = XPLMGetDataf(dref_const.dref_flap_ratio_f);    //  Flap Ratio

  this->stats.faxil_gear = missionx::dataref_manager::getFaxilGear();     // XPLMGetDataf(dref_const.dref_faxil_gear_f); //
  this->stats.brake_L    = missionx::dataref_manager::getBrakeLeftAdd();  // XPLMGetDataf(dref_const.dref_brake_Left_add_f); //
  this->stats.brake_R    = missionx::dataref_manager::getBrakeRightAdd(); // XPLMGetDataf(dref_const.dref_brake_Right_add_f); //


  this->stats.gforce_normal = missionx::dataref_manager::get_g_normal();      // XPLMGetDataf(dref_const.dref_gforce_normal_f);
  this->stats.gforce_axil   = missionx::dataref_manager::get_gforce_axil();   // XPLMGetDataf(dref_const.dref_gforce_axil_f); //
  this->stats.fnrml_total   = missionx::dataref_manager::get_fnrml_total();   // XPLMGetDataf(dref_const.dref_fnrml_f); //


  this->stats.AoA   = missionx::dataref_manager::getAoA();   // XPLMGetDataf(dref_const.AoA_f); //
  this->stats.pitch = missionx::dataref_manager::getPitch(); // XPLMGetDataf(dref_const.dref_pitch_f); //
  this->stats.roll  = missionx::dataref_manager::getRoll();  // XPLMGetDataf(dref_const.dref_roll_f); //

  this->stats.heading_mag    = XPLMGetDataf(dref_const.dref_heading_mag_psi_f);  // this->getMagPsiHeading();
  this->stats.heading_no_mag = XPLMGetDataf(dref_const.dref_heading_true_psi_f); // this->getPsiHeading();

  this->stats.prev_total_running_time_sec = stats.total_running_time_sec;
  this->stats.total_running_time_sec      = missionx::dataref_manager::getTotalRunningTimeSec(); // XPLMGetDataf(dref_const.dref_total_running_time_sec_f);  //this->getTotalRunningTimeSec();

  // v3.303.8.3
  this->stats.vh_ind_fpm     = XPLMGetDataf(dref_const.dref_vh_ind_fpm_f); // v3.303.8.3 VH (vertical velocity in feet per second)
  this->stats.Qrad           = XPLMGetDataf(dref_const.dref_Qrad_f);       // v3.303.8.3 The pitch rotation rates in rad/sec (relative to the flight)
  this->stats.Q              = XPLMGetDataf(dref_const.dref_Q_f);          // v3.303.8.3 The pitch rotation rates in deg/sec (relative to the flight)
  //this->stats.y_agl          = XPLMGetDataf(dref_const.dref_y_agl_f);      // v3.303.14 remomved since it was renamed to fAGL// v3.303.8.3 AGL in meters (above ground level)
  this->stats.onground_any_i = XPLMGetDatai(dref_const.dref_onground_any_i); // v3.303.8.3 any wheel on ground
  this->stats.onground_all_i = XPLMGetDatai(dref_const.dref_onground_all_i); // v3.303.8.3 all wheels on ground

  this->stats.m_total = missionx::dataref_manager::get_mTotal_currentTotalWeightK(); // v3.303.14

  this->stats.activity.clear();
}

// ----------------------------------------------------


void
GatherStats::gather_and_store_stats(const std::string& inActivity)
{
  this->gather_stats_data();
  this->stats.activity = inActivity;

  this->addInjectedPlaneStat(this->stats);
}

// ----------------------------------------------------
void
GatherStats::store_stats()
{
  this->addInjectedPlaneStat(this->stats);
}

// ----------------------------------------------------

void
GatherStats::init_seq_from_checkpoint_stats()
{
  assert(db->db_is_open_and_ready && "stats db is not available, probably was not initialized correctly");

  if (db->db_is_open_and_ready)
  {
    const std::string stmt = "select max(line_id) as last_id from stats";
    const std::string unique_name_s = "get_last_stats_line_id";

    if (db->prepareNewStatement(unique_name_s, stmt))
    {
      if (db->step(db->mapStatements[unique_name_s]) == SQLITE_ROW) // fetch 1 row))
      {
        int iCol = 0;
        //line_seq = sqlite3_column_int(data_manager::db_stats.mapStatements[unique_name_s], iCol); // we init the line sequence to "last saved value" in the sqlite database        
        line_seq = sqlite3_column_int(this->db->mapStatements[unique_name_s], iCol); // we init the line sequence to "last saved value" in the sqlite database        
      }
    }
  
  }
}


// ----------------------------------------------------
bool
GatherStats::exec_gather_thread(thread_state* xxthread_state)
{
  if (!db->db_is_open_and_ready)
    return false;

  if (!db->getIsStatement_withTheName_exists(INS_SQL))
  {
    Log::logMsgErr("[stats] Insert statement is not prepared, notify developer.", true);
    return false;
  }

  xxthread_state->flagIsActive        = true;
  xxthread_state->flagThreadDoneWork = false;
  int i                            = 1; // field index
  int row_counter                  = 0; // for debug

  // start transaction
  db->start_transaction();

  while (!qInjectPlaneStats.empty() && !(xxthread_state->flagAbortThread))
  {
    i = 1;         // in sqlite c++ api, column numbering start with 1. Reset i if Queue has more than 1 value. This wil solve trying to place data outside of field index.
    row_counter++; // for debug
    line_seq++;    // unique numbering of each row

    // Utils::logMsg("Thread exec_gather_thread(): " + mxUtils::formatNumber<int>(line_seq) + ", Loop iteration : " + mxUtils::formatNumber<int>(row_counter), ture ); // debug

    // get queue data for insertion
    auto q = qInjectPlaneStats.front();

    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::int_typ, i, mxUtils::formatNumber<int>(line_seq));
    i++; // line_seq
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.vvi_fpm_pilot));
    i++; // fpm pilot - might assist in landing
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.flap_ratio));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::int_typ, i, mxUtils::formatNumber<int>(q.local_date_days));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.local_time_sec, 3));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::double_typ, i, mxUtils::formatNumber<double>(q.lat, 6));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::double_typ, i, mxUtils::formatNumber<double>(q.lon, 6));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::double_typ, i, mxUtils::formatNumber<double>(q.elev, 2));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::double_typ, i, mxUtils::formatNumber<float>(q.fAGL, 2)); // v3.303.14
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.airspeed, 2));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.groundspeed, 2));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.vvi, 2));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.faxil_gear, 3));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.brake_L, 3));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.brake_R, 3));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.gforce_normal, 3));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.gforce_axil, 3));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.fnrml_total, 3));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.AoA, 2));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.pitch, 3));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.roll, 3));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.heading_mag, 2));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.heading_no_mag, 2));
    i++;
    // v3.303.14 insert NULL and not just empty string
    if (q.activity.empty())
      db->bind_to_stored_stmt(INS_SQL, missionx::db_types::null_typ, i, q.activity);
    else 
      db->bind_to_stored_stmt(INS_SQL, missionx::db_types::text_typ, i, q.activity);
    i++;
    // v3.303.8.3
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::int_typ, i, mxUtils::formatNumber<int>(q.onground_any_i));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::int_typ, i, mxUtils::formatNumber<int>(q.onground_all_i));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.vh_ind_fpm, 2));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.Qrad, 2));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.Q, 2));
    //i++;
    //db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.y_agl, 2));
    i++;
    db->bind_to_stored_stmt(INS_SQL, missionx::db_types::float_typ, i, mxUtils::formatNumber<float>(q.m_total, 2));
    i++;


    db->step(db->mapStatements[INS_SQL]);
    // Log::logMsgThread("After step");

    db->clear_and_reset(db->mapStatements[INS_SQL]);
    // Log::logMsgThread("After clear_reset");

    qInjectPlaneStats.pop(); // erase oldest member in queue

  } // end loop over vector

  db->end_transaction();

  xxthread_state->flagIsActive        = false;
  xxthread_state->flagThreadDoneWork = true;

  return true;
}

// ----------------------------------------------------
void
GatherStats::addInjectedPlaneStat(_stats_gather_ &inStat)
{
  // calculate cumulative distance relative to previous
  if ((this->prev_stats.lat != 0.0) * (this->prev_stats.lon != 0.0) > 0)
  {
    auto fDelta = static_cast<float>(mxUtils::mxCalcDistanceBetween2Points(this->prev_stats.lat, prev_stats.lon, inStat.lat, inStat.lon));
    if (std::isnan(fDelta))
      fDelta = 0.0f;

    inStat.fCumulativeDistance_nm += fDelta;
  }

  this->qInjectPlaneStats.push(inStat);
}

void
GatherStats::copyLastStatsToUiStruct(missionx::mx_enrout_stats_strct& inoutStats)
{
  inoutStats.fDistanceFlew = this->stats.fCumulativeDistance_nm - inoutStats.fCumulativeDistanceFlew_beforeCurrentLeg;

  if (this->stats.gforce_normal > inoutStats.fMaxG)
    inoutStats.fMaxG = this->stats.gforce_normal;
  else if (this->stats.gforce_normal < inoutStats.fMinG)
    inoutStats.fMinG = this->stats.gforce_normal;

  if (this->stats.pitch > inoutStats.fMaxPitch)
    inoutStats.fMaxPitch = this->stats.pitch;
  else if (this->stats.pitch < inoutStats.fMinPitch)
    inoutStats.fMinPitch = this->stats.pitch;

  if (this->stats.roll > inoutStats.fMaxRoll)
    inoutStats.fMaxRoll = this->stats.roll;
  else if (this->stats.roll < inoutStats.fMinRoll)
    inoutStats.fMinRoll = this->stats.roll;

}

// ----------------------------------------------------
// ----------------------------------------------------


} // missionx namespace
