#ifndef __glow_h
#define __glow_h

//------------------------------------------------------------------------------

class CGlowRenderer {
	private:
		GLhandleARB m_shaderProg;
		tScreenPos m_screenMin, m_screenMax;
		int m_x, m_y, m_w, m_h;
		int m_nStrength;
		bool m_bReplace;
		bool m_bViewPort;
		float m_brightness;

	public:
		bool Available (bool bForce = false);
		void InitShader (void);
		bool ShaderActive (void);
		bool End (void);
		bool Begin (int const nStrength = 1, bool const bReplace = true, float const brightness = 1.0f);
		void ViewPort (CFloatVector3* vertexP, int nVerts);
		void ViewPort (CFloatVector* vertexP, int nVerts);
		void ViewPort (CFixVector pos, float radius);
		void ViewPort (CFloatVector3 pos, float width, float height);
		CGlowRenderer () : m_shaderProg (0), m_nStrength (-1), m_bReplace (true), m_bViewPort (false), m_brightness (1.1f) {}

	private:
		bool LoadShader (int const direction, float const radius);
		void Render (int const source, int const direction = -1, float const radius = 1.0f);
		bool Blur (int const direction);
		void Activate (void);
		void SetupProjection (void);
		void SetExtent (CFloatVector3 v, bool bTransformed = false);
		void InitViewPort (void);
	};

extern CGlowRenderer glowRenderer;

//------------------------------------------------------------------------------

#endif //__glow_h
