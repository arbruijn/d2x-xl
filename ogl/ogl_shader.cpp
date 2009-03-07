/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
#else
# include <malloc.h>
# include <SDL.h>
#endif

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "texmerge.h"
#include "ogl_defs.h"
#include "ogl_shader.h"
#include "ogl_fastrender.h"
#include "glare.h"
#include "rendermine.h"
#include "gpgpu_lighting.h"

#ifndef GL_VERSION_20
PFNGLCREATESHADEROBJECTARBPROC	glCreateShaderObject = NULL;
PFNGLSHADERSOURCEARBPROC			glShaderSource = NULL;
PFNGLCOMPILESHADERARBPROC			glCompileShader = NULL;
PFNGLCREATEPROGRAMOBJECTARBPROC	glCreateProgramObject = NULL;
PFNGLATTACHOBJECTARBPROC			glAttachObject = NULL;
PFNGLLINKPROGRAMARBPROC				glLinkProgram = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC		glUseProgramObject = NULL;
PFNGLDELETEOBJECTARBPROC			glDeleteObject = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC	glGetObjectParameteriv = NULL;
PFNGLGETINFOLOGARBPROC				glGetInfoLog = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC	glGetUniformLocation = NULL;
PFNGLUNIFORM4FARBPROC				glUniform4f = NULL;
PFNGLUNIFORM3FARBPROC				glUniform3f = NULL;
PFNGLUNIFORM1FARBPROC				glUniform1f = NULL;
PFNGLUNIFORM4FVARBPROC				glUniform4fv = NULL;
PFNGLUNIFORM3FVARBPROC				glUniform3fv = NULL;
PFNGLUNIFORM2FVARBPROC				glUniform2fv = NULL;
PFNGLUNIFORM1FVARBPROC				glUniform1fv = NULL;
PFNGLUNIFORM1IARBPROC				glUniform1i = NULL;
#endif

//------------------------------------------------------------------------------

char *LoadShader (char* fileName) //, char* Shadersource)
{
	FILE	*fp;
	char	*bufP = NULL;
	int 	fSize;
#ifdef _WIN32
	int	f;
#endif
	char 	fn [FILENAME_LEN];

if (!(fileName && *fileName))
	return NULL;	// no fileName

sprintf (fn, "%s%s%s", gameFolders.szShaderDir, *gameFolders.szShaderDir ? "/" : "", fileName);
#ifdef _WIN32
if (0 > (f = _open (fn, _O_RDONLY)))
	return NULL;	// couldn't open file
fSize = _lseek (f, 0, SEEK_END);
_close (f);
if (fSize <= 0)
	return NULL;	// empty file or seek error
#endif

if (!(fp = fopen (fn, "rt")))
	return NULL;	// couldn't open file

#ifndef _WIN32
fseek (fp, 0, SEEK_END);
fSize = ftell (fp);
if (fSize <= 0) {
	fclose (fp);
	return NULL;	// empty file or seek error
	}
#endif

if (!(bufP = new char [fSize + 1])) {
	fclose (fp);
	return NULL;	// out of memory
	}
fSize = (int) fread (bufP, sizeof (char), fSize, fp);
bufP [fSize] = '\0';
fclose (fp);
return bufP;
}

//------------------------------------------------------------------------------

#ifdef __macosx__
#	define	_HANDLE	(uint) handle
#else
#	define	_HANDLE	handle
#endif

void PrintShaderInfoLog (GLhandleARB handle, int bProgram)
{
   GLint nLogLen = 0;
   GLint charsWritten = 0;
   char *infoLog;

#ifdef GL_VERSION_20
if (bProgram) {
	glGetProgramiv (_HANDLE, GL_INFO_LOG_LENGTH, &nLogLen);
	if ((nLogLen > 0) && (infoLog = new char [nLogLen])) {
		glGetProgramInfoLog (_HANDLE, nLogLen, &charsWritten, infoLog);
		if (*infoLog)
			PrintLog ("\n%s\n\n", infoLog);
		delete[] infoLog;
		}
	}
else {
	glGetShaderiv (_HANDLE, GL_INFO_LOG_LENGTH, &nLogLen);
	if ((nLogLen > 0) && (infoLog = new char [nLogLen])) {
		glGetShaderInfoLog (_HANDLE, nLogLen, &charsWritten, infoLog);
		if (*infoLog)
			PrintLog ("\n%s\n\n", infoLog);
		delete[] infoLog;
		}
	}
#else
glGetObjectParameteriv (_HANDLE, GL_OBJECT_INFO_LOG_LENGTH_ARB, &nLogLen);
if ((nLogLen > 0) && (infoLog = new char [nLogLen])) {
	glGetInfoLog (_HANDLE, nLogLen, &charsWritten, infoLog);
	if (*infoLog)
		PrintLog ("\n%s\n\n", infoLog);
	delete[] infoLog;
	}
#endif
}

#undef _HANDLE

//------------------------------------------------------------------------------

const char *progVS [] = {
	"void TexMergeVS ();" \
	"void main (void) {TexMergeVS ();}"
,
	"void LightingVS ();" \
	"void main (void) {LightingVS ();}"
,
	"void LightingVS ();" \
	"void TexMergeVS ();" \
	"void main (void) {TexMergeVS (); LightingVS ();}"
	};

const char *progFS [] = {
	"void TexMergeFS ();" \
	"void main (void) {TexMergeFS ();}"
,
	"void LightingFS ();" \
	"void main (void) {LightingFS ();}"
,
	"void LightingFS ();" \
	"void TexMergeFS ();" \
	"void main (void) {TexMergeFS (); LightingFS ();}"
	};

GLhandleARB mainVS = 0;
GLhandleARB mainFS = 0;

//------------------------------------------------------------------------------

GLhandleARB	genShaderProg = 0;

int CreateShaderProg (GLhandleARB *progP)
{
if (!progP)
	progP = &genShaderProg;
if (*progP)
	return 1;
*progP = glCreateProgramObject ();
if (*progP)
	return 1;
PrintLog ("   Couldn't create shader program CObject\n");
return 0;
}

//------------------------------------------------------------------------------

void DeleteShaderProg (GLhandleARB *progP)
{
#if 0
if (!progP)
	progP = &genShaderProg;
#endif
if (progP && *progP) {
	glDeleteObject (*progP);
	*progP = 0;
	}
}

//------------------------------------------------------------------------------

int CreateShaderFunc (GLhandleARB *progP, GLhandleARB *fsP, GLhandleARB *vsP,
		const char *fsName, const char *vsName, int bFromFile)
{
	GLhandleARB	fs, vs;
	GLint bFragCompiled, bVertCompiled;

if (!gameStates.ogl.bShadersOk)
	return 0;
if (!CreateShaderProg (progP))
	return 0;
if (*fsP) {
	glDeleteObject (*fsP);
	*fsP = 0;
	}
if (*vsP) {
	glDeleteObject (*vsP);
	*vsP = 0;
	}
if (!(vs = glCreateShaderObject (GL_VERTEX_SHADER)))
	return 0;
if (!(fs = glCreateShaderObject (GL_FRAGMENT_SHADER))) {
	glDeleteObject (vs);
	return 0;
	}
#if DBG_SHADERS
if (bFromFile) {
	vsName = LoadShader (vsName);
	fsName = LoadShader (fsName);
	if (!vsName || !fsName)
		return 0;
	}
#endif
glShaderSource (vs, 1, reinterpret_cast<const GLcharARB **> (&vsName), NULL);
glShaderSource (fs, 1, reinterpret_cast<const GLcharARB **> (&fsName), NULL);
#if DBG_SHADERS
if (bFromFile) {
	delete[] vsName;
	delete[] fsName;
	}
#endif
glCompileShader (vs);
glCompileShader (fs);
glGetObjectParameteriv (vs, GL_OBJECT_COMPILE_STATUS_ARB, &bVertCompiled);
glGetObjectParameteriv (fs, GL_OBJECT_COMPILE_STATUS_ARB, &bFragCompiled);
if (!bVertCompiled || !bFragCompiled) {
	if (!bVertCompiled) {
		PrintLog ("   Couldn't compile vertex shader\n   \"%s\"\n", vsName);
		PrintShaderInfoLog (vs, 0);
		}
	if (!bFragCompiled) {
		PrintLog ("   Couldn't compile fragment shader\n   \"%s\"\n", fsName);
		PrintShaderInfoLog (fs, 0);
		}
	return 0;
	}
glAttachObject (*progP, vs);
glAttachObject (*progP, fs);
*fsP = fs;
*vsP = vs;
return 1;
}

//------------------------------------------------------------------------------

int LinkShaderProg (GLhandleARB *progP)
{
	int	i = 0;
	GLint	bLinked;

if (!progP) {
	progP = &genShaderProg;
	if (!*progP)
		return 0;
	if (gameOpts->ogl.bGlTexMerge)
		i |= 1;
	if (gameStates.render.nLightingMethod)
		i |= 2;
	if (!i)
		return 0;
	if (!CreateShaderFunc (progP, &mainFS, &mainVS, progFS [i - 1], progVS [i - 1], 0)) {
		DeleteShaderProg (progP);
		return 0;
		}
	}
glLinkProgram (*progP);
glGetObjectParameteriv (*progP, GL_OBJECT_LINK_STATUS_ARB, &bLinked);
if (bLinked)
	return 1;
PrintLog ("   Couldn't link shader programs\n");
PrintShaderInfoLog (*progP, 1);
DeleteShaderProg (progP);
return 0;
}

//------------------------------------------------------------------------------

void InitShaders (void)
{
	GLint	nTMUs;

PrintLog ("initializing shader programs\n");
glGetIntegerv (GL_MAX_TEXTURE_UNITS, &nTMUs);
if (gameStates.ogl.bShadersOk) {
	gameStates.ogl.bShadersOk = (nTMUs > 1);
	if (!gameStates.ogl.bShadersOk)
		PrintLog ("GPU has too few texture units (%d)\n", nTMUs);
	}
if (gameStates.render.color.bLightmapsOk)
	gameStates.render.color.bLightmapsOk = (nTMUs > 2);
PrintLog ("   initializing texture merging shader programs\n");
InitTexMergeShaders ();
gameData.render.ogl.nHeadlights = 0;
PrintLog ("   initializing lighting shader programs\n");
InitHeadlightShaders (1);
PrintLog ("   initializing vertex lighting shader programs\n");
gpgpuLighting.InitShader ();
PrintLog ("   initializing glare shader programs\n");
InitGlareShader ();
PrintLog ("   initializing gray scale shader programs\n");
InitGrayScaleShader ();
ResetPerPixelLightingShaders ();
InitPerPixelLightingShaders ();
ResetLightmapShaders ();
InitLightmapShaders ();
LinkShaderProg (NULL);
}

//------------------------------------------------------------------------------

void OglInitShaders (void)
{
PrintLog ("Checking shaders ...\n");
if (!gameOpts->render.bUseShaders)
	PrintLog ("   Shaders have been disabled in d2x.ini\n");
else if (!gameStates.ogl.bMultiTexturingOk)
	PrintLog ("   Multi-texturing not supported by the OpenGL driver\n");
else if (!pszOglExtensions)
	PrintLog ("   Required Extensions not supported by the OpenGL driver\n");
else if (!strstr (pszOglExtensions, "GL_ARB_shading_language_100"))
	PrintLog ("   Shading language not supported by the OpenGL driver\n");
else if (!strstr (pszOglExtensions, "GL_ARB_shader_objects"))
	PrintLog ("   Shader objects not supported by the OpenGL driver\n");
else {
#ifndef GL_VERSION_20
	if (!(glCreateProgramObject = (PFNGLCREATEPROGRAMOBJECTARBPROC) wglGetProcAddress ("   glCreateProgramObjectARB")))
		PrintLog ("   glCreateProgramObject not supported by the OpenGL driver\n");
	if (!(glDeleteObject = (PFNGLDELETEOBJECTARBPROC) wglGetProcAddress ("   glDeleteObjectARB")))
		PrintLog ("   glDeleteObject not supported by the OpenGL driver\n");
	if (!(glUseProgramObject = (PFNGLUSEPROGRAMOBJECTARBPROC) wglGetProcAddress ("   glUseProgramObjectARB")))
		PrintLog ("   glUseProgramObject not supported by the OpenGL driver\n");
	if (!(glCreateShaderObject = (PFNGLCREATESHADEROBJECTARBPROC) wglGetProcAddress ("   glCreateShaderObjectARB")))
		PrintLog ("   glCreateShaderObject not supported by the OpenGL driver\n");
	if (!(glShaderSource = (PFNGLSHADERSOURCEARBPROC) wglGetProcAddress ("   glShaderSourceARB")))
		PrintLog ("   glShaderSource not supported by the OpenGL driver\n");
	if (!(glCompileShader = (PFNGLCOMPILESHADERARBPROC) wglGetProcAddress ("   glCompileShaderARB")))
		PrintLog ("   glCompileShader not supported by the OpenGL driver\n");
	if (!(glGetObjectParameteriv = (PFNGLGETOBJECTPARAMETERIVARBPROC) wglGetProcAddress ("   glGetObjectParameterivARB")))
		PrintLog ("   glGetObjectParameteriv not supported by the OpenGL driver\n");
	if (!(glAttachObject = (PFNGLATTACHOBJECTARBPROC) wglGetProcAddress ("   glAttachObjectARB")))
		PrintLog ("   glAttachObject not supported by the OpenGL driver\n");
	if (!(glGetInfoLog = (PFNGLGETINFOLOGARBPROC) wglGetProcAddress ("   glGetInfoLogARB")))
		PrintLog ("   glGetInfoLog not supported by the OpenGL driver\n");
	if (!(glLinkProgram = (PFNGLLINKPROGRAMARBPROC) wglGetProcAddress ("   glLinkProgramARB")))
		PrintLog ("   glLinkProgram not supported by the OpenGL driver\n");
	if (!(glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONARBPROC) wglGetProcAddress ("   glGetUniformLocationARB")))
		PrintLog ("   glGetUniformLocation not supported by the OpenGL driver\n");
	if (!(glUniform4f = (PFNGLUNIFORM4FARBPROC) wglGetProcAddress ("   glUniform4fARB")))
		PrintLog ("   glUniform4f not supported by the OpenGL driver\n");
	if (!(glUniform3f = (PFNGLUNIFORM3FARBPROC) wglGetProcAddress ("   glUniform3fARB")))
		PrintLog ("   glUniform3f not supported by the OpenGL driver\n");
	if (!(glUniform1f = (PFNGLUNIFORM1FARBPROC) wglGetProcAddress ("   glUniform1fARB")))
		PrintLog ("   glUniform1f not supported by the OpenGL driver\n");
	if (!(glUniform4fv = (PFNGLUNIFORM4FVARBPROC) wglGetProcAddress ("   glUniform4fvARB")))
		PrintLog ("   glUniform4fv not supported by the OpenGL driver\n");
	if (!(glUniform3fv = (PFNGLUNIFORM3FVARBPROC) wglGetProcAddress ("   glUniform3fvARB")))
		PrintLog ("   glUniform3fv not supported by the OpenGL driver\n");
	if (!(glUniform2fv = (PFNGLUNIFORM3FVARBPROC) wglGetProcAddress ("   glUniform2fvARB")))
		PrintLog ("   glUniform2fv not supported by the OpenGL driver\n");
	if (!(glUniform1fv = (PFNGLUNIFORM1FVARBPROC) wglGetProcAddress ("   glUniform1fvARB")))
		PrintLog ("   glUniform1fv not supported by the OpenGL driver\n");
	if (!(glUniform1i = (PFNGLUNIFORM1IARBPROC) wglGetProcAddress ("   glUniform1iARB")))
		PrintLog ("   glUniform1i not supported by the OpenGL driver\n");
	else
		gameStates.ogl.bShadersOk = 1;
#else
	gameStates.ogl.bShadersOk = 1;
#endif
	}
PrintLog (gameStates.ogl.bShadersOk ? "Shaders are available\n" : "No shaders available\n");
}

//------------------------------------------------------------------------------
//eof
