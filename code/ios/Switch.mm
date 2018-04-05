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

#import "Switch.h"


@implementation idSwitch

@synthesize on;

/*
 ========================
 setOn
 ========================
 */
- (void)setOn:(BOOL)turnOn
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

/*
 ========================
 touchesBegan
 ========================
 */
- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	[super touchesBegan:touches withEvent:event];
	[self setOn: !on ];
    
}


/*
 ========================
 touchesEnded
 ========================
 */
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
