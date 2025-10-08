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
      SetDllDirectoryA ("Resources\\dlls\\64");
      SetDllDirectoryA ("Resources\\plugins\\missionx\\win_x64\\dlls");
      SetDllDirectoryA ("Resources\\plugins\\missionx\\libs\\64");
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
