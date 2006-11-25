#ifndef __POF_H
#define __POF_H

//------------------------------------------------------------------------------
// Subobject flags

#define POF_PMF_LIGHTMAP_RES	1
#define POF_PMF_TIMED			2			// Uses new timed animation
#define POF_PMF_ALPHA			4			// Has alpha per vertex qualities
#define POF_PMF_FACING			8			// Has a submodel that is always facing
#define POF_PMF_NOT_RESIDENT	16			// This tPolyModel is not in memory
#define POF_PMF_SIZE_COMPUTED	32			// This tPolyModel's size is computed

#define POF_SOF_ROTATE		0x01			// This subobject is a rotator
#define POF_SOF_TURRET		0x02			//	This subobject is a turret that tracks
#define POF_SOF_SHELL		0x04			// This subobject is a door housing
#define POF_SOF_FRONTFACE	0x08			// This subobject contains the front face for the door
#define POF_SOF_MONITOR1	0x010			// This subobject contains it's first monitor
#define POF_SOF_MONITOR2	0x020			// This subobject contains it's second monitor
#define POF_SOF_MONITOR3	0x040			// This subobject contains it's third monitor
#define POF_SOF_MONITOR4	0x080			// This subobject contains it's fourth monitor
#define POF_SOF_MONITOR5	0x0100
#define POF_SOF_MONITOR6	0x0200
#define POF_SOF_MONITOR7	0x0400
#define POF_SOF_MONITOR8	0x0800
#define POF_SOF_FACING		0x01000		// This subobject always faces you
#define POF_SOF_VIEWER		0x02000		// This subobject is marked as a 'viewer'.
#define POF_SOF_LAYER		0x04000		// This subobject is marked as part of possible secondary model rendering.
#define POF_SOF_WB			0x08000		// This subobject is part of a weapon battery
#define POF_SOF_GLOW			0x0200000	// This subobject glows
#define POF_SOF_CUSTOM		0x0400000	// This subobject has textures/colors that are customizable
#define POF_SOF_THRUSTER	0x0800000   // This is a thruster subobject
#define POF_SOF_JITTER		0x01000000  // This tObject jitters by itself
#define POF_SOF_HEADLIGHT	0x02000000	// This suboject is a headlight

#define POF_MAX_GUNS_PER_MODEL	      64
#define POF_MAX_SUBOBJECTS		         30
#define POF_MAX_POINTS_PER_SUBOBJECT	300

#define POF_WB_INDEX_SHIFT		16       // bits to shift over to get the weapon battery index (after masking out flags)
#define POF_SOF_WB_MASKS		0x01F0000// Room for 32 weapon batteries (currently we only use 21 slots)

#define POF_SOF_MONITOR_MASK	0x0ff0	// mask for monitors

//------------------------------------------------------------------------------

#define POF_PAGENAME_LEN	35

typedef unsigned short tPOF_angle;	//make sure this matches up with fix.h

typedef char tPOF_chunkType [4];

typedef struct tPOF_rgb {
	ubyte		r, g, b;
} tPOF_rgb;

typedef struct tPOF_chunkHeader {
	tPOF_chunkType		nType;
	int					nLength;
} tPOF_chunkHeader;

typedef tPOF_string	char [9];

typedef struct tPOF_textures {
	int					nTextures;
	tPOF_string			*pszNames;
	grsBitmap			*pBitmaps;
} tPOF_textures;

typedef struct tPOF_faceVert {
	int					nIndex;
	float					fu;
	float					fv;
} tPOF_faceVert;

typedef struct tPOF_face {
	tPOF_vector			vNormal;
	int					nVerts;
	int					bTextured;
	union {
		int				nTexId;
		tPOF_rgb			color;
		} texProps;
	tPOF_faceVert		*pVerts;
	float					fBoundingLength;
	float					fBoundingWidth;
	tPOF_vector			vMin;
	tPOF_vector			vMax;
	ubyte					bFront;
} tPOF_face;

typedef struct tPOF_faceList {
	int					nFaces;
	tPOF_face			*pFaces;
	tPOF_faceVert		*pFaceVerts;
} tPOF_faceList;

typedef struct tPOF_glowInfo {
	tPOF_frgb			color;
	float					fSize;
	float					fLength;
	tPOF_vector			vCenter;
	tPOF_vector			vNormal;
} tPOF_glowInfo;

typedef struct tPOF_specialPoint {
	char					*pszName;
	char					*pszProps;
	tPOF_vector			vPos;
	float					fRadius;
} tPOF_specialPoint;

typedef struct tPOF_specialList {
	int					nVerts;
	tPOF_specialPoint	*pVerts;
} tPOF_specialList;

typedef struct tPOF_point {
   int					nParent;
   tPOF_vector			vPos;
   tPOF_vector			vDir;
} tPOF_point;

typedef struct tPOF_pointList {
   int					nPoints;        
   tPOF_point			*pPoints;
} tPOF_pointList;

typedef struct tPOF_attachPoint {
	tPOF_point			point;
	tPOF_vector			vu;
	char					bu;
} tPOF_attachPoint;

typedef struct tPOF_attachList {
   int					nPoints;        
   tPOF_attachPoint	*pPoints;
} tPOF_attachList;

typedef struct tPOF_battery {
	int					nVerts;
	int					*pVertIndex;
	int					nTurrets;
	int					*pTurretIndex;
} tPOF_battery;

typedef struct tPOF_armament {
	int					nBatts;
	tPOF_battery		*pBatts;
} tPOF_armament;

typedef struct tPOF_frameInfo {
	int					nFrames;
	int					nFirstFrame;
	int					nLastFrame;
} tPOF_frameInfo;

typedef struct tPOF_posFrame {
	int					iKeyFrame;
	tPOF_vector			vPos;
	int					nStartTime;
} tPOF_posFrame;

typedef struct tPOF_posAnim {
	tPOF_frameInfo		frameInfo;
	tPOF_posFrame		*pFrames;
	int					nTicks;
	ubyte					*pRemapTicks;
} tPOF_posAnim;

typedef struct tPOF_rotFrame {
	int					iKeyFrame;
	tPOF_vector			vAxis;
	int					nAngle;
	tPOF_matrix			mMat;
	int					nStartTime;
} tPOF_rotFrame;

typedef struct tPOF_rotAnim {
	tPOF_frameInfo		frameInfo;
	tPOF_rotFrame		*pFrames;
	int					nTicks;
	ubyte					*pRemapTicks;
} tPOF_rotAnim;

typedef struct tPOF_edge {
	int					v0;
	int					v1;
	tPOF_face			*pf0;
	tPOF_face			*pf1;
	char					bSilhouette;
} tPOF_edge;

typedef struct tPOF_edgeList {
	int					nEdges;
	tPOF_edge			*pEdges;
} tPOF_edgeList;

typedef struct tPOFSubObject {
	short					nIndex;
	short					nParent;
	vmsVector			vClipPlaneNormal;
	vmsVector			vClipPlanePoint;
	int					nRadius;
	int					nOffset;
} tPOFSubObject;

typedef struct tPOFObject {
	int					nVersion;
	int					nFlags;
	int					nRadius;
	vmsVector			vMin;
	vmsVector			vMax;
	int					nDetailLevels;
	int					nSubObjects;
	tPOFSubObject		*pSubObjects;
	tPOF_pointList		gunPoints;
	tPOF_attachList	attachPoints;
	tPOF_specialList	specialPoints;
	tPOF_armament		armament;
	tPOF_textures		textures;
	tPOF_frameInfo		frameInfo;
} tPOFObject;

int POF_ReadFile (char *pszFile, tPOFObject *po);
int POF_FreeObject (tPOFObject *po);
int POF_Render (tPOFObject *po, vmsVector *vPos, vmsMatrix *mOrient, float *fLight);
float *POF_MatVms2Gl (float *pDest, vmsMatrix *pSrc);
float *POF_VecVms2Gl (float *pDest, vmsVector *pSrc);
tPOF_vector *POF_VecSub (tPOF_vector *pvDest, tPOF_vector *pvMin, tPOF_vector *pvSub);
float POF_VecMag (tPOF_vector *pv);
float *POF_GlIdent (float *pm);
float *POF_GlInverse (float *pDest, float *pSrc);

#endif //__POF_H
//eof
