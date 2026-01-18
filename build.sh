#!/bin/sh
cd "${0%/*}";
rm -fr pci.ids pci.ids.hpp;
curl --output pci.ids "https://pci-ids.ucw.cz/pci.ids";
xxd -i pci.ids > pci.ids.hpp;
rm -fr libfiledialogs && git clone https://github.com/samuelvenable/SDL2-ImGui-FileDialogs libfiledialogs && chmod 755 libfiledialogs/build.sh && libfiledialogs/build.sh;
if [ $(uname) = "Darwin" ]; then
  cp -fr libfiledialogs/filedialogs/filedialogs filedialogs;
  clang++ sigu.cpp system.cpp shmem.mm -o sigu -std=c++17 -fPIC -framework Cocoa -framework CoreGraphics -framework Metal -framework Foundation -mmacos-version-min=10.13 -arch x86_64 -arch arm64;
elif [ $(uname) = "Linux" ]; then
  cp -fr libfiledialogs/filedialogs/filedialogs filedialogs;
  g++ sigu.cpp system.cpp -o sigu -std=c++17 -static-libgcc -static-libstdc++ `pkg-config --cflags --libs x11` -lGL -DCREATE_CONTEXT `pkg-config --cflags --libs sdl2 --static` -fPIC;
elif [ $(uname) = "FreeBSD" ]; then
  cp -fr libfiledialogs/filedialogs/filedialogs filedialogs;
  clang++ sigu.cpp system.cpp -o sigu -std=c++17 `pkg-config --cflags --libs x11` -lGL -DCREATE_CONTEXT `pkg-config --cflags --libs sdl2 --static` -lkvm -fPIC;
elif [ $(uname) = "DragonFly" ]; then
  cp -fr libfiledialogs/filedialogs/filedialogs filedialogs;
  g++ sigu.cpp system.cpp -o sigu -std=c++17 -static-libgcc -static-libstdc++ `pkg-config --cflags --libs x11` -lGL -DCREATE_CONTEXT `pkg-config --cflags --libs sdl2 --static` `pkg-config --cflags --libs hwloc --static` -lkvm -lpthread -fPIC;
elif [ $(uname) = "NetBSD" ]; then
  cp -fr libfiledialogs/filedialogs/filedialogs filedialogs;
  g++ sigu.cpp system.cpp -o sigu -std=c++17 -static-libgcc `pkg-config --cflags --libs x11` -I/usr/X11R7/include -Wl,-rpath,/usr/X11R7/lib -L/usr/X11R7/lib -lGL -DCREATE_CONTEXT `pkg-config --cflags --libs sdl2 --static` `pkg-config --cflags --libs hwloc --static` -fPIC;
elif [ $(uname) = "OpenBSD" ]; then
  cp -fr libfiledialogs/filedialogs/filedialogs filedialogs;
  clang++ sigu.cpp system.cpp -o sigu -std=c++17 `pkg-config --cflags --libs x11` -lkvm -lGL -DCREATE_CONTEXT `pkg-config --cflags --libs sdl2 --static` -fPIC;
elif [ $(uname) = "SunOS" ]; then
  cp -fr libfiledialogs/filedialogs/filedialogs filedialogs;
  export PKG_CONFIG_PATH=/usr/lib/64/pkgconfig && g++ sigu.cpp system.cpp -o sigu -std=c++17 -static-libgcc `pkg-config --cflags --libs x11` -lGL -DCREATE_CONTEXT `pkg-config --cflags --libs sdl2 --static` `pkg-config --cflags --libs hwloc --static` -fPIC;
else
  cp -fr libfiledialogs/filedialogs/filedialogs.exe filedialogs.exe;
  g++ sigu.cpp system.cpp -o sigu.exe -std=c++17 -static-libgcc -static-libstdc++ -lws2_32 -ldxgi -lpsapi -static -fPIC;
fi
