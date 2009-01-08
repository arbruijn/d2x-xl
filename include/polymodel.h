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

#ifndef _POLYOBJ_H
#define _POLYOBJ_H

#include "vecmat.h"
#include "gr.h"
#include "3d.h"

#include "robot.h"
#include "piggy.h"

//------------------------------------------------------------------------------

#define MAX_POLYGON_MODELS		500
#define D1_MAX_POLYGON_MODELS 300

#define SLOWMOTION_MODEL		(MAX_POLYGON_MODELS - 33)
#define BULLETTIME_MODEL		(MAX_POLYGON_MODELS - 34)
#define HOSTAGE_MODEL			(MAX_POLYGON_MODELS - 35)
#define BULLET_MODEL				(MAX_POLYGON_MODELS - 36)

//------------------------------------------------------------------------------

class CSubModelData {
	public:
		CArray<int>				ptrs ;//[MAX_SUBMODELS];
		CArray<CFixVector>	offsets ;//[MAX_SUBMODELS];
		CArray<CFixVector>	norms ;//[MAX_SUBMODELS];   // norm for sep plane
		CArray<CFixVector>	pnts ;//[MAX_SUBMODELS];    // point on sep plane
		CArray<fix>				rads ;//[MAX_SUBMODELS];       // radius for each submodel
		CArray<ubyte>			parents ;//[MAX_SUBMODELS];    // what is parent for each submodel
		CArray<CFixVector>	mins ;//[MAX_SUBMODELS];
		CArray<CFixVector>	maxs ;//[MAX_SUBMODELS];

	public:
		CSubModelData () { Create (); }
		void Create (void);
		void Setup (ubyte* dataP);
		void Destroy (void);
	};

//------------------------------------------------------------------------------
//used to describe a polygon model

typedef struct tPolyModelInfo {
		short				nType;
		int				nModels;
		int				nDataSize;
		CSubModelData	subModels;
		CFixVector		mins, maxs;                       // min,max for whole model
		fix				rad;
		ubyte				nTextures;
		ushort			nFirstTexture;
		ubyte				nSimplerModel;                      // alternate model with less detail (0 if none, nModel+1 else)
} tPolyModelInfo;

class CPolyModel : public CByteArray {
	private:
		tPolyModelInfo	m_info;

	public:
		CPolyModel () { Init (); }
		void Init (void);
		int Read (int bHMEL, CFile& cf);
		void ReadData (CPolyModel* defModelP, CFile& cf);
		void Load (const char *filename, int nTextures, int nFirstTexture, tRobotInfo *botInfoP);
		int LoadTextures (tBitmapIndex*	altTextures);
		void FindMinMax (void);
		fix Size (void);

		inline short Type (void) { return m_info.nType; }
		inline void SetType (short nType) { m_info.nType = nType; }
		inline int DataSize (void) { return m_info.nDataSize; }
		inline void SetDataSize (int nDataSize) { m_info.nDataSize = nDataSize; }
		inline ubyte SimplerModel (void) { return m_info.nSimplerModel; }
		inline fix Rad (void) { return m_info.rad; }
		inline void SetRad (fix rad) { m_info.rad = rad; }
		inline ubyte* Data (void) { return Buffer (); }
		inline int ModelCount (void) { return m_info.nModels; }
		inline CSubModelData& SubModels (void) { return m_info.subModels; }
		inline ushort FirstTexture (void) { return m_info.nFirstTexture; }
		inline void SetFirstTexture (ushort nFirstTexture) { m_info.nFirstTexture = nFirstTexture; }
		inline ubyte TextureCount (void) { return m_info.nTextures; }
		inline tPolyModelInfo& Info (void) { return m_info; }
#if 0
		inline CPolyModel& operator= (CPolyModel& other) { 
			m_info.data = other.m_info.data; 
			return *this;
			}
#endif

	private:
		int	m_fileEnd;
		int	m_filePos;

		void POF_Seek (int len, int nType);
		size_t POF_Read (void *dst, size_t elsize, size_t nelem, ubyte *bufP);
		int POF_ReadInt (ubyte *bufP);
		short POF_ReadShort (ubyte *bufP);
		void POF_ReadString (char *buf, int max_char, ubyte *bufP);
		void POF_ReadVecs (CFixVector *vecs, int n, ubyte *bufP);
		void POF_ReadAngs (CAngleVector *angs, int n, ubyte *bufP);

		void Parse (const char *filename, tRobotInfo *botInfoP);
		void Check (ubyte* dataP);
		void Setup (void);
	};

//------------------------------------------------------------------------------
// array of pointers to polygon objects
// switch to simpler model when the CObject has depth
// greater than this value times its radius.
extern int nSimpleModelThresholdScale;

// how many polygon objects there are
// array of names of currently-loaded models
extern char pofNames [MAX_POLYGON_MODELS][SHORT_FILENAME_LEN];

int LoadPolyModel (const char* filename, int nTextures, int nFirstTexture, tRobotInfo* botInfoP);

// draw a polygon model
int DrawPolyModel (CObject* objP, CFixVector* pos, CFixMatrix* orient, CAngleVector* animAngles, int nModel, int flags, fix light, 
							 fix* glowValues, tBitmapIndex nAltTextures[], tRgbaColorf* obj_color);

// draws the given model in the current canvas.  The distance is set to
// more-or-less fill the canvas.  Note that this routine actually renders
// into an off-screen canvas that it creates, then copies to the current
// canvas.
void DrawModelPicture (int mn,CAngleVector* orient_angles);

// free up a model, getting rid of all its memory
#define MAX_POLYOBJ_TEXTURES 100

int ReadPolyModels (CPolyModel* pm, int n, CFile& cf);

CPolyModel* GetPolyModel (CObject* objP, CFixVector* pos, int nModel, int flags);

//	-----------------------------------------------------------------------------

#endif // _POLYOBJ_H
