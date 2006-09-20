/* $Id: paging.c,v 1.3 2003/10/04 03:14:47 btb Exp $ */
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

/*
 *
 * Routines for paging in/out textures.
 *
 * Old Log:
 * Revision 1.5  1995/10/30  11:06:58  allender
 * added change to paging code ala John -- check tmap_override
 * when paging in robots
 *
 * Revision 1.4  1995/09/13  08:48:28  allender
 * John's new paging code
 *
 * Revision 1.3  1995/08/18  10:20:31  allender
 * changed hard coded black pixel value to use BM_XRGB
 *
 * Revision 1.2  1995/07/26  17:02:10  allender
 * small fix to page in effect bitmaps correctly
 *
 * Revision 1.1  1995/05/16  15:29:35  allender
 * Initial revision
 *
 * Revision 2.5  1995/10/07  13:18:21  john
 * Added PSX debugging stuff that builds .PAG files.
 *
 * Revision 2.4  1995/08/24  13:40:03  john
 * Added code to page in vclip for powerup disapperance and to
 * fix bug that made robot makers not page in the correct bot
 * textures.
 *
 * Revision 2.3  1995/07/26  12:09:19  john
 * Made code that pages in weapon_info->robot_hit_vclip not
 * page in unless it is a badass weapon.  Took out old functionallity
 * of using this if no robot exp1_vclip, since all robots have these.
 *
 * Revision 2.2  1995/07/24  13:22:11  john
 * Made sure everything gets paged in at the
 * level start.  Fixed bug with robot effects not
 * getting paged in correctly.
 *
 * Revision 2.1  1995/05/12  15:50:16  allender
 * fix to check effects dest_bm_num > -1 before paging in
 *
 * Revision 2.0  1995/02/27  11:27:39  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.18  1995/02/22  14:12:28  allender
 * remove anonyous union from object structure
 *
 * Revision 1.17  1995/02/11  22:54:15  john
 * Made loading for pig not show up for demos.
 *
 * Revision 1.16  1995/02/11  22:37:04  john
 * Made cockpit redraw.
 *
 * Revision 1.15  1995/01/28  16:29:35  john
 * *** empty log message ***
 *
 * Revision 1.14  1995/01/27  17:16:18  john
 * Added code to page in all the weapons.
 *
 * Revision 1.13  1995/01/24  21:51:22  matt
 * Clear the boxed message to fix a mem leakage
 *
 * Revision 1.12  1995/01/23  13:00:46  john
 * Added hostage vclip paging.
 *
 * Revision 1.11  1995/01/23  12:29:52  john
 * Added code to page in eclip on robots, dead control center,
 * gauges bitmaps, and weapon pictures.
 *
 * Revision 1.10  1995/01/21  12:54:15  adam
 * *** empty log message ***
 *
 * Revision 1.9  1995/01/21  12:41:29  adam
 * changed orb to loading box
 *
 * Revision 1.8  1995/01/18  15:09:02  john
 * Added start/stop time around paging.
 * Made paging clear screen around globe.
 *
 * Revision 1.7  1995/01/18  10:37:00  john
 * Added code to page in exploding monitors.
 *
 * Revision 1.6  1995/01/17  19:03:35  john
 * Added cool spinning orb during loading.
 *
 * Revision 1.5  1995/01/17  14:49:26  john
 * Paged in weapons.
 *
 * Revision 1.4  1995/01/17  12:14:07  john
 * Made walls, object explosion vclips load at level start.
 *
 * Revision 1.3  1995/01/15  13:23:24  john
 * First working version
 *
 * Revision 1.2  1995/01/15  11:56:45  john
 * Working version of paging.
 *
 * Revision 1.1  1995/01/15  11:33:37  john
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

//#define PSX_BUILD_TOOLS

#ifdef RCS
static char rcsid[] = "$Id: paging.c,v 1.3 2003/10/04 03:14:47 btb Exp $";
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "error.h"
#include "gameseg.h"
#include "game.h"
#include "piggy.h"
#include "texmerge.h"
#include "polyobj.h"
#include "vclip.h"
#include "effects.h"
#include "fireball.h"
#include "weapon.h"
#include "palette.h"
#include "timer.h"
#include "text.h"
#include "cntrlcen.h"
#include "gauges.h"
#include "powerup.h"
#include "fuelcen.h"
#include "mission.h"
#include "menu.h"
#include "newmenu.h"
#include "gamesave.h"
#include "gamepal.h"

//------------------------------------------------------------------------------

#ifdef WINDOWS
void PagingTouchVClipW( vclip * vc )
{
	int i;

	for (i=0; i<vc->nFrameCount; i++ )	{
		if (!(vc->flags & EF_ALTFMT) && 
			 (gameData.pig.tex.bitmaps[(vc->frames[i]).index].bm_props.flags & BM_FLAG_PAGED_OUT))
			PiggyBitmapPageInW( vc->frames[i],1 );
	}
}
#endif

//------------------------------------------------------------------------------

void PagingTouchVClip( vclip * vc, int bD1 )
{
	int i;

	for (i=0; i<vc->nFrameCount; i++ )	{
		PIGGY_PAGE_IN( vc->frames[i], bD1 );
	}
}

//------------------------------------------------------------------------------

void PagingTouchWallEffects( int tmap_num )
{
	int i, j;
	eclip *ecP = gameData.eff.pEffects;

	for (i=0, j = gameData.eff.nEffects [gameStates.app.bD1Data];i<j;i++, ecP++)	{
		if ( ecP->changing_wall_texture == tmap_num )	{
			PagingTouchVClip( &ecP->vc, gameStates.app.bD1Data );

			if (ecP->dest_bm_num > -1)
				PIGGY_PAGE_IN( gameData.pig.tex.pBmIndex[ecP->dest_bm_num], gameStates.app.bD1Data );	//use this bitmap when monitor destroyed
			if ( ecP->dest_vclip > -1 )
				PagingTouchVClip( &gameData.eff.pVClips[ecP->dest_vclip], gameStates.app.bD1Data );		  //what vclip to play when exploding

			if ( ecP->dest_eclip > -1 )
				PagingTouchVClip( &gameData.eff.pEffects[ecP->dest_eclip].vc, gameStates.app.bD1Data ); //what eclip to play when exploding

			if ( ecP->crit_clip > -1 )
				PagingTouchVClip( &gameData.eff.pEffects[ecP->crit_clip].vc, gameStates.app.bD1Data ); //what eclip to play when mine critical
		}

	}
}

//------------------------------------------------------------------------------

void PagingTouchObjectEffects( int tmap_num )
{
	int i, j;
	eclip *ecP = gameData.eff.pEffects;

	for (i=0, j = gameData.eff.nEffects [gameStates.app.bD1Data];i<j;i++, ecP++)	{
		if ( ecP->changing_object_texture == tmap_num )	{
			PagingTouchVClip( &ecP->vc, 0 );
		}
	}
}

//------------------------------------------------------------------------------

void PagingTouchModel( int modelnum )
{
	int i;
	polymodel *pm = gameData.models.polyModels + modelnum;

	for (i=0;i<pm->n_textures;i++)	{
		PIGGY_PAGE_IN( gameData.pig.tex.objBmIndex[gameData.pig.tex.pObjBmIndex[pm->first_texture+i]], 0 );
		PagingTouchObjectEffects( gameData.pig.tex.pObjBmIndex[pm->first_texture+i] );
		#ifdef PSX_BUILD_TOOLS
		// cmp added
		PagingTouchObjectEffects( pm->first_texture+i );
		#endif
	}
}

//------------------------------------------------------------------------------

void PagingTouchWeapon( int weapon_type )
{
	// Page in the robot's weapons.
	
	if ( (weapon_type < 0) || (weapon_type > gameData.weapons.nTypes [0]) ) return;

	if ( gameData.weapons.info[weapon_type].picture.index )	{
		PIGGY_PAGE_IN( gameData.weapons.info[weapon_type].picture,0 );
	}		
	
	if ( gameData.weapons.info[weapon_type].flash_vclip > -1 )
		PagingTouchVClip(&gameData.eff.vClips [0][gameData.weapons.info[weapon_type].flash_vclip], 0);
	if ( gameData.weapons.info[weapon_type].wall_hit_vclip > -1 )
		PagingTouchVClip(&gameData.eff.vClips [0][gameData.weapons.info[weapon_type].wall_hit_vclip], 0);
	if ( WI_damage_radius (weapon_type) )	{
		// Robot_hit_vclips are actually badass_vclips
		if ( gameData.weapons.info[weapon_type].robot_hit_vclip > -1 )
			PagingTouchVClip(&gameData.eff.vClips [0][gameData.weapons.info[weapon_type].robot_hit_vclip], 0);
	}

	switch( gameData.weapons.info[weapon_type].render_type )	{
	case WEAPON_RENDER_VCLIP:
		if ( gameData.weapons.info[weapon_type].weapon_vclip > -1 )
			PagingTouchVClip( &gameData.eff.vClips [0][gameData.weapons.info[weapon_type].weapon_vclip], 0 );
		break;
	case WEAPON_RENDER_NONE:
		break;
	case WEAPON_RENDER_POLYMODEL:
		PagingTouchModel( gameData.weapons.info[weapon_type].model_num );
		break;
	case WEAPON_RENDER_BLOB:
		PIGGY_PAGE_IN( gameData.weapons.info[weapon_type].bitmap, 0 );
		break;
	}
}

//------------------------------------------------------------------------------

sbyte super_boss_gate_type_list[13] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22 };

void PagingTouchRobot( int robot_index )
{
	int i;

	// Page in robot_index
	PagingTouchModel(gameData.bots.pInfo[robot_index].model_num);
	if ( gameData.bots.pInfo[robot_index].exp1_vclip_num>-1 )
		PagingTouchVClip(&gameData.eff.vClips [0][gameData.bots.pInfo[robot_index].exp1_vclip_num], 0);
	if ( gameData.bots.pInfo[robot_index].exp2_vclip_num>-1 )
		PagingTouchVClip(&gameData.eff.vClips [0][gameData.bots.pInfo[robot_index].exp2_vclip_num], 0);

	// Page in his weapons
	PagingTouchWeapon( gameData.bots.pInfo[robot_index].weapon_type );

	// A super-boss can gate in robots...
	if ( gameData.bots.pInfo[robot_index].boss_flag==2 )	{
		for (i=0; i<13; i++ )
			PagingTouchRobot(super_boss_gate_type_list[i]);

		PagingTouchVClip( &gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT], 0 );
	}
}

//------------------------------------------------------------------------------

void PagingTouchObject( object *objP )
{
	int v;

	switch (objP->render_type) {

		case RT_NONE:	break;		//doesn't render, like the player

		case RT_POLYOBJ:
			if ( objP->rtype.pobj_info.tmap_override == -1 )
				PagingTouchModel(objP->rtype.pobj_info.model_num);
			else
				PIGGY_PAGE_IN( gameData.pig.tex.bmIndex [0][objP->rtype.pobj_info.tmap_override], 0 );
			break;

		case RT_POWERUP:
			if ((objP->rtype.vclip_info.nClipIndex > -1) && 
				 (objP->rtype.vclip_info.nClipIndex < gameData.eff.nClips [0])) {
		//@@	#ifdef WINDOWS
		//@@		PagingTouchVClip_w(&gameData.eff.vClips [0][objP->rtype.vclip_info.nClipIndex]);
		//@@	#else
				PagingTouchVClip(&gameData.eff.vClips [0][objP->rtype.vclip_info.nClipIndex], 0);
		//@@	#endif
				}
			else
				objP->rtype.vclip_info.nClipIndex = -1;
			break;

		case RT_MORPH:	break;
		case RT_FIREBALL: break;
		case RT_THRUSTER: break;
		case RT_WEAPON_VCLIP: break;

		case RT_HOSTAGE:
			PagingTouchVClip(&gameData.eff.vClips [0][objP->rtype.vclip_info.nClipIndex], 0);
			break;

		case RT_LASER: break;
 	}

	switch (objP->type) {	
	case OBJ_PLAYER:	
		v = GetExplosionVClip(objP, 0);
		if ( v > -1 )
			PagingTouchVClip(&gameData.eff.vClips [0][v], 0);
		break;
	case OBJ_ROBOT:
		PagingTouchRobot( objP->id );
		break;
	case OBJ_CNTRLCEN:
		PagingTouchWeapon( CONTROLCEN_WEAPON_NUM );
		if (Dead_modelnums[objP->rtype.pobj_info.model_num] != -1)	{
			PagingTouchModel( Dead_modelnums[objP->rtype.pobj_info.model_num] );
		}
		break;
	}
}

//------------------------------------------------------------------------------

void PagingTouchSide( segment * segp, short sidenum )
{
	int tmap1, tmap2;

	if ((segp - gameData.segs.segments == 208) && (sidenum == 4))
		tmap1 = segp->sides [sidenum].tmap_num;
	if (!(WALL_IS_DOORWAY (segp,sidenum, NULL) & WID_RENDER_FLAG))
		return;
	
	tmap1 = segp->sides [sidenum].tmap_num;
	PagingTouchWallEffects (tmap1);
	tmap2 = segp->sides [sidenum].tmap_num2 & 0x3fff;
	if (tmap2) {
		PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tmap2], gameStates.app.bD1Data);
		PagingTouchWallEffects (tmap2);
		}
	else
		PIGGY_PAGE_IN( gameData.pig.tex.pBmIndex[tmap1], gameStates.app.bD1Data );

	// PSX STUFF
	#ifdef PSX_BUILD_TOOLS
		// If there is water on the level, then force the water splash into memory
		if(!(gameData.pig.tex.pTMapInfo[tmap1].flags & TMI_VOLATILE) && (gameData.pig.tex.pTMapInfo[tmap1].flags & TMI_WATER)) {
			bitmap_index Splash;
			Splash.index = 1098;
			PIGGY_PAGE_IN(Splash);
			Splash.index = 1099;
			PIGGY_PAGE_IN(Splash);
			Splash.index = 1100;
			PIGGY_PAGE_IN(Splash);
			Splash.index = 1101;
			PIGGY_PAGE_IN(Splash);
			Splash.index = 1102;
			PIGGY_PAGE_IN(Splash);
		}
	#endif

}

//------------------------------------------------------------------------------

void PagingTouchRobotMaker( segment * segp )
{
	segment2	*seg2p = &gameData.segs.segment2s[SEG_IDX (segp)];

	if ( seg2p->special == SEGMENT_IS_ROBOTMAKER )	{
		PagingTouchVClip(&gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT], 0);
		if (gameData.matCens.robotCenters[seg2p->matcen_num].robot_flags != 0) {
			int	i;
			uint	flags;
			int	robot_index;

			for (i=0;i<2;i++) {
				robot_index = i*32;
				flags = gameData.matCens.robotCenters[seg2p->matcen_num].robot_flags[i];
				while (flags) {
					if (flags & 1)	{
						// Page in robot_index
						PagingTouchRobot( robot_index );
					}
					flags >>= 1;
					robot_index++;
				}
			}
		}
	}
}


//------------------------------------------------------------------------------

void PagingTouchSegment(segment * segp)
{
	short sn, objnum;
	segment2	*seg2p = &gameData.segs.segment2s[SEG_IDX (segp)];

	if ( seg2p->special == SEGMENT_IS_ROBOTMAKER )	
		PagingTouchRobotMaker(segp);

//	paging_draw_orb();
	for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
//		paging_draw_orb();
		PagingTouchSide( segp, sn );
	}

	for (objnum=segp->objects;objnum!=-1;objnum=gameData.objs.objects[objnum].next)	{
//		paging_draw_orb();
		PagingTouchObject( &gameData.objs.objects[objnum] );
	}
}

//------------------------------------------------------------------------------

void PagingTouchWall (wall *wallP)
{
	int	j;
	wclip *anim;

if (wallP->clip_num > -1 )	{
	anim = gameData.walls.pAnims + wallP->clip_num;
	for (j=0; j < anim->nFrameCount; j++ )
		PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex[anim->frames[j]], gameStates.app.bD1Data);
	}
}

//------------------------------------------------------------------------------

void PagingTouchWalls()
{
	int i;

for (i=0;i<gameData.walls.nWalls;i++)
	PagingTouchWall (gameData.walls.walls + i);
}

//------------------------------------------------------------------------------

void PagingTouchSegments (void)
{
	int	s;

for (s=0; s < gameData.segs.nSegments; s++)
	PagingTouchSegment( gameData.segs.segments + s );
}

//------------------------------------------------------------------------------

void PagingTouchPowerups (void)
{
	int	s;

for ( s=0; s < gameData.objs.pwrUp.nTypes; s++ )
	if ( gameData.objs.pwrUp.info[s].nClipIndex > -1 )	
		PagingTouchVClip(&gameData.eff.vClips [0][gameData.objs.pwrUp.info[s].nClipIndex], 0);
}

//------------------------------------------------------------------------------

void PagingTouchWeapons (void)
{
	int	s;

for ( s = 0; s < gameData.weapons.nTypes [0]; s++ )
	PagingTouchWeapon(s);
}

//------------------------------------------------------------------------------

void PagingTouchGauges (void)
{
	int	s;

for (s = 0; s < MAX_GAUGE_BMS; s++)
	if ( Gauges[s].index )
		PIGGY_PAGE_IN( Gauges[s], 0 );
}

//------------------------------------------------------------------------------

void PagingTouchAllSub ()
{
	int 			bBlackScreen;
	
	StopTime();
	bBlackScreen = gameStates.render.bPaletteFadedOut;
	if ( gameStates.render.bPaletteFadedOut )	{
		GrClearCanvas (BLACK_RGBA);
		GrPaletteStepLoad (NULL);
	}
	
//	ShowBoxedMessage(TXT_LOADING);

#if TRACE				
	con_printf (CON_VERBOSE, "Loading all textures in mine..." );
#endif
	PagingTouchSegments ();
	PagingTouchWalls ();
	PagingTouchPowerups ();
	PagingTouchWeapons ();
	PagingTouchPowerups ();
	PagingTouchGauges ();
	PagingTouchVClip( &gameData.eff.vClips [0][VCLIP_PLAYER_APPEARANCE], 0 );
	PagingTouchVClip( &gameData.eff.vClips [0][VCLIP_POWERUP_DISAPPEARANCE], 0 );


#ifdef PSX_BUILD_TOOLS

	//PSX STUFF
	PagingTouchWalls();
	for(s=0; s<=gameData.objs.nLastObject; s++) {
		PagingTouchObject(gameData.objs.objects + s);
	}


	{
		char * p;
		extern int gameData.missions.nCurrentLevel;
		extern ushort gameData.pig.tex.bitmapXlat[MAX_BITMAP_FILES];
		short bmUsed[MAX_BITMAP_FILES];
		FILE * fp;
		char fname[128];
		int i, bPageIn;
		grs_bitmap *bmP;

		if (gameData.missions.nCurrentLevel<0)                //secret level
			strcpy( fname, gameData.missions.szSecretLevelNames[-gameData.missions.nCurrentLevel-1] );
		else                                    //normal level
			strcpy( fname, gameData.missions.szLevelNames[gameData.missions.nCurrentLevel-1] );
		p = strchr( fname, '.' );
		if ( p ) *p = 0;
		strcat( fname, ".pag" );

		fp = fopen( fname, "wt" );
		for (i=0; i<MAX_BITMAP_FILES;i++ )      {
			bmUsed[i] = 0;
		}
		for (i=0; i<MAX_BITMAP_FILES;i++ )      {
			bmUsed[gameData.pig.tex.bitmapXlat[i]]++;
		}

		//cmp added so that .damage bitmaps are included for paged-in lights of the current level
		for (i=0; i<MAX_TEXTURES;i++) {
			if(gameData.pig.tex.pBmIndex[i].index > 0 && gameData.pig.tex.pBmIndex[i].index < MAX_BITMAP_FILES &&
				bmUsed[gameData.pig.tex.pBmIndex[i].index] > 0 &&
				gameData.pig.tex.pTMapInfo[i].destroyed > 0 && gameData.pig.tex.pTMapInfo[i].destroyed < MAX_BITMAP_FILES) {
				bmUsed[gameData.pig.tex.pBmIndex[gameData.pig.tex.pTMapInfo[i].destroyed].index] += 1;
				PIGGY_PAGE_IN(gameData.pig.tex.pBmIndex[gameData.pig.tex.pTMapInfo[i].destroyed]);

			}
		}

		//	Force cockpit to be paged in.
		{
			bitmap_index bonk;
			bonk.index = 109;
			PIGGY_PAGE_IN(bonk);
		}

		// Force in the frames for markers
		{
			bitmap_index bonk2;
			bonk2.index = 2014;
			PIGGY_PAGE_IN(bonk2);
			bonk2.index = 2015;
			PIGGY_PAGE_IN(bonk2);
			bonk2.index = 2016;
			PIGGY_PAGE_IN(bonk2);
			bonk2.index = 2017;
			PIGGY_PAGE_IN(bonk2);
			bonk2.index = 2018;
			PIGGY_PAGE_IN(bonk2);
		}

		for (i = 0, bmP = gameData.pig.tex.pBitmaps; i < MAX_BITMAP_FILES; i++, bmP++) {
			bPageIn = 1;
			// cmp debug
			//piggy_get_bitmap_name(i,fname);

			if (!bmP->bm_texBuf || (bmP->bm_props.flags & BM_FLAG_PAGED_OUT))
				bPageIn = 0;
//                      if (gameData.pig.tex.bitmapXlat[i]!=i)
//                              bPageIn = 0;

			if ( !bmUsed[i] )
				bPageIn = 0;
			if ( (i==47) || (i==48) )               // Mark red mplayer ship textures as paged in.
				bPageIn = 1;
			if ( !bPageIn )
				fprintf( fp, "0,\t// Bitmap %d (%s)\n", i, "test\0"); // cmp debug fname );
			else
				fprintf( fp, "1,\t// Bitmap %d (%s)\n", i, "test\0"); // cmp debug fname );
		}

		fclose(fp);
	}
#endif

#if TRACE				
	con_printf (CON_VERBOSE, "... loading all textures in mine done\n" );
#endif
//@@	ClearBoxedMessage();

	if ( bBlackScreen )	{
		GrPaletteStepClear();
		GrClearCanvas(BLACK_RGBA);
	}
	StartTime();
	ResetCockpit();		//force cockpit redraw next time

}

//------------------------------------------------------------------------------

int PagingGaugeSize (void)
{
return PROGRESS_STEPS (gameData.segs.nSegments) + 
		 PROGRESS_STEPS (gameFileInfo.walls.count) +
		 PROGRESS_STEPS (gameData.objs.pwrUp.nTypes) * 2 +
		 PROGRESS_STEPS (gameData.weapons.nTypes [0]) + 
		 PROGRESS_STEPS (MAX_GAUGE_BMS);
}

//------------------------------------------------------------------------------

static int nTouchSeg = 0;
static int nTouchWall = 0;
static int nTouchWeapon = 0;
static int nTouchPowerup1 = 0;
static int nTouchPowerup2 = 0;
static int nTouchGauge = 0;

static void PagingTouchPoll (int nItems, newmenu_item *m, int *key, int cItem)
{
	int	i;

GrPaletteStepLoad (NULL);
if (nTouchSeg < gameData.segs.nSegments) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchSeg < gameData.segs.nSegments); i++)
		PagingTouchSegment (gameData.segs.segments + nTouchSeg++);
	}
else if (nTouchWall < gameData.walls.nWalls) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchWall < gameData.walls.nWalls); i++)
		PagingTouchWall (gameData.walls.walls + nTouchWall++);
	}
else if (nTouchPowerup1 < gameData.objs.pwrUp.nTypes) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchPowerup1 < gameData.objs.pwrUp.nTypes); i++, nTouchPowerup1++)
		if (gameData.objs.pwrUp.info [nTouchPowerup1].nClipIndex > -1)	
			PagingTouchVClip (&gameData.eff.vClips [0][gameData.objs.pwrUp.info [nTouchPowerup1].nClipIndex], 0);
	}
else if (nTouchWeapon < gameData.weapons.nTypes [0]) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchWeapon < gameData.weapons.nTypes [0]); i++)
		PagingTouchWeapon (nTouchWeapon++);
	}
else if (nTouchPowerup2 < gameData.objs.pwrUp.nTypes) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchPowerup2 < gameData.objs.pwrUp.nTypes); i++, nTouchPowerup2++)
		if (gameData.objs.pwrUp.info [nTouchPowerup2].nClipIndex > -1)	
			PagingTouchVClip (&gameData.eff.vClips [0][gameData.objs.pwrUp.info [nTouchPowerup2].nClipIndex], 0);
	}
else if (nTouchGauge < MAX_GAUGE_BMS) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchGauge < MAX_GAUGE_BMS); i++, nTouchGauge++)
		if (Gauges [nTouchGauge].index)
			PIGGY_PAGE_IN (Gauges [nTouchGauge], 0);
	}
else {
	PagingTouchVClip (&gameData.eff.vClips [0][VCLIP_PLAYER_APPEARANCE], 0);
	PagingTouchVClip (&gameData.eff.vClips [0][VCLIP_POWERUP_DISAPPEARANCE], 0);
	*key = -2;
	GrPaletteStepLoad (NULL);
	return;
	}
m [0].value++;
m [0].rebuild = 1;
*key = 0;
GrPaletteStepLoad (NULL);
return;
}

//------------------------------------------------------------------------------

void PagingTouchAll ()
{
if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
		int	i = LoadMineGaugeSize ();

	nTouchSeg = 0;
	nTouchWall = 0;
	nTouchWeapon = 0;
	nTouchPowerup1 = 0;
	nTouchPowerup2 = 0;
	nTouchGauge = 0;
	NMProgressBar (TXT_PREP_DESCENT, i, i + PagingGaugeSize (), PagingTouchPoll); 
	}
else
	PagingTouchAllSub ();
}

//------------------------------------------------------------------------------
