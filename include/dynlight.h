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

#ifndef _DYNLIGHT_H
#define _DYNLIGHT_H

#define	MAX_LIGHTS_PER_PIXEL 8

void RegisterLight (tFaceColor *pc, short nSegment, short nSide);
int AddDynLight (grsFace *faceP, tRgbaColorf *pc, fix xBrightness, 
					  short nSegment, short nSide, short nOwner, short nTexture, vmsVector *vPos);
int RemoveDynLight (short nSegment, short nSide, short nObject);
void AddDynGeometryLights (void);
void DeleteDynLight (short nLight);
void DeleteLightningLights (void);
void RemoveDynLights (void);
void RemoveDynLightningLights (void);
void SetDynLightPos (short nObject);
void SetDynLightMaterial (short nSegment, short nSide, short nObject);
void MoveDynLight (short nObject);
void TransformDynLights (int bStatic, int bVariable);
short FindDynLight (short nSegment, short nSide, short nObject);
int ToggleDynLight (short nSegment, short nSide, short nObject, int bState);
void SetDynLightMaterial (short nSegment, short nSide, short nObject);
void SetNearestVertexLights (int nFace, int nVertex, vmsVector *vNormalP, ubyte nType, int bStatic, int bVariable, int nThread);
int SetNearestFaceLights (grsFace *faceP, int bTextured);
short SetNearestPixelLights (int nSegment, vmsVector *vPixelPos, float fLightRad, int nThread);
void SetNearestStaticLights (int nSegment, int bStatic, ubyte nType, int nThread);
void ResetNearestStaticLights (int nSegment, int nThread);
void ResetNearestVertexLights (int nVertex, int nThread);
short SetNearestSegmentLights (int nSegment, int nFace, int bVariable, int nType, int nThread);
void ComputeStaticVertexLights (int nVertex, int nMax, int nThread);
void ComputeStaticDynLighting (int nLevel);
tShaderLight *GetActiveShaderLight (tActiveShaderLight *activeLightsP, int nThread);
int InitPerPixelLightingShader (int nType, int nLights);
void ResetPerPixelLightingShaders (void);
void InitHeadlightShaders (int nLights);
char *BuildLightingShader (char *pszTemplate, int nLights);
tFaceColor *AvgSgmColor (int nSegment, vmsVector *vPos);
int IsLight (int tMapNum);
void ResetUsedLight (tShaderLight *psl, int nThread);
void ResetUsedLights (int bVariable, int nThread);
void ResetActiveLights (int nThread, int nActive);

#define	SHOW_DYN_LIGHT \
			(!(gameStates.app.bNostalgia || gameStates.render.bBriefing || (gameStates.app.bEndLevelSequence >= EL_OUTSIDE)) && \
			 /*gameStates.render.bHaveDynLights &&*/ \
			 gameOpts->render.nLightingMethod)

#define HAVE_DYN_LIGHT	(gameStates.render.bHaveDynLights && SHOW_DYN_LIGHT)

#define	APPLY_DYN_LIGHT \
			(gameStates.render.bUseDynLight && (gameOpts->ogl.bLightObjects || gameStates.render.nState))

extern tFaceColor tMapColor, lightColor, vertColors [8];

#endif //_DYNLIGHT_H
