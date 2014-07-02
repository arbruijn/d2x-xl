#ifndef _MONSTERBALL_H
#define _MONSTERBALL_H

int32_t ResetMonsterball (bool bCreate = true);
int32_t CreateMonsterball (void);
void RemoveMonsterball (void);
int32_t CheckMonsterballScore (void);
void SetMonsterballForces (void);
CObject* FindMonsterball (void);
void SetMonsterballDefaults (tMonsterballInfo *monsterballP = NULL);

extern int16_t nMonsterballForces [];
extern int16_t nMonsterballPyroForce;

#endif //_MONSTERBALL_H
