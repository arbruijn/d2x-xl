#include "oglmatrix.h"

//------------------------------------------------------------------------------

COGLMatrix COGLMatrix::Inverse (void)
{
	COGLMatrix im;

im [0] =  m_data [5] * m_data [10] * m_data [15] - m_data [5] * m_data [11] * m_data [14] - m_data [9] * m_data [6] * m_data [15] + m_data [9] * m_data [7] * m_data [14] + m_data [13] * m_data [6] * m_data [11] - m_data [13] * m_data [7] * m_data [10];
im [4] = -m_data [4] * m_data [10] * m_data [15] + m_data [4] * m_data [11] * m_data [14] + m_data [8] * m_data [6] * m_data [15] - m_data [8] * m_data [7] * m_data [14] - m_data [12] * m_data [6] * m_data [11] + m_data [12] * m_data [7] * m_data [10];
im [8] =  m_data [4] * m_data [9] * m_data [15] - m_data [4] * m_data [11] * m_data [13] - m_data [8] * m_data [5] * m_data [15] + m_data [8] * m_data [7] * m_data [13] + m_data [12] * m_data [5] * m_data [11] - m_data [12] * m_data [7] * m_data [9];
im [12] = -m_data [4] * m_data [9] * m_data [14] + m_data [4] * m_data [10] * m_data [13] + m_data [8] * m_data [5] * m_data [14] - m_data [8] * m_data [6] * m_data [13] - m_data [12] * m_data [5] * m_data [10] + m_data [12] * m_data [6] * m_data [9];
im [1] =  -m_data [1] * m_data [10] * m_data [15] + m_data [1] * m_data [11] * m_data [14] + m_data [9] * m_data [2] * m_data [15] - m_data [9] * m_data [3] * m_data [14] - m_data [13] * m_data [2] * m_data [11] + m_data [13] * m_data [3] * m_data [10];
im [5] =   m_data [0] * m_data [10] * m_data [15] - m_data [0] * m_data [11] * m_data [14] - m_data [8] * m_data [2] * m_data [15] + m_data [8] * m_data [3] * m_data [14] + m_data [12] * m_data [2] * m_data [11] - m_data [12] * m_data [3] * m_data [10];
im [9] =  -m_data [0] * m_data [9] * m_data [15] + m_data [0] * m_data [11] * m_data [13] + m_data [8] * m_data [1] * m_data [15] - m_data [8] * m_data [3] * m_data [13] - m_data [12] * m_data [1] * m_data [11] + m_data [12] * m_data [3] * m_data [9];
im [13] =  m_data [0] * m_data [9] * m_data [14] - m_data [0] * m_data [10] * m_data [13] - m_data [8] * m_data [1] * m_data [14] + m_data [8] * m_data [2] * m_data [13] + m_data [12] * m_data [1] * m_data [10] - m_data [12] * m_data [2] * m_data [9];
im [2] =   m_data [1] * m_data [6] * m_data [15] - m_data [1] * m_data [7] * m_data [14] - m_data [5] * m_data [2] * m_data [15] + m_data [5] * m_data [3] * m_data [14] + m_data [13] * m_data [2] * m_data [7] - m_data [13] * m_data [3] * m_data [6];
im [6] =  -m_data [0] * m_data [6] * m_data [15] + m_data [0] * m_data [7] * m_data [14] + m_data [4] * m_data [2] * m_data [15] - m_data [4] * m_data [3] * m_data [14] - m_data [12] * m_data [2] * m_data [7] + m_data [12] * m_data [3] * m_data [6];
im [10] =  m_data [0] * m_data [5] * m_data [15] - m_data [0] * m_data [7] * m_data [13] - m_data [4] * m_data [1] * m_data [15] + m_data [4] * m_data [3] * m_data [13] + m_data [12] * m_data [1] * m_data [7] - m_data [12] * m_data [3] * m_data [5];
im [14] = -m_data [0] * m_data [5] * m_data [14] + m_data [0] * m_data [6] * m_data [13] + m_data [4] * m_data [1] * m_data [14] - m_data [4] * m_data [2] * m_data [13] - m_data [12] * m_data [1] * m_data [6] + m_data [12] * m_data [2] * m_data [5];
im [3] =  -m_data [1] * m_data [6] * m_data [11] + m_data [1] * m_data [7] * m_data [10] + m_data [5] * m_data [2] * m_data [11] - m_data [5] * m_data [3] * m_data [10] - m_data [9] * m_data [2] * m_data [7] + m_data [9] * m_data [3] * m_data [6];
im [7] =   m_data [0] * m_data [6] * m_data [11] - m_data [0] * m_data [7] * m_data [10] - m_data [4] * m_data [2] * m_data [11] + m_data [4] * m_data [3] * m_data [10] + m_data [8] * m_data [2] * m_data [7] - m_data [8] * m_data [3] * m_data [6];
im [11] = -m_data [0] * m_data [5] * m_data [11] + m_data [0] * m_data [7] * m_data [9] + m_data [4] * m_data [1] * m_data [11] - m_data [4] * m_data [3] * m_data [9] - m_data [8] * m_data [1] * m_data [7] + m_data [8] * m_data [3] * m_data [5];
im [15] =  m_data [0] * m_data [5] * m_data [10] - m_data [0] * m_data [6] * m_data [9] - m_data [4] * m_data [1] * m_data [10] + m_data [4] * m_data [2] * m_data [9] + m_data [8] * m_data [1] * m_data [6] - m_data [8] * m_data [2] * m_data [5];

double det = Det (im);
if (det == 0.0)
	return *this;

det = 1.0 / det;
for (int i = 0; i < 16; i++)
	im [i] *= det;
return im;
}

//------------------------------------------------------------------------------

