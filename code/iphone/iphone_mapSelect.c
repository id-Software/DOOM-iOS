/*
 *  iphone_mapSelect.c
 *  doom
 *
 *  Created by John Carmack on 4/19/09.
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


#include "doomiphone.h"

typedef struct {
	int		dataset;
	int		episode;
	int		map;
	const char *name;
} mapData_t;

mapData_t mapData[] = {
{ 0, 1, 1,    "E1M1: Hangar" },
{ 0, 1, 2,    "E1M2: Nuclear Plant" },
{ 0, 1, 3,    "E1M3: Toxin Refinery" },
{ 0, 1, 4,    "E1M4: Command Control" },
{ 0, 1, 5,    "E1M5: Phobos Lab" },
{ 0, 1, 6,    "E1M6: Central Processing" },
{ 0, 1, 7,    "E1M7: Computer Station" },
{ 0, 1, 8,    "E1M8: Phobos Anomaly" },
{ 0, 1, 9,    "E1M9: Military Base" },

{ 0, 2, 1,    "E2M1: Deimos Anomaly" },
{ 0, 2, 2,    "E2M2: Containment Area" },
{ 0, 2, 3,    "E2M3: Refinery" },
{ 0, 2, 4,    "E2M4: Deimos Lab" },
{ 0, 2, 5,    "E2M5: Command Center" },
{ 0, 2, 6,    "E2M6: Halls of the Damned" },
{ 0, 2, 7,    "E2M7: Spawning Vats" },
{ 0, 2, 8,    "E2M8: Tower of Babel" },
{ 0, 2, 9,    "E2M9: Fortress of Mystery" },

{ 0, 3, 1,    "E3M1: Hell Keep" },
{ 0, 3, 2,    "E3M2: Slough of Despair" },
{ 0, 3, 3,    "E3M3: Pandemonium" },
{ 0, 3, 4,    "E3M4: House of Pain" },
{ 0, 3, 5,    "E3M5: Unholy Cathedral" },
{ 0, 3, 6,    "E3M6: Mt. Erebus" },
{ 0, 3, 7,    "E3M7: Limbo" },
{ 0, 3, 8,    "E3M8: Dis" },
{ 0, 3, 9,    "E3M9: Warrens" },

{ 0, 4, 1,    "E4M1: Hell Beneath" },
{ 0, 4, 2,    "E4M2: Perfect Hatred" },
{ 0, 4, 3,    "E4M3: Sever The Wicked" },
{ 0, 4, 4,    "E4M4: Unruly Evil" },
{ 0, 4, 5,    "E4M5: They Will Repent" },
{ 0, 4, 6,    "E4M6: Against Thee Wickedly" },
{ 0, 4, 7,    "E4M7: And Hell Followed" },
{ 0, 4, 8,    "E4M8: Unto The Cruel" },
{ 0, 4, 9,    "E4M9: Fear" },

#if 0

{ 0, 0, 0,       "level 1: entryway" },
{ 0, 0, 0,       "level 2: underhalls" },
{ 0, 0, 0,       "level 3: the gantlet" },
{ 0, 0, 0,       "level 4: the focus" },
{ 0, 0, 0,       "level 5: the waste tunnels" },
{ 0, 0, 0,       "level 6: the crusher" },
{ 0, 0, 0,       "level 7: dead simple" },
{ 0, 0, 0,       "level 8: tricks and traps" },
{ 0, 0, 0,       "level 9: the pit" },
{ 0, 0, 0,      "level 10: refueling base" },
{ 0, 0, 0,      "level 11: 'o' of destruction!" },
{ 0, 0, 0,      "level 12: the factory" },
{ 0, 0, 0,      "level 13: downtown" },
{ 0, 0, 0,      "level 14: the inmost dens" },
{ 0, 0, 0,      "level 15: industrial zone" },
{ 0, 0, 0,      "level 16: suburbs" },
{ 0, 0, 0,      "level 17: tenements" },
{ 0, 0, 0,      "level 18: the courtyard" },
{ 0, 0, 0,      "level 19: the citadel" },
{ 0, 0, 0,      "level 20: gotcha!" },
{ 0, 0, 0,      "level 21: nirvana" },
{ 0, 0, 0,      "level 22: the catacombs" },
{ 0, 0, 0,      "level 23: barrels o' fun" },
{ 0, 0, 0,      "level 24: the chasm" },
{ 0, 0, 0,      "level 25: bloodfalls" },
{ 0, 0, 0,      "level 26: the abandoned mines" },
{ 0, 0, 0,      "level 27: monster condo" },
{ 0, 0, 0,      "level 28: the spirit world" },
{ 0, 0, 0,      "level 29: the living end" },
{ 0, 0, 0,      "level 30: icon of sin" },
{ 0, 0, 0,      "level 31: wolfenstein" },
{ 0, 0, 0,      "level 32: grosse" },

{ 0, 0, 0,      "level 1: congo" },
{ 0, 0, 0,      "level 2: well of souls" },
{ 0, 0, 0,      "level 3: aztec" },
{ 0, 0, 0,      "level 4: caged" },
{ 0, 0, 0,      "level 5: ghost town" },
{ 0, 0, 0,      "level 6: baron's lair" },
{ 0, 0, 0,      "level 7: caughtyard" },
{ 0, 0, 0,      "level 8: realm" },
{ 0, 0, 0,      "level 9: abattoire" },
{ 0, 0, 0,     "level 10: onslaught" },
{ 0, 0, 0,     "level 11: hunted" },
{ 0, 0, 0,     "level 12: speed" },
{ 0, 0, 0,     "level 13: the crypt" },
{ 0, 0, 0,     "level 14: genesis" },
{ 0, 0, 0,     "level 15: the twilight" },
{ 0, 0, 0,     "level 16: the omen" },
{ 0, 0, 0,     "level 17: compound" },
{ 0, 0, 0,     "level 18: neurosphere" },
{ 0, 0, 0,     "level 19: nme" },
{ 0, 0, 0,     "level 20: the death domain" },
{ 0, 0, 0,     "level 21: slayer" },
{ 0, 0, 0,     "level 22: impossible mission" },
{ 0, 0, 0,     "level 23: tombstone" },
{ 0, 0, 0,     "level 24: the final frontier" },
{ 0, 0, 0,     "level 25: the temple of darkness" },
{ 0, 0, 0,     "level 26: bunker" },
{ 0, 0, 0,     "level 27: anti-christ" },
{ 0, 0, 0,     "level 28: the sewers" },
{ 0, 0, 0,     "level 29: odyssey of noises" },
{ 0, 0, 0,     "level 30: the gateway of hell" },
{ 0, 0, 0,     "level 31: cyberden" },
{ 0, 0, 0,     "level 32: go 2 it" },

{ 0, 0, 0,      "level 1: system control" },
{ 0, 0, 0,      "level 2: human bbq" },
{ 0, 0, 0,      "level 3: power control" },
{ 0, 0, 0,      "level 4: wormhole" },
{ 0, 0, 0,      "level 5: hanger" },
{ 0, 0, 0,      "level 6: open season" },
{ 0, 0, 0,      "level 7: prison" },
{ 0, 0, 0,      "level 8: metal" },
{ 0, 0, 0,      "level 9: stronghold" },
{ 0, 0, 0,     "level 10: redemption" },
{ 0, 0, 0,     "level 11: storage facility" },
{ 0, 0, 0,     "level 12: crater" },
{ 0, 0, 0,     "level 13: nukage processing" },
{ 0, 0, 0,     "level 14: steel works" },
{ 0, 0, 0,     "level 15: dead zone" },
{ 0, 0, 0,     "level 16: deepest reaches" },
{ 0, 0, 0,     "level 17: processing area" },
{ 0, 0, 0,     "level 18: mill" },
{ 0, 0, 0,     "level 19: shipping/respawning" },
{ 0, 0, 0,     "level 20: central processing" },
{ 0, 0, 0,     "level 21: administration center" },
{ 0, 0, 0,     "level 22: habitat" },
{ 0, 0, 0,     "level 23: lunar mining project" },
{ 0, 0, 0,     "level 24: quarry" },
{ 0, 0, 0,     "level 25: baron's den" },
{ 0, 0, 0,     "level 26: ballistyx" },
{ 0, 0, 0,     "level 27: mount pain" },
{ 0, 0, 0,     "level 28: heck" },
{ 0, 0, 0,     "level 29: river styx" },
{ 0, 0, 0,     "level 30: last call" },
{ 0, 0, 0,     "level 31: pharaoh" },
{ 0, 0, 0,     "level 32: caribbean" },

#endif

{ 0, 0, 0,  NULL }

};


/*
 ===================
 FindMapStats
 
 Finds or creats a mapStats_t structure for the given level.
 This can return NULL if the entire array is filled up, which may
 happen when people have an absurd number of downloaded levels.
 ===================
 */
mapStats_t *FindMapStats( int dataset, int theEpisode, int map, boolean create ) {
	for ( int i = 0 ; i < playState.numMapStats ; i++ ) {
		mapStats_t *ms = &playState.mapStats[i];
		if ( ms->dataset == dataset && ms->episode == theEpisode && ms->map == map ) {
			return ms;
		}
	}
	if ( playState.numMapStats == MAX_MAPS ) {
		// all full.
		return NULL;
	}
	
	if ( !create ) {
		return NULL;
	}
	mapStats_t *cms = &playState.mapStats[playState.numMapStats];
	cms->dataset = dataset;
	cms->episode = theEpisode;
	cms->map = map;
	playState.numMapStats++;
	
	return cms;
}

/*
 ==================
 FindMapName
 
 episodes and maps are one base 
 ==================
 */
const char *FindMapName( int dataset, int theEpisode, int map ) {
	for ( mapData_t *md = mapData ; md->name ; md++ ) {
		if ( md->dataset == dataset && md->episode == theEpisode && md->map == map ) {
			return md->name;
		}
	}
	return "UNKNOWN MAP NAME";
}

