#ifdef HAVE_CONFIG_H 
#	include <conf.h> 
#endif 

#include "inferno.h"
#include "text.h"
#include "error.h"
#include "cfile.h"
#include "u_mem.h"
#include "interp.h"
#include "oof.h"
#include "newmenu.h"
#include "hiresmodels.h"

// ----------------------------------------------------------------------------

typedef struct tOOFToModel {
	char	*pszOOF;
	char	*pszPOL;
	short	nModel;
	short	nType;
	int	bFlipV;
} tOOFToModel;

static tOOFToModel oofToModel [] = {
	// player ship
	{"\001pyrogl.oof", "pyrogl.pol", 108, 0, 0}, 
	{NULL, NULL, 110, 0, 0}, 	//filename NULL means this is an additional model number to be used with the last listed oof filename
#if 0//def _DEBUG	//D3 robots for testing
/*
	{"\001thresherboss.oof", NULL, 39, 1, 0}, 
	{"\001orbot.oof", NULL, 58, 1, 0}, 
	{"\001thresh.oof", NULL, 60, 1, 0}, 
	{"\001squid.oof", NULL, 68, 1, 0}, 
*/
	{"\001pumpingpipesmall.oof", NULL, 56, 1, 0}, 
#endif
	// robots
	{"\001smallhulk.oof", NULL, 0, 1, 0}, 
	{"\001mediumlifter.oof", NULL, 2, 1, 0}, 
	{"\001spider.oof", NULL, 4, 1, 0}, 
	{"\001class1drone.oof", NULL, 6, 1, 0}, 
	{"\001class2drone.oof", NULL, 8, 1, 0}, 
	{"\001class1driller-cloaked.oof", NULL, 10, 1, 0}, 
	{"\001class2supervisor.oof", NULL, 14, 1, 0}, 
	{"\001secondarylifter.oof", NULL, 15, 1, 0}, 
	{"\001class1heavydriller.oof", NULL, 17, 1, 0}, 
	{"\001class3gopher.oof", NULL, 19, 1, 0}, 
	{"\001class1platform.oof", NULL, 20, 1, 0}, 
	{"\001class2platform.oof", NULL, 21, 1, 0}, 
	{"\001smallspider.oof", NULL, 23, 1, 0}, 
	{"\001fusionhulk.oof", NULL, 25, 1, 0}, 
	{"\001superhulk.oof", NULL, 26, 1, 0}, 
	{"\001d1boss1.oof", NULL, 28, 1, 0}, 
	{NULL, NULL, 50, 1, 0}, 
	{"\001cloakedlifter.oof", NULL, 29, 1, 0}, 
	{"\001class1driller.oof", NULL, 31, 1, 0}, 
	{"\001mediumhulk.oof", NULL, 33, 1, 0}, 
	{"\001advancedlifter.oof", NULL, 35, 1, 0}, 
	{"\001ptmcdefense.oof", NULL, 37, 1, 0}, 
	{"\001d1boss2.oof", NULL, 39, 1, 0}, 
	{"\001bper.oof", NULL, 40, 1, 0}, 
	{"\001smelter.oof", NULL, 41, 1, 0}, 
	{"\001icespider.oof", NULL, 43, 1, 0}, 
	{"\001bulkdestroyer.oof", NULL, 44, 1, 0}, 
	{"\001trnracer.oof", NULL, 45, 1, 0}, 
	{"\001foxattackbot.oof", NULL, 46, 1, 0}, 
	{"\001sidarm.oof", NULL, 47, 1, 0}, 
	{"\001redfattyboss.oof", NULL, 49, 1, 0}, 
	{"\001guidebot.oof", NULL, 51, 1, 0}, 
	{"\001eviltwin.oof", NULL, 54, 1, 0}, 
	{"\001itsc.oof", NULL, 55, 1, 0}, 
	{"\001itdroid.oof", NULL, 56, 1, 0}, 
	{"\001pest.oof", NULL, 58, 1, 0}, 
	{"\001pig.oof", NULL, 60, 1, 0}, 
	{"\001d-claw.oof", NULL, 62, 1, 0}, 
	{"\001redhornet.oof", NULL, 64, 1, 0}, 
	{"\001thief.oof", NULL, 65, 1, 0}, 
	{"\001seeker.oof", NULL, 67, 1, 0}, 
	{"\001e-bandit.oof", NULL, 68, 1, 0}, 
	{"\001fireboss.oof", NULL, 69, 1, 0}, 
	{"\001waterboss.oof", NULL, 70, 1, 0}, 
	{"\001boarshead.oof", NULL, 71, 1, 0}, 
	{"\001greenspider.oof", NULL, 72, 1, 0}, 
	{"\001omega.oof", NULL, 73, 1, 0}, 
	{"\001louguard.oof", NULL, 75, 1, 0}, 
	{"\001alienboss1.oof", NULL, 76, 1, 0}, 
	{"\001redfattyjunior.oof", NULL, 77, 1, 0}, 
	{"\001d-claw-cloaked.oof", NULL, 78, 1, 0}, 
	{NULL, NULL, 79, 1, 0}, 
	{"\001smelter-cloaked.oof", NULL, 80, 1, 0}, 
	{"\001smelterclone-cloaked.oof", NULL, 81, 1, 0}, 
	{"\001smelterclone.oof", NULL, 83, 1, 0}, 
	{"\001omegaclone.oof", NULL, 85, 1, 0}, 
	{"\001bperclone.oof", NULL, 86, 1, 0}, 
	{"\001spiderclone.oof", NULL, 87, 1, 0}, 
	{"\001spiderspawn.oof", NULL, 88, 1, 0}, 
	{"\001iceboss.oof", NULL, 89, 1, 0}, 
	{"\001spiderspawnclone.oof", NULL, 90, 1, 0}, 
	{"\001alienboss2.oof", NULL, 91, 1, 0}, 
	// Vertigo robots
	{"\001compactlifter.oof", NULL, 166, 1, 0}, 
	{"\001fervid99.oof", NULL, 167, 1, 0}, 
	{"\001fiddler.oof", NULL, 168, 1, 0}, 
	{"\001class2heavydriller.oof", NULL, 169, 1, 0}, 
	{"\001smelter2.oof", NULL, 170, 1, 0}, 
	{"\001max.oof", NULL, 171, 1, 0}, 
	{"\001sniperng.oof", NULL, 172, 1, 0}, 
	{"\001logikill.oof", NULL, 173, 1, 0}, 
	{"\001canary.oof", NULL, 174, 1, 0}, 
	{"\001vertigoboss.oof", NULL, 175, 1, 0}, 
	{"\001redguard.oof", NULL, 176, 1, 0}, 
	{"\001spike.oof", NULL, 177, 1, 0}, 
	// powerups/missiles
	{"\001concussion.oof", NULL, 127, 0, 1}, 
	{NULL, NULL, 137, 0, 1}, 
	{"\001homer.oof", NULL, 133, 0, 1}, 
	{NULL, NULL, 136, 0, 1}, 
	{"\001smartmsl.oof", NULL, 134, 0, 1}, 
	{NULL, NULL, 162, 0, 1}, 
	{"\001mega.oof", NULL, 135, 0, 1}, 
	{NULL, NULL, 142, 0, 1}, 
	{"\001flashmsl.oof", NULL, 151, 0, 1}, 
	{NULL, NULL, 158, 0, 1}, 
	{NULL, NULL, 165, 0, 1}, 
	{"\001guided.oof", NULL, 152, 0, 1}, 
	{"\001mercury.oof", NULL, 153, 0, 1}, 
	{NULL, NULL, 161, 0, 1}, 
	{"\001eshaker.oof", NULL, 154, 0, 1}, 
	{NULL, NULL, 163, 0, 1}, 
	{"\001shakrsub.oof", NULL, 160, 0, 1},
	{"\001pminepack.oof", "pminpack.pol", MAX_POLYGON_MODELS - 1, 0, 1},
	{"\001proxmine.oof", "proxmine.pol", MAX_POLYGON_MODELS - 2, 0, 1},
	{"\001sminepack.oof", "sminpack.pol", MAX_POLYGON_MODELS - 3, 0, 1},
	{"\001smartmine.oof", "smrtmine.pol", MAX_POLYGON_MODELS - 4, 0, 1},
	{"\001concussion4.oof", "concpack.pol", MAX_POLYGON_MODELS - 5, 0, 1},
	{"\001homer4.oof", "homrpack.pol", MAX_POLYGON_MODELS - 6, 0, 1},
	{"\001flash4.oof", "flshpack.pol", MAX_POLYGON_MODELS - 7, 0, 1},
	{"\001guided4.oof", "guidpack.pol", MAX_POLYGON_MODELS - 8, 0, 1},
	{"\001mercury4.oof", "mercpack.pol", MAX_POLYGON_MODELS - 9, 0, 1},
	{"\001laser.oof", NULL, MAX_POLYGON_MODELS - 10, 0, 1},
	{"\001vulcan.oof", NULL, MAX_POLYGON_MODELS - 11, 0, 1},
	{"\001spreadfire.oof", NULL, MAX_POLYGON_MODELS - 12, 0, 1},
	{"\001plasma.oof", NULL, MAX_POLYGON_MODELS - 13, 0, 1},
	{"\001fusion.oof", NULL, MAX_POLYGON_MODELS - 14, 0, 1},
	{"\001superlaser.oof", NULL, MAX_POLYGON_MODELS - 15, 0, 1},
	{"\001gauss.oof", NULL, MAX_POLYGON_MODELS - 16, 0, 1},
	{"\001helix.oof", NULL, MAX_POLYGON_MODELS - 17, 0, 1},
	{"\001phoenix.oof", NULL, MAX_POLYGON_MODELS - 18, 0, 1},
	{"\001omega.oof", NULL, MAX_POLYGON_MODELS - 19, 0, 1},
	{"\001quadlaser.oof", NULL, MAX_POLYGON_MODELS - 20, 0, 1},
	{"\001afterburner.oof", NULL, MAX_POLYGON_MODELS - 21, 0, 1},
	{"\001headlight.oof", NULL, MAX_POLYGON_MODELS - 22, 0, 1},
	{"\001ammorack.oof", NULL, MAX_POLYGON_MODELS - 23, 0, 1},
	{"\001converter.oof", NULL, MAX_POLYGON_MODELS - 24, 0, 1},
	{"\001fullmap.oof", NULL, MAX_POLYGON_MODELS - 25, 0, 1},
	{"\001cloak.oof", NULL, MAX_POLYGON_MODELS - 26, 0, 1},
	{"\001invul.oof", NULL, MAX_POLYGON_MODELS - 27, 0, 1},
	{"\001extralife.oof", NULL, MAX_POLYGON_MODELS - 28, 0, 1},
	{"\001bluekey.oof", NULL, MAX_POLYGON_MODELS - 29, 0, 1},
	{"\001redkey.oof", NULL, MAX_POLYGON_MODELS - 30, 0, 1},
	{"\001goldkey.oof", NULL, MAX_POLYGON_MODELS - 31, 0, 1},
	{"\001vulcanammo.oof", NULL, MAX_POLYGON_MODELS - 32, 0, 1},
	{"\001hostage.oof", NULL, HOSTAGE_MODEL, 0, 1}
	};

void InitModelToOOF (void)
{
memset (gameData.models.modelToOOF, 0, sizeof (gameData.models.modelToOOF));
memset (gameData.models.modelToPOL, 0, sizeof (gameData.models.modelToPOL));
}

// ----------------------------------------------------------------------------

short LoadLoresModel (short i)
{
	CFILE			*fp;
	tPolyModel	*pm;
	short			nModel, j = sizeofa (oofToModel);
	char			szModel [FILENAME_LEN];

sprintf (szModel, "model%d.pol", oofToModel [i].nModel);
if (!(oofToModel [i].pszPOL && 
	  ((fp = CFOpen (oofToModel [i].pszPOL, gameFolders.szDataDir, "rb", 0)) ||
	   (fp = CFOpen (szModel, gameFolders.szDataDir, "rb", 0)))))
	return ++i;
nModel = oofToModel [i].nModel;
pm = ((gameStates.app.bFixModels && gameStates.app.bAltModels) ? gameData.models.altPolyModels : gameData.models.polyModels) + nModel;
if (!PolyModelRead (pm, fp, 1)) {
	CFClose (fp);
	return ++i;
	}
pm->modelData = 
pm->modelData = NULL;
PolyModelDataRead (pm, nModel, gameData.models.defPolyModels + nModel, fp);
CFClose (fp);
pm->rad = G3PolyModelSize (pm, nModel);
do {
	gameData.models.modelToPOL [oofToModel [i].nModel] = pm;
	} while ((++i < j) && !oofToModel [i].pszOOF);
gameData.models.nLoresModels++;
return i;
}

// ----------------------------------------------------------------------------

short LoadHiresModel (tOOFObject *po, short i, int bCustom)
{
	short	j = sizeofa (oofToModel);
	char	szModel [FILENAME_LEN];

sprintf (szModel, "\001model%d.oof", oofToModel [i].nModel);
#ifdef _DEBUG
if (oofToModel [i].pszOOF && !strcmp (oofToModel [i].pszOOF, "squid.oof"))
	j = j;
#endif
#if OOF_TEST_CUBE
if (!strcmp (oofToModel [i].pszOOF + 1, "pyrogl.oof"))
	oofToModel [i].pszOOF = "cube.oof";
#endif
if (!(oofToModel [i].pszOOF && 
	   (OOF_ReadFile (oofToModel [i].pszOOF + !bCustom, po, oofToModel [i].nType, oofToModel [i].bFlipV, bCustom) || 
	    OOF_ReadFile (szModel + !bCustom, po, oofToModel [i].nType, oofToModel [i].bFlipV, bCustom))))
	return bCustom ? ++i : LoadLoresModel (i);
do {
	CBP (!oofToModel [i].nModel);
	gameData.models.modelToOOF [bCustom][oofToModel [i].nModel] = po;
	} while ((++i < j) && !oofToModel [i].pszOOF);
gameData.models.nHiresModels++;
return i;
}

//------------------------------------------------------------------------------

static int loadIdx;
static int loadOp = 0;

static void LoadModelsPoll (int nItems, tMenuItem *m, int *key, int cItem)
{
GrPaletteStepLoad (NULL);
if (loadOp == 0) {
	loadIdx = LoadHiresModel (gameData.models.hiresModels [0] + gameData.models.nHiresModels, loadIdx, 0);
	if (loadIdx >= sizeofa (oofToModel)) {
		loadOp = 1;
		loadIdx = 0;
		}
	}
else if (loadOp == 1) {
	loadIdx = LoadLoresModel (loadIdx);
	if (loadIdx >= sizeofa (oofToModel)) {
		*key = -2;
		GrPaletteStepLoad (NULL);
		return;
		}
	}
m [0].value++;
m [0].rebuild = 1;
*key = 0;
GrPaletteStepLoad (NULL);
}

//------------------------------------------------------------------------------

int ModelsGaugeSize (void)
{
	int h, i, j;

for (h = i = 0, j = sizeofa (oofToModel); i < j; i++)
	if (oofToModel [i].pszOOF) {
		if (gameOpts->render.bHiresModels)
			h++;
		if (oofToModel [i].pszPOL)
			h++;
		}
return h = (gameOpts->render.bHiresModels + 1) * (sizeofa (oofToModel) - 1);
}

//------------------------------------------------------------------------------

void LoadModelsGauge (void)
{
loadIdx = 0;
loadOp = gameOpts->render.bHiresModels ? 0 : 1;
NMProgressBar (TXT_LOADING_MODELS, 0, ModelsGaugeSize (), LoadModelsPoll); 
}

// ----------------------------------------------------------------------------

void LoadHiresModels (int bCustom)
{
if (!bCustom) {
	InitModelToOOF ();
	gameData.models.nHiresModels = 0;
	}
if (gameStates.app.bNostalgia)
	gameOpts->render.bHiresModels = 0;
else /*if (gameOpts->render.bHiresModels)*/ {
	if (!bCustom && gameStates.app.bProgressBars && gameOpts->menus.nStyle)
		LoadModelsGauge ();
	else {
		short	i = 0, j = sizeofa (oofToModel);
		if (!bCustom)
			ShowBoxedMessage (TXT_LOADING_MODELS);
		if (gameOpts->render.bHiresModels) {
			while (i < j)
				i = LoadHiresModel (gameData.models.hiresModels [bCustom] + gameData.models.nHiresModels, i, bCustom);
			i = 0;
			}
		if (!bCustom) {
			while (i < j)
				i = LoadLoresModel (i);
			}
		}
	ClearBoxedMessage ();
	}
}

// ----------------------------------------------------------------------------

void FreeHiresModels (int bCustom)
{
	int	i, j;

for (i = 0; i < gameData.models.nHiresModels; i++)
	for (j = bCustom; j < 2; j++)
		OOF_FreeObject (gameData.models.hiresModels [j] + i);
}

// ----------------------------------------------------------------------------
// eof
