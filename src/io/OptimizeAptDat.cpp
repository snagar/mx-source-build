#ifdef MAC
#if __has_include(<filesystem>) && (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif

#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif
#else // Linux and IBM
#include <filesystem>
namespace fs = std::filesystem;
#endif 

#include "OptimizeAptDat.h"

#include <cctype>
#include <chrono>
#include <fstream>

#include "../core/data_manager.h"
//#include "dbase.h"
#include "system_actions.h" // v3.303.14

#define SKIPGLOBALAIRPORTS 0

namespace missionx
{

base_thread::thread_state OptimizeAptDat::aptState;
std::thread               OptimizeAptDat::thread_ref;

missionx::dbase* OptimizeAptDat::db_airports_xp_ptr;
missionx::dbase* OptimizeAptDat::db_airports_cache_ptr;
}

// ---------------------------------------------------------
// ---------------------------------------------------------
// ---------------------------------------------------------



missionx::OptimizeAptDat::OptimizeAptDat()
{
  OptimizeAptDat::db_airports_xp_ptr    = nullptr;
  OptimizeAptDat::db_airports_cache_ptr = nullptr;
}

// ---------------------------------------------------------

missionx::OptimizeAptDat::~OptimizeAptDat() {}

// ---------------------------------------------------------

void
missionx::OptimizeAptDat::setAptDataFolders(std::string inCustomScenery, std::string inDefaultAptDatFolder, std::string inMissionxFolder)
{
  Utils::addElementToMap(OptimizeAptDat::aptState.mapValues, mxconst::get_FLD_CUSTOM_SCENERY_FOLDER_PATH(), inCustomScenery);
  Utils::addElementToMap(OptimizeAptDat::aptState.mapValues, mxconst::get_FLD_DEFAULT_APTDATA_PATH(), inDefaultAptDatFolder); /// Resources/default scenery/default apt dat/Earth nav data/
  Utils::addElementToMap(OptimizeAptDat::aptState.mapValues, mxconst::get_PROP_MISSIONX_PATH(), inMissionxFolder);
}

// ---------------------------------------------------------

void
missionx::OptimizeAptDat::set_database_pointers(missionx::dbase* in_db_airports_xp)
{
  if (OptimizeAptDat::db_airports_xp_ptr == nullptr)
    OptimizeAptDat::db_airports_xp_ptr = in_db_airports_xp;
}

// ---------------------------------------------------------

void
missionx::OptimizeAptDat::set_database_pointers2(missionx::dbase* in_db_airports_xp)
{
  if (OptimizeAptDat::db_airports_cache_ptr == nullptr)
    OptimizeAptDat::db_airports_cache_ptr = in_db_airports_xp;
}

// ---------------------------------------------------------

void
missionx::OptimizeAptDat::exec_optimize_aptdat_thread()
{

  if (missionx::OptimizeAptDat::aptState.flagIsActive)
    return;

  //  // start thread
  if (!OptimizeAptDat::aptState.flagIsActive)
  {
    if (OptimizeAptDat::thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
      OptimizeAptDat::thread_ref.join();       // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"

    missionx::data_manager::cachedNavInfo_map.clear();

    OptimizeAptDat::thread_ref = std::thread(&OptimizeAptDat::read_and_parse_all_apt_dat_files, this, &OptimizeAptDat::aptState);
  }
}

// ---------------------------------------------------------

bool
missionx::OptimizeAptDat::read_and_parse_all_apt_dat_files(thread_state* inThreadState)
{
  ///////////////////////////////////////////////
  // REMEMBER that every new table needs respective copy command from memory db to the file db. Found at the end of "read_and_parse_all_apt_files() function.
  ///////////////////////////////////////////////


  double       duration         = 0.0;
  unsigned int lineCounter      = 0; // count the lines written to file
  auto         startThreadClock = std::chrono::steady_clock::now();

  inThreadState->flagIsActive        = true;
  inThreadState->flagThreadDoneWork = false;
  inThreadState->flagAbortThread     = false;

  inThreadState->startThreadStopper();

  assert(OptimizeAptDat::db_airports_cache_ptr != nullptr && "[OPT Thread] Cached DB is not set correctly");
  OptimizeAptDat::prepare_sqlite_db_tables(inThreadState, OptimizeAptDat::db_airports_cache_ptr); // Initialize in memory database
  this->icao_id_counter = 0;


  // Custom Scenery Folder
  fs::path path = inThreadState->mapValues[mxconst::get_FLD_CUSTOM_SCENERY_FOLDER_PATH()];
  if (fs::directory_entry(path).is_directory()) // if path is not a directory
  {
    const std::string customIniFile     = inThreadState->mapValues[mxconst::get_FLD_CUSTOM_SCENERY_FOLDER_PATH()] + missionx::XPLANE_SCENERY_INI_FILENAME;
    const std::string defaultAptDatFile = inThreadState->mapValues[mxconst::get_FLD_DEFAULT_APTDATA_PATH()] + mxconst::get_FOLDER_SEPARATOR() + "apt.dat";
    const std::string customAptDat      = inThreadState->mapValues[mxconst::get_PROP_MISSIONX_PATH()] + mxconst::get_CUSTOM_APT_DAT_FILE();
    const int         xplane_ver_i      = mxUtils::stringToNumber<int>(inThreadState->mapValues[mxconst::get_PROP_XPLANE_VERSION()]);

    std::ifstream infs;
    std::ofstream outCustAptdatFile;

    /// prepare file streams ///
    // prepare out file: CUSTOM_APT_DAT_FILE.txt file
    std::ios_base::sync_with_stdio(false); // v3.0.219.10
    std::cin.tie(nullptr);                 // v3.0.219.10

    outCustAptdatFile.open(customAptDat.c_str()); // can we create/open the file ?
    if (outCustAptdatFile.fail())
    {
      Log::logAttention((std::string("Fail to create file: ") + customAptDat + "\n"));
      return false;
    }


    // prepare reading file: "custom_scenery.ini" file
    infs.open(customIniFile.c_str(), std::ios::in); // can we read the file
    if (!infs.is_open())
    {
      Log::logAttention((std::string("[parse Custom ini] Fail to open file: ") + customIniFile + "\n"), true);
      return false;
    }

    //// read each line and search for string "SCENERY_PACK ", the space in the end is important
    std::string line;
    inThreadState->counter = 1; // reset line split_fields_counter_i before reading all apt.dat files    

    while ((getline(infs, line)) && !(inThreadState->flagAbortThread))
    {

      if (line.find(missionx::SCENERY_PACK_) == 0) // if line starts with "SCENERY_PACK " then should be valid
      {
        const std::string scenery_pack          = line.substr(std::string_view(missionx::SCENERY_PACK_).length());

        auto const lmbda_get_apt_dat_path = [](std::string inSceneryPack, const int inXplaneVersion, std::string inDefaultAptDatFile)
        {
          if (inXplaneVersion < 12000)
          {
            return inSceneryPack + "Earth nav data/apt.dat"; // XP11
          }
          else
          {
            if (inSceneryPack.find("*GLOBAL_AIRPORTS*") == std::string::npos)
              return inSceneryPack + "Earth nav data/apt.dat";
            else
              return inDefaultAptDatFile;
          }

          return std::string("");
        };

        std::string relative_apt_dat_path = lmbda_get_apt_dat_path(scenery_pack, xplane_ver_i, defaultAptDatFile); // v3.303.8

        #if (SKIPGLOBALAIRPORTS == 1)
        if (line.find("Global Airports") != std::string::npos) // v3.0.255.3 skip for debug
          continue;
        #endif

        #ifndef RELEASE
        Log::logAttention(std::string("parsing: ") + relative_apt_dat_path, true);
        const auto start = std::chrono::steady_clock::now();
        #endif

        fs::path path_to_aptDat = relative_apt_dat_path;
        if (fs::directory_entry(path_to_aptDat).is_regular_file())
        {
          /// PARSE FILE
          OptimizeAptDat::db_airports_cache_ptr->start_transaction();                                                                                                                               // v3.0.255.3
          missionx::OptimizeAptDat::parse_aptdat(inThreadState, relative_apt_dat_path, scenery_pack, false, ((relative_apt_dat_path.find("Global Airports") == std::string::npos) ? true : false)); // if "find" returned "npos", return "true" = custom scenery
          OptimizeAptDat::db_airports_cache_ptr->end_transaction();                                                                                                                                 // v3.0.255.3
        }
        else 
        {
          Log::logMsgThread("\t[File Not Found] \"apt.dat\" does not exists in [" + scenery_pack + "], skipping... !!!");
        }



        #ifndef RELEASE
        auto end  = std::chrono::steady_clock::now();
        auto diff = end - start;
        duration  = std::chrono::duration<double, std::milli>(diff).count();
        Log::logAttention("Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec), for: " + relative_apt_dat_path, true);
        #endif // !RELEASE
      }

    }

    /// close custom ini file
    if (infs.is_open())
      infs.close();

    ///// Flush to disk
    {
      auto start = std::chrono::steady_clock::now();
      for (auto &[airport_code, info] : missionx::data_manager::cachedNavInfo_map)
      {
        bool bWroteNav = false;
        ++lineCounter;
        for (auto nav : info.listNavInfo) // flush data into optimized apt.dat
        {
          // lambda to find first space TODO: make a function out of this
          const auto lmbda_get_first_space_in_line = [](std::string& inLine)
          {
            // loop until you find first space
            int i = 0;
            for (auto c : inLine)
            {
              if (c == ' ')
                return i;

              ++i;
            }

            return 0;
          };
          auto space_location_i = lmbda_get_first_space_in_line(nav);
          const std::string code_s = mxUtils::rtrim(nav.substr(0, space_location_i));
          if (code_s.compare("100") == 0) // do not store runway info
            continue;
          else
          {
            outCustAptdatFile << nav;
            ++lineCounter;
            bWroteNav = true;
          }
        }
        if (info.isCustom && bWroteNav) // * at the end of the NavAid means custom airport
          outCustAptdatFile << "*"
                            << "\n";

        outCustAptdatFile << '\n';
      }

      auto end  = std::chrono::steady_clock::now();
      auto diff = end - start;
      duration  = std::chrono::duration<double, std::milli>(diff).count();
      Log::logAttention("Flush to Disk Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec). Lines Written:  " + Utils::formatNumber<int>(lineCounter) + "\n", true);
    }


    //// Close files
    if (outCustAptdatFile.is_open())
      outCustAptdatFile.close();

    if (infs.is_open())
      infs.close();


    auto endCacheLoad = std::chrono::steady_clock::now();
    auto diff_cache   = endCacheLoad - startThreadClock;
    duration          = std::chrono::duration<double, std::milli>(diff_cache).count();
    Log::logAttention("*** Finish Parsing APT.DAT files. Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec)  ****", true);



    // v3.0.241.10 // v3.0.253.3
    std::string err;
    //bool        write_to_db_b = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::SETUP_WRITE_CACHE_TO_DB, true);

    if ( OptimizeAptDat::db_airports_cache_ptr != nullptr) // v3.303.14 deprecated write_to_db_b - always true, always write to DB
    {
      Log::logMsgThread(">>> Finished Database load <<<\n");
      upload_navdata_to_inMemory_db(inThreadState, *OptimizeAptDat::db_airports_cache_ptr);


      // v3.0.255.3 Copy all information to the target database
      const std::string attachName = "dbTarget";
      if (OptimizeAptDat::db_airports_cache_ptr->db_is_open_and_ready && OptimizeAptDat::db_airports_xp_ptr->db_is_open_and_ready)
      {
        this->prepare_sqlite_db_tables(inThreadState, OptimizeAptDat::db_airports_xp_ptr); // prepare the local file database tables


        if (OptimizeAptDat::db_airports_cache_ptr->attach_database(OptimizeAptDat::db_airports_xp_ptr->get_db_path(), attachName))
        {
          OptimizeAptDat::db_airports_cache_ptr->start_transaction();

          OptimizeAptDat::db_airports_cache_ptr->execute_stmt("CREATE INDEX icao_id_metadata_n1 on xp_ap_metadata (icao_id);"); // create index on metadata icao_id column

          // update all airports lat/lon based on metadata: "datum_lat" and "datum_lon" values, if any
          const std::string sql = R"(update xp_airports 
                                   set ap_lat = IFNULL( (select  val_col from xp_ap_metadata where xp_airports.icao_id = xp_ap_metadata.icao_id and xp_ap_metadata.key_col = 'datum_lat' limit 1), xp_airports.ap_lat )
                                     , ap_lon = IFNULL( (select  val_col from xp_ap_metadata where xp_airports.icao_id = xp_ap_metadata.icao_id and xp_ap_metadata.key_col = 'datum_lon' limit 1), xp_airports.ap_lon ) 
                                  )";
          OptimizeAptDat::db_airports_cache_ptr->execute_stmt(sql);


          // Copy all tables from in memory to the external file
          std::string copyStmt = "INSERT INTO " + attachName + ".xp_airports SELECT * FROM xp_airports";
          OptimizeAptDat::db_airports_cache_ptr->execute_stmt(copyStmt);

          copyStmt = "INSERT INTO " + attachName + ".xp_ap_ramps SELECT * FROM xp_ap_ramps";
          OptimizeAptDat::db_airports_cache_ptr->execute_stmt(copyStmt);

          copyStmt = "INSERT INTO " + attachName + ".xp_helipads SELECT * FROM xp_helipads";
          OptimizeAptDat::db_airports_cache_ptr->execute_stmt(copyStmt);

          copyStmt = "INSERT INTO " + attachName + ".xp_rw SELECT * FROM xp_rw";
          OptimizeAptDat::db_airports_cache_ptr->execute_stmt(copyStmt);

          copyStmt = "INSERT INTO " + attachName + ".xp_loc SELECT * FROM xp_loc";
          OptimizeAptDat::db_airports_cache_ptr->execute_stmt(copyStmt);

          copyStmt = "INSERT INTO " + attachName + ".xp_ap_metadata SELECT * FROM xp_ap_metadata";
          OptimizeAptDat::db_airports_cache_ptr->execute_stmt(copyStmt);

          copyStmt = "INSERT INTO " + attachName + ".xp_ap_frq SELECT * FROM xp_ap_frq"; // v24.03.1
          OptimizeAptDat::db_airports_cache_ptr->execute_stmt(copyStmt);



          OptimizeAptDat::db_airports_cache_ptr->end_transaction();

          if (OptimizeAptDat::db_airports_cache_ptr->detach_database(attachName))
          {
            Log::logMsgThread("[opt copy target] Detached database: " + attachName);
          }


          missionx::OptimizeAptDat::sqlite_post_tables_copy(OptimizeAptDat::db_airports_xp_ptr);
        }

        // clear memory sqlite cahced db
        OptimizeAptDat::prepare_sqlite_db_tables(inThreadState, OptimizeAptDat::db_airports_cache_ptr); // Release memory with reconstructing all tables that hold the information
      }


      // v3.303.14 Fix airport ICAO names that are not the same as their metadata ones. We do not fix the xp_ap_metadata.icao field in order to keep the original values as a reference
      // Must come after the in memory cach flush itself so it won't override this fix statement.
      const std::string update_stmt_fix_xp_airports_icao = R"(
update xp_airports
set icao = (
                select t1.val_col
                from xp_ap_metadata t1
                where t1.key_col = 'icao_code'
                and t1.icao != t1.val_col
                and t1.icao_id = xp_airports.icao_id
          )
where 1 = 1
and xp_airports.icao != (select val_col
                         from  xp_ap_metadata t2
                         where t2.key_col = 'icao_code'
                         and t2.icao != t2.val_col
                         and t2.icao_id = xp_airports.icao_id
                         )     
    )";


      // v3.305.3 Remove metadata icao_id that is not in xp_airports
      const std::string remove_metadata_icaoid_not_in_xp_airports_table = R"(
delete from xp_ap_metadata
where icao_id in (
select icao_id
from xp_ap_metadata
EXCEPT
select icao_id
from xp_airports
)
    )";


      // Fix ICAOs that are not the same as xp_airports.icaos. Must come after the "update_stmt_fix_xp_airports_icao" fix.
      const std::string update_stmt_fix_metadata_icao = R"(
update xp_ap_metadata
set icao = (
                select t1.icao
                from xp_airports t1
                where t1.icao_id = xp_ap_metadata.icao_id
                and t1.icao != xp_ap_metadata.icao
          )
where  xp_ap_metadata.icao not in (select t1.icao
                                   from  xp_airports t1        
                                   where t1.icao_id = xp_ap_metadata.icao_id)
    )";



      if (OptimizeAptDat::db_airports_xp_ptr != nullptr && OptimizeAptDat::db_airports_xp_ptr->db_is_open_and_ready)
      {
          OptimizeAptDat::db_airports_xp_ptr->start_transaction();
          OptimizeAptDat::db_airports_xp_ptr->execute_stmt(update_stmt_fix_xp_airports_icao);
          OptimizeAptDat::db_airports_xp_ptr->end_transaction();

          // v3.305.3 fix cases where metadata table has icao_id not in xp_airports, this fixes NOT NULL constraint when running "update_stmt_fix_metadata_icao".
          OptimizeAptDat::db_airports_xp_ptr->start_transaction();
          OptimizeAptDat::db_airports_xp_ptr->execute_stmt(remove_metadata_icaoid_not_in_xp_airports_table);
          OptimizeAptDat::db_airports_xp_ptr->end_transaction();

          OptimizeAptDat::db_airports_xp_ptr->start_transaction();
          OptimizeAptDat::db_airports_xp_ptr->execute_stmt(update_stmt_fix_metadata_icao);
          OptimizeAptDat::db_airports_xp_ptr->end_transaction();

      }



      auto endThreadClock = std::chrono::steady_clock::now();
      auto diff           = endThreadClock - startThreadClock;
      duration            = std::chrono::duration<double, std::milli>(diff).count();

      Log::logAttention("*** Finish Parsing and Loading APT.DAT + ILS data into the database. Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec)  ****", true);
    }

    inThreadState->flagIsActive        = false;
    inThreadState->flagThreadDoneWork = true; // we reset the thread at Mission::flc_aptdat() function
  }
  else
  {
    Log::logAttention("[Parse Custom ini] Failed to open information folder: " + inThreadState->mapValues[mxconst::get_FLD_CUSTOM_SCENERY_FOLDER_PATH()], true);
    return false; // skip
  }


  return true;
}



// ---------------------------------------------------------



bool
missionx::OptimizeAptDat::parse_aptdat(thread_state* inThreadState, std::string& relative_apt_dat_path, const std::string scenery_pack_name, bool inReadCachedFile, const bool is_custom)
{
  std::ifstream      file_aptDat;
  std::istringstream stringStream;

  std::ios_base::sync_with_stdio(false); // v3.0.219.10
  std::cin.tie(nullptr);                 // v3.0.219.10

  if (inThreadState->flagAbortThread)
    return false;


  // prepare reading file: "custom_scenery.ini" file
  file_aptDat.open(relative_apt_dat_path.c_str(), std::ios::in); // can we read the file
  if (!file_aptDat.is_open())
  {
    Log::logAttention(std::string("[Fail parse aptdat] Fail to open file: ") + relative_apt_dat_path, true);

    return false;
  }

  // read line by line and copy only lines with codes: if ($code eq "1" || $code eq "16" || $code eq "17" || $code eq "1300" || $code eq "100" || $code eq "101" || $code eq "102" || $code eq "56" )
  std::string line;
  char        c = '\0'; // v3.0.219.10 optimizing file read
  std::string code, airportCode, word;



  //// Loop over lines
  bool                     flag_codeIsAirport = false; // code == 1
  int                      charCounter_i      = STARTING_COUNTER_I; //, wordCounter_i = 1, maxWordsInLine = 0; // always reset charCounter_i to 1 and not zero
  std::vector<std::string> vec_splitLineAfterCode;


  bool flag_has105x_code        = false; // v24.03.1 frequency 1050-1056
  bool flag_has1300_code        = false;
  bool flag_has1302_code        = false; // MetaData code
  bool flag_has100_101_102_code = false; // runway info
  bool flag_start_code130_node       = false; // start airport boundary
  bool flag_closingBoundary_code130_with_code113_node= false; // End airport boundary with code 113

  auto const lmbda_has130_valid_code = [&]() { return (flag_start_code130_node * flag_closingBoundary_code130_with_code113_node == 1); };

  missionx::mx_aptdat_cached_info info;

  std::string indexNavInfo;
  indexNavInfo.clear();

  auto const lmbda_store_navinfo_in_cache = [&]() {
    if (flag_has105x_code || flag_has1300_code || flag_has100_101_102_code || flag_has1302_code || lmbda_has130_valid_code()) // v3.303.8.3 added flag_start_code130_node
    {
      if (!indexNavInfo.empty() && !airportCode.empty() && !(info.listNavInfo.size() < 2)) // skip Nav info if one of the parameters is empty. We must have enough data to make it worth storing
      {
        if (!inReadCachedFile)
          info.isCustom = is_custom;                                                          // (relative_apt_dat_path.find("Global Airports") == std::string::npos);
        Utils::addElementToMap(missionx::data_manager::cachedNavInfo_map, airportCode, info); // ADD TO CACHE

        ++this->icao_id_counter;
        if (!inReadCachedFile) // v3.0.255.3
        {
          //#ifndef RELEASE
          //          if (airportCode.compare("UHBB") == 0) // used to debug specific issues
          //          {
          //            int d = 1;
          //            d = 2;
          //          }
          //#endif // !RELEASE
          parse_airport_to_sqlite(inThreadState, airportCode, info, (*OptimizeAptDat::db_airports_cache_ptr), scenery_pack_name); // v3.0.255.3
        }
      }
    }

    // Reset info and wait for new airport information
    indexNavInfo.clear();
    airportCode.clear();
    info.clear();
    flag_has105x_code                              = false; // v24.03.1
    flag_has1300_code                              = false;
    flag_has1302_code                              = false; // v3.0.241.10
    flag_has100_101_102_code                       = false; // v3.0.241.10
    flag_start_code130_node                        = false; // v3.303.8.3
    flag_closingBoundary_code130_with_code113_node = false; // v3.303.8.3
  };

  const auto lmbda_reset_bound_flags = [&]() {
    flag_start_code130_node        = false; // v3.303.8.3
    flag_closingBoundary_code130_with_code113_node = false; // v3.303.8.3
  };

  const auto lmbda_store_bound_nodes = [&]() {
    std::getline(file_aptDat, line);
    vec_splitLineAfterCode = mxUtils::split(mxUtils::trim(line), ' ');
    if (vec_splitLineAfterCode.size() > 1)
    {
      if (info.boundary.empty())
        info.boundary += (vec_splitLineAfterCode.at(0) + "," + vec_splitLineAfterCode.at(1));
      else 
        info.boundary += "|" + (vec_splitLineAfterCode.at(0) + "," + vec_splitLineAfterCode.at(1));

    }
  };

  while (file_aptDat.get(c) && !(inThreadState->flagAbortThread))
  {
    // Test if to STORE nav data
    if (charCounter_i == 1 && c == '\n') // have we reached end of line ? //v3.0.255.2 added "charCounter_i == 1" since we need to make sure this is an empty line and not "\n" right after the code was read. Example: "120\n" without information
    {
      lmbda_store_navinfo_in_cache();
      continue;
    }                                                                                   // End handling "\n"
    else if (charCounter_i == 3 && c == '\n' && code.at(0) == '9' && code.at(1) == '9') // code 99 = end of file
    {
      lmbda_store_navinfo_in_cache();
      continue;
    }
    else if (charCounter_i == 4 && (c == '\n' || c == ' ') && code.compare("130") == 0) // code 130 = start boundary
    {
      std::getline(file_aptDat, line); // skip until end of line
      flag_start_code130_node = true;
      flag_closingBoundary_code130_with_code113_node = false; // v24.03.1

      code.clear();
      charCounter_i      = STARTING_COUNTER_I;
      flag_codeIsAirport = false;

      //// debug
      //#ifndef RELEASE
      //if (airportCode.compare("KSEA") == 0)
      //{
      //    __debugbreak();
      //    int i = 0;
      //}
      //#endif

      continue;
    }

    // Test if to skip character
    else if (charCounter_i > 1 && (c == '\n' || c == '\r')) // v3.0.255.2 skip line if we only have code in ap.dat and no value, like: "120\n" or "130\n". The old code did not know how to deal with this
    {
      code.clear();
      charCounter_i      = STARTING_COUNTER_I; // always reset charCounter_i to 1 and not zero
      flag_codeIsAirport = false;
      continue;
    }
    else if (!std::isdigit(c) && c != ' ' && c != '\n' && c != '\r')
      continue;
    else if (charCounter_i == 1 && (c == '\n' || c == '\r')) // skip lines that represent only new lines (blank lines)
      continue;
    else if (c == ' ' || c == '\0' || file_aptDat.eof())
    {


      //// check code is airport.
      if (flag_codeIsAirport)
      {
        std::getline(file_aptDat, line);
        vec_splitLineAfterCode = mxUtils::split(mxUtils::trim(line), ' ');
        if (vec_splitLineAfterCode.size() > 3)
          airportCode = vec_splitLineAfterCode.at(3);
        {
          info.listNavInfo.push_back(code + mxconst::get_SPACE() + line + "\n");
          indexNavInfo = code + mxconst::get_SPACE() + line + " ~";
        }
      }
      
      auto iCode = mxUtils::stringToNumber<int>(code);

      // 16   1355 0 0 01MN [S] Barnes  => seaway
      // 17    122 0 0 02CA [H] Swepi Beta Platform Ellen => Helipad
      if (code.compare("16") == 0 || code.compare("17") == 0)
      {
        std::getline(file_aptDat, line);
        vec_splitLineAfterCode = mxUtils::split(mxUtils::trim(line), ' ');
        if (vec_splitLineAfterCode.size() > 3)
          airportCode = vec_splitLineAfterCode.at(3);

        info.listNavInfo.push_back(code + mxconst::get_SPACE() + line + "\n");
        indexNavInfo = code + mxconst::get_SPACE() + line + " ~";

        if (lmbda_has130_valid_code()) // v3.303.8.3
          lmbda_reset_bound_flags(); 
      }
      else if ((code.compare("1300") == 0) || (code.compare("15") == 0)) // ramp data
      {
        // 1300  10.91196027  124.43800826 221.55 misc jets|turboprops|props|helos Cargo Ramp
        std::getline(file_aptDat, line);

        vec_splitLineAfterCode = mxUtils::split(mxUtils::trim(line), ' ');

        info.listNavInfo.push_back(code + mxconst::get_SPACE() + line + "\n");
        flag_has1300_code = true;

        if (lmbda_has130_valid_code()) // v3.303.8.3
          lmbda_reset_bound_flags(); 
      }
      else if ((code.compare("1302") == 0)) // Metadata // v3.0.241.10
      {
        // 1302 icao_code NJ51  // in the format of code, key value
        std::getline(file_aptDat, line);
        vec_splitLineAfterCode = mxUtils::split(mxUtils::trim(line), ' ');

        info.listNavInfo.push_back(code + mxconst::get_SPACE() + line + "\n");
        flag_has1302_code = true;

        if (lmbda_has130_valid_code()) // v3.303.8.3
          lmbda_reset_bound_flags(); 
      }
      else if ( iCode > 1049 && iCode < 1057 ) // Frequency // v24.03.1
      {
        // 1050 - 1056 8.33kHz communication frequencies (11.30+) Zero, one or many for each airport
        std::getline(file_aptDat, line);
        //vec_splitLineAfterCode = mxUtils::split(mxUtils::trim(line), ' ');

        info.listNavInfo.push_back(code + mxconst::get_SPACE() + line + "\n");
        flag_has105x_code = true;

        if (lmbda_has130_valid_code())
          lmbda_reset_bound_flags(); 
      }
      else if (code.compare("100") == 0 || code.compare("101") == 0 || code.compare("102") == 0 /*|| code.compare("56") == 0*/)
      {

        ////100 25.00 3 0 1.00 0 0 0 05R  50.12933714  014.52302572    0    0 0 0 0 0 23L  50.13387196  014.53174893    0    0 0 0 0 0
        ////102 H1  50.12860101  014.52351585 179.78 11.90 11.90 1 0 0 0.00 0
        ////101 91.44 1 08  21.31186721 -157.91575685 26  21.31191433 -157.90103531
        //#ifndef RELEASE
        //        if (code.compare("101") == 0)
        //          Log::logMsgThread("[[code 101]]\n"); // do we have seaports ?
        //#endif
        std::getline(file_aptDat, line);
        vec_splitLineAfterCode = mxUtils::split(mxUtils::trim(line), ' ');

        info.listNavInfo.push_back(code + mxconst::get_SPACE() + line + "\n");
        flag_has100_101_102_code = true;

        if (lmbda_has130_valid_code()) // v3.303.8.3
          lmbda_reset_bound_flags(); 
      }
      else if ( (flag_start_code130_node + flag_closingBoundary_code130_with_code113_node) == 1 && code.compare("111") == 0) // Node of a boundary - in this context
      {
        lmbda_store_bound_nodes();
      }
      else if ( (flag_start_code130_node + flag_closingBoundary_code130_with_code113_node) == 1 && code.compare("113") == 0) // End node of an airport boundary
      {
        lmbda_store_bound_nodes();
        flag_closingBoundary_code130_with_code113_node = true;
      }
      else if ( (flag_start_code130_node + flag_closingBoundary_code130_with_code113_node) == 1 && ((code.compare("113") == 0) + (code.compare("111") == 0) == 0)) // Airport boundary is not valid because we don't have the correct set of lines
      {
        lmbda_reset_bound_flags(); // reset flags: flag_start_code130_node, flag_closingBoundary_code130_with_code113_node and flag_has130_valid_code
        info.boundary.clear();
      }
      else
        std::getline(file_aptDat, line); // skip until end of line

      code.clear();
      flag_codeIsAirport = false;
      charCounter_i      = STARTING_COUNTER_I;
    }
    else
    {
      if (charCounter_i == 1 && c == '1')
        flag_codeIsAirport = true;
      else if (inReadCachedFile && charCounter_i == 1 && c == '*') // v3.0.253.6 special case for cache file read to distinguish custom vs none custom
        info.isCustom = true;
      else if (flag_codeIsAirport && charCounter_i > 1)
        flag_codeIsAirport = false;
      else
      flag_codeIsAirport = false;

      code += c;
      ++charCounter_i;

    }

  } // end loop over apt.dat line characters (while)



  if (file_aptDat.is_open())
    file_aptDat.close();

  return true;
}


// ---------------------------------------------------------


missionx::db_field
missionx::OptimizeAptDat::get_db_field_by_colName(std::list<missionx::db_field>& list_of_fields, std::string inColNameToSearch)
{
  missionx::db_field dummy;
  for (auto& field : list_of_fields)
  {
    if (mxUtils::stringToLower(field.col_name).compare(mxUtils::stringToLower(inColNameToSearch)) == 0)
    {
      return field;
    }
  }

  return dummy;
}


// ---------------------------------------------------------


void
missionx::OptimizeAptDat::prepare_sqlite_db_tables(thread_state* inThreadState, missionx::dbase* inDB_ptr)
{
  ///////////////////////////////////////////////
  // REMEMBER that every new table needs respective copy command from memory db to the file db. Found at the end of "read_and_parse_all_apt_files() function.
  ///////////////////////////////////////////////

  assert(inDB_ptr != nullptr && "Database pointer is empty");

  if (inDB_ptr->db_is_open_and_ready) // status. 0 = success/OK
  {
    // auto commit since we are not in transaction mode
    inDB_ptr->execute_stmt("drop table if exists dual"); // will have only one row and it will hold feature version number. We will create it at the end of the optimization, so it it is not exists then we have to rerun optimization code
    inDB_ptr->execute_stmt("drop table if exists xp_airports");
    inDB_ptr->execute_stmt("drop table if exists xp_ap_metadata");
    inDB_ptr->execute_stmt("drop table if exists xp_ap_frq");
    inDB_ptr->execute_stmt("drop table if exists xp_ap_ramps");
    inDB_ptr->execute_stmt("drop table if exists xp_helipads");
    inDB_ptr->execute_stmt("drop table if exists xp_loc");
    inDB_ptr->execute_stmt("drop table if exists xp_rw");
    inDB_ptr->execute_stmt("drop view if exists airports_vu");
    inDB_ptr->execute_stmt("drop view if exists ramps_vu");

    inDB_ptr->execute_stmt("create table if not exists xp_airports ( icao_id int, icao text, ap_elev integer, ap_name text, ap_type int, is_custom int, ap_lat real, ap_lon real, boundary text NULL, primary key (icao_id) )");
    inDB_ptr->execute_stmt("create table if not exists xp_ap_metadata ( icao_id int, icao text NOT NULL, key_col text NOT NULL, val_col text NULL ) ");
    inDB_ptr->execute_stmt("create table if not exists xp_ap_frq ( icao_id int, icao text NOT NULL, frq int NOT NULL, frq_desc text NULL ) ");
    inDB_ptr->execute_stmt(
      "create table if not exists xp_ap_ramps ( icao_id int, icao text NOT NULL, ramp_lat real NOT NULL, ramp_lon real NOT NULL, ramp_heading_true NULL, location_type text NULL, for_planes text NULL, ramp_uq_name text NULL)");
    inDB_ptr->execute_stmt("create table if not exists xp_rw ( icao_id int,  icao text NOT NULL, rw_width   real NULL, rw_surf integer NULL, rw_sholder integer NULL, rw_smooth  real NULL, rw_no_1    text NOT NULL, rw_no_1_lat real not "
                           "null, rw_no_1_lon real not null, rw_no_1_disp_hold real null, rw_no_2    text NOT NULL, rw_no_2_lat real not null, rw_no_2_lon real not null, rw_no_2_disp_hold real null, rw_length_mt integer null )");
    inDB_ptr->execute_stmt("create table if not exists xp_helipads ( icao_id int,  icao text NOT NULL, name text NULL, lat    real NOT NULL, lon    real NOT NULL, length real NULL,  width  real NULL ) ");
    inDB_ptr->execute_stmt("create table if not exists xp_loc ( lat real NOT NULL, lon real NOT NULL, ap_elev_ft integer, frq_mhz integer, max_reception_range integer, loc_bearing real, ident text, icao text, icao_region_code text, loc_rw "
                           "text, loc_type text, terminal_region text, class integer, name text, bias real ) ");

    // main view to fetch basic data on airports and their ramps
    inDB_ptr->execute_stmt(R"(create view airports_vu as  
WITH helipads_view as (select icao_id, count(1) as helipad_counter from xp_helipads group by icao_id),
     oilrig_view as (select t1.icao_id, t1.icao, 1 as is_oilrig  from xp_helipads t1,  xp_ap_metadata t2 where t1.icao_id = t2.icao_id  and t2.key_col = 'is_oilrig' and t2.val_col = '1'  group by t1.icao_id, t1.icao),
     heli_ramps_view as (select icao_id, count(1) as ramp_helis from xp_ap_ramps where for_planes like '%helos%' or lower(ramp_uq_name) like '%heli%' group by icao_id),
     plane_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where (for_planes <> 'helos' or for_planes is null) and lower(ramp_uq_name) not like '%heli%' and lower(ramp_uq_name) not like '%hold short%' group by icao_id),
     props_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%props%' group by icao_id),
     turboprop_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%turboprops%' group by icao_id),
     jets_heavy_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%jets%' or for_planes like '%heavy%' group by icao_id),
     fighter_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%fighter%' group by icao_id),
     rw_hard_vu as (select icao_id, count(1) as rw_hard from xp_rw where rw_surf in (1,2) group by icao_id),
     rw_dirt_gravel_vu as (select icao_id, count(1) as rw_dirt_n_gravel from xp_rw where rw_surf in (4,5) group by icao_id),
     rw_grass_vu as (select icao_id, count(1) as rw_grass from xp_rw where rw_surf = 3 group by icao_id),     
     rw_water_vu as (select icao_id, count(1) as rw_water from xp_rw where rw_surf = 13 group by icao_id),
     region_meta_vu as (select icao_id, val_col as region_name from xp_ap_metadata t1 where key_col = 'region_code'),
     country_vu as (select icao_id, val_col as country from xp_ap_metadata t1 where key_col = 'country')
SELECT t1.icao_id,
       t1.icao,
       t1.ap_elev as ap_elev_ft,
       t1.ap_name,
       t1.ap_type,
       t1.is_custom,
       t1.ap_lat,
       t1.ap_lon
       ,IFNULL((select helipad_counter from helipads_view v1 where t1.icao_id = v1.icao_id), 0) as helipads
       ,IFNULL((select is_oilrig from oilrig_view v1 where t1.icao_id = v1.icao_id), 0) as is_oilrig
       ,IFNULL((select ramp_helis from heli_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_helos
       ,IFNULL((select ramp_planes from plane_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_planes       
       ,IFNULL((select ramp_planes from props_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_props       
       ,IFNULL((select ramp_planes from turboprop_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_turboprops       
       ,IFNULL((select ramp_planes from jets_heavy_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_jet_heavy       
       ,IFNULL((select ramp_planes from fighter_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_fighters       
       ,IFNULL((select rw_hard from rw_hard_vu v1 where t1.icao_id = v1.icao_id ), 0) as rw_hard           
       ,IFNULL((select rw_dirt_n_gravel from rw_dirt_gravel_vu v1 where t1.icao_id = v1.icao_id ), 0) as rw_dirt_gravel           
       ,IFNULL((select rw_grass from rw_grass_vu v1 where t1.icao_id = v1.icao_id ), 0) as rw_grass           
       ,IFNULL((select rw_water from rw_water_vu v1 where t1.icao_id = v1.icao_id ), 0) as rw_water
       ,IFNULL((select region_name from region_meta_vu v1 where t1.icao_id = v1.icao_id ), NULL) as region_code     
       ,IFNULL((select country from country_vu v1 where t1.icao_id = v1.icao_id ), NULL) as country     
       , t1.boundary           
  FROM xp_airports t1)");

    inDB_ptr->execute_stmt(R"(create view ramps_vu as
SELECT icao_id, icao, ramp_lat as lat, ramp_lon as lon, ramp_heading_true heading, ramp_uq_name as name, for_planes
     -- All the last "OR" logic in each case statements is for compatibility with old scenery files codes and format. We try to guess the ramp type
     , case when (instr( lower(IFNULL(for_planes,'')), 'helos') > 0) or (instr( lower(IFNULL(ramp_uq_name,'')), 'heli') > 0) then 1  else 0 END as helos  
     , case when instr( lower(IFNULL(for_planes,'')), 'props') > 0 or  (instr( lower(IFNULL(ramp_uq_name,'')), 'general') > 0) then 1 else 0 END as props
     , case when instr( lower(IFNULL(for_planes,'')), 'turboprops') > 0 or  (instr( lower(IFNULL(ramp_uq_name,'')), 'apron') > 0) then 1 else 0 END as turboprops
     , case when instr( lower(IFNULL(for_planes,'')), 'heavy') > 0 or (instr( lower(IFNULL(for_planes,'')), 'jet') > 0) or (instr( lower(IFNULL(ramp_uq_name,'')), 'apron') > 0) then 1 else 0 END as jet_n_heavy
     , case when instr( lower(IFNULL(for_planes,'')), 'fighter') > 0 then 1 else 0 END as fighters
     , 1 as which_table
FROM xp_ap_ramps t1
union
SELECT icao_id, icao, lat, lon, 0 as heading, name, "helos" as for_planes, 1 as helos, 0 as props, 0 as turboprops, 0 as jet_n_heavy, 0 as fighters, 2 as which_table
FROM xp_helipads t1
ORDER BY icao_id
)");

    inDB_ptr->execute_stmt("CREATE UNIQUE INDEX icao_apName_n1 on xp_airports (icao, ap_name)");
    inDB_ptr->execute_stmt("CREATE INDEX ap_type_n2 on xp_airports (ap_type)");
    inDB_ptr->execute_stmt("CREATE INDEX icao_id_helipad_n1 on xp_helipads (icao_id)");
    inDB_ptr->execute_stmt("CREATE INDEX icao_id_rw_n1 on xp_rw (icao_id)");
    inDB_ptr->execute_stmt("CREATE INDEX surf_type_rw_n2 on xp_rw (rw_surf)");
    inDB_ptr->execute_stmt("CREATE INDEX ap_frq_icao_id_idx on xp_ap_frq (icao_id)"); // v24.03.1

    // the following indexes are optional
    // inDB_ptr->execute_stmt("CREATE INDEX icao_helipad_n2 on xp_helipads (icao)");
    // inDB_ptr->execute_stmt("CREATE INDEX icao_rw_n2 on xp_rw (icao)");
  }
}


// ---------------------------------------------------------


void
missionx::OptimizeAptDat::sqlite_post_tables_copy(missionx::dbase* inDB_ptr)
{
  assert(inDB_ptr != nullptr && inDB_ptr->db_is_open_and_ready && "database is not open for dual table creation. Notify programmer.");

  inDB_ptr->execute_stmt("CREATE INDEX icao_helipad_n2 on xp_helipads (icao)");
  inDB_ptr->execute_stmt("CREATE INDEX icao_rw_n2 on xp_rw (icao)");
  inDB_ptr->execute_stmt("CREATE INDEX icao_id_metadata_n1 on xp_ap_metadata (icao_id);"); // create index on metadata icao_id column

  inDB_ptr->execute_stmt("create table if not exists dual ( feature_version int NOT NULL ) ");
  inDB_ptr->execute_stmt("insert into dual ( feature_version ) values ( " + mxUtils::formatNumber<int>(missionx::MX_FEATURES_VERSION) + " ) ");


          // THE LENGTH OF THE QUERY IS IMPORTANT - Try to stay below 1024 characters
  // -- delete all duplicate ICAOs based on the distance between duplicate ICAOs or if they are in same major coordinates like (40, -74)
  // Currently min distance is 5 nautical miles
  // -- Remove the higher icao_id rows since their scenery apt.dat is lower in the scenery hierarchy
  // LAG (expr, offset, [default value]) OVER (PARTITION BY col ORDER BY col)
  const std::string deleteDuplicateICAOs = R"(DELETE FROM xp_airports
  WHERE icao_id IN (
    SELECT icao_id
    FROM (
      SELECT t1.*,
        LAG(icao_id, 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao) pos_in_group,
        mx_calc_distance(t1.ap_lat, t1.ap_lon, LAG(t1.ap_lat, 1, t1.ap_lat) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id), LAG(t1.ap_lon, 1, t1.ap_lon) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id), 3440) AS distance,
        trunc(t1.ap_lat) - LAG(trunc(t1.ap_lat), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) AS lat_diff,
        trunc(t1.ap_lon) - LAG(trunc(t1.ap_lon), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) AS lon_diff
      FROM xp_airports t1
      WHERE t1.icao IN (
        SELECT icao
        FROM xp_airports t3
        GROUP BY icao
        HAVING count(icao) > 1
        )
      ORDER BY icao,
      icao_id
    ) v1
  WHERE v1.pos_in_group != 0
  AND (v1.distance < 2.4 OR (lat_diff = 0 AND lon_diff = 0) )
  ))";


   //// Post target DB duplicate cleanup
  inDB_ptr->start_transaction();
  if (!inDB_ptr->execute_stmt(deleteDuplicateICAOs))
  {
    Log::logMsgThread("[opt] DELETE ERROR - Failed to clean duplicate ICAOs.");
  }
  inDB_ptr->end_transaction();



}


// ---------------------------------------------------------

void
missionx::OptimizeAptDat::parse_airport_to_sqlite(thread_state* inThreadState, const std::string& airportCode, const missionx::mx_aptdat_cached_info& info, missionx::dbase& db, const std::string& scenery_pack_name)
{
  ///////////////////////////////////////////////
  // REMEMBER that every new table needs respective copy command from memory db to the file db. Found at the end of "read_and_parse_all_apt_files() function.
  ///////////////////////////////////////////////


  const std::string            icao_code    = airportCode;
  const std::list<std::string> ap_list_data = info.listNavInfo;

  ///// PREPARE STATMENTS
  std::string prep_stmt_key_s; 
  std::map<std::string, std::string> map_key_value; // store values in readable key/value, unique to each code line

  std::pair<std::string, missionx::db_types> store_pair;

  if (db.db_is_open_and_ready) // status. 0 = success/OK
  {

    // read main airport list
    bool flag_wrote_airport_lat_long_based_on_runway = false;
    auto it_end                                      = ap_list_data.cend();

    // v3.0.255.3 Add icao_id field and emplace_front() icao_id_counter
    missionx::db_field field_icao_id;
    field_icao_id.col_name = "icao_id";
    field_icao_id.dataype  = missionx::db_types::int_typ;
    field_icao_id.value_s  = mxUtils::formatNumber<int>(this->icao_id_counter);

    for (auto it = ap_list_data.cbegin(); it != it_end; ++it)
    {
      const std::string line                   = Utils::rtrim(*it);
      int               split_fields_counter_i = 1; // holds the field we are working on. We start at 1 and not 0

      map_key_value.clear();
      std::list<missionx::db_field> list_of_fields; // store col_name, {value, datatype}


      list_of_fields.emplace_front(field_icao_id); // v3.0.255.3 manually adding the icao_id to the list


      //static constexpr char const *delimeters       = " \t";
      static constexpr char const *delimeters       = " ";
      //const std::vector<std::string> vec_split_line = mxUtils::split_v2(line, delimeters, false); // This version of split_v2, handles empty tokens.
      const std::vector<std::string> vec_split_line = mxUtils::split_skipEmptyTokens(line, ' '); // This code seem to be ~2seconds faster than split_v2

      if (vec_split_line.size() > 2)
      {
        auto it_field    = vec_split_line.cbegin();
        auto it_line_end = vec_split_line.cend();

        int code_i = Utils::stringToNumber<int>(*it_field); // fetch first value

        auto it_field_mapping     = table_col_name_type_mapping[code_i].begin();
        auto it_field_mapping_end = table_col_name_type_mapping[code_i].cend();

        const int no_of_mapped_fields = (int)table_col_name_type_mapping[code_i].size();

        // we store ICAO manually since it is not in the split line data
        if (it_field_mapping->dataype != db_types::skip_field)
        {
          it_field_mapping->value_s = icao_code;
          list_of_fields.emplace_back(*it_field_mapping);
        }

        split_fields_counter_i++;
        // loop over split data + mapping rules and assign the values only for the "none skipped" fields
        // We basically construct a one line of key|value in format "col_name", "field_class". The field class holds {col_name + value_s + value_type}. It does not hold the skipped fields
        while (++it_field != it_line_end && ++it_field_mapping != it_field_mapping_end &&
               (split_fields_counter_i < no_of_mapped_fields ||
                (split_fields_counter_i == no_of_mapped_fields && it_field_mapping->dataype != missionx::db_types::text_typ))) // loop as long as there are more fields or the last value is not a text.
        {
          if (it_field_mapping->dataype != missionx::db_types::skip_field)
          {
            it_field_mapping->value_s = *it_field;
            list_of_fields.emplace_back(*it_field_mapping);
          }
          split_fields_counter_i++;
        }

        if (it_field != it_line_end && it_field_mapping != it_field_mapping_end && split_fields_counter_i <= no_of_mapped_fields) // if last field is a text field, then append all the rest of splitted strings
        {
          bool        flag_read_more_than_one_word = false;
          std::string last_field_to_the_end = "";
          do
          {
            if (flag_read_more_than_one_word)
              last_field_to_the_end.append(" ").append(*it_field);
            else 
            {
              last_field_to_the_end.append(*it_field);
              flag_read_more_than_one_word ^= 1;
            }
                          
          } while (++it_field != it_line_end );

          it_field_mapping->value_s = last_field_to_the_end;
          list_of_fields.emplace_back(*it_field_mapping); // store column name as "key", and the whole class as "value"
        }


        // Insert commands
        switch (code_i)
        {
          case 1:  // Land airport
          case 16: // sea port
          case 17: // heliport
          {

            prep_stmt_key_s = "ins_xp_airports";
            if (list_of_fields.size() > 0)
            {
              // v3.0.253.6 add extra field that holds custom scenery flag
              {
                missionx::db_field field;
                field.col_name = "is_custom";
                field.dataype  = missionx::db_types::int_typ;
                if (info.isCustom)
                {
                  field.value_s = "1"; // true, this airport has custom scenery
                }
                list_of_fields.emplace_back(field);
              }

	            // airport type
              missionx::db_field field_ap_type;
              field_ap_type.col_name = "ap_type";
              field_ap_type.dataype  = missionx::db_types::int_typ;
              field_ap_type.value_s  = mxUtils::formatNumber<int>(code_i);
              list_of_fields.emplace_back(field_ap_type);

              // v3.303.8.3 add airport boundary
              {
                missionx::db_field field;
                field.col_name = "boundary";
                field.dataype  = missionx::db_types::text_typ;
                if (!info.boundary.empty())
                {
                  field.value_s = info.boundary; // boundary string, delimited by "|"
                }
                list_of_fields.emplace_back(field);
              }

              if (!db.bind_and_execute_ins_stmt(prep_stmt_key_s, " xp_airports ", list_of_fields))
              {
#ifndef RELEASE
                Log::logMsgThread("\tduplicate airport: (" + icao_code + ", " + scenery_pack_name + ") !!!!");
#endif                  // !RELEASE
                return; // exit function due to duplication or failure in binding insert command
              }
            }
          }
          break;
          case 1302: // airport metadata
          {
            prep_stmt_key_s = "ins_xp_ap_metadata_1302";
            if (list_of_fields.size() > 0)
            {
              db.bind_and_execute_ins_stmt(prep_stmt_key_s, " xp_ap_metadata ", list_of_fields);
            }
          }
          break;
          case 1050: // airport frq
          case 1051: // airport frq
          case 1052: // airport frq
          case 1053: // airport frq
          case 1054: // airport frq
          case 1055: // airport frq
          case 1056: // airport frq
          {
            prep_stmt_key_s = "ins_xp_ap_frq_1050_1056";
            if (list_of_fields.size() > 0)
            {
              db.bind_and_execute_ins_stmt(prep_stmt_key_s, " xp_ap_frq ", list_of_fields);
            }
          }
          break;
          case 1300: // startup location, like ramps
          {
            prep_stmt_key_s = "ins_xp_ap_ramps_1300";
            if (list_of_fields.size() > 0)
            {
              db.bind_and_execute_ins_stmt(prep_stmt_key_s, " xp_ap_ramps ", list_of_fields);
            }
          }
          break;
          case 15: // older startup location, like ramps
          {
            prep_stmt_key_s = "ins_xp_ap_ramps_15";
            if (list_of_fields.size() > 0)
            {
              db.bind_and_execute_ins_stmt(prep_stmt_key_s, " xp_ap_ramps ", list_of_fields);
            }
          }
          break;
          case 100: // Land runway
          case 101: // Water runway
          {
            if (list_of_fields.size() > 0)
            {
              double             lat1, lon1, lat2, lon2;
              std::string        lat1_s, lon1_s, lat2_s, lon2_s;
              std::string        disp_rw1_s, disp_rw2_s;
              missionx::db_field rwLength_field("rw_length_mt", missionx::db_types::int_typ);

              // calculate rw length and add it as a column
              lat1_s     = this->get_db_field_by_colName(list_of_fields, "rw_no_1_lat").value_s;
              lon1_s     = this->get_db_field_by_colName(list_of_fields, "rw_no_1_lon").value_s;
              lat2_s     = this->get_db_field_by_colName(list_of_fields, "rw_no_2_lat").value_s;
              lon2_s     = this->get_db_field_by_colName(list_of_fields, "rw_no_2_lon").value_s;
              disp_rw1_s = this->get_db_field_by_colName(list_of_fields, "rw_no_1_disp_hold").value_s;
              disp_rw2_s = this->get_db_field_by_colName(list_of_fields, "rw_no_2_disp_hold").value_s;
              if (mxUtils::is_number(lat1_s) && mxUtils::is_number(lon1_s) && mxUtils::is_number(lat2_s) && mxUtils::is_number(lon2_s))
              {
                lat1            = mxUtils::stringToNumber<double>(lat1_s);
                lon1            = mxUtils::stringToNumber<double>(lon1_s);
                lat2            = mxUtils::stringToNumber<double>(lat2_s);
                lon2            = mxUtils::stringToNumber<double>(lon2_s);
                int disp_rw_1_i = (disp_rw1_s.empty()) ? 0 : mxUtils::stringToNumber<int>(disp_rw1_s); // v3.0.255.3 fixed calculation if value is empty.
                int disp_rw_2_i = (disp_rw2_s.empty()) ? 0 : mxUtils::stringToNumber<int>(disp_rw2_s); // v3.0.255.3 fixed calculation if value is empty.


                double dist_d          = Utils::calcDistanceBetween2Points_nm(lat1, lon1, lat2, lon2, missionx::mx_units_of_measure::meter) - ((double)(disp_rw_1_i + disp_rw_2_i)); // v3.0.253.6 added display hold to the calculation. need to detract from the distance
                rwLength_field.value_s = mxUtils::formatNumber<double>(dist_d, 0);
              }

              list_of_fields.push_back(rwLength_field);

              if (code_i == 100)
                prep_stmt_key_s = "ins_xp_rw_100";
              else
                prep_stmt_key_s = "ins_xp_rw_101";

              db.bind_and_execute_ins_stmt(prep_stmt_key_s, " xp_rw ", list_of_fields);

              if (!flag_wrote_airport_lat_long_based_on_runway)
              {
                // update xp_airports with lat/lon
                const std::string query = R"(update xp_airports
                                          set ap_lat = (select rw_no_1_lat from(select ROW_NUMBER() OVER(partition by t1.icao_id) as row, t1.rw_no_1_lat from xp_rw t1 where t1.icao_id = ')" +
                                          field_icao_id.value_s + R"(') where row = 1),
                                              ap_lon = (select rw_no_1_lon from(select ROW_NUMBER() OVER(partition by t1.icao_id) as row, t1.rw_no_1_lon from xp_rw t1 where t1.icao_id = ')" +
                                          field_icao_id.value_s + R"(') where row = 1)
                                          where xp_airports.icao_id = )" +
                                          field_icao_id.value_s + " and xp_airports.ap_lat is null";

                db.execute_stmt(query);
                flag_wrote_airport_lat_long_based_on_runway = true;
              }
            }
          }
          break;
          case 102: // Helipad
          {
            prep_stmt_key_s = "ins_helipad_102";
            if (list_of_fields.size() > 0)
            {
              db.bind_and_execute_ins_stmt(prep_stmt_key_s, " xp_helipads ", list_of_fields);
              if (!flag_wrote_airport_lat_long_based_on_runway)
              {
                // update xp_airports with lat/lon
                const std::string query = R"(update xp_airports
                                          set ap_lat = (select lat from(select ROW_NUMBER() OVER(partition by t1.icao_id) as row, t1.lat from xp_helipads t1 where t1.icao_id = ')" +
                                          field_icao_id.value_s + R"(') where row = 1),
                                              ap_lon = (select lon from(select ROW_NUMBER() OVER(partition by t1.icao_id) as row, t1.lon from xp_helipads t1 where t1.icao_id = ')" +
                                          field_icao_id.value_s + R"(') where row = 1)
                                          where xp_airports.icao_id = )" +
                                          field_icao_id.value_s + " and xp_airports.ap_lat is null";


                db.execute_stmt(query);
                flag_wrote_airport_lat_long_based_on_runway = true;
              }
            }
          }
          break;
          default:
            break;
        }


      } // end vec_split_line

    } // end loop over 1 airport information lines

  }
  else
  {
    // Something is wrong with allocating DB memory
    Log::logMsgThread("[optimize apt dat, airport :memory:] Failed allocation of db.");
  }

  // OptimizeAptDat::db_airports_cache_ptr->detach_database(attachName);



} // parse airport to sqlite



// ---------------------------------------------------------



void
missionx::OptimizeAptDat::upload_navdata_to_inMemory_db(thread_state* inThreadState, missionx::dbase& db)
{
  std::string                        prep_stmt_key_s = "";
  std::map<std::string, std::string> map_key_value; // store values in readable key/value, unique to each code line

  // Field names: nav_code, lat, lon, ap_elev_ft, frq_mhz,max_reception_Range,loc_bearing,ident,icao, icao_region
  //            ,  loc_rw, loc_type, terminal_region, class, name, bias
  std::map<int, std::list<missionx::db_field>> table_nav_col_name_type_mapping = {
    { 2, // NDB
      { db_field("nav_code", db_types::skip_field),
        db_field("lat", db_types::real_typ),
        db_field("lon", db_types::real_typ),
        db_field("ap_elev_ft", db_types::int_typ),
        db_field("frq_mhz", db_types::int_typ),
        db_field("class", db_types::int_typ),
        db_field("loc_bearing", db_types::zero_type),
        db_field("ident", db_types::text_typ),
        db_field("terminal_region", db_types::text_typ),
        db_field("icao_region_code", db_types::text_typ),
        db_field("name", db_types::text_typ),
        db_field("loc_type", db_types::text_typ) } }, // NDB 2
    { 3, // VOR
      { db_field("nav_code", db_types::skip_field),
        db_field("lat", db_types::real_typ),
        db_field("lon", db_types::real_typ),
        db_field("ap_elev_ft", db_types::int_typ),
        db_field("frq_mhz", db_types::int_typ),
        db_field("class", db_types::int_typ),
        db_field("loc_bearing", db_types::real_typ),
        db_field("ident", db_types::text_typ),
        db_field("terminal_region", db_types::text_typ),
        db_field("icao_region_code", db_types::text_typ),
        db_field("name", db_types::text_typ),
        db_field("loc_type", db_types::text_typ) } }, // VOR 3
    { 4, // ILS
      { db_field("nav_code", db_types::skip_field),
        db_field("lat", db_types::real_typ),
        db_field("lon", db_types::real_typ),
        db_field("ap_elev_ft", db_types::int_typ),
        db_field("frq_mhz", db_types::int_typ),
        db_field("max_reception_range", db_types::int_typ),
        db_field("loc_bearing", db_types::real_typ),
        db_field("ident", db_types::text_typ),
        db_field("icao", db_types::text_typ),
        db_field("icao_region_code", db_types::text_typ),
        db_field("loc_rw", db_types::text_typ),
        db_field("loc_type", db_types::text_typ) } }, // LOC 3
    { 5,                                              // LOC
      { db_field("nav_code", db_types::skip_field),
        db_field("lat", db_types::real_typ),
        db_field("lon", db_types::real_typ),
        db_field("ap_elev_ft", db_types::int_typ),
        db_field("frq_mhz", db_types::int_typ),
        db_field("max_reception_range", db_types::int_typ),
        db_field("loc_bearing", db_types::real_typ),
        db_field("ident", db_types::text_typ),
        db_field("icao", db_types::text_typ),
        db_field("icao_region_code", db_types::text_typ),
        db_field("loc_rw", db_types::text_typ),
        db_field("loc_type", db_types::text_typ) } }, // LOC 4
    { 12, // DME
      { db_field("nav_code", db_types::skip_field),
        db_field("lat", db_types::real_typ),
        db_field("lon", db_types::real_typ),
        db_field("ap_elev_ft", db_types::int_typ),
        db_field("frq_mhz", db_types::int_typ),
        db_field("class", db_types::int_typ),
        db_field("bias", db_types::real_typ),
        db_field("ident", db_types::text_typ),
        db_field("terminal_region", db_types::text_typ),
        db_field("icao_region_code", db_types::text_typ),
        db_field("name", db_types::text_typ),
        db_field("loc_type", db_types::text_typ) } }, // DME 12
    { 14,                                             // WAAS/GBAS
      { db_field("nav_code", db_types::skip_field),
        db_field("lat", db_types::real_typ),
        db_field("lon", db_types::real_typ),
        db_field("ap_elev_ft", db_types::int_typ),
        db_field("frq_mhz", db_types::int_typ), // waas (sbas) or GLS (gbas) channel
        db_field("max_reception_range", db_types::zero_type), // always 0 - mapped to max_reception_range
        db_field("loc_bearing", db_types::real_typ),
        db_field("ident", db_types::text_typ),
        db_field("icao", db_types::text_typ),
        db_field("icao_region_code", db_types::text_typ),
        db_field("loc_rw", db_types::text_typ),
        db_field("loc_type", db_types::text_typ) } }, // LPV // SBAS GBAS
    { 15,                                             // GLS
      { db_field("nav_code", db_types::skip_field),
        db_field("lat", db_types::real_typ),
        db_field("lon", db_types::real_typ),
        db_field("ap_elev_ft", db_types::int_typ),
        db_field("frq_mhz", db_types::int_typ), // GLS Channel number
        db_field("max_reception_range", db_types::zero_type), // always 0 - mapped to max_reception_range
        db_field("approach_course", db_types::real_typ), // format: {glide slope}{approach deg} 3/N-3.  Associated final approach course in true degrees prefixed by glidepath angle
        db_field("ident", db_types::text_typ),
        db_field("icao", db_types::text_typ),
        db_field("icao_region_code", db_types::text_typ),
        db_field("loc_rw", db_types::text_typ),
        db_field("loc_type", db_types::text_typ) } } // GLS 

  };


  const std::string earth_nav_dat_file = Utils::get_earth_nav_dat_file();
  std::ifstream     infs;
  std::cin.tie(nullptr);

#ifndef RELEASE
  missionx::Log::logMsgThread("[Read ILS] Nav data file: " + earth_nav_dat_file);
#endif // !RELEASE



  infs.open(earth_nav_dat_file.c_str(), std::ios::in);
  if (infs.is_open())
  {
    std::string line;
    int         line_processed = 0;
    while ((getline(infs, line)) && !(inThreadState->flagAbortThread))
    {
      map_key_value.clear();

      const std::vector<std::string> vec_split_line = Utils::split(line);
      if (vec_split_line.size() > 2)
      {
        // if line starts with 2, 3, 4, 5, 12, 14, 15
        if (vec_split_line.front() == mxconst::get_TWO() || vec_split_line.front() == mxconst::get_THREE()
         || vec_split_line.front() == mxconst::get_FOUR() || vec_split_line.front() == mxconst::get_FIVE()
         || vec_split_line.front() == mxconst::get_TWELVE()
         || vec_split_line.front() == mxconst::get_FORTEEN()|| vec_split_line.front() == mxconst::get_FIFTEEN() )
        {
          {
            std::list<missionx::db_field> list_of_fields;
            auto it_field               = vec_split_line.cbegin();
            auto it_line_end            = vec_split_line.cend();

            const int code_i = Utils::stringToNumber<int>(*it_field); // fetch first value

            auto it_field_mapping     = table_nav_col_name_type_mapping[code_i].begin();
            auto it_field_mapping_end = table_nav_col_name_type_mapping[code_i].end();

            while (it_field != it_line_end && it_field_mapping != it_field_mapping_end) // loop as long as field number is lower than the no of fields.
            {
              //#ifndef RELEASE // debug
              //if ((*it_field_mapping).col_name.compare("icao") == 0 && (*it_field).compare("UHBB") == 0)
              //  int i = 0;
              //#endif // !RELEASE


              if (it_field_mapping->dataype != missionx::db_types::skip_field)
              {
                if ((*it_field_mapping).col_name == "loc_bearing") // fix bug
                {
                  it_field_mapping->value_s = mxUtils::formatNumber<float>(fmod(mxUtils::stringToNumber<float>(*it_field, 3), 360.0f)); // divide by 360 to find the correct ILS bearing
                }
                else if ((*it_field_mapping).col_name == "approach_course") // Code 15 GLS and Code 16
                {
                  #ifndef RELEASE
                  std::string sGlideSlope;
                  std::string sBearing;
                  // split string to "first 3" and "rest"
                  sGlideSlope = (*it_field).substr(0, 3);
                  sBearing    = (*it_field).substr(3);
                  #endif
                  if ((*it_field).length() > 3)
                  {
                    it_field_mapping->value_s = (*it_field).substr(3); // We only keep the heading and ignore the slope
                  }
                  else
                  {
                    it_field_mapping->value_s = "-1"; // n/a - no valid value
                  }
                  it_field_mapping->col_name = "loc_bearing"; // remaps to "loc_bearing" field for table column compatibility
                }
                else if (code_i == 2 && (*it_field_mapping).col_name == "loc_type")
                  it_field_mapping->value_s = "NDB";
                else if (code_i == 3 && (*it_field_mapping).col_name == "loc_type")
                  it_field_mapping->value_s = "VOR";
                else if (code_i == 12 && (*it_field_mapping).col_name == "loc_type")
                  it_field_mapping->value_s = "DME";
                else if ((*it_field_mapping).col_name == "loc_type")
                  it_field_mapping->value_s = mxUtils::trim( *it_field ); // fix loc_type has space at the end of the string
                else
                  it_field_mapping->value_s = *it_field;

                list_of_fields.push_back(*it_field_mapping);
              }
              ++it_field;
              ++it_field_mapping;


            } // end loop over all split strings except last one

            // Insert commands
            switch (code_i)
            {
              case 4:
              case 5:
              case 14:
              case 15:
              {
                prep_stmt_key_s = "ins_xp_loc_codes-4-5-14-15"; // insert localizer data
                if (!list_of_fields.empty())
                {
                  db.bind_and_execute_ins_stmt(prep_stmt_key_s, " xp_loc ", list_of_fields);
                  ++line_processed;
                }
              }
              break;
              case 2:
              case 3:
              {
                prep_stmt_key_s = "ins_xp_loc_codes-2-3"; // insert VOR/NDB data
                if (!list_of_fields.empty())
                {
                  db.bind_and_execute_ins_stmt(prep_stmt_key_s, " xp_loc ", list_of_fields);
                  ++line_processed;
                }

              }
              break;
              case 12:
              {
                prep_stmt_key_s = "ins_xp_loc_codes-12"; // insert DME data, has different field number
                if (list_of_fields.size() > 0)
                {
                  db.bind_and_execute_ins_stmt(prep_stmt_key_s, " xp_loc ", list_of_fields);
                  ++line_processed;
                }

              }
              break;
              default:
                break;
            } // switch


          } // end if split is larger than 2
        }
      }


      if (inThreadState->flagAbortThread)
        break;

    } // end loop over all lines

    Log::logMsgThread("\tLines processed in ILS: " + mxUtils::formatNumber<int>(line_processed)); // DEBUG

  } // end if file is open

  if (infs.is_open())
    infs.close();
}


// ---------------------------------------------------------


void
missionx::OptimizeAptDat::stop_plugin()
{
  OptimizeAptDat::aptState.flagAbortThread = true;
  if (OptimizeAptDat::thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
    OptimizeAptDat::thread_ref.join();       // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"

}
