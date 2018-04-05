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


@interface iphone_glViewController : UIViewController {
    
    CADisplayLink *displayLink;
    
}

@property (nonatomic, retain) IBOutlet CADisplayLink *displayLink;

- (void) StartDisplay;
- (void) StopDisplay;
- (bool) IsDisplaying;

// JDS revised orientation code
-(BOOL)shouldAutorotate;
- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation;
- (UIInterfaceOrientationMask)supportedInterfaceOrientations;

@end
