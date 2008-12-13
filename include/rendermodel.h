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
		CFixVector				m_vNormal;
		short						m_nVerts;
		short						m_nBitmap;
		CBitmap*					m_textureP;
		short						m_nIndex;
		short						m_nId;
		ubyte						m_nSubModel;
		ubyte						m_bGlow :1;
		ubyte						m_bThruster :1;

	public:
		inline const bool CFace::operator< (CFace& other);
		inline const bool CFace::operator> (CFace& other);
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
		CArray<CBitmap>						m_textures;
		int										m_teamTextures [8];
		CArray<fVector3>						m_verts;
		CArray<fVector3>						m_vertNorms;
		CArray<tFaceColor>					m_color;
		CArray<RenderModel::CVertex>		m_faceVerts;
		CArray<RenderModel::CVertex>		m_sortedVerts;
		CArray<ubyte>							m_vbData;
		CArray<tTexCoord2f>					m_vbTexCoord;
		CArray<tRgbaColorf>					m_vbColor;
		CArray<fVector3>						m_vbVerts;
		CArray<fVector3>						m_vbNormals;
		CArray<RenderModel::CSubModel>	m_subModels;
		CArray<RenderModel::CFace>			m_faces;
		CArray<RenderModel::CVertex>		m_vertBuf [2];
		CArray<short>							m_index [2];
		short										m_nGunSubModels [MAX_GUNS];
		float										m_fScale;
		short										m_nType; //-1: custom mode, 0: default model, 1: alternative model, 2: hires model
		short										m_nFaces;
		short										m_iFace;
		short										m_nVerts;
		short										m_nFaceVerts;
		short										m_iFaceVert;
		short										m_nSubModels;
		short										m_nTextures;
		short										m_iSubModel;
		short										m_bHasTransparency;
		short										m_bValid;
		short										m_bRendered;
		short										m_bBullets;
		CFixVector								m_vBullets;
		GLuint									m_vboDataHandle;
		GLuint									m_vboIndexHandle;

	public:
		CModel () { Init (); }
		~CModel () { Destroy (); }
		void Init (void);
		void Setup (int bHires, int bSort);
		bool Create (void);
		void Destroy (void);
	};	

//	-----------------------------------------------------------------------------------------------------------

} //RenderModel

#endif


