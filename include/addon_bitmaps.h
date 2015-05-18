#ifndef _ADDON_BITMAPS_H
#define _ADDON_BITMAPS_H

#include "descent.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class CAddonBitmap : public CTGA {
	private:
		int32_t	m_bAvailable;
		int32_t	m_bCartoonize;
		int32_t	m_fps;
		char		m_szName [FILENAME_LEN];
		CTimeout	m_to;

		static CStack<CAddonBitmap*>	m_list;

	public:
		static void Register (CAddonBitmap* pBm);
		static void Unregister (CAddonBitmap* pBm);
		static void Prepare (void);

	public:
		CAddonBitmap (char *pszName = NULL, int32_t bCartoonize = 0);
		int32_t Load (char *pszName = NULL);
		void Unload (void);
		int32_t Bind (int32_t bMipMaps);

		inline CBitmap* Bitmap (void) { return m_pBm; }

		inline CTexture* Texture (void) { return m_pBm ? m_pBm->Texture () : NULL; }
		inline void SetCartoonizable (int32_t bCartoonize) { m_bCartoonize = bCartoonize; }

		void SetFPS (int32_t fps) {
			if ((fps != m_fps) && (m_fps = fps)) {
				m_to.Setup (1000 / m_fps);
				m_to.Start ();
				}
			}

		void Animate (int32_t fps = 0) {
			SetFPS (fps);
			if (m_fps && m_to.Expired () && m_pBm && m_pBm->CurFrame ()) 
				m_pBm->NextFrame ();
			}

		int32_t IsMe (CBitmap *pBm) { return m_pBm && (pBm == m_pBm); }

		~CAddonBitmap () { Unload (); }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class CAnimation {
	private:
		uint32_t					m_nFrames;
		CArray<CAddonBitmap>	m_frames;
		char						m_szName [FILENAME_LEN];
		bool						m_bLoaded;
		int32_t					m_bCartoonize;

	public:
		CAnimation (const char* pszName = NULL, uint32_t nFrames = 0, int32_t bCartoonize = 0);

		~CAnimation () { Destroy (); }

		void Destroy (void) {
			m_frames.Destroy ();
			m_nFrames = 0;
			m_bLoaded = false;
			}

		bool Load (const char* pszName = NULL);

		void Unload (void);

		CBitmap* Bitmap (fix xTTL, fix xLifeLeft);
	};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32_t LoadAddonBitmap (CBitmap **bmPP, const char *pszName, int32_t *bHaveP, bool bBind = true);

void LoadAddonImages (void);
void UnloadAddonImages (void);

extern CAddonBitmap explBlast;
extern CAddonBitmap corona;
extern CAddonBitmap glare;
extern CAddonBitmap halo;
extern CAddonBitmap thruster;
extern CAddonBitmap shield [3];
extern CAddonBitmap pyroIcon;
extern CAddonBitmap cloakIcon;
extern CAddonBitmap invulIcon;
//extern CAddonBitmap caustic;
extern CAddonBitmap deadzone;
extern CAddonBitmap damageIcon [3];
extern CAddonBitmap scope;
extern CAddonBitmap sparks;
extern CAddonBitmap joyMouse;
extern CAnimation shockwave;

//------------------------------------------------------------------------------

#endif // _ADDON_BITMAPS_H
