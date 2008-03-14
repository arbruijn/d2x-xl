noinst_LIBRARIES = libmaths.a
INCLUDES = -I $(top_srcdir)/include -I $(top_srcdir)/3d

if USE_ASM
libmaths_a_SOURCES = vecmat.cpp rand.cpp tables.cpp
else
libmaths_a_SOURCES = vecmat.cpp rand.cpp tables.cpp fixc.cpp
endif

SUFFIXES = .asm
%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

if USE_ASM
libmaths_a_LIBADD = vecmata.o fix.o
endif

EXTRA_DIST = \
fix.asm vecmata.asm \
fixc.cpp rand.cpp tables.cpp vecmat.cpp
