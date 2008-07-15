/*
 * strio.c: string/file manipulation functions by Victor Rachels
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "cfile.h"
#include "strio.h"
//added on 9/16/98 by adb to add memory tracking for this module
#include "u_mem.h"
//end additions - adb

char *fsplitword (CFILE *f, char splitchar)
{
	int	i, mem;
	char	c, *buf;

mem = 256;
buf = (char *) D2_ALLOC (sizeof (char) * mem);
c = CFGetC (f);
for (i = 0; (c != splitchar) && !CFEoF (f); i++) {
	if (i == mem) {
		mem += 256;
		buf = (char *) D2_REALLOC (buf, mem);
		}
	buf [i] = c;
	c = CFGetC (f);
	}
if (CFEoF (f) && (c != splitchar))
	buf [i++] = c;
buf [i] = 0;
return buf;
}


char *splitword (char *s, char splitchar)
{
	int	l, lw;
	char	*buf, *p;

l = (int) strlen (s);
p = strchr (s, splitchar);
lw = p ? (int) (p - s) : l;
buf = (char *) D2_ALLOC (lw + 1);
memcpy (buf, s, lw + 1);
buf [lw] = '\0';
if (p)
	memmove (s, p + 1, l - lw);
else
	*s = '\0';
return buf;
}
