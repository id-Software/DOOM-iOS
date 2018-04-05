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

#import "Doom_App.h"
#import "MainMenuViewController.h"
#import "MainNavController.h"
#include "doomiphone.h"

@implementation DoomApp

- (void) InitializeInterfaceBuilder {
    
    // Create the Main Menu View controller.
	
    Doom_MainMenuViewController *rootController = nil;
    rootController = [[Doom_MainMenuViewController alloc] initWithNibName:@"MainMenuView" bundle:nil];

    // Create a Navigation Controller for Pushing/Popping Views.
    navigationController = [[MainNavController alloc] initWithRootViewController:rootController];
    [navigationController setNavigationBarHidden:YES];
    [rootController release];
    
    // Create the OpenGLView so that our context is created. Don't push it on yet though.
    openGLViewController = [ [ iphone_glViewController alloc] initWithNibName:@"OpenGLView" bundle:nil ];
	
    [ openGLViewController StopDisplay];
}

@end
