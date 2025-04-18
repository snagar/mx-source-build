#ifdef IBM

#include <psapi.h>
#include <tchar.h>
#include <windows.h>
#pragma comment(lib, "psapi.lib")
#include <chrono>
#include <conio.h>
#include <libloaderapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

BOOL APIENTRY
DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{

  std::string BACKSLASHx2 = "\\";
  std::string missionx_library_path;
  missionx_library_path.clear();
  //  PCWSTR missionx_library_path; missionx_library_path.clear();
  std::string xp_dll_folder_path;
  xp_dll_folder_path.clear();
  HMODULE fmod_module1 = NULL;

  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    {
      // Log::logMsg("Process Attached"); // debug

      // Add plugin library folder to the "search dir".
      HMODULE hMods[1024];
      HANDLE  hProcess;
      DWORD   cbNeeded;
      // unsigned int i;

      DWORD  processID = GetCurrentProcessId();
      LPTSTR lpFilename;
      DWORD  nSize = sizeof(lpFilename);


      hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
      if (NULL == hProcess)
        break;

      if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
      {
        // char nodeCName[MAX_PATH]; // dummy
        TCHAR szModName[1025];

        if (GetModuleFileNameEx(hProcess, hMods[0], szModName, sizeof(szModName)))
        {
          std::string executableName = std::string(szModName);
          size_t      lastBackslash  = executableName.find_last_of(BACKSLASHx2.c_str());
          std::string base_dir       = executableName.substr(0, lastBackslash);
          xp_dll_folder_path         = base_dir + BACKSLASHx2 + "Resources" + BACKSLASHx2 + "dlls" + BACKSLASHx2 + "64"; // v2.1.29.2
          missionx_library_path      = base_dir + BACKSLASHx2 + "Resources" + BACKSLASHx2 + "plugins" + BACKSLASHx2 + "missionx" + BACKSLASHx2 + "libs";
#ifdef MX_EXE
          xp_dll_folder_path    = "r:/X-Plane_11/Resources/plugins";
          missionx_library_path = "r:/X-Plane_11/Resources/plugins";
#endif

        } // end if
      }
      CloseHandle(hProcess);



      missionx_library_path += BACKSLASHx2 + "64" + BACKSLASHx2;

      bool result = SetDllDirectory(missionx_library_path.c_str());
    }
    break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:

      break;
  }

  return TRUE;
}

#endif // if IBM


#include "core/Timer.hpp"
#include "core/Utils.h"
#include "core/base_xp_include.h"
#include "core/dataref_manager.h"
#include "core/xx_mission_constants.hpp"
#include "data/Task.h"
#include "mission.h"

using namespace std;
using namespace missionx;

namespace missionx
{
static Mission mission;
}

void
test1()
{
}

void
test2_numbers()
{
  std::string s = "true";
  bool        b = Utils::is_number(s);
  std::cout << "Is string '" << s << "' is digit: " << b << std::endl;
  s = "-123.5";
  b = Utils::is_number(s);
  std::cout << "Is string '" << s << "' is digit: " << b << std::endl;

  double d = 0.0;
  d        = Utils::stringToNumber<double>(s);
  if (b)
  {
    d = Utils::stringToNumber<double>(s);
    std::cout << "Value of D is: \"" << d << "\"" << std::endl;
  }
}

void
load_data_file(Mission& mission)
{
  // read_mission_file rmf;
  // rmf.load_mission_file("example01.xml");
  void* inItemRef = (void*)Mission::MENU_LOAD_DUMMY_MISSION;

  intptr_t menu = ((intptr_t)inItemRef);
  // mission.execMenuHandler(menu);
  mission.MissionMenuHandler(NULL, inItemRef);
}

void
chrono_timer()
{
  using nano_s  = std::chrono::nanoseconds;
  using micro_s = std::chrono::microseconds;
  using milli_s = std::chrono::milliseconds;
  using seconds = std::chrono::seconds;
  using minutes = std::chrono::minutes;
  using hours   = std::chrono::hours;

  //  std::clock_t start;
  double duration;

  //  auto start = std::clock();
  auto start = std::chrono::steady_clock::now(); //  clock();


  /* Your algorithm here */
  for (int i = 0; i < 50000; i++)
    int b = 0;

  // duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
  // std::cout << "printf: " << duration << '\n';

  auto end = std::chrono::steady_clock::now(); //  clock();


  auto d_nano  = std::chrono::duration_cast<nano_s>(end - start).count();
  auto d_micro = std::chrono::duration_cast<micro_s>(end - start).count();
  auto d_milli = std::chrono::duration_cast<milli_s>(end - start).count();
  auto d_s     = std::chrono::duration_cast<seconds>(end - start).count();
  auto d_m     = std::chrono::duration_cast<minutes>(end - start).count();
  auto d_h     = std::chrono::duration_cast<hours>(end - start).count();

  duration = (double)d_milli / 1000;

  std::cout << "sum:      " << 0 << "\n"
            << "d_nano:   " << d_nano << "\n"
            << "d_micro:  " << d_micro << "\n"
            << "d_milli:  " << d_milli << "\n"
            << "d_s:      " << d_s << "\n"
            << "d_m:      " << d_m << "\n"
            << "d_h:      " << d_h << "\n"
            << "duration(double)" << duration << "\n"
            << std::endl;

  /// test TimeFragment.hpp ///
  TimeFragment t1, t2;
  TimeFragment::init(t1);
  int b = 0;
  for (int i = 0; i < 100000; i++)
    b = b + i;

  TimeFragment::init(t2);

  std::cout << "TimeFragment t1 zulu sec: " << t1.getTimePassedSec() << endl;
  std::cout << "TimeFragment t1 zulu sec: " << t2 - t1 << endl;
}

void
ccw_test()
{
  missionx::Point p1 = Point(33.620621, -100.837260);
  missionx::Point p2 = Point(33.626717, -100.787964);
  missionx::Point p3 = Point(33.606756, -100.474835);
  missionx::Point p4 = Point(33.271770, -100.488510);
  missionx::Point p5 = Point(33.507755, -100.863375);

  Points p;
  p.addPoint(p1);
  p.addPoint(p2);
  p.addPoint(p3);
  p.addPoint(p4);
  p.addPoint(p5);

  Log::logMsg(p.to_string(), format_type::none);

  // Points::sortPointsGrahmScan(p);
  // Points::convex_hull(p.vecPoints);
  missionx::Point plane(33.422677, -100.542788);

  if (p.contains(33.422677, -100.542788))
    Log::logMsg("Contains");
  else
    Log::logMsg("Not Contains");


  if (p.isPointInPolyArea(plane))
    Log::logMsg("Plane in Area");
  else
    Log::logMsg("Plane is not in Area");


  plane = missionx::Point(33.329618, -100.750263);
  if (p.contains(33.329618, -100.750263))
    Log::logMsg("Contains");
  else
    Log::logMsg("Not Contains");

  if (p.isPointInPolyArea(plane))
    Log::logMsg("Plane in Area");
  else
    Log::logMsg("Plane is not in Area");


  Log::logMsg(p.to_string(), format_type::none);
}

void
slope()
{
  //  double x1, y1; x1 = y1 = 0.0;

  missionx::Point p1(5, 0);
  missionx::Point p2(10, 0);
  missionx::Point p3;
  missionx::Point p4;

  Log::logMsg(Utils::formatNumber<float>(Point::slope(p1, p2)), format_type::none);
  // https://stackoverflow.com/questions/37472810/slope-and-length-of-a-line-between-2-points-in-opencv

  p1 = Point(33.626717, -100.787964);
  Point::calcPointBasedOnDistanceAndBearing_2DPlane(p1, p2, 90.0f, 5000, mx_units_of_measure::meter);
  Point::calcPointBasedOnDistanceAndBearing_2DPlane(p2, p3, 0.0f, 300, mx_units_of_measure::meter);
  Point::calcPointBasedOnDistanceAndBearing_2DPlane(p2, p4, 180.0f, 300, mx_units_of_measure::meter);

  // Utils::calcPointBasedOnDistanceAndBearing_2DPlane(x1, y1, p1.lat, p1.lon, 90.0f, 5.0); // center
  // p2 = Point(x1, y1);
  // Utils::calcPointBasedOnDistanceAndBearing_2DPlane(x1, y1, p2.lat, p2.lon, 0.0f, 5.0); // upper
  // p3 = Point(x1, y1);
  // Utils::calcPointBasedOnDistanceAndBearing_2DPlane(x1, y1, p2.lat, p2.lon, 180.0f, 5.0); // lower
  // p4 = Point(x1, y1);



  Log::logMsgNone(p1.to_string_xy());
  Log::logMsgNone(p3.to_string_xy());
  Log::logMsgNone(p2.to_string_xy());
  Log::logMsgNone(p4.to_string_xy());
  Log::logMsg(Utils::formatNumber<float>(Point::slope(p1, p3)), format_type::none);
  Log::logMsg(Utils::formatNumber<float>(Point::slope(p1, p4)), format_type::none);



  Point::calcPointBasedOnDistanceAndBearing_2DPlane(p1, p2, 75, 3000, mx_units_of_measure::meter);
  Point::calcPointBasedOnDistanceAndBearing_2DPlane(p2, p3, 0.0f, 300, mx_units_of_measure::meter);
  Point::calcPointBasedOnDistanceAndBearing_2DPlane(p2, p4, 150, 300, mx_units_of_measure::meter);
  Log::logMsgNone(p1.to_string_xy());
  Log::logMsgNone(p3.to_string_xy());
  Log::logMsgNone(p2.to_string_xy());
  Log::logMsgNone(p4.to_string_xy());
  Log::logMsg(Utils::formatNumber<float>(Point::slope(p1, p3)), format_type::none);
  Log::logMsg(Utils::formatNumber<float>(Point::slope(p1, p4)), format_type::none);
}

void
save(Mission& mission)
{
  void* inItemRef = (void*)Mission::MENU_CREATE_SAVEPOINT;

  intptr_t menu = ((intptr_t)inItemRef);
  // mission.execMenuHandler(menu);
  mission.MissionMenuHandler(NULL, inItemRef);
}

void
loadSavePoint(Mission& mission)
{
  void* inItemRef = (void*)Mission::MENU_LOAD_SAVEPOINT;

  intptr_t menu = ((intptr_t)inItemRef);
  mission.MissionMenuHandler(NULL, inItemRef);
}

void
startMission(Mission& mission)
{
  void* inItemRef = (void*)Mission::MENU_START_MISSION;

  intptr_t menu = ((intptr_t)inItemRef);
  mission.MissionMenuHandler(NULL, inItemRef);
}


void
filterDataRefs(Mission& mission)
{
  std::string err;
  err.clear();
  // system_actions::read_and_filter_dataref_file("r:/X-Plane_11/Resources/plugins/DataRefs.txt", "r:/X-Plane_11/Resources/plugins/missionx/mx_datarefs.txt", err);
  system_actions::save_datarefs_with_savepoint(data_manager::mx_folders_properties.getPropertyValue(PROP_XPLANE_PLUGINS_PATH, err) + "/DataRefs.txt", std::string(MX_PLUGIN_FOLDER) + "/mx_datarefs.txt", err);

  if (err.empty())
    return;

  Log::logMsgNone(err);
}


void
testVectorPointer()
{
  std::unordered_set<std::string> vecUnStrings = { { "Hello" }, { "World" } };
  size_t                          size         = vecUnStrings.size();

  // for (auto a : vecUnStrings)
  //{
  //  a = "Walla";
  //}

  // works
  // std::vector <std::string > vecStrings = { {"Hello"}, {"World"} };
  // size_t size = vecStrings.size();
  // for (size_t i=0; i < size; i++)
  //{
  //  std::string *s = &vecStrings.at(i);
  //  (*s) = "Walla";
  //}


  // doesn't work
  // for (auto iter : vecStrings)
  //{
  //  std::string *s = &iter;
  //  (*s) = "Walla";
  //}

  for (auto s : vecUnStrings)
  {
    std::cout << s << ", ";
  }
}


void
loadMissionList(Mission& mission)
{
  void* inItemRef = (void*)Mission::MENU_OPEN_LIST_OF_MISSION;

  intptr_t menu = ((intptr_t)inItemRef);
  mission.MissionMenuHandler(NULL, inItemRef);
}

//////////////////////////////////////////
/////////////////////////////////////////
int
main()
{
  // dataref manager

  missionx::data_manager::clear();

  // missionx::dataref_manager dref;
  // test dataref manager
  // std::cout << "long: " << dref.getLong() << ", lat: " << dref.getLat() << ", elev: " << dref.getElevation() << std::endl;

  // test1();

  Timer t1;
  Timer::start(t1, 0, "main");


  loadMissionList(mission);

  // load(mission);

  // filterDataRefs(mission);
  // std::string strValue = "{0,15,40,40,0,0,0,0,0,0}";
  // Utils::replaceCharsWithString(strValue, "{}", "");
  // std::cout << "New value: " + strValue;

  // testVectorPointer();

  // std::string script1 = "myscripts.task1.LandInKOAJ";
  ////std::string script1 = "";
  // std::string scriptName;
  // std::string scriptFunc;

  // scriptName = Utils::extractBaseFromString(script1, ".", &scriptFunc);
  // Log::logMsg("script name: '" + scriptName + "', script function: " + scriptFunc); // debug

  // scriptName = Utils::extractLastFromString(script1, ".", &scriptFunc);
  // Log::logMsg("Lat of name: '" + scriptName + "', start of string: " + scriptFunc); // debug

  // load_data_file(missionx::mission);
  // save(missionx::mission);
  // startMission(mission);



  // loadSavePoint(mission);

  // ccw_test();
  // slope();

  // length_and_units
  // std::string lengthAndUnit = "1006km";
  // std::string num, unit;
  // Utils::extractUnitsFromString(lengthAndUnit, num, unit);
  // Log::logMsgNone("Length and Unit: " + lengthAndUnit + ", num: " + num + ", unit: " + unit);


  Log::logMsgNone("\n");
  Timer::wasEnded(t1);
  Log::logMsg(Timer::to_string(t1), format_type::none);


  ////// Dummy - Keep console open ///////////
  char c;
  std::cout << "Enter character and press [enter]" << std::endl;
  std::cin >> c;

  return 0;
}
