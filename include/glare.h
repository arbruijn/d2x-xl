#ifndef _GLARE_H
#define _GLARE_H

class CGlareRenderer {
	private:
		GLhandleARB	m_shaderProg;
		GLuint		m_hDepthBuffer;

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
		void CalcSpriteCoords (CFloatVector *vSprite, CFloatVector *vCenter, CFloatVector *vEye, float dx, float dy, CFloatMatrix *r);
		int CalcFaceDimensions (short nSegment, short nSide, fix *w, fix *h, ushort* corners, ubyte nCorners);
		float ComputeCoronaSprite (CFloatVector *sprite, CFloatVector *vCenter, short nSegment, short nSide, ubyte& nVertices);
		void ComputeSpriteZRange (CFloatVector *sprite, tIntervalf *zRangeP);
		float MoveSpriteIn (CFloatVector *sprite, CFloatVector *vCenter, tIntervalf *zRangeP, float fIntensity);
		void ComputeHardGlare (CFloatVector *sprite, CFloatVector *vCenter, CFloatVector *vNormal);
		void RenderHardGlare (CFloatVector *sprite, CFloatVector *vCenter, int nTexture, float fLight,
									 float fIntensity, tIntervalf *zRangeP, int bAdditive, int bColored);
		float ComputeSoftGlare (CFloatVector *sprite, CFloatVector *vLight, CFloatVector *vEye);
		void RenderSoftGlare (CFloatVector *sprite, ubyte nVertices, CFloatVector *vCenter, int nTexture, float fIntensity, int bAdditive, int bColored);
	
};

extern CGlareRenderer	glareRenderer;
extern float coronaIntensities [];

#endif /* _GLARE_H */
