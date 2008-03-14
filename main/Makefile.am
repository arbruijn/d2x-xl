 SUBDIRS = @D2X_MAIN_SUBDIRS@

if EDITOR
DIST_SUBDIRS = editor
endif

EXTRA_SUBDIRS = old

noinst_LIBRARIES = libmain.a

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/arch/include -I$(top_srcdir)/3d

if USE_NETWORK
NETWORK_SRCS = highscores.cpp multi.cpp multibot.cpp network.cpp netmisc.cpp \
network_init.cpp network_join.cpp network_lib.cpp network_phandler.cpp \
network_process.cpp network_read.cpp network_send.cpp network_sync.cpp \
dumpmine.cpp
endif

if EDITOR
EDITOR_SRCS = bmread.cpp dumpmine.cpp
endif

libmain_a_SOURCES = ${EDITOR_SRCS} ${NETWORK_SRCS} \
ai.cpp        aianimate.cpp	aiboss.cpp		aifire.cpp		aiframe.cpp	\
aiinit.cpp		ailib.cpp		aimove.cpp		aipath.cpp		aisave.cpp		\		
aisnipe.cpp	automap.cpp   bm.cpp        sparkeffect.cpp				\
cmd.cpp       cntrlcen.cpp  collide.cpp   config.cpp    console.cpp   \
controls.cpp  credits.cpp   crypt.cpp     digiobj.cpp   effects.cpp   \
escort.cpp    fireball.cpp  fuelcen.cpp   fvi.cpp       game.cpp      \
gamecntl.cpp  gamefont.cpp  gamemine.cpp  gamepal.cpp   gamerend.cpp  \
gamesave.cpp  gameseg.cpp   loadgame.cpp  gauges.cpp    hostage.cpp   \
hud.cpp       inferno.cpp   kconfig.cpp   kludge.cpp    laser.cpp     \
light.cpp		menu.cpp      mglobal.cpp   mission.cpp   morph.cpp     \
movie.cpp     particles.cpp aseread.cpp 	sphere.cpp 	input.cpp		\
newdemo.cpp   newmenu.cpp   object.cpp    paging.cpp    physics.cpp   \
piggy.cpp     player.cpp    playsave.cpp  polyobj.cpp   powerup.cpp   \
render.cpp    robot.cpp     scores.cpp    segment.cpp   slew.cpp      \
songs.cpp     state.cpp     switch.cpp    terrain.cpp   texmerge.cpp  \
text.cpp      briefings.cpp vclip.cpp     wall.cpp      weapon.cpp	\
autodl.cpp		cameras.cpp	lightmap.cpp	tracker.cpp	objsmoke.cpp	\
netmenu.cpp	cheats.cpp		banlist.cpp	thief.cpp		endlevel.cpp	\
multimsg.cpp	hoard.cpp		objrender.cpp lightning.cpp perlin.cpp		\
tga.cpp			glare.cpp		radar.cpp		renderlib.cpp	transprender.cpp \
fastrender.cpp	renderthreads.cpp		flightpath.cpp \
rendershadows.cpp		objeffects.cpp	hiresmodels.cpp textdata.cpp \
omega.cpp trackobject.cpp	monsterball.cpp	slowmotion.cpp	dropobject.cpp	\
entropy.cpp	dynlight.cpp	headlight.cpp	marker.cpp	soundthreads.cpp \
oofread.cpp	oofrender.cpp	oofmath.cpp	hudicons.cpp	statusbar.cpp \
dropweapon.cpp pickupweapon.cpp weaponorder.cpp seismic.cpp


EXTRA_libmain_a_SOURCES = \
highscores.cpp multi.cpp multibot.cpp network.cpp netmisc.cpp


EXTRA_DIST = ${EXTRA_SUBDIRS} \
descent.cppfg inferno.ini \
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
dynlight.h headlight.h marker.h gamerend.h soundthreads.h hud_defs.h statusbar.h \
sparkeffect.h

