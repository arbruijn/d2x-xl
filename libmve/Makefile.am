EXTRA_PROGRAMS = mveplayer

mveplayer_SOURCES = decoder16.cpp decoder8.cpp mve_audio.cpp mve_main.cpp mvelib.cpp mveplay.cpp

mveplayer_MANS = mveplayer.1


noinst_LIBRARIES = libmve.a

INCLUDES = -I $(top_srcdir)/include -I $(top_srcdir)/input/include -I $(top_srcdir)/network/linux/include -I $(top_srcdir)/audio/linux/include -I /usr/local/include/SDL

libmve_a_SOURCES = decoder16.cpp decoder8.cpp mve_audio.cpp mvelib.cpp mveplay.cpp

EXTRA_DIST = decoders.h mve_audio.h mvelib.h $(mveplayer_MANS)
