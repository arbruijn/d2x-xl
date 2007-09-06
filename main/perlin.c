#include <math.h>
#include <stdlib.h>

#include "perlin.h"

static int randSeed = 0;

//------------------------------------------------------------------------------

double Noise1D (int x)			 
{
#if 1
srand (randSeed + x);
x = rand ();
return (double) (((RAND_MAX + 1) / 2) - x) / (double) ((RAND_MAX + 1) / 2);
#else
double h = (int) pow (x << 13, x);
h = h * (h * (h * 15731 + 789221) + 1376312589);
return 1.0 - ((int) h & 0x7fffffff) / 1073741824.0;    
#endif
}

//------------------------------------------------------------------------------

double LinearInterpolate (double a, double b, double x)
{
return a * (1.0 - x) + b * x;
}

//------------------------------------------------------------------------------

double CosineInterpolate (double a, double b, double x)
{
double ft = x * 3.1415927;	//Pi
double f = (1 - cos (ft)) * 0.5;
return  a * (1.0 - f) + b * f;
}
 
//------------------------------------------------------------------------------

double CubicInterpolate (double v0, double v1, double v2, double v3, double x)
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

double SmoothedNoise1D (int x)
{
return Noise1D (x) / 2  +  Noise1D (x-1) / 4  +  Noise1D (x+1) / 4;
}

//------------------------------------------------------------------------------

double InterpolatedNoise1D (double x)
{
double xInt, xFrac = modf (x, &xInt);
double v1 = SmoothedNoise1D ((int) xInt);
double v2 = SmoothedNoise1D ((int) xInt + 1);
return CosineInterpolate (v1, v2, xFrac);
}

//------------------------------------------------------------------------------

double PerlinNoise1D (double x, double persistence, int octaves, int seed)
{
double total = 0, frequency = 1.0, amplitude = 1.0;
int i;
randSeed = seed;
for (i = 0; i < octaves; i++) {
	total += InterpolatedNoise1D (x * frequency) * amplitude;
	frequency *= 2.0;
	amplitude *= persistence;
	}
return total;
}

//------------------------------------------------------------------------------

double Noise2D (int x, int y)
{
return Noise1D (x + y * 57);
}

//------------------------------------------------------------------------------

double SmoothedNoise2D (int x, int y)
{
double corners = (Noise2D (x-1, y-1) + Noise2D (x+1, y-1) + Noise2D (x-1, y+1) + Noise2D (x+1, y+1)) / 16;
double sides = (Noise2D (x-1, y) + Noise2D (x+1, y) + Noise2D (x, y-1) + Noise2D (x, y+1)) / 8;
double center = Noise2D (x, y) / 4;
return corners + sides + center;
}

//------------------------------------------------------------------------------

double InterpolatedNoise2D (double x, double y)
{
double xInt, yInt,
		 xFrac = modf (x, &xInt),
		 yFrac = modf (x, &yInt);
double v1 = SmoothedNoise2D ((int) xInt, (int) yInt);
double v2 = SmoothedNoise2D ((int) xInt+1, (int) yInt);
double v3 = SmoothedNoise2D ((int) xInt, (int) yInt+1);
double v4 = SmoothedNoise2D ((int) xInt+1, (int) yInt+1);
double i1 = CosineInterpolate (v1, v2, xFrac);
double i2 = CosineInterpolate (v3, v4, xFrac);
return CosineInterpolate (i1, i2, yFrac);
}

//------------------------------------------------------------------------------

double PerlinNoise2D (double x, double y, double persistence, int octaves, int nSeed)
{
double total = 0, frequency = 1.0, amplitude = 1.0;
int i;
randSeed = nSeed;
for (i = 0; i < octaves; i++) {
	total += InterpolatedNoise1D (x * frequency) * amplitude;
	frequency *= 2.0;
	amplitude *= persistence;
	}
return total;
}
  
//------------------------------------------------------------------------------


