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
#include "hitbox.h"

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

#define POF_CFSeek(_buf, _len, Type) POF_Seek ((_len), (Type))
#define POF_ReadIntNew(i, f) POF_Read (& (i), sizeof (i), 1, (f))

int	Pof_file_end;
int	Pof_addr;

//------------------------------------------------------------------------------

void POF_Seek (int len, int nType)
{
switch (nType) {
	case SEEK_SET:	
		Pof_addr = len;	
		break;
	case SEEK_CUR:	
		Pof_addr += len;	
		break;
	case SEEK_END:
		Assert (len <= 0);	//	seeking from end, better be moving back.
		Pof_addr = Pof_file_end + len;
		break;
	}
if (Pof_addr > MODEL_BUF_SIZE)
	return;
}

//------------------------------------------------------------------------------

int POF_ReadInt (ubyte *bufp)
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

//------------------------------------------------------------------------------

size_t POF_Read (void *dst, size_t elsize, size_t nelem, ubyte *bufp)
{
if (nelem*elsize + (size_t) Pof_addr > (size_t) Pof_file_end)
	return 0;
memcpy (dst, &bufp [Pof_addr], elsize*nelem);
Pof_addr += (int) (elsize * nelem);
if (Pof_addr > MODEL_BUF_SIZE)
	return 0;
return nelem;
}

//------------------------------------------------------------------------------

short POF_ReadShort (ubyte *bufp)
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

//------------------------------------------------------------------------------

void POF_ReadString (char *buf, int max_char, ubyte *bufp)
{
	int	i;

	for (i=0; i<max_char; i++) {
		if ((*buf++ = bufp [Pof_addr++]) == 0)
			break;
	}

//	while (max_char-- && (*buf=CFGetC (f)) != 0) buf++;

}

//------------------------------------------------------------------------------

void POF_ReadVecs (CFixVector *vecs, int n, ubyte *bufp)
{
//	cf.Read (vecs, sizeof (CFixVector), n, f);

memcpy (vecs, &bufp [Pof_addr], n * sizeof (*vecs));
Pof_addr += n * sizeof (*vecs);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
while (n > 0)
	VmsVectorSwap (vecs [--n]);
#endif
}

//------------------------------------------------------------------------------

void POF_ReadAngs (CAngleVector *angs, int n, ubyte *bufp)
{
memcpy (angs, &bufp [Pof_addr], n * sizeof (*angs));
Pof_addr += n * sizeof (*angs);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
while (n > 0)
	VmsAngVecSwap (angs [--n]);
#endif
}

#ifdef DRIVE
#define tRobotInfo void
#else
CAngleVector animAngles [N_ANIM_STATES][MAX_SUBMODELS];

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void SetRobotAngles (tRobotInfo *r, CPolyModel* modelP, CAngleVector angs [N_ANIM_STATES][MAX_SUBMODELS]);
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

void AlignPolyModelData (CPolyModel* modelP)
{
	int i, chunk_len;
	int total_correction = 0;
	ubyte *cur_old, *cur_new;
	chunk cur_ch;
	chunk ch_list [MAX_CHUNKS];
	int no_chunks = 0;
	int tmp_size = modelP->nDataSize + SHIFT_SPACE;
	ubyte *tmp = new ubyte [tmp_size]; // where we build the aligned version of modelP->Data ()

	Assert (tmp != NULL);
	//start with first chunk (is always aligned!)
	cur_old = modelP->Data ();
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
		//correct submodel_ptr's for modelP, too
		for (i = 0; i < MAX_SUBMODELS; i++)
			if (modelP->Data () + modelP->SubModels ().ptrs [i] >= cur_old
			    && modelP->Data () + modelP->SubModels ().ptrs [i] < cur_old + chunk_len)
				modelP->SubModels ().ptrs [i] += (cur_new - tmp) - (cur_old - modelP->Data ());
 	}
	modelP->Data ().Destroy ();
	modelP->nDataSize += total_correction;
	if (!modelP->Data ().Create (modelP->nDataSize))
		Error ("Not enough memory for game models.");
	modelP->Data () = tmp;
	delete [] tmp;
}
#endif //def WORDS_NEED_ALIGNMENT

//------------------------------------------------------------------------------
//reads a binary file containing a 3d model
void CPolyModel::Parse (const char *filename, tRobotInfo *botInfoP)
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
id = POF_ReadInt (modelBuf);
if (id != 0x4f505350) /* 'OPSP' */
	Error ("Bad ID in model file <%s>", filename);
version = POF_ReadShort (modelBuf);
if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
	Error ("Bad version (%d) in model file <%s>", version, filename);
//if (FindArg ("-bspgen"))
//printf ("bspgen -c1");
while (POF_ReadIntNew (id, modelBuf) == 1) {
	id = INTEL_INT (id);
	//id  = POF_ReadInt (modelBuf);
	len = POF_ReadInt (modelBuf);
	next_chunk = Pof_addr + len;
	switch (id) {
		case ID_OHDR: {		//Object header
			CFixVector pmmin, pmmax;
			m_info.nModels = POF_ReadInt (modelBuf);
			m_info.rad = POF_ReadInt (modelBuf);
			Assert (m_info.nModels <= MAX_SUBMODELS);
			POF_ReadVecs (&pmmin, 1, modelBuf);
			POF_ReadVecs (&pmmax, 1, modelBuf);
			if (FindArg ("-bspgen")) {
				CFixVector v = pmmax - pmmin;
				fix l = v [X];
				if (v [Y] > l)
					l = v [Y];
				if (v [Z] > l)
					l = v [Z];
				//printf (" -l%.3f", X2F (l));
				}
			break;
			}

		case ID_SOBJ: {		//Subobject header
			int n = POF_ReadShort (modelBuf);
			Assert (n < MAX_SUBMODELS);
			animFlag++;
			m_info.subModels.parents [n] = (char) POF_ReadShort (modelBuf);
			POF_ReadVecs (&m_info.subModels.norms [n], 1, modelBuf);
			POF_ReadVecs (&m_info.subModels.pnts [n], 1, modelBuf);
			POF_ReadVecs (&m_info.subModels.offsets [n], 1, modelBuf);
			m_info.subModels.rads [n] = POF_ReadInt (modelBuf);		//radius
			m_info.subModels.ptrs [n] = POF_ReadInt (modelBuf);	//offset
			break;
			}

		case ID_GUNS: {		//List of guns on this CObject
			if (botInfoP) {
				int i;
				CFixVector gun_dir;
				ubyte gun_used [MAX_GUNS];
				botInfoP->nGuns = POF_ReadInt (modelBuf);
				if (botInfoP->nGuns)
					animFlag++;
				Assert (botInfoP->nGuns <= MAX_GUNS);
				for (i = 0; i < botInfoP->nGuns; i++)
					gun_used [i] = 0;
				for (i = 0; i < botInfoP->nGuns; i++) {
					int id = POF_ReadShort (modelBuf);
					Assert (id < botInfoP->nGuns);
					Assert (gun_used [id] == 0);
					gun_used [id] = 1;
					botInfoP->gunSubModels [id] = (char) POF_ReadShort (modelBuf);
					Assert (botInfoP->gunSubModels [id] != 0xff);
					POF_ReadVecs (&botInfoP->gunPoints [id], 1, modelBuf);
					if (version >= 7)
						POF_ReadVecs (&gun_dir, 1, modelBuf);
					}
				}
			else
				POF_CFSeek (modelBuf, len, SEEK_CUR);
			break;
			}

		case ID_ANIM:		//Animation data
			animFlag++;
			if (botInfoP) {
				int f, m, n_frames = POF_ReadShort (modelBuf);
				Assert (n_frames == N_ANIM_STATES);
				for (m = 0; m <m_info.nModels; m++)
					for (f = 0; f < n_frames; f++)
						POF_ReadAngs (&animAngles [f][m], 1, modelBuf);
							SetRobotAngles (botInfoP, this, animAngles);
				}
			else
				POF_CFSeek (modelBuf, len, SEEK_CUR);
			break;

		case ID_TXTR: {		//Texture filename list
			char name_buf [128];
			int n = POF_ReadShort (modelBuf);
			while (n--)
				POF_ReadString (name_buf, 128, modelBuf);
			break;
			}

		case ID_IDTA:		//Interpreter data
			if (!m_info.data.Create (len))
				Error ("Not enough memory for game models.");
			m_info.nDataSize = len;
			POF_Read (m_info.data.Buffer (), 1, len, modelBuf);
			break;

		default:
			POF_CFSeek (modelBuf, len, SEEK_CUR);
			break;
		}
	if (version >= 8)		// Version 8 needs 4-byte alignment!!!
		POF_CFSeek (modelBuf, next_chunk, SEEK_SET);
	}
//	for (i=0;i<m_info.nModels;i++)
//		m_info.subModels.ptrs [i] += (int) m_info.data;
#ifdef WORDS_NEED_ALIGNMENT
G3AlignPolyModelData (modelP);
#endif
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
G3SwapPolyModelData (m_info.data);
#endif
	//verify (m_info.data);
}

//------------------------------------------------------------------------------
//reads the gun information for a model
//fills in arrays gunPoints & gunDirs, returns the number of guns read
int ReadModelGuns (const char *filename, CFixVector *gunPoints, CFixVector *gunDirs, int *gunSubModels)
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
id = POF_ReadInt (modelBuf);
if (id!=0x4f505350) /* 'OPSP' */
	Error ("Bad ID in model file <%s>", filename);
version = POF_ReadShort (modelBuf);
Assert (version >= 7);		//must be 7 or higher for this data
if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
	Error ("Bad version (%d) in model file <%s>", version, filename);
while (POF_ReadIntNew (id, modelBuf) == 1) {
	id = INTEL_INT (id);
	//id  = POF_ReadInt (modelBuf);
	len = POF_ReadInt (modelBuf);
	if (id == ID_GUNS) {		//List of guns on this CObject
		nGuns = POF_ReadInt (modelBuf);
		for (int i = 0; i < nGuns; i++) {
			int id = POF_ReadShort (modelBuf);
			int sm = POF_ReadShort (modelBuf);
			if (gunSubModels)
				gunSubModels [id] = sm;
			else if (sm!=0)
				Error ("Invalid gun submodel in file <%s>", filename);
			POF_ReadVecs (&gunPoints [id], 1, modelBuf);

			POF_ReadVecs (&gunDirs [id], 1, modelBuf);
			}
		}
		else
			POF_CFSeek (modelBuf, len, SEEK_CUR);
	}
return nGuns;
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

CPolyModel* GetPolyModel (CObject *objP, CFixVector *pos, int nModel, int flags)
{
	CPolyModel	*modelP = NULL;
	int			bHaveAltModel, bIsDefModel;

if (gameStates.app.bEndLevelSequence && 
	 ((nModel == gameData.endLevel.exit.nModel) || (nModel == gameData.endLevel.exit.nDestroyedModel))) {
	bHaveAltModel = 0;
	bIsDefModel = 1;
	}
else {
	bHaveAltModel = gameData.models.altPolyModels [nModel].Data ().Buffer () != NULL;
	bIsDefModel = IsDefaultModel (nModel);
	}
#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
if ((nModel >= gameData.models.nPolyModels) && !(modelP = gameData.models.modelToPOL [nModel]))
	return NULL;
// only render shadows for custom models and for standard models with a shadow proof alternative model
if (!objP)
	modelP = ((gameStates.app.bAltModels && bIsDefModel && bHaveAltModel) ? gameData.models.altPolyModels : gameData.models.polyModels) + nModel;
else if (!modelP) {
	if (!(bIsDefModel && bHaveAltModel)) {
		if (gameStates.app.bFixModels && (objP->info.nType == OBJ_ROBOT) && (gameStates.render.nShadowPass == 2))
			return NULL;
		modelP = gameData.models.polyModels + nModel;
		}
	else if (gameStates.render.nShadowPass != 2) {
		if ((gameStates.app.bAltModels || (objP->info.nType == OBJ_PLAYER)) && bHaveAltModel)
			modelP = gameData.models.altPolyModels + nModel;
		else
			modelP = gameData.models.polyModels + nModel;
		}
	else if (bHaveAltModel)
		modelP = gameData.models.altPolyModels + nModel;
	else
		return NULL;
	if ((gameStates.render.nShadowPass == 2) && (objP->info.nType == OBJ_REACTOR) && !(nModel & 1))	// use the working reactor model for rendering destroyed reactors' shadows
		modelP--;
	}
//check if should use simple model (depending on detail level chosen)
if (!(SHOW_DYN_LIGHT || SHOW_SHADOWS) && modelP->SimplerModel () && !flags && pos) {
	int	cnt = 1;
	fix depth = G3CalcPointDepth (*pos);		//gets 3d depth
	while (modelP->SimplerModel () && (depth > cnt++ * gameData.models.nSimpleModelThresholdScale * modelP->Rad ()))
		modelP = gameData.models.polyModels + modelP->SimplerModel () - 1;
	}
return modelP;
}

//------------------------------------------------------------------------------

//draw a polygon model
int DrawPolyModel (
	CObject*			objP,
	CFixVector*		pos,
	CFixMatrix*		orient,
	CAngleVector*	animAngles,
	int				nModel,
	int				flags,
	fix				light,
	fix*				glowValues,
	tBitmapIndex	altTextures [],
	tRgbaColorf*	colorP)
{
	CPolyModel*	modelP;
	int			nTextures, bHires = 0;

if ((gameStates.render.nShadowPass == 2) && !ObjectHasShadow (objP))
	return 1;
if (!(modelP = GetPolyModel (objP, pos, nModel, flags))) {
	if (!flags && (gameStates.render.nShadowPass != 2) && HaveHiresModel (nModel))
		bHires = 1;
	else
		return gameStates.render.nShadowPass == 2;
	}
if (gameStates.render.nShadowPass == 2) {
	if (!bHires) {
		G3SetModelPoints (gameData.models.polyModelPoints.Buffer ());
		G3DrawPolyModelShadow (objP, modelP->Data ().Buffer (), animAngles, nModel);
		}
	return 1;
	}
#if 1//def _DEBUG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
nTextures = bHires ? 0 : LoadModelTextures (modelP, altTextures);
gameStates.ogl.bUseTransform = 1;
G3SetModelPoints (gameData.models.polyModelPoints.Buffer ());
gameData.render.vertP = gameData.models.fPolyModelVerts.Buffer ();
if (!flags) {	//draw entire CObject
	if (!G3RenderModel (objP, nModel, -1, modelP, gameData.models.textures.Buffer (), animAngles, NULL, light, glowValues, colorP)) {
		if (bHires)
			return 0;
#if 0//def _DEBUG
		if (objP && (objP->info.nType == OBJ_ROBOT))
			G3RenderModel (objP, nModel, -1, modelP, gameData.models.textures.Buffer (), animAngles, NULL, light, glowValues, colorP);
#endif
		if (objP && (objP->info.nType == OBJ_POWERUP)) {
			if ((objP->info.nId == POW_SMARTMINE) || (objP->info.nId == POW_PROXMINE))
				gameData.models.vScale.Set (I2X (2), I2X (2), I2X (2));
			else
				gameData.models.vScale.Set (I2X (3) / 2, I2X (3) / 2, I2X (3) / 2);
			}
		gameStates.ogl.bUseTransform = 
			(gameStates.app.bEndLevelSequence < EL_OUTSIDE) && 
			!(SHOW_DYN_LIGHT && ((RENDERPATH && gameOpts->ogl.bObjLighting) || gameOpts->ogl.bLightObjects));
		transformation.Begin (*pos, *orient);
		G3DrawPolyModel (objP, modelP->Data ().Buffer (), gameData.models.textures.Buffer (), animAngles, NULL, light, glowValues, colorP, NULL, nModel);
		transformation.End ();
		}
	}
else {
	int i;

	//transformation.Begin (pos, orient);
	for (i = 0; flags > 0; flags >>= 1, i++)
		if (flags & 1) {
			CFixVector vOffset;

			//Assert (i < modelP->nModels);
			if (i < modelP->ModelCount ()) {
			//if submodel, rotate around its center point, not pivot point
				vOffset = CFixVector::Avg (modelP->SubModels ().mins [i], modelP->SubModels ().maxs [i]);
				vOffset.Neg();
				if (!G3RenderModel (objP, nModel, i, modelP, gameData.models.textures.Buffer (), animAngles, &vOffset, light, glowValues, colorP)) {
					if (bHires)
						return 0;
#if DBG
					G3RenderModel (objP, nModel, i, modelP, gameData.models.textures.Buffer (), animAngles, &vOffset, light, glowValues, colorP);
#endif
					transformation.Begin (vOffset);
					G3DrawPolyModel (objP, modelP->Data () + modelP->SubModels ().ptrs [i], gameData.models.textures.Buffer (),
										  animAngles, NULL, light, glowValues, colorP, NULL, nModel);
					transformation.End ();
					}
				}
			}
	//transformation.End ();
	}
gameStates.ogl.bUseTransform = 0;
gameData.render.vertP = NULL;
#if 0
{
	g3sPoint p0, p1;

transformation.Transform (&p0.p3_vec, &objP->info.position.vPos);
VmVecSub (&p1.p3_vec, &objP->info.position.vPos, &objP->mType.physInfo.velocity);
transformation.Transform (&p1.p3_vec, &p1.p3_vec);
glLineWidth (20);
glDisable (GL_TEXTURE_2D);
glBegin (GL_LINES);
glColor4d (1.0, 0.5, 0.0, 0.3);
OglVertex3x (p0.p3_vec [X], p0.p3_vec [Y], p0.p3_vec [Z]);
glColor4d (1.0, 0.5, 0.0, 0.1);
OglVertex3x (p1.p3_vec [X], p1.p3_vec [Y], p1.p3_vec [Z]);
glEnd ();
glLineWidth (1);
}
#endif
return 1;
}

//------------------------------------------------------------------------------

void CPolyModel::FindMinMax (void)
{
	ushort		nVerts;
	CFixVector*	vp;
	ushort*		dataP, nType;

CFixVector& big_mn = m_info.mins;
CFixVector& big_mx = m_info.maxs;

for (int i = 0; i < m_info.nModels; i++) {
	CFixVector& mn = m_info.subModels.mins [i];
	CFixVector& mx = m_info.subModels.maxs [i];
	CFixVector& ofs = m_info.subModels.offsets [i];
	dataP = reinterpret_cast<ushort*> (m_info.data + m_info.subModels.ptrs [i]);
	nType = *dataP++;
	Assert (nType == 7 || nType == 1);
	nVerts = *dataP++;
	if (nType==7)
		dataP+=2;		//skip start & pad
	vp = reinterpret_cast<CFixVector*> (dataP);
	mn = mx = *vp++;
	nVerts--;
	if (i == 0)
		big_mn = big_mx = mn;
	while (nVerts--) {
		if ((*vp) [X] > mx [X]) mx [X] = (*vp) [X];
		if ((*vp) [Y] > mx [Y]) mx [Y] = (*vp) [Y];
		if ((*vp) [Z] > mx [Z]) mx [Z] = (*vp) [Z];
		if ((*vp) [X] < mn [X]) mn [X] = (*vp) [X];
		if ((*vp) [Y] < mn [Y]) mn [Y] = (*vp) [Y];
		if ((*vp) [Z] < mn [Z]) mn [Z] = (*vp) [Z];
		if ((*vp) [X] + ofs [X] > big_mx [X]) big_mx [X] = (*vp) [X] + ofs [X];
		if ((*vp) [Y] + ofs [Y] > big_mx [Y]) big_mx [Y] = (*vp) [Y] + ofs [Y];
		if ((*vp) [Z] + ofs [Z] > big_mx [Z]) big_mx [Z] = (*vp) [Z] + ofs [Z];
		if ((*vp) [X] + ofs [X] < big_mn [X]) big_mn [X] = (*vp) [X] + ofs [X];
		if ((*vp) [Y] + ofs [Y] < big_mn [Y]) big_mn [Y] = (*vp) [Y] + ofs [Y];
		if ((*vp) [Z] + ofs [Z] < big_mn [Z]) big_mn [Z] = (*vp) [Z] + ofs [Z];
		vp++;
		}
	}
}

//------------------------------------------------------------------------------

extern short nHighestTexture;	//from interp.c

char pofNames [MAX_POLYGON_MODELS][SHORT_FILENAME_LEN];

//returns the number of this model
void CPolyModel::Load (const char *filename, int nTextures, int nFirstTexture, tRobotInfo *botInfoP)
{
FindMinMax ();
Setup ();
if (nHighestTexture + 1 != nTextures)
	Error ("Model <%s> references %d textures but specifies %d.", filename, nHighestTexture+1, nTextures);
m_info.nTextures = nTextures;
m_info.nFirstTexture = nFirstTexture;
m_info.nSimplerModel = 0;
}

//------------------------------------------------------------------------------
//walks through all submodels of a polymodel and determines the coordinate extremes
fix CPolyModel::Size (void)
{
	int			nModel = this - gameData.models.polyModels.Buffer (), nSubModels = 1;
	int			i;
	tHitbox		*phb = gameData.models.hitboxes [nModel].hitboxes;
	CFixVector	hv;
	double		dx, dy, dz;

for (i = 0; i <= MAX_HITBOXES; i++) {
	phb [i].vMin [X] = phb [i].vMin [Y] = phb [i].vMin [Z] = 0x7fffffff;
	phb [i].vMax [X] = phb [i].vMax [Y] = phb [i].vMax [Z] = -0x7fffffff;
	phb [i].vOffset [X] = phb [i].vOffset [Y] = phb [i].vOffset [Z] = 0;
	}
if (!(nSubModels = G3ModelMinMax (nModel, phb + 1)))
	nSubModels = GetPolyModelMinMax (reinterpret_cast<void*> (m_info.data.Buffer ()), phb + 1, 0) + 1;
for (i = 1; i <= nSubModels; i++) {
	dx = (phb [i].vMax [X] - phb [i].vMin [X]) / 2;
	dy = (phb [i].vMax [Y] - phb [i].vMin [Y]) / 2;
	dz = (phb [i].vMax [Z] - phb [i].vMin [Z]) / 2;
	phb [i].vSize [X] = (fix) dx;
	phb [i].vSize [Y] = (fix) dy;
	phb [i].vSize [Z] = (fix) dz;
	hv = phb [i].vMin + phb [i].vOffset;
	if (phb [0].vMin [X] > hv [X])
		phb [0].vMin [X] = hv [X];
	if (phb [0].vMin [Y] > hv [Y])
		phb [0].vMin [Y] = hv [Y];
	if (phb [0].vMin [Z] > hv [Z])
		phb [0].vMin [Z] = hv [Z];
	hv = phb [i].vMax + phb [i].vOffset;
	if (phb [0].vMax [X] < hv [X])
		phb [0].vMax [X] = hv [X];
	if (phb [0].vMax [Y] < hv [Y])
		phb [0].vMax [Y] = hv [Y];
	if (phb [0].vMax [Z] < hv [Z])
		phb [0].vMax [Z] = hv [Z];
	}
dx = (phb [0].vMax [X] - phb [0].vMin [X]) / 2;
dy = (phb [0].vMax [Y] - phb [0].vMin [Y]) / 2;
dz = (phb [0].vMax [Z] - phb [0].vMin [Z]) / 2;
phb [0].vSize [X] = (fix) dx;
phb [0].vSize [Y] = (fix) dy;
phb [0].vSize [Z] = (fix) dz;
gameData.models.hitboxes [nModel].nHitboxes = nSubModels;
for (i = 0; i <= nSubModels; i++)
	ComputeHitbox (nModel, i);
return (fix) (sqrt (dx * dx + dy * dy + dz + dz) /** 1.33*/);
}

//------------------------------------------------------------------------------

void CPolyModel::Check (ubyte *dataP)
{
for (;;) {
	switch (WORDVAL (dataP)) {
		case OP_EOF:
			return;

		case OP_DEFPOINTS: {
			int n = WORDVAL (dataP + 2);
			dataP += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int n = WORDVAL (dataP + 2);
			dataP += n * sizeof (CFixVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (dataP + 2);
			Assert (nVerts > 2);		//must have 3 or more points
			dataP += 30 + ((nVerts & ~1) + 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (dataP + 2);
			Assert (nVerts > 2);		//must have 3 or more points
			if (WORDVAL (dataP + 28) > nHighestTexture)
				nHighestTexture = WORDVAL (dataP + 28);
			dataP += 30 + ((nVerts & ~1) + 1) * 2 + nVerts * 12;
			break;
			}

		case OP_SORTNORM:
			Check (dataP + WORDVAL (dataP + 28));
			Check (dataP + WORDVAL (dataP + 30));
			dataP += 32;
			break;

		case OP_RODBM:
			dataP += 36;
			break;

		case OP_SUBCALL: {
			Check (dataP + WORDVAL (dataP + 16));
			dataP += 20;
			break;
			}

		case OP_GLOW:
			dataP += 4;
			break;

		default:
			Error ("invalid polygon model\n");
		}
	}
}

//------------------------------------------------------------------------------

void CPolyModel::Setup (void)
{
nHighestTexture = -1;
G3CheckAndSwap (m_info.data.Buffer ());
Check (m_info.data.Buffer ());
Size ();
}

//------------------------------------------------------------------------------

void CPolyModel::ReadData (CPolyModel* defModelP, CFile& cf)
{
if (!m_info.data.Create (m_info.nDataSize))
	Error ("Not enough memory for game models.");
cf.Read (m_info.data.Buffer (), sizeof (ubyte), m_info.nDataSize);
if (defModelP) {
	defModelP->Data ().Destroy ();
	defModelP->Data () = m_info.data;
	if (!defModelP->Data ().Buffer ())
		Error ("Not enough memory for game models.");
	}
#ifdef WORDS_NEED_ALIGNMENT
AlignPolyModelData (modelP);
#endif
G3CheckAndSwap (m_info.data.Buffer ());
//verify (m_info.data);
Setup ();
}

//------------------------------------------------------------------------------

int CPolyModel::Read (int bHMEL, CFile& cf)
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
	m_info.nModels = 1;
	}
else
	m_info.nModels = cf.ReadInt ();
m_info.nDataSize = cf.ReadInt ();
cf.ReadInt ();
m_info.data = NULL;
for (i = 0; i < MAX_SUBMODELS; i++)
	m_info.subModels.ptrs [i] = cf.ReadInt ();
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (m_info.subModels.offsets [i]);
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (m_info.subModels.norms [i]);
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (m_info.subModels.pnts [i]);
for (i = 0; i < MAX_SUBMODELS; i++)
	m_info.subModels.rads [i] = cf.ReadFix ();
cf.Read (m_info.subModels.parents.Buffer (), MAX_SUBMODELS, 1);
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (m_info.subModels.mins [i]);
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (m_info.subModels.maxs [i]);
cf.ReadVector (m_info.mins);
cf.ReadVector (m_info.maxs);
m_info.rad = cf.ReadFix ();
m_info.nTextures = cf.ReadByte ();
m_info.nFirstTexture = cf.ReadShort ();
m_info.nSimplerModel = cf.ReadByte ();
m_info.nType = 0;
return 1;
}

//------------------------------------------------------------------------------

int CPolyModel::LoadTextures (tBitmapIndex*	altTextures)
{
	int	i, j, nTextures = m_info.nTextures;

if (altTextures) {
	for (i = 0; i < nTextures; i++) {
		gameData.models.textureIndex [i] = altTextures [i];
		gameData.models.textures [i] = gameData.pig.tex.bitmaps [gameStates.app.bD1Model] + altTextures [i].index;
#ifdef _3DFX
      gameData.models.textures [i]->nId = gameData.models.textureIndex [i].index;
#endif
		}
	}
else {
	for (i = 0, j = m_info.nFirstTexture; i < nTextures; i++, j++) {
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
	LoadBitmap (gameData.models.textureIndex [i].index, gameStates.app.bD1Model);
// Hmmm... cache got flushed in the middle of paging all these in,
// so we need to reread them all in.
if (gameData.pig.tex.bPageFlushed) {
	gameData.pig.tex.bPageFlushed = 0;
	for (i = 0; i < nTextures; i++)
		LoadBitmap (gameData.models.textureIndex [i].index, gameStates.app.bD1Model);
}
// Make sure that they can all fit in memory.
Assert (gameData.pig.tex.bPageFlushed == 0);
#endif
return nTextures;
}

//------------------------------------------------------------------------------

void CPolyModel::Destroy (void)
{
m_info.data.Destroy ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CSubModelData::Create (void)
{
ptrs.Create (MAX_SUBMODELS);
offsets.Create (MAX_SUBMODELS);
norms.Create (MAX_SUBMODELS);   
pnts.Create (MAX_SUBMODELS);    
rads.Create (MAX_SUBMODELS);       
parents.Create (MAX_SUBMODELS);    
mins.Create (MAX_SUBMODELS);
maxs.Create (MAX_SUBMODELS);

ptrs.Clear ();
offsets.Clear ();
norms.Clear ();   
pnts.Clear ();    
rads.Clear ();    
parents.Clear (); 
mins.Clear ();
maxs.Clear ();
}

//------------------------------------------------------------------------------

void CSubModelData::Destroy (void)
{
ptrs.Destroy ();
offsets.Destroy ();
norms.Destroy ();   
pnts.Destroy ();    
rads.Destroy ();       
parents.Destroy ();    
mins.Destroy ();
maxs.Destroy ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int ReadPolyModels (CPolyModel* modelP, int n, CFile& cf)
{
	int i;

for (i = 0; i < n; i++)
	if (!modelP [i].Read (0, cf))
		break;
return i;
}

//------------------------------------------------------------------------------

int LoadPolyModel (const char* filename, int nTextures, int nFirstTexture, tRobotInfo *botInfoP)
{
Assert (gameData.models.nPolyModels < MAX_POLYGON_MODELS);
Assert (nTextures < MAX_POLYOBJ_TEXTURES);
#if TRACE
if (gameData.models.nPolyModels > MAX_POLYGON_MODELS - 10)
	console.printf (CON_VERBOSE, "Used %d/%d polygon model slots\n", gameData.models.nPolyModels+1, MAX_POLYGON_MODELS);
#endif
Assert (strlen (filename) <= 12);
strcpy (pofNames [gameData.models.nPolyModels], filename);
gameData.models.polyModels [gameData.models.nPolyModels++].Load (filename, nTextures, nFirstTexture, botInfoP);
return gameData.models.nPolyModels - 1;
}

//------------------------------------------------------------------------------
//compare against this size when figuring how far to place eye for picture
//draws the given model in the current canvas.  The distance is set to
//more-or-less fill the canvas.  Note that this routine actually renders
//into an off-screen canvas that it creates, then copies to the current
//canvas.

void DrawModelPicture (int nModel, CAngleVector *orientAngles)
{
	CFixVector	p = CFixVector::ZERO;
	CFixMatrix	o = CFixMatrix::IDENTITY;

Assert ((nModel >= 0) && (nModel < gameData.models.nPolyModels));
G3StartFrame (0, 0);
glDisable (GL_BLEND);
G3SetViewMatrix (p, o, gameStates.render.xZoom, 1);
if (gameData.models.polyModels [nModel].Rad ())
	p [Z] = FixMulDiv (DEFAULT_VIEW_DIST, gameData.models.polyModels [nModel].Rad (), BASE_MODEL_SIZE);
else
	p [Z] = DEFAULT_VIEW_DIST;
o = CFixMatrix::Create (*orientAngles);
DrawPolyModel (NULL, &p, &o, NULL, nModel, 0, I2X (1), NULL, NULL, NULL);
G3EndFrame ();
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	GrUpdate (0);
}

//------------------------------------------------------------------------------
//eof
