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

#ifndef _MENU_H
#define _MENU_H

#include "pstypes.h"
#include "timeout.h"
#include "font.h"
#include "error.h"
#include "strutil.h"
#include "menubackground.h"
// returns number of item chosen

//------------------------------------------------------------------------------

extern char *menu_difficulty_text[];
extern int32_t Max_debrisObjects;
extern int32_t MissileView_enabled;
extern int32_t EscortView_enabled;
extern int32_t Cockpit_rearView;
extern char *nmAllowedChars;
extern char bPauseableMenu;
extern char bAlreadyShowingInfo;

extern char NORMAL_RADIO_BOX[2];
extern char CHECKED_RADIO_BOX[2];
extern char NORMAL_CHECK_BOX[2];
extern char CHECKED_CHECK_BOX[2];
extern char CURSOR_STRING[2];
extern char SLIDER_LEFT[2];
extern char SLIDER_RIGHT[2];
extern char SLIDER_MIDDLE[2];
extern char SLIDER_MARKER[2];
extern char UP_ARROW_MARKER[2];
extern char DOWN_ARROW_MARKER[2];

//------------------------------------------------------------------------------

#define RETRO_STYLE			0 //ogl.m_states.nDrawBuffer != GL_BACK
#define FAST_MENUS			1 //gameOpts->menus.bFastMenus
#define MENU_CLOSE_X			 (gameStates.menus.bHires?15:7)
#define MENU_CLOSE_Y			 (gameStates.menus.bHires?15:7)
#define MENU_CLOSE_SIZE		 (gameStates.menus.bHires?10:5)
#define MENU_MAX_FILES		300
#define MSGBOX_TEXT_SIZE	10000		// How many characters in messagebox
#define MENU_FADE_TIME		150		// ms
//------------------------------------------------------------------------------

#define EXPMODE_DEFAULTS 0

#define NMCLAMP(_v,_min,_max)	 ((_v) < (_min) ? (_min) : (_v) > (_max) ? (_max) : (_v))
#define NMBOOL(_v) ((_v) != 0)

#define GET_VAL(_v,_id)	if (m.Available (_id)) (_v) = m.Value (_id)

#define MENU_KEY(_k,_d)	 ((_k) < 0) ? (_d) : ((_k) == 0) ? 0 : gameStates.app.bEnglish ? toupper (KeyToASCII (_k)) : (_k)

#define PROGRESS_INCR		20
#define PROGRESS_STEPS(_i)	 (( (_i) + PROGRESS_INCR - 1) / PROGRESS_INCR)

//------------------------------------------------------------------------------

#define INIT_PROGRESS_LOOP(_i,_j,_max) \
	if ((_i) < 0) { \
		 (_i) = 0; (_j) = (_max);} \
	else { \
		 (_j) = (_i) + PROGRESS_INCR; \
		if ((_j) > (_max)) \
			 (_j) = (_max); \
		}

#include "gr.h"
#include "cfile.h"

#define NM_TYPE_MENU        0   // A menu item... when enter is hit on this, ExecMenu returns this item number
#define NM_TYPE_INPUT       1   // An input box... fills the text field in, and you need to fill in nTextLen field.
#define NM_TYPE_CHECK       2   // A check box. Set and get its status by looking at flags field (1=on, 0=off)
#define NM_TYPE_RADIO       3   // Same as check box, but only 1 in a group can be set at a time. Set group fields.
#define NM_TYPE_TEXT        4   // A line of text that does nothing.
#define NM_TYPE_NUMBER      5   // A numeric entry counter.  Changes value from minValue to maxValue;
#define NM_TYPE_INPUT_MENU  6   // A inputbox that you hit Enter to edit, when done, hit enter and menu leaves.
#define NM_TYPE_SLIDER      7   // A slider from minValue to maxValue. Draws with nTextLen chars.
#define NM_TYPE_GAUGE       8   // A slider from minValue to maxValue. Draws with nTextLen chars.
#define MENU_MAX_TEXTLEN    1000

//------------------------------------------------------------------------------

typedef char tMenuText[MENU_MAX_TEXTLEN];

class CMenuItem {
public:
	int32_t		m_nType; // What kind of item this is, see NM_TYPE_????? defines
	int32_t		m_group; // What group this belongs to for radio buttons.
	int32_t		m_nTextLen; // The maximum length of characters that can be entered by this inputboxes
	uint32_t		m_color;
	int16_t		m_nKey;
	// The rest of these are used internally by by the menu system, so don't set 'em!!
	int16_t		m_x, m_y, m_xSave, m_ySave;
	int16_t		m_xSlider;
	int16_t		m_w, m_h;
	int16_t		m_rightOffset;
	uint8_t		m_bRedraw;
	uint8_t		m_bRebuild;
	uint8_t		m_bNoScroll;
	uint8_t		m_bUnavailable;
	uint8_t		m_bCentered;
	char			m_text [MENU_MAX_TEXTLEN + 1];
	char			m_savedText [MENU_MAX_TEXTLEN + 1];
	char			*m_pszText;
	CBitmap		*m_bmText [2];
	const char	*m_szHelp;
	char			*m_szId;

protected:
	int32_t		m_value; // For checkboxes and radio buttons, this is 1 if marked initially, else 0
	int32_t		m_minValue, m_maxValue; // For sliders and number bars.

public:
	CMenuItem () {
		memset (this, 0, sizeof (*this));
	}
	~CMenuItem () {
		Destroy ();
	}
	void Destroy (void);
	void FreeTextBms (void);
	void FreeId (void);
	int32_t GetSize (int32_t h, int32_t aw, int32_t& nStringWidth, int32_t& nStringHeight,
			int32_t& nAverageWidth, int32_t& nMenus, int32_t& nOthers, int32_t bTiny);
	int16_t SetColor (int32_t bIsCurrent, int32_t bTiny);

	void DrawHotKeyString (int32_t bIsCurrent, int32_t bTiny, int32_t bCreateTextBms, int32_t nDepth);
	bool DrawHotKeyStringBitmap (int32_t bIsCurrent, int32_t bTiny, int32_t bCreateTextBms, int32_t nDepth);
	void DrawString (int32_t bIsCurrent, int32_t bTiny);
	void DrawSlider (int32_t bIsCurrent, int32_t bTiny);
	void DrawRightString (int32_t bIsCurrent, int32_t bTiny, char* s);
	void DrawInputBox (int32_t w, int32_t x, int32_t y, char* text, int32_t current, int32_t bTiny);
	void DrawBlackBox (int32_t w1, int32_t x, int32_t y, const char* s, int32_t bTiny);
	void DrawGauge (int32_t w, int32_t x, int32_t y, int32_t val, int32_t maxVal, int32_t current);
	void Draw (int32_t bIsCurrent, int32_t bTiny);

	void ShowHelp (void);

	void UpdateCursor (void);
	void TrimWhitespace (void);
	void SetText (const char* pszSrc, char* pszDest = NULL);
	void SaveText (void);
	void RestoreText (void);
	char* GetInput (void);

	inline int32_t& Value (void) {
		return m_value;
	}

	inline void SetValue (int32_t value) {
		if (m_value == value)
			return;
		m_value = value;
		Rebuild ();
	}
	inline int32_t& MinValue (void) {
		return m_minValue;
	}
	inline int32_t& MaxValue (void) {
		return m_maxValue;
	}
	inline int32_t Range (void) {
		return MaxValue () - MinValue () + 1;
	}
	inline void Redraw (void) {
		m_bRedraw = 1;
	}
	inline void Rebuild (void) {
		m_bRebuild = 1;
	}
	inline int32_t Rebuilding (void) {
		return m_bRebuild;
	}
	inline char* Text (void) {
		return m_text;
	}
	inline int32_t ToInt (void) {
		return atol (m_text);
	}

	inline bool Selectable (void) {
		return (m_nType != NM_TYPE_TEXT) && !m_bUnavailable && *m_text;
	}

	void SetId (const char* pszId);
};

//------------------------------------------------------------------------------

typedef struct tMenuProps {
	int32_t			screenWidth, screenHeight, x, y, xOffs, yOffs, userWidth, userHeight, width, height, aw, titleWidth, titleHeight,
						ty, titlePos, rightOffset, nStringHeight, bTinyMode, nMenus, nOthers,
						nMaxNoScroll, nMaxOnMenu, nMaxDisplayable, nScrollOffset,
						bIsScrollBox, nDisplayMode, bValid;
	const char		* pszTitle, * pszSubTitle;
	const CCanvas	* gameCanvasP;
} tMenuProps;

//------------------------------------------------------------------------------

class CMenu;

typedef int32_t (*pMenuCallback) (CMenu& m, int32_t& lastKey, int32_t nItem, int32_t nState);

class CMenu : public CStack<CMenuItem> {
private:
	int32_t			m_nGroup;
	CTimeout			m_to;
	CMenu*			m_parent;
	static CMenu*	m_active;
	static int32_t	m_level;

protected:
	int32_t			m_bStart;
	int32_t			m_bDone;
	int32_t			m_nLastScrollCheck;
	int32_t			m_bRedraw;
	int32_t			m_bCloseBox;
	int32_t			m_bDontRestore;
	int32_t			m_bAllText;
	uint32_t			m_tEnter;
	int32_t			m_nChoice;
	int32_t			m_nOldChoice;
	int32_t			m_nTopChoice;
	int32_t			m_nMouseState;
	int32_t			m_nOldMouseState;
	int32_t			m_bWheelUp;
	int32_t			m_bWheelDown;
	int32_t			m_bDblClick;
	int32_t			m_nKey;
	int32_t			m_xMouse;
	int32_t			m_yMouse;
	int32_t			m_bPause;
	bool				m_bThrottle;
	pMenuCallback	m_callback;
	CMenuItem		m_null;
	CMenuItem*		m_current;
	tMenuProps		m_props;
	CBackground		m_background;

public:
	CMenu () {
		Init ();
	}

	explicit CMenu (uint32_t nLength) {
		Init ();
		Create (nLength);
		}

	inline void Register (void) {
		m_parent = m_active;
		m_active = this;
		++m_level;
		}

	inline void Unregister (void) {
		m_active = m_parent;
		--m_level;
		}

	inline void Init (void) {
		SetGrowth (10);
		m_nGroup = 0;
		m_to.Setup (10);
		m_bThrottle = true;
		m_null.SetId (" (null)");
		m_current = NULL;
	}

	void Destroy (void) {
		CStack<CMenuItem>::Destroy ();
		m_current = NULL;
	}

	inline int32_t NewGroup (int32_t nGroup = 0) {
		if (!nGroup)
			m_nGroup++;
		else if (nGroup > 0)
			m_nGroup = nGroup;
		else
			m_nGroup--;
		return m_nGroup;
	}

	inline int32_t IndexOf (const char* szId, bool bLogErrors = true) {
		if (szId && *szId) {
			if (*szId == '?')
				return IndexOf (szId + 1, false);
			if (m_current && m_current->m_szId
					&& !stricmp (szId, m_current->m_szId))
				return (int32_t) (m_current - Buffer ());
			m_current = Buffer ();
			for (uint32_t i = 0; i < m_tos; i++, m_current++) {
				if (m_current->m_szId && !stricmp (szId, m_current->m_szId))
					return i;
			}
		}
		m_current = NULL;
#if DBG
		if (bLogErrors)
			PrintLog (0, "invalid menu id '%s' queried\n", szId ? szId : "n/a");
#endif
		return -1;
		}

	inline bool Available (const char* szId) {
		return IndexOf (szId, false) >= 0;
		}

	inline int32_t Value (const char* szId) {
		int32_t i = IndexOf (szId);
		return (i < 0) ? 0 : Buffer (i)->Value ();
		}

	inline void SetValue (const char* szId, int32_t value) {
		int32_t i = IndexOf (szId);
		if (i >= 0)
			Buffer (i)->Value () = value;
		}

	inline int32_t MinValue (const char* szId) {
		int32_t i = IndexOf (szId);
		if (i >= 0)
			return Buffer (i)->MinValue ();
		return 0;
		}

	inline int32_t MaxValue (const char* szId) {
		int32_t i = IndexOf (szId);
		if (i >= 0)
			return Buffer (i)->MaxValue ();
		return 0;
		}

	inline char* Text (const char* szId) {
		int32_t i = IndexOf (szId);
		if (i >= 0)
			return Buffer (i)->Text ();
		return NULL;
		}

	inline int32_t ToInt (const char* szId) {
		int32_t i = IndexOf (szId);
		if (i >= 0)
			return Buffer (i)->ToInt ();
		return 0;
		}

	inline CMenuItem& operator[] (const int32_t i) {
		return CStack<CMenuItem>::operator[] (i);
		}

	inline CMenuItem* operator[] (const char* szId) {
		int32_t i = IndexOf (szId);
		return (i < 0) ? NULL : Buffer (i);
		}

	int32_t AddCheck (const char* szId, const char* szText, int32_t nValue, int32_t nKey = 0, const char* szHelp = NULL);
	int32_t AddRadio (const char* szId, const char* szText, int32_t nValue, int32_t nKey = 0, const char* szHelp = NULL);
	int32_t AddMenu (const char* szId, const char* szText, int32_t nKey = 0, const char* szHelp = NULL);
	int32_t AddText (const char* szId, const char* szText, int32_t nKey = 0);
	int32_t AddSlider (const char* szId, const char* szText, int32_t nValue, int32_t nMin, int32_t nMax, int32_t nKey = 0, const char* szHelp = NULL);
	int32_t AddInput (const char* szId, const char* szText, int32_t nLen, const char* szHelp = NULL);
	int32_t AddInput (const char* szId, const char* szText, char* szValue, int32_t nLen, const char* szHelp = NULL);
	int32_t AddInput (const char* szId, const char* szText, char* szValue, int32_t nValue, int32_t nLen, const char* szHelp = NULL);
	int32_t AddInputBox (const char* szId, const char* szText, int32_t nLen, int32_t nKey = 0, const char* szHelp = NULL);
	int32_t AddNumber (const char* szId, const char* szText, int32_t nValue, int32_t nMin, int32_t nMax);
	int32_t AddGauge (const char* szId, const char* szText, int32_t nValue, int32_t nMax);
	inline CMenuItem& Item (int32_t i = -1) {return (i < 0) ? m_data.buffer [ToS () - 1] : m_data.buffer [i];	}

	inline CBackground& Background (void) { return m_background; }
	void SetBoxColor (uint8_t red = PAL2RGBA (22), uint8_t green = PAL2RGBA (22), uint8_t blue = PAL2RGBA (38)) { Background ().SetBoxColor (red, green, blue); }

	int32_t XOffset (void);
	int32_t YOffset (void);

	void GetMousePos (void);

	int32_t Menu (const char* pszTitle, const char* pszSubTitle,
					  pMenuCallback callback = NULL, int32_t* nCurItemP = NULL,
					  int32_t nType = BG_SUBMENU, int32_t nWallpaper = BG_STANDARD, 
					  int32_t width = -1, int32_t height = -1,
					  int32_t bTinyMode = 0);

	int32_t TinyMenu (const char *pszTitle, const char *pszSubTitle, pMenuCallback callBack = NULL);

	int32_t FixedFontMenu (const char* pszTitle, const char* pszSubTitle,
								  pMenuCallback callback, int32_t* nCurItemP,
								  int32_t nType = BG_SUBMENU, int32_t nWallpaper = BG_STANDARD, 
								  int32_t width = -1, int32_t height = -1);

	void DrawCloseBox (int32_t x, int32_t y);
	bool FadeIn (void);
	void FadeOut (const char* pszTitle = NULL, const char* pszSubTitle = NULL, CCanvas* gameCanvasP = NULL);

	inline void SetThrottle (time_t t) {
		m_to.Setup (t);
	}

	void Render (const char* pszTitle, const char* pszSubTitle, CCanvas* gameCanvasP = NULL);
	virtual void Render (void);
	inline const char* Title (void) { return m_props.pszTitle; }
	inline const char* SubTitle (void) { return m_props.pszSubTitle; }

	static float GetScale (void);
	static inline int32_t Scaled (int32_t v) { return (int32_t) FRound ((float (v) * GetScale ())); }
	static inline int32_t Unscaled (int32_t v) { return (int32_t) FRound ((float (v) / GetScale ())); }

	static CMenu * Active (void) { return m_active; }

private:
	int32_t InitProps (const char* pszTitle, const char* pszSubTitle);
	void GetTitleSize (const char* pszTitle, CFont *font, int32_t& tw, int32_t& th);
	int32_t GetSize (int32_t& w, int32_t& h, int32_t& aw, int32_t& nMenus, int32_t& nOthers);
	void SetItemPos (int32_t twidth, int32_t xOffs, int32_t yOffs, int32_t right_offset);

	void DrawRightStringWXY (int32_t w1, int32_t x, int32_t y, const char* s);
	int32_t CharAllowed (char c);
	void TrimWhitespace (char* text);
	int32_t DrawTitle (const char* pszTitle, CFont *font, uint32_t color, int32_t ty);

	void SaveScreen (CCanvas **gameCanvasP);
	void RestoreScreen (void);
	void FreeTextBms (void);
	void SwapText (int32_t i, int32_t j);

	int32_t Setup (int32_t nType, int32_t width, int32_t height, int32_t bTinyMode, pMenuCallback callback, int32_t& nItem);
	void Update (const char* pszTitle, const char* pszSubTitle, int32_t nType, int32_t nWallpaper, int32_t width, int32_t height, int32_t bTinyMode, int32_t* nCurItemP);
	int32_t HandleKey (int32_t width, int32_t height, int32_t bTinyMode, int32_t* nCurItemP);
	int32_t HandleMouse (int32_t* nCurItemP);
	int32_t FindClickedOption (int32_t iMin, int32_t iMax);

	void KeyScrollUp (void);
	void KeyScrollDown (void);
	void KeyCheckOption (void);
	int32_t KeyScrollValue (void);
	int32_t QuickSelect (int32_t* nCurItemP);
	int32_t EditValue (void);
	int32_t MouseScrollUp (void);
	int32_t MouseScrollDown (void);
	int32_t MouseCheckOption (void);
	int32_t MouseScrollValue (void);
	void LaunchOption (int32_t* nCurItemP);

	CMenuItem* AddItem (void);
	void SetId (CMenuItem& item, const char* pszId);

};

//------------------------------------------------------------------------------

class CFileSelector: public CMenu {
private:
	int32_t m_nFirstItem;
	int32_t m_nVisibleItems;
	int32_t m_nMode;
	int32_t m_bDemosDeleted;
	int32_t m_nTextLeft;
	int32_t m_nTextTop;
	int32_t m_nTextWidth;
	int32_t m_nTextHeight;
	int32_t m_nFileCount;
	CArray<CFilename> m_filenames;

public:
	int32_t FileSelector (const char *pszTitle, const char *filespec, char *filename, int32_t bAllowAbort);
	int32_t DeleteFile (void);
	virtual void Render (void);
};

//------------------------------------------------------------------------------

typedef int32_t
		 (*pListBoxCallback) (int32_t* nItem, CArray<char*>& items, int32_t* keypress);

class CListBox: public CMenu {
	int32_t m_nFirstItem;
	int32_t m_nVisibleItems;
	int32_t m_nWidth;
	int32_t m_nHeight;
	int32_t m_nTitleHeight;
	int32_t m_nOffset;
	CStack<char*>* m_items;

public:
	int32_t ListBox (const char* pszTitle, CStack<char*>& items, int32_t nDefaultItem = 0, int32_t bAllowAbort = 1, pListBoxCallback callback = NULL);
	virtual void Render (void);
};

//------------------------------------------------------------------------------

class CMessageBox: public CMenu {
	int32_t m_nDrawBuffer;
	const char* m_pszMsg;
	CBitmap		m_bm;
	int32_t		m_x;
	int32_t		m_y;
	bool			m_bCentered;

public:
	~CMessageBox () {
		Clear ();
	}
	void Show (const char *pszMsg, const char *pszImage = NULL, bool bFade = true, bool bCentered = false);
	void Clear (void);
	virtual void Render (void);
};

extern CMessageBox messageBox;

//------------------------------------------------------------------------------

// This function pops up a messagebox and returns which choice was selected...
// Example:
int32_t _CDECL_ TextBox (const char *pszTitle, int32_t nWallpaper, int32_t nChoices, ...);

// Same as above, but you can pass a function
int32_t _CDECL_ InfoBox (const char *pszTitle, pMenuCallback callBack, int32_t nWallpaper, int32_t nChoices, ...);

void ProgressBar (const char *szCaption, int32_t nBars, int32_t nCurProgress, int32_t nMaxProgress, pMenuCallback doProgress);

int32_t FileList (const char *pszTitle, const char *filespec, char *filename);

int32_t stoip (char *szServerIpAddr, uint8_t *ipAddrP, int32_t* portP = NULL);
int32_t stoport (char *szPort, int32_t *pPort, int32_t *pSign);
int32_t SetCustomDisplayMode (int32_t x, int32_t y);

void AutomapOptionsMenu (void);
void CameraOptionsMenu (void);
void CockpitOptionsMenu (void);
void ConfigMenu (void);
void CoronaOptionsMenu (void);
void DetailLevelMenu (void);
void CustomDetailsMenu (void);
void SetMaxCustomDetails (void);
void EffectOptionsMenu (void);
void GameplayOptionsMenu (void);
void LightOptionsMenu (void);
void LightningOptionsMenu (void);
void MainOptionsMenu (void);
void MiscellaneousMenu (void);
void MovieOptionsMenu (void);
int32_t MultiplayerMenu (void);
int32_t NewGameMenu (void);
void LegacyNewGameMenu (void);
void PhysicsOptionsMenu (void);
void PowerupOptionsMenu (void);
void PerformanceSettingsMenu (void);
void PerformanceSettingsMenu (void);
void RenderOptionsMenu (void);
void AdvancedRenderOptionsMenu (void);
void ScreenResMenu (void);
void ShadowOptionsMenu (void);
void ShipRenderOptionsMenu (void);
void SmokeOptionsMenu (void);
void SoundMenu (void);
void LoadoutOptionsMenu (void);

int32_t MainMenu (void);
int32_t QuitSaveLoadMenu (void);
int32_t SelectAndLoadMission (int32_t bMulti, int32_t *bAnarchyOnly);

int32_t InitDetailLevels (int32_t detailLevel);

int32_t SwitchDisplayMode (int32_t dir);

void IpxSetDriver (int32_t ipx_driver);
void DoNewIPAddress (void);

bool MenuRenderTimeout (int32_t& t0, int32_t tFade);
bool BeginRenderMenu (void);
void RenderMenuGameFrame (void);

void AddShipSelection (CMenu& m, int32_t& optShip);
void GetShipSelection (CMenu& m, int32_t& optShip);

//------------------------------------------------------------------------------

#endif /* _MENU_H */
