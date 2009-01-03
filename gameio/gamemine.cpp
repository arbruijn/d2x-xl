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

#define LIGHT_VERSION 4

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
#include "ogl_defs.h"
#include "oof.h"
#include "lightmap.h"
#include "render.h"
#include "gameseg.h"

#include "game.h"
#include "menu.h"
#include "menu.h"

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
#include "network.h"
#include "light.h"
#include "lightprecalc.h"
#include "dynlight.h"
#include "renderlib.h"

//------------------------------------------------------------------------------

#define COMPILED_MINE_VERSION 0

#define REMOVE_EXT (s)  (* (strchr ((s), '.' ))='\0')

struct mtfi mine_top_fileinfo;    // Should be same as first two fields below...
struct mfi mine_fileinfo;
struct mh mine_header;
struct me mine_editor;

typedef struct v16_segment {
	#ifdef EDITOR
	short   nSegment;             // CSegment number, not sure what it means
	#endif
	CSide   sides [MAX_SIDES_PER_SEGMENT];       // 6 sides
	short   children [MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
	short   verts [MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
	#ifdef  EDITOR
	short   group;              // group number to which the CSegment belongs.
	#endif
	short   objects;            // pointer to OBJECTS in this CSegment
	ubyte   special;            // what nType of center this is
	sbyte   nMatCen;         // which center CSegment is associated with.
	short   value;
	fix     xAvgSegLight;       // average static light in CSegment
	#ifndef EDITOR
	short   pad;                // make structure longword aligned
	#endif
} v16_segment;

struct mfi_v19 {
	ushort  fileinfo_signature;
	ushort  fileinfoVersion;
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
	tGameItemInfo	CObject;
	int     unused_offset;      // was: doors.offset
	int     unused_howmamy;     // was: doors.count
	int     unused_sizeof;      // was: doors.size
	short   level_shake_frequency;  // Shakes every level_shake_frequency seconds
	short   level_shake_duration;   // for level_shake_duration seconds (on average, Random).  In 16ths second.
	int     secret_return_segment;
	CFixMatrix  secret_return_orient;
	tGameItemInfo	lightDeltaIndices;
	tGameItemInfo	lightDeltas;
};

int CreateDefaultNewSegment ();

bool bNewFileFormat = true; // "new file format" is everything newer than d1 shareware

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

//	-----------------------------------------------------------------------------------------------------------
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
//	 {246, 246, {321, 181}},
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
//	 {341, 341, {364, 364}},
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
//	 {351, 352, {-1, 374 - 351}},
	 {351, 351, {394, 394}},
	 {352, 352, {370, 370}},
	 {353, 353, {371, 371}},
//	 {354, 354, {377, 377}},
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

	for (h = sizeofa (nD1ToD2Texture), i = 0; i < h; i++)
		if ((nD1ToD2Texture [i].d1_min <= nD1Texture) && (nD1ToD2Texture [i].d1_max >= nD1Texture)) {
			if (nD1ToD2Texture [i].repl [0] == -1)	// -> repl [1] contains an offset
				return nD1Texture + nD1ToD2Texture [i].repl [1];
			else
				return nD1ToD2Texture [i].repl [bForce ? 0 : bD1PigPresent];
			}

 { // handle rare case where orientation != 0
		short nTexture = nD1Texture & TMAP_NUM_MASK;
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
int load_mine_data (CFile& cf)
{
	int   i, j, oldsizeadjust;
	short tmap_xlate;
	int 	translate;
	char 	*temptr;
	int	mine_start = CFTell ();
	bD1PigPresent = CFile::Exist (D1_PIGFILE);

	oldsizeadjust= (sizeof (int)*2)+sizeof (CFixMatrix);
	ResetGenerators ();

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
	mine_fileinfo.vertex_sizeof     =   sizeof (CFixVector);
	mine_fileinfo.segment_offset    =   -1;
	mine_fileinfo.segment_howmany   =   0;
	mine_fileinfo.segment_sizeof    =   sizeof (CSegment);
	mine_fileinfo.newseg_verts_offset     =   -1;
	mine_fileinfo.newseg_verts_howmany    =   0;
	mine_fileinfo.newseg_verts_sizeof     =   sizeof (CFixVector);
	mine_fileinfo.group_offset		  =	-1;
	mine_fileinfo.group_howmany	  =	0;
	mine_fileinfo.group_sizeof		  =	sizeof (group);
	mine_fileinfo.texture_offset    =   -1;
	mine_fileinfo.texture_howmany   =   0;
 	mine_fileinfo.texture_sizeof    =   FILENAME_LEN;  // num characters in a name
 	mine_fileinfo.walls.offset		  =	-1;
	mine_fileinfo.walls.count	  =	0;
	mine_fileinfo.walls.size		  =	sizeof (CWall);
 	mine_fileinfo.triggers.offset	  =	-1;
	mine_fileinfo.triggers.count  =	0;
	mine_fileinfo.triggers.size	  =	sizeof (CTrigger);
	mine_fileinfo.object.offset		=	-1;
	mine_fileinfo.object.count		=	1;
	mine_fileinfo.object.size		=	sizeof (CObject);

	mine_fileinfo.level_shake_frequency		=	0;
	mine_fileinfo.level_shake_duration		=	0;

	//	Delta light stuff for blowing out light sources.
//	if (mine_top_fileinfo.fileinfoVersion >= 19) {
		mine_fileinfo.gameData.render.lights.deltaIndices.offset		=	-1;
		mine_fileinfo.gameData.render.lights.deltaIndices.count		=	0;
		mine_fileinfo.gameData.render.lights.deltaIndices.size		=	sizeof (tLightDeltaIndex);

		mine_fileinfo.deltaLight.offset		=	-1;
		mine_fileinfo.deltaLight.count		=	0;
		mine_fileinfo.deltaLight.size		=	sizeof (tLightDelta);

//	}

	mine_fileinfo.segment2_offset		= -1;
	mine_fileinfo.segment2_howmany	= 0;
	mine_fileinfo.segment2_sizeof    = sizeof (CSegment);

	// Read in mine_top_fileinfo to get size of saved fileinfo.

	memset ( &mine_top_fileinfo, 0, sizeof (mine_top_fileinfo) );

	if (cf.Seek ( loadFile, mine_start, SEEK_SET ))
		Error ( "Error moving to top of file in gamemine.c" );

	if (cf.Read ( &mine_top_fileinfo, sizeof (mine_top_fileinfo), 1 )!=1)
		Error ( "Error reading mine_top_fileinfo in gamemine.c" );

	if (mine_top_fileinfo.fileinfo_signature != 0x2884)
		return -1;

	// Check version number
	if (mine_top_fileinfo.fileinfoVersion < COMPATIBLE_VERSION )
		return -1;

	// Now, Read in the fileinfo
	if (cf.Seek ( loadFile, mine_start, SEEK_SET ))
		Error ( "Error seeking to top of file in gamemine.c" );

	if (cf.Read ( &mine_fileinfo, mine_top_fileinfo.fileinfo_sizeof, 1 )!=1)
		Error ( "Error reading mine_fileinfo in gamemine.c" );

	if (mine_top_fileinfo.fileinfoVersion < 18) {
#if TRACE
		console.printf (1, "Old version, setting shake intensity to 0.\n");
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
		if (cf.Seek ( loadFile, mine_fileinfo.header_offset, SEEK_SET ))
			Error ( "Error seeking to header_offset in gamemine.c" );

		if (cf.Read ( &mine_header, mine_fileinfo.header_size, 1 )!=1)
			Error ( "Error reading mine_header in gamemine.c" );
	}

	//===================== READ EDITOR INFO ==========================

	// Set default values
	mine_editor.current_seg         =   0;
	mine_editor.newsegment_offset   =   -1; // To be written
	mine_editor.newsegment_size     =   sizeof (CSegment);
	mine_editor.Curside             =   0;
	mine_editor.Markedsegp          =   -1;
	mine_editor.Markedside          =   0;

	if (mine_fileinfo.editor_offset > -1 )
 {
		if (cf.Seek ( loadFile, mine_fileinfo.editor_offset, SEEK_SET ))
			Error ( "Error seeking to editor_offset in gamemine.c" );

		if (cf.Read ( &mine_editor, mine_fileinfo.editor_size, 1 )!=1)
			Error ( "Error reading mine_editor in gamemine.c" );
	}

	//===================== READ TEXTURE INFO ==========================

	if ((mine_fileinfo.texture_offset > -1) && (mine_fileinfo.texture_howmany > 0))
 {
		if (cf.Seek ( loadFile, mine_fileinfo.texture_offset, SEEK_SET ))
			Error ( "Error seeking to texture_offset in gamemine.c" );

		for (i=0; i< mine_fileinfo.texture_howmany; i++ )
	 {
			if (cf.Read ( &old_tmap_list [i], mine_fileinfo.texture_sizeof, 1 )!=1)
				Error ( "Error reading old_tmap_list [i] in gamemine.c" );
		}
	}

	//=============== GENERATE TEXTURE TRANSLATION TABLE ===============

	translate = 0;

	Assert (gameData.pig.tex.nTextures < MAX_TEXTURES);

 {
		CHashTable ht;

		HashTableInit ( &ht, gameData.pig.tex.nTextures );

		// Remove all the file extensions in the textures list

		for (i=0;i<gameData.pig.tex.nTextures;i++) {
			temptr = strchr (gameData.pig.tex.tMapInfoP [i].filename, '.');
			if (temptr) *temptr = '\0';
			HashTableInsert ( &ht, gameData.pig.tex.tMapInfoP [i].filename, i );
		}

		// For every texture, search through the texture list
		// to find a matching name.
		for (j=0;j<mine_fileinfo.texture_howmany;j++)  {
			// Remove this texture name's extension
			temptr = strchr (old_tmap_list [j], '.');
			if (temptr) *temptr = '\0';

			tmap_xlate_table [j] = HashTableSearch ( &ht, old_tmap_list [j]);
			if (tmap_xlate_table [j]	< 0 ) {
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
			console.printf (CON_DBG, "This mine has %d unique textures in it (~%d KB)\n", count, (count*4096) /1024 );
#endif
		}

		HashTableFree ( &ht );
	}

	//====================== READ VERTEX INFO ==========================

	// New check added to make sure we don't read in too many vertices.
	if ( mine_fileinfo.vertex_howmany > MAX_VERTICES )
	 {
#if TRACE
		console.printf (CON_DBG, "Num vertices exceeds maximum.  Loading MAX %d vertices\n", MAX_VERTICES);
#endif
		mine_fileinfo.vertex_howmany = MAX_VERTICES;
		}

	if ((mine_fileinfo.vertex_offset > -1) && (mine_fileinfo.vertex_howmany > 0))
 {
		if (cf.Seek ( loadFile, mine_fileinfo.vertex_offset, SEEK_SET ))
			Error ( "Error seeking to vertex_offset in gamemine.c" );

		for (i=0; i< mine_fileinfo.vertex_howmany; i++ )
	 {
			// Set the default values for this vertex
			gameData.segs.vertices [i].x = 1;
			gameData.segs.vertices [i].y = 1;
			gameData.segs.vertices [i].z = 1;

			if (cf.Read ( &gameData.segs.vertices [i], mine_fileinfo.vertex_sizeof, 1 )!=1)
				Error ( "Error reading gameData.segs.vertices [i] in gamemine.c" );
		}
	}

	//==================== READ SEGMENT INFO ===========================

	// New check added to make sure we don't read in too many segments.
	if ( mine_fileinfo.segment_howmany > MAX_SEGMENTS ) {
#if TRACE
		console.printf (CON_DBG, "Num segments exceeds maximum.  Loading MAX %d segments\n", MAX_SEGMENTS);
#endif
		mine_fileinfo.segment_howmany = MAX_SEGMENTS;
		mine_fileinfo.segment2_howmany = MAX_SEGMENTS;
	}

	// [commented out by mk on 11/20/94 (weren't we supposed to hit final in October?) because it looks redundant.  I think I'll test it now...]  ResetGenerators ();

	if ((mine_fileinfo.segment_offset > -1) && (mine_fileinfo.segment_howmany > 0)) {

		if (cf.Seek ( loadFile, mine_fileinfo.segment_offset, SEEK_SET ))

			Error ( "Error seeking to segment_offset in gamemine.c" );

		gameData.segs.nLastSegment = mine_fileinfo.segment_howmany-1;

		for (i=0; i< mine_fileinfo.segment_howmany; i++ ) {

			// Set the default values for this CSegment (clear to zero )
			//memset ( &SEGMENTS [i], 0, sizeof (CSegment) );

			if (mine_top_fileinfo.fileinfoVersion < 20) {
				v16_segment v16_seg;

				Assert (mine_fileinfo.segment_sizeof == sizeof (v16_seg);

				if (cf.Read ( &v16_seg, mine_fileinfo.segment_sizeof, 1 )!=1)
					Error ( "Error reading segments in gamemine.c" );

				#ifdef EDITOR
				SEGMENTS [i].nSegment = v16_seg.nSegment;
				// -- SEGMENTS [i].pad = v16_seg.pad;
				#endif

				for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
					SEGMENTS [i].m_sides [j] = v16_seg.m_sides [j];

				for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
					SEGMENTS [i].children [j] = v16_seg.children [j];

				for (j=0; j<MAX_VERTICES_PER_SEGMENT; j++)
					SEGMENTS [i].verts [j] = v16_seg.verts [j];

				SEGMENTS [i].m_nType = v16_seg.m_nType;
				SEGMENTS [i].value = v16_seg.value;
				SEGMENTS [i].s2Flags = 0;
				SEGMENTS [i].nMatCen = v16_seg.nMatCen;
				SEGMENTS [i].m_xAvgSegLight = v16_seg.m_xAvgSegLight;
				FuelCenActivate ( &SEGMENTS [i], SEGMENTS [i].m_nType );

			} else  {
				if (cf.Read (SEGMENTS + i, mine_fileinfo.segment_sizeof, 1 )!=1)
					Error ("Unable to read CSegment %i\n", i);
			}

			SEGMENTS [i].objects = -1;
			#ifdef EDITOR
			SEGMENTS [i].m_group = -1;
			#endif

			if (mine_top_fileinfo.fileinfoVersion < 15) {	//used old tUVL ranges
				int sn, uvln;

				for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++)
					for (uvln=0;uvln<4;uvln++) {
						SEGMENTS [i].m_sides [sn].uvls [uvln].u /= 64;
						SEGMENTS [i].m_sides [sn].uvls [uvln].v /= 64;
						SEGMENTS [i].m_sides [sn].uvls [uvln].l /= 32;
					}
			}

			if (translate == 1)
				for (j=0;j<MAX_SIDES_PER_SEGMENT;j++) {
					ushort orient;
					tmap_xlate = SEGMENTS [i].m_sides [j].m_nBaseTex;
					SEGMENTS [i].m_sides [j].m_nBaseTex = tmap_xlate_table [tmap_xlate];
					if ((WALL_IS_DOORWAY (SEGMENTS + i, j, NULL) & WID_RENDER_FLAG))
						if (SEGMENTS [i].m_sides [j].m_nBaseTex < 0) {
#if TRACE
							console.printf (CON_DBG, "Couldn't find texture '%s' for Segment %d, CSide %d\n", old_tmap_list [tmap_xlate], i, j);
#endif
							Int3 ();
							SEGMENTS [i].m_sides [j].m_nBaseTex = gameData.pig.tex.nTextures-1;
						}
					tmap_xlate = SEGMENTS [i].m_sides [j].nOvlTex;
					if (tmap_xlate != 0) {
						int xlated_tmap = tmap_xlate_table [tmap_xlate];

						if ((WALL_IS_DOORWAY (SEGMENTS + i, j, NULL) & WID_RENDER_FLAG))
							if (xlated_tmap <= 0) {
#if TRACE
								console.printf (CON_DBG, "Couldn't find texture '%s' for Segment %d, CSide %d\n", old_tmap_list [tmap_xlate], i, j);
#endif
								Int3 ();
								SEGMENTS [i].m_sides [j].nOvlTex = gameData.pig.tex.nTextures - 1;
							}
						SEGMENTS [i].m_sides [j].nOvlTex = xlated_tmap;
					}
				}
		}


		if (mine_top_fileinfo.fileinfoVersion >= 20)
			for (i = 0; i<=gameData.segs.nLastSegment; i++) {
				cf.Read (SEGMENTS + i, sizeof (CSegment), 1);
				FuelCenActivate (SEGMENTS + i, SEGMENTS [i].m_nType );
			}
	}

	//===================== READ NEWSEGMENT INFO =====================

	#ifdef EDITOR

 {		// Default CSegment created.
		CFixVector	sizevec;
		med_create_new_segment (VmVecMake (&sizevec, DEFAULT_X_SIZE, DEFAULT_Y_SIZE, DEFAULT_Z_SIZE);		// New_segment = SEGMENTS [0];
		//memset ( &New_segment, 0, sizeof (CSegment) );
	}

	if (mine_editor.newsegment_offset > -1)
 {
		if (cf.Seek ( loadFile, mine_editor.newsegment_offset, SEEK_SET ))
			Error ( "Error seeking to newsegment_offset in gamemine.c" );
		if (cf.Read ( &New_segment, mine_editor.newsegment_size, 1 )!=1)
			Error ( "Error reading new_segment in gamemine.c" );
	}

	if ((mine_fileinfo.newseg_verts_offset > -1) && (mine_fileinfo.newseg_verts_howmany > 0))
 {
		if (cf.Seek ( loadFile, mine_fileinfo.newseg_verts_offset, SEEK_SET ))
			Error ( "Error seeking to newseg_verts_offset in gamemine.c" );
		for (i=0; i< mine_fileinfo.newseg_verts_howmany; i++ )
	 {
			// Set the default values for this vertex
			gameData.segs.vertices [NEW_SEGMENT_VERTICES+i].x = 1;
			gameData.segs.vertices [NEW_SEGMENT_VERTICES+i].y = 1;
			gameData.segs.vertices [NEW_SEGMENT_VERTICES+i].z = 1;

			if (cf.Read ( &gameData.segs.vertices [NEW_SEGMENT_VERTICES+i], mine_fileinfo.newseg_verts_sizeof, 1 )!=1)
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
		Cursegp = mine_editor.current_seg + SEGMENTS;
	else
 		Cursegp = NULL;

	if (mine_editor.Markedsegp != -1 )
		Markedsegp = mine_editor.Markedsegp + SEGMENTS;
	else
		Markedsegp = NULL;

	num_groups = 0;
	current_group = -1;

	#endif

	gameData.segs.nVertices = mine_fileinfo.vertex_howmany;
	gameData.segs.nSegments = mine_fileinfo.segment_howmany;
	gameData.segs.nLastVertex = gameData.segs.nVertices-1;
	gameData.segs.nLastSegment = gameData.segs.nSegments-1;

	ResetObjects (1);		//one CObject, the CPlayerData

	#ifdef EDITOR
	gameData.segs.nLastVertex = MAX_SEGMENT_VERTICES-1;
	gameData.segs.nLastSegment = MAX_SEGMENTS-1;
	set_vertexCounts ();
	gameData.segs.nLastVertex = gameData.segs.nVertices-1;
	gameData.segs.nLastSegment = gameData.segs.nSegments-1;

	warn_if_concave_segments ();
	#endif

	#ifdef EDITOR
		SetupSegments ();
	#endif

	//create_local_segment_data ();

	//gamemine_find_textures ();

	if (mine_top_fileinfo.fileinfoVersion < MINE_VERSION )
		return 1;		//old version
	else
		return 0;

}
#endif

//------------------------------------------------------------------------------

void ReadColor (CFile& cf, tFaceColor *pc, int bFloatData, int bRegisterColor)
{
pc->index = cf.ReadByte ();
if (bFloatData) {
	tRgbColord	c;
	cf.Read (&c, sizeof (c), 1);
	pc->color.red = (float) c.red;
	pc->color.green = (float) c.green;
	pc->color.blue = (float) c.blue;
	}
else {
	int c = cf.ReadInt ();
	pc->color.red = (float) c / (float) 0x7fffffff;
	c = cf.ReadInt ();
	pc->color.green = (float) c / (float) 0x7fffffff;
	c = cf.ReadInt ();
	pc->color.blue = (float) c / (float) 0x7fffffff;
	}
if (bRegisterColor &&
	 (((pc->color.red > 0) && (pc->color.red < 1)) ||
	 ((pc->color.green > 0) && (pc->color.green < 1)) ||
	 ((pc->color.blue > 0) && (pc->color.blue < 1))))
	gameStates.render.bColored = 1;
pc->color.alpha = 1;
}

//------------------------------------------------------------------------------

#if DBG
tFace *FindDupFace (short nSegment, short nSide)
{
	tSegFaces	*segFaceP = SEGFACES + nSegment;
	tFace		*faceP0, *faceP1;
	int			i, j;

for (i = segFaceP->nFaces, faceP0 = segFaceP->pFaces; i; faceP0++, i--)
	if (faceP0->nSide == nSide)
		break;
for (i = 0, segFaceP = SEGFACES.Buffer (); i < gameData.segs.nSegments; i++, segFaceP++) {
	for (j = segFaceP->nFaces, faceP1 = segFaceP->pFaces; j; faceP1++, j--) {
		if (faceP1 == faceP0)
			continue;
		if ((faceP1->nIndex == faceP0->nIndex) || !memcmp (faceP1->index, faceP0->index, sizeof (faceP0->index)))
			return faceP1;
		}
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

void LoadSegmentsCompiled (short nSegment, CFile& cf)
{
	short			nLastSeg;

INIT_PROGRESS_LOOP (nSegment, nLastSeg, gameData.segs.nSegments);
for (; nSegment < nLastSeg; nSegment++) {
#ifdef EDITOR
	segP->nSegment = nSegment;
	segP->m_group = 0;
#endif
#if DBG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	SEGFACES [nSegment].nFaces = 0;
	SEGMENTS [nSegment].Read (cf);
	}
}

//------------------------------------------------------------------------------

void LoadExtSegmentsCompiled (CFile& cf)
{
	int	i;

gameData.matCens.nRepairCenters = 0;
for (i = 0; i < gameData.segs.nSegments; i++) {
	if (gameData.segs.nLevelVersion > 5)
		SEGMENTS [i].ReadExtras (cf);
	SEGMENTS [i].CreateGenerator (SEGMENTS [i].m_nType);
	}
}

//------------------------------------------------------------------------------

void LoadVertLightsCompiled (int i, CFile& cf)
{
	int	j;

gameData.render.shadows.nLights = 0;
if (gameStates.app.bD2XLevel) {
	INIT_PROGRESS_LOOP (i, j, gameData.segs.nVertices);
	for (; i < j; i++) {
		ReadColor (cf, gameData.render.color.ambient + i, gameData.segs.nLevelVersion <= 14, 1);
		}
	}
}

//------------------------------------------------------------------------------

int HasColoredLight (void)
{
	int			i, bColored = 0;
	tFaceColor	*pvc = gameData.render.color.ambient.Buffer ();

if (!gameStates.app.bD2XLevel)
	return 0;
// get the default colors
for (i = 0; i < gameData.segs.nVertices; i++, pvc++) {
#if 0
	if (pvc->index <= 0)
		continue;
#endif
	if ((pvc->color.red == 0) && (pvc->color.green == 0) && (pvc->color.blue == 0)) {
		pvc->index = 0;
		continue;
		}
	if ((pvc->color.red < 1) || (pvc->color.green < 1) || (pvc->color.blue < 1))
		bColored = 1;
	}
return bColored;
}

//------------------------------------------------------------------------------

void InitTexColors (void)
{
	int			i;
	tFaceColor	*pf = gameData.render.color.textures.Buffer ();
	int			bBW = gameStates.app.bNostalgia || !gameOpts->render.color.bAmbientLight;

for (i = 0; i < MAX_WALL_TEXTURES; i++, pf++) {
	pf->index = IsLight (i);
	if (bBW)
		pf->color.red =
		pf->color.green =
		pf->color.blue = 1;
	}
}

//------------------------------------------------------------------------------

void LoadTexColorsCompiled (int i, CFile& cf)
{
	int			j;

// get the default colors
if (gameStates.app.bD2XLevel) {
	INIT_PROGRESS_LOOP (i, j, MAX_WALL_TEXTURES);
	for (; i < j; i++)
		ReadColor (cf, gameData.render.color.textures + i, gameData.segs.nLevelVersion <= 15,
					  gameOpts->render.nLightingMethod);
	}
}

//------------------------------------------------------------------------------

void LoadSideLightsCompiled (int i, CFile& cf)
{
	tFaceColor	*pc;
	int			j;

gameData.render.shadows.nLights = 0;
#if LIGHTMAPS
if (gameStates.app.bD2XLevel) {
	INIT_PROGRESS_LOOP (i, j, gameData.segs.nSegments * 6);
	pc = gameData.render.color.lights + i;
	for (; i < j; i++, pc++) {
		ReadColor (cf, pc, gameData.segs.nLevelVersion <= 13, 1);
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
	CSegment	*segP;
	CSide		*sideP;
	int		h;

	INIT_PROGRESS_LOOP (i, j, gameData.segs.nSegments);
	segP = SEGMENTS + i;
	for (i = 0; i < j; i++, segP++)
		for (h = 0, sideP = segP->m_sides; h < 6; h++, sideP++)
			if (IsLight (sideP->m_nBaseTex) || IsLight (sideP->nOvlTexf))
				RegisterLight (NULL, (short) i, (short) h);
#endif
	}
}

//------------------------------------------------------------------------------

void ComputeSegSideCenters (int nSegment)
{
	int			i, j, nSide;
	CSegment*	segP;
	CSide*		sideP;
#if CALC_SEGRADS
	fix			xSideDists [6], xMinDist;
#endif

INIT_PROGRESS_LOOP (nSegment, j, gameData.segs.nSegments);

for (i = nSegment * 6, segP = SEGMENTS + nSegment; nSegment < j; nSegment++, segP++) {
	segP->ComputeCenter ();
#if CALC_SEGRADS
	segP->GetSideDists (segP->m_vCenter, xSideDists, 0);
	xMinDist = 0x7fffffff;
#endif
	for (nSide = 0, sideP = segP->m_sides; nSide < 6; nSide++, i++) {
		segP->ComputeSideCenter (nSide);
#if CALC_SEGRADS
		if (xMinDist > xSideDists [nSide])
			xMinDist = xSideDists [nSide];
#endif
		}
#if CALC_SEGRADS
	segP->ComputeRads (xMinDist);
#endif
	}
}

//------------------------------------------------------------------------------

static int loadIdx = 0;
static int loadOp = 0;
static CFile *mineDataFile;

static int LoadSegmentsPoll (CMenu& menu, int& key, int nCurItem)
{
	int	bLightmaps = 0, bShadows = 0;

#if LIGHTMAPS
if (gameStates.app.bD2XLevel && gameStates.render.color.bLightmapsOk)
	bLightmaps = 1;
#endif
#if SHADOWS
bShadows = 1;
#endif

paletteManager.LoadEffect  ();
if (loadOp == 0) {
	LoadSegmentsCompiled (loadIdx, *mineDataFile);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 1;
		}
	}
else if (loadOp == 1) {
	SetupSegments ();			// Fill in CSide nType and normals.
	loadOp++;
	}
else if (loadOp == 2) {
	ComputeSegSideCenters (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp++;
		}
	}
else if (loadOp == 4) {
	LoadExtSegmentsCompiled (*mineDataFile);
	loadOp++;
	}
else if (loadOp == 5) {
	LoadVertLightsCompiled (loadIdx, *mineDataFile);
	loadIdx += PROGRESS_INCR;
	if (!gameStates.app.bD2XLevel || (loadIdx >= gameData.segs.nVertices)) {
		loadIdx = 0;
		loadOp++;
		}
	}
else if (loadOp == 6) {
	LoadSideLightsCompiled (loadIdx, *mineDataFile);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= (gameStates.app.bD2XLevel ?
						 gameData.segs.nSegments * 6 : bShadows ? gameData.segs.nSegments : 1)) {
		loadIdx = 0;
		loadOp++;
		}
	}
else if (loadOp == 6) {
	LoadTexColorsCompiled (loadIdx, *mineDataFile);
	loadIdx += PROGRESS_INCR;
	if (!gameStates.app.bD2XLevel || (loadIdx >= MAX_WALL_TEXTURES)) {
		loadIdx = 0;
		loadOp++;
		}
	}
else {
	key = -2;
	paletteManager.LoadEffect  ();
	return nCurItem;
	}
menu [0].m_value++;
menu [0].m_bRebuild = 1;
key = 0;
paletteManager.LoadEffect  ();
return nCurItem;
}

//------------------------------------------------------------------------------

int LoadMineGaugeSize (void)
{
	int	i = 2 * PROGRESS_STEPS (gameData.segs.nSegments) + 2;
	int	bLightmaps = 0, bShadows = 0;

#if LIGHTMAPS
	if (gameStates.render.color.bLightmapsOk)
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
return i;
}

//------------------------------------------------------------------------------

void LoadSegmentsGauge (CFile& cf)
{
loadOp = 0;
loadIdx = 0;
mineDataFile = &cf;
ProgressBar (TXT_PREP_DESCENT, 0, LoadMineGaugeSize () + PagingGaugeSize () + SortLightsGaugeSize (), LoadSegmentsPoll);
}

//------------------------------------------------------------------------------

int LoadMineSegmentsCompiled (CFile& cf)
{
	int			i;
	ubyte			nCompiledVersion;
	char			*psz;

gameData.segs.vMin.Set (0x7fffffff, 0x7fffffff, 0x7fffffff);
/*	[X] =
gameData.segs.vMin[Y] =
gameData.segs.vMin[Y] = 0x7fffffff;*/
gameData.segs.vMax.Set (-0x7fffffff, -0x7fffffff, -0x7fffffff);
/*[X] =
gameData.segs.vMax[X] =
gameData.segs.vMax[Y] =
gameData.segs.vMax[Y] = -0x7fffffff;*/
gameStates.render.bColored = 0;
bD1PigPresent = CFile::Exist (D1_PIGFILE, gameFolders.szDataDir, 0);
psz = strchr (gameData.segs.szLevelFilename, '.');
bNewFileFormat = !psz || strcmp (psz, ".sdl");
//	For compiled levels, textures map to themselves, prevent nTexOverride always being gray,
//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
#ifdef EDITOR
for (i = 0; i < MAX_TEXTURES; i++)
	tmap_xlate_table [i] = i;
#endif

//	memset ( SEGMENTS, 0, sizeof (CSegment)*MAX_SEGMENTS );
ResetGenerators ();
//=============================== Reading part ==============================
nCompiledVersion = cf.ReadByte ();
//Assert ( nCompiledVersion==COMPILED_MINE_VERSION );
#if TRACE
if (nCompiledVersion != COMPILED_MINE_VERSION)
	console.printf (CON_DBG, "compiled mine version=%i\n", nCompiledVersion); //many levels have "wrong" versions.  Theres no point in aborting because of it, I think.
console.printf (CON_DBG, "   compiled mine version = %d\n", nCompiledVersion);
#endif
gameData.segs.nVertices = bNewFileFormat ? cf.ReadShort () : cf.ReadInt ();
Assert (gameData.segs.nVertices <= MAX_VERTICES);
#if TRACE
console.printf (CON_DBG, "   %d vertices\n", gameData.segs.nVertices);
#endif
gameData.segs.nSegments = bNewFileFormat ? cf.ReadShort () : cf.ReadInt ();
if (gameData.segs.nSegments >= MAX_SEGMENTS) {
	Warning (TXT_LEVEL_TOO_LARGE);
	return -1;
	}
gameData.Create ();
#if TRACE
console.printf (CON_DBG, "   %d segments\n", gameData.segs.nSegments);
#endif
for (i = 0; i < gameData.segs.nVertices; i++) {
	cf.ReadVector (gameData.segs.vertices [i]);
#if !FLOAT_COORD
	gameData.segs.fVertices [i][X] = X2F (gameData.segs.vertices [i][X]);
	gameData.segs.fVertices [i][Y] = X2F (gameData.segs.vertices [i][Y]);
	gameData.segs.fVertices [i][Z] = X2F (gameData.segs.vertices [i][Z]);
#endif
	}
if (gameData.segs.vMin [X] > gameData.segs.vertices [i][X])
	gameData.segs.vMin [X] = gameData.segs.vertices [i][X];
if (gameData.segs.vMin [Y] > gameData.segs.vertices [i][Y])
	gameData.segs.vMin [Y] = gameData.segs.vertices [i][Y];
if (gameData.segs.vMin [Z] > gameData.segs.vertices [i][Z])
	gameData.segs.vMin [Z] = gameData.segs.vertices [i][Z];
if (gameData.segs.vMax [X] < gameData.segs.vertices [i][X])
	gameData.segs.vMax [X] = gameData.segs.vertices [i][X];
if (gameData.segs.vMax [Y] < gameData.segs.vertices [i][Y])
	gameData.segs.vMax [Y] = gameData.segs.vertices [i][Y];
if (gameData.segs.vMax [Z] < gameData.segs.vertices [i][Z])
	gameData.segs.vMax [Z] = gameData.segs.vertices [i][Z];
SEGMENTS.Clear ();
#if TRACE
console.printf (CON_DBG, "   loading segments ...\n");
#endif
gameData.segs.nLastVertex = gameData.segs.nVertices - 1;
gameData.segs.nLastSegment = gameData.segs.nSegments - 1;
if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
	LoadSegmentsGauge (cf);
else {
	LoadSegmentsCompiled (-1, cf);
	SetupSegments ();			// Fill in side type and normals.
	LoadExtSegmentsCompiled (cf);
	LoadVertLightsCompiled (-1, cf);
	LoadSideLightsCompiled (-1, cf);
	LoadTexColorsCompiled (-1, cf);
	ComputeSegSideCenters (-1);
	}
gameData.segs.fRad = X2F (CFixVector::Dist(gameData.segs.vMax, gameData.segs.vMin));
ResetObjects (1);		//one CObject, the player
return 0;
}

//------------------------------------------------------------------------------
//eof
