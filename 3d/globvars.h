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

#ifndef _TRANSFORMATION_H
#define _TRANSFORMATION_H

class CTransformation {
	public:
		CFixVector		pos;
		CAngleVector	playerHeadAngles;
		int32_t				bUsePlayerHeadAngles;
		CFixMatrix		view [2];
		CFixVector		scale;
		CFixVector		windowScale;		//scaling for window aspect
		CFloatVector	posf;
		CFloatMatrix	viewf [2];
		fix				zoom;
		float				glZoom;
		float				glPosf [4];
		float				glViewf [16];
	public:
		CTransformation () { Init (); }
		void Init (void);

		memset (this, 0, sizeof (*this)); }
	};

extern CTransformation	transformation;

//vertex buffers for polygon drawing and clipping
//list of 2d coords
extern fix polyVertList[];

#endif
