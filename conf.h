/* conf.h.  Generated from conf.h.in by configure.  */
/* conf.h.in.  Generated from configure.ac by autoheader.  */

/* Define to enable console */
/* #undef CONSOLE */

/* d2x major version */
#define D2XMAJOR 1

/* d2x micro version */
#define D2XMICRO 36

/* d2x minor version */
#define D2XMINOR 13

/* Define if you want to build the editor */
/* #undef EDITOR */

/* Define for faster input device polling when using SDL */
#define FAST_EVENTPOLL 

/* Define for faster i/o on little-endian cpus */
/* #undef FAST_FILE_IO */

/* Define if you want a GGI build */
/* #undef GGI */

/* Define to 1 if you have the declaration of `nanosleep', and to 0 if you
   don't. */
#define HAVE_DECL_NANOSLEEP 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <netipx/ipx.h> header file. */
#define HAVE_NETIPX_IPX_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if the system has the type `struct timespec'. */
#define HAVE_STRUCT_TIMESPEC 1

/* Define to 1 if the system has the type `struct timeval'. */
#define HAVE_STRUCT_TIMEVAL 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to enable use of the KaliNix driver */
#define KALINIX 

/* Define if you want to build for mac datafiles */
/* #undef MACDATA */

/* Define to use the IPX support of the OS */
#define NATIVE_IPX 

/* Define to disable asserts, int3, etc. */
#define NDEBUG 

/* Define if you want a network build */
#define NETWORK 

/* Define if you want an assembler free build */
#define NO_ASM 

/* Define if you want an OpenGL build */
#define OGL 

/* Define if you want D2X to use Z-buffering */
#define OGL_ZBUF 1

/* Name of package */
#define PACKAGE "d2x-xl"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "karx11erx@hotmail.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "d2x-xl"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "d2x-xl 1.13.36"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "d2x-xl"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.13.36"

/* Define for a "release" build */
#define RELEASE 

/* Define this to be the shared game directory root */
#define SHAREPATH "${prefix}/share/d2x-xl"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define if you want an SVGALib build */
/* #undef SVGA */

/* define to not use the SDL_Joystick routines. */
/* #undef USE_LINUX_JOY */

/* Define to enable MIDI playback using SDL_mixer */
#define USE_SDL_MIXER 1

/* Version number of package */
#define VERSION "1.13.36"

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define if your processor needs data to be word-aligned */
/* #undef WORDS_NEED_ALIGNMENT */

/* Define to enable asserts, int3, etc. */
/* #undef _DEBUG */


        /* General defines */
#ifndef PACKAGE_STRING
#define PACKAGE_STRING "d2x-xl 1.13.36"
#endif
#define VERSION_NAME PACKAGE_STRING
#define NMONO 1
#define PIGGY_USE_PAGING 1
#define NEWDEMO 1

#if defined(__APPLE__) && defined(__MACH__)
#define __unix__ /* since we're doing a unix-style compilation... */
#endif

#ifdef __unix__
# ifdef GGI
#  define GII_INPUT 1
#  define GGI_VIDEO 1
# else
#  ifdef SVGA
#   define SVGALIB_INPUT 1
#   define SVGALIB_VIDEO 1
#  else
#   define SDL_INPUT 1
#   ifdef OGL
#    define SDL_GL_VIDEO 1
#   else
#    define SDL_VIDEO 1
#   endif
#  endif
# endif
#endif

#ifdef _WIN32
# define SDL_INPUT 1
# ifdef OGL
#  define SDL_GL_VIDEO 1
# else
#  define SDL_VIDEO 1
# endif
#endif
        
