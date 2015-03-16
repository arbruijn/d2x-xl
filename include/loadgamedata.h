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


#ifndef _LOADGAMEDATA_H
#define _LOADGAMEDATA_H

#include "gr.h"
#include "piggy.h"

#define MAX_TEXTURES    1200
#define D1_MAX_TEXTURES 800

//tmapinfo flags
#define TMI_VOLATILE    1   //this material blows up when hit
#define TMI_WATER       2   //this material is water
#define TMI_FORCE_FIELD 4   //this is force field - flares don't stick
#define TMI_GOAL_BLUE   8   //this is used to remap the blue goal
#define TMI_GOAL_RED    16  //this is used to remap the red goal
#define TMI_GOAL_HOARD  32  //this is used to remap the goals
#define TMI_PRODUCER     64  //this is used to remap the goals

typedef struct {
	uint8_t		flags;     //values defined above
	uint8_t		pad[3];    //keep alignment
	fix			lighting;  //how much light this casts
	fix			damage;    //how much damage being against this does (for lava)
	int16_t		nEffectClip; //the tEffectInfo that changes this, or -1
	int16_t		destroyed; //bitmap to show when destroyed, or -1
	int16_t		slide_u,slide_v;    //slide rates of texture, stored in 8:8 fix
} __pack__ tTexMapInfo;

typedef struct {
	char			filename[13];
	uint8_t		flags;
	fix			lighting;		// 0 to 1
	fix			damage;			//how much damage being against this does
	int32_t		nEffectClip;		//if not -1, the tEffectInfo that changes this   
} __pack__ tTexMapInfoD1;

extern int32_t Num_tmaps;
#ifdef EDITOR
extern int32_t TmapList[MAX_TEXTURES];
#endif

//for each model, a model number for dying & dead variants, or -1 if none
extern int32_t Dying_modelnums[];
extern int32_t Dead_modelnums[];

//the model number of the marker CObject
extern int32_t Marker_model_num;

// Initializes the palette, bitmap system...
int32_t BMInit();
void BMClose();

// Initializes the Texture[] array of bmd_bitmap structures.
void InitTextures();

#define OL_ROBOT            1
#define OL_HOSTAGE          2
#define OL_POWERUP          3
#define OL_CONTROL_CENTER   4
#define OL_PLAYER           5
#define OL_CLUTTER          6   //some sort of misc CObject
#define OL_EXIT             7   //the exit model for external scenes
#define OL_WEAPON           8   //a weapon that can be placed

#define MAX_OBJTYPE         140

#define MAX_OBJ_BITMAPS     610
#define D1_MAX_OBJ_BITMAPS  210

// Initializes all bitmaps from BITMAPS.TBL file.
int32_t bm_init_use_tbl(void);

void BMReadAll (CFile& cf, bool bDefault = true);
void BMReadWeaponInfoD1 (CFile& cf);
void BMReadGameDataD1 (CFile&  cf);
void RestoreDefaultModels (void);
int32_t ComputeAvgPixel (CBitmap *bmP);

void LoadTextureBrightness (const char *pszLevel, int32_t *brightnessP);
int32_t LoadExitModels (void);
int32_t LoadRobotExtensions (const char *fname, char *folder, int32_t nType);
void FreeModelExtensions (void);
int32_t LoadRobotReplacements (const char *pszLevel, const char* pszFolder, int32_t bAddBots, int32_t bOnlyModels, bool bCustom = false, bool bUseHog = true);
int32_t ReadHamFile (bool bDefault = true);
int32_t LoadD2Sounds (bool bCustom = false);
bool LoadD1Sounds (bool bCustom);
void UnloadSounds (int32_t bD1);
void _CDECL_ FreeObjExtensionBitmaps (void);

void InitDefaultShipProps (void);
void SetDefaultShipProps (void);
void SetDefaultWeaponProps (void);


#endif //_LOADGAMEDATA_H
