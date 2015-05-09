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

#define BLUEKEY_MODEL			(MAX_POLYGON_MODELS - 29)
#define REDKEY_MODEL				(MAX_POLYGON_MODELS - 30)
#define GOLDKEY_MODEL			(MAX_POLYGON_MODELS - 31)
#define SLOWMOTION_MODEL		(MAX_POLYGON_MODELS - 33)
#define BULLETTIME_MODEL		(MAX_POLYGON_MODELS - 34)
#define HOSTAGE_MODEL			(MAX_POLYGON_MODELS - 35)
#define BULLET_MODEL				(MAX_POLYGON_MODELS - 36)
#define COCKPIT_MODEL			(MAX_POLYGON_MODELS - 37)

//------------------------------------------------------------------------------

class __pack__ CSubModelData {
	public:
#if 1
		int32_t		ptrs [MAX_SUBMODELS];
		CFixVector	offsets [MAX_SUBMODELS];
		CFixVector	norms [MAX_SUBMODELS];   // norm for sep plane
		CFixVector	pnts [MAX_SUBMODELS];    // point on sep plane
		fix			rads [MAX_SUBMODELS];       // radius for each submodel
		uint8_t		parents [MAX_SUBMODELS];    // what is parent for each submodel
		CFixVector	mins [MAX_SUBMODELS];
		CFixVector	maxs [MAX_SUBMODELS];
#else
		CArray<int32_t>		ptrs ;//[MAX_SUBMODELS];
		CArray<CFixVector>	offsets ;//[MAX_SUBMODELS];
		CArray<CFixVector>	norms ;//[MAX_SUBMODELS];   // norm for sep plane
		CArray<CFixVector>	pnts ;//[MAX_SUBMODELS];    // point on sep plane
		CArray<fix>				rads ;//[MAX_SUBMODELS];       // radius for each submodel
		CArray<uint8_t>		parents ;//[MAX_SUBMODELS];    // what is parent for each submodel
		CArray<CFixVector>	mins ;//[MAX_SUBMODELS];
		CArray<CFixVector>	maxs ;//[MAX_SUBMODELS];
#endif

	public:
		CSubModelData () { Create (); }
		void Create (void);
		void Setup (uint8_t* dataP);
		void Destroy (void);
	};

//------------------------------------------------------------------------------
//used to describe a polygon model

typedef struct tPolyModelInfo {
		uint16_t			nId;
		int16_t			nType;
		int32_t			nModels;
		int32_t			nDataSize;
		CSubModelData	subModels;
		CFixVector		mins, maxs;		// min,max for whole model
		fix				rad [2];			// 0: recomputed rad, 1: original rad from Descent data
		uint8_t			nTextures;
		uint16_t			nFirstTexture;
		uint8_t			nSimplerModel;	// alternate model with less detail (0 if none, nModel+1 else)
		bool				bCustom;
} __pack__ tPolyModelInfo;

class CPolyModel : public CByteArray {
	private:
		tPolyModelInfo	m_info;

	public:
		CPolyModel () { Init (); }
		void Init (void);
		int32_t Read (int32_t bHMEL, int32_t bCustom, CFile& cf);
		void ReadData (CPolyModel* defModelP, CFile& cf);
		void Load (const char *filename, int32_t nTextures, int32_t nFirstTexture, tRobotInfo *pRobotInfo);
		int32_t LoadTextures (tBitmapIndex*	altTextures);
		void FindMinMax (void);
		fix Size (void);

		inline uint16_t Id (void) { return m_info.nId; }
		inline void SetKey (uint16_t nId) { m_info.nId = nId; }
		inline int16_t Type (void) { return m_info.nType; }
		inline void SetType (int16_t nType) { m_info.nType = nType; }
		inline bool Custom (void) { return m_info.bCustom; }
		inline void SetCustom (bool bCustom) { m_info.bCustom = bCustom; }
		inline int32_t DataSize (void) { return m_info.nDataSize; }
		inline void SetDataSize (int32_t nDataSize) { m_info.nDataSize = nDataSize; }
		inline uint8_t SimplerModel (void) { return m_info.nSimplerModel; }
		inline fix Rad (int32_t i = 0) { return m_info.rad [i] ? m_info.rad [i] : m_info.rad [!i]; }
		inline void SetRad (fix rad, int32_t bCustom) { m_info.rad [bCustom] = rad; }
		inline uint8_t* Data (void) { return Buffer (); }
		inline int32_t ModelCount (void) { return m_info.nModels; }
		inline CSubModelData& SubModels (void) { return m_info.subModels; }
		inline uint16_t FirstTexture (void) { return m_info.nFirstTexture; }
		inline void SetFirstTexture (uint16_t nFirstTexture) { m_info.nFirstTexture = nFirstTexture; }
		inline uint8_t TextureCount (void) { return m_info.nTextures; }
		inline tPolyModelInfo& Info (void) { return m_info; }
		inline void ResetBuffer (void) {
			SetBuffer (NULL);
			//m_info.nDataSize = 0;
			}
		inline void Destroy (void) {
			CByteArray::Destroy ();
			ResetBuffer ();
			}
#if 0
		inline CPolyModel& operator= (CPolyModel& other) { 
			m_info.data = other.m_info.data; 
			return *this;
			}
#endif

	private:
		int32_t	m_fileEnd;
		int32_t	m_filePos;

		void POF_Seek (int32_t len, int32_t nType);
		size_t POF_Read (void *dst, size_t elsize, size_t nelem, uint8_t *pBuffer);
		int32_t POF_ReadInt (uint8_t *pBuffer);
		int16_t POF_ReadShort (uint8_t *pBuffer);
		void POF_ReadString (char *buf, int32_t max_char, uint8_t *pBuffer);
		void POF_ReadVecs (CFixVector *vecs, int32_t n, uint8_t *pBuffer);
		void POF_ReadAngs (CAngleVector *angs, int32_t n, uint8_t *pBuffer);

		void Parse (const char *filename, tRobotInfo *pRobotInfo);
		void Check (uint8_t* dataP);
		void Setup (void);
	};

//------------------------------------------------------------------------------
// array of pointers to polygon objects
// switch to simpler model when the CObject has depth
// greater than this value times its radius.
extern int32_t nSimpleModelThresholdScale;

// how many polygon objects there are
// array of names of currently-loaded models
extern char pofNames [MAX_POLYGON_MODELS][SHORT_FILENAME_LEN];

int32_t LoadPolyModel (const char* filename, int32_t nTextures, int32_t nFirstTexture, tRobotInfo* pRobotInfo);

// draw a polygon model
int32_t DrawPolyModel (CObject* pObj, CFixVector* pos, CFixMatrix* orient, CAngleVector* animAngles, int32_t nModel, int32_t flags, fix light, 
							  fix* glowValues, tBitmapIndex nAltTextures[], CFloatVector* obj_color);

// draws the given model in the current canvas.  The distance is set to
// more-or-less fill the canvas.  Note that this routine actually renders
// into an off-screen canvas that it creates, then copies to the current
// canvas.
void DrawModelPicture (int32_t mn,CAngleVector* orient_angles);

// free up a model, getting rid of all its memory
#define MAX_POLYOBJ_TEXTURES 100

int32_t ReadPolyModels (CArray<CPolyModel>& models, int32_t nModels, CFile& cf, int32_t nOffset = 0);

CPolyModel* GetPolyModel (CObject* pObj, CFixVector* pos, int32_t nModel, int32_t flags, int32_t* bCustomModel = NULL);

//	-----------------------------------------------------------------------------

#endif // _POLYOBJ_H
