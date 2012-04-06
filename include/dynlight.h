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
		ubyte				shininess;
		ubyte				bValid;
		short				nLight;
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
		short					nVerts [4];
		int					nTarget;	//lit segment/face
		int					nFrame;
		ubyte					nType;
		ubyte					bState;
		ubyte					bShadow;
		ubyte					bLightning;
		ubyte					bExclusive;
		ubyte					bUsed [MAX_THREADS];
		CActiveDynLight*	activeLightsP [MAX_THREADS];

	public:
		CLightRenderData ();
	};

//------------------------------------------------------------------------------

class CDynLightInfo {
	public:
		CSegFace*		faceP;
		CFixVector		vPos;
		CFloatVector	vDirf;
		CFloatVector	color;
		float				fBrightness;
		float				fBoost;
		float				fRange;
		float				fRad;
		float				fSpotAngle;
		float				fSpotExponent;
		short				nIndex;
		short				nSegment;
		short				nSide;
		short				nObject;
		ubyte				nPlayer;
		ubyte				nType;
		ubyte				bState;
		ubyte				bOn;
		ubyte				bSpot;
		ubyte				bVariable;
		ubyte				bPowerup;
		ubyte				bAmbient;
		ubyte				bSelf; // only illuminates segment face it is located at
		sbyte				bDiffuse [MAX_THREADS];
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
		ubyte					bTransform;
		CDynLightInfo		info;
		CLightRenderData	render;
		//CShadowLightData	shadow;

	public:
		CDynLight ();
		~CDynLight ();
		void Init (void);
		void Destroy (void);
		CObject* Object (void);
		int LightSeg (void);
		inline short Index (void) { return info.nIndex; }
		inline short Segment (void) { return info.nSegment; }
		inline short Side (void) { return info.nSide; }
		int SeesPoint (const short nDestSeg, const CFixVector* vNormal, CFixVector* vPoint, const int nLevel, int nThread);
		int SeesPoint (const short nSegment, const short nSide, CFixVector* vPoint, const int nLevel, int nThread);
		int LightPathLength (const short nLightSeg, const short nDestSeg, const CFixVector& vDestPos, fix xMaxLightRange, int bFastRoute, int nThread);
		int Contribute (const short nDestSeg, const short nDestSide, const short nDestVertex, CFixVector& vDestPos, const CFixVector* vNormal, 
							 fix xMaxLightRange, float fRangeMod, fix xDistMod, int nThread);
		inline int Illuminate (short nSegment, short nSide) { 
			return !info.bSelf || (nSegment < 0) || ((info.nSegment == nSegment) && ((nSide < 0) || (info.nSide == nSide))); 
			}
		int ComputeVisibleVertices (int nThread);
		int Compare (CDynLight& other);
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
		short			nType;
		CDynLight*	lightP;
};

class CDynLightIndex {
	public:
		short	nFirst;
		short	nLast;
		short	nActive;
		short	iVertex;
		short	iStatic;
	};

//------------------------------------------------------------------------------

class CHeadlightManager {
	public:
		CDynLight*			lights [MAX_PLAYERS];
		CFloatVector		pos [MAX_PLAYERS];
		CFloatVector3		dir [MAX_PLAYERS];
		float					brightness [MAX_PLAYERS];
		CObject*				objects [MAX_PLAYERS];
		int					lightIds [MAX_PLAYERS];
		int					nLights;

	public:
		CHeadlightManager () { Init (); }
		void Init (void);
		void Transform (void);
		fix ComputeLightOnObject (CObject *objP);
		void Toggle (void);
		void Prepare (void);
		int Add (CObject* objP);
		void Remove (CObject* objP);
		void Update (void);
		int SetupShader (int nType, int bLightmaps, CFloatVector *colorP);
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
		short					nLights [3];
		short					nGeometryLights;
		short					nVariable;
		short					nDynLights;
		short					nVertLights;
		short					nHeadlights [MAX_PLAYERS];
		short					nSegment;
		GLuint				nTexHandle;
		int					nThread;

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
		int Setup (int nLevel);

		void SetColor (short nLight, float red, float green, float blue, float fBrightness);
		void SetPos (short nObject);
		short Find (short nSegment, short nSide, short nObject);
		bool Ambient (short nSegment, short nSide);
		short Update (CFloatVector *colorP, float fBrightness, short nSegment, short nSide, short nObject);
		int LastEnabled (void);
		void Swap (CDynLight* pl1, CDynLight* pl2);
		int Toggle (short nSegment, short nSide, short nObject, int bState);
		void Register (CFaceColor *colorP, short nSegment, short nSide);
		int Add (CSegFace* faceP, CFloatVector *colorP, fix xBrightness, short nSegment,
				   short nSide, short nObject, short nTexture, CFixVector *vPos, ubyte bAmbient = 0);
		void Delete (short nLight);
		void DeleteLightning (void);
		int Delete (short nSegment, short nSide, short nObject);
		void Reset (void);
		void SetMaterial (short nSegment, short nSide, short nObject);
		void AddGeometryLights (void);
		void Transform (int bStatic, int bVariable);
		ubyte VariableVertexLights (int nVertex);
		void SetNearestToVertex (int nSegment, int nSide, int nVertex, CFixVector *vNormalP, ubyte nType, int bStatic, int bVariable, int nThread);
		int SetNearestToFace (CSegFace* faceP, int bTextured);
		short SetNearestToSegment (int nSegment, int nFace, int bVariable, int nType, int nThread);
		void SetNearestStatic (int nSegment, int bStatic, int nThread);
		short SetNearestToPixel (short nSegment, short nSide, CFixVector *vNormal, CFixVector *vPixelPos, float fLightRad, int nThread);
		void ResetNearestStatic (int nSegment, int nThread);
		void ResetNearestToVertex (int nVertex, int nThread);
		int SetNearestToSgmAvg (short nSegment, int nThread);
		void ResetSegmentLights (void);
		CFaceColor* AvgSgmColor (int nSegment, CFixVector *vPosP, int nThread);
		void GatherStaticLights (int nLevel);
		void GatherStaticVertexLights (int nVertex, int nMax, int nThread);
		int SetActive (CActiveDynLight* activeLightsP, CDynLight* lightP, short nType, int nThread, bool bForce = false);
		CDynLight* GetActive (CActiveDynLight* activeLightsP, int nThread);
		void ResetUsed (CDynLight* lightP, int nThread);
		void ResetActive (int nThread, int nActive);
		void ResetAllUsed (int bVariable, int nThread);
		int SetMethod (void);

		inline void SetThreadId (int nThread = -1) { m_data.nThread = nThread; }
		inline int ThreadId (uint i) { return (m_data.nThread < 0) ? i : m_data.nThread; }
		inline CDynLight* Lights (uint i = 0) { return &m_data.lights [i]; }
		inline CDynLight* RenderLights (uint i) { return m_data.renderLights [i]; }
		inline CDynLight** RenderLights (void) { return m_data.renderLights.Buffer (); }
		inline short LightCount (uint i) { return m_data.nLights [i]; }
		inline short GeometryLightCount (void) { return m_data.nGeometryLights; }
		inline void SetLightCount (short nCount, uint i) { m_data.nLights [i] = nCount; }
		inline CActiveDynLight* Active (uint i) { return m_data.active [ThreadId (i)].Buffer (); }
		inline CDynLightIndex& Index (uint i, int nThread) { return m_data.index [0][ThreadId (nThread)]; }
		inline CFBO& FBO (void) { return m_data.fbo; }
		inline CHeadlightManager& Headlights (void) { return m_headlights; }
		inline CShortArray& NearestSegLights (void) { return m_data.nearestSegLights; }
		inline CShortArray& NearestVertLights (void) { return m_data.nearestVertLights; }
		inline ubyte& VariableVertLights (uint i = 0) { return m_data.variableVertLights [i]; }
		inline COglMaterial& Material (void) { return m_data.material; }
		inline void ResetIndex (void) { m_data.ResetIndex (); }
		//inline void SetShadowSource (CDynLight& source, int i) { m_data.shadowSources [i] = source; }
		//inline CDynLight& GetShadowSource (int i) { return m_data.shadowSources [i]; }
	private:
		static int IsTriggered (short nSegment, short nSide, bool bOppSide = false);
		static int IsFlickering (short nSegment, short nSide);
		int IsDestructible (short nTexture);
		bool DeleteFromList (CDynLight* lightP, short nLight);
		void Sort (void);
	};

//------------------------------------------------------------------------------

extern int nMaxNearestLights [21];

extern CLightManager lightManager;

//------------------------------------------------------------------------------

#if 0

void RegisterLight (CFaceColor *pc, short nSegment, short nSide);
int lightManager.Add (CSegFace *faceP, CFloatVector *pc, fix xBrightness, 
					  short nSegment, short nSide, short nOwner, short nTexture, CFixVector *vPos);
int RemoveDynLight (short nSegment, short nSide, short nObject);
void AddDynGeometryLights (void);
void DeleteDynLight (short nLight);
void RemoveDynLights (void);
void SetDynLightPos (short nObject);
void SetDynLightMaterial (short nSegment, short nSide, short nObject);
void MoveDynLight (short nObject);
void TransformDynLights (int bStatic, int bVariable);
short FindDynLight (short nSegment, short nSide, short nObject);
int ToggleDynLight (short nSegment, short nSide, short nObject, int bState);
void SetDynLightMaterial (short nSegment, short nSide, short nObject);
void SetNearestVertexLights (int nFace, int nVertex, CFixVector *vNormalP, ubyte nType, int bStatic, int bVariable, int nThread);
int SetNearestFaceLights (CSegFace *faceP, int bTextured);
short SetNearestPixelLights (short nSegment, short nSide, CFixVector *vNormal, CFixVector *vPixelPos, float fLightRad, int nThread);
void SetNearestStaticLights (int nSegment, int bStatic, ubyte nType, int nThread);
void ResetNearestStaticLights (int nSegment, int nThread);
void lightManager.ResetNearestToVertex (int nVertex, int nThread);
short SetNearestSegmentLights (int nSegment, int nFace, int bVariable, int nType, int nThread);
void ComputeStaticVertexLights (int nVertex, int nMax, int nThread);
void ComputeStaticDynLighting (int nLevel);
CDynLight *GetActiveRenderLight (CActiveDynLight *activeLightsP, int nThread);
CFaceColor *AvgSgmColor (int nSegment, CFixVector *vPos);
void ResetSegmentLights (void);
int IsLight (int tMapNum);
void ResetUsedLight (CDynLight *lightP, int nThread);
void ResetUsedLights (int bVariable, int nThread);
void ResetActiveLights (int nThread, int nActive);

#endif

int CreatePerPixelLightingShader (int nType, int nLights);
void InitPerPixelLightingShaders (void);
void ResetPerPixelLightingShaders (void);
void InitHeadlightShaders (int nLights);
char *BuildLightingShader (const char *pszTemplate, int nLights);
int CreateLightmapShader (int nType);
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
