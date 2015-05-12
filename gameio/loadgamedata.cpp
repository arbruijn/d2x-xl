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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "descent.h"
#include "pstypes.h"
#include "strutil.h"
#include "text.h"
#include "gr.h"
#include "ogl_defs.h"
#include "loadgamedata.h"
#include "u_mem.h"
#include "mono.h"
#include "error.h"
#include "object.h"
#include "vclip.h"
#include "effects.h"
#include "polymodel.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#include "multi.h"
#include "iff.h"
#include "cfile.h"
#include "powerup.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "cockpit.h"
#include "player.h"
#include "endlevel.h"
#include "reactor.h"
#include "makesig.h"
#include "interp.h"
#include "light.h"
#include "byteswap.h"
#include "network.h"

#define PRINT_WEAPON_INFO	0

//-----------------------------------------------------------------------------

CPlayerShip defaultShipProps;
#if 0
 = {
	108, 58, 262144, 2162, 511180, 0, 0, I2X (1) / 2, 9175,
 {CFixVector::Create(146013, -59748, 35756),
	 CFixVector::Create(-147477, -59892, 34430),
 	 CFixVector::Create(222008, -118473, 148201),
	 CFixVector::Create(-223479, -118213, 148302),
	 CFixVector::Create(153026, -185, -91405),
	 CFixVector::Create(-156840, -185, -91405),
	 CFixVector::Create(1608, -87663, 184978),
	 CFixVector::Create(-1608, -87663, -190825)}};
#endif

//-----------------------------------------------------------------------------

CWeaponInfo defaultWeaponInfoD2 [] = {
   {2,0,111,114,11,59,13,1,1,11,0,0,28,1,0,0,0,128,0,0,0,-1,32112,16384,65536,{0},0,65536,196608,{589824,589824,589824,589824,589824},{7864320,7864320,7864320,7864320,7864320},32768,0,0,642252,49152,655360,0,{253},{254}},
   {2,0,115,118,15,21,14,30,1,11,0,0,28,1,0,0,0,128,0,0,0,-1,32112,16384,65536,{0},0,72089,196608,{638976,638976,638976,638976,589824},{8192000,8192000,8192000,8192000,8192000},32768,0,0,642252,65536,655360,0,{0},{0}},
   {2,0,119,122,14,23,15,31,1,11,0,0,28,1,0,0,0,128,0,0,0,-1,32112,16384,65536,{0},0,75366,196608,{688128,688128,688128,688128,655360},{8519680,8519680,8519680,8519680,8519680},32768,0,0,642252,81920,655360,0,{0},{0}},
   {2,0,123,126,12,22,16,32,1,11,0,0,28,1,0,0,0,128,0,0,0,-1,32112,16384,65536,{0},0,78643,196608,{737280,737280,737280,737280,720896},{8847360,8847360,8847360,8847360,8847360},32768,0,0,642252,114688,655360,0,{0},{0}},
   {3,0,-1,-1,14,23,18,23,1,11,0,55,28,1,0,0,0,128,0,0,0,-1,32768,16384,65536,{0},65536,131072,163840,{131072,196608,262144,327680,393216},{983040,1966080,2949120,3932160,5570560},65536,0,0,655360,65536,655360,0,{0},{0}},
   {3,0,-1,-1,11,59,19,59,1,11,0,56,28,1,0,0,0,128,0,0,0,-1,32768,16384,65536,{0},65536,163840,163840,{196608,262144,393216,458752,524288},{983040,1966080,2949120,3932160,5570560},65536,0,0,655360,65536,655360,0,{0},{0}},
   {3,0,-1,-1,11,59,22,1,1,11,0,64,28,1,0,0,0,128,0,0,0,-1,32768,16384,65536,{0},65536,196608,196608,{262144,393216,589824,720896,851968},{983040,1966080,2949120,3932160,5570560},65536,0,0,655360,65536,655360,0,{0},{0}},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,{0},0,0,0,{0,0,0,0,0},{0,0,0,0,0},0,0,0,0,0,0,0,{0},{0}},
   {2,0,127,-1,11,5,130,5,1,11,1,0,11,0,1,0,0,128,0,0,0,-1,0,32768,65536,{0},0,327680,458752,{1966080,1966080,1966080,1966080,1966080},{5898240,5898240,5898240,5898240,5898240},65536,0,0,655360,262144,655360,1966080,{255},{256}},
   {2,0,128,-1,11,59,23,-1,1,11,0,0,-1,1,0,0,0,128,0,0,0,-1,65536,75366,65536,{0},0,32768,65536,{65536,65536,65536,65536,65536},{7864320,7864320,7864320,7864320,7864320},6553,0,0,327680,196608,327680,0,{0},{0}},
   {2,0,129,130,14,23,15,31,1,11,0,0,11,1,0,0,0,128,0,0,0,-1,32768,16384,65536,{0},0,196608,196608,{131072,196608,196608,262144,393216},{1966080,2621440,3276800,3932160,5242880},6553,0,0,642252,65536,655360,0,{0},{0}},
   {-1,0,-1,-1,11,1,115,1,1,11,1,0,-1,0,1,0,0,128,0,0,0,-1,0,3276,65536,{259},32768,131072,163840,{262144,262144,262144,262144,262144},{32768000,32768000,32768000,32768000,32768000},655,0,0,655360,0,196608,0,{257},{258}},
   {1,0,-1,-1,14,23,18,23,1,11,0,0,11,1,0,0,0,128,0,0,0,-1,32768,13107,65536,{262},32768,131072,163840,{589824,589824,589824,589824,589824},{8519680,8519680,8519680,8519680,8519680},13107,0,0,655360,65536,655360,0,{260},{261}},
   {3,0,-1,-1,12,23,25,32,1,11,0,54,11,1,0,0,0,128,0,0,0,-1,32768,9830,65536,{0},85196,131072,229376,{720896,720896,720896,720896,720896},{9830400,9830400,9830400,9830400,9830400},655,0,0,655360,131072,655360,0,{263},{264}},
   {2,1,131,132,15,65,24,65,1,11,0,0,11,1,0,0,0,128,0,0,0,-1,32768,65536,32768,{0},0,393216,327680,{1966080,1966080,1966080,1966080,1966080},{9830400,9830400,9830400,9830400,9830400},32768,0,0,65536,327680,655360,0,{265},{266}},
   {2,0,133,-1,11,5,130,5,1,11,1,0,11,0,1,0,1,128,0,0,0,-1,0,32768,65536,{0},655360,327680,458752,{2621440,2621440,2621440,2621440,2621440},{5898240,5898240,5898240,5898240,5898240},65536,0,0,65536,262144,655360,1310720,{267},{268}},
   {3,0,-1,-1,11,5,26,5,1,11,1,53,11,1,1,1,0,128,0,0,0,-1,0,13107,65536,{0},196608,327680,458752,{1966080,1966080,1966080,1966080,1966080},{0,0,0,0,0},65536,2162,0,655360,0,2293760,2621440,{269},{270}},
   {2,0,134,-1,11,5,130,5,1,11,1,0,11,0,1,0,0,128,0,0,0,19,0,32768,65536,{0},655360,327680,458752,{1638400,1638400,1638400,1638400,1638400},{5570560,5570560,5570560,5570560,5570560},65536,0,0,65536,524288,196608,1310720,{271},{272}},
   {2,0,135,-1,11,63,132,5,1,231,1,0,231,0,1,0,1,128,0,0,0,-1,0,65536,65536,{0},0,327680,1966080,{13041664,13041664,13041664,13041664,13041664},{5570560,5570560,5570560,5570560,5570560},65536,0,0,655360,524288,655360,5242880,{274},{275}},
   {3,0,-1,-1,11,5,12,5,1,11,1,54,11,0,0,0,1,128,0,0,0,-1,0,98304,65536,{0},98304,327680,458752,{2293760,2293760,2293760,2293760,2293760},{5898240,5898240,5898240,5898240,5898240},65536,0,0,65536,65536,786432,0,{0},{0}},
   {1,0,-1,-1,14,23,18,23,1,11,0,0,11,1,0,0,0,128,0,0,0,-1,65536,26214,65536,{262},32768,131072,163840,{65536,262144,393216,524288,655360},{5242880,5570560,5898240,6553600,7208960},13107,0,0,655360,32768,655360,0,{260},{261}},
   {2,0,136,-1,11,5,130,5,1,11,1,0,11,0,1,0,1,128,0,0,0,-1,0,32768,65536,{0},655360,327680,458752,{655360,983040,1310720,1638400,1966080},{3932160,3932160,3932160,3932160,5242880},65536,0,0,65536,262144,655360,2621440,{269},{268}},
   {2,0,137,-1,11,5,130,5,1,11,1,0,11,0,1,0,0,128,0,0,0,-1,0,32768,65536,{0},0,327680,458752,{524288,786432,1179648,1507328,1966080},{2621440,3276800,3932160,4587520,6225920},65536,0,0,655360,262144,655360,2621440,{255},{256}},
   {1,0,-1,-1,14,23,18,23,1,11,0,0,-1,1,0,0,0,128,0,0,0,-1,65536,13107,65536,{262},32768,131072,163840,{786432,786432,786432,786432,786432},{13107200,13107200,13107200,13107200,13107200},13107,0,0,655360,0,655360,0,{260},{261}},
   {2,0,138,139,11,59,15,1,1,11,0,0,28,1,0,0,0,128,0,0,0,-1,32768,16384,65536,{0},0,196608,196608,{65536,131072,196608,262144,393216},{1966080,2621440,3276800,3932160,5242880},6553,0,0,642252,65536,655360,0,{0},{0}},
   {2,0,140,141,12,22,15,32,1,11,0,0,28,1,0,0,0,128,0,0,0,-1,32768,16384,65536,{0},0,196608,196608,{131072,196608,196608,262144,393216},{2490368,3145728,3801088,4456448,5111808},6553,0,0,642252,65536,655360,0,{0},{0}},
   {3,0,-1,-1,12,23,18,32,1,11,0,54,11,1,0,0,0,128,0,0,0,-1,39321,9830,65536,{0},85196,131072,229376,{196608,327680,458752,524288,655360},{5242880,5898240,6553600,7864320,9830400},655,0,0,655360,0,655360,0,{263},{264}},
   {3,0,-1,-1,11,59,19,59,1,11,0,56,28,1,0,0,0,128,0,0,0,-1,32768,16384,65536,{0},65536,163840,163840,{327680,393216,524288,786432,1179648},{983040,1966080,2949120,3932160,5242880},131072,0,0,655360,65536,655360,0,{0},{0}},
   {2,0,142,-1,11,63,132,5,1,11,1,0,11,0,1,0,1,128,0,0,0,-1,0,32768,65536,{0},0,327680,1966080,{3276800,4587520,5898240,7208960,7864320},{4259840,4915200,5570560,5570560,5570560},65536,0,0,655360,524288,655360,5242880,{274},{275}},
   {3,0,-1,-1,11,5,12,5,1,11,1,54,11,0,0,0,1,128,0,0,0,-1,0,98304,65536,{0},98304,327680,458752,{262144,393216,589824,720896,851968},{2621440,3276800,3932160,4587520,5898240},65536,0,0,65536,65536,524288,0,{0},{0}},
   {2,0,143,146,85,59,44,87,1,11,0,0,28,1,0,0,0,128,0,0,0,-1,49152,16384,65536,{0},0,81920,196608,{786432,786432,786432,786432,786432},{8847360,8847360,8847360,8847360,8847360},32768,0,0,642252,114688,655360,0,{277},{278}},
   {2,0,147,150,86,59,45,88,1,11,0,0,28,1,0,0,0,128,0,0,0,-1,49152,16384,65536,{0},0,81920,196608,{819200,819200,819200,819200,786432},{8847360,8847360,8847360,8847360,8847360},39321,0,0,642252,114688,655360,0,{0},{0}},
   {-1,0,-1,-1,11,95,230,95,1,11,2,0,-1,0,1,0,0,128,0,0,0,-1,0,9830,65536,{259},32768,131072,229376,{1638400,1638400,1638400,1638400,1638400},{45875200,45875200,45875200,45875200,45875200},13107,0,0,655360,0,196608,655360,{279},{280}},
   {3,0,-1,-1,12,22,37,22,1,11,0,98,11,1,0,0,0,128,0,0,0,-1,49152,9830,49152,{0},32768,131072,163840,{720896,720896,720896,720896,720896},{8519680,8519680,8519680,8519680,8519680},13107,0,0,655360,98304,655360,0,{281},{282}},
   {3,0,-1,-1,11,23,38,59,1,11,0,68,11,1,0,2,0,128,0,0,0,-1,65536,7864,39321,{0},147456,131072,229376,{786432,786432,786432,786432,786432},{9175040,9175040,9175040,9175040,9175040},655,0,0,655360,131072,655360,0,{283},{284}},
   {3,0,-1,-1,86,105,77,105,1,11,0,103,11,1,0,0,0,128,0,0,0,-1,0,65536,32768,{0},58982,81920,262144,{458752,458752,458752,458752,458752},{65536000,65536000,65536000,65536000,65536000},655,0,0,655360,65536,655,0,{285},{286}},
   {2,0,151,-1,11,5,130,5,1,76,1,0,76,0,1,0,0,128,0,16,0,-1,0,39321,65536,{0},0,327680,458752,{589824,589824,589824,589824,589824},{7208960,7208960,7208960,7208960,7208960},65536,0,0,655360,262144,1966080,5242880,{287},{288}},
   {2,0,152,-1,11,5,130,5,1,11,1,0,11,0,1,1,1,128,0,0,0,-1,0,19660,65536,{0},655360,327680,458752,{3604480,3604480,3604480,3604480,3604480},{3932160,3932160,3932160,3932160,3932160},65536,0,0,65536,262144,1310720,2621440,{290},{291}},
   {3,0,-1,-1,11,5,26,5,1,11,1,80,11,1,1,1,0,128,0,0,0,47,0,13107,65536,{0},196608,327680,458752,{1966080,1966080,1966080,1966080,1966080},{0,0,0,0,0},65536,2162,0,655360,0,7864320,655,{293},{294}},
   {2,0,153,-1,11,106,133,5,1,11,1,0,11,0,1,0,0,128,0,0,16,-1,0,49152,65536,{0},0,327680,327680,{3276800,3276800,3276800,3276800,3276800},{26214400,26214400,26214400,26214400,26214400},78643,0,0,655360,262144,655360,1966080,{295},{296}},
   {2,0,154,-1,11,104,250,5,1,231,1,0,231,0,1,0,0,128,0,0,32,54,0,98304,65536,{0},0,327680,3276800,{14417920,14417920,14417920,14417920,14417920},{7864320,7864320,7864320,7864320,7864320},65536,0,0,655360,524288,655360,5242880,{298},{299}},
   {2,0,155,-1,11,1,115,1,1,11,1,0,-1,0,1,0,0,128,0,0,0,-1,0,6553,65536,{0},32768,131072,163840,{65536,65536,131072,131072,196608},{4587520,5898240,7864320,9830400,11141120},655,0,0,642252,0,196608,0,{257},{0}},
   {3,0,-1,-1,11,23,15,59,1,11,0,97,28,1,0,0,0,128,0,0,0,-1,32768,16384,65536,{0},32768,196608,131072,{131072,196608,262144,327680,393216},{983040,1638400,2949120,3932160,5242880},6553,0,0,642252,65536,655360,0,{0},{0}},
   {2,0,156,157,86,22,15,32,1,11,0,0,28,1,0,0,0,128,0,0,0,-1,32768,16384,65536,{0},0,91750,196608,{196608,262144,327680,458752,524288},{1638400,2293760,3604480,5242880,5898240},6553,0,0,642252,65536,655360,0,{0},{0}},
   {3,0,-1,-1,11,23,38,59,1,11,0,68,11,1,0,2,0,128,0,0,0,-1,32768,6553,65536,{0},147456,131072,229376,{262144,327680,393216,524288,589824},{2293760,2949120,4259840,5242880,5898240},655,0,0,655360,131072,655360,0,{283},{0}},
   {3,0,-1,-1,11,23,38,59,1,11,0,68,11,1,0,2,0,128,0,0,0,-1,32768,6553,65536,{0},147456,131072,229376,{327680,458752,589824,720896,786432},{4587520,5898240,7208960,9175040,10485760},655,0,0,655360,131072,655360,0,{283},{0}},
   {3,0,-1,-1,14,23,18,23,1,11,0,67,11,1,0,0,0,128,0,0,0,-1,65536,26214,65536,{0},32768,131072,163840,{196608,327680,458752,589824,720896},{4259840,5242880,6553600,7208960,7864320},13107,0,0,655360,0,655360,0,{281},{0}},
   {3,0,-1,-1,11,5,12,2,1,11,1,92,11,0,0,0,1,128,0,0,0,-1,0,98304,65536,{0},131072,327680,196608,{1638400,1638400,1638400,1638400,1638400},{5898240,5898240,5898240,5898240,5898240},65536,0,0,65536,65536,786432,0,{0},{0}},
   {3,0,-1,-1,12,23,18,32,1,11,0,92,11,1,0,0,0,128,0,0,0,-1,39321,9830,65536,{0},85196,131072,229376,{196608,327680,458752,524288,589824},{3276800,5242880,6553600,7864320,9830400},655,0,0,655360,0,655360,0,{263},{0}},
   {3,0,-1,-1,11,5,12,2,1,11,1,92,11,0,0,0,1,128,0,0,0,-1,0,98304,65536,{0},131072,327680,196608,{786432,851968,917504,983040,1048576},{2293760,3604480,4259840,4915200,5242880},65536,0,0,65536,65536,786432,0,{0},{0}},
   {2,0,158,-1,11,5,130,5,1,11,1,0,11,0,1,0,0,128,0,10,0,-1,0,32768,65536,{0},0,327680,458752,{131072,262144,393216,458752,524288},{2293760,3604480,4259840,5570560,6225920},65536,0,0,655360,262144,655360,3276800,{255},{0}},
   {2,0,159,-1,11,5,26,5,1,11,1,0,11,1,1,1,0,128,1,0,0,-1,0,13107,65536,{0},0,327680,458752,{655360,983040,1310720,1638400,1966080},{0,0,0,0,0},65536,2162,0,65536,0,2293760,2621440,{0},{0}},
   {-1,0,-1,-1,11,95,230,95,1,11,2,0,-1,0,1,0,0,128,0,0,0,-1,0,13107,65536,{259},32768,131072,229376,{393216,524288,655360,851968,983040},{6553600,16384000,26214400,26214400,26214400},13107,0,0,655360,0,196608,458752,{279},{0}},
   {3,0,-1,-1,11,5,26,5,1,11,1,80,11,1,0,1,0,128,0,0,0,49,0,13107,65536,{0},196608,327680,458752,{327680,655360,983040,1310720,1966080},{0,0,0,0,0},65536,2162,0,655360,0,4915200,655360,{293},{0}},
   {2,0,160,-1,11,104,132,5,1,231,1,0,231,0,1,0,1,128,0,0,32,-1,0,65536,65536,{0},0,327680,1966080,{6553600,6553600,6553600,6553600,6553600},{18677760,18677760,18677760,18677760,18677760},65536,0,0,655360,524288,655360,5242880,{274},{0}},
   {2,0,161,-1,11,5,130,5,1,11,1,0,11,0,1,0,0,128,0,0,16,-1,0,49152,65536,{0},0,327680,655360,{983040,1310720,1638400,1966080,2293760},{3276800,4587520,6553600,9830400,13107200},78643,0,0,655360,262144,655360,1638400,{255},{0}},
   {3,0,-1,-1,11,5,12,2,1,11,1,92,11,0,0,0,1,128,0,0,0,-1,0,98304,65536,{0},131072,327680,196608,{983040,1310720,1638400,1966080,2293760},{2621440,3276800,3932160,4587520,5898240},65536,0,0,65536,65536,786432,0,{0},{0}},
   {2,0,162,-1,11,5,130,5,1,11,1,0,11,0,1,0,0,128,0,0,0,29,0,32768,65536,{0},655360,327680,458752,{589824,851968,1179648,1507328,1966080},{2949120,3932160,5242880,5898240,6225920},65536,0,0,65536,524288,262144,1310720,{271},{272}},
   {2,0,163,-1,11,104,132,5,1,231,1,0,231,0,1,0,0,128,0,0,32,59,0,98304,65536,{0},0,327680,3276800,{3932160,4587520,5570560,6553600,7208960},{3276800,4587520,5898240,7208960,7864320},65536,0,0,655360,524288,655360,5242880,{298},{299}},
   {2,0,164,-1,11,104,132,5,1,231,1,0,231,0,1,0,1,128,0,0,32,-1,0,65536,65536,{0},0,327680,1966080,{589824,983040,1310720,1769472,1966080},{2621440,3932160,5242880,6553600,7864320},65536,0,0,655360,524288,655360,3276800,{274},{0}},
   {3,0,-1,-1,86,105,38,105,1,11,0,103,11,1,0,0,1,128,0,0,0,-1,327680,65536,65536,{0},58982,81920,262144,{327680,458752,655360,786432,917504},{6553600,9830400,11141120,12451840,13762560},655,0,0,655360,131072,376832,0,{285},{286}},
   {2,0,165,-1,11,5,130,5,1,11,1,0,11,0,1,0,1,128,0,10,0,-1,0,32768,65536,{0},0,327680,458752,{458752,589824,655360,720896,786432},{3604480,4915200,5570560,6225920,6881280},65536,0,0,655360,262144,655360,5242880,{255},{0}}
};

//-----------------------------------------------------------------------------

uint8_t Sounds [2][MAX_SOUNDS];
uint8_t AltSounds [2][MAX_SOUNDS];

//right now there's only one CPlayerData ship, but we can have another by
//adding an array and setting the pointer to the active ship.

//---------------- Variables for CObject textures ----------------

/*
 * reads n tTexMapInfo structs from a CFile
 */
int32_t ReadTMapInfoN (CArray<tTexMapInfo>& ti, int32_t n, CFile& cf)
{
	int32_t i;

for (i = 0;i < n;i++) {
	ti [i].flags = cf.ReadByte ();
	ti [i].pad [0] = cf.ReadByte ();
	ti [i].pad [1] = cf.ReadByte ();
	ti [i].pad [2] = cf.ReadByte ();
	ti [i].lighting = cf.ReadFix ();
	ti [i].damage = cf.ReadFix ();
	ti [i].nEffectClip = cf.ReadShort ();
	ti [i].destroyed = cf.ReadShort ();
	ti [i].slide_u = cf.ReadShort ();
	ti [i].slide_v = cf.ReadShort ();
	}
return i;
}

//------------------------------------------------------------------------------

int32_t ReadTMapInfoND1 (tTexMapInfo *ti, int32_t n, CFile& cf)
{
	int32_t i;

for (i = 0;i < n;i++) {
	cf.Seek (13, SEEK_CUR);// skip filename
	ti [i].flags = cf.ReadByte ();
	ti [i].lighting = cf.ReadFix ();
	ti [i].damage = cf.ReadFix ();
	ti [i].nEffectClip = cf.ReadInt ();
	}
return i;
}

//------------------------------------------------------------------------------
// Read data from piggy.
// This is called when the editor is OUT.
// If editor is in, bm_init_use_table () is called.
int32_t BMInit (void)
{
if (!PiggyInit ())				// This calls BMReadAll
	Error ("Cannot open pig and/or ham file");
/*---*/PrintLog (1, "Initializing endlevel data\n");
InitEndLevel ();		//this is in bm_init_use_tbl (), so I gues it goes here
PrintLog (-1);
return 0;
}

//------------------------------------------------------------------------------

void BMSetAfterburnerSizes (void)
{
	int8_t	nSize = gameData.weapons.info [MERCURYMSL_ID].nAfterburnerSize;

//gameData.weapons.info [VULCAN_ID].nAfterburnerSize = 
//gameData.weapons.info [GAUSS_ID].nAfterburnerSize = nSize / 8;
gameData.weapons.info [CONCUSSION_ID].nAfterburnerSize =
gameData.weapons.info [HOMINGMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_CONCUSSION_ID].nAfterburnerSize =
gameData.weapons.info [FLASHMSL_ID].nAfterburnerSize =
gameData.weapons.info [GUIDEDMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_FLASHMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_MEGA_FLASHMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_MERCURYMSL_ID].nAfterburnerSize = nSize;
gameData.weapons.info [ROBOT_HOMINGMSL_ID].nAfterburnerSize =
gameData.weapons.info [SMARTMSL_ID].nAfterburnerSize = 2 * nSize;
gameData.weapons.info [MEGAMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_MEGAMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_SHAKER_MEGA_ID].nAfterburnerSize =
gameData.weapons.info [EARTHSHAKER_MEGA_ID].nAfterburnerSize = 3 * nSize;
gameData.weapons.info [EARTHSHAKER_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_EARTHSHAKER_ID].nAfterburnerSize = 4 * nSize;
}

//------------------------------------------------------------------------------

void QSortTextureIndex (int16_t *pti, int16_t left, int16_t right)
{
	int16_t	l = left,
			r = right,
			m = pti [(left + right) / 2],
			h;

do {
	while (pti [l] < m)
		l++;
	while (pti [r] > m)
		r--;
	if (l <= r) {
		if (l < r) {
			h = pti [l];
			pti [l] = pti [r];
			pti [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortTextureIndex (pti, l, right);
if (left < r)
	QSortTextureIndex (pti, left, r);
}

//------------------------------------------------------------------------------

void BuildTextureIndex (int32_t i, int32_t n)
{
	int16_t			*pti = gameData.pig.tex.textureIndex [i].Buffer (); // translates global texture ids to level (geometry) texture ids
	tBitmapIndex	*pbi = gameData.pig.tex.bmIndex [i].Buffer ();  // translates level texture ids to global texture ids

gameData.pig.tex.textureIndex [i].Clear (0xff);
for (i = 0; i < n; i++)
	pti [pbi [i].index] = i;
//QSortTextureIndex (pti, 0, n - 1);
}

//------------------------------------------------------------------------------

void BMReadAll (CFile& cf, bool bDefault)
{
	int32_t i, t;

gameData.pig.tex.nTextures [0] = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d texture indices\n", gameData.pig.tex.nTextures [0]);
ReadBitmapIndices (gameData.pig.tex.bmIndex [0], gameData.pig.tex.nTextures [0], cf);
BuildTextureIndex (0, gameData.pig.tex.nTextures [0]);
ReadTMapInfoN (gameData.pig.tex.tMapInfo [0], gameData.pig.tex.nTextures [0], cf);
PrintLog (-1);

t = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d sound indices\n", t);
cf.Read (Sounds [0], sizeof (uint8_t), t);
cf.Read (AltSounds [0], sizeof (uint8_t), t);
PrintLog (-1);

gameData.effects.nClips [0] = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d animation clips\n", gameData.effects.nClips [0]);
ReadAnimationInfo (gameData.effects.animations [0], gameData.effects.nClips [0], cf);
PrintLog (-1);

gameData.effects.nEffects [0] = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d animation descriptions\n", gameData.effects.nEffects [0]);
ReadEffectInfo (gameData.effects.effects [0], gameData.effects.nEffects [0], cf);
// red glow texture animates way too fast
gameData.effects.effects [0][32].animationInfo.xTotalTime *= 10;
gameData.effects.effects [0][32].animationInfo.xFrameTime *= 10;
gameData.wallData.nAnims [0] = cf.ReadInt ();
PrintLog (-1);
/*---*/PrintLog (1, "Loading %d CWall animations\n", gameData.wallData.nAnims [0]);
ReadWallEffectInfo (gameData.wallData.anims [0], gameData.wallData.nAnims [0], cf);
PrintLog (-1);

gameData.botData.nTypes [0] = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d robot descriptions\n", gameData.botData.nTypes [0]);
ReadRobotInfos (gameData.botData.info [0], gameData.botData.nTypes [0], cf);
gameData.botData.nDefaultTypes = gameData.botData.nTypes [0];
gameData.botData.defaultInfo = gameData.botData.info [0];
PrintLog (-1);

gameData.botData.nJoints = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d robot joint descriptions\n", gameData.botData.nJoints);
ReadJointPositions (gameData.botData.joints, gameData.botData.nJoints, cf);
gameData.botData.nDefaultJoints = gameData.botData.nJoints;
gameData.botData.defaultJoints = gameData.botData.joints;
PrintLog (-1);

gameData.weapons.nTypes [0] = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d weapon descriptions\n", gameData.weapons.nTypes [0]);
ReadWeaponInfos (0, gameData.weapons.nTypes [0], cf, gameData.pig.tex.nHamFileVersion, bDefault);
gameData.weapons.info [48].light = I2X (1); // fix light for BPer shots and smart missile blobs - don't make them too bright though
BMSetAfterburnerSizes ();
PrintLog (-1);

gameData.objData.pwrUp.nTypes = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d powerup descriptions\n", gameData.objData.pwrUp.nTypes);
ReadPowerupTypeInfos (gameData.objData.pwrUp.info.Buffer (), gameData.objData.pwrUp.nTypes, cf);
PrintLog (-1);

gameData.models.nPolyModels = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d CPolyModel descriptions\n", gameData.models.nPolyModels);
ReadPolyModels (gameData.models.polyModels [0], gameData.models.nPolyModels, cf);
if (bDefault) {
	gameData.models.nDefPolyModels = gameData.models.nPolyModels;
	memcpy (gameData.models.polyModels [1].Buffer (), gameData.models.polyModels [0].Buffer (), gameData.models.nPolyModels * sizeof (CPolyModel));
	}
PrintLog (-1);

/*---*/PrintLog (1, "Loading poly model data\n");
for (i = 0; i < gameData.models.nPolyModels; i++) {
	gameData.models.polyModels [0][i].ResetBuffer ();
	if (bDefault)
		gameData.models.polyModels [1][i].ResetBuffer ();
	gameData.models.polyModels [0][i].SetCustom (!bDefault);
	gameData.models.polyModels [0][i].ReadData (bDefault ? gameData.models.polyModels [1] + i : NULL, cf);
	}

for (i = 0; i < gameData.models.nPolyModels; i++)
	gameData.models.nDyingModels [i] = cf.ReadInt ();
for (i = 0; i < gameData.models.nPolyModels; i++)
	gameData.models.nDeadModels [i] = cf.ReadInt ();
PrintLog (-1);

t = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d cockpit gauges\n", t);
ReadBitmapIndices (gameData.cockpit.gauges [1], t, cf);
ReadBitmapIndices (gameData.cockpit.gauges [0], t, cf);
PrintLog (-1);

gameData.pig.tex.nObjBitmaps = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d CObject bitmap indices\n", gameData.pig.tex.nObjBitmaps);
ReadBitmapIndices (gameData.pig.tex.objBmIndex, gameData.pig.tex.nObjBitmaps, cf);
gameData.pig.tex.defaultObjBmIndex = gameData.pig.tex.objBmIndex;
for (i = 0; i < gameData.pig.tex.nObjBitmaps; i++)
	gameData.pig.tex.pObjBmIndex [i] = cf.ReadShort ();
PrintLog (-1);

/*---*/PrintLog (1, "Loading CPlayerData ship description\n");
PlayerShipRead (&gameData.pig.ship.only, cf);
PrintLog (-1);

gameData.models.nCockpits = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d cockpit bitmaps\n", gameData.models.nCockpits);
ReadBitmapIndices (gameData.pig.tex.cockpitBmIndex, gameData.models.nCockpits, cf);
gameData.pig.tex.nFirstMultiBitmap = cf.ReadInt ();
PrintLog (-1);

gameData.reactor.nReactors = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d reactor descriptions\n", gameData.reactor.nReactors);
ReadReactors (cf);
PrintLog (-1);

gameData.models.nMarkerModel = cf.ReadInt ();
if (gameData.pig.tex.nHamFileVersion < 3) {
	gameData.endLevel.exit.nModel = cf.ReadInt ();
	gameData.endLevel.exit.nDestroyedModel = cf.ReadInt ();
	}
else
	gameData.endLevel.exit.nModel = 
	gameData.endLevel.exit.nDestroyedModel = gameData.models.nPolyModels;
}

//------------------------------------------------------------------------------

void BMReadWeaponInfoD1N (CFile& cf, int32_t i)
{
	CD1WeaponInfo& wi = gameData.weapons.infoD1 [i];

wi.renderType = cf.ReadByte ();
wi.nModel = cf.ReadByte ();
wi.nInnerModel = cf.ReadByte ();
wi.persistent = cf.ReadByte ();
wi.nFlashAnimation = cf.ReadByte ();
wi.flashSound = cf.ReadShort ();
wi.nRobotHitAnimation = cf.ReadByte ();
wi.nRobotHitSound = cf.ReadShort ();
wi.nWallHitAnimation = cf.ReadByte ();
wi.nWallHitSound = cf.ReadShort ();
wi.fireCount = cf.ReadByte ();
wi.nAmmoUsage = cf.ReadByte ();
wi.nAnimationIndex = cf.ReadByte ();
wi.destructible = cf.ReadByte ();
wi.matter = cf.ReadByte ();
wi.bounce = cf.ReadByte ();
wi.homingFlag = cf.ReadByte ();
wi.dum1 = cf.ReadByte (); 
wi.dum2 = cf.ReadByte ();
wi.dum3 = cf.ReadByte ();
wi.xEnergyUsage = cf.ReadFix ();
wi.xFireWait = cf.ReadFix ();
wi.bitmap.index = cf.ReadShort ();
wi.blob_size = cf.ReadFix ();
wi.xFlashSize = cf.ReadFix ();
wi.xImpactSize = cf.ReadFix ();
for (i = 0; i < DIFFICULTY_LEVEL_COUNT; i++)
	wi.strength [i] = cf.ReadFix ();
for (i = 0; i < DIFFICULTY_LEVEL_COUNT; i++)
	wi.speed [i] = cf.ReadFix ();
wi.mass = cf.ReadFix ();
wi.drag = cf.ReadFix ();
wi.thrust = cf.ReadFix ();
wi.poLenToWidthRatio = cf.ReadFix ();
wi.light = cf.ReadFix ();
wi.lifetime = cf.ReadFix ();
wi.xDamageRadius = cf.ReadFix ();
wi.picture.index = cf.ReadShort ();

#if PRINT_WEAPON_INFO
PrintLog (0, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,{",
	wi.renderType,
	wi.nModel,
	wi.nInnerModel,
	wi.persistent,
	wi.nFlashAnimation,
	wi.flashSound,
	wi.nRobotHitAnimation,
	wi.nRobotHitSound,
	wi.nWallHitAnimation,
	wi.nWallHitSound,
	wi.fireCount,
	wi.nAmmoUsage,
	wi.nAnimationIndex,
	wi.destructible,
	wi.matter,
	wi.bounce,
	wi.homingFlag,
	wi.dum1, 
	wi.dum2,
	wi.dum3,
	wi.xEnergyUsage,
	wi.xFireWait,
	wi.bitmap.index,
	wi.blob_size,
	wi.xFlashSize,
	wi.xImpactSize);
for (i = 0; i < DIFFICULTY_LEVEL_COUNT; i++)
	PrintLog (0, "%s%d", i ? "," : "", wi.strength [i]);
PrintLog (1, "},{");
for (i = 0; i < DIFFICULTY_LEVEL_COUNT; i++)
	PrintLog (0, "%s%d", i ? "," : "", wi.speed [i]);
PrintLog (0, "},%d,%d,%d,%d,%d,%d,%d,{%d}},\n",
	wi.mass,
	wi.drag,
	wi.thrust,
	wi.poLenToWidthRatio,
	wi.light,
	wi.lifetime,
	wi.xDamageRadius,
	wi.picture.index);
#endif
}

//------------------------------------------------------------------------------
// the following values are needed for compiler settings causing struct members 
// not to be byte aligned. If D2X-XL is compiled with such settings, the size of
// the Descent data structures in memory is bigger than on disk. This needs to
// be compensated when reading such data structures from disk, or needing to skip
// them in the disk files.

#define D1_ROBOT_INFO_SIZE			486
#define D1_WEAPON_INFO_SIZE		115
#define JOINTPOS_SIZE				8
#define JOINTLIST_SIZE				4
#define POWERUP_TYPE_INFO_SIZE	16
#define POLYMODEL_SIZE				734
#define PLAYER_SHIP_SIZE			132
#define MODEL_DATA_SIZE_OFFS		4


typedef struct tD1TextureHeader {
	char	name [8];
	uint8_t frame; //bits 0-5 anim frame num, bit 6 abm flag
	uint8_t xsize; //low 8 bits here, 4 more bits in size2
	uint8_t ysize; //low 8 bits here, 4 more bits in size2
	uint8_t flag; //see BM_FLAG_XXX
	uint8_t ave_color; //palette index of average color
	uint32_t	offset; //relative to end of directory
} tD1TextureHeader;

typedef struct tD1SoundHeader {
	char name [8];
	int32_t length; //size in bytes
	int32_t data_length; //actually the same as above
	int32_t offset; //relative to end of directory
} tD1SoundHeader;


void BMReadGameDataD1 (CFile& cf)
{
	int32_t				h, i, j;
#if 1
	tTexMapInfoD1	t; // only needed for sizeof term below
	//D1Robot_info	r;
#endif
	tWallEffect		*pw;	
	tTexMapInfo		*pt;
	tRobotInfo		*pr;
	CPolyModel		model;
	uint8_t			tmpSounds [D1_MAX_SOUNDS];

cf.ReadInt ();
cf.Read (&gameData.pig.tex.nTextures [1], sizeof (int32_t), 1);
j = (gameData.pig.tex.nTextures [1] == 70) ? 70 : D1_MAX_TEXTURES;
/*---*/PrintLog (1, "Loading %d texture indices\n", j);
//cf.Read (gameData.pig.tex.bmIndex [1], sizeof (tBitmapIndex), D1_MAX_TEXTURES);
ReadBitmapIndices (gameData.pig.tex.bmIndex [1], D1_MAX_TEXTURES, cf);
BuildTextureIndex (1, D1_MAX_TEXTURES);
PrintLog (-1);

/*---*/PrintLog (1, "Loading %d texture descriptions\n", j);
for (i = 0, pt = &gameData.pig.tex.tMapInfo [1][0]; i < j; i++, pt++) {
#if DBG
	cf.Read (t.filename, sizeof (t.filename), 1);
#else
	cf.Seek (sizeof (t.filename), SEEK_CUR);
#endif
	pt->flags = (uint8_t) cf.ReadByte ();
	pt->lighting = cf.ReadFix ();
	pt->damage = cf.ReadFix ();
	pt->nEffectClip = cf.ReadInt ();
	pt->slide_u = 
	pt->slide_v = 0;
	pt->destroyed = -1;
	}
PrintLog (-1);

cf.Read (Sounds [1], sizeof (uint8_t), D1_MAX_SOUNDS);
cf.Read (AltSounds [1], sizeof (uint8_t), D1_MAX_SOUNDS);

/*---*/PrintLog (1, "Initializing %d sounds\n", D1_MAX_SOUNDS);
if (gameOpts->sound.bUseD1Sounds) {
	memcpy (Sounds [1] + D1_MAX_SOUNDS, Sounds [0] + D1_MAX_SOUNDS, MAX_SOUNDS - D1_MAX_SOUNDS);
	memcpy (AltSounds [1] + D1_MAX_SOUNDS, AltSounds [0] + D1_MAX_SOUNDS, MAX_SOUNDS - D1_MAX_SOUNDS);
	}
else {
	memcpy (Sounds [1], Sounds [0], MAX_SOUNDS);
	memcpy (AltSounds [1], AltSounds [0], MAX_SOUNDS);
	}
for (i = 0; i < D1_MAX_SOUNDS; i++) {
	if (Sounds [1][i] == 255)
		Sounds [1][i] = Sounds [0][i];
	if (AltSounds [1][i] == 255)
		AltSounds [1][i] = AltSounds [0][i];
	}
PrintLog (-1);

gameData.effects.nClips [1] = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d animation clips\n", gameData.effects.nClips [1]);
ReadAnimationInfo (gameData.effects.animations [1], MAX_ANIMATIONS_D1, cf);
PrintLog (-1);

gameData.effects.nEffects [1] = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d animation descriptions\n", gameData.effects.nEffects [1]);
ReadEffectInfo (gameData.effects.effects [1], D1_MAX_EFFECTS, cf);
PrintLog (-1);

gameData.wallData.nAnims [1] = cf.ReadInt ();
/*---*/PrintLog (1, "Loading %d wall animations\n", gameData.wallData.nAnims [1]);
for (i = 0, pw = &gameData.wallData.anims [1][0]; i < D1_MAX_WALL_ANIMS; i++, pw++) {
	//cf.Read (&w, sizeof (w), 1);
	pw->xTotalTime = cf.ReadFix ();
	pw->nFrameCount = cf.ReadShort ();
	for (j = 0; j < MAX_WALL_EFFECT_FRAMES_D1; j++)
		pw->frames [j] = cf.ReadShort ();
	pw->openSound = cf.ReadShort ();
	pw->closeSound = cf.ReadShort ();
	pw->flags = cf.ReadShort ();
	cf.Read (pw->filename, sizeof (pw->filename), 1);
	pw->pad = (char) cf.ReadByte ();
	}
PrintLog (-1);

/*---*/PrintLog (1, "Loading %d robot descriptions\n", gameData.botData.nTypes [1]);
cf.Read (&gameData.botData.nTypes [1], sizeof (int32_t), 1);
PrintLog (-1);
gameData.botData.info [1] = gameData.botData.info [0];
if (!gameOpts->sound.bUseD1Sounds) {
	PrintLog (-1);
	return;
	}

for (i = 0, pr = &gameData.botData.info [1][0]; i < D1_MAX_ROBOT_TYPES; i++, pr++) {
	//cf.Read (&r, sizeof (r), 1);
	cf.Seek (
		sizeof (int32_t) * 3 + 
		(sizeof (CFixVector) + sizeof (uint8_t)) * MAX_GUNS + 
		sizeof (int16_t) * 5 +
		sizeof (int8_t) * 7 +
		sizeof (fix) * 4 +
		sizeof (fix) * 7 * DIFFICULTY_LEVEL_COUNT +
		sizeof (int8_t) * 2 * DIFFICULTY_LEVEL_COUNT,
		SEEK_CUR);

	pr->seeSound = (uint8_t) cf.ReadByte ();
	pr->attackSound = (uint8_t) cf.ReadByte ();
	pr->clawSound = (uint8_t) cf.ReadByte ();
	cf.Seek (
		JOINTLIST_SIZE * (MAX_GUNS + 1) * N_ANIM_STATES +
		sizeof (int32_t),
		SEEK_CUR);
	pr->always_0xabcd = 0xabcd;   
	}         

cf.Seek (
	sizeof (int32_t) +
	JOINTPOS_SIZE * D1_MAX_ROBOT_JOINTS +
	sizeof (int32_t) +
	D1_WEAPON_INFO_SIZE * D1_MAX_WEAPON_TYPES +
	sizeof (int32_t) +
	POWERUP_TYPE_INFO_SIZE * MAX_POWERUP_TYPES_D1,
	SEEK_CUR);

i = cf.ReadInt ();
/*---*/PrintLog (1, "Acquiring model data size of %d polymodels\n", i);
for (h = 0; i; i--) {
	cf.Seek (MODEL_DATA_SIZE_OFFS, SEEK_CUR);
	model.SetDataSize (cf.ReadInt ());
	h += model.DataSize ();
	cf.Seek (POLYMODEL_SIZE - MODEL_DATA_SIZE_OFFS - sizeof (int32_t), SEEK_CUR);
	}
cf.Seek (
	h +
	sizeof (tBitmapIndex) * D1_MAX_GAUGE_BMS +
	sizeof (int32_t) * 2 * D1_MAX_POLYGON_MODELS +
	sizeof (tBitmapIndex) * D1_MAX_OBJ_BITMAPS +
	sizeof (uint16_t) * D1_MAX_OBJ_BITMAPS +
	PLAYER_SHIP_SIZE +
	sizeof (int32_t) +
	sizeof (tBitmapIndex) * D1_N_COCKPIT_BITMAPS,
	SEEK_CUR);
PrintLog (-1);

/*---*/PrintLog (1, "Loading sound data\n", i);
cf.Read (tmpSounds, sizeof (uint8_t), D1_MAX_SOUNDS);
PrintLog (-1);

//for (i = 0, pr = &gameData.botData.info [1][0]; i < gameData.botData.nTypes [1]; i++, pr++) 
pr = gameData.botData.info [1] + 17;
/*---*/PrintLog (1, "Initializing sound data\n", i);
for (i = 0; i < D1_MAX_SOUNDS; i++) {
	if (Sounds [1][i] == tmpSounds [pr->seeSound])
		pr->seeSound = i;
	if (Sounds [1][i] == tmpSounds [pr->attackSound])
		pr->attackSound = i;
	if (Sounds [1][i] == tmpSounds [pr->clawSound])
		pr->clawSound = i;
	}
pr = gameData.botData.info [1] + 23;
for (i = 0; i < D1_MAX_SOUNDS; i++) {
	if (Sounds [1][i] == tmpSounds [pr->seeSound])
		pr->seeSound = i;
	if (Sounds [1][i] == tmpSounds [pr->attackSound])
		pr->attackSound = i;
	if (Sounds [1][i] == tmpSounds [pr->clawSound])
		pr->clawSound = i;
	}
cf.Read (tmpSounds, sizeof (uint8_t), D1_MAX_SOUNDS);
//	for (i = 0, pr = &gameData.botData.info [1][0]; i < gameData.botData.nTypes [1]; i++, pr++) {
pr = gameData.botData.info [1] + 17;
for (i = 0; i < D1_MAX_SOUNDS; i++) {
	if (AltSounds [1][i] == tmpSounds [pr->seeSound])
		pr->seeSound = i;
	if (AltSounds [1][i] == tmpSounds [pr->attackSound])
		pr->attackSound = i;
	if (AltSounds [1][i] == tmpSounds [pr->clawSound])
		pr->clawSound = i;
	}
pr = gameData.botData.info [1] + 23;
for (i = 0; i < D1_MAX_SOUNDS; i++) {
	if (AltSounds [1][i] == tmpSounds [pr->seeSound])
		pr->seeSound = i;
	if (AltSounds [1][i] == tmpSounds [pr->attackSound])
		pr->attackSound = i;
	if (AltSounds [1][i] == tmpSounds [pr->clawSound])
		pr->clawSound = i;
	}
PrintLog (-1);
}

//------------------------------------------------------------------------------

void BMReadWeaponInfoD1 (CFile& cf)
{
cf.Seek (
	sizeof (int32_t) +
	sizeof (int32_t) +
	sizeof (tBitmapIndex) * D1_MAX_TEXTURES +
	sizeof (tTexMapInfoD1) * D1_MAX_TEXTURES +
	sizeof (uint8_t) * D1_MAX_SOUNDS +
	sizeof (uint8_t) * D1_MAX_SOUNDS +
	sizeof (int32_t) +
	sizeof (tAnimationInfo) * MAX_ANIMATIONS_D1 +
	sizeof (int32_t) +
	sizeof (D1_eclip) * D1_MAX_EFFECTS +
	sizeof (int32_t) +
	sizeof (tWallEffectD1) * D1_MAX_WALL_ANIMS +
	sizeof (int32_t) +
	sizeof (D1Robot_info) * D1_MAX_ROBOT_TYPES +
	sizeof (int32_t) +
	sizeof (tJointPos) * D1_MAX_ROBOT_JOINTS,
	SEEK_CUR);
gameData.weapons.nTypes [1] = cf.ReadInt ();
#if PRINT_WEAPON_INFO
PrintLog (1, "\nCD1WeaponInfo defaultWeaponInfosD1 [] = {\n");
#endif
for (int32_t i = 0; i < gameData.weapons.nTypes [1]; i++)
	BMReadWeaponInfoD1N (cf, i);
#if PRINT_WEAPON_INFO
PrintLog (-1, "};\n\n");
#endif
}

//------------------------------------------------------------------------------

void LoadTextureBrightness (const char *pszLevel, int32_t *brightnessP)
{
	CFile		cf;
	char		szFile [FILENAME_LEN];
	int32_t		i, *pb;

if (!brightnessP)
	brightnessP = gameData.pig.tex.brightness.Buffer ();
CFile::ChangeFilenameExtension (szFile, pszLevel, ".lgt");
if (cf.Open (szFile, gameFolders.game.szData [0], "rb", 0) &&
	 (cf.Read (brightnessP, sizeof (*brightnessP) * MAX_WALL_TEXTURES, 1) == 1)) {
	for (i = MAX_WALL_TEXTURES, pb = gameData.pig.tex.brightness.Buffer (); i; i--, pb++)
		*pb = INTEL_INT (*pb);
	cf.Close ();
	}
}

//-----------------------------------------------------------------------------

void InitDefaultShipProps (void)
{
defaultShipProps.nModel = 108;
defaultShipProps.nExplVClip = 58;
defaultShipProps.mass = 262144;
defaultShipProps.drag = 2162;
defaultShipProps.maxThrust = 511180;
defaultShipProps.reverseThrust = 0;
defaultShipProps.brakes = 0;
defaultShipProps.wiggle = I2X (1) / 2;
defaultShipProps.maxRotThrust = 9175;
defaultShipProps.gunPoints [0].Create (146013, -59748, 35756);
defaultShipProps.gunPoints [0].Create (-147477, -59892, 34430);
defaultShipProps.gunPoints [0].Create (222008, -118473, 148201);
defaultShipProps.gunPoints [0].Create (-223479, -118213, 148302);
defaultShipProps.gunPoints [0].Create (153026, -185, -91405);
defaultShipProps.gunPoints [0].Create (-156840, -185, -91405);
defaultShipProps.gunPoints [0].Create (1608, -87663, 184978);
defaultShipProps.gunPoints [0].Create (-1608, -87663, -190825);
}

//-----------------------------------------------------------------------------

void SetDefaultShipProps (void)
{
gameData.pig.ship.player->brakes = defaultShipProps.brakes;
gameData.pig.ship.player->drag = defaultShipProps.drag;
gameData.pig.ship.player->mass = defaultShipProps.mass;
gameData.pig.ship.player->maxThrust = defaultShipProps.maxThrust;
gameData.pig.ship.player->maxRotThrust = defaultShipProps.maxRotThrust;
gameData.pig.ship.player->reverseThrust = defaultShipProps.reverseThrust;
gameData.pig.ship.player->brakes = defaultShipProps.brakes;
gameData.pig.ship.player->wiggle = defaultShipProps.wiggle;
}

//-----------------------------------------------------------------------------

void SetDefaultWeaponProps (void)
{
if (!EGI_FLAG (bAllowCustomWeapons, 0, 0, 1)) {
	for (int32_t i = 0; i < int32_t (sizeofa (defaultWeaponInfoD2)); i++)
		gameData.weapons.info [i] = defaultWeaponInfoD2 [i];
	}
}

//------------------------------------------------------------------------------
//eof
