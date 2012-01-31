/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */


#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "UIFontButton.h"

@interface GenericMenuView : UIView {

    IBOutlet UIScrollView *     scrollView;
    IBOutlet UILabel *          lastItem;
 
    
    IBOutlet UIFontButton *     epi1Button;
    IBOutlet UIFontButton *     epi2Button;
    IBOutlet UIFontButton *     epi3Button;
    IBOutlet UIFontButton *     epi4Button;
    
    int                         episodeSelection;
    IBOutlet UIFontButton *     nextButton;
    IBOutlet UIFontLabel *      nextLabel;
    
}

- (IBAction) BackToMain;
- (IBAction) NextToMissions;


- (IBAction) SelectEpisode1;
- (IBAction) SelectEpisode2;
- (IBAction) SelectEpisode3;
- (IBAction) SelectEpisode4;

@end
