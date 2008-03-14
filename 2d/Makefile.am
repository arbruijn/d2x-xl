noinst_LIBRARIES = lib2d.a
INCLUDES = -I $(top_srcdir)/include -I $(top_srcdir)/3d

if MACDATA
TRANSPARENCY_COLOR = 0
else
TRANSPARENCY_COLOR = 255
endif

lib2d_a_SOURCES = \
	2dsline.cpp box.cpp disc.cpp ibitblt.cpp pcx.cpp rect.cpp scalec.cpp \
	bitblt.cpp canvas.cpp font.cpp line.cpp pixel.cpp rle.cpp \
	bitmap.cpp circle.cpp gpixel.cpp palette.cpp poly.cpp

SUFFIXES = .asm
%.o: %.asm
	$(NASM) -dTRANSPARENCY_COLOR=${TRANSPARENCY_COLOR} $(NASMFLAGS) $< -o $@
