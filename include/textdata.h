#ifndef _TEXTDATA_H
#define _TEXTDATA_H

void LoadTextData (const char *pszLevelName, const char *pszExt, CTextData *msgP);
void FreeTextData (CTextData *msgP);
int32_t ShowGameMessage (CTextData *msgP, int32_t nId, int32_t nDuration);
tTextIndex *FindTextData (CTextData *msgP, int32_t nId);

#endif //_TEXTDATA_H
