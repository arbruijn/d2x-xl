/* $Id: escort.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */

/*
 *
 * Header for escort.c
 *
 */

#ifndef _ESCORT_H
#define _ESCORT_H

extern int Buddy_dude_cheat;


extern void ChangeGuidebotName(void);


extern void DoEscortMenu(void);
extern void DetectEscortGoalAccomplished(int index);
extern void EscortSetSpecialGoal(int key);

#endif // _ESCORT_H
