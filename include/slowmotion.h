#ifndef _SLOWMOTION_H
#define _SLOWMOTION_H

void DoSlowMotionFrame (void);
int SlowMotionActive (void);
int BulletTimeActive (void);
void SlowMotionOff (void);
void BulletTimeOn (void);
void SpeedupSound (void);
void SlowdownSound (void);

#define USE_SOUND_THREADS	1

#endif //_SLOWMOTION_H
