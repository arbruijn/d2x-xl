#ifndef _MONSTERBALL_H
#define _MONSTERBALL_H

int ResetMonsterball (bool bCreate = true);
int CreateMonsterball (void);
void RemoveMonsterball (void);
int CheckMonsterballScore (void);
void SetMonsterballForces (void);
CObject* FindMonsterball (void);

extern short nMonsterballForces [];
extern short nMonsterballPyroForce;

#endif //_MONSTERBALL_H
