//Put these at the top.

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

//#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <string.h>
#include <math.h>

#include "3d.h"
#include "piggy.h"
#include "../../3d/globvars.h"
#include "error.h"
#include "texmap.h"
#include "palette.h"
#include "rle.h"
#include "mono.h"

#include "segment.h"
#include "textures.h"
#include "texmerge.h"
#include "effects.h"
#include "weapon.h"
#include "powerup.h"
#include "polyobj.h"
#include "gamefont.h"
#include "byteswap.h"
#include "glext.h"
#include "u_mem.h"

PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB = NULL;

#include "ogl_init.h"

#if OGL_NEW

//------------------------------------------------------------------------------

ogl_texture TestTex;  //Test Lightmap

GLuint EmptyTexture(int Xsize, int Ysize)			// Create An Empty Texture
{
	//This is code from a tutorial.  Function is not currently used, but will become useful shortly.	

	GLuint txtnumber;						// Texture ID
	//unsigned int* data;						// Stored Data

	// Create Storage Space For Texture Data (128x128x4)
	unsigned int *data = (unsigned int*)d_malloc(((Xsize * Ysize)* 4 * sizeof(unsigned int)));

	ZeroMemory(data,((Xsize * Ysize)* 4 * sizeof(unsigned int)));	// Clear Storage Memory

	glGenTextures(1, &txtnumber);					// Create 1 Texture
	
	glBindTexture(GL_TEXTURE_2D, txtnumber);			// Bind The Texture
	glTexImage2D(GL_TEXTURE_2D, 0, 4, Xsize, Ysize, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data);			// Build Texture Using Information In data
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	d_free(data);							// Release data

	return txtnumber;						// Return The Texture ID
}

//------------------------------------------------------------------------------

void OglInitExtensions(void)
{
//This function initializes the multitexturing stuff.  Pixel Shader stuff should be put here eventually.
glMultiTexCoord2fARB	=  (PFNGLMULTITEXCOORD2FARBPROC)	wglGetProcAddress("glMultiTexCoord2fARB");
glActiveTextureARB =  (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress("glActiveTextureARB");		
glClientActiveTextureARB =  (PFNGLCLIENTACTIVETEXTUREARBPROC) wglGetProcAddress("glClientActiveTextureARB");
}

//------------------------------------------------------------------------------

bool G3DrawTexPolyMulti (
	int nv,
	g3sPoint **pointlist,
	uvl *uvl_list,
	grsBitmap *bmbot,
	grsBitmap *bm,
	int orient)
{
	bool lightmapping=0;
	int c;
	g3sPoint *p, **pp;
	GLint depthFunc;

glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
glDepthFunc(GL_LEQUAL);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
r_tpolyc++;
if (lightmapping) {//lightmapping enabled
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GL_RGBA);
	OglBindBmTex(bmbot);
	if (bm) {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GL_RGBA);
		OglBindBmTex(bm);
		}

	glBegin(GL_TRIANGLE_FAN);
	pp = pointlist;
	cap_tmap_color (uvl_list, nv, bmbot);
	for (c = 0; c < nv; c++, pp++) {
		set_tmap_color (uvl_list + c,c, bmbot);
		glMultiTexCoord2fARB(GL_TEXTURE0_ARB,f2glf(uvl_list[c].u),f2glf(uvl_list[c].v));
		set_texcoord (uvl_list + c, orient, 1);
		glColor3d (1, 1, 1);
		p = *pp;
		glVertex3d (f2glf(p->p3_vec.x), f2glf(p->p3_vec.y), -f2glf(p->p3_vec.z));
		}
	glEnd();
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glBindTexture(GL_TEXTURE_2D,0);
	glDisable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D,0);
	glDisable(GL_TEXTURE_2D);
	}
else {//lightmapping disabled - render old way
	g3sPoint *p;

	if (tmap_drawer_ptr==DrawTexPolyFlat){
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glDisable (GL_TEXTURE_2D);
		glColor4d(0,0,0,1.0-(gameStates.render.grAlpha/(double)NUM_LIGHTING_LEVELS));
		glBegin(GL_TRIANGLE_FAN);
		for (c = 0; c < nv; c++, pointlist++) {
			p = *pointlist;
			glVertex3d (f2glf(p->p3_vec.x), f2glf(p->p3_vec.y), -f2glf(p->p3_vec.z));
			}
		glEnd();
		pointlist=pointlist-4;
		}
	else if (tmap_drawer_ptr == draw_tmap) {
		r_tpolyc++;
		glActiveTextureARB(GL_TEXTURE0_ARB);
		OGL_ENABLE(TEXTURE_2D);
		OglBindBmTex(bmbot);
		OglTexWrap(bmbot->glTexture,GL_REPEAT);
		cap_tmap_color (uvl_list, nv, bmbot);
		glBegin(GL_TRIANGLE_FAN);
		for (c = 0; c < nv; c++, pointlist++) {
			set_tmap_color (uvl_list + c, c, bmbot);
			
			glTexCoord2d (f2glf(uvl_list[c].u), f2glf(uvl_list[c].v));
			p = *pointlist;
			glVertex3d (f2glf(p->p3_vec.x), f2glf(p->p3_vec.y), -f2glf(p->p3_vec.z));
			}
		glEnd();
		pointlist=pointlist-4;
		}
	else {
	#if TRACE	
			con_printf (CON_DEBUG,"G3DrawTexPoly: unhandled tmap_drawer %p\n",tmap_drawer_ptr);
	#endif
		}
	tMapColor.index =
	lightColor.index = 0;
	//G3DrawTexPoly (nv,pointlist,uvl_list,bmbot);//draw the bottom texture first.. could be optimized with multitexturing..
	
	//then draw the second layer
	if (bm) {
		glDepthFunc(GL_LEQUAL);
		r_tpolyc++;
		glActiveTextureARB(GL_TEXTURE0_ARB);
		OGL_ENABLE(TEXTURE_2D);
		OglBindBmTex(bm);
		OglTexWrap(bm->glTexture,GL_REPEAT);
		cap_tmap_color (uvl_list, nv, bm);
		
		glBegin(GL_TRIANGLE_FAN);
			for (c = 0; c < nv; c++, pointlist++) {
			set_tmap_color (uvl_list + c, c, bm);
			set_texcoord (uvl_list + c, orient, 0);
			p = *pointlist;
			glVertex3d (f2glf(p->p3_vec.x), f2glf(p->p3_vec.y), -f2glf(p->p3_vec.z));
			}
		glEnd();
		glDepthFunc(GL_LESS);
		}
	//glActiveTextureARB(GL_TEXTURE0_ARB);
	//glBindTexture(GL_TEXTURE_2D,0);
	//glDisable(GL_TEXTURE_2D);
	}
return 0;
}

//------------------------------------------------------------------------------

bool G3DrawTexPoly (int nv,g3sPoint **pointlist,uvl *uvl_list,grsBitmap *bm)
{
return g4_draw_tmap_2 (nv, pointlist, uvl_list, bm, NULL, 0);
}

//------------------------------------------------------------------------------

bool G3DrawBitMap(vmsVector *pos,fix width,fix height,grsBitmap *bm, int orientation)
{
	vmsVector pv,v1;
	int i;
	r_bitmapc++;
	v1.z=0;

glActiveTextureARB(GL_TEXTURE0_ARB);
glEnable(GL_TEXTURE_2D);
OglBindBmTex(bm);
OglTexWrap(bm->glTexture,GL_CLAMP);

glBegin(GL_QUADS);
glColor3d(1.0,1.0,1.0);
width = FixMul(width,viewInfo.scale.x);
height = FixMul(height,viewInfo.scale.y);
for (i = 0; i < 4; i++) {
	VmVecSub(&v1,pos,&viewInfo.position);
	VmVecRotate(&pv,&v1,&viewInfo.view);
	switch (i) {
		case 0:
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0, 0.0);
			pv.x+=-width;
			pv.y+=height;
			break;
		case 1:
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, bm->glTexture->u, 0.0);
			pv.x+=width;
			pv.y+=height;
			break;
		case 2:
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, bm->glTexture->u, bm->glTexture->v);
			pv.x+=width;
			pv.y+=-height;
			break;
		case 3:
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB,0.0, bm->glTexture->v);
			pv.x+=-width;
			pv.y+=-height;
			break;
		}
	glVertex3d (f2glf(pv.x), f2glf(pv.y), -f2glf(pv.z));
	}
glEnd();

//These next lines are important for later leave these here - Lehm 4/26/05
//glActiveTextureARB(GL_TEXTURE0_ARB);
//glBindTexture(GL_TEXTURE_2D,0);
//glDisable(GL_TEXTURE_2D);
return 0;
} 

//------------------------------------------------------------------------------

bool OglUBitMapMC(int x, int y,grsBitmap *bm,int c)
{
	GLint curFunc;
	GLdouble xo,yo,xf,yf;
	GLdouble u1,u2,v1,v2;

r_ubitmapc++;
x+=grdCurCanv->cv_bitmap.bm_props.x;
y+=grdCurCanv->cv_bitmap.bm_props.y;
xo=x/(double)last_width;
xf=(bm->bm_props.w+x)/(double)last_width;
yo=1.0-y/(double)last_height;
yf=1.0-(bm->bm_props.h+y)/(double)last_height;

glActiveTextureARB(GL_TEXTURE0_ARB);
OglBindBmTex(bm);
OglTexWrap(bm->glTexture,GL_CLAMP);

glEnable(GL_TEXTURE_2D);
glGetIntegerv (GL_DEPTH_FUNC, &curFunc);
glDepthFunc(GL_ALWAYS);
glEnable (GL_ALPHA_TEST);
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

if (bm->bm_props.x==0) {
	u1=0;
	if (bm->bm_props.w==bm->glTexture->w)
		u2=bm->glTexture->u;
	else
		u2=(bm->bm_props.w+bm->bm_props.x)/(double)bm->glTexture->tw;
	}
else {
	u1=bm->bm_props.x/(double)bm->glTexture->tw;
	u2=(bm->bm_props.w+bm->bm_props.x)/(double)bm->glTexture->tw;
	}
if (bm->bm_props.y==0) {
	v1=0;
	if (bm->bm_props.h==bm->glTexture->h)
		v2=bm->glTexture->v;
	else
		v2=(bm->bm_props.h+bm->bm_props.y)/(double)bm->glTexture->th;
	}
else{
	v1=bm->bm_props.y/(double)bm->glTexture->th;
	v2=(bm->bm_props.h+bm->bm_props.y)/(double)bm->glTexture->th;
	}

glBegin(GL_QUADS);
if (c < 0)
	glColor3d (1.0,1.0,1.0);
else
	glColor3d (CPAL2Tr (bmP->bm_palette, c), CPAL2Tg (bmP->bm_palette, c), CPAL2Tb (bmP->bm_palette, c));
glMultiTexCoord2fARB(GL_TEXTURE0_ARB,u1, v1);
glVertex2d(xo, yo);
glMultiTexCoord2fARB(GL_TEXTURE0_ARB,u2, v1);
glVertex2d(xf, yo);
glMultiTexCoord2fARB(GL_TEXTURE0_ARB,u2, v2);
glVertex2d(xf, yf);
glMultiTexCoord2fARB(GL_TEXTURE0_ARB,u1, v2);
glVertex2d(xo, yf);
glEnd();
glDepthFunc(curFunc);
glDisable (GL_ALPHA_TEST);
glDisable (GL_BLEND);
glActiveTextureARB(GL_TEXTURE0_ARB);
glBindTexture(GL_TEXTURE_2D,0);
glDisable(GL_TEXTURE_2D);
return 0;
}

//------------------------------------------------------------------------------

#endif //OGL_NEW