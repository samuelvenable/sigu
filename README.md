# System Information Graphical Utility (SIGU)

Welcome to SIGU, the System Information Graphical Utility! It is similar to [neofetch](https://github.com/dylanaraps/neofetch) or [fastfetch](https://github.com/fastfetch-cli/fastfetch), but designed for graphical environments. This small open source and permissively-licensed tool allows you to graphically display your OS Device Name, OS Product Name, OS Kernel Name, OS Kernel Release, OS Architecture, CPU Processor, CPU Vendor, CPU Core Count, CPU Processor Count, Random-Access Memory Total, Random-Access Memory Used, Random-Access Memory Free, Swap Memory Total, Swap Memory Used, Swap Memory Free, GPU Manufacturer, GPU Renderer, and GPU Memory. The dialog box window used to display this information can be highly customized in its appearance and behavior with the help of various environment variables. The GUI used is powered by [SDL3](https://github.com/libsdl-org/SDL) and [Dear ImGui](https://github.com/ocornut/imgui).

# Platforms

Supports Windows, macOS, GNU/Linux, FreeBSD, DragonFly BSD, NetBSD, OpenBSD, Solaris, and illumos.

**Build Dependencies:**
- Windows: {{visual studio, msvc} or {msys2, mingw, g++, ninja}}, sdl3, cmake
- macOS: {xcode command line tools, clang++, make}, sdl3, pkg-config, cmake
- GNU/Linux: g++, make, opengl, sdl3, x11, pkg-config, cmake
- FreeBSD: clang++, make, opengl, sdl3, x11, pkg-config, cmake
- DragonFly BSD: g++, make, hwloc, opengl, sdl3, x11, pkg-config, cmake
- NetBSD: g++, make, hwloc, opengl, sdl3, x11, pkg-config, cmake
- OpenBSD: clang++, make, opengl, sdl3, x11, pkg-config, cmake
- Solaris: g++, make, hwloc, opengl, sdl3, x11, pkg-config, cmake
- illumos: g++, make, hwloc, opengl, sdl3, x11, pkg-config, cmake
