#ifndef __PERLIN_H
#define __PERLIN_H

double Noise1D (long x);			 
double LinearInterpolate (double a, double b, double x);
double CosineInterpolate (double a, double b, double x);
double CubicInterpolate (double v0, double v1, double v2, double v3, double x);
double SmoothedNoise1D (long x);
double InterpolatedNoise1D (double x);
double PerlinNoise1D (double x, double persistence, long octaves, long nSeed);
double Noise2D (long x, long y);
double SmoothedNoise2D (long x, long y);
double InterpolatedNoise2D (double x, double y);
double PerlinNoise2D (double x, double y, double persistence, long octaves, long nSeed);

#endif //__PERLIN_H

