#include "plugin_segfault/plugin_segfault.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#include <cstring>
#include <unistd.h>
#endif

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/stacktrace.hpp>
#include <filesystem>
#include <fstream>

namespace {

struct LogInitializer
{
  LogInitializer()
  {
    boost::log::add_common_attributes();
    boost::log::add_console_log();
  }
};


void ensure_log_initialized() { static LogInitializer init; }

}// namespace
//

void write_stacktrace_to_file(int signum)
{
  std::filesystem::path filepath = std::filesystem::current_path() / "crash_stacktrace.txt";
  std::ofstream file(filepath);
  if (file.is_open()) {
    file << "Signal: " << signum << "\n"
         << "Stacktrace:\n"
         << boost::stacktrace::stacktrace() << std::endl;
  }
}
#ifdef _WIN32

LONG WINAPI win_crash_handler(EXCEPTION_POINTERS *info)
{
  std::cerr << "Exception code: 0x" << std::hex << info->ExceptionRecord->ExceptionCode << std::dec << "\n";
  write_stacktrace_to_file(info->ExceptionRecord->ExceptionCode);
  return EXCEPTION_CONTINUE_SEARCH;
}

void setup_crash_handlers() { SetUnhandledExceptionFilter(win_crash_handler); }

#else

void linux_crash_handler(int signum, siginfo_t *info, void *context)
{
  const char *msg = "Signal caught\n";
  write(STDERR_FILENO, msg, strlen(msg));
  std::cerr << "Signal: " << signum << " (" << strsignal(signum) << ")\n";
  write_stacktrace_to_file(signum);

  struct sigaction sa;
  sa.sa_handler = SIG_DFL;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(signum, &sa, nullptr);
  raise(signum);
}

void setup_crash_handlers()
{
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = linux_crash_handler;
  sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGSEGV, &sa, nullptr);
  sigaction(SIGABRT, &sa, nullptr);
  sigaction(SIGFPE, &sa, nullptr);
  sigaction(SIGBUS, &sa, nullptr);
  sigaction(SIGILL, &sa, nullptr);
}

#endif

extern "C" {

PLUGIN_API int plugin_segfault_init(void) {
  ensure_log_initialized();
  setup_crash_handlers();
  BOOST_LOG_TRIVIAL(info) << "Segfault plugin loaded — handler installed";
  return 0;
}

PLUGIN_API const char *plugin_segfault_get_name(void) { return "segfault_plugin"; }
}
