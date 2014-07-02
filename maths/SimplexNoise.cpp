
#include <math.h>
#include <stdlib.h>

#include "SimplexNoise.h"

#if 0
static int32_t permutation [] = {
	151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
	140,36,103,30,69,142,8,99,37,240,21,10,23,190, 6,148,
	247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
	57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,
	175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,
	229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,
	208, 89,18,169,200,196,135,130,116,188,159,86,164,100
	,109,198,173,186, 3,64,52,217,226,250,124,123,5,202,38,
	147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,
	182,189,28,42,223,183,170,213,119,248,152, 2,44,154,163, 
	70,221,153,101,155,167, 43,172,9,129,22,39,253, 19,98,
	108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,
	235,249,14,239,107,49,192,214, 31,181,199,106,157,184, 
	84,204,176,115,121,50,45,127, 4,150,254,138,236,205,93,
	222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
	};
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

double CSimplexNoise::Noise (double v) 
{
int32_t i = int32_t (FastFloor (v)) & 255;
v -= FastFloor (v);
double u = Fade (v);
#if DBG
double a = Grad (m_random [i], v);
double b = Grad (m_random [i+1], v - 1);
double l = Lerp (u, a, b);
return l;
#else
return Lerp (u, Grad (m_random [i], v), Grad (m_random [i+1], v - 1));
#endif
}

//------------------------------------------------------------------------------

double CSimplexNoise::Noise (double x, double y) 
{
int32_t xi = (int32_t) FastFloor (x);
int32_t yi = (int32_t) FastFloor (y);
x -= (double) xi;
y -= (double) yi;
xi &= 255;
yi &= 255;
double u = Fade (x);
double v = Fade (y);
int32_t A = m_random [xi] + yi;
int32_t B = m_random [xi + 1] + yi;
return Lerp (v, Lerp (u, Grad (m_random [A], x, y), Grad (m_random [B], x - 1, y)), Lerp (u, Grad (m_random [A + 1], x, y - 1), Grad (m_random [B + 1], x - 1, y - 1)));
}

//------------------------------------------------------------------------------
#if 0
double CSimplexNoise::Noise (double x, double y, double z) 
{
int32_t X = (int32_t) FastFloor (x) & 255;
int32_t Y = (int32_t) FastFloor (y) & 255;
int32_t Z = (int32_t) FastFloor (z) & 255;
x -= (double) FastFloor (x);
y -= (double) FastFloor (y);
z -= (double) FastFloor (z);
double u = Fade (x);
double v = Fade (y);
double w = Fade (z);
int32_t A = m_random [X] + Y;
int32_t AA = m_random [A] + Z;
int32_t AB = m_random [A + 1] + Z;
int32_t B = m_random [X + 1] + Y;
int32_t BA = m_random [B] + Z;
int32_t BB = m_random [B + 1] + Z;
return Lerp (w, Lerp (v, Lerp (u, Grad (m_random [AA], x, y, z), Grad (m_random [BA], x - 1, y, z)),
							 Lerp (u, Grad (m_random [AB], x, y - 1, z), Grad (m_random [BB], x - 1, y - 1, z))),
					Lerp (v, Lerp (u, Grad (m_random [AA + 1], x, y, z - 1), Grad (m_random [BA + 1], x - 1, y, z - 1)),
							Lerp (u, Grad (m_random [AB + 1], x, y - 1, z - 1), Grad (m_random [BB + 1], x - 1, y - 1, z - 1))));
}
#endif
//------------------------------------------------------------------------------

double CSimplexNoise::Grad (int32_t bias, double x, double y, double z) 
{
int32_t h = bias & 15;
double u = (h < 8) ? x : y;
double v = (h < 4) ? y : ((h == 12) || (h == 14)) ? x : z;
return (((h & 1) == 0) ? u : -u) + (((h & 2) == 0) ? v : -v);
}

//------------------------------------------------------------------------------

void CSimplexNoise::Setup (double amplitude, double persistence, int32_t octaves, int32_t randomize)
{
CPerlinNoise::Setup (amplitude * 2.0, persistence, octaves, 0);
#if 0
memcpy (m_random, permutation, PERLIN_RANDOM_SIZE * sizeof (m_random [0]));
memcpy (m_random + PERLIN_RANDOM_SIZE, permutation, PERLIN_RANDOM_SIZE * sizeof (m_random [0]));
#else
int32_t i, permutation [PERLIN_RANDOM_SIZE];

for (i = 0; i < PERLIN_RANDOM_SIZE; i++)
	permutation [i] = i;
int32_t l = PERLIN_RANDOM_SIZE;

int32_t* p = m_random + PERLIN_RANDOM_SIZE;
memcpy (p, permutation, sizeof (permutation));
for (int32_t i = 0; i < PERLIN_RANDOM_SIZE; i++) {
	int32_t j = (int32_t) (rand () % l);
	m_random [i] = p [j];
	if (j < --l)
		p [j] = p [l];
	}
memcpy (p, m_random, sizeof (permutation));
#endif
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
