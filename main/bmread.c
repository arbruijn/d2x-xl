/* $Id: bmread.c,v 1.5 2003/10/10 09:36:34 btb Exp $ */
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
 * Routines to parse bitmaps.tbl
 *
 * Old Log:
 * Revision 2.4  1995/03/28  18:05:29  john
 * Fixed it so you don't have to delete pig after changing bitmaps.tbl
 *
 * Revision 2.3  1995/03/07  16:52:03  john
 * Fixed robots not moving without edtiro bug.
 *
 * Revision 2.2  1995/03/06  16:10:20  mike
 * Fix compile errors if building without editor.
 *
 * Revision 2.1  1995/03/02  14:55:40  john
 * Fixed bug with EDITOR never defined.
 *
 * Revision 2.0  1995/02/27  11:33:10  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.1  1995/02/25  14:02:36  john
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "pstypes.h"
#include "inferno.h"
#include "gr.h"
#include "bm.h"
#include "gamepal.h"
#include "u_mem.h"
#include "mono.h"
#include "error.h"
#include "object.h"
#include "vclip.h"
#include "effects.h"
#include "polyobj.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#ifdef NETWORK
#include "multi.h"
#endif

#include "iff.h"
#include "cfile.h"

#include "hostage.h"
#include "powerup.h"
#include "laser.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "gauges.h"
#include "player.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "compbit.h"
#include "args.h"


#include "editor/texpage.h"

#define BM_NONE			-1
#define BM_COCKPIT		 0
#define BM_TEXTURES		 2
#define BM_UNUSED		 	 3
#define BM_VCLIP		 	 4
#define BM_EFFECTS	    5
#define BM_ECLIP 	 		 6
#define BM_WEAPON			 7
#define BM_DEMO	 		 8
#define BM_ROBOTEX	    9
#define BM_WALL_ANIMS	12
#define BM_WCLIP 			13
#define BM_ROBOT			14
#define BM_GAUGES			20
#define BM_GAUGES_HIRES	21

#define MAX_BITMAPS_PER_BRUSH 30

extern player_ship only_player_ship;		// In bm.c

short		gameData.pig.tex.nObjBitmaps=0;
short		N_ObjBitmapPtrs=0;
static int			Num_robot_ais = 0;
int	TmapList[MAX_TEXTURES];
char	Powerup_names[MAX_POWERUP_TYPES][POWERUP_NAME_LENGTH];
char	Robot_names[MAX_ROBOT_TYPES][ROBOT_NAME_LENGTH];

//---------------- Internal variables ---------------------------
static int 			Registered_only = 0;		//	Gets set by ! in column 1.
static int			SuperX = -1;
static int			Installed=0;
static char 		*arg;
static short 		tmap_count = 0;
static short 		texture_count = 0;
static short 		clip_count = 0;
static short 		clip_num;
static short 		sound_num;
static short 		frames;
static double 		time;
static int			hit_sound = -1;
static sbyte 		bm_flag = BM_NONE;
static int 			abm_flag = 0;
static int 			rod_flag = 0;
static short		wall_open_sound, wall_close_sound,wall_explodes,wall_blastable, wall_hidden;
double		vlighting=0;
static int			obj_eclip;
static char 		*dest_bm;		//clip number to play when destroyed
static int			dest_vclip;		//what vclip to play when exploding
static int			dest_eclip;		//what eclip to play when exploding
static fix			dest_size;		//3d size of explosion
static int			crit_clip;		//clip number to play when destroyed
static int			crit_flag;		//flag if this is a destroyed eclip
static int			tmap1_flag;		//flag if this is used as tmap_num (not tmap_num2)
static int			num_sounds=0;

int	linenum;		//line int table currently being parsed

//------------------- Useful macros and variables ---------------
#define REMOVE_EOL(bObjectRendered)		remove_char((bObjectRendered),'\n')
#define REMOVE_COMMENTS(bObjectRendered)	remove_char((bObjectRendered),';')
#define REMOVE_DOTS(bObjectRendered)  	remove_char((bObjectRendered),'.')

#define IFTOK(str) if (!strcmp(arg, str))
char *space = { " \t" };	
//--unused-- char *equal = { "=" };
char *equal_space = { " \t=" };


//	For the sake of LINT, defining prototypes to module'bObjectRendered functions
void bm_read_alias(void);
void bm_read_marker(void);
void bm_read_robot_ai(void);
void bm_read_powerup(int unused_flag);
void bm_read_hostage(void);
void bm_read_robot(void);
void bm_read_weapon(int unused_flag);
void bm_read_reactor(void);
void bm_read_exitmodel(void);
void bm_read_player_ship(void);
void bm_read_some_file(void);
void bm_read_sound(void);
void bm_write_extra_robots(void);
void clear_to_end_of_line(void);
void verify_textures(void);


//----------------------------------------------------------------------
void remove_char( char * bObjectRendered, char c )
{
	char *p;
	p = strchr(bObjectRendered,c);
	if (p) *p = '\0';
}

//---------------------------------------------------------------

int ComputeAvgPixel(grs_bitmap *newBm)
{
	int	row, column, color, size;
	char	*pptr;
	int	total_red, total_green, total_blue;

	pptr = (char *)newBm->bm_texBuf;

	total_red = 0;
	total_green = 0;
	total_blue = 0;

	for (row=0; row<newBm->bm_props.h; row++)
		for (column=0; column<newBm->bm_props.w; column++) {
			color = *pptr++;
			total_red += grPalette[color*3];
			total_green += grPalette[color*3+1];
			total_blue += grPalette[color*3+2];
		}

	size = newBm->bm_props.h * newBm->bm_props.w * 2;
	total_red /= size;
	total_green /= size;
	total_blue /= size;

	return BM_XRGB(total_red, total_green, total_blue);
}

//---------------------------------------------------------------
// Loads a bitmap from either the piggy file, a r64 file, or a
// whatever extension is passed.

bitmap_index bm_load_sub( char * filename )
{
	bitmap_index bitmap_num;
	grs_bitmap * new;
	ubyte newpal[256*3];
	int iff_error;		//reference parm to avoid warning message
	char fname[20];

	bitmap_num.index = 0;

#ifdef SHAREWARE
	if (Registered_only) {
		return bitmap_num;
	}
#endif

	_splitpath(  filename, NULL, NULL, fname, NULL );

	bitmap_num=piggy_find_bitmap( fname );
	if (bitmap_num.index)	{
		return bitmap_num;
	}

	MALLOC( new, grs_bitmap, 1 );
	iff_error = iff_read_bitmap(filename,new,BM_LINEAR,newpal);
	new->bm_handle=0;
	if (iff_error != IFF_NO_ERROR)		{
#if TRACE
		con_printf (1, "File %bObjectRendered - IFF error: %bObjectRendered",filename,iff_errormsg(iff_error));
#endif
		Error("File <%bObjectRendered> - IFF error: %bObjectRendered, line %d",filename,iff_errormsg(iff_error),linenum);
	}

	if ( iff_has_transparency )
		GrRemapBitmapGood( new, newpal, iff_transparent_color, SuperX );
	else
		GrRemapBitmapGood( new, newpal, -1, SuperX );

	new->bm_avgColor = ComputeAvgPixel(new);

	bitmap_num = PiggyRegisterBitmap( new, fname, 0 );
	d_free( new );
	return bitmap_num;
}

extern grs_bitmap bogus_bitmap;
extern ubyte bogus_bitmap_initialized;
extern digi_sound bogus_sound;

void ab_load( char * filename, bitmap_index bmp[], int *nframes )
{
	grs_bitmap * bm[MAX_BITMAPS_PER_BRUSH];
	bitmap_index bi;
	int i;
	int iff_error;		//reference parm to avoid warning message
	ubyte newpal[768];
	char fname[20];
	char tempname[20];

#ifdef SHAREWARE
	if (Registered_only) {
		Assert( bogus_bitmap_initialized != 0 );
#if TRACE
		con_printf ( 0, "Skipping registered-only animation '%bObjectRendered'\n", filename );
#endif
		bmp[0].index = 0;		//index of bogus bitmap==0 (I think)		//&bogus_bitmap;
		*nframes = 1;
		return;
	}
#endif


	_splitpath( filename, NULL, NULL, fname, NULL );
	
	for (i=0; i<MAX_BITMAPS_PER_BRUSH; i++ )	{
		sprintf( tempname, "%bObjectRendered#%d", fname, i );
		bi = piggy_find_bitmap( tempname );
		if ( !bi.index )	
			break;
		bmp[i] = bi;
	}

	if (i) {
		*nframes = i;
		return;
	}

//	Note that last argument passes an address to the array newpal (which is a pointer).
//	type mismatch found using lint, will substitute this line with an adjusted
//	one.  If fatal error, then it can be easily changed.
//	iff_error = iff_read_animbrush(filename,bm,MAX_BITMAPS_PER_BRUSH,nframes,&newpal);
   iff_error = iff_read_animbrush(filename,bm,MAX_BITMAPS_PER_BRUSH,nframes,newpal);
	if (iff_error != IFF_NO_ERROR)	{
#if TRACE
		con_printf (1,"File %bObjectRendered - IFF error: %bObjectRendered",filename,iff_errormsg(iff_error));
#endif
		Error("File <%bObjectRendered> - IFF error: %bObjectRendered, line %d",filename,iff_errormsg(iff_error),linenum);
	}

	for (i=0;i< *nframes; i++)	{
		bitmap_index new_bmp;
		sprintf( tempname, "%bObjectRendered#%d", fname, i );
		if ( iff_has_transparency )
			GrRemapBitmapGood( bm[i], newpal, iff_transparent_color, SuperX );
		else
			GrRemapBitmapGood( bm[i], newpal, -1, SuperX );

		bm[i]->bm_avgColor = ComputeAvgPixel(bm[i]);

		new_bmp = PiggyRegisterBitmap( bm[i], tempname, 0 );
		d_free( bm[i] );
		bmp[i] = new_bmp;
#if TRACE
		if (!i)
			con_printf (CON_DEBUG, "Registering %bObjectRendered in piggy file.", tempname );
		else
			con_printf (CON_DEBUG, ".");
#endif
	}
#if TRACE
	con_printf (CON_DEBUG, "\n");
#endif
}

int ds_load( char * filename )	{
	int i;
	CFILE * cfp;
	digi_sound new;
	char fname[20];
	char rawname[100];

#ifdef SHAREWARE
	if (Registered_only) {
		return 0;	//don't know what I should return here		//&bogus_sound;
	}
#endif

	_splitpath(  filename, NULL, NULL, fname, NULL );
	_makepath( rawname, NULL, NULL,fname, (gameOpts->sound.digiSampleRate==SAMPLE_RATE_22K)?".R22":".RAW" );

	i=piggy_find_sound( fname );
	if (i!=255)	{
		return i;
	}

	cfp = CFOpen( rawname, gameFolders.szDataDir, "rb", 0 );

	if (cfp!=NULL) {
		new.length	= CFLength( cfp );
		MALLOC( new.data, ubyte, new.length );
		CFRead( new.data, 1, new.length, cfp );
		CFClose(cfp);
	} else {
#if TRACE
		con_printf (1, "Warning: Couldn't find '%bObjectRendered'\n", filename );
#endif
		return 255;
	}
	i = piggy_register_sound( &new, fname, 0 );
	return i;
}

//parse a double
double get_float()
{
	char *xarg;

	xarg = strtok( NULL, space );
	return atof( xarg );
}

//parse an int
int get_int()
{
	char *xarg;

	xarg = strtok( NULL, space );
	return atoi( xarg );
}

// rotates a byte left one bit, preserving the bit falling off the right
//void
//rotate_left(char *c)
//{
//	int found;
//
//	found = 0;
//	if (*c & 0x80)
//		found = 1;
//	*c = *c << 1;
//	if (found)
//		*c |= 0x01;
//}

//loads a texture and returns the texture num
int get_texture(char *name)
{
	char short_name[FILENAME_LEN];
	int i;

	strcpy(short_name,name);
	REMOVE_DOTS(short_name);
	for (i=0;i<texture_count;i++)
		if (!stricmp(TmapInfo [gameStates.app.bD1Data][i].filename,short_name))
			break;
	if (i==texture_count) {
		Textures [gameStates.app.bD1Data][texture_count] = bm_load_sub(name);
		strcpy( TmapInfo [gameStates.app.bD1Data][texture_count].filename, short_name);
		texture_count++;
		Assert(texture_count < MAX_TEXTURES);
		NumTextures = texture_count;
	}

	return i;
}

#define LINEBUF_SIZE 600

#define DEFAULT_PIG_PALETTE	"groupa.256"

//-----------------------------------------------------------------
// Initializes all bitmaps from BITMAPS.TBL file.
// This is called when the editor is IN.
// If no editor, BMInit() is called.
int bm_init_use_tbl()
{
	CFILE	* InfoFile;
	char	szInput[LINEBUF_SIZE];
	int	i, have_bin_tbl;

	GrUsePaletteTable(DEFAULT_PIG_PALETTE);

	LoadPalette(DEFAULT_PIG_PALETTE,-2,0,0);		//special: tell palette code which pig is loaded

	InitPolygonModels();

	gameData.objs.types.nType[0] = OL_PLAYER;
	gameData.objs.types.nType.nId[0] = 0;
	gameData.objs.types.nCount = 1;

	for (i=0; i<MAX_SOUNDS; i++ )	{
		Sounds [gameStates.app.bD1Data][i] = 255;
		AltSounds[i] = 255;
	}

	for (i=0; i<MAX_TEXTURES; i++ ) {
		TmapInfo [gameStates.app.bD1Data][i].eclip_num = -1;
		TmapInfo [gameStates.app.bD1Data][i].flags = 0;
		TmapInfo [gameStates.app.bD1Data][i].slide_u = TmapInfo [gameStates.app.bD1Data][i].slide_v = 0;
		TmapInfo [gameStates.app.bD1Data][i].destroyed = -1;
	}

	for (i=0;i<MAX_REACTORS;i++)
		Reactors[i].model_num = -1;

	Num_effects = 0;
	for (i=0; i<MAX_EFFECTS; i++ ) {
		//Effects [gameStates.app.bD1Data][i].bm_ptr = (grs_bitmap **) -1;
		Effects [gameStates.app.bD1Data][i].changing_wall_texture = -1;
		Effects [gameStates.app.bD1Data][i].changing_object_texture = -1;
		Effects [gameStates.app.bD1Data][i].segnum = -1;
		Effects [gameStates.app.bD1Data][i].vc.nFrameCount = -1;		//another mark of being unused
	}

	for (i=0;i<MAX_POLYGON_MODELS;i++)
		gameData.models.nDyingModels[i] = gameData.models.nDeadModels[i] = -1;

	Num_vclips = 0;
	for (i=0; i<VCLIP_MAXNUM; i++ )	{
		Vclip [gameStates.app.bD1Data][i].nFrameCount = -1;
		Vclip [gameStates.app.bD1Data][i].flags = 0;
	}

	for (i=0; i<MAX_WALL_ANIMS; i++ )
		WallAnims[i].nFrameCount = -1;
	Num_wall_anims = 0;

	setbuf(stdout, NULL);	// unbuffered output via //printf

	if (Installed)
		return 1;

	Installed = 1;

	PiggyInit();		//don't care about error, since no pig is ok for editor

//	if ( FindArg( "-nobm" ) )	{
//		PiggyReadSounds();
//		return 0;
//	}

	// Open BITMAPS.TBL for reading.
	have_bin_tbl = 0;
	InfoFile = CFOpen( "BITMAPS.TBL", gameFolders.szDataDir, "rb", 0 );
	if (InfoFile == NULL) {
		InfoFile = CFOpen("BITMAPS.BIN", gameFolders.szDataDir, "rb", 0);
		if (InfoFile == NULL)
			Error("Missing BITMAPS.TBL and BITMAPS.BIN file\n");
		have_bin_tbl = 1;
	}
	linenum = 0;
	
	CFSeek( InfoFile, 0L, SEEK_SET);

	while (CFGetS(szInput, LINEBUF_SIZE, InfoFile)) {
		int l;
		char *temp_ptr;

		linenum++;

		if (szInput[0]==' ' || szInput[0]=='\t') {
			char *t;
			for (t=szInput;*t && *t!='\n';t++)
				if (! (*t==' ' || *t=='\t')) {
#if TRACE
					con_printf (1,"Suspicious: line %d of BITMAPS.TBL starts with whitespace\n",linenum);
#endif
					break;
				}
		}

		if (have_bin_tbl) {				// is this a binary tbl file
			for (i = 0; i < strlen(szInput) - 1; i++) {
				szInput [i] = EncodeRotateLeft (EncodeRotateLeft (szInput + i) ^ BITMAP_TBL_XOR);
			}
		} else {
			while (szInput[(l=strlen(szInput))-2]=='\\') {
				if (!isspace(szInput[l-3])) {		//if not space before backslash...
					szInput[l-2] = ' ';				//add one
					l++;
				}
				CFGetS(szInput+l-2,LINEBUF_SIZE-(l-2), InfoFile);
				linenum++;
			}
		}
		REMOVE_EOL(szInput);
		if (strchr(szInput, ';')!=NULL) REMOVE_COMMENTS(szInput);

		if (strlen(szInput) == LINEBUF_SIZE-1)
			Error("Possible line truncation in BITMAPS.TBL on line %d\n",linenum);

		SuperX = -1;

		if ( (temp_ptr=strstr( szInput, "superx=" )) )	{
			SuperX = atoi( &temp_ptr[7] );
			Assert(SuperX == 254);
				//the superx color isn't kept around, so the new piggy regeneration
				//code doesn't know what it is, so it assumes that it'bObjectRendered 254, so
				//this code requires that it be 254
										
		}

		arg = strtok( szInput, space );
		if (arg[0] == '@') {
			arg++;
			Registered_only = 1;
		} else
			Registered_only = 0;

		while (arg != NULL )
			{
			// Check all possible flags and defines.
			if (*arg == '$') bm_flag = BM_NONE; // reset to no flags as default.

			IFTOK("$COCKPIT") 			bm_flag = BM_COCKPIT;
			else IFTOK("$GAUGES")		{bm_flag = BM_GAUGES;   clip_count = 0;}
			else IFTOK("$GAUGES_HIRES"){bm_flag = BM_GAUGES_HIRES; clip_count = 0;}
			else IFTOK("$SOUND") 		bm_read_sound();
			else IFTOK("$DOOR_ANIMS")	bm_flag = BM_WALL_ANIMS;
			else IFTOK("$WALL_ANIMS")	bm_flag = BM_WALL_ANIMS;
			else IFTOK("$TEXTURES") 	bm_flag = BM_TEXTURES;
			else IFTOK("$VCLIP")			{bm_flag = BM_VCLIP;		vlighting = 0;	clip_count = 0;}
			else IFTOK("$ECLIP")			{bm_flag = BM_ECLIP;		vlighting = 0;	clip_count = 0; obj_eclip=0; dest_bm=NULL; dest_vclip=-1; dest_eclip=-1; dest_size=-1; crit_clip=-1; crit_flag=0; sound_num=-1;}
			else IFTOK("$WCLIP")			{bm_flag = BM_WCLIP;		vlighting = 0;	clip_count = 0; wall_explodes = wall_blastable = 0; wall_open_sound=wall_close_sound=-1; tmap1_flag=0; wall_hidden=0;}

			else IFTOK("$EFFECTS")		{bm_flag = BM_EFFECTS;	clip_num = 0;}
			else IFTOK("$ALIAS")			bm_read_alias();

			#ifdef EDITOR
			else IFTOK("!METALS_FLAG")		TextureMetals = texture_count;
			else IFTOK("!LIGHTS_FLAG")		TextureLights = texture_count;
			else IFTOK("!EFFECTS_FLAG")	TextureEffects = texture_count;
			#endif

			else IFTOK("lighting") 			TmapInfo [gameStates.app.bD1Data][texture_count-1].lighting = fl2f(get_float();
			else IFTOK("damage") 			TmapInfo [gameStates.app.bD1Data][texture_count-1].damage = fl2f(get_float();
			else IFTOK("volatile") 			TmapInfo [gameStates.app.bD1Data][texture_count-1].flags |= TMI_VOLATILE;
			else IFTOK("goal_blue")			TmapInfo [gameStates.app.bD1Data][texture_count-1].flags |= TMI_GOAL_BLUE;
			else IFTOK("goal_red")			TmapInfo [gameStates.app.bD1Data][texture_count-1].flags |= TMI_GOAL_RED;
			else IFTOK("water")	 			TmapInfo [gameStates.app.bD1Data][texture_count-1].flags |= TMI_WATER;
			else IFTOK("force_field")		TmapInfo [gameStates.app.bD1Data][texture_count-1].flags |= TMI_FORCE_FIELD;
			else IFTOK("slide")	 			{TmapInfo [gameStates.app.bD1Data][texture_count-1].slide_u = fl2f(get_float())>>8; TmapInfo [gameStates.app.bD1Data][texture_count-1].slide_v = fl2f(get_float())>>8;}
			else IFTOK("destroyed")	 		{int t=texture_count-1; TmapInfo [gameStates.app.bD1Data][t].destroyed = get_texture(strtok( NULL, space );}
			//else IFTOK("Num_effects")		Num_effects = get_int();
			else IFTOK("Num_wall_anims")	Num_wall_anims = get_int();
			else IFTOK("clip_num")			clip_num = get_int();
			else IFTOK("dest_bm")			dest_bm = strtok( NULL, space );
			else IFTOK("dest_vclip")		dest_vclip = get_int();
			else IFTOK("dest_eclip")		dest_eclip = get_int();
			else IFTOK("dest_size")			dest_size = fl2f(get_float();
			else IFTOK("crit_clip")			crit_clip = get_int();
			else IFTOK("crit_flag")			crit_flag = get_int();
			else IFTOK("sound_num") 		sound_num = get_int();
			else IFTOK("frames") 			frames = get_int();
			else IFTOK("time") 				time = get_float();
			else IFTOK("obj_eclip")			obj_eclip = get_int();
			else IFTOK("hit_sound") 		hit_sound = get_int();
			else IFTOK("abm_flag")			abm_flag = get_int();
			else IFTOK("tmap1_flag")		tmap1_flag = get_int();
			else IFTOK("vlighting")			vlighting = get_float();
			else IFTOK("rod_flag")			rod_flag = get_int();
			else IFTOK("superx") 			get_int();
			else IFTOK("open_sound") 		wall_open_sound = get_int();
			else IFTOK("close_sound") 		wall_close_sound = get_int();
			else IFTOK("explodes")	 		wall_explodes = get_int();
			else IFTOK("blastable")	 		wall_blastable = get_int();
			else IFTOK("hidden")	 			wall_hidden = get_int();
			else IFTOK("$ROBOT_AI") 		bm_read_robot_ai();

			else IFTOK("$POWERUP")			{bm_read_powerup(0);		continue;}
			else IFTOK("$POWERUP_UNUSED")	{bm_read_powerup(1);		continue;}
			else IFTOK("$HOSTAGE")			{bm_read_hostage();		continue;}
			else IFTOK("$ROBOT")				{bm_read_robot();			continue;}
			else IFTOK("$WEAPON")			{bm_read_weapon(0);		continue;}
			else IFTOK("$WEAPON_UNUSED")	{bm_read_weapon(1);		continue;}
			else IFTOK("$REACTOR")			{bm_read_reactor();		continue;}
			else IFTOK("$MARKER")			{bm_read_marker();		continue;}
			else IFTOK("$PLAYER_SHIP")		{bm_read_player_ship();	continue;}
			else IFTOK("$EXIT") {
				#ifdef SHAREWARE
					bm_read_exitmodel();	
				#else
					clear_to_end_of_line();
				#endif
				continue;
			}
			else	{		//not a special token, must be a bitmap!

				// Remove any illegal/unwanted spaces and tabs at this point.
				while ((*arg=='\t') || (*arg==' ')) arg++;
				if (*arg == '\0') { break; }	

				//check for '=' in token, indicating error
				if (strchr(arg,'='))
					Error("Unknown token <'%bObjectRendered'> on line %d of BITMAPS.TBL",arg,linenum);

				// Otherwise, 'arg' is apparently a bitmap filename.
				// Load bitmap and process it below:
				bm_read_some_file();

			}

			arg = strtok( NULL, equal_space );
			continue;
      }
	}

	NumTextures = texture_count;
	Num_tmaps = tmap_count;

	Textures [gameStates.app.bD1Data][NumTextures++].index = 0;		//entry for bogus tmap

	CFClose( InfoFile );

	atexit(BMClose);

	Assert(N_robot_types == Num_robot_ais);		//should be one ai info per robot

	#ifdef SHAREWARE
	InitEndLevel();		//this is here so endlevel bitmaps go into pig
	#endif

	verify_textures();

	//check for refereced but unused clip count
	for (i=0; i<MAX_EFFECTS; i++ )
		if (	(
				  (Effects [gameStates.app.bD1Data][i].changing_wall_texture!=-1) ||
				  (Effects [gameStates.app.bD1Data][i].changing_object_texture!=-1)
             )
			 && (Effects [gameStates.app.bD1Data][i].vc.nFrameCount==-1) )
			Error("EClip %d referenced (by polygon object?), but not defined",i);

	#ifndef NDEBUG
	{
		int used;
		for (i=used=0; i<num_sounds; i++ )
			if (Sounds [gameStates.app.bD1Data][i] != 255)
				used++;
#if TRACE
		con_printf (CON_DEBUG,"Sound slots used: %d of %d, highest index %d\n",used,MAX_SOUNDS,num_sounds);
#endif
		//make sure all alt sounds refer to valid main sounds
		for (i=used=0; i<num_sounds; i++ ) {
			int alt = AltSounds[i];
			Assert(alt==0 || alt==-1 || Sounds [gameStates.app.bD1Data][alt]!=255);
		}
	}
	#endif

	PiggyReadSounds();

	#ifdef EDITOR
	PiggyDumpAll();
	#endif

	GrUsePaletteTable(D2_DEFAULT_PALETTE);

	return 0;
}

void verify_textures()
{
	grs_bitmap * bmp;
	int i,j;
	j=0;
	for (i=0; i<Num_tmaps; i++ )	{
		bmp = GameBitmaps + Textures [gameStates.app.bD1Data][i].index;
		if ( (bmp->bm_props.w!=64)||(bmp->bm_props.h!=64)||(bmp->bm_props.rowsize!=64) )	{
#if TRACE
			con_printf (1, "ERROR: Texture '%bObjectRendered' isn't 64x64 !\n", TmapInfo [gameStates.app.bD1Data][i].filename );
#endif
			j++;
		}
	}
	if (j)
		Error("%d textures were not 64x64.  See mono screen for list.",j);

	for (i=0;i<Num_effects;i++)
		if (Effects [gameStates.app.bD1Data][i].changing_object_texture != -1)
			if (GameBitmaps[gameData.pig.tex.objBmIndex[Effects [gameStates.app.bD1Data][i].changing_object_texture].index].bm_props.w!=64 || 
				 GameBitmaps[gameData.pig.tex.objBmIndex[Effects [gameStates.app.bD1Data][i].changing_object_texture].index].bm_props.h!=64)
				Error("Effect %d is used on object, but is not 64x64",i);

}

void bm_read_alias()
{
	char *t;

	Assert(Num_aliases < MAX_ALIASES);

	t = strtok( NULL, space );  
	strncpy(alias_list[Num_aliases].alias_name,t,sizeof(alias_list[Num_aliases].alias_name);
	t = strtok( NULL, space );  
	strncpy(alias_list[Num_aliases].file_name,t,sizeof(alias_list[Num_aliases].file_name);

	Num_aliases++;
}

//--unused-- void dump_all_transparent_textures()
//--unused-- {
//--unused-- 	FILE * fp;
//--unused-- 	int i,j,k;
//--unused-- 	ubyte * p;
//--unused-- 	fp = fopen( "XPARENT.LST", "wt" );
//--unused-- 	for (i=0; i<Num_tmaps; i++ )	{
//--unused-- 		k = 0;
//--unused-- 		p = Textures [gameStates.app.bD1Data][i]->bm_texBuf;
//--unused-- 		for (j=0; j<64*64; j++ )
//--unused-- 			if ( (*p++)==255 ) k++;
//--unused-- 		if ( k )	{
//--unused-- 			fprintf( fp, "'%bObjectRendered' has %d transparent pixels\n", TmapInfo [gameStates.app.bD1Data][i].filename, k );
//--unused-- 		}				
//--unused-- 	}
//--unused-- 	fclose(fp);	
//--unused-- }


void BMClose()
{
	if (Installed)
	{
		Installed=0;
 	}
}

void set_lighting_flag(sbyte *bp)
{
	if (vlighting < 0)
		*bp |= BM_FLAG_NO_LIGHTING;
	else
		*bp &= (0xff ^ BM_FLAG_NO_LIGHTING);
}

void set_texture_name(char *name)
{
	strcpy ( TmapInfo [gameStates.app.bD1Data][texture_count].filename, name );
	REMOVE_DOTS(TmapInfo [gameStates.app.bD1Data][texture_count].filename);
}

void bm_read_eclip()
{
	bitmap_index bitmap;
	int dest_bm_num = 0;

	Assert(clip_num < MAX_EFFECTS);

	if (clip_num+1 > Num_effects)
		Num_effects = clip_num+1;

	Effects [gameStates.app.bD1Data][clip_num].flags = 0;

	//load the dest bitmap first, so that after this routine, the last-loaded
	//texture will be the monitor, so that lighting parameter will be applied
	//to the correct texture
	if (dest_bm) {			//deal with bitmap for blown up clip
		char short_name[FILENAME_LEN];
		int i;
		strcpy(short_name,dest_bm);
		REMOVE_DOTS(short_name);
		for (i=0;i<texture_count;i++)
			if (!stricmp(TmapInfo [gameStates.app.bD1Data][i].filename,short_name))
				break;
		if (i==texture_count) {
			Textures [gameStates.app.bD1Data][texture_count] = bm_load_sub(dest_bm);
			strcpy( TmapInfo [gameStates.app.bD1Data][texture_count].filename, short_name);
			texture_count++;
			Assert(texture_count < MAX_TEXTURES);
			NumTextures = texture_count;
		}
		else if (Textures [gameStates.app.bD1Data][i].index == 0)		//was found, but registered out
			Textures [gameStates.app.bD1Data][i] = bm_load_sub(dest_bm);
		dest_bm_num = i;
	}

	if (!abm_flag)	{
		bitmap = bm_load_sub(arg);

		Effects [gameStates.app.bD1Data][clip_num].vc.xTotalTime = fl2f(time);
		Effects [gameStates.app.bD1Data][clip_num].vc.nFrameCount = frames;
		Effects [gameStates.app.bD1Data][clip_num].vc.xFrameTime = fl2f(time)/frames;

		Assert(clip_count < frames);
		Effects [gameStates.app.bD1Data][clip_num].vc.frames[clip_count] = bitmap;
		set_lighting_flag(&GameBitmaps[bitmap.index].bm_props.flags);

		Assert(!obj_eclip);		//obj eclips for non-abm files not supported!
		Assert(crit_flag==0);

		if (clip_count == 0) {
			Effects [gameStates.app.bD1Data][clip_num].changing_wall_texture = texture_count;
			Assert(tmap_count < MAX_TEXTURES);
	  		TmapList[tmap_count++] = texture_count;
			Textures [gameStates.app.bD1Data][texture_count] = bitmap;
			set_texture_name(arg);
			Assert(texture_count < MAX_TEXTURES);
			texture_count++;
			TmapInfo [gameStates.app.bD1Data][texture_count].eclip_num = clip_num;
			NumTextures = texture_count;
		}

		clip_count++;

	} else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;

		ab_load( arg, bm, &Effects [gameStates.app.bD1Data][clip_num].vc.nFrameCount );

		////printf("EC%d.", clip_num);
		Effects [gameStates.app.bD1Data][clip_num].vc.xTotalTime = fl2f(time);
		Effects [gameStates.app.bD1Data][clip_num].vc.xFrameTime = Effects [gameStates.app.bD1Data][clip_num].vc.xTotalTime/Effects [gameStates.app.bD1Data][clip_num].vc.nFrameCount;

		clip_count = 0;	
		set_lighting_flag( &GameBitmaps[bm[clip_count].index].bm_props.flags);
		Effects [gameStates.app.bD1Data][clip_num].vc.frames[clip_count] = bm[clip_count];

		if (!obj_eclip && !crit_flag) {
			Effects [gameStates.app.bD1Data][clip_num].changing_wall_texture = texture_count;
			Assert(tmap_count < MAX_TEXTURES);
  			TmapList[tmap_count++] = texture_count;
			Textures [gameStates.app.bD1Data][texture_count] = bm[clip_count];
			set_texture_name( arg );
			Assert(texture_count < MAX_TEXTURES);
			TmapInfo [gameStates.app.bD1Data][texture_count].eclip_num = clip_num;
			texture_count++;
			NumTextures = texture_count;
		}

		if (obj_eclip) {

			if (Effects [gameStates.app.bD1Data][clip_num].changing_object_texture == -1) {		//first time referenced
				Effects [gameStates.app.bD1Data][clip_num].changing_object_texture = gameData.pig.tex.nObjBitmaps;		// XChange ObjectBitmaps
				gameData.pig.tex.nObjBitmaps++;
			}

			gameData.pig.tex.objBmIndex[Effects [gameStates.app.bD1Data][clip_num].changing_object_texture] = Effects [gameStates.app.bD1Data][clip_num].vc.frames[0];
		}

		//if for an object, Effects_bm_ptrs set in object load

		for(clip_count=1;clip_count < Effects [gameStates.app.bD1Data][clip_num].vc.nFrameCount; clip_count++) {
			set_lighting_flag( &GameBitmaps[bm[clip_count].index].bm_props.flags);
			Effects [gameStates.app.bD1Data][clip_num].vc.frames[clip_count] = bm[clip_count];
		}

	}

	Effects [gameStates.app.bD1Data][clip_num].crit_clip = crit_clip;
	Effects [gameStates.app.bD1Data][clip_num].sound_num = sound_num;

	if (dest_bm) {			//deal with bitmap for blown up clip

		Effects [gameStates.app.bD1Data][clip_num].dest_bm_num = dest_bm_num;

		if (dest_vclip==-1)
			Error("Desctruction vclip missing on line %d",linenum);
		if (dest_size==-1)
			Error("Desctruction vclip missing on line %d",linenum);

		Effects [gameStates.app.bD1Data][clip_num].dest_vclip = dest_vclip;
		Effects [gameStates.app.bD1Data][clip_num].dest_size = dest_size;

		Effects [gameStates.app.bD1Data][clip_num].dest_eclip = dest_eclip;
	}
	else {
		Effects [gameStates.app.bD1Data][clip_num].dest_bm_num = -1;
		Effects [gameStates.app.bD1Data][clip_num].dest_eclip = -1;
	}

	if (crit_flag)
		Effects [gameStates.app.bD1Data][clip_num].flags |= EF_CRITICAL;
}


void bm_read_gauges()
{
	bitmap_index bitmap;
	int i, num_abm_frames;

	if (!abm_flag)	{
		bitmap = bm_load_sub(arg);
		Assert(clip_count < MAX_GAUGE_BMS);
		Gauges[clip_count] = bitmap;
		clip_count++;
	} else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;
		ab_load( arg, bm, &num_abm_frames );
		for (i=clip_count; i<clip_count+num_abm_frames; i++) {
			Assert(i < MAX_GAUGE_BMS);
			Gauges[i] = bm[i-clip_count];
		}
		clip_count += num_abm_frames;
	}
}

void bm_read_gauges_hires()
{
	bitmap_index bitmap;
	int i, num_abm_frames;

	if (!abm_flag)	{
		bitmap = bm_load_sub(arg);
		Assert(clip_count < MAX_GAUGE_BMS);
		Gauges_hires[clip_count] = bitmap;
		clip_count++;
	} else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;
		ab_load( arg, bm, &num_abm_frames );
		for (i=clip_count; i<clip_count+num_abm_frames; i++) {
			Assert(i < MAX_GAUGE_BMS);
			Gauges_hires[i] = bm[i-clip_count];
		}
		clip_count += num_abm_frames;
	}
}

void bm_read_wclip()
{
	bitmap_index bitmap;
	Assert(clip_num < MAX_WALL_ANIMS);

	WallAnims[clip_num].flags = 0;

	if (wall_explodes)	WallAnims[clip_num].flags |= WCF_EXPLODES;
	if (wall_blastable)	WallAnims[clip_num].flags |= WCF_BLASTABLE;
	if (wall_hidden)		WallAnims[clip_num].flags |= WCF_HIDDEN;
	if (tmap1_flag)		WallAnims[clip_num].flags |= WCF_TMAP1;

	if (!abm_flag)	{
		bitmap = bm_load_sub(arg);
		if ( (WallAnims[clip_num].nFrameCount>-1) && (clip_count==0) )
			Error( "Wall Clip %d is already used!", clip_num );
		WallAnims[clip_num].xTotalTime = fl2f(time);
		WallAnims[clip_num].nFrameCount = frames;
		//WallAnims[clip_num].xFrameTime = fl2f(time)/frames;
		Assert(clip_count < frames);
		WallAnims[clip_num].frames[clip_count++] = texture_count;
		WallAnims[clip_num].open_sound = wall_open_sound;
		WallAnims[clip_num].close_sound = wall_close_sound;
		Textures [gameStates.app.bD1Data][texture_count] = bitmap;
		set_lighting_flag(&GameBitmaps[bitmap.index].bm_props.flags);
		set_texture_name( arg );
		Assert(texture_count < MAX_TEXTURES);
		texture_count++;
		NumTextures = texture_count;
		if (clip_num >= Num_wall_anims) Num_wall_anims = clip_num+1;
	} else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		int nframes;
		if ( (WallAnims[clip_num].nFrameCount>-1)  )
			Error( "AB_Wall clip %d is already used!", clip_num );
		abm_flag = 0;
		ab_load( arg, bm, &nframes );
		WallAnims[clip_num].nFrameCount = nframes;
		////printf("WC");
		WallAnims[clip_num].xTotalTime = fl2f(time);
		//WallAnims[clip_num].xFrameTime = fl2f(time)/nframes;
		WallAnims[clip_num].open_sound = wall_open_sound;
		WallAnims[clip_num].close_sound = wall_close_sound;

		WallAnims[clip_num].close_sound = wall_close_sound;
		strcpy(WallAnims[clip_num].filename, arg);
		REMOVE_DOTS(WallAnims[clip_num].filename);	

		if (clip_num >= Num_wall_anims) Num_wall_anims = clip_num+1;

		set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_props.flags);

		for (clip_count=0;clip_count < WallAnims[clip_num].nFrameCount; clip_count++)	{
			////printf("%d", clip_count);
			Textures [gameStates.app.bD1Data][texture_count] = bm[clip_count];
			set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_props.flags);
			WallAnims[clip_num].frames[clip_count] = texture_count;
			REMOVE_DOTS(arg);
			sprintf( TmapInfo [gameStates.app.bD1Data][texture_count].filename, "%bObjectRendered#%d", arg, clip_count);
			Assert(texture_count < MAX_TEXTURES);
			texture_count++;
			NumTextures = texture_count;
		}
	}
}

void bm_read_vclip()
{
	bitmap_index bi;
	Assert(clip_num < VCLIP_MAXNUM);

	if (clip_num >= Num_vclips)
		Num_vclips = clip_num+1;

	if (!abm_flag)	{
		if ( (Vclip [gameStates.app.bD1Data][clip_num].nFrameCount>-1) && (clip_count==0)  )
			Error( "Vclip %d is already used!", clip_num );
		bi = bm_load_sub(arg);
		Vclip [gameStates.app.bD1Data][clip_num].xTotalTime = fl2f(time);
		Vclip [gameStates.app.bD1Data][clip_num].nFrameCount = frames;
		Vclip [gameStates.app.bD1Data][clip_num].xFrameTime = fl2f(time)/frames;
		Vclip [gameStates.app.bD1Data][clip_num].light_value = fl2f(vlighting);
		Vclip [gameStates.app.bD1Data][clip_num].sound_num = sound_num;
		set_lighting_flag(&GameBitmaps[bi.index].bm_props.flags);
		Assert(clip_count < frames);
		Vclip [gameStates.app.bD1Data][clip_num].frames[clip_count++] = bi;
		if (rod_flag) {
			rod_flag=0;
			Vclip [gameStates.app.bD1Data][clip_num].flags |= VF_ROD;
		}			

	} else	{
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;
		if ( (Vclip [gameStates.app.bD1Data][clip_num].nFrameCount>-1)  )
			Error( "AB_Vclip %d is already used!", clip_num );
		ab_load( arg, bm, &Vclip [gameStates.app.bD1Data][clip_num].nFrameCount );

		if (rod_flag) {
			//int i;
			rod_flag=0;
			Vclip [gameStates.app.bD1Data][clip_num].flags |= VF_ROD;
		}			
		////printf("VC");
		Vclip [gameStates.app.bD1Data][clip_num].xTotalTime = fl2f(time);
		Vclip [gameStates.app.bD1Data][clip_num].xFrameTime = fl2f(time)/Vclip [gameStates.app.bD1Data][clip_num].nFrameCount;
		Vclip [gameStates.app.bD1Data][clip_num].light_value = fl2f(vlighting);
		Vclip [gameStates.app.bD1Data][clip_num].sound_num = sound_num;
		set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_props.flags);

		for (clip_count=0;clip_count < Vclip [gameStates.app.bD1Data][clip_num].nFrameCount; clip_count++) {
			////printf("%d", clip_count);
			set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_props.flags);
			Vclip [gameStates.app.bD1Data][clip_num].frames[clip_count] = bm[clip_count];
		}
	}
}

// ------------------------------------------------------------------------------
void get4fix(fix *fixp)
{
	char	*curtext;
	int	i;

	for (i=0; i<NDL; i++) {
		curtext = strtok(NULL, space);
		fixp[i] = fl2f(atof(curtext);
	}
}

// ------------------------------------------------------------------------------
void get4byte(sbyte *bytep)
{
	char	*curtext;
	int	i;

	for (i=0; i<NDL; i++) {
		curtext = strtok(NULL, space);
		bytep[i] = atoi(curtext);
	}
}

// ------------------------------------------------------------------------------
//	Convert field of view from an angle in 0..360 to cosine.
void adjust_field_of_view(fix *fovp)
{
	int		i;
	fixang	tt;
	double		ff;
	fix		temp;

	for (i=0; i<NDL; i++) {
		ff = - f2fl(fovp[i]);
		if (ff > 179) {
#if TRACE
			con_printf (1, "Warning: Bogus field of view (%7.3f).  Must be in 0..179.\n", ff);
#endif
			ff = 179;
		}
		ff = ff/360;
		tt = fl2f(ff);
		fix_sincos(tt, &temp, &fovp[i]);
	}
}

void clear_to_end_of_line(void)
{
	arg = strtok( NULL, space );
	while (arg != NULL)
		arg = strtok( NULL, space );
}

void bm_read_sound()
{
	int sound_num;
	int alt_sound_num;

	sound_num = get_int();
	alt_sound_num = get_int();

	if ( sound_num>=MAX_SOUNDS )
		Error( "Too many sound files.\n" );

	if (sound_num >= num_sounds)
		num_sounds = sound_num+1;

	if (Sounds [gameStates.app.bD1Data][sound_num] != 255)
		Error("Sound num %d already used, bitmaps.tbl, line %d\n",sound_num,linenum);

	arg = strtok(NULL, space);

	Sounds [gameStates.app.bD1Data][sound_num] = ds_load(arg);

	if ( alt_sound_num == 0 )
		AltSounds[sound_num] = sound_num;
	else if (alt_sound_num < 0 )
		AltSounds[sound_num] = 255;
	else
		AltSounds[sound_num] = alt_sound_num;

	if (Sounds [gameStates.app.bD1Data][sound_num] == 255)
		Error("Can't load soundfile <%bObjectRendered>",arg);
}

// ------------------------------------------------------------------------------
void bm_read_robot_ai()	
{
	char			*robotnum_text;
	int			robotnum;
	robot_info	*robptr;

	robotnum_text = strtok(NULL, space);
	robotnum = atoi(robotnum_text);
	Assert(robotnum < MAX_ROBOT_TYPES);
	robptr = &Robot_info [gameStates.app.bD1Data][robotnum];

	Assert(robotnum == Num_robot_ais);		//make sure valid number

#ifdef SHAREWARE
	if (Registered_only) {
		Num_robot_ais++;
		clear_to_end_of_line();
		return;
	}
#endif

	Num_robot_ais++;

	get4fix(robptr->field_of_view);
	get4fix(robptr->firing_wait);
	get4fix(robptr->firing_wait2);
	get4byte(robptr->rapidfire_count);
	get4fix(robptr->turn_time);
//	get4fix(robptr->fire_power);
//	get4fix(robptr->shield);
	get4fix(robptr->max_speed);
	get4fix(robptr->circle_distance);
	get4byte(robptr->evade_speed);

	robptr->always_0xabcd	= 0xabcd;

	adjust_field_of_view(robptr->field_of_view);

}

//	----------------------------------------------------------------------------------------------
//this will load a bitmap for a polygon models.  it puts the bitmap into
//the array gameData.pig.tex.objBmIndex[], and also deals with animating bitmaps
//returns a pointer to the bitmap
grs_bitmap *load_polymodel_bitmap(char *name)
{
	Assert(gameData.pig.tex.nObjBitmaps < MAX_OBJ_BITMAPS);

//	Assert( gameData.pig.tex.nObjBitmaps == N_ObjBitmapPtrs );

	if (name[0] == '%') {		//an animating bitmap!
		int eclip_num;

		eclip_num = atoi(name+1);

		if (Effects [gameStates.app.bD1Data][eclip_num].changing_object_texture == -1) {		//first time referenced
			Effects [gameStates.app.bD1Data][eclip_num].changing_object_texture = gameData.pig.tex.nObjBitmaps;
			gameData.pig.tex.pObjBmIndex[N_ObjBitmapPtrs++] = gameData.pig.tex.nObjBitmaps;
			gameData.pig.tex.nObjBitmaps++;
		} else {
			gameData.pig.tex.pObjBmIndex[N_ObjBitmapPtrs++] = Effects [gameStates.app.bD1Data][eclip_num].changing_object_texture;
		}
		Assert(gameData.pig.tex.nObjBitmaps < MAX_OBJ_BITMAPS);
		Assert(N_ObjBitmapPtrs < MAX_OBJ_BITMAPS);
		return NULL;
	}
	else 	{
		gameData.pig.tex.objBmIndex[gameData.pig.tex.nObjBitmaps] = bm_load_sub(name);
		if (GameBitmaps[gameData.pig.tex.objBmIndex[gameData.pig.tex.nObjBitmaps].index].bm_props.w!=64 || GameBitmaps[gameData.pig.tex.objBmIndex[gameData.pig.tex.nObjBitmaps].index].bm_props.h!=64)
			Error("Bitmap <%bObjectRendered> is not 64x64",name);
		gameData.pig.tex.pObjBmIndex[N_ObjBitmapPtrs++] = gameData.pig.tex.nObjBitmaps;
		gameData.pig.tex.nObjBitmaps++;
		Assert(gameData.pig.tex.nObjBitmaps < MAX_OBJ_BITMAPS);
		Assert(N_ObjBitmapPtrs < MAX_OBJ_BITMAPS);
		return &GameBitmaps[gameData.pig.tex.objBmIndex[gameData.pig.tex.nObjBitmaps-1].index];
	}
}

#define MAX_MODEL_VARIANTS	4

// ------------------------------------------------------------------------------
void bm_read_robot()	
{
	char			*model_name[MAX_MODEL_VARIANTS];
	int			n_models,i;
	int			first_bitmap_num[MAX_MODEL_VARIANTS];
	char			*equal_ptr;
	int 			exp1_vclip_num=-1;
	int			exp1_sound_num=-1;
	int 			exp2_vclip_num=-1;
	int			exp2_sound_num=-1;
	fix			lighting = F1_0/2;		// Default
	fix			strength = F1_0*10;		// Default strength
	fix			mass = f1_0*4;
	fix			drag = f1_0/2;
	short 		weapon_type = 0, weapon_type2 = -1;
	int			g,bObjectRendered;
	char			name[ROBOT_NAME_LENGTH];
	int			contains_count=0, contains_id=0, contains_prob=0, contains_type=0, behavior=AIB_NORMAL;
	int			companion = 0, smart_blobs=0, energy_blobs=0, badass=0, energy_drain=0, kamikaze=0, thief=0, pursuit=0, lightcast=0, death_roll=0;
	fix			glow=0, aim=F1_0;
	int			deathroll_sound = SOUND_BOSS_SHARE_DIE;	//default
	int			score_value=1000;
	int			cloak_type=0;		//	Default = this robot does not cloak
	int			attack_type=0;		//	Default = this robot attacks by firing (1=lunge)
	int			boss_flag=0;				//	Default = robot is not a boss.
	int			see_sound = ROBOT_SEE_SOUND_DEFAULT;
	int			attack_sound = ROBOT_ATTACK_SOUND_DEFAULT;
	int			claw_sound = ROBOT_CLAW_SOUND_DEFAULT;
	int			taunt_sound = ROBOT_SEE_SOUND_DEFAULT;
	ubyte flags=0;

	Assert(N_robot_types < MAX_ROBOT_TYPES);

#ifdef SHAREWARE
	if (Registered_only) {
		Robot_info [gameStates.app.bD1Data][N_robot_types].model_num = -1;
		N_robot_types++;
		Assert(N_robot_types < MAX_ROBOT_TYPES);
		gameData.objs.types.nCount++;
		Assert(gameData.objs.types.nCount < MAX_OBJTYPE);
		clear_to_end_of_line();
		return;
	}
#endif

	model_name[0] = strtok( NULL, space );
	first_bitmap_num[0] = N_ObjBitmapPtrs;
	n_models = 1;

	// Process bitmaps
	bm_flag=BM_ROBOT;
	arg = strtok( NULL, space );
	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!stricmp( arg, "exp1_vclip" ))	{
				exp1_vclip_num = atoi(equal_ptr);
			} else if (!stricmp( arg, "exp2_vclip" ))	{
				exp2_vclip_num = atoi(equal_ptr);
			} else if (!stricmp( arg, "exp1_sound" ))	{
				exp1_sound_num = atoi(equal_ptr);
			} else if (!stricmp( arg, "exp2_sound" ))	{
				exp2_sound_num = atoi(equal_ptr);
			} else if (!stricmp( arg, "lighting" ))	{
				lighting = fl2f(atof(equal_ptr);
				if ( (lighting < 0) || (lighting > F1_0 )) {
#if TRACE
					con_printf (1, "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting));
#endif
					Error( "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting);
				}
			} else if (!stricmp( arg, "weapon_type" )) {
				weapon_type = atoi(equal_ptr);
			} else if (!stricmp( arg, "weapon_type2" )) {
				weapon_type2 = atoi(equal_ptr);
			} else if (!stricmp( arg, "strength" )) {
				strength = i2f(atoi(equal_ptr);
			} else if (!stricmp( arg, "mass" )) {
				mass = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "drag" )) {
				drag = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "contains_id" )) {
				contains_id = atoi(equal_ptr);
			} else if (!stricmp( arg, "contains_type" )) {
				contains_type = atoi(equal_ptr);
			} else if (!stricmp( arg, "contains_count" )) {
				contains_count = atoi(equal_ptr);
			} else if (!stricmp( arg, "companion" )) {
				companion = atoi(equal_ptr);
			} else if (!stricmp( arg, "badass" )) {
				badass = atoi(equal_ptr);
			} else if (!stricmp( arg, "lightcast" )) {
				lightcast = atoi(equal_ptr);
			} else if (!stricmp( arg, "glow" )) {
				glow = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "death_roll" )) {
				death_roll = atoi(equal_ptr);
			} else if (!stricmp( arg, "deathroll_sound" )) {
				deathroll_sound = atoi(equal_ptr);
			} else if (!stricmp( arg, "thief" )) {
				thief = atoi(equal_ptr);
			} else if (!stricmp( arg, "kamikaze" )) {
				kamikaze = atoi(equal_ptr);
			} else if (!stricmp( arg, "pursuit" )) {
				pursuit = atoi(equal_ptr);
			} else if (!stricmp( arg, "smart_blobs" )) {
				smart_blobs = atoi(equal_ptr);
			} else if (!stricmp( arg, "energy_blobs" )) {
				energy_blobs = atoi(equal_ptr);
			} else if (!stricmp( arg, "energy_drain" )) {
				energy_drain = atoi(equal_ptr);
			} else if (!stricmp( arg, "contains_prob" )) {
				contains_prob = atoi(equal_ptr);
			} else if (!stricmp( arg, "cloak_type" )) {
				cloak_type = atoi(equal_ptr);
			} else if (!stricmp( arg, "attack_type" )) {
				attack_type = atoi(equal_ptr);
			} else if (!stricmp( arg, "boss" )) {
				boss_flag = atoi(equal_ptr);
			} else if (!stricmp( arg, "score_value" )) {
				score_value = atoi(equal_ptr);
			} else if (!stricmp( arg, "see_sound" )) {
				see_sound = atoi(equal_ptr);
			} else if (!stricmp( arg, "attack_sound" )) {
				attack_sound = atoi(equal_ptr);
			} else if (!stricmp( arg, "claw_sound" )) {
				claw_sound = atoi(equal_ptr);
			} else if (!stricmp( arg, "taunt_sound" )) {
				taunt_sound = atoi(equal_ptr);
			} else if (!stricmp( arg, "aim" )) {
				aim = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "big_radius" )) {
				if (atoi(equal_ptr))
					flags |= RIF_BIG_RADIUS;
			} else if (!stricmp( arg, "behavior" )) {
				if (!stricmp(equal_ptr, "STILL"))
					behavior = AIB_STILL;
				else if (!stricmp(equal_ptr, "NORMAL"))
					behavior = AIB_NORMAL;
				else if (!stricmp(equal_ptr, "BEHIND"))
					behavior = AIB_BEHIND;
				else if (!stricmp(equal_ptr, "RUN_FROM"))
					behavior = AIB_RUN_FROM;
				else if (!stricmp(equal_ptr, "SNIPE"))
					behavior = AIB_SNIPE;
				else if (!stricmp(equal_ptr, "STATION"))
					behavior = AIB_STATION;
				else if (!stricmp(equal_ptr, "FOLLOW"))
					behavior = AIB_FOLLOW;
				else
					Int3();	//	Error.  Illegal behavior type for current robot.
			} else if (!stricmp( arg, "name" )) {
				Assert(strlen(equal_ptr) < ROBOT_NAME_LENGTH);	//	Oops, name too long.
				strcpy(name, &equal_ptr[1]);
				name[strlen(name)-1] = 0;
			} else if (!stricmp( arg, "simple_model" )) {
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
				Assert(n_models < MAX_MODEL_VARIANTS);
			} else {
				Int3();
#if TRACE
				con_printf (1, "Invalid parameter, %bObjectRendered=%bObjectRendered in bitmaps.tbl\n", arg, equal_ptr );
#endif
			}		
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(arg);
		}
		arg = strtok( NULL, space );
	}

	//clear out anim info
	for (g=0;g<MAX_GUNS+1;g++)
		for (bObjectRendered=0;bObjectRendered<N_ANIM_STATES;bObjectRendered++)
			Robot_info [gameStates.app.bD1Data][N_robot_types].anim_states[g][bObjectRendered].n_joints = 0;	//inialize to zero

	first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	for (i=0;i<n_models;i++) {
		int n_textures;
		int model_num,last_model_num=0;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		model_num = LoadPolygonModel(model_name[i],n_textures,first_bitmap_num[i],(i==0)?&Robot_info [gameStates.app.bD1Data][N_robot_types]:NULL);
		Assert (model_num < gameData.models.nPolyModels);
		if (i==0)
			Robot_info [gameStates.app.bD1Data][N_robot_types].model_num = model_num;
		else
			gameData.models.polyModels[last_model_num].simpler_model = model_num+1;

		last_model_num = model_num;
	}

	if ((glow > i2f(15)) || (glow < 0) || (glow != 0 && glow < 0x1000)) {
#if TRACE
		con_printf (CON_DEBUG,"Invalid glow value %x for robot %d\n",glow,N_robot_types);
#endif
		Int3();
	}

	gameData.objs.types.nType[gameData.objs.types.nCount] = OL_ROBOT;
	gameData.objs.types.nType.nId[gameData.objs.types.nCount] = N_robot_types;

	Robot_info [gameStates.app.bD1Data][N_robot_types].exp1_vclip_num = exp1_vclip_num;
	Robot_info [gameStates.app.bD1Data][N_robot_types].exp2_vclip_num = exp2_vclip_num;
	Robot_info [gameStates.app.bD1Data][N_robot_types].exp1_sound_num = exp1_sound_num;
	Robot_info [gameStates.app.bD1Data][N_robot_types].exp2_sound_num = exp2_sound_num;
	Robot_info [gameStates.app.bD1Data][N_robot_types].lighting = lighting;
	Robot_info [gameStates.app.bD1Data][N_robot_types].weapon_type = weapon_type;
	Robot_info [gameStates.app.bD1Data][N_robot_types].weapon_type2 = weapon_type2;
	Robot_info [gameStates.app.bD1Data][N_robot_types].strength = strength;
	Robot_info [gameStates.app.bD1Data][N_robot_types].mass = mass;
	Robot_info [gameStates.app.bD1Data][N_robot_types].drag = drag;
	Robot_info [gameStates.app.bD1Data][N_robot_types].cloak_type = cloak_type;
	Robot_info [gameStates.app.bD1Data][N_robot_types].attack_type = attack_type;
	Robot_info [gameStates.app.bD1Data][N_robot_types].boss_flag = boss_flag;

	Robot_info [gameStates.app.bD1Data][N_robot_types].contains_id = contains_id;
	Robot_info [gameStates.app.bD1Data][N_robot_types].contains_count = contains_count;
	Robot_info [gameStates.app.bD1Data][N_robot_types].contains_prob = contains_prob;
	Robot_info [gameStates.app.bD1Data][N_robot_types].companion = companion;
	Robot_info [gameStates.app.bD1Data][N_robot_types].badass = badass;
	Robot_info [gameStates.app.bD1Data][N_robot_types].lightcast = lightcast;
	Robot_info [gameStates.app.bD1Data][N_robot_types].glow = (glow>>12);		//convert to 4:4
	Robot_info [gameStates.app.bD1Data][N_robot_types].death_roll = death_roll;
	Robot_info [gameStates.app.bD1Data][N_robot_types].deathroll_sound = deathroll_sound;
	Robot_info [gameStates.app.bD1Data][N_robot_types].thief = thief;
	Robot_info [gameStates.app.bD1Data][N_robot_types].flags = flags;
	Robot_info [gameStates.app.bD1Data][N_robot_types].kamikaze = kamikaze;
	Robot_info [gameStates.app.bD1Data][N_robot_types].pursuit = pursuit;
	Robot_info [gameStates.app.bD1Data][N_robot_types].smart_blobs = smart_blobs;
	Robot_info [gameStates.app.bD1Data][N_robot_types].energy_blobs = energy_blobs;
	Robot_info [gameStates.app.bD1Data][N_robot_types].energy_drain = energy_drain;
	Robot_info [gameStates.app.bD1Data][N_robot_types].score_value = score_value;
	Robot_info [gameStates.app.bD1Data][N_robot_types].see_sound = see_sound;
	Robot_info [gameStates.app.bD1Data][N_robot_types].attack_sound = attack_sound;
	Robot_info [gameStates.app.bD1Data][N_robot_types].claw_sound = claw_sound;
	Robot_info [gameStates.app.bD1Data][N_robot_types].taunt_sound = taunt_sound;
	Robot_info [gameStates.app.bD1Data][N_robot_types].behavior = behavior;		//	Default behavior for this robot, if coming out of matcen.
	Robot_info [gameStates.app.bD1Data][N_robot_types].aim = min(f2i(aim*255), 255);		//	how well this robot type can aim.  255=perfect

	if (contains_type)
		Robot_info [gameStates.app.bD1Data][N_robot_types].contains_type = OBJ_ROBOT;
	else
		Robot_info [gameStates.app.bD1Data][N_robot_types].contains_type = OBJ_POWERUP;

	strcpy(Robot_names[N_robot_types], name);

	N_robot_types++;
	gameData.objs.types.nCount++;

	Assert(N_robot_types < MAX_ROBOT_TYPES);
	Assert(gameData.objs.types.nCount < MAX_OBJTYPE);

	bm_flag = BM_NONE;
}

//read a reactor model
void bm_read_reactor()
{
	char *model_name, *model_name_dead=NULL;
	int first_bitmap_num, first_bitmap_num_dead=0, n_normal_bitmaps;
	char *equal_ptr;
	short model_num;
	short explosion_vclip_num = -1;
	short explosion_sound_num = SOUND_ROBOT_DESTROYED;
	fix	lighting = F1_0/2;		// Default
	int type=-1;
	fix strength=0;

	Assert(Num_reactors < MAX_REACTORS);

#ifdef SHAREWARE
	if (Registered_only) {
		Num_reactors++;
		clear_to_end_of_line();
		return;
	}
#endif

	model_name = strtok( NULL, space );

	// Process bitmaps
	bm_flag = BM_NONE;
	arg = strtok( NULL, space );
	first_bitmap_num = N_ObjBitmapPtrs;

	type = OL_CONTROL_CENTER;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			//@@if (!stricmp(arg,"type")) {
			//@@	if (!stricmp(equal_ptr,"controlcen"))
			//@@		type = OL_CONTROL_CENTER;
			//@@	else if (!stricmp(equal_ptr,"clutter"))
			//@@		type = OL_CLUTTER;
			//@@}

			if (!stricmp( arg, "exp_vclip" ))	{
				explosion_vclip_num = atoi(equal_ptr);
			} else if (!stricmp( arg, "dead_pof" ))	{
				model_name_dead = equal_ptr;
				first_bitmap_num_dead=N_ObjBitmapPtrs;
			} else if (!stricmp( arg, "exp_sound" ))	{
				explosion_sound_num = atoi(equal_ptr);
			} else if (!stricmp( arg, "lighting" ))	{
				lighting = fl2f(atof(equal_ptr);
				if ( (lighting < 0) || (lighting > F1_0 )) {
#if TRACE
					con_printf (1, "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting));
#endif
					Error( "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting);
				}
			} else if (!stricmp( arg, "strength" )) {
				strength = fl2f(atof(equal_ptr);
			} else {
				Int3();
#if TRACE
				con_printf (1, "Invalid parameter, %bObjectRendered=%bObjectRendered in bitmaps.tbl\n", arg, equal_ptr );
#endif
			}		
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(arg);
		}
		arg = strtok( NULL, space );
	}

	if ( model_name_dead )
		n_normal_bitmaps = first_bitmap_num_dead-first_bitmap_num;
	else
		n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	model_num = LoadPolygonModel(model_name,n_normal_bitmaps,first_bitmap_num,NULL);

	if ( model_name_dead )
		gameData.models.nDeadModels[model_num]  = LoadPolygonModel(model_name_dead,N_ObjBitmapPtrs-first_bitmap_num_dead,first_bitmap_num_dead,NULL);
	else
		gameData.models.nDeadModels[model_num] = -1;

	if (type == -1)
		Error("No object type specfied for object in BITMAPS.TBL on line %d\n",linenum);

	Reactors[Num_reactors].model_num = model_num;
	Reactors[Num_reactors].n_guns = read_model_guns(model_name,Reactors[Num_reactors].gun_points,Reactors[Num_reactors].gun_dirs,NULL);

	gameData.objs.types.nType[gameData.objs.types.nCount] = type;
	gameData.objs.types.nType.nId[gameData.objs.types.nCount] = Num_reactors;
	gameData.objs.types.nType.nStrength[gameData.objs.types.nCount] = strength;
	
	////printf( "Object type %d is a control center\n", gameData.objs.types.nCount );
	gameData.objs.types.nCount++;
	Assert(gameData.objs.types.nCount < MAX_OBJTYPE);

	Num_reactors++;
}

//read the marker object
void bm_read_marker()
{
	char *model_name;
	int first_bitmap_num, n_normal_bitmaps;
	char *equal_ptr;

	model_name = strtok( NULL, space );

	// Process bitmaps
	bm_flag = BM_NONE;
	arg = strtok( NULL, space );
	first_bitmap_num = N_ObjBitmapPtrs;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
#if TRACE
			con_printf (1, "Invalid parameter, %bObjectRendered=%bObjectRendered in bitmaps.tbl\n", arg, equal_ptr );
#endif
			Int3();

		} else {			// Must be a texture specification...
			load_polymodel_bitmap(arg);
		}
		arg = strtok( NULL, space );
	}

	n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	gameData.models.nMarkerModel = LoadPolygonModel(model_name,n_normal_bitmaps,first_bitmap_num,NULL);
}

#ifdef SHAREWARE
//read the exit model
void bm_read_exitmodel()
{
	char *model_name, *model_name_dead=NULL;
	int first_bitmap_num, first_bitmap_num_dead, n_normal_bitmaps;
	char *equal_ptr;
	short model_num;

	model_name = strtok( NULL, space );

	// Process bitmaps
	bm_flag = BM_NONE;
	arg = strtok( NULL, space );
	first_bitmap_num = N_ObjBitmapPtrs;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!stricmp( arg, "dead_pof" ))	{
				model_name_dead = equal_ptr;
				first_bitmap_num_dead=N_ObjBitmapPtrs;
			} else {
				Int3();
#if TRACE
				con_printf (1, "Invalid parameter, %bObjectRendered=%bObjectRendered in bitmaps.tbl\n", arg, equal_ptr );
#endif
			}		
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(arg);
		}
		arg = strtok( NULL, space );
	}

	if ( model_name_dead )
		n_normal_bitmaps = first_bitmap_num_dead-first_bitmap_num;
	else
		n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	model_num = LoadPolygonModel(model_name,n_normal_bitmaps,first_bitmap_num,NULL);

	if ( model_name_dead )
		gameData.models.nDeadModels[model_num]  = LoadPolygonModel(model_name_dead,N_ObjBitmapPtrs-first_bitmap_num_dead,first_bitmap_num_dead,NULL);
	else
		gameData.models.nDeadModels[model_num] = -1;

//@@	gameData.objs.types.nType[gameData.objs.types.nCount] = type;
//@@	gameData.objs.types.nType.nId[gameData.objs.types.nCount] = model_num;
//@@	gameData.objs.types.nType.nStrength[gameData.objs.types.nCount] = strength;
//@@	
//@@	////printf( "Object type %d is a control center\n", gameData.objs.types.nCount );
//@@	gameData.objs.types.nCount++;
//@@	Assert(gameData.objs.types.nCount < MAX_OBJTYPE);

	gameData.endLevel.exit.nModel = model_num;
	gameData.endLevel.exit.nDestroyedModel = gameData.models.nDeadModels[model_num];

}
#endif

void bm_read_player_ship()
{
	char	*model_name_dying=NULL;
	char	*model_name[MAX_MODEL_VARIANTS];
	int	n_models=0,i;
	int	first_bitmap_num[MAX_MODEL_VARIANTS];
	char *equal_ptr;
	robot_info ri;
	int last_multi_bitmap_num=-1;

	// Process bitmaps
	bm_flag = BM_NONE;

	arg = strtok( NULL, space );

	Player_ship->mass = Player_ship->drag = 0;	//stupid defaults
	Player_ship->expl_vclip_num = -1;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{

			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!stricmp( arg, "model" )) {
				Assert(n_models==0);
				model_name[0] = equal_ptr;
				first_bitmap_num[0] = N_ObjBitmapPtrs;
				n_models = 1;
			} else if (!stricmp( arg, "simple_model" )) {
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
				Assert(n_models < MAX_MODEL_VARIANTS);

				if (gameData.pig.tex.nFirstMultiBitmap!=-1 && last_multi_bitmap_num==-1)
					last_multi_bitmap_num=N_ObjBitmapPtrs;
			}
			else if (!stricmp( arg, "mass" ))
				Player_ship->mass = fl2f(atof(equal_ptr);
			else if (!stricmp( arg, "drag" ))
				Player_ship->drag = fl2f(atof(equal_ptr);
//			else if (!stricmp( arg, "low_thrust" ))
//				Player_ship->low_thrust = fl2f(atof(equal_ptr);
			else if (!stricmp( arg, "max_thrust" ))
				Player_ship->max_thrust = fl2f(atof(equal_ptr);
			else if (!stricmp( arg, "reverse_thrust" ))
				Player_ship->reverse_thrust = fl2f(atof(equal_ptr);
			else if (!stricmp( arg, "brakes" ))
				Player_ship->brakes = fl2f(atof(equal_ptr);
			else if (!stricmp( arg, "wiggle" ))
				Player_ship->wiggle = fl2f(atof(equal_ptr);
			else if (!stricmp( arg, "max_rotthrust" ))
				Player_ship->max_rotthrust = fl2f(atof(equal_ptr);
			else if (!stricmp( arg, "dying_pof" ))
				model_name_dying = equal_ptr;
			else if (!stricmp( arg, "expl_vclip_num" ))
				Player_ship->expl_vclip_num=atoi(equal_ptr);
			else {
				Int3();
#if TRACE
				con_printf (1, "Invalid parameter, %bObjectRendered=%bObjectRendered in bitmaps.tbl\n", arg, equal_ptr );
#endif
			}		
		}
		else if (!stricmp( arg, "multi_textures" )) {

			gameData.pig.tex.nFirstMultiBitmap = N_ObjBitmapPtrs;
			first_bitmap_num[n_models] = N_ObjBitmapPtrs;

		}
		else			// Must be a texture specification...

			load_polymodel_bitmap(arg);

		arg = strtok( NULL, space );
	}

	Assert(model_name != NULL);

	if (gameData.pig.tex.nFirstMultiBitmap!=-1 && last_multi_bitmap_num==-1)
		last_multi_bitmap_num=N_ObjBitmapPtrs;

	if (gameData.pig.tex.nFirstMultiBitmap==-1)
		first_bitmap_num[n_models] = N_ObjBitmapPtrs;

#ifdef NETWORK
	Assert(last_multi_bitmap_num-gameData.pig.tex.nFirstMultiBitmap == (MAX_NUM_NET_PLAYERS-1)*2);
#endif

	for (i=0;i<n_models;i++) {
		int n_textures;
		int model_num,last_model_num=0;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		model_num = LoadPolygonModel(model_name[i],n_textures,first_bitmap_num[i],(i==0)?&ri:NULL);

		if (i==0)
			Player_ship->model_num = model_num;
		else
			gameData.models.polyModels[last_model_num].simpler_model = model_num+1;

		last_model_num = model_num;
	}

	if ( model_name_dying ) {
		Assert(n_models);
		gameData.models.nDyingModels[Player_ship->model_num]  = LoadPolygonModel(model_name_dying,first_bitmap_num[1]-first_bitmap_num[0],first_bitmap_num[0],NULL);
	}

	Assert(ri.n_guns == N_PLAYER_GUNS);

	//calc player gun positions

	{
		polymodel *pm;
		robot_info *r;
		vms_vector pnt;
		int mn;				//submodel number
		int gun_num;
	
		r = &ri;
		pm = &gameData.models.polyModels[Player_ship->model_num];

		for (gun_num=0;gun_num<r->n_guns;gun_num++) {

			pnt = r->gun_points[gun_num];
			mn = r->gun_submodels[gun_num];
		
			//instance up the tree for this gun
			while (mn != 0) {
				vm_vec_add2(&pnt,&pm->submodel_offsets[mn]);
				mn = pm->submodel_parents[mn];
			}

			Player_ship->gun_points[gun_num] = pnt;
		
		}
	}


}

void bm_read_some_file()
{

	switch (bm_flag) {
	case BM_NONE:
		Error("Trying to read bitmap <%bObjectRendered> with bm_flag==BM_NONE on line %d of BITMAPS.TBL",arg,linenum);
		break;
	case BM_COCKPIT:	{
		bitmap_index bitmap;
		bitmap = bm_load_sub(arg);
		Assert( gameData.models.nCockpits < N_COCKPIT_BITMAPS );
		gameData.pig.tex.cockpitBmIndex[gameData.models.nCockpits++] = bitmap;
		//bm_flag = BM_NONE;
		return;
		}
		break;
	case BM_GAUGES:
		bm_read_gauges();
		return;
		break;
	case BM_GAUGES_HIRES:
		bm_read_gauges_hires();
		return;
		break;
	case BM_WEAPON:
		bm_read_weapon(0);
		return;
		break;
	case BM_VCLIP:
		bm_read_vclip();
		return;
		break;					
	case BM_ECLIP:
		bm_read_eclip();
		return;
		break;
	case BM_TEXTURES:			{
		bitmap_index bitmap;
		bitmap = bm_load_sub(arg);
		Assert(tmap_count < MAX_TEXTURES);
  		TmapList[tmap_count++] = texture_count;
		Textures [gameStates.app.bD1Data][texture_count] = bitmap;
		set_texture_name( arg );
		Assert(texture_count < MAX_TEXTURES);
		texture_count++;
		NumTextures = texture_count;
		return;
		}
		break;
	case BM_WCLIP:
		bm_read_wclip();
		return;
		break;
	}

	Error("Trying to read bitmap <%bObjectRendered> with unknown bm_flag <%x> on line %d of BITMAPS.TBL",arg,bm_flag,linenum);
}

// ------------------------------------------------------------------------------
//	If unused_flag is set, then this is just a placeholder.  Don't actually reference vclips or load bbms.
void bm_read_weapon(int unused_flag)
{
	int	i,n;
	int	n_models=0;
	char 	*equal_ptr;
	char	*pof_file_inner=NULL;
	char	*model_name[MAX_MODEL_VARIANTS];
	int	first_bitmap_num[MAX_MODEL_VARIANTS];
	int	lighted;					//flag for whether is a texture is lighted

	Assert(N_weapon_types < MAX_WEAPON_TYPES);

	n = N_weapon_types;
	N_weapon_types++;
	Assert(N_weapon_types <= MAX_WEAPON_TYPES);

	if (unused_flag) {
		clear_to_end_of_line();
		return;
	}

#ifdef SHAREWARE
	if (Registered_only) {
		clear_to_end_of_line();
		return;
	}
#endif

	// Initialize weapon array
	Weapon_info[n].render_type = WEAPON_RENDER_NONE;		// 0=laser, 1=blob, 2=object
	Weapon_info[n].bitmap.index = 0;
	Weapon_info[n].model_num = -1;
	Weapon_info[n].model_num_inner = -1;
	Weapon_info[n].blob_size = 0x1000;									// size of blob
	Weapon_info[n].flash_vclip = -1;
	Weapon_info[n].flash_sound = SOUND_LASER_FIRED;
	Weapon_info[n].flash_size = 0;
	Weapon_info[n].robot_hit_vclip = -1;
	Weapon_info[n].robot_hit_sound = -1;
	Weapon_info[n].wall_hit_vclip = -1;
	Weapon_info[n].wall_hit_sound = -1;
	Weapon_info[n].impact_size = 0;
	for (i=0; i<NDL; i++) {
		Weapon_info[n].strength[i] = F1_0;
		Weapon_info[n].speed[i] = F1_0*10;
	}
	Weapon_info[n].mass = F1_0;
	Weapon_info[n].thrust = 0;
	Weapon_info[n].drag = 0;
	Weapon_info[n].persistent = 0;

	Weapon_info[n].energy_usage = 0;					//	How much fuel is consumed to fire this weapon.
	Weapon_info[n].ammo_usage = 0;					//	How many units of ammunition it uses.
	Weapon_info[n].fire_wait = F1_0/4;				//	Time until this weapon can be fired again.
	Weapon_info[n].fire_count = 1;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.
	Weapon_info[n].damage_radius = 0;				//	Radius of damage for missiles, not lasers.  Does damage to objects within this radius of hit point.
//--01/19/95, mk--	Weapon_info[n].damage_force = 0;					//	Force (movement) due to explosion
	Weapon_info[n].destroyable = 1;					//	Weapons default to destroyable
	Weapon_info[n].matter = 0;							//	Weapons default to not being constructed of matter (they are energy!)
	Weapon_info[n].bounce = 0;							//	Weapons default to not bouncing off walls

	Weapon_info[n].flags = 0;

	Weapon_info[n].lifetime = WEAPON_DEFAULT_LIFETIME;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.

	Weapon_info[n].po_len_to_width_ratio = F1_0*10;

	Weapon_info[n].picture.index = 0;
	Weapon_info[n].hires_picture.index = 0;
	Weapon_info[n].homing_flag = 0;

	Weapon_info[n].flash = 0;
	Weapon_info[n].multi_damage_scale = F1_0;
	Weapon_info[n].afterburner_size = 0;
	Weapon_info[n].children = -1;
	
	// Process arguments
	arg = strtok( NULL, space );

	lighted = 1;			//assume first texture is lighted

	Weapon_info[n].speedvar = 128;

	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!stricmp( arg, "laser_bmp" ))	{
				// Load bitmap with name equal_ptr

				Weapon_info[n].bitmap = bm_load_sub(equal_ptr);		//load_polymodel_bitmap(equal_ptr);
				Weapon_info[n].render_type = WEAPON_RENDER_LASER;

			} else if (!stricmp( arg, "blob_bmp" ))	{
				// Load bitmap with name equal_ptr

				Weapon_info[n].bitmap = bm_load_sub(equal_ptr);		//load_polymodel_bitmap(equal_ptr);
				Weapon_info[n].render_type = WEAPON_RENDER_BLOB;

			} else if (!stricmp( arg, "weapon_vclip" ))	{
				// Set vclip to play for this weapon.
				Weapon_info[n].bitmap.index = 0;
				Weapon_info[n].render_type = WEAPON_RENDER_VCLIP;
				Weapon_info[n].weapon_vclip = atoi(equal_ptr);

			} else if (!stricmp( arg, "none_bmp" )) {
				Weapon_info[n].bitmap = bm_load_sub(equal_ptr);
				Weapon_info[n].render_type = WEAPON_RENDER_NONE;

			} else if (!stricmp( arg, "weapon_pof" ))	{
				// Load pof file
				Assert(n_models==0);
				model_name[0] = equal_ptr;
				first_bitmap_num[0] = N_ObjBitmapPtrs;
				n_models=1;
			} else if (!stricmp( arg, "simple_model" )) {
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
				Assert(n_models < MAX_MODEL_VARIANTS);
			} else if (!stricmp( arg, "weapon_pof_inner" ))	{
				// Load pof file
				pof_file_inner = equal_ptr;
			} else if (!stricmp( arg, "strength" )) {
				for (i=0; i<NDL-1; i++) {
					Weapon_info[n].strength[i] = fl2f(atof(equal_ptr);
					equal_ptr = strtok(NULL, space);
				}
				Weapon_info[n].strength[i] = i2f(atoi(equal_ptr);
			} else if (!stricmp( arg, "mass" )) {
				Weapon_info[n].mass = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "drag" )) {
				Weapon_info[n].drag = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "thrust" )) {
				Weapon_info[n].thrust = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "matter" )) {
				Weapon_info[n].matter = atoi(equal_ptr);
			} else if (!stricmp( arg, "bounce" )) {
				Weapon_info[n].bounce = atoi(equal_ptr);
			} else if (!stricmp( arg, "speed" )) {
				for (i=0; i<NDL-1; i++) {
					Weapon_info[n].speed[i] = i2f(atoi(equal_ptr);
					equal_ptr = strtok(NULL, space);
				}
				Weapon_info[n].speed[i] = i2f(atoi(equal_ptr);
			} else if (!stricmp( arg, "speedvar" ))	{
				Weapon_info[n].speedvar = (atoi(equal_ptr) * 128) / 100;
			} else if (!stricmp( arg, "flash_vclip" ))	{
				Weapon_info[n].flash_vclip = atoi(equal_ptr);
			} else if (!stricmp( arg, "flash_sound" ))	{
				Weapon_info[n].flash_sound = atoi(equal_ptr);
			} else if (!stricmp( arg, "flash_size" ))	{
				Weapon_info[n].flash_size = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "blob_size" ))	{
				Weapon_info[n].blob_size = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "robot_hit_vclip" ))	{
				Weapon_info[n].robot_hit_vclip = atoi(equal_ptr);
			} else if (!stricmp( arg, "robot_hit_sound" ))	{
				Weapon_info[n].robot_hit_sound = atoi(equal_ptr);
			} else if (!stricmp( arg, "wall_hit_vclip" ))	{
				Weapon_info[n].wall_hit_vclip = atoi(equal_ptr);
			} else if (!stricmp( arg, "wall_hit_sound" ))	{
				Weapon_info[n].wall_hit_sound = atoi(equal_ptr);
			} else if (!stricmp( arg, "impact_size" ))	{
				Weapon_info[n].impact_size = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "lighted" ))	{
				lighted = atoi(equal_ptr);
			} else if (!stricmp( arg, "lw_ratio" ))	{
				Weapon_info[n].po_len_to_width_ratio = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "lightcast" ))	{
				Weapon_info[n].light = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "persistent" ))	{
				Weapon_info[n].persistent = atoi(equal_ptr);
			} else if (!stricmp(arg, "energy_usage" )) {
				Weapon_info[n].energy_usage = fl2f(atof(equal_ptr);
			} else if (!stricmp(arg, "ammo_usage" )) {
				Weapon_info[n].ammo_usage = atoi(equal_ptr);
			} else if (!stricmp(arg, "fire_wait" )) {
				Weapon_info[n].fire_wait = fl2f(atof(equal_ptr);
			} else if (!stricmp(arg, "fire_count" )) {
				Weapon_info[n].fire_count = atoi(equal_ptr);
			} else if (!stricmp(arg, "damage_radius" )) {
				Weapon_info[n].damage_radius = fl2f(atof(equal_ptr);
//--01/19/95, mk--			} else if (!stricmp(arg, "damage_force" )) {
//--01/19/95, mk--				Weapon_info[n].damage_force = fl2f(atof(equal_ptr);
			} else if (!stricmp(arg, "lifetime" )) {
				Weapon_info[n].lifetime = fl2f(atof(equal_ptr);
			} else if (!stricmp(arg, "destroyable" )) {
				Weapon_info[n].destroyable = atoi(equal_ptr);
			} else if (!stricmp(arg, "picture" )) {
				Weapon_info[n].picture = bm_load_sub(equal_ptr);
			} else if (!stricmp(arg, "hires_picture" )) {
				Weapon_info[n].hires_picture = bm_load_sub(equal_ptr);
			} else if (!stricmp(arg, "homing" )) {
				Weapon_info[n].homing_flag = !!atoi(equal_ptr);
			} else if (!stricmp(arg, "flash" )) {
				Weapon_info[n].flash = atoi(equal_ptr);
			} else if (!stricmp(arg, "multi_damage_scale" )) {
				Weapon_info[n].multi_damage_scale = fl2f(atof(equal_ptr);
			} else if (!stricmp(arg, "afterburner_size" )) {
				Weapon_info[n].afterburner_size = f2i(16*fl2f(atof(equal_ptr));
			} else if (!stricmp(arg, "children" )) {
				Weapon_info[n].children = atoi(equal_ptr);
			} else if (!stricmp(arg, "placable" )) {
				if (atoi(equal_ptr)) {
					Weapon_info[n].flags |= WIF_PLACABLE;

					Assert(gameData.objs.types.nCount < MAX_OBJTYPE);
					gameData.objs.types.nType[gameData.objs.types.nCount] = OL_WEAPON;
					gameData.objs.types.nType.nId[gameData.objs.types.nCount] = n;
					gameData.objs.types.nCount++;
				}
			} else {
				Int3();
#if TRACE
				con_printf (1, "Invalid parameter, %bObjectRendered=%bObjectRendered in bitmaps.tbl\n", arg, equal_ptr );
#endif
			}		
		} else {			// Must be a texture specification...
			grs_bitmap *bm;

			bm = load_polymodel_bitmap(arg);
			if (! lighted)
				bm->bm_props.flags |= BM_FLAG_NO_LIGHTING;

			lighted = 1;			//default for next bitmap is lighted
		}
		arg = strtok( NULL, space );
	}

	first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	for (i=0;i<n_models;i++) {
		int n_textures;
		int model_num,last_model_num=0;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		model_num = LoadPolygonModel(model_name[i],n_textures,first_bitmap_num[i],NULL);

		if (i==0) {
			Weapon_info[n].render_type = WEAPON_RENDER_POLYMODEL;
			Weapon_info[n].model_num = model_num;
		}
		else
			gameData.models.polyModels[last_model_num].simpler_model = model_num+1;

		last_model_num = model_num;
	}

	if ( pof_file_inner )	{
		Assert(n_models);
		Weapon_info[n].model_num_inner = LoadPolygonModel(pof_file_inner,first_bitmap_num[1]-first_bitmap_num[0],first_bitmap_num[0],NULL);
	}

	if ((WI_ammo_usage (n) == 0) && (WI_energy_usage (n) == 0))
#if TRACE
		con_printf (1, "Warning: Weapon %i has ammo and energy usage of 0.\n", n);
#endif
// -- render type of none is now legal --	Assert( Weapon_info[n].render_type != WEAPON_RENDER_NONE );
}





// ------------------------------------------------------------------------------
#define DEFAULT_POWERUP_SIZE i2f(3)

void bm_read_powerup(int unused_flag)
{
	int n;
	char 	*equal_ptr;

	Assert(N_powerup_types < MAX_POWERUP_TYPES);

	n = N_powerup_types;
	N_powerup_types++;

	if (unused_flag) {
		clear_to_end_of_line();
		return;
	}

	// Initialize powerup array
	Powerup_info[n].light = F1_0/3;		//	Default lighting value.
	Powerup_info[n].nClipIndex = -1;
	Powerup_info[n].hit_sound = -1;
	Powerup_info[n].size = DEFAULT_POWERUP_SIZE;
	Powerup_names[n][0] = 0;

	// Process arguments
	arg = strtok( NULL, space );

	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!stricmp( arg, "vclip_num" ))	{
				Powerup_info[n].nClipIndex = atoi(equal_ptr);
			} else if (!stricmp( arg, "light" ))	{
				Powerup_info[n].light = fl2f(atof(equal_ptr);
			} else if (!stricmp( arg, "hit_sound" ))	{
				Powerup_info[n].hit_sound = atoi(equal_ptr);
			} else if (!stricmp( arg, "name" )) {
				Assert(strlen(equal_ptr) < POWERUP_NAME_LENGTH);	//	Oops, name too long.
				strcpy(Powerup_names[n], &equal_ptr[1]);
				Powerup_names[n][strlen(Powerup_names[n])-1] = 0;
			} else if (!stricmp( arg, "size" ))	{
				Powerup_info[n].size = fl2f(atof(equal_ptr);
			} else {
				Int3();
#if TRACE
				con_printf (1, "Invalid parameter, %bObjectRendered=%bObjectRendered in bitmaps.tbl\n", arg, equal_ptr );
#endif
			}		
		} else {			// Must be a texture specification...
			Int3();
#if TRACE
			con_printf (1, "Invalid argument, %bObjectRendered in bitmaps.tbl\n", arg );
#endif
		}
		arg = strtok( NULL, space );
	}

	gameData.objs.types.nType[gameData.objs.types.nCount] = OL_POWERUP;
	gameData.objs.types.nType.nId[gameData.objs.types.nCount] = n;
	////printf( "Object type %d is a powerup\n", gameData.objs.types.nCount );
	gameData.objs.types.nCount++;
	Assert(gameData.objs.types.nCount < MAX_OBJTYPE);

}

void bm_read_hostage()
{
	int n;
	char 	*equal_ptr;

	Assert(N_hostage_types < MAX_HOSTAGE_TYPES);

	n = N_hostage_types;
	N_hostage_types++;

	// Process arguments
	arg = strtok( NULL, space );

	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			if (!stricmp( arg, "vclip_num" ))

				Hostage_vclip_num[n] = atoi(equal_ptr);

			else {
				Int3();
#if TRACE
				con_printf (1, "Invalid parameter, %bObjectRendered=%bObjectRendered in bitmaps.tbl\n", arg, equal_ptr );
#endif
			}

		} else {
			Int3();
#if TRACE
			con_printf (1, "Invalid argument, %bObjectRendered in bitmaps.tbl at line %d\n", arg, linenum );
#endif
		}
		arg = strtok( NULL, space );
	}

	gameData.objs.types.nType[gameData.objs.types.nCount] = OL_HOSTAGE;
	gameData.objs.types.nType.nId[gameData.objs.types.nCount] = n;
	////printf( "Object type %d is a hostage\n", gameData.objs.types.nCount );
	gameData.objs.types.nCount++;
	Assert(gameData.objs.types.nCount < MAX_OBJTYPE);

}

//these values are the number of each item in the release of d2
//extra items added after the release get written in an additional hamfile
#define N_D2_ROBOT_TYPES		66
#define N_D2_ROBOT_JOINTS		1145
#define N_D2_POLYGON_MODELS	166
#define N_D2_OBJBITMAPS			422
#define N_D2_OBJBITMAPPTRS		502
#define N_D2_WEAPON_TYPES		62

void bm_write_all(FILE *fp)
{
	int i,t;
	FILE *tfile;
	int bObjectRendered=0;

tfile = fopen("hamfile.lst","wt");

	t = NumTextures-1;	//don't save bogus texture
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( Textures, sizeof(bitmap_index), t, fp );
	for (i=0;i<t;i++)
		fwrite( &TmapInfo [gameStates.app.bD1Data][i], sizeof(*TmapInfo)-sizeof(TmapInfo->filename)-sizeof(TmapInfo->pad2), 1, fp );

fprintf(tfile,"NumTextures = %d, Textures array = %d, TmapInfo array = %d\n",NumTextures,sizeof(bitmap_index)*NumTextures,sizeof(tmap_info)*NumTextures);

	t = MAX_SOUNDS;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( Sounds [gameStates.app.bD1Data], sizeof(ubyte), t, fp );
	fwrite( AltSounds, sizeof(ubyte), t, fp );

fprintf(tfile,"Num Sounds [gameStates.app.bD1Data] = %d, Sounds [gameStates.app.bD1Data] array = %d, AltSounds array = %d\n",t,t,t);

	fwrite( &Num_vclips, sizeof(int), 1, fp );
	fwrite( Vclip, sizeof(vclip), Num_vclips, fp );

fprintf(tfile,"Num_vclips = %d, Vclip array = %d\n",Num_vclips,sizeof(vclip)*Num_vclips);

	fwrite( &Num_effects, sizeof(int), 1, fp );
	fwrite( Effects, sizeof(eclip), Num_effects, fp );

fprintf(tfile,"Num_effects = %d, Effects array = %d\n",Num_effects,sizeof(eclip)*Num_effects);

	fwrite( &Num_wall_anims, sizeof(int), 1, fp );
	fwrite( WallAnims, sizeof(wclip), Num_wall_anims, fp );

fprintf(tfile,"Num_wall_anims = %d, WallAnims array = %d\n",Num_wall_anims,sizeof(wclip)*Num_wall_anims);

	t = N_D2_ROBOT_TYPES;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( Robot_info [gameStates.app.bD1Data], sizeof(robot_info), t, fp );

fprintf(tfile,"N_robot_types = %d, Robot_info [gameStates.app.bD1Data] array = %d\n",t,sizeof(robot_info)*N_robot_types);

	t = N_D2_ROBOT_JOINTS;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( Robot_joints, sizeof(jointpos), t, fp );

fprintf(tfile,"N_robot_joints = %d, Robot_joints array = %d\n",t,sizeof(jointpos)*N_robot_joints);

	t = N_D2_WEAPON_TYPES;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( Weapon_info, sizeof(weapon_info), t, fp );

fprintf(tfile,"N_weapon_types = %d, Weapon_info array = %d\n",N_weapon_types,sizeof(weapon_info)*N_weapon_types);

	fwrite( &N_powerup_types, sizeof(int), 1, fp );
	fwrite( Powerup_info, sizeof(powerup_type_info), N_powerup_types, fp );
	
fprintf(tfile,"N_powerup_types = %d, Powerup_info array = %d\n",N_powerup_types,sizeof(powerup_info)*N_powerup_types);

	t = N_D2_POLYGON_MODELS;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( gameData.models.polyModels, sizeof(polymodel), t, fp );

fprintf(tfile,"gameData.models.nPolyModels = %d, gameData.models.polyModels array = %d\n",t,sizeof(polymodel)*t);

	for (i=0; i<t; i++ )	{
		g3_uninit_polygon_model(gameData.models.polyModels[i].model_data);	//get RGB colors
		fwrite( gameData.models.polyModels[i].model_data, sizeof(ubyte), gameData.models.polyModels[i].model_data_size, fp );
fprintf(tfile,"  Model %d, data size = %d\n",i,gameData.models.polyModels[i].model_data_size); bObjectRendered+=gameData.models.polyModels[i].model_data_size;
		g3_init_polygon_model(gameData.models.polyModels[i].model_data);	//map colors again
	}
fprintf(tfile,"Total model size = %d\n",bObjectRendered);

	fwrite( gameData.models.nDyingModels, sizeof(int), t, fp );
	fwrite( gameData.models.nDeadModels, sizeof(int), t, fp );

fprintf(tfile,"gameData.models.nDyingModels array = %d, gameData.models.nDeadModels array = %d\n",sizeof(int)*t,sizeof(int)*t);

	t = MAX_GAUGE_BMS;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( Gauges, sizeof(bitmap_index), t, fp );
	fwrite( Gauges_hires, sizeof(bitmap_index), t, fp );

fprintf(tfile,"Num gauge bitmaps = %d, Gauges array = %d, Gauges_hires array = %d\n",t,sizeof(bitmap_index)*t,sizeof(bitmap_index)*t);

	t = MAX_OBJ_BITMAPS;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( gameData.pig.tex.objBmIndex, sizeof(bitmap_index), t, fp );
	fwrite( gameData.pig.tex.pObjBmIndex, sizeof(ushort), t, fp );

fprintf(tfile,"Num obj bitmaps = %d, gameData.pig.tex.objBmIndex array = %d, gameData.pig.tex.pObjBmIndex array = %d\n",t,sizeof(bitmap_index)*t,sizeof(ushort)*t);

	fwrite( &only_player_ship, sizeof(player_ship), 1, fp );

fprintf(tfile,"player_ship size = %d\n",sizeof(player_ship);

	fwrite( &gameData.models.nCockpits, sizeof(int), 1, fp );
	fwrite( gameData.pig.tex.cockpitBmIndex, sizeof(bitmap_index), gameData.models.nCockpits, fp );

fprintf(tfile,"gameData.models.nCockpits = %d, cockpit_bitmaps array = %d\n",gameData.models.nCockpits,sizeof(bitmap_index)*gameData.models.nCockpits);

//@@	fwrite( &gameData.objs.types.nCount, sizeof(int), 1, fp );
//@@	fwrite( gameData.objs.types.nType, sizeof(sbyte), gameData.objs.types.nCount, fp );
//@@	fwrite( gameData.objs.types.nType.nId, sizeof(sbyte), gameData.objs.types.nCount, fp );
//@@	fwrite( gameData.objs.types.nType.nStrength, sizeof(fix), gameData.objs.types.nCount, fp );

fprintf(tfile,"gameData.objs.types.nCount = %d, gameData.objs.types.nType array = %d, gameData.objs.types.nType.nId array = %d, gameData.objs.types.nType.nStrength array = %d\n",gameData.objs.types.nCount,gameData.objs.types.nCount,gameData.objs.types.nCount,sizeof(fix)*gameData.objs.types.nCount);

	fwrite( &gameData.pig.tex.nFirstMultiBitmap, sizeof(int), 1, fp );

	fwrite( &Num_reactors, sizeof(Num_reactors), 1, fp );
	fwrite( Reactors, sizeof(*Reactors), Num_reactors, fp);

fprintf(tfile,"Num_reactors = %d, Reactors array = %d\n",Num_reactors,sizeof(*Reactors)*Num_reactors);

	fwrite( &gameData.models.nMarkerModel, sizeof(gameData.models.nMarkerModel), 1, fp);

	//@@fwrite( &N_controlcen_guns, sizeof(int), 1, fp );
	//@@fwrite( controlcen_gun_points, sizeof(vms_vector), N_controlcen_guns, fp );
	//@@fwrite( controlcen_gun_dirs, sizeof(vms_vector), N_controlcen_guns, fp );

	#ifdef SHAREWARE
	fwrite( &gameData.endLevel.exit.nModel, sizeof(int), 1, fp );
	fwrite( &gameData.endLevel.exit.nDestroyedModel, sizeof(int), 1, fp );
	#endif

fclose(tfile);

	bm_write_extra_robots();
}

void bm_write_extra_robots()
{
	FILE *fp;
	u_int32_t t;
	int i;

	fp = fopen("robots.ham","wb");

	t = 0x5848414d; /* 'XHAM' */
	fwrite( &t, sizeof(int), 1, fp );
	t = 1;	//version
	fwrite( &t, sizeof(int), 1, fp );

	//write weapon info
	t = N_weapon_types - N_D2_WEAPON_TYPES;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( &Weapon_info[N_D2_WEAPON_TYPES], sizeof(weapon_info), t, fp );

	//now write robot info

	t = N_robot_types - N_D2_ROBOT_TYPES;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( &Robot_info [gameStates.app.bD1Data][N_D2_ROBOT_TYPES], sizeof(robot_info), t, fp );

	t = N_robot_joints - N_D2_ROBOT_JOINTS;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( &Robot_joints[N_D2_ROBOT_JOINTS], sizeof(jointpos), t, fp );

	t = gameData.models.nPolyModels - N_D2_POLYGON_MODELS;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( &gameData.models.polyModels[N_D2_POLYGON_MODELS], sizeof(polymodel), t, fp );

	for (i=N_D2_POLYGON_MODELS; i<gameData.models.nPolyModels; i++ )	{
		g3_uninit_polygon_model(gameData.models.polyModels[i].model_data);	//get RGB colors
		fwrite( gameData.models.polyModels[i].model_data, sizeof(ubyte), gameData.models.polyModels[i].model_data_size, fp );
		g3_init_polygon_model(gameData.models.polyModels[i].model_data);	//map colors again
	}

	fwrite( &gameData.models.nDyingModels[N_D2_POLYGON_MODELS], sizeof(int), t, fp );
	fwrite( &gameData.models.nDeadModels[N_D2_POLYGON_MODELS], sizeof(int), t, fp );

	t = gameData.pig.tex.nObjBitmaps - N_D2_OBJBITMAPS;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( &gameData.pig.tex.objBmIndex[N_D2_OBJBITMAPS], sizeof(bitmap_index), t, fp );

	t = N_ObjBitmapPtrs - N_D2_OBJBITMAPPTRS;
	fwrite( &t, sizeof(int), 1, fp );
	fwrite( &gameData.pig.tex.pObjBmIndex[N_D2_OBJBITMAPPTRS], sizeof(ushort), t, fp );

	fwrite( gameData.pig.tex.pObjBmIndex, sizeof(ushort), t, fp );

	fclose(fp);
}
