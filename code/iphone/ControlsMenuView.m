/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */



#import "ControlsMenuView.h"
#import "doomAppDelegate.h"
#include "doomiphone.h"

@implementation ControlsMenuView

- (void)     SetOptions {
    
    movestickSize.value = stickMove->value / 255;
    turnstickSize.value = stickTurn->value / 255;
    tiltMoveSpeed.value = tiltMove->value;
    tiltTurnSpeed.value = tiltTurn->value;
    
    if( controlScheme->value == 0 ) {
        singleThumbButton.enabled = NO;
        dualThumbButton.enabled = YES;
        dirWheelButton.enabled = YES;
    } else if( controlScheme->value == 1 ) {
        singleThumbButton.enabled = YES;
        dualThumbButton.enabled = NO;
        dirWheelButton.enabled = YES;
    } else if( controlScheme->value == 2 ) {
        singleThumbButton.enabled = YES;
        dualThumbButton.enabled = YES;
        dirWheelButton.enabled = NO;
    }
    
}

- (void) SetupSlider:(UISlider*)slider minimumTrack:(UIImage*)minImage
        maximumTrack:(UIImage*)maxImage
               thumb:(UIImage*)thumbImage {
	
	[slider setMinimumTrackImage:minImage forState:UIControlStateNormal];
	[slider setMaximumTrackImage:maxImage forState:UIControlStateNormal];
    
	[slider setThumbImage:thumbImage forState:UIControlStateNormal];
	[slider setThumbImage:thumbImage forState:UIControlStateHighlighted];
}

- (void) initialize{
    

    // Minimum track image setup.
	UIImage* minimumTrackImage = [UIImage imageNamed:@"SliderBar.png"];
	CGFloat minimumTrackImageCap = minimumTrackImage.size.width * 0.5f;
    
	UIImage* minimumTrackImageCapped = [minimumTrackImage stretchableImageWithLeftCapWidth:minimumTrackImageCap topCapHeight: 0.0f];
    
    
	// Maximum track image setup.
	UIImage* maximumTrackImage = [UIImage imageNamed:@"SliderBackground.png"];
	CGFloat maximumTrackImageCap = maximumTrackImage.size.width * 0.5f;

	UIImage* maximumTrackImageCapped = [maximumTrackImage stretchableImageWithLeftCapWidth:maximumTrackImageCap topCapHeight: 0.0f];
    
	// Thumb image.
	UIImage* thumbImage = [UIImage imageNamed:@"SliderSkull.png"];
    
	// Set up slider instances.
	[self SetupSlider:movestickSize minimumTrack:minimumTrackImageCapped
         maximumTrack:maximumTrackImageCapped
                thumb:thumbImage];
    
    
	[self SetupSlider:turnstickSize minimumTrack:minimumTrackImageCapped
         maximumTrack:maximumTrackImageCapped
                thumb:thumbImage];
    
    
	
	[self SetupSlider:tiltMoveSpeed minimumTrack:minimumTrackImageCapped
         maximumTrack:maximumTrackImageCapped
                thumb:thumbImage];
    
    
    
    
	[self SetupSlider:tiltTurnSpeed minimumTrack:minimumTrackImageCapped
         maximumTrack:maximumTrackImageCapped
                thumb:thumbImage];
    
    
}

- (void)awakeFromNib {
    
    [self initialize];
    
}

- (id) initWithCoder:(NSCoder *)aCoder{
    
    if(self = [super initWithCoder:aCoder] ) {
        [self initialize];
    }
    
    return self;
}

- (id) initWithFrame:(CGRect)rect{
    if(self = [super initWithFrame:rect] ) {
        [self initialize];
    }
    return self;
}


- (IBAction) BackToMain {
    [gAppDelegate MainMenu];
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}

- (IBAction) HudLayoutPressed {
    
    [ gAppDelegate HUDLayout ];
}

- (IBAction) SingleThumbpadPressed {
    
    Cvar_SetValue( controlScheme->name, 0 );
    HudSetForScheme( 0 );
    
    if( controlScheme->value == 0 ) {
        singleThumbButton.enabled = NO;
        dualThumbButton.enabled = YES;
        dirWheelButton.enabled = YES;
    } else if( controlScheme->value == 1 ) {
        singleThumbButton.enabled = YES;
        dualThumbButton.enabled = NO;
        dirWheelButton.enabled = YES;
    } else if( controlScheme->value == 2 ) {
        singleThumbButton.enabled = YES;
        dualThumbButton.enabled = YES;
        dirWheelButton.enabled = NO;
    }
    
}

- (IBAction) DualThumbpadPressed {
    
    Cvar_SetValue( controlScheme->name, 1 );
    HudSetForScheme( 1 );
    
    if( controlScheme->value == 0 ) {
        singleThumbButton.enabled = NO;
        dualThumbButton.enabled = YES;
        dirWheelButton.enabled = YES;
    } else if( controlScheme->value == 1 ) {
        singleThumbButton.enabled = YES;
        dualThumbButton.enabled = NO;
        dirWheelButton.enabled = YES;
    } else if( controlScheme->value == 2 ) {
        singleThumbButton.enabled = YES;
        dualThumbButton.enabled = YES;
        dirWheelButton.enabled = NO;
    }
}

- (IBAction) DirWheelPressed {
    
    Cvar_SetValue( controlScheme->name, 2 );
    HudSetForScheme( 2 );
    
    if( controlScheme->value == 0 ) {
        singleThumbButton.enabled = NO;
        dualThumbButton.enabled = YES;
        dirWheelButton.enabled = YES;
    } else if( controlScheme->value == 1 ) {
        singleThumbButton.enabled = YES;
        dualThumbButton.enabled = NO;
        dirWheelButton.enabled = YES;
    } else if( controlScheme->value == 2 ) {
        singleThumbButton.enabled = YES;
        dualThumbButton.enabled = YES;
        dirWheelButton.enabled = NO;
    }
}

- (IBAction) MoveStickValChanged {
    
    Cvar_SetValue( stickMove->name, movestickSize.value * 256.0f );
    
}

- (IBAction) TurnStickValChanged {
    
    Cvar_SetValue( stickTurn->name, turnstickSize.value * 256.0f );
}

- (IBAction) TiltMoveValChanged {
    Cvar_SetValue( tiltMove->name, tiltMoveSpeed.value );
    
    if ( tiltMove->value == 100 ) {
		Cvar_SetValue( tiltMove->name, 0 );
        tiltMoveSpeed.value = tiltMove->value;
	}
	if ( tiltMove->value ) {
		Cvar_SetValue( tiltTurn->name, 0 );
        tiltTurnSpeed.value = tiltTurn->value;
	}
    
    
    
}

- (IBAction) TiltTurnValChanged {
    Cvar_SetValue( tiltTurn->name, tiltTurnSpeed.value );
    
    if ( tiltTurn->value == 1500 ) {
		Cvar_SetValue( tiltTurn->name, 0 );
        tiltTurnSpeed.value = tiltTurn->value;
	}
	if ( tiltTurn->value ) {
		Cvar_SetValue( tiltMove->name, 0 );
        tiltMoveSpeed.value = tiltMove->value;
	}
    
    
    
}
@end
