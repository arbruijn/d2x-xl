#ifndef __PERLIN_H
#define __PERLIN_H

double Noise1D (int x);			 
double LinearInterpolate (double a, double b, double x);
double CosineInterpolate (double a, double b, double x);
double CubicInterpolate (double v0, double v1, double v2, double v3, double x);
double SmoothedNoise1D (int x);
double InterpolatedNoise1D (double x);
double PerlinNoise1D (double x, double persistence, int octaves, int nSeed);
double Noise2D (int x, int y);
double SmoothedNoise2D (int x, int y);
double InterpolatedNoise2D (double x, double y);
double PerlinNoise2D (double x, double y, double persistence, int octaves, int nSeed);

#endif //__PERLIN_H

