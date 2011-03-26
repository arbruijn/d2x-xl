#ifndef __improved_perlin_h
#define __improved_perlin_h

#include "perlin.h"

#define PERLIN_RANDOM_SIZE	256

class CImprovedPerlin : public CPerlin {
	private:
		int	m_random [2 * PERLIN_RANDOM_SIZE];

	public:
		virtual bool Setup (int nNodes, int nOctaves, int nDimensions = 1);

	protected:
		virtual void Initialize (void);

		inline double Lerp (double t, double a, double b) { return a + t * (b - a); }
		inline double Fade (double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
		inline double Grad (int hash, double x) { return ((hash & 1) == 0) ? x : -x; }
		inline double Grad (int hash, double x, double y) { return (((hash & 2) == 0) ? x : -x) + (((hash & 1) == 0) ? y : -y); }
		double Grad (int hash, double x, double y, double z);

		virtual double Noise (double x);
		virtual double Noise (double x, double y);
		//virtual double Noise (double x, double y, double z);
	};

#endif //__improved_perlin_h
