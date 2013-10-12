#ifndef __SIXENSE_WRAPPER__
#define __SIXENSE_WRAPPER__

#if DBG

#include "sixense.h"
#include "vecmat.h"

// -----------------------------------------------------------------------------

class CSixense {
	private:
		sixenseAllControllerData	m_data;
		bool								m_bAvailable;
		int								m_nBases;
		int								m_nActive;
		int								m_nAxis;
		fix*								m_axis;

	public:
		CSixense ();
		~CSixense ();

		inline bool Available (void) { return m_bAvailable && (m_nBases > 0) && (QueryActiveBase () >= 0); }
		int QueryAxis (void);
		bool QueryAngles (CFloatVector* v);
		inline fix GetAxis (int nAxis) { return (nAxis > m_nAxis) ? 0 : m_axis [nAxis]; }

	private:
		bool QueryAngles (ubyte nController, CFloatVector& v);
		bool QueryAnglesInternal (ubyte nController, CFloatVector& v);
		int QueryActiveBase (void);
	};

extern CSixense sixense;

#endif

// -----------------------------------------------------------------------------

#endif //__SIXENSE_WRAPPER__