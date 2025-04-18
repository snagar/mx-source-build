#pragma once
#ifndef OPTIMIZEAPTDAT_H_
#define OPTIMIZEAPTDAT_H_

#include <cstdio>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "../core/thread/base_thread.hpp"
#include "dbase.h"

using namespace missionx;
// using namespace std;


namespace missionx
{

class OptimizeAptDat : public base_thread
{
private:
  static missionx::dbase* db_airports_xp_ptr;
  static missionx::dbase* db_airports_cache_ptr;
  missionx::db_field      get_db_field_by_colName(std::list<missionx::db_field>& list_of_fields, std::string inColNameToSearch);

  static const int STARTING_COUNTER_I = 1; // v3.303.8.3 used with apt_dat internal character counter. Should always start with 1

public:
  OptimizeAptDat();
  virtual ~OptimizeAptDat();

  static void setAptDataFolders(std::string inCustomScenery, std::string inDefaultAptDatFolder, std::string inMissionxFolder);
  void        set_database_pointers(missionx::dbase* in_db_airports_xp); // v3.0.241.10 "airports_xp.sqlite.db"
  void set_database_pointers2(missionx::dbase* in_db_airports_xp); // v3.0.255.3 store the temporary database working file. we will need to copy all table content to the final database. [airports_xp.sqlite2.db => airports_xp.sqlite.db]

  ///// APT DAT Thread /////
  static std::thread  thread_ref;
  static thread_state aptState;
  // members
  void exec_optimize_aptdat_thread();
  bool read_and_parse_all_apt_dat_files(thread_state* inThreadState);
  bool       parse_aptdat(thread_state* inThreadState, std::string& relative_apt_dat_path, const std::string scenery_pack_name, bool inReadCachedFile = false, const bool is_custom = false);


  // sqlite members
  // void upload_data_to_sqlite(thread_state *inThreadState); // v3.0.255.3 Deprecated. Moved to parse_airport_to_sqlite
  // void prepare_working_sqlite_db(thread_state* inThreadState);
  void        prepare_sqlite_db_tables(thread_state* inThreadState, missionx::dbase* inDB_ptr);
  static void sqlite_post_tables_copy(missionx::dbase* inDB_ptr);
  void        parse_airport_to_sqlite(thread_state* inThreadState, const std::string& airportCode, const missionx::mx_aptdat_cached_info& info, missionx::dbase& db, const std::string& scenery_pack_name); // v3.0.255.3

  void stop_plugin();
  //// End Thread AptData ////////

  ///// indexing files
private:
  int               icao_id_counter{ 0 };
  const std::string pathTemp = "d:/temp/inmemory.sqlite.db";
  void upload_navdata_to_inMemory_db(thread_state* inThreadState, missionx::dbase& db);

  // Each row represent a fields names and their type.
  std::map<int, std::list<missionx::db_field>> table_col_name_type_mapping = {
    { 1,
      { db_field("icao", db_types::text_typ, ""),
        db_field("ap_elev", db_types::int_typ),
        db_field("", db_types::skip_field),
        db_field("", db_types::skip_field),
        db_field("[icao]", db_types::skip_field),
        db_field("ap_name", db_types::text_typ) } }, // land airports
    { 16,
      { db_field("icao", db_types::text_typ, ""),
        db_field("ap_elev", db_types::int_typ),
        db_field("", db_types::skip_field),
        db_field("", db_types::skip_field),
        db_field("[icao]", db_types::skip_field),
        db_field("ap_name", db_types::text_typ) } }, // sea port
    { 17,
      { db_field("icao", db_types::text_typ, ""),
        db_field("ap_elev", db_types::int_typ),
        db_field("", db_types::skip_field),
        db_field("", db_types::skip_field),
        db_field("[icao]", db_types::skip_field),
        db_field("ap_name", db_types::text_typ) } },// heliport
    { 1302, 
      { db_field("icao", db_types::text_typ, ""), 
        db_field("key_col", db_types::text_typ), 
        db_field("val_col", db_types::text_typ) } }, // airport metadata
    { 1300,
      { db_field("icao", db_types::text_typ, ""),
        db_field("ramp_lat", db_types::real_typ),
        db_field("ramp_lon", db_types::real_typ),
        db_field("ramp_heading_true", db_types::int_typ),
        db_field("location_type", db_types::text_typ),
        db_field("for_planes", db_types::text_typ),
        db_field("ramp_uq_name", db_types::text_typ) } }, // ramps
    { 15,
      { db_field("icao", db_types::text_typ, ""),
        db_field("ramp_lat", db_types::real_typ),
        db_field("ramp_lon", db_types::real_typ),
        db_field("ramp_heading_true", db_types::int_typ),
        db_field("ramp_uq_name", db_types::text_typ) } }, // ramps - old format
    { 100,
      { db_field("icao", db_types::text_typ, ""),        db_field("rw_width", db_types::real_typ),       db_field("rw_surf", db_types::int_typ),
        db_field("rw_sholder", db_types::int_typ),       db_field("rw_smooth", db_types::real_typ),      db_field("[center_lights]", db_types::skip_field),
        db_field("[edge_lights]", db_types::skip_field), db_field("[auto signs]", db_types::skip_field), db_field("rw_no_1", db_types::text_typ),
        db_field("rw_no_1_lat", db_types::real_typ),     db_field("rw_no_1_lon", db_types::real_typ),    db_field("rw_no_1_disp_hold", db_types::real_typ),
        db_field("[blast]", db_types::skip_field),       db_field("[marking]", db_types::skip_field),    db_field("[approach light]", db_types::skip_field),
        db_field("[tdz lights]", db_types::skip_field),  db_field("[reil light]", db_types::skip_field), db_field("rw_no_2", db_types::text_typ),
        db_field("rw_no_2_lat", db_types::real_typ),     db_field("rw_no_2_lon", db_types::real_typ),    db_field("rw_no_2_disp_hold", db_types::real_typ) } }, // land rw
    { 101,
      { db_field("icao", db_types::text_typ, ""),
        db_field("rw_width", db_types::real_typ),
        db_field("[perimeter buoy]", db_types::skip_field),
        db_field("rw_no_1", db_types::text_typ),
        db_field("rw_no_1_lat", db_types::real_typ),
        db_field("rw_no_1_lon", db_types::real_typ),
        db_field("rw_no_2", db_types::text_typ),
        db_field("rw_no_2_lat", db_types::real_typ),
        db_field("rw_no_2_lon", db_types::real_typ) } }, // sea lane
    { 102,
      { db_field("icao", db_types::text_typ, ""),
        db_field("name", db_types::text_typ),
        db_field("lat", db_types::real_typ),
        db_field("lon", db_types::real_typ),
        db_field("[orientation deg]", db_types::skip_field),
        db_field("length", db_types::text_typ),
        db_field("width", db_types::real_typ) } }, // Helos
    { 1050, 
      { db_field("icao", db_types::text_typ, ""), 
        db_field("frq", db_types::int_typ),
        db_field("frq_desc", db_types::text_typ) } }, //  Frequence devide by 1000
    { 1051, 
      { db_field("icao", db_types::text_typ, ""), 
        db_field("frq", db_types::int_typ),
        db_field("frq_desc", db_types::text_typ) } }, //  Frequence devide by 1000
    { 1052, 
      { db_field("icao", db_types::text_typ, ""), 
        db_field("frq", db_types::int_typ),
        db_field("frq_desc", db_types::text_typ) } }, //  Frequence devide by 1000
    { 1053, 
      { db_field("icao", db_types::text_typ, ""), 
        db_field("frq", db_types::int_typ),
        db_field("frq_desc", db_types::text_typ) } }, //  Frequence devide by 1000
    { 1054, 
      { db_field("icao", db_types::text_typ, ""), 
        db_field("frq", db_types::int_typ),
        db_field("frq_desc", db_types::text_typ) } }, //  Frequence devide by 1000
    { 1055, 
      { db_field("icao", db_types::text_typ, ""), 
        db_field("frq", db_types::int_typ),
        db_field("frq_desc", db_types::text_typ) } }, //  Frequence devide by 1000    
    { 1056, 
      { db_field("icao", db_types::text_typ, ""), 
        db_field("frq", db_types::int_typ),
        db_field("frq_desc", db_types::text_typ) } } //  Frequence devide by 1000


  };
};




}

#endif
