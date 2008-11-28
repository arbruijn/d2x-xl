/* $Id: iff.c, v 1.7 2003/10/04 03:14:47 btb Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#define COMPRESS		1	//do the RLE or not? (for debugging mostly)
#define WRITE_TINY	0	//should we write a TINY chunk?

#define MIN_COMPRESS_WIDTH	65	//don't compress if less than this wide

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inferno.h"
#include "u_mem.h"
#include "iff.h"
#include "cfile.h"
#include "error.h"

//Internal constants and structures for this library

//Type values for bitmaps
#define TYPE_PBM		0
#define TYPE_ILBM		1

//Compression types
#define cmpNone	0
#define cmpByteRun1	1

//Masking types
#define mskNone	0
#define mskHasMask	1
#define mskHasTransparentColor 2

//#define min(a, b) ((a<b)?a:b)

//------------------------------------------------------------------------------

#define MAKE_SIG(a, b, c, d) (((int)(a)<<24)+((int)(b)<<16)+((c)<<8)+(d))

#define form_sig MAKE_SIG('F', 'O', 'R', 'M')
#define ilbm_sig MAKE_SIG('I', 'L', 'B', 'M')
#define body_sig MAKE_SIG('B', 'O', 'D', 'Y')
#define pbm_sig  MAKE_SIG('P', 'B', 'M', ' ')
#define bmhd_sig MAKE_SIG('B', 'M', 'H', 'D')
#define anhd_sig MAKE_SIG('A', 'N', 'H', 'D')
#define cmap_sig MAKE_SIG('C', 'M', 'A', 'P')
#define tiny_sig MAKE_SIG('T', 'I', 'N', 'Y')
#define anim_sig MAKE_SIG('A', 'N', 'I', 'M')
#define dlta_sig MAKE_SIG('D', 'L', 'T', 'A')

//------------------------------------------------------------------------------

#if DBG
//void printsig(int s)
//{
//	char *t=(char *) &s;
//
///*  //printf("%c%c%c%c", *(&s+3), *(&s+2), *(&s+1), s);*/
//	//printf("%c%c%c%c", t[3], t[2], t[1], t[0]);
//}
#endif

//------------------------------------------------------------------------------

int PutSig (int sig, FILE *fp)
{
	char *s = (char *) &sig;

fputc(s[3], fp);
fputc(s[2], fp);
fputc(s[1], fp);
return fputc(s[0], fp);

}

//------------------------------------------------------------------------------

int PutByte (unsigned char c, FILE *fp)
{
return fputc(c, fp);
}

//------------------------------------------------------------------------------

int PutWord (int n, FILE *fp)
{
	unsigned char c0, c1;

c0 = (n & 0xff00) >> 8;
c1 = n & 0xff;
PutByte (c0, fp);
return PutByte (c1, fp);
}

//------------------------------------------------------------------------------

int PutLong(int n, FILE *fp)
{
	int n0, n1;

n0 = (int) ((n & 0xffff0000l) >> 16);
n1 = (int) (n & 0xffff);
PutWord (n0, fp);
return PutWord (n1, fp);
}

//------------------------------------------------------------------------------

int CIFF::GetSig (void)
{
	char s[4];

//	if ((s[3]=CFGetC(f))==EOF) return(EOF);
//	if ((s[2]=CFGetC(f))==EOF) return(EOF);
//	if ((s[1]=CFGetC(f))==EOF) return(EOF);
//	if ((s[0]=CFGetC(f))==EOF) return(EOF);

#if !(defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__))
	if (Pos()>=Len()) return EOF;
	s[3] = Data() [Pos()++];
	if (Pos()>=Len()) return EOF;
	s[2] = Data() [Pos()++];
	if (Pos()>=Len()) return EOF;
	s[1] = Data() [Pos()++];
	if (Pos()>=Len()) return EOF;
	s[0] = Data() [Pos()++];
#else
	if (Pos()>=Len()) return EOF;
	s[0] = Data() [Pos()++];
	if (Pos()>=Len()) return EOF;
	s[1] = Data() [Pos()++];
	if (Pos()>=Len()) return EOF;
	s[2] = Data() [Pos()++];
	if (Pos()>=Len()) return EOF;
	s[3] = Data() [Pos()++];
#endif
	return(*((int *) s));
}

//------------------------------------------------------------------------------

char CIFF::GetByte (void)
{
return Data() [Pos()++];
}

//------------------------------------------------------------------------------

int CIFF::GetWord (void)
{
unsigned char c0, c1;
if (Pos()>=Len()) return EOF;
c1 = Data() [Pos()++];
if (Pos()>=Len()) return EOF;
c0 = Data() [Pos()++];
if (c0==0xff) return(EOF);
return(((int)c1<<8) + c0);
}

//------------------------------------------------------------------------------

int CIFF::GetLong (void)
{
	unsigned char c0, c1, c2, c3;
if (Pos()>=Len()) return EOF;
c3 = Data() [Pos()++];
if (Pos()>=Len()) return EOF;
c2 = Data() [Pos()++];
if (Pos()>=Len()) return EOF;
c1 = Data() [Pos()++];
if (Pos()>=Len()) return EOF;
c0 = Data() [Pos()++];
return(((int)c3<<24) + ((int)c2<<16) + ((int)c1<<8) + c0);
}

//------------------------------------------------------------------------------

int CIFF::ParseHeader (int len, tIFFBitmapHeader *bmheader)
{
bmheader->w = GetWord ();
bmheader->h = GetWord ();
bmheader->x = GetWord ();
bmheader->y = GetWord ();
bmheader->nplanes = GetByte ();
bmheader->masking = GetByte ();
bmheader->compression = GetByte ();
GetByte ();        /* skip pad */
bmheader->transparentcolor = GetWord ();
bmheader->xaspect = GetByte ();
bmheader->yaspect = GetByte ();
bmheader->pagewidth = GetWord ();
bmheader->pageheight = GetWord ();
m_transparentColor = (char) bmheader->transparentcolor;
m_hasTransparency = 0;
if (bmheader->masking == mskHasTransparentColor)
	m_hasTransparency = 1;
else if (bmheader->masking != mskNone && bmheader->masking != mskHasMask)
	return IFF_UNKNOWN_MASK;
return IFF_NO_ERROR;
}
//------------------------------------------------------------------------------
//  the buffer pointed to by raw_data is stuffed with a pointer to decompressed pixel data

int CIFF::ParseBody (int len, tIFFBitmapHeader *bmheader)
{
	unsigned char  *p = bmheader->raw_data;
	int				width, depth;
	signed char		n;
	int				nn, wid_cnt, end_cnt, plane;
	char				ignore = 0;
	unsigned char	*data_end;
	int				end_pos;

#if DBG
	int rowCount=0;
#endif

width = 0;
depth = 0;
end_pos = Pos() + len;
if (len&1)
	end_pos++;
if (bmheader->nType == TYPE_PBM) {
	width=bmheader->w;
	depth=1;
	}
else if (bmheader->nType == TYPE_ILBM) {
	width = (bmheader->w+7)/8;
	depth=bmheader->nplanes;
	}
end_cnt = (width&1)?-1:0;
data_end = p + width*bmheader->h*depth;
if (bmheader->compression == cmpNone) {        /* no compression */
	for (int y = bmheader->h; y; y--) {
		for (int x = 0; x < width * depth; x++)
			*p++= Data() [Pos()++];
		if (bmheader->masking == mskHasMask)
			Pos() += width;				//skip mask!
		if (bmheader->w & 1) 
			Pos()++;
		}
	}
else if (bmheader->compression == cmpByteRun1)
	for (wid_cnt = width, plane = 0;Pos() < end_pos && p < data_end; ) {
		unsigned char c;
		if (wid_cnt == end_cnt) {
			wid_cnt = width;
			plane++;
			if ((bmheader->masking == mskHasMask && plane==depth+1) ||
				 (bmheader->masking != mskHasMask && plane==depth))
				plane=0;
			}
		Assert(wid_cnt > end_cnt);
	n=Data() [Pos()++];
	if (n >= 0) {                       // copy next n+1 bytes from source, they are not compressed
		nn = (int) n+1;
		wid_cnt -= nn;
		if (wid_cnt==-1) {
			--nn; 
			Assert(width&1);
			}
		if (plane==depth)	//masking row
			Pos() += nn;
		else
			while (nn--) 
				*p++= Data() [Pos()++];
			if (wid_cnt==-1) 
				Pos()++;
		}
	else if (n>=-127) {             // next -n + 1 bytes are following byte
		c=Data() [Pos()++];
		nn = (int) -n+1;
		wid_cnt -= nn;
		if (wid_cnt==-1) {
			--nn; 
			Assert(width&1);
			}
		if (plane!=depth)	{//not masking row
			memset(p, c, nn); 
			p += nn;
			}
		}
#if DBG
	if ((p-bmheader->raw_data) % width == 0)
		rowCount++;
	Assert((p-bmheader->raw_data) - (width*rowCount) < width);
#endif
	}
if (p!=data_end)				//if we don't have the whole bitmap...
	return IFF_CORRUPT;		//...the give an error
//Pretend we read the whole chuck, because if we didn't, it's because
//we didn't read the last mask like or the last rle record for padding
//or whatever and it's not important, because we check to make sure
//we got the while bitmap, and that's what really counts.
Pos() = end_pos;
if (ignore) 
	ignore++;   // haha, suppress the evil warning message
return IFF_NO_ERROR;
}


//------------------------------------------------------------------------------
//modify passed bitmap
int CIFF::ParseDelta (int len, tIFFBitmapHeader *bmheader)
{
	unsigned char  *p=bmheader->raw_data;
	int y;
	int chunk_end = Pos() + len;

	GetByte ();		//longword, seems to be equal to 4.  Don't know what it is

for (y=0;y<bmheader->h;y++) {
	ubyte n_items;
	int cnt = bmheader->w;
	ubyte code;
	n_items = GetByte ();
	while (n_items--) {
		code = GetByte ();
		if (code==0) {				//repeat
			ubyte rep = GetByte ();
			ubyte val = GetByte ();
			cnt -= rep;
			if (cnt==-1)
				rep--;
			while (rep--)
				*p++= val;
			}
		else if (code > 0x80) {	//skip
			cnt -= (code-0x80);
			p += (code-0x80);
			if (cnt==-1)
				p--;
			}
		else {						//literal
			cnt -= code;
			if (cnt==-1)
				code--;
			while (code--)
				*p++= GetByte ();
			if (cnt==-1)
				GetByte ();
			}
		}
	if (cnt == -1) {
		if (!bmheader->w&1)
			return IFF_CORRUPT;
		}
	else if (cnt)
		return IFF_CORRUPT;
	}
if (Pos() == chunk_end-1)		//pad
	Pos()++;
if (Pos() != chunk_end)
	return IFF_CORRUPT;
return IFF_NO_ERROR;
}

//------------------------------------------------------------------------------
//  the buffer pointed to by raw_data is stuffed with a pointer to bitplane pixel data
void CIFF::SkipChunk (int len)
{
	//int c, i;
	int ilen;
	ilen = (len+1) & ~1;

////printf( "Skipping %d chunk\n", ilen );
Pos() += ilen;
if (Pos() >= Len())
	Pos() = Len();
}

//------------------------------------------------------------------------------
//read an ILBM or PBM file
// Pass pointer to opened file, and to empty bitmap_header structure, and form length
int CIFF::Parse (int formType, tIFFBitmapHeader *bmheader, int form_len, grsBitmap *prevBmP)
{
	int sig, len;
	//char ignore=0;
	int start_pos, end_pos;

start_pos = Pos();
end_pos = start_pos-4+form_len;
if (formType == pbm_sig)
	bmheader->nType = TYPE_PBM;
else
	bmheader->nType = TYPE_ILBM;
while ((Pos() < end_pos) && (sig=GetSig ()) != EOF) {
	len = GetByte ();
	if (len==EOF) 
		break;
	switch (sig) {
		case bmhd_sig: {
			int save_w=bmheader->w, save_h=bmheader->h;
			int ret = ParseHeader (len, bmheader);
			if (ret != IFF_NO_ERROR)
				return ret;
			if (bmheader->raw_data) {
				if (save_w != bmheader->w  ||  save_h != bmheader->h)
					return IFF_BM_MISMATCH;
				}
			else {
				MALLOC( bmheader->raw_data, ubyte, bmheader->w * bmheader->h );
				if (!bmheader->raw_data)
					return IFF_NO_MEM;
				}
			break;
			}

		case anhd_sig:
			if (!prevBmP) 
				return IFF_CORRUPT;
			bmheader->w = prevBmP->bmProps.w;
			bmheader->h = prevBmP->bmProps.h;
			bmheader->nType = prevBmP->bmProps.nType;
			MALLOC( bmheader->raw_data, ubyte, bmheader->w * bmheader->h );
			memcpy(bmheader->raw_data, prevBmP->bmTexBuf, bmheader->w * bmheader->h );
			SkipChunk(len);
			break;

		case cmap_sig: {
			int ncolors=(int) (len/3), cnum;
			unsigned char r, g, b;
			for (cnum=0;cnum<ncolors;cnum++) {
				r = Data() [Pos()++];
				g = Data() [Pos()++];
				b = Data() [Pos()++];
				bmheader->palette[cnum].r = r >> 2;
				bmheader->palette[cnum].g = g >> 2;
				bmheader->palette[cnum].b = b >> 2;
				}
			if (len & 1 ) 
				Pos()++;
			break;
			}

		case body_sig: {
			int r;
			if ((r=ParseBody (len, bmheader))!=IFF_NO_ERROR)
				return r;
			break;
			}

		case dlta_sig: {
			int r;
			if ((r=ParseDelta (len, bmheader))!=IFF_NO_ERROR)
				return r;
			break;
			}

		default:
			SkipChunk(len);
			break;
		}
	}
if (Pos() != start_pos-4+form_len)
	return IFF_CORRUPT;
return IFF_NO_ERROR;    /* ok! */
}

//------------------------------------------------------------------------------
//convert an ILBM file to a PBM file
int CIFF::ConvertToPBM (tIFFBitmapHeader *bmheader)
{
	int x, y, p;
	ubyte *new_data, *destptr, *rowptr;
	int bytes_per_row, byteofs;
	ubyte checkmask, newbyte, setbit;

MALLOC(new_data, ubyte, bmheader->w * bmheader->h);
if (new_data == NULL) 
	return IFF_NO_MEM;
destptr = new_data;
bytes_per_row = 2*((bmheader->w+15)/16);
for (y=0;y<bmheader->h;y++) {
	rowptr = &bmheader->raw_data[y * bytes_per_row * bmheader->nplanes];
	for (x=0, checkmask=0x80;x<bmheader->w;x++) {
		byteofs = x >> 3;
		for (p=newbyte=0, setbit=1;p<bmheader->nplanes;p++) {
			if (rowptr[bytes_per_row * p + byteofs] & checkmask)
				newbyte |= setbit;
			setbit <<= 1;
			}
		*destptr++= newbyte;
		if ((checkmask >>= 1) == 0) checkmask=0x80;
		}
	}
D2_FREE(bmheader->raw_data);
bmheader->raw_data = new_data;
bmheader->nType = TYPE_PBM;
return IFF_NO_ERROR;
}

//------------------------------------------------------------------------------

#define INDEX_TO_15BPP(i) ((short)((((palptr[(i)].r/2)&31)<<10)+(((palptr[(i)].g/2)&31)<<5)+((palptr[(i)].b/2 )&31)))

int CIFF::ConvertRgb15 (grsBitmap *bmP, tIFFBitmapHeader *bmheader)
{
	ushort *new_data;
	int x, y;
	int newptr = 0;
	tPalEntry *palptr;

palptr = bmheader->palette;
MALLOC(new_data, ushort, bmP->bmProps.w * bmP->bmProps.h * 2);
if (new_data == NULL)
	return IFF_NO_MEM;
for (y=0; y<bmP->bmProps.h; y++) {
	for (x=0; x<bmheader->w; x++)
		new_data[newptr++] = INDEX_TO_15BPP(bmheader->raw_data[y*bmheader->w+x]);
	}
D2_FREE(bmP->bmTexBuf);				//get rid of old-style data
bmP->bmTexBuf = (ubyte *) new_data;			//..ccAnd point to new data
bmP->bmProps.rowSize *= 2;				//two bytes per row
return IFF_NO_ERROR;
}

//------------------------------------------------------------------------------
//read in a entire file into a fake file structure
int CIFF::Open (const char *cfname)
{
	CFile fp;
	int	ret;

Data() = NULL;
if (!fp.Open(cfname, gameFolders.szDataDir, "rb", gameStates.app.bD1Mission))
	return IFF_NO_FILE;
Len () = fp.Length ();
MALLOC (Data(), ubyte, Len());
if (fp.Read (Data(), 1, Len()) < (size_t) Len())
	ret = IFF_READ_ERROR;
else
	ret = IFF_NO_ERROR;
Pos () = 0;
fp.Close();
return ret;
}

//------------------------------------------------------------------------------

void CIFF::Close (void)
{
if (Data ())
	D2_FREE (Data ());
Pos () = 0;
Len () = 0;
}

//------------------------------------------------------------------------------
//copy an iff header structure to a grsBitmap structure
void CIFF::CopyIffToGrs (grsBitmap *bmP, tIFFBitmapHeader *bmheader)
{
bmP->bmProps.x = bmP->bmProps.y = 0;
bmP->bmProps.w = bmheader->w;
bmP->bmProps.h = bmheader->h;
bmP->bmProps.nType = (char) bmheader->nType;
bmP->bmProps.rowSize = bmheader->w;
bmP->bmTexBuf = bmheader->raw_data;
bmP->bmProps.flags = 0;
}

//------------------------------------------------------------------------------
//if bmP->bmTexBuf is set, use it (making sure w & h are correct), else
//allocate the memory
int CIFF::ParseBitmap (grsBitmap *bmP, int bitmapType, grsBitmap *prevBmP)
{
	int ret;			//return code
	tIFFBitmapHeader bmheader;
	int sig, form_len;
	int formType;

bmheader.raw_data = bmP->bmTexBuf;
if (bmheader.raw_data) {
	bmheader.w = bmP->bmProps.w;
	bmheader.h = bmP->bmProps.h;
	}
sig=GetSig ();
if (sig != form_sig)
	return IFF_NOT_IFF;
form_len = GetLong ();
formType = GetSig ();
if (formType == anim_sig)
	ret = IFF_FORM_ANIM;
else if ((formType == pbm_sig) || (formType == ilbm_sig))
	ret = Parse (formType, &bmheader, form_len, prevBmP);
else
	ret = IFF_UNKNOWN_FORM;
if (ret != IFF_NO_ERROR) {		//got an error parsing
	if (bmheader.raw_data) 
		D2_FREE(bmheader.raw_data);
	return ret;
	}
//If IFF file is ILBM, convert to PPB
if (bmheader.nType == TYPE_ILBM) {
	ret = ConvertToPBM (&bmheader);
	if (ret != IFF_NO_ERROR)
		return ret;
	}
//Copy data from tIFFBitmapHeader structure into grsBitmap structure
CopyIffToGrs (bmP, &bmheader);
bmP->bmPalette = AddPalette ((ubyte *) &bmheader.palette);
//Now do post-process if required
if (bitmapType == BM_RGB15)
	ret = ConvertRgb15 (bmP, &bmheader);
return ret;
}

//------------------------------------------------------------------------------
//returns error codes - see IFF.H.  see GR[HA] for bitmapType
int CIFF::ReadBitmap (const char *cfname, grsBitmap *bmP, int bitmapType)
{
	int ret;			//return code

ret = Open (cfname);		//read in entire file
if (ret == IFF_NO_ERROR) {
	bmP->bmTexBuf = NULL;
	ret = ParseBitmap (bmP, bitmapType, NULL);
	}
Close ();
return ret;
}

//------------------------------------------------------------------------------
//like iff_read_bitmap(), but reads into a bitmap that already exists, 
//without allocating memory for the bitmap.
int CIFF::ReplaceBitmap (const char *cfname, grsBitmap *bmP)
{
	int ret;			//return code

ret = Open (cfname);		//read in entire file
if (ret == IFF_NO_ERROR)
	ret = ParseBitmap (bmP, bmP->bmProps.nType, NULL);
Close();
return ret;
}

#define BMHD_SIZE 20

//------------------------------------------------------------------------------

int CIFF::WriteHeader (FILE *fp, tIFFBitmapHeader *bitmap_header)
{
PutSig(bmhd_sig, fp);
PutLong ((int) BMHD_SIZE, fp);
PutWord (bitmap_header->w, fp);
PutWord (bitmap_header->h, fp);
PutWord (bitmap_header->x, fp);
PutWord (bitmap_header->y, fp);
PutByte (bitmap_header->nplanes, fp);
PutByte (bitmap_header->masking, fp);
PutByte (bitmap_header->compression, fp);
PutByte (0, fp);	/* pad */
PutWord (bitmap_header->transparentcolor, fp);
PutByte (bitmap_header->xaspect, fp);
PutByte (bitmap_header->yaspect, fp);
PutWord (bitmap_header->pagewidth, fp);
PutWord (bitmap_header->pageheight, fp);
return IFF_NO_ERROR;
}

//------------------------------------------------------------------------------

int CIFF::WritePalette (FILE *fp, tIFFBitmapHeader *bitmap_header)
{
	int	i;

	int n_colors = 1<<bitmap_header->nplanes;

PutSig(cmap_sig, fp);
PutLong(3 * n_colors, fp);
for (i=0; i<256; i++) {
	unsigned char r, g, b;
	r = bitmap_header->palette[i].r * 4 + (bitmap_header->palette[i].r?3:0);
	g = bitmap_header->palette[i].g * 4 + (bitmap_header->palette[i].g?3:0);
	b = bitmap_header->palette[i].b * 4 + (bitmap_header->palette[i].b?3:0);
	fputc(r, fp);
	fputc(g, fp);
	fputc(b, fp);
	}
return IFF_NO_ERROR;
}

//------------------------------------------------------------------------------

int CIFF::RLESpan (ubyte *dest, ubyte *src, int len)
{
	int n, lit_cnt, rep_cnt;
	ubyte last, *cnt_ptr, *dptr;

cnt_ptr = 0;
dptr = dest;
last = src[0]; 
lit_cnt = 1;
for (n = 1; n < len; n++) {
	if (src [n] != last) 
		n--;
	else {
		rep_cnt = 2;
		n++;
		while ((n < len) && (rep_cnt < 128) && (src [n] == last)) {
			n++; 
			rep_cnt++;
			}
		if (rep_cnt > 2 || lit_cnt < 2) {
			if (lit_cnt > 1) {
				*cnt_ptr = lit_cnt-2; 
				--dptr;
				}		//save old lit count
			*dptr++= -(rep_cnt-1);
			*dptr++= last;
			last = src [n];
			lit_cnt = (n < len) ? 1 : 0;
			continue;		//go to next char
			} 
		}
	if (lit_cnt==1) {
		cnt_ptr = dptr++;		//save place for count
		*dptr++=last;			//store first char
		}
	*dptr++= last = src[n];
	if (lit_cnt == 127) {
		*cnt_ptr = lit_cnt;
		lit_cnt = 1;
		last = src[++n];
		}
	else 
		lit_cnt++;
	}

if (lit_cnt==1) {
	*dptr++= 0;
	*dptr++=last;			//store first char
	}
else if (lit_cnt > 1)
	*cnt_ptr = lit_cnt-1;
return (int) (dptr - dest);
}

#define EVEN(a) ((a+1)&0xfffffffel)

//------------------------------------------------------------------------------

//returns length of chunk
int CIFF::WriteBody (FILE *fp, tIFFBitmapHeader *bitmap_header, int bCompression)
{
	int w=bitmap_header->w, h=bitmap_header->h;
	int y, odd=w&1;
	int len = EVEN(w) * h, newlen, total_len=0;
	ubyte *p=bitmap_header->raw_data, *new_span;
	int save_pos;

PutSig(body_sig, fp);
save_pos = ftell(fp);
PutLong(len, fp);
MALLOC( new_span, ubyte, bitmap_header->w + (bitmap_header->w/128+2)*2);
if (new_span == NULL) 
	return IFF_NO_MEM;
for (y=bitmap_header->h;y--;) {
	if (bCompression) {
		total_len += newlen = RLESpan (new_span, p, bitmap_header->w+odd);
		fwrite(new_span, newlen, 1, fp);
		}
	else
		fwrite(p, bitmap_header->w+odd, 1, fp);
	p+=bitmap_header->row_size;	//bitmap_header->w;
	}
if (bCompression) {		//write actual data length
	Assert(fseek(fp, save_pos, SEEK_SET)==0);
	PutLong(total_len, fp);
	Assert(fseek(fp, total_len, SEEK_CUR)==0);
	if (total_len&1) fputc(0, fp);		//pad to even
	}
D2_FREE(new_span);
return ((bCompression) ? (EVEN(total_len)+8) : (len+8));
}


//------------------------------------------------------------------------------

int CIFF::WritePbm (FILE *fp, tIFFBitmapHeader *bitmap_header, int bCompression)			/* writes a pbm iff file */
{
	int ret;
	int raw_size = EVEN(bitmap_header->w) * bitmap_header->h;
	int body_size, tiny_size, pbm_size = 4 + BMHD_SIZE + 8 + EVEN(raw_size) + sizeof(tPalEntry)*(1<<bitmap_header->nplanes)+8;
	int save_pos;

////printf("write_pbm\n");

PutSig(form_sig, fp);
save_pos = ftell(fp);
PutLong(pbm_size+8, fp);
PutSig(pbm_sig, fp);
ret = WriteHeader (fp, bitmap_header);
if (ret != IFF_NO_ERROR) 
	return ret;
ret = WritePalette(fp, bitmap_header);
if (ret != IFF_NO_ERROR) 
	return ret;
tiny_size = 0;
body_size = WriteBody(fp, bitmap_header, bCompression);
pbm_size = 4 + BMHD_SIZE + body_size + tiny_size + sizeof(tPalEntry)*(1<<bitmap_header->nplanes)+8;
Assert(fseek(fp, save_pos, SEEK_SET)==0);
PutLong(pbm_size+8, fp);
Assert(fseek(fp, pbm_size+8, SEEK_CUR)==0);
return ret;
}

//------------------------------------------------------------------------------
//writes an IFF file from a grsBitmap structure. writes palette if not null
//returns error codes - see IFF.H.
int CIFF::WriteBitmap (const char *cfname, grsBitmap *bmP, ubyte *palette)
{
	FILE					*fp;
	tIFFBitmapHeader bmheader;
	int					ret;
	int					bCompression;

if (bmP->bmProps.nType == BM_RGB15) return IFF_BAD_BM_TYPE;
#if COMPRESS
	bCompression = (bmP->bmProps.w>=MIN_COMPRESS_WIDTH);
#else
	bCompression = 0;
#endif
//fill in values in bmheader
bmheader.x = bmheader.y = 0;
bmheader.w = bmP->bmProps.w;
bmheader.h = bmP->bmProps.h;
bmheader.nType = TYPE_PBM;
bmheader.transparentcolor = m_transparentColor;
bmheader.pagewidth = bmP->bmProps.w;	//I don't think it matters what I write
bmheader.pageheight = bmP->bmProps.h;
bmheader.nplanes = 8;
bmheader.masking = mskNone;
if (m_hasTransparency)	{
	 bmheader.masking |= mskHasTransparentColor;
	}
bmheader.compression = (bCompression?cmpByteRun1:cmpNone);
bmheader.xaspect = bmheader.yaspect = 1;	//I don't think it matters what I write
bmheader.raw_data = bmP->bmTexBuf;
bmheader.row_size = bmP->bmProps.rowSize;
if (palette) 
	memcpy(&bmheader.palette, palette, 256*3);
//open file and write
if (!(fp = fopen(cfname, "wb"))) 
	ret=IFF_NO_FILE;
else
	ret = WritePbm (fp, &bmheader, bCompression);
fclose(fp);
return ret;
}

//------------------------------------------------------------------------------
//read in many brushes.  fills in array of pointers, and n_bitmaps.
//returns iff error codes
int CIFF::ReadAnimBrush (const char *cfname, grsBitmap **bm_list, int max_bitmaps, int *n_bitmaps)
{
	int					ret;			//return code
	tIFFBitmapHeader bmheader;
	int					sig, form_len;
	int					formType;

*n_bitmaps=0;
ret = Open (cfname);		//read in entire file
if (ret != IFF_NO_ERROR) 
	goto done;
bmheader.raw_data = NULL;
sig=GetSig();
form_len = GetLong();
if (sig != form_sig) {
	ret = IFF_NOT_IFF;
	goto done;
	}
formType = GetSig();
if ((formType == pbm_sig) || (formType == ilbm_sig))
	ret = IFF_FORM_BITMAP;
else if (formType == anim_sig) {
	int anim_end = Pos() + form_len - 4;
		while (Pos() < anim_end && *n_bitmaps < max_bitmaps) {
			grsBitmap *prevBmP;
			prevBmP = *n_bitmaps>0?bm_list[*n_bitmaps-1]:NULL;
			MALLOC(bm_list[*n_bitmaps] , grsBitmap, 1 );
			bm_list[*n_bitmaps]->bmTexBuf = NULL;
			ret = ParseBitmap (bm_list[*n_bitmaps], formType, prevBmP);
			if (ret != IFF_NO_ERROR)
				goto done;
			(*n_bitmaps)++;
			}
		if (Pos() < anim_end)	//ran out of room
			ret = IFF_TOO_MANY_BMS;
		}
	else
		ret = IFF_UNKNOWN_FORM;

done:

Close();
return ret;
}

//------------------------------------------------------------------------------
//text for error messges
const char error_messages[] = {
	"No error.\0"
	"Not enough mem for loading or processing bitmap.\0"
	"IFF file has unknown FORM nType.\0"
	"Not an IFF file.\0"
	"Cannot open file.\0"
	"Tried to save invalid nType, like BM_RGB15.\0"
	"Bad data in file.\0"
	"ANIM file cannot be loaded with Normal bitmap loader.\0"
	"Normal bitmap file cannot be loaded with anim loader.\0"
	"Array not big enough on anim brush read.\0"
	"Unknown mask nType in bitmap header.\0"
	"Error reading file.\0"
	"No exit found.\0"
};


//------------------------------------------------------------------------------
//function to return pointer to error message
const char *CIFF::ErrorMsg (int nError)
{
const char *p = error_messages;

while (nError--) {
	if (!p) 
		return NULL;
	p += (int) strlen(p)+1;
	}
return p;
}

//------------------------------------------------------------------------------
//eof
