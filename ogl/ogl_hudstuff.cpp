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
#include "maths.h"
#include "mouse.h"
#include "input.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texture.h"
#include "ogl_bitmap.h"
#include "ogl_render.h"
#include "ogl_hudstuff.h"
#include "cockpit.h"

//------------------------------------------------------------------------------

CBitmap *bmpDeadzone = NULL;
int bHaveDeadzone = 0;

int LoadDeadzone (void)
{
if (!bHaveDeadzone) {
	bmpDeadzone = CreateAndReadTGA ("deadzone.tga");
	bHaveDeadzone = bmpDeadzone ? 1 : -1;
	}
return bHaveDeadzone > 0;
}

//------------------------------------------------------------------------------

void FreeDeadzone (void)
{
if (bmpDeadzone) {
	delete bmpDeadzone;
	bHaveDeadzone = 0;
	}
}

//------------------------------------------------------------------------------

CBitmap *bmpJoyMouse = NULL;
int bHaveJoyMouse = 0;

int LoadJoyMouse (void)
{
if (!bHaveJoyMouse) {
	bmpJoyMouse = CreateAndReadTGA ("joymouse.tga");
	bHaveJoyMouse = bmpJoyMouse ? 1 : -1;
	}
return bHaveJoyMouse > 0;
}

//------------------------------------------------------------------------------

void FreeJoyMouse (void)
{
if (bmpJoyMouse) {
	delete bmpJoyMouse;
	bHaveJoyMouse = 0;
	}
}

//------------------------------------------------------------------------------

void OglDrawMouseIndicator (void)
{
	float 	scale = (float) screen.Width () / (float) screen.Height ();

	static tSinCosf sinCos30 [30];
	static tSinCosf sinCos12 [12];
	static int bInitSinCos = 1;

	static tCanvasColor color = {-1, 1, {255, 255, 255, 96}};

	float	r, w, h;

if (!gameOpts->input.mouse.nDeadzone)
	return;
if (bInitSinCos) {
	OglComputeSinCos (sizeofa (sinCos30), sinCos30);
	OglComputeSinCos (sizeofa (sinCos12), sinCos12);
	bInitSinCos = 0;
	}
#if 0
if (mouseIndList)
glCallList (mouseIndList);
else {
	glNewList (mouseIndList, GL_COMPILE_AND_EXECUTE);
#endif
	if (LoadJoyMouse ()) {
		color.color.alpha = 255;
		bmpJoyMouse->RenderScaled (mouseData.x - 8, mouseData.y - 8, 16, 16, I2X (1), 0, &color);
		color.color.alpha = 96;
		}
	else {
	glPushMatrix ();
	glTranslatef ((float) (mouseData.x) / (float) screen.Width (), 1.0f - (float) (mouseData.y) / (float) screen.Height (), 0);
	glScalef (scale / 320.0f, scale / 200.0f, scale);//the positions are based upon the standard reticle at 320x200 res.
	glDisable (GL_TEXTURE_2D);
	glEnable (GL_LINE_SMOOTH);
	glColor4f (1.0f, 0.8f, 0.0f, 0.9f);
	glLineWidth (3);
	OglDrawEllipse (12, GL_LINE_LOOP, 1.5f, 0, 1.5f * float (screen.Height ()) / float (screen.Width ()), 0, sinCos12);
	glPopMatrix ();
	}
	if (LoadDeadzone ()) {
		r = (float) CalcDeadzone (0, gameOpts->input.mouse.nDeadzone);
		w = r / (float) screen.Width ();
		h = r / (float) screen.Height ();
		glEnable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (bmpDeadzone->Bind (1, -1)) 
			return;
		bmpDeadzone->Texture ()->Wrap (GL_CLAMP);
		glPushMatrix ();
		glTranslatef (0.5f, 0.5f, 0);
		glColor4f (1.0f, 1.0f, 1.0f, 0.8f / (float) gameOpts->input.mouse.nDeadzone);
		glBegin (GL_QUADS);
		glTexCoord2f (0, 0);
		glVertex2f (-w, -h);
		glTexCoord2f (1, 0);
		glVertex2f (w, -h);
		glTexCoord2f (1, 1);
		glVertex2f (w, h);
		glTexCoord2f (0, 1);
		glVertex2f (-w, h);
		glEnd ();
		OGL_BINDTEX (0);
		glDisable (GL_TEXTURE_2D);
		glPopMatrix ();
		}
	else {
		LoadDeadzone ();
		glPushMatrix ();
		glTranslatef (0.5f, 0.5f, 0);
		glScalef (scale / 320.0f, scale / 200.0f, scale);	//the positions are based upon the standard reticle at 320x200 res.
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f (1.0f, 0.8f, 0.0f, 1.0f / (3.0f + 0.5f * gameOpts->input.mouse.nDeadzone));
		glLineWidth (4); //(GLfloat) (4 + 2 * gameOpts->input.mouse.nDeadzone));
		r = (float) CalcDeadzone (0, gameOpts->input.mouse.nDeadzone) / 4;
		glDisable (GL_TEXTURE_2D);
		OglDrawEllipse (30, GL_LINES, r, 0, r * float (screen.Height ()) / float (screen.Width ()), 0, sinCos30);
		glPopMatrix ();
		}
	glDisable (GL_LINE_SMOOTH);
	glLineWidth (1);
#if 0
	glEndList ();
   }
#endif
}

//------------------------------------------------------------------------------

float bright_g [4]={32.0f/256, 252.0f/256, 32.0f/256, 1.0f};
float dark_g  [4]={32.0f/256, 148.0f/256, 32.0f/256, 1.0f};
float darker_g [4]={32.0f/256, 128.0f/256, 32.0f/256, 1.0f};

void OglDrawReticle (int cross, int primary, int secondary)
{
	float scale = (float)nCanvasHeight / (float) screen.Height ();

	static tSinCosf sinCos8 [8];
	static tSinCosf sinCos12 [12];
	static tSinCosf sinCos16 [16];
	static int bInitSinCos = 1;

if (bInitSinCos) {
	OglComputeSinCos (sizeofa (sinCos8), sinCos8);
	OglComputeSinCos (sizeofa (sinCos12), sinCos12);
	OglComputeSinCos (sizeofa (sinCos16), sinCos16);
	bInitSinCos = 0;
	}
glPushMatrix ();
//	glTranslated (0.5, 0.5, 0);
glTranslated (
	(CCanvas::Current ()->Width ()/2+CCanvas::Current ()->Left ()) / (float) gameStates.ogl.nLastW, 
	1.0f - (CCanvas::Current ()->Height () / ((cockpit->Type () == CM_FULL_COCKPIT) ? 2 : 2) +
	CCanvas::Current ()->Top ()) / (float) gameStates.ogl.nLastH, 
	0);
glScaled (scale/320.0f, scale/200.0f, scale);//the positions are based upon the standard reticle at 320x200 res.
glDisable (GL_TEXTURE_2D);

glLineWidth (5);
glEnable (GL_LINE_SMOOTH);
if (cross_lh [cross])
	glCallList (cross_lh [cross]);
else {
	cross_lh [cross] = glGenLists (1);
	glNewList (cross_lh [cross], GL_COMPILE_AND_EXECUTE);
	glBegin (GL_LINES);
	//cross top left
	glColor3fv (darker_g);
	glVertex2f (-4.0f, 4.0f);
	glColor3fv (cross ? bright_g : dark_g);
	glVertex2f (-2.0f, 2.0f);
	//cross bottom left
	glColor3fv (dark_g);
	glVertex2f (-3.0f, -2.0f);
	if (cross)
		glColor3fv (bright_g);
	glVertex2f (-2.0f, -1.0f);
	//cross top right
	glColor3fv (darker_g);
	glVertex2f (4.0f, 4.0f);
	glColor3fv (cross ? bright_g : dark_g);
	glVertex2f (2.0f, 2.0f);
	//cross bottom right
	glColor3fv (dark_g);
	glVertex2f (3.0f, -2.0f);
	if (cross)
		glColor3fv (bright_g);
	glVertex2f (2.0f, -1.0f);
	glEnd ();
	glEndList ();
	}

//	if (nCanvasHeight>200)
//		glLineWidth (nCanvasHeight/ (float)200);
if (primary_lh [primary])
	glCallList (primary_lh [primary]);
else {
	primary_lh [primary] = glGenLists (1);
	glNewList (primary_lh [primary], GL_COMPILE_AND_EXECUTE);
	glColor3fv (dark_g);
	glBegin (GL_LINES);
	//left primary bar
	glVertex2f (-14.0f, -8.0f);
	glVertex2f (-8.0f, -5.0f);
	//right primary bar
	glVertex2f (14.0f, -8.0f);
	glVertex2f (8.0f, -5.0f);
	glEnd ();
	glColor3fv (primary ? bright_g : dark_g);
	//left upper
	OglDrawEllipse (12, GL_POLYGON, 1.5, -7.0f, 1.5, -5.0f, sinCos12);
	//right upper
	OglDrawEllipse (12, GL_POLYGON, 1.5, 7.0f, 1.5, -5.0f, sinCos12);
	glColor3fv ((primary == 2) ? bright_g : dark_g);
	//left lower
	OglDrawEllipse (8, GL_POLYGON, 1.0f, -14.0f, 1.0f, -8.0f, sinCos8);
	//right lower
	OglDrawEllipse (8, GL_POLYGON, 1.0f, 14.0f, 1.0f, -8.0f, sinCos8);
	glEndList ();
	}
//	if (nCanvasHeight>200)
//		glLineWidth (1);

if (!secondary_lh [secondary])
	glCallList (secondary_lh [secondary]);
else {
	secondary_lh [secondary] = glGenLists (1);
	glNewList (secondary_lh [secondary], GL_COMPILE_AND_EXECUTE);
	if (secondary <= 2) {
		//left secondary
		glColor3fv ((secondary == 1) ? bright_g : darker_g);
		OglDrawEllipse (16, GL_LINE_LOOP, 2.0f, -10.0f, 2.0f, -1.0f, sinCos16);
		//right secondary
		glColor3fv ((secondary == 2) ? bright_g : darker_g);
		OglDrawEllipse (16, GL_LINE_LOOP, 2.0f, 10.0f, 2.0f, -1.0f, sinCos16);
		}
	else {
		//bottom/middle secondary
		glColor3fv ((secondary == 4) ? bright_g : darker_g);
		OglDrawEllipse (16, GL_LINE_LOOP, 2.0f, 0.0f, 2.0f, -7.0f, sinCos16);
		}
	glEndList ();
	}
glDisable (GL_LINE_SMOOTH);
glLineWidth (1);
glPopMatrix ();
}

//------------------------------------------------------------------------------
//eof
