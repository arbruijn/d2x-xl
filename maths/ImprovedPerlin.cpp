
#include <math.h>
#include <stdlib.h>

#include "ImprovedPerlin.h"

//------------------------------------------------------------------------------

bool CImprovedPerlin::Setup (int nNodes, int nOctaves, int nDimensions)
{
Initialize ();
return true;
}

//------------------------------------------------------------------------------

void CImprovedPerlin::Initialize (void) 
{
for (int i = 0; i < PERLIN_RANDOM_SIZE; i++)
	source [i] = (unsigned char) (rand () % 256);
for (int i = 0; i < PERLIN_RANDOM_SIZE; i++)
	values [i + PERLIN_RANDOM_SIZE] = values [i] = int (source [i]);
}
      
//------------------------------------------------------------------------------

double CImprovedPerlin::Noise (double x) 
{
int X = (int) floor (x) & 255;
x -= (double) floor (x);
double u = Fade (x);
return Lerp (u, Grad (values [X], x), Grad (values [X+1], x - 1));
}

//------------------------------------------------------------------------------

double CImprovedPerlin::Noise (double x, double y) 
{
int X = (int) floor (x) & 255;
int Y = (int) floor (y) & 255;
x -= (double) floor (x);
y -= (double) floor (y);
double u = Fade(x);
double v = Fade(y);
int A = values [X] + Y;
int B = values [X + 1] + Y;
return Lerp (v, Lerp (u, Grad (values [A], x, y), Grad (values [B], x - 1, y)), Lerp (u, Grad (values [A + 1], x, y - 1), Grad (values [B + 1], x - 1, y - 1)));
}

//------------------------------------------------------------------------------

double CImprovedPerlin::Noise (double x, double y, double z) 
{
int X = (int) floor (x) & 255;
int Y = (int) floor (y) & 255;
int Z = (int) floor (z) & 255;
x -= (double) floor (x);
y -= (double) floor (y);
z -= (double) floor (z);
double u = Fade(x);
double v = Fade(y);
double w = Fade(z);
int A = values [X] + Y;
int AA = values [A] + Z;
int AB = values [A + 1] + Z;
int B = values [X + 1] + Y;
int BA = values [B] + Z;
int BB = values [B + 1] + Z;
return Lerp (w, Lerp (v, Lerp (u, Grad (values [AA], x, y, z), Grad (values [BA], x - 1, y, z)),
							Lerp (u, Grad (values [AB], x, y - 1, z), Grad (values [BB], x - 1, y - 1, z))),
					Lerp (v, Lerp (u, Grad (values [AA + 1], x, y, z - 1), Grad (values [BA + 1], x - 1, y, z - 1)),
							Lerp (u, Grad (values [AB + 1], x, y - 1, z - 1), Grad (values [BB + 1], x - 1, y - 1, z - 1))));
}

//------------------------------------------------------------------------------

double CImprovedPerlin::Grad (int hash, double x, double y, double z) 
{
int h = hash & 15;
double u = (h < 8) ? x : y;
double v = (h < 4) ? y : ((h == 12) || (h == 14)) ? x : z;
return (((h & 1) == 0) ? u : -u) + (((h & 2) == 0) ? v : -v);
}

//------------------------------------------------------------------------------
