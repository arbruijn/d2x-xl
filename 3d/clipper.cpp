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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "3d.h"
#include "globvars.h"
#include "clipper.h"
#include "error.h"

int nFreePoints=0;

CRenderPoint temp_points[MAX_POINTS_IN_POLY];
CRenderPoint *free_points[MAX_POINTS_IN_POLY];

void InitFreePoints(void)
{
	int i;

for (i = 0; i < MAX_POINTS_IN_POLY; i++)
	free_points [i] = temp_points + i;
}


CRenderPoint *get_temp_point()
{
	CRenderPoint *p;

	Assert (nFreePoints < MAX_POINTS_IN_POLY );
	p = free_points[nFreePoints++];

	p->m_flags = PF_TEMP_POINT;

	return p;
}

void free_temp_point(CRenderPoint *p)
{
	Assert(p->m_flags & PF_TEMP_POINT);

	free_points[--nFreePoints] = p;

	p->m_flags &= ~PF_TEMP_POINT;
}

//clips an edge against one plane.
CRenderPoint *clip_edge(int planeFlag,CRenderPoint *on_pnt,CRenderPoint *off_pnt)
{
	fix psx_ratio;
	fix a,b,kn,kd;
	CRenderPoint *tmp;

	//compute clipping value k = (xs-zs) / (xs-xe-zs+ze)
	//use x or y as appropriate, and negate x/y value as appropriate

	if (planeFlag & (CC_OFF_RIGHT | CC_OFF_LEFT)) {
		a = on_pnt->m_vertex [1].v.coord.x;
		b = off_pnt->m_vertex [1].v.coord.x;
	}
	else {
		a = on_pnt->m_vertex [1].v.coord.y;
		b = off_pnt->m_vertex [1].v.coord.y;
	}

	if (planeFlag & (CC_OFF_LEFT | CC_OFF_BOT)) {
		a = -a;
		b = -b;
	}

	kn = a - on_pnt->m_vertex [1].v.coord.z;						//xs-zs
	kd = kn - b + off_pnt->m_vertex [1].v.coord.z;				//xs-zs-xe+ze

	tmp = get_temp_point();

	psx_ratio = FixDiv( kn, kd );


// PSX_HACK!!!!
//	tmp->m_vertex [1].v.c.x = on_pnt->m_vertex [1].v.c.x + FixMulDiv(off_pnt->m_vertex [1].v.c.x-on_pnt->m_vertex [1].v.c.x,kn,kd);
//	tmp->m_vertex [1].v.c.y = on_pnt->m_vertex [1].v.c.y + FixMulDiv(off_pnt->m_vertex [1].v.c.y-on_pnt->m_vertex [1].v.c.y,kn,kd);

	tmp->m_vertex [1].v.coord.x = on_pnt->m_vertex [1].v.coord.x + FixMul( (off_pnt->m_vertex [1].v.coord.x-on_pnt->m_vertex [1].v.coord.x), psx_ratio);
	tmp->m_vertex [1].v.coord.y = on_pnt->m_vertex [1].v.coord.y + FixMul( (off_pnt->m_vertex [1].v.coord.y-on_pnt->m_vertex [1].v.coord.y), psx_ratio);

	if (planeFlag & (CC_OFF_TOP|CC_OFF_BOT))
		tmp->m_vertex [1].v.coord.z = tmp->m_vertex [1].v.coord.y;
	else
		tmp->m_vertex [1].v.coord.z = tmp->m_vertex [1].v.coord.x;

	if (planeFlag & (CC_OFF_LEFT|CC_OFF_BOT))
		tmp->m_vertex [1].v.coord.z = -tmp->m_vertex [1].v.coord.z;

	if (on_pnt->m_flags & PF_UVS) {
// PSX_HACK!!!!
//		tmp->m_uvl.u = on_pnt->m_uvl.u + FixMulDiv(off_pnt->m_uvl.u-on_pnt->m_uvl.u,kn,kd);
//		tmp->m_uvl.v = on_pnt->m_uvl.v + FixMulDiv(off_pnt->m_uvl.v-on_pnt->m_uvl.v,kn,kd);
		tmp->m_uvl.u = on_pnt->m_uvl.u + FixMul((off_pnt->m_uvl.u-on_pnt->m_uvl.u), psx_ratio);
		tmp->m_uvl.v = on_pnt->m_uvl.v + FixMul((off_pnt->m_uvl.v-on_pnt->m_uvl.v), psx_ratio);

		tmp->m_flags |= PF_UVS;
	}

	if (on_pnt->m_flags & PF_LS) {
// PSX_HACK
//		tmp->m_r = on_pnt->m_r + FixMulDiv(off_pnt->m_r-on_pnt->m_r,kn,kd);
//		tmp->m_g = on_pnt->m_g + FixMulDiv(off_pnt->m_g-on_pnt->m_g,kn,kd);
//		tmp->m_b = on_pnt->m_b + FixMulDiv(off_pnt->m_b-on_pnt->m_b,kn,kd);

		tmp->m_uvl.l = on_pnt->m_uvl.l + FixMul((off_pnt->m_uvl.l-on_pnt->m_uvl.l), psx_ratio);

		tmp->m_flags |= PF_LS;
	}

	G3Encode(tmp);

	return tmp;
}

//clips a line to the viewing pyramid.
void clip_line(CRenderPoint **p0,CRenderPoint **p1,ubyte codes_or)
{
	int planeFlag;
	CRenderPoint *old_p1;

	//might have these left over
	(*p0)->m_flags &= ~(PF_UVS|PF_LS);
	(*p1)->m_flags &= ~(PF_UVS|PF_LS);

	for (planeFlag=1;planeFlag<16;planeFlag<<=1)
		if (codes_or & planeFlag) {

			if ((*p0)->m_codes & planeFlag)
			 {CRenderPoint *t=*p0; *p0=*p1; *p1=t;}	//swap!

			old_p1 = *p1;

			*p1 = clip_edge(planeFlag,*p0,*p1);

			if (old_p1->m_flags & PF_TEMP_POINT)
				free_temp_point(old_p1);
		}

}


int clip_plane(int planeFlag,CRenderPoint **src,CRenderPoint **dest,int *nv,tRenderCodes *cc)
{
	int i;
	CRenderPoint **save_dest=dest;

	//copy first two verts to end
	src[*nv] = src[0];
	src[*nv+1] = src[1];

	cc->ccAnd = 0xff; cc->ccOr = 0;

	for (i=1;i<=*nv;i++) {

		if (src[i]->m_codes & planeFlag) {				//cur point off?

			if (! (src[i-1]->m_codes & planeFlag)) {	//prev not off?

				*dest = clip_edge(planeFlag,src[i-1],src[i]);
				cc->ccOr  |= (*dest)->m_codes;
				cc->ccAnd &= (*dest)->m_codes;
				dest++;
			}

			if (! (src[i+1]->m_codes & planeFlag)) {

				*dest = clip_edge(planeFlag,src[i+1],src[i]);
				cc->ccOr  |= (*dest)->m_codes;
				cc->ccAnd &= (*dest)->m_codes;
				dest++;
			}

			if (src[i]->m_flags & PF_TEMP_POINT)
				free_temp_point(src[i]);
		}
		else {			//cur not off, copy to dest buffer

			*dest++ = src[i];

			cc->ccOr  |= src[i]->m_codes;
			cc->ccAnd &= src[i]->m_codes;
		}
	}

	return (int) (dest - save_dest);
}


CRenderPoint **clip_polygon(CRenderPoint **src,CRenderPoint **dest,int *nv,tRenderCodes *cc)
{
	int planeFlag;
	CRenderPoint **t;

	for (planeFlag=1;planeFlag<16;planeFlag<<=1)

		if (cc->ccOr & planeFlag) {

			*nv = clip_plane(planeFlag,src,dest,nv,cc);

			if (cc->ccAnd)		//clipped away
				return dest;

			t = src; src = dest; dest = t;

		}

	return src;		//we swapped after we copied
}

