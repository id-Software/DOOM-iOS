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

#if TARGET_OS_TV
- (BOOL)canBecomeFocused {
    return YES;
}
#endif


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
    NSLog(@"DOOM: touchesEnded");
    [super touchesBegan:touches withEvent:event];
#if !TARGET_OS_TV
	[self setOn: !on ];
#endif
}


/*
 ========================
 touchesEnded
 ========================
 */
- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
    NSLog(@"DOOM: touchesEnded");
    [super touchesEnded:touches withEvent:event];
#if !TARGET_OS_TV
    if (on)
	{
		[ self setHighlighted: YES ];
	}
	else 
	{
		[ self setHighlighted: NO ];
	}
#endif
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    NSLog(@"DOOM: pressesBegan");
    [super pressesBegan:presses withEvent:event];
#if TARGET_OS_TV
//    [self setOn: !on ];
//    if (on)
//    {
//        [ self setHighlighted: YES ];
//    }
//    else
//    {
//        [ self setHighlighted: NO ];
//    }
#endif
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    NSLog(@"DOOM: pressesEnded");
}

//- (void)drawRect:(CGRect)rect {
//    
//}

@end
