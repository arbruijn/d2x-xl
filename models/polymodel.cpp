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

#include "descent.h"
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
#include "objrender.h"

#define PM_COMPATIBLE_VERSION 6
#define PM_OBJFILE_VERSION 8

#define ID_OHDR 0x5244484f // 'RDHO'  //Object header
#define ID_SOBJ 0x4a424f53 // 'JBOS'  //Subobject header
#define ID_GUNS 0x534e5547 // 'SNUG'  //List of guns on this CObject
#define ID_ANIM 0x4d494e41 // 'MINA'  //Animation data
#define ID_IDTA 0x41544449 // 'ATDI'  //Interpreter data
#define ID_TXTR 0x52545854 // 'RTXT'  //Texture filename list

#define	MODEL_BUF_SIZE	32768

//------------------------------------------------------------------------------

CAngleVector animAngles [N_ANIM_STATES][MAX_SUBMODELS];

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void SetRobotAngles (tRobotInfo* pRobotInfo, CPolyModel* pModel, CAngleVector angs [N_ANIM_STATES][MAX_SUBMODELS]);

//------------------------------------------------------------------------------

void CPolyModel::POF_Seek (int32_t len, int32_t nType)
{
switch (nType) {
	case SEEK_SET:	
		m_filePos = len;	
		break;
	case SEEK_CUR:	
		m_filePos += len;	
		break;
	case SEEK_END:
		Assert (len <= 0);	//	seeking from end, better be moving back.
		m_filePos = m_fileEnd + len;
		break;
	}
if (m_filePos > MODEL_BUF_SIZE)
	return;
}

//------------------------------------------------------------------------------

int32_t CPolyModel::POF_ReadInt (uint8_t *pBuffer)
{
int32_t i = *(reinterpret_cast<int32_t*> (&pBuffer [m_filePos]));
m_filePos += 4;
return INTEL_INT (i);
}

//------------------------------------------------------------------------------

size_t CPolyModel::POF_Read (void *dst, size_t elsize, size_t nelem, uint8_t *pBuffer)
{
if (nelem * elsize + (size_t) m_filePos > (size_t) m_fileEnd)
	return 0;
memcpy (dst, &pBuffer [m_filePos], elsize*nelem);
m_filePos += (int32_t) (elsize * nelem);
if (m_filePos > MODEL_BUF_SIZE)
	return 0;
return nelem;
}

//------------------------------------------------------------------------------

int16_t CPolyModel::POF_ReadShort (uint8_t *pBuffer)
{
int16_t s = * (reinterpret_cast<int16_t*> (&pBuffer [m_filePos]));
m_filePos += 2;
return INTEL_SHORT (s);
}

//------------------------------------------------------------------------------

void CPolyModel::POF_ReadString (char *buf, int32_t max_char, uint8_t *pBuffer)
{
for (int32_t i = 0; i < max_char; i++)
	if ((*buf++ = pBuffer [m_filePos++]) == 0)
		break;
}

//------------------------------------------------------------------------------

void CPolyModel::POF_ReadVecs (CFixVector *vecs, int32_t n, uint8_t *pBuffer)
{
memcpy (vecs, &pBuffer [m_filePos], n * sizeof (*vecs));
m_filePos += n * sizeof (*vecs);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
while (n > 0)
	VmsVectorSwap (vecs [--n]);
#endif
}

//------------------------------------------------------------------------------

void CPolyModel::POF_ReadAngs (CAngleVector *angs, int32_t n, uint8_t *pBuffer)
{
memcpy (angs, &pBuffer [m_filePos], n * sizeof (*angs));
m_filePos += n * sizeof (*angs);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
while (n > 0)
	VmsAngVecSwap (angs [--n]);
#endif
}

#ifdef WORDS_NEED_ALIGNMENT

//------------------------------------------------------------------------------

uint8_t * old_dest (chunk o) // return where chunk is (in unaligned struct)
{
	return o.old_base + INTEL_SHORT (*reinterpret_cast<int16_t*> (o.old_base + o.offset));
}

//------------------------------------------------------------------------------

uint8_t * new_dest (chunk o) // return where chunk is (in aligned struct)
{
	return o.new_base + INTEL_SHORT (*reinterpret_cast<int16_t*> (o.old_base + o.offset)) + o.correction;
}

//------------------------------------------------------------------------------
/*
 * find chunk with smallest address
 */
int32_t get_first_chunks_index (chunk *chunk_list, int32_t no_chunks)
{
	int32_t i, first_index = 0;
	Assert (no_chunks >= 1);
	for (i = 1; i < no_chunks; i++)
		if (old_dest (chunk_list [i]) < old_dest (chunk_list [first_index]))
			first_index = i;
	return first_index;
}

//------------------------------------------------------------------------------

void AlignPolyModelData (CPolyModel* pModel)
{
	int32_t i, chunk_len;
	int32_t total_correction = 0;
	uint8_t *cur_old, *cur_new;
	chunk cur_ch;
	chunk ch_list [MAX_CHUNKS];
	int32_t no_chunks = 0;
	int32_t tmp_size = pModel->nDataSize + SHIFT_SPACE;
	uint8_t *tmp = new uint8_t [tmp_size]; // where we build the aligned version of pModel->Data ()

	Assert (tmp != NULL);
	//start with first chunk (is always aligned!)
	cur_old = pModel->Data ();
	cur_new = tmp;
	chunk_len = get_chunks (cur_old, cur_new, ch_list, &no_chunks);
	memcpy (cur_new, cur_old, chunk_len);
	while (no_chunks > 0) {
		int32_t first_index = get_first_chunks_index (ch_list, no_chunks);
		cur_ch = ch_list [first_index];
		// remove first chunk from array:
		no_chunks--;
		for (i = first_index; i < no_chunks; i++)
			ch_list [i] = ch_list [i + 1];
		// if (new) address unaligned:
		if ((uint32_t)new_dest (cur_ch) % 4L != 0) {
			// calculate how much to move to be aligned
			int16_t to_shift = 4 - (uint32_t)new_dest (cur_ch) % 4L;
			// correct chunks' addresses
			cur_ch.correction += to_shift;
			for (i = 0; i < no_chunks; i++)
				ch_list [i].correction += to_shift;
			total_correction += to_shift;
			Assert ((uint32_t)new_dest (cur_ch) % 4L == 0);
			Assert (total_correction <= SHIFT_SPACE); // if you get this, increase SHIFT_SPACE
		}
		//write (corrected) chunk for current chunk:
		* (reinterpret_cast<int16_t*> (cur_ch.new_base + cur_ch.offset))
		  = INTEL_SHORT (cur_ch.correction)
				+ INTEL_SHORT (* (reinterpret_cast<int16_t*> (cur_ch.old_base + cur_ch.offset)));
		//write (correctly aligned) chunk:
		cur_old = old_dest (cur_ch);
		cur_new = new_dest (cur_ch);
		chunk_len = get_chunks (cur_old, cur_new, ch_list, &no_chunks);
		memcpy (cur_new, cur_old, chunk_len);
		//correct submodel_ptr's for pModel, too
		for (i = 0; i < MAX_SUBMODELS; i++)
			if (pModel->Data () + pModel->SubModels ().ptrs [i] >= cur_old
			    && pModel->Data () + pModel->SubModels ().ptrs [i] < cur_old + chunk_len)
				pModel->SubModels ().ptrs [i] += (cur_new - tmp) - (cur_old - pModel->Data ());
 	}
	pModel->Data ().Destroy ();
	pModel->nDataSize += total_correction;
	if (!pModel->Data ().Create (pModel->nDataSize))
		Error ("Not enough memory for game models.");
	pModel->Data () = tmp;
	delete [] tmp;
}
#endif //def WORDS_NEED_ALIGNMENT

//------------------------------------------------------------------------------

void CPolyModel::Init (void)
{
m_info.nType = 0;
m_info.nModels = 0;
m_info.nDataSize = 0;
m_info.rad [0] = 
m_info.rad [1] = 0;
m_info.nTextures = 0;
m_info.nFirstTexture = 0;
m_info.nSimplerModel = 0;
m_info.bCustom = false;
m_fileEnd = -1;
m_filePos = -1;
m_info.mins.SetZero ();
m_info.maxs.SetZero ();
}

//------------------------------------------------------------------------------
//reads a binary file containing a 3d model
void CPolyModel::Parse (const char *filename, tRobotInfo *pRobotInfo)
{
	CFile cf;
	int16_t version;
	int32_t	id, len, next_chunk;
	int32_t	animFlag = 0;
	uint8_t modelBuf [MODEL_BUF_SIZE];

if (!cf.Open (filename, gameFolders.game.szData [0], "rb", 0))
	Error ("Can't open file <%s>", filename);
Assert (cf.Length () <= MODEL_BUF_SIZE);
m_filePos = 0;
m_fileEnd = (int32_t) cf.Read (modelBuf, 1, cf.Length ());
cf.Close ();
id = POF_ReadInt (modelBuf);
if (id != 0x4f505350) /* 'OPSP' */
	Error ("Bad ID in model file <%s>", filename);
version = POF_ReadShort (modelBuf);
if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
	Error ("Bad version (%d) in model file <%s>", version, filename);
//if (FindArg ("-bspgen"))
//printf ("bspgen -c1");
while (POF_Read (&id, sizeof (id), 1, modelBuf) == 1) {
	id = INTEL_INT (id);
	//id  = POF_ReadInt (modelBuf);
	len = POF_ReadInt (modelBuf);
	next_chunk = m_filePos + len;
	switch (id) {
		case ID_OHDR: {		//Object header
			CFixVector pmmin, pmmax;
			m_info.nModels = POF_ReadInt (modelBuf);
			m_info.rad [0] =
			m_info.rad [1] = POF_ReadInt (modelBuf);
			Assert (m_info.nModels <= MAX_SUBMODELS);
			POF_ReadVecs (&pmmin, 1, modelBuf);
			POF_ReadVecs (&pmmax, 1, modelBuf);
			if (FindArg ("-bspgen")) {
				CFixVector v = pmmax - pmmin;
				fix l = v.v.coord.x;
				if (v.v.coord.y > l)
					l = v.v.coord.y;
				if (v.v.coord.z > l)
					l = v.v.coord.z;
				//printf (" -l%.3f", X2F (l));
				}
			break;
			}

		case ID_SOBJ: {		//Subobject header
			int32_t n = POF_ReadShort (modelBuf);
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
			if (pRobotInfo) {
				int32_t i;
				CFixVector gun_dir;
				pRobotInfo->nGuns = POF_ReadInt (modelBuf);
				if (pRobotInfo->nGuns)
					animFlag++;
				Assert (pRobotInfo->nGuns <= MAX_GUNS);
				for (i = 0; i < pRobotInfo->nGuns; i++) {
					int32_t id = POF_ReadShort (modelBuf);
					Assert (id < pRobotInfo->nGuns);
					pRobotInfo->gunSubModels [id] = (char) POF_ReadShort (modelBuf);
					Assert (pRobotInfo->gunSubModels [id] != 0xff);
					POF_ReadVecs (&pRobotInfo->gunPoints [id], 1, modelBuf);
					if (version >= 7)
						POF_ReadVecs (&gun_dir, 1, modelBuf);
					}
				}
			else
				POF_Seek (len, SEEK_CUR);
			break;
			}

		case ID_ANIM:		//Animation data
			animFlag++;
			if (pRobotInfo) {
				int32_t f, m, n_frames = POF_ReadShort (modelBuf);
				Assert (n_frames == N_ANIM_STATES);
				for (m = 0; m <m_info.nModels; m++)
					for (f = 0; f < n_frames; f++)
						POF_ReadAngs (&animAngles [f][m], 1, modelBuf);
				SetRobotAngles (pRobotInfo, this, animAngles);
				}
			else
				POF_Seek (len, SEEK_CUR);
			break;

		case ID_TXTR: {		//Texture filename list
			char name_buf [128];
			int32_t n = POF_ReadShort (modelBuf);
			while (n--)
				POF_ReadString (name_buf, 128, modelBuf);
			break;
			}

		case ID_IDTA:		//Interpreter data
			if (!Create (len))
				Error ("Not enough memory for game models.");
			m_info.nDataSize = len;
			POF_Read (Buffer (), 1, len, modelBuf);
			break;

		default:
			POF_Seek (len, SEEK_CUR);
			break;
		}
	if (version >= 8)		// Version 8 needs 4-byte alignment!!!
		POF_Seek (next_chunk, SEEK_SET);
	}
#ifdef WORDS_NEED_ALIGNMENT
G3AlignPolyModelData (pModel);
#endif
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
G3SwapPolyModelData (Buffer ());
#endif
}

//------------------------------------------------------------------------------

void CPolyModel::FindMinMax (void)
{
	uint16_t		nVerts;
	CFixVector*	vp;
	uint16_t*		pData, nType;

CFixVector& big_mn = m_info.mins;
CFixVector& big_mx = m_info.maxs;

for (int32_t i = 0; i < m_info.nModels; i++) {
	CFixVector& mn = m_info.subModels.mins [i];
	CFixVector& mx = m_info.subModels.maxs [i];
	CFixVector& ofs = m_info.subModels.offsets [i];
	pData = reinterpret_cast<uint16_t*> (Buffer () + m_info.subModels.ptrs [i]);
	nType = *pData++;
	Assert (nType == 7 || nType == 1);
	nVerts = *pData++;
	if (nType == 7)
		pData += 2;		//skip start & pad
	vp = reinterpret_cast<CFixVector*> (pData);
	mn = mx = *vp++;
	nVerts--;
	if (i == 0)
		big_mn = big_mx = mn;
	while (nVerts--) {
		if ((*vp).v.coord.x > mx.v.coord.x) mx.v.coord.x = (*vp).v.coord.x;
		if ((*vp).v.coord.y > mx.v.coord.y) mx.v.coord.y = (*vp).v.coord.y;
		if ((*vp).v.coord.z > mx.v.coord.z) mx.v.coord.z = (*vp).v.coord.z;
		if ((*vp).v.coord.x < mn.v.coord.x) mn.v.coord.x = (*vp).v.coord.x;
		if ((*vp).v.coord.y < mn.v.coord.y) mn.v.coord.y = (*vp).v.coord.y;
		if ((*vp).v.coord.z < mn.v.coord.z) mn.v.coord.z = (*vp).v.coord.z;
		if ((*vp).v.coord.x + ofs.v.coord.x > big_mx.v.coord.x) big_mx.v.coord.x = (*vp).v.coord.x + ofs.v.coord.x;
		if ((*vp).v.coord.y + ofs.v.coord.y > big_mx.v.coord.y) big_mx.v.coord.y = (*vp).v.coord.y + ofs.v.coord.y;
		if ((*vp).v.coord.z + ofs.v.coord.z > big_mx.v.coord.z) big_mx.v.coord.z = (*vp).v.coord.z + ofs.v.coord.z;
		if ((*vp).v.coord.x + ofs.v.coord.x < big_mn.v.coord.x) big_mn.v.coord.x = (*vp).v.coord.x + ofs.v.coord.x;
		if ((*vp).v.coord.y + ofs.v.coord.y < big_mn.v.coord.y) big_mn.v.coord.y = (*vp).v.coord.y + ofs.v.coord.y;
		if ((*vp).v.coord.z + ofs.v.coord.z < big_mn.v.coord.z) big_mn.v.coord.z = (*vp).v.coord.z + ofs.v.coord.z;
		vp++;
		}
	}
}

//------------------------------------------------------------------------------

extern int16_t nHighestTexture;	//from interp.c

char pofNames [MAX_POLYGON_MODELS][SHORT_FILENAME_LEN];

//returns the number of this model
void CPolyModel::Load (const char *filename, int32_t nTextures, int32_t nFirstTexture, tRobotInfo *pRobotInfo)
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
	int32_t			i, nSubModels;
	tHitbox*		phb = &gameData.models.hitboxes [m_info.nId].hitboxes [0];
	CFixVector	hv;
	double		dx, dy, dz;

for (i = 0; i <= MAX_HITBOXES; i++) {
	phb [i].vMin.v.coord.x = phb [i].vMin.v.coord.y = phb [i].vMin.v.coord.z = 0x7fffffff;
	phb [i].vMax.v.coord.x = phb [i].vMax.v.coord.y = phb [i].vMax.v.coord.z = -0x7fffffff;
	phb [i].vOffset.v.coord.x = phb [i].vOffset.v.coord.y = phb [i].vOffset.v.coord.z = 0;
	}
if (!(nSubModels = G3ModelMinMax (m_info.nId, phb + 1)))
	nSubModels = GetPolyModelMinMax (reinterpret_cast<void*> (Buffer ()), phb + 1, 0) + 1;
for (i = 1; i <= nSubModels; i++) {
	dx = (phb [i].vMax.v.coord.x - phb [i].vMin.v.coord.x) / 2;
	dy = (phb [i].vMax.v.coord.y - phb [i].vMin.v.coord.y) / 2;
	dz = (phb [i].vMax.v.coord.z - phb [i].vMin.v.coord.z) / 2;
	phb [i].vSize.v.coord.x = (fix) dx;
	phb [i].vSize.v.coord.y = (fix) dy;
	phb [i].vSize.v.coord.z = (fix) dz;
	hv = phb [i].vMin + phb [i].vOffset;
	if (phb [0].vMin.v.coord.x > hv.v.coord.x)
		phb [0].vMin.v.coord.x = hv.v.coord.x;
	if (phb [0].vMin.v.coord.y > hv.v.coord.y)
		phb [0].vMin.v.coord.y = hv.v.coord.y;
	if (phb [0].vMin.v.coord.z > hv.v.coord.z)
		phb [0].vMin.v.coord.z = hv.v.coord.z;
	hv = phb [i].vMax + phb [i].vOffset;
	if (phb [0].vMax.v.coord.x < hv.v.coord.x)
		phb [0].vMax.v.coord.x = hv.v.coord.x;
	if (phb [0].vMax.v.coord.y < hv.v.coord.y)
		phb [0].vMax.v.coord.y = hv.v.coord.y;
	if (phb [0].vMax.v.coord.z < hv.v.coord.z)
		phb [0].vMax.v.coord.z = hv.v.coord.z;
	}
dx = (phb [0].vMax.v.coord.x - phb [0].vMin.v.coord.x) / 2;
dy = (phb [0].vMax.v.coord.y - phb [0].vMin.v.coord.y) / 2;
dz = (phb [0].vMax.v.coord.z - phb [0].vMin.v.coord.z) / 2;
phb [0].vSize.v.coord.x = (fix) dx;
phb [0].vSize.v.coord.y = (fix) dy;
phb [0].vSize.v.coord.z = (fix) dz;
gameData.models.hitboxes [m_info.nId].nHitboxes = nSubModels;
for (i = 0; i <= nSubModels; i++)
	ComputeHitbox (m_info.nId, i);
return (fix) (sqrt (dx * dx + dy * dy + dz + dz) /** 1.33*/);
}

//------------------------------------------------------------------------------

void CPolyModel::Check (uint8_t *pData)
{
for (;;) {
	switch (WORDVAL (pData)) {
		case OP_EOF:
			return;

		case OP_DEFPOINTS: {
			int32_t n = WORDVAL (pData + 2);
			pData += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int32_t n = WORDVAL (pData + 2);
			pData += n * sizeof (CFixVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int32_t nVerts = WORDVAL (pData + 2);
			Assert (nVerts > 2);		//must have 3 or more points
			pData += 30 + ((nVerts & ~1) + 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int32_t nVerts = WORDVAL (pData + 2);
			Assert (nVerts > 2);		//must have 3 or more points
			if (WORDVAL (pData + 28) > nHighestTexture)
				nHighestTexture = WORDVAL (pData + 28);
			pData += 30 + ((nVerts & ~1) + 1) * 2 + nVerts * 12;
			break;
			}

		case OP_SORTNORM:
			Check (pData + WORDVAL (pData + 28));
			Check (pData + WORDVAL (pData + 30));
			pData += 32;
			break;

		case OP_RODBM:
			pData += 36;
			break;

		case OP_SUBCALL: {
			Check (pData + WORDVAL (pData + 16));
			pData += 20;
			break;
			}

		case OP_GLOW:
			pData += 4;
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
G3CheckAndSwap (Buffer ());
Check (Buffer ());
Size ();
}

//------------------------------------------------------------------------------

void CPolyModel::ReadData (CPolyModel* pDefModel, CFile& cf)
{
#if DBG
if (m_info.nId == nDbgModel)
	BRP;
#endif
if (!Create (m_info.nDataSize))
	Error ("Not enough memory for game models.");
CByteArray::Read (cf, m_info.nDataSize);
if (pDefModel) {
	pDefModel->Destroy ();
	*pDefModel = *this;
	if (!pDefModel->Data ())
		Error ("Not enough memory for game models.");
	}
#ifdef WORDS_NEED_ALIGNMENT
AlignPolyModelData (pModel);
#endif
G3CheckAndSwap (Buffer ());
Setup ();
}

//------------------------------------------------------------------------------

int32_t CPolyModel::Read (int32_t bHMEL, int32_t bCustom, CFile& cf)
{
	int32_t	i;

#if DBG
if (m_info.nId == nDbgModel)
	BRP;
#endif
if (bHMEL) { // HMEL
	char	szId [4];

	cf.Read (szId, sizeof (szId), 1);
	if (strnicmp (szId, "HMEL", 4))
		return 0;
	if (cf.ReadInt () != 1)
		return 0;
	cf.ReadInt ();
	cf.ReadInt ();
	m_info.nModels = 1;
	}
else
	m_info.nModels = cf.ReadInt ();
m_info.nDataSize = cf.ReadInt ();
cf.ReadInt ();
Destroy ();
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
cf.Read (&m_info.subModels.parents [0], MAX_SUBMODELS, 1);
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (m_info.subModels.mins [i]);
for (i = 0; i < MAX_SUBMODELS; i++)
	cf.ReadVector (m_info.subModels.maxs [i]);
cf.ReadVector (m_info.mins);
cf.ReadVector (m_info.maxs);
SetRad (cf.ReadFix (), bCustom);
m_info.nTextures = cf.ReadByte ();
m_info.nFirstTexture = cf.ReadShort ();
m_info.nSimplerModel = cf.ReadByte ();
m_info.nType = 0;
return 1;
}

//------------------------------------------------------------------------------

int32_t CPolyModel::LoadTextures (tBitmapIndex*	altTextures)
{
	int32_t	i, j, nTextures = m_info.nTextures;

if (altTextures) {
	for (i = 0; i < nTextures; i++) {
		gameData.models.textureIndex [i] = altTextures [i];
		gameData.models.textures [i] = gameData.pig.tex.bitmaps [gameStates.app.bD1Model] + altTextures [i].index;
		if (gameStates.render.bBuildModels)
			LoadTexture (altTextures [i].index, 0, gameStates.app.bD1Model);
		}
	}
else {
	for (i = 0, j = m_info.nFirstTexture; i < nTextures; i++, j++) {
		gameData.models.textureIndex [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [j]];
		gameData.models.textures [i] = gameData.pig.tex.bitmaps [gameStates.app.bD1Model] + gameData.models.textureIndex [i].index;
		if (gameStates.render.bBuildModels)
			LoadTexture (gameData.models.textureIndex [i].index, 0, gameStates.app.bD1Model);
		}
	}
#if DBG
if (m_info.nId == nDbgModel)
	BRP;
#endif
// Make sure the textures for this CObject are paged in...
gameData.pig.tex.bPageFlushed = 0;
for (i = 0; i < nTextures; i++)
	LoadTexture (gameData.models.textureIndex [i].index, 0, gameStates.app.bD1Model);
// Hmmm... cache got flushed in the middle of paging all these in,
// so we need to reread them all in.
if (gameData.pig.tex.bPageFlushed) {
	gameData.pig.tex.bPageFlushed = 0;
	for (i = 0; i < nTextures; i++)
		LoadTexture (gameData.models.textureIndex [i].index, 0, gameStates.app.bD1Model);
}
// Make sure that they can all fit in memory.
Assert (gameData.pig.tex.bPageFlushed == 0);
return nTextures;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CSubModelData::Create (void)
{
#if 1
memset (this, 0, sizeof (*this));
#else
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
#endif
}

//------------------------------------------------------------------------------

void CSubModelData::Destroy (void)
{
#if 0
ptrs.Destroy ();
offsets.Destroy ();
norms.Destroy ();   
pnts.Destroy ();    
rads.Destroy ();       
parents.Destroy ();    
mins.Destroy ();
maxs.Destroy ();
#endif
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32_t ReadPolyModels (CArray<CPolyModel>& models, int32_t nModels, CFile& cf, int32_t nOffset)
{
	int32_t i;

for (i = nOffset, nModels += nOffset; i < nModels; i++)
	if (!models [i].Read (0, 0, cf))
		break;
return i;
}

//------------------------------------------------------------------------------

int32_t LoadPolyModel (const char* filename, int32_t nTextures, int32_t nFirstTexture, tRobotInfo *pRobotInfo)
{
Assert (gameData.models.nPolyModels < MAX_POLYGON_MODELS);
Assert (nTextures < MAX_POLYOBJ_TEXTURES);
#if TRACE
if (gameData.models.nPolyModels > MAX_POLYGON_MODELS - 10)
	console.printf (CON_VERBOSE, "Used %d/%d polygon model slots\n", gameData.models.nPolyModels+1, MAX_POLYGON_MODELS);
#endif
Assert (strlen (filename) <= 12);
strcpy (pofNames [gameData.models.nPolyModels], filename);
gameData.models.polyModels [0][gameData.models.nPolyModels++].Load (filename, nTextures, nFirstTexture, pRobotInfo);
return gameData.models.nPolyModels - 1;
}

//------------------------------------------------------------------------------

#define BUILD_ALL_MODELS 0

void CModelData::Prepare (void)
{
	int32_t		h, i, j;
	CObject		o, *pObj = OBJECTS.Buffer ();
	const char*	pszHires;

if (!OBJECTS.Buffer ())
	return;
PrintLog (1, "building optimized polygon model data\n");
gameStates.render.nType = RENDER_TYPE_OBJECTS;
gameStates.render.nShadowPass = 1;
gameStates.render.bBuildModels = 1;
h = 0;
#if !BUILD_ALL_MODELS
for (i = 0, j = int32_t (OBJECTS.Length ()); i < j; i++, pObj++) {
	if ((pObj->info.nSegment >= 0) && (pObj->info.nType != 255) && (pObj->info.renderType == RT_POLYOBJ) &&
		 !G3HaveModel (pObj->ModelId ())) {
		if (gameStates.app.nLogLevel > 1)
			PrintLog (1, "building model %d\n", pObj->ModelId ());
#if DBG
		if (pObj->ModelId () == nDbgModel)
			BRP;
#endif
		if (DrawPolygonObject (pObj, 0))
			h++;
		if (gameStates.app.nLogLevel > 1)
			PrintLog (-1);
		}
	}
#endif
memset (&o, 0, sizeof (o));
o.info.nType = OBJ_WEAPON;
o.info.position = OBJECT (0)->info.position;
o.rType.polyObjInfo.nTexOverride = -1;
PrintLog (1, "building optimized replacement model data\n");
#if BUILD_ALL_MODELS
j = 0;
for (i = 0; i < MAX_POLYGON_MODELS; i++) {
	o.rType.polyObjInfo.nModel = i; //replacementModels [i].nModel;
#else
j = ReplacementModelCount ();
for (i = 0; i < j; i++)
	if ((pszHires = replacementModels [i].pszHires) && (strstr (pszHires, "laser") == pszHires))
		break;
for (tReplacementModel *pReplModel = replacementModels + i; i < j; i++, pReplModel++) {
#endif
	if ((pszHires = pReplModel->pszHires)) {
		if (strstr (pszHires, "pminepack") == pszHires)
			o.info.nType = OBJ_POWERUP;
		else if (strstr (pszHires, "hostage") == pszHires)
			o.info.nType = OBJ_HOSTAGE;
		}
	o.info.nId = (uint8_t) pReplModel->nId;
#if DBG
	if (o.info.nId == nDbgObjId)
		BRP;
#endif
	o.rType.polyObjInfo.nModel = pReplModel->nModel;
	if (!G3HaveModel (o.ModelId ())) {
#if DBG
		if (o.ModelId () == nDbgModel)
			BRP;
#endif
		if (gameStates.app.nLogLevel > 1)
			PrintLog (1, "building model %d (%s)\n", o.ModelId (), pszHires ? pszHires : "n/a");
		if (DrawPolygonObject (&o, 0))
			h++;
		if (gameStates.app.nLogLevel > 1)
			PrintLog (-1);
		}
	if (o.info.nType == OBJ_HOSTAGE)
		o.info.nType = OBJ_POWERUP;
	}
PrintLog (-1);
gameStates.render.nShadowPass = 0;
	gameStates.render.bBuildModels = 0;
PrintLog (1, "saving optimized polygon model data\n", h);
SaveModelData ();
PrintLog (-1);
PrintLog (-1, "finished building optimized polygon model data (%d models converted)\n", h);
}

//------------------------------------------------------------------------------
//eof
