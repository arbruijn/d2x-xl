#include "sixense_wrapper.h"

CSixense sixense;

// -----------------------------------------------------------------------------

CSixense::CSixense ()
	 : m_bAvailable (false), m_nBases (0), m_nActive (-1), m_axis (NULL)
{
if ((m_bAvailable = (sixenseInit () == SIXENSE_SUCCESS))) {
	m_nBases = sixenseGetMaxBases ();
	if (m_nBases)
		m_axis = new fix [m_nBases * 2 * 3];
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

int CSixense::QueryActiveBase (void)
{
for (int i = m_nActive + 1; i < m_nBases; i++) {
	if (!sixenseIsBaseConnected (i))
		continue;
	if (sixenseSetActiveBase (m_nActive) == SIXENSE_FAILURE)
		continue;
	for (int i = sixenseGetNumActiveControllers (); i; )
		if (sixenseIsControllerEnabled (--i))
			return m_nActive = i;
	}
return m_nActive = -1;
}

// -----------------------------------------------------------------------------

bool CSixense::QueryAngles (ubyte nController, CFloatVector& v)
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

bool CSixense::QueryAnglesInternal (ubyte nController, CFloatVector& v)
{
if (sixenseGetNewestData (nController, m_data.controllers + nController) == SIXENSE_FAILURE)
	return false;

CFloatMatrix m;

for (int i = 0; i < 3; i++)
	for (int j = 0; j < 3; j++)
		m [4 * i + j] = m_data.controllers [nController].rot_mat [i][j];

v = m.ComputeAngles ();
return true;
}

// -----------------------------------------------------------------------------

int CSixense::QueryAxis (void)
{
if (!m_axis)
	return 0;

m_nActive = -1;
int m_nAxis = 0;
while (QueryActiveBase ()) {
	CFloatVector v;
	int n = sixenseGetNumActiveControllers ();
	for (ubyte i = 0; i < n; i++) {
		if (QueryAnglesInternal (i, v)) {
			for (int j = 0; j < 3; j++)
				m_axis [m_nAxis++] = F2X (v [j]);
			}
		}	
	}
return m_nAxis;
}

// -----------------------------------------------------------------------------
