EXTRA_PROGRAMS = mveplayer

mveplayer_SOURCES = decoder16.cpp decoder8.cpp mve_audio.cpp mve_main.cpp mvelib.cpp mveplay.cpp

mveplayer_MANS = mveplayer.1


noinst_LIBRARIES = libmve.a

INCLUDES = -I$(top_srcdir)/include

libmve_a_SOURCES = decoder16.cpp decoder8.cpp mve_audio.cpp mvelib.cpp mveplay.cpp

EXTRA_DIST = decoders.h mve_audio.h mvelib.h $(mveplayer_MANS)
