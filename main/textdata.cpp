#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include <stdio.h>

#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "key.h"
#include "ogl_bitmap.h"
#include "menu.h"
#include "gamefont.h"
#include "gamecntl.h"
#include "text.h"
#include "textdata.h"
#include "menubackground.h"

//------------------------------------------------------------------------------

void FreeTextData (CTextData *pMsg)
{
delete[] pMsg->textBuffer;
pMsg->textBuffer = NULL;
delete[] pMsg->index;
pMsg->index = NULL;
if (pMsg->pBm) {
	delete pMsg->pBm;
	pMsg->pBm = NULL;
	}
pMsg->nMessages = 0;
}

//------------------------------------------------------------------------------

void QSortTextData (tTextIndex *pIndex, int32_t left, int32_t right)
{
	int32_t	l = left,
			r = right,
			m = pIndex [(left + right) / 2].nId;

do {
	while (pIndex [l].nId < m)
		l++;
	while (pIndex [r].nId > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tTextIndex h = pIndex [l];
			pIndex [l] = pIndex [r];
			pIndex [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortTextData (pIndex, l, right);
if (left < r)
	QSortTextData (pIndex, left, r);
}

//------------------------------------------------------------------------------

void LoadTextData (const char *pszLevelName, const char *pszExt, CTextData *pMsg)
{
	char			szFilename [FILENAME_LEN];
	CFile			cf;
	int32_t			bufSize, nLines;
	char			*p, *q;
	tTextIndex	*pi;

//first, free up data allocated for old bitmaps
PrintLog (1, "loading mission messages\n");
FreeTextData (pMsg);
CFile::ChangeFilenameExtension (szFilename, pszLevelName, pszExt);
bufSize = (int32_t) cf.Size (szFilename, gameFolders.game.szData [0], 0);
if (bufSize <= 0) {
	PrintLog (-1);
	return;
	}
if (!(pMsg->textBuffer = new char [bufSize + 2])) {
	PrintLog (-1);
	return;
	}
if (!cf.Open (szFilename, gameFolders.game.szData [0], "rb", 0)) {
	FreeTextData (pMsg);
 	PrintLog (-1);
	return;
	}
cf.Read (pMsg->textBuffer + 1, 1, bufSize);
cf.Close ();
pMsg->textBuffer [0] = '\n';
pMsg->textBuffer [bufSize + 1] = '\0';
for (p = pMsg->textBuffer + 1, nLines = 1; *p; p++) {
	if (*p == '\n')
		nLines++;
	}
if (!(pMsg->index = new tTextIndex [nLines])) {
	FreeTextData (pMsg);
 	PrintLog (-1);
	return;
	}
pMsg->nMessages = nLines;
nLines = 0;
for (p = q = pMsg->textBuffer, pi = pMsg->index; ; p++) {
	if (!*p) {
		*q++ = *p;
		(pi - 1)->nLines = nLines;
		break;
		}
	else if ((*p == '\r') || (*p == '\n')) {
		if (nLines)
			(pi - 1)->nLines = nLines;
		*q++ = '\0';
		if ((*p == '\r') && (*(p + 1) == '\n'))
			p += 2;
		else
			p++;
		if (!*p) {
			*q++ = '\0';
			break;
			}
		pi->nId = atoi (p);
		if (!pi->nId)
			continue;
		while (::isdigit (*p))
			p++;
		pi->pszText = q;
		pi++;
		nLines = 1;
		}
	else if ((*p == '\\') && (*(p + 1) == 'n')) {
		*q++ = '\n';
		nLines++;
		p++;
		}
	else
		*q++ = *p;
	}
pMsg->nMessages = (int32_t) (pi - pMsg->index);
QSortTextData (pMsg->index, 0, pMsg->nMessages - 1);
PrintLog (-1);
}

//------------------------------------------------------------------------------

tTextIndex *FindTextData (CTextData *pMsg, int32_t nId)
{
if (!pMsg->index)
	return NULL;

	int32_t	h, m, l = 0, r = pMsg->nMessages - 1;

do {
	m = (l + r) / 2;
	h = pMsg->index [m].nId;
	if (nId > h)
		l = m + 1;
	else if (nId < h)
		r = m - 1;
	else
		return pMsg->index + m;
	} while (l <= r);
return NULL;
}

//------------------------------------------------------------------------------

int32_t ShowGameMessage (CTextData *pMsg, int32_t nId, int32_t nDuration)
{
	tTextIndex	*pIndex;
	int16_t			w, h, x, y;
	float			fAlpha;

if (nId < 0) {
	if (!(pIndex = pMsg->currentMsg))
		return 0;
	if ((pMsg->nEndTime > 0) && (pMsg->nEndTime <= gameStates.app.nSDLTicks [0])) {
		pMsg->currentMsg = NULL;
		return 0;
		}
	}
else {
	if (!(pIndex = FindTextData (pMsg, nId)))
		return 0;
	pMsg->currentMsg = pIndex;
	pMsg->nStartTime = gameStates.app.nSDLTicks [0];
	pMsg->nEndTime = (nDuration < 0) ? -1 : gameStates.app.nSDLTicks [0] + 1000 * nDuration;
	if (pMsg->pBm) {
		delete pMsg->pBm;
		pMsg->pBm = NULL;
		}
	}
if (pMsg->nEndTime < 0) {
	if (nId < 0)
		return 0;
	PauseGame ();
	TextBox (NULL, BG_STANDARD, 1, TXT_CLOSE, pIndex->pszText);
	pMsg->currentMsg = NULL;
	ResumeGame ();
	}
else if (!gameStates.render.nWindow [0]) {
	if (!pMsg->pBm) {
		fontManager.SetCurrent (NORMAL_FONT);
		fontManager.SetColorRGBi (GOLD_RGBA, 1, 0, 0);
		}
	if (pMsg->pBm || (pMsg->pBm = CreateStringBitmap (pIndex->pszText, 0, 0, NULL, 0, 0, 0, 0))) {
		w = pMsg->pBm->Width ();
		h = pMsg->pBm->Height ();
		x = (CCanvas::Current ()->Width () - w) / 2;
		y = (CCanvas::Current ()->Height () - h) * 2 / 5;
		if (pMsg->nEndTime < 0)
			fAlpha = 1.0f;
		else if (gameStates.app.nSDLTicks [0] - pMsg->nStartTime < 250)
			fAlpha = (float) (gameStates.app.nSDLTicks [0] - pMsg->nStartTime) / 250.0f;
		else if (pMsg->nEndTime - gameStates.app.nSDLTicks [0] < 500)
			fAlpha = (float) (pMsg->nEndTime - gameStates.app.nSDLTicks [0]) / 500.0f;
		else
			fAlpha = 1.0f;
		backgroundManager.DrawBox (x - 8, y - 8, x + w + 8, y + h + 8, 3, fAlpha, 1);
		pMsg->pBm->Render (CCanvas::Current (), x, y, w, h, 0, 0, w, h, 1, 0, 0, fAlpha);
		}
	}
return 1;
}

//------------------------------------------------------------------------------
//eof
