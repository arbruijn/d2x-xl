#ifndef __glow_h
#define __glow_h

//------------------------------------------------------------------------------

class CGlowRenderer {
	private:
		GLhandleARB m_shaderProg;
		CFloatVector3 m_vMin, m_vMax;
		int m_x, m_y, m_w, m_h;
		int m_nActivated;

	public:
		void InitShader (void);
		bool ShaderActive (void);
		void End (bool bReplace = true, int nStrength = 1);
		void Begin (void);
		void Begin (CFloatVector3* vertexP, int nVerts);
		void Begin (CFixVector* pos, float radius);
		void Begin (CFloatVector3* pos, float width, float height);
		CGlowRenderer () : m_shaderProg (0), m_nActivated (0) {}

	private:
		bool LoadShader (int const direction, float const radius);
		void Render (int const source, int const direction = -1, float const radius = 1.0f);
		bool Blur (int const direction);
		void Project (CFloatVector3& v);
		void Activate (void);
		void SetExtent (CFloatVector3& v);
	};

extern CGlowRenderer glowRenderer;

//------------------------------------------------------------------------------

#endif //__glow_h
