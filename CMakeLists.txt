cmake_minimum_required(VERSION 2.6)
project(D2X-XL)
include(CheckIncludeFiles)
set(OpenGL_GL_PREFERENCE LEGACY)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
add_compile_options(-Wno-parentheses-equality)
endif()

#checks
#need to do a proper check for this
if(NOT MSVC)
	add_definitions(-DHAVE_STRUCT_TIMESPEC)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
check_include_files(netipx/ipx.h HAVE_NETIPX_IPX_H)
if (HAVE_NETIPX_IPX_H)
add_definitions(-DHAVE_NETIPX_IPX_H)
add_definitions(-DNATIVE_IPX)
endif()
add_definitions(-DKALINIX)
set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} GLEW)
else()
set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} wsock32 ws2_32 winmm urlmon)
endif()

find_package(GLEW REQUIRED)
set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} GLEW::GLEW)

if (${CMAKE_BUILD_TYPE} MATCHES "Release")
	add_definitions(-DRELEASE)
elseif (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	#add_definitions(-DDBG)
endif()

if(NOT MSVC)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-strict-aliasing -Wno-format-security")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing -Wno-format-security")
endif()

if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
if (DBGSYM)
message(STATUS "DBGSYM IS ON")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else()
message(STATUS "DBGSYM IS OFF")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g1")
endif()
else()
message(STATUS "type Debug ${CMAKE_BUILD_TYPE}")
endif()

#finds
if(USE_OPENMP)
find_package(OpenMP)
if(OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif(OPENMP_FOUND)
endif()

find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${OPENGL_LIBRARY})
endif(OPENGL_FOUND)

set(GLEW_USE_STATIC_LIBS) 
find_package(GLEW REQUIRED)
if(GLEW_FOUND)
	include_directories(${GLEW_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${GLEW_LIBRARY})
endif(GLEW_FOUND)

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
	include_directories(${ZLIB_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${ZLIB_LIBRARY})
endif(ZLIB_FOUND)

find_package(CURL REQUIRED)
if(CURL_FOUND)
	include_directories(${CURL_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${CURL_LIBRARIES})
endif(CURL_FOUND)

find_package(SDL REQUIRED)
if(SDL_FOUND)
	add_definitions(-DSDL_INPUT)
	include_directories(${SDL_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${SDL_LIBRARY})
endif(SDL_FOUND)

if(MSVC)
	add_subdirectory(sdlimage)
	add_definitions(-DUSE_SDL_IMAGE)
	include_directories(sdlimage)
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} SDL_image)
else()
find_package(SDL_image REQUIRED)
if(SDL_IMAGE_FOUND)
	add_definitions(-DUSE_SDL_IMAGE)
	include_directories(${SDL_IMAGE_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${SDL_IMAGE_LIBRARY})
elseif(SDLIMAGE_FOUND)
	add_definitions(-DUSE_SDL_IMAGE)
	include_directories(${SDLIMAGE_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${SDLIMAGE_LIBRARY})
endif()
endif()

find_package(SDL_mixer REQUIRED)
if(SDL_MIXER_FOUND)
	add_definitions(-DUSE_SDL_MIXER)
	include_directories(${SDL_MIXER_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${SDL_MIXER_LIBRARY})
elseif(SDLMIXER_FOUND)
	add_definitions(-DUSE_SDL_MIXER)
	include_directories(${SDLMIXER_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${SDLMIXER_LIBRARY})
endif()

find_package(SDL_net REQUIRED)
if(SDL_NET_FOUND)
	add_definitions(-DUSE_SDL_NET)
	include_directories(${SDL_NET_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${SDL_NET_LIBRARY})
elseif(SDLNET_FOUND)
	add_definitions(-DUSE_SDL_NET)
	include_directories(${SDLNET_INCLUDE_DIR})
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${SDLNET_LIBRARY})
endif()

#local includes
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/input/include)
include_directories(${PROJECT_SOURCE_DIR}/effects)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	include_directories(${PROJECT_SOURCE_DIR}/network/linux/include)
else()
	include_directories(${PROJECT_SOURCE_DIR}/network/win32/include)
endif()

#sources
set(2d_sources 2d/bitblt.cpp 2d/bitmap.cpp 2d/canvas.cpp 2d/circle.cpp 2d/font.cpp 2d/ibitblt.cpp 2d/palette.cpp 2d/pcx.cpp 2d/pixel.cpp 2d/rle.cpp 2d/scalec.cpp 2d/string.cpp 2d/tmerge.cpp)
set(3d_sources 3d/ase2model.cpp 3d/buildmodel.cpp 3d/draw.cpp 3d/fastmodels.cpp 3d/globvars.cpp 3d/hitbox.cpp 3d/interp.cpp 3d/matrix.cpp 3d/oof2model.cpp 3d/pof2model.cpp 3d/points.cpp 3d/rod.cpp 3d/setup.cpp 3d/shadows.cpp 3d/transformation.cpp)
set(ai_sources ai/ai.cpp ai/aianimate.cpp ai/aiboss.cpp ai/aifire.cpp ai/aiframe.cpp ai/aiinit.cpp ai/ailib.cpp ai/aimove.cpp ai/aipath.cpp ai/aisave.cpp ai/aisnipe.cpp ai/escort.cpp ai/thief.cpp ai/d1_ai.cpp)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(audio_sources audio/linux/hmiplay.cpp audio/linux/hmistub.cpp audio/linux/hmpfile.cpp audio/win32/midi.cpp audio/audio.cpp audio/objectaudio.cpp audio/rbaudio.cpp audio/songs.cpp audio/soundthreads.cpp)
else()
set(audio_sources audio/win32/hmpfile.cpp audio/win32/midi.cpp audio/audio.cpp audio/objectaudio.cpp audio/rbaudio.cpp audio/songs.cpp audio/soundthreads.cpp)
endif()
set(cockpit_sources cockpit/cockpit.cpp cockpit/cockpitdata.cpp cockpit/crosshairs.cpp cockpit/genericcockpit.cpp cockpit/genericdrawers.cpp cockpit/hud.cpp cockpit/hudicons.cpp cockpit/hudmsgs.cpp cockpit/statusbar.cpp cockpit/targetindicators.cpp cockpit/utilities.cpp)
set(console_sources console/console.cpp console/cvar.cpp)
set(effects_sources effects/fireball.cpp effects/glare.cpp effects/lightning.cpp effects/objsmoke.cpp effects/shrapnel.cpp effects/sparkeffect.cpp effects/particle_shader.cpp effects/particle_buffer.cpp effects/particles.cpp effects/particle_emitter.cpp effects/particle_system.cpp effects/particle_manager.cpp effects/particle_images.cpp effects/waypoint.cpp effects/postprocessing.cpp effects/lightning_node.cpp effects/lightning_emitter.cpp effects/lightning_manager.cpp effects/omega_lightning.cpp effects/glow.cpp)
set(gameio_sources gameio/addon_bitmaps.cpp gameio/createmesh.cpp gameio/facegrid.cpp gameio/loadgeometry.cpp gameio/loadobjects.cpp gameio/hoard.cpp gameio/iff.cpp gameio/loadgame.cpp gameio/loadgamedata.cpp gameio/loadmodeldata.cpp gameio/loadsounds.cpp gameio/loadtextures.cpp gameio/mission.cpp gameio/paging.cpp gameio/piggy.cpp gameio/playerprofile.cpp gameio/savegame.cpp gameio/tga.cpp gameio/IpToCountry.cpp)
set(gamemodes_sources gamemodes/entropy.cpp gamemodes/monsterball.cpp)
set(input_sources input/event.cpp input/input.cpp input/joy.cpp input/key.cpp input/mouse.cpp input/systemkeys.cpp)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(io_sources io/linux/findfile.cpp io/args.cpp io/cfile.cpp io/hogfile.cpp io/rwops.cpp)
else()
set(io_sources io/win32/findfile.cpp io/args.cpp io/cfile.cpp io/hogfile.cpp io/rwops.cpp)
endif()
set(libmve_sources libmve/decoder16.cpp libmve/decoder8.cpp libmve/mve_audio.cpp libmve/mvelib.cpp libmve/mveplay.cpp)
set(lighting_sources lighting/dynlight.cpp lighting/grayscaleshaders.cpp lighting/headlight.cpp lighting/lightcluster.cpp lighting/light.cpp lighting/lightmap.cpp lighting/lightmapshaders.cpp lighting/lightprecalc.cpp lighting/lightrender.cpp lighting/lightshaders.cpp)

set(main_sources main/briefings.cpp main/cheats.cpp main/cmd.cpp
 main/config.cpp main/credits.cpp main/crypt.cpp main/dialheap.cpp
 main/dropobject.cpp main/effects.cpp main/endlevel.cpp main/findpath.cpp
 main/findsegment.cpp main/flightpath.cpp main/meshedge.cpp
 main/producers.cpp main/game.cpp main/gameargs.cpp main/gamecntl.cpp
 main/gamedata.cpp main/gamefolders.cpp main/gameoptions.cpp
 main/gametime.cpp main/gamestates.cpp main/highscores.cpp main/descent.cpp
 main/kconfig.cpp main/mglobal.cpp main/movie.cpp main/newdemo.cpp
 main/scores.cpp main/segmath.cpp main/segment.cpp main/setup.cpp
 main/side.cpp main/skybox.cpp main/slew.cpp main/trigger.cpp
 main/terrain.cpp main/texmerge.cpp main/text.cpp main/textdata.cpp
 main/trackobject.cpp main/vclip.cpp main/update.cpp main/wall.cpp)

set(maths_sources maths/fixc.cpp maths/PerlinNoise.cpp maths/rand.cpp maths/SimplexNoise.cpp maths/tables.cpp maths/vecmat.cpp)
set(menus_sources menus/cockpitmenu.cpp menus/configmenu.cpp menus/detailsmenu.cpp menus/effectsmenu.cpp menus/entropymenu.cpp menus/fileselector.cpp menus/gameplaymenu.cpp menus/inputdevicemenu.cpp menus/listbox.cpp menus/mainmenu.cpp menus/menubackground.cpp menus/menu.cpp menus/menuitem.cpp menus/messagebox.cpp menus/miscmenu.cpp menus/monsterballmenu.cpp menus/msgbox.cpp menus/netgamebrowser.cpp menus/netgamehelp.cpp menus/netgameinfo.cpp menus/netmenu.cpp menus/netplayerbrowser.cpp menus/newgamemenu.cpp menus/physicsmenu.cpp menus/rendermenu.cpp menus/screenresmenu.cpp menus/smokemenu.cpp menus/soundmenu.cpp)
set(misc_sources misc/error.cpp misc/hash.cpp misc/strio.cpp misc/strutil.cpp)
set(model_sources models/aseread.cpp models/hiresmodels.cpp models/oofmath.cpp models/oofread.cpp models/pof.cpp models/polymodel.cpp)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(network_sources network/linux/ipx_kali.cpp network/linux/ukali.cpp network/linux/ipx_mcast4.cpp network/linux/linuxnet.cpp network/linux/ipx_bsd.cpp network/autodl.cpp network/banlist.cpp network/multi.cpp network/multibot.cpp network/multimsg.cpp network/netmisc.cpp network/network.cpp network/network_handshake.cpp network/network_init.cpp network/network_join.cpp network/network_lib.cpp network/network_phandler.cpp network/network_process.cpp network/network_read.cpp network/network_send.cpp network/network_sync.cpp network/network_thread.cpp network/tracker.cpp network/udp_interface.cpp)
else()
set(network_sources network/win32/ipx_mcast4.cpp network/win32/ipx_win.cpp network/win32/winnet.cpp network/autodl.cpp network/banlist.cpp network/multi.cpp network/multibot.cpp network/multimsg.cpp network/netmisc.cpp network/network.cpp network/network_handshake.cpp network/network_init.cpp network/network_join.cpp network/network_lib.cpp network/network_phandler.cpp network/network_process.cpp network/network_read.cpp network/network_send.cpp network/network_sync.cpp network/network_thread.cpp network/tracker.cpp network/udp_interface.cpp)
endif()
set(objects_sources objects/boss.cpp objects/createobject.cpp objects/criticalhit.cpp objects/effectobject.cpp objects/hostage.cpp objects/marker.cpp objects/object.cpp objects/objectio.cpp objects/object_physics.cpp objects/player.cpp objects/playerdeath.cpp objects/powerup.cpp objects/reactor.cpp objects/robot.cpp objects/updateobject.cpp objects/objbufio.cpp)

set(ogl_sources ogl/clientstates.cpp ogl/drawbuffer.cpp ogl/fbuffer.cpp
 ogl/flushbuffers.cpp ogl/gr.cpp ogl/ogl_bitmap.cpp ogl/ogl_cartoonize.cpp
 ogl/ogl_color.cpp ogl/ogl_defs.cpp ogl/ogl_draw.cpp ogl/ogl_fastrender.cpp
 ogl/ogl_hudstuff.cpp ogl/ogl_lib.cpp ogl/oglmatrix.cpp ogl/ogl_palette.cpp
 ogl/ogl_render.cpp ogl/ogl_shader.cpp ogl/ogl_texcache.cpp
 ogl/ogl_texture.cpp ogl/pbuffer.cpp ogl/rendertextures.cpp
 ogl/screenshot.cpp ogl/sdlgl.cpp ogl/shadowmap.cpp ogl/stereo.cpp)

set(physics_sources physics/collide.cpp physics/hitbox_collision.cpp physics/physics.cpp physics/slowmotion.cpp physics/sphere_collision.cpp physics/visibility.cpp)
set(render_sources render/automap.cpp render/cameras.cpp render/fastrender.cpp render/gpgpu_lighting.cpp render/lighttrails.cpp render/morph.cpp render/objeffects.cpp render/objrender.cpp render/oofrender.cpp render/polymodelrender.cpp render/radar.cpp render/renderframe.cpp render/renderlib.cpp render/render_lighting.cpp render/rendermine.cpp render/rendersetup.cpp render/rendershadows.cpp render/rendershields.cpp render/renderthreads.cpp render/sphere.cpp render/thrusterflames.cpp render/transprender.cpp render/visculling.cpp)
set(weapon_sources weapons/controlweapon.cpp weapons/createweapon.cpp weapons/dropweapon.cpp weapons/fireweapon.cpp weapons/fusion.cpp weapons/omega.cpp weapons/pickupweapon.cpp weapons/seismic.cpp weapons/weaponhandlers.cpp weapons/weaponorder.cpp)
set(d2x-xl_sources ${2d_sources} ${3d_sources} ${ai_sources} ${audio_sources} ${cockpit_sources} ${console_sources} ${effects_sources} ${gameio_sources} ${gamemodes_sources} ${input_sources} ${io_sources} ${libmve_sources} ${lighting_sources} ${main_sources} ${maths_sources} ${menus_sources} ${misc_sources} ${model_sources} ${network_sources} ${objects_sources} ${ogl_sources} ${physics_sources} ${render_sources} ${weapon_sources})
add_executable(d2x-xl ${d2x-xl_sources})
target_link_libraries(d2x-xl ${EXTRA_LIBRARIES})
