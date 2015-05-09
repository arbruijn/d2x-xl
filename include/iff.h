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

#ifndef _IFF_H
#define _IFF_H

#include "pstypes.h"
#include "gr.h"

//Palette entry structure
typedef struct tPalEntry {
	int8_t r, g, b;
} __pack__ tPalEntry;

//structure of the header in the file
typedef struct tIFFBitmapHeader {
	int16_t w, h;						//width and height of this bitmap
	int16_t x, y;						//generally unused
	int16_t nType;						//see types above
	int16_t transparentColor;		//which color is transparent (if any)
	int16_t pagewidth, pageheight; //width & height of source screen
	int8_t nplanes;              //number of planes (8 for 256 color image)
	int8_t masking, compression;  //see constants above
	int8_t xaspect, yaspect;      //aspect ratio (usually 5/6)
	tPalEntry palette [256];		//the palette for this bitmap
	uint8_t *raw_data;				//ptr to array of data
	int16_t row_size;				//offset to next row
} __pack__ tIFFBitmapHeader;

typedef struct tMemoryFile {
	uint8_t *data;
	int32_t	position;
	int32_t	length;
} tMemoryFile;

class CIFF {
	private:
		tMemoryFile	m_file;
		uint8_t m_transparentColor;
		uint8_t m_hasTransparency;	// 0=no transparency, 1=iff_transparent_color is valid

public:
		CIFF () { memset (&m_file, 0, sizeof (m_file)); }
		~CIFF () { Close (); }
		int32_t Open (const char *cfname);
		void Close ();
		int32_t GetSig ();
		int32_t GetBytes (char* dest, int32_t len);
		char GetByte ();
		int32_t GetWord ();
		int32_t GetLong ();

		int32_t ReadBitmap (const char *cfname, CBitmap *pBm, int32_t bitmapType);
		int32_t ReplaceBitmap (const char *cfname, CBitmap *pBm);
		int32_t ReadAnimBrush (const char *cfname, CBitmap **bm_list, int32_t max_bitmaps, int32_t *n_bitmaps);

		int32_t ParseBitmap (CBitmap *pBm, int32_t bitmapType, CBitmap *prevBmP);
		int32_t Parse (int32_t formType, tIFFBitmapHeader *bmheader, int32_t form_len, CBitmap *prevBmP);
		int32_t ParseHeader (int32_t len, tIFFBitmapHeader *bmheader);
		int32_t ParseBody (int32_t len, tIFFBitmapHeader *bmheader);
		int32_t ParseDelta (int32_t len, tIFFBitmapHeader *bmheader);
		void SkipChunk (int32_t len);
		int32_t ConvertToPBM (tIFFBitmapHeader *bmheader);
		int32_t ConvertRgb15 (CBitmap *pBm, tIFFBitmapHeader *bmheader);
		void CopyIffToBitmap (CBitmap *pBm, tIFFBitmapHeader *bmheader);

		int32_t WriteBitmap (const char *cfname, CBitmap *pBm, uint8_t *palette);
		int32_t WriteHeader (FILE *fp, tIFFBitmapHeader *bitmap_header);
		int32_t WritePalette (FILE *fp, tIFFBitmapHeader *bitmap_header);
		int32_t WriteBody (FILE *fp, tIFFBitmapHeader *bitmap_header, int32_t bCompression);
		int32_t WritePbm (FILE *fp, tIFFBitmapHeader *bitmap_header, int32_t bCompression);
		int32_t RLESpan (uint8_t *dest, uint8_t *src, int32_t len);

		inline uint8_t*& Data () { return m_file.data; }
		inline int32_t Pos (void) { return m_file.position; }
		inline int32_t Len (void) { return m_file.length; }
		inline int32_t Rem (void) { return Len () - Pos (); }
		inline void SetPos (int32_t position) { m_file.position = position; }
		inline void SetLen (int32_t length) { m_file.length = length; }
		inline int32_t NextPos (int32_t i = 1) { 
			int32_t p = m_file.position;
			if (Pos () < Len ()) 
				m_file.position += i;
			return p; 
			}

		inline uint8_t HasTransparency (void) { return m_hasTransparency; }
		inline uint8_t TransparentColor (void) { return m_transparentColor; }

		const char *ErrorMsg (int32_t nError);
};

//Error codes for read & write routines

#define IFF_NO_ERROR        0   //everything is fine, have a nice day
#define IFF_NO_MEM          1   //not enough mem for loading or processing
#define IFF_UNKNOWN_FORM    2   //IFF file, but not a bitmap
#define IFF_NOT_IFF         3   //this isn't even an IFF file
#define IFF_NO_FILE         4   //cannot find or open file
#define IFF_BAD_BM_TYPE     5   //tried to save invalid nType, like BM_RGB15
#define IFF_CORRUPT         6   //bad data in file
#define IFF_FORM_ANIM       7   //this is an anim, with non-anim load rtn
#define IFF_FORM_BITMAP     8   //this is not an anim, with anim load rtn
#define IFF_TOO_MANY_BMS    9   //anim read had more bitmaps than room for
#define IFF_UNKNOWN_MASK    10  //unknown masking nType
#define IFF_READ_ERROR      11  //error reading from file
#define IFF_BM_MISMATCH     12  //bm being loaded doesn't match bm loaded into

#endif

