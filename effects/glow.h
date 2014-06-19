#ifndef __glow_h
#define __glow_h

//------------------------------------------------------------------------------

class CGlowRenderer {
	private:
		GLhandleARB m_shaderProg;
		tScreenPos m_renderMin, m_renderMax;
		tScreenPos m_itemMin, m_itemMax;
		int m_x, m_y, m_w, m_h;
		int m_nType;
		int m_nStrength;
		bool m_bReplace;
		int m_bViewport;
		float m_brightness;

	public:
		bool Available (int const nType, bool bForce = false);
		void InitShader (void);
		bool ShaderActive (void);
		void Done (const int nType);
		bool End (void);
		bool Begin (int const nType, int const nStrength = 1, bool const bReplace = true, float const brightness = 1.0f);
		bool SetViewport (int const nType, CFloatVector3* vertexP, int nVerts);
		bool SetViewport (int const nType, CFloatVector* vertexP, int nVerts);
		bool SetViewport (int const nType, CFixVector pos, float radius);
		bool SetViewport (int const nType, CFloatVector3 pos, float width, float height, bool bTransformed = false);
		bool Visible (void);
		CGlowRenderer () : m_shaderProg (0), m_nType (-1), m_nStrength (-1), m_bReplace (true), m_bViewport (0), m_brightness (1.1f) {}

	private:
		bool LoadShader (int const direction, float const radius);
		void Render (int const source, int const direction = -1, float const radius = 1.0f, bool const bClear = false);
		bool Blur (int const direction);
		int Activate (void);
		void SetupProjection (void);
		void SetItemExtent (CFloatVector3 v, bool bTransformed = false);
		void InitViewport (void);
		void ClearViewport (float const radius);
		void ChooseDrawBuffer (void);
		bool Reset (int bGlow, int bOgl = 0);
		bool UseViewport (void);

	};

extern CGlowRenderer glowRenderer;

//------------------------------------------------------------------------------

#define GLOW_LIGHTNING		1
#define GLOW_SHIELDS			2
#define GLOW_SPRITES			4
#define GLOW_THRUSTERS		8
#define GLOW_LIGHTTRAILS	16
#define GLOW_POLYS			32
#define GLOW_FACES			64
#define GLOW_OBJECTS			128
#define GLOW_HEADLIGHT		256
#define BLUR_SHADOW			1024

#if 0 //DBG
#	define GLOW_FLAGS (GLOW_SHIELDS)
#else
#	define GLOW_FLAGS (GLOW_LIGHTNING | GLOW_SHIELDS | GLOW_SPRITES | GLOW_THRUSTERS | GLOW_HEADLIGHT | BLUR_SHADOW)
#endif

//------------------------------------------------------------------------------

#endif //__glow_h
