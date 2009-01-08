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


#ifndef _TITLES_H
#define _TITLES_H

//-----------------------------------------------------------------------------

typedef struct tBriefingScreen {
	char    szName [14];                //  filename, eg merc01.  Assumes .lbm suffix.
	sbyte   nLevel;
	sbyte   nMessage;
	short   textLeft, textTop;         //  upper left x, y of text window
	short   textWidth, textHeight;    //  width and height of text window
} tBriefingScreen;


class CBriefingInfo {
	public:
		char*					message;
		int					nScreen;
		int					nLevel;
		tBriefingScreen	curScreen;
		tBriefingScreen*	bsP;
		char*					pi;
		char*					pj;
		int					ch;
		int					prevCh;
		int					bPageDone;
		int					bRedraw;
		int					bKeyCheck;
		int					bFlashingCursor;
		int					bNewPage;
		int					bHaveScreen;
		int					bDumbAdjust;
		int					bChattering;
		int					bGotZ;
		int					bOnlyRobots;
		int					bExtraSounds;
		int					nHumChannel;
		int					nPrintingChannel;
		int					nBotChannel;
		int					nLineAdjustment;
		int					nDelayCount;
		int					nRobot;
		int					bInitRobot;
		int					bRobotPlaying;
		int					nBotSig;
		int					nDoorDir;
		int					nDoorDivCount; 
		int					nAnimatingBitmapType;
		int					tAnimate;
		int					bInitAnimate;
		int					x;
		int					y;
		int					briefingTextX;
		int					briefingTextY;
		int					nCurrentColor;
		uint					nEraseColor;
		char					szSpinningRobot [8];
		char					szBriefScreen [15];
		char					szBriefScreenB [15];
		char					szCurImage [15];
		char					szBitmapName [32];
		CCharArray			briefingText;
		int					nBriefingTextLen;
		int					nTabStop;
		CCanvas*				robotCanvP;
		CAngleVector		vRobotAngles;
		time_t				t0;
		int					nFuncRes;

	public:
		void Init (void);
		void Setup (char* message, int nLevel, int nScreen);
};

//-----------------------------------------------------------------------------

typedef struct tD1ExtraBotSound {
	const char*	pszName;
	short			nLevel;
	short			nBotSig;
} tD1ExtraBotSound;


class CBriefing {
	private:
		CBriefingInfo	m_info;

	public:
		CBriefing () { Init (); }
		void Init (void);
		void Run (const char* filename, int nLevel);

		int HandleA (void);
		int HandleB (void);
		int HandleC (void);
		int HandleD (void);
		int HandleF (void);
		int HandleN (void);
		int HandleO (void);
		int HandleP (void);
		int HandleR (void);
		int HandleS (void);
		int HandleT (void);
		int HandleU (void);
		int HandleZ (void);
		int HandleTAB (void);
		int HandleBS (void);
		int HandleNEWL (void);
		int HandleSEMI (void);
		int HandleANY (void);

	private:
		int StartSound (int nChannel, short nSound, fix nVolume, const char* pszWAV);
		void StopSound (int& nChannel);
		int StartHum (int nChannel, int nLevel, int nScreen, int bExtraSounds);
		tD1ExtraBotSound* FindExtraBotSound (short nLevel, short nBotSig);
		int StartExtraBotSound (int nChannel, short nLevel, short nBotSig);

		int RenderImage (char* pszImg);
		void RenderBitmapFrame (int bRedraw);
		void RenderBitmap (CBitmap* bmp);
		void RenderRobotFrame (void);
		void RenderRobotMovie (void);
		void InitSpinningRobot (void); 
		void Animate (void);

		int LoadImage (char* szBriefScreen);
		int LoadImage (int nScreen);
		int ParseMessageInt (char*& pszMsg, bool bSkip = false);
		void ParseMessageText (char* pszName);
		const char* NextPage (const char* message);
		int PageHasRobot (const char* message);

		void InitCharPos (tBriefingScreen* bsP, int bRescale);
		int PrintCharDelayed (int delay);
		void FlashCursor (int bCursor);

		int InKey (void);
		int WaitForKeyPress (void);

		int DefineBox (void);

		int HandleInput (void);
		int HandleNewPage (void);

		int ShowMessage (int nScreen, char* message, int nLevel);
		char* GetMessage (int nScreen);
		int LoadImageText (char* filename, CCharArray& buf);
		int ShowText (int nScreen, short nLevel);
		int LoadLevelScreen (int nScreen, short nLevel);
		void SetColors (void);

		char* SkipPage (void); 

		inline int RescaleX (int x) { return x*  CCanvas::Current ()->Width () / 320; }
		inline int RescaleY (int y) { return y*  CCanvas::Current ()->Height () / 200; }
	};

extern CBriefing briefing;

//-----------------------------------------------------------------------------

// Descent 1 briefings

extern int ShowTitleScreen (const char* filename, int bAllowKeys, int from_hog_only);
extern int ShowBriefingScreen (const char* filename, int bAllowKeys);
extern void ShowTitleFlick (const char* name, int bAllowKeys);
extern void DoBriefingScreens(const char* filename,int nLevel);
extern char* GetBriefingScreen (int nLevel);

extern void show_endgame_briefing(void);

#endif
