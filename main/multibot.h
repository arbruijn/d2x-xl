/* $Id: multibot.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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
 * Old Log:
 * Revision 1.2  1995/08/24  16:04:38  allender
 * fix function prototype for compiler warning
 *
 * Revision 1.1  1995/05/16  15:59:53  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/21  14:40:18  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.0  1995/02/27  11:30:57  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.20  1995/02/06  18:18:05  rob
 * Tweaking cooperative stuff.
 *
 * Revision 1.19  1995/02/04  16:49:50  rob
 * Changed MultiSendRobotFrame to return the number of robot positions that were sent.
 *
 * Revision 1.18  1995/02/04  12:29:32  rob
 * Added buffered fire sends.
 *
 * Revision 1.17  1995/01/30  17:22:02  rob
 * Added prototype for modem robot send function.
 *
 * Revision 1.16  1995/01/26  18:24:56  rob
 * Reduced # of controlled bots.
 *
 * Revision 1.15  1995/01/25  19:26:35  rob
 * Added define for robot laser agitation.
 *
 * Revision 1.14  1995/01/25  13:45:26  rob
 * Added prototype of robot transfer fucm.
 *
 * Revision 1.13  1995/01/14  19:01:08  rob
 * Added prototypes for new functionality.
 *
 * Revision 1.12  1995/01/12  16:41:53  rob
 * Added new prototypes.
 *
 * Revision 1.11  1995/01/12  15:42:55  rob
 * Fixing score awards for coop.
 *
 * Revision 1.10  1995/01/02  21:00:40  rob
 * added robot matcen support.
 *
 * Revision 1.9  1994/12/31  21:03:53  rob
 * More tweaking for robot behavior.
 *
 * Revision 1.8  1994/12/29  13:54:25  rob
 * Ooops.. fixed a bug..
 *
 * Revision 1.7  1994/12/29  12:51:38  rob
 * ADded proto for multi_robot_explode_sub
 *
 * Revision 1.6  1994/12/21  21:08:32  rob
 * Added new functions for robot firing.
 *
 * Revision 1.5  1994/12/21  19:04:02  rob
 * Fixing score accounting for multi robot games.
 *
 * Revision 1.4  1994/12/21  17:36:09  rob
 * Added a new func.
 *
 * Revision 1.3  1994/12/21  11:12:02  rob
 * Added new function and new vars.
 *
 * Revision 1.2  1994/12/19  16:41:42  rob
 * first revision.
 *
 * Revision 1.1  1994/12/16  15:48:20  rob
 * Initial revision
 *
 *
 */


#ifndef _MULTIBOT_H
#define _MULTIBOT_H

int MultiCanRemoveRobot(int objnum, int agitation);
void MultiSendRobotPosition(int objnum, int fired);
void MultiSendRobotFire(int objnum, int gun_num, vms_vector *fire);
void MultiSendClaimRobot(int objnum);
void MultiSendRobotExplode(int,int,char);
void MultiSendCreateRobot(int robotcen, int objnum, int type);
void MultiSendBossActions(int bossobjnum, int action, int secondary, int objnum);
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

void MultiDropRobotPowerups(int objnum);
void MultiDumpRobots(void);

void MultiStripRobots(int playernum);
void MultiCheckRobotTimeout(void);

void MultiRobotRequestChange(object *robot, int playernum);

#endif /* _MULTIBOT_H */
