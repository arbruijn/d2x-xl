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
#include "ogl_shader.h"
#include "ogl_fastrender.h"
#include "glare.h"
#include "rendermine.h"
#include "renderthreads.h"
#include "gpgpu_lighting.h"

CGPGPULighting gpgpuLighting;

//------------------------------------------------------------------------------

void CGPGPULighting::Init (void)
{
m_nShaderProg = -1;
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
	PrintLog (1, "unloading dynamic lighting buffers\n");
	lightManager.FBO ().Destroy ();
	PrintLog (-1);
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
ogl.SelectTMU (GL_TEXTURE0 + i, true);
ogl.SetTexturing (true);
ogl.BindTexture (hBuffer);
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
	nType = vertNorm->v.coord.w;
	radius = lightPos->v.coord.w;
	brightness = lightColor.v.coord.w;
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
	vertColor.Red () = (matAmbient.Red () + NdotL) * lightColor.Red ();
	vertColor.Green () = (matAmbient.Green () + NdotL) * lightColor.Green ();
	vertColor.Blue () = (matAmbient.Blue () + NdotL) * lightColor.Blue ();
	if (NdotL > 0.0f) {
		vReflect = CFloatVector::Reflect (lightDir.Neg (), *vertNorm);
		CFloatVector::Normalize (vReflect);
		lightPos->Neg ();
		CFloatVector::Normalize (*lightPos);
		RdotE = CFloatVector::Dot (vReflect, *lightPos);
		if (RdotE < 0.0f)
			RdotE = 0.0f;
		specular = (float) pow (RdotE, shininess);
		vertColor.Red () += lightColor.Red () * specular;
		vertColor.Green () += lightColor.Green () * specular;
		vertColor.Blue () += lightColor.Blue () * specular;
		}	
	m_vld.colors [i].Red () = vertColor.Red () / attenuation;
	m_vld.colors [i].Green () = vertColor.Green () / attenuation;
	m_vld.colors [i].Blue () = vertColor.Blue () / attenuation;
	}
}

//------------------------------------------------------------------------------

int CGPGPULighting::Render (void)
{
	CFaceColor*		vertColorP;
	CFloatVector	vertColor;
	CFloatVector*	colorP;
	int				i, j;
	short				nVertex, nLights;
	GLuint			hBuffer [GPGPU_LIGHT_BUFFERS] = {0,0,0,0};

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
OglDrawArrays (GL_QUADS, 0, 4);
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
	ogl.SelectTMU (GL_TEXTURE0 + i, true);
	ogl.BindTexture (0);
	}
#endif
ogl.DeleteTextures (VL_SHADER_BUFFERS, hBuffer);
memset (hBuffer, 0, sizeof (hBuffer));
ogl.SetReadBuffer (GL_COLOR_ATTACHMENT0_EXT, 1);
glReadPixels (0, 0, GPGPU_LIGHT_BUF_WIDTH, GPGPU_LIGHT_BUF_WIDTH, GL_RGBA, GL_FLOAT, m_vld.colors);
#endif

for (i = 0, colorP = m_vld.colors; i < m_vld.nVertices; i++) {
	nVertex = m_vld.index [i].nVertex;
#if DBG
	if (nVertex == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	vertColor = m_vld.index [i].color;
	vertColor += gameData.render.color.ambient [nVertex];
	if (gameOpts->render.color.nSaturation == 2) {
		for (j = 0, nLights = m_vld.index [i].nLights; j < nLights; j++, colorP++) {
			if (vertColor.Red () < colorP->Red ())
				vertColor.Red () = colorP->Red ();
			if (vertColor.Green () < colorP->Green ())
				vertColor.Green () = colorP->Green ();
			if (vertColor.Blue () < colorP->Blue ())
				vertColor.Blue () = colorP->Blue ();
			}
		}
	else {
		for (j = 0, nLights = m_vld.index [i].nLights; j < nLights; j++, colorP++) {
			vertColor += *colorP;
			}
		if (gameOpts->render.color.nSaturation) {	//if a color component is > 1, cap color components using highest component value
			float	cMax = vertColor.Red ();
			if (cMax < vertColor.Green ())
				cMax = vertColor.Green ();
			if (cMax < vertColor.Blue ())
				cMax = vertColor.Blue ();
			if (cMax > 1)
				vertColor /= cMax;
			}
		}
	vertColorP = gameData.render.color.vertices + nVertex;
	(CFloatVector) *vertColorP = vertColor;
	vertColorP->index = gameStates.render.nFrameFlipFlop + 1;
	}
m_vld.nVertices = 0;
m_vld.nLights = 0;
return 1;
}

//------------------------------------------------------------------------------

int CGPGPULighting::Compute (short nVertex, int nState, CFaceColor *colorP)
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
	GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy (m_nShaderProg));
	shaderManager.Rebuild (shaderProg);
	if (!shaderProg)
		return 0;
	for (i = 0; i < VL_SHADER_BUFFERS; i++) {
		glUniform1i (glGetUniformLocation (shaderProg, szTexNames [i]), i);
		if ((j = glGetError ())) {
			Compute (-1, 2, NULL);
			return 0;
			}
		}
	glUniform1f (glGetUniformLocation (shaderProg, "lightRange"), ogl.m_states.fLightRange);
#endif
#if 0
	glUniform1f (glGetUniformLocation (shaderProg, "shininess"), 64.0f);
	glUniform3fv (glGetUniformLocation (shaderProg, "matAmbient"), 1, reinterpret_cast<GLfloat*> (&gameData.render.vertColor.matAmbient));
	glUniform3fv (glGetUniformLocation (shaderProg, "matDiffuse"), 1, reinterpret_cast<GLfloat*> (&gameData.render.vertColor.matDiffuse));
	glUniform3fv (glGetUniformLocation (shaderProg, "matSpecular"), 1, reinterpret_cast<GLfloat*> (&matSpecular));
#endif
	ogl.SetDrawBuffer (GL_COLOR_ATTACHMENT0_EXT, 0); 
	ogl.SetReadBuffer (GL_COLOR_ATTACHMENT0_EXT, 0);
	ogl.SetFaceCulling (false);
	ogl.SetBlending (false);
	ogl.SetAlphaTest (false);
	ogl.SetDepthTest (false);
	glColor3f (0,0,0);
#if GPGPU_LIGHT_DRAWARRAYS
	for (i = 0; i < VL_SHADER_BUFFERS; i++) {
		ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0 + i);
		OglTexCoordPointer (2, GL_FLOAT, 0, texCoord);
		OglVertexPointer (2, GL_FLOAT, 0, quadCoord);
		}
#endif
	if ((j = glGetError ())) {
		Compute (-1, 2, NULL);
		return 0;
		}
	ogl.SetDepthWrite (false);
	}
else if (nState == 1) {
	CDynLight*		lightP;
	int				bSkipHeadlight = ogl.m_states.bHeadlight && (lightManager.Headlights ().nLights > 0) && !gameStates.render.nState;
	CFloatVector	vPos = gameData.segs.fVertices [nVertex],
						vNormal = *gameData.segs.points [nVertex].GetNormal ();
		
	lightManager.SetNearestToVertex (-1, -1, nVertex, NULL, 1, 0, 1, 0);
	if (!(h = lightManager.Index (0,0).nActive))
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
		lightP = lightManager.Active (0) [j].lightP;
		if (bSkipHeadlight && (lightP->info.nType == 3))
			continue;
		m_vld.buffers [0][i] = vPos;
		m_vld.buffers [1][i] = vNormal;
		m_vld.buffers [2][i] = lightP->render.vPosf [0];
		m_vld.buffers [3][i] = *((CFloatVector*) &lightP->info.color);
		m_vld.buffers [0][i].v.coord.w = 1.0f;
		m_vld.buffers [1][i].v.coord.w = (lightP->info.nType < 2) ? 1.0f : 0.0f;
		m_vld.buffers [2][i].v.coord.w = lightP->info.fRad;
		m_vld.buffers [3][i].v.coord.w = lightP->info.fBrightness;
		nLights++;
		}
	if (nLights) {
		m_vld.index [m_vld.nVertices].nVertex = nVertex;
		m_vld.index [m_vld.nVertices].nLights = nLights;
		m_vld.index [m_vld.nVertices].color = (CFloatVector) *colorP;
		m_vld.nVertices++;
		m_vld.nLights += nLights;
		}
	//lightManager.Index (0) [0].nActive = gameData.render.lights.dynamic.shader.iVertexLights [0];
	}	
else if (nState == 2) {
	Render ();
	shaderManager.Deploy (-1);
	lightManager.FBO ().Disable ();
	glMatrixMode (GL_PROJECTION);    
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);                         
	glPopMatrix ();
	glPopAttrib ();
	ogl.SetDrawBuffer (GL_BACK, 1);
	for (i = 0; i < VL_SHADER_BUFFERS; i++) {
		ogl.DisableClientStates (1, 0, 0, GL_TEXTURE0 + i);
		ogl.SelectTMU (GL_TEXTURE0 + i);
		ogl.BindTexture (0);
		}
	ogl.SetDepthWrite (true);
	ogl.SetDepthTest (true);
	ogl.SetDepthMode (GL_LESS);
	ogl.SetAlphaTest (true);
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
if (!ogl.m_states.bVertexLighting)
	return;
ogl.m_states.bVertexLighting = 0;
#if GPGPU_VERTEX_LIGHTING
if (ogl.m_features.bRenderToTexture.Available () && ogl.m_features.bShaders.Available ()) {
	PrintLog (1, "building vertex lighting shader program\n");
	ogl.m_states.bVertexLighting = 1;
	if (m_nShaderProg)
		DeleteShaderProg (&m_nShaderProg);
	if (0 > shaderManager.Build (m_nShaderProg, gpgpuLightFS, vertLightVS)) {
		ogl.m_states.bVertexLighting = 0;
		shaderManager.Delete (m_nShaderProg);
		}
	PrintLog (-1);
	}
#endif
}

//------------------------------------------------------------------------------
// eof
