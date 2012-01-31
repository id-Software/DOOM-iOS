/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */


#import "UIFontButton.h"


@implementation UIFontButton

@synthesize label;
@synthesize label2;

- (void)awakeFromNib {
    CGFloat points = self.titleLabel.font.pointSize;
    
    self.titleLabel.font = [UIFont fontWithName:@"idGinza Narrow" size:points];
    
    if( self.label2 )
        label2Color = self.label2.textColor;
    if( self.label )
        labelColor = self.label.textColor;
}

- (void)setHighlighted:(BOOL)highlight {
    if( highlight ) {
        if( self.label )
            self.label.textColor = self.label.highlightedTextColor;
        if( self.label2 )
            self.label2.textColor = self.label2.highlightedTextColor;
    } else if( self.enabled ) {
        if( self.label )
            self.label.textColor = labelColor;
        if( self.label2 )
            self.label2.textColor = label2Color;
    }
    
    [super setHighlighted:highlight];
}

- (void)setEnabled:(BOOL)enabled {
    if( !enabled ) {
        if( self.label )
            self.label.textColor = self.label.highlightedTextColor;
        if( self.label2 )
            self.label2.textColor = self.label2.highlightedTextColor;
    } else {
        if( self.label )
            self.label.textColor = labelColor;
        if( self.label2 )
            self.label2.textColor = label2Color;
    }
    
    [super setEnabled:enabled];
}

- (void) Enable {
        
    self.enabled = YES;
    if( self.label )
        label.enabled = YES;
    if( self.label2 )
        label2.enabled = YES;
}

- (void) Disable {
    self.enabled = NO;
    if( self.label )
        label.enabled = NO;
    if( self.label2 )
        label2.enabled = NO;
}

- (void) Hide {

    [UIView beginAnimations:@"Hide" context:nil];
    [UIView setAnimationDuration:1.0f];
    [UIView setAnimationCurve: UIViewAnimationCurveEaseInOut];
    [UIView setAnimationBeginsFromCurrentState:YES];
    [UIView setAnimationDelegate:self];
    [UIView setAnimationDidStopSelector:@selector(Disable)];
    
    self.alpha = 0.0f;
    if( self.label )
        label.alpha = 0.0f;
    if( self.label2 )
        label2.alpha = 0.0f;
    
    [UIView commitAnimations];
    
    
}

- (void) Show {
  
    [UIView beginAnimations:@"Show" context:nil];
    [UIView setAnimationDuration:1.0f];
    [UIView setAnimationCurve: UIViewAnimationCurveEaseInOut];
    [UIView setAnimationBeginsFromCurrentState:YES];
    [UIView setAnimationDelegate:self];
    [UIView setAnimationDidStopSelector:@selector(Enable)];
    
    self.alpha = 1.0f;
    if( self.label )
        label.alpha = 1.0f;
    if( self.label2 )
        label2.alpha = 1.0f;
    
    [UIView commitAnimations];
}

@end
