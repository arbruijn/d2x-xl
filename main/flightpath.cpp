#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"

//------------------------------------------------------------------------------

#define	PP_DELTAZ	-I2X(30)
#define	PP_DELTAY	I2X(10)

CFlightPath	externalView;

//------------------------------------------------------------------------------

void CFlightPath::Reset (int32_t nSize, int32_t nFPS)
{
m_nSize = (nSize < 0) ? MAX_PATH_POINTS : nSize;
m_tRefresh = (time_t) (1000 / ((nFPS < 0) ? 40 : nFPS));
m_nStart =
m_nEnd = 0;
m_posP = NULL;
m_tUpdate = -1;
}

//------------------------------------------------------------------------------

void CFlightPath::SetPoint (CObject *objP)
{
	time_t	t = SDL_GetTicks () - m_tUpdate;

if (m_nSize && ((m_tUpdate < 0) || (t >= m_tRefresh))) {
	m_tUpdate = t;
//	h = m_nEnd;
	m_nEnd = (m_nEnd + 1) % m_nSize;
	m_path [m_nEnd].vOrgPos = objP->info.position.vPos;
	m_path [m_nEnd].vPos = objP->info.position.vPos;
	m_path [m_nEnd].mOrient = objP->info.position.mOrient;
	m_path [m_nEnd].vPos += objP->info.position.mOrient.m.dir.f * 0;
	m_path [m_nEnd].vPos += objP->info.position.mOrient.m.dir.u * 0;
//	if (!memcmp (m_path + h, m_path + m_nEnd, sizeof (tMovementPath)))
//		m_nEnd = h;
//	else
	if (m_nEnd == m_nStart)
		m_nStart = (m_nStart + 1) % m_nSize;
	}
}

//------------------------------------------------------------------------------

tPathPoint* CFlightPath::GetPoint (void)
{
	CFixVector		*p = &m_path [m_nEnd].vPos;
	int32_t				i;

if (m_nStart == m_nEnd) {
	m_posP = NULL;
	return NULL;
	}
i = m_nEnd;
do {
	if (!i)
		i = m_nSize;
	i--;
	if (CFixVector::Dist(m_path [i].vPos, *p) >= I2X (15))
		break;
	}
while (i != m_nStart);
return m_posP = m_path + i;
}

//------------------------------------------------------------------------------

void CFlightPath::GetViewPoint (void)
{
	tPathPoint* p = GetPoint ();

if (!p)
	gameData.render.mine.viewer.vPos += gameData.objs.viewerP->info.position.mOrient.m.dir.f * PP_DELTAZ;
else {
	gameData.render.mine.viewer.vPos = p->vPos;
	gameData.render.mine.viewer.vPos += p->mOrient.m.dir.f * (PP_DELTAZ * 2 / 3);
	gameData.render.mine.viewer.vPos += p->mOrient.m.dir.u * (PP_DELTAY * 2 / 3);
	}
}

//------------------------------------------------------------------------------

CFlightPath::CFlightPath ()
{
if (m_path.Create (MAX_PATH_POINTS))
	m_path.Clear (0);
m_posP = NULL;
m_nSize = 0;
m_nStart = 0;
m_nEnd = 0;
m_tRefresh = 0;
m_tUpdate = 0;
}

//------------------------------------------------------------------------------
// eof
