#ifndef __glow_h
#define __glow_h

//------------------------------------------------------------------------------

class CGlowRenderer {
	private:
		GLhandleARB m_shaderProg;
		CFloatVector	m_vMin, m_vMax;
		int m_x, m_y, m_w, m_h;

	public:
		void InitShader (void);
		bool ShaderActive (void);
		void End (bool bReplace = true);
		void Begin (CFloatVector* vertexP, int nVerts);
		void Begin (CFloatVector* pos, float radius);
		CGlowRenderer () : m_shaderProg (0) {}

	private:
		bool LoadShader (int const direction, float const radius);
		void Render (int const source, int const direction = -1, float const radius = 1.0f);
		bool Blur (int const direction);
		void Project (CFloatVector& v);
		void Activate (void);
	};

extern CGlowRenderer glowRenderer;

//------------------------------------------------------------------------------

#endif //__glow_h
