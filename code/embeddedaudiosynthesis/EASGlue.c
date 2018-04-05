/*
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.
 
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

#include "EASGlue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "eas.h"
#include "eas_wave.h"
#include "eas_report.h"

#define NUM_BUFFERS         1

#ifndef NDEBUG
static EAS_BOOL EASLibraryCheck (const S_EAS_LIB_CONFIG *pLibConfig);
#endif

static EAS_DATA_HANDLE pEASData;
static const S_EAS_LIB_CONFIG *pLibConfig;
static long int polyphony;
static long int bufferSize;

static EAS_FILE file;
static EAS_HANDLE handle;

void EASGlueInit(void) {
	EAS_RESULT result;
	
	/* get the library configuration */
    pLibConfig = EAS_Config();
	assert( EASLibraryCheck(pLibConfig) );
	
    if (polyphony > pLibConfig->maxVoices)
        polyphony = pLibConfig->maxVoices;
		
	EAS_I32 mixSize = pLibConfig->mixBufferSize;
	
	bufferSize = mixSize * pLibConfig->numChannels * (EAS_I32)sizeof(EAS_PCM) * NUM_BUFFERS;
	
    /* calculate buffer size */
    //bufferSize = pLibConfig->mixBufferSize * pLibConfig->numChannels * (EAS_I32)sizeof(EAS_PCM) * NUM_BUFFERS;

	
	if ( (result = EAS_Init(&pEASData)) != EAS_SUCCESS ) {
		printf( "Error initializing EAS: %li\n", result );
	}
}


void EASGlueShutdown(void) {
	EAS_RESULT result;
	
	EASGlueCloseFile();
	if ( (result = EAS_Shutdown(pEASData)) != EAS_SUCCESS ) {
		printf( "Error shutting down EAS: %li\n", result );
	}
}

void EASGlueOpenFile( const char * filename ) {

	EAS_RESULT result;
	
	
	/* open the file */
	file.path = filename;
	file.fd = 0;
	if ((result = EAS_OpenFile(pEASData, &file, &handle)) != EAS_SUCCESS) {
		printf( "Error opening EAS file: %li\n", result );
		return;
	}
	
	EAS_SetRepeat( pEASData, handle, -1 );
	
	/* prepare for playback */
	if ((result = EAS_Prepare(pEASData, handle)) != EAS_SUCCESS) {
		printf( "Error preparing EAS file: %li\n", result );
		return;
	}
}

void EASGluePause(void) {
	EAS_RESULT result;
	
	if ( handle == 0 ) {
		return;
	}
	
	result = EAS_Pause( pEASData, handle );
	
	if ( result != EAS_SUCCESS ) {
		printf( "Error pausing EAS file: %li\n", result );
	}
}

void EASGlueResume(void) {
	EAS_RESULT result;
	
	result = EAS_Resume( pEASData, handle );
	
	if ( result != EAS_SUCCESS ) {
		printf( "Error pausing EAS file: %li\n", result );
	}
}

void EASGlueCloseFile(void) {
	
	if ( handle == 0 ) {
		return;
	}
	
	// File must be paused or stopped before closing it.
	EASGluePause();
	
	EAS_RESULT result;
	
	result = EAS_CloseFile( pEASData, handle );
	
	if ( result != EAS_SUCCESS ) {
		printf( "Error closing EAS file: %li\n", result );
	}
	
	handle = 0;
}

void EASGlueRender( EAS_PCM * outputBuffer, EAS_I32 * generatedSamples ) {
	EAS_RESULT result;
	
	if ( ( result = EAS_Render( pEASData, outputBuffer, pLibConfig->mixBufferSize, generatedSamples ) ) != EAS_SUCCESS ) {
		printf( "Error rendering EAS: %li\n.", result );
		return;
	}
}


#ifndef NDEBUG
/*----------------------------------------------------------------------------
 * EASLibraryCheck()
 *----------------------------------------------------------------------------
 * Purpose:
 * Displays the library version and checks it against the header
 * file used to build this code.
 *
 * Inputs:
 * pLibConfig       - library configuration retrieved from the library
 *
 * Outputs:
 * returns EAS_TRUE if matched
 *
 * Side Effects:
 *
 *----------------------------------------------------------------------------
*/
static EAS_BOOL EASLibraryCheck (const S_EAS_LIB_CONFIG *libConfig)
{

    /* display the library version */
    { /* dpp: EAS_ReportEx(_EAS_SEVERITY_INFO, "EAS Library Version %d.%d.%d.%d\n",
        libConfig->libVersion >> 24,
        (libConfig->libVersion >> 16) & 0x0f,
        (libConfig->libVersion >> 8) & 0x0f,
        libConfig->libVersion & 0x0f); */ }

    /* display some info about the library build */
    if (libConfig->checkedVersion)
        { /* dpp: EAS_ReportEx(_EAS_SEVERITY_INFO, "\tChecked library\n"); */ }
    { /* dpp: EAS_ReportEx(_EAS_SEVERITY_INFO, "\tMaximum polyphony: %d\n", libConfig->maxVoices); */ }
    { /* dpp: EAS_ReportEx(_EAS_SEVERITY_INFO, "\tNumber of channels: %d\n", libConfig->numChannels); */ }
    { /* dpp: EAS_ReportEx(_EAS_SEVERITY_INFO, "\tSample rate: %d\n", libConfig->sampleRate); */ }
    { /* dpp: EAS_ReportEx(_EAS_SEVERITY_INFO, "\tMix buffer size: %d\n", libConfig->mixBufferSize); */ }
    if (libConfig->filterEnabled)
        { /* dpp: EAS_ReportEx(_EAS_SEVERITY_INFO, "\tFilter enabled\n"); */ }
#ifndef _WIN32_WCE
    { /* dpp: EAS_ReportEx(_EAS_SEVERITY_INFO, "\tLibrary Build Timestamp: %s", ctime((time_t*)&libConfig->buildTimeStamp)); */ }
#endif
    { /* dpp: EAS_ReportEx(_EAS_SEVERITY_INFO, "\tLibrary Build ID: %s\n", libConfig->buildGUID); */ }

    /* check it against the header file used to build this code */
    /*lint -e{778} constant expression used for display purposes may evaluate to zero */
    if (LIB_VERSION != libConfig->libVersion)
    {
        { /* dpp: EAS_ReportEx(_EAS_SEVERITY_FATAL, "Library version does not match header files. EAS Header Version %d.%d.%d.%d\n",
            LIB_VERSION >> 24,
            (LIB_VERSION >> 16) & 0x0f,
            (LIB_VERSION >> 8) & 0x0f,
            LIB_VERSION & 0x0f); */ }
        return EAS_FALSE;
    }
    return EAS_TRUE;
} /* end EASLibraryCheck */

#endif
