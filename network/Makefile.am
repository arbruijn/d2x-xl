EXTRA_SUBDIRS = include

noinst_LIBRARIES = libnetwork.a

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/main -I$(srcdir)/include -I$(top_srcdir)/3d

if USE_NETWORK
NETWORK_SRCS = ./linux/ipx_mcast4.cpp ./linux/ipx_udp.cpp ./linux/linuxnet.cpp
if USE_NATIVE_IPX
IPX_SRCS = ./linux/ipx_bsd.cpp
endif
if USE_KALINIX
KALI_SRCS = ./linux/ipx_kali.cpp ./linux/ukali.cpp
endif
endif

if USE_LINUX_JOY
JOYSTICK_SRCS = joystick.cpp joydefs.cpp
endif

network_a_SOURCES = ${NETWORK_SRCS} ${IPX_SRCS} ${KALI_SRCS} ${JOYSTICK_SRCS} \
	autodl.cpp multi.cpp multibot.cpp multimsg.cpp netmenu.cpp netmisc.cpp \
	network.cpp network_init.cpp .cpp network_join.cpp network_lib.cpp \
	network_phandler.cpp network_process.cpp network_read.cpp network_send.cpp \
	network_sync.cpp tracker.cpp

