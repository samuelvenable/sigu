/*

 MIT License
 
 Copyright © 2026 Samuel Venable
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
*/

#include "system.hpp"
#include "libfiledialogs/source/apifilesystem/filesysten.h"
#include "libfiledialogs/source/apifiledialogs/filedialogs.h"
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <climits>
#endif
#if (defined(__APPLE__) && defined(__MACH__))
#include <mach-o/dyld.h>
#endif
#include <cstdlib>
#if ((defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) || defined(__DragonFly__) || defined(__NetBSD__))
#include <sys/sysctl.h>
#if defined(__DragonFly__)
#include <alloca.h>
#endif
#endif
#if defined(__OpenBSD__)
#include <cstddef>
#include <sys/sysctl.h>
#include <kvm.h>
#endif

using namespace ngs::si;

#define SYSINFO std::string(((os_device_name() != std::string("(null)")) ? (std::string("OS DEVICE NAME: ") + os_device_name() + std::string("\n")) : "") +\
((os_product_name() != std::string("(null)")) ? (std::string("OS PRODUCT NAME: ") + os_product_name() + std::string("\n")) : "") +\
((os_kernel_name() != std::string("(null)")) ? (std::string("OS KERNEL NAME: ") + os_kernel_name() + std::string("\n")) : "") +\
((os_kernel_release() != std::string("(null)")) ? (std::string("OS KERNEL RELEASE: ") + os_kernel_release() + std::string("\n")) : "") +\
((os_architecture() != std::string("(null)")) ? (std::string("OS ARCHITECTURE: ") + os_architecture() + std::string("\n")) : "") +\
((cpu_processor() != std::string("(null)")) ? (std::string("CPU PROCESSOR: ") + cpu_processor() + std::string("\n")) : "") +\
((cpu_vendor() != std::string("(null)")) ? (std::string("CPU VENDOR: ") + cpu_vendor() + std::string("\n")) : "") +\
((cpu_core_count() != std::string("(null)")) ? (std::string("CPU CORE COUNT: ") + cpu_core_count() + std::string("\n")) : "") +\
((cpu_processor_count() != std::string("(null)")) ? (std::string("CPU PROCESSOR COUNT: ") + cpu_processor_count() + std::string("\n")) : "") +\
((memory_totalram(true) != std::string("(null)")) ? (std::string("RANDOM-ACCESS MEMORY TOTAL: ") + memory_totalram(true) + std::string("\n")) : "") +\
((memory_usedram(true) != std::string("(null)")) ? (std::string("RANDOM-ACCESS MEMORY USED: ") + memory_usedram(true) + std::string("\n")) : "") +\
((memory_freeram(true) != std::string("(null)")) ? (std::string("RANDOM-ACCESS MEMORY FREE: ") + memory_freeram(true) + std::string("\n")) : "") +\
((memory_totalswap(true) != std::string("(null)")) ? (std::string("SWAP MEMORY TOTAL: ") + memory_totalswap(true) + std::string("\n")) : "") +\
((memory_usedswap(true) != std::string("(null)")) ? (std::string("SWAP MEMORY USED: ") + memory_usedswap(true) + std::string("\n")) : "") +\
((memory_freeswap(true) != std::string("(null)")) ? (std::string("SWAP MEMORY FREE: ") + memory_freeswap(true) + std::string("\n")) : "") +\
((gpu_manufacturer() != std::string("(null)")) ? (std::string("GPU MANUFACTURER: ") + gpu_manufacturer() + std::string("\n")) : "") +\
((gpu_renderer() != std::string("(null)")) ? (std::string("GPU RENDERER: ") + gpu_renderer() + std::string("\n")) : "") +\
((memory_totalvram(true) != std::string("(null)")) ? (std::string("GPU MEMORY: ") + memory_totalvram(true) + std::string("\n")) : ""))

static std::string pidEnv;
static std::string pipeName;

void string_send() {
  #if defined(_WIN32)
  pidEnv = std::to_string(GetCurrentProcessId());
  SetEnvironmentVariableA("IMGUI_DIALOG_PID", pidEnv.c_str());
  pipeName = std::string("\\\\.\\pipe\\IMGUI_DIALOG_PIPE_") + pidEnv;
  HANDLE hPipe = CreateNamedPipeA(pipeName.c_str(), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_NOWAIT, 1, 0, 0, 0, nullptr);
  if (hPipe == INVALID_HANDLE_VALUE) {
    return;
  }
  bool connected = ConnectNamedPipe(hPipe, nullptr) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);
  if (!connected) {
    CloseHandle(hPipe);
    return;
  }
  DWORD bytesWritten = 0;
  WriteFile(hPipe, SYSINFO.c_str(), (DWORD)SYSINFO.length() + 1, &bytesWritten, nullptr);
  CloseHandle(hPipe);
  #else
  int fd = 0;
  pidEnv = std::to_string(getpid());
  setenv("IMGUI_DIALOG_PID", pidEnv.c_str(), 1);
  pipeName = std::string("/tmp/IMGUI_DIALOG_PIPE_") + pidEnv;
  if (mkfifo(pipeName.c_str(), 0666) != 0) {
    if (errno != EEXIST) {
      return;
    }
  }
  fd = open(pipeName.c_str(), O_WRONLY);
  if (fd == -1) {
    return;
  }
  write(fd, SYSINFO.c_str(), SYSINFO.length() + 1);
  close(fd);
  #endif
}

static std::atomic_bool stop_thread = false;
static void thloop() {
  while (!stop_thread) {
    string_send();
    #if !defined(_WIN32)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    #endif
  }
}

int main() {
  std::thread th(thloop);
  if (!ngs::fs::environment_get_variable_exists("IMGUI_DIALOG_THEME")) ngs::fs::environment_set_variable("IMGUI_DIALOG_THEME", std::to_string(2).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_TEXT_COLOR_0")) ngs::fs::environment_set_variable("IMGUI_TEXT_COLOR_0", std::to_string(1).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_TEXT_COLOR_1")) ngs::fs::environment_set_variable("IMGUI_TEXT_COLOR_1", std::to_string(1).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_TEXT_COLOR_2")) ngs::fs::environment_set_variable("IMGUI_TEXT_COLOR_2", std::to_string(1).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_HEAD_COLOR_0")) ngs::fs::environment_set_variable("IMGUI_HEAD_COLOR_0", std::to_string(0.35).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_HEAD_COLOR_1")) ngs::fs::environment_set_variable("IMGUI_HEAD_COLOR_1", std::to_string(0.55).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_HEAD_COLOR_2")) ngs::fs::environment_set_variable("IMGUI_HEAD_COLOR_2", std::to_string(0.55).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_AREA_COLOR_0")) ngs::fs::environment_set_variable("IMGUI_AREA_COLOR_0", std::to_string(0.18).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_AREA_COLOR_1")) ngs::fs::environment_set_variable("IMGUI_AREA_COLOR_1", std::to_string(0.18).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_AREA_COLOR_2")) ngs::fs::environment_set_variable("IMGUI_AREA_COLOR_2", std::to_string(0.18).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_BODY_COLOR_0")) ngs::fs::environment_set_variable("IMGUI_BODY_COLOR_0", std::to_string(1).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_BODY_COLOR_1")) ngs::fs::environment_set_variable("IMGUI_BODY_COLOR_1", std::to_string(1).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_BODY_COLOR_2")) ngs::fs::environment_set_variable("IMGUI_BODY_COLOR_2", std::to_string(1).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_POPS_COLOR_0")) ngs::fs::environment_set_variable("IMGUI_POPS_COLOR_0", std::to_string(0.07).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_POPS_COLOR_1")) ngs::fs::environment_set_variable("IMGUI_POPS_COLOR_1", std::to_string(0.07).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_POPS_COLOR_2")) ngs::fs::environment_set_variable("IMGUI_POPS_COLOR_2", std::to_string(0.07).c_str());
  if (!ngs::fs::environment_get_variable_exists("IMGUI_DIALOG_CAPTION")) ngs::fs::environment_set_variable("IMGUI_DIALOG_CAPTION", "About this Computer");
  if (!ngs::fs::environment_get_variable_exists("IMGUI_DIALOG_PARENT")) ngs::fs::environment_set_variable("IMGUI_DIALOG_PARENT", "");
  if (!ngs::fs::environment_get_variable_exists("IMGUI_DIALOG_WIDTH")) ngs::fs::environment_set_variable("IMGUI_DIALOG_WIDTH", std::to_string(800));
  #if defined(_WIN32)
  if (!ngs::fs::environment_get_variable_exists("IMGUI_FONT_FILES")) ngs::fs::environment_set_variable("IMGUI_FONT_FILES", ngs::fs::executable_get_directory() + "fonts\\Roboto\\Roboto-Medium.ttf");
  #else
  if (!ngs::fs::environment_get_variable_exists("IMGUI_FONT_FILES")) ngs::fs::environment_set_variable("IMGUI_FONT_FILES", ngs::fs::executable_get_directory() + "fonts/Roboto/Roboto-Medium.ttf");
  #endif
  if (!ngs::fs::environment_get_variable_exists("IMGUI_FONT_SIZE")) ngs::fs::environment_set_variable("IMGUI_FONT_SIZE", std::to_string(20));
  ngs::im::show_message(SYSINFO);
  stop_thread = true;
  th.join();
  stop_thread = false;
  #if !defined(_WIN32)
  unlink(pipeName.c_str());
  #endif
  return 0;
}
