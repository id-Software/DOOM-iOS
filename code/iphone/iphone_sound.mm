/*
 *  iphone_sound.c
 *  doom
 *
 *  Created by John Carmack on 4/16/09.
 *  Copyright 2009 Id Software. All rights reserved.
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
#include "w_wad.h"
#include <AudioToolbox/AudioServices.h>
#import <AVFoundation/AVAudioSession.h>
#import <Foundation/Foundation.h>

typedef struct  {
	unsigned			sourceName;		// OpenAL sourceName
	pkWav_t				*sfx;			// NULL if unused
	float				volume;			// stored for showSound display
} channel_t;

static ALCcontext *Context;
static ALCdevice *Device;

@interface AudioInterruptionListener : NSObject  {
}
- (void)handleAudioSessionInterruption:(NSNotification *)notification;
- (void)registerInterruptionListener;
@end

@implementation AudioInterruptionListener {
}
- (void)registerInterruptionListener {
    
[[NSNotificationCenter defaultCenter] addObserver:self
                                         selector:@selector(handleAudioSessionInterruption:)
                                             name:AVAudioSessionInterruptionNotification
                                           object:[AVAudioSession sharedInstance]];

}

- (void)handleAudioSessionInterruption:(NSNotification *)notification {
    NSDictionary *interruptionDict = notification.userInfo;
    NSInteger interruptionType = [[interruptionDict valueForKey:AVAudioSessionInterruptionTypeKey] integerValue];
    printf("Session interrupted! --- %s ---\n", interruptionType == AVAudioSessionInterruptionTypeBegan ? "Begin Interruption" : "End Interruption");
    
    /* JDS Log interruption for now but don't do anything with it yet
    NSError* error = nil;
    switch (interruptionType) {
        case AVAudioSessionInterruptionTypeBegan:
            printf("Audio interrupted.\n" );
            iphonePauseMusic();
            alcMakeContextCurrent( NULL );
            
            [[AVAudioSession sharedInstance] setActive:NO error:&error];
            
            UInt32 otherAudioIsPlaying = [[AVAudioSession sharedInstance] isOtherAudioPlaying];
            Com_Printf("OtherAudioIsPlaying = %d\n", otherAudioIsPlaying );
            
            // If other audio is playing now, switch to the Ambient category so it
            // can continue in the background. If not, use the default category.
            if ( otherAudioIsPlaying ) {
                
                NSError *error = nil;
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:&error];
                
                if( error != nil ) {
                    NSLog(@"%@", error);
                }
                
            } else {
                NSError *error = nil;
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategorySoloAmbient error:&error];
                
                if( error != nil ) {
                    NSLog(@"%@", error);
                }
            }

            break;
            
        case AVAudioSessionInterruptionTypeEnded:
            printf("Audio restored.\n" );
            
            UInt32 otherAudioIsPlaying = [[AVAudioSession sharedInstance] isOtherAudioPlaying];
            Com_Printf("OtherAudioIsPlaying = %d\n", otherAudioIsPlaying );
            
            // If other audio is playing now, switch to the Ambient category so it
            // can continue in the background. If not, use the default category.
            if ( otherAudioIsPlaying ) {
                
                NSError *error = nil;
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:&error];
                
                if( error != nil ) {
                    NSLog(@"%@", error);
                }

            } else {
                NSError *error = nil;
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategorySoloAmbient error:&error];
                
                if( error != nil ) {
                    NSLog(@"%@", error);
                }
            }

            [[AVAudioSession sharedInstance] setActive:YES error:&error];
            if( error != nil ) {
                NSLog(@"%@", error);
            }
            alcMakeContextCurrent( Context );
            if( alcGetError( Device ) != ALC_NO_ERROR ) {
                Com_Error( "Failed to alcMakeContextCurrent\n" );
            }
            iphoneResumeMusic();
    }
     */
}
@end

#define MAX_CHANNELS		16
static channel_t	s_channels[ MAX_CHANNELS ];

cvar_t	*s_sfxVolume;

void Sound_StartLocalSound( const char *filename ) {
	Sound_StartLocalSoundAtVolume( filename, 1.0f );
}

void Sound_StartLocalSoundAtVolume( const char *filename, float volume ) {
	pkWav_t	*sfx;
	
	sfx = PK_FindWav( filename );
	if( ! sfx ) {
		Com_Printf( "Sound_StartLocalSound: could not cache (%s)\n", filename );
		return;
	}
//	printf( "sound:%s\n", filename );	
	// channel 0 is reserved for UI sounds, the other channels
	// are for DOOM sounds
	channel_t *ch = &s_channels[ 0 ];
	
	ch->sfx = sfx;
	ch->volume = s_sfxVolume->value * volume;
	
	alSourceStop( ch->sourceName );
	alSourcef( ch->sourceName, AL_GAIN, ch->volume );
	alSourcei( ch->sourceName, AL_BUFFER, sfx->alBufferNum );
	alSourcef( ch->sourceName, AL_PITCH, 1.0f );
	alSourcePlay( ch->sourceName );
}


static void Sound_Play_f( void ) {
	if( Cmd_Argc() == 1 ) {
		Com_Printf( "Usage: play <soundfile>\n" );
		return;
	}
	Sound_StartLocalSound( Cmd_Argv( 1 ) );
}

// we won't allow music to be toggled on or off in the menu when this is true
int otherAudioIsPlaying;

int SysIPhoneOtherAudioIsPlaying() {
	return otherAudioIsPlaying;
}

void Sound_Init(void) {

	Com_Printf( "\n------- Sound Initialization -------\n" );
	
    s_sfxVolume		= Cvar_Get( "s_sfxVolume", "1.0", static_cast<CVARFlags>(0) );
	
	Cmd_AddCommand( "play", Sound_Play_f );
	
	// make sure background ipod music mixes with our sound effects
	Com_Printf( "...Initializing AudioSession\n" );
   
	//OSStatus status = 0;
    
    /* JDS FIXME [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(interruptionListener:)
                                                 name:AVAudioSessionInterruptionNotification object:[AVAudioSession sharedInstance]];
     */
    AudioInterruptionListener *listener = [[AudioInterruptionListener alloc] init];
    [listener registerInterruptionListener];
    
     // JDS deprecated status = AudioSessionInitialize(NULL, NULL, interruptionListener, NULL);	// else "couldn't initialize audio session"

    // if there is iPod music playing in the background, we want to use
	// the AmbientSound catagory, otherwise we will leave it at the default.
	// If we always set it to AmbientSound, then the mp3 background music
	// playback goes to software on 3.0 for a huge slowdown.
	UInt32 otherAudioIsPlaying = [[AVAudioSession sharedInstance] isOtherAudioPlaying];
    //JDS deprecated UInt32  propOtherAudioIsPlaying = kAudioSessionProperty_OtherAudioIsPlaying;
	//JDS deprecated UInt32  size = sizeof( otherAudioIsPlaying );
    //AudioServicesGetProperty( propOtherAudioIsPlaying, &size, &otherAudioIsPlaying );
    
	Com_Printf("OtherAudioIsPlaying = %d\n", otherAudioIsPlaying );
    
    // JDS FIXME
	if ( 1 ) {
		//UInt32 audioCategory = kAudioSessionCategory_AmbientSound;
        
        // Allow sound to play in the background
        NSError *error = nil;
        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:&error];
        
        if( error != nil ) {
            NSLog(@"%@", error);
        }

		//status = AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory);
	}
	
    NSError* error = nil;
    [[AVAudioSession sharedInstance] setActive:YES error:&error];
    if( error != nil ) {
        NSLog(@"%@", error);
    }
	//JDS deprecated status = AudioSessionSetActive(true);                                       // else "couldn't set audio session active\n"
    
	Com_Printf( "...Initializing OpenAL subsystem\n" );
	
	// get the OpenAL device
	Device = alcOpenDevice( NULL );
	if( Device == NULL ) {
		Com_Printf( "Failed to alcOpenDevice\n" );
	}
	
	// Create context(s)
	Context = alcCreateContext( Device, NULL );
	if( Context == NULL ) {
		Com_Error( "Failed to alcCreateContext\n" );
	}
	
	// Set active context
	alcGetError( Device );
	alcMakeContextCurrent( Context );
	if( alcGetError( Device ) != ALC_NO_ERROR ) {
		Com_Error( "Failed to alcMakeContextCurrent\n" );
	}
	
	// allocate all the channels we are going to use
	channel_t	*ch;
	int			i;
	for( i = 0, ch = s_channels ; i < MAX_CHANNELS ; ++i, ++ch ) {
		alGenSources( 1, &ch->sourceName );
		
		if( alGetError() != AL_NO_ERROR ) {
			Com_Error( "Allocating AL sound sources" );
		}
		alSourcei( ch->sourceName, AL_SOURCE_RELATIVE, AL_FALSE );
	}
	
	Com_Printf( "------------------------------------\n" );
}

/*
 ==================
 ShowSound
 
 Display active sound channels
 ==================
 */
void ShowSound() {

	if ( !showSound->value ) {
		return;
	}
	channel_t	*ch;
	int			i;
	for( i = 0, ch = s_channels ; i < MAX_CHANNELS ; ++i, ++ch ) {
		int state;
		alGetSourcei( ch->sourceName, AL_SOURCE_STATE, &state );
		if ( state != AL_PLAYING ) {
			continue;
		}
		
		int v = ch->volume * 255;
		if ( v > 255 ) {
			v = 255;
		}
        color4_t color = { static_cast<unsigned char>(v), static_cast<unsigned char>(v), static_cast<unsigned char>(v), 255 };
		R_Draw_Fill( i*16, 0, 12, 12, color );		
	}
}



/*
 ==================================================================

 PrBoom interface
 
 ==================================================================
*/

// Initialize channels?
void I_SetChannels(void) {}

// Get raw data lump index for sound descriptor.
int I_GetSfxLumpNum (sfxinfo_t *sfx) {
	// find the pkWav_t for this sfxinfo
	char	upper[16], *d = upper;
	for ( const char *c = sfx->name ; *c ; c++ ) {
		*d++ = toupper( *c );
	}
	*d = 0;
	pkWav_t *pkwav = PK_FindWav( va( "newsfx/DS%s.wav", upper ) );	
	
	return (int) (pkwav - pkWavs);
}

// Starts a sound in a particular sound channel.
// volume ranges 0 - 64
// seperation tanges is 128 straight ahead, 0 = all left ear, 255 = all right ear
// pitch centers around 128
long int I_StartSound(int sfx_id, int channel, int vol, int sep, int pitch, int priority) {

	sfxinfo_t *dsfx = &S_sfx[sfx_id];
	
	assert( dsfx->lumpnum >= 0 && dsfx->lumpnum < pkHeader->wavs.count );
	
	pkWav_t *sfx = &pkWavs[dsfx->lumpnum];
//	printf( "sound: %s chan:%i vol:%i sep:%i pitch:%i priority:%i\n", sfx->wavData->name.name, channel, vol, sep, pitch, priority );	

	assert( channel >= 0 && channel < MAX_CHANNELS - 1 );
	channel_t *ch = &s_channels[ 1+channel ];
	
	alSourceStop( ch->sourceName );
	if ( ch->sfx == sfx ) {
		// restarting the same sound
		alSourceRewind( ch->sourceName );
	} else {
		alSourcei( ch->sourceName, AL_BUFFER, sfx->alBufferNum );
	}
	
	ch->sfx = sfx;
	ch->volume = s_sfxVolume->value * vol / 64.0;
	alSourcef( ch->sourceName, AL_GAIN, ch->volume );
	alSourcef( ch->sourceName, AL_PITCH, pitch / 128.0f );
	alSourcePlay( ch->sourceName );
	
	return (long int)ch;
}

// Stops a sound channel.
void I_StopSound(long int handle) {}

// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
boolean I_SoundIsPlaying(long int handle) {

	channel_t *ch = (channel_t *)handle;
	if ( !ch ) {
		return false;
	}
	int state;
    
	alGetSourcei( ch->sourceName, AL_SOURCE_STATE, &state );
	
	return state == AL_PLAYING;
}

// Called by m_menu.c to let the quit sound play and quit right after it stops
boolean I_AnySoundStillPlaying(void) { return false; }

// Updates the volume, separation,
//  and pitch of a sound channel.
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch) {}

// This function overwrites the audio data loaded from the iPack file
// with the lumps from the loaded WAD files.
void I_OverwriteSoundBuffersWithLumps() {
    for ( int i = 0 ; i < pkHeader->wavs.count ; i++ ) {
        pkWav_t *sfx = &pkWavs[i];
        sfx->wavData = (pkWavData_t *)( (byte *)pkHeader + pkHeader->wavs.tableOfs + i * pkHeader->wavs.structSize );
        
        // get the lump instead of the normal sound effect
        const char* sfxname = sfx->wavData->name.name;
        
        // strip off the path name
        // JDS FIXME: clean this up
        while(*sfxname && *sfxname != '/') sfxname++;
        if( sfxname ) sfxname++;
        
        // If this is a Doom sound...
        // JDS FIXME: clean this up
        if( sfxname && tolower(sfxname[0]) == 'd' && tolower(sfxname[1]) == 's' ) {
            char* sfxname_noext = strdup(sfxname);
            char* sfxname_noext_s = sfxname_noext;
            // strip off extension, convert to uppercase
            // JDS FIXME: clean this up
            while( *sfxname_noext && *sfxname_noext != '.' ) {
                *sfxname_noext = toupper(*sfxname_noext);
                sfxname_noext++;
            }
            *sfxname_noext = '\0';
            sfxname_noext = sfxname_noext_s;
            // extract the lump
            // BUG! Need to parse the Doom sound header format vs.
            // hardcoding all this
            int lumpNum = (W_CheckNumForName)( sfxname_noext,0 );
            free(sfxname_noext);
            if( lumpNum != -1 ) {
                int lumpSize = W_LumpLength( lumpNum );
                byte* replacementSound = (byte*)malloc( lumpSize );
                replacementSound = (byte*)W_CacheLumpNum( lumpNum );
                uint16_t sampleRate = *((uint16_t*)(replacementSound + 2));
                uint32_t numSamples = (*((uint32_t*)(replacementSound + 4)) - 32); // remove 32 bytes of padding
                alBufferData( sfx->alBufferNum, AL_FORMAT_MONO8, replacementSound + 0x18
                     , numSamples
                     , sampleRate );
            }
        }
    }
}
