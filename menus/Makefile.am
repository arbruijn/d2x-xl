noinst_LIBRARIES = libmenus.a

INCLUDES = -I $(top_srcdir)/include -I $(top_srcdir)/input/include -I $(top_srcdir)/network/linux/include -I $(top_srcdir)/audio/linux/include -I /usr/local/include/SDL

if MACDATA
TRANSPARENCY_COLOR = 0
else
TRANSPARENCY_COLOR = 255
endif

libmenus_a_SOURCES = \
	mainmenu.cpp  automapmenu.cpp cameramenu.cpp cockpitmenu.cpp configmenu.cpp coronamenu.cpp detailsmenu.cpp effectsmenu.cpp entropymenu.cpp \
	fileselector.cpp gameplaymenu.cpp inputdevicemenu.cpp lightmenu.cpp lightningmenu.cpp listbox.cpp menuitem.cpp messagebox.cpp miscmenu.cpp \
	monsterballmenu.cpp moviemenu.cpp menu.cpp menubackground.cpp msgbox.cpp netgamebrowser.cpp netgamehelp.cpp netgameinfo.cpp \
	netmenu.cpp netplayerbrowser.cpp newgamemenu.cpp performancemenu.cpp physicsmenu.cpp powerupmenu.cpp rendermenu.cpp screenresmenu.cpp \
	shadowmenu.cpp shiprendermenu.cpp smokemenu.cpp soundmenu.cpp
	