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

/*
 *
 * Basic slew system for moving around the mine
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:30:57  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:29:32  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.34  1995/02/22  14:23:28  allender
 * remove anonymous unions from tObject structure
 *
 * Revision 1.33  1995/02/22  13:24:26  john
 * Removed the vecmat anonymous unions.
 *
 * Revision 1.32  1994/09/10  15:46:42  john
 * First version of new keyboard configuration.
 *
 * Revision 1.31  1994/08/31  18:29:58  matt
 * Made slew work with new key system
 *
 * Revision 1.30  1994/08/31  14:10:48  john
 * Made slew go faster.
 *
 * Revision 1.29  1994/08/29  19:16:38  matt
 * Made slew tObject not have physics movement nType, so slew gameData.objs.objects don't
 * get bumped.
 *
 * Revision 1.28  1994/08/24  18:59:59  john
 * Changed KeyDownTime to return fixed seconds instead of
 * milliseconds.
 *
 * Revision 1.27  1994/07/01  11:33:05  john
 * Fixed bug with looking for stick even if one not present.
 *
 * Revision 1.26  1994/05/20  11:56:33  matt
 * Cleaned up FindVectorIntersection() interface
 * Killed check_point_in_seg(), check_player_seg(), checkObject_seg()
 *
 * Revision 1.25  1994/05/19  12:08:41  matt
 * Use new vecmat macros and globals
 *
 * Revision 1.24  1994/05/14  17:16:18  matt
 * Got rid of externs in source (non-header) files
 *
 * Revision 1.23  1994/05/03  12:26:38  matt
 * Removed use of tPhysicsInfo var rotVel, which wasn't used for rotational
 * velocity at all.
 *
 * Revision 1.22  1994/02/17  11:32:34  matt
 * Changes in tObject system
 *
 * Revision 1.21  1994/01/18  14:03:53  john
 * made joy_get_pos use the new ints instead of
 * shorts.
 *
 * Revision 1.20  1994/01/10  17:11:35  mike
 * Add prototype for checkObject_seg
 *
 * Revision 1.19  1994/01/05  10:53:38  john
 * New tObject code by John.
 *
 * Revision 1.18  1993/12/22  15:32:50  john
 * took out previos code that attempted to make
 * modifiers cancel keydowntime.
 *
 * Revision 1.17  1993/12/22  11:41:56  john
 * Made so that keydowntime recognizes editor special case!
 *
 * Revision 1.16  1993/12/14  18:13:52  matt
 * Made slew work in editor even when game isn't in slew mode
 *
 * Revision 1.15  1993/12/07  23:53:39  matt
 * Made slew work in editor even when game isn't in slew mode
 *
 * Revision 1.14  1993/12/05  22:47:49  matt
 * Reworked include files in an attempt to cut down on build times
 *
 * Revision 1.13  1993/12/01  11:44:14  matt
 * Chagned Frfract to gameData.time.xFrame
 *
 * Revision 1.12  1993/11/08  16:21:42  john
 * made stop_slew or whatever return an int
 *
 * Revision 1.11  1993/11/01  13:59:49  john
 * more slew experiments.
 *
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
#include "kconfig.h"
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
	if (!slewObjP || slewObjP->controlType!=CT_SLEW) return;

	slewObjP->orient.rVec.x = slewObjP->orient.uVec.y = slewObjP->orient.fVec.z = f1_0;

	slewObjP->orient.rVec.y = slewObjP->orient.rVec.z = slewObjP->orient.uVec.x =
   slewObjP->orient.uVec.z = slewObjP->orient.fVec.x = slewObjP->orient.fVec.y = 0;

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
				objP->mType.physInfo.velocity.x += VEL_SPEED * (KeyDownTime(KEY_PAD3) - KeyDownTime(KEY_PAD1));
			else
				objP->mType.physInfo.velocity.x += VEL_SPEED * (KeyDownTime(KEY_PAD9) - KeyDownTime(KEY_PAD7));
			objP->mType.physInfo.velocity.y += VEL_SPEED * (KeyDownTime(KEY_PADMINUS) - KeyDownTime(KEY_PADPLUS));
			objP->mType.physInfo.velocity.z += VEL_SPEED * (KeyDownTime(KEY_PAD8) - KeyDownTime(KEY_PAD2));

			rotang.p = (KeyDownTime(KEY_LBRACKET) - KeyDownTime(KEY_RBRACKET))/ROT_SPEED ;
			if (FindArg("-jasen"))
				rotang.b  = (KeyDownTime(KEY_PAD7) - KeyDownTime(KEY_PAD9))/ROT_SPEED;
			else
				rotang.b  = (KeyDownTime(KEY_PAD1) - KeyDownTime(KEY_PAD3))/ROT_SPEED;
			rotang.h  = (KeyDownTime(KEY_PAD6) - KeyDownTime(KEY_PAD4))/ROT_SPEED;
		}
		else {
			objP->mType.physInfo.velocity.x += VEL_SPEED * Controls.sideways_thrustTime;
			objP->mType.physInfo.velocity.y += VEL_SPEED * Controls.vertical_thrustTime;
			objP->mType.physInfo.velocity.z += VEL_SPEED * Controls.forward_thrustTime;

			rotang.p = Controls.pitchTime/ROT_SPEED ;
			rotang.b  = Controls.bankTime/ROT_SPEED;
			rotang.h  = Controls.headingTime/ROT_SPEED;
		}
	}
	else
		rotang.p = rotang.b  = rotang.h  = 0;

	//check for joystick movement

	if (check_joy && joy_present && (gameStates.app.nFunctionMode == FMODE_EDITOR) )	{
		joy_get_pos(&joy_x,&joy_y);
		btns=joy_get_btns();
	
		joyx_moved = (abs(joy_x - old_joy_x)>JOY_NULL);
		joyy_moved = (abs(joy_y - old_joy_y)>JOY_NULL);
	
		if (abs(joy_x) < JOY_NULL) joy_x = 0;
		if (abs(joy_y) < JOY_NULL) joy_y = 0;
	
		if (btns)
			if (!rotang.p) rotang.p = FixMul(-joy_y * 512,gameData.time.xFrame); else;
		else
			if (joyy_moved) objP->mType.physInfo.velocity.z = -joy_y * 8192;
	
		if (!rotang.h) rotang.h = FixMul(joy_x * 512,gameData.time.xFrame);
	
		if (joyx_moved) old_joy_x = joy_x;
		if (joyy_moved) old_joy_y = joy_y;
	}

	moved = rotang.p | rotang.b | rotang.h;

	VmAngles2Matrix(&rotmat,&rotang);
	VmMatMul(&new_pm,&objP->orient,&rotmat);
	objP->orient = new_pm;
	VmTransposeMatrix(&new_pm);		//make those columns rows

	moved |= objP->mType.physInfo.velocity.x | objP->mType.physInfo.velocity.y | objP->mType.physInfo.velocity.z;

	svel = objP->mType.physInfo.velocity;
	VmVecScale(&svel,gameData.time.xFrame);		//movement in this frame
	VmVecRotate(&movement,&svel,&new_pm);

//	objP->last_pos = objP->pos;
	VmVecInc(&objP->pos,&movement);

	moved |= (movement.x || movement.y || movement.z);

	if (moved)
		UpdateObjectSeg(objP);	//update tSegment id

	return moved;
}

//do slew for this frame
int slew_frame(int check_keys)
{
	return do_slew_movement( slewObjP, !check_keys, 1 );

}

