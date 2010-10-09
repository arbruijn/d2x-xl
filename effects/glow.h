#ifndef __glow_h
#define __glow_h

//------------------------------------------------------------------------------

class CGlowRenderer {
	private:
		GLhandleARB m_shaderProg;
		CFloatVector	m_vMin, m_vMax;

	public:
		void Reset (void);
		void InitShader (void);
		bool ShaderActive (void);
		void Flush (bool bReplace = true);
		void SetExtent (CFloatVector* vertexP, int nVerts);
		CGlowRenderer () : m_shaderProg (0) {}

	private:
		bool LoadShader (int const direction, float const radius);
		void Render (int const source, int const direction = -1, float const radius = 1.0f);
		bool Blur (int const direction);
		void Project (CFloatVector& v);
	};

extern CGlowRenderer glowRenderer;

//------------------------------------------------------------------------------

#endif //__glow_h
