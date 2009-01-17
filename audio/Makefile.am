noinst_LIBRARIES = libaudio.a

INCLUDES = -I $(top_srcdir)/include -I $(top_srcdir)/input/include -I $(top_srcdir)/network/linux/include -I $(top_srcdir)/audio/linux/include -I /usr/local/include/SDL

libaudio_a_SOURCES = \
	audio.cpp objectaudio.cpp rbaudio.cpp songs.cpp soundthreads.cpp ./win32/midi.cpp \
	./linux/hmiplay.cpp ./linux/hmistub.cpp ./linux/hmpfile.cpp
	