/* altsound.h created on 11/13/99 by Victor Rachels to use alternate sounds */
#ifndef _ALTSOUND_H
#define _ALTSOUND_H
 
extern int use_altSounds;
extern int use_altsound[MAX_SOUNDS];
extern digiSound altsound_list[MAX_SOUNDS];

void load_altSounds(char *fname);
void free_altSounds();
 
digiSound *Sounddat(int nSound);
int DigiXlatSound(short nSound);

#endif // _ALTSOUND_H