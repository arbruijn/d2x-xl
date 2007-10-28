noinst_LIBRARIES = libarch_ogl.a

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/arch/include -I$(top_srcdir)/main -I$(top_srcdir)/3d

libarch_ogl_a_SOURCES = gr.c ogl.c sdlgl.c pbuffer.c fbuffer.c \
ogl_defs.c ogl_lib.c ogl_texture.c ogl_texcache.c ogl_color.c \
ogl_shader.c ogl_bitmap.c ogl_render.c ogl_fastrender.c ogl_hudstuff.c

EXTRA_DIST = \
internal.h \
wgl.c glx.c
