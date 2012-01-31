/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */


#import "GenericMenuView.h"
#import "doomAppDelegate.h"
#include "doomiphone.h"

@implementation GenericMenuView

- (void) initialize{
    
    if( scrollView != nil ) {
    
        [scrollView setContentSize:CGSizeMake(
                                               scrollView.bounds.size.width,
                                               CGRectGetMaxY(lastItem.frame)
                                               )];
    }
    
    episodeSelection = -1;
    [ nextButton setEnabled: NO ];
    [ nextLabel setEnabled: NO ];
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

- (IBAction) NextToMissions {
    
    [gAppDelegate SelectEpisode: episodeSelection ];
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}

- (IBAction) SelectEpisode1 {
    
    [ nextButton setEnabled: YES ];
    [ nextLabel setEnabled: YES ];
    episodeSelection = 0;
    [ epi1Button setEnabled: NO ];
    [ epi2Button setEnabled: YES ];
    [ epi3Button setEnabled: YES ];
    [ epi4Button setEnabled: YES ];
}

- (IBAction) SelectEpisode2 {
    [ nextButton setEnabled: YES ];
    [ nextLabel setEnabled: YES ];
    episodeSelection = 1;
    [ epi1Button setEnabled: YES ];
    [ epi2Button setEnabled: NO ];
    [ epi3Button setEnabled: YES ];
    [ epi4Button setEnabled: YES ];
}

- (IBAction) SelectEpisode3 {
    [ nextButton setEnabled: YES ];
    [ nextLabel setEnabled: YES ];
    episodeSelection = 2;
    [ epi1Button setEnabled: YES ];
    [ epi2Button setEnabled: YES ];
    [ epi3Button setEnabled: NO ];
    [ epi4Button setEnabled: YES ];
}

- (IBAction) SelectEpisode4 {
    [ nextButton setEnabled: YES ];
    [ nextLabel setEnabled: YES ];
    episodeSelection = 3;
    [ epi1Button setEnabled: YES ];
    [ epi2Button setEnabled: YES ];
    [ epi3Button setEnabled: YES ];
    [ epi4Button setEnabled: NO ];
}

@end
