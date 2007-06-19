#ifndef __lightmap_h
#define __lightmap_h

#include "ogl_init.h"

typedef struct tLightMap {
	vmsVector	pos;
	GLfloat		color[4];
	//float		bright;
	double		range;
	vmsVector  dir;  //currently based on face normals
	int			refside;  //(seg*6)+tSide ie which tSide the light is on
} tLightMap;

#ifndef GL_VERSION_20
#		define	glCreateShaderObject			pglCreateShaderObjectARB
#		define	glShaderSource					pglShaderSourceARB
#		define	glCompileShader				pglCompileShaderARB
#		define	glCreateProgramObject		pglCreateProgramObjectARB
#		define	glAttachObject					pglAttachObjectARB
#		define	glLinkProgram					pglLinkProgramARB
#		define	glUseProgramObject			pglUseProgramObjectARB
#		define	glDeleteObject					pglDeleteObjectARB
#		define	glGetObjectParameteriv		pglGetObjectParameterivARB
#		define	glGetInfoLog					pglGetInfoLogARB
#		define	glGetUniformLocation			pglGetUniformLocationARB
#		define	glUniform4f						pglUniform4fARB
#		define	glUniform3f						pglUniform3fARB
#		define	glUniform1f						pglUniform1fARB
#		define	glUniform1i						pglUniform1iARB
#		define	glUniform4fv					pglUniform4fvARB
#		define	glUniform3fv					pglUniform3fvARB
#		define	glUniform1fv					pglUniform1fvARB
#		define	glUniform1i						pglUniform1iARB

extern PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObject;
extern PFNGLSHADERSOURCEARBPROC				glShaderSource;
extern PFNGLCOMPILESHADERARBPROC				glCompileShader;
extern PFNGLCREATEPROGRAMOBJECTARBPROC		glCreateProgramObject;
extern PFNGLATTACHOBJECTARBPROC				glAttachObject;
extern PFNGLLINKPROGRAMARBPROC				glLinkProgram;
extern PFNGLUSEPROGRAMOBJECTARBPROC			glUseProgramObject;
extern PFNGLDELETEOBJECTARBPROC				glDeleteObject;
extern PFNGLGETOBJECTPARAMETERIVARBPROC	glGetObjectParameteriv;
extern PFNGLGETINFOLOGARBPROC					glGetInfoLog;
extern PFNGLGETUNIFORMLOCATIONARBPROC		glGetUniformLocation;
extern PFNGLUNIFORM4FARBPROC					glUniform4f;
extern PFNGLUNIFORM3FARBPROC					glUniform3f;
extern PFNGLUNIFORM1FARBPROC					glUniform1f;
extern PFNGLUNIFORM1IARBPROC					glUniform1i;
extern PFNGLUNIFORM4FVARBPROC					glUniform4fv;
extern PFNGLUNIFORM3FVARBPROC					glUniform3fv;
extern PFNGLUNIFORM1FVARBPROC					glUniform1fv;
#else
#  ifdef __macosx__
#    define glCreateShaderObject   glCreateShaderObjectARB
#    define glShaderSource         glShaderSourceARB
#    define glCompileShader        glCompileShaderARB
#    define glCreateProgramObject  glCreateProgramObjectARB
#    define glAttachObject         glAttachObjectARB
#    define glLinkProgram          glLinkProgramARB
#    define glUseProgramObject     glUseProgramObjectARB
#    define glDeleteObject         glDeleteObjectARB
#    define glGetObjectParameteriv glGetObjectParameterivARB
#    define glGetInfoLog           glGetInfoLogARB
#    define glGetUniformLocation   glGetUniformLocationARB
#    define glUniform4f            glUniform4fARB
#    define glUniform3f            glUniform3fARB
#    define glUniform1f            glUniform1fARB
#    define glUniform1i            glUniform1iARB
#    define glUniform4fv           glUniform4fvARB
#    define glUniform3fv           glUniform3fvARB
#    define glUniform1fv           glUniform1fvARB
#  endif
#endif

extern int				numLightMaps;
extern tOglTexture	*lightMaps;  //Level Lightmaps - currently hardset to 5400, probably need to change this to a variable number
extern tLightMap		*lightData;  //Level lights
extern GLhandleARB	lmShaderProgs [3];

void InitLightmapShaders (void);
void CreateLightMaps (void);
void DestroyLightMaps (void);
int HaveLightMaps (void);
int IsLight (int tMapNum);
int GetLightColor (int tMapNum, tLightMap *pTempLight);

#define	USE_LIGHTMAPS \
			(gameStates.render.color.bLightMapsOk && \
			 gameOpts->render.color.bUseLightMaps && \
			 !IsMultiGame && \
			 !gameOpts->render.bDynLighting)

#endif //__lightmap_h
