#ifndef __PERLIN_H
#define __PERLIN_H

#include "carray.h"

#define CUSTOM_RAND 2

class CPerlin {
	private:
		double m_amplitude;
		double m_persistence;
		int m_octaves;
		int m_nOffset;

	public:
		bool Setup (int nNodes, double amplitude, double persistence, int nOctaves, int nDimensions = 1, int nOffset = -1);
		double ComputeNoise (double x);
		double ComputeNoise (double x, double y);

	protected:	
		double LinearInterpolate (double a, double b, double x);
		double CosineInterpolate (double a, double b, double x);
		double CubicInterpolate (double v0, double v1, double v2, double v3, double x);

		double Noise (int v);			 
		double Noise (int x, int y);

		double SmoothedNoise (int x);
		double SmoothedNoise (int x, int y);

		virtual double InterpolatedNoise (double x, int octave);
		virtual double InterpolatedNoise (double x, double y, int octave);

	};

#endif //__PERLIN_H

