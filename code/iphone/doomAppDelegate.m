/*
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.
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

#import "doomAppDelegate.h"
#import "EAGLView.h"
#import <AudioToolbox/AudioServices.h>
#include "../doomiphone.h"
#import <QuartzCore/CADisplayLink.h>
#import "SettingsMenuView.h"
#import "ControlsMenuView.h"
#include "IBGlue.h"
#import "MapMenuView.h"

@interface UIApplication (Private)

- (void)setSystemVolumeHUDEnabled:(BOOL)enabled forAudioCategory:(NSString *)category;
- (void)setSystemVolumeHUDEnabled:(BOOL)enabled;

- (void)runFrame;
- (void)asyncTic;

@end


char iphoneDocDirectory[1024];
char iphoneAppDirectory[1024];


@implementation gameAppDelegate

@synthesize window;
@synthesize glView;
@synthesize displayLink;

extern	EAGLContext *context;

gameAppDelegate * gAppDelegate = NULL;

NSTimer *animationTimer;
bool inBackgroundProcess = false;

touch_t		sysTouches[MAX_TOUCHES];
touch_t		gameTouches[MAX_TOUCHES];
pthread_mutex_t	eventMutex;		// used to sync between game and event threads
bool        firstRun = true;

pthread_t gameThreadHandle;
volatile boolean startupCompleted;
void *GameThread( void *args ) {
	if ( ![EAGLContext setCurrentContext:context]) {
		printf( "Couldn't setCurrentContext for game thread\n" );
		exit( 1 );
	}
	
    while( inBackgroundProcess ) {
         usleep( 1000 );   
    }
    
	printf( "original game thread priority: %f\n", (float)[NSThread threadPriority] );
	[NSThread setThreadPriority: 0.5];
	printf( "new game thread priority: %f\n", (float)[NSThread threadPriority] );
	    
	iphoneStartup();

	// make sure one frame has been run before setting
	// startupCompleted, so we don't get one grey frame
	iphoneFrame();
	
	startupCompleted = TRUE;	// OK to start touch / accel callbacks
	while( 1 ) {
        
        // we are in the background.. dont do anything.
        if( inBackgroundProcess ) {
            usleep( 1000 );
        }
		iphoneFrame();
	}
}

- (void)asyncTic {
	iphoneAsyncTic();
	[ self restartAccelerometerIfNeeded];
}

- (void)runFrame {
    iphoneAsyncTic(); 
	iphoneFrame();
}

- (void)applicationDidFinishLaunching:(UIApplication *)application {
    inBackgroundProcess = false;
	application.statusBarHidden = YES;
	application.statusBarOrientation = UIInterfaceOrientationLandscapeLeft;
	gAppDelegate = self;
    
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
	
	// disable screen dimming
	[UIApplication sharedApplication].idleTimerDisabled = YES;
	
    // Add the Main Menu as the SubView
    [self MainMenu];
    
	// start the flow of accelerometer events
	UIAccelerometer *accelerometer = [UIAccelerometer sharedAccelerometer];
	accelerometer.delegate = self;
	accelerometer.updateInterval = 1.0f / 30.0f;

	// use this mutex for coordinating touch handling between
	// the run loop thread and the game thread
	if ( pthread_mutex_init( &eventMutex, NULL ) == -1 ) {
		perror( "pthread_mutex_init" );
	}
	
	// use this semaphore for signaling from the async cmd generation thread that
	// the game / draw thread can wake up

	// sem_init is unimplemented on iPhone
	//if ( sem_init( &ticSemaphore, 0, 0 ) == -1 ) {
	//	perror( "sem_init" );
	//}
	ticSemaphore = sem_open( "ticSemaphore", O_CREAT, S_IRWXU, 0 );
	if ( ticSemaphore == SEM_FAILED ) {
		perror( "sem_open" );
	}
	
	// we want the main (event/async) thread to be as high a priority as possible
	// so the game/render thread will be interrupted immediately.
	// It looks like the default scheduling on iPhone is already what we want --
	// the main thread is at 1.0, and new threads are at 0.5.
	printf( "original event thread priority: %f\n", (float)[NSThread threadPriority] );
	[NSThread setThreadPriority: 1.0];
	printf( "new event thread priority: %f\n", (float)[NSThread threadPriority] );
	
    // do all the game startup work
	iphoneStartup();
    
    int animationFrameInterval = 2;
    CADisplayLink *aDisplayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(runFrame)];
    [aDisplayLink setFrameInterval:animationFrameInterval];
    [aDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    self.displayLink = aDisplayLink;
    aDisplayLink.paused = YES;
    
    
    startupCompleted = TRUE;	// OK to start touch / accel callbacks
}


- (void)applicationWillResignActive:(UIApplication *)application {
    displayLink.paused = YES;
    inBackgroundProcess = YES;
    iphonePauseMusic();
    iphoneShutdown();
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    displayLink.paused = NO;
    inBackgroundProcess = NO;
    
    if( IBMenuVisible && !firstRun ) {
        iphonePlayMusic( "intro" );
    }
    
    firstRun = false;
}

- (void)applicationWillTerminate:(UIApplication *)application {
    iphoneStopMusic();
	iphoneShutdown();
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
	Com_Printf( "applicationDidReceiveMemoryWarning\n" );
}



- (void)dealloc {
	[window release];
	[glView release];
	[super dealloc];
}

- (void)restartAccelerometerIfNeeded {

	// I have no idea why this seems to happen sometimes...
	if ( SysIphoneMilliseconds() - lastAccelUpdateMsec > 1000 ) {
		static int count;
		if ( ++count < 100 ) {
			printf( "Restarting accelerometer updates.\n" );
		}
		UIAccelerometer *accelerometer = [UIAccelerometer sharedAccelerometer];
		accelerometer.delegate = self;
		accelerometer.updateInterval = 0.01;
	}
}

- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration
{	
	float acc[4];
	acc[0] = acceleration.x;
	acc[1] = acceleration.y;
	acc[2] = acceleration.z;
	acc[3] = acceleration.timestamp;
	iphoneTiltEvent( acc );
	lastAccelUpdateMsec = SysIphoneMilliseconds();
}

- (void) PrepareForViewSwap {
    
    [ mainMenuViewController.view removeFromSuperview ];
    [ mapMenuViewController.view removeFromSuperview ];
    [ creditsMenuViewController.view removeFromSuperview ];
    [ legalMenuViewController.view removeFromSuperview ];
    [ settingsMenuViewController.view removeFromSuperview ];
    [ controlsMenuViewController.view removeFromSuperview ];
    [ episodeMenuViewController.view removeFromSuperview ];
}

- (void) ResumeGame {
    
    ResumeGame();
    
    // Switch to the Game View.
    [window addSubview:glView];
    [window makeKeyAndVisible];
    
    displayLink.paused = NO;
    IBMenuVisible = NO;
}

- (void) MainMenu {
    
    [self PrepareForViewSwap];
    
    // Switch to the Game View.
    [window addSubview: mainMenuViewController.view];
    [window makeKeyAndVisible];
    iphonePauseMusic();
    
    displayLink.paused = YES;
    IBMenuVisible = YES;
}

- (void) DemoGame {
    
    StartDemoGame( false );
    
    // Switch to the Game View.
    [window addSubview:glView];
    [window makeKeyAndVisible];
    
    displayLink.paused = NO;
    IBMenuVisible = NO;
}

- (void) NewGame {
    
    [self PrepareForViewSwap];
    
    // Switch to the Game View.
    [window addSubview: episodeMenuViewController.view];
    [window makeKeyAndVisible];
    
    displayLink.paused = YES;
    IBMenuVisible = YES;
    
}

- (void) playMap: (int) dataset: (int) episode: (int) map: (int) skill {
    mapStart_t startmap;
    
    startmap.map = map;
    startmap.episode = episode;
    startmap.dataset = dataset;
    startmap.skill = skill;
    
    StartSinglePlayerGame( startmap );
    
    [self HideIB];
}

- (void) CreditsMenu {
    
    [self PrepareForViewSwap];
    
    // Switch to the Game View.
    [window addSubview: creditsMenuViewController.view];
    [window makeKeyAndVisible];
    
    displayLink.paused = YES;
    IBMenuVisible = YES;
    
}

- (void) LegalMenu {
    
    [self PrepareForViewSwap];
    
    // Switch to the Game View.
    [window addSubview: legalMenuViewController.view];
    [window makeKeyAndVisible];
    
    displayLink.paused = YES;
    IBMenuVisible = YES;
    
}

- (void) GotoSupport {
    
    SysIPhoneOpenURL("http://www.idsoftware.com/doom-classic/index.html");
    
}

- (void) idSoftwareApps {
    
    SysIPhoneOpenURL("http://itunes.com/apps/idsoftware");
}

- (void) ControlsMenu {
    
    [self PrepareForViewSwap];
    
    ControlsMenuView * menu = controlsMenuViewController.view;
    [ menu SetOptions];
    
    // Switch to the Game View.
    [window addSubview: controlsMenuViewController.view];
    [window makeKeyAndVisible];
    
    displayLink.paused = YES;
    IBMenuVisible = YES;
    
}

- (void) SettingsMenu {
    
    [self PrepareForViewSwap];
    
    SettingsMenuView * menu = settingsMenuViewController.view;
    [ menu resetSwitches];
    
    // Switch to the Game View.
    [window addSubview: settingsMenuViewController.view];
    [window makeKeyAndVisible];
    
    displayLink.paused = YES;
    IBMenuVisible = YES;
    
}

- (void) HUDLayout {
    
    menuState = IPM_HUDEDIT;
    
    [self HideIB];
}

- (void) HideIB {
    
     [self PrepareForViewSwap];
    
    // Switch to the Game View.
    [window addSubview:glView];
    [window makeKeyAndVisible];
    
    displayLink.paused = NO;
    IBMenuVisible = NO;
}

- (void) SelectEpisode: (int) episode {
    
    [self PrepareForViewSwap];
    
    [ (MapMenuView*)mapMenuViewController.view setEpisode: episode ];
    
    // Switch to the Game View.
    [window addSubview: mapMenuViewController.view];
    [window makeKeyAndVisible];
    
    displayLink.paused = YES;
    IBMenuVisible = YES;
    
}

@end



