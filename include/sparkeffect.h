#ifndef _SPARKEFFECT_H
#define _SPARKEFFECT_H

class CEnergySpark {
	public:
		int16_t				m_nProb;
		char				m_nFrame;
		char				m_nRotFrame;
		uint8_t				m_bRendered :1;
		uint8_t				m_nType :1;
		uint8_t				m_nOrient :1;
		fix				m_xSize;
		time_t			m_tRender;
		time_t			m_tCreate;
		CFixVector		m_vPos;
		CFixVector		m_vDir;

	public:
		void Setup (int16_t nSegment, uint8_t nType);
		void Update (void);
		void Render (void);
	};

class CSparks {
	public:
		CArray<CEnergySpark>	m_sparks;
		int16_t						m_nSegment;
		int16_t						m_nMaxSparks;
		uint8_t						m_nType;
		uint8_t						m_bUpdate;

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
		void Setup (int16_t nSegment, uint8_t nType);
		void Create (void);
		void Render (void);
		void Update (void);
	};


class CSparkManager {
	private:
		CSparks		m_sparks [2 * MAX_FUEL_CENTERS];	//0: repair, 1: fuel center
		int16_t			m_segments [2 * MAX_FUEL_CENTERS];
		int16_t			m_nSegments;

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
		inline bool HaveSparks (void) { return m_nSegments > 0; }

	private:
		inline int32_t Type (int16_t nObjProducer);
		inline CSparks& Sparks (int16_t nObjProducer);
		int32_t BuildSegList (void);
		void SetupSparks (int16_t nObjProducer);
		void UpdateSparks (int16_t nObjProducer);
		void RenderSparks (int16_t nObjProducer);
		void DestroySparks (int16_t nObjProducer);
	};

extern CSparkManager sparkManager;

#endif //_SPARKEFFECT_H
