/* $Id: escort.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */

/*
 *
 * Header for escort.c
 *
 */

#ifndef _ESCORT_H
#define _ESCORT_H

extern int Buddy_dude_cheat;


void ChangeGuidebotName(void);
void DoEscortMenu(void);
void DetectEscortGoalAccomplished(int index);
void EscortSetSpecialGoal(int key);
void InitBuddyForLevel (void);
void _CDECL_ BuddyMessage (const char * format, ... );

#endif // _ESCORT_H
