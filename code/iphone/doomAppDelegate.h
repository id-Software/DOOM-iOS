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

#import <UIKit/UIKit.h>
#import <UIKit/UIAccelerometer.h>
#import "MenuViewController.h"

@class EAGLView;

@interface gameAppDelegate : NSObject <UIApplicationDelegate, UIAccelerometerDelegate> {
    UIWindow *window;
    EAGLView *glView;
    CADisplayLink * displayLink;
    
	int		lastAccelUpdateMsec;
    
    
    IBOutlet MenuViewController *   mainMenuViewController;
    IBOutlet MenuViewController *   mapMenuViewController;
    IBOutlet MenuViewController *   creditsMenuViewController;
    IBOutlet MenuViewController *   legalMenuViewController;
    IBOutlet MenuViewController *   settingsMenuViewController;
    IBOutlet MenuViewController *   controlsMenuViewController;
    IBOutlet MenuViewController *   episodeMenuViewController;
    
    BOOL    IBMenuVisible;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet EAGLView *glView;
@property (nonatomic, retain) IBOutlet CADisplayLink *displayLink;

- (void)restartAccelerometerIfNeeded;

- (void) SelectEpisode: (int) episode;
- (void) PrepareForViewSwap;
- (void) ResumeGame;
- (void) NewGame;
- (void) DemoGame;
- (void) MainMenu;
- (void) CreditsMenu;
- (void) LegalMenu;
- (void) playMap: (int) dataset: (int) episode: (int) map: (int) skill;
- (void) GotoSupport;
- (void) idSoftwareApps;
- (void) SettingsMenu;
- (void) ControlsMenu;
- (void) HUDLayout;
- (void) HideIB;

extern gameAppDelegate * gAppDelegate;

@end

