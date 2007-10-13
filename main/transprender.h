#ifndef _TRANSPRENDER_H
#define _TRANSPRENDER_H

#include "inferno.h"

//------------------------------------------------------------------------------

#define ITEM_DEPTHBUFFER_SIZE	100000
#define ITEM_BUFFER_SIZE		100000

typedef enum tRenderItemType {
	riPoly,
	riSprite,
	riSphere,
	riParticle,
	riLightning,
	riLightningSegment,
	riThruster
} tRenderItemType;

typedef struct tRIPoly {
	grsBitmap			*bmP;
	fVector				vertices [4];
	tTexCoord3f			texCoord [4];
	tRgbaColorf			color [4];
	short					sideLength [4];
	int					nWrap;
	int					nPrimitive;
	char					nVertices;
	char					nColors;
	char					bDepthMask;
	char					bAdditive;
} tRIPoly;

typedef struct tRISprite {
	grsBitmap			*bmP;
	fVector				position;
	tRgbaColorf			color;
	int					nWidth;
	int					nHeight;
	char					nFrame;
	char					bColor;
	char					bAdditive;
} tRISprite;

typedef struct tRIParticle {
	tParticle			*particle;
	float					fBrightness;
} tRIParticle;

typedef enum tRISphereType {
	riSphereShield,
	riMonsterball
} tRISphereType;

typedef struct tRISphere {
	tRISphereType		nType;
	tRgbaColorf			color;
	tObject				*objP;
} tRISphere;

typedef struct tRILightning {
	tLightning			*lightning;
	short					nLightnings;
	short					nDepth;
} tRILightning;

typedef struct tRILightningSegment {
	fVector					vLine [2];
	fVector					vPlasma [4];
	tRgbaColorf				color;
	short						nDepth;
	char						bStart;
	char						bEnd;
	char						bPlasma;
} tRILightningSegment;

typedef struct tRIThruster {
	grsBitmap				*bmP;
	fVector					vertices [7];
	tTexCoord3f						texCoord [7];
	char						bFlame;
} tRIThruster;

typedef struct tRenderItem {
	struct tRenderItem	*pNextItem;
	tRenderItemType		nType;
	int						z;
	union {
		tRIPoly					poly;
		tRISprite				sprite;
		tRIParticle				particle;
		tRISphere				sphere;
		tRILightning			lightning;
		tRILightningSegment	lightningSegment;
		tRIThruster				thruster;
	} item;
} tRenderItem;

typedef struct tRenderItemBuffer {
	tRenderItem		**pDepthBuffer;
	tRenderItem		*pItemList;
	int				nItems;
	int				nFreeItems;
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
	grsBitmap		*bmP;
} tRenderItemBuffer;

int AllocRenderItemBuffer (void);
void FreeRenderItemBuffer (void);
void ResetRenderItemBuffer (void);
void InitRenderItemBuffer (int zMin, int zMax);
int AddRenderItem (tRenderItemType nType, void *itemData, int itemSize, int nDepth, int nIndex);
int RIAddFace (grsFace *faceP);
int RIAddPoly (grsBitmap *bmP, fVector *vertices, char nVertices, tTexCoord3f *texCoord, tRgbaColorf *color, 
					tFaceColor *altColor, char nColors, char bDepthMask, int nPrimitive, int nWrap, int bAdditive);
int RIAddSprite (grsBitmap *bmP, vmsVector *position, tRgbaColorf *color, int nWidth, int nHeight, char nFrame, char bAdditive);
int RIAddSphere (tRISphereType nType, float red, float green, float blue, float alpha, tObject *objP);
int RIAddParticle (tParticle *particle, float fBrightness);
int RIAddLightnings (tLightning *lightnings, short nLightnings, short nDepth);
int RIAddLightningSegment (fVector *vLine, fVector *vPlasma, tRgbaColorf *color, 
									char bPlasma, char bStart, char bEnd, short nDepth);
int RIAddThruster (grsBitmap *bmP, fVector *vThruster, tTexCoord3f *uvlThruster, fVector *vFlame, tTexCoord3f *uvlFlame);
void RenderItems (void);
void StartRenderThreads (void);
void EndRenderThreads (void);

extern tRenderItemBuffer renderItems;

//------------------------------------------------------------------------------

#endif /* _TRANSPRENDER_H */
