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
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "Label.h"
#include "Localization.h"

@implementation idLabel

/*
 ========================
 awakeFromNib
 ========================
 */
- (void)awakeFromNib {
    [ super awakeFromNib ];
    [ self SetupLabel:@"idGinza Narrow" ];
}


/*
 ========================
 SetupLabel
 ========================
 */
- (void) SetupLabel: ( NSString * )fontName {

    // Set the Font 
    CGFloat points = self.font.pointSize;
    self.font = [UIFont fontWithName:fontName size:points];
    
    // Localize the text.
    NSString * unLocText = self.text;
    const char * utf8string = idLocalization_GetString( [unLocText UTF8String ] );
    self.text = [ NSString stringWithCString: utf8string encoding:NSUTF8StringEncoding ];
}

@end
