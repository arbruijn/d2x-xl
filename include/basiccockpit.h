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

#include "descent.h"
#include "gr.h"
#include "piggy.h"
#include "object.h"
#include "hudmsg.h"
#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
//from gauges.c

// Flags for gauges/hud stuff
extern ubyte Reticle_on;

void InitGaugeCanvases();
void CloseGaugeCanvases();

void ShowScore ();
void ShowScoreAdded ();
void AddPointsToScore (int);
void AddBonusPointsToScore (int);

void RenderGauges(void);
void InitGauges(void);
void check_erase_message(void);

void HUDRenderMessageFrame();
void HUDClearMessages();

// Call to flash a message on the HUD.  Returns true if message drawn.
// (message might not be drawn if previous message was same)
#define gauge_message HUDInitMessage

void DrawHUD();     // draw all the HUD stuff

void PlayerDeadMessage(void);
//void say_afterburner_status(void);

// fills in the coords of the hostage video window
void get_hostage_window_coords(int& x, int& y, int& w, int& h);

// from testgaug.c

void gauge_frame(void);
void UpdateLaserWeaponInfo(void);
void PlayHomingWarning(void);

extern tRgbColorb playerColors [];

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

// draws a 3d view into one of the cockpit windows.  win is 0 for
// left, 1 for right.  viewer is CObject.  NULL CObject means give up
// window user is one of the WBU_ constants.  If rearViewFlag is
// set, show a rear view.  If label is non-NULL, print the label at
// the top of the window.
void HUDRenderWindow (int win, CObject* viewerP, int bRearView, int nUser, const char *pszLabel);
void FreeInventoryIcons (void);
void FreeObjTallyIcons (void);
void HUDShowIcons (void);
int CanSeeObject(int nObject, int bCheckObjs);
void ShowFrameRate (void);
void ToggleCockpit ();

#define SHOW_COCKPIT	((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nMode == CM_STATUS_BAR))
#define SHOW_HUD		(!gameStates.app.bEndLevelSequence && (!gameStates.app.bNostalgia || gameOpts->render.cockpit.bHUD || !SHOW_COCKPIT))
#define HIDE_HUD		(gameStates.app.bEndLevelSequence || (!(gameStates.app.bNostalgia || gameOpts->render.cockpit.bHUD) && (gameStates.render.cockpit.nMode >= CM_FULL_SCREEN)))

extern double cmScaleX, cmScaleY;
extern int nHUDLineSpacing;

//	-----------------------------------------------------------------------------

#define HUD_SCALE(v, s)	((int) (double (v) * (s) /*+ 0.5*/))
#define HUD_SCALE_X(v)	HUD_SCALE (v, cmScaleX)
#define HUD_SCALE_Y(v)	HUD_SCALE (v, cmScaleY)
#define HUD_LHX(x)      (gameStates.menus.bHires ? 2 * (x) : x)
#define HUD_LHY(y)      (gameStates.menus.bHires? (24 * (y)) / 10 : y)
#define HUD_ASPECT		((double) screen.Height () / (double) screen.Width () / 0.75)

//	-----------------------------------------------------------------------------

CBitmap* HUDBitBlt (int nGauge, int x, int y, bool bScalePos = true, bool bScaleSize = true, int scale = I2X (1), int orient = 0, CBitmap* bmP = NULL);

//	-----------------------------------------------------------------------------

typedef struct tOldCockpitInfo {
	int	score;
	int	energy;
	int	shields;
	uint	flags;
	int	weapon [2];
	int	ammo [2];
	int	oldOmegaCharge;
	int	laserLevel;
	int	cloak;
	int	lives;
	fix	afterburner;
	int	bombCount;
} tOldCockpitInfo;

class CCockpitInfo {
	public:
		tOldCockpitInfo	old [2];

		int				bLastHomingWarningShown [2];
		int				scoreDisplay [2];
		fix				scoreTime;
		fix				lastWarningBeepTime [2] = {0, 0};		
		int				bHaveGaugeCanvases = 0;
		int				nInvulnerableFrame = 0;
		int				nCloakFadeState;		
		int				weaponBoxUser [2] = {WBU_WEAPON, WBU_WEAPON};		
		int				weaponBoxStates [2];
		fix				weaponBoxFadeValues [2];
		int				nLineSpacing;
		float				xScale;
		float				yScale;
		float				xGaugeScale;
		float				yGaugeScale;

		int				nEnergy;
		int				nShields;
		int				bCloak;


	public:
		CCockpitInfo () { Init (); }
		void Init (void);
	};

class CGenericCockpit {
	private:
		CCockpitInfo	m_info;

	public:
		void Init (void);

		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h) = 0;
		virtual void DrawTimerCount (void) = 0;
		virtual void DrawScore (void) = 0;
		virtual void DrawSlowMotion (void) = 0;
		virtual void DrawScoreAdded (void) = 0;
		virtual void DrawHomingWarning (void) = 0;
		virtual void DrawKeys (void) = 0;
		virtual void DrawOrbs (void) = 0;
		virtual void DrawFlag (void) = 0;
		virtual void DrawFlashGauge (void) = 0;
		virtual void DrawEnergy (int nEnergy) = 0;
		virtual void DrawEnergyBar (int nEnergy) = 0;
		virtual void DrawShield (int shield) = 0;
		virtual void DrawShieldBar (int shield) = 0;
		virtual void DrawAfterburner (int nEnergy) = 0;
		virtual void DrawAfterburnerBar (int nEnergy) = 0;
		virtual void DrawWeapons (void) = 0;
		virtual void DrawCloakInvul (void) = 0;
		virtual void DrawShield (void) = 0;
		virtual void DrawLives (void) = 0;
		virtual void DrawTime (void) = 0;
		virtual void DrawWeaponInfo (int nInfoIndex, tGaugeBox *box, int xPic, int yPic, const char *pszName, int xText, int yText, int orient) = 0;
		virtual void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel) = 0;
		virtual void DrawAmmoInfo (int x, int y, int ammoCount, int bPrimary) = 0;
		virtual void DrawSecondaryAmmoInfo (int ammoCount) = 0;
		virtual void ClearBombCount (void) = 0;
		virtual int DrawBombCount (int* nId, int x, int y, char* pszBombCount) = 0;
		virtual void SetupWindow (int nWindow) = 0;

		int DrawWeaponDisplay (int nWeaponType, int nWeaponId);

		void DrawBombCount (int x, int y, int bgColor, int bDrawAlways);
		void DrawWeaponDisplays (void);
		void DrawStatic (int nWindow);

		virtual void DrawPlayerShip (int nCloakState, int nOldCloakState, int x, int y);
		virtual void DrawInvulnerableShip (void);
		void CGenericCockpit::DrawTimerCount (void)
		void DrawNumericalDisplay (int shield, int energy);
		void DrawKeys (void);

		void Show (void);

	protected:
		void DrawPrimaryAmmoInfo (int ammoCount);
		CBitmap* BitBlt (int nGauge, int x, int y, bool bScalePos, bool bScaleSize, int scale, int orient, CBitmap* bmP);
		int _CDECL_ Print (int& idP, int x, int y, const char *pszFmt, ...);
		char* ftoa (char *pszVal, fix f);

	public:
		void PlayHomingWarning (void);
		int FlashGauge (int h, int& bFlash, int tToggle);
		void CheckAndExtraLife (int nPrevScore);
		void AddPointsToScore (int points);
		void AddBonusPointsToScore (int points);
	}; 

//	-----------------------------------------------------------------------------

class CCockpit : public CGenericCockpit {
	public:
		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h);
		virtual void DrawScore (void);
		virtual void DrawSlowMotion (void);
		virtual void DrawScoreAdded (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawPrimaryAmmoInfo (int ammoCount);
		virtual void DrawSecondaryAmmoInfo (int ammoCount);
		virtual void DrawLives (void);
		virtual void DrawEnergy (int nEnergy);
		virtual void DrawEnergyBar (int nEnergy);
		virtual void DrawShield (int shield);
		virtual void DrawShieldBar (int shield);
		virtual void DrawAfterburner (int nAfterburner);
		virtual void DrawAfterburnerBar (int nAfterburner);
		virtual void ClearBombCount (void);
		virtual int DrawBombCount (int* nId, int x, int y, char* pszBombCount);
		virtual void DrawInvulnerableShip (void);
	};

//	-----------------------------------------------------------------------------

class CStatusBar : public CGenericCockpit {
	public:
		CBitmap* StretchBlt (int nGauge, int x, int y, double xScale, double yScale, 
								   int scale = I2X (1), int orient = 0, CBitmap* bmP = NULL);

		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h);
		virtual void DrawScore (void);
		virtual void DrawSlowMotion (void);
		virtual void DrawScoreAdded (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawPrimaryAmmoInfo (int ammoCount);
		virtual void DrawSecondaryAmmoInfo (int ammoCount);
		virtual void DrawLives (void);
		virtual void DrawEnergy (int nEnergy);
		virtual void DrawEnergyBar (int nEnergy);
		virtual void DrawShield (int shield);
		virtual void DrawShieldBar (int shield);
		virtual void DrawAfterburner (int nAfterburner);
		virtual void DrawAfterburnerBar (int nAfterburner);
		virtual void ClearBombCount (void);
		virtual int DrawBombCount (int* nId, int x, int y, char* pszBombCount);
		virtual void DrawInvulnerableShip (void);
	};

//	-----------------------------------------------------------------------------

#endif /* _COCKPIT_H */
