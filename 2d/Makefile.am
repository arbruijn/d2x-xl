noinst_LIBRARIES = lib2d.a

INCLUDES = -I $(top_srcdir)/include -I $(top_srcdir)/input/include -I $(top_srcdir)/network/linux/include -I $(top_srcdir)/audio/linux/include -I /usr/local/include/SDL

if MACDATA
TRANSPARENCY_COLOR = 0
else
TRANSPARENCY_COLOR = 255
endif

lib2d_a_SOURCES = \
	bitblt.cpp bitmap.cpp canvas.cpp circle.cpp font.cpp ibitblt.cpp palette.cpp \
	pcx.cpp pixel.cpp poly.cpp rect.cpp rle.cpp scalec.cpp string.cpp tmerge.cpp	
