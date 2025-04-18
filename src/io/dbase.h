#ifndef DBASEH_
#define DBASEH_
#pragma once

/******
DBASE should work in synchronized mode and not threaded by default.
Threading should be an option.

*/

#include <cstdio>
#include <iostream>
#include <map>
#include <memory> // https://bryanstamour.com/post/sqlite-with-cpp/
#include <stdio.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <assert.h>

//#include "../core/Utils.h"
#include "../core/thread/base_thread.hpp"


//// SQLITE special FLAGS ////
#define SQLITE_OMIT_DEPRECATED
#define SQLITE_OMIT_PROGRESS_CALLBACK
#define SQLITE_OMIT_SHARED_CACHE

#include "sqlite3.h"

namespace missionx
{
  inline static constexpr const int MAX_ALLOWED_ROWS_TO_FETCH = 50000;
class db_field
{
private:
  

public:
  missionx::db_types dataype{ missionx::db_types::skip_field };
  std::string        value_s{ "" };
  std::string        col_name{ "" };

  db_field() {}
  ~db_field() {}
  db_field(std::string inColName, missionx::db_types inType, std::string inVal_s = "")
    : dataype(inType)
    , value_s(inVal_s)
    , col_name(inColName)
  {
  }
}; // end class db_field


// --------------------------------------------
// --------------------------------------------
// --------------------------------------------

class dbase : public base_thread
{
private:
  std::string absolute_path_to_db_file_s{ "" };
  // https://visualstudiomagazine.com/articles/2014/02/01/using-sqlite-with-modern-c.aspx Looks more compatible aware

  //// https://bryanstamour.com/post/sqlite-with-cpp/
  //// quote: "... put any cleanup code you want to execute inside of it, and you will be exception-safe!"
  // c++ 14/17
  // template <typename Func> struct scope_exit {
  //  explicit scope_exit(Func f) : f_{ f } {}
  //  ~scope_exit() { f_(); }
  // private:
  //  Func f_;
  //};

  //// Quote: "... Here we will make a simple function object (an object with an overloaded function-call operator) that simply calls sqlite3_close on the passed-in pointer."
  //// Not sure if we need this, since our destructor do the same
  // c++ 14/17
  // struct sqlite3_deleter {
  //  void operator () (sqlite3* db) const { sqlite3_close(db); }
  //};

  // quate: Next we introduce a type alias to make the rest of our code easier to follow.
  // Not sure what that means... new staff :-)
  // using sqlite3_handle = unique_ptr<sqlite3, sqlite3_deleter>; // c++ 14/17


  // auto make_sqlite3_handle(char const* db_name); // c++ 14/17


  int callback(void*, int argc, char** argv, char** col_names);

public:
  // enum enum_sqlite_types
  //{
  //  sqlite_int = 1,
  //  sqlite_float = 2,
  //  sqlite_text = 3,
  //  sqlite_null = 5,
  //  sqlite_double = 6

  //};
  dbase();
  virtual ~dbase();

  thread_state xxthread_state;

  std::string last_err{ "" };

  sqlite3* db; // pointer to sqlite databse
  // sqlite3_handle db; // c++ 14/17 pointer to sqlite databse
  bool                                         db_is_open_and_ready;
  std::map<std::string, sqlite3_stmt*>         mapStatements;
  std::map<std::string, const char* /*zTail*/> mapZTail; // for each statement

  sqlite3_stmt* stmt;
  char*         zTail;
  const char*   zTail_ins_stats;
  unsigned int  seq_ins_stats;

  // sqlite3_stmt* stmt_insert_stats;


  char* zErr; // error message
  int   rc;   // status. 0 = success/OK

  // std::string sqlStmt;
  char sql[1024] = { 0 }; //= "drop table if exists stats;";
  int  size      = 0;     // used with snprintf

  //// members to keep
  std::string get_db_path() { return this->absolute_path_to_db_file_s; }
  void        set_db_path_and_file(std::string inValue) { this->absolute_path_to_db_file_s = inValue; }
  bool        open_database();
  bool        open_database_in_memory(); // v3.0.255.3
  bool        remove_database(std::string in_absolute_path);
  bool        close_database(); // finalize all statements and clode database
  bool        execute_stmt(std::string inStmt);
  bool        execute_stmt(const char* inStmt);
  bool        prepareNewStatement(std::string inName, std::string inSql);

  
  bool start_transaction(); // begin
  bool end_transaction();   // commit
  bool bind_stmt(sqlite3_stmt* inStmt, missionx::db_types inType, int indx, std::string inValue);
  bool bind_to_stored_stmt(std::string stmt_key, missionx::db_types inType, int indx, std::string inValue);
  bool bind_and_execute_ins_stmt(std::string stmt_key, std::string in_table_name, const std::list<missionx::db_field> in_map_colValTypes);

  int  step(sqlite3_stmt* inStmt);

  // dynamic rows read into an unordered map.
  // YUou must provide exact column mapping, like: std::map<int, std::string> = { { 0, "distance" }, { 1, "icao_id" }, { 2, "icao" }, { 3, "ap_name" }, { 4, "ap_lat" }, { 5, "ap_lon" } }
  // >> Returns <<
  // Return true if all went well, and a map of index + rows.
  // All rows are maps of col,value in string format.
  bool step_and_write_rows_into_map(sqlite3_stmt*                                                inStmt,
                                    std::map<int, std::string>                                   inColumnMapping,
                                    std::unordered_map<int, std::map<std::string, std::string>>& outRows,
                                    std::string                                                & outErr,
                                    int                                                          inMaxRowsToFetch = missionx::MAX_ALLOWED_ROWS_TO_FETCH); // v3.303.14 will execute a prepared statement and will step over all rows while writing all fields into an unordered map but as "strings"
  bool clear_and_reset(sqlite3_stmt* inStmt);

  bool getIsStatement_withTheName_exists(const std::string& inStmtName);

  bool attach_database(const std::string inDatabaseLocation, const std::string inName);
  bool detach_database(const std::string inName = "dbAttached");

private:
};

} // end namespace missionx

#endif // DBASEH_
