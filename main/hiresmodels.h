#ifndef _HIRESMODELS_H
#define _HIRESMODELS_H

void LoadModelsGauge (void);
void LoadHiresModels (int bCustom);
void FreeHiresModels (int bCustom);

// ----------------------------------------------------------------------------

static inline tASEModel *ASEModel (int nModel)
{
if (gameData.models.modelToASE [1][nModel])
	return gameData.models.modelToASE [1][nModel];
if (gameData.models.modelToASE [0][nModel])
	return gameData.models.modelToASE [0][nModel];
return NULL;
}

// ----------------------------------------------------------------------------

static inline tOOFObject *OOFModel (int nModel)
{
if (gameData.models.modelToOOF [1][nModel])
	return gameData.models.modelToOOF [1][nModel];
if (gameData.models.modelToOOF [0][nModel])
	return gameData.models.modelToOOF [0][nModel];
return NULL;
}

// ----------------------------------------------------------------------------

static inline tPolyModel *POLModel (int nModel)
{
if (gameData.models.modelToPOL [nModel])
	return gameData.models.modelToPOL [nModel];
return NULL;
}

// ----------------------------------------------------------------------------

static inline int HaveHiresModel (int nModel)
{
return (ASEModel (nModel) != NULL) || (OOFModel (nModel) != NULL);
}

// ----------------------------------------------------------------------------

static inline int HaveReplacementModel (int nModel)
{
return HaveHiresModel (nModel) || (POLModel (nModel) != NULL);
}

// ----------------------------------------------------------------------------

#endif //_HIRESMODELS_H