
#include <math.h>
#include <stdlib.h>

#include "ImprovedPerlin.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CImprovedPerlinCore::Initialize (void) 
{
for (int i = 0; i < PERLIN_RANDOM_SIZE; i++)
	m_random [i + PERLIN_RANDOM_SIZE] = m_random [i] = (int) (rand () % 256);
}
      
//------------------------------------------------------------------------------

double CImprovedPerlinCore::Noise (double x) 
{
int xi = int (floor (x)) & 255;
x -= floor (x);
double u = Fade (x);
#if DBG
double g1 = Grad (m_random [xi], x);
double g2 = Grad (m_random [xi+1], x - 1);
double l = Lerp (u, g1, g2);
return l;
#else
return Lerp (u, Grad (m_random [xi], x), Grad (m_random [xi+1], x - 1));
#endif
}

//------------------------------------------------------------------------------

double CImprovedPerlinCore::Noise (double x, double y) 
{
int xi = (int) floor (x);
int yi = (int) floor (y);
x -= (double) xi;
y -= (double) yi;
xi &= 255;
yi &= 255;
double u = Fade (x);
double v = Fade (y);
int A = m_random [xi] + yi;
int B = m_random [xi + 1] + yi;
return Lerp (v, Lerp (u, Grad (m_random [A], x, y), Grad (m_random [B], x - 1, y)), Lerp (u, Grad (m_random [A + 1], x, y - 1), Grad (m_random [B + 1], x - 1, y - 1)));
}

//------------------------------------------------------------------------------
#if 0
double CImprovedPerlinCore::Noise (double x, double y, double z) 
{
int X = (int) floor (x) & 255;
int Y = (int) floor (y) & 255;
int Z = (int) floor (z) & 255;
x -= (double) floor (x);
y -= (double) floor (y);
z -= (double) floor (z);
double u = Fade (x);
double v = Fade (y);
double w = Fade (z);
int A = m_random [X] + Y;
int AA = m_random [A] + Z;
int AB = m_random [A + 1] + Z;
int B = m_random [X + 1] + Y;
int BA = m_random [B] + Z;
int BB = m_random [B + 1] + Z;
return Lerp (w, Lerp (v, Lerp (u, Grad (m_random [AA], x, y, z), Grad (m_random [BA], x - 1, y, z)),
							Lerp (u, Grad (m_random [AB], x, y - 1, z), Grad (m_random [BB], x - 1, y - 1, z))),
					Lerp (v, Lerp (u, Grad (m_random [AA + 1], x, y, z - 1), Grad (m_random [BA + 1], x - 1, y, z - 1)),
							Lerp (u, Grad (m_random [AB + 1], x, y - 1, z - 1), Grad (m_random [BB + 1], x - 1, y - 1, z - 1))));
}
#endif
//------------------------------------------------------------------------------

double CImprovedPerlinCore::Grad (int bias, double x, double y, double z) 
{
int h = bias & 15;
double u = (h < 8) ? x : y;
double v = (h < 4) ? y : ((h == 12) || (h == 14)) ? x : z;
return (((h & 1) == 0) ? u : -u) + (((h & 2) == 0) ? v : -v);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool CImprovedPerlin::Setup (int nNodes, int nOctaves, int nDimensions)
{
if (m_cores.Buffer () && (int (m_cores.Length ()) < nOctaves))
	m_cores.Destroy ();
if (!(m_cores.Buffer () || m_cores.Create (nOctaves)))
	return false;
for (int i = 0; i < nOctaves; i++)
	m_cores [i].Initialize ();
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
