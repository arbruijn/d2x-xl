#ifndef __PERLIN_H
#define __PERLIN_H

#include "carray.h"

#define CUSTOM_RAND 0

class CPerlin {
	private:
		CArray<double>	m_random;
		CArray<ubyte>	m_valid;
		int m_nNodes;
		int m_nValues;
		double m_scale;

	public:
		bool Setup (int nNodes, int nOctaves, int nDimensions = 1);
		double ComputeNoise (double x, double persistence, long octaves);
		double ComputeNoise (double x, double y, double persistence, long octaves);

	protected:	
#if CUSTOM_RAND
		inline double Random (int v);
#else
		inline double Random (void);
#endif
		double LinearInterpolate (double a, double b, double x);
		double CosineInterpolate (double a, double b, double x);
		double CubicInterpolate (double v0, double v1, double v2, double v3, double x);

		double Noise (double x);			 
		double Noise (double x, double y);

		double SmoothedNoise (int x);
		double SmoothedNoise (int x, int y);

		virtual double InterpolatedNoise (double x, int octave);
		virtual double InterpolatedNoise (double x, double y, int octave);

	};

#endif //__PERLIN_H

