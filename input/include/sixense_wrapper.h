#ifndef __SIXENSE_WRAPPER__
#define __SIXENSE_WRAPPER__

#ifndef USE_SIXENSE
//#	error SIXENSE is unavailable in this project configuration
#else

#include "sixense.h"
#include "vecmat.h"

// -----------------------------------------------------------------------------

class CSixense {
	private:
		sixenseAllControllerData	m_data;
		bool								m_bAvailable;
		int32_t								m_nBases;
		int32_t								m_nActive;
		int32_t								m_nAxis;
		fix*								m_axis;

	public:
		CSixense ();
		~CSixense ();

		inline bool Available (void) { return m_bAvailable && (m_nBases > 0) && (QueryActiveBase () >= 0); }
		int32_t QueryAxis (void);
		bool QueryAngles (CFloatVector* v);
		inline fix GetAxis (int32_t nAxis) { return (nAxis > m_nAxis) ? 0 : m_axis [nAxis]; }

	private:
		bool QueryAngles (uint8_t nController, CFloatVector& v);
		bool QueryAnglesInternal (uint8_t nController, CFloatVector& v);
		int32_t QueryActiveBase (void);
	};

extern CSixense sixense;

#	endif
#endif

// -----------------------------------------------------------------------------

#endif //__SIXENSE_WRAPPER__
