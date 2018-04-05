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
#import "../ios/Switch.h"

/*
 ================================================================================================
 Doom_SettingsMenuViewController
 
 ================================================================================================
 */
@interface Doom_SettingsMenuViewController : UIViewController {
    
    
    IBOutlet idSwitch * autoUseSwitch;
    IBOutlet idSwitch * statusbarSwitch;
    IBOutlet idSwitch * touchclickSwitch;
    IBOutlet idSwitch * textMessageSwitch;
    IBOutlet idSwitch * drawControlsSwitch;
    IBOutlet idSwitch * musicSwitch;
    IBOutlet idSwitch * centerSticksSwitch;
    IBOutlet idSwitch * rampTurnSwitch;
    

}

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
