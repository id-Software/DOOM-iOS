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

@interface MapMenuView : UIView {
@public
    
    IBOutlet UIScrollView * mapScroller1;
    IBOutlet UIScrollView * mapScroller2;
    IBOutlet UIScrollView * mapScroller3;
    IBOutlet UIScrollView * mapScroller4;
    
    IBOutlet UIButton *     lastElement1;
    IBOutlet UIButton *     lastElement2;
    IBOutlet UIButton *     lastElement3;
    IBOutlet UIButton *     lastElement4;
    
    
    IBOutlet UIImageView *   easySelection;
    IBOutlet UIImageView *   mediumSelection;
    IBOutlet UIImageView *   hardSelection;
    IBOutlet UIImageView *   NightmareSelection;
    
    UIScrollView * selectedScroller;
    
    IBOutlet UIFontLabel *  easySelectionLabel;
    IBOutlet UIFontLabel *  mediumSelectionLabel;
    IBOutlet UIFontLabel *  hardSelectionLabel;
    IBOutlet UIFontLabel *  nightmareSelectionLabel;
    
    IBOutlet UIFontButton * playButton;
    IBOutlet UIFontLabel * playLabel;
    
    UIFontButton * selectedMap;
    int episodeSelected;
    int mapSelected;
    
}

- (int)  getSkill;
- (void) playMap: (int) dataset: (int) episode: (int) map;
- (void) resetDifficulty;

- (void) setEpisode: (int) episode;

-(IBAction)     BackPressed;
-(IBAction)     Play;

-(IBAction)     UpMission;
-(IBAction)     DownMission;

    // Difficulty Setting
-(IBAction)     EasyPressed;
-(IBAction)     MediumPressed;
-(IBAction)     HardPressed;
-(IBAction)     NightmarePressed;


// DOOM EPISODES
-(IBAction)     E1M1;
-(IBAction)     E1M2;
-(IBAction)     E1M3;
-(IBAction)     E1M4;
-(IBAction)     E1M5;
-(IBAction)     E1M6;
-(IBAction)     E1M7;
-(IBAction)     E1M8;
-(IBAction)     E1M9;

-(IBAction)     E2M1;
-(IBAction)     E2M2;
-(IBAction)     E2M3;
-(IBAction)     E2M4;
-(IBAction)     E2M5;
-(IBAction)     E2M6;
-(IBAction)     E2M7;
-(IBAction)     E2M8;
-(IBAction)     E2M9;

-(IBAction)     E3M1;
-(IBAction)     E3M2;
-(IBAction)     E3M3;
-(IBAction)     E3M4;
-(IBAction)     E3M5;
-(IBAction)     E3M6;
-(IBAction)     E3M7;
-(IBAction)     E3M8;
-(IBAction)     E3M9;

-(IBAction)     E4M1;
-(IBAction)     E4M2;
-(IBAction)     E4M3;
-(IBAction)     E4M4;
-(IBAction)     E4M5;
-(IBAction)     E4M6;
-(IBAction)     E4M7;
-(IBAction)     E4M8;
-(IBAction)     E4M9;
    
@end
