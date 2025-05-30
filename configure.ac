# 
-*- mode: shell-script -*-

define([D2X_MAJOR],1)
define([D2X_MINOR],18)
define([D2X_MICRO],74)
define([D2X_VARIANT],ar10)

AC_INIT(d2x-xl,[D2X_MAJOR.D2X_MINOR.D2X_MICRO-D2X_VARIANT],[karx11erx@hotmail.com])
AC_PREREQ(2.52)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_EXEEXT
AC_OBJEXT

AM_INIT_AUTOMAKE([1.6 subdir-objects])

AM_CONFIG_HEADER(conf.h)

AC_DEFINE_UNQUOTED(D2XMAJOR,D2X_MAJOR,[d2x major version])
AC_DEFINE_UNQUOTED(D2XMINOR,D2X_MINOR,[d2x minor version])
AC_DEFINE_UNQUOTED(D2XMICRO,D2X_MICRO,[d2x micro version])
AC_DEFINE_UNQUOTED(D2XVARIANT,"D2X_VARIANT",[d2x variant])

AM_MAINTAINER_MODE

#kludge to keep autoconf from adding -g -O2
CFLAGS=" $CFLAGS"
CXXFLAGS=" $CXXFLAGS"

AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_ISC_POSIX
AC_CHECK_TOOL(CC, gcc, :)
AC_CHECK_TOOL(CXX, g++, :)
AC_CHECK_TOOL(RANLIB, ranlib, :)
AC_CHECK_TOOL(STRIP, strip, :)
AC_CHECK_TOOL(AR, ar, :)
AC_PROG_CC
AC_PROG_CXX
AC_STDC_HEADERS
AC_PATH_PROGS(NASM, nasm nasmw, no)
AC_C_BIGENDIAN

case $host_cpu in
	sparc)
		AC_DEFINE(WORDS_NEED_ALIGNMENT,,[Define if your processor needs data to be word-aligned])
	;;
esac

case $host_os in
    cygwin)
        AC_MSG_WARN([Cygwin found, setting -mno-cygwin])
        CFLAGS="-mno-cygwin $CFLAGS"
        MINGW32=yes
        ;;
    mingw*)
        MINGW32=yes
        ;;
    darwin*)
        MACOSX=yes
	CFLAGS="-no-cpp-precomp $CFLAGS"
	;;
    msdos*)
        MSDOS=yes
        ;;
esac

AC_CHECK_DECLS(nanosleep,,,[#include <time.h>])
AC_CHECK_TYPES([struct timespec, struct timeval],,,[#include <sys/time.h>])

AM_CONDITIONAL(MINGW32, test x$MINGW32 = xyes)
if test x$MINGW32 = xyes; then
    GL_LIBS="opengl32 glu32"
elif test x$MACOSX = xyes; then
    GL_LIBS=""
else
    GL_LIBS="GL GLU"
fi

AM_CONDITIONAL(MACOSX, test x$MACOSX = xyes)

dnl Set $prefix and $exec_prefix to $ac_default_prefix if they are not set
test "x$prefix" = "xNONE" && prefix=$ac_default_prefix
test "x$exec_prefix" = "xNONE" && exec_prefix="${prefix}"

dnl Set default locations to find data files.
default_sharepath="$datadir/games/$PACKAGE"

CFLAGS="-Wall -Wno-char-subscripts $CFLAGS"
CXXFLAGS="-Wall -Wno-char-subscripts $CXXFLAGS"

D2X_SUBDIRS=""
D2X_MAIN_SUBDIRS=""

dnl Enable editor build?
AC_ARG_ENABLE(editor,
    [  --enable-editor         Build editor? (not functional)],,)
if test x$enable_editor = xyes; then
    AC_DEFINE(EDITOR,,[Define if you want to build the editor])
    D2X_SUBDIRS="ui ${D2X_SUBDIRS}"
    D2X_MAIN_SUBDIRS="editor ${D2X_MAIN_SUBDIRS}"
fi
AM_CONDITIONAL(EDITOR, test x$enable_editor = xyes)

dnl Enable macdata build?
AC_ARG_ENABLE(macdata,
    [  --enable-macdata        Build for mac datafiles],,)
if test x$enable_macdata = xyes; then
    AC_DEFINE(MACDATA,,[Define if you want to build for mac datafiles])
    D2X_FEATURES="macdata ${D2X_FEATURES}"
fi
AM_CONDITIONAL(MACDATA, test x$enable_macdata = xyes)

dnl Build with NDEBUG?
AC_ARG_ENABLE(debug,
    [  --enable-debug         Take out debug symbols, Asserts, Int3, etc ],,)
if test x$enable_debug != xyes; then
    AC_DEFINE(NDEBUG,,[Define to disable asserts, int3, etc.])
    CFLAGS="-O3 $CFLAGS"
    CXXFLAGS="-O3 $CXXFLAGS"
   else
    CFLAGS="-g $CFLAGS"
    CXXFLAGS="-g $CXXFLAGS"
    AC_DEFINE(_DEBUG,,[Define to enable asserts, int3, etc.])
    D2X_FEATURES="debug ${D2X_FEATURES}"
fi

dnl Build with OpenMP?
AC_ARG_ENABLE(openmp,
    [  --enable-openmp         OpenMP multithreading build ],,)
if test x$enable_openmp = xyes; then
    CFLAGS="$CFLAGS -fopenmp"
    CXXFLAGS="$CXXFLAGS -fopenmp"
#    AC_DEFINE(OPENMP,,[Define to enable openmp code])
fi

dnl Build with RELEASE?
AC_ARG_ENABLE(release,
    [  --enable-release        Release build.  Removes debug/editor things ],,)
if test x$enable_release != xno; then
    AC_DEFINE(RELEASE,,[Define for a "release" build])
    D2X_FEATURES="release ${D2X_FEATURES}"
fi

dnl Build without using sdl-joystick ?
AC_ARG_ENABLE(linuxjoy,
    [  --enable-linuxjoy       do not use the SDL Joystick. (eg for MS Sidewinder)],,)
if test x$enable_linuxjoy = xyes; then
    AC_DEFINE(USE_LINUX_JOY,,[define to not use the SDL_Joystick routines.])
    D2X_FEATURES="linuxjoy ${D2X_FEATURES}"
fi
AM_CONDITIONAL(USE_LINUX_JOY, test x$enable_linuxjoy = xyes)

dnl Build with FAST_FILE_IO?
AC_ARG_ENABLE(fastfileio,
    [  --disable-fastfileio    Disable fast file i/o. ],,)
if test x$enable_fastfileio == xyes; then
	if test x$ac_cv_c_bigendian = xyes; then
		AC_MSG_WARN([big-endian cpu detected.  disabling fastfileio])
		enable_fastfileio="no";
	fi
fi
if test x$enable_fastfileio == xyes; then
    AC_DEFINE(FAST_FILE_IO,,[Define for faster i/o on little-endian cpus])
    D2X_FEATURES="fastfileio ${D2X_FEATURES}"
fi

dnl Build with FAST_EVENTPOLL?
AC_ARG_ENABLE(fasteventpoll,
    [  --disable-fasteventpoll    Disable fast input device polling. ],,)
if test x$enable_fasteventpoll != xno; then
    AC_DEFINE(FAST_EVENTPOLL,,[Define for faster input device polling when using SDL])
    D2X_FEATURES="fasteventpoll ${D2X_FEATURES}"
fi
dnl Build with CONSOLE?
AC_ARG_ENABLE(console,
    [  --enable-console        Enable console (experimental) ],,)
if test x$enable_console = xyes; then
    AC_DEFINE(CONSOLE,,[Define to enable console])
    D2X_SUBDIRS="console ${D2X_SUBDIRS}"
fi
AM_CONDITIONAL(CONSOLE, test x$enable_console = xyes)

AC_ARG_WITH(sharepath,
    [[  --with-sharepath=DIR    Use DIR for shared game data (unix only) [DATADIR/games/d2x]]],
    sharepath=$withval, sharepath="auto")
if test "x$sharepath" = "xauto" -o "x$sharepath" = "xyes" -o "x$sharepath" = "x"; then
    sharepath="$default_sharepath"
fi
AC_SUBST(sharepath)
# eval sharepath=$sharepath
# AC_DEFINE_UNQUOTED(SHAREPATH, "$sharepath",[Define this to be the shared game directory root])

# Test for MSDOS
if test x$MSDOS = xyes; then
    CFLAGS="-I \$(top_srcdir)/arch/dos/include $CFLAGS"
    TARGETS=d2x
    D2X_ARCH_SUBDIRS=dos
else

    CFLAGS="-pipe $CFLAGS"

    dnl Check for SDL
    AM_PATH_SDL(1.2.8,
        :,
        AC_MSG_ERROR(SDL not found. Make sure sdl-config is in your PATH, or specify with --with-sdl-prefix)
    )
    CFLAGS="$SDL_CFLAGS -I /usr/include/SDL $CFLAGS"
    CXXFLAGS="$SDL_CFLAGS $CXXFLAGS"
    LIBS="$SDL_LIBS $LIBS"

    TARGETS=d2x-xl

    AC_CHECK_HEADERS(netipx/ipx.h)

  # Check for SDL_image
    AC_CHECK_LIB(SDL_image, IMG_Load,
        LIBS="-lSDL_image $LIBS";
        AC_DEFINE(USE_SDL_IMAGE,1,[Define if you have the SDL_image library]))

  	# Check for SDL_mixer
   	AC_CHECK_LIB(
			SDL_mixer, Mix_OpenAudio,
        	LIBS="-lSDL_mixer $LIBS";
         AC_DEFINE(USE_SDL_MIXER,1,[Define to enable MIDI playback using SDL_mixer]))

  	# Check for SDL_net
   	AC_CHECK_LIB(
			SDL_net, SDLNet_Init,
        	LIBS="-lSDL_net $LIBS";
         AC_DEFINE(USE_SDL_NET,1,[Define to enable usage of TCP networking functions]))

  # Check for libpng
    have_libpng=no
    #PKG_CHECK_MODULES(LIBPNG, libpng,
    #	AC_DEFINE(HAVE_LIBPNG,,[Define if you have libpng])
    #	have_libpng=yes
    #	LIBS="$LIBPNG_LIBS $LIBS",
    #	AC_MSG_WARN([libpng not found. disabling png replacement texture support])
#)
    AM_CONDITIONAL(USE_LIBPNG, test x$have_libpng = xyes)

  # Check for PhysicsFS
#    AC_CHECK_HEADERS(physfs.h, , [AC_MSG_ERROR([physfs.h required])])
#    AC_CHECK_LIB(physfs, PHYSFS_init, LIBS="-lphysfs $LIBS",
#                 [AC_MSG_ERROR([physfs library required])])

  # Check for OpenGL
    AC_ARG_WITH(opengl,
        [  --with-opengl           Build OpenGL support ],,)
    if test x$with_opengl != xno; then
        for lib in $GL_LIBS; do
            AC_CHECK_LIB(
					$lib, main,
               OGL_LIBS="${OGL_LIBS} -l$lib",
               [AC_MSG_ERROR([$lib not found, --with-opengl cannot be used])
                opengl=false],
               ${OGL_LIBS}
            )
        done
        AC_DEFINE(OGL,,[Define if you want an OpenGL build])
        AC_DEFINE(OGL_ZBUF,1,[Define if you want D2X to use Z-buffering])
        TARGETS=d2x-xl
        D2X_ARCH_SUBDIRS="ogl ${D2X_ARCH_SUBDIRS}"
    fi
    AC_SUBST(OGL_LIBS)
    D2X_FEATURES="OpenGL ${D2X_FEATURES}"
    AM_CONDITIONAL(USE_OPENGL, test x$with_opengl != xno)

  # Check for GGI
    AC_ARG_WITH(ggi,
        [  --with-ggi              Build GGI support ],,)
    if test x$with_ggi = xyes; then
        AC_CHECK_LIB(ggi, ggiInit,
            GGI_LIBS="-lggi",
            [AC_MSG_ERROR([GGI not found, GGI version cannot be built]); ggi=false])
        AC_CHECK_LIB(gii, giiInit,
            GGI_LIBS="${GGI_LIBS} -lgii",
            [AC_MSG_ERROR([GII not found, GGI version cannot be built]); ggi=false])
        AC_DEFINE(GGI,,[Define if you want a GGI build])
        TARGETS=d2x-ggi
        D2X_ARCH_SUBDIRS="ggi ${D2X_ARCH_SUBDIRS}"
    fi
    AC_SUBST(GGI_LIBS)
    AM_CONDITIONAL(USE_GGI, test x$with_ggi = xyes)

  # Check for SVGALib
    AC_ARG_WITH(svga,
        [  --with-svga             Build SVGALib support ],,)
    if test x$with_svga = xyes; then
        AC_CHECK_LIB(vga,vga_getmousetype,
            SVGA_LIBS="-lvga",
            [AC_MSG_ERROR([vga not found, SVGALib cannot be built])
                svga=false],
        )
        AC_CHECK_LIB(vgagl,gl_getcontext,
            SVGA_LIBS="${SVGA_LIBS} -lvgagl",
            [AC_MSG_ERROR([vgagl not found, SVGALib cannot be built])
                svga=false],
            -lvga)
        AC_DEFINE(SVGA,,[Define if you want an SVGALib build])
        TARGETS=d2x-svga
        D2X_ARCH_SUBDIRS="svgalib ${D2X_ARCH_SUBDIRS}"
    fi
    AC_SUBST(SVGA_LIBS)
    AM_CONDITIONAL(USE_SVGALIB, test x$with_svga = xyes)

    if test x$MINGW32 = xyes; then
        D2X_ARCH_SUBDIRS="win32 ${D2X_ARCH_SUBDIRS}"
    else
        CFLAGS="-I \$(top_srcdir)/arch/linux/include $CFLAGS"
        D2X_ARCH_SUBDIRS="linux ${D2X_ARCH_SUBDIRS}"
    fi
fi

if test x$MINGW32 = xyes; then
    dnl Build with Oculus Rift support
    AC_ARG_WITH(oculusrift,
        [  --with-oculusrift       Build Oculus Rift VR support ],,)
    if test x$with_oculusrift = xyes; then
        AC_DEFINE(OCULUS_RIFT,1,[Define if you want an Oculus Rift build])
    fi
fi

dnl Check for network
AC_ARG_ENABLE(network,
    [  --disable-network       Do not build network/serial support ],,)
if test x$enable_network != xno; then
    case $host_os in
        cygwin* | mingw* | msdos* | linux*)
            enable_native_ipx="yes";
           ;;
       *)
            AC_MSG_WARN([d2x does not support native ipx on this host.  disabling native ipx])
            enable_native_ipx="no";
            ;;
    esac
	case $host_os in
		cygwin* | mingw* | msdos*)
			enable_kalinix="no";
			;;
	esac
    case $host_os in
        *solaris*)
            AC_CHECK_LIB(socket, socket, LIBS="${LIBS} -lsocket",
	        [AC_MSG_ERROR(socket lib required for net support on solaris not found)])
            AC_CHECK_LIB(nsl, inet_addr, LIBS="${LIBS} -lnsl",
	        [AC_MSG_ERROR(nsl lib required for net support on solaris not found)])		
           ;;
    esac
fi
AC_ARG_ENABLE(kalinix,
    [  --disable-kalinix       Disable the KaliNix driver (unix-only)],,)
if test x$enable_network != xno; then
    if test x$enable_native_ipx != xno; then
        D2X_FEATURES="ipx ${D2X_FEATURES}"
        AC_DEFINE(NATIVE_IPX,,[Define to use the IPX support of the OS])
    fi
    if test x$enable_kalinix != xno; then
        D2X_FEATURES="kalinix ${D2X_FEATURES}"
        AC_DEFINE(KALINIX,,[Define to enable use of the KaliNix driver])
    fi
    AC_DEFINE(NETWORK,,[Define if you want a network build])
    D2X_FEATURES="network ${D2X_FEATURES}"
fi
AM_CONDITIONAL(USE_NETWORK, test x$enable_network != xno)
AM_CONDITIONAL(USE_NATIVE_IPX, test x$enable_native_ipx != xno)
AM_CONDITIONAL(USE_KALINIX, test x$enable_kalinix != xno)

dnl Check for assembler
AC_ARG_ENABLE(assembler,
    [  --disable-assembler     Do not use assembler optimization ],,)
if test x$enable_assembler == xyes; then
    if test x$NASM = xno; then
        AC_MSG_WARN([nasm not found. disabling assembler])
        enable_assembler="no"
    else
        NASMFLAGS="-i\$(srcdir)/ $NASMFLAGS"
        case $host_os in
            cygwin* | mingw*)
                NASMFLAGS="-f win32 $NASMFLAGS"
                ;;
            msdos*)
                NASMFLAGS="-f coff $NASMFLAGS"
                ;;
			linux* | freebsd*)
				NASMFLAGS="-f elf -d__ELF__ $NASMFLAGS"
                ;;
            *)
                AC_MSG_WARN([nasm not supported for this OS. disabling assembler])
                enable_assembler="no"
                ;;
        esac
    fi
fi
if test x$enable_assembler != xyes; then
    AC_DEFINE(NO_ASM,,[Define if you want an assembler free build])
    D2X_FEATURES="no_asm ${D2X_FEATURES}"
fi
AM_CONDITIONAL(USE_ASM, test x$enable_assembler = xyes)

AH_BOTTOM([
        /* General defines */
#ifndef PACKAGE_STRING
#define PACKAGE_STRING PACKAGE " " VERSION
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
        ])

TARGETS="$TARGETS$ac_cv_exeext"

AM_PROG_AS

AC_SUBST(D2X_SUBDIRS)

AC_SUBST(D2X_ARCH_SUBDIRS)

AC_SUBST(D2X_MAIN_SUBDIRS)

AC_SUBST(TARGETS)

AC_SUBST(NASMFLAGS)

AC_SUBST(CFLAGS)

AC_SUBST(CXXFLAGS)

AC_OUTPUT(
    Makefile
    2d/Makefile
    3d/Makefile
    ai/Makefile
    audio/Makefile
    cockpit/Makefile
    console/Makefile
    effects/Makefile
    gameio/Makefile
    gamemodes/Makefile
    input/Makefile
    io/Makefile
    lighting/Makefile
    libmve/Makefile
    main/Makefile
    maths/Makefile
    menus/Makefile
    misc/Makefile
    models/Makefile
    network/Makefile
    objects/Makefile
    ogl/Makefile
    physics/Makefile
    render/Makefile
    weapons/Makefile
)

AC_MSG_RESULT([
        d2x has been configured successfully.

        Platform(s): $D2X_ARCH_SUBDIRS
        Features   : $D2X_FEATURES $D2X_SUBDIRS $D2X_MAIN_SUBDIRS

        Shared game data directory (unix only): $sharepath
        ])
