#ifndef __improved_perlin_h
#define __improved_perlin_h

#include "perlin.h"

#include "carray.h"

#define PERLIN_RANDOM_SIZE	256

class CImprovedPerlinCore {
	private:
		int m_random [2 * PERLIN_RANDOM_SIZE];

	public:
		void Initialize (void);
		double Noise (double x);
		double Noise (double x, double y);

	protected:

		inline double Lerp (double t, double a, double b) { return a + t * (b - a); }
		inline double Fade (double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
		inline double Grad (int bias, double x) { return ((bias & 1) == 0) ? x : -x; }
		inline double Grad (int bias, double x, double y) { return (((bias & 2) == 0) ? x : -x) + (((bias & 1) == 0) ? y : -y); }
		double Grad (int bias, double x, double y, double z);

		//virtual double Noise (double x, double y, double z);
	};


class CImprovedPerlin : public CPerlin {
	private:
		CStaticArray<CImprovedPerlinCore, 1>	m_cores;

	public:
		virtual void Setup (double amplitude, double persistence, int octaves, int randomize = -1);

	protected:
		virtual double InterpolatedNoise (double x) { return m_cores [0].Noise (x); }
		virtual double InterpolatedNoise (double x, double y) { return m_cores [0].Noise (x, y); }
		//virtual double Noise (double x, double y, double z);
	};

#endif //__improved_perlin_h
