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

static std::string ppidenv;
static std::string pipeName;

void string_send() {
  #if defined(_WIN32)
  ppidenv = std::to_string(GetCurrentProcessId());
  SetEnvironmentVariableA("IMGUI_DIALOG_PPID", ppidenv.c_str());
  pipeName = std::string("\\\\.\\pipe\\IMGUI_DIALOG_PIPE_") + ppidenv;
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
  ppidenv = std::to_string(getpid());
  setenv("IMGUI_DIALOG_PPID", ppidenv.c_str(), 1);
  pipeName = std::string("/tmp/IMGUI_DIALOG_PIPE_") + ppidenv;
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

#if defined(_WIN32)
static std::wstring widen(std::string str) {
  if (str.empty()) return L"";
  size_t wchar_count = str.size() + 1;
  std::vector<wchar_t> buf(wchar_count);
  wchar_count = (size_t)MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buf.data(), (int)wchar_count);
  if (!wchar_count) return L"";
  return std::wstring { buf.data(), wchar_count };
}

static std::string narrow(std::wstring wstr) {
  if (wstr.empty()) return "";
  int nbytes = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), nullptr, 0, nullptr, nullptr);
  if (!nbytes) return "";
  std::vector<char> buf((size_t)nbytes);
  nbytes = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), buf.data(), nbytes, nullptr, nullptr);
  if (!nbytes) return "";
  return std::string { buf.data(), (size_t)nbytes };
}

static wchar_t *_wrealpath(const wchar_t *path, wchar_t *resolved_path) {
  std::wstring result;
  wchar_t buf[MAX_PATH];
  wchar_t *ptr = (((wchar_t *)resolved_path) ? ((wchar_t *)resolved_path) : ((wchar_t *)buf));
  HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (hFile != INVALID_HANDLE_VALUE) {
    DWORD len = GetFinalPathNameByHandleW(hFile, ptr, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (len && len <= MAX_PATH - 1) {
      result = ptr;
      if (!result.substr(0, 8).compare(L"\\\\?\\UNC\\")) {
        result = L"\\" + result.substr(7);
      } else if (!result.substr(0, 4).compare(L"\\\\?\\")) {
        result = result.substr(4);
      }
    }
    CloseHandle(hFile);
  }
  if (!result.empty()) {
    if (!resolved_path) {
      return _wcsdup(result.c_str());
    } else {
      wcsncpy_s(ptr, MAX_PATH, result.c_str(), _TRUNCATE);
      return (wchar_t *)ptr;
    }
  }
  return nullptr;
}
#endif

static std::string get_executable_path() {
  std::string path;
  #if (defined(_WIN32) || defined(_WIN64))
  wchar_t buffer[MAX_PATH];
  if (GetModuleFileNameW(nullptr, buffer, sizeof(buffer))) {
    wchar_t exe[MAX_PATH];
    if (_wrealpath(buffer, exe)) {
      path = narrow(exe);
    }
  }
  #elif (defined(__APPLE__) && defined(__MACH__))
  char buffer[PATH_MAX];
  uint32_t size = sizeof(buffer);
  if (!_NSGetExecutablePath(buffer, &size)) {
    char exe[PATH_MAX];
    if (realpath(buffer, exe)) {
      path = exe;
    }
  }
  #elif (defined(__linux__) || defined(__ANDROID__))
  char exe[PATH_MAX];
  if (realpath("/proc/self/exe", exe)) {
    path = exe;
  }
  #elif ((defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) || defined(__DragonFly__))
  int mib[4]; 
  size_t len = 0;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = -1;
  if (!sysctl(mib, 4, nullptr, &len, nullptr, 0)) {
    std::string strbuff;
    strbuff.resize(len, '\0');
    char *buffer = strbuff.data();
    if (!sysctl(mib, 4, buffer, &len, nullptr, 0)) {
      char exe[PATH_MAX];
      if (realpath(buffer, exe)) {
        path = exe;
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
  if (!sysctl(mib, 4, nullptr, &len, nullptr, 0)) {
    std::string strbuff;
    strbuff.resize(len, '\0');
    char *buffer = strbuff.data();
    if (!sysctl(mib, 4, buffer, &len, nullptr, 0)) {
      char exe[PATH_MAX];
      if (realpath(buffer, exe)) {
        path = exe;
      }
    }
  }
  #elif defined(__OpenBSD__)
  auto cpp_getexe = [](std::string exe) {
    int cntp = 0;
    std::string res;
    kvm_t *kd = nullptr;
    kinfo_file *kif = nullptr;
    bool error1 = false, error2 = false;
    kd = kvm_openfiles(nullptr, nullptr, nullptr, KVM_NO_FILES, nullptr);
    if (kd) {
      if ((kif = kvm_getfiles(kd, KERN_FILE_BYPID, getpid(), sizeof(struct kinfo_file), &cntp))) {
        for (int i = 0; i < cntp && kif[i].fd_fd < 0; i++) {
          if (kif[i].fd_fd == KERN_FILE_TEXT) {
            fallback:
            struct stat st;
            char buffer[PATH_MAX];
            if (!stat(exe.c_str(), &st) && (st.st_mode & S_IXUSR) &&
              S_ISREG(st.st_mode) && realpath(exe.c_str(), buffer) &&
              st.st_dev == (dev_t)kif[i].va_fsid && st.st_ino == (ino_t)kif[i].va_fileid) {
              res = buffer;
            }
            if (res.empty() && !error1) {
              error1 = true;
              size_t last_slash_pos = exe.find_last_of("/");
              if (last_slash_pos != std::string::npos) {
                exe = exe.substr(0, last_slash_pos + 1) + kif[i].p_comm;
                goto fallback;
              }
            }
            if (res.empty() && !error2) {
              error2 = true;
              size_t last_slash_pos = exe.find_last_of("/");
              if (last_slash_pos != std::string::npos) {
                const char *progname = getprogname();
                if (progname) {
                  exe = exe.substr(0, last_slash_pos + 1) + progname;
                  goto fallback;
                }
              }
            }
            break;
          }
        }
      }
      kvm_close(kd);
    }
    return res;
  };
  auto cpp_getenv = [](std::string name) {
    const char *cvalue = getenv(name.c_str());
    std::string value = cvalue ? cvalue : "";
    return value;
  };
  int cntp = 0;
  std::string buffer;
  kvm_t *kd = nullptr;
  kinfo_proc *proc_info = nullptr;
  bool error = false, retried = false, leading_dash_removed = false;
  kd = kvm_openfiles(nullptr, nullptr, nullptr, KVM_NO_FILES, nullptr);
  if (kd) {
    if ((proc_info = kvm_getprocs(kd, KERN_PROC_PID, getpid(), sizeof(struct kinfo_proc), &cntp))) {
      char **cmd = kvm_getargv(kd, proc_info, 0);
      if (cmd && cmd[0]) {
        buffer = cmd[0];
      }
    }
    kvm_close(kd);
  }
  std::string argv0;
  bool argv0_does_not_exist = false;
  size_t slash_pos = std::string::npos;
  size_t colon_pos = std::string::npos;
  if (buffer.empty()) {
    argv0_does_not_exist = true;
    goto path_lookup;
  } else {
    fallback:
    slash_pos = buffer.find('/');
    colon_pos = buffer.find(':');
    if (slash_pos == 0) {
      argv0 = buffer;
      path = cpp_getexe(argv0);
    } else if (slash_pos == std::string::npos || (colon_pos != std::string::npos && colon_pos > 0 && slash_pos > colon_pos)) {
      path_lookup:
      retry_without_leading_dash:
      std::string penv = cpp_getenv("PATH");
      if (!penv.empty()) {
        retry:
        std::string tmp;
        std::stringstream sstr(penv);
        while (std::getline(sstr, tmp, ':')) {
          argv0 = tmp + "/" + buffer;
          path = cpp_getexe(argv0);
          if (!path.empty()) break;
          if (!argv0_does_not_exist && colon_pos != std::string::npos && colon_pos > 0 && slash_pos > colon_pos) {
            argv0 = tmp + "/" + buffer.substr(0, colon_pos);
            path = cpp_getexe(argv0);
            if (!path.empty()) break;
          }
        }
      }
      if (path.empty() && !retried) {
        retried = true;
        penv = "/usr/bin:/bin:/usr/sbin:/sbin:/usr/X11R6/bin:/usr/local/bin:/usr/local/sbin";
        std::string home = cpp_getenv("HOME");
        if (!home.empty()) {
          penv = home + "/bin:" + penv;
        }
        goto retry;
      }
      if (path.empty() && !argv0_does_not_exist && !leading_dash_removed && slash_pos == std::string::npos && buffer.length() > 1 && buffer[0] == '-') {
        buffer = buffer.substr(1);
        retried = false;
        leading_dash_removed = true;
        goto retry_without_leading_dash;
      }
    }
    if (path.empty() && (argv0_does_not_exist || (slash_pos != std::string::npos && slash_pos > 0))) {
      std::string pwd = cpp_getenv("PWD");
      if (!pwd.empty()) {
        argv0 = pwd + "/" + buffer;
        path = cpp_getexe(argv0);
      }
      if (path.empty()) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, PATH_MAX)) {
          argv0 = std::string(cwd) + "/" + buffer;
          path = cpp_getexe(argv0);
        }
      }
    }
    if (path.empty() && !error) {
      error = true;
      buffer.clear();
      std::string underscore = cpp_getenv("_");
      if (!underscore.empty()) {
        buffer = underscore;
        leading_dash_removed = false;
        retried = false;
        goto fallback;
      }
    }
  }
  if (path.empty() && !argv0_does_not_exist) {
    argv0_does_not_exist = true;
    retried = false;
    buffer.clear();
    goto path_lookup;
  }
  #elif (defined(__sun) && defined(__SVR4))
  const char *execname = getexecname();
  if (execname) {
    char exe[PATH_MAX];
    if (realpath(execname, exe)) {
      path = exe;
    }
  }
  if (path.empty()) {
    char exe[PATH_MAX];
    if (realpath("/proc/self/path/a.out", exe)) {
      path = exe;
    }
  }
  #endif
  return path;
}

static std::string filename_path(std::string fname) {
  #if defined(_WIN32)
  size_t fp = fname.find_last_of("\\/");
  #else
  size_t fp = fname.find_last_of("/");
  #endif
  if (fp == std::string::npos) return fname;
  return fname.substr(0, fp + 1);
}

static std::string string_replace_all(std::string str, std::string substr, std::string newstr) {
  size_t pos = 0;
  const size_t sublen = substr.length(), newlen = newstr.length();
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

bool env_var_exists(const char *name) {
  #if defined(_WIN32)
  DWORD result = GetEnvironmentVariableA(name, nullptr, 0);
  if (result == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
    return false;
  }
  return true;
  #else
  const char *result = getenv(name);
  if (!result) {
    return false;
  }
  return true;
  #endif
}

int main() {
  std::thread th(thloop);
  #if defined(_WIN32)
  std::wstring dname = widen(filename_path(get_executable_path())); 
  SetCurrentDirectoryW(dname.c_str());
  if (!env_var_exists("IMGUI_DIALOG_CAPTION")) SetEnvironmentVariableW(L"IMGUI_DIALOG_CAPTION", L"About this Computer");
  if (!env_var_exists("IMGUI_DIALOG_PARENT")) SetEnvironmentVariableW(L"IMGUI_DIALOG_PARENT", L"");
  if (!env_var_exists("IMGUI_DIALOG_WIDTH")) SetEnvironmentVariableW(L"IMGUI_DIALOG_WIDTH", L"800");
  if (!env_var_exists("IMGUI_FONT_FILES")) SetEnvironmentVariableW(L"IMGUI_FONT_FILES", L"fonts\\Roboto\\Roboto-Medium.ttf");
  if (!env_var_exists("IMGUI_FONT_SIZE")) SetEnvironmentVariableW(L"IMGUI_FONT_SIZE", L"20");
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
  if (!env_var_exists("IMGUI_DIALOG_CAPTION")) setenv("IMGUI_DIALOG_CAPTION", "About this Computer", 1);
  if (!env_var_exists("IMGUI_DIALOG_PARENT")) setenv("IMGUI_DIALOG_PARENT", "", 1);
  if (!env_var_exists("IMGUI_DIALOG_WIDTH")) setenv("IMGUI_DIALOG_WIDTH", "800", 1);
  if (!env_var_exists("IMGUI_FONT_FILES")) setenv("IMGUI_FONT_FILES", "fonts/Roboto/Roboto-Medium.ttf", 1);
  if (!env_var_exists("IMGUI_FONT_SIZE")) setenv("IMGUI_FONT_SIZE", "20", 1);
  chdir(filename_path(get_executable_path()).c_str());
  chmod((filename_path(get_executable_path()) + "filedialogs").c_str(), S_IRWXU);
  if (system(nullptr) && !get_executable_path().empty() && get_executable_path() != filename_path(get_executable_path()) + "filedialogs") {
    system((std::string("\"") + filename_path(get_executable_path()) + std::string("filedialogs\" --show-message \"") +
    string_replace_all(SYSINFO, "\"", "\\\"") + std::string("\"")).c_str());
  }
  #endif
  stop_thread = true;
  th.join();
  stop_thread = false;
  #if !defined(_WIN32)
  unlink(pipeName.c_str());
  #endif
  return 0;
}
