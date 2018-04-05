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

#import <Foundation/Foundation.h>

#include "iphone_common.h"
#include "doomiphone.h"
// disabling it, not sure if used -tkidd
//#include "ios/GameCenter.h"
#include "../ios/Localization.h"
#import "../ios/LocalizationObjectiveC.h"
#include "iphone_delegate.h"
//#include "DoomGameCenterMatch.h"
//#import "ios/objectivec_utilities.h"

@class UIViewController;

/*
=======================
CommonSystemSetup
 
Called in each game's didFinishLaunching method.
=======================
*/ 
void CommonSystemSetup( UIViewController * gameViewController ) {

	// Authenticate the Game Center player.
	//idGameCenter::Initialize();
	//idGameCenter::AuthenticateLocalPlayer( gameViewController, &gDoomGameCenterMatch );

	// get the documents directory, where we will write configs and save games
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	[documentsDirectory getCString: iphoneDocDirectory 
							maxLength: sizeof( iphoneDocDirectory ) - 1
							encoding: NSASCIIStringEncoding ];
	
	// get the app directory, where our data files live
	// this gives something like:
	// /var/mobile/Applications/71355F9F-6400-4267-B07D-E7980764F5A8/Applications
	// when what we want is:
	// /var/mobile/Applications/71355F9F-6400-4267-B07D-E7980764F5A8/doom.app
	// so we get that in main() from argv[0]
#if 0	
	paths = NSSearchPathForDirectoriesInDomains(NSApplicationDirectory, NSUserDomainMask, YES);
	NSString *appDirectory = [paths objectAtIndex:0];

	static char iphoneAppDirectoryFromAPI[1024];
	[appDirectory getCString: iphoneAppDirectoryFromAPI 
							maxLength: sizeof( iphoneAppDirectoryFromAPI ) - 1
							encoding: NSASCIIStringEncoding ];
#endif
	
	
	// Get the temporary directory.
	NSString * tempDirectory = NSTemporaryDirectory();
	[tempDirectory getCString:iphoneTempDirectory maxLength:PATH_MAX encoding:NSUTF8StringEncoding];
    
    // Initialize the localization system.
    idLocalization_Initialize();
}

/*
 =======================
 MainMenu
 
 Hides the OpenGL View and reverts back to Interface builder.
 =======================
 */ 
void iphoneMainMenu() {
    
    [ gAppDelegate HideGLView];
	menuState = IPM_MAIN;
}

/*
 =======================
 iphonePopGL
 
 Hides the OpenGL View and reverts back to Interface builder.
 =======================
 */ 
void iphonePopGL() {
    
    [ gAppDelegate PopGLView];
	menuState = IPM_MAIN;
}

void iphoneNSLog(const char* message) {
    NSLog( @"%@", [NSString stringWithUTF8String:message] );
}

