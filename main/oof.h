#ifndef __OOF_H
#define __OOF_H

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

//------------------------------------------------------------------------------
// Subobject flags

#define OOF_PMF_LIGHTMAP_RES	1
#define OOF_PMF_TIMED			2			// Uses new timed animation
#define OOF_PMF_ALPHA			4			// Has alpha per vertex qualities
#define OOF_PMF_FACING			8			// Has a submodel that is always facing
#define OOF_PMF_NOT_RESIDENT	16			// This polymodel is not in memory
#define OOF_PMF_SIZE_COMPUTED	32			// This polymodel's size is computed

#define OOF_SOF_ROTATE		0x01			// This subobject is a rotator
#define OOF_SOF_TURRET		0x02			//	This subobject is a turret that tracks
#define OOF_SOF_SHELL		0x04			// This subobject is a door housing
#define OOF_SOF_FRONTFACE	0x08			// This subobject contains the front face for the door
#define OOF_SOF_MONITOR1	0x010			// This subobject contains it's first monitor
#define OOF_SOF_MONITOR2	0x020			// This subobject contains it's second monitor
#define OOF_SOF_MONITOR3	0x040			// This subobject contains it's third monitor
#define OOF_SOF_MONITOR4	0x080			// This subobject contains it's fourth monitor
#define OOF_SOF_MONITOR5	0x0100
#define OOF_SOF_MONITOR6	0x0200
#define OOF_SOF_MONITOR7	0x0400
#define OOF_SOF_MONITOR8	0x0800
#define OOF_SOF_FACING		0x01000		// This subobject always faces you
#define OOF_SOF_VIEWER		0x02000		// This subobject is marked as a 'viewer'.
#define OOF_SOF_LAYER		0x04000		// This subobject is marked as part of possible secondary model rendering.
#define OOF_SOF_WB			0x08000		// This subobject is part of a weapon battery
#define OOF_SOF_GLOW			0x0200000	// This subobject glows
#define OOF_SOF_CUSTOM		0x0400000	// This subobject has textures/colors that are customizable
#define OOF_SOF_THRUSTER	0x0800000   // This is a thruster subobject
#define OOF_SOF_JITTER		0x01000000  // This object jitters by itself
#define OOF_SOF_HEADLIGHT	0x02000000	// This suboject is a headlight

#define OOF_MAX_GUNS_PER_MODEL	      64
#define OOF_MAX_SUBOBJECTS		         30
#define OOF_MAX_POINTS_PER_SUBOBJECT	300

#define OOF_WB_INDEX_SHIFT		16       // bits to shift over to get the weapon battery index (after masking out flags)
#define OOF_SOF_WB_MASKS		0x01F0000// Room for 32 weapon batteries (currently we only use 21 slots)

#define OOF_SOF_MONITOR_MASK	0x0ff0	// mask for monitors

//------------------------------------------------------------------------------

#define OOF_PAGENAME_LEN	35

typedef unsigned short tOOF_angle;	//make sure this matches up with fix.h

typedef char tOOF_chunkType [4];

typedef struct tOOF_angVec {
	tOOF_angle p,h,b;
} tOOF_angVec;

typedef struct tOOF_vector {
	float		x, y, z;
} tOOF_vector;

typedef struct tOOF_matrix {
	tOOF_vector	r, u, f;
} tOOF_matrix;

typedef struct tOOF_triangle {
	tOOF_vector	p [3];
	tOOF_vector c;
} tOOF_triangle;

typedef struct tOOF_quad {
	tOOF_vector	p [4];
	tOOF_vector c;
} tOOF_quad;

typedef struct tOOF_rgb {
	ubyte		r, g, b;
} tOOF_rgb;

typedef struct tOOF_frgb {
	float		r, g, b;
} tOOF_frgb;

typedef struct tOOF_chunkHeader {
	tOOF_chunkType		type;
	int					nLength;
} tOOF_chunkHeader;

typedef struct tOOF_textures {
	int					nTextures;
	char					**pszNames;
	grs_bitmap			*pBitmaps;
} tOOF_textures;

typedef struct tOOF_faceVert {
	int					nIndex;
	float					fu;
	float					fv;
} tOOF_faceVert;

typedef struct tOOF_face {
	tOOF_vector			vNormal;
	tOOF_vector			vRotNormal;
	tOOF_vector			vCenter;
	tOOF_vector			vRotCenter;
	int					nVerts;
	int					bTextured;
	union {
		int				nTexId;
		tOOF_rgb			color;
		} texProps;
	tOOF_faceVert		*pVerts;
	float					fBoundingLength;
	float					fBoundingWidth;
	tOOF_vector			vMin;
	tOOF_vector			vMax;
	ubyte					bFacingLight : 1;
	ubyte					bFacingViewer : 1;
} tOOF_face;

typedef struct tOOF_faceList {
	int					nFaces;
	tOOF_face			*pFaces;
	tOOF_faceVert		*pFaceVerts;
} tOOF_faceList;

typedef struct tOOF_glowInfo {
	tOOF_frgb			color;
	float					fSize;
	float					fLength;
	tOOF_vector			vCenter;
	tOOF_vector			vNormal;
} tOOF_glowInfo;

typedef struct tOOF_specialPoint {
	char					*pszName;
	char					*pszProps;
	tOOF_vector			vPos;
	float					fRadius;
} tOOF_specialPoint;

typedef struct tOOF_specialList {
	int					nVerts;
	tOOF_specialPoint	*pVerts;
} tOOF_specialList;

typedef struct tOOF_point {
   int					nParent;
   tOOF_vector			vPos;
   tOOF_vector			vDir;
} tOOF_point;

typedef struct tOOF_pointList {
   int					nPoints;        
   tOOF_point			*pPoints;
} tOOF_pointList;

typedef struct tOOF_attachPoint {
	tOOF_point			point;
	tOOF_vector			vu;
	char					bu;
} tOOF_attachPoint;

typedef struct tOOF_attachList {
   int					nPoints;        
   tOOF_attachPoint	*pPoints;
} tOOF_attachList;

typedef struct tOOF_battery {
	int					nVerts;
	int					*pVertIndex;
	int					nTurrets;
	int					*pTurretIndex;
} tOOF_battery;

typedef struct tOOF_armament {
	int					nBatts;
	tOOF_battery		*pBatts;
} tOOF_armament;

typedef struct tOOF_frameInfo {
	int					nFrames;
	int					nFirstFrame;
	int					nLastFrame;
} tOOF_frameInfo;

typedef struct tOOF_posFrame {
	int					iKeyFrame;
	tOOF_vector			vPos;
	int					nStartTime;
} tOOF_posFrame;

typedef struct tOOF_posAnim {
	tOOF_frameInfo		frameInfo;
	tOOF_posFrame		*pFrames;
	int					nTicks;
	ubyte					*pRemapTicks;
} tOOF_posAnim;

typedef struct tOOF_rotFrame {
	int					iKeyFrame;
	tOOF_vector			vAxis;
	int					nAngle;
	tOOF_matrix			mMat;
	int					nStartTime;
} tOOF_rotFrame;

typedef struct tOOF_rotAnim {
	tOOF_frameInfo		frameInfo;
	tOOF_rotFrame		*pFrames;
	int					nTicks;
	ubyte					*pRemapTicks;
} tOOF_rotAnim;

typedef struct tOOF_edge {
	int					v0;
	int					v1;
	tOOF_face			*pf0;
	tOOF_face			*pf1;
	char					bContour;
} tOOF_edge;

typedef struct tOOF_edgeList {
	int					nEdges;
	int					nContourEdges;
	tOOF_edge			*pEdges;
} tOOF_edgeList;

typedef struct tOOF_subObject {
	int					nIndex;
	int					nParent;
	int					nFlags;
	tOOF_vector			vNormal;
	float					fd;
	tOOF_vector			vPlaneVert;
	tOOF_vector			vOffset;
	float					fRadius;
	int					nTreeOffset;
	int					nDataOffset;
	tOOF_vector			vCenter;
	char					*pszName;
	char					*pszProps;
	int					nMovementType;
	int					nMovementAxis;
	int					nFSLists;
	int					*pFSList;
	int					nVerts;
	tOOF_vector			*pvVerts;
	tOOF_vector			*pvRotVerts;
	tOOF_vector			*pvNormals;
	tFaceColor			*pVertColors;
	float					*pfAlpha;	// only present if version >= 2300
	tOOF_faceList		faces;
	tOOF_edgeList		edges;
	tOOF_posAnim		posAnim;
	tOOF_rotAnim		rotAnim;
	tOOF_vector			vMin;
	tOOF_vector			vMax;
	tOOF_glowInfo		glowInfo;
	float					fFOV;
	float					fRPS;
	float					fUpdate;
	int					nChildren;
	int					children [OOF_MAX_SUBOBJECTS];
	tOOF_angVec			aMod;
	tOOF_matrix			mMod;				// The angles from parent.  Stuffed by model_set_instance
	tOOF_vector			vMod;
} tOOF_subObject;

typedef struct tOOF_object {
	int					nVersion;
	int					nFlags;
	float					fMaxRadius;
	tOOF_vector			vMin;
	tOOF_vector			vMax;
	int					nDetailLevels;
	int					nSubObjects;
	tOOF_subObject		*pSubObjects;
	tOOF_pointList		gunPoints;
	tOOF_attachList	attachPoints;
	tOOF_specialList	specialPoints;
	tOOF_armament		armament;
	tOOF_textures		textures;
	tOOF_frameInfo		frameInfo;
	int					bCloaked;
	int					nCloakPulse;
	int					nCloakChangedTime;
	float					fAlpha;
} tOOF_object;

typedef float glVectorf [4];
typedef float glMatrixf [4*4];

//------------------------------------------------------------------------------

int OOF_ReadFile (char *pszFile, tOOF_object *po);
int OOF_FreeObject (tOOF_object *po);
int OOF_Render (tOOF_object *po, vms_vector *vPos, vms_matrix *mOrient, float *fLight, int bCloaked);
float *OOF_MatVms2Gl (float *pDest, vms_matrix *pSrc);
float *OOF_VecVms2Gl (float *pDest, vms_vector *pSrc);
float *OOF_VecVms2Oof (tOOF_vector *pDest, vms_vector *pSrc);
float *OOF_MatVms2Oof (tOOF_matrix *pDest, vms_matrix *pSrc);
tOOF_vector *OOF_VecAdd (tOOF_vector *pvDest, tOOF_vector *pvSrc, tOOF_vector *pvAdd);
tOOF_vector *OOF_VecSub (tOOF_vector *pvDest, tOOF_vector *pvMin, tOOF_vector *pvSub);
tOOF_vector *OOF_VecInc (tOOF_vector *pvDest, tOOF_vector *pvSrc);
tOOF_vector *OOF_VecDec (tOOF_vector *pvDest, tOOF_vector *pvSrc);
float OOF_VecMul (tOOF_vector *pvSrc, tOOF_vector *pvMul);
inline float OOF_VecDot (tOOF_vector *pv0, tOOF_vector *pv1);
tOOF_vector *OOF_VecScale (tOOF_vector *pv, float fScale);
float OOF_VecMag (tOOF_vector *pv);
tOOF_vector *OOF_VecRot (tOOF_vector *pDest, tOOF_vector *pSrc, tOOF_matrix *pRot);
tOOF_vector *OOF_VecPerp (tOOF_vector *pvPerp, tOOF_vector *pv0, tOOF_vector *pv1, tOOF_vector *pv2);
tOOF_vector *OOF_VecNormal (tOOF_vector *pvNormal, tOOF_vector *pv0, tOOF_vector *pv1, tOOF_vector *pv2);
float *OOF_GlIdent (float *pm);
float *OOF_GlInverse (float *pDest, float *pSrc);
int OOF_ReleaseTextures (void);
int OOF_ReloadTextures (void);

//------------------------------------------------------------------------------

#endif //__OOF_H
//eof
