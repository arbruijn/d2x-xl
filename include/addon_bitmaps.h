#ifndef _ADDON_BITMAPS_H
#define _ADDON_BITMAPS_H

#include "descent.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class CAddonBitmap : public CTGA {
	private:
		int		m_bAvailable;
		char		m_szName [FILENAME_LEN];

	public:
		CAddonBitmap (char *pszName = NULL);
		int Load (char *pszName = NULL);
		void Unload (void);
		inline CBitmap* Bitmap (void) { return m_bmP; }
		inline CTexture* Texture (void) { return m_bmP->Texture (); }
		inline int Bind (int bMipMaps) { return m_bmP->Bind (bMipMaps); }
		~CAddonBitmap () { Unload (); }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class CAnimation {
	private:
		uint						m_nFrames;
		CArray<CAddonBitmap>	m_frames;
		char						m_szName [FILENAME_LEN];

	public:
		CAnimation (const char* pszName = NULL, uint nFrames = 0);

		~CAnimation () { Destroy (); }

		void Destroy (void) {
			m_frames.Destroy ();
			m_nFrames = 0;
			}

		bool Load (const char* pszName = NULL);

		void Unload (void);

		CBitmap* Bitmap (fix xTTL, fix xLifeLeft);
	};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int LoadAddonBitmap (CBitmap **bmPP, const char *pszName, int *bHaveP, bool bBind = true);

void LoadAddonImages (void);
void UnloadAddonImages (void);

extern CAddonBitmap explBlast;
extern CAddonBitmap corona;
extern CAddonBitmap glare;
extern CAddonBitmap halo;
extern CAddonBitmap thruster;
extern CAddonBitmap shield;
extern CAddonBitmap deadzone;
extern CAddonBitmap damageIcon [3];
extern CAddonBitmap scope;
extern CAddonBitmap sparks;
extern CAddonBitmap joyMouse;
extern CAnimation shockwave;

//------------------------------------------------------------------------------

#endif // _ADDON_BITMAPS_H
