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
	CFloatVector	color;
} __pack__ tVertLightIndex;

typedef struct tVertLightData {
	short					nVertices;
	short					nLights;
	CFloatVector		buffers [GPGPU_LIGHT_BUFFERS][GPGPU_LIGHT_BUF_SIZE];
	CFloatVector		colors [GPGPU_LIGHT_BUF_SIZE];
	tVertLightIndex	index [GPGPU_LIGHT_BUF_SIZE];
} __pack__ tVertLightData;

//------------------------------------------------------------------------------

class CGPGPULighting {
	private:
		int				m_nShaderProg;
		tVertLightData	m_vld;


	public:
		CGPGPULighting () { Init (); }
		void Init (void);
		void Begin (void);
		void End (void);
		void InitShader (void);
		int Compute (int nVertex, int nState, CFaceColor *colorP);

	private:
		GLuint CreateBuffer (int i);
		void ComputeFragLight (float lightRange);
		int Render (void);
	};

extern CGPGPULighting gpgpuLighting;

//------------------------------------------------------------------------------

#endif /* _GPGPU_LIGHTING_H */
