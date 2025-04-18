#include "dbase.h"
#include "../core/Utils.h"
#include "../../libs/sqlite/sqlite/extension-functions.h"

namespace missionx
{



// auto dbase::make_sqlite3_handle(char const* db_name)
//{
//  sqlite3* p;
//  int rc = sqlite3_open(db_name, &p);
//  sqlite3_handle h{ p };
//  if (rc) h.reset();
//  return h;
//}

int
dbase::callback(void*, int argc, char** argv, char** col_names)
{
  // for (int i = 0; i < argc; ++i)
  //  cout << col_names[i] << " = " << (argv[i] ? argv[i] : "NULL") << '\n';
  // cout << endl;
  return 0;
}
// ----------------------------------------------------
// ----------------------------------------------------
// ----------------------------------------------------

dbase::dbase()
{
  db = nullptr;
  this->db_is_open_and_ready = false;
  memset(sql, '\0', sizeof(sql));

  this->rc = 0;
  this->seq_ins_stats = 0;
  this->stmt = nullptr;
  this->zErr = nullptr;
  this->zTail= nullptr;
  this->zTail_ins_stats = nullptr;
}

// ----------------------------------------------------

dbase::~dbase()
{
  if (this->db != nullptr)
  {
    if (db_is_open_and_ready)
      close_database();
  }
}

// ----------------------------------------------------

bool
dbase::open_database()
{
  this->last_err.clear();

  if (db != nullptr)
  {
    this->db_is_open_and_ready = true;
    return true;
  }

  if (this->absolute_path_to_db_file_s.empty())
  {
    this->last_err = "Bad database file path.\n";
    Log::logMsgThread(last_err);
    db_is_open_and_ready = false;
    return false;
  }

  this->rc = sqlite3_open(this->absolute_path_to_db_file_s.c_str(), &this->db);
  if (this->rc != SQLITE_OK)
  {
    this->last_err = "Failed to open / create database: '" + this->absolute_path_to_db_file_s + "'\nERROR: " + sqlite3_errmsg(this->db);
    Log::logMsgThread(this->last_err + "\n");

    db_is_open_and_ready = false;
    return false;
  }
  else
  {
    db_is_open_and_ready = true;
    mx_RegisterExtensionFunctions(db); // v3.0.241.10 added the extension function to the SQLite engine so we can use all the mathematical "gooddies" function in our query, instead of coding them in C++
  }

  return true;
}

// ----------------------------------------------------

bool
dbase::open_database_in_memory()
{
  this->last_err.clear();
  this->absolute_path_to_db_file_s = ":memory:";

  if (db != nullptr)
  {
    this->db_is_open_and_ready = true;
    return true;
  }

  this->rc = sqlite3_open(":memory:", &this->db);
  if (this->rc != SQLITE_OK)
  {
    this->last_err = "Failed to create database in :memory:'\nERROR: " + std::string(sqlite3_errmsg(this->db));
    Log::logMsgThread(this->last_err + "\n");

    db_is_open_and_ready = false;
    return false;
  }
  else
  {
    db_is_open_and_ready = true;
    mx_RegisterExtensionFunctions(db); // v3.0.241.10 added the extension function to the SQLite engine so we can use all the mathematical "gooddies" function in our query, instead of coding them in C++
  }

  return true;
}

// ----------------------------------------------------

bool
dbase::remove_database(std::string in_absolute_path)
{
  this->last_err.clear();

  // delete original file and create a new one
  if (remove(in_absolute_path.c_str()) != 0) // try to remove old missionx stats database
  {
    this->last_err = "Fail to find and remove the file: " + in_absolute_path;
    Log::logMsgWarn(this->last_err, true);
    return false;
  }

  Log::logMsgThread(std::string("Deleted file: ") + in_absolute_path + ".  Preparing new one.!!!");

  return true;
}

// ----------------------------------------------------

bool
dbase::close_database()
{
  if (this->db != nullptr && this->db_is_open_and_ready)
  {
    for (auto& prep : this->mapStatements)
    {
      if (prep.second != nullptr)
        sqlite3_finalize(prep.second);
    }
    rc = sqlite3_close_v2(this->db);

    db_is_open_and_ready = false;
    this->mapStatements.clear();
    this->db = nullptr; // v3.303.8.3
  }

  return (rc == SQLITE_OK) ? true : false;
}

// ----------------------------------------------------


bool
dbase::execute_stmt(std::string inStmt)
{
  this->last_err.clear();
  if (!db_is_open_and_ready)
  {
    this->last_err = "Database is not ready.";
    return false;
  }


  size = snprintf(sql, inStmt.length() + 1, "%s", inStmt.c_str());
  // execute DDL command
  char* error_msg{};

  rc = sqlite3_exec(this->db, sql, 0, 0, &error_msg);
  if (rc != SQLITE_OK)
  {
    this->last_err = std::string("[DBASE - ERROR] STMT Error: ") + sqlite3_errmsg(this->db) + "\n" + sql + "\n";
    Log::logMsgThread(this->last_err);
    return false;
  }

  return true;
}

// ----------------------------------------------------

bool
dbase::execute_stmt(const char* inStmt)
{
  this->last_err.clear();
  if (!db_is_open_and_ready)
  {
    this->last_err = "Database is not ready.";
    return false;
  }

  char* error_msg{};

  rc = sqlite3_exec(this->db, inStmt, 0, 0, &error_msg);
  if (rc != SQLITE_OK)
  {
    this->last_err = std::string("[DBASE - ERROR] STMT Error: ") + sqlite3_errmsg(this->db) + "\n" + inStmt + "\n";
    Log::logMsgThread(this->last_err);
    return false;
  }

  return true;
}

// ----------------------------------------------------

bool
dbase::prepareNewStatement(std::string inName, std::string inSql)
{
  this->last_err.clear();
  if (!db_is_open_and_ready)
  {
    this->last_err = "Database is not ready.";
    Log::logMsgErr("[prepNewStmt] Fail creating new statment. " + this->last_err, true);
    return false;
  }

  if (inName.empty())
  {
    this->last_err = "Statement by the name: '" + inName + "' might already be present or name is not valid.\n";
    Log::logMsgErr("[prepNewStmt] " + this->last_err, true);
    return false;
  }

  if (Utils::isElementExists(this->mapStatements, inName))
  {
    this->last_err = "Statement by the name: '" + inName + "' is already present. Will override with new pointer.\n";
    Log::logMsgWarn("[prepNewStmt] " + this->last_err, true);

    sqlite3_clear_bindings(this->mapStatements[inName]);
    sqlite3_reset(this->mapStatements[inName]);

    // not duing "sqlite3_finalize(this->mapStatements[inName]);" since it will crash the plugin
    this->mapStatements[inName] = NULL;

  }

  // Prepare the new statement and store it in mapStatements[] if all is well
  sqlite3_stmt* newStmt_ptr = nullptr;
  rc = sqlite3_prepare_v2(this->db, inSql.c_str(), (int)inSql.size(), &newStmt_ptr, &zTail_ins_stats);
  if (rc != SQLITE_OK)
  {
    this->last_err = std::string("[prepNewStmt] Prepare statement failed. Error: ") + sqlite3_errmsg(this->db) + "\nStatement: " + inSql + "\n";
    Log::logMsgErr(this->last_err, true);

    this->mapStatements.erase(inName); // remove from container

    return false;
  }
  else
  {
    this->mapStatements[inName] = newStmt_ptr;
  }

  return true;
}

// ----------------------------------------------------

bool
dbase::start_transaction()
{
  this->last_err.clear();
  rc = sqlite3_exec(this->db, "BEGIN TRANSACTION", NULL, NULL, &zErr);
  if (rc == SQLITE_OK)
    return true;

  this->last_err = std::string("[DB] Error while begin transaction: ") + sqlite3_errmsg(this->db) + "\n";
  Log::logMsg(this->last_err, true);
  return false;
}

// ----------------------------------------------------

bool
dbase::end_transaction()
{
  this->last_err.clear();

  rc = sqlite3_exec(this->db, "END TRANSACTION", NULL, NULL, &zErr);
  if (rc == SQLITE_OK)
    return true;

  this->last_err = std::string("[DB] Error while commiting data: ") + sqlite3_errmsg(this->db) + "\n";
  Log::logMsg(this->last_err, true);

  return false;
}

// ----------------------------------------------------

bool
dbase::bind_stmt(sqlite3_stmt* stmt_ptr, missionx::db_types inType, int indx, std::string inValue)
{
  this->last_err.clear();

  if (stmt_ptr != nullptr && this->db_is_open_and_ready)
  {

    assert(stmt_ptr != nullptr && "statement pointer can't be null");


    switch (inType)
    {
      case missionx::db_types::real_typ:
      case missionx::db_types::double_typ:
      case missionx::db_types::float_typ:
      {
        rc = sqlite3_bind_double(stmt_ptr, indx, Utils::stringToNumber<double>(inValue, (int)inValue.length()));
      }
      break;
      case missionx::db_types::int_typ:
      {
        rc = sqlite3_bind_int(stmt_ptr, indx, Utils::stringToNumber<int>(inValue));
      }
      break;
      case missionx::db_types::text_typ:
      {
        rc = sqlite3_bind_text(stmt_ptr, indx, inValue.c_str(), (int)inValue.size(), SQLITE_TRANSIENT);
      }
      break;
      case missionx::db_types::null_typ:
      {
        rc = sqlite3_bind_null(stmt_ptr, indx);
      }
      break;
      default:
        break;
    }

    if (rc == SQLITE_OK)
      return true;
  }


  return false;
}

// ----------------------------------------------------

bool
dbase::bind_to_stored_stmt(std::string stmt_key, missionx::db_types inType, int indx, std::string inValue)
{
  this->last_err.clear();

  if (mxUtils::isElementExists(this->mapStatements, stmt_key))
  {
    sqlite3_stmt* stmt_ptr = this->mapStatements[stmt_key];

    assert(stmt_ptr != nullptr);

    if (stmt_ptr)
    {
      switch (inType)
      {
        case missionx::db_types::real_typ:
        case missionx::db_types::double_typ:
        case missionx::db_types::float_typ:
        {
          rc = sqlite3_bind_double(stmt_ptr, indx, Utils::stringToNumber<double>(inValue, (int)inValue.length()));
        }
        break;
        case missionx::db_types::int_typ:
        {
          rc = sqlite3_bind_int(stmt_ptr, indx, Utils::stringToNumber<int>(inValue));
        }
        break;
        case missionx::db_types::text_typ:
        {
          rc = sqlite3_bind_text(stmt_ptr, indx, inValue.c_str(), (int)inValue.size(), SQLITE_TRANSIENT);
        }
        break;
        case missionx::db_types::null_typ:
        {
          rc = sqlite3_bind_null(stmt_ptr, indx);
        }
        break;
        default:
          break;
      }

      if (rc == SQLITE_OK)
        return true;
    }
  }
  else
  {
    Log::logMsgThread("[db bind_stmt] No prepared statement by the name: " + stmt_key + ", was found. Please notify the developer !!!");
  }


  return false;
}

// ----------------------------------------------------

bool
dbase::bind_and_execute_ins_stmt(std::string stmt_key, std::string in_table_name, const std::list<missionx::db_field> in_map_colValTypes)
{

  this->last_err.clear();
  std::string debug_value_helper;

  if (!mxUtils::isElementExists(this->mapStatements, stmt_key))
  {

    // create new statement and prepare it
    const auto lmbda_prepare_insert_header_and_body = [](std::list<missionx::db_field> in_row_colValTypes, std::string& outValueLine) {
      std::string cols_header = "(";
      outValueLine            = "(";

      unsigned int counter = 1;

      for (auto& row : in_row_colValTypes)
      {
        if (counter > 1)
        {
          cols_header.append(", ");
          outValueLine.append(", ");
        }
        cols_header.append(row.col_name);
        outValueLine.append("?");

        ++counter;
      }
      cols_header.append(")");
      outValueLine.append(")");

      return cols_header;
    };

    std::string       ins_body;
    const auto        inst_header = lmbda_prepare_insert_header_and_body(in_map_colValTypes, ins_body);
    const std::string sql         = "insert into " + in_table_name + inst_header + " values " + ins_body;

#ifndef RELEASE
    // Log::logMsgThread("\nSQL: " + sql + "\n\n");
#endif
    sqlite3_stmt* stmt;
    this->rc = sqlite3_prepare_v2(this->db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
#ifndef RELEASE
      this->last_err = std::string("[DB] Error preparing sql: ") + sql + "\n\t" + sqlite3_errmsg(this->db) + "\n";
      Log::logMsg(this->last_err, true);
#endif // !RELEASE

      return false;
    }


    Utils::addElementToMap(this->mapStatements, stmt_key, stmt);

  } // end making the statement and preparing it ahead of time



  if (mxUtils::isElementExists(this->mapStatements, stmt_key))
  {
    sqlite3_stmt* stmt_ptr = this->mapStatements[stmt_key];

    // loop over all columns and bind them
    auto it     = in_map_colValTypes.cbegin();
    auto it_end = in_map_colValTypes.cend();
    for (int indx = 1; it != it_end; ++it, ++indx)
    {
      const missionx::db_types field_type = it->dataype; // datatype
      const std::string        value      = it->value_s; // value
      debug_value_helper += ((indx == 1) ? value : ", " + value);
      if (stmt_ptr)
      {
        switch (field_type)
        {
          case missionx::db_types::real_typ:
          case missionx::db_types::double_typ:
          case missionx::db_types::float_typ:
          {
            if (value.empty())
              goto INSERT_NULL;

            this->rc = sqlite3_bind_double(stmt_ptr, indx, Utils::stringToNumber<double>(value, (int)value.length()));
          }
          break;
          case missionx::db_types::int_typ:
          {
            if (value.empty())
              goto INSERT_NULL;
            this->rc = sqlite3_bind_int(stmt_ptr, indx, Utils::stringToNumber<int>(value));
          }
          break;
          case missionx::db_types::text_typ:
          {
            this->rc = sqlite3_bind_text(stmt_ptr, indx, value.c_str(), (int)value.size(), SQLITE_TRANSIENT);
          }
          break;
          case missionx::db_types::null_typ:
          {
          INSERT_NULL:
            this->rc = sqlite3_bind_null(stmt_ptr, indx);
          }
          break;
          case missionx::db_types::zero_type:
          {
            this->rc = sqlite3_bind_int(stmt_ptr, indx, 0);
          }
          break;
          default:
            break;
        }

        if (!(this->rc == SQLITE_OK))
        {
          this->last_err = std::string("[DB] Error binding col: ") + it->col_name + " with value: " + it->value_s + "\n\t" + sqlite3_errmsg(this->db) + "\n";
          Log::logMsg(this->last_err, true);
          return false;
        }
      } // end if stmt pointer is valid

    } // end loop over row fields and bind all values to their types

    if (this->rc == SQLITE_OK && stmt_ptr)
    {
      rc = sqlite3_step(stmt_ptr); // execute the statement
      //if (!(this->rc == SQLITE_OK))
      //{
      //  this->last_err = std::string("[DB] Error Executing stmt: ") + sqlite3_errmsg(this->db) + "\nsql: " + sql;
      //  Log::logMsg(this->last_err, true);
      //}
    }

    rc = sqlite3_clear_bindings(stmt_ptr); // clear binding

    if (sqlite3_reset(stmt_ptr) != SQLITE_OK) // reset stmt pointer
    {
#ifndef RELEASE
      // Log::logMsgThread(std::string("ERROR: [db exec error] reset: ") + sqlite3_errmsg(this->db) + "\t (" + debug_value_helper + ")\n");
#endif // !RELEASE

      return false;
    }

    return true;
  }

  return false;
}



// ----------------------------------------------------

int
dbase::step(sqlite3_stmt* inStmt)
{
  this->last_err.clear();

  return sqlite3_step(inStmt);
}

// ----------------------------------------------------

bool
dbase::step_and_write_rows_into_map(sqlite3_stmt* inStmt, std::map<int, std::string> inColumnMapping, std::unordered_map<int, std::map<std::string, std::string>>& outRows, std::string& outErr, int inMaxRowsToFetch)
{
  assert(inStmt != nullptr && "Prepared statement object is not initialized!!!");

  if (inStmt == nullptr)
    return false;

  bool flagContinueIterateRows = true;
  outRows.clear();

  std::map<std::string, std::string> row_data;

  while (flagContinueIterateRows && inMaxRowsToFetch)
  {
    auto rc = this->step(inStmt);
    if (rc == SQLITE_ROW)
    {
      for (const auto &[colNumber, colName] : inColumnMapping)
      {
        row_data[colName] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(inStmt, colNumber)));
      }
      outRows[(int)outRows.size()] = row_data;
      row_data.clear();
    }
    else // end of row iteration
    {
      flagContinueIterateRows = false;
      if (rc < SQLITE_ROW) // check for errors
      {
        outErr = sqlite3_errmsg(this->db);
        return false;
      }
    }
      
    inMaxRowsToFetch--;
  } // end step
  

  return true;
}

// ----------------------------------------------------

bool
dbase::clear_and_reset(sqlite3_stmt* inStmt)
{
  this->last_err.clear();
  if (sqlite3_clear_bindings(inStmt) != SQLITE_OK)
  {
    this->last_err = std::string("[DB] Error while clearing bindings: ") + sqlite3_errmsg(this->db) + "\n";
    Log::logMsg(this->last_err, true);
  }


  if (sqlite3_reset(inStmt) != SQLITE_OK)
  {
    this->last_err = std::string("[DB] Error while reset statement: ") + sqlite3_errmsg(this->db) + "\n";
    Log::logMsg(this->last_err, true);
  }

  return true;
}

// ----------------------------------------------------

bool
dbase::getIsStatement_withTheName_exists(const std::string& inStmtName)
{

  return mxUtils::isElementExists(this->mapStatements, inStmtName);
}

// ----------------------------------------------------


bool
dbase::attach_database(const std::string inDatabaseLocation, const std::string inName)
{
  char* error_msg{};
  if (this->db_is_open_and_ready && !inDatabaseLocation.empty() && !inName.empty())
  {
    std::string query = "ATTACH DATABASE '" + inDatabaseLocation + "' as " + inName;
    if (sqlite3_exec(db, query.c_str(), 0, 0, &error_msg) != SQLITE_OK)
    {
      this->last_err = std::string("[DBASE - ERROR] ATTACH Error: ") + sqlite3_errmsg(this->db) + "\n";
      Log::logMsgThread(this->last_err);
      return false;
    }

    return true;
  }

  return false;
}

// ----------------------------------------------------

bool
dbase::detach_database(const std::string inName)
{
  char* error_msg{};

  if (db_is_open_and_ready)
  {
    std::string query = "DETACH DATABASE '" + inName + "'";
    if (sqlite3_exec(db, query.c_str(), 0, 0, &error_msg) != SQLITE_OK)
    {
      this->last_err = std::string("[DBASE - ERROR] DETACH Error: ") + sqlite3_errmsg(this->db) + "\n";
      Log::logMsgThread(this->last_err);
      return false;
    }

    return true;
  }
  return false;
}

// ----------------------------------------------------

} // end namespace mission
