/*
 *  iphoneRender.c
 *  doom
 *
 *  Created by John Carmack on 4/29/09.
 *  Copyright 2009 id Software. All rights reserved.
 *
 */
/*
 
 Copyright (C) 2009 Id Software, Inc.
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 */


#include <stdio.h>
#include <string.h>
#include <math.h>
#include "SDL_opengl.h"
#include "doomtype.h"
#include "w_wad.h"
#include "m_argv.h"
#include "d_event.h"
#include "v_video.h"
#include "doomstat.h"
#include "r_bsp.h"
#include "r_main.h"
#include "r_draw.h"
#include "r_sky.h"
#include "r_plane.h"
#include "r_data.h"
#include "r_things.h"
#include "r_fps.h"
#include "p_maputl.h"
#include "m_bbox.h"
#include "lprintf.h"
#include "gl_intern.h"
#include "gl_struct.h"

// If the Doom levels had been built with realistic visibility
// taken into account for the sky areas, we could just draw the
// sky first and then the walls, but that gives artifacts where
// you see some sectors floating in the sky now.  This causes
// the walls to draw extended top and bottom sections for skies.
#define SKYWALLS

#define MINZ        (FRACUNIT*4)
#define BASEYCENTER 100
#define MINZ_FLOAT	4

typedef struct {
	GLTexture		*tex;
	side_t			*side;
	int				flag;		// GLDWF_TOP, GLDWF_M1S, etc
} sortLine_t;

#define MAX_SORT_LINES	4096
sortLine_t			sortLines[MAX_SORT_LINES];
int					numSortLines;

typedef struct {
	GLTexture		*texture;
	sector_t		*sector;
	boolean			ceiling;
} sortSectorPlane_t;

#define MAX_SECTOR_PLANES	1024
sortSectorPlane_t	sectorPlanes[MAX_SECTOR_PLANES];
int					numSectorPlanes;

typedef struct {
	GLTexture		*tex;
} sortSprite_t;

// Cleared to 0 at frame start.
// Individual columns will be set to 1 as occluding segments are processed.
// An occluding segment is either a one-sided line, a line that has a back
// sector with equal floor and ceiling heights, a line with a back ceiling
// height lower than the fron floor height, or a line with a back floor height
// higher than the front ceiling height.
// Entire nodes are culled when their bounds does not include a 0 column.
// Individual line segments are culled when their span does not include 0 columns.
// Sprites could be checked against it, but it may not be worth it.
char	occlusion[MAX_SCREENWIDTH+2];	// +2 for guard columns to avoid clamping

// when the iphone is upside down, the occlusion segments are reversed
boolean	reversedLandscape;

// this matrix is exactly what GL uses, but there will still
// be floating point differences between the GPU and CPU
float	glMVPmatrix[16];

// if any sector textures are the sky texture, we will draw the sky and
// ignore those sector geometries
boolean	skyIsVisible;

// these should really be initialized based on viewwidth somewhere...
float	halfWidthFloat = 240.0f;

// used during debugging to isolate incorrect culling
int		failCount;

// Some of the two sided line segments in the original game don't have a valid
// texture, so stick something there instead of leaving a gaping hole in the world.
GLTexture *defaultTexture;

// just for the sky texture setup
float	yaw;

// counters
int		c_occludedSprites;
int		c_sectors;
int		c_subsectors;

// test options
int		testClear = 0;
int		testNewRenderer = 0;
int		showRenderTime;
int		blendAll;


void BuildIndexedTriangles(void);
void BuildSideSegs(void);

void IR_MergeSectors( int fromSector, int intoSector ) {
	// E3M8 (and possibly others somewhere) has a bad sector
	// classification with two stray lines in sector 2 that
	// should be a part of sector 1.  This makes both of the
	// sectors "broken" and unable to be properly tesselated.
	assert( (unsigned)fromSector < numsectors );
	assert( (unsigned)intoSector < numsectors );
	sector_t *fromSectorPtr = &sectors[fromSector];
	sector_t *intoSectorPtr = &sectors[intoSector];
	
	int moveLines = 0;
	for ( int i = 0 ; i < numlines ; i++ ) {
		if ( lines[i].frontsector == fromSectorPtr ) {
			moveLines++;
		} else if ( lines[i].backsector == fromSectorPtr ) {
			moveLines++;
		}
	}

	// add these lines to intoSector
	// Unfortunately, the sector->lines list is not allocated per-sector, but
	// is a single block for the entire level, so we can't realloc it.  I'm
	// going to just let the new table leak.
	line_t **newLines = Z_Malloc( ( intoSectorPtr->linecount + moveLines ) * sizeof( *intoSectorPtr->lines ),
									 PU_LEVEL,0);
	memcpy( newLines, intoSectorPtr->lines, intoSectorPtr->linecount * sizeof( *newLines ) );
	intoSectorPtr->lines = newLines;
	for ( int i = 0 ; i < numlines ; i++ ) {
		if ( lines[i].frontsector == fromSectorPtr ) {
			intoSectorPtr->lines[intoSectorPtr->linecount++] = &lines[i];
			lines[i].frontsector = intoSectorPtr;
		} else if ( lines[i].backsector == fromSectorPtr ) {
			intoSectorPtr->lines[intoSectorPtr->linecount++] = &lines[i];
			lines[i].backsector = intoSectorPtr;
		}
	}
	
	// change all the segs
	for ( int i = 0 ; i < numsegs ; i++ ) {
		if ( segs[i].frontsector == fromSectorPtr ) {
			segs[i].frontsector = intoSectorPtr;
		}
		if ( segs[i].backsector == fromSectorPtr ) {
			segs[i].backsector = intoSectorPtr;
		}
	}
	
	// change all the sides to point to the new one
	for ( int i = 0 ; i < numsides ; i++ ) {
		if ( sides[i].sector == fromSectorPtr ) {
			sides[i].sector = intoSectorPtr;
		}
	}
	
	// change all the subsectors to point to the new one
	for ( int i = 0 ; i < numsubsectors ; i++ ) {
		if ( subsectors[i].sector == fromSectorPtr ) {
			subsectors[i].sector = intoSectorPtr;
		}
	}
	
	
	// make fromSector vestigial so it doesn't get tesselated
	fromSectorPtr->linecount = 0;
}

void IR_InitLevel() {
	BuildIndexedTriangles();	// convert the loops into indexed triangles
	BuildSideSegs();			// create a seg_t for each side_t so we can draw the
								// unclipped versions that fit perfectly with the sectors
	
	// find something else used in the level for a default texture
	for ( int i = 0 ; i < numsides ; i++ ) {
		if ( sides[i].toptexture ) {
			defaultTexture=gld_RegisterTexture(sides[i].toptexture, true, false);
			if ( defaultTexture ) {
				break;
			}
		}
	}
	assert( defaultTexture );
}


float	lightDistance = 10.0f;	// in prBoom MAP_SCALE units, increasing this makes things get dimmer faster
#define MAX_LIGHT_DROP	96
float	lightingVector[3];	// transform and scale [ x y 1 ] to get color units to subtract
static int FadedLighting( float x, float y, int sectorLightLevel ) {
	// Ramp down the lightover lightDistance world units.
	// Triangles that extend across behind the view origin and past
	// the lightDistance clamping boundary will not have completely linear fading,
	// but nobody should notice.
	
	// A proportional drop in lighting sounds like a better idea, but
	// this linear drop seems to look nicer.  It's not like Doom's
	// lighting is realistic in any case...
	
	int	idist = x * lightingVector[0] + y * lightingVector[1] + lightingVector[2];
	if ( idist < 0 ) {
		idist = 0;
	} else if ( idist > MAX_LIGHT_DROP ) {
		idist = MAX_LIGHT_DROP;
	}
	sectorLightLevel -= idist;	
	if ( sectorLightLevel < 0 ) {
		sectorLightLevel = 0;
	}
	if ( sectorLightLevel > 255 ) {
		sectorLightLevel = 255;
	}
	return sectorLightLevel | (sectorLightLevel<<8) | (sectorLightLevel<<16) | (255<<24);
}


//
// IR_ProjectSprite
// Generates a vissprite for a thing if it might be visible.
//

static void IR_ProjectSprite (mobj_t* thing, int lightlevel)
{
	fixed_t   gzt;               // killough 3/27/98
	fixed_t   tx;
	fixed_t   xscale;
	int       x1;
	int       x2;
	spritedef_t   *sprdef;
	spriteframe_t *sprframe;
	int       lump;
	boolean   flip;
	
	// transform the origin point
	fixed_t tr_x, tr_y;
	fixed_t fx, fy, fz;
	fixed_t gxt, gyt;
	fixed_t tz;
	int width;
	
	fx = thing->x;
	fy = thing->y;
	fz = thing->z;
	
	tr_x = fx - viewx;
	tr_y = fy - viewy;
	
	gxt = FixedMul(tr_x,viewcos);
	gyt = -FixedMul(tr_y,viewsin);
	
	tz = gxt-gyt;
	
    // thing is behind view plane?
	if (tz < MINZ)
		return;
	
	xscale = FixedDiv(projection, tz);
	
	gxt = -FixedMul(tr_x,viewsin);
	gyt = FixedMul(tr_y,viewcos);
	tx = -(gyt+gxt);
	
	// too far off the side?
	if (D_abs(tx)>(tz<<2))
		return;
	
    // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
	if ((unsigned) thing->sprite >= (unsigned)numsprites)
		I_Error ("R_ProjectSprite: Invalid sprite number %i", thing->sprite);
#endif
	
	sprdef = &sprites[thing->sprite];
	
#ifdef RANGECHECK
	if ((thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
		I_Error ("R_ProjectSprite: Invalid sprite frame %i : %i", thing->sprite,
				 thing->frame);
#endif
	
	if (!sprdef->spriteframes)
		I_Error ("R_ProjectSprite: Missing spriteframes %i : %i", thing->sprite,
				 thing->frame);
	
	sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];
	
	if (sprframe->rotate)
    {
		// choose a different rotation based on player view
		// JDC: this could be better...
		angle_t ang = R_PointToAngle(fx, fy);
		unsigned rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
		lump = sprframe->lump[rot];
		flip = (boolean) sprframe->flip[rot];
    }
	else
    {
		// use single rotation for all views
		lump = sprframe->lump[0];
		flip = (boolean) sprframe->flip[0];
    }
	
	{
		const rpatch_t* patch = R_CachePatchNum(lump+firstspritelump);
		
		/* calculate edges of the shape
		 * cph 2003/08/1 - fraggle points out that this offset must be flipped
		 * if the sprite is flipped; e.g. FreeDoom imp is messed up by this. */
		if (flip) {
			tx -= (patch->width - patch->leftoffset) << FRACBITS;
		} else {
			tx -= patch->leftoffset << FRACBITS;
		}
		x1 = (centerxfrac + FixedMul(tx,xscale)) >> FRACBITS;
		
		tx += patch->width<<FRACBITS;
		x2 = ((centerxfrac + FixedMul (tx,xscale) ) >> FRACBITS) - 1;
		
		gzt = fz + (patch->topoffset << FRACBITS);
		width = patch->width;
		
		// JDC: we don't care if they never get freed,
		// so don't bother changing the zone tag status each time
		//R_UnlockPatchNum(lump+firstspritelump);
	}
	
	// off the side?
	if (x1 > viewwidth || x2 < 0)
		return;
	
	// killough 4/9/98: clip things which are out of view due to height
	// e6y: fix of hanging decoration disappearing in Batman Doom MAP02
	// centeryfrac -> viewheightfrac
	if (fz  > viewz + FixedDiv(viewheightfrac, xscale) ||
		gzt < viewz - FixedDiv(viewheightfrac-viewheight, xscale))
		return;
	
	// JDC: clip to the occlusio buffer
	int	testLow = x1 < 0 ? 0 : x1;
	int testHigh = x2 >= viewwidth ? viewwidth - 1 : x2;
	//if ( reversedLandscape ) {
	//	testLow = viewwidth-1-testLow;
	//	testHigh = viewwidth-1-testHigh;
	//}
	if ( !memchr( occlusion+testLow, 0, testHigh - testLow ) ) {
		c_occludedSprites++;
		return;
	}
	
	// ------------ gld_AddSprite ----------
	mobj_t *pSpr= thing;
	GLSprite sprite;
	float voff,hoff;
	
	sprite.scale= FixedDiv(projectiony, tz);
	if (pSpr->frame & FF_FULLBRIGHT)
		sprite.light = 255;
	else
		sprite.light = pSpr->subsector->sector->lightlevel+(extralight<<5);
	sprite.cm=CR_LIMIT+(int)((pSpr->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT));
	sprite.gltexture=gld_RegisterPatch(lump+firstspritelump,sprite.cm);
	if (!sprite.gltexture)
		return;
	sprite.shadow = (pSpr->flags & MF_SHADOW) != 0;
	sprite.trans  = (pSpr->flags & MF_TRANSLUCENT) != 0;
	if (movement_smooth)
	{
		sprite.x = (float)(-pSpr->PrevX + FixedMul (tic_vars.frac, -pSpr->x - (-pSpr->PrevX)))/MAP_SCALE;
		sprite.y = (float)(pSpr->PrevZ + FixedMul (tic_vars.frac, pSpr->z - pSpr->PrevZ))/MAP_SCALE;
		sprite.z = (float)(pSpr->PrevY + FixedMul (tic_vars.frac, pSpr->y - pSpr->PrevY))/MAP_SCALE;
	}
	else
	{
		sprite.x=-(float)pSpr->x/MAP_SCALE;
		sprite.y= (float)pSpr->z/MAP_SCALE;
		sprite.z= (float)pSpr->y/MAP_SCALE;
	}
	
	sprite.vt=0.0f;
	sprite.vb=(float)sprite.gltexture->height/(float)sprite.gltexture->tex_height;
	if (flip)
	{
		sprite.ul=0.0f;
		sprite.ur=(float)sprite.gltexture->width/(float)sprite.gltexture->tex_width;
	}
	else
	{
		sprite.ul=(float)sprite.gltexture->width/(float)sprite.gltexture->tex_width;
		sprite.ur=0.0f;
	}
	hoff=(float)sprite.gltexture->leftoffset/(float)(MAP_COEFF);
	voff=(float)sprite.gltexture->topoffset/(float)(MAP_COEFF);
	sprite.x1=hoff-((float)sprite.gltexture->realtexwidth/(float)(MAP_COEFF));
	sprite.x2=hoff;
	sprite.y1=voff;
	sprite.y2=voff-((float)sprite.gltexture->realtexheight/(float)(MAP_COEFF));
	
	// JDC: don't let sprites poke below the ground level.
	// Software rendering Doom didn't use depth buffering, 
	// so sprites always got drawn on top of the flat they
	// were on, but in GL they tend to get a couple pixel
	// rows clipped off.
	if ( sprite.y2 < 0 ) {
		sprite.y1 -= sprite.y2;
		sprite.y2 = 0;
	}
	
	if (gld_drawinfo.num_sprites>=gld_drawinfo.max_sprites)
	{
		gld_drawinfo.max_sprites+=128;
		gld_drawinfo.sprites=Z_Realloc(gld_drawinfo.sprites,gld_drawinfo.max_sprites*sizeof(GLSprite),PU_LEVEL,0);
	}
	gld_drawinfo.sprites[gld_drawinfo.num_sprites++]=sprite;
}

// JDC: removed the 0.001f epsilons that were presumably added
// to try to hide T-junction cracks, but now that we are drawing
// source lines instead of clipped segs, it is a non-problem.
#define LINE seg->linedef
#define CALC_Y_VALUES(w, lineheight, floor_height, ceiling_height)\
(w).ytop=((float)(ceiling_height)/(float)MAP_SCALE);\
(w).ybottom=((float)(floor_height)/(float)MAP_SCALE);\
lineheight=((float)fabs(((ceiling_height)/(float)FRACUNIT)-((floor_height)/(float)FRACUNIT)))

#define OU(w,seg) (((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width)
#define OV(w,seg) (((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height)
#define OV_PEG(w,seg,v_offset) (OV((w),(seg))-(((float)(v_offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height))

#define CALC_TEX_VALUES_TOP(w, seg, peg, linelength, lineheight)\
(w).flag=GLDWF_TOP;\
(w).ul=OU((w),(seg))+(0.0f);\
(w).ur=OU((w),(seg))+((linelength)/(float)(w).gltexture->buffer_width);\
(peg)?\
(\
(w).vb=OV((w),(seg))+((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
(w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height))\
):(\
(w).vt=OV((w),(seg))+(0.0f),\
(w).vb=OV((w),(seg))+((float)(lineheight)/(float)(w).gltexture->buffer_height)\
)

#define CALC_TEX_VALUES_MIDDLE1S(w, seg, peg, linelength, lineheight)\
(w).flag=GLDWF_M1S;\
(w).ul=OU((w),(seg))+(0.0f);\
(w).ur=OU((w),(seg))+((linelength)/(float)(w).gltexture->buffer_width);\
(peg)?\
(\
(w).vb=OV((w),(seg))+((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
(w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height))\
):(\
(w).vt=OV((w),(seg))+(0.0f),\
(w).vb=OV((w),(seg))+((float)(lineheight)/(float)(w).gltexture->buffer_height)\
)

#define CALC_TEX_VALUES_MIDDLE2S(w, seg, peg, linelength, lineheight)\
(w).flag=GLDWF_M2S;\
(w).ul=OU((w),(seg))+(0.0f);\
(w).ur=OU((w),(seg))+((linelength)/(float)(w).gltexture->buffer_width);\
(peg)?\
(\
(w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
(w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height))\
):(\
(w).vt=(0.0f),\
(w).vb=((float)(lineheight)/(float)(w).gltexture->buffer_height)\
)

#define CALC_TEX_VALUES_BOTTOM(w, seg, peg, linelength, lineheight, v_offset)\
(w).flag=GLDWF_BOT;\
(w).ul=OU((w),(seg))+(0.0f);\
(w).ur=OU((w),(seg))+((linelength)/(float)(w).gltexture->realtexwidth);\
(peg)?\
(\
(w).vb=OV_PEG((w),(seg),(v_offset))+((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
(w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height))\
):(\
(w).vt=OV((w),(seg))+(0.0f),\
(w).vb=OV((w),(seg))+((float)(lineheight)/(float)(w).gltexture->buffer_height)\
)

// e6y
// Sky textures with a zero index should be forced
// See third episode of requiem.wad
#define SKYTEXTURE_PRBOOM(sky1,sky2)\
if ((sky1) & PL_SKYFLAT)\
{\
const line_t *l = &lines[sky1 & ~PL_SKYFLAT];\
const side_t *s = *l->sidenum + sides;\
wall.gltexture=gld_RegisterTexture(texturetranslation[s->toptexture], false, texturetranslation[s->toptexture]==skytexture);\
wall.skyyaw=-2.0f*((-(float)((viewangle+s->textureoffset)>>ANGLETOFINESHIFT)*360.0f/FINEANGLES)/90.0f);\
wall.skyymid = 200.0f/319.5f*(((float)s->rowoffset/(float)FRACUNIT - 28.0f)/100.0f);\
wall.flag = l->special==272 ? GLDWF_SKY : GLDWF_SKYFLIP;\
}\
else\
 if ((sky2) & PL_SKYFLAT)\
{\
const line_t *l = &lines[sky2 & ~PL_SKYFLAT];\
const side_t *s = *l->sidenum + sides;\
wall.gltexture=gld_RegisterTexture(texturetranslation[s->toptexture], false, texturetranslation[s->toptexture]==skytexture);\
wall.skyyaw=-2.0f*((-(float)((viewangle+s->textureoffset)>>ANGLETOFINESHIFT)*360.0f/FINEANGLES)/90.0f);\
wall.skyymid = 200.0f/319.5f*(((float)s->rowoffset/(float)FRACUNIT - 28.0f)/100.0f);\
wall.flag = l->special==272 ? GLDWF_SKY : GLDWF_SKYFLIP;\
}\
else\
{\
wall.gltexture=gld_RegisterTexture(skytexture, false, true);\
wall.skyyaw=-2.0f*((yaw+90.0f)/90.0f);\
wall.skyymid = 200.0f/319.5f*((100.0f)/100.0f);\
wall.flag = GLDWF_SKY;\
};

#define SKYTEXTURE(sky1,sky2)\
wall.gltexture=NULL;\
wall.flag = GLDWF_SKY;

#define ADDWALL(wall)\
{\
if (gld_drawinfo.num_walls>=gld_drawinfo.max_walls)\
{\
gld_drawinfo.max_walls+=128;\
gld_drawinfo.walls=Z_Realloc(gld_drawinfo.walls,gld_drawinfo.max_walls*sizeof(GLWall),PU_LEVEL,0);\
}\
gld_drawinfo.walls[gld_drawinfo.num_walls++]=*wall;\
};

extern GLSeg *gl_segs;
extern byte rendermarker;
extern byte *segrendered;
extern int tran_filter_pct;

void IR_AddWall(seg_t *seg)
{
	GLWall wall;
	GLTexture *temptex;
	sector_t *thefrontsector;
	sector_t *thebacksector;
    sector_t ftempsec; // needed for R_FakeFlat
    sector_t btempsec; // needed for R_FakeFlat
	float lineheight;
	int rellight = 0;

	wall.glseg=NULL;
	wall.side = seg->sidedef;
	thefrontsector=R_FakeFlat(seg->frontsector, &ftempsec, NULL, NULL, false); // for boom effects
    if (!thefrontsector)
        return;
	
	// JDC: improve this lighting tweak
	rellight = seg->linedef->dx==0? +8 : seg->linedef->dy==0 ? -8 : 0;
	int light = thefrontsector->lightlevel+rellight+(extralight<<5);
	wall.light = MAX(MIN((light),255),0);
	wall.alpha=1.0f;
	wall.gltexture=NULL;
	
	if (!seg->backsector) /* onesided */
	{
#ifdef SKYWALLS		
		if (thefrontsector->ceilingpic==skyflatnum)
		{
			wall.ytop=255.0f;
			wall.ybottom=(float)thefrontsector->ceilingheight/MAP_SCALE;
			SKYTEXTURE(thefrontsector->sky,thefrontsector->sky);
			ADDWALL(&wall);
		}
		if (thefrontsector->floorpic==skyflatnum)
		{
			wall.ytop=(float)thefrontsector->floorheight/MAP_SCALE;
			wall.ybottom=-255.0f;
			SKYTEXTURE(thefrontsector->sky,thefrontsector->sky);
			ADDWALL(&wall);
		}
#endif		
		temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true, false);
		if (temptex)
		{
			wall.gltexture=temptex;
			CALC_Y_VALUES(wall, lineheight, thefrontsector->floorheight, thefrontsector->ceilingheight);
			CALC_TEX_VALUES_MIDDLE1S(
									 wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0,
									 seg->length, lineheight
									 );
			ADDWALL(&wall);
		}
	}
	else /* twosided */
	{
		int floor_height,ceiling_height;
		
		thebacksector=R_FakeFlat(seg->backsector, &btempsec, NULL, NULL, true); // for boom effects
        if(!thebacksector)
            return;
		/* toptexture */
		ceiling_height=thefrontsector->ceilingheight;
		floor_height=thebacksector->ceilingheight;
#ifdef SKYWALLS
		if (thefrontsector->ceilingpic==skyflatnum)
		{
			wall.ytop=255.0f;
			if (
				// e6y
				// Fix for HOM in the starting area on Memento Mori map29 and on map30.
				// old code: (thebacksector->ceilingheight==thebacksector->floorheight) &&
				(thebacksector->ceilingheight==thebacksector->floorheight||(thebacksector->ceilingheight<=thefrontsector->floorheight)) &&
				(thebacksector->ceilingpic==skyflatnum)
				)
			{
				wall.ybottom=(float)thebacksector->floorheight/MAP_SCALE;
				SKYTEXTURE(thefrontsector->sky,thebacksector->sky);
				ADDWALL(&wall);
			}
			else
			{
				if ( (texturetranslation[seg->sidedef->toptexture]!=NO_TEXTURE) )
				{
					// e6y
					// It corrects some problem with sky, but I do not remember which one
					// old code: wall.ybottom=(float)thefrontsector->ceilingheight/MAP_SCALE;
					wall.ybottom=(float)MAX(thefrontsector->ceilingheight,thebacksector->ceilingheight)/MAP_SCALE;
					
					SKYTEXTURE(thefrontsector->sky,thebacksector->sky);
					ADDWALL(&wall);
				}
				else
					if ( (thebacksector->ceilingheight <= thefrontsector->floorheight) ||
						(thebacksector->ceilingpic != skyflatnum) )
					{
						wall.ybottom=(float)thebacksector->ceilingheight/MAP_SCALE;
						SKYTEXTURE(thefrontsector->sky,thebacksector->sky);
						ADDWALL(&wall);
					}
			}
		}
#endif		
		if (floor_height<ceiling_height)
		{
			if (!((thefrontsector->ceilingpic==skyflatnum) && (thebacksector->ceilingpic==skyflatnum)))
			{
				temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->toptexture], true, false);
				if ( !temptex ) {
					temptex = defaultTexture;	// it seems some line segments have bad data...
				}
				wall.gltexture=temptex;
				CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
				CALC_TEX_VALUES_TOP(
									wall, seg, (LINE->flags & (ML_DONTPEGBOTTOM | ML_DONTPEGTOP))==0,
									seg->length, lineheight
									);
				ADDWALL(&wall);
			}
		}
		
		/* midtexture */
		//e6y
		if (comp[comp_maskedanim])
			temptex=gld_RegisterTexture(seg->sidedef->midtexture, true, false);
		else
			// e6y
			// Animated middle textures with a zero index should be forced
			// See spacelab.wad (http://www.doomworld.com/idgames/index.php?id=6826)
			temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true, true);
		
		if (temptex && seg->sidedef->midtexture != NO_TEXTURE)
		{
			wall.gltexture=temptex;
			if ( (LINE->flags & ML_DONTPEGBOTTOM) >0)
			{
				if (seg->backsector->ceilingheight<=seg->frontsector->floorheight)
					goto bottomtexture;
				floor_height=MAX(seg->frontsector->floorheight,seg->backsector->floorheight)+(seg->sidedef->rowoffset);
				ceiling_height=floor_height+(wall.gltexture->realtexheight<<FRACBITS);
			}
			else
			{
				if (seg->backsector->ceilingheight<=seg->frontsector->floorheight)
					goto bottomtexture;
				ceiling_height=MIN(seg->frontsector->ceilingheight,seg->backsector->ceilingheight)+(seg->sidedef->rowoffset);
				floor_height=ceiling_height-(wall.gltexture->realtexheight<<FRACBITS);
			}
			// e6y
			// The fix for wrong middle texture drawing
			// if it exceeds the boundaries of its floor and ceiling
			
			/*CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
			 CALC_TEX_VALUES_MIDDLE2S(
			 wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0,
			 segs[seg->iSegID].length, lineheight
			 );*/
			{
				int floormax, ceilingmin, linelen;
				float mip;
				mip = (float)wall.gltexture->realtexheight/(float)wall.gltexture->buffer_height;
				//        if ( (texturetranslation[seg->sidedef->bottomtexture]!=R_TextureNumForName("-")) )
				if (seg->sidedef->bottomtexture)
					floormax=MAX(seg->frontsector->floorheight,seg->backsector->floorheight);
				else
					floormax=floor_height;
				if (seg->sidedef->toptexture)
					ceilingmin=MIN(seg->frontsector->ceilingheight,seg->backsector->ceilingheight);
				else
					ceilingmin=ceiling_height;
				linelen=abs(ceiling_height-floor_height);
				wall.ytop=((float)MIN(ceilingmin, ceiling_height)/(float)MAP_SCALE);
				wall.ybottom=((float)MAX(floormax, floor_height)/(float)MAP_SCALE);
				wall.flag=GLDWF_M2S;
				wall.ul=OU((wall),(seg))+(0.0f);
				wall.ur=OU(wall,(seg))+((seg->length)/(float)wall.gltexture->buffer_width);
				if (floormax<=floor_height)
					wall.vb=mip*1.0f;
				else
					wall.vb=mip*((float)(ceiling_height - floormax))/linelen;
				if (ceilingmin>=ceiling_height)
					wall.vt=0.0f;
				else
					wall.vt=mip*((float)(ceiling_height - ceilingmin))/linelen;
			}
			
            if (seg->linedef->tranlump >= 0 && general_translucency)
                wall.alpha=(float)tran_filter_pct/100.0f;
			wall.alpha=1.0f;
			ADDWALL(&wall);
		}
	bottomtexture:
		/* bottomtexture */
		ceiling_height=thebacksector->floorheight;
		floor_height=thefrontsector->floorheight;
#ifdef SKYWALLS	
		if (thefrontsector->floorpic==skyflatnum)
		{
			wall.ybottom=-255.0f;
			if (
				(thebacksector->ceilingheight==thebacksector->floorheight) &&
				(thebacksector->floorpic==skyflatnum)
				)
			{
				wall.ytop=(float)thebacksector->floorheight/MAP_SCALE;
				SKYTEXTURE(thefrontsector->sky,thebacksector->sky);
				ADDWALL(&wall);
			}
			else
			{
				if ( (texturetranslation[seg->sidedef->bottomtexture]!=NO_TEXTURE) )
				{
					wall.ytop=(float)thefrontsector->floorheight/MAP_SCALE;
					SKYTEXTURE(thefrontsector->sky,thebacksector->sky);
					ADDWALL(&wall);
				}
				else
					if ( (thebacksector->floorheight >= thefrontsector->ceilingheight) ||
						(thebacksector->floorpic != skyflatnum) )
					{
						wall.ytop=(float)thebacksector->floorheight/MAP_SCALE;
						SKYTEXTURE(thefrontsector->sky,thebacksector->sky);
						ADDWALL(&wall);
					}
			}
		}
#endif		
		if (floor_height<ceiling_height)
		{
			temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->bottomtexture], true, false);
			if ( !temptex ) {
				temptex = defaultTexture;	// it seems some line segments have bad data...
			}
			wall.gltexture=temptex;
			CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
			CALC_TEX_VALUES_BOTTOM(
								   wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0,
								   seg->length, lineheight,
								   floor_height-thefrontsector->ceilingheight
								   );
			ADDWALL(&wall);
		}
	}
}

#undef LINE
#undef CALC_Y_VALUES
#undef OU
#undef OV
#undef OV_PEG
#undef CALC_TEX_VALUES_TOP
#undef CALC_TEX_VALUES_MIDDLE1S
#undef CALC_TEX_VALUES_MIDDLE2S
#undef CALC_TEX_VALUES_BOTTOM
#undef SKYTEXTURE
#undef ADDWALL



/*
 TransformAndClipSegment
 
 Converts a world coordinate line segment to screen space.
 Returns false if the segment is off screen.
 
 There would be some savings if all the points in a subsector
 were transformed and clip tested as a unit, instead of as discrete segments.
 */
boolean TransformAndClipSegment( float v[2][2], float ends[2] ) {
	float	clip[2][4];
	
	// if we are in iphone reverse-landscape mode, we need
	// to flip the coordinates around
	float	*v0, *v1;
	//if ( reversedLandscape ) {
	//	v0 = v[1];
	//	v1 = v[0];
	//} else {
		v0 = v[0];
		v1 = v[1];
	//}
	
	// transform from model to clip space
	// because the iPhone screen hardware is portrait mode,
	// we need to look at the Y axis for the segment ends,
	// not the X axis.
	clip[0][1] = v0[0] * glMVPmatrix[1] + v0[1] * glMVPmatrix[2*4+1] + glMVPmatrix[3*4+1];
	clip[0][3] = v0[0] * glMVPmatrix[3] + v0[1] * glMVPmatrix[2*4+3] + glMVPmatrix[3*4+3];
	
	clip[1][1] = v1[0] * glMVPmatrix[1] + v1[1] * glMVPmatrix[2*4+1] + glMVPmatrix[3*4+1];
	clip[1][3] = v1[0] * glMVPmatrix[3] + v1[1] * glMVPmatrix[2*4+3] + glMVPmatrix[3*4+3];

	float	d0, d1;
	
	// clip to the near plane
	float	nearClip = 0.01f;
	d0 = clip[0][3] - nearClip;
	d1 = clip[1][3] - nearClip;
	if ( d0 < 0 && d1 < 0 ) {
		// near clipped
		return false;
	}
	if ( d0 < 0 ) {
		float f = d0 / ( d0 - d1 );
		clip[0][1] = clip[0][1] + f * ( clip[1][1] - clip[0][1] );
		clip[0][3] = nearClip;
	} else if ( d1 < 0 ) {
		float f = d1 / ( d1 - d0 );
		clip[1][1] = clip[1][1] + f * ( clip[0][1] - clip[1][1] );
		clip[1][3] = nearClip;
	}

	if ( clip[0][1] > clip[0][3] ) {
		// entire segment is off the right side of the screen
		return false;
	}
	if ( clip[1][1] < -clip[1][3] ) {
		// entire segment is off the left side of the screen
		return false;
	}
		
	// project
	for ( int i = 0 ; i < 2 ; i++ ) {
		float x = viewwidth * ( ( clip[i][1] / clip[i][3] ) * 0.5f + 0.5f ); 		
		if ( x < 0 ) {
			x = 0;
		} else if ( x > viewwidth ) {
			x = viewwidth;
		}
		ends[i] = x;
	}
	
	// part of the segment is on screen
	return true;
}

/*
 IR_Subsector
 
 All possible culling should be performed here, but most calculations should be
 deferred until draw time, rather than storing intermediate values that are
 later referenced.
 
 Don't make this static, or the compiler inlines it in the recursive node
 function, which bloats the stack.
*/ 
void IR_Subsector(int num)
{
	subsector_t *sub = &subsectors[num];
	c_subsectors++;
	// at this point we know that at least part of the subsector is
	// not covered in the occlusion array
	
	// if the sector that this subsector is a part of has not already had its
	// planes and sprites added, add them now.
	sector_t *thefrontsector = sub->sector;
	int lightlevel = thefrontsector->lightlevel+(extralight<<5);

	// There can be several subsectors in each sector due to non-convex
	// sectors or BSP splits, but we draw the floors, ceilings and lines
	// with a single draw call for the entire thing, so ensure that they
	// are only added once per frame.
	if ( thefrontsector->validcount != validcount ) {
		thefrontsector->validcount = validcount;
		
		c_sectors++;
		GLFlat flat;
		flat.sectornum = thefrontsector->iSectorID;
		flat.light = lightlevel;
		flat.uoffs= 0;	// no support in standard doom
		flat.voffs= 0;
		
		if ( thefrontsector->floorheight < viewz ) {
			if (thefrontsector->floorpic == skyflatnum) {
				skyIsVisible = true;
			} else {
				// get the texture. flattranslation is maintained by doom and
				// contains the number of the current animation frame
				GLTexture *tex = gld_RegisterFlat(flattranslation[thefrontsector->floorpic], true);
				if ( tex ) {
					sectorPlanes[numSectorPlanes].texture = tex;
					sectorPlanes[numSectorPlanes].ceiling = false;
					sectorPlanes[numSectorPlanes].sector = thefrontsector;
					numSectorPlanes++;
				}
			}
		}
		if ( thefrontsector->ceilingheight > viewz ) {
			if (thefrontsector->ceilingpic == skyflatnum) {
				skyIsVisible = true;
			} else {
				// get the texture. flattranslation is maintained by doom and
				// contains the number of the current animation frame
				GLTexture *tex = gld_RegisterFlat(flattranslation[thefrontsector->ceilingpic], true);
				if ( tex ) {
					sectorPlanes[numSectorPlanes].texture = tex;
					sectorPlanes[numSectorPlanes].ceiling = true;
					sectorPlanes[numSectorPlanes].sector = thefrontsector;
					numSectorPlanes++;
				}
			}
		}
		
		// Add all the sprites in this sector.
		// It would be better if they were linked into all the subsectors, because
		// we could do more accurate occlusion culling.  With non-convex sectors,
		// occasionally a sprite will be added in a rear portion of the sector that
		// would have been occluded away if everything was done in BSP subsector order.
		for ( mobj_t *thing = thefrontsector->thinglist; thing; thing = thing->snext) {
			IR_ProjectSprite( thing, lightlevel );
		}
	}
	
	// If a segment in this subsector is not fully occluded, mark
	// the line that it is a part of as needing to be drawn.  Because
	// we are using a depth buffer, we can draw complete line segments
	// instead of just segments.
	for ( int i = 0 ; i < sub->numlines ; i++ ) {
		seg_t *seg = &segs[sub->firstline+i];
		
		line_t *line = seg->linedef;

		// Determine if it will completely occlude farther objects.
		// Given that changing sector heights is much less common than
		// traversing lines during every render, it would be marginally better if
		// lines had an "occluder" flag on them that was updated as sectors
		// moved, but it hardly matters.
		boolean	occluder;
		if ( seg->backsector == NULL || 
			seg->backsector->floorheight >= seg->backsector->ceilingheight ||
			seg->backsector->floorheight >= seg->frontsector->ceilingheight ||
			seg->backsector->ceilingheight <= seg->frontsector->floorheight ) {
			// this segment can't be seen past, so fill in the occlusion table
			occluder = true;
		} else {
			// If the line has already been made visible and we don't need to
			// update the occlusion buffer, we don't need to do anything else here.
			// This happens when a line is split into multiple segs, and also
			// when the line is reached from the backsector.  In the backsector
			// case, it would be back-face culled, but this test throws it out
			// without having to transform and clip the ends.
			if ( line->validcount == validcount ) {
				continue;
			}
			
			// check to see if the seg won't draw any walls at all
			
			// we won't fill in the occlusion table for this
			occluder = false;
		}
	
		// transform and clip the two endpoints
		float	v[2][2];
		float	floatEnds[2];
		v[0][0] = -seg->v1->x/MAP_SCALE;
		v[0][1] = seg->v1->y/MAP_SCALE;
		v[1][0] = -seg->v2->x/MAP_SCALE;
		v[1][1] = seg->v2->y/MAP_SCALE;
		if ( !TransformAndClipSegment( v, floatEnds ) ) {
			// the line is off to the side or facing away
			continue;
		}
	
		// Allow segs that we consider to be slightly back
		// facing to still pass through, because GPU floating
		// point calculations may not see them exactly the same.
		if ( floatEnds[0] > floatEnds[1] + 1.0f ) {
			// back face
			continue;
		}
		// Check it against the occlusion buffer.
		// Because the perspective divide is not going to be bit-exact between
		// the CPU and GPU, we check an extra column here.  That will result
		// in an occasional line being drawn that might not need to be, but
		// it avoids missing columns.
		int checkMin = floor( floatEnds[0] - 1 );
		int checkMax = ceil( floatEnds[1] + 1 );
		if ( checkMin < 0 ) {
			checkMin = 0;
		}
		if ( checkMax > viewwidth ) {
			checkMax = viewwidth;
		}
		if ( !memchr( occlusion + checkMin, 0, checkMax - checkMin ) ) {
			failCount++;
			// every column it would touch is already solid, so it isn't visible
			continue;
		}
		if ( occluder ) {
			// It is important to update the occlusion array as individual
			// segs are processed to maintain pure front to back order.  If
			// the occlusion buffer was updated by complete lines, it would
			// result in some elements being incorrectly occlusion culled.
			
			// Use a consistant fill rule for the occlusion, which is only
			// referenced by the CPU, and should be water tight.
			int fillMin = (int) (floatEnds[0]+0.5);
			int fillMax = (int) (floatEnds[1]+0.5);
			if ( fillMax > fillMin ) {
				memset( occlusion + fillMin, 1, fillMax-fillMin );
			}
		}

		if ( line->validcount == validcount ) {
			continue;
		}
		line->validcount = validcount;

		// this line can show up on the automap now
		line->flags |= ML_MAPPED;
	
		// Adding a line may generate up to four drawn walls -- a top wall,
		// a bottom wall, a perforated middle wall, and a sky wall.

		// Use the complete, unclipped segment for the side
		IR_AddWall( &seg->sidedef->sideSeg );
	}
}

PUREFUNC static int IR_PointOnSide(fixed_t x, fixed_t y, const node_t *node)
{
	// JDC: these early tests probably aren't worth it on iphone...
	if (!node->dx)
		return x <= node->x ? node->dy > 0 : node->dy < 0;
	
	if (!node->dy)
		return y <= node->y ? node->dx < 0 : node->dx > 0;
	
	x -= node->x;
	y -= node->y;
	
	// Try to quickly decide by looking at sign bits.
	if ((node->dy ^ node->dx ^ x ^ y) < 0)
		return (node->dy ^ x) < 0;  // (left is negative)

	return (__int64_t)y * (__int64_t)node->dx >= (__int64_t)x * (__int64_t)node->dy;
}


static const int checkcoord[12][4] = // killough -- static const
{
{3,0,2,1},
{3,0,2,0},
{3,1,2,0},
{0},
{2,0,2,1},
{0,0,0,0},
{3,1,3,0},
{0},
{2,0,3,1},
{2,1,3,1},
{2,1,3,0}
};
static boolean IR_IsBBoxCompletelyOccluded(const fixed_t *bspcoord) {
	// conservatively accept if close to the box, so
	// we don't need to worry about the near clip plane
	// in TrnasformAndClipSegment.  Mapscale is 128*fracunit
	// and nearclip is 0.1, so accepting 2 fracunits away works.
	if ( viewx > bspcoord[BOXLEFT]-2*FRACUNIT && viewx < bspcoord[BOXRIGHT] + 2*FRACUNIT
		&& viewy > bspcoord[BOXBOTTOM]-2*FRACUNIT && viewy < bspcoord[BOXTOP] + 2*FRACUNIT ) {
		return false;
	}
	
	// Find the corners of the box
	// that define the edges from current viewpoint.
	int boxpos = (viewx <= bspcoord[BOXLEFT] ? 0 : viewx < bspcoord[BOXRIGHT ] ? 1 : 2) +
		(viewy >= bspcoord[BOXTOP ] ? 0 : viewy > bspcoord[BOXBOTTOM] ? 4 : 8);
	
	const int *check = checkcoord[boxpos];
	float	v[2][2];

	v[0][0] = -bspcoord[check[0]]/MAP_SCALE;
	v[0][1] = bspcoord[check[1]]/MAP_SCALE;

	v[1][0] = -bspcoord[check[2]]/MAP_SCALE;
	v[1][1] = bspcoord[check[3]]/MAP_SCALE;
	float		ends[2];
	if ( !TransformAndClipSegment( v, ends ) ) {
		// the line is off to the side or facing away
		return true;
	}
	
	if ( ends[0] >= ends[1] ) {
		return true;
	}
	
	assert( ends[0] >= 0 );
	assert( ends[1] <= viewwidth );
	
	// Check it against the occlusion buffer, with an extra column
	// of slop to account for GPU / CPU floating point differences.
	if ( !memchr( occlusion + (int)ends[0], 0, (int)ends[1]-(int)ends[0]+1 ) ) {
		// every column it would touch is already solid, so it isn't visible
		return true;
	}
	 // there are gaps, so we may need to draw something
	return false;
}
		 
		 
/*
 RenderBSPNode
 
 Renders all subsectors below a given node,
 traversing subtree recursively.
 Because this function is recursive, avoid doing work that
 would give a large stack frame.  Important that the compiler
 doesn't inline big functions.
 */
static void IR_RenderBSPNode( int bspnum ) {		 
	while (!(bspnum & NF_SUBSECTOR)) {
		// decision node
		const node_t *bsp = &nodes[bspnum];
		
		// Decide which side the view point is on.
		int side = IR_PointOnSide(viewx, viewy, bsp);
		 
		// check the front space
		 if ( !IR_IsBBoxCompletelyOccluded(bsp->bbox[side]) ) {
			IR_RenderBSPNode( bsp->children[side] );
		 }
		// continue down the back space
		 if ( IR_IsBBoxCompletelyOccluded( bsp->bbox[side^1]) ) {
			return;
		 }
		bspnum = bsp->children[side^1];
    }
	
	// subsector with contents
	// add all the drawable elements in the subsector
	if ( bspnum == -1 ) {
		bspnum = 0;
	}
	bspnum &= ~NF_SUBSECTOR;
	IR_Subsector( bspnum );
}


typedef struct {
	void *texture;
	void *user;
} texSort_t;

static int TexSort( const void *a, const void *b ) {
	if ( ((texSort_t *)a)->texture < ((texSort_t *)b)->texture ) {
		return -1;
	}
	return 1;
}

int SysIphoneMicroseconds(void);
void SetImmediateModeGLVertexArrays(void);
extern float yaw;
extern GLTexture **gld_GLTextures;
extern GLTexture **gld_GLPatchTextures;
extern GLVertex *gld_vertexes;
extern GLTexcoord *gld_texcoords;

#define MAX_DRAW_INDEXES	0x10000
unsigned short	drawIndexes[MAX_DRAW_INDEXES];
int			numDrawIndexes;

drawVert_t		drawVerts[MAX_DRAW_VERTS];
int			numDrawVerts;

void NewDrawScene(player_t *player)	// JDC: new version
{
	int i,k;
	
	glDisable( GL_ALPHA_TEST );
	glDisable(GL_CULL_FACE);
	glDisable(GL_FOG);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	
	// Use the 2x texture combiner mode to allow the game to be brightened up
	// above the normal maximum.  All calculated color values for lighting will
	// be multiplied by a value ranging from 0.5 for original brightness, up to
	// 1.0 for 2x brightness.  This combine mode will be canceled after all
	// the 3D and view weapon drawing is completed, so the hud elements are
	// drawn at normal brightness.
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
	glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0 );
	
	// opaque skies, flats, and walls write to the depth buffer and don't blend
	glDisable( GL_BLEND );
	glDepthMask( 1 );
	
	// debug tool to check for things being drawn that shouldn't be
	if ( blendAll ) {
		glClearColor( 0, 0, 0, 0 );
		glClear( GL_COLOR_BUFFER_BIT );
		glEnable( GL_BLEND );
		glDisable( GL_DEPTH_TEST );
		glBlendFunc( GL_ONE, GL_ONE );
		skyIsVisible = false;
	}
	
	// debug tool to look for pixel cracks
	if ( testClear ) {
		glClearColor( 1, 0, 0, 0 );
		glClear( GL_COLOR_BUFFER_BIT );
		skyIsVisible = false;
	}
	
	//-----------------------------------------
	// draw the sky if needed
	//-----------------------------------------
	if ( skyIsVisible ) {
		float	s;
		float	y;
		
		// Note that these texcoords would have to be corrected
		// for different screen aspect ratios or fields of view!
		s = ((yaw+90.0f)/90.0f);
		y = 1 - 2 * 128.0 / 200;
		
		// With identity matricies, the vertex coordinates
		// can just be in the 0-1 range.
		glMatrixMode( GL_MODELVIEW );
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode( GL_PROJECTION );
		glPushMatrix();
		glLoadIdentity();
		
		gld_BindTexture( gld_RegisterTexture( skytexture, true, true ) );
		glColor4f( 0.5, 0.5, 0.5, 1.0 );	// native texture color, not double bright
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f( s, 1 ); glVertex3f(-1,y,0.999);
		glTexCoord2f( s, 0 ); glVertex3f(-1,1,0.999);
		glTexCoord2f( s+1, 1 ); glVertex3f(1,y,0.999);
		glTexCoord2f( s+1, 0 ); glVertex3f(1,1,0.999);
		glEnd();
		
		// back to the normal drawing matrix
		glPopMatrix();
		glMatrixMode( GL_MODELVIEW );
		glPopMatrix();
	}

	// walls and flats use the drawVerts array for everything
	glTexCoordPointer(2,GL_FLOAT,sizeof(drawVert_t),drawVerts[0].st);
	glVertexPointer(3,GL_FLOAT,sizeof(drawVert_t),drawVerts[0].xyz);
	glColorPointer(4,GL_UNSIGNED_BYTE,sizeof(drawVert_t),drawVerts[0].rgba);
	
	// everything will draw at full brightness in this case
	if (player->fixedcolormap) {
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f );
		glDisableClientState(GL_COLOR_ARRAY);
	} else {
		glEnableClientState( GL_COLOR_ARRAY );
	}
	
	int	c_drawElements = 0;
	numDrawIndexes = 0;
	
	//-----------------------------------------
	// draw all the walls, sky walls sorted first
	//-----------------------------------------
	// sort the walls by texture
	texSort_t	*wallSort = alloca( sizeof( wallSort[0] ) * gld_drawinfo.num_walls );
	for (i=0 ; i < gld_drawinfo.num_walls ; i++ ) {
		GLWall *wall = &gld_drawinfo.walls[i];
		wallSort[i].texture = wall->gltexture;
		wallSort[i].user = wall;
	}
	qsort( wallSort, gld_drawinfo.num_walls, sizeof( wallSort[0] ), TexSort );
	
	// continue building drawverts after the floor and ceiling data 
	int		tempDrawVert = numDrawVerts;
	
	// alpha tested walls will use half alpha to get the best edging effects
	glAlphaFunc( GL_GREATER, 0.5 );
	
	for (i=0 ; i < gld_drawinfo.num_walls ; i++ ) {
		texSort_t *sort = &wallSort[i];
		GLWall *wall = sort->user;
		rendered_segs++;
		
		// add two tris to draw the wall
		drawIndexes[numDrawIndexes+0] = tempDrawVert;
		drawIndexes[numDrawIndexes+1] = tempDrawVert+1;
		drawIndexes[numDrawIndexes+2] = tempDrawVert+2;
		
		drawIndexes[numDrawIndexes+3] = tempDrawVert+1;
		drawIndexes[numDrawIndexes+4] = tempDrawVert+2;
		drawIndexes[numDrawIndexes+5] = tempDrawVert+3;
		
		numDrawIndexes += 6;
		
		unsigned char	rgba[4];
		rgba[0] = rgba[1] = rgba[2] = wall->light;
		rgba[3] = 255;
		int lightInt = *(int *)rgba;		
		
		drawVerts[tempDrawVert].st[0] = wall->ul;
		drawVerts[tempDrawVert].st[1] = wall->vt;
		drawVerts[tempDrawVert].xyz[0] = -wall->side->sideSeg.v1->x / MAP_SCALE;
		drawVerts[tempDrawVert].xyz[1] = wall->ytop;
		drawVerts[tempDrawVert].xyz[2] = wall->side->sideSeg.v1->y / MAP_SCALE;
		
		lightInt = FadedLighting( drawVerts[tempDrawVert].xyz[0], drawVerts[tempDrawVert].xyz[2], wall->light );
		
		*(int *)drawVerts[tempDrawVert].rgba = lightInt;
		tempDrawVert++;
		
		drawVerts[tempDrawVert].st[0] = wall->ul;
		drawVerts[tempDrawVert].st[1] = wall->vb;
		drawVerts[tempDrawVert].xyz[0] = -wall->side->sideSeg.v1->x / MAP_SCALE;
		drawVerts[tempDrawVert].xyz[1] = wall->ybottom;
		drawVerts[tempDrawVert].xyz[2] = wall->side->sideSeg.v1->y / MAP_SCALE;
		*(int *)drawVerts[tempDrawVert].rgba = lightInt;
		tempDrawVert++;
		
		drawVerts[tempDrawVert].st[0] = wall->ur;
		drawVerts[tempDrawVert].st[1] = wall->vt;
		drawVerts[tempDrawVert].xyz[0] = -wall->side->sideSeg.v2->x / MAP_SCALE;
		drawVerts[tempDrawVert].xyz[1] = wall->ytop;
		drawVerts[tempDrawVert].xyz[2] = wall->side->sideSeg.v2->y / MAP_SCALE;
		
		lightInt = FadedLighting( drawVerts[tempDrawVert].xyz[0], drawVerts[tempDrawVert].xyz[2], wall->light );
		*(int *)drawVerts[tempDrawVert].rgba = lightInt;
		tempDrawVert++;
		
		drawVerts[tempDrawVert].st[0] = wall->ur;
		drawVerts[tempDrawVert].st[1] = wall->vb;
		drawVerts[tempDrawVert].xyz[0] = -wall->side->sideSeg.v2->x / MAP_SCALE;
		drawVerts[tempDrawVert].xyz[1] = wall->ybottom;
		drawVerts[tempDrawVert].xyz[2] = wall->side->sideSeg.v2->y / MAP_SCALE;
		*(int *)drawVerts[tempDrawVert].rgba = lightInt;
		tempDrawVert++;		
		
		// only draw when textures change
		if ( i == gld_drawinfo.num_walls-1 || sort->texture != (sort+1)->texture ) {
			if ( numDrawIndexes ) {
				if ( wall->flag == GLDWF_SKY ) {
					// we aren't actually drawing anything with this,
					// we are just masking off areas in the depth
					// buffer so nothing can overwrite the already
					// drawn sky image
					glColorMask( 0, 0, 0, 0 );
				}
				if ( wall->flag == GLDWF_M2S ) {
					glEnable( GL_ALPHA_TEST );
				}
				
				if ( wall->gltexture ) {	// skies are texture = NULL
					gld_BindTexture( wall->gltexture );
				}
				
				glDrawElements( GL_TRIANGLES, numDrawIndexes, GL_UNSIGNED_SHORT,
							   drawIndexes );
				
				if ( wall->flag == GLDWF_M1S ) {
					glDisable( GL_ALPHA_TEST );
				}
				if ( wall->flag == GLDWF_SKY ) {
					glColorMask( 1, 1, 1, 1 );
				}
				
				numDrawIndexes = 0;
				tempDrawVert = numDrawVerts;
				c_drawElements++;
			}
		}
		
	}
	
	//-----------------------------------------
	// draw all the flats
	//
	// If we were able to directly fill the GPU command buffers,
	// we would be using multiple DrawArrays instead of a single DrawElements,
	// and we would fill the plane height and color in as we copied vertexes
	// from a single set of verts per sector, but because the driver validation
	// overhead is the main poison on the iPhone currently, it is better to
	// duplicate the windings for the floors and ceilings, patch the
	// vertex data, and generate new index lists to minimize the number of
	// draw calls.
	//-----------------------------------------
	
	// sort the flats by texture
	qsort( sectorPlanes, numSectorPlanes, sizeof( sectorPlanes[0] ), TexSort );
	
	// draw them in texture order
	for (i=0 ; i < numSectorPlanes ; i++ ) {
		sortSectorPlane_t *sort = &sectorPlanes[i];
		sector_t *sector = sort->sector;
		
		drawVert_t *dv = sector->verts[(int)sort->ceiling];
		
		// Patch the height values if they have changed since the last draw.
		float	y = sort->ceiling ? sector->ceilingheight : sector->floorheight;
		y *= ( 1.0 / MAP_SCALE );
		if ( y != dv->xyz[1] ) {
			for ( int j = 0 ; j < sector->numVerts ; j++ ) {
				(dv+j)->xyz[1] = y;
			}
		}

		// per-vertex faded light color
		int	light = sector->lightlevel + (extralight<<5);
		if ( light > 255 ) {
			light = 255;
		}
		for ( int j = 0 ; j < sector->numVerts ; j++ ) {
			*(int *)(dv+j)->rgba = FadedLighting( (dv+j)->xyz[0], (dv+j)->xyz[2], light );
		}
		
		// copy the indexes
		assert( numDrawIndexes + sector->numIndexes < MAX_DRAW_INDEXES );
		memcpy( drawIndexes + numDrawIndexes, sector->indexes[(int)sort->ceiling], sector->numIndexes*sizeof(drawIndexes[0]) );
		numDrawIndexes += sector->numIndexes;
		
		// only change textures when necessary
		if ( i == numSectorPlanes - 1 || sort->texture != (sort+1)->texture ) {
			if ( numDrawIndexes ) {
				gld_BindFlat( sort->texture );			
				glDrawElements( GL_TRIANGLES, numDrawIndexes, GL_UNSIGNED_SHORT,
							   drawIndexes );
				numDrawIndexes = 0;
				c_drawElements++;
			}
		}
	}
	
	glDisableClientState( GL_COLOR_ARRAY );
	
	// back to our immediate mode vertex arrays
	SetImmediateModeGLVertexArrays();
	
	
	//-----------------------------------------
	// draw all the sprites
	//-----------------------------------------
	
	// transparent sprites blend and don't write to the depth buffer
	glEnable( GL_BLEND );
	glDepthMask( 0 );
	
	glEnable( GL_ALPHA_TEST );
	
	// get the screen space vector for sprites
	float	yaws = -sin( yaw * 3.141592657 / 180.0 );
	float	yawc = -cos( yaw * 3.141592657 / 180.0  );
	int c_spriteBind = 0;
	int c_spriteDraw = 0;
	while( 1 )
	{
		// pick out the sprites from farthest to nearest
		fixed_t max_scale=INT_MAX;
		k=-1;
		for (i=0 ; i < gld_drawinfo.num_sprites ; i++ ) {
			GLSprite *sprite = &gld_drawinfo.sprites[i];
			if (sprite->scale<max_scale)
			{
				max_scale=sprite->scale;
				k=i;
			}
		}
		if ( k == -1 ) {
			break;
		}
		
		GLSprite *sprite = &gld_drawinfo.sprites[k];
		sprite->scale=INT_MAX;
		
		if ( sprite->gltexture != last_gltexture ) {
			c_spriteBind++;
		}
		c_spriteDraw++;
		
		gld_BindPatch(sprite->gltexture,sprite->cm);
		if(sprite->shadow)
		{
			glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(0.2f,0.2f,0.2f,0.33f);
			glAlphaFunc( GL_GREATER, 0.1 );	// don't alpha test away the blended-down version
		}
		else
		{
			float	flight = (float)sprite->light*(1.0f/255);
			
			// We could do the distance-lighting here, but leaving the sprites
			// brighter is a good accent in most cases.  There are a few places
			// where environmental sprites look a little wrong, but it is probably
			// better in general.
			
			if (player->fixedcolormap) {
				flight = 1.0;	// light amp goggles
			}
			glColor4f(flight, flight, flight, 1.0f );
		}
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(sprite->ul, sprite->vt); 
		glVertex3f(sprite->x + sprite->x1 * yawc, sprite->y + sprite->y1, sprite->z + sprite->x1 * yaws );
		glTexCoord2f(sprite->ur, sprite->vt); 
		glVertex3f(sprite->x + sprite->x2 * yawc, sprite->y + sprite->y1, sprite->z + sprite->x2 * yaws );
		glTexCoord2f(sprite->ul, sprite->vb); 
		glVertex3f(sprite->x + sprite->x1 * yawc, sprite->y + sprite->y2, sprite->z + sprite->x1 * yaws );
		glTexCoord2f(sprite->ur, sprite->vb); 
		glVertex3f(sprite->x + sprite->x2 * yawc, sprite->y + sprite->y2, sprite->z + sprite->x2 * yaws );
		glEnd();
		
		if(sprite->shadow)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glAlphaFunc( GL_GREATER, 0.5 );	
		}						
	}
	
	glDisable( GL_ALPHA_TEST );
	glDepthMask( 1 );
}


static float roll     = 0.0f;
/* JDC static */ float yaw      = 0.0f;
static float inv_yaw  = 0.0f;
static float pitch    = 0.0f;

#define __glPi 3.14159265358979323846

static void infinitePerspective(GLdouble fovy, GLdouble aspect, GLdouble znear)
{
	GLfloat left, right, bottom, top;	// JDC: was GLdouble
	GLfloat m[16];						// JDC: was GLdouble
	
	top = znear * tan(fovy * __glPi / 360.0);
	bottom = -top;
	left = bottom * aspect;
	right = top * aspect;
	
	//qglFrustum(left, right, bottom, top, znear, zfar);
	
	m[ 0] = (2 * znear) / (right - left);
	m[ 4] = 0;
	m[ 8] = (right + left) / (right - left);
	m[12] = 0;
	
	m[ 1] = 0;
	m[ 5] = (2 * znear) / (top - bottom);
	m[ 9] = (top + bottom) / (top - bottom);
	m[13] = 0;
	
	m[ 2] = 0;
	m[ 6] = 0;
	//m[10] = - (zfar + znear) / (zfar - znear);
	//m[14] = - (2 * zfar * znear) / (zfar - znear);
	m[10] = -1;
	m[14] = -2 * znear;
	
	m[ 3] = 0;
	m[ 7] = 0;
	m[11] = -1;
	m[15] = 0;
	
	glMultMatrixf(m);	// JDC: was glMultMatrixd
}

/*
 IR_RenderPlayerView
 
 Replace the prBoom rendering code with a higher performance
 version.  Most of the fancy new features are gone, because I
 have no idea what the reight test cases would be for them.
 */
void IR_RenderPlayerView (player_t* player) {
	int	start = 0;
	if ( showRenderTime ) {
		start = SysIphoneMicroseconds();
	}
		
	viewplayer = player;
	tic_vars.frac = FRACUNIT;
	
    viewx = player->mo->x;
    viewy = player->mo->y;
    viewz = player->viewz;
    viewangle = player->mo->angle;
	extralight = player->extralight;	// gun flashes
	
	yaw=270.0f-(float)(viewangle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;
	
	viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
	viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];
	
	// IR goggles
	if (player->fixedcolormap) {
		fixedcolormap = fullcolormap + player->fixedcolormap*256*sizeof(lighttable_t);
    } else {
		fixedcolormap = 0;
	}
	
	// this is used to tell if a line, sector, or sprite is going to be drawn this frame
	validcount++;
	
	// clear the 1D occlusion buffer, set a final guard column to occluded so we can
	// check an extra pixel to account for slight floating point differences between
	// the GPU and CPU transformations
	assert( viewwidth <= MAX_SCREENWIDTH );
	memset( occlusion, 0, sizeof( occlusion ) );
	occlusion[viewwidth] = 1;
	
	float trY ;
	float xCamera,yCamera;
	
	extern int screenblocks;
	int height;
	
	gld_SetPalette(-1);
	
	if (screenblocks == 11)
		height = SCREENHEIGHT;
	else if (screenblocks == 10)
		height = SCREENHEIGHT;
	else
		height = (screenblocks*SCREENHEIGHT/10) & ~7;
	
	glViewport(viewwindowx, SCREENHEIGHT-(height+viewwindowy-((height-viewheight)/2)), viewwidth, height);
	glScissor(viewwindowx, SCREENHEIGHT-(viewheight+viewwindowy), viewwidth, viewheight);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glShadeModel(GL_SMOOTH);
	
	// Player coordinates
	xCamera=-(float)viewx/MAP_SCALE;
	yCamera=(float)viewy/MAP_SCALE;
	trY=(float)viewz/MAP_SCALE;
	
	yaw=270.0f-(float)(viewangle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;
	inv_yaw=-90.0f+(float)(viewangle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;
	
#ifdef _DEBUG
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
	
	// To make it easier to accurately mimic the GL model to screen transformation,
	// this is set up so that the projection transformation is also done in the
	// modelview matrix, leaving the projection matrix as an identity.  This means
	// that things done in eye space, like lighting and fog, won't work, but
	// we don't need them.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	infinitePerspective(64.0f, 320.0f/200.0f, 5.0f/100.0f);
	
	glRotatef(roll,  0.0f, 0.0f, 1.0f);
	glRotatef(pitch, 1.0f, 0.0f, 0.0f);
	glRotatef(yaw,   0.0f, 1.0f, 0.0f);
	glTranslatef(-xCamera, -trY, -yCamera);
	
	// read back the matrix so we can do exact calculations that match
	// what GL is doing.  It would probably be better to build the matricies
	// ourselves and just do a loadMatrix...
	glGetFloatv( GL_MODELVIEW_MATRIX, glMVPmatrix );
	
	// setup the vector for calculating light fades, which is just a scale
	// of the forward vector
	lightingVector[0] = lightDistance * glMVPmatrix[2];
	lightingVector[1] = lightDistance * glMVPmatrix[10];
	lightingVector[2] = lightDistance * glMVPmatrix[14];
	
	
	rendermarker++;
	gld_drawinfo.num_walls=0;
	gld_drawinfo.num_flats=0;
	gld_drawinfo.num_sprites=0;
	gld_drawinfo.num_drawitems=0;
	
	c_occludedSprites = 0;
	c_sectors = 0;
	c_subsectors = 0;
	numSectorPlanes = 0;
	failCount = 0;
	
	// Find everything we need to draw, but don't draw anything yet,
	// because we want to sort by texture to reduce GL driver overhead.
	IR_RenderBSPNode( numnodes-1 );
	
    NewDrawScene(player);

    gld_EndDrawScene();	
	
	if ( showRenderTime ) {
		int end = SysIphoneMicroseconds();
		printf( "%i usec\n", end - start );
	}
}


