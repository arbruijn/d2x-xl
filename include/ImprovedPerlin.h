#ifndef __improved_perlin_h
#define __improved_perlin_h

#include "perlin.h"

#define PERLIN_RANDOM_SIZE	256

class CImprovedPerlinCore {
	private:
		int	m_random [2 * PERLIN_RANDOM_SIZE];

	public:
		void Initialize (void);
		double Noise (double x);
		double Noise (double x, double y);

	protected:

		inline double Lerp (double t, double a, double b) { return a + t * (b - a); }
		inline double Fade (double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
		inline double Grad (int hash, double x) { return ((hash & 1) == 0) ? x : -x; }
		inline double Grad (int hash, double x, double y) { return (((hash & 2) == 0) ? x : -x) + (((hash & 1) == 0) ? y : -y); }
		double Grad (int hash, double x, double y, double z);

		//virtual double Noise (double x, double y, double z);
	};


class CImprovedPerlin : public CPerlin {
	private:
		CArray<CImprovedPerlinCore>	m_cores;

	public:
		virtual bool Setup (int nNodes, int nOctaves, int nDimensions = 1);

	protected:
		virtual double InterpolatedNoise (double x, int octave) { return m_cores [octave].Noise (x); }
		virtual double InterpolatedNoise (double x, double y, int octave) { return m_cores [octave].Noise (x, y); }
		//virtual double Noise (double x, double y, double z);
	};

#endif //__improved_perlin_h
