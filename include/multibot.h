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
 * Header file for multiplayer robot support.
 *
 *
 */


#ifndef _MULTIBOT_H
#define _MULTIBOT_H

int32_t MultiCanControlRobot (int32_t nObject, int32_t agitation);
void MultiSendRobotPosition (int32_t nObject, int32_t fired);
void MultiSendRobotFire (int32_t nObject, int32_t nGun, CFixVector *fire);
void MultiSendClaimRobot (int32_t nObject);
void MultiSendRobotExplode (int32_t, int32_t, char);
void MultiSendCreateRobot (int32_t robotcen, int32_t nObject, int32_t nType);
void MultiSendBossActions (int32_t bossobjnum, int32_t action, int32_t secondary, int32_t nObject);
int32_t MultiSendRobotFrame (int32_t sent);

void MultiDoRobotExplode (uint8_t* buf);
void MultiDoRobotPosition (uint8_t* buf);
void MultiDoClaimRobot (uint8_t* buf);
void MultiDoReleaseRobot (uint8_t* buf);
void MultiDoRobotFire (uint8_t* buf);
void MultiDoCreateRobot (uint8_t* buf);
void MultiDoBossActions (uint8_t* buf);
void MultiDoCreateRobotPowerups (uint8_t* buf);

int32_t MultiDestroyRobot (CObject* robotP, char bIsThief = 0);
CObject* MultiExplodeRobot (int32_t nRobot, int32_t nkiller, char bIsThief);

void MultiDropRobotPowerups (int32_t nObject);
void MultiDumpRobots (void);

void MultiStripRobots (int32_t playernum);
void MultiCheckRobotTimeout (void);

void MultiRobotRequestChange (CObject *robot, int32_t playernum);

#endif /* _MULTIBOT_H */
