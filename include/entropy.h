#ifndef _ENTROPY_H
#define _ENTROPY_H

int32_t CountRooms (void);
int32_t GatherFlagGoals (void);
void ConquerRoom (int32_t newOwner, int32_t oldOwner, int32_t roomId);
void StartConquerWarning (void);
void StopConquerWarning (void);

#endif //_ENTROPY_H
