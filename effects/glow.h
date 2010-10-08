#ifndef __blur_h
#define __blur_h

//------------------------------------------------------------------------------

class CGlowRenderer {
	private:
		GLhandleARB m_shaderProg;

	public:
		void InitShader (void);
		bool ShaderActive (void);
		CGlowRenderer () : m_shaderProg (0) {}

	private:
		bool LoadShader (int const direction);
		void Draw (int const direction);
		bool Blur (int const direction);
		void Render (void);
	};

extern CGlowRenderer glowRenderer;

//------------------------------------------------------------------------------

#endif //__blur_h
