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


// v26.08.1 disabled as per gemini suggestion to avoid overriding other plugins dll search path. This is a known issue with X-Plane and the way it handles DLLs. The recommended approach is to use the X-Plane SDK's built-in functions to load DLLs from the plugin's own directory, rather than modifying the global DLL search path.
BOOL APIENTRY
DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:

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
