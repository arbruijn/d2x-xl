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

#ifndef _FIREBALL_H
#define _FIREBALL_H

// explosion types
#define ET_SPARKS       0   //little sparks, like when laser hits CWall
#define ET_MULTI_START  1   //first part of multi-part explosion
#define ET_MULTI_SECOND 2   //second part of multi-part explosion

#define BLAST_LIFE		(I2X (2) / 5)
#define SHOCKWAVE_LIFE	I2X (1)

CObject *CreateSplashDamageExplosion (CObject* parentObjP, int16_t nSegment, CFixVector& position, CFixVector& impact, fix size, uint8_t vclipType,
										        fix maxdamage, fix maxdistance, fix maxforce, int16_t parent);

// blows up a splash damage weapon, creating the splash damage explosion
// return the explosion CObject
// blows up the player with a splash damage explosion
// return the explosion CObject
void DoDebrisFrame (CObject* objP);      // deal with debris for this frame
void DrawFireball (CObject* objP);
CObject* CreateExplosion (CObject* parentP, int16_t nSegment, CFixVector& vPos, CFixVector& vImpact, fix xSize, uint8_t nVClip, 
                          fix xMaxDamage = 0, fix xMaxDistance = 0, fix xMaxForce = 0, int16_t nParent = -1);


int16_t GetExplosionVClip (CObject *obj, int32_t stage);

//------------------------------------------------------------------------------

static inline CObject* CreateMuzzleFlash (int16_t nSegment, CFixVector& position, fix size, uint8_t nVClip)
{
return CreateExplosion (NULL, nSegment, position, position, size, nVClip, 0, 0, 0, -1);
}

//------------------------------------------------------------------------------

static inline CObject* CreateExplosion (int16_t nSegment, CFixVector& position, fix size, uint8_t nVClip)
{
return CreateExplosion (NULL, nSegment, position, position, size, nVClip, 0, 0, 0, -1);
}

//------------------------------------------------------------------------------

#endif /* _FIREBALL_H */
