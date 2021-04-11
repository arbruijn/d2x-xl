#ifdef USE_SIXENSE

#include "sixense_wrapper.h"

CSixense sixense;

// -----------------------------------------------------------------------------

CSixense::CSixense ()
	 : m_bAvailable (false), m_nBases (0), m_nActive (-1), m_axis (NULL)
{
if ((m_bAvailable = (sixenseInit () == SIXENSE_SUCCESS))) {
	m_nBases = sixenseGetMaxBases ();
	if (m_nBases)
		m_axis = NEW fix [m_nBases * 2 * 3];
	}
}

// -----------------------------------------------------------------------------

CSixense::~CSixense ()
{
if (m_axis) {
	delete m_axis;
	m_axis = NULL;
	}
sixenseExit ();
}

// -----------------------------------------------------------------------------

int32_t CSixense::QueryActiveBase (void)
{
for (int32_t i = m_nActive + 1; i < m_nBases; i++) {
	if (!sixenseIsBaseConnected (i))
		continue;
	if (sixenseSetActiveBase (m_nActive) == SIXENSE_FAILURE)
		continue;
	for (int32_t i = sixenseGetNumActiveControllers (); i; )
		if (sixenseIsControllerEnabled (--i))
			return m_nActive = i;
	}
return m_nActive = -1;
}

// -----------------------------------------------------------------------------

bool CSixense::QueryAngles (uint8_t nController, CFloatVector& v)
{
if ((m_nActive < 0) || (sixenseSetActiveBase (m_nActive) == SIXENSE_FAILURE))
	return false;
if (nController >= sixenseGetNumActiveControllers ())
	return false;
if (!sixenseIsControllerEnabled (nController))
	return false;
return QueryAnglesInternal (nController, v);
}

// -----------------------------------------------------------------------------

bool CSixense::QueryAnglesInternal (uint8_t nController, CFloatVector& v)
{
if (sixenseGetNewestData (nController, m_data.controllers + nController) == SIXENSE_FAILURE)
	return false;

CFloatMatrix m;

for (int32_t i = 0; i < 3; i++)
	for (int32_t j = 0; j < 3; j++)
		m [4 * i + j] = m_data.controllers [nController].rot_mat [i][j];

v = m.ComputeAngles ();
return true;
}

// -----------------------------------------------------------------------------

int32_t CSixense::QueryAxis (void)
{
if (!m_axis)
	return 0;

m_nActive = -1;
int32_t m_nAxis = 0;
while (QueryActiveBase () >= 0) {
	CFloatVector v;
	int32_t n = sixenseGetNumActiveControllers ();
	for (uint8_t i = 0; i < n; i++) {
		if (QueryAnglesInternal (i, v)) {
			for (int32_t j = 0; j < 3; j++)
				m_axis [m_nAxis++] = F2X (v [j]);
			}
		}	
	}
return m_nAxis;
}

// -----------------------------------------------------------------------------

#endif //USE_SIXENSE
