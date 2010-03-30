#ifndef _ADDON_BITMAPS_H
#define _ADDON_BITMAPS_H

#include "descent.h"

//------------------------------------------------------------------------------

class CAddonBitmap {
	private:
		int		m_bAvailable;
		char		m_szName [PATHNAME_LEN];
		CBitmap*	m_bmP;

	public:
		CAddonBitmap (char *pszName = NULL);
		int Load (char *pszName = NULL);
		void Unload (void);
};

//------------------------------------------------------------------------------

int LoadAddonBitmap (CBitmap **bmPP, const char *pszName, int *bHaveP);

extern CAddonBitmap explBlast;
extern CAddonBitmap corona;
extern CAddonBitmap glare;
extern CAddonBitmap halo;
extern CAddonBitmap thruster;
extern CAddonBitmap shield;
extern CAddonBitmap deadzone;
extern CAddonBitmap damageIcons [3];
extern CAddonBitmap scope;
extern CAddonBitmap joyMouse;

#if 0
int LoadExplBlast (void);
void FreeExplBlast (void);
int LoadCorona (void);
int LoadGlare (void);
int LoadHalo (void);
int LoadThruster (int nStyle = -1);
int LoadShield (void);
int LoadDeadzone (void);
void LoadDamageIcons (void);
void FreeDamageIcons (void);
int LoadDamageIcon (int i);
void FreeDamageIcon (int i);
int LoadScope (void);
void FreeScope (void);
void LoadAddonImages (void);
void FreeExtraImages (void);
void FreeDeadzone (void);
int LoadJoyMouse (void);
void FreeJoyMouse (void);

extern CBitmap* bmpCorona;
extern CBitmap* bmpGlare;
extern CBitmap* bmpHalo;
extern CBitmap* bmpThruster;
extern CBitmap* bmpShield;
extern CBitmap* bmpExplBlast;
extern CBitmap* bmpSparks;
extern CBitmap* bmpDeadzone;
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
