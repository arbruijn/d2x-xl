#ifndef __SIXENSE_WRAPPER__
#define __SIXENSE_WRAPPER__

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

		inline bool Available (void) { return m_bAvailable && (m_nBases > 0) && (GetActiveBase () >= 0); }
		int GetAxis (void);

	private:
		bool GetAngles (ubyte nController, CFloatVector& v);
		bool GetAnglesInternal (ubyte nController, CFloatVector& v);
		int GetActiveBase (void);
	};

extern CSixense sixense;

// -----------------------------------------------------------------------------

#endif //__SIXENSE_WRAPPER__