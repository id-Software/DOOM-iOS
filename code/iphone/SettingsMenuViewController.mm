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
    
    [ autoUseSwitch setOn: (BOOL)autoUse->value ];
    [ statusbarSwitch setOn: (BOOL)statusBar->value ];
    [ touchclickSwitch setOn: (BOOL)touchClick->value ];
    [ textMessageSwitch setOn: (BOOL)messages->value ];
    [ drawControlsSwitch setOn: (BOOL)drawControls->value ];
    [ musicSwitch setOn: (BOOL)music->value ];
    [ centerSticksSwitch setOn: (BOOL)centerSticks->value ];
    [ rampTurnSwitch setOn: (BOOL)rampTurn->value ];
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
    
    [ autoUseSwitch setOn: (BOOL)autoUse->value ];
    [ statusbarSwitch setOn: (BOOL)statusBar->value ];
    [ touchclickSwitch setOn: (BOOL)touchClick->value ];
    [ textMessageSwitch setOn: (BOOL)messages->value ];
    [ drawControlsSwitch setOn: (BOOL)drawControls->value ];
    [ musicSwitch setOn: (BOOL)music->value ];
    [ centerSticksSwitch setOn: (BOOL)centerSticks->value ];
    [ rampTurnSwitch setOn: (BOOL)rampTurn->value ];
}

/*
 ========================
 Doom_SettingsMenuViewController::AutoUseChanged
 ========================
 */
- (IBAction) AutoUseChanged {
    Cvar_SetValue( autoUse->name, !autoUse->value );
}

/*
 ========================
 Doom_SettingsMenuViewController::StatusBarChanged
 ========================
 */
- (IBAction) StatusBarChanged {
    Cvar_SetValue( statusBar->name, !statusBar->value );
}

/*
 ========================
 Doom_SettingsMenuViewController::TouchClickChanged
 ========================
 */
- (IBAction) TouchClickChanged {
    Cvar_SetValue( touchClick->name, !touchClick->value );
}

/*
 ========================
 Doom_SettingsMenuViewController::TextMessagesChanged
 ========================
 */
- (IBAction) TextMessagesChanged {
    Cvar_SetValue( messages->name, !messages->value );
}

/*
 ========================
 Doom_SettingsMenuViewController::DrawControlsChanged
 ========================
 */
- (IBAction) DrawControlsChanged {
    Cvar_SetValue( drawControls->name, !drawControls->value );
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
}

/*
 ========================
 Doom_SettingsMenuViewController::RampTurnChanged
 ========================
 */
- (IBAction) RampTurnChanged {
    Cvar_SetValue( rampTurn->name, !rampTurn->value );
}

@end
