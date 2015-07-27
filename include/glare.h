#ifndef _GLARE_H
#define _GLARE_H

class CGlareRenderer {
	private:
		GLhandleARB		m_shaderProg;
		GLuint			m_hDepthBuffer;
		CFloatVector	m_sprite [4];
		int				m_nVertices;
		CFloatVector	m_vCenter;
		CFloatVector	m_vEye;
		tIntervalf		m_zRange;

	public:
		int FaceHasCorona (short nSegment, short nSide, int *bAdditiveP, float *dimP);
		void Render (short nSegment, short nSide, float fIntensity, float fSize);
		float Visibility (int nQuery);
		bool LoadShader (float dMax, int nBlendMode);
		void UnloadShader (void);
		void InitShader (void);
		bool ShaderActive (void);
		int Style (void);
		inline GLuint DepthBuffer (void) { return m_hDepthBuffer; }

	private:
		GLuint CopyDepthTexture (void);
		int CalcFaceDimensions (short nSegment, short nSide, fix *w, fix *h, ushort* corners);
		float ComputeCoronaSprite (short nSegment, short nSide);
		void ComputeSpriteZRange (void);
		float MoveSpriteIn (float fIntensity);
		float ComputeSoftGlare (void);
		void RenderSoftGlare (int nTexture, float fIntensity, int bAdditive, int bColored);
	
};

extern CGlareRenderer	glareRenderer;
extern float coronaIntensities [];

#endif /* _GLARE_H */
