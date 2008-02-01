#ifndef _TGA_H
#define _TGA_H

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

typedef struct {
    char  identSize;          // size of ID field that follows 18 char header (0 usually)
    char  colorMapType;      // nType of colour map 0=none, 1=has palette
    char  imageType;          // nType of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    short colorMapStart;     // first colour map entry in palette
    short colorMapLength;    // number of colours in palette
    char  colorMapBits;      // number of bits per palette entry 15,16,24,32

    short xStart;             // image x origin
    short yStart;             // image y origin
    short width;              // image width in pixels
    short height;             // image height in pixels
    char  bits;               // image bits per pixel 8,16,24,32
    char  descriptor;         // image descriptor bits (vh flip bits)
} tTgaHeader;

typedef struct tModelTextures {
	int					nBitmaps;
	char					**pszNames;
	grsBitmap			*pBitmaps;
} tModelTextures;

int ShrinkTGA (grsBitmap *bm, int xFactor, int yFactor, int bRealloc);
int ReadTGAHeader (CFILE *fp, tTgaHeader *ph, grsBitmap *pb);
int ReadTGAImage (CFILE *fp, tTgaHeader *ph, grsBitmap *pb, int alpha, 
						double brightness, int bGrayScale, int bRedBlueFlip);
int LoadTGA (CFILE *fp, grsBitmap *pb, int alpha, double brightness, 
				 int bGrayScale, int bRedBlueFlip);
int ReadTGA (char *pszFile, char *pszFolder, grsBitmap *pb, int alpha, 
				 double brightness, int bGrayScale, int bRedBlueFlip);
grsBitmap *CreateAndReadTGA (char *szFile);
int SaveTGA (char *pszFile, char *pszFolder, tTgaHeader *ph, grsBitmap *bmP);
double TGABrightness (grsBitmap *bmP);
void TGAChangeBrightness (grsBitmap *bmP, double dScale, int bInverse, int nOffset, int bSkipAlpha);
int TGAInterpolate (grsBitmap *bmP, int nScale);
int TGAMakeSquare (grsBitmap *bmP);
int ReadModelTGA (char *pszFile, grsBitmap *bmP, short nType, int bCustom);
int ReadModelTextures (tModelTextures *pt, int nType, int bCustom);
void FreeModelTextures (tModelTextures *pt);

#endif //_TGA_H
