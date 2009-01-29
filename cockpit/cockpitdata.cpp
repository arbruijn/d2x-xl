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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "descent.h"
#include "screens.h"
#include "error.h"
#include "newdemo.h"
#include "gamefont.h"
#include "text.h"
#include "network.h"
#include "network_lib.h"
#include "render.h"
#include "transprender.h"
#include "gamepal.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
#include "playsave.h"
#include "cockpit.h"
#include "hud_defs.h"
#include "statusbar.h"
#include "slowmotion.h"
#include "automap.h"
#include "gr.h"

#define SHOW_PLAYER_IP		0

void DrawGuidedCrosshair (void);

#if 0
CCanvas *Canv_LeftEnergyGauge;
CCanvas *Canv_AfterburnerGauge;
CCanvas *Canv_RightEnergyGauge;
CCanvas *numericalGaugeCanv;
#endif

//store delta x values from left of box
tSpan weaponWindowLeft [WEAPON_WINDOW_SIZE] = {		//first tSpan 67, 151
	 {8, 51},
	 {6, 53},
	 {5, 54},
	 {4-1, 53+2},
	 {4-1, 53+3},
	 {4-1, 53+3},
	 {4-2, 53+3},
	 {4-2, 53+3},
	 {3-1, 53+3},
	 {3-1, 53+3},
	 {3-1, 53+3},
	 {3-1, 53+3},
	 {3-1, 53+3},
	 {3-1, 53+3},
	 {3-1, 53+3},
	 {3-2, 53+3},
	 {2-1, 53+3},
	 {2-1, 53+3},
	 {2-1, 53+3},
	 {2-1, 53+3},
	 {2-1, 53+3},
	 {2-1, 53+3},
	 {2-1, 53+3},
	 {2-1, 53+3},
	 {1-1, 53+3},
	 {1-1, 53+2},
	 {1-1, 53+2},
	 {1-1, 53+2},
	 {1-1, 53+2},
	 {1-1, 53+2},
	 {1-1, 53+2},
	 {1-1, 53+2},
	 {0, 53+2},
	 {0, 53+2},
	 {0, 53+2},
	 {0, 53+2},
	 {0, 52+3},
	 {1-1, 52+2},
	 {2-2, 51+3},
	 {3-2, 51+2},
	 {4-2, 50+2},
	 {5-2, 50},
	 {5-2+2, 50-2}
	};


//store delta x values from left of box
tSpan weaponWindowRight [WEAPON_WINDOW_SIZE] = {		//first tSpan 207, 154
	 {208-202, 255-202},
	 {206-202, 257-202},
	 {205-202, 258-202},
	 {204-202, 259-202},
	 {203-202, 260-202},
	 {203-202, 260-202},
	 {203-202, 260-202},
	 {203-202, 260-202},
	 {203-202, 260-202},
	 {203-202, 261-202},
	 {203-202, 261-202},
	 {203-202, 261-202},
	 {203-202, 261-202},
	 {203-202, 261-202},
	 {203-202, 261-202},
	 {203-202, 261-202},
	 {203-202, 261-202},
	 {203-202, 261-202},
	 {203-202, 262-202},
	 {203-202, 262-202},
	 {203-202, 262-202},
	 {203-202, 262-202},
	 {203-202, 262-202},
	 {203-202, 262-202},
	 {203-202, 262-202},
	 {203-202, 262-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {204-202, 263-202},
	 {205-202, 263-202},
	 {206-202, 262-202},
	 {207-202, 261-202},
	 {208-202, 260-202},
	 {211-202, 255-202}
	};

//store delta x values from left of box
tSpan weaponWindowLeftHires [WEAPON_WINDOW_SIZE_HIRES] = {		//first tSpan 67, 154
 {20, 110},
 {18, 113},
 {16, 114},
 {15, 116},
 {14, 117},
 {13, 118},
 {12, 119},
 {11, 119},
 {10, 120},
 {10, 120},
 {9, 121},
 {8, 121},
 {8, 121},
 {8, 122},
 {7, 122},
 {7, 122},
 {7, 122},
 {7, 122},
 {7, 122},
 {6, 122},
 {6, 122},
 {6, 122},
 {6, 122},
 {6, 122},
 {6, 122},
 {6, 122},
 {6, 122},
 {6, 122},
 {6, 122},
 {5, 122},
 {5, 122},
 {5, 122},
 {5, 122},
 {5, 121},
 {5, 121},
 {5, 121},
 {5, 121},
 {5, 121},
 {5, 121},
 {4, 121},
 {4, 121},
 {4, 121},
 {4, 121},
 {4, 121},
 {4, 121},
 {4, 121},
 {4, 121},
 {4, 121},
 {4, 121},
 {3, 121},
 {3, 121},
 {3, 120},
 {3, 120},
 {3, 120},
 {3, 120},
 {3, 120},
 {3, 120},
 {3, 120},
 {3, 120},
 {3, 120},
 {2, 120},
 {2, 120},
 {2, 120},
 {2, 120},
 {2, 120},
 {2, 120},
 {2, 120},
 {2, 120},
 {2, 120},
 {2, 120},
 {2, 120},
 {2, 120},
 {1, 120},
 {1, 120},
 {1, 119},
 {1, 119},
 {1, 119},
 {1, 119},
 {1, 119},
 {1, 119},
 {1, 119},
 {1, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 118},
 {0, 118},
 {0, 118},
 {0, 117},
 {0, 117},
 {0, 117},
 {1, 116},
 {1, 116},
 {2, 115},
 {2, 114},
 {3, 113},
 {4, 112},
 {5, 111},
 {5, 110},
 {7, 109},
 {9, 107},
 {10, 105},
 {12, 102},
};


//store delta x values from left of box
tSpan weaponWindowRightHires [WEAPON_WINDOW_SIZE_HIRES] = {		//first tSpan 207, 154
 {12, 105},
 {9, 107},
 {8, 109},
 {6, 110},
 {5, 111},
 {4, 112},
 {3, 113},
 {3, 114},
 {2, 115},
 {2, 115},
 {1, 116},
 {1, 117},
 {1, 117},
 {0, 117},
 {0, 118},
 {0, 118},
 {0, 118},
 {0, 118},
 {0, 118},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 119},
 {0, 120},
 {0, 120},
 {0, 120},
 {0, 120},
 {1, 120},
 {1, 120},
 {1, 120},
 {1, 120},
 {1, 120},
 {1, 120},
 {1, 121},
 {1, 121},
 {1, 121},
 {1, 121},
 {1, 121},
 {1, 121},
 {1, 121},
 {1, 121},
 {1, 121},
 {1, 121},
 {1, 122},
 {1, 122},
 {2, 122},
 {2, 122},
 {2, 122},
 {2, 122},
 {2, 122},
 {2, 122},
 {2, 122},
 {2, 122},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 123},
 {2, 124},
 {2, 124},
 {3, 124},
 {3, 124},
 {3, 124},
 {3, 124},
 {3, 124},
 {3, 124},
 {3, 124},
 {3, 125},
 {3, 125},
 {3, 125},
 {3, 125},
 {3, 125},
 {3, 125},
 {3, 125},
 {3, 125},
 {4, 125},
 {4, 125},
 {4, 125},
 {5, 125},
 {5, 125},
 {5, 125},
 {6, 125},
 {6, 124},
 {7, 123},
 {8, 123},
 {9, 122},
 {10, 121},
 {11, 120},
 {12, 120},
 {13, 118},
 {15, 117},
 {18, 115},
 {20, 114},
};


tGaugeBox hudWindowAreas [8] = {
// primary left/right low res
 {PRIMARY_W_BOX_LEFT_L, PRIMARY_W_BOX_TOP_L, PRIMARY_W_BOX_RIGHT_L, PRIMARY_W_BOX_BOT_L, weaponWindowLeft},
 {SECONDARY_W_BOX_LEFT_L, SECONDARY_W_BOX_TOP_L, SECONDARY_W_BOX_RIGHT_L, SECONDARY_W_BOX_BOT_L, weaponWindowRight},
//sb left/right low res
 {SB_PRIMARY_W_BOX_LEFT_L, SB_PRIMARY_W_BOX_TOP_L, SB_PRIMARY_W_BOX_RIGHT_L, SB_PRIMARY_W_BOX_BOT_L, NULL},
 {SB_SECONDARY_W_BOX_LEFT_L, SB_SECONDARY_W_BOX_TOP_L, SB_SECONDARY_W_BOX_RIGHT_L, SB_SECONDARY_W_BOX_BOT_L, NULL},
// primary left/right hires
 {PRIMARY_W_BOX_LEFT_H, PRIMARY_W_BOX_TOP_H, PRIMARY_W_BOX_RIGHT_H, PRIMARY_W_BOX_BOT_H, weaponWindowLeftHires},
 {SECONDARY_W_BOX_LEFT_H, SECONDARY_W_BOX_TOP_H, SECONDARY_W_BOX_RIGHT_H, SECONDARY_W_BOX_BOT_H, weaponWindowRightHires},
// sb left/right hires
 {SB_PRIMARY_W_BOX_LEFT_H, SB_PRIMARY_W_BOX_TOP_H, SB_PRIMARY_W_BOX_RIGHT_H, SB_PRIMARY_W_BOX_BOT_H, NULL},
 {SB_SECONDARY_W_BOX_LEFT_H, SB_SECONDARY_W_BOX_TOP_H, SB_SECONDARY_W_BOX_RIGHT_H, SB_SECONDARY_W_BOX_BOT_H, NULL},
	};

//	-----------------------------------------------------------------------------

tCanvasColor gaugeFadeColors [2][4] = {
	{{-1, 1, {0, 0, 0, 0}},
	 {-1, 1, {0, 0, 0, 255}},
	 {-1, 1, {0, 0, 0, 255}},
	 {-1, 1, {0, 0, 0, 0}}},
	{{-1, 1, {0, 0, 0, 255}},
	 {-1, 1, {0, 0, 0, 0}},
	 {-1, 1, {0, 0, 0, 0}},
	 {-1, 1, {0, 0, 0, 255}}}
	};

//	-----------------------------------------------------------------------------

int SW_drawn [2], SW_x [2], SW_y [2], SW_w [2], SW_h [2];

int nDbgGauge = -1;

//	-----------------------------------------------------------------------------
