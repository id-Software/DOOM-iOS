/*----------------------------------------------------------------------------
 *
 * File:
 * eas_hostmm.c
 *
 * Contents and purpose:
 * This file contains the host wrapper functions for stdio, stdlib, etc.
 * This is a sample version that maps the requested files to an
 * allocated memory block and uses in-memory pointers to replace
 * file system calls. The file locator (EAS_FILE_LOCATOR) handle passed
 * HWOpenFile is the same one that is passed to EAS_OpenFile. If your
 * system stores data in fixed locations (such as flash) instead of
 * using a file system, you can use the locator handle to point to
 * your memory. You will need a way of knowing the length of the
 * data stored at that location in order to respond correctly in the
 * HW_FileLength function.
 *
 * Modify this file to suit the needs of your particular system.
 *
 * EAS_MAX_FILE_HANDLES sets the maximum number of MIDI streams within
 * a MIDI type 1 file that can be played.
 *
 * EAS_HW_FILE is a structure to support the file I/O functions. It
 * comprises the base memory pointer, the file read pointer, and
 * the dup flag, which when sets, indicates that the file handle has
 * been duplicated. If your system uses in-memory resources, you
 * can eliminate the duplicate handle logic, and simply copy the
 * base memory pointer and file read pointer to the duplicate handle.
 *
 * Copyright 2005 Sonic Network Inc.

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
 *----------------------------------------------------------------------------
 * Revision Control:
 *   $Revision: 795 $
 *   $Date: 2007-08-01 00:14:45 -0700 (Wed, 01 Aug 2007) $
 *----------------------------------------------------------------------------
*/

#ifdef _lint
#include "lint_stdlib.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "eas_host.h"

/* Only for debugging LED, vibrate, and backlight functions */
#include "eas_report.h"

/* this module requires dynamic memory support */
#ifdef _STATIC_MEMORY
#error "eas_hostmm.c requires the dynamic memory model!\n"
#endif

#ifndef EAS_MAX_FILE_HANDLES
#define EAS_MAX_FILE_HANDLES    32
#endif

/*
 * this structure and the related function are here
 * to support the ability to create duplicate handles
 * and buffering it in memory. If your system uses
 * in-memory resources, you can eliminate the calls
 * to malloc and free, the dup flag, and simply track
 * the file size and read position.
 */
typedef struct eas_hw_file_tag
{
    EAS_I32 fileSize;
    EAS_I32 filePos;
    EAS_BOOL dup;
    EAS_U8 *buffer;
} EAS_HW_FILE;

typedef struct eas_hw_inst_data_tag
{
    EAS_HW_FILE files[EAS_MAX_FILE_HANDLES];
} EAS_HW_INST_DATA;

/*----------------------------------------------------------------------------
 * EAS_HWInit
 *
 * Initialize host wrapper interface
 *
 *----------------------------------------------------------------------------
*/
EAS_RESULT EAS_HWInit (EAS_HW_DATA_HANDLE *pHWInstData)
{

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

    free(hwInstData);
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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
void *EAS_HWMalloc (EAS_HW_DATA_HANDLE hwInstData, EAS_I32 size)
{
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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWReadFile (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void *pBuffer, EAS_I32 n, EAS_I32 *pBytesRead)
{
    EAS_I32 count;

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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWGetByte (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void *p)
{

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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWFilePos (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 *pPosition)
{

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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWFileSeek (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 position)
{

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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWFileSeekOfs (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 position)
{

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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWFileLength (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 *pLength)
{

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

/*----------------------------------------------------------------------------
 *
 * EAS_HWVibrate
 *
 * Turn on/off vibrate function
 *
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWVibrate (EAS_HW_DATA_HANDLE hwInstData, EAS_BOOL state)
{
    EAS_ReportEx(_EAS_SEVERITY_NOFILTER, 0x1a54b6e8, 0x00000001 , state);
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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWLED (EAS_HW_DATA_HANDLE hwInstData, EAS_BOOL state)
{
    EAS_ReportEx(_EAS_SEVERITY_NOFILTER, 0x1a54b6e8, 0x00000002 , state);
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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWBackLight (EAS_HW_DATA_HANDLE hwInstData, EAS_BOOL state)
{
    EAS_ReportEx(_EAS_SEVERITY_NOFILTER, 0x1a54b6e8, 0x00000003 , state);
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
/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_BOOL EAS_HWYield (EAS_HW_DATA_HANDLE hwInstData)
{
    /* put your code here */
    return EAS_FALSE;
}

