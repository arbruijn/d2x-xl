#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
//#include  "oof.h
#include "inferno.h"
#include "cfile.h"
#include "args.h"
#include "u_mem.h"
#include "gr.h"
#include "error.h"
#include "globvars.h"
#include "3d.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "vecmat.h"
#include "render.h"
#include "strutil.h"
#include "hudmsg.h"
#include "tga.h"
#include "oof.h"

//------------------------------------------------------------------------------

#define OOF_MEM_OPT	1
#define GL_G3_INFINITY	0
#define SHADOW_TEST	0
#define NORM_INF		1

#ifdef __unix
#	ifndef stricmp
#		define	stricmp	strcasecmp
#	endif
#	ifndef strnicmp
#		define	strnicmp	strncasecmp
#	endif
#endif

//------------------------------------------------------------------------------

static int nIndent = 0;
static int bLogOOF = 0;
extern  FILE *fErr;

void _CDECL_ OOF_PrintLog (const char *fmt, ...)
{
if (bLogOOF) {
	va_list arglist;
	static char szLog [1024];

	memset (szLog, ' ', nIndent);
	va_start (arglist, fmt);
	vsprintf (szLog + nIndent, fmt, arglist);
	va_end (arglist);
	fprintf (fErr, szLog);
	}
}

//------------------------------------------------------------------------------

sbyte OOF_ReadByte (CFILE *fp, const char *pszIdent)
{
sbyte b = CFReadByte (fp);
OOF_PrintLog ("      %s = %d\n", pszIdent, b);
return b;
}

//------------------------------------------------------------------------------

int OOF_ReadInt (CFILE *fp, const char *pszIdent)
{
int i = CFReadInt (fp);
OOF_PrintLog ("      %s = %d\n", pszIdent, i);
return i;
}

//------------------------------------------------------------------------------

float OOF_ReadFloat (CFILE *fp, const char *pszIdent)
{
float f = CFReadFloat (fp);
OOF_PrintLog ("      %s = %1.4f\n", pszIdent, f);
return f;
}

//------------------------------------------------------------------------------

void OOF_ReadVector (CFILE *fp, tOOF_vector *pv, const char *pszIdent)
{
pv->x = CFReadFloat (fp);
pv->y = CFReadFloat (fp);
pv->z = CFReadFloat (fp);
OOF_PrintLog ("      %s = %1.4f,%1.4f,%1.4f\n", pszIdent, pv->x, pv->y, pv->z);
}

//------------------------------------------------------------------------------

char *OOF_ReadString (CFILE *fp, const char *pszIdent, char *pszPrefix)
{
	char	*psz;
	int	l, lPrefix = pszPrefix ? (int) strlen (pszPrefix) : 0;

l = OOF_ReadInt (fp, "string length");
if (!(psz = (char *) D2_ALLOC (l + lPrefix + 1)))
	return NULL;
if (lPrefix)
	memcpy (psz, pszPrefix, lPrefix);
if (CFRead (psz + lPrefix, l, 1, fp)) {
	psz [l + lPrefix] = '\0';
	OOF_PrintLog ("      %s = '%s'\n", pszIdent, psz);
	return psz;
	}
D2_FREE (psz);
return NULL;
}

//------------------------------------------------------------------------------

int ListType (char *pListId)
{
if (!strncmp (pListId, "TXTR", 4))
	return 0;
if (!strncmp (pListId, "OHDR", 4))
	return 1;
if (!strncmp (pListId, "SOBJ", 4))
	return 2;
if (!strncmp (pListId, "GPNT", 4))
	return 3;
if (!strncmp (pListId, "SPCL", 4))
	return 4;
if (!strncmp (pListId, "ATCH", 4))
	return 5;
if (!strncmp (pListId, "PANI", 4))
	return 6;
if (!strncmp (pListId, "RANI", 4))
	return 7;
if (!strncmp (pListId, "ANIM", 4))
	return 8;
if (!strncmp (pListId, "WBAT", 4))
	return 9;
if (!strncmp (pListId, "NATH", 4))
	return 10;
return -1;
}

//------------------------------------------------------------------------------

void OOF_InitMinMax (tOOF_vector *pvMin, tOOF_vector *pvMax)
{
if (pvMin && pvMax) {
	pvMin->x =
	pvMin->y =
	pvMin->z = 1000000;
	pvMax->x =
	pvMax->y =
	pvMax->z = -1000000;
	}
}

//------------------------------------------------------------------------------

void OOF_GetMinMax (tOOF_vector *pv, tOOF_vector *pvMin, tOOF_vector *pvMax)
{
if (pvMin && pvMax) {
	if (pvMin->x > pv->x)
		pvMin->x = pv->x;
	if (pvMax->x < pv->x)
		pvMax->x = pv->x;
	if (pvMin->y > pv->y)
		pvMin->y = pv->y;
	if (pvMax->y < pv->y)
		pvMax->y = pv->y;
	if (pvMin->z > pv->z)
		pvMin->z = pv->z;
	if (pvMax->z < pv->z)
		pvMax->z = pv->z;
	}
}

//------------------------------------------------------------------------------

void OOF_SetModelProps (tOOF_subObject *pso,char *pszProps)
{
	// first, extract the command

	int l;
	char command [200], data [200], *psz;

if (3 > (l = (int) strlen (pszProps)))
	return;
if ((psz = strchr (pszProps, '=')))
	l = (int) (psz - pszProps + 1);
memcpy (command, pszProps, l);
command [l] = '\0';
if (psz)
	strcpy (data, psz + 1);
else
	*data = '\0';

// now act on the command/data pair

if (!stricmp (command,"$rotate=")) { // constant rotation for a subobject
	float spinrate = (float) atof( data);
	if ((spinrate <= 0) || (spinrate > 20))
		return;		// bad data
	pso->nFlags |= OOF_SOF_ROTATE;
	pso->fRPS = 1.0f / spinrate;
	return;
	}

if (!strnicmp (command,"$jitter",7)) {	// this subobject is a jittery tObject
	pso->nFlags |= OOF_SOF_JITTER;
	return;
	}

if (!strnicmp (command,"$shell",6)) { // this subobject is a door shell
	pso->nFlags |= OOF_SOF_SHELL;
	return;
	}

if (!strnicmp (command,"$facing",7)) { // this subobject always faces you
	pso->nFlags |= OOF_SOF_FACING;
	return;
	}

if (!strnicmp (command,"$frontface",10)) { // this subobject is a door front
	pso->nFlags |= OOF_SOF_FRONTFACE;
	return;
	}

if (!stricmp (command,"$glow=")) {
	float r,g,b;
	float size;
	int nValues;

	Assert (!(pso->nFlags & (OOF_SOF_GLOW | OOF_SOF_THRUSTER)));
	nValues = sscanf (data, " %f, %f, %f, %f", &r,&g,&b,&size);
	Assert (nValues == 4);
	pso->nFlags |= OOF_SOF_GLOW;
	//pso->glowInfo = (tOOF_glowInfo *) D2_ALLOC (sizeof(tOOF_glowInfo));
	pso->glowInfo.color.r = r;
	pso->glowInfo.color.g = g;
	pso->glowInfo.color.b = b;
	pso->glowInfo.fSize = size;
	return;
	}

if (!stricmp (command,"$thruster=")) {
	float r,g,b;
	float size;
	int nValues;

	Assert (!(pso->nFlags & (OOF_SOF_GLOW | OOF_SOF_THRUSTER)));
	nValues = sscanf(data, " %f, %f, %f, %f", &r,&g,&b,&size);
	Assert(nValues == 4);
	pso->nFlags |= OOF_SOF_THRUSTER;
	//pso->glowInfo = (tOOF_glowInfo *) D2_ALLOC (sizeof (tOOF_glowInfo));
	pso->glowInfo.color.r = r;
	pso->glowInfo.color.g = g;
	pso->glowInfo.color.b = b;
	pso->glowInfo.fSize = size;
	return;
	}

if (!stricmp (command,"$fov=")) {
	float fov_angle;
	float turret_spr;
	float reactionTime;
	int nValues;

	nValues = sscanf(data, " %f, %f, %f", &fov_angle, &turret_spr, &reactionTime);
	Assert(nValues == 3);
	if (fov_angle < 0.0f || fov_angle > 360.0f) { // Bad data
		Assert(0);
		fov_angle = 1.0;
		}
	// .4 is really fast and really arbitrary
	if (turret_spr < 0.0f || turret_spr > 60.0f) { // Bad data
		Assert(0);
		turret_spr = 1.0f;
		}
	// 10 seconds is really slow and really arbitrary
	if (reactionTime < 0.0f || reactionTime > 10.0f) { // Bad data
		Assert(0);
		reactionTime = 10.0;
		}
	pso->nFlags |= OOF_SOF_TURRET;
	pso->fFOV = fov_angle/720.0f; // 720 = 360 * 2 and we want to make fov the amount we can move in either direction
	                             // it has a minimum value of (0.0) to [0.5]
	pso->fRPS = 1.0f / turret_spr;  // convert spr to rps (rotations per second)
	pso->fUpdate = reactionTime;
	return;
	}

if (!stricmp (command,"$monitor01")) { // this subobject is a monitor
	pso->nFlags |= OOF_SOF_MONITOR1;
	return;
	}

if (!stricmp (command,"$monitor02")) { // this subobject is a 2nd monitor
	pso->nFlags |= OOF_SOF_MONITOR2;
	return;
	}

if (!stricmp (command,"$monitor03")) { // this subobject is a 3rd monitor
	pso->nFlags |= OOF_SOF_MONITOR3;
	return;
	}

if (!stricmp (command,"$monitor04")) { // this subobject is a 4th monitor
	pso->nFlags |= OOF_SOF_MONITOR4;
	return;
	}

if (!stricmp (command,"$monitor05")) { // this subobject is a 4th monitor
	pso->nFlags |= OOF_SOF_MONITOR5;
	return;
	}

if (!stricmp (command,"$monitor06")) { // this subobject is a 4th monitor
	pso->nFlags |= OOF_SOF_MONITOR6;
	return;
	}

if (!stricmp (command,"$monitor07")) { // this subobject is a 4th monitor
	pso->nFlags |= OOF_SOF_MONITOR7;
	return;
	}

if (!stricmp (command,"$monitor08")) { // this subobject is a 4th monitor
	pso->nFlags |= OOF_SOF_MONITOR8;
	return;
	}

if (!stricmp (command,"$viewer")) { // this subobject is a viewer
	pso->nFlags |= OOF_SOF_VIEWER;
	return;
	}

if (!stricmp (command,"$layer")) { // this subobject is a layer to be drawn after original tObject.
	pso->nFlags |= OOF_SOF_LAYER;
	return;
	}

if (!stricmp (command,"$custom")) { // this subobject has custom textures/colors
	pso->nFlags |= OOF_SOF_CUSTOM;
	return;
	}
}

//------------------------------------------------------------------------------

static tOOF_vector *OOF_ReadVertList (CFILE *fp, int nVerts, tOOF_vector *pvMin, tOOF_vector *pvMax)
{
	tOOF_vector	*pv;
	char			szId [20] = "";

OOF_InitMinMax (pvMin, pvMax);
if ((pv = (tOOF_vector *) D2_ALLOC (nVerts * sizeof (tOOF_vector)))) {
	int	i;

	for (i = 0; i < nVerts; i++) {
		if (bLogOOF)
			sprintf (szId, "pv [%d]", i);
		OOF_ReadVector (fp, pv + i, szId);
#if OOF_TEST_CUBE
		pv [i].x -= 10;
		pv [i].y += 15;
		//pv [i].z += 5;
		pv [i].x /= 2;
		pv [i].y /= 2;
		pv [i].z /= 2;
#endif
		OOF_GetMinMax (pv + i, pvMin, pvMax);
		}
	}
return pv;
}

//------------------------------------------------------------------------------

int OOF_ReadFrameInfo (CFILE *fp, tOOFObject *po, tOOF_frameInfo *pfi, int bTimed)
{
nIndent += 2;
OOF_PrintLog ("reading frame info\n");
if (bTimed) {
	pfi->nFrames = OOF_ReadInt (fp, "nFrames");
	pfi->nFirstFrame = OOF_ReadInt (fp, "nFirstFrame");
	pfi->nLastFrame = OOF_ReadInt (fp, "nLastFrame");
	if (po->frameInfo.nFirstFrame > pfi->nFirstFrame)
		po->frameInfo.nFirstFrame = pfi->nFirstFrame;
	if (po->frameInfo.nLastFrame < pfi->nLastFrame)
		po->frameInfo.nLastFrame = pfi->nLastFrame;
	}
else
	pfi->nFrames = po->frameInfo.nFrames;
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_ReadRotFrame (CFILE *fp, tOOF_rotFrame *pf, int bTimed)
{
	float	fMag;

nIndent += 2;
OOF_PrintLog ("reading rot frame\n");
if (bTimed)
	pf->nStartTime = OOF_ReadInt (fp, "nStartTime");
OOF_ReadVector (fp, &pf->vAxis, "vAxis");
if (0 < (fMag = OOF_VecMag (&pf->vAxis)))
	OOF_VecScale (&pf->vAxis, 1.0f / fMag);
pf->nAngle = OOF_ReadInt (fp, "nAngle");
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreeRotAnim (tOOF_rotAnim *pa)
{
D2_FREE (pa->pFrames);
D2_FREE (pa->pRemapTicks);
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadRotAnim (CFILE *fp, tOOFObject *po, tOOF_rotAnim *pa, int bTimed)
{
	tOOF_rotAnim a;
	int	i;

memset (&a, 0, sizeof (a));
OOF_ReadFrameInfo (fp, po, &a.frameInfo, bTimed);
if (!(a.pFrames = (tOOF_rotFrame *) D2_ALLOC (a.frameInfo.nFrames * sizeof (tOOF_rotFrame))))
	return 0;
memset (a.pFrames, 0, a.frameInfo.nFrames * sizeof (tOOF_rotFrame));
if (bTimed &&
	 (a.nTicks = abs (a.frameInfo.nLastFrame - a.frameInfo.nFirstFrame) + 1) &&
	 !(a.pRemapTicks = (ubyte *) D2_ALLOC (a.nTicks * sizeof (ubyte))))
	return OOF_FreeRotAnim (&a);
if (a.nTicks)
	for (i = 0; i < a.frameInfo.nFrames; i++)
		if (!OOF_ReadRotFrame (fp, a.pFrames + i, bTimed))
			return OOF_FreeRotAnim (&a);
*pa = a;
return 1;
}

//------------------------------------------------------------------------------

int OOF_ReadPosFrame (CFILE *fp, tOOF_posFrame *pf, int bTimed)
{
nIndent += 2;
OOF_PrintLog ("reading pos frame\n");
if (bTimed)
	pf->nStartTime = OOF_ReadInt (fp, "nStartTime");
OOF_ReadVector (fp, &pf->vPos, "vPos");
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreePosAnim (tOOF_posAnim *pa)
{
D2_FREE (pa->pFrames);
D2_FREE (pa->pRemapTicks);
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadPosAnim (CFILE *fp, tOOFObject *po, tOOF_posAnim *pa, int bTimed)
{
	tOOF_posAnim a;
	int	i;

memset (&a, 0, sizeof (a));
OOF_ReadFrameInfo (fp, po, &a.frameInfo, bTimed);
if (bTimed &&
	 (a.nTicks = a.frameInfo.nLastFrame - a.frameInfo.nFirstFrame) &&
	 !(a.pRemapTicks = (ubyte *) D2_ALLOC (a.nTicks * sizeof (ubyte))))
	return OOF_FreePosAnim (&a);
if (!(a.pFrames = (tOOF_posFrame *) D2_ALLOC (a.frameInfo.nFrames * sizeof (tOOF_posFrame))))
	return OOF_FreePosAnim (pa);
memset (a.pFrames, 0, a.frameInfo.nFrames * sizeof (tOOF_posFrame));
for (i = 0; i < a.frameInfo.nFrames; i++)
	if (!OOF_ReadPosFrame (fp, a.pFrames + i, bTimed))
		return OOF_FreePosAnim (pa);
*pa = a;
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreeSpecialPoint (tOOF_specialPoint *pVert)
{
D2_FREE (pVert->pszName);
D2_FREE (pVert->pszProps);
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadSpecialPoint (CFILE *fp, tOOF_specialPoint *pVert)
{
	memset (pVert, 0, sizeof (tOOF_specialPoint));

nIndent += 2;
OOF_PrintLog ("reading special point\n");
if (!(pVert->pszName = OOF_ReadString (fp, "pszName", NULL))) {
	nIndent -= 2;
	return 0;
	}
if (!(pVert->pszProps = OOF_ReadString (fp, "pszProps", NULL))) {
	nIndent -= 2;
	return 0;
	}
OOF_ReadVector (fp, &pVert->vPos, "vPos");
pVert->fRadius = OOF_ReadFloat (fp, "fRadius");
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreeSpecialList (tOOF_specialList *pList)
{
	int	i;

if (pList->pVerts) {
	for (i = 0; i < pList->nVerts; i++)
		OOF_FreeSpecialPoint (pList->pVerts + i);
	D2_FREE (pList->pVerts);
	}
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadSpecialList (CFILE *fp, tOOF_specialList *pList)
{
	int	i;

pList->nVerts = OOF_ReadInt (fp, "nVerts");
if (!pList->nVerts)
	return 1;
if (!(pList->pVerts = (tOOF_specialPoint *) D2_ALLOC (pList->nVerts * sizeof (tOOF_specialPoint))))
	return 0;
for (i = 0; i < pList->nVerts; i++)
	OOF_ReadSpecialPoint (fp, pList->pVerts + i);
return 1;
}

//------------------------------------------------------------------------------

int OOF_ReadPoint (CFILE *fp, tOOF_point *pPoint, int bParent)
{
nIndent += 2;
OOF_PrintLog ("reading point\n");
pPoint->nParent = bParent ? OOF_ReadInt (fp, "nParent") : 0;
OOF_ReadVector (fp, &pPoint->vPos, "vPos");
OOF_ReadVector (fp, &pPoint->vDir, "vDir");
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreePointList (tOOF_pointList *pList)
{
D2_FREE (pList->pPoints);
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadPointList (CFILE *fp, tOOF_pointList *pList, int bParent, int nSize)
{
	int	i;

nIndent += 2;
OOF_PrintLog ("reading point list\n");
pList->nPoints = OOF_ReadInt (fp, "nPoints");
if (nSize < pList->nPoints)
	nSize = pList->nPoints;
if (!(pList->pPoints = (tOOF_point *) D2_ALLOC (nSize * sizeof (tOOF_point)))) {
	nIndent -= 2;
	return OOF_FreePointList (pList);
	}
for (i = 0; i < pList->nPoints; i++)
	if (!OOF_ReadPoint (fp, pList->pPoints + i, bParent)) {
		nIndent -= 2;
		return 0;
		}
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreeAttachList (tOOF_attachList *pList)
{
D2_FREE (pList->pPoints);
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadAttachList (CFILE *fp, tOOF_attachList *pList)
{
	int	i;

nIndent += 2;
OOF_PrintLog ("reading attach list\n");
pList->nPoints = OOF_ReadInt (fp, "nPoints");
if (!(pList->pPoints = (tOOF_attachPoint *) D2_ALLOC (pList->nPoints * sizeof (tOOF_attachPoint)))) {
	nIndent -= 2;
	return OOF_FreeAttachList (pList);
	}
for (i = 0; i < pList->nPoints; i++)
	if (!OOF_ReadPoint (fp, &pList->pPoints [i].point, 1)) {
		nIndent -= 2;
		return OOF_FreeAttachList (pList);
		}
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_ReadAttachNormals (CFILE *fp, tOOF_attachList *pList)
{
	int	i;
	tOOF_attachPoint *pp = pList->pPoints;

nIndent += 2;
OOF_PrintLog ("reading attach normals\n");
i = OOF_ReadInt (fp, "nPoints");
if (i != pList->nPoints) {
	nIndent -= 2;
	return 0;
	}
for (i = 0; i < pList->nPoints; i++) {
	OOF_ReadVector (fp, &pp->vu, "vu");	//actually ignored
	OOF_ReadVector (fp, &pp->vu, "vu");
	pList->pPoints->bu = 1;
	}
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

static int OOF_ReadIntList (CFILE *fp, int **ppList)
{
	int	*pList, nList, i;
	char	szId [20] = "";

if (!(nList = OOF_ReadInt (fp, "nList"))) {
	*ppList = NULL;
	return 0;
	}
if (!(pList = (int *) D2_ALLOC (nList * sizeof (int))))
	return -1;
for (i = 0; i < nList; i++) {
	if (bLogOOF)
		sprintf (szId, "pList [%d]", i);
	pList [i] = OOF_ReadInt (fp, szId);
	}
*ppList = pList;
return nList;
}

//------------------------------------------------------------------------------

int OOF_FreeBattery (tOOF_battery *pBatt)
{
D2_FREE (pBatt->pVertIndex);
D2_FREE (pBatt->pTurretIndex);
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadBattery (CFILE *fp, tOOF_battery *pBatt)
{
nIndent += 2;
OOF_PrintLog ("reading battery\n");
if (0 > (pBatt->nVerts = OOF_ReadIntList (fp, &pBatt->pVertIndex))) {
	nIndent -= 2;
	return OOF_FreeBattery (pBatt);
	}
if (0 > (pBatt->nTurrets = OOF_ReadIntList (fp, &pBatt->pTurretIndex))) {
	nIndent -= 2;
	return OOF_FreeBattery (pBatt);
	}
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreeArmament (tOOF_armament *pa)
{
if (pa->pBatts) {
	int	i;

	for (i = 0; i < pa->nBatts; i++)
		OOF_FreeBattery (pa->pBatts + i);
	D2_FREE (pa->pBatts);
	}
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadArmament (CFILE *fp, tOOF_armament *pa)
{
	int	i;

nIndent += 2;
OOF_PrintLog ("reading armament\n");
if (!(pa->nBatts = OOF_ReadInt (fp, "nBatts"))) {
	nIndent -= 2;
	return 1;
	}
if (!(pa->pBatts = (tOOF_battery *) D2_ALLOC (pa->nBatts * sizeof (tOOF_battery)))) {
	nIndent -= 2;
	return OOF_FreeArmament (pa);
	}
for (i = 0; i < pa->nBatts; i++)
	if (!OOF_ReadBattery (fp, pa->pBatts + i)) {
		nIndent -= 2;
		return OOF_FreeArmament (pa);
		}
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_ReadFaceVert (CFILE *fp, tOOF_faceVert *pfv, int bFlipV)
{
nIndent += 2;
OOF_PrintLog ("reading face vertex\n");
pfv->nIndex = OOF_ReadInt (fp, "nIndex");
pfv->fu = OOF_ReadFloat (fp, "fu");
pfv->fv = OOF_ReadFloat (fp, "fv");
if (bFlipV)
	pfv->fv = -pfv->fv;
#if OOF_TEST_CUBE
/*!!!*/if (pfv->fu == 0.5) pfv->fu = 1.0;
/*!!!*/if (pfv->fv == 0.5) pfv->fv = 1.0;
/*!!!*/if (pfv->fu == -0.5) pfv->fu = 1.0;
/*!!!*/if (pfv->fv == -0.5) pfv->fv = 1.0;
#endif
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreeFace (tOOF_face *pf)
{
#if !OOF_MEM_OPT
D2_FREE (pf->pVerts);
#endif
return 0;
}

//------------------------------------------------------------------------------

int OOF_FindVertex (tOOF_subObject *pso, int i)
{
	tOOF_vector	v, *pv;
	int			j;

pv = pso->pvVerts;
v = pv [i];
for (j = 0; i < i; j++, pv++)
	if ((v.x == pv->x) && (v.y == pv->y) && (v.z == pv->z))
		return j;
return i;
}

//------------------------------------------------------------------------------

#define MAXGAP	0.01f

int OOF_FindEdge (tOOF_subObject *pso, int i0, int i1)
{
	int			i;
	tOOF_edge	h;
	tOOF_vector	v0, v1, hv0, hv1;

#ifdef _DEBUG
i0 = OOF_FindVertex (pso, i0);
i1 = OOF_FindVertex (pso, i1);
#endif
for (i = 0; i < pso->edges.nEdges; i++) {
	h = pso->edges.pEdges [i];
	if (((h.v0 [0] == i0) && (h.v1 [0] == i1)) || ((h.v0 [0] == i1) && (h.v1 [0] == i0)))
		return i;
	}
v0 = pso->pvVerts [i0]; 
v1 = pso->pvVerts [i1]; 
for (i = 0; i < pso->edges.nEdges; i++) {
	h = pso->edges.pEdges [i];
	hv0 = pso->pvVerts [h.v0 [0]]; 
	hv1 = pso->pvVerts [h.v1 [0]]; 
	if ((hv0.x == v0.x) && (hv0.y == v0.y) && (hv0.z == v0.z) &&
		 (hv1.x == v1.x) && (hv1.y == v1.y) && (hv1.z == v1.z))
		return i;
	if ((hv1.x == v0.x) && (hv1.y == v0.y) && (hv1.z == v0.z) &&
		 (hv0.x == v1.x) && (hv0.y == v1.y) && (hv0.z == v1.z))
		return i;
	}
for (i = 0; i < pso->edges.nEdges; i++) {
	h = pso->edges.pEdges [i];
	OOF_VecSub (&hv0, pso->pvVerts + h.v0 [0], &v0);
	OOF_VecSub (&hv1, pso->pvVerts + h.v1 [0], &v1);
	if ((OOF_VecMag (&hv0) < MAXGAP) && (OOF_VecMag (&hv1) < MAXGAP))
		return i;
	OOF_VecSub (&hv0, pso->pvVerts + h.v0 [0], &v1);
	OOF_VecSub (&hv1, pso->pvVerts + h.v1 [0], &v0);
	if ((OOF_VecMag (&hv0) < MAXGAP) && (OOF_VecMag (&hv1) < MAXGAP))
		return i;
	}
return -1;
}

//------------------------------------------------------------------------------

int OOF_AddEdge (tOOF_subObject *pso, tOOF_face *pf, int v0, int v1)
{
	int			i = OOF_FindEdge (pso, v0, v1);
	tOOF_edge	*pe;

if (pso->nFlags & (OOF_SOF_GLOW | OOF_SOF_THRUSTER))
	return -1;
if (i < 0)
	i = pso->edges.nEdges++;
pe = pso->edges.pEdges + i;
if (i < 0) {
	}
if (pe->pf [0]) {
	pe->pf [1] = pf;
	if (pf->bReverse) {
		pe->v0 [1] = v1;
		pe->v1 [1] = v0;
		}
	else {
		pe->v0 [1] = v0;
		pe->v1 [1] = v1;
		}
	}
else {
	pe->pf [0] = pf;
	if (pf->bReverse) {
		pe->v0 [0] = v1;
		pe->v1 [0] = v0;
		}
	else {
		pe->v0 [0] = v0;
		pe->v1 [0] = v1;
		}
	}
return i;
}

//------------------------------------------------------------------------------

inline tOOF_vector *OOF_CalcFaceCenter (tOOF_subObject *pso, tOOF_face *pf)
{
	tOOF_faceVert	*pfv = pf->pVerts;
	tOOF_vector		vc, *pv = pso->pvVerts;
	int				i;

vc.x = vc.y = vc.z = 0.0f;
for (i = pf->nVerts; i; i--, pfv++)
	OOF_VecInc (&vc, pv + pfv->nIndex);
OOF_VecScale (&vc, 1.0f / (float) pf->nVerts);
pf->vCenter = vc;
return &pf->vCenter;
}

//------------------------------------------------------------------------------

inline tOOF_vector *OOF_CalcFaceNormal (tOOF_subObject *pso, tOOF_face *pf)
{
	tOOF_vector		*pv = pso->pvRotVerts;
	tOOF_faceVert	*pfv = pf->pVerts;

return OOF_VecNormal (&pf->vRotNormal, pv + pfv [0].nIndex, pv + pfv [1].nIndex, pv + pfv [2].nIndex);
}

//------------------------------------------------------------------------------

#if OOF_TEST_CUBE
/*!!!*/static int nTexId = 0;
#endif

int OOF_ReadFace (CFILE *fp, tOOF_subObject *pso, tOOF_face *pf, tOOF_faceVert *pfv, int bFlipV)
{
	tOOF_face	f;
	int			i, v0 = 0;
	tOOF_edge	e;

nIndent += 2;
OOF_PrintLog ("reading face\n");
memset (&f, 0, sizeof (f));
OOF_ReadVector (fp, &f.vNormal, "vNormal");
#if 0
f.vNormal.x = -f.vNormal.x;
f.vNormal.y = -f.vNormal.y;
f.vNormal.z = -f.vNormal.z;
#endif
f.nVerts = OOF_ReadInt (fp, "nVerts");
f.bTextured = OOF_ReadInt (fp, "bTextured");
if (f.bTextured) {
	f.texProps.nTexId = OOF_ReadInt (fp, "texProps.nTexId");
#if OOF_TEST_CUBE
/*!!!*/	f.texProps.nTexId = nTexId % 6;
/*!!!*/	nTexId++;
#endif
	}
else {
	f.texProps.color.r = OOF_ReadByte (fp, "texProps.color.r");
	f.texProps.color.g = OOF_ReadByte (fp, "texProps.color.g");
	f.texProps.color.b = OOF_ReadByte (fp, "texProps.color.b");
	}
#if OOF_MEM_OPT
if (pfv) {
	f.pVerts = pfv;
#else
	if (!(f.pVerts = (tOOF_faceVert *) D2_ALLOC (f.nVerts * sizeof (tOOF_faceVert)))) {
		nIndent -= 2;
		return OOF_FreeFace (&f);
		}
#endif
	OOF_InitMinMax (&f.vMin, &f.vMax);
	e.v1 [0] = -1;
	for (i = 0; i < f.nVerts; i++)
		if (!OOF_ReadFaceVert (fp, f.pVerts + i, bFlipV)) {
			nIndent -= 2;
			return OOF_FreeFace (&f);
			}
		else {
			e.v0 [0] = e.v1 [0];
			e.v1 [0] = f.pVerts [i].nIndex;
			OOF_GetMinMax (pso->pvVerts + e.v1 [0], &f.vMin, &f.vMax);
			if (i)
				OOF_AddEdge (pso, pf, e.v0 [0], e.v1 [0]);
			else
				v0 = e.v1 [0];
			}
	OOF_AddEdge (pso, pf, e.v1 [0], v0);
	//OOF_CalcFaceNormal (pso, &f);
	OOF_CalcFaceCenter (pso, &f);
#if OOF_MEM_OPT
	}
else
	CFSeek (fp, f.nVerts * sizeof (tOOF_faceVert), SEEK_CUR);
#endif
f.fBoundingLength = OOF_ReadFloat (fp, "fBoundingLength");
f.fBoundingWidth = OOF_ReadFloat (fp, "fBoundingWidth");
*pf = f;
nIndent -= 2;
return f.nVerts;
}

//------------------------------------------------------------------------------

int OOF_FreeSubObject (tOOF_subObject *pso)
{
#if !OOF_MEM_OPT
	int	i;
#endif

D2_FREE (pso->pszName);
D2_FREE (pso->pszProps);
D2_FREE (pso->pvVerts);
D2_FREE (pso->pvRotVerts);
D2_FREE (pso->pVertColors);
D2_FREE (pso->pvNormals);
D2_FREE (pso->pfAlpha);
OOF_FreePosAnim (&pso->posAnim);
OOF_FreeRotAnim (&pso->rotAnim);
if (pso->faces.pFaces) {
#if OOF_MEM_OPT
	D2_FREE (pso->faces.pFaceVerts);
#else
	for (i = 0; i < pso->faces.nFaces; i++)
		OOF_FreeFace (pso->faces.pFaces + i);
#endif
	D2_FREE (pso->faces.pFaces);
	}
D2_FREE (pso->edges.pEdges);
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadSubObject (CFILE *fp, tOOFObject *po, int bFlipV)
{
	tOOF_subObject	so;
	int				h, i;
#if OOF_MEM_OPT
	int				bReadData, nPos, nFaceVerts = 0;
#endif
	char				szId [20] = "";

nIndent += 2;
OOF_PrintLog ("reading sub tObject\n");
memset (&so, 0, sizeof (so));
so.nIndex = OOF_ReadInt (fp, "nIndex");
if (so.nIndex >= OOF_MAX_SUBOBJECTS) {
	nIndent -= 2;
	return 0;
	}
so.nParent = OOF_ReadInt (fp, "nParent");
OOF_ReadVector (fp, &so.vNormal, "vNormal");
so.fd = OOF_ReadFloat (fp, "fd");
OOF_ReadVector (fp, &so.vPlaneVert, "vPlaneVert");
OOF_ReadVector (fp, &so.vOffset, "vOffset");
so.fRadius = OOF_ReadFloat (fp, "fRadius");
so.nTreeOffset = OOF_ReadInt (fp, "nTreeOffset");
so.nDataOffset = OOF_ReadInt (fp, "nDataOffset");
if (po->nVersion > 1805)
	OOF_ReadVector (fp, &so.vCenter, "vCenter");
if (!(so.pszName = OOF_ReadString (fp, "pszName", NULL)))
	return OOF_FreeSubObject (&so);
if (!(so.pszProps = OOF_ReadString (fp, "pszProps", NULL)))
	return OOF_FreeSubObject (&so);
OOF_SetModelProps (&so, so.pszProps);
so.nMovementType = OOF_ReadInt (fp, "nMovementType");
so.nMovementAxis = OOF_ReadInt (fp, "nMovementAxis");
so.pFSList = NULL;
if ((so.nFSLists = OOF_ReadInt (fp, "nFSLists")))
	CFSeek (fp, so.nFSLists * sizeof (int), SEEK_CUR);
so.nVerts = OOF_ReadInt (fp, "nVerts");
if (so.nVerts) {
	if (!(so.pvVerts = OOF_ReadVertList (fp, so.nVerts, &so.vMin, &so.vMax))) {
		nIndent -= 2;
		return OOF_FreeSubObject (&so);
		OOF_VecAdd (&so.vCenter, &so.vMin, &so.vMax);
		OOF_VecScale (&so.vCenter, 0.5f);
		}
	if (!(so.pvRotVerts = (tOOF_vector *) D2_ALLOC (so.nVerts * sizeof (tOOF_vector)))) {
		nIndent -= 2;
		return OOF_FreeSubObject (&so);
		}
	if (!(so.pVertColors = (tFaceColor *) D2_ALLOC (so.nVerts * sizeof (tFaceColor)))) {
		nIndent -= 2;
		return OOF_FreeSubObject (&so);
		}
	memset (so.pVertColors, 0, so.nVerts * sizeof (tFaceColor));
	if (!(so.pvNormals = OOF_ReadVertList (fp, so.nVerts, NULL, NULL))) {
		nIndent -= 2;
		return OOF_FreeSubObject (&so);
		}
	if (!(so.pfAlpha = (float *) D2_ALLOC (so.nVerts * sizeof (float)))) {
		nIndent -= 2;
		return OOF_FreeSubObject (&so);
		}
	for (i = 0; i < so.nVerts; i++)
		if (po->nVersion < 2300) 
			so.pfAlpha [i] = 1.0f;
		else {
			if (bLogOOF)
				sprintf (szId, "pfAlpha [%d]", i);
			so.pfAlpha [i] = OOF_ReadFloat (fp, szId);
			if	(so.pfAlpha [i] < 0.99)
				po->nFlags |= OOF_PMF_ALPHA;
			}
	}
so.faces.nFaces = OOF_ReadInt (fp, "nFaces");
if (!(so.faces.pFaces = (tOOF_face *) D2_ALLOC (so.faces.nFaces * sizeof (tOOF_face)))) {
	nIndent -= 2;
	return OOF_FreeSubObject (&so);
	}
#if OOF_MEM_OPT
nPos = CFTell (fp);
so.edges.nEdges = 0;
for (bReadData = 0; bReadData < 2; bReadData++) {
	CFSeek (fp, nPos, SEEK_SET);
	if (bReadData) {
		if (!(so.faces.pFaceVerts = (tOOF_faceVert *) D2_ALLOC (nFaceVerts * sizeof (tOOF_faceVert)))) {
			nIndent -= 2;
			return OOF_FreeSubObject (&so);
			}
		if (!(so.edges.pEdges = (tOOF_edge *) D2_ALLOC (nFaceVerts * sizeof (tOOF_edge)))) {
			nIndent -= 2;
			return OOF_FreeSubObject (&so);
			}
		memset (so.edges.pEdges, 0, nFaceVerts * sizeof (tOOF_edge));
		so.edges.nEdges = 0;
		}
	for (i = 0, nFaceVerts = 0; i < so.faces.nFaces; i++) {
		if (!(h = OOF_ReadFace (fp, &so, so.faces.pFaces + i, bReadData ? so.faces.pFaceVerts + nFaceVerts : NULL, bFlipV))) {
			nIndent -= 2;
			return OOF_FreeSubObject (&so);
			}
		nFaceVerts += h;
		}
	}
#else
for (i = 0; i < so.faces.nFaces; i++)
	if (!OOF_ReadFace (fp, &so, so.faces.pFaces + i, NULL, bFlipV)) {
		nIndent -= 2;
		return OOF_FreeSubObject (&so);
		}
#endif
po->pSubObjects [so.nIndex] = so;
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int OOF_ReleaseTextures (void)
{
	tOOFObject	*po;
	int			bCustom, i;

PrintLog ("releasing OOF model textures\n");
for (bCustom = 0; bCustom < 2; bCustom++)
	for (i = gameData.models.nHiresModels, po = gameData.models.oofModels [bCustom]; i; i--, po++)
		ReleaseModelTextures (&po->textures);
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReloadTextures (void)
{
	tOOFObject *po;
	int			bCustom, i;

PrintLog ("reloading OOF model textures\n");
for (bCustom = 0; bCustom < 2; bCustom++)
	for (i = gameData.models.nHiresModels, po = gameData.models.oofModels [bCustom]; i; i--, po++)
		if (!ReadModelTextures (&po->textures, po->nType, bCustom)) {
			OOF_FreeObject (po);
			return 0;
			}
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreeTextures (tOOFObject *po)
{
FreeModelTextures (&po->textures);
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadTextures (CFILE *fp, tOOFObject *po, short nType, int bCustom)
{
	tOOFObject	o = *po;
	int			i;
	char			szId [30];
#ifdef _DEBUG
	int			bOk = 1;
#endif

nIndent += 2;
OOF_PrintLog ("reading textures\n");
o.textures.nBitmaps = OOF_ReadInt (fp, "nBitmaps");
#if OOF_TEST_CUBE
/*!!!*/o.textures.nBitmaps = 6;
#endif
if (!(o.textures.pszNames = (char **) D2_ALLOC (o.textures.nBitmaps * sizeof (char **)))) {
	nIndent -= 2;
	return OOF_FreeTextures (&o);
	}
memset (o.textures.pszNames, 0, o.textures.nBitmaps * sizeof (char **));
i = o.textures.nBitmaps * sizeof (grsBitmap);
if (!(o.textures.pBitmaps = (grsBitmap *) D2_ALLOC (i))) {
	nIndent -= 2;
	return OOF_FreeTextures (&o);
	}
memset (o.textures.pBitmaps, 0, i);
for (i = 0; i < o.textures.nBitmaps; i++) {
	if (bLogOOF)
		sprintf (szId, "textures.pszId [%d]", i);
#if OOF_TEST_CUBE
if (!i)	//cube.oof only contains one texture
#endif
	if (!(o.textures.pszNames [i] = OOF_ReadString (fp, szId, "\001"))) {
		nIndent -= 2;
		return OOF_FreeTextures (&o);
		}
#if OOF_TEST_CUBE
if (!i)
	D2_FREE (o.textures.pszNames [i]);
o.textures.pszNames [i] = D2_ALLOC (20);
sprintf (o.textures.pszNames [i], "%d.tga", i + 1);
#endif
	if (!ReadModelTGA (o.textures.pszNames [i], o.textures.pBitmaps + i, nType, bCustom)) {
#ifdef _DEBUG
		bOk = 0;
#else
		nIndent -= 2;
		return OOF_FreeTextures (&o);
#endif
		}
	}
#ifdef _DEBUG
if (!bOk) {
	nIndent -= 2;
	return OOF_FreeTextures (&o);
	}
#endif
*po = o;
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreeObject (tOOFObject *po)
{
	int	i;

OOF_FreeTextures (po);
if (po->pSubObjects) {
	for (i = 0; i < po->nSubObjects; i++)
		OOF_FreeSubObject (po->pSubObjects + i);
	D2_FREE (po->pSubObjects);
	}
OOF_FreePointList (&po->gunPoints);
OOF_FreeAttachList (&po->attachPoints);
OOF_FreeSpecialList (&po->specialPoints);
if (po->armament.pBatts) {
	for (i = 0; i < po->armament.nBatts; i++)
		OOF_FreeBattery (po->armament.pBatts + i);
	D2_FREE (po->armament.pBatts);
	}
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReadObject (CFILE *fp, tOOFObject *po)
{
	tOOFObject	o = *po;

nIndent += 2;
OOF_PrintLog ("reading tObject\n");
o.nVersion = po->nVersion;
o.nSubObjects = OOF_ReadInt (fp, "nSubObjects");
if (o.nSubObjects >= OOF_MAX_SUBOBJECTS) {
	nIndent -= 2;
	return 0;
	}
o.fMaxRadius = OOF_ReadFloat (fp, "fMaxRadius");
OOF_ReadVector (fp, &o.vMin, "vMin");
OOF_ReadVector (fp, &o.vMax, "vMax");
o.nDetailLevels = OOF_ReadInt (fp, "nDetailLevels");
nIndent -= 2;
CFSeek (fp, o.nDetailLevels * sizeof (int), SEEK_CUR);
if (!(o.pSubObjects = (tOOF_subObject *) D2_ALLOC (o.nSubObjects * sizeof (tOOF_subObject))))
	return 0;
*po = o;
return 1;
}

//------------------------------------------------------------------------------

void BuildModelAngleMatrix (tOOF_matrix *pm, int a, tOOF_vector *pAxis)
{
float x = pAxis->x;
float y = pAxis->y;
float z = pAxis->z;
float s = (float) sin ((float) a);
float c = (float) cos ((float) a);
float t = 1.0f - c;
float i = t * x;
float j = s * z;
//pm->r.x = t * x * x + c;
pm->r.x = i * x + c;
i *= y;
//pm->r.y = t * x * y + s * z;
//pm->u.x = t * x * y - s * z;
pm->r.y = i + j;
pm->u.x = i - j;
i = t * z;
//pm->f.z = t * z * z + c;
pm->f.z = i * z + c;
i *= x;
j = s * y;
//pm->r.z = t * x * z - s * y;
//pm->f.x = t * x * z + s * y;
pm->r.z = i - j;
pm->f.x = i + j;
i = t * y;
//pm->u.y = t * y * y + c;
pm->u.y = i * y + c;
i *= z;
j = s * x;
//pm->u.z = t * y * z + s * x;	
//pm->f.y = t * y * z - s * x;
pm->u.z = i + j;
pm->f.y = i - j;
}

//------------------------------------------------------------------------------

void BuildAnimMatrices (tOOFObject *po)
{
	tOOF_subObject *pso;
	tOOF_matrix		mBase, mDest, mTemp;
	tOOF_rotFrame	*pf;
	int				i, j, a;

for (i = po->nSubObjects, pso = po->pSubObjects; i; i--, pso++) {
	OOF_MatIdentity (&mBase);
	for (j = pso->rotAnim.frameInfo.nFrames, pf = pso->rotAnim.pFrames; j; j--, pf++) {
		a = pf->nAngle;
		BuildModelAngleMatrix (&mTemp, a, &pf->vAxis);
		OOF_MatIdentity (&mDest);
		OOF_MatMul (&mDest, &mTemp, &mBase);
		mBase = mDest;
		pf->mMat = mBase;
		}
	}
}

//------------------------------------------------------------------------------

void AssignChildren (tOOFObject *po)
{
	tOOF_subObject *pso, *pParent;
	int				i;

for (i = 0, pso = po->pSubObjects; i < po->nSubObjects; i++, pso++) {
	int nParent = pso->nParent;
	if (nParent == i)
		pso->nParent = -1;
	else if (nParent != -1) {
		pParent = po->pSubObjects + nParent;
		pParent->children [pParent->nChildren++] = i;
		}
	}
}

//------------------------------------------------------------------------------

inline void RecursiveAssignBatt (tOOFObject *po, int iObject, int iBatt)
{
	tOOF_subObject	*pso = po->pSubObjects + iObject;
	int				i, nFlags = iBatt << OOF_WB_INDEX_SHIFT;

pso->nFlags |= nFlags | OOF_SOF_WB;	
for (i = 0; i < pso->nChildren; i++)
	RecursiveAssignBatt (po, pso->children [i], iBatt);
}

//------------------------------------------------------------------------------

void AssignBatteries (tOOFObject *po)
{
	tOOF_subObject	*pso;
	tOOF_battery	*pb;
	int				*pti;
	int				i, j, k;

for (i = 0, pso = po->pSubObjects; i < po->nSubObjects; i++, pso++) {
	if (!(pso->nFlags & OOF_SOF_TURRET))
		continue;
	for (j = 0, pb = po->armament.pBatts; j < po->armament.nBatts; j++, pb++)
		for (k = pb->nTurrets, pti = pb->pTurretIndex; k; k--, pti++)
			if (*pti == i) {
				RecursiveAssignBatt (po, i, j);
				j = po->armament.nBatts;
				break;
				}
	}
}

//------------------------------------------------------------------------------

void BuildPosTickRemapList (tOOFObject *po)
{
	int				i, j, k, t, nTicks;
	tOOF_subObject	*pso;

for (i = po->nSubObjects, pso = po->pSubObjects; i; i--, pso++) {
	nTicks = pso->posAnim.frameInfo.nLastFrame - pso->posAnim.frameInfo.nFirstFrame;
	for (j = 0, t = pso->posAnim.frameInfo.nFirstFrame; j < nTicks; j++, t++)
		if ((k = pso->posAnim.frameInfo.nFrames - 1))
			for (; k >= 0; k--)
				if (t >= pso->posAnim.pFrames [k].nStartTime) {
					pso->posAnim.pRemapTicks [j] = k;
					break;
					}
	}
}

//------------------------------------------------------------------------------

void BuildRotTickRemapList (tOOFObject *po)
{
	int				i, j, k, t, nTicks;
	tOOF_subObject	*pso;

for (i = po->nSubObjects, pso = po->pSubObjects; i; i--, pso++) {
	nTicks = pso->rotAnim.frameInfo.nLastFrame - pso->rotAnim.frameInfo.nFirstFrame;
	for (j = 0, t = pso->rotAnim.frameInfo.nFirstFrame; j < nTicks; j++, t++)
		if ((k = pso->rotAnim.frameInfo.nFrames - 1))
			for (; k >= 0; k--)
				if (t >= pso->rotAnim.pFrames [k].nStartTime) {
					pso->rotAnim.pRemapTicks [j] = k;
					break;
					}
	}
}

//------------------------------------------------------------------------------

void ConfigureSubObjects (tOOFObject *po)
{
	int				i, j;
	tOOF_subObject	*pso;

for (i = po->nSubObjects, pso = po->pSubObjects; i; i--, pso++) {
	if (!pso->rotAnim.frameInfo.nFrames)
		pso->nFlags &= ~(OOF_SOF_ROTATE | OOF_SOF_TURRET);

	if (pso->nFlags & OOF_SOF_FACING) {
		tOOF_vector v [30], avg;
		tOOF_face	*pf = pso->faces.pFaces;

		for (j = 0; j < pf->nVerts; j++)
			v [j] = pso->pvVerts [pf->pVerts [j].nIndex];
	
		pso->fRadius = (float) (sqrt (OOF_Centroid (&avg, v, pf->nVerts)) / 2);
		po->nFlags |= OOF_PMF_FACING;

		}

	if (pso->nFlags & (OOF_SOF_GLOW | OOF_SOF_THRUSTER)) {
		tOOF_vector v [30];
		tOOF_face	*pf = pso->faces.pFaces;

		for (j = 0; j < pf->nVerts; j++)
			v [j] = pso->pvVerts [pf->pVerts [j].nIndex];
		OOF_VecNormal (&pso->glowInfo.vNormal, v, v + 1, v + 2);
		po->nFlags |= OOF_PMF_FACING;	// Set this so we know when to draw
		}
	}
}

//------------------------------------------------------------------------------

void GetSubObjectBounds (tOOFObject *po, tOOF_subObject *pso, tOOF_vector vo)
{
	int	i;
	float	h;

vo.x += pso->vOffset.x;
vo.y += pso->vOffset.y;
vo.z += pso->vOffset.z;
if (po->vMax.x < (h = pso->vMax.x + vo.x))
	 po->vMax.x = h;
if (po->vMax.y < (h = pso->vMax.y + vo.y))
	 po->vMax.y = h;
if (po->vMax.z < (h = pso->vMax.z + vo.z))
	 po->vMax.z = h;
if (po->vMin.x > (h = pso->vMin.x + vo.x))
	 po->vMin.x = h;
if (po->vMin.y > (h = pso->vMin.y + vo.y))
	 po->vMin.y = h;
if (po->vMin.z > (h = pso->vMin.z + vo.z))
	 po->vMin.z = h;
for (i = 0; i < pso->nChildren; i++)
	GetSubObjectBounds (po, po->pSubObjects + pso->children [i], vo);
}

//------------------------------------------------------------------------------

void GetObjectBounds (tOOFObject *po)
{
	tOOF_subObject	*pso;
	tOOF_vector		vo;
	int				i;

vo.x = vo.y = vo.z = 0.0f;
OOF_InitMinMax (&po->vMin, &po->vMax);
for (i = 0, pso = po->pSubObjects; i < po->nSubObjects; i++, pso++)
	if (pso->nParent == -1)
		GetSubObjectBounds (po, pso, vo);
}

//------------------------------------------------------------------------------

int OOF_ReadFile (char *pszFile, tOOFObject *po, short nModel, short nType, int bFlipV, int bCustom)
{
	CFILE				cf;
	char				fileId [4];
	tOOFObject		o;
	int				i, nLength, nFrames, bTimed = 0;

bLogOOF = (fErr != NULL) && FindArg ("-printoof");
nIndent = 0;
OOF_PrintLog ("\nreading %s/%s\n", gameFolders.szModelDir [nType], pszFile);
if (!CFOpen (&cf, pszFile, gameFolders.szModelDir [nType], "rb", 0)) {
	OOF_PrintLog ("  file not found");
	return 0;
	}

if (!CFRead (fileId, sizeof (fileId), 1, &cf)) {
	OOF_PrintLog ("  invalid file id\n");
	CFClose (&cf);
	return 0;
	}
if (strncmp (fileId, "PSPO", 4)) {
	OOF_PrintLog ("  invalid file id\n");
	CFClose (&cf);
	return 0;
	}
memset (&o, 0, sizeof (o));
o.nVersion = OOF_ReadInt (&cf, "nVersion");
if (o.nVersion >= 2100)
	o.nFlags |= OOF_PMF_LIGHTMAP_RES;
if (o.nVersion >= 22) {
	bTimed = 1;
	o.nFlags |= OOF_PMF_TIMED;
	o.frameInfo.nFirstFrame = 0;
	o.frameInfo.nLastFrame = 0;
	}
o.nModel = nModel;
o.nType = nType;

while (!CFEoF (&cf)) {
	char chunkId [4];

	if (!CFRead (chunkId, sizeof (chunkId), 1, &cf)) {
		CFClose (&cf);
		return 0;
		}
	OOF_PrintLog ("  chunkId = '%c%c%c%c'\n", chunkId [0], chunkId [1], chunkId [2], chunkId [3]);
	nLength = OOF_ReadInt (&cf, "nLength");
	switch (ListType (chunkId)) {
		case 0:
			if (!OOF_ReadTextures (&cf, &o, nType, bCustom))
				return OOF_FreeObject (&o);
			break;

		case 1:
			if (!OOF_ReadObject (&cf, &o))
				return OOF_FreeObject (&o);
			break;

		case 2:
			if (!OOF_ReadSubObject (&cf, &o, bFlipV))
				return OOF_FreeObject (&o);
			break;

		case 3:
			if (!OOF_ReadPointList (&cf, &o.gunPoints, o.nVersion >= 1908, MAX_GUNS))
				return OOF_FreeObject (&o);
			break;

		case 4:
			if (!OOF_ReadSpecialList (&cf, &o.specialPoints))
				return OOF_FreeObject (&o);
			break;

		case 5:
			if (!OOF_ReadAttachList (&cf, &o.attachPoints))
				return OOF_FreeObject (&o);
			break;

		case 6:
			nFrames = o.frameInfo.nFrames;
			if (!bTimed)
				o.frameInfo.nFrames = OOF_ReadInt (&cf, "nFrames");
			for (i = 0; i < o.nSubObjects; i++)
				if (!OOF_ReadPosAnim (&cf, &o, &o.pSubObjects [i].posAnim, bTimed))
				return OOF_FreeObject (&o);
			if (o.frameInfo.nFrames < nFrames)
				o.frameInfo.nFrames = nFrames;
			break;

		case 7:
		case 8:
			nFrames = o.frameInfo.nFrames;
			if (!bTimed)
				o.frameInfo.nFrames = OOF_ReadInt (&cf, "nFrames");
			for (i = 0; i < o.nSubObjects; i++)
				if (!OOF_ReadRotAnim (&cf, &o, &o.pSubObjects [i].rotAnim, bTimed))
					return OOF_FreeObject (&o);
			if (o.frameInfo.nFrames < nFrames)
				o.frameInfo.nFrames = nFrames;
			break;

		case 9:
			if (!OOF_ReadArmament (&cf, &o.armament))
				return OOF_FreeObject (&o);
			break;

		case 10:
			if (!OOF_ReadAttachNormals (&cf, &o.attachPoints))
				return OOF_FreeObject (&o);
			break;

		default:
			CFSeek (&cf, nLength, SEEK_CUR);
			break;
		}
	}
CFClose (&cf);
ConfigureSubObjects (&o);
BuildAnimMatrices (&o);
AssignChildren (&o);
AssignBatteries (&o);
BuildPosTickRemapList (po);
BuildRotTickRemapList (po);
*po = o;
gameData.models.bHaveHiresModel [po - gameData.models.oofModels [bCustom]] = 1;
return 1;
}

//------------------------------------------------------------------------------
//eof

