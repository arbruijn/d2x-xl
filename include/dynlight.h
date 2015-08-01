#ifndef _DYNLIGHT_H
#define _DYNLIGHT_H

//------------------------------------------------------------------------------

#define	MAX_LIGHTS_PER_PIXEL 8
#define USE_OGL_LIGHTS			0
#define MAX_OGL_LIGHTS			(64 * 64) //MUST be a power of 2!
#define MAX_SHADER_LIGHTS		1000

//------------------------------------------------------------------------------

class COglMaterial {
	public:
#if 0 //using global default values instead
		CFloatVector	diffuse;
		CFloatVector	ambient;
#endif
		CFloatVector	specular;
		CFloatVector	emissive;
		uint8_t			shininess;
		uint8_t			bValid;
		int16_t			nLight;
	public:
		COglMaterial () { Init (); }
		void Init (void);
};

//------------------------------------------------------------------------------

class CDynLight;
class CActiveDynLight;

class CLightRenderData {
	public:
		CFloatVector		vPosf [2];
		fix					xDistance [MAX_THREADS];
		int16_t				nVerts [4];
		int32_t				nTarget;	//lit segment/face
		int32_t				nFrame;
		uint8_t				nType;
		uint8_t				bState;
		uint8_t				bShadow;
		uint8_t				bLightning;
		uint8_t				bExclusive;
		uint8_t				bUsed [MAX_THREADS];
		CActiveDynLight	*pActiveLights [MAX_THREADS];

	public:
		CLightRenderData ();
	};

//------------------------------------------------------------------------------

class CDynLightInfo {
	public:
		CSegFace			*pFace;
		CFixVector		vPos;
		CFloatVector	vDirf;
		CFloatVector	color;
		float				fBrightness;
		float				fBoost;
		float				fRange;
		float				fRad;
		float				fSpotAngle;
		float				fSpotExponent;
		int16_t			nIndex;
		int16_t			nSegment;
		int16_t			nSide;
		int16_t			nObject;
		uint8_t			nPlayer;
		uint8_t			nType;
		uint8_t			bState;
		uint8_t			bOn;
		uint8_t			bSpot;
		uint8_t			bVariable;
		uint8_t			bPowerup;
		uint8_t			bAmbient;
		uint8_t			bSelf; // only illuminates segment face it is located at
		int8_t			bDiffuse [MAX_THREADS];
		CByteArray*		visibleVertices;
	public:
		CDynLightInfo () { Init (); }
		void Init (void) { memset (this, 0, sizeof (*this)); }
};

class CDynLight {
	public:
		CFixVector			vDir;
		CFloatVector		color;
		CFloatVector		fSpecular;
		CFloatVector		fEmissive;
		uint8_t				bTransform;
		CDynLightInfo		info;
		CLightRenderData	render;
		//CShadowLightData	shadow;

	public:
		CDynLight ();
		~CDynLight ();
		void Init (void);
		void Destroy (void);
		CObject* Object (void);
		int32_t LightSeg (void);
		inline int16_t Index (void) { return info.nIndex; }
		inline int16_t Segment (void) { return info.nSegment; }
		inline int16_t Side (void) { return info.nSide; }
		int32_t SeesPoint (const int16_t nDestSeg, const int8_t nDestSide, const CFixVector* vNormal, CFixVector* vPoint, const int32_t nLevel, int32_t nThread);
		int32_t SeesPoint (const int16_t nSegment, const int8_t nSide, CFixVector* vPoint, const int32_t nLevel, int32_t nThread);
		int32_t LightPathLength (const int16_t nLightSeg, const int16_t nDestSeg, const CFixVector& vDestPos, fix xMaxLightRange, int32_t bFastRoute, int32_t nThread);
		int32_t Contribute (const int16_t nDestSeg, const int8_t nDestSide, const int16_t nDestVertex, CFixVector& vDestPos, const CFixVector* vNormal, 
								  fix xMaxLightRange, float fRangeMod, fix xDistMod, int32_t nThread);
		inline int32_t Illuminate (int16_t nSegment, int16_t nSide) { 
			return !info.bAmbient || (nSegment < 0) || ((info.nSegment == nSegment) && ((nSide < 0) || (info.nSide == nSide))); 
			}
		int32_t ComputeVisibleVertices (int32_t nThread);
		int32_t Compare (CDynLight& other);
		inline bool operator< (CDynLight& other)
		 { return Compare (other) < 0; }
		inline bool operator> (CDynLight& other)
		 { return Compare (other) > 0; }
		inline bool operator== (CDynLight& other)
		 { return Compare (other) == 0; }
	};

//------------------------------------------------------------------------------

class CActiveDynLight {
	public:
		int16_t		nType;
		CDynLight	*pLight;
};

class CDynLightIndex {
	public:
		int16_t	nFirst;
		int16_t	nLast;
		int16_t	nActive;
		int16_t	iVertex;
		int16_t	iStatic;
	};

//------------------------------------------------------------------------------

class CHeadlightManager {
	public:
		CDynLight*			lights [MAX_PLAYERS];
		CFloatVector		pos [MAX_PLAYERS];
		CFloatVector3		dir [MAX_PLAYERS];
		float					brightness [MAX_PLAYERS];
		CObject*				objects [MAX_PLAYERS];
		int32_t				lightIds [MAX_PLAYERS];
		int32_t				nLights;

	public:
		CHeadlightManager () { Init (); }
		void Init (void);
		void Transform (void);
		fix ComputeLightOnObject (CObject *pObj);
		void Toggle (void);
		void Prepare (void);
		int32_t Add (CObject* pObj);
		void Remove (CObject* pObj);
		void Update (void);
		int32_t SetupShader (int32_t nType, int32_t bLightmaps, CFloatVector *pColor);
};

//------------------------------------------------------------------------------

#include "oglmatrix.h"

#define MAX_NEAREST_LIGHTS 32

class CDynLightData {
	public:
		CStaticArray<CDynLight, MAX_OGL_LIGHTS>		lights; // [MAX_OGL_LIGHTS];
		CStaticArray<CDynLight*, MAX_OGL_LIGHTS>		renderLights; // [MAX_OGL_LIGHTS];
		//CStaticArray<CDynLight, MAX_SHADOWMAPS>		shadowSources;
		CArray< CActiveDynLight >							active [MAX_THREADS]; //[MAX_OGL_LIGHTS];
		CStaticArray< CDynLightIndex, MAX_THREADS >	index [2]; //[MAX_THREADS];

		CShortArray			nearestSegLights;		//the 8 nearest static lights for every segment
		CShortArray			nearestVertLights;	//the 8 nearest static lights for every vertex
		CByteArray			variableVertLights;	//the 8 nearest veriable lights for every vertex
		CShortArray			owners;
		COglMaterial		material;
		CFBO					fbo;
		int16_t				nLights [3];
		int16_t				nGeometryLights;
		int16_t				nVariable;
		int16_t				nDynLights;
		int16_t				nVertLights;
		int16_t				nHeadlights [MAX_PLAYERS];
		int16_t				nSegment;
		GLuint				nTexHandle;
		int32_t				nThread;

	public:
		CDynLightData ();
		bool Create (void);
		void Init (void);
		void Destroy (void);
		inline void ResetIndex (void) { 
			index [0].Clear ();
			index [1].Clear ();
			}

};

//------------------------------------------------------------------------------

class CLightManager {
	private:
		CDynLightData		m_data;
		CHeadlightManager	m_headlights;

	public:
		CLightManager () { Init (); }
		~CLightManager () { Destroy (); }
		void Init (void);
		void Destroy (void);
		bool Create (void);
		int32_t Setup (int32_t nLevel);

		void SetColor (int16_t nLight, float red, float green, float blue, float fBrightness);
		void SetPos (int16_t nObject);
		int16_t Find (int16_t nSegment, int16_t nSide, int16_t nObject);
		bool Ambient (int16_t nSegment, int16_t nSide);
		int16_t Update (CFloatVector *pColor, float fBrightness, int16_t nSegment, int16_t nSide, int16_t nObject);
		int32_t LastEnabled (void);
		void Swap (CDynLight* pl1, CDynLight* pl2);
		int32_t Toggle (int16_t nSegment, int16_t nSide, int16_t nObject, int32_t bState);
		void Register (CFaceColor *pColor, int16_t nSegment, int16_t nSide);
		int32_t Add (CSegFace* pFace, CFloatVector *pColor, fix xBrightness, int16_t nSegment,
				   int16_t nSide, int16_t nObject, int16_t nTexture, CFixVector *vPos, uint8_t bAmbient = 0);
		void Delete (int16_t nLight);
		void DeleteLightning (void);
		int32_t Delete (int16_t nSegment, int16_t nSide, int16_t nObject);
		void Reset (void);
		void SetMaterial (int16_t nSegment, int16_t nSide, int16_t nObject);
		void AddGeometryLights (void);
		void Transform (int32_t bStatic, int32_t bVariable);
		uint8_t VariableVertexLights (int32_t nVertex);
		void SetNearestToVertex (int32_t nSegment, int32_t nSide, int32_t nVertex, CFixVector *vNormalP, uint8_t nType, int32_t bStatic, int32_t bVariable, int32_t nThread);
		int32_t SetNearestToFace (CSegFace* pFace, int32_t bTextured);
		int16_t SetNearestToSegment (int32_t nSegment, int32_t nFace, int32_t bVariable, int32_t nType, int32_t nThread);
		void SetNearestStatic (int32_t nSegment, int32_t bStatic, int32_t nThread);
		int16_t SetNearestToPixel (int16_t nSegment, int8_t nSide, CFixVector *vNormal, CFixVector *vPixelPos, float fLightRad, int32_t nThread);
		void ResetNearestStatic (int32_t nSegment, int32_t nThread);
		void ResetNearestToVertex (int32_t nVertex, int32_t nThread);
		int32_t SetNearestToSgmAvg (int16_t nSegment, int32_t nThread);
		void ResetSegmentLights (void);
		CFaceColor* AvgSgmColor (int32_t nSegment, CFixVector *vPosP, int32_t nThread);
		void GatherStaticLights (int32_t nLevel);
		void GatherStaticVertexLights (int32_t nVertex, int32_t nMax, int32_t nThread);
		int32_t SetActive (CActiveDynLight* pActiveLights, CDynLight* pLight, int16_t nType, int32_t nThread, bool bForce = false);
		CDynLight* GetActive (CActiveDynLight* pActiveLights, int32_t nThread);
		void ResetUsed (CDynLight* pLight, int32_t nThread);
		void ResetActive (int32_t nThread, int32_t nActive);
		void ResetAllUsed (int32_t bVariable, int32_t nThread);
		int32_t SetMethod (void);

		inline void SetThreadId (int32_t nThread = -1) { m_data.nThread = nThread; }
		inline int32_t ThreadId (int32_t i) { return (m_data.nThread < 0) ? i : m_data.nThread; }
		inline CDynLight* Lights (uint32_t i = 0) { return &m_data.lights [i]; }
		inline CDynLight* RenderLights (uint32_t i) { return m_data.renderLights [i]; }
		inline CDynLight** RenderLights (void) { return m_data.renderLights.Buffer (); }
		inline int16_t LightCount (uint32_t i) { return m_data.nLights [i]; }
		inline int16_t GeometryLightCount (void) { return m_data.nGeometryLights; }
		inline void SetLightCount (int16_t nCount, uint32_t i) { m_data.nLights [i] = nCount; }
		inline CActiveDynLight* Active (uint32_t i) { return m_data.active [ThreadId (i)].Buffer (); }
		inline CDynLightIndex& Index (uint32_t i, int32_t nThread) { return m_data.index [0][ThreadId (nThread)]; }
		inline CFBO& FBO (void) { return m_data.fbo; }
		inline CHeadlightManager& Headlights (void) { return m_headlights; }
		inline CShortArray& NearestSegLights (void) { return m_data.nearestSegLights; }
		inline CShortArray& NearestVertLights (void) { return m_data.nearestVertLights; }
		inline uint8_t& VariableVertLights (uint32_t i = 0) { return m_data.variableVertLights [i]; }
		inline COglMaterial& Material (void) { return m_data.material; }
		inline void ResetIndex (void) { m_data.ResetIndex (); }
		//inline void SetShadowSource (CDynLight& source, int32_t i) { m_data.shadowSources [i] = source; }
		//inline CDynLight& GetShadowSource (int32_t i) { return m_data.shadowSources [i]; }
	private:
		static int32_t IsTriggered (int16_t nSegment, int16_t nSide, bool bOppSide = false);
		static int32_t IsFlickering (int16_t nSegment, int16_t nSide);
		int32_t IsDestructible (int16_t nTexture);
		bool DeleteFromList (CDynLight* pLight, int16_t nLight);
		void Sort (void);
	};

//------------------------------------------------------------------------------

extern int32_t nMaxNearestLights [21];

extern CLightManager lightManager;

//------------------------------------------------------------------------------

#if 0

void RegisterLight (CFaceColor *pc, int16_t nSegment, int16_t nSide);
int32_t lightManager.Add (CSegFace *pFace, CFloatVector *pc, fix xBrightness, 
					  int16_t nSegment, int16_t nSide, int16_t nOwner, int16_t nTexture, CFixVector *vPos);
int32_t RemoveDynLight (int16_t nSegment, int16_t nSide, int16_t nObject);
void AddDynGeometryLights (void);
void DeleteDynLight (int16_t nLight);
void RemoveDynLights (void);
void SetDynLightPos (int16_t nObject);
void SetDynLightMaterial (int16_t nSegment, int16_t nSide, int16_t nObject);
void MoveDynLight (int16_t nObject);
void TransformDynLights (int32_t bStatic, int32_t bVariable);
int16_t FindDynLight (int16_t nSegment, int16_t nSide, int16_t nObject);
int32_t ToggleDynLight (int16_t nSegment, int16_t nSide, int16_t nObject, int32_t bState);
void SetDynLightMaterial (int16_t nSegment, int16_t nSide, int16_t nObject);
void SetNearestVertexLights (int32_t nFace, int32_t nVertex, CFixVector *vNormalP, uint8_t nType, int32_t bStatic, int32_t bVariable, int32_t nThread);
int32_t SetNearestFaceLights (CSegFace *pFace, int32_t bTextured);
int16_t SetNearestPixelLights (int16_t nSegment, int16_t nSide, CFixVector *vNormal, CFixVector *vPixelPos, float fLightRad, int32_t nThread);
void SetNearestStaticLights (int32_t nSegment, int32_t bStatic, uint8_t nType, int32_t nThread);
void ResetNearestStaticLights (int32_t nSegment, int32_t nThread);
void lightManager.ResetNearestToVertex (int32_t nVertex, int32_t nThread);
int16_t SetNearestSegmentLights (int32_t nSegment, int32_t nFace, int32_t bVariable, int32_t nType, int32_t nThread);
void ComputeStaticVertexLights (int32_t nVertex, int32_t nMax, int32_t nThread);
void ComputeStaticDynLighting (int32_t nLevel);
CDynLight *GetActiveRenderLight (CActiveDynLight *pActiveLights, int32_t nThread);
CFaceColor *AvgSgmColor (int32_t nSegment, CFixVector *vPos);
void ResetSegmentLights (void);
int32_t IsLight (int32_t tMapNum);
void ResetUsedLight (CDynLight *pLight, int32_t nThread);
void ResetUsedLights (int32_t bVariable, int32_t nThread);
void ResetActiveLights (int32_t nThread, int32_t nActive);

#endif

int32_t CreatePerPixelLightingShader (int32_t nType, int32_t nLights);
void InitPerPixelLightingShaders (void);
void ResetPerPixelLightingShaders (void);
void InitHeadlightShaders (int32_t nLights);
char *BuildLightingShader (const char *pszTemplate, int32_t nLights);
int32_t CreateLightmapShader (int32_t nType);
void InitLightmapShaders (void);
void ResetLightmapShaders (void);

#define	SHOW_DYN_LIGHT \
			(!(gameStates.app.bNostalgia || gameStates.render.bBriefing || (gameStates.app.bEndLevelSequence >= EL_OUTSIDE)) && \
			 /*gameStates.render.bHaveDynLights &&*/ \
			 gameOpts->render.nLightingMethod)

#define HAVE_DYN_LIGHT	(gameStates.render.bHaveDynLights && SHOW_DYN_LIGHT)

#define	APPLY_DYN_LIGHT \
			(gameStates.render.bUseDynLight && (gameOpts->ogl.bLightObjects || gameStates.render.nState))

extern CFaceColor tMapColor, lightColor, vertColors [8];

#endif //_DYNLIGHT_H
