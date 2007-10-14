/* $Id: gr.c,v 1.16 2003/11/27 04:59:49 btb Exp $ */
/*
 *
 * OGL video functions. - Added 9/15/99 Matthew Mueller
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#include <io.h>
#include <windows.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "ogl_init.h"

#include "inferno.h"
#include "hudmsg.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"

#include "inferno.h"
#include "screens.h"

#include "strutil.h"
#include "mono.h"
#include "args.h"
#include "key.h"
#include "gauges.h"
#include "render.h"
#include "particles.h"
#include "newmenu.h"

#define DECLARE_VARS

void GrPaletteStepClear(void); // Function prototype for GrInit;
void ResetHoardData (void);

extern int screenShotIntervals [];

//------------------------------------------------------------------------------

tScrSize	scrSizes [] = {
	{ 320, 200,1},
	{ 640, 400,0},
	{ 640, 480,0},
	{ 800, 600,0},
	{1024, 768,0},
	{1152, 864,0},
	{1280, 960,0},
	{1280,1024,0},
	{1600,1200,0},
	{2048,1526,0},
	{ 720, 480,0},
	{1280, 768,0},
	{1280, 800,0},
	{1280, 854,0},
	{1400,1050,0},
	{1440, 900,0},
	{1440, 960,0},
	{1680,1050,0},
   {1920,1200,0},
	{0,0,0}
};

//------------------------------------------------------------------------------

char *ScrSizeArg (int x, int y)
{
	static	char	szScrMode [20];

sprintf (szScrMode, "-%dx%d", x, y);
return szScrMode;
}

//------------------------------------------------------------------------------

int SCREENMODE (int x, int y, int c)
{
	int i = FindArg (ScrSizeArg (x, y));

if (i && (i < Num_args)) {
	gameStates.gfx.bOverride = 1; 
	gameData.render.window.w = x;
	gameData.render.window.h = y;
	return gameStates.gfx.nStartScrMode = GetDisplayMode (SM (x, y)); 
	}
return -1;
}

//------------------------------------------------------------------------------

int S_MODE (u_int32_t *VV, int *VG)
{
	int	h, i;

for (i = 0; scrSizes [i].x && scrSizes [i].y; i++)
	if ((h = FindArg (ScrSizeArg (scrSizes [i].x, scrSizes [i].y))) && (h < Num_args)) {
		*VV = SM (scrSizes [i].x, scrSizes [i].y);
		*VG = 1; //always 1 in d2x-xl
		return h;
		}
return -1;
}	

//------------------------------------------------------------------------------

int GrCheckFullScreen(void)
{
return gameStates.ogl.bFullScreen;
}

//------------------------------------------------------------------------------

void GrDoFullScreen (int f)
{
if (gameStates.ogl.bVoodooHack)
	gameStates.ogl.bFullScreen = 1;//force fullscreen mode on voodoos.
else
	gameStates.ogl.bFullScreen = f;
if (gameStates.ogl.bInitialized)
	OglDoFullScreenInternal (0);
}

//------------------------------------------------------------------------------

int GrToggleFullScreen(void)
{
GrDoFullScreen(!gameStates.ogl.bFullScreen);
	//	grdCurScreen->scMode=0;//hack to get it to reset screen mode
return gameStates.ogl.bFullScreen;
}

//------------------------------------------------------------------------------

int arch_toggle_fullscreen_menu(void)
{
	unsigned char *buf=NULL;

if (gameStates.ogl.bReadPixels) {
	MALLOC(buf,unsigned char,grdCurScreen->scWidth*grdCurScreen->scHeight*3);
	glReadBuffer(GL_FRONT);
	glReadPixels(0,0,grdCurScreen->scWidth,grdCurScreen->scHeight,GL_RGB,GL_UNSIGNED_BYTE,buf);
	}
GrDoFullScreen(!gameStates.ogl.bFullScreen);
if (gameStates.ogl.bReadPixels){
//		glWritePixels(0,0,grdCurScreen->scWidth,grdCurScreen->scHeight,GL_RGB,GL_UNSIGNED_BYTE,buf);
	glRasterPos2f(0,0);
	glDrawPixels(grdCurScreen->scWidth,grdCurScreen->scHeight,GL_RGB,GL_UNSIGNED_BYTE,buf);
	D2_FREE(buf);
	}
	//	grdCurScreen->scMode=0;//hack to get it to reset screen mode
return gameStates.ogl.bFullScreen;
}

//------------------------------------------------------------------------------

void OglInitState(void)
{
/* select clearing (background) color   */
glClearColor (0.0, 0.0, 0.0, 0.0);
glShadeModel (GL_SMOOTH);
/* initialize viewing values */
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();
glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
glScalef (1.0f, -1.0f, 1.0f);
glTranslatef (0.0f, -1.0f, 0.0f);
GrPaletteStepUp (0,0,0);//in case its left over from in game
}

//------------------------------------------------------------------------------

GLenum curDrawBuffer = -1;

void OglSetScreenMode (void)
{
	GLint	glRes;

if ((gameStates.video.nLastScreenMode == gameStates.video.nScreenMode) && 
	 (gameStates.ogl.bLastFullScreen == gameStates.ogl.bFullScreen) &&
	 (gameStates.app.bGameRunning || (gameStates.video.nScreenMode == SCREEN_GAME) || (curDrawBuffer == GL_FRONT)))
	return;
if (gameStates.video.nScreenMode == SCREEN_GAME)
	glDrawBuffer (curDrawBuffer = GL_BACK);
else {
	glDrawBuffer (curDrawBuffer = (gameOpts->menus.nStyle ? GL_BACK : GL_FRONT));
	if (!(gameStates.app.bGameRunning && gameOpts->menus.nStyle)) {
		glGetIntegerv (GL_DRAW_BUFFER, &glRes);
		glClearColor (0,0,0,0);
		glClear (GL_COLOR_BUFFER_BIT);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();//clear matrix
		glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity ();//clear matrix
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable (GL_TEXTURE_2D);
		glDepthFunc (GL_ALWAYS); //LEQUAL);
		glDisable (GL_DEPTH_TEST);
		}
	}
gameStates.video.nLastScreenMode = gameStates.video.nScreenMode;
gameStates.ogl.bLastFullScreen = gameStates.ogl.bFullScreen;
}

//------------------------------------------------------------------------------

void GrUpdate (int bClear)
{
if (gameStates.ogl.bInitialized) {// && (gameStates.video.nScreenMode != SCREEN_GAME))
	if ((curDrawBuffer == GL_BACK) && (gameOpts->menus.nStyle || !gameStates.menus.nInMenu))
		OglSwapBuffers (1, bClear);
	else
		glFlush ();
	}
}

//------------------------------------------------------------------------------

const char *gl_vendor,*gl_renderer,*gl_version,*gl_extensions;

void OglGetVerInfo (void)
{
gl_vendor = (char *) glGetString (GL_VENDOR);
gl_renderer = (char *) glGetString (GL_RENDERER);
gl_version = (char *) glGetString (GL_VERSION);
gl_extensions = (char *) glGetString (GL_EXTENSIONS);
#if 0 //TRACE
con_printf(
	CON_VERBOSE, 
	"\ngl vendor:%s\nrenderer:%s\nversion:%s\nextensions:%s\n",
	gl_vendor,gl_renderer,gl_version,gl_extensions);
#endif
#if 0 //WGL only, I think
	dglMultiTexCoord2fARB = (glMultiTexCoord2fARB_fp)wglGetProcAddress("glMultiTexCoord2fARB");
	dglActiveTextureARB = (glActiveTextureARB_fp)wglGetProcAddress("glActiveTextureARB");
	dglMultiTexCoord2fSGIS = (glMultiTexCoord2fSGIS_fp)wglGetProcAddress("glMultiTexCoord2fSGIS");
	dglSelectTextureSGIS = (glSelectTextureSGIS_fp)wglGetProcAddress("glSelectTextureSGIS");
#endif

//multitexturing doesn't work yet.
#ifdef GL_ARB_multitexture
gameOpts->ogl.bArbMultiTexture=0;//(strstr(gl_extensions,"GL_ARB_multitexture")!=0 && glActiveTextureARB!=0 && 0);
#	if 0 //TRACE	
con_printf (CONDBG,"c:%p d:%p e:%p\n",strstr(gl_extensions,"GL_ARB_multitexture"),glActiveTextureARB,glBegin);
#	endif
#endif
#ifdef GL_SGIS_multitexture
gameOpts->ogl.bSgisMultiTexture=0;//(strstr(gl_extensions,"GL_SGIS_multitexture")!=0 && glSelectTextureSGIS!=0 && 0);
#	if TRACE	
con_printf (CONDBG,"a:%p b:%p\n",strstr(gl_extensions,"GL_SGIS_multitexture"),glSelectTextureSGIS);
#	endif	
#endif

//add driver specific hacks here.  whee.
if (gl_renderer) {
	if ((stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.0\n")==0 || stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.2\n")==0) && stricmp(gl_version,"1.2 Mesa 3.0")==0){
		gameStates.ogl.bIntensity4 = 0;	//ignores alpha, always black background instead of transparent.
		gameStates.ogl.bReadPixels = 0;	//either just returns all black, or kills the X server entirely
		gameStates.ogl.bGetTexLevelParam = 0;	//returns random data..
		}
	if (stricmp(gl_vendor,"Matrox Graphics Inc.")==0){
		//displays garbage. reported by
		//  redomen@crcwnet.com (render="Matrox G400" version="1.1.3 5.52.015")
		//  orulz (Matrox G200)
		gameStates.ogl.bIntensity4=0;
		}
	}
//allow overriding of stuff.
#if 0 //TRACE	
con_printf(CON_VERBOSE, 
	"gl_arb_multitexture:%i, gl_sgis_multitexture:%i\n",
	gameOpts->ogl.bArbMultiTexture,gameOpts->ogl.bSgisMultiTexture);
con_printf(CON_VERBOSE, 
	"gl_intensity4:%i, gl_luminance4_alpha4:%i, gl_readpixels:%i, gl_gettexlevelparam:%i\n",
	gameStates.ogl.bIntensity4, gameStates.ogl.bLuminance4Alpha4, gameStates.ogl.bReadPixels, gameStates.ogl.bGetTexLevelParam);
#endif
}

//------------------------------------------------------------------------------

int GrVideoModeOK (u_int32_t mode)
{
int w = SM_W(mode);
int h = SM_H(mode);
return OglVideoModeOK (w, h); // platform specific code
}

//------------------------------------------------------------------------------

extern int nCurrentVGAMode; // DPH: kludge - remove at all costs

int GrSetMode(u_int32_t mode)
{
	unsigned int w,h;
	unsigned char *gr_bm_data;
	//int bForce = (nCurrentVGAMode < 0);

#ifdef NOGRAPH
return 0;
#endif
if (mode<=0)
	return 0;
w=SM_W(mode);
h=SM_H(mode);
nCurrentVGAMode = mode;
gr_bm_data = grdCurScreen->scCanvas.cvBitmap.bmTexBuf;//since we use realloc, we want to keep this pointer around.
memset( grdCurScreen, 0, sizeof(grsScreen));
grdCurScreen->scMode = mode;
grdCurScreen->scWidth = w;
grdCurScreen->scHeight = h;
//grdCurScreen->scAspect = FixDiv(grdCurScreen->scWidth*3,grdCurScreen->scHeight*4);
grdCurScreen->scAspect = FixDiv (grdCurScreen->scWidth, (fix) (grdCurScreen->scHeight * ((double) w / (double) h)));
grdCurScreen->scCanvas.cvBitmap.bmProps.x = 0;
grdCurScreen->scCanvas.cvBitmap.bmProps.y = 0;
grdCurScreen->scCanvas.cvBitmap.bmProps.w = w;
grdCurScreen->scCanvas.cvBitmap.bmProps.h = h;
grdCurScreen->scCanvas.cvBitmap.bmBPP = 1;
grdCurScreen->scCanvas.cvBitmap.bmPalette = defaultPalette; //just need some valid palette here
//grdCurScreen->scCanvas.cvBitmap.bmProps.rowSize = screen->pitch;
grdCurScreen->scCanvas.cvBitmap.bmProps.rowSize = w;
grdCurScreen->scCanvas.cvBitmap.bmProps.nType = BM_OGL;
//grdCurScreen->scCanvas.cvBitmap.bmTexBuf = (unsigned char *)screen->pixels;
grdCurScreen->scCanvas.cvBitmap.bmTexBuf = D2_REALLOC(gr_bm_data,w*h);
GrSetCurrentCanvas(NULL);
//gr_enable_default_palette_loading();
/***/LogErr ("   initializing OpenGL window\n");
if (!OglInitWindow (w,h,0))	//platform specific code
	return 0;
OglGetVerInfo ();
/***/LogErr ("   initializing OpenGL view port\n");
OGL_VIEWPORT (0,0,w,h);
/***/LogErr ("   initializing OpenGL screen mode\n");
OglSetScreenMode();
GrUpdate (0);
//	gamefont_choose_game_font(w,h);
return 0;
}

//------------------------------------------------------------------------------

#define GLstrcmptestr(a,b) if (stricmp(a,#b)==0 || stricmp(a,"GL_" #b)==0)return GL_ ## b;

int ogl_atotexfilti(char *a,int min)
{
GLstrcmptestr(a,NEAREST);
GLstrcmptestr(a,LINEAR);
if (min){//mipmaps are valid only for the min filter
	GLstrcmptestr(a,NEAREST_MIPMAP_NEAREST);
	GLstrcmptestr(a,NEAREST_MIPMAP_LINEAR);
	GLstrcmptestr(a,LINEAR_MIPMAP_NEAREST);
	GLstrcmptestr(a,LINEAR_MIPMAP_LINEAR);
	}
Error("unknown/invalid texture filter %s\n",a);
return GL_NEAREST;
}

//------------------------------------------------------------------------------

int ogl_testneedmipmaps(int i)
{
switch (i){
	case GL_NEAREST:
	case GL_LINEAR:
		return 0;
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_LINEAR:
		return 1;
	}
Error("unknown texture filter %x\n",i);
return -1;
}

//------------------------------------------------------------------------------

#ifdef OGL_RUNTIME_LOAD
#ifdef _WIN32
char *OglLibPath="opengl32.dll";
#endif
#if defined(__unix__) || defined(__macosx__)
char *OglLibPath="libGL.so";
#endif

int ogl_rt_loaded=0;

int OglInitLoadLibrary(void)
{
	int t, retcode = 0;

if (ogl_rt_loaded)
	return 0;

if ((t = FindArg ("-gl_library")))
	OglLibPath = Args [t + 1];
retcode = OpenGL_LoadLibrary(true);
if (retcode) {
#if TRACE	
	con_printf (CONDBG,"OpenGL successfully loaded\n");
#endif	
	if(!glEnd)
		Error("Opengl: Functions not imported\n");
	else
		Error("Opengl: error loading %s\n",OglLibPath);
	ogl_rt_loaded = 1;
	}
return retcode;
}
#endif

//------------------------------------------------------------------------------

void GrRemapMonoFonts ();

void SetRenderQuality ()
{
	static int nCurQual = -1;

if (nCurQual == gameOpts->render.nQuality)
	return;
nCurQual = gameOpts->render.nQuality;
gameStates.ogl.texMagFilter = renderQualities [gameOpts->render.nQuality].texMagFilter;
gameStates.ogl.texMinFilter = renderQualities [gameOpts->render.nQuality].texMinFilter;
gameStates.ogl.bNeedMipMaps = renderQualities [gameOpts->render.nQuality].bNeedMipmap;
gameStates.ogl.bAntiAliasing = renderQualities [gameOpts->render.nQuality].bAntiAliasing;
ResetTextures (1, gameStates.app.bGameRunning);
}

//------------------------------------------------------------------------------

void ResetTextures (int bReload, int bGame)
{
if (gameStates.app.bInitialized && gameStates.ogl.bInitialized) {
	OglSmashTextureListInternal (); 
	NMFreeAltBg (1);
	if (bReload)
		GrRemapMonoFonts ();
	if (bGame) {
		FreeInventoryIcons ();
		FreeObjTallyIcons ();
		ResetHoardData ();
		FreeParticleImages ();
		FreeExtraImages ();
		LoadExtraImages ();
		FreeStringPool ();
		OOF_ReleaseTextures ();
		if (bReload) {
			OglCacheLevelTextures ();
			OOF_ReloadTextures ();
			}
		}
	}
}

//------------------------------------------------------------------------------

void OglInitExtensions (void);

int GrInit (void)
{
	int mode = SM (640, 480);
	int retcode, t;

// Only do this function once!
if (gameStates.gfx.bInstalled)
	return -1;

#ifdef OGL_RUNTIME_LOAD
OglInitLoadLibrary();
#endif
/***/LogErr ("   initializing SDL\n");
#ifdef _DEBUG
if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0)
#else
if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0)
#endif	
{
	LogErr ("SDL library video initialisation failed: %s.\n", SDL_GetError());
	Error("SDL library video initialisation failed: %s.", SDL_GetError());
}
if ((t = FindArg ("-gl_voodoo"))) {
	gameStates.ogl.bVoodooHack = 
	gameStates.ogl.bFullScreen = NumArg (t, 1);
	//GrToggleFullScreen();
	}
if ((t = FindArg("-fullscreen"))) {
	/***/LogErr ("   switching to fullscreen\n");
	gameStates.ogl.bFullScreen = NumArg (t, 1);
	//GrToggleFullScreen();
	}
SetRenderQuality ();
if ((t=FindArg ("-gl_vidmem"))){
	ogl_mem_target=atoi(Args[t+1])*1024*1024;
}
if ((t=FindArg ("-gl_reticle"))){
	gameStates.ogl.nReticle=atoi(Args[t+1]);
}
/***/LogErr ("   initializing internal texture list\n");
OglInitTextureListInternal();
/***/LogErr ("   allocating screen buffer\n");
MALLOC (grdCurScreen, grsScreen, 1);
memset (grdCurScreen, 0, sizeof (grsScreen));
grdCurScreen->scCanvas.cvBitmap.bmTexBuf = NULL;

// Set the mode.
for (t = 0; scrSizes [t].x && scrSizes [t].y; t++)
	if (FindArg (ScrSizeArg (scrSizes [t].x, scrSizes [t].y))) {
		gameStates.gfx.bOverride = 1;
		gameStates.gfx.nStartScrSize = t;
		gameStates.gfx.nStartScrMode =
		mode = SM (scrSizes [t].x, scrSizes [t].y);
		break;
		}
if ((retcode = GrSetMode (mode)))
	return retcode;

grdCurScreen->scCanvas.cvColor.index = 0;
grdCurScreen->scCanvas.cvColor.rgb = 0;
grdCurScreen->scCanvas.cvDrawMode = 0;
grdCurScreen->scCanvas.cvFont = NULL;
grdCurScreen->scCanvas.cvFontFgColor.index = 0;
grdCurScreen->scCanvas.cvFontBgColor.index = 0;
grdCurScreen->scCanvas.cvFontFgColor.rgb = 0;
grdCurScreen->scCanvas.cvFontBgColor.rgb = 0;
GrSetCurrentCanvas( &grdCurScreen->scCanvas );

gameStates.gfx.bInstalled = 1;
InitGammaRamp ();
//atexit(GrClose);
/***/LogErr ("   initializing OpenGL extensions\n");
OglInitExtensions();
return 0;
}

//------------------------------------------------------------------------------

void _CDECL_ GrClose()
{
LogErr ("shutting down graphics subsystem\n");
OglClose();//platform specific code
if (grdCurScreen) {
	if (grdCurScreen->scCanvas.cvBitmap.bmTexBuf)
		D2_FREE (grdCurScreen->scCanvas.cvBitmap.bmTexBuf);
	D2_FREE (grdCurScreen);
	}
#ifdef OGL_RUNTIME_LOAD
if (ogl_rt_loaded)
	OpenGL_LoadLibrary(false);
#endif
}

//------------------------------------------------------------------------------

extern int r_upixelc;

void OglUPixelC(int x, int y, grsColor *c)
{
r_upixelc++;
glDisable (GL_TEXTURE_2D);
glPointSize(1.0);
glBegin(GL_POINTS);
OglGrsColor (c);
glVertex2d ((x + grdCurCanv->cvBitmap.bmProps.x) / (double) gameStates.ogl.nLastW,
				1.0 - (y + grdCurCanv->cvBitmap.bmProps.y) / (double) gameStates.ogl.nLastW);
if (c->rgb)
	glDisable (GL_BLEND);
glEnd();
}

//------------------------------------------------------------------------------

void OglURect(int left,int top,int right,int bot)
{
	GLfloat		xo, yo, xf, yf;
	
xo = ((float) left + grdCurCanv->cvBitmap.bmProps.x) / (float) gameStates.ogl.nLastW;
xf = (float) (right + grdCurCanv->cvBitmap.bmProps.x)/ (float) gameStates.ogl.nLastW;
yo = 1.0f - (float) (top + grdCurCanv->cvBitmap.bmProps.y) / (float) gameStates.ogl.nLastH;
yf = 1.0f - (float) (bot + grdCurCanv->cvBitmap.bmProps.y) / (float) gameStates.ogl.nLastH;
	
glDisable (GL_TEXTURE_2D);
OglGrsColor (&COLOR);
glBegin (GL_QUADS);
glVertex2f (xo,yo);
glVertex2f (xo,yf);
glVertex2f (xf,yf);
glVertex2f (xf,yo);
if (COLOR.rgb || (gameStates.render.grAlpha < GR_ACTUAL_FADE_LEVELS))
	glDisable (GL_BLEND);
glEnd ();
}

//------------------------------------------------------------------------------

void OglULineC(int left,int top,int right,int bot, grsColor *c)
{
	GLfloat xo, yo, xf, yf;
	
xo = (left + grdCurCanv->cvBitmap.bmProps.x) / (float) gameStates.ogl.nLastW;
xf = (right + grdCurCanv->cvBitmap.bmProps.x) / (float) gameStates.ogl.nLastW;
yo = 1.0f - (top + grdCurCanv->cvBitmap.bmProps.y) / (float) gameStates.ogl.nLastH;
yf = 1.0f - (bot + grdCurCanv->cvBitmap.bmProps.y) / (float) gameStates.ogl.nLastH;
glDisable (GL_TEXTURE_2D);
OglGrsColor (c);
glBegin (GL_LINES);
glVertex2f (xo,yo);
glVertex2f (xf,yf);
if (c->rgb)
	glDisable (GL_BLEND);
glEnd();
}
	
//------------------------------------------------------------------------------

void OglUPolyC (int left, int top, int right, int bot, grsColor *c)
{
	GLfloat xo, yo, xf, yf;
	
xo = (left + grdCurCanv->cvBitmap.bmProps.x) / (float) gameStates.ogl.nLastW;
xf = (right + grdCurCanv->cvBitmap.bmProps.x) / (float) gameStates.ogl.nLastW;
yo = 1.0f - (top + grdCurCanv->cvBitmap.bmProps.y) / (float) gameStates.ogl.nLastH;
yf = 1.0f - (bot + grdCurCanv->cvBitmap.bmProps.y) / (float) gameStates.ogl.nLastH;
glDisable (GL_TEXTURE_2D);
OglGrsColor (c);
glBegin (GL_LINE_LOOP);
glVertex2f (xo, yo);
glVertex2f (xf, yo);
glVertex2f (xf, yf);
glVertex2f (xo, yf);
//glVertex2f (xo, yo);
if (c->rgb)
	glDisable (GL_BLEND);
glEnd();
}
	
//------------------------------------------------------------------------------

void OglDoPalFx (void)
{
	int	bDepthTest, bBlend;
	GLint	blendSrc, blendDest;
	
if (gameStates.render.bPaletteFadedOut) {
	if ((bBlend = glIsEnabled (GL_BLEND))) {
		glGetIntegerv (GL_BLEND_SRC, &blendSrc);
		glGetIntegerv (GL_BLEND_DST, &blendDest);
		}
	else
		glEnable (GL_BLEND);
	glBlendFunc (GL_ONE,GL_ONE);
	glColor3f (0,0,0);
	}
else if (gameStates.ogl.bDoPalStep) {
	if ((bBlend = glIsEnabled (GL_BLEND))) {
		glGetIntegerv (GL_BLEND_SRC, &blendSrc);
		glGetIntegerv (GL_BLEND_DST, &blendDest);
		}
	else
		glEnable (GL_BLEND);
	glBlendFunc (GL_ONE,GL_ONE);
	glColor3fv ((GLfloat *) &gameStates.ogl.fBright);
	}
else
	return;
if ((bDepthTest = glIsEnabled (GL_DEPTH_TEST)))
	glDisable (GL_DEPTH_TEST);
glDisable (GL_TEXTURE_2D);
glBegin (GL_QUADS);
glVertex2f (0,0);
glVertex2f (0,1);
glVertex2f (1,1);
glVertex2f (1,0);
glEnd ();
if (bDepthTest)
	glEnable (GL_DEPTH_TEST);
if (bBlend)
	glBlendFunc (blendSrc, blendDest);
else
	glDisable (GL_BLEND);
}

//------------------------------------------------------------------------------

void GrPaletteStepUp (int r, int g, int b)
{
if (gameStates.render.bPaletteFadedOut)
	return;
#if 0
if (!gameOpts->render.bDynLighting || gameStates.menus.nInMenu || !gameStates.app.bGameRunning) 
#endif
	{
	r += gameData.render.nPaletteGamma;
	g += gameData.render.nPaletteGamma;
	b += gameData.render.nPaletteGamma;
	}
CLAMP (r, 0, 64);
CLAMP (g, 0, 64);
CLAMP (b, 0, 64);
if ((gameStates.ogl.bright.red == r) && 
	 (gameStates.ogl.bright.green == g) && 
	 (gameStates.ogl.bright.blue == b))
	return;
gameStates.ogl.bright.red = r;
gameStates.ogl.bright.green = g;
gameStates.ogl.bright.blue = b;
if (gameOpts->ogl.bSetGammaRamp && gameStates.ogl.bBrightness) {
	gameStates.ogl.bBrightness = !OglSetBrightnessInternal ();
	gameStates.ogl.bDoPalStep = 0;
	}
else {
	gameStates.ogl.fBright.red = gameStates.ogl.bright.red / 64.0f;
	gameStates.ogl.fBright.green = gameStates.ogl.bright.green / 64.0f;
	gameStates.ogl.fBright.blue = gameStates.ogl.bright.blue / 64.0f;
	gameStates.ogl.bDoPalStep = (r || g || b); //if we arrive here, brightness needs adjustment
	}
}

//------------------------------------------------------------------------------

void GrPaletteStepClear()
{
GrPaletteStepUp (0, 0, 0);
//gameStates.ogl.bDoPalStep = !(gameOpts->ogl.bSetGammaRamp && gameStates.ogl.bBrightness);
gameStates.render.bPaletteFadedOut = 1;
}

//------------------------------------------------------------------------------

//added on 980913 by adb to fix palette problems
// need a min without tSide effects...
#undef min
static inline int min(int x, int y) { return x < y ? x : y; }
//end changes by adb

ubyte *pCurPal = NULL;

int GrPaletteStepLoad (ubyte *pal)	
{
	static int nCurPalGamma = -1;

#if 0
if (memcmp (gr_current_pal, pal, sizeof (gr_current_pal))) {
#if 0//def _WIN32
_asm {
	mov	esi,pal
	lea	edi,gr_current_pal
	mov	ecx,768/4
	cld
nextQuad:
	lodsd
	and	eax,0x3f3f3f3f
	stosd
	loop	nextQuad;
	}
#else
	int i;
 
	unsigned int	*pSrc = (unsigned int *) pal,
						*pDest = (unsigned int *) gr_current_pal;
for (i = 768 / 4; i; i--, pSrc++)
	*pDest++ = *pSrc & 0x3f3f3f3f;	//compute 4 bytes & 64 at once
#endif
	}
else 
#endif
	if (nCurPalGamma == gameData.render.nPaletteGamma)
		return 1;
nCurPalGamma = gameData.render.nPaletteGamma;
gameStates.render.bPaletteFadedOut = 0;
GrPaletteStepUp (0, 0, 0);
pCurPal = pal;
return 1;
//gameStates.ogl.bDoPalStep = !(gameOpts->ogl.bSetGammaRamp && gameStates.ogl.bBrightness);
}

//------------------------------------------------------------------------------

int GrPaletteFadeOut(ubyte *pal, int nsteps, int allow_keys)
{
gameStates.render.bPaletteFadedOut=1;
return 0;
}

//------------------------------------------------------------------------------

int GrPaletteFadeIn(ubyte *pal, int nsteps, int allow_keys)
{
gameStates.render.bPaletteFadedOut=0;
return 0;
}

//------------------------------------------------------------------------------

void GrPaletteRead(ubyte * pal)
{
Assert (1);
}

//------------------------------------------------------------------------------

//writes out an uncompressed RGB .tga file
//if we got really spiffy, we could optionally link in libpng or something, and use that.
void WriteScreenShot (char *szSaveName, int w, int h, unsigned char *buf, int nFormat)
{
	FILE *f = fopen (szSaveName, "wb");
if (!f) {
#if TRACE	
	con_printf (CONDBG,"screenshot error, couldn't open %s (err %i)\n",szSaveName,errno);
#endif
	}
else {
		tTgaHeader	hdr;
		int			i, j, r;
		tBGR			*outBuf = (tBGR *) D2_ALLOC (w * h * sizeof (tBGR));
		tBGR			*bgrP;
		tRGB			*rgbP;

	memset (&hdr, 0, sizeof (hdr));
#ifdef _WIN32
	hdr.imageType = 2;
	hdr.width = w;
	hdr.height = h;
	hdr.bits = 24;
	//write .TGA header.
	fwrite (&hdr, sizeof (hdr), 1, f);
#else	// if only I knew how to control struct member packing with gcc or XCode ... ^_^
	*(((char *) &hdr) + 2) = 2;
	*((short *) (((char *) &hdr) + 12)) = w;
	*((short *) (((char *) &hdr) + 14)) = h;
	*(((char *) &hdr) + 16) = 24;
	//write .TGA header.
	fwrite (&hdr, 18, 1, f);
#endif
	rgbP = ((tRGB *) buf);// + w * (h - 1);
	bgrP = outBuf;
	for (i = h; i; i--) {
		for (j = w; j; j--, rgbP++, bgrP++) {
			bgrP->r = rgbP->r;
			bgrP->g = rgbP->g;
			bgrP->b = rgbP->b;
			}
		}
	r = (int) fwrite (outBuf, w * h * 3, 1, f);
#if TRACE	
	if (r <= 0)
		con_printf (CONDBG,"screenshot error, couldn't write to %s (err %i)\n",szSaveName,errno);
#endif
	fclose (f);
	}
}

//------------------------------------------------------------------------------

extern int bSaveScreenShot;
extern char *pszSystemNames [];

void SaveScreenShot (unsigned char *buf, int bAutomap)
{
	char				szMessage [100];
	char				szSaveName [FILENAME_LEN], szLevelName [128];
	int				i, j, bTmpBuf;
	static int		nSaveNum = 0;
	GLenum			glErrCode;
	
if (!bSaveScreenShot)
	return;
bSaveScreenShot = 0;
if (!gameStates.ogl.bReadPixels) {
	if (!bAutomap)
		HUDMessage (MSGC_GAME_FEEDBACK, "Screenshots not supported on your configuration");
	return;
	}
for (i = j = 0; gameData.missions.szCurrentLevel [i]; i++)
	if (isalnum (gameData.missions.szCurrentLevel [i]))
		szLevelName [j++] = gameData.missions.szCurrentLevel [i];
	else if ((gameData.missions.szCurrentLevel [i] == ' ') && j && (szLevelName [j - 1] != '_'))
		szLevelName [j++] = '_';
while (j && (szLevelName [j - 1] == '_'))
	j--;
if (j) {
	szLevelName [j++] = '-';
	szLevelName [j] = '\0';
	}
else
	strcpy (szLevelName, "scrn");
StopTime();
if (*gameFolders.szScrShotDir)
	sprintf (szSaveName, "%s", gameFolders.szScrShotDir);
else
	*szSaveName = '\0';
i = (int) strlen (szSaveName);
do {
	sprintf (szSaveName + i, "/%s%04d.tga", szLevelName, nSaveNum++);
	nSaveNum %= 9999;
	} while (!access (szSaveName, 0));

if ((bTmpBuf = (buf == NULL))) {
	buf = D2_ALLOC (grdCurScreen->scWidth * grdCurScreen->scHeight * 3);
	glDisable (GL_TEXTURE_2D);
	glReadBuffer (GL_FRONT);
	glReadPixels (0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight, GL_RGB, GL_UNSIGNED_BYTE, buf);
	glErrCode = glGetError ();
	glErrCode = GL_NO_ERROR;
	}
else
	glErrCode = GL_NO_ERROR;
if (glErrCode == GL_NO_ERROR) {
	WriteScreenShot (szSaveName, grdCurScreen->scWidth, grdCurScreen->scHeight, buf, 0);
	if (!(bAutomap || screenShotIntervals [gameOpts->app.nScreenShotInterval])) {
		sprintf (szMessage, "%s '%s'", TXT_DUMPING_SCREEN, szSaveName);
		HUDMessage (MSGC_GAME_FEEDBACK, szMessage);
		}
	}
if (bTmpBuf)
	D2_FREE (buf);
//KeyFlush ();
StartTime ();
}

//------------------------------------------------------------------------------

