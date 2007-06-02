/* $Id: menu.h,v 1.7 2003/10/10 09:36:35 btb Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _MENU_H
#define _MENU_H

// returns number of item chosen
int CallMenu ();
void ConfigMenu ();
void RenderOptionsMenu ();
void GameplayOptionsMenu ();
int QuitSaveLoadMenu (void);
int SelectAndLoadMission (int bMulti, int *bAnarchyOnly);

#ifndef _DEBUG  // read only from hog file
#define MENU_PCX_MAC_SHARE ("\x01menub.pcx")
#define MENU_PCX_SHAREWARE ("\x01menud.pcx")
#define MENU_PCX_OEM (gameStates.menus.bHires?"\x01menuob.pcx":"\x01menuo.pcx")
#define MENU_PCX_FULL (gameStates.menus.bHires?"\x01menub.pcx":"\x01menu.pcx")
#else
#define MENU_PCX_MAC_SHARE ("menub.pcx")
#define MENU_PCX_SHAREWARE ("menud.pcx")
#define MENU_PCX_OEM (gameStates.menus.bHires?"menuob.pcx":"menuo.pcx")
#define MENU_PCX_FULL (gameStates.menus.bHires?"menub.pcx":"menu.pcx")
#endif

// name of background bitmap
#define Menu_pcx_name \
			(CFExist(MENU_PCX_FULL,gameFolders.szDataDir,0)?MENU_PCX_FULL:\
			(CFExist(MENU_PCX_OEM,gameFolders.szDataDir,0)?MENU_PCX_OEM:\
			CFExist(MENU_PCX_SHAREWARE,gameFolders.szDataDir,0)?MENU_PCX_SHAREWARE:MENU_PCX_MAC_SHARE))

void InitDetailLevels(int detailLevel);

int SwitchDisplayMode (int dir);

extern char *menu_difficulty_text[];
extern int Max_debrisObjects;
extern int MissileView_enabled;
extern int EscortView_enabled;
extern int Cockpit_rearView;

#define EXPMODE_DEFAULTS 0

#define NMCLAMP(_v,_min,_max)	((_v) < (_min) ? (_min) : (_v) > (_max) ? (_max) : (_v))
#define NMBOOL(_v) ((_v) != 0)

#define	ADD_CHECK(_n,_t,_v,_k,_h) \
			m [_n].nType = NM_TYPE_CHECK, m [_n].text = _t, m [_n].value = NMBOOL(_v), m [_n].key=_k, m [_n].szHelp = _h
#define	ADD_RADIO(_n,_t,_v,_k,_g,_h) \
			m [_n].nType = NM_TYPE_RADIO, m [_n].text = _t, m [_n].value = _v, m [_n].key=_k, m [_n].group=_g, m [_n].szHelp = _h
#define	ADD_MENU(_n,_t,_k,_h) \
			m [_n].nType = NM_TYPE_MENU, m [_n].text = _t, m [_n].key = _k, m [_n].szHelp = _h
#define	ADD_TEXT(_n,_t,_k)  \
			m [_n].nType = NM_TYPE_TEXT, m [_n].text = _t, m [_n].key = _k
#define	ADD_SLIDER(_n,_t,_v,_min,_max,_k,_h) \
			m [_n].nType = NM_TYPE_SLIDER, m [_n].text = _t, m [_n].value = NMCLAMP(_v,_min,_max), m [_n].minValue = _min, m [_n].maxValue = _max, m [_n].key = _k, m [_n].szHelp = _h
#define	ADD_INPUT(_n,_t,_l,_h) \
			m [_n].nType = NM_TYPE_INPUT, m [_n].text = _t, m [_n].text_len = _l, m [_n].szHelp = _h
#define	ADD_GAUGE(_n,_t,_v,_max) \
			m [_n].nType = NM_TYPE_GAUGE, m [_n].text = _t, m [_n].text_len = *_t ? (int) strlen (_t) : 20, m [_n].value = NMCLAMP(_v,0,_max), m [_n].maxValue = _max

#define GET_VAL(_v,_n)	if ((_n) >= 0) (_v) = m [_n].value

#define MENU_KEY(_k,_d)	((_k) < 0) ? (_d) : ((_k) == 0) ? 0 : gameStates.app.bEnglish ? toupper (KeyToASCII (_k)) : (_k)

#define PROGRESS_INCR		20
#define PROGRESS_STEPS(_i)	(((_i) + PROGRESS_INCR - 1) / PROGRESS_INCR)

//------------------------------------------------------------------------------

#define INIT_PROGRESS_LOOP(_i,_j,_max) \
	if ((_i) < 0) { \
		(_i) = 0; (_j) = (_max);} \
	else { \
		(_j) = (_i) + PROGRESS_INCR; \
		if ((_j) > (_max)) \
			(_j) = (_max); \
		}


typedef struct {
	int	VGA_mode;
	short	w,h;
	short	render_method;
	short	flags;
	char	isWideScreen;
	char	isAvailable;
} dmi;

#define NUM_DISPLAY_MODES	21

extern dmi displayModeInfo [NUM_DISPLAY_MODES + 1];

int stoip (char *szServerIpAddr, unsigned char *pIpAddr);
int stoport (char *szPort, int *pPort, int *pSign);
int SetCustomDisplayMode (int x, int y);

#endif /* _MENU_H */
