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
#include "../ios/LabelButton.h"

/*
 ================================================================================================
    Doom Sub Menu Banner Interface object
 ================================================================================================
 */
@interface Banner_SubItem : idLabelButton {
@public
    
}
@end

@interface Banner_SubMenu: UIView {
@public
    
    BOOL isHidden;
    BOOL didInit;
}

- (void) Hide;
- (void) Show;

@end 

/*
 ================================================================================================
 MainMenuViewController
 
 ================================================================================================
*/
@interface Doom_MainMenuViewController : UIViewController {


    IBOutlet idLabelButton *  mPlayButton;
    IBOutlet idLabelButton *  mSettingsButton;
    IBOutlet idLabelButton *  mAboutButton;
    IBOutlet idLabelButton *  mExtrasButton;

    IBOutlet Banner_SubMenu * mPlaySubMenu;
    IBOutlet Banner_SubMenu * mSettingsSubMenu;
    IBOutlet Banner_SubMenu * mAboutSubMenu;
    IBOutlet Banner_SubMenu * mExtrasSubMenu;
}

// Sub Menu Banner Actions
- (IBAction) ShowPlayBanner;
- (IBAction) ShowSettingsBanner;
- (IBAction) ShowAboutBanner;
- (IBAction) ShowExtrasBanner;

// Interface Builder Actions  ( Connected through IB )
- (IBAction) ResumeGamePressed;
- (IBAction) NewGamePressed;
- (IBAction) MultiplayerPressed;
- (IBAction) CreditsPressed;
- (IBAction) SupportPressed;
- (IBAction) LegalPressed;
- (IBAction) DemoPressed;
- (IBAction) OtherIdGamesPressed;
- (IBAction) ControlsOptionsPressed;
- (IBAction) SettingsOptionsPressed;

@end
