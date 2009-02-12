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

#ifndef _GPGPU_LIGHTING_H
#define _GPGPU_LIGHTING_H

#include "descent.h"
#include "endlevel.h"

#if DBG
#	define GPGPU_VERTEX_LIGHTING 0
#else
#	define GPGPU_VERTEX_LIGHTING 0
#endif

#define GPGPU_LIGHT_DRAWARRAYS	1
#define GPGPU_LIGHT_BUFFERS		4
#define VL_SHADER_BUFFERS			GPGPU_LIGHT_BUFFERS
#define GPGPU_LIGHT_BUF_WIDTH		32
#define GPGPU_LIGHT_BUF_SIZE		(GPGPU_LIGHT_BUF_WIDTH * GPGPU_LIGHT_BUF_WIDTH)

//------------------------------------------------------------------------------

typedef struct tVertLightIndex {
	short			nVertex;
	short			nLights;
	tRgbaColorf	color;
} tVertLightIndex;

typedef struct tVertLightData {
	short					nVertices;
	short					nLights;
	CFloatVector		buffers [GPGPU_LIGHT_BUFFERS][GPGPU_LIGHT_BUF_SIZE];
	CFloatVector		colors [GPGPU_LIGHT_BUF_SIZE];
	tVertLightIndex	index [GPGPU_LIGHT_BUF_SIZE];
} tVertLightData;

//------------------------------------------------------------------------------

class CGPGPULighting {
	private:
		GLhandleARB		m_hShaderProg;
		GLhandleARB		m_hVertShader; 
		GLhandleARB		m_hFragShader; 
		tVertLightData	m_vld;


	public:
		CGPGPULighting () { Init (); }
		void Init (void);
		void Begin (void);
		void End (void);
		void InitShader (void);
		int Compute (short nVertex, int nState, tFaceColor *colorP);

	private:
		GLuint CreateBuffer (int i);
		void ComputeFragLight (float lightRange);
		int Render (void);
	};

extern CGPGPULighting gpgpuLighting;

//------------------------------------------------------------------------------

#endif /* _GPGPU_LIGHTING_H */
