#ifndef __improved_perlin_h
#define __improved_perlin_h

#include "PerlinNoise.h"

#include "carray.h"

#define PERLIN_RANDOM_SIZE	256

class CSimplexNoise : public CPerlinNoise {
	private:
		int m_random [2 * PERLIN_RANDOM_SIZE];

	public:
		virtual void Setup (double amplitude, double persistence, int octaves, int randomize = -1);

	protected:
		virtual double Noise (double x);
		virtual double Noise (double x, double y);
		//virtual double InterpolatedNoise (double x) { return Noise (x); }
		//virtual double InterpolatedNoise (double x, double y) { return Noise (x, y); }
		//virtual double Noise (double x, double y, double z);

	private:
		inline double Lerp (double t, double a, double b) { return a + t * (b - a); }
		inline double Fade (double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
		inline double Grad (int bias, double x) { return ((bias & 1) == 0) ? x : -x; }
		inline double Grad (int bias, double x, double y) { return (((bias & 2) == 0) ? x : -x) + (((bias & 1) == 0) ? y : -y); }
		double Grad (int bias, double x, double y, double z);

	};

#endif //__improved_perlin_h
