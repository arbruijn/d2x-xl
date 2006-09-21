/* $Id: gamesave.c,v 1.21 2003/06/16 07:15:59 btb Exp $ */
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
 * Save game information
 *
 * Old Log:
 * Revision 1.3  1996/02/21  13:59:17  allender
 * check Data folder when can't open a level file from a hog
 *
 * Revision 1.2  1995/10/31  10:23:23  allender
 * shareware stuff
 *
 * Revision 1.1  1995/05/16  15:25:37  allender
 * Initial revision
 *
 * Revision 2.2  1995/04/23  14:53:12  john
 * Made some mine structures read in with no structure packing problems.
 *
 * Revision 2.1  1995/03/20  18:15:43  john
 * Added code to not store the normals in the segment structure.
 *
 * Revision 2.0  1995/02/27  11:29:50  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.207  1995/02/23  10:17:36  allender
 * fixed parameter mismatch with COMPUTE_SEGMENT_CENTER
 *
 * Revision 1.206  1995/02/22  14:51:17  allender
 * fixed some things that I missed
 *
 * Revision 1.205  1995/02/22  13:31:38  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.204  1995/02/01  20:58:08  john
 * Made editor check hog.
 *
 * Revision 1.203  1995/01/28  17:40:34  mike
 * correct level names (use rdl, sdl) for dumpmine stuff.
 *
 * Revision 1.202  1995/01/25  20:03:46  matt
 * Moved matrix check to avoid orthogonalizing an uninitialize matrix
 *
 * Revision 1.201  1995/01/20  16:56:53  mike
 * remove some mprintfs.
 *
 * Revision 1.200  1995/01/15  19:42:13  matt
 * Ripped out hostage faces for registered version
 *
 * Revision 1.199  1995/01/05  16:59:09  yuan
 * Make it so if editor is loaded, don't get error from typo
 * in filename.
 *
 * Revision 1.198  1994/12/19  12:49:46  mike
 * Change fgets to CFGetS.  fgets was getting a pointer mismatch warning.
 *
 * Revision 1.197  1994/12/12  01:20:03  matt
 * Took out object size hack for green claw guys
 *
 * Revision 1.196  1994/12/11  13:19:37  matt
 * Restored calls to FixObjectSegs() when debugging is turned off, since
 * it's not a big routine, and could fix some possibly bad problems.
 *
 * Revision 1.195  1994/12/10  16:17:24  mike
 * fix editor bug that was converting transparent walls into rock.
 *
 * Revision 1.194  1994/12/09  14:59:27  matt
 * Added system to attach a fireball to another object for rendering purposes,
 * so the fireball always renders on top of (after) the object.
 *
 * Revision 1.193  1994/12/08  17:19:02  yuan
 * Cfiling stuff.
 *
 * Revision 1.192  1994/12/02  20:01:05  matt
 * Always give vulcan cannon powerup same amount of ammo, regardless of
 * how much it was saved with
 *
 * Revision 1.191  1994/11/30  17:45:57  yuan
 * Saving files now creates RDL/SDLs instead of CDLs.
 *
 * Revision 1.190  1994/11/30  17:22:14  matt
 * Ripped out hostage faces in shareware version
 *
 * Revision 1.189  1994/11/28  00:09:30  allender
 * commented out call to NDRecordStartDemo in LoadLevelSub...what is
 * this doing here anyway?????
 *
 * Revision 1.188  1994/11/27  23:13:48  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.187  1994/11/27  18:06:20  matt
 * Cleaned up LVL/CDL file loading
 *
 * Revision 1.186  1994/11/25  22:46:29  matt
 * Allow ESC out of compiled/normal menu (esc=compiled).
 *
 * Revision 1.185  1994/11/23  12:18:35  mike
 * move level names here...a more logical place than dumpmine.
 *
 * Revision 1.184  1994/11/21  20:29:19  matt
 * If hostage info is bad, fix it.
 *
 * Revision 1.183  1994/11/21  20:26:07  matt
 * Fixed bug, I hope
 *
 * Revision 1.182  1994/11/21  20:20:37  matt
 * Fixed stupid mistake
 *
 * Revision 1.181  1994/11/21  20:18:40  matt
 * Fixed (hopefully) totally bogus writing of hostage data
 *
 * Revision 1.180  1994/11/20  14:11:56  matt
 * Gracefully handle two hostages having same id
 *
 * Revision 1.179  1994/11/19  23:55:05  mike
 * remove Assert, put in comment for Matt.
 *
 * Revision 1.178  1994/11/19  19:53:24  matt
 * Added code to full support different hostage head clip & message for
 * each hostage.
 *
 * Revision 1.177  1994/11/19  15:15:21  mike
 * remove unused code and data
 *
 * Revision 1.176  1994/11/19  10:28:28  matt
 * Took out write routines when editor compiled out
 *
 * Revision 1.175  1994/11/17  20:38:25  john
 * Took out warning.
 *
 * Revision 1.174  1994/11/17  20:36:34  john
 * Made it so that saving a mine will write the .cdl even
 * if .lvl gets error.
 *
 * Revision 1.173  1994/11/17  20:26:19  john
 * Made the game load whichever of .cdl or .lvl exists,
 * and if they both exist, prompt the user for which one.
 *
 * Revision 1.172  1994/11/17  20:11:20  john
 * Fixed warning.
 *
 * Revision 1.171  1994/11/17  20:09:26  john
 * Added new compiled level format.
 *
 * Revision 1.170  1994/11/17  14:57:21  mike
 * moved segment validation functions from editor to main.
 *
 * Revision 1.169  1994/11/17  11:39:21  matt
 * Ripped out code to load old mines
 *
 * Revision 1.168  1994/11/16  11:24:53  matt
 * Made attack-type robots have smaller radius, so they get closer to player
 *
 * Revision 1.167  1994/11/15  21:42:47  mike
 * better error messages.
 *
 * Revision 1.166  1994/11/15  15:30:41  matt
 * Save ptr to name of level being loaded
 *
 * Revision 1.165  1994/11/14  20:47:46  john
 * Attempted to strip out all the code in the game
 * directory that uses any ui code.
 *
 * Revision 1.164  1994/11/14  14:34:23  matt
 * Fixed up handling when textures can't be found during remap
 *
 * Revision 1.163  1994/11/10  14:02:49  matt
 * Hacked in support for player ships with different textures
 *
 * Revision 1.162  1994/11/06  14:38:17  mike
 * Remove an apparently unnecessary con_printf.
 *
 * Revision 1.161  1994/10/30  14:11:28  mike
 * ripout local segments stuff.
 *
 * Revision 1.160  1994/10/28  12:10:41  matt
 * Check that was supposed to happen only when editor was in was happening
 * only when editor was out.
 *
 * Revision 1.159  1994/10/27  11:25:32  matt
 * Only do connectivity error check when editor in
 *
 * Revision 1.158  1994/10/27  10:54:00  matt
 * Made connectivity error checking put up warning if errors found
 *
 * Revision 1.157  1994/10/25  10:50:54  matt
 * Vulcan cannon powerups now contain ammo count
 *
 * Revision 1.156  1994/10/23  02:10:43  matt
 * Got rid of obsolete hostage_info stuff
 *
 * Revision 1.155  1994/10/22  18:57:26  matt
 * Added call to check_segment_connections()
 *
 * Revision 1.154  1994/10/21  12:19:23  matt
 * Clear transient gameData.objs.objects when saving (& loading) games
 *
 * Revision 1.153  1994/10/21  11:25:10  mike
 * Use new constant IMMORTAL_TIME.
 *
 * Revision 1.152  1994/10/20  12:46:59  matt
 * Replace old save files (MIN/SAV/HOT) with new LVL files
 *
 * Revision 1.151  1994/10/19  19:26:32  matt
 * Fixed stupid bug
 *
 * Revision 1.150  1994/10/19  16:46:21  matt
 * Made tmap overrides for robots remap texture numbers
 *
 * Revision 1.149  1994/10/18  08:50:27  yuan
 * Fixed correct variable this time.
 *
 * Revision 1.148  1994/10/18  08:45:02  yuan
 * Oops. forgot load function.
 *
 * Revision 1.147  1994/10/18  08:42:10  yuan
 * Avoid the int3.
 *
 * Revision 1.146  1994/10/17  21:34:57  matt
 * Added support for new Control Center/Main Reactor
 *
 * Revision 1.145  1994/10/15  19:06:34  mike
 * Fix bug, maybe, having to do with something or other, ...
 *
 * Revision 1.144  1994/10/12  21:07:33  matt
 * Killed unused field in object structure
 *
 * Revision 1.143  1994/10/06  14:52:55  mike
 * Put check in to detect possibly bogus walls in last segment which leaked through an earlier check
 * due to misuse of gameData.segs.nLastSegment.
 *
 * Revision 1.142  1994/10/05  22:12:44  mike
 * Put in cleanup for matcen/fuelcen links.
 *
 * Revision 1.141  1994/10/03  11:30:05  matt
 * Make sure player in a valid segment before saving
 *
 * Revision 1.140  1994/09/28  11:14:41  mike
 * Better error messaging on bogus mines: Only bring up dialog box if a "real" (level??.*) level.
 *
 * Revision 1.139  1994/09/28  09:22:58  mike
 * Comment out a con_printf.
 *
 * Revision 1.138  1994/09/27  17:08:36  mike
 * Message boxes when you load bogus mines.
 *
 * Revision 1.137  1994/09/27  15:43:45  mike
 * Move the dump stuff to dumpmine.
 *
 * Revision 1.136  1994/09/27  00:02:31  mike
 * Dump text files (".txm") when loading a mine, showing all kinds of useful mine info.
 *
 * Revision 1.135  1994/09/26  11:30:41  matt
 * Took out code which loaded bogus player structure
 *
 * Revision 1.134  1994/09/26  11:18:44  john
 * Fixed some conflicts with newseg.
 *
 * Revision 1.133  1994/09/26  10:56:58  matt
 * Fixed inconsistancies in lifeleft for immortal gameData.objs.objects
 *
 * Revision 1.132  1994/09/25  23:41:10  matt
 * Changed the object load & save code to read/write the structure fields one
 * at a time (rather than the whole structure at once).  This mean that the
 * object structure can be changed without breaking the load/save functions.
 * As a result of this change, the local_object data can be and has been
 * incorporated into the object array.  Also, timeleft is now a property
 * of all gameData.objs.objects, and the object structure has been otherwise cleaned up.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
char gamesave_rcsid[] = "$Id: gamesave.c,v 1.21 2003/06/16 07:15:59 btb Exp $";
#endif

#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "strutil.h"
#include "mono.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "newmenu.h"

#include "inferno.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif
#include "error.h"
#include "object.h"
#include "game.h"
#include "screens.h"
#include "wall.h"
#include "gamemine.h"
#include "robot.h"
#include "fireball.h"

#include "cfile.h"
#include "bm.h"
#include "menu.h"
#include "switch.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "weapon.h"
#include "newdemo.h"
#include "gameseq.h"
#include "automap.h"
#include "polyobj.h"
#include "text.h"
#include "gamefont.h"
#include "gamesave.h"
#include "gamepal.h"
#include "laser.h"
#include "byteswap.h"
#include "multi.h"
#include "makesig.h"
#include "gameseg.h"

#define GAME_VERSION            32
#define GAME_COMPATIBLE_VERSION 22

//version 28->29  add delta light support
//version 27->28  controlcen id now is reactor number, not model number
//version 28->29  ??
//version 29->30  changed trigger structure
//version 30->31  changed trigger structure some more
//version 31->32  change segment structure, make it 512 bytes w/o editor, add gameData.segs.segment2s array.

#define MENU_CURSOR_X_MIN       MENU_X
#define MENU_CURSOR_X_MAX       MENU_X+6

game_fileinfo	gameFileInfo;
game_top_fileinfo	gameTopFileInfo;

//  LINT: adding function prototypes
void read_object(object *objP, CFILE *f, int version);
#ifdef EDITOR
void write_object(object *objP, FILE *f);
void do_load_save_levels(int save);
#endif
#ifndef NDEBUG
void dump_mine_info(void);
#endif

#ifdef EDITOR
extern char mine_filename[];
extern int save_mine_data_compiled(FILE * SaveFile);
//--unused-- #else
//--unused-- char mine_filename[128];
#endif

int Gamesave_num_org_robots = 0;
//--unused-- grs_bitmap * Gamesave_saved_bitmap = NULL;

//------------------------------------------------------------------------------
#ifdef EDITOR
// Return true if this level has a name of the form "level??"
// Note that a pathspec can appear at the beginning of the filename.
int is_real_level(char *filename)
{
	int len = (int) strlen(filename);

	if (len < 6)
		return 0;

	return !strnicmp(&filename[len-11], "level", 5);

}
#endif

//------------------------------------------------------------------------------

void ChangeFilenameExtension(char *dest, char *src, char *new_ext)
{
	int i;

	strcpy (dest, src);

	if (new_ext[0]=='.')
		new_ext++;

	for (i=1; i < (int) strlen(dest); i++)
		if (dest[i]=='.'||dest[i]==' '||dest[i]==0)
			break;

	if (i < 123) {
		dest[i]='.';
		dest[i+1]=new_ext[0];
		dest[i+2]=new_ext[1];
		dest[i+3]=new_ext[2];
		dest[i+4]=0;
		return;
	}
}

//------------------------------------------------------------------------------
//--unused-- vms_angvec zero_angles={0,0,0};

#define vm_angvec_zero(v) do {(v)->p=(v)->b=(v)->h=0;} while (0)

int Gamesave_num_players=0;

int N_save_pof_names;
char Save_pof_names[MAX_POLYGON_MODELS][SHORT_FILENAME_LEN];

void check_and_fix_matrix(vms_matrix *m);

void VerifyObject(object * objP)	{

objP->lifeleft = IMMORTAL_TIME;		//all loaded object are immortal, for now
if (objP->type == OBJ_ROBOT) {
	Gamesave_num_org_robots++;
	// Make sure valid id...
	if (objP->id >= gameData.bots.nTypes [gameStates.app.bD1Data])
		objP->id %= gameData.bots.nTypes [0];
	// Make sure model number & size are correct...
	if (objP->render_type == RT_POLYOBJ) {
		Assert(gameData.bots.pInfo[objP->id].model_num != -1);
			//if you fail this assert, it means that a robot in this level
			//hasn't been loaded, possibly because he's marked as
			//non-shareware.  To see what robot number, print objP->id.
		Assert(gameData.bots.pInfo[objP->id].always_0xabcd == 0xabcd);
			//if you fail this assert, it means that the robot_ai for
			//a robot in this level hasn't been loaded, possibly because
			//it's marked as non-shareware.  To see what robot number,
			//print objP->id.
		objP->rtype.pobj_info.model_num = gameData.bots.pInfo[objP->id].model_num;
		objP->size = gameData.models.polyModels[objP->rtype.pobj_info.model_num].rad;
		}
	if (objP->id == 65)						//special "reactor" robots
		objP->movement_type = MT_NONE;
	if (objP->movement_type == MT_PHYSICS) {
		objP->mtype.phys_info.mass = gameData.bots.pInfo[objP->id].mass;
		objP->mtype.phys_info.drag = gameData.bots.pInfo[objP->id].drag;
		}
	}
else {		//Robots taken care of above
	if (objP->render_type == RT_POLYOBJ) {
		int i;
		char *name = Save_pof_names[objP->rtype.pobj_info.model_num];
		for (i=0;i<gameData.models.nPolyModels;i++)
			if (!stricmp(Pof_names[i],name)) {		//found it!	
				objP->rtype.pobj_info.model_num = i;
				break;
				}
		}
	}
if (objP->type == OBJ_POWERUP) {
	if (objP->id >= gameData.objs.pwrUp.nTypes) {
		objP->id = 0;
		Assert(objP->render_type != RT_POLYOBJ);
		}
	objP->control_type = CT_POWERUP;
	objP->size = gameData.objs.pwrUp.info[objP->id].size;
	objP->ctype.powerup_info.creation_time = 0;
#ifdef NETWORK
	if (gameData.app.nGameMode & GM_NETWORK) {
	  if (MultiPowerupIs4Pack(objP->id)) {
			gameData.multi.powerupsInMine[objP->id-1]+=4;
	 		gameData.multi.maxPowerupsAllowed[objP->id-1]+=4;
			}
		gameData.multi.powerupsInMine[objP->id]++;
		gameData.multi.maxPowerupsAllowed[objP->id]++;
#if TRACE
		con_printf (CON_DEBUG,"PowerupLimiter: ID=%d\n",objP->id);
		if (objP->id>MAX_POWERUP_TYPES)
			con_printf (1,"POWERUP: Overwriting array bounds!\n");
#endif
		}
#endif
	}
if (objP->type == OBJ_WEAPON)	{
	if (objP->id >= gameData.weapons.nTypes [0])	{
		objP->id = 0;
		Assert(objP->render_type != RT_POLYOBJ);
		}
	if (objP->id == PMINE_ID) {		//make sure pmines have correct values
		objP->mtype.phys_info.mass = gameData.weapons.info[objP->id].mass;
		objP->mtype.phys_info.drag = gameData.weapons.info[objP->id].drag;
		objP->mtype.phys_info.flags |= PF_FREE_SPINNING;
		// Make sure model number & size are correct...		
		Assert(objP->render_type == RT_POLYOBJ);
		objP->rtype.pobj_info.model_num = gameData.weapons.info[objP->id].model_num;
		objP->size = gameData.models.polyModels[objP->rtype.pobj_info.model_num].rad;
		}
	}
if (objP->type == OBJ_CNTRLCEN) {
	objP->render_type = RT_POLYOBJ;
	objP->control_type = CT_CNTRLCEN;
	if (gameData.segs.nLevelVersion <= 1) { // descent 1 reactor
		objP->id = 0;                         // used to be only one kind of reactor
		objP->rtype.pobj_info.model_num = gameData.reactor.reactors[0].model_num;// descent 1 reactor
		}
#ifdef EDITOR
	{
	int i;
	// Check, and set, strength of reactor
	for (i=0; i<gameData.objs.types.nCount; i++)	
		if (gameData.objs.types.nType[i]==OL_CONTROL_CENTER && gameData.objs.types.nType.nId[i] == objP->id) {
			objP->shields = gameData.objs.types.nType.nStrength[i];
			break;		
			}
		Assert(i < gameData.objs.types.nCount);		//make sure we found it
		}
#endif
	}
if (objP->type == OBJ_PLAYER)	{
	if (objP == gameData.objs.console)		
		InitPlayerObject();
	else
		if (objP->render_type == RT_POLYOBJ)	//recover from Matt's pof file matchup bug
			objP->rtype.pobj_info.model_num = gameData.pig.ship.player->model_num;
	//Make sure orient matrix is orthogonal
	gameOpts->render.nMathFormat = 0;
	check_and_fix_matrix(&objP->orient);
	gameOpts->render.nMathFormat = gameOpts->render.nDefMathFormat;
	objP->id = Gamesave_num_players++;
	}
if (objP->type == OBJ_HOSTAGE) {
	objP->render_type = RT_HOSTAGE;
	objP->control_type = CT_POWERUP;
	}
}

//------------------------------------------------------------------------------
//static gs_skip(int len,CFILE *file)
//{
//
//	CFSeek(file,len,SEEK_CUR);
//}

#ifdef EDITOR
static void gs_write_int(int i,FILE *file)
{
	if (fwrite(&i, sizeof(i), 1, file) != 1)
		Error("Error reading int in gamesave.c");

}

static void gs_write_fix(fix f,FILE *file)
{
	if (fwrite(&f, sizeof(f), 1, file) != 1)
		Error("Error reading fix in gamesave.c");

}

static void gs_write_short(short s,FILE *file)
{
	if (fwrite(&s, sizeof(s), 1, file) != 1)
		Error("Error reading short in gamesave.c");

}

static void gs_write_fixang(fixang f,FILE *file)
{
	if (fwrite(&f, sizeof(f), 1, file) != 1)
		Error("Error reading fixang in gamesave.c");

}

static void gs_write_byte(byte b,FILE *file)
{
	if (fwrite(&b, sizeof(b), 1, file) != 1)
		Error("Error reading byte in gamesave.c");

}

static void gr_write_vector(vms_vector *v,FILE *file)
{
	gs_write_fix(v->x,file);
	gs_write_fix(v->y,file);
	gs_write_fix(v->z,file);
}

static void gs_write_matrix(vms_matrix *m,FILE *file)
{
	gr_write_vector(&m->rvec,file);
	gr_write_vector(&m->uvec,file);
	gr_write_vector(&m->fvec,file);
}

static void gs_write_angvec(vms_angvec *v,FILE *file)
{
	gs_write_fixang(v->p,file);
	gs_write_fixang(v->b,file);
	gs_write_fixang(v->h,file);
}

#endif

//------------------------------------------------------------------------------

extern int MultiPowerupIs4Pack(int);
//reads one object of the given version from the given file
void read_object(object *objP,CFILE *f,int version)
{

	objP->type           = CFReadByte(f);
	objP->id             = CFReadByte(f);

	objP->control_type   = CFReadByte(f);
	objP->movement_type  = CFReadByte(f);
	objP->render_type    = CFReadByte(f);
	objP->flags          = CFReadByte(f);

	objP->segnum         = CFReadShort(f);
	objP->attached_obj   = -1;

	CFReadVector(&objP->pos,f);
	CFReadMatrix(&objP->orient,f);

	objP->size           = CFReadFix(f);
	objP->shields        = CFReadFix(f);

	CFReadVector(&objP->last_pos,f);

	objP->contains_type  = CFReadByte(f);
	objP->contains_id    = CFReadByte(f);
	objP->contains_count = CFReadByte(f);

	switch (objP->movement_type) {

		case MT_PHYSICS:

			CFReadVector(&objP->mtype.phys_info.velocity,f);
			CFReadVector(&objP->mtype.phys_info.thrust,f);

			objP->mtype.phys_info.mass		= CFReadFix(f);
			objP->mtype.phys_info.drag		= CFReadFix(f);
			objP->mtype.phys_info.brakes	= CFReadFix(f);

			CFReadVector(&objP->mtype.phys_info.rotvel,f);
			CFReadVector(&objP->mtype.phys_info.rotthrust,f);

			objP->mtype.phys_info.turnroll	= CFReadFixAng(f);
			objP->mtype.phys_info.flags		= CFReadShort(f);

			break;

		case MT_SPINNING:

			CFReadVector(&objP->mtype.spin_rate,f);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (objP->control_type) {

		case CT_AI: {
			int i;

			objP->ctype.ai_info.behavior = CFReadByte(f);

			for (i=0;i<MAX_AI_FLAGS;i++)
				objP->ctype.ai_info.flags[i] = CFReadByte(f);

			objP->ctype.ai_info.hide_segment = CFReadShort(f);
			objP->ctype.ai_info.hide_index = CFReadShort(f);
			objP->ctype.ai_info.path_length = CFReadShort(f);
			objP->ctype.ai_info.cur_path_index = (char) CFReadShort(f);

			if (version <= 25) {
				CFReadShort(f);	//				objP->ctype.ai_info.follow_path_start_seg	= 
				CFReadShort(f);	//				objP->ctype.ai_info.follow_path_end_seg		= 
			}

			break;
		}

		case CT_EXPLOSION:

			objP->ctype.expl_info.spawn_time		= CFReadFix(f);
			objP->ctype.expl_info.delete_time		= CFReadFix(f);
			objP->ctype.expl_info.delete_objnum	= CFReadShort(f);
			objP->ctype.expl_info.next_attach = objP->ctype.expl_info.prev_attach = objP->ctype.expl_info.attach_parent = -1;

			break;

		case CT_WEAPON:

			//do I really need to read these?  Are they even saved to disk?

			objP->ctype.laser_info.parent_type		= CFReadShort(f);
			objP->ctype.laser_info.parent_num		= CFReadShort(f);
			objP->ctype.laser_info.parent_signature	= CFReadInt(f);

			break;

		case CT_LIGHT:

			objP->ctype.light_info.intensity = CFReadFix(f);
			break;

		case CT_POWERUP:

			if (version >= 25)
				objP->ctype.powerup_info.count = CFReadInt(f);
			else
				objP->ctype.powerup_info.count = 1;

			if (objP->id == POW_VULCAN_WEAPON)
					objP->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

			if (objP->id == POW_GAUSS_WEAPON)
					objP->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

			if (objP->id == POW_OMEGA_WEAPON)
					objP->ctype.powerup_info.count = MAX_OMEGA_CHARGE;

			break;


		case CT_NONE:
		case CT_FLYING:
		case CT_DEBRIS:
			break;

		case CT_SLEW:		//the player is generally saved as slew
			break;

		case CT_CNTRLCEN:
			break;

		case CT_MORPH:
		case CT_FLYTHROUGH:
		case CT_REPAIRCEN:
		default:
			Int3();
	
	}

	switch (objP->render_type) {

		case RT_NONE:
			break;

		case RT_MORPH:
		case RT_POLYOBJ: {
			int i,tmo;

			objP->rtype.pobj_info.model_num		= CFReadInt(f);

			for (i=0;i<MAX_SUBMODELS;i++)
				CFReadAngVec(&objP->rtype.pobj_info.anim_angles[i],f);

			objP->rtype.pobj_info.subobj_flags	= CFReadInt(f);

			tmo = CFReadInt(f);

			#ifndef EDITOR
			objP->rtype.pobj_info.tmap_override	= tmo;
			#else
			if (tmo==-1)
				objP->rtype.pobj_info.tmap_override	= -1;
			else {
				int xlated_tmo = tmap_xlate_table[tmo];
				if (xlated_tmo < 0)	{
#if TRACE
					con_printf (CON_DEBUG, "Couldn't find texture for demo object, model_num = %d\n", objP->rtype.pobj_info.model_num);
#endif
					Int3();
					xlated_tmo = 0;
				}
				objP->rtype.pobj_info.tmap_override	= xlated_tmo;
			}
			#endif

			objP->rtype.pobj_info.alt_textures	= 0;

			break;
		}

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:

			objP->rtype.vclip_info.nClipIndex	= CFReadInt(f);
			objP->rtype.vclip_info.xFrameTime	= CFReadFix(f);
			objP->rtype.vclip_info.nCurFrame	= CFReadByte(f);

			break;

		case RT_THRUSTER:
		case RT_LASER:
			break;

		default:
			Int3();

	}

}

//------------------------------------------------------------------------------
#ifdef EDITOR

//writes one object to the given file
void write_object(object *objP,FILE *f)
{
	gs_write_byte(objP->type,f);
	gs_write_byte(objP->id,f);

	gs_write_byte(objP->control_type,f);
	gs_write_byte(objP->movement_type,f);
	gs_write_byte(objP->render_type,f);
	gs_write_byte(objP->flags,f);

	gs_write_short(objP->segnum,f);

	gr_write_vector(&objP->pos,f);
	gs_write_matrix(&objP->orient,f);

	gs_write_fix(objP->size,f);
	gs_write_fix(objP->shields,f);

	gr_write_vector(&objP->last_pos,f);

	gs_write_byte(objP->contains_type,f);
	gs_write_byte(objP->contains_id,f);
	gs_write_byte(objP->contains_count,f);

	switch (objP->movement_type) {

		case MT_PHYSICS:

	 		gr_write_vector(&objP->mtype.phys_info.velocity,f);
			gr_write_vector(&objP->mtype.phys_info.thrust,f);

			gs_write_fix(objP->mtype.phys_info.mass,f);
			gs_write_fix(objP->mtype.phys_info.drag,f);
			gs_write_fix(objP->mtype.phys_info.brakes,f);

			gr_write_vector(&objP->mtype.phys_info.rotvel,f);
			gr_write_vector(&objP->mtype.phys_info.rotthrust,f);

			gs_write_fixang(objP->mtype.phys_info.turnroll,f);
			gs_write_short(objP->mtype.phys_info.flags,f);

			break;

		case MT_SPINNING:

			gr_write_vector(&objP->mtype.spin_rate,f);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (objP->control_type) {

		case CT_AI: {
			int i;

			gs_write_byte(objP->ctype.ai_info.behavior,f);

			for (i=0;i<MAX_AI_FLAGS;i++)
				gs_write_byte(objP->ctype.ai_info.flags[i],f);

			gs_write_short(objP->ctype.ai_info.hide_segment,f);
			gs_write_short(objP->ctype.ai_info.hide_index,f);
			gs_write_short(objP->ctype.ai_info.path_length,f);
			gs_write_short(objP->ctype.ai_info.cur_path_index,f);

			// -- unused! mk, 08/13/95 -- gs_write_short(objP->ctype.ai_info.follow_path_start_seg,f);
			// -- unused! mk, 08/13/95 -- gs_write_short(objP->ctype.ai_info.follow_path_end_seg,f);

			break;
		}

		case CT_EXPLOSION:

			gs_write_fix(objP->ctype.expl_info.spawn_time,f);
			gs_write_fix(objP->ctype.expl_info.delete_time,f);
			gs_write_short(objP->ctype.expl_info.delete_objnum,f);

			break;

		case CT_WEAPON:

			//do I really need to write these gameData.objs.objects?

			gs_write_short(objP->ctype.laser_info.parent_type,f);
			gs_write_short(objP->ctype.laser_info.parent_num,f);
			gs_write_int(objP->ctype.laser_info.parent_signature,f);

			break;

		case CT_LIGHT:

			gs_write_fix(objP->ctype.light_info.intensity,f);
			break;

		case CT_POWERUP:

			gs_write_int(objP->ctype.powerup_info.count,f);
			break;

		case CT_NONE:
		case CT_FLYING:
		case CT_DEBRIS:
			break;

		case CT_SLEW:		//the player is generally saved as slew
			break;

		case CT_CNTRLCEN:
			break;			//control center object.

		case CT_MORPH:
		case CT_REPAIRCEN:
		case CT_FLYTHROUGH:
		default:
			Int3();
	
	}

	switch (objP->render_type) {

		case RT_NONE:
			break;

		case RT_MORPH:
		case RT_POLYOBJ: {
			int i;

			gs_write_int(objP->rtype.pobj_info.model_num,f);

			for (i=0;i<MAX_SUBMODELS;i++)
				gs_write_angvec(&objP->rtype.pobj_info.anim_angles[i],f);

			gs_write_int(objP->rtype.pobj_info.subobj_flags,f);

			gs_write_int(objP->rtype.pobj_info.tmap_override,f);

			break;
		}

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:

			gs_write_int(objP->rtype.vclip_info.nClipIndex,f);
			gs_write_fix(objP->rtype.vclip_info.xFrameTime,f);
			gs_write_byte(objP->rtype.vclip_info.nCurFrame,f);

			break;

		case RT_THRUSTER:
		case RT_LASER:
			break;

		default:
			Int3();

	}

}
#endif

extern int remove_trigger_num(int trigger_num);

// -----------------------------------------------------------------------------

#define	DLIndex(_i)	(gameData.render.lights.deltaIndices + (_i))

void SortDLIndexD2X (int left, int right)
{
	int	l = left,
			r = right,
			m = (left + right) / 2;
	short	mSeg = DLIndex (m)->d2x.segnum, 
			mSide = DLIndex (m)->d2x.sidenum;
	dl_index	*pl, *pr;

do {
	pl = DLIndex (l);
	while ((pl->d2x.segnum < mSeg) || ((pl->d2x.segnum == mSeg) && (pl->d2x.sidenum < mSide))) {
		pl++;
		l++;
		}
	pr = DLIndex (r);
	while ((pr->d2x.segnum > mSeg) || ((pr->d2x.segnum == mSeg) && (pr->d2x.sidenum > mSide))) {
		pr--;
		r--;
		}
	if (l <= r) {
		if (l < r) {
			dl_index	h = *pl;
			*pl = *pr;
			*pr = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (right > l)
   SortDLIndexD2X (l, right);
if (r > left)
   SortDLIndexD2X (left, r);
}

// -----------------------------------------------------------------------------

void SortDLIndexD2 (int left, int right)
{
	int	l = left,
			r = right,
			m = (left + right) / 2;
	short	mSeg = DLIndex (m)->d2.segnum, 
			mSide = DLIndex (m)->d2.sidenum;
	dl_index	*pl, *pr;

do {
	pl = DLIndex (l);
	while ((pl->d2.segnum < mSeg) || ((pl->d2.segnum == mSeg) && (pl->d2.sidenum < mSide))) {
		pl++;
		l++;
		}
	pr = DLIndex (r);
	while ((pr->d2.segnum > mSeg) || ((pr->d2.segnum == mSeg) && (pr->d2.sidenum > mSide))) {
		pr--;
		r--;
		}
	if (l <= r) {
		if (l < r) {
			dl_index	h = *pl;
			*pl = *pr;
			*pr = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (right > l)
   SortDLIndexD2 (l, right);
if (r > left)
   SortDLIndexD2 (left, r);
}

// -----------------------------------------------------------------------------

void SortDLIndex (void)
{
if (gameStates.render.bD2XLights)
	SortDLIndexD2X (0, gameFileInfo.lightDeltaIndices.count - 1);
else
	SortDLIndexD2 (0, gameFileInfo.lightDeltaIndices.count - 1);
}

// -----------------------------------------------------------------------------
// Load game 
// Loads all the relevant data for a level.
// If level != -1, it loads the filename with extension changed to .min
// Otherwise it loads the appropriate level mine.
// returns 0=everything ok, 1=old version, -1=error
int LoadMineDataCompiled(CFILE *LoadFile, int bFileInfo)
{
	int i,j;
	int start_offset;

	start_offset = CFTell(LoadFile);

	//===================== READ FILE INFO ========================

#if TRACE
	con_printf(CON_DEBUG, "   \nloading game data ...\n");
#endif
	// Set default values
	gameFileInfo.level					=	-1;
	gameFileInfo.player.offset		=	-1;
	gameFileInfo.player.size		=	sizeof(player);
 	gameFileInfo.object.offset		=	-1;
	gameFileInfo.object.count		=	0;
	gameFileInfo.object.size		=	sizeof(object);  
	gameFileInfo.walls.offset			=	-1;
	gameFileInfo.walls.count		=	0;
	gameFileInfo.walls.size			=	sizeof(wall);  
	gameFileInfo.doors.offset			=	-1;
	gameFileInfo.doors.count		=	0;
	gameFileInfo.doors.size			=	sizeof(active_door);  
	gameFileInfo.triggers.offset		=	-1;
	gameFileInfo.triggers.count	=	0;
	gameFileInfo.triggers.size		=	sizeof(trigger);  
	gameFileInfo.control.offset		=	-1;
	gameFileInfo.control.count		=	0;
	gameFileInfo.control.size		=	sizeof(reactor_triggers);
	gameFileInfo.matcen.offset		=	-1;
	gameFileInfo.matcen.count		=	0;
	gameFileInfo.matcen.size		=	sizeof(matcen_info);

	gameFileInfo.lightDeltaIndices.offset		=	-1;
	gameFileInfo.lightDeltaIndices.count		=	0;
	gameFileInfo.lightDeltaIndices.size		=	sizeof(dl_index);

	gameFileInfo.lightDeltas.offset		=	-1;
	gameFileInfo.lightDeltas.count		=	0;
	gameFileInfo.lightDeltas.size		=	sizeof(delta_light);

	// Read in gameTopFileInfo to get size of saved fileinfo.

	if (CFSeek(LoadFile, start_offset, SEEK_SET)) 
		Error("Error seeking in gamesave.c"); 

//	if (CFRead(&gameTopFileInfo, sizeof(gameTopFileInfo), 1, LoadFile) != 1)
//		Error("Error reading gameTopFileInfo in gamesave.c");

	gameTopFileInfo.fileinfo_signature = CFReadShort(LoadFile);
	gameTopFileInfo.fileinfo_version = CFReadShort(LoadFile);
	gameTopFileInfo.fileinfo_sizeof = CFReadInt(LoadFile);

	gameStates.render.bD2XLights = gameStates.app.bD2XLevel && (gameTopFileInfo.fileinfo_version >= 34);
	// Check signature
	if (gameTopFileInfo.fileinfo_signature != 0x6705)
		return -1;

	// Check version number
	if (gameTopFileInfo.fileinfo_version < GAME_COMPATIBLE_VERSION)
		return -1;

	// Now, Read in the fileinfo
	if (CFSeek(LoadFile, start_offset, SEEK_SET)) 
		Error("Error seeking to gameFileInfo in gamesave.c");

//	if (CFRead(&gameFileInfo, gameTopFileInfo.fileinfo_sizeof, 1, LoadFile)!=1)
//		Error("Error reading gameFileInfo in gamesave.c");

	gameFileInfo.fileinfo_signature = CFReadShort(LoadFile);
	gameFileInfo.fileinfo_version = CFReadShort(LoadFile);
	gameFileInfo.fileinfo_sizeof = CFReadInt(LoadFile);
	for(i=0; i<15; i++)
		gameFileInfo.mine_filename[i] = CFReadByte(LoadFile);
	gameFileInfo.level = CFReadInt(LoadFile);
	gameFileInfo.player.offset = CFReadInt(LoadFile);				// Player info
	gameFileInfo.player.size = CFReadInt(LoadFile);
	gameFileInfo.object.offset = CFReadInt(LoadFile);				// Object info
	gameFileInfo.object.count = CFReadInt(LoadFile);    	
	gameFileInfo.object.size = CFReadInt(LoadFile);  
	gameFileInfo.walls.offset = CFReadInt(LoadFile);
	gameFileInfo.walls.count = CFReadInt(LoadFile);
	gameFileInfo.walls.size = CFReadInt(LoadFile);
	gameFileInfo.doors.offset = CFReadInt(LoadFile);
	gameFileInfo.doors.count = CFReadInt(LoadFile);
	gameFileInfo.doors.size = CFReadInt(LoadFile);
	gameFileInfo.triggers.offset = CFReadInt(LoadFile);
	gameFileInfo.triggers.count = CFReadInt(LoadFile);
	gameFileInfo.triggers.size = CFReadInt(LoadFile);
	gameFileInfo.links.offset = CFReadInt(LoadFile);
	gameFileInfo.links.count = CFReadInt(LoadFile);
	gameFileInfo.links.size = CFReadInt(LoadFile);
	gameFileInfo.control.offset = CFReadInt(LoadFile);
	gameFileInfo.control.count = CFReadInt(LoadFile);
	gameFileInfo.control.size = CFReadInt(LoadFile);
	gameFileInfo.matcen.offset = CFReadInt(LoadFile);
	gameFileInfo.matcen.count = CFReadInt(LoadFile);
	gameFileInfo.matcen.size = CFReadInt(LoadFile);

	if (gameTopFileInfo.fileinfo_version >= 29) {
		gameFileInfo.lightDeltaIndices.offset = CFReadInt(LoadFile);
		gameFileInfo.lightDeltaIndices.count = CFReadInt(LoadFile);
		gameFileInfo.lightDeltaIndices.size = CFReadInt(LoadFile);

		gameFileInfo.lightDeltas.offset = CFReadInt(LoadFile);
		gameFileInfo.lightDeltas.count = CFReadInt(LoadFile);
		gameFileInfo.lightDeltas.size = CFReadInt(LoadFile);
	}
	if (gameTopFileInfo.fileinfo_version >= 31) { //load mine filename
		// read newline-terminated string, not sure what version this changed.
		CFGetS(gameData.missions.szCurrentLevel,sizeof(gameData.missions.szCurrentLevel),LoadFile);

		if (gameData.missions.szCurrentLevel[strlen(gameData.missions.szCurrentLevel)-1] == '\n')
			gameData.missions.szCurrentLevel[strlen(gameData.missions.szCurrentLevel)-1] = 0;
	}
	else if (gameTopFileInfo.fileinfo_version >= 14) { //load mine filename
		// read null-terminated string
		char *p=gameData.missions.szCurrentLevel;
		//must do read one char at a time, since no CFGetS()
		do {
			*p = CFGetC (LoadFile);
			} while (*p++);
	}
	else
		gameData.missions.szCurrentLevel[0]=0;

	if (gameTopFileInfo.fileinfo_version >= 19) {	//load pof names
		N_save_pof_names = CFReadShort(LoadFile);
		if (N_save_pof_names != 0x614d && N_save_pof_names != 0x5547) { // "Ma"de w/DMB beta/"GU"ILE
			Assert(N_save_pof_names < MAX_POLYGON_MODELS);
			CFRead(Save_pof_names,N_save_pof_names,SHORT_FILENAME_LEN,LoadFile);
		}
	}

	if (bFileInfo)
		return 0;

	//===================== READ PLAYER INFO ==========================
	gameData.objs.nNextSignature = 0;

	//===================== READ OBJECT INFO ==========================

	Gamesave_num_org_robots = 0;
	Gamesave_num_players = 0;
i = CFTell (LoadFile);
	if (gameFileInfo.object.offset > -1) {
		object	*objP = gameData.objs.objects;
#if TRACE
		con_printf(CON_DEBUG, "   loading object data ...\n");
#endif
		if (CFSeek(LoadFile, gameFileInfo.object.offset, SEEK_SET))
			Error("Error seeking to object.offset in gamesave.c");
		for (i = 0; i < gameFileInfo.object.count; i++, objP++) {
			read_object (objP, LoadFile, gameTopFileInfo.fileinfo_version);
//			if ((objP->type == OBJ_POWERUP) && (objP->id == POW_KEY_RED))
//				objP = objP;
			objP->signature = gameData.objs.nNextSignature++;
			VerifyObject(objP);
			gameData.objs.init [i] = *objP;
		}
	}
	for (i = 0; i < MAX_OBJECTS - 1; i++)
		gameData.objs.dropInfo [i].nNextPowerup = i + 1;
	gameData.objs.dropInfo [i].nNextPowerup = -1;
	gameData.objs.nFirstDropped =
	gameData.objs.nLastDropped = -1;
	gameData.objs.nFreeDropped = 0;
i = CFTell (LoadFile);

	//===================== READ WALL INFO ============================

	if (gameFileInfo.walls.offset > -1) {
#if TRACE
		con_printf(CON_DEBUG, "   loading wall data ...\n");
#endif
		if (!CFSeek(LoadFile, gameFileInfo.walls.offset,SEEK_SET))	{
			for (i=0;i<gameFileInfo.walls.count;i++) {

				if (gameTopFileInfo.fileinfo_version >= 20)
					wall_read(&gameData.walls.walls[i], LoadFile); // v20 walls and up.
				else if (gameTopFileInfo.fileinfo_version >= 17) {
					v19_wall w;

					v19_wall_read(&w, LoadFile);
					gameData.walls.walls[i].segnum	   = w.segnum;
					gameData.walls.walls[i].sidenum		= w.sidenum;
					gameData.walls.walls[i].linked_wall	= w.linked_wall;
					gameData.walls.walls[i].type			= w.type;
					gameData.walls.walls[i].flags			= w.flags;
					gameData.walls.walls[i].hps			= w.hps;
					gameData.walls.walls[i].trigger		= w.trigger;
					gameData.walls.walls[i].clip_num		= w.clip_num;
					gameData.walls.walls[i].keys			= w.keys;
					gameData.walls.walls[i].state			= WALL_DOOR_CLOSED;
				} else {
					v16_wall w;

					v16_wall_read(&w, LoadFile);
					gameData.walls.walls[i].segnum = gameData.walls.walls[i].sidenum = gameData.walls.walls[i].linked_wall = -1;
					gameData.walls.walls[i].type		= w.type;
					gameData.walls.walls[i].flags		= w.flags;
					gameData.walls.walls[i].hps		= w.hps;
					gameData.walls.walls[i].trigger	= w.trigger;
					gameData.walls.walls[i].clip_num	= w.clip_num;
					gameData.walls.walls[i].keys		= w.keys;
				}

			}
		}
	}

	//===================== READ DOOR INFO ============================

	if (gameFileInfo.doors.offset > -1) {
#if TRACE
		con_printf(CON_DEBUG, "   loading door data ...\n");
#endif
		if (!CFSeek(LoadFile, gameFileInfo.doors.offset,SEEK_SET))	{
			for (i=0;i<gameFileInfo.doors.count;i++) {
				if (gameTopFileInfo.fileinfo_version >= 20)
					active_door_read(&gameData.walls.activeDoors[i], LoadFile); // version 20 and up
				else {
					v19_door d;
					int p;

					v19_door_read(&d, LoadFile);
					gameData.walls.activeDoors[i].n_parts = d.n_parts;
					for (p=0;p<d.n_parts;p++) {
						short cseg,cside;

						cseg = gameData.segs.segments[d.seg[p]].children[d.side[p]];
						cside = FindConnectedSide(gameData.segs.segments + d.seg[p], gameData.segs.segments + cseg);
						gameData.walls.activeDoors[i].front_wallnum[p] = WallNumI (d.seg[p], d.side[p]);
						gameData.walls.activeDoors[i].back_wallnum[p] = WallNumI (cseg, cside);
					}
				}

			}
		}
	}

	//==================== READ TRIGGER INFO ==========================


// for MACINTOSH -- assume all triggers >= verion 31 triggers.

	if (gameFileInfo.triggers.offset > -1) {
#if TRACE
		con_printf(CON_DEBUG, "   loading trigger data ...\n");
#endif
		if (!CFSeek(LoadFile, gameFileInfo.triggers.offset,SEEK_SET))	{
			for (i=0;i<gameFileInfo.triggers.count;i++) {
				if (gameTopFileInfo.fileinfo_version >= 31) 
					TriggerRead(gameData.trigs.triggers + i, LoadFile, 0);
				else {
					v30_trigger trig;
					int t,type,flags;

					type=flags=0;

					if (gameTopFileInfo.fileinfo_version < 30) {
						v29_trigger trig29;
						int t;

						v29_trigger_read(&trig29, LoadFile);

						trig.flags		= trig29.flags;
						trig.num_links	= (char) trig29.num_links;
						trig.num_links	= (char) trig29.num_links;
						trig.value		= trig29.value;
						trig.time		= trig29.time;

						for (t=0;t<trig.num_links;t++) {
							trig.seg[t]  = trig29.seg[t];
							trig.side[t] = trig29.side[t];
						}
					}
					else
						v30_trigger_read(&trig, LoadFile);

					//Assert(trig.flags & TRIGGER_ON);
					trig.flags &= ~TRIGGER_ON;

					if (trig.flags & TRIGGER_CONTROL_DOORS)
						type = TT_OPEN_DOOR;
					else if (trig.flags & TRIGGER_SHIELD_DAMAGE)
						type = TT_SHIELD_DAMAGE;
					else if (trig.flags & TRIGGER_ENERGY_DRAIN)
						type = TT_ENERGY_DRAIN;
					else if (trig.flags & TRIGGER_EXIT)
						type = TT_EXIT;
					else if (trig.flags & TRIGGER_MATCEN)
						type = TT_MATCEN;
					else if (trig.flags & TRIGGER_ILLUSION_OFF)
						type = TT_ILLUSION_OFF;
					else if (trig.flags & TRIGGER_SECRET_EXIT)
						type = TT_SECRET_EXIT;
					else if (trig.flags & TRIGGER_ILLUSION_ON)
						type = TT_ILLUSION_ON;
					else if (trig.flags & TRIGGER_UNLOCK_DOORS)
						type = TT_UNLOCK_DOOR;
					else if (trig.flags & TRIGGER_OPEN_WALL)
						type = TT_OPEN_WALL;
					else if (trig.flags & TRIGGER_CLOSE_WALL)
						type = TT_CLOSE_WALL;
					else if (trig.flags & TRIGGER_ILLUSORY_WALL)
						type = TT_ILLUSORY_WALL;
					else
						Int3();
					if (trig.flags & TRIGGER_ONE_SHOT)
						flags = TF_ONE_SHOT;

					gameData.trigs.triggers[i].type        = type;
					gameData.trigs.triggers[i].flags       = flags;
					gameData.trigs.triggers[i].num_links   = trig.num_links;
					gameData.trigs.triggers[i].num_links   = trig.num_links;
					gameData.trigs.triggers[i].value       = trig.value;
					gameData.trigs.triggers[i].time        = trig.time;

					for (t=0;t<trig.num_links;t++) {
						gameData.trigs.triggers[i].seg[t] = trig.seg[t];
						gameData.trigs.triggers[i].side[t] = trig.side[t];
					}
				}
			}
		i = CFTell (LoadFile);
		if (gameTopFileInfo.fileinfo_version >= 33) {
			gameData.trigs.nObjTriggers = CFReadInt (LoadFile);
			if (gameData.trigs.nObjTriggers) {
				for (i = 0; i < gameData.trigs.nObjTriggers; i++)
					TriggerRead (gameData.trigs.objTriggers + i, LoadFile, 1);
				for (i = 0; i < gameData.trigs.nObjTriggers; i++) {
					gameData.trigs.objTriggerRefs [i].prev = CFReadShort (LoadFile);
					gameData.trigs.objTriggerRefs [i].next = CFReadShort (LoadFile);
					gameData.trigs.objTriggerRefs [i].objnum = CFReadShort (LoadFile);
					}
				}
			i = CFTell (LoadFile);
			for (i = 0; i < MAX_OBJECTS_D2X; i++)
				gameData.trigs.firstObjTrigger [i] = CFReadShort (LoadFile);
			}
		else {
			gameData.trigs.nObjTriggers = 0;
			memset (gameData.trigs.objTriggers, 0, sizeof (trigger) * MAX_OBJ_TRIGGERS);
			memset (gameData.trigs.objTriggerRefs, 0xff, sizeof (obj_trigger_ref) * MAX_OBJ_TRIGGERS);
			memset (gameData.trigs.firstObjTrigger, 0xff, sizeof (short) * MAX_OBJECTS_D2X);
			}
		}
	}

	//================ READ CONTROL CENTER TRIGGER INFO ===============

	if (gameFileInfo.control.offset > -1) {
#if TRACE
		con_printf(CON_DEBUG, "   loading reactor data ...\n");
#endif
		if (!CFSeek(LoadFile, gameFileInfo.control.offset, SEEK_SET))
		{
			Assert(gameFileInfo.control.size == sizeof(reactor_triggers));
			ControlCenterTriggersReadN(&gameData.reactor.triggers, gameFileInfo.control.count, LoadFile);
		}
	}	

	//================ READ MATERIALIZATION CENTERS INFO ===============
	if (gameFileInfo.matcen.offset > -1) {
		int	j;

#if TRACE
		con_printf(CON_DEBUG, "   loading matcen data ...\n");
#endif
		if (!CFSeek(LoadFile, gameFileInfo.matcen.offset,SEEK_SET))	{
			for (i=0;i<gameFileInfo.matcen.count;i++) {
				if (gameTopFileInfo.fileinfo_version < 27) {
					old_matcen_info m;

					OldMatCenInfoRead(&m, LoadFile);

					gameData.matCens.robotCenters[i].robot_flags[0] = m.robot_flags;
					gameData.matCens.robotCenters[i].robot_flags[1] = 0;
					gameData.matCens.robotCenters[i].hit_points = m.hit_points;
					gameData.matCens.robotCenters[i].interval = m.interval;
					gameData.matCens.robotCenters[i].segnum = m.segnum;
					gameData.matCens.robotCenters[i].fuelcen_num = m.fuelcen_num;
				}
				else
					MatCenInfoRead(&gameData.matCens.robotCenters[i], LoadFile);

				//	Set links in gameData.matCens.robotCenters to gameData.matCens.fuelCenters array

				for (j=0; j<=gameData.segs.nLastSegment; j++)
					if (gameData.segs.segment2s[j].special == SEGMENT_IS_ROBOTMAKER)
						if (gameData.segs.segment2s[j].matcen_num == i)
							gameData.matCens.robotCenters[i].fuelcen_num = gameData.segs.segment2s[j].value;
			}
		}
	}

	//================ READ DL_INDICES INFO ===============

	gameData.render.lights.nStatic = 0;

	if (gameFileInfo.lightDeltaIndices.offset > -1) {
		int	i;

#if TRACE
		con_printf(CON_DEBUG, "   loading light index data ...\n");
#endif
		if (!CFSeek(LoadFile, gameFileInfo.lightDeltaIndices.offset, SEEK_SET))	{
			gameData.render.lights.nStatic = gameFileInfo.lightDeltaIndices.count;
			for (i = 0; i < gameFileInfo.lightDeltaIndices.count; i++) {
				if (gameTopFileInfo.fileinfo_version < 29) {
#if TRACE
					con_printf (CON_DEBUG, "Warning: Old mine version.  Not reading gameData.render.lights.deltaIndices info.\n");
#endif
					Int3();	//shouldn't be here!!!
					}
				else
					dl_index_read(gameData.render.lights.deltaIndices + i, LoadFile);
				}
			}
		SortDLIndex ();
		}

	//	Indicate that no light has been subtracted from any vertices.
	clear_light_subtracted();

	//================ READ DELTA LIGHT INFO ===============

	if (gameFileInfo.lightDeltas.offset > -1) {
		int	i;

#if TRACE
		con_printf(CON_DEBUG, "   loading light data ...\n");
#endif
		if (!CFSeek(LoadFile, gameFileInfo.lightDeltas.offset, SEEK_SET))	{
			for (i=0; i<gameFileInfo.lightDeltas.count; i++) {
				if (gameTopFileInfo.fileinfo_version < 29) {
#if TRACE
					con_printf (CON_DEBUG, "Warning: Old mine version.  Not reading delta light info.\n");
#endif
				} else
					delta_light_read(&gameData.render.lights.deltas[i], LoadFile);
			}
		}
	}

	//========================= UPDATE VARIABLES ======================

	ResetObjects(gameFileInfo.object.count);

	for (i=0; i<gameFileInfo.object.count/*MAX_OBJECTS*/; i++) {
		gameData.objs.objects[i].next = gameData.objs.objects[i].prev = -1;
		if (gameData.objs.objects[i].type != OBJ_NONE) {
			int objsegnum = gameData.objs.objects[i].segnum;
			if ((objsegnum < 0) || (objsegnum > gameData.segs.nLastSegment))		//bogus object
				gameData.objs.objects[i].type = OBJ_NONE;
			else {
				gameData.objs.objects[i].segnum = -1;			//avoid Assert()
				LinkObject(i,objsegnum);
			}
		}
	}

	clear_transient_objects(1);		//1 means clear proximity bombs

	// Make sure non-transparent doors are set correctly.
	for (i = 0; i < gameData.segs.nSegments; i++) {
		side	*sidep = gameData.segs.segments [i].sides;
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++, sidep++) {
			short wall_num = WallNumS (sidep);
			wall  *w;
			if (!IS_WALL (wall_num))
				continue;
			w = gameData.walls.walls + wall_num;
			if (w->clip_num == -1)
				continue;
			if (gameData.walls.pAnims [w->clip_num].flags & WCF_TMAP1) {
				sidep->tmap_num = gameData.walls.pAnims [w->clip_num].frames [0];
				sidep->tmap_num2 = 0;
				}
			}
		}

	gameData.walls.nWalls = gameFileInfo.walls.count;
	ResetWalls();

	gameData.walls.nOpenDoors = gameFileInfo.doors.count;
	gameData.trigs.nTriggers = gameFileInfo.triggers.count;

	//go through all walls, killing references to invalid triggers
	for (i=0;i<gameData.walls.nWalls;i++)
		if (gameData.walls.walls[i].trigger >= gameData.trigs.nTriggers) {
#if TRACE
			con_printf (CON_DEBUG,"Removing reference to invalid trigger %d from wall %d\n",gameData.walls.walls[i].trigger,i);
#endif
			gameData.walls.walls[i].trigger = NO_TRIGGER;	//kill trigger
		}

	//go through all triggers, killing unused ones
	for (i=0;i<gameData.trigs.nTriggers;) {
		int w;

		//	Find which wall this trigger is connected to.
		for (w=0; w<gameData.walls.nWalls; w++)
			if (gameData.walls.walls[w].trigger == i)
				break;

	#ifdef EDITOR
		if (w == gameData.walls.nWalls) {
#if TRACE
			con_printf (CON_DEBUG,"Removing unreferenced trigger %d\n",i);
#endif
			remove_trigger_num(i);
		}
		else
	#endif
			i++;
	}

	//	MK, 10/17/95: Make walls point back at the triggers that control them.
	//	Go through all triggers, stuffing controlling_trigger field in gameData.walls.walls.
	{	int t;

	for (i=0; i<gameData.walls.nWalls; i++)
		gameData.walls.walls[i].controlling_trigger = -1;

	for (t=0; t<gameData.trigs.nTriggers; t++) {
		int	l;
		for (l=0; l<gameData.trigs.triggers[t].num_links; l++) {
			short	seg_num, side_num, wall_num;

			seg_num = gameData.trigs.triggers[t].seg[l];
			side_num = gameData.trigs.triggers[t].side[l];
			wall_num = WallNumI (seg_num, side_num);

			// -- if (gameData.walls.walls[wall_num].controlling_trigger != -1)
			// -- 	Int3();

			//check to see that if a trigger requires a wall that it has one,
			//and if it requires a matcen that it has one

			if (gameData.trigs.triggers[t].type == TT_MATCEN) {
				if (gameData.segs.segment2s[seg_num].special != SEGMENT_IS_ROBOTMAKER)
					Int3();		//matcen trigger doesn't point to matcen
			}
			else if (gameData.trigs.triggers[t].type != TT_LIGHT_OFF && gameData.trigs.triggers[t].type != TT_LIGHT_ON) {	//light triggers don't require walls
				if (!IS_WALL (wall_num))
					Int3();	//	This is illegal.  This trigger requires a wall
				else
					gameData.walls.walls[wall_num].controlling_trigger = t;
			}
		}
	}
	}

	gameData.matCens.nRobotCenters = gameFileInfo.matcen.count;
	//fix old wall structs
	if (gameTopFileInfo.fileinfo_version < 17) {
		short segnum,sidenum,wallnum;

		for (segnum=0; segnum<=gameData.segs.nLastSegment; segnum++)
			for (sidenum=0;sidenum<6;sidenum++)
				if (IS_WALL (wallnum= WallNumI (segnum, sidenum))) {
					gameData.walls.walls[wallnum].segnum = segnum;
					gameData.walls.walls[wallnum].sidenum = sidenum;
				}
	}

	#ifndef NDEBUG
	{
		short	sidenum;
		for (sidenum=0; sidenum<6; sidenum++) {
			short	wallnum = WallNumI (gameData.segs.nLastSegment, sidenum);
			if (IS_WALL (wallnum))
				if ((gameData.walls.walls[wallnum].segnum != gameData.segs.nLastSegment) || 
					 (gameData.walls.walls[wallnum].sidenum != sidenum))
					Int3();	//	Error.  Bogus walls in this segment.
								// Consult Yuan or Mike.
		}
	}
	#endif

	//create_local_segment_data();

	FixObjectSegs();

	#ifndef NDEBUG
	dump_mine_info();
	#endif

	if ((gameTopFileInfo.fileinfo_version < GAME_VERSION) && 
		 ((gameTopFileInfo.fileinfo_version != 25) || (GAME_VERSION != 26)))
		return 1;		//means old version
	else
		return 0;
}

// ----------------------------------------------------------------------------

int check_segment_connections(void);

extern void	set_ambient_sound_flags(void);


#define LEVEL_FILE_VERSION      8
//1 -> 2  add palette name
//2 -> 3  add control center explosion time
//3 -> 4  add reactor strength
//4 -> 5  killed hostage text stuff
//5 -> 6  added gameData.segs.secret.nReturnSegment and gameData.segs.secret.returnOrient
//6 -> 7  added flickering lights
//7 -> 8  made version 8 to be not compatible with D2 1.0 & 1.1

#ifndef RELEASE
char *Level_being_loaded=NULL;
#endif

#ifdef COMPACT_SEGS
extern void ncache_flush();
#endif

int no_old_level_file_error=0;

//loads a level (.LVL) file from disk
//returns 0 if success, else error code
int LoadLevelSub(char * filename_passed)
{
#ifdef EDITOR
	int use_compiled_level=1;
#endif
	CFILE * LoadFile;
	char filename[128];
	int sig, minedata_offset, gamedata_offset;
	int mine_err, game_err;
#ifdef NETWORK
	//int i;
#endif
	gameData.segs.bHaveSlideSegs = 0;

#ifdef NETWORK
   if (gameData.app.nGameMode & GM_NETWORK) {
		memset (gameData.multi.maxPowerupsAllowed, 0, sizeof (gameData.multi.maxPowerupsAllowed));
		memset (gameData.multi.powerupsInMine, 0, sizeof (gameData.multi.powerupsInMine));
		}
#endif

#ifdef COMPACT_SEGS
ncache_flush();
#endif

#ifndef RELEASE
Level_being_loaded = filename_passed;
#endif

strcpy(filename,filename_passed);

#ifdef EDITOR
	//if we have the editor, try the LVL first, no matter what was passed.
	//if we don't have an LVL, try RDL  
	//if we don't have the editor, we just use what was passed

	ChangeFilenameExtension(filename,filename_passed,".lvl");
	use_compiled_level = 0;

	if (!CFExist(filename))	{
		ChangeFilenameExtension(filename,filename,".rl2");
		use_compiled_level = 1;
	}		
#endif

LoadFile = CFOpen (filename, "", "rb", gameStates.app.bD1Mission);
if (!LoadFile)	{
	#ifdef EDITOR
#if TRACE
		con_printf (CON_DEBUG,"Can't open level file <%s>\n", filename);
#endif
		return 1;
	#else
//			Warning ("Can't open file <%s>\n",filename);
		return 1;
	#endif
}

strcpy(gameData.segs.szLevelFilename, filename);

//	#ifdef NEWDEMO
//	if (gameData.demo.nState == ND_STATE_RECORDING)
//		NDRecordStartDemo();
//	#endif

sig = CFReadInt(LoadFile);
gameData.segs.nLevelVersion = CFReadInt(LoadFile);
gameStates.app.bD2XLevel = (gameData.segs.nLevelVersion >= 10);
#if TRACE
con_printf (CON_DEBUG, "gameData.segs.nLevelVersion = %d\n", gameData.segs.nLevelVersion);
#endif
minedata_offset = CFReadInt(LoadFile);
gamedata_offset = CFReadInt(LoadFile);

Assert(sig == MAKE_SIG('P','L','V','L'));
if (gameData.segs.nLevelVersion >= 8) {    //read dummy data
	CFReadInt(LoadFile);
	CFReadShort(LoadFile);
	CFReadByte(LoadFile);
}

if (gameData.segs.nLevelVersion < 5)
	CFReadInt(LoadFile);       //was hostagetext_offset

if (gameData.segs.nLevelVersion > 1) {
	CFGetS(szCurrentLevelPalette,sizeof(szCurrentLevelPalette),LoadFile);
	if (szCurrentLevelPalette[strlen(szCurrentLevelPalette)-1] == '\n')
		szCurrentLevelPalette[strlen(szCurrentLevelPalette)-1] = 0;
}
if (gameData.segs.nLevelVersion <= 1 || szCurrentLevelPalette[0]==0) // descent 1 level
	strcpy(szCurrentLevelPalette, DEFAULT_LEVEL_PALETTE); //D1_PALETTE

if (gameData.segs.nLevelVersion >= 3)
	gameStates.app.nBaseCtrlCenExplTime = CFReadInt(LoadFile);
else
	gameStates.app.nBaseCtrlCenExplTime = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;

if (gameData.segs.nLevelVersion >= 4)
	gameData.reactor.nStrength = CFReadInt(LoadFile);
else
	gameData.reactor.nStrength = -1;  //use old defaults

if (gameData.segs.nLevelVersion >= 7) {
	int i;

#if TRACE
con_printf(CON_DEBUG, "   loading dynamic lights ...\n");
#endif
Num_flickering_lights = CFReadInt(LoadFile);
Assert((Num_flickering_lights >= 0) && (Num_flickering_lights < MAX_FLICKERING_LIGHTS));
for (i = 0; i < Num_flickering_lights; i++)
	flickering_light_read(&Flickering_lights[i], LoadFile);
}
else
	Num_flickering_lights = 0;

if (gameData.segs.nLevelVersion < 6) {
	gameData.segs.secret.nReturnSegment = 0;
	gameData.segs.secret.returnOrient.rvec.x = F1_0;
	gameData.segs.secret.returnOrient.rvec.y = 0;
	gameData.segs.secret.returnOrient.rvec.z = 0;
	gameData.segs.secret.returnOrient.fvec.x = 0;
	gameData.segs.secret.returnOrient.fvec.y = F1_0;
	gameData.segs.secret.returnOrient.fvec.z = 0;
	gameData.segs.secret.returnOrient.uvec.x = 0;
	gameData.segs.secret.returnOrient.uvec.y = 0;
	gameData.segs.secret.returnOrient.uvec.z = F1_0;
} else {
	gameData.segs.secret.nReturnSegment = CFReadInt(LoadFile);
	gameData.segs.secret.returnOrient.rvec.x = CFReadInt(LoadFile);
	gameData.segs.secret.returnOrient.rvec.y = CFReadInt(LoadFile);
	gameData.segs.secret.returnOrient.rvec.z = CFReadInt(LoadFile);
	gameData.segs.secret.returnOrient.fvec.x = CFReadInt(LoadFile);
	gameData.segs.secret.returnOrient.fvec.y = CFReadInt(LoadFile);
	gameData.segs.secret.returnOrient.fvec.z = CFReadInt(LoadFile);
	gameData.segs.secret.returnOrient.uvec.x = CFReadInt(LoadFile);
	gameData.segs.secret.returnOrient.uvec.y = CFReadInt(LoadFile);
	gameData.segs.secret.returnOrient.uvec.z = CFReadInt(LoadFile);
}

SetDataVersion (-1);
#ifdef EDITOR
if (!use_compiled_level) {
	CFSeek(LoadFile,minedata_offset,SEEK_SET);
	mine_err = load_mine_data(LoadFile);
#if 0 // get from d1src if needed
	// Compress all uv coordinates in mine, improves texmap precision. --MK, 02/19/96
	compress_uv_coordinates_all();
#endif
	}
else
#endif
	{
	//NOTE LINK TO ABOVE!!
	CFSeek(LoadFile,gamedata_offset,SEEK_SET);
	game_err = LoadMineDataCompiled(LoadFile, 1);
	CFSeek(LoadFile,minedata_offset,SEEK_SET);
	mine_err = LoadMineSegmentsCompiled(LoadFile);
	}
if (mine_err == -1) {   //error!!
	CFClose(LoadFile);
	return 2;
}
CFSeek(LoadFile,gamedata_offset,SEEK_SET);
game_err = LoadMineDataCompiled(LoadFile, 0);
if (game_err == -1) {   //error!!
	CFClose(LoadFile);
	return 3;
}

//======================== CLOSE FILE =============================

CFClose(LoadFile);

set_ambient_sound_flags();

#ifdef EDITOR
write_game_text_file(filename);
if (Errors_in_mine) {
	if (is_real_level(filename)) {
		char  ErrorMessage[200];

		sprintf(ErrorMessage, TXT_MINE_ERRORS, Errors_in_mine, Level_being_loaded);
		StopTime();
		GrPaletteStepLoad (NULL);
		ExecMessageBox(NULL, 1, TXT_CONTINUE, ErrorMessage);
		StartTime();
	} else {
#if TRACE
		con_printf (1, TXT_MINE_ERRORS, Errors_in_mine, Level_being_loaded);
#endif
	}
}
#endif

#ifdef EDITOR
//If an old version, ask the use if he wants to save as new version
if (!no_old_level_file_error && (gameStates.app.nFunctionMode == FMODE_EDITOR) && 
    (((LEVEL_FILE_VERSION > 3) && gameData.segs.nLevelVersion < LEVEL_FILE_VERSION) || mine_err == 1 || game_err == 1)) {
	char  ErrorMessage[200];

	sprintf(ErrorMessage,
				"You just loaded a old version\n"
				"level.  Would you like to save\n"
				"it as a current version level?");

	StopTime();
	GrPaletteStepLoad (NULL);
	if (ExecMessageBox(NULL, 2, "Don't Save", "Save", ErrorMessage)==1)
		save_level(filename);
	StartTime();
}
#endif

#ifdef EDITOR
if (gameStates.app.nFunctionMode == FMODE_EDITOR)
	editor_status("Loaded NEW mine %s, \"%s\"",filename,gameData.missions.szCurrentLevel);
#endif

#ifdef EDITOR
if (check_segment_connections())
	ExecMessageBox("ERROR", 1, "Ok", 
			"Connectivity errors detected in\n"
			"mine.  See monochrome screen for\n"
			"details, and contact Matt or Mike.");
#endif

return 0;
}

#ifdef EDITOR
void get_level_name()
{
//NO_UI!!!	UI_WINDOW 				*NameWindow = NULL;
//NO_UI!!!	UI_GADGET_INPUTBOX	*NameText;
//NO_UI!!!	UI_GADGET_BUTTON 		*QuitButton;
//NO_UI!!!
//NO_UI!!!	// Open a window with a quit button
//NO_UI!!!	NameWindow = ui_open_window(20, 20, 300, 110, WIN_DIALOG);
//NO_UI!!!	QuitButton = ui_add_gadget_button(NameWindow, 150-24, 60, 48, 40, "Done", NULL);
//NO_UI!!!
//NO_UI!!!	ui_wprintf_at(NameWindow, 10, 12,"Please enter a name for this mine:");
//NO_UI!!!	NameText = ui_add_gadget_inputbox(NameWindow, 10, 30, LEVEL_NAME_LEN, LEVEL_NAME_LEN, gameData.missions.szCurrentLevel);
//NO_UI!!!
//NO_UI!!!	NameWindow->keyboard_focus_gadget = (UI_GADGET *)NameText;
//NO_UI!!!	QuitButton->hotkey = KEY_ENTER;
//NO_UI!!!
//NO_UI!!!	ui_gadget_calc_keys(NameWindow);
//NO_UI!!!
//NO_UI!!!	while (!QuitButton->pressed && last_keypress!=KEY_ENTER) {
//NO_UI!!!		ui_mega_process();
//NO_UI!!!		ui_window_do_gadgets(NameWindow);
//NO_UI!!!	}
//NO_UI!!!
//NO_UI!!!	strcpy(gameData.missions.szCurrentLevel, NameText->text);
//NO_UI!!!
//NO_UI!!!	if (NameWindow!=NULL)	{
//NO_UI!!!		ui_close_window(NameWindow);
//NO_UI!!!		NameWindow = NULL;
//NO_UI!!!	}
//NO_UI!!!

	newmenu_item m[2];

	memset (m, 0, sizeof (m));
	m[0].type = NM_TYPE_TEXT; 
	m[0].text = "Please enter a name for this mine:";
	m[1].type = NM_TYPE_INPUT; 
	m[1].text = gameData.missions.szCurrentLevel; 
	m[1].text_len = LEVEL_NAME_LEN;

	ExecMenu(NULL, "Enter mine name", 2, m, NULL);

}
#endif


#ifdef EDITOR

int	Errors_in_mine;

// -----------------------------------------------------------------------------
int compute_num_delta_light_records(void)
{
	int	i;
	int	total = 0;

	for (i=0; i<gameData.render.lights.nStatic; i++) {
		total += gameData.render.lights.deltaIndices[i].count;
	}

	return total;

}

// -----------------------------------------------------------------------------
// Save game
int save_game_data(FILE * SaveFile)
{
	int  player.offset, object.offset, walls.offset, doors.offset, triggers.offset, control.offset, matcen.offset; //, links.offset;
	int	gameData.render.lights.deltaIndices.offset, deltaLight.offset;
	int start_offset,end_offset;

	start_offset = ftell(SaveFile);

	//===================== SAVE FILE INFO ========================

	gameFileInfo.fileinfo_signature =	0x6705;
	gameFileInfo.fileinfo_version	=	GAME_VERSION;
	gameFileInfo.level					=  gameData.missions.nCurrentLevel;
	gameFileInfo.fileinfo_sizeof		=	sizeof(gameFileInfo);
	gameFileInfo.player.offset		=	-1;
	gameFileInfo.player.size		=	sizeof(player);
	gameFileInfo.object.offset		=	-1;
	gameFileInfo.object.count		=	gameData.objs.nLastObject+1;
	gameFileInfo.object.size		=	sizeof(object);
	gameFileInfo.walls.offset			=	-1;
	gameFileInfo.walls.count		=	gameData.walls.nWalls;
	gameFileInfo.walls.size			=	sizeof(wall);
	gameFileInfo.doors.offset			=	-1;
	gameFileInfo.doors.count		=	gameData.walls.nOpenDoors;
	gameFileInfo.doors.size			=	sizeof(active_door);
	gameFileInfo.triggers.offset		=	-1;
	gameFileInfo.triggers.count	=	gameData.trigs.nTriggers;
	gameFileInfo.triggers.size		=	sizeof(trigger);
	gameFileInfo.control.offset		=	-1;
	gameFileInfo.control.count		=  1;
	gameFileInfo.control.size		=  sizeof(reactor_triggers);
 	gameFileInfo.matcen.offset		=	-1;
	gameFileInfo.matcen.count		=	gameData.matCens.nRobotCenters;
	gameFileInfo.matcen.size		=	sizeof(matcen_info);

 	gameFileInfo.lightDeltaIndices.offset		=	-1;
	gameFileInfo.lightDeltaIndices.count		=	gameData.render.lights.nStatic;
	gameFileInfo.lightDeltaIndices.size		=	sizeof(dl_index);

 	gameFileInfo.lightDeltas.offset		=	-1;
	gameFileInfo.lightDeltas.count	=	compute_num_delta_light_records();
	gameFileInfo.lightDeltas.size		=	sizeof(delta_light);

	// Write the fileinfo
	fwrite(&gameFileInfo, sizeof(gameFileInfo), 1, SaveFile);

	// Write the mine name
	fprintf(SaveFile,"%s\n",gameData.missions.szCurrentLevel);

	fwrite(&gameData.models.nPolyModels,2,1,SaveFile);
	fwrite(Pof_names,gameData.models.nPolyModels,sizeof(*Pof_names),SaveFile);

	//==================== SAVE PLAYER INFO ===========================

	player.offset = ftell(SaveFile);
	fwrite(&gameData.multi.players[gameData.multi.nLocalPlayer], sizeof(player), 1, SaveFile);

	//==================== SAVE OBJECT INFO ===========================

	object.offset = ftell(SaveFile);
	//fwrite(&gameData.objs.objects, sizeof(object), gameFileInfo.object.count, SaveFile);
	{
		int i;
		for (i=0;i<gameFileInfo.object.count;i++)
			write_object(&gameData.objs.objects[i],SaveFile);
	}

	//==================== SAVE WALL INFO =============================

	walls.offset = ftell(SaveFile);
	fwrite(gameData.walls.walls, sizeof(wall), gameFileInfo.walls.count, SaveFile);

	//==================== SAVE DOOR INFO =============================

	doors.offset = ftell(SaveFile);
	fwrite(gameData.walls.activeDoors, sizeof(active_door), gameFileInfo.doors.count, SaveFile);

	//==================== SAVE TRIGGER INFO =============================

	triggers.offset = ftell(SaveFile);
	fwrite(gameData.trigs.triggers, sizeof(trigger), gameFileInfo.triggers.count, SaveFile);

	//================ SAVE CONTROL CENTER TRIGGER INFO ===============

	control.offset = ftell(SaveFile);
	fwrite(&gameData.reactor.triggers, sizeof(reactor_triggers), 1, SaveFile);


	//================ SAVE MATERIALIZATION CENTER TRIGGER INFO ===============

	matcen.offset = ftell(SaveFile);
	fwrite(gameData.matCens.robotCenters, sizeof(matcen_info), gameFileInfo.matcen.count, SaveFile);

	//================ SAVE DELTA LIGHT INFO ===============
	gameData.render.lights.deltaIndices.offset = ftell(SaveFile);
	fwrite(gameData.render.lights.deltaIndices, sizeof(dl_index), gameFileInfo.lightDeltaIndices.count, SaveFile);

	deltaLight.offset = ftell(SaveFile);
	fwrite(gameData.render.lights.deltas, sizeof(delta_light), gameFileInfo.lightDeltas.count, SaveFile);

	//============= REWRITE FILE INFO, TO SAVE OFFSETS ===============

	// Update the offset fields
	gameFileInfo.player.offset		=	player.offset;
	gameFileInfo.object.offset		=	object.offset;
	gameFileInfo.walls.offset			=	walls.offset;
	gameFileInfo.doors.offset			=	doors.offset;
	gameFileInfo.triggers.offset		=	triggers.offset;
	gameFileInfo.control.offset		=	control.offset;
	gameFileInfo.matcen.offset		=	matcen.offset;
	gameFileInfo.lightDeltaIndices.offset	=	gameData.render.lights.deltaIndices.offset;
	gameFileInfo.lightDeltas.offset	=	deltaLight.offset;


	end_offset = ftell(SaveFile);

	// Write the fileinfo
	fseek( SaveFile, start_offset, SEEK_SET);  // Move to TOF
	fwrite(&gameFileInfo, sizeof(gameFileInfo), 1, SaveFile);

	// Go back to end of data
	fseek(SaveFile,end_offset,SEEK_SET);

	return 0;
}

int save_mine_data(FILE * SaveFile);

// -----------------------------------------------------------------------------
// Save game
int save_level_sub(char * filename, int compiled_version)
{
	FILE * SaveFile;
	char temp_filename[128];
	int sig = MAKE_SIG('P','L','V','L'),version=LEVEL_FILE_VERSION;
	int minedata_offset=0,gamedata_offset=0;

	if (!compiled_version)	{
		write_game_text_file(filename);

		if (Errors_in_mine) {
			if (is_real_level(filename)) {
				char  ErrorMessage[200];
	
				sprintf(ErrorMessage, TXT_MINE_ERRORS2, Errors_in_mine);
				StopTime();
				GrPaletteStepLoad (NULL);
	 
				if (ExecMessageBox(NULL, 2, TXT_CANCEL_SAVE, TXT_DO_SAVE, ErrorMessage)!=1)	{
					StartTime();
					return 1;
				}
				StartTime();
			}
		}
		ChangeFilenameExtension(temp_filename,filename,".LVL");
	}
	else
	{
		// macs are using the regular hog/rl2 files for shareware
		#if defined(SHAREWARE)
			ChangeFilenameExtension(temp_filename,filename,".SL2");
		#endif
	}

	SaveFile = fopen(temp_filename, "wb");
	if (!SaveFile)
	{
		char ErrorMessage[256];

		char fname[20];
		_splitpath(temp_filename, NULL, NULL, fname, NULL);

		sprintf(ErrorMessage, \
			"ERROR: Cannot write to '%s'.\nYou probably need to check out a locked\nversion of the file. You should save\nthis under a different filename, and then\ncheck out a locked copy by typing\n\'co -l %s.lvl'\nat the DOS prompt.\n" 
			, temp_filename, fname);
		StopTime();
		GrPaletteStepLoad (NULL);
		ExecMessageBox(NULL, 1, "Ok", ErrorMessage);
		StartTime();
		return 1;
	}

	if (gameData.missions.szCurrentLevel[0] == 0)
		strcpy(gameData.missions.szCurrentLevel,"Untitled");

	clear_transient_objects(1);		//1 means clear proximity bombs

	compress_objects();		//after this, gameData.objs.nLastObject == num gameData.objs.objects

	//make sure player is in a segment
	if (UpdateObjectSeg(&gameData.objs.objects[gameData.multi.players[0].objnum]) == 0) {
		if (gameData.objs.console->segnum > gameData.segs.nLastSegment)
			gameData.objs.console->segnum = 0;
		COMPUTE_SEGMENT_CENTER(&gameData.objs.console->pos,&(gameData.segs.segments[gameData.objs.console->segnum]);
	}
 
	FixObjectSegs();

	//Write the header

	gs_write_int(sig,SaveFile);
	gs_write_int(version,SaveFile);

	//save placeholders
	gs_write_int(minedata_offset,SaveFile);
	gs_write_int(gamedata_offset,SaveFile);

	//Now write the damn data

	//write the version 8 data (to make file unreadable by 1.0 & 1.1)
	gs_write_int(gameData.app.xGameTime,SaveFile);
	gs_write_short(gameData.app.nFrameCount,SaveFile);
	gs_write_byte(gameData.app.xFrameTime,SaveFile);

	// Write the palette file name
	fprintf(SaveFile,"%s\n",szCurrentLevelPalette);

	gs_write_int(gameStates.app.nBaseCtrlCenExplTime,SaveFile);
	gs_write_int(gameData.reactor.nStrength,SaveFile);

	gs_write_int(Num_flickering_lights,SaveFile);
	fwrite(Flickering_lights,sizeof(*Flickering_lights),Num_flickering_lights,SaveFile);
	
	gs_write_int(gameData.segs.secret.nReturnSegment, SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient.rvec.x, SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient.rvec.y, SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient.rvec.z, SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient.fvec.x, SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient.fvec.y, SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient.fvec.z, SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient.uvec.x, SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient.uvec.y, SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient.uvec.z, SaveFile);

	minedata_offset = ftell(SaveFile);
	if (!compiled_version)	
		save_mine_data(SaveFile);
	else
		save_mine_data_compiled(SaveFile);
	gamedata_offset = ftell(SaveFile);
	save_game_data(SaveFile);

	fseek(SaveFile,sizeof(sig)+sizeof(version),SEEK_SET);
	gs_write_int(minedata_offset,SaveFile);
	gs_write_int(gamedata_offset,SaveFile);

	//==================== CLOSE THE FILE =============================
	fclose(SaveFile);

	if (!compiled_version)	{
		if (gameStates.app.nFunctionMode == FMODE_EDITOR)
			editor_status("Saved mine %s, \"%s\"",filename,gameData.missions.szCurrentLevel);
	}

	return 0;

}

#if 0 //dunno - 3rd party stuff?
extern void compress_uv_coordinates_all(void);
#endif

int save_level(char * filename)
{
	int r1;

	// Save normal version...
	r1 = save_level_sub(filename, 0);

	// Save compiled version...
	save_level_sub(filename, 1);

	return r1;
}

#endif	//EDITOR

#ifndef NDEBUG
void dump_mine_info(void)
{
	int	segnum, sidenum;
	fix	min_u, max_u, min_v, max_v, min_l, max_l, max_sl;

	min_u = F1_0*1000;
	min_v = min_u;
	min_l = min_u;

	max_u = -min_u;
	max_v = max_u;
	max_l = max_u;

	max_sl = 0;

	for (segnum=0; segnum<=gameData.segs.nLastSegment; segnum++) {
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			int	vertnum;
			side	*sidep = &gameData.segs.segments[segnum].sides[sidenum];

			if (gameData.segs.segment2s[segnum].static_light > max_sl)
				max_sl = gameData.segs.segment2s[segnum].static_light;

			for (vertnum=0; vertnum<4; vertnum++) {
				if (sidep->uvls[vertnum].u < min_u)
					min_u = sidep->uvls[vertnum].u;
				else if (sidep->uvls[vertnum].u > max_u)
					max_u = sidep->uvls[vertnum].u;

				if (sidep->uvls[vertnum].v < min_v)
					min_v = sidep->uvls[vertnum].v;
				else if (sidep->uvls[vertnum].v > max_v)
					max_v = sidep->uvls[vertnum].v;

				if (sidep->uvls[vertnum].l < min_l)
					min_l = sidep->uvls[vertnum].l;
				else if (sidep->uvls[vertnum].l > max_l)
					max_l = sidep->uvls[vertnum].l;
			}

		}
	}
}

#endif

#ifdef EDITOR

//read in every level in mission and save out compiled version 
void save_all_compiled_levels(void)
{
	do_load_save_levels(1);
}

//read in every level in mission
void load_all_levels(void)
{
	do_load_save_levels(0);
}


void do_load_save_levels(int save)
{
	int level_num;

	if (! SafetyCheck())
		return;

	no_old_level_file_error=1;

	for (level_num=1;level_num<=gameData.missions.nLastLevel;level_num++) {
		LoadLevelSub(gameData.missions.szLevelNames[level_num-1]);
		LoadPalette(szCurrentLevelPalette,1,1,0);		//don't change screen
		if (save)
			save_level_sub(gameData.missions.szLevelNames[level_num-1],1);
	}

	for (level_num=-1;level_num>=gameData.missions.nLastSecretLevel;level_num--) {
		LoadLevelSub(gameData.missions.szSecretLevelNames[-level_num-1]);
		LoadPalette(szCurrentLevelPalette,1,1,0);		//don't change screen
		if (save)
			save_level_sub(gameData.missions.szSecretLevelNames[-level_num-1],1);
	}

	no_old_level_file_error=0;

}

#endif
