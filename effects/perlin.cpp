#include <math.h>
#include <stdlib.h>

#include "perlin.h"

static long randSeed = 0;

CPerlin perlin;

//------------------------------------------------------------------------------

#define RAND_HALF	((double (RAND_MAX) + 1) / 2)

double CPerlin::Random (void)
{
return (double (rand ()) - RAND_HALF) / RAND_HALF;
}

//------------------------------------------------------------------------------

double CPerlin::Noise1D (double x)
{
return m_noise [int (x) + 1];
}

//------------------------------------------------------------------------------

double CPerlin::LinearInterpolate (double a, double b, double x)
{
return a * (1.0 - x) + b * x;
}

//------------------------------------------------------------------------------

double CPerlin::CosineInterpolate (double a, double b, double x)
{
double ft = x * 3.1415927;	//Pi
double f = (1.0 - cos (ft)) * 0.5;
return  a * (1.0 - f) + b * f;
}

//------------------------------------------------------------------------------

double CPerlin::CubicInterpolate (double v0, double v1, double v2, double v3, double x)
{
double p = (v3 - v2) - (v0 - v1);
double q = (v0 - v1) - p;
double r = v2 - v0;
double bObjectRendered = v1;
double h = x;
bObjectRendered += r * h;
h *= x;
bObjectRendered += q * h;
h *= x;
bObjectRendered += p * h;
return bObjectRendered;
}

//------------------------------------------------------------------------------

double CPerlin::SmoothedNoise1D (double x)
{
return Noise1D (x) / 2  +  Noise1D (x-1) / 4  +  Noise1D (x+1) / 4;
}

//------------------------------------------------------------------------------

double CPerlin::InterpolatedNoise1D (double x)
{
double xInt, xFrac = modf (x, &xInt);
double v1 = SmoothedNoise1D (xInt);
double v2 = SmoothedNoise1D (xInt + 1);
return CosineInterpolate (v1, v2, xFrac);
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

double CPerlin::Noise2D (double x, double y)
{
return (Noise1D (x) + Noise1D (y)) / 2.0;
}

//------------------------------------------------------------------------------

double CPerlin::SmoothedNoise2D (double x, double y)
{
double corners = (Noise2D (x-1, y-1) + Noise2D (x+1, y-1) + Noise2D (x-1, y+1) + Noise2D (x+1, y+1)) / 16;
double sides = (Noise2D (x-1, y) + Noise2D (x+1, y) + Noise2D (x, y-1) + Noise2D (x, y+1)) / 8;
double center = Noise2D (x, y) / 4;
return corners + sides + center;
}

//------------------------------------------------------------------------------

double CPerlin::InterpolatedNoise2D (double x, double y)
{
double xInt, yInt,
		 xFrac = modf (x, &xInt),
		 yFrac = modf (x, &yInt);
double v1 = SmoothedNoise2D ((long) xInt, (long) yInt);
double v2 = SmoothedNoise2D ((long) xInt+1, (long) yInt);
double v3 = SmoothedNoise2D ((long) xInt, (long) yInt+1);
double v4 = SmoothedNoise2D ((long) xInt+1, (long) yInt+1);
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

bool CPerlin::Setup (int nNodes, int nDimensions)
{
nNodes += 2;
m_nNodes = nNodes;
nNodes *= nDimensions;
if (m_noise.Buffer () && (int (m_noise.Length ()) < nNodes))
	m_noise.Destroy ();
if (!(m_noise.Buffer () || m_noise.Create (nNodes)))
	return false;
for (int i = 0; i < m_nNodes; i++)
	m_noise [i] = Random ();
return true;
}

//------------------------------------------------------------------------------


