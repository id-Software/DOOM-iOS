/*
 *  BackgroundMusic.cpp
 *  doom
 *
 *  Created by John Carmack on 5/15/09.
 *  Copyright 2009 Id Software. All rights reserved.
 *
 */
/*
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

// TODO: Figure out why I had to add a "2" to the end of a bunch of stuff here


//==================================================================================================
//	Includes
//==================================================================================================

//	System Includes
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CFURL.h>
#include <map>
#include <vector>
#include <pthread.h>
#include <mach/mach.h>
#include <string>

extern "C" {
#include "doomiphone.h"
}

enum {
	kSoundEngineErrUnitialized			= 1,
	kSoundEngineErrInvalidID			= 2,
	kSoundEngineErrFileNotFound			= 3,
	kSoundEngineErrInvalidFileFormat	= 4,
	kSoundEngineErrDeviceNotFound		= 5
};


#define	AssertNoError(inMessage, inHandler)						\
if(result != noErr)									\
{													\
printf("%s: %d\n", inMessage, (int)result);		\
goto inHandler;									\
}


#define kNumberBuffers 3


static Float32				gMasterVolumeGain = 0.5f;


//==================================================================================================
//	Helper functions
//==================================================================================================

OSStatus LoadFileDataInfo2(const char *inFilePath, AudioFileID &outAFID, AudioStreamBasicDescription &outFormat, UInt64 &outDataSize)
{
	UInt32 thePropSize;
	
	CFURLRef theURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)inFilePath, strlen(inFilePath), false);
	if (theURL == NULL)
		return kSoundEngineErrFileNotFound;	

	OSStatus result = AudioFileOpenURL(theURL, kAudioFileReadPermission, 0, &outAFID);
	CFRelease(theURL);
	AssertNoError("Error opening file", end);
	
	thePropSize = sizeof(outFormat);
	result = AudioFileGetProperty(outAFID, kAudioFilePropertyDataFormat, &thePropSize, &outFormat);
	AssertNoError("Error getting file format", end);
	
	thePropSize = sizeof(UInt64);
	result = AudioFileGetProperty(outAFID, kAudioFilePropertyAudioDataByteCount, &thePropSize, &outDataSize);
	AssertNoError("Error getting file data size", end);
	
end:
	return result;
}

void CalculateBytesForTime2 (AudioStreamBasicDescription & inDesc, UInt32 inMaxPacketSize, Float64 inSeconds, UInt32 *outBufferSize, UInt32 *outNumPackets)
{
	static const UInt32 maxBufferSize = 0x10000; // limit size to 64K
	static const UInt32 minBufferSize = 0x4000; // limit size to 16K
	
	if (inDesc.mFramesPerPacket) {
		Float64 numPacketsForTime = inDesc.mSampleRate / inDesc.mFramesPerPacket * inSeconds;
		*outBufferSize = (UInt32)(numPacketsForTime * inMaxPacketSize);
	} else {
		// if frames per packet is zero, then the codec has no predictable packet == time
		// so we can't tailor this (we don't know how many Packets represent a time period
		// we'll just return a default buffer size
		*outBufferSize = maxBufferSize > inMaxPacketSize ? maxBufferSize : inMaxPacketSize;
	}
	
	// we're going to limit our size to our default
	if (*outBufferSize > maxBufferSize && *outBufferSize > inMaxPacketSize)
		*outBufferSize = maxBufferSize;
	else {
		// also make sure we're not too small - we don't want to go the disk for too small chunks
		if (*outBufferSize < minBufferSize)
			*outBufferSize = minBufferSize;
	}
	*outNumPackets = *outBufferSize / inMaxPacketSize;
}

static Boolean MatchFormatFlags(const AudioStreamBasicDescription& x, const AudioStreamBasicDescription& y)
{
	UInt32 xFlags = x.mFormatFlags;
	UInt32 yFlags = y.mFormatFlags;
	
	// match wildcards
	if (x.mFormatID == 0 || y.mFormatID == 0 || xFlags == 0 || yFlags == 0) 
		return true;
	
	if (x.mFormatID == kAudioFormatLinearPCM)
	{		 		
		// knock off the all clear flag
		xFlags = xFlags & ~kAudioFormatFlagsAreAllClear;
		yFlags = yFlags & ~kAudioFormatFlagsAreAllClear;
		
		// if both kAudioFormatFlagIsPacked bits are set, then we don't care about the kAudioFormatFlagIsAlignedHigh bit.
		if (xFlags & yFlags & kAudioFormatFlagIsPacked) {
			xFlags = xFlags & ~kAudioFormatFlagIsAlignedHigh;
			yFlags = yFlags & ~kAudioFormatFlagIsAlignedHigh;
		}
		
		// if both kAudioFormatFlagIsFloat bits are set, then we don't care about the kAudioFormatFlagIsSignedInteger bit.
		if (xFlags & yFlags & kAudioFormatFlagIsFloat) {
			xFlags = xFlags & ~kAudioFormatFlagIsSignedInteger;
			yFlags = yFlags & ~kAudioFormatFlagIsSignedInteger;
		}
		
		//	if the bit depth is 8 bits or less and the format is packed, we don't care about endianness
		if((x.mBitsPerChannel <= 8) && ((xFlags & kAudioFormatFlagIsPacked) == kAudioFormatFlagIsPacked))
		{
			xFlags = xFlags & ~kAudioFormatFlagIsBigEndian;
		}
		if((y.mBitsPerChannel <= 8) && ((yFlags & kAudioFormatFlagIsPacked) == kAudioFormatFlagIsPacked))
		{
			yFlags = yFlags & ~kAudioFormatFlagIsBigEndian;
		}
		
		//	if the number of channels is 0 or 1, we don't care about non-interleavedness
		if (x.mChannelsPerFrame <= 1 && y.mChannelsPerFrame <= 1) {
			xFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
			yFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
		}
	}
	return xFlags == yFlags;
}

Boolean FormatIsEqual2(AudioStreamBasicDescription x, AudioStreamBasicDescription y)
{
	//	the semantics for equality are:
	//		1) Values must match exactly
	//		2) wildcard's are ignored in the comparison
	
#define MATCH(name) ((x.name) == 0 || (y.name) == 0 || (x.name) == (y.name))
	
	return 
	((x.mSampleRate==0.) || (y.mSampleRate==0.) || (x.mSampleRate==y.mSampleRate)) 
	&& MATCH(mFormatID)
	&& MatchFormatFlags(x, y)  
	&& MATCH(mBytesPerPacket) 
	&& MATCH(mFramesPerPacket) 
	&& MATCH(mBytesPerFrame) 
	&& MATCH(mChannelsPerFrame) 		
	&& MATCH(mBitsPerChannel) ;
}

#pragma mark ***** BackgroundTrackMgr2 *****
//==================================================================================================
//	BackgroundTrackMgr2 class
//==================================================================================================
typedef struct BG_FileInfo {
	std::string						mFilePath;
	AudioFileID						mAFID;
	AudioStreamBasicDescription		mFileFormat;
	UInt64							mFileDataSize;
	//UInt64							mFileNumPackets; // this is only used if loading file to memory
	Boolean							mLoadAtOnce;
	Boolean							mFileDataInQueue;
} BackgroundMusicFileInfo;

class BackgroundTrackMgr2
	{	
	public:
		BackgroundTrackMgr2();
		~BackgroundTrackMgr2();
		
		void Teardown();
				
		static void QueueCallback( void * inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inCompleteAQBuffer);
		
		OSStatus SetupQueue(BG_FileInfo *inFileInfo);
		OSStatus SetupBuffers(BG_FileInfo *inFileInfo);
		OSStatus LoadTrack(const char* inFilePath, Boolean inAddToQueue, Boolean inLoadAtOnce);
		
		OSStatus SetVolume(Float32 inVolume);
		Float32 GetVolume() const;
		
		OSStatus Start();
		OSStatus Stop(Boolean inStopAtEnd);
		
		AudioQueueRef						mQueue;
		AudioQueueBufferRef					mBuffers[kNumberBuffers];
		UInt32								mBufferByteSize;
		SInt64								mCurrentPacket;
		UInt32								mNumPacketsToRead;
		Float32								mVolume;
		AudioStreamPacketDescription *		mPacketDescs;
		static BG_FileInfo *				CurFileInfo;
		Boolean								mStopAtEnd;
	};

BG_FileInfo *BackgroundTrackMgr2::CurFileInfo;


BackgroundTrackMgr2::BackgroundTrackMgr2()
:	mQueue(0),
mBufferByteSize(0),
mCurrentPacket(0),
mNumPacketsToRead(0),
mVolume(1.0f),
mPacketDescs(NULL),
mStopAtEnd(false)
{ }

BackgroundTrackMgr2::~BackgroundTrackMgr2() {
	Teardown();
}

void BackgroundTrackMgr2::Teardown() {
	if (mQueue) {
		AudioQueueDispose(mQueue, true);
		mQueue = NULL;
	}
	if ( CurFileInfo ) {
		AudioFileClose( CurFileInfo->mAFID);
		delete CurFileInfo;
		CurFileInfo = NULL;
	}
	if (mPacketDescs) {
		delete[] mPacketDescs;
		mPacketDescs = NULL;
	}
}


void BackgroundTrackMgr2::QueueCallback( void * inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inCompleteAQBuffer ) {
	// dispose of the buffer if no longer in use
	OSStatus result = noErr;
	BackgroundTrackMgr2 *THIS = (BackgroundTrackMgr2*)inUserData;
	UInt32 nPackets = 0;
	// loop the current buffer if the following:
	// 1. file was loaded into the buffer previously
	// 2. only one file in the queue
	// 3. we have not been told to stop at playlist completion
	if ((CurFileInfo->mFileDataInQueue) && (!THIS->mStopAtEnd)) {
		nPackets = THIS->mNumPacketsToRead;
	} else {
		UInt32 numBytes;
		while (nPackets == 0) {
			// if loadAtOnce, get all packets in the file, otherwise ~.5 seconds of data
			nPackets = THIS->mNumPacketsToRead;					
			result = AudioFileReadPacketData(CurFileInfo->mAFID, false, &numBytes, THIS->mPacketDescs, THIS->mCurrentPacket, &nPackets, inCompleteAQBuffer->mAudioData);
			AssertNoError("Error reading file data", end);
			
			inCompleteAQBuffer->mAudioDataByteSize = numBytes;	
			
			if (nPackets == 0) { // no packets were read, this file has ended.
				if (CurFileInfo->mLoadAtOnce) {
					CurFileInfo->mFileDataInQueue = true;
				}
				
				THIS->mCurrentPacket = 0;
				
				// we have gone through the playlist. if mStopAtEnd, stop the queue here
				if ( THIS->mStopAtEnd ) {
					result = AudioQueueStop(inAQ, false);
					AssertNoError("Error stopping queue", end);
					return;
				}
			}
		}
	}
	
	result = AudioQueueEnqueueBuffer(inAQ, inCompleteAQBuffer, (THIS->mPacketDescs ? nPackets : 0), THIS->mPacketDescs);
    if(result != noErr) {
        result = AudioQueueFreeBuffer(inAQ, inCompleteAQBuffer);
		AssertNoError("Error freeing buffers that didn't enqueue", end);
    }
	AssertNoError("Error enqueuing new buffer", end);
	if (CurFileInfo->mLoadAtOnce) {
		CurFileInfo->mFileDataInQueue = true;
	}
	
	THIS->mCurrentPacket += nPackets;
	
end:
	return;
}

OSStatus BackgroundTrackMgr2::SetupQueue(BG_FileInfo *inFileInfo) {
	UInt32 size = 0;
	OSStatus err;
	OSStatus result = AudioQueueNewOutput(&inFileInfo->mFileFormat, QueueCallback, this, 
										  CFRunLoopGetMain() /* CFRunLoopGetCurrent() */, kCFRunLoopCommonModes, 0, &mQueue);
	AssertNoError("Error creating queue", end);
#if 0	
	// (2) If the file has a cookie, we should get it and set it on the AQ
	size = sizeof(UInt32);
	result = AudioFileGetPropertyInfo (inFileInfo->mAFID, kAudioFilePropertyMagicCookieData, &size, NULL);
	
	if (!result && size) {
		char* cookie = new char [size];		
		result = AudioFileGetProperty (inFileInfo->mAFID, kAudioFilePropertyMagicCookieData, &size, cookie);
		AssertNoError("Error getting magic cookie", end);
		result = AudioQueueSetProperty(mQueue, kAudioQueueProperty_MagicCookie, cookie, size);
		delete [] cookie;
		AssertNoError("Error setting magic cookie", end);
	}
#endif
	// channel layout
	err = AudioFileGetPropertyInfo(inFileInfo->mAFID, kAudioFilePropertyChannelLayout, &size, NULL);
	if (err == noErr && size > 0) {
		AudioChannelLayout *acl = (AudioChannelLayout *)malloc(size);
		result = AudioFileGetProperty(inFileInfo->mAFID, kAudioFilePropertyChannelLayout, &size, acl);
		AssertNoError("Error getting channel layout from file", end);
		result = AudioQueueSetProperty(mQueue, kAudioQueueProperty_ChannelLayout, acl, size);
		free(acl);
		AssertNoError("Error setting channel layout on queue", end);
	}
	
	// volume
	result = SetVolume(mVolume);
	
end:
	return result;
}

OSStatus BackgroundTrackMgr2::SetupBuffers(BG_FileInfo *inFileInfo) {
	int numBuffersToQueue = kNumberBuffers;
	UInt32 maxPacketSize;
	UInt32 size = sizeof(maxPacketSize);
	bool isFormatVBR;
	// we need to calculate how many packets we read at a time, and how big a buffer we need
	// we base this on the size of the packets in the file and an approximate duration for each buffer
	
	// first check to see what the max size of a packet is - if it is bigger
	// than our allocation default size, that needs to become larger
	OSStatus result = AudioFileGetProperty(inFileInfo->mAFID, kAudioFilePropertyPacketSizeUpperBound, &size, &maxPacketSize);
	AssertNoError("Error getting packet upper bound size", end);
	isFormatVBR = (inFileInfo->mFileFormat.mBytesPerPacket == 0 || inFileInfo->mFileFormat.mFramesPerPacket == 0);
	
	CalculateBytesForTime2(inFileInfo->mFileFormat, maxPacketSize, 0.5/*seconds*/, &mBufferByteSize, &mNumPacketsToRead);
	
	// if the file is smaller than the capacity of all the buffer queues, always load it at once
	if ((mBufferByteSize * numBuffersToQueue) > inFileInfo->mFileDataSize) {
		inFileInfo->mLoadAtOnce = true;
	}
	
	if (inFileInfo->mLoadAtOnce) {
		UInt64 theFileNumPackets;
		size = sizeof(UInt64);
		result = AudioFileGetProperty(inFileInfo->mAFID, kAudioFilePropertyAudioDataPacketCount, &size, &theFileNumPackets);
		AssertNoError("Error getting packet count for file", end);
		
		mNumPacketsToRead = (UInt32)theFileNumPackets;
		mBufferByteSize = (UInt32)inFileInfo->mFileDataSize;
		numBuffersToQueue = 1;
	} else {
		mNumPacketsToRead = mBufferByteSize / maxPacketSize;
	}
	
	if (isFormatVBR) {
		mPacketDescs = new AudioStreamPacketDescription [mNumPacketsToRead];
	} else {
		mPacketDescs = NULL; // we don't provide packet descriptions for constant bit rate formats (like linear PCM)	
	}
	
	// allocate the queue's buffers
	for (int i = 0; i < numBuffersToQueue; ++i) {
		result = AudioQueueAllocateBuffer(mQueue, mBufferByteSize, &mBuffers[i]);
		AssertNoError("Error allocating buffer for queue", end);
		QueueCallback (this, mQueue, mBuffers[i]);
		if (inFileInfo->mLoadAtOnce) {
			inFileInfo->mFileDataInQueue = true;
		}
	}
	
end:
	return result;
}

OSStatus BackgroundTrackMgr2::LoadTrack(const char* inFilePath, Boolean inAddToQueue, Boolean inLoadAtOnce) {
//	OSStatus result = LoadFileDataInfo(CurFileInfo->mFilePath.c_str(), CurFileInfo->mAFID, CurFileInfo->mFileFormat, CurFileInfo->mFileDataSize);
//	AssertNoError("Error getting file data info", fail);
	OSStatus result;
	UInt32 thePropSize;
	
	CFURLRef theURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)inFilePath, strlen(inFilePath), false);
	if (theURL == NULL)
		result = kSoundEngineErrFileNotFound;
	else
		result = 0;
	AssertNoError("Error opening URL", fail);
	
	CurFileInfo = new BG_FileInfo;
	CurFileInfo->mFilePath = inFilePath;
	
	result = AudioFileOpenURL(theURL, kAudioFileReadPermission, 0, &CurFileInfo->mAFID);
	CFRelease(theURL);
	AssertNoError("Error opening file", fail);
	
	thePropSize = sizeof(CurFileInfo->mFileFormat);
	result = AudioFileGetProperty(CurFileInfo->mAFID, kAudioFilePropertyDataFormat, &thePropSize, &CurFileInfo->mFileFormat);
	AssertNoError("Error getting file format", fail);
	
	thePropSize = sizeof(UInt64);
	result = AudioFileGetProperty(CurFileInfo->mAFID, kAudioFilePropertyAudioDataByteCount, &thePropSize, &CurFileInfo->mFileDataSize);
	AssertNoError("Error getting file data size", fail);
	
	CurFileInfo->mLoadAtOnce = inLoadAtOnce;
	CurFileInfo->mFileDataInQueue = false;
	
	result = SetupQueue(CurFileInfo);
	AssertNoError("Error setting up queue", fail);
	
	result = SetupBuffers(CurFileInfo);
	AssertNoError("Error setting up queue buffers", fail);					
	
	return result;
	
fail:
	if (CurFileInfo) {
		delete CurFileInfo;
		CurFileInfo = NULL;
	}
	return result;
}

OSStatus BackgroundTrackMgr2::SetVolume(Float32 inVolume) {
	mVolume = inVolume;
	return AudioQueueSetParameter(mQueue, kAudioQueueParam_Volume, mVolume * gMasterVolumeGain);
}

Float32 BackgroundTrackMgr2::GetVolume() const {
	return mVolume;
}

OSStatus BackgroundTrackMgr2::Start() {
	OSStatus result = AudioQueuePrime(mQueue, 1, NULL);	
	if (result)	{
		printf("BackgroundTrackMgr2: Error priming queue: %d\n", (int)result);
		return result;
	}
	return AudioQueueStart(mQueue, NULL);
}

OSStatus BackgroundTrackMgr2::Stop(Boolean inStopAtEnd) {
	if (inStopAtEnd) {
		mStopAtEnd = true;
		return noErr;
	} else {
		return AudioQueueStop(mQueue, true);
	}
}


static BackgroundTrackMgr2	sBackgroundTrackMgr;

static char currentMusicName[1024];

void iphonePauseMusic() {
    if( music ) {
        if ( music->value == 0 ) {
            // music is disabled
            return;
        }
        AudioQueuePause(sBackgroundTrackMgr.mQueue);
    }
}
void iphoneResumeMusic() {
	if ( music->value == 0 ) {
		// music is disabled
		return;
	}
	AudioQueueStart(sBackgroundTrackMgr.mQueue,NULL);
}
void iphoneStopMusic() {
	sBackgroundTrackMgr.Teardown();
}

void iphoneStartMusic() {
	if ( music->value == 0 ) {
		// music is disabled
		return;
	}
	char	fullName[1024];
	sprintf( fullName, "%s/base/music/d_%s.mp3", SysIphoneGetAppDir(), currentMusicName );
	
	printf( "Starting music '%s'\n", fullName );

	iphoneStopMusic();
	sBackgroundTrackMgr.LoadTrack( fullName, false, true);
	sBackgroundTrackMgr.Start();
	
	if ( !strcmp( currentMusicName, "intro" ) ) {
		// stop the intro music at end, don't loop
		sBackgroundTrackMgr.mStopAtEnd = true;
	} else {
		sBackgroundTrackMgr.mStopAtEnd = false;
	}
}

void iphonePlayMusic( const char *name ) {
	strcpy( currentMusicName, name );
	
	iphoneStartMusic();
}

boolean    sigilShredsLoaded;    // Buckethead!

void iphoneStartMP3(const char *filename) {
    if ( music->value == 0 ) {
        // music is disabled
        return;
    }
    
    // necessary? -tkidd
    char    fullName[1024];
    sprintf( fullName, "%s", filename );
    
    printf( "Starting music '%s'\n", fullName );
    
    iphoneStopMusic();
    sBackgroundTrackMgr.LoadTrack( fullName, false, true);
    sBackgroundTrackMgr.Start();
    
    if ( !strcmp( currentMusicName, "intro" ) ) {
        // stop the intro music at end, don't loop
        sBackgroundTrackMgr.mStopAtEnd = true;
    } else {
        sBackgroundTrackMgr.mStopAtEnd = false;
    }
}


