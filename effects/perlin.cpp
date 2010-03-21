#include <math.h>
#include <stdlib.h>

#include "descent.h"
#include "maths.h"
#include "perlin.h"

static long randSeed = 0;

CPerlin perlinX [MAX_THREADS], perlinY [MAX_THREADS];

//------------------------------------------------------------------------------

#define RAND_HALF	((double (RAND_MAX) + 1) / 2)

#if CUSTOM_RAND

inline double CPerlin::Random (int x)
{
x = (x << 13) ^ x;
return 1.0 - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;    
}

#else

inline double CPerlin::Random (void)
{
return (double (rand ()) - RAND_HALF) / RAND_HALF;
}

#endif

//------------------------------------------------------------------------------

inline double CPerlin::Noise1D (int x)
{
#if CUSTOM_RAND > 1
return Random (x);
#else
return m_noise [x + 1];
#endif
}

//------------------------------------------------------------------------------

double CPerlin::LinearInterpolate (double a, double b, double x)
{
return a * (1.0 - x) + b * x;
}

//------------------------------------------------------------------------------

double CPerlin::CosineInterpolate (double a, double b, double x)
{
double f = (1.0 - cos (x * Pi)) * 0.5;
return  a * (1.0 - f) + b * f;
}

//------------------------------------------------------------------------------

double CPerlin::CubicInterpolate (double v0, double v1, double v2, double v3, double x)
{
#if DBG
double p = (v3 - v2) - (v0 - v1);
double q = (v0 - v1) - p;
double r = v2 - v0;
double x2 = x * x;
return v1 + r * x + q * x2 + p * x2 * x;
#else
double p = (v3 - v2) - (v0 - v1);
double x2 = x * x;
return v1 + (v2 - v0) * x + (v0 - v1 - p) * x2 + p * x2 * x;
#endif
}

//------------------------------------------------------------------------------

double CPerlin::SmoothedNoise1D (int x)
{
return Noise1D (x) / 2  +  Noise1D (x-1) / 4  +  Noise1D (x+1) / 4;
}

//------------------------------------------------------------------------------

double CPerlin::InterpolatedNoise1D (double x)
{
int xInt = int (x);
double v1 = SmoothedNoise1D (xInt);
double v2 = SmoothedNoise1D (xInt + 1);
return CosineInterpolate (v1, v2, x - xInt);
}

//------------------------------------------------------------------------------

double CPerlin::PerlinNoise1D (double x, double persistence, long octaves)
{
double total = 0, frequency = 1.0, amplitude = 1.0;
for (int i = 0; i < octaves; i++) {
	total += InterpolatedNoise1D (x * frequency) * amplitude;
	frequency *= 2.0;
	amplitude *= persistence;
	}
return total;
}

//------------------------------------------------------------------------------

double CPerlin::Noise2D (int x, int y)
{
return (Noise1D (x) + Noise1D (y)) / 2.0;
}

//------------------------------------------------------------------------------

double CPerlin::SmoothedNoise2D (int x, int y)
{
double corners = (Noise2D (x-1, y-1) + Noise2D (x+1, y-1) + Noise2D (x-1, y+1) + Noise2D (x+1, y+1)) / 16;
double sides = (Noise2D (x-1, y) + Noise2D (x+1, y) + Noise2D (x, y-1) + Noise2D (x, y+1)) / 8;
double center = Noise2D (x, y) / 4;
return corners + sides + center;
}

//------------------------------------------------------------------------------

double CPerlin::InterpolatedNoise2D (double x, double y)
{
#if 1
int xInt = int (x);
double xFrac = x - xInt;
int yInt = int (y);
double yFrac = y - yInt;
#else
double xInt, yInt,
		 xFrac = modf (x, &xInt),
		 yFrac = modf (x, &yInt);
#endif
double v1 = SmoothedNoise2D (xInt, yInt);
double v2 = SmoothedNoise2D (xInt+1, yInt);
double v3 = SmoothedNoise2D (xInt, yInt+1);
double v4 = SmoothedNoise2D (xInt+1, yInt+1);
double i1 = CosineInterpolate (v1, v2, xFrac);
double i2 = CosineInterpolate (v3, v4, xFrac);
return CosineInterpolate (i1, i2, yFrac);
}

//------------------------------------------------------------------------------

double CPerlin::PerlinNoise2D (double x, double y, double persistence, long octaves)
{
double total = 0, frequency = 1.0, amplitude = 1.0;
for (int i = 0; i < octaves; i++) {
	total += InterpolatedNoise1D (x * frequency) * amplitude;
	frequency *= 2.0;
	amplitude *= persistence;
	}
return total;
}

//------------------------------------------------------------------------------

bool CPerlin::Setup (int nNodes, int nOctaves, int nDimensions)
{
#if CUSTOM_RAND < 2
m_nNodes = nNodes << nOctaves;
m_nNodes += 2;
m_nNodes *= nDimensions;
if (m_noise.Buffer () && (int (m_noise.Length ()) < m_nNodes))
	m_noise.Destroy ();
if (!(m_noise.Buffer () || m_noise.Create (m_nNodes)))
	return false;
#	if CUSTOM_RAND
int x = rand ();
for (int i = 0; i < m_nNodes + 1; i++, x++) 
	m_noise [i] = Random (x);
#	else
for (int i = 0; i < m_nNodes + 1; i++) 
	m_noise [i] = Random ();
#	endif
#endif
return true;
}

//------------------------------------------------------------------------------


