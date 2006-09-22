/* $Id: polyobj.c, v 1.16 2003/10/10 09:36:35 btb Exp $ */
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
 * Hacked-in polygon gameData.objs.objects
 *
 * Old Log:
 * Revision 1.3  1995/10/25  14:07:07  allender
 * removed load_poly_model function
 *
 * Revision 1.2  1995/09/14  14:10:20  allender
 * two funtions should be void
 *
 * Revision 1.1  1995/05/16  15:30:08  allender
 * Initial revision
 *
 * Revision 2.1  1995/05/26  16:10:37  john
 * Support for new 4-byte align v8 pof files.
 *
 * Revision 2.0  1995/02/27  11:32:44  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.64  1995/01/14  19:16:43  john
 * First version of new bitmap paging code.
 *
 * Revision 1.63  1994/12/14  18:06:54  matt
 * Removed compile warnings
 *
 * Revision 1.62  1994/12/09  17:54:31  john
 * Made the CFILE's close right after reading in data.
 *
 * Revision 1.61  1994/12/09  16:13:28  mike
 * speedup pof file reading, but still horribly slow using hog file...problem somewhere else.
 *
 * Revision 1.60  1994/12/08  17:41:20  yuan
 * Cfiling stuff.
 *
 * Revision 1.59  1994/11/21  11:02:19  matt
 * Added error checking
 *
 * Revision 1.58  1994/11/14  11:32:49  matt
 * Allow switching to simpler models even when altTextures specified
 *
 * Revision 1.57  1994/11/13  21:15:24  matt
 * Added basic support for more than one level of detail simplification
 *
 * Revision 1.56  1994/11/11  19:29:25  matt
 * Added code to show low detail polygon models
 *
 * Revision 1.55  1994/11/10  14:02:57  matt
 * Hacked in support for player ships with different textures
 *
 * Revision 1.54  1994/11/03  11:01:59  matt
 * Made robot pics lighted
 *
 * Revision 1.53  1994/11/02  16:18:34  matt
 * Moved DrawModelPicture () out of editor
 *
 * Revision 1.52  1994/10/18  14:38:11  matt
 * Restored assert now that bug is fixed
 *
 * Revision 1.51  1994/10/17  21:35:03  matt
 * Added support for new Control Center/Main Reactor
 *
 * Revision 1.50  1994/10/14  17:46:23  yuan
 * Made the soft Int3 only work in net mode.
 *
 * Revision 1.49  1994/10/14  17:43:47  yuan
 * Added soft int3's instead of Asserts  for some common network bugs.
 *
 * Revision 1.48  1994/10/14  17:09:04  yuan
 * Made Assert on line 610 be if in an attempt
 * to bypass.
 *
 * Revision 1.47  1994/09/09  14:23:42  matt
 * Added glow code to polygon models for engine glow
 *
 * Revision 1.46  1994/08/26  18:03:30  matt
 * Added code to remap polygon model numbers by matching filenames
 *
 * Revision 1.45  1994/08/26  15:35:58  matt
 * Made eclips usable on more than one object at a time
 *
 * Revision 1.44  1994/08/25  18:11:58  matt
 * Made player's weapons and flares fire from the positions on the 3d model.
 * Also added support for quad lasers.
 *
 * Revision 1.43  1994/07/25  00:14:18  matt
 * Made a couple of minor changes for the drivethrough
 *
 * Revision 1.42  1994/07/25  00:02:41  matt
 * Various changes to accomodate new 3d, which no longer takes point numbers
 * as parms, and now only takes pointers to points.
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: polyobj.c, v 1.16 2003/10/10 09:36:35 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -- I hate this warning in make depend! -- #ifdef DRIVE
// -- I hate this warning in make depend! -- #include "drive.h"
// -- I hate this warning in make depend! -- #else
#include "inferno.h"
// -- I hate this warning in make depend! -- #endif

#include "polyobj.h"

#include "vecmat.h"
#include "interp.h"
#include "error.h"
#include "mono.h"
#include "u_mem.h"
#include "args.h"
#include "byteswap.h"
#include "ogl_init.h"
#include "gamepal.h"

#ifndef DRIVE
#include "texmap.h"
#include "bm.h"
#include "textures.h"
#include "object.h"
#include "lighting.h"
#include "cfile.h"
#include "piggy.h"
#endif

#include "pa_enabl.h"

#ifdef _3DFX
#include "3dfx_des.h"
#endif

#define DEBUG_LEVEL CON_NORMAL

#define PM_COMPATIBLE_VERSION 6
#define PM_OBJFILE_VERSION 8

#define BASE_MODEL_SIZE 0x28000
#define DEFAULT_VIEW_DIST 0x60000

#define SHIFT_SPACE 500 // increase if insufficent

#define ID_OHDR 0x5244484f // 'RDHO'  //Object header
#define ID_SOBJ 0x4a424f53 // 'JBOS'  //Subobject header
#define ID_GUNS 0x534e5547 // 'SNUG'  //List of guns on this object
#define ID_ANIM 0x4d494e41 // 'MINA'  //Animation data
#define ID_IDTA 0x41544449 // 'ATDI'  //Interpreter data
#define ID_TXTR 0x52545854 // 'RTXT'  //Texture filename list

#define	MODEL_BUF_SIZE	32768

#define TEMP_CANV	0

#define pof_CFSeek(_buf, _len, _type) _pof_CFSeek ((_len), (_type))
#define new_pof_read_int(i, f) pof_cfread (& (i), sizeof (i), 1, (f))

int	Pof_file_end;
int	Pof_addr;

void _pof_CFSeek (int len, int type)
{
	switch (type) {
		case SEEK_SET:	Pof_addr = len;	break;
		case SEEK_CUR:	Pof_addr += len;	break;
		case SEEK_END:
			Assert (len <= 0);	//	seeking from end, better be moving back.
			Pof_addr = Pof_file_end + len;
			break;
	}

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3 ();
}

int pof_read_int (ubyte *bufp)
{
	int i;

	i = * ((int *) &bufp [Pof_addr]);
	Pof_addr += 4;
	return INTEL_INT (i);

//	if (CFRead (&i, sizeof (i), 1, f) != 1)
//		Error ("Unexpected end-of-file while reading object");
//
//	return i;
}

size_t pof_cfread (void *dst, size_t elsize, size_t nelem, ubyte *bufp)
{
	if (nelem*elsize + (size_t) Pof_addr > (size_t) Pof_file_end)
		return 0;

	memcpy (dst, &bufp [Pof_addr], elsize*nelem);

	Pof_addr += (int) (elsize * nelem);

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3 ();

	return nelem;
}

short pof_read_short (ubyte *bufp)
{
	short s;

	s = * ((short *) &bufp [Pof_addr]);
	Pof_addr += 2;
	return INTEL_SHORT (s);
//	if (CFRead (&s, sizeof (s), 1, f) != 1)
//		Error ("Unexpected end-of-file while reading object");
//
//	return s;
}

void pof_read_string (char *buf, int max_char, ubyte *bufp)
{
	int	i;

	for (i=0; i<max_char; i++) {
		if ((*buf++ = bufp [Pof_addr++]) == 0)
			break;
	}

//	while (max_char-- && (*buf=CFGetC (f)) != 0) buf++;

}

void pof_read_vecs (vms_vector *vecs, int n, ubyte *bufp)
{
//	CFRead (vecs, sizeof (vms_vector), n, f);

	memcpy (vecs, &bufp [Pof_addr], n*sizeof (*vecs));
	Pof_addr += n*sizeof (*vecs);

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	while (n > 0)
		VmsVectorSwap (&vecs [--n]);
#endif

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3 ();
}

void pof_read_angs (vms_angvec *angs, int n, ubyte *bufp)
{
	memcpy (angs, &bufp [Pof_addr], n*sizeof (*angs));
	Pof_addr += n*sizeof (*angs);

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	while (n > 0)
		VmsAngVecSwap (&angs [--n]);
#endif

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3 ();
}

#ifdef DRIVE
#define robot_info void
#else
vms_angvec anim_angs [N_ANIM_STATES][MAX_SUBMODELS];

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void robot_set_angles (robot_info *r, polymodel *pm, vms_angvec angs [N_ANIM_STATES][MAX_SUBMODELS]);
#endif

#ifdef WORDS_NEED_ALIGNMENT

//------------------------------------------------------------------------------

ubyte * old_dest (chunk o) // return where chunk is (in unaligned struct)
{
	return o.old_base + INTEL_SHORT (* ((short *) (o.old_base + o.offset)));
}

//------------------------------------------------------------------------------

ubyte * new_dest (chunk o) // return where chunk is (in aligned struct)
{
	return o.new_base + INTEL_SHORT (* ((short *) (o.old_base + o.offset))) + o.correction;
}

//------------------------------------------------------------------------------
/*
 * find chunk with smallest address
 */
int get_first_chunks_index (chunk *chunk_list, int no_chunks)
{
	int i, first_index = 0;
	Assert (no_chunks >= 1);
	for (i = 1; i < no_chunks; i++)
		if (old_dest (chunk_list [i]) < old_dest (chunk_list [first_index]))
			first_index = i;
	return first_index;
}

//------------------------------------------------------------------------------

void AlignPolyModelData (polymodel *pm)
{
	int i, chunk_len;
	int total_correction = 0;
	ubyte *cur_old, *cur_new;
	chunk cur_ch;
	chunk ch_list [MAX_CHUNKS];
	int no_chunks = 0;
	int tmp_size = pm->model_data_size + SHIFT_SPACE;
	ubyte *tmp = d_malloc (tmp_size); // where we build the aligned version of pm->model_data

	Assert (tmp != NULL);
	//start with first chunk (is always aligned!)
	cur_old = pm->model_data;
	cur_new = tmp;
	chunk_len = get_chunks (cur_old, cur_new, ch_list, &no_chunks);
	memcpy (cur_new, cur_old, chunk_len);
	while (no_chunks > 0) {
		int first_index = get_first_chunks_index (ch_list, no_chunks);
		cur_ch = ch_list [first_index];
		// remove first chunk from array:
		no_chunks--;
		for (i = first_index; i < no_chunks; i++)
			ch_list [i] = ch_list [i + 1];
		// if (new) address unaligned:
		if ((u_int32_t)new_dest (cur_ch) % 4L != 0) {
			// calculate how much to move to be aligned
			short to_shift = 4 - (u_int32_t)new_dest (cur_ch) % 4L;
			// correct chunks' addresses
			cur_ch.correction += to_shift;
			for (i = 0; i < no_chunks; i++)
				ch_list [i].correction += to_shift;
			total_correction += to_shift;
			Assert ((u_int32_t)new_dest (cur_ch) % 4L == 0);
			Assert (total_correction <= SHIFT_SPACE); // if you get this, increase SHIFT_SPACE
		}
		//write (corrected) chunk for current chunk:
		* ((short *) (cur_ch.new_base + cur_ch.offset))
		  = INTEL_SHORT (cur_ch.correction)
				+ INTEL_SHORT (* ((short *) (cur_ch.old_base + cur_ch.offset)));
		//write (correctly aligned) chunk:
		cur_old = old_dest (cur_ch);
		cur_new = new_dest (cur_ch);
		chunk_len = get_chunks (cur_old, cur_new, ch_list, &no_chunks);
		memcpy (cur_new, cur_old, chunk_len);
		//correct submodel_ptr's for pm, too
		for (i = 0; i < MAX_SUBMODELS; i++)
			if (pm->model_data + pm->submodel_ptrs [i] >= cur_old
			    && pm->model_data + pm->submodel_ptrs [i] < cur_old + chunk_len)
				pm->submodel_ptrs [i] += (cur_new - tmp) - (cur_old - pm->model_data);
 	}
	d_free (pm->model_data);
	pm->model_data_size += total_correction;
	pm->model_data = d_malloc (pm->model_data_size);
	Assert (pm->model_data != NULL);
	memcpy (pm->model_data, tmp, pm->model_data_size);
	d_free (tmp);
}
#endif //def WORDS_NEED_ALIGNMENT

//------------------------------------------------------------------------------
//reads a binary file containing a 3d model
polymodel *ReadModelFile (polymodel *pm, char *filename, robot_info *r)
{
	CFILE *ifile;
	short version;
	int id, len, next_chunk;
	int anim_flag = 0;
	ubyte *model_buf;

if (!(model_buf = (ubyte *)d_malloc (MODEL_BUF_SIZE * sizeof (ubyte))))
	Error ("Can't allocate space to read model %s\n", filename);
if (!(ifile=CFOpen (filename, gameFolders.szDataDir, "rb", 0)))
	Error ("Can't open file <%s>", filename);
Assert (CFLength (ifile, 0) <= MODEL_BUF_SIZE);
Pof_addr = 0;
Pof_file_end = (int) CFRead (model_buf, 1, CFLength (ifile, 0), ifile);
CFClose (ifile);
id = pof_read_int (model_buf);
if (id!=0x4f505350) /* 'OPSP' */
	Error ("Bad ID in model file <%s>", filename);
version = pof_read_short (model_buf);
if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
	Error ("Bad version (%d) in model file <%s>", version, filename);
//if (FindArg ("-bspgen"))
//printf ("bspgen -c1");
while (new_pof_read_int (id, model_buf) == 1) {
	id = INTEL_INT (id);
	//id  = pof_read_int (model_buf);
	len = pof_read_int (model_buf);
	next_chunk = Pof_addr + len;
	switch (id) {
		case ID_OHDR: {		//Object header
			vms_vector pmmin, pmmax;
			pm->n_models = pof_read_int (model_buf);
			pm->rad = pof_read_int (model_buf);
			Assert (pm->n_models <= MAX_SUBMODELS);
			pof_read_vecs (&pmmin, 1, model_buf);
			pof_read_vecs (&pmmax, 1, model_buf);
			if (FindArg ("-bspgen")) {
				vms_vector v;
				fix l;
				VmVecSub (&v, &pmmax, &pmmin);
				l = v.x;
				if (v.y > l) 
					l = v.y;					
				if (v.z > l) 
					l = v.z;					
				//printf (" -l%.3f", f2fl (l));
				}
			break;
			}

		case ID_SOBJ: {		//Subobject header
			int n = pof_read_short (model_buf);
			Assert (n < MAX_SUBMODELS);
			anim_flag++;
			pm->submodel_parents [n] = (char) pof_read_short (model_buf);
			pof_read_vecs (&pm->submodel_norms [n], 1, model_buf);
			pof_read_vecs (&pm->submodel_pnts [n], 1, model_buf);
			pof_read_vecs (&pm->submodel_offsets [n], 1, model_buf);
			pm->submodel_rads [n] = pof_read_int (model_buf);		//radius
			pm->submodel_ptrs [n] = pof_read_int (model_buf);	//offset
			break;
			}

#ifndef DRIVE
		case ID_GUNS: {		//List of guns on this object
			if (r) {
				int i;
				vms_vector gun_dir;
				ubyte gun_used [MAX_GUNS];
				r->n_guns = pof_read_int (model_buf);
				if (r->n_guns)
					anim_flag++;
				Assert (r->n_guns <= MAX_GUNS);
				for (i = 0; i < r->n_guns; i++)
					gun_used [i] = 0;
				for (i = 0; i < r->n_guns; i++) {
					int id = pof_read_short (model_buf);
					Assert (id < r->n_guns);
					Assert (gun_used [id] == 0);
					gun_used [id] = 1;
					r->gun_submodels [id] = (char) pof_read_short (model_buf);
					Assert (r->gun_submodels [id] != 0xff);
					pof_read_vecs (&r->gun_points [id], 1, model_buf);
					if (version >= 7)
						pof_read_vecs (&gun_dir, 1, model_buf);
					}
				}
			else
				pof_CFSeek (model_buf, len, SEEK_CUR);
			break;
			}

		case ID_ANIM:		//Animation data
			anim_flag++;
			if (r) {
				int f, m, n_frames = pof_read_short (model_buf);
				Assert (n_frames == N_ANIM_STATES);
				for (m = 0; m <pm->n_models; m++)
					for (f = 0; f < n_frames; f++)
						pof_read_angs (&anim_angs [f][m], 1, model_buf);
							robot_set_angles (r, pm, anim_angs);
				}
			else
				pof_CFSeek (model_buf, len, SEEK_CUR);
			break;
#endif

		case ID_TXTR: {		//Texture filename list
			char name_buf [128];
			int n = pof_read_short (model_buf);
			while (n--)
				pof_read_string (name_buf, 128, model_buf);
			break;
			}

		case ID_IDTA:		//Interpreter data
			pm->model_data = d_malloc (len);
			pm->model_data_size = len;
			pof_cfread (pm->model_data, 1, len, model_buf);
			break;
	
		default:
			pof_CFSeek (model_buf, len, SEEK_CUR);
			break;
		}
	if (version >= 8)		// Version 8 needs 4-byte alignment!!!
		pof_CFSeek (model_buf, next_chunk, SEEK_SET);
	}
//	for (i=0;i<pm->n_models;i++)
//		pm->submodel_ptrs [i] += (int) pm->model_data;
if (FindArg ("-bspgen")) {
	char *p = strchr (filename, '.');
	*p = 0;
	//if (anim_flag > 1)
	//printf (" -a");
	//printf (" %s.3ds\n", filename);
	*p = '.';
	}
d_free (model_buf);
#ifdef WORDS_NEED_ALIGNMENT
G3AlignPolyModelData (pm);
#endif
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__) 
G3SwapPolyModelData (pm->model_data);
#endif
	//verify (pm->model_data);
return pm;
}

//------------------------------------------------------------------------------
//reads the gun information for a model
//fills in arrays gun_points & gun_dirs, returns the number of guns read
int read_model_guns (char *filename, vms_vector *gun_points, vms_vector *gun_dirs, int *gun_submodels)
{
	CFILE *ifile;
	short version;
	int id, len;
	int n_guns=0;
	ubyte	*model_buf;

	model_buf = (ubyte *)d_malloc (MODEL_BUF_SIZE * sizeof (ubyte));
	if (!model_buf)
		Error ("Can't allocate space to read model %s\n", filename);

	if ((ifile=CFOpen (filename, gameFolders.szDataDir, "rb", 0))==NULL)
		Error ("Can't open file <%s>", filename);

	Assert (CFLength (ifile, 0) <= MODEL_BUF_SIZE);

	Pof_addr = 0;
	Pof_file_end = (int) CFRead (model_buf, 1, CFLength (ifile, 0), ifile);
	CFClose (ifile);

	id = pof_read_int (model_buf);

	if (id!=0x4f505350) /* 'OPSP' */
		Error ("Bad ID in model file <%s>", filename);

	version = pof_read_short (model_buf);

	Assert (version >= 7);		//must be 7 or higher for this data

	if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
		Error ("Bad version (%d) in model file <%s>", version, filename);

	while (new_pof_read_int (id, model_buf) == 1) {
		id = INTEL_INT (id);
		//id  = pof_read_int (model_buf);
		len = pof_read_int (model_buf);

		if (id == ID_GUNS) {		//List of guns on this object

			int i;

			n_guns = pof_read_int (model_buf);

			for (i=0;i<n_guns;i++) {
				int id, sm;

				id = pof_read_short (model_buf);
				sm = pof_read_short (model_buf);
				if (gun_submodels)
					gun_submodels [id] = sm;
				else if (sm!=0)
					Error ("Invalid gun submodel in file <%s>", filename);
				pof_read_vecs (&gun_points [id], 1, model_buf);

				pof_read_vecs (&gun_dirs [id], 1, model_buf);
			}

		}
		else
			pof_CFSeek (model_buf, len, SEEK_CUR);

	}

	d_free (model_buf);
	
	return n_guns;
}

//------------------------------------------------------------------------------
//d_free up a model, getting rid of all its memory
void FreeModel (polymodel *po)
{
if (po->model_data) {
	d_free (po->model_data);
	po->model_data = NULL;
	}
}

//------------------------------------------------------------------------------

//draw a polygon model

void DrawPolygonModel (
	object			*objP, 
	vms_vector		*pos, 
	vms_matrix		*orient, 
	vms_angvec		*anim_angles, 
	int				model_num, 
	int				flags, 
	fix				light, 
	fix				*glow_values, 
	bitmap_index	altTextures [], 
	tRgbColorf		*color)
{
	polymodel	*po;
	int			i, j, nTextures;
	PA_DFX (int save_light);

	if (model_num >= gameData.models.nPolyModels)
		return;
	Assert (model_num < gameData.models.nPolyModels);
	po = gameData.models.polyModels + model_num;
	if (objP && ((objP->type == OBJ_ROBOT) || (objP->type == OBJ_PLAYER)) && (gameStates.render.nShadowPass == 2)) {
		G3SetInterpPoints (gameData.models.polyModelPoints);
		G3DrawPolyModelShadow (objP, po->model_data, anim_angles, gameData.bots.pofData [gameStates.app.bD1Mission] + objP->id);
		return;
		}
	//check if should use simple model (depending on detail level chosen)
	if (po->simpler_model)					//must have a simpler model
		if (!flags) {							//can't switch if this is debris
			int	cnt = 1;
			fix depth = G3CalcPointDepth (pos);		//gets 3d depth
			while (po->simpler_model && (depth > cnt++ * gameData.models.nSimpleModelThresholdScale * po->rad))
				po = gameData.models.polyModels + po->simpler_model - 1;
			}
	nTextures = po->n_textures;
	if (altTextures) {
		for (i = 0; i < nTextures; i++)	{
			gameData.models.textureIndex [i] = altTextures [i];
			gameData.models.textures [i] = gameData.pig.tex.bitmaps [gameStates.app.bD1Model] + altTextures [i].index;
#ifdef _3DFX
         gameData.models.textures [i]->bm_handle = gameData.models.textureIndex [i].index;
#endif
			}
		}
	else {
		for (i = 0, j = po->first_texture; i < nTextures; i++, j++) {
			gameData.models.textureIndex [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [j]];
			gameData.models.textures [i] = gameData.pig.tex.bitmaps [gameStates.app.bD1Model] + gameData.models.textureIndex [i].index;
#ifdef _3DFX
         gameData.models.textures [i]->bm_handle = gameData.models.textureIndex [i].index;
#endif
		}
   }

#ifdef PIGGY_USE_PAGING
	// Make sure the textures for this object are paged in...
	gameData.pig.tex.bPageFlushed = 0;
	for (i = 0; i < nTextures; i++)	
		PIGGY_PAGE_IN (gameData.models.textureIndex [i], gameStates.app.bD1Model);
	// Hmmm... cache got flushed in the middle of paging all these in, 
	// so we need to reread them all in.
	if (gameData.pig.tex.bPageFlushed)	{
		gameData.pig.tex.bPageFlushed = 0;
		for (i = 0; i < nTextures; i++)	
			PIGGY_PAGE_IN (gameData.models.textureIndex [i], gameStates.app.bD1Model);
	}
	// Make sure that they can all fit in memory.
	Assert (gameData.pig.tex.bPageFlushed == 0);
#endif
	G3StartInstanceMatrix (pos, orient);
	G3SetInterpPoints (gameData.models.polyModelPoints);
#ifdef _3DFX
   _3dfx_rendering_poly_obj = 1;
#endif
	PA_DFX (save_light = gameStates.render.nLighting);
	PA_DFX (gameStates.render.nLighting = 0);

	if (flags == 0)		//draw entire object
		G3DrawPolyModel (po->model_data, gameData.models.textures, anim_angles, light, glow_values, color);
	else {
		int i;
	
		for (i = 0; flags; flags >>= 1, i++)
			if (flags & 1) {
				vms_vector ofs;

				Assert (i < po->n_models);
				//if submodel, rotate around its center point, not pivot point
				VmVecAvg (&ofs, &po->submodel_mins [i], &po->submodel_maxs [i]);
				VmVecNegate (&ofs);
				G3StartInstanceMatrix (&ofs, NULL);
				G3DrawPolyModel (&po->model_data [po->submodel_ptrs [i]], gameData.models.textures, anim_angles, 
									 light, glow_values, color);
				G3DoneInstance ();
				}	
		}
	G3DoneInstance ();
#ifdef _3DFX
   _3dfx_rendering_poly_obj = 0;
#endif
	PA_DFX (gameStates.render.nLighting = save_light);
}

//------------------------------------------------------------------------------

void polyobj_find_min_max (polymodel *pm)
{
	ushort nverts;
	vms_vector *vp;
	ushort *data, type;
	int m;
	vms_vector *big_mn, *big_mx;
	
	big_mn = &pm->mins;
	big_mx = &pm->maxs;

	for (m = 0; m < pm->n_models; m++) {
		vms_vector *mn, *mx, *ofs;

		mn = pm->submodel_mins + m;
		mx = pm->submodel_maxs + m;
		ofs= pm->submodel_offsets + m;
		data = (ushort *) (pm->model_data + pm->submodel_ptrs [m]);
		type = *data++;
		Assert (type == 7 || type == 1);
		nverts = *data++;
		if (type==7)
			data+=2;		//skip start & pad
		vp = (vms_vector *) data;
		*mn = *mx = *vp++; 
		nverts--;
		if (m == 0)
			*big_mn = *big_mx = *mn;
		while (nverts--) {
			if (vp->x > mx->x) mx->x = vp->x;
			if (vp->y > mx->y) mx->y = vp->y;
			if (vp->z > mx->z) mx->z = vp->z;
			if (vp->x < mn->x) mn->x = vp->x;
			if (vp->y < mn->y) mn->y = vp->y;
			if (vp->z < mn->z) mn->z = vp->z;
			if (vp->x + ofs->x > big_mx->x) big_mx->x = vp->x + ofs->x;
			if (vp->y + ofs->y > big_mx->y) big_mx->y = vp->y + ofs->y;
			if (vp->z + ofs->z > big_mx->z) big_mx->z = vp->z + ofs->z;
			if (vp->x + ofs->x < big_mn->x) big_mn->x = vp->x + ofs->x;
			if (vp->y + ofs->y < big_mn->y) big_mn->y = vp->y + ofs->y;
			if (vp->z + ofs->z < big_mn->z) big_mn->z = vp->z + ofs->z;
			vp++;
		}

	//printf ("Submodel %d:  (%8x, %8x) (%8x, %8x) (%8x, %8x)\n", m, mn->x, mx->x, mn->y, mx->y, mn->z, mx->z);
	}

//printf ("Whole model: (%8x, %8x) (%8x, %8x) (%8x, %8x)\n", big_mn->x, big_mx->x, big_mn->y, big_mx->y, big_mn->z, big_mx->z);

}

//------------------------------------------------------------------------------

extern short nHighestTexture;	//from interp.c

char Pof_names [MAX_POLYGON_MODELS][SHORT_FILENAME_LEN];

//returns the number of this model
#ifndef DRIVE
int LoadPolygonModel (char *filename, int n_textures, int first_texture, robot_info *r)
#else
int LoadPolygonModel (char *filename, int n_textures, grs_bitmap ***textures)
#endif
{
	#ifdef DRIVE
	#define r NULL
	#endif

	Assert (gameData.models.nPolyModels < MAX_POLYGON_MODELS);
	Assert (n_textures < MAX_POLYOBJ_TEXTURES);

	//	MK was real tired of those useless, slow mprintfs...
#if TRACE	
	if (gameData.models.nPolyModels > MAX_POLYGON_MODELS - 10)
		con_printf (CON_VERBOSE, "Used %d/%d polygon model slots\n", gameData.models.nPolyModels+1, MAX_POLYGON_MODELS);
#endif
	Assert (strlen (filename) <= 12);
	strcpy (Pof_names [gameData.models.nPolyModels], filename);
	ReadModelFile (gameData.models.polyModels+gameData.models.nPolyModels, filename, r);
	polyobj_find_min_max (gameData.models.polyModels+gameData.models.nPolyModels);
	G3InitPolyModel (gameData.models.polyModels [gameData.models.nPolyModels].model_data);
	if (nHighestTexture + 1 != n_textures)
		Error ("Model <%s> references %d textures but specifies %d.", filename, nHighestTexture+1, n_textures);
	gameData.models.polyModels [gameData.models.nPolyModels].n_textures = n_textures;
	gameData.models.polyModels [gameData.models.nPolyModels].first_texture = first_texture;
	gameData.models.polyModels [gameData.models.nPolyModels].simpler_model = 0;
//	Assert (polygon_models [gameData.models.nPolyModels]!=NULL);
	gameData.models.nPolyModels++;
	return gameData.models.nPolyModels-1;
}

//------------------------------------------------------------------------------

void _CDECL_ free_polygon_models (void)
{
	int i;

LogErr ("unloading poly models\n");
for (i = 0; i < gameData.models.nPolyModels; i++) {
	FreeModel (gameData.models.polyModels + i);
	FreeModel (gameData.models.defPolyModels + i);
	}
}

//------------------------------------------------------------------------------

void InitPolygonModels ()
{
	gameData.models.nPolyModels = 0;
	atexit (free_polygon_models);
}

//------------------------------------------------------------------------------
//compare against this size when figuring how far to place eye for picture
//draws the given model in the current canvas.  The distance is set to
//more-or-less fill the canvas.  Note that this routine actually renders
//into an off-screen canvas that it creates, then copies to the current
//canvas.

void DrawModelPicture (int mn, vms_angvec *orient_angles)
{
	vms_vector	temp_pos=ZERO_VECTOR;
	vms_matrix	temp_orient = IDENTITY_MATRIX;
#if TEMP_CANV
	grs_canvas	*save_canv = grdCurCanv, *temp_canv;
#endif
	Assert (mn>=0 && mn<gameData.models.nPolyModels);
#if TEMP_CANV
	temp_canv = GrCreateCanvas (save_canv->cv_bitmap.bm_props.w, save_canv->cv_bitmap.bm_props.h);
	temp_canv->cv_bitmap.bm_props.x = grdCurCanv->cv_bitmap.bm_props.x;
	temp_canv->cv_bitmap.bm_props.y = grdCurCanv->cv_bitmap.bm_props.y;
	GrSetCurrentCanvas (temp_canv);
#endif
	GrClearCanvas (0);
	G3StartFrame (1, 0);
	glDisable (GL_BLEND);
	G3SetViewMatrix (&temp_pos, &temp_orient, 0x9000);
	if (gameData.models.polyModels [mn].rad != 0)
		temp_pos.z = fixmuldiv (DEFAULT_VIEW_DIST, gameData.models.polyModels [mn].rad, BASE_MODEL_SIZE);
	else
		temp_pos.z = DEFAULT_VIEW_DIST;
	VmAngles2Matrix (&temp_orient, orient_angles);
	PA_DFX (save_light = gameStates.render.nLighting);
	PA_DFX (gameStates.render.nLighting = 0);
	DrawPolygonModel (NULL, &temp_pos, &temp_orient, NULL, mn, 0, f1_0, NULL, NULL, NULL);
	PA_DFX (gameStates.render.nLighting = save_light);
#if TEMP_CANV
	GrSetCurrentCanvas (save_canv);
	glDisable (GL_BLEND);
	GrBitmap (0, 0, &temp_canv->cv_bitmap);
	glEnable (GL_BLEND);
	GrFreeCanvas (temp_canv);
#endif
	G3EndFrame ();
	if (curDrawBuffer != GL_BACK)
		GrUpdate (0);
}

//------------------------------------------------------------------------------

#ifndef FAST_FILE_IO
/*
 * reads a polymodel structure from a CFILE
 */
extern void PolyModelRead (polymodel *pm, CFILE *fp)
{
	int i;

pm->n_models = CFReadInt (fp);
pm->model_data_size = CFReadInt (fp);
CFReadInt (fp);
pm->model_data = NULL;
for (i = 0; i < MAX_SUBMODELS; i++)
	pm->submodel_ptrs [i] = CFReadInt (fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	CFReadVector (& (pm->submodel_offsets [i]), fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	CFReadVector (& (pm->submodel_norms [i]), fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	CFReadVector (& (pm->submodel_pnts [i]), fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	pm->submodel_rads [i] = CFReadFix (fp);
CFRead (pm->submodel_parents, MAX_SUBMODELS, 1, fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	CFReadVector (& (pm->submodel_mins [i]), fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	CFReadVector (& (pm->submodel_maxs [i]), fp);
CFReadVector (& (pm->mins), fp);
CFReadVector (& (pm->maxs), fp);
pm->rad = CFReadFix (fp);
pm->n_textures = CFReadByte (fp);
pm->first_texture = CFReadShort (fp);
pm->simpler_model = CFReadByte (fp);
}

//------------------------------------------------------------------------------
/*
 * reads n polymodel structs from a CFILE
 */
extern int PolyModelReadN (polymodel *pm, int n, CFILE *fp)
{
	int i;

for (i = n; i; i--, pm++)
	PolyModelRead (pm, fp);
return i;
}
#endif

//------------------------------------------------------------------------------
/*
 * routine which allocates, reads, and inits a polymodel's model_data
 */
void PolyModelDataRead (polymodel *pm, polymodel *pdm, CFILE *fp)
{
	if (pm->model_data)
		d_free (pm->model_data);
	pm->model_data = d_malloc (pm->model_data_size);
	Assert (pm->model_data != NULL);
	CFRead (pm->model_data, sizeof (ubyte), pm->model_data_size, fp);
	if (pdm) {
		if (pdm->model_data)
			d_free (pdm->model_data);
		pdm->model_data = d_malloc (pm->model_data_size);
		Assert (pdm->model_data != NULL);
		memcpy (pdm->model_data, pm->model_data, pm->model_data_size);
		}
#ifdef WORDS_NEED_ALIGNMENT
	AlignPolyModelData (pm);
#endif
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__) 
	G3SwapPolyModelData (pm->model_data);
#endif
	//verify (pm->model_data);
	G3InitPolyModel (pm->model_data);
}

//------------------------------------------------------------------------------
//eof
