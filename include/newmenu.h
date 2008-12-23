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
#define NM_TYPE_INPUT       1   // An input box... fills the text field in, and you need to fill in text_len field.
#define NM_TYPE_CHECK       2   // A check box. Set and get its status by looking at flags field (1=on, 0=off)
#define NM_TYPE_RADIO       3   // Same as check box, but only 1 in a group can be set at a time. Set group fields.
#define NM_TYPE_TEXT        4   // A line of text that does nothing.
#define NM_TYPE_NUMBER      5   // A numeric entry counter.  Changes value from minValue to maxValue;
#define NM_TYPE_INPUT_MENU  6   // A inputbox that you hit Enter to edit, when done, hit enter and menu leaves.
#define NM_TYPE_SLIDER      7   // A slider from minValue to maxValue. Draws with text_len chars.
#define NM_TYPE_GAUGE       8   // A slider from minValue to maxValue. Draws with text_len chars.

#define NM_MAX_TEXT_LEN     50

//------------------------------------------------------------------------------

typedef struct tMenuItem {
	int			nType;           // What kind of item this is, see NM_TYPE_????? defines
	int			value;          // For checkboxes and radio buttons, this is 1 if marked initially, else 0
	int			minValue, maxValue;   // For sliders and number bars.
	int			group;          // What group this belongs to for radio buttons.
	int			text_len;       // The maximum length of characters that can be entered by this inputboxes
	char			*text;          // The text associated with this item.
	char			*textSave;
	uint color;
	short			key;
	// The rest of these are used internally by by the menu system, so don't set 'em!!
	short			x, y, xSave, ySave;
	short			w, h;
	short			right_offset;
	ubyte			redraw;
	ubyte			rebuild;
	ubyte			noscroll;
	ubyte			unavailable;
	ubyte			centered;
	char			saved_text [NM_MAX_TEXT_LEN+1];
	CBitmap		*text_bm [2];
	char			*szHelp;
} tMenuItem;

//------------------------------------------------------------------------------

typedef struct bkg {
	short			x, y, w, h;		// The location of the menu.
	CCanvas*		menu_canvas;
	CBitmap*		saved;			// The background under the menu.
	CBitmap*		background;
	CBitmap*		bmP;
	char			bIgnoreBg;
	char			bIgnoreCanv;
	char			*pszPrevBg;
} bkg;

class CBackground {
	private:
		CCanvas*	m_menu;			// canvas (screen area) of a menu
		CBitmap*	m_saved;			// copy of a screen area covered by a menu
		CBitmap*	m_background;	// complete background
		char*		m_name;
		bool		m_bIgnoreCanv;
		bool		m_bIgnoreBg;
		bool		m_bTopMenu;
		bool		m_bMenuBox;

	public:
		CBackground () { Init (); }
		~CBackground () { Destroy (); }
		void Init (void);
		void Destroy (void);
		bool Create (void);
		void Save (bool bForce);
		void Restore (void);
		void Draw (void);
		void Box (void);
		bool Load (char* filename);

		static inline CCanvas* Canvas () { return m_canvas; }
};

class CBackgroundManager : private CStack<CBackground> {
	private:
		CStack<CBackground>	m_save;
		CBackground				m_bg;
		CBitmap*					m_background;
		int						m_nDepth;
		char*						m_filename;

	public:
		CBackgroundManager ();
		void Setup (char* filename);
		void Load (void);
		void Restore (void);
		void Remove (void);
		void LoadBackground (char* filename);

	private:
		bool IsDefault (char* filename);
		void LoadCustomBackground (void);
	};

extern  CBackgroundManager backgroundManager;

//------------------------------------------------------------------------------

// Pass an array of newmenu_items and it processes the menu. It will
// return a -1 if Esc is pressed, otherwise, it returns the index of
// the item that was current when Enter was was selected.
// The menuCallback function gets called constantly, so you can dynamically
// change the text of an item.  Just pass NULL if you don't want this.
// Title draws big, Subtitle draw medium sized.  You can pass NULL for
// either/both of these if you don't want them.
int ExecMenu (const char *pszTitle, const char *pszSubTitle, int nItems, tMenuItem *item, 
				  int (*menuCallback)(int nItems, tMenuItem *items, int *lastKeyP, int nItem),
				  char *filename);

// Same as above, only you can pass through what item is initially selected.
int ExecMenu1 (const char *pszTitle, const char *pszSubTitle, int nItems, tMenuItem *item, 
				   int (*menuCallback)(int nItems, tMenuItem *items, int *lastKeyP, int nItem), 
				   int *pnItem);

// Same as above, only you can pass through what background bitmap to use.
int ExecMenu2 (const char *pszTitle, const char *pszSubTitle, int nItems, tMenuItem *item, 
				   int (*menuCallback)(int nItems, tMenuItem *items, int *lastKeyP, int nItem), 
				   int *pnItem, char *filename);

// Same as above, only you can pass through the width & height
int ExecMenu3 (const char *pszTitle, const char *pszSubTitle, int nItems, tMenuItem *item, 
					 int (*menuCallback)(int nItems, tMenuItem *items, int *lastKeyP, int nItem), 
					 int *pnItem, char *filename, int width, int height);

void NMLoadBackground (char *filename, CBackground& bg, int bReload);

void NMDrawBackground (CBackground& bg, int x1, int y1, int x2, int y2, int bReload);

void NMRemoveBackground (CBackground& bg);


// This function pops up a messagebox and returns which choice was selected...
// Example:
// ExecMessageBox( "Title", "Subtitle", 2, "Ok", "Cancel", "There are %d objects", nobjects );
// Returns 0 through nChoices-1.
int _CDECL_ ExecMessageBox (const char *pszTitle, char *filename, int nChoices, ...);
// Same as above, but you can pass a function
int _CDECL_ ExecMessageBox1 (
					const char *pszTitle,
					int (*menuCallback)(int nItems, tMenuItem *items, int *lastKeyP, int nItem), 
					char *filename, int nChoices, ...);

// Returns 0 if no file selected, else filename is filled with selected file.
int ExecMenuFileSelector (const char *pszTitle, const char *filespec, char *filename, int bAllowAbort);

// in menu.c
extern int Max_linear_depthObjects;

extern char *nmAllowedChars;

void NMInitBackground (char *filename, CBackground& bg, int x, int y, int w, int h, int bRedraw);
void NMDrawBackground (CBackground& bg, int x1, int y1, int x2, int y2, int bRedraw);

int ExecMenuListBox (const char *pszTitle, int nItems, char *itemP [], int bAllowAbort, 
						   int (*listbox_callback)(int *nItem, int *nItems, char *itemP [], int *keypress));
int ExecMenuListBox1 (const char *pszTitle, int nItems, char *itemP [], int bAllowAbort, int nDefaultItem, 
							 int (*listbox_callback)(int *nItem, int *nItems, char *itemP [], int *keypress));

int ExecMenuFileList (const char *pszTitle, const char *filespace, char *filename);

int ExecMenuTiny (const char *pszTitle, const char *pszSubTitle, int nItems, tMenuItem *itemP, 
						 int (*menuCallback) (int nItems, tMenuItem *itemP, int *lastKeyP, int nItem));

int ExecMenutiny2 (const char *pszTitle, const char *pszSubTitle, int nItems, tMenuItem *itemP, 
							int (*menuCallback) (int nItems,tMenuItem *itemP, int *lastKeyP, int nItem));

void NMProgressBar (const char *szCaption, int nCurProgress, int nMaxProgress, 
						  int (*doProgress) (int nItems, tMenuItem *items, int *lastKeyP, int cItem));

int ExecMenutiny2 (const char *pszTitle, const char *pszSubTitle, int nItems, tMenuItem *itemP, 
						 int (*menuCallback) (int nItems, tMenuItem *itemP, int *lastKeyP, int nItem));

//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
int _CDECL_ NMMsgBoxFixedFont (const char *pszTitle, int nChoices, ...);
//end this section addition

//should be called whenever the palette changes
void NMRemapBackground (void);
void NMLoadAltBg (void);
int NMFreeAltBg (int bForce);

void NMRestoreScreen (char *filename, CBackground& bg, CCanvas *saveCanvasP, CFont *saveFontP, int bDontRestore);
void NMBlueBox (int x1, int y1, int x2, int y2, int nLineWidth, float fAlpha, int bForce);

extern double altBgAlpha;
extern double altBgBrightness;
extern int altBgGrayScale;
extern char altBgName [FILENAME_LEN];
extern char bAlreadyShowingInfo;

#define STARS_BACKGROUND \
			((gameStates.menus.bHires && CFile::Exist ("starsb.pcx", gameFolders.szDataDir, 0)) ? "starsb.pcx":\
			CFile::Exist ("stars.pcx", gameFolders.szDataDir, 0) ? "stars.pcx" : "starsb.pcx")

#endif /* _NEWMENU_H */
