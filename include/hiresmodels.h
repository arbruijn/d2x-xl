#ifndef _HIRESMODELS_H
#define _HIRESMODELS_H

#include "ase.h"

typedef struct tReplacementModel {
	const char*	pszHires;
	const char*	pszLores;
	int16_t		nModel;
	int16_t		nType;
	int32_t		bFlipV;
	int16_t		nId;
} tReplacementModel;

extern tReplacementModel replacementModels [];

// ----------------------------------------------------------------------------

void LoadModelsGauge (void);
void LoadHiresModels (int32_t bCustom);
void FreeHiresModels (int32_t bCustom);
int16_t LoadHiresModel (int16_t nModel, int32_t bCustom, const char* filename = NULL);
void FreeHiresModel (int32_t nModel);
int32_t ReplacementModelCount (void);

// ----------------------------------------------------------------------------

static inline ASE::CModel* GetASEModel (int32_t nModel)
{
if (gameData.modelData.modelToASE [1][nModel])
	return gameData.modelData.modelToASE [1][nModel];
if (gameData.modelData.modelToASE [0][nModel])
	return gameData.modelData.modelToASE [0][nModel];
return NULL;
}

// ----------------------------------------------------------------------------

static inline OOF::CModel* GetOOFModel (int32_t nModel)
{
if (gameData.modelData.modelToOOF [1][nModel])
	return gameData.modelData.modelToOOF [1][nModel];
if (gameData.modelData.modelToOOF [0][nModel])
	return gameData.modelData.modelToOOF [0][nModel];
return NULL;
}

// ----------------------------------------------------------------------------

static inline CPolyModel* GetPOLModel (int32_t nModel)
{
if (gameData.modelData.modelToPOL [nModel])
	return gameData.modelData.modelToPOL [nModel];
return NULL;
}

// ----------------------------------------------------------------------------

static inline int32_t HaveHiresModel (int32_t nModel)
{
return (GetASEModel (nModel) != NULL) || (GetOOFModel (nModel) != NULL);
}

// ----------------------------------------------------------------------------

static inline int32_t HaveReplacementModel (int32_t nModel)
{
return HaveHiresModel (nModel) || (GetPOLModel (nModel) != NULL);
}

// ----------------------------------------------------------------------------

static inline bool IsPlayerShip (int32_t nModel)
{
return (nModel >= 108) && (nModel <= 110);
}

// ----------------------------------------------------------------------------

#endif //_HIRESMODELS_H
