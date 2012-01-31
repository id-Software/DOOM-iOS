/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */



#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "UIFontLabel.h"
#import "UIFontButton.h"

@interface Banner_SubItem : UIFontButton {
@public

}
@end

@interface Banner_SubMenu: UIView {
@public
    
    BOOL isHidden;
}

- (void) Hide;
- (void) Show;

@end 

@interface MainMenuView : UIView {

@public
    
    IBOutlet UIFontButton *  mPlayButton;
    IBOutlet UIFontButton *  mSettingsButton;
    IBOutlet UIFontButton *  mAboutButton;
    IBOutlet UIFontButton *  mExtrasButton;
    
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

- (void) ResetMenu;

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
