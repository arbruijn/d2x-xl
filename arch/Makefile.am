SUBDIRS = @D2X_ARCH_SUBDIRS@

#deprecated? dos
if USE_SVGALIB_TRUE
if USE_GGI_TRUE
DIST_SUBDIRS = ggi linux ogl sdl svgalib win32
else
DIST_SUBDIRS = linux ogl sdl svgalib win32
endif
else
if USE_GGI_TRUE
DIST_SUBDIRS = ggi linux ogl sdl win32
else
DIST_SUBDIRS = linux ogl sdl win32
endif
endif

EXTRA_SUBDIRS = include

EXTRA_DIST = ${EXTRA_SUBDIRS}
