#ifndef _TRANSPRENDER_H
#define _TRANSPRENDER_H

#include "particles.h"
#include "lightning.h"

//------------------------------------------------------------------------------

#define ITEM_DEPTHBUFFER_SIZE	100000
#define ITEM_BUFFER_SIZE		100000

typedef enum tTranspItemType {
	tiSprite,
	tiSpark,
	tiSphere,
	tiParticle,
	tiLightning,
	tiThruster,
	tiObject,
	tiPoly,
	tiTexPoly,
	tiFlatPoly,
} tTranspItemType;

typedef struct tTranspPoly {
	grsFace				*faceP;
	grsTriangle			*triP;
	CBitmap				*bmP;
	fVector				vertices [4];
	tTexCoord2f			texCoord [4];
	tRgbaColorf			color [4];
	short					sideLength [4];
	short					nSegment;
	int					nWrap;
	int					nPrimitive;
	char					nVertices;
	char					nColors;
	char					bDepthMask;
	char					bAdditive;
} tTranspPoly;

typedef struct tTranspObject {
	tObject				*objP;
} tTranspObject;

typedef struct tTranspSprite {
	CBitmap				*bmP;
	fVector				position;
	tRgbaColorf			color;
	int					nWidth;
	int					nHeight;
	char					nFrame;
	char					bColor;
	char					bAdditive;
	char					bDepthMask;
	float					fSoftRad;
} tTranspSprite;

typedef struct tTranspSpark {
	fVector				position;
	int					nSize;
	char					nFrame;
	char					nType;
} tTranspSpark;

typedef struct tTranspParticle {
	CParticle			*particle;
	float					fBrightness;
} tTranspParticle;

typedef enum tTranspSphereType {
	riSphereShield,
	riMonsterball
} tTranspSphereType;

typedef struct tTranspSphere {
	tTranspSphereType	nType;
	tRgbaColorf			color;
	tObject				*objP;
} tTranspSphere;

typedef struct tTranspLightning {
	CLightning			*lightning;
	short					nDepth;
} tTranspLightning;

typedef struct tTranspLightTrail {
	CBitmap					*bmP;
	fVector					vertices [7];
	tTexCoord2f				texCoord [7];
	tRgbaColorf				color;
	char						bTrail;
} tTranspLightTrail;

typedef struct tTranspItem {
	struct tTranspItem	*pNextItem;
	struct tTranspItem	*parentP;
	tTranspItemType		nType;
	int						nItem;
	int						z;
	bool						bValid;
	bool						bRendered;
	union {
		tTranspPoly					poly;
		tTranspObject				object;
		tTranspSprite				sprite;
		tTranspSpark				spark;
		tTranspParticle			particle;
		tTranspSphere				sphere;
		tTranspLightning			lightning;
		tTranspLightTrail			thruster;
	} item;
} tTranspItem;

typedef struct tTranspItemBuffer {
	tTranspItem		**depthBufP;
	tTranspItem		*itemListP;
	int				nMinOffs;
	int				nMaxOffs;
	int				nItems;
	int				nFreeItems;
	int				nCurType;
	int				nPrevType;
	int				zMin;
	int				zMax;
	double			zScale;
	int				nWrap;
	char				nFrame;
	char				bClientState;
	char				bTextured;
	char				bClientColor;
	char				bClientTexCoord;
	char				bDepthMask;
	char				bDisplay;
	char				bHaveParticles;
	char				bLightmaps;
	char				bUseLightmaps;
	char				bDecal;
	char				bSplitPolys;
	CBitmap			*bmP [3];
} tTranspItemBuffer;

typedef struct tTranspItemData {
	tTranspItem		item;
	int				nType;
	int				nSize;
	int				nDepth;
	int				nIndex;
	} tTranspItemData;

int AllocTranspItemBuffer (void);
void FreeTranspItemBuffer (void);
void ResetTranspItemBuffer (void);
void InitTranspItemBuffer (int zMin, int zMax);
int AddTranspItem (tTranspItemType nType, void *itemData, int itemSize, int nDepth, int nIndex);
int TIAddFace (grsFace *faceP);
int TIAddPoly (grsFace *faceP, grsTriangle *triP, CBitmap *bmP,
					fVector *vertices, char nVertices, tTexCoord2f *texCoord, tRgbaColorf *color,
					tFaceColor *altColor, char nColors, char bDepthMask, int nPrimitive, int nWrap, int bAdditive,
					short nSegment);
int TIAddObject (tObject *objP);
int TIAddSprite (CBitmap *bmP, const vmsVector& position, tRgbaColorf *color,
					  int nWidth, int nHeight, char nFrame, char bAdditive, float fSoftRad);
int TIAddSpark (const vmsVector& position, char nType, int nSize, char nFrame);
int TIAddSphere (tTranspSphereType nType, float red, float green, float blue, float alpha, tObject *objP);
int TIAddParticle (CParticle *particle, float fBrightness, int nThread);
int TIAddLightning (CLightning *lightningP, short nDepth);
int TIAddLightTrail (CBitmap *bmP, fVector *vThruster, tTexCoord2f *tcThruster, fVector *vFlame, tTexCoord2f *tcFlame, tRgbaColorf *colorP);
void RenderTranspItems (void);
void StartRenderThreads (void);
void EndRenderThreads (void);
void FreeTranspItems (void);
tTranspItem *TISetParent (int nChild, int nParent);

extern tTranspItemBuffer transpItems;

//------------------------------------------------------------------------------

#endif /* _TRANSPRENDER_H */
