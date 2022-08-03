/*
 *  iphone_email.c
 *  Doom
 *
 *  Created by Greg Hodges on 10/20/09.
 *  Copyright 2009 id Software. All rights reserved.
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


#include "iphone_email.h"
#include <time.h>

#import <Foundation/NSURL.h>
#import <UIKit/UIApplication.h>


char *consoleBuffer = NULL;	//buffer for the console output
long int size = 0;

/*
 *	ReplaceAt()
 *	Replaces the char at location with the insertString
 */
void ReplaceAt( char *oldString, const char *insertString, int location)
{
	unsigned long length = strlen(oldString);
	unsigned long chunkLength = strlen(insertString);
	
	char *newString = malloc(length + chunkLength + 1);//the 1 includes space for the null terminating character
	
#if 0
	//strcpy(newString, old);
	strncpy(newString, oldString, location);
	newString[location] = '\0';
	strcat(newString, insertString);
	strcpy(&newString[location+chunkLength], &oldString[location + 1]);
	
	free(oldString);
	oldString = newString;
#endif
	
	//copy the front part
	char frontPart[location+1];
	strncpy(frontPart, oldString, location);
	frontPart[location] = '\0';
	printf("\nfrontPart: %s", frontPart);
	
	//copy the back part
	char backPart[length - location];
	strcpy(backPart, &oldString[location+1]);
	backPart[length - location - 1] = '\0';
	printf("\nbackPart: %s\n\n", backPart);
	
	//put it all together in the new string
	newString[0] = '\0';
	strcat(newString, frontPart);
	strcat(newString, insertString);
	strcat(newString, backPart);
	
	//delete old string
	free(oldString);
	
	//replace old string
	oldString = newString;
}

/*
 *	AppendBuffer()
 *	Directly appends the console buffer
 */
void AppendBuffer(const char *buf)
{	
	unsigned long int length = strlen(buf) + 1; //strlen doesn't include the null terminating character
	char *temp = malloc(length);
	strcpy(temp, buf);
	
	
	for (int i = 0; i < length; ++i)
	{
		if (temp[i] == ' ' || temp[i] == '\n' || temp[i] == '=' )
			temp[i] = '_';
	}
	
#if 0
	int i = 0;
	while (temp[i] != '\0')
	{
		if (temp[i] == ' ')
			ReplaceAt(temp, "_testString_", i);
		
		++i;
	}
	length = strlen(temp) + 1;
#endif
	
	//copy the old & new string into a buffer
	char *newBuf = malloc(size + length);
	if (consoleBuffer)
	{
		strcpy(newBuf, consoleBuffer);
	}
	strcpy(&newBuf[size], temp);

	
	//delete the old string and have it point to the new one
	free(consoleBuffer);
	consoleBuffer = newBuf;
	size = strlen(consoleBuffer);
	
	
	//delete the temp string
	free(temp);
}

/*
 *	AppendChunk()
 *	Just append a chunk of the incoming string from start to i
 */
void AppendChunk(const char *buf, int start, int i)
{
	int chunkSize = i+1 - start;
	char chunk[chunkSize];
	chunk[0] = '\0';
	strncpy(chunk, &buf[start], chunkSize-1);
	chunk[chunkSize-1] = '\0';
	AppendBuffer(chunk);
}

/*
 *	AppendConsoleBuffer()
 *	Appends the console buffer while replacings non-URLscheme compatible text
 */
void AppendConsoleBuffer(const char *buf)
{

}


/*
 *
 *  Email the console buffer to id
 *
 */
void EmailConsole()
{
	
	return; // do not email me anymore.
	
#if 0
	if (!consoleBuffer)
		return;
	
	//copy everything from consoleBuffer into a char*
	char *buffer = malloc(1024+size+1);
	
#if 0
	time_t rawtime;
	struct tm * timeinfo;	
	time ( &rawtime );
	timeinfo = gmtime ( &rawtime );
	char dateBuffer[128];
	sprintf(dateBuffer, asctime(timeinfo));
	for (int i = 0; i < strlen(dateBuffer); ++i)
	{
		if (dateBuffer[i] == ' ' || dateBuffer[i] == ':')
			dateBuffer[i] = '_';
	}
	printf("formatted time:  %s\n", dateBuffer);
	
	sprintf( buffer, "mailto:iphonesupport@idsoftware.com?subject=DoomClassicReport%s&body=%s", dateBuffer, consoleBuffer);
#else
	sprintf( buffer, "mailto:iphonesupport@idsoftware.com?subject=DoomClassicReport&body=%s", consoleBuffer);
#endif	
	
	//call the mail app
	NSURL *url = [[NSURL alloc] initWithString:[NSString stringWithCString:buffer encoding:NSUTF8StringEncoding]];
	[[UIApplication sharedApplication] openURL:url];
	
	free(buffer);
#endif
}


