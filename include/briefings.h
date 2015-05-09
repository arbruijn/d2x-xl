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
	int8_t   nLevel;
	int8_t   nMessage;
	int16_t   textLeft, textTop;         //  upper left x, y of text window
	int16_t   textWidth, textHeight;    //  width and height of text window
} tBriefingScreen;


class CBriefingInfo {
	public:
		char*					message;
		int32_t				nScreen;
		int32_t				nLevel;
		tBriefingScreen	curScreen;
		tBriefingScreen*	pScreen;
		char*					pi;
		char*					pj;
		int32_t				ch;
		int32_t				prevCh;
		int32_t				bPageDone;
		int32_t				bRedraw;
		int32_t				bKeyCheck;
		int32_t				bFlashingCursor;
		int32_t				bNewPage;
		int32_t				bHaveScreen;
		int32_t				bDumbAdjust;
		int32_t				bChattering;
		int32_t				bGotZ;
		int32_t				bOnlyRobots;
		int32_t				bExtraSounds;
		int32_t				nHumChannel;
		int32_t				nPrintingChannel;
		int32_t				nBotChannel;
		int32_t				nLineAdjustment;
		int32_t				nDelayCount;
		int32_t				nRobot;
		int32_t				bInitRobot;
		int32_t				bRobotPlaying;
		int32_t				nBotSig;
		int32_t				nDoorDir;
		int32_t				nDoorDivCount; 
		int32_t				nAnimatingBitmapType;
		int32_t				tAnimate;
		int32_t				bInitAnimate;
		int32_t				x;
		int32_t				y;
		int32_t				briefingTextX;
		int32_t				briefingTextY;
		int32_t				nCurrentColor;
		uint32_t				nEraseColor;
		char					szSpinningRobot [8];
		char					szBriefScreen [15];
		char					szBriefScreenB [15];
		char					szCurImage [15];
		char					szBitmapName [32];
		CCharArray			briefingText;
		int32_t				nBriefingTextLen;
		int32_t				nTabStop;
		CCanvas				animCanv;
		CAngleVector		vRobotAngles;
		time_t				t0;
		int32_t				nFuncRes;

	public:
		void Init (void);
		void Setup (char* message, int32_t nLevel, int32_t nScreen);
};

//-----------------------------------------------------------------------------

typedef struct tD1ExtraBotSound {
	const char*	pszName;
	int16_t			nLevel;
	int16_t			nBotSig;
} tD1ExtraBotSound;


class CBriefing {
	private:
		CBriefingInfo	m_info;
		CBitmap			m_background;
		CBitmap*			m_bitmap;
		char				m_szBackground [FILENAME_LEN + 1];
		char				m_message [2];
		int32_t				m_pcxError;

	public:
		CBriefing () { Init (); }
		void Init (void);
		void Run (const char* filename, int32_t nLevel);

		int32_t HandleA (void);
		int32_t HandleB (void);
		int32_t HandleC (void);
		int32_t HandleD (void);
		int32_t HandleF (void);
		int32_t HandleN (void);
		int32_t HandleO (void);
		int32_t HandleP (void);
		int32_t HandleR (void);
		int32_t HandleS (void);
		int32_t HandleT (void);
		int32_t HandleU (void);
		int32_t HandleZ (void);
		int32_t HandleTAB (void);
		int32_t HandleBS (void);
		int32_t HandleNEWL (void);
		int32_t HandleSEMI (void);
		int32_t HandleANY (void);

	private:
		int32_t StartSound (int32_t nChannel, int16_t nSound, fix nVolume, const char* pszWAV);
		void StopSound (int32_t& nChannel);
		int32_t StartHum (int32_t nChannel, int32_t nLevel, int32_t nScreen, int32_t bExtraSounds);
		tD1ExtraBotSound* FindExtraBotSound (int16_t nLevel, int16_t nBotSig);
		int32_t StartExtraBotSound (int32_t nChannel, int16_t nLevel, int16_t nBotSig);

		int32_t RenderImage (char* pszImg);
		void RenderBitmapFrame (int32_t bRedraw);
		void RenderBitmap (CBitmap* bmp);
		void RenderRobotFrame (void);
		void RenderRobotMovie (void);
		void InitSpinningRobot (void); 
		void Animate (void);

		int32_t LoadImage (char* szBriefScreen);
		int32_t LoadImage (int32_t nScreen);
		int32_t ParseMessageInt (char*& pszMsg, bool bSkip = false);
		void ParseMessageText (char* pszName);
		const char* NextPage (const char* message);
		int32_t PageHasRobot (const char* message);

		void InitCharPos (tBriefingScreen* bsP, int32_t bRescale);
		int32_t PrintCharDelayed (int32_t delay);
		void FlashCursor (int32_t bCursor);

		int32_t InKey (void);
		int32_t WaitForKeyPress (void);

		int32_t DefineBox (void);

		int32_t HandleInput (void);
		int32_t HandleNewPage (void);

		int32_t ShowMessage (int32_t nScreen, char* message, int32_t nLevel);
		char* GetMessage (int32_t nScreen);
		int32_t LoadImageText (char* filename, CCharArray& buf);
		int32_t ShowText (int32_t nScreen, int16_t nLevel);
		int32_t LoadLevelScreen (int32_t nScreen, int16_t nLevel);
		void SetColors (void);

		char* SkipPage (void); 

		void SetupAnimationCanvas (CCanvas* baseCanv);

		CCanvas& AnimCanv (void) { return m_info.animCanv; }

		void RenderElement (int32_t nElement);

		float GetScale (void);
		inline int32_t Scaled (int32_t v) { return int32_t (FRound (v * GetScale ())); }
		inline int32_t AdjustX (int32_t x) { return x * CCanvas::Current ()->Width (false) / 320; }
		inline int32_t AdjustY (int32_t y) { return y * CCanvas::Current ()->Height (false) / 200; }
		inline int32_t RescaleX (int32_t x) { return Scaled (AdjustX (x)); }
		inline int32_t RescaleY (int32_t y) { return Scaled (AdjustY (y)); }

	};

extern CBriefing briefing;

//-----------------------------------------------------------------------------

#endif
