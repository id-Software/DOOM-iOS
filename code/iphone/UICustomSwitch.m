/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */


#import "UICustomSwitch.h"


@implementation UICustomSwitch

@synthesize on;

- (void)setOn:(BOOL)turnOn;
{
	on = turnOn;
	
	if (on)
	{
		[ self setHighlighted: YES ];
	}
	else 
	{
		[ self setHighlighted: NO ];
	}
}


- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	[super touchesBegan:touches withEvent:event];
	[self setOn: !on ];
    
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	[super touchesEnded:touches withEvent:event];
    
    if (on)
	{
		[ self setHighlighted: YES ];
	}
	else 
	{
		[ self setHighlighted: NO ];
	}
}


@end
