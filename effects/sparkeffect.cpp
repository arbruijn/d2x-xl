#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "text.h"
#include "cfile.h"
#include "bm.h"
#include "textures.h"
#include "ogl_defs.h"
#include "maths.h"
#include "vecmat.h"
#include "hudmsg.h"
#include "gameseg.h"
#include "piggy.h"
#include "renderlib.h"
#include "sparkeffect.h"
#include "transprender.h"

#define SPARK_MIN_PROB		16
#define SPARK_FRAME_TIME	50

CSparkManager sparkManager;

//-----------------------------------------------------------------------------

void CEnergySpark::Setup (short nSegment, ubyte nType)
{
	CSegment*			segP = SEGMENTS + nSegment;
	CFixVector			vOffs;
	CFloatVector		vMaxf, vMax2f;

vMaxf.Assign (segP->m_extents [0]);
vMax2f = vMaxf * 2;
if (m_tRender)
	return;
if (!m_nProb)
	m_nProb = SPARK_MIN_PROB;
if (gameStates.app.nSDLTicks - m_tCreate < SPARK_FRAME_TIME)
	return;
m_nType = nType;
m_tCreate = gameStates.app.nSDLTicks;
if (d_rand () % m_nProb)
	m_nProb--;
else {
	vOffs [X] = F2X (vMaxf [X] - f_rand () * vMax2f [X]);
	vOffs [Y] = F2X (vMaxf [Y] - f_rand () * vMax2f [Y]);
	vOffs [Z] = F2X (vMaxf [Z] - f_rand () * vMax2f [Z]);
	m_vPos = segP->Center () + vOffs;
	if ((vOffs.Mag () > segP->MinRad ()) && segP->Masks (m_vPos, 0).m_center)
		m_nProb = 1;
	else {
		m_xSize = I2X (1) + 4 * d_rand ();
		m_nFrame = 0;
		m_tRender = -1;
		m_bRendered = 0;
		m_nProb = SPARK_MIN_PROB;
		if (gameOpts->render.effects.bMovingSparks) {
			m_vDir [X] = (I2X (1) / 4) - d_rand ();
			m_vDir [Y] = (I2X (1) / 4) - d_rand ();
			m_vDir [Z] = (I2X (1) / 4) - d_rand ();
			CFixVector::Normalize (m_vDir);
			m_vDir *= ((I2X (1) / (16 + d_rand () % 16)));
			}
		else
			m_vDir.SetZero ();
		}
	}
}

//-----------------------------------------------------------------------------

void CEnergySpark::Update (void)
{
if (!m_tRender)
	return;
if (gameStates.app.nSDLTicks - m_tRender < SPARK_FRAME_TIME)
	return;
if (++m_nFrame < 32) {
	m_tRender = gameStates.app.nSDLTicks; //+= SPARK_FRAME_TIME;
	if (gameOpts->render.effects.bMovingSparks)
		m_vPos += m_vDir;
	}
else {
	m_tRender = 0;
	m_tCreate = -1;
	}
}

//-----------------------------------------------------------------------------

void CEnergySpark::Render (void)
{
if (m_tRender) {
	if (m_nFrame > 31)
		m_tRender = 0;
	else
		transparencyRenderer.AddSpark (m_vPos, (char) m_nType, m_xSize, (char) m_nFrame);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CSparks::Create (void)
{
for (int i = 0; i < m_nMaxSparks; i++)
	m_sparks [i].Setup (m_nSegment, m_nType);
}

//-----------------------------------------------------------------------------

void CSparks::Setup (short nSegment, ubyte nType)
{
m_nMaxSparks = (ushort) (2 * SEGMENTS [nSegment].AvgRadf () + 0.5f);
if (!m_sparks.Create (m_nMaxSparks))
	m_nMaxSparks = 0;
else {
	m_bUpdate = 0;
	m_nSegment = nSegment;
	m_nType = nType;
	m_sparks.Clear ();
	for (int i = 0; i < m_nMaxSparks; i++)
		m_sparks [i].m_nProb = d_rand () % SPARK_MIN_PROB + 1;
	}
}

//-----------------------------------------------------------------------------

void CSparks::Render (void)
{
m_bUpdate = 1;
for (int i = 0; i < m_nMaxSparks; i++)
	m_sparks [i].Render ();
}

//-----------------------------------------------------------------------------

void CSparks::Update (void)
{
if (m_bUpdate) {
	for (int i = 0; i < m_nMaxSparks; i++)
		m_sparks [i].Update ();
	Create ();
	m_bUpdate = 0;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

inline int CSparkManager::Type (short nMatCen)
{
return SEGMENTS [m_segments [nMatCen]].m_nType == SEGMENT_IS_FUELCEN;
}

//-----------------------------------------------------------------------------

inline CSparks& CSparkManager::Sparks (short nMatCen)
{
return m_sparks [nMatCen];
}

//-----------------------------------------------------------------------------

void CSparkManager::SetupSparks (short nMatCen)
{
m_sparks [nMatCen].Setup (m_segments [nMatCen], Type (nMatCen));
}

//-----------------------------------------------------------------------------

void CSparkManager::UpdateSparks (short nMatCen)
{
m_sparks [nMatCen].Update ();
}

//-----------------------------------------------------------------------------

void CSparkManager::RenderSparks (short nMatCen)
{
m_sparks [nMatCen].Render ();
}

//-----------------------------------------------------------------------------

void CSparkManager::DestroySparks (short nMatCen)
{
m_sparks [nMatCen].Destroy ();
}

//-----------------------------------------------------------------------------

int CSparkManager::BuildSegList (void)
{
	CSegment*	segP = SEGMENTS.Buffer ();
	short			nSegment;

m_nSegments = 0;
for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, segP++)
	if ((segP->m_nType == SEGMENT_IS_FUELCEN) || (segP->m_nType == SEGMENT_IS_REPAIRCEN))
		m_segments [m_nSegments++] = nSegment;
return m_nSegments;
}


//-----------------------------------------------------------------------------

void CSparkManager::Init (void)
{
m_nSegments = 0;
}

//-----------------------------------------------------------------------------

void CSparkManager::Setup (void)
{
SEM_ENTER (SEM_SPARKS)
if ((gameStates.render.bHaveSparks = gameOpts->render.effects.bEnergySparks) && bmpSparks && (BuildSegList () > 0)) {
	for (short i = 0; i < m_nSegments; i++)
		SetupSparks (i);
	}	
SEM_LEAVE (SEM_SPARKS)
}

//-----------------------------------------------------------------------------

void CSparkManager::Render (void)
{
if (gameStates.render.bHaveSparks) {
	for (short i = 0; i < m_nSegments; i++)
		RenderSparks (i);
	}
}

//-----------------------------------------------------------------------------

void CSparkManager::Update (void)
{
if (gameStates.render.bHaveSparks) {
	for (short i = 0; i < m_nSegments; i++)
		UpdateSparks (i);
	}
}

//-----------------------------------------------------------------------------

void CSparkManager::Destroy (void)
{
SEM_ENTER (SEM_SPARKS)
if (gameStates.render.bHaveSparks) {
	for (int i = 0; i < m_nSegments; i++)
		DestroySparks (i);
	gameStates.render.bHaveSparks = 0;
	}
SEM_LEAVE (SEM_SPARKS)
}

//-----------------------------------------------------------------------------

void CSparkManager::DoFrame (void)
{
SEM_ENTER (SEM_SPARKS)
Update ();
SEM_LEAVE (SEM_SPARKS)
}

//-----------------------------------------------------------------------------
