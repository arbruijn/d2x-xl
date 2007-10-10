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
#include "network.h"
#include "strutil.h"

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
#define ID_GUNS 0x534e5547 // 'SNUG'  //List of guns on this tObject
#define ID_ANIM 0x4d494e41 // 'MINA'  //Animation data
#define ID_IDTA 0x41544449 // 'ATDI'  //Interpreter data
#define ID_TXTR 0x52545854 // 'RTXT'  //Texture filename list

#define	MODEL_BUF_SIZE	32768

#define TEMP_CANV	0

#define pof_CFSeek(_buf, _len, Type) _pof_CFSeek ((_len), (Type))
#define new_pof_read_int(i, f) pof_cfread (& (i), sizeof (i), 1, (f))

int	Pof_file_end;
int	Pof_addr;

void _pof_CFSeek (int len, int nType)
{
	switch (nType) {
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
//		Error ("Unexpected end-of-file while reading tObject");
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
//		Error ("Unexpected end-of-file while reading tObject");
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

void pof_read_vecs (vmsVector *vecs, int n, ubyte *bufp)
{
//	CFRead (vecs, sizeof (vmsVector), n, f);

	memcpy (vecs, &bufp [Pof_addr], n*sizeof (*vecs));
	Pof_addr += n*sizeof (*vecs);

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	while (n > 0)
		VmsVectorSwap (&vecs [--n]);
#endif

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3 ();
}

void pof_read_angs (vmsAngVec *angs, int n, ubyte *bufp)
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
#define tRobotInfo void
#else
vmsAngVec anim_angs [N_ANIM_STATES][MAX_SUBMODELS];

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void robot_set_angles (tRobotInfo *r, tPolyModel *pm, vmsAngVec angs [N_ANIM_STATES][MAX_SUBMODELS]);
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

void AlignPolyModelData (tPolyModel *pm)
{
	int i, chunk_len;
	int total_correction = 0;
	ubyte *cur_old, *cur_new;
	chunk cur_ch;
	chunk ch_list [MAX_CHUNKS];
	int no_chunks = 0;
	int tmp_size = pm->nDataSize + SHIFT_SPACE;
	ubyte *tmp = D2_ALLOC (tmp_size); // where we build the aligned version of pm->modelData

	Assert (tmp != NULL);
	//start with first chunk (is always aligned!)
	cur_old = pm->modelData;
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
			if (pm->modelData + pm->subModels.ptrs [i] >= cur_old
			    && pm->modelData + pm->subModels.ptrs [i] < cur_old + chunk_len)
				pm->subModels.ptrs [i] += (cur_new - tmp) - (cur_old - pm->modelData);
 	}
	D2_FREE (pm->modelData);
	pm->nDataSize += total_correction;
	pm->modelData = D2_ALLOC (pm->nDataSize);
	Assert (pm->modelData != NULL);
	memcpy (pm->modelData, tmp, pm->nDataSize);
	D2_FREE (tmp);
}
#endif //def WORDS_NEED_ALIGNMENT

//------------------------------------------------------------------------------
//reads a binary file containing a 3d model
tPolyModel *ReadModelFile (tPolyModel *pm, char *filename, tRobotInfo *r)
{
	CFILE *ifile;
	short version;
	int id, len, next_chunk;
	int animFlag = 0;
	ubyte *model_buf;

if (!(model_buf = (ubyte *)D2_ALLOC (MODEL_BUF_SIZE * sizeof (ubyte))))
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
			vmsVector pmmin, pmmax;
			pm->nModels = pof_read_int (model_buf);
			pm->rad = pof_read_int (model_buf);
			Assert (pm->nModels <= MAX_SUBMODELS);
			pof_read_vecs (&pmmin, 1, model_buf);
			pof_read_vecs (&pmmax, 1, model_buf);
			if (FindArg ("-bspgen")) {
				vmsVector v;
				fix l;
				VmVecSub (&v, &pmmax, &pmmin);
				l = v.p.x;
				if (v.p.y > l) 
					l = v.p.y;					
				if (v.p.z > l) 
					l = v.p.z;					
				//printf (" -l%.3f", f2fl (l));
				}
			break;
			}

		case ID_SOBJ: {		//Subobject header
			int n = pof_read_short (model_buf);
			Assert (n < MAX_SUBMODELS);
			animFlag++;
			pm->subModels.parents [n] = (char) pof_read_short (model_buf);
			pof_read_vecs (&pm->subModels.norms [n], 1, model_buf);
			pof_read_vecs (&pm->subModels.pnts [n], 1, model_buf);
			pof_read_vecs (&pm->subModels.offsets [n], 1, model_buf);
			pm->subModels.rads [n] = pof_read_int (model_buf);		//radius
			pm->subModels.ptrs [n] = pof_read_int (model_buf);	//offset
			break;
			}

#ifndef DRIVE
		case ID_GUNS: {		//List of guns on this tObject
			if (r) {
				int i;
				vmsVector gun_dir;
				ubyte gun_used [MAX_GUNS];
				r->nGuns = pof_read_int (model_buf);
				if (r->nGuns)
					animFlag++;
				Assert (r->nGuns <= MAX_GUNS);
				for (i = 0; i < r->nGuns; i++)
					gun_used [i] = 0;
				for (i = 0; i < r->nGuns; i++) {
					int id = pof_read_short (model_buf);
					Assert (id < r->nGuns);
					Assert (gun_used [id] == 0);
					gun_used [id] = 1;
					r->gunSubModels [id] = (char) pof_read_short (model_buf);
					Assert (r->gunSubModels [id] != 0xff);
					pof_read_vecs (&r->gunPoints [id], 1, model_buf);
					if (version >= 7)
						pof_read_vecs (&gun_dir, 1, model_buf);
					}
				}
			else
				pof_CFSeek (model_buf, len, SEEK_CUR);
			break;
			}

		case ID_ANIM:		//Animation data
			animFlag++;
			if (r) {
				int f, m, n_frames = pof_read_short (model_buf);
				Assert (n_frames == N_ANIM_STATES);
				for (m = 0; m <pm->nModels; m++)
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
			pm->modelData = D2_ALLOC (len);
			pm->nDataSize = len;
			pof_cfread (pm->modelData, 1, len, model_buf);
			break;
	
		default:
			pof_CFSeek (model_buf, len, SEEK_CUR);
			break;
		}
	if (version >= 8)		// Version 8 needs 4-byte alignment!!!
		pof_CFSeek (model_buf, next_chunk, SEEK_SET);
	}
//	for (i=0;i<pm->nModels;i++)
//		pm->subModels.ptrs [i] += (int) pm->modelData;
if (FindArg ("-bspgen")) {
	char *p = strchr (filename, '.');
	*p = 0;
	//if (animFlag > 1)
	//printf (" -a");
	//printf (" %s.3ds\n", filename);
	*p = '.';
	}
D2_FREE (model_buf);
#ifdef WORDS_NEED_ALIGNMENT
G3AlignPolyModelData (pm);
#endif
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__) 
G3SwapPolyModelData (pm->modelData);
#endif
	//verify (pm->modelData);
return pm;
}

//------------------------------------------------------------------------------
//reads the gun information for a model
//fills in arrays gunPoints & gun_dirs, returns the number of guns read
int read_model_guns (char *filename, vmsVector *gunPoints, vmsVector *gun_dirs, int *gunSubModels)
{
	CFILE *ifile;
	short version;
	int id, len;
	int nGuns=0;
	ubyte	*model_buf;

	model_buf = (ubyte *)D2_ALLOC (MODEL_BUF_SIZE * sizeof (ubyte));
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

		if (id == ID_GUNS) {		//List of guns on this tObject

			int i;

			nGuns = pof_read_int (model_buf);

			for (i=0;i<nGuns;i++) {
				int id, sm;

				id = pof_read_short (model_buf);
				sm = pof_read_short (model_buf);
				if (gunSubModels)
					gunSubModels [id] = sm;
				else if (sm!=0)
					Error ("Invalid gun submodel in file <%s>", filename);
				pof_read_vecs (&gunPoints [id], 1, model_buf);

				pof_read_vecs (&gun_dirs [id], 1, model_buf);
			}

		}
		else
			pof_CFSeek (model_buf, len, SEEK_CUR);

	}

	D2_FREE (model_buf);
	
	return nGuns;
}

//------------------------------------------------------------------------------
//D2_FREE up a model, getting rid of all its memory
void FreeModel (tPolyModel *po)
{
if (po->modelData)
	D2_FREE (po->modelData);
}

//------------------------------------------------------------------------------

tPolyModel *GetPolyModel (tObject *objP, vmsVector *pos, int nModel, int flags)
{
	tPolyModel	*po = NULL;
	int			bHaveAltModel = gameData.models.altPolyModels [nModel].modelData != NULL,
					bIsDefModel = (gameData.models.polyModels [nModel].nDataSize == 
										gameData.models.defPolyModels [nModel].nDataSize);

if ((nModel >= gameData.models.nPolyModels) && !(po = gameData.models.modelToPOL [nModel]))
	return NULL;
// only render shadows for custom models and for standard models with a shadow proof alternative model
if (!objP) 
	po = ((gameStates.app.bAltModels && bIsDefModel && bHaveAltModel) ? gameData.models.altPolyModels : gameData.models.polyModels) + nModel;
else if (!po) {
	if (!(bIsDefModel && bHaveAltModel)) {
		if (gameStates.app.bFixModels && (objP->nType == OBJ_ROBOT) && (gameStates.render.nShadowPass == 2))
			return NULL;
		po = gameData.models.polyModels + nModel;
		}
	else if (gameStates.render.nShadowPass != 2) {
		if ((gameStates.app.bAltModels || (objP->nType == OBJ_PLAYER)) && bHaveAltModel)
			po = gameData.models.altPolyModels + nModel;
		else
			po = gameData.models.polyModels + nModel;
		}
	else if (bHaveAltModel)
		po = gameData.models.altPolyModels + nModel;
	else
		return NULL;
	if (gameStates.render.nShadowPass == 2) {
		if (objP->nType == OBJ_ROBOT) {
			if (!gameOpts->render.shadows.bRobots)
				return NULL;
			if (objP->cType.aiInfo.CLOAKED)
				return NULL;
			}
		else if (objP->nType == OBJ_WEAPON) {
			if (!gameOpts->render.shadows.bMissiles)
				return NULL;
			if (!gameData.objs.bIsMissile [objP->id] && (objP->id != SMALLMINE_ID))
				return NULL;
			}
		else if ((objP->nType == OBJ_POWERUP)) {
			if (!gameOpts->render.shadows.bPowerups)
				return NULL;
			}
		else if (objP->nType == OBJ_PLAYER) {
			if (!gameOpts->render.shadows.bPlayers)
				return NULL;
			if (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_CLOAKED)
				return NULL;
			}
		else if (objP->nType == OBJ_CNTRLCEN) {
			if (!gameOpts->render.shadows.bReactors)
				return NULL;
			if (!(nModel & 1))	// use the working reactor model for rendering destroyed reactors' shadows
				po--;
			}
		else 
			return NULL;
		return po;
		}
	}
//check if should use simple model (depending on detail level chosen)
if (!(SHOW_DYN_LIGHT || SHOW_SHADOWS) && po->nSimplerModel && !flags && pos) {
	int	cnt = 1;
	fix depth = G3CalcPointDepth (pos);		//gets 3d depth
	while (po->nSimplerModel && (depth > cnt++ * gameData.models.nSimpleModelThresholdScale * po->rad))
		po = gameData.models.polyModels + po->nSimplerModel - 1;
	}
return po;
}

//------------------------------------------------------------------------------

int LoadModelTextures (tPolyModel *po, tBitmapIndex *altTextures)
{
	int	i, j, nTextures = po->nTextures;

if (altTextures) {
	for (i = 0; i < nTextures; i++)	{
		gameData.models.textureIndex [i] = altTextures [i];
		gameData.models.textures [i] = gameData.pig.tex.bitmaps [gameStates.app.bD1Model] + altTextures [i].index;
#ifdef _3DFX
      gameData.models.textures [i]->bmHandle = gameData.models.textureIndex [i].index;
#endif
		}
	}
else {
	for (i = 0, j = po->nFirstTexture; i < nTextures; i++, j++) {
		gameData.models.textureIndex [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [j]];
		gameData.models.textures [i] = gameData.pig.tex.bitmaps [gameStates.app.bD1Model] + gameData.models.textureIndex [i].index;
#ifdef _3DFX
      gameData.models.textures [i]->bmHandle = gameData.models.textureIndex [i].index;
#endif
		}
	}
#ifdef PIGGY_USE_PAGING
// Make sure the textures for this tObject are paged in...
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
return nTextures;
}

//------------------------------------------------------------------------------

//draw a polygon model
extern int nInstanceDepth;

void DrawPolygonModel (
	tObject			*objP, 
	vmsVector		*pos, 
	vmsMatrix		*orient, 
	vmsAngVec		*animAngles, 
	int				nModel, 
	int				flags, 
	fix				light, 
	fix				*glowValues, 
	tBitmapIndex	altTextures [], 
	tRgbaColorf		*color)
{
	tPolyModel	*po;
	int			nTextures;

if (!(po = GetPolyModel (objP, pos, nModel, flags)))
	return;
if (gameStates.render.nShadowPass == 2) {
	G3SetModelPoints (gameData.models.polyModelPoints);
	G3DrawPolyModelShadow (objP, po->modelData, animAngles, nModel);
	return;
	}
nTextures = LoadModelTextures (po, altTextures);
gameStates.ogl.bUseTransform = 1;
G3SetModelPoints (gameData.models.polyModelPoints);
#ifdef _3DFX
_3dfx_rendering_poly_obj = 1;
#endif
PA_DFX (bSaveLight = gameStates.render.nLighting);
PA_DFX (gameStates.render.nLighting = 0);

gameData.render.pVerts = gameData.models.fPolyModelVerts;
G3StartInstanceMatrix (pos, orient);
if (!flags)	{	//draw entire tObject
	if (!G3RenderModel (objP, nModel, po, gameData.models.textures, animAngles, light, glowValues, color)) {
		G3DoneInstance ();
		gameStates.ogl.bUseTransform = !(SHOW_DYN_LIGHT && gameOpts->ogl.bLightObjects);
		G3StartInstanceMatrix (pos, orient);
		G3DrawPolyModel (objP, po->modelData, gameData.models.textures, animAngles, NULL, light, glowValues, color, NULL, nModel);
		}
	}
else {
	int i;

	for (i = 0; flags; flags >>= 1, i++)
		if (flags & 1) {
			vmsVector vOffs;

			//Assert (i < po->nModels);
			if (i < po->nModels) {
			//if submodel, rotate around its center point, not pivot point
				VmVecAvg (&vOffs, po->subModels.mins + i, po->subModels.maxs + i);
				VmVecNegate (&vOffs);
				G3StartInstanceMatrix (&vOffs, NULL);
				G3DrawPolyModel (objP, po->modelData + po->subModels.ptrs [i], gameData.models.textures, 
									  animAngles, NULL, light, glowValues, color, NULL, nModel);
				G3DoneInstance ();
				}
			}	
	}
G3DoneInstance ();
gameStates.ogl.bUseTransform = 0;
gameData.render.pVerts = NULL;
#if 0
{
	g3sPoint p0, p1;

G3TransformPoint (&p0.p3_vec, &objP->position.vPos);
VmVecSub (&p1.p3_vec, &objP->position.vPos, &objP->mType.physInfo.velocity);
G3TransformPoint (&p1.p3_vec, &p1.p3_vec);
glLineWidth (20);
glDisable (GL_TEXTURE_2D);
glBegin (GL_LINES);
glColor4d (1.0, 0.5, 0.0, 0.3);
OglVertex3x (p0.p3_vec.p.x, p0.p3_vec.p.y, p0.p3_vec.p.z);
glColor4d (1.0, 0.5, 0.0, 0.1);
OglVertex3x (p1.p3_vec.p.x, p1.p3_vec.p.y, p1.p3_vec.p.z);
glEnd ();
glLineWidth (1);
}
#endif
}

//------------------------------------------------------------------------------

void PolyObjFindMinMax (tPolyModel *pm)
{
	ushort nverts;
	vmsVector *vp;
	ushort *data, nType;
	int m;
	vmsVector *big_mn, *big_mx;
	
	big_mn = &pm->mins;
	big_mx = &pm->maxs;

	for (m = 0; m < pm->nModels; m++) {
		vmsVector *mn, *mx, *ofs;

		mn = pm->subModels.mins + m;
		mx = pm->subModels.maxs + m;
		ofs= pm->subModels.offsets + m;
		data = (ushort *) (pm->modelData + pm->subModels.ptrs [m]);
		nType = *data++;
		Assert (nType == 7 || nType == 1);
		nverts = *data++;
		if (nType==7)
			data+=2;		//skip start & pad
		vp = (vmsVector *) data;
		*mn = *mx = *vp++; 
		nverts--;
		if (m == 0)
			*big_mn = *big_mx = *mn;
		while (nverts--) {
			if (vp->p.x > mx->p.x) mx->p.x = vp->p.x;
			if (vp->p.y > mx->p.y) mx->p.y = vp->p.y;
			if (vp->p.z > mx->p.z) mx->p.z = vp->p.z;
			if (vp->p.x < mn->p.x) mn->p.x = vp->p.x;
			if (vp->p.y < mn->p.y) mn->p.y = vp->p.y;
			if (vp->p.z < mn->p.z) mn->p.z = vp->p.z;
			if (vp->p.x + ofs->p.x > big_mx->p.x) big_mx->p.x = vp->p.x + ofs->p.x;
			if (vp->p.y + ofs->p.y > big_mx->p.y) big_mx->p.y = vp->p.y + ofs->p.y;
			if (vp->p.z + ofs->p.z > big_mx->p.z) big_mx->p.z = vp->p.z + ofs->p.z;
			if (vp->p.x + ofs->p.x < big_mn->p.x) big_mn->p.x = vp->p.x + ofs->p.x;
			if (vp->p.y + ofs->p.y < big_mn->p.y) big_mn->p.y = vp->p.y + ofs->p.y;
			if (vp->p.z + ofs->p.z < big_mn->p.z) big_mn->p.z = vp->p.z + ofs->p.z;
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
int LoadPolygonModel (char *filename, int nTextures, int nFirstTexture, tRobotInfo *r)
#else
int LoadPolygonModel (char *filename, int nTextures, grsBitmap ***textures)
#endif
{
	#ifdef DRIVE
	#define r NULL
	#endif

	Assert (gameData.models.nPolyModels < MAX_POLYGON_MODELS);
	Assert (nTextures < MAX_POLYOBJ_TEXTURES);

	//	MK was real tired of those useless, slow mprintfs...
#if TRACE	
	if (gameData.models.nPolyModels > MAX_POLYGON_MODELS - 10)
		con_printf (CON_VERBOSE, "Used %d/%d polygon model slots\n", gameData.models.nPolyModels+1, MAX_POLYGON_MODELS);
#endif
	Assert (strlen (filename) <= 12);
	strcpy (Pof_names [gameData.models.nPolyModels], filename);
	ReadModelFile (gameData.models.polyModels+gameData.models.nPolyModels, filename, r);
	PolyObjFindMinMax (gameData.models.polyModels + gameData.models.nPolyModels);
	G3InitPolyModel (gameData.models.polyModels + gameData.models.nPolyModels, gameData.models.nPolyModels);
	if (nHighestTexture + 1 != nTextures)
		Error ("Model <%s> references %d textures but specifies %d.", filename, nHighestTexture+1, nTextures);
	gameData.models.polyModels [gameData.models.nPolyModels].nTextures = nTextures;
	gameData.models.polyModels [gameData.models.nPolyModels].nFirstTexture = nFirstTexture;
	gameData.models.polyModels [gameData.models.nPolyModels].nSimplerModel = 0;
//	Assert (polygon_models [gameData.models.nPolyModels]!=NULL);
	gameData.models.nPolyModels++;
	return gameData.models.nPolyModels-1;
}

//------------------------------------------------------------------------------

void _CDECL_ FreePolygonModels (void)
{
	int i;

LogErr ("unloading poly models\n");
for (i = 0; i < gameData.models.nPolyModels; i++) {
	FreeModel (gameData.models.polyModels + i);
	FreeModel (gameData.models.defPolyModels + i);
	}
for (i = 0; i < MAX_POLYGON_MODELS; i++)
	FreeModel (gameData.models.altPolyModels + i);
}

//------------------------------------------------------------------------------

void InitPolygonModels ()
{
gameData.models.nPolyModels = 0;
atexit (FreePolygonModels);
}

//------------------------------------------------------------------------------
//compare against this size when figuring how far to place eye for picture
//draws the given model in the current canvas.  The distance is set to
//more-or-less fill the canvas.  Note that this routine actually renders
//into an off-screen canvas that it creates, then copies to the current
//canvas.

void DrawModelPicture (int nModel, vmsAngVec *orient_angles)
{
	vmsVector	p = ZERO_VECTOR;
	vmsMatrix	o = IDENTITY_MATRIX;

Assert ((nModel >= 0) && (nModel < gameData.models.nPolyModels));
G3StartFrame (0, 0);
glDisable (GL_BLEND);
G3SetViewMatrix (&p, &o, gameStates.render.xZoom);
if (gameData.models.polyModels [nModel].rad != 0)
	p.p.z = FixMulDiv (DEFAULT_VIEW_DIST, gameData.models.polyModels [nModel].rad, BASE_MODEL_SIZE);
else
	p.p.z = DEFAULT_VIEW_DIST;
VmAngles2Matrix (&o, orient_angles);
DrawPolygonModel (NULL, &p, &o, NULL, nModel, 0, f1_0, NULL, NULL, NULL);
G3EndFrame ();
if (curDrawBuffer != GL_BACK)
	GrUpdate (0);
}

//------------------------------------------------------------------------------

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads a tPolyModel structure from a CFILE
 */
int PolyModelRead (tPolyModel *pm, CFILE *fp, int bHMEL)
{
	int	i;

if (bHMEL) {
	char	szId [4];
	int	nElement, nBlocks;

	CFRead (szId, sizeof (szId), 1, fp);
	if (strnicmp (szId, "HMEL", 4))
		return 0;
	if (CFReadInt (fp) != 1)
		return 0;
	nElement = CFReadInt (fp);
	nBlocks = CFReadInt (fp);
	pm->nModels = 1;
	}
else
	pm->nModels = CFReadInt (fp);
pm->nDataSize = CFReadInt (fp);
CFReadInt (fp);
pm->modelData = NULL;
for (i = 0; i < MAX_SUBMODELS; i++)
	pm->subModels.ptrs [i] = CFReadInt (fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	CFReadVector (& (pm->subModels.offsets [i]), fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	CFReadVector (& (pm->subModels.norms [i]), fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	CFReadVector (& (pm->subModels.pnts [i]), fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	pm->subModels.rads [i] = CFReadFix (fp);
CFRead (pm->subModels.parents, MAX_SUBMODELS, 1, fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	CFReadVector (& (pm->subModels.mins [i]), fp);
for (i = 0; i < MAX_SUBMODELS; i++)
	CFReadVector (& (pm->subModels.maxs [i]), fp);
CFReadVector (& (pm->mins), fp);
CFReadVector (& (pm->maxs), fp);
pm->rad = CFReadFix (fp);
pm->nTextures = CFReadByte (fp);
pm->nFirstTexture = CFReadShort (fp);
pm->nSimplerModel = CFReadByte (fp);
return 1;
}

//------------------------------------------------------------------------------
/*
 * reads n tPolyModel structs from a CFILE
 */
int PolyModelReadN (tPolyModel *pm, int n, CFILE *fp)
{
	int i;

for (i = n; i; i--, pm++)
	if (!PolyModelRead (pm, fp, 0))
		break;
return i;
}
#endif

//------------------------------------------------------------------------------
/*
 * routine which allocates, reads, and inits a tPolyModel's modelData
 */
void PolyModelDataRead (tPolyModel *pm, int nModel, tPolyModel *pdm, CFILE *fp)
{
if (pm->modelData)
	D2_FREE (pm->modelData);
pm->modelData = D2_ALLOC (pm->nDataSize);
Assert (pm->modelData != NULL);
CFRead (pm->modelData, sizeof (ubyte), pm->nDataSize, fp);
if (pdm) {
	if (pdm->modelData)
		D2_FREE (pdm->modelData);
	pdm->modelData = D2_ALLOC (pm->nDataSize);
	Assert (pdm->modelData != NULL);
	memcpy (pdm->modelData, pm->modelData, pm->nDataSize);
	}
#ifdef WORDS_NEED_ALIGNMENT
AlignPolyModelData (pm);
#endif
G3CheckAndSwap (pm->modelData);
//verify (pm->modelData);
G3InitPolyModel (pm, nModel);
}

//------------------------------------------------------------------------------
//eof
