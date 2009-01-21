#ifndef _MONSTERBALL_H
#define _MONSTERBALL_H

int FindMonsterball (void);
int CreateMonsterball (void);
void RemoveMonsterball (void);
int CheckMonsterballScore (void);
void SetMonsterballForces (void);

extern short nMonsterballForces [];
extern short nMonsterballPyroForce;

#endif //_MONSTERBALL_H
