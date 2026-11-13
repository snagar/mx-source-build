#include "gather_stats.h"
#include "../dataref_manager.h"
#include <chrono>     // v26.10.2 in-flight stats copy time
#include <filesystem> // v26.10.2

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
      // place stats inside the queue. v26.10.1 addInjectedPlaneStat() locks the queue itself.
      this->addInjectedPlaneStat(this->stats);

      // Call Thread to flush data into SQLite // v26.10.1
      if (!xxthread_state.flagIsActive && this->db != nullptr && !GatherStats::flag_ignore_stats_setup)
      {
        xxthread_state.init();
        if (thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
          thread_ref.join();       // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"

        thread_ref = std::thread(&GatherStats::exec_gather_thread, this, &xxthread_state, true); // v26.10.2 true: the writer thread may make the in-flight stats copy
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

    // v26.10.1 skip table creation if flag_ignore_stats_setup is set.
    if (missionState_s.empty() && !GatherStats::flag_ignore_stats_setup)
    {

      db->execute_stmt(sqlStmt);
      this->line_seq = 1; // v26.10.2 new table: the numbering starts again (the first row is 2). Was only reset when the class was created, so a second mission continued the numbering

      // sqlStmt = "CREATE TABLE if not exists stats "
      //           "( line_id INTEGER, vvi_fpm_pilot FLOAT, flap_ratio FLOAT, local_date_days INT, local_time_sec FLOAT, lat DOUBLE, "
      //           " lon DOUBLE, elev DOUBLE, agl FLOAT, airspeed FLOAT, groundspeed FLOAT, vh_ind FLOAT, faxil_gear FLOAT,"
      //           " brakes_L FLOAT, brakes_R FLOAT,"
      //           " gforce_normal FLOAT, gforce_axil FLOAT, fnrml_total_nw FLOAT, AoA FLOAT, pitch FLOAT, roll FLOAT, heading_mag FLOAT, "
      //           " heading_no_mag FLOAT, activity TEXT "
      //           " ,onground_any INT, onground_all INT ,vh_ind_fpm FLOAT, Qrad FLOAT, Q FLOAT, m_total FLOAT )"; // v3.303.14 added m_total v3.303.8.3 added onground_any, onground_all, vvi_ind_fpm_f, Qrad_f, Q_f, y_agl_f

      sqlStmt = "CREATE TABLE if not exists stats "
                "( line_id INTEGER, vvi_fpm_pilot FLOAT, flap_ratio FLOAT, local_date_days INT, local_time_sec FLOAT, lat DOUBLE, "
                " lon DOUBLE, elev DOUBLE, agl FLOAT, airspeed FLOAT, groundspeed FLOAT, vh_ind FLOAT, faxil_gear FLOAT,"
                " brakes_L FLOAT, brakes_R FLOAT,"
                " gforce_normal FLOAT, gforce_axil FLOAT, fnrml_total_nw FLOAT, AoA FLOAT, pitch FLOAT, roll FLOAT, heading_mag FLOAT, "
                " heading_no_mag FLOAT, activity TEXT "
                " ,onground_any INT, onground_all INT ,vh_ind_fpm FLOAT, Qrad FLOAT, Q FLOAT, m_total FLOAT, date_time TEXT DEFAULT (STRFTIME('%Y-%m-%d %H:%M:%f', 'now')) )"; // v25.09.2 added date_time // v3.303.14 added m_total v3.303.8.3 added onground_any, onground_all, vvi_ind_fpm_f, Qrad_f, Q_f, y_agl_f

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

      // v26.10.2 flight metadata tables. New mission: drop them here, create_meta_tables_cai() below creates them empty.
      db->execute_stmt(fmt::format("DROP TABLE if exists {}", mxconst::get_DB_TABLE_FLIGHT_INFO()).c_str());
      db->execute_stmt(fmt::format("DROP TABLE if exists {}", mxconst::get_DB_TABLE_PLANE_META()).c_str());
      db->execute_stmt(fmt::format("DROP TABLE if exists {}", mxconst::get_DB_TABLE_LEG_STATS()).c_str()); // v26.10.2
    } // end if missionState_s is empty - rebuild objects

    // v26.10.2 Checkpoint: the metadata tables came back with the saved stats DB, "if not exists" keeps them.
    //          A checkpoint saved before v26.10.2 has no metadata tables, they are created empty here.
    if (!GatherStats::flag_ignore_stats_setup)
      this->create_meta_tables_cai();

    if (GatherStats::flag_ignore_stats_setup) // v26.10.1
      Log::logMsgWarn(fmt::format("[{}] 'Ignore stats' is set: the existing stats table is kept and no rows will be written.", __func__) );

    // v3.303.8.3 This prepared statement always need to be executed.
    // Create the "insert" statement to the stats table
    sqlStmt = "insert into stats "
              "(line_id, vvi_fpm_pilot, flap_ratio, local_date_days, local_time_sec, lat, " // 6
              "lon, elev, agl, airspeed, groundspeed, vh_ind, faxil_gear, "                 // 7
              "brakes_L, brakes_R,"                                                         // 2
              "gforce_normal, gforce_axil, fnrml_total_nw, AoA, pitch, roll, heading_mag, "  // 7
              "heading_no_mag, activity, "                                                  // 2
              "onground_any, onground_all, vh_ind_fpm, Qrad, Q, m_total  ) "                // 7
              " values (?, ?, ?, ?, ?, ?,  ?, ?, ?, ?, ?, ?, ?,  ?, ?,  ?, ?, ?, ?, ?, ?, ?,  ?, ?,  ?, ?, ?, ?, ?, ?);"; // total 30 cols. The date_time columns will be added automatically

    if (!db->prepareNewStatement(INS_SQL, sqlStmt))
    {
      Log::logMsgErr("[Stats] Failed to create 'insert' statement: " + db->last_err);
    }


  } // end if db is not nullptr
}


// ----------------------------------------------------
// v26.10.2 Create the flight metadata tables if they do not exist. Rows are written by data_manager::stats_meta_xxx_cai() functions.
//          flight_info: one row (id = 1).
//          plane_meta : one row for each plane flown. line_id = last stats row written before the plane was loaded (0 = mission start).
//          leg_stats  : one row for each finished flight leg, in the order flown (leg_seq 1, 2, 3...).
//          Times use the same UTC format as stats.date_time.
//          Uses execute_stmt(const char*): execute_stmt(std::string) copies into a 1024 char buffer.
void
GatherStats::create_meta_tables_cai() const
{
  if (db == nullptr || !db->db_is_open_and_ready)
    return;

  const std::string sql_flight_info = fmt::format(R"(CREATE TABLE if not exists {} (
  id             INTEGER PRIMARY KEY CHECK (id = 1),
  flight_id      TEXT,
  schema_version INT,
  plugin_version TEXT,
  xp_version     INT,
  mission_key    TEXT,
  mission_title  TEXT,
  os_start_time  TEXT DEFAULT (STRFTIME('%Y-%m-%d %H:%M:%f', 'now')),
  os_end_time    TEXT,
  outcome        TEXT,
  outcome_reason TEXT,
  crashed        INT DEFAULT 0,
  crash_source   TEXT,
  route_s        TEXT,
  scoring_xml    TEXT,
  archived       INT DEFAULT 0 ))",
                                                  mxconst::get_DB_TABLE_FLIGHT_INFO());

  const std::string sql_plane_meta = fmt::format(R"(CREATE TABLE if not exists {} (
  line_id       INTEGER,
  date_time     TEXT DEFAULT (STRFTIME('%Y-%m-%d %H:%M:%f', 'now')),
  acf_file      TEXT,
  acf_path      TEXT,
  acf_icao      TEXT,
  acf_desc      TEXT,
  acf_author    TEXT,
  category      TEXT,
  is_helicopter INT,
  empty_kg      FLOAT,
  max_kg        FLOAT,
  max_fuel_kg   FLOAT,
  vno_kias      FLOAT,
  num_engines   INT ))",
                                                 mxconst::get_DB_TABLE_PLANE_META());

  // leg_seq is the rowid: 1 for the first leg flown. Fuel and payload are kg (m_fuel_total, m_fixed). Times are OS time (UTC).
  // The row is added when the leg starts (status "running") and updated when it ends ("completed"), at a checkpoint save (progress, still "running"),
  // or at the end of the flight when the leg did not end (status = the flight outcome: failure, aborted, stopped).
  const std::string sql_leg_stats = fmt::format(R"(CREATE TABLE if not exists {} (
  leg_seq            INTEGER PRIMARY KEY,
  leg_name           TEXT,
  status             TEXT,
  start_line_id      INTEGER,
  end_line_id        INTEGER,
  start_time         TEXT DEFAULT (STRFTIME('%Y-%m-%d %H:%M:%f', 'now')),
  end_time           TEXT,
  distance_before_nm FLOAT,
  distance_nm        FLOAT,
  start_fuel_kg      FLOAT,
  end_fuel_kg        FLOAT,
  start_payload_kg   FLOAT,
  end_payload_kg     FLOAT ))",
                                                mxconst::get_DB_TABLE_LEG_STATS());

  if (!db->execute_stmt(sql_flight_info.c_str()))
    Log::logMsgErr(fmt::format("[{}] Failed to create '{}' table: {}", __func__, mxconst::get_DB_TABLE_FLIGHT_INFO(), db->last_err));

  if (!db->execute_stmt(sql_plane_meta.c_str()))
    Log::logMsgErr(fmt::format("[{}] Failed to create '{}' table: {}", __func__, mxconst::get_DB_TABLE_PLANE_META(), db->last_err));

  if (!db->execute_stmt(sql_leg_stats.c_str()))
    Log::logMsgErr(fmt::format("[{}] Failed to create '{}' table: {}", __func__, mxconst::get_DB_TABLE_LEG_STATS(), db->last_err));

  if (!db->execute_stmt(fmt::format("PRAGMA user_version = {}", mxconst::STATS_SCHEMA_VERSION).c_str()))
    Log::logMsgErr(fmt::format("[{}] Failed to set the stats schema version: {}", __func__, db->last_err));
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
GatherStats::exec_gather_thread(strct_thread_state* inout_thread_state_ptr, const bool in_flag_allow_copy)
{
  if (!db->db_is_open_and_ready)
    return false;

  if (!db->getIsStatement_withTheName_exists(INS_SQL))
  {
    Log::logMsgErr("[stats] Insert statement is not prepared, notify developer.");
    return false;
  }

  inout_thread_state_ptr->flagIsActive        = true;
  inout_thread_state_ptr->flagThreadDoneWork = false;
  int i                            = 1; // field index
  int row_counter                  = 0; // for debug

  // v26.10.1 take all queued rows in one step. The draw callback can keep pushing into the (now empty) member queue
  //          while we write, so the queue is never used by two threads at the same time.
  std::queue<mx_struct_stats_gather> local;
  {
    std::lock_guard<std::mutex> lock(missionx::mutexGatherStats);
    std::swap(local, this->qInjectPlaneStats);
  }

  // start transaction
  db->start_transaction();

  while (!local.empty() && !(inout_thread_state_ptr->flagAbortThread))
  {
    i = 1;         // in sqlite c++ api, column numbering start with 1. Reset i if Queue has more than 1 value. This wil solve trying to place data outside of field index.
    row_counter++; // for debug
    line_seq++;    // unique numbering of each row

    // Utils::logMsg("Thread exec_gather_thread(): " + mxUtils::formatNumber<int>(line_seq) + ", Loop iteration : " + mxUtils::formatNumber<int>(row_counter), ture ); // debug

    // get queue data for insertion
    auto q = local.front();

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

    local.pop(); // erase oldest member in queue

  } // end loop over vector

  db->end_transaction();

  // v26.10.2 the in-flight stats copy: right after the commit, on this thread, only when the stats tab asked for it
  if (in_flag_allow_copy && !inout_thread_state_ptr->flagAbortThread && this->copy_state.load() == missionx::mx_stats_copy_state::requested)
    this->make_copy_cai();

  inout_thread_state_ptr->flagIsActive        = false;
  inout_thread_state_ptr->flagThreadDoneWork = true;

  return true;
}

// ----------------------------------------------------
void
GatherStats::addInjectedPlaneStat(mx_struct_stats_gather &inStat)
{
  // calculate cumulative distance relative to previous
  if ((this->prev_stats.lat != 0.0) * (this->prev_stats.lon != 0.0) > 0)
  {
    auto fDelta = static_cast<float>(mxUtils::mxCalcDistanceBetween2Points(this->prev_stats.lat, prev_stats.lon, inStat.lat, inStat.lon));
    if (std::isnan(fDelta))
      fDelta = 0.0f;

    inStat.fCumulativeDistance_nm += fDelta;
  }

  // v26.10.1 gathered (distance above is updated) but not queued for writing
  if (GatherStats::flag_ignore_stats_setup)
    return;

  std::lock_guard<std::mutex> lock(missionx::mutexGatherStats); // v26.10.1 the writer thread takes rows from this queue
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
// v26.10.1 Wait until the running writer thread finished. Its transaction is committed, nothing is aborted.
void
GatherStats::wait_for_writer()
{
  if (this->thread_ref.joinable())
    this->thread_ref.join();

  if (this->xxthread_state.flagThreadDoneWork)
    this->xxthread_state.init();
}

// ----------------------------------------------------
// v26.10.1 Stop using the stats database. Call it before the database is closed.
//   in_flush_pending_rows = true : wait for the writer, then write the rows still in the queue (end of mission).
//   in_flush_pending_rows = false: abort the writer and drop the queue (stop mission, load checkpoint, plugin stop).
void
GatherStats::detach_db(const bool in_flush_pending_rows)
{
  if (in_flush_pending_rows)
  {
    this->flush_pending_rows_cai(); // v26.10.2 was inline here: wait for the writer, then write the last samples (for example the landing burst)
  }
  else
  {
    this->stop_gather(); // abort + join
  }

  this->clear_queue();
  this->db = nullptr; // flc(), the draw callback and exec_gather_thread() all check for nullptr
}

// ----------------------------------------------------
// v26.10.2 Write every queued row now, on the calling thread (main thread). Used by detach_db(true) and before a plane_meta row is added,
//          so that get_last_line_id_cai() is the last row sampled with the previous plane. The connection stays attached and flc() continues as usual.
void
GatherStats::flush_pending_rows_cai()
{
  this->wait_for_writer();

  // No writer thread is running now.
  if (this->db != nullptr && this->db->db_is_open_and_ready && !GatherStats::flag_ignore_stats_setup) // v26.10.1
  {
    this->xxthread_state.init();
    this->exec_gather_thread(&this->xxthread_state, false); // v26.10.2 false: main thread, no in-flight stats copy here
  }
}

// ----------------------------------------------------
// v26.10.2 Ask for the in-flight stats copy (main thread). The next writer thread makes it after its commit (make_copy_cai()).
//          While the sim is paused no writer runs, so the copy waits until the sim runs again.
bool
GatherStats::request_copy_cai(const std::string &in_file_path)
{
  if (this->copy_state.load() != missionx::mx_stats_copy_state::idle)
    return false;

  this->copy_file_path = in_file_path; // written before the state changes: the writer reads it only after it sees "requested"
  this->copy_error.clear();
  this->flag_abort_copy.store(false);
  this->copy_state.store(missionx::mx_stats_copy_state::requested);
  return true;
}

// ----------------------------------------------------
// v26.10.2 Back to "idle" (main thread). Safe when no writer runs, or when the copy is "ready" or "failed" (the writer does not touch it then).
void
GatherStats::reset_copy_cai()
{
  this->copy_state.store(missionx::mx_stats_copy_state::idle);
}

// ----------------------------------------------------
// v26.10.2 Drop the in-flight stats copy (main thread): the mission is not running any more.
//          "requested": the writer will not start it. "copying": sqlite3_interrupt() stops the VACUUM INTO (safe from another thread,
//          a no-op when no statement runs), and the writer deletes the part it wrote. "ready"/"failed": the writer does not touch it, back to idle.
void
GatherStats::abort_copy_cai()
{
  this->flag_abort_copy.store(true);

  auto expected_state = missionx::mx_stats_copy_state::requested;
  if (this->copy_state.compare_exchange_strong(expected_state, missionx::mx_stats_copy_state::idle))
    return; // not started

  if (expected_state == missionx::mx_stats_copy_state::copying)
  {
    if (this->db != nullptr && this->db->db != nullptr)
      sqlite3_interrupt(this->db->db);
    return; // the writer sets "idle" when it sees the abort
  }

  if (expected_state == missionx::mx_stats_copy_state::ready || expected_state == missionx::mx_stats_copy_state::failed)
    this->copy_state.store(missionx::mx_stats_copy_state::idle);
}

// ----------------------------------------------------
// v26.10.2 The in-flight stats copy. Writer thread only, right after its commit: no other code uses the connection now
//          (the main thread waits for the writer before it writes its own rows), so the live file is not read while it is written.
//          "VACUUM INTO" does not overwrite a file, the old copy is deleted first.
void
GatherStats::make_copy_cai()
{
  auto expected_state = missionx::mx_stats_copy_state::requested;
  if (!this->copy_state.compare_exchange_strong(expected_state, missionx::mx_stats_copy_state::copying))
    return;

  if (this->flag_abort_copy.load()) // aborted between the request and now
  {
    this->copy_state.store(missionx::mx_stats_copy_state::idle);
    return;
  }

  const auto start_time = std::chrono::steady_clock::now();

  std::error_code ec;
  std::filesystem::remove(this->copy_file_path, ec); // no file is not an error

  char     *err_msg = nullptr;
  const int rc      = sqlite3_exec(this->db->db, ("VACUUM INTO " + mxUtils::sql_quote(this->copy_file_path)).c_str(), nullptr, nullptr, &err_msg);

  const auto copy_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
  if (rc == SQLITE_INTERRUPT || this->flag_abort_copy.load()) // the mission is not running any more: the copy is not needed
  {
    std::filesystem::remove(this->copy_file_path, ec);
    Log::logMsg(fmt::format("[{}] In-flight stats copy aborted after {} ms: the mission is not running.", __func__, copy_ms));
    this->copy_state.store(missionx::mx_stats_copy_state::idle);
  }
  else if (rc == SQLITE_OK)
  {
    Log::logMsg(fmt::format("[{}] In-flight stats copy ready: '{}' ({} ms, last line_id {}).", __func__, this->copy_file_path, copy_ms, this->line_seq));
    this->copy_state.store(missionx::mx_stats_copy_state::ready);
  }
  else
  {
    this->copy_error = (err_msg != nullptr) ? err_msg : "unknown error";
    Log::logMsgErr(fmt::format("[{}] In-flight stats copy failed after {} ms: {}", __func__, copy_ms, this->copy_error));
    this->copy_state.store(missionx::mx_stats_copy_state::failed);
  }
  sqlite3_free(err_msg);
}

// ----------------------------------------------------
void
GatherStats::clear_queue()
{
  std::lock_guard<std::mutex> lock(missionx::mutexGatherStats);
  std::queue<mx_struct_stats_gather>().swap(this->qInjectPlaneStats);
}

// ----------------------------------------------------
// ----------------------------------------------------

} // missionx namespace
