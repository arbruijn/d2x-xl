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
#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)
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
# include <SDL.h>
#endif

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "config.h"
#include "maths.h"
#include "crypt.h"
#include "strutil.h"
#include "segmath.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "network.h"
#include "gr.h"
#include "glow.h"
#include "gamefont.h"
#include "screens.h"
#include "interp.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texcache.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "findfile.h"
#include "rendermine.h"
#include "sphere.h"
#include "glare.h"
#include "menu.h"
#include "menubackground.h"
#include "cockpit.h"
#include "renderframe.h"
#include "automap.h"
#include "gpgpu_lighting.h"
#include "postprocessing.h"

//#define _WIN32_WINNT		0x0600

int32_t enhance3DShaderProg [2][2][3] = {{{-1,-1,-1},{-1,-1,-1}},{{-1,-1,-1},{-1,-1,-1}}};
int32_t duboisShaderProg = -1;

int32_t vrWarpShaderProg [2] = {-1, -1};

int32_t nScreenDists [10] = {1, 2, 5, 10, 15, 20, 30, 50, 70, 100};
float nDeghostThresholds [4][2] = {{1.0f, 1.0f}, {0.8f, 0.8f}, {0.7f, 0.7f}, {0.6f, 0.6f}};

extern tTexCoord2f quadTexCoord [3][4];
extern float quadVerts [3][4][2];

//-----------------------------------------------------------------------------------

#define CHROM_AB_CORRECTION	1

#ifdef USE_OPENVR

struct DistortionConfig {
	float XCenterOffset;
	float YCenterOffset;
	float Scale;
	float K[4];
	float ChromaticAberration[4];
	DistortionConfig(float k0, float k1, float k2, float k3) {
		K[0] = k0;
		K[1] = k1;
		K[2] = k2;
		K[3] = k3;
	}
	DistortionConfig() {
	}
	void SetChromaticAberration(float v0, float v1, float v2, float v3) {
		ChromaticAberration[0] = v0;
		ChromaticAberration[1] = v1;
		ChromaticAberration[2] = v2;
		ChromaticAberration[3] = v3 ; 
	}
};

class CDefaultDistortionConfig : public DistortionConfig {
	public:
		CDefaultDistortionConfig () 
			: DistortionConfig (1.0f, 0.22f, 0.24f, 0.0f)
			{
			XCenterOffset = 0.15197642f;
			YCenterOffset = 0.0f;
			Scale = 1.7146056f;
			SetChromaticAberration (0.996f, -0.004f, 1.014f, 0.0f);
			}
	};

static CDefaultDistortionConfig defaultDistortion;

static bool VRWarpFrame (const DistortionConfig* pDistortion, int32_t nEye)
{
	DistortionConfig distortion;

distortion = (gameData.renderData.vr.Available () && pDistortion) ? *pDistortion : defaultDistortion;

float w = float (gameData.renderData.frame.Width ()) / float (gameData.renderData.screen.Width ()),
      h = float (gameData.renderData.frame.Height ()) / float (gameData.renderData.screen.Height ()),
      x = float (gameData.renderData.frame.Left ()) / float (gameData.renderData.screen.Width ()),
      y = float (gameData.renderData.frame.Top ()) / float (gameData.renderData.screen.Height ());

float as = float (gameData.renderData.frame.Width ()) / float(gameData.renderData.frame.Height ());

GLhandleARB warpProg = GLhandleARB (shaderManager.Deploy (vrWarpShaderProg [gameOpts->render.stereo.bChromAbCorr]));
if (!warpProg)
	return false;
if (shaderManager.Rebuild (warpProg))
	;
// We are using 1/4 of DistortionCenter offset value here, since it is
// relative to [-1,1] range that gets mapped to [0, 0.5].
shaderManager.Set ("LensCenter", x + (w + (nEye ? -distortion.XCenterOffset : distortion.XCenterOffset) * 0.5f) * 0.5f, y + h * 0.5f);
shaderManager.Set ("ScreenCenter", x + w * 0.5f, y + h * 0.5f);

// MA: This is more correct but we would need higher-res texture vertically; we should adopt this
// once we have asymmetric input texture scale.
#if 0 //DBG -- obsolete, experimental code to decrease the size of the render output. gameOpts->render.stereo.nVRFOV now actually influences the FOV.
float scaleFactor;
if (gameOpts->render.stereo.nVRFOV == VR_MAX_FOV) 
	scaleFactor = 1.0f / distortion.Scale;
else {
	float i, f = modf (distortion.Scale, &i);
	scaleFactor = 1.0f / (i + f * (float (gameOpts->render.stereo.nVRFOV) / float (VR_MAX_FOV)));
	}
#else
float scaleFactor = 1.0f / distortion.Scale;
#endif
shaderManager.Set ("Scale", (w / 2) * scaleFactor, (h / 2) * scaleFactor * as);
shaderManager.Set ("ScaleIn", 2.0f / w, 2.0f / h / as);
shaderManager.Set ("HmdWarpParam", distortion.K [0], distortion.K [1], distortion.K [2], distortion.K [3]);
if (gameOpts->render.stereo.bChromAbCorr)
	shaderManager.Set ("ChromAbParam",
		                distortion.ChromaticAberration [0], 
			             distortion.ChromaticAberration [1],
				          distortion.ChromaticAberration [2],
					       distortion.ChromaticAberration [3]);
glUniform1i (glGetUniformLocation (warpProg, "SceneTex"), 0);
OglDrawArrays (GL_QUADS, 0, 4);
return true;
}

#endif

//-----------------------------------------------------------------------------------

#ifdef USE_OPENVR
#	if DBG
static bool bWarpFrame = true;
#	endif
#endif

bool VRWarpScene (void)
{
#if !DBG
if (!gameData.renderData.vr.Available ())
	return false;
#endif
#ifdef USE_OPENVR
if (!ogl.VRActive ())
	return false;
if (!gameStates.render.textures.bHaveVRWarpShader)
	return false;

for (int32_t i = 0; i < 2; i++) {
	gameData.SetStereoSeparation (i ? STEREO_RIGHT_FRAME : STEREO_LEFT_FRAME);
	SetupCanvasses ();
	gameData.renderData.frame.Activate ("VRWarpScene (frame)");
	ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
	OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord [i + 1]);
	OglVertexPointer (2, GL_FLOAT, 0, quadVerts [0]);
#if DBG
	if (!bWarpFrame)
		OglDrawArrays (GL_QUADS, 0, 4);
	else
#endif
	if (!VRWarpFrame (gameData.renderData.vr.m_eyes [i].pDistortion, i)) {
		gameData.renderData.frame.Deactivate ();
		return false;
		}
	gameData.renderData.frame.Deactivate ();
	}	

#endif
return true;
}

//-----------------------------------------------------------------------------------

const char* enhance3DFS [2][2][3] = {
	{
		{
	#if 0
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"void main() {\r\n" \
		"gl_FragColor = vec4 (texture2D (leftFrame, gl_TexCoord [0].xy).xy, dot (texture2D (rightFrame, gl_TexCoord [0].xy).rgb, vec3 (0.15, 0.15, 0.7)), 1.0);\r\n" \
		"}",
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"gl_FragColor = vec4 (clamp (1.0, dot (texture2D (leftFrame, gl_TexCoord [0].xy).rgb, vec3 (1.0, 0.15, 0.15))), texture2D (rightFrame, gl_TexCoord [0].xy).yz, 0.0, 1.0);\r\n" \
		"}",
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"gl_FragColor = vec4 (cl.r, dot (texture2D (rightFrame, gl_TexCoord [0].xy).rgb, vec3 (0.15, 0.7, 0.15)), cl.b, 1.0);\r\n" \
		"}"
	#else
		// amber/blue
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"void main() {\r\n" \
		"vec3 c = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - c.b, 0.3) / max (0.000001, c.r + c.g);\r\n" \
		"gl_FragColor = vec4 (texture2D (leftFrame, gl_TexCoord [0].xy).xy, dot (c, vec3 (c.r * s, c.g * s, 1.0)), 1.0);\r\n" \
		"}",
		// red/cyan
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"void main() {\r\n" \
		"vec3 c = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - c.r, 0.3) / max (0.000001, c.g + c.b);\r\n" \
		"gl_FragColor = vec4 (dot (c, vec3 (1.0, c.g * s, c.b * s)), texture2D (rightFrame, gl_TexCoord [0].xy).yz, 1.0);\r\n" \
		"}",
		// green/magenta
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - cl.g, 0.3) / max (0.000001, cl.r + cl.b);\r\n" \
		"gl_FragColor = vec4 (cr.r, dot (cl, vec3 (cl.r * s, 1.0, cl.b * s)), cr.b, 1.0);\r\n" \
		"}"
#endif
		},
		{
		// amber/blue
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dr = (1.0 - cr.b) /  max (0.000001, cr.r + cr.g) / gain;\r\n" \
		"float dl = 1.0 + cl.b /  max (0.000001, cl.r + cl.g) / gain;\r\n" \
		"gl_FragColor = vec4 (cl.r * dl, cl.g * dl, dot (cr, vec3 (dr * cr.r, dr * cr.g, 1.0)), 1.0);\r\n" \
		"}",
		// red/cyan
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dl = (1.0 - cl.r) / max (0.000001, cl.g + cl.b) / gain;\r\n" \
		"float dr = 1.0 + cr.r /  max (0.000001, cr.g + cr.b) / gain;\r\n" \
		"gl_FragColor = vec4 (dot (cl, vec3 (1.0, dl * cl.g, dl * cl.b)), cr.g * dr, cr.b * dr, 1.0);\r\n" \
		"}",
		// green/magenta
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dl = (1.0 - cl.g) / max (0.000001, cl.r + cl.b) / gain;\r\n" \
		"float dr = 1.0 + cr.g /  max (0.000001, cr.r + cr.b) / gain;\r\n" \
		"gl_FragColor = vec4 (cr.r * dr, dot (cl, vec3 (dl * cl.r, 1.0, dl * cl.b)), cr.b * dr, 1.0);\r\n" \
		"}"
		},
	},
	// with de-ghosting
	{
		// no color adjustment
		{
		// amber/blue
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"void main() {\r\n" \
		"vec3 c = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - c.b, 0.3) / max (0.000001, c.r + c.g);\r\n" \
		"vec3 h = vec3 (texture2D (leftFrame, gl_TexCoord [0].xy).xy, dot (c, vec3 (c.r * s, c.g * s, 1.0)));\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"s = strength [0] / max (strength [0], (l.g + l.r) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.b / t);\r\n" \
		"gl_FragColor = vec4 (h.r * s, h.g * s, h.b * t, 1.0);\r\n" \
		"}",
		// red/cyan
		// de-ghosting by reducing red or cyan brightness if they exceed a certain threshold compared to the pixels total brightness
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"void main() {\r\n" \
		"vec3 c = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - c.r, 0.3) / max (0.000001, c.g + c.b);\r\n" \
		"vec3 h = vec3 (dot (c, vec3 (1.0, c.g * s, c.b * s)), texture2D (rightFrame, gl_TexCoord [0].xy).yz);\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"s = strength [0] / max (strength [0], (l.g + l.b) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.r / t);\r\n" \
		"gl_FragColor = vec4 (h.r * t, h.g * s, h.b * s, 1.0);\r\n" \
		"}",
		// green/magenta
		// de-ghosting by reducing red or cyan brightness if they exceed a certain threshold compared to the pixels total brightness
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - cl.g, 0.3) / max (0.000001, cl.r + cl.b);\r\n" \
		"vec3 h = vec3 (cr.r, dot (cl, vec3 (cl.r * s, 1.0, cl.b * s)), cr.b);\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"s = strength [0] / max (strength [0], (l.r + l.b) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.g / t);\r\n" \
		"gl_FragColor = vec4 (h.r * s, h.g * t, h.b * s, 1.0);\r\n" \
		"}"
		},
		// color adjustment
		{
		// amber/blue
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dr = (1.0 - cr.b) / max (0.000001, cr.r + cr.g) / gain;\r\n" \
		"float dl = 1.0 + cl.b /  max (0.000001, cl.r + cl.g) / gain;\r\n" \
		"vec3 h = vec3 (cl.r * dl, cl.g * dl, dot (cr, vec3 (dr * cr.r, dr * cr.g, 1.0)));\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"float s = strength [0] / max (strength [0], (l.g + l.r) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.b / t);\r\n" \
		"gl_FragColor = vec4 (h.r * s, h.g * s, h.b * t, 1.0);\r\n" \
		"}",
		// red/cyan
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dl = (1.0 - cl.r) / max (0.000001, cl.g + cl.b) / gain;\r\n" \
		"float dr = 1.0 + cr.r /  max (0.000001, cr.g + cr.b) / gain;\r\n" \
		"vec3 h = vec3 (dot (cl, vec3 (1.0, dl * cl.g, dl * cl.b)), cr.g * dr, cr.b * dr);\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"float s = strength [0] / max (strength [0], (l.g + l.b) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.r / t);\r\n" \
		"gl_FragColor = vec4 (h.r * t, h.g * s, h.b * s, 1.0);\r\n" \
		"}",
		// red/cyan
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dl = (1.0 - cl.g) / max (0.000001, cl.r + cl.b) / gain;\r\n" \
		"float dr = 1.0 + cr.g /  max (0.000001, cr.r + cr.b) / gain;\r\n" \
		"vec3 h = vec3 (cr.r * dr, dot (cl, vec3 (dl * cl.r, 1.0, dl * cl.b)), cr.b * dr);\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"float s = strength [0] / max (strength [0], (l.r + l.b) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.g / t);\r\n" \
		"gl_FragColor = vec4 (h.r * s, h.g * t, h.b * s, 1.0);\r\n" \
		"}"
		}
	}
};

const char* duboisFS = 
#if 0
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"vec3 rl = vec3 ( 0.4561000,  0.5004840,  0.17638100);\r\n" \
		"vec3 gl = vec3 (-0.0400822, -0.0378246, -0.01575890);\r\n" \
		"vec3 bl = vec3 (-0.0152161, -0.0205971, -0.00546856);\r\n" \
		"vec3 rr = vec3 (-0.0434706, -0.0879388, -0.00155529);\r\n" \
		"vec3 gr = vec3 ( 0.3784760,  0.7336400, -0.01845030);\r\n" \
		"vec3 br = vec3 (-0.0721527, -0.1129610,  1.22640000);\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"gl_FragColor = vec4 (clamp (dot (cl, rl) + dot (cr, rr), 0.0, 1.0),\r\n" \
		"                     clamp (dot (cl, gl) + dot (cr, gr), 0.0, 1.0),\r\n" \
		"                     clamp (dot (cl, bl) + dot (cr, br), 0.0, 1.0), 1.0);\r\n" \
		"}";
#else
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"mat3 lScale = mat3 ( 0.4561000,  0.5004840,  0.17638100,\r\n" \
		"                    -0.0400822, -0.0378246, -0.01575890,\r\n" \
		"                    -0.0152161, -0.0205971, -0.00546856);\r\n" \
		"mat3 rScale = mat3 (-0.0434706, -0.0879388, -0.00155529,\r\n" \
		"                     0.3784760,  0.7336400, -0.01845030,\r\n" \
		"                    -0.0721527, -0.1129610,  1.22640000);\r\n" \
		"void main() {\r\n" \
		"gl_FragColor = vec4 (clamp (texture2D (leftFrame, gl_TexCoord [0].xy).rgb * lScale + texture2D (rightFrame, gl_TexCoord [0].xy).rgb * rScale,\r\n" \
		"                            vec3 (0.0, 0.0, 0.0), vec3 (1.0, 1.0, 1.0)), 1.0);\r\n" \
		"}";
#endif

const char* enhance3DVS = 
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_Position=ftransform();"\
	"gl_FrontColor=gl_Color;}"
	;


static const char* vrWarpVS =
    "void main()\n"
    "{\n"
	 "gl_Position = ftransform ();\n"
	 "gl_TexCoord [0]=gl_MultiTexCoord0;"
    "}\n";

#if CHROM_AB_CORRECTION

static const char* vrWarpFS [2] = {
	"uniform vec2 LensCenter;\n"
	"uniform vec2 ScreenCenter;\n"
	"uniform vec2 Scale;\n"
	"uniform vec2 ScaleIn;\n"
	"uniform vec4 HmdWarpParam;\n"
	"uniform vec4 ChromAbParam;\n"
	"uniform sampler2D SceneTex;\n"
	"\n"
	"void main()\n"
	"{\n"
	"vec2 theta = (gl_TexCoord [0].xy - LensCenter) * ScaleIn;\n" // Scales to [-1,1]
	"float rSq = theta.x * theta.x + theta.y * theta.y;\n"
	"theta *= Scale * (HmdWarpParam.x + HmdWarpParam.y * rSq + HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);\n"
	"vec2 tcBlue = LensCenter + theta * (ChromAbParam.z + ChromAbParam.w * rSq);\n"
	"gl_FragColor =\n"
	"   (clamp (tcBlue, ScreenCenter - vec2 (0.25, 0.5), ScreenCenter + vec2 (0.25, 0.5)) == tcBlue)\n"
	"   ? gl_FragColor = vec4 (texture2D (SceneTex, LensCenter + theta * (ChromAbParam.x + ChromAbParam.y * rSq)).r,\n"
	"                          texture2D (SceneTex, LensCenter + theta).g,\n"
	"                          texture2D (SceneTex, tcBlue).b,\n"
	"                          1.0)\n"
	"   : vec4 (0.0, 0.0, 0.0, 1.0);\n"
	"}\n"
	,
	"uniform vec2 LensCenter;\n"
	"uniform vec2 ScreenCenter;\n"
	"uniform vec2 Scale;\n"
	"uniform vec2 ScaleIn;\n"
	"uniform vec4 HmdWarpParam;\n"
	"uniform sampler2D SceneTex;\n"
	"\n"
	"void main()\n"
	"{\n"
	"vec2 theta = (gl_TexCoord [0].xy - LensCenter) * ScaleIn;\n" // Scales to [-1,1]
	"float rSq = theta.x * theta.x + theta.y * theta.y;\n"
	"theta *= (HmdWarpParam.x + HmdWarpParam.y * rSq + HmdWarpParam.z * rSq * rSq +	HmdWarpParam.w	* rSq * rSq * rSq);\n"
	"vec2 tc = LensCenter + Scale * theta;\n"
	"gl_FragColor =\n"
	"(clamp(tc, ScreenCenter-vec2(0.25,0.5), ScreenCenter+vec2(0.25,0.5)) == tc)\n"
	"? texture2D(SceneTex, tc)\n"
	": vec4(0.0, 0.0, 0.0, 1.0);\n"
	"}\n"
};

#endif

//------------------------------------------------------------------------------

void COGL::InitEnhanced3DShader (void)
{
if (gameOpts->render.bUseShaders && m_features.bShaders.Available ()) {
	PrintLog (0, "building dubois optimization programs\n");
	gameStates.render.textures.bHaveEnhanced3DShader = (0 <= shaderManager.Build (duboisShaderProg, duboisFS, enhance3DVS));
	if (!gameStates.render.textures.bHaveEnhanced3DShader) {
		DeleteEnhanced3DShader ();
		gameOpts->render.stereo.nGlasses = GLASSES_NONE;
		return;
		}

	PrintLog (0, "building VR warp shader program\n");
	gameStates.render.textures.bHaveVRWarpShader = 
		(0 <= shaderManager.Build (vrWarpShaderProg [0], vrWarpFS [0], vrWarpVS)) &&
		(0 <= shaderManager.Build (vrWarpShaderProg [1], vrWarpFS [1], vrWarpVS));

	PrintLog (0, "building enhanced 3D shader programs\n");
	for (int32_t h = 0; h < 2; h++) {
		for (int32_t i = 0; i < 2; i++) {
			for (int32_t j = 0; j < 3; j++) {
				gameStates.render.textures.bHaveEnhanced3DShader = (0 <= shaderManager.Build (enhance3DShaderProg [h][i][j], enhance3DFS [h][i][j], enhance3DVS));
				if (!gameStates.render.textures.bHaveEnhanced3DShader) {
					DeleteEnhanced3DShader ();
					gameOpts->render.stereo.nGlasses = GLASSES_NONE;
					return;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void COGL::DeleteEnhanced3DShader (void)
{
if (duboisShaderProg >= 0) {
	shaderManager.Delete (duboisShaderProg);
	shaderManager.Delete (vrWarpShaderProg [0]);
	shaderManager.Delete (vrWarpShaderProg [1]);
	for (int32_t h = 0; h < 2; h++) {
		for (int32_t i = 0; i < 2; i++) {
			for (int32_t j = 0; j < 3; j++) {
				shaderManager.Delete (enhance3DShaderProg [h][i][j]);
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------------

void COGL::MergeAnaglyphBuffers (void)
{
if (m_data.xStereoSeparation > 0) {
	static float gain [4] = {1.0, 4.0, 2.0, 1.0};

	int32_t nDevice = StereoDevice ();
	int32_t h = gameOpts->render.stereo.bDeghost;
	int32_t i = (gameOpts->render.stereo.bColorGain > 0);
	if (postProcessManager.HaveEffects ()) // additional effect and/or shadow map rendering
		SelectDrawBuffer (2); // use as temporary render buffer
	else
		SetDrawBuffer (GL_BACK, 0);
	EnableClientStates (1, 0, 0, GL_TEXTURE0);
	BindTexture (DrawBuffer (0)->ColorBuffer ()); // set source for subsequent rendering step
	OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord [0]);
	OglVertexPointer (2, GL_FLOAT, 0, quadVerts [0]);
	if ((nDevice > 0) && (nDevice <= 3) && ((h < 4) || (nDevice == 2))) {
		GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy ((h == 4) ? duboisShaderProg : enhance3DShaderProg [h > 0][i][--nDevice]));
		if (shaderProg) {
			shaderManager.Rebuild (shaderProg);
			ogl.EnableClientStates (1, 0, 0, GL_TEXTURE1);
			ogl.BindTexture (DrawBuffer (1)->ColorBuffer ());
			OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord [0]);
			OglVertexPointer (2, GL_FLOAT, 0, quadVerts [0]);

			glUniform1i (glGetUniformLocation (shaderProg, "leftFrame"), gameOpts->render.stereo.bFlipFrames);
			glUniform1i (glGetUniformLocation (shaderProg, "rightFrame"), !gameOpts->render.stereo.bFlipFrames);
			if (h < 4) {
				if (h)
					glUniform2fv (glGetUniformLocation (shaderProg, "strength"), 1, reinterpret_cast<GLfloat*> (&nDeghostThresholds [gameOpts->render.stereo.bDeghost]));
				if (i)
					glUniform1f (glGetUniformLocation (shaderProg, "gain"), gain [gameOpts->render.stereo.bColorGain]);
				}
			ClearError (0);
			}
		}
	OglDrawArrays (GL_QUADS, 0, 4);
	}
}

//------------------------------------------------------------------------------
