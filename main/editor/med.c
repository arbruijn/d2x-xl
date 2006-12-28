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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvs/cvsroot/d2x/main/editor/med.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001/10/25 02:27:17 $
 *
 * Editor loop for Inferno
 *
 * $Log: med.c,v $
 * Revision 1.1  2001/10/25 02:27:17  bradleyb
 * attempt at support for editor, makefile changes, etc
 *
 * Revision 1.1.1.1  1999/06/14 22:03:43  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.3  1995/03/06  18:23:52  john
 * Fixed bug with font screwing up.
 * 
 * Revision 2.2  1995/03/06  16:34:55  john
 * Fixed bug with previous.
 * 
 * Revision 2.1  1995/03/06  15:20:57  john
 * New screen mode method.
 * 
 * Revision 2.0  1995/02/27  11:35:54  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.192  1994/11/30  12:33:55  mike
 * set window clearing mode for editor.
 * 
 * Revision 1.191  1994/11/27  23:17:02  matt
 * Made changes for new con_printf calling convention
 * 
 * Revision 1.190  1994/11/19  00:04:33  john
 * Changed some shorts to ints.
 * 
 * Revision 1.189  1994/11/17  14:47:57  mike
 * validation functions moved from editor to game.
 * 
 * Revision 1.188  1994/11/14  11:41:38  john
 * Fixed bug with editor/game sequencing.
 * 
 * Revision 1.187  1994/11/13  15:36:44  john
 * Changed game sequencing with editor.
 * 
 * Revision 1.186  1994/11/10  16:49:12  matt
 * Don't sort seg list if no segs in list
 * 
 * Revision 1.185  1994/11/08  09:28:39  mike
 * reset ai paths on going to game.
 * 
 * Revision 1.184  1994/10/30  14:13:05  mike
 * rip out repair center stuff.
 * 
 * Revision 1.183  1994/10/27  10:07:06  mike
 * adapt to no inverse table.
 * 
 * Revision 1.182  1994/10/20  12:48:03  matt
 * Replaced old save files (MIN/SAV/HOT) with new LVL files
 * 
 * Revision 1.181  1994/10/13  11:39:22  john
 * Took out network stuff/.
 * 
 * Revision 1.180  1994/10/07  22:21:38  mike
 * Stop Delete-{whatever} from hanging you!
 * 
 * Revision 1.179  1994/10/03  23:39:37  mike
 * Adapt to newer, better, FuelCenActivate function.
 * 
 * Revision 1.178  1994/09/30  00:38:05  mike
 * Shorten diagnostic message erase -- was erasing outside canvas.
 * 
 * Revision 1.177  1994/09/28  17:31:37  mike
 * Add call to check_wall_validity();
 * 
 * Revision 1.176  1994/08/19  10:57:42  mike
 * Fix status message erase bug.
 * 
 * Revision 1.175  1994/08/18  10:48:12  john
 * Cleaned up game sequencing.
 * 
 * Revision 1.174  1994/08/16  18:11:04  yuan
 * Maded C place you in the center of a tSegment.
 * 
 * Revision 1.173  1994/08/10  19:55:05  john
 * Changed font stuff.
 * 
 * Revision 1.172  1994/08/09  16:06:06  john
 * Added the ability to place players.  Made old
 * Player variable be gameData.objs.console.
 * 
 * Revision 1.171  1994/08/04  09:14:11  matt
 * Fixed problem I said I fixed last time
 * 
 * Revision 1.170  1994/08/04  00:27:57  matt
 * When viewing a wall, update the gameData.objs.objects nSegment if moved out of the tSegment
 * 
 * Revision 1.169  1994/08/02  14:18:12  mike
 * Clean up dialog boxes.
 * 
 * Revision 1.168  1994/07/29  15:34:35  mike
 * Kill some mprintfs.
 * 
 * Revision 1.167  1994/07/29  14:56:46  yuan
 * Close centers window, when you go into game.
 * 
 * Revision 1.166  1994/07/28  17:16:20  john
 * MAde editor use Network stuff.
 * 
 * Revision 1.165  1994/07/28  16:59:10  mike
 * gameData.objs.objects containing gameData.objs.objects.
 * 
 * Revision 1.164  1994/07/22  12:37:07  matt
 * Cleaned up editor/game interactions some more.
 * 
 * Revision 1.163  1994/07/21  19:35:11  yuan
 * Fixed #include problem
 * 
 * Revision 1.162  1994/07/21  18:02:09  matt
 * Don't re-init tPlayer stats when going from editor -> game
 * 
 * Revision 1.161  1994/07/21  12:47:53  mike
 * Add tilde key functionality for tObject movement.
 * 
 * Revision 1.160  1994/07/18  10:44:55  mike
 * One-click access to keypads.
 * 
 * Revision 1.159  1994/07/01  18:05:54  john
 * *** empty log message ***
 * 
 * Revision 1.158  1994/07/01  17:57:06  john
 * First version of not-working hostage system
 * 
 * 
 * Revision 1.157  1994/07/01  11:32:29  john
 * *** empty log message ***
 * 
 * Revision 1.156  1994/06/24  17:04:36  john
 * *** empty log message ***
 * 
 * Revision 1.155  1994/06/23  15:53:47  matt
 * Finished hacking in 3d rendering in big window
 * 
 * Revision 1.154  1994/06/21  16:17:54  yuan
 * Init stats when you go to game from editor
 * 
 * Revision 1.153  1994/06/21  12:57:14  yuan
 * Remove center from tSegment function added to menu.
 * 
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

//#define DEMO 1

#define	DIAGNOSTIC_MESSAGE_MAX				90
#define	EDITOR_STATUS_MESSAGE_DURATION	4		//	Shows for 3+..4 seconds

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

//#define INCLUDE_XLISP

#include "inferno.h"
#include "segment.h"
#include "gr.h"
#include "ui.h"
#include "editor.h"
//#include "gamemine.h"
#include "gamesave.h"
#include "gameseg.h"

#include "key.h"
#include "mono.h"
#include "error.h"
#include "kfuncs.h"
#include "macro.h"

#ifdef INCLUDE_XLISP
#include "medlisp.h"
#endif
#include "u_mem.h"
#include "render.h"
#include "game.h"
#include "slew.h"
#include "kdefs.h"
#include "func.h"
#include "textures.h"
#include "screens.h"
#include "texmap.h"
#include "object.h"
#include "effects.h"
#include "wall.h"
#include "info.h"
#include "ai.h"

#include "texpage.h"		// Textue selection paging stuff
#include "objpage.h"		// Object selection paging stuff

#include "medmisc.h"
#include "meddraw.h"
#include "medsel.h"
#include "medrobot.h"
#include "medwall.h"
#include "eswitch.h"
#include "ehostage.h"
#include "centers.h"

#include "fuelcen.h"
#include "gameseq.h"
#include "newmenu.h"

//#define _MARK_ON 1
//#include <wsample.h>		//should come after inferno.h to get mark setting //Not included here.

#define COMPRESS_INTERVAL	5			// seconds

//char *undo_status[128];

int initializing;

//these are instances of canvases, pointed to by variables below
grs_canvas _canv_editor_game;		//the game on the editor screen

//these are pointers to our canvases
grs_canvas *Canv_editor;			//the editor screen
grs_canvas *Canv_editor_game=&_canv_editor_game; //the game on the editor screen

grs_canvas *canv_offscreen;		//for off-screen rendering
grs_canvas *Pad_text_canvas;		// Keypad text

grs_font *editor_font=NULL;

//where the editor is looking
vmsVector Ed_view_target={0,0,0};

int gamestate_not_restored = 0;

UI_WINDOW * EditorWindow;

int	Large_view_index = -1;

UI_GADGET_USERBOX * GameViewBox;
UI_GADGET_USERBOX * LargeViewBox;
UI_GADGET_USERBOX * GroupViewBox;

#if ORTHO_VIEWS
UI_GADGET_USERBOX * TopViewBox;
UI_GADGET_USERBOX * FrontViewBox;
UI_GADGET_USERBOX * RightViewBox;
#endif

UI_GADGET_ICON * ViewIcon;
UI_GADGET_ICON * AllIcon;
UI_GADGET_ICON * AxesIcon;
UI_GADGET_ICON * ChaseIcon;
UI_GADGET_ICON * OutlineIcon;
UI_GADGET_ICON * LockIcon;
//-NOLIGHTICON- UI_GADGET_ICON * LightIcon;

UI_EVENT * DemoBuffer = NULL;

//grs_canvas * BigCanvas[2];
//int CurrentBigCanvas = 0;
//int BigCanvasFirstTime = 1;

int	Found_seg_index=0;				// Index in Found_segs corresponding to Cursegp


void print_status_bar( char message[DIAGNOSTIC_MESSAGE_MAX] ) {
	int w,h,aw;

	GrSetCurrentCanvas( NULL );
	GrSetCurFont(editor_font);
	GrSetFontColor( CBLACK, CGREY );
	GrGetStringSize( message, &w, &h, &aw );
	GrString( 4, 583, message );
	GrSetFontColor( CBLACK, CWHITE );
	GrSetColor( CGREY );
	GrRect( 4+w, 583, 799, 599 );
}

void print_diagnostic( char message[DIAGNOSTIC_MESSAGE_MAX] ) {
	int w,h,aw;

	GrSetCurrentCanvas( NULL );
	GrSetCurFont(editor_font);
	GrSetFontColor( CBLACK, CGREY );
	GrGetStringSize( message, &w, &h, &aw );
	GrString( 4, 583, message );
	GrSetFontColor( CBLACK, CWHITE );
	GrSetColor( CGREY );
	GrRect( 4+w, 583, 799, 599 );
}

static char status_line[DIAGNOSTIC_MESSAGE_MAX];

struct tm	Editor_status_lastTime;

void editor_status( const char *format, ... )
{
	va_list ap;

	va_start(ap, format);
	vsprintf(status_line, format, ap);
	va_end(ap);

	print_status_bar(status_line);

	Editor_status_lastTime = EditorTime_of_day;

}

// 	int  tm_sec;	/* seconds after the minute -- [0,61] */
// 	int  tm_min;	/* minutes after the hour	-- [0,59] */
// 	int  tm_hour;	/* hours after midnight	-- [0,23] */
// 	int  tm_mday;	/* day of the month		-- [1,31] */
// 	int  tm_mon;	/* months since January	-- [0,11] */
// 	int  tm_year;	/* years since 1900				*/
// 	int  tm_wday;	/* days since Sunday		-- [0,6]  */
// 	int  tm_yday;	/* days since January 1	-- [0,365]*/
// 	int  tm_isdst;	/* Daylight Savings Time flag */

void clear_editor_status(void)
{
	int curTime = EditorTime_of_day.tm_hour * 3600 + EditorTime_of_day.tm_min*60 + EditorTime_of_day.tm_sec;
	int eraseTime = Editor_status_lastTime.tm_hour * 3600 + Editor_status_lastTime.tm_min*60 + Editor_status_lastTime.tm_sec + EDITOR_STATUS_MESSAGE_DURATION;

	if (curTime > eraseTime) {
		int	i;
		char	message[DIAGNOSTIC_MESSAGE_MAX];

		for (i=0; i<DIAGNOSTIC_MESSAGE_MAX-1; i++)
			message[i] = ' ';

		message[i] = 0;
		print_status_bar(message);
		Editor_status_lastTime.tm_hour = 99;
	}
}


void diagnostic_message( const char *format, ... )
{
	char diag_line[DIAGNOSTIC_MESSAGE_MAX];

	va_list ap;

	va_start(ap, format);
	vsprintf(diag_line, format, ap);
	va_end(ap);

	editor_status(diag_line);
}


static char sub_status_line[DIAGNOSTIC_MESSAGE_MAX];

void editor_sub_status( const char *format, ... )
{
	int w,h,aw;
	va_list ap;

	va_start(ap, format);
	vsprintf(sub_status_line, format, ap);
	va_end(ap);

	GrSetCurrentCanvas( NULL );
	GrSetCurFont(editor_font);
	GrSetFontColor( BM_XRGB(0,0,31), CGREY );
	GrGetStringSize( sub_status_line, &w, &h, &aw );
	GrString( 500, 583, sub_status_line );
	GrSetFontColor( CBLACK, CWHITE );
	GrSetColor( CGREY );
	GrRect( 500+w, 583, 799, 599 );
}

int DropIntoDebugger()
{
	Int3();
	return 1;
}


#ifdef INCLUDE_XLISP
int CallLisp()
{
	medlisp_go();
	return 1;
}
#endif


int ExitEditor()
{
	if (SafetyCheck())  {
		ModeFlag = 1;
	}
	return 1;
}

int	GotoGameCommon(int mode) {
	StopTime();

//@@	init_player_stats();
//@@
//@@	gameData.multi.playerInit.position.vPos = Player->position.vPos;
//@@	gameData.multi.playerInit.position.mOrient = Player->position.mOrient;
//@@	gameData.multi.playerInit.nSegment = Player->nSegment;	
	
// -- must always save gamesave.sav because the restore-gameData.objs.objects code relies on it
// -- that code could be made smarter and use the original file, if appropriate.
//	if (mine_changed) 
	if (gamestate_not_restored == 0) {
		gamestate_not_restored = 1;
		SaveLevel("GAMESAVE.LVL");
		editor_status("Gamestate saved.\n");
	}

	AIResetAllPaths();

	StartTime();

	ModeFlag = mode;
	return 1;
}

int GotoGameScreen()
{
	return GotoGameCommon(3);
}

int GotoGame()
{
	return GotoGameCommon(2);
}


void ReadLispMacro( FILE * file, char * buffer )
{
//	char c;
//	int size=0;
//	int pcount = 0;
//	char text[100];
//	int i=0;
	
	fscanf( file, " { %s } ", buffer );

/*
	while (1)
	{
		c = text[i++];
		if (pcount > 0 )
			buffer[size++] = c;
		if ( c == '(' ) pcount++;
		if ( c == ')' ) break;
	}
	buffer[size++] = 0;
*/

	return;
}

static int (*KeyFunction[2048])();

void medkey_init()
{
	FILE * keyfile;
	char keypress[100];
	int key;
	int i;	//, size;
	int np;
	char * LispCommand;

	MALLOC( LispCommand, char, DIAGNOSTIC_MESSAGE_MAX );

	for (i=0; i<2048; i++ )
		KeyFunction[i] = NULL;

	keyfile = fopen( "GLOBAL.KEY", "rt" );
	if (keyfile)
	{
		while (fscanf( keyfile, " %s %s ", keypress, LispCommand ) != EOF )
		{
			//ReadLispMacro( keyfile, LispCommand );

			if ( (key=DecodeKeyText( keypress ))!= -1 )
			{
				Assert( key < 2048);
				KeyFunction[key] = func_get( LispCommand, &np );
			} else {
				Error( "Bad key %s in GLOBAL.KEY!", keypress );
			}
		}
		fclose(keyfile);
	}
	d_free( LispCommand );
}

void init_editor()
{
	minit();

	ui_init();

	init_med_functions();	// Must be called before medlisp_init

	ui_pad_read( 0, "segmove.pad" );
	ui_pad_read( 1, "segsize.pad" );
	ui_pad_read( 2, "curve.pad" );
	ui_pad_read( 3, "texture.pad" );
	ui_pad_read( 4, "tObject.pad" );
	ui_pad_read( 5, "objmov.pad" );
	ui_pad_read( 6, "group.pad" );
	ui_pad_read( 7, "lighting.pad" );
	ui_pad_read( 8, "test.pad" );

	medkey_init();

	editor_font = GrInitFont( "pc8x16.fnt" );
	
	menubar_init( "MED.MNU" );

	canv_offscreen = GrCreateCanvas(LVIEW_W,LVIEW_H);
	
	Draw_all_segments = 1;						// Say draw all segments, not just connected ones

	init_autosave();
  
//	atexit(close_editor);

	nClearWindow = 1;	//	do full window clear.
}

int ShowAbout()
{
	MessageBox( -2, -2, 1, 	"INFERNO Mine Editor\n\n"		\
									"Copyright (c) 1993  Parallax Software Corp.",
									"OK");
	return 0;
}

void MovePlayerToSegment(tSegment *seg,int tSide);

int SetPlayerFromCurseg()
{
	MovePlayerToSegment(Cursegp,Curside);
	UpdateFlags |= UF_ED_STATE_CHANGED | UF_GAME_VIEW_CHANGED;
	return 1;
}

int fuelcen_create_from_curseg()
{
	Cursegp->special = SEGMENT_IS_FUELCEN;
	FuelCenActivate( Cursegp, Cursegp->special);
	return 1;
}

int repaircen_create_from_curseg()
{
	Int3();	//	-- no longer supported!
//	Cursegp->special = SEGMENT_IS_REPAIRCEN;
//	FuelCenActivate( Cursegp, Cursegp->special);
	return 1;
}

int controlcen_create_from_curseg()
{
	Cursegp->special = SEGMENT_IS_CONTROLCEN;
	FuelCenActivate( Cursegp, Cursegp->special);
	return 1;
}

int robotmaker_create_from_curseg()
{
	Cursegp->special = SEGMENT_IS_ROBOTMAKER;
	FuelCenActivate( Cursegp, Cursegp->special);
	return 1;
}

int fuelcen_reset_all()	{
	FuelCenReset();
	return 1;
}

int fuelcen_delete_from_curseg() {
	FuelCenDelete( Cursegp );
	return 1;
}


//@@//this routine places the viewer in the center of the tSide opposite to curside,
//@@//with the view toward the center of curside
//@@int SetPlayerFromCursegMinusOne()
//@@{
//@@	vmsVector vp;
//@@
//@@//	int newseg,newside;
//@@//	get_previous_segment(SEG_PTR_2_NUM(Cursegp),Curside,&newseg,&newside);
//@@//	MovePlayerToSegment(&gameData.segs.segments[newseg],newside);
//@@
//@@	med_compute_center_point_on_side(&Player->tObjPosition,Cursegp,sideOpposite[Curside]);
//@@	med_compute_center_point_on_side(&vp,Cursegp,Curside);
//@@	VmVecDec(&vp,&Player->position.vPosition);
//@@	VmVector2Matrix(&Player->position.mOrient,&vp,NULL,NULL);
//@@
//@@	Player->seg = SEG_PTR_2_NUM(Cursegp);
//@@
//@@	UpdateFlags |= UF_GAME_VIEW_CHANGED;
//@@	return 1;
//@@}

//this constant determines how much of the window will be occupied by the
//viewed tSide when SetPlayerFromCursegMinusOne() is called.  It actually
//determine how from from the center of the window the farthest point will be
#define SIDE_VIEW_FRAC (f1_0*8/10)	//80%


void move_player_2_segment_and_rotate(tSegment *seg,int tSide)
{
	vmsVector vp;
	vmsVector	upvec;
        static int edgenum=0;

	COMPUTE_SEGMENT_CENTER(&gameData.objs.console->position.vPos,seg);
	COMPUTE_SIDE_CENTER(&vp,seg,tSide);
	VmVecDec(&vp,&gameData.objs.console->position.vPos);

	VmVecSub(&upvec, &gameData.segs.vertices[Cursegp->verts[sideToVerts[Curside][edgenum%4]]], &gameData.segs.vertices[Cursegp->verts[sideToVerts[Curside][(edgenum+3)%4]]]);
	edgenum++;

	VmVector2Matrix(&gameData.objs.console->position.mOrient,&vp,&upvec,NULL);
//	VmVector2Matrix(&gameData.objs.console->position.mOrient,&vp,NULL,NULL);

	RelinkObject( OBJ_IDX (gameData.objs.console), SEG_PTR_2_NUM(seg) );
	
}

int SetPlayerFromCursegAndRotate()
{
	move_player_2_segment_and_rotate(Cursegp,Curside);
	UpdateFlags |= UF_ED_STATE_CHANGED | UF_GAME_VIEW_CHANGED;
	return 1;
}


//sets the tPlayer facing curseg/curside, normal to face0 of curside, and
//far enough away to see all of curside
int SetPlayerFromCursegMinusOne()
{
	vmsVector view_vec,view_vec2,side_center;
	vmsVector corner_v[4];
	vmsVector	upvec;
	g3sPoint corner_p[4];
	int i;
	fix max,viewDist=f1_0*10;
        static int edgenum=0;
	int newseg;

	view_vec = Cursegp->sides[Curside].normals[0];
	VmVecNegate(&view_vec);

	COMPUTE_SIDE_CENTER(&side_center,Cursegp,Curside);
	VmVecCopyScale(&view_vec2,&view_vec,viewDist);
	VmVecSub(&gameData.objs.console->position.vPos,&side_center,&view_vec2);

	VmVecSub(&upvec, &gameData.segs.vertices[Cursegp->verts[sideToVerts[Curside][edgenum%4]]], &gameData.segs.vertices[Cursegp->verts[sideToVerts[Curside][(edgenum+3)%4]]]);
	edgenum++;

	VmVector2Matrix(&gameData.objs.console->position.mOrient,&view_vec,&upvec,NULL);

	GrSetCurrentCanvas(Canv_editor_game);
	G3StartFrame();
	G3SetViewMatrix(&gameData.objs.console->position.vPos,&gameData.objs.console->position.mOrient,xRenderZoom);

	for (i=max=0;i<4;i++) {
		corner_v[i] = gameData.segs.vertices[Cursegp->verts[sideToVerts[Curside][i]]];
		G3TransformAndEncodePoint(&corner_p[i],&corner_v[i]);
		if (labs(corner_p[i].p3_x) > max) max = labs(corner_p[i].p3_x);
		if (labs(corner_p[i].p3_y) > max) max = labs(corner_p[i].p3_y);
	}

	viewDist = FixMul(viewDist,FixDiv(FixDiv(max,SIDE_VIEW_FRAC),corner_p[0].p3_z);
	VmVecCopyScale(&view_vec2,&view_vec,viewDist);
	VmVecSub(&gameData.objs.console->position.vPos,&side_center,&view_vec2);

	//RelinkObject(OBJ_IDX (gameData.objs.console), SEG_PTR_2_NUM(Cursegp) );
	//UpdateObjectSeg(gameData.objs.console);		//might have backed right out of curseg

	newseg = FindSegByPoint(&gameData.objs.console->position.vPos,SEG_PTR_2_NUM(Cursegp) );
	if (newseg != -1)
		RelinkObject(OBJ_IDX (gameData.objs.console),newseg);

	UpdateFlags |= UF_ED_STATE_CHANGED | UF_GAME_VIEW_CHANGED;
	return 1;
}

int ToggleLighting(void)
{
	char	outstr[80] = "[shift-L] ";
	int	chindex;

	gameStates.render.nLighting++;
	if (gameStates.render.nLighting >= 2)
		gameStates.render.nLighting = 0;

	UpdateFlags |= UF_GAME_VIEW_CHANGED;

	if (last_keypress == KEY_L + KEY_SHIFTED)
		chindex = 0;
	else
		chindex = 10;

	switch (gameStates.render.nLighting) {
		case 0:
			strcpy(&outstr[chindex],"Lighting off.");
			break;
		case 1:
			strcpy(&outstr[chindex],"Static lighting.");
			break;
		case 2:
			strcpy(&outstr[chindex],"Ship lighting.");
			break;
		case 3:
			strcpy(&outstr[chindex],"Ship and static lighting.");
			break;
	}

	diagnostic_message(outstr);

	return gameStates.render.nLighting;
}

void find_concave_segs();

int FindConcaveSegs()
{
	find_concave_segs();

	UpdateFlags |= UF_ED_STATE_CHANGED;		//list may have changed

	return 1;
}

int DosShell()
{
	int ok, w, h;
	grsBitmap * save_bitmap;

	// Save the current graphics state.

	w = grdCurScreen->sc_canvas.cv_bitmap.bm_props.w;
	h = grdCurScreen->sc_canvas.cv_bitmap.bm_props.h;

	save_bitmap = GrCreateBitmap( w, h );
	GrBmUBitBlt(w, h, 0, 0, 0, 0, &(grdCurScreen->sc_canvas.cv_bitmap), save_bitmap );

	GrSetMode( SM_ORIGINAL );

	//printf( "\n\nType EXIT to return to Inferno" );
	//fflush(stdout);

	key_close();
#ifndef __LINUX__
	ok = spawnl(P_WAIT,getenv("COMSPEC"), NULL );
#else
        system("");
#endif
	key_init();

	GrSetMode(grdCurScreen->sc_mode);
	GrBmUBitBlt(w, h, 0, 0, 0, 0, save_bitmap, &(grdCurScreen->sc_canvas.cv_bitmap);
	GrFreeBitmap( save_bitmap );
	//gr_pal_setblock( 0, 256, grdCurScreen->pal );
	//GrUsePaletteTable();

	return 1;

}

int ToggleOutlineMode()
{	int mode;

	mode=ToggleOutlineMode();

        if (mode)
         {
		if (last_keypress != KEY_O)
			diagnostic_message("[O] Outline Mode ON.");
		else
			diagnostic_message("Outline Mode ON.");
         }
	else
         {
		if (last_keypress != KEY_O)
			diagnostic_message("[O] Outline Mode OFF.");
		else
			diagnostic_message("Outline Mode OFF.");
         }

	UpdateFlags |= UF_GAME_VIEW_CHANGED;
	return mode;
}

//@@int do_reset_orient()
//@@{
//@@	slew_reset_orient(SlewObj);
//@@
//@@	UpdateFlags |= UF_GAME_VIEW_CHANGED;
//@@
//@@	* (ubyte *) 0x417 &= ~0x20;
//@@
//@@	return 1;
//@@}

int GameZoomOut()
{
	xRenderZoom = FixMul(xRenderZoom,68985);
	UpdateFlags |= UF_GAME_VIEW_CHANGED;
	return 1;
}

int GameZoomIn()
{
	xRenderZoom = FixMul(xRenderZoom,62259);
	UpdateFlags |= UF_GAME_VIEW_CHANGED;
	return 1;
}


int med_keypad_goto_0()	{	ui_pad_goto(0);	return 0;	}
int med_keypad_goto_1()	{	ui_pad_goto(1);	return 0;	}
int med_keypad_goto_2()	{	ui_pad_goto(2);	return 0;	}
int med_keypad_goto_3()	{	ui_pad_goto(3);	return 0;	}
int med_keypad_goto_4()	{	ui_pad_goto(4);	return 0;	}
int med_keypad_goto_5()	{	ui_pad_goto(5);	return 0;	}
int med_keypad_goto_6()	{	ui_pad_goto(6);	return 0;	}
int med_keypad_goto_7()	{	ui_pad_goto(7);	return 0;	}
int med_keypad_goto_8()	{	ui_pad_goto(8);	return 0;	}

#define	PAD_WIDTH	30
#define	PAD_WIDTH1	(PAD_WIDTH + 7)

int editor_screen_open = 0;

//setup the editors windows, canvases, gadgets, etc.
//called whenever the editor screen is selected
void init_editor_screen()
{	
//	grsBitmap * bmp;

	if (editor_screen_open) return;

	grdCurScreen->sc_canvas.cv_font = editor_font;
	
	//create canvas for game on the editor screen
	initializing = 1;
	GrSetCurrentCanvas(Canv_editor);
	Canv_editor->cv_font = editor_font;
	GrInitSubCanvas(Canv_editor_game,Canv_editor,GAMEVIEW_X,GAMEVIEW_Y,GAMEVIEW_W,GAMEVIEW_H);
	
	//Editor renders into full (320x200) game screen 

	init_info = 1;

	//do other editor screen setup

	// Since the palette might have changed, find some good colors...
	CBLACK = GrFindClosestColor( 1, 1, 1 );
	CGREY = GrFindClosestColor( 28, 28, 28 );
	CWHITE = GrFindClosestColor( 38, 38, 38 );
	CBRIGHT = GrFindClosestColor( 60, 60, 60 );
	CRED = GrFindClosestColor( 63, 0, 0 );

	GrSetCurFont(editor_font);
	GrSetFontColor( CBLACK, CWHITE );

	EditorWindow = ui_open_window( 0 , 0, ED_SCREEN_W, ED_SCREEN_H, WIN_FILLED );

	LargeViewBox	= ui_add_gadget_userbox( EditorWindow,LVIEW_X,LVIEW_Y,LVIEW_W,LVIEW_H);
#if ORTHO_VIEWS
	TopViewBox		= ui_add_gadget_userbox( EditorWindow,TVIEW_X,TVIEW_Y,TVIEW_W,TVIEW_H);
 	FrontViewBox	= ui_add_gadget_userbox( EditorWindow,FVIEW_X,FVIEW_Y,FVIEW_W,FVIEW_H);
	RightViewBox	= ui_add_gadget_userbox( EditorWindow,RVIEW_X,RVIEW_Y,RVIEW_W,RVIEW_H);
#endif
	ui_gadget_calc_keys(EditorWindow);	//make tab work for all windows

	GameViewBox	= ui_add_gadget_userbox( EditorWindow, GAMEVIEW_X, GAMEVIEW_Y, GAMEVIEW_W, GAMEVIEW_H );
//	GroupViewBox	= ui_add_gadget_userbox( EditorWindow,GVIEW_X,GVIEW_Y,GVIEW_W,GVIEW_H);

//	GameViewBox->when_tab = GameViewBox->when_btab = (UI_GADGET *) LargeViewBox;
//	LargeViewBox->when_tab = LargeViewBox->when_btab = (UI_GADGET *) GameViewBox;

//	ui_gadget_calc_keys(EditorWindow);	//make tab work for all windows

	ViewIcon	= ui_add_gadget_icon( EditorWindow, "Lock\nview",	455,25+530, 	40, 22,	KEY_V+KEY_CTRLED, ToggleLockViewToCursegp );
	AllIcon	= ui_add_gadget_icon( EditorWindow, "Draw\nall",	500,25+530,  	40, 22,	KEY_A+KEY_CTRLED, ToggleDrawAllSegments );
	AxesIcon	= ui_add_gadget_icon( EditorWindow, "Coord\naxes",545,25+530,		40, 22,	KEY_D+KEY_CTRLED, ToggleCoordAxes );
//-NOLIGHTICON-	LightIcon	= ui_add_gadget_icon( EditorWindow, "Light\ning",	590,25+530, 	40, 22,	KEY_L+KEY_SHIFTED,ToggleLighting );
	ChaseIcon	= ui_add_gadget_icon( EditorWindow, "Chase\nmode",635,25+530,		40, 22,	-1,				ToggleChaseMode );
	OutlineIcon = ui_add_gadget_icon( EditorWindow, "Out\nline", 	680,25+530,  	40, 22,	KEY_O,			ToggleOutlineMode );
	LockIcon	= ui_add_gadget_icon( EditorWindow, "Lock\nstep", 725,25+530, 	40, 22,	KEY_L,			ToggleLockstep );

	meddraw_init_views(LargeViewBox->canvas);

	//ui_add_gadget_button( EditorWindow, 460, 510, 50, 25, "Quit", ExitEditor );
	//ui_add_gadget_button( EditorWindow, 520, 510, 50, 25, "Lisp", CallLisp );
	//ui_add_gadget_button( EditorWindow, 580, 510, 50, 25, "Mine", MineMenu );
	//ui_add_gadget_button( EditorWindow, 640, 510, 50, 25, "Help", DoHelp );
	//ui_add_gadget_button( EditorWindow, 460, 540, 50, 25, "Macro", MacroMenu );
	//ui_add_gadget_button( EditorWindow, 520, 540, 50, 25, "About", ShowAbout );
	//ui_add_gadget_button( EditorWindow, 640, 540, 50, 25, "Shell", DosShell );

	ui_pad_activate( EditorWindow, PAD_X, PAD_Y );
	Pad_text_canvas = GrCreateSubCanvas(Canv_editor, PAD_X + 250, PAD_Y + 8, 180, 160);
	ui_add_gadget_button( EditorWindow, PAD_X+6, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "<<",  med_keypad_goto_prev );
	ui_add_gadget_button( EditorWindow, PAD_X+PAD_WIDTH1+6, PAD_Y+(30*5)+22, PAD_WIDTH, 20, ">>",  med_keypad_goto_next );

	{	int	i;
		i = 0;	ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "SR",  med_keypad_goto_0 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "SS",  med_keypad_goto_1 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "CF",  med_keypad_goto_2 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "TM",  med_keypad_goto_3 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "OP",  med_keypad_goto_4 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "OR",  med_keypad_goto_5 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "GE",  med_keypad_goto_6 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "LI",  med_keypad_goto_7 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "TT",  med_keypad_goto_8 );
	}

	GrSetCurFont(editor_font);
	menubar_show();

	// INIT TEXTURE STUFF
	texpage_init( EditorWindow );
	objpage_init( EditorWindow );

	EditorWindow->keyboard_focus_gadget = (UI_GADGET *)LargeViewBox;

	canv_offscreen->cv_font = grdCurScreen->sc_canvas.cv_font;
//	BigCanvas[0]->cv_font = grdCurScreen->sc_canvas.cv_font; 
//	BigCanvas[1]->cv_font = grdCurScreen->sc_canvas.cv_font; 
//	BigCanvasFirstTime = 1;

	// Draw status box
	GrSetCurrentCanvas( NULL );
	GrSetColor( CGREY );
	GrRect(STATUS_X,STATUS_Y,STATUS_X+STATUS_W-1,STATUS_Y+STATUS_H-1);			//0, 582, 799, 599 );

	// Draw icon box
	// GrSetCurrentCanvas( NULL );
	//  GrSetColor( CBRIGHT );
	//  GrRect( 528, 2, 798, 22);
	//  GrSetColor( CGREY);
	//  GrRect( 530, 2, 799, 20);

	UpdateFlags = UF_ALL;
	initializing = 0;
	editor_screen_open = 1;
}

//shutdown ui on the editor screen
void close_editor_screen()
{
	if (!editor_screen_open) return;

	editor_screen_open = 0;
	ui_pad_deactivate();
	GrFreeSubCanvas(Pad_text_canvas);

	ui_close_window(EditorWindow);

	close_all_windows();

	// CLOSE TEXTURE STUFF
	texpage_close();
	objpage_close();

	menubar_hide();

}

void med_show_warning(char *s)
{
	grs_canvas *save_canv=grdCurCanv;

	//gr_pal_fade_in(grdCurScreen->pal);	//in case palette is blacked

	MessageBox(-2,-2,1,s,"OK");

	GrSetCurrentCanvas(save_canv);

}

// Returns 1 if OK to trash current mine.
int SafetyCheck()
{
	int x;
			
	if (mine_changed) {
		StopTime();				
		x = ExecMessageBox( "Warning!", 2, "Cancel", "OK", "You are about to lose work." );
		if (x<1) {
			StartTime();
			return 0;
		}
		StartTime();
	}
	return 1;
}

//called at the end of the program
void close_editor() {

	close_autosave();

	menubar_close();
	
	GrCloseFont(editor_font);

	GrFreeCanvas(canv_offscreen); canv_offscreen = NULL;

	return;

}

//variables for find segments process

// ---------------------------------------------------------------------------------------------------
//	Subtract all elements in Found_segs from selected list.
void subtract_found_segments_from_selected_list(void)
{
	int	s,f;

	for (f=0; f<N_found_segs; f++) {
		int	foundnum = Found_segs[f];

		for (s=0; s<N_selected_segs; s++) {
			if (Selected_segs[s] == foundnum) {
				Selected_segs[s] = Selected_segs[N_selected_segs-1];
				N_selected_segs--;
				break;
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------
//	Add all elements in Found_segs to selected list.
void add_found_segments_to_selected_list(void) {
	int	s,f;

	for (f=0; f<N_found_segs; f++) {
		int	foundnum = Found_segs[f];

		for (s=0; s<N_selected_segs; s++)
			if (Selected_segs[s] == foundnum)
				break;

		if (s == N_selected_segs)
			Selected_segs[N_selected_segs++] = foundnum;
	}
}

void gamestate_restore_check() {
	char Message[DIAGNOSTIC_MESSAGE_MAX];
	tObjPosition Save_position;

	if (gamestate_not_restored) {
		sprintf( Message, "Do you wish to restore game state?\n");
	
		if (MessageBox( -2, -2, 2, Message, "Yes", "No" )==1) {

			// Save current position
			Save_position.position.vPos = gameData.objs.console->position.vPos;
			Save_position.position.mOrient = gameData.objs.console->position.mOrient;
			Save_position.nSegment = gameData.objs.console->position.nSegment;

			LoadLevelSub("GAMESAVE.LVL");

			// Restore current position
			if (Save_position.nSegment <= gameData.segs.nLastSegment) {
				gameData.objs.console->position.vPos = Save_position.position.vPos;
				gameData.objs.console->position.mOrient = Save_position.position.mOrient;
				RelinkObject(OBJ_IDX (gameData.objs.console),Save_position.nSegment);
			}

			gamestate_not_restored = 0;
			UpdateFlags |= UF_WORLD_CHANGED;	
			}
		else
			gamestate_not_restored = 1;
		}
}

int RestoreGameState() {
	LoadLevelSub("GAMESAVE.LVL");
	gamestate_not_restored = 0;

	editor_status("Gamestate restored.\n");

	UpdateFlags |= UF_WORLD_CHANGED;
	return 0;
}

extern void check_wall_validity(void);

// ---------------------------------------------------------------------------------------------------
//this function is the editor. called when editor mode selected.  runs until
//game mode or exit selected
void editor(void)
{
	int w,h;
	grsBitmap * savedbitmap;
	editor_view *new_cv;
        static int padnum=0;
	vmsMatrix	MouseRotMat,tempm;
	//@@short camera_objnum;			//a camera for viewing

	init_editor();

	InitCurve();

	RestoreEffectBitmapIcons();

	if (!SetScreenMode(SCREEN_EDITOR))	{
		SetScreenMode(SCREEN_GAME);
		gameStates.app.nFunctionMode=FMODE_GAME;			//force back into game
		return;
	}

	GrSetCurrentCanvas( NULL );
	GrSetCurFont(editor_font);

	//Editor renders into full (320x200) game screen 

	SetWarnFunc(med_show_warning);

	keyd_repeat = 1;		// Allow repeat in editor

//	_MARK_("start of editor");//Nuked to compile -KRB

	ui_mouse_hide();
	ui_reset_idle_seconds();
	gameData.objs.viewer = gameData.objs.console;
	slew_init(gameData.objs.console);
	UpdateFlags = UF_ALL;
	medlisp_update_screen();

	//set the wire-frame window to be the current view
	current_view = &LargeView;

	if (faded_in==0)
	{
		faded_in = 1;
		//gr_pal_fade_in( grdCurScreen->pal );
	}

	w = GameViewBox->canvas->cv_bitmap.bm_props.w;
	h = GameViewBox->canvas->cv_bitmap.bm_props.h;
	
	savedbitmap = GrCreateBitmap(w, h );

	GrBmUBitBlt( w, h, 0, 0, 0, 0, &GameViewBox->canvas->cv_bitmap, savedbitmap );

	GrSetCurrentCanvas( GameViewBox->canvas );
	GrSetCurFont(editor_font);
	//GrSetColor( CBLACK );
	//gr_deaccent_canvas();
	//gr_grey_canvas();
	
	ui_mouse_show();

	GrSetCurFont(editor_font);
	ui_pad_goto(padnum);

	gamestate_restore_check();

	while (gameStates.app.nFunctionMode == FMODE_EDITOR) {

		GrSetCurFont(editor_font);
		info_display_all(EditorWindow);

		ModeFlag = 0;

		// Update the windows

		// Only update if there is no key waiting and we're not in
		// fast play mode.
		if (!key_peekkey()) //-- && (MacroStatus != UI_STATUS_FASTPLAY))
			medlisp_update_screen();

		//do editor stuff
		GrSetCurFont(editor_font);
		ui_mega_process();
		last_keypress &= ~KEY_DEBUGGED;		//	mask off delete key bit which has no function in editor.
		ui_window_do_gadgets(EditorWindow);
		doRobot_window();
		doObject_window();
		do_wall_window();
		do_trigger_window();
		do_hostage_window();
		do_centers_window();
		check_wall_validity();
		Assert(gameData.walls.nWalls>=0);

		if (Gameview_lockstep) {
			static tSegment *old_cursegp=NULL;
			static int old_curside=-1;

			if (old_cursegp!=Cursegp || old_curside!=Curside) {
				SetPlayerFromCursegMinusOne();
				old_cursegp = Cursegp;
				old_curside = Curside;
			}
		}

		if ( ui_get_idle_seconds() > COMPRESS_INTERVAL ) 
			{
			med_compress_mine();
			ui_reset_idle_seconds();
			}
  
//	Commented out because it occupies about 25% of time in twirling the mine.
// Removes some Asserts....
//		med_check_all_vertices();
		clear_editor_status();		// if enough time elapsed, clear editor status message
		TimedAutosave(mine_filename);
		set_editorTime_of_day();
		GrSetCurrentCanvas( GameViewBox->canvas );
		
		// Remove keys used for slew
		switch(last_keypress)
		{
		case KEY_PAD9:
		case KEY_PAD7:
		case KEY_PADPLUS:
		case KEY_PADMINUS:
		case KEY_PAD8:
		case KEY_PAD2:
		case KEY_LBRACKET:
		case KEY_RBRACKET:
		case KEY_PAD1:
		case KEY_PAD3:
		case KEY_PAD6:
		case KEY_PAD4:
			last_keypress = 0;
		}
		if ((last_keypress&0xff)==KEY_LSHIFT) last_keypress=0;
		if ((last_keypress&0xff)==KEY_RSHIFT) last_keypress=0;
		if ((last_keypress&0xff)==KEY_LCTRL) last_keypress=0;
		if ((last_keypress&0xff)==KEY_RCTRL) last_keypress=0;
//		if ((last_keypress&0xff)==KEY_LALT) last_keypress=0;
//		if ((last_keypress&0xff)==KEY_RALT) last_keypress=0;

		GrSetCurFont(editor_font);
		menubar_do( last_keypress );

		//=================== DO FUNCTIONS ====================

		if ( KeyFunction[ last_keypress ] != NULL )	{
			KeyFunction[last_keypress]();
			last_keypress = 0;
		}
		switch (last_keypress)
		{
		case 0:
		case KEY_Z:
		case KEY_G:
		case KEY_LALT:
		case KEY_RALT:
		case KEY_LCTRL:
		case KEY_RCTRL:
		case KEY_LSHIFT:
		case KEY_RSHIFT:
		case KEY_LAPOSTRO:
			break;
		case KEY_SHIFTED + KEY_L:
			ToggleLighting();
			break;
		case KEY_F1:
			render_3d_in_big_window = !render_3d_in_big_window;
			UpdateFlags |= UF_ALL;
			break;			
		default:
			{
			char kdesc[100];
			GetKeyDescription( kdesc, last_keypress );
			editor_status("Error: %s isn't bound to anything.", kdesc  );
			}
		}

		//================================================================

		if (ModeFlag==1)
		{
			close_editor_screen();
			gameStates.app.nFunctionMode=FMODE_EXIT;
				GrFreeBitmap( savedbitmap );
			break;
		}

		if (ModeFlag==2) //-- && MacroStatus==UI_STATUS_NORMAL )
		{
			ui_mouse_hide();
			gameStates.app.nFunctionMode = FMODE_GAME;
			GrBmUBitBlt( w, h, 0, 0, 0, 0, savedbitmap, &GameViewBox->canvas->cv_bitmap);
			GrFreeBitmap( savedbitmap );
			break;
		}

		if (ModeFlag==3) //-- && MacroStatus==UI_STATUS_NORMAL )
		{
//			med_compress_mine();						//will be called anyways before game.
			close_editor_screen();
			gameStates.app.nFunctionMode=FMODE_GAME;			//force back into game
			SetScreenMode(SCREEN_GAME);		//put up game screen
			GrFreeBitmap( savedbitmap );
			break;
		}

//		if (CurWindow->keyboard_focus_gadget == (UI_GADGET *)GameViewBox) current_view=NULL;
//		if (CurWindow->keyboard_focus_gadget == (UI_GADGET *)GroupViewBox) current_view=NULL;

		new_cv = current_view ;

#if ORTHO_VIEWS
		if (CurWindow->keyboard_focus_gadget == (UI_GADGET *)LargeViewBox) new_cv=&LargeView;
		if (CurWindow->keyboard_focus_gadget == (UI_GADGET *)TopViewBox)	new_cv=&TopView;
		if (CurWindow->keyboard_focus_gadget == (UI_GADGET *)FrontViewBox) new_cv=&FrontView;
		if (CurWindow->keyboard_focus_gadget == (UI_GADGET *)RightViewBox) new_cv=&RightView;
#endif
		if (new_cv != current_view ) {
			current_view->ev_changed = 1;
			new_cv->ev_changed = 1;
			current_view = new_cv;
		}

		CalcFrameTime();
		if (slew_frame(0)) {		//do movement and check keys
			UpdateFlags |= UF_GAME_VIEW_CHANGED;
			if (Gameview_lockstep) {
				Cursegp = &gameData.segs.segments[gameData.objs.console->position.nSegment];
				med_create_new_segment_from_cursegp();
				UpdateFlags |= UF_ED_STATE_CHANGED;
			}
		}

		// DO TEXTURE STUFF
		texpage_do();
		objpage_do();


		// Process selection of Cursegp using mouse.
		if (LargeViewBox->mouse_onme && LargeViewBox->b1_clicked && !render_3d_in_big_window) 
		{
			int	xcrd,ycrd;
			xcrd = LargeViewBox->b1_drag_x1;
			ycrd = LargeViewBox->b1_drag_y1;

			find_segments(xcrd,ycrd,LargeViewBox->canvas,&LargeView,Cursegp,Big_depth);	// Sets globals N_found_segs, Found_segs

			// If shift is down, then add tSegment to found list
			if (keyd_pressed[ KEY_LSHIFT ] || keyd_pressed[ KEY_RSHIFT ])
				subtract_found_segments_from_selected_list();
			else
				add_found_segments_to_selected_list();

  			Found_seg_index = 0;	
		
			if (N_found_segs > 0) {
				sort_seg_list(N_found_segs,Found_segs,&gameData.objs.console->position.vPos);
				Cursegp = &gameData.segs.segments[Found_segs[0]];
				med_create_new_segment_from_cursegp();
				if (Lock_view_to_cursegp)
					set_view_target_from_segment(Cursegp);
			}

			UpdateFlags |= UF_ED_STATE_CHANGED | UF_VIEWPOINT_MOVED;
		}

		if (GameViewBox->mouse_onme && GameViewBox->b1_dragging) {
			int	x, y;
			x = GameViewBox->b1_drag_x2;
			y = GameViewBox->b1_drag_y2;

			ui_mouse_hide();
			GrSetCurrentCanvas( GameViewBox->canvas );
			GrSetColor( 15 );
			GrRect( x-1, y-1, x+1, y+1 );
			ui_mouse_show();

		}
		
		// Set current tSegment and tSide by clicking on a polygon in game window.
		//	If ctrl pressed, also assign current texture map to that tSide.
		//if (GameViewBox->mouse_onme && (GameViewBox->b1_done_dragging || GameViewBox->b1_clicked)) {
		if ((GameViewBox->mouse_onme && GameViewBox->b1_clicked && !render_3d_in_big_window) ||
			(LargeViewBox->mouse_onme && LargeViewBox->b1_clicked && render_3d_in_big_window)) {

			int	xcrd,ycrd;
			int seg,tSide,face,poly,tmap;

			if (render_3d_in_big_window) {
				xcrd = LargeViewBox->b1_drag_x1;
				ycrd = LargeViewBox->b1_drag_y1;
			}
			else {
				xcrd = GameViewBox->b1_drag_x1;
				ycrd = GameViewBox->b1_drag_y1;
			}
	
			//Int3();

			if (find_seg_side_face(xcrd,ycrd,&seg,&tSide,&face,&poly)) {


				if (seg<0) {							//found an tObject

					CurObject_index = -seg-1;
					editor_status("Object %d selected.",CurObject_index);

					UpdateFlags |= UF_ED_STATE_CHANGED;
				}
				else {

					//	See if either shift key is down and, if so, assign texture map
					if (keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) {
						Cursegp = &gameData.segs.segments[seg];
						Curside = tSide;
						AssignTexture();
						med_create_new_segment_from_cursegp();
						editor_status("Texture assigned");
					} else if (keyd_pressed[KEY_G])	{
						tmap = gameData.segs.segments[seg].sides[tSide].nBaseTex;
						texpage_grab_current(tmap);
						editor_status( "Texture grabbed." );
					} else if (keyd_pressed[ KEY_LAPOSTRO] ) {
						ui_mouse_hide();
						moveObject_to_mouse_click();
					} else {
						Cursegp = &gameData.segs.segments[seg];
						Curside = tSide;
						med_create_new_segment_from_cursegp();
						editor_status("Curseg and curside selected");
					}
				}

				UpdateFlags |= UF_ED_STATE_CHANGED;
			}
			else 
				editor_status("Click on non-texture ingored");

		}

		// Allow specification of LargeView using mouse
		if (keyd_pressed[ KEY_LCTRL ] || keyd_pressed[ KEY_RCTRL ]) {
			ui_mouse_hide();
			if ( (Mouse.dx!=0) && (Mouse.dy!=0) ) {
				GetMouseRotation( Mouse.dx, Mouse.dy, &MouseRotMat );
				VmMatMul(&tempm,&LargeView.ev_matrix,&MouseRotMat);
				LargeView.ev_matrix = tempm;
				LargeView.ev_changed = 1;
				Large_view_index = -1;			// say not one of the orthogonal views
			}
		} else  {
			ui_mouse_show();
		}

		if ( keyd_pressed[ KEY_Z ] ) {
			ui_mouse_hide();
			if ( Mouse.dy!=0 ) {
				current_view->evDist += Mouse.dy*10000;
				current_view->ev_changed = 1;
			}
		} else {
			ui_mouse_show();
		}

	}

//	_MARK_("end of editor");//Nuked to compile -KRB

	ClearWarnFunc(med_show_warning);

	//kill our camera tObject

	gameData.objs.viewer = gameData.objs.console;					//reset viewer
	//@@ReleaseObject(camera_objnum);

	padnum = ui_pad_get_current();

	close_editor();
	ui_close();


}

void test_fade(void)
{
#if 0
	int	i,c;

	for (c=0; c<256; c++) {
		//printf("%4i: {%3i %3i %3i} ",c,grPalette[3*c],grPalette[3*c+1],grPalette[3*c+2]);
		for (i=0; i<16; i++) {
			int col = grFadeTable[256*i+c];

			//printf("[%3i %3i %3i] ",grPalette[3*col],grPalette[3*col+1],grPalette[3*col+2]);
		}
		//if ( (c%16) == 15)
			//printf("\n");
		//printf("\n");
	}
#endif
}

void dump_stuff(void)
{
#if 0
	int	i,j,prev_color;

	//printf("Palette:\n");

	//for (i=0; i<256; i++)
		//printf("%3i: %2i %2i %2i\n",i,grPalette[3*i],grPalette[3*i+1],grPalette[3*i+2]);

	for (i=0; i<16; i++) {
		//printf("\nFade table #%i\n",i);
		for (j=0; j<256; j++) {
			int	c = grFadeTable[i*256 + j];
			//printf("[%3i %2i %2i %2i] ",c, grPalette[3*c], grPalette[3*c+1], grPalette[3*c+2]);
			//if ((j % 8) == 7)
				//printf("\n");
		}
	}

	//printf("Colors indexed by intensity:\n");
	//printf(". = change from previous, * = no change\n");
	for (j=0; j<256; j++) {
		//printf("%3i: ",j);
		prev_color = -1;
		for (i=0; i<16; i++) {
			int	c = grFadeTable[i*256 + j];
			if (c == prev_color)
				//printf("*");
			else
				//printf(".");
			prev_color = c;
		}
		//printf("  ");
		for (i=0; i<16; i++) {
			int	c = grFadeTable[i*256 + j];
			//printf("[%3i %2i %2i %2i] ", c, grPalette[3*c], grPalette[3*c+1], grPalette[3*c+2]);
		}
		//printf("\n");
	}
#endif
}


int MarkStart(void)
{
	char mystr[30];
	sprintf(mystr,"mark %i start",Mark_count);
//	_MARK_(mystr);//Nuked to compile -KRB

	return 1;
}

int MarkEnd(void)
{
	char mystr[30];
	sprintf(mystr,"mark %i end",Mark_count);
	Mark_count++;
//	_MARK_(mystr);//Nuked to compile -KRB

	return 1;
}

