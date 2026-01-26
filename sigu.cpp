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
#include <string>
#include <sstream>
#include <fstream>
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
#include <sys/proc_info.h>
#include <libproc.h>
#endif
#include <cstdlib>
#if (defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__))
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

using namespace ngs::sys;

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

void string_send() {
  #if defined(_WIN32)
  const char *pipeName = R"(\\.\pipe\IMGUI_DIALOG_PIPE)";
  HANDLE hPipe = CreateNamedPipeA(pipeName, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_NOWAIT, 1, 0, 0, 0, nullptr);
  if (hPipe == INVALID_HANDLE_VALUE) {
    return;
  }
  bool connected = ConnectNamedPipe(hPipe, nullptr) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);
  if (!connected) {
    CloseHandle(hPipe);
    return;
  }
  DWORD bytesWritten = 0;
  WriteFile(hPipe, SYSINFO.c_str(), (DWORD)SYSINFO.size() + 1, &bytesWritten, nullptr);
  CloseHandle(hPipe);
  #else
  int fd = 0;
  if (mkfifo("/tmp/IMGUI_DIALOG_PIPE", 0666) != 0) {
    if (errno != EEXIST) {
      return;
    }
  }
  fd = open("/tmp/IMGUI_DIALOG_PIPE", O_WRONLY);
  if (fd == -1) {
    return;
  }
  write(fd, SYSINFO.c_str(), SYSINFO.size() + 1);
  close(fd);
  #endif
}

#if defined(_WIN32)
static std::wstring widen(std::string str) {
  if (str.empty()) return std::wstring(L"");
  std::size_t wchar_count = str.size() + 1;
  std::vector<wchar_t> buf(wchar_count);
  return std::wstring{ buf.data(), (std::size_t)MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buf.data(), (int)wchar_count) };
}

static std::string narrow(std::wstring wstr) {
  if (wstr.empty()) return std::string("");
  int nbytes = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), nullptr, 0, nullptr, nullptr);
  std::vector<char> buf(nbytes);
  return std::string { buf.data(), (std::size_t)WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), buf.data(), nbytes, nullptr, nullptr) };
}
#endif

static std::string get_executable_path() {
  std::string path;
  #if defined(_WIN32)
  wchar_t buffer[MAX_PATH];
  if (GetModuleFileNameW(nullptr, buffer, sizeof(buffer)) != 0) {
    wchar_t exe[MAX_PATH];
    if (_wfullpath(exe, buffer, MAX_PATH)) {
      path = narrow(exe);
    }
  }
  #elif (defined(__APPLE__) && defined(__MACH__))
  char exe[PROC_PIDPATHINFO_MAXSIZE];
  if (proc_pidpath(getpid(), exe, sizeof(exe)) > 0) {
    char buffer[PATH_MAX];
    if (realpath(exe, buffer)) {
      path = buffer;
    }
  }
  #elif (defined(__linux__) && !defined(__ANDROID__))
  char exe[PATH_MAX];
  if (realpath("/proc/self/exe", exe)) {
    path = exe;
  }
  #elif defined(__FreeBSD__) || defined(__DragonFly__)
  int mib[4]; 
  size_t len = 0;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = -1;
  if (sysctl(mib, 4, nullptr, &len, nullptr, 0) == 0) {
    std::string strbuff;
    strbuff.resize(len, '\0');
    char *exe = strbuff.data();
    if (sysctl(mib, 4, exe, &len, nullptr, 0) == 0) {
      char buffer[PATH_MAX];
      if (realpath(exe, buffer)) {
        path = buffer;
      }
    }
  }
  #elif defined(__NetBSD__)
  int mib[4]; 
  size_t len = 0;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC_ARGS;
  mib[2] = -1;
  mib[3] = KERN_PROC_PATHNAME;
  if (sysctl(mib, 4, nullptr, &len, nullptr, 0) == 0) {
    std::string strbuff;
    strbuff.resize(len, '\0');
    char *exe = strbuff.data();
    if (sysctl(mib, 4, exe, &len, nullptr, 0) == 0) {
      char buffer[PATH_MAX];
      if (realpath(exe, buffer)) {
        path = buffer;
      }
    }
  }
  #elif defined(__OpenBSD__)
  auto is_exe = [](std::string exe) {
    int cntp = 0;
    std::string res;
    kvm_t *kd = nullptr;
    kinfo_file *kif = nullptr;
    bool error = false;
    kd = kvm_openfiles(nullptr, nullptr, nullptr, KVM_NO_FILES, nullptr);
    if (!kd) return res;
    if ((kif = kvm_getfiles(kd, KERN_FILE_BYPID, getpid(), sizeof(struct kinfo_file), &cntp))) {
      for (int i = 0; i < cntp && kif[i].fd_fd < 0; i++) {
        if (kif[i].fd_fd == KERN_FILE_TEXT) {
          struct stat st;
          fallback:
          char buffer[PATH_MAX];
          if (!stat(exe.c_str(), &st) && (st.st_mode & S_IXUSR) &&
            (st.st_mode & S_IFREG) && realpath(exe.c_str(), buffer) &&
            st.st_dev == (dev_t)kif[i].va_fsid && st.st_ino == (ino_t)kif[i].va_fileid) {
            res = buffer;
          }
          if (res.empty() && !error) {
            error = true;
            std::size_t last_slash_pos = exe.find_last_of("/");
            if (last_slash_pos != std::string::npos) {
              exe = exe.substr(0, last_slash_pos + 1) + kif[i].p_comm;
              goto fallback;
            }
          }
          break;
        }
      }
    }
    kvm_close(kd);
    return res;
  };
  auto cppstr_getenv = [](std::string name) {
    const char *cresult = getenv(name.c_str());
    std::string result = cresult ? cresult : "";
    return result;
  };
  int cntp = 0;
  kvm_t *kd = nullptr;
  kinfo_proc *proc_info = nullptr;
  std::vector<std::string> buffer;
  bool error = false, retried = false;
  kd = kvm_openfiles(nullptr, nullptr, nullptr, KVM_NO_FILES, nullptr);
  if (!kd) {
    path.clear();
    return path;
  }
  if ((proc_info = kvm_getprocs(kd, KERN_PROC_PID, getpid(), sizeof(struct kinfo_proc), &cntp))) {
    char **cmd = kvm_getargv(kd, proc_info, 0);
    if (cmd) {
      for (int i = 0; cmd[i]; i++) {
        buffer.push_back(cmd[i]);
      }
    }
  }
  kvm_close(kd);
  if (!buffer.empty()) {
    std::string argv0;
    if (!buffer[0].empty()) {
      fallback:
      std::size_t slash_pos = buffer[0].find('/');
      std::size_t colon_pos = buffer[0].find(':');
      if (slash_pos == 0) {
        argv0 = buffer[0];
        path = is_exe(argv0);
      } else if (slash_pos == std::string::npos || slash_pos > colon_pos) { 
        std::string penv = cppstr_getenv("PATH");
        if (!penv.empty()) {
          retry:
          std::string tmp;
          std::stringstream sstr(penv);
          while (std::getline(sstr, tmp, ':')) {
            argv0 = tmp + "/" + buffer[0];
            path = is_exe(argv0);
            if (!path.empty()) break;
            if (slash_pos > colon_pos) {
              argv0 = tmp + "/" + buffer[0].substr(0, colon_pos);
              path = is_exe(argv0);
              if (!path.empty()) break;
            }
          }
        }
        if (path.empty() && !retried) {
          retried = true;
          penv = "/usr/bin:/bin:/usr/sbin:/sbin:/usr/X11R6/bin:/usr/local/bin:/usr/local/sbin";
          std::string home = cppstr_getenv("HOME");
          if (!home.empty()) {
            penv = home + "/bin:" + penv;
          }
          goto retry;
        }
      }
      if (path.empty() && slash_pos > 0) {
        std::string pwd = cppstr_getenv("PWD");
        if (!pwd.empty()) {
          argv0 = pwd + "/" + buffer[0];
          path = is_exe(argv0);
        }
        if (path.empty()) {
          char cwd[PATH_MAX];
          if (getcwd(cwd, PATH_MAX)) {
            argv0 = std::string(cwd) + "/" + buffer[0];
            path = is_exe(argv0);
          }
        }
      }
    }
    if (path.empty() && !error) {
      error = true;
      buffer.clear();
      std::string underscore = cppstr_getenv("_");
      if (!underscore.empty()) {
        buffer.push_back(underscore);
        goto fallback;
      }
    }
  }
  if (!path.empty()) {
    errno = 0;
  }
  #elif defined(__sun)
  char exe[PATH_MAX];
  if (realpath("/proc/self/path/a.out", exe)) {
    path = exe;
  }
  #endif
  return path;

}

static std::string filename_path(std::string fname) {
  #if defined(_WIN32)
  std::size_t fp = fname.find_last_of("\\/");
  #else
  std::size_t fp = fname.find_last_of("/");
  #endif
  if (fp == std::string::npos) return fname;
  return fname.substr(0, fp + 1);
}

static std::string string_replace_all(std::string str, std::string substr, std::string newstr) {
  std::size_t pos = 0;
  const std::size_t sublen = substr.length(), newlen = newstr.length();
  while ((pos = str.find(substr, pos)) != std::string::npos) {
    str.replace(pos, sublen, newstr);
    pos += newlen;
  }
  return str;
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
  #if defined(_WIN32)
  std::wstring dname = widen(filename_path(get_executable_path())); 
  SetCurrentDirectoryW(dname.c_str());
  SetEnvironmentVariableW(L"IMGUI_DIALOG_WIDTH", L"1080");
  SetEnvironmentVariableW(L"IMGUI_FONT_FILES", L"fonts/BBHHegarty-Regular.ttf");
  SetEnvironmentVariableW(L"IMGUI_FONT_SIZE", L"24");
  if (!get_executable_path().empty() && get_executable_path() != filename_path(get_executable_path()) + "filedialogs.exe") {
    STARTUPINFOW si; PROCESS_INFORMATION pi; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si); ZeroMemory(&pi, sizeof(pi)); 
    std::wstring program = widen(std::string("\"") + filename_path(get_executable_path()) + std::string("filedialogs\" --show-message \"") +
    string_replace_all(SYSINFO, "\"", "\\\"") + std::string("\""));
    if (CreateProcessW(nullptr, (wchar_t *)program.c_str(), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi)) {
      MSG msg; while (WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    }
  }
  #else
  setenv("IMGUI_DIALOG_WIDTH", "1080", 1);
  setenv("IMGUI_FONT_FILES", "fonts/BBHHegarty-Regular.ttf", 1);
  setenv("IMGUI_FONT_SIZE", "24", 1);
  chdir(filename_path(get_executable_path()).c_str());
  chmod((filename_path(get_executable_path()) + "filedialogs").c_str(), 755);
  if (system(nullptr) && !get_executable_path().empty() && get_executable_path() != filename_path(get_executable_path()) + "filedialogs") {
    system((std::string("\"") + filename_path(get_executable_path()) + std::string("filedialogs\" --show-message \"") +
    string_replace_all(SYSINFO, "\"", "\\\"") + std::string("\"")).c_str());
  }
  #endif
  stop_thread = true;
  th.join();
  stop_thread = false;
  #if !defined(_WIN32)
  unlink("/tmp/IMGUI_DIALOG_PIPE");
  #endif
  return 0;
}
