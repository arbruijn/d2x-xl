#define __macosx__

/* Set to 1 to use SDL Mixer */
#define USE_SDL_MIXER 1

#define USE_SDL_IMAGE 1

/* Define to enable console */
#define CONSOLE
#define TRACE 1

/* Define for faster input device polling when using SDL */
#define FAST_EVENTPOLL 

/* Define for faster i/o on little-endian cpus */
#ifdef __LITTLE_ENDIAN__
#define FAST_FILE_IO
#endif

/* Define to 1 if you have the declaration of `nanosleep', and to 0 if you
don't. */
#define HAVE_DECL_NANOSLEEP 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if the system has the nType `struct timespec'. */
#define HAVE_STRUCT_TIMESPEC 1

/* Define to 1 if the system has the nType `struct timeval'. */
#define HAVE_STRUCT_TIMEVAL 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if you want a network build */
#define NETWORK 

/* Define if you want an assembler free build */
#define NO_ASM 

/* Define if you want an OpenGL build */
#define OGL 

/* Define if you want D2X to use Z-buffering */
#define OGL_ZBUF 1


/* Define if you have the SDL_image library */
/* #undef SDL_IMAGE */

/* Define this to be the shared game directory root */
#define SHAREPATH "/usr/local/share/games/d2x-xl"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

#define NMONO 1
#define PIGGY_USE_PAGING 1
#define NEWDEMO 1

#define SDL_INPUT 1
#define SDL_GL_VIDEO 1
