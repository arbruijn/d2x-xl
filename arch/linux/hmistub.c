#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif
#if !(defined (USE_SDL_MIXER) && USE_SDL_MIXER)
void DigiSetMidiVolume( int mvolume ) { }
int DigiPlayMidiSong( char * filename, char * melodic_bank, char * drum_bank, int loop ) {return 0;}
#endif
