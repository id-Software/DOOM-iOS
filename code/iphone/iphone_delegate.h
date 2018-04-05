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
#import <UIKit/UIAccelerometer.h>
#import "iphone_glViewController.h"

@class EAGLView;

@interface iphoneApp : NSObject <UIApplicationDelegate, UIAccelerometerDelegate> {
    
    UIWindow *                  window;                 // Main Application Window.
    UINavigationController *    navigationController;   // Our View Stack
    iphone_glViewController  *  openGLViewController;   // our OpenGL (Game) View
	BOOL						hasPushedGLView;		// Keep track of whether the GL view
														// is on top of the stack, for the
														// hacked delay used to hide the first
														// frame of garbage.
}

@property (nonatomic, retain) IBOutlet UIWindow *window;

extern iphoneApp * gAppDelegate;

- (void) InitializeInterfaceBuilder;

- (void) HACK_PushController;
- (void) ShowGLView;
- (void) HideGLView;
- (void) PopGLView;

@end

