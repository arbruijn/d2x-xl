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
#include "renderlib.h"
#include "cockpit.h"

//------------------------------------------------------------------------------

void OglDrawMouseIndicator (void)
{
	float 	scale = float (screen.Width ()) / float (screen.Height ());

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
		if (bmpDeadzone->Bind (1)) 
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
//eof
