/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _LIGHT_H
#define _LIGHT_H

#include "endlevel.h"

#define MAX_LIGHT       0x10000     // max value
#define MIN_LIGHT_DIST  (F1_0*4)

extern fix xBeamBrightness;

extern void SetDynamicLight (void);

int LightingMethod (void);

// compute the average dynamic light in a CSegment.  Takes the CSegment number
fix ComputeSegDynamicLight (int nSegment);

// compute the lighting for an CObject.  Takes a pointer to the CObject,
// and possibly a rotated 3d point.  If the point isn't specified, the
// CObject's center point is rotated.
fix ComputeObjectLight(CObject *obj,CFixVector *rotated_pnt);

void ComputeEngineGlow (CObject *obj, fix *engine_glowValue);

// returns ptr to flickering light structure, or NULL if can't find
tVariableLight *FindVariableLight (int nSegment, int nSide);

// turn flickering off (because light has been turned off)
void DisableVariableLight (int nSegment, int nSide);

// turn flickering off (because light has been turned on)
void EnableVariableLight (int nSegment, int nSide);

// returns 1 if ok, 0 if error
int AddVariableLight(int nSegment, int nSide, fix delay, unsigned long mask);

void ReadVariableLight (tVariableLight *fl, CFile& cf);

void InitTextureBrightness (void);

int IsLight (int tMapNum);
#endif //_LIGHT_H
