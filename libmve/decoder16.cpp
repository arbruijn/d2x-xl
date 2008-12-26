/* 16 bit decoding routines */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "decoders.h"

static ushort *backBuf1, *backBuf2;
static int lookup_initialized;

static void dispatchDecoder16(ushort **frameP, ubyte codeType, ubyte **dataP, ubyte **pOffData, int *remainDataP, int *curXb, int *curYb);
static void genLoopkupTable();

void decodeFrame16(ubyte *frameP, ubyte *mapP, int mapRemain, ubyte *dataP, int dataRemain)
{
    ubyte *pOrig;
    ubyte *pOffData, *pEnd;
    ushort offset;
    ushort *FramePtr = reinterpret_cast<ushort*> (frameP);
    ubyte op;
    int length;
    int i, j;
    int xb, yb;

	if (!lookup_initialized) {
		genLoopkupTable();
	}

    backBuf1 = reinterpret_cast<ushort*> (g_vBackBuf [g_currBuf]);
    backBuf2 = reinterpret_cast<ushort*> (g_vBackBuf [!g_currBuf]);

    xb = g_width >> 3;
    yb = g_height >> 3;

    offset = dataP[0]|(dataP[1]<<8);

    pOffData = dataP + offset;
    pEnd = dataP + offset;

    dataP += 2;

    pOrig = dataP;
    length = offset - 2; /*dataRemain-2;*/

    for (j=0; j<yb; j++)
    {
        for (i=0; i<xb/2; i++)
        {
            op = (*mapP) & 0xf;
            dispatchDecoder16(&FramePtr, op, &dataP, &pOffData, &dataRemain, &i, &j);

			/*
			  if (FramePtr < backBuf1)
			  fprintf(stderr, "danger!  pointing out of bounds below after dispatch decoder: %d, %d (1) [%x]\n", i, j, (*mapP) & 0xf);
			  else if (FramePtr >= backBuf1 + g_width*g_height)
			  fprintf(stderr, "danger!  pointing out of bounds above after dispatch decoder: %d, %d (1) [%x]\n", i, j, (*mapP) & 0xf);
			*/

			op = ((*mapP) >> 4) & 0xf;
            dispatchDecoder16(&FramePtr, op, &dataP, &pOffData, &dataRemain, &i, &j);

			/*
			  if (FramePtr < backBuf1)
			  fprintf(stderr, "danger!  pointing out of bounds below after dispatch decoder: %d, %d (2) [%x]\n", i, j, (*mapP) >> 4);
			  else if (FramePtr >= backBuf1 + g_width*g_height)
			  fprintf(stderr, "danger!  pointing out of bounds above after dispatch decoder: %d, %d (2) [%x]\n", i, j, (*mapP) >> 4);
			*/

            ++mapP;
            --mapRemain;
        }

        FramePtr += 7*g_width;
    }

#if 0
    if ((length-(dataP-pOrig)) != 0) {
    	fprintf(stderr, "DEBUG: junk left over: %d,%d,%d\n", (dataP-pOrig), length, (length-(dataP-pOrig));
    }
#endif
}

static ushort GETPIXEL(ubyte **buf, int off)
{
	ushort val = (*buf)[0+off] | ((*buf)[1+off] << 8);
	return val;
}

static ushort GETPIXELI(ubyte **buf, int off)
{
	ushort val = (*buf)[0+off] | ((*buf)[1+off] << 8);
	(*buf) += 2;
	return val;
}

static void relClose(int i, int *x, int *y)
{
    int ma, mi;

    ma = i >> 4;
    mi = i & 0xf;

    *x = mi - 8;
    *y = ma - 8;
}

static void relFar(int i, int sign, int *x, int *y)
{
    if (i < 56)
    {
        *x = sign * (8 + (i % 7));
        *y = sign *      (i / 7);
    }
    else
    {
        *x = sign * (-14 + (i - 56) % 29);
        *y = sign *   (8 + (i - 56) / 29);
    }
}

static int close_table[512];
static int far_p_table[512];
static int far_n_table[512];

static void genLoopkupTable()
{
	int i;
	int x, y;

	for (i = 0; i < 256; i++) {
		relClose(i, &x, &y);

		close_table[i*2+0] = x;
		close_table[i*2+1] = y;

		relFar(i, 1, &x, &y);

		far_p_table[i*2+0] = x;
		far_p_table[i*2+1] = y;

		relFar(i, -1, &x, &y);

		far_n_table[i*2+0] = x;
		far_n_table[i*2+1] = y;
	}

	lookup_initialized = 1;
}

static void copyFrame(ushort *pDest, ushort *pSrc)
{
    int i;

    for (i=0; i<8; i++)
    {
        memcpy(pDest, pSrc, 16);
        pDest += g_width;
        pSrc += g_width;
    }
}

static void patternRow4Pixels(ushort *frameP,
                              ubyte pat0, ubyte pat1,
                              ushort *p)
{
    ushort mask=0x0003;
    ushort shift=0;
    ushort pattern = (pat1 << 8) | pat0;

    while (mask != 0)
    {
        *frameP++ = p[(mask & pattern) >> shift];
        mask <<= 2;
        shift += 2;
    }
}

static void patternRow4Pixels2(ushort *frameP,
                               ubyte pat0,
                               ushort *p)
{
    ubyte mask=0x03;
    ubyte shift=0;
    ushort pel;
	/* ORIGINAL VERSION IS BUGGY
	   int skip=1;

	   while (mask != 0)
	   {
	   pel = p[(mask & pat0) >> shift];
	   frameP[0] = pel;
	   frameP[2] = pel;
	   frameP[g_width + 0] = pel;
	   frameP[g_width + 2] = pel;
	   frameP += skip;
	   skip = 4 - skip;
	   mask <<= 2;
	   shift += 2;
	   }
	*/
    while (mask != 0)
    {
        pel = p[(mask & pat0) >> shift];
        frameP[0] = pel;
        frameP[1] = pel;
        frameP[g_width + 0] = pel;
        frameP[g_width + 1] = pel;
        frameP += 2;
        mask <<= 2;
        shift += 2;
    }
}

static void patternRow4Pixels2x1(ushort *frameP, ubyte pat,
								 ushort *p)
{
    ubyte mask=0x03;
    ubyte shift=0;
    ushort pel;

    while (mask != 0)
    {
        pel = p[(mask & pat) >> shift];
        frameP[0] = pel;
        frameP[1] = pel;
        frameP += 2;
        mask <<= 2;
        shift += 2;
    }
}

static void patternQuadrant4Pixels(ushort *frameP,
								   ubyte pat0, ubyte pat1, ubyte pat2,
								   ubyte pat3, ushort *p)
{
    uint mask = 3;
    int shift=0;
    int i;
    uint pat = (pat3 << 24) | (pat2 << 16) | (pat1 << 8) | pat0;

    for (i=0; i<16; i++)
    {
        frameP[i&3] = p[(pat & mask) >> shift];

        if ((i&3) == 3)
            frameP += g_width;

        mask <<= 2;
        shift += 2;
    }
}


static void patternRow2Pixels(ushort *frameP, ubyte pat,
							  ushort *p)
{
    ubyte mask=0x01;

    while (mask != 0)
    {
        *frameP++ = p[(mask & pat) ? 1 : 0];
        mask <<= 1;
    }
}

static void patternRow2Pixels2(ushort *frameP, ubyte pat,
							   ushort *p)
{
    ushort pel;
    ubyte mask=0x1;

	/* ORIGINAL VERSION IS BUGGY
	   int skip=1;
	   while (mask != 0x10)
	   {
	   pel = p[(mask & pat) ? 1 : 0];
	   frameP[0] = pel;
	   frameP[2] = pel;
	   frameP[g_width + 0] = pel;
	   frameP[g_width + 2] = pel;
	   frameP += skip;
	   skip = 4 - skip;
	   mask <<= 1;
	   }
	*/
	while (mask != 0x10) {
		pel = p[(mask & pat) ? 1 : 0];

		frameP[0] = pel;
		frameP[1] = pel;
		frameP[g_width + 0] = pel;
		frameP[g_width + 1] = pel;
		frameP += 2;

		mask <<= 1;
	}
}

static void patternQuadrant2Pixels(ushort *frameP, ubyte pat0,
								   ubyte pat1, ushort *p)
{
    ushort mask = 0x0001;
    int i;
    ushort pat = (pat1 << 8) | pat0;

    for (i=0; i<16; i++)
    {
        frameP[i&3] = p[(pat & mask) ? 1 : 0];

        if ((i&3) == 3)
            frameP += g_width;

        mask <<= 1;
    }
}

static void dispatchDecoder16(ushort **frameP, ubyte codeType, ubyte **dataP, ubyte **pOffData, int *remainDataP, int *curXb, int *curYb)
{
    ushort p[4];
    ubyte pat[16];
    int i, j, k;
    int x, y;
    ushort *pDstBak;

    pDstBak = *frameP;

    switch(codeType)
    {
	case 0x0:
		copyFrame(*frameP, *frameP + (backBuf2 - backBuf1));
	case 0x1:
		break;
	case 0x2: /*
				relFar(*(*pOffData)++, 1, &x, &y);
			  */

		k = *(*pOffData)++;
		x = far_p_table[k*2+0];
		y = far_p_table[k*2+1];

		copyFrame(*frameP, *frameP + x + y*g_width);
		--*remainDataP;
		break;
	case 0x3: /*
				relFar(*(*pOffData)++, -1, &x, &y);
			  */

		k = *(*pOffData)++;
		x = far_n_table[k*2+0];
		y = far_n_table[k*2+1];

		copyFrame(*frameP, *frameP + x + y*g_width);
		--*remainDataP;
		break;
	case 0x4: /*
				relClose(*(*pOffData)++, &x, &y);
			  */

		k = *(*pOffData)++;
		x = close_table[k*2+0];
		y = close_table[k*2+1];

		copyFrame(*frameP, *frameP + (backBuf2 - backBuf1) + x + y*g_width);
		--*remainDataP;
		break;
	case 0x5:
		x = (char)*(*dataP)++;
		y = (char)*(*dataP)++;
		copyFrame(*frameP, *frameP + (backBuf2 - backBuf1) + x + y*g_width);
		*remainDataP -= 2;
		break;
	case 0x6:
#ifndef WIN32
		fprintf(stderr, "STUB: encoding 6 not tested\n");
#endif
		for (i=0; i<2; i++)
		{
			*frameP += 16;
			if (++*curXb == (g_width >> 3))
			{
				*frameP += 7*g_width;
				*curXb = 0;
				if (++*curYb == (g_height >> 3))
					return;
			}
		}
		break;

	case 0x7:
		p[0] = GETPIXELI(dataP, 0);
		p[1] = GETPIXELI(dataP, 0);

		if (!((p[0]/*|p[1]*/)&0x8000))
		{
			for (i=0; i<8; i++)
			{
				patternRow2Pixels(*frameP, *(*dataP), p);
				(*dataP)++;

				*frameP += g_width;
			}
		}
		else
		{
			for (i=0; i<2; i++)
			{
				patternRow2Pixels2(*frameP, (ubyte) (*(*dataP) & 0xf), p);
				*frameP += 2*g_width;
				patternRow2Pixels2(*frameP, (ubyte) (*(*dataP) >> 4), p);
				(*dataP)++;

				*frameP += 2*g_width;
			}
		}
		break;

	case 0x8:
		p[0] = GETPIXEL(dataP, 0);

		if (!(p[0] & 0x8000))
		{
			for (i=0; i<4; i++)
			{
				p[0] = GETPIXELI(dataP, 0);
				p[1] = GETPIXELI(dataP, 0);

				pat[0] = (*dataP)[0];
				pat[1] = (*dataP)[1];
				(*dataP) += 2;

				patternQuadrant2Pixels(*frameP, pat[0], pat[1], p);

				if (i & 1)
					*frameP -= (4*g_width - 4);
				else
					*frameP += 4*g_width;
			}


		} else {
			p[2] = GETPIXEL(dataP, 8);

			if (!(p[2]&0x8000)) {
				for (i=0; i<4; i++)
				{
					if ((i & 1) == 0)
					{
						p[0] = GETPIXELI(dataP, 0);
						p[1] = GETPIXELI(dataP, 0);
					}
					pat[0] = *(*dataP)++;
					pat[1] = *(*dataP)++;
					patternQuadrant2Pixels(*frameP, pat[0], pat[1], p);

					if (i & 1)
						*frameP -= (4*g_width - 4);
					else
						*frameP += 4*g_width;
				}
			} else {
				for (i=0; i<8; i++)
				{
					if ((i & 3) == 0)
					{
						p[0] = GETPIXELI(dataP, 0);
						p[1] = GETPIXELI(dataP, 0);
					}
					patternRow2Pixels(*frameP, *(*dataP), p);
					(*dataP)++;

					*frameP += g_width;
				}
			}
		}
		break;

	case 0x9:
		p[0] = GETPIXELI(dataP, 0);
		p[1] = GETPIXELI(dataP, 0);
		p[2] = GETPIXELI(dataP, 0);
		p[3] = GETPIXELI(dataP, 0);

		*remainDataP -= 8;

		if (!(p[0] & 0x8000))
		{
			if (!(p[2] & 0x8000))
			{

				for (i=0; i<8; i++)
				{
					pat[0] = (*dataP)[0];
					pat[1] = (*dataP)[1];
					(*dataP) += 2;
					patternRow4Pixels(*frameP, pat[0], pat[1], p);
					*frameP += g_width;
				}
				*remainDataP -= 16;

			}
			else
			{
				patternRow4Pixels2(*frameP, (*dataP)[0], p);
				*frameP += 2*g_width;
				patternRow4Pixels2(*frameP, (*dataP)[1], p);
				*frameP += 2*g_width;
				patternRow4Pixels2(*frameP, (*dataP)[2], p);
				*frameP += 2*g_width;
				patternRow4Pixels2(*frameP, (*dataP)[3], p);

				(*dataP) += 4;
				*remainDataP -= 4;

			}
		}
		else
		{
			if (!(p[2] & 0x8000))
			{
				for (i=0; i<8; i++)
				{
					pat[0] = (*dataP)[0];
					(*dataP) += 1;
					patternRow4Pixels2x1(*frameP, pat[0], p);
					*frameP += g_width;
				}
				*remainDataP -= 8;
			}
			else
			{
				for (i=0; i<4; i++)
				{
					pat[0] = (*dataP)[0];
					pat[1] = (*dataP)[1];

					(*dataP) += 2;

					patternRow4Pixels(*frameP, pat[0], pat[1], p);
					*frameP += g_width;
					patternRow4Pixels(*frameP, pat[0], pat[1], p);
					*frameP += g_width;
				}
				*remainDataP -= 8;
			}
		}
		break;

	case 0xa:
		p[0] = GETPIXEL(dataP, 0);

		if (!(p[0] & 0x8000))
		{
			for (i=0; i<4; i++)
			{
				p[0] = GETPIXELI(dataP, 0);
				p[1] = GETPIXELI(dataP, 0);
				p[2] = GETPIXELI(dataP, 0);
				p[3] = GETPIXELI(dataP, 0);
				pat[0] = (*dataP)[0];
				pat[1] = (*dataP)[1];
				pat[2] = (*dataP)[2];
				pat[3] = (*dataP)[3];

				(*dataP) += 4;

				patternQuadrant4Pixels(*frameP, pat[0], pat[1], pat[2], pat[3], p);

				if (i & 1)
					*frameP -= (4*g_width - 4);
				else
					*frameP += 4*g_width;
			}
		}
		else
		{
			p[0] = GETPIXEL(dataP, 16);

			if (!(p[0] & 0x8000))
			{
				for (i=0; i<4; i++)
				{
					if ((i&1) == 0)
					{
						p[0] = GETPIXELI(dataP, 0);
						p[1] = GETPIXELI(dataP, 0);
						p[2] = GETPIXELI(dataP, 0);
						p[3] = GETPIXELI(dataP, 0);
					}

					pat[0] = (*dataP)[0];
					pat[1] = (*dataP)[1];
					pat[2] = (*dataP)[2];
					pat[3] = (*dataP)[3];

					(*dataP) += 4;

					patternQuadrant4Pixels(*frameP, pat[0], pat[1], pat[2], pat[3], p);

					if (i & 1)
						*frameP -= (4*g_width - 4);
					else
						*frameP += 4*g_width;
				}
			}
			else
			{
				for (i=0; i<8; i++)
				{
					if ((i&3) == 0)
					{
						p[0] = GETPIXELI(dataP, 0);
						p[1] = GETPIXELI(dataP, 0);
						p[2] = GETPIXELI(dataP, 0);
						p[3] = GETPIXELI(dataP, 0);
					}

					pat[0] = (*dataP)[0];
					pat[1] = (*dataP)[1];
					patternRow4Pixels(*frameP, pat[0], pat[1], p);
					*frameP += g_width;

					(*dataP) += 2;
				}
			}
		}
		break;

	case 0xb:
		for (i=0; i<8; i++)
		{
			memcpy(*frameP, *dataP, 16);
			*frameP += g_width;
			*dataP += 16;
			*remainDataP -= 16;
		}
		break;

	case 0xc:
		for (i=0; i<4; i++)
		{
			p[0] = GETPIXEL(dataP, 0);
			p[1] = GETPIXEL(dataP, 2);
			p[2] = GETPIXEL(dataP, 4);
			p[3] = GETPIXEL(dataP, 6);

			for (j=0; j<2; j++)
			{
				for (k=0; k<4; k++)
				{
					(*frameP)[j+2*k] = p[k];
					(*frameP)[g_width+j+2*k] = p[k];
				}
				*frameP += g_width;
			}
			*dataP += 8;
			*remainDataP -= 8;
		}
		break;

	case 0xd:
		for (i=0; i<2; i++)
		{
			p[0] = GETPIXEL(dataP, 0);
			p[1] = GETPIXEL(dataP, 2);

			for (j=0; j<4; j++)
			{
				for (k=0; k<4; k++)
				{
					(*frameP)[k*g_width+j] = p[0];
					(*frameP)[k*g_width+j+4] = p[1];
				}
			}

			*frameP += 4*g_width;

			*dataP += 4;
			*remainDataP -= 4;
		}
		break;

	case 0xe:
		p[0] = GETPIXEL(dataP, 0);

		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				(*frameP)[j] = p[0];
			}

			*frameP += g_width;
		}

		*dataP += 2;
		*remainDataP -= 2;

		break;

	case 0xf:
		p[0] = GETPIXEL(dataP, 0);
		p[1] = GETPIXEL(dataP, 1);

		for (i=0; i<8; i++)
		{
			for (j=0; j<8; j++)
			{
				(*frameP)[j] = p[(i+j)&1];
			}
			*frameP += g_width;
		}

		*dataP += 4;
		*remainDataP -= 4;
		break;

	default:
		break;
    }

    *frameP = pDstBak+8;
}
