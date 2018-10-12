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
#import "LabelButton.h"
#import "Label.h"
#include "iphone_delegate.h"
#if !TARGET_OS_TV
#import "Slider.h"
#endif
#import "Switch.h"
#import "Carousel.h"
#import "Localization.h"
@implementation idLabelButton

@synthesize label;
@synthesize label2;

#if TARGET_OS_TV
- (BOOL)canBecomeFocused {
    return YES;
}
#endif

/*
 ========================
 idLabelButton::awakeFromNib
 ========================
 */
- (void)awakeFromNib {
    
    // Do not optimize my class out
    [idLabel class ];
#if !TARGET_OS_TV
    [idSlider class];
#endif
    [idSwitch class];
    [idCarousel class];
    
    CGFloat points = self.titleLabel.font.pointSize;
    
    self.titleLabel.font = [UIFont fontWithName:[gAppDelegate GetFontName] size:points];
    
    // Localize the text.
    NSString * unLocText = self.titleLabel.text;
    if( unLocText ) {
        const char * utf8string = idLocalization_GetString( [unLocText UTF8String ] );
        
		NSString * localizedString = [ NSString stringWithCString: utf8string encoding:NSUTF8StringEncoding ];
		[self setTitle: localizedString forState:UIControlStateNormal];
		[self setTitle: localizedString forState:UIControlStateHighlighted];
    }
    
    if( self.label2 )
        label2Color = self.label2.textColor;
    if( self.label ) {
        labelColor = self.label.textColor;
    }

    [super awakeFromNib];
}

/*
 ========================
 idLabelButton::setHighlighted
 ========================
 */
- (void)setHighlighted:(BOOL)highlight {
    if( highlight ) {
        if( self.label )
            self.label.textColor = self.label.highlightedTextColor;
        if( self.label2 )
            self.label2.textColor = self.label2.highlightedTextColor;
    } else if( self.enabled ) {
        if( self.label )
            // don't know what this does, but it's crashing things, so I'm disabling for now -tkidd
//            self.label.textColor = labelColor;
        if( self.label2 )
            self.label2.textColor = label2Color;
    }
    
    [super setHighlighted:highlight];
    
    // Localize the text.
    NSString * unLocText = self.titleLabel.text;
    if( unLocText ) {
        const char * utf8string = idLocalization_GetString( [unLocText UTF8String ] );
        self.titleLabel.text = [ NSString stringWithCString: utf8string encoding:NSUTF8StringEncoding ];
    }
}

/*
 ========================
 idLabelButton::setEnabled
 ========================
 */
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
    
    // Localize the text.
    NSString * unLocText = self.titleLabel.text;
    if( unLocText ) {
        const char * utf8string = idLocalization_GetString( [unLocText UTF8String ] );
        self.titleLabel.text = [ NSString stringWithCString: utf8string encoding:NSUTF8StringEncoding ];
    }
}

/*
 ========================
 idLabelButton::Enable
 ========================
 */
- (void) Enable {
    
    self.enabled = YES;
    if( self.label )
        label.enabled = YES;
    if( self.label2 )
        label2.enabled = YES;
}

/*
 ========================
 idLabelButton::Disable
 ========================
 */
- (void) Disable {
    self.enabled = NO;
    if( self.label )
        label.enabled = NO;
    if( self.label2 )
        label2.enabled = NO;
}

/*
 ========================
 idLabelButton::Hide
 ========================
 */
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

/*
 ========================
 idLabelButton::Show
 ========================
 */
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

- (void) dealloc {
    label = nil;
    label2 = nil;
    
    [super dealloc];
}

@end
