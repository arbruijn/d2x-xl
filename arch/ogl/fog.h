//fog.c

#include "ogl_init.h"

typedef struct tFog {
	float		color [4];
	float		density;
	float		nearDist;
	float		farDist;
	GLenum	mode;
} tFog;

tFog fog = {{0.6f,0.58f,0.79f,0.0f}, 0.1f, 0.05f, 200.0f, GL_EXP};

#ifndef GL_EXT_fog_coord
#	define GL_EXT_fog_coord 1

#	define GL_FOG_COORDINATE_SOURCE_EXT				0x8450
#	define GL_FOG_COORDINATE_EXT						0x8451
#	define GL_FRAGMENT_DEPTH_EXT						0x8452
#	define GL_CURRENT_FOG_COORDINATE_EXT			0x8453
#	define GL_FOG_COORDINATE_ARRAY_TYPE_EXT		0x8454
#	define GL_FOG_COORDINATE_ARRAY_STRIDE_EXT		0x8455
#	define GL_FOG_COORDINATE_ARRAY_POINTER_EXT	0x8456
#	define GL_FOG_COORDINATE_ARRAY_EXT				0x8457

typedef void (APIENTRY * PFNGLFOGCOORDFEXTPROC) (GLfloat coord);
typedef void (APIENTRY * PFNGLFOGCOORDFVEXTPROC) (const GLfloat *coord);
typedef void (APIENTRY * PFNGLFOGCOORDDEXTPROC) (GLdouble coord);
typedef void (APIENTRY * PFNGLFOGCOORDDVEXTPROC) (const GLdouble *coord);
typedef void (APIENTRY * PFNGLFOGCOORDPOINTEREXTPROC) (GLenum type, GLsizei stride, const GLvoid *pointer);
#	endif

#ifdef _WIN32
PFNGLFOGCOORDFEXTPROC glFogCoordfEXT = NULL;
PFNGLFOGCOORDFVEXTPROC glFogCoordfvEXT = NULL;
PFNGLFOGCOORDDEXTPROC glFogCoorddEXT = NULL;
PFNGLFOGCOORDDVEXTPROC glFogCoorddvEXT = NULL;
PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointerEXT = NULL;
#endif


