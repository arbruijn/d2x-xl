/* $Id: polyobj.h,v 1.7 2003/10/10 09:36:35 btb Exp $ */
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

#ifndef DRIVE
#include "robot.h"
#endif
#include "piggy.h"

#define MAX_POLYGON_MODELS 500
#define D1_MAX_POLYGON_MODELS 300

typedef struct tSubModelData {
	int			ptrs [MAX_SUBMODELS];
	vmsVector	offsets [MAX_SUBMODELS];
	vmsVector	norms [MAX_SUBMODELS];   // norm for sep plane
	vmsVector	pnts [MAX_SUBMODELS];    // point on sep plane
	fix			rads [MAX_SUBMODELS];       // radius for each submodel
	ubyte			parents [MAX_SUBMODELS];    // what is parent for each submodel
	vmsVector	mins [MAX_SUBMODELS];
	vmsVector	maxs [MAX_SUBMODELS];
} tSubModelData;

//used to describe a polygon model
typedef struct tPolyModel {
	int				nModels;
	int				nDataSize;
	ubyte				*modelData;
	tSubModelData	subModels;
	vmsVector		mins,maxs;                       // min,max for whole model
	fix				rad;
	ubyte				nTextures;
	ushort			nFirstTexture;
	ubyte				nSimplerModel;                      // alternate model with less detail (0 if none, nModel+1 else)
	//vmsVector min,max;
} __pack__ tPolyModel;

// array of pointers to polygon objects
// switch to simpler model when the tObject has depth
// greater than this value times its radius.
extern int nSimpleModelThresholdScale;

// how many polygon objects there are
// array of names of currently-loaded models
extern char Pof_names[MAX_POLYGON_MODELS][SHORT_FILENAME_LEN];

void InitPolygonModels();

#ifndef DRIVE
int LoadPolygonModel(char *filename,int n_textures,int first_texture,tRobotInfo *r);
#else
int LoadPolygonModel(char *filename,int n_textures,grsBitmap ***textures);
#endif

// draw a polygon model
void DrawPolygonModel (tObject *objP, vmsVector *pos,vmsMatrix *orient,vmsAngVec *animAngles, int nModel, int flags, fix light, 
							  fix *glowValues, tBitmapIndex nAltTextures[], tRgbaColorf *obj_color);

// fills in arrays gunPoints & gun_dirs, returns the number of guns read
int read_model_guns(char *filename,vmsVector *gunPoints, vmsVector *gun_dirs, int *gunSubModels);

// draws the given model in the current canvas.  The distance is set to
// more-or-less fill the canvas.  Note that this routine actually renders
// into an off-screen canvas that it creates, then copies to the current
// canvas.
void DrawModelPicture(int mn,vmsAngVec *orient_angles);

// free up a model, getting rid of all its memory
void FreeModel (tPolyModel *po);

#define MAX_POLYOBJ_TEXTURES 100

#if 0
#define PolyModelRead(pm, fp, bHMEL) CFRead(pm, sizeof(tPolyModel), 1, fp)
#define PolyModelReadN(pm, n, fp, bHMEL) CFRead(pm, sizeof(tPolyModel), n, fp)
#else
/*
 * reads a tPolyModel structure from a CFILE
 */
int PolyModelRead(tPolyModel *pm, CFILE *fp, int bHMEL);

/*
 * reads n tPolyModel structs from a CFILE
 */
extern int PolyModelReadN(tPolyModel *pm, int n, CFILE *fp);
#endif

/*
 * routine which allocates, reads, and inits a tPolyModel's modelData
 */
void PolyModelDataRead (tPolyModel *pm, int nModel, tPolyModel *pdm, CFILE *fp);

tPolyModel *GetPolyModel (tObject *objP, vmsVector *pos, int nModel, int flags);

int LoadModelTextures (tPolyModel *po, tBitmapIndex *altTextures);

#endif /* _POLYOBJ_H */
