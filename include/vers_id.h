#ifndef _VERS_ID_H
#define _VERS_ID_H

//#ifndef HAVE_CONFIG_H
#	ifndef VERSION
#		define VERSION		"1.18.74-ar10-png4"
#	endif
//#endif

#define D2X_NAME		"D2X-XL "

#ifndef D2X_MAJOR
#	define D2X_MAJOR	1
#endif
#ifndef D2X_MINOR
#	define D2X_MINOR	18
#endif
#ifndef D2X_MICRO
#	define D2X_MICRO	74
#endif
#ifndef D2X_VARIANT
#	define D2X_VARIANT	"ar10"
#endif

#define VERSION_TYPE		"Full Version"
#define DESCENT_VERSION D2X_NAME VERSION
#define D2X_IVER			(D2X_MAJOR * 100000 + D2X_MINOR * 1000 + D2X_MICRO)

#endif //_VERS_ID_H
