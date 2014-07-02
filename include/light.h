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
#define MIN_LIGHT_DIST  (I2X (1)*4)

extern fix xBeamBrightness;

extern void SetDynamicLight (void);

int32_t LightingMethod (void);

// compute the average dynamic light in a CSegment.  Takes the CSegment number
fix ComputeSegDynamicLight (int32_t nSegment);

// compute the lighting for an CObject.  Takes a pointer to the CObject,
// and possibly a rotated 3d point.  If the point isn't specified, the
// CObject's center point is rotated.
fix ComputeObjectLight(CObject *obj,CFixVector *rotated_pnt);

void ComputeEngineGlow (CObject *obj, fix *engine_glowValue);

// returns ptr to flickering light structure, or NULL if can't find
CVariableLight *FindVariableLight (int32_t nSegment, int32_t nSide);

// turn flickering off (because light has been turned off)
void DisableVariableLight (int32_t nSegment, int32_t nSide);

// turn flickering off (because light has been turned on)
void EnableVariableLight (int32_t nSegment, int32_t nSide);

// returns 1 if ok, 0 if error
int32_t AddVariableLight(int32_t nSegment, int32_t nSide, fix delay, unsigned long mask);

void ReadVariableLight (CVariableLight *fl, CFile& cf);

void InitTextureBrightness (void);

int32_t IsLight (int32_t tMapNum);
#endif //_LIGHT_H
