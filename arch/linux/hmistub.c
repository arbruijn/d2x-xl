#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif
#if !(defined (USE_SDL_MIXER) && USE_SDL_MIXER)
void DigiSetMidiVolume( int mvolume ) { }
void DigiPlayMidiSong( char * filename, char * melodic_bank, char * drum_bank, int loop ) {}
#endif
