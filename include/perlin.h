#ifndef __PERLIN_H
#define __PERLIN_H

class CPerlin {
	private:
		int m_randomize;

	protected:	
		double m_amplitude;
		double m_persistence;
		int m_octaves;

	public:
		virtual void Setup (double amplitude, double persistence, int octaves, int randomize = -1);
		double ComputeNoise (double x);
		double ComputeNoise (double x, double y);

	protected:	
		double LinearInterpolate (double a, double b, double x);
		double CosineInterpolate (double a, double b, double x);
		double CubicInterpolate (double v0, double v1, double v2, double v3, double x);

		virtual double Noise (double v);			 
		virtual double Noise (double x, double y);

		double SmoothedNoise (double x);
		double SmoothedNoise (double x, double y);

		virtual double InterpolatedNoise (double x);
		virtual double InterpolatedNoise (double x, double y);

		inline int FastFloor (double n) { return (n > 0) ? (int) n : (int) n - 1; }
	};

#endif //__PERLIN_H

