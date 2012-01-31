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
#include <math.h>

hud_t	huds;

void HudDraw();

void HudWrite();

void HudRead();

ibutton_t	*dragHud;
int			dragX, dragY;

void SetHudPic( ibutton_t *hp, const char *image ) {
	pkTexture_t *gl;
	gl = PK_FindTexture( image );
	assert( gl );
	hp->texture = gl;
	hp->touch = NULL;	// in case one was down when it was saved
}

void SetHudSpot( ibutton_t *hp, int x, int y, int dw, int dh ) {
	hp->touch = NULL;	// in case one was down when it was saved	
    
    float xRatio = ((float)displaywidth) / 480.0f;
    float yRatio = ((float)displayheight) / 320.0f;
    
    float themin = MIN( xRatio, yRatio );
    
    x *= ((float)displaywidth) / 480.0f;
    y *= ((float)displayheight) / 320.0f;
    
    dw *= themin;
    dh *= themin;
    
	hp->x = x - dw/2;
	hp->y = y - dh/2;
	hp->drawWidth = dw;
	hp->drawHeight = dh;
	hp->buttonFlags = 0;
	hp->scale = 1.0f;
}

void HudSetTexnums() {
	SetHudPic( &huds.forwardStick, "iphone/up_down.tga" );
	SetHudPic( &huds.sideStick, "iphone/side_2_side.tga" );
	SetHudPic( &huds.turnStick, "iphone/directional_2.tga" );
	SetHudPic( &huds.turnRotor, "iphone/rotate.tga" );
	SetHudPic( &huds.fire, "iphone/fire.tga" );
	SetHudPic( &huds.menu, "iphone/menu_button.tga" );
	SetHudPic( &huds.map, "iphone/map_button.tga" );
	
	SetHudSpot( &huds.weaponSelect, 240, 280, 40, 90 );	
}

void HudSetForScheme( int schemeNum ) {
	for ( ibutton_t *hud = (ibutton_t *)&huds ; hud != (ibutton_t *)(&huds+1) ; hud++ ) {
		hud->buttonFlags = BF_IGNORE;
	}
	int STICK_SIZE = 128;
	int HALF_STICK = 128/2;
    
    if( displaywidth >= 1024 ) {
        STICK_SIZE = 64;
        HALF_STICK = 64/2;
    }
    
	static const int BOTTOM = 320 - 44;	// above the status bar
	SetHudSpot( &huds.weaponSelect, 240, 280, 40, 90 );	// the touch area is doubled
	
	// make the forward / back sticks touch taller than they draw
	switch ( schemeNum ) {
		default:
		case 0:		// turn stick
			SetHudSpot( &huds.forwardStick, HALF_STICK, BOTTOM-HALF_STICK, STICK_SIZE, STICK_SIZE );
			SetHudSpot( &huds.turnStick, HALF_STICK, BOTTOM-HALF_STICK, STICK_SIZE, STICK_SIZE );
			SetHudSpot( &huds.fire, 480-40, BOTTOM-HALF_STICK, 80, 80 );
			SetHudSpot( &huds.menu, 480-24, 24, 48, 48 );
			SetHudSpot( &huds.map, 24, 24, 48, 48 );
			break;
		case 1:		// dual stick
			SetHudSpot( &huds.forwardStick, HALF_STICK, BOTTOM-HALF_STICK, STICK_SIZE, STICK_SIZE );
			SetHudSpot( &huds.sideStick, HALF_STICK, BOTTOM-HALF_STICK, STICK_SIZE, STICK_SIZE );
			SetHudSpot( &huds.turnStick, 480-HALF_STICK, BOTTOM-HALF_STICK, STICK_SIZE, STICK_SIZE );
			SetHudSpot( &huds.fire, 480-40, 40, 80, 80 );	
			SetHudSpot( &huds.menu, 48+24, 24, 48, 48 );
			SetHudSpot( &huds.map, 24, 24, 48, 48 );
			break;
		case 2:		// rotor
			SetHudSpot( &huds.forwardStick, HALF_STICK, BOTTOM-HALF_STICK, STICK_SIZE, STICK_SIZE );
			SetHudSpot( &huds.sideStick, HALF_STICK, BOTTOM-HALF_STICK, STICK_SIZE, STICK_SIZE );
			SetHudSpot( &huds.turnRotor, 480-HALF_STICK, BOTTOM-HALF_STICK, STICK_SIZE, STICK_SIZE );
			SetHudSpot( &huds.fire, 480-40, 40, 80, 80 );	
			SetHudSpot( &huds.menu, 48+24, 24, 48, 48 );
			SetHudSpot( &huds.map, 24, 24, 48, 48 );
			break;
	}
	
	// don't process these in the update hud touch loop, because they will be
	// handled with normal button calls
	huds.menu.buttonFlags |= BF_HUDBUTTON;
	huds.map.buttonFlags |= BF_HUDBUTTON;
	
	// don't make the big button click sound for the fire button
	huds.fire.buttonFlags |= BF_SMALL_CLICK;
	
}

void SnapSticks( ibutton_t *test, const ibutton_t *to ) {
	if ( abs( test->x - to->x ) < test->drawWidth && abs( test->y - to->y ) < test->drawHeight ) {
		test->x = to->x;
		test->y = to->y;
	}
}

/*
 ==================
 HudEditFrame
 
 ==================
 */
void HudEditFrame() {
	color3_t gray = { 32, 32, 32 };
		
	if ( numTouches == 0 && numPrevTouches == 1 && dragHud ) {
		Sound_StartLocalSound( "iphone/baction_01.wav" );
		dragHud = NULL;
	}
	
	if ( numTouches == 1 && numPrevTouches == 0 ) {
		// identify the hud being touched for drag
		int x = touches[0][0];
		int y = touches[0][1];
		dragHud = NULL;
		for ( ibutton_t *hud = (ibutton_t *)&huds ; hud != (ibutton_t *)(&huds+1) ; hud++ ) {
			if ( hud->buttonFlags & BF_IGNORE ) {
				continue;
			}
			if ( x >= hud->x && x - hud->x < hud->drawWidth && y >= hud->y && y - hud->y < hud->drawHeight ) {
				dragHud = hud;
				dragX = dragHud->x - x;
				dragY = dragHud->y - y;
				Sound_StartLocalSound( "iphone/bdown_01.wav" );
				break;
			}
		}
	}
	
	if ( numTouches == 1 && numPrevTouches == 1 && dragHud ) {
		// adjust the position of the dragHud
		dragHud->x = touches[0][0] + dragX;
		dragHud->y = touches[0][1] + dragY;
		if ( dragHud->x < 0 ) {
			dragHud->x = 0;
		}
		if ( dragHud->x > displaywidth - dragHud->drawWidth ) {
			dragHud->x = displaywidth - dragHud->drawWidth;
		}
		if ( dragHud->y < 0 ) {
			dragHud->y = 0;
		}
		if ( dragHud->y > displayheight - dragHud->drawHeight ) {
			dragHud->y = displayheight - dragHud->drawHeight;
		}
		
		// magnet pull a matchable axis
		if ( controlScheme->value == 0 ) {
			if ( dragHud == &huds.forwardStick ) {
				SnapSticks( &huds.turnStick, dragHud );				
			} 
		} else {
			if ( dragHud == &huds.forwardStick ) {
				SnapSticks( &huds.sideStick, dragHud );
			}
		}
	}
	
	// solid background color and some UI elements for context
	R_Draw_Fill( 0, 0, 480, 320, gray );	
	glColor4f( 1, 1, 1, 1 );
	iphoneCenterText( 240, 20, 0.75, "Drag the controls" );

	// draw the status bar
	extern patchnum_t stbarbg;
	if ( statusBar->value ) {
		// force doom to rebind, since we have changed the active GL_TEXTURE_2D
		last_gltexture = NULL;
		gld_DrawNumPatch(0, ST_Y, stbarbg.lumpnum, CR_DEFAULT, VPT_STRETCH);
	}
	
	// draw the active items at their current locations
	for ( ibutton_t *hud = (ibutton_t *)&huds ; hud != (ibutton_t *)(&huds+1) ; hud++ ) {
		if ( !hud->texture ) {
			continue;
		}
		if ( hud->buttonFlags & BF_IGNORE ) {
			continue;
		}
		PK_StretchTexture( hud->texture, hud->x, hud->y, hud->drawWidth, hud->drawHeight );
	}
	
	// draw the done button
	static ibutton_t btnDone;	
	if ( !btnDone.texture ) {
		// initial setup
		SetButtonPicsAndSizes( &btnDone, "iphone/back_button.tga", "Done", 240 - 32, 160-32, 64, 64 );
	}
	if ( HandleButton( &btnDone ) ) {
		menuState = IPM_MAIN;
	}
	
}

