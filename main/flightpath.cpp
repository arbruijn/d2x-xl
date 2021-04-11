#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"

//------------------------------------------------------------------------------

#define	PP_DELTAZ	-I2X(30)
#define	PP_DELTAY	I2X(10)

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

void CFlightPath::Update (CObject *pObj)
{
if (!pObj)
	return;

	time_t	t = SDL_GetTicks () - m_tUpdate;

if (m_nSize && ((m_tUpdate < 0) || (t >= m_tRefresh))) {
	m_tUpdate = t;
//	h = m_nEnd;
	m_nEnd = (m_nEnd + 1) % m_nSize;
	tPathPoint& p = m_path [m_nEnd];
	p.vOrgPos = pObj->info.position.vPos;
	p.vPos = pObj->info.position.vPos;
	p.mOrient = pObj->info.position.mOrient;
	p.vPos += pObj->info.position.mOrient.m.dir.f * 0;
	p.vPos += pObj->info.position.mOrient.m.dir.u * 0;
	p.bFlipped = false;
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
	CFixVector*	p = &m_path [m_nEnd].vPos;
	int32_t		i;

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

void CFlightPath::GetViewPoint (CFixVector* vPos)
{
	tPathPoint* p = GetPoint ();

if (!vPos)
	vPos = &gameData.renderData.mine.viewer.vPos;
if (!p)
	*vPos += gameData.objData.pViewer->info.position.mOrient.m.dir.f * PP_DELTAZ;
else {
	*vPos = p->vPos;
	int32_t nStage = LOCALOBJECT->AppearanceStage ();
	if (nStage) {
		float fDist = (nStage > 0)
						  ? Min (1.0f, 4.0f * LOCALOBJECT->AppearanceScale ())
						  : 1.0f; //1.0f - LOCALOBJECT->AppearanceScale ();
		if (!p->bFlipped) {
			p->bFlipped = true;
			p->mOrient.m.dir.f.Neg ();
			p->mOrient.m.dir.r.Neg ();
		}
		*vPos += p->mOrient.m.dir.f * fix (float (PP_DELTAZ) * fDist * 1.5f);
		*vPos += p->mOrient.m.dir.u * fix (float (PP_DELTAY) * fDist * 0.6666667f);
		}
	else {
		*vPos += p->mOrient.m.dir.f * (2 * PP_DELTAZ / 3);
		*vPos += p->mOrient.m.dir.u * (2 * PP_DELTAY / 3);
		}
	}
}

//------------------------------------------------------------------------------

CFlightPath::CFlightPath ()
{
if (m_path.Create (MAX_PATH_POINTS, "CFlightPath::m_path"))
	m_path.Clear (0);
m_posP = NULL;
m_nSize = 0;
m_nStart = 0;
m_nEnd = 0;
m_tRefresh = 0;
m_tUpdate = 0;
}

//------------------------------------------------------------------------------

CFlightPath::~CFlightPath ()
{
m_path.Destroy ();
}

//------------------------------------------------------------------------------
// eof
