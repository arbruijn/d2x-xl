#ifndef _ADDON_BITMAPS_H
#define _ADDON_BITMAPS_H

#include "descent.h"

//------------------------------------------------------------------------------

class CAddonBitmap : public CTGA {
	private:
		int		m_bAvailable;
		char		m_szName [FILENAME_LEN];
		CBitmap*	m_bmP;

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

int LoadAddonBitmap (CBitmap **bmPP, const char *pszName, int *bHaveP);

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

#if 0
int explBlast.Load (void);
void FreeExplBlast (void);
int corona.Load () (void);
int LoadGlare (void);
int LoadHalo (void);
int LoadThruster (int nStyle = -1);
int shield.Load (void);
int LoadDeadzone (void);
void LoadDamageIcons (void);
void FreeDamageIcons (void);
int LoadDamageIcon (int i);
void FreeDamageIcon (int i);
int LoadScope (void);
void FreeScope (void);
void FreeDeadzone (void);
int LoadJoyMouse (void);
void FreeJoyMouse (void);

extern CBitmap* corona.Bitmap ();
extern CBitmap* bmpGlare;
extern CBitmap* bmpHalo;
extern CBitmap* thruster.Bitmap ();
extern CBitmap* shield.Bitmap ();
extern CBitmap* explBlast.Bitmap ();
extern CBitmap* bmpSparks;
extern CBitmap* deadzone.Bitmap ();
extern CBitmap* bmpScope;
extern CBitmap* bmpJoyMouse;
extern CBitmap* bmpDamageIcon [3];
extern int bHaveDamageIcon [3];
extern const char* szDamageIcon [3];

extern int bHaveDeadzone;
extern int bHaveJoyMouse;
#endif

//------------------------------------------------------------------------------

#endif // _ADDON_BITMAPS_H
