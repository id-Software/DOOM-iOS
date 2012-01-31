/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */



#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "UICustomSwitch.h"

@interface SettingsMenuView : UIImageView {

    IBOutlet UICustomSwitch * autoUseSwitch;
    IBOutlet UICustomSwitch * statusbarSwitch;
    IBOutlet UICustomSwitch * touchclickSwitch;
    IBOutlet UICustomSwitch * textMessageSwitch;
    IBOutlet UICustomSwitch * drawControlsSwitch;
    IBOutlet UICustomSwitch * musicSwitch;
    IBOutlet UICustomSwitch * centerSticksSwitch;
    IBOutlet UICustomSwitch * rampTurnSwitch;
    

    
}

- (void)   resetSwitches;

- (IBAction) BackToMain;
- (IBAction) ResetToDefaults;
- (IBAction) AutoUseChanged;
- (IBAction) StatusBarChanged;
- (IBAction) TouchClickChanged;
- (IBAction) TextMessagesChanged;
- (IBAction) DrawControlsChanged;
- (IBAction) MusicChanged;
- (IBAction) CenterSticksChanged;
- (IBAction) RampTurnChanged;

@end
