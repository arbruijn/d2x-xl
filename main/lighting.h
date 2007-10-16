/* $Id: lighting.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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

#ifndef _LIGHTING_H
#define _LIGHTING_H

#define MAX_LIGHT       0x10000     // max value

#define MIN_LIGHT_DIST  (F1_0*4)

extern fix Beam_brightness;
#if 0
extern fix dynamicLight[MAX_VERTICES];
extern tRgbColorf dynamicColor[MAX_VERTICES];
extern char bGotDynColor [MAX_VERTICES];
#endif

extern void SetDynamicLight(void);

// Compute the lighting from the headlight for a given vertex on a face.
// Takes:
//  point - the 3d coords of the point
//  face_light - a scale factor derived from the surface normal of the face
// If no surface normal effect is wanted, pass F1_0 for face_light
fix compute_headlight_light(vmsVector *point,fix face_light);

// compute the average dynamic light in a tSegment.  Takes the tSegment number
fix ComputeSegDynamicLight(int nSegment);

// compute the lighting for an tObject.  Takes a pointer to the tObject,
// and possibly a rotated 3d point.  If the point isn't specified, the
// tObject's center point is rotated.
fix ComputeObjectLight(tObject *obj,vmsVector *rotated_pnt);
void ComputeEngineGlow (tObject *obj, fix *engine_glowValue);
// turn headlight boost on & off
void toggle_headlight_active(void);

// returns ptr to flickering light structure, or NULL if can't find
tFlickeringLight *FindFlicker(int nSegment, int nSide);

// turn flickering off (because light has been turned off)
void DisableFlicker(int nSegment, int nSide);

// turn flickering off (because light has been turned on)
void EnableFlicker(int nSegment, int nSide);

// returns 1 if ok, 0 if error
int AddFlicker(int nSegment, int nSide, fix delay, unsigned long mask);

void ReadFlickeringLight(tFlickeringLight *fl, CFILE *fp);

void InitTextureBrightness (void);

void RegisterLight (tFaceColor *pc, short nSegment, short nSide);
int AddDynLight (tRgbaColorf *pc, fix xBrightness, short nSegment, short nSide, short nOwner, vmsVector *vPos);
int RemoveDynLight (short nSegment, short nSide, short nObject);
void AddDynLights (void);
void DeleteDynLight (short nLight);
void DeleteLightningLights (void);
void RemoveDynLights (void);
void RemoveDynLightningLights (void);
void SetDynLightPos (short nObject);
void MoveDynLight (short nObject);
void TransformDynLights (int bStatic, int bVariable);
short FindDynLight (short nSegment, short nSide, short nObject);
int ToggleDynLight (short nSegment, short nSide, short nObject, int bState);
void SetDynLightMaterial (short nSegment, short nSide, short nObject);
void SetNearestVertexLights (int nVertex, ubyte nType, int bStatic, int bVariable, int nThread);
void SetNearestStaticLights (int nSegment, ubyte nType, int nThread);
short SetNearestDynamicLights (int nSegment, int bVariable, int nThread);
void ComputeStaticVertexLights (int nVertex, int nMax, int nThread);
void ComputeStaticDynLighting (void);
void InitLightingShaders (void);
tFaceColor *AvgSgmColor (int nSegment, vmsVector *vPos);
int IsLight (int tMapNum);

#define	SHOW_DYN_LIGHT \
			(!(gameStates.app.bNostalgia || gameStates.render.bBriefing || gameStates.app.bEndLevelSequence) && \
			 gameStates.render.bHaveDynLights && \
			 gameOpts->render.bDynLighting)

#define	APPLY_DYN_LIGHT \
			(gameStates.render.bUseDynLight && (gameOpts->ogl.bLightObjects || gameStates.render.nState))

extern tFaceColor tMapColor, lightColor, vertColors [8];

#endif /* _LIGHTING_H */
