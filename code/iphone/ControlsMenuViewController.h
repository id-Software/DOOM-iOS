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
#import <UIKit/UIKit.h>

/*
 ================================================================================================
 ControlsMenuViewController
 
 ================================================================================================
 */
@interface Doom_ControlsMenuViewController : UIViewController {
    
    IBOutlet UISlider * movestickSize;
    IBOutlet UISlider * turnstickSize;
    IBOutlet UISlider * tiltMoveSpeed;
    IBOutlet UISlider * tiltTurnSpeed;
    
    
    IBOutlet UIButton * singleThumbButton;
    IBOutlet UIButton * dualThumbButton;
    IBOutlet UIButton * dirWheelButton;
    
}


- (void) SetupSlider:(UISlider*)slider minimumTrack:(UIImage*)minImage
        maximumTrack:(UIImage*)maxImage
               thumb:(UIImage*)thumbImage;

- (IBAction) BackToMain;
- (IBAction) HudLayoutPressed;
- (IBAction) SingleThumbpadPressed;
- (IBAction) DualThumbpadPressed; 
- (IBAction) DirWheelPressed; 

- (IBAction) MoveStickValChanged;
- (IBAction) TurnStickValChanged;
- (IBAction) TiltMoveValChanged;
- (IBAction) TiltTurnValChanged;

@end
