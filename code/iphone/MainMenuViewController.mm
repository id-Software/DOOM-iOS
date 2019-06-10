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

#import "MainMenuViewController.h"
#include "iphone_delegate.h"
#include "doomiphone.h"
#import "EpisodeMenuViewController.h"
#import "MissionMenuViewController.h"
#import "CreditsMenuViewController.h"
#import "SettingsMenuViewController.h"
#import "ControlsMenuViewController.h"
#import "LegalMenuViewController.h"

/*
 ================================================================================================
 Doom Sub Menu Banner Interface object
 ================================================================================================
 */

@implementation Banner_SubItem
@end

@implementation Banner_SubMenu

/*
 ========================
 Banner_SubMenu::awakeFromNib
 ========================
 */
- (void)awakeFromNib {
    [super awakeFromNib];
    isHidden = YES;
    
    sigilShredsLoaded = false;
    
#if GAME_DOOM
    iphoneIWADSelect("doom.wad");
#endif
    
#if GAME_DOOM2
    iphoneIWADSelect("doom2.wad");
#endif
    
#if GAME_FINALDOOM
    // just picking a wad file to make the startup happy
    iphoneIWADSelect("tnt.wad");
#endif
    
#if GAME_SIGIL
    iphoneIWADSelect("doom.wad");
    iphoneClearPWADs();
    iphonePWADAdd("SIGIL.wad");
    
    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"SIGIL_SHREDS"
                                                         ofType:@"wad"];
    
    NSFileManager *fileManager = [NSFileManager defaultManager];

    if ([fileManager fileExistsAtPath:filePath]){
        iphonePWADAdd("SIGIL_SHREDS.wad");
        sigilShredsLoaded = true;
    }
    
#endif
    
    if( !didInit ) {
        char full_iwad[1024];
        
//        doom_iwad = strdup(Cvar_VariableString("iwadSelection"));
//        doom_pwads = strdup(Cvar_VariableString("pwadSelection"));
        
        I_FindFile( doom_iwad, ".wad", full_iwad );
        
        // fall back to default DOOM wad
        if( full_iwad[0] == '\0' ) {
            I_FindFile( "doom.wad", ".wad", full_iwad );
            if( doom_iwad ) free(doom_iwad);
            doom_iwad = strdup(full_iwad);
        } else if( strcmp(doom_iwad,full_iwad) != 0 ) {
            if( doom_iwad ) free(doom_iwad);
            doom_iwad = strdup( full_iwad );
        }
        
        iphoneDoomStartup();
        didInit = YES;
    }
}

/*
 ========================
 Banner_SubMenu::hitTest
 ========================
 */
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hitView = [super hitTest:point withEvent:event];
    
    if (hitView != self) {
        return hitView;
    }
    
    return nil;
}

/*
 ========================
 Banner_SubMenu::Hide
 ========================
 */
- (void) Hide {
    
    if( !isHidden ) {
        
        isHidden = YES;
        
        [UIView beginAnimations:@"Show" context:nil];
        [UIView setAnimationDuration:0.5f];
        [UIView setAnimationCurve: UIViewAnimationCurveEaseInOut];
        [UIView setAnimationBeginsFromCurrentState:NO];
        [UIView setAnimationDelegate:self];
        //[UIView setAnimationDidStopSelector:@selector(Disable)];
        
        self.alpha = 1.0f;
        [ self viewWithTag: 0 ].alpha = 0.0f;
        
        [UIView commitAnimations];
        
    }
}

/*
 ========================
 Banner_SubMenu::Show
 ========================
 */
- (void) Show {
    
    if( isHidden ) {
        
        isHidden = NO;
        
        [UIView beginAnimations:@"Show" context:nil];
        [UIView setAnimationDuration:0.5f];
        [UIView setAnimationCurve: UIViewAnimationCurveEaseInOut];
        [UIView setAnimationBeginsFromCurrentState:NO];
        [UIView setAnimationDelegate:self];
        //[UIView setAnimationDidStopSelector:@selector(Enable)];
        
        self.alpha = 1.0f;
        [ self viewWithTag: 0 ].alpha = 1.0f;
        
        [UIView commitAnimations];
    }
}

@end


/*
 ================================================================================================
 MainMenuViewController
 ================================================================================================
 */
@implementation Doom_MainMenuViewController

BOOL playMenuSelected = NO;
BOOL extrasMenuSelected = NO;
BOOL aboutMenuSelected = NO;
BOOL settingsMenuSelected = NO;

/*
 ========================
 MainMenuViewController::initWithNibName
 ========================
 */
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {

    }
    return self;
}

/*
 ========================
 MainMenuViewController::didReceiveMemoryWarning
 ========================
 */
- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

/*
 ========================
 MainMenuViewController::viewDidLoad
 ========================
 */
- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
}

/*
 ========================
 MainMenuViewController::ResumeGamePressed
 ========================
 */
- (IBAction) ResumeGamePressed {
    
    [ gAppDelegate ShowGLView ];
    
    ResumeGame();
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
 
}

/*
 ========================
 MainMenuViewController::NewGamePressed
 ========================
 */
- (IBAction) NewGamePressed {
    
#if GAME_DOOM2
    Doom_MissionMenuViewController *vc = [[Doom_MissionMenuViewController alloc] initWithNibName:[gAppDelegate GetNibNameForDevice:@"MissionMenuView"] bundle:nil];
#elif GAME_SIGIL
    Doom_MissionMenuViewController *vc = [[Doom_MissionMenuViewController alloc] initWithNibName:[gAppDelegate GetNibNameForDevice:@"MissionMenuView"] bundle:nil];
#else
    // Switch to episode view menu.
    Doom_EpisodeMenuViewController *vc = [[Doom_EpisodeMenuViewController alloc] initWithNibName:[gAppDelegate GetNibNameForDevice:@"EpisodeMenuView"] bundle:nil];
#endif
    
    [self.navigationController pushViewController:vc animated:NO];
#if GAME_DOOM2
    [vc setEpisode:0 ];
#elif GAME_SIGIL
    [vc setEpisode:4 ];
#endif
    [vc release];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
    
}

/*
 ========================
 MainMenuViewController::MultiplayerPressed
 ========================
 */
- (IBAction) MultiplayerPressed {
    
	
	//ShowMatchmaker( self, 2, 4 );
	return;
	
    // Go to the MP Menu.
    // get the address for the local service, which may
    // start up a bluetooth personal area network
    //bool serverResolved = ResolveNetworkServer( &netServer.address );
    
    // open our socket now that the network interfaces have been configured
    // Explicitly open on interface 1, which is en0.  If bluetooth ever starts
    // working better, we can handle multiple interfaces.
//    if ( gameSocket <= 0 ) {
//        gameSocket = UDPSocket( "en0", DOOM_PORT );
//    }
    
	/*
    // get the address for the local service
    if ( !serverResolved ) {
        // nobody else is acting as a server, so start one here
        RegisterGameService();
        SetupEmptyNetGame();
    }
	*/	
	
//    menuState = IPM_MULTIPLAYER;
//
//    [gAppDelegate ShowGLView];
//
//    Sound_StartLocalSound( "iphone/baborted_01.wav" );
    
}

/*
 ========================
 MainMenuViewController::CreditsPressed
 ========================
 */
- (IBAction) CreditsPressed {
    
    Doom_CreditsMenuViewController *vc = [[Doom_CreditsMenuViewController alloc] initWithNibName:[gAppDelegate GetNibNameForDevice:@"CreditsMenuView"] bundle:nil];

    [self.navigationController pushViewController:vc animated:NO];
    [vc release];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

/*
 ========================
 MainMenuViewController::SupportPressed
 ========================
 */
- (IBAction) SupportPressed {
    
    SysIPhoneOpenURL("http://www.idsoftware.com/doom-classic/index.html");
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

/*
 ========================
 MainMenuViewController::LegalPressed
 ========================
 */
- (IBAction) LegalPressed {
    
    Doom_LegalMenuViewController *vc = [[Doom_LegalMenuViewController alloc] initWithNibName:[gAppDelegate GetNibNameForDevice:@"LegalMenuView"] bundle:nil];

    [self.navigationController pushViewController:vc animated:NO];
    [vc release];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

/*
 ========================
 MainMenuViewController::DemoPressed
 ========================
 */
- (IBAction) DemoPressed {
   
    StartDemoGame( false );
    
    [gAppDelegate ShowGLView];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

/*
 ========================
 MainMenuViewController::OtherIdGamesPressed
 ========================
 */
- (IBAction) OtherIdGamesPressed {
    
    SysIPhoneOpenURL("http://itunes.com/apps/idsoftware");
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

/*
 ========================
 MainMenuViewController::ControlsOptionsPressed
 ========================
 */
- (IBAction) ControlsOptionsPressed {
    
    Doom_ControlsMenuViewController *vc = [[Doom_ControlsMenuViewController alloc] initWithNibName:[gAppDelegate GetNibNameForDevice:@"ControlsMenuView"] bundle:nil];

    [self.navigationController pushViewController:vc animated:NO];
    [vc release];

    Sound_StartLocalSound( "iphone/baborted_01.wav" );
    
}

/*
 ========================
 MainMenuViewController::SettingsOptionsPressed
 ========================
 */
- (IBAction) SettingsOptionsPressed {
    
    Doom_SettingsMenuViewController *vc = [[Doom_SettingsMenuViewController alloc] initWithNibName:[gAppDelegate GetNibNameForDevice:@"SettingsMenuView"] bundle:nil];

     [self.navigationController pushViewController:vc animated:NO];
     [vc release];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

/*
 ========================
 MainMenuViewController::ShowPlayBanner
 ========================
 */
- (void) ShowPlayBanner {
    
    [ mPlayButton setEnabled: NO ];
    [ mSettingsButton setEnabled: YES ];
    [ mAboutButton setEnabled: YES ];
    [ mExtrasButton setEnabled: YES ];
    
    [ mPlaySubMenu Show ];
    [ mSettingsSubMenu Hide ];
    [ mExtrasSubMenu Hide ];
    [ mAboutSubMenu Hide ];
    
#if TARGET_OS_TV
    playMenuSelected = YES;
    extrasMenuSelected = NO;
    settingsMenuSelected = NO;
    aboutMenuSelected = NO;
    [self setNeedsFocusUpdate];
    [self updateFocusIfNeeded];
#endif
    
}

/*
 ========================
 MainMenuViewController::ShowSettingsBanner
 ========================
 */
- (void) ShowSettingsBanner {
    
    [ mPlayButton setEnabled: YES ];
    [ mSettingsButton setEnabled: NO ];
    [ mAboutButton setEnabled: YES ];
    [ mExtrasButton setEnabled: YES ];
    
    [ mSettingsSubMenu Show ];
    [ mPlaySubMenu Hide ];
    [ mExtrasSubMenu Hide ];
    [ mAboutSubMenu Hide ];

#if TARGET_OS_TV
    playMenuSelected = NO;
    extrasMenuSelected = NO;
    settingsMenuSelected = YES;
    aboutMenuSelected = NO;
    [self setNeedsFocusUpdate];
    [self updateFocusIfNeeded];
#endif
}

/*
 ========================
 MainMenuViewController::ShowAboutBanner
 ========================
 */
- (void) ShowAboutBanner {
    
    [ mPlayButton setEnabled: YES ];
    [ mSettingsButton setEnabled: YES ];
    [ mAboutButton setEnabled: NO ];
    [ mExtrasButton setEnabled: YES ];
    
    [ mAboutSubMenu Show ];
    [ mPlaySubMenu Hide ];
    [ mSettingsSubMenu Hide ];
    [ mExtrasSubMenu Hide ];
    
#if TARGET_OS_TV
    playMenuSelected = NO;
    extrasMenuSelected = NO;
    settingsMenuSelected = NO;
    aboutMenuSelected = YES;
    [self setNeedsFocusUpdate];
    [self updateFocusIfNeeded];
#endif

}

/*
 ========================
 MainMenuViewController::ShowExtrasBanner
 ========================
 */
- (void) ShowExtrasBanner {
    
//    Doom_CreditsMenuViewController *vc = nil;
//
//    vc = [[Doom_CreditsMenuViewController alloc] initWithNibName:@"CreditsMenuView" bundle:nil];
//
//    [self.navigationController pushViewController:vc animated:NO];
//    [vc release];
//
//    Sound_StartLocalSound( "iphone/baborted_01.wav" );
    
    [ mPlayButton setEnabled: YES ];
    [ mSettingsButton setEnabled: YES ];
    [ mAboutButton setEnabled: YES ];
    [ mExtrasButton setEnabled: NO ];
    
    [ mSettingsSubMenu Hide ];
    [ mPlaySubMenu Hide ];
    [ mExtrasSubMenu Show ];
    [ mAboutSubMenu Hide ];
    
    
#if TARGET_OS_TV
    playMenuSelected = NO;
    extrasMenuSelected = YES;
    settingsMenuSelected = NO;
    aboutMenuSelected = NO;
    [self setNeedsFocusUpdate];
    [self updateFocusIfNeeded];
#endif

}

#if TARGET_OS_TV
-(NSArray<id<UIFocusEnvironment>> *)preferredFocusEnvironments {
    if (playMenuSelected) {
        return @[mPlaySubMenu];
    } else if (extrasMenuSelected) {
        return @[mExtrasSubMenu];
    } else if (settingsMenuSelected) {
        return @[mSettingsSubMenu];
    } else if (aboutMenuSelected) {
        return @[mAboutSubMenu];
    } else {
        return @[mPlayButton];
    }
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress* press in presses) {
        switch (press.type) {
            case UIPressTypeMenu:
                if (playMenuSelected || extrasMenuSelected || settingsMenuSelected || aboutMenuSelected) {
                    break;
                } else {
                    [super pressesBegan: presses withEvent: event];
                }
                default:
                    [super pressesBegan: presses withEvent: event];
                break;
        }
    }
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress* press in presses) {
        switch (press.type) {
            case UIPressTypeMenu:
                if (playMenuSelected) {
                    playMenuSelected = NO;
                    [mPlaySubMenu Hide];
                    [self setNeedsFocusUpdate];
                    [self updateFocusIfNeeded];
                } else if (extrasMenuSelected) {
                    extrasMenuSelected = NO;
                    [mExtrasSubMenu Hide];
                    [self setNeedsFocusUpdate];
                    [self updateFocusIfNeeded];
                } else if (settingsMenuSelected) {
                    settingsMenuSelected = NO;
                    [mSettingsSubMenu Hide];
                    [self setNeedsFocusUpdate];
                    [self updateFocusIfNeeded];
                } else if (aboutMenuSelected) {
                    aboutMenuSelected = NO;
                    [mAboutSubMenu Hide];
                    [self setNeedsFocusUpdate];
                    [self updateFocusIfNeeded];
                } else {
                    [super pressesEnded: presses withEvent: event];
                }
                break;
            default:
                [super pressesEnded: presses withEvent: event];
                break;
        }
    }
}

#endif

@end
