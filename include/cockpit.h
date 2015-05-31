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

#ifndef _COCKPIT_H
#define _COCKPIT_H

#include "fix.h"
#include "gr.h"
#include "piggy.h"
#include "object.h"
#include "hudmsgs.h"
#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
#include "ogl_render.h"
#include "endlevel.h"
//from gauges.c

// Flags for gauges/hud stuff
extern uint8_t Reticle_on;

// Call to flash a message on the HUD.  Returns true if message drawn.
// (message might not be drawn if previous message was same)
#define gauge_message HUDInitMessage

//valid modes for cockpit
#define CM_FULL_COCKPIT    0	// normal screen with cockpit
#define CM_REAR_VIEW       1	// looking back with bitmap
#define CM_STATUS_BAR      2	// small status bar, w/ reticle
#define CM_FULL_SCREEN     3	// full screen, no cockpit (w/ reticle)
#define CM_LETTERBOX       4	// half-height window (for cutscenes)

#define WBU_WEAPON			0	// the weapons display
#define WBUMSL					1	// the missile view
#define WBU_ESCORT			2	// the "buddy bot"
#define WBU_REAR				3	// the rear view
#define WBU_COOP				4	// coop or team member view
#define WBU_GUIDED			5	// the guided missile
#define WBU_MARKER			6	// a dropped marker
#define WBU_STATIC			7	// playing static after missile hits
#define WBU_RADAR_TOPDOWN	8
#define WBU_RADAR_HEADSUP	9

//	-----------------------------------------------------------------------------

#define HUD_SCALE(v, s)			(int32_t (float (v) * (s)))
#define HUD_SCALE_X(v)			HUD_SCALE (v, m_info.xScale)
#define HUD_SCALE_Y(v)			HUD_SCALE (v, m_info.yScale)
#define HUD_LHX(x)				(gameStates.menus.bHires ? 2 * (x) : x)
#define HUD_LHY(y)				(gameStates.menus.bHires? (24 * (y)) / 10 : y)
#define HUD_ASPECT				(float (CCanvas::Current ()->Height ()) / float (CCanvas::Current ()->Width ()) / 0.75f)

#define X_GAUGE_BASE_OFFSET	20
#define X_GAUGE_OFFSET(_pos)	(20 + (_pos) * 20)
#define Y_GAUGE_OFFSET(_pos)	(30 + (_pos) * 60)

//	-----------------------------------------------------------------------------

typedef struct tSpan {
	int8_t l, r;
} tSpan;

typedef struct tGaugeBox {
	int32_t left, top;
	int32_t right, bot;		//maximal box
	tSpan *spanlist;	//list of left, right spans for copy
} tGaugeBox;

//	-----------------------------------------------------------------------------

class CCockpitHistory {
	public:
		int32_t		score;
		int32_t		energy;
		int32_t		shield;
		uint32_t		flags;
		int32_t		bCloak;
		int32_t		lives;
		fix			afterburner;
		int32_t		bombCount;
		int32_t		laserLevel;
		int32_t		weapon [2];
		int32_t		ammo [2];
		fix			xOmegaCharge;

	public:
		void Init (void);
};

class CCockpitInfo {
	public:
		int32_t		nCloakFadeState;
		int32_t		bLastHomingWarningDrawn [2];
		int32_t		addedScore [2];
		fix			scoreTime;
		fix			lastWarningBeepTime [2];
		int32_t		bHaveGaugeCanvases;
		int32_t		nInvulnerableFrame;
		int32_t		weaponBoxStates [2];
		fix			weaponBoxFadeValues [2];
		int32_t		weaponBoxUser [2];
		int32_t		nLineSpacing;
		int32_t		nType;
		int32_t		nColor;
		float			xScale;
		float			yScale;
		float			xGaugeScale;
		float			yGaugeScale;
		float			fontScale;
		int32_t		fontWidth;
		int32_t		fontHeight;
		uint32_t		fontColor;
		int32_t		heightPad;
		int32_t		nCockpit;
		int32_t		nShield;
		int32_t		nEnergy;
		int32_t		bCloak;
		int32_t		bAdjustCoords;
		int32_t		nDamage [3];
		fix			tInvul;
		fix			xStereoSeparation;
		CCanvas		windows [2];
		bool			bRebuild;

		static bool	bWindowDrawn [2];

	public:
		void Init (void);
};

//	-----------------------------------------------------------------------------

class CGenericCockpit {
	protected:
		CCockpitHistory		m_history [2];
		CCockpitInfo			m_info;
		static CStack<int32_t>	m_save;

	public:
		CGenericCockpit();
		void Init (void);

		static bool Save (bool bInitial = false);
		static bool Restore (void);
		static bool IsSaved (void);
		static void Rewind (bool bActivate = true);

		inline float Aspect (void) { return float (gameData.renderData.screen.Height ()) / float (gameData.renderData.screen.Width ()) / 0.75f; }
		inline int32_t ScaleX (int32_t v) { return (int32_t) FRound (float (v) * m_info.xScale); }
		inline int32_t ScaleY (int32_t v) { return (int32_t) FRound (float (v) * m_info.yScale); }
		inline int32_t LHX (int32_t x) { return x << gameStates.render.fonts.bHires; }
		inline int32_t LHY (int32_t y) { return gameStates.render.fonts.bHires ? 24 * y / 10 : y; }
		inline float FontScale (void) { return m_info.fontScale; }
		inline void SetFontScale (float fontScale) { m_info.fontScale = (fontScale < 1.0f) ? 1.0f : fontScale; }
		inline uint32_t FontColor (void) { return m_info.fontColor; }
		inline void SetFontColor (uint32_t fontColor) { m_info.fontColor = fontColor; }
		inline CCanvas* Canvas (void) { return CCanvas::Current (); }
		inline CCanvas& Window (int32_t nWindow) { return m_info.windows [ nWindow]; }
		inline void PageInGauge (int32_t nGauge) { LoadTexture (gameData.cockpitData.gauges [!gameStates.render.fonts.bHires][nGauge].index, 0, 0); }
		inline uint16_t GaugeIndex (int32_t nGauge) { return gameData.cockpitData.gauges [!gameStates.render.fonts.bHires][nGauge].index; }
		CBitmap* BitBlt (int32_t nGauge, int32_t x, int32_t y, bool bScalePos = true, bool bScaleSize = true, int32_t scale = I2X (1), int32_t orient = 0, CBitmap* pBm = NULL, CBitmap* pBmo = NULL);
		int32_t _CDECL_ PrintF (int32_t *idP, int32_t x, int32_t y, const char *pszFmt, ...);
		void Rect (int32_t left, int32_t top, int32_t width, int32_t height) {
			OglDrawFilledRect (HUD_SCALE_X (left), HUD_SCALE_Y (top), HUD_SCALE_X (width), HUD_SCALE_Y (height));
			}

		char* ftoa (char *pszVal, fix f);
		char* Convert1s (char* s);
		void DemoRecording (void);

		void DrawMarkerMessage (void);
		void DrawMultiMessage (void);
		void DrawCountdown (int32_t y);
		void DrawRecording (int32_t y);
		void DrawPacketLoss (void);
		void DrawFrameRate (void);
		void DrawPlayerStats (void);
		void DrawTime (void);
		void DrawTimerCount (void);
		void PlayHomingWarning (void);
		void DrawCruise (int32_t x, int32_t y);
		void DrawBombCount (int32_t x, int32_t y, int32_t bgColor, int32_t bShowAlways);
		void DrawAmmoInfo (int32_t x, int32_t y, int32_t ammoCount, int32_t bPrimary);
		void CheckForExtraLife (int32_t nPrevScore);
		void AddPointsToScore (int32_t points);
		void AddBonusPointsToScore (int32_t points);
		void DrawPlayerShip (int32_t nCloakState, int32_t nOldCloakState, int32_t x, int32_t y);
		void DrawWeaponInfo (int32_t nWeaponType, int32_t nIndex, tGaugeBox *box, int32_t xPic, int32_t yPic, const char *pszName, int32_t xText, int32_t yText, int32_t orient);
		int32_t DrawWeaponDisplay (int32_t nWeaponType, int32_t nWeaponId);
		void DrawStatic (int32_t nWindow, int32_t nIndex);
		void DrawOrbs (int32_t x, int32_t y);
		void DrawFlag (int32_t x, int32_t y);
		void DrawKillList (int32_t x, int32_t y);
		void DrawModuleDamage (void);
		void DrawCockpit (int32_t nCockpit, int32_t y, bool bAlphaTest = false);
		void UpdateLaserWeaponInfo (void);
		void DrawReticle (int32_t bForceBig, fix xStereoSeparation = 0);
		int32_t CanSeeObject (int32_t nObject, int32_t bCheckObjs);
		void DrawPlayerNames (void);
		void RenderWindows (void);
		void Render (int32_t bExtraInfo, fix xStereoSeparation = 0);
		void RenderWindow (int32_t nWindow, CObject *pViewer, int32_t bRearView, int32_t nUser, const char *pszLabel);
		void Activate (int32_t nType, bool bClearMessages = false);

		virtual void GetHostageWindowCoords (int32_t& x, int32_t& y, int32_t& w, int32_t& h) = 0;
		virtual void DrawRecording (void) = 0;
		virtual void DrawCountdown (void) = 0;
		virtual void DrawCruise (void) = 0;
		virtual void DrawScore (void) = 0;
		virtual void DrawAddedScore (void) = 0;
		virtual void DrawHomingWarning (void) = 0;
		virtual void DrawKeys (void) = 0;
		virtual void DrawOrbs (void) = 0;
		virtual void DrawFlag (void) = 0;
		virtual void DrawShieldText (void) = 0;
		virtual void DrawShieldBar (void) = 0;
		virtual void DrawEnergyText (void) = 0;
		virtual void DrawEnergyBar (void) = 0;
		virtual void DrawAfterburnerText (void) = 0;
		virtual void DrawAfterburnerBar (void) = 0;
		virtual void DrawEnergyLevels (void) {
			DrawEnergyBar ();
			DrawAfterburnerBar ();
			DrawShieldBar ();
			DrawEnergyText ();
			DrawShieldText ();
			DrawAfterburnerText ();
			}

		virtual void ClearBombCount (int32_t bgColor) = 0;
		virtual void DrawBombCount (void) = 0;
		virtual int32_t DrawBombCount (int32_t& nIdBombCount, int32_t y, int32_t x, int32_t nColor, char* pszBombCount) = 0;
		virtual void DrawPrimaryAmmoInfo (int32_t ammoCount) = 0;
		virtual void DrawSecondaryAmmoInfo (int32_t ammoCount) = 0;
		virtual void DrawCloak (void) = 0;
		virtual void DrawInvul (void) = 0;
		virtual void DrawLives (void) = 0;
		virtual void DrawPlayerShip (void) = 0;
		virtual void DrawKillList (void) = 0;
		virtual void DrawStatic (int32_t nWindow) = 0;
		virtual void Toggle (void) = 0;
		virtual void DrawWeaponInfo (int32_t nWeaponType, int32_t nWeaponId, int32_t laserLevel);
		virtual void DrawWeapons (void);
		virtual void DrawCockpit (bool bAlphaTest = false) = 0;
		virtual void SetupWindow (int32_t nWindow) = 0;
		virtual bool Setup (bool bScene = false, bool bRebuild = false);
		virtual void DrawSlowMotion (void);

		inline CCockpitInfo& Info (void) { return m_info; }
		inline int32_t Type (void) { return m_info.nType; }
		inline int32_t LineSpacing (void) { return m_info.nLineSpacing = GAME_FONT->Height () + GAME_FONT->Height () / 4; }
		inline float XScale (void) { return m_info.xScale = CCanvas::Current ()->XScale (); }
		inline float YScale (void) { return m_info.yScale = CCanvas::Current ()->YScale (); }
		inline void SetScales (float xScale, float yScale) { m_info.xScale = xScale, m_info.yScale = yScale; }
		inline void Rebuild (void) { m_info.bRebuild = true; }

		int32_t WidthPad (char* pszText);
		int32_t WidthPad (int32_t nValue);
		int32_t HeightPad (void);

		inline bool ShowAlways (void) { 
			return (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nType == CM_STATUS_BAR); 
			}

		inline bool Show (void) { 
			return !gameStates.app.bEndLevelSequence && (!gameStates.app.bNostalgia || gameOpts->render.cockpit.bHUD || !ShowAlways ()); 
			}
		
		inline bool Hide (void) {
			return (gameStates.app.bEndLevelSequence >= EL_LOOKBACK) || 
					 (!(gameStates.app.bNostalgia || gameOpts->render.cockpit.bHUD) && (gameStates.render.cockpit.nType >= CM_FULL_SCREEN));
			}

		int32_t BombCount (int32_t& nBombType);
		inline int32_t ScoreTime (void) { return m_info.scoreTime; }
		inline int32_t SetScoreTime (int32_t nTime) { return m_info.scoreTime = nTime; }
		inline int32_t AddedScore (int32_t i = 0) { return m_info.addedScore [i]; }
		inline void AddScore (int32_t i, int32_t nScore) { m_info.addedScore [i] += nScore; }
		inline void SetAddedScore (int32_t i, int32_t nScore) { m_info.addedScore [i] = nScore; }
		inline void SetLineSpacing (int32_t nLineSpacing) { m_info.nLineSpacing = nLineSpacing; }
		inline void SetColor (int32_t nColor) { m_info.nColor = nColor; }

		bool ShowTextGauges (void);
		
		int32_t X (int32_t x, bool bForce = false);

		void ScaleUp (void);
		void ScaleDown (void);

		int32_t _CDECL_ DrawHUDText (int32_t *idP, int32_t x, int32_t y, const char * format, ...);

		int32_t AdjustCockpitXY (char* s, int32_t& x, int32_t& y);
		int32_t AdjustCockpitY (int32_t y);
		void SetupSceneCenter (CCanvas* refCanv, int32_t& w, int32_t& h);
	};

//	-----------------------------------------------------------------------------

class CHUD : public CGenericCockpit {
	public:
		virtual void GetHostageWindowCoords (int32_t& x, int32_t& y, int32_t& w, int32_t& h);
		virtual void DrawRecording (void);
		virtual void DrawCountdown (void);
		virtual void DrawCruise (void);
		virtual void DrawScore (void);
		virtual void DrawAddedScore (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawOrbs (void);
		virtual void DrawFlag (void);
		virtual void DrawShieldText (void);
		virtual void DrawShieldBar (void);
		virtual void DrawEnergyText (void);
		virtual void DrawEnergyBar (void);
		virtual void DrawAfterburnerText (void);
		virtual void DrawAfterburnerBar (void);
		virtual void DrawEnergyLevels (void);
		virtual void DrawSlowMotion (void);
		virtual void ClearBombCount (int32_t bgColor);
		virtual void DrawBombCount (void);
		virtual int32_t DrawBombCount (int32_t& nIdBombCount, int32_t y, int32_t x, int32_t nColor, char* pszBombCount);
		virtual void DrawPrimaryAmmoInfo (int32_t ammoCount);
		virtual void DrawSecondaryAmmoInfo (int32_t ammoCount);
		virtual void DrawWeapons (void);
		virtual void DrawCloak (void);
		virtual void DrawInvul (void);
		virtual void DrawLives (void);
		virtual void DrawPlayerShip (void) {}
		virtual void DrawStatic (int32_t nWindow);
		virtual void DrawKillList (void);
		virtual void DrawCockpit (bool bAlphaTest = false);
		virtual void Toggle (void);

		virtual void SetupWindow (int32_t nWindow);
		virtual bool Setup (bool bScene, bool bRebuild);

	private:
		void DrawEnergyLevelsCombined (void);
		void DrawInvulCloakCombined (void);
		int32_t FlashGauge (int32_t h, int32_t *bFlash, int32_t tToggle);
		inline int32_t LineSpacing (void) {
			return ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nType == CM_STATUS_BAR))
					 ? m_info.nLineSpacing
					 : 5 * GAME_FONT->Height () / 4;
			}

		char* StrPrimaryWeapon (char* szLabel);
		char* StrSecondaryWeapon (char* szLabel);
		int32_t StrWeaponStateColor (char* pszList, int32_t l, int32_t bAvailable, int32_t bActive);
		int32_t StrProxMineStatus (char* pszList, int32_t l, int32_t n, char tag, int32_t* nState);
		char* StrPrimaryWeaponList (char* pszList, char* pszAmmo);
		char* StrSecondaryWeaponList (char* pszList);
		void SetWeaponStateColor (int32_t bAvailable, int32_t bActive);
		void DrawPrimaryWeaponList (void);
		void DrawSecondaryWeaponList (void);
		void DrawSecondaryAmmoList (char* pszList);
		int32_t DrawAmmoCount (char* szLabel, int32_t x, int32_t y, int32_t j, int32_t k, int32_t* nState);

	};

//	-----------------------------------------------------------------------------

class CWideHUD : public CHUD {
	public:
		virtual void DrawRecording (void);
		virtual void Toggle (void);
		virtual bool Setup (bool bScene, bool bRebuild);
		virtual void SetupWindow (int32_t nWindow);
	};

//	-----------------------------------------------------------------------------

class CStatusBar : public CGenericCockpit {
	public:
		CBitmap* StretchBlt (int32_t nGauge, int32_t x, int32_t y, double xScale, double yScale, int32_t scale = I2X (1), int32_t orient = 0);

		virtual void GetHostageWindowCoords (int32_t& x, int32_t& y, int32_t& w, int32_t& h);
		virtual void DrawRecording (void);
		virtual void DrawCountdown (void);
		virtual void DrawCruise (void);
		virtual void DrawScore (void);
		virtual void DrawAddedScore (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawOrbs (void);
		virtual void DrawFlag (void);
		virtual void DrawEnergyText (void);
		virtual void DrawShieldText (void);
		virtual void DrawShieldBar (void);
		virtual void DrawEnergyBar (void);
		virtual void DrawAfterburnerText (void);
		virtual void DrawAfterburnerBar (void);
		virtual void DrawBombCount (void);
		virtual int32_t DrawBombCount (int32_t& nIdBombCount, int32_t y, int32_t x, int32_t nColor, char* pszBombCount);
		virtual void DrawPrimaryAmmoInfo (int32_t ammoCount);
		virtual void DrawSecondaryAmmoInfo (int32_t ammoCount);
		virtual void DrawWeaponInfo (int32_t nWeaponType, int32_t nWeaponId, int32_t laserLevel);
		virtual void DrawCloak (void) {}
		virtual void DrawInvul (void);
		virtual void DrawLives (void);
		virtual void DrawPlayerShip (void);
		virtual void DrawStatic (int32_t nWindow);
		virtual void DrawKillList (void);
		virtual void ClearBombCount (int32_t bgColor);
		virtual void DrawCockpit (bool bAlphaTest = false);
		virtual void Toggle (void);

		virtual void SetupWindow (int32_t nWindow);
		virtual bool Setup (bool bScene, bool bRebuild);
	};

//	-----------------------------------------------------------------------------

class CCockpit : public CGenericCockpit {
	public:
		virtual void GetHostageWindowCoords (int32_t& x, int32_t& y, int32_t& w, int32_t& h);
		virtual void DrawRecording (void);
		virtual void DrawCountdown (void);
		virtual void DrawCruise (void);
		virtual void DrawScore (void);
		virtual void DrawAddedScore (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawOrbs (void);
		virtual void DrawFlag (void);
		virtual void DrawEnergyText (void);
		virtual void DrawEnergyBar (void);
		virtual void DrawAfterburnerText (void) {}
		virtual void DrawAfterburnerBar (void);
		virtual void DrawBombCount (void);
		virtual int32_t DrawBombCount (int32_t& nIdBombCount, int32_t y, int32_t x, int32_t nColor, char* pszBombCount);
		virtual void DrawPrimaryAmmoInfo (int32_t ammoCount);
		virtual void DrawSecondaryAmmoInfo (int32_t ammoCount);
		virtual void DrawWeaponInfo (int32_t nWeaponType, int32_t nWeaponId, int32_t laserLevel);
		virtual void DrawCloak (void) {}
		virtual void DrawInvul (void);
		virtual void DrawShieldText (void);
		virtual void DrawShieldBar (void);
		virtual void DrawLives (void);
		virtual void DrawPlayerShip (void);
		virtual void DrawStatic (int32_t nWindow);
		virtual void DrawKillList (void);
		virtual void ClearBombCount (int32_t bgColor);
		virtual void DrawCockpit (bool bAlphaTest = false);
		virtual void Toggle (void);

		virtual void SetupWindow (int32_t nWindow);
		virtual bool Setup (bool bScene, bool bRebuild);
	};

//	-----------------------------------------------------------------------------

class CRearView : public CGenericCockpit {
	public:
		virtual void GetHostageWindowCoords (int32_t& x, int32_t& y, int32_t& w, int32_t& h) { x = y = w = h = -1; }
		virtual void DrawRecording (void) {}
		virtual void DrawCountdown (void) {}
		virtual void DrawCruise (void) {}
		virtual void DrawScore (void) {}
		virtual void DrawAddedScore (void) {}
		virtual void DrawHomingWarning (void) {}
		virtual void DrawKeys (void) {}
		virtual void DrawOrbs (void) {}
		virtual void DrawFlag (void) {}
		virtual void DrawEnergyText (void) {}
		virtual void DrawEnergyBar (void) {}
		virtual void DrawAfterburnerText (void) {}
		virtual void DrawAfterburnerBar (void) {}
		virtual void DrawBombCount (void) {}
		virtual int32_t DrawBombCount (int32_t& nIdBombCount, int32_t y, int32_t x, int32_t nColor, char* pszBombCount) { return -1; }
		virtual void DrawPrimaryAmmoInfo (int32_t ammoCount) {}
		virtual void DrawSecondaryAmmoInfo (int32_t ammoCount) {}
		virtual void DrawWeaponInfo (int32_t nWeaponType, int32_t nWeaponId, int32_t laserLevel) {}
		virtual void DrawWeapons (void) {}
		virtual void DrawCloak (void) {}
		virtual void DrawInvul (void) {}
		virtual void DrawShieldText (void) {}
		virtual void DrawShieldBar (void) {}
		virtual void DrawLives (void) {}
		virtual void DrawPlayerShip (void) {}
		virtual void DrawCockpit (void) {}
		virtual void DrawStatic (int32_t nWindow) {}
		virtual void DrawKillList (void) {}
		virtual void ClearBombCount (int32_t bgColor) {}
		virtual void DrawCockpit (bool bAlphaTest = false) { 
			CGenericCockpit::DrawCockpit (CM_REAR_VIEW + m_info.nCockpit, 0, bAlphaTest); 
			}
		virtual void Toggle (void) {};
		virtual void SetupWindow (int32_t nWindow) {}
		virtual bool Setup (bool bScene, bool bRebuild);
	};

//	-----------------------------------------------------------------------------

extern CRGBColor playerColors [];

extern CHUD			hudCockpit;
extern CWideHUD	letterboxCockpit;
extern CCockpit	fullCockpit;
extern CStatusBar	statusBarCockpit;
extern CRearView	rearViewCockpit;

extern CGenericCockpit* cockpit;

//	-----------------------------------------------------------------------------

#endif // _COCKPIT_H
