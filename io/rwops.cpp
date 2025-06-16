#include <inttypes.h>
#include <SDL.h>
#include "rwops.h"

static int rwops_seek(SDL_RWops* rw, int offset, int whence)
{
    CFile* cf = (CFile*)rw->hidden.unknown.data1;
    return (int)cf->Seek (offset, whence) == -1 ? -1 : cf->Tell ();
}

static int rwops_read(SDL_RWops *rw, void *ptr, int size, int maxnum)
{
    CFile* cf = (CFile*)rw->hidden.unknown.data1;
    return (int)cf->Read (ptr, size, maxnum, 0, 1);
}

static int rwops_write(SDL_RWops *rw, const void *ptr, int size, int num)
{
    CFile* cf = (CFile*)rw->hidden.unknown.data1;
    return (int)cf->Write (ptr, size, num);
}

static int rwops_close(SDL_RWops *rw)
{
	CFile* cf = (CFile*)rw->hidden.unknown.data1;
	if (cf->Close ())
		return EOF;
    SDL_FreeRW(rw);
	delete cf;
    return 0;
}

SDL_RWops *CFileOpenRWOps(const char *file, const char *folder)
{
	SDL_RWops *retval = NULL;
	CFile *cf;

	if (!(cf = new CFile()))
		return NULL;

	if (!(cf->Open(file, folder, "rb", 0))) {
		delete cf;
		return NULL;
		}

	if (!(retval = SDL_AllocRW ())) {
		delete cf;
		return NULL;
		}

	retval->seek  = rwops_seek;
	retval->read  = rwops_read;
	retval->write = rwops_write;
	retval->close = rwops_close;
	retval->hidden.unknown.data1 = (void *)cf;

	return retval;
}

