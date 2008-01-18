/* $Id: light.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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

#ifndef _HEADLIGHT_H
#define _HEADLIGHT_H

#define HEADLIGHT_BOOST_SCALE		8		//how much to scale light when have headlight boost
#define	MAX_HEADLIGHTS				8
#define MAX_DIST_LOG					6							//log (MAX_DIST-expressed-as-integer)
#define MAX_DIST						(f1_0<<MAX_DIST_LOG)	//no light beyond this dist
#define	HEADLIGHT_CONE_DOT		(F1_0*9/10)
#define	HEADLIGHT_SCALE			(F1_0*10)
#define HEADLIGHT_TRANSFORMATION	0

extern tObject	*Headlights [MAX_HEADLIGHTS];
extern int		nHeadLights;
extern fix		xBeamBrightness;

extern void SetDynamicLight (void);

int LightingMethod (void);

// Compute the lighting from the headlight for a given vertex on a face.
// Takes:
//  point - the 3d coords of the point
//  face_light - a scale factor derived from the surface normal of the face
// If no surface normal effect is wanted, pass F1_0 for face_light
fix ComputeHeadLight (vmsVector *point, fix xFaceLight);
fix ComputeHeadlightLightOnObject (tObject *objP);
void ToggleHeadLight (void);
void InitLightingShaders (int nLights);
void TransformOglHeadLight (tDynLight *pl, tShaderLight *psl);
int AddOglHeadLight (tObject *objP);
void RemoveOglHeadLight (tObject *objP);
void UpdateOglHeadLight (void);

#endif //_HEADLIGHT_H 
