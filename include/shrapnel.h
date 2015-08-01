#ifndef _SHRAPNEL_H
#define _SHRAPNEL_H

#include "descent.h"

//------------------------------------------------------------------------------

typedef struct tShrapnel {
	CFixVector	vPos, vStartPos;
	CFixVector	vDir;
	CFixVector	vOffs;
	fix			xSpeed;
	fix			xBaseSpeed;
	fix			xLife;
	fix			xTTL;
	int32_t		nSmoke;
	int32_t		nTurn;
	int32_t		nParts;
	int32_t		d;
	double		fScale;
	time_t		tUpdate;
} tShrapnel;

class CShrapnel {
	private:
		tShrapnel	m_info;

	public:
		void Create (CObject* parentObjP, CObject* objP, float fScale);
		void Destroy (void);
		void Move (void);
		void Draw (void);
		int32_t Update (void);
		inline fix TTL (void) { return m_info.xTTL; }
};

class CShrapnelCloud : private CStack<CShrapnel> {
	public:
		~CShrapnelCloud () { Destroy (); }
		int32_t Create (CObject* parentObjP, CObject* objP);
		uint32_t Update (void);
		void Draw (void);
		void Destroy (void);
	};

class CShrapnelManager : private CArray<CShrapnelCloud> {
	public:
		bool Init (void);
		void Reset (void);
		int32_t Create (CObject *objP);
		void Draw (CObject *objP);
		int32_t Update (CObject *objP);
		void Move (CObject *objP);
		void Destroy (CObject *objP); 
		void DoFrame (void);
	};

extern CShrapnelManager shrapnelManager;

//------------------------------------------------------------------------------

#endif //_SHRAPNEL_H
//eof
