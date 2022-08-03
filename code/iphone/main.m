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

#import <UIKit/UIKit.h>
#import "Doom_App.h"
#include <unistd.h>
#include <string.h>

extern char iphoneAppDirectory[1024];
extern int myargc;
extern char **myargv;

int main(int argc, char *argv[]) {
	// save for doom
	myargc = argc;
	myargv = argv;

	// get the app directory based on argv[0]
	strcpy( iphoneAppDirectory, argv[0] );
	
	char * slashPosition = strrchr( iphoneAppDirectory, '/' );
	if ( slashPosition != NULL ) {
		*slashPosition = 0;
	} else {
		iphoneAppDirectory[0] = 0;
	}
	
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    NSString * classString = NSStringFromClass([DoomApp class]);
    int retVal = UIApplicationMain(argc, argv, nil, classString );
    [pool release];
    return retVal;
}
