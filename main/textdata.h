#ifndef _TEXTDATA_H
#define _TEXTDATA_H

void LoadTextData (char *pszLevelName, char *pszExt, tTextData *msgP);
void FreeTextData (tTextData *msgP);
void ShowGameMessage (tTextData *msgP, int nId, int nDuration);
tTextIndex *FindTextData (tTextData *msgP, int nId);

#endif //_TEXTDATA_H
