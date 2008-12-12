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
	sbyte r, g, b;
} tPalEntry;

//structure of the header in the file
typedef struct tIFFBitmapHeader {
	short w, h;						//width and height of this bitmap
	short x, y;						//generally unused
	short nType;						//see types above
	short transparentcolor;		//which color is transparent (if any)
	short pagewidth, pageheight; //width & height of source screen
	sbyte nplanes;              //number of planes (8 for 256 color image)
	sbyte masking, compression;  //see constants above
	sbyte xaspect, yaspect;      //aspect ratio (usually 5/6)
	tPalEntry palette[256];		//the palette for this bitmap
	ubyte *raw_data;				//ptr to array of data
	short row_size;				//offset to next row
} tIFFBitmapHeader;

typedef struct tMemoryFile {
	ubyte *data;
	int	position;
	int	length;
} tMemoryFile;

class CIFF {
	private:
		tMemoryFile	m_file;
		ubyte m_transparentColor;
		ubyte m_hasTransparency;	// 0=no transparency, 1=iff_transparent_color is valid

public:
		CIFF () { memset (&m_file, 0, sizeof (m_file)); }
		~CIFF () { Close (); }
		int Open (const char *cfname);
		void Close ();
		int GetSig ();
		char GetByte ();
		int GetWord ();
		int GetLong ();

		int ReadBitmap (const char *cfname, CBitmap *bmP, int bitmapType);
		int ReplaceBitmap (const char *cfname, CBitmap *bmP);
		int ReadAnimBrush (const char *cfname, CBitmap **bm_list, int max_bitmaps, int *n_bitmaps);

		int ParseBitmap (CBitmap *bmP, int bitmapType, CBitmap *prevBmP);
		int Parse (int formType, tIFFBitmapHeader *bmheader, int form_len, CBitmap *prevBmP);
		int ParseHeader (int len, tIFFBitmapHeader *bmheader);
		int ParseBody (int len, tIFFBitmapHeader *bmheader);
		int ParseDelta (int len, tIFFBitmapHeader *bmheader);
		void SkipChunk (int len);
		int ConvertToPBM (tIFFBitmapHeader *bmheader);
		int ConvertRgb15 (CBitmap *bmP, tIFFBitmapHeader *bmheader);
		void CopyIffToBitmap (CBitmap *bmP, tIFFBitmapHeader *bmheader);

		int WriteBitmap (const char *cfname, CBitmap *bmP, ubyte *palette);
		int WriteHeader (FILE *fp, tIFFBitmapHeader *bitmap_header);
		int WritePalette (FILE *fp, tIFFBitmapHeader *bitmap_header);
		int WriteBody (FILE *fp, tIFFBitmapHeader *bitmap_header, int bCompression);
		int WritePbm (FILE *fp, tIFFBitmapHeader *bitmap_header, int bCompression);
		int RLESpan (ubyte *dest, ubyte *src, int len);

		inline ubyte*& Data () { return m_file.data; }
		inline int Pos () { return m_file.position; }
		inline int Len () { return m_file.length; }
		inline void SetPos (int position) { m_file.position = position; }
		inline void SetLen (int length) { m_file.length = length; }
		inline int NextPos () { return (Pos () < Len ()) ? m_file.position++ : m_file.position; }

		inline ubyte HasTransparency (void) { return m_hasTransparency; }
		inline ubyte TransparentColor (void) { return m_transparentColor; }

		const char *ErrorMsg (int nError);
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

