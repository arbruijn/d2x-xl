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

typedef struct tReplacementModel {
	char	*pszHires;
	char	*pszLores;
	short	nModel;
	short	nType;
	int	bFlipV;
} tReplacementModel;

static tReplacementModel replacementModels [] = {
	// player ship
	{"pyrogl", "pyrogl.pol", 108, 0, 0}, 
	{NULL, NULL, 110, 0, 0}, 	//filename NULL means this is an additional model number to be used with the last listed oof filename
#if 0//def _DEBUG	//D3 robots for testing
/*
	{"thresherboss", NULL, 39, 1, 0}, 
	{"orbot", NULL, 58, 1, 0}, 
	{"thresh", NULL, 60, 1, 0}, 
	{"squid", NULL, 68, 1, 0}, 
*/
	{"pumpingpipesmall", NULL, 56, 1, 0}, 
#endif
	// robots
	{"smallhulk", NULL, 0, 1, 0}, 
	{"mediumlifter", NULL, 2, 1, 0}, 
	{"spider", NULL, 4, 1, 0}, 
	{"class1drone", NULL, 6, 1, 0}, 
	{"class2drone", NULL, 8, 1, 0}, 
	{"class1driller-cloaked", NULL, 10, 1, 0}, 
	{"class2supervisor", NULL, 14, 1, 0}, 
	{"secondarylifter", NULL, 15, 1, 0}, 
	{"class1heavydriller", NULL, 17, 1, 0}, 
	{"class3gopher", NULL, 19, 1, 0}, 
	{"class1platform", NULL, 20, 1, 0}, 
	{"class2platform", NULL, 21, 1, 0}, 
	{"smallspider", NULL, 23, 1, 0}, 
	{"fusionhulk", NULL, 25, 1, 0}, 
	{"superhulk", NULL, 26, 1, 0}, 
	{"d1boss1", NULL, 28, 1, 0}, 
	{NULL, NULL, 50, 1, 0}, 
	{"cloakedlifter", NULL, 29, 1, 0}, 
	{"class1driller", NULL, 31, 1, 0}, 
	{"mediumhulk", NULL, 33, 1, 0}, 
	{"advancedlifter", NULL, 35, 1, 0}, 
	{"ptmcdefense", NULL, 37, 1, 0}, 
	{"d1boss2", NULL, 39, 1, 0}, 
	{"bper", NULL, 40, 1, 0}, 
	{"smelter", NULL, 41, 1, 0}, 
	{"icespider", NULL, 43, 1, 0}, 
	{"bulkdestroyer", NULL, 44, 1, 0}, 
	{"trnracer", NULL, 45, 1, 0}, 
	{"foxattackbot", NULL, 46, 1, 0}, 
	{"sidarm", NULL, 47, 1, 0}, 
	{"redfattyboss", NULL, 49, 1, 0}, 
	{"guidebot", NULL, 51, 1, 0}, 
	{"eviltwin", NULL, 54, 1, 0}, 
	{"itsc", NULL, 55, 1, 0}, 
	{"itdroid", NULL, 56, 1, 0}, 
	{"pest", NULL, 58, 1, 0}, 
	{"pig", NULL, 60, 1, 0}, 
	{"d-claw", NULL, 62, 1, 0}, 
	{"redhornet", NULL, 64, 1, 0}, 
	{"thief", NULL, 65, 1, 0}, 
	{"seeker", NULL, 67, 1, 0}, 
	{"e-bandit", NULL, 68, 1, 0}, 
	{"fireboss", NULL, 69, 1, 0}, 
	{"waterboss", NULL, 70, 1, 0}, 
	{"boarshead", NULL, 71, 1, 0}, 
	{"greenspider", NULL, 72, 1, 0}, 
	{"omega", NULL, 73, 1, 0}, 
	{"louguard", NULL, 75, 1, 0}, 
	{"alienboss1", NULL, 76, 1, 0}, 
	{"redfattyjunior", NULL, 77, 1, 0}, 
	{"d-claw-cloaked", NULL, 78, 1, 0}, 
	{NULL, NULL, 79, 1, 0}, 
	{"smelter-cloaked", NULL, 80, 1, 0}, 
	{"smelterclone-cloaked", NULL, 81, 1, 0}, 
	{"smelterclone", NULL, 83, 1, 0}, 
	{"omegaclone", NULL, 85, 1, 0}, 
	{"bperclone", NULL, 86, 1, 0}, 
	{"spiderclone", NULL, 87, 1, 0}, 
	{"spiderspawn", NULL, 88, 1, 0}, 
	{"iceboss", NULL, 89, 1, 0}, 
	{"spiderspawnclone", NULL, 90, 1, 0}, 
	{"alienboss2", NULL, 91, 1, 0}, 
	// Vertigo robots
	{"compactlifter", NULL, 166, 1, 0}, 
	{"fervid99", NULL, 167, 1, 0}, 
	{"fiddler", NULL, 168, 1, 0}, 
	{"class2heavydriller", NULL, 169, 1, 0}, 
	{"smelter2", NULL, 170, 1, 0}, 
	{"max", NULL, 171, 1, 0}, 
	{"sniperng", NULL, 172, 1, 0}, 
	{"logikill", NULL, 173, 1, 0}, 
	{"canary", NULL, 174, 1, 0}, 
	{"vertigoboss", NULL, 175, 1, 0}, 
	{"redguard", NULL, 176, 1, 0}, 
	{"spike", NULL, 177, 1, 0}, 
	// powerups/missiles
	{"concussion", NULL, 127, 0, 1}, 
	{NULL, NULL, 137, 0, 1}, 
	{"homer", NULL, 133, 0, 1}, 
	{NULL, NULL, 136, 0, 1}, 
	{"smartmsl", NULL, 134, 0, 1}, 
	{NULL, NULL, 162, 0, 1}, 
	{"mega", NULL, 135, 0, 1}, 
	{NULL, NULL, 142, 0, 1}, 
	{"flashmsl", NULL, 151, 0, 1}, 
	{NULL, NULL, 158, 0, 1}, 
	{NULL, NULL, 165, 0, 1}, 
	{"guided", NULL, 152, 0, 1}, 
	{"mercury", NULL, 153, 0, 1}, 
	{NULL, NULL, 161, 0, 1}, 
	{"eshaker", NULL, 154, 0, 1}, 
	{NULL, NULL, 163, 0, 1}, 
	{"shakrsub", NULL, 160, 0, 1},
	{"pminepack", "pminpack.pol", MAX_POLYGON_MODELS - 1, 0, 1},
	{"proxmine", "proxmine.pol", MAX_POLYGON_MODELS - 2, 0, 1},
	{"sminepack", "sminpack.pol", MAX_POLYGON_MODELS - 3, 0, 1},
	{"smartmine", "smrtmine.pol", MAX_POLYGON_MODELS - 4, 0, 1},
	{"concussion4", "concpack.pol", MAX_POLYGON_MODELS - 5, 0, 1},
	{"homer4", "homrpack.pol", MAX_POLYGON_MODELS - 6, 0, 1},
	{"flash4", "flshpack.pol", MAX_POLYGON_MODELS - 7, 0, 1},
	{"guided4", "guidpack.pol", MAX_POLYGON_MODELS - 8, 0, 1},
	{"mercury4", "mercpack.pol", MAX_POLYGON_MODELS - 9, 0, 1},
	{"laser", NULL, MAX_POLYGON_MODELS - 10, 0, 1},
	{"vulcan", NULL, MAX_POLYGON_MODELS - 11, 0, 1},
	{"spreadfire", NULL, MAX_POLYGON_MODELS - 12, 0, 1},
	{"plasma", NULL, MAX_POLYGON_MODELS - 13, 0, 1},
	{"fusion", NULL, MAX_POLYGON_MODELS - 14, 0, 1},
	{"superlaser", NULL, MAX_POLYGON_MODELS - 15, 0, 1},
	{"gauss", NULL, MAX_POLYGON_MODELS - 16, 0, 1},
	{"helix", NULL, MAX_POLYGON_MODELS - 17, 0, 1},
	{"phoenix", NULL, MAX_POLYGON_MODELS - 18, 0, 1},
	{"omega", NULL, MAX_POLYGON_MODELS - 19, 0, 1},
	{"quadlaser", NULL, MAX_POLYGON_MODELS - 20, 0, 1},
	{"afterburner", NULL, MAX_POLYGON_MODELS - 21, 0, 1},
	{"headlight", NULL, MAX_POLYGON_MODELS - 22, 0, 1},
	{"ammorack", NULL, MAX_POLYGON_MODELS - 23, 0, 1},
	{"converter", NULL, MAX_POLYGON_MODELS - 24, 0, 1},
	{"fullmap", NULL, MAX_POLYGON_MODELS - 25, 0, 1},
	{"cloak", NULL, MAX_POLYGON_MODELS - 26, 0, 1},
	{"invul", NULL, MAX_POLYGON_MODELS - 27, 0, 1},
	{"extralife", NULL, MAX_POLYGON_MODELS - 28, 0, 1},
	{"bluekey", NULL, MAX_POLYGON_MODELS - 29, 0, 1},
	{"redkey", NULL, MAX_POLYGON_MODELS - 30, 0, 1},
	{"goldkey", NULL, MAX_POLYGON_MODELS - 31, 0, 1},
	{"vulcanammo", NULL, MAX_POLYGON_MODELS - 32, 0, 1},
	{"slowmotion", NULL, MAX_POLYGON_MODELS - 33, 0, 1},
	{"bullettime", NULL, MAX_POLYGON_MODELS - 34, 0, 1},
	{"hostage", NULL, HOSTAGE_MODEL, 0, 1}
	};

void InitReplacementModels (void)
{
memset (gameData.models.modelToOOF, 0, sizeof (gameData.models.modelToOOF));
memset (gameData.models.modelToASE, 0, sizeof (gameData.models.modelToASE));
memset (gameData.models.modelToPOL, 0, sizeof (gameData.models.modelToPOL));
}

// ----------------------------------------------------------------------------

short LoadLoresModel (short i)
{
	CFILE			cf;
	tPolyModel	*pm;
	short			nModel, j = sizeofa (replacementModels);
	char			szModel [FILENAME_LEN];

sprintf (szModel, "model%d.pol", replacementModels [i].nModel);
if (!(replacementModels [i].pszLores && 
	  (CFOpen (&cf, replacementModels [i].pszLores, gameFolders.szDataDir, "rb", 0) ||
	   CFOpen (&cf, szModel, gameFolders.szDataDir, "rb", 0))))
	return ++i;
nModel = replacementModels [i].nModel;
pm = ((gameStates.app.bFixModels && gameStates.app.bAltModels) ? gameData.models.altPolyModels : gameData.models.polyModels) + nModel;
if (!PolyModelRead (pm, &cf, 1)) {
	CFClose (&cf);
	return ++i;
	}
pm->modelData = 
pm->modelData = NULL;
PolyModelDataRead (pm, nModel, gameData.models.defPolyModels + nModel, &cf);
CFClose (&cf);
pm->rad = G3PolyModelSize (pm, nModel);
do {
	gameData.models.modelToPOL [nModel] = pm;
	} while ((++i < j) && !replacementModels [i].pszHires);
gameData.models.nLoresModels++;
return i;
}

// ----------------------------------------------------------------------------

short LoadOOFModel (tOOFObject *po, short i, int bCustom)
{
	short	j = sizeofa (replacementModels);
	char	szModel [2][FILENAME_LEN];

sprintf (szModel [0], "\001model%d.oof", replacementModels [i].nModel);
if (replacementModels [i].pszHires)
	sprintf (szModel [1], "\001%s.oof", replacementModels [i].pszHires);
else
	szModel [1][0] = '\0';
if (!(OOF_ReadFile (szModel [1] + !bCustom, po, replacementModels [i].nModel, replacementModels [i].nType, replacementModels [i].bFlipV, bCustom) || 
	   OOF_ReadFile (szModel [0] + !bCustom, po, replacementModels [i].nModel, replacementModels [i].nType, replacementModels [i].bFlipV, bCustom)))
	return 0;
do {
	CBP (!replacementModels [i].nModel);
	gameData.models.modelToOOF [bCustom][replacementModels [i].nModel] = po;
	} while ((++i < j) && !replacementModels [i].pszHires);
gameData.models.nHiresModels++;
return i;
}

// ----------------------------------------------------------------------------

short LoadASEModel (tASEModel *pa, short i, int bCustom)
{
	short	j = sizeofa (replacementModels);
	char	szModel [2][FILENAME_LEN];

sprintf (szModel [0], "\001model%d.ase", replacementModels [i].nModel);
if (replacementModels [i].pszHires)
	sprintf (szModel [1], "\001%s.ase", replacementModels [i].pszHires);
else
	szModel [1][0] = '\0';
#if 0//def _DEBUG
while (!ASE_ReadFile (szModel [1] + !bCustom, pa, replacementModels [i].nType, bCustom))
	;
#endif
if (!(ASE_ReadFile (szModel [1] + !bCustom, pa, replacementModels [i].nModel, replacementModels [i].nType, bCustom) || 
	   ASE_ReadFile (szModel [0] + !bCustom, pa, replacementModels [i].nModel, replacementModels [i].nType, bCustom)))
	return 0;
do {
	CBP (!replacementModels [i].nModel);
	gameData.models.modelToASE [bCustom][replacementModels [i].nModel] = pa;
	} while ((++i < j) && !replacementModels [i].pszHires);
gameData.models.nHiresModels++;
return i;
}

// ----------------------------------------------------------------------------

short LoadHiresModel (int nModel, short i, int bCustom)
{
	short	j = sizeofa (replacementModels);
	char	szModel [FILENAME_LEN];

sprintf (szModel, "\001model%d.oof", replacementModels [i].nModel);
#ifdef _DEBUG
if (replacementModels [i].pszHires && !strcmp (replacementModels [i].pszHires, "pyrogl.oof"))
	;
#endif
#if OOF_TEST_CUBE
if (!strcmp (replacementModels [i].pszHires + 1, "pyrogl.oof"))
	replacementModels [i].pszHires = "cube.oof";
#endif
if ((j = LoadASEModel (gameData.models.aseModels [bCustom] + i, i, bCustom)))
	return j;
#if 0
if ((j = LoadOOFModel (gameData.models.oofModels [bCustom] + i, i, bCustom)))
	return j;
#endif
return bCustom ? ++i : LoadLoresModel (i);
}

//------------------------------------------------------------------------------

static int loadIdx;
static int loadOp = 0;

static void LoadModelsPoll (int nItems, tMenuItem *m, int *key, int cItem)
{
GrPaletteStepLoad (NULL);
if (loadOp == 0) {
	loadIdx = LoadHiresModel (gameData.models.nHiresModels, loadIdx, 0);
	if (loadIdx >= sizeofa (replacementModels)) {
		loadOp = 1;
		loadIdx = 0;
		}
	}
else if (loadOp == 1) {
	loadIdx = LoadLoresModel (loadIdx);
	if (loadIdx >= sizeofa (replacementModels)) {
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

for (h = i = 0, j = sizeofa (replacementModels); i < j; i++)
	if (replacementModels [i].pszHires) {
		if (gameOpts->render.bHiresModels)
			h++;
		if (replacementModels [i].pszLores)
			h++;
		}
return h = (gameOpts->render.bHiresModels + 1) * (sizeofa (replacementModels) - 1);
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
	InitReplacementModels ();
	gameData.models.nHiresModels = 0;
	}
if (gameStates.app.bNostalgia)
	gameOpts->render.bHiresModels = 0;
else /*if (gameOpts->render.bHiresModels)*/ {
	if (!bCustom && gameStates.app.bProgressBars && gameOpts->menus.nStyle)
		LoadModelsGauge ();
	else {
		short	i = 0, j = sizeofa (replacementModels);
		if (!bCustom)
			ShowBoxedMessage (TXT_LOADING_MODELS);
		if (gameOpts->render.bHiresModels) {
			while (i < j)
				i = LoadHiresModel (gameData.models.nHiresModels, i, bCustom);
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
	int	h, i, j;

for (i = 0; i < gameData.models.nHiresModels; i++)
	for (j = bCustom; j < 2; j++) {
		h = gameData.models.oofModels [j][i].nModel;
		if (gameData.models.modelToOOF [j][h]) {
			gameData.models.modelToOOF [j][h] = NULL;
			OOF_FreeObject (gameData.models.oofModels [j] + i);
			}
		h = gameData.models.aseModels [j][i].nModel;
		if (gameData.models.modelToASE [j][h]) {
			gameData.models.modelToASE [j][h] = NULL;
			ASE_FreeModel (gameData.models.aseModels [j] + i);
			}
		}
}

// ----------------------------------------------------------------------------
// eof
