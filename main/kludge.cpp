/* $Id: kludge.c,v 1.18 2004/12/01 12:48:13 btb Exp $ */

/*
 *
 * DPH: This is the file where all the stub functions go. The aim is
 * to have nothing in here, eventually
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "gr.h"
#include "pstypes.h"
#include "maths.h"

int nWindowClipLeft,nWindowClipTop,nWindowClipRight,nWindowClipBot;

char CDROM_dir[40] = ".";

#ifndef __DJGPP__
void JoySetBtnValues( int btn, int state, fix timeDown, int downcount, int upcount )
{

}
#endif

void KeyPutKey(char i)
{

}

void g3_remap_interp_colors()
{

}

/*
extern short interpColorTable
void g3_remap_interp_colors()
{
 int eax, ebx;
 
 ebx = 0;
 if (ebx != n_interp_colors) {
   eax = 0;
   eax = interpColorTable
 }

}
*/

int com_init(void)
{
	return 0;
}

void comLevel_sync(void)
{

}

void com_main_menu()
{

}

void com_do_frame()
{

}

void com_send_data(char *ptr, int len, int repeat)
{
}

void com_endlevel(int *secret)
{
}

void serial_leave_game()
{
}

void network_dump_appletalk_player(ubyte node, ushort net, ubyte socket, int why)
{
}
