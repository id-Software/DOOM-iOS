/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *   Setup a game, startup stuff.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __P_SETUP__
#define __P_SETUP__

#include "p_mobj.h"


void P_SetupLevel(int episode, int map, int playermask, skill_t skill);
void P_Init(void);               /* Called by startup code. */

extern const byte *rejectmatrix;   /* for fast sight rejection -  cph - const* */

/* killough 3/1/98: change blockmap from "short" to "long" offsets: */
extern long     *blockmaplump;   /* offsets in blockmap are from here */
extern long     *blockmap;
extern int      bmapwidth;
extern int      bmapheight;      /* in mapblocks */
extern fixed_t  bmaporgx;
extern fixed_t  bmaporgy;        /* origin of block map */
extern mobj_t   **blocklinks;    /* for thing chains */

#endif
