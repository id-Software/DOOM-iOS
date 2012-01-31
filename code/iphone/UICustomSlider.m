/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */

#import "UICustomSlider.h"


@implementation UICustomSlider

- (CGRect)trackRectForBounds:(CGRect)bounds {
    
	UIImage* trackImage = [self minimumTrackImageForState:UIControlStateNormal];
	
	CGFloat trackImageHeight = trackImage.size.height;
	
	return CGRectMake(bounds.origin.x, bounds.origin.y, self.bounds.size.width, trackImageHeight);
}

@end
