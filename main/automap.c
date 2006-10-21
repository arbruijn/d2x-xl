/* $Id: automap.c,v 1.16 2003/11/15 00:37:48 btb Exp $ */
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

/*
 *
 * Routines for displaying the auto-map.
 *
 * Old Log:
 * Revision 1.8  1995/10/31  10:24:54  allender
 * shareware stuff
 *
 * Revision 1.7  1995/10/21  16:18:20  allender
 * blit pcx background directly to Page canvas instead of creating
 * seperate bitmap for it -- hope to solve VM bug on some macs
 *
 * Revision 1.6  1995/10/20  00:49:16  allender
 * added redbook check during automap
 *
 * Revision 1.5  1995/09/13  08:44:07  allender
 * Dave Denhart's changes to speed up the automap
 *
 * Revision 1.4  1995/08/18  15:46:00  allender
 * put text all on upper bar -- and fixed background since
 * changing xparency color
 *
 * Revision 1.3  1995/08/03  15:15:18  allender
 * fixed edge hashing problem causing automap to crash
 *
 * Revision 1.2  1995/07/12  12:49:27  allender
 * works in 640x480 mode
 *
 * Revision 1.1  1995/05/16  15:22:59  allender
 * Initial revision
 *
 * Revision 2.2  1995/03/21  14:41:26  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.1  1995/03/20  18:16:06  john
 * Added code to not store the normals in the tSegment structure.
 *
 * Revision 2.0  1995/02/27  11:32:55  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.117  1995/02/22  14:11:31  allender
 * remove anonymous unions from tObject structure
 *
 * Revision 1.116  1995/02/22  13:24:39  john
 * Removed the vecmat anonymous unions.
 *
 * Revision 1.115  1995/02/09  14:57:02  john
 * Reduced mem usage. Made automap slide farther.
 *
 * Revision 1.114  1995/02/07  20:40:44  rob
 * Allow for anarchy automap of player pos by option.
 *
 * Revision 1.113  1995/02/07  15:45:33  john
 * Made automap memory be static.
 *
 * Revision 1.112  1995/02/02  12:24:00  adam
 * played with automap labels
 *
 * Revision 1.111  1995/02/02  01:52:52  john
 * Made the automap use small font.
 *
 * Revision 1.110  1995/02/02  01:34:34  john
 * Made Reset in automap not change segmentlimit.
 *
 * Revision 1.109  1995/02/02  01:23:11  john
 * Finalized the new automap partial viewer.
 *
 * Revision 1.108  1995/02/02  00:49:45  mike
 * new automap tSegment-depth functionality.
 *
 * Revision 1.107  1995/02/02  00:23:04  john
 * Half of the code for new connected distance stuff in automap.
 *
 * Revision 1.106  1995/02/01  22:54:00  john
 * Made colored doors not fade in automap. Made default
 * viewing area be maxxed.
 *
 * Revision 1.105  1995/02/01  13:16:13  john
 * Added great grates.
 *
 * Revision 1.104  1995/01/31  12:47:06  john
 * Made Alt+F only work with cheats enabled.
 *
 * Revision 1.103  1995/01/31  12:41:23  john
 * Working with new controls.
 *
 * Revision 1.102  1995/01/31  12:04:19  john
 * Version 2 of new key control.
 *
 * Revision 1.101  1995/01/31  11:32:00  john
 * First version of new automap system.
 *
 * Revision 1.100  1995/01/28  16:55:48  john
 * Made keys draw in automap in the segments that you have
 * visited.
 *
 * Revision 1.99  1995/01/28  14:44:51  john
 * Made hostage doors show up on automap.
 *
 * Revision 1.98  1995/01/22  17:03:49  rob
 * Fixed problem drawing playerships in automap coop/team mode
 *
 * Revision 1.97  1995/01/21  17:23:11  john
 * Limited S movement in map. Made map bitmap load from disk
 * and then freed it.
 *
 * Revision 1.96  1995/01/19  18:55:38  john
 * Don't draw players in automap if not obj_player.
 *
 * Revision 1.95  1995/01/19  18:48:13  john
 * Made player colors better in automap.
 *
 * Revision 1.94  1995/01/19  17:34:52  rob
 * Added team colorizations in automap.
 *
 * Revision 1.93  1995/01/19  17:15:36  rob
 * Trying to add player ships into map for coop and team mode.
 *
 * Revision 1.92  1995/01/19  17:11:09  john
 * Added code for Rob to draw Multiplayer ships in automap.
 *
 * Revision 1.91  1995/01/12  13:35:20  john
 * Fixed bug with Segment 0 not getting displayed
 * in automap if you have EDITOR compiled in.
 *
 * Revision 1.90  1995/01/08  16:17:14  john
 * Added code to draw player's up vector while in automap.
 *
 * Revision 1.89  1995/01/08  16:09:41  john
 * Fixed problems with grate.
 *
 * Revision 1.88  1994/12/14  22:54:17  john
 * Fixed bug that didn't show hostages in automap.
 *
 * Revision 1.87  1994/12/09  00:41:03  mike
 * fix hang in automap print screen
 *
 * Revision 1.86  1994/12/05  23:37:15  matt
 * Took out calls to warning () function
 *
 * Revision 1.85  1994/12/03  22:35:28  yuan
 * Localization 412
 *
 * Revision 1.84  1994/12/02  15:05:45  matt
 * Added new "official" cheats
 *
 * Revision 1.83  1994/11/30  12:10:49  adam
 * added support for PCX titles/brief screens
 *
 * Revision 1.82  1994/11/27  23:15:12  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.81  1994/11/27  15:35:52  matt
 * Enable screen shots even when debugging is turned off
 *
 * Revision 1.80  1994/11/26  22:51:43  matt
 * Removed editor-only fields from tSegment structure when editor is compiled
 * out, and padded tSegment structure to even multiple of 4 bytes.
 *
 * Revision 1.79  1994/11/26  16:22:48  matt
 * Reduced leaveTime
 *
 * Revision 1.78  1994/11/23  22:00:10  mike
 * show level number.
 *
 * Revision 1.77  1994/11/21  11:40:33  rob
 * Tweaked the game-loop for automap in multiplayer games.
 *
 * Revision 1.76  1994/11/18  16:42:06  adam
 * removed a font
 *
 * Revision 1.75  1994/11/17  13:06:48  adam
 * changed font
 *
 * Revision 1.74  1994/11/14  20:47:17  john
 * Attempted to strip out all the code in the game
 * directory that uses any ui code.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef OGL
#include "ogl_init.h"
#endif

#include "pa_enabl.h"                   //$$POLY_ACC
#include "error.h"
#include "3d.h"
#include "inferno.h"
#include "u_mem.h"
#include "render.h"
#include "object.h"
#include "vclip.h"
#include "game.h"
#include "mono.h"
#include "polyobj.h"
#include "sounds.h"
#include "player.h"
#include "bm.h"
#include "key.h"
#include "newmenu.h"
#include "menu.h"
#include "screens.h"
#include "textures.h"
#include "mouse.h"
#include "timer.h"
#include "segpoint.h"
#include "joy.h"
#include "iff.h"
#include "pcx.h"
#include "palette.h"
#include "wall.h"
#include "gameseq.h"
#include "gamefont.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "kconfig.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "endlevel.h"
#include "text.h"
#include "gauges.h"
#include "songs.h"
#include "powerup.h"
#include "switch.h"
#include "automap.h"
#include "cntrlcen.h"
#include "hudmsg.h"
#include "inferno.h"
#include "globvars.h"
#include "gameseg.h"
#include "input.h"

#if defined (POLY_ACC)
#include "poly_acc.h"
#endif

#ifdef OGL
#	define AUTOMAP_DIRECT_RENDER	1
#endif

#define EF_USED     1   // This edge is used
#define EF_DEFINING 2   // A structure defining edge that should always draw.
#define EF_FRONTIER 4   // An edge between the known and the unknown.
#define EF_SECRET   8   // An edge that is part of a secret wall.
#define EF_GRATE    16  // A grate... draw it all the time.
#define EF_NO_FADE  32  // An edge that doesn't fade with distance
#define EF_TOO_FAR  64  // An edge that is too far away

void ModexPrintF (int x,int y, char *s, grs_font *font, unsigned int color);

typedef struct Edge_info {
	short verts[2];     // 4 bytes
	ubyte sides[4];     // 4 bytes
	short nSegment[4];    // 8 bytes  // This might not need to be stored... If you can access the normals of a tSide.
	ubyte flags;        // 1 bytes  // See the EF_??? defines above.
	unsigned int color; // 4 bytes
	ubyte num_faces;    // 1 bytes  // 19 bytes...
} Edge_info;

// OLD BUT GOOD -- #define MAX_EDGES_FROM_VERTS (v) ((v*5)/2)
// THE following was determined by John by loading levels 1-14 and recording
// numbers on 10/26/94.
//#define MAX_EDGES_FROM_VERTS (v) (((v)*21)/10)
#define MAX_EDGES_FROM_VERTS(v)   ((v)*4)
//#define MAX_EDGES (MAX_EDGES_FROM_VERTS (MAX_VERTICES))

#define MAX_EDGES 65536 // Determined by loading all the levels by John & Mike, Feb 9, 1995

#define K_WALL_NORMAL_COLOR     RGBA_PAL2 (29, 29, 29)
#define K_WALL_DOOR_COLOR       RGBA_PAL2 (5, 27, 5)
#define K_WALL_DOOR_BLUE        RGBA_PAL2 (0, 0, 31)
#define K_WALL_DOOR_GOLD        RGBA_PAL2 (31, 31, 0)
#define K_WALL_DOOR_RED         RGBA_PAL2 (31, 0, 0)
#define K_WALL_REVEALED_COLOR   RGBA_PAL2 (0, 0, 25) //what you see when you have the full map powerup
#define K_HOSTAGE_COLOR         RGBA_PAL2 (0, 31, 0)
#define K_MONSTERBALL_COLOR     RGBA_PAL2 (31, 23, 0)
#define K_FONT_COLOR_20         RGBA_PAL2 (20, 20, 20)
#define K_GREEN_31              RGBA_PAL2 (0, 31, 0)

typedef struct amWallColors {
	unsigned int	nNormal;
	unsigned int	nDoor;
	unsigned int	nDoorBlue;
	unsigned int	nDoorGold;
	unsigned int	nDoorRed;
	unsigned int	nRevealed;
} amWallColors;

typedef struct amColors {
	amWallColors	walls;
	unsigned int	nHostage;
	unsigned int	nMonsterball;
	unsigned int	nWhite;
	unsigned int	nMedGreen;
	unsigned int	nLgtBlue;
	unsigned int	nLgtRed;
	unsigned int	nDkGray;
} amColors;

amColors automapColors;

void init_automap_colors (void)
{
automapColors.walls.nNormal = K_WALL_NORMAL_COLOR;
automapColors.walls.nDoor = K_WALL_DOOR_COLOR;
automapColors.walls.nDoorBlue = K_WALL_DOOR_BLUE;
automapColors.walls.nDoorGold = K_WALL_DOOR_GOLD;
automapColors.walls.nDoorRed = K_WALL_DOOR_RED;
automapColors.walls.nRevealed = K_WALL_REVEALED_COLOR;
automapColors.nHostage = K_HOSTAGE_COLOR;
automapColors.nMonsterball = K_MONSTERBALL_COLOR;
automapColors.nDkGray = RGBA_PAL2 (20,20,20);
automapColors.nMedGreen = RGBA_PAL2 (0,31,0);
automapColors.nWhite = RGBA_PAL2 (63,63,63);
automapColors.nLgtBlue = RGBA_PAL2 (0,0,48);
automapColors.nLgtRed = RGBA_PAL2 (48,0,0);
}

// Segment visited list
ubyte bAutomapVisited[MAX_SEGMENTS];
ubyte bRadarVisited[MAX_SEGMENTS];

// Edge list variables
static int Num_edges=0;
static int Max_edges;		//set each frame
static int Highest_edge_index = -1;
static Edge_info Edges[MAX_EDGES];
static int DrawingListBright[MAX_EDGES];

#define EDGE_IDX(_edgeP)	((int) ((_edgeP) - Edges))

//static short DrawingListBright[MAX_EDGES];
//static short Edge_used_list[MAX_EDGES];				//which entries in edge_list have been used

// Map movement defines
#define PITCH_DEFAULT 9000
#define ZOOM_DEFAULT i2f (20*10)
#define ZOOM_MIN_VALUE i2f (20*5)
#define ZOOM_MAX_VALUE i2f (20*100)

#define SLIDE_SPEED 				 (350)
#define ZOOM_SPEED_FACTOR		500	// (1500)
#define ROT_SPEED_DIVISOR		 (115000)

#if AUTOMAP_DIRECT_RENDER
//static grs_canvas	automap_canvas;
static grs_bitmap bmAutomapBackground;
#else
// Screen anvas variables
static int current_page=0;
#ifdef WINDOWS
static dd_grs_canvas ddPages[2];
static dd_grs_canvas ddDrawingPages[2];

#define ddPage ddPages[0]
#define ddDrawingPage ddDrawingPages[0]

#endif

	static grs_canvas Pages[2];
	static grs_canvas DrawingPages[2];
#endif /* AUTOMAP_DIRECT_RENDER */

#define Page Pages[0]
#define DrawingPage DrawingPages[0]

typedef struct tAutomapData {
	int			bCheat;
	fix			nViewDist;
	fix			nMaxDist;
	fix			nZoom;
	vmsVector	viewTarget;
	vmsMatrix	viewMatrix;
} tAutomapData;

// Rendering variables
static tAutomapData	amData = {0, 0, F1_0 * 20 * 100, 0x9000, {0,0,0}, {{0,0,0},{0,0,0},{0,0,0}}};

//	Function Prototypes
void adjust_segment_limit (int SegmentLimit, ubyte *pVisited);
void DrawAllEdges (void);
void AutomapBuildEdgeList (void);

#define	MAX_DROP_MULTI		2
#define	MAX_DROP_SINGLE	9

# define AutomapDrawLine G3DrawLine


// -------------------------------------------------------------

void DrawMarkerNumber (int num)
 {
  int i;
  g3s_point basePoint,fromPoint,toPoint;

  float ArrayX[10][20]={ {-.25, 0.0, 0.0, 0.0, -1.0, 1.0},
                         {-1.0, 1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0, 1.0},
                         {-1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 0.0, 1.0},
                         {-1.0, -1.0, -1.0, 1.0, 1.0, 1.0},
                         {-1.0, 1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0, 1.0},
                         {-1.0, 1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0, 1.0},
                         {-1.0, 1.0, 1.0, 1.0},
                         {-1.0, 1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0, 1.0},
                         {-1.0, 1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0}

                       };
  float ArrayY[10][20]={ {.75, 1.0, 1.0, -1.0, -1.0, -1.0},
                         {1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, -1.0, -1.0},
                         {1.0, 1.0, 1.0, -1.0, -1.0, -1.0, 0.0, 0.0},
                         {1.0, 0.0, 0.0, 0.0, 1.0, -1.0},
                         {1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, -1.0, -1.0},
                         {1.0, 1.0, 1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 0.0},
                         {1.0, 1.0, 1.0, -1.0},
                         {1.0, 1.0, 1.0, -1.0, -1.0, -1.0, -1.0, 1.0, 0.0, 0.0},
                         {1.0, 1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 1.0}
                       };
  int NumOfPoints[]={6,10,8,6,10,10,4,10,8};

  for (i=0;i<NumOfPoints[num];i++)
   {
    ArrayX[num][i]*=gameData.marker.fScale;
    ArrayY[num][i]*=gameData.marker.fScale;
   }

  if (num==gameData.marker.nHighlight)
   GrSetColorRGB (255, 255, 255, 255);
  else
   GrSetColorRGBi (RGBA_PAL2 (48, 0, 0));


G3TransformAndEncodePoint (&basePoint, &gameData.objs.objects [gameData.marker.tObject[(gameData.multi.nLocalPlayer*2)+num]].pos);
fromPoint.p3_index =
toPoint.p3_index =
basePoint.p3_index = -1;

  for (i=0;i<NumOfPoints[num];i+=2)
   {

    fromPoint=basePoint;
    toPoint=basePoint;

    fromPoint.p3_x+=FixMul ((fl2f (ArrayX[num][i])),viewInfo.scale.x);
    fromPoint.p3_y+=FixMul ((fl2f (ArrayY[num][i])),viewInfo.scale.y);
    G3EncodePoint (&fromPoint);
    G3ProjectPoint (&fromPoint);

    toPoint.p3_x+=FixMul ((fl2f (ArrayX[num][i+1])),viewInfo.scale.x);
    toPoint.p3_y+=FixMul ((fl2f (ArrayY[num][i+1])),viewInfo.scale.y);
    G3EncodePoint (&toPoint);
    G3ProjectPoint (&toPoint);

	AutomapDrawLine (&fromPoint, &toPoint);
   }
 }

//------------------------------------------------------------------------------

void DropMarker (char player_marker_num)
{
	ubyte marker_num = (gameData.multi.nLocalPlayer*2)+player_marker_num;
	tObject *playerp = &gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject];

	gameData.marker.point[marker_num] = playerp->pos;

	if (gameData.marker.tObject[marker_num] != -1)
		ReleaseObject (gameData.marker.tObject[marker_num]);

	gameData.marker.tObject[marker_num] = 
		DropMarkerObject (&playerp->pos, (short) playerp->nSegment, &playerp->orient, marker_num);

#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendDropMarker (gameData.multi.nLocalPlayer, playerp->pos, player_marker_num, gameData.marker.szMessage[marker_num]);
#endif

}

//------------------------------------------------------------------------------

void DropBuddyMarker (tObject *objP)
{
	ubyte	marker_num;

	//	Find spare marker slot.  "if" code below should be an assert, but what if someone changes NUM_MARKERS or MAX_CROP_SINGLE and it never gets hit?
	marker_num = MAX_DROP_SINGLE+1;
	if (marker_num > NUM_MARKERS-1)
		marker_num = NUM_MARKERS-1;

   sprintf (gameData.marker.szMessage[marker_num], "RIP: %s",gameData.escort.szName);

	gameData.marker.point[marker_num] = objP->pos;

	if (gameData.marker.tObject[marker_num] != -1 && gameData.marker.tObject[marker_num] !=0)
		ReleaseObject (gameData.marker.tObject[marker_num]);

	gameData.marker.tObject[marker_num] = DropMarkerObject (&objP->pos, (short) objP->nSegment, &objP->orient, marker_num);

}

//------------------------------------------------------------------------------

#define MARKER_SPHERE_SIZE 0x58000

void DrawMarkers ()
 {
	int i,maxdrop;
	static int cyc=10,cycdir=1;
	g3s_point spherePoint;

	if (gameData.app.nGameMode & GM_MULTI)
   	maxdrop=2;
	else
   	maxdrop=9;

	spherePoint.p3_index = -1;
	for (i=0;i<maxdrop;i++)
		if (gameData.marker.tObject[ (gameData.multi.nLocalPlayer*2)+i] != -1) {

			G3TransformAndEncodePoint (&spherePoint,&gameData.objs.objects[gameData.marker.tObject[ (gameData.multi.nLocalPlayer*2)+i]].pos);

			GrSetColorRGB (PAL2RGBA (10), 0, 0, 255);
			G3DrawSphere (&spherePoint,MARKER_SPHERE_SIZE, 1);
			GrSetColorRGB (PAL2RGBA (20), 0, 0, 255);
			G3DrawSphere (&spherePoint,MARKER_SPHERE_SIZE/2, 1);
			GrSetColorRGB (PAL2RGBA (30), 0, 0, 255);
			G3DrawSphere (&spherePoint,MARKER_SPHERE_SIZE/4, 1);

			DrawMarkerNumber (i);
		}

	if (cycdir)
		cyc+=2;
	else
		cyc-=2;

	if (cyc>43)
	 {
		cyc=43;
		cycdir=0;
	 }
	else if (cyc<10)
	 {
		cyc=10;
		cycdir=1;
	 }

 }

//------------------------------------------------------------------------------

void ClearMarkers ()
 {
	int i;

	for (i=0;i<NUM_MARKERS;i++) {
		gameData.marker.szMessage[i][0]=0;
		gameData.marker.tObject[i]=-1;
	}
 }

//------------------------------------------------------------------------------

void AutomapClearVisited ()	
{
#if 1
	memset (bAutomapVisited, 0, sizeof (bAutomapVisited));
#else
	int i;

	for (i=0; i<MAX_SEGMENTS; i++)
		bAutomapVisited[i] = 0;
#endif
	ClearMarkers ();
}

grs_canvas *name_canv_left,*name_canv_right;

extern void OglDrawCircle2 (int nsides,int nType,float xsc,float xo,float ysc,float yo);

#ifndef M_PI
#	define M_PI 3.141592653589793240
#endif
#define f2glf(x) (f2fl (x))

//------------------------------------------------------------------------------

bool G3DrawSphere3D (g3s_point *p0, int nSides, int rad)
{
	grs_color	c = grdCurCanv->cv_color;
	g3s_point	p = *p0;
	int			i;
	float			hx, hy, x, y, z, r;
	float			ang;

glDisable (GL_TEXTURE_2D);
OglGrsColor (&grdCurCanv->cv_color);
x = f2glf (p.p3_vec.x);
y = f2glf (p.p3_vec.y);
z = -f2glf (p.p3_vec.z);
r = f2glf (rad);
glBegin (GL_POLYGON);
for (i = 0; i <= nSides; i++) {
	ang = 2.0f * (float) M_PI * (i % nSides) / nSides;
	hx = x + (float) cos (ang) * r;
	hy = y + (float) sin (ang) * r;
	glVertex3f (hx, hy, -z);
	}
if (c.rgb)
	glDisable (GL_BLEND);
glEnd ();
return 1;
}

//------------------------------------------------------------------------------

bool G3DrawCircle3D (g3s_point *p0, int nSides, int rad)
{
	g3s_point	p = *p0;
	int			i, j;
	float			hx, hy, x, y, z, r;
	float			ang;

glDisable (GL_TEXTURE_2D);
OglGrsColor (&grdCurCanv->cv_color);
x = f2glf (p.p3_vec.x);
y = f2glf (p.p3_vec.y);
z = -f2glf (p.p3_vec.z);
r = f2glf (rad);
glBegin (GL_LINES);
for (i = 0; i <= nSides; i++)
	for (j = i; j <= i + 1; j++) {
		ang = 2.0f * (float) M_PI * (j % nSides) / nSides;
		hx = x + (float) cos (ang) * r;
		hy = y + (float) sin (ang) * r;
		glVertex3f (hx, hy, z);
		}
if (grdCurCanv->cv_color.rgb)
	glDisable (GL_BLEND);
glEnd ();
return 1;
}

//------------------------------------------------------------------------------

void DrawPlayer (tObject * objP, int bRadar)
{
	vmsVector	arrow_pos, head_pos;
	g3s_point	spherePoint, arrowPoint, headPoint;
	int size = objP->size * (bRadar ? 2 : 1);

headPoint.p3_index =
arrowPoint.p3_index =
spherePoint.p3_index = -1;
// Draw Console player -- shaped like a ellipse with an arrow.
G3TransformAndEncodePoint (&spherePoint, &objP->pos);
G3DrawSphere (&spherePoint, bRadar ? objP->size * 2 : objP->size, !bRadar);
//G3DrawSphere3D (&spherePoint, bRadar ? 8 : 20, objP->size * (bRadar + 1));

if (bRadar && (OBJ_IDX (objP) != gameData.multi.players [gameData.multi.nLocalPlayer].nObject))
	return;
// Draw shaft of arrow
VmVecScaleAdd (&arrow_pos, &objP->pos, &objP->orient.fvec, size*3);
G3TransformAndEncodePoint (&arrowPoint,&arrow_pos);
AutomapDrawLine (&spherePoint, &arrowPoint);

// Draw right head of arrow
VmVecScaleAdd (&head_pos, &objP->pos, &objP->orient.fvec, size*2);
VmVecScaleInc (&head_pos, &objP->orient.rvec, size*1);
G3TransformAndEncodePoint (&headPoint,&head_pos);
AutomapDrawLine (&arrowPoint, &headPoint);

// Draw left head of arrow
VmVecScaleAdd (&head_pos, &objP->pos, &objP->orient.fvec, size*2);
VmVecScaleInc (&head_pos, &objP->orient.rvec, size* (-1));
G3TransformAndEncodePoint (&headPoint,&head_pos);
AutomapDrawLine (&arrowPoint, &headPoint);

// Draw player's up vector
VmVecScaleAdd (&arrow_pos, &objP->pos, &objP->orient.uvec, size*2);
G3TransformAndEncodePoint (&arrowPoint,&arrow_pos);
AutomapDrawLine (&spherePoint, &arrowPoint);
}

int AutomapHires;

//------------------------------------------------------------------------------

#ifndef Pi
#  define  Pi    3.141592653589793240
#endif

int automap_width = 640;
int automap_height = 480;

#define RESCALE_X(x) ((x) * automap_width / 640)
#define RESCALE_Y(y) ((y) * automap_height / 480)

void DrawAutomap (int bRadar)
{
	int			i, color;
	tObject		*objP;
	vmsVector	viewer_position;
	g3s_point	spherePoint;
	vmsMatrix	vmRadar;

if (bRadar && gameStates.render.bTopDownRadar) {
	vmsMatrix *po = &gameData.multi.playerInit [gameData.multi.nLocalPlayer].orient;
	vmRadar.rvec.x = po->rvec.x;
	vmRadar.rvec.y = po->rvec.y;
	vmRadar.rvec.z = po->rvec.z;
	vmRadar.fvec.x = po->uvec.x;
	vmRadar.fvec.y = -po->uvec.y;
	vmRadar.fvec.z = po->uvec.z;
	vmRadar.uvec.x = po->fvec.x;
	vmRadar.uvec.y = po->fvec.y;
	vmRadar.uvec.z = po->fvec.z;
	}
	
#if AUTOMAP_DIRECT_RENDER == 0
if (!AutomapHires) {
#if TRACE
	WIN (con_printf (1, "Can't do lores automap in Windows!\n"));
#endif
	current_page ^= 1;
	GrSetCurrentCanvas (DrawingPages + current_page);
	}
else {
	WINDOS (
		DDGrSetCurrentCanvas (&ddDrawingPage),
		GrSetCurrentCanvas (&DrawingPage)
		);
	}
#endif

#if defined (POLY_ACC)
   pa_flush ();
#endif

WINDOS (
	dd_gr_clear_canvas (RGBA_PAL2 (0,0,0)),
	GrClearCanvas (RGBA_PAL2 (0,0,0))
	);

WIN (DDGRLOCK (dd_grd_curcanv));
	{
#ifdef OGL_ZBUF
	if (bRadar && !gameOpts->legacy.bZBuf)
		gameStates.ogl.bEnableScissor = 0;
#endif
#if AUTOMAP_DIRECT_RENDER
	if (!bRadar && (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)) {
		WIN (DDGRLOCK (dd_grd_curcanv));
		show_fullscr (&bmAutomapBackground);
		GrSetCurFont (HUGE_FONT);
		GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
		GrPrintF (RESCALE_X (80), RESCALE_Y (36), TXT_AUTOMAP, HUGE_FONT);
		GrSetCurFont (SMALL_FONT);
		GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
		GrPrintF (RESCALE_X (60), RESCALE_Y (426), TXT_TURN_SHIP);
		GrPrintF (RESCALE_X (60), RESCALE_Y (443), TXT_SLIDE_UPDOWN);
		GrPrintF (RESCALE_X (60), RESCALE_Y (460), TXT_VIEWING_DISTANCE);
		WIN (DDGRUNLOCK (dd_grd_curcanv));
		//GrUpdate (0);
		}
#endif
	G3StartFrame (1,0); //!bRadar);
#if AUTOMAP_DIRECT_RENDER
	if (!bRadar && (gameStates.render.cockpit.nMode != CM_FULL_SCREEN))
		OGL_VIEWPORT (RESCALE_X (27), RESCALE_Y (80), RESCALE_X (582), RESCALE_Y (334));
#endif
	RenderStartFrame ();
	if (bRadar && gameStates.render.bTopDownRadar) {
		VmVecScaleAdd (&viewer_position, &amData.viewTarget, &vmRadar.fvec, -amData.nViewDist);
		G3SetViewMatrix (&viewer_position, &vmRadar, amData.nZoom * 2);
		}
	else {
		VmVecScaleAdd (&viewer_position, &amData.viewTarget, &amData.viewMatrix.fvec, bRadar ? -amData.nViewDist : -amData.nViewDist);
		G3SetViewMatrix (&viewer_position, &amData.viewMatrix, bRadar ? (amData.nZoom * 3) / 2 : amData.nZoom);
		}
	//if (!bRadar)
		DrawAllEdges ();
	// Draw player...
#ifdef NETWORK
	if (gameData.app.nGameMode & GM_TEAM)
		color = GetTeam (gameData.multi.nLocalPlayer);
	else
#endif	
		color = gameData.multi.nLocalPlayer;	// Note link to above if!

	GrSetColorRGBi (RGBA_PAL2 (player_rgb [color].r, player_rgb [color].g,player_rgb [color].b));
	DrawPlayer (gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].nObject, bRadar);

	if (!bRadar) {
		DrawMarkers ();
		if (gameData.marker.nHighlight>-1 && gameData.marker.szMessage[gameData.marker.nHighlight][0]!=0)
		 {
			char msg[10+MARKER_MESSAGE_LEN+1];
			sprintf (msg, TXT_MARKER_MSG, gameData.marker.nHighlight+1,
						gameData.marker.szMessage[ (gameData.multi.nLocalPlayer*2)+gameData.marker.nHighlight]);
			GrSetColorRGB (196, 0, 0, 255);
			ModexPrintF (5,20,msg,SMALL_FONT,automapColors.nDkGray);
		 }
	}				
	// Draw player (s)...
#ifdef NETWORK
	if ( (gameData.app.nGameMode & (GM_TEAM | GM_MULTI_COOP)) || (netGame.gameFlags & NETGAME_FLAG_SHOW_MAP))	{
		for (i = 0; i<gameData.multi.nPlayers; i++)		{
			if ((i != gameData.multi.nLocalPlayer) && ((gameData.app.nGameMode & GM_MULTI_COOP) || 
				 (GetTeam (gameData.multi.nLocalPlayer) == GetTeam (i)) || 
				 (netGame.gameFlags & NETGAME_FLAG_SHOW_MAP)))	{
				if (gameData.objs.objects[gameData.multi.players[i].nObject].nType == OBJ_PLAYER)	{
					if (gameData.app.nGameMode & GM_TEAM)
						color = GetTeam (i);
					else
						color = i;
					GrSetColorRGBi (RGBA_PAL2 (player_rgb [color].r, player_rgb [color].g, player_rgb [color].b));
					DrawPlayer (gameData.objs.objects + gameData.multi.players[i].nObject, bRadar);
				}
			}
		}
	}
#endif
	if (1) {//!bRadar) {
		int size;
		objP = gameData.objs.objects;
		for (i = 0; i <= gameData.objs.nLastObject; i++, objP++) {
			size = objP->size;
			switch (objP->nType)	{
			case OBJ_HOSTAGE:
				GrSetColorRGBi (automapColors.nHostage);
				G3TransformAndEncodePoint (&spherePoint,&objP->pos);
				G3DrawSphere (&spherePoint,size, !bRadar);	
				break;

			case OBJ_MONSTERBALL:
				GrSetColorRGBi (automapColors.nMonsterball);
				G3TransformAndEncodePoint (&spherePoint,&objP->pos);
				G3DrawSphere (&spherePoint,size, !bRadar);	
				break;

			case OBJ_ROBOT:
				if (
#if 1//ndef _DEBUG
					 bAutomapVisited[objP->nSegment] && 
#endif
					 EGI_FLAG (bRobotsOnRadar, 0, 0)) {
					static int c = 0;
					static int t = 0;
					int h = SDL_GetTicks ();
					if (h - t > 250) {
						t = h;
						c = !c;
						}
					if (gameData.bots.pInfo[objP->id].companion)
						if (c)
							GrSetColorRGB (0, 123, 151, 255); //gr_getcolor (47, 1, 47)); 
						else
							GrSetColorRGB (0, 78, 112, 255); //gr_getcolor (47, 1, 47)); 
					else
						if (c)
							GrSetColorRGB (123, 0, 135, 255); //gr_getcolor (47, 1, 47)); 
						else
							GrSetColorRGB (78, 0, 96, 255); //gr_getcolor (47, 1, 47)); 
					G3TransformAndEncodePoint (&spherePoint,&objP->pos);
					G3DrawSphere (&spherePoint, (size*3)/2, !bRadar);	
				}
				break;

			case OBJ_POWERUP:
				if (EGI_FLAG (bPowerUpsOnRadar, 0, 0) && 
					 !(gameData.app.nGameMode & GM_MULTI) && 
					 (gameStates.render.bAllVisited || bAutomapVisited[objP->nSegment]))	{
					//if ( (objP->id==POW_KEY_RED) || (objP->id==POW_KEY_BLUE) || (objP->id==POW_KEY_GOLD))	
					{
						switch (objP->id) {
						case POW_KEY_RED:		
							GrSetColorRGBi (RGBA_PAL2 (63, 5, 5));	
							size *= 4;
							break;
						case POW_KEY_BLUE:	
							GrSetColorRGBi (RGBA_PAL2 (5, 5, 63)); 
							size *= 4;
							break;
						case POW_KEY_GOLD:	
							GrSetColorRGBi (RGBA_PAL2 (63, 63, 10)); 
							size *= 4;
							break;
						default:
							GrSetColorRGBi (ORANGE_RGBA); //orange
							//Error ("Illegal key nType: %i", objP->id);
						}
						G3TransformAndEncodePoint (&spherePoint,&objP->pos);
						G3DrawSphere (&spherePoint,size, !bRadar);	
					}
				}
				break;
			}
		}
	}

	G3EndFrame ();

	if (bRadar) {
#ifdef OGL_ZBUF
		if (!gameOpts->legacy.bZBuf)
			gameStates.ogl.bEnableScissor = 0;
		return;
#endif
		}
	else {
		GrBitmapM (AutomapHires?10:5, AutomapHires?10:5, &name_canv_left->cv_bitmap);
		GrBitmapM (grdCurCanv->cv_bitmap.bm_props.w- (AutomapHires?10:5)-name_canv_right->cv_bitmap.bm_props.w,AutomapHires?10:5,&name_canv_right->cv_bitmap);
		}
}
WIN (DDGRUNLOCK (dd_grd_curcanv));

#ifdef OGL
	OglSwapBuffers (0, 0);
#else
#if AUTOMAP_DIRECT_RENDER == 0
	if (!AutomapHires)
		GrShowCanvas (&Pages[current_page]);
	else {
	#ifndef WINDOWS
		//GrBmUBitBlt (Page.cv_bitmap.bm_props.w, Page.cv_bitmap.bm_props.h, Page.cv_bitmap.bm_props.x, Page.cv_bitmap.bm_props.y, 0, 0, &Page.cv_bitmap, &VR_screen_pages[0].cv_bitmap);
		GrBmUBitBlt (Page.cv_bitmap.bm_props.w, Page.cv_bitmap.bm_props.h, Page.cv_bitmap.bm_props.x, Page.cv_bitmap.bm_props.y, 0, 0, &Page.cv_bitmap, &grdCurScreen->sc_canvas.cv_bitmap);
	#else
		dd_gr_blt_screen (&ddPage, 0,0,0,0,0,0,0,0);
	#endif
	}
	GrUpdate (0);
#endif
#endif
}

#ifdef WINDOWS
#define LEAVE_TIME 0x00010000
#else
#define LEAVE_TIME 0x4000
#endif

#define WINDOW_WIDTH		288


//------------------------------------------------------------------------------

//print to canvas & float height
grs_canvas *PrintToCanvas (char *s,grs_font *font, unsigned int fc, unsigned int bc, int doubleFlag)
{
	int y;
	ubyte *data;
	int rs;
	grs_canvas *temp_canv;
	grs_font *save_font;
	int w,h,aw;

WINDOS (
	dd_grs_canvas *save_canv,
	grs_canvas *save_canv
);

WINDOS (
	save_canv = dd_grd_curcanv,
	save_canv = grdCurCanv
);

	save_font = grdCurCanv->cv_font;
	GrSetCurFont (font);					//set the font we're going to use
	GrGetStringSize (s,&w,&h,&aw);		//now get the string size
	GrSetCurFont (save_font);				//restore real font

	//temp_canv = GrCreateCanvas (font->ft_w*strlen (s),font->ft_h*2);
	temp_canv = GrCreateCanvas (w,font->ft_h*2);
	temp_canv->cv_bitmap.bm_palette = gamePalette;
	GrSetCurrentCanvas (temp_canv);
	GrSetCurFont (font);
	GrClearCanvas (0);						//trans color
	GrSetFontColorRGBi (fc, 1, bc, bc != 0);
	GrPrintF (0,0,s);

	//now float it, since we're drawing to 400-line modex screen

	if (doubleFlag) {
		data = temp_canv->cv_bitmap.bm_texBuf;
		rs = temp_canv->cv_bitmap.bm_props.rowsize;

		for (y=temp_canv->cv_bitmap.bm_props.h/2;y--;) {
			memcpy (data+ (rs*y*2),data+ (rs*y),temp_canv->cv_bitmap.bm_props.w);
			memcpy (data+ (rs* (y*2+1)),data+ (rs*y),temp_canv->cv_bitmap.bm_props.w);
		}
	}

WINDOS (
	DDGrSetCurrentCanvas (save_canv),
	GrSetCurrentCanvas (save_canv)
);

	return temp_canv;
}

//------------------------------------------------------------------------------
//print to buffer, float heights, and blit bitmap to screen
void ModexPrintF (int x,int y,char *s,grs_font *font, unsigned int color)
{
	grs_canvas *temp_canv;

temp_canv = PrintToCanvas (s, font, color, 0, !AutomapHires);
GrBitmapM (x,y,&temp_canv->cv_bitmap);
GrFreeCanvas (temp_canv);
}

//------------------------------------------------------------------------------
//name for each group.  maybe move somewhere else
char *system_name[] = {
			"Zeta Aquilae",
			"Quartzon System",
			"Brimspark System",
			"Limefrost Spiral",
			"Baloris Prime",
			"Omega System"};

void create_name_canv ()
{
	char	nameLevel_left[128],nameLevel_right[128];

	if (gameData.missions.nCurrentLevel > 0)
		sprintf (nameLevel_left, "%s %i",TXT_LEVEL, gameData.missions.nCurrentLevel);
	else
		sprintf (nameLevel_left, "Secret Level %i",-gameData.missions.nCurrentLevel);

	if (gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission && gameData.missions.nCurrentLevel>0)		//built-in mission
		sprintf (nameLevel_right,"%s %d: ",system_name[ (gameData.missions.nCurrentLevel-1)/4], ((gameData.missions.nCurrentLevel-1)%4)+1);
	else
		strcpy (nameLevel_right, " ");

	strcat (nameLevel_right, gameData.missions.szCurrentLevel);

	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
	name_canv_left = PrintToCanvas (nameLevel_left, SMALL_FONT, automapColors.nMedGreen, 0, !AutomapHires);
	name_canv_right = PrintToCanvas (nameLevel_right,SMALL_FONT, automapColors.nMedGreen, 0, !AutomapHires);

}

//------------------------------------------------------------------------------

extern void GameLoop (int, int);
extern int SetSegmentDepths (int start_seg, ubyte *segbuf);

#ifdef RELEASE
#define MAP_BACKGROUND_FILENAME (AutomapHires?"\x01MAPB.PCX":"\x01MAP.PCX")	//load only from hog file
#else
#define MAP_BACKGROUND_FILENAME ((AutomapHires && CFExist ("mapb.pcx",gameFolders.szDataDir,0))?"MAPB.PCX":"MAP.PCX")
#endif

u_int32_t automap_mode = SM (640,480);

void DoAutomap (int key_code, int bRadar)	
{
	int			done=0;
	vmsMatrix	tempm;
	vmsAngVec	tangles;
	int			leave_mode=0;
	int			firstTime=1;
	int			pcx_error;
#if !defined (AUTOMAP_DIRECT_RENDER) || !defined (NDEBUG)
	int			i;
#endif
	int			c, marker_num;
	fix			entryTime;
	int			pause_game=1;		// Set to 1 if everything is paused during automap...No pause during net.
	fix			t1, t2;
	control_info saved_control_info;
	int			Max_segments_away = 0;
	int			SegmentLimit = 1;
	char			maxdrop;
	int			must_free_canvas=0;
	int			nContrast = gameStates.ogl.nContrast;

	static ubyte	automap_pal [256*3];

	WIN (int dd_VR_screegameStates.render.cockpit.nModeSave);
	WIN (int redraw_screen=0);

	gameStates.app.bAutoMap = 1;
	gameStates.ogl.nContrast = 8;
	init_automap_colors ();
	key_code = key_code;	// disable warning...
	if (bRadar || ((gameData.app.nGameMode & GM_MULTI) && (gameStates.app.nFunctionMode == FMODE_GAME) && (!gameStates.app.bEndLevelSequence)))
		pause_game = 0;
	if (pause_game) {
		StopTime ();
		DigiPauseDigiSounds ();
	}
	Max_edges = MAX_EDGES; //min (MAX_EDGES_FROM_VERTS (gameData.segs.nVertices), MAX_EDGES);			//make maybe smaller than max
#if !defined (WINDOWS)
	if (bRadar || (gameStates.video.nDisplayMode > 1) || (gameOpts->render.bAutomapAlwaysHires && gameStates.menus.bHiresAvailable)) {
#if !defined (POLY_ACC)
		//edit 4/23/99 Matt Mueller - don't switch res unless we need to
		if (grdCurScreen->sc_mode != AUTOMAP_MODE)
			GrSetMode (gameStates.render.bAutomapUseGameRes ? gameStates.video.nLastScreenMode : AUTOMAP_MODE);
		else if (!bRadar)
			GrSetCurrentCanvas (NULL);
		//end edit -MM
		if (bRadar) {
			automap_width=grdCurCanv->cv_bitmap.bm_props.w;
			automap_height=grdCurCanv->cv_bitmap.bm_props.h;
			}
		else {
			automap_width=grdCurScreen->sc_canvas.cv_bitmap.bm_props.w;
			automap_height=grdCurScreen->sc_canvas.cv_bitmap.bm_props.h;
			}
#endif
		PA_DFX (pa_set_frontbuffer_current ());
		AutomapHires = 1;
	}
	else {
		GrSetMode (SM (320, 400));
		AutomapHires = 0;
	}
	#else
		AutomapHires = 1;		//Mac & Windows (?) always in hires
	#endif

	#ifdef WINDOWS
		dd_VR_screegameStates.render.cockpit.nModeSave = VR_screen_mode;
		VR_screen_mode = SM95_640x480x8;	// HACK! Forcing reinit of 640x480
		SetScreenMode (SCREEN_GAME);
	#endif

	gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && AutomapHires;

	if (!bRadar) {
		create_name_canv ();
		GrPaletteStepClear ();
		}

WIN (AutomapRedraw:)
	if (!AutomapHires) {
#if AUTOMAP_DIRECT_RENDER == 0
		GrInitSubCanvas (&Pages[0],grdCurCanv,0,0,320,400);
		GrInitSubCanvas (&Pages[1],grdCurCanv,0,401,320,400);
		GrInitSubCanvas (&DrawingPages[0],&Pages[0],16,69,WINDOW_WIDTH,272);
		GrInitSubCanvas (&DrawingPages[1],&Pages[1],16,69,WINDOW_WIDTH,272);
#endif

		if (!bRadar) {
			GrInitBitmapData (&bmAutomapBackground);
			pcx_error = pcx_read_bitmap (MAP_BACKGROUND_FILENAME, &bmAutomapBackground, BM_LINEAR, 0);
			if (pcx_error != PCX_ERROR_NONE)
				Error ("File %s - PCX error: %s", MAP_BACKGROUND_FILENAME, pcx_errormsg (pcx_error));
			GrRemapBitmapGood (&bmAutomapBackground, NULL, -1, -1);
			}

#if AUTOMAP_DIRECT_RENDER == 0
		for (i=0; i<2; i++) {
			GrSetCurrentCanvas (Pages + i);
			GrBitmap (0, 0, &bmAutomapBackground);
			ModexPrintF (40,  22, TXT_AUTOMAP, HUGE_FONT, automapColors.nDkGray);
			ModexPrintF (30, 353, TXT_TURN_SHIP, SMALL_FONT, automapColors.nDkGray);
			ModexPrintF (30, 369, TXT_SLIDE_UPDOWN, SMALL_FONT, automapColors.nDkGray);
			ModexPrintF (30, 385, TXT_VIEWING_DISTANCE, SMALL_FONT, automapColors.nDkGray);
			}
		GrFreeBitmapData (&bmAutomapBackground);	
		GrSetCurrentCanvas (&DrawingPages[current_page]);
#endif /* AUTOMAP_DIRECT_RENDER */
		}
	else {
#if AUTOMAP_DIRECT_RENDER == 0
		if (VR_render_buffer[0].cv_w >= automap_width && VR_render_buffer[0].cv_h >= automap_height) {
			WIN (DDGrInitSubCanvas (&ddPage, &dd_VR_render_buffer[0], 0, 0, automap_width,automap_height);
			GrInitSubCanvas (&Page,&VR_render_buffer[0],0, 0, automap_width, automap_height);)
			}
		else {
#ifndef WINDOWS
			void *raw_data;
			MALLOC (raw_data,ubyte,automap_width*automap_height);
			GrInitCanvas (&Page,raw_data,BM_LINEAR,automap_width,automap_height);
#else
			dd_gr_init_canvas (&ddPage, BM_LINEAR, automap_width,automap_height);
			GrInitCanvas (&Page,NULL,BM_LINEAR,automap_width,automap_height);
#endif
			must_free_canvas = 1;
		}

		WIN (
			DDGrInitSubCanvas (&ddDrawingPage, &ddPage, RESCALE_X (27), RESCALE_Y (80), RESCALE_X (582), RESCALE_Y (334));
			GrInitSubCanvas (&DrawingPage, &Page, RESCALE_X (27), RESCALE_Y (80), RESCALE_X (582), RESCALE_Y (334));
			)

		WINDOS (
			DDGrSetCurrentCanvas (&ddPage),
			GrSetCurrentCanvas (&Page)
		);
#endif //AUTOMAP_DIRECT_RENDER

		if (!bRadar) {
#if defined (POLY_ACC)
    	pcx_error = pcx_read_bitmap (MAP_BACKGROUND_FILENAME,& (grdCurCanv->cv_bitmap),BM_LINEAR15, 0);
#else
#	if 1
			GrInitBitmapData (&bmAutomapBackground);
			pcx_error = pcx_read_bitmap (MAP_BACKGROUND_FILENAME, &bmAutomapBackground, BM_LINEAR, 0);
			if (pcx_error != PCX_ERROR_NONE)
				Error ("File %s - PCX error: %s", MAP_BACKGROUND_FILENAME, pcx_errormsg (pcx_error));
			GrRemapBitmapGood (&bmAutomapBackground, NULL, -1, -1);
#	else
			pcx_error = pcx_read_fullscr (MAP_BACKGROUND_FILENAME, automap_pal);
			if (pcx_error != PCX_ERROR_NONE)	{
				//printf ("File %s - PCX error: %s",MAP_BACKGROUND_FILENAME,pcx_errormsg (pcx_error);
				Error ("File %s - PCX error: %s",MAP_BACKGROUND_FILENAME,pcx_errormsg (pcx_error));
				return;
				}
			GrRemapBitmapGood (& (grdCurCanv->cv_bitmap), automap_pal, -1, -1);
#	endif
#endif
		}
	
#if AUTOMAP_DIRECT_RENDER == 0
		WINDOS (
			DDGrSetCurrentCanvas (&ddDrawingPage),
			GrSetCurrentCanvas (&DrawingPage)
		);
#endif
	}


WIN (if (!redraw_screen) {)
	AutomapBuildEdgeList ();

	if (bRadar)
		amData.nViewDist = ZOOM_DEFAULT;
	else if (!amData.nViewDist)
		amData.nViewDist = ZOOM_DEFAULT;
	amData.viewMatrix = gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].orient;

	tangles.p = PITCH_DEFAULT;
	tangles.h  = 0;
	tangles.b  = 0;

	done = 0;

	amData.viewTarget = gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].pos;

	t1 = entryTime = TimerGetFixedSeconds ();
	t2 = t1;

	//Fill in bAutomapVisited from gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].nSegment
	if (bRadar) {
#ifdef RELEASE
		if (! (gameData.app.nGameMode & GM_MULTI))
			memcpy (bRadarVisited, bAutomapVisited, sizeof (bRadarVisited));
#endif
		memset (bAutomapVisited, 1, sizeof (bRadarVisited));
		}
	Max_segments_away = 
		SetSegmentDepths (
			gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].nSegment, 
			bRadar ? bAutomapVisited : bAutomapVisited);
	SegmentLimit = bRadar ? Max_segments_away : Max_segments_away;

	adjust_segment_limit (SegmentLimit, bRadar ? bAutomapVisited : bAutomapVisited);
#ifdef RELEASE
	if (bRadar && ! (gameData.app.nGameMode & GM_MULTI))
		memcpy (bAutomapVisited, bRadarVisited, sizeof (bRadarVisited));
#endif
WIN (})

WIN (if (redraw_screen) redraw_screen = 0);

	if (bRadar) {
		DrawAutomap (1);
		gameStates.ogl.nContrast = nContrast;
		gameStates.app.bAutoMap = 0;
		return;
		}

	while (!done)	{
		if (leave_mode==0 && Controls.automap_state && (TimerGetFixedSeconds ()-entryTime)>LEAVE_TIME)
			leave_mode = 1;

		if (!Controls.automap_state && (leave_mode==1))
			done=1;

		if (!pause_game)	{
			ushort old_wiggle;
			saved_control_info = Controls;				// Save controls so we can zero them
			memset (&Controls,0,sizeof (control_info));	// Clear everything...
			old_wiggle = gameData.objs.console->mType.physInfo.flags & PF_WIGGLE;	// Save old wiggle
			gameData.objs.console->mType.physInfo.flags &= ~PF_WIGGLE;		// Turn off wiggle
			#ifdef NETWORK
			if (MultiMenuPoll ())
				done = 1;
			#endif
//			GameLoop (0, 0);		// Do game loop with no rendering and no reading controls.
			gameData.objs.console->mType.physInfo.flags |= old_wiggle;	// Restore wiggle
			Controls = saved_control_info;
		}

	GetSlowTick ();
	#ifndef WINDOWS
		ControlsReadAll ();		
	#else
		controls_read_all_win ();
	#endif

		if (Controls.automapDownCount)	{
			if (leave_mode==0)
				done = 1;
			c = 0;
		}

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat ();

		#ifdef WINDOWS
		{
			MSG msg;
			DoMessageStuff (&msg);
			if (_RedrawScreen) {
				_RedrawScreen = FALSE;
				redraw_screen = 1;
				goto AutomapRedraw;
			}
				
			if (msg.message == WM_QUIT) exit (1);

			DDGRRESTORE;
		}
		#endif

		while ( (c=KeyInKey ()))	{
			MultiDoFrame();
			switch (c) {
			#ifndef NDEBUG
			case KEY_BACKSP: Int3 (); break;
			#endif
	
			case KEY_PRINT_SCREEN: {
				if (AutomapHires) {
				WINDOS (
					DDGrSetCurrentCanvas (NULL),
						GrSetCurrentCanvas (NULL)
				);
				}
#if AUTOMAP_DIRECT_RENDER == 0
				else
					GrSetCurrentCanvas (&Pages[current_page]);
#endif
				bSaveScreenShot = 1;
				SaveScreenShot (NULL, 1);
				break;
			}
	
			case KEY_ESC:
				if (leave_mode==0)
					done = 1;
				 break;

			#ifndef NDEBUG
		  	case KEY_DEBUGGED+KEY_F: 	{
				for (i=0; i<=gameData.segs.nLastSegment; i++)
					bAutomapVisited[i] = 1;
				AutomapBuildEdgeList ();
				Max_segments_away = SetSegmentDepths (gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].nSegment, bAutomapVisited);
				SegmentLimit = Max_segments_away;
				adjust_segment_limit (SegmentLimit, bAutomapVisited);
				}
				break;
			#endif

			case KEY_MINUS:
				if (SegmentLimit > 1) 		{
					SegmentLimit--;
					adjust_segment_limit (SegmentLimit, bAutomapVisited);
				}
				break;
			case KEY_EQUAL:
				if (SegmentLimit < Max_segments_away) 	{
					SegmentLimit++;
					adjust_segment_limit (SegmentLimit, bAutomapVisited);
				}
				break;
			case KEY_1:
			case KEY_2:
			case KEY_3:
			case KEY_4:
			case KEY_5:
			case KEY_6:
			case KEY_7:
			case KEY_8:
			case KEY_9:
			case KEY_0:
				if (gameData.app.nGameMode & GM_MULTI)
			   	maxdrop=2;
				else
			   	maxdrop=9;

			marker_num = c-KEY_1;
            if (marker_num<=maxdrop)
				 {
					if (gameData.marker.tObject[marker_num] != -1)
						gameData.marker.nHighlight=marker_num;
				 }
			  break;

			case KEY_D+KEY_CTRLED:
#if AUTOMAP_DIRECT_RENDER == 0
				if (current_page)		//menu will only work on page 0
					DrawAutomap (0);	//..so switch from 1 to 0
#endif

				if (gameData.marker.nHighlight > -1 && gameData.marker.tObject[gameData.marker.nHighlight] != -1) {
#if AUTOMAP_DIRECT_RENDER == 0
					WINDOS (
						DDGrSetCurrentCanvas (&ddPages[current_page]),
						GrSetCurrentCanvas (&Pages[current_page])
					);
#endif

					if (ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, "Delete Marker?") == 0) {
						ReleaseObject (gameData.marker.tObject[gameData.marker.nHighlight]);
						gameData.marker.tObject[gameData.marker.nHighlight]=-1;
						gameData.marker.szMessage[gameData.marker.nHighlight][0]=0;
						gameData.marker.nHighlight = -1;
					}					
				}
				break;

			#ifndef RELEASE
			case KEY_COMMA:
				if (gameData.marker.fScale>.5)
					gameData.marker.fScale-=.5;
				break;
			case KEY_PERIOD:
				if (gameData.marker.fScale<30.0)
					gameData.marker.fScale+=.5;
				break;
			#endif

//added 8/23/99 by Matt Mueller for hot key res/fullscreen changing, and menu access
#if 0
			case KEY_CTRLED+KEY_SHIFTED+KEY_PADDIVIDE:
			case KEY_ALTED+KEY_CTRLED+KEY_PADDIVIDE:
			case KEY_ALTED+KEY_SHIFTED+KEY_PADDIVIDE:
				LY:
			case KEY_ALTED+KEY_SHIFTED+KEY_PADMULTIPLY:
				change_res ();
				break;
			case KEY_CTRLED+KEY_SHIFTED+KEY_PADMINUS:
			case KEY_ALTED+KEY_CTRLED+KEY_PADMINUS:
			case KEY_ALTED+KEY_SHIFTED+KEY_PADMINUS:
				//lower res
				//should we just cycle through the list that is displayed in the res change menu?
				// what if their card/X/etc can't handle that mode? hrm.
				//well, the quick access to the menu is good enough for now.
				break;
			case KEY_CTRLED+KEY_SHIFTED+KEY_PADPLUS:
			case KEY_ALTED+KEY_CTRLED+KEY_PADPLUS:
			case KEY_ALTED+KEY_SHIFTED+KEY_PADPLUS:
				//increase res
				break;
#endif
			case KEY_ALTED+KEY_ENTER:
			case KEY_ALTED+KEY_PADENTER:
				GrToggleFullScreenGame ();
				break;
//end addition -MM

	      }
		}

		if (Controls.fire_primaryDownCount)	{
			// Reset orientation
			amData.nViewDist = ZOOM_DEFAULT;
			tangles.p = PITCH_DEFAULT;
			tangles.h  = 0;
			tangles.b  = 0;
			amData.viewTarget = gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].pos;
		}
#if 0
		amData.nViewDist -= Controls.forward_thrustTime*ZOOM_SPEED_FACTOR;
#else		
		if (Controls.forward_thrustTime)
			VmVecScaleInc (&amData.viewTarget, &amData.viewMatrix.fvec, Controls.forward_thrustTime*ZOOM_SPEED_FACTOR); 
#endif
		tangles.p += FixDiv (Controls.pitchTime, ROT_SPEED_DIVISOR);
		tangles.h  += FixDiv (Controls.headingTime, ROT_SPEED_DIVISOR);
		tangles.b  += FixDiv (Controls.bankTime, ROT_SPEED_DIVISOR*2);

		if (Controls.vertical_thrustTime || Controls.sideways_thrustTime)	{
			vmsAngVec	tangles1;
			vmsVector	old_vt;
			old_vt = amData.viewTarget;
			tangles1 = tangles;
			VmAngles2Matrix (&tempm,&tangles1);
			VmMatMul (&amData.viewMatrix,&gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].orient,&tempm);
			VmVecScaleInc (&amData.viewTarget, &amData.viewMatrix.uvec, Controls.vertical_thrustTime*SLIDE_SPEED);
			VmVecScaleInc (&amData.viewTarget, &amData.viewMatrix.rvec, Controls.sideways_thrustTime*SLIDE_SPEED);
#if 0
			if (VmVecDistQuick (&amData.viewTarget, &gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].pos) > i2f (1000))	{
				amData.viewTarget = old_vt;
			}
#endif
		}

		VmAngles2Matrix (&tempm,&tangles);
		VmMatMul (&amData.viewMatrix,&gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].orient,&tempm);

		if (amData.nViewDist < ZOOM_MIN_VALUE) 
			amData.nViewDist = ZOOM_MIN_VALUE;
		if (amData.nViewDist > ZOOM_MAX_VALUE) 
			amData.nViewDist = ZOOM_MAX_VALUE;

		DrawAutomap (0);

		if (firstTime)	{
			firstTime = 0;
			GrPaletteStepLoad (NULL);
		}

		t2 = TimerGetFixedSeconds ();
		if (pause_game)
			gameData.time.xFrame=t2-t1;
		t1 = t2;
	}

	//d_free (Edges);
	//d_free (DrawingListBright);

	GrFreeCanvas (name_canv_left);  name_canv_left=NULL;
	GrFreeCanvas (name_canv_right);  name_canv_right=NULL;

	if (must_free_canvas)	{
#if AUTOMAP_DIRECT_RENDER == 0
	WINDOS (
		DDFreeSurface (ddPages[0].lpdds),
		d_free (Page.cv_bitmap.bm_texBuf)
	);
#endif
	}
	GameFlushInputs ();

	if (pause_game)
	{
		StartTime ();
		DigiResumeDigiSounds ();
	}

#ifdef WINDOWS
	VR_screen_mode = dd_VR_screegameStates.render.cockpit.nModeSave;
#endif

	gameStates.ogl.nContrast = nContrast;
	gameStates.app.bAutoMap = 0;
}

//------------------------------------------------------------------------------

void adjust_segment_limit (int SegmentLimit, ubyte *pVisited)
{
	int i,e1;
	Edge_info * e;
	for (i=0; i<=Highest_edge_index; i++)	{
		e = Edges + i;
		e->flags |= EF_TOO_FAR;
		for (e1=0; e1<e->num_faces; e1++)	{
			if (pVisited[e->nSegment[e1]] <= SegmentLimit)	{
				e->flags &= (~EF_TOO_FAR);
				break;
			}
		}
	}

}

//------------------------------------------------------------------------------

void DrawAllEdges ()
{
	g3s_codes cc;
	int i,j,nbright;
	ubyte nfacing,nnfacing;
	Edge_info *e;
	vmsVector *tv1;
	fix distance;
	fix minDistance = 0x7fffffff;
	g3s_point *p1, *p2;


	nbright=0;

	for (i=0; i<=Highest_edge_index; i++)	{
		//e = &Edges[Edge_used_list[i]];
		e = Edges + i;
		if (! (e->flags & EF_USED)) 
			continue;

		if (e->flags & EF_TOO_FAR) 
			continue;

		if (e->flags & EF_FRONTIER) {		// A line that is between what we have seen and what we haven't
			if ((!(e->flags & EF_SECRET)) && (e->color == automapColors.walls.nNormal))
				continue;		// If a line isn't secret and is normal color, then don't draw it
		}

		cc=RotateList (2,e->verts);
		distance = gameData.segs.points[e->verts[1]].p3_z;

		if (minDistance>distance)
			minDistance = distance;

		if (!cc.and) 	{	//all off screen?
			nfacing = nnfacing = 0;
			tv1 = gameData.segs.vertices + e->verts[0];
			j = 0;
			while (j<e->num_faces && (nfacing==0 || nnfacing==0))	{
#ifdef COMPACT_SEGS
				vmsVector temp_v;
				GetSideNormal (&gameData.segs.segments[e->nSegment[j]], e->sides[j], 0, &temp_v);
				if (!G3CheckNormalFacing (tv1, &temp_v))
#else
				if (!G3CheckNormalFacing (tv1, &gameData.segs.segments[e->nSegment[j]].sides[e->sides[j]].normals[0]))
#endif
					nfacing++;
				else
					nnfacing++;
				j++;
			}

			if (nfacing && nnfacing)	{
				// a contour line
				DrawingListBright[nbright++] = EDGE_IDX (e);
				}
			else if (e->flags & (EF_DEFINING|EF_GRATE))	{
				if (nfacing == 0)	{
					if (e->flags & EF_NO_FADE)
						GrSetColorRGBi (e->color);
					else
						GrSetColorRGBi (RGBA_FADE (e->color, 32.0 / 8.0));
					G3DrawLine (gameData.segs.points + e->verts[0], gameData.segs.points + e->verts[1]);
					}
				else {
					DrawingListBright[nbright++] = EDGE_IDX (e);
				}
			}
		}
	}

	if (minDistance < 0) minDistance = 0;

	// Sort the bright ones using a shell sort
	{
		int t;
		int i, j, incr, v1, v2;

		incr = nbright / 2;
		while (incr > 0)	{
			for (i=incr; i<nbright; i++)	{
				j = i - incr;
				while (j>=0)	{
					// compare element j and j+incr
					v1 = Edges[DrawingListBright[j]].verts[0];
					v2 = Edges[DrawingListBright[j+incr]].verts[0];

					if (gameData.segs.points[v1].p3_z < gameData.segs.points[v2].p3_z) {
						// If not in correct order, them swap 'em
						t=DrawingListBright[j+incr];
						DrawingListBright[j+incr]=DrawingListBright[j];
						DrawingListBright[j]=t;
						j -= incr;
					}
					else
						break;
				}
			}
			incr /= 2;
		}
	}

	// Draw the bright ones
	for (i=0; i<nbright; i++)	{
		int color;
		fix dist;
		e = Edges + DrawingListBright[i];
		p1 = gameData.segs.points + e->verts[0];
		p2 = gameData.segs.points + e->verts[1];
		dist = p1->p3_z - minDistance;
		// Make distance be 1.0 to 0.0, where 0.0 is 10 segments away;
		if (dist < 0) 
			dist=0;
		if (dist >= amData.nMaxDist) 
			continue;

		if (e->flags & EF_NO_FADE)	{
			GrSetColorRGBi (e->color);
		} else {
			dist = F1_0 - FixDiv (dist, amData.nMaxDist);
			color = f2i (dist*31);
			GrSetColorRGBi (RGBA_FADE (e->color, 32.0 / color));
		}
		G3DrawLine (p1, p2);
	}

}


//==================================================================
//
// All routines below here are used to build the Edge list
//
//==================================================================


//finds edge, filling in edge_ptr. if found old edge, returns index, else return -1
static int AutomapFindEdge (int v0,int v1,Edge_info **edge_ptr)
{
	long vv, evv;
	int hash,oldhash;
	int ret, ev0, ev1;

	vv = (v1<<16) + v0;

	oldhash = hash = ((v0*5+v1) % Max_edges);

	ret = -1;

	while (ret==-1) {
		ev0 = (int) (Edges[hash].verts[0]);
		ev1 = (int) (Edges[hash].verts[1]);
		evv = (ev1<<16)+ev0;
		if (Edges[hash].num_faces == 0) ret=0;
		else if (evv == vv) ret=1;
		else {
			if (++hash==Max_edges) hash=0;
			if (hash==oldhash) Error ("Edge list full!");
		}
	}
CBRK (hash > MAX_EDGES);
	*edge_ptr = &Edges[hash];

	if (ret == 0)
		return -1;
	else
		return hash;

}

//------------------------------------------------------------------------------

void AddOneEdge (int va, int vb, unsigned int color, ubyte tSide, short nSegment, 
					  int hidden, int grate, int bNoFade)	
{
	int found;
	Edge_info *e;
	int tmp;

	if (Num_edges >= Max_edges)	{
		// GET JOHN! (And tell him that his
		// MAX_EDGES_FROM_VERTS formula is hosed.)
		// If he's not around, save the mine,
		// and send him  mail so he can look
		// at the mine later. Don't modify it.
		// This is important if this happens.
		Int3 ();		// LOOK ABOVE!!!!!!
		return;
	}

if (va > vb) {
	tmp = va;
	va = vb;
	vb = tmp;
	}
found = AutomapFindEdge (va,vb,&e);

if (found == -1) {
	e->verts[0] = va;
	e->verts[1] = vb;
	e->color = color;
	e->num_faces = 1;
	e->flags = EF_USED | EF_DEFINING;			// Assume a normal line
	e->sides[0] = tSide;
	e->nSegment[0] = nSegment;
	//Edge_used_list[Num_edges] = EDGE_IDX (e);
	if ( EDGE_IDX (e) > Highest_edge_index)
		Highest_edge_index = EDGE_IDX (e);
	Num_edges++;
	} 
else {
	//Assert (e->num_faces < 8);
	if (color != automapColors.walls.nNormal)
		if (color != automapColors.walls.nRevealed)
			e->color = color;
	if (e->num_faces < 4) {
		e->sides[e->num_faces] = tSide;
		e->nSegment[e->num_faces] = nSegment;
		e->num_faces++;
		}
	}
if (grate)
	e->flags |= EF_GRATE;
if (hidden)
	e->flags|=EF_SECRET;		// Mark this as a hidden edge
if (bNoFade)
	e->flags |= EF_NO_FADE;
}

//------------------------------------------------------------------------------

void AddOneUnknownEdge (int va, int vb)	
{
	int found;
	Edge_info *e;
	int tmp;

	if (va > vb)	{
		tmp = va;
		va = vb;
		vb = tmp;
	}

	found = AutomapFindEdge (va,vb,&e);
	if (found != -1)
		e->flags|=EF_FRONTIER;		// Mark as a border edge
}

//------------------------------------------------------------------------------

#ifndef _GAMESEQ_H
extern tObjPosition gameData.multi.playerInit[];
#endif

void AddSegmentEdges (tSegment *seg)
{
	int		 		bIsGrate, bNoFade;
	unsigned int	color;
	int				wn;
	ubyte				sn;
	short				nSegment = SEG_IDX (seg);
	int				bHidden;
	int				ttype, nTrigger;

	for (sn = 0; sn < MAX_SIDES_PER_SEGMENT; sn++) {
		short	vertex_list[4];

bHidden = 0;
bIsGrate = 0;
bNoFade = 0;
color = WHITE_RGBA;
if (seg->children[sn] == -1)
	color = automapColors.walls.nNormal;
switch (gameData.segs.segment2s[nSegment].special)	{
	case SEGMENT_IS_FUELCEN:
		color = GOLD_RGBA;
		break;
	case SEGMENT_IS_SPEEDBOOST:
		color = ORANGE_RGBA;
		break;
	case SEGMENT_IS_BLOCKED:
		color = RGBA_PAL2 (13, 13, 13);
		break;
	case SEGMENT_IS_REPAIRCEN:
		color = RGBA_PAL2 (0, 31, 0);
		break;
	case SEGMENT_IS_CONTROLCEN:
		if (gameData.reactor.bPresent)
			color = RGBA_PAL2 (29, 0, 0);
		break;
	case SEGMENT_IS_ROBOTMAKER:
		color = RGBA_PAL2 (29, 0, 31);
		break;
	case SEGMENT_IS_SKYBOX:
		continue;
	}

if (IS_WALL (wn = WallNumP (seg, sn)))	{
	wall	*wallP = gameData.walls.walls + wn;
	nTrigger = wallP->tTrigger;
	ttype = gameData.trigs.triggers[nTrigger].nType;
	if (ttype==TT_SECRET_EXIT)	{
	 	color = RGBA_PAL2 (29, 0, 31);
		bNoFade=1;
		goto addEdge;
		}
	switch (wallP->nType)	{
		case WALL_DOOR:
			if (wallP->keys == KEY_BLUE) {
				bNoFade = 1;
				color = automapColors.walls.nDoorBlue;
				}
			else if (wallP->keys == KEY_GOLD) {
				bNoFade = 1;
				color = automapColors.walls.nDoorGold;
				}
			else if (wallP->keys == KEY_RED) {
				bNoFade = 1;
				color = automapColors.walls.nDoorRed;
				}
			else if (! (gameData.walls.pAnims[wallP->clip_num].flags & WCF_HIDDEN)) {
				short	connected_seg = seg->children [sn];
				if (connected_seg != -1) {
					short connected_side = FindConnectedSide (seg, &gameData.segs.segments[connected_seg]);
					switch (gameData.walls.walls [WallNumI (connected_seg, connected_side)].keys) {
						case KEY_BLUE:	
							color = automapColors.walls.nDoorBlue;	
							bNoFade = 1; 
							break;
						case KEY_GOLD:	
							color = automapColors.walls.nDoorGold;	
							bNoFade = 1; 
							break;
						case KEY_RED:	
							color = automapColors.walls.nDoorRed;	
							bNoFade = 1; 
							break;
						default:
							color = automapColors.walls.nDoor;
						}
					}
				}
			else {
				color = automapColors.walls.nNormal;
				bHidden = 1;
				}
			break;
		case WALL_CLOSED:
			// Make grates draw properly
			if (WALL_IS_DOORWAY (seg, sn, NULL) & WID_RENDPAST_FLAG)
				bIsGrate = 1;
			else
				bHidden = 1;
			color = automapColors.walls.nNormal;
			break;
		case WALL_BLASTABLE:
			// Hostage doors
			color = automapColors.walls.nDoor;
			break;
		}
	}

if (nSegment == gameData.multi.playerInit[gameData.multi.nLocalPlayer].nSegment)
	color = RGBA_PAL2 (31,0,31);

	if (color != WHITE_RGBA) {
		// If they have a map powerup, draw unvisited areas in dark blue.
		if ((gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_MAP_ALL) && 
				!(gameStates.render.bAllVisited || bAutomapVisited[nSegment]))
			color = automapColors.walls.nRevealed;

addEdge:

		GetSideVerts (vertex_list,nSegment,sn);
		AddOneEdge (vertex_list[0], vertex_list[1], color, sn, nSegment, bHidden, 0, bNoFade);
		AddOneEdge (vertex_list[1], vertex_list[2], color, sn, nSegment, bHidden, 0, bNoFade);
		AddOneEdge (vertex_list[2], vertex_list[3], color, sn, nSegment, bHidden, 0, bNoFade);
		AddOneEdge (vertex_list[3], vertex_list[0], color, sn, nSegment, bHidden, 0, bNoFade);

		if (bIsGrate) {
			AddOneEdge (vertex_list[0], vertex_list[2], color, sn, nSegment, bHidden, 1, bNoFade);
			AddOneEdge (vertex_list[1], vertex_list[3], color, sn, nSegment, bHidden, 1, bNoFade);
			}
		}
	}
}

//------------------------------------------------------------------------------
// Adds all the edges from a tSegment we haven't visited yet.

void AddUnknownSegmentEdges (tSegment *seg)
{
	int sn;
	int nSegment = SEG_IDX (seg);

	for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
		short	vertex_list[4];

		// Only add edges that have no children
		if (seg->children[sn] == -1) {
			GetSideVerts (vertex_list,nSegment,sn);

			AddOneUnknownEdge (vertex_list[0], vertex_list[1]);
			AddOneUnknownEdge (vertex_list[1], vertex_list[2]);
			AddOneUnknownEdge (vertex_list[2], vertex_list[3]);
			AddOneUnknownEdge (vertex_list[3], vertex_list[0]);
		}


	}

}

//------------------------------------------------------------------------------

void AutomapBuildEdgeList ()
{
	int	i,e1,e2,s;
	Edge_info * e;

	amData.bCheat = 0;

	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_MAP_ALL_CHEAT)
		amData.bCheat = 1;		// Damn cheaters...

	// clear edge list
	for (i=0; i<Max_edges; i++) {
		Edges[i].num_faces = 0;
		Edges[i].flags = 0;
	}
	Num_edges = 0;
	Highest_edge_index = -1;

	if (amData.bCheat || (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_MAP_ALL))	{
		// Cheating, add all edges as visited
		for (s=0; s<=gameData.segs.nLastSegment; s++)
			#ifdef EDITOR
			if (gameData.segs.segments[s].nSegment != -1)
			#endif
			{
				AddSegmentEdges (&gameData.segs.segments[s]);
			}
	} else {
		// Not cheating, add visited edges, and then unvisited edges
		for (s=0; s<=gameData.segs.nLastSegment; s++)
			#ifdef EDITOR
			if (gameData.segs.segments[s].nSegment != -1)
			#endif
				if (bAutomapVisited[s]) {
					AddSegmentEdges (&gameData.segs.segments[s]);
				}

		for (s=0; s<=gameData.segs.nLastSegment; s++)
			#ifdef EDITOR
			if (gameData.segs.segments[s].nSegment != -1)
			#endif
				if (!bAutomapVisited[s]) {
					AddUnknownSegmentEdges (&gameData.segs.segments[s]);
				}
	}

	// Find unnecessary lines (These are lines that don't have to be drawn because they have small curvature)
	for (i = 0; i <= Highest_edge_index; i++)	{
		e = Edges + i;
		if (!(e->flags & EF_USED))
			continue;

		for (e1 = 0; e1 < e->num_faces; e1++)	{
			for (e2 = 1; e2 < e->num_faces; e2++)	{
				if ( (e1 != e2) && (e->nSegment[e1] != e->nSegment[e2]))	{
					#ifdef COMPACT_SEGS
					vmsVector v1, v2;
					GetSideNormal (&gameData.segs.segments[e->nSegment[e1]], e->sides[e1], 0, &v1);
					GetSideNormal (&gameData.segs.segments[e->nSegment[e2]], e->sides[e2], 0, &v2);
					if (VmVecDot (&v1,&v2) > (F1_0- (F1_0/10)) )	{
					#else
					if (VmVecDot (&gameData.segs.segments[e->nSegment[e1]].sides[e->sides[e1]].normals[0], &gameData.segs.segments[e->nSegment[e2]].sides[e->sides[e2]].normals[0]) > (F1_0- (F1_0/10)) )	{
					#endif
						e->flags &= (~EF_DEFINING);
						break;
					}
				}
			}
			if (! (e->flags & EF_DEFINING))
				break;
		}
	}
}

//------------------------------------------------------------------------------

static int	 nMarkerIndex=0;
static ubyte bDefiningMarker;
static ubyte nLastMarkerDropped;

void InitMarkerInput ()
 {
	int maxdrop,i;

//find d_free marker slot
if (gameData.app.nGameMode & GM_MULTI)
   maxdrop=MAX_DROP_MULTI;
else
   maxdrop=MAX_DROP_SINGLE;
for (i=0;i<maxdrop;i++)
	if (gameData.marker.tObject[ (gameData.multi.nLocalPlayer*2)+i] == -1)		//found d_free slot!
		break;
if (i == maxdrop) {		//no d_free slot
	if (gameData.app.nGameMode & GM_MULTI)
		i = !nLastMarkerDropped;		//in multi, replace older of two
	else {
		HUDInitMessage (TXT_MARKER_SLOTS);
		return;
		}
	}
//got a d_free slot.  start inputting marker message
gameData.marker.szInput[0]=0;
nMarkerIndex=0;
gameData.marker.nDefiningMsg=1;
bDefiningMarker = i;
}

//------------------------------------------------------------------------------

void MarkerInputMessage (int key)
 {
	switch (key)	{
	case KEY_F8:
	case KEY_ESC:
		gameData.marker.nDefiningMsg = 0;
		GameFlushInputs ();
		break;
	case KEY_LEFT:
	case KEY_BACKSP:
	case KEY_PAD4:
		if (nMarkerIndex > 0)
			nMarkerIndex--;
		gameData.marker.szInput[nMarkerIndex] = 0;
		break;
	case KEY_ENTER:
		strcpy (gameData.marker.szMessage[ (gameData.multi.nLocalPlayer*2)+bDefiningMarker],gameData.marker.szInput);
		if (gameData.app.nGameMode & GM_MULTI)
		 strcpy (gameData.marker.nOwner[ (gameData.multi.nLocalPlayer*2)+bDefiningMarker],gameData.multi.players[gameData.multi.nLocalPlayer].callsign);
		DropMarker (bDefiningMarker);
		nLastMarkerDropped = bDefiningMarker;
		GameFlushInputs ();
		gameData.marker.nDefiningMsg = 0;
		break;
	default:
		if (key > 0)
		 {
		  int ascii = KeyToASCII (key);
		  if ((ascii < 255))
		    if (nMarkerIndex < 38)
		      {
			gameData.marker.szInput[nMarkerIndex++] = ascii;
			gameData.marker.szInput[nMarkerIndex] = 0;
		      }
		 }
		break;

	}
 }

//------------------------------------------------------------------------------
//eof
