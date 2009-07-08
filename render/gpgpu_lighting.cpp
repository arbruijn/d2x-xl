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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "automap.h"
#include "texmerge.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_fastrender.h"
#include "glare.h"
#include "rendermine.h"
#include "renderthreads.h"
#include "gpgpu_lighting.h"

CGPGPULighting gpgpuLighting;

//------------------------------------------------------------------------------

void CGPGPULighting::Init (void)
{
m_hShaderProg = 0;
m_hVertShader = 0; 
m_hFragShader = 0; 
}

//------------------------------------------------------------------------------

void CGPGPULighting::Begin (void)
{
if (ogl.m_states.bVertexLighting)
	ogl.m_states.bVertexLighting = lightManager.FBO ().Create (GPGPU_LIGHT_BUF_WIDTH, GPGPU_LIGHT_BUF_WIDTH, 2);
}

//------------------------------------------------------------------------------

void CGPGPULighting::End (void)
{
if (ogl.m_states.bVertexLighting) {
	PrintLog ("unloading dynamic lighting buffers\n");
	lightManager.FBO ().Destroy ();
	}
}

//------------------------------------------------------------------------------

#if GPGPU_VERTEX_LIGHTING
static char	*szTexNames [GPGPU_LIGHT_BUFFERS] = {"vertPosTex", "vertNormTex", "lightPosTex", "lightColorTex"};
#endif

GLuint CGPGPULighting::CreateBuffer (int i)
{
	GLuint	hBuffer;

//create render texture
ogl.GenTextures (1, &hBuffer);
if (!hBuffer)
	return 0;
glActiveTexture (GL_TEXTURE0 + i);
glClientActiveTexture (GL_TEXTURE0 + i);
glEnable (GL_TEXTURE_2D);
glBindTexture (GL_TEXTURE_2D, hBuffer);
// set up texture parameters, turn off filtering
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
//glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 
// define texture with same size as viewport
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, GPGPU_LIGHT_BUF_WIDTH, GPGPU_LIGHT_BUF_WIDTH, 0, GL_RGBA, GL_FLOAT, m_vld.buffers [i]);
return hBuffer;
}

//------------------------------------------------------------------------------

void CGPGPULighting::ComputeFragLight (float lightRange)
{
	CFloatVector	*vertPos, *vertNorm, *lightPos, lightColor, lightDir;
	CFloatVector	vReflect, vertColor = CFloatVector::ZERO;
	float		nType, radius, brightness, specular;
	float		attenuation, lightDist, NdotL, RdotE;
	int		i;

	static CFloatVector matAmbient = CFloatVector::Create(0.01f, 0.01f, 0.01f, 1.0f);
	//static CFloatVector matDiffuse = {{1.0f, 1.0f, 1.0f, 1.0f}};
	//static CFloatVector matSpecular = {{1.0f, 1.0f, 1.0f, 1.0f}};
	static float shininess = 96.0;

for (i = 0; i < m_vld.nLights; i++) {
	vertPos = m_vld.buffers [0] + i;
	vertNorm = m_vld.buffers [1] + i;
	lightPos = m_vld.buffers [2] + i;
	lightColor = m_vld.buffers [3][i];
	nType = (*vertNorm) [W];
	radius = (*lightPos) [W];
	brightness = lightColor [W];
	lightDir = *lightPos - *vertPos;
	lightDist = lightDir.Mag() / lightRange;
	CFloatVector::Normalize (lightDir);
	if (nType)
		lightDist -= radius;
	if (lightDist < 1.0f) {
		lightDist = 1.4142f;
		NdotL = 1.0f;
		}
	else {
		lightDist *= lightDist;
		if (nType)
			lightDist *= 2.0f;
		NdotL = CFloatVector::Dot (*vertNorm, lightDir);
		if (NdotL < 0.0f)
			NdotL = 0.0f;
		}	
	attenuation = lightDist / brightness;
	vertColor [R] = (matAmbient [R] + NdotL) * lightColor [R];
	vertColor [G] = (matAmbient [G] + NdotL) * lightColor [G];
	vertColor [B] = (matAmbient [B] + NdotL) * lightColor [B];
	if (NdotL > 0.0f) {
		vReflect = CFloatVector::Reflect (lightDir.Neg (), *vertNorm);
		CFloatVector::Normalize (vReflect);
		lightPos->Neg ();
		CFloatVector::Normalize (*lightPos);
		RdotE = CFloatVector::Dot (vReflect, *lightPos);
		if (RdotE < 0.0f)
			RdotE = 0.0f;
		specular = (float) pow (RdotE, shininess);
		vertColor [R] += lightColor [R] * specular;
		vertColor [G] += lightColor [G] * specular;
		vertColor [B] += lightColor [B] * specular;
		}	
	m_vld.colors [i][R] = vertColor [R] / attenuation;
	m_vld.colors [i][G] = vertColor [G] / attenuation;
	m_vld.colors [i][B] = vertColor [B] / attenuation;
	}
}

//------------------------------------------------------------------------------

int CGPGPULighting::Render (void)
{
	tFaceColor	*vertColorP;
	tRgbaColorf	vertColor;
	CFloatVector		*pc;
	int			i, j;
	short			nVertex, nLights;
	GLuint		hBuffer [GPGPU_LIGHT_BUFFERS] = {0,0,0,0};

#if !GPGPU_LIGHT_DRAWARRAYS
	static float	quadCoord [4][2] = {{0, 0}, {0, GPGPU_LIGHT_BUF_WIDTH}, {GPGPU_LIGHT_BUF_WIDTH, GPGPU_LIGHT_BUF_WIDTH}, {GPGPU_LIGHT_BUF_WIDTH, 0}};
	static float	texCoord [4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
#endif

#if 0
ComputeFragLight (ogl.m_states.fLightRange);
#else
if (!m_vld.nLights)
	return 1;
for (i = 0; i < VL_SHADER_BUFFERS; i++) {
	if (!(hBuffer [i] = CreateBuffer (i)))
		return 0;
#if 0 //DBG
	memset (m_vld.buffers [i] + m_vld.nLights, 0, (GPGPU_LIGHT_BUF_SIZE - m_vld.nLights) * sizeof (m_vld.buffers [i][0]));
#endif
	}
#if GPGPU_LIGHT_DRAWARRAYS
glDrawArrays (GL_QUADS, 0, 4);
#else
glBegin (GL_QUADS);
for (i = 0; i < 4; i++) {
	for (j = 0; j < VL_SHADER_BUFFERS; j++)
		glMultiTexCoord2fv (GL_TEXTURE0 + j, texCoord [i]);
	glVertex2fv (quadCoord [i]);
	}
glEnd ();
#endif
#if DBG
memset (m_vld.colors, 0, sizeof (m_vld.colors));
#endif
#if 0
for (i = 0; i < VL_SHADER_BUFFERS; i++) {
	glActiveTexture (GL_TEXTURE0 + i);
	glClientActiveTexture (GL_TEXTURE0 + i);
	glBindTexture (GL_TEXTURE_2D, 0);
	}
#endif
ogl.DeleteTextures (VL_SHADER_BUFFERS, hBuffer);
memset (hBuffer, 0, sizeof (hBuffer));
ogl.SetReadBuffer (GL_COLOR_ATTACHMENT0_EXT, 1);
glReadPixels (0, 0, GPGPU_LIGHT_BUF_WIDTH, GPGPU_LIGHT_BUF_WIDTH, GL_RGBA, GL_FLOAT, m_vld.colors);
#endif

for (i = 0, pc = m_vld.colors; i < m_vld.nVertices; i++) {
	nVertex = m_vld.index [i].nVertex;
#if DBG
	if (nVertex == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	vertColor = m_vld.index [i].color;
	vertColor.red += gameData.render.color.ambient [nVertex].color.red;
	vertColor.green += gameData.render.color.ambient [nVertex].color.green;
	vertColor.blue += gameData.render.color.ambient [nVertex].color.blue;
	if (gameOpts->render.color.nSaturation == 2) {
		for (j = 0, nLights = m_vld.index [i].nLights; j < nLights; j++, pc++) {
			if (vertColor.red < (*pc) [R])
				vertColor.red = (*pc) [R];
			if (vertColor.green < (*pc) [G])
				vertColor.green = (*pc) [G];
			if (vertColor.blue < (*pc) [B])
				vertColor.blue = (*pc) [B];
			}
		}
	else {
		for (j = 0, nLights = m_vld.index [i].nLights; j < nLights; j++, pc++) {
			vertColor.red += (*pc) [R];
			vertColor.green += (*pc) [G];
			vertColor.blue += (*pc) [B];
			}
		if (gameOpts->render.color.nSaturation) {	//if a color component is > 1, cap color components using highest component value
			float	cMax = vertColor.red;
			if (cMax < vertColor.green)
				cMax = vertColor.green;
			if (cMax < vertColor.blue)
				cMax = vertColor.blue;
			if (cMax > 1) {
				vertColor.red /= cMax;
				vertColor.green /= cMax;
				vertColor.blue /= cMax;
				}
			}
		}
	vertColorP = gameData.render.color.vertices + nVertex;
	vertColorP->color = vertColor;
	vertColorP->index = gameStates.render.nFrameFlipFlop + 1;
	}
m_vld.nVertices = 0;
m_vld.nLights = 0;
return 1;
}

//------------------------------------------------------------------------------

int CGPGPULighting::Compute (short nVertex, int nState, tFaceColor *colorP)
{
	int	nLights, h, i, j;

	static float		quadCoord [4][2] = {{0, 0}, {0, GPGPU_LIGHT_BUF_WIDTH}, {GPGPU_LIGHT_BUF_WIDTH, GPGPU_LIGHT_BUF_WIDTH}, {GPGPU_LIGHT_BUF_WIDTH, 0}};
	static float		texCoord [4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
	static const char	*szTexNames [GPGPU_LIGHT_BUFFERS] = {"vertPosTex", "vertNormTex", "lightPosTex", "lightColorTex"};
#if 0
	static CFloatVector3	matSpecular = {{1.0f, 1.0f, 1.0f}};
#endif

if (!ogl.m_states.bVertexLighting)
	return 0;
if (nState == 0) {
	glMatrixMode (GL_PROJECTION);    
	glPushMatrix ();
	glLoadIdentity ();
	glOrtho (0.0, GPGPU_LIGHT_BUF_WIDTH, 0.0, GPGPU_LIGHT_BUF_WIDTH, -1.0, 1.0);
	glMatrixMode (GL_MODELVIEW);                         
	glPushMatrix ();
	glLoadIdentity ();
	glPushAttrib (GL_VIEWPORT_BIT);
   glViewport (0, 0, GPGPU_LIGHT_BUF_WIDTH, GPGPU_LIGHT_BUF_WIDTH);
	lightManager.FBO ().Enable ();
#if 1
	glUseProgramObject (m_hShaderProg);
	for (i = 0; i < VL_SHADER_BUFFERS; i++) {
		glUniform1i (glGetUniformLocation (m_hShaderProg, szTexNames [i]), i);
		if ((j = glGetError ())) {
			Compute (-1, 2, NULL);
			return 0;
			}
		}
	glUniform1f (glGetUniformLocation (m_hShaderProg, "lightRange"), ogl.m_states.fLightRange);
#endif
#if 0
	glUniform1f (glGetUniformLocation (m_hShaderProg, "shininess"), 64.0f);
	glUniform3fv (glGetUniformLocation (m_hShaderProg, "matAmbient"), 1, reinterpret_cast<GLfloat*> (&gameData.render.vertColor.matAmbient));
	glUniform3fv (glGetUniformLocation (m_hShaderProg, "matDiffuse"), 1, reinterpret_cast<GLfloat*> (&gameData.render.vertColor.matDiffuse));
	glUniform3fv (glGetUniformLocation (m_hShaderProg, "matSpecular"), 1, reinterpret_cast<GLfloat*> (&matSpecular));
#endif
	ogl.SetDrawBuffer (GL_COLOR_ATTACHMENT0_EXT, 0); 
	ogl.SetReadBuffer (GL_COLOR_ATTACHMENT0_EXT, 0);
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);
	glDisable (GL_ALPHA_TEST);
	glDisable (GL_DEPTH_TEST);
	glColor3f (0,0,0);
#if GPGPU_LIGHT_DRAWARRAYS
	for (i = 0; i < VL_SHADER_BUFFERS; i++) {
		ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0 + i);
		glTexCoordPointer (2, GL_FLOAT, 0, texCoord);
		glVertexPointer (2, GL_FLOAT, 0, quadCoord);
		}
#endif
	if ((j = glGetError ())) {
		Compute (-1, 2, NULL);
		return 0;
		}
	glDepthMask (0);
	}
else if (nState == 1) {
	CDynLight*		psl;
	int				bSkipHeadlight = ogl.m_states.bHeadlight && (lightManager.Headlights ().nLights > 0) && !gameStates.render.nState;
	CFloatVector	vPos = gameData.segs.fVertices [nVertex],
						vNormal = gameData.segs.points [nVertex].p3_normal.vNormal;
		
	lightManager.SetNearestToVertex (-1, nVertex, NULL, 1, 0, 1, 0);
	if (!(h = lightManager.Index (0) [0].nActive))
		return 1;
	if (h > GPGPU_LIGHT_BUF_SIZE)
		h = GPGPU_LIGHT_BUF_SIZE;
	if (m_vld.nVertices >= GPGPU_LIGHT_BUF_SIZE)
		Render ();
	else if (m_vld.nLights + h > GPGPU_LIGHT_BUF_SIZE)
		Render ();
#if DBG
	if (nVertex == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	for (nLights = 0, i = m_vld.nLights, j = 0; j < h; i++, j++) {
		psl = lightManager.Active (0) [j].pl;
		if (bSkipHeadlight && (psl->info.nType == 3))
			continue;
		m_vld.buffers [0][i] = vPos;
		m_vld.buffers [1][i] = vNormal;
		m_vld.buffers [2][i] = psl->render.vPosf [0];
		m_vld.buffers [3][i] = *(reinterpret_cast<CFloatVector*> (&psl->info.color));
		m_vld.buffers [0][i][W] = 1.0f;
		m_vld.buffers [1][i][W] = (psl->info.nType < 2) ? 1.0f : 0.0f;
		m_vld.buffers [2][i][W] = psl->info.fRad;
		m_vld.buffers [3][i][W] = psl->info.fBrightness;
		nLights++;
		}
	if (nLights) {
		m_vld.index [m_vld.nVertices].nVertex = nVertex;
		m_vld.index [m_vld.nVertices].nLights = nLights;
		m_vld.index [m_vld.nVertices].color = colorP->color;
		m_vld.nVertices++;
		m_vld.nLights += nLights;
		}
	//lightManager.Index (0) [0].nActive = gameData.render.lights.dynamic.shader.iVertexLights [0];
	}	
else if (nState == 2) {
	Render ();
	glUseProgramObject (0);
	lightManager.FBO ().Disable ();
	glMatrixMode (GL_PROJECTION);    
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);                         
	glPopMatrix ();
	glPopAttrib ();
	ogl.SetDrawBuffer (GL_BACK, 1);
	for (i = 0; i < VL_SHADER_BUFFERS; i++) {
		ogl.DisableClientStates (1, 0, 0, GL_TEXTURE0 + i);
		glActiveTexture (GL_TEXTURE0 + i);
		glBindTexture (GL_TEXTURE_2D, 0);
		}
	glDepthMask (1);
	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LESS);
	glEnable (GL_ALPHA_TEST);
	glAlphaFunc (GL_GEQUAL, (float) 0.01);	
	ogl.SetDrawBuffer (GL_BACK, 1);
	}
return 1;
}

//------------------------------------------------------------------------------

const char *gpgpuLightFS = 
	"uniform sampler2D vertPosTex, vertNormTex, lightPosTex, lightColorTex;\r\n" \
	"uniform float lightRange;\r\n" \
	"void main (void) {\r\n" \
	"	vec3 vertPos = texture2D (vertPosTex, gl_TexCoord [0].xy).xyz;\r\n" \
	"	vec3 vertNorm = texture2D (vertNormTex, gl_TexCoord [0].xy).xyz;\r\n" \
	"	vec3 lightPos = texture2D (lightPosTex, gl_TexCoord [0].xy).xyz;\r\n" \
	"	vec3 lightColor = texture2D (lightColorTex, gl_TexCoord [0].xy).xyz;\r\n" \
	"	vec3 lightDir = lightPos - vertPos;\r\n" \
	"	float type = texture2D (vertNormTex, gl_TexCoord [0].xy).a;\r\n" \
	"	float radius = texture2D (lightPosTex, gl_TexCoord [0].xy).a;\r\n" \
	"	float brightness = texture2D (lightColorTex, gl_TexCoord [0].xy).a;\r\n" \
	"	float attenuation, lightDist, NdotL, RdotE;\r\n" \
	"  vec3 matAmbient = vec3 (0.01, 0.01, 0.01);\r\n" \
	"  float shininess = 64.0;\r\n" \
	"  vec3 vertColor = vec3 (0.0, 0.0, 0.0);\r\n" \
	"	lightDist = length (lightDir) / lightRange - type * radius;\r\n" \
	"	lightDir = Normalize (lightDir);\r\n" \
	"	if (lightDist < 1.0) {\r\n" \
	"		lightDist = 1.4142;\r\n" \
	"		NdotL = 1.0;\r\n" \
	"		}\r\n" \
	"	else {\r\n" \
	"		lightDist *= lightDist * (type + 1.0);\r\n" \
	"		NdotL = max (dot (vertNorm, lightDir), 0.0);\r\n" \
	"		}\r\n" \
	"	attenuation = lightDist / brightness;\r\n" \
	"	vertColor = (matAmbient + vec3 (NdotL, NdotL, NdotL)) * lightColor;\r\n" \
	"	if (NdotL > 0.0) {\r\n" \
	"		RdotE = max (dot (Normalize (Reflect (-lightDir, vertNorm)), Normalize (-lightPos)), 0.0);\r\n" \
	"		vertColor += lightColor * pow (RdotE, shininess);\r\n" \
	"		}\r\n" \
	"  gl_FragColor = vec4 (vertColor / attenuation, 1.0);\r\n" \
	"	}";

const char *vertLightVS = 
	"void main(void){" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;"\
	"/*gl_TexCoord [1] = gl_MultiTexCoord1;"\
	"gl_TexCoord [2] = gl_MultiTexCoord2;"\
	"gl_TexCoord [3] = gl_MultiTexCoord3;*/"\
	"gl_Position = ftransform() /*gl_ModelViewProjectionMatrix * gl_Vertex*/;" \
	"gl_FrontColor = gl_Color;}";

//-------------------------------------------------------------------------

void CGPGPULighting::InitShader (void)
{
#if GPGPU_VERTEX_LIGHTING
	int	bOk;
#endif

if (!ogl.m_states.bVertexLighting)
	return;
ogl.m_states.bVertexLighting = 0;
#if GPGPU_VERTEX_LIGHTING
if (ogl.m_states.bRender2TextureOk && ogl.m_states.bShadersOk) {
	PrintLog ("building vertex lighting shader program\n");
	ogl.m_states.bVertexLighting = 1;
	if (m_hShaderProg)
		DeleteShaderProg (&m_hShaderProg);
	bOk = CreateShaderProg (&m_hShaderProg) &&
			CreateShaderFunc (&m_hShaderProg, &m_hFragShader, &m_hVertShader, gpgpuLightFS, vertLightVS, 1) &&
			LinkShaderProg (&m_hShaderProg);
	if (!bOk) {
		ogl.m_states.bVertexLighting = 0;
		DeleteShaderProg (&m_hShaderProg);
		}
	}
#endif
}

//------------------------------------------------------------------------------
// eof
