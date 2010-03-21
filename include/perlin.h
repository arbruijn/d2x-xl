#ifndef __PERLIN_H
#define __PERLIN_H

#include "carray.h"

#define CUSTOM_RAND 0

class CPerlin {
	private:
		CArray<double>	m_noise;
		int m_nNodes;

	public:
#if CUSTOM_RAND
		inline double Random (int x);
#else
		inline double Random (void);
#endif
		inline double Noise1D (int x);			 
		double LinearInterpolate (double a, double b, double x);
		double CosineInterpolate (double a, double b, double x);
		double CubicInterpolate (double v0, double v1, double v2, double v3, double x);
		double SmoothedNoise1D (int x);
		double InterpolatedNoise1D (double x);
		double PerlinNoise1D (double x, double persistence, long octaves);
		double Noise2D (int x, int y);
		double SmoothedNoise2D (int x, int y);
		double InterpolatedNoise2D (double x, double y);
		double PerlinNoise2D (double x, double y, double persistence, long octaves);

		bool Setup (int nNodes, int nOctaves, int nDimensions = 1);
	};

extern CPerlin perlinX [], perlinY [];

#endif //__PERLIN_H

