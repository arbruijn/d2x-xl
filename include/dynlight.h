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
		fix					xDistance;
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
		CSegFace*			faceP;
		CFixVector		vPos;
		CFloatVector	vDirf;
		tRgbaColorf		color;
		float				fBrightness;
		float				fBoost;
		float				fRange;
		float				fRad;
		float				fSpotAngle;
		float				fSpotExponent;
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
		CShadowLightData	shadow;

	public:
		CDynLight ();
		void Init (void);
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
		CDynLight*	pl;
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
		int SetupShader (int nType, int bLightmaps, tRgbaColorf *colorP);
};

//------------------------------------------------------------------------------

#define MAX_NEAREST_LIGHTS 32

class CDynLightData {
	public:
		CStaticArray< CDynLight, MAX_OGL_LIGHTS >		lights; // [MAX_OGL_LIGHTS];
		CStaticArray< CDynLight*, MAX_OGL_LIGHTS >	renderLights; // [MAX_OGL_LIGHTS];
		CArray< CActiveDynLight >							active [MAX_THREADS]; //[MAX_OGL_LIGHTS];
		CArray< CDynLightIndex >							index [2]; //[MAX_THREADS];
		CShortArray			nearestSegLights;		//the 8 nearest static lights for every segment
		CShortArray			nearestVertLights;	//the 8 nearest static lights for every vertex
		CByteArray			variableVertLights;	//the 8 nearest veriable lights for every vertex
		CShortArray			owners;
		COglMaterial		material;
		CFBO					fbo;
		short					nLights [2];
		short					nVariable;
		short					nDynLights;
		short					nVertLights;
		short					nHeadlights [MAX_PLAYERS];
		short					nSegment;
		GLuint				nTexHandle;

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
		void Setup (int nLevel);

		void SetColor (short nLight, float red, float green, float blue, float fBrightness);
		void SetPos (short nObject);
		short Find (short nSegment, short nSide, short nObject);
		short Update (tRgbaColorf *colorP, float fBrightness, short nSegment, short nSide, short nObject);
		int LastEnabled (void);
		void Swap (CDynLight* pl1, CDynLight* pl2);
		int Toggle (short nSegment, short nSide, short nObject, int bState);
		void Register (tFaceColor *colorP, short nSegment, short nSide);
		int Add (CSegFace* faceP, tRgbaColorf *colorP, fix xBrightness, short nSegment,
				   short nSide, short nObject, short nTexture, CFixVector *vPos);
		void Delete (short nLight);
		void DeleteLightnings (void);
		int Delete (short nSegment, short nSide, short nObject);
		void Reset (void);
		void SetMaterial (short nSegment, short nSide, short nObject);
		void AddGeometryLights (void);
		void Transform (int bStatic, int bVariable);
		ubyte VariableVertexLights (int nVertex);
		void SetNearestToVertex (int nFace, int nVertex, CFixVector *vNormalP, ubyte nType, int bStatic, int bVariable, int nThread);
		int SetNearestToFace (CSegFace* faceP, int bTextured);
		short SetNearestToSegment (int nSegment, int nFace, int bVariable, int nType, int nThread);
		void SetNearestStatic (int nSegment, int bStatic, ubyte nType, int nThread);
		short SetNearestToPixel (short nSegment, short nSide, CFixVector *vNormal, CFixVector *vPixelPos, float fLightRad, int nThread);
		void ResetNearestStatic (int nSegment, int nThread);
		void ResetNearestToVertex (int nVertex, int nThread);
		int SetNearestToSgmAvg (short nSegment);
		void ResetSegmentLights (void);
		tFaceColor* AvgSgmColor (int nSegment, CFixVector *vPosP);
		void GatherStaticLights (int nLevel);
		void GatherStaticVertexLights (int nVertex, int nMax, int nThread);
		int SetActive (CActiveDynLight* activeLightsP, CDynLight* prl, short nType, int nThread, bool bForce = false);
		CDynLight* GetActive (CActiveDynLight* activeLightsP, int nThread);
		void ResetUsed (CDynLight* prl, int nThread);
		void ResetActive (int nThread, int nActive);
		void ResetAllUsed (int bVariable, int nThread);
		int SetMethod (void);

		inline CDynLight* Lights (void) { return m_data.lights.Buffer (); }
		inline CDynLight* RenderLights (uint i) { return m_data.renderLights [i]; }
		inline CDynLight** RenderLights (void) { return m_data.renderLights.Buffer (); }
		inline int LightCount (uint i) { return m_data.nLights [i]; }
		inline CActiveDynLight* Active (uint i) { return m_data.active [i].Buffer (); }
		inline CDynLightIndex* Index (uint i) { return m_data.index [i].Buffer (); }
		inline CFBO& FBO (void) { return m_data.fbo; }
		inline CHeadlightManager& Headlights (void) { return m_headlights; }
		inline CShortArray& NearestSegLights (void) { return m_data.nearestSegLights; }
		inline CShortArray& NearestVertLights (void) { return m_data.nearestVertLights; }
		inline CByteArray& VariableVertLights (void) { return m_data.variableVertLights; }
		inline COglMaterial& Material (void) { return m_data.material; }
		inline void ResetIndex (void) { m_data.ResetIndex (); }


	private:
		static int IsTriggered (short nSegment, short nSide, bool bOppSide = false);
		static int IsFlickering (short nSegment, short nSide);
		int IsDestructible (short nTexture);
		void DeleteFromList (CDynLight* pl, short nLight);
		void Sort (void);
	};

//------------------------------------------------------------------------------

extern int nMaxNearestLights [21];

extern CLightManager lightManager;

//------------------------------------------------------------------------------

#if 0

void RegisterLight (tFaceColor *pc, short nSegment, short nSide);
int lightManager.Add (CSegFace *faceP, tRgbaColorf *pc, fix xBrightness, 
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
tFaceColor *AvgSgmColor (int nSegment, CFixVector *vPos);
void ResetSegmentLights (void);
int IsLight (int tMapNum);
void ResetUsedLight (CDynLight *prl, int nThread);
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

extern tFaceColor tMapColor, lightColor, vertColors [8];

#endif //_DYNLIGHT_H
