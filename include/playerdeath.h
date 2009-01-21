#ifndef _PLAYERDEATH_H
#define _PLAYERDEATH_H

void StartPlayerDeathSequence (CObject *playerP);
void DeadPlayerFrame (void);
void DeadPlayerEnd (void);
void SetCameraPos (CFixVector *vCameraPos, CObject *objP);

#endif //_PLAYERDEATH_H
