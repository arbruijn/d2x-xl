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

#define	HEADLIGHT_BOOST_SCALE		8		//how much to scale light when have headlight boost
#define	MAX_HEADLIGHTS					8
#define	MAX_DIST_LOG					6							//log (MAX_DIST-expressed-as-integer)
#define	MAX_DIST							(I2X (1) << MAX_DIST_LOG)	//no light beyond this dist
#define	HEADLIGHT_CONE_DOT			(I2X (9) / 10)
#define	HEADLIGHT_SCALE				(I2X (10))
#define	HEADLIGHT_TRANSFORMATION	2

extern fix		xBeamBrightness;

void SetDynamicLight (void);

int LightingMethod (void);

#if 0
fix ComputeHeadlight (CFixVector *point, fix xFaceLight);
fix ComputeHeadlightLightOnObject (CObject *objP);
void ToggleHeadlight (void);
void SetupHeadlight (CDynLight *pl, CDynLight *prl);
void TransformHeadlights (void);
int AddOglHeadlight (CObject *objP);
void RemoveOglHeadlight (CObject *objP);
void UpdateOglHeadlight (void);
#endif

void InitHeadlightShaders (int nLights);

#endif //_HEADLIGHT_H 
