/*
   Datei: IConv.C
          Ganzzahl-Konvertierung
   Autor: D.Mali
   Stand: 08.05.95
          Copyright (C) PTV GmbH Karlsruhe
*/

#include "stringh.h"
#include "iconv.h"

tIntConvRes intConvRes  = I_INT_OK;

UINT1 char2digit [256] =
   BEGIN
#if EBCDIC
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255,  10,  11,  12,  13,  14,  15,  16,  17,  18, 255, 255, 255, 255, 255, 255,
   255,  19,  20,  21,  22,  23,  24,  25,  26,  27, 255, 255, 255, 255, 255, 255,
   255, 255,  28,  29,  30,  31,  32,  33,  34,  35, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255,  10,  11,  12,  13,  14,  15,  16,  17,  18, 255, 255, 255, 255, 255, 255,
   255,  19,  20,  21,  22,  23,  24,  25,  26,  27, 255, 255, 255, 255, 255, 255,
   255,  28,  29,  30,  31,  32,  33,  34,  35, 255, 255, 255, 255, 255, 255, 255,
     0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 255, 255, 255, 255, 255, 255
#else
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
     0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 255, 255, 255, 255, 255, 255,
   255,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
    25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35, 255, 255, 255, 255, 255,
   255,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
    25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
#endif
   END;

/*----------------------------------------------------------------------------+
|                                                                             |
+----------------------------------------------------------------------------*/

LPSTR  DIM (intConvMsgs, I_OUT_OF_RANGE + 1) =
   BEGIN
   "",
   "Basis unzulaessig",
   "Zahl erwartet",
   "Zahl zu gross",
   "Gueltigkeitsbereich verlassen"
   END;

/*----------------------------------------------------------------------------+
|                                                                             |
+----------------------------------------------------------------------------*/

tIntConvRes LIBEXP IntConvRes (NOPARAM)
BEGIN
   tIntConvRes _intConvRes;

_intConvRes = intConvRes;
intConvRes = I_INT_OK;
return _intConvRes;
ENDFUNC

/*----------------------------------------------------------------------------+
|                                                                             |
+----------------------------------------------------------------------------*/

LPSTR LIBEXP IntConvMsg
#ifdef _ANSIC
   (tIntConvRes intConvRes)
#else
   (intConvRes)
   tIntConvRes intConvRes;
#endif
BEGIN
return *(intConvMsgs + intConvRes);
ENDFUNC

/*----------------------------------------------------------------------------+
|                                                                             |
+----------------------------------------------------------------------------*/

LPSTR LIBEXP SSign
#ifdef _ANSIC
   (LPSTR s, pINT2 sign)
#else
   (s, sign)
   LPSTR    s;
   pINT2    sign;
#endif
BEGIN
   INT2  sg = 1;

FOREVER
   BEGIN
   switch (*s)
      BEGIN
      case '-':
         sg = -sg;
         break;

      case '+':
         break;

      default:
         *sign = sg;
         return s;
      ENDCASE
   ++s;
   ENDFOR
ENDFUNC /*Sign*/

/*----------------------------------------------------------------------------+
|                                                                             |
+----------------------------------------------------------------------------*/

INT4 LIBEXP SToI
#ifdef _ANSIC
   (LPSTR  s, pINT2 p, UINT1 base, INT4 min, INT4 max)
#else
   (s, p, base, min, max)
   LPSTR    s;
   pINT2    p;
   UINT1     base;
   INT4     min, max;
#endif
BEGIN
   INT4  h, v;
   CHAR  c;
   UINT1  d;
   INT2  sign;
   LPSTR t;

if ((base < 2) OR (base > 36))
   BEGIN
   intConvRes = I_INV_BASE;
   return 1;
   ENDIF

intConvRes = I_INT_OK;
t = SSign (s + (p ? *p : 0), ADDR (sign));
for (h = -1, v = 0; c = *t; t++)
   BEGIN
   if ((d = *(char2digit + (UINT1) c)) >= base)
      break;
   if (v > (h = base * v + d))
      BEGIN
      intConvRes = I_OVERFLOW;
      break;
      ENDIF
   v = h;
   ENDFOR

if (p)
   *p = t - s;
if (intConvRes)
   return 1;
v *= sign;
if ((h == -1) AND (v == 0))
   intConvRes = I_NO_NUMBER;
else if ((v < min) OR (v > max))
   intConvRes = I_OUT_OF_RANGE;
else
   return v;
return 1;
END /*SToI*/

/*----------------------------------------------------------------------------+
|                                                                             |
+----------------------------------------------------------------------------*/

UINT4 LIBEXP SToU
#ifdef _ANSIC
   (LPSTR  s, pINT2 p, UINT1 base, UINT4 min, UINT4 max)
#else
   (s, p, base, min, max)
   LPSTR    s;
   pINT2    p;
   UINT1     base;
   UINT4    min, max;
#endif
BEGIN
   UINT4 h, v;
   CHAR  c;
   UINT1  d;
   LPSTR t;

if ((base < 2) OR (base > 36))
   BEGIN
   intConvRes = I_INV_BASE;
   return 1;
   ENDIF

intConvRes = I_INT_OK;
for (t = s + (p ? *p : 0); *t == '+'; t++)
   ;
if ((p))
   *p = t - s;
s = t;
for (h = v = 0; c = *t; t++)
   BEGIN
   if ((d = *(char2digit + (UINT1) c)) >= base)
      break;
   if (v > (h = base * v + d))
      BEGIN
      intConvRes = I_OVERFLOW;
      break;
      ENDIF
   v = h;
   ENDFOR

if (p)
   *p += t - s;
if (intConvRes)
   return 1;
if (t == s)
   intConvRes = I_NO_NUMBER;
else if ((v < min) OR (v > max))
   intConvRes = I_OUT_OF_RANGE;
else
   return v;
return 1;
END /*SToU*/

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

static CHAR ODIM (digits) = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

LPSTR IToS
#ifdef _ANSIC
   (LPSTR s, INT4 i, UINT1 base, INT2 width, CHAR filler, UINT1 align)
#else
   (s, i, base, width, filler, align)
   LPSTR s;
   INT4  i;
   UINT1 base;
   INT2  width;
   CHAR  filler;
   UINT1 align;
#endif
BEGIN
   LPSTR    t;
   INT2     l, w;
   BOOLEAN  neg = (i < 0);
   INT4     h;

if (neg)
   i = -i;
if (NOT width)
   for (h = i, width = 1 + neg; h > 9; h /= 10, width++)
      ;
switch (filler)
   BEGIN
   case '0':
      align = I_RIGHT;
      break;
   case 0:
      filler = ' ';
      break;
   ENDCASE
MemSet (s, filler, width);
s [width] = (CHAR) 0;
t = s + width; 

while (t > s)
   BEGIN
   --t;
   *t = digits[i % base];
   if (NOT (i = i / base))
      break;
   ENDWHILE
if (t == s)
   return t;
if (neg)
   if ((align != I_RIGHT))
      *--t = '-';
   else
      *s = '-';
l = t - s;
w = width - l;
switch (align)
   BEGIN
   case I_LEFT:
      MemMove (s, t, w);
      MemSet (s + width - l, filler, l);
     break;

   case I_CENTERED:
      l /= 2;
      MemMove (s + l, t, w);
      MemSet (t + w, filler, width - w - l);
   ENDCASE
return s;
ENDFUNC

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

LPSTR UToS
#ifdef _ANSIC
   (LPSTR s, UINT4 i, UINT1 base, INT2 width, CHAR filler, UINT1 align)
#else
   (s, i, base, width, filler, align)
   LPSTR s;
   UINT4 i;
   UINT1 base;
   INT2  width;
   CHAR  filler;
   UINT1 align;
#endif
BEGIN
   LPSTR    t;
   INT2     l, w;
   UINT4    h;

if (NOT width)
   for (h = i, width = 1; h > 9; h /= 10, width++)
      ;
MemSet (s, filler, width);
if (filler == '0')
   align = I_RIGHT;
s [width] = (CHAR) 0;
t = s + width; 
while (t > s)
   BEGIN
   --t;
   *t = digits [i % base];
   if (NOT (i = i / base))
      break;
   ENDWHILE
if (t == s)
   return t;
l = t - s;
w = width - l;
switch (align)
   BEGIN
   case I_LEFT:
      MemMove (s, t, w);
      MemSet (s + width - l, filler, l);
     break;

   case I_CENTERED:
      l /= 2;
      MemMove (s + l, t, w);
      MemSet (t + w, filler, width - w - l);
   ENDCASE
return s;
ENDFUNC

/*----------------------------------------------------------------------------+
|                                                                             |
+----------------------------------------------------------------------------*/

char *FormatUInt (UINT4 nVal, char *pszVal)
BEGIN
	char	*ps = pszVal;
	INT1	i = 0;

FOREVER
	BEGIN
	*ps++ = digits [nVal % 10];
	nVal /= 10;
	if (nVal == 0)
		break;
	if (++i % 3 == 0)
		*ps++ = '.';
	ENDFOR
return strrev (pszVal);	
ENDFUNC

								/*---------------------------*/

char *FormatInt (INT4 nVal, char *pszVal)
BEGIN
	BOOLEAN	bNeg;

if (bNeg = (nVal < 0))
	nVal = -nVal;
FormatUInt ((UINT4) nVal, pszVal + bNeg);
if (bNeg)
	*pszVal = '-';
return pszVal;
ENDFUNC

/*----------------------------------------------------------------------------+
|                                                                             |
+----------------------------------------------------------------------------*/

