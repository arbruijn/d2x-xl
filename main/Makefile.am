 SUBDIRS = @D2X_MAIN_SUBDIRS@

if EDITOR
DIST_SUBDIRS = editor
endif

noinst_LIBRARIES = libmain.a

INCLUDES = -I $(top_srcdir)/include -I $(top_srcdir)/input/include -I $(top_srcdir)/network/linux/include -I $(top_srcdir)/audio/linux/include -I /usr/local/include/SDL

libmain_a_SOURCES = ${EDITOR_SRCS} ${NETWORK_SRCS} \
	briefings.cpp cheats.cpp cmd.cpp config.cpp controls.cpp credits.cpp crypt.cpp dropobject.cpp effects.cpp endlevel.cpp \
	flightpath.cpp fuelcen.cpp game.cpp gameargs.cpp gamecntl.cpp gamedata.cpp gamefolders.cpp gameoptions.cpp gameseg.cpp \
	gamestates.cpp highscores.cpp descent.cpp kconfig.cpp mglobal.cpp movie.cpp newdemo.cpp scores.cpp segment.cpp side.cpp \
	skybox.cpp slew.cpp switch.cpp terrain.cpp texmerge.cpp text.cpp textdata.cpp trackobject.cpp vclip.cpp wall.cpp 
	
EXTRA_DIST = ${EXTRA_SUBDIRS} \
	ai.h aistruct.h automap.h bm.h bmread.h collide.h reactor.h \
	config.h controls.h credits.h desc_id.h desw.h digi.h effects.h \
	endlevel.h escort.h oof.h ase.h \
	fireball.h fuelcen.h fvi.h fvi_a.h game.h gamefont.h gamemine.h \
	gamepal.h gamesave.h gameseg.h loadgame.h gamestat.h gauges.h \
	hostage.h hudmsg.h descent.h joydefs.h kconfig.h \
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

