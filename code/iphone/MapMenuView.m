/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */



#import "MapMenuView.h"
#import "doomAppDelegate.h"

@implementation MapMenuView

- (void) initialize{
    
    [self resetDifficulty ];
    
    [mapScroller1 setContentSize:CGSizeMake(
                                          mapScroller1.bounds.size.width,
                                          CGRectGetMaxY(lastElement1.frame)
                                          )];
    
    [mapScroller2 setContentSize:CGSizeMake(
                                            mapScroller2.bounds.size.width,
                                            CGRectGetMaxY(lastElement2.frame)
                                            )];
    
    [mapScroller3 setContentSize:CGSizeMake(
                                            mapScroller3.bounds.size.width,
                                            CGRectGetMaxY(lastElement3.frame)
                                            )];
    
    [mapScroller4 setContentSize:CGSizeMake(
                                            mapScroller4.bounds.size.width,
                                            CGRectGetMaxY(lastElement4.frame)
                                            )];
    
    [ playButton setEnabled: NO ];
    [ playLabel setEnabled: NO ];
    
    selectedMap = nil;
    episodeSelected = -1;
    mapSelected = -1;
    
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

- (void) resetDifficulty {
    
    easySelection.hidden        = NO;
    easySelectionLabel.hidden   = NO;
    mediumSelection.hidden      = YES;
    mediumSelectionLabel.hidden = YES;
    hardSelection.hidden        = YES;
    hardSelectionLabel.hidden   = YES;
    NightmareSelection.hidden   = YES;
    nightmareSelectionLabel.hidden = YES;

}

- (void) setEpisode: (int) episode {
    
    mapScroller1.alpha = 0.0f;
    mapScroller2.alpha = 0.0f;
    mapScroller3.alpha = 0.0f;
    mapScroller4.alpha = 0.0f;
    
    
    switch( episode ) {
      
        case 0:
            selectedScroller = mapScroller1;
            break;
        case 1:
            selectedScroller = mapScroller2;
            break;
        case 2:
            selectedScroller = mapScroller3;
            break;
        case 3:
            selectedScroller = mapScroller4;
            break;
            
    };
    
    selectedScroller.alpha = 1.0f;
}

-(IBAction)     BackPressed {
    
    // Go Back to MainMenu.
    [gAppDelegate NewGame ];
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
    
}

- (int)  getSkill {
    
    if( easySelection.hidden == NO ) {
        return 0;
    } else if( mediumSelection.hidden == NO ) {
        return 1;
    } else if( hardSelection.hidden == NO ) {
        return 2;
    } else if( NightmareSelection.hidden == NO ) {
        return 3;
    }
    
    return 0;
}

-(IBAction)     UpMission {
    
    
    CGFloat xOffset = selectedScroller.contentOffset.x;
    CGFloat yOffset = selectedScroller.contentOffset.y;
    
    if (selectedScroller.contentOffset.y > 10 )
    {
        [selectedScroller setContentOffset:CGPointMake(xOffset, yOffset - 50 ) animated:YES];
    }
}

-(IBAction)     DownMission {
    
    CGFloat xOffset = selectedScroller.contentOffset.x;
    CGFloat yOffset = selectedScroller.contentOffset.y;
    
    if (selectedScroller.contentOffset.y < 300 )
    {
        [selectedScroller setContentOffset:CGPointMake(xOffset, yOffset + 50 ) animated:YES];
    }
    
}

-(IBAction)     Play {
    
    int skillLevel = [self getSkill];
    
    [gAppDelegate playMap: 0: episodeSelected: mapSelected: skillLevel ];
    
}

- (void) playMap: (int) dataset: (int) episode: (int) map {
    
    [ playButton setEnabled: YES ];
    [ playLabel setEnabled: YES ];
    
    if( selectedMap != nil ) {
        [ selectedMap setEnabled: YES ];
    }
    episodeSelected = episode;
    mapSelected = map;
    
    int mapTag = episode * 10 + ( map - 1 );
    selectedMap = (UIFontButton *)[ self viewWithTag: mapTag ];
    
    [selectedMap setEnabled: NO];
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}

// Difficulty Setting
-(IBAction)     EasyPressed {
    
    [self resetDifficulty];
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}

-(IBAction)     MediumPressed {
        
    easySelection.hidden        = YES;
    mediumSelection.hidden      = NO;
    hardSelection.hidden        = YES;
    NightmareSelection.hidden   = YES;
    
    easySelectionLabel.hidden   = YES;
    mediumSelectionLabel.hidden = NO;
    hardSelectionLabel.hidden   = YES;
    nightmareSelectionLabel.hidden = YES;
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
    
}
-(IBAction)     HardPressed {
        
    easySelection.hidden        = YES;
    mediumSelection.hidden      = YES;
    hardSelection.hidden        = NO;
    NightmareSelection.hidden   = YES;
    
    easySelectionLabel.hidden   = YES;
    mediumSelectionLabel.hidden = YES;
    hardSelectionLabel.hidden   = NO;
    nightmareSelectionLabel.hidden = YES;
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
    
}
-(IBAction)     NightmarePressed{
    
    easySelection.hidden        = YES;
    mediumSelection.hidden      = YES;
    hardSelection.hidden        = YES;
    NightmareSelection.hidden   = NO;
    
    easySelectionLabel.hidden   = YES;
    mediumSelectionLabel.hidden = YES;
    hardSelectionLabel.hidden   = YES;
    nightmareSelectionLabel.hidden = NO;
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}


// DOOM EPISODES
-(IBAction)     E1M1 {
    [ self playMap: 0: 1: 1 ];
}
-(IBAction)     E1M2{
    [ self playMap: 0: 1: 2 ];
}
-(IBAction)     E1M3{
    [ self playMap: 0: 1: 3 ];
}
-(IBAction)     E1M4{
    [ self playMap: 0: 1: 4 ];
}
-(IBAction)     E1M5{
    [ self playMap: 0: 1: 5 ];
}
-(IBAction)     E1M6{
    [ self playMap: 0: 1: 6 ];
}
-(IBAction)     E1M7{
    [ self playMap: 0: 1: 7 ];
}
-(IBAction)     E1M8{
    [ self playMap: 0: 1: 8 ];
}
-(IBAction)     E1M9{
    [ self playMap: 0: 1: 9 ];
}

-(IBAction)     E2M1{
    [ self playMap: 0: 2: 1 ];
}
-(IBAction)     E2M2{
    [ self playMap: 0: 2: 2 ];
}
-(IBAction)     E2M3{
    [ self playMap: 0: 2: 3 ];
}
-(IBAction)     E2M4{
    [ self playMap: 0: 2: 4 ];
}
-(IBAction)     E2M5{
    [ self playMap: 0: 2: 5 ];
}
-(IBAction)     E2M6{
    [ self playMap: 0: 2: 6 ];
}
-(IBAction)     E2M7{
    [ self playMap: 0: 2: 7 ];
}
-(IBAction)     E2M8{
    [ self playMap: 0: 2: 8 ];
}
-(IBAction)     E2M9{
    [ self playMap: 0: 2: 9 ];
}

-(IBAction)     E3M1{
    [ self playMap: 0: 3: 1 ];
}
-(IBAction)     E3M2{
    [ self playMap: 0: 3: 2 ];
}
-(IBAction)     E3M3{
    [ self playMap: 0: 3: 3 ];
}
-(IBAction)     E3M4{
    [ self playMap: 0: 3: 4 ];
}
-(IBAction)     E3M5{
    [ self playMap: 0: 3: 5 ];
}
-(IBAction)     E3M6{
    [ self playMap: 0: 3: 6 ];
}
-(IBAction)     E3M7 {
    [ self playMap: 0: 3: 7 ];
}
-(IBAction)     E3M8{
    [ self playMap: 0: 3: 8 ];
}
-(IBAction)     E3M9{
    [ self playMap: 0: 3: 9 ];
}

-(IBAction)     E4M1{
    [ self playMap: 0: 4: 1 ];
}
-(IBAction)     E4M2{
    [ self playMap: 0: 4: 2 ];
}
-(IBAction)     E4M3{
    [ self playMap: 0: 4: 3 ];
}
-(IBAction)     E4M4{
    [ self playMap: 0: 4: 4 ];
}
-(IBAction)     E4M5{
    [ self playMap: 0: 4: 5 ];
}
-(IBAction)     E4M6{
    [ self playMap: 0: 4: 6 ];
}
-(IBAction)     E4M7{
    [ self playMap: 0: 4: 7 ];
}
-(IBAction)     E4M8{
    [ self playMap: 0: 4: 8 ];
}
-(IBAction)     E4M9{
    [ self playMap: 0: 4: 9 ];
}

@end
