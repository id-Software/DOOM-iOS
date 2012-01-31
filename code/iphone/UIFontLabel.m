/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */



#import "UIFontLabel.h"


@implementation UIFontLabel

- (void)awakeFromNib {
    CGFloat points = self.font.pointSize;
    
    self.font = [UIFont fontWithName:@"idGinza Narrow" size:points];
}

@end



