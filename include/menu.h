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
// returns number of item chosen

//------------------------------------------------------------------------------

extern char *menu_difficulty_text[];
extern int Max_debrisObjects;
extern int MissileView_enabled;
extern int EscortView_enabled;
extern int Cockpit_rearView;
extern char *nmAllowedChars;
extern char bPauseableMenu;
extern char bAlreadyShowingInfo;

extern char NORMAL_RADIO_BOX [2];
extern char CHECKED_RADIO_BOX [2];
extern char NORMAL_CHECK_BOX [2];
extern char CHECKED_CHECK_BOX [2];
extern char CURSOR_STRING [2];
extern char SLIDER_LEFT [2];
extern char SLIDER_RIGHT [2];
extern char SLIDER_MIDDLE [2];
extern char SLIDER_MARKER [2];
extern char UP_ARROW_MARKER [2];
extern char DOWN_ARROW_MARKER [2];

//------------------------------------------------------------------------------

#define RETRO_STYLE			0 //gameStates.ogl.nDrawBuffer != GL_BACK
#define MODERN_STYLE			1 //gameOpts->menus.nStyle
#define FAST_MENUS			1 //gameOpts->menus.bFastMenus

#define MENU_CLOSE_X			(gameStates.menus.bHires?15:7)
#define MENU_CLOSE_Y			(gameStates.menus.bHires?15:7)
#define MENU_CLOSE_SIZE		(gameStates.menus.bHires?10:5)
#define MENU_MAX_FILES		300
#define MSGBOX_TEXT_SIZE	10000		// How many characters in messagebox

#define MENU_FADE_TIME		150		// ms

//------------------------------------------------------------------------------

#define EXPMODE_DEFAULTS 0

#define NMCLAMP(_v,_min,_max)	((_v) < (_min) ? (_min) : (_v) > (_max) ? (_max) : (_v))
#define NMBOOL(_v) ((_v) != 0)

#define GET_VAL(_v,_n)	if ((_n) >= 0) (_v) = m [_n].m_value

#define MENU_KEY(_k,_d)	((_k) < 0) ? (_d) : ((_k) == 0) ? 0 : gameStates.app.bEnglish ? toupper (KeyToASCII (_k)) : (_k)

#define PROGRESS_INCR		20
#define PROGRESS_STEPS(_i)	(((_i) + PROGRESS_INCR - 1) / PROGRESS_INCR)

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

#define MENU_MAX_TEXTLEN    100

//------------------------------------------------------------------------------

typedef char tMenuText [MENU_MAX_TEXTLEN];

class CMenuItem {
	public:
		int			m_nType;           // What kind of item this is, see NM_TYPE_????? defines
		int			m_value;          // For checkboxes and radio buttons, this is 1 if marked initially, else 0
		int			m_minValue, m_maxValue;   // For sliders and number bars.
		int			m_group;          // What group this belongs to for radio buttons.
		int			m_nTextLen;       // The maximum length of characters that can be entered by this inputboxes
		uint			m_color;
		short			m_nKey;
		// The rest of these are used internally by by the menu system, so don't set 'em!!
		short			m_x, m_y, m_xSave, m_ySave;
		short			m_w, m_h;
		short			m_rightOffset;
		ubyte			m_bRedraw;
		ubyte			m_bRebuild;
		ubyte			m_bNoScroll;
		ubyte			m_bUnavailable;
		ubyte			m_bCentered;
		char			m_text [MENU_MAX_TEXTLEN + 1];
		char			m_savedText [MENU_MAX_TEXTLEN + 1];
		char*			m_pszText;
		CBitmap*		m_bmText [2];
		const char*	m_szHelp;

	public:
		CMenuItem () { memset (this, 0, sizeof (*this)); }
		~CMenuItem () { Destroy (); }
		void Destroy (void);
		int GetSize (int h, int aw, int& nStringWidth, int& nStringHeight, int& nAverageWidth, int& nMenus, int& nOthers);
		short SetColor (int bIsCurrent, int bTiny);

		void DrawHotKeyString (int bIsCurrent, int bTiny, int bCreateTextBms, int nDepth);
		void DrawString (int bIsCurrent, int bTiny);
		void DrawSlider (int bIsCurrent, int bTiny);
		void DrawRightString (int bIsCurrent, int bTiny, char* s);
		void DrawInputBox (int w, int x, int y, char* text, int current);
		void DrawBlackBox (int w1, int x, int y, const char* s);
		void DrawGauge (int w, int x, int y, int val, int maxVal, int current);
		void Draw (int bIsCurrent, int bTiny);

		void ShowHelp (void);

		void UpdateCursor (void);
		void TrimWhitespace (void);
		void SetText (const char* pszSrc, char* pszDest = NULL);
		void SaveText (void);
		void RestoreText (void);
		char* GetInput (void);

		inline bool Selectable (void) { return (m_nType != NM_TYPE_TEXT) && !m_bUnavailable && *m_text; }

	};

//------------------------------------------------------------------------------

typedef struct tMenuProps {
	int	scWidth,
			scHeight,
			x, 
			y, 
			xOffs, 
			yOffs,
			width,
			height,
			w, 
			h, 
			aw, 
			tw, 
			th, 
			ty,
			twidth, 
			rightOffset, 
			nStringHeight,
			bTinyMode,
			nMenus, 
			nOthers, 
			nMaxNoScroll, 
			nMaxOnMenu,
			nMaxDisplayable,
			nScrollOffset,
			bIsScrollBox,
			nDisplayMode,
			bValid;
} tMenuProps;

//------------------------------------------------------------------------------

class CMenu;

typedef int (*pMenuCallback) (CMenu& m, int& lastKey, int nItem, int nState);

class CMenu : public CStack<CMenuItem> {
	private:
		tMenuProps	m_props;
		int			m_nGroup;

	protected:
		int				m_bStart;
		int				m_nLastScrollCheck;
		int				m_bRedraw;
		int				m_bCloseBox;
		int				m_bDontRestore;
		int				m_bAllText;
		int				m_tEnter;
		int				m_nChoice;
		int				m_nKey;
		pMenuCallback	m_callback;

	public:
		CMenu () { Init (); }
		CMenu (uint nLength) { 
			Init (); 
			Create (nLength);
			}
		inline void Init (void) { 
			SetGrowth (10);
			m_nGroup = 0;
			}
		inline int NewGroup (int nGroup = 0) { 
			if (!nGroup)
				m_nGroup++; 
			else if (nGroup > 0) 
				m_nGroup = nGroup;
			else 
				m_nGroup--;
			return m_nGroup;
			}
		int AddCheck (const char* szText, int nValue, int nKey = 0, const char* szHelp = NULL);
		int AddRadio (const char* szText, int nValue, int nKey = 0, const char* szHelp = NULL);
		int AddMenu (const char* szText, int nKey = 0, const char* szHelp = NULL);
		int AddText (const char* szText, int nKey = 0);
		int AddSlider (const char* szText, int nValue, int nMin, int nMax, int nKey = 0, const char* szHelp = NULL);
		int AddInput (const char* szText, int nLen, const char* szHelp = NULL);
		int AddInput (const char* szText, char* szValue, int nLen, const char* szHelp = NULL);
		int AddInput (const char* szText, char* szValue, int nValue, int nLen, const char* szHelp = NULL);
		int AddInputBox (const char* szText, int nLen, int nKey = 0, const char* szHelp = NULL);
		int AddNumber (const char* szText, int nValue, int nMin, int nMax);
		int AddGauge (const char* szText, int nValue, int nMax);
		inline CMenuItem& Item (int i = -1) { return (i < 0) ? m_data.buffer [ToS () - 1] : m_data.buffer [i]; }

		int Menu (const char *pszTitle, const char *pszSubTitle, pMenuCallback callback = NULL, 
					 int *nCurItemP = NULL, char *filename = NULL, int width = -1, int height = -1, int bTinyMode = 0);

		int TinyMenu (const char *pszTitle, const char *pszSubTitle, pMenuCallback callBack = NULL);

		int FixedFontMenu (const char* pszTitle, const char* pszSubTitle, 
								 pMenuCallback callback, int* nCurItemP, char* filename, int width, int height);

		static void DrawCloseBox (int x, int y);
		void FadeIn (void);
		void FadeOut (const char* pszTitle = NULL, const char* pszSubTitle = NULL, CCanvas* gameCanvasP = NULL);

		virtual void Render (const char* pszTitle = NULL, const char* pszSubTitle = NULL, CCanvas* gameCanvasP = NULL);

	private:
		int InitProps (const char* pszTitle, const char* pszSubTitle);
		void GetTitleSize (const char* pszTitle, CFont *font, int& tw, int& th);
		int GetSize (int& w, int& h, int& aw, int& nMenus, int& nOthers);
		void SetItemPos (int twidth, int xOffs, int yOffs, int right_offset);

		void DrawRightStringWXY (int w1, int x, int y, const char* s);
		int CharAllowed (char c);
		void TrimWhitespace (char* text);
		int DrawTitle (const char* pszTitle, CFont *font, uint color, int ty);

		void SaveScreen (CCanvas **gameCanvasP);
		void RestoreScreen (char* filename, int bDontRestore);
		void FreeTextBms (void);
		void SwapText (int i, int j);
	};

//------------------------------------------------------------------------------

class CFileSelector : public CMenu {
	private:
		int					m_nFirstItem;
		int					m_nVisibleItems;
		int					m_bPlayerMode;
		int					m_nLeft;
		int					m_nTop;
		int					m_nWidth;
		int					m_nHeight;
		int					m_xOffset;
		int					m_yOffset;
		int					m_nFileCount;
		CArray<CFilename>	m_filenames;

	public:
		int FileSelector (const char *pszTitle, const char *filespec, char *filename, int bAllowAbort);
		virtual void Render (const char* pszTitle = NULL, const char* pszSubTitle = NULL, CCanvas* gameCanvasP = NULL);
	};

//------------------------------------------------------------------------------

typedef int (*pListBoxCallback) (int* nItem, CArray<char*>& items, int* keypress);

class CListBox : public CMenu {
		int				m_nFirstItem;
		int				m_nVisibleItems;
		int				m_nWidth;
		int				m_nHeight;
		int				m_xOffset;
		int				m_yOffset;
		int				m_nTitleHeight;
		CStack<char*>*	m_items;

	public:
		int ListBox (const char* pszTitle, CStack<char*>& items, int nDefaultItem = 0, int bAllowAbort = 1, pListBoxCallback callback = NULL);
		virtual void Render (const char* pszTitle = NULL, const char* pszSubTitle = NULL, CCanvas* gameCanvasP = NULL);
	};

//------------------------------------------------------------------------------

class CMessageBox : public CMenu {
		int			m_nDrawBuffer;
		const char*	m_pszMsg;

	public:
		~CMessageBox () { Clear (); }
		void Show (const char *pszMsg, bool bFade = true);
		void Clear (void);
		virtual void Render (const char* pszTitle = NULL, const char* pszSubTitle = NULL, CCanvas* gameCanvasP = NULL);
	};

extern CMessageBox messageBox;

//------------------------------------------------------------------------------

// This function pops up a messagebox and returns which choice was selected...
// Example:
// MsgBox( "Title", "Subtitle", 2, "Ok", "Cancel", "There are %d objects", nobjects );
// Returns 0 through nChoices-1.
int _CDECL_ MsgBox (const char *pszTitle, char *filename, int nChoices, ...);

// Same as above, but you can pass a function
int _CDECL_ MsgBox (const char *pszTitle, pMenuCallback callBack, char *filename, int nChoices, ...);

void ProgressBar (const char *szCaption, int nCurProgress, int nMaxProgress, pMenuCallback doProgress);

int FileList (const char *pszTitle, const char *filespec, char *filename);

int stoip (char *szServerIpAddr, ubyte *pIpAddr);
int stoport (char *szPort, int *pPort, int *pSign);
int SetCustomDisplayMode (int x, int y);

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
void MultiplayerMenu (void);
void NewGameMenu (void);
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

int MainMenu (void);
void ConfigMenu (void);
void RenderOptionsMenu (void);
void GameplayOptionsMenu (void);
int QuitSaveLoadMenu (void);
int SelectAndLoadMission (int bMulti, int *bAnarchyOnly);

void InitDetailLevels (int detailLevel);

int SwitchDisplayMode (int dir);

void IpxSetDriver (int ipx_driver);
void DoNewIPAddress (void);

//------------------------------------------------------------------------------

#endif /* _MENU_H */
