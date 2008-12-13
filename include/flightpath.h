/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _FLIGHTPATH_H
#define _FLIGHTPATH_H

#include "inferno.h"

#define MAX_PATH_POINTS		20

typedef struct tPathPoint {
	CFixVector			vPos;
	CFixVector			vOrgPos;
	CFixMatrix			mOrient;
} tPathPoint;

class CFlightPath {
	public:
		CArray<tPathPoint>	m_path; // [MAX_PATH_POINTS];
		tPathPoint*				m_posP;
		int						m_nSize;
		int						m_nStart;
		int						m_nEnd;
		time_t					m_tRefresh;
		time_t					m_tUpdate;
	public:
		CFlightPath ();
		void SetPoint (CObject *objP);
		tPathPoint *GetPoint (void);
		void GetViewPoint (void);
		void Reset (int nSize, int nFPS);
		tPathPoint* GetPos (void) { return m_posP; }
		inline tPathPoint* Pos (void) { return m_posP; }
		inline void SetPos (tPathPoint *posP) { m_posP = posP; }
};

extern CFlightPath externalView;

#endif /* _FLIGHTPATH_H */
