#ifndef __PERLIN_H
#define __PERLIN_H

#include "carray.h"

#define CUSTOM_RAND 0


class CPerlin {
	private:
		CArray<double>	m_random;
		CArray<ubyte>	m_valid;
		int m_nNodes;

	public:
		bool Setup (int nNodes, int nOctaves, int nDimensions = 1);
		double Noise (double x, double persistence, long octaves);
		double Noise (double x, double y, double persistence, long octaves);

	protected:	
		void Initialize (void);
#if CUSTOM_RAND
		inline double Random (int x);
#else
		inline double Random (void);
#endif
		double LinearInterpolate (double a, double b, double x);
		double CosineInterpolate (double a, double b, double x);
		double CubicInterpolate (double v0, double v1, double v2, double v3, double x);

		double Noise (double x);			 
		double SmoothedNoise (int x);
		double InterpolatedNoise (double x);
		double Noise (double x, double y);
		double SmoothedNoise (int x, int y);
		double InterpolatedNoise (double x, double y);

	};

#endif //__PERLIN_H

