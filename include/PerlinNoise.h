#ifndef __PERLINNOISE_H
#define __PERLINNOISE_H

class CPerlinNoise {
	private:
		int32_t m_randomize;

	protected:	
		double m_amplitude;
		double m_persistence;
		int32_t m_octaves;

	public:
		virtual void Setup (double amplitude, double persistence, int32_t octaves, int32_t randomize = -1);
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

		inline int32_t FastFloor (double n) { return (n > 0) ? (int32_t) n : (int32_t) n - 1; }
	};

#endif //__PERLINNOISE_H

