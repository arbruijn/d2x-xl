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
#include "loadgamedata.h"
#include "textures.h"
#include "ogl_defs.h"
#include "maths.h"
#include "vecmat.h"
#include "hudmsgs.h"
#include "segmath.h"
#include "piggy.h"
#include "renderlib.h"
#include "sparkeffect.h"
#include "transprender.h"
#include "automap.h"
#include "addon_bitmaps.h"

#define SPARK_MIN_PROB		16
#define SPARK_FRAME_TIME	50

CSparkManager sparkManager;

//-----------------------------------------------------------------------------

void CEnergySpark::Setup (short nSegment, ubyte nType)
{
	CSegment*			segP = SEGMENTS + nSegment;
	CFixVector			vOffs;
	CFloatVector		vRadf;

vRadf.Assign (segP->m_extents [1] - segP->m_extents [0]);
vRadf *= 0.5f;
if (m_tRender)
	return;
if (!m_nProb)
	m_nProb = SPARK_MIN_PROB;
if (gameStates.app.nSDLTicks [0] - m_tCreate < SPARK_FRAME_TIME)
	return;
m_nType = nType;
m_tCreate = gameStates.app.nSDLTicks [0];
if (RandShort () % m_nProb)
	m_nProb--;
else {
	vOffs.v.coord.x = F2X (vRadf.v.coord.x - (2 * RandFloat ()) * vRadf.v.coord.x);
	vOffs.v.coord.y = F2X (vRadf.v.coord.y - (2 * RandFloat ()) * vRadf.v.coord.y);
	vOffs.v.coord.z = F2X (vRadf.v.coord.z - (2 * RandFloat ()) * vRadf.v.coord.z);
	m_vPos = segP->Center () + vOffs;
	if ((vOffs.Mag () > segP->MinRad ()) && segP->Masks (m_vPos, 0).m_center)
		m_nProb = 1;
	else {
		m_xSize = I2X (1) + 4 * RandShort ();
		m_nFrame = 0;
		m_nRotFrame = rand () % 64;
		m_nOrient = rand () % 2;
		m_tRender = -1;
		m_bRendered = 0;
		m_nProb = SPARK_MIN_PROB;
		if (gameOpts->render.effects.bMovingSparks) {
			m_vDir.v.coord.x = (I2X (1) / 4) - RandShort ();
			m_vDir.v.coord.y = (I2X (1) / 4) - RandShort ();
			m_vDir.v.coord.z = (I2X (1) / 4) - RandShort ();
			CFixVector::Normalize (m_vDir);
			m_vDir *= ((I2X (1) / (8 + RandShort () % 8)));
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
if (gameStates.app.nSDLTicks [0] - m_tRender < SPARK_FRAME_TIME)
	return;
if (++m_nFrame < 32) {
	m_tRender = gameStates.app.nSDLTicks [0]; //+= SPARK_FRAME_TIME;
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
		transparencyRenderer.AddSpark (m_vPos, (char) m_nType, m_xSize, (char) m_nFrame, (char) m_nRotFrame, (char) m_nOrient);
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
		m_sparks [i].m_nProb = RandShort () % SPARK_MIN_PROB + 1;
	}
}

//-----------------------------------------------------------------------------

void CSparks::Render (void)
{
m_bUpdate = 1;
//if (!automap.Display () || automap.m_bFull || automap.m_visible [m_nSegment])
if (gameData.render.mine.Visible (m_nSegment)) {
//#if USE_OPENMP > 1
//#	pragma omp parallel for
//#endif
	for (int i = 0; i < m_nMaxSparks; i++)
		m_sparks [i].Render ();
	}
}

//-----------------------------------------------------------------------------

void CSparks::Update (void)
{
if (m_bUpdate) {
#if USE_OPENMP //> 1
#	pragma omp parallel for
#endif
	for (int i = 0; i < m_nMaxSparks; i++)
		m_sparks [i].Update ();
	Create ();
	m_bUpdate = 0;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

inline int CSparkManager::Type (short nObjProducer)
{
return SEGMENTS [m_segments [nObjProducer]].m_function == SEGMENT_FUNC_PRODUCERTER;
}

//-----------------------------------------------------------------------------

inline CSparks& CSparkManager::Sparks (short nObjProducer)
{
return m_sparks [nObjProducer];
}

//-----------------------------------------------------------------------------

void CSparkManager::SetupSparks (short nObjProducer)
{
m_sparks [nObjProducer].Setup (m_segments [nObjProducer], Type (nObjProducer));
}

//-----------------------------------------------------------------------------

void CSparkManager::UpdateSparks (short nObjProducer)
{
m_sparks [nObjProducer].Update ();
}

//-----------------------------------------------------------------------------

void CSparkManager::RenderSparks (short nObjProducer)
{
m_sparks [nObjProducer].Render ();
}

//-----------------------------------------------------------------------------

void CSparkManager::DestroySparks (short nObjProducer)
{
m_sparks [nObjProducer].Destroy ();
}

//-----------------------------------------------------------------------------

int CSparkManager::BuildSegList (void)
{
	CSegment*	segP = SEGMENTS.Buffer ();
	short			nSegment;

m_nSegments = 0;
for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, segP++)
	if ((segP->m_function == SEGMENT_FUNC_PRODUCERTER) || (segP->m_function == SEGMENT_FUNC_REPAIRCENTER))
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
if ((gameStates.render.bHaveSparks = (gameOpts->render.effects.bEnabled && gameOpts->render.effects.bEnergySparks)) && sparks.Bitmap () && (BuildSegList () > 0)) {
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
if ((gameStates.render.bHaveSparks = (gameOpts->render.effects.bEnabled && gameOpts->render.effects.bEnergySparks))) {
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
