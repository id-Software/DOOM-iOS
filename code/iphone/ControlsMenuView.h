/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */



#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

@interface ControlsMenuView : UIView {

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

- (void)     SetOptions;
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
