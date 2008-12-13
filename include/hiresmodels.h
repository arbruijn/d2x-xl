#ifndef _HIRESMODELS_H
#define _HIRESMODELS_H

#include "ase.h"

typedef struct tReplacementModel {
	const char	*pszHires;
	const char	*pszLores;
	short	nModel;
	short	nType;
	int	bFlipV;
	short	nId;
} tReplacementModel;

extern tReplacementModel replacementModels [];

// ----------------------------------------------------------------------------

void LoadModelsGauge (void);
void LoadHiresModels (int bCustom);
void FreeHiresModels (int bCustom);
int ReplacementModelCount (void);

// ----------------------------------------------------------------------------

static inline ASEModel::CModel* GetASEModel (int nModel)
{
if (gameData.models.modelToASE [1][nModel])
	return gameData.models.modelToASE [1][nModel];
if (gameData.models.modelToASE [0][nModel])
	return gameData.models.modelToASE [0][nModel];
return NULL;
}

// ----------------------------------------------------------------------------

static inline tOOFObject* GetOOFModel (int nModel)
{
if (gameData.models.modelToOOF [1][nModel])
	return gameData.models.modelToOOF [1][nModel];
if (gameData.models.modelToOOF [0][nModel])
	return gameData.models.modelToOOF [0][nModel];
return NULL;
}

// ----------------------------------------------------------------------------

static inline tPolyModel* GetPOLModel (int nModel)
{
if (gameData.models.modelToPOL [nModel])
	return gameData.models.modelToPOL [nModel];
return NULL;
}

// ----------------------------------------------------------------------------

static inline int HaveHiresModel (int nModel)
{
return (GetASEModel (nModel) != NULL) || (GetOOFModel (nModel) != NULL);
}

// ----------------------------------------------------------------------------

static inline int HaveReplacementModel (int nModel)
{
return HaveHiresModel (nModel) || (GetPOLModel (nModel) != NULL);
}

// ----------------------------------------------------------------------------

#endif //_HIRESMODELS_H
