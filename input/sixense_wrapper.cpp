#include "sixense_wrapper.h"

// -----------------------------------------------------------------------------

CSixense::CSixense ()
	 : m_bAvailable (false), m_nBases (0), m_nActive (-1), m_axis (NULL)
{
if ((m_bAvailable = (sixenseInit () == SIXENSE_SUCCESS))) {
	m_nBases = sixenseGetMaxBases ();
	if (m_nBases)
		m_axis = new fix [m_nBases];
	}
}

// -----------------------------------------------------------------------------

CSixense::~CSixense ()
{
if (m_axis)
	delete m_axis;
sixenseExit ();
}

// -----------------------------------------------------------------------------

int CSixense::GetActiveBase (void)
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

bool CSixense::GetAngles (ubyte nController, CFloatVector& v)
{
if ((m_nActive < 0) || (sixenseSetActiveBase (m_nActive) == SIXENSE_FAILURE))
	return false;
if (nController >= sixenseGetNumActiveControllers ())
	return false;
if (!sixenseIsControllerEnabled (nController))
	return false;
return GetAnglesInternal (nController, v);
}

// -----------------------------------------------------------------------------

bool CSixense::GetAnglesInternal (ubyte nController, CFloatVector& v)
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

int CSixense::GetAxis (void)
{
if (!m_axis)
	return 0;

m_nActive = -1;
int m_nAxis = 0;
while (GetActiveBase ()) {
	CFloatVector v;
	int n = sixenseGetNumActiveControllers ();
	for (ubyte i = 0; i < n; i++) {
		if (GetAnglesInternal (i, v)) {
			for (int j = 0; j < 3; j++)
				m_axis [m_nAxis++] = F2X (v [j]);
			}
		}	
	}
return m_nAxis;
}

// -----------------------------------------------------------------------------
