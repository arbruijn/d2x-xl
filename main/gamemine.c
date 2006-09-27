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

/*
 *
 * Functions for loading mines in the game
 *
 * Old Log:
 * Revision 1.2  1995/10/31  10:15:58  allender
 * code for shareware levels
 *
 * Revision 1.1  1995/05/16  15:25:29  allender
 * Initial revision
 *
 * Revision 2.2  1995/03/06  15:23:14  john
 * New screen techniques.
 *
 * Revision 2.1  1995/02/27  13:13:37  john
 * Removed floating point.
 *
 * Revision 2.0  1995/02/27  11:27:45  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.70  1995/02/13  20:35:09  john
 * Lintized
 *
 * Revision 1.69  1995/02/07  17:12:03  rob
 * Added ifdef's for Editor.
 *
 * Revision 1.68  1995/02/07  16:51:48  mike
 * fix gray rock josh problem.
 *
 * Revision 1.67  1995/02/01  15:46:26  yuan
 * Fixed matcen_nums.
 *
 * Revision 1.66  1995/01/19  15:19:28  mike
 * new super-compressed registered file format.
 *
 * Revision 1.65  1994/12/10  16:44:59  matt
 * Added debugging code to track down door that turns into rock
 *
 * Revision 1.64  1994/12/10  14:58:24  yuan
 * *** empty log message ***
 *
 * Revision 1.63  1994/12/08  17:19:10  yuan
 * Cfiling stuff.
 *
 * Revision 1.62  1994/12/07  14:05:33  yuan
 * Fixed wall assert problem... Bashed highest_segment
 * _index before WALL_IS_DOORWAY check.
 *
 * Revision 1.61  1994/11/27  23:14:52  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.60  1994/11/27  18:05:20  matt
 * Compile out LVL reader when editor compiled out
 *
 * Revision 1.59  1994/11/26  22:51:45  matt
 * Removed editor-only fields from segment structure when editor is compiled
 * out, and padded segment structure to even multiple of 4 bytes.
 *
 * Revision 1.58  1994/11/26  21:48:02  matt
 * Fixed saturation in short light value
 *
 * Revision 1.57  1994/11/20  22:11:49  mike
 * comment out an apparently unnecessary call to FuelCenReset ().
 *
 * Revision 1.56  1994/11/18  21:56:42  john
 * Added a better, leaner pig format.
 *
 * Revision 1.55  1994/11/17  20:09:18  john
 * Added new compiled level format.
 *
 * Revision 1.54  1994/11/17  15:40:17  mike
 * Comment out con_printf which was causing important information to scroll away.
 *
 * Revision 1.53  1994/11/17  14:56:37  mike
 * moved segment validation functions from editor to main.
 *
 * Revision 1.52  1994/11/17  11:39:35  matt
 * Ripped out code to load old mines
 *
 * Revision 1.51  1994/11/14  20:47:53  john
 * Attempted to strip out all the code in the game
 * directory that uses any ui code.
 *
 * Revision 1.50  1994/11/14  16:05:38  matt
 * Fixed, maybe, again, errors when can't find texture during remap
 *
 * Revision 1.49  1994/11/14  14:34:03  matt
 * Fixed up handling when textures can't be found during remap
 *
 * Revision 1.48  1994/11/14  13:01:55  matt
 * Added Int3 () when can't find texture
 *
 * Revision 1.47  1994/10/30  14:12:21  mike
 * rip out local segments stuff.
 *
 * Revision 1.46  1994/10/27  19:43:07  john
 * Disable the piglet option.
 *
 * Revision 1.45  1994/10/27  18:51:42  john
 * Added -piglet option that only loads needed textures for a
 * mine.  Only saved ~1MB, and code still doesn't d_free textures
 * before you load a new mine.
 *
 * Revision 1.44  1994/10/20  12:47:22  matt
 * Replace old save files (MIN/SAV/HOT) with new LVL files
 *
 * Revision 1.43  1994/10/19  16:46:40  matt
 * Made tmap overrides for robots remap texture numbers
 *
 * Revision 1.42  1994/10/03  23:37:01  mike
 * Adapt to changed FuelCenActivate parameters.
 *
 * Revision 1.41  1994/09/23  22:14:49  matt
 * Took out obsolete structure fields
 *
 * Revision 1.40  1994/08/01  11:04:11  yuan
 * New materialization centers.
 *
 * Revision 1.39  1994/07/21  19:01:47  mike
 * Call Lsegment stuff.
 *
 *
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
	short   segnum;             // segment number, not sure what it means
	#endif
	side    sides [MAX_SIDES_PER_SEGMENT];       // 6 sides
	short   children [MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
	short   verts [MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
	#ifdef  EDITOR
	short   group;              // group number to which the segment belongs.
	#endif
	short   objects;            // pointer to gameData.objs.objects in this segment
	ubyte   special;            // what type of center this is
	sbyte   matcen_num;         // which center segment is associated with.
	short   value;
	fix     static_light;       // average static light in segment
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
	game_item_info	walls;
	game_item_info	triggers;
	game_item_info	links;
	game_item_info	object;
	int     unused_offset;      // was: doors.offset
	int     unused_howmamy;     // was: doors.count
	int     unused_sizeof;      // was: doors.size
	short   level_shake_frequency;  // Shakes every level_shake_frequency seconds
	short   level_shake_duration;   // for level_shake_duration seconds (on average, random).  In 16ths second.
	int     secret_return_segment;
	vms_matrix  secret_return_orient;
	game_item_info	lightDeltaIndices;
	game_item_info	lightDeltas;
};

int CreateDefaultNewSegment ();

int bNewFileFormat = 1; // "new file format" is everything newer than d1 shareware

int d1_pig_present = 0; // can descent.pig from descent 1 be loaded?

/* returns nonzero if d1_tmap_num references a texture which isn't available in d2. */
int d1_tmap_num_unique (short d1_tmap_num) 
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
	if (t == d1_tmap_num)
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

typedef struct d1_to_d2_tmap {
	short	d1_min, d1_max;
	short	repl [2];
} d1_to_d2_tmap;

short convert_d1_tmap_num (short d1_tmap_num) 
{
	int h, i;

	static d1_to_d2_tmap d1_to_d2_map [] = {
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

	if (gameStates.app.bHaveD1Data)
		return d1_tmap_num;
	if ((d1_tmap_num > 370) && (d1_tmap_num < 584)) {
		if (bNewFileFormat) {
			if (d1_tmap_num == 550) 
				return 615;
			return d1_tmap_num + 64;
			}
		// d1 shareware needs special treatment:
		if (d1_tmap_num < 410) 
			return d1_tmap_num + 68;
		if (d1_tmap_num < 417) 
			return d1_tmap_num + 73;
		if (d1_tmap_num < 446) 
			return d1_tmap_num + 91;
		if (d1_tmap_num < 453) 
			return d1_tmap_num + 104;
		if (d1_tmap_num < 462) 
			return d1_tmap_num + 111;
		if (d1_tmap_num < 486) 
			return d1_tmap_num + 117;
		if (d1_tmap_num < 494) 
			return d1_tmap_num + 141;
		if (d1_tmap_num < 584) 
			return d1_tmap_num + 147;
		}

	for (h = sizeof (d1_to_d2_map) / sizeof (d1_to_d2_tmap), i = 0; i < h; i++)
		if ((d1_to_d2_map [i].d1_min <= d1_tmap_num) && (d1_to_d2_map [i].d1_max >= d1_tmap_num)) {
			if (d1_to_d2_map [i].repl [0] == -1)	// -> repl [1] contains an offset
				return d1_tmap_num + d1_to_d2_map [i].repl [1];
			else
				return d1_to_d2_map [i].repl [d1_pig_present];
			}

	{ // handle rare case where orientation != 0
		short tmap_num = d1_tmap_num &  TMAP_NUM_MASK;
		short orient = d1_tmap_num & ~TMAP_NUM_MASK;
	if (orient)
		return orient | convert_d1_tmap_num (tmap_num);
	Warning (TXT_D1TEXTURE, tmap_num);
		return d1_tmap_num;
	}
}

#ifdef EDITOR

static char old_tmap_list [MAX_TEXTURES][FILENAME_LEN];
short tmap_xlate_table [MAX_TEXTURES];
static short tmap_times_used [MAX_TEXTURES];

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
	d1_pig_present = CFExist (D1_PIGFILE);

	oldsizeadjust= (sizeof (int)*2)+sizeof (vms_matrix);
	FuelCenReset ();

	for (i=0; i<MAX_TEXTURES; i++ )
		tmap_times_used [i] = 0;

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
	mine_fileinfo.vertex_sizeof     =   sizeof (vms_vector);
	mine_fileinfo.segment_offset    =   -1;
	mine_fileinfo.segment_howmany   =   0;
	mine_fileinfo.segment_sizeof    =   sizeof (segment);
	mine_fileinfo.newseg_verts_offset     =   -1;
	mine_fileinfo.newseg_verts_howmany    =   0;
	mine_fileinfo.newseg_verts_sizeof     =   sizeof (vms_vector);
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
	mine_fileinfo.triggers.size	  =	sizeof (trigger);  
	mine_fileinfo.object.offset		=	-1;
	mine_fileinfo.object.count		=	1;
	mine_fileinfo.object.size		=	sizeof (object);  

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
	mine_editor.newsegment_size     =   sizeof (segment);
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
				tmap_times_used [tmap_xlate_table [j]]++;
		}
	
		{
			int count = 0;
			for (i=0; i<MAX_TEXTURES; i++ )
				if (tmap_times_used [i])
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

			// Set the default values for this segment (clear to zero )
			//memset ( &gameData.segs.segments [i], 0, sizeof (segment) );

			if (mine_top_fileinfo.fileinfo_version < 20) {
				v16_segment v16_seg;

				Assert (mine_fileinfo.segment_sizeof == sizeof (v16_seg);

				if (CFRead ( &v16_seg, mine_fileinfo.segment_sizeof, 1, loadFile )!=1)
					Error ( "Error reading segments in gamemine.c" );

				#ifdef EDITOR
				gameData.segs.segments [i].segnum = v16_seg.segnum;
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
				gameData.segs.segment2s [i].s2_flags = 0;
				gameData.segs.segment2s [i].matcen_num = v16_seg.matcen_num;
				gameData.segs.segment2s [i].static_light = v16_seg.static_light;
				FuelCenActivate ( &gameData.segs.segments [i], gameData.segs.segment2s [i].special );

			} else  {
				if (CFRead (gameData.segs.segments + i, mine_fileinfo.segment_sizeof, 1, loadFile )!=1)
					Error ("Unable to read segment %i\n", i);
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
					tmap_xlate = gameData.segs.segments [i].sides [j].tmap_num;
					gameData.segs.segments [i].sides [j].tmap_num = tmap_xlate_table [tmap_xlate];
					if ((WALL_IS_DOORWAY (gameData.segs.segments + i, j, NULL) & WID_RENDER_FLAG))
						if (gameData.segs.segments [i].sides [j].tmap_num < 0)	{
#if TRACE
							con_printf (CON_DEBUG, "Couldn't find texture '%s' for Segment %d, side %d\n", old_tmap_list [tmap_xlate], i, j);
#endif
							Int3 ();
							gameData.segs.segments [i].sides [j].tmap_num = gameData.pig.tex.nTextures-1;
						}
					tmap_xlate = gameData.segs.segments [i].sides [j].tmap_num2 & TMAP_NUM_MASK;
					orient = gameData.segs.segments [i].sides [j].tmap_num2 & (~TMAP_NUM_MASK);
					if (tmap_xlate != 0) {
						int xlated_tmap = tmap_xlate_table [tmap_xlate];

						if ((WALL_IS_DOORWAY (gameData.segs.segments + i, j, NULL) & WID_RENDER_FLAG))
							if (xlated_tmap <= 0)	{
#if TRACE
								con_printf (CON_DEBUG, "Couldn't find texture '%s' for Segment %d, side %d\n", old_tmap_list [tmap_xlate], i, j);
#endif
								Int3 ();
								gameData.segs.segments [i].sides [j].tmap_num2 = gameData.pig.tex.nTextures-1;
							}
						gameData.segs.segments [i].sides [j].tmap_num2 = xlated_tmap | orient;
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

	{		// Default segment created.
		vms_vector	sizevec;
		med_create_new_segment (VmVecMake (&sizevec, DEFAULT_X_SIZE, DEFAULT_Y_SIZE, DEFAULT_Z_SIZE);		// New_segment = gameData.segs.segments [0];
		//memset ( &New_segment, 0, sizeof (segment) );
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

	ResetObjects (1);		//one object, the player

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

void read_children (int segnum, ubyte bit_mask, CFILE *loadFile)
{
	int bit;

	for (bit=0; bit<MAX_SIDES_PER_SEGMENT; bit++) {
		if (bit_mask & (1 << bit)) {
			gameData.segs.segments [segnum].children [bit] = CFReadShort (loadFile);
		} else
			gameData.segs.segments [segnum].children [bit] = -1;
	}
}

//------------------------------------------------------------------------------

void ReadColor (tFaceColor *pc, CFILE *loadFile, int bFloatData)
{

pc->index = CFReadByte (loadFile);
if (bFloatData)
	CFRead (&pc->color, sizeof (pc->color), 1, loadFile);
else {
	int c = CFReadInt (loadFile);
	pc->color.red = (float) c / (float) 0x7fffffff;
	c = CFReadInt (loadFile);
	pc->color.green = (float) c / (float) 0x7fffffff;
	c = CFReadInt (loadFile);
	pc->color.blue = (float) c / (float) 0x7fffffff;
	}
}

//------------------------------------------------------------------------------

void read_verts (int segnum, CFILE *loadFile)
{
	int i;
	// Read short gameData.segs.segments [segnum].verts [MAX_VERTICES_PER_SEGMENT]
	for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
		gameData.segs.segments [segnum].verts [i] = CFReadShort (loadFile);
}

//------------------------------------------------------------------------------

void read_special (int segnum, ubyte bit_mask, CFILE *loadFile)
{
	if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
		// Read ubyte	gameData.segs.segment2s [segnum].special
		gameData.segs.segment2s [segnum].special = CFReadByte (loadFile);
		// Read byte	gameData.segs.segment2s [segnum].matcen_num
		gameData.segs.segment2s [segnum].matcen_num = CFReadByte (loadFile);
		// Read short	gameData.segs.segment2s [segnum].value
		gameData.segs.segment2s [segnum].value = (char) CFReadShort (loadFile);
	} else {
		gameData.segs.segment2s [segnum].special = 0;
		gameData.segs.segment2s [segnum].matcen_num = -1;
		gameData.segs.segment2s [segnum].value = 0;
	}
}

//------------------------------------------------------------------------------

void RegisterLight (tFaceColor *pc, int i)
{
if (!pc || pc->index) {
	tLightInfo	*pli = gameData.render.shadows.lightInfo + gameData.render.shadows.nLights++;
	short			sideVerts [4];
#ifdef _DEBUG
	vms_angvec	a;
#endif
	pli->nIndex = i;
	GetSideVerts (sideVerts, i / 6, i % 6);
	VmVecAvg4 (&pli->pos, 
					 gameData.segs.vertices + sideVerts [0], 
					 gameData.segs.vertices + sideVerts [1], 
					 gameData.segs.vertices + sideVerts [2], 
					 gameData.segs.vertices + sideVerts [3]);
	OOF_VecVms2Gl (pli->glPos, &pli->pos);
	pli->nSegNum = i / 6;
	pli->nSideNum = i % 6;
#ifdef _DEBUG
	VmExtractAnglesVector (&a, gameData.segs.segments [i / 6].sides [i % 6].normals);
	VmAngles2Matrix (&pli->orient, &a);
#endif
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
					m = pDist [ (l + r) / 2].nDist;
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

int ComputeNearestSegmentLights (void)
{
	segment				*segP;
	tLightInfo			*pli;
	int					h, i, j;
	tOOF_vector			center, dist;
	vms_vector			*pv;
	struct tLightDist	*pDists;

if (!gameData.render.shadows.nLights)
	return 0;
if (! (pDists = d_malloc (gameData.render.shadows.nLights * sizeof (tLightDist)))) {
	gameData.render.shadows.nLights = 0;
	return 0;
	}
for (i = 0, segP = gameData.segs.segments; i < gameData.segs.nSegments; i++, segP++) {
	center.x =
	center.y =
	center.z = 0;
	for (j = 0; j < 8; j++) {
		pv = gameData.segs.vertices + segP->verts [j];
		center.x += (float) pv->x / 65336.0f;
		center.y += (float) pv->y / 65336.0f;
		center.z += (float) pv->z / 65336.0f;
		}	
	center.x /= 8.0f;
	center.y /= 8.0f;
	center.z /= 8.0f;
	pli = gameData.render.shadows.lightInfo;
	for (j = 0; j < gameData.render.shadows.nLights; j++, pli++) {
		OOF_VecSub (&dist, &center, (tOOF_vector *) &pli->glPos);
		pDists [j].nIndex = j;
		pDists [j].nDist = (int) (OOF_VecMag (&dist) * 65536.0f);
		}
	QSortLightDist (pDists, 0, gameData.render.shadows.nLights - 1);
	h = (gameData.render.shadows.nLights < 8) ? gameData.render.shadows.nLights : 8;
	for (j = 0; j < h; j++)
		gameData.render.shadows.nNearestLights [i][j] = pDists [j].nIndex;
	for (; j < 8; j++)
		gameData.render.shadows.nNearestLights [i][j] = -1;
	}
d_free (pDists);
return 1;
}

//------------------------------------------------------------------------------

void LoadSegmentsCompiled (short segnum, CFILE *loadFile)
{
	short			lastSeg, sidenum, i;
	segment		*segP;
	side			*sideP;
	short			temp_short;
	ushort		temp_ushort = 0;
	ubyte			bit_mask;

INIT_PROGRESS_LOOP (segnum, lastSeg, gameData.segs.nSegments);
for (segP = gameData.segs.segments + segnum;  segnum < lastSeg; segnum++, segP++) {

#ifdef EDITOR
	segP->segnum = segnum;
	segP->group = 0;
#endif

	if (gameStates.app.bD2XLevel) { 
		gameData.segs.xSegments [segnum].owner = CFReadByte (loadFile);
		gameData.segs.xSegments [segnum].group = CFReadByte (loadFile);
		}
	else {
		gameData.segs.xSegments [segnum].owner = -1;
		gameData.segs.xSegments [segnum].group = -1;
		}
	if (bNewFileFormat)
		bit_mask = CFReadByte (loadFile);
	else
		bit_mask = 0x7f; // read all six children and special stuff...

	if (gameData.segs.nLevelVersion == 5) { // d2 SHAREWARE level
		read_special (segnum, bit_mask, loadFile);
		read_verts (segnum, loadFile);
		read_children (segnum, bit_mask, loadFile);
		}
	else {
		read_children (segnum, bit_mask, loadFile);
		read_verts (segnum, loadFile);
		if (gameData.segs.nLevelVersion <= 1) { // descent 1 level
			read_special (segnum, bit_mask, loadFile);
			}
		}

	segP->objects = -1;

	if (gameData.segs.nLevelVersion <= 5) { // descent 1 thru d2 SHAREWARE level
		// Read fix	segP->static_light (shift down 5 bits, write as short)
		temp_ushort = CFReadShort (loadFile);
		gameData.segs.segment2s [segnum].static_light	= ((fix)temp_ushort) << 4;
		//CFRead ( &segP->static_light, sizeof (fix), 1, loadFile );
		}

	// Read the walls as a 6 byte array
	for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++ )	{
		segP->sides [sidenum].frame_num = 0;
		}

	if (bNewFileFormat)
		bit_mask = CFReadByte (loadFile);
	else
		bit_mask = 0x3f; // read all six sides
	for (sidenum = 0, sideP = segP->sides; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++, sideP++) {
		sideP->wall_num = NO_WALL;
		if (bit_mask & (1 << sidenum)) {
			ushort wall_num;
			if (gameData.segs.nLevelVersion >= 13)
				wall_num = (ushort) CFReadShort (loadFile);
			else
				wall_num = (ushort) ((ubyte) CFReadByte (loadFile));
			if (IS_WALL (wall_num))
				sideP->wall_num = wall_num;
			}
		}

	for (sidenum=0, sideP = segP->sides; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++, sideP++ )	{
#ifdef _DEBUG
		int bReadSideData;
		if (segP->children [sidenum]==-1)
			bReadSideData = 1;
		else if (IS_WALL (WallNumI (segnum, sidenum)))
			bReadSideData = 2;
		else
			bReadSideData = 0;
		if (bReadSideData) {
#else
		if ((segP->children [sidenum] == -1) || IS_WALL (WallNumI (segnum, sidenum))) {
#endif
			// Read short sideP->tmap_num;
			if (bNewFileFormat) {
				temp_ushort = CFReadShort (loadFile);
				sideP->tmap_num = temp_ushort & 0x7fff;
				} 
			else
				sideP->tmap_num = CFReadShort (loadFile);
			if (gameData.segs.nLevelVersion <= 1)
				sideP->tmap_num = convert_d1_tmap_num (sideP->tmap_num);
			if (bNewFileFormat && ! (temp_ushort & 0x8000))
				sideP->tmap_num2 = 0;
			else {
				// Read short sideP->tmap_num2;
				sideP->tmap_num2 = CFReadShort (loadFile);
				if ((gameData.segs.nLevelVersion <= 1) && sideP->tmap_num2)
					sideP->tmap_num2 = convert_d1_tmap_num (sideP->tmap_num2);
				}

			// Read uvl sideP->uvls [4] (u, v>>5, write as short, l>>1 write as short)
			for (i=0; i<4; i++ )	{
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
				//CFRead ( &sideP->uvls [i].l, sizeof (fix), 1, loadFile );
				}
			} 
		else {
			sideP->tmap_num =
			sideP->tmap_num2 = 0;
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

// get the default colors
memset (gameData.render.color.textures, 0, sizeof (gameData.render.color.textures));
for (i = 0; i < MAX_WALL_TEXTURES; i++) {
	if (GetColor (i, &lm)) {
		gameData.render.color.textures [i].index = 1;
		gameData.render.color.textures [i].color.red = lm.color [0];
		gameData.render.color.textures [i].color.green = lm.color [1];
		gameData.render.color.textures [i].color.blue = lm.color [2];
		}
	}
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
if (gameStates.app.bD2XLevel && gameStates.render.color.bLightMapsOk) { 
	INIT_PROGRESS_LOOP (i, j, gameData.segs.nSegments * 6);
	pc = &gameData.render.color.lights [0][0] + i;
	for (; i < j; i++, pc++) {
		ReadColor (pc, loadFile, gameData.segs.nLevelVersion <= 13);
#if SHADOWS
		RegisterLight (pc, i);
#endif
		}
	}
else {
#else
	{
#endif
#if SHADOWS
	segment	*segP;
	side		*sideP;
	int		h;

	INIT_PROGRESS_LOOP (i, j, gameData.segs.nSegments);
	segP = gameData.segs.segments + i;
	for (i = 0; i < j; i++, segP++)
		for (h = 0, sideP = segP->sides; h < 6; h++, sideP++)
			if (IsLight (sideP->tmap_num) || IsLight (sideP->tmap_num2 & 0x3fff))
				RegisterLight (NULL, i * 6 + h);
#endif
	}
}

//------------------------------------------------------------------------------

void ComputeSegSideCenters (int segnum)
{
	int		i, j, sidenum;
	segment	*segP;
	side		*sideP;

INIT_PROGRESS_LOOP (segnum, j, gameData.segs.nSegments);

for (i = segnum * 6, segP = gameData.segs.segments + segnum; segnum < j; segnum++, segP++) {
	ComputeSegmentCenter (gameData.segs.segCenters + segnum, segP);
	for (sidenum = 0, sideP = segP->sides; sidenum < 6; sidenum++, i++)
		ComputeSideCenter (gameData.segs.sideCenters + i, segP, sidenum);
	}
}

//------------------------------------------------------------------------------

static short loadIdx = 0;
static int loadOp = 0;
static CFILE *mineDataFile;

static void LoadSegmentsPoll (int nItems, newmenu_item *m, int *key, int cItem)
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
	ValidateSegmentAll ();			// Fill in side type and normals.
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
	if (loadIdx >= (bLightmaps ? gameData.segs.nSegments * 6 : bShadows ? gameData.segs.nSegments : 1)) {
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
return;
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
	if (bLightmaps)
		i += PROGRESS_STEPS (gameData.segs.nSegments * 6);
	else if (bShadows)
		i += PROGRESS_STEPS (gameData.segs.nSegments);
	else
		i++;
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

void LoadSegmentsGauge (CFILE *loadFile)
{
loadOp = 0;
loadIdx = 0;
mineDataFile = loadFile;
NMProgressBar (TXT_PREP_DESCENT, 0, LoadMineGaugeSize () + PagingGaugeSize (), LoadSegmentsPoll); 
}

//------------------------------------------------------------------------------

int LoadMineSegmentsCompiled (CFILE *loadFile)
{
	short			i;
	ubyte			nCompiledVersion;
	ushort		temp_ushort = 0;
	char			*psz;

d1_pig_present = CFExist (D1_PIGFILE, gameFolders.szDataDir, 0);

psz = strchr (gameData.segs.szLevelFilename, '.');
bNewFileFormat = !psz || strcmp (psz, ".sdl");
//	For compiled levels, textures map to themselves, prevent tmap_override always being gray, 
//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
#ifdef EDITOR
for (i = 0; i < MAX_TEXTURES; i++)
	tmap_xlate_table [i] = i;
#endif

//	memset ( gameData.segs.segments, 0, sizeof (segment)*MAX_SEGMENTS );
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
	gameData.segs.fVertices [i].p.x = ((float) gameData.segs.vertices [i].x) / 65536.0f;
	gameData.segs.fVertices [i].p.y = ((float) gameData.segs.vertices [i].y) / 65536.0f;
	gameData.segs.fVertices [i].p.z = ((float) gameData.segs.vertices [i].z) / 65536.0f;
#endif
	}
memset (gameData.segs.segments, 0, MAX_SEGMENTS * sizeof (segment));
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
ResetObjects (1);		//one object, the player
if (gameOpts->ogl.bUseLighting)
	AddOglLights ();
#if SHADOWS
ComputeNearestSegmentLights ();
#endif
return 0;
}

//------------------------------------------------------------------------------
//eof
