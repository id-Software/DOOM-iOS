/*
 *  EASLib.c
 *  EASLIb
 *
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "eas.h"
#include "eas_report.h"
#include "eas_host.h"
#include <AudioUnit/AudioUnit.h>
#include <CoreServices/CoreServices.h>

#ifdef JET_INTERFACE
#include "jet.h"
#endif


#define EAS_EXPORT __attribute__((visibility("default")))

// #define DEBUG_FILE_IO

/* include debug interface */
#include "eas_host_debug.h"

#ifdef AUX_MIXER
#include "eas_auxmix.h"
#endif

/* this module requires dynamic memory support */
#ifdef _STATIC_MEMORY
#error "eas_hostmm.c requires the dynamic memory model!\n"
#endif

#ifndef EAS_MAX_FILE_HANDLES
#define EAS_MAX_FILE_HANDLES    32
#endif

#ifndef EAS_FILE_BUFFER_SIZE
#define EAS_FILE_BUFFER_SIZE    32
#endif

/*
 * this structure and the related function are here
 * to support the ability to create duplicate handles
 * and buffering it in memory. If your system uses
 * in-memory resources, you can eliminate the calls
 * to malloc and free, the dup flag, and simply track
 * the file size and read position.
 */
 #ifdef BUFFERED_FILE_ACCESS
typedef struct eas_hw_file_tag
{
    FILE *pFile;
    EAS_I32 bytesInBuffer;
    EAS_I32 readIndex;
    EAS_I32 filePos;
    EAS_I32 fileSize;
    EAS_BOOL dup;
    EAS_U8 buffer[EAS_FILE_BUFFER_SIZE];
} EAS_HW_FILE;
#else
typedef struct eas_hw_file_tag
{
    EAS_I32         fileSize;
    EAS_I32         filePos;
    EAS_BOOL        dup;
    EAS_U8          *buffer;
} EAS_HW_FILE;
#endif

typedef struct eas_hw_inst_data_tag
{
    EAS_HW_FILE     files[EAS_MAX_FILE_HANDLES];
} EAS_HW_INST_DATA;

EAS_BOOL errorConditions[eNumErrorConditions];
EAS_BOOL ledState;
EAS_BOOL vibState;
EAS_BOOL backlightState;

#define MAX_DEBUG_MSG_LEN 1024

typedef void (*EAS_LOG_FUNC)(EAS_INT severity, char *msg);

static EAS_LOG_FUNC logCallback = NULL;
static char messageBuffer[MAX_DEBUG_MSG_LEN];

/* error counts */
static EAS_INT eas_fatalErrors;
static EAS_INT eas_errors;
static EAS_INT eas_warnings;
static int severityLevel = 9999;


#define MAX_BUFFERS         8

// The output unit
AudioUnit   OutputUnit;
AudioStreamBasicDescription streamFormat;

// sync stuf
pthread_mutex_t mtx;
pthread_cond_t  cond;
bool bStopped = true;

// buffer to hold the data
typedef struct
{
    UInt32          uOutBufferLength;
    UInt32          uOutFrames;
    int             ix;
    short*          pData[8];
    unsigned int    uLength;
} S_BUFFER_INFO;

static S_BUFFER_INFO *pBuf = NULL;
const S_EAS_LIB_CONFIG *pConfig = NULL;

/*----------------------------------------------------------------------------
 * ResetErrorCounters()
 *----------------------------------------------------------------------------
*/
EAS_EXPORT void ResetErrorCounters()
{
    eas_fatalErrors = 0;
    eas_errors = 0;
    eas_warnings = 0;
}

/*----------------------------------------------------------------------------
 * SetLogCallback()
 *----------------------------------------------------------------------------
*/
EAS_EXPORT void SetLogCallback (EAS_LOG_FUNC callback)
{
    logCallback = callback;
}

#ifndef _NO_DEBUG_PREPROCESSOR
static S_DEBUG_MESSAGES debugMessages[] =
{
#ifdef UNIFIED_DEBUG_MESSAGES
#include "eas_debugmsgs.h"
#endif
    { 0,0,0 }
};

/*----------------------------------------------------------------------------
 * EAS_ReportEx()
 *----------------------------------------------------------------------------
*/
void EAS_ReportEx (int severity, unsigned long hashCode, int serialNum, ...)
{
    va_list vargs;
    int i;

    switch (severity)
    {
        case _EAS_SEVERITY_FATAL:
            eas_fatalErrors++;
            break;

        case _EAS_SEVERITY_ERROR:
            eas_errors++;
            break;

        case _EAS_SEVERITY_WARNING:
            eas_warnings++;
            break;

        default:
            break;
    }

    /* check severity level */
    if (severity > severityLevel)
        return;

    /* check for callback */
    if (logCallback == NULL)
        return;

    /* find the error message and output to stdout */
    for (i = 0; debugMessages[i].m_pDebugMsg; i++)
    {
        if ((debugMessages[i].m_nHashCode == hashCode) &&
        (debugMessages[i].m_nSerialNum == serialNum))
        {
            va_start(vargs, serialNum);
#ifdef WIN32
            vsprintf_s(messageBuffer, sizeof(messageBuffer), fmt, vargs);
#else
            vsprintf(messageBuffer, debugMessages[i].m_pDebugMsg, vargs);
#endif
            logCallback(severity, messageBuffer);
            va_end(vargs);
            return;
        }
    }
    printf("Unrecognized error: Severity=%d; HashCode=%lu; SerialNum=%d\n", severity, hashCode, serialNum);
} /* end EAS_ReportEx */

#else
/*----------------------------------------------------------------------------
 * EAS_Report()
 *----------------------------------------------------------------------------
*/
void EAS_Report (int severity, const char *fmt, ...)
{
    va_list vargs;

    switch (severity)
    {
        case _EAS_SEVERITY_FATAL:
            eas_fatalErrors++;
            break;

        case _EAS_SEVERITY_ERROR:
            eas_errors++;
            break;

        case _EAS_SEVERITY_WARNING:
            eas_warnings++;
            break;

        default:
            break;
    }

    /* check severity level */
    if (severity > severityLevel)
        return;

    /* check for callback */
    if (logCallback == NULL)
        return;

    va_start(vargs, fmt);
#ifdef _WIN32
    vsprintf_s(messageBuffer, sizeof(messageBuffer), fmt, vargs);
#else
    vsprintf(messageBuffer, fmt, vargs);
#endif
    logCallback(severity, messageBuffer);
    va_end(vargs);
} /* end EAS_Report */

/*----------------------------------------------------------------------------
 * EAS_ReportX()
 *----------------------------------------------------------------------------
*/
void EAS_ReportX (int severity, const char *fmt, ...)
{
    va_list vargs;

    switch (severity)
    {
        case _EAS_SEVERITY_FATAL:
            eas_fatalErrors++;
            break;

        case _EAS_SEVERITY_ERROR:
            eas_errors++;
            break;

        case _EAS_SEVERITY_WARNING:
            eas_warnings++;
            break;

        default:
            break;
    }

    /* check severity level */
    if (severity > severityLevel)
        return;

    /* check for callback */
    if (logCallback == NULL)
        return;

    va_start(vargs, fmt);
#ifdef _WIN32
    vsprintf_s(messageBuffer, sizeof(messageBuffer), fmt, vargs);
#else
    vsprintf(messageBuffer, fmt, vargs);
#endif
    logCallback(severity, messageBuffer);
    va_end(vargs);
}
#endif

/*----------------------------------------------------------------------------
 * EAS_DLLSetDebugLevel()
 *----------------------------------------------------------------------------
*/
EAS_EXPORT void EAS_DLLSetDebugLevel (int severity)
{
    severityLevel = severity;
}

/*----------------------------------------------------------------------------
 * EAS_ExSetDebugLevel()
 *----------------------------------------------------------------------------
*/
void EAS_SetDebugLevel (int severity)
{
    severityLevel = severity;
}

/*----------------------------------------------------------------------------
 * EAS_SelectLibrary()
 *----------------------------------------------------------------------------
*/
EAS_EXPORT EAS_RESULT EAS_SelectLib (EAS_DATA_HANDLE pEASData, EAS_HANDLE streamHandle, EAS_BOOL testLib)
{
    extern EAS_SNDLIB_HANDLE VMGetLibHandle(EAS_INT libNum);
    return EAS_SetSoundLibrary(pEASData, streamHandle, VMGetLibHandle(testLib ? 1 : 0));
}

// Callback proc
static OSStatus RenderProc( void                                    *inRefCon,
                                        AudioUnitRenderActionFlags  *ioActionFlags,
                                        const AudioTimeStamp                *inTimeStamp,
                                        UInt32                              inBusNumber,
                                        UInt32                              inNumberFrames,
                                        AudioBufferList                 *ioData)
{
    // Get the mutex
    pthread_mutex_lock(&mtx);

    short* ptrOutL = (short *)ioData->mBuffers[0].mData;
    short* ptrOutR = (short *)ioData->mBuffers[1].mData;

    memset(ptrOutL, 0, pBuf->uOutFrames);
    memset(ptrOutR, 0, pBuf->uOutFrames);

    // check if there is any data in the buffer
    if (pBuf->ix == 0 )
    {
        // Release the mutex and signal the python thread
        pthread_mutex_unlock(&mtx);
        pthread_cond_signal(&cond);
        return 0;
    }

    // Get a ptr to the data
    short* pData = pBuf->pData[--(pBuf->ix)];

    // Now copy the data
    int i;
    for (i = 0; i < pBuf->uOutFrames; i+=2 )
    {
        *ptrOutL++ = pData[i];
        *ptrOutR++ = pData[i+1];
    }

    // Release the mutex
    pthread_mutex_unlock(&mtx);
    pthread_cond_signal(&cond);

    return 0;
}

EAS_RESULT addData(EAS_PCM *pAudioBuffer)
{
    // Copy the data we got from the synth
    memcpy(pBuf->pData[(pBuf->ix)++], pAudioBuffer, 2048);

    // Start the output Audio Unit only the first time
    if ( bStopped == true )
    {
        bStopped = false;
        OSStatus err = AudioOutputUnitStart (OutputUnit);
        if (err)
        {
            printf ("AudioDeviceStart=%ld\n", err);
            return EAS_FAILURE;
        }
    }

    return EAS_SUCCESS;
}


EAS_EXPORT EAS_RESULT EAS_RenderWaveOut(EAS_DATA_HANDLE easHandle, EAS_PCM *pAudioBuffer, EAS_I32 numRequested, EAS_I32 *pNumGenerated)
{
    // Get the mutex
    pthread_mutex_lock(&mtx);

    // Check if our buffer is full
    while(pBuf->ix == MAX_BUFFERS - 1)
        pthread_cond_wait(&cond, &mtx);

    // Call the synth the render a buffer
    EAS_RESULT result = EAS_Render(easHandle, pAudioBuffer, numRequested, pNumGenerated);
    addData( pAudioBuffer );

    // Release the mutex
    pthread_mutex_unlock(&mtx);

    //Done
    return result;
}

#ifdef AUX_MIXER
EAS_EXPORT EAS_RESULT EAS_RenderAuxMixerWaveOut (EAS_DATA_HANDLE easHandle, EAS_PCM *pAudioBuffer, EAS_I32 *pNumGenerated)
{
    // Get the mutex
    pthread_mutex_lock(&mtx);

    // Check if our buffer is full
    while(pBuf->ix == MAX_BUFFERS - 1)
        pthread_cond_wait(&cond, &mtx);

    EAS_RESULT result = EAS_RenderAuxMixer(easHandle, pAudioBuffer, pNumGenerated);
    addData( pAudioBuffer );

    // Release the mutex
    pthread_mutex_unlock(&mtx);

    return result;
}
#endif

EAS_EXPORT EAS_RESULT OpenWaveOutDevice(EAS_INT devNum, EAS_INT sampleRate, EAS_INT maxBufSize)
{
    // Open the default output unit
    ComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    Component comp = FindNextComponent(NULL, &desc);
    if (comp == NULL)
    {
        printf ("Could find the default output unit!!!\n");
        return EAS_FAILURE;
    }

    OSStatus err = OpenAComponent(comp, &OutputUnit);
    if (comp == NULL)
    {
        printf ("OpenAComponent=%ld\n", err);
        return EAS_FAILURE;
    }

    // Set up a callback function to generate output to the output unit
    AURenderCallbackStruct auRenderCallback;
    auRenderCallback.inputProc = RenderProc;
    auRenderCallback.inputProcRefCon = NULL;

    err = AudioUnitSetProperty (OutputUnit,
                                kAudioUnitProperty_SetRenderCallback,
                                kAudioUnitScope_Input,
                                0,
                                &auRenderCallback,
                                sizeof(auRenderCallback));
    if (err)
    {
        printf ("AudioUnitSetProperty-CB=%ld\n", err);
        return EAS_FAILURE;;
    }

    pConfig = EAS_Config();

    // The synth engine already uses short* for the buffers so let CoreAudio do any conversions if needed
    if (sampleRate != 0)
        streamFormat.mSampleRate = sampleRate;
    else
        streamFormat.mSampleRate = pConfig->sampleRate;

    streamFormat.mFormatID =    kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger
                                | kAudioFormatFlagsNativeEndian
                                | kLinearPCMFormatFlagIsPacked
                                | kAudioFormatFlagIsNonInterleaved;

    streamFormat.mBytesPerPacket   = 2;
    streamFormat.mFramesPerPacket  = 1;
    streamFormat.mBytesPerFrame    = 2;
    streamFormat.mChannelsPerFrame = 2;
    streamFormat.mBitsPerChannel   = 16;

    err = AudioUnitSetProperty (OutputUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &streamFormat,
                                sizeof(AudioStreamBasicDescription));
    if (err)
    {
        printf ("AudioUnitSetProperty-SF= %4.4s, %ld\n", (char*)&err, err);
        return EAS_FAILURE;
    }

     // Initialize
    err = AudioUnitInitialize(OutputUnit);
    if (err)
    {
        printf ("AudioUnitInitialize = %ld\n", err);
        return EAS_FAILURE;
    }

    pBuf = (S_BUFFER_INFO *) malloc(sizeof(S_BUFFER_INFO));
    if( !pBuf )
        return EAS_FAILURE;

    pBuf->uOutBufferLength = pConfig->mixBufferSize * streamFormat.mBitsPerChannel / 2;
    UInt32 uDataSize = sizeof(pBuf->uOutBufferLength);

    err = AudioUnitSetProperty(OutputUnit, kAudioDevicePropertyBufferSize, kAudioUnitScope_Output, 0, &pBuf->uOutBufferLength, uDataSize);
    if (err)
    {
        printf ("AudioUnitSetProperty = %ld\n", err);
        return EAS_FAILURE;
    }

    err = AudioUnitGetProperty(OutputUnit, kAudioDevicePropertyBufferSize, kAudioUnitScope_Output, 0, &pBuf->uOutBufferLength, &uDataSize);
    if (err)
    {
        printf ("AudioUnitGetProperty = %ld\n", err);
        return EAS_FAILURE;
    }

    pBuf->uLength = pBuf->uOutBufferLength;
    int i;
    for ( i = 0; i < MAX_BUFFERS; i++)
        pBuf->pData[i]   = malloc(pBuf->uLength);

    pBuf->uOutBufferLength /= pConfig->numChannels;
    pBuf->uOutFrames = pBuf->uOutBufferLength / sizeof(short);

    pBuf->ix = 0;

    // Init the stop flag
    bStopped = true;

    int result = pthread_mutex_init(&mtx, NULL);
    if (result)
    {
        printf("pthread_mutex_init failed\n");
        return EAS_FAILURE;
    }

    result = pthread_cond_init(&cond, NULL);
    if (result)
    {
        printf("pthread_cond_init failed\n");
        return EAS_FAILURE;
    }

    // Done
    return EAS_SUCCESS;
}


EAS_EXPORT EAS_RESULT StartStream()
{
    OSStatus err = noErr;
    pthread_mutex_lock(&mtx);
    if ( bStopped == true )
    {
        err = AudioOutputUnitStart (OutputUnit);
        if (err)
        {
            printf ("AudioOutputUnitStart=%ld\n", err);
            return EAS_FAILURE;
        }
        bStopped = false;
    }

    return EAS_SUCCESS;
}


EAS_EXPORT EAS_RESULT CloseWaveOutDevice()
{
    OSStatus err;

    pthread_mutex_lock(&mtx);
    if( false == bStopped )
    {
        AudioOutputUnitStop (OutputUnit);
        bStopped = true;

        err = AudioUnitUninitialize (OutputUnit);
        if (err)
        {
            printf ("AudioUnitUninitialize=%ld\n", err);
            return EAS_FAILURE;
        }

        CloseComponent (OutputUnit);
        int i = 0;
        for(i; i < MAX_BUFFERS; i++)
            free(pBuf->pData[i]);

        free(pBuf);
    }

    pthread_mutex_unlock(&mtx);
    return EAS_SUCCESS;
}


#if defined(_DEBUG) && !defined(MSC)
#include <crtdbg.h>
/*----------------------------------------------------------------------------
 * EnableHeapDebug()
 *----------------------------------------------------------------------------
*/
static void EnableHeapDebug (void)
{
    int temp;
    temp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    temp |= _CRTDBG_ALLOC_MEM_DF;
    temp |= _CRTDBG_CHECK_ALWAYS_DF;
    temp |= _CRTDBG_LEAK_CHECK_DF;
//  temp |= _CRTDBG_DELAY_FREE_MEM_DF;
    _CrtSetDbgFlag(temp);
}

/*----------------------------------------------------------------------------
 * HeapCheck()
 *----------------------------------------------------------------------------
 * Check heap status
 *----------------------------------------------------------------------------
*/
void HeapCheck (void)
{
    int heapStatus;

    /* Check heap status */
    heapStatus = _heapchk();
    if ((heapStatus == _HEAPOK) || (heapStatus == _HEAPEMPTY))
        return;

    EAS_ReportX(_EAS_SEVERITY_FATAL, "Heap corrupt\n" );
}
#endif


/*----------------------------------------------------------------------------
 * EAS_HWInit
 *
 * Initialize host wrapper interface
 *
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWInit (EAS_HW_DATA_HANDLE *pHWInstData)
{

#if defined(_DEBUG) && !defined(MSC)
    EnableHeapDebug();
#endif

 #ifdef BUFFERED_FILE_ACCESS
    EAS_ReportX(_EAS_SEVERITY_INFO, "EAS_HWInit: Buffered file access\n");
 #else
    EAS_ReportX(_EAS_SEVERITY_INFO, "EAS_HWInit: Memory mapped file access\n");
 #endif

    /* simulate failure */
    if (errorConditions[eInitError])
        return EAS_FAILURE;

    /* need to track file opens for duplicate handles */
    *pHWInstData = malloc(sizeof(EAS_HW_INST_DATA));
    if (!(*pHWInstData))
        return EAS_ERROR_MALLOC_FAILED;

    EAS_HWMemSet(*pHWInstData, 0, sizeof(EAS_HW_INST_DATA));
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 * EAS_HWShutdown
 *
 * Shut down host wrapper interface
 *
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWShutdown (EAS_HW_DATA_HANDLE hwInstData)
{

    /* simulate failure */
    if (errorConditions[eShutdownError])
        return EAS_FAILURE;

    free(hwInstData);

#if defined(_DEBUG) && !defined(MSC)
    HeapCheck();
#endif

    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWMalloc
 *
 * Allocates dynamic memory
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
void *EAS_HWMalloc (EAS_HW_DATA_HANDLE hwInstData, EAS_I32 size)
{
    /* simulate failure */
    if (errorConditions[eMallocError])
        return NULL;

    return malloc((size_t) size);
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWFree
 *
 * Frees dynamic memory
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
void EAS_HWFree (EAS_HW_DATA_HANDLE hwInstData, void *p)
{
    free(p);
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWMemCpy
 *
 * Copy memory wrapper
 *
 *----------------------------------------------------------------------------
*/
void *EAS_HWMemCpy (void *dest, const void *src, EAS_I32 amount)
{
    return memcpy(dest, src, (size_t) amount);
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWMemSet
 *
 * Set memory wrapper
 *
 *----------------------------------------------------------------------------
*/
void *EAS_HWMemSet (void *dest, int val, EAS_I32 amount)
{
    return memset(dest, val, (size_t) amount);
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWMemCmp
 *
 * Compare memory wrapper
 *
 *----------------------------------------------------------------------------
*/
EAS_I32 EAS_HWMemCmp (const void *s1, const void *s2, EAS_I32 amount)
{
    return (EAS_I32) memcmp(s1, s2, (size_t) amount);
}

#ifdef BUFFERED_FILE_ACCESS
/*----------------------------------------------------------------------------
 *
 * EAS_HWOpenFile
 *
 * Open a file for read or write
 *
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWOpenFile (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_LOCATOR locator, EAS_FILE_HANDLE *pFile, EAS_FILE_MODE mode)
{
    EAS_HW_FILE *file;
    int i;

    /* set return value to NULL */
    *pFile = NULL;

    /* only support read mode at this time */
    if (mode != EAS_FILE_READ)
        return EAS_ERROR_INVALID_FILE_MODE;

    /* find an empty entry in the file table */
    file = hwInstData->files;
    for (i = 0; i < EAS_MAX_FILE_HANDLES; i++)
    {
        /* is this slot being used? */
        if (file->pFile == NULL)
        {
            EAS_RESULT result;

            /* open the file */
            file->pFile = fopen((const char*) locator, "rb");
            if (file->pFile == NULL)
                return EAS_ERROR_FILE_OPEN_FAILED;

            /* get file length */
            if ((result = EAS_HWFileLength(hwInstData, file, &file->fileSize)) != EAS_SUCCESS)
            {
                EAS_HWCloseFile(hwInstData, file);
                return result;
            }

#ifdef DEBUG_FILE_IO
            EAS_ReportX(_EAS_SEVERITY_NOFILTER, "EAS_HWOpenFile: Open file %d\n", i);
#endif

            /* initialize some values */
            file->bytesInBuffer = 0;
            file->readIndex = 0;
            file->filePos = 0;
            file->dup = EAS_FALSE;

            *pFile = file;
            return EAS_SUCCESS;
        }
        file++;
    }

    /* too many open files */
    return EAS_ERROR_MAX_FILES_OPEN;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWFillBuffer
 *
 * Fill buffer from file
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWFillBuffer (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file)
{
    /* reposition the file pointer */
    if (fseek(file->pFile, file->filePos, SEEK_SET) != 0)
        return EAS_ERROR_FILE_SEEK;

    /* read some data from the file */
    file->bytesInBuffer = (EAS_I32) fread(file->buffer, 1, EAS_FILE_BUFFER_SIZE, file->pFile);
    file->readIndex = 0;
    if (file->bytesInBuffer == 0)
        return EAS_EOF;
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWReadFile
 *
 * Read data from a file
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWReadFile (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void *pBuffer, EAS_I32 n, EAS_I32 *pBytesRead)
{
    EAS_RESULT result;
    EAS_I32 temp;
    EAS_U8 *p = pBuffer;
    EAS_I32 bytesLeft = n;

    *pBytesRead = 0;

    /* check handle integrity */
    if (file->pFile == NULL)
        return EAS_ERROR_INVALID_HANDLE;

#ifdef DEBUG_FILE_IO
    EAS_ReportX(_EAS_SEVERITY_NOFILTER, "EAS_HWReadFile: Reading %d bytes from position %d\n", n, file->filePos);
#endif

    /* try to fulfill request from buffer */
    for (;bytesLeft > 0;)
    {
        /* how many bytes can we get from buffer? */
        temp = file->bytesInBuffer - file->readIndex;
        if (temp > bytesLeft)
            temp = bytesLeft;

        /* copy data from buffer */
        EAS_HWMemCpy(p, &file->buffer[file->readIndex], temp);
        *pBytesRead += temp;
        file->readIndex += temp;
        file->filePos += temp;
        p += temp;
        bytesLeft -= temp;

        /* don't refill buffer if request is bigger than buffer */
        if ((bytesLeft == 0) || (bytesLeft >= EAS_FILE_BUFFER_SIZE))
            break;

        /* refill buffer */
        if ((result = EAS_HWFillBuffer(hwInstData, file)) != EAS_SUCCESS)
            return result;
    }

    /* more to read? do unbuffered read directly to target memory */
    if (bytesLeft)
    {

        /* position the file pointer */
        if (fseek(file->pFile, file->filePos, SEEK_SET) != 0)
            return EAS_ERROR_FILE_SEEK;

        /* read data in the buffer */
        temp = (EAS_I32) fread(p, 1, (size_t) bytesLeft, file->pFile);
        *pBytesRead += temp;
        file->filePos += temp;

        /* reset buffer info */
        file->bytesInBuffer = 0;
        file->readIndex = 0;
    }

#ifdef DEBUG_FILE_IO
    {
#define BYTES_PER_LINE 16
        char str[BYTES_PER_LINE * 3 + 1];
        EAS_INT i;
        for (i = 0; i < (n > BYTES_PER_LINE ? BYTES_PER_LINE : n) ; i ++)
            sprintf(&str[i*3], "%02x ", ((EAS_U8*)pBuffer)[i]);
        if (i)
            EAS_ReportX(_EAS_SEVERITY_NOFILTER, "%s\n", str);
    }
#endif

    /* were n bytes read? */
    if (*pBytesRead != n)
        return EAS_EOF;

    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWGetByte
 *
 * Read a byte from a file
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWGetByte (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void *p)
{
    EAS_RESULT result;

    /* check handle integrity */
    if (file->pFile == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* use local buffer - do we have any data? */
    if (file->readIndex >= file->bytesInBuffer)
    {
        if ((result = EAS_HWFillBuffer(hwInstData, file)) != EAS_SUCCESS)
            return result;

        /* if nothing to read, return EOF */
        if (file->bytesInBuffer == 0)
            return EAS_EOF;
    }

    /* get a character from the buffer */
    *((EAS_U8*) p) = file->buffer[file->readIndex++];

#ifdef DEBUG_FILE_IO
    EAS_ReportX(_EAS_SEVERITY_NOFILTER, "EAS_HWGetByte: Reading from position %d, byte = 0x%02x\n", file->filePos, *(EAS_U8*)p);
#endif

    file->filePos++;
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWGetWord
 *
 * Read a 16-bit value from the file
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWGetWord (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void *p, EAS_BOOL msbFirst)
{
    EAS_RESULT result;
    EAS_I32 count;
    EAS_U8 c[2];

#ifdef DEBUG_FILE_IO
    EAS_ReportX(_EAS_SEVERITY_NOFILTER, "EAS_HWGetWord: Reading 2 bytes from position %d\n", file->filePos);
#endif

    /* read 2 bytes from the file */
    if ((result = EAS_HWReadFile(hwInstData, file, c, 2, &count)) != EAS_SUCCESS)
        return result;

    /* order them as requested */
    if (msbFirst)
        *((EAS_U16*) p) = ((EAS_U16) c[0] << 8) | c[1];
    else
        *((EAS_U16*) p) = ((EAS_U16) c[1] << 8) | c[0];

    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWGetDWord
 *
 * Read a 16-bit value from the file
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWGetDWord (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void *p, EAS_BOOL msbFirst)
{
    EAS_RESULT result;
    EAS_I32 count;
    EAS_U8 c[4];

#ifdef DEBUG_FILE_IO
    EAS_ReportX(_EAS_SEVERITY_NOFILTER, "EAS_HWGetDWord: Reading 4 bytes from position %d\n", file->filePos);
#endif

    /* read 4 bytes from the file */
    if ((result = EAS_HWReadFile(hwInstData, file, c, 4, &count)) != EAS_SUCCESS)
        return result;

    /* order them as requested */
    if (msbFirst)
        *((EAS_U32*) p) = ((EAS_U32) c[0] << 24) | ((EAS_U32) c[1] << 16) | ((EAS_U32) c[2] << 8) | c[3];
    else
        *((EAS_U32*) p) = ((EAS_U32) c[3] << 24) | ((EAS_U32) c[2] << 16) | ((EAS_U32) c[1] << 8) | c[0];

    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWFilePos
 *
 * Returns the current location in the file
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWFilePos (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 *pPosition)
{

    /* check handle integrity */
    if (file->pFile == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    *pPosition = file->filePos;
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWFileSeek
 *
 * Seek to a specific location in the file
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWFileSeek (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 position)
{
    EAS_I32 newIndex;

    /* check handle integrity */
    if (file->pFile == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* check for seek past end */
    if ((position < 0) || (position > file->fileSize))
        return EAS_ERROR_FILE_SEEK;

#ifdef DEBUG_FILE_IO
    EAS_ReportX(_EAS_SEVERITY_NOFILTER, "EAS_HWFileSeek: Seeking to new position %d\n", file->filePos);
#endif

    /* is new position in current buffer? */
    newIndex = position - file->filePos + file->readIndex;
    if ((newIndex >= 0) && (newIndex < file->bytesInBuffer))
    {
        file->readIndex = newIndex;
        file->filePos = position;
        return EAS_SUCCESS;
    }

    /* save new position and reset buffer info so EAS_HWGetByte doesn't fail */
    file->filePos = position;
    file->bytesInBuffer = 0;
    file->readIndex = 0;
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWFileSeekOfs
 *
 * Seek forward or back relative to the current position
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWFileSeekOfs (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 position)
{
    EAS_I32 temp;

#ifdef DEBUG_FILE_IO
    EAS_ReportX(_EAS_SEVERITY_NOFILTER, "EAS_HWFileSeekOfs: Seeking to new position %d\n", file->filePos + position);
#endif

    /* check handle integrity */
    if (file->pFile == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* check for seek past end */
    temp = file->filePos + position;
    if ((temp < 0) || (temp > file->fileSize))
        return EAS_ERROR_FILE_SEEK;

    /* is new position in current buffer? */
    temp = position + file->readIndex;
    if ((temp >= 0) && (temp < file->bytesInBuffer))
    {
        file->readIndex = temp;
        file->filePos += position;
        return EAS_SUCCESS;
    }

    /* save new position and reset buffer info so EAS_HWGetByte doesn't fail */
    file->filePos += position;
    file->bytesInBuffer = 0;
    file->readIndex = 0;
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWFileLength
 *
 * Return the file length
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWFileLength (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 *pLength)
{
    long pos;

    /* check handle integrity */
    if (file->pFile == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    if ((pos = ftell(file->pFile)) == -1L)
        return EAS_ERROR_FILE_LENGTH;
    if (fseek(file->pFile, 0L, SEEK_END) != 0)
        return EAS_ERROR_FILE_LENGTH;
    if ((*pLength = ftell(file->pFile)) == -1L)
        return EAS_ERROR_FILE_LENGTH;
    if (fseek(file->pFile, pos, SEEK_SET) != 0)
        return EAS_ERROR_FILE_LENGTH;
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWDupHandle
 *
 * Duplicate a file handle
 *
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWDupHandle (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_FILE_HANDLE* pDupFile)
{
    EAS_HW_FILE *dupfile;
    int i;

    /* check handle integrity */
    *pDupFile = NULL;
    if (file->pFile == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* find an empty entry in the file table */
    dupfile = hwInstData->files;
    for (i = 0; i < EAS_MAX_FILE_HANDLES; i++)
    {
        /* is this slot being used? */
        if (dupfile->pFile == NULL)
        {

            /* copy info from the handle to be duplicated */
            dupfile->filePos = file->filePos;
            dupfile->pFile = file->pFile;
            dupfile->fileSize = file->fileSize;

            /* set the duplicate handle flag */
            dupfile->dup = file->dup = EAS_TRUE;

            /* initialize some values */
            dupfile->bytesInBuffer = 0;
            dupfile->readIndex = 0;

            *pDupFile = dupfile;
            return EAS_SUCCESS;
        }
        dupfile++;
    }

    /* too many open files */
    return EAS_ERROR_MAX_FILES_OPEN;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWClose
 *
 * Wrapper for fclose function
 *
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWCloseFile (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file1)
{
    EAS_HW_FILE *file2,*dupFile;
    int i;

    /* check handle integrity */
    if (file1->pFile == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* check for duplicate handle */
    if (file1->dup)
    {
        dupFile = NULL;
        file2 = hwInstData->files;
        for (i = 0; i < EAS_MAX_FILE_HANDLES; i++)
        {
            /* check for duplicate */
            if ((file1 != file2) && (file2->pFile == file1->pFile))
            {
                /* is there more than one duplicate? */
                if (dupFile != NULL)
                {
                    /* clear this entry and return */
                    file1->pFile = NULL;
                    return EAS_SUCCESS;
                }

                /* this is the first duplicate found */
                dupFile = file2;
            }
            file2++;
        }

        /* there is only one duplicate, clear the dup flag */
        if (dupFile)
            dupFile->dup = EAS_FALSE;
        else
            /* if we get here, there's a serious problem */
            return EAS_ERROR_HANDLE_INTEGRITY;

        /* clear this entry and return */
        file1->pFile = NULL;
        return EAS_SUCCESS;
    }

    /* no duplicates - close the file */
    if (fclose(file1->pFile) != 0)
        return EAS_ERROR_CLOSE_FAILED;

    /* clear this entry and return */
    file1->pFile = NULL;
    return EAS_SUCCESS;
}
#else
/*----------------------------------------------------------------------------
 *
 * EAS_HWOpenFile
 *
 * Open a file for read or write
 *
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWOpenFile (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_LOCATOR locator, EAS_FILE_HANDLE *pFile, EAS_FILE_MODE mode)
{
    EAS_HW_FILE *file;
    FILE *ioFile;
    int i, temp;

    /* set return value to NULL */
    *pFile = NULL;

    /* simulate failure */
    if (errorConditions[eOpenError])
        return EAS_FAILURE;

    /* only support read mode at this time */
    if (mode != EAS_FILE_READ)
        return EAS_ERROR_INVALID_FILE_MODE;

    /* find an empty entry in the file table */
    file = hwInstData->files;
    for (i = 0; i < EAS_MAX_FILE_HANDLES; i++)
    {
        /* is this slot being used? */
        if (file->buffer == NULL)
        {
            /* open the file */
            if ((ioFile = fopen(locator,"rb")) == NULL)
                return EAS_ERROR_FILE_OPEN_FAILED;

            /* determine the file size */
            if (fseek(ioFile, 0L, SEEK_END) != 0)
                return EAS_ERROR_FILE_LENGTH;
            if ((file->fileSize = ftell(ioFile)) == -1L)
                return EAS_ERROR_FILE_LENGTH;
            if (fseek(ioFile, 0L, SEEK_SET) != 0)
                return EAS_ERROR_FILE_LENGTH;

            /* allocate a buffer */
            file->buffer = EAS_HWMalloc(hwInstData, file->fileSize);
            if (file->buffer == NULL)
            {
                fclose(ioFile);
                return EAS_ERROR_MALLOC_FAILED;
            }

            /* read the file into memory */
            temp = (int) fread(file->buffer, (size_t) file->fileSize, 1, ioFile);

            /* close the file - don't need it any more */
            fclose(ioFile);

            /* check for error reading file */
            if (temp != 1)
                return EAS_ERROR_FILE_READ_FAILED;

            /* initialize some values */
            file->filePos = 0;
            file->dup = EAS_FALSE;

            *pFile = file;
            return EAS_SUCCESS;
        }
        file++;
    }

    /* too many open files */
    return EAS_ERROR_MAX_FILES_OPEN;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWReadFile
 *
 * Read data from a file
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_RESULT EAS_HWReadFile (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void *pBuffer, EAS_I32 n, EAS_I32 *pBytesRead)
{
    EAS_I32 count;

    /* simulate failure */
    if (errorConditions[eReadError])
        return EAS_FAILURE;

    /* make sure we have a valid handle */
    if (file->buffer == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* calculate the bytes to read */
    count = file->fileSize - file->filePos;
    if (n < count)
        count = n;

    /* copy the data to the requested location, and advance the pointer */
    if (count)
        EAS_HWMemCpy(pBuffer, &file->buffer[file->filePos], count);
    file->filePos += count;
    *pBytesRead = count;

    /* were n bytes read? */
    if (count!= n)
        return EAS_EOF;
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWGetByte
 *
 * Read a byte from a file
 *
 *----------------------------------------------------------------------------
*/
/*lint -e{715} hwInstData available for customer use */
EAS_RESULT EAS_HWGetByte (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void *p)
{

    /* simulate failure */
    if (errorConditions[eReadError])
        return EAS_FAILURE;

    /* make sure we have a valid handle */
    if (file->buffer == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* check for end of file */
    if (file->filePos >= file->fileSize)
    {
        *((EAS_U8*) p) = 0;
        return EAS_EOF;
    }

    /* get a character from the buffer */
    *((EAS_U8*) p) = file->buffer[file->filePos++];
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWGetWord
 *
 * Returns the current location in the file
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_RESULT EAS_HWGetWord (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void *p, EAS_BOOL msbFirst)
{
    EAS_RESULT result;
    EAS_U8 c1, c2;

    /* read 2 bytes from the file */
    if ((result = EAS_HWGetByte(hwInstData, file, &c1)) != EAS_SUCCESS)
        return result;
    if ((result = EAS_HWGetByte(hwInstData, file, &c2)) != EAS_SUCCESS)
        return result;

    /* order them as requested */
    if (msbFirst)
        *((EAS_U16*) p) = ((EAS_U16) c1 << 8) | c2;
    else
        *((EAS_U16*) p) = ((EAS_U16) c2 << 8) | c1;

    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWGetDWord
 *
 * Returns the current location in the file
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_RESULT EAS_HWGetDWord (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void *p, EAS_BOOL msbFirst)
{
    EAS_RESULT result;
    EAS_U8 c1, c2,c3,c4;

    /* read 4 bytes from the file */
    if ((result = EAS_HWGetByte(hwInstData, file, &c1)) != EAS_SUCCESS)
        return result;
    if ((result = EAS_HWGetByte(hwInstData, file, &c2)) != EAS_SUCCESS)
        return result;
    if ((result = EAS_HWGetByte(hwInstData, file, &c3)) != EAS_SUCCESS)
        return result;
    if ((result = EAS_HWGetByte(hwInstData, file, &c4)) != EAS_SUCCESS)
        return result;

    /* order them as requested */
    if (msbFirst)
        *((EAS_U32*) p) = ((EAS_U32) c1 << 24) | ((EAS_U32) c2 << 16) | ((EAS_U32) c3 << 8) | c4;
    else
        *((EAS_U32*) p)= ((EAS_U32) c4 << 24) | ((EAS_U32) c3 << 16) | ((EAS_U32) c2 << 8) | c1;

    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWFilePos
 *
 * Returns the current location in the file
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_RESULT EAS_HWFilePos (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 *pPosition)
{

    /* simulate failure */
    if (errorConditions[ePosError])
        return EAS_FAILURE;

    /* make sure we have a valid handle */
    if (file->buffer == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    *pPosition = file->filePos;
    return EAS_SUCCESS;
} /* end EAS_HWFilePos */

/*----------------------------------------------------------------------------
 *
 * EAS_HWFileSeek
 *
 * Seek to a specific location in the file
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_RESULT EAS_HWFileSeek (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 position)
{

    /* simulate failure */
    if (errorConditions[eSeekError])
        return EAS_FAILURE;

    /* make sure we have a valid handle */
    if (file->buffer == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* validate new position */
    if ((position < 0) || (position > file->fileSize))
        return EAS_ERROR_FILE_SEEK;

    /* save new position */
    file->filePos = position;
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWFileSeekOfs
 *
 * Seek forward or back relative to the current position
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_RESULT EAS_HWFileSeekOfs (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 position)
{

    /* simulate failure */
    if (errorConditions[eSeekError])
        return EAS_FAILURE;

    /* make sure we have a valid handle */
    if (file->buffer == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* determine the file position */
    position += file->filePos;
    if ((position < 0) || (position > file->fileSize))
        return EAS_ERROR_FILE_SEEK;

    /* save new position */
    file->filePos = position;
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWFileLength
 *
 * Return the file length
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_RESULT EAS_HWFileLength (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 *pLength)
{

    /* simulate failure */
    if (errorConditions[eLengthError])
        return EAS_FAILURE;

    /* make sure we have a valid handle */
    if (file->buffer == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    *pLength = file->fileSize;
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWDupHandle
 *
 * Duplicate a file handle
 *
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWDupHandle (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_FILE_HANDLE *pDupFile)
{
    EAS_HW_FILE *dupFile;
    int i;

    /* simulate failure */
    if (errorConditions[eDupError])
        return EAS_FAILURE;

    /* make sure we have a valid handle */
    if (file->buffer == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* find an empty entry in the file table */
    dupFile = hwInstData->files;
    for (i = 0; i < EAS_MAX_FILE_HANDLES; i++)
    {
        /* is this slot being used? */
        if (dupFile->buffer == NULL)
        {

            /* copy info from the handle to be duplicated */
            dupFile->filePos = file->filePos;
            dupFile->fileSize = file->fileSize;
            dupFile->buffer = file->buffer;

            /* set the duplicate handle flag */
            dupFile->dup = file->dup = EAS_TRUE;

            *pDupFile = dupFile;
            return EAS_SUCCESS;
        }
        dupFile++;
    }

    /* too many open files */
    return EAS_ERROR_MAX_FILES_OPEN;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWClose
 *
 * Wrapper for fclose function
 *
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWCloseFile (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file1)
{
    EAS_HW_FILE *file2,*dupFile;
    int i;

    /* simulate failure */
    if (errorConditions[eCloseError])
        return EAS_FAILURE;

    /* make sure we have a valid handle */
    if (file1->buffer == NULL)
        return EAS_ERROR_INVALID_HANDLE;

    /* check for duplicate handle */
    if (file1->dup)
    {
        dupFile = NULL;
        file2 = hwInstData->files;
        for (i = 0; i < EAS_MAX_FILE_HANDLES; i++)
        {
            /* check for duplicate */
            if ((file1 != file2) && (file2->buffer == file1->buffer))
            {
                /* is there more than one duplicate? */
                if (dupFile != NULL)
                {
                    /* clear this entry and return */
                    file1->buffer = NULL;
                    return EAS_SUCCESS;
                }

                /* this is the first duplicate found */
                else
                    dupFile = file2;
            }
            file2++;
        }

        /* there is only one duplicate, clear the dup flag */
        if (dupFile)
            dupFile->dup = EAS_FALSE;
        else
            /* if we get here, there's a serious problem */
            return EAS_ERROR_HANDLE_INTEGRITY;

        /* clear this entry and return */
        file1->buffer = NULL;
        return EAS_SUCCESS;
    }

    /* no duplicates -free the buffer */
    EAS_HWFree(hwInstData, file1->buffer);

    /* clear this entry and return */
    file1->buffer = NULL;
    return EAS_SUCCESS;
}
#endif

/*----------------------------------------------------------------------------
 *
 * EAS_HWVibrate
 *
 * Turn on/off vibrate function
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_RESULT EAS_HWVibrate (EAS_HW_DATA_HANDLE hwInstData, EAS_BOOL state)
{
    vibState = state;
//  EAS_ReportX(_EAS_SEVERITY_NOFILTER, "Vibrate state: %d\n", state);
    return EAS_SUCCESS;
} /* end EAS_HWVibrate */

/*----------------------------------------------------------------------------
 *
 * EAS_HWLED
 *
 * Turn on/off LED
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_RESULT EAS_HWLED (EAS_HW_DATA_HANDLE hwInstData, EAS_BOOL state)
{
    ledState = state;
//  EAS_ReportX(_EAS_SEVERITY_NOFILTER, "LED state: %d\n", state);
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWBackLight
 *
 * Turn on/off backlight
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_RESULT EAS_HWBackLight (EAS_HW_DATA_HANDLE hwInstData, EAS_BOOL state)
{
    backlightState = state;
//  EAS_ReportX(_EAS_SEVERITY_NOFILTER, "Backlight state: %d\n", state);
    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 *
 * EAS_HWYield
 *
 * This function is called periodically by the EAS library to give the
 * host an opportunity to allow other tasks to run. There are two ways to
 * use this call:
 *
 * If you have a multi-tasking OS, you can call the yield function in the
 * OS to allow other tasks to run. In this case, return EAS_FALSE to tell
 * the EAS library to continue processing when control returns from this
 * function.
 *
 * If tasks run in a single thread by sequential function calls (sometimes
 * call a "commutator loop"), return EAS_TRUE to cause the EAS Library to
 * return to the caller. Be sure to check the number of bytes rendered
 * before passing the audio buffer to the codec - it may not be filled.
 * The next call to EAS_Render will continue processing until the buffer
 * has been filled.
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) available for customer use */
EAS_BOOL EAS_HWYield (EAS_HW_DATA_HANDLE hwInstData)
{
    /* put your code here */
    return EAS_FALSE;
}


