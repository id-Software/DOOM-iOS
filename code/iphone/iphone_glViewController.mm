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

#import "iphone_glViewController.h"
#import "EAGLView.h"
#import <QuartzCore/CADisplayLink.h>
#include "doomiphone.h"
#include "iphone_delegate.h"

#if TARGET_OS_TV
const static int   DISPLAY_LINK_FRAME_INTERVAL = 30;
#else
const static int   DISPLAY_LINK_FRAME_INTERVAL = 2;
#endif

// Need one buffer frame when transitioning from IB menus to the OpenGL game view.
// Otherwise, occasionally the IB view stays onscreen during the Doom loading frame.
// This seems to make precaching take way to long (about a whole minute).
// This flag will be set to true in StartDisplay, and reset to false after one display link
// frame has fired. 
static bool inTransition = false;

@implementation iphone_glViewController

@synthesize displayLink;

/*
 ========================
 runFrame
 ========================
 */
- (void)runFrame {
    
	// Skip the frist frame after coming from IB menus, to give the view a chance to switch to
	// OpenGL. For some reason, not doing this causes precaching to take way too long.
	if ( inTransition ) {
		inTransition = false;
		return;
	}
	
    // Update the Game
    iphoneAsyncTic(); 
    
    // Render the Game.
	iphoneFrame();
}

/*
 ========================
 initWithNibName
 ========================
 */
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        
        // Create the OpenGL View.
#if TARGET_OS_TV
        EAGLView *glView = [[EAGLView alloc] initWithFrame:[UIScreen mainScreen].bounds];
#else
        EAGLView *glView = [[EAGLView alloc] initWithFrame:[UIScreen mainScreen].bounds];
#endif
        self.view = glView;
        [glView release];
        
        
        // Setup the Display Link
        CADisplayLink *aDisplayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(runFrame)];
#if TARGET_OS_TV
        aDisplayLink.preferredFramesPerSecond= DISPLAY_LINK_FRAME_INTERVAL;
#else
        [ aDisplayLink setFrameInterval: DISPLAY_LINK_FRAME_INTERVAL];
#endif
        [ aDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        [ self setDisplayLink: aDisplayLink ];
        
    }
    return self;
}

/*
========================
shouldAutorotateToInterfaceOrientation
========================

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	// Return YES for supported orientations.
	return UIInterfaceOrientationIsLandscape(interfaceOrientation);
}
*/

#if !TARGET_OS_TV
- (BOOL)shouldAutorotate {
    return NO;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation {
    return UIInterfaceOrientationLandscapeRight;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskLandscape;
}
#endif


/*
 ========================
 viewDidLoad
 ========================
 */
- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Stop the Display link.
    [self.displayLink invalidate];
    self.displayLink = nil;
}


/*
 ========================
 didReceiveMemoryWarning
 ========================
 */
- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
}

/*
 ========================
 viewDidUnload
 ========================
 */
/*
- (void)viewDidUnload {
    [super viewDidUnload];
}
*/

/*
 ========================
 dealloc
 ========================
 */
- (void)dealloc {
    [super dealloc];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
#if TARGET_OS_TV
    [ gAppDelegate HideGLView ];
#endif
}

/*
 ========================
 StartDisplay
 ========================
 */
- (void) StartDisplay {
	inTransition = true;
    displayLink.paused = NO;
}

/*
 ========================
 StopDisplay
 ========================
 */
- (void) StopDisplay {
    displayLink.paused = YES;
}

/*
 ========================
 IsDisplaying
 ========================
 */
- (bool) IsDisplaying {
    return displayLink.paused;
}

@end
