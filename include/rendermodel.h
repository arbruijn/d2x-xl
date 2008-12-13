/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header file for Inferno.  Should be included in all source files.
 *
 */

#ifndef _RENDERMODEL_H
#define _RENDERMODEL_H

#include <math.h>

#include "inferno.h"
#include "carray.h"

namespace RenderModel {

//	-----------------------------------------------------------------------------

#define G3_BUFFER_OFFSET(_i)	(GLvoid *) ((char *) NULL + (_i))

class CRenderVertex {
	public:
		fVector3					vertex;
		fVector3					normal;
		tRgbaColorf				color;
		tTexCoord2f				texCoord;
		};

class CVertex {
	public:
		tTexCoord2f				texCoord;
		tRgbaColorf				renderColor;
		fVector3					vertex;
		fVector3					normal;
		tRgbaColorf				baseColor;
		short						nIndex;
		char						bTextured;
	};

inline int operator- (RenderModel::CVertex* f, CArray<RenderModel::CVertex>& a) { return a.Index (f); }

class CFace {
	public:
		CFixVector				vNormal;
		short						nVerts;
		short						nBitmap;
		short						nIndex;
		short						nId;
		ubyte						nSubModel;
		ubyte						bGlow :1;
		ubyte						bThruster :1;
	};

inline int operator- (RenderModel::CFace* f, CArray<RenderModel::CFace>& a) { return a.Index (f); }

class CSubModel {
	public:
#if DBG
		char						szName [256];
#endif
		CFixVector				vOffset;
		CFixVector				vCenter;
		fVector3					vMin;
		fVector3					vMax;
		RenderModel::CFace*	faces;
		short						nParent;
		short						nFaces;
		short						nIndex;
		short						nBitmap;
		short						nHitbox;
		int						nRad;
		ushort					nAngles;
		ubyte						bRender :1;
		ubyte						bGlow :1;
		ubyte						bThruster :1;
		ubyte						bWeapon :1;
		ubyte						bBullets :1;
		ubyte						nType :2;
		char						nGunPoint;
		char						nGun;
		char						nBomb;
		char						nMissile;
		char						nWeaponPos;
		ubyte						nFrames;
		ubyte						iFrame;
		time_t					tFrame;
};

inline int operator- (RenderModel::CSubModel* f, CArray<RenderModel::CSubModel>& a) { return a.Index (f); }

class CVertNorm {
	public:
		fVector3	vNormal;
		ubyte		nVerts;
	};


class CModel {
	public:

	public:
		CArray<CBitmap>						textures;
		int										teamTextures [8];
		CArray<fVector3>						verts;
		CArray<fVector3>						vertNorms;
		CArray<tFaceColor>					color;
		CArray<RenderModel::CVertex>		faceVerts;
		CArray<RenderModel::CVertex>		sortedVerts;
		CArray<ubyte>							vbData;
		CArray<tTexCoord2f>					vbTexCoord;
		CArray<tRgbaColorf>					vbColor;
		CArray<fVector3>						vbVerts;
		CArray<fVector3>						vbNormals;
		CArray<RenderModel::CSubModel>	subModels;
		CArray<RenderModel::CFace>			faces;
		CArray<RenderModel::CVertex>		vertBuf [2];
		CArray<short>							index [2];
		short										nGunSubModels [MAX_GUNS];
		float										fScale;
		short										nType; //-1: custom mode, 0: default model, 1: alternative model, 2: hires model
		short										nFaces;
		short										iFace;
		short										nVerts;
		short										nFaceVerts;
		short										iFaceVert;
		short										nSubModels;
		short										nTextures;
		short										iSubModel;
		short										bHasTransparency;
		short										bValid;
		short										bRendered;
		short										bBullets;
		CFixVector								vBullets;
		GLuint									vboDataHandle;
		GLuint									vboIndexHandle;

	public:
	};	

//	-----------------------------------------------------------------------------------------------------------

} //RenderModel

#endif


