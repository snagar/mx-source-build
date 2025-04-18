#ifndef PLUGIN_H_
#define PLUGIN_H_

#pragma once


#ifdef IBM

#include <tchar.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
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
  // PCWSTR missionx_library_path; missionx_library_path.clear();
  std::string xp_dll_folder_path;
  xp_dll_folder_path.clear();
  HMODULE fmod_module1 = NULL;

  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    {
      //// Utils::logMsg("Process Attached"); // debug

      //// Add plugin library folder to the "search dir".
      //HMODULE hMods[1024];
      //HANDLE  hProcess;
      //DWORD   cbNeeded;
      //// unsigned int i;

      //DWORD  processID = GetCurrentProcessId();
      //LPTSTR lpFilename;
      //DWORD  nSize = sizeof(lpFilename);

      //constexpr auto CURRENT_DLL_PATHS_W = 1024 * 5;
      //LPTSTR         currentDllPath;

      //hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
      //if (NULL == hProcess)
      //  break;

      //if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
      //{
      //  // char nodeCName[MAX_PATH]; // dummy
      //  TCHAR szModName[1025];

      //  if (GetModuleFileNameEx(hProcess, hMods[0], szModName, sizeof(szModName)))
      //  {
      //    std::string executableName = std::string(szModName);
      //    size_t      lastBackslash  = executableName.find_last_of(BACKSLASHx2.c_str());
      //    std::string base_dir       = executableName.substr(0, lastBackslash);
      //    xp_dll_folder_path         = base_dir + BACKSLASHx2 + "Resources" + BACKSLASHx2 + "dlls" + BACKSLASHx2 + "64"; // v2.1.29.2
      //    missionx_library_path      = base_dir + BACKSLASHx2 + "Resources" + BACKSLASHx2 + "plugins" + BACKSLASHx2 + "missionx" + BACKSLASHx2 + "libs";

      //  } // end if
      //}
      //CloseHandle(hProcess);


      //auto length_of_current_search_path = GetDllDirectory(CURRENT_DLL_PATHS_W, currentDllPath);
      //missionx_library_path += BACKSLASHx2 + "64" + BACKSLASHx2;
      //bool result = SetDllDirectory(missionx_library_path.c_str());
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



#endif // PLUGIN_H_
