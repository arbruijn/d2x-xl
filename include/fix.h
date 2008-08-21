/* fix[HA] points to maths[HA] */
#ifndef _FIX_H
#define _FIX_H

#include "maths.h"

// ------------------------------------------------------------------------

//extract a fix from a quad product
static inline fix FixQuadAdjust (tQuadInt *q)
{
return (q->high << 16) + (q->low >> 16);
}

// ------------------------------------------------------------------------

static inline void FixQuadNegate (tQuadInt *q)
{
q->low  = (u_int32_t) -((int32_t) q->low);
q->high = -q->high - (q->low != 0);
}

// ------------------------------------------------------------------------

#endif //_FIX_H
