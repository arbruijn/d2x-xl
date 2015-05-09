#ifndef __WAYPOINT_H
#define __WAYPOINT_H

class CWayPointManager {
	private:
		CArray<CObject*>		m_wayPoints;
		int32_t					m_nWayPoints;

	CObject* Find (int32_t nId);
	CObject* Target (CObject* pObj);
	int32_t Count (void);
	void Gather (void);
	void Remap (int32_t& nId);
	void Renumber (void);
	void LinkBack (void);
	void Attach (void);
	CObject* Current (CObject* pObj);
	CObject* Successor (CObject* pObj);
	bool Hop (CObject* pObj);
	bool Synchronize (CObject* obj);
	void Move (CObject* pObj);
	inline CObject* WayPoint (uint32_t i) { return m_wayPoints.IsIndex (i) ? m_wayPoints [i] : NULL; }

	public:
		CWayPointManager () : m_nWayPoints (0)
			{}
		bool Setup (bool bAttach = true);
		void Update (void);
		void Destroy (void);
	};

extern CWayPointManager wayPointManager;

#endif //__WAYPOINT_H