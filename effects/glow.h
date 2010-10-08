#ifndef __blur_h
#define __blur_h

//------------------------------------------------------------------------------

class CBlurRenderer {
	private:
		GLhandleARB m_shaderProg;

	public:
		void InitShader (void);
		bool ShaderActive (void);
		CBlurRenderer () : m_shaderProg (0) {}

	private:
		bool LoadShader (int const direction);
		void Draw (int const direction);
		bool Blur (int const direction);
		void Render (void);
	};

extern CBlurRenderer blurRenderer;

//------------------------------------------------------------------------------

#endif //__blur_h
