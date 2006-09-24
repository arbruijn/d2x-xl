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
#include "particles.h"

#define DECLARE_VARS

void GrPaletteStepClear(void); // Function prototype for GrInit;
void ResetHoardData (void);

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
	bScreenModeOverride = 1; 
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
	//	grdCurScreen->sc_mode=0;//hack to get it to reset screen mode
return gameStates.ogl.bFullScreen;
}

//------------------------------------------------------------------------------

int arch_toggle_fullscreen_menu(void)
{
	unsigned char *buf=NULL;

	if (gameOpts->ogl.bReadPixels){
		MALLOC(buf,unsigned char,grdCurScreen->sc_w*grdCurScreen->sc_h*3);
		glReadBuffer(GL_FRONT);
		glReadPixels(0,0,grdCurScreen->sc_w,grdCurScreen->sc_h,GL_RGB,GL_UNSIGNED_BYTE,buf);
	}

	GrDoFullScreen(!gameStates.ogl.bFullScreen);

	if (gameOpts->ogl.bReadPixels){
//		glWritePixels(0,0,grdCurScreen->sc_w,grdCurScreen->sc_h,GL_RGB,GL_UNSIGNED_BYTE,buf);
		glRasterPos2f(0,0);
		glDrawPixels(grdCurScreen->sc_w,grdCurScreen->sc_h,GL_RGB,GL_UNSIGNED_BYTE,buf);
		d_free(buf);
	}
	//	grdCurScreen->sc_mode=0;//hack to get it to reset screen mode

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
GrPaletteStepUp(0,0,0);//in case its left over from in game
}

//------------------------------------------------------------------------------

GLenum curDrawBuffer = -1;

void OglSetScreenMode (void)
{
	GLint	glRes;

if ((gameStates.video.nLastScreenMode == gameStates.video.nScreenMode) && 
	 (gameStates.ogl.bLastFullScreen == gameStates.ogl.bFullScreen) &&
	 (gameStates.app.bGameRunning || 
	  (gameStates.video.nScreenMode == SCREEN_GAME) || 
	  (curDrawBuffer == GL_FRONT)))
	return;
#if 0
if (gameStates.app.bGameRunning) {
//	gameStates.ogl.nLastW = gameStates.ogl.nLastH = 0;
	OGL_VIEWPORT (0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
	}
else {
	OGL_VIEWPORT (0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
	}
#endif
//	OGL_VIEWPORT(grdCurCanv->cv_bitmap.bm_props.x,grdCurCanv->cv_bitmap.bm_props.y,grdCurCanv->cv_bitmap.bm_props.w,grdCurCanv->cv_bitmap.bm_props.h);
if (gameStates.video.nScreenMode == SCREEN_GAME) {
	glDrawBuffer (curDrawBuffer = GL_BACK);
	}
else {
	glDrawBuffer (curDrawBuffer = (gameOpts->menus.nStyle ? GL_BACK : GL_FRONT));
	if (!(gameStates.app.bGameRunning && gameOpts->menus.nStyle)) {
		glGetIntegerv (GL_DRAW_BUFFER, &glRes);
		glClearColor (0.0, 0.0, 0.0, 0.0);
		glClear (GL_COLOR_BUFFER_BIT);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();//clear matrix
		//glFrustum (-1.0f, 1.0f, -1.0f, 1.0f, 1.5f, 20.0f);
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
if (gameStates.ogl.bInitialized)// && (gameStates.video.nScreenMode != SCREEN_GAME))
	if (curDrawBuffer == GL_BACK)
		OglSwapBuffers (1, bClear);
	else
		glFlush ();
}

//------------------------------------------------------------------------------

const char *gl_vendor,*gl_renderer,*gl_version,*gl_extensions;

void OglGetVerInfo(void)
{
	gl_vendor= (char *) glGetString (GL_VENDOR);
	gl_renderer= (char *) glGetString (GL_RENDERER);
	gl_version= (char *) glGetString (GL_VERSION);
	gl_extensions= (char *) glGetString (GL_EXTENSIONS);
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
#if 0 //TRACE	
	con_printf (CON_DEBUG,"c:%p d:%p e:%p\n",strstr(gl_extensions,"GL_ARB_multitexture"),glActiveTextureARB,glBegin);
#endif
#endif
#ifdef GL_SGIS_multitexture
	gameOpts->ogl.bSgisMultiTexture=0;//(strstr(gl_extensions,"GL_SGIS_multitexture")!=0 && glSelectTextureSGIS!=0 && 0);
#if TRACE	
	con_printf (CON_DEBUG,"a:%p b:%p\n",strstr(gl_extensions,"GL_SGIS_multitexture"),glSelectTextureSGIS);
#endif	
#endif

	//add driver specific hacks here.  whee.
	if (gl_renderer) {
		if ((stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.0\n")==0 || stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.2\n")==0) && stricmp(gl_version,"1.2 Mesa 3.0")==0){
			gameOpts->ogl.bIntensity4=0;//ignores alpha, always black background instead of transparent.
			gameOpts->ogl.bReadPixels=0;//either just returns all black, or kills the X server entirely
			gameOpts->ogl.bGetTexLevelParam=0;//returns random data..
		}
		if (stricmp(gl_vendor,"Matrox Graphics Inc.")==0){
			//displays garbage. reported by
			//  redomen@crcwnet.com (render="Matrox G400" version="1.1.3 5.52.015")
			//  orulz (Matrox G200)
			gameOpts->ogl.bIntensity4=0;
		}
	}
	//allow overriding of stuff.
#ifdef GL_ARB_multitexture
	if ((t=FindArg("-gl_arb_multitexture_ok"))){
		gameOpts->ogl.bArbMultiTexture=atoi(Args[t+1]);
	}
#endif
#ifdef GL_SGIS_multitexture
	if ((t=FindArg("-gl_sgis_multitexture_ok"))){
		gameOpts->ogl.bSgisMultiTexture=atoi(Args[t+1]);
	}
#endif
#if 0 //TRACE	
	con_printf(CON_VERBOSE, 
		"gl_arb_multitexture:%i, gl_sgis_multitexture:%i\n",
		gameOpts->ogl.bArbMultiTexture,gameOpts->ogl.bSgisMultiTexture);
	con_printf(CON_VERBOSE, 
		"gl_intensity4:%i, gl_luminance4_alpha4:%i, gl_rgba2:%i, gl_readpixels:%i, gl_gettexlevelparam:%i\n",
		gameOpts->ogl.bIntensity4,gameOpts->ogl.bLuminance4Alpha4,gameOpts->ogl.bRgba2,gameOpts->ogl.bReadPixels,gameOpts->ogl.bGetTexLevelParam);
#endif
}

//------------------------------------------------------------------------------

int GrVideoModeOK(u_int32_t mode)
{
	int w, h;

	w = SM_W(mode);
	h = SM_H(mode);
	return OglVideoModeOK(w, h); // platform specific code
}

//------------------------------------------------------------------------------

extern int VGA_current_mode; // DPH: kludge - remove at all costs

int GrSetMode(u_int32_t mode)
{
	unsigned int w,h;
	unsigned char *gr_bm_data;
	int bForce = (VGA_current_mode < 0);

#ifdef NOGRAPH
return 0;
#endif
//	mode=0;
	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);
	VGA_current_mode = mode;

	//if (screen != NULL) GrPaletteStepClear();

//	OglInitState();
	
	gr_bm_data = grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf;//since we use realloc, we want to keep this pointer around.
	memset( grdCurScreen, 0, sizeof(grs_screen));
	grdCurScreen->sc_mode = mode;
	grdCurScreen->sc_w = w;
	grdCurScreen->sc_h = h;
	//grdCurScreen->sc_aspect = fixdiv(grdCurScreen->sc_w*3,grdCurScreen->sc_h*4);
	grdCurScreen->sc_aspect = fixdiv (grdCurScreen->sc_w, (fix) (grdCurScreen->sc_h * ((double) w / (double) h)));
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.x = 0;
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.y = 0;
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.w = w;
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.h = h;
	grdCurScreen->sc_canvas.cv_bitmap.bm_palette = defaultPalette; //just need some valid palette here
	//grdCurScreen->sc_canvas.cv_bitmap.bm_props.rowsize = screen->pitch;
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.rowsize = w;
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.type = BM_OGL;
	//grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf = (unsigned char *)screen->pixels;
	grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf = d_realloc(gr_bm_data,w*h);
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

int ogl_testneedmipmaps(int i){
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
	int retcode=0;
	if (!ogl_rt_loaded) {
		int t;
		if ((t=FindArg("-gl_library")))
			OglLibPath=Args[t+1];

		retcode = OpenGL_LoadLibrary(true);
		if(retcode)
		{
#if TRACE	
			con_printf (CON_DEBUG,"OpenGL successfully loaded\n");
#endif	
			if(!glEnd)
			{
				Error("Opengl: Functions not imported\n");
			}
		}else{
			Error("Opengl: error loading %s\n",OglLibPath);
		}
		ogl_rt_loaded=1;
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
	if (bReload)
		GrRemapMonoFonts ();
	if (bGame) {
		FreeInventoryIcons ();
		ResetHoardData ();
		FreeParticleImages ();
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
	int retcode, t, glt = 0;

// Only do this function once!
if (gameStates.gfx.bInstalled==1)
	return -1;

#ifdef OGL_RUNTIME_LOAD
OglInitLoadLibrary();
#endif
/***/LogErr ("   initializing SDL\n");
#ifdef _DEBUG
if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0)
#else
if (SDL_Init(SDL_INIT_VIDEO) < 0)
#endif	
{
	LogErr ("SDL library video initialisation failed: %s.\n", SDL_GetError());
	Error("SDL library video initialisation failed: %s.", SDL_GetError());
}
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
if (t = FindArg ("-gl_voodoo")) {
	gameStates.ogl.bVoodooHack = 
	gameStates.ogl.bFullScreen = NumArg (t, 1);
	//GrToggleFullScreen();
	}
if (t = FindArg("-fullscreen")) {
	/***/LogErr ("   switching to fullscreen\n");
	gameStates.ogl.bFullScreen = NumArg (t, 1);
	//GrToggleFullScreen();
	}
#endif
#if 1	// use generalized render quality settings instead of allowing detailled settings
SetRenderQuality ();
#else
if ((glt=FindArg("-gl_mipmap"))){
	gameStates.ogl.texMagFilter=GL_LINEAR;
	gameStates.ogl.texMinFilter=GL_LINEAR_MIPMAP_NEAREST;
	}
if ((glt=FindArg("-gl_trilinear"))) {
	gameStates.ogl.texMagFilter = GL_LINEAR;
	gameStates.ogl.texMinFilter = GL_LINEAR_MIPMAP_LINEAR;
	}
if ((t=FindArg("-gl_simple"))) {
	if (t>=glt){//allow overriding of earlier args
		glt=t;
		gameStates.ogl.texMagFilter=GL_NEAREST;
		gameStates.ogl.texMinFilter=GL_NEAREST;
		}
	}
if ((t=FindArg("-gl_texmagfilt")) || (t=FindArg("-gl_texmagfilter"))){
	if (t>=glt)//allow overriding of earlier args
		gameStates.ogl.texMagFilter=ogl_atotexfilti(Args[t+1],0);
	}
if ((t=FindArg("-gl_texminfilt")) || (t=FindArg("-gl_texminfilter"))){
	if (t>=glt)//allow overriding of earlier args
		gameStates.ogl.texMinFilter=ogl_atotexfilti(Args[t+1],1);
	}
gameStates.ogl.bNeedMipMaps=ogl_testneedmipmaps (gameStates.ogl.texMinFilter);
#endif
if ((t=FindArg("-gl_vidmem"))){
	ogl_mem_target=atoi(Args[t+1])*1024*1024;
}
if ((t=FindArg("-gl_reticle"))){
	gameStates.ogl.nReticle=atoi(Args[t+1]);
}
/***/LogErr ("   initializing internal texture list\n");
OglInitTextureListInternal();
/***/LogErr ("   allocating screen buffer\n");
MALLOC (grdCurScreen, grs_screen, 1);
memset (grdCurScreen, 0, sizeof (grs_screen));
grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf = NULL;

// Set the mode.
for (t = 0; scrSizes [t].x && scrSizes [t].y; t++)
	if (FindArg (ScrSizeArg (scrSizes [t].x, scrSizes [t].y))) {
		gameStates.gfx.nStartScrSize = t;
		mode = SM (scrSizes [t].x, scrSizes [t].y);
		break;
		}
if ((retcode = GrSetMode (mode)))
	return retcode;

grdCurScreen->sc_canvas.cv_color.index = 0;
grdCurScreen->sc_canvas.cv_color.rgb = 0;
grdCurScreen->sc_canvas.cv_drawmode = 0;
grdCurScreen->sc_canvas.cv_font = NULL;
grdCurScreen->sc_canvas.cv_font_fg_color.index = 0;
grdCurScreen->sc_canvas.cv_font_bg_color.index = 0;
grdCurScreen->sc_canvas.cv_font_fg_color.rgb = 0;
grdCurScreen->sc_canvas.cv_font_bg_color.rgb = 0;
GrSetCurrentCanvas( &grdCurScreen->sc_canvas );

gameStates.gfx.bInstalled = 1;
InitGammaRamp ();
atexit(GrClose);
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
	if (grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf)
		d_free(grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf);
	d_free(grdCurScreen);
	}
#ifdef OGL_RUNTIME_LOAD
if (ogl_rt_loaded)
	OpenGL_LoadLibrary(false);
#endif
}

//------------------------------------------------------------------------------

extern int r_upixelc;

void OglUPixelC(int x, int y, grs_color *c)
{
r_upixelc++;
glDisable (GL_TEXTURE_2D);
glPointSize(1.0);
glBegin(GL_POINTS);
OglGrsColor (c);
glVertex2d ((x + grdCurCanv->cv_bitmap.bm_props.x) / (double) gameStates.ogl.nLastW,
				1.0 - (y + grdCurCanv->cv_bitmap.bm_props.y) / (double) gameStates.ogl.nLastW);
if (c->rgb)
	glDisable (GL_BLEND);
glEnd();
}

//------------------------------------------------------------------------------

void OglURect(int left,int top,int right,int bot)
{
	GLfloat		xo, yo, xf, yf;
	
xo = ((float) left + grdCurCanv->cv_bitmap.bm_props.x) / (float) gameStates.ogl.nLastW;
xf = (float) (right + grdCurCanv->cv_bitmap.bm_props.x)/ (float) gameStates.ogl.nLastW;
yo = 1.0f - (float) (top + grdCurCanv->cv_bitmap.bm_props.y) / (float) gameStates.ogl.nLastH;
yf = 1.0f - (float) (bot + grdCurCanv->cv_bitmap.bm_props.y) / (float) gameStates.ogl.nLastH;
	
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

void OglULineC(int left,int top,int right,int bot, grs_color *c)
{
	GLfloat xo, yo, xf, yf;
	
xo = (left + grdCurCanv->cv_bitmap.bm_props.x) / (float) gameStates.ogl.nLastW;
xf = (right + grdCurCanv->cv_bitmap.bm_props.x) / (float) gameStates.ogl.nLastW;
yo = 1.0f - (top + grdCurCanv->cv_bitmap.bm_props.y) / (float) gameStates.ogl.nLastH;
yf = 1.0f - (bot + grdCurCanv->cv_bitmap.bm_props.y) / (float) gameStates.ogl.nLastH;
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

void OglDoPalFx (void)
{
	int	bDepthTest, bBlend;
	GLint	blendSrc, blendDest;
	
glDisable (GL_TEXTURE_2D);
if (gameStates.render.bPaletteFadedOut) {
	if (bBlend = glIsEnabled (GL_BLEND)) {
		glGetIntegerv (GL_BLEND_SRC, &blendSrc);
		glGetIntegerv (GL_BLEND_DST, &blendDest);
		}
	else
		glEnable (GL_BLEND);
	glBlendFunc (GL_ONE,GL_ONE);
	glColor3f (0,0,0);
	}
else if (gameStates.ogl.bDoPalStep) {
	if (bBlend = glIsEnabled (GL_BLEND)) {
		glGetIntegerv (GL_BLEND_SRC, &blendSrc);
		glGetIntegerv (GL_BLEND_DST, &blendDest);
		}
	else
		glEnable (GL_BLEND);
	glBlendFunc (GL_ONE,GL_ONE);
	glColor3f (gameStates.ogl.fBright.red, gameStates.ogl.fBright.green, gameStates.ogl.fBright.blue);
	}
else
	return;
#ifdef OGL_ZBUF
if(!gameOpts->legacy.bZBuf)
	if (bDepthTest = glIsEnabled (GL_DEPTH_TEST))
		glDisable (GL_DEPTH_TEST);
#endif
glBegin (GL_QUADS);
glVertex2f (0,0);
glVertex2f (0,1);
glVertex2f (1,1);
glVertex2f (1,0);
glEnd ();
#ifdef OGL_ZBUF
if(!gameOpts->legacy.bZBuf)
	if (bDepthTest)
		glEnable (GL_DEPTH_TEST);
#endif
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
r += gameData.render.nPaletteGamma;
g += gameData.render.nPaletteGamma;
b += gameData.render.nPaletteGamma;
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
	gameStates.ogl.bDoPalStep = 1; //if we arrive here, brightness needs adjustment
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
// need a min without side effects...
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
	con_printf (CON_DEBUG,"screenshot error, couldn't open %s (err %i)\n",szSaveName,errno);
#endif
	}
else {
		tTgaHeader	hdr;
		int			i, j, r;
		tBGR			*outBuf = (tBGR *) d_malloc (w * h * sizeof (tBGR));
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
#if 0
	r = fwrite (buf, w * h * (hdr.bits / 8), 1, f);
#elif 1
	rgbP = ((tRGB *) buf);// + w * (h - 1);
	bgrP = outBuf;
	for (i = h; i; i--) {
		for (j = w; j; j--, rgbP++, bgrP++) {
			bgrP->r = rgbP->r;
			bgrP->g = rgbP->g;
			bgrP->b = rgbP->b;
			}
		}
	r = (int) fwrite (outBuf, w * h * (hdr.bits / 8), 1, f);
#else
	w *= (hdr.bits / 8);
	buf += (h - 1) * w;
	for (; h; h--, buf -= w)
		if (0 >= (r = fwrite (buf, w, 1, f)))
			break;
#endif
#if TRACE	
	if (r <= 0)
		con_printf (CON_DEBUG,"screenshot error, couldn't write to %s (err %i)\n",szSaveName,errno);
#endif
	fclose (f);
	}
}

//------------------------------------------------------------------------------

extern int bSaveScreenShot;

void SaveScreenShot (unsigned char *buf, int automap_flag)
{
//	fix t1;
	char				szMessage [100];
	char				szSaveName [FILENAME_LEN];
	int				i, bTmpBuf;
	static int		nSaveNum = 0;
	GLenum			glErrCode;
	
if (!bSaveScreenShot)
	return;
bSaveScreenShot = 0;
if (!gameOpts->ogl.bReadPixels){
	if (!automap_flag)
		HUDMessage(MSGC_GAME_FEEDBACK,"glReadPixels not supported on your configuration");
	return;
}

StopTime();
if (*gameFolders.szScrShotDir)
	sprintf (szSaveName, "%s/", gameFolders.szScrShotDir);
else
	*szSaveName = '\0';
i = (int) strlen (szSaveName);
//added/changed on 10/31/98 by Victor Rachels to fix overwrite each new game
do {
	sprintf (szSaveName + i, "scrn%04d.tga", nSaveNum++);
	nSaveNum %= 9999;
} while (!access (szSaveName, 0));
sprintf (szMessage, "%s '%s'", TXT_DUMPING_SCREEN, szSaveName);
//end this section addition/change - Victor Rachels

if (!automap_flag)
	HUDMessage(MSGC_GAME_FEEDBACK,szMessage);

if (bTmpBuf = (buf == NULL)) {
	buf = d_malloc (grdCurScreen->sc_w * grdCurScreen->sc_h * 3);
	glDisable (GL_TEXTURE_2D);
	glReadBuffer (GL_FRONT);
	glReadPixels (0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h, GL_RGB, GL_UNSIGNED_BYTE, buf);
	glErrCode = glGetError ();
	glErrCode = GL_NO_ERROR;
	}
else
	glErrCode = GL_NO_ERROR;
if (glErrCode == GL_NO_ERROR)
	WriteScreenShot (szSaveName, grdCurScreen->sc_w, grdCurScreen->sc_h, buf, 0);
if (bTmpBuf)
	d_free (buf);
KeyFlush ();
StartTime ();
}

//------------------------------------------------------------------------------

