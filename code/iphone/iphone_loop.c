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
#include "TargetConditionals.h"

playState_t playState;

#define OWNER_AUTOMAP	(void *)0x123

int		iphoneFrameNum;
int		levelLoadFrameNum;
boolean	iphoneTimeDemo;
char	timeDemoResultString[80];
int		timeDemoFrames;
int		timeDemoStart;

// set to 1 when app is exiting to cause game thread to do a save game,
// which would not be safe to do from the event thread
volatile int		saveOnExitState;

// console mode
int consoleActive;

// current touches latched from the system touches
int	numTouches;
int	touches[5][2];	// [0] = x, [1] = y in landscape mode, raster order with y = 0 at top

// so we can detect button releases
int	numPrevTouches;
int prevTouches[5][2];

float	tilt;		// -1.0 to 1.0
float	tiltPitch;

#define MAX_TILT_HISTORY 64
float	tiltHistory[MAX_TILT_HISTORY][4];
int		tiltHistoryNum;

// pressing on the weapon brings up the weaponSelect overlay
boolean	drawWeaponSelect;
int		weaponSelected = -1;

pkTexture_t *arialFontTexture;


logTime_t	loggedTimes[MAX_LOGGED_TIMES];	// indexed by iphoneFrameNum

int		gameSocket;
int		gameID;
int		playerID;
int		packetSequence;					// for logging dropped packets and estimating latency
netFail_t netGameFailure;				// set by asyncThread
netPlayer_t	netPlayers[MAXPLAYERS];
netPeer_t	netServer;

// set after each game tic if a usable line is in front of the player
boolean	autoUseActive;
boolean respawnActive;

// if we haven't processed a game tic in a half second, draw the net problem icon
int	lastGameProcessedTime;

// this flag lets us give a shotgun and some ammo after the player has respawned
boolean	addGear;

extern bool inBackgroundProcess;
/*
=================================================================================
 
 Touch Handling
 
 With multiple draggable controls on screen, it is important to track touches
 as continuous events, rather than discrete points, because we want to allow
 a finger to start on a dragable control, but still function when it has
 been dragged outside the original hot area, and not trigger anything else
 as it wanders around.
 
 I considered a Down / Dragged / Released interface, but especially with
 threading, it was better to keep them as a set of structures that could
 be looked at at different points in the game loop.
 
 if touch goes down on a dragable control, it will be owned by that control until it
 is released, even if it is dragged outside the original bounds.
 
 If we support pinch for anything, a single control will own multiple touches.
 
 Touches that aren't owned by a dragable control are free-roaming, and can hit buttons.
 
 =================================================================================
 */


/*
 
 Does not claim ownership or play any sounds.
 The touch can be dragged in if it isn't owned by
 another control.
 
 */
touch_t *TouchInBounds( int x, int y, int w, int h ) {
	for ( int i = 0 ; i < MAX_TOUCHES ; i++ ) {
		touch_t *t = &gameTouches[i];
		if ( t->controlOwner ) {
			continue;	// already claimed
		}
		if ( !t->down ) {
			continue;	// not pressed
		}
		if ( t->x >= x && t->x < x + w
			&& t->y >= y && t->y < y + h ) {
            // JDS Debug
            //printf("Touch HIT! %d vs. %d / %d vs. %d (%d %d)\n",x,t->x,y,t->y,w,h);
			return t;
		}
        // JDS Debug
        else {
            //printf("Touch missed %d vs. %d / %d vs. %d (%d %d)\n",x,t->x,y,t->y,w,h);
        }
	}
	return NULL;
}


// even a touch claimed by another control will count (fire button)
touch_t *AnyTouchInBounds( int x, int y, int w, int h ) {
	for ( int i = 0 ; i < MAX_TOUCHES ; i++ ) {
		touch_t *t = &gameTouches[i];
		if ( !t->down ) {
			continue;	// not pressed
		}
		if ( t->x >= x && t->x < x + w
			&& t->y >= y && t->y < y + h ) {
			return t;
		}
	}
	return NULL;
}


touch_t *UpdateHudTouch( ibutton_t *hud ) {
	if ( hud->buttonFlags & ( BF_IGNORE | BF_HUDBUTTON ) ) {
		// hud element isn't active
		return NULL;
	}
    
	if ( !hud->touch ) {
		// see if a free touch was just made in, or dragged into the bounds
		// make the active boxes twice as large as the drawing bounds
		int x = hud->x - ( hud->drawWidth >> 1 );
		int y = hud->y - ( hud->drawHeight >> 1 );
		int w = hud->drawWidth << 1;
		int h = hud->drawHeight << 1;
		
		hud->touch = TouchInBounds( x, y, w, h );
        //printf("TIB: %d %d %d %d -> %s\n",x,y,w,h,(hud->touch == NULL) ? "none" : "found" );
		if ( hud->touch ) {
			// claim this touch so it won't activate anything else
			hud->touch->controlOwner = hud;
			if ( touchClick->value ) {
				Sound_StartLocalSoundAtVolume( "iphone/controller_down_01_SILENCE.wav", touchClick->value ? 0.15f : 0.0f );
			}
			// save the initial touch position for auto-centering stcks
			hud->downX = hud->touch->x;
			hud->downY = hud->touch->y;
			
			// Clamp it so that you are guaranteed to have a full range of motion.
			// This means that a touch at the edge of the screen won't center it,
			// but will instead cause immediate movement.  This prevents the main
			// drawback of the centering controls -- if you pressed down too close
			// to the side, you wouldn't have full mobility that direction.
			int width = hud->drawWidth / 2;
			int height = hud->drawHeight / 2;
			if ( hud->downX < width ) {
				hud->downX = width;
			}
            if ( hud->downX + width > displaywidth ) {
                hud->downX = displaywidth - width;
			}
			if ( hud->downY < height ) {
				hud->downY = height;
			}
            if ( hud->downY > displayheight - height ) {
                hud->downY = displayheight - height;
			}
		}
	}
	
	if ( hud->touch ) {
		// see if the touch was released
		if ( !hud->touch->down ) {
			if ( touchClick->value ) {
				Sound_StartLocalSoundAtVolume( "iphone/controller_up_01_SILENCE.wav", touchClick->value ? 0.15f : 0.0f );
			}
			hud->touch = NULL;
		}
	}
	return hud->touch;
}


void SetButtonPics( ibutton_t *button, const char *picBase, const char *title, int x, int y ) {	
	button->texture = PK_FindTexture( picBase );	
	button->scale = 1.0f;
	button->title = title;
    
    button->x = x * ((float)displaywidth) / 480.0f;
    button->y = y * ((float)displayheight) / 320.0f;
    
    float xRatio = ((float)displaywidth) / 480.0f;
    float yRatio = ((float)displayheight) / 320.0f;
    
    float themin = MIN( xRatio, yRatio );
    
	button->drawWidth = button->texture->textureData->srcWidth * themin;
	button->drawHeight = button->texture->textureData->srcHeight * themin;
}

void SetButtonPicsAndSizes( ibutton_t *button, const char *picBase, const char *title, int x, int y, int w, int h ) {	
	SetButtonPics( button, picBase, title, x, y );
    
    float xRatio = ((float)displaywidth) / 480.0f;
    float yRatio = ((float)displayheight) / 320.0f;
    
    float themin = MIN( xRatio, yRatio );
    
	button->drawWidth = w  * themin;
	button->drawHeight = h * themin;
}

/*
 ==================
 HandleButton
 
 Plays enter / exit / action sounds, returns true if the
 touch was released inside the bounds.
 
 Touches can slide onto a button, they aren't required
 to tap initially inside it.
 
 Main menu buttons and the small in-game buttons are
 done with this.  Because these handle both drawing and
 decision making, there is a frame of latency involved
 versus splitting the decision making and drawing.
  
 Returns true if the button should perform its action.
 ==================
 */
float	buttonScaleStep = 0.01f;
float	buttonScaleMin = 0.95f;
boolean HandleButton( ibutton_t *button ) {
	if ( button->buttonFlags & BF_IGNORE ) {
		return false;
	}
	
    
    // Hack
    button->drawHeight = button->drawWidth;
    
	if ( ( button->buttonFlags & BF_TRANSPARENT ) && !button->touch ) {
		// draw half-transparent
		glColor4f( 1, 1, 1, 0.5 );
	} else if ( button->buttonFlags & BF_DIMMED ) {
		// draw half-bright
		glColor4f( 0.5, 0.5, 0.5, 1 );
	} else {
		glColor4f( 1, 1, 1, 1 );
	}
	
	if ( button->touch && !button->touch->down ) {
		button->touch->controlOwner = NULL;
		button->touch = NULL;
	}
	
	bool released = false;
	if ( !(button->buttonFlags & BF_INACTIVE) ) {
		if ( button->touch ) {
			// see if the touch was dragged outside the button bounds
			if ( button->touch->x < button->x || button->touch->x >= button->x + button->drawWidth
				|| button->touch->y < button->y || button->touch->y >= button->y + button->drawHeight ) {
				// dragged outside, don't trigger on release now
				if ( button->buttonFlags & BF_SMALL_CLICK ) {
					Sound_StartLocalSoundAtVolume( "iphone/controller_up_01_SILENCE.wav", touchClick->value ? 0.15f : 0.0f  );
				} else {
					Sound_StartLocalSound( "iphone/baborted_01.wav" );	
				}
				button->touch->controlOwner = NULL;
				button->touch = NULL;
				button->pressed = false;
			}
		}
		if ( !button->touch ) {
			if ( button->pressed ) {
				// released inside the button, so do the action
				if ( button->buttonFlags & BF_SMALL_CLICK ) {
					Sound_StartLocalSoundAtVolume( "iphone/controller_up_01_SILENCE.wav", touchClick->value ? 0.15f : 0.0f  );
				} else {
					Sound_StartLocalSound( "iphone/baction_01.wav" );	
				}
				button->pressed = false;
				released = true;
			}
		}
		
		// see if a new touch went down or moved into the button
		if ( !released && !button->touch ) {
			button->twoFingerPress = false;
			
			for ( int i = 0 ; i < MAX_TOUCHES ; i++ ) {
				touch_t *t = &gameTouches[i];
				if ( t->controlOwner ) {
					continue;	// already claimed
				}
				if ( !t->down ) {
					continue;
				}
				if ( t->x >= button->x && t->x < button->x + button->drawWidth 
					&& t->y >= button->y && t->y < button->y + button->drawHeight ) {
					if ( button->buttonFlags & BF_SMALL_CLICK ) {
						Sound_StartLocalSoundAtVolume( "iphone/controller_down_01_SILENCE.wav", touchClick->value ? 0.15f : 0.0f );
					} else {
						Sound_StartLocalSound( "iphone/bdown_01.wav" );	
					}
					button->touch = t;
					button->pressed = true;
					t->controlOwner = &button;
					break;
				}
			}
		}
	}
	
	// animate scale
	if ( button->frameNum != iphoneFrameNum - 1 ) {
		button->scale = 1.0f;	// just came back to a menu
	}
	button->frameNum = iphoneFrameNum;
	
	if ( button->touch && button->touch->down ) {
		// check for a two-finger touch for alternate modes
		for ( int i = 0 ; i < MAX_TOUCHES ; i++ ) {
			touch_t *t = &gameTouches[i];
			if ( t->controlOwner ) {
				continue;	// already claimed
			}
			if ( t == button->touch ) {
			 	continue;	// the primary touch
			}
			if ( !t->down ) {
				continue;
			}
			if ( t->x >= button->x && t->x < button->x + button->drawWidth 
				&& t->y >= button->y && t->y < button->y + button->drawHeight ) {
				button->twoFingerPress = true;
				break;
			}
		}
		
		// adjust the animated scale
		button->scale -= buttonScaleStep;
		if ( button->scale < buttonScaleMin ) {
			button->scale = buttonScaleMin;
		}
	} else {
		button->scale += buttonScaleStep;
		if ( button->scale > 1.0f ) {
			button->scale = 1.0f;
		}
	}
	
	if ( button->buttonFlags & BF_GLOW ) {
		// cycle through double-bright
		float f = 0.75 + 0.25 * sin( 3.14159 * 2 * ( iphoneFrameNum & 31 ) / 32.0 );
					
		glColor4f( f, f, f, 1 );

		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
		glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0 );
	}
    
	PK_StretchTexture( button->texture, button->x+button->drawWidth/2 - button->drawWidth/2 * button->scale, 
					  button->y + button->drawHeight/2 - button->drawHeight/2 * button->scale, 
					  button->drawWidth * button->scale, button->drawHeight * button->scale );
    
	if ( button->buttonFlags & BF_GLOW ) {
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glColor4f( 1, 1, 1, 1 );
	}
	
    if ( button->title ) {
        float    length = StringFontWidth( button->title ) * 0.75;
        float    x = button->x + button->drawWidth/2 - length*2;
        // don't push the text off the edge of the screen
        if ( x < 0 ) {
            x = 0;
        } else if ( x + length > displaywidth ) {
            x = displaywidth - length;
        }
        float y;
        float textScale = 0.75;
        if ( button->buttonFlags & BF_CENTERTEXT ) {
            glColor4f( 1, 1, 1, 1 );    // !@# remove when we get a button background that doesn't need dimming
            y = button->y + button->drawHeight / 2 + 8;
            textScale *= button->scale;        // animate text scale when centered
        } else {
            y = button->y + button->drawHeight + 16;
        }
        
        textScale *= screenResolutionScale * 2;
        
        iphoneDrawText( x, y, textScale, button->title );
    }

	
	glColor4f( 1, 1, 1, 1 );
	
	return released;	
}

//=========================================================================


typedef struct {
	unsigned short	x0, y0, x1, y1;
	float	xoff, yoff, xadvance;
} GlyphRect;

#include "arialGlyphRects.h"	// precalculated offsets in the font image

float	StringFontWidth( const char *str ) {
	float	len = 0;
	while ( *str ) {
		int i = *str;
		if ( i >= ' ' && i < 128 ) {
			len += glyphRects[i-32].xadvance;
		}
		str++;
	}
	return len;
}

/*
 ==================
 iphoneDrawText
 
 Returns the width in pixels
 ==================
 */
float iphoneDrawText( float x, float y, float scale, const char *str ) {

float    fx = x;
float    fy = y;

PK_BindTexture( arialFontTexture );
glBegin( GL_QUADS );

while ( *str ) {
    int i = *str;
    if ( i >= ' ' && i < 128 ) {
        GlyphRect *glyph = &glyphRects[i-32];
        
        // the glyphRects don't include the shadow outline
        float    x0 = ( glyph->x0 - 1 ) / 256.0;
        float    y0 = ( glyph->y0 - 1 ) / 256.0;
        float    x1 = ( glyph->x1 + 2 ) / 256.0;
        float    y1 = ( glyph->y1 + 2 ) / 256.0;
        
        float    width = ( x1 - x0 ) * 256 * scale;
        float    height = ( y1 - y0 ) * 256 * scale;
        
        float    xoff = ( glyph->xoff - 1 ) * scale;
        float    yoff = ( glyph->yoff - 1 ) * scale;
        
        // red would be awesome here but it's kinda hard to read
//        glColor4f( 1, 0, 0, 1 );
        
        glTexCoord2f( x0, y0 );
        glVertex2f( fx + xoff, fy + yoff );
        
        glTexCoord2f( x1, y0 );
        glVertex2f( fx + xoff + width, fy + yoff );
        
        glTexCoord2f( x1, y1 );
        glVertex2f( fx + xoff + width, fy + yoff + height );
        
        glTexCoord2f( x0, y1 );
        glVertex2f( fx + xoff, fy + yoff + height );
        
        // with our default texture, the difference is negligable
        fx += glyph->xadvance * scale;
        //            fx += ceil(glyph->xadvance);    // with the outline, ceil is probably the right thing
    }
    str++;
}

glEnd();

return fx - x;
}


/*
 ==================
 iphoneCenterText
 
 Returns the width in pixels
 ==================
 */
float iphoneCenterText( float x, float y, float scale, const char *str ) {
	float l = StringFontWidth( str );
    
    x *= ((float)displaywidth) / 480.0f;
    y *= ((float)displayheight) / 320.0f;
    
	x -= l * scale * 0.5 * screenResolutionScale * 2;
    
	return iphoneDrawText( x, y, scale * screenResolutionScale * 2, str );
}


/*
 ==================
 TouchDown
 
 Checks all touches against a square
 ==================
 */
int	TouchDown( int x, int y, int w, int h ) {
	int	i;
	for ( i = 0 ; i < numTouches ; i++ ) {
		if ( touches[i][0] >= x && touches[i][1] >= y
			&& touches[i][0] < x + w && touches[i][1] < y + h ) {
			return 1;
		}
	}
	return 0;
}


/*
 ==================
 TouchPressed
 
 Requires that the touch be inside the bounds, and that it didn't seem to be
 dragged in from out of the bounds in the previous frame.
 ==================
 */
int	TouchPressed( int x, int y, int w, int h ) {
    
    x *= ((float)displaywidth) / 480.0f;
    y *= ((float)displayheight) / 320.0f;
    w *= ((float)displaywidth) / 480.0f;
    h *= ((float)displayheight) / 320.0f;
    
	for ( int i = 0 ; i < MAX_TOUCHES ; i++ ) {
		touch_t *t = &gameTouches[i];
		if ( !t->down ) {
			continue;
		}
		if ( t->controlOwner ) {
			continue;
		}
		if ( t->stateCount != 1 ) {
			// not just pressed
			continue;
		}
		
		if ( t->x < x || t->x >= x + w
			|| t->y < y || t->y >= y + h ) {
			continue;
		}
		// just pressed this frame
		return 1;
	}
	return 0;
}


/*
 ==================
 TouchReleased
 
 Perform an action when released in the box.
 If not down this frame, but down the previous frame, it is released
 ==================
 */
int	TouchReleased( int x, int y, int w, int h ) {
#if 0	
	// only check when the touch count just went down by one
	if ( numTouches != numPrevTouches - 1 ) {
		return 0;
	}
	int	i;
	int	downPrev = 0;
	int downNow = 0;
	
	for ( i = 0 ; i < numPrevTouches ; i++ ) {
		if ( prevTouches[i][0] >= x && prevTouches[i][1] >= y
			&& prevTouches[i][0] < x + w && prevTouches[i][1] < y + h ) {
			downPrev = 1;
			break;
		}
	}
	
	// see if not down this frame
	for ( i = 0 ; i < numTouches ; i++ ) {
		if ( touches[i][0] >= x && touches[i][1] >= y
			&& touches[i][0] < x + w && touches[i][1] < y + h ) {
			downNow = 1;
			break;
		}
	}
	
	if ( !downPrev ) {
		if ( downNow ) {
			Sound_StartLocalSound( "iphone/bdown_01.wav" );
		}
		// wasn't down the previous frame
		return 0;
	}
	
	if ( downNow ) {
		// still down
		return 0;
	}
	
	if ( numTouches == numPrevTouches ) {
		// finger dragged off
		Sound_StartLocalSound( "iphone/baborted_01.wav" );
		return 0;
	}
	
	// released
	Sound_StartLocalSound( "iphone/baction_01.wav" );
	return 1;
#else
	for ( int i = 0 ; i < MAX_TOUCHES ; i++ ) {
		touch_t *t = &gameTouches[i];
		if ( t->down ) {
			continue;	// still pressed
		}
		if ( t->x >= x && t->x < x + w
			&& t->y >= y && t->y < y + h ) {
			if ( t->stateCount <= 1 ) {
				// just released
				Sound_StartLocalSound( "iphone/baction_01.wav" );
				return 1;
			}
		}
	}
	return 0;
#endif
}

/*
 ==================
 iphoneSet2D
 
 ==================
 */
void iphoneSet2D( void ) {
	// note that GL thinks the iphone is always
	// in portrait mode as far as the framebuffer
	// is concerned.
    /* JDS proper fix for landscape orientation */
	glViewport( 0,0, displaywidth, displayheight );
	glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_SCISSOR_TEST );
	glDisable( GL_FOG );
	glDisable( GL_CULL_FACE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_ALPHA_TEST );
	glColor4f( 1,1,1,1 );
	glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	glOrthof( 0, displaywidth, displayheight, 0, -99999, 99999 );
}

void iphoneTiltEvent( float *tilts ) {
	int		i;
	int		j;
	int		c;
	float	sum[3];
	static float prevTime;

	// we probably should mutex this, but I doubt it causes any problems
	/*
	if ( revLand->value ) {
		tilts[1] = -tilts[1];
		tilts[0] = -tilts[0];
	}
	*/

	c = tiltAverages->value;
	if ( c < 1 ) {
		c = 1;
	} else if ( c > MAX_TILT_HISTORY ) {
		c = MAX_TILT_HISTORY;
	}
	
	// acc[0] - [2] are accelerometer values, ax[3] is the timestamp
	for ( i = 0 ; i < 3 ; i++ ) {
		tiltHistory[tiltHistoryNum&(MAX_TILT_HISTORY-1)][i] = tilts[i];
		sum[i] = 0;
		for ( j = 0 ; j < c ; j++ ) {
			sum[i] += tiltHistory[(tiltHistoryNum-j)&(MAX_TILT_HISTORY-1)][i];
		}
		sum[i] /= c;
	}
	// save the timestamp for analysis and tap detection
	tiltHistory[tiltHistoryNum&(MAX_TILT_HISTORY-1)][3] = tilts[3] - prevTime;	
	prevTime = tilts[3];
	tiltHistoryNum++;
	
	tilt = sum[1];
	tiltPitch = sum[0];
//	Com_Printf( "%4.2f %4.2f %4.2f\n", tilts[0], tilts[1], tilts[2] );
}

void ShowTilt() {
	int	i;
	int	axis = (int)showTilt->value - 1;
	color4_t fillColor = { 255, 0, 0, 255 };
	color4_t whiteColor = { 255, 255, 255, 255 };
	color4_t nowColor = { 0, 255, 0, 255 };
	float	x;
	
	if ( axis < 0 || axis > 2 ) {
		return;
	}
	for ( i = 0 ; i < MAX_TILT_HISTORY ; i++ ) {
		x = tiltHistory[(tiltHistoryNum-1-i)&(MAX_TILT_HISTORY-1)][axis] * ( 10 / 0.018168604 );
		if ( x < 0 ) {
			R_Draw_Fill( 240 + x, i*4, -x, 4, fillColor );
		} else if ( x > 0 ) {
			R_Draw_Fill( 240, i*4, x, 4, fillColor );
		}
	}
	x = tilt * ( 10 / 0.018168604 );
	if ( x < 0 ) {
		R_Draw_Fill( 240 + x, i*4, -x, 4, nowColor );
	} else if ( x > 0 ) {
		R_Draw_Fill( 240, i*4, x, 4, nowColor );
	}
	R_Draw_Fill( 240, 0, 1, MAX_TILT_HISTORY*4, whiteColor );
}

void ShowTime() {
	if ( !showTime->value ) {
		return;
	}
	color4_t sleepColor = { 255, 0, 0, 255 };
	color4_t activeColor = { 0, 255, 0, 255 };
	color4_t swapColor = { 0, 0, 255, 255 };
	color4_t ticColor = { 255, 255, 255, 255 };
	
	for ( int i = 1 ; i < 30 ; i++ ) {
		logTime_t *lt = &loggedTimes[(iphoneFrameNum - i ) & (MAX_LOGGED_TIMES-1)];
		int	sleepTime = ( lt->afterSleep - lt->enterFrame ) >> 7;
		int	activeTime = ( lt->beforeSwap - lt->afterSleep ) >> 7;
		int	swapTime = ( lt->afterSwap - lt->beforeSwap ) >> 7;
		R_Draw_Fill( 0, i * 4, activeTime, 2, activeColor );
		R_Draw_Fill( activeTime, i * 4, swapTime, 2, swapColor );
		R_Draw_Fill( activeTime + swapTime, i * 4, sleepTime, 2, sleepColor );
		
		R_Draw_Fill( 480 - lt->numGameTics * 10, i * 4, lt->numGameTics * 10, 2, ticColor );		
		R_Draw_Fill( 480 - lt->numPingTics * 10, i * 4+2, lt->numPingTics * 10, 2, swapColor );
	}
}


/*
==================
iphoneHighlightPicWhenTouched

Draw transparent except when touched
=================
*/
void iphoneHighlightPicWhenTouched( pkTexture_t *texture, int x, int y, int w, int h ) {
	if ( TouchDown( x, y, w, h ) ) {
		glColor4f(1,1,1,1);
	} else {
		glColor4f(1,1,1,0.5);
	}

	PK_StretchTexture( texture, x, y, w, h );
	glColor4f(1,1,1,1);
}


/*
 ==================
 iphoneSetNotifyText
 
 Notify text is a single centered line for "got a key", "found a secret", etc
 ==================
 */
char	notifyText[128];
int		notifyFrameNum;
void iphoneSetNotifyText( const char *str, ... ) {
	va_list		argptr;

	if ( !messages->value ) {
		// option to disable all the message prints
		return;
	}
	va_start( argptr, str );
	(void)vsnprintf( notifyText, sizeof( notifyText )-1, str, argptr );
	va_end( argptr );

	notifyFrameNum = iphoneFrameNum;
}

void iphoneDrawNotifyText() {
	if ( notifyFrameNum == 0 ) {
		return;
	}
	// display for three seconds, then fade over 0.3
	float f = iphoneFrameNum - notifyFrameNum - 80;
	if ( f < 0 ) {
		f = 1.0;
	} else {
		f = 1.0 - f * 0.1f;
		if ( f < 0 ) {
			notifyFrameNum = 0;
			return;
		}
	}
	
	glColor4f( 1, 1, 1, f );
	iphoneCenterText( 240, 16, 0.75, notifyText );
	glColor4f( 1, 1, 1, 1 );
}


/*
 ==================
 Rotor control
 
 ==================
 */
void iphoneDrawRotorControl( ibutton_t *hud ) {
	if ( hud->buttonFlags & BF_IGNORE ) {
		return;
	}
	pkTexture_t *tex = hud->texture;
	PK_BindTexture( tex );
    
    float	cx = (hud->x + hud->drawWidth / 2);
	float	cy = (hud->y + hud->drawHeight / 2);
	float	as = sin( hud->drawState );
	float	ac = cos( hud->drawState );
    float	sz = (hud->drawWidth / 2);
	
	float	xv[2] = { sz*ac, sz*as };
	float	yv[2] = { -sz*as, sz*ac };
	
	glColor4f( 1, 1, 1, 1 );
	
	glBegin( GL_TRIANGLE_STRIP );
	
	glTexCoord2f( 0.0f, 0.0f );	glVertex2f( (cx - xv[0] - yv[0]), (cy - xv[1] - yv[1]) );
	glTexCoord2f( tex->textureData->maxS, 0.0f );	glVertex2f( (cx + xv[0] - yv[0]), (cy + xv[1] - yv[1]) );
	glTexCoord2f( 0.0f, tex->textureData->maxT );	glVertex2f( (cx - xv[0] + yv[0]), (cy - xv[1] + yv[1]) );
	glTexCoord2f( tex->textureData->maxS, tex->textureData->maxT );	glVertex2f( (cx + xv[0] + yv[0]), (cy + xv[1] + yv[1]) );

    /*
    glTexCoord2f( 0.0f, 0.0f );    glVertex2f( cx - xv[0] - yv[0], cy - xv[1] - yv[1] );
    glTexCoord2f( tex->textureData->maxS, 0.0f );    glVertex2f( cx + xv[0] - yv[0], cy + xv[1] - yv[1] );
    glTexCoord2f( 0.0f, tex->textureData->maxT );    glVertex2f( cx - xv[0] + yv[0], cy - xv[1] + yv[1] );
    glTexCoord2f( tex->textureData->maxS, tex->textureData->maxT );    glVertex2f( cx + xv[0] + yv[0], cy + xv[1] + yv[1] );
    */
    
	glEnd();
}

//===================================================================================




void iphoneDrawHudControl( ibutton_t *hud ) {
    
    if (TARGET_OS_TV) {
        return;
    }
    
	if ( hud->buttonFlags & BF_IGNORE ) {
		return;
	}
	if ( !hud->texture ) {
		return;
	}
	if ( hud->drawAsLimit ) {
		// green tint when at maximum displacement
		glColor4f(0.5,1,0.5,1);
	} else if ( hud->touch || ( hud->buttonFlags & BF_DRAW_ACTIVE ) ) {
		// red tint when active
		glColor4f(1,0.5,0.5,1);
	} else {
		glColor4f(1,1,1,1);
	}	
	if ( hud->scale <= 0 ) {
		hud->scale = 1.0f;
	}
	float w = hud->drawWidth * hud->scale;
	float h = hud->drawHeight * hud->scale;
	float x = hud->x + ( hud->drawWidth - w ) * 0.5f;
	float y = hud->y + ( hud->drawHeight - w ) * 0.5f;
	
	if ( centerSticks->value && hud->touch ) {
		// reposiition the control after each touch
		x = hud->touch->x - w*0.5f;
		y = hud->touch->y - h*0.5f;
	}
    

	PK_StretchTexture( hud->texture, x, y, w, h );
	glColor4f(1,1,1,1);
}

bool NewTextButton( ibutton_t *b,  const char *title, int x, int y, int w, int h ) {
	if ( !b->texture ) {
		const char *pic = (w>128 ? "iphone/long_string_box.tga" : "iphone/short_string_box.tga" );
		SetButtonPicsAndSizes( b, pic, "", x, y, w, h );
		b->buttonFlags = BF_DIMMED | BF_CENTERTEXT;
	}
	b->title = title;
	return HandleButton( b );
}

/*
 =====================================================================
 
 Smart USE test -- determine if there is a usable line within range
 of the player.  This is done by the main thread after running a game tic,
 the async thread just checks the flag, because calling the traverse
 functions is not thread safe.
 
 =====================================================================
*/

static boolean	usableInRange;

boolean PTR_UseTestTraverse (intercept_t* in)
{
	if (!in->d.line->special)
    {
		P_LineOpening (in->d.line);
		if (openrange <= 0)
		{
			// can't use through a wall
			return false;
		}
		
		// not a special line, but keep checking
		
		return true;
    }
	
	player_t*  player = &players[consoleplayer];
	int side = 0;
	if (P_PointOnLineSide (player->mo->x, player->mo->y, in->d.line) == 1)
		side = 1;

	// e6y
	// b.m. side test was broken in boom201
	if ((demoplayback ? (demover != 201) : (compatibility_level != boom_201_compatibility)))
		if (side) //jff 6/1/98 fix inadvertent deletion of side test
			return false;

	if ( in->d.line->special == 76 ) {
		// The button that opens the final door at E1M6 has a trigger line
		// right in front of it, which causes the use button to go down without
		// doing anything, and it gets held down as you close to the actual
		// button, so it doesn't get activated.  You could turn to face the wall
		// to get the use button up, then turn back towards the button, but it
		// feels totally broken.  The correct solution would be to return false
		// for all line specials that won't actually be player-usable, but I can't
		// tell if that is a simple range or a huge scattered list.  For now, this
		// hack to ignore this particular line type solves the problem.  We'll
		// see if other levels have similar issues...
		return false;
	}
	if ( in->d.line->special == 88 ) {	// yellow door above plat on E1M6
		return false;
	}
	
	
	// this is a reasonable target for use
	usableInRange = true;
	
	//WAS can't use for than one special line in a row
	//jff 3/21/98 NOW multiple use allowed with enabling line flag
	
	return (!demo_compatibility && (in->d.line->flags&ML_PASSUSE))?
	true : false;	
}

boolean P_TestUseLines()
{
	int     angle;
	fixed_t x1;
	fixed_t y1;
	fixed_t x2;
	fixed_t y2;
	
	player_t*  player = &players[consoleplayer];
	if ( !player->mo ) {
		return false;	// at intermission
	}
	angle = player->mo->angle >> ANGLETOFINESHIFT;
	
	x1 = player->mo->x;
	y1 = player->mo->y;
	x2 = x1 + (USERANGE>>FRACBITS)*finecosine[angle];
	y2 = y1 + (USERANGE>>FRACBITS)*finesine[angle];
	
	// itterate over the lines and run the callback function
	usableInRange = false;
	P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_UseTestTraverse );
	return usableInRange;
}

//=====================================================================

/*
 ==================
 AutomapControls
 
 This is strictly client-side, done in the game thread instead of the async thread
 ==================
 */
void AutomapControls() {
	//------------------------
	// automap controls
	//------------------------
	extern fixed_t m_x, m_y;     // LL x,y window location on the map (map coords)
	extern fixed_t m_x2, m_y2;   // UR x,y window location on the map (map coords)
	
	// width/height of window on map (map coords)
	extern fixed_t  m_w;
	extern fixed_t  m_h;
	
	// used by MTOF to scale from map-to-frame-buffer coords
	extern  fixed_t scale_mtof;
	// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
	extern fixed_t scale_ftom;
	
	extern fixed_t  min_scale_mtof; // used to tell when to stop zooming out
	extern fixed_t  max_scale_mtof; // used to tell when to stop zooming in
	
	static int prevX = -1, prevY = -1;
	
	// any touch not down in another control will
	// drag-scroll and be claimed by the automap
	int touchCount = 0;
	touch_t	*mapTouch[MAX_TOUCHES];
	for ( int i = 0 ; i < MAX_TOUCHES ; i++ ) {
		touch_t *t = &gameTouches[i];
		if ( t->down ) {
			if ( t->controlOwner == NULL || t->controlOwner == OWNER_AUTOMAP ) {
				// claim it so dragging onto another control won't
				// cause it to activate
				t->controlOwner = OWNER_AUTOMAP;
				mapTouch[touchCount] = t;
				touchCount++;
			}
		}
	}
	if ( touchCount != 1 ) {
		prevX = -1;
	}
	static int pinching;
	if ( touchCount != 2 ) {
		pinching = 0;
	}
	if ( touchCount == 1 ) {
		// adjust the automap values, assume square aspect ratio
		touch_t *t = mapTouch[0];
		// drag position
		if ( prevX == -1 ) {
			prevX = t->x;
			prevY = t->y;
		}
		m_x -= ( t->x - prevX ) * (float)m_w / displaywidth;
		m_y += ( t->y - prevY ) * (float)m_w / displaywidth;
		m_x2 = m_x + m_w;
		m_y2 = m_y + m_h;
		
		prevX = t->x;
		prevY = t->y;		
	} else if ( touchCount == 2 ) {
		// pinch scale
		touch_t *t1 = mapTouch[0];
		touch_t *t2  = mapTouch[1];
		static float baseDist;
		static float baseMtoF;
		static int basem_w;
		static int basem_h;
		float	dist = sqrt( (t2->x-t1->x)*(t2->x-t1->x)+(t2->y-t1->y)*(t2->y-t1->y) );
		if ( !pinching ) {
			pinching = 1;
			baseDist = dist;
			baseMtoF = scale_mtof;
			basem_w = m_w;
			basem_h = m_h;
		}
		scale_mtof = baseMtoF * dist / baseDist;
		if ( scale_mtof < min_scale_mtof ) {
			scale_mtof = min_scale_mtof;
			dist = (float)min_scale_mtof * baseDist / baseMtoF;
		} else if ( scale_mtof > max_scale_mtof ) {
			scale_mtof = max_scale_mtof;
			dist = (float)max_scale_mtof * baseDist / baseMtoF;
		}			
		scale_ftom = FixedDiv(FRACUNIT, scale_mtof);			
		
		float	midx = (t2->x+t1->x)*0.5;
		float	midy = (t2->y+t1->y)*0.5;
		float	midxDoom = m_x + m_w * midx / displaywidth;
		float	midyDoom = m_y + m_w * midy / displaywidth;
		m_w = basem_w * baseDist / dist;
		m_h = basem_h * baseDist / dist;
		m_x = midxDoom - m_w * midx / displaywidth;
		m_y = midyDoom - m_w * midy / displaywidth;
		m_x2 = m_x + m_w;
		m_y2 = m_y + m_h;
	}
	
}

void SwapBuffersAndTouches() {
	// debug graphs
	ShowTilt();
	ShowTime();
	ShowNet();
	ShowSound();
	
	for ( int i = 0 ; i < MAX_TOUCHES ; i++ ) {
		if ( sysTouches[i].down && gameTouches[i].down ) {
			sysTouches[i].controlOwner = gameTouches[i].controlOwner;
		}
	}

	loggedTimes[iphoneFrameNum&(MAX_LOGGED_TIMES-1)].beforeSwap = SysIphoneMicroseconds();
	SysIPhoneSwapBuffers();
	int	now = SysIphoneMicroseconds();
	loggedTimes[iphoneFrameNum&(MAX_LOGGED_TIMES-1)].afterSwap = now;
}

float	weaponSelectDrawScale = 1.25f;
void DrawWeapon(int weaponlump, int x, int y, int w, int h, int lightlevel)
{
	GLTexture *gltexture;
	float fU1,fU2,fV1,fV2;
	int x1,y1,x2,y2;
    
    x *= ((float)displaywidth) / 480.0f;
    y *= ((float)displayheight) / 320.0f;
    w *= ((float)displaywidth) / 480.0f;
    h *= ((float)displayheight) / 320.0f;
    
	// force doom to rebind, since we have changed the active GL_TEXTURE_2D
	last_gltexture = NULL;
	
	gltexture=gld_RegisterPatch(firstspritelump+weaponlump, CR_DEFAULT);
	if (!gltexture)
		return;

	float	scaledWidth = gltexture->width * weaponSelectDrawScale;
	float	scaledHeight = gltexture->height * weaponSelectDrawScale; // JDS Hack
	
	// pin the middle bottom of the patch to the middle bottom of
	// the draw rectangle, then let everything else scale as needed
	fU1=0;
	fV1=0;
	fU2=(float)gltexture->width/(float)gltexture->tex_width;
	fV2=(float)gltexture->height/(float)gltexture->tex_height;
	x1=x+(w-scaledWidth)*0.5;
	x2=x1 + scaledWidth;
	y1=y+h-scaledHeight;
	y2=y+h;;
	
	gld_BindPatch(gltexture, CR_DEFAULT);
	
	glColor4f( lightlevel, lightlevel, lightlevel, 1 );
	
	glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(fU1, fV1); glVertex2f((float)(x1),(float)(y1));
    glTexCoord2f(fU1, fV2); glVertex2f((float)(x1),(float)(y2));
    glTexCoord2f(fU2, fV1); glVertex2f((float)(x2),(float)(y1));
    glTexCoord2f(fU2, fV2); glVertex2f((float)(x2),(float)(y2));
	glEnd();

	glColor4f(1.0f,1.0f,1.0f,1.0f);
}


/*
 ==================
 DrawWeaponSelect
 
 ==================
 */
static const char *weaponNames[9] = {
"fist",
"pistol",
"shotgun",
"chaingun",
"rockets",
"plasma",
"BFG",
"chainsaw",
"dblshotgun"
};
int weaponSprites[9] = {
SPR_PUNG,
SPR_PISG,
SPR_SHTG,
SPR_CHGG,
SPR_MISG,
SPR_PLSG,
SPR_BFGG,
SPR_SAWG,
SPR_SHT2
};

void DrawWeaponSelect() {
	player_t *player = &players[consoleplayer];
	
	for ( int i = wp_fist ; i <= wp_supershotgun ; i++ ) {
		int bx = i % 3;
		int by = i / 3;
		color4_t	color = { 0, 0, 255, 200 };
		color4_t	textColor = { 255, 255, 255, 255 };
		boolean	selectable = false;
		int	ammo = -1;
		switch ( i ) {
			case wp_pistol: ammo = player->ammo[am_clip]; break;
			case wp_shotgun: ammo = player->ammo[am_shell]; break;
			case wp_chaingun: ammo = player->ammo[am_clip]; break;
			case wp_missile: ammo = player->ammo[am_misl]; break;
			case wp_plasma: ammo = player->ammo[am_cell]; break;
			case wp_bfg: ammo = player->ammo[am_cell]; if ( ammo < 40 ) ammo = 0; break;
			case wp_supershotgun: ammo = player->ammo[am_shell]; if ( ammo < 2 ) ammo = 0; break;
			
			
			// These cases were here originally - but they feel buggy.
			/*
			case wp_plasma: ammo = player->ammo[wp_plasma]; break;
			case wp_bfg: ammo = player->ammo[wp_plasma]; if ( ammo < 40 ) ammo = 0; break;
			case wp_supershotgun: ammo = player->ammo[wp_plasma]; if ( ammo < 2 ) ammo = 0; break;
			*/
		}
		if ( !player->weaponowned[i] ) {
			// don't have the weapon
			color[0] = color[1] = color[2] = 50;
			textColor[3] = 128;
		} else {
			// selectable
			color[0] = 255; color[1] = 255; color[2] = 255; color[3] = 255;
			selectable = true;
            
            if ( ammo == 0 ) {
                // have it, but out of ammo
                color[0] = 255; color[1] = color[2] = 0;
                textColor[3] = 128;
            }
		}
		
		int x = bx * 160 + 20;
		int y = by * 88;
		int w = 120;
		int h = 80;
		
        float nx = x * ((float)displaywidth) / 480.0f;
        float ny = y * ((float)displayheight) / 320.0f;
        float nw = w * ((float)displaywidth) / 480.0f;
        float nh = h * ((float)displayheight) / 320.0f;
        
        
		if ( selectable && TouchDown( nx, ny, nw, nh ) ) {
			color[0] = 128;
			color[1] = color[2] = 128;
			color[3] = 200;
		}
		
		glColor4ubv( color );
		
        


		PK_StretchTexture( PK_FindTexture( "iphone/multi_backdrop.tga" ), nx, ny, nw, nh );
//		R_Draw_Blend( x, y, w, h, color );
		
		glColor4ubv( textColor );
		iphoneCenterText( x + w/2, y+16, 0.75, weaponNames[i] );

		// draw the weapon sprite full color if available or black if not
		
		spritedef_t *sprdef = &sprites[weaponSprites[i]];		
		if ( sprdef->spriteframes ) {	// restricted wads won't have all weapons
			spriteframe_t *sprframe = &sprdef->spriteframes[0];
			DrawWeapon( sprframe->lump[0] , x, y - 2, w, h, player->weaponowned[i] );
			
			if ( selectable && TouchReleased( nx, ny, nw, nh ) ) {
				drawWeaponSelect = false;
				weaponSelected = i;			
			}
		}
	}
}

/*
 ==================
 iphoneFrame
 
 This is continuously called by the game thread main loop, any sleeping
 is done explicitly.  If the game isn't holding 30hz, it will be running
 flat out with no sleeping at all.
 ==================
 */
void iphoneFrame() {
	iphoneFrameNum++;
	loggedTimes[iphoneFrameNum&(MAX_LOGGED_TIMES-1)].numGameTics = 0;
	loggedTimes[iphoneFrameNum&(MAX_LOGGED_TIMES-1)].afterSleep =
	loggedTimes[iphoneFrameNum&(MAX_LOGGED_TIMES-1)].enterFrame = SysIphoneMicroseconds();
    
	//-------------------------------------------------
	// grabs the console command under mutex.
	//-------------------------------------------------

	// execute a console command if one was typed
	if ( consoleCommand[0] ) {
		// send it a character at a time to the classic dooom cheat processing
		// for idkfa, idclev, etc
		for ( int i = 0 ; consoleCommand[i] != 0 ; i++ ) {
			M_FindCheats( consoleCommand[i] );
		}
		
		// send it to the new concole command processing
		Com_Printf( "%s\n", consoleCommand );
		Cmd_ExecuteString( consoleCommand );
		consoleCommand[0] = 0;
	}
	
	// move touches to prevTouches (old style use, remove...)
	numPrevTouches = numTouches;
	memcpy( prevTouches, touches, sizeof( prevTouches ) );
	
	// process old style touches
	numTouches = 0;
	for ( int i = 0 ; i < MAX_TOUCHES ; i++ ) {
		touch_t *t = &gameTouches[i];
		if ( t->down ) {
			touches[numTouches][0] = t->x;
			touches[numTouches][1] = t->y;
			numTouches++;
		}
	}
		
	// go to the next demo if needed
	if ( advancedemo ) {
		if ( iphoneTimeDemo && timeDemoStart ) {
			// go back to the menu after a timedemo
			menuState = IPM_MAIN;
			timeDemoStart = 0;
            iphoneMainMenu();
			return;
		}
		static int demoState;
		players[consoleplayer].playerstate = PST_LIVE;  /* not reborn */
		advancedemo = usergame = paused = false;
		gameaction = ga_nothing;
		gamestate = GS_DEMOSCREEN;
		static const char *demoNames[3] = { "demo1", "demo2", "demo3" };
		G_DeferedPlayDemo( demoNames[demoState] );
		if ( ++demoState == 3 ) {
			demoState = 0;
		}
	}

	if ( saveOnExitState == 1 ) {
		printf( "SaveOnExitState == 1\n" );
		if ( !netgame && !demoplayback && usergame && gamestate == GS_LEVEL ) {
			G_SaveGame( 0, "quicksave" );
			G_DoSaveGame(true);
		}
		saveOnExitState = 2;
		return;
	}

	if ( saveOnExitState == 2 ) {
		// the app is exiting
		return;
	}
	
	//--------------------------------------------------------------------------------------
	// game tic processing
	//--------------------------------------------------------------------------------------
	boolean	runGame = false;
	
    if( inBackgroundProcess ) {
        return;
    }
    
	if ( menuState == IPM_GAME ) {
		// don't run the game when in the menus
		runGame = true;
	}
	if ( automapmode & am_active ) {
		// Unlike PC Doom, don't run time when in the automap, since
		// drawing the controls clutters the screen too much.
		runGame = false;		
	}
	if ( netgame ) {
		// even when in the menus or automap, the tics must be processed if it is a net game
		runGame = true;
	}
	
	if ( netGameFailure ) {
		// consistancy failure or interruption
		runGame = false;
	}
	
	// since we don't allow movement control in the automap, 
	// don't advance time.
	if ( runGame ) {		
		int	stopTic;
		
		// block until the AsyncTic() has said we can run at least one frame,
		// unless we are doing a flat-out timedemo run
		if ( iphoneTimeDemo ) {
			stopTic = gametic+1;
			maketic = stopTic+1;
		} else {
			

			loggedTimes[iphoneFrameNum&(MAX_LOGGED_TIMES-1)].afterSleep = SysIphoneMicroseconds();
			if ( localGameID == gameID ) {
				loggedTimes[iphoneFrameNum&(MAX_LOGGED_TIMES-1)].numPingTics = netPlayers[1].peer.currentPingTics;
			} else {
				loggedTimes[iphoneFrameNum&(MAX_LOGGED_TIMES-1)].numPingTics = netServer.currentPingTics;
			}
			
			// On the server, we always want to execute all available tics.
			// For a remote client, that would also give the minimum lag, but things are much
			// smoother if they instead try to leave one buffer tic, unless that would
			// leave the frame without running a single gametic.
			stopTic = maketic;
			if ( consoleplayer != 0 ) {
				// we are a client, so try to leave a buffer frame
				static const int COMMAND_BUFFER_SIZE = 1;
				
				stopTic = maketic - COMMAND_BUFFER_SIZE;
				
				if ( gametic >= stopTic ) {
					// Ideally, clients should run 1 gametic per frame. But if the client is
					// waiting on the server to deliver more tics, the client has to stall
					// if it runs out of buffered tics.
					const int idealTic = gametic + 1;
					
					if ( idealTic < maketic ) {
						stopTic = idealTic;
					} else {
						stopTic = maketic;
					}
				}
			}			
		}
		
		//---------------------------------
		// run game tics
		//---------------------------------
		while( gametic < stopTic ) {
			// run the gametic with all the player and monster logic
			// this will extract netcmds[player][gametic%BACKUPTICS] for each player
//	Com_Printf( "gametic %i\n", gametic );
			G_Ticker();

			// if we just respawned with add-gear, give items now
			if ( addGear ) {
				players[0].weaponowned[wp_shotgun] = true;
				players[0].ammo[am_shell] = 20;
				players[0].pendingweapon = wp_shotgun;
				addGear = false;
			}
			
			// show the network trouble icon if we haven't run a game tic in a long time
			lastGameProcessedTime = SysIphoneMilliseconds();
			
			// see if there is a usable line in front of the player right now,
			// which can be picked up by the asyncTic
			autoUseActive = P_TestUseLines();
			
			// generate the checksum for consistency failure testing
			P_Checksum(gametic);
			
			// on to the next tic
			loggedTimes[iphoneFrameNum&(MAX_LOGGED_TIMES-1)].numGameTics++;
			gametic++;
			
			// this probably doesn't need to be tic-synced, but it doesn't hurt
			if (players[displayplayer].mo) {
				// move positional sounds and free up channels that have completed
                //GUS temporarily removed
                //S_UpdateSounds(players[displayplayer].mo);
			}
		}
	}
	
	
	if ( consoleActive ) {	
		iphoneSet2D();
		// FIXME: actually draw a console...
		//		Console_Draw();
		SwapBuffersAndTouches();
		return;
	}
	if ( menuState != IPM_GAME ) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		iphoneSet2D();
		
        if ( menuState == IPM_HUDEDIT ) {
            HudEditFrame();
        }
        
        SwapBuffersAndTouches();
        
		return;
	}
	
	//------------------
	// any touch release during demo playback goes to main menu
	//------------------
	if ( !usergame && !iphoneTimeDemo ) {
		if ( numTouches == 0 && numPrevTouches == 1 ) {
			menuState = IPM_MAIN;
            iphoneMainMenu();
		}
	}
	
    if( inBackgroundProcess ) {
        return;
    }
    
	// Draw the game screen.  This can also be called by the pacifier update
	// during level loading.
	iphoneDrawScreen();
	
	// If we just loaded a level, do the texture precaching after we
	// have drawn and displayed the first frame, so the user has
	// something to look at while it is loading.
//    if ( false ) { // iphoneFrameNum == levelLoadFrameNum + 1 ) {
//        int    start = SysIphoneMilliseconds();
//        gld_Precache();
//        int end = SysIphoneMilliseconds();
//        Com_Printf( "%3.1f seconds to gld_Precache()\n", (end-start)*0.001f );
//        timeDemoStart = end;
//        timeDemoFrames = 0;
//    }
}

int	pacifierCycle;
int	pacifierTime;

void iphonePacifierUpdate() {
	// Only update a few times a second so it doesn't actually make it
	// take longer to load.
	int	now = SysIphoneMilliseconds();
	if ( now < pacifierTime + 200 ) {
		return;
	}
	pacifierTime = now;
	pacifierCycle = ( pacifierCycle + 1 ) & 7;
	
	// Is this causing a massive slowdown while precaching?
	iphoneDrawScreen();
}

/*
 ==================
 iphoneDrawScreen
 
 Called by the main loop and also during pacifier update when preloading textures
 ==================
 */
void iphoneDrawScreen() {
	// tell the classic code about turning the status bar on or off
	if ( statusBar->modified ) {
		statusBar->modified = false;
		
        if ( statusBar->value ) {
            R_SetViewSize( 10 );
            hud_displayed = 0;
            hud_active = 0;
            hud_distributed = 0;
        } else {
            R_SetViewSize( 11 );
            hud_displayed = 1;
            hud_active = 2;
            hud_distributed = 1;
        }
	}

	//------------------------------------------------
	// Update display with current state.
	//------------------------------------------------

	// force doom to rebind, since we have changed the active GL_TEXTURE_2D
	last_gltexture = NULL;
	
	D_Display();

	iphoneSet2D();	

	//-----------------------------------
	// draw 2D overlays for game screen
	//-----------------------------------
	if ( automapmode & am_active ) {
		if ( HandleButton( &huds.map ) ) {
			AM_Stop();
		}
		if ( !netgame ) {	// no save game option during net play
			static ibutton_t btnSave;
			if ( NewTextButton( &btnSave, "SAVE", 480-64, 0, 64, 32 ) ) {
				G_SaveGame( 0, "ManualSave" );
				G_DoSaveGame(true);
				AM_Stop();
			}
		}
		// update scroll and zoom after the buttons have potentially claimed a touch
		AutomapControls();
	} else if ( iphoneFrameNum == levelLoadFrameNum + 1 ) {	
		// don't draw hud elements during the precache
        color4_t black = { 0, 0, 0, 255 };
        R_Draw_Fill( 0, 0, 480, 320, black );	
        
        
		// draw rotating pacifier icon 
		PK_BindTexture( PK_FindTexture( "iphone/loading.tga" ) );
		glColor4f( 1, 1, 1, 1 );
        
		float	cx = 240 * ((float)displaywidth) / 480.0f;
		float	cy = 160 * ((float)displayheight) / 320.0f;
		float	as = sin( pacifierCycle * M_PI / 4 );
		float	ac = cos( pacifierCycle * M_PI / 4 );
		float	sz = 64;
		
		float	xv[2] = { sz*ac, sz*as };
		float	yv[2] = { -sz*as, sz*ac };
				
		glBegin( GL_TRIANGLE_STRIP );
		
		glTexCoord2f( 0, 0 );	glVertex2f( cx - xv[0] - yv[0], cy - xv[1] - yv[1] );
		glTexCoord2f( 1, 0 );	glVertex2f( cx + xv[0] - yv[0], cy + xv[1] - yv[1] );
		glTexCoord2f( 0, 1 );	glVertex2f( cx - xv[0] + yv[0], cy - xv[1] + yv[1] );
		glTexCoord2f( 1, 1 );	glVertex2f( cx + xv[0] + yv[0], cy + xv[1] + yv[1] );
		
		glEnd();
	} else {
		// normal gameplay
		if ( gamestate == GS_FINALE ) {
			// leave the main menu button on the screen so they can start the
			// next episode
			if ( HandleButton( &huds.menu ) ) {
				iphonePauseMusic();
				menuState = IPM_MAIN;
                iphoneMainMenu();
			}
		} else if ( !menuactive && !demoplayback && usergame && gamestate == GS_LEVEL ) {
			if ( players[consoleplayer].playerstate == PST_DEAD ) {
				// when dead, only show the main menu con and the
				// respawn / load game icons
#if !TARGET_OS_TV
				if ( HandleButton( &huds.menu ) ) {
					iphonePauseMusic();
					menuState = IPM_MAIN;
                    iphoneMainMenu();
				}
#endif
				if ( !deathmatch && !netgame ) {
                    
                    // for now we're going to not draw these on the screen for the TV version
                    // all you can do is respawn with the USE button (A)
                    
#if TARGET_OS_TV
                    static ibutton_t btnRespawn;
                    SetButtonPicsAndSizes( &btnRespawn, "iphone/respawn.tga", "Press A to restart", 240 - 48, 80, 96, 96 );
                    if ( HandleButton( &btnRespawn ) ) {
//                        players[consoleplayer].playerstate = PST_REBORN;
                    }
#else
					static ibutton_t btnSaved;
					static ibutton_t btnRespawn;
					static ibutton_t btnGear;
					
					if ( !btnSaved.texture ) {
						// initial setup
						SetButtonPicsAndSizes( &btnSaved, "iphone/load_saved.tga", "Saved game", 240 - 48 - 96 - 48, 80, 96, 96 );
						SetButtonPicsAndSizes( &btnRespawn, "iphone/respawn.tga", "Restart", 240 - 48, 80, 96, 96 );
						SetButtonPicsAndSizes( &btnGear, "iphone/respawn_gear.tga", "Add gear", 240 + 48 + 48, 80, 96, 96 );
					}
					
					if ( HandleButton( &btnSaved ) ) {
						StartSaveGame();
					}
					if ( HandleButton( &btnRespawn ) ) {
					    players[consoleplayer].playerstate = PST_REBORN;	
					}
					if ( HandleButton( &btnGear ) ) {
					    players[consoleplayer].playerstate = PST_REBORN;
						addGear = true;
					}
#endif
				} else {
					static ibutton_t btnNetRespawn;
					if ( !btnNetRespawn.texture ) {
						// initial setup
						SetButtonPicsAndSizes( &btnNetRespawn, "iphone/respawn.tga", "Respawn", 240 - 96/2, 90, 96, 96 );
					}
					if ( HandleButton( &btnNetRespawn ) ) {
						// this will cause the next command sent to include a use action,
						// then clear this flag
					    respawnActive = true;	
					}
				}
			} else if ( drawWeaponSelect ) {
				DrawWeaponSelect();
			} else {
				if ( drawControls->value ) {
					iphoneDrawHudControl( &huds.forwardStick );
					iphoneDrawHudControl( &huds.sideStick );
					iphoneDrawHudControl( &huds.turnStick );
					iphoneDrawRotorControl( &huds.turnRotor );
//					iphoneDrawHudControl( &huds.fire );
				}
                
                if (!TARGET_OS_TV) {
                    
                    if ( HandleButton( &huds.menu ) ) {
                        iphonePauseMusic();
                        menuState = IPM_MAIN;
                        iphoneMainMenu();
                    }
                    if ( HandleButton( &huds.map ) ) {
                        AM_Start();
                    }
                    if ( HandleButton( &huds.fire ) ) {
                    }
                }
				
				if ( netgame ) {
#if 0				
					static ibutton_t btnPlayer;
					if ( NewTextButton( &btnPlayer, "PLAYER", 0, 48, 100, 32 ) ) {
						displayplayer ^= 1;
					}
					
					static ibutton_t btnNet;
					if ( NewTextButton( &btnNet, "NET", 240-32, 0, 80, 32 ) ) {
						showNet->value = !showNet->value;
					}
					static ibutton_t btnThrottle;
					const char *title = throttle->value ? "Throttle:ON" : "Throttle:OFF";
					if ( NewTextButton( &btnThrottle, title, 0, 0, 128, 32 ) ) {
						throttle->value = !throttle->value;
					}
#endif
				}

#if 0				
				static ibutton_t btnSpeeds;
				if ( NewTextButton( &btnSpeeds, "SPEEDS", 240-32, 0, 80, 32 ) ) {
					showTime->value = !showTime->value;
				}
				static ibutton_t btnTest;
				if ( NewTextButton( &btnTest, "TEST", 0, 48, 70, 32 ) ) {
					testNewRenderer = !testNewRenderer;
				}
#endif				
			}

			// notify text last, so it is always on top and legible
			iphoneDrawNotifyText();
			
			// not getting network tics
			const int timeSinceLastProcessed = SysIphoneMilliseconds() - lastGameProcessedTime;
			if ( timeSinceLastProcessed > 800 ) { // Origianlly was 500, let's be a little more lenient for internet play.
				PK_StretchTexture( PK_FindTexture("iphone/multiplay.tga"), 240 - 96/2, 90, 96, 96 );
			}
			
			// draw the little graph in the upper right corner
#ifndef NDEBUG
			if ( netgame ) {
				ShowMiniNet();
			}
#endif
		}
	}
	
	// update timedemo display
	if ( iphoneTimeDemo ) {
		if ( iphoneFrameNum > levelLoadFrameNum + 1 ) {
			timeDemoFrames++;
			float	fps = timeDemoFrames * 1000.0f / ( SysIphoneMilliseconds() - timeDemoStart );
			sprintf( timeDemoResultString, "%5.1f fps", fps );
		} else {
			strcpy( timeDemoResultString, "TIMEDEMO" );
		}
		iphoneCenterText( 240, 80, 0.75, timeDemoResultString );
	}
	
	// time how long the GPU takes to render the entire frame
	if ( glfinish->value ) {
		int start = SysIphoneMicroseconds();
		glFinish();
		int end = SysIphoneMicroseconds();
		Com_Printf( "%4.1f msec for glFinish()\n", ( end - start ) * 0.001f );
	}
	
	SwapBuffersAndTouches();
}
