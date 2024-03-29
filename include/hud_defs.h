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

#ifndef _HUD_DEFS_H
#define _HUD_DEFS_H

#include "cockpit.h"

#define GAUGE_SHIELDS			0		//0..9, in decreasing order (100%, 90%...0%)

#define GAUGE_INVULNERABLE		10		//10..19
#define N_INVULNERABLE_FRAMES	10

#define GAUGE_AFTERBURNER   	20
#define GAUGE_ENERGY_LEFT		21
#define GAUGE_ENERGY_RIGHT		22
#define GAUGE_NUMERICAL			23

#define GAUGE_BLUE_KEY			24
#define GAUGE_GOLD_KEY			25
#define GAUGE_RED_KEY			26
#define GAUGE_BLUE_KEY_OFF		27
#define GAUGE_GOLD_KEY_OFF		28
#define GAUGE_RED_KEY_OFF		29

#define SB_GAUGE_BLUE_KEY		30
#define SB_GAUGE_GOLD_KEY		31
#define SB_GAUGE_RED_KEY		32
#define SB_GAUGE_BLUE_KEY_OFF	33
#define SB_GAUGE_GOLD_KEY_OFF	34
#define SB_GAUGE_RED_KEY_OFF	35

#define SB_GAUGE_ENERGY			36

#define GAUGE_LIVES				37

#define GAUGE_SHIPS				38
#define GAUGE_SHIPS_LAST		45

#define RETICLE_CROSS			46
#define RETICLE_PRIMARY			48
#define RETICLE_SECONDARY		51
#define RETICLE_LAST				55

#define GAUGE_HOMING_WARNING_ON	56
#define GAUGE_HOMING_WARNING_OFF	57

#define SML_RETICLE_CROSS		58
#define SML_RETICLE_PRIMARY	60
#define SML_RETICLE_SECONDARY	63
#define SML_RETICLE_LAST		67

#define KEY_ICON_BLUE			68
#define KEY_ICON_YELLOW			69
#define KEY_ICON_RED				70

#define SB_GAUGE_AFTERBURNER	71

//#define GAME_HIRES (gameStates.video.nDisplayMode != 0)
#define GAME_HIRES 1

#define FLAG_ICON_RED			72
#define FLAG_ICON_BLUE			73

#define GAUGE_BLUE_KEY_X_L		272
#define GAUGE_BLUE_KEY_Y_L		152
#define GAUGE_BLUE_KEY_X_H		535
#define GAUGE_BLUE_KEY_Y_H		374
#define GAUGE_BLUE_KEY_X		 (GAME_HIRES ? GAUGE_BLUE_KEY_X_H : GAUGE_BLUE_KEY_X_L)
#define GAUGE_BLUE_KEY_Y		 (GAME_HIRES ? GAUGE_BLUE_KEY_Y_H : GAUGE_BLUE_KEY_Y_L)

#define GAUGE_GOLD_KEY_X_L		273
#define GAUGE_GOLD_KEY_Y_L		162
#define GAUGE_GOLD_KEY_X_H		537
#define GAUGE_GOLD_KEY_Y_H		395
#define GAUGE_GOLD_KEY_X		 (GAME_HIRES ? GAUGE_GOLD_KEY_X_H : GAUGE_GOLD_KEY_X_L)
#define GAUGE_GOLD_KEY_Y		 (GAME_HIRES ? GAUGE_GOLD_KEY_Y_H : GAUGE_GOLD_KEY_Y_L)

#define GAUGE_RED_KEY_X_L		274
#define GAUGE_RED_KEY_Y_L		172
#define GAUGE_RED_KEY_X_H		539
#define GAUGE_RED_KEY_Y_H		416
#define GAUGE_RED_KEY_X			 (GAME_HIRES ? GAUGE_RED_KEY_X_H : GAUGE_RED_KEY_X_L)
#define GAUGE_RED_KEY_Y			 (GAME_HIRES ? GAUGE_RED_KEY_Y_H : GAUGE_RED_KEY_Y_L)

// status bar keys

#define SB_GAUGE_KEYS_X_L	   11
#define SB_GAUGE_KEYS_X_H		26
#define SB_GAUGE_KEYS_X			 (GAME_HIRES ? SB_GAUGE_KEYS_X_H : SB_GAUGE_KEYS_X_L)

#define SB_GAUGE_BLUE_KEY_Y_L		153
#define SB_GAUGE_GOLD_KEY_Y_L		169
#define SB_GAUGE_RED_KEY_Y_L		185

#define SB_GAUGE_BLUE_KEY_Y_H		390
#define SB_GAUGE_GOLD_KEY_Y_H		422
#define SB_GAUGE_RED_KEY_Y_H		454

#define SB_GAUGE_BLUE_KEY_Y		(GAME_HIRES ? SB_GAUGE_BLUE_KEY_Y_H : SB_GAUGE_BLUE_KEY_Y_L)
#define SB_GAUGE_GOLD_KEY_Y		(GAME_HIRES ? SB_GAUGE_GOLD_KEY_Y_H : SB_GAUGE_GOLD_KEY_Y_L)
#define SB_GAUGE_RED_KEY_Y			(GAME_HIRES ? SB_GAUGE_RED_KEY_Y_H : SB_GAUGE_RED_KEY_Y_L)

// cockpit enery gauges

#define LEFT_ENERGY_GAUGE_X_L 	70
#define LEFT_ENERGY_GAUGE_Y_L 	131
#define LEFT_ENERGY_GAUGE_W_L 	64
#define LEFT_ENERGY_GAUGE_H_L 	8

#define LEFT_ENERGY_GAUGE_X_H 	137
#define LEFT_ENERGY_GAUGE_Y_H 	314
#define LEFT_ENERGY_GAUGE_W_H 	133
#define LEFT_ENERGY_GAUGE_H_H 	21

#define LEFT_ENERGY_GAUGE_X 		(GAME_HIRES ? LEFT_ENERGY_GAUGE_X_H : LEFT_ENERGY_GAUGE_X_L)
#define LEFT_ENERGY_GAUGE_Y 		(GAME_HIRES ? LEFT_ENERGY_GAUGE_Y_H : LEFT_ENERGY_GAUGE_Y_L)
#define LEFT_ENERGY_GAUGE_W 		(GAME_HIRES ? LEFT_ENERGY_GAUGE_W_H : LEFT_ENERGY_GAUGE_W_L)
#define LEFT_ENERGY_GAUGE_H 		(GAME_HIRES ? LEFT_ENERGY_GAUGE_H_H : LEFT_ENERGY_GAUGE_H_L)

#define RIGHT_ENERGY_GAUGE_X_L 	190
#define RIGHT_ENERGY_GAUGE_Y_L 	131
#define RIGHT_ENERGY_GAUGE_W_L 	64
#define RIGHT_ENERGY_GAUGE_H_L 	8

#define RIGHT_ENERGY_GAUGE_X_H 	380
#define RIGHT_ENERGY_GAUGE_Y_H 	314
#define RIGHT_ENERGY_GAUGE_W_H 	133
#define RIGHT_ENERGY_GAUGE_H_H 	21

#define RIGHT_ENERGY_GAUGE_X 		(GAME_HIRES ? RIGHT_ENERGY_GAUGE_X_H : RIGHT_ENERGY_GAUGE_X_L)
#define RIGHT_ENERGY_GAUGE_Y 		(GAME_HIRES ? RIGHT_ENERGY_GAUGE_Y_H : RIGHT_ENERGY_GAUGE_Y_L)
#define RIGHT_ENERGY_GAUGE_W 		(GAME_HIRES ? RIGHT_ENERGY_GAUGE_W_H : RIGHT_ENERGY_GAUGE_W_L)
#define RIGHT_ENERGY_GAUGE_H 		(GAME_HIRES ? RIGHT_ENERGY_GAUGE_H_H : RIGHT_ENERGY_GAUGE_H_L)

// cockpit afterburner gauge

#define AFTERBURNER_GAUGE_X_L	45-1
#define AFTERBURNER_GAUGE_Y_L	158
#define AFTERBURNER_GAUGE_W_L	12
#define AFTERBURNER_GAUGE_H_L	32

#define AFTERBURNER_GAUGE_X_H	88
#define AFTERBURNER_GAUGE_Y_H	377
#define AFTERBURNER_GAUGE_W_H	21
#define AFTERBURNER_GAUGE_H_H	65

#define AFTERBURNER_GAUGE_X		(GAME_HIRES ? AFTERBURNER_GAUGE_X_H : AFTERBURNER_GAUGE_X_L)
#define AFTERBURNER_GAUGE_Y		(GAME_HIRES ? AFTERBURNER_GAUGE_Y_H : AFTERBURNER_GAUGE_Y_L)
#define AFTERBURNER_GAUGE_W		(GAME_HIRES ? AFTERBURNER_GAUGE_W_H : AFTERBURNER_GAUGE_W_L)
#define AFTERBURNER_GAUGE_H		(GAME_HIRES ? AFTERBURNER_GAUGE_H_H : AFTERBURNER_GAUGE_H_L)

// sb afterburner gauge

#define SB_AFTERBURNER_GAUGE_X 	(GAME_HIRES ? 196 : 98)
#define SB_AFTERBURNER_GAUGE_Y 	(GAME_HIRES ? 446 : 184)
#define SB_AFTERBURNER_GAUGE_W 	(GAME_HIRES ? 33 : 16)
#define SB_AFTERBURNER_GAUGE_H 	(GAME_HIRES ? 29 : 13)

// sb energy gauge

#define SB_ENERGY_GAUGE_X 			(GAME_HIRES ? 196 : 98)
#define SB_ENERGY_GAUGE_Y 			(GAME_HIRES ? 382 :  (155-2))
#define SB_ENERGY_GAUGE_W 			(GAME_HIRES ? 32 : 16)
#define SB_ENERGY_GAUGE_H 			((GAME_HIRES ? 60 : 29) + (gameStates.app.bD1Mission  ?  SB_AFTERBURNER_GAUGE_H  :  0))

#define SB_ENERGY_NUM_X 			(SB_ENERGY_GAUGE_X+ (GAME_HIRES ? 4 : 2))
#define SB_ENERGY_NUM_Y 			(GAME_HIRES ? 457 : 175)

#define SHIELD_GAUGE_X 				(GAME_HIRES ? 292 : 146)
#define SHIELD_GAUGE_Y				(GAME_HIRES ? 374 : 155)
#define SHIELD_GAUGE_W 				(GAME_HIRES ? 70 : 35)
#define SHIELD_GAUGE_H				(GAME_HIRES ? 77 : 32)

#define SHIP_GAUGE_X 				(SHIELD_GAUGE_X + (GAME_HIRES ? 11 : 5))
#define SHIP_GAUGE_Y					(SHIELD_GAUGE_Y + (GAME_HIRES ? 10 : 5))

#define SB_SHIELD_GAUGE_X 			(GAME_HIRES ? 247 : 123)		//139
#define SB_SHIELD_GAUGE_Y 			(GAME_HIRES ? 395 : 163)

#define SB_SHIP_GAUGE_X 			(SB_SHIELD_GAUGE_X + (GAME_HIRES ? 11 : 5))
#define SB_SHIP_GAUGE_Y 			(SB_SHIELD_GAUGE_Y + (GAME_HIRES ? 10 : 5))

#define SB_SHIELD_NUM_X 			(SB_SHIELD_GAUGE_X + (GAME_HIRES ? 21 : 12))	//151
#define SB_SHIELD_NUM_Y 			(SB_SHIELD_GAUGE_Y - (GAME_HIRES ? 16 : 8))			//156 -- MWA used to be hard coded to 156
#define SB_SHIELD_NUM_W 			(2 * SB_AFTERBURNER_GAUGE_W / 2)

#define NUMERICAL_GAUGE_X			(GAME_HIRES ? 308 : 154)
#define NUMERICAL_GAUGE_Y			(GAME_HIRES ? 316 : 130)
#define NUMERICAL_GAUGE_W			(GAME_HIRES ? 38 : 19)
#define NUMERICAL_GAUGE_H			(GAME_HIRES ? 55 : 22)

#define PRIMARY_W_PIC_X				(GAME_HIRES ?  (135-10) : 64)
#define PRIMARY_W_PIC_Y				(GAME_HIRES ? 370 : 154)
#define PRIMARY_W_TEXT_X			(GAME_HIRES ? 182 : 87)
#define PRIMARY_W_TEXT_Y			(GAME_HIRES ? 400 : 157)
#define PRIMARY_AMMO_X				(GAME_HIRES ? 186 :  (96-3))
#define PRIMARY_AMMO_Y				(GAME_HIRES ? 420 : 171)

#define SECONDARY_W_PIC_X			(GAME_HIRES ? 466 : 234)
#define SECONDARY_W_PIC_Y			(GAME_HIRES ? 374 : 154)
#define SECONDARY_W_TEXT_X			(GAME_HIRES ? 413 : 207)
#define SECONDARY_W_TEXT_Y			(GAME_HIRES ? 378 : 157)
#define SECONDARY_AMMO_X			(GAME_HIRES ? 428 : 213)
#define SECONDARY_AMMO_Y			(GAME_HIRES ? 407 : 171)

#define SB_LIVES_X					(GAME_HIRES ?  (550-10-3) : 266)
#define SB_LIVES_Y					(GAME_HIRES ? 450-3 : 185)
#define SB_LIVES_LABEL_X			(GAME_HIRES ? 475 : 237)
#define SB_LIVES_LABEL_Y			(SB_LIVES_Y+1)

#define SB_SCORE_RIGHT_L			301
#define SB_SCORE_RIGHT_H			(605+8)
#define SB_SCORE_RIGHT				(GAME_HIRES ? SB_SCORE_RIGHT_H : SB_SCORE_RIGHT_L)

#define SB_SCORE_Y					(GAME_HIRES ? 398 : 158)
#define SB_SCORE_LABEL_X			(GAME_HIRES ? 475 : 237)

#define SB_SCORE_ADDED_RIGHT		(GAME_HIRES ? SB_SCORE_RIGHT_H : SB_SCORE_RIGHT_L)
#define SB_SCORE_ADDED_Y			(GAME_HIRES ? 413 : 165)

#define HOMING_WARNING_X			(GAME_HIRES ? 14 : 7)
#define HOMING_WARNING_Y			(GAME_HIRES ? 415 : 171)

#define BOMB_COUNT_X					(GAME_HIRES ? 546 : 275)
#define BOMB_COUNT_Y					(GAME_HIRES ? 445 : 186)

#define SB_BOMB_COUNT_X				(GAME_HIRES ? 342 : 171)
#define SB_BOMB_COUNT_Y				(GAME_HIRES ? 458 : 191)

#define N_LEFT_WINDOW_SPANS		sizeofa (weaponWindowLeft)
#define N_RIGHT_WINDOW_SPANS		sizeofa (weaponWindowRight)

#define N_LEFT_WINDOW_SPANS_H		sizeofa (weaponWindowLeftHires)
#define N_RIGHT_WINDOW_SPANS_H	sizeofa (weaponWindowRightHires)

// defining box boundries for weapon pictures

#define PRIMARY_W_BOX_LEFT_L			63
#define PRIMARY_W_BOX_TOP_L			151		//154
#define PRIMARY_W_BOX_RIGHT_L			(PRIMARY_W_BOX_LEFT_L+58)
#define PRIMARY_W_BOX_BOT_L			(PRIMARY_W_BOX_TOP_L+N_LEFT_WINDOW_SPANS-1)
										
#define PRIMARY_W_BOX_LEFT_H			121
#define PRIMARY_W_BOX_TOP_H			364
#define PRIMARY_W_BOX_RIGHT_H			242

#define PRIMARY_W_BOX_BOT_H			(PRIMARY_W_BOX_TOP_H+N_LEFT_WINDOW_SPANS_H-1)		//470
										
#define PRIMARY_W_BOX_LEFT				(GAME_HIRES ? PRIMARY_W_BOX_LEFT_H : PRIMARY_W_BOX_LEFT_L)
#define PRIMARY_W_BOX_TOP				(GAME_HIRES ? PRIMARY_W_BOX_TOP_H : PRIMARY_W_BOX_TOP_L)
#define PRIMARY_W_BOX_RIGHT			(GAME_HIRES ? PRIMARY_W_BOX_RIGHT_H : PRIMARY_W_BOX_RIGHT_L)
#define PRIMARY_W_BOX_BOT				(GAME_HIRES ? PRIMARY_W_BOX_BOT_H : PRIMARY_W_BOX_BOT_L)

#define SECONDARY_W_BOX_LEFT_L		202	//207
#define SECONDARY_W_BOX_TOP_L			151
#define SECONDARY_W_BOX_RIGHT_L		263	// (SECONDARY_W_BOX_LEFT+54)
#define SECONDARY_W_BOX_BOT_L			(SECONDARY_W_BOX_TOP_L+N_RIGHT_WINDOW_SPANS-1)

#define SECONDARY_W_BOX_LEFT_H		404
#define SECONDARY_W_BOX_TOP_H			363
#define SECONDARY_W_BOX_RIGHT_H		529
#define SECONDARY_W_BOX_BOT_H			(SECONDARY_W_BOX_TOP_H+N_RIGHT_WINDOW_SPANS_H-1)		//470

#define SECONDARY_W_BOX_LEFT			(GAME_HIRES ? SECONDARY_W_BOX_LEFT_H : SECONDARY_W_BOX_LEFT_L)
#define SECONDARY_W_BOX_TOP			(GAME_HIRES ? SECONDARY_W_BOX_TOP_H : SECONDARY_W_BOX_TOP_L)
#define SECONDARY_W_BOX_RIGHT			(GAME_HIRES ? SECONDARY_W_BOX_RIGHT_H : SECONDARY_W_BOX_RIGHT_L)
#define SECONDARY_W_BOX_BOT			(GAME_HIRES ? SECONDARY_W_BOX_BOT_H : SECONDARY_W_BOX_BOT_L)

#define SB_PRIMARY_W_BOX_LEFT_L		34		//50
#define SB_PRIMARY_W_BOX_TOP_L		153
#define SB_PRIMARY_W_BOX_RIGHT_L		(SB_PRIMARY_W_BOX_LEFT_L+53+2)
#define SB_PRIMARY_W_BOX_BOT_L		(195+1)
										
#define SB_PRIMARY_W_BOX_LEFT_H		68
#define SB_PRIMARY_W_BOX_TOP_H		381
#define SB_PRIMARY_W_BOX_RIGHT_H		179
#define SB_PRIMARY_W_BOX_BOT_H		473
										
#define SB_PRIMARY_W_BOX_LEFT			(GAME_HIRES ? SB_PRIMARY_W_BOX_LEFT_H : SB_PRIMARY_W_BOX_LEFT_L)
#define SB_PRIMARY_W_BOX_TOP			(GAME_HIRES ? SB_PRIMARY_W_BOX_TOP_H : SB_PRIMARY_W_BOX_TOP_L)
#define SB_PRIMARY_W_BOX_RIGHT		(GAME_HIRES ? SB_PRIMARY_W_BOX_RIGHT_H : SB_PRIMARY_W_BOX_RIGHT_L)
#define SB_PRIMARY_W_BOX_BOT			(GAME_HIRES ? SB_PRIMARY_W_BOX_BOT_H : SB_PRIMARY_W_BOX_BOT_L)
										
#define SB_SECONDARY_W_BOX_LEFT_L	169
#define SB_SECONDARY_W_BOX_TOP_L		153
#define SB_SECONDARY_W_BOX_RIGHT_L	(SB_SECONDARY_W_BOX_LEFT_L+54+1)
#define SB_SECONDARY_W_BOX_BOT_L		(153+43)

#define SB_SECONDARY_W_BOX_LEFT_H	338
#define SB_SECONDARY_W_BOX_TOP_H		381
#define SB_SECONDARY_W_BOX_RIGHT_H	449
#define SB_SECONDARY_W_BOX_BOT_H		473

#define SB_SECONDARY_W_BOX_LEFT		(GAME_HIRES ? SB_SECONDARY_W_BOX_LEFT_H : SB_SECONDARY_W_BOX_LEFT_L)	//210
#define SB_SECONDARY_W_BOX_TOP		(GAME_HIRES ? SB_SECONDARY_W_BOX_TOP_H : SB_SECONDARY_W_BOX_TOP_L)
#define SB_SECONDARY_W_BOX_RIGHT		(GAME_HIRES ? SB_SECONDARY_W_BOX_RIGHT_H : SB_SECONDARY_W_BOX_RIGHT_L)
#define SB_SECONDARY_W_BOX_BOT		(GAME_HIRES ? SB_SECONDARY_W_BOX_BOT_H : SB_SECONDARY_W_BOX_BOT_L)

#define SB_PRIMARY_W_PIC_X				(SB_PRIMARY_W_BOX_LEFT+1)	//51
#define SB_PRIMARY_W_PIC_Y				(GAME_HIRES ? 382 : 154)
#define SB_PRIMARY_W_TEXT_X			(SB_PRIMARY_W_BOX_LEFT+ (GAME_HIRES ? 50 : 24))	// (51+23)
#define SB_PRIMARY_W_TEXT_Y			(GAME_HIRES ? 390 : 157)
#define SB_PRIMARY_AMMO_X				(SB_PRIMARY_W_BOX_LEFT+ (GAME_HIRES ?  (38+20) : 30))	// ((SB_PRIMARY_W_BOX_LEFT+33)-3)	// (51+32)
#define SB_PRIMARY_AMMO_Y				(GAME_HIRES ? 410 : 171)

#define SB_SECONDARY_W_PIC_X		 (GAME_HIRES ? 385 :  (SB_SECONDARY_W_BOX_LEFT+29))	// (212+27)
#define SB_SECONDARY_W_PIC_Y		 (GAME_HIRES ? 382 : 154)
#define SB_SECONDARY_W_TEXT_X		 (SB_SECONDARY_W_BOX_LEFT+2)	//212
#define SB_SECONDARY_W_TEXT_Y		 (GAME_HIRES ? 389 : 157)
#define SB_SECONDARY_AMMO_X		 (SB_SECONDARY_W_BOX_LEFT+ (GAME_HIRES ?  (14-4) : 11))	// (212+9)
#define SB_SECONDARY_AMMO_Y		 (GAME_HIRES ? 414 : 171)

#define COCKPIT_PRIMARY_BOX		 (!GAME_HIRES ? 0 : 4)
#define COCKPIT_SECONDARY_BOX		 (!GAME_HIRES ? 1 : 5)
#define SB_PRIMARY_BOX				 (!GAME_HIRES ? 2 : 6)
#define SB_SECONDARY_BOX			 (!GAME_HIRES ? 3 : 7)

#define EXTRA_SHIP_SCORE		50000		//get new ship every this many points

#define INV_FRAME_TIME			(I2X (1)/10)		//how long for each frame
#define CLOAK_FADE_WAIT_TIME  0x400

#define WS_SET				0		//in correct state
#define WS_FADING_OUT	1
#define WS_FADING_IN		2

#define FADE_SCALE	 (2*I2X (FADE_LEVELS)/REARM_TIME)		// fade out and back in REARM_TIME, in fade levels per seconds (int32_t)

//	-----------------------------------------------------------------------------

#define WEAPON_WINDOW_SIZE			63
#define WEAPON_WINDOW_SIZE_HIRES	126

extern tGaugeBox gaugeBoxes [8];
extern tSpan weaponWindowLeft [WEAPON_WINDOW_SIZE];
extern tSpan weaponWindowRight [WEAPON_WINDOW_SIZE];
extern tSpan weaponWindowLeftHires [WEAPON_WINDOW_SIZE_HIRES];
extern tSpan weaponWindowRightHires [WEAPON_WINDOW_SIZE_HIRES];
extern tGaugeBox hudWindowAreas [];
extern uint8_t afterburnerBarTable [AFTERBURNER_GAUGE_H_L * 2];
extern int32_t nDbgGauge;
extern int32_t SW_drawn [2], SW_x [2], SW_y [2], SW_w [2], SW_h [2];
extern CStaticCanvasColor<0,0,0,255> gaugeFadeColors [2][4];

//	-----------------------------------------------------------------------------

#endif //_HUD_DEFS_H
