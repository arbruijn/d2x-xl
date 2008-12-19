#ifndef _SPARKEFFECT_H
#define _SPARKEFFECT_H

class CEnergySpark {
	public:
		short				m_nProb;
		char				m_nFrame;
		ubyte				m_bRendered :1;
		ubyte				m_nType :1;
		fix				m_xSize;
		time_t			m_tRender;
		time_t			m_tCreate;
		CFixVector		m_vPos;
		CFixVector		m_vDir;

	public:
		void Setup (short nSegment, ubyte nType);
		void Update (void);
		void Render (void);
	};

class CSparks {
	public:
		CArray<CEnergySpark>	m_sparks;
		short						m_nSegment;
		short						m_nMaxSparks;
		ubyte						m_nType;
		ubyte						m_bUpdate;

	public:
		CSparks () { Init (); }
		~CSparks () { Destroy (); }
		void Init (void) {
			m_nMaxSparks = 0;
			m_bUpdate = 0;
			m_nSegment = -1;
			}
		void Destroy (void) {
			m_sparks.Destroy ();
			Init ();
			}
		void Setup (short nSegment, ubyte nType);
		void Create (void);
		void Render (void);
		void Update (void);
	};


class CSparkManager {
	private:
		CSparks		m_sparks [2 * MAX_FUEL_CENTERS];	//0: repair, 1: fuel center
		short			m_segments [2 * MAX_FUEL_CENTERS];
		short			m_nSegments;

	public:
		CSparkManager () { Init (); }
		~CSparkManager () { Destroy (); }
		void Init (void);
		void Setup (void);
		void Render (void);
		void Update (void);
		void Destroy (void);
		void Create (void);
		void DoFrame (void);

	private:
		inline int Type (short nMatCen);
		inline CSparks& Sparks (short nMatCen);
		int BuildSegList (void);
		void SetupSparks (short nMatCen);
		void UpdateSparks (short nMatCen);
		void RenderSparks (short nMatCen);
		void DestroySparks (short nMatCen);
	};

extern CSparkManager sparkManager;

#endif //_SPARKEFFECT_H
