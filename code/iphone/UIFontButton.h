/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */


#import <Foundation/Foundation.h>
#import <UIKit/UIButton.h>
#import "UIFontLabel.h"

@interface UIFontButton : UIButton {
@public

    IBOutlet UIFontLabel * label;
    IBOutlet UIFontLabel * label2;
    
    UIColor *               labelColor;
    UIColor *               label2Color;
}

@property (nonatomic, retain) IBOutlet UIFontLabel *label;
@property (nonatomic, retain) IBOutlet UIFontLabel *label2;

- (void) Hide;
- (void) Show;
- (void) Enable;
- (void) Disable;
@end
