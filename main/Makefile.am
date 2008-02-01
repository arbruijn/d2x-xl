 SUBDIRS = @D2X_MAIN_SUBDIRS@

if EDITOR
DIST_SUBDIRS = editor
endif

EXTRA_SUBDIRS = old

noinst_LIBRARIES = libmain.a

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/arch/include -I$(top_srcdir)/3d

if USE_NETWORK
NETWORK_SRCS = highscores.c multi.c multibot.c network.c netmisc.c
endif

if EDITOR
EDITOR_SRCS = bmread.c dumpmine.c
endif

libmain_a_SOURCES = ${EDITOR_SRCS} ${NETWORK_SRCS} \
ai.c        ai2.c       aipath.c    automap.c   bm.c        \
cmd.c       cntrlcen.c  collide.c   config.c    console.c   \
controls.c  credits.c   crypt.c     digiobj.c   effects.c   \
escort.c    fireball.c  fuelcen.c   fvi.c       game.c      \
gamecntl.c  gamefont.c  gamemine.c  gamepal.c   gamerend.c  \
gamesave.c  gameseg.c   loadgame.c  gauges.c    hostage.c   \
hud.c       inferno.c   kconfig.c   kludge.c    laser.c     \
light.c		menu.c      mglobal.c   mission.c   morph.c     \
movie.c     particles.c aseread.c 	sphere.c 	input.c		\
newdemo.c   newmenu.c   object.c    paging.c    physics.c   \
piggy.c     player.c    playsave.c  polyobj.c   powerup.c   \
render.c    robot.c     scores.c    segment.c   slew.c      \
songs.c     state.c     switch.c    terrain.c   texmerge.c  \
text.c      briefings.c vclip.c     wall.c      weapon.c	\
autodl.c		cameras.c	lightmap.c	tracker.c	objsmoke.c	\
netmenu.c	cheats.c		banlist.c	thief.c		endlevel.c	\
multimsg.c	hoard.c		objrender.c lightning.c perlin.c		\
tga.c			glare.c		radar.c		renderlib.c	transprender.c \
fastrender.c	renderthreads.c		flightpath.c \
rendershadows.c		objeffects.c	hiresmodels.c textdata.c \
omega.c trackobject.c	monsterball.c	slowmotion.c	dropobject.c	\
entropy.c	dynlight.c	headlight.c	marker.c	soundthreads.c \
oofread.c	oofrender.c	oofmath.c

EXTRA_libmain_a_SOURCES = \
highscores.c multi.c multibot.c network.c netmisc.c \
bmread.c dumpmine.c

EXTRA_DIST = ${EXTRA_SUBDIRS} \
descent.cfg inferno.ini \
link.bat \
ai.h aistruct.h automap.h bm.h bmread.h cntrlcen.h collide.h \
config.h controls.h credits.h desc_id.h desw.h digi.h effects.h \
endlevel.h escort.h oof.h ase.h \
fireball.h fuelcen.h fvi.h fvi_a.h game.h gamefont.h gamemine.h \
gamepal.h gamesave.h gameseg.h loadgame.h gamestat.h gauges.h \
hostage.h hudmsg.h inferno.h joydefs.h kconfig.h \
highscores.h laser.h light.h menu.h mission.h modem.h morph.h movie.h multi.h \
multibot.h netmisc.h network.h newdemo.h newmenu.h \
object.h paging.h physics.h piggy.h player.h playsave.h polyobj.h powerup.h \
render.h reorder.h robot.h scores.h screens.h segment.h segpoint.h slew.h \
songs.h sounds.h state.h switch.h terrain.h texmerge.h text.h textures.h \
briefings.h vclip.h vers_id.h wall.h weapon.h objrender.h lightning.h perlin.h \
autodl.h cameras.h lightmap.h tracker.h findfile.h tga.h \
glare.h radar.h renderlib.h transprender.h fastrender.h renderthreads.h flightpath.h \
objeffects.h hiresmodels.h textdata.h omega.h trackobject.h \
joydefs.h monsterball.h slowmotion.h dropobject.h entropy.h \
dynlight.h headlight.h marker.h gamerend.h soundthreads.h

