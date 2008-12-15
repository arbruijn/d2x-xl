/* $Id: globvars.h,v 1.2 2002/07/17 21:55:19 bradleyb Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * 
 * Private (internal) header for 3d library
 * 
 * Old Log:
 *
 * Revision 1.2  1995/09/13  11:31:19  allender
 * added fxCanvW2 and vxCanvH2 for PPC implementation
 *
 * Revision 1.1  1995/05/05  08:51:02  allender
 * Initial revision
 *
 * Revision 1.1  1995/04/17  04:07:58  matt
 * Initial revision
 * 
 * 
 */

#ifndef _GLOBVARS_H
#define _GLOBVARS_H

#define MAX_POINTS_IN_POLY 100

extern int nCanvasWidth,nCanvasHeight;	//the actual width & height
extern fix xCanvW2,xCanvH2;				//fixed-point width,height/2
extern double fxCanvW2, fxCanvH2;

typedef struct CViewInfo {
	CFixVector		pos;
	CAngleVector	playerHeadAngles;
	int				bUsePlayerHeadAngles;
	CFixMatrix		view [2];
	CFixVector		scale;
	CFloatVector	scalef;
	CFixVector		windowScale;		//scaling for window aspect
	CFloatVector	posf [2];
	CFloatMatrix	viewf [3];
	fix				zoom;
	float				glZoom;
} CViewInfo;

extern CViewInfo	viewInfo;

extern int nFreePoints;

//vertex buffers for polygon drawing and clipping
//list of 2d coords
extern fix polyVertList[];

#endif
