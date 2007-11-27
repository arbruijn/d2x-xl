/* $Id: slew.c,v 1.4 2003/10/10 09:36:35 btb Exp $ */
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

#ifdef RCS
static char rcsid[] = "$Id: slew.c,v 1.4 2003/10/10 09:36:35 btb Exp $";
#endif

#include <stdlib.h>

#include "inferno.h"
#include "game.h"
#include "vecmat.h"
#include "key.h"
#include "joy.h"
#include "object.h"
#include "error.h"
#include "physics.h"
#include "joydefs.h"
#include "input.h"
#include "args.h"

//variables for slew system

tObject *slewObjP=NULL;	//what tObject is slewing, or NULL if none

#define JOY_NULL 15
#define ROT_SPEED 8		//rate of rotation while key held down
#define VEL_SPEED (2*55)	//rate of acceleration while key held down

short old_joy_x,old_joy_y;	//position last time around

//	Function Prototypes
int slew_stop(void);


// -------------------------------------------------------------------
//say start slewing with this tObject
void slew_init(tObject *objP)
{
	slewObjP = objP;

	slewObjP->controlType = CT_SLEW;
	slewObjP->movementType = MT_NONE;

	slew_stop();		//make sure not moving
}


int slew_stop()
{
	if (!slewObjP || slewObjP->controlType!=CT_SLEW) return 0;

	VmVecZero(&slewObjP->mType.physInfo.velocity);
	return 1;
}

void slew_reset_orient()
{
if (!slewObjP || slewObjP->controlType!=CT_SLEW) 
	return;
slewObjP->position.mOrient.rVec.p.x = 
slewObjP->position.mOrient.uVec.p.y = 
slewObjP->position.mOrient.fVec.p.z = f1_0;
slewObjP->position.mOrient.rVec.p.y = 
slewObjP->position.mOrient.rVec.p.z = 
slewObjP->position.mOrient.uVec.p.x =
slewObjP->position.mOrient.uVec.p.z = 
slewObjP->position.mOrient.fVec.p.x = 
slewObjP->position.mOrient.fVec.p.y = 0;
}

int do_slew_movement(tObject *objP, int check_keys, int check_joy )
{
	int moved = 0;
	vmsVector svel, movement;				//scaled velocity (per this frame)
	vmsMatrix rotmat,new_pm;
	int joy_x,joy_y,btns;
	int joyx_moved,joyy_moved;
	vmsAngVec rotang;

	if (!slewObjP || slewObjP->controlType!=CT_SLEW) return 0;

	if (check_keys) {
		if (gameStates.app.nFunctionMode == FMODE_EDITOR) {
			if (FindArg("-jasen"))
				objP->mType.physInfo.velocity.p.x += VEL_SPEED * (KeyDownTime(KEY_PAD3) - KeyDownTime(KEY_PAD1));
			else
				objP->mType.physInfo.velocity.p.x += VEL_SPEED * (KeyDownTime(KEY_PAD9) - KeyDownTime(KEY_PAD7));
			objP->mType.physInfo.velocity.p.y += VEL_SPEED * (KeyDownTime(KEY_PADMINUS) - KeyDownTime(KEY_PADPLUS));
			objP->mType.physInfo.velocity.p.z += VEL_SPEED * (KeyDownTime(KEY_PAD8) - KeyDownTime(KEY_PAD2));

			rotang.p = (KeyDownTime(KEY_LBRACKET) - KeyDownTime(KEY_RBRACKET))/ROT_SPEED ;
			if (FindArg("-jasen"))
				rotang.b  = (KeyDownTime(KEY_PAD7) - KeyDownTime(KEY_PAD9))/ROT_SPEED;
			else
				rotang.b  = (KeyDownTime(KEY_PAD1) - KeyDownTime(KEY_PAD3))/ROT_SPEED;
			rotang.h  = (KeyDownTime(KEY_PAD6) - KeyDownTime(KEY_PAD4))/ROT_SPEED;
		}
		else {
			objP->mType.physInfo.velocity.p.x += VEL_SPEED * Controls [0].sidewaysThrustTime;
			objP->mType.physInfo.velocity.p.y += VEL_SPEED * Controls [0].verticalThrustTime;
			objP->mType.physInfo.velocity.p.z += VEL_SPEED * Controls [0].forwardThrustTime;

			rotang.p = Controls [0].pitchTime/ROT_SPEED ;
			rotang.b  = Controls [0].bankTime/ROT_SPEED;
			rotang.h  = Controls [0].headingTime/ROT_SPEED;
		}
	}
	else
		rotang.p = rotang.b  = rotang.h  = 0;

	//check for joystick movement

	if (check_joy && joy_present && (gameStates.app.nFunctionMode == FMODE_EDITOR) )	{
		JoyGetPos(&joy_x,&joy_y);
		btns=JoyGetBtns();

		joyx_moved = (abs(joy_x - old_joy_x)>JOY_NULL);
		joyy_moved = (abs(joy_y - old_joy_y)>JOY_NULL);

		if (abs(joy_x) < JOY_NULL) joy_x = 0;
		if (abs(joy_y) < JOY_NULL) joy_y = 0;

		if (btns) {
			if (!rotang.p) 
				rotang.p = (fixang) FixMul (-joy_y * 512,gameData.time.xFrame);
			}
		else {
			if (joyy_moved) 
				objP->mType.physInfo.velocity.p.z = -joy_y * 8192;
			}
		if (!rotang.h) 
			rotang.h = (fixang) FixMul(joy_x * 512,gameData.time.xFrame);

		if (joyx_moved) old_joy_x = joy_x;
		if (joyy_moved) old_joy_y = joy_y;
	}

	moved = rotang.p | rotang.b | rotang.h;

	VmAngles2Matrix(&rotmat,&rotang);
	VmMatMul(&new_pm,&objP->position.mOrient,&rotmat);
	objP->position.mOrient = new_pm;
	VmTransposeMatrix(&new_pm);		//make those columns rows

	moved |= objP->mType.physInfo.velocity.p.x | objP->mType.physInfo.velocity.p.y | objP->mType.physInfo.velocity.p.z;

	svel = objP->mType.physInfo.velocity;
	VmVecScale(&svel,gameData.time.xFrame);		//movement in this frame
	VmVecRotate(&movement,&svel,&new_pm);

//	objP->vLastPos = objP->position.vPos;
	VmVecInc(&objP->position.vPos,&movement);

	moved |= (movement.p.x || movement.p.y || movement.p.z);

	if (moved)
		UpdateObjectSeg(objP);	//update tSegment id

	return moved;
}

//do slew for this frame
int slew_frame(int check_keys)
{
	return do_slew_movement( slewObjP, !check_keys, 1 );

}

