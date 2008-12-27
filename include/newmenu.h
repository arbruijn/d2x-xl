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

#ifndef _NEWMENU_H
#define _NEWMENU_H

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

#define NM_MAX_TEXT_LEN     50

//------------------------------------------------------------------------------

class CMenuItem {
	public:
		int			m_nType;           // What kind of item this is, see NM_TYPE_????? defines
		int			m_value;          // For checkboxes and radio buttons, this is 1 if marked initially, else 0
		int			m_minValue, m_maxValue;   // For sliders and number bars.
		int			m_group;          // What group this belongs to for radio buttons.
		int			m_nTextLen;       // The maximum length of characters that can be entered by this inputboxes
		char*			m_text;          // The text associated with this item.
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
		char			m_savedText [NM_MAX_TEXT_LEN+1];
		CBitmap*		m_bmText [2];
		char*			m_szHelp;

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


class CMenuManager;

typedef int (*pMenuCallback) (CMenuManager& m, int& lastKey, int nItem);

class CMenuManager : public CStack<CMenuItem> {
	private:
		int			m_opt;
		tMenuProps	m_props;

	public:
		CMenuManager () { Init (); }
		inline void Init (void) { 
			SetGrowth (10);
			m_opt = 0; 
			}
		int AddCheck (char* szText, int nValue, int nKey = 0, char* szHelp = NULL);
		int AddRadio (char* szText, int nValue, int nGroup, int nKey = 0, char* szHelp = NULL);
		int AddMenu (char* szText, int nKey = 0, char* szHelp = NULL);
		int AddText (char* szText, int nKey = 0);
		int AddSlider (char* szText, int nValue, int nMin, int nMax, int nKey = 0, char* szHelp = NULL);
		int AddInput (char* szText, int nLen, char* szHelp = NULL);
		int AddInputBox (char* szText, int nLen, int nKey = 0, char* szHelp = NULL);
		int AddGauge (char* szText, int nValue, int nMax);
		inline CMenuItem& Item (int i = -1) { return (i < 0) ? m_data.buffer [m_opt] : m_data.buffer [i]; }

		int Menu (const char *pszTitle, const char *pszSubTitle, pMenuCallback callback = NULL, 
					 int *nCurItemP = NULL, char *filename = NULL, int width = -1, int height = -1, int bTinyMode = 0);

		int TinyMenu (const char *pszTitle, const char *pszSubTitle, pMenuCallback callBack);

		int FixedFontMenu (const char* pszTitle, const char* pszSubTitle, 
								 pMenuCallback callback, int* nCurItemP, char* filename, int width, int height);

		static void DrawCloseBox (int x, int y);

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
	};

//------------------------------------------------------------------------------

// This function pops up a messagebox and returns which choice was selected...
// Example:
// ExecMessageBox( "Title", "Subtitle", 2, "Ok", "Cancel", "There are %d objects", nobjects );
// Returns 0 through nChoices-1.
int _CDECL_ MessageBox (const char *pszTitle, char *filename, int nChoices, ...);

// Same as above, but you can pass a function
int _CDECL_ MessageBox (const char *pszTitle, pMenuCallback callBack, char *filename, int nChoices, ...);

int FileSelector (const char *pszTitle, const char *filespec, char *filename, int bAllowAbort);

void ProgressBar (const char *szCaption, int nCurProgress, int nMaxProgress, pMenuCallback doProgress);

int FileList (const char *pszTitle, const char *filespec, char *filename);

typedef int (*pListBoxCallback) (int* nItem, CArray<char*>& items, int* keypress);

int ListBox (const char *pszTitle, CStack<char*>& items, int bAllowAbort, pListBoxCallback callBack);

extern int Max_linear_depthObjects;

extern char *nmAllowedChars;

#endif /* _NEWMENU_H */
