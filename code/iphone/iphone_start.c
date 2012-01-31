/*
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.
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


#include "../doomiphone.h"


/*
 ==================
 ResumeGame
 
 ==================
 */
void ResumeGame() {
	if ( levelHasBeenLoaded && !demoplayback ) {
		// return to the already started game
		iphoneResumeMusic();
		drawWeaponSelect = false;
		weaponSelected = -1;
		automapmode = 0;
		advancedemo = false;			
		menuState = IPM_GAME;
        lastState = IPM_GAME;
		return;
	}
	
	if ( !playState.saveGameIsValid || !G_SaveGameValid() ) {
		// they hit "resume game" on the first app lounch, so just start E1M1
		mapStart_t	map;
		map.skill = 1;
		map.episode = 1;
		map.map = 1;
		StartSinglePlayerGame( map );
        lastState = IPM_GAME;
	} else {
		StartSaveGame();
        lastState = IPM_GAME;
	}
}


/*
 ==================
 GameSetup
 
 ==================
 */
void GameSetup() {
	// resume game just goes back to playing, it doesn't load anything
	levelHasBeenLoaded = true;
	
	// make sure doom doesn't cycle to the next demo loop and kill the new game
	advancedemo = false;	

	// not in a timedemo yet
	timeDemoStart = 0;
	iphoneTimeDemo = false;
	
	// display the game next time through the main loop
	drawWeaponSelect = false;
	weaponSelected = -1;
	automapmode = 0;
	menuState = IPM_GAME;

	demoplayback = false;

	levelTimer = false;
	levelFragLimit = false;

	// single player game
	netgame = false;
	deathmatch = false;
	nomonsters = false;
	memset( playeringame, 0, sizeof( playeringame ) );
	consoleplayer = 0;
	displayplayer = 0;
	playeringame[consoleplayer] = 1;	
}

/*
 =======================
 StartSaveGame
 
 Can be called by both the resume game button after launch, or the
 load game button after a player death
 =======================
 */ 
void StartSaveGame() {
	GameSetup();
	G_LoadGame( 0, true );
	G_DoLoadGame();
}

/*
 =======================
 StartSinglePlayerGame
 
 =======================
 */ 
void StartSinglePlayerGame( mapStart_t map ) {
	playState.map = map;
	playState.saveGameIsValid = true;	// assume we will save the game on exit
	lastState = IPM_GAME;
    
	// mark this level / skill combination as tried
	// 
	mapStats_t *cms = FindMapStats( playState.map.dataset, playState.map.episode, playState.map.map, true );
	if ( cms ) {
		// if we are at MAX_MAPS, no stat tracking...
		cms->completionFlags[playState.map.skill] |= MF_TRIED;
	}
	
	GameSetup();
	
	// start the map
	G_InitNew( playState.map.skill, playState.map.episode, playState.map.map );		
}


/*
 =======================
 StartNetGame
 
 Begins a game based on the contents of setupPacket
 =======================
 */ 
boolean StartNetGame() {
	// make sure we are supposed to be in this game
	int	slot = -1;
	for ( int i = 0 ; i < MAXPLAYERS ; i++ ) {
		if ( setupPacket.playerID[i] == playerID ) {
			slot = i;
		}
	}
	if ( slot == -1 ) {
		return false;
	}
	GameSetup();
	
	consoleplayer = displayplayer = slot;

	netgame = true;			// respawn without restarting levels
	
	if ( setupPacket.deathmatch ) {
		// deathmatch game
		deathmatch = setupPacket.deathmatch;	// could be either 1 or 2 for altdeath
		nomonsters = true;
		
		if ( setupPacket.timelimit > 0 ) {
			levelTimer = true;
			// 30 hz, minutes
			levelTimeCount = setupPacket.timelimit * 30 * 60;
		}
		
		if ( setupPacket.fraglimit > 0 ) {
			levelFragLimit = true;
			levelFragLimitCount = setupPacket.fraglimit;
		}
	} else {
		// coop game
		deathmatch = false;
		nomonsters = false;	
	}
	
	for ( int i = 0 ; i < MAXPLAYERS ; i++ ) {
		if ( setupPacket.playerID[i] != 0 ) {
			playeringame[i] = 1;
		} else {
			playeringame[i] = 0;
		}
	}
	
	gametic = 0;
	maketic = 1;	// allow everyone to run the first frame without waiting for a packet
	
	memset( netcmds, 0, sizeof( netcmds ) );
	memset( consistancy, 0, sizeof( consistancy ) );
	
	gameID = setupPacket.gameID;
	
	// start the map
	G_InitNew( setupPacket.map.skill, setupPacket.map.episode, setupPacket.map.map );
	
	return true;
}

/*
 =======================
 StartDemoGame
 
 The demo button has been hit on the main menu
 =======================
 */ 
void StartDemoGame( boolean timeDemoMode ) {
	if ( levelHasBeenLoaded && !netgame && !demoplayback && usergame && gamestate == GS_LEVEL ) {
		// save the current game before starting the demos
		levelHasBeenLoaded = false;
		G_SaveGame( 0, "quicksave" );
		G_DoSaveGame(true);
	}
    lastState = IPM_GAME;
    
	GameSetup();
	if ( timeDemoMode ) {
		iphoneTimeDemo = true;
	}

	// always skip to the next one on each exit from the menu
	advancedemo = true;
}

