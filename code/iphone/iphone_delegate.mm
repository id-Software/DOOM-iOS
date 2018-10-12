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

#import "iphone_delegate.h"

#import <AudioToolbox/AudioServices.h>
#include "doomiphone.h"
#include "iphone_common.h"
// disabling all app store and game center stuff, not sure if used
//#include "ios/InAppStore.h"
//#include "ios/GameCenter.h"



@implementation iphoneApp

@synthesize window;

iphoneApp * gAppDelegate = NULL;
bool inBackgroundProcess = false;

touch_t		sysTouches[MAX_TOUCHES];
touch_t		gameTouches[MAX_TOUCHES];

#define FRAME_HERTZ 30.0f
#if !TARGET_OS_TV
//const static float ACCELEROMETER_UPDATE_INTERVAL = 1.0f / FRAME_HERTZ;
#endif
//FIXME: JadingTsunami (fix) const static float ACCELEROMETER_UPDATE_INTERVAL = 1.0f / FRAME_HERTZ;

/*
 ========================
 applicationDidFinishLaunching
 ========================
 */
- (void)applicationDidFinishLaunching:(UIApplication *)application {
    
    gAppDelegate = self;
    inBackgroundProcess = false;
	hasPushedGLView = NO;
    
    // Create the window programmatically instead of loading from a nib file.
	self.window = [[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
    
    // Disable Screen Dimming.
    [[ UIApplication sharedApplication] setIdleTimerDisabled: YES ];
    
#if !TARGET_OS_TV
    
    // Initial Application Style config.
    [UIApplication sharedApplication].statusBarHidden = YES;
	
	// start the flow of accelerometer events
    /* //FIXME: JadingTsunami (fix) 
	UIAccelerometer *accelerometer = [UIAccelerometer sharedAccelerometer];
    [ accelerometer setDelegate: self ];
    [ accelerometer setUpdateInterval: ACCELEROMETER_UPDATE_INTERVAL ];
     */
    
#endif
    
    [self InitializeInterfaceBuilder ];

	CommonSystemSetup( [navigationController topViewController] );
	
    // do all the game startup work
	iphoneStartup();
    
    UIView * view = navigationController.view;
	
    [window addSubview: view];
	[window setRootViewController:navigationController];
    [window setBackgroundColor: [UIColor blackColor] ];
	[window makeKeyAndVisible];
}

/*
 ========================
 applicationWillResignActive
 ========================
 */
- (void)applicationWillResignActive:(UIApplication *)application {
    inBackgroundProcess = YES;
    
	//idGameCenter::HandleMoveToBackground();
	
	// If we're in a multiplater game, and showing the OpenGL view,
	// go back to the main menu since the multiplayer game is hosed.
	if ( netgame && navigationController.topViewController == gAppDelegate->openGLViewController ) {
		iphoneMainMenu();
	}
	
	iphonePauseMusic();
    iphoneShutdown();
}

/*
 ========================
 applicationDidBecomeActive
 ========================
 */
- (void)applicationDidBecomeActive:(UIApplication *)application {
	inBackgroundProcess = NO;
}


/*
 ========================
 applicationWillTerminate
 ========================
 */
- (void)applicationWillTerminate:(UIApplication *)application {
	iphoneStopMusic();
	iphoneShutdown();
}


/*
 ========================
 applicationDidReceiveMemoryWarning
 ========================
 */
- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
	Com_Printf( "applicationDidReceiveMemoryWarning\n" );
}


/*
 ========================
 dealloc
 ========================
 */
- (void)dealloc {
	[window release];
	[super dealloc];
}

#if !TARGET_OS_TV

/*
 ========================
 accelerometer 
 ========================
- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration
{	
	float acc[4];
	acc[0] = acceleration.x;
	acc[1] = acceleration.y;
	acc[2] = acceleration.z;
	acc[3] = acceleration.timestamp;
    
    
	iphoneTiltEvent( acc );
}
*/

#endif

/*
 ========================
 HACK_PushController  - Removes Flicker from Loading Wads.
  God forgive me.
 
 ========================
 */
- (void) HACK_PushController {
    [navigationController pushViewController:openGLViewController animated:NO];
}

/*
 ========================
 ShowGLView 
 ========================
 */
- (void)ShowGLView {
	
    if( hasPushedGLView == NO ) {
		hasPushedGLView = YES;
		// Hack city.
		[NSTimer scheduledTimerWithTimeInterval:0.2f target:self selector:@selector(HACK_PushController) userInfo:nil repeats:NO];

		[ openGLViewController StartDisplay ];
    }
}

/*
 ========================
 HideGLView 
 ========================
 */
- (void) HideGLView {
    
    [ navigationController popToRootViewControllerAnimated:NO ];
	hasPushedGLView = NO;
}

/*
 ========================
 PopGLView 
 ========================
 */
- (void) PopGLView {
    [ navigationController popViewControllerAnimated:NO];
    hasPushedGLView = NO;
}


/*
 ========================
 InitializeInterfaceBuilder 
 ========================
 */
- (void) InitializeInterfaceBuilder {
    
}

- (NSString*) GetNibNameForDevice:(NSString*) nibName
{
    NSString *extension = @"";
    
#if TARGET_OS_TV
        extension = @"-tvos";
#endif
    
    return [NSString stringWithFormat:@"%@%@", nibName, extension];
}

- (NSString*) GetFontName
{
    // To restore usage of the original font, un-comment out this line and comment out the next line
    //return @"idGinza Narrow";
    return @"Chicago";
}

@end

void ShowGLView( void ) {
	[ gAppDelegate ShowGLView ];
}

