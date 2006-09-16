#include <math.h>

double persistance = 0.5;
double octaves = 6;

//------------------------------------------------------------------------------

double Noise1D (int x)			 
{
double h = (long) pow (x << 13, x);
h = h * (h * (h * 15731 + 789221) + 1376312589);
return 1.0 - ((int) h & 0x7fffffff) / 1073741824.0;    
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

double SmoothedNoise1D (double x)
{
return Noise1D (x) / 2  +  Noise1D (x-1) / 4  +  Noise1D (x+1) / 4;
}

//------------------------------------------------------------------------------

double InterpolatedNoise1D (double x)
{
double xInt, xFrac = modf (x, &xInt);
double v1 = SmoothedNoise1D (xInt);
double v2 = SmoothedNoise1D (xInt + 1);
return CosineInterpolate (v1, v2, xFrac);
}

//------------------------------------------------------------------------------

double PerlinNoise1D (double x)
{
double total = 0;
double p = persistance;
double n = octaves - 1;
double frequency = 1.0, amplitude = 1.0;
int i;
for (i = 0; i < octaves; i++) {
	//frequency = pow (2, i);
	//amplitude = pow (p, i);
	total += InterpolatedNoise1D (x * frequency) * amplitude;
	frequency *= 2.0;
	amplitude *= p;
	}
return total;
}

//------------------------------------------------------------------------------

double Noise2D (double x, double y)
{
return Noise1D (x + y * 57);
}

//------------------------------------------------------------------------------

double SmoothedNoise2D (double x, double y)
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
double v1 = SmoothedNoise2D (xInt, yInt);
double v2 = SmoothedNoise2D (xInt+1, yInt);
double v3 = SmoothedNoise2D (xInt, yInt+1);
double v4 = SmoothedNoise2D (xInt+1, yInt+1);
double i1 = CosineInterpolate(v1, v2, xFrac);
double i2 = CosineInterpolate(v3, v4, xFrac);
return CosineInterpolate (i1, i2, yFrac);
}

//------------------------------------------------------------------------------

double PerlinNoise2D (double x, double y)
{
double total = 0;
double p = persistance;
double frequency = 1.0, amplitude = 1.0;
int i;
for (i = 0; i < octaves; i++) {
	//frequency = pow (2, i);
	//amplitude = pow (p, i);
	total += InterpolatedNoise1D (x * frequency) * amplitude;
	frequency *= 2.0;
	amplitude *= p;
	}
return total;
}
  
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------


