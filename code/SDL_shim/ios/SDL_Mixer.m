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
 
 
/*
===============================

iOS implementation of our SDL_Mixer shim for playing MIDI files.

===============================
*/

#include <stddef.h>

#include "../SDL_Mixer.h"


// Use the Embedded Audio Synthesis library as the backend MIDI renderer.
#include "../../embeddedaudiosynthesis/EASGlue.h"


// Use Core Audio Units for sound output on the device.
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>


/*
===============================

"Constants"

===============================
*/
#define ID_GRAPH_SAMPLE_RATE 22050


/*
===============================

Internal Structures

===============================
*/

// Data structure for mono or stereo sound, to pass to the application's render callback function, 
//    which gets invoked by a Mixer unit input bus when it needs more audio to play.
typedef struct {

    BOOL                 isStereo;           // set to true if there is data in the audioDataRight member
    UInt32               frameCount;         // the total number of frames in the audio data
    UInt32               sampleNumber;       // the next audio sample to play
} soundStruct, *soundStructPtr;



typedef struct MIDIPlayerGraph_tag {

	AUGraph		processingGraph;
	AudioUnit	ioUnit;
	BOOL		playing;
	
	AudioStreamBasicDescription streamFormat;
	
	soundStruct	soundStructInst;
} MIDIPlayerGraph;

static MIDIPlayerGraph midiPlayer;



/*
===============================

Internal prototypes

===============================
*/
AudioStreamBasicDescription getStreamFormat( void );
static void printASBD( AudioStreamBasicDescription asbd );
static void printErrorMessage( NSString * errorString, OSStatus result );
static void configureAndInitializeAudioProcessingGraph( MIDIPlayerGraph * player );
static void startMIDIPlayer( MIDIPlayerGraph * player );

// AU graph callback.
static OSStatus inputRenderCallback (

    void                        *inRefCon,      // A pointer to a struct containing the complete audio data 
                                                //    to play, as well as state information such as the  
                                                //    first sample to play on this invocation of the callback.
    AudioUnitRenderActionFlags  *ioActionFlags, // Unused here. When generating audio, use ioActionFlags to indicate silence 
                                                //    between sounds; for silence, also memset the ioData buffers to 0.
    const AudioTimeStamp        *inTimeStamp,   // Unused here.
    UInt32                      inBusNumber,    // The mixer unit input bus that is requesting some new
                                                //        frames of audio data to play.
    UInt32                      inNumberFrames, // The number of frames of audio to provide to the buffer(s)
                                                //        pointed to by the ioData parameter.
    AudioBufferList             *ioData         // On output, the audio data to play. The callback's primary 
                                                //        responsibility is to fill the buffer(s) in the 
                                                //        AudioBufferList.
);



/* Open the mixer with a certain audio format */
int Mix_OpenAudio(int frequency, uint16_t format, int channels,
				  int chunksize) {
	
	EASGlueInit();
	
	midiPlayer.streamFormat = getStreamFormat();
	midiPlayer.playing = FALSE;
	
	configureAndInitializeAudioProcessingGraph( &midiPlayer );
	
	return 0;
}


/* Close the mixer, halting all playing audio */
void Mix_CloseAudio(void) {
	AUGraphStop( midiPlayer.processingGraph );
	EASGlueShutdown();
}


/* Set a function that is called after all mixing is performed.
   This can be used to provide real-time visual display of the audio stream
   or add a custom mixer filter for the stream data.
*/
void Mix_SetPostMix(void (*mix_func)
					(void *udata, uint8_t *stream, int len), void *arg) {

}


/* Fade in music or a channel over "ms" milliseconds, same semantics as the "Play" functions */
int Mix_FadeInMusic(Mix_Music *music, int loops, int ms) {
	
	
	startMIDIPlayer( &midiPlayer );
	
	
	return 0;
}


/* Pause/Resume the music stream */
void Mix_PauseMusic(void) {
	EASGluePause();
}


void Mix_ResumeMusic(void) {
	EASGlueResume();
}


/* Halt a channel, fading it out progressively till it's silent
   The ms parameter indicates the number of milliseconds the fading
   will take.
 */
int Mix_FadeOutMusic(int ms) {

	EASGlueCloseFile();
	
	AUGraphStop( midiPlayer.processingGraph );

	return 1;
}


/* Free an audio chunk previously loaded */
void Mix_FreeMusic(Mix_Music *music) {
	free(music);
}


/* Load a wave file or a music (.mod .s3m .it .xm) file */
Mix_Music * Mix_LoadMUS(const char *file) {
	
	EASGlueOpenFile( file );
	
	Mix_Music * musicStruct = malloc( sizeof(Mix_Music) );
	
	return musicStruct;
}


const char * Mix_GetError(void) {
	return "";
}


/* Set the volume in the range of 0-128 of a specific channel or chunk.
   If the specified channel is -1, set volume for all channels.
   Returns the original volume.
   If the specified volume is -1, just return the current volume.
*/
int Mix_VolumeMusic(int volume) {

	return 0;

}






/*
=================================
 Audio Unit helper functions
=================================
*/


AudioStreamBasicDescription getStreamFormat( void ) {

	AudioStreamBasicDescription streamFormat = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    
    // The AudioUnitSampleType data type is the recommended type for sample data in audio
    //    units. This obtains the byte size of the type for use in filling in the ASBD.
    size_t bytesPerSample = sizeof (SInt32);

    // Fill the application audio format struct's fields to define a linear PCM, 
    //        stereo, noninterleaved stream at the hardware sample rate.
    streamFormat.mFormatID          = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags       = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved | (kAudioUnitSampleFractionBits << kLinearPCMFormatFlagsSampleFractionShift);
    streamFormat.mBytesPerPacket    = (UInt32) bytesPerSample;
    streamFormat.mFramesPerPacket   = 1;
    streamFormat.mBytesPerFrame     = (UInt32) bytesPerSample;
    streamFormat.mChannelsPerFrame  = 2;                    // 2 indicates stereo
    streamFormat.mBitsPerChannel    = (UInt32) (8 * bytesPerSample);
    streamFormat.mSampleRate        = ID_GRAPH_SAMPLE_RATE;


    NSLog (@"The stereo stream format for the I/O unit:");
    printASBD( streamFormat );
	
	return streamFormat;
}

static void startMIDIPlayer( MIDIPlayerGraph * player ) {

	if ( player == 0 ) {
		NSLog( @"NULL player object, can't start!" );
		return;
	}
	
	NSLog (@"Starting audio processing graph");
    OSStatus result = AUGraphStart (player->processingGraph);
    if (noErr != result) { printErrorMessage( @"AUGraphStart", result ); return;}

    player->playing = YES;

}

// You can use this method during development and debugging to look at the
//    fields of an AudioStreamBasicDescription struct.
static void printASBD( AudioStreamBasicDescription asbd ) {

    char formatIDString[5];
    UInt32 formatID = CFSwapInt32HostToBig (asbd.mFormatID);
    bcopy (&formatID, formatIDString, 4);
    formatIDString[4] = '\0';
    
    NSLog (@"  Sample Rate:         %10.0f",  asbd.mSampleRate);
    NSLog (@"  Format ID:           %10s",    formatIDString);
    NSLog (@"  Format Flags:        %10X",    (unsigned int)asbd.mFormatFlags);
    NSLog (@"  Bytes per Packet:    %10u",    (unsigned int)asbd.mBytesPerPacket);
    NSLog (@"  Frames per Packet:   %10u",    (unsigned int)asbd.mFramesPerPacket);
    NSLog (@"  Bytes per Frame:     %10u",    (unsigned int)asbd.mBytesPerFrame);
    NSLog (@"  Channels per Frame:  %10u",    (unsigned int)asbd.mChannelsPerFrame);
    NSLog (@"  Bits per Channel:    %10u",    (unsigned int)asbd.mBitsPerChannel);
}


static void printErrorMessage( NSString * errorString, OSStatus result ) {

    char resultString[5];
    UInt32 swappedResult = CFSwapInt32HostToBig (result);
    bcopy (&swappedResult, resultString, 4);
    resultString[4] = '\0';

    NSLog (
        @"*** %@ error: %s\n",
                errorString,
                (char*) &resultString
    );
}


// This method performs all the work needed to set up the audio processing graph:

    // 1. Instantiate and open an audio processing graph
    // 2. Obtain the audio unit nodes for the graph
    // 3. Configure the Multichannel Mixer unit
    //     * specify the number of input buses
    //     * specify the output sample rate
    //     * specify the maximum frames-per-slice
    // 4. Initialize the audio processing graph

static void configureAndInitializeAudioProcessingGraph( MIDIPlayerGraph * player ) {

	if ( player == 0 ) {
		NSLog( @"NULL player graph object, can't initialize it!" );
		return;
	}
	

    NSLog (@"Configuring and then initializing audio processing graph");
    OSStatus result = noErr;

//............................................................................
// Create a new audio processing graph.
    result = NewAUGraph (&player->processingGraph);

    if (noErr != result) { printErrorMessage( @"NewAUGraph", result ); return;}
    
    
//............................................................................
// Specify the audio unit component descriptions for the audio units to be
//    added to the graph.

    // I/O unit
    AudioComponentDescription iOUnitDescription;
    iOUnitDescription.componentType          = kAudioUnitType_Output;
    iOUnitDescription.componentSubType       = kAudioUnitSubType_RemoteIO;
    iOUnitDescription.componentManufacturer  = kAudioUnitManufacturer_Apple;
    iOUnitDescription.componentFlags         = 0;
    iOUnitDescription.componentFlagsMask     = 0;
    

//............................................................................
// Add nodes to the audio processing graph.
    NSLog (@"Adding nodes to audio processing graph");

    AUNode   iONode;         // node for I/O unit
    
    // Add the nodes to the audio processing graph
    result =    AUGraphAddNode (
                    player->processingGraph,
                    &iOUnitDescription,
                    &iONode);
    
    if (noErr != result) { printErrorMessage( @"AUGraphNewNode failed for I/O unit", result ); return;}    

//............................................................................
// Open the audio processing graph

    // Following this call, the audio units are instantiated but not initialized
    //    (no resource allocation occurs and the audio units are not in a state to
    //    process audio).
    result = AUGraphOpen (player->processingGraph);
    
    if (noErr != result) { printErrorMessage( @"AUGraphOpen", result ); return;}
    
    
//............................................................................
// Obtain the mixer unit instance from its corresponding node.

    result =    AUGraphNodeInfo (
                    player->processingGraph,
					iONode,
                    NULL,
                    &player->ioUnit
                );
    
    if (noErr != result) { printErrorMessage( @"AUGraphNodeInfo", result ); return;}
    

//............................................................................
// Multichannel Mixer unit Setup


	// Setup the struture that contains the input render callback 
	AURenderCallbackStruct inputCallbackStruct;
	inputCallbackStruct.inputProc        = &inputRenderCallback;
	inputCallbackStruct.inputProcRefCon  = &player->soundStructInst;
	
	NSLog (@"Registering the render callback with the I/O unit" );
	// Set a callback for the specified node's specified input
	result = AUGraphSetNodeInputCallback (
				 player->processingGraph,
				 iONode,
				 0,
				 &inputCallbackStruct
			 );

	if (noErr != result) { printErrorMessage( @"AUGraphSetNodeInputCallback", result ); return;}


    NSLog (@"Setting stereo stream format for I/O unit input bus");
    result = AudioUnitSetProperty (
                 player->ioUnit,
                 kAudioUnitProperty_StreamFormat,
                 kAudioUnitScope_Input,
                 0,
                 &player->streamFormat,
                 sizeof (player->streamFormat)
             );

    if (noErr != result) { printErrorMessage( @"AudioUnitSetProperty (set input bus stream format)", result ); return;}

//............................................................................
// Initialize audio processing graph

    // Diagnostic code
    // Call CAShow if you want to look at the state of the audio processing 
    //    graph.
    NSLog (@"Audio processing graph state immediately before initializing it:");
    CAShow (player->processingGraph);

    NSLog (@"Initializing the audio processing graph");
    // Initialize the audio processing graph, configure audio data stream formats for
    //    each input and output, and validate the connections between audio units.
    result = AUGraphInitialize (player->processingGraph);
    
    if (noErr != result) { printErrorMessage( @"AUGraphInitialize", result ); return;}
}

#define RAW_EAS_BUFFER_FRAMES 128

static OSStatus inputRenderCallback (

    void                        *inRefCon,      // A pointer to a struct containing the complete audio data 
                                                //    to play, as well as state information such as the  
                                                //    first sample to play on this invocation of the callback.
    AudioUnitRenderActionFlags  *ioActionFlags, // Unused here. When generating audio, use ioActionFlags to indicate silence 
                                                //    between sounds; for silence, also memset the ioData buffers to 0.
    const AudioTimeStamp        *inTimeStamp,   // Unused here.
    UInt32                      inBusNumber,    // The mixer unit input bus that is requesting some new
                                                //        frames of audio data to play.
    UInt32                      inNumberFrames, // The number of frames of audio to provide to the buffer(s)
                                                //        pointed to by the ioData parameter.
    AudioBufferList             *ioData         // On output, the audio data to play. The callback's primary 
                                                //        responsibility is to fill the buffer(s) in the 
                                                //        AudioBufferList.
) {

	//printf( "Need %lu samples in %lu buffers!\n", inNumberFrames, ioData->mNumberBuffers );
	
	EAS_I32 generatedThisRender = 0;
	EAS_I32 totalGenerated = 0;
	
	// It looks like EAS interleaves stereo samples, so we have to separate them into the two
	// different buffers that the audio unit provides.
	//const UInt32 totalInterleavedSamplesNeeded = inNumberFrames * 2;
	
	AudioBuffer * audioBufferLeft = &ioData->mBuffers[0];
	AudioBuffer * audioBufferRight = &ioData->mBuffers[1];
	
	/*
	printf( "Need %lu samples in %lu buffers!\n"
			"audioBuffer byte size: %lu channels: %lu\n",
			inNumberFrames, ioData->mNumberBuffers,
			audioBuffer->mDataByteSize, audioBuffer->mNumberChannels );
	*/
	SInt32 * hardwareBufferLeft = (SInt32 *) audioBufferLeft->mData;
	SInt32 * hardwareBufferRight = (SInt32 *) audioBufferRight->mData;
	
	
	// EAS_Render always produces BUFFER_SIZE_IN_MONO_SAMPLES frames per call. Currently, this
	// is defined to 128. Let's fill up a 128 frame buffer, then do a conversion from EAS_PCM
	// (which is signed 16-bit integer) to AudioUnitSampleType (which is 8.24 fixed-point with
	// a range of -1 to +1).
	//
	// Note that EAS renders interleaved stereo, so we actually a buffer size of
	// 2 * BUFFER_SIZE_IN_MONO_SAMPLES.
	
	EAS_PCM rawEASSamples[RAW_EAS_BUFFER_FRAMES * 2];
	
	
	// EAS generates interleaved stereo samples, but the AudioUnit wants noninterleaved.
	while ( totalGenerated < inNumberFrames ) {
		//EASGlueRender( hardwareBuffer + totalGenerated*2, &generatedThisRender );
		EASGlueRender( rawEASSamples, &generatedThisRender );
		
		
		// Convert from EAS's signed 16-bit format to the AudioUnit's 8.24 fixed-point format.
		// Couldn't find this in the Apple docs, but the 8.24 format should be in the range of
		// -1.0 to 1.0, wasting 6 bits of precision.
		// All we have to do here is left-shift by 9 bits. This will not overflow, because the
		// destination is a 32-bit value.
		// Also take this opportunity to de-interleave the EAS-rendered samples.
		for ( int i = 0; i < RAW_EAS_BUFFER_FRAMES; ++i ) {
			hardwareBufferLeft[totalGenerated + i] = rawEASSamples[i * 2 + 0] << 9;
			hardwareBufferRight[totalGenerated + i] = rawEASSamples[i * 2 + 1] << 9;
		}
		
		totalGenerated += generatedThisRender;
	}
	
	return noErr;
}





