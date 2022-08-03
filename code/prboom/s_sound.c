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
 * DESCRIPTION:  Platform-independent sound code
 *
 *-----------------------------------------------------------------------------*/

// killough 3/7/98: modified to allow arbitrary listeners in spy mode
// killough 5/2/98: reindented, removed useless code, beautified

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doomstat.h"
#include "s_sound.h"
#include "i_sound.h"
#include "i_system.h"
#include "d_main.h"
#include "r_main.h"
#include "m_random.h"
#include "w_wad.h"
#include "lprintf.h"

// when to clip out sounds
// Does not fit the large outdoor areas.
#define S_CLIPPING_DIST (1200<<FRACBITS)

// Distance tp origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).

#define S_CLOSE_DIST (160<<FRACBITS)
#define S_ATTENUATOR ((S_CLIPPING_DIST-S_CLOSE_DIST)>>FRACBITS)

// Adjustable by menu.
#define NORM_PITCH 128
#define NORM_PRIORITY 64
#define NORM_SEP 128
#define S_STEREO_SWING (96<<FRACBITS)

const char* S_music_files[NUMMUSIC]; // cournia - stores music file names

typedef struct
{
  sfxinfo_t *sfxinfo;  // sound information (if null, channel avail.)
  void *origin;        // origin of sound
  long int handle;          // handle of the sound being played (JDS: 64-bit)
  int is_pickup;       // killough 4/25/98: whether sound is a player's weapon
	int volume;			// JDC: perform overrides based on dynamic volume instead of static priority
} channel_t;

// the set of channels available
static channel_t *channels;

// These are not used, but should be (menu).
// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
int snd_SfxVolume = 15;

// Maximum volume of music. Useless so far.
int snd_MusicVolume = 15;

// whether songs are mus_paused
static boolean mus_paused;

// music currently being played
static musicinfo_t *mus_playing;

// following is set
//  by the defaults code in M_misc:
// number of channels available
int default_numChannels;
int numChannels;

//jff 3/17/98 to keep track of last IDMUS specified music num
int idmusnum;

//
// Internals.
//

void S_StopChannel(int cnum);

int S_AdjustSoundParams(mobj_t *listener, mobj_t *source,
                        int *vol, int *sep, int *pitch);

// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//

void S_Init(int sfxVolume, int musicVolume)
{
  //jff 1/22/98 skip sound init if sound not enabled
  numChannels = default_numChannels;
  if (snd_card && !nosfxparm)
  {
    int i;

    lprintf(LO_CONFIRM, "S_Init: default sfx volume %d\n", sfxVolume);

    // Whatever these did with DMX, these are rather dummies now.
    I_SetChannels();

    S_SetSfxVolume(sfxVolume);

    // Allocating the internal channels for mixing
    // (the maximum numer of sounds rendered
    // simultaneously) within zone memory.
    // CPhipps - calloc
    channels =
      (channel_t *) calloc(numChannels,sizeof(channel_t));

    // Note that sounds have not been cached (yet).
    for (i=1 ; i<NUMSFX ; i++)
      S_sfx[i].lumpnum = S_sfx[i].usefulness = -1;
  }

  // CPhipps - music init reformatted
  if (mus_card && !nomusicparm) {
    S_SetMusicVolume(musicVolume);

    // no sounds are playing, and they are not mus_paused
    mus_paused = 0;
  }
}

void S_Stop(void)
{
  int cnum;

  //jff 1/22/98 skip sound init if sound not enabled
  if (snd_card && !nosfxparm)
    for (cnum=0 ; cnum<numChannels ; cnum++)
      if (channels[cnum].sfxinfo)
        S_StopChannel(cnum);
}

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void)
{
  int mnum;

  // kill all playing sounds at start of level
  //  (trust me - a good idea)

    //S_Stop();
    //Gus Temporarily Disable
    // MAY NEED TVOS STUFF HERE -tkidd
  S_Stop();

  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;

  // start new music for the level
  mus_paused = 0;

  if (idmusnum!=-1)
    mnum = idmusnum; //jff 3/17/98 reload IDMUS music if not -1
  else
    if (gamemode == commercial)
      mnum = mus_runnin + gamemap - 1;
    else
      {
        static const int spmus[] =     // Song - Who? - Where?
        {
          mus_e3m4,     // American     e4m1
          mus_e3m2,     // Romero       e4m2
          mus_e3m3,     // Shawn        e4m3
          mus_e1m5,     // American     e4m4
          mus_e2m7,     // Tim  e4m5
          mus_e2m4,     // Romero       e4m6
          mus_e2m6,     // J.Anderson   e4m7 CHIRON.WAD
          mus_e2m5,     // Shawn        e4m8
          mus_e1m9      // Tim          e4m9
        };

          static const int sigilmus[] =     // Song - Who? - Where?
          {
              mus_e5m1,     // Buckethead   e5m1
              mus_e5m2,     // Buckethead   e5m2
              mus_e5m3,     // Buckethead   e5m3
              mus_e5m4,     // Buckethead   e5m4
              mus_e5m5,     // Buckethead   e5m5
              mus_e5m6,     // Buckethead   e5m6
              mus_e5m7,     // Buckethead   e5m7
              mus_e5m8,     // Buckethead   e5m8
              mus_e5m9      // Buckethead   e5m9
          };
          
        if (gameepisode < 4)
          mnum = mus_e1m1 + (gameepisode-1)*9 + gamemap-1;
        else if (gameepisode == 4)
          mnum = spmus[gamemap-1];
        else
          mnum = sigilmus[gamemap-1];
      }
  S_ChangeMusic(mnum, true);
}

void S_StartSoundAtVolume(void *origin_p, int sfx_id, int volume)
{
  int sep, pitch, priority, cnum, is_pickup;
  sfxinfo_t *sfx;
  mobj_t *origin = (mobj_t *) origin_p;

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return;

  is_pickup = sfx_id & PICKUP_SOUND || sfx_id == sfx_oof || (compatibility_level >= prboom_2_compatibility && sfx_id == sfx_noway); // killough 4/25/98
  sfx_id &= ~PICKUP_SOUND;

  // check for bogus sound #
  if (sfx_id < 1 || sfx_id > NUMSFX)
    I_Error("S_StartSoundAtVolume: Bad sfx #: %d", sfx_id);

  sfx = &S_sfx[sfx_id];

  // Initialize sound parameters
  if (sfx->link)
    {
      pitch = sfx->pitch;
      priority = sfx->priority;
      volume += sfx->volume;

      if (volume < 1)
        return;

      if (volume > snd_SfxVolume)
        volume = snd_SfxVolume;
    }
  else
    {
      pitch = NORM_PITCH;
      priority = NORM_PRIORITY;
    }

  // Check to see if it is audible, modify the params
  // killough 3/7/98, 4/25/98: code rearranged slightly

  if (!origin || origin == players[displayplayer].mo) {
    sep = NORM_SEP;
    volume *= 8;
  } else
    if (!S_AdjustSoundParams(players[displayplayer].mo, origin, &volume,
                             &sep, &pitch)) {
      return;
	}
    else
      if ( origin->x == players[displayplayer].mo->x &&
           origin->y == players[displayplayer].mo->y)
        sep = NORM_SEP;

  // hacks to vary the sfx pitches
  if (sfx_id >= sfx_sawup && sfx_id <= sfx_sawhit)
    pitch += 8 - (M_Random()&15);
  else
    if (sfx_id != sfx_itemup && sfx_id != sfx_tink)
      pitch += 16 - (M_Random()&31);

  if (pitch<0)
    pitch = 0;

  if (pitch>255)
    pitch = 255;

#if 0
  // kill old sound
  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (channels[cnum].sfxinfo && channels[cnum].origin == origin &&
        (comp[comp_sound] || channels[cnum].is_pickup == is_pickup))
      {
        S_StopChannel(cnum);
        break;
      }
	
  // try to find a channel
  cnum = S_getChannel(origin, sfx, is_pickup);
#else
	// JDC: new sound channel logic
	
	// Find look for a channel that we should override
	// "pickup" acts like a one bit channel-on-emitter field,
	// so pickup sounds woon't override action sounds and vise-versa.
	// In later games we have explicit CHAN_VOICE, CHAN_FOOTSTEP, CHAN_WEAPON, etc
	cnum = numChannels;
	for (cnum=0; cnum<numChannels ; cnum++) {
		if ( channels[cnum].origin == origin &&
			channels[cnum].is_pickup == is_pickup) {
			break;
		}
	}
	if ( cnum == numChannels ) {
		// second, look for a completely free channel
		for (cnum=0; cnum<numChannels ; cnum++) {
			if ( !channels[cnum].sfxinfo ) {
				break;
			}
		}
	}
	if ( cnum == numChannels ) {
		// third, look for a lower volume sound to override
		// It is better to do this based on volume than on static
		// prioritites.
		int	lowestVolume = volume+1;	// override an older sound of same volume
		int	test;
		for (test=0; test<numChannels ; test++) {
			if ( channels[test].volume < lowestVolume ) {
				lowestVolume = channels[test].volume;
				cnum = test;
			}
		}
	}
	
	if ( cnum == numChannels ) {
		// nothing available
		printf( "dropping sound for no channels available (%i)\n", numChannels );
		return;
	}
	
	// we are using this channel now
	channels[cnum].sfxinfo = sfx;
	channels[cnum].origin = origin;
	channels[cnum].volume = volume;
	channels[cnum].is_pickup = is_pickup;
#endif	
	
	
  if (cnum<0)
    return;

  // get lumpnum if necessary
  // killough 2/28/98: make missing sounds non-fatal
  if (sfx->lumpnum < 0 && (sfx->lumpnum = I_GetSfxLumpNum(sfx)) < 0)
    return;

  // increase the usefulness
  if (sfx->usefulness++ < 0)
    sfx->usefulness = 1;

  // Assigns the handle to one of the channels in the mix/output buffer.
  { // e6y: [Fix] Crash with zero-length sounds.
    // JDS 64-bit handles
      long int h = I_StartSound(sfx_id, cnum, volume, sep, pitch, priority);
    if (h != -1) channels[cnum].handle = h;
  }
}

void S_StartSound(void *origin, int sfx_id)
{
  S_StartSoundAtVolume(origin, sfx_id, snd_SfxVolume);
}

void S_StopSound(void *origin)
{
  int cnum;

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return;

  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
      {
        S_StopChannel(cnum);
        break;
      }
}


//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound(void)
{
  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;

  if (mus_playing && !mus_paused)
    {
      I_PauseSong(mus_playing->handle);
      mus_paused = true;
    }
}

void S_ResumeSound(void)
{
  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;

  if (mus_playing && mus_paused)
    {
      I_ResumeSong(mus_playing->handle);
      mus_paused = false;
    }
}


//
// Updates music & sounds
//
void S_UpdateSounds(void* listener_p)
{
  mobj_t *listener = (mobj_t*) listener_p;
  int cnum;

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return;

#ifdef UPDATE_MUSIC
  I_UpdateMusic();
#endif

  for (cnum=0 ; cnum<numChannels ; cnum++)
    {
      sfxinfo_t *sfx;
      channel_t *c = &channels[cnum];
      if ((sfx = c->sfxinfo))
        {
          if (I_SoundIsPlaying(c->handle))
            {
              // initialize parameters
              int volume = snd_SfxVolume;
              int pitch = NORM_PITCH;
              int sep = NORM_SEP;

              if (sfx->link)
                {
                  pitch = sfx->pitch;
                  volume += sfx->volume;
                  if (volume < 1)
                    {
                      S_StopChannel(cnum);
                      continue;
                    }
                  else
                    if (volume > snd_SfxVolume)
                      volume = snd_SfxVolume;
                }

              // check non-local sounds for distance clipping
              // or modify their params
              if (c->origin && listener_p != c->origin) { // killough 3/20/98
                if (!S_AdjustSoundParams(listener, c->origin,
                                         &volume, &sep, &pitch))
                  S_StopChannel(cnum);
                else
                  I_UpdateSoundParams((int)c->handle, volume, sep, pitch);
        }
            }
          else   // if channel is allocated but sound has stopped, free it
            S_StopChannel(cnum);
        }
    }
}



void S_SetMusicVolume(int volume)
{
  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;
  if (volume < 0 || volume > 15)
    I_Error("S_SetMusicVolume: Attempt to set music volume at %d", volume);
  I_SetMusicVolume(volume);
  snd_MusicVolume = volume;
}



void S_SetSfxVolume(int volume)
{
  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return;
  if (volume < 0 || volume > 127)
    I_Error("S_SetSfxVolume: Attempt to set sfx volume at %d", volume);
  snd_SfxVolume = volume;
}



// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(int m_id)
{
  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;
  S_ChangeMusic(m_id, false);
}



void S_ChangeMusic(int musicnum, int looping)
{
  musicinfo_t *music;
  int music_file_failed; // cournia - if true load the default MIDI music
  char music_filename[ 1024 ];  // cournia

  (void)music_file_failed;
  (void)music_filename;

  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;

  if (musicnum <= mus_None || musicnum >= NUMMUSIC)
    I_Error("S_ChangeMusic: Bad music number %d", musicnum);

  music = &S_music[musicnum];
#ifndef IPHONE	// music was paused in the menu, we want it to always restart even if starting the same level again
  if (mus_playing == music)
    return;
#endif

/*	
#ifdef IPHONE
	extern void iphonePlayMusic( const char *name );
	iphonePlayMusic( music->name );
	
#else
*/
  // shutdown old music
  S_StopMusic();

  // get lumpnum if neccessary
  if (!music->lumpnum)
    {
      char namebuf[9];
      sprintf(namebuf, "d_%s", music->name);
      music->lumpnum = W_GetNumForName(namebuf);
    }
    
  music_file_failed = 1;

  // proff_fs - only load when from IWAD
  //if (lumpinfo[music->lumpnum].source == source_iwad)
//    {
      // cournia - check to see if we can play a higher quality music file
      //           rather than the default MIDI

    I_FindFile(S_music_files[musicnum], "", music_filename);
      if ( music_filename[0] != '\0' )
        {
          music_file_failed = I_RegisterMusic(music_filename, music);
          //free(music_filename);
        }

    //    }

  if (music_file_failed)
    {
      //cournia - could not load music file, play default MIDI music
        
      // load & register it
      music->data = W_CacheLumpNum(music->lumpnum);
      music->handle = I_RegisterSong(music->data, W_LumpLength(music->lumpnum));
    }
    
    extern void iphoneStartMP3( const char *name );
    extern boolean    sigilShredsLoaded;    // Buckethead!

    if (sigilShredsLoaded) {
        
        char* mp3filename = I_RegisterMusic_MP3(music, music->data, W_LumpLength(music->lumpnum));
        
        iphoneStartMP3(mp3filename);

    } else {

  // play it
        I_PlaySong(music->handle, looping);
    }
    
    
//#endif

  mus_playing = music;
}


void S_StopMusic(void)
{
  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;

  if (mus_playing)
    {
      if (mus_paused)
        I_ResumeSong(mus_playing->handle);

      I_StopSong(mus_playing->handle);
      I_UnRegisterSong(mus_playing->handle);
      if (mus_playing->lumpnum >= 0)
  W_UnlockLumpNum(mus_playing->lumpnum); // cph - release the music data

      mus_playing->data = 0;
      mus_playing = 0;
    }
}



void S_StopChannel(int cnum)
{
  int i;
  channel_t *c = &channels[cnum];

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return;

  if (c->sfxinfo)
    {
      // stop the sound playing
      if (I_SoundIsPlaying(c->handle))
        I_StopSound(c->handle);

      // check to see
      //  if other channels are playing the sound
      for (i=0 ; i<numChannels ; i++)
        if (cnum != i && c->sfxinfo == channels[i].sfxinfo)
          break;

      // degrade usefulness of sound data
      c->sfxinfo->usefulness--;
      c->sfxinfo = 0;
    }
}

//
// Changes volume, stereo-separation, and pitch variables
//  from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//

int S_AdjustSoundParams(mobj_t *listener, mobj_t *source,
                        int *vol, int *sep, int *pitch)
{
  fixed_t adx, ady,approx_dist;
  angle_t angle;

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return 0;

  // e6y
  // Fix crash when the program wants to S_AdjustSoundParams() for player
  // which is not displayplayer and displayplayer was not spawned at the moment.
  // It happens in multiplayer demos only.
  //
  // Stack trace is:
  // P_SetupLevel() \ P_LoadThings() \ P_SpawnMapThing() \ P_SpawnPlayer(players[0]) \
  // P_SetupPsprites() \ P_BringUpWeapon() \ S_StartSound(players[0]->mo, sfx_sawup) \
  // S_StartSoundAtVolume() \ S_AdjustSoundParams(players[displayplayer]->mo, ...);
  // players[displayplayer]->mo is NULL
  //
  // There is no more crash on e1cmnet3.lmp between e1m2 and e1m3
  // http://competn.doom2.net/pub/compet-n/doom/coop/movies/e1cmnet3.zip
  if (!listener)
    return 0;

  // calculate the distance to sound origin
  //  and clip it if necessary
  adx = D_abs(listener->x - source->x);
  ady = D_abs(listener->y - source->y);

  // From _GG1_ p.428. Appox. eucledian distance fast.
  approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

  if (!approx_dist)  // killough 11/98: handle zero-distance as special case
    {
      *sep = NORM_SEP;
      *vol = snd_SfxVolume * 8;	// JDC: plasma fire sounds were 1/8 volume without this
      return *vol > 0;
    }

  if (approx_dist > S_CLIPPING_DIST)
    return 0;

  // angle of source to listener
  angle = R_PointToAngle2(listener->x, listener->y, source->x, source->y);

  if (angle <= listener->angle)
    angle += 0xffffffff;
  angle -= listener->angle;
  angle >>= ANGLETOFINESHIFT;

  // stereo separation
  *sep = 128 - (FixedMul(S_STEREO_SWING,finesine[angle])>>FRACBITS);

  // volume calculation
  if (approx_dist < S_CLOSE_DIST)
    *vol = snd_SfxVolume*8;
  else
    // distance effect
    *vol = (snd_SfxVolume * ((S_CLIPPING_DIST-approx_dist)>>FRACBITS) * 8)
      / S_ATTENUATOR;

  return (*vol > 0);
}

//
// S_getChannel :
//   If none available, return -1.  Otherwise channel #.
//
// killough 4/25/98: made static, added is_pickup argument
#ifndef IPHONE
static int S_getChannel(void *origin, sfxinfo_t *sfxinfo, int is_pickup)
{
  // channel number to use
  int cnum;
  channel_t *c;

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return -1;

  // Find an open channel
  for (cnum=0; cnum<numChannels && channels[cnum].sfxinfo; cnum++)
    if (origin && channels[cnum].origin == origin &&
        channels[cnum].is_pickup == is_pickup)
      {
        S_StopChannel(cnum);
        break;
      }

    // None available
  if (cnum == numChannels)
    {      // Look for lower priority
      for (cnum=0 ; cnum<numChannels ; cnum++)
        if (channels[cnum].sfxinfo->priority >= sfxinfo->priority)
          break;
      if (cnum == numChannels)
        return -1;                  // No lower priority.  Sorry, Charlie.
      else
        S_StopChannel(cnum);        // Otherwise, kick out lower priority.
    }

  c = &channels[cnum];              // channel is decided to be cnum.
  c->sfxinfo = sfxinfo;
  c->origin = origin;
  c->is_pickup = is_pickup;         // killough 4/25/98
  return cnum;
}
#endif
