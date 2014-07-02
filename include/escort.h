/*
 *
 * Header for escort.c
 *
 */

#ifndef _ESCORT_H
#define _ESCORT_H

extern int32_t Buddy_dude_cheat;


void ChangeGuidebotName(void);
void DoEscortMenu(void);
void DetectEscortGoalAccomplished(int32_t index);
void EscortSetSpecialGoal(int32_t key);
void InitBuddyForLevel (void);
void BuddyOuchMessage (fix damage);
void _CDECL_ BuddyMessage (const char * format, ... );

#endif // _ESCORT_H
