#ifndef _LIGHTCLUSTER_H
#define _LIGHTCLUSTER_H

#include "carray.h"

//--------------------------------------------------------------------------

class CLightObjId {
	public:
		int16_t	nObject;
		int32_t	nSignature;
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
		int32_t Add (int16_t nObject, CFloatVector *color, fix xObjIntensity);
		void Add (int16_t nObject, int16_t nLightObj);
		void AddForAI (CObject *objP, int16_t nObject, int16_t nShot);
		void Delete (int16_t nObject);
		int16_t Create (CObject *objP);
		inline bool Use (void) { return m_bUse; }
		inline void SetUsage (bool bUse) { m_bUse = bUse; }
		inline CLightObjId& Object (uint32_t i) { return m_objects [i]; }
		void Set (void);
};

extern CLightClusterManager lightClusterManager;

//--------------------------------------------------------------------------

#endif //_LIGHTCLUSTER_H

//eof
