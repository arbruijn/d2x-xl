#ifndef _TEXTDATA_H
#define _TEXTDATA_H

void LoadTextData (const char *pszLevelName, const char *pszExt, CTextData *pMsg);
void FreeTextData (CTextData *pMsg);
int32_t ShowGameMessage (CTextData *pMsg, int32_t nId, int32_t nDuration);
tTextIndex *FindTextData (CTextData *pMsg, int32_t nId);

#endif //_TEXTDATA_H
