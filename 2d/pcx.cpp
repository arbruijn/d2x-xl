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
#include "tga.h"
#include "palette.h"

int PCXEncodeByte (ubyte byt, ubyte cnt, CFile& cf);
int PCXEncodeLine (ubyte *inBuff, int inLen, CFile& cf);

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
#define PCXHeaderReadN (ph, n, cf) cf.Read (ph, sizeof (PCXHeader), n, cf)
#else
/*
 * reads n PCXHeader structs from a CFile
 */
int PCXHeaderReadN (PCXHeader *ph, int n, CFile& cf)
{
	int i;

	for (i = 0; i < n; i++) {
		ph->Manufacturer = cf.ReadByte ();
		ph->Version = cf.ReadByte ();
		ph->Encoding = cf.ReadByte ();
		ph->BitsPerPixel = cf.ReadByte ();
		ph->Xmin = cf.ReadShort ();
		ph->Ymin = cf.ReadShort ();
		ph->Xmax = cf.ReadShort ();
		ph->Ymax = cf.ReadShort ();
		ph->Hdpi = cf.ReadShort ();
		ph->Vdpi = cf.ReadShort ();
		cf.Read (&ph->ColorMap, 16*3, 1);
		ph->Reserved = cf.ReadByte ();
		ph->Nplanes = cf.ReadByte ();
		ph->BytesPerLine = cf.ReadShort ();
		cf.Read (&ph->filler, 60, 1);
	}
	return i;
}
#endif

//------------------------------------------------------------------------------

int PCXGetDimensions (const char *filename, int *width, int *height)
{
	CFile cf;
	PCXHeader header;

if (!cf.Open (filename, gameFolders.szDataDir, "rb", 0))
	return PCX_ERROR_OPENING;

	if (PCXHeaderReadN (&header, 1, cf) != 1) {
		cf.Close ();
		return PCX_ERROR_NO_HEADER;
	}
	cf.Close ();

	*width = header.Xmax - header.Xmin+1;
	*height = header.Ymax - header.Ymin+1;

	return PCX_ERROR_NONE;
}

//------------------------------------------------------------------------------

int PCXReadBitmap (const char * filename, CBitmap * bmP, int bitmapType, int bD1Mission)
{
	PCXHeader 	header;
	CFile 		cf;
	int 			i, row, col, count, xsize, ysize;
	ubyte			data, *pixdata;
	CPalette		palette;

if (!cf.Open (filename, gameFolders.szDataDir, "rb", bD1Mission))
	return PCX_ERROR_OPENING;

// read 128 char PCX header
if (PCXHeaderReadN (&header, 1,  cf)!=1) {
	cf.Close ();
	return PCX_ERROR_NO_HEADER;
	}

// Is it a 256 color PCX file?
if ((header.Manufacturer != 10)|| (header.Encoding != 1)|| (header.Nplanes != 1)|| (header.BitsPerPixel != 8)|| (header.Version != 5))	{
	cf.Close ();
	return PCX_ERROR_WRONG_VERSION;
	}

// Find the size of the image
xsize = header.Xmax - header.Xmin + 1;
ysize = header.Ymax - header.Ymin + 1;

if (!bmP) {
	cf.Seek (-PALETTE_SIZE * 3, SEEK_END);
	palette.Read (cf);
	cf.Seek (PCXHEADER_SIZE, SEEK_SET);
	for (i = 0; i < PALETTE_SIZE * 3; i++)
		palette.Raw () [i] >>= 2;
	cf.Close ();
	return PCX_ERROR_NONE;
	}

if (bitmapType == BM_LINEAR) {
	if (bmP->TexBuf () == NULL) 
		bmP->Setup (bitmapType, xsize, ysize, 1);

	}

if (bmP->Mode () == BM_LINEAR) {
	for (row = 0; row < ysize ; row++) {
		pixdata = bmP->TexBuf (bmP->RowSize () * row);
		for (col = 0; col < xsize ;) {
			if (cf.Read (&data, 1, 1) != 1) {
				cf.Close ();
				return PCX_ERROR_READING;
				}
			if ((data & 0xC0) == 0xC0) {
				count =  data & 0x3F;
				if (cf.Read (&data, 1, 1) != 1) {
					cf.Close ();
					return PCX_ERROR_READING;
					}
				memset (pixdata, data, count);
				pixdata += count;
				col += count;
				}
			else {
				*pixdata++ = data;
				col++;
				}
			}
		}
	} 
else {
	for (row=0; row< ysize ; row++)      {
		for (col=0; col< xsize ;)      {
			if (cf.Read (&data, 1, 1)!=1)	{
				cf.Close ();
				return PCX_ERROR_READING;
				}
			if ( (data & 0xC0) == 0xC0)     {
				count =  data & 0x3F;
				if (cf.Read (&data, 1, 1)!=1)	{
					cf.Close ();
					return PCX_ERROR_READING;
					}
				for (i=0;i<count;i++)
					gr_bm_pixel (bmP, col+i, row, data);
				col += count;
				} 
			else {
				gr_bm_pixel (bmP, col, row, data);
				col++;
				}
			}
		}
	}

// Read the extended palette at the end of PCX file
// Read in a character which should be 12 to be extended palette file
if (cf.Read (&data, 1, 1)==1)	{
	if (data == 12)	{
		if (!palette.Read (cf)) {
			cf.Close ();
			return PCX_ERROR_READING;
			}
		for (i = 0; i < PALETTE_SIZE * 3; i++)
			palette.Data ().raw [i] >>= 2;
		}
	}
else {
	cf.Close ();
	return PCX_ERROR_NO_PALETTE;
	}
bmP->SetPalette (&palette);
cf.Close ();
return PCX_ERROR_NONE;
}

//------------------------------------------------------------------------------

int pcx_write_bitmap (const char * filename, CBitmap * bmP)
{
	int retval;
	int i;
	ubyte data;
	PCXHeader header;
	CFile cf;
	CPalette	palette;

	memset (&header, 0, PCXHEADER_SIZE);

	header.Manufacturer = 10;
	header.Encoding = 1;
	header.Nplanes = 1;
	header.BitsPerPixel = 8;
	header.Version = 5;
	header.Xmax = bmP->Width () - 1;
	header.Ymax = bmP->Height () - 1;
	header.BytesPerLine = bmP->Width ();

	if (!cf.Open (filename, gameFolders.szDataDir, "wb", 0))
		return PCX_ERROR_OPENING;

	if (cf.Write (&header, PCXHEADER_SIZE, 1) != 1)
	{
		cf.Close ();
		return PCX_ERROR_WRITING;
	}

	for (i=0; i < bmP->Height (); i++)	{
		if (!PCXEncodeLine (&bmP->TexBuf ()[bmP->RowSize ()*i], bmP->Width (), cf))	{
			cf.Close ();
			return PCX_ERROR_WRITING;
		}
	}

	// Mark an extended palette
	data = 12;
	if (cf.Write (&data, 1, 1) != 1)
	{
		cf.Close ();
		return PCX_ERROR_WRITING;
	}

	// Write the extended palette
	for (i=0; i<768; i++)
		palette.Raw () [i] = bmP->Palette ()->Raw ()  [i] << 2;

	retval = palette.Write (cf);

	for (i=0; i<768; i++)
		palette.Raw () [i] >>= 2;

	if (retval !=1)	{
		cf.Close ();
		return PCX_ERROR_WRITING;
	}

	cf.Close ();
	return PCX_ERROR_NONE;

}

//------------------------------------------------------------------------------
// returns number of bytes written into outBuff, 0 if failed
int PCXEncodeLine (ubyte *inBuff, int inLen, CFile& cf)
{
	ubyte current, last;
	int srcIndex, i;
	int total;
	ubyte runCount; 	// max single runlength is 63
	total = 0;
	last = * (inBuff);
	runCount = 1;

	for (srcIndex = 1; srcIndex < inLen; srcIndex++) {
		current = * (++inBuff);
		if (current == last)	{
			runCount++;			// it encodes
			if (runCount == 63)	{
				if (! (i=PCXEncodeByte (last, runCount, cf)))
					return (0);
				total += i;
				runCount = 0;
			}
		} else {   	// current != last
			if (runCount)	{
				if (! (i=PCXEncodeByte (last, runCount, cf)))
					return (0);
				total += i;
			}
			last = current;
			runCount = 1;
		}
	}

	if (runCount)	{		// finish up
		if (! (i=PCXEncodeByte (last, runCount, cf)))
			return 0;
		return total + i;
	}
	return total;
}

//------------------------------------------------------------------------------
// subroutine for writing an encoded byte pair
// returns count of bytes written, 0 if error
int PCXEncodeByte (ubyte byt, ubyte cnt, CFile& cf)
{
	if (cnt) {
		if ( (cnt==1) && (0xc0 != (0xc0 & byt)))	{
			if (EOF == cf.PutC ( (int)byt))
				return 0; 	// disk write error (probably full)
			return 1;
		} else {
			if (EOF == cf.PutC ( (int)0xC0 | cnt))
				return 0; 	// disk write error
			if (EOF == cf.PutC ( (int)byt))
				return 0; 	// disk write error
			return 2;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
//text for error messges
const char pcx_error_messages[] = {
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
const char *pcx_errormsg (int error_number)
{
	const char *p = pcx_error_messages;

while (error_number--) {
	if (!p)
		return NULL;
	p += (int) strlen (p) + 1;
	}
return p;
}

//------------------------------------------------------------------------------
// fullscreen loading, 10/14/99 Jan Bobrowski

int PcxReadFullScrImage (const char * filename, int bD1Mission)
{
		int		pcxError;
		CBitmap	bm;

if (strstr (filename, ".tga"))
	pcxError = ReadTGA (filename, gameFolders.szDataDir, &bm, -1, 1.0, 0, 0) ? PCX_ERROR_NONE : PCX_ERROR_OPENING;
else {
	bm.SetMask (NULL);
	pcxError = PCXReadBitmap (filename, &bm, BM_LINEAR, bD1Mission);
	}
if (pcxError == PCX_ERROR_NONE) {
	paletteManager.LoadEffect  ();
	ShowFullscreenImage (&bm);
	}
return pcxError;
}

//------------------------------------------------------------------------------
//eof
