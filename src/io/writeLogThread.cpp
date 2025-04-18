#include <filesystem>
namespace fs = std::filesystem;

#include "writeLogThread.h"
#include <cassert>

namespace missionx
{
long long writeLogThread::seq; // v3.305.2
char writeLogThread::buff[21]; // v3.305.2

std::queue<std::string>        writeLogThread::qLogMessages;            // v3.0.217.8
std::queue<std::string>        writeLogThread::qLogMessages_mainThread; // v3.0.221.4
std::string                    writeLogThread::staticLogFilePath;
std::vector<std::future<bool>> writeLogThread::mWriteFuture;
std::vector<std::string>       writeLogThread::vecLogCycle = { "missionx.log", "missionx.1.log", "missionx.2.log", "missionx.3.log" };
writeLogThread::thread_state   writeLogThread::tState;
std::thread                    writeLogThread::thread_ref;
std::mutex                     writeLogThread::s_write_mutex;
}

missionx::writeLogThread::writeLogThread() = default;

// -------------------------------------

missionx::writeLogThread::~writeLogThread()
{
  if (writeLogThread::thread_ref.joinable())
    writeLogThread::thread_ref.join();
}

// -------------------------------------

void
missionx::writeLogThread::set_logFilePath(const std::string& inFilePath, const bool inCycleLogFiles)
{
  missionx::writeLogThread::staticLogFilePath = inFilePath;
  fs::path pathObj = inFilePath;
  fs::path base_folder = pathObj.parent_path();

  if (inCycleLogFiles)
  {
    int counter = 4;
    for (auto i = writeLogThread::vecLogCycle.rbegin (); i != writeLogThread::vecLogCycle.rend (); ++i)
    {
      fs::path logFile = base_folder.string () + "/" + (*i);
      if (counter == 4)
      {
        if (fs::exists (logFile))
          fs::remove (logFile);

        --counter;
        continue;
      }

      fs::path logFilePrev = base_folder.string () + "/" + (*--i);
      ++i; // one step back and one forward
      if (fs::exists (logFile))
      {
        fs::copy_file (logFile, logFilePrev, fs::copy_options::overwrite_existing);
      }

      --counter;

      if (counter <= 0)
        break;
    } // end cycle log files
  } // end !inDontCycleLogFile

  // Prepare the file
  std::ofstream ofs;
  ofs.open(missionx::writeLogThread::staticLogFilePath.c_str(), std::ofstream::trunc); // can we create/open the file ?
  if (ofs.fail())
  {
    assert(false && "Fail to open plugin log file");
  }
  ofs.close();

}


// -------------------------------------

void
missionx::writeLogThread::flc()
{
  const auto lmbda_reset_local_thread =[&]()
  {
    if (writeLogThread::thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
      writeLogThread::thread_ref.join();       // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"

    writeLogThread::tState.init();
  };

  // If thread work finished, we can join the thread
  if (writeLogThread::tState.flagThreadDoneWork + writeLogThread::tState.flagAbortThread)
  {
    lmbda_reset_local_thread();
  }
  else if (!writeLogThread::tState.flagIsActive && !writeLogThread::qLogMessages.empty()) // start thread
  {
    lmbda_reset_local_thread();
    writeLogThread::thread_ref = std::thread(&writeLogThread::exec_thread, &writeLogThread::tState);
  }
}

// -------------------------------------

void
missionx::writeLogThread::stop_plugin()
{
  writeLogThread::tState.flagAbortThread = true;
  if (writeLogThread::thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
    writeLogThread::thread_ref.join();       // joining also solved our issue with crashing xplane. error: abort() was called from "win.xpl"

  writeLogThread::clear_all_messages();
}

// -------------------------------------

void
missionx::writeLogThread::add_message(const std::string& inMsg)
{
  if (!writeLogThread::tState.flagAbortThread)
  {
    missionx::writeLogThread::qLogMessages.emplace(fmt::format("missionx T({}): {}", ++writeLogThread::seq, inMsg) );
  }
}


// -------------------------------------

void
missionx::writeLogThread::clear_all_messages()
{
  while (!missionx::writeLogThread::qLogMessages.empty())
    missionx::writeLogThread::qLogMessages.pop();
}

// -------------------------------------

void
missionx::writeLogThread::init()
{
  writeLogThread::tState.init();
}

// -------------------------------------

void
missionx::writeLogThread::exec_thread(thread_state* xxthread_state)
{
  std::lock_guard<std::mutex> lock(writeLogThread::s_write_mutex);

  static std::ofstream ofs;

  xxthread_state->flagIsActive       = true;
  xxthread_state->flagThreadDoneWork = false;
  xxthread_state->flagAbortThread    = false;

#ifdef ENABLE_MISSIONX_LOG

  if (!xxthread_state->flagAbortThread && !(missionx::writeLogThread::staticLogFilePath.empty()))
  {
    ofs.open(missionx::writeLogThread::staticLogFilePath.c_str(), std::ofstream::app); // can we create/open the file ?
    if (ofs.fail())
    {
      assert(false && "Fail to open plugin log file");

      missionx::writeLogThread::qLogMessages_mainThread.emplace(std::string("Fail to open file: ") + missionx::writeLogThread::staticLogFilePath);
    }
  }
#endif

  while (!xxthread_state->flagAbortThread && !missionx::writeLogThread::qLogMessages.empty())
  {
    auto msg = missionx::writeLogThread::qLogMessages.front();

    if (!qLogMessages.empty())
      qLogMessages.pop();

#ifdef ENABLE_MISSIONX_LOG
    if (ofs.is_open()) // skip if file is empty or failed open the file
    {
      ///// Write to missionx.log /////
      // try to write
      if (ofs)
        ofs.write(msg.c_str(), msg.length()); // //ofs << msg;
      

      if (ofs.bad())
      {
        missionx::writeLogThread::qLogMessages_mainThread.push(std::string("Fail to write to log file: ") + staticLogFilePath);
        break;
      }
    } // end if file is open state
#endif

  } // end while loop

  if (ofs.is_open())
    ofs.close();


  xxthread_state->flagThreadDoneWork = true;

}
