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

typedef struct {
	int	msecFromLast;
	int	msecToExecute;
	int	sent;
	int	received;
	int	latency;
} asyncStats_t;

#define MAX_ASYNC_LOGS	256
static asyncStats_t	asyncStats[MAX_ASYNC_LOGS];
int	asyncTicNum;

// we save this for the packet acknowledge, and also for debugging
static packetServer_t	lastServerPacket;

/*
 ==================
 ShowNet
 
 Graph packet receives and transmits
 ==================
 */
void ShowNet() {
	if ( !showNet->value ) {
		return;
	}
	color4_t red = { 255, 0, 0, 255 };
	color4_t green = { 0, 255, 0, 255 };
	color4_t blue = { 0, 0, 255, 255 };
	
	int	now = asyncTicNum;	// latch it in case it changes
	
	for ( int i = 1 ; i < 30 ; i++ ) {
		asyncStats_t *lt = &asyncStats[(now - i ) & (MAX_ASYNC_LOGS-1)];
		R_Draw_Fill( 0, i * 4, lt->sent * 10, 2, red );
		R_Draw_Fill( 100, i * 4, lt->received * 10, 2, green );
		R_Draw_Fill( 200, i * 4, lt->latency * 10, 2, blue );
	}
}

void ShowMiniNet() {
	if ( !miniNet->value ) {
		return;
	}
	color4_t red = { 255, 0, 0, 255 };
	color4_t green = { 0, 255, 0, 255 };
	color4_t blue = { 0, 0, 255, 255 };
	
	int	now = asyncTicNum;	// latch it in case it changes
	now--;
	
	int x = huds.menu.x;
	int y = huds.menu.y;
	
	for ( int i = 0 ; i < 10 ; i++ ) {
		asyncStats_t *lt = &asyncStats[(now - i ) & (MAX_ASYNC_LOGS-1)];
		R_Draw_Fill( x, y+i * 4, lt->sent * 4, 2, red );
		R_Draw_Fill( x+20, y+i * 4, lt->received * 4, 2, green );
		R_Draw_Fill( x+40, y+i * 4, lt->latency * 4, 2, blue );
	}
}

/*
 ==================
 UpdatePeerTiming
 
 Calculates one way latency based on local and remote times
 ==================
 */
void UpdatePeerTiming( netPeer_t *peer, int remoteMilliseconds ) {
	peer->lastPacketAsyncTic == asyncTicNum;
	peer->lastPacketTime = SysIphoneMilliseconds();
	peer->lastTimeDelta = abs( remoteMilliseconds - peer->lastPacketTime );
	if ( peer->lowestTimeDelta == 0 || peer->lastTimeDelta < peer->lowestTimeDelta ) {
		peer->lowestTimeDelta = peer->lastTimeDelta;
	}
	peer->oneWayLatency = peer->lastTimeDelta - peer->lowestTimeDelta;
	if ( peer->oneWayLatency < 0 ) {
		// this can happen if we context switched at a bad time
		peer->lowestTimeDelta = peer->lastTimeDelta;
		peer->oneWayLatency = 0;
	}
//	printf( "OWL:%i timeDelta:%i  lowest:%i\n", peer->oneWayLatency,
//		   peer->lastTimeDelta, peer->lowestTimeDelta );
}

/*
 ==================
 iphoneProcessPacket
 
 A packet has been received over WiFi or bluetooth
 ==================
 */
void iphoneProcessPacket( const struct sockaddr *from, const void *data, int len ) {
	if ( len < 4 ) {
		printf( "discarding packet because len = %i.\n", len );
		return;
	}
	int	packetID = *(int *)data;
	
	if ( !netgame ) {
		// setup and join are only processed while in the menu system
		
		if ( packetID == PACKET_VERSION_SETUP ) {
			if ( localGameID == setupPacket.gameID ) {
				// if we are sending packets, always ignore other setup packets
				printf( "discarding setup packet because we are the server\n" );
				return;
			}
			setupPacketFrameNum = iphoneFrameNum;

			// save this packet
//			printf( "valid setup packet\n" );
			setupPacket = *(packetSetup_t *)data;
			return;
		}
		
		if ( packetID == PACKET_VERSION_JOIN ) {
			// we should only process join packets if we are in the multiplayer
			// menu and running the current game
			if ( menuState != IPM_MULTIPLAYER ) {
				printf( "discarding join packet because not in IPM_MULTIPLAYER\n" );
				return;
			}
			if ( setupPacket.gameID != localGameID ) {
				printf( "discarding join packet because we aren't the server\n" );
				return;
			}
			
			packetJoin_t *pj = (packetJoin_t *)data;
			if ( pj->playerID == 0 ) {
				// should never happen
				printf( "discarding join packet because playerID is 0\n" );
				return;
			}
			// add this player
			int	i;
			for ( i = 0 ; i < MAXPLAYERS ; i++ ) {
				if ( setupPacket.playerID[i] == pj->playerID ) {
					netPlayers[i].peer.lastPacketTime = SysIphoneMilliseconds();
					break;
				}
			}
			if ( i == MAXPLAYERS ) {
				// not in yet, add if possible
				for ( i = 0 ; i < MAXPLAYERS ; i++ ) {
					if ( setupPacket.playerID[i] == 0 ) {
						setupPacket.playerID[i] = pj->playerID;
						netPlayers[i].peer.address = *from;
						netPlayers[i].peer.lastPacketTime = SysIphoneMilliseconds();
						break;
					}
				}
				// if all players are active, the new join gets ignored
			}
//			printf( "valid join packet\n" );
			return;
		}
		
		// no other packets are processed unless we are in the game
		printf( "discarding packet with id 0x%x when not in netgame\n", packetID );
		return;
	}
	
	// the following are only for running games
	
	if ( consoleplayer == 0 ) {
		// we are the server, and should only receive packetClient_t
		if ( packetID != PACKET_VERSION_CLIENT ) {
			static boolean typeErrorPrinted;
			if ( !typeErrorPrinted ) {
				typeErrorPrinted = true;
				printf( "Packet received with type 0x%x instead of 0x%x\n", packetID, PACKET_VERSION_CLIENT );
			}
			return;
		}
		packetClient_t		*pc = (packetClient_t *)data;
		if ( len != sizeof( *pc ) ) {
			// this should always be an exact length match
			return;
		}
		
		if ( pc->gameID != gameID ) {
			static boolean gameErrorPrinted;
			if ( !gameErrorPrinted ) {
				gameErrorPrinted = true;
				printf( "Packet received with gameID 0x%x instead of 0x%x\n", pc->gameID, gameID );
			}
			return;
		}
		assert( pc->consoleplayer > 0 && pc->consoleplayer < MAXPLAYERS );
		
		netPlayer_t *np = &netPlayers[pc->consoleplayer];
		if ( np->pc.packetSequence >= pc->packetSequence ) {
			printf( "Out of order or duplicated packet from player %i\n", pc->consoleplayer );
			return;
		}
		np->peer.currentPingTics = packetSequence - pc->packetAcknowledge;
		if ( np->pc.packetSequence != pc->packetSequence - 1 ) {
			printf( "Dropped %i packets from player %i\n", pc->packetSequence - 1 - np->pc.packetSequence,
				   pc->consoleplayer );
		}
		
		// good packet from client
		np->pc = *pc;
		UpdatePeerTiming( &np->peer, np->pc.milliseconds );
	} else {
		// we are a client, and should only receive server packets
		if ( packetID != PACKET_VERSION_SERVER ) {
			static boolean typeErrorPrinted;
			if ( !typeErrorPrinted ) {
				typeErrorPrinted = true;
				printf( "Packet received with type 0x%x instead of 0x%x\n", packetID, PACKET_VERSION_CLIENT );
			}
			return;
		}
		packetServer_t		*ps = (packetServer_t *)data;
		if ( len > sizeof( *ps ) ) {
			// packets will usually have less ticcmd_t, but never more
			return;
		}
		
		if ( ps->gameID != gameID ) {
			static boolean gameErrorPrinted;
			if ( !gameErrorPrinted ) {
				gameErrorPrinted = true;
				printf( "Packet received with gameID 0x%x instead of 0x%x\n", ps->gameID, gameID );
			}
			return;
		}
		
		if ( ps->packetSequence <= lastServerPacket.packetSequence ) {
			printf( "Out of order or duplicated packet from server: %i <= %i\n", 
				   ps->packetSequence , lastServerPacket.packetSequence );
			return;
		}
		int drop = ps->packetSequence - (lastServerPacket.packetSequence + 1);
		if ( drop > 0 ) {
			printf( "Dropped %i packets from server\n", drop );
		}
		
		// good packet from server
		memcpy( &lastServerPacket, ps, len );
		UpdatePeerTiming( &netServer, ps->milliseconds );
		netServer.currentPingTics = packetSequence - ps->packetAcknowledge;
		
		// It is possible to have a client run a tic that hasn't been run yet on the game
		// server, since the server can be generating cmds and sending packets while
		// its game frame is hitched for an image load, so this is not an error condition.
		// assert( ps->gametic >= gametic );
		
		// this should never happen
		assert( ps->maketic >= maketic );
		
		// if a ticcmd_t that we need has permanently rolled off the end, we are hosed.
		// This shouldn't happen, since we don't create commands if all the clients
		// haven't processed most of the ones already sent.
		if ( ps->maketic - gametic >= BACKUPTICS ) {
			printf( "BACKUPTICS exceeded: ps->maketic %i, gametic %i\n",
				   ps->maketic, gametic );
			netGameFailure = NF_INTERRUPTED;
		}
		
		// move over the new commands
		// it is possible that some early frames of these are redundant, due
		// to packets crossing in flight.
		ticcmd_t *cmd_p = ps->netcmds;
		for ( int i = ps->starttic ; i < ps->maketic ; i++ ) {
			for ( int j = 0 ; j < MAXPLAYERS ; j++ ) {
				if ( playeringame[j] ) {
					netcmds[j][i&BACKUPTICMASK] = *cmd_p++;
				}
			}
		}
		
		// copy this after the cmds have been updated
		maketic = ps->maketic;
		
		// check consistancy for all in-game players on the most
		// recent gametic that the server knew this client had run
		int	checkTic = ps->consistancyTic;
		assert( checkTic > gametic - BACKUPTICS );	// if older than this, we have lost the data
		checkTic &= BACKUPTICMASK;
		for ( int i = 0 ; i < MAXPLAYERS ; i++ ) {
			if ( playeringame[i] ) {
				if ( ps->consistancy[i] != consistancy[i][checkTic] ) {
					printf( "ConsistancyFailure for player %i on consistancyTic %i\n",
						   i, ps->consistancyTic );
					netGameFailure = NF_CONSISTANCY;
				}
			}
		}
	}	
}

/*
 ==================
 SendSetupPacketIfNecessary
 
 the server sends out a setup packet to each joined client so they
 can see the game options needed to start the game.
 ==================
 */
void SendSetupPacketIfNecessary() {
	if ( setupPacket.gameID != localGameID ) {
		// we aren't the server
		return;
	}
	
	if ( gametic >= 2 ) {
		// everyone has already started, so they don't need more setup packets
		return;
	}

	
	setupPacket.sendCount++;
	
	// player 0 is always the server, no need to send to ourselves
	for ( int i = 1 ; i < MAXPLAYERS ; i++ ) {
		if ( setupPacket.playerID[i] == 0 ) {
			continue;
		}
		int r = sendto( gameSocket, &setupPacket, sizeof( setupPacket ), 0, 
					   &netPlayers[i].peer.address, sizeof( netPlayers[i].peer.address ) );
		if ( r == -1 ) {
			Com_Printf( "UDP sendTo failed: %s\n", strerror( errno ) );
			close( gameSocket );
			gameSocket = -1;
		}
	}
}


/*
 ==================
 DeadBandAdjust
 
 Compresses the 0.0 - 1.0 range into deadband - 1.0
 ==================
 */
float DeadBandAdjust( float f, float deadBand ) {
	if ( f < 0 ) {
		return -DeadBandAdjust( -f, deadBand );
	}
	if ( f > 1.0 ) {
		return 1.0;
	}
	if ( f < deadBand ) {
		return 0;
	}
	return (f-deadBand) / (1.0 - deadBand);
}

/*
 ==================
 AxisHit
 
 Returns a -1 to 1 range
 
 If activeFraction is less than 1.0, the range will clamp
 to the limits before the edge of the box is hit.
 ==================
 */
float	AxisHit( ibutton_t *hud ) {
	// will be set true if -1 or 1
	hud->drawAsLimit = false;	
	
	if ( hud->buttonFlags & BF_IGNORE ) {
		return 0;
	}
	
	touch_t *t = hud->touch;
	if ( !t ) {
		return 0;
	}
	
	int	centerX, centerY;

	if ( centerSticks->value ) {
		// center on each touch
		centerX = hud->downX;
		centerY = hud->downY;
	} else {
		centerX = hud->x + hud->drawWidth / 2;
		centerY = hud->y + hud->drawHeight / 2;
	}
	
	float	w = hud->drawWidth * 0.5 * hud->scale;
	float	h = hud->drawHeight * 0.5 * hud->scale;
	int x = t->x - centerX;
	int y = t->y - centerY;
	float	f;
	int isXaxis = ( hud != &huds.forwardStick );
	if ( isXaxis ) {
		f = (float)x / w;
	} else {
		f = (float)y / h;
	}
	float	deadBand = stickDeadBand->value;
	if ( hud == &huds.turnStick ) {
		deadBand = 0;
	}
	if ( f > deadBand ) {
		f -= deadBand;
	} else if ( f < -deadBand ) {
		f += deadBand;
	} else {
		// inside the deadband
		return 0;
	}
	
	// adjust so you can hit the limit even if the control is drawn at the very edge
	// of the screen
	f /= (0.95-deadBand);
	if ( f > 1.0f ) {
		f = 1.0f;
		hud->drawAsLimit = true;	
	} else if ( f < -1.0f ) {
		f = -1.0f;
		hud->drawAsLimit = true;	
	}
	
	if ( hud == &huds.turnStick && rampTurn->value ) {
		// do "gamma corrected" movement, so changes are always proportional
		if ( f > 0 ) {
			f = 0.01 * pow( 1.047, f * 100 );
		} else {
			f = -0.01 * pow( 1.047, f * -100 );
		}
	}
	return f;
}


static const float NOT_TOUCHED_STATE = 99999.0f;

float RotorControl( ibutton_t *hud ) {
	if ( hud->buttonFlags & BF_IGNORE ) {
		return 0;
	}
	touch_t *t = hud->touch;
	if ( !t ) {
		// no touches in the control
		hud->touchState = NOT_TOUCHED_STATE;
		return 0;
	}
	float	delta[2];
	
	int	centerX = hud->x + hud->drawWidth / 2;
	int	centerY = hud->y + hud->drawHeight / 2;
		
	delta[0] = t->x - centerX;
	delta[1] = t->y - centerY;
	
	float	rotorAngle = atan2( delta[1], delta[0] );
	if ( hud->touchState == NOT_TOUCHED_STATE ) {
		// just touched, haven't moved yet
		hud->touchState = rotorAngle;
		return 0;
	}
	float	deltaAngle = rotorAngle - hud->touchState;
	// handle the wrap around cases
	if ( deltaAngle >= M_PI ) {
		deltaAngle = deltaAngle - 2*M_PI;
	} else if ( deltaAngle <= -M_PI ) {
		deltaAngle = 2*M_PI + deltaAngle;
	}
	hud->touchState = rotorAngle;
	hud->drawState += deltaAngle;
	return deltaAngle / (2*M_PI);	
}

#define TURBOTHRESHOLD  0x32
static int ClampMove( int v ) {
	if ( v > TURBOTHRESHOLD ) {
		return TURBOTHRESHOLD;
	}
	if ( v < -TURBOTHRESHOLD ) {
		return -TURBOTHRESHOLD;
	}
	return v;
}


/*
 ==================
 iphoneBuildTiccmd
 
 Use touch and tilt controls to set up a doom ticcmd_t
 ==================
 */
static void iphoneBuildTiccmd(ticcmd_t* cmd) {
	memset(cmd,0,sizeof*cmd);	
	//	cmd->consistancy = consistancy[consoleplayer][maketic & BACKUPTICMASK];
	
	if ( menuState != IPM_GAME ) {
		// if in the menus, always generate an empty event
		return;
	}
	
	// the respawn button triggers a use
	if ( respawnActive ) {
		cmd->buttons |= BT_USE;
		respawnActive = false;
	}
	
	if ( gamestate != GS_LEVEL ) {
		// at intermissions, all taps equal attack
		// FIXME: better latched value
		if ( numTouches == numPrevTouches + 1 ) {
			cmd->buttons |= BT_ATTACK;
		}
		return;
	}
	
	// don't allow movement control use during automap
	if ( automapmode & am_active ) {
		return;
	}
	
	// don't built a tic when dead, other than the respawn use
	if ( players[consoleplayer].playerstate == PST_DEAD ) {
		return;
	}
	
	//------------------------
	// No controls during weapon-select screen
	//------------------------
	boolean	weaponCycle = false;
	if ( drawWeaponSelect ) {
		// if the weaponSelect overlay is up, continue tracking held touches
		// until the are released
		for ( ibutton_t *hud = (ibutton_t *)&huds ; hud != (ibutton_t *)(&huds+1) ; hud++ ) {
			if ( hud->touch || hud == &huds.weaponSelect ) {
				UpdateHudTouch( hud );
			}
		}
    
		return;
	}
	
	//------------------------
	// gameplay controls
	//------------------------
	
	// update all the hud touch states
	if ( menuState == IPM_GAME ) {
		UpdateHudTouch( &huds.forwardStick );
		UpdateHudTouch( &huds.sideStick );
		UpdateHudTouch( &huds.turnStick );
		UpdateHudTouch( &huds.turnRotor );
		UpdateHudTouch( &huds.weaponSelect );
	}
	// tap in the lower center for weapon switch
	touch_t *t = huds.weaponSelect.touch;
	if ( t && t->down && t->stateCount == 1 ) {
		drawWeaponSelect = true;
	}		
	
	// hack to let a single touch control both hud elements on combo sticks
	// This is dependent on the order in the structure, and probably not a good
	// way to do things.
	if ( huds.sideStick.x == huds.forwardStick.x &&  huds.sideStick.y == huds.forwardStick.y ) {
		huds.sideStick.touch = huds.forwardStick.touch;
		huds.sideStick.downX = huds.forwardStick.downX;
		huds.sideStick.downY = huds.forwardStick.downY;
	}
	if ( huds.turnStick.x == huds.forwardStick.x &&  huds.turnStick.y == huds.forwardStick.y ) {
		huds.turnStick.touch = huds.forwardStick.touch;
		huds.turnStick.downX = huds.forwardStick.downX;
		huds.turnStick.downY = huds.forwardStick.downY;
	}
	
	// the fire button doesn't grab touches
	{
		int x = huds.fire.x - ( huds.fire.drawWidth >> 1 );
		int y = huds.fire.y - ( huds.fire.drawHeight >> 1 );
		int w = huds.fire.drawWidth << 1;
		int h = huds.fire.drawHeight << 1;
		if ( AnyTouchInBounds( x, y, w, h ) ) {
			cmd->buttons |= BT_ATTACK;
			huds.fire.buttonFlags |= BF_DRAW_ACTIVE;	// draw with color
		} else {
			huds.fire.buttonFlags &= ~BF_DRAW_ACTIVE;
		}
	}
	int	forwardmove;
	int	sidemove;
	
	// the edge of the drawn control should give the maximum
	// legal doom movement speed
	huds.forwardStick.scale = stickMove->value / 128.0f;
	huds.sideStick.scale = stickMove->value / 128.0f;
	
	forwardmove = -TURBOTHRESHOLD * AxisHit( &huds.forwardStick );
	sidemove = TURBOTHRESHOLD * AxisHit( &huds.sideStick );
	
	huds.turnStick.scale = stickTurn->value / 128.0f;
	cmd->angleturn = -1500.0f * AxisHit( &huds.turnStick );
	
	// rotary wheel
	cmd->angleturn -= rotorTurn->value * RotorControl( &huds.turnRotor );
	
	// accelerometer tilting
	sidemove += tiltMove->value * DeadBandAdjust( tilt, tiltDeadBand->value );
	cmd->angleturn -= tiltTurn->value * DeadBandAdjust( tilt, tiltDeadBand->value );
	
	// clamp movements
	cmd->forwardmove = ClampMove( forwardmove );
	cmd->sidemove = ClampMove( sidemove );
	
	// tap in the upper center for use
	if ( TouchPressed( 140, 0, 240, 200 ) ) {
		cmd->buttons |= BT_USE;
	}
	
	// auto-use if the game thread found a usable line in front of the player
	if ( autoUse->value && autoUseActive ) {
		if ( cmd->buttons & BT_USE ) {
			// Allow a tap to briefly cancel the auto-use, which works around
			// some issues with incorrectly started auto-uses preventing
			// a real door from opening.
			cmd->buttons &= ~BT_USE;			
		} else {
			cmd->buttons |= BT_USE;
		}		
	}
	
	if ( weaponSelected != -1 ) {
		cmd->buttons |= BT_CHANGE;
		cmd->buttons |= weaponSelected<<BT_WEAPONSHIFT;
		weaponSelected = -1;
	} else {		
		// auto-cycle weapons when firing on empty
		if ( players[consoleplayer].attackdown && !P_CheckAmmo(&players[consoleplayer]) ) {
			weaponCycle = true;
		}
		
		// weapon switch
		int newweapon = wp_nochange;
		if ( weaponCycle ) {
			// witch to next weapon when out of ammo
			newweapon = P_SwitchWeapon(&players[consoleplayer]);
		}
		
		if (newweapon != wp_nochange) {
			cmd->buttons |= BT_CHANGE;
			cmd->buttons |= newweapon<<BT_WEAPONSHIFT;
		}
	}
}

/*
 ==================
 ShouldSendPacket
 
 WiFi can suffer from bad "pileup" because of the link-level
 retransmit behavior.  What we would dearly love is an ioctl or
 something that returned a value of how many packets are backed
 up in the transmit queue, but that would be terribly driver
 specific, and we are unlikely to ever see it.
 
 We are going to infer that there is a pileup by looking at the
 round trip latency.  If we ever support internet connections with
 real latencies that are this large, we will have to use something
 else.
 ==================
 */
// while 1 is usual, some combinations of AsyncTic phases will give 2 commonly,
// and we don't want to throttle back in those cases.
static const int okPacketLatency = 4;
static const int okOneWayLatency = 70;
static boolean ShouldSendPacket( netPeer_t *peer, int packetLatency ) {
	if ( throttle->value == 0 ) {
		// disabled completely, always send
		return true;
	}
	
	if ( peer == &netServer ) {
		if ( throttle->value == 2 ) {
			return false;				// don't send client messages at all
		}
	} else {
		if ( throttle->value == 3 ) {	// don't send server messages at all
			return false;
		}
	}
	
	// immediately fire back a packet if it looks like we just got one through
	// clearly
	if ( peer->lastPacketAsyncTic == asyncTicNum && peer->oneWayLatency < okOneWayLatency ) {
		return true;
	}
	
	if ( packetLatency <= okPacketLatency ) {
		return true;
	}
	// limit from 1 to 4, in worst case only transmit a packet
	// every 16 frames, or about half a second
	packetLatency -= okPacketLatency;
	if ( packetLatency > 4 ) {
		packetLatency = 4;
	}

	if ( asyncTicNum & ((1<<packetLatency)-1) ) {
		return false;	// inhibit transmit this frame
	}

	printf( "Sending on packetLatency %i\n", packetLatency );
		   
	return true;		// we are throttling back, but transmit this frame 
}

						 
/*
 ==================
 iphoneAsyncTic
 
 This is called by a 30hz scheduled timeer in the main application thread.
 Commands are generated and sent to the packet server
 on this regular basis, regardless of the frame rate held by iphoneFrame().
 This thread should be higher priority, so it always interrupts the game
 thread.
 
 It might be nice to run the game tics here, but the rendering code is not
 thread safe with the game, so it isn't an option.
 ==================
 */
void iphoneAsyncTic() {
	// log our timing accuracy (seems to be within a few msec -- not bad)
	static int prev;
	int	now = SysIphoneMilliseconds();
	asyncStats_t	*stats = &asyncStats[asyncTicNum&(MAX_ASYNC_LOGS-1)];
	asyncTicNum++;

	memset( stats, 0, sizeof( *stats ) );
	stats->msecFromLast = now - prev;
	stats->msecToExecute = 0;

	prev = now;

	// listen for changes to available servers
	//ProcessDNSMessages();
	
	// send out the setup packets if we are just starting the game
	SendSetupPacketIfNecessary();
	
	// don't generate any commands while loading levels
	if ( iphoneFrameNum == levelLoadFrameNum  ) {
		return;
	}

	// don't let the game thread mess with touches during the async tic execution
	pthread_mutex_lock( &eventMutex );

	int	afterLock = SysIphoneMilliseconds();
	
	// latch the current touches for processing
	for ( int i = 0 ; i < MAX_TOUCHES ; i++ ) {
		touch_t *t = &sysTouches[i];
		
		gameTouches[i] = *t;
		
		// handle the special case of a touch that went down and up
		// inside a single frame
		if ( t->stateCount == -1 ) {
			gameTouches[i].stateCount = 1;
			t->stateCount = 0;
			t->down = false;
		} else {
			t->stateCount++;
		}
	}
	
	//---------------------------------
	// read network packets
	//
	// we may receive multiple packets in one frame due to timer skew
	// even with only one other player
	//---------------------------------
	if ( !netGameFailure ) {
		byte	packet[1500];
		
		while( gameSocket > 0 ) {
			struct sockaddr	from;
			unsigned senderLen = sizeof( from );
			int r = recvfrom( gameSocket, &packet, sizeof( packet ), 0, &from, &senderLen );
			if ( r == -1 ) {
				if ( errno != EAGAIN ) {
					perror( "recvfrom" );
				}
				break;
			}
			stats->received++;
			iphoneProcessPacket( &from, packet, r );
		}
	}

	//---------------------------------
	// Create local user command
	// 
	// We always create one, but it might not wind up being used for a game
	// tic if it doesn't make it to the server at the right time.
	//---------------------------------
	ticcmd_t cmd;
	iphoneBuildTiccmd( &cmd );
	
	//---------------------------------
	// If we are a client, send our command to the server
	//---------------------------------
	if ( consoleplayer != 0 ) {
		if ( gameID != 0 && netgame && !netGameFailure ) {
			stats->latency = packetSequence - lastServerPacket.packetAcknowledge;
			if ( ShouldSendPacket( &netServer, packetSequence - lastServerPacket.packetAcknowledge ) ) {
				packetClient_t	cp;
				memset( &cp, 0, sizeof( cp ) );
				cp.packetType = PACKET_VERSION_CLIENT;
				cp.gameID = gameID;
				cp.packetAcknowledge = lastServerPacket.packetSequence;
				cp.milliseconds = SysIphoneMilliseconds();
				cp.packetSequence = packetSequence++;
				cp.consoleplayer = consoleplayer;
				cp.gametic = gametic;
				cp.cmd = cmd;
	 			int r = sendto( gameSocket, &cp, sizeof( cp ), 0, &netServer.address, sizeof( netServer.address ) );
				stats->sent++;
				if ( r == -1 ) {
					printf( "UDP sendTo failed: %s\n", strerror( errno ) );
					close( gameSocket );
					gameSocket = -1;
				}
			}
		}
	} else {
		
		// take our command directly
		netPlayers[0].pc.cmd = cmd;
		netPlayers[0].pc.gametic = gametic;
		netPlayers[0].peer.lastPacketTime = now;
		
		
		//---------------------------------
		// Decide if we want to latch the current commands for execution by the game
		//
		//---------------------------------
		int	ticIndex = maketic & BACKUPTICMASK;
		
		int	worstTic = gametic;
		for ( int i = 0 ; i < MAXPLAYERS ; i++ ) {
			if ( playeringame[i] ) {
				netcmds[i][ticIndex] = netPlayers[i].pc.cmd;
				if ( netPlayers[i].pc.gametic < worstTic ) {
					worstTic = netPlayers[i].pc.gametic;
				}
			}
		}
		
		// only let the server get a few tics ahead of any client, so if
		// anyone is having significant net delivery problems, everyone will
		// stall instead of losing the player.  If this is too small, then
		// every little hitch that any player gets will cause everyone to hitch.
		if ( maketic - worstTic < netBuffer->value ) {
			maketic++;
		}
		
		
		//---------------------------------
		// Build server packets to send to clients
		//
		// Always send out the current command set over the network
		// even if we didn't create a new command, in case we are just
		// recovering from a lot of dropped packets.
		//---------------------------------
		if ( netgame && !netGameFailure ) {
			// since we are sampling a shared wireless network, any of the player's
			// latencies should be a good enough metric
			stats->latency = packetSequence - netPlayers[1].pc.packetAcknowledge;

			if ( ShouldSendPacket( &netPlayers[1].peer, stats->latency ) ) {
				packetServer_t	gp;
				memset( &gp, 0, sizeof( gp ) );
				gp.packetType = PACKET_VERSION_SERVER;
				gp.gameID = gameID;
				gp.packetSequence = packetSequence++;
				gp.maketic = maketic;
				memcpy( gp.netcmds, netcmds, sizeof( gp.netcmds ) );
				
				//---------------------------------
				// Send network packets to the clients
				//---------------------------------
				for ( int i = 1 ; i < MAXPLAYERS ; i++ ) {
					if ( !playeringame[i] ) {
						continue;
					}
					if ( gameSocket <= 0 ) {
						continue;
					}
					netPlayer_t *np = &netPlayers[i];
					
					// only send over the ticcmd that this client needs
					gp.starttic = np->pc.gametic;				
					ticcmd_t *cmd_p = gp.netcmds;
					for ( int i = gp.starttic ; i < gp.maketic ; i++ ) {
						for ( int j = 0 ; j < MAXPLAYERS ; j++ ) {
							if ( playeringame[j] ) {
								*cmd_p++ = netcmds[j][i&BACKUPTICMASK];
							}
						}
					}
					int	packetSize = (byte *)cmd_p - (byte *)&gp;
					
					// use the most recent tic that both the client and
					// server have run 
					gp.consistancyTic = np->pc.gametic < gametic ? np->pc.gametic : gametic;
					gp.consistancyTic--;
					
					for ( int j = 0 ; j < MAXPLAYERS ; j++ ) {
						gp.consistancy[j] = consistancy[j][gp.consistancyTic&BACKUPTICMASK];
					}
					
					gp.packetAcknowledge = np->pc.packetSequence;
					gp.milliseconds = SysIphoneMilliseconds();
					
					// transmit the packet
					stats->sent++;
					int r = sendto( gameSocket, &gp, packetSize, 0, &np->peer.address, sizeof( np->peer.address ) );
					if ( r == -1 ) {
						printf( "UDP sendTo failed: %s\n", strerror( errno ) );
						close( gameSocket );
						gameSocket = -1;
					}
				}
			}
		}
	}
	
	stats->msecToExecute = SysIphoneMilliseconds() - now;
	if ( stats->msecToExecute > 6 ) {
		printf( "long asyncTic %i: %i msec (%i in lock), %i packets\n", asyncTicNum - 1, stats->msecToExecute, 
			   afterLock - now, stats->received );
	}
	
	// the game thread can now swap touches
	pthread_mutex_unlock( &eventMutex );
	
	// signal the main thread that is probably blocked on this semaphore
	if ( sem_post( ticSemaphore ) == -1 ) {
		perror( "sem_post");
	}

}


