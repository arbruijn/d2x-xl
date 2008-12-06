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

int MultiCanRemoveRobot(int nObject, int agitation);
void MultiSendRobotPosition(int nObject, int fired);
void MultiSendRobotFire(int nObject, int gun_num, vmsVector *fire);
void MultiSendClaimRobot(int nObject);
void MultiSendRobotExplode(int,int,char);
void MultiSendCreateRobot(int robotcen, int nObject, int nType);
void MultiSendBossActions(int bossobjnum, int action, int secondary, int nObject);
int MultiSendRobotFrame(int sent);

void MultiDoRobotExplode(char *buf);
void MultiDoRobotPosition(char *buf);
void MultiDoClaimRobot(char *buf);
void MultiDoReleaseRobot(char *buf);
void MultiDoRobotFire(char *buf);
void MultiDoCreateRobot(char *buf);
void MultiDoBossActions(char *buf);
void MultiDoCreateRobotPowerups(char *buf);

int MultiExplodeRobotSub(int botnum, int killer,char);

void MultiDropRobotPowerups(int nObject);
void MultiDumpRobots(void);

void MultiStripRobots(int playernum);
void MultiCheckRobotTimeout(void);

void MultiRobotRequestChange(CObject *robot, int playernum);

#endif /* _MULTIBOT_H */
