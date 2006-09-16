//fog.c

#include "fog.h"

//------------------------------------------------------------------------------

tFog fog = {{0.6f,0.58f,0.79f,0.0f}, 0.1f, 0.05f, 200.0f, GL_EXP};

//------------------------------------------------------------------------------

void init_fog (void)
{
glClearColor (fog.color[0], fog.color[1], fog.color[2], 0.0f);
	
#ifdef WIN32
glFogCoordfEXT       = (PFNGLFOGCOORDFEXTPROC) sdlGetProcAddress ("glFogCoordfEXT");
glFogCoordfvEXT      = (PFNGLFOGCOORDFVEXTPROC) sdlGetProcAddress ("glFogCoordfvEXT");
glFogCoorddEXT       = (PFNGLFOGCOORDDEXTPROC) sdlGetProcAddress ("glFogCoorddEXT");
glFogCoorddvEXT      = (PFNGLFOGCOORDDVEXTPROC) sdlGetProcAddress ("glFogCoorddvEXT");
glFogCoordPointerEXT = (PFNGLFOGCOORDPOINTEREXTPROC) sdlGetProcAddress ("glFogCoordPointerEXT");
#endif

// set the fog attributes
glFogf (GL_FOG_START,  fog.nearDist);
glFogf (GL_FOG_END,    fog.farDist);
glFogfv (GL_FOG_COLOR,  fog.color);      
glFogi (GL_FOG_MODE,   fog.mode);
glFogf (GL_FOG_DENSITY,fog.density);
glFogi (GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
// enable the fog
glEnable (GL_FOG);
}

//------------------------------------------------------------------------------

void render_fog (vms_vector *pos)
{
	glBegin( GL_LINES );
		glNormal3f(0,1,0);
		for(i=-20;i<=20;i+=2)
		{
			float fog_c;
			float diff[3];

			diff[0] = camera_pos[0] - i;
			diff[2] = camera_pos[2] + 20;
			diff[1] = camera_pos[1];
			fog_c = 1.3f * sqrt (diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
			fog.colorfog_c*2);
			glVertex3f(i,0,-20);

			diff[0] = camera_pos[0] - i;
			diff[2] = camera_pos[2] + 20;
			diff[1] = camera_pos[1];
			fog_c = 1.3f*sqrt(diff[0]*diff[0]+diff[1]*diff[1]+diff[2]*diff[2]);
			glFogCoordfEXT(fog_c*2);
			glVertex3f(i,0,20);

			diff[0] = camera_pos[0] - i;
			diff[2] = camera_pos[2] + 20;
			diff[1] = camera_pos[1];
			fog_c = 1.3f*sqrt(diff[0]*diff[0]+diff[1]*diff[1]+diff[2]*diff[2]);
			glFogCoordfEXT(fog_c*2);
			glVertex3f(-20,0,i);

			diff[0] = camera_pos[0] - i;
			diff[2] = camera_pos[2] + 20;
			diff[1] = camera_pos[1];
			fog_c = 1.3f*sqrt(diff[0]*diff[0]+diff[1]*diff[1]+diff[2]*diff[2]);
			glFogCoordfEXT(fog_c*2);
			glVertex3f(20,0,i);
		}

	glEnd();
	
	glutSwapBuffers();
}

//------------------------------------------------------------------------------
//eof
