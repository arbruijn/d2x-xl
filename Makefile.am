DIST_SUBDIRS = \
	2d 3d ai audio cockpit console effects gameio gamemodes input io libmve lighting \
	main maths menus misc models network objects ogl physics render weapons

SUBDIRS = ${DIST_SUBDIRS} @D2X_SUBDIRS@

EXTRA_SUBDIRS = include

bin_PROGRAMS = @TARGETS@

EXTRA_PROGRAMS = d2x-xl 

if MINGW32
LD_KLUDGE = main/descent.o
endif

if MACOSX
AM_LDFLAGS = -all_load
endif

if CONSOLE
CONSOLE_LIBS = console/libconsole.a
endif

d2x_xl_SOURCES =

d2x_LDADD = \
	${LD_KLUDGE} \
	2d/lib2d.a \
	main/libmain.a \
	audio/libaudio.a \
	console/libconsole.a \
	menus/libmenus.a \
	cockpit/libcockpit.a \
	effects/libeffects.a \
	gameio/libgameio.a \
	gamemodes/libgamemodes.a \
	input/libinput.a \
	io/libio.a \
	libmve/libmve.a \
	lighting/liblighting.a \
	maths/libmaths.a \
	misc/libmisc.a \
	models/libmodels.a \
	network/libnetwork.a \
	menus/libmenus.a \
	objects/libobjects.a \
	ogl/libogl.a \
	physics/libphysics.a \
	render/librender.a \
	lighting/liblighting.a \
	menus/libmenus.a \
	cockpit/libcockpit.a \
	weapons/libweapons.a \
	2d/lib2d.a \
	3d/lib3d.a \
	ai/libai.a \
	main/libmain.a \
	ogl/libogl.a \
	console/libconsole.a \
	${CONSOLE_LIBS}

if MINGW32
d2x_sdl_LDADD = ${d2x_LDADD} -lwinmm -lole32 -lglew32 -lurlmon
if USE_NETWORK
d2x_sdl_LDADD += -lwsock32 -lws2_32
endif
else
d2x_sdl_LDADD = ${d2x_LDADD} -lm -lX11 -lGLEW -lcurl
endif
d2x_xl_LDADD = ${d2x_sdl_LDADD} -lz -lstdc++ ${OGL_LIBS}

dist-hook:
	rm -rf `find $(distdir) -name CVS`
#	rm -f `find $(distdir) -name Makefile`

pkgdatadir = @sharepath@
missiondir = ${pkgdatadir}/missions

CLEANFILES = @TARGETS@ Info.plist

EXTRA_DIST = ${EXTRA_SUBDIRS} \
	ChangeLog ChangeLog-old \
	D2X.make \
	MVEPlayer-Info.plist d2x-Info.plist d2xgl-Info.plist \
	README.utils \
	djgpp.bat djgpp.sh \
	autogen.sh \
	cvshowto.txt readme.txt installation.txt compilerdefs.txt \
	d2x-xl.sh
