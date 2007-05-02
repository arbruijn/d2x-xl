/* $Id: pcx.c,v 1.8 2003/06/16 06:57:34 btb Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gr.h"
#include "grdef.h"
#include "u_mem.h"
#include "pcx.h"
#include "cfile.h"
#include "palette.h"

int pcx_encode_byte(ubyte byt, ubyte cnt, CFILE *fid);
int pcx_encode_line(ubyte *inBuff, int inLen, CFILE *fp);

/* PCX Header data nType */
typedef struct {
	ubyte   Manufacturer;
	ubyte   Version;
	ubyte   Encoding;
	ubyte   BitsPerPixel;
	short   Xmin;
	short   Ymin;
	short   Xmax;
	short   Ymax;
	short   Hdpi;
	short   Vdpi;
	ubyte   ColorMap[16][3];
	ubyte   Reserved;
	ubyte   Nplanes;
	short   BytesPerLine;
	ubyte   filler[60];
} __pack__ PCXHeader;

#define PCXHEADER_SIZE 128

//------------------------------------------------------------------------------

#if 0//def FAST_FILE_IO /*disabled for a reason!*/
#define PCXHeader_read_n(ph, n, fp) CFRead(ph, sizeof(PCXHeader), n, fp)
#else
/*
 * reads n PCXHeader structs from a CFILE
 */
int PCXHeader_read_n(PCXHeader *ph, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		ph->Manufacturer = CFReadByte(fp);
		ph->Version = CFReadByte(fp);
		ph->Encoding = CFReadByte(fp);
		ph->BitsPerPixel = CFReadByte(fp);
		ph->Xmin = CFReadShort(fp);
		ph->Ymin = CFReadShort(fp);
		ph->Xmax = CFReadShort(fp);
		ph->Ymax = CFReadShort(fp);
		ph->Hdpi = CFReadShort(fp);
		ph->Vdpi = CFReadShort(fp);
		CFRead(&ph->ColorMap, 16*3, 1, fp);
		ph->Reserved = CFReadByte(fp);
		ph->Nplanes = CFReadByte(fp);
		ph->BytesPerLine = CFReadShort(fp);
		CFRead(&ph->filler, 60, 1, fp);
	}
	return i;
}
#endif

//------------------------------------------------------------------------------

int pcx_get_dimensions( char *filename, int *width, int *height)
{
	CFILE *PCXfile;
	PCXHeader header;

	PCXfile = CFOpen(filename, gameFolders.szDataDir, "rb", 0);
	if (!PCXfile) return PCX_ERROR_OPENING;

	if (PCXHeader_read_n(&header, 1, PCXfile) != 1) {
		CFClose(PCXfile);
		return PCX_ERROR_NO_HEADER;
	}
	CFClose(PCXfile);

	*width = header.Xmax - header.Xmin+1;
	*height = header.Ymax - header.Ymin+1;

	return PCX_ERROR_NONE;
}

//------------------------------------------------------------------------------

int PCXReadBitmap (char * filename, grsBitmap * bmP, int bitmapType, int bD1Mission)
{
	PCXHeader header;
	CFILE * PCXfile;
	int i, row, col, count, xsize, ysize;
	ubyte data, *pixdata;
    ubyte palette [768];
PCXfile = CFOpen( filename, gameFolders.szDataDir, "rb", bD1Mission );
if ( !PCXfile )
	return PCX_ERROR_OPENING;

// read 128 char PCX header
if (PCXHeader_read_n( &header, 1, PCXfile )!=1) {
	CFClose( PCXfile );
	return PCX_ERROR_NO_HEADER;
	}

// Is it a 256 color PCX file?
if ((header.Manufacturer != 10)||(header.Encoding != 1)||(header.Nplanes != 1)||(header.BitsPerPixel != 8)||(header.Version != 5))	{
	CFClose( PCXfile );
	return PCX_ERROR_WRONG_VERSION;
	}

// Find the size of the image
xsize = header.Xmax - header.Xmin + 1;
ysize = header.Ymax - header.Ymin + 1;

	if (palette && !bmP)
    {
        CFSeek( PCXfile, -768, SEEK_END );
        CFRead( palette, 3, 256, PCXfile );
        CFSeek( PCXfile, PCXHEADER_SIZE, SEEK_SET );
        for (i=0; i<768; i++ )
            palette [i] >>= 2;
		CFClose(PCXfile);
		return PCX_ERROR_NONE;
    }

	if ( bitmapType == BM_LINEAR )	{
		if ( bmP->bm_texBuf == NULL )	{
			GrInitBitmapAlloc (bmP, bitmapType, 0, 0, xsize, ysize, xsize, 1);
		}
	}

	if ( bmP->bm_props.nType == BM_LINEAR )	{
		for (row=0; row< ysize ; row++)      {
			pixdata = &bmP->bm_texBuf [bmP->bm_props.rowsize*row];
			for (col=0; col< xsize ; )      {
				if (CFRead( &data, 1, 1, PCXfile )!=1 )	{
					CFClose( PCXfile );
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0)     {
					count =  data & 0x3F;
					if (CFRead( &data, 1, 1, PCXfile )!=1 )	{
						CFClose( PCXfile );
						return PCX_ERROR_READING;
					}
					memset( pixdata, data, count );
					pixdata += count;
					col += count;
				} else {
					*pixdata++ = data;
					col++;
				}
			}
		}
	} else {
		for (row=0; row< ysize ; row++)      {
			for (col=0; col< xsize ; )      {
				if (CFRead( &data, 1, 1, PCXfile )!=1 )	{
					CFClose( PCXfile );
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0)     {
					count =  data & 0x3F;
					if (CFRead( &data, 1, 1, PCXfile )!=1 )	{
						CFClose( PCXfile );
						return PCX_ERROR_READING;
					}
					for (i=0;i<count;i++)
						gr_bm_pixel( bmP, col+i, row, data );
					col += count;
				} else {
					gr_bm_pixel( bmP, col, row, data );
					col++;
				}
			}
		}
	}

// Read the extended palette at the end of PCX file
// Read in a character which should be 12 to be extended palette file
if (CFRead( &data, 1, 1, PCXfile )==1)	{
	if ( data == 12 )	{
		if (CFRead(palette,768, 1, PCXfile)!=1)	{
			CFClose( PCXfile );
			return PCX_ERROR_READING;
			}
		for (i=0; i<768; i++ )
			palette[i] >>= 2;
		}
	}
else {
	CFClose( PCXfile );
	return PCX_ERROR_NO_PALETTE;
	}
bmP->bm_palette = AddPalette (palette);
CFClose(PCXfile);
return PCX_ERROR_NONE;
}

//------------------------------------------------------------------------------

int pcx_write_bitmap( char * filename, grsBitmap * bmP)
{
	int retval;
	int i;
	ubyte data;
	PCXHeader header;
	CFILE *PCXfile;
	tPalette	palette;

	memset( &header, 0, PCXHEADER_SIZE );

	header.Manufacturer = 10;
	header.Encoding = 1;
	header.Nplanes = 1;
	header.BitsPerPixel = 8;
	header.Version = 5;
	header.Xmax = bmP->bm_props.w-1;
	header.Ymax = bmP->bm_props.h-1;
	header.BytesPerLine = bmP->bm_props.w;

	PCXfile = CFOpen(filename, gameFolders.szDataDir, "wb", 0);
	if ( !PCXfile )
		return PCX_ERROR_OPENING;

	if (CFWrite(&header, PCXHEADER_SIZE, 1, PCXfile) != 1)
	{
		CFClose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	for (i=0; i<bmP->bm_props.h; i++ )	{
		if (!pcx_encode_line( &bmP->bm_texBuf[bmP->bm_props.rowsize*i], bmP->bm_props.w, PCXfile ))	{
			CFClose(PCXfile);
			return PCX_ERROR_WRITING;
		}
	}

	// Mark an extended palette
	data = 12;
	if (CFWrite(&data, 1, 1, PCXfile) != 1)
	{
		CFClose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	// Write the extended palette
	for (i=0; i<768; i++ )
		palette[i] = bmP->bm_palette [i] << 2;

	retval = CFWrite(palette, 768, 1, PCXfile);

	for (i=0; i<768; i++ )
		palette[i] >>= 2;

	if (retval !=1)	{
		CFClose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	CFClose(PCXfile);
	return PCX_ERROR_NONE;

}

//------------------------------------------------------------------------------
// returns number of bytes written into outBuff, 0 if failed
int pcx_encode_line(ubyte *inBuff, int inLen, CFILE *fp)
{
	ubyte this, last;
	int srcIndex, i;
	register int total;
	register ubyte runCount; 	// max single runlength is 63
	total = 0;
	last = *(inBuff);
	runCount = 1;

	for (srcIndex = 1; srcIndex < inLen; srcIndex++) {
		this = *(++inBuff);
		if (this == last)	{
			runCount++;			// it encodes
			if (runCount == 63)	{
				if (!(i=pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
				runCount = 0;
			}
		} else {   	// this != last
			if (runCount)	{
				if (!(i=pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
			}
			last = this;
			runCount = 1;
		}
	}

	if (runCount)	{		// finish up
		if (!(i=pcx_encode_byte(last, runCount, fp)))
			return 0;
		return total + i;
	}
	return total;
}

//------------------------------------------------------------------------------
// subroutine for writing an encoded byte pair
// returns count of bytes written, 0 if error
int pcx_encode_byte(ubyte byt, ubyte cnt, CFILE *fid)
{
	if (cnt) {
		if ( (cnt==1) && (0xc0 != (0xc0 & byt)) )	{
			if(EOF == CFPutC((int)byt, fid))
				return 0; 	// disk write error (probably full)
			return 1;
		} else {
			if(EOF == CFPutC((int)0xC0 | cnt, fid))
				return 0; 	// disk write error
			if(EOF == CFPutC((int)byt, fid))
				return 0; 	// disk write error
			return 2;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
//text for error messges
char pcx_error_messages[] = {
	"No error.\0"
	"Error opening file.\0"
	"Couldn't read PCX header.\0"
	"Unsupported PCX version.\0"
	"Error reading data.\0"
	"Couldn't find palette information.\0"
	"Error writing data.\0"
};


//------------------------------------------------------------------------------
//function to return pointer to error message
char *pcx_errormsg(int error_number)
{
	char *p = pcx_error_messages;

while (error_number--) {
	if (!p)
		return NULL;
	p += (int) strlen(p)+1;
	}
return p;
}

//------------------------------------------------------------------------------
// fullscreen loading, 10/14/99 Jan Bobrowski

int pcx_read_fullscr(char * filename, int bD1Mission)
{
	int			pcx_error;
	grsBitmap	bm;

GrInitBitmapData (&bm);
BM_MASK (&bm) = NULL;
pcx_error = PCXReadBitmap (filename, &bm, BM_LINEAR, bD1Mission);
if (pcx_error == PCX_ERROR_NONE) {
	GrPaletteStepLoad (NULL);
	show_fullscr (&bm);
	GrFreeBitmapData (&bm);
	}
return pcx_error;
}

//------------------------------------------------------------------------------
//eof