# Microsoft Developer Studio Project File - Name="d2x" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=d2x - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "d2x.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "d2x.mak" CFG="d2x - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "d2x - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "d2x - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "d2x - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gr /Zp1 /MD /W3 /GX /O2 /Ob2 /I "..\..\include" /I "..\..\main" /I "..\..\input\include" /I "..\..\network\win32\include" /I "..\..\..\audio\include\win32" /I "..\..\audio\include\win32" /I "..\3d" /I "..\..\3d" /I "\projects\libraries\sdl-1.2.13\include" /I "\projects\libraries\sdl_mixer-1.2.8" /I "\projects\libraries\SDL_image-1.2.7" /D "NDEBUG" /D "RELEASE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NO_ASM" /D "NMONO" /D "PIGGY_USE_PAGING" /D "NEWDEMO" /D "SDL_INPUT" /D "SDL_GL_VIDEO" /D "NETWORK" /D "NATIVE_IPX" /D "OGL" /D "OGL_ZBUF" /D "FAST_EVENTPOLL" /D WIN_VER=0x0500 /D "CONSOLE" /Fr /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib opengl32.lib glu32.lib winmm.lib /nologo /subsystem:windows /pdb:none /machine:I386 /out:"\programs\d2\bin\win16\d2x-xl-win16.exe" /libpath:"C:\Programme\Microsoft Platform SDK for Windows XP SP2\Lib" /libpath:"\projects\SDL-1.2.11\VisualC\SDLmain\Release" /libpath:"\projects\SDL-1.2.13\VisualC\SDL\win16\Release" /libpath:"\projects\SDL_mixer-1.2.8\VisualC\win16\Release"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "d2x - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /Zp1 /MDd /W3 /GX /ZI /Od /I "..\..\include" /I "..\..\main" /I "..\..\input\include" /I "..\..\network\win32\include" /I "..\..\audio\include\win32" /I "..\3d" /I "..\..\3d" /I "\projects\sdl-1.2.13\include" /I "\projects\sdl_mixer-1.2.8" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NO_ASM" /D "NMONO" /D "PIGGY_USE_PAGING" /D "NEWDEMO" /D "SDL_INPUT" /D "SDL_GL_VIDEO" /D "CONSOLE" /D "NETWORK" /D "NATIVE_IPX" /D "OGL" /D "OGL_ZBUF" /D "FAST_EVENTPOLL" /D WIN_VER=0x0500 /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib opengl32.lib glu32.lib winmm.lib sdl-win16.lib sdl_mixer-win16.lib /nologo /subsystem:windows /debug /machine:I386 /out:"d:\programme\d2\d2x-win16-dbg.exe" /libpath:"C:\Programme\Microsoft Platform SDK for Windows XP SP2\Lib" /libpath:"\projects\SDL-1.2.13\VisualC\SDL\win16\Debug" /libpath:"D:\projects\SDL_mixer-1.2.8\VisualC\win16\Debug" /libpath:"\projects\SDL-1.2.13\VisualC\SDLmain\win16\Debug"
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "d2x - Win32 Release"
# Name "d2x - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "2d"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\2d\bitblt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\bitmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\canvas.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\circle.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\font.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\ibitblt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\palette.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\pcx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\pixel.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\rle.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\scalec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\string.cpp
# End Source File
# Begin Source File

SOURCE=..\..\2d\tmerge.cpp
# End Source File
# End Group
# Begin Group "3d"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\3d\ase2model.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\buildmodel.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\clipper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\draw.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\fastmodels.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\globvars.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\hitbox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\interp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\matrix.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\oof2model.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\pof2model.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\points.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\rod.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\setup.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\shadows.cpp
# End Source File
# Begin Source File

SOURCE=..\..\3d\transformation.cpp
# End Source File
# End Group
# Begin Group "misc"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\console\console.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cquicksort.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\crypt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\console\cvar.cpp
# End Source File
# Begin Source File

SOURCE=..\..\misc\error.cpp
# End Source File
# Begin Source File

SOURCE=..\..\misc\hash.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mem\mem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\misc\strio.cpp
# End Source File
# Begin Source File

SOURCE=..\..\misc\strutil.cpp
# End Source File
# End Group
# Begin Group "main"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\main\briefings.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\cheats.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\cmd.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\controls.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\credits.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\descent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\dropobject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\effects.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\endlevel.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\flightpath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\fuelcen.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\game.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\gameargs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\gamecntl.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\gamedata.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\gamefolders.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\gameoptions.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\gameseg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\gamestates.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\highscores.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\kconfig.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\mglobal.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\movie.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\newdemo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\scores.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\segment.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\side.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\skybox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\slew.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\terrain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\texmerge.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\text.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\textdata.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\trackobject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\trigger.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\vclip.cpp
# End Source File
# Begin Source File

SOURCE=..\..\main\wall.cpp
# End Source File
# End Group
# Begin Group "maths"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\maths\fixc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\maths\rand.cpp
# End Source File
# Begin Source File

SOURCE=..\..\maths\tables.cpp
# End Source File
# Begin Source File

SOURCE=..\..\maths\vecmat.cpp
# End Source File
# End Group
# Begin Group "ai"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\ai\ai.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\aianimate.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\aiboss.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\aifire.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\aiframe.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\aiinit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\ailib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\aimove.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\aipath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\aisave.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\aisnipe.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\d1_ai.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\escort.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ai\thief.cpp
# End Source File
# End Group
# Begin Group "network"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\network\autodl.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\banlist.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\win32\ipx_mcast4.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\win32\ipx_win.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\multi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\multibot.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\multimsg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\netmisc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\network.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\network_init.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\network_join.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\network_lib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\network_phandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\network_process.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\network_read.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\network_send.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\network_sync.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\tracker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\udp_interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\network\win32\winnet.cpp
# End Source File
# End Group
# Begin Group "audio"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\audio\audio.cpp
# End Source File
# Begin Source File

SOURCE=..\..\audio\win32\hmpfile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\audio\win32\midi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\audio\objectaudio.cpp
# End Source File
# Begin Source File

SOURCE=..\..\audio\rbaudio.cpp
# End Source File
# Begin Source File

SOURCE=..\..\audio\songs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\audio\soundthreads.cpp
# End Source File
# End Group
# Begin Group "cockpit"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\cockpit\cockpit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\cockpit\cockpitdata.cpp
# End Source File
# Begin Source File

SOURCE=..\..\cockpit\genericcockpit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\cockpit\hud.cpp
# End Source File
# Begin Source File

SOURCE=..\..\cockpit\hudicons.cpp
# End Source File
# Begin Source File

SOURCE=..\..\cockpit\hudmsgs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\cockpit\statusbar.cpp
# End Source File
# End Group
# Begin Group "effects"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\effects\fireball.cpp
# End Source File
# Begin Source File

SOURCE=..\..\effects\glare.cpp
# End Source File
# Begin Source File

SOURCE=..\..\effects\lightning.cpp
# End Source File
# Begin Source File

SOURCE=..\..\effects\objsmoke.cpp
# End Source File
# Begin Source File

SOURCE=..\..\effects\particles.cpp
# End Source File
# Begin Source File

SOURCE=..\..\effects\perlin.cpp
# End Source File
# Begin Source File

SOURCE=..\..\effects\shrapnel.cpp
# End Source File
# Begin Source File

SOURCE=..\..\effects\sparkeffect.cpp
# End Source File
# End Group
# Begin Group "gameio"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\main\config.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\createmesh.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\gamemine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\gamesave.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\hoard.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\iff.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\loadgame.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\loadgamedata.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\loadmodeldata.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\loadsounds.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\loadtextures.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\mission.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\paging.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\piggy.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\playsave.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\state.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gameio\tga.cpp
# End Source File
# End Group
# Begin Group "gamemodes"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\gamemodes\entropy.cpp
# End Source File
# Begin Source File

SOURCE=..\..\gamemodes\monsterball.cpp
# End Source File
# End Group
# Begin Group "input"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\input\event.cpp
# End Source File
# Begin Source File

SOURCE=..\..\input\input.cpp
# End Source File
# Begin Source File

SOURCE=..\..\input\joy.cpp
# End Source File
# Begin Source File

SOURCE=..\..\input\key.cpp
# End Source File
# Begin Source File

SOURCE=..\..\input\mouse.cpp
# End Source File
# Begin Source File

SOURCE=..\..\input\systemkeys.cpp
# End Source File
# End Group
# Begin Group "io"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\io\args.cpp
# End Source File
# Begin Source File

SOURCE=..\..\io\cfile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\io\d_io.cpp
# End Source File
# Begin Source File

SOURCE=..\..\io\win32\findfile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\io\hogfile.cpp
# End Source File
# End Group
# Begin Group "lighting"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\lighting\dynlight.cpp
# End Source File
# Begin Source File

SOURCE=..\..\lighting\headlight.cpp
# End Source File
# Begin Source File

SOURCE=..\..\lighting\light.cpp
# End Source File
# Begin Source File

SOURCE=..\..\lighting\lightcluster.cpp
# End Source File
# Begin Source File

SOURCE=..\..\lighting\lightmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\lighting\lightprecalc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\lighting\lightrender.cpp
# End Source File
# Begin Source File

SOURCE=..\..\lighting\lightshaders.cpp
# End Source File
# End Group
# Begin Group "models"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\models\aseread.cpp
# End Source File
# Begin Source File

SOURCE=..\..\models\hiresmodels.cpp
# End Source File
# Begin Source File

SOURCE=..\..\models\oofmath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\models\oofread.cpp
# End Source File
# Begin Source File

SOURCE=..\..\models\pof.cpp
# End Source File
# Begin Source File

SOURCE=..\..\models\polymodel.cpp
# End Source File
# End Group
# Begin Group "ogl"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\ogl\fbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\gr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_bitmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_color.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_defs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_draw.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_fastrender.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_hudstuff.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_lib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_palette.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_render.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_shader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_texcache.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\ogl_texture.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\pbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\screenshot.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ogl\sdlgl.cpp
# End Source File
# End Group
# Begin Group "render"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\render\automap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\cameras.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\fastrender.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\gpgpu_lighting.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\morph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\objeffects.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\objrender.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\oofrender.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\polymodelrender.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\radar.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\render_lighting.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\renderframe.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\renderlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\rendermine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\rendershadows.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\renderthreads.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\sphere.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\transprender.cpp
# End Source File
# Begin Source File

SOURCE=..\..\render\visculling.cpp
# End Source File
# End Group
# Begin Group "weapons"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\weapons\dropweapon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\weapons\laser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\weapons\omega.cpp
# End Source File
# Begin Source File

SOURCE=..\..\weapons\pickupweapon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\weapons\seismic.cpp
# End Source File
# Begin Source File

SOURCE=..\..\weapons\weapon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\weapons\weaponorder.cpp
# End Source File
# End Group
# Begin Group "physics"

# PROP Default_Filter "*.cpp *.h"
# Begin Source File

SOURCE=..\..\physics\collide.cpp
# End Source File
# Begin Source File

SOURCE=..\..\physics\hitbox_collision.cpp
# End Source File
# Begin Source File

SOURCE=..\..\physics\physics.cpp
# End Source File
# Begin Source File

SOURCE=..\..\physics\slowmotion.cpp
# End Source File
# Begin Source File

SOURCE=..\..\physics\sphere_collision.cpp
# End Source File
# Begin Source File

SOURCE=..\..\physics\visibility.cpp
# End Source File
# End Group
# Begin Group "objects"

# PROP Default_Filter "*.cpp *.h"
# Begin Source File

SOURCE=..\..\objects\boss.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\createobject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\effectobject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\hostage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\marker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\object.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\objectio.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\player.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\playerdeath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\powerup.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\reactor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\robot.cpp
# End Source File
# Begin Source File

SOURCE=..\..\objects\updateobject.cpp
# End Source File
# End Group
# Begin Group "menus"

# PROP Default_Filter "*.cpp *.h"
# Begin Source File

SOURCE=..\..\menus\cockpitmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\configmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\detailsmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\effectsmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\entropymenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\fileselector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\gameplaymenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\inputdevicemenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\listbox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\mainmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\menu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\menubackground.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\menuitem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\messagebox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\miscmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\monsterballmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\msgbox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\netgamebrowser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\netgamehelp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\netgameinfo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\netmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\netplayerbrowser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\newgamemenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\physicsmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\rendermenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\screenresmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\menus\soundmenu.cpp

!IF  "$(CFG)" == "d2x - Win32 Release"

# ADD CPP /I ".\audio\include\win32"
# SUBTRACT CPP /I "..\..\..\audio\include\win32"

!ELSEIF  "$(CFG)" == "d2x - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\3d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ai.h
# End Source File
# Begin Source File

SOURCE=..\..\include\aistruct.h
# End Source File
# Begin Source File

SOURCE=..\..\include\altsound.h
# End Source File
# Begin Source File

SOURCE=..\..\include\args.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ase.h
# End Source File
# Begin Source File

SOURCE=..\..\include\autodl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\automap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\banlist.h
# End Source File
# Begin Source File

SOURCE=..\..\include\bm.h
# End Source File
# Begin Source File

SOURCE=..\..\include\bmread.h
# End Source File
# Begin Source File

SOURCE=..\..\include\briefings.h
# End Source File
# Begin Source File

SOURCE=..\..\include\buildmodel.h
# End Source File
# Begin Source File

SOURCE=..\..\include\byteswap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cameras.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cfile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cheats.h
# End Source File
# Begin Source File

SOURCE=..\..\include\checker.h
# End Source File
# Begin Source File

SOURCE=..\..\include\clipper.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cmd.h
# End Source File
# Begin Source File

SOURCE=..\..\include\collide.h
# End Source File
# Begin Source File

SOURCE=..\..\include\compbit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\CON_console.h
# End Source File
# Begin Source File

SOURCE=..\..\include\config.h
# End Source File
# Begin Source File

SOURCE=..\..\include\console.h
# End Source File
# Begin Source File

SOURCE=..\..\include\controls.h
# End Source File
# Begin Source File

SOURCE=..\..\include\credits.h
# End Source File
# Begin Source File

SOURCE=..\..\include\d_io.h
# End Source File
# Begin Source File

SOURCE=..\..\include\desc_id.h
# End Source File
# Begin Source File

SOURCE=..\..\include\desw.h
# End Source File
# Begin Source File

SOURCE=..\..\include\digi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\dropobject.h
# End Source File
# Begin Source File

SOURCE=..\..\include\dynlight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\effects.h
# End Source File
# Begin Source File

SOURCE=..\..\include\endlevel.h
# End Source File
# Begin Source File

SOURCE=..\..\include\entropy.h
# End Source File
# Begin Source File

SOURCE=..\..\include\error.h
# End Source File
# Begin Source File

SOURCE=..\..\include\escort.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fastrender.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fbuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\findfile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fireball.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fix.h
# End Source File
# Begin Source File

SOURCE=..\..\include\flightpath.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fuelcen.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fvi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fvi_a.h
# End Source File
# Begin Source File

SOURCE=..\..\include\game.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gamecntl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gamefont.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gamemine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gamepal.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gamerend.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gamesave.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gameseg.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gamestat.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gauges.h
# End Source File
# Begin Source File

SOURCE=..\..\include\glare.h
# End Source File
# Begin Source File

SOURCE=..\..\include\glext.h
# End Source File
# Begin Source File

SOURCE=..\..\include\globvars.h
# End Source File
# Begin Source File

SOURCE=..\..\include\glxext.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gr.h
# End Source File
# Begin Source File

SOURCE=..\..\include\grdef.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hash.h
# End Source File
# Begin Source File

SOURCE=..\..\include\headlight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\highscores.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hiresmodels.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hitbox.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hmpfile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hostage.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hud_defs.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hudicons.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hudmsg.h
# End Source File
# Begin Source File

SOURCE=..\..\include\i86.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ibitblt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iff.h
# End Source File
# Begin Source File

SOURCE=..\..\include\inferno.h
# End Source File
# Begin Source File

SOURCE=..\..\include\input.h
# End Source File
# Begin Source File

SOURCE=..\..\include\interp.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ipx.h
# End Source File
# Begin Source File

SOURCE=..\..\include\kconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\include\laser.h
# End Source File
# Begin Source File

SOURCE=..\..\include\light.h
# End Source File
# Begin Source File

SOURCE=..\..\include\lightmap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\lightning.h
# End Source File
# Begin Source File

SOURCE=..\..\include\loadgame.h
# End Source File
# Begin Source File

SOURCE=..\..\include\loadgl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\makesig.h
# End Source File
# Begin Source File

SOURCE=..\..\include\marker.h
# End Source File
# Begin Source File

SOURCE=..\..\include\maths.h
# End Source File
# Begin Source File

SOURCE=..\..\include\menu.h
# End Source File
# Begin Source File

SOURCE=..\..\include\midi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mission.h
# End Source File
# Begin Source File

SOURCE=..\..\include\modem.h
# End Source File
# Begin Source File

SOURCE=..\..\include\modex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mono.h
# End Source File
# Begin Source File

SOURCE=..\..\include\monsterball.h
# End Source File
# Begin Source File

SOURCE=..\..\include\morph.h
# End Source File
# Begin Source File

SOURCE=..\..\include\movie.h
# End Source File
# Begin Source File

SOURCE=..\..\include\multi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\multibot.h
# End Source File
# Begin Source File

SOURCE=..\..\include\multimsg.h
# End Source File
# Begin Source File

SOURCE=..\..\include\netmenu.h
# End Source File
# Begin Source File

SOURCE=..\..\include\netmisc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\network.h
# End Source File
# Begin Source File

SOURCE=..\..\include\network_lib.h
# End Source File
# Begin Source File

SOURCE=..\..\include\newdemo.h
# End Source File
# Begin Source File

SOURCE=..\..\include\newmenu.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nocfile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\object.h
# End Source File
# Begin Source File

SOURCE=..\..\include\objeffects.h
# End Source File
# Begin Source File

SOURCE=..\..\include\objrender.h
# End Source File
# Begin Source File

SOURCE=..\..\include\objsmoke.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_bitmap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_color.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_defs.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_fastrender.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_hudstuff.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_init.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_lib.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_render.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_shader.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_texcache.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_texture.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_tmu.h
# End Source File
# Begin Source File

SOURCE=..\..\include\omega.h
# End Source File
# Begin Source File

SOURCE=..\..\include\oof.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pa_enabl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\paging.h
# End Source File
# Begin Source File

SOURCE=..\..\include\palette.h
# End Source File
# Begin Source File

SOURCE=..\..\include\particles.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pbuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pcx.h
# End Source File
# Begin Source File

SOURCE=..\..\include\perlin.h
# End Source File
# Begin Source File

SOURCE=..\..\include\physics.h
# End Source File
# Begin Source File

SOURCE=..\..\include\piggy.h
# End Source File
# Begin Source File

SOURCE=..\..\include\player.h
# End Source File
# Begin Source File

SOURCE=..\..\include\playsave.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pof.h
# End Source File
# Begin Source File

SOURCE=..\..\include\polyobj.h
# End Source File
# Begin Source File

SOURCE=..\..\include\powerup.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pstypes.h
# End Source File
# Begin Source File

SOURCE=..\..\include\radar.h
# End Source File
# Begin Source File

SOURCE=..\..\include\rbaudio.h
# End Source File
# Begin Source File

SOURCE=..\..\include\reactor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\render.h
# End Source File
# Begin Source File

SOURCE=..\..\include\renderlib.h
# End Source File
# Begin Source File

SOURCE=..\..\include\rendershadows.h
# End Source File
# Begin Source File

SOURCE=..\..\include\renderthreads.h
# End Source File
# Begin Source File

SOURCE=..\..\include\reorder.h
# End Source File
# Begin Source File

SOURCE=..\..\include\rle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\robot.h
# End Source File
# Begin Source File

SOURCE=..\..\include\scores.h
# End Source File
# Begin Source File

SOURCE=..\..\include\screens.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sdlgl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\segment.h
# End Source File
# Begin Source File

SOURCE=..\..\include\segpoint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\shadows.h
# End Source File
# Begin Source File

SOURCE=..\..\include\slew.h
# End Source File
# Begin Source File

SOURCE=..\..\include\slowmotion.h
# End Source File
# Begin Source File

SOURCE=..\..\include\songs.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sounds.h
# End Source File
# Begin Source File

SOURCE=..\..\include\soundthreads.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sparkeffect.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sphere.h
# End Source File
# Begin Source File

SOURCE=..\..\include\state.h
# End Source File
# Begin Source File

SOURCE=..\..\include\statusbar.h
# End Source File
# Begin Source File

SOURCE=..\..\include\strio.h
# End Source File
# Begin Source File

SOURCE=..\..\include\strutil.h
# End Source File
# Begin Source File

SOURCE=..\..\include\terrain.h
# End Source File
# Begin Source File

SOURCE=..\..\include\texmap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\texmerge.h
# End Source File
# Begin Source File

SOURCE=..\..\include\text.h
# End Source File
# Begin Source File

SOURCE=..\..\include\textdata.h
# End Source File
# Begin Source File

SOURCE=..\..\include\textures.h
# End Source File
# Begin Source File

SOURCE=..\..\include\tga.h
# End Source File
# Begin Source File

SOURCE=..\..\include\timer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\tracker.h
# End Source File
# Begin Source File

SOURCE=..\..\include\trackobject.h
# End Source File
# Begin Source File

SOURCE=..\..\include\transprender.h
# End Source File
# Begin Source File

SOURCE=..\..\include\trigger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\u_dpmi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\u_mem.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ui.h
# End Source File
# Begin Source File

SOURCE=..\..\include\vclip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\vecmat.h
# End Source File
# Begin Source File

SOURCE=..\..\include\vers_id.h
# End Source File
# Begin Source File

SOURCE=..\..\include\vesa.h
# End Source File
# Begin Source File

SOURCE=..\..\include\wall.h
# End Source File
# Begin Source File

SOURCE=..\..\include\weapon.h
# End Source File
# Begin Source File

SOURCE=..\..\include\wglext.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE="..\d2x-w32.ico"
# End Source File
# Begin Source File

SOURCE="..\d2x-xl.ico"
# End Source File
# Begin Source File

SOURCE="..\d2x.ico"
# End Source File
# Begin Source File

SOURCE=..\d2x.w32.rc
# End Source File
# Begin Source File

SOURCE=..\descent.ico
# End Source File
# Begin Source File

SOURCE=..\icon1.ico
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# End Group
# End Target
# End Project
