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
#include "hudmsg.h"
#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
#include "ogl_render.h"
//from gauges.c

// Flags for gauges/hud stuff
extern ubyte Reticle_on;

// Call to flash a message on the HUD.  Returns true if message drawn.
// (message might not be drawn if previous message was same)
#define gauge_message HUDInitMessage

//valid modes for cockpit
#define CM_FULL_COCKPIT    0   // normal screen with cockput
#define CM_REAR_VIEW       1   // looking back with bitmap
#define CM_STATUS_BAR      2   // small status bar, w/ reticle
#define CM_FULL_SCREEN     3   // full screen, no cockpit (w/ reticle)
#define CM_LETTERBOX       4   // half-height window (for cutscenes)

#define WBU_WEAPON			0       // the weapons display
#define WBUMSL					1       // the missile view
#define WBU_ESCORT			2       // the "buddy bot"
#define WBU_REAR				3       // the rear view
#define WBU_COOP				4       // coop or team member view
#define WBU_GUIDED			5       // the guided missile
#define WBU_MARKER			6       // a dropped marker
#define WBU_STATIC			7       // playing static after missile hits
#define WBU_RADAR_TOPDOWN	8
#define WBU_RADAR_HEADSUP	9

//	-----------------------------------------------------------------------------

#define HUD_SCALE(v, s)	(int (float (v) * (s) /*+ 0.5*/))
#define HUD_SCALE_X(v)	HUD_SCALE (v, m_info.xScale)
#define HUD_SCALE_Y(v)	HUD_SCALE (v, m_info.yScale)
#define HUD_LHX(x)      (gameStates.menus.bHires ? 2 * (x) : x)
#define HUD_LHY(y)      (gameStates.menus.bHires? (24 * (y)) / 10 : y)
#define HUD_ASPECT		((double) screen.Height () / (double) screen.Width () / 0.75)

//	-----------------------------------------------------------------------------

typedef struct tSpan {
	sbyte l, r;
} tSpan;

typedef struct tGaugeBox {
	int left, top;
	int right, bot;		//maximal box
	tSpan *spanlist;	//list of left, right spans for copy
} tGaugeBox;

//	-----------------------------------------------------------------------------

class CCockpitHistory {
	public:
		int	score;
		int	energy;
		int	shields;
		int	flags;
		int	bCloak;
		int	lives;
		fix	afterburner;
		int	bombCount;
		int	laserLevel;
		int	weapon [2];
		int	ammo [2];
		fix	xOmegaCharge;

	public:
		void Init (void);
};

class CCockpitInfo {
	public:
		int	nCloakFadeState;
		int	bLastHomingWarningDrawn [2];
		int	scoreDisplay [2];
		fix	scoreTime;
		fix	lastWarningBeepTime [2];
		int	bHaveGaugeCanvases;
		int	nInvulnerableFrame;
		int	weaponBoxStates [2];
		fix	weaponBoxFadeValues [2];
		int	weaponBoxUser [2];
		int	nLineSpacing;
		int	nType;
		float	xScale;
		float	yScale;
		float	xGaugeScale;
		float	yGaugeScale;
		int	fontWidth;
		int	fontHeight;
		int	nCockpit;
		int	nShields;
		int	nEnergy;
		int	bCloak;
		fix	tInvul;

	public:
		CCockpitInfo () { Init (); }
		void Init (void);
};

//	-----------------------------------------------------------------------------

class CGenericCockpit {
	protected:
		CCockpitHistory	m_history [2];
		CCockpitInfo		m_info;

	public:
		void Init (void);

		CBitmap* BitBlt (int nGauge, int x, int y, bool bScalePos = true, bool bScaleSize = true, int scale = I2X (1), int orient = 0, CBitmap* bmP = NULL);
		int _CDECL_ PrintF (int *idP, int x, int y, const char *pszFmt, ...);
		void Rect (int left, int top, int width, int height) {
			OglDrawFilledRect (HUD_SCALE_X (left), HUD_SCALE_Y (top), HUD_SCALE_X (width), HUD_SCALE_Y (height));
			}

		char* ftoa (char *pszVal, fix f);
		char* Convert1s (char* s);
		void DemoRecording (void);

		void DrawMarkerMessage (void);
		void DrawMultiMessage (void);
		void DrawCountdown (int y);
		void DrawRecording (int y);
		void DrawFrameRate (void);
		void DrawPlayerStats (void);
		void DrawSlowMotion (void);
		void DrawTime (void);
		void DrawTimerCount (void);
		void PlayHomingWarning (void);
		void DrawCruise (int x, int y);
		void DrawBombCount (int x, int y, int bgColor, int bShowAlways);
		void DrawAmmoInfo (int x, int y, int ammoCount, int bPrimary);
		void CheckForExtraLife (int nPrevScore);
		void AddPointsToScore (int points);
		void AddBonusPointsToScore (int points);
		void DrawPlayerShip (int nCloakState, int nOldCloakState, int x, int y);
		void DrawWeaponInfo (int info_index, tGaugeBox *box, int xPic, int yPic, const char *pszName, int xText, int yText, int orient);
		int DrawWeaponDisplay (int nWeaponType, int nWeaponId);
		void DrawStatic (int nWindow, int nIndex);
		void DrawOrbs (int x, int y);
		void DrawFlag (int x, int y);
		void DrawKillList (int x, int y);
		void DrawCockpit (int nCockpit, int y, bool bAlphaTest = false);
		void UpdateLaserWeaponInfo (void);
		void DrawReticle (int bForceBig);
		int CanSeeObject (int nObject, int bCheckObjs);
		void DrawPlayerNames (void);
		void Render (int bExtraInfo);
		void RenderWindow (int nWindow, CObject *viewerP, int bRearView, int nUser, const char *pszLabel);
		void Activate (int nType);

		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h) = 0;
		virtual void DrawRecording (void) = 0;
		virtual void DrawCountdown (void) = 0;
		virtual void DrawCruise (void) = 0;
		virtual void DrawScore (void) = 0;
		virtual void DrawScoreAdded (void) = 0;
		virtual void DrawHomingWarning (void) = 0;
		virtual void DrawKeys (void) = 0;
		virtual void DrawOrbs (void) = 0;
		virtual void DrawFlag (void) = 0;
		virtual void DrawShield (void) = 0;
		virtual void DrawShieldBar (void) = 0;
		virtual void DrawEnergy (void) = 0;
		virtual void DrawEnergyBar (void) = 0;
		virtual void DrawAfterburner (void) = 0;
		virtual void DrawAfterburnerBar (void) = 0;
		virtual void ClearBombCount (int bgColor) = 0;
		virtual void DrawBombCount (void) = 0;
		virtual int DrawBombCount (int& nIdBombCount, int y, int x, char* pszBombCount) = 0;
		virtual void DrawPrimaryAmmoInfo (int ammoCount) = 0;
		virtual void DrawSecondaryAmmoInfo (int ammoCount) = 0;
		virtual void DrawCloak (void) = 0;
		virtual void DrawInvul (void) = 0;
		virtual void DrawLives (void) = 0;
		virtual void DrawPlayerShip (void) = 0;
		virtual void DrawKillList (void) = 0;
		virtual void DrawStatic (int nWindow) = 0;
		virtual void Toggle (void);
		virtual void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel);
		virtual void DrawWeapons (void);
		virtual void DrawCockpit (bool bAlphaTest = false);
		virtual void SetupWindow (int nWindow, CCanvas* canvP) = 0;
		virtual bool Setup (void);

		inline CCockpitInfo& Info (void) { return m_info; }
		inline int Type (void) { return m_info.nType; }
		inline int LineSpacing (void) { return m_info.nLineSpacing = GAME_FONT->Height () + GAME_FONT->Height () / 4; }
		inline float XScale (void) { return m_info.xScale = screen.Scale (0); }
		inline float YScale (void) { return m_info.yScale = screen.Scale (1); }

		inline bool Always (void) { 
			return (m_info.nType == CM_FULL_COCKPIT) || (m_info.nType == CM_STATUS_BAR); 
			}

		inline bool Show (void) { 
			return !gameStates.app.bEndLevelSequence && (!gameStates.app.bNostalgia || gameOpts->render.cockpit.bHUD || !Always ()); 
			}
		
		inline bool Hide (void) {
			return gameStates.app.bEndLevelSequence || 
					 (!(gameStates.app.bNostalgia || gameOpts->render.cockpit.bHUD) && (m_info.nType >= CM_FULL_SCREEN));
			}
	};

//	-----------------------------------------------------------------------------

class CHUD : public CGenericCockpit {
	public:
		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h);
		virtual void DrawRecording (void);
		virtual void DrawCountdown (void);
		virtual void DrawCruise (void);
		virtual void DrawScore (void);
		virtual void DrawScoreAdded (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawOrbs (void);
		virtual void DrawFlag (void);
		virtual void DrawShield (void);
		virtual void DrawShieldBar (void);
		virtual void DrawEnergy (void);
		virtual void DrawEnergyBar (void);
		virtual void DrawAfterburner (void);
		virtual void DrawAfterburnerBar (void);
		virtual void ClearBombCount (int bgColor);
		virtual void DrawBombCount (void);
		virtual int DrawBombCount (int& nIdBombCount, int y, int x, char* pszBombCount);
		virtual void DrawPrimaryAmmoInfo (int ammoCount);
		virtual void DrawSecondaryAmmoInfo (int ammoCount);
		virtual void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel) {}
		virtual void DrawWeapons (void);
		virtual void DrawCloak (void);
		virtual void DrawInvul (void);
		virtual void DrawLives (void);
		virtual void DrawPlayerShip (void) {}
		virtual void DrawCockpit (void);

		virtual void DrawStatic (int nWindow);
		virtual void DrawKillList (void);
		virtual void DrawCockpit (bool bAlphaTest = false);
		virtual void Toggle (void);

		virtual void SetupWindow (int nWindow, CCanvas* canvP);
		virtual bool Setup (void);

	private:
		int FlashGauge (int h, int *bFlash, int tToggle);
	};

//	-----------------------------------------------------------------------------

class CWideHUD : public CHUD {
	public:
		virtual void DrawRecording (void);
		virtual void Toggle (void);
		virtual bool Setup (void);
	};

//	-----------------------------------------------------------------------------

class CStatusBar : public CGenericCockpit {
	public:
		CBitmap* StretchBlt (int nGauge, int x, int y, double xScale, double yScale, int scale = I2X (1), int orient = 0);

		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h);
		virtual void DrawRecording (void);
		virtual void DrawCountdown (void);
		virtual void DrawCruise (void);
		virtual void DrawScore (void);
		virtual void DrawScoreAdded (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawOrbs (void);
		virtual void DrawFlag (void);
		virtual void DrawEnergy (void);
		virtual void DrawShield (void);
		virtual void DrawShieldBar (void);
		virtual void DrawEnergyBar (void);
		virtual void DrawAfterburner (void);
		virtual void DrawAfterburnerBar (void);
		virtual void DrawBombCount (void);
		virtual int DrawBombCount (int& nIdBombCount, int y, int x, char* pszBombCount);
		virtual void DrawPrimaryAmmoInfo (int ammoCount);
		virtual void DrawSecondaryAmmoInfo (int ammoCount);
		virtual void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel);
		virtual void DrawWeapons (void);
		virtual void DrawCloak (void) {}
		virtual void DrawInvul (void);
		virtual void DrawLives (void);
		virtual void DrawPlayerShip (void);

		virtual void DrawStatic (int nWindow);
		virtual void DrawKillList (void);
		virtual void ClearBombCount (int bgColor);
		virtual void DrawCockpit (bool bAlphaTest = false);
		virtual void Toggle (void);

		virtual void SetupWindow (int nWindow, CCanvas* canvP);
		virtual bool Setup (void);
	};

//	-----------------------------------------------------------------------------

class CCockpit : public CGenericCockpit {
	public:
		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h);
		virtual void DrawRecording (void);
		virtual void DrawCountdown (void);
		virtual void DrawCruise (void);
		virtual void DrawScore (void);
		virtual void DrawScoreAdded (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawOrbs (void);
		virtual void DrawFlag (void);
		virtual void DrawEnergy (void);
		virtual void DrawEnergyBar (void);
		virtual void DrawAfterburner (void);
		virtual void DrawAfterburnerBar (void);
		virtual void DrawBombCount (void);
		virtual int DrawBombCount (int& nIdBombCount, int y, int x, char* pszBombCount);
		virtual void DrawPrimaryAmmoInfo (int ammoCount);
		virtual void DrawSecondaryAmmoInfo (int ammoCount);
		virtual void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel);
		virtual void DrawWeapons (void);
		virtual void DrawCloak (void) {}
		virtual void DrawInvul (void);
		virtual void DrawShield (void);
		virtual void DrawShieldBar (void);
		virtual void DrawLives (void);
		virtual void DrawPlayerShip (void);

		virtual void DrawStatic (int nWindow);
		virtual void DrawKillList (void);
		virtual void ClearBombCount (int bgColor);
		virtual void DrawCockpit (bool bAlphaTest = false);
		virtual void Toggle (void);

		virtual void SetupWindow (int nWindow, CCanvas* canvP);
		virtual bool Setup (void);
	};

//	-----------------------------------------------------------------------------

class CRearView : public CGenericCockpit {
	public:
		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h) { x = y = w = h = -1; }
		virtual void DrawRecording (void) {}
		virtual void DrawCountdown (void) {}
		virtual void DrawCruise (void) {}
		virtual void DrawScore (void) {}
		virtual void DrawScoreAdded (void) {}
		virtual void DrawHomingWarning (void) {}
		virtual void DrawKeys (void) {}
		virtual void DrawOrbs (void) {}
		virtual void DrawFlag (void) {}
		virtual void DrawEnergy (void) {}
		virtual void DrawEnergyBar (void) {}
		virtual void DrawAfterburner (void) {}
		virtual void DrawAfterburnerBar (void) {}
		virtual void DrawBombCount (void) {}
		virtual int DrawBombCount (int& nIdBombCount, int y, int x, char* pszBombCount) { return -1; }
		virtual void DrawPrimaryAmmoInfo (int ammoCount) {}
		virtual void DrawSecondaryAmmoInfo (int ammoCount) {}
		virtual void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel) {}
		virtual void DrawWeapons (void) {}
		virtual void DrawCloak (void) {}
		virtual void DrawInvul (void) {}
		virtual void DrawShield (void) {}
		virtual void DrawShieldBar (void) {}
		virtual void DrawLives (void) {}
		virtual void DrawPlayerShip (void) {}
		virtual void DrawCockpit (void) {}

		virtual void DrawStatic (int nWindow) {}
		virtual void DrawKillList (void) {}
		virtual void ClearBombCount (int bgColor) {}
		virtual void DrawCockpit (bool bAlphaTest = false) { 
			CGenericCockpit::DrawCockpit (CM_REAR_VIEW + m_info.nCockpit, 0, bAlphaTest); 
			}
		virtual void Toggle (void) {};
		virtual void SetupWindow (int nWindow, CCanvas* canvP) {}
		virtual bool Setup (void);
	};

//	-----------------------------------------------------------------------------

void FreeInventoryIcons (void);
void FreeObjTallyIcons (void);
void HUDShowIcons (void);

extern tRgbColorb playerColors [];

extern CHUD			hudCockpit;
extern CWideHUD	letterboxCockpit;
extern CCockpit	fullCockpit;
extern CStatusBar	statusBarCockpit;
extern CRearView	rearViewCockpit;

extern CGenericCockpit* cockpit;

//	-----------------------------------------------------------------------------

#endif // _COCKPIT_H
