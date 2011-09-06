#ifndef __WAYPOINT_H
#define __WAYPOINT_H

class CWayPointManager {
	private:
		CArray<CObject*>		m_wayPoints;
		int						m_nWayPoints;

	CObject* Find (int nId);
	int Count (void);
	void Gather (void);
	void Remap (int& nId);
	void Renumber (void);
	void LinkBack (void);
	void Attach (void);
	CObject* Current (CObject* objP);
	CObject* Successor (CObject* objP);
	bool Hop (CObject* objP);
	void Move (CObject* objP);

	public:
		CWayPointManager () : m_nWayPoints (0)
			{}
		bool Setup (bool bAttach = true);
		void Update (void);
		void Destroy (void);
	};

extern CWayPointManager wayPointManager;

#endif //__WAYPOINT_H