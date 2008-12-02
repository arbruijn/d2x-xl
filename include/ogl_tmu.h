//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_TMU_H
#define _OGL_TMU_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

//------------------------------------------------------------------------------

typedef void tInitTMU (int);
typedef tInitTMU *pInitTMU;

//------------------------------------------------------------------------------

static inline void InitTMU (int i, int bVertexArrays)
{
	static GLuint tmuIds [] = {GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2_ARB};

if (glIsList (g3InitTMU [i][bVertexArrays]))
	glCallList (g3InitTMU [i][bVertexArrays]);
else {
	g3InitTMU [i][bVertexArrays] = glGenLists (1);
	if (g3InitTMU [i][bVertexArrays])
		glNewList (g3InitTMU [i][bVertexArrays], GL_COMPILE);
	OglActiveTexture (tmuIds [i], 0);
	if (bVertexArrays) {
		OglActiveTexture (tmuIds [i], bVertexArrays);
		}
	glEnable (GL_TEXTURE_2D);
	if (i == 0)
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	else if (i == 1)
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	else
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (g3InitTMU [i][bVertexArrays]) {
		glEndList ();
		InitTMU (i, bVertexArrays);
		}
	}
}

//------------------------------------------------------------------------------

static inline void InitTMU0 (int bVertexArrays, int bLightmaps = 0)
{
if (glIsList (g3InitTMU [0][bVertexArrays]))
	glCallList (g3InitTMU [0][bVertexArrays]);
else 
	{
	g3InitTMU [0][bVertexArrays] = glGenLists (1);
	if (g3InitTMU [0][bVertexArrays])
		glNewList (g3InitTMU [0][bVertexArrays], GL_COMPILE);
	if (bVertexArrays)
		glClientActiveTexture (GL_TEXTURE0);
	glActiveTexture (GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (g3InitTMU [0][bVertexArrays]) {
		glEndList ();
		InitTMU0 (bVertexArrays);
		}
	}
}

//------------------------------------------------------------------------------

static inline void InitTMU1 (int bVertexArrays, int bLightmaps = 0)
{
if (glIsList (g3InitTMU [1][bVertexArrays]))
	glCallList (g3InitTMU [1][bVertexArrays]);
else 
	{
	g3InitTMU [1][bVertexArrays] = glGenLists (1);
	if (g3InitTMU [1][bVertexArrays])
		glNewList (g3InitTMU [1][bVertexArrays], GL_COMPILE);
	if (bVertexArrays)
		glClientActiveTexture (GL_TEXTURE1);
	glActiveTexture (GL_TEXTURE1);
	glEnable (GL_TEXTURE_2D);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, bLightmaps ? GL_MODULATE : GL_DECAL);
	if (g3InitTMU [1][bVertexArrays]) {
		glEndList ();
		InitTMU1 (bVertexArrays);
		}
	}
}

//------------------------------------------------------------------------------

static inline void InitTMU2 (int bVertexArrays, int bLightmaps = 0)
{
if (glIsList (g3InitTMU [2][bVertexArrays]))
	glCallList (g3InitTMU [2][bVertexArrays]);
else 
	{
	g3InitTMU [2][bVertexArrays] = glGenLists (1);
	if (g3InitTMU [2][bVertexArrays])
		glNewList (g3InitTMU [2][bVertexArrays], GL_COMPILE);
	if (bVertexArrays)
		glClientActiveTexture (GL_TEXTURE2);
	glActiveTexture (GL_TEXTURE2);
	glEnable (GL_TEXTURE_2D);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, bLightmaps ? GL_DECAL : GL_MODULATE);
	if (g3InitTMU [2][bVertexArrays]) {
		glEndList ();
		InitTMU2 (bVertexArrays);
		}
	}
}

//------------------------------------------------------------------------------

static inline void InitTMU3 (int bVertexArrays, int bLightmaps = 0)
{
if (glIsList (g3InitTMU [3][bVertexArrays]))
	glCallList (g3InitTMU [3][bVertexArrays]);
else 
	{
	g3InitTMU [3][bVertexArrays] = glGenLists (1);
	if (g3InitTMU [3][bVertexArrays])
		glNewList (g3InitTMU [3][bVertexArrays], GL_COMPILE);
	glActiveTexture (GL_TEXTURE2_ARB);
	if (bVertexArrays)
		glClientActiveTexture (GL_TEXTURE2_ARB);
	glEnable (GL_TEXTURE_2D);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (g3InitTMU [3][bVertexArrays]) {
		glEndList ();
		InitTMU3 (bVertexArrays);
		}
	}
}

//------------------------------------------------------------------------------

static inline void ExitTMU (int bVertexArrays)
{
if (glIsList (g3ExitTMU [bVertexArrays]))
	glCallList (g3ExitTMU [bVertexArrays]);
else 
	{
	g3ExitTMU [bVertexArrays] = glGenLists (1);
	if (g3ExitTMU [bVertexArrays])
		glNewList (g3ExitTMU [bVertexArrays], GL_COMPILE);
	glActiveTexture (GL_TEXTURE2_ARB);
	if (bVertexArrays)
		glClientActiveTexture (GL_TEXTURE2_ARB);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	glActiveTexture (GL_TEXTURE1);
	if (bVertexArrays)
		glClientActiveTexture (GL_TEXTURE1);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	glActiveTexture (GL_TEXTURE0);
	if (bVertexArrays)
		glClientActiveTexture (GL_TEXTURE0);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	if (g3ExitTMU [bVertexArrays]) {
		glEndList ();
		ExitTMU (bVertexArrays);
		}
	}
}

//------------------------------------------------------------------------------

#define	G3_BIND(_tmu,_bmP,_lmP,_bClient) \
			glActiveTexture (_tmu); \
			if (_bClient) \
				glClientActiveTexture (_tmu); \
			if (_bmP) {\
				if (OglBindBmTex (_bmP, 1, 3)) \
					return 1; \
				(_bmP) = (_bmP)->CurFrame (-1); \
				OglTexWrap ((_bmP)->TexInfo (), GL_REPEAT); \
				} \
			else { \
				OGL_BINDTEX ((_lmP)->handle); \
				OglTexWrap (NULL, GL_CLAMP); \
				}

#define	INIT_TMU(_initTMU,_tmu,_bmP,_lmP,_bClient,bLightmaps) \
			_initTMU (_bClient, bLightmaps); \
			G3_BIND (_tmu,_bmP,_lmP,_bClient)


//------------------------------------------------------------------------------

#endif
