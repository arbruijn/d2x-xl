#ifndef _GLARE_H
#define _GLARE_H

class CGlareRenderer {
	private:
		GLhandleARB		m_shaderProg;
		GLuint			m_hDepthBuffer;
		CFloatVector	m_sprite [4];
		int32_t				m_nVertices;
		CFloatVector	m_vCenter;
		CFloatVector	m_vEye;
		tIntervalf		m_zRange;

	public:
		int32_t FaceHasCorona (int16_t nSegment, int16_t nSide, int32_t *bAdditiveP, float *dimP);
		void Render (int16_t nSegment, int16_t nSide, float fIntensity, float fSize);
		float Visibility (int32_t nQuery);
		bool LoadShader (float dMax, int32_t nBlendMode);
		void UnloadShader (void);
		void InitShader (void);
		bool ShaderActive (void);
		int32_t Style (void);
		inline GLuint DepthBuffer (void) { return m_hDepthBuffer; }

	private:
		GLuint CopyDepthTexture (void);
		int32_t CalcFaceDimensions (int16_t nSegment, int16_t nSide, fix *w, fix *h, uint16_t* corners);
		float ComputeCoronaSprite (int16_t nSegment, int16_t nSide);
		void ComputeSpriteZRange (void);
		float MoveSpriteIn (float fIntensity);
		float ComputeSoftGlare (void);
		void RenderSoftGlare (int32_t nTexture, float fIntensity, int32_t bAdditive, int32_t bColored);
	
};

extern CGlareRenderer	glareRenderer;
extern float coronaIntensities [];

#endif /* _GLARE_H */
