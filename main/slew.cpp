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

#include "inferno.h"
#include "key.h"
#include "joy.h"
#include "error.h"
#include "physics.h"
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

	slewObjP->mType.physInfo.velocity.setZero();
	return 1;
}

void slew_reset_orient()
{
if (!slewObjP || slewObjP->controlType!=CT_SLEW) 
	return;
slewObjP->position.mOrient[RVEC][X] = 
slewObjP->position.mOrient[UVEC][Y] = 
slewObjP->position.mOrient[FVEC][Z] = f1_0;
slewObjP->position.mOrient[RVEC][Y] = 
slewObjP->position.mOrient[RVEC][Z] = 
slewObjP->position.mOrient[UVEC][X] =
slewObjP->position.mOrient[UVEC][Z] = 
slewObjP->position.mOrient[FVEC][X] = 
slewObjP->position.mOrient[FVEC][Y] = 0;
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
				objP->mType.physInfo.velocity[X] += VEL_SPEED * (KeyDownTime(KEY_PAD3) - KeyDownTime(KEY_PAD1));
			else
				objP->mType.physInfo.velocity[X] += VEL_SPEED * (KeyDownTime(KEY_PAD9) - KeyDownTime(KEY_PAD7));
			objP->mType.physInfo.velocity[Y] += VEL_SPEED * (KeyDownTime(KEY_PADMINUS) - KeyDownTime(KEY_PADPLUS));
			objP->mType.physInfo.velocity[Z] += VEL_SPEED * (KeyDownTime(KEY_PAD8) - KeyDownTime(KEY_PAD2));

			rotang[PA] = (KeyDownTime(KEY_LBRACKET) - KeyDownTime(KEY_RBRACKET))/ROT_SPEED ;
			if (FindArg("-jasen"))
				rotang[BA]  = (KeyDownTime(KEY_PAD7) - KeyDownTime(KEY_PAD9))/ROT_SPEED;
			else
				rotang[BA]  = (KeyDownTime(KEY_PAD1) - KeyDownTime(KEY_PAD3))/ROT_SPEED;
			rotang[HA]  = (KeyDownTime(KEY_PAD6) - KeyDownTime(KEY_PAD4))/ROT_SPEED;
		}
		else {
			objP->mType.physInfo.velocity[X] += VEL_SPEED * Controls [0].sidewaysThrustTime;
			objP->mType.physInfo.velocity[Y] += VEL_SPEED * Controls [0].verticalThrustTime;
			objP->mType.physInfo.velocity[Z] += VEL_SPEED * Controls [0].forwardThrustTime;

			rotang[PA] = Controls [0].pitchTime/ROT_SPEED ;
			rotang[BA]  = Controls [0].bankTime/ROT_SPEED;
			rotang[HA]  = Controls [0].headingTime/ROT_SPEED;
		}
	}
	else
		rotang[PA] = rotang[BA]  = rotang[HA]  = 0;

	//check for joystick movement

	if (check_joy && bJoyPresent && (gameStates.app.nFunctionMode == FMODE_EDITOR) )	{
		JoyGetPos(&joy_x,&joy_y);
		btns=JoyGetBtns();

		joyx_moved = (abs(joy_x - old_joy_x)>JOY_NULL);
		joyy_moved = (abs(joy_y - old_joy_y)>JOY_NULL);

		if (abs(joy_x) < JOY_NULL) joy_x = 0;
		if (abs(joy_y) < JOY_NULL) joy_y = 0;

		if (btns) {
			if (!rotang[PA]) 
				rotang[PA] = (fixang) FixMul (-joy_y * 512,gameData.time.xFrame);
			}
		else {
			if (joyy_moved) 
				objP->mType.physInfo.velocity[Z] = -joy_y * 8192;
			}
		if (!rotang[HA]) 
			rotang[HA] = (fixang) FixMul(joy_x * 512,gameData.time.xFrame);

		if (joyx_moved) old_joy_x = joy_x;
		if (joyy_moved) old_joy_y = joy_y;
	}

	moved = rotang[PA] | rotang[BA] | rotang[HA];

	rotmat = vmsMatrix::Create(rotang);
	// TODO MM
	new_pm = objP->position.mOrient * rotmat;
	objP->position.mOrient = new_pm;
	vmsMatrix::transpose(new_pm);		//make those columns rows

	moved |= objP->mType.physInfo.velocity[X] | objP->mType.physInfo.velocity[Y] | objP->mType.physInfo.velocity[Z];

	svel = objP->mType.physInfo.velocity;
	svel *= gameData.time.xFrame;		//movement in this frame
	movement = new_pm * svel;

//	objP->vLastPos = objP->position.vPos;
	objP->position.vPos += movement;

	moved |= (movement[X] || movement[Y] || movement[Z]);

	if (moved)
		UpdateObjectSeg(objP);	//update tSegment id

	return moved;
}

//do slew for this frame
int slew_frame(int check_keys)
{
	return do_slew_movement( slewObjP, !check_keys, 1 );

}

