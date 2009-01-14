#ifndef _LIGHTCLUSTER_H
#define _LIGHTCLUSTER_H

#include "carray.h"

//--------------------------------------------------------------------------

class CLightObjId {
	public:
		short	nObject;
		int	nSignature;
	};

class CLightClusterManager {
	private:	
		bool						m_bUse;
		CArray<CLightObjId>	m_objects;

	public:
		CLightClusterManager ();
		~CLightClusterManager () { Destroy (); }
		bool Init (void);
		void Destroy (void);
		void Reset (void);
		int Add (short nObject, tRgbaColorf *color, fix xObjIntensity);
		void AddForAI (CObject *objP, short nObject, short nShot);
		short Create (CObject *objP);
		inline bool Use (void) { return m_bUse; }
		inline void SetUsage (bool bUse) { m_bUse = bUse; }
		inline CLightObjId& Object (uint i) { return m_objects [i]; }
		void Set (void);
};

extern CLightClusterManager lightClusterManager;

//--------------------------------------------------------------------------

#endif //_LIGHTCLUSTER_H

//eof
