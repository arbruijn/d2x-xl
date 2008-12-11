/*
 * some misc. file/disk routines
 * Arne de Bruijn, 1998
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include "d_io.h"
#ifdef __DJGPP__
#include "dos_disk.h"
#endif
//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM

#if defined(_WIN32) && !defined(_WIN32_WCE)
#include <windows.h>
#define lseek(a,b,c) _lseek(a,b,c)
#endif

#if 0
long filelength(int fd) {
	long old_pos, size;

	if ((old_pos = lseek(fd, 0, SEEK_CUR)) == -1 ||
	    (size = lseek(fd, 0, SEEK_END)) == -1 ||
	    (lseek(fd, old_pos, SEEK_SET)) == -1)
		return -1L;
	return size;
}
#endif

long ffilelength(FILE *file)
{
	long old_pos, size;

	if ((old_pos = ftell(file)) == -1 ||
	    fseek(file, 0, SEEK_END) == -1 ||
	    (size = ftell(file)) == -1 ||
	    fseek(file, old_pos, SEEK_SET) == -1)
		return -1L;
	return size;
}


ulong d_getdiskfree()
{
 // FIXME:
return 999999999;
}

ulong GetDiskFree()
{
 return d_getdiskfree();
}

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out) {
	char *p;
	if ((p = const_cast<char*> (strrchr (filename, '.')))) {
		strncpy(out, filename, p - filename);
		out[p - filename] = 0;
	} else
		strcpy(out, filename);
}
