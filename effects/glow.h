#ifndef __glow_h
#define __glow_h

//------------------------------------------------------------------------------

class CGlowRenderer {
	private:
		GLhandleARB m_shaderProg;

	public:
		void InitShader (void);
		bool ShaderActive (void);
		void Flush (void);
		CGlowRenderer () : m_shaderProg (0) {}

	private:
		bool LoadShader (int const direction);
		void Render (int const direction, bool bBlur = false);
		bool Blur (int const direction);
	};

extern CGlowRenderer glowRenderer;

//------------------------------------------------------------------------------

#endif //__glow_h
