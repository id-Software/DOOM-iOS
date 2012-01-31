/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */



#import "MainMenuView.h"
#import "doomAppDelegate.h"
#include "doomiphone.h"

@implementation Banner_SubItem



@end



@implementation Banner_SubMenu

- (void)awakeFromNib {
    isHidden = YES;
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hitView = [super hitTest:point withEvent:event];
    
    if (hitView != self) {
        return hitView;
    }
    
    return nil;
}

- (void) Hide {
    
    if( !isHidden ) {
    
        isHidden = YES;
        
        [UIView beginAnimations:@"Show" context:nil];
        [UIView setAnimationDuration:0.5f];
        [UIView setAnimationCurve: UIViewAnimationCurveEaseInOut];
        [UIView setAnimationBeginsFromCurrentState:NO];
        [UIView setAnimationDelegate:self];
        [UIView setAnimationDidStopSelector:@selector(Disable)];
        
        self.alpha = 1.0f;
        [ self viewWithTag: 0 ].alpha = 0.0f;
        
        [UIView commitAnimations];
    
    }
}

- (void) Show {
    
    if( isHidden ) {
    
        isHidden = NO;
        
        [UIView beginAnimations:@"Show" context:nil];
        [UIView setAnimationDuration:0.5f];
        [UIView setAnimationCurve: UIViewAnimationCurveEaseInOut];
        [UIView setAnimationBeginsFromCurrentState:NO];
        [UIView setAnimationDelegate:self];
        [UIView setAnimationDidStopSelector:@selector(Enable)];
        
        self.alpha = 1.0f;
        [ self viewWithTag: 0 ].alpha = 1.0f;
        
        [UIView commitAnimations];
    }
}

@end




@implementation MainMenuView

- (void) initialize{
    
    // Hide Everything.
    [self ResetMenu];
    
}

- (void)awakeFromNib {
    
    [self initialize];
    
}
               
- (void) ResetMenu {
    

}


- (IBAction) ResumeGamePressed {
    
    [ gAppDelegate ResumeGame ];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

- (IBAction) NewGamePressed {
    
    // Go to the Map Menu.
    [gAppDelegate NewGame];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
    
}

- (IBAction) MultiplayerPressed {
    
    // Go to the MP Menu.
    // get the address for the local service, which may
    // start up a bluetooth personal area network
    boolean serverResolved = ResolveNetworkServer( &netServer.address );
    
    // open our socket now that the network interfaces have been configured
    // Explicitly open on interface 1, which is en0.  If bluetooth ever starts
    // working better, we can handle multiple interfaces.
    if ( gameSocket <= 0 ) {
        gameSocket = UDPSocket( "en0", DOOM_PORT );
    }
    
    // get the address for the local service
    if ( !serverResolved ) {
        // nobody else is acting as a server, so start one here
        RegisterGameService();
        SetupEmptyNetGame();
    }	
	
    menuState = IPM_MULTIPLAYER;
    
    [gAppDelegate HideIB];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
    
}

- (IBAction) CreditsPressed {
    
    [gAppDelegate CreditsMenu];
        
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

- (IBAction) SupportPressed {
    
    [gAppDelegate GotoSupport];
        
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

- (IBAction) LegalPressed {
    
    [gAppDelegate LegalMenu];
        
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

- (IBAction) DemoPressed {
    [gAppDelegate DemoGame ];
        
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

- (IBAction) OtherIdGamesPressed {
    
    [gAppDelegate idSoftwareApps];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

- (IBAction) ControlsOptionsPressed {
    
    [gAppDelegate ControlsMenu];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
    
}

- (IBAction) SettingsOptionsPressed {
    
    [gAppDelegate SettingsMenu ];
    
    Sound_StartLocalSound( "iphone/baborted_01.wav" );
}

- (void) ShowPlayBanner {
    
    [ mPlayButton setEnabled: NO ];
    [ mSettingsButton setEnabled: YES ];
    [ mAboutButton setEnabled: YES ];
    [ mExtrasButton setEnabled: YES ];
    
    [ mPlaySubMenu Show ];
    [ mSettingsSubMenu Hide ];
    [ mExtrasSubMenu Hide ];
    [ mAboutSubMenu Hide ];
    
}

- (void) ShowSettingsBanner {
    
    [ mPlayButton setEnabled: YES ];
    [ mSettingsButton setEnabled: NO ];
    [ mAboutButton setEnabled: YES ];
    [ mExtrasButton setEnabled: YES ];
    
    [ mSettingsSubMenu Show ];
    [ mPlaySubMenu Hide ];
    [ mExtrasSubMenu Hide ];
    [ mAboutSubMenu Hide ];
}

- (void) ShowAboutBanner {
    
    [ mPlayButton setEnabled: YES ];
    [ mSettingsButton setEnabled: YES ];
    [ mAboutButton setEnabled: NO ];
    [ mExtrasButton setEnabled: YES ];
    
    [ mAboutSubMenu Show ];
    [ mPlaySubMenu Hide ];
    [ mSettingsSubMenu Hide ];
    [ mExtrasSubMenu Hide ];
}

- (void) ShowExtrasBanner {
    
    [ mPlayButton setEnabled: YES ];
    [ mSettingsButton setEnabled: YES ];
    [ mAboutButton setEnabled: YES ];
    [ mExtrasButton setEnabled: NO ];
    
    [ mExtrasSubMenu Show ];
    [ mPlaySubMenu Hide ];
    [ mSettingsSubMenu Hide ];
    [ mAboutSubMenu Hide ];
}

@end
