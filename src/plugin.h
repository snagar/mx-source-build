#ifndef PLUGIN_H_
#define PLUGIN_H_

#pragma once

#include <XPLMPlugin.h>

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
#include <XPLMUtilities.h>


// v26.08.2 Disabled DllMain LoadLibraryExA implementation since we statically linked libcurl into the plugin.
// Global handle to preserve your loaded library instance
//HMODULE g_hCurlModule = NULL;
//#define MX_MAX_PATH 2048
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    //{
    //  // 1. Get absolute file path of missionx.xpl
    //  char  pluginPath[MX_MAX_PATH];
    //  DWORD length = GetModuleFileNameA(hModule, pluginPath, MX_MAX_PATH);

    //  if (length > 0 && length < MX_MAX_PATH)
    //  {
    //    // 2. Extract directory (e.g., ".../missionx/win_x64/")
    //    std::string pathStr(pluginPath);
    //    size_t      lastSlash = pathStr.find_last_of("\\/");
    //    if (lastSlash != std::string::npos)
    //    {
    //      std::string pluginDir = pathStr.substr(0, lastSlash);

    //      const std::string debug_msg = "\nmissionx: Manually loading libcurl from: " + pluginDir + "\n\n";
    //      XPLMDebugString(debug_msg.c_str());


    //      // 3. Build full path to your specific cURL DLL
    //      std::string curlPath = pluginDir + "\\libcurl.dll"; // Adjust filename if named curl.dll

    //      // 4. Force Windows to load YOUR libcurl and resolve its dependencies from your directory
    //      g_hCurlModule = LoadLibraryExA(curlPath.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    //    }
    //  }
    //  break;
    //}

  case DLL_PROCESS_DETACH:
    //{
    //  // Free the module on unload if it was loaded successfully
    //  if (g_hCurlModule != NULL)
    //  {
    //    FreeLibrary(g_hCurlModule);
    //    g_hCurlModule = NULL;
    //  }
    //  break;
    //}

  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;
  }

  return TRUE;
}
#endif // if IBM



#ifdef __cplusplus
extern "C" {
#endif

PLUGIN_API int  XPluginStart(char* outName, char* outSig, char* outDesc);
PLUGIN_API void XPluginStop(void);
PLUGIN_API void XPluginDisable(void);
PLUGIN_API int  XPluginEnable(void);
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, intptr_t inMessage, void* inParam);

#ifdef __cplusplus
}
#endif


#endif // PLUGIN_H_
