#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#define PP_EFFECT_SHOCKWAVE	1

//------------------------------------------------------------------------------

class CPostEffect {
	private:
		class CPostEffect*	m_prev;
		class CPostEffect*	m_next;
		int32_t					m_nType;

	protected:
		bool						m_bValid;

	public:
		CPostEffect (int32_t nType = 0) : 
			m_prev (0), m_next (0), m_nType (nType), m_bValid (false)
			{}

		virtual ~CPostEffect () {}

		inline CPostEffect* Prev (void) { return m_prev; }

		inline CPostEffect* Next (void) { return m_next; }

		inline void Setup (int32_t nType) { m_nType = nType; }

		inline void Link (CPostEffect* prev, CPostEffect* next) {
			m_prev = prev, m_next = next;
			}

		inline void Insert (CPostEffect* next) { 
			if (next)
				next->Link (this, m_next);
			if (m_next)
				m_next->Link (next, m_next->Next ());
			m_next = next;
			}

		inline void Unlink (void) {
			if (m_prev)
				m_prev->Link (m_prev->Prev (), m_next);
			if (m_next)
				m_next->Link (m_prev, m_next->Next ());
			m_prev = m_next = 0;
			}

		virtual bool Terminate (void) = 0;

		virtual bool Setup (void) = 0;

		virtual bool Update (void) = 0;

		virtual void Render (void) = 0;

		virtual bool Enabled (void) = 0;

		virtual float Life (void) = 0;

		inline bool Valid (void) { return m_bValid; }
};

//------------------------------------------------------------------------------

class CPostEffectShockwave : public CPostEffect {
	private:
		uint32_t			m_nStart;
		uint32_t			m_nLife;
		int32_t			m_nSize;
		int32_t			m_nBias;
		CFixVector		m_pos;
		int32_t			m_nObject;

		CFloatVector3	m_renderPos;
		float				m_rad;
		float				m_screenRad;
		float				m_ttl;

	public:
		static bool m_bRendered;
		static int32_t m_nShockwaves;
		static GLhandleARB m_shaderProg;

	public:
		CPostEffectShockwave (int32_t nStart = 0, int32_t nLife = 0, int32_t nSize = 0, int32_t nBias = 1, CFixVector pos = CFixVector::ZERO, int32_t nObject = -1) :
			CPostEffect (PP_EFFECT_SHOCKWAVE), 
			m_nStart (nStart), m_nLife (int32_t (1000 * X2F (nLife))), m_nSize (nSize), m_nBias (nBias), m_pos (pos), m_nObject (nObject), m_rad (0.0f), m_screenRad (0.0f), m_ttl (0.0f)
			{ m_pos = pos; }

		void Setup (int32_t nStart, int32_t nLife, int32_t nSize, int32_t nBias, CFixVector pos, int32_t nObject) {
			CPostEffect::Setup (PP_EFFECT_SHOCKWAVE);
			m_nStart = nStart, m_nLife = nLife, m_nSize = nSize, m_nBias = nBias, m_pos = pos, m_nObject = nObject;
			}

		virtual bool Terminate (void) { return SDL_GetTicks () - m_nStart >= (uint32_t) Life (); }

		virtual bool Setup (void);

		virtual bool Update (void);

		virtual void Render (void);

		virtual bool Enabled (void);

		virtual float Life (void);

	private:
		void InitShader (void);

		bool SetupShader (void);
	};

//------------------------------------------------------------------------------

class CPostProcessManager {
	private:
		int32_t			m_nEffects;
		CPostEffect*	m_effects;

	public:
		CPostProcessManager () { Init (); }

		~CPostProcessManager () { Destroy (); }

		inline void Init (void) { 
			m_nEffects = 0;
			m_effects = NULL;
			}

		void Destroy (void);

		void Add (CPostEffect* e);

		void Update (void);

		bool Prepare (void);

		bool Setup (void);

		void Render (void);

		bool HaveEffects (void);

		inline CPostEffect* Effects (void) { return m_effects; }
	};

extern CPostProcessManager postProcessManager;

//------------------------------------------------------------------------------

void InitFogShader (void);
void RenderFog (void);

//------------------------------------------------------------------------------

#endif //POSTPROCESS_H
