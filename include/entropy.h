#ifndef _ENTROPY_H
#define _ENTROPY_H

int CountRooms (void);
int GatherFlagGoals (void);
int CheckConquerRoom (xsegment *segP);
void ConquerRoom (int newOwner, int oldOwner, int roomId);
void StartConquerWarning (void);
void StopConquerWarning (void);

#endif //_ENTROPY_H
