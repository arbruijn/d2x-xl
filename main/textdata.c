#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "u_mem.h"
#include "error.h"
#include "cfile.h"
#include "key.h"
#include "ogl_bitmap.h"
#include "newmenu.h"
#include "textdata.h"

//------------------------------------------------------------------------------

void FreeTextData (tTextData *msgP)
{
D2_FREE (msgP->textBuffer);
D2_FREE (msgP->index);
if (msgP->bmP) {
	GrFreeBitmap (msgP->bmP);
	msgP->bmP = NULL;
	}
msgP->nLines = 0;
}

//------------------------------------------------------------------------------

void QSortTextData (tTextIndex *indexP, int left, int right)
{
	int	l = left,
			r = right,
			m = indexP [(left + right) / 2].nId;

do {
	while (indexP [l].nId < m)
		l++;
	while (indexP [r].nId > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tTextIndex h = indexP [l];
			indexP [l] = indexP [r];
			indexP [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
}

//------------------------------------------------------------------------------

void LoadTextData (char *pszLevelName, char *pszExt, tTextData *msgP)
{
	char			szFilename [SHORT_FILENAME_LEN];
	CFILE			cf;
	int			bufSize, nLines;
	char			*p, *q;
	tTextIndex	*pi;

	//first, D2_FREE up data allocated for old bitmaps
LogErr ("   loading mission messages\n");
FreeTextData (msgP);
ChangeFilenameExtension (szFilename, pszLevelName, pszExt);
bufSize = CFSize (szFilename, gameFolders.szDataDir, 0);
if (bufSize <= 0)
	return;
if (!(msgP->textBuffer = D2_ALLOC (bufSize + 2)))
	return;
if (!CFOpen (&cf, szFilename, gameFolders.szDataDir, "rb", 0)) {
	FreeTextData (msgP);
	return;
	}
CFRead (msgP->textBuffer + 1, bufSize, 1, &cf);
CFClose (&cf);
msgP->textBuffer [0] = '\n';
msgP->textBuffer [bufSize] = '\0';
for (p = msgP->textBuffer, nLines = 0; *p; p++) {
	if (*p == '\n')
		nLines++;
	}
if (!(msgP->index = (tTextIndex *) D2_ALLOC (nLines * sizeof (tTextIndex)))) {
	FreeTextData (msgP);
	return;
	}
msgP->nLines = nLines;
nLines = 0;
for (p = q = msgP->textBuffer, pi = msgP->index; ; p++) {
	if (!*p) {
		pi->nLines = nLines;
		break;
		}
	else if (*p == '\n') {
		if (nLines)
			(pi - 1)->nLines = nLines;
		*q++ = '\0';
		p++;
		pi->nId = atoi (p);
		while (isdigit (*p))
			p++;
		pi->pszText = ++p;
		pi++;
		*q++ = *p;
		}
	else if ((*p = '\\') && (*(p + 1) == 'n')) {
		*q++ = '\n';
		nLines++;
		p++;
		}
	else
		*q++ = *p;
	}
QSortTextData (msgP->index, 0, msgP->nLines - 1);
}

//------------------------------------------------------------------------------

tTextIndex *FindGameMessage (tTextData *msgP, int nId)
{
	int	h, m, l = 0, r = msgP->nLines - 1;

do {
	m = (l + r) / 2;
	h = msgP->index [m].nId;
	if (nId > h)
		l = m + 1;
	else if (nId < h)
		r = m - 1;
	else
		return msgP->index + m;
	} while (l <= r);
return NULL;
}

//------------------------------------------------------------------------------

void ShowGameMessage (tTextData *msgP, int nId, int nDuration)
{
	tTextIndex	*indexP;
	short			w, h, x, y;

if (nId < 0) {
	if (!(indexP = msgP->currentMsg))
		return;
	if ((msgP->nEndTime > 0) && (msgP->nEndTime <= gameStates.app.nSDLTicks)) {
		msgP->currentMsg = NULL;
		return;
		}
	}
else {
	if (!(indexP = FindGameMessage (msgP, nId)))
		return;
	msgP->currentMsg = indexP;
	msgP->nEndTime = (nDuration < 0) ? -1 : gameStates.app.nSDLTicks + 1000 * nDuration;
	if (msgP->bmP) {
		GrFreeBitmap (msgP->bmP);
		msgP->bmP = NULL;
		}
	}
if (msgP->bmP || (msgP->bmP = CreateStringBitmap (indexP->pszText, 0, 0, NULL, 0, 0, 0))) {
	w = msgP->bmP->bmProps.w;
	h = msgP->bmP->bmProps.h;
	x = (grdCurCanv->cv_w - w) / 2;
	y = (grdCurCanv->cv_h - h) / 2;
	NMBlueBox (x - 4, y - 4, x + w + 4, y + w + 4);
	OglUBitBltI (w, h, x, y, w, h, 0, 0, msgP->bmP, &grdCurCanv->cvBitmap, 0, 1,
					 (msgP->nEndTime < 0) ? 1.0f : (float) (msgP->nEndTime - gameStates.app.nSDLTicks) / 1000.0f);
	}
if (msgP->nEndTime < 0) {
	while (KeyInKey () != KEY_ESC)
		;
	}
}

//------------------------------------------------------------------------------
//eof
