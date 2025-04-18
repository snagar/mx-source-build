#include "XPLMDataAccess.h"
#include "XPLMMenus.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"

// This option enables support for multi threaded crash handling.
// It requires C++11 or newer
#define SUPPORT_BACKGROUND_THREADS 1

#include <cstdlib>
#include <cstring>

#if SUPPORT_BACKGROUND_THREADS
#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#endif

#if APL || LIN
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#endif

/*
 * This plugin demonstrates how to intercept and handle crashes in a way that plays nicely with the X-Plane crash reporter.
 * The core idea is to intercept all crashes by installing appropriate crash handlers and then filtering out crashes that weren't caused
 * by our plugin. To do so, we track both the threads created by us at runtime, as well as checking whether our plugin is active when we crash on the main thread.
 * If the crash wasn't caused by us, the crash is forwarded to the next crash handler (potentially X-Plane, or another plugin) which then gets a chance
 * to process the crash.
 * If the crash is caused by us, we eat the crash to not falsely trigger X-Planes crash reporter and pollute the X-Plane crash database. This example
 * also writes a minidump file on Windows and a simple backtrace on macOS and Linux. Production code might want to integrate more sophisticated crash handling
 */

#if SUPPORT_BACKGROUND_THREADS
static std::thread::id           s_main_thread;
static std::atomic_flag          s_thread_lock;
static std::set<std::thread::id> s_known_threads;
#endif

static XPLMPluginID s_my_plugin_id;

// Function called when we detect a crash that was caused by us
void handle_crash(void* context);

#if APL || LIN
static struct sigaction s_prev_sigsegv = {};
static struct sigaction s_prev_sigabrt = {};
static struct sigaction s_prev_sigfpe  = {};
static struct sigaction s_prev_sigint  = {};
static struct sigaction s_prev_sigill  = {};
static struct sigaction s_prev_sigterm = {};

static void handle_posix_sig(int sig, siginfo_t* siginfo, void* context);
#endif

#if IBM
static LPTOP_LEVEL_EXCEPTION_FILTER s_previous_windows_exception_handler;
LONG WINAPI                         handle_windows_exception(EXCEPTION_POINTERS* ei);
#endif

#if SUPPORT_BACKGROUND_THREADS
// Registers the calling thread with the crash handler. We use this to figure out if a crashed thread belongs to us when we later try to figure out if we caused a crash
void
register_thread_for_crash_handler()
{
  while (s_thread_lock.test_and_set(std::memory_order_acquire))
  {
  }

  s_known_threads.insert(std::this_thread::get_id());

  s_thread_lock.clear(std::memory_order_release);
}

// Unregisters the calling thread from the crash handler. MUST be called at the end of thread that was registered via register_thread_for_crash_handler()
void
unregister_thread_from_crash_handler()
{
  while (s_thread_lock.test_and_set(std::memory_order_acquire))
  {
  }

  s_known_threads.erase(std::this_thread::get_id());

  s_thread_lock.clear(std::memory_order_release);
}
#endif

// Registers the global crash handler. Should be called from XPluginStart
void
register_crash_handler()
{
#if SUPPORT_BACKGROUND_THREADS
  s_main_thread = std::this_thread::get_id();
#endif
  s_my_plugin_id = XPLMGetMyID();

#if APL || LIN
  struct sigaction sig_action = { .sa_sigaction = handle_posix_sig };

  sigemptyset(&sig_action.sa_mask);

#if LIN
  static uint8_t alternate_stack[SIGSTKSZ];
  stack_t        ss = { .ss_sp = (void*)alternate_stack, .ss_size = SIGSTKSZ, .ss_flags = 0 };

  sigaltstack(&ss, NULL);
  sig_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
#else
  sig_action.sa_flags = SA_SIGINFO;
#endif

  sigaction(SIGSEGV, &sig_action, &s_prev_sigsegv);
  sigaction(SIGABRT, &sig_action, &s_prev_sigabrt);
  sigaction(SIGFPE, &sig_action, &s_prev_sigfpe);
  sigaction(SIGINT, &sig_action, &s_prev_sigint);
  sigaction(SIGILL, &sig_action, &s_prev_sigill);
  sigaction(SIGTERM, &sig_action, &s_prev_sigterm);
#endif

#if IBM
  // Load the debug helper library into the process already, this way we don't have to hit the dynamic loader
  // in an exception context where it's potentially unsafe to do so.
  HMODULE module = ::GetModuleHandleA("dbghelp.dll");
  if (!module)
    module = ::LoadLibraryA("dbghelp.dll");

  (void)module;
  s_previous_windows_exception_handler = SetUnhandledExceptionFilter(handle_windows_exception);
#endif
}

// Unregisters the global crash handler. You need to call this in XPluginStop so we can clean up after ourselves
void
unregister_crash_handler()
{
#if APL || LIN
  sigaction(SIGSEGV, &s_prev_sigsegv, NULL);
  sigaction(SIGABRT, &s_prev_sigabrt, NULL);
  sigaction(SIGFPE, &s_prev_sigfpe, NULL);
  sigaction(SIGINT, &s_prev_sigint, NULL);
  sigaction(SIGILL, &s_prev_sigill, NULL);
  sigaction(SIGTERM, &s_prev_sigterm, NULL);
#endif

#if IBM
  SetUnhandledExceptionFilter(s_previous_windows_exception_handler);
#endif
}

#if SUPPORT_BACKGROUND_THREADS
// A RAII helper class to register and unregister threads to participate in crash detection
class StThreadCrashCookie
{
public:
  StThreadCrashCookie() { register_thread_for_crash_handler(); }
  ~StThreadCrashCookie() { unregister_thread_from_crash_handler(); }
};
#endif


// Predicates that returns true if a thread is caused by us
// The main idea is to check the plugin ID if we are on the main thread,
// if not, we check if the current thread is known to be from us.
// Returns false if the crash was caused by code that didn't come from our plugin
bool
is_us_executing()
{
#if SUPPORT_BACKGROUND_THREADS
  const std::thread::id thread_id = std::this_thread::get_id();

  if (thread_id == s_main_thread)
  {
    // Check if the plugin executing is our plugin.
    // XPLMGetMyID() will return the ID of the currently executing plugin. If this is us, then it will return the plugin ID that we have previously stashed away
    return (s_my_plugin_id == XPLMGetMyID());
  }

  if (s_thread_lock.test_and_set(std::memory_order_acquire))
  {
    // We couldn't acquire our lock. In this case it's better if we just say it's not us so we don't eat the exception
    return false;
  }

  const bool is_our_thread = (s_known_threads.find(thread_id) != s_known_threads.end());
  s_thread_lock.clear(std::memory_order_release);

  return is_our_thread;
#else
  return (s_my_plugin_id == XPLMGetMyID());
#endif
}

#if APL || LIN

static void
handle_posix_sig(int sig, siginfo_t* siginfo, void* context)
{
  if (is_us_executing())
  {
    static bool has_called_out = false;

    if (!has_called_out)
    {
      has_called_out = true;
      handle_crash((void*)sig);
    }

    abort();
  }

  // Forward the signal to the other handlers
#define FORWARD_SIGNAL(sigact)                                       \
  do                                                                 \
  {                                                                  \
    if ((sigact)->sa_sigaction && ((sigact)->sa_flags & SA_SIGINFO)) \
      (sigact)->sa_sigaction(sig, siginfo, context);                 \
    else if ((sigact)->sa_handler)                                   \
      (sigact)->sa_handler(sig);                                     \
  } while (0)

  switch (sig)
  {
    case SIGSEGV:
      FORWARD_SIGNAL(&s_prev_sigsegv);
      break;
    case SIGABRT:
      FORWARD_SIGNAL(&s_prev_sigabrt);
      break;
    case SIGFPE:
      FORWARD_SIGNAL(&s_prev_sigfpe);
      break;
    case SIGILL:
      FORWARD_SIGNAL(&s_prev_sigill);
      break;
    case SIGTERM:
      FORWARD_SIGNAL(&s_prev_sigterm);
      break;
  }

#undef FORWARD_SIGNAL

  abort();
}

#endif

#if IBM
LONG WINAPI
handle_windows_exception(EXCEPTION_POINTERS* ei)
{
  if (is_us_executing())
  {
    handle_crash(ei);
    return EXCEPTION_CONTINUE_SEARCH;
  }

  if (s_previous_windows_exception_handler)
    return s_previous_windows_exception_handler(ei);

  return EXCEPTION_CONTINUE_SEARCH;
}
#endif

// Runtime
#if IBM
void write_mini_dump(PEXCEPTION_POINTERS exception_pointers);
#endif

void
handle_crash(void* context)
{
#if APL || LIN
  // NOTE: This is definitely NOT production code
  // backtrace and backtrace_symbols are NOT signal handler safe and are just put in here for demonstration purposes
  // A better alternative would be to use something like libunwind here

  void*  frames[64];
  int    frame_count = backtrace(frames, 64);
  char** names       = backtrace_symbols(frames, frame_count);

  const int fd = open("backtrace.txt", O_CREAT | O_RDWR | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd >= 0)
  {
    for (int i = 0; i < frame_count; ++i)
    {
      write(fd, names[i], strlen(names[i]));
      write(fd, "\n", 1);
    }

    close(fd);
  }

#endif
#if IBM
  // Create a mini-dump file that can be later opened up in Visual Studio or WinDbg to do post mortem debugging
  write_mini_dump((PEXCEPTION_POINTERS)context);
#endif
}

#if IBM
#include <DbgHelp.h>

typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(HANDLE                                  hProcess,
                                        DWORD                                   dwPid,
                                        HANDLE                                  hFile,
                                        MINIDUMP_TYPE                           DumpType,
                                        CONST PMINIDUMP_EXCEPTION_INFORMATION   ExceptionParam,
                                        CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                        CONST PMINIDUMP_CALLBACK_INFORMATION    CallbackParam);

void
write_mini_dump(PEXCEPTION_POINTERS exception_pointers)
{
  HMODULE module = ::GetModuleHandleA("dbghelp.dll");
  if (!module)
    module = ::LoadLibraryA("dbghelp.dll");

  if (module)
  {
    const MINIDUMPWRITEDUMP pDump = MINIDUMPWRITEDUMP(::GetProcAddress(module, "MiniDumpWriteDump"));

    if (pDump)
    {
      // Create dump file
      const HANDLE handle = ::CreateFileA("crash_dump.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

      if (handle != INVALID_HANDLE_VALUE)
      {
        MINIDUMP_EXCEPTION_INFORMATION exception_information = {};
        exception_information.ThreadId                       = ::GetCurrentThreadId();
        exception_information.ExceptionPointers              = exception_pointers;
        exception_information.ClientPointers                 = false;

        pDump(GetCurrentProcess(), GetCurrentProcessId(), handle, MiniDumpNormal, &exception_information, nullptr, nullptr);
        ::CloseHandle(handle);
      }
    }
  }
}
#endif

#if SUPPORT_BACKGROUND_THREADS
static std::thread       s_background_thread;
static std::atomic<bool> s_background_thread_shutdown(false);
static std::atomic<bool> s_background_thread_want_crash(false);

void
background_thread_func()
{
  StThreadCrashCookie thread_cookie; // Make sure our thread is registered with the crash handling system

  while (!s_background_thread_shutdown.load(std::memory_order_acquire))
  {
    if (s_background_thread_want_crash.load(std::memory_order_acquire))
    {
      s_background_thread_want_crash = false;

      int* death_is_a_destination = NULL;
      *death_is_a_destination     = 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
#endif

// User Interface
#define MENU_ITEM_CRASH_MAIN_THREAD       (void*)0x1
#define MENU_ITEM_CRASH_BACKGROUND_THREAD (void*)0x2

void
menu_handler(void* inMenuRef, void* inItemRef)
{
  if (inItemRef == MENU_ITEM_CRASH_MAIN_THREAD)
  {
    int* death_is_a_destination = NULL;
    *death_is_a_destination     = 1;
  }
#if SUPPORT_BACKGROUND_THREADS
  else if (inItemRef == MENU_ITEM_CRASH_BACKGROUND_THREAD)
  {
    s_background_thread_want_crash = true;
  }
#endif
}

// Plugin functions

//PLUGIN_API int
//XPluginStart(char* name, char* sig, char* desc)
//{
//  strcpy(name, "Crash Handling example");
//  strcpy(sig, "com.laminarresearch.example.crash-handling");
//  strcpy(desc, "Example plugin for crash handling");
//
//  register_crash_handler();
//
//#if SUPPORT_BACKGROUND_THREADS
//  s_background_thread = std::thread(&background_thread_func);
//#endif
//
//  const int        plugin_menu_item = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "Crash Handler", NULL, 0);
//  const XPLMMenuID menu             = XPLMCreateMenu("Crash Handler", XPLMFindPluginsMenu(), plugin_menu_item, menu_handler, NULL);
//
//  XPLMAppendMenuItem(menu, "Crash main thread", MENU_ITEM_CRASH_MAIN_THREAD, 0);
//#if SUPPORT_BACKGROUND_THREADS
//  XPLMAppendMenuItem(menu, "Crash background thread", MENU_ITEM_CRASH_BACKGROUND_THREAD, 0);
//#endif
//
//  return 1;
//}
//
//PLUGIN_API void
//XPluginStop(void)
//{
//#if SUPPORT_BACKGROUND_THREADS
//  // Tell the background thread to shutdown and then wait for it
//  s_background_thread_shutdown.store(true, std::memory_order_release);
//  s_background_thread.join();
//#endif
//
//  unregister_crash_handler();
//}
//
//
//PLUGIN_API int
//XPluginEnable(void)
//{
//  return 1;
//}
//
//PLUGIN_API void
//XPluginDisable(void)
//{
//}
//
//PLUGIN_API void
//XPluginReceiveMessage(XPLMPluginID from, int msg, void* param)
//{
//}