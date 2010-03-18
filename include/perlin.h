#ifndef __PERLIN_H
#define __PERLIN_H

#include "carray.h"

class CPerlin {
	private:
		CArray<double>	m_noise;
		int m_nNodes;

	public:
		double Random (void);
		double Noise1D (double x);			 
		double LinearInterpolate (double a, double b, double x);
		double CosineInterpolate (double a, double b, double x);
		double CubicInterpolate (double v0, double v1, double v2, double v3, double x);
		double SmoothedNoise1D (double x);
		double InterpolatedNoise1D (double x);
		double PerlinNoise1D (double x, double persistence, long octaves);
		double Noise2D (double x, double y);
		double SmoothedNoise2D (double x, double y);
		double InterpolatedNoise2D (double x, double y);
		double PerlinNoise2D (double x, double y, double persistence, long octaves);

		bool Setup (int nNodes, int nOctaves, int nDimensions = 1);
	};

extern CPerlin perlinX, perlinY;

#endif //__PERLIN_H

