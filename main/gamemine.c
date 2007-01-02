/* $Id: gamemine.c, v 1.26 2003/10/22 15:00:37 schaffner Exp $ */
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

#ifdef RCS
static char rcsid [] = "$Id: gamemine.c, v 1.26 2003/10/22 15:00:37 schaffner Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"

#include "inferno.h"
#include "text.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "error.h"
#include "gameseg.h"
#include "switch.h"
#include "ogl_init.h"
#include "oof.h"
#include "lightmap.h"
#include "gameseg.h"

#include "game.h"
#include "menu.h"
#include "newmenu.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "cfile.h"
#include "fuelcen.h"

#include "hash.h"
#include "key.h"
#include "piggy.h"

#include "byteswap.h"
#include "gamesave.h"
#include "u_mem.h"
#include "vecmat.h"
#include "gamepal.h"
#include "paging.h"
#include "maths.h"
#include "lighting.h"

//------------------------------------------------------------------------------

#define REMOVE_EXT (s)  (* (strchr ((s), '.' ))='\0')

struct mtfi mine_top_fileinfo;    // Should be same as first two fields below...
struct mfi mine_fileinfo;
struct mh mine_header;
struct me mine_editor;

typedef struct v16_segment {
	#ifdef EDITOR
	short   nSegment;             // tSegment number, not sure what it means
	#endif
	tSide    sides [MAX_SIDES_PER_SEGMENT];       // 6 sides
	short   children [MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
	short   verts [MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
	#ifdef  EDITOR
	short   group;              // group number to which the tSegment belongs.
	#endif
	short   objects;            // pointer to gameData.objs.objects in this tSegment
	ubyte   special;            // what nType of center this is
	sbyte   nMatCen;         // which center tSegment is associated with.
	short   value;
	fix     static_light;       // average static light in tSegment
	#ifndef EDITOR
	short   pad;                // make structure longword aligned
	#endif
} v16_segment;

struct mfi_v19 {
	ushort  fileinfo_signature;
	ushort  fileinfo_version;
	int     fileinfo_sizeof;
	int     header_offset;      // Stuff common to game & editor
	int     header_size;
	int     editor_offset;      // Editor specific stuff
	int     editor_size;
	int     segment_offset;
	int     segment_howmany;
	int     segment_sizeof;
	int     newseg_verts_offset;
	int     newseg_verts_howmany;
	int     newseg_verts_sizeof;
	int     group_offset;
	int     group_howmany;
	int     group_sizeof;
	int     vertex_offset;
	int     vertex_howmany;
	int     vertex_sizeof;
	int     texture_offset;
	int     texture_howmany;
	int     texture_sizeof;
	tGameItemInfo	walls;
	tGameItemInfo	triggers;
	tGameItemInfo	links;
	tGameItemInfo	tObject;
	int     unused_offset;      // was: doors.offset
	int     unused_howmamy;     // was: doors.count
	int     unused_sizeof;      // was: doors.size
	short   level_shake_frequency;  // Shakes every level_shake_frequency seconds
	short   level_shake_duration;   // for level_shake_duration seconds (on average, random).  In 16ths second.
	int     secret_return_segment;
	vmsMatrix  secret_return_orient;
	tGameItemInfo	lightDeltaIndices;
	tGameItemInfo	lightDeltas;
};

int CreateDefaultNewSegment ();

int bNewFileFormat = 1; // "new file format" is everything newer than d1 shareware

int bD1PigPresent = 0; // can descent.pig from descent 1 be loaded?

/* returns nonzero if nD1Texture references a texture which isn't available in d2. */
int d1_tmap_num_unique (short nD1Texture) 
{
	short t, i;
	static short unique_tmap_nums [] = {
		  0,   2,   4,   5,   6,   7,   9, 
		 10,  11,  12,  17,  18, 
		 20,  21,  25,  28, 
		 38,  39,  41,  44,  49, 
		 50,  55,  57,  88, 
		132, 141, 147, 
		154, 155, 158, 159, 
		160, 161, 167, 168, 169, 
		170, 171, 174, 175, 185, 
		193, 194, 195, 198, 199, 
		200, 202, 210, 211, 
		220, 226, 227, 228, 229, 230, 
		240, 241, 242, 243, 246, 
		250, 251, 252, 253, 257, 258, 259, 
		260, 263, 266, 283, 298, 
		305, 308, 311, 312, 
		315, 317, 319, 320, 321, 
		330, 331, 332, 333, 349, 
		351, 352, 353, 354, 
		355, 357, 358, 359, 
		362, 370, 
		-1};
	  
for (i = 0;; i++) {
	if (0 > (t = unique_tmap_nums [i]))
		break;
	if (t == nD1Texture)
		return 1;
	}
return 0;
}

#define TMAP_NUM_MASK 0x3FFF

/* Converts descent 1 texture numbers to descent 2 texture numbers.
 * gameData.pig.tex.bmIndex from d1 which are unique to d1 have extra spaces around "return".
 * If we can load the original d1 pig, we make sure this function is bijective.
 * This function was updated using the file config/convtabl.ini from devil 2.2.
 */

typedef struct nD1ToD2Texture {
	short	d1_min, d1_max;
	short	repl [2];
} nD1ToD2Texture;

short ConvertD1Texture (short nD1Texture, int bForce) 
{
	int h, i;

	static nD1ToD2Texture nD1ToD2Texture [] = {
		{0, 0, {43, 137}}, 
		{1, 1, {0, 0}}, 
		{2, 2, {43, 137}}, 
		{3, 3, {1, 1}}, 
		{4, 4, {43, 137}}, 
		{5, 5, {43, 137}}, 
		{6, 7, {-1, 270 - 6}}, 
		{8, 8, {2, 2}}, 
		{9, 9, {62, 138}}, 
		{10, 10, {272}}, 
		{11, 11, {117, 139}}, 
		{12, 12, {12, 140}}, 
		{13, 16, {-1, 3 - 13}}, 
		{17, 17, {52, 141}}, 
		{18, 18, {129, 129}}, 
		{19, 19, {7, 7}}, 
		{20, 20, {22, 142}}, 
		{21, 21, {9, 143}}, 
		{22, 22, {8, 8}}, 
		{23, 23, {9, 9}}, 
		{24, 24, {10, 10}}, 
		{25, 25, {12, 144}}, 
		{26, 27, {-1, 11 - 26}}, 
		{28, 28, {11, 145}}, 
		{29, 37, {-1, -16}}, 
		{38, 38, {163, 163}}, 
		{39, 39, {147, 147}}, 
		{40, 40, {22, 22}}, 
		{41, 41, {266, 266}}, 
		{42, 43, {-1, 23 - 42}}, 
		{44, 44, {136, 136}}, 
		{45, 48, {-1, 25 - 45}}, 
		{49, 49, {43, 146}}, 
		{50, 50, {131, 131}}, 
		{51, 54, {-1, 29 - 51}}, 
		{55, 55, {165, 165}}, 
		{56, 56, {33, 33}}, 
		{57, 57, {132, 132}}, 
		{58, 87, {-1, -24}}, 
		{88, 88, {197, 197}}, 
		{89, 131, {-1, -25}}, 
		{132, 132, {167, 167}}, 
		{133, 140, {-1, -26}}, 
		{141, 141, {110, 148}}, 
		{142, 146, {-1, 115 - 142}}, 
		{147, 147, {93, 149}}, 
		{148, 153, {-1, 120 - 148}}, 
		{154, 154, {27, 150}}, 
		{155, 155, {126, 126}}, 
		{156, 157, {-1, 200 - 156}}, 
		{158, 158, {186, 186}}, 
		{159, 159, {190, 190}}, 
		{160, 160, {206, 151}}, 
		{161, 161, {114, 152}}, 
		{162, 166, {-1, 202 - 162}}, 
		{167, 167, {206, 153}}, 
		{168, 168, {206, 154}}, 
		{169, 169, {206, 155}}, 
		{170, 170, {227, 156}}, 
		{171, 171, {206, 157}}, 
		{172, 173, {-1, 207 - 172}}, 
		{174, 174, {202, 158}}, 
		{175, 175, {206, 159}}, 
		{176, 184, {-1, 33}}, 
		{185, 185, {217, 160}}, 
		{186, 192, {-1, 32}}, 
		{193, 193, {206, 161}}, 
		{194, 194, {203, 162}}, 
		{195, 195, {234, 166}}, 
		{196, 197, {-1, 225 - 196}}, 
		{198, 198, {225, 167}}, 
		{199, 199, {206, 168}}, 
		{200, 200, {206, 169}}, 
		{201, 201, {227, 227}}, 
		{202, 202, {206, 170}}, 
		{203, 209, {-1, 25}}, 
		{210, 210, {234, 171}}, 
		{211, 211, {206, 172}}, 
		{212, 219, {-1, 23}}, 
		{220, 220, {242, 173}}, 
		{221, 222, {-1, 243 - 221}}, 
		{223, 223, {313, 174}}, 
		{224, 225, {-1, 245 - 224}}, 
		{226, 226, {164, 164}}, 
		{227, 227, {179, 179}}, 
		{228, 228, {196, 196}}, 
		{229, 229, {15, 175}}, 
		{230, 230, {15, 176}}, 
		{231, 239, {-1, 18}}, 
		{240, 240, {6, 177}}, 
		{241, 241, {130, 130}}, 
		{242, 242, {78, 178}}, 
		{243, 243, {33, 180}}, 
		{244, 245, {-1, 258 - 244}}, 
//		{246, 246, {321, 181}}, 
		{246, 246, {321, 321}}, 
		{247, 249, {-1, 260 - 247}}, 
		{250, 250, {340, 340}}, 
		{251, 251, {412, 412}}, 
		{252, 253, {-1, 410 - 252}}, 
		{254, 256, {-1, 263 - 254}}, 
		{257, 257, {249, 182}}, 
		{258, 258, {251, 183}}, 
		{259, 259, {252, 184}}, 
		{260, 260, {256, 185}}, 
		{261, 262, {-1, 273 - 261}}, 
		{263, 263, {281, 187}}, 
		{264, 265, {-1, 275 - 264}}, 
		{266, 266, {279, 188}}, 
		{267, 281, {-1, 10}}, 
		{282, 282, {293, 293}}, 
		{283, 283, {295, 189}}, 
		{284, 284, {295, 295}}, 
		{285, 285, {296, 296}}, 
		{286, 286, {298, 298}}, 
		{287, 297, {-1, 13}}, 
		{298, 298, {364, 191}}, 
		{299, 304, {-1, 12}}, 
		{305, 305, {322, 322}}, 
		{306, 307, {-1, 12}}, 
		{308, 308, {324, 324}}, 
		{309, 314, {-1, 12}}, 
		{315, 315, {361, 192}}, 
		{316, 326, {-1, 11}}, 
		{327, 329, {-1, 352 - 327}}, 
		{330, 330, {380, 380}}, 
		{331, 331, {379, 379}}, 
		{332, 332, {350, 350}}, 
		{333, 333, {409, 409}}, 
		{334, 340, {-1, 356 - 334}}, 
//		{341, 341, {364, 364}}, 
		{341, 341, {372, 372}}, 
		{342, 342, {363, 363}}, 
		{343, 343, {366, 366}}, 
		{344, 344, {365, 365}}, 
		{345, 345, {382, 382}}, 
		{346, 346, {376, 376}}, 
		{347, 347, {370, 370}}, 
		{348, 348, {367, 367}}, 
		{349, 349, {331, 331}}, 
		{350, 350, {369, 369}}, 
//		{351, 352, {-1, 374 - 351}}, 
		{351, 351, {394, 394}}, 
		{352, 352, {370, 370}}, 
		{353, 353, {371, 371}}, 
//		{354, 354, {377, 377}}, 
		{354, 354, {394, 394}}, 
		{355, 355, {408, 408}}, 
		{356, 356, {378, 378}}, 
		{357, 361, {-1, 383 - 357}}, 
		{362, 362, {388, 194}}, 
		{363, 363, {388, 388}}, 
		{364, 369, {-1, 388 - 364}}, 
		{370, 370, {392, 195}}, 
		{371, 374, {-1, 64}}
		};

	if (gameStates.app.bHaveD1Data && !bForce)
		return nD1Texture;
	if ((nD1Texture > 370) && (nD1Texture < 584)) {
		if (bNewFileFormat) {
			if (nD1Texture == 550) 
				return 615;
			return nD1Texture + 64;
			}
		// d1 shareware needs special treatment:
		if (nD1Texture < 410) 
			return nD1Texture + 68;
		if (nD1Texture < 417) 
			return nD1Texture + 73;
		if (nD1Texture < 446) 
			return nD1Texture + 91;
		if (nD1Texture < 453) 
			return nD1Texture + 104;
		if (nD1Texture < 462) 
			return nD1Texture + 111;
		if (nD1Texture < 486) 
			return nD1Texture + 117;
		if (nD1Texture < 494) 
			return nD1Texture + 141;
		if (nD1Texture < 584) 
			return nD1Texture + 147;
		}

	for (h = sizeof (nD1ToD2Texture) / sizeof (nD1ToD2Texture), i = 0; i < h; i++)
		if ((nD1ToD2Texture [i].d1_min <= nD1Texture) && (nD1ToD2Texture [i].d1_max >= nD1Texture)) {
			if (nD1ToD2Texture [i].repl [0] == -1)	// -> repl [1] contains an offset
				return nD1Texture + nD1ToD2Texture [i].repl [1];
			else
				return nD1ToD2Texture [i].repl [bForce ? 0 : bD1PigPresent];
			}

	{ // handle rare case where orientation != 0
		short nTexture = nD1Texture &  TMAP_NUM_MASK;
		short orient = nD1Texture & ~TMAP_NUM_MASK;
	if (orient)
		return orient | ConvertD1Texture (nTexture, bForce);
	//Warning (TXT_D1TEXTURE, nTexture);
	return nD1Texture;
	}
}

#ifdef EDITOR

static char old_tmap_list [MAX_TEXTURES][FILENAME_LEN];
short tmap_xlate_table [MAX_TEXTURES];
static short tmapTimes_used [MAX_TEXTURES];

// -----------------------------------------------------------------------------
//loads from an already-open file
// returns 0=everything ok, 1=old version, -1=error
int load_mine_data (CFILE *loadFile)
{
	int   i, j, oldsizeadjust;
	short tmap_xlate;
	int 	translate;
	char 	*temptr;
	int	mine_start = CFTell (loadFile);
	bD1PigPresent = CFExist (D1_PIGFILE);

	oldsizeadjust= (sizeof (int)*2)+sizeof (vmsMatrix);
	FuelCenReset ();

	for (i=0; i<MAX_TEXTURES; i++ )
		tmapTimes_used [i] = 0;

	#ifdef EDITOR
	// Create a new mine to initialize things.
	//texpage_goto_first ();
	create_new_mine ();
	#endif

	//===================== READ FILE INFO ========================

	// These are the default values... version and fileinfo_sizeof
	// don't have defaults.
	mine_fileinfo.header_offset     =   -1;
	mine_fileinfo.header_size       =   sizeof (mine_header);
	mine_fileinfo.editor_offset     =   -1;
	mine_fileinfo.editor_size       =   sizeof (mine_editor);
	mine_fileinfo.vertex_offset     =   -1;
	mine_fileinfo.vertex_howmany    =   0;
	mine_fileinfo.vertex_sizeof     =   sizeof (vmsVector);
	mine_fileinfo.segment_offset    =   -1;
	mine_fileinfo.segment_howmany   =   0;
	mine_fileinfo.segment_sizeof    =   sizeof (tSegment);
	mine_fileinfo.newseg_verts_offset     =   -1;
	mine_fileinfo.newseg_verts_howmany    =   0;
	mine_fileinfo.newseg_verts_sizeof     =   sizeof (vmsVector);
	mine_fileinfo.group_offset		  =	-1;
	mine_fileinfo.group_howmany	  =	0;
	mine_fileinfo.group_sizeof		  =	sizeof (group);
	mine_fileinfo.texture_offset    =   -1;
	mine_fileinfo.texture_howmany   =   0;
 	mine_fileinfo.texture_sizeof    =   FILENAME_LEN;  // num characters in a name
 	mine_fileinfo.walls.offset		  =	-1;
	mine_fileinfo.walls.count	  =	0;
	mine_fileinfo.walls.size		  =	sizeof (wall);  
 	mine_fileinfo.triggers.offset	  =	-1;
	mine_fileinfo.triggers.count  =	0;
	mine_fileinfo.triggers.size	  =	sizeof (tTrigger);  
	mine_fileinfo.object.offset		=	-1;
	mine_fileinfo.object.count		=	1;
	mine_fileinfo.object.size		=	sizeof (tObject);  

	mine_fileinfo.level_shake_frequency		=	0;
	mine_fileinfo.level_shake_duration		=	0;

	//	Delta light stuff for blowing out light sources.
//	if (mine_top_fileinfo.fileinfo_version >= 19) {
		mine_fileinfo.gameData.render.lights.deltaIndices.offset		=	-1;
		mine_fileinfo.gameData.render.lights.deltaIndices.count		=	0;
		mine_fileinfo.gameData.render.lights.deltaIndices.size		=	sizeof (dl_index);  

		mine_fileinfo.deltaLight.offset		=	-1;
		mine_fileinfo.deltaLight.count		=	0;
		mine_fileinfo.deltaLight.size		=	sizeof (delta_light);  

//	}

	mine_fileinfo.segment2_offset		= -1;
	mine_fileinfo.segment2_howmany	= 0;
	mine_fileinfo.segment2_sizeof    = sizeof (segment2);

	// Read in mine_top_fileinfo to get size of saved fileinfo.
	
	memset ( &mine_top_fileinfo, 0, sizeof (mine_top_fileinfo) );

	if (CFSeek ( loadFile, mine_start, SEEK_SET ))
		Error ( "Error moving to top of file in gamemine.c" );

	if (CFRead ( &mine_top_fileinfo, sizeof (mine_top_fileinfo), 1, loadFile )!=1)
		Error ( "Error reading mine_top_fileinfo in gamemine.c" );

	if (mine_top_fileinfo.fileinfo_signature != 0x2884)
		return -1;

	// Check version number
	if (mine_top_fileinfo.fileinfo_version < COMPATIBLE_VERSION )
		return -1;

	// Now, Read in the fileinfo
	if (CFSeek ( loadFile, mine_start, SEEK_SET ))
		Error ( "Error seeking to top of file in gamemine.c" );

	if (CFRead ( &mine_fileinfo, mine_top_fileinfo.fileinfo_sizeof, 1, loadFile )!=1)
		Error ( "Error reading mine_fileinfo in gamemine.c" );

	if (mine_top_fileinfo.fileinfo_version < 18) {
#if TRACE
		con_printf (1, "Old version, setting shake intensity to 0.\n");
#endif
		gameStates.gameplay.seismic.nShakeFrequency = 0;
		gameStates.gameplay.seismic.nShakeDuration = 0;
		gameData.segs.secret.nReturnSegment = 0;
		gameData.segs.secret.returnOrient = vmdIdentityMatrix;
	} else {
		gameStates.gameplay.seismic.nShakeFrequency = mine_fileinfo.level_shake_frequency << 12;
		gameStates.gameplay.seismic.nShakeDuration = mine_fileinfo.level_shake_duration << 12;
		gameData.segs.secret.nReturnSegment = mine_fileinfo.secret_return_segment;
		gameData.segs.secret.returnOrient = mine_fileinfo.secret_return_orient;
	}

	//===================== READ HEADER INFO ========================

	// Set default values.
	mine_header.num_vertices        =   0;
	mine_header.num_segments        =   0;

	if (mine_fileinfo.header_offset > -1 )
	{
		if (CFSeek ( loadFile, mine_fileinfo.header_offset, SEEK_SET ))
			Error ( "Error seeking to header_offset in gamemine.c" );
	
		if (CFRead ( &mine_header, mine_fileinfo.header_size, 1, loadFile )!=1)
			Error ( "Error reading mine_header in gamemine.c" );
	}

	//===================== READ EDITOR INFO ==========================

	// Set default values
	mine_editor.current_seg         =   0;
	mine_editor.newsegment_offset   =   -1; // To be written
	mine_editor.newsegment_size     =   sizeof (tSegment);
	mine_editor.Curside             =   0;
	mine_editor.Markedsegp          =   -1;
	mine_editor.Markedside          =   0;

	if (mine_fileinfo.editor_offset > -1 )
	{
		if (CFSeek ( loadFile, mine_fileinfo.editor_offset, SEEK_SET ))
			Error ( "Error seeking to editor_offset in gamemine.c" );
	
		if (CFRead ( &mine_editor, mine_fileinfo.editor_size, 1, loadFile )!=1)
			Error ( "Error reading mine_editor in gamemine.c" );
	}

	//===================== READ TEXTURE INFO ==========================

	if ((mine_fileinfo.texture_offset > -1) && (mine_fileinfo.texture_howmany > 0))
	{
		if (CFSeek ( loadFile, mine_fileinfo.texture_offset, SEEK_SET ))
			Error ( "Error seeking to texture_offset in gamemine.c" );

		for (i=0; i< mine_fileinfo.texture_howmany; i++ )
		{
			if (CFRead ( &old_tmap_list [i], mine_fileinfo.texture_sizeof, 1, loadFile )!=1)
				Error ( "Error reading old_tmap_list [i] in gamemine.c" );
		}
	}

	//=============== GENERATE TEXTURE TRANSLATION TABLE ===============

	translate = 0;
	
	Assert (gameData.pig.tex.nTextures < MAX_TEXTURES);

	{
		hashtable ht;
	
		hashtable_init ( &ht, gameData.pig.tex.nTextures );
	
		// Remove all the file extensions in the textures list
	
		for (i=0;i<gameData.pig.tex.nTextures;i++)	{
			temptr = strchr (gameData.pig.tex.pTMapInfo [i].filename, '.');
			if (temptr) *temptr = '\0';
			hashtable_insert ( &ht, gameData.pig.tex.pTMapInfo [i].filename, i );
		}
	
		// For every texture, search through the texture list
		// to find a matching name.
		for (j=0;j<mine_fileinfo.texture_howmany;j++) 	{
			// Remove this texture name's extension
			temptr = strchr (old_tmap_list [j], '.');
			if (temptr) *temptr = '\0';
	
			tmap_xlate_table [j] = hashtable_search ( &ht, old_tmap_list [j]);
			if (tmap_xlate_table [j]	< 0 )	{
				//tmap_xlate_table [j] = 0;
				;
			}
			if (tmap_xlate_table [j] != j ) translate = 1;
			if (tmap_xlate_table [j] >= 0)
				tmapTimes_used [tmap_xlate_table [j]]++;
		}
	
		{
			int count = 0;
			for (i=0; i<MAX_TEXTURES; i++ )
				if (tmapTimes_used [i])
					count++;
#if TRACE
			con_printf (CON_DEBUG, "This mine has %d unique textures in it (~%d KB)\n", count, (count*4096) /1024 );
#endif
		}
	
		hashtable_free ( &ht );
	}

	//====================== READ VERTEX INFO ==========================

	// New check added to make sure we don't read in too many vertices.
	if ( mine_fileinfo.vertex_howmany > MAX_VERTICES )
		{
#if TRACE
		con_printf (CON_DEBUG, "Num vertices exceeds maximum.  Loading MAX %d vertices\n", MAX_VERTICES);
#endif
		mine_fileinfo.vertex_howmany = MAX_VERTICES;
		}

	if ((mine_fileinfo.vertex_offset > -1) && (mine_fileinfo.vertex_howmany > 0))
	{
		if (CFSeek ( loadFile, mine_fileinfo.vertex_offset, SEEK_SET ))
			Error ( "Error seeking to vertex_offset in gamemine.c" );

		for (i=0; i< mine_fileinfo.vertex_howmany; i++ )
		{
			// Set the default values for this vertex
			gameData.segs.vertices [i].x = 1;
			gameData.segs.vertices [i].y = 1;
			gameData.segs.vertices [i].z = 1;

			if (CFRead ( &gameData.segs.vertices [i], mine_fileinfo.vertex_sizeof, 1, loadFile )!=1)
				Error ( "Error reading gameData.segs.vertices [i] in gamemine.c" );
		}
	}

	//==================== READ SEGMENT INFO ===========================

	// New check added to make sure we don't read in too many segments.
	if ( mine_fileinfo.segment_howmany > MAX_SEGMENTS ) {
#if TRACE
		con_printf (CON_DEBUG, "Num segments exceeds maximum.  Loading MAX %d segments\n", MAX_SEGMENTS);
#endif
		mine_fileinfo.segment_howmany = MAX_SEGMENTS;
		mine_fileinfo.segment2_howmany = MAX_SEGMENTS;
	}

	// [commented out by mk on 11/20/94 (weren't we supposed to hit final in October?) because it looks redundant.  I think I'll test it now...]  FuelCenReset ();

	if ((mine_fileinfo.segment_offset > -1) && (mine_fileinfo.segment_howmany > 0))	{

		if (CFSeek ( loadFile, mine_fileinfo.segment_offset, SEEK_SET ))

			Error ( "Error seeking to segment_offset in gamemine.c" );

		gameData.segs.nLastSegment = mine_fileinfo.segment_howmany-1;

		for (i=0; i< mine_fileinfo.segment_howmany; i++ ) {

			// Set the default values for this tSegment (clear to zero )
			//memset ( &gameData.segs.segments [i], 0, sizeof (tSegment) );

			if (mine_top_fileinfo.fileinfo_version < 20) {
				v16_segment v16_seg;

				Assert (mine_fileinfo.segment_sizeof == sizeof (v16_seg);

				if (CFRead ( &v16_seg, mine_fileinfo.segment_sizeof, 1, loadFile )!=1)
					Error ( "Error reading segments in gamemine.c" );

				#ifdef EDITOR
				gameData.segs.segments [i].position.nSegment = v16_seg.nSegment;
				// -- gameData.segs.segments [i].pad = v16_seg.pad;
				#endif

				for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
					gameData.segs.segments [i].sides [j] = v16_seg.sides [j];

				for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
					gameData.segs.segments [i].children [j] = v16_seg.children [j];

				for (j=0; j<MAX_VERTICES_PER_SEGMENT; j++)
					gameData.segs.segments [i].verts [j] = v16_seg.verts [j];

				gameData.segs.segment2s [i].special = v16_seg.special;
				gameData.segs.segment2s [i].value = v16_seg.value;
				gameData.segs.segment2s [i].s2Flags = 0;
				gameData.segs.segment2s [i].nMatCen = v16_seg.nMatCen;
				gameData.segs.segment2s [i].static_light = v16_seg.static_light;
				FuelCenActivate ( &gameData.segs.segments [i], gameData.segs.segment2s [i].special );

			} else  {
				if (CFRead (gameData.segs.segments + i, mine_fileinfo.segment_sizeof, 1, loadFile )!=1)
					Error ("Unable to read tSegment %i\n", i);
			}

			gameData.segs.segments [i].objects = -1;
			#ifdef EDITOR
			gameData.segs.segments [i].group = -1;
			#endif

			if (mine_top_fileinfo.fileinfo_version < 15) {	//used old uvl ranges
				int sn, uvln;

				for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++)
					for (uvln=0;uvln<4;uvln++) {
						gameData.segs.segments [i].sides [sn].uvls [uvln].u /= 64;
						gameData.segs.segments [i].sides [sn].uvls [uvln].v /= 64;
						gameData.segs.segments [i].sides [sn].uvls [uvln].l /= 32;
					}
			}

			if (translate == 1)
				for (j=0;j<MAX_SIDES_PER_SEGMENT;j++) {
					unsigned short orient;
					tmap_xlate = gameData.segs.segments [i].sides [j].nBaseTex;
					gameData.segs.segments [i].sides [j].nBaseTex = tmap_xlate_table [tmap_xlate];
					if ((WALL_IS_DOORWAY (gameData.segs.segments + i, j, NULL) & WID_RENDER_FLAG))
						if (gameData.segs.segments [i].sides [j].nBaseTex < 0)	{
#if TRACE
							con_printf (CON_DEBUG, "Couldn't find texture '%s' for Segment %d, tSide %d\n", old_tmap_list [tmap_xlate], i, j);
#endif
							Int3 ();
							gameData.segs.segments [i].sides [j].nBaseTex = gameData.pig.tex.nTextures-1;
						}
					tmap_xlate = gameData.segs.segments [i].sides [j].nOvlTex;
					if (tmap_xlate != 0) {
						int xlated_tmap = tmap_xlate_table [tmap_xlate];

						if ((WALL_IS_DOORWAY (gameData.segs.segments + i, j, NULL) & WID_RENDER_FLAG))
							if (xlated_tmap <= 0)	{
#if TRACE
								con_printf (CON_DEBUG, "Couldn't find texture '%s' for Segment %d, tSide %d\n", old_tmap_list [tmap_xlate], i, j);
#endif
								Int3 ();
								gameData.segs.segments [i].sides [j].nOvlTex = gameData.pig.tex.nTextures - 1;
							}
						gameData.segs.segments [i].sides [j].nOvlTex = xlated_tmap;
					}
				}
		}


		if (mine_top_fileinfo.fileinfo_version >= 20)
			for (i=0; i<=gameData.segs.nLastSegment; i++) {
				CFRead (gameData.segs.segment2s + i, sizeof (segment2), 1, loadFile);
				FuelCenActivate (gameData.segs.segments + i, gameData.segs.segment2s [i].special );
			}
	}

	//===================== READ NEWSEGMENT INFO =====================

	#ifdef EDITOR

	{		// Default tSegment created.
		vmsVector	sizevec;
		med_create_new_segment (VmVecMake (&sizevec, DEFAULT_X_SIZE, DEFAULT_Y_SIZE, DEFAULT_Z_SIZE);		// New_segment = gameData.segs.segments [0];
		//memset ( &New_segment, 0, sizeof (tSegment) );
	}

	if (mine_editor.newsegment_offset > -1)
	{
		if (CFSeek ( loadFile, mine_editor.newsegment_offset, SEEK_SET ))
			Error ( "Error seeking to newsegment_offset in gamemine.c" );
		if (CFRead ( &New_segment, mine_editor.newsegment_size, 1, loadFile )!=1)
			Error ( "Error reading new_segment in gamemine.c" );
	}

	if ((mine_fileinfo.newseg_verts_offset > -1) && (mine_fileinfo.newseg_verts_howmany > 0))
	{
		if (CFSeek ( loadFile, mine_fileinfo.newseg_verts_offset, SEEK_SET ))
			Error ( "Error seeking to newseg_verts_offset in gamemine.c" );
		for (i=0; i< mine_fileinfo.newseg_verts_howmany; i++ )
		{
			// Set the default values for this vertex
			gameData.segs.vertices [NEW_SEGMENT_VERTICES+i].x = 1;
			gameData.segs.vertices [NEW_SEGMENT_VERTICES+i].y = 1;
			gameData.segs.vertices [NEW_SEGMENT_VERTICES+i].z = 1;
			
			if (CFRead ( &gameData.segs.vertices [NEW_SEGMENT_VERTICES+i], mine_fileinfo.newseg_verts_sizeof, 1, loadFile )!=1)
				Error ( "Error reading gameData.segs.vertices [NEW_SEGMENT_VERTICES+i] in gamemine.c" );

			New_segment.verts [i] = NEW_SEGMENT_VERTICES+i;
		}
	}

	#endif
															
	//========================= UPDATE VARIABLES ======================

	#ifdef EDITOR

	// Setting to Markedsegp to NULL ignores Curside and Markedside, which
	// we want to do when reading in an old file.
	
 	Markedside = mine_editor.Markedside;
	Curside = mine_editor.Curside;
	for (i=0;i<10;i++)
		Groupside [i] = mine_editor.Groupside [i];

	if ( mine_editor.current_seg != -1 )
		Cursegp = mine_editor.current_seg + gameData.segs.segments;
	else
 		Cursegp = NULL;

	if (mine_editor.Markedsegp != -1 ) 
		Markedsegp = mine_editor.Markedsegp + gameData.segs.segments;
	else
		Markedsegp = NULL;

	num_groups = 0;
	current_group = -1;

	#endif

	gameData.segs.nVertices = mine_fileinfo.vertex_howmany;
	gameData.segs.nSegments = mine_fileinfo.segment_howmany;
	gameData.segs.nLastVertex = gameData.segs.nVertices-1;
	gameData.segs.nLastSegment = gameData.segs.nSegments-1;

	ResetObjects (1);		//one tObject, the tPlayer

	#ifdef EDITOR
	gameData.segs.nLastVertex = MAX_SEGMENT_VERTICES-1;
	gameData.segs.nLastSegment = MAX_SEGMENTS-1;
	set_vertex_counts ();
	gameData.segs.nLastVertex = gameData.segs.nVertices-1;
	gameData.segs.nLastSegment = gameData.segs.nSegments-1;

	warn_if_concave_segments ();
	#endif

	#ifdef EDITOR
		ValidateSegmentAll ();
	#endif

	//create_local_segment_data ();

	//gamemine_find_textures ();

	if (mine_top_fileinfo.fileinfo_version < MINE_VERSION )
		return 1;		//old version
	else
		return 0;

}
#endif

//------------------------------------------------------------------------------

#define COMPILED_MINE_VERSION 0

void read_children (int nSegment, ubyte bit_mask, CFILE *loadFile)
{
	int bit;

	for (bit=0; bit<MAX_SIDES_PER_SEGMENT; bit++) {
		if (bit_mask & (1 << bit)) {
			gameData.segs.segments [nSegment].children [bit] = CFReadShort (loadFile);
		} else
			gameData.segs.segments [nSegment].children [bit] = -1;
	}
}

//------------------------------------------------------------------------------

void ReadColor (tFaceColor *pc, CFILE *loadFile, int bFloatData)
{
pc->index = CFReadByte (loadFile);
if (bFloatData) {
	tRgbColord	c;
	CFRead (&c, sizeof (c), 1, loadFile);
	pc->color.red = (float) c.red;
	pc->color.green = (float) c.green;
	pc->color.blue = (float) c.blue;
	}
else {
	int c = CFReadInt (loadFile);
	pc->color.red = (float) c / (float) 0x7fffffff;
	c = CFReadInt (loadFile);
	pc->color.green = (float) c / (float) 0x7fffffff;
	c = CFReadInt (loadFile);
	pc->color.blue = (float) c / (float) 0x7fffffff;
	}
#if 0
CBRK ((pc->color.red < 0) || (pc->color.red > 1.0f) ||
		(pc->color.green < 0) || (pc->color.green > 1.0f) ||
		(pc->color.blue < 0) || (pc->color.blue > 1.0f));
#endif
}

//------------------------------------------------------------------------------

void read_verts (int nSegment, CFILE *loadFile)
{
	int i;
	// Read short gameData.segs.segments [nSegment].verts [MAX_VERTICES_PER_SEGMENT]
	for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
		gameData.segs.segments [nSegment].verts [i] = CFReadShort (loadFile);
}

//------------------------------------------------------------------------------

void read_special (int nSegment, ubyte bit_mask, CFILE *loadFile)
{
	if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
		// Read ubyte	gameData.segs.segment2s [nSegment].special
		gameData.segs.segment2s [nSegment].special = CFReadByte (loadFile);
		// Read byte	gameData.segs.segment2s [nSegment].nMatCen
		gameData.segs.segment2s [nSegment].nMatCen = CFReadByte (loadFile);
		// Read short	gameData.segs.segment2s [nSegment].value
		gameData.segs.segment2s [nSegment].value = (char) CFReadShort (loadFile);
	} else {
		gameData.segs.segment2s [nSegment].special = 0;
		gameData.segs.segment2s [nSegment].nMatCen = -1;
		gameData.segs.segment2s [nSegment].value = 0;
	}
}

//------------------------------------------------------------------------------

typedef struct tLightDist {
	int		nIndex;
	int		nDist;
} tLightDist;

void QSortLightDist (tLightDist *pDist, int left, int right)
{
	int			l = left, 
					r = right, 
					m = pDist [(l + r) / 2].nDist;
	tLightDist	h;

while (pDist [l].nDist < m)
	l++;
while (pDist [r].nDist > m)
	r--;
if (l <= r) {
	if (l < r) {
		h = pDist [l];
		pDist [l] = pDist [r];
		pDist [r] = h;
		}
	l++;
	r--;
	}
if (l < right)
	QSortLightDist (pDist, l, right);
if (left < r)
	QSortLightDist (pDist, left, r);
}

//------------------------------------------------------------------------------

int nMaxNearestLights [21] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,20,24,28,32};

int ComputeNearestSegmentLights (int i)
{
	tSegment				*segP;
	tDynLight			*pl;
	int					h, j, l, n, nMaxLights;
	vmsVector			center, dist;
	struct tLightDist	*pDists;

if (!gameData.render.lights.dynamic.nLights)
	return 0;
if (!(pDists = d_malloc (gameData.render.lights.dynamic.nLights * sizeof (tLightDist)))) {
	gameOpts->render.bDynLighting = 0;
	gameData.render.shadows.nLights = 0;
	return 0;
	}
nMaxLights = nMaxNearestLights [gameOpts->ogl.nMaxLights];
if (nMaxLights > MAX_NEAREST_LIGHTS)
	nMaxLights = MAX_NEAREST_LIGHTS;
INIT_PROGRESS_LOOP (i, j, gameData.segs.nSegments);
for (segP = gameData.segs.segments + i; i < j; i++, segP++) {
	COMPUTE_SEGMENT_CENTER (&center, segP);
	pl = gameData.render.lights.dynamic.lights;
	for (l = n = 0; l < gameData.render.lights.dynamic.nLights; l++, pl++) {
		VmVecSub (&dist, &center, &pl->vPos);
		h = VmVecMag (&dist) - (int) (pl->rad * 65536);
		if ((pDists [n].nDist = h) <= F1_0 * 125) {
			pDists [n].nIndex = l;
			n++;
			}
		}
	if (n)
		QSortLightDist (pDists, 0, n - 1);
	h = (nMaxLights < n) ? nMaxLights : n;
	for (l = 0; l < h; l++)
		gameData.render.lights.dynamic.nNearestSegLights [i][l] = pDists [l].nIndex;
	for (; l < MAX_NEAREST_LIGHTS; l++)
		gameData.render.lights.dynamic.nNearestSegLights [i][l] = -1;
	}
d_free (pDists);
return 1;
}

//------------------------------------------------------------------------------

int ComputeNearestVertexLights (int i)
{
	vmsVector			*vertP;
	tDynLight			*pl;
	int					h, j, l, n, nMaxLights;
	vmsVector			dist;
	struct tLightDist	*pDists;

if (!gameData.render.lights.dynamic.nLights)
	return 0;
if (!(pDists = d_malloc (gameData.render.lights.dynamic.nLights * sizeof (tLightDist)))) {
	gameOpts->render.bDynLighting = 0;
	gameData.render.shadows.nLights = 0;
	return 0;
	}
nMaxLights = nMaxNearestLights [gameOpts->ogl.nMaxLights];
if (nMaxLights > MAX_NEAREST_LIGHTS)
	nMaxLights = MAX_NEAREST_LIGHTS;
INIT_PROGRESS_LOOP (i, j, gameData.segs.nVertices);
for (vertP = gameData.segs.vertices + i; i < j; i++, vertP++) {
	pl = gameData.render.lights.dynamic.lights;
	for (l = n = 0; l < gameData.render.lights.dynamic.nLights; l++, pl++) {
		VmVecSub (&dist, vertP, &pl->vPos);
		h = VmVecMag (&dist) - (int) (pl->rad * 65536);
		if ((pDists [n].nDist = h) <= F1_0 * 125) {
			pDists [n].nIndex = l;
			n++;
			}
		}
	if (n)
		QSortLightDist (pDists, 0, n - 1);
	h = (nMaxLights < n) ? nMaxLights : n;
	for (l = 0; l < h; l++)
		gameData.render.lights.dynamic.nNearestVertLights [i][l] = pDists [l].nIndex;
	for (; l < MAX_NEAREST_LIGHTS; l++)
		gameData.render.lights.dynamic.nNearestVertLights [i][l] = -1;
	}
d_free (pDists);
return 1;
}

//------------------------------------------------------------------------------

void LoadSegmentsCompiled (short nSegment, CFILE *loadFile)
{
	short			lastSeg, nSide, i;
	tSegment		*segP;
	tSide			*sideP;
	short			temp_short;
	ushort		nWall, temp_ushort = 0;
	short			sideVerts [4];
	ubyte			bit_mask;

INIT_PROGRESS_LOOP (nSegment, lastSeg, gameData.segs.nSegments);
for (segP = gameData.segs.segments + nSegment; nSegment < lastSeg; nSegment++, segP++) {

#ifdef EDITOR
	segP->nSegment = nSegment;
	segP->group = 0;
#endif

	if (gameStates.app.bD2XLevel) { 
		gameData.segs.xSegments [nSegment].owner = CFReadByte (loadFile);
		gameData.segs.xSegments [nSegment].group = CFReadByte (loadFile);
		}
	else {
		gameData.segs.xSegments [nSegment].owner = -1;
		gameData.segs.xSegments [nSegment].group = -1;
		}
	if (bNewFileFormat)
		bit_mask = CFReadByte (loadFile);
	else
		bit_mask = 0x7f; // read all six children and special stuff...

	if (gameData.segs.nLevelVersion == 5) { // d2 SHAREWARE level
		read_special (nSegment, bit_mask, loadFile);
		read_verts (nSegment, loadFile);
		read_children (nSegment, bit_mask, loadFile);
		}
	else {
		read_children (nSegment, bit_mask, loadFile);
		read_verts (nSegment, loadFile);
		if (gameData.segs.nLevelVersion <= 1) { // descent 1 level
			read_special (nSegment, bit_mask, loadFile);
			}
		}

	segP->objects = -1;

	if (gameData.segs.nLevelVersion <= 5) { // descent 1 thru d2 SHAREWARE level
		// Read fix	segP->static_light (shift down 5 bits, write as short)
		temp_ushort = CFReadShort (loadFile);
		gameData.segs.segment2s [nSegment].static_light	= ((fix)temp_ushort) << 4;
		//CFRead ( &segP->static_light, sizeof (fix), 1, loadFile );
		}

	// Read the walls as a 6 byte array
	for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++ )	{
		segP->sides [nSide].nFrame = 0;
		}

	if (bNewFileFormat)
		bit_mask = CFReadByte (loadFile);
	else
		bit_mask = 0x3f; // read all six sides
	for (nSide = 0, sideP = segP->sides; nSide < MAX_SIDES_PER_SEGMENT; nSide++, sideP++) {
		sideP->nWall = NO_WALL;
		if (bit_mask & (1 << nSide)) {
			if (gameData.segs.nLevelVersion >= 13)
				nWall = (ushort) CFReadShort (loadFile);
			else
				nWall = (ushort) ((ubyte) CFReadByte (loadFile));
			if (IS_WALL (nWall))
				sideP->nWall = nWall;
			}
		}

	for (nSide=0, sideP = segP->sides; nSide<MAX_SIDES_PER_SEGMENT; nSide++, sideP++ )	{
#ifdef _DEBUG
		int bReadSideData;
		if (segP->children [nSide]==-1)
			bReadSideData = 1;
		else if (IS_WALL (WallNumI (nSegment, nSide)))
			bReadSideData = 2;
		else
			bReadSideData = 0;
		if (bReadSideData) {
#else
		if ((segP->children [nSide] == -1) || IS_WALL (WallNumI (nSegment, nSide))) {
#endif
			// Read short sideP->nBaseTex;
			if (bNewFileFormat) {
				temp_ushort = CFReadShort (loadFile);
				sideP->nBaseTex = temp_ushort & 0x7fff;
				} 
			else
				sideP->nBaseTex = CFReadShort (loadFile);
			if (gameData.segs.nLevelVersion <= 1)
				sideP->nBaseTex = ConvertD1Texture (sideP->nBaseTex, 0);
			if (bNewFileFormat && !(temp_ushort & 0x8000))
				sideP->nOvlTex = 0;
			else {
				// Read short sideP->nOvlTex;
				short h = CFReadShort (loadFile);
				sideP->nOvlTex = h & 0x3fff;
				sideP->nOvlOrient = (h >> 14) & 3;
				if ((gameData.segs.nLevelVersion <= 1) && sideP->nOvlTex)
					sideP->nOvlTex = ConvertD1Texture (sideP->nOvlTex, 0);
				}

			// Read uvl sideP->uvls [4] (u, v>>5, write as short, l>>1 write as short)
			GetSideVerts (sideVerts, nSegment, nSide);
			for (i = 0; i < 4; i++ ) {
				temp_short = CFReadShort (loadFile);
				sideP->uvls [i].u = ((fix)temp_short) << 5;
				temp_short = CFReadShort (loadFile);
				sideP->uvls [i].v = ((fix)temp_short) << 5;
				temp_ushort = CFReadShort (loadFile);
#if 0 //LIGHTMAPS
				if (USE_LIGHTMAPS)
					sideP->uvls [i].l = F1_0 / 2;
				else
#endif
				sideP->uvls [i].l = ((fix)temp_ushort) << 1;
				gameData.render.color.vertBright [sideVerts [i]] = f2fl (sideP->uvls [i].l);
				//CFRead ( &sideP->uvls [i].l, sizeof (fix), 1, loadFile );
				}
			} 
		else {
			sideP->nBaseTex =
			sideP->nOvlTex = 0;
			for (i = 0; i < 4; i++) {
				sideP->uvls [i].u = 0;
				sideP->uvls [i].v = 0;
				sideP->uvls [i].l = 0;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void LoadSegment2sCompiled (CFILE *loadFile)
{
	int	i;

for (i = 0; i < gameData.segs.nSegments; i++) {
	if (gameData.segs.nLevelVersion > 5)
		segment2_read (gameData.segs.segment2s + i, loadFile);
	FuelCenActivate (gameData.segs.segments + i, gameData.segs.segment2s [i].special);
	}
}

//------------------------------------------------------------------------------

void LoadVertLightsCompiled (int i, CFILE *loadFile)
{
	int	j;

gameData.render.shadows.nLights = 0;
if (gameStates.app.bD2XLevel) {
	INIT_PROGRESS_LOOP (i, j, gameData.segs.nVertices);
	for (; i < j; i++)
		ReadColor (gameData.render.color.vertices + i, loadFile, gameData.segs.nLevelVersion <= 14);
	}
}

//------------------------------------------------------------------------------

void InitTexColors (void)
{
	int			i;
	tLightMap	lm;
	tFaceColor	*pf = gameData.render.color.textures;

// get the default colors
memset (gameData.render.color.textures, 0, sizeof (gameData.render.color.textures));
for (i = 0; i < MAX_WALL_TEXTURES; i++, pf++) {
	if (GetLightColor (i, &lm)) {
		pf->index = 1;
		pf->color.red = lm.color [0];
		pf->color.green = lm.color [1];
		pf->color.blue = lm.color [2];
		}
	}
}

//------------------------------------------------------------------------------

int HasTexColors (void)
{
	int			i;
	tFaceColor	*pf = gameData.render.color.textures;

if (!gameStates.app.bD2XLevel)
	return 0;
// get the default colors
for (i = 0; i < MAX_WALL_TEXTURES; i++, pf++) {
	if (pf->index <= 0)
		continue;
	if ((pf->color.red == 0) && (pf->color.green == 0) && (pf->color.blue == 0))
		continue;
	if ((pf->color.red < 1) || (pf->color.green < 1) || (pf->color.blue < 1))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

void LoadTexColorsCompiled (int i, CFILE *loadFile)
{
	int			j;

// get the default colors
if (gameStates.app.bD2XLevel) {
	INIT_PROGRESS_LOOP (i, j, MAX_WALL_TEXTURES);
	for (; i < j; i++)
		ReadColor (gameData.render.color.textures + i, loadFile, gameData.segs.nLevelVersion <= 15);
	}
}

//------------------------------------------------------------------------------

void LoadSideLightsCompiled (int i, CFILE *loadFile)
{
	tFaceColor	*pc;
	int			j;

gameData.render.shadows.nLights = 0;
#if LIGHTMAPS
if (gameStates.app.bD2XLevel) { 
	INIT_PROGRESS_LOOP (i, j, gameData.segs.nSegments * 6);
	pc = &gameData.render.color.lights [0][0] + i;
	for (; i < j; i++, pc++) {
		ReadColor (pc, loadFile, gameData.segs.nLevelVersion <= 13);
#if 0//SHADOWS
		RegisterLight (pc, (short) (i / 6), (short) (i % 6));
#endif
		}
	}
else {
#else
	{
#endif
#if 0//SHADOWS
	tSegment	*segP;
	tSide		*sideP;
	int		h;

	INIT_PROGRESS_LOOP (i, j, gameData.segs.nSegments);
	segP = gameData.segs.segments + i;
	for (i = 0; i < j; i++, segP++)
		for (h = 0, sideP = segP->sides; h < 6; h++, sideP++)
			if (IsLight (sideP->nBaseTex) || IsLight (sideP->nOvlTexf))
				RegisterLight (NULL, (short) i, (short) h);
#endif
	}
}

//------------------------------------------------------------------------------

void ComputeSegSideCenters (int nSegment)
{
	int			i, j, nSide;
	tSegment		*segP;
	tSide			*sideP;
#if USE_SEGRADS
	fix			xSideDists [6], xMinDist;
#endif

INIT_PROGRESS_LOOP (nSegment, j, gameData.segs.nSegments);

for (i = nSegment * 6, segP = gameData.segs.segments + nSegment; nSegment < j; nSegment++, segP++) {
	ComputeSegmentCenter (gameData.segs.segCenters [nSegment], segP);
#if USE_SEGRADS
	GetSideDists (gameData.segs.segCenters [nSegment], nSegment, xSideDists, 0);
	xMinDist = 0x7fffffff;
#endif
	for (nSide = 0, sideP = segP->sides; nSide < 6; nSide++, i++) {
		ComputeSideCenter (gameData.segs.sideCenters + i, segP, nSide);
#if USE_SEGRADS
		if (xMinDist > xSideDists [nSide])
			xMinDist = xSideDists [nSide];
#endif
		}
#if USE_SEGRADS
	gameData.segs.segRads [nSegment] = xMinDist;
#endif
	}
}

//------------------------------------------------------------------------------

static short loadIdx = 0;
static int loadOp = 0;
static CFILE *mineDataFile;

static void LoadSegmentsPoll (int nItems, tMenuItem *m, int *key, int cItem)
{
	int	bLightmaps = 0, bShadows = 0;

#if LIGHTMAPS
if (gameStates.app.bD2XLevel && gameStates.render.color.bLightMapsOk)
	bLightmaps = 1;
#endif
#if SHADOWS
bShadows = 1;
#endif

GrPaletteStepLoad (NULL);
if (loadOp == 0) {
	LoadSegmentsCompiled (loadIdx, mineDataFile);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 1;
		}
	}
else if (loadOp == 1) {
	ValidateSegmentAll ();			// Fill in tSide nType and normals.
	loadOp = 2;
	}
else if (loadOp == 2) {
	LoadSegment2sCompiled (mineDataFile);
	loadOp = 3;
	}
else if (loadOp == 3) {
	LoadVertLightsCompiled (loadIdx, mineDataFile);
	loadIdx += PROGRESS_INCR;
	if (!gameStates.app.bD2XLevel || (loadIdx >= gameData.segs.nVertices)) {
		loadIdx = 0;
		loadOp = 4;
		}
	}
else if (loadOp == 4) {
	LoadSideLightsCompiled (loadIdx, mineDataFile);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= (gameStates.app.bD2XLevel ? 
						 gameData.segs.nSegments * 6 : bShadows ? gameData.segs.nSegments : 1)) {
		loadIdx = 0;
		loadOp = 5;
		}
	}
else if (loadOp == 5) {
	LoadTexColorsCompiled (loadIdx, mineDataFile);
	loadIdx += PROGRESS_INCR;
	if (!gameStates.app.bD2XLevel || (loadIdx >= MAX_WALL_TEXTURES)) {
		loadIdx = 0;
		loadOp = 6;
		}
	}
else if (loadOp == 6) {
	ComputeSegSideCenters (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 7;
		}
	}
else {
	*key = -2;
	GrPaletteStepLoad (NULL);
	return;
	}
m [0].value++;
m [0].rebuild = 1;
*key = 0;
GrPaletteStepLoad (NULL);
}

//------------------------------------------------------------------------------

static void SortLightsPoll (int nItems, tMenuItem *m, int *key, int cItem)
{
GrPaletteStepLoad (NULL);
if (loadOp == 0) {
	ComputeNearestSegmentLights (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 1;
		}
	}
else if (loadOp == 1) {
	ComputeNearestVertexLights (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nVertices) {
		loadIdx = 0;
		loadOp = 2;
		}
	}
if (loadOp == 2) {
	*key = -2;
	GrPaletteStepLoad (NULL);
	return;
	}
m [0].value++;
m [0].rebuild = 1;
*key = 0;
GrPaletteStepLoad (NULL);
}

//------------------------------------------------------------------------------

int SortLightsGaugeSize (void)
{
return
#if !SHADOWS
	(!gameOpts->render.bDynLighting && gameStates.app.bD2XLevel) ? 0 :
#endif
	PROGRESS_STEPS (gameData.segs.nSegments) +
	PROGRESS_STEPS (gameData.segs.nVertices);
}

//------------------------------------------------------------------------------

int LoadMineGaugeSize (void)
{
	int	i = 2 * PROGRESS_STEPS (gameData.segs.nSegments) + 2;
	int	bLightmaps = 0, bShadows = 0;

#if LIGHTMAPS
	if (gameStates.render.color.bLightMapsOk)
		bLightmaps = 1;
#endif
#if SHADOWS
	bShadows = 1;
#endif
if (gameStates.app.bD2XLevel) {
	i += PROGRESS_STEPS (gameData.segs.nVertices) + PROGRESS_STEPS (MAX_WALL_TEXTURES);
	i += PROGRESS_STEPS (gameData.segs.nSegments * 6);
	}
else {
	i++;
	if (bShadows)
		i += PROGRESS_STEPS (gameData.segs.nSegments);
	else
		i++;
	}
//if (gameOpts->render.bDynLighting)
	i += SortLightsGaugeSize ();
return i;
}

//------------------------------------------------------------------------------

void LoadSegmentsGauge (CFILE *loadFile)
{
loadOp = 0;
loadIdx = 0;
mineDataFile = loadFile;
NMProgressBar (TXT_PREP_DESCENT, 0, LoadMineGaugeSize () + PagingGaugeSize (), LoadSegmentsPoll); 
}

//------------------------------------------------------------------------------

void SortLightsGauge (void)
{
loadOp = 0;
loadIdx = 0;
if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
	NMProgressBar (TXT_PREP_DESCENT, 
						LoadMineGaugeSize () - SortLightsGaugeSize (), 
						LoadMineGaugeSize () + PagingGaugeSize (), SortLightsPoll); 
else {
	ComputeNearestSegmentLights (-1);
	ComputeNearestVertexLights (-1);
	}
}

//------------------------------------------------------------------------------

int LoadMineSegmentsCompiled (CFILE *loadFile)
{
	int			i;
	ubyte			nCompiledVersion;
	ushort		temp_ushort = 0;
	char			*psz;

bD1PigPresent = CFExist (D1_PIGFILE, gameFolders.szDataDir, 0);

psz = strchr (gameData.segs.szLevelFilename, '.');
bNewFileFormat = !psz || strcmp (psz, ".sdl");
//	For compiled levels, textures map to themselves, prevent nTexOverride always being gray, 
//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
#ifdef EDITOR
for (i = 0; i < MAX_TEXTURES; i++)
	tmap_xlate_table [i] = i;
#endif

//	memset ( gameData.segs.segments, 0, sizeof (tSegment)*MAX_SEGMENTS );
FuelCenReset ();
InitTexColors ();
//=============================== Reading part ==============================
nCompiledVersion = CFReadByte (loadFile);
//Assert ( nCompiledVersion==COMPILED_MINE_VERSION );
#if TRACE
if (nCompiledVersion!=COMPILED_MINE_VERSION)
	con_printf (CON_DEBUG, "compiled mine version=%i\n", nCompiledVersion); //many levels have "wrong" versions.  Theres no point in aborting because of it, I think.
con_printf (CON_DEBUG, "   compiled mine version = %d\n", nCompiledVersion);
#endif
if (bNewFileFormat)
	gameData.segs.nVertices = CFReadShort (loadFile);
else
	gameData.segs.nVertices = CFReadInt (loadFile);
Assert ( gameData.segs.nVertices <= MAX_VERTICES );
#if TRACE
con_printf (CON_DEBUG, "   %d vertices\n", gameData.segs.nVertices);
#endif
if (bNewFileFormat)
	gameData.segs.nSegments = CFReadShort (loadFile);
else
	gameData.segs.nSegments = CFReadInt (loadFile);
Assert ( gameData.segs.nSegments <= MAX_SEGMENTS );
#if TRACE
con_printf (CON_DEBUG, "   %d segments\n", gameData.segs.nSegments);
#endif
for (i = 0; i < gameData.segs.nVertices; i++) {
	CFReadVector (gameData.segs.vertices+i, loadFile);
#if !FLOAT_COORD
	gameData.segs.fVertices [i].p.x = ((float) gameData.segs.vertices [i].p.x) / 65536.0f;
	gameData.segs.fVertices [i].p.y = ((float) gameData.segs.vertices [i].p.y) / 65536.0f;
	gameData.segs.fVertices [i].p.z = ((float) gameData.segs.vertices [i].p.z) / 65536.0f;
#endif
	}
memset (gameData.segs.segments, 0, MAX_SEGMENTS * sizeof (tSegment));
#if TRACE
con_printf (CON_DEBUG, "   loading segments ...\n");
#endif
gameData.segs.nLastVertex = gameData.segs.nVertices-1;
gameData.segs.nLastSegment = gameData.segs.nSegments-1;
if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
	LoadSegmentsGauge (loadFile);
else {
	LoadSegmentsCompiled (-1, loadFile);
	ValidateSegmentAll ();			// Fill in side type and normals.
	LoadSegment2sCompiled (loadFile);
	LoadVertLightsCompiled (-1, loadFile);
	LoadSideLightsCompiled (-1, loadFile);
	LoadTexColorsCompiled (-1, loadFile);
	ComputeSegSideCenters (-1);
	}
if (!HasTexColors ())
	InitTexColors ();
ResetObjects (1);		//one tObject, the player
#if !SHADOWS
if (gameOpts->render.bDynLighting || !gameStates.app.bD2XLevel) 
#endif
	{
	AddDynLights ();
	SortLightsGauge ();
	}
return 0;
}

//------------------------------------------------------------------------------
//eof
