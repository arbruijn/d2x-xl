// Palette functions

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "hudmsgs.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"

#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_texcache.h"
#include "sdlgl.h"

#include "screens.h"

#include "strutil.h"
#include "mono.h"
#include "args.h"
#include "key.h"
#include "cockpit.h"
#include "rendermine.h"
#include "particles.h"
#include "glare.h"
#include "menu.h"

//------------------------------------------------------------------------------

void CPaletteManager::RenderEffect (void)
{
#if 1
if (m_data.nSuspended)
	return;
if (!m_data.bDoEffect)
	return;
ogl.SetBlending (true);
ogl.SetBlendMode (OGL_BLEND_ADD);
UpdateEffect ();
glColor3fv (reinterpret_cast<GLfloat*> (&m_data.flash));
bool bDepthTest = ogl.GetDepthTest ();
ogl.SetDepthTest (false);
ogl.SetTexturing (false);
ogl.RenderScreenQuad ();
ogl.SetDepthTest (bDepthTest);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
#endif
}

//------------------------------------------------------------------------------

