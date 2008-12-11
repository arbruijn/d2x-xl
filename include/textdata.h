#ifndef _TEXTDATA_H
#define _TEXTDATA_H

void LoadTextData (const char *pszLevelName, const char *pszExt, CTextData *msgP);
void FreeTextData (CTextData *msgP);
int ShowGameMessage (CTextData *msgP, int nId, int nDuration);
tTextIndex *FindTextData (CTextData *msgP, int nId);

#endif //_TEXTDATA_H
