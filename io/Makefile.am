noinst_LIBRARIES = libio.a

INCLUDES = -I $(top_srcdir)/include -I $(top_srcdir)/input/include -I $(top_srcdir)/network/linux/include -I $(top_srcdir)/audio/linux/include -I /usr/local/include/SDL

if MINGW32
libio_a_SOURCES = \
	args.cpp cfile.cpp hogfile.cpp \
	./win32/findfile.cpp
else
libio_a_SOURCES = \
	args.cpp cfile.cpp hogfile.cpp \
	./linux/findfile.cpp 
endif
