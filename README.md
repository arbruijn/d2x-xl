# D2X-XL-ar

[D2X-XL](https://www.descent2.de) with stability, compatibility and build fixes.

## Download latest Windows build

[d2x-xl-1.18.74-ar10.zip](https://github.com/arbruijn/d2x-xl/releases/download/v1.18.74-ar10/d2x-xl-1.18.74-ar10.zip)

## Changes

#### Stability

- Disable OpenMP, network and sound threads
- Fix various out of bounds access
- Fix various unintialized memory access
- Fix memory leak with FindFileFirst

#### Game play

- Restore bot / powerup collision if Collision model set to standard
- Restore GuideBot flare fire behaviour
- Fix level 12 boss not spewing on energy hits
- Fire primary in same frame
- GuideBot pathing fixes
- Various AI fixes
- No extra transparency if image quality is low

#### Setup

- Run without d2x-xl files
- Set default brightness to standard
- Use energy spark texture if effect bitmap missing
- Load vanilla/Rebirth D2 save games

#### Build/platform

- Fix cmake build
- Fix Windows build with cmake/autotools/gcc
- Fix build with newer gcc versions
- Use unpatched SDL on Windows
- Fix midi on Win64
- Fix Fx volume on Linux
- Fix warnings
- Disable structure packing except for network structs
- Print warnings/errors to console on Linux

## How to build

d2x-xl requires the following libraries: SDL 1.2, SDL-mixer 1.2, SDL-net 1.2, GLEW and libcurl.

### Install packages for Debian/Ubuntu:

`apt install build-essential cmake libsdl1.2-dev libsdl-mixer1.2-dev libsdl-net1.2-dev libglew-dev
libcurl3-dev libopenvr-dev`

### Install packages for MSYS2 (Windows):

`pacman -S
 mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-gcc
 mingw-w64-x86_64-SDL mingw-w64-x86_64-SDL_image
 mingw-w64-x86_64-SDL_mixer mingw-w64-x86_64-SDL_net
 mingw-w64-x86_64-glew mingw-w64-x86_64-openvr`

### Install packages for Visual Studio

`vcpkg install sdl1 sdl1-mixer sdl1-net glew curl openvr`

### Build

Configure build in subdirectory `build`:

`cmake -B build`

Compile:

`cmake --build build`

Run:

`build/d2x-xl -datadir /path/to/xl/or/d2/files`

If you specify a path to the D2 files (.hog, .pig, .ham, etc.) it will copy the files into a `data/` subdirectory and create a few more subdirectories.

To make all options work you need the d2x-xl data files:
[d2x-xl-data-1.18.64.7z](https://www.descent2.de/files/d2x-xl-data-1.18.64.7z)
