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
#include <stdlib.h>
#include <string.h>

#include "inferno.h"
#include "interp.h"
#include "error.h"
#include "u_mem.h"
#include "args.h"
#include "gamepal.h"
#include "network.h"
#include "strutil.h"
#include "hiresmodels.h"
#include "texmap.h"
#include "textures.h"
#include "light.h"
#include "dynlight.h"
#include "buildmodel.h"

#ifdef _3DFX
#include "3dfx_des.h"
#endif

#define PM_COMPATIBLE_VERSION 6
#define PM_OBJFILE_VERSION 8

#define BASE_MODEL_SIZE 0x28000
#define DEFAULT_VIEW_DIST 0x60000

#define ID_OHDR 0x5244484f // 'RDHO'  //Object header
#define ID_SOBJ 0x4a424f53 // 'JBOS'  //Subobject header
#define ID_GUNS 0x534e5547 // 'SNUG'  //List of guns on this CObject
#define ID_ANIM 0x4d494e41 // 'MINA'  //Animation data
#define ID_IDTA 0x41544449 // 'ATDI'  //Interpreter data
#define ID_TXTR 0x52545854 // 'RTXT'  //Texture filename list

#define	MODEL_BUF_SIZE	32768

#define POF_CFSeek(_buf, _len, Type) _POF_CFSeek ((_len), (Type))
#define POF_ReadIntNew(i, f) pof_read (& (i), sizeof (i), 1, (f))

int	Pof_file_end;
int	Pof_addr;

void _POF_CFSeek (int len, int nType)
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
		return;
}

int pof_read_int (ubyte *bufp)
{
	int i;

	i = * (reinterpret_cast<int*> (&bufp [Pof_addr]));
	Pof_addr += 4;
	return INTEL_INT (i);

//	if (cf.Read (&i, sizeof (i), 1, f) != 1)
//		Error ("Unexpected end-of-file while reading CObject");
//
//	return i;
}

size_t pof_read (void *dst, size_t elsize, size_t nelem, ubyte *bufp)
{
if (nelem*elsize + (size_t) Pof_addr > (size_t) Pof_file_end)
	return 0;
memcpy (dst, &bufp [Pof_addr], elsize*nelem);
Pof_addr += (int) (elsize * nelem);
if (Pof_addr > MODEL_BUF_SIZE)
	return 0;
return nelem;
}

short pof_read_short (ubyte *bufp)
{
	short s;

	s = * (reinterpret_cast<short*> (&bufp [Pof_addr]));
	Pof_addr += 2;
	return INTEL_SHORT (s);
//	if (cf.Read (&s, sizeof (s), 1, f) != 1)
//		Error ("Unexpected end-of-file while reading CObject");
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
//	cf.Read (vecs, sizeof (vmsVector), n, f);

memcpy (vecs, &bufp [Pof_addr], n*sizeof (*vecs));
Pof_addr += n*sizeof (*vecs);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
while (n > 0)
	VmsVectorSwap (vecs [--n]);
#endif
}

void pof_read_angs (vmsAngVec *angs, int n, ubyte *bufp)
{
memcpy (angs, &bufp [Pof_addr], n*sizeof (*angs));
Pof_addr += n*sizeof (*angs);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
while (n > 0)
	VmsAngVecSwap (angs [--n]);
#endif
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
	return o.old_base + INTEL_SHORT (*reinterpret_cast<short*> (o.old_base + o.offset));
}

//------------------------------------------------------------------------------

ubyte * new_dest (chunk o) // return where chunk is (in aligned struct)
{
	return o.new_base + INTEL_SHORT (*reinterpret_cast<short*> (o.old_base + o.offset)) + o.correction;
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
	ubyte *tmp = new ubyte [tmp_size]; // where we build the aligned version of pm->modelData

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
		* (reinterpret_cast<short*> (cur_ch.new_base + cur_ch.offset))
		  = INTEL_SHORT (cur_ch.correction)
				+ INTEL_SHORT (* (reinterpret_cast<short*> (cur_ch.old_base + cur_ch.offset)));
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
	pm->modelData.Destroy ();
	pm->nDataSize += total_correction;
	if (!pm->modelData.Create (pm->nDataSize))
		Error ("Not enough memory for game models.");
	pm->modelData = tmp;
	delete[] tmp;
}
#endif //def WORDS_NEED_ALIGNMENT

//------------------------------------------------------------------------------
//reads a binary file containing a 3d model
tPolyModel *ReadModelFile (tPolyModel *pm, const char *filename, tRobotInfo *r)
{
	CFile cf;
	short version;
	int	id, len, next_chunk;
	int	animFlag = 0;
	ubyte modelBuf [MODEL_BUF_SIZE];

if (!cf.Open (filename, gameFolders.szDataDir, "rb", 0))
	Error ("Can't open file <%s>", filename);
Assert (cf.Length () <= MODEL_BUF_SIZE);
Pof_addr = 0;
Pof_file_end = (int) cf.Read (modelBuf, 1, cf.Length ());
cf.Close ();
id = pof_read_int (modelBuf);
if (id!=0x4f505350) /* 'OPSP' */
	Error ("Bad ID in model file <%s>", filename);
version = pof_read_short (modelBuf);
if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
	Error ("Bad version (%d) in model file <%s>", version, filename);
//if (FindArg ("-bspgen"))
//printf ("bspgen -c1");
while (POF_ReadIntNew (id, modelBuf) == 1) {
	id = INTEL_INT (id);
	//id  = pof_read_int (modelBuf);
	len = pof_read_int (modelBuf);
	next_chunk = Pof_addr + len;
	switch (id) {
		case ID_OHDR: {		//Object header
			vmsVector pmmin, pmmax;
			pm->nModels = pof_read_int (modelBuf);
			pm->rad = pof_read_int (modelBuf);
			Assert (pm->nModels <= MAX_SUBMODELS);
			pof_read_vecs (&pmmin, 1, modelBuf);
			pof_read_vecs (&pmmax, 1, modelBuf);
			if (FindArg ("-bspgen")) {
				vmsVector v = pmmax - pmmin;
				fix l = v[X];
				if (v[Y] > l)
					l = v[Y];
				if (v[Z] > l)
					l = v[Z];
				//printf (" -l%.3f", X2F (l));
				}
			break;
			}

		case ID_SOBJ: {		//Subobject header
			int n = pof_read_short (modelBuf);
			Assert (n < MAX_SUBMODELS);
			animFlag++;
			pm->subModels.parents [n] = (char) pof_read_short (modelBuf);
			pof_read_vecs (&pm->subModels.norms [n], 1, modelBuf);
			pof_read_vecs (&pm->subModels.pnts [n], 1, modelBuf);
			pof_read_vecs (&pm->subModels.offsets [n], 1, modelBuf);
			pm->subModels.rads [n] = pof_read_int (modelBuf);		//radius
			pm->subModels.ptrs [n] = pof_read_int (modelBuf);	//offset
			break;
			}

#ifndef DRIVE
		case ID_GUNS: {		//List of guns on this CObject
			if (r) {
				int i;
				vmsVector gun_dir;
				ubyte gun_used [MAX_GUNS];
				r->nGuns = pof_read_int (modelBuf);
				if (r->nGuns)
					animFlag++;
				Assert (r->nGuns <= MAX_GUNS);
				for (i = 0; i < r->nGuns; i++)
					gun_used [i] = 0;
				for (i = 0; i < r->nGuns; i++) {
					int id = pof_read_short (modelBuf);
					Assert (id < r->nGuns);
					Assert (gun_used [id] == 0);
					gun_used [id] = 1;
					r->gunSubModels [id] = (char) pof_read_short (modelBuf);
					Assert (r->gunSubModels [id] != 0xff);
					pof_read_vecs (&r->gunPoints [id], 1, modelBuf);
					if (version >= 7)
						pof_read_vecs (&gun_dir, 1, modelBuf);
					}
				}
			else
				POF_CFSeek (modelBuf, len, SEEK_CUR);
			break;
			}

		case ID_ANIM:		//Animation data
			animFlag++;
			if (r) {
				int f, m, n_frames = pof_read_short (modelBuf);
				Assert (n_frames == N_ANIM_STATES);
				for (m = 0; m <pm->nModels; m++)
					for (f = 0; f < n_frames; f++)
						pof_read_angs (&anim_angs [f][m], 1, modelBuf);
							robot_set_angles (r, pm, anim_angs);
				}
			else
				POF_CFSeek (modelBuf, len, SEEK_CUR);
			break;
#endif

		case ID_TXTR: {		//Texture filename list
			char name_buf [128];
			int n = pof_read_short (modelBuf);
			while (n--)
				pof_read_string (name_buf, 128, modelBuf);
			break;
			}

		case ID_IDTA:		//Interpreter data
			if (!pm->modelData.Create (len))
				Error ("Not enough memory for game models.");
			pm->nDataSize = len;
			pof_read (pm->modelData.Buffer (), 1, len, modelBuf);
			break;

		default:
			POF_CFSeek (modelBuf, len, SEEK_CUR);
			break;
		}
	if (version >= 8)		// Version 8 needs 4-byte alignment!!!
		POF_CFSeek (modelBuf, next_chunk, SEEK_SET);
	}
//	for (i=0;i<pm->nModels;i++)
//		pm->subModels.ptrs [i] += (int) pm->modelData;
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
int ReadModelGuns (const char *filename, vmsVector *gunPoints, vmsVector *gun_dirs, int *gunSubModels)
{
	CFile cf;
	short version;
	int	id, len;
	int	nGuns=0;
	ubyte	modelBuf [MODEL_BUF_SIZE];

if (!cf.Open (filename, gameFolders.szDataDir, "rb", 0))
	Error ("Can't open file <%s>", filename);
Assert (cf.Length () <= MODEL_BUF_SIZE);
Pof_addr = 0;
Pof_file_end = (int) cf.Read (modelBuf, 1, cf.Length ());
cf.Close ();
id = pof_read_int (modelBuf);
if (id!=0x4f505350) /* 'OPSP' */
	Error ("Bad ID in model file <%s>", filename);
version = pof_read_short (modelBuf);
Assert (version >= 7);		//must be 7 or higher for this data
if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
	Error ("Bad version (%d) in model file <%s>", version, filename);
while (POF_ReadIntNew (id, modelBuf) == 1) {
	id = INTEL_INT (id);
	//id  = pof_read_int (modelBuf);
	len = pof_read_int (modelBuf);
	if (id == ID_GUNS) {		//List of guns on this CObject
		nGuns = pof_read_int (modelBuf);
		for (int i = 0; i < nGuns; i++) {
			int id = pof_read_short (modelBuf);
			int sm = pof_read_short (modelBuf);
			if (gunSubModels)
				gunSubModels [id] = sm;
			else if (sm!=0)
				Error ("Invalid gun submodel in file <%s>", filename);
			pof_read_vecs (&gunPoints [id], 1, modelBuf);

			pof_read_vecs (&gun_dirs [id], 1, modelBuf);
			}
		}
		else
			POF_CFSeek (modelBuf, len, SEEK_CUR);
	}
return nGuns;
}

//------------------------------------------------------------------------------
//D2_FREE up a model, getting rid of all its memory
void FreeModel (tPolyModel *po)
{
po->modelData.Destroy ();
}

//------------------------------------------------------------------------------

int ObjectHasShadow (CObject *objP)
{
if (objP->info.nType == OBJ_ROBOT) {
	if (!gameOpts->render.shadows.bRobots)
		return 0;
	if (objP->cType.aiInfo.CLOAKED)
		return 0;
	}
else if (objP->info.nType == OBJ_WEAPON) {
	if (!gameOpts->render.shadows.bMissiles)
		return 0;
	if (!gameData.objs.bIsMissile [objP->info.nId] && (objP->info.nId != SMALLMINE_ID))
		return 0;
	}
else if (objP->info.nType == OBJ_POWERUP) {
	if (!gameOpts->render.shadows.bPowerups)
		return 0;
	}
else if (objP->info.nType == OBJ_PLAYER) {
	if (!gameOpts->render.shadows.bPlayers)
		return 0;
	if (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_CLOAKED)
		return 0;
	}
else if (objP->info.nType == OBJ_REACTOR) {
	if (!gameOpts->render.shadows.bReactors)
		return 0;
	}
else
	return 0;
return 1;
}

//------------------------------------------------------------------------------

tPolyModel *GetPolyModel (CObject *objP, vmsVector *pos, int nModel, int flags)
{
	tPolyModel	*po = NULL;
	int			bHaveAltModel, bIsDefModel;

if (gameStates.app.bEndLevelSequence && 
	 ((nModel == gameData.endLevel.exit.nModel) || (nModel == gameData.endLevel.exit.nDestroyedModel))) {
	bHaveAltModel = 0;
	bIsDefModel = 1;
	}
else {
	bHaveAltModel = gameData.models.altPolyModels [nModel].modelData.Buffer () != NULL;
	bIsDefModel = IsDefaultModel (nModel);
	}
#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
if ((nModel >= gameData.models.nPolyModels) && !(po = gameData.models.modelToPOL [nModel]))
	return NULL;
// only render shadows for custom models and for standard models with a shadow proof alternative model
if (!objP)
	po = ((gameStates.app.bAltModels && bIsDefModel && bHaveAltModel) ? gameData.models.altPolyModels : gameData.models.polyModels) + nModel;
else if (!po) {
	if (!(bIsDefModel && bHaveAltModel)) {
		if (gameStates.app.bFixModels && (objP->info.nType == OBJ_ROBOT) && (gameStates.render.nShadowPass == 2))
			return NULL;
		po = gameData.models.polyModels + nModel;
		}
	else if (gameStates.render.nShadowPass != 2) {
		if ((gameStates.app.bAltModels || (objP->info.nType == OBJ_PLAYER)) && bHaveAltModel)
			po = gameData.models.altPolyModels + nModel;
		else
			po = gameData.models.polyModels + nModel;
		}
	else if (bHaveAltModel)
		po = gameData.models.altPolyModels + nModel;
	else
		return NULL;
	if ((gameStates.render.nShadowPass == 2) && (objP->info.nType == OBJ_REACTOR) && !(nModel & 1))	// use the working reactor model for rendering destroyed reactors' shadows
		po--;
	}
//check if should use simple model (depending on detail level chosen)
if (!(SHOW_DYN_LIGHT || SHOW_SHADOWS) && po->nSimplerModel && !flags && pos) {
	int	cnt = 1;
	fix depth = G3CalcPointDepth (*pos);		//gets 3d depth
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
      gameData.models.textures [i]->nId = gameData.models.textureIndex [i].index;
#endif
		}
	}
else {
	for (i = 0, j = po->nFirstTexture; i < nTextures; i++, j++) {
		gameData.models.textureIndex [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [j]];
		gameData.models.textures [i] = gameData.pig.tex.bitmaps [gameStates.app.bD1Model] + gameData.models.textureIndex [i].index;
#ifdef _3DFX
      gameData.models.textures [i]->nId = gameData.models.textureIndex [i].index;
#endif
		}
	}
#ifdef PIGGY_USE_PAGING
// Make sure the textures for this CObject are paged in...
gameData.pig.tex.bPageFlushed = 0;
for (i = 0; i < nTextures; i++)
	PIGGY_PAGE_IN (gameData.models.textureIndex [i].index, gameStates.app.bD1Model);
// Hmmm... cache got flushed in the middle of paging all these in,
// so we need to reread them all in.
if (gameData.pig.tex.bPageFlushed)	{
	gameData.pig.tex.bPageFlushed = 0;
	for (i = 0; i < nTextures; i++)
		PIGGY_PAGE_IN (gameData.models.textureIndex [i].index, gameStates.app.bD1Model);
}
// Make sure that they can all fit in memory.
Assert (gameData.pig.tex.bPageFlushed == 0);
#endif
return nTextures;
}

//------------------------------------------------------------------------------

//draw a polygon model
int DrawPolygonModel (
	CObject			*objP,
	vmsVector		*pos,
	vmsMatrix		*orient,
	vmsAngVec		*animAngles,
	int				nModel,
	int				flags,
	fix				light,
	fix				*glowValues,
	tBitmapIndex	altTextures [],
	tRgbaColorf		*colorP)
{
	tPolyModel	*po;
	int			nTextures, bHires = 0;

if ((gameStates.render.nShadowPass == 2) && !ObjectHasShadow (objP))
	return 1;
if (!(po = GetPolyModel (objP, pos, nModel, flags))) {
	if (!flags && (gameStates.render.nShadowPass != 2) && HaveHiresModel (nModel))
		bHires = 1;
	else
		return gameStates.render.nShadowPass == 2;
	}
if (gameStates.render.nShadowPass == 2) {
	if (!bHires) {
		G3SetModelPoints (gameData.models.polyModelPoints);
		G3DrawPolyModelShadow (objP, po->modelData.Buffer (), animAngles, nModel);
		}
	return 1;
	}
#if 1//def _DEBUG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
nTextures = bHires ? 0 : LoadModelTextures (po, altTextures);
gameStates.ogl.bUseTransform = 1;
G3SetModelPoints (gameData.models.polyModelPoints);
gameData.render.vertP = gameData.models.fPolyModelVerts;
if (!flags)	{	//draw entire CObject
	if (!G3RenderModel (objP, nModel, -1, po, gameData.models.textures, animAngles, NULL, light, glowValues, colorP)) {
		if (bHires)
			return 0;
#if 0//def _DEBUG
		if (objP && (objP->info.nType == OBJ_ROBOT))
			G3RenderModel (objP, nModel, -1, po, gameData.models.textures, animAngles, NULL, light, glowValues, colorP);
#endif
		if (objP && (objP->info.nType == OBJ_POWERUP)) {
			if ((objP->info.nId == POW_SMARTMINE) || (objP->info.nId == POW_PROXMINE))
				gameData.models.vScale.Set (2 * F1_0, 2 * F1_0, 2 * F1_0);
			else
				gameData.models.vScale.Set (3 * F1_0 / 2, 3 * F1_0 / 2, 3 * F1_0 / 2);
			}
		gameStates.ogl.bUseTransform = 
			(gameStates.app.bEndLevelSequence < EL_OUTSIDE) && 
			!(SHOW_DYN_LIGHT && ((RENDERPATH && gameOpts->ogl.bObjLighting) || gameOpts->ogl.bLightObjects));
		G3StartInstanceMatrix (*pos, *orient);
		G3DrawPolyModel (objP, po->modelData.Buffer (), gameData.models.textures, animAngles, NULL, light, glowValues, colorP, NULL, nModel);
		G3DoneInstance ();
		}
	}
else {
	int i;

	//G3StartInstanceMatrix (pos, orient);
	for (i = 0; flags > 0; flags >>= 1, i++)
		if (flags & 1) {
			vmsVector vOffset;

			//Assert (i < po->nModels);
			if (i < po->nModels) {
			//if submodel, rotate around its center point, not pivot point
				vOffset = vmsVector::Avg(po->subModels.mins[i], po->subModels.maxs[i]);
				vOffset.Neg();
				if (!G3RenderModel (objP, nModel, i, po, gameData.models.textures, animAngles, &vOffset, light, glowValues, colorP)) {
					if (bHires)
						return 0;
#if DBG
					G3RenderModel (objP, nModel, i, po, gameData.models.textures, animAngles, &vOffset, light, glowValues, colorP);
#endif
					G3StartInstanceMatrix(vOffset);
					G3DrawPolyModel (objP, po->modelData + po->subModels.ptrs [i], gameData.models.textures,
										  animAngles, NULL, light, glowValues, colorP, NULL, nModel);
					G3DoneInstance ();
					}
				}
			}
	//G3DoneInstance ();
	}
gameStates.ogl.bUseTransform = 0;
gameData.render.vertP = NULL;
#if 0
{
	g3sPoint p0, p1;

G3TransformPoint (&p0.p3_vec, &objP->info.position.vPos);
VmVecSub (&p1.p3_vec, &objP->info.position.vPos, &objP->mType.physInfo.velocity);
G3TransformPoint (&p1.p3_vec, &p1.p3_vec);
glLineWidth (20);
glDisable (GL_TEXTURE_2D);
glBegin (GL_LINES);
glColor4d (1.0, 0.5, 0.0, 0.3);
OglVertex3x (p0.p3_vec[X], p0.p3_vec[Y], p0.p3_vec[Z]);
glColor4d (1.0, 0.5, 0.0, 0.1);
OglVertex3x (p1.p3_vec[X], p1.p3_vec[Y], p1.p3_vec[Z]);
glEnd ();
glLineWidth (1);
}
#endif
return 1;
}

//------------------------------------------------------------------------------

void PolyObjFindMinMax (tPolyModel *pm)
{
	ushort nverts;
	vmsVector *vp;
	ushort *data, nType;
	int m;

	vmsVector& big_mn = pm->mins;
	vmsVector& big_mx = pm->maxs;

	for (m = 0; m < pm->nModels; m++) {

		vmsVector& mn = pm->subModels.mins[m];
		vmsVector& mx = pm->subModels.maxs[m];
		vmsVector& ofs= pm->subModels.offsets[m];
		data = reinterpret_cast<ushort*> (pm->modelData + pm->subModels.ptrs [m]);
		nType = *data++;
		Assert (nType == 7 || nType == 1);
		nverts = *data++;
		if (nType==7)
			data+=2;		//skip start & pad
		vp = reinterpret_cast<vmsVector*> (data);
		mn = mx = *vp++;
		nverts--;
		if (m == 0)
			big_mn = big_mx = mn;
		while (nverts--) {
			if ((*vp)[X] > mx[X]) mx[X] = (*vp)[X];
			if ((*vp)[Y] > mx[Y]) mx[Y] = (*vp)[Y];
			if ((*vp)[Z] > mx[Z]) mx[Z] = (*vp)[Z];
			if ((*vp)[X] < mn[X]) mn[X] = (*vp)[X];
			if ((*vp)[Y] < mn[Y]) mn[Y] = (*vp)[Y];
			if ((*vp)[Z] < mn[Z]) mn[Z] = (*vp)[Z];
			if ((*vp)[X] + ofs[X] > big_mx[X]) big_mx[X] = (*vp)[X] + ofs[X];
			if ((*vp)[Y] + ofs[Y] > big_mx[Y]) big_mx[Y] = (*vp)[Y] + ofs[Y];
			if ((*vp)[Z] + ofs[Z] > big_mx[Z]) big_mx[Z] = (*vp)[Z] + ofs[Z];
			if ((*vp)[X] + ofs[X] < big_mn[X]) big_mn[X] = (*vp)[X] + ofs[X];
			if ((*vp)[Y] + ofs[Y] < big_mn[Y]) big_mn[Y] = (*vp)[Y] + ofs[Y];
			if ((*vp)[Z] + ofs[Z] < big_mn[Z]) big_mn[Z] = (*vp)[Z] + ofs[Z];
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
int LoadPolygonModel (const char *filename, int nTextures, int nFirstTexture, tRobotInfo *r)
#else
int LoadPolygonModel (const char *filename, int nTextures, CBitmap ***textures)
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
	ReadModelFile (gameData.models.polyModels + gameData.models.nPolyModels, filename, r);
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

PrintLog ("unloading poly models\n");
for (i = 0; i < MAX_POLYGON_MODELS; i++) {
	FreeModel (gameData.models.polyModels + i);
	FreeModel (gameData.models.defPolyModels + i);
	FreeModel (gameData.models.altPolyModels + i);
	}
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

void DrawModelPicture (int nModel, vmsAngVec *orientAngles)
{
	vmsVector	p = vmsVector::ZERO;
	vmsMatrix	o = vmsMatrix::IDENTITY;

Assert ((nModel >= 0) && (nModel < gameData.models.nPolyModels));
G3StartFrame (0, 0);
glDisable (GL_BLEND);
G3SetViewMatrix (p, o, gameStates.render.xZoom, 1);
if (gameData.models.polyModels [nModel].rad != 0)
	p [Z] = FixMulDiv (DEFAULT_VIEW_DIST, gameData.models.polyModels [nModel].rad, BASE_MODEL_SIZE);
else
	p [Z] = DEFAULT_VIEW_DIST;
o = vmsMatrix::Create (*orientAngles);
DrawPolygonModel (NULL, &p, &o, NULL, nModel, 0, f1_0, NULL, NULL, NULL);
G3EndFrame ();
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	GrUpdate (0);
}

//------------------------------------------------------------------------------

/*
 * reads a tPolyModel structure from a CFile
 */
int ReadPolyModel (tPolyModel* pm, int bHMEL, CFile& cf)
{
	int	i;

if (bHMEL) {
	char	szId [4];
	int	nElement, nBlocks;

	cf.Read (szId, sizeof (szId), 1);
	if (strnicmp (szId, "HMEL", 4))
		return 0;
	if (cf.ReadInt () != 1)
		return 0;
	nElement = cf.ReadInt ();
	nBlocks = cf.ReadInt ();
	pm->nModels = 1;
	}
else
	pm->nModels = cf.ReadInt ();
pm->nDataSize = cf.ReadInt ();
cf.ReadInt ();
pm->modelData = NULL;
for (i = 0; i < MAX_SUBMODELS; i++)
	pm->subModels.ptrs [i] = cf.ReadInt ();
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (pm->subModels.offsets[i]);
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (pm->subModels.norms[i]);
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (pm->subModels.pnts[i]);
for (i = 0; i < MAX_SUBMODELS; i++)
	pm->subModels.rads [i] = cf.ReadFix ();
cf.Read (pm->subModels.parents, MAX_SUBMODELS, 1);
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (pm->subModels.mins[i]);
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (pm->subModels.maxs[i]);
cf.ReadVector (pm->mins);
cf.ReadVector (pm->maxs);
pm->rad = cf.ReadFix ();
pm->nTextures = cf.ReadByte ();
pm->nFirstTexture = cf.ReadShort ();
pm->nSimplerModel = cf.ReadByte ();
pm->nType = 0;
return 1;
}

//------------------------------------------------------------------------------
/*
 * reads n tPolyModel structs from a CFile
 */
int ReadPolyModels (tPolyModel *pm, int n, CFile& cf)
{
	int i;

for (i = 0; i < n; i++)
	if (!ReadPolyModel (pm + i, 0, cf))
		break;
return i;
}

//------------------------------------------------------------------------------
/*
 * routine which allocates, reads, and inits a tPolyModel's modelData
 */
void ReadPolyModelData (tPolyModel *pm, int nModel, tPolyModel *pdm, CFile& cf)
{
if (!pm->modelData.Create (pm->nDataSize))
	Error ("Not enough memory for game models.");
cf.Read (pm->modelData.Buffer (), sizeof (ubyte), pm->nDataSize);
if (pdm) {
	pdm->modelData.Destroy ();
	pdm->modelData = pm->modelData;
	if (!pdm->modelData.Buffer ())
		Error ("Not enough memory for game models.");
	}
#ifdef WORDS_NEED_ALIGNMENT
AlignPolyModelData (pm);
#endif
G3CheckAndSwap (pm->modelData.Buffer ());
//verify (pm->modelData);
G3InitPolyModel (pm, nModel);
}

//------------------------------------------------------------------------------
//eof
