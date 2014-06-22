#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "u_mem.h"
#include "gr.h"
#include "ogl_lib.h"
#include "renderthreads.h"
#include "tga.h"

#if USE_SDL_IMAGE
#	ifdef __macosx__
#	include <SDL_image/SDL_image.h>
#	else
#	include <SDL_image.h>
#	endif
#endif

#define MIN_OPACITY	224

//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------

int CTGAHeader::Read (CFile& cf, CBitmap *bmP)
{
m_data.identSize = char (cf.ReadByte ());
m_data.colorMapType = char (cf.ReadByte ());
m_data.imageType = char (cf.ReadByte ());
m_data.colorMapStart = cf.ReadShort ();
m_data.colorMapLength = cf.ReadShort ();
m_data.colorMapBits = char (cf.ReadByte ());
m_data.xStart = cf.ReadShort ();
m_data.yStart = cf.ReadShort ();
m_data.width = cf.ReadShort ();
m_data.height = cf.ReadShort ();
m_data.bits = char (cf.ReadByte ());
m_data.descriptor = char (cf.ReadByte ());
if (m_data.identSize && cf.Seek (m_data.identSize, SEEK_CUR))
	return 0;
if (bmP) {
	bmP->Init (0, 0, 0, m_data.width, m_data.height, m_data.bits / 8, NULL, false);
	}
return (m_data.width <= 4096) && (m_data.height <= 32768) && 
       (m_data.xStart < m_data.width) && (m_data.yStart < m_data.height) &&
		 ((m_data.bits == 8) || (m_data.bits == 16) || (m_data.bits == 24) || (m_data.bits == 32));
}

//---------------------------------------------------------------

int CTGAHeader::Write (CFile& cf, CBitmap *bmP)
{
memset (&m_data, 0, sizeof (m_data));
m_data.width = bmP->Width ();
m_data.height = bmP->Height ();
m_data.bits = bmP->BPP () * 8;
m_data.imageType = 2;
cf.WriteByte (m_data.identSize);
cf.WriteByte (m_data.colorMapType);
cf.WriteByte (m_data.imageType);
cf.WriteShort (m_data.colorMapStart);
cf.WriteShort (m_data.colorMapLength);
cf.WriteByte (m_data.colorMapBits);
cf.WriteShort (m_data.xStart);
cf.WriteShort (m_data.yStart);
cf.WriteShort (m_data.width);
cf.WriteShort (m_data.height);
if (!bmP->HasTransparency ())
	m_data.bits = 24;
cf.WriteByte (m_data.bits);
cf.WriteByte (m_data.descriptor);
if (m_data.identSize)
	cf.Seek (m_data.identSize, SEEK_CUR);
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CTGA::Compress (void)
{
if (!ogl.m_features.bTextureCompression/*.Apply ()*/)
	return 0;
if (m_bmP->LoadTexture (0, 0, 0))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

void CTGA::ConvertToRGB (void)
{
if ((m_bmP->BPP () == 4) && m_bmP->Buffer ()) {
	CRGBColor *rgbP = reinterpret_cast<CRGBColor*> (m_bmP->Buffer ());
	CRGBAColor *rgbaP = reinterpret_cast<CRGBAColor*> (m_bmP->Buffer ());

	for (int i = m_bmP->Length () / 4; i; i--, rgbP++, rgbaP++) {
		rgbP->Red () = rgbaP->Red ();
		rgbP->Green () = rgbaP->Green ();
		rgbP->Blue () = rgbaP->Blue ();
		}
	m_bmP->Resize (uint (3 * m_bmP->Size () / 4));
	m_bmP->SetBPP (3);
	m_bmP->DelFlags (BM_FLAG_SEE_THRU | BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT);
	}
}

//------------------------------------------------------------------------------

void CTGA::PreMultiplyAlpha (float fScale)
{
if ((m_bmP->BPP () == 4) && m_bmP->Buffer ()) {
	CRGBAColor *rgbaP = reinterpret_cast<CRGBAColor*> (m_bmP->Buffer ());
	float alpha;

	for (int i = m_bmP->Length () / 4; i; i--, rgbaP++) {
		alpha = float (rgbaP->Alpha () / 255.0f) * fScale;
		rgbaP->Red () = (ubyte) FRound (rgbaP->Red () * alpha);
		rgbaP->Green () = (ubyte) FRound (rgbaP->Green () * alpha);
		rgbaP->Blue () = (ubyte) FRound (rgbaP->Blue () * alpha);
		}
	}
}

//------------------------------------------------------------------------------

void CTGA::SetProperties (int alpha, int bGrayScale, double brightness, bool bSwapRB)
{
	int			i, n, nAlpha = 0, nFrames;
	int			h = m_bmP->Height ();
	int			w = m_bmP->Width ();
	float			nVisible = 0;
	CFloatVector	avgColor;
	CRGBColor	avgColorb;
	float			a;

//CRGBAColor *p;
//if (!gameStates.app.bMultiThreaded)
// 	p = reinterpret_cast<CRGBAColor*> (m_bmP->Buffer ());

m_bmP->AddFlags (BM_FLAG_TGA);
m_bmP->SetTranspType (-1);
memset (m_bmP->TransparentFrames (), 0, 4 * sizeof (int));
memset (m_bmP->SuperTranspFrames (), 0, 4 * sizeof (int));
memset (&avgColor, 0, sizeof (avgColor));

m_bmP->DelFlags (BM_FLAG_SEE_THRU | BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT);
if (m_bmP->BPP () == 3) {
	CRGBColor*	p = reinterpret_cast<CRGBColor*> (m_bmP->Buffer ());
#if USE_OPENMP
	if (gameStates.app.bMultiThreaded) {
		int			tId, j = w * h;
		CFloatVector3	ac [MAX_THREADS];

		memset (ac, 0, sizeof (ac));
#	pragma omp parallel private (tId)
			{
			tId = omp_get_thread_num ();
#		pragma omp for reduction (+: nVisible)
			for (int i = 0; i < j; i++) {
				if (p [i].Red () || p [i].Green () || p [i].Blue ()) {
					::Swap (p [i].Red (), p [i].Blue ());
					ac [tId].Red () += p [i].Red ();
					ac [tId].Green () += p [i].Green ();
					ac [tId].Blue () += p [i].Blue ();
					nVisible++;
					}
				}
			}
		for (i = 0, j = gameStates.app.nThreads; i < j; i++) {
			avgColor.Red () += ac [i].Red ();
			avgColor.Green () += ac [i].Green ();
			avgColor.Blue () += ac [i].Blue ();
			}
		}
	else
#endif
	for (int i = 0, j = w * h; i < j; i++) {
		if (p [i].Red () || p [i].Green () || p [i].Blue ()) {
			::Swap (p [i].Red (), p [i].Blue ());
			avgColor.Red () += p [i].Red ();
			avgColor.Green () += p [i].Green ();
			avgColor.Blue () += p [i].Blue ();
			nVisible++;
			}
		}
	avgColor.Alpha () = 1.0f;
	}
else {
	int nSuperTransp;

	if (!(nFrames = h / w))
		nFrames = 1;
	for (n = 0; n < nFrames; n++) {
		nSuperTransp = 0;

	int				j = w * (h / nFrames);
	CRGBAColor*	p = reinterpret_cast<CRGBAColor*> (m_bmP->Buffer ()) + n * j;

#if USE_OPENMP
		if (gameStates.app.bMultiThreaded) {
			int			nst [MAX_THREADS], nac [MAX_THREADS], tId;
			CFloatVector	avc [MAX_THREADS];

			memset (avc, 0, sizeof (avc));
			memset (nst, 0, sizeof (nst));
			memset (nac, 0, sizeof (nac));
#pragma omp parallel private (tId)
			{
			tId = omp_get_thread_num ();
#	pragma omp for reduction (+: nVisible)
			for (int i = 0; i < j; i++) {
				if (bSwapRB)
					::Swap (p [i].Red (), p [i].Blue ());
				if (bGrayScale) {
					p [i].Red () =
					p [i].Green () =
					p [i].Blue () = ubyte ((int (p [i].Red ()) + int (p [i].Green ()) + int (p [i].Blue ())) / 3 * brightness);
					}
				else if ((p [i].Red () == 120) && (p [i].Green () == 88) && (p [i].Blue () == 128)) {
					nst [tId]++;
					p [i].Alpha () = 0;
					}
				else {
					if (alpha >= 0)
						p [i].Alpha () = alpha;
					if (!p [i].Alpha ())
						p [i].Red () =		//avoid colored transparent areas interfering with visible image edges
						p [i].Green () =
						p [i].Blue () = 0;
					}
				if (p [i].Alpha () < MIN_OPACITY) {
					avc [tId].Alpha () += p [i].Alpha ();
					nac [tId]++;
					}
				a = float (p [i].Alpha ()) / 255.0f;
				nVisible += a;
				avc [tId].Red () += float (p [i].Red ()) * a;
				avc [tId].Green () += float (p [i].Green ()) * a;
				avc [tId].Blue () += float (p [i].Blue ()) * a;
				}
			}
		for (int i = 0, j = gameStates.app.nThreads; i < j; i++) {
			avgColor.Red () += avc [i].Red ();
			avgColor.Green () += avc [i].Green ();
			avgColor.Blue () += avc [i].Blue ();
			avgColor.Alpha () += avc [i].Alpha ();
			nSuperTransp += nst [i];
			nAlpha += nac [i];
			}
		}
	else
#endif
		{
		for (i = j; i; i--, p++) {
			if (bSwapRB)
				::Swap (p->Red (), p->Blue ());
			if (bGrayScale) {
				p->Red () =
				p->Green () =
				p->Blue () = ubyte ((int (p->Red ()) + int (p->Green ()) + int (p->Blue ())) / 3 * brightness);
				}
			else if ((p->Red () == 120) && (p->Green () == 88) && (p->Blue () == 128)) {
				nSuperTransp++;
				p->Alpha () = 0;
				}
			else {
				if (alpha >= 0)
					p->Alpha () = alpha;
				if (!p->Alpha ())
					p->Red () =		//avoid colored transparent areas interfering with visible image edges
					p->Green () =
					p->Blue () = 0;
				}
			if (p->Alpha () < MIN_OPACITY) {
				avgColor.Alpha () += p->Alpha ();
				nAlpha++;
				}
			a = float (p->Alpha ()) / 255.0f;
			nVisible += a;
			avgColor.Red () += float (p->Red ()) * a;
			avgColor.Green () += float (p->Green ()) * a;
			avgColor.Blue () += float (p->Blue ()) * a;
			}
		}
		if (nAlpha > w * w / 1000) {
			if (!n) {
				if (avgColor.Alpha () / nAlpha > 5.0f)
					m_bmP->AddFlags (BM_FLAG_TRANSPARENT);
				else
					m_bmP->AddFlags (BM_FLAG_SEE_THRU | BM_FLAG_TRANSPARENT);
				}
			m_bmP->TransparentFrames () [n / 32] |= (1 << (n % 32));
			}
		if (nSuperTransp > w * w / 2000) {
			if (!n)
				m_bmP->AddFlags (BM_FLAG_SUPER_TRANSPARENT);
			//m_bmP->AddFlags (BM_FLAG_SEE_THRU);
			m_bmP->SuperTranspFrames () [n / 32] |= (1 << (n % 32));
			}
		}
	}
avgColorb.Red () = ubyte (avgColor.Red () / nVisible);
avgColorb.Green () = ubyte (avgColor.Green () / nVisible);
avgColorb.Blue () = ubyte (avgColor.Blue () / nVisible);
m_bmP->SetAvgColor (avgColorb);
if (!nAlpha)
	ConvertToRGB ();
}

//------------------------------------------------------------------------------

int CTGA::ReadData (CFile& cf, int alpha, double brightness, int bGrayScale, int bReverse)
{
	int	nBytes = int (m_header.Bits ()) / 8;

m_bmP->AddFlags (BM_FLAG_TGA);
m_bmP->SetBPP (nBytes);
if (!(m_bmP->Buffer () || m_bmP->CreateBuffer ()))
	 return 0;
m_bmP->SetTranspType (-1);
memset (m_bmP->TransparentFrames (), 0, 4 * sizeof (int));
memset (m_bmP->SuperTranspFrames (), 0, 4 * sizeof (int));
#if 1
if (bReverse)
	m_bmP->Read (cf);
else {
	int		h = m_bmP->Height ();
	int		w = m_bmP->RowSize ();
	ubyte*	bufP = m_bmP->Buffer () + w * h;
	while (h--) {
		bufP -= w;
		cf.Read (bufP, 1, w);
		}
	}
SetProperties (alpha, bGrayScale, brightness, bReverse == 0);
#else
	int				i, j, n, nAlpha = 0, nVisible = 0, nFrames;
	int				h = m_bmP->Height ();
	int				w = m_bmP->Width ();
	CFloatVector3		avgColor;
	CRGBColor		avgColorb;
	float				vec, avgAlpha = 0;

avgColor.Red () = avgColor.Green () = avgColor.Blue () = 0;
m_bmP->DelFlags (BM_FLAG_SEE_THRU | BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT);
if (m_header.Bits () == 24) {
	tBGRA	coord;
	CRGBColor *p = reinterpret_cast<CRGBColor*> (m_bmP->Buffer ()) + w * (h - 1);

	for (i = h; i; i--) {
		for (j = w; j; j--, p++) {
			if (cf.Read (&coord, 1, 3) != (size_t) 3)
				return 0;
			if (bGrayScale) {
				p->Red () =
				p->Green () =
				p->Blue () = (ubyte) (((int) coord.r + (int) coord.g + (int) coord.b) / 3 * brightness);
				}
			else {
				p->Red () = (ubyte) (coord.r * brightness);
				p->Green () = (ubyte) (coord.g * brightness);
				p->Blue () = (ubyte) (coord.b * brightness);
				}
			avgColor.Red () += p->Red ();
			avgColor.Green () += p->Green ();
			avgColor.Blue () += p->Blue ();
			//p->Alpha () = (alpha < 0) ? 255 : alpha;
			}
		p -= 2 * w;
		nVisible = w * h * 255;
		}
	}
else {
	m_bmP->AddFlags (BM_FLAG_SEE_THRU | BM_FLAG_TRANSPARENT);
	if (bReverse) {
		tRGBA	coord;
		CRGBAColor	*p = reinterpret_cast<CRGBAColor*> (m_bmP->Buffer ());
		int nSuperTransp;

		nFrames = h / w - 1;
		for (i = 0; i < h; i++) {
			n = nFrames - i / w;
			nSuperTransp = 0;
			for (j = w; j; j--, p++) {
				if (cf.Read (&coord, 1, 4) != (size_t) 4)
					return 0;
				if (bGrayScale) {
					p->Red () =
					p->Green () =
					p->Blue () = (ubyte) (((int) coord.r + (int) coord.g + (int) coord.b) / 3 * brightness);
					}
				else if (coord.vec) {
					p->Red () = (ubyte) (coord.r * brightness);
					p->Green () = (ubyte) (coord.g * brightness);
					p->Blue () = (ubyte) (coord.b * brightness);
					}
				if ((coord.r == 120) && (coord.g == 88) && (coord.b == 128)) {
					nSuperTransp++;
					p->Alpha () = 0;
					}
				else {
					p->Alpha () = (alpha < 0) ? coord.vec : alpha;
					if (!p->Alpha ())
						p->Red () =		//avoid colored transparent areas interfering with visible image edges
						p->Green () =
						p->Blue () = 0;
					}
				if (p->Alpha () < MIN_OPACITY) {
					if (!n) {
						m_bmP->AddFlags (BM_FLAG_TRANSPARENT);
						if (p->Alpha ())
							m_bmP->DelFlags (BM_FLAG_SEE_THRU);
						}
					if (bmP)
						m_bmP->TransparentFrames () [n / 32] |= (1 << (n % 32));
					avgAlpha += p->Alpha ();
					nAlpha++;
					}
				nVisible += p->Alpha ();
				vec = (float) p->Alpha () / 255;
				avgColor.Red () += p->Red () * vec;
				avgColor.Green () += p->Green () * vec;
				avgColor.Blue () += p->Blue () * vec;
				}
			if (nSuperTransp > 50) {
				if (!n)
					m_bmP->AddFlags (BM_FLAG_SUPER_TRANSPARENT);
				m_bmP->SuperTranspFrames () [n / 32] |= (1 << (n % 32));
				}
			}
		}
	else {
		tBGRA	coord;
		CRGBAColor *p = reinterpret_cast<CRGBAColor*> (m_bmP->Buffer ()) + w * (h - 1);
		int nSuperTransp;

		nFrames = h / w - 1;
		for (i = 0; i < h; i++) {
			n = nFrames - i / w;
			nSuperTransp = 0;
			for (j = w; j; j--, p++) {
				if (cf.Read (&coord, 1, 4) != (size_t) 4)
					return 0;
				if (bGrayScale) {
					p->Red () =
					p->Green () =
					p->Blue () = (ubyte) (((int) coord.r + (int) coord.g + (int) coord.b) / 3 * brightness);
					}
				else {
					p->Red () = (ubyte) (coord.r * brightness);
					p->Green () = (ubyte) (coord.g * brightness);
					p->Blue () = (ubyte) (coord.b * brightness);
					}
				if ((coord.r == 120) && (coord.g == 88) && (coord.b == 128)) {
					nSuperTransp++;
					p->Alpha () = 0;
					}
				else {
					p->Alpha () = (alpha < 0) ? coord.vec : alpha;
					if (!p->Alpha ())
						p->Red () =		//avoid colored transparent areas interfering with visible image edges
						p->Green () =
						p->Blue () = 0;
					}
				if (p->Alpha () < MIN_OPACITY) {
					if (!n) {
						m_bmP->AddFlags (BM_FLAG_TRANSPARENT);
						if (p->Alpha ())
							m_bmP->DelFlags (BM_FLAG_SEE_THRU);
						}
					m_bmP->TransparentFrames () [n / 32] |= (1 << (n % 32));
					avgAlpha += p->Alpha ();
					nAlpha++;
					}
				nVisible += p->Alpha ();
				vec = (float) p->Alpha () / 255;
				avgColor.Red () += p->Red () * vec;
				avgColor.Green () += p->Green () * vec;
				avgColor.Blue () += p->Blue () * vec;
				}
			if (nSuperTransp > w * w / 2000) {
				if (!n)
					m_bmP->AddFlags (BM_FLAG_SUPER_TRANSPARENT);
				m_bmP->SuperTranspFrames () [n / 32] |= (1 << (n % 32));
				}
			p -= 2 * w;
			}
		}
	}
vec = (float) nVisible / 255.0f;
avgColorb.Red () = (ubyte) (avgColor.Red () / vec);
avgColorb.Green () = (ubyte) (avgColor.Green () / vec);
avgColorb.Blue () = (ubyte) (avgColor.Blue () / vec);
m_bmP->SetAvgColor (avgColorb);
if (!nAlpha)
	ConvertToRGB (bmP);
#endif
return 1;
}

//	-----------------------------------------------------------------------------

int CTGA::WriteData (void)
{
	int				i, j;
	int				h = m_bmP->Height ();
	int				w = m_bmP->Width ();

if (m_header.Bits () == 24) {
	if (m_bmP->BPP () == 3) {
		tBGR	c;
		CRGBColor *p = reinterpret_cast<CRGBColor*> (m_bmP->Buffer ()) + w * (m_bmP->Height () - 1);
		for (i = m_bmP->Height (); i; i--) {
			for (j = w; j; j--, p++) {
				c.r = p->Red ();
				c.g = p->Green ();
				c.b = p->Blue ();
				if (m_cf.Write (&c, 1, 3) != (size_t) 3)
					return 0;
				}
			p -= 2 * w;
			}
		}
	else {
		tBGR	c;
		CRGBAColor *p = reinterpret_cast<CRGBAColor*> (m_bmP->Buffer ()) + w * (m_bmP->Height () - 1);
		for (i = m_bmP->Height (); i; i--) {
			for (j = w; j; j--, p++) {
				c.r = p->Red ();
				c.g = p->Green ();
				c.b = p->Blue ();
				if (m_cf.Write (&c, 1, 3) != (size_t) 3)
					return 0;
				}
			p -= 2 * w;
			}
		}
	}
else {
	tBGRA	c;
	CRGBAColor *p = reinterpret_cast<CRGBAColor*> (m_bmP->Buffer ()) + w * (m_bmP->Height () - 1);
	int bShaderMerge = gameOpts->ogl.bGlTexMerge;
	for (i = 0; i < h; i++) {
		for (j = w; j; j--, p++) {
			if (bShaderMerge && !(p->Red () || p->Green () || p->Blue ()) && (p->Alpha () == 1)) {
				c.r = 120;
				c.g = 88;
				c.b = 128;
				c.a = 1;
				}
			else {
				c.r = p->Red ();
				c.g = p->Green ();
				c.b = p->Blue ();
				c.a = ((p->Red () == 120) && (p->Green () == 88) && (p->Blue () == 128)) ? 255 : p->Alpha ();
				}
			if (m_cf.Write (&c, 1, 4) != (size_t) 4)
				return 0;
			}
		p -= 2 * w;
		}
	}
return 1;
}

//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------

int CTGA::Load (int alpha, double brightness, int bGrayScale)
{
return m_header.Read (m_cf, m_bmP) && ReadData (m_cf, alpha, brightness, bGrayScale, m_header.m_data.yStart != 0);
}

//---------------------------------------------------------------

int CTGA::Write (void)
{
return m_header.Write (m_cf, m_bmP) && WriteData ();
}

//---------------------------------------------------------------

#if USE_SDL_IMAGE

int CTGA::ReadImage (const char* pszFile, const char* pszFolder, int alpha, double brightness, int bGrayScale)
{
	char	szFolder [FILENAME_LEN], szFile [FILENAME_LEN], szExt [FILENAME_LEN], szImage [FILENAME_LEN];
	CFile	cf;

m_cf.SplitPath (pszFile, szFolder, szFile, szExt);
if (!pszFolder)
	pszFolder = gameFolders.game.szData [0];
if (!*szFolder)
	strcpy (szFolder, pszFolder);
int l = int (strlen (szFolder));
if (l && ((szFolder [l - 1] == '/') || (szFolder [l - 1] == '\\')))
	szFolder [l - 1] = '\0';
#	if 1
sprintf (szImage, "%s/%s.png", szFolder, szFile);
if (!m_cf.Exist (szImage, NULL, 0)) {
	return 0;
#	else
sprintf (szImage, "%s/%s%s", szFolder, szFile, *szExt ? szExt : ".png");
if (!m_cf.Exist (szImage, NULL, 0)) {
	if (*szExt)
		return 0;
	sprintf (szImage, "%s/%s%s", szFolder, szFile, *szExt ? szExt : ".tga");
	if (!m_cf.Exist (szImage, NULL, 0))
		return 0;
#	endif
	}

SDL_Surface*	imageP;

if (!(imageP = IMG_Load (szImage)))
	return 0;
m_bmP->SetWidth (imageP->w);
m_bmP->SetHeight (imageP->h);
m_bmP->SetBPP (imageP->format->BytesPerPixel);
if (!m_bmP->CreateBuffer ())
	return 0;
memcpy (m_bmP->Buffer (), imageP->pixels, m_bmP->Size ());
SDL_FreeSurface (imageP);
SetProperties (alpha, bGrayScale, brightness);
return 1;
}

#endif

//---------------------------------------------------------------

int CTGA::Read (const char *pszFile, const char *pszFolder, int alpha, double brightness, int bGrayScale, bool bAutoComplete)
{
	int	r;

#if USE_SDL_IMAGE

if (ReadImage (pszFile, pszFolder, alpha, brightness, bGrayScale))
	r = 1;
else

#endif //USE_SDL_IMAGE
{
	char	szFile [FILENAME_LEN], *psz;

if (!pszFolder) {
	m_cf.SplitPath (pszFile, szFile, NULL, NULL);
	if (!*szFile)
		pszFolder = gameFolders.game.szData [0];
	}

#if TEXTURE_COMPRESSION
if (m_bmP->ReadS3TC (pszFolder, m_szFilename))
	return 1;
#endif
if (bAutoComplete && !(psz = const_cast<char*> (strstr (pszFile, ".tga")))) {
	strcpy (szFile, pszFile);
	if ((psz = strchr (szFile, '.')))
		*psz = '\0';
	strcat (szFile, ".tga");
	pszFile = szFile;
	}
m_cf.Open (pszFile, pszFolder, "rb", 0);
r = (m_cf.File() != NULL) && Load (alpha, brightness, bGrayScale);
#if TEXTURE_COMPRESSION
if (r && CompressTGA (bmP))
	m_bmP->SaveS3TC (pszFolder, pszFile);
#endif
m_cf.Close ();
}

return r;
}

//	-----------------------------------------------------------------------------

CBitmap* CTGA::CreateAndRead (char* pszFile)
{
if (!(m_bmP = CBitmap::Create (0, 0, 0, 4)))
	return NULL;

char szName [FILENAME_LEN], szExt [FILENAME_LEN];
CFile::SplitPath (pszFile, NULL, szName, szExt);
sprintf (m_bmP->Name (), "%s%s", szName, szExt);

if (Read (pszFile, NULL, -1, 1.0, 0)) {
	m_bmP->SetType (BM_TYPE_ALT);
	return m_bmP;
	}
m_bmP->SetType (BM_TYPE_ALT);
delete m_bmP;
return m_bmP = NULL;
}

//	-----------------------------------------------------------------------------

int CTGA::Save (const char *pszFile, const char *pszFolder)
{
	char	szFolder [FILENAME_LEN], fn [FILENAME_LEN];
	int	r;

if (!pszFolder)
	pszFolder = gameFolders.game.szData [0];
CFile::SplitPath (pszFile, NULL, fn, NULL);
sprintf (szFolder, "%s%d/", pszFolder, m_bmP->Width ());
strcat (fn, ".tga");
r = m_cf.Open (fn, szFolder, "wb", 0) && Write ();
if (m_cf.File ())
	m_cf.Close ();
return r;
}

//	-----------------------------------------------------------------------------

int CTGA::Shrink (int xFactor, int yFactor, int bRealloc)
{
	int		bpp = m_bmP->BPP ();
	int		xSrc, ySrc, xMax, yMax, xDest, yDest, x, y, w, h, i, nFactor2, nSuperTransp, bSuperTransp;
	ubyte		*dataP, *pSrc, *pDest;
	int		cSum [4];

	static ubyte superTranspKeys [3] = {120,88,128};

if (!m_bmP->Buffer ())
	return 0;
if ((xFactor < 1) || (yFactor < 1))
	return 0;
if ((xFactor == 1) && (yFactor == 1))
	return 0;
w = m_bmP->Width ();
h = m_bmP->Height ();
xMax = w / xFactor;
yMax = h / yFactor;
nFactor2 = xFactor * yFactor;
if (!bRealloc)
	pDest = dataP = m_bmP->Buffer ();
else {
	if (!(dataP = new ubyte [xMax * yMax * bpp]))
		return 0;
	UseBitmapCache (m_bmP, (int) -m_bmP->Height () * int (m_bmP->RowSize ()));
	pDest = dataP;
	}
if (bpp == 3) {
	for (yDest = 0; yDest < yMax; yDest++) {
		for (xDest = 0; xDest < xMax; xDest++) {
			memset (&cSum, 0, sizeof (cSum));
			ySrc = yDest * yFactor;
			for (y = yFactor; y; ySrc++, y--) {
				xSrc = xDest * xFactor;
				pSrc = m_bmP->Buffer () + (ySrc * w + xSrc) * bpp;
				for (x = xFactor; x; xSrc++, x--) {
					for (i = 0; i < bpp; i++)
						cSum [i] += *pSrc++;
					}
				}
			for (i = 0; i < bpp; i++)
				*pDest++ = (ubyte) (cSum [i] / (nFactor2));
			}
		}
	}
else {
	for (yDest = 0; yDest < yMax; yDest++) {
		for (xDest = 0; xDest < xMax; xDest++) {
			memset (&cSum, 0, sizeof (cSum));
			ySrc = yDest * yFactor;
			nSuperTransp = 0;
			for (y = yFactor; y; ySrc++, y--) {
				xSrc = xDest * xFactor;
				pSrc = m_bmP->Buffer () + (ySrc * w + xSrc) * bpp;
				for (x = xFactor; x; xSrc++, x--) {
						bSuperTransp = (pSrc [0] == 120) && (pSrc [1] == 88) && (pSrc [2] == 128);
					if (bSuperTransp) {
						nSuperTransp++;
						pSrc += bpp;
						}
					else
						for (i = 0; i < bpp; i++)
							cSum [i] += *pSrc++;
					}
				}
			if (nSuperTransp >= nFactor2 / 2) {
				pDest [0] = 120;
				pDest [1] = 88;
				pDest [2] = 128;
				pDest [3] = 0;
				pDest += bpp;
				}
			else {
				for (i = 0, bSuperTransp = 1; i < bpp; i++)
					pDest [i] = (ubyte) (cSum [i] / (nFactor2 - nSuperTransp));
				if (!(m_bmP->Flags () & BM_FLAG_SUPER_TRANSPARENT)) {
					for (i = 0; i < 3; i++)
						if (pDest [i] != superTranspKeys [i])
							break;
					if (i == 3)
						pDest [0] =
						pDest [1] =
						pDest [2] =
						pDest [3] = 0;
					}
				pDest += bpp;
				}
			}
		}
	}
if (bRealloc) {
	m_bmP->DestroyBuffer ();
	m_bmP->SetBuffer (dataP);
	}
m_bmP->SetWidth (xMax);
m_bmP->SetHeight (yMax);
m_bmP->SetRowSize (m_bmP->RowSize () / xFactor);
if (bRealloc)
	UseBitmapCache (m_bmP, int (m_bmP->Height ()) * int (m_bmP->RowSize ()));
return 1;
}

//	-----------------------------------------------------------------------------

double CTGA::Brightness (void)
{
if (!m_bmP)
	return 0;
else {
		int		bAlpha = m_bmP->BPP () == 4, i, j;
		ubyte		*dataP;
		double	pixelBright, totalBright, nPixels, alpha;

	if (!(dataP = m_bmP->Buffer ()))
		return 0;
	totalBright = 0;
	nPixels = 0;
	for (i = m_bmP->Width () * m_bmP->Height (); i; i--) {
		for (pixelBright = 0, j = 0; j < 3; j++)
			pixelBright += ((double) (*dataP++)) / 255.0;
		pixelBright /= 3;
		if (bAlpha) {
			alpha = ((double) (*dataP++)) / 255.0;
			pixelBright *= alpha;
			nPixels += alpha;
			}
		totalBright += pixelBright;
		}
	return totalBright / nPixels;
	}
}

//	-----------------------------------------------------------------------------

void CTGA::ChangeBrightness (double dScale, int bInverse, int nOffset, int bSkipAlpha)
{
if (m_bmP) {
		int		bpp = m_bmP->BPP (), h, i, j, c, bAlpha = (bpp == 4);
		ubyte		*dataP;

	if ((dataP = m_bmP->Buffer ())) {
	if (!bAlpha)
		bSkipAlpha = 1;
	else if (bSkipAlpha)
		bpp = 3;
		if (nOffset) {
			for (i = m_bmP->Width () * m_bmP->Height (); i; i--) {
				for (h = 0, j = 3; j; j--, dataP++) {
					c = (int) *dataP + nOffset;
					h += c;
					*dataP = (ubyte) ((c < 0) ? 0 : (c > 255) ? 255 : c);
					}
				if (bSkipAlpha)
					dataP++;
				else if (bAlpha) {
					if ((c = *dataP)) {
						c += nOffset;
						*dataP = (ubyte) ((c < 0) ? 0 : (c > 255) ? 255 : c);
						}
					dataP++;
					}
				}
			}
		else if (dScale && (dScale != 1.0)) {
			if (dScale < 0) {
				for (i = m_bmP->Width () * m_bmP->Height (); i; i--) {
					for (j = bpp; j; j--, dataP++)
						*dataP = (ubyte) (*dataP * dScale);
					if (bSkipAlpha)
						dataP++;
					}
				}
			else if (bInverse) {
				dScale = 1.0 / dScale;
				for (i = m_bmP->Width () * m_bmP->Height (); i; i--) {
					for (j = bpp; j; j--, dataP++)
						if ((c = 255 - *dataP))
							*dataP = 255 - (ubyte) (c * dScale);
					if (bSkipAlpha)
						dataP++;
					}
				}
			else {
				for (i = m_bmP->Width () * m_bmP->Height (); i; i--) {
					for (j = bpp; j; j--, dataP++)
						if ((c = 255 - *dataP)) {
							c = (int) (*dataP * dScale);
							*dataP = (ubyte) ((c > 255) ? 255 : c);
							}
					if (bSkipAlpha)
						dataP++;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

int CTGA::Interpolate (int nScale)
{
	ubyte	*bufP, *destP, *srcP1, *srcP2;
	int	nSize, nFrameSize, nStride, nFrames, i, j;

if (nScale < 1)
	nScale = 1;
else if (nScale > 3)
	nScale = 3;
nScale = 1 << nScale;
nFrames = m_bmP->Height () / m_bmP->Width ();
nFrameSize = m_bmP->Width () * m_bmP->Width () * m_bmP->BPP ();
nSize = nFrameSize * nFrames * nScale;
if (!(bufP = new ubyte [nSize]))
	return 0;
m_bmP->SetHeight (m_bmP->Height () * nScale);
memset (bufP, 0, nSize);
for (destP = bufP, srcP1 = m_bmP->Buffer (), i = 0; i < nFrames; i++) {
	memcpy (destP, srcP1, nFrameSize);
	destP += nFrameSize * nScale;
	srcP1 += nFrameSize;
	}
#if 1
while (nScale > 1) {
	nStride = nFrameSize * nScale;
	for (i = 0; i < nFrames; i++) {
		srcP1 = bufP + nStride * i;
		srcP2 = bufP + nStride * ((i + 1) % nFrames);
		destP = srcP1 + nStride / 2;
		for (j = nFrameSize; j; j--) {
			*destP++ = (ubyte) (((short) *srcP1++ + (short) *srcP2++) / 2);
			if (destP - bufP > nSize)
				destP = destP;
			}
		}
	nScale >>= 1;
	nFrames <<= 1;
	}
#endif
m_bmP->DestroyBuffer ();
m_bmP->SetBuffer (bufP);
return nFrames;
}

//------------------------------------------------------------------------------

int CTGA::MakeSquare (void)
{
	ubyte	*bufP, *destP, *srcP;
	int	nSize, nFrameSize, nRowSize, nFrames, i, j, w, q;

nFrames = m_bmP->Height () / m_bmP->Width ();
if (nFrames < 4)
	return 0;
for (q = nFrames; q * q > nFrames; q >>= 1)
	;
if (q * q != nFrames)
	return 0;
w = m_bmP->Width ();
nFrameSize = w * w * m_bmP->BPP ();
nSize = nFrameSize * nFrames;
if (!(bufP = new ubyte [nSize]))
	return 0;
srcP = m_bmP->Buffer ();
nRowSize = w * m_bmP->BPP ();
for (destP = bufP, i = 0; i < nFrames; i++) {
	for (j = 0; j < w; j++) {
		destP = bufP + (i / q) * q * nFrameSize + j * q * nRowSize + (i % q) * nRowSize;
		memcpy (destP, srcP, nRowSize);
		srcP += nRowSize;
		}
	}
m_bmP->DestroyBuffer ();
m_bmP->SetBuffer (bufP);
m_bmP->SetWidth (q * w);
m_bmP->SetHeight (q * w);
return q;
}

//------------------------------------------------------------------------------

CBitmap* CTGA::ReadModelTexture (const char *pszFile, int bCustom)
{
	char			fn [FILENAME_LEN], en [FILENAME_LEN], fnBase [FILENAME_LEN], szShrunkFolder [FILENAME_LEN];
	int			nShrinkFactor = 1 << (3 - gameStates.render.nModelQuality);
	time_t		tBase, tShrunk;

if (!pszFile)
	return NULL;
CFile::SplitPath (pszFile, NULL, fn, en);
if (nShrinkFactor > 1) {
	sprintf (fnBase, "%s.tga", fn);
	sprintf (szShrunkFolder, "%s%d", gameFolders.var.szModels [bCustom], 512 / nShrinkFactor);
	tBase = m_cf.Date (fnBase, bCustom ? gameFolders.mods.szModels [0] : gameFolders.game.szModels, 0);
	tShrunk = m_cf.Date (fnBase, szShrunkFolder, 0);
	if ((tShrunk > tBase) && Read (fnBase, szShrunkFolder, -1, 1.0, 0)) {
		UseBitmapCache (m_bmP, int (m_bmP->Height ()) * int (m_bmP->RowSize ()));
		return m_bmP;
		}
	}
if (!(Read (pszFile, bCustom ? gameFolders.mods.szModels [bCustom - 1] : gameFolders.game.szModels, -1, 1.0, 0, false) ||
	   Read (pszFile, bCustom ? gameFolders.mods.szModels [bCustom - 1] : gameFolders.game.szModels, -1, 1.0, 0, true)))
	return NULL;
UseBitmapCache (m_bmP, int (m_bmP->Height ()) * int (m_bmP->RowSize ()));
if (gameStates.app.bCacheTextures && (nShrinkFactor > 1) &&
	 (m_bmP->Width () == 512) && Shrink (nShrinkFactor, nShrinkFactor, 1)) {
	strcat (fn, ".tga");
	if (!m_cf.Open (fn, bCustom ? gameFolders.mods.szModels [bCustom - 1] : gameFolders.game.szModels, "rb", 0))
		return m_bmP;
	if (m_header.Read (m_cf, NULL))
		Save (fn, gameFolders.var.szModels [bCustom]);
	m_cf.Close ();
	}
return m_bmP;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CModelTextures::ReadBitmap (int i, int bCustom)
{
	CBitmap*	bmP = m_bitmaps + i;
	CTGA		tga (bmP);

bmP->Destroy ();
if (!(bmP = tga.ReadModelTexture (m_names [i].Buffer (), bCustom)))
	return 0;
if (bmP->Buffer ()) {
	bmP = bmP->Override (-1);
	if (bmP->Frames ())
		bmP = bmP->CurFrame ();
	bmP->Bind (1);
	m_bitmaps [i].SetTeam (m_nTeam.Buffer () ? m_nTeam [i] : 0);
	}
return 1;
}

//------------------------------------------------------------------------------

int CModelTextures::Read (int bCustom)
{
for (int i = 0; i < m_nBitmaps; i++)
	if (!ReadBitmap (i, bCustom))
		return 0;
return 1;
}

//------------------------------------------------------------------------------

void CModelTextures::Release (void)
{
if ((m_bitmaps.Buffer ()))
	for (int i = 0; i < m_nBitmaps; i++)
		m_bitmaps [i].ReleaseTexture ();
}

//------------------------------------------------------------------------------

int CModelTextures::Bind (int bCustom)
{
if ((m_bitmaps.Buffer ()))
	for (int i = 0; i < m_nBitmaps; i++) {
		if (!(m_bitmaps [i].Buffer () || ReadBitmap (i, bCustom)))
			return 0;
		m_bitmaps [i].Bind (1);
		}
return 1;
}

//------------------------------------------------------------------------------

void CModelTextures::Destroy (void)
{
	int	i;

if (m_names.Buffer ()) {
	for (i = 0; i < m_nBitmaps; i++)
		m_names [i].Destroy ();
if (m_bitmaps.Buffer ())
	for (i = 0; i < m_nBitmaps; i++) {
		UseBitmapCache (&m_bitmaps [i], -static_cast<int> (m_bitmaps [i].Size ()));
		m_bitmaps [i].Destroy ();
		}
	m_nTeam.Destroy ();
	m_names.Destroy ();
	m_bitmaps.Destroy ();
	m_nBitmaps = 0;
	}
}

//------------------------------------------------------------------------------

bool CModelTextures::Create (int nBitmaps)
{
if (nBitmaps <= 0)
	return false;
if (!(m_bitmaps.Create (nBitmaps)))
	return false;
if (!(m_names.Create (nBitmaps)))
	return false;
if (!(m_nTeam.Create (nBitmaps)))
	return false;
m_nBitmaps = nBitmaps;
//m_bitmaps.Clear ();
m_names.Clear ();
m_nTeam.Clear ();
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

