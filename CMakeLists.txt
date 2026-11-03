cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(ImFileDialogExample)
if(UNIX AND NOT APPLE)
	set(SDL_X11_XSCRNSAVER OFF)
	set(SDL_X11_XINPUT OFF)
	set(SDL_X11_XFIXES OFF)
	set(SDL_X11_XTEST OFF)
endif()

# remove below line to use OS-default icon theme
add_compile_definitions(IFD_USE_BASIC_ICONS)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_compile_definitions(IFD_USE_OPENGL)
else()
	if(WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIFD_USE_OPENGL -g -ggdb -static")
	elseif(APPLE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIFD_USE_OPENGL -g -ggdb -ObjC++")
		if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
		endif()
	else()
		if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
			elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
			elseif(CMAKE_SYSTEM_NAME STREQUAL "DragonFly")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc")
			elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc")
			elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc")
			endif()
		endif()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIFD_USE_OPENGL -g -ggdb")
	endif()
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# cmake toolchain
if(CMAKE_TOOLCHAIN_FILE)
	include("${CMAKE_TOOLCHAIN_FILE}")
endif(CMAKE_TOOLCHAIN_FILE)

# pkg
if(UNIX)
	find_package(PkgConfig REQUIRED)
endif()

# opengl
set(OpenGL_GL_PREFERENCE GLVND CACHE STRING "Linux only: if GLVND, use the vendor-neutral GL libraries (default). If LEGACY, use the legacy ones (might be necessary to have working optirun/primusrun)")
set_property(CACHE OpenGL_GL_PREFERENCE PROPERTY STRINGS GLVND LEGACY)
find_package(OpenGL REQUIRED)

include("${CMAKE_CURRENT_SOURCE_DIR}/cl-bootstrap.cmake")
cl_repo()

# openbsd is hostile toward static linking
if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
	set(CATALOG_REQ_TYPE PREFER_SHARED)
else()
	set(CATALOG_REQ_TYPE PREFER_STATIC)
endif()

# source code
set(SOURCES
	example.cpp
	ImFileDialog.cpp
	libs/imgui/imgui.cpp
	libs/imgui/imgui_widgets.cpp
	libs/imgui/imgui_draw.cpp
	libs/imgui/imgui_tables.cpp
	libs/imgui/backends/imgui_impl_opengl3.cpp
	libs/imgui/backends/imgui_impl_sdl2.cpp
	libs/apifilesystem/filesystem.cpp	
)

# create executable
add_executable(ImFileDialogExample ${SOURCES})

cl_add_dep(ImFileDialogExample SDL2)
cl_add_dep(ImFileDialogExample GLEW)

# properties
set_target_properties(ImFileDialogExample PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# include directories
target_include_directories(ImFileDialogExample PRIVATE ${GLEW_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})
target_include_directories(ImFileDialogExample PRIVATE libs libs/imgui)

# link libraries
if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
	target_include_directories(ImFileDialogExample PRIVATE ${LIBUSB_LIBRARIES})
endif()
target_link_libraries(ImFileDialogExample PRIVATE ${OPENGL_LIBRARIES})

if(WIN32)
	# link specific win32 libraries
	target_link_libraries(ImFileDialogExample PRIVATE shell32)
elseif(UNIX AND NOT APPLE)
	# link non-apple unix libraries
	add_subdirectory(libs/lunasvg)
	include_directories(libs/lunasvg/include)
	pkg_search_module(PKG_GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
	pkg_search_module(PKG_GIO2 REQUIRED IMPORTED_TARGET gio-2.0)
	pkg_search_module(PKG_GLIB2 REQUIRED IMPORTED_TARGET glib-2.0)
	if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
		target_link_libraries(ImFileDialogExample PRIVATE lunasvg PkgConfig::PKG_GTK3 PkgConfig::PKG_GIO2 PkgConfig::PKG_GLIB2 ${CMAKE_DL_LIBS} kvm pthread)
	else()
		target_link_libraries(ImFileDialogExample PRIVATE lunasvg PkgConfig::PKG_GTK3 PkgConfig::PKG_GIO2 PkgConfig::PKG_GLIB2 ${CMAKE_DL_LIBS} pthread)
	endif()
elseif(APPLE)
	target_link_libraries(ImFileDialogExample PRIVATE "-framework CoreFoundation -framework AppKit" ${CMAKE_DL_LIBS})
endif()
