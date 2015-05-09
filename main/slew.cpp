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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>

#include "descent.h"
#include "key.h"
#include "joy.h"
#include "error.h"
#include "physics.h"
#include "input.h"
#include "args.h"

//variables for slew system

CObject *slewObjP=NULL;	//what CObject is slewing, or NULL if none

#define JOY_NULL 15
#define ROT_SPEED 8		//rate of rotation while key held down
#define VEL_SPEED (2*55)	//rate of acceleration while key held down

int16_t old_joy_x,old_joy_y;	//position last time around

//	Function Prototypes
int32_t slew_stop(void);


// -------------------------------------------------------------------
//say start slewing with this CObject
void slew_init(CObject *pObj)
{
	slewObjP = pObj;

	slewObjP->info.controlType = CT_SLEW;
	slewObjP->info.movementType = MT_NONE;

	slew_stop();		//make sure not moving
}


int32_t slew_stop()
{
	if (!slewObjP || slewObjP->info.controlType!=CT_SLEW) return 0;

	slewObjP->mType.physInfo.velocity.SetZero ();
	return 1;
}

void slew_reset_orient()
{
if (!slewObjP || slewObjP->info.controlType!=CT_SLEW) 
	return;
slewObjP->info.position.mOrient.m.dir.r.v.coord.x = 
slewObjP->info.position.mOrient.m.dir.u.v.coord.y = 
slewObjP->info.position.mOrient.m.dir.f.v.coord.z = I2X (1);
slewObjP->info.position.mOrient.m.dir.r.v.coord.y = 
slewObjP->info.position.mOrient.m.dir.r.v.coord.z = 
slewObjP->info.position.mOrient.m.dir.u.v.coord.x =
slewObjP->info.position.mOrient.m.dir.u.v.coord.z = 
slewObjP->info.position.mOrient.m.dir.f.v.coord.x = 
slewObjP->info.position.mOrient.m.dir.f.v.coord.y = 0;
}

int32_t do_slew_movement(CObject *pObj, int32_t check_keys, int32_t check_joy )
{
	int32_t moved = 0;
	CFixVector svel, movement;				//scaled velocity (per this frame)
	CFixMatrix rotmat,new_pm;
	int32_t joy_x,joy_y,btns;
	int32_t joyx_moved,joyy_moved;
	CAngleVector rotang;

	if (!slewObjP || slewObjP->info.controlType!=CT_SLEW) return 0;

	if (check_keys) {
		if (gameStates.app.nFunctionMode == FMODE_EDITOR) {
			if (FindArg ("-jasen"))
				pObj->mType.physInfo.velocity.v.coord.x += VEL_SPEED * (KeyDownTime(KEY_PAD3) - KeyDownTime(KEY_PAD1));
			else
				pObj->mType.physInfo.velocity.v.coord.x += VEL_SPEED * (KeyDownTime(KEY_PAD9) - KeyDownTime(KEY_PAD7));
			pObj->mType.physInfo.velocity.v.coord.y += VEL_SPEED * (KeyDownTime(KEY_PADMINUS) - KeyDownTime(KEY_PADPLUS));
			pObj->mType.physInfo.velocity.v.coord.z += VEL_SPEED * (KeyDownTime(KEY_PAD8) - KeyDownTime(KEY_PAD2));

			rotang.v.coord.p = (KeyDownTime(KEY_LBRACKET) - KeyDownTime(KEY_RBRACKET))/ROT_SPEED ;
			if (FindArg ("-jasen"))
				rotang.v.coord.b  = (KeyDownTime(KEY_PAD7) - KeyDownTime(KEY_PAD9))/ROT_SPEED;
			else
				rotang.v.coord.b  = (KeyDownTime(KEY_PAD1) - KeyDownTime(KEY_PAD3))/ROT_SPEED;
			rotang.v.coord.h  = (KeyDownTime(KEY_PAD6) - KeyDownTime(KEY_PAD4))/ROT_SPEED;
		}
		else {
			pObj->mType.physInfo.velocity.v.coord.x += VEL_SPEED * controls [0].sidewaysThrustTime;
			pObj->mType.physInfo.velocity.v.coord.y += VEL_SPEED * controls [0].verticalThrustTime;
			pObj->mType.physInfo.velocity.v.coord.z += VEL_SPEED * controls [0].forwardThrustTime;

			rotang.v.coord.p = controls [0].pitchTime/ROT_SPEED ;
			rotang.v.coord.b  = controls [0].bankTime/ROT_SPEED;
			rotang.v.coord.h  = controls [0].headingTime/ROT_SPEED;
		}
	}
	else
		rotang.v.coord.p = rotang.v.coord.b  = rotang.v.coord.h  = 0;

	//check for joystick movement

	if (check_joy && bJoyPresent && (gameStates.app.nFunctionMode == FMODE_EDITOR) ) {
		JoyGetPos(&joy_x,&joy_y);
		btns=JoyGetBtns();

		joyx_moved = (abs(joy_x - old_joy_x)>JOY_NULL);
		joyy_moved = (abs(joy_y - old_joy_y)>JOY_NULL);

		if (abs(joy_x) < JOY_NULL) joy_x = 0;
		if (abs(joy_y) < JOY_NULL) joy_y = 0;

		if (btns) {
			if (!rotang.v.coord.p)
				rotang.v.coord.p = (fixang) FixMul (-joy_y * 512,gameData.time.xFrame);
			}
		else {
			if (joyy_moved) 
				pObj->mType.physInfo.velocity.v.coord.z = -joy_y * 8192;
			}
		if (!rotang.v.coord.h)
			rotang.v.coord.h = (fixang) FixMul(joy_x * 512,gameData.time.xFrame);

		if (joyx_moved) old_joy_x = joy_x;
		if (joyy_moved) old_joy_y = joy_y;
	}

	moved = rotang.v.coord.p | rotang.v.coord.b | rotang.v.coord.h;

	rotmat = CFixMatrix::Create(rotang);
	new_pm = pObj->info.position.mOrient * rotmat;
	pObj->info.position.mOrient = new_pm;
	CFixMatrix::Transpose(new_pm);		//make those columns rows

	moved |= pObj->mType.physInfo.velocity.v.coord.x | pObj->mType.physInfo.velocity.v.coord.y | pObj->mType.physInfo.velocity.v.coord.z;

	svel = pObj->mType.physInfo.velocity;
	svel *= gameData.time.xFrame;		//movement in this frame
	movement = new_pm * svel;

//	pObj->info.vLastPos = pObj->info.position.vPos;
	pObj->info.position.vPos += movement;

	moved |= (movement.v.coord.x || movement.v.coord.y || movement.v.coord.z);

	if (moved)
		UpdateObjectSeg(pObj);	//update CSegment id

	return moved;
}

//do slew for this frame
int32_t slew_frame(int32_t check_keys)
{
	return do_slew_movement( slewObjP, !check_keys, 1 );

}

