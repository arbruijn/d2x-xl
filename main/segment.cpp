#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"
#include "u_mem.h"
#include "error.h"

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/

// Number of vertices in current mine (ie, gameData.segs.vertices, pointed to by Vp)
//	Translate table to get opposite CSide of a face on a CSegment.
char	sideOpposite[MAX_SIDES_PER_SEGMENT] = {WRIGHT, WBOTTOM, WLEFT, WTOP, WFRONT, WBACK};

//	Note, this MUST be the same as sideVertIndex, it is an int for speed reasons.
int sideVertIndex [MAX_SIDES_PER_SEGMENT][4] = {
			{7,6,2,3},			// left
			{0,4,7,3},			// top
			{0,1,5,4},			// right
			{2,6,5,1},			// bottom
			{4,5,6,7},			// back
			{3,2,1,0}			// front
	};	

//------------------------------------------------------------------------------
// reads a tSegment2 structure from a CFile
 
void CSegment::Read (CFile& cf, bool bExtended)
{
if (bExtended) {
	m_nType = cf.ReadByte ();
	m_nMatCen = cf.ReadByte ();
	m_value = cf.ReadByte ();
	m_s2Flags = cf.ReadByte ();
	m_xAvgSegLight = cf.ReadFix ();
	}
}

//------------------------------------------------------------------------------
// reads a tLightDelta structure from a CFile

void ReadlightDelta (tLightDelta *dlP, CFile& cf)
{
dlP->nSegment = cf.ReadShort ();
dlP->nSide = cf.ReadByte ();
cf.ReadByte ();
if (!(dlP->bValid = (dlP->nSegment >= 0) && (dlP->nSegment < gameData.segs.nSegments) && (dlP->nSide >= 0) && (dlP->nSide < 6)))
	PrintLog ("Invalid delta light data %d (%d,%d)\n", dlP - gameData.render.lights.deltas, dlP->nSegment, dlP->nSide);
cf.Read (dlP->vertLight, sizeof (dlP->vertLight [0]), sizeofa (dlP->vertLight));
}


//------------------------------------------------------------------------------
// reads a tLightDeltaIndex structure from a CFile

void ReadlightDeltaIndex (tLightDeltaIndex *di, CFile& cf)
{
if (gameStates.render.bD2XLights) {
	short	i, j;
	di->d2x.nSegment = cf.ReadShort ();
	i = (short) cf.ReadByte ();
	j = (short) cf.ReadByte ();
	di->d2x.nSide = i;
	di->d2x.count = (j << 5) + ((i >> 3) & 63);
	di->d2x.index = cf.ReadShort ();
	}
else {
	di->d2.nSegment = cf.ReadShort ();
	di->d2.nSide = cf.ReadByte ();
	di->d2.count = cf.ReadByte ();
	di->d2.index = cf.ReadShort ();
	}
}
#endif

//------------------------------------------------------------------------------

int CSkyBox::CountSegments (void)
{
	tSegment2	*seg2P;
	int			i, nSegments;

for (i = gameData.segs.nSegments, nSegments = 0, seg2P = SEGMENTS.Buffer (); i; i--, seg2P++)
	if (seg2P->special == SEGMENT_IS_SKYBOX)
		nSegments++;
return nSegments;
}


//------------------------------------------------------------------------------

void CSkyBox::Destroy (void)
{
CStack::Destroy ();
gameStates.render.bHaveSkyBox = 0;
}

//------------------------------------------------------------------------------

int BuildSkyBoxSegList (void)
{
gameData.segs.skybox.Destroy ();

short nSegments = gameData.segs.skybox.CountSegments ();

if (!nSegments) {
	return 0;

	tSegment2	*seg2P;
	int			h, i;

if (!(gameData.segs.skybox.Create (nSegments)))
	return 0;
for (h = gameData.segs.nSegments, i = 0, seg2P = SEGMENTS.Buffer (); i < h; i++, seg2P++)
	if (seg2P->special == SEGMENT_IS_SKYBOX)
		gameData.segs.skybox.Push (i);
	}
gameStates.render.bHaveSkyBox = (gameData.segs.skybox.ToS () > 0);
return gameData.segs.skybox.ToS ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

inline CWall* CSide::Wall (void) { return IS_WALL (m_nWall) ? WALLS + m_nWall : NULL; }

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

inline CFixVector& CSegment::Normal (int nSide, int nFace) {
	if (gameStates.render.bRendering)
		return m_sides [nSide].m_rotNorms [nFace];
	else
		return m_sides [nSide].m_normals [nFace];
	}

//------------------------------------------------------------------------------
//eof
