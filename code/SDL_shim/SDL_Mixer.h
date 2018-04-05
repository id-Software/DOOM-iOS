/*
 
 Copyright (C) 2011 Id Software, Inc.
 
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

#ifndef doomengine_SDL_Mixer_h
#define doomengine_SDL_Mixer_h

#ifdef __cplusplus
extern "C" {
#endif

/*
===============================

This is a minimal implemenation of SDL_Mixer for iOS, just to get MIDI files
playing so that we can play the music directly from the WADs.

===============================
*/

#include <stdint.h>

typedef struct Mix_Music_tag {
	char unused;
} Mix_Music;


/* Open the mixer with a certain audio format */
extern int Mix_OpenAudio(int frequency, uint16_t format, int channels,
				  int chunksize);


/* Close the mixer, halting all playing audio */
extern void Mix_CloseAudio(void);


/* Set a function that is called after all mixing is performed.
   This can be used to provide real-time visual display of the audio stream
   or add a custom mixer filter for the stream data.
*/
extern void Mix_SetPostMix(void (*mix_func)
						   (void *udata, uint8_t *stream, int len), void *arg);


/* Fade in music or a channel over "ms" milliseconds, same semantics as the "Play" functions */
extern int Mix_FadeInMusic(Mix_Music *music, int loops, int ms);


/* Pause/Resume the music stream */
extern void Mix_PauseMusic(void);
extern void Mix_ResumeMusic(void);


/* Halt a channel, fading it out progressively till it's silent
   The ms parameter indicates the number of milliseconds the fading
   will take.
 */
extern int Mix_FadeOutMusic(int ms);


/* Free an audio chunk previously loaded */
extern void Mix_FreeMusic(Mix_Music *music);


/* Load a wave file or a music (.mod .s3m .it .xm) file */
extern Mix_Music * Mix_LoadMUS(const char *file);


extern const char * Mix_GetError(void);




/* Set the volume in the range of 0-128 of a specific channel or chunk.
   If the specified channel is -1, set volume for all channels.
   Returns the original volume.
   If the specified volume is -1, just return the current volume.
*/
extern int Mix_VolumeMusic(int volume);

#ifdef __cplusplus
}
#endif


#endif
