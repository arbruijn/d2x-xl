/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <conio.h>
#include <stdio.h>

#include "iff.h"
//#include "gr.h"
#include "vesa.h"

draw_ubitmap(int x,int y,grsBitmap *bm)
{
	int xx,yy;
	short *data15 = (short *) bm->bmTexBuf;

//printf("x,y=%d,%d  w,h=%d,%d\n",x,y,bm->bmProps.w,bm->bmProps.h);

	for (yy=0;yy<bm->bmProps.h;yy++)

		for (xx=0;xx<bm->bmProps.w;xx++)

			gr_vesa_pixel15(x+xx,y+yy,data15[yy*bm->bmProps.w+xx]);


}


main(int argc,char **argv)
{
	int ret;
	grsBitmap my_bitmap;
	ubyte my_palette[256*3];

	ret = iff_read_bitmap(argv[1],&my_bitmap,BM_RGB15,&my_palette);

	//printf("ret = %d\n",ret);

	if (ret == IFF_NO_ERROR) {

		gr_vesa_setmode(0x110);

		draw_ubitmap(0,0,&my_bitmap);
	
		getch();

	}

}

