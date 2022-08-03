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
#import "SettingsMenuViewController.h"
#include "doomiphone.h"
#include "iphone_delegate.h"

/*
 ================================================================================================
 Doom_SettingsMenuViewController
 
 ================================================================================================
 */
@implementation Doom_SettingsMenuViewController

UIFocusGuide *focusGuide;

/*
 ========================
 Doom_SettingsMenuViewController::initWithNibName
 ========================
 */
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

/*
 ========================
 Doom_SettingsMenuViewController::didReceiveMemoryWarning
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
 Doom_SettingsMenuViewController::viewDidLoad
 ========================
 */
- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [self setValues];
    
#if TARGET_OS_TV
    focusGuide = [[UIFocusGuide alloc] init];
    [self.view addLayoutGuide:focusGuide];
    
    [focusGuide.widthAnchor constraintEqualToAnchor:textMessageSwitch.widthAnchor].active = YES;
    [focusGuide.heightAnchor constraintEqualToAnchor:resetButton.heightAnchor].active = YES;
    [focusGuide.topAnchor constraintEqualToAnchor:resetButton.topAnchor].active = YES;
    focusGuide.preferredFocusEnvironments = @[resetButton];

#endif
}

/*
 ========================
 Doom_SettingsMenuViewController::BackToMain
 ========================
 */
- (IBAction) BackToMain {
    [self.navigationController popViewControllerAnimated:NO];    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}

- (void)setValues {
    [ autoUseSwitch setOn: (BOOL)autoUse->value ];
    [ statusbarSwitch setOn: (BOOL)statusBar->value ];
    [ touchclickSwitch setOn: (BOOL)touchClick->value ];
    [ textMessageSwitch setOn: (BOOL)messages->value ];
    [ drawControlsSwitch setOn: (BOOL)drawControls->value ];
    [ musicSwitch setOn: (BOOL)music->value ];
    [ centerSticksSwitch setOn: (BOOL)centerSticks->value ];
    [ rampTurnSwitch setOn: (BOOL)rampTurn->value ];
    
#if TARGET_OS_TV
    // This may be a bad way to do it but it lets me leave them as idSwitches instead of changing the type
    [ autoUseSwitch setImage:(BOOL)autoUse->value ? [UIImage imageNamed:@"SettingsButton_Highlighted"] : [UIImage imageNamed:@"SettingsButton"]  forState:UIControlStateFocused];
    [ statusbarSwitch setImage:(BOOL)statusBar->value ? [UIImage imageNamed:@"SettingsButton_Highlighted"] : [UIImage imageNamed:@"SettingsButton"]  forState:UIControlStateFocused];
    [ touchclickSwitch setImage:(BOOL)touchClick->value ? [UIImage imageNamed:@"SettingsButton_Highlighted"] : [UIImage imageNamed:@"SettingsButton"]  forState:UIControlStateFocused];
    [ textMessageSwitch setImage:(BOOL)messages->value ? [UIImage imageNamed:@"SettingsButton_Highlighted"] : [UIImage imageNamed:@"SettingsButton"]  forState:UIControlStateFocused];
    [ drawControlsSwitch setImage:(BOOL)drawControls->value ? [UIImage imageNamed:@"SettingsButton_Highlighted"] : [UIImage imageNamed:@"SettingsButton"]  forState:UIControlStateFocused];
    [ musicSwitch setImage:(BOOL)music->value ? [UIImage imageNamed:@"SettingsButton_Highlighted"] : [UIImage imageNamed:@"SettingsButton"]  forState:UIControlStateFocused];
    [ centerSticksSwitch setImage:(BOOL)centerSticks->value ? [UIImage imageNamed:@"SettingsButton_Highlighted"] : [UIImage imageNamed:@"SettingsButton"]  forState:UIControlStateFocused];
    [ rampTurnSwitch setImage:(BOOL)rampTurn->value ? [UIImage imageNamed:@"SettingsButton_Highlighted"] : [UIImage imageNamed:@"SettingsButton"]  forState:UIControlStateFocused];

#endif
}

/*
 ========================
 Doom_SettingsMenuViewController::ResetToDefaults
 ========================
 */
- (IBAction) ResetToDefaults {
    
    // reset all cvars
    Cvar_Reset_f();
    HudSetForScheme(0);
    iphoneStartMusic();
    
	mus_on = true;
	mus_pause_opt = 1;
	S_ResumeSound();
	
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
    
    [self setValues];
}

/*
 ========================
 Doom_SettingsMenuViewController::AutoUseChanged
 ========================
 */
- (IBAction) AutoUseChanged {
    Cvar_SetValue( autoUse->name, !autoUse->value );
//    NSLog(@"DOOM: AutoUseChanged: %f", autoUse->value);
    [self setValues];
}

/*
 ========================
 Doom_SettingsMenuViewController::StatusBarChanged
 ========================
 */
- (IBAction) StatusBarChanged {
    Cvar_SetValue( statusBar->name, !statusBar->value );
//    NSLog(@"DOOM: StatusBarChanged: %f", statusBar->value);
    [self setValues];
}

/*
 ========================
 Doom_SettingsMenuViewController::TouchClickChanged
 ========================
 */
- (IBAction) TouchClickChanged {
    Cvar_SetValue( touchClick->name, !touchClick->value );
//    NSLog(@"DOOM: TouchClickChanged: %f", touchClick->value);
    [self setValues];
}

/*
 ========================
 Doom_SettingsMenuViewController::TextMessagesChanged
 ========================
 */
- (IBAction) TextMessagesChanged {
    Cvar_SetValue( messages->name, !messages->value );
//    NSLog(@"DOOM: TextMessagesChanged: %f", messages->value);
    [self setValues];
}

/*
 ========================
 Doom_SettingsMenuViewController::DrawControlsChanged
 ========================
 */
- (IBAction) DrawControlsChanged {
    Cvar_SetValue( drawControls->name, !drawControls->value );
//    NSLog(@"DOOM: DrawControlsChanged: %f", drawControls->value);
    [self setValues];
}
extern int mus_pause_opt; // From m_misc.c
extern bool mus_on;

/*
 ========================
 Doom_SettingsMenuViewController::MusicChanged
 ========================
 */
- (IBAction) MusicChanged {
    if ( !SysIPhoneOtherAudioIsPlaying() ) {
        Cvar_SetValue( music->name, !music->value );
//        NSLog(@"DOOM: MusicChanged: %f", music->value);
        [self setValues];
        if ( music->value ) {
			mus_on = true;
			mus_pause_opt = 1;
			S_ResumeSound();
        } else {
			mus_on = false;
			mus_pause_opt = 1;
			S_PauseSound();
        }
    }
    
}

/*
 ========================
 Doom_SettingsMenuViewController::CenterSticksChanged
 ========================
 */
- (IBAction) CenterSticksChanged {
    Cvar_SetValue( centerSticks->name, !centerSticks->value );
//    NSLog(@"DOOM: CenterSticksChanged: %f", centerSticks->value);
    [self setValues];
}

/*
 ========================
 Doom_SettingsMenuViewController::RampTurnChanged
 ========================
 */
- (IBAction) RampTurnChanged {
    Cvar_SetValue( rampTurn->name, !rampTurn->value );
//    NSLog(@"DOOM: RampTurnChanged: %f", rampTurn->value);
    [self setValues];
}

#if TARGET_OS_TV

- (void)didUpdateFocusInContext:(UIFocusUpdateContext *)context withAnimationCoordinator:(UIFocusAnimationCoordinator *)coordinator {
    NSLog(@"DOOM: %@", context.nextFocusedView);
    NSLog(@"DOOM: %@", context.previouslyFocusedView);

    [super didUpdateFocusInContext:context withAnimationCoordinator:coordinator];
    
    if ([context.nextFocusedView isKindOfClass:[idSwitch class]]) {
        
        if (context.nextFocusedView.tag > 0) {
            
            autoUseSelection.hidden         = YES;
            statusbarSelection.hidden       = YES;
            touchclickSelection.hidden      = YES;
            textMessageSelection.hidden     = YES;
            drawControlsSelection.hidden    = YES;
            musicSelection.hidden           = YES;
            centerSticksSelection.hidden    = YES;
            rampTurnSelection.hidden        = YES;
            
            if (context.nextFocusedView.tag == 1) {
                autoUseSelection.hidden         = NO;
            } else if (context.nextFocusedView.tag == 2) {
                statusbarSelection.hidden       = NO;
            } else if (context.nextFocusedView.tag == 3) {
                touchclickSelection.hidden      = NO;
            } else if (context.nextFocusedView.tag == 4) {
                textMessageSelection.hidden     = NO;
            } else if (context.nextFocusedView.tag == 5) {
                drawControlsSelection.hidden    = NO;
            } else if (context.nextFocusedView.tag == 6) {
                musicSelection.hidden           = NO;
            } else if (context.nextFocusedView.tag == 7) {
                centerSticksSelection.hidden    = NO;
            } else if (context.nextFocusedView.tag == 8) {
                rampTurnSelection.hidden        = NO;
            }
        }
        
        [self setValues];
    }
}

#endif

@end
